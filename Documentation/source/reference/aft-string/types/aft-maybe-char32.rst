AftMaybeChar32
==============

.. c:type:: AftMaybeChar32

    An optional type representing either a :c:type:`char32_t` or nothing.

    .. c:member:: bool valid

        True when its value is valid.

    .. c:member:: char32_t value

        An :c:type:`char32_t` that may be invalid.

