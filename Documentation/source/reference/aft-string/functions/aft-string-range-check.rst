aft_string_range_check
======================

.. c:function:: bool aft_string_range_check(const AftString* string, \
        int start, int end)

    Determine if a range is valid and within the bounds of a string.

    A range is allowed to be empty, where the start and end are the same.

    :param string: the string
    :param start: the index of the first byte in the range
    :param end: the index one past the last byte in the range
    :return: true if the range is valid for the string

