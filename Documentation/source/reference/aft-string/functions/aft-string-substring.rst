aft_string_substring
====================

.. c:function:: AftMaybeString aft_string_substring(const AftString* string, \
        const AftStringRange* range)

    Create a copy of a range within a string.

    The substring inherits the :term:`allocator` associated with the original
    string. See also :ref:`allocator-propagation`.

    :param string: the string
    :param range: the range
    :return: a substring

