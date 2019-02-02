aft_string_add
==============

.. c:function:: bool aft_string_add(AftString* to, AftStringSlice from, \
        int index)

    Insert a slice into another string before the byte at the given index.

    :param to: the recieving string
    :param from: the inserted slice
    :param index: the index in the recieving string, from zero to its count
    :return: true if the slice is added

Example
-------

.. literalinclude:: aft-string-add.c
    :language: C

Output:

.. code::

    Price 99¢

