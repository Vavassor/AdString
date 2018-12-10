aft_string_range_check
======================

.. c:function:: bool aft_string_range_check(const AftString* string, \
        const AftStringRange* range)

    Determine if a range is valid and within the bounds of a string.

    A range is allowed to be empty, where the start and end are the same.

    :param string: the string
    :param range: the range
    :return: true if the range is valid for the string

