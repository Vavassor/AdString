ad_string_get_contents
======================

.. c:function:: char* ad_string_get_contents(AdString* string)

    Get a direct reference to the modifiable contents of the string.

    The contents can only be changed within its :c:member:`AdString.count`
    bytes. The reference only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: the contents

