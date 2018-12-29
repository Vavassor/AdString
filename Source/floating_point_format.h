#ifndef FLOATING_POINT_FORMAT_H_
#define FLOATING_POINT_FORMAT_H_

#include <stdbool.h>


typedef enum FloatResultType
{
    FLOAT_RESULT_TYPE_NORMAL,
    FLOAT_RESULT_TYPE_INFINITY,
    FLOAT_RESULT_TYPE_NAN,
} FloatResultType;

typedef enum CutoffMode
{
    CUTOFF_MODE_NONE,
    CUTOFF_MODE_SIGNIFICANT_DIGITS,
    CUTOFF_MODE_FRACTION_DIGITS,
} CutoffMode;


typedef struct FloatResult
{
    int digits[32];
    int digits_count;
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
            int max_fraction_digits;
        };
        struct
        {
            int max_significant_digits;
        };
    };
    CutoffMode cutoff_mode;
} FloatFormat;


FloatResult format_double(double value, const FloatFormat* format);
FloatResult format_float(float value, const FloatFormat* format);

#endif // FLOATING_POINT_FORMAT_H_
