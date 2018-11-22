ad_string_deallocate
====================

.. c:function:: bool ad_string_deallocate(void* allocator, AdMemoryBlock block)

    Deallocate a block of memory.

    This function may be used-defined as described in
    :doc:`../../custom-memory-management`. Otherwise, the default implementation
    of this function will use :c:func:`free` to release the memory.

    :param allocator: The allocator to give the block to, or :c:macro:`NULL` if
        no allocator is being used. Pass :c:macro:`NULL` when using the default
        implementation.
    :param block: the block previously returned from
        :c:func:`ad_string_allocate` or an empty block

