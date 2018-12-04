#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AftMemoryBlock aft_allocate(void* allocator_pointer, uint64_t bytes)
{
    ASSERT(allocator_pointer);

    Allocator* allocator = (Allocator*) allocator_pointer;

    if(allocator->force_allocation_failure)
    {
        AftMemoryBlock empty =
        {
            .memory = NULL,
            .bytes = 0,
        };
        return empty;
    }

    allocator->bytes_used += bytes;

    AftMemoryBlock block =
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

bool aft_deallocate(void* allocator_pointer, AftMemoryBlock block)
{
    ASSERT(allocator_pointer);

    Allocator* allocator = (Allocator*) allocator_pointer;
    allocator->bytes_used -= block.bytes;

    free(block.memory);

    return true;
}

void add_test(Suite* suite, RunCall run, const char* name)
{
    if(suite->test_count + 1 >= suite->test_cap)
    {
        int prior_cap = suite->test_cap;
        int cap = (suite->test_cap) ? suite->test_cap * 2 : 16;

        TestSpec* specs = calloc(cap, sizeof(TestSpec));
        bool* tests_failed = calloc(cap, sizeof(bool));

        if(prior_cap)
        {
            memcpy(specs, suite->specs, sizeof(TestSpec) * prior_cap);
            memcpy(tests_failed, suite->tests_failed, sizeof(bool) * prior_cap);
            free(suite->specs);
            free(suite->tests_failed);
        }

        suite->specs = specs;
        suite->tests_failed = tests_failed;
        suite->test_cap = cap;
    }

    int index = suite->test_count;
    suite->specs[index].name = name;
    suite->specs[index].run = run;
    suite->test_count += 1;
}

AftMaybeString make_random_string(RandomGenerator* generator,
        Allocator* allocator)
{
    AftMaybeString result;

    int codepoint_count = random_int_range(generator, 1, 128);
    uint64_t bytes = sizeof(char32_t) * codepoint_count;

    AftMemoryBlock block = aft_allocate(allocator, bytes);

    if(!block.memory)
    {
        result.valid = false;
        return result;
    }

    AftUtf32String string;
    string.count = codepoint_count;
    string.contents = (char32_t*) block.memory;

    for(int code_index = 0; code_index < codepoint_count; code_index += 1)
    {
        string.contents[code_index] = random_int_range(generator, 0, 0x10ffff);
    }

    result = aft_utf32_to_utf8_with_allocator(&string, allocator);

    bool destroyed = aft_utf32_destroy_with_allocator(&string, allocator);

    if(!destroyed)
    {
        aft_string_destroy(&result.value);
        result.valid = false;
    }

    return result;
}

bool run_tests(Suite* suite)
{
    int total_failed = 0;

    for(int test_index = 0; test_index < suite->test_count; test_index += 1)
    {
        TestSpec spec = suite->specs[test_index];

        Test test = {0};
        test.bad_allocator.force_allocation_failure = true;

        random_seed_by_time(&test.generator);

        bool failure = !spec.run(&test);
        total_failed += failure;
        suite->tests_failed[test_index] = failure;

        ASSERT(test.allocator.bytes_used == 0);
    }

    FILE* file = stdout;

    if(total_failed > 0)
    {
        fprintf(file, "test failed: %d\n", total_failed);

        int printed = 0;

        for(int test_index = 0; test_index < suite->test_count; test_index += 1)
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

            if(suite->tests_failed[test_index])
            {
                fprintf(file, "%s%s%s", separator, also,
                        suite->specs[test_index].name);
                printed += 1;
            }
        }

        fprintf(file, "\n\n");
    }
    else
    {
        fprintf(file, "All tests succeeded!\n\n");
    }

    return total_failed == 0;
}

int string_size(const char* string)
{
    if(string)
    {
        const char* s;
        for(s = string; *s; s += 1);
        return (int) (s - string);
    }

    return 0;
}

bool strings_match(const char* a, const char* b)
{
    while(*a && (*a == *b))
    {
        a += 1;
        b += 1;
    }
    return *a == *b;
}

