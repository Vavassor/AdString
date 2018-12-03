aft_string_from_buffer_with_allocator
=====================================

.. c:function:: AftMaybeString aft_string_from_buffer_with_allocator( \
        const char* buffer, int bytes, void* allocator)

    Create a string from an array of bytes and associate it with an
    :term:`allocator`.

    :param buffer: the byte array
    :param bytes: the number of bytes
    :param allocator: the allocator
    :return: a string

