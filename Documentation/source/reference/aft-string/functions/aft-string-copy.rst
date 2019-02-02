aft_string_copy
===============

.. c:function:: AftMaybeString aft_string_copy(AftString* string)

    Create a copy of a string.

    The resulting string is not associated with an :term:`allocator`.

    :param string: the string
    :return: a copy

Example
-------

.. literalinclude:: aft-string-copy.c
    :language: C

Output:

.. code::

    It's me!

