AftStringSlice
==============

.. c:type:: AftStringSlice

    A constant range of bytes within a string. It is not null-terminated.

    The underlying string can be a :term:`C string`, an :c:type:`AftString`,
    or an array of bytes.

    If the underlying string is an :c:type:`AftString` the slice only remains
    valid as long as the original string isn't modified. This is further
    specified in the section :ref:`string-modification`.
