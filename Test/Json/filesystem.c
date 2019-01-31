#include "filesystem.h"


AftMaybeString aft_load_text_file(const AftPath* path, void* allocator)
{
    AftMaybeString result;
    result.valid = false;
    aft_string_initialise_with_allocator(&result.value, allocator);

    AftFile* file = aft_open_file(path, AFT_OPEN_FILE_MODE_READ, allocator);
    if(!file)
    {
        return result;
    }

    AftMaybeUint64 size = aft_get_file_size(file);
    if(!size.valid)
    {
        aft_close_file(file);
        return result;
    }

    AftMemoryBlock block = aft_allocate(allocator, size.value);
    if(!block.memory)
    {
        aft_close_file(file);
        return result;
    }

    AftReadFileResult read_result = aft_read_file_sync(file, block.memory, block.bytes);
    aft_close_file(file);

    if(!read_result.success || read_result.bytes != block.bytes)
    {
        aft_deallocate(allocator, block);
        return result;
    }

    AftStringSlice slice = aft_string_slice_from_buffer(block.memory, block.bytes);
    result = aft_string_copy_slice_with_allocator(&slice, allocator);

    aft_deallocate(allocator, block);

    return result;
}


bool aft_path_append(AftPath* path, const AftPath* after)
{
    bool appended = true;

    appended = appended && aft_string_append_char(&path->string, '/');
    appended = appended && aft_string_append(&path->string, &after->string);

    return appended;
}

void aft_path_destroy(AftPath* path)
{
    aft_string_destroy(&path->string);
}

AftMaybePath aft_path_from_c_string_with_allocator(const char* path, void* allocator)
{
    AftMaybeString string = aft_string_copy_c_string_with_allocator(path, allocator);

    AftMaybePath result;
    result.valid = string.valid;
    result.value.string = string.value;

    return result;
}

const char* aft_path_get_contents_const(const AftPath* path)
{
    return aft_string_get_contents_const(&path->string);
}

const AftString* aft_path_get_string_const(const AftPath* path)
{
    return &path->string;
}

void aft_path_initialise_with_allocator(AftPath* path, void* allocator)
{
    aft_string_initialise_with_allocator(&path->string, allocator);
}

void aft_path_remove_basename(AftPath* path)
{
    AftStringSlice slice = aft_string_slice_from_string(&path->string);
    AftMaybeInt slash = aft_string_slice_find_last_char(&slice, '/');

    if(slash.valid)
    {
        int count = aft_string_get_count(&path->string);
        AftStringRange range = {slash.value, count};
        aft_string_remove(&path->string, &range);
    }
}

