aft_c_string_copy_string
========================

.. c:function:: char* aft_c_string_copy_string(const AftString* string)

    Create a copy of a string as a :term:`C string`.

    The returned C string should be deallocated using
    :c:func:`aft_c_string_deallocate`.

    :param string: the string
    :return: a C string or :c:macro:`NULL` if it fails to be copied

