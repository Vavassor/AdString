aft_string_get_contents
=======================

.. c:function:: char* aft_string_get_contents(AftString* string)

    Get a direct reference to the modifiable contents of the string.

    The contents can only be changed within the amount of bytes returned by
    :c:func:`aft_string_get_count`. The reference only remains valid as long as
    the original string isn't modified. This is further specified in the section
    :ref:`string-modification`.

    The contents is guaranteed to be a null-terminated :term:`C string`.

    :param string: the string
    :return: the contents

