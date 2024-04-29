#include "sfs_disk.h"
#include "disk_emu.h"
#include <stdlib.h>
#include <string.h>

void clear_buffer(char *buf, int size) {
    for (int i = 0; i < size; ++i) {
        buf[i] = 0;
    }
}

int divide_round_up(int a1, int a2) { return (a1 + a2 - 1) / a2; }

int min(int a1, int a2) {
    if (a1 < a2)
        return a1;
    return a2;
}

void disk_init(bool fresh) {
    if (fresh)
        init_fresh_disk(DISK_NAME, BLOCK_SIZE, MAX_BLOCK);
    else
        init_disk(DISK_NAME, BLOCK_SIZE, MAX_BLOCK);
}

void deserialize(void *obj, int obj_size, int start_address, int num_blocks) {
    int size = num_blocks * BLOCK_SIZE;
    char *buffer = malloc(size);
    clear_buffer(buffer, size);
    read_blocks(start_address, num_blocks, buffer);
    memcpy(obj, buffer, obj_size);
    free(buffer);
}

void serialize(void *obj, int obj_size, int start_address, int num_blocks) {
    int size = num_blocks * BLOCK_SIZE;
    char *buffer = malloc(size);
    clear_buffer(buffer, size);
    memcpy(buffer, obj, obj_size);
    write_blocks(start_address, num_blocks, buffer);
    free(buffer);
}

void load_data_block(int block_number, void *buf, int buf_size) {
    deserialize(buf, buf_size, DATA_BLOCK_ADDRESS + block_number, SINGLE_BLOCK);
}

void sync_data_block(int block_number, void *buf, int buf_size) {
    serialize(buf, buf_size, DATA_BLOCK_ADDRESS + block_number, SINGLE_BLOCK);
}

void load_index_block(int block_number, index_block *block) {
    load_data_block(block_number, block, sizeof(index_block));
}

void sync_index_block(int block_number, index_block *block) {
    sync_data_block(block_number, block, sizeof(index_block));
}

void clear_data_block(int block_number) {
    char buf[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        buf[i] = '0';
    }
    sync_data_block(block_number, buf, BLOCK_SIZE);
}

void load_inodes(inode_table *inode_tb) {
    deserialize(inode_tb->inodes, INODE_COUNT * sizeof(inode),
                INODE_TABLE_ADDRESS, INODE_TABLE_SIZE);
}

void sync_inodes(inode_table *inode_tb) {
    serialize(inode_tb->inodes, INODE_COUNT * sizeof(inode),
              INODE_TABLE_ADDRESS, INODE_TABLE_SIZE);
}

void sync_fbm(free_byte_map *free_bm) {
    serialize(free_bm->map, DATA_BLOCK_SIZE, FREE_BYTE_MAP_ADDRESS,
              FREE_BYTE_MAP_SIZE);
}

void load_fbm(free_byte_map *free_bm) {
    deserialize(free_bm->map, DATA_BLOCK_SIZE, FREE_BYTE_MAP_ADDRESS,
                FREE_BYTE_MAP_SIZE);
}

void load_super_block(super_block *block) {
    deserialize(block, sizeof(super_block), SUPER_BLOCK_ADDRESS,
                SUPER_BLOCK_SIZE);
}

void sync_super_block(super_block *block) {
    serialize(block, sizeof(super_block), SUPER_BLOCK_ADDRESS,
              SUPER_BLOCK_SIZE);
}

void clear_index_block(index_block *block) {
    for (int i = 0; i < INDEX_BLOCK_NUM_POINTER; ++i) {
        block->data[i] = -1;
    }
}

void init_super_block(void) {
    super_block superBlock = {123, BLOCK_SIZE, MAX_BLOCK, INODE_TABLE_SIZE,
                              ROOT_INODE};
    sync_super_block(&superBlock);
}
