aft_allocate
============

.. c:function:: AftMemoryBlock aft_allocate(void* allocator, uint64_t bytes)

    Allocate a block of memory.

    This function may be user-defined as described in
    :doc:`../../custom-memory-management`. Otherwise, the default implementation
    of this function will use :c:func:`calloc` to get the required memory.

    :param allocator: The allocator to get the block from, or :c:macro:`NULL`
        if no allocator is being used. Pass :c:macro:`NULL` when using the
        default implementation.
    :param bytes: the number of bytes required for the block
    :return: a block with the requested number of bytes of memory, or an empty
        block if it fails

