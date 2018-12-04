aft_ascii_compare_alphabetic
============================

.. c:function:: int aft_ascii_compare_alphabetic(const AftString* a, \
        const AftString* b)

    Compare the alphabetic order of two strings.

    This is case insensitive and is done as though letters are first converted
    to upper case. The first pair of characters that differ in the strings are
    compared according to their character value.

    :param a: the first string
    :param b: the second string
    :return:
        - A negative number if ``a`` appears before ``b`` in order.
        - Zero if the strings are the same order.
        - A positive number if ``b`` appears before ``a`` in order.

