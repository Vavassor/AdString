aft_c_string_copy_string_with_allocator
=======================================

.. c:function:: char* aft_c_string_copy_string_with_allocator( \
        const AftString* string, void* allocator)

    Create a copy of a string as a :term:`C string` and
    associate it with an :term:`allocator`.

    The returned C string should be deallocated using
    :c:func:`aft_c_string_deallocate_with_allocator`.

    :param string: the string
    :return: a C string or :c:macro:`NULL` if it fails to be copied

