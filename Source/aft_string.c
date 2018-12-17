#include <AftString/aft_string.h>

#include <assert.h>
#include <stddef.h>

#define AFT_ASSERT(expression) \
    assert(expression)

#define UINT64_MAX_DIGITS 20


#if !defined(AFT_USE_CUSTOM_ALLOCATOR)

#include <stdlib.h>

AftMemoryBlock aft_allocate(void* allocator, uint64_t bytes)
{
    (void) allocator;
    AftMemoryBlock block =
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

bool aft_deallocate(void* allocator, AftMemoryBlock block)
{
    (void) allocator;
    free(block.memory);
    return true;
}

#endif // !defined(AFT_USE_CUSTOM_ALLOCATOR)


static const uint32_t canary_start = 0x9573cea9;
static const uint32_t canary_end = 0x5f33ccfa;

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


static bool aft_string_check_uncorrupted(const AftString* string)
{
#if defined(AFT_CHECK_CORRUPTION)
    return string->canary_start == canary_start
            && string->canary_end == canary_end;
#else
    return true;
#endif // defined(AFT_CHECK_CORRUPTION)
}

static bool aft_string_is_big(const AftString* string)
{
    return string->cap > AFT_STRING_SMALL_CAP;
}

static void aft_string_set_count(AftString* string, int count)
{
    if(aft_string_is_big(string))
    {
        string->big.count = count;
    }
    else
    {
        string->small.bytes_left = AFT_STRING_SMALL_CAP - count;
    }
}

static void aft_string_set_uncorrupted(AftString* string)
{
#if defined(AFT_CHECK_CORRUPTION)
    string->canary_start = canary_start;
    string->canary_end = canary_end;
#endif // defined(AFT_CHECK_CORRUPTION)
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
    AFT_ASSERT(a);
    AFT_ASSERT(b);

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


bool aft_ascii_check(const AftString* string)
{
    int count = aft_string_get_count(string);
    AftStringRange range = {0, count};
    return aft_ascii_check_range(string, &range);
}

bool aft_ascii_check_range(const AftString* string, const AftStringRange* range)
{
    AFT_ASSERT(string);
    AFT_ASSERT(range);

    const char* contents = aft_string_get_contents_const(string);

    for(int char_index = range->start; char_index < range->end; char_index += 1)
    {
        if(contents[char_index] & 0x80)
        {
            return false;
        }
    }

    return true;
}

int aft_ascii_compare_alphabetic(const AftString* a, const AftString* b)
{
    AFT_ASSERT(a);
    AFT_ASSERT(b);

    const char* a_contents = aft_string_get_contents_const(a);
    const char* b_contents = aft_string_get_contents_const(b);
    int a_count = aft_string_get_count(a);
    int b_count = aft_string_get_count(b);

    for(int char_index = 0; char_index < a_count; char_index += 1)
    {
        char c0 = aft_ascii_to_uppercase_char(a_contents[char_index]);
        char c1 = aft_ascii_to_uppercase_char(b_contents[char_index]);

        if(c0 != c1)
        {
            return c0 - c1;
        }
    }

    return a_count - b_count;
}

int aft_ascii_digit_to_int(char c)
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

bool aft_ascii_is_alphabetic(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool aft_ascii_is_alphanumeric(char c)
{
    return aft_ascii_is_alphabetic(c) || aft_ascii_is_numeric(c);
}

bool aft_ascii_is_lowercase(char c)
{
    return c >= 'a' && c <= 'z';
}

bool aft_ascii_is_newline(char c)
{
    return c >= '\n' && c <= '\r';
}

bool aft_ascii_is_numeric(char c)
{
    return c >= '0' && c <= '9';
}

bool aft_ascii_is_space_or_tab(char c)
{
    return c == ' ' || c == '\t';
}

bool aft_ascii_is_uppercase(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool aft_ascii_is_whitespace(char c)
{
    return c == ' ' || c - 9 <= 5;
}

void aft_ascii_reverse(AftString* string)
{
    AftStringRange range = {0, aft_string_get_count(string)};
    aft_ascii_reverse_range(string, &range);
}

void aft_ascii_reverse_range(AftString* string, const AftStringRange* range)
{
    AFT_ASSERT(string);

    char* contents = aft_string_get_contents(string);

    for(int start = range->start, end = range->end - 1;
            start < end;
            start += 1, end -= 1)
    {
        char temp = contents[start];
        contents[start] = contents[end];
        contents[end] = temp;
    }
}

void aft_ascii_to_lowercase(AftString* string)
{
    AFT_ASSERT(string);

    char* contents = aft_string_get_contents(string);
    int count = aft_string_get_count(string);

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        contents[char_index] =
                aft_ascii_to_lowercase_char(contents[char_index]);
    }
}

char aft_ascii_to_lowercase_char(char c)
{
    if(aft_ascii_is_uppercase(c))
    {
        return 'A' + (c - 'a');
    }
    else
    {
        return c;
    }
}

void aft_ascii_to_uppercase(AftString* string)
{
    AFT_ASSERT(string);

    char* contents = aft_string_get_contents(string);
    int count = aft_string_get_count(string);

    for(int char_index = 0; char_index < count; char_index += 1)
    {
        contents[char_index] =
                aft_ascii_to_uppercase_char(contents[char_index]);
    }
}

char aft_ascii_to_uppercase_char(char c)
{
    if(aft_ascii_is_lowercase(c))
    {
        return 'A' + (c - 'a');
    }
    else
    {
        return c;
    }
}

AftMaybeUint64 aft_ascii_uint64_from_string(const AftString* string)
{
    int count = aft_string_get_count(string);
    AftStringRange range = {0, count};
    return aft_ascii_uint64_from_string_range(string, &range);
}

AftMaybeUint64 aft_ascii_uint64_from_string_range(const AftString* string,
        const AftStringRange* range)
{
    AFT_ASSERT(string);
    AFT_ASSERT(range);
    AFT_ASSERT(aft_string_range_check(string, range));
    AFT_ASSERT(aft_ascii_check_range(string, range));

    AftMaybeUint64 result;
    result.valid = true;
    result.value = 0;

    const char* contents = aft_string_get_contents_const(string);

    int count = range->end - range->start;
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


bool aft_c_string_deallocate(char* string)
{
    return aft_c_string_deallocate_with_allocator(NULL, string);
}

bool aft_c_string_deallocate_with_allocator(void* allocator, char* string)
{
    AftMemoryBlock block =
    {
        .memory = string,
        .bytes = string_size(string) + 1,
    };
    return aft_deallocate(allocator, block);
}


bool aft_string_add(AftString* to, const AftString* from, int index)
{
    int to_count = aft_string_get_count(to);
    int from_count = aft_string_get_count(from);

    AFT_ASSERT(to);
    AFT_ASSERT(from);
    AFT_ASSERT(index >= 0 && index <= to_count);

    int count = to_count + from_count;
    bool reserved = aft_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    aft_string_set_count(to, count);
    char* to_contents = aft_string_get_contents(to);
    const char* from_contents = aft_string_get_contents_const(from);
    int shift_bytes = prior_count - index;
    if(shift_bytes > 0)
    {
        copy_memory(&to_contents[index + from_count], &to_contents[index],
                shift_bytes);
    }
    copy_memory(&to_contents[index], from_contents, from_count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(to));

    return true;
}

bool aft_string_append(AftString* to, const AftString* from)
{
    AFT_ASSERT(to);
    AFT_ASSERT(from);

    int to_count = aft_string_get_count(to);
    int from_count = aft_string_get_count(from);
    int count = to_count + from_count;
    bool reserved = aft_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    aft_string_set_count(to, count);
    char* to_contents = aft_string_get_contents(to);
    const char* from_contents = aft_string_get_contents_const(from);
    copy_memory(&to_contents[prior_count], from_contents, from_count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(to));

    return true;
}

bool aft_string_append_c_string(AftString* to, const char* from)
{
    AFT_ASSERT(to);
    AFT_ASSERT(from);

    int from_count = string_size(from);
    int to_count = aft_string_get_count(to);
    int count = to_count + from_count;
    bool reserved = aft_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    int prior_count = to_count;
    aft_string_set_count(to, count);
    char* to_contents = aft_string_get_contents(to);
    copy_memory(&to_contents[prior_count], from, from_count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(to));

    return true;
}

bool aft_string_assign(AftString* to, const AftString* from)
{
    AFT_ASSERT(to);
    AFT_ASSERT(from);

    int from_count = aft_string_get_count(from);
    bool reserved = aft_string_reserve(to, from_count);

    if(!reserved)
    {
        return false;
    }

    int count = from_count;
    aft_string_set_count(to, count);
    char* to_contents = aft_string_get_contents(to);
    const char* from_contents = aft_string_get_contents_const(from);
    copy_memory(to_contents, from_contents, count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(to));

    return true;
}

AftMaybeString aft_string_copy(AftString* string)
{
    AFT_ASSERT(string);

    int count = aft_string_get_count(string);

    AftMaybeString result;
    result.valid = true;
    result.value.allocator = string->allocator;
    result.value.cap = AFT_STRING_SMALL_CAP;
    aft_string_set_count(&result.value, count);
    aft_string_set_uncorrupted(&result.value);
    bool reserved = aft_string_reserve(&result.value, count);

    if(!reserved)
    {
        result.valid = false;
        zero_memory(&result.value, sizeof(result.value));
        return result;
    }

    char* to_contents = aft_string_get_contents(&result.value);
    const char* from_contents = aft_string_get_contents_const(string);
    copy_memory(to_contents, from_contents, count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(&result.value));

    return result;
}

bool aft_string_destroy(AftString* string)
{
    AFT_ASSERT(string);

    bool result = true;

    if(aft_string_is_big(string))
    {
        AftMemoryBlock block =
        {
            .memory = string->big.contents,
            .bytes = string->cap,
        };
        result = aft_deallocate(string->allocator, block);
    }

    aft_string_initialise_with_allocator(string, string->allocator);

    return result;
}

bool aft_string_ends_with(const AftString* string, const AftString* lookup)
{
    AFT_ASSERT(string);
    AFT_ASSERT(lookup);

    int lookup_count = aft_string_get_count(lookup);
    int string_count = aft_string_get_count(string);

    if(lookup_count > string_count)
    {
        return false;
    }
    else
    {
        const char* string_contents = aft_string_get_contents_const(string);
        const char* lookup_contents = aft_string_get_contents_const(lookup);
        const char* near_end = &string_contents[string_count - lookup_count];
        return memory_matches(near_end, lookup_contents, lookup_count);
    }
}

AftMaybeInt aft_string_find_first_char(const AftString* string, char c)
{
    AFT_ASSERT(string);

    AftMaybeInt result;

    const char* contents = aft_string_get_contents_const(string);
    int count = aft_string_get_count(string);

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

AftMaybeInt aft_string_find_first_string(const AftString* string,
        const AftString* lookup)
{
    AFT_ASSERT(string);
    AFT_ASSERT(lookup);

    AftMaybeInt result;

    const char* string_contents = aft_string_get_contents_const(string);
    const char* lookup_contents = aft_string_get_contents_const(lookup);

    int string_count = aft_string_get_count(string);
    int lookup_count = aft_string_get_count(lookup);
    int search_count = string_count - lookup_count + 1;

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

AftMaybeInt aft_string_find_last_char(const AftString* string, char c)
{
    AFT_ASSERT(string);

    AftMaybeInt result;

    const char* contents = aft_string_get_contents_const(string);
    int count = aft_string_get_count(string);

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

AftMaybeInt aft_string_find_last_string(const AftString* string,
        const AftString* lookup)
{
    AFT_ASSERT(string);
    AFT_ASSERT(lookup);

    AftMaybeInt result;

    const char* string_contents = aft_string_get_contents_const(string);
    const char* lookup_contents = aft_string_get_contents_const(lookup);

    int string_count = aft_string_get_count(string);
    int lookup_count = aft_string_get_count(lookup);
    int search_count = string_count - lookup_count + 1;

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

AftMaybeString aft_string_from_buffer(const char* buffer, int bytes)
{
    return aft_string_from_buffer_with_allocator(buffer, bytes, NULL);
}

AftMaybeString aft_string_from_buffer_with_allocator(const char* buffer,
        int bytes, void* allocator)
{
    int cap = bytes + 1;

    AftMaybeString result;
    result.valid = true;
    result.value.allocator = allocator;
    aft_string_set_uncorrupted(&result.value);

    if(cap > AFT_STRING_SMALL_CAP)
    {
        AftMemoryBlock block = aft_allocate(allocator, cap);
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
        aft_string_set_count(&result.value, bytes);
    }
    else
    {
        copy_memory(result.value.small.contents, buffer, bytes);
        result.value.small.contents[bytes] = '\0';
        result.value.cap = AFT_STRING_SMALL_CAP;
        aft_string_set_count(&result.value, bytes);
    }

    return result;
}

AftMaybeString aft_string_from_c_string(const char* original)
{
    return aft_string_from_c_string_with_allocator(original, NULL);
}

AftMaybeString aft_string_from_c_string_with_allocator(const char* original,
        void* allocator)
{
    int bytes = string_size(original);
    int cap = bytes + 1;

    AftMaybeString result;
    result.valid = true;
    result.value.allocator = allocator;
    aft_string_set_uncorrupted(&result.value);

    if(cap > AFT_STRING_SMALL_CAP)
    {
        AftMemoryBlock block = aft_allocate(allocator, cap);
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
        aft_string_set_count(&result.value, bytes);
    }
    else
    {
        copy_memory(result.value.small.contents, original, bytes);
        result.value.small.contents[bytes] = '\0';
        result.value.cap = AFT_STRING_SMALL_CAP;
        aft_string_set_count(&result.value, bytes);
    }

    return result;
}

int aft_string_get_capacity(const AftString* string)
{
    AFT_ASSERT(string);

    return string->cap - 1;
}

char* aft_string_get_contents(AftString* string)
{
    AFT_ASSERT(string);

    if(aft_string_is_big(string))
    {
        AFT_ASSERT(string->big.contents);
        return string->big.contents;
    }
    else
    {
        AFT_ASSERT(string->small.contents);
        return string->small.contents;
    }
}

const char* aft_string_get_contents_const(const AftString* string)
{
    AFT_ASSERT(string);

    if(aft_string_is_big(string))
    {
        AFT_ASSERT(string->big.contents);
        return string->big.contents;
    }
    else
    {
        AFT_ASSERT(string->small.contents);
        return string->small.contents;
    }
}

int aft_string_get_count(const AftString* string)
{
    AFT_ASSERT(string);

    if(aft_string_is_big(string))
    {
        return string->big.count;
    }
    else
    {
        return AFT_STRING_SMALL_CAP - string->small.bytes_left;
    }
}

void aft_string_initialise(AftString* string)
{
    aft_string_initialise_with_allocator(string, NULL);
}

void aft_string_initialise_with_allocator(AftString* string, void* allocator)
{
    AFT_ASSERT(string);

    string->allocator = allocator;
    string->cap = AFT_STRING_SMALL_CAP;
    aft_string_set_count(string, 0);
    zero_memory(string->small.contents, AFT_STRING_SMALL_CAP - 1);
    aft_string_set_uncorrupted(string);
}

void aft_string_remove(AftString* string, const AftStringRange* range)
{
    AFT_ASSERT(string);
    AFT_ASSERT(range);
    AFT_ASSERT(aft_string_range_check(string, range));

    int count = aft_string_get_count(string);
    int copied_bytes = count - range->end;
    int removed_bytes = range->end - range->start;

    char* contents = aft_string_get_contents(string);
    copy_memory(&contents[range->start], &contents[range->end], copied_bytes);
    count -= removed_bytes;
    aft_string_set_count(string, count);
    contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(string));
}

bool aft_string_replace(AftString* to, const AftStringRange* range,
        const AftString* from)
{
    AFT_ASSERT(to);
    AFT_ASSERT(range);
    AFT_ASSERT(from);
    AFT_ASSERT(aft_string_range_check(to, range));

    int view_bytes = range->end - range->start;
    int to_count = aft_string_get_count(to);
    int from_count = aft_string_get_count(from);
    int count = to_count - view_bytes + from_count;

    bool reserved = aft_string_reserve(to, count);

    if(!reserved)
    {
        return false;
    }

    char* to_contents = aft_string_get_contents(to);
    const char* from_contents = aft_string_get_contents_const(from);

    int inserted_end = range->start + from_count;
    int moved_bytes = to_count - range->end;
    copy_memory(&to_contents[inserted_end], &to_contents[range->end],
            moved_bytes);

    copy_memory(&to_contents[range->start], from_contents, from_count);
    aft_string_set_count(to, count);
    to_contents[count] = '\0';

    AFT_ASSERT(aft_string_check_uncorrupted(to));

    return true;
}

bool aft_string_reserve(AftString* string, int space)
{
    AFT_ASSERT(string);
    AFT_ASSERT(space >= 0);

    int needed_cap = space + 1;
    int existing_cap = aft_string_get_capacity(string);

    if(needed_cap > existing_cap)
    {
        int prior_cap = existing_cap;
        int cap = int_max(2 * prior_cap, needed_cap);
        AftMemoryBlock block = aft_allocate(string->allocator, cap);
        char* contents = block.memory;

        if(!contents)
        {
            return false;
        }

        int count = aft_string_get_count(string);

        if(count)
        {
            if(prior_cap > AFT_STRING_SMALL_CAP)
            {
                char* prior_contents = string->big.contents;
                copy_memory(contents, prior_contents, count);
                AftMemoryBlock block =
                {
                    .memory = prior_contents,
                    .bytes = string->cap,
                };
                aft_deallocate(string->allocator, block);
            }
            else
            {
                char* prior_contents = string->small.contents;
                copy_memory(contents, prior_contents, count);
                zero_memory(prior_contents, prior_cap);
            }
        }

        AFT_ASSERT(cap > AFT_STRING_SMALL_CAP);
        string->big.contents = contents;
        string->cap = cap;
    }

    AFT_ASSERT(aft_string_check_uncorrupted(string));

    return true;
}

bool aft_string_starts_with(const AftString* string, const AftString* lookup)
{
    AFT_ASSERT(string);
    AFT_ASSERT(lookup);

    int string_count = aft_string_get_count(string);
    int lookup_count = aft_string_get_count(lookup);

    if(lookup_count > string_count)
    {
        return false;
    }
    else
    {
        const char* string_contents = aft_string_get_contents_const(string);
        const char* lookup_contents = aft_string_get_contents_const(lookup);
        return memory_matches(string_contents, lookup_contents, lookup_count);
    }
}

AftMaybeString aft_string_substring(const AftString* string,
        const AftStringRange* range)
{
    AFT_ASSERT(string);
    AFT_ASSERT(range);
    AFT_ASSERT(aft_string_range_check(string, range));

    const char* contents = aft_string_get_contents_const(string);
    const char* buffer = &contents[range->start];
    int bytes = range->end - range->start;
    return aft_string_from_buffer_with_allocator(buffer, bytes,
            string->allocator);
}

char* aft_string_to_c_string(const AftString* string)
{
    return aft_string_to_c_string_with_allocator(string, NULL);
}

char* aft_string_to_c_string_with_allocator(const AftString* string,
        void* allocator)
{
    AFT_ASSERT(string);

    int count = aft_string_get_count(string);
    AftMemoryBlock block = aft_allocate(allocator, count + 1);
    char* result = block.memory;

    if(!result)
    {
        return NULL;
    }

    const char* contents = aft_string_get_contents_const(string);
    copy_memory(result, contents, count);
    result[count] = '\0';
    return result;
}


bool aft_string_range_check(const AftString* string,
        const AftStringRange* range)
{
    int count = aft_string_get_count(string);

    return range->start <= range->end
            && range->start >= 0
            && range->end >= 0
            && range->end <= count;
}


bool aft_strings_match(const AftString* a, const AftString* b)
{
    AFT_ASSERT(a);
    AFT_ASSERT(b);

    int a_count = aft_string_get_count(a);
    int b_count = aft_string_get_count(b);

    if(a_count != b_count)
    {
        return false;
    }

    const char* a_contents = aft_string_get_contents_const(a);
    const char* b_contents = aft_string_get_contents_const(b);

    for(int char_index = 0; char_index < a_count; char_index += 1)
    {
        if(a_contents[char_index] != b_contents[char_index])
        {
            return false;
        }
    }

    return true;
}


bool aft_utf32_destroy(AftUtf32String* string)
{
    return aft_utf32_destroy_with_allocator(string, NULL);
}

bool aft_utf32_destroy_with_allocator(AftUtf32String* string, void* allocator)
{
    AftMemoryBlock block =
    {
        .memory = string->contents,
        .bytes = sizeof(char32_t) * string->count,
    };
    return aft_deallocate(allocator, block);
}

AftMaybeString aft_utf32_to_utf8(const AftUtf32String* string)
{
    return aft_utf32_to_utf8_with_allocator(string, NULL);
}

AftMaybeString aft_utf32_to_utf8_with_allocator(const AftUtf32String* string,
        void* allocator)
{
    AFT_ASSERT(string);

    AftMaybeString result;
    aft_string_initialise_with_allocator(&result.value, allocator);
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
            aft_string_destroy(&result.value);
            result.valid = false;
            return result;
        }

        bool appended = aft_string_append_c_string(&result.value, buffer);
        if(!appended)
        {
            aft_string_destroy(&result.value);
            result.valid = false;
            return result;
        }
    }

    return result;
}


bool aft_utf8_check(const AftString* string)
{
    AFT_ASSERT(string);

    const uint8_t* contents =
            (const uint8_t*) aft_string_get_contents_const(string);
    int count = aft_string_get_count(string);
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

int aft_utf8_codepoint_count(const AftString* string)
{
    AFT_ASSERT(string);

    int count = 0;
    int string_count = aft_string_get_count(string);
    const char* contents = aft_string_get_contents_const(string);

    for(int char_index = 0; char_index < string_count; char_index += 1)
    {
        char c = contents[char_index];
        count += is_heading_byte(c);
    }

    return count;
}

AftMaybeUtf32String aft_utf8_to_utf32(const AftString* string)
{
    AFT_ASSERT(string);
    AFT_ASSERT(aft_utf8_check(string));

    AftMaybeUtf32String result;
    result.valid = true;

    int count = aft_utf8_codepoint_count(string) + 1;
    int bytes = sizeof(char32_t) * count;
    AftMemoryBlock block = aft_allocate(string->allocator, bytes);
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
    const char* string_contents = aft_string_get_contents_const(string);
    int string_count = aft_string_get_count(string);
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

    AFT_ASSERT(codepoint_index == count);

    return result;
}


void aft_codepoint_iterator_end(AftCodepointIterator* it)
{
    AFT_ASSERT(it);

    it->index = it->range.end;
}

int aft_codepoint_iterator_get_index(AftCodepointIterator* it)
{
    AFT_ASSERT(it);

    return it->index;
}

AftString* aft_codepoint_iterator_get_string(AftCodepointIterator* it)
{
    AFT_ASSERT(it);

    return it->string;
}

AftMaybeChar32 aft_codepoint_iterator_next(AftCodepointIterator* it)
{
    AFT_ASSERT(it);
    AFT_ASSERT(it->string);

    if(it->index < it->range.end)
    {
        char32_t codepoint = 0;

        const char* contents = aft_string_get_contents_const(it->string);
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
                AftMaybeChar32 result = {codepoint, true};
                return result;
            }
        }
    }

    AftMaybeChar32 result = {U'\0', false};
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
AftMaybeChar32 aft_codepoint_iterator_prior(AftCodepointIterator* it)
{
    AFT_ASSERT(it);
    AFT_ASSERT(it->string);

    if(it->index - 1 >= it->range.start)
    {
        char32_t codepoint = 0;

        const char* contents = aft_string_get_contents_const(it->string);
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
                AftMaybeChar32 result = {codepoint, true};
                return result;
            }
        }
    }

    AftMaybeChar32 result = {U'\0', false};
    return result;
}

void aft_codepoint_iterator_set_string(AftCodepointIterator* it,
        AftString* string)
{
    AFT_ASSERT(it);
    AFT_ASSERT(string);

    it->string = string;
    it->range.start = 0;
    it->range.end = aft_string_get_count(it->string);
    aft_codepoint_iterator_start(it);
}

void aft_codepoint_iterator_start(AftCodepointIterator* it)
{
    AFT_ASSERT(it);

    it->index = it->range.start;
}
