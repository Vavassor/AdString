aft_string_from_c_string_with_allocator
=======================================

.. c:function:: AftMaybeString aft_string_from_c_string_with_allocator( \
        const char* original, void* allocator)

    Create a string from a :term:`C string` and associate it with an
    :term:`allocator`.

    :param original: the C string
    :param allocator: the allocator
    :return: a string

