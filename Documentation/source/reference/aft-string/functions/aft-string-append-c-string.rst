aft_string_append_c_string
==========================

.. c:function:: bool aft_string_append_c_string(AftString* to, const char* from)

    Add a :term:`C string` to the end of a string.

    :param to: the recieving string
    :param from: the appended string
    :return: true if the string is appended

Example
-------

.. literalinclude:: aft-string-append-c-string.c
    :language: C

Output:

.. code::

    30.5m/s

