aft_string_append
=================

.. c:function:: bool aft_string_append(AftString* to, const AftString* from)

    Add one string to the end of another.

    :param to: the recieving string
    :param from: the appended string
    :return: true if the string is appended

Example
-------

.. literalinclude:: aft-string-append.c
    :language: C

Output:

.. code::

    Date: 2018-12-09

