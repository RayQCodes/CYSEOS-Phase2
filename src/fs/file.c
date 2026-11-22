
#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "kernel.h"
#include "fat/fat16.h"
#include "string/string.h"
#include "disk/disk.h"

// We have a maximum total filesystems as specified in our config file.
struct filesystem* filesystems[PEACHOS_MAX_FILESYSTEMS];

struct file_descriptor* file_descriptors[PEACHOS_MAX_FILE_DESCRIPTORS];

// Gets a free filesystem slot.
static struct filesystem** fs_get_free_filesystem()
{
    int i = 0;

    for (i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

// Registers a filesystem with the VFS layer.
void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;

    fs = fs_get_free_filesystem();

    if (!fs)
    {
        print("Problem inserting filesystem");
        while (1) {}
    }

    *fs = filesystem;
}

// Loads built-in filesystem drivers.
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

// Loads all filesystem implementations.
void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

// Initializes VFS state.
void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

// Creates a new file descriptor.
static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;

    for (int i = 0; i < PEACHOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc =
                kzalloc(sizeof(struct file_descriptor));

            if (!desc)
            {
                res = -ENOMEM;
                break;
            }

            // Descriptors start at 1.
            desc->index = i + 1;

            file_descriptors[i] = desc;
            *desc_out = desc;

            res = 0;
            break;
        }
    }

    return res;
}

// Returns a descriptor based on its descriptor number.
static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd > PEACHOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    // Descriptors start at 1.
    int index = fd - 1;

    return file_descriptors[index];
}

// Returns the filesystem capable of reading this disk.
struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = 0;

    for (int i = 0; i < PEACHOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != 0 &&
            filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

// Converts a mode string into a FILE_MODE.
FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;

    if (strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if (strncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if (strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

// Opens a file using the appropriate filesystem driver.
int fopen(const char* filename, const char* mode_str)
{
    int res = 0;

    struct path_root* root_path =
        pathparser_parse(filename, NULL);

    if (!root_path)
    {
        res = -EINVARG;
        goto out;
    }

    // Cannot open only the root, for example "0:/".
    if (!root_path->first)
    {
        res = -EINVARG;
        goto out;
    }

    struct disk* disk =
        disk_get(root_path->drive_no);

    if (!disk)
    {
        res = -EIO;
        goto out;
    }

    if (!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode =
        file_get_mode_by_string(mode_str);

    if (mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    void* descriptor_private_data =
        disk->filesystem->open(
            disk,
            root_path->first,
            mode
        );

    if (ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;

    res = file_new_descriptor(&desc);

    if (res < 0)
    {
        goto out;
    }

    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;

    res = desc->index;

out:

    // fopen should not return negative values.
    if (res < 0)
    {
        res = 0;
    }

    return res;
}

// Reads data from an opened file.
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;

    if (size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc =
        file_get_descriptor(fd);

    if (!desc)
    {
        res = -EINVARG;
        goto out;
    }

    if (!desc->filesystem || !desc->filesystem->read)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->read(
        desc->disk,
        desc->private,
        size,
        nmemb,
        (char*)ptr
    );

out:
    return res;
}

// Changes the current position in an opened file.
int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;

    struct file_descriptor* desc =
        file_get_descriptor(fd);

    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    if (!desc->filesystem || !desc->filesystem->seek)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->seek(
        desc->private,
        offset,
        whence
    );

out:
    return res;
}

// Retrieves statistics for an opened file.
int fstat(int fd, struct file_stat* stat)
{
    int res = 0;

    struct file_descriptor* desc =
        file_get_descriptor(fd);

    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    if (!desc->filesystem || !desc->filesystem->stat)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->stat(
        desc->disk,
        desc->private,
        stat
    );

out:
    return res;
}

// Closes an opened file using the underlying filesystem driver.
int fclose(int fd)
{
    int res = 0;

    struct file_descriptor* desc =
        file_get_descriptor(fd);

    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    if (!desc->filesystem || !desc->filesystem->close)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->close(desc->private);

out:
    return res;
}


