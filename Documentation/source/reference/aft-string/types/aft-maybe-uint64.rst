AftMaybeUint64
==============

.. c:type:: AftMaybeUint64

    An optional type representing either a :c:type:`uint64_t` or nothing.

    .. c:member:: bool valid

        True when its value is valid.

    .. c:member:: uint64_t value

        A :c:type:`uint64_t` that may be invalid.

