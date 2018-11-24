#ifndef AD_STRING_H_
#define AD_STRING_H_

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>


#define AD_STRING_SMALL_CAP 16
#define AD_USE_CUSTOM_ALLOCATOR


typedef struct AdMemoryBlock
{
    void* memory;
    uint64_t bytes;
} AdMemoryBlock;

typedef struct AdStringBig
{
    char* contents;
    int cap;
} AdStringBig;

typedef struct AdStringSmall
{
    char contents[AD_STRING_SMALL_CAP];
} AdStringSmall;

typedef struct AdString
{
    union
    {
        AdStringBig big;
        AdStringSmall small;
    };
    void* allocator;
    int count;
    uint8_t type;
} AdString;

typedef struct AdStringRange
{
    int start;
    int end;
} AdStringRange;

typedef struct AdUtf32String
{
    char32_t* contents;
    int count;
} AdUtf32String;

typedef struct AdMaybeString
{
    AdString value;
    bool valid;
} AdMaybeString;

typedef struct AdMaybeUint64
{
    uint64_t value;
    bool valid;
} AdMaybeUint64;

typedef struct AdMaybeUtf32String
{
    AdUtf32String value;
    bool valid;
} AdMaybeUtf32String;


bool ad_ascii_check(const AdString* string);
bool ad_ascii_check_range(const AdString* string, const AdStringRange* range);
int ad_ascii_compare_alphabetic(const AdString* a, const AdString* b);
int ad_ascii_digit_to_int(char c);
bool ad_ascii_is_alphabetic(char c);
bool ad_ascii_is_alphanumeric(char c);
bool ad_ascii_is_lowercase(char c);
bool ad_ascii_is_newline(char c);
bool ad_ascii_is_numeric(char c);
bool ad_ascii_is_space_or_tab(char c);
bool ad_ascii_is_uppercase(char c);
bool ad_ascii_is_whitespace(char c);
void ad_ascii_to_lowercase(AdString* string);
char ad_ascii_to_lowercase_char(char c);
void ad_ascii_to_uppercase(AdString* string);
char ad_ascii_to_uppercase_char(char c);
AdMaybeUint64 ad_ascii_uint64_from_string(const AdString* string);
AdMaybeUint64 ad_ascii_uint64_from_string_range(const AdString* string,
        const AdStringRange* range);

bool ad_c_string_deallocate(char* string);
bool ad_c_string_deallocate_with_allocator(void* allocator, char* string);

AdMemoryBlock ad_string_allocate(void* allocator, uint64_t bytes);
bool ad_string_deallocate(void* allocator, AdMemoryBlock block);

bool ad_string_add(AdString* to, const AdString* from, int index);
bool ad_string_append(AdString* to, const AdString* from);
bool ad_string_append_c_string(AdString* to, const char* from);
bool ad_string_assign(AdString* to, const AdString* from);
AdMaybeString ad_string_copy(AdString* string);
bool ad_string_destroy(AdString* string);
bool ad_string_ends_with(const AdString* string, const AdString* lookup);
int ad_string_find_first_char(const AdString* string, char c);
int ad_string_find_first_string(const AdString* string, const AdString* lookup);
int ad_string_find_last_char(const AdString* string, char c);
int ad_string_find_last_string(const AdString* string, const AdString* lookup);
AdMaybeString ad_string_from_buffer(const char* buffer, int bytes);
AdMaybeString ad_string_from_buffer_with_allocator(const char* buffer,
        int bytes, void* allocator);
AdMaybeString ad_string_from_c_string(const char* original);
AdMaybeString ad_string_from_c_string_with_allocator(const char* original,
        void* allocator);
int ad_string_capacity(AdString* string);
char* ad_string_get_contents(AdString* string);
const char* ad_string_get_contents_const(const AdString* string);
int ad_string_count(AdString* string);
void ad_string_initialise(AdString* string);
void ad_string_initialise_with_allocator(AdString* string, void* allocator);
void ad_string_remove(AdString* string, const AdStringRange* range);
bool ad_string_replace(AdString* to, const AdStringRange* range,
        const AdString* from);
bool ad_string_reserve(AdString* string, int count);
bool ad_string_starts_with(const AdString* string, const AdString* lookup);
AdMaybeString ad_string_substring(const AdString* string,
        const AdStringRange* range);
char* ad_string_to_c_string(const AdString* string);
char* ad_string_to_c_string_with_allocator(const AdString* string,
        void* allocator);

bool ad_string_range_check(const AdString* string, const AdStringRange* range);

bool ad_strings_match(const AdString* a, const AdString* b);

bool ad_utf32_destroy(AdUtf32String* string);
bool ad_utf32_destroy_with_allocator(AdUtf32String* string, void* allocator);
AdMaybeString ad_utf32_to_utf8(const AdUtf32String* string);
AdMaybeString ad_utf32_to_utf8_with_allocator(const AdUtf32String* string,
        void* allocator);

bool ad_utf8_check(const AdString* string);
int ad_utf8_codepoint_count(const AdString* string);
AdMaybeUtf32String ad_utf8_to_utf32(const AdString* string);

#endif // AD_STRING_H_
