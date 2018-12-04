AftString
=========

.. c:type:: AftString

    A growable string of bytes. It's expected to be :term:`UTF-8` encoded and in
    :term:`Normalization Form C`, but its basic functions don't enforce any
    encoding.

    .. c:member:: void* allocator

        The :term:`allocator` for the string or :c:macro:`NULL` when no
        allocator is associated.

        This member is read-only.

