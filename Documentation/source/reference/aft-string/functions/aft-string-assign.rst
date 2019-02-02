aft_string_assign
=================

.. c:function:: bool aft_string_assign(AftString* to, const AftString* from)

    Overwrite the contents of one string with another string.

    The strings do not need to be associated with the same allocator, which
    makes it usable for an allocator transfer.

    :param to: the recieving string
    :param from: the giving string
    :return: true if the string is assigned

Example
-------

.. literalinclude:: aft-string-assign.c
    :language: C

Output:

.. code::

    one thing
    and then another

