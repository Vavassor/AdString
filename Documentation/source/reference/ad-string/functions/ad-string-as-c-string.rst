ad_string_as_c_string
=====================

.. c:function:: const char* ad_string_as_c_string(const AdString* string)

    Get the contents of the string as a :term:`C string`.

    The C string only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: a C string

