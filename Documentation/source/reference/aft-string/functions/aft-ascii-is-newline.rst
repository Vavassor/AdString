aft_ascii_is_newline
====================

.. c:function:: bool aft_ascii_is_newline(char c)

    Determine if a character is a newline.
    
    .. table:: Newline Characters

        =============== ========
        Name            Literal
        =============== ========
        Carriage Return ``'\r'``
        Form Feed       ``'\f'``
        Line Feed       ``'\n'``
        Vertical Tab    ``'\v'``
        =============== ========

    :param c: the character
    :return: true if a character is a newline

