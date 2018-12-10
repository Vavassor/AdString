aft_c_string_deallocate_with_allocator
======================================

.. c:function:: bool aft_c_string_deallocate_with_allocator(void* allocator, \
        char* string)

    Deallocate a :term:`C string` returned by
    :c:func:`aft_string_to_c_string_with_allocator`.

    :param allocator: the allocator
    :param string: the C string or :c:macro:`NULL`
    :return: true if the C string was deallocated

