#include "json.h"

#include "../Utility/test.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>


typedef struct AftPath
{
    AftString string;
} AftPath;

typedef struct AftMaybePath
{
    AftPath value;
    bool valid;
} AftMaybePath;


static bool aft_path_append(AftPath* path, const AftPath* after)
{
    bool appended = true;

    appended = appended && aft_string_append_char(&path->string, '/');
    appended = appended && aft_string_append(&path->string, &after->string);

    return appended;
}

static void aft_path_destroy(AftPath* path)
{
    aft_string_destroy(&path->string);
}

static AftMaybePath aft_path_from_c_string_with_allocator(const char* path, void* allocator)
{
    AftMaybeString string = aft_string_from_c_string_with_allocator(path, allocator);

    AftMaybePath result;
    result.valid = string.valid;
    result.value.string = string.value;

    return result;
}

static const char* aft_path_get_contents_const(const AftPath* path)
{
    return aft_string_get_contents_const(&path->string);
}

static void aft_path_initialise_with_allocator(AftPath* path, void* allocator)
{
    aft_string_initialise_with_allocator(&path->string, allocator);
}

static void aft_path_remove_basename(AftPath* path)
{
    AftMaybeInt slash = aft_string_find_last_char(&path->string, '/');

    if(slash.valid)
    {
        int count = aft_string_get_count(&path->string);
        AftStringRange range = {slash.value, count};
        aft_string_remove(&path->string, &range);
    }
}

static AftMaybePath get_executable_directory(void* allocator)
{
    AftMaybePath result;
    result.valid = false;
    aft_path_initialise_with_allocator(&result.value, allocator);

    char* path = NULL;
    int path_size = 128;
    int chars_copied;
    do
    {
        path_size *= 2;

        free(path);
        path = calloc(path_size + 1, sizeof(char));
        if(!path)
        {
            return result;
        }

        chars_copied = readlink("/proc/self/exe", path, path_size);
        if(chars_copied == -1 || chars_copied > path_size)
        {
            free(path);
            return result;
        }
    } while(chars_copied == path_size);

    path[chars_copied] = '\0';

    result = aft_path_from_c_string_with_allocator(path, allocator);
    aft_path_remove_basename(&result.value);

    return result;
}

static AftMaybeString load_file(const AftPath* path, Allocator* allocator)
{
    AftMaybeString result;
    result.valid = false;
    aft_string_initialise_with_allocator(&result.value, allocator);

    FILE* file = fopen(aft_path_get_contents_const(path), "r");
    if(!file)
    {
        return result;
    }

    int seek_result = fseek(file, 0, SEEK_END);
    if(seek_result != 0)
    {
        int close_result = fclose(file);
        ASSERT(close_result == 0);
        return result;
    }

    long offset = ftell(file);
    if(offset == -1l)
    {
        int close_result = fclose(file);
        ASSERT(close_result == 0);
        return result;
    }

    seek_result = fseek(file, 0, SEEK_SET);
    if(seek_result != 0)
    {
        int close_result = fclose(file);
        ASSERT(close_result == 0);
        return result;
    }

    size_t bytes_in_file = offset;
    char* contents = calloc(sizeof(char), bytes_in_file);
    if(!contents)
    {
        int close_result = fclose(file);
        ASSERT(close_result == 0);
        return result;
    }

    size_t elements_read = fread(contents, sizeof(char), bytes_in_file, file);
    int close_result = fclose(file);
    ASSERT(close_result == 0);

    if(elements_read != bytes_in_file)
    {
        free(contents);
        return result;
    }

    AftStringSlice slice = aft_string_slice_from_buffer(contents, bytes_in_file);
    result = aft_string_from_slice_with_allocator(&slice, allocator);

    free(contents);

    return result;
}


int main(const char* argv, int argc)
{
    Allocator allocator = {0};

    AftMaybePath path = get_executable_directory(&allocator);
    AftMaybePath json_path = aft_path_from_c_string_with_allocator("handmade.json", &allocator);
    aft_path_append(&path.value, &json_path.value);

    aft_path_destroy(&json_path.value);

    AftMaybeString file_contents = load_file(&path.value, &allocator);
    if(!file_contents.valid)
    {
        fprintf(stderr, "Failed to load %s.\n", aft_path_get_contents_const(&path.value));
        return -1;
    }

    aft_path_destroy(&path.value);

    AftStringSlice slice = aft_string_slice_from_string(&file_contents.value);
    JsonResult result = json_deserialize(&slice, &allocator);

    aft_string_destroy(&file_contents.value);

    if(result.error != JSON_ERROR_NONE)
    {
        fprintf(stderr, "Failed to parse json.\n");
    }
    else
    {
        AftMaybeString serialized = json_serialize(&result.root, &allocator);

        if(!serialized.valid)
        {
            fprintf(stderr, "Failed to serialize the resulting json.\n");
        }
        else
        {
            printf("Output:\n%s\n", aft_string_get_contents_const(&serialized.value));

            aft_string_destroy(&serialized.value);
        }
    }

    json_element_destroy(&result.root);

    if(allocator.bytes_used != 0)
    {
        fprintf(stderr, "Memory leak warning! %" PRIu64 " bytes were not deallocated. \n", allocator.bytes_used);
    }

    return 0;
}
