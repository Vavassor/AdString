#ifndef FLOATING_POINT_FORMAT_H_
#define FLOATING_POINT_FORMAT_H_

#include <stdbool.h>


typedef enum FloatResultType
{
    FLOAT_RESULT_TYPE_NORMAL,
    FLOAT_RESULT_TYPE_INFINITY,
    FLOAT_RESULT_TYPE_NAN,
} FloatResultType;


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
    int placeholder;
} FloatFormat;


FloatResult format_double(double value, const FloatFormat* format);
FloatResult format_float(float value, const FloatFormat* format);

#endif // FLOATING_POINT_FORMAT_H_
