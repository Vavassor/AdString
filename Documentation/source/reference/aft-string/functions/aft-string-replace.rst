aft_string_replace
==================

.. c:function:: bool aft_string_replace(AftString* to, \
        const AftStringRange* range, const AftString* from)

    Replace a range of bytes in a string with a replacement string.

    :param to: the string
    :param range: the range to replace
    :param from: the replacement
    :return: true if the range is replaced

