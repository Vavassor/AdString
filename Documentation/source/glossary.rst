.. _glossary:

Glossary
========

.. glossary::

    Allocator
        An allocator is any user-specified structure used for memory allocation.
        This is intended for any custom memory management that's tied to a local
        structure. See the section on specific usage, :ref:`using-an-allocator`.

    ASCII
        The American Standard Code for Information Interchange is a character
        encoding for electronic communication. It's a fixed-width 7-bit
        encoding. But, here is assumed to be stored in 8-bit bytes where the
        most significant bit is zero; this is extremely common.

        It's a subset of :term:`UTF-8`, and consists of characters that are
        common in many contexts, like western arabic numerals and the latin
        alphabet. So, it's sometimes useful to know when a string is ASCII only
        because shortcuts can be taken.

    C string
        C strings are the form of string that string literals in C and C++ use.
        They're an array of characters followed by a single null character.

    Normalization Form C
        Unicode allows multiple representations of the same string. Because of
        this, Unicode Normalization Forms are defined to make it possible to
        determine whether two strings are equivalent despite any difference in
        representation.

        Normalization Form C is the form that uses canonical composite
        characters where possible. It and the other forms are formally defined
        in `Unicode Normlization Forms <https://unicode.org/reports/tr15/>`__.

    UTF-8
        The Unicode Transformation Format – 8-bit is a variable width character
        encoding for Unicode. It's the default encoding assumed by this library.

