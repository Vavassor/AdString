#include "../Utility/test.h"

#include <float.h>
#include <math.h>

static bool test_default(Test* test)
{
    const uint64_t value = UINT64_C(0xffffffffffffffff);

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
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

static bool test_default_double(Test* test)
{
    const double value = 789.04;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "789.04";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_big(Test* test)
{
    const double value = 789040000000000000000.0;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "789040000000000000000";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_infinity(Test* test)
{
    const double value = -HUGE_VAL;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = u8"-∞";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_max(Test* test)
{
    const double value = DBL_MAX;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference =
            "179769313486231570814527423731704356798070567525844996598917476803"
            "157260780028538760589558632766878171540458953514382464234321326889"
            "464182768467546703537516986049910576551282076245490090389328944075"
            "868508455133942304583236903222948165808559332123348274797826204144"
            "723168738177180919299881250404026184124858368";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_nan(Test* test)
{
    const double value = NAN;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "NaN";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_one(Test* test)
{
    const double value = 1.0;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "1";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_small(Test* test)
{
    const double value = 0.00000000000078904;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "0";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_double_zero(Test* test)
{
    const double value = 0.0;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "0";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_float_infinity(Test* test)
{
    const float value = -HUGE_VALF;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_float_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = u8"-∞";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_float_max(Test* test)
{
    const float value = FLT_MAX;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_float_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "340282346638528859811704183484516925440";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_float_nan(Test* test)
{
    const float value = NAN;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_float_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "NaN";
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
    const int64_t value = INT64_C(-9223372036854775807) - 1;

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

static bool test_default_scientific_double(Test* test)
{
    const double value = 789.04;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_scientific_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "7.89E2";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_default_scientific_double_small(Test* test)
{
    const double value = 0.00000000000078904;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_scientific_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "7.89E-13";
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

static bool test_max_fraction_digits_double(Test* test)
{
    const double value = 0.00000000000078904;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    format.max_fraction_digits = 17;
    ASSERT(defaulted);

    AftMaybeString string =
            aft_string_from_double_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "0.00000000000078904";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_max_integer_digits(Test* test)
{
    const int value = 123456789;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
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
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.use_significant_digits = true;
    format.max_significant_digits = 4;
    format.min_significant_digits = 0;
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

static bool test_min_fraction_digits(Test* test)
{
    const int value = 1234;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.min_fraction_digits = 3;

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "1234.000";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_min_fraction_digits_double(Test* test)
{
    const double values[3] = {0.78904, 1.78904, 78904.0};
    const char* references[3] = {"0.78904000", "1.78904000", "78904.00000000"};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    format.min_fraction_digits = 8;
    format.max_fraction_digits = 15;
    ASSERT(defaulted);

    bool result = true;

    for(int case_index = 0; case_index < 3; case_index += 1)
    {
        AftMaybeString string =
                aft_string_from_double_with_allocator(values[case_index],
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

static bool test_min_integer_digits(Test* test)
{
    const int value = 1234;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
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

static bool test_min_significant_digits_int(Test* test)
{
    const int value = 123456789;

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format, &test->allocator);
    ASSERT(defaulted);
    format.use_significant_digits = true;
    format.max_significant_digits = 14;
    format.min_significant_digits = 14;
    format.use_grouping = true;

    AftMaybeString string =
            aft_string_from_int_with_allocator(value, &format,
                    &test->allocator);

    const char* reference = "123,456,789.00000";
    const char* contents = aft_string_get_contents_const(&string.value);
    bool result = string.valid && strings_match(reference, contents);

    aft_string_destroy(&string.value);
    aft_decimal_format_destroy(&format);

    return result;
}

static bool test_min_significant_digits_double(Test* test)
{
    const double values[3] = {0.78904, 1.78904, 78904.0};
    const char* references[3] = {"0.78904000", "1.7890400", "78904.000"};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    format.use_significant_digits = true;
    format.min_significant_digits = 8;
    format.max_significant_digits = 15;
    ASSERT(defaulted);

    bool result = true;

    for(int case_index = 0; case_index < 3; case_index += 1)
    {
        AftMaybeString string =
                aft_string_from_double_with_allocator(values[case_index],
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

static bool test_round_ceiling(Test* test)
{
    const int values[11] = {20, 22, 24, 26, 28, 0, -20, -22, -24, -26, -28};
    const char* references[11] =
            {"24", "24", "24", "32", "32", "0", "-16", "-16", "-24", "-24",
                    "-24"};
    const int increments[11] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_CEILING;

    bool result = true;

    for(int case_index = 0; case_index < 11; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_down(Test* test)
{
    const int values[11] = {20, 22, 24, 26, 28, 0, -20, -22, -24, -26, -28};
    const char* references[11] =
            {"16", "16", "24", "24", "24", "0", "-16", "-16", "-24", "-24",
                    "-24"};
    const int increments[11] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_DOWN;

    bool result = true;

    for(int case_index = 0; case_index < 11; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_floor(Test* test)
{
    const int values[11] = {20, 22, 24, 26, 28, 0, -20, -22, -24, -26, -28};
    const char* references[11] =
            {"16", "16", "24", "24", "24", "0", "-24", "-24", "-24", "-32",
                    "-32"};
    const int increments[11] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_FLOOR;

    bool result = true;

    for(int case_index = 0; case_index < 11; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_half_down(Test* test)
{
    const int values[11] = {20, 22, 24, 26, 28, 0, -20, -22, -24, -26, -28};
    const char* references[11] =
            {"16", "24", "24", "24", "24", "0","-16", "-24", "-24", "-24",
                    "-24"};
    const int increments[11] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_DOWN;

    bool result = true;

    for(int case_index = 0; case_index < 11; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_half_even(Test* test)
{
    const int values[9] = {20, 24, 28, 32, 0, -20, -24, -28, -32};
    const char* references[9] =
            {"16", "24", "32", "32", "0", "-16", "-24", "-32", "-32"};
    const int increments[9] = {8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);

    bool result = true;

    for(int case_index = 0; case_index < 9; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_half_up(Test* test)
{
    const int values[7] = {20, 24, 28, 0, -20, -24, -28};
    const char* references[7] = {"24", "24", "32", "0", "-24", "-24", "-32"};
    const int increments[7] = {8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_UP;

    bool result = true;

    for(int case_index = 0; case_index < 7; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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

static bool test_round_up(Test* test)
{
    const int values[11] = {20, 22, 24, 26, 28, 0, -20, -22, -24, -26, -28};
    const char* references[11] =
            {"24", "24", "24", "32", "32", "0", "-24", "-24", "-24", "-32",
                    "-32"};
    const int increments[11] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

    AftDecimalFormat format;
    bool defaulted =
            aft_decimal_format_default_with_allocator(&format,
                    &test->allocator);
    ASSERT(defaulted);
    format.rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_UP;

    bool result = true;

    for(int case_index = 0; case_index < 11; case_index += 1)
    {
        format.rounding_increment_int = increments[case_index];

        AftMaybeString string =
                aft_string_from_int_with_allocator(values[case_index],
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
    add_test(&suite, test_default_double, "Test Default double");
    add_test(&suite, test_default_double_big, "Test Default double Big");
    add_test(&suite, test_default_double_infinity,
            "Test Default double Infinity");
    add_test(&suite, test_default_double_max, "Test Default double Max");
    add_test(&suite, test_default_double_nan, "Test Default double NaN");
    add_test(&suite, test_default_double_one, "Test Default double One");
    add_test(&suite, test_default_double_small, "Test Default double Small");
    add_test(&suite, test_default_double_zero, "Test Default double Zero");
    add_test(&suite, test_default_float_infinity,
                "Test Default float Infinity");
    add_test(&suite, test_default_float_max, "Test Default float Max");
    add_test(&suite, test_default_float_nan, "Test Default float NaN");
    add_test(&suite, test_default_int, "Test Default int");
    add_test(&suite, test_default_int64, "Test Default int64_t");
    add_test(&suite, test_default_scientific_double,
                "Test Default Scientific double");
    add_test(&suite, test_default_scientific_double_small,
                "Test Default Scientific double Small");
    add_test(&suite, test_hexadecimal, "Test Hexadecimal");
    add_test(&suite, test_max_fraction_digits_double,
                "Test Max Fraction Digits double");
    add_test(&suite, test_max_integer_digits, "Test Max Integer Digits");
    add_test(&suite, test_max_significant_digits,
            "Test Max Significant Digits");
    add_test(&suite, test_min_fraction_digits, "Test Min Fraction Digits");
    add_test(&suite, test_min_fraction_digits_double,
                    "Test Min Fraction Digits double");
    add_test(&suite, test_min_integer_digits, "Test Min Integer Digits");
    add_test(&suite, test_min_significant_digits_int,
                "Test Min Significant Digits int");
    add_test(&suite, test_min_significant_digits_double,
                "Test Min Significant Digits double");
    add_test(&suite, test_round_ceiling, "Test Round Down");
    add_test(&suite, test_round_down, "Test Round Down");
    add_test(&suite, test_round_floor, "Test Round Floor");
    add_test(&suite, test_round_half_down, "Test Round Half Down");
    add_test(&suite, test_round_half_even, "Test Round Half Even");
    add_test(&suite, test_round_half_up, "Test Round Half Up");
    add_test(&suite, test_round_up, "Test Round Up");

    bool success = run_tests(&suite);
    return !success;
}
