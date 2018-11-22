AdString
========

.. c:type:: AdString

    A growable string of bytes. It's expected to be UTF-8 encoded, but its basic
    functions don't enforce any encoding.

    .. c:member:: void* allocator

        The :term:`allocator` for the string.

    .. c:member:: int count

        The number of bytes in the string.

