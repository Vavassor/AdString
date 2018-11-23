ad_string_substring
===================

.. c:function:: AdMaybeString ad_string_substring(const AdString* string, \
        const AdStringRange* range)

    Create a copy of a range within a string.

    The substring inherits the :term:`allocator` associated with the original
    string.

    :param string: the string
    :param range: the range
    :return: a substring

