#include "../Utility/test.h"

static bool test_default(Test* test)
{
    const uint64_t value = 0xffffffffffffffff;

    AftNumberFormat format;
    bool defaulted =
            aft_number_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_uint64_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "18446744073709551615";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_number_format_destroy(&format);

    return result;
}

static bool test_hexadecimal(Test* test)
{
    const uint64_t value = 0xffffffffffffffff;

    AftBaseFormat format =
    {
        .base = 16,
        .use_uppercase = true,
    };
    AftMaybeString string =
            aft_ascii_from_uint64_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "ffffffffffffffff";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);

    return result;
}

int main(int argc, const char** argv)
{
    Suite suite = {0};

    add_test(&suite, test_default, "Test Default");
    add_test(&suite, test_hexadecimal, "Test Hexadecimal");

    bool success = run_tests(&suite);
    return !success;
}
