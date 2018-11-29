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
    TEST_TYPE_APPEND,
    TEST_TYPE_APPEND_C_STRING,
    TEST_TYPE_ASSIGN,
    TEST_TYPE_COPY,
    TEST_TYPE_DESTROY,
    TEST_TYPE_ENDS_WITH,
    TEST_TYPE_FIND_FIRST_CHAR,
    TEST_TYPE_FIND_FIRST_STRING,
    TEST_TYPE_FIND_LAST_STRING,
    TEST_TYPE_FIND_LAST_CHAR,
    TEST_TYPE_FROM_BUFFER,
    TEST_TYPE_FROM_C_STRING,
    TEST_TYPE_FUZZ_ASSIGN,
    TEST_TYPE_GET_CONTENTS,
    TEST_TYPE_GET_CONTENTS_CONST,
    TEST_TYPE_INITIALISE,
    TEST_TYPE_ITERATOR_NEXT,
    TEST_TYPE_ITERATOR_PRIOR,
    TEST_TYPE_ITERATOR_SET_STRING,
    TEST_TYPE_REMOVE,
    TEST_TYPE_REPLACE,
    TEST_TYPE_RESERVE,
    TEST_TYPE_STARTS_WITH,
    TEST_TYPE_SUBSTRING,
    TEST_TYPE_TO_C_STRING,
    TEST_TYPE_COUNT,
} TestType;


typedef struct Allocator
{
    uint64_t bytes_used;
    bool force_allocation_failure;
} Allocator;

typedef struct Test
{
    Allocator allocator;
    Allocator bad_allocator;
    RandomGenerator generator;
    TestType type;
} Test;


static const char* describe_test(TestType type)
{
    switch(type)
    {
        case TEST_TYPE_ADD_END:             return "Add End";
        case TEST_TYPE_ADD_MIDDLE:          return "Add Middle";
        case TEST_TYPE_ADD_START:           return "Add Start";
        case TEST_TYPE_APPEND:              return "Append";
        case TEST_TYPE_APPEND_C_STRING:     return "Append C String";
        case TEST_TYPE_ASSIGN:              return "Assign";
        case TEST_TYPE_COPY:                return "Copy";
        case TEST_TYPE_DESTROY:             return "Destroy";
        case TEST_TYPE_ENDS_WITH:           return "Ends With";
        case TEST_TYPE_FIND_FIRST_CHAR:     return "Find First Char";
        case TEST_TYPE_FIND_FIRST_STRING:   return "Find First String";
        case TEST_TYPE_FIND_LAST_CHAR:      return "Find Last Char";
        case TEST_TYPE_FIND_LAST_STRING:    return "Find Last String";
        case TEST_TYPE_FROM_BUFFER:         return "From Buffer";
        case TEST_TYPE_FROM_C_STRING:       return "From C String";
        case TEST_TYPE_FUZZ_ASSIGN:         return "Fuzz Assign";
        case TEST_TYPE_GET_CONTENTS:        return "Get Contents";
        case TEST_TYPE_GET_CONTENTS_CONST:  return "Get Contents Const";
        case TEST_TYPE_INITIALISE:          return "Initialise";
        case TEST_TYPE_ITERATOR_NEXT:       return "Iterator Next";
        case TEST_TYPE_ITERATOR_PRIOR:      return "Iterator Prior";
        case TEST_TYPE_ITERATOR_SET_STRING: return "Iterator Set String";
        case TEST_TYPE_REMOVE:              return "Remove";
        case TEST_TYPE_REPLACE:             return "Replace";
        case TEST_TYPE_RESERVE:             return "Reserve";
        case TEST_TYPE_STARTS_WITH:         return "Starts With";
        case TEST_TYPE_SUBSTRING:           return "Substring";
        case TEST_TYPE_TO_C_STRING:         return "To C String";
        default:
        {
            ASSERT(false);
            return "Unknown";
        }
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

    const char* contents = ad_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"курица蛋");
    bool size_correct =
            ad_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

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

    const char* contents = ad_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"ку蛋рица");
    bool size_correct =
            ad_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

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

    const char* contents = ad_string_get_contents_const(&outer.value);
    bool contents_match = strings_match(contents, u8"蛋курица");
    bool size_correct =
            ad_string_get_count(&outer.value) == string_size(contents);
    bool result = contents_match && size_correct;

    ad_string_destroy(&outer.value);
    ad_string_destroy(&inner.value);

    return result;
}

static bool test_append(Test* test)
{
    const char* a = u8"a猫🍌";
    const char* b = u8"a猫🍌";
    AdMaybeString base =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    AdMaybeString extension =
                ad_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(base.valid);
    ASSERT(extension.valid);

    bool appended = ad_string_append(&base.value, &extension.value);
    ASSERT(appended);

    const char* combined = ad_string_get_contents_const(&base.value);
    bool contents_match = strings_match(combined, u8"a猫🍌a猫🍌");
    bool size_correct =
            ad_string_get_count(&base.value) == string_size(combined);
    bool result = contents_match && size_correct;

    ad_string_destroy(&base.value);
    ad_string_destroy(&extension.value);

    return result;
}

static bool test_append_c_string(Test* test)
{
    const char* a = u8"👌🏼";
    const char* b = u8"🙋🏾‍♀️";
    AdMaybeString base =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(base.valid);

    bool appended = ad_string_append_c_string(&base.value, b);
    ASSERT(appended);

    const char* combined = ad_string_get_contents_const(&base.value);
    bool contents_match = strings_match(combined, u8"👌🏼🙋🏾‍♀️");
    bool size_correct =
            ad_string_get_count(&base.value) == string_size(combined);
    bool result = contents_match && size_correct;

    ad_string_destroy(&base.value);

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
    ASSERT(original.valid);
    ASSERT(copy.valid);

    const char* original_contents = ad_string_get_contents_const(&original.value);
    const char* copy_contents = ad_string_get_contents_const(&copy.value);

    bool size_correct =
            ad_string_get_count(&original.value) == ad_string_get_count(&copy.value);
    bool result = strings_match(original_contents, copy_contents)
            && size_correct
            && original.value.allocator == copy.value.allocator;

    ad_string_destroy(&original.value);
    ad_string_destroy(&copy.value);

    return result;
}

static bool test_destroy(Test* test)
{
    const char* reference = "Moist";
    AdMaybeString original =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(original.valid);

    bool destroyed = ad_string_destroy(&original.value);

    int post_count = ad_string_get_count(&original.value);

    return destroyed
            && post_count == 0
            && original.value.allocator == &test->allocator;
}

static bool test_ends_with(Test* test)
{
    const char* a = u8"Wow a猫🍌";
    const char* b = u8"a猫🍌";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    AdMaybeString ending =
            ad_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(ending.valid);

    bool result = ad_string_ends_with(&string.value, &ending.value);

    ad_string_destroy(&string.value);
    ad_string_destroy(&ending.value);

    return result;
}

static bool test_find_first_char(Test* test)
{
    const char* a = "a A AA s";
    int known_index = 2;

    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(string.valid);

    AdMaybeInt index = ad_string_find_first_char(&string.value, 'A');

    bool result = index.value == known_index;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_find_first_string(Test* test)
{
    const char* a = u8"وَالشَّمْسُ تَجْرِي لِمُسْتَقَرٍّ لَّهَا ذَٰلِكَ تَقْدِيرُ الْعَزِيزِ الْعَلِيمِ";
    const char* b = u8"الْعَ";
    int known_index = 112;

    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    AdMaybeString target =
            ad_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(target.valid);

    AdMaybeInt index =
            ad_string_find_first_string(&string.value, &target.value);

    bool result = index.value == known_index;

    ad_string_destroy(&string.value);
    ad_string_destroy(&target.value);

    return result;
}

static bool test_find_last_char(Test* test)
{
    const char* a = "a A AA s";
    int known_index = 5;

    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    ASSERT(string.valid);

    AdMaybeInt index = ad_string_find_last_char(&string.value, 'A');

    bool result = index.value == known_index;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_find_last_string(Test* test)
{
    const char* a = u8"My 1st page הדף מספר 2 שלי My 3rd page";
    const char* b = u8"My";
    int known_index = 37;

    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    AdMaybeString target =
            ad_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(target.valid);

    AdMaybeInt index = ad_string_find_last_string(&string.value, &target.value);

    bool result = (index.value == known_index);

    ad_string_destroy(&string.value);
    ad_string_destroy(&target.value);

    return result;
}

static bool test_from_c_string(Test* test)
{
    const char* reference = u8"a猫🍌";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    const char* contents = ad_string_get_contents_const(&string.value);

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
    const char* contents = ad_string_get_contents_const(&string.value);

    bool result = string.valid
            && strings_match(contents, reference)
            && ad_string_get_count(&string.value) == bytes;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_get_contents(Test* test)
{
    const char* reference = "Test me in the most basic way possible.";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    char* contents = ad_string_get_contents(&string.value);

    bool result = contents && strings_match(contents, reference);

    ad_string_destroy(&string.value);

    return result;
}

static bool test_get_contents_const(Test* test)
{
    const char* original = u8"👌🏼";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(original, &test->allocator);
    ASSERT(string.valid);

    const char* after = ad_string_get_contents_const(&string.value);

    bool result = after && strings_match(original, after);

    ad_string_destroy(&string.value);

    return result;
}

static bool test_initialise(Test* test)
{
    AdString string;
    ad_string_initialise_with_allocator(&string, &test->allocator);
    return string.allocator == &test->allocator
            && ad_string_get_count(&string) == 0;
}

static bool test_iterator_next(Test* test)
{
    const char* reference = u8"Бума́га всё сте́рпит.";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AdCodepointIterator it;
    ad_codepoint_iterator_set_string(&it, &string.value);
    ad_codepoint_iterator_next(&it);

    int before_index = ad_codepoint_iterator_get_index(&it);
    AdMaybeChar32 next = ad_codepoint_iterator_next(&it);
    int after_index = ad_codepoint_iterator_get_index(&it);

    bool result = next.valid
            && next.value == U'у'
            && before_index == 2
            && after_index == 4;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_iterator_prior(Test* test)
{
    const char* reference = u8"Бума́га всё сте́рпит.";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AdCodepointIterator it;
    ad_codepoint_iterator_set_string(&it, &string.value);
    ad_codepoint_iterator_end(&it);
    ad_codepoint_iterator_prior(&it);

    int before_index = ad_codepoint_iterator_get_index(&it);
    AdMaybeChar32 prior = ad_codepoint_iterator_prior(&it);
    int after_index = ad_codepoint_iterator_get_index(&it);

    bool result = prior.valid
            && prior.value == U'т'
            && before_index == 38
            && after_index == 36;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_iterator_set_string(Test* test)
{
    const char* reference = "9876543210";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AdCodepointIterator it;
    ad_codepoint_iterator_set_string(&it, &string.value);

    bool result =
            ad_codepoint_iterator_get_string(&it) == &string.value
            && ad_codepoint_iterator_get_index(&it) == 0;

    ad_string_destroy(&string.value);

    return result;
}

static bool test_remove(Test* test)
{
    const char* reference = "9876543210";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AdStringRange range = {3, 6};
    ad_string_remove(&string.value, &range);
    const char* contents = ad_string_get_contents_const(&string.value);
    bool result = strings_match(contents, "9873210");

    ad_string_destroy(&string.value);

    return result;
}

static bool test_replace(Test* test)
{
    const char* reference = "9876543210";
    const char* insert = "abc";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    AdMaybeString replacement =
            ad_string_from_c_string_with_allocator(insert, &test->allocator);
    ASSERT(string.valid);
    ASSERT(replacement.valid);

    AdStringRange range = {3, 6};
    ad_string_replace(&string.value, &range, &replacement.value);
    const char* contents = ad_string_get_contents_const(&string.value);
    bool result = strings_match(contents, "987abc3210");

    ad_string_destroy(&string.value);
    ad_string_destroy(&replacement.value);

    return result;
}

static bool test_reserve(Test* test)
{
    AdString string;
    ad_string_initialise_with_allocator(&string, &test->allocator);
    int requested_cap = 100;

    bool reserved = ad_string_reserve(&string, requested_cap);
    int cap = ad_string_get_capacity(&string);
    bool result = reserved && cap == requested_cap;

    ad_string_destroy(&string);

    return result;
}

static bool test_starts_with(Test* test)
{
    const char* a = u8"a猫🍌 Wow";
    const char* b = u8"a猫🍌";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(a, &test->allocator);
    AdMaybeString ending =
            ad_string_from_c_string_with_allocator(b, &test->allocator);
    ASSERT(string.valid);
    ASSERT(ending.valid);

    bool result = ad_string_starts_with(&string.value, &ending.value);

    ad_string_destroy(&string.value);
    ad_string_destroy(&ending.value);

    return result;
}

static bool test_substring(Test* test)
{
    const char* reference = "9876543210";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    AdStringRange range = {3, 6};
    AdMaybeString middle = ad_string_substring(&string.value, &range);
    ASSERT(middle.valid);

    const char* contents = ad_string_get_contents_const(&middle.value);
    bool result = strings_match(contents, "654")
            && string.value.allocator == &test->allocator;

    ad_string_destroy(&string.value);
    ad_string_destroy(&middle.value);

    return result;
}

static bool test_to_c_string(Test* test)
{
    const char* reference = "It's okay.";
    AdMaybeString string =
            ad_string_from_c_string_with_allocator(reference, &test->allocator);
    ASSERT(string.valid);

    char* copy =
            ad_string_to_c_string_with_allocator(&string.value,
                    &test->allocator);
    bool result = strings_match(copy, reference);

    ad_string_destroy(&string.value);
    ad_c_string_deallocate_with_allocator(&test->allocator, copy);

    return result;
}

static bool run_test(Test* test)
{
    switch(test->type)
    {
        case TEST_TYPE_ADD_END:             return test_add_end(test);
        case TEST_TYPE_ADD_MIDDLE:          return test_add_middle(test);
        case TEST_TYPE_ADD_START:           return test_add_start(test);
        case TEST_TYPE_APPEND:              return test_append(test);
        case TEST_TYPE_APPEND_C_STRING:     return test_append_c_string(test);
        case TEST_TYPE_ASSIGN:              return test_assign(test);
        case TEST_TYPE_COPY:                return test_copy(test);
        case TEST_TYPE_DESTROY:             return test_destroy(test);
        case TEST_TYPE_ENDS_WITH:           return test_ends_with(test);
        case TEST_TYPE_FIND_FIRST_CHAR:     return test_find_first_char(test);
        case TEST_TYPE_FIND_FIRST_STRING:   return test_find_first_string(test);
        case TEST_TYPE_FIND_LAST_CHAR:      return test_find_last_char(test);
        case TEST_TYPE_FIND_LAST_STRING:    return test_find_last_string(test);
        case TEST_TYPE_FROM_BUFFER:         return test_from_buffer(test);
        case TEST_TYPE_FROM_C_STRING:       return test_from_c_string(test);
        case TEST_TYPE_FUZZ_ASSIGN:         return fuzz_assign(test);
        case TEST_TYPE_GET_CONTENTS:        return test_get_contents(test);
        case TEST_TYPE_GET_CONTENTS_CONST:  return test_get_contents_const(test);
        case TEST_TYPE_INITIALISE:          return test_initialise(test);
        case TEST_TYPE_ITERATOR_NEXT:       return test_iterator_next(test);
        case TEST_TYPE_ITERATOR_PRIOR:      return test_iterator_prior(test);
        case TEST_TYPE_ITERATOR_SET_STRING: return test_iterator_set_string(test);
        case TEST_TYPE_REMOVE:              return test_remove(test);
        case TEST_TYPE_REPLACE:             return test_replace(test);
        case TEST_TYPE_RESERVE:             return test_reserve(test);
        case TEST_TYPE_STARTS_WITH:         return test_starts_with(test);
        case TEST_TYPE_SUBSTRING:           return test_substring(test);
        case TEST_TYPE_TO_C_STRING:         return test_to_c_string(test);
        default:
        {
            ASSERT(false);
            return false;
        }
    }
}

static bool run_tests()
{
    bool tests_failed[TEST_TYPE_COUNT];
    int total_failed = 0;

    for(int test_index = 0; test_index < TEST_TYPE_COUNT; test_index += 1)
    {
        Test test = {0};
        test.type = (TestType) test_index;
        test.bad_allocator.force_allocation_failure = true;

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
                TestType type = (TestType) test_index;
                const char* test = describe_test(type);
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
    ASSERT(allocator_pointer);

    Allocator* allocator = (Allocator*) allocator_pointer;

    if(allocator->force_allocation_failure)
    {
        AdMemoryBlock empty =
        {
            .memory = NULL,
            .bytes = 0,
        };
        return empty;
    }

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
    ASSERT(allocator_pointer);

    Allocator* allocator = (Allocator*) allocator_pointer;
    allocator->bytes_used -= block.bytes;

    free(block.memory);

    return true;
}
