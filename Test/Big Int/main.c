#include "../../Source/big_int.h"
#include "../Utility/test.h"

static BigInt big_int_from_c_string(const char* string)
{
    BigInt result;
    big_int_set_uint32(&result, 0);

    int bytes = 0;
    for(int char_index = 0; string[char_index]; char_index += 1)
    {
        bytes += 1;
    }

    for(int char_index = 0; char_index < bytes; char_index += 1)
    {
        BigInt digit;
        big_int_set_uint32(&digit, string[char_index] - '0');
        BigInt product = big_int_multiply_by_pow10(&digit, bytes - 1 - char_index);
        result = big_int_add(&result, &product);
    }

    return result;
}

static bool test_add(Test* test)
{
    const BigInt a = big_int_from_c_string("590223487857837587888821111122222");
    const BigInt b = big_int_from_c_string("112444555266666666677777775542221");
    const BigInt answer = big_int_from_c_string("702668043124504254566598886664443");

    BigInt sum = big_int_add(&a, &b);

    return big_int_compare(&sum, &answer) == 0;
}

static bool test_compare(Test* test)
{
    const BigInt a = big_int_from_c_string("112444555266666666677777775542220");
    const BigInt b = big_int_from_c_string("112444555266666666677777775542221");
    const BigInt c = big_int_from_c_string("112444555266666666677777775542225");

    return big_int_compare(&a, &a) == 0
            && big_int_compare(&b, &b) == 0
            && big_int_compare(&c, &c) == 0
            && big_int_compare(&a, &b) < 0
            && big_int_compare(&b, &c) < 0
            && big_int_compare(&a, &c) < 0
            && big_int_compare(&c, &b) > 0
            && big_int_compare(&b, &a) > 0
            && big_int_compare(&c, &a) > 0;
}

static bool test_decuple(Test* test)
{
    BigInt a = big_int_from_c_string("23423512512512412323332323232");
    const BigInt answer = big_int_from_c_string("234235125125124123233323232320");

    big_int_decuple(&a);

    return big_int_compare(&a, &answer) == 0;
}

static bool test_double(Test* test)
{
    BigInt a = big_int_from_c_string("23542352111117009");
    const BigInt answer = big_int_from_c_string("47084704222234018");

    big_int_double(&a);

    return big_int_compare(&a, &answer) == 0;
}

static bool test_multiply(Test* test)
{
    const BigInt a = {{0x3ac76ec1, 0x3d3f5867, 0x4f4}, 3};
    const BigInt b = {{0xe0e3331, 0xd48bd7f, 0x15}, 3};
    const BigInt answer = {{0x97caa5f1, 0x2b950a2d, 0xb5c43b67, 0xd5aa786d, 0x684a}, 5};

    BigInt product = big_int_multiply(&a, &b);

    return big_int_compare(&product, &answer) == 0;
}

static bool test_multiply_by_pow10(Test* test)
{
    const BigInt a = big_int_from_c_string("23542352111117009235462177");
    const BigInt answer = big_int_from_c_string("235423521111170092354621770000000000");

    BigInt product = big_int_multiply_by_pow10(&a, 10);

    BigInt multiplicand = big_int_pow10(10);
    BigInt equivalent_product = big_int_multiply(&a, &multiplicand);

    return big_int_compare(&product, &answer) == 0
            && big_int_compare(&product, &equivalent_product) == 0;
}

static bool test_multiply_by_2(Test* test)
{
    const BigInt a = big_int_from_c_string("23542352111117009235462177");
    const BigInt answer = big_int_from_c_string("47084704222234018470924354");

    BigInt product = big_int_multiply_by_2(&a);

    return big_int_compare(&product, &answer) == 0;
}

static bool test_multiply_uint32(Test* test)
{
    const BigInt a = big_int_from_c_string("23542352111117009235462177");
    const BigInt answer = big_int_from_c_string("3612974151436794056338673917659");

    BigInt product = big_int_multiply_uint32(&a, 153467);

    return big_int_compare(&product, &answer) == 0;
}

static bool test_pow2(Test* test)
{
    const BigInt answer = big_int_from_c_string("10633823966279326983230456482242756608");

    BigInt power = big_int_pow2(123);

    return big_int_compare(&power, &answer) == 0;
}

static bool test_pow10(Test* test)
{
    const BigInt answer = big_int_from_c_string("1000000000000000000000000000000");

    BigInt power = big_int_pow10(30);

    return big_int_compare(&power, &answer) == 0;
}

static bool test_shift_left(Test* test)
{
    BigInt a = big_int_from_c_string("239532523146662462532532623644");
    const BigInt answer = big_int_from_c_string("30660162962772795204164175826432");

    big_int_shift_left(&a, 7);

    return big_int_compare(&a, &answer) == 0;
}

int main(int argc, const char** argv)
{
    Suite suite = {0};

    add_test(&suite, test_add, "Test Add");
    add_test(&suite, test_compare, "Test Compare");
    add_test(&suite, test_decuple, "Test Decuple");
    add_test(&suite, test_double, "Test Double");
    add_test(&suite, test_multiply, "Test Multiply");
    add_test(&suite, test_multiply_by_pow10, "Test Multiply By Power Of 10");
    add_test(&suite, test_multiply_by_2, "Test Multiply By 2");
    add_test(&suite, test_multiply_uint32, "Test Multiply By uint32_t");
    add_test(&suite, test_pow2, "Test Power Of 2");
    add_test(&suite, test_pow10, "Test Power Of 10");
    add_test(&suite, test_shift_left, "Test Shift Left");

    bool success = run_tests(&suite);
    return !success;
}
