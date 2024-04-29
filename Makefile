CFLAGS = -c -g -ansi -pedantic -Wall -std=gnu99 `pkg-config fuse --cflags --libs`

LDFLAGS = `pkg-config fuse --cflags --libs`

# Uncomment on of the following three lines to compile

# Tests
# SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h main_test.c
# SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h sfs_test0.c
# SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h sfs_test1.c
# SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h sfs_test2.c

# FS
# SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h fuse_wrap_existing_fs.c
SOURCES= src/disk_emu.c src/sfs_disk.c src/sfs_cache.c src/sfs_api.c src/disk_emu.h src/sfs_disk.h src/sfs_cache.h src/sfs_api.h fuse_wrap_new_fs.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=sfs

all: $(SOURCES) $(HEADERS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) $(LDFLAGS) -o $@
	# gcc $(OBJECTS) $(LDFLAGS) #-o $@

.c.o:
	gcc $(CFLAGS) $< -o $@

clean:
#	rm -rf *.gch *.o *~ $(EXECUTABLE)
	rm -rf src/*.gch src/*.o *.o *~ $(EXECUTABLE)
