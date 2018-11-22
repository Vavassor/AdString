ad_string_find_last_string
==========================

.. c:function:: int ad_string_find_last_string(const AdString* string, \
        const AdString* lookup)

    Find the last location of a substring in a string.

    :param string: the string
    :param lookup: the substring
    :return: the byte index of the beginning of the substring, or -1 if it
        isn't found

