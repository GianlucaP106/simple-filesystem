# Mountable simple filesystem

## Project overview
This is a implementation of a simple, mountable filesystem. It uses a regular file as the disk. This project follows type of layered architecture. The disk_emu set of methods is given, the sfs_disk are the methods that interact with disk_emu, the sfs_cache interacts with sfs_disk and sfs_api interacts with sfs_cache.

## Run the filesystem

**Run the following command to enter the container**
```bash
docker build -t sfs . && docker run -it --cap-add SYS_ADMIN --device /dev/fuse sfs
```

**Run the following commands in the container**
```bash
# To compile
make

# To create the directory the fs will mount to
mkdir myfs

# Will run the fs and mount
./sfs myfs

# Now you can create new things in the directory and it is stored in the "disk" file
echo "Hello world!!!!!!" > myfs/hello.txt
ls myfs

# to see the content of the disk
cat disk
```

>Note that some commads will not work within the filesystem (such as touch) because it is not implemented.
Also only files can be created (not directories)

**The program will wipe the disk fresh at each run, so to reuse the disk do the following**

```bash
# Change the `SOURCES` in the Makefile to include the `fuse_wrap_existing_fs.c`.

# Recompile
make

# Rerun
./sfs myfs
```


## Architecture overview

### sfs_disk
Responsible for exposing methods that interact with the disk. Serialization, deserialization and methods to sync and load structures. It also contains the structs and macros that will be useful for the entire project.  

### sfs_cache
Responsible for exposing methods that manage the in memory data structures (caches). It manages the inode table, root dir, FBM and fd table (the fd table is not a cache... not synced to disk)

### sfs_api
Responsible for exposing the high level API to power the fs. Interacts with the caches to create, delete, read, write files...


## Filesystem dimensions

### Restrictions and limitations
Max number of files: 99

Max Number of open files: 99

Max file size: 274432 B or 0.274432 MB

Disk size: 27479040 B or 27.47904 MB

Max file name size: 20 characters or 20 B


### Disk structure
#### Super Block
1 block.

#### Inode table
7 blocks. 1 inode is 64 bytes and 100 inodes are needed.

#### Data blocks
26800 blocks. 99 Inodes needed with a max size of 269 blocks (268 data, 1 index), 3 blocks needed for the directory. (There are data blocks left over for future implementation)

#### FBM
27 blocks. The FBM uses one byte to represent a free data block. 26800 bytes are needed, hence 27 blocks.

#### Total: 26835 blocks

