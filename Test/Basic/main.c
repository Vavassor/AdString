#include "../../Source/ad_string.h"

#include <stdio.h>

int main(int argc, const char** argv)
{
    AdString test = ad_string_from_c_string("What if").value;

    ad_string_append_c_string(&test, " we were all");
    ad_string_append_c_string(&test, " dancing?");

    printf("%s\n", ad_string_as_c_string(&test));

    ad_string_destroy(&test);

    return 0;
}
