aft_string_slice_starts_with
============================

.. c:function:: bool aft_string_slice_starts_with(AftStringSlice slice, \
        AftStringSlice lookup)

    Determine if a slice has a given beginning.

    :param slice: the slice
    :param lookup: the beginning
    :return: true if the slice has the beginning

        This is always true when the beginning is the empty string.

