AftMaybeInt
===========

.. c:type:: AftMaybeInt

    An optional type representing either an :c:type:`int` or nothing.

    .. c:member:: bool valid

        True when its value is valid.

    .. c:member:: int value

        An :c:type:`int` that may be invalid.

