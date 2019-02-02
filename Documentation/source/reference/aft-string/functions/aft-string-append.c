#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString readout = aft_string_copy_c_string("Date: ").value;
    AftString date = aft_string_copy_c_string("2018-12-09").value;
    aft_string_append(&readout, &date);
    
    const char* message = aft_string_get_contents_const(&readout);
    printf("%s\n", message);

    aft_string_destroy(&readout);
    aft_string_destroy(&date);

    return 0;
}

