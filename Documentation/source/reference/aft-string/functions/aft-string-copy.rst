aft_string_copy
===============

.. c:function:: AftMaybeString aft_string_copy(AftString* string)

    Create a copy of a string.

    The copy inherits the :term:`allocator` associated with the original string.

    :param string: the string
    :return: a copy

