aft_ascii_is_whitespace
=======================

.. c:function:: bool aft_ascii_is_whitespace(char c)

    Determine if a character is whitespace.

    .. table:: Whitespace Characters

        =============== ========
        Name            Literal
        =============== ========
        Carriage Return ``'\r'``
        Form Feed       ``'\f'``
        Line Feed       ``'\n'``
        Space           ``' '``
        Tab             ``'\t'``
        Vertical Tab    ``'\v'``
        =============== ========

    :param c: the character
    :return: true if the character is whitespace

