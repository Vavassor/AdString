#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString price = aft_string_copy_c_string(u8"Price ¢").value;
    AftString cents = aft_string_copy_c_string("99").value;
    aft_string_add(&price, &cents, 6);

    const char* message = aft_string_get_contents_const(&price);
    printf("%s\n", message);

    aft_string_destroy(&price);
    aft_string_destroy(&cents);

    return 0;
}

