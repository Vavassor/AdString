aft_string_slice
================

.. c:function:: AftStringSlice aft_string_slice(AftStringSlice slice, \
        int start, int end)

    Slice a range of bytes out of another slice.

    :param slice: the original slice
    :param start: the index of the first byte in the range
    :param end: the index one past the last byte in the range
    :return: a slice within the original slice
