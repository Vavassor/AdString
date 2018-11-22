AdString
========

Types
-----

.. c:type:: AdMaybeString

    An optional type representing either a string or nothing.

    .. c:member:: bool valid

        True when its value is valid.

    .. c:member:: AdString value

        A string that may be invalid.

.. c:type:: AdMemoryBlock

    .. c:member:: void* memory

        The block's contents or :c:macro:`NULL` if the block is empty.

    .. c:member:: uint64_t bytes

        The number of bytes of memory in the block.

.. c:type:: AdString

    A growable string of bytes. It's expected to be UTF-8 encoded, but its basic
    functions don't enforce any encoding.

    .. c:member:: void* allocator

        The :term:`allocator` for the string.

    .. c:member:: int count

        The number of bytes in the string.

.. c:type:: AdStringRange

    A range of bytes within an :c:type:`AdString`.

    .. c:member:: int end
    
        The index one past the last byte.

    .. c:member:: int start
    
        The index of the first byte.

Functions
---------

.. c:function:: bool ad_c_string_deallocate(char* string)

    Deallocate a :term:`C string` returned by :c:func:`ad_string_to_c_string`.

    :param string: the C string
    :return: true if the C string was deallocated

.. c:function:: bool ad_c_string_deallocate_with_allocator(void* allocator, \
        char* string)

    Deallocate a :term:`C string` returned by
    :c:func:`ad_string_to_c_string_with_allocator`.

    :param allocator: the allocator
    :param string: the C string
    :return: true if the C string was deallocated

.. c:function:: AdMemoryBlock ad_string_allocate(void* allocator, \
        uint64_t bytes)

    Allocate a block of memory.

    This function may be used-defined as described in
    :doc:`custom-memory-management`. Otherwise, the default implementation of
    this function will use :c:func:`calloc` to get the required memory.

    :param allocator: The allocator to get the block from, or :c:macro:`NULL`
        if no allocator is being used. Pass :c:macro:`NULL` when using the
        default implementation.
    :param bytes: the number of bytes required for the block
    :return: a block with the requested number of bytes of memory, or an empty
        block if it fails

.. c:function:: bool ad_string_deallocate(void* allocator, AdMemoryBlock block)

    Deallocate a block of memory.

    This function may be used-defined as described in
    :doc:`custom-memory-management`. Otherwise, the default implementation of
    this function will use :c:func:`free` to release the memory.

    :param allocator: The allocator to give the block to, or :c:macro:`NULL` if
        no allocator is being used. Pass :c:macro:`NULL` when using the default
        implementation.
    :param block: the block previously returned from
        :c:func:`ad_string_allocate` or an empty block
    
.. c:function:: bool ad_string_add(AdString* to, const AdString* from, \
        int index)

    Insert a string into another string before the byte at the given index.

    :param to: the recieving string
    :param from: the inserted string
    :return: true if the string is added

.. c:function:: bool ad_string_append(AdString* to, const AdString* from)

    Add one string to the end of another.

    :param to: the recieving string
    :param from: the appended string
    :return: true if the string is appended

.. c:function:: bool ad_string_append_c_string(AdString* to, const char* from)

    Add a :term:`C string` to the end of a string.

    :param to: the recieving string
    :param from: the appended string
    :return: true if the string is appended

.. c:function:: const char* ad_string_as_c_string(const AdString* string)

    Get the contents of the string as a :term:`C string`.

    The C string only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: a C string

.. c:function:: bool ad_string_assign(AdString* to, const AdString* from)

    Overwrite the contents of one string with another string.

    :param to: the recieving string
    :param from: the giving string
    :return: true if the string is assigned

.. c:function:: AdMaybeString ad_string_copy(AdString* string)

    Create a copy of a string.

    :param string: the string
    :return: a copy

.. c:function:: bool ad_string_destroy(AdString* string)

    Destroy a string.

    :param string: the string
    :return: true if the string was destroyed

.. c:function:: bool ad_string_ends_with(const AdString* string, \
        const AdString* lookup)

    Determine if a string has a given ending.

    :param string: the string
    :param lookup: the ending
    :return: true if the string has the ending

.. c:function:: int ad_string_find_first_char(const AdString* string, char c)

    Find the first location of a :c:type:`char` in a string.

    :param string: the string
    :param c: the :c:type:`char` to find
    :return: the byte index of the :c:type:`char`, or -1 if it isn't found

.. c:function:: int ad_string_find_first_string(const AdString* string, \
        const AdString* lookup)

    Find the first location of a substring in a string.

    :param string: the string
    :param lookup: the substring
    :return: the byte index of the beginning of the substring, or -1 if it
        isn't found

.. c:function:: int ad_string_find_last_char(const AdString* string, char c)

    Find the last location of a :c:type:`char` in a string.

    :param string: the string
    :param c: the :c:type:`char` to find
    :return: the byte index of the :c:type:`char`, or -1 if it isn't found

.. c:function:: int ad_string_find_last_string(const AdString* string, \
        const AdString* lookup)

    Find the last location of a substring in a string.

    :param string: the string
    :param lookup: the substring
    :return: the byte index of the beginning of the substring, or -1 if it
        isn't found

.. c:function:: AdMaybeString ad_string_from_buffer(const char* buffer, \
        int bytes)

    Create a string from an array of bytes.

    :param buffer: the byte array
    :param bytes: the number of bytes
    :return: a string

.. c:function:: AdMaybeString ad_string_from_buffer_with_allocator( \
        const char* buffer, int bytes, void* allocator)

    Create a string from an array of bytes and associate it with an
    :term:`allocator`.

    :param buffer: the byte array
    :param bytes: the number of bytes
    :param allocator: the allocator
    :return: a string

.. c:function:: AdMaybeString ad_string_from_c_string(const char* original)

    Create a string from a :term:`C string`.

    :param original: the C string
    :return: a string

.. c:function:: AdMaybeString ad_string_from_c_string_with_allocator( \
        const char* original, void* allocator)

    Create a string from a :term:`C string` and associate it with an
    :term:`allocator`.

    :param original: the C string
    :param allocator: the allocator
    :return: a string

.. c:function:: char* ad_string_get_contents(AdString* string)

    Get a direct reference to the contents of the string. The contents can be
    modified within its :c:member:`AdString.count` bytes.

    The reference only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: the contents

.. c:function:: const char* ad_string_get_contents_const(const AdString* string)

    Get a direct reference to the contents of the string.

    The reference only remains valid as long as the original string isn't
    modified.

    :param string: the string
    :return: the contents

.. c:function:: void ad_string_initialise(AdString* string)

    Initialise a string with no contents.

    :param string: the string

.. c:function:: void ad_string_initialise_with_allocator(AdString* string, \
        void* allocator)

    Initialise a string with no contents and associate it with an
    :term:`allocator`.

    :param string: the string

.. c:function:: void ad_string_remove(AdString* string, \
        const AdStringRange* range)

    Remove a range of bytes from a string.

    :param string: the string
    :param range: the range

.. c:function:: bool ad_string_reserve(AdString* string, int count)

    Reserve some amount of space for the contents of the string.

    :param string: the string
    :param count: the number of bytes
    :return: true if the space is reserved

.. c:function:: bool ad_string_starts_with(const AdString* string, \
        const AdString* lookup)

    Determine if a string has a given beginning.

    :param string: the string
    :param lookup: the beginning
    :return: true if the string has the beginning

.. c:function:: AdMaybeString ad_string_substring(const AdString* string, \
        const AdStringRange* range)

    Create a copy of a range within a string.

    :param string: the string
    :param range: the range
    :return: a substring

.. c:function:: char* ad_string_to_c_string(const AdString* string)

    Create a copy of a string as a :term:`C string`.

    The returned C string should be deallocated using
    :c:func:`ad_c_string_deallocate`.

    :param string: the string
    :return: a C string or :c:macro:`NULL` if it fails to be copied

.. c:function:: char* ad_string_to_c_string_with_allocator( \
        const AdString* string, void* allocator)

    Create a copy of a string as a :term:`C string` and
    associate it with an :term:`allocator`.

    The returned C string should be deallocated using
    :c:func:`ad_c_string_deallocate_with_allocator`.

    :param string: the string
    :return: a C string or :c:macro:`NULL` if it fails to be copied

.. c:function:: bool ad_string_range_check(const AdString* string, \
        const AdStringRange* range)

    Determine if a range is valid and within the bounds of a string.

    :param string: the string
    :param range: the range
    :return: true if the range is valid for the string

.. c:function:: bool ad_strings_match(const AdString* a, const AdString* b)

    Determine if two strings' contents are exactly the same.

    :param a: the first string
    :param b: the second string
    :return: true if the strings match
        
