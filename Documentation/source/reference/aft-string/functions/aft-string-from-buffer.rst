aft_string_from_buffer
======================

.. c:function:: AftMaybeString aft_string_from_buffer(const char* buffer, \
        int bytes)

    Create a string from an array of bytes.

    The resulting string is not associated with an :term:`allocator`.

    :param buffer: the byte array
    :param bytes: the number of bytes
    :return: a string

