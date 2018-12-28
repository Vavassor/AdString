#include "floating_point_format.h"

#include "big_int.h"

#include <assert.h>
#include <stdint.h>


#define AFT_ASSERT(expression) \
    assert(expression)


typedef union Ieee754Binary32OrFloat
{
    float value;

    struct
    {
        uint32_t fraction: 23;
        uint32_t exponent: 8;
        uint32_t sign: 1;
    };
} Ieee754Binary32OrFloat;

typedef union Ieee754Binary64OrDouble
{
    double value;

    struct
    {
        uint64_t fraction: 52;
        uint64_t exponent: 11;
        uint64_t sign: 1;
    };
} Ieee754Binary64OrDouble;

typedef struct FloatParts
{
    uint64_t mantissa;
    uint32_t mantissa_high_bit;
    int32_t exponent;
    bool sign;
} FloatParts;


static FloatParts unpack_binary32(const Ieee754Binary32OrFloat* binary32)
{
    FloatParts result =
    {
        .mantissa = UINT64_C(0x800000) | binary32->fraction,
        .mantissa_high_bit = UINT32_C(23),
        .exponent = (int32_t) binary32->exponent - INT32_C(128),
        .sign = binary32->sign,
    };

    return result;
}

static FloatParts unpack_binary64(const Ieee754Binary64OrDouble* binary64)
{
    FloatParts result =
    {
        .mantissa = UINT64_C(0x10000000000000) | binary64->fraction,
        .mantissa_high_bit = UINT32_C(52),
        .exponent = (int32_t) binary64->exponent - INT32_C(1024),
        .sign = binary64->sign,
    };

    return result;
}

static FloatResult handle_nan_or_infinity(const FloatParts* value)
{
    FloatResult result;

    if((((UINT64_C(1) << value->mantissa_high_bit) - 1) & value->mantissa) == 0)
    {
        result.type = FLOAT_RESULT_TYPE_INFINITY;
        result.sign = value->sign;
    }
    else
    {
        result.type = FLOAT_RESULT_TYPE_NAN;
    }

    return result;
}

static FloatResult dragon4(const FloatParts* parts,
        const FloatFormat* format)
{
    FloatResult result;

    result.digits[0] = 7;
    result.digits[1] = 8;
    result.digits[2] = 9;
    result.digits[3] = 0;
    result.digits[4] = 4;
    result.digits_count = 5;
    result.exponent = -12;
    result.sign = false;
    result.type = FLOAT_RESULT_TYPE_NORMAL;

    return result;
}


FloatResult format_double(double value, const FloatFormat* format)
{
    Ieee754Binary64OrDouble binary64 = {.value = value};
    FloatParts parts = unpack_binary64(&binary64);

    if(parts.exponent == INT32_C(1023))
    {
        return handle_nan_or_infinity(&parts);
    }
    else
    {
        return dragon4(&parts, format);
    }
}

FloatResult format_float(float value, const FloatFormat* format)
{
    Ieee754Binary32OrFloat binary32 = {.value = value};
    FloatParts parts = unpack_binary32(&binary32);

    if(parts.exponent == INT32_C(127))
    {
        return handle_nan_or_infinity(&parts);
    }
    else
    {
        return dragon4(&parts, format);
    }
}
