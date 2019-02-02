aft_string_replace
==================

.. c:function:: bool aft_string_replace(AftString* to, int start, int end, \
        const AftString* from)

    Replace a range of bytes in a string with a replacement string.

    :param to: the string
    :param start: the index of the first byte in the range
    :param start: the index one past the last byte in the range
    :param from: the replacement
    :return: true if the range is replaced

