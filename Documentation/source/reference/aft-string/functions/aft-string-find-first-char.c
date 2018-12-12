#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString string = aft_string_from_c_string("aft_string_append_c_string(&string, \"?\")").value;
    AftString entity = aft_string_from_c_string("&amp;").value;

    int index = aft_string_find_first_char(&string, '&').value;
    AftStringRange range = {index, index + 1};
    aft_string_replace(&string, &range, &entity);

    const char* message = aft_string_get_contents_const(&string);
    printf("%s\n", message);
    
    aft_string_destroy(&string);
    aft_string_destroy(&entity);

    return 0;
}

