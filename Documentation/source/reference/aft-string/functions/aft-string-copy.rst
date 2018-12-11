aft_string_copy
===============

.. c:function:: AftMaybeString aft_string_copy(AftString* string)

    Create a copy of a string.

    The copy inherits the :term:`allocator` associated with the original string.
    See also :ref:`allocator-propagation`.

    :param string: the string
    :return: a copy

Example
-------

.. literalinclude:: aft-string-copy.c
    :language: C

Output:

.. code::

    It's me!

