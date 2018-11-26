#include "ad_number_format.h"


#include <stddef.h>


static int count_digits_uint64(uint64_t value, int base)
{
    uint64_t divisor = (uint64_t) base;
    int digits = 0;
    while(value)
    {
        value /= divisor;
        digits += 1;
    }
    return digits;
}

static bool default_number_symbols(AdNumberSymbols* symbols, void* allocator)
{
    struct
    {
        const char* symbol;
        AdString* result;
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
        AdMaybeString result =
                ad_string_from_c_string_with_allocator(symbol, allocator);

        if(!result.valid)
        {
            for(int destroy_index = 0;
                    destroy_index < symbol_index;
                    destroy_index += 1)
            {
                ad_string_destroy(table[destroy_index].result);
            }

            return false;
        }

        *table[symbol_index].result = result.value;
    }

    return true;
}

static void destroy_number_symbols(AdNumberSymbols* symbols)
{
    for(int digit_index = 0; digit_index < 10; digit_index += 1)
    {
        ad_string_destroy(&symbols->digits[digit_index]);
    }

    ad_string_destroy(&symbols->currency_decimal_separator);
    ad_string_destroy(&symbols->currency_group_separator);
    ad_string_destroy(&symbols->radix_separator);
    ad_string_destroy(&symbols->exponential_sign);
    ad_string_destroy(&symbols->group_separator);
    ad_string_destroy(&symbols->infinity_sign);
    ad_string_destroy(&symbols->minus_sign);
    ad_string_destroy(&symbols->nan_sign);
    ad_string_destroy(&symbols->per_mille_sign);
    ad_string_destroy(&symbols->percent_sign);
    ad_string_destroy(&symbols->plus_sign);
    ad_string_destroy(&symbols->superscripting_exponent_sign);
}


bool ad_number_format_default(AdNumberFormat* format)
{
    return ad_number_format_default_with_allocator(format, NULL);
}

bool ad_number_format_default_with_allocator(AdNumberFormat* format,
        void* allocator)
{
    bool symbols_done = default_number_symbols(&format->symbols, allocator);
    if(!symbols_done)
    {
        return false;
    }

    AdMaybeString negative_pattern =
            ad_string_from_c_string_with_allocator("-#,##0.###", allocator);
    if(!negative_pattern.valid)
    {
        return false;
    }
    format->negative_pattern = negative_pattern.value;

    AdMaybeString positive_pattern =
            ad_string_from_c_string_with_allocator("#,##0.###", allocator);
    if(!positive_pattern.valid)
    {
        ad_string_destroy(&format->negative_pattern);
        return false;
    }
    format->positive_pattern = positive_pattern.value;

    format->max_fraction_digits = 3;
    format->max_integer_digits = 42;
    format->min_fraction_digits = 0;
    format->min_integer_digits = 1;
    format->rounding_mode = AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_EVEN;
    format->style = AD_NUMBER_FORMAT_STYLE_FIXED_POINT;
    format->rounding_increment_int = 1;
    format->base = 10;
    format->use_explicit_plus_sign = false;
    format->use_grouping = false;
    format->use_significant_digits = false;

    return true;
}


void ad_number_format_destroy(AdNumberFormat* format)
{
    destroy_number_symbols(&format->symbols);

    ad_string_destroy(&format->negative_pattern);
    ad_string_destroy(&format->positive_pattern);
}

AdMaybeString ad_string_from_uint64(uint64_t value,
        const AdNumberFormat* format)
{
    return ad_string_from_uint64_with_allocator(value, format, NULL);
}

AdMaybeString ad_string_from_uint64_with_allocator(uint64_t value,
        const AdNumberFormat* format, void* allocator)
{
    AdMaybeString result;
    result.valid = true;
    ad_string_initialise_with_allocator(&result.value, allocator);

    if(format->style == AD_NUMBER_FORMAT_STYLE_PERCENT)
    {
        value *= format->percent.multiplier;
    }

    const AdString* pattern = &format->positive_pattern;

    int total_digits = count_digits_uint64(value, format->base);

    return result;
}
