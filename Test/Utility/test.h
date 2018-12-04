#ifndef TEST_H_
#define TEST_H_

#include "random.h"

#include "../../Source/aft_string.h"

#include <assert.h>
#include <stdbool.h>


#define ASSERT(expression) \
    assert(expression)


typedef struct Allocator
{
    uint64_t bytes_used;
    bool force_allocation_failure;
} Allocator;

typedef struct Test Test;

typedef bool (*RunCall)(Test* test);

struct Test
{
    Allocator allocator;
    Allocator bad_allocator;
    RandomGenerator generator;
};

typedef struct TestSpec
{
    RunCall run;
    const char* name;
} TestSpec;

typedef struct Suite
{
    TestSpec* specs;
    bool* tests_failed;
    int test_count;
    int test_cap;
} Suite;


void add_test(Suite* suite, RunCall run, const char* name);
AftMaybeString make_random_string(RandomGenerator* generator,
        Allocator* allocator);
bool run_tests(Suite* suite);
int string_size(const char* string);
bool strings_match(const char* a, const char* b);

#endif // TEST_H_

