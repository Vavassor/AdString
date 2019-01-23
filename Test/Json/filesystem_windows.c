#include "filesystem.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <assert.h>


#define ASSERT(expression) assert(expression)


struct AftFile
{
    HANDLE handle;
    void* allocator;
};


static wchar_t* utf8_to_wchar_string(const AftString* string, void* allocator)
{
    const char* contents = aft_string_get_contents_const(string);
    int count = MultiByteToWideChar(CP_UTF8, 0, contents, -1, NULL, 0);

    AftMemoryBlock block = aft_allocate(allocator, sizeof(wchar_t) * count);
    if(!block.memory)
    {
        return NULL;
    }

    wchar_t* result = block.memory;

    count = MultiByteToWideChar(CP_UTF8, 0, contents, -1, result, count);
    if(count == 0)
    {
        aft_deallocate(allocator, block);
        return NULL;
    }

    return result;
}

static AftMaybeString wchar_string_to_utf8(const wchar_t* string, void* allocator)
{
    AftMaybeString result;
    result.valid = false;
    aft_string_initialise_with_allocator(&result.value, allocator);

    int bytes = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL);

    AftMemoryBlock block = aft_allocate(allocator, bytes);
    bytes = WideCharToMultiByte(CP_UTF8, 0, string, -1, block.memory, (int) block.bytes, NULL, NULL);

    if(bytes == 0)
    {
        aft_deallocate(allocator, block);
        return result;
    }

    AftStringSlice slice = {block.memory, (int) block.bytes};
    result = aft_string_from_slice_with_allocator(&slice, allocator);

    aft_deallocate(allocator, block);

    return result;
}

static int wchar_string_size(const wchar_t* string)
{
    int count = 0;
    for (; string[count]; count += 1);
    return count;
}


void aft_close_file(AftFile* file)
{
    CloseHandle(file->handle);

    AftMemoryBlock block = {file, sizeof(AftFile)};
    aft_deallocate(file->allocator, block);
}

AftMaybeUint64 aft_get_file_size(AftFile* file)
{
    AftMaybeUint64 result;
    result.valid = false;

    BY_HANDLE_FILE_INFORMATION info;
    BOOL got = GetFileInformationByHandle(file->handle, &info);
    if (!got)
    {
        return result;
    }

    result.value = (uint64_t) info.nFileSizeHigh << 32 | info.nFileSizeLow;
    result.valid = true;

    return result;
}

AftFile* aft_open_file(const AftPath* path, AftOpenFileMode mode, void* allocator)
{
    DWORD access;
    DWORD share_mode = 0;
    DWORD disposition;
    DWORD flags = FILE_ATTRIBUTE_NORMAL;

    switch(mode)
    {
        case AFT_OPEN_FILE_MODE_READ:
        {
            access = GENERIC_READ;
            disposition = OPEN_EXISTING;
            break;
        }
        case AFT_OPEN_FILE_MODE_WRITE:
        {
            access = GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            break;
        }
    }

    wchar_t* wide_path = utf8_to_wchar_string(aft_path_get_string_const(path), allocator);

    HANDLE handle = CreateFileW(wide_path, access, share_mode, NULL, disposition, flags, NULL);

    AftMemoryBlock wide_path_block = {wide_path, sizeof(wchar_t) * (wchar_string_size(wide_path) + 1)};
    aft_deallocate(allocator, wide_path_block);

    if(handle == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    AftMemoryBlock block = aft_allocate(allocator, sizeof(AftFile));

    AftFile* file = block.memory;
    file->allocator = allocator;
    file->handle = handle;
    return file;
}

AftReadFileResult aft_read_file_sync(AftFile* file, void* data, uint64_t bytes)
{
    AftReadFileResult result = {0};
    DWORD bytes_read = 0;
    BOOL file_read = ReadFile(file->handle, data, (DWORD) bytes, &bytes_read, NULL);

    if(file_read && (uint64_t) bytes_read == bytes)
    {
        result.bytes = bytes;
        result.success = true;
    }

    return result;
}

bool aft_write_file_sync(AftFile* file, const void* data, uint64_t bytes)
{
    ASSERT(bytes <= UINT32_MAX);
    DWORD bytes_written = 0;
    BOOL wrote = WriteFile(file->handle, data, (DWORD) bytes, &bytes_written, NULL);
    return wrote && (uint64_t) bytes_written == bytes;
}

void aft_write_to_error_output(const AftStringSlice* slice)
{
    HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);

    const char* start = aft_string_slice_start(slice);
    int count = aft_string_slice_count(slice);
    DWORD bytes_written;
    WriteFile(handle, start, count, &bytes_written, NULL);
}

void aft_write_to_standard_output(const AftStringSlice* slice)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    
    const char* start = aft_string_slice_start(slice);
    int count = aft_string_slice_count(slice);
    DWORD bytes_written;
    WriteFile(handle, start, count, &bytes_written, NULL);
}

AftMaybePath aft_get_executable_directory(void* allocator)
{
    AftMaybePath result;
    result.valid = false;
    aft_path_initialise_with_allocator(&result.value, allocator);

    int wide_path_cap = 128;
    AftMemoryBlock block = {NULL, 0};
    DWORD chars_copied;
    do
    {
        aft_deallocate(allocator, block);
        wide_path_cap *= 2;
        block = aft_allocate(allocator, sizeof(wchar_t) * wide_path_cap);
        chars_copied = GetModuleFileNameW(NULL, block.memory, (DWORD) wide_path_cap);
    } while(chars_copied == wide_path_cap);

    wchar_t* wide_path = block.memory;
    AftMaybeString utf8_path = wchar_string_to_utf8(wide_path, allocator);

    aft_deallocate(allocator, block);

    if(!utf8_path.valid)
    {
        return result;
    }
    
    AftStringSlice slice = aft_string_slice_from_string(&utf8_path.value);
    result = aft_path_from_slice_with_allocator(&slice, allocator);

    aft_string_destroy(&utf8_path.value);

    aft_path_remove_basename(&result.value);

    return result;
}
