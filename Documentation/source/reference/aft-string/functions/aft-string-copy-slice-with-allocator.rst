aft_string_copy_slice_with_allocator
====================================

.. c:function:: AftMaybeString aft_string_copy_slice_with_allocator( \
        AftStringSlice slice, void* allocator)

    Create a copy of a slice and associate it with an :term:`allocator`.

    :param slice: the slice
    :param allocator: the allocator
    :return: a copy
