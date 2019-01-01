#include "floating_point_format.h"

#include "big_int.h"

#include <assert.h>
#include <math.h>
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
    int32_t factored_exponent;
    bool has_unequal_margins;
    bool sign;
} FloatParts;


static uint32_t log2_uint32(uint32_t x)
{
    static const uint8_t log_table[256] =
    {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };

    uint32_t temp;

    temp = x >> 24;
    if(temp)
    {
        return 24 + log_table[temp];
    }

    temp = x >> 16;
    if(temp)
    {
        return 16 + log_table[temp];
    }

    temp = x >> 8;
    if(temp)
    {
        return 8 + log_table[temp];
    }

    return log_table[x];
}

static FloatParts unpack_binary32(const Ieee754Binary32OrFloat* binary32)
{
    FloatParts result =
    {
        .mantissa = UINT64_C(0x800000) | binary32->fraction,
        .mantissa_high_bit = UINT32_C(23),
        .exponent = (int32_t) binary32->exponent - INT32_C(128),
        .factored_exponent = (int32_t) binary32->exponent - INT32_C(127) - INT32_C(23),
        .has_unequal_margins = binary32->exponent != 1 && binary32->fraction == 0,
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
        .factored_exponent = (int32_t) binary64->exponent - INT32_C(1023) - INT32_C(52),
        .has_unequal_margins = binary64->exponent != 1 && binary64->fraction == 0,
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
    result.type = FLOAT_RESULT_TYPE_NORMAL;
    result.digits_count = 0;

    if(parts->mantissa == UINT64_C(0))
    {
        result.digits[0] = 0;
        result.digits_count = 1;
        result.exponent = 0;
        return result;
    }

    result.sign = parts->sign;

    int32_t exponent = parts->factored_exponent;

    BigInt scale;
    BigInt scaledValue;
    BigInt scaledMarginLow;

    BigInt* pScaledMarginHigh;
    BigInt optionalMarginHigh;

    if(parts->has_unequal_margins)
    {
        if(exponent > 0)
        {
            big_int_set_uint64(&scaledValue, 4 * parts->mantissa);
            big_int_shift_left(&scaledValue, exponent);
            big_int_set_uint32(&scale, 4);
            scaledMarginLow = big_int_pow2(exponent);
            optionalMarginHigh = big_int_pow2(exponent + 1);
        }
        else
        {
            big_int_set_uint64(&scaledValue, 4 * parts->mantissa);
            scale = big_int_pow2(-exponent + 2);
            big_int_set_uint32(&scaledMarginLow, 1);
            big_int_set_uint32(&optionalMarginHigh, 2);
        }

        pScaledMarginHigh = &optionalMarginHigh;
    }
    else
    {
        if(exponent > 0)
        {
            big_int_set_uint64(&scaledValue, 2 * parts->mantissa);
            big_int_shift_left(&scaledValue, exponent);
            big_int_set_uint32(&scale, 2);
            scaledMarginLow = big_int_pow2(exponent);
        }
        else
        {
            big_int_set_uint64(&scaledValue, 2 * parts->mantissa);
            scale = big_int_pow2(-exponent + 1);
            big_int_set_uint32(&scaledMarginLow, 1);
        }

        pScaledMarginHigh = &scaledMarginLow;
    }

    const double log10_2 = 0.30102999566398119521373889472449;
    int32_t digitExponent = (int32_t) ceil(log10_2 * parts->exponent - 0.69);

    if(format->cutoff_mode == CUTOFF_MODE_FRACTION_DIGITS
            && digitExponent <= -(int32_t) format->max_fraction_digits)
    {
        digitExponent = -(int32_t) format->max_fraction_digits + 1;
    }

    if(digitExponent > 0)
    {
        scale = big_int_multiply_by_pow10(&scale, digitExponent);
    }
    else if(digitExponent < 0)
    {
        BigInt pow10 = big_int_pow10(-digitExponent);
        scaledValue = big_int_multiply(&scaledValue, &pow10);
        scaledMarginLow = big_int_multiply(&scaledMarginLow, &pow10);

        if(pScaledMarginHigh != &scaledMarginLow)
        {
            *pScaledMarginHigh = big_int_multiply_by_2(&scaledMarginLow);
        }
    }

    if(big_int_compare(&scaledValue, &scale) >= 0)
    {
        digitExponent += 1;
    }
    else
    {
        big_int_decuple(&scaledValue);
        big_int_decuple(&scaledMarginLow);
        if(pScaledMarginHigh != &scaledMarginLow)
        {
            *pScaledMarginHigh = big_int_multiply_by_2(&scaledMarginLow);
        }
    }

    int32_t cutoffExponent;

    switch(format->cutoff_mode)
    {
        case CUTOFF_MODE_FRACTION_DIGITS:
        {
            cutoffExponent = -(int32_t) format->max_fraction_digits;
            break;
        }
        case CUTOFF_MODE_SIGNIFICANT_DIGITS:
        {
            cutoffExponent =
                    digitExponent - (int32_t) format->max_significant_digits;
            break;
        }
    }

    result.exponent = digitExponent;

    AFT_ASSERT(scale.blocks_count > 0);
    uint32_t hiBlock = scale.blocks[scale.blocks_count - 1];
    if(hiBlock < 8 || hiBlock > 429496729)
    {
        uint32_t hiBlockLog2 = log2_uint32(hiBlock);
        AFT_ASSERT(hiBlockLog2 < 3 || hiBlockLog2 > 27);
        uint32_t shift = (32 + 27 - hiBlockLog2) % 32;

        big_int_shift_left(&scale, shift);
        big_int_shift_left(&scaledValue, shift);
        big_int_shift_left(&scaledMarginLow, shift);
        if(pScaledMarginHigh != &scaledMarginLow)
        {
            *pScaledMarginHigh = big_int_multiply_by_2(&scaledMarginLow);
        }
    }

    uint32_t outputDigit;
    bool low = false;
    bool high = false;

    for(;;)
    {
        digitExponent -= 1;

        outputDigit = big_int_divide_max_quotient_9(&scaledValue, &scale);
        AFT_ASSERT(outputDigit < 10);

        if(big_int_is_zero(&scaledValue) || digitExponent == cutoffExponent)
        {
            break;
        }

        result.digits[result.digits_count] = outputDigit;
        result.digits_count += 1;

        big_int_decuple(&scaledValue);
    }

    bool roundDown = low;

    if(low == high)
    {
        big_int_double(&scaledValue);
        int32_t compare = big_int_compare(&scaledValue, &scale);
        roundDown = compare < 0;

        if(compare == 0)
        {
            roundDown = (outputDigit & 1) == 0;
        }
    }

    if(roundDown)
    {
        result.digits[result.digits_count] = outputDigit;
        result.digits_count += 1;
    }
    else
    {
        if(outputDigit == 9)
        {
            for(;;)
            {
                if(result.digits_count == 0)
                {
                    result.digits[result.digits_count] = 1;
                    result.digits_count += 1;
                    result.exponent += 1;
                    break;
                }

                result.digits_count -= 1;

                if(result.digits[result.digits_count] != 9)
                {
                    result.digits[result.digits_count] += 1;
                    result.digits_count += 1;
                    break;
                }
            }
        }
        else
        {
            result.digits[result.digits_count] = outputDigit + 1;
            result.digits_count += 1;
        }
    }

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
