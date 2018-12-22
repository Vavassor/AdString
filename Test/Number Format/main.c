#include "../Utility/test.h"

static bool test_default(Test* test)
{
    const uint64_t value = UINT64_C(0xffffffffffffffff);

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_uint64_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "18446744073709551615";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_int(Test* test)
{
    const int value = -7;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "-7";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_int64(Test* test)
{
    const int64_t value = INT64_C(-9223372036854775808);

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_int64_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "-9223372036854775808";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_hexadecimal(Test* test)
{
    const uint64_t value = UINT64_C(0xffffffffffffffff);

    AftBaseFormat format =
    {
        .base = 16,
        .use_uppercase = false,
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

static bool test_max_integer_digits(Test* test)
{
    const int value = 123456789;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);
    format.max_integer_digits = 4;
    format.use_grouping = true;

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "6,789";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_max_significant_digits(Test* test)
{
    const int value = 123456789;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);
    format.use_significant_digits = true;
    format.max_significant_digits = 4;
    format.use_grouping = true;

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "123,400,000";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_min_integer_digits(Test* test)
{
    const int value = 1234;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);
    format.min_integer_digits = 7;
    format.use_grouping = true;
    format.primary_grouping_size = 2;

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "00,012,34";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_round_half_even(Test* test)
{
    const uint64_t values[4] = {20, 24, 28, 32};
    const char* references[4] = {"16", "24", "32", "32"};
    const int increments[4] = {8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);

    bool result = true;

    for(int case_index = 0; case_index < 4; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_uint64_with_allocator(values[case_index],
                        &format, &test->allocator);

        const char* contents = aft_string_get_contents_const(&string.value);
        result = result
                && string.valid
                && strings_match(references[case_index], contents);

        aft_string_destroy(&string.value);
    }

    aft_decimal_format_destroy(&format);

    return result;
}

int main(int argc, const char** argv)
{
    Suite suite = {0};

    add_test(&suite, test_default, "Test Default");
    add_test(&suite, test_default_int, "Test Default int");
    add_test(&suite, test_default_int64, "Test Default int64_t");
    add_test(&suite, test_hexadecimal, "Test Hexadecimal");
    add_test(&suite, test_max_integer_digits, "Test Max Integer Digits");
    add_test(&suite, test_max_significant_digits,
            "Test Max Significant Digits");
    add_test(&suite, test_min_integer_digits, "Test Min Integer Digits");
    add_test(&suite, test_round_half_even, "Test Round Half Even");

    bool success = run_tests(&suite);
    return !success;
}
