aft_string_destroy
==================

.. c:function:: bool aft_string_destroy(AftString* string)

    Empty a string and deallocate any memory.

    If it is associated with an :term:`allocator`, it remains associated.

    If emptying the string but *not* deallocating memory is desired, instead use
    :c:func:`aft_string_remove` on the entire range of the string.

    :param string: the string
    :return: true if the string was destroyed

Example
-------

.. literalinclude:: aft-string-destroy.c
    :language: C

Output:

.. code::

    shown instead

