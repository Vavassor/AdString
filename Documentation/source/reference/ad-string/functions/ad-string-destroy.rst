ad_string_destroy
=================

.. c:function:: bool ad_string_destroy(AdString* string)

    Empty a string and deallocate any memory.

    If it is associated with an :term:`allocator`, it remains associated.

    :param string: the string
    :return: true if the string was destroyed

