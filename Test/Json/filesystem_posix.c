#include "filesystem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

struct AftFile
{
    int descriptor;
    void* allocator;
};


void aft_close_file(AftFile* file)
{
    if(!file)
    {
        return;
    }

    close(file->descriptor);

    AftMemoryBlock block = {file, sizeof(AftFile)};
    aft_deallocate(file->allocator, block);
}

AftMaybeUint64 aft_get_file_size(AftFile* file)
{
    AftMaybeUint64 result;
    result.valid = false;

    struct stat status;
    int stat_result = fstat(file->descriptor, &status);
    if(stat_result == 0)
    {
        result.value = (uint64_t) status.st_size;
        result.valid = true;
    }

    return result;
}

AftFile* aft_open_file(const AftPath* path, AftOpenFileMode open_mode, void* allocator)
{
    int flag;
    switch(open_mode)
    {
        case AFT_OPEN_FILE_MODE_READ:
        {
            flag = O_RDONLY;
            break;
        }
        case AFT_OPEN_FILE_MODE_WRITE:
        {
            flag = O_WRONLY;
            break;
        }
    }

    const char* path_contents = aft_path_get_contents_const(path);

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int descriptor = open(path_contents, flag, mode);
    if(descriptor == -1)
    {
        return NULL;
    }

    AftMemoryBlock block = aft_allocate(allocator, sizeof(AftFile));
    if(!block.memory)
    {
        close(descriptor);
        return NULL;
    }

    AftFile* file = block.memory;
    file->allocator = allocator;
    file->descriptor = descriptor;

    return file;
}

AftReadFileResult aft_read_file_sync(AftFile* file, void* data, uint64_t bytes)
{
    AftReadFileResult result = {0};
    ssize_t bytes_read = read(file->descriptor, data, bytes);

    if(bytes_read != -1)
    {
        result.bytes = bytes_read;
        result.success = true;
    }

    return result;
}

bool aft_write_file_sync(AftFile* file, const void* data, uint64_t bytes)
{
    int64_t written = write(file->descriptor, data, bytes);
    return ((uint64_t) written) == bytes;
}

void aft_write_to_error_output(AftStringSlice slice)
{
    int flag = O_WRONLY;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int descriptor = open("/proc/self/fd/2", flag, mode);
    if(descriptor != -1)
    {
        const char* contents = aft_string_slice_start(slice);
        int count = aft_string_slice_count(slice);
        write(descriptor, contents, count);
        close(descriptor);
    }
}

void aft_write_to_standard_output(AftStringSlice slice)
{
    int flag = O_WRONLY;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int descriptor = open("/proc/self/fd/1", flag, mode);
    if(descriptor != -1)
    {
        const char* contents = aft_string_slice_start(slice);
        int count = aft_string_slice_count(slice);
        write(descriptor, contents, count);
        close(descriptor);
    }
}

AftMaybePath aft_get_executable_directory(void* allocator)
{
    AftMaybePath result;
    result.valid = false;
    aft_path_initialise_with_allocator(&result.value, allocator);

    AftMemoryBlock block = {NULL, 0};
    char* path = NULL;
    int path_size = 128;
    int chars_copied;
    do
    {
        path_size *= 2;

        aft_deallocate(allocator, block);
        block = aft_allocate(allocator, path_size + 1);

        if(!block.memory)
        {
            return result;
        }

        path = block.memory;

        chars_copied = readlink("/proc/self/exe", path, path_size);
        if(chars_copied == -1 || chars_copied > path_size)
        {
            aft_deallocate(allocator, block);
            return result;
        }
    } while(chars_copied == path_size);

    path[chars_copied] = '\0';

    result = aft_path_from_c_string_with_allocator(path, allocator);
    aft_deallocate(allocator, block);

    aft_path_remove_basename(&result.value);

    return result;
}
