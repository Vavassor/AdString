#include <AftString/aft_string.h>

#include <stddef.h>


#define AFT_UINT64_MAX_DIGITS 20


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


bool aft_number_format_default(AftNumberFormat* format)
{
    return aft_number_format_default_with_allocator(format, NULL);
}

bool aft_number_format_default_with_allocator(AftNumberFormat* format,
        void* allocator)
{
    bool symbols_done = default_number_symbols(&format->symbols, allocator);
    if(!symbols_done)
    {
        return false;
    }

    AftMaybeString negative_pattern =
            aft_string_from_c_string_with_allocator("-#,##0.###", allocator);
    if(!negative_pattern.valid)
    {
        return false;
    }
    format->negative_pattern = negative_pattern.value;

    AftMaybeString positive_pattern =
            aft_string_from_c_string_with_allocator("#,##0.###", allocator);
    if(!positive_pattern.valid)
    {
        aft_string_destroy(&format->negative_pattern);
        return false;
    }
    format->positive_pattern = positive_pattern.value;

    format->max_fraction_digits = 3;
    format->max_integer_digits = 42;
    format->min_fraction_digits = 0;
    format->min_integer_digits = 1;
    format->rounding_mode = AFT_NUMBER_FORMAT_ROUNDING_MODE_HALF_EVEN;
    format->style = AFT_NUMBER_FORMAT_STYLE_FIXED_POINT;
    format->rounding_increment_int = 1;
    format->base = 10;
    format->use_explicit_plus_sign = false;
    format->use_grouping = false;
    format->use_significant_digits = false;

    return true;
}


void aft_number_format_destroy(AftNumberFormat* format)
{
    destroy_number_symbols(&format->symbols);

    aft_string_destroy(&format->negative_pattern);
    aft_string_destroy(&format->positive_pattern);
}

AftMaybeString aft_string_from_uint64(uint64_t value,
        const AftNumberFormat* format)
{
    return aft_string_from_uint64_with_allocator(value, format, NULL);
}

AftMaybeString aft_string_from_uint64_with_allocator(uint64_t value,
        const AftNumberFormat* format, void* allocator)
{
    AftMaybeString result;
    result.valid = true;
    aft_string_initialise_with_allocator(&result.value, allocator);

    if(format->style == AFT_NUMBER_FORMAT_STYLE_PERCENT)
    {
        value *= format->percent.multiplier;
    }

    const AftString* pattern = &format->positive_pattern;

    uint64_t digits[AFT_UINT64_MAX_DIGITS];
    int digit_index = 0;
    do
    {
        digits[digit_index] = value % 10;
        digit_index += 1;
        value /= 10;
    } while(value && digit_index < AFT_UINT64_MAX_DIGITS);

    for(digit_index -= 1; digit_index >= 0; digit_index -= 1)
    {
        aft_string_append(&result.value,
                &format->symbols.digits[digits[digit_index]]);
    }

    return result;
}
