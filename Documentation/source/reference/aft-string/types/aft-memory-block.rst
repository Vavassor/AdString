AftMemoryBlock
==============

.. c:type:: AftMemoryBlock

    A fixed region of allocated memory.

    .. c:member:: void* memory

        The block's contents or :c:macro:`NULL` if the block is empty.

    .. c:member:: uint64_t bytes

        The number of bytes of memory in the block.

