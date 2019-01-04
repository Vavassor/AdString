#ifndef FLOATING_POINT_FORMAT_H_
#define FLOATING_POINT_FORMAT_H_

#include <AftString/aft_string.h>

#include <stdbool.h>


typedef enum FloatResultType
{
    FLOAT_RESULT_TYPE_NORMAL,
    FLOAT_RESULT_TYPE_INFINITY,
    FLOAT_RESULT_TYPE_NAN,
} FloatResultType;

typedef enum CutoffMode
{
    CUTOFF_MODE_SIGNIFICANT_DIGITS,
    CUTOFF_MODE_FRACTION_DIGITS,
} CutoffMode;


typedef struct FloatResult
{
    AftString digits;
    int exponent;
    FloatResultType type;
    bool sign;
} FloatResult;

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
} FloatFormat;

FloatResult format_double(double value, const FloatFormat* format, void* allocator);
FloatResult format_float(float value, const FloatFormat* format, void* allocator);

#endif // FLOATING_POINT_FORMAT_H_
