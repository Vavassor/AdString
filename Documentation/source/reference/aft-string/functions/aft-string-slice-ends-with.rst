aft_string_slice_ends_with
==========================

.. c:function:: bool aft_string_slice_ends_with(AftStringSlice slice, \
        AftStringSlice lookup)

    Determine if a slice has a given ending.

    :param slice: the slice
    :param lookup: the ending
    :return: true if the slice has the ending

        This is always true when the ending is the empty string.
