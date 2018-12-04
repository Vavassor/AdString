#ifndef AD_NUMBER_FORMAT_H_
#define AD_NUMBER_FORMAT_H_

#include <stdint.h>
#include "aft_string.h"


typedef enum AftNumberFormatRoundingMode
{
    AD_NUMBER_FORMAT_ROUNDING_MODE_CEILING,
    AD_NUMBER_FORMAT_ROUNDING_MODE_DOWN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_FLOOR,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_DOWN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_EVEN,
    AD_NUMBER_FORMAT_ROUNDING_MODE_HALF_UP,
    AD_NUMBER_FORMAT_ROUNDING_MODE_UP,
} AftNumberFormatRoundingMode;

typedef enum AftNumberFormatStyle
{
    AD_NUMBER_FORMAT_STYLE_CURRENCY,
    AD_NUMBER_FORMAT_STYLE_FIXED_POINT,
    AD_NUMBER_FORMAT_STYLE_PERCENT,
    AD_NUMBER_FORMAT_STYLE_SCIENTIFIC,
} AftNumberFormatStyle;


typedef struct AftNumberSymbols
{
    AftString digits[10];
    AftString currency_decimal_separator;
    AftString currency_group_separator;
    AftString exponential_sign;
    AftString group_separator;
    AftString infinity_sign;
    AftString minus_sign;
    AftString nan_sign;
    AftString per_mille_sign;
    AftString percent_sign;
    AftString plus_sign;
    AftString radix_separator;
    AftString superscripting_exponent_sign;
} AftNumberSymbols;

typedef struct AftNumberFormatCurrency
{
    AftString symbol;
} AftNumberFormatCurrency;

typedef struct AftNumberFormatPercent
{
    int multiplier;
} AftNumberFormatPercent;

typedef struct AftNumberFormat
{
    AftNumberSymbols symbols;
    AftString negative_pattern;
    AftString positive_pattern;

    union
    {
        AftNumberFormatCurrency currency;
        AftNumberFormatPercent percent;
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

    AftNumberFormatRoundingMode rounding_mode;
    AftNumberFormatStyle style;
    int base;
    bool use_explicit_plus_sign;
    bool use_grouping;
    bool use_significant_digits;
} AftNumberFormat;


bool aft_number_format_default(AftNumberFormat* format);
bool aft_number_format_default_with_allocator(AftNumberFormat* format,
        void* allocator);
void aft_number_format_destroy(AftNumberFormat* format);

AftMaybeString aft_string_from_uint64(uint64_t value,
        const AftNumberFormat* format);
AftMaybeString aft_string_from_uint64_with_allocator(uint64_t value,
        const AftNumberFormat* format, void* allocator);

#endif // AD_NUMBER_FORMAT_H_