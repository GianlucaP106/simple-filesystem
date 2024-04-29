#include "sfs_api.h"
#include <stdio.h>
#include <string.h>

/*
 * next file name index
 * (for the getnextfilename function)
 */
int current_file_name_index;

/*
 * Closes the file in the fd_table
 */
int close_file(int fd) {
    file_descriptor *_fd = get_fd(fd);
    if (_fd == NULL)
        return -1;
    if (_fd->inode < 0)
        return -1;
    _fd->inode = -1;
    _fd->op_pointer = 0;
    return 0;
}

int write_file(int _fd, const char *_buf, int _length) {
    file_descriptor *fd = get_fd(_fd);
    if (fd == NULL || fd->inode == -1 || _length <= 0) {
        return -1;
    }

    inode *file_inode = get_inode(fd->inode);
    int used_blocks[DATA_BLOCKS_CONTENT_PER_FILE];
    clear_array(used_blocks, DATA_BLOCKS_CONTENT_PER_FILE);
    get_inode_data_blocks(file_inode, used_blocks);

    // block calc
    int current_block_number = fd->op_pointer / BLOCK_SIZE;
    int current_byte = fd->op_pointer % BLOCK_SIZE;
    int length_remaining = _length;
    int out_length = _length;
    const char *buf = _buf;

    // update size and pointer
    int new_pointer = fd->op_pointer + _length;
    if (new_pointer > MAX_BYTES_PER_FILE) {
        file_inode->size = MAX_BYTES_PER_FILE;
        out_length = file_inode->size - fd->op_pointer;
        length_remaining = out_length;
    } else {
        fd->op_pointer = new_pointer;
        file_inode->size = new_pointer;
    }

    // current block alloc
    if (current_byte > 0) {
        int remaining_in_current_block = BLOCK_SIZE - current_byte;
        int amount_to_copy_first_block =
            min(remaining_in_current_block, length_remaining);
        char first_block_buf[BLOCK_SIZE];
        clear_buffer(first_block_buf, BLOCK_SIZE);
        load_data_block(used_blocks[current_block_number], first_block_buf,
                        current_byte);
        memcpy(first_block_buf + current_byte, buf, amount_to_copy_first_block);
        sync_data_block(used_blocks[current_block_number++], first_block_buf,
                        BLOCK_SIZE);
        length_remaining = length_remaining - amount_to_copy_first_block;
        buf = buf + amount_to_copy_first_block;
    }

    if (length_remaining <= 0)
        return out_length;

    int blocks_needed = divide_round_up(length_remaining, BLOCK_SIZE);
    int upper_bound = current_block_number + blocks_needed;
    int index = 0;
    for (int i = current_block_number; i < upper_bound; ++i) {
        if (used_blocks[i] < 0)
            used_blocks[i] = find_one_unused_block();
        int to_copy = min(length_remaining, BLOCK_SIZE);
        char block_buf[to_copy];
        memcpy(block_buf, buf + (index++ * BLOCK_SIZE), to_copy);
        sync_data_block(used_blocks[i], block_buf, to_copy);
        length_remaining = length_remaining - to_copy;
    }

    update_inode_data_blocks(file_inode, used_blocks);
    return out_length;
}

int read_file(int _fd, char *_buf, int _length) {
    file_descriptor *fd = get_fd(_fd);
    if (fd == NULL)
        return -1;
    inode *file_inode = get_inode(fd->inode);
    if (file_inode == NULL)
        return -1;
    int used_blocks[DATA_BLOCKS_CONTENT_PER_FILE];
    clear_array(used_blocks, DATA_BLOCKS_CONTENT_PER_FILE);
    get_inode_data_blocks(file_inode, used_blocks);

    // block calc
    int current_block_number = fd->op_pointer / BLOCK_SIZE;
    int current_byte = fd->op_pointer % BLOCK_SIZE;
    int length_remaining = _length;
    int out_length = length_remaining;
    char *buf = _buf;

    int new_pointer = fd->op_pointer + out_length;
    if (new_pointer > file_inode->size) {
        out_length = file_inode->size - fd->op_pointer;
        fd->op_pointer = file_inode->size;
        length_remaining = out_length;
    } else {
        fd->op_pointer = new_pointer;
    }

    if (current_byte > 0) {
        int amount_to_read = min(BLOCK_SIZE - current_byte, length_remaining);
        char first_block_buf[BLOCK_SIZE];
        load_data_block(used_blocks[current_block_number++], first_block_buf,
                        BLOCK_SIZE);
        memcpy(buf, first_block_buf + current_byte, amount_to_read);
        buf = buf + amount_to_read;
        length_remaining = length_remaining - amount_to_read;
    }

    if (length_remaining > 0) {
        int blocks_to_read = divide_round_up(length_remaining, BLOCK_SIZE);
        int upper_bound = current_block_number + blocks_to_read;
        int index = 0;
        for (int i = current_block_number; i < upper_bound; ++i) {
            int to_read = min(length_remaining, BLOCK_SIZE);
            char block_buf[to_read];
            load_data_block(used_blocks[i], block_buf, to_read);
            memcpy(buf + (index++ * BLOCK_SIZE), block_buf, to_read);
            length_remaining = length_remaining - to_read;
        }
    }
    return out_length;
}

/*
 * Deletes a file and frees all data, inodes, and dir entry
 */
int delete_file(char *file) {
    directory_entry *entry = find_dir_entry(file);
    if (entry == NULL)
        return -1;
    delete_inode(entry->inode);
    entry->inode = 0;
    for (int i = 0; i < MAX_FILE_NAME_SIZE; ++i) {
        entry->name[i] = '\0';
    }
    sync_root_dir();
    return 0;
}

/*
 * Checks to see if the file exists in the dir and returns
 * the existing inode, otherwise creates the file
 * Creates a file and stores it in root dir
 * Increases the size in the root dir inode by 1
 * Syncs to the disk (inode and root dir)
 * Returns the fd of the file
 */
int open_file(const char *name) {
    int name_length = (int)strlen(name);
    if (name_length >= MAX_FILE_NAME_SIZE)
        return -1;

    inode *root_inode = get_root_inode();

    // return fd right away if file exists in root dir
    directory_entry *existing = find_dir_entry(name);
    if (existing != NULL) {
        inode *existing_inode = get_inode(existing->inode);
        int file_size = existing_inode->size;
        return add_fd(existing->inode, file_size);
    }

    // TODO: FIX SIZE CHECK
    // create a new inode and increase size of root dir
    int inode_index = create_inode();
    root_inode->size++;

    // add dir entry
    int upper_bound = min(name_length, MAX_FILE_NAME_SIZE - 1);
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        directory_entry *entry = get_dir_entry(i);
        if (entry->inode <= 0) {
            entry->inode = inode_index;
            for (int j = 0; j < upper_bound; j++) {
                entry->name[j] = name[j];
            }
            entry->name[upper_bound] = '\0';
            break;
        }
    }
    sync_root_dir();
    sync_inodes(inode_tb);
    return add_fd(inode_index, 0);
}

/*
 * #############
 * ## SFS API ##
 * #############
 */

void mksfs(int fresh) {
    current_file_name_index = 0;
    if (fresh) {
        disk_init(fresh);
        init_super_block();
        init_fbm(fresh);
        init_inode_table();
        init_root_dir_cache();
        create_root_directory();
        init_fd_table();
    } else {
        disk_init(fresh);
        init_fbm(fresh);
        init_inode_table();
        init_root_dir_cache();
        load_root_dir();
        init_fd_table();
    }
}

int sfs_getnextfilename(char *name) {
    int index = current_file_name_index;
    directory_entry *entry = get_dir_entry(index);
    while (true) {
        if (entry != NULL && entry->inode > 0)
            break;
        if (index >= MAX_NUMBER_OF_DIRECTORY_ENTRIES) {
            current_file_name_index = 0;
            return 0;
        }
        entry = get_dir_entry(++index);
    }
    current_file_name_index = index + 1;
    strcpy(name, entry->name);
    return 1;
}

int sfs_getfilesize(const char *path) {
    directory_entry *file = find_dir_entry(path);
    if (file == NULL)
        return -1;
    inode *file_inode = get_inode(file->inode);
    return file_inode->size;
}

int sfs_fopen(char *name) { return open_file(name); }

int sfs_fclose(int fileId) { return close_file(fileId); }

int sfs_fwrite(int fileId, const char *buf, int length) {
    return write_file(fileId, buf, length);
}

int sfs_fread(int fileId, char *buf, int length) {
    return read_file(fileId, buf, length);
}

int sfs_fseek(int fileId, int loc) {
    file_descriptor *fd = get_fd(fileId);
    fd->op_pointer = loc;
    return 0;
}

int sfs_remove(char *file) { return delete_file(file); }
