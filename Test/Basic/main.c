#define AD_STRING_ALLOCATE(allocator, bytes) \
        my_allocate(allocator, bytes)
#define AD_STRING_DEALLOCATE(allocator, block) \
        my_deallocate(allocator, block)

#include "../../Source/ad_string.h"

#include <stdio.h>
#include <stdlib.h>


typedef enum TestType
{
    TEST_TYPE_FROM_C_STRING,
    TEST_TYPE_INITIALISE,
    TEST_TYPE_COUNT,
} TestType;


typedef struct Allocator
{
    uint64_t bytes_used;
} Allocator;

typedef struct Test
{
    Allocator allocator;
    TestType type;
} Test;


static const char* describe_test(TestType type)
{
    switch(type)
    {
        case TEST_TYPE_FROM_C_STRING: return "From C String";
        case TEST_TYPE_INITIALISE:    return "Initialise";
        default:                      return "Unknown";
    }
}

static bool strings_match(const char* a, const char* b)
{
    while(*a && (*a == *b))
    {
        a += 1;
        b += 1;
    }
    return *a == *b;
}

static bool test_from_c_string(Test* test)
{
    const char* reference = u8"a猫🍌";
    AdMaybeString result =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    const char* contents = ad_string_as_c_string(&result.value);
    return result.valid && strings_match(contents, reference);
}

static bool test_initialise(Test* test)
{
    AdString string;
    ad_string_initialise_with_allocator(&string, &test->allocator);
    return string.allocator == &test->allocator
            && string.count == 0;
}

static bool run_test(Test* test)
{
    switch(test->type)
    {
        case TEST_TYPE_FROM_C_STRING: return test_from_c_string(test);
        case TEST_TYPE_INITIALISE:    return test_initialise(test);
        default:                      return false;
    }
}

static bool run_tests()
{
    TestType tests[TEST_TYPE_COUNT] =
    {
        TEST_TYPE_INITIALISE,
    };
    bool tests_failed[TEST_TYPE_COUNT];
    int total_failed = 0;

    for(int test_index = 0; test_index < TEST_TYPE_COUNT; test_index += 1)
    {
        Test test =
        {
            .type = tests[test_index],
        };
        bool failure = !run_test(&test);
        total_failed += failure;
    }

    FILE* file = stdout;

    if(total_failed > 0)
    {
        fprintf(file, "test failed: %d\n", total_failed);

        int printed = 0;

        for(int test_index = 0; test_index < TEST_TYPE_COUNT; test_index += 1)
        {
            const char* separator = "";
            const char* also = "";

            if(total_failed > 2 && printed > 0)
            {
                separator = ", ";
            }

            if(total_failed > 1 && printed == total_failed - 1)
            {
                if(total_failed == 2)
                {
                    also = " and ";
                }
                else
                {
                    also = "and ";
                }
            }

            if(tests_failed[test_index])
            {
                const char* test = describe_test(tests[test_index]);
                fprintf(file, "%s%s%s", separator, also, test);
                printed += 1;
            }
        }

        fprintf(file, "\n\n");
    }
    else
    {
        fprintf(file, "All tests succeeded!\n\n");
    }

    return tests_failed == 0;
}


int main(int argc, const char** argv)
{
    bool success = run_tests();
    return !success;
}

AdMemoryBlock my_allocate(void* allocator_pointer, uint64_t bytes)
{
    Allocator* allocator = (Allocator*) allocator_pointer;
    allocator->bytes_used += bytes;

    AdMemoryBlock block =
    {
        .memory = calloc(bytes, 1),
        .bytes = bytes,
    };

    if(!block.memory)
    {
        block.bytes = 0;
    }

    return block;
}

bool my_deallocate(void* allocator_pointer, AdMemoryBlock block)
{
    Allocator* allocator = (Allocator*) allocator_pointer;
    allocator->bytes_used -= block.bytes;

    free(block.memory);

    return true;
}
