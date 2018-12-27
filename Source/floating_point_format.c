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

typedef struct UnpackedFloat
{
    uint64_t mantissa;
    uint32_t mantissa_high_bit;
    int32_t exponent;
    bool sign;
} UnpackedFloat;


static UnpackedFloat unpack_binary32(const Ieee754Binary32OrFloat* binary32)
{
    UnpackedFloat result =
    {
        .mantissa = UINT64_C(0x800000) | binary32->fraction,
        .mantissa_high_bit = UINT32_C(23),
        .exponent = (int32_t) binary32->exponent - INT32_C(128),
        .sign = binary32->sign,
    };

    return result;
}

static UnpackedFloat unpack_binary64(const Ieee754Binary64OrDouble* binary64)
{
    UnpackedFloat result =
    {
        .mantissa = UINT64_C(0x10000000000000) | binary64->fraction,
        .mantissa_high_bit = UINT32_C(52),
        .exponent = (int32_t) binary64->exponent - INT32_C(1024),
        .sign = binary64->sign,
    };

    return result;
}

static FloatResult handle_nan_or_infinity(const UnpackedFloat* value)
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

static FloatResult dragon4(const UnpackedFloat* value,
        const FloatFormat* format)
{

}


FloatResult format_double(double value, const FloatFormat* format)
{
    Ieee754Binary64OrDouble binary64 = {.value = value};
    UnpackedFloat unpacked = unpack_binary64(&binary64);

    if(unpacked.exponent == INT32_C(1023))
    {
        return handle_nan_or_infinity(&unpacked);
    }
    else
    {
        return dragon4(&unpacked, format);
    }
}

FloatResult format_float(float value, const FloatFormat* format)
{
    Ieee754Binary32OrFloat binary32 = {.value = value};
    UnpackedFloat unpacked = unpack_binary32(&binary32);

    if(unpacked.exponent == INT32_C(127))
    {
        return handle_nan_or_infinity(&unpacked);
    }
    else
    {
        return dragon4(&unpacked, format);
    }
}
