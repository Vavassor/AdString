aft_string_add
==============

.. c:function:: bool aft_string_add(AftString* to, const AftString* from, \
        int index)

    Insert a string into another string before the byte at the given index.

    :param to: the recieving string
    :param from: the inserted string
    :param index: the index in the recieving string, from zero to its count
    :return: true if the string is added

Example
-------

.. literalinclude:: aft-string-add.c
    :language: C

Output:

.. code::

    Price 99¢

