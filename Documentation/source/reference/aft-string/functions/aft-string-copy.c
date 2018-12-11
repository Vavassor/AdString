#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString me = aft_string_from_c_string("It's me!").value;

    AftString clone = aft_string_copy(&me).value;

    const char* message = aft_string_get_contents_const(&clone);
    printf("%s\n", message);

    aft_string_destroy(&me);
    aft_string_destroy(&clone);

    return 0;
}

