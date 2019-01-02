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
        .factored_exponent =
                (int32_t) binary32->exponent - INT32_C(127) - INT32_C(23),
        .has_unequal_margins =
                binary32->exponent != 1 && binary32->fraction == 0,
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
        .factored_exponent =
                (int32_t) binary64->exponent - INT32_C(1023) - INT32_C(52),
        .has_unequal_margins =
                binary64->exponent != 1 && binary64->fraction == 0,
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
    BigInt scaled_value;
    BigInt scaled_margin_low;
    BigInt* scaled_margin_high;
    BigInt optional_margin_high;

    if(parts->has_unequal_margins)
    {
        if(exponent > 0)
        {
            big_int_set_uint64(&scaled_value, 4 * parts->mantissa);
            big_int_shift_left(&scaled_value, exponent);
            big_int_set_uint32(&scale, 4);
            scaled_margin_low = big_int_pow2(exponent);
            optional_margin_high = big_int_pow2(exponent + 1);
        }
        else
        {
            big_int_set_uint64(&scaled_value, 4 * parts->mantissa);
            scale = big_int_pow2(-exponent + 2);
            big_int_set_uint32(&scaled_margin_low, 1);
            big_int_set_uint32(&optional_margin_high, 2);
        }

        scaled_margin_high = &optional_margin_high;
    }
    else
    {
        if(exponent > 0)
        {
            big_int_set_uint64(&scaled_value, 2 * parts->mantissa);
            big_int_shift_left(&scaled_value, exponent);
            big_int_set_uint32(&scale, 2);
            scaled_margin_low = big_int_pow2(exponent);
        }
        else
        {
            big_int_set_uint64(&scaled_value, 2 * parts->mantissa);
            scale = big_int_pow2(-exponent + 1);
            big_int_set_uint32(&scaled_margin_low, 1);
        }

        scaled_margin_high = &scaled_margin_low;
    }

    const double log10_2 = 0.30102999566398119521373889472449;
    int32_t estimated_exponent =
            (int32_t) ceil(log10_2 * parts->exponent - 0.69);

    if(format->cutoff_mode == CUTOFF_MODE_FRACTION_DIGITS
            && estimated_exponent <= -(int32_t) format->max_fraction_digits)
    {
        estimated_exponent = -(int32_t) format->max_fraction_digits + 1;
    }

    if(estimated_exponent > 0)
    {
        scale = big_int_multiply_by_pow10(&scale, estimated_exponent);
    }
    else if(estimated_exponent < 0)
    {
        BigInt pow10 = big_int_pow10(-estimated_exponent);
        scaled_value = big_int_multiply(&scaled_value, &pow10);
        scaled_margin_low = big_int_multiply(&scaled_margin_low, &pow10);

        if(scaled_margin_high != &scaled_margin_low)
        {
            *scaled_margin_high = big_int_multiply_by_2(&scaled_margin_low);
        }
    }

    if(big_int_compare(&scaled_value, &scale) >= 0)
    {
        estimated_exponent += 1;
    }
    else
    {
        big_int_decuple(&scaled_value);
        big_int_decuple(&scaled_margin_low);

        if(scaled_margin_high != &scaled_margin_low)
        {
            *scaled_margin_high = big_int_multiply_by_2(&scaled_margin_low);
        }
    }

    int32_t digit_exponent = estimated_exponent;
    int32_t cutoff_exponent;

    switch(format->cutoff_mode)
    {
        case CUTOFF_MODE_FRACTION_DIGITS:
        {
            cutoff_exponent = -(int32_t) format->max_fraction_digits;
            break;
        }
        case CUTOFF_MODE_SIGNIFICANT_DIGITS:
        {
            cutoff_exponent =
                    digit_exponent - (int32_t) format->max_significant_digits;
            break;
        }
    }

    result.exponent = digit_exponent;

    AFT_ASSERT(scale.blocks_count > 0);
    uint32_t high_block = scale.blocks[scale.blocks_count - 1];

    if(high_block < 8 || high_block > 429496729)
    {
        uint32_t high_block_log2 = log2_uint32(high_block);
        AFT_ASSERT(high_block_log2 < 3 || high_block_log2 > 27);
        uint32_t shift = (32 + 27 - high_block_log2) % 32;

        big_int_shift_left(&scale, shift);
        big_int_shift_left(&scaled_value, shift);
        big_int_shift_left(&scaled_margin_low, shift);

        if(scaled_margin_high != &scaled_margin_low)
        {
            *scaled_margin_high = big_int_multiply_by_2(&scaled_margin_low);
        }
    }

    uint32_t output_digit;
    bool low = false;
    bool high = false;

    for(;;)
    {
        digit_exponent -= 1;

        output_digit = big_int_divide_max_quotient_9(&scaled_value, &scale);
        AFT_ASSERT(output_digit < 10);

        if(big_int_is_zero(&scaled_value) || digit_exponent == cutoff_exponent)
        {
            break;
        }

        result.digits[result.digits_count] = output_digit;
        result.digits_count += 1;

        big_int_decuple(&scaled_value);
    }

    bool round_down = low;

    if(low == high)
    {
        big_int_double(&scaled_value);
        int32_t compare = big_int_compare(&scaled_value, &scale);
        round_down = compare < 0;

        if(compare == 0)
        {
            round_down = (output_digit & 1) == 0;
        }
    }

    if(round_down)
    {
        result.digits[result.digits_count] = output_digit;
        result.digits_count += 1;
    }
    else
    {
        if(output_digit == 9)
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
            result.digits[result.digits_count] = output_digit + 1;
            result.digits_count += 1;
        }
    }

    int min_digits;
    switch(format->cutoff_mode)
    {
        case CUTOFF_MODE_FRACTION_DIGITS:
        {
            int integer_digits = result.digits_count - result.exponent;
            min_digits = integer_digits + format->min_fraction_digits;
            break;
        }
        case CUTOFF_MODE_SIGNIFICANT_DIGITS:
        {
            min_digits = format->min_significant_digits;
            break;
        }
    }

    for(int digit_index = result.digits_count - 1;
            digit_index > min_digits;
            digit_index -= 1)
    {
        int digit = result.digits[digit_index];

        if(digit != 0)
        {
            break;
        }

        result.digits_count -= 1;
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
