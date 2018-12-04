#include "test.h"

#include <stdlib.h>
#include <string.h>

void add_test(Suite* suite, RunCall run, const char* name)
{
    if(suite->test_count + 1 >= suite->test_cap)
    {
        int prior_cap = suite->test_cap;
        int cap = (suite->test_cap) ? suite->test_cap * 2 : 16;

        TestSpec* specs = calloc(cap, sizeof(TestSpec));
        bool* tests_failed = calloc(cap, sizeof(bool));

        if(prior_cap)
        {
            memcpy(specs, suite->specs, sizeof(TestSpec) * prior_cap);
            memcpy(tests_failed, suite->tests_failed, sizeof(bool) * prior_cap);
            free(suite->specs);
            free(suite->tests_failed);
        }

        suite->specs = specs;
        suite->tests_failed = tests_failed;
        suite->test_cap = cap;
    }

    int index = suite->test_count;
    suite->specs[index].name = name;
    suite->specs[index].run = run;
    suite->test_count += 1;
}

int string_size(const char* string)
{
    if(string)
    {
        const char* s;
        for(s = string; *s; s += 1);
        return (int) (s - string);
    }

    return 0;
}

bool strings_match(const char* a, const char* b)
{
    while(*a && (*a == *b))
    {
        a += 1;
        b += 1;
    }
    return *a == *b;
}
