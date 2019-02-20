#ifndef AFT_STRING_H_
#define AFT_STRING_H_

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>


#if defined(__cplusplus)
extern "C" {
#endif


#define AFT_STRING_SMALL_CAP sizeof(AftStringBig)

#if !defined(NDEBUG)
#define AFT_CHECK_CORRUPTION
#endif // !defined(NDEBUG)


typedef struct AftMemoryBlock
{
    void* memory;
    uint64_t bytes;
} AftMemoryBlock;

typedef struct AftStringBig
{
    char* contents;
    int count;
} AftStringBig;

typedef struct AftStringSmall
{
    char contents[AFT_STRING_SMALL_CAP - 1];

    // The count is stored as "bytes left" so that it can serve a dual-purpose
    // as the null terminator when the bytes left are zero.
    char bytes_left;
} AftStringSmall;

typedef struct AftString
{
#if defined(AFT_CHECK_CORRUPTION)
    uint32_t canary_start;
#endif
    union
    {
        AftStringBig big;
        AftStringSmall small;
    };
#if defined(AFT_CHECK_CORRUPTION)
    uint32_t canary_end;
#endif
    void* allocator;
    int cap;
} AftString;

typedef struct AftStringSlice
{
    const char* contents;
    int count;
} AftStringSlice;

typedef struct AftUtf32String
{
    char32_t* contents;
    int count;
} AftUtf32String;

typedef struct AftCodepointIterator
{
    AftStringSlice slice;
    int end;
    int index;
    int start;
} AftCodepointIterator;

typedef struct AftMaybeChar32
{
    char32_t value;
    bool valid;
} AftMaybeChar32;

typedef struct AftMaybeDouble
{
    double value;
    bool valid;
} AftMaybeDouble;

typedef struct AftMaybeInt
{
    int value;
    bool valid;
} AftMaybeInt;

typedef struct AftMaybeString
{
    AftString value;
    bool valid;
} AftMaybeString;

typedef struct AftMaybeUint64
{
    uint64_t value;
    bool valid;
} AftMaybeUint64;

typedef struct AftMaybeUtf32String
{
    AftUtf32String value;
    bool valid;
} AftMaybeUtf32String;


bool aft_ascii_check(AftStringSlice slice);
int aft_ascii_compare_alphabetic(const AftString* a, const AftString* b);
int aft_ascii_digit_to_int(char c);
bool aft_ascii_is_alphabetic(char c);
bool aft_ascii_is_alphanumeric(char c);
bool aft_ascii_is_lowercase(char c);
bool aft_ascii_is_newline(char c);
bool aft_ascii_is_numeric(char c);
bool aft_ascii_is_space_or_tab(char c);
bool aft_ascii_is_uppercase(char c);
bool aft_ascii_is_whitespace(char c);
void aft_ascii_reverse(AftString* string);
void aft_ascii_reverse_range(AftString* string, int start, int end);
void aft_ascii_to_lowercase(AftString* string);
char aft_ascii_to_lowercase_char(char c);
void aft_ascii_to_uppercase(AftString* string);
char aft_ascii_to_uppercase_char(char c);

char* aft_c_string_copy_string(const AftString* string);
char* aft_c_string_copy_string_with_allocator(const AftString* string, void* allocator);
bool aft_c_string_deallocate(char* string);
bool aft_c_string_deallocate_with_allocator(void* allocator, char* string);

void aft_codepoint_iterator_end(AftCodepointIterator* it);
int aft_codepoint_iterator_get_index(AftCodepointIterator* it);
AftStringSlice aft_codepoint_iterator_get_string(AftCodepointIterator* it);
AftMaybeChar32 aft_codepoint_iterator_next(AftCodepointIterator* it);
AftMaybeChar32 aft_codepoint_iterator_prior(AftCodepointIterator* it);
void aft_codepoint_iterator_set_string(AftCodepointIterator* it, AftStringSlice slice);
void aft_codepoint_iterator_start(AftCodepointIterator* it);

AftMemoryBlock aft_allocate(void* allocator, uint64_t bytes);
bool aft_deallocate(void* allocator, AftMemoryBlock block);

bool aft_string_add(AftString* to, AftStringSlice from, int index);
bool aft_string_append(AftString* to, const AftString* from);
bool aft_string_append_c_string(AftString* to, const char* from);
bool aft_string_append_char(AftString* to, char from);
bool aft_string_append_slice(AftString* to, AftStringSlice from);
bool aft_string_assign(AftString* to, const AftString* from);
AftMaybeString aft_string_copy(AftString* string);
AftMaybeString aft_string_copy_with_allocator(AftString* string, void* allocator);
AftMaybeString aft_string_copy_c_string(const char* original);
AftMaybeString aft_string_copy_c_string_with_allocator(const char* original, void* allocator);
AftMaybeString aft_string_copy_slice(AftStringSlice slice);
AftMaybeString aft_string_copy_slice_with_allocator(AftStringSlice slice, void* allocator);
bool aft_string_destroy(AftString* string);
int aft_string_get_capacity(const AftString* string);
char* aft_string_get_contents(AftString* string);
const char* aft_string_get_contents_const(const AftString* string);
int aft_string_get_count(const AftString* string);
void aft_string_initialise(AftString* string);
void aft_string_initialise_with_allocator(AftString* string, void* allocator);
void aft_string_remove(AftString* string, int start, int end);
bool aft_string_replace(AftString* to, int start, int end, AftStringSlice from);
bool aft_string_reserve(AftString* string, int count);

bool aft_string_range_check(const AftString* string, int start, int end);

AftStringSlice aft_string_slice(AftStringSlice slice, int start, int end);
int aft_string_slice_count(AftStringSlice slice);
const char* aft_string_slice_end(AftStringSlice slice);
bool aft_string_slice_ends_with(AftStringSlice slice, AftStringSlice lookup);
AftMaybeInt aft_string_slice_find_first_char(AftStringSlice string, char c);
AftMaybeInt aft_string_slice_find_first_string(AftStringSlice string, AftStringSlice lookup);
AftMaybeInt aft_string_slice_find_last_char(AftStringSlice string, char c);
AftMaybeInt aft_string_slice_find_last_string(AftStringSlice string, AftStringSlice lookup);
AftStringSlice aft_string_slice_from_buffer(const char* contents, int count);
AftStringSlice aft_string_slice_from_c_string(const char* contents);
AftStringSlice aft_string_slice_from_string(const AftString* string);
bool aft_string_slice_in_string(AftStringSlice slice, const AftString* string);
bool aft_string_slice_matches(AftStringSlice a, AftStringSlice b);
void aft_string_slice_remove_end(AftStringSlice* slice, int count);
void aft_string_slice_remove_start(AftStringSlice* slice, int count);
const char* aft_string_slice_start(AftStringSlice slice);
bool aft_string_slice_starts_with(AftStringSlice slice, AftStringSlice lookup);
AftStringSlice aft_string_slice_string(const AftString* string, int start, int end);

bool aft_strings_match(const AftString* a, const AftString* b);

bool aft_utf32_destroy(AftUtf32String* string);
bool aft_utf32_destroy_with_allocator(AftUtf32String* string, void* allocator);
AftMaybeString aft_utf32_to_utf8(const AftUtf32String* string);
AftMaybeString aft_utf32_to_utf8_with_allocator(const AftUtf32String* string, void* allocator);

bool aft_utf8_append_codepoint(AftString* string, char32_t codepoint);
bool aft_utf8_check(const AftString* string);
int aft_utf8_codepoint_count(const AftString* string);
AftMaybeUtf32String aft_utf8_to_utf32(const AftString* string);


#include <AftString/aft_number_format.h>


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // AFT_STRING_H_
