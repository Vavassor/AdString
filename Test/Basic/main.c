#include "../../Source/ad_string.h"

#include "random.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


#define ASSERT(expression) \
    assert(expression)


typedef enum TestType
{
    TEST_TYPE_ADD_END,
    TEST_TYPE_ADD_MIDDLE,
    TEST_TYPE_ADD_START,
    TEST_TYPE_ASSIGN,
    TEST_TYPE_COPY,
    TEST_TYPE_FROM_BUFFER,
    TEST_TYPE_FROM_C_STRING,
    TEST_TYPE_FUZZ_ASSIGN,
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
    RandomGenerator generator;
    TestType type;
} Test;


static const char* describe_test(TestType type)
{
    switch(type)
    {
        case TEST_TYPE_ADD_END:       return "Add End";
        case TEST_TYPE_ADD_MIDDLE:    return "Add Middle";
        case TEST_TYPE_ADD_START:     return "Add Start";
        case TEST_TYPE_ASSIGN:        return "Assign";
        case TEST_TYPE_COPY:          return "Copy";
        case TEST_TYPE_FROM_BUFFER:   return "From Buffer";
        case TEST_TYPE_FROM_C_STRING: return "From C String";
        case TEST_TYPE_FUZZ_ASSIGN:   return "Fuzz Assign";
        case TEST_TYPE_INITIALISE:    return "Initialise";
        default:                      return "Unknown";
    }
}

static AdMaybeString make_random_string(RandomGenerator* generator,
        Allocator* allocator)
{
    AdMaybeString result;

    int codepoint_count = random_int_range(generator, 1, 128);
    uint64_t bytes = sizeof(char32_t) * codepoint_count;

    AdMemoryBlock block = ad_string_allocate(allocator, bytes);

    if(!block.memory)
    {
        result.valid = false;
        return result;
    }

    AdUtf32String string;
    string.count = codepoint_count;
    string.contents = (char32_t*) block.memory;

    for(int code_index = 0; code_index < codepoint_count; code_index += 1)
    {
        string.contents[code_index] = random_int_range(generator, 0, 0x10ffff);
    }

    result = ad_utf32_to_utf8_with_allocator(&string, allocator);

    bool destroyed = ad_utf32_destroy_with_allocator(&string, allocator);

    if(!destroyed)
    {
        ad_string_destroy(&result.value);
        result.valid = false;
    }

    return result;
}

static bool fuzz_assign(Test* test)
{
    AdMaybeString garble =
            make_random_string(&test->generator, &test->allocator);
    ASSERT(garble.valid);

    AdString string;
    ad_string_initialise_with_allocator(&string, &test->allocator);
    bool assigned = ad_string_assign(&string, &garble.value);

    bool matched = ad_strings_match(&garble.value, &string);

    bool destroyed_garble = ad_string_destroy(&garble.value);
    bool destroyed_string = ad_string_destroy(&string);
    ASSERT(destroyed_garble);
    ASSERT(destroyed_string);

    return assigned && matched;
}

static int string_size(const char* string)
{
    if(string)
    {
        const char* s;
        for(s = string; *s; s += 1);
        return (int) (s - string);
    }

    return 0;
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

static bool test_add_end(Test* test)
{
    const char* chicken = u8"курица";
    const char* egg = u8"蛋";

    AdMaybeString outer =
            ad_string_from_c_string_with_allocator(chicken, &test->allocator);
    AdMaybeString inner =
            ad_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = ad_string_add(&outer.value, &inner.value,
            string_size(chicken));
    ASSERT(combined);

    const char* contents = ad_string_as_c_string(&outer.value);
    bool result = strings_match(contents, u8"курица蛋");

    ad_string_destroy(&outer.value);
    ad_string_destroy(&inner.value);

    return result;
}

static bool test_add_middle(Test* test)
{
    const char* chicken = u8"курица";
    const char* egg = u8"蛋";

    AdMaybeString outer =
            ad_string_from_c_string_with_allocator(chicken, &test->allocator);
    AdMaybeString inner =
            ad_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = ad_string_add(&outer.value, &inner.value, 4);
    ASSERT(combined);

    const char* contents = ad_string_as_c_string(&outer.value);
    bool result = strings_match(contents, u8"ку蛋рица");

    ad_string_destroy(&outer.value);
    ad_string_destroy(&inner.value);

    return result;
}

static bool test_add_start(Test* test)
{
    const char* chicken = u8"курица";
    const char* egg = u8"蛋";

    AdMaybeString outer =
            ad_string_from_c_string_with_allocator(chicken, &test->allocator);
    AdMaybeString inner =
            ad_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = ad_string_add(&outer.value, &inner.value, 0);
    ASSERT(combined);

    const char* contents = ad_string_as_c_string(&outer.value);
    bool result = strings_match(contents, u8"蛋курица");

    ad_string_destroy(&outer.value);
    ad_string_destroy(&inner.value);

    return result;
}

static bool test_assign(Test* test)
{
    const char* reference = u8"a猫🍌";
    AdMaybeString original =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);

    AdString string;
    ad_string_initialise_with_allocator(&string, &test->allocator);
    bool assigned = ad_string_assign(&string, &original.value);

    bool matched = ad_strings_match(&original.value, &string);

    bool destroyed_original = ad_string_destroy(&original.value);
    bool destroyed_string = ad_string_destroy(&string);
    ASSERT(destroyed_original);
    ASSERT(destroyed_string);

    return assigned && matched;
}

static bool test_copy(Test* test)
{
    const char* reference = u8"a猫🍌";
    AdMaybeString original =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    AdMaybeString copy = ad_string_copy(&original.value);
    const char* original_contents = ad_string_as_c_string(&original.value);
    const char* copy_contents = ad_string_as_c_string(&copy.value);

    bool result = copy.valid
            && strings_match(original_contents, copy_contents)
            && original.value.count == copy.value.count
            && original.value.allocator == copy.value.allocator;

    ad_string_destroy(&original.value);
    ad_string_destroy(&copy.value);

    return result;
}

static bool test_from_c_string(Test* test)
{
    const char* reference = u8"a猫🍌";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    const char* contents = ad_string_as_c_string(&string.value);

    bool result = string.valid && strings_match(contents, reference);

    ad_string_destroy(&string.value);

    return result;
}

static bool test_from_buffer(Test* test)
{
    const char* reference = u8"a猫🍌";
    int bytes = string_size(reference);
    AdMaybeString string =
            ad_string_from_buffer_with_allocator(reference, bytes,
                    &test->allocator);
    const char* contents = ad_string_as_c_string(&string.value);

    bool result = string.valid
            && strings_match(contents, reference)
            && string.value.count == bytes;

    ad_string_destroy(&string.value);

    return result;
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
        case TEST_TYPE_ADD_END:       return test_add_end(test);
        case TEST_TYPE_ADD_MIDDLE:    return test_add_middle(test);
        case TEST_TYPE_ADD_START:     return test_add_start(test);
        case TEST_TYPE_ASSIGN:        return test_assign(test);
        case TEST_TYPE_COPY:          return test_copy(test);
        case TEST_TYPE_FROM_BUFFER:   return test_from_buffer(test);
        case TEST_TYPE_FROM_C_STRING: return test_from_c_string(test);
        case TEST_TYPE_FUZZ_ASSIGN:   return fuzz_assign(test);
        case TEST_TYPE_INITIALISE:    return test_initialise(test);
        default:                      return false;
    }
}

static bool run_tests()
{
    TestType tests[TEST_TYPE_COUNT] =
    {
        TEST_TYPE_ADD_END,
        TEST_TYPE_ADD_MIDDLE,
        TEST_TYPE_ADD_START,
        TEST_TYPE_ASSIGN,
        TEST_TYPE_COPY,
        TEST_TYPE_FROM_BUFFER,
        TEST_TYPE_FUZZ_ASSIGN,
        TEST_TYPE_FROM_C_STRING,
        TEST_TYPE_INITIALISE,
    };
    bool tests_failed[TEST_TYPE_COUNT];
    int total_failed = 0;

    for(int test_index = 0; test_index < TEST_TYPE_COUNT; test_index += 1)
    {
        Test test = {0};
        test.type = tests[test_index];

        random_seed_by_time(&test.generator);

        bool failure = !run_test(&test);
        total_failed += failure;
        tests_failed[test_index] = failure;

        ASSERT(test.allocator.bytes_used == 0);
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

AdMemoryBlock ad_string_allocate(void* allocator_pointer, uint64_t bytes)
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

bool ad_string_deallocate(void* allocator_pointer, AdMemoryBlock block)
{
    Allocator* allocator = (Allocator*) allocator_pointer;
    allocator->bytes_used -= block.bytes;

    free(block.memory);

    return true;
}
