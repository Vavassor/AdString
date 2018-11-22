ad_string_to_c_string
=====================

.. c:function:: char* ad_string_to_c_string(const AdString* string)

    Create a copy of a string as a :term:`C string`.

    The returned C string should be deallocated using
    :c:func:`ad_c_string_deallocate`.

    :param string: the string
    :return: a C string or :c:macro:`NULL` if it fails to be copied

