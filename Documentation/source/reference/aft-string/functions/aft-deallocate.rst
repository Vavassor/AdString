aft_deallocate
==============

.. c:function:: bool aft_deallocate(void* allocator, AftMemoryBlock block)

    Deallocate a block of memory.

    This function may be user-defined as described in
    :doc:`../../custom-memory-management`. Otherwise, the default implementation
    of this function will use :c:func:`free` to release the memory.

    :param allocator: The allocator to give the block to, or :c:macro:`NULL` if
        no allocator is being used. Pass :c:macro:`NULL` when using the default
        implementation.
    :param block: the block previously returned from :c:func:`aft_allocate` or
        an empty block

