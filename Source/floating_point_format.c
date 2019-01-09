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
    if(binary32->exponent != 0)
    {
        FloatParts result =
        {
            .mantissa = UINT64_C(0x800000) | binary32->fraction,
            .mantissa_high_bit = 23,
            .exponent = (int32_t) binary32->exponent - 127,
            .factored_exponent = (int32_t) binary32->exponent - 127 - 23,
            .has_unequal_margins = binary32->exponent != 1 && binary32->fraction == 0,
            .sign = binary32->sign,
        };

        return result;
    }
    else
    {
        FloatParts result =
        {
            .mantissa = binary32->fraction,
            .mantissa_high_bit = log2_uint32(binary32->fraction),
            .exponent = 1 - 127,
            .factored_exponent = 1 - 127 - 23,
            .has_unequal_margins = false,
            .sign = binary32->sign,
        };

        return result;
    }
}

static FloatParts unpack_binary64(const Ieee754Binary64OrDouble* binary64)
{
    if(binary64->exponent != 0)
    {
        FloatParts result =
        {
            .mantissa = UINT64_C(0x10000000000000) | binary64->fraction,
            .mantissa_high_bit = 52,
            .exponent = (int32_t) binary64->exponent - 1023,
            .factored_exponent = (int32_t) binary64->exponent - 1023 - 52,
            .has_unequal_margins = binary64->exponent != 1 && binary64->fraction == 0,
            .sign = binary64->sign,
        };

        return result;
    }
    else
    {
        FloatParts result =
        {
            .mantissa = binary64->fraction,
            .mantissa_high_bit = log2_uint32(binary64->fraction),
            .exponent = 1 - 1023,
            .factored_exponent = 1 - 1023 - 52,
            .has_unequal_margins = false,
            .sign = binary64->sign,
        };

        return result;
    }
}

static DecimalQuantity handle_nan_or_infinity(const FloatParts* value, void* allocator)
{
    DecimalQuantity result;
    aft_string_initialise_with_allocator(&result.digits, allocator);

    if((((UINT64_C(1) << value->mantissa_high_bit) - 1) & value->mantissa) == 0)
    {
        result.type = DECIMAL_QUANTITY_TYPE_INFINITY;
        result.sign = value->sign;
    }
    else
    {
        result.type = DECIMAL_QUANTITY_TYPE_NAN;
    }

    return result;
}

static DecimalQuantity dragon4(const FloatParts* parts, const FloatFormat* format, void* allocator)
{
    DecimalQuantity result;
    aft_string_initialise_with_allocator(&result.digits, allocator);
    result.type = DECIMAL_QUANTITY_TYPE_NORMAL;

    if(parts->mantissa == 0)
    {
        aft_string_append_char(&result.digits, 0);
        result.exponent = 0;
        result.sign = false;
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
    int32_t estimated_exponent = (int32_t) ceil(log10_2 * parts->exponent - 0.69);

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
            cutoff_exponent = digit_exponent - (int32_t) format->max_significant_digits;
            break;
        }
    }

    result.exponent = digit_exponent;

    AFT_ASSERT(scale.blocks_count > 0);
    uint32_t high_block = scale.blocks[scale.blocks_count - 1];

    if(high_block < 8 || high_block > 0x19999999)
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

    for(;;)
    {
        digit_exponent -= 1;

        output_digit = big_int_divide_max_quotient_9(&scaled_value, &scale);
        AFT_ASSERT(output_digit < 10);

        if(big_int_is_zero(&scaled_value) || digit_exponent == cutoff_exponent)
        {
            break;
        }

        aft_string_append_char(&result.digits, output_digit);

        big_int_decuple(&scaled_value);
    }

    bool round_down = false;

    switch(format->rounding_mode)
    {
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_CEILING:
        {
            round_down = big_int_is_zero(&scaled_value) || result.sign;
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_DOWN:
        {
            round_down = true;
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_FLOOR:
        {
            round_down = big_int_is_zero(&scaled_value) || !result.sign;
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_DOWN:
        {
            big_int_double(&scaled_value);
            int32_t compare = big_int_compare(&scaled_value, &scale);
            round_down = compare < 0;

            if(compare == 0)
            {
                round_down = true;
            }
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_EVEN:
        {
            big_int_double(&scaled_value);
            int32_t compare = big_int_compare(&scaled_value, &scale);
            round_down = compare < 0;

            if(compare == 0)
            {
                round_down = (output_digit & 1) == 0;
            }
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_HALF_UP:
        {
            big_int_double(&scaled_value);
            int32_t compare = big_int_compare(&scaled_value, &scale);
            round_down = compare < 0;

            if(compare == 0)
            {
                round_down = false;
            }
            break;
        }
        case AFT_DECIMAL_FORMAT_ROUNDING_MODE_UP:
        {
            round_down = big_int_is_zero(&scaled_value);
            break;
        }
    }

    if(round_down)
    {
        aft_string_append_char(&result.digits, output_digit);
    }
    else
    {
        if(output_digit == 9)
        {
            char* contents = aft_string_get_contents(&result.digits);

            for(int digit_index = aft_string_get_count(&result.digits);;)
            {
                if(digit_index == 0)
                {
                    aft_string_append_char(&result.digits, 1);
                    result.exponent += 1;
                    break;
                }

                digit_index -= 1;

                if(contents[digit_index] != 9)
                {
                    contents[digit_index] += 1;
                    break;
                }

                AftStringRange range = {digit_index, digit_index + 1};
                aft_string_remove(&result.digits, &range);
            }
        }
        else
        {
            aft_string_append_char(&result.digits, output_digit + 1);
        }
    }

    int min_digits;
    switch(format->cutoff_mode)
    {
        case CUTOFF_MODE_FRACTION_DIGITS:
        {
            int integer_digits = aft_string_get_count(&result.digits) - result.exponent;
            min_digits = integer_digits + format->min_fraction_digits;
            break;
        }
        case CUTOFF_MODE_SIGNIFICANT_DIGITS:
        {
            min_digits = format->min_significant_digits;
            break;
        }
    }

    int digits_count = aft_string_get_count(&result.digits);
    const char* digits_contents = aft_string_get_contents_const(&result.digits);

    for(int digit_index = digits_count - 1;
            digit_index >= min_digits;
            digit_index -= 1)
    {
        int digit = digits_contents[digit_index];

        if(digit != 0)
        {
            break;
        }

        AftStringRange range = {digit_index, digit_index + 1};
        aft_string_remove(&result.digits, &range);
    }

    if(aft_string_get_count(&result.digits) == 1 && digits_contents[0] == 0)
    {
        result.sign = false;
    }

    return result;
}


DecimalQuantity format_double(double value, const FloatFormat* format, void* allocator)
{
    Ieee754Binary64OrDouble binary64 = {.value = value};
    FloatParts parts = unpack_binary64(&binary64);

    if(parts.exponent == 1024)
    {
        return handle_nan_or_infinity(&parts, allocator);
    }
    else
    {
        return dragon4(&parts, format, allocator);
    }
}

DecimalQuantity format_float(float value, const FloatFormat* format, void* allocator)
{
    Ieee754Binary32OrFloat binary32 = {.value = value};
    FloatParts parts = unpack_binary32(&binary32);

    if(parts.exponent == 128)
    {
        return handle_nan_or_infinity(&parts, allocator);
    }
    else
    {
        return dragon4(&parts, format, allocator);
    }
}
