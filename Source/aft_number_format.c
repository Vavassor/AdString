#include <AftString/aft_string.h>

#include "floating_point_format.h"

#include <assert.h>
#include <stddef.h>


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


static const char* base36_digits_uppercase =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char* base36_digits_lowercase =
        "0123456789abcdefghijklmnopqrstuvwxyz";


static uint64_t abs_int64_to_uint64(int64_t x)
{
    uint64_t mask = -((uint64_t) x >> (8 * sizeof(uint64_t) - 1));
    return ((uint64_t) x + mask) ^ mask;
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
        AftMaybeString result =
                aft_string_from_c_string_with_allocator(symbol, allocator);

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

static uint64_t round_half_up(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * ((value + (increment / 2)) / increment);
}

static uint64_t round_half_even(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    uint64_t even_increment = (1 & (value / increment)) ^ 1;
    return increment * ((value + increment / 2 - even_increment) / increment);
}

static uint64_t round_up(uint64_t value, uint64_t increment)
{
    AFT_ASSERT(increment > 0);

    return increment * ((value + (increment - 1)) / increment);
}

static uint64_t round_uint64_and_sign(uint64_t value, uint64_t increment,
        bool sign, AftDecimalFormatRoundingMode rounding_mode)
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

static bool separate_group_at_location(const AftDecimalFormat* format,
        int index, int length)
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

static void append_pattern(AftString* string, const AftString* pattern,
        const AftDecimalFormat* format)
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
                AftStringRange range = {prior_index, index};
                aft_string_append_range(string, pattern, &range);
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

static uint64_t apply_rounding(uint64_t value, bool sign,
        const AftDecimalFormat* format)
{
    uint64_t result = value;

    if(format->rounding_increment_int != 1)
    {
        result = round_uint64_and_sign(value, format->rounding_increment_int,
                sign, format->rounding_mode);
    }

    return result;
}

static void apply_prefix(AftString* string, const AftDecimalFormat* format,
        bool sign)
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

                if(separate_group_at_location(format, digit_index,
                        format->min_integer_digits))
                {
                    aft_string_append(formatter->string,
                            formatter->group_separator);
                }

                aft_string_append(formatter->string,
                        &format->symbols.digits[0]);
            }

            digit_total = format->min_integer_digits;
        }
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

        if(separate_group_at_location(formatter->format, digit_index,
                formatter->digit_total))
        {
            aft_string_append(formatter->string, formatter->group_separator);
        }

        int digit = formatter->digits[digit_index];
        aft_string_append(formatter->string,
                &formatter->format->symbols.digits[digit]);
    }

    formatter->digit_index = digit_index;
}

static void pad_zeros_without_separator(AftString* string,
        const AftDecimalFormat* format, int count)
{
    for(int zero_index = 0;
            zero_index < count;
            zero_index += 1)
    {
        aft_string_append(string, &format->symbols.digits[0]);
    }
}

static void format_remaining_integer_digits_and_fraction_digits(
        DecimalFormatter* formatter)
{
    const AftDecimalFormat* format = formatter->format;
    int digit_index = formatter->digit_index;

    if(format->use_significant_digits)
    {
        for(digit_index -= 1; digit_index >= 0; digit_index -= 1)
        {
            if(separate_group_at_location(format, digit_index,
                    formatter->digit_total))
            {
                aft_string_append(formatter->string,
                        formatter->group_separator);
            }

            aft_string_append(formatter->string, &format->symbols.digits[0]);
        }

        if(format->min_significant_digits > formatter->digit_total)
        {
            aft_string_append(formatter->string,
                    &format->symbols.radix_separator);
            pad_zeros_without_separator(formatter->string, format,
                    format->min_significant_digits - formatter->digit_total);
        }
    }
    else if(format->min_fraction_digits > 0)
    {
        aft_string_append(formatter->string, &format->symbols.radix_separator);
        pad_zeros_without_separator(formatter->string, format,
                format->min_fraction_digits);
    }

    formatter->digit_index = digit_index;
}

static void apply_suffix(AftString* string, const AftDecimalFormat* format,
        bool sign)
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

static AftMaybeString string_from_uint64_and_sign(uint64_t value, bool sign,
        const AftDecimalFormat* format, void* allocator)
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
    pad_leading_zeros_and_set_digit_limits(&formatter);
    format_significant_integer_digits(&formatter);
    format_remaining_integer_digits_and_fraction_digits(&formatter);
    apply_suffix(formatter.string, formatter.format, sign);

    return result;
}

static void format_float_result(AftMaybeString* result,
        const FloatResult* digits, const AftDecimalFormat* format)
{
    switch(digits->type)
    {
        case FLOAT_RESULT_TYPE_INFINITY:
        {
            apply_prefix(&result->value, format, digits->sign);
            aft_string_append(&result->value, &format->symbols.infinity_sign);
            apply_suffix(&result->value, format, digits->sign);
            break;
        }
        case FLOAT_RESULT_TYPE_NAN:
        {
            aft_string_assign(&result->value, &format->symbols.nan_sign);
            break;
        }
        case FLOAT_RESULT_TYPE_NORMAL:
        {
            apply_prefix(&result->value, format, digits->sign);

            switch(format->style)
            {
                case AFT_DECIMAL_FORMAT_STYLE_CURRENCY:
                case AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT:
                case AFT_DECIMAL_FORMAT_STYLE_PERCENT:
                {
                    if(digits->exponent <= 0)
                    {
                        aft_string_append(&result->value,
                                &format->symbols.digits[0]);
                        aft_string_append(&result->value,
                                &format->symbols.radix_separator);
                        pad_zeros_without_separator(&result->value, format,
                                -digits->exponent);

                        for(int digit_index = 0;
                                digit_index < digits->digits_count;
                                digit_index += 1)
                        {
                            int digit = digits->digits[digit_index];
                            aft_string_append(&result->value,
                                    &format->symbols.digits[digit]);
                        }
                    }
                    else if(digits->exponent < digits->digits_count)
                    {
                        for(int digit_index = 0;
                                digit_index < digits->exponent;
                                digit_index += 1)
                        {
                            int digit = digits->digits[digit_index];
                            aft_string_append(&result->value,
                                    &format->symbols.digits[digit]);
                        }

                        aft_string_append(&result->value,
                                &format->symbols.radix_separator);

                        for(int digit_index = digits->exponent;
                                digit_index < digits->digits_count;
                                digit_index += 1)
                        {
                            int digit = digits->digits[digit_index];
                            aft_string_append(&result->value,
                                    &format->symbols.digits[digit]);
                        }
                    }
                    else
                    {
                        for(int digit_index = 0;
                                digit_index < digits->digits_count;
                                digit_index += 1)
                        {
                            int digit = digits->digits[digit_index];
                            aft_string_append(&result->value,
                                    &format->symbols.digits[digit]);
                        }

                        pad_zeros_without_separator(&result->value, format,
                                digits->exponent - digits->digits_count);
                    }
                    break;
                }
                case AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC:
                {
                    aft_string_append(&result->value,
                            &format->symbols.digits[digits->digits[0]]);
                    aft_string_append(&result->value,
                            &format->symbols.radix_separator);

                    for(int digit_index = 1;
                            digit_index < digits->digits_count;
                            digit_index += 1)
                    {
                        int digit = digits->digits[digit_index];
                        aft_string_append(&result->value,
                                &format->symbols.digits[digit]);
                    }

                    aft_string_append(&result->value,
                            &format->symbols.exponential_sign);

                    int exponent = digits->exponent;
                    if(exponent < 0)
                    {
                        aft_string_append(&result->value,
                                &format->symbols.minus_sign);
                        exponent = -exponent;
                    }

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
                        aft_string_append(&result->value,
                                &format->symbols.digits[digit]);
                    }
                    break;
                }
            }

            apply_suffix(&result->value, format, digits->sign);
            break;
        }
    }
}


AftMaybeString aft_ascii_from_uint64(uint64_t value,
        const AftBaseFormat* format)
{
    return aft_ascii_from_uint64_with_allocator(value, format, NULL);
}

AftMaybeString aft_ascii_from_uint64_with_allocator(uint64_t value,
        const AftBaseFormat* format, void* allocator)
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

    AftMaybeString result =
            aft_string_from_c_string_with_allocator(digits, allocator);
    aft_ascii_reverse(&result.value);

    return result;
}

bool aft_decimal_format_default(AftDecimalFormat* format)
{
    return aft_decimal_format_default_with_allocator(format, NULL);
}

bool aft_decimal_format_default_with_allocator(AftDecimalFormat* format,
        void* allocator)
{
    bool symbols_done = default_number_symbols(&format->symbols, allocator);
    if(!symbols_done)
    {
        return false;
    }

    AftMaybeString positive_pattern =
            aft_string_from_c_string_with_allocator("+", allocator);
    if(!positive_pattern.valid)
    {
        return false;
    }
    format->positive_prefix_pattern = positive_pattern.value;

    AftMaybeString negative_pattern =
            aft_string_from_c_string_with_allocator("-", allocator);
    if(!negative_pattern.valid)
    {
        return false;
    }
    format->negative_prefix_pattern = negative_pattern.value;

    aft_string_initialise_with_allocator(&format->negative_suffix_pattern,
            allocator);
    aft_string_initialise_with_allocator(&format->positive_suffix_pattern,
            allocator);

    format->max_fraction_digits = 3;
    format->max_integer_digits = 42;
    format->min_fraction_digits = 0;
    format->min_integer_digits = 1;
    format->rounding_mode = AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN;
    format->style = AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT;
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
    if(format->use_significant_digits)
    {
        return format->min_significant_digits <= format->max_significant_digits;
    }
    else
    {
        return format->min_fraction_digits <= format->max_fraction_digits
                && format->min_integer_digits <= format->max_integer_digits;
    }
}

AftMaybeString aft_string_from_double(double value,
        const AftDecimalFormat* format)
{
    return aft_string_from_double_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_double_with_allocator(double value,
        const AftDecimalFormat* format, void* allocator)
{
    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    FloatFormat float_format;
    FloatResult digits = format_double(value, &float_format);

    format_float_result(&result, &digits, format);

    return result;
}

AftMaybeString aft_string_from_float(float value,
        const AftDecimalFormat* format)
{
    return aft_string_from_double_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_float_with_allocator(float value,
        const AftDecimalFormat* format, void* allocator)
{
    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    FloatFormat float_format;
    FloatResult digits = format_float(value, &float_format);

    format_float_result(&result, &digits, format);

    return result;
}

AftMaybeString aft_string_from_int(int value, const AftDecimalFormat* format)
{
    return aft_string_from_int64(value, format);
}

AftMaybeString aft_string_from_int_with_allocator(int value,
        const AftDecimalFormat* format, void* allocator)
{
    return aft_string_from_int64_with_allocator(value, format, allocator);
}

AftMaybeString aft_string_from_int64(int64_t value,
        const AftDecimalFormat* format)
{
    return aft_string_from_int64_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_int64_with_allocator(int64_t value,
        const AftDecimalFormat* format, void* allocator)
{
    return string_from_uint64_and_sign(abs_int64_to_uint64(value), value < 0,
            format, allocator);
}

AftMaybeString aft_string_from_uint64(uint64_t value,
        const AftDecimalFormat* format)
{
    return aft_string_from_uint64_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_uint64_with_allocator(uint64_t value,
        const AftDecimalFormat* format, void* allocator)
{
    return string_from_uint64_and_sign(value, false, format, allocator);
}
