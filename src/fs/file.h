
#ifndef FILE_H
#define FILE_H

#include "pparser.h"
#include <stdint.h>

typedef unsigned int FILE_SEEK_MODE;

enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int FILE_MODE;

enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

enum
{
    FILE_STAT_READ_ONLY = 0b00000001
};

typedef unsigned int FILE_STAT_FLAGS;

struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};

struct disk;

// These are function pointers to be inherited by a particular filesystem driver.
typedef void* (*FS_OPEN_FUNCTION)(
    struct disk* disk,
    struct path_part* path,
    FILE_MODE mode
);

typedef int (*FS_RESOLVE_FUNCTION)(
    struct disk* disk
);

typedef int (*FS_READ_FUNCTION)(
    struct disk* disk,
    void* private,
    uint32_t size,
    uint32_t nmemb,
    char* out
);

typedef int (*FS_SEEK_FUNCTION)(
    void* private,
    uint32_t offset,
    FILE_SEEK_MODE seek_mode
);

typedef int (*FS_STAT_FUNCTION)(
    struct disk* disk,
    void* private,
    struct file_stat* stat
);

typedef int (*FS_CLOSE_FUNCTION)(
    void* private
);

struct filesystem
{
    // Return zero if this filesystem can read the provided disk.
    FS_RESOLVE_FUNCTION resolve;

    // Called when opening a file in this filesystem.
    FS_OPEN_FUNCTION open;

    // Called when reading from a file.
    FS_READ_FUNCTION read;

    // Called when seeking inside an open file.
    FS_SEEK_FUNCTION seek;

    // Called when retrieving file statistics.
    FS_STAT_FUNCTION stat;

    // Called when closing an open file.
    FS_CLOSE_FUNCTION close;

    // Filesystem name, for example FAT16.
    char name[20];
};

struct file_descriptor
{
    // Descriptor index.
    int index;

    // Filesystem this file belongs to.
    struct filesystem* filesystem;

    // Private filesystem-specific data.
    void* private;

    // Disk this descriptor belongs to.
    struct disk* disk;
};

void fs_init();

int fopen(
    const char* filename,
    const char* mode_str
);

int fread(
    void* ptr,
    uint32_t size,
    uint32_t nmemb,
    int fd
);

int fseek(
    int fd,
    int offset,
    FILE_SEEK_MODE whence
);

int fstat(
    int fd,
    struct file_stat* stat
);

int fclose(int fd);

void fs_insert_filesystem(
    struct filesystem* filesystem
);

struct filesystem* fs_resolve(
    struct disk* disk
);

#endif

