aft_string_find_first_char
==========================

.. c:function:: AftMaybeInt aft_string_find_first_char( \
        const AftString* string, char c)

    Find the first location of a :c:type:`char` in a string.

    :param string: the string
    :param c: the :c:type:`char` to find
    :return: the byte index of the :c:type:`char`

Example
-------

.. literalinclude:: aft-string-find-first-char.c
    :language: C

Output:

.. code::

    aft_string_append_c_string(&amp;string, "?")

