aft_codepoint_iterator_next
===========================

.. c:function:: AftMaybeChar32 aft_codepoint_iterator_next( \
        AftCodepointIterator* it)

    Get the codepoint at the current iterator position and move the iterator to
    the next one.

    The iterator is not moved when its position is at the end of the string
    range.

    :param it: the iterator
    :return: a codepoint

