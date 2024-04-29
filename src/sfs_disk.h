#ifndef SFS_DISK_H
#define SFS_DISK_H

#include <stdbool.h>

/*
 * File system dimensions
 * Supports 100 files of max size = 268 blocks
 * *Numbers (Filesystem, Super block, Inode table, Data blocks, FBM) are
 * expressed in blocks*
 */

// Filesystem
#define BLOCK_SIZE 1024
#define MAX_BLOCK 26835

// Super block
#define SUPER_BLOCK_ADDRESS 0
#define SUPER_BLOCK_SIZE 1

// Inode table
#define INODE_TABLE_ADDRESS (SUPER_BLOCK_ADDRESS + SUPER_BLOCK_SIZE)
#define INODE_TABLE_SIZE 7

// Data blocks
#define DATA_BLOCK_ADDRESS (INODE_TABLE_ADDRESS + INODE_TABLE_SIZE)
#define DATA_BLOCK_SIZE 26800

// Index blocks
#define INDEX_BLOCK_NUM_POINTER (BLOCK_SIZE / 4)

// FBM
#define FREE_BYTE_MAP_ADDRESS (DATA_BLOCK_ADDRESS + DATA_BLOCK_SIZE)
#define FREE_BYTE_MAP_SIZE 27

/*
 * Filesystem metadata
 */
#define DISK_NAME "disk"
#define ROOT_INODE 0
#define INODE_COUNT 100
#define INODE_DIRECT_BLOCK_COUNT 12
#define MAX_FILE_NAME_SIZE 20
#define MAXFILENAME MAX_FILE_NAME_SIZE
#define MAX_NUMBER_OF_DIRECTORY_ENTRIES (INODE_COUNT - 1)
#define MAX_NUMBER_DIR_ENTRIES_PER_BLOCK 42
#define MAX_DIR_BYTES_PER_BLOCK                                                \
    (MAX_NUMBER_DIR_ENTRIES_PER_BLOCK * sizeof(directory_entry))
#define PRE_ALLOCATED_DIR_BLOCKS 3
#define DATA_BLOCKS_CONTENT_PER_FILE                                           \
    (INODE_DIRECT_BLOCK_COUNT + (BLOCK_SIZE / 4))
#define DATA_BLOCKS_PER_FILE (DATA_BLOCKS_CONTENT_PER_FILE + 1)
#define MAX_BYTES_PER_FILE (DATA_BLOCKS_CONTENT_PER_FILE * BLOCK_SIZE)
/*
 * Inode modes
 */
#define INODE_MODE_UNUSED 0
#define INODE_MODE_USED 1

/*
 * Misc
 */
#define SINGLE_BLOCK 1

/*
 * Super block def
 */
typedef struct {
    int magic;
    int block_size;
    int num_blocks;
    int num_inode_blocks;
    int root_inode;
} super_block;

/*
 * Inode def
 */
typedef struct {
    int mode;
    int link_cnt;
    int size;
    int direct[INODE_DIRECT_BLOCK_COUNT];
    int indirect;
} inode;

/*
 * Inode table
 */
typedef struct {
    inode inodes[INODE_COUNT];
} inode_table;

/*
 * Index block
 */
typedef struct {
    int data[INDEX_BLOCK_NUM_POINTER];
} index_block;

/*
 * Free byte map
 */
typedef struct {
    char map[DATA_BLOCK_SIZE];
} free_byte_map;

/*
 * Directory type defs
 */
typedef struct {
    char name[MAX_FILE_NAME_SIZE];
    int inode;
} directory_entry;

typedef struct {
    directory_entry entries[MAX_NUMBER_OF_DIRECTORY_ENTRIES];
} directory;

/*
 * FD
 */
typedef struct {
    int inode;
    int op_pointer;
} file_descriptor;

typedef struct {
    file_descriptor entries[MAX_NUMBER_OF_DIRECTORY_ENTRIES];
} file_descriptor_table;

void clear_buffer(char *, int);

int divide_round_up(int, int);

int min(int, int);

/*
 * wrapper over init
 */
void disk_init(bool);

/**
 *
 * Reads an object from the disk (deserialize)
 */
void deserialize(void *, int, int, int);

/**
 *
 * Writes an object into the disk (serialize)
 */
void serialize(void *, int, int, int);

/*
 * Load one data block
 */
void load_data_block(int, void *, int);

/*
 * Sync one data block
 */
void sync_data_block(int, void *, int);

/*
 * Loads an index block
 */
void load_index_block(int, index_block *);

/*
 * Syncs an index block
 */
void sync_index_block(int, index_block *);

/*
 * writes 0s to a block to clear it
 */
void clear_data_block(int);

/*
 * loads the inode table from the disk into the cache
 */
void load_inodes(inode_table *);

/*
 * Sync the inode cache into the disk
 */
void sync_inodes(inode_table *);

/*
 * Sync a memory fbm into the disk
 */
void sync_fbm(free_byte_map *);

/*
 * Load the fbm from the disk into memory
 */
void load_fbm(free_byte_map *);

/*
 * Load the super block from the disk into memory
 */
void load_super_block(super_block *);

/*
 * Sync a memory super block into the disk
 */
void sync_super_block(super_block *);

/*
 * Clear index block
 */
void clear_index_block(index_block *);

/*
 * Init the super block in the disk
 */
void init_super_block(void);

#endif
