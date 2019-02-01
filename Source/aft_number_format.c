#include <AftString/aft_string.h>

#include "floating_point_format.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>


#define AFT_ASSERT(expression) \
    assert(expression)

#define AFT_UINT64_MAX_BINARY_DIGITS 64
#define AFT_UINT64_MAX_DECIMAL_DIGITS 20


typedef struct DecimalFormatter
{
    uint64_t digits[AFT_UINT64_MAX_DECIMAL_DIGITS];
    const AftDecimalFormat* format;
    const AftString* group_separator;
    AftString* string;
    int digit_index;
    int digit_total;
    int digits_shown;
} DecimalFormatter;


static const char* base36_digits_uppercase = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char* base36_digits_lowercase = "0123456789abcdefghijklmnopqrstuvwxyz";

static const uint64_t powers_of_ten[AFT_UINT64_MAX_DECIMAL_DIGITS] =
{
    UINT64_C(10000000000000000000),
    UINT64_C(1000000000000000000),
    UINT64_C(100000000000000000),
    UINT64_C(10000000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(100000000000000),
    UINT64_C(10000000000000),
    UINT64_C(1000000000000),
    UINT64_C(100000000000),
    UINT64_C(10000000000),
    UINT64_C(1000000000),
    UINT64_C(100000000),
    UINT64_C(10000000),
    UINT64_C(1000000),
    UINT64_C(100000),
    UINT64_C(10000),
    UINT64_C(1000),
    UINT64_C(100),
    UINT64_C(10),
    UINT64_C(1),
};

static int count_digits(int value)
{
    int digits = 0;
    for(; value; digits += 1)
    {
        value /= 10;
    }
    return digits;
}

static bool default_number_symbols(AftNumberSymbols* symbols, void* allocator)
{
    struct
    {
        const char* symbol;
        AftString* result;
    } table[22] =
    {
        {"0", &symbols->digits[0]},
        {"1", &symbols->digits[1]},
        {"2", &symbols->digits[2]},
        {"3", &symbols->digits[3]},
        {"4", &symbols->digits[4]},
        {"5", &symbols->digits[5]},
        {"6", &symbols->digits[6]},
        {"7", &symbols->digits[7]},
        {"8", &symbols->digits[8]},
        {"9", &symbols->digits[9]},
        {".", &symbols->radix_separator},
        {",", &symbols->group_separator},
        {"%", &symbols->percent_sign},
        {"+", &symbols->plus_sign},
        {"-", &symbols->minus_sign},
        {"E", &symbols->exponential_sign},
        {"\xC3\x97", &symbols->superscripting_exponent_sign},
        {"\xE2\x80\xB0", &symbols->per_mille_sign},
        {"\xE2\x88\x9E", &symbols->infinity_sign},
        {"NaN", &symbols->nan_sign},
        {".", &symbols->currency_decimal_separator},
        {",", &symbols->currency_group_separator},
    };

    for(int symbol_index = 0; symbol_index < 22; symbol_index += 1)
    {
        const char* symbol = table[symbol_index].symbol;
        AftMaybeString result = aft_string_copy_c_string_with_allocator(symbol, allocator);

        if(!result.valid)
        {
            for(int destroy_index = 0;
                    destroy_index < symbol_index;
                    destroy_index += 1)
            {
                aft_string_destroy(table[destroy_index].result);
            }

            return false;
        }

        *table[symbol_index].result = result.value;
    }

    return true;
}

static void destroy_number_symbols(AftNumberSymbols* symbols)
{
    for(int digit_index = 0; digit_index < 10; digit_index += 1)
    {
        aft_string_destroy(&symbols->digits[digit_index]);
    }

    aft_string_destroy(&symbols->currency_decimal_separator);
    aft_string_destroy(&symbols->currency_group_separator);
    aft_string_destroy(&symbols->radix_separator);
    aft_string_destroy(&symbols->exponential_sign);
    aft_string_destroy(&symbols->group_separator);
    aft_string_destroy(&symbols->infinity_sign);
    aft_string_destroy(&symbols->minus_sign);
    aft_string_destroy(&symbols->nan_sign);
    aft_string_destroy(&symbols->per_mille_sign);
    aft_string_destroy(&symbols->percent_sign);
    aft_string_destroy(&symbols->plus_sign);
    aft_string_destroy(&symbols->superscripting_exponent_sign);
}

static uint64_t round_down(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * (value / increment);
}

static uint64_t round_half_down(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * ((value + increment / 2 - 1) / increment);
}

static uint64_t round_half_even(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    uint64_t even_increment = (1 & (value / increment)) ^ 1;
    return increment * ((value + increment / 2 - even_increment) / increment);
}

static uint64_t round_half_up(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * ((value + (increment / 2)) / increment);
}

static uint64_t round_up(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * ((value + (increment - 1)) / increment);
}

static uint64_t round_uint64_and_sign(uint64_t value, uint64_t increment, bool sign, AftDecimalFormatRoundingMode rounding_mode)
{
    AFT_ASSERT(increment < value || value == 0);

    switch(rounding_mode)
    {
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_CEILING:
        {
            if(sign)
            {
                return round_down(value, increment);
            }
            else
            {
                return round_up(value, increment);
            }
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_UP:
        {
            return round_up(value, increment);
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_DOWN:
        {
            return round_down(value, increment);
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_FLOOR:
        {
            if(sign)
            {
                return round_up(value, increment);
            }
            else
            {
                return round_down(value, increment);
            }
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_DOWN:
        {
            return round_half_down(value, increment);
        }
        default:
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN:
        {
            return round_half_even(value, increment);
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_UP:
        {
            return round_half_up(value, increment);
        }
    }
}

static bool separate_group_at_location(const AftDecimalFormat* format, int index, int length)
{
    int from_end = index + 1;

    if(!format->use_grouping || from_end == length)
    {
        return false;
    }
    else
    {
        int secondary_index = from_end - format->primary_grouping_size;
        if(secondary_index > 0)
        {
            return secondary_index % format->secondary_grouping_size == 0;
        }
        else
        {
            return from_end == format->primary_grouping_size;
        }
    }
}

static void append_pattern(AftString* string, const AftString* pattern, const AftDecimalFormat* format)
{
    AftCodepointIterator it;
    aft_codepoint_iterator_set_string(&it, (AftString*) pattern);

    int prior_index = 0;

    for(;;)
    {
        AftMaybeChar32 codepoint = aft_codepoint_iterator_next(&it);

        if(!codepoint.valid)
        {
            break;
        }

        int index = aft_codepoint_iterator_get_index(&it);

        switch(codepoint.value)
        {
            case U'+':
            {
                if(format->use_explicit_plus_sign)
                {
                    aft_string_append(string, &format->symbols.plus_sign);
                }
                break;
            }
            case U'-':
            {
                aft_string_append(string, &format->symbols.minus_sign);
                break;
            }
            case U'$':
            {
                AFT_ASSERT(format->style == AFT_DECIMAL_FORMAT_STYLE_CURRENCY);
                aft_string_append(string, &format->currency.symbol);
                break;
            }
            case U'%':
            {
                AFT_ASSERT(format->style == AFT_DECIMAL_FORMAT_STYLE_PERCENT);
                aft_string_append(string, &format->symbols.percent_sign);
                break;
            }
            case U'‰':
            {
                AFT_ASSERT(format->style == AFT_DECIMAL_FORMAT_STYLE_PERCENT);
                aft_string_append(string, &format->symbols.per_mille_sign);
                break;
            }
            default:
            {
                AftStringSlice slice = aft_string_slice_string(pattern, prior_index, index);
                aft_string_append_slice(string, slice);
                break;
            }
        }

        prior_index = index;
    }
}

static uint64_t apply_multiplier(uint64_t value, const AftDecimalFormat* format)
{
    uint64_t result = value;

    if(format->style == AFT_DECIMAL_FORMAT_STYLE_PERCENT)
    {
        result *= format->percent.multiplier;
    }

    return result;
}

static uint64_t apply_rounding(uint64_t value, bool sign, const AftDecimalFormat* format)
{
    uint64_t result = value;

    if(format->rounding_increment_int != 1)
    {
        result = round_uint64_and_sign(value, format->rounding_increment_int, sign, format->rounding_mode);
    }

    return result;
}

static void apply_prefix(AftString* string, const AftDecimalFormat* format, bool sign)
{
    if(sign)
    {
        append_pattern(string, &format->negative_prefix_pattern, format);
    }
    else
    {
        append_pattern(string, &format->positive_prefix_pattern, format);
    }
}

static void digitize(DecimalFormatter* formatter, uint64_t value)
{
    int digit_index = 0;
    do
    {
        formatter->digits[digit_index] = value % 10;
        digit_index += 1;
        value /= 10;
    } while(value && digit_index < AFT_UINT64_MAX_DECIMAL_DIGITS);

    formatter->digit_index = digit_index;
    formatter->digit_total = digit_index;
}

static void pad_leading_zeros_and_set_digit_limits(DecimalFormatter* formatter)
{
    int digit_index = formatter->digit_index;
    int digit_total = formatter->digit_total;
    const AftDecimalFormat* format = formatter->format;

    int digits_shown;

    if(format->use_significant_digits)
    {
        digits_shown = format->max_significant_digits;
    }
    else
    {
        digits_shown = format->max_integer_digits;

        if(digits_shown < digit_total)
        {
            digit_index -= digit_total - digits_shown;
        }
        else if(digit_total < format->min_integer_digits)
        {
            digit_index += format->min_integer_digits - digit_total;

            for(;;)
            {
                if(digit_index - 1 < digit_total)
                {
                    break;
                }

                digit_index -= 1;

                if(separate_group_at_location(format, digit_index, format->min_integer_digits))
                {
                    aft_string_append(formatter->string, formatter->group_separator);
                }

                aft_string_append(formatter->string, &format->symbols.digits[0]);
            }

            digit_total = format->min_integer_digits;
        }
    }

    formatter->digit_index = digit_index;
    formatter->digit_total = digit_total;
    formatter->digits_shown = digits_shown;
}

static void set_digit_limits_scientific(DecimalFormatter* formatter)
{
    int digit_index = formatter->digit_index;
    int digit_total = formatter->digit_total;
    const AftDecimalFormat* format = formatter->format;

    int digits_shown;

    if(format->use_significant_digits)
    {
        digits_shown = format->max_significant_digits;
    }
    else
    {
        digits_shown = format->max_fraction_digits + 1;
    }

    formatter->digit_index = digit_index;
    formatter->digit_total = digit_total;
    formatter->digits_shown = digits_shown;
}

static void format_significant_integer_digits(DecimalFormatter* formatter)
{
    int digit_index = formatter->digit_index;

    for(int digit_limiter = 0;
            digit_limiter < formatter->digits_shown;
            digit_limiter += 1)
    {
        digit_index -= 1;

        if(digit_index < 0)
        {
            break;
        }

        if(separate_group_at_location(formatter->format, digit_index, formatter->digit_total))
        {
            aft_string_append(formatter->string, formatter->group_separator);
        }

        int digit = (int) formatter->digits[digit_index];
        aft_string_append(formatter->string, &formatter->format->symbols.digits[digit]);
    }

    formatter->digit_index = digit_index;
}

static void format_scientific_digits_and_exponent(DecimalFormatter* formatter)
{
    const AftDecimalFormat* format = formatter->format;

    int digit_index = formatter->digit_index - 1;

    aft_string_append(formatter->string, &format->symbols.digits[formatter->digits[digit_index]]);
    digit_index -= 1;

    if(formatter->digits_shown > 1)
    {
        aft_string_append(formatter->string, &format->symbols.radix_separator);

        for(int digit_limiter = 1;
                digit_limiter < formatter->digits_shown && digit_index >= 0;
                digit_limiter += 1, digit_index -= 1)
        {
            int digit = (int) formatter->digits[digit_index];
            aft_string_append(formatter->string, &format->symbols.digits[digit]);
        }

        aft_string_append(formatter->string, &format->symbols.exponential_sign);

        int exponent = formatter->digit_index - 1;
        if(exponent < 0)
        {
            aft_string_append(formatter->string, &format->symbols.minus_sign);
            exponent = -exponent;
        }

        AFT_ASSERT(count_digits(exponent) <= 3);
        int exponent_digits[3];
        int exponent_digit_index = 0;
        do
        {
            exponent_digits[exponent_digit_index] = exponent % 10;
            exponent_digit_index += 1;
            exponent /= 10;
        } while(exponent);

        for(exponent_digit_index -= 1;
                exponent_digit_index >= 0;
                exponent_digit_index -= 1)
        {
            int digit = exponent_digits[exponent_digit_index];
            aft_string_append(formatter->string, &format->symbols.digits[digit]);
        }
    }
}

static void pad_zeros_without_separator(AftString* string, const AftDecimalFormat* format, int count)
{
    for(int zero_index = 0;
            zero_index < count;
            zero_index += 1)
    {
        aft_string_append(string, &format->symbols.digits[0]);
    }
}

static void format_remaining_integer_digits_and_fraction_digits(DecimalFormatter* formatter)
{
    const AftDecimalFormat* format = formatter->format;
    int digit_index = formatter->digit_index;

    if(format->use_significant_digits)
    {
        for(digit_index -= 1; digit_index >= 0; digit_index -= 1)
        {
            if(separate_group_at_location(format, digit_index, formatter->digit_total))
            {
                aft_string_append(formatter->string, formatter->group_separator);
            }

            aft_string_append(formatter->string, &format->symbols.digits[0]);
        }

        if(format->min_significant_digits > formatter->digit_total)
        {
            aft_string_append(formatter->string, &format->symbols.radix_separator);

            int zeros = format->min_significant_digits - formatter->digit_total;
            pad_zeros_without_separator(formatter->string, format, zeros);
        }
    }
    else if(format->min_fraction_digits > 0)
    {
        aft_string_append(formatter->string, &format->symbols.radix_separator);
        pad_zeros_without_separator(formatter->string, format, format->min_fraction_digits);
    }

    formatter->digit_index = digit_index;
}

static void apply_suffix(AftString* string, const AftDecimalFormat* format, bool sign)
{
    if(sign)
    {
        append_pattern(string, &format->negative_suffix_pattern, format);
    }
    else
    {
        append_pattern(string, &format->positive_suffix_pattern, format);
    }
}

static AftMaybeString string_from_uint64_and_sign(uint64_t value, bool sign, const AftDecimalFormat* format, void* allocator)
{
    AFT_ASSERT(aft_decimal_format_validate(format));

    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    value = apply_multiplier(value, format);
    value = apply_rounding(value, sign, format);

    DecimalFormatter formatter =
    {
        .format = format,
        .string = &result.value,
    };

    formatter.group_separator = &format->symbols.group_separator;
    if(format->style == AFT_DECIMAL_FORMAT_STYLE_CURRENCY)
    {
        formatter.group_separator = &format->symbols.currency_group_separator;
    }

    apply_prefix(formatter.string, formatter.format, sign);
    digitize(&formatter, value);

    switch(format->style)
    {
        case AFT_DECIMAL_FORMAT_STYLE_CURRENCY:
        case AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT:
        case AFT_DECIMAL_FORMAT_STYLE_PERCENT:
        {
            pad_leading_zeros_and_set_digit_limits(&formatter);
            format_significant_integer_digits(&formatter);
            format_remaining_integer_digits_and_fraction_digits(&formatter);
            break;
        }
        case AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC:
        {
            set_digit_limits_scientific(&formatter);
            format_scientific_digits_and_exponent(&formatter);
            break;
        }
    }

    apply_suffix(formatter.string, formatter.format, sign);

    return result;
}

static void format_float_result(AftMaybeString* result, const DecimalQuantity* quantity, const AftDecimalFormat* format)
{
    switch(quantity->type)
    {
        case DECIMAL_QUANTITY_TYPE_INFINITY:
        {
            apply_prefix(&result->value, format, quantity->sign);
            aft_string_append(&result->value, &format->symbols.infinity_sign);
            apply_suffix(&result->value, format, quantity->sign);
            break;
        }
        case DECIMAL_QUANTITY_TYPE_NAN:
        {
            aft_string_assign(&result->value, &format->symbols.nan_sign);
            break;
        }
        case DECIMAL_QUANTITY_TYPE_NORMAL:
        {
            apply_prefix(&result->value, format, quantity->sign);

            const char* digits_contents = aft_string_get_contents_const(&quantity->digits);
            int digits_count = aft_string_get_count(&quantity->digits);

            const AftString* group_separator = &format->symbols.group_separator;
            if(format->style == AFT_DECIMAL_FORMAT_STYLE_CURRENCY)
            {
                const AftString* group_separator = &format->symbols.currency_group_separator;
            }

            switch(format->style)
            {
                case AFT_DECIMAL_FORMAT_STYLE_CURRENCY:
                case AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT:
                case AFT_DECIMAL_FORMAT_STYLE_PERCENT:
                {
                    if(quantity->exponent <= 0)
                    {
                        aft_string_append(&result->value, &format->symbols.digits[0]);

                        if(digits_count >= 1 && (digits_count != 1 || digits_contents[0] != 0))
                        {
                            aft_string_append(&result->value, &format->symbols.radix_separator);
                            pad_zeros_without_separator(&result->value, format, -quantity->exponent);

                            for(int digit_index = 0;
                                    digit_index < digits_count;
                                    digit_index += 1)
                            {
                                int digit = digits_contents[digit_index];
                                aft_string_append(&result->value, &format->symbols.digits[digit]);
                            }

                            if(format->use_significant_digits)
                            {
                                if(digits_count < format->min_significant_digits)
                                {
                                    int zeros = format->min_significant_digits - digits_count;
                                    pad_zeros_without_separator(&result->value, format, zeros);
                                }
                            }
                            else
                            {
                                int fraction_digits = digits_count - quantity->exponent;
                                if(fraction_digits < format->min_fraction_digits)
                                {
                                    int zeros = format->min_fraction_digits - fraction_digits;
                                    pad_zeros_without_separator(&result->value, format, zeros);
                                }
                            }
                        }
                    }
                    else if(quantity->exponent < digits_count)
                    {
                        int integer_digits = quantity->exponent;
                        int integers_so_far = 0;
                        int start_digit = 0;

                        if(integer_digits < format->min_integer_digits)
                        {
                            integer_digits = format->min_integer_digits;
                            int zeros = integer_digits - quantity->exponent;

                            for(int digit_index = 0;
                                    digit_index < zeros;
                                    digit_index += 1)
                            {
                                if(separate_group_at_location(format, integer_digits - digit_index - 1, integer_digits))
                                {
                                    aft_string_append(&result->value, group_separator);
                                }

                                aft_string_append(&result->value, &format->symbols.digits[0]);
                            }

                            integers_so_far += zeros;
                        }
                        else if(quantity->exponent > format->max_integer_digits)
                        {
                            start_digit = quantity->exponent - format->max_integer_digits;
                        }

                        for(int digit_index = start_digit;
                                digit_index < quantity->exponent;
                                digit_index += 1)
                        {
                            int location = digit_index + integers_so_far;

                            if(separate_group_at_location(format, integer_digits - location - 1, integer_digits))
                            {
                                aft_string_append(&result->value, group_separator);
                            }

                            int digit = digits_contents[digit_index];
                            aft_string_append(&result->value, &format->symbols.digits[digit]);
                        }

                        aft_string_append(&result->value, &format->symbols.radix_separator);

                        for(int digit_index = quantity->exponent;
                                digit_index < digits_count;
                                digit_index += 1)
                        {
                            int digit = digits_contents[digit_index];
                            aft_string_append(&result->value, &format->symbols.digits[digit]);
                        }

                        if(format->use_significant_digits)
                        {
                            if(digits_count < format->min_significant_digits)
                            {
                                int zeros = format->min_significant_digits - digits_count;
                                pad_zeros_without_separator(&result->value, format, zeros);
                            }
                        }
                        else
                        {
                            int fraction_digits = digits_count - quantity->exponent;
                            if(fraction_digits < format->min_fraction_digits)
                            {
                                int zeros = format->min_fraction_digits - fraction_digits;
                                pad_zeros_without_separator(&result->value, format, zeros);
                            }
                        }
                    }
                    else
                    {
                        int integer_digits = quantity->exponent;
                        int integers_so_far = 0;
                        int start_digit = 0;

                        if(integer_digits < format->min_integer_digits)
                        {
                            integer_digits = format->min_integer_digits;
                            int zeros = integer_digits - quantity->exponent;

                            for(int digit_index = 0;
                                    digit_index < zeros;
                                    digit_index += 1)
                            {
                                if(separate_group_at_location(format, integer_digits - digit_index - 1, integer_digits))
                                {
                                    aft_string_append(&result->value, group_separator);
                                }

                                aft_string_append(&result->value, &format->symbols.digits[0]);
                            }

                            integers_so_far += zeros;
                        }
                        else if(quantity->exponent > format->max_integer_digits)
                        {
                            start_digit = quantity->exponent - format->max_integer_digits;
                        }

                        for(int digit_index = start_digit;
                                digit_index < digits_count;
                                digit_index += 1)
                        {
                            int location = digit_index + integers_so_far;

                            if(separate_group_at_location(format, integer_digits - location - 1, integer_digits))
                            {
                                aft_string_append(&result->value, group_separator);
                            }

                            int digit = digits_contents[digit_index];
                            aft_string_append(&result->value, &format->symbols.digits[digit]);
                        }

                        for(int digit_index = digits_count;
                                digit_index < quantity->exponent;
                                digit_index += 1)
                        {
                            int location = digit_index + integers_so_far;

                            if(separate_group_at_location(format, integer_digits - location - 1, integer_digits))
                            {
                                aft_string_append(&result->value, group_separator);
                            }

                            aft_string_append(&result->value, &format->symbols.digits[0]);
                        }

                        if(format->use_significant_digits)
                        {
                            if(format->min_significant_digits > digits_count)
                            {
                                aft_string_append(&result->value, &format->symbols.radix_separator);

                                int zeros = format->min_significant_digits - digits_count;
                                pad_zeros_without_separator(&result->value, format, zeros);
                            }
                        }
                        else if(format->min_fraction_digits > 0)
                        {
                            aft_string_append(&result->value, &format->symbols.radix_separator);

                            pad_zeros_without_separator(&result->value, format, format->min_fraction_digits);
                        }
                    }
                    break;
                }
                case AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC:
                {
                    aft_string_append(&result->value, &format->symbols.digits[digits_contents[0]]);

                    if(digits_count > 1)
                    {
                        aft_string_append(&result->value, &format->symbols.radix_separator);

                        for(int digit_index = 1;
                                digit_index < digits_count;
                                digit_index += 1)
                        {
                            int digit = digits_contents[digit_index];
                            aft_string_append(&result->value, &format->symbols.digits[digit]);
                        }

                        aft_string_append(&result->value, &format->symbols.exponential_sign);

                        int exponent = quantity->exponent - 1;
                        if(exponent < 0)
                        {
                            aft_string_append(&result->value, &format->symbols.minus_sign);
                            exponent = -exponent;
                        }

                        AFT_ASSERT(count_digits(exponent) <= 3);
                        int exponent_digits[3];
                        int exponent_digit_index = 0;
                        do
                        {
                            exponent_digits[exponent_digit_index] = exponent % 10;
                            exponent_digit_index += 1;
                            exponent /= 10;
                        } while(exponent);

                        for(exponent_digit_index -= 1;
                                exponent_digit_index >= 0;
                                exponent_digit_index -= 1)
                        {
                            int digit = exponent_digits[exponent_digit_index];
                            aft_string_append(&result->value, &format->symbols.digits[digit]);
                        }
                    }
                    break;
                }
            }

            apply_suffix(&result->value, format, quantity->sign);
            break;
        }
    }
}

AftMaybeString aft_ascii_from_double(double value)
{
    return aft_ascii_from_double_with_allocator(value, NULL);
}

AftMaybeString aft_ascii_from_double_with_allocator(double value, void* allocator)
{
    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    char cheating[25];
    snprintf(cheating, 25, "%g", value);
    aft_string_append_c_string(&result.value, cheating);

    return result;
}

AftMaybeString aft_ascii_from_uint64(uint64_t value, const AftBaseFormat* format)
{
    return aft_ascii_from_uint64_with_allocator(value, format, NULL);
}

AftMaybeString aft_ascii_from_uint64_with_allocator(uint64_t value, const AftBaseFormat* format, void* allocator)
{
    AFT_ASSERT(allocator);
    AFT_ASSERT(format);
    AFT_ASSERT(format->base >= 2);
    AFT_ASSERT(format->base <= 36);

    const char* base36_digits = base36_digits_lowercase;
    if(format->use_uppercase)
    {
        base36_digits = base36_digits_uppercase;
    }

    char digits[AFT_UINT64_MAX_BINARY_DIGITS + 1];
    int digit_index = 0;
    do
    {
        digits[digit_index] = base36_digits[value % format->base];
        digit_index += 1;
        value /= format->base;
    } while(value && digit_index < AFT_UINT64_MAX_BINARY_DIGITS);

    digits[digit_index] = '\0';

    AftMaybeString result = aft_string_copy_c_string_with_allocator(digits, allocator);
    aft_ascii_reverse(&result.value);

    return result;
}

// Warning!
//
// This implementation does not produce very accurate results. A more robust
// algorithm would use large integers and assemble the binary representation of
// the double by parts. Repeatedly operating on a float-point intermediate value
// compounds rounding errors and reduces the accuracy of the result in this
// implementation.
//
// A good modern implementation might be something like Albert Chan's
// strtod-fast in this repository https://github.com/achan001/dtoa-fast. Or
// a simpler but less speedy traditional implementation using big integers
// as outlined here
// https://www.exploringbinary.com/correct-decimal-to-floating-point-using-big-integers/.
AftMaybeDouble aft_ascii_to_double(AftStringSlice slice)
{
    AFT_ASSERT(aft_ascii_check(slice));

    AftMaybeDouble result;
    result.valid = false;
    result.value = 0.0;

    bool sign = false;

    const char* contents = aft_string_slice_start(slice);
    int count = aft_string_slice_count(slice);

    int char_index = 0;

    if(char_index < count)
    {
        if(contents[char_index] == '-')
        {
            sign = true;
            char_index += 1;
        }
        else if(contents[char_index] == '+')
        {
            sign = false;
            char_index += 1;
        }
    }

    double value = 0.0;
    int mantissa_count = 0;
    int fraction_digits = 0;
    int exponent = 0;

    while(char_index < count && aft_ascii_is_numeric(contents[char_index]))
    {
        value = (10.0 * value) + (contents[char_index] - '0');
        char_index += 1;
        mantissa_count += 1;
    }

    if(char_index < count && contents[char_index] == '.')
    {
        char_index += 1;

        while(char_index < count && aft_ascii_is_numeric(contents[char_index]))
        {
            value = (10.0 * value) + (contents[char_index] - '0');
            char_index += 1;
            fraction_digits += 1;
            mantissa_count += 1;
        }

        exponent -= fraction_digits;
    }

    if(mantissa_count == 0)
    {
        return result;
    }

    if(char_index < count && (contents[char_index] == 'E' || contents[char_index] == 'e'))
    {
        char_index += 1;

        bool exponent_sign = false;

        if(char_index < count)
        {
            if(contents[char_index] == '-')
            {
                exponent_sign = true;
                char_index += 1;
            }
            else if(contents[char_index] == '+')
            {
                exponent_sign = false;
                char_index += 1;
            }
        }

        int explicit_exponent = 0;
        while(char_index < count && aft_ascii_is_numeric(contents[char_index]))
        {
            explicit_exponent = (10 * explicit_exponent) + (contents[char_index] - '0');
            char_index += 1;
        }

        if(exponent_sign)
        {
            exponent -= explicit_exponent;
        }
        else
        {
            exponent += explicit_exponent;
        }
    }

    if(exponent < -307 || exponent > 308)
    {
        return result;
    }

    double power_of_ten = 10.0;
    int exponent_magnitude = exponent;
    if(exponent_magnitude < 0)
    {
        exponent_magnitude = -exponent_magnitude;
    }

    while(exponent_magnitude)
    {
        if(exponent_magnitude & 1)
        {
            if(exponent < 0)
            {
                value /= power_of_ten;
            }
            else
            {
                value *= power_of_ten;
            }
        }

        exponent_magnitude >>= 1;
        power_of_ten *= power_of_ten;
    }

    if(sign)
    {
        value = -value;
    }

    result.valid = true;
    result.value = value;

    return result;
}

AftMaybeUint64 aft_ascii_to_uint64(AftStringSlice slice)
{
    AFT_ASSERT(aft_ascii_check(slice));

    AftMaybeUint64 result;
    result.valid = true;
    result.value = 0;

    const char* contents = aft_string_slice_start(slice);
    int count = aft_string_slice_count(slice);

    int power_index = AFT_UINT64_MAX_DECIMAL_DIGITS - count;

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        uint64_t character = contents[char_index];
        uint64_t digit = character - '0';
        if(digit >= 10)
        {
            result.valid = false;
            return result;
        }
        result.value += powers_of_ten[power_index] * digit;
    }

    return result;
}

bool aft_decimal_format_default(AftDecimalFormat* format)
{
    return aft_decimal_format_default_with_allocator(format, NULL);
}

bool aft_decimal_format_default_with_allocator(AftDecimalFormat* format, void* allocator)
{
    bool symbols_done = default_number_symbols(&format->symbols, allocator);
    if(!symbols_done)
    {
        return false;
    }

    AftMaybeString positive_pattern = aft_string_copy_c_string_with_allocator("+", allocator);
    if(!positive_pattern.valid)
    {
        return false;
    }
    format->positive_prefix_pattern = positive_pattern.value;

    AftMaybeString negative_pattern = aft_string_copy_c_string_with_allocator("-", allocator);
    if(!negative_pattern.valid)
    {
        return false;
    }
    format->negative_prefix_pattern = negative_pattern.value;

    aft_string_initialise_with_allocator(&format->negative_suffix_pattern, allocator);
    aft_string_initialise_with_allocator(&format->positive_suffix_pattern, allocator);

    format->max_fraction_digits = 3;
    format->max_integer_digits = 42;
    format->min_fraction_digits = 0;
    format->min_integer_digits = 1;
    format->rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN;
    format->style = AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT;
    format->rounding_increment_double = 0.0;
    format->rounding_increment_float = 0.0f;
    format->rounding_increment_int = 1;
    format->primary_grouping_size = 3;
    format->secondary_grouping_size = 3;
    format->use_explicit_plus_sign = false;
    format->use_grouping = false;
    format->use_significant_digits = false;

    return true;
}

bool aft_decimal_format_default_scientific(AftDecimalFormat* format)
{
    return aft_decimal_format_default_scientific_with_allocator(format, NULL);
}

bool aft_decimal_format_default_scientific_with_allocator(AftDecimalFormat* format, void* allocator)
{
    bool symbols_done = default_number_symbols(&format->symbols, allocator);
    if(!symbols_done)
    {
        return false;
    }

    AftMaybeString positive_pattern = aft_string_copy_c_string_with_allocator("+", allocator);
    if(!positive_pattern.valid)
    {
        return false;
    }
    format->positive_prefix_pattern = positive_pattern.value;

    AftMaybeString negative_pattern = aft_string_copy_c_string_with_allocator("-", allocator);
    if(!negative_pattern.valid)
    {
        return false;
    }
    format->negative_prefix_pattern = negative_pattern.value;

    aft_string_initialise_with_allocator(&format->negative_suffix_pattern, allocator);
    aft_string_initialise_with_allocator(&format->positive_suffix_pattern, allocator);

    format->max_fraction_digits = 3;
    format->max_integer_digits = 1;
    format->min_fraction_digits = 0;
    format->min_integer_digits = 1;
    format->rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN;
    format->style = AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC;
    format->rounding_increment_double = 0.0;
    format->rounding_increment_float = 0.0f;
    format->rounding_increment_int = 1;
    format->primary_grouping_size = 3;
    format->secondary_grouping_size = 3;
    format->use_explicit_plus_sign = false;
    format->use_grouping = false;
    format->use_significant_digits = false;

    return true;
}


void aft_decimal_format_destroy(AftDecimalFormat* format)
{
    destroy_number_symbols(&format->symbols);

    aft_string_destroy(&format->negative_prefix_pattern);
    aft_string_destroy(&format->negative_suffix_pattern);
    aft_string_destroy(&format->positive_prefix_pattern);
    aft_string_destroy(&format->positive_suffix_pattern);
}

bool aft_decimal_format_validate(const AftDecimalFormat* format)
{
    bool rounding_increments_valid =
            format->rounding_increment_double >= 0.0
            && format->rounding_increment_float >= 0.0f
            && format->rounding_increment_int > 0;

    bool digit_limits_valid;

    if(format->use_significant_digits)
    {
        digit_limits_valid = format->min_significant_digits <= format->max_significant_digits;
    }
    else
    {
        digit_limits_valid =
                format->min_integer_digits <= format->max_integer_digits
                && format->min_fraction_digits <= format->max_fraction_digits;
    }

    return rounding_increments_valid && digit_limits_valid;
}

AftMaybeString aft_string_from_double(double value, const AftDecimalFormat* format)
{
    return aft_string_from_double_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_double_with_allocator(double value, const AftDecimalFormat* format, void* allocator)
{
    AFT_ASSERT(aft_decimal_format_validate(format));

    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    if(format->style == AFT_DECIMAL_FORMAT_STYLE_PERCENT)
    {
        value *= (double) format->percent.multiplier;
    }

    FloatFormat float_format;
    float_format.rounding_mode = format->rounding_mode;

    if(format->use_significant_digits)
    {
        float_format.cutoff_mode = CUTOFF_MODE_SIGNIFICANT_DIGITS;
        float_format.max_significant_digits = format->max_significant_digits;
        float_format.min_significant_digits = format->min_significant_digits;
    }
    else
    {
        if(format->style == AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC)
        {
            float_format.cutoff_mode = CUTOFF_MODE_SIGNIFICANT_DIGITS;
            float_format.max_significant_digits = format->max_fraction_digits + 1;
            float_format.min_significant_digits = format->min_fraction_digits + 1;
        }
        else
        {
            float_format.cutoff_mode = CUTOFF_MODE_FRACTION_DIGITS;
            float_format.max_fraction_digits = format->max_fraction_digits;
            float_format.min_fraction_digits = format->min_fraction_digits;
        }
    }

    DecimalQuantity quantity = format_double(value, &float_format, allocator);
    format_float_result(&result, &quantity, format);

    aft_string_destroy(&quantity.digits);

    return result;
}

AftMaybeString aft_string_from_float(float value, const AftDecimalFormat* format)
{
    return aft_string_from_double_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_float_with_allocator(float value, const AftDecimalFormat* format, void* allocator)
{
    AFT_ASSERT(aft_decimal_format_validate(format));

    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    if(format->style == AFT_DECIMAL_FORMAT_STYLE_PERCENT)
    {
        value *= (float) format->percent.multiplier;
    }

    FloatFormat float_format;
    float_format.rounding_mode = format->rounding_mode;

    if(format->use_significant_digits)
    {
        float_format.cutoff_mode = CUTOFF_MODE_SIGNIFICANT_DIGITS;
        float_format.max_significant_digits = format->max_significant_digits;
        float_format.min_significant_digits = format->min_significant_digits;
    }
    else
    {
        if(format->style == AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC)
        {
            float_format.cutoff_mode = CUTOFF_MODE_SIGNIFICANT_DIGITS;
            float_format.max_significant_digits = format->max_fraction_digits + 1;
            float_format.min_significant_digits = format->min_fraction_digits + 1;
        }
        else
        {
            float_format.cutoff_mode = CUTOFF_MODE_FRACTION_DIGITS;
            float_format.max_fraction_digits = format->max_fraction_digits;
            float_format.min_fraction_digits = format->min_fraction_digits;
        }
    }

    DecimalQuantity quantity = format_float(value, &float_format, allocator);
    format_float_result(&result, &quantity, format);

    aft_string_destroy(&quantity.digits);

    return result;
}

AftMaybeString aft_string_from_int(int value, const AftDecimalFormat* format)
{
    return aft_string_from_int64(value, format);
}

AftMaybeString aft_string_from_int_with_allocator(int value, const AftDecimalFormat* format, void* allocator)
{
    return aft_string_from_int64_with_allocator(value, format, allocator);
}

AftMaybeString aft_string_from_int64(int64_t value, const AftDecimalFormat* format)
{
    return aft_string_from_int64_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_int64_with_allocator(int64_t value, const AftDecimalFormat* format, void* allocator)
{
    bool sign = value < 0;
    if(sign)
    {
        value = -value;
    }

    return string_from_uint64_and_sign((uint64_t) value, sign, format, allocator);
}

AftMaybeString aft_string_from_uint64(uint64_t value, const AftDecimalFormat* format)
{
    return aft_string_from_uint64_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_uint64_with_allocator(uint64_t value, const AftDecimalFormat* format, void* allocator)
{
    return string_from_uint64_and_sign(value, false, format, allocator);
}
