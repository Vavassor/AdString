ad_string_from_c_string
=======================

.. c:function:: AdMaybeString ad_string_from_c_string(const char* original)

    Create a string from a :term:`C string`.

    The resulting string is not associated with an :term:`allocator`.

    :param original: the C string
    :return: a string

