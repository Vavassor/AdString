aft_string_get_contents_const
=============================

.. c:function:: const char* aft_string_get_contents_const( \
        const AftString* string)

    Get a direct reference to the contents of the string.

    The reference only remains valid as long as the original string isn't
    modified. This is further specified in the section
    :ref:`string-modification`.

    The contents is guaranteed to be a null-terminated :term:`C string`.

    :param string: the string
    :return: the contents

