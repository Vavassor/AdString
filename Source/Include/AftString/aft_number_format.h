#ifndef AD_NUMBER_FORMAT_H_
#define AD_NUMBER_FORMAT_H_

#include <AftString/aft_string.h>

#include <stdint.h>


typedef enum AftDecimalFormatRoundingMode
{
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_CEILING,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_DOWN,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_FLOOR,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_DOWN,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_UP,
    AFT_DECIMAL_FORMAT_ROUNDING_MODE_UP,
} AftDecimalFormatRoundingMode;

typedef enum AftDecimalFormatStyle
{
    AFT_DECIMAL_FORMAT_STYLE_CURRENCY,
    AFT_DECIMAL_FORMAT_STYLE_FIXED_POINT,
    AFT_DECIMAL_FORMAT_STYLE_PERCENT,
    AFT_DECIMAL_FORMAT_STYLE_SCIENTIFIC,
} AftDecimalFormatStyle;


typedef struct AftBaseFormat
{
    int base;
    bool use_uppercase;
} AftBaseFormat;

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

typedef struct AftDecimalFormatCurrency
{
    AftString symbol;
} AftDecimalFormatCurrency;

typedef struct AftDecimalFormatPercent
{
    int multiplier;
} AftDecimalFormatPercent;

typedef struct AftDecimalFormat
{
    AftNumberSymbols symbols;
    AftString negative_prefix_pattern;
    AftString negative_suffix_pattern;
    AftString positive_prefix_pattern;
    AftString positive_suffix_pattern;

    union
    {
        AftDecimalFormatCurrency currency;
        AftDecimalFormatPercent percent;
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

    double rounding_increment_double;
    float rounding_increment_float;
    int rounding_increment_int;

    AftDecimalFormatRoundingMode rounding_mode;
    AftDecimalFormatStyle style;
    int primary_grouping_size;
    int secondary_grouping_size;
    bool use_explicit_plus_sign;
    bool use_grouping;
    bool use_significant_digits;
} AftDecimalFormat;


AftMaybeString aft_ascii_from_double(double value);
AftMaybeString aft_ascii_from_double_with_allocator(double value, void* allocator);
AftMaybeString aft_ascii_from_uint64(uint64_t value, const AftBaseFormat* format);
AftMaybeString aft_ascii_from_uint64_with_allocator(uint64_t value, const AftBaseFormat* format, void* allocator);
AftMaybeDouble aft_ascii_to_double(const AftStringSlice* slice);
AftMaybeUint64 aft_ascii_uint64_from_string(const AftString* string);
AftMaybeUint64 aft_ascii_uint64_from_string_range(const AftString* string, const AftStringRange* range);

bool aft_decimal_format_default(AftDecimalFormat* format);
bool aft_decimal_format_default_with_allocator(AftDecimalFormat* format, void* allocator);
bool aft_decimal_format_default_scientific(AftDecimalFormat* format);
bool aft_decimal_format_default_scientific_with_allocator(AftDecimalFormat* format, void* allocator);
void aft_decimal_format_destroy(AftDecimalFormat* format);
bool aft_decimal_format_validate(const AftDecimalFormat* format);

AftMaybeString aft_string_from_double(double value, const AftDecimalFormat* format);
AftMaybeString aft_string_from_double_with_allocator(double value, const AftDecimalFormat* format, void* allocator);
AftMaybeString aft_string_from_float(float value, const AftDecimalFormat* format);
AftMaybeString aft_string_from_float_with_allocator(float value, const AftDecimalFormat* format, void* allocator);
AftMaybeString aft_string_from_int(int value, const AftDecimalFormat* format);
AftMaybeString aft_string_from_int_with_allocator(int value, const AftDecimalFormat* format, void* allocator);
AftMaybeString aft_string_from_int64(int64_t value, const AftDecimalFormat* format);
AftMaybeString aft_string_from_int64_with_allocator(int64_t value, const AftDecimalFormat* format, void* allocator);
AftMaybeString aft_string_from_uint64(uint64_t value, const AftDecimalFormat* format);
AftMaybeString aft_string_from_uint64_with_allocator(uint64_t value, const AftDecimalFormat* format, void* allocator);


#endif // AD_NUMBER_FORMAT_H_
