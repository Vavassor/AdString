#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString string = aft_string_copy_c_string("unseen text").value;

    aft_string_destroy(&string);
    aft_string_append_c_string(&string, "shown instead");

    const char* message = aft_string_get_contents_const(&string);
    printf("%s\n", message);

    aft_string_destroy(&string);

    return 0;
}

