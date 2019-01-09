#ifndef FLOATING_POINT_FORMAT_H_
#define FLOATING_POINT_FORMAT_H_

#include <AftString/aft_number_format.h>
#include <AftString/aft_string.h>

#include <stdbool.h>


typedef enum DecimalQuantityType
{
    DECIMAL_QUANTITY_TYPE_NORMAL,
    DECIMAL_QUANTITY_TYPE_INFINITY,
    DECIMAL_QUANTITY_TYPE_NAN,
} DecimalQuantityType;

typedef enum CutoffMode
{
    CUTOFF_MODE_SIGNIFICANT_DIGITS,
    CUTOFF_MODE_FRACTION_DIGITS,
} CutoffMode;


typedef struct DecimalQuantity
{
    AftString digits;
    int exponent;
    DecimalQuantityType type;
    bool sign;
} DecimalQuantity;

typedef struct FloatFormat
{
    union
    {
        struct
        {
            int min_fraction_digits;
            int max_fraction_digits;
        };
        struct
        {
            int min_significant_digits;
            int max_significant_digits;
        };
    };
    CutoffMode cutoff_mode;
    AftDecimalFormatRoundingMode rounding_mode;
} FloatFormat;

DecimalQuantity format_double(double value, const FloatFormat* format, void* allocator);
DecimalQuantity format_float(float value, const FloatFormat* format, void* allocator);

#endif // FLOATING_POINT_FORMAT_H_
