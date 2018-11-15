#ifndef AD_NUMBER_FORMAT_H_
#define AD_NUMBER_FORMAT_H_

#include "ad_string.h"

#include <stdint.h>


typedef enum AdNumberFormatRoundingMode
{
    AD_NUMBER_FORMAT_ROUNDING_MODE_CEILING,
    AD_NUMBER_FORMAT_ROUNDING_MODE_DOWN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_FLOOR,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_DOWN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_EVEN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_UP,
    AD_NUMBER_FORMAT_ROUNDING_MODE_UP,
} AdNumberFormatRoundingMode;

typedef enum AdNumberFormatStyle
{
    AD_NUMBER_FORMAT_STYLE_CURRENCY,
    AD_NUMBER_FORMAT_STYLE_FIXED_POINT,
    AD_NUMBER_FORMAT_STYLE_PERCENT,
    AD_NUMBER_FORMAT_STYLE_SCIENTIFIC,
} AdNumberFormatStyle;


typedef struct AdNumberSymbols
{
    AdString digits[10];
    AdString currency_decimal_separator;
    AdString currency_group_separator;
    AdString exponential_sign;
    AdString group_separator;
    AdString infinity_sign;
    AdString minus_sign;
    AdString nan_sign;
    AdString per_mille_sign;
    AdString percent_sign;
    AdString plus_sign;
    AdString radix_separator;
    AdString superscripting_exponent_sign;
} AdNumberSymbols;

typedef struct AdNumberFormatCurrency
{
    AdString symbol;
} AdNumberFormatCurrency;

typedef struct AdNumberFormatPercent
{
    int multiplier;
} AdNumberFormatPercent;

typedef struct AdNumberFormat
{
    AdNumberSymbols symbols;
    AdString negative_pattern;
    AdString positive_pattern;

    union
    {
        AdNumberFormatCurrency currency;
        AdNumberFormatPercent percent;
    };

    union
    {
        struct
        {
            int max_fraction_digits;
            int max_integer_digits;
            int min_fraction_digits;
            int min_integer_digits;
        };

        struct
        {
            int max_significant_digits;
            int min_significant_digits;
        };
    };

    union
    {
        int rounding_increment_int;
        double rounding_increment_double;
    };

    AdNumberFormatRoundingMode rounding_mode;
    AdNumberFormatStyle style;
    int base;
    bool use_explicit_plus_sign;
    bool use_grouping;
    bool use_significant_digits;
} AdNumberFormat;


bool ad_number_format_default(AdNumberFormat* format);
bool ad_number_format_default_with_allocator(AdNumberFormat* format,
        void* allocator);
void ad_number_format_destroy(AdNumberFormat* format);

AdMaybeString ad_string_from_uint64(uint64_t value,
        const AdNumberFormat* format);
AdMaybeString ad_string_from_uint64_with_allocator(uint64_t value,
        const AdNumberFormat* format, void* allocator);

#endif // AD_NUMBER_FORMAT_H_
