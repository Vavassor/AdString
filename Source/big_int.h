#ifndef BIG_INT_H_
#define BIG_INT_H_

#include <stdbool.h>
#include <stdint.h>

#define BIG_INT_BLOCK_CAP 35

typedef struct BigInt
{
    uint32_t blocks[BIG_INT_BLOCK_CAP];
    int blocks_count;
} BigInt;

BigInt big_int_add(const BigInt* a, const BigInt* b);
int big_int_compare(const BigInt* a, const BigInt* b);
void big_int_decuple(BigInt* a);
uint32_t big_int_divide_max_quotient_9(BigInt* dividend, const BigInt* divisor);
void big_int_double(BigInt* a);
bool big_int_is_zero(const BigInt* a);
BigInt big_int_multiply(const BigInt* a, const BigInt* b);
BigInt big_int_multiply_by_pow10(const BigInt* a, uint32_t exponent);
BigInt big_int_multiply_by_2(const BigInt* a);
BigInt big_int_multiply_uint32(const BigInt* a, uint32_t b);
BigInt big_int_pow10(uint32_t exponent);
BigInt big_int_pow2(uint32_t exponent);
void big_int_set_uint32(BigInt* a, uint32_t value);
void big_int_set_uint64(BigInt* a, uint64_t value);
void big_int_shift_left(BigInt* a, uint32_t shift);

#endif // BIG_INT_H_
