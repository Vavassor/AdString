aft_codepoint_iterator_prior
============================

.. c:function:: AftMaybeChar32 aft_codepoint_iterator_prior( \
        AftCodepointIterator* it)

    Move the iterator to the prior codepoint before its current position and
    get that codepoint.

    The iterator does not move if it's at the start of the string range.

    :param it: the iterator
    :return: a codepoint

