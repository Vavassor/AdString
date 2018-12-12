aft_string_ends_with
====================

.. c:function:: bool aft_string_ends_with(const AftString* string, \
        const AftString* lookup)

    Determine if a string has a given ending.

    :param string: the string
    :param lookup: the ending
    :return: true if the string has the ending

        This is always true when the ending is the empty string.

Example
-------

.. literalinclude:: aft-string-ends-with.c
    :language: C

Output:

.. code::

    /home/fella/.local/include

