ad_string_get_contents_const
============================

.. c:function:: const char* ad_string_get_contents_const(const AdString* string)

    Get a direct reference to the contents of the string.

    The reference only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: the contents

