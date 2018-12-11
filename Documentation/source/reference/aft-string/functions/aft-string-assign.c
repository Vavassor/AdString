#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString one = aft_string_from_c_string("one thing").value;

    const char* message = aft_string_get_contents_const(&one);
    printf("%s\n", message);

    AftString another = aft_string_from_c_string("and then another").value;
    aft_string_assign(&one, &another);

    message = aft_string_get_contents_const(&one);
    printf("%s\n", message);

    aft_string_destroy(&one);
    aft_string_destroy(&another);

    return 0;
}

