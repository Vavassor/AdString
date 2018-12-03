aft_codepoint_iterator_get_index
================================

.. c:function:: int aft_codepoint_iterator_get_index(AftCodepointIterator* it)

    Get the byte index of the iterator within the string range.

    :param it: the iterator
    :return: a byte index at a heading byte of a UTF-8 byte sequence, or one
        past the end of the range

