Customizing Memory Management
=============================

By default, AftString uses the C standard library's functions for memory
allocation. To override this behaviour, add the following definition to
:file:`aft_string.h`.

.. code-block:: c

    #define AFT_USE_CUSTOM_ALLOCATOR

:c:func:`aft_allocate` and :c:func:`aft_deallocate` can then be defined by the
user at link time. The names and function definition must match the
declarations. There is no way to redefine the functions at run-time, so custom
memory functions can only be used when building from source or statically
linking the library.

.. _using-an-allocator:

Using An Allocator
------------------

The declarations include an :term:`allocator` parameter so that a user-defined
structure can be used for allocation. This can be used for local or per-thread
allocation. Global memory allocation functions, like in the C standard library,
don't need to use this.

The allocator is set only when the string is created and should not be changed.
This is to ensure the same allocator is used for allocating and deallocating a
memory block.

.. _allocator-propagation:

Allocator Propagation
^^^^^^^^^^^^^^^^^^^^^

Some string operations, such as copying or taking a substring will propagate the
allocator. This means the new string will inherit the allocator associated with
the original string. For example, in the case of substring, the new substring
will be associated with the same allocator as the string it came from.

Generally this would be fine, but it may be a detail to consider because
transferring ownership of a string between allocators after it's created is
prevented by design. If transfer is needed, use :c:func:`aft_string_assign`.

