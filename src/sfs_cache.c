#include "sfs_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

inode_table *inode_tb;

free_byte_map *free_bm;

directory *root_directory;

file_descriptor_table *fd_table;

void clear_array(int *arr, int count) {
    for (int i = 0; i < count; ++i) {
        arr[i] = -1;
    }
}

directory_entry *get_dir_entry(int entry) {
    if (entry < 0 || entry >= MAX_NUMBER_OF_DIRECTORY_ENTRIES)
        return NULL;
    return &root_directory->entries[entry];
}

file_descriptor *get_fd(int fd) {
    if (fd < 0 || fd >= MAX_NUMBER_OF_DIRECTORY_ENTRIES)
        return NULL;
    return &fd_table->entries[fd];
}

inode *get_inode(int index) {
    if (index < 0 || index >= INODE_COUNT)
        return NULL;
    return &inode_tb->inodes[index];
}

inode *get_root_inode(void) { return get_inode(ROOT_INODE); }

void clear_root_dir(void) {
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        root_directory->entries[i].inode = 0;
        for (int j = 0; j < MAX_FILE_NAME_SIZE; j++) {
            root_directory->entries[i].name[j] = '\0';
        }
    }
}

void init_inode_table(void) {
    if (inode_tb == NULL)
        inode_tb = (inode_table *)malloc(sizeof(inode_table));
    load_inodes(inode_tb);
}

void init_fbm(bool fresh) {
    if (free_bm == NULL)
        free_bm = (free_byte_map *)malloc(sizeof(free_byte_map));
    if (fresh) {
        for (int i = 0; i < DATA_BLOCK_SIZE; i++) {
            free_bm->map[i] = '0';
        }
        sync_fbm(free_bm);
    } else {
        load_fbm(free_bm);
    }
}

void init_fd_table(void) {
    if (fd_table == NULL)
        fd_table =
            (file_descriptor_table *)malloc(sizeof(file_descriptor_table));
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        file_descriptor *fd = get_fd(i);
        fd->inode = -1;
        fd->op_pointer = 0;
    }
}

void init_root_dir_cache(void) {
    if (root_directory == NULL)
        root_directory = (directory *)malloc(sizeof(directory));
    clear_root_dir();
}

directory_entry *find_dir_entry(const char *name) {
    char str[MAX_FILE_NAME_SIZE];
    memcpy(str, name, MAX_FILE_NAME_SIZE);
    str[MAX_FILE_NAME_SIZE - 1] = '\0';
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        directory_entry *entry = get_dir_entry(i);
        if (entry->inode > 0 && strcmp(str, entry->name) == 0) {
            return entry;
        }
    }
    return NULL;
}

void load_root_dir(void) {
    inode *root_inode = get_root_inode();
    int entries_size = sizeof(directory);
    int max_bytes = MAX_DIR_BYTES_PER_BLOCK;
    char buf[PRE_ALLOCATED_DIR_BLOCKS * max_bytes];
    for (int i = 0; i < PRE_ALLOCATED_DIR_BLOCKS; i++) {
        char block[max_bytes];
        load_data_block(root_inode->direct[i], block, max_bytes);
        memcpy(buf + (i * max_bytes), block, max_bytes);
    }
    memcpy(root_directory->entries, buf, entries_size);
}

void sync_root_dir(void) {
    inode *root_inode = get_root_inode();
    int size = PRE_ALLOCATED_DIR_BLOCKS * BLOCK_SIZE;
    char *buf = malloc(size);
    clear_buffer(buf, size);
    int found = 0;
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        directory_entry *entry = get_dir_entry(i);
        if (entry->inode > 0) {
            memcpy(buf + (found++ * sizeof(directory_entry)), entry,
                   sizeof(directory_entry));
        }
    }

    for (int i = 0; i < PRE_ALLOCATED_DIR_BLOCKS; i++) {
        sync_data_block(root_inode->direct[i],
                        buf + (i * MAX_DIR_BYTES_PER_BLOCK),
                        MAX_DIR_BYTES_PER_BLOCK);
    }
    free(buf);
}

int create_inode(void) {
    int inode_index = -1;
    for (int i = 0; i < INODE_COUNT; i++) {
        if (inode_tb->inodes[i].mode == INODE_MODE_UNUSED) {
            inode_index = i;
            break;
        }
    }
    if (inode_index < 0) {
        printf("Maximum number of files has been reached\n");
        return -1;
    }
    inode *node = get_inode(inode_index);
    node->mode = INODE_MODE_USED;
    node->size = 0;
    node->indirect = -1;
    node->link_cnt = -1;

    clear_array(node->direct, INODE_DIRECT_BLOCK_COUNT);

    return inode_index;
}

int delete_inode(int inode_index) {
    inode *file_inode = get_inode(inode_index);
    if (file_inode == NULL)
        return -1;
    file_inode->size = 0;
    file_inode->mode = INODE_MODE_UNUSED;
    file_inode->link_cnt = -1;

    int blocks_to_free[DATA_BLOCKS_PER_FILE];
    clear_array(blocks_to_free, DATA_BLOCKS_PER_FILE);
    int blocks_used = get_inode_data_blocks(file_inode, blocks_to_free);
    if (file_inode->indirect > -1)
        blocks_to_free[blocks_used++] = file_inode->indirect;
    free_used_blocks(blocks_used, blocks_to_free);
    clear_array(file_inode->direct, INODE_DIRECT_BLOCK_COUNT);
    sync_inodes(inode_tb);
    return 0;
}

int get_inode_data_blocks(inode *node, int *buf) {
    int blocks_used = 0;
    for (int i = 0; i < INODE_DIRECT_BLOCK_COUNT; ++i) {
        if (node->direct[i] < 0)
            return blocks_used;
        buf[blocks_used++] = node->direct[i];
    }

    if (node->indirect < 0)
        return blocks_used;

    index_block index_b;
    load_index_block(node->indirect, &index_b);
    for (int i = 0; i < INDEX_BLOCK_NUM_POINTER; ++i) {
        if (index_b.data[i] < 0)
            return blocks_used;
        buf[blocks_used++] = index_b.data[i];
    }
    return blocks_used;
}

void update_inode_data_blocks(inode *node, const int *buf) {
    int blocks_used = 0;
    for (int i = 0; i < DATA_BLOCKS_CONTENT_PER_FILE; ++i) {
        if (buf[i] > -1)
            blocks_used++;
    }

    int blocks_copied = 0;
    int up_bound = min(INODE_DIRECT_BLOCK_COUNT, blocks_used);

    for (int j = 0; j < up_bound; ++j) {
        node->direct[j] = buf[blocks_copied++];
    }

    if (blocks_copied >= blocks_used) {
        sync_inodes(inode_tb);
        return;
    }

    index_block index_b;
    clear_array(index_b.data, INDEX_BLOCK_NUM_POINTER);

    if (node->indirect < 0) {
        node->indirect = find_one_unused_block();
    }

    int blocks_left = blocks_used - blocks_copied;
    for (int i = 0; i < blocks_left; ++i) {
        index_b.data[i] = buf[blocks_copied++];
    }
    sync_inodes(inode_tb);
    sync_index_block(node->indirect, &index_b);
}

int add_fd(int inode_index, int file_size) {
    // check if exists
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; ++i) {
        file_descriptor *fd = get_fd(i);
        if (fd->inode == inode_index) {
            fd->op_pointer = file_size;
            return i;
        }
    }

    // add
    for (int i = 0; i < MAX_NUMBER_OF_DIRECTORY_ENTRIES; i++) {
        file_descriptor *fd = get_fd(i);
        if (fd->inode == -1) {
            fd->inode = inode_index;
            fd->op_pointer = file_size;
            return i;
        }
    }
    return -1;
}

int find_unused_blocks(int number_blocks, int *blocks) {
    int blocks_found = 0;
    for (int i = 0; i < DATA_BLOCK_SIZE; i++) {
        if (blocks_found >= number_blocks)
            break;
        if (free_bm->map[i] == '0') {
            free_bm->map[i] = '1';
            blocks[blocks_found++] = i;
        }
    }
    sync_fbm(free_bm);
    return blocks_found;
}

int find_one_unused_block(void) {
    int block[SINGLE_BLOCK];
    find_unused_blocks(SINGLE_BLOCK, block);
    return block[0];
}

void free_used_blocks(int number_blocks, const int *blocks) {
    for (int i = 0; i < number_blocks; ++i) {
        int data_block = blocks[i];
        free_bm->map[data_block] = '0';
        clear_data_block(data_block);
    }
    sync_fbm(free_bm);
}

/*
 * Creates the root directory and pre allocates 3 data blocks
 */
void create_root_directory(void) {
    int inode_index = create_inode();
    if (inode_index < 0) {
        printf("Maximum number of files has been reached\n");
        return;
    }
    inode *root_inode = get_inode(inode_index);
    int blocks[PRE_ALLOCATED_DIR_BLOCKS];
    find_unused_blocks(PRE_ALLOCATED_DIR_BLOCKS, blocks);
    for (int i = 0; i < PRE_ALLOCATED_DIR_BLOCKS; i++) {
        root_inode->direct[i] = blocks[i];
    }
    sync_inodes(inode_tb);
}
