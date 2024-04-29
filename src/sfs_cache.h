
#ifndef SFS_CACHE_H
#define SFS_CACHE_H

#include "sfs_disk.h"

extern inode_table *inode_tb;

extern free_byte_map *free_bm;

extern directory *root_directory;

extern file_descriptor_table *fd_table;

void clear_array(int *, int);

/*
 * gets a dir entry by index
 */
directory_entry *get_dir_entry(int);

/*
 * returns the file_descriptor at the given location
 */
file_descriptor *get_fd(int);

/*
 * Returns the inode at a given index
 */
inode *get_inode(int);

/*
 * Returns the root inode (index 0)
 */
inode *get_root_inode(void);

/*
 * Clears the root directory (sets every inode to -1 and sets every name to
 * '\0')
 */
void clear_root_dir(void);

/*
 * Init the inode table
 */
void init_inode_table(void);

/*
 * Init the fbm into the disk
 */
void init_fbm(bool);

/*
 * Init FD table
 */
void init_fd_table(void);

/*
 * Init root dir cache
 */
void init_root_dir_cache(void);

/*
 * Finds a dir entry with given name and returns the index, returns -1 if not
 * found
 */
directory_entry *find_dir_entry(const char *);

/*
 * Loads the root directory from the disk into memory
 */
void load_root_dir(void);

/*
 * Syncs the root directory from memory into the disk
 */
void sync_root_dir(void);

/*
 * Creates an inode at an unused slot, does not sync to the disk
 */
int create_inode(void);

int delete_inode(int);

/*
 * copies the data block numbers from the inode into the buf, returns the number
 * of data blocks used (assumes the size of buf is 268)
 */
int get_inode_data_blocks(inode *node, int *buf);

/*
 * updates the inode with the blocks used by the file
 */
void update_inode_data_blocks(inode *node, const int *buf);

/*
 * Adds an entry to the fd_table
 * Returns the fd for this file (index on in the fd_table)
 * Returns -1 if fd_table is full
 */
int add_fd(int, int);

/*
 * Finds free data blocks using fbm, updates the fbm
 */
int find_unused_blocks(int, int *);

/*
 * find one unused block
 */
int find_one_unused_block(void);

/*
 * Frees data blocks and updates fbm
 */
void free_used_blocks(int, const int *);

/*
 * Creates the root directory and pre allocates 3 data blocks
 */
void create_root_directory(void);

#endif
