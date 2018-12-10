aft_string_starts_with
======================

.. c:function:: bool aft_string_starts_with(const AftString* string, \
        const AftString* lookup)

    Determine if a string has a given beginning.

    :param string: the string
    :param lookup: the beginning
    :return: true if the string has the beginning

        This is always true when the beginning is the empty string.

