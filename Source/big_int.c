#include "big_int.h"

#include <assert.h>


#define AFT_ASSERT(expression) \
    assert(expression)


static const uint32_t powers_of_ten_uint32[] =
{
    UINT32_C(1),
    UINT32_C(10),
    UINT32_C(100),
    UINT32_C(1000),
    UINT32_C(10000),
    UINT32_C(100000),
    UINT32_C(1000000),
    UINT32_C(10000000),
};

static const BigInt powers_of_ten_big_int[] =
{
    (BigInt){{UINT32_C(100000000)}, 1},
    (BigInt){{UINT32_C(0x6fc10000), UINT32_C(0x002386f2)}, 2},
    (BigInt){{UINT32_C(0x00000000), UINT32_C(0x85acef81), UINT32_C(0x2d6d415b),
            UINT32_C(0x000004ee)}, 4},
    (BigInt){{UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0xbf6a1f01),
            UINT32_C(0x6e38ed64), UINT32_C(0xdaa797ed), UINT32_C(0xe93ff9f4),
            UINT32_C(0x00184f03)}, 7},
    (BigInt){{UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x2e953e01), UINT32_C(0x03df9909),
            UINT32_C(0x0f1538fd), UINT32_C(0x2374e42f), UINT32_C(0xd3cff5ec),
            UINT32_C(0xc404dc08), UINT32_C(0xbccdb0da), UINT32_C(0xa6337f19),
            UINT32_C(0xe91f2603), UINT32_C(0x0000024e)}, 14},
    (BigInt){{UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x00000000),
            UINT32_C(0x00000000), UINT32_C(0x00000000), UINT32_C(0x982e7c01),
            UINT32_C(0xbed3875b), UINT32_C(0xd8d99f72), UINT32_C(0x12152f87),
            UINT32_C(0x6bde50c6), UINT32_C(0xcf4a6e70), UINT32_C(0xd595d80f),
            UINT32_C(0x26b2716e), UINT32_C(0xadc666b0), UINT32_C(0x1d153624),
            UINT32_C(0x3c42d35a), UINT32_C(0x63ff540e), UINT32_C(0xcc5573c0),
            UINT32_C(0x65f9ef17), UINT32_C(0x55bc28f2), UINT32_C(0x80dcc7f7),
            UINT32_C(0xf46eeddc), UINT32_C(0x5fdcefce), UINT32_C(0x000553f7)},
            27},
};

static void zero_memory(void* memory, uint64_t bytes)
{
    for(uint8_t* p = memory; bytes; bytes -= 1, p += 1)
    {
        *p = 0;
    }
}

BigInt big_int_add(const BigInt* a, const BigInt* b)
{
    BigInt result;

    const BigInt* longer;
    const BigInt* shorter;
    if(a->blocks_count < b->blocks_count)
    {
        shorter = a;
        longer = b;
    }
    else
    {
        shorter = b;
        longer = a;
    }

    const int longer_count = longer->blocks_count;
    const int shorter_count = shorter->blocks_count;

    uint64_t carry = 0;
    int block_index = 0;

    for(; block_index < shorter_count; block_index += 1)
    {
        uint64_t sum =
                (uint64_t) longer->blocks[block_index]
                + (uint64_t) shorter->blocks[block_index] + carry;
        carry = sum >> 32;
        result.blocks[block_index] = sum & 0xffffffff;
    }

    for(; block_index != longer_count; block_index += 1)
    {
        uint64_t sum = (uint64_t) longer->blocks[block_index] + carry;
        carry = sum >> 32;
        result.blocks[block_index] = sum & 0xffffffff;
    }

    if(carry != 0)
    {
        AFT_ASSERT(carry == 1);
        AFT_ASSERT(longer_count < BIG_INT_BLOCK_CAP);
        AFT_ASSERT(longer_count == block_index);
        result.blocks[block_index] = 1;
        result.blocks_count = longer_count + 1;
    }
    else
    {
        result.blocks_count = longer_count;
    }

    return result;
}

int big_int_compare(const BigInt* a, const BigInt* b)
{
    int count_difference = a->blocks_count - b->blocks_count;
    if(count_difference != 0)
    {
        return count_difference;
    }

    for(int block_index = a->blocks_count - 1;
            block_index >= 0;
            block_index -= 1)
    {
        if(a->blocks[block_index] < b->blocks[block_index])
        {
            return -1;
        }
        else if(a->blocks[block_index] > b->blocks[block_index])
        {
            return 1;
        }
    }

    return 0;
}

void big_int_decuple(BigInt* a)
{
    uint64_t carry = 0;

    int block_index = 0;

    for(; block_index < a->blocks_count; block_index += 1)
    {
        uint64_t product =
                UINT64_C(10) * (uint64_t) a->blocks[block_index] + carry;
        a->blocks[block_index] = (uint32_t) (product & 0xffffffff);
        carry = product >> 32;
    }

    if(carry != 0)
    {
        AFT_ASSERT(a->blocks_count + 1 <= BIG_INT_BLOCK_CAP);
        a->blocks[block_index] = carry;
        a->blocks_count += 1;
    }
}

uint32_t big_int_divide_max_quotient_9(BigInt* dividend, const BigInt* divisor)
{
    AFT_ASSERT(!big_int_is_zero(divisor)
            && divisor->blocks[divisor->blocks_count - 1] >= 8
            && divisor->blocks[divisor->blocks_count - 1] < 0xffffffff
            && dividend->blocks_count <= divisor->blocks_count);

    if(dividend->blocks_count < divisor->blocks_count)
    {
        return 0;
    }

    uint32_t blocks_count = divisor->blocks_count;
    int final_divisor_block = blocks_count - 1;
    int final_dividend_block = blocks_count - 1;

    uint32_t quotient = dividend->blocks[final_dividend_block]
             / (divisor->blocks[final_divisor_block] + 1);
    AFT_ASSERT(quotient <= 9);

    if(quotient != 0)
    {
        int dividend_index = 0;
        int divisor_index = 0;

        uint64_t borrow = 0;
        uint64_t carry = 0;
        do
        {
            uint64_t product = (uint64_t) divisor->blocks[divisor_index]
                    * (uint64_t) quotient + carry;
            carry = product >> 32;

            uint64_t difference = (uint64_t) dividend->blocks[dividend_index]
                    - (product & 0xffffffff) - borrow;
            borrow = (difference >> 32) & 1;

            dividend->blocks[dividend_index] = difference & 0xffffffff;

            divisor_index += 1;
            dividend_index += 1;
        } while(divisor_index <= final_divisor_block);

        while(blocks_count > 0 && dividend->blocks[blocks_count - 1] == 0)
        {
            blocks_count -= 1;
        }

        dividend->blocks_count = blocks_count;
    }

    if(big_int_compare(dividend, divisor) >= 0)
    {
        ++quotient;

        int dividend_index = 0;
        int divisor_index = 0;

        uint64_t borrow = 0;
        do
        {
            uint64_t difference = (uint64_t) dividend->blocks[dividend_index]
                    - (uint64_t) divisor->blocks[divisor_index] - borrow;
            borrow = (difference >> 32) & 1;

            dividend->blocks[dividend_index] = difference & 0xffffffff;

            divisor_index += 1;
            dividend_index += 1;
        } while(divisor_index <= final_divisor_block);

        while(blocks_count > 0 && dividend->blocks[blocks_count - 1] == 0)
        {
            blocks_count -= 1;
        }

        dividend->blocks_count = blocks_count;
    }

    return quotient;
}

void big_int_double(BigInt* a)
{
    uint32_t carry = 0;
    int block_index = 0;

    for(; block_index < a->blocks_count; block_index += 1)
    {
        uint32_t block = a->blocks[block_index];
        a->blocks[block_index] = block << 1 | carry;
        carry = block >> 31;
    }

    if(carry != 0)
    {
        AFT_ASSERT(a->blocks_count + 1 <= BIG_INT_BLOCK_CAP);
        a->blocks[block_index] = carry;
        a->blocks_count += 1;
    }
}

bool big_int_is_zero(const BigInt* a)
{
    return a->blocks_count == 0;
}

BigInt big_int_multiply(const BigInt* a, const BigInt* b)
{
    BigInt result;

    const BigInt* longer;
    const BigInt* shorter;
    if(a->blocks_count < b->blocks_count)
    {
        shorter = a;
        longer = b;
    }
    else
    {
        shorter = b;
        longer = a;
    }

    const int longer_count = longer->blocks_count;
    const int shorter_count = shorter->blocks_count;

    int max_result_count = longer_count + shorter_count;
    AFT_ASSERT(max_result_count <= BIG_INT_BLOCK_CAP);

    zero_memory(result.blocks, sizeof(*result.blocks) * max_result_count);

    for(int shorter_index = 0;
            shorter_index < shorter_count;
            shorter_index += 1)
    {
        const uint64_t multiplier = shorter->blocks[shorter_index];

        if(multiplier != UINT64_C(0))
        {
            int longer_index = 0;
            int result_index = shorter_index;
            uint64_t carry = 0;
            do
            {
                uint64_t product =
                        result.blocks[result_index]
                               + multiplier * longer->blocks[longer_index]
                               + carry;
                carry = product >> 32;
                result.blocks[result_index] = product & 0xffffffff;
                longer_index += 1;
                result_index += 1;
            } while(longer_index < longer_count);

            AFT_ASSERT(result_index < max_result_count);
            result.blocks[result_index] = (uint32_t) (carry & 0xffffffff);
        }
    }

    if(max_result_count > 0 && result.blocks[max_result_count - 1] == 0)
    {
        result.blocks_count = max_result_count - 1;
    }
    else
    {
        result.blocks_count = max_result_count;
    }

    return result;
}

BigInt big_int_multiply_by_pow10(const BigInt* a, uint32_t exponent)
{
    AFT_ASSERT(exponent < 512);

    BigInt temps[2];
    BigInt* current_temp = &temps[0];
    BigInt* next_temp = &temps[1];

    uint32_t small_exponent = exponent & 0x7;
    if(small_exponent != 0)
    {
        *current_temp =
                big_int_multiply_uint32(a,
                        powers_of_ten_uint32[small_exponent]);
    }
    else
    {
        *current_temp = *a;
    }

    exponent >>= 3;

    for(int table_index = 0; exponent != 0; table_index += 1, exponent >>= 1)
    {
        if(exponent & 1)
        {
            *next_temp =
                    big_int_multiply(current_temp,
                            &powers_of_ten_big_int[table_index]);

            BigInt* swap = current_temp;
            current_temp = next_temp;
            next_temp = swap;
        }
    }

    return *current_temp;
}

BigInt big_int_multiply_by_2(const BigInt* a)
{
    BigInt result;

    uint32_t carry = 0;
    int block_index = 0;
    for(; block_index < a->blocks_count; block_index += 1)
    {
        uint32_t block = a->blocks[block_index];
        result.blocks[block_index] = (block << 1) | carry;
        carry = block >> 31;
    }

    if(carry != 0)
    {
        AFT_ASSERT(a->blocks_count + 1 <= BIG_INT_BLOCK_CAP);
        result.blocks[block_index] = (uint32_t) carry;
        result.blocks_count = a->blocks_count + 1;
    }
    else
    {
        result.blocks_count = a->blocks_count;
    }

    return result;
}

BigInt big_int_multiply_uint32(const BigInt* a, uint32_t b)
{
    BigInt result;

    uint32_t carry = 0;
    int block_index = 0;
    for(; block_index < a->blocks_count; block_index += 1)
    {
        uint64_t product = (uint64_t) a->blocks[block_index] * b + carry;
        result.blocks[block_index] = (uint32_t) (product & 0xffffffff);
        carry = product >> 32;
    }

    if(carry != 0)
    {
        AFT_ASSERT(a->blocks_count + 1 <= BIG_INT_BLOCK_CAP);
        result.blocks[block_index] = (uint32_t) carry;
        result.blocks_count = a->blocks_count + 1;
    }
    else
    {
        result.blocks_count = a->blocks_count;
    }

    return result;
}

BigInt big_int_pow10(uint32_t exponent)
{
    AFT_ASSERT(exponent < 512);

    BigInt temps[2];
    BigInt* current_temp = &temps[0];
    BigInt* next_temp = &temps[1];

    uint32_t small_exponent = exponent & 0x7;
    big_int_set_uint32(current_temp, powers_of_ten_uint32[small_exponent]);

    exponent >>= 3;

    for(uint32_t table_index = 0;
            exponent != 0;
            table_index += 1, exponent >>= 1)
    {
        if(exponent & 1)
        {
            *next_temp =
                    big_int_multiply(current_temp,
                            &powers_of_ten_big_int[table_index]);

            BigInt* swap = current_temp;
            current_temp = next_temp;
            next_temp = swap;
        }
    }

    return *current_temp;
}

BigInt big_int_pow2(uint32_t exponent)
{
    BigInt result;

    uint32_t block_index = exponent / 32;
    AFT_ASSERT(block_index < BIG_INT_BLOCK_CAP);

    zero_memory(result.blocks, sizeof(*result.blocks) * (block_index + 1));

    uint32_t bit_index = exponent % 32;
    result.blocks[block_index] |= 1 << bit_index;
    result.blocks_count = block_index + 1;

    return result;
}

void big_int_set_uint32(BigInt* a, uint32_t value)
{
    if(value == 0)
    {
        a->blocks_count = 0;
    }
    else
    {
        a->blocks[0] = value;
        a->blocks_count = 1;
    }
}

void big_int_set_uint64(BigInt* a, uint64_t value)
{
    if(value > UINT64_C(0xffffffff))
    {
        a->blocks[0] = value & 0xffffffff;
        a->blocks[1] = (value >> 32) & 0xffffffff;
        a->blocks_count = 2;
    }
    else if(value != UINT64_C(0))
    {
        a->blocks[0] = value & 0xffffffff;
        a->blocks_count = 1;
    }
    else
    {
        a->blocks_count = 0;
    }
}

void big_int_shift_left(BigInt* a, uint32_t shift)
{
    AFT_ASSERT(shift != 0);

    uint32_t shift_blocks = shift / 32;
    uint32_t shift_bits = shift % 32;

    AFT_ASSERT(a->blocks_count + shift_blocks <= BIG_INT_BLOCK_CAP);

    if(shift_bits == 0)
    {
        int from_index = a->blocks_count - 1;
        for(int to_index = from_index + shift_blocks;
                from_index >= 0;
                from_index -= 1, to_index -= 1)
        {
            a->blocks[to_index] = a->blocks[from_index];
        }

        zero_memory(a->blocks, sizeof(*a->blocks) * shift_blocks);
        a->blocks_count += shift_blocks;
    }
    else
    {
        int from_index  = a->blocks_count - 1;
        int to_index = a->blocks_count + shift_blocks;

        AFT_ASSERT(to_index < BIG_INT_BLOCK_CAP);
        a->blocks_count = to_index + 1;

        const uint32_t low_bits_shift = 32 - shift_bits;
        uint32_t high_bits = 0;
        uint32_t block = a->blocks[from_index];
        uint32_t low_bits = block >> low_bits_shift;

        while(from_index > 0)
        {
            a->blocks[to_index] = high_bits | low_bits;
            high_bits = block << shift_bits;

            from_index -= 1;
            to_index -= 1;

            block = a->blocks[from_index];
            low_bits = block >> low_bits_shift;
        }

        AFT_ASSERT(to_index == shift_blocks + 1);
        a->blocks[to_index] = high_bits | low_bits;
        a->blocks[to_index - 1] = block << shift_bits;

        zero_memory(a->blocks, sizeof(*a->blocks) * shift_blocks);

        if(a->blocks[a->blocks_count - 1] == 0)
        {
            a->blocks_count -= 1;
        }
    }
}
