#include "json.h"

#include "filesystem.h"

#include "../Utility/test.h"


int main(const char* argv, int argc)
{
    Allocator allocator = {0};

    const char* test_paths[2] =
    {
        "handmade.json",
        "omdb.json",
    };

    for(int path_index = 0;
            path_index < 2;
            path_index += 1)
    {
        AftMaybePath path = aft_get_executable_directory(&allocator);
        AftMaybePath json_path = aft_path_from_c_string_with_allocator(test_paths[path_index], &allocator);
        aft_path_append(&path.value, &json_path.value);

        aft_path_destroy(&json_path.value);

        AftMaybeString file_contents = aft_load_text_file(&path.value, &allocator);
        if(!file_contents.valid)
        {
            AftString message = aft_string_from_c_string_with_allocator("Failed to load ", &allocator).value;
            aft_string_append(&message, aft_path_get_string_const(&path.value));
            aft_string_append_c_string(&message, ".\n");
            AftStringSlice slice = aft_string_slice_from_string(&message);
            aft_write_to_error_output(&slice);
            aft_string_destroy(&message);

            return -1;
        }

        aft_path_destroy(&path.value);

        AftStringSlice slice = aft_string_slice_from_string(&file_contents.value);
        JsonResult result = json_deserialize(&slice, &allocator);

        aft_string_destroy(&file_contents.value);

        if(result.error != JSON_ERROR_NONE)
        {
            AftStringSlice slice = aft_string_slice_from_c_string("Failed to parse json.\n");
            aft_write_to_error_output(&slice);
        }
        else
        {
            AftMaybeString serialized = json_serialize(&result.root, &allocator);

            if(!serialized.valid)
            {
                AftStringSlice slice = aft_string_slice_from_c_string("Failed to serialize the resulting json.\n");
                aft_write_to_error_output(&slice);
            }
            else
            {
                AftString message = aft_string_from_c_string_with_allocator("Output:\n", &allocator).value;
                aft_string_append(&message, &serialized.value);
                aft_string_append_c_string(&message, "\n");
                AftStringSlice slice = aft_string_slice_from_string(&message);
                aft_write_to_standard_output(&slice);
                aft_string_destroy(&message);

                aft_string_destroy(&serialized.value);
            }
        }

        json_element_destroy(&result.root);
    }

    if(allocator.bytes_used != 0)
    {
        AftString message = aft_string_from_c_string_with_allocator("Memory leak warning! ", &allocator).value;

        AftBaseFormat format = {.base = 10};
        AftString bytes = aft_ascii_from_uint64_with_allocator(allocator.bytes_used, &format, &allocator).value;
        aft_string_append(&message, &bytes);

        aft_string_append_c_string(&message, " bytes were not deallocated. \n");

        AftStringSlice slice = aft_string_slice_from_string(&message);
        aft_write_to_error_output(&slice);

        aft_string_destroy(&message);
        aft_string_destroy(&bytes);
    }

    return 0;
}
