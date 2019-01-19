#include "json.h"

#include "../Utility/test.h"

#include <inttypes.h>
#include <stdio.h>

int main(const char* argv, int argc)
{
    const char* test_json = "  [1.95e7, \"whattup \\\\ yo\", [null], false, [], {\"help\": {}, \"count_dracula\": 365}] \t ";

    Allocator allocator = {0};

    AftStringSlice slice = aft_string_slice_from_c_string(test_json);
    JsonResult result = json_parse(&slice, &allocator);

    printf("Input: %s\n", test_json);

    if(result.error != JSON_ERROR_NONE)
    {
        fprintf(stderr, "Failed to parse json.\n");
    }
    else
    {
        AftMaybeString serialized = json_serialize(result.root, &allocator);

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

    json_result_destroy(&result);

    if(allocator.bytes_used != 0)
    {
        fprintf(stderr, "Memory leak warning! %" PRIu64 " bytes were not deallocated. \n", allocator.bytes_used);
    }

    return 0;
}
