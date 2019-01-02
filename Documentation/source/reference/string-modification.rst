.. _string-modification:

String Modification
===================

Because of string functions that allow direct references to their contents,
such as :c:func:`aft_string_get_contents`, it's important to define the
situations that would invalidate that reference.

Modifying Functions
-------------------

Calling any of these functions on the string would be a modification.

- :c:func:`aft_string_add`
- :c:func:`aft_string_append`
- :c:func:`aft_string_append_c_string`
- :c:func:`aft_string_append_char`
- :c:func:`aft_string_assign`
- :c:func:`aft_string_destroy`
- :c:func:`aft_string_remove`
- :c:func:`aft_string_replace`

