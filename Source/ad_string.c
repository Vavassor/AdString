#include "ad_string.h"

#include <assert.h>
#include <stddef.h>


#define AD_ASSERT(expression) \
    assert(expression)

#define UINT64_MAX_DIGITS 20


#if !defined(AD_USE_CUSTOM_ALLOCATOR)

#include <stdlib.h>

AdMemoryBlock ad_string_allocate(void* allocator, uint64_t bytes)
{
    (void) allocator;
    AdMemoryBlock block =
    {
        .memory = calloc(bytes, 1),
        .bytes = bytes,
    };
    if(!block.memory)
    {
        block.bytes = 0;
    }
    return block;
}

bool ad_string_deallocate(void* allocator, AdMemoryBlock block)
{
    (void) allocator;
    free(block.memory);
    return true;
}

#endif // !defined(AD_USE_CUSTOM_ALLOCATOR)


static const uint64_t powers_of_ten[UINT64_MAX_DIGITS] =
{
    UINT64_C(10000000000000000000),
    UINT64_C(1000000000000000000),
    UINT64_C(100000000000000000),
    UINT64_C(10000000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(100000000000000),
    UINT64_C(10000000000000),
    UINT64_C(1000000000000),
    UINT64_C(100000000000),
    UINT64_C(10000000000),
    UINT64_C(1000000000),
    UINT64_C(100000000),
    UINT64_C(10000000),
    UINT64_C(1000000),
    UINT64_C(100000),
    UINT64_C(10000),
    UINT64_C(1000),
    UINT64_C(100),
    UINT64_C(10),
    UINT64_C(1),
};

static const uint8_t utf8_decode_state_table[] =
{
    0, 1, 2, 3, 5, 8, 7, 1, 1, 1, 4, 6, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,
    1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1,
    1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1,
    1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

static const uint8_t utf8_decode_type_table[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3,
    11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};


static bool ad_string_is_big(const AdString* string)
{
    return string->cap > AD_STRING_SMALL_CAP;
}

static void ad_string_set_count(AdString* string, int count)
{
    if(ad_string_is_big(string))
    {
        string->big.count = count;
    }
    else
    {
        string->small.bytes_left = AD_STRING_SMALL_CAP - count;
    }
}

static void copy_memory(void* to, const void* from, uint64_t bytes)
{
    const uint8_t* p0 = from;
    uint8_t* p1 = to;

    if(p0 < p1)
    {
        for(p0 += bytes, p1 += bytes; bytes; bytes -= 1)
        {
            p0 -= 1;
            p1 -= 1;
            *p1 = *p0;
        }
    }
    else
    {
        for(; bytes; bytes -= 1, p0 +=1, p1 += 1)
        {
            *p1 = *p0;
        }
    }
}

static int int_max(int a, int b)
{
    return (a > b) ? a : b;
}

static bool is_heading_byte(char c)
{
    return (c & 0xc0) != 0x80;
}

static bool memory_matches(const void* a, const void* b, int n)
{
    AD_ASSERT(a);
    AD_ASSERT(b);

    const uint8_t* p1 = (const uint8_t*) a;
    const uint8_t* p2 = (const uint8_t*) b;

    for(; n; n -= 1)
    {
        if(*p1 != *p2)
        {
            return false;
        }
        else
        {
            p1 += 1;
            p2 += 1;
        }
    }

    return true;
}

static int string_size(const char* string)
{
    if(string)
    {
        const char* s;
        for(s = string; *s; s += 1);
        return (int) (s - string);
    }

    return 0;
}

static void zero_memory(void* memory, uint64_t bytes)
{
    for(uint8_t* p = memory; bytes; bytes -= 1, p += 1)
    {
        *p = 0;
    }
}


bool ad_ascii_check(const AdString* string)
{
    int count = ad_string_get_count(string);
    AdStringRange range = {0, count};
    return ad_ascii_check_range(string, &range);
}

bool ad_ascii_check_range(const AdString* string, const AdStringRange* range)
{
    AD_ASSERT(string);
    AD_ASSERT(range);

    const char* contents = ad_string_get_contents_const(string);

    for(int char_index = range->start; char_index < range->end; char_index += 1)
    {
        if(contents[char_index] & 0x80)
        {
            return false;
        }
    }

    return true;
}

int ad_ascii_compare_alphabetic(const AdString* a, const AdString* b)
{
    AD_ASSERT(a);
    AD_ASSERT(b);

    const char* a_contents = ad_string_get_contents_const(a);
    const char* b_contents = ad_string_get_contents_const(b);
    int a_count = ad_string_get_count(a);
    int b_count = ad_string_get_count(b);

    for(int char_index = 0; char_index < a_count; char_index += 1)
    {
        char c0 = ad_ascii_to_uppercase_char(a_contents[char_index]);
        char c1 = ad_ascii_to_uppercase_char(b_contents[char_index]);

        if(c0 != c1)
        {
            return c0 - c1;
        }
    }

    return a_count - b_count;
}

int ad_ascii_digit_to_int(char c)
{
    if('0' <= c && c <= '9')
    {
        return c - '0';
    }
    else if('a' <= c && c <= 'z')
    {
        return c - 'a' + 10;
    }
    else if('A' <= c && c <= 'Z')
    {
        return c - 'A' + 10;
    }
    return 0;
}

bool ad_ascii_is_alphabetic(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool ad_ascii_is_alphanumeric(char c)
{
    return ad_ascii_is_alphabetic(c) || ad_ascii_is_numeric(c);
}

bool ad_ascii_is_lowercase(char c)
{
    return c >= 'a' && c <= 'z';
}

bool ad_ascii_is_newline(char c)
{
    return c >= '\n' && c <= '\r';
}

bool ad_ascii_is_numeric(char c)
{
    return c >= '0' && c <= '9';
}

bool ad_ascii_is_space_or_tab(char c)
{
    return c == ' ' || c == '\t';
}

bool ad_ascii_is_uppercase(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool ad_ascii_is_whitespace(char c)
{
    return c == ' ' || c - 9 <= 5;
}

void ad_ascii_to_lowercase(AdString* string)
{
    AD_ASSERT(string);

    char* contents = ad_string_get_contents(string);
    int count = ad_string_get_count(string);

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        contents[char_index] = ad_ascii_to_lowercase_char(contents[char_index]);
    }
}

char ad_ascii_to_lowercase_char(char c)
{
    if(ad_ascii_is_uppercase(c))
    {
        return 'A' + (c - 'a');
    }
    else
    {
        return c;
    }
}

void ad_ascii_to_uppercase(AdString* string)
{
    AD_ASSERT(string);

    char* contents = ad_string_get_contents(string);
    int count = ad_string_get_count(string);

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        contents[char_index] = ad_ascii_to_uppercase_char(contents[char_index]);
    }
}

char ad_ascii_to_uppercase_char(char c)
{
    if(ad_ascii_is_lowercase(c))
    {
        return 'A' + (c - 'a');
    }
    else
    {
        return c;
    }
}

AdMaybeUint64 ad_ascii_uint64_from_string(const AdString* string)
{
    int count = ad_string_get_count(string);
    AdStringRange range = {0, count};
    return ad_ascii_uint64_from_string_range(string, &range);
}

AdMaybeUint64 ad_ascii_uint64_from_string_range(const AdString* string,
        const AdStringRange* range)
{
    AD_ASSERT(string);
    AD_ASSERT(range);
    AD_ASSERT(ad_string_range_check(string, range));
    AD_ASSERT(ad_ascii_check_range(string, range));

    AdMaybeUint64 result;
    result.valid = true;
    result.value = 0;

    const char* contents = ad_string_get_contents_const(string);

    int count = ad_string_get_count(string);
    int power_index = UINT64_MAX_DIGITS - count;

    for(int char_index = range->start; char_index < range->end; char_index += 1)
    {
        uint64_t character = contents[char_index];
        uint64_t digit = character - '0';
        if(digit >= 10)
        {
            result.valid = false;
            return result;
        }
        result.value += powers_of_ten[power_index] * digit;
    }

    return result;
}


bool ad_c_string_deallocate(char* string)
{
    return ad_c_string_deallocate_with_allocator(NULL, string);
}

bool ad_c_string_deallocate_with_allocator(void* allocator, char* string)
{
    AdMemoryBlock block =
    {
        .memory = string,
        .bytes = string_size(string) + 1,
    };
    return ad_string_deallocate(allocator, block);
}


bool ad_string_add(AdString* to, const AdString* from, int index)
{
    int to_count = ad_string_get_count(to);
    int from_count = ad_string_get_count(from);

    AD_ASSERT(to);
    AD_ASSERT(from);
    AD_ASSERT(index >= 0 && index <= to_count);

    int count = to_count + from_count;
    bool reserved = ad_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    ad_string_set_count(to, count);
    char* to_contents = ad_string_get_contents(to);
    const char* from_contents = ad_string_get_contents_const(from);
    int shift_bytes = prior_count - index;
    if(shift_bytes > 0)
    {
        copy_memory(&to_contents[index + from_count], &to_contents[index],
                shift_bytes);
    }
    copy_memory(&to_contents[index], from_contents, from_count);
    to_contents[count] = '\0';

    return true;
}

bool ad_string_append(AdString* to, const AdString* from)
{
    AD_ASSERT(to);
    AD_ASSERT(from);

    int to_count = ad_string_get_count(to);
    int from_count = ad_string_get_count(from);
    int count = to_count + from_count;
    bool reserved = ad_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    ad_string_set_count(to, count);
    char* to_contents = ad_string_get_contents(to);
    const char* from_contents = ad_string_get_contents_const(from);
    copy_memory(&to_contents[prior_count], from_contents, from_count);
    to_contents[count] = '\0';

    return true;
}

bool ad_string_append_c_string(AdString* to, const char* from)
{
    AD_ASSERT(to);
    AD_ASSERT(from);

    int from_count = string_size(from);
    int to_count = ad_string_get_count(to);
    int count = to_count + from_count;
    bool reserved = ad_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    ad_string_set_count(to, count);
    char* to_contents = ad_string_get_contents(to);
    copy_memory(&to_contents[prior_count], from, from_count);
    to_contents[count] = '\0';

    return true;
}

bool ad_string_assign(AdString* to, const AdString* from)
{
    AD_ASSERT(to);
    AD_ASSERT(from);

    int from_count = ad_string_get_count(from);
    bool reserved = ad_string_reserve(to, from_count);

    if(!reserved)
    {
        return false;
    }

    int count = from_count;
    ad_string_set_count(to, count);
    char* to_contents = ad_string_get_contents(to);
    const char* from_contents = ad_string_get_contents_const(from);
    copy_memory(to_contents, from_contents, count);
    to_contents[count] = '\0';

    return true;
}

AdMaybeString ad_string_copy(AdString* string)
{
    AD_ASSERT(string);

    int count = ad_string_get_count(string);

    AdMaybeString result;
    result.valid = true;
    result.value.allocator = string->allocator;
    result.value.cap = AD_STRING_SMALL_CAP;
    ad_string_set_count(&result.value, count);
    bool reserved = ad_string_reserve(&result.value, count);

    if(!reserved)
    {
        result.valid = false;
        zero_memory(&result.value, sizeof(result.value));
        return result;
    }

    char* to_contents = ad_string_get_contents(&result.value);
    const char* from_contents = ad_string_get_contents_const(string);
    copy_memory(to_contents, from_contents, count);
    to_contents[count] = '\0';

    return result;
}

bool ad_string_destroy(AdString* string)
{
    AD_ASSERT(string);

    bool result = true;

    if(ad_string_is_big(string))
    {
        AdMemoryBlock block =
        {
            .memory = string->big.contents,
            .bytes = string->cap,
        };
        result = ad_string_deallocate(string->allocator, block);
    }

    ad_string_initialise_with_allocator(string, string->allocator);

    return result;
}

bool ad_string_ends_with(const AdString* string, const AdString* lookup)
{
    AD_ASSERT(string);
    AD_ASSERT(lookup);

    int lookup_count = ad_string_get_count(lookup);
    int string_count = ad_string_get_count(string);

    if(lookup_count > string_count)
    {
        return false;
    }
    else
    {
        const char* string_contents = ad_string_get_contents_const(string);
        const char* lookup_contents = ad_string_get_contents_const(lookup);
        const char* near_end = &string_contents[string_count - lookup_count];
        return memory_matches(near_end, lookup_contents, lookup_count);
    }
}

AdMaybeInt ad_string_find_first_char(const AdString* string, char c)
{
    AD_ASSERT(string);

    AdMaybeInt result;

    const char* contents = ad_string_get_contents_const(string);
    int count = ad_string_get_count(string);

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        if(contents[char_index] == c)
        {
            result.value = char_index;
            result.valid = true;
            return result;
        }
    }

    result.valid = false;
    result.value = 0;
    return result;
}

AdMaybeInt ad_string_find_first_string(const AdString* string,
        const AdString* lookup)
{
    AD_ASSERT(string);
    AD_ASSERT(lookup);

    AdMaybeInt result;

    const char* string_contents = ad_string_get_contents_const(string);
    const char* lookup_contents = ad_string_get_contents_const(lookup);

    int string_count = ad_string_get_count(string);
    int lookup_count = ad_string_get_count(lookup);
    int search_count = string_count - lookup_count;

    for(int char_index = 0; char_index < search_count; char_index += 1)
    {
        if(memory_matches(&string_contents[char_index], lookup_contents,
                lookup_count))
        {
            result.value = char_index;
            result.valid = true;
            return result;
        }
    }

    result.value = 0;
    result.valid = false;
    return result;
}

AdMaybeInt ad_string_find_last_char(const AdString* string, char c)
{
    AD_ASSERT(string);

    AdMaybeInt result;

    const char* contents = ad_string_get_contents_const(string);
    int count = ad_string_get_count(string);

    for(int char_index = count - 1; char_index >= 0; char_index -= 1)
    {
        if(contents[char_index] == c)
        {
            result.value = char_index;
            result.valid = true;
            return result;
        }
    }

    result.value = 0;
    result.valid = false;
    return result;
}

AdMaybeInt ad_string_find_last_string(const AdString* string,
        const AdString* lookup)
{
    AD_ASSERT(string);
    AD_ASSERT(lookup);

    AdMaybeInt result;

    const char* string_contents = ad_string_get_contents_const(string);
    const char* lookup_contents = ad_string_get_contents_const(lookup);

    int string_count = ad_string_get_count(string);
    int lookup_count = ad_string_get_count(lookup);
    int search_count = string_count - lookup_count;

    for(int char_index = search_count - 1; char_index >= 0; char_index -= 1)
    {
        if(memory_matches(&string_contents[char_index], lookup_contents,
                lookup_count))
        {
            result.value = char_index;
            result.valid = true;
            return result;
        }
    }

    result.value = 0;
    result.valid = false;
    return result;
}

AdMaybeString ad_string_from_buffer(const char* buffer, int bytes)
{
    return ad_string_from_buffer_with_allocator(buffer, bytes, NULL);
}

AdMaybeString ad_string_from_buffer_with_allocator(const char* buffer,
        int bytes, void* allocator)
{
    int cap = bytes + 1;

    AdMaybeString result;
    result.valid = true;
    result.value.allocator = allocator;

    if(cap > AD_STRING_SMALL_CAP)
    {
        AdMemoryBlock block = ad_string_allocate(allocator, cap);
        char* copy = block.memory;

        if(!copy)
        {
            result.valid = false;
            zero_memory(&result.value, sizeof(result.value));
            return result;
        }

        copy_memory(copy, buffer, bytes);
        copy[bytes] = '\0';
        result.value.big.contents = copy;
        result.value.cap = cap;
        ad_string_set_count(&result.value, bytes);
    }
    else
    {
        copy_memory(result.value.small.contents, buffer, bytes);
        result.value.small.contents[bytes] = '\0';
        result.value.cap = AD_STRING_SMALL_CAP;
        ad_string_set_count(&result.value, bytes);
    }

    return result;
}

AdMaybeString ad_string_from_c_string(const char* original)
{
    return ad_string_from_c_string_with_allocator(original, NULL);
}

AdMaybeString ad_string_from_c_string_with_allocator(const char* original,
        void* allocator)
{
    int bytes = string_size(original);
    int cap = bytes + 1;

    AdMaybeString result;
    result.valid = true;
    result.value.allocator = allocator;

    if(cap > AD_STRING_SMALL_CAP)
    {
        AdMemoryBlock block = ad_string_allocate(allocator, cap);
        char* copy = block.memory;

        if(!copy)
        {
            result.valid = false;
            zero_memory(&result.value, sizeof(result.value));
            return result;
        }

        copy_memory(copy, original, bytes);
        copy[bytes] = '\0';
        result.value.big.contents = copy;
        result.value.cap = cap;
        ad_string_set_count(&result.value, bytes);
    }
    else
    {
        copy_memory(result.value.small.contents, original, bytes);
        result.value.small.contents[bytes] = '\0';
        result.value.cap = AD_STRING_SMALL_CAP;
        ad_string_set_count(&result.value, bytes);
    }

    return result;
}

int ad_string_get_capacity(const AdString* string)
{
    AD_ASSERT(string);

    return string->cap - 1;
}

char* ad_string_get_contents(AdString* string)
{
    AD_ASSERT(string);

    if(ad_string_is_big(string))
    {
        return string->big.contents;
    }
    else
    {
        return string->small.contents;
    }
}

const char* ad_string_get_contents_const(const AdString* string)
{
    AD_ASSERT(string);

    if(ad_string_is_big(string))
    {
        return string->big.contents;
    }
    else
    {
        return string->small.contents;
    }
}

int ad_string_get_count(const AdString* string)
{
    AD_ASSERT(string);

    if(ad_string_is_big(string))
    {
        return string->big.count;
    }
    else
    {
        return AD_STRING_SMALL_CAP - string->small.bytes_left;
    }
}

void ad_string_initialise(AdString* string)
{
    ad_string_initialise_with_allocator(string, NULL);
}

void ad_string_initialise_with_allocator(AdString* string, void* allocator)
{
    AD_ASSERT(string);

    string->allocator = allocator;
    string->cap = AD_STRING_SMALL_CAP;
    ad_string_set_count(string, 0);
    zero_memory(string->small.contents, AD_STRING_SMALL_CAP - 1);
}

void ad_string_remove(AdString* string, const AdStringRange* range)
{
    AD_ASSERT(string);
    AD_ASSERT(range);
    AD_ASSERT(ad_string_range_check(string, range));

    int count = ad_string_get_count(string);
    int copied_bytes = count - range->end;
    int removed_bytes = range->end - range->start;

    char* contents = ad_string_get_contents(string);
    copy_memory(&contents[range->start], &contents[range->end], copied_bytes);
    count -= removed_bytes;
    ad_string_set_count(string, count);
    contents[count] = '\0';
}

bool ad_string_replace(AdString* to, const AdStringRange* range,
        const AdString* from)
{
    AD_ASSERT(to);
    AD_ASSERT(range);
    AD_ASSERT(from);
    AD_ASSERT(ad_string_range_check(to, range));

    int view_bytes = range->end - range->start;
    int to_count = ad_string_get_count(to);
    int from_count = ad_string_get_count(from);
    int count = to_count - view_bytes + from_count;

    bool reserved = ad_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    char* to_contents = ad_string_get_contents(to);
    const char* from_contents = ad_string_get_contents_const(from);

    int inserted_end = range->start + from_count;
    int moved_bytes = to_count - range->end;
    copy_memory(&to_contents[inserted_end], &to_contents[range->end],
            moved_bytes);

    copy_memory(&to_contents[range->start], from_contents, from_count);
    ad_string_set_count(to, count);
    to_contents[count] = '\0';

    return true;
}

bool ad_string_reserve(AdString* string, int space)
{
    AD_ASSERT(string);
    AD_ASSERT(space > 0);

    int needed_cap = space + 1;
    int existing_cap = ad_string_get_capacity(string);

    if(needed_cap > existing_cap)
    {
        int prior_cap = existing_cap;
        int cap = int_max(2 * prior_cap, needed_cap);
        AdMemoryBlock block = ad_string_allocate(string->allocator, cap);
        char* contents = block.memory;

        if(!contents)
        {
            return false;
        }

        int count = ad_string_get_count(string);

        if(count)
        {
            if(prior_cap > AD_STRING_SMALL_CAP)
            {
                char* prior_contents = string->big.contents;
                copy_memory(contents, prior_contents, count);
                AdMemoryBlock block =
                {
                    .memory = prior_contents,
                    .bytes = string->cap,
                };
                ad_string_deallocate(string->allocator, block);
            }
            else
            {
                char* prior_contents = string->small.contents;
                copy_memory(contents, prior_contents, count);
                zero_memory(prior_contents, prior_cap);
            }
        }

        AD_ASSERT(cap > AD_STRING_SMALL_CAP);
        string->big.contents = contents;
        string->cap = cap;
    }

    return true;
}

bool ad_string_starts_with(const AdString* string, const AdString* lookup)
{
    AD_ASSERT(string);
    AD_ASSERT(lookup);

    int string_count = ad_string_get_count(string);
    int lookup_count = ad_string_get_count(lookup);

    if(lookup_count > string_count)
    {
        return false;
    }
    else
    {
        const char* string_contents = ad_string_get_contents_const(string);
        const char* lookup_contents = ad_string_get_contents_const(lookup);
        return memory_matches(string_contents, lookup_contents, lookup_count);
    }
}

AdMaybeString ad_string_substring(const AdString* string,
        const AdStringRange* range)
{
    AD_ASSERT(string);
    AD_ASSERT(range);
    AD_ASSERT(ad_string_range_check(string, range));

    const char* contents = ad_string_get_contents_const(string);
    const char* buffer = &contents[range->start];
    int bytes = range->end - range->start;
    return ad_string_from_buffer_with_allocator(buffer, bytes,
            string->allocator);
}

char* ad_string_to_c_string(const AdString* string)
{
    return ad_string_to_c_string_with_allocator(string, NULL);
}

char* ad_string_to_c_string_with_allocator(const AdString* string,
        void* allocator)
{
    AD_ASSERT(string);

    int count = ad_string_get_count(string);
    AdMemoryBlock block = ad_string_allocate(allocator, count + 1);
    char* result = block.memory;

    if(!result)
    {
        return NULL;
    }

    const char* contents = ad_string_get_contents_const(string);
    copy_memory(result, contents, count);
    result[count] = '\0';
    return result;
}


bool ad_string_range_check(const AdString* string, const AdStringRange* range)
{
    int count = ad_string_get_count(string);

    return range->start <= range->end
            && range->start > 0
            && range->end > 0
            && range->end < count;
}


bool ad_strings_match(const AdString* a, const AdString* b)
{
    AD_ASSERT(a);
    AD_ASSERT(b);

    int a_count = ad_string_get_count(a);
    int b_count = ad_string_get_count(b);

    if(a_count != b_count)
    {
        return false;
    }

    const char* a_contents = ad_string_get_contents_const(a);
    const char* b_contents = ad_string_get_contents_const(b);

    for(int char_index = 0; char_index < a_count; char_index += 1)
    {
        if(a_contents[char_index] != b_contents[char_index])
        {
            return false;
        }
    }

    return true;
}


bool ad_utf32_destroy(AdUtf32String* string)
{
    return ad_utf32_destroy_with_allocator(string, NULL);
}

bool ad_utf32_destroy_with_allocator(AdUtf32String* string, void* allocator)
{
    AdMemoryBlock block =
    {
        .memory = string->contents,
        .bytes = sizeof(char32_t) * string->count,
    };
    return ad_string_deallocate(allocator, block);
}

AdMaybeString ad_utf32_to_utf8(const AdUtf32String* string)
{
    return ad_utf32_to_utf8_with_allocator(string, NULL);
}

AdMaybeString ad_utf32_to_utf8_with_allocator(const AdUtf32String* string,
        void* allocator)
{
    AD_ASSERT(string);

    AdMaybeString result;
    ad_string_initialise_with_allocator(&result.value, allocator);
    result.valid = true;

    int char_index = 0;

    for(int code_index = 0; code_index < string->count; code_index += 1)
    {
        char32_t codepoint = string->contents[code_index];
        char buffer[5];

        if(codepoint < 0x80)
        {
            buffer[0] = (char) codepoint;
            buffer[1] = '\0';
        }
        else if(codepoint < 0x800)
        {
            buffer[0] = (codepoint >> 6) | 0xc0;
            buffer[1] = (codepoint & 0x3f) | 0x80;
            buffer[2] = '\0';
        }
        else if(codepoint < 0x10000)
        {
            buffer[0] = (codepoint >> 12) | 0xe0;
            buffer[1] = ((codepoint >> 6) & 0x3f) | 0x80;
            buffer[2] = (codepoint & 0x3f) | 0x80;
            buffer[3] = '\0';
        }
        else if(codepoint < 0x110000)
        {
            buffer[0] = (codepoint >> 18) | 0xf0;
            buffer[1] = ((codepoint >> 12) & 0x3f) | 0x80;
            buffer[2] = ((codepoint >> 6) & 0x3f) | 0x80;
            buffer[3] = (codepoint & 0x3f) | 0x80;
            buffer[4] = '\0';
        }
        else
        {
            ad_string_destroy(&result.value);
            result.valid = false;
            return result;
        }

        bool appended = ad_string_append_c_string(&result.value, buffer);
        if(!appended)
        {
            ad_string_destroy(&result.value);
            result.valid = false;
            return result;
        }
    }

    return result;
}


bool ad_utf8_check(const AdString* string)
{
    AD_ASSERT(string);

    const uint8_t* contents =
            (const uint8_t*) ad_string_get_contents_const(string);
    int count = ad_string_get_count(string);
    uint8_t state = 0;

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        uint8_t byte = contents[char_index];
        uint8_t type = utf8_decode_type_table[byte];
        state = utf8_decode_state_table[(16 * state) + type];

        if(state == 1)
        {
            return false;
        }
    }

    return state != 0;
}

int ad_utf8_codepoint_count(const AdString* string)
{
    AD_ASSERT(string);

    int count = 0;
    int string_count = ad_string_get_count(string);
    const char* contents = ad_string_get_contents_const(string);

    for(int char_index = 0; char_index < string_count; char_index += 1)
    {
        char c = contents[char_index];
        count += is_heading_byte(c);
    }

    return count;
}

AdMaybeUtf32String ad_utf8_to_utf32(const AdString* string)
{
    AD_ASSERT(string);
    AD_ASSERT(ad_utf8_check(string));

    AdMaybeUtf32String result;
    result.valid = true;

    int count = ad_utf8_codepoint_count(string) + 1;
    int bytes = sizeof(char32_t) * count;
    AdMemoryBlock block = ad_string_allocate(string->allocator, bytes);
    char32_t* result_contents = block.memory;

    if(!result_contents)
    {
        result.valid = false;
        return result;
    }

    result.value.contents = result_contents;
    result.value.count = count;
    result_contents[count - 1] = U'\0';

    uint32_t codepoint = 0;
    uint32_t state = 0;
    const char* string_contents = ad_string_get_contents_const(string);
    int string_count = ad_string_get_count(string);
    int codepoint_index = 0;

    for(int byte_index = 0; byte_index < string_count; byte_index += 1)
    {
        uint32_t byte = (uint8_t) string_contents[byte_index];
        uint32_t type = utf8_decode_type_table[byte];

        if(state)
        {
            codepoint = (byte & 0x3fu) | (codepoint << 6);
        }
        else
        {
            codepoint = (0xff >> type) & byte;
        }

        state = utf8_decode_state_table[(16 * state) + type];

        if(!state)
        {
            result_contents[codepoint_index] = codepoint;
            codepoint_index += 1;
        }
    }

    AD_ASSERT(codepoint_index == count);

    return result;
}


void ad_codepoint_iterator_end(AdCodepointIterator* it)
{
    AD_ASSERT(it);

    it->index = it->range.end;
}

int ad_codepoint_iterator_get_index(AdCodepointIterator* it)
{
    AD_ASSERT(it);

    return it->index;
}

AdString* ad_codepoint_iterator_get_string(AdCodepointIterator* it)
{
    AD_ASSERT(it);

    return it->string;
}

AdMaybeChar32 ad_codepoint_iterator_next(AdCodepointIterator* it)
{
    AD_ASSERT(it);
    AD_ASSERT(it->string);

    if(it->index < it->range.end)
    {
        char32_t codepoint = 0;

        const char* contents = ad_string_get_contents_const(it->string);
        uint32_t state = 0;

        for(int byte_index = it->index;
                byte_index < it->range.end;
                byte_index += 1)
        {
            uint32_t byte = (uint8_t) contents[byte_index];
            uint32_t type = utf8_decode_type_table[byte];

            if(state)
            {
                codepoint = (byte & 0x3fu) | (codepoint << 6);
            }
            else
            {
                codepoint = (0xff >> type) & byte;
            }

            state = utf8_decode_state_table[(16 * state) + type];

            if(!state)
            {
                it->index = byte_index + 1;
                AdMaybeChar32 result = {codepoint, true};
                return result;
            }
        }
    }

    AdMaybeChar32 result = {U'\0', false};
    return result;
}

// This steps back to the prior heading byte before the iterator and decodes
// moving forward. Then sets the iterator to the heading byte index it started
// decoding at.
//
// An alternate idea would be to decode backwards until a heading byte is found.
// This would entail not using the state machine and instead building the
// codepoint without error checking. Then, after, checking for overlong
// encodings, not enough continuation bytes, and validity of the resulting
// codepoint. It might or might not be an improvement. Also, this idea has the
// disadvantage of not being able to determine the byte the error first occurs,
// just that it was somewhere before it finally hit a heading byte. Which the
// state machine does do!
AdMaybeChar32 ad_codepoint_iterator_prior(AdCodepointIterator* it)
{
    AD_ASSERT(it);
    AD_ASSERT(it->string);

    if(it->index - 1 >= it->range.start)
    {
        char32_t codepoint = 0;

        const char* contents = ad_string_get_contents_const(it->string);
        int codepoint_index = it->range.start;

        for(int byte_index = it->index - 1;
                byte_index >= it->range.start;
                byte_index -= 1)
        {
            if(is_heading_byte(contents[byte_index]))
            {
                codepoint_index = byte_index;
                break;
            }
        }

        uint32_t state = 0;

        for(int byte_index = codepoint_index;
                byte_index < it->range.end;
                byte_index += 1)
        {
            uint32_t byte = (uint8_t) contents[byte_index];
            uint32_t type = utf8_decode_type_table[byte];

            if(state)
            {
                codepoint = (byte & 0x3fu) | (codepoint << 6);
            }
            else
            {
                codepoint = (0xff >> type) & byte;
            }

            state = utf8_decode_state_table[(16 * state) + type];

            if(!state)
            {
                it->index = codepoint_index;
                AdMaybeChar32 result = {codepoint, true};
                return result;
            }
        }
    }

    AdMaybeChar32 result = {U'\0', false};
    return result;
}

void ad_codepoint_iterator_set_string(AdCodepointIterator* it, AdString* string)
{
    AD_ASSERT(it);
    AD_ASSERT(string);

    it->string = string;
    it->range.start = 0;
    it->range.end = ad_string_get_count(it->string);
    ad_codepoint_iterator_start(it);
}

void ad_codepoint_iterator_start(AdCodepointIterator* it)
{
    AD_ASSERT(it);

    it->index = it->range.start;
}
