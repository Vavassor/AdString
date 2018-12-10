aft_string_initialise_with_allocator
====================================

.. c:function:: void aft_string_initialise_with_allocator(AftString* string, \
        void* allocator)

    Initialise a string with no contents and associate it with an
    :term:`allocator`.

    .. warning::

        Do not call this on the same string twice. It could create a memory
        leak because any memory for the existing contents would not be
        deallocated. Use :c:func:`aft_string_destroy` to empty a string.

    :param string: the string
    :param allocator: the allocator

