#include "random.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../Source/aft_string.h"


#define ASSERT(expression) \
    assert(expression)


typedef struct Allocator
{
    uint64_t bytes_used;
    bool force_allocation_failure;
} Allocator;

typedef struct Test Test;

typedef bool (*RunCall)(Test* test);

struct Test
{
    Allocator allocator;
    Allocator bad_allocator;
    RandomGenerator generator;
};

typedef struct TestSpec
{
    RunCall run;
    const char* name;
} TestSpec;

typedef struct Suite
{
    TestSpec* specs;
    bool* tests_failed;
    int test_count;
    int test_cap;
} Suite;


static void add_test(Suite* suite, RunCall run, const char* name)
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

static AftMaybeString make_random_string(RandomGenerator* generator,
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

static bool fuzz_assign(Test* test)
{
    AftMaybeString garble =
            make_random_string(&test->generator, &test->allocator);
    ASSERT(garble.valid);

    AftString string;
    aft_string_initialise_with_allocator(&string, &test->allocator);
    bool assigned = aft_string_assign(&string, &garble.value);

    bool matched = aft_strings_match(&garble.value, &string);

    bool destroyed_garble = aft_string_destroy(&garble.value);
    bool destroyed_string = aft_string_destroy(&string);
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

    AftMaybeString outer =
            aft_string_from_c_string_with_allocator(chicken, &test->allocator);
    AftMaybeString inner =
            aft_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = aft_string_add(&outer.value, &inner.value,
            string_size(chicken));
    ASSERT(combined);

    const char* contents = aft_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"курица蛋");
    bool size_correct =
            aft_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

    aft_string_destroy(&outer.value);
    aft_string_destroy(&inner.value);

    return result;
}

static bool test_add_middle(Test* test)
{
    const char* chicken = u8"курица";
    const char* egg = u8"蛋";

    AftMaybeString outer =
            aft_string_from_c_string_with_allocator(chicken, &test->allocator);
    AftMaybeString inner =
            aft_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = aft_string_add(&outer.value, &inner.value, 4);
    ASSERT(combined);

    const char* contents = aft_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"ку蛋рица");
    bool size_correct =
            aft_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

    aft_string_destroy(&outer.value);
    aft_string_destroy(&inner.value);

    return result;
}

static bool test_add_start(Test* test)
{
    const char* chicken = u8"курица";
    const char* egg = u8"蛋";

    AftMaybeString outer =
            aft_string_from_c_string_with_allocator(chicken, &test->allocator);
    AftMaybeString inner =
            aft_string_from_c_string_with_allocator(egg, &test->allocator);
    ASSERT(outer.valid);
    ASSERT(inner.valid);

    bool combined = aft_string_add(&outer.value, &inner.value, 0);
    ASSERT(combined);

    const char* contents = aft_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"蛋курица");
    bool size_correct =
            aft_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

    aft_string_destroy(&outer.value);
    aft_string_destroy(&inner.value);

    return result;
}

static bool test_append(Test* test)
{
    const char* a = u8"a猫🍌";
    const char* b = u8"a猫🍌";
    AftMaybeString base =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    AftMaybeString extension =
                aft_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(base.valid);
    ASSERT(extension.valid);

    bool appended = aft_string_append(&base.value, &extension.value);
    ASSERT(appended);

    const char* combined = aft_string_get_contents_const(&base.value);
    bool contents_match = strings_match(combined, u8"a猫🍌a猫🍌");
    bool size_correct =
            aft_string_get_count(&base.value) == string_size(combined);
    bool result = contents_match && size_correct;

    aft_string_destroy(&base.value);
    aft_string_destroy(&extension.value);

    return result;
}

static bool test_append_c_string(Test* test)
{
    const char* a = u8"👌🏼";
    const char* b = u8"🙋🏾‍♀️";
    AftMaybeString base =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(base.valid);

    bool appended = aft_string_append_c_string(&base.value, b);
    ASSERT(appended);

    const char* combined = aft_string_get_contents_const(&base.value);
    bool contents_match = strings_match(combined, u8"👌🏼🙋🏾‍♀️");
    bool size_correct =
            aft_string_get_count(&base.value) == string_size(combined);
    bool result = contents_match && size_correct;

    aft_string_destroy(&base.value);

    return result;
}

static bool test_assign(Test* test)
{
    const char* reference = u8"a猫🍌";
    AftMaybeString original =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);

    AftString string;
    aft_string_initialise_with_allocator(&string, &test->allocator);
    bool assigned = aft_string_assign(&string, &original.value);

    bool matched = aft_strings_match(&original.value, &string);

    bool destroyed_original = aft_string_destroy(&original.value);
    bool destroyed_string = aft_string_destroy(&string);
    ASSERT(destroyed_original);
    ASSERT(destroyed_string);

    return assigned && matched;
}

static bool test_copy(Test* test)
{
    const char* reference = u8"a猫🍌";

    AftMaybeString original =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    AftMaybeString copy = aft_string_copy(&original.value);
    ASSERT(original.valid);
    ASSERT(copy.valid);

    const char* original_contents = aft_string_get_contents_const(&original.value);
    const char* copy_contents = aft_string_get_contents_const(&copy.value);

    bool size_correct =
            aft_string_get_count(&original.value) == aft_string_get_count(&copy.value);
    bool result = strings_match(original_contents, copy_contents)
            && size_correct
            && original.value.allocator == copy.value.allocator;

    aft_string_destroy(&original.value);
    aft_string_destroy(&copy.value);

    return result;
}

static bool test_destroy(Test* test)
{
    const char* reference = "Moist";
    AftMaybeString original =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(original.valid);

    bool destroyed = aft_string_destroy(&original.value);

    int post_count = aft_string_get_count(&original.value);

    return destroyed
            && post_count == 0
            && original.value.allocator == &test->allocator;
}

static bool test_ends_with(Test* test)
{
    const char* a = u8"Wow a猫🍌";
    const char* b = u8"a猫🍌";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    AftMaybeString ending =
            aft_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(ending.valid);

    bool result = aft_string_ends_with(&string.value, &ending.value);

    aft_string_destroy(&string.value);
    aft_string_destroy(&ending.value);

    return result;
}

static bool test_find_first_char(Test* test)
{
    const char* a = "a A AA s";
    int known_index = 2;

    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(string.valid);

    AftMaybeInt index = aft_string_find_first_char(&string.value, 'A');

    bool result = index.value == known_index;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_find_first_string(Test* test)
{
    const char* a = u8"وَالشَّمْسُ تَجْرِي لِمُسْتَقَرٍّ لَّهَا ذَٰلِكَ تَقْدِيرُ الْعَزِيزِ الْعَلِيمِ";
    const char* b = u8"الْعَ";
    int known_index = 112;

    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    AftMaybeString target =
            aft_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(target.valid);

    AftMaybeInt index =
            aft_string_find_first_string(&string.value, &target.value);

    bool result = index.value == known_index;

    aft_string_destroy(&string.value);
    aft_string_destroy(&target.value);

    return result;
}

static bool test_find_last_char(Test* test)
{
    const char* a = "a A AA s";
    int known_index = 5;

    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(string.valid);

    AftMaybeInt index = aft_string_find_last_char(&string.value, 'A');

    bool result = index.value == known_index;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_find_last_string(Test* test)
{
    const char* a = u8"My 1st page הדף מספר 2 שלי My 3rd page";
    const char* b = u8"My";
    int known_index = 37;

    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    AftMaybeString target =
            aft_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(target.valid);

    AftMaybeInt index = aft_string_find_last_string(&string.value, &target.value);

    bool result = (index.value == known_index);

    aft_string_destroy(&string.value);
    aft_string_destroy(&target.value);

    return result;
}

static bool test_from_c_string(Test* test)
{
    const char* reference = u8"a猫🍌";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    const char* contents = aft_string_get_contents_const(&string.value);

    bool result = string.valid && strings_match(contents, reference);

    aft_string_destroy(&string.value);

    return result;
}

static bool test_from_buffer(Test* test)
{
    const char* reference = u8"a猫🍌";
    int bytes = string_size(reference);
    AftMaybeString string =
            aft_string_from_buffer_with_allocator(reference, bytes,
                    &test->allocator);
    const char* contents = aft_string_get_contents_const(&string.value);

    bool result = string.valid
            && strings_match(contents, reference)
            && aft_string_get_count(&string.value) == bytes;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_get_contents(Test* test)
{
    const char* reference = "Test me in the most basic way possible.";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    char* contents = aft_string_get_contents(&string.value);

    bool result = contents && strings_match(contents, reference);

    aft_string_destroy(&string.value);

    return result;
}

static bool test_get_contents_const(Test* test)
{
    const char* original = u8"👌🏼";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(original, &test->allocator);
    ASSERT(string.valid);

    const char* after = aft_string_get_contents_const(&string.value);

    bool result = after && strings_match(original, after);

    aft_string_destroy(&string.value);

    return result;
}

static bool test_initialise(Test* test)
{
    AftString string;
    aft_string_initialise_with_allocator(&string, &test->allocator);
    return string.allocator == &test->allocator
            && aft_string_get_count(&string) == 0;
}

static bool test_iterator_next(Test* test)
{
    const char* reference = u8"Бума́га всё сте́рпит.";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AftCodepointIterator it;
    aft_codepoint_iterator_set_string(&it, &string.value);
    aft_codepoint_iterator_next(&it);

    int before_index = aft_codepoint_iterator_get_index(&it);
    AftMaybeChar32 next = aft_codepoint_iterator_next(&it);
    int after_index = aft_codepoint_iterator_get_index(&it);

    bool result = next.valid
            && next.value == U'у'
            && before_index == 2
            && after_index == 4;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_iterator_prior(Test* test)
{
    const char* reference = u8"Бума́га всё сте́рпит.";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AftCodepointIterator it;
    aft_codepoint_iterator_set_string(&it, &string.value);
    aft_codepoint_iterator_end(&it);
    aft_codepoint_iterator_prior(&it);

    int before_index = aft_codepoint_iterator_get_index(&it);
    AftMaybeChar32 prior = aft_codepoint_iterator_prior(&it);
    int after_index = aft_codepoint_iterator_get_index(&it);

    bool result = prior.valid
            && prior.value == U'т'
            && before_index == 38
            && after_index == 36;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_iterator_set_string(Test* test)
{
    const char* reference = "9876543210";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AftCodepointIterator it;
    aft_codepoint_iterator_set_string(&it, &string.value);

    bool result =
            aft_codepoint_iterator_get_string(&it) == &string.value
            && aft_codepoint_iterator_get_index(&it) == 0;

    aft_string_destroy(&string.value);

    return result;
}

static bool test_remove(Test* test)
{
    const char* reference = "9876543210";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AftStringRange range = {3, 6};
    aft_string_remove(&string.value, &range);
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = strings_match(contents, "9873210");

    aft_string_destroy(&string.value);

    return result;
}

static bool test_replace(Test* test)
{
    const char* reference = "9876543210";
    const char* insert = "abc";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    AftMaybeString replacement =
            aft_string_from_c_string_with_allocator(insert, &test->allocator);
    ASSERT(string.valid);
    ASSERT(replacement.valid);

    AftStringRange range = {3, 6};
    aft_string_replace(&string.value, &range, &replacement.value);
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = strings_match(contents, "987abc3210");

    aft_string_destroy(&string.value);
    aft_string_destroy(&replacement.value);

    return result;
}

static bool test_reserve(Test* test)
{
    AftString string;
    aft_string_initialise_with_allocator(&string, &test->allocator);
    int requested_cap = 100;

    bool reserved = aft_string_reserve(&string, requested_cap);
    int cap = aft_string_get_capacity(&string);
    bool result = reserved && cap == requested_cap;

    aft_string_destroy(&string);

    return result;
}

static bool test_starts_with(Test* test)
{
    const char* a = u8"a猫🍌 Wow";
    const char* b = u8"a猫🍌";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(a, &test->allocator);
    AftMaybeString ending =
            aft_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(ending.valid);

    bool result = aft_string_starts_with(&string.value, &ending.value);

    aft_string_destroy(&string.value);
    aft_string_destroy(&ending.value);

    return result;
}

static bool test_substring(Test* test)
{
    const char* reference = "9876543210";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AftStringRange range = {3, 6};
    AftMaybeString middle = aft_string_substring(&string.value, &range);
    ASSERT(middle.valid);

    const char* contents = aft_string_get_contents_const(&middle.value);
    bool result = strings_match(contents, "654")
            && string.value.allocator == &test->allocator;

    aft_string_destroy(&string.value);
    aft_string_destroy(&middle.value);

    return result;
}

static bool test_to_c_string(Test* test)
{
    const char* reference = "It's okay.";
    AftMaybeString string =
            aft_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    char* copy =
            aft_string_to_c_string_with_allocator(&string.value,
                    &test->allocator);
    bool result = strings_match(copy, reference);

    aft_string_destroy(&string.value);
    aft_c_string_deallocate_with_allocator(&test->allocator, copy);

    return result;
}

static bool run_tests()
{
    Suite suite = {0};

    add_test(&suite, fuzz_assign, "Fuzz Assign");
    add_test(&suite, test_add_end, "Add End");
    add_test(&suite, test_add_middle, "Add Middle");
    add_test(&suite, test_add_start, "Add Start");
    add_test(&suite, test_append, "Append");
    add_test(&suite, test_append_c_string, "Append C String");
    add_test(&suite, test_assign, "Assign");
    add_test(&suite, test_copy, "Copy");
    add_test(&suite, test_destroy, "Destroy");
    add_test(&suite, test_ends_with, "Ends With");
    add_test(&suite, test_find_first_char, "Find First Char");
    add_test(&suite, test_find_first_string, "Find First String");
    add_test(&suite, test_find_last_char, "Find Last Char");
    add_test(&suite, test_find_last_string, "Find Last String");
    add_test(&suite, test_from_c_string, "From C String");
    add_test(&suite, test_from_buffer, "From Buffer");
    add_test(&suite, test_get_contents, "Get Contents");
    add_test(&suite, test_get_contents_const, "Get Contents Const");
    add_test(&suite, test_initialise, "Initialise");
    add_test(&suite, test_iterator_next, "Iterator Next");
    add_test(&suite, test_iterator_prior, "Iterator Prior");
    add_test(&suite, test_iterator_set_string, "Iterator Set String");
    add_test(&suite, test_remove, "Remove");
    add_test(&suite, test_replace, "Replace");
    add_test(&suite, test_reserve, "Reserve");
    add_test(&suite, test_starts_with, "Starts With");
    add_test(&suite, test_substring, "Substring");
    add_test(&suite, test_to_c_string, "To C String");

    int total_failed = 0;

    for(int test_index = 0; test_index < suite.test_count; test_index += 1)
    {
        TestSpec spec = suite.specs[test_index];

        Test test = {0};
        test.bad_allocator.force_allocation_failure = true;

        random_seed_by_time(&test.generator);

        bool failure = !spec.run(&test);
        total_failed += failure;
        suite.tests_failed[test_index] = failure;

        ASSERT(test.allocator.bytes_used == 0);
    }

    FILE* file = stdout;

    if(total_failed > 0)
    {
        fprintf(file, "test failed: %d\n", total_failed);

        int printed = 0;

        for(int test_index = 0; test_index < suite.test_count; test_index += 1)
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

            if(suite.tests_failed[test_index])
            {
                fprintf(file, "%s%s%s", separator, also,
                        suite.specs[test_index].name);
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


int main(int argc, const char** argv)
{
    bool success = run_tests();
    return !success;
}

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
