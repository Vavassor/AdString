#include <AftString/aft_string.h>

#include <stdio.h>

int main(int argc, const char** argv)
{
    AftString path = aft_string_from_c_string("/home/fella/.local").value;
    AftString slash = aft_string_from_c_string("/").value;

    if(!aft_string_ends_with(&path, &slash))
    {
        aft_string_append(&path, &slash);
    }

    aft_string_append_c_string(&path, "include");

    const char* message = aft_string_get_contents_const(&path);
    printf("%s\n", message);
    
    aft_string_destroy(&path);
    aft_string_destroy(&slash);

    return 0;
}

