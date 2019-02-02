aft_string_slice_string
=======================

.. c:function:: AftStringSlice aft_string_slice_string( \
        const AftString* string, int start, int end)

    Slice a range of bytes out of a string.

    :param slice: the original string
    :param start: the index of the first byte in the range
    :param end: the index one past the last byte in the range
    :return: a slice
