#include "json.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define AFT_ASSERT(expression) assert(expression)


typedef enum TokenKind
{
    TOKEN_KIND_COLON,
    TOKEN_KIND_COMMA,
    TOKEN_KIND_END_OF_SLICE,
    TOKEN_KIND_FALSE,
    TOKEN_KIND_LEFT_BRACE,
    TOKEN_KIND_LEFT_BRACKET,
    TOKEN_KIND_NULL,
    TOKEN_KIND_NUMBER,
    TOKEN_KIND_RIGHT_BRACE,
    TOKEN_KIND_RIGHT_BRACKET,
    TOKEN_KIND_STRING,
    TOKEN_KIND_TRUE,
} TokenKind;


typedef struct Token
{
    AftStringSlice slice;
    TokenKind kind;
} Token;

typedef struct JsonMemoryBlock
{
    void* memory;
    uint64_t bytes;
} JsonMemoryBlock;

typedef struct Lexer
{
    Token token;
    AftStringSlice* slice;
    JsonError error;
} Lexer;

typedef struct Parser
{
    Lexer lexer;
    void* allocator;
    JsonError error;
} Parser;

typedef struct Serializer
{
    AftString* string;
    int indent_level;
    int spaces_per_indent;
} Serializer;


static JsonElement* const object_slot_empty = 0;
static JsonElement* const object_overflow_empty = ((void*) 1);
static const int object_iterator_end_index = -1;


static JsonMemoryBlock json_allocate(void* allocator, uint64_t bytes)
{
    AFT_ASSERT(allocator);

    JsonMemoryBlock block =
    {
        .memory = calloc(bytes, 1),
        .bytes = bytes,
    };

    return block;
}

static bool json_deallocate(void* allocator, JsonMemoryBlock block)
{
    AFT_ASSERT(allocator);

    free(block.memory);

    return true;
}

static uint32_t hexadecimal_digit_to_int(char digit)
{
    if(digit >= '0' && digit <= '9')
    {
        return digit - '0';
    }
    else if(digit >= 'A' && digit <= 'F')
    {
        return digit - 'A' + 10;
    }
    else if(digit >= 'a' && digit <= 'f')
    {
        return digit - 'a' + 10;
    }
    else
    {
        AFT_ASSERT(false);
        return 0;
    }
}

static char32_t four_digit_hex_to_codepoint(const AftStringSlice* slice)
{
    AFT_ASSERT(slice->count == 4);

    const char* contents = aft_string_slice_start(slice);

    uint32_t value = hexadecimal_digit_to_int(contents[0]) << 12
            | hexadecimal_digit_to_int(contents[1]) << 8
            | hexadecimal_digit_to_int(contents[2]) << 4
            | hexadecimal_digit_to_int(contents[3]);

    return (char32_t) value;
}

static bool is_hexadecimal_digit(char c)
{
    return (c >= '0' && c <= '9')
            || (c >= 'A' && c <= 'F')
            || (c >= 'a' && c <= 'f');
}

static bool is_hexadecimal(const char* string, int count)
{
    for(int digit_index = 0;
            digit_index < count;
            digit_index += 1)
    {
        if(!is_hexadecimal_digit(string[digit_index]))
        {
            return false;
        }
    }

    return true;
}

static int string_size(const char* string)
{
    const char* s;
    for(s = string; *s; s += 1);
    return (int) (s - string);
}

static bool has_token_kind(Lexer* lexer, TokenKind kind)
{
    return lexer->token.kind == kind;
}

static bool read_keyword(Lexer* lexer, const char* keyword, TokenKind kind)
{
    int keyword_count = string_size(keyword);

    if(aft_string_slice_count(lexer->slice) < keyword_count)
    {
        lexer->error = JSON_ERROR_INVALID_TOKEN;
        return false;
    }

    AftStringSlice slice = aft_string_slice(lexer->slice, 0, keyword_count);
    AftStringSlice keyword_slice = aft_string_slice_from_buffer(keyword, keyword_count);

    if(!aft_string_slice_matches(&slice, &keyword_slice))
    {
        lexer->error = JSON_ERROR_INVALID_TOKEN;
        return false;
    }

    lexer->token.kind = kind;
    aft_string_slice_remove_start(lexer->slice, keyword_count);

    return true;
}

static bool has_char(Lexer* lexer, char c)
{
    return aft_string_slice_count(lexer->slice) > 1
            && aft_string_slice_start(lexer->slice)[0] == c;
}

static bool read_digits(Lexer* lexer)
{
    bool digit_found = false;

    while(aft_string_slice_count(lexer->slice) != 0)
    {
        const char* start = aft_string_slice_start(lexer->slice);

        if(!aft_ascii_is_numeric(start[0]))
        {
            break;
        }

        digit_found = true;
        aft_string_slice_remove_start(lexer->slice, 1);
    }

    return digit_found;
}

static bool read_number(Lexer* lexer)
{
    if(has_char(lexer, '-'))
    {
        aft_string_slice_remove_start(lexer->slice, 1);
    }

    bool digits_found = read_digits(lexer);
    if(!digits_found)
    {
        lexer->error = JSON_ERROR_INVALID_TOKEN;
        return false;
    }

    if(has_char(lexer, '.'))
    {
        aft_string_slice_remove_start(lexer->slice, 1);

        digits_found = read_digits(lexer);
        if(!digits_found)
        {
            lexer->error = JSON_ERROR_INVALID_TOKEN;
            return false;
        }
    }

    if(has_char(lexer, 'E') || has_char(lexer, 'e'))
    {
        aft_string_slice_remove_start(lexer->slice, 1);

        if(has_char(lexer, '-') || has_char(lexer, '+'))
        {
            aft_string_slice_remove_start(lexer->slice, 1);
        }

        digits_found = read_digits(lexer);
        if(!digits_found)
        {
            lexer->error = JSON_ERROR_INVALID_TOKEN;
            return false;
        }
    }

    lexer->token.kind = TOKEN_KIND_NUMBER;

    return true;
}

static bool read_string(Lexer* lexer)
{
    bool string_ended = false;

    while(!string_ended)
    {
        int slice_left = aft_string_slice_count(lexer->slice);

        if(slice_left == 0)
        {
            lexer->error = JSON_ERROR_INVALID_TOKEN;
            return false;
        }

        const char* start = aft_string_slice_start(lexer->slice);

        switch(start[0])
        {
            case '\x00': case '\x01': case '\x02': case '\x03': case '\x04':
            case '\x05': case '\x06': case '\x07': case '\x08': case '\x09':
            case '\x0a': case '\x0b': case '\x0c': case '\x0d': case '\x0e':
            case '\x0f': case '\x10': case '\x11': case '\x12': case '\x13':
            case '\x14': case '\x15': case '\x16': case '\x17': case '\x18':
            case '\x19': case '\x1a': case '\x1b': case '\x1c': case '\x1d':
            case '\x1e': case '\x1f':
            {
                lexer->error = JSON_ERROR_INVALID_TOKEN;
                return false;
            }
            case '"':
            {
                string_ended = true;
                break;
            }
            case '\\':
            {
                if(slice_left <= 2)
                {
                    lexer->error = JSON_ERROR_INVALID_TOKEN;
                    return false;
                }

                switch(start[1])
                {
                    case '"':
                    case '/':
                    case '\\':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                    {
                        aft_string_slice_remove_start(lexer->slice, 2);
                        break;
                    }
                    case 'u':
                    {
                        if(slice_left <= 6 || !is_hexadecimal(&start[2], 4))
                        {
                            lexer->error = JSON_ERROR_INVALID_TOKEN;
                            return false;
                        }

                        aft_string_slice_remove_start(lexer->slice, 6);
                        break;
                    }
                    default:
                    {
                        lexer->error = JSON_ERROR_INVALID_TOKEN;
                        return false;
                    }
                }
                break;
            }
            default:
            {
                aft_string_slice_remove_start(lexer->slice, 1);
                break;
            }
        }
    }

    lexer->token.kind = TOKEN_KIND_STRING;

    aft_string_slice_remove_start(lexer->slice, 1);

    return true;
}

static void next_token(Lexer* lexer)
{
    bool token_found = false;

    while(!token_found && lexer->error == JSON_ERROR_NONE)
    {
        const char* start = aft_string_slice_start(lexer->slice);

        if(aft_string_slice_count(lexer->slice) == 0)
        {
            lexer->token.kind = TOKEN_KIND_END_OF_SLICE;
            lexer->token.slice = aft_string_slice_from_buffer(start, 0);
            break;
        }

        switch(start[0])
        {
            case '\t':
            case '\n':
            case '\r':
            case ' ':
            {
                aft_string_slice_remove_start(lexer->slice, 1);
                break;
            }
            case '"':
            {
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = read_string(lexer);
                break;
            }
            case ',':
            {
                lexer->token.kind = TOKEN_KIND_COMMA;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            case '-': case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            {
                token_found = read_number(lexer);
                break;
            }
            case ':':
            {
                lexer->token.kind = TOKEN_KIND_COLON;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            case '[':
            {
                lexer->token.kind = TOKEN_KIND_LEFT_BRACKET;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            case ']':
            {
                lexer->token.kind = TOKEN_KIND_RIGHT_BRACKET;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            case 'f':
            {
                token_found = read_keyword(lexer, "false", TOKEN_KIND_FALSE);
                break;
            }
            case 'n':
            {
                token_found = read_keyword(lexer, "null", TOKEN_KIND_NULL);
                break;
            }
            case 't':
            {
                token_found = read_keyword(lexer, "true", TOKEN_KIND_TRUE);
                break;
            }
            case '{':
            {
                lexer->token.kind = TOKEN_KIND_LEFT_BRACE;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            case '}':
            {
                lexer->token.kind = TOKEN_KIND_RIGHT_BRACE;
                aft_string_slice_remove_start(lexer->slice, 1);
                token_found = true;
                break;
            }
            default:
            {
                lexer->error = JSON_ERROR_INVALID_TOKEN;
                break;
            }
        }

        int count = aft_string_slice_start(lexer->slice) - start;
        lexer->token.slice = aft_string_slice_from_buffer(start, count);
    }
}

static void json_element_destroy(JsonElement* element)
{
    switch(element->kind)
    {
        case JSON_ELEMENT_KIND_ARRAY:
        {
            for(int element_index = 0;
                    element_index < element->array.count;
                    element_index += 1)
            {
                json_element_destroy(&element->array.elements[element_index]);
            }

            JsonMemoryBlock block = {element->array.elements, sizeof(JsonElement) * element->array.cap};
            json_deallocate(element->array.allocator, block);
            break;
        }
        case JSON_ELEMENT_KIND_STRING:
        {
            aft_string_destroy(&element->string);
            break;
        }
    }
}

static JsonElement parse_string(Parser* parser);
static JsonElement parse_value(Parser* parser);

static JsonElement parse_array(Parser* parser)
{
    JsonElement element =
    {
        .kind = JSON_ELEMENT_KIND_ARRAY,
        .array =
        {
            .allocator = parser->allocator,
        },
    };

    next_token(&parser->lexer);

    if(!has_token_kind(&parser->lexer, TOKEN_KIND_RIGHT_BRACKET))
    {
        for(;;)
        {
            JsonElement value = parse_value(parser);
            json_array_add(&element, &value);

            if(parser->error != JSON_ERROR_NONE)
            {
                json_element_destroy(&element);
                element.kind = JSON_ELEMENT_KIND_INVALID;
                return element;
            }

            if(has_token_kind(&parser->lexer, TOKEN_KIND_RIGHT_BRACKET))
            {
                break;
            }

            if(!has_token_kind(&parser->lexer, TOKEN_KIND_COMMA))
            {
                parser->error = JSON_ERROR_INVALID_TOKEN;
                json_element_destroy(&element);
                element.kind = JSON_ELEMENT_KIND_INVALID;
                return element;
            }

            next_token(&parser->lexer);
        }
    }

    next_token(&parser->lexer);

    return element;
}

static JsonElement parse_object(Parser* parser)
{
    JsonElement element;
    json_object_create(&element, 0, parser->allocator);

    next_token(&parser->lexer);

    if(!has_token_kind(&parser->lexer, TOKEN_KIND_RIGHT_BRACE))
    {
        for(;;)
        {
            JsonElement key = parse_string(parser);

            if(parser->error != JSON_ERROR_NONE
                    || !has_token_kind(&parser->lexer, TOKEN_KIND_COLON))
            {
                json_element_destroy(&key);
                json_element_destroy(&element);
                element.kind = JSON_ELEMENT_KIND_INVALID;
                return element;
            }

            next_token(&parser->lexer);

            JsonElement value = parse_value(parser);

            if(parser->error != JSON_ERROR_NONE)
            {
                json_element_destroy(&key);
                json_element_destroy(&value);
                json_element_destroy(&element);
                element.kind = JSON_ELEMENT_KIND_INVALID;
                return element;
            }

            json_object_add(&element, &key, &value);

            if(has_token_kind(&parser->lexer, TOKEN_KIND_RIGHT_BRACE))
            {
                break;
            }

            if(!has_token_kind(&parser->lexer, TOKEN_KIND_COMMA))
            {
                parser->error = JSON_ERROR_INVALID_TOKEN;
                json_element_destroy(&element);
                element.kind = JSON_ELEMENT_KIND_INVALID;
                return element;
            }

            next_token(&parser->lexer);
        }
    }

    next_token(&parser->lexer);

    return element;
}

static JsonElement parse_number(Parser* parser)
{
    JsonElement element;
    element.kind = JSON_ELEMENT_KIND_NUMBER;

    AftMaybeDouble result = aft_ascii_to_double(&parser->lexer.token.slice);
    AFT_ASSERT(result.valid);
    element.number = result.value;

    next_token(&parser->lexer);

    return element;
}

static JsonElement parse_string(Parser* parser)
{
    JsonElement element;
    element.kind = JSON_ELEMENT_KIND_STRING;
    aft_string_initialise_with_allocator(&element.string, parser->allocator);

    AftStringSlice slice = parser->lexer.token.slice;
    aft_string_slice_remove_start(&slice, 1);
    aft_string_slice_remove_end(&slice, 1);
    const char* contents = aft_string_slice_start(&slice);
    int count = aft_string_slice_count(&slice);

    for(int char_index = 0;
            char_index < count;
            char_index += 1)
    {
        char character = contents[char_index];

        bool appended = false;

        if(character == '\\')
        {
            char_index += 1;

            AFT_ASSERT(char_index < count);

            switch(contents[char_index])
            {
                case '"':
                {
                    appended = aft_string_append_char(&element.string, '"');
                    break;
                }
                case '/':
                {
                    appended = aft_string_append_char(&element.string, '/');
                    break;
                }
                case '\\':
                {
                    appended = aft_string_append_char(&element.string, '\\');
                    break;
                }
                case 'b':
                {
                    appended = aft_string_append_char(&element.string, '\b');
                    break;
                }
                case 'f':
                {
                    appended = aft_string_append_char(&element.string, '\f');
                    break;
                }
                case 'n':
                {
                    appended = aft_string_append_char(&element.string, '\n');
                    break;
                }
                case 'r':
                {
                    appended = aft_string_append_char(&element.string, '\r');
                    break;
                }
                case 't':
                {
                    appended = aft_string_append_char(&element.string, '\t');
                    break;
                }
                case 'u':
                {
                    char_index += 1;
                    AFT_ASSERT(count - char_index >= 4);
                    AftStringSlice slice = aft_string_slice_from_buffer(&contents[char_index], 4);
                    char32_t codepoint = four_digit_hex_to_codepoint(&slice);
                    appended = aft_utf8_append_codepoint(&element.string, codepoint);
                    char_index += 4;
                    break;
                }
                default:
                {
                    AFT_ASSERT(false);
                    break;
                }
            }
        }
        else
        {
            AFT_ASSERT(character != '"');

            appended = aft_string_append_char(&element.string, character);
        }

        if(!appended)
        {
            parser->error = JSON_ERROR_ALLOCATION_FAILURE;
            break;
        }
    }

    next_token(&parser->lexer);

    return element;
}

static JsonElement parse_value(Parser* parser)
{
    if(parser->lexer.error != JSON_ERROR_NONE)
    {
        parser->error = parser->lexer.error;
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_STRING))
    {
        return parse_string(parser);
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_LEFT_BRACE))
    {
        return parse_object(parser);
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_LEFT_BRACKET))
    {
        return parse_array(parser);
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_NUMBER))
    {
        return parse_number(parser);
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_FALSE))
    {
        next_token(&parser->lexer);

        JsonElement element;
        element.kind = JSON_ELEMENT_KIND_FALSE;
        return element;
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_NULL))
    {
        next_token(&parser->lexer);

        JsonElement element;
        element.kind = JSON_ELEMENT_KIND_NULL;
        return element;
    }
    else if(has_token_kind(&parser->lexer, TOKEN_KIND_TRUE))
    {
        next_token(&parser->lexer);

        JsonElement element;
        element.kind = JSON_ELEMENT_KIND_TRUE;
        return element;
    }

    JsonElement element;
    element.kind = JSON_ELEMENT_KIND_INVALID;
    return element;
}

void json_array_add(JsonElement* array_element, const JsonElement* element)
{
    AFT_ASSERT(array_element->kind == JSON_ELEMENT_KIND_ARRAY);

    JsonArray* array = &array_element->array;

    if(array->count + 1 > array->cap)
    {
        int old_cap = array->cap;
        int cap = old_cap;

        if(cap == 0)
        {
            cap = 16;
        }
        else
        {
            cap *= 2;
        }

        JsonMemoryBlock block = json_allocate(array->allocator, cap * sizeof(JsonElement));
        JsonElement* elements = block.memory;

        if(old_cap > 0)
        {
            for(int element_index = 0;
                    element_index < array->count;
                    element_index += 1)
            {
                elements[element_index] = array->elements[element_index];
            }

            JsonMemoryBlock old_block = {array->elements, old_cap * sizeof(JsonElement)};
            json_deallocate(array->allocator, old_block);
        }

        array->elements = elements;
        array->cap = cap;
    }

    array->elements[array->count] = *element;
    array->count += 1;
}

static uint32_t next_power_of_two(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

static uint32_t fnv_hash(const char* key, int bytes)
{
    const uint32_t p = UINT32_C(16777619);
    uint32_t hash = UINT32_C(2166136261);

    for(int byte_index = 0; byte_index < bytes; byte_index += 1)
    {
        hash = p * (hash ^ key[byte_index]);
    }

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;

    return hash;
}

static uint32_t hash_key(JsonElement* key)
{
    AFT_ASSERT(key->kind == JSON_ELEMENT_KIND_STRING);

    const char* contents = aft_string_get_contents_const(&key->string);
    int count = aft_string_get_count(&key->string);

    return fnv_hash(contents, count);
}

static int find_slot(JsonElement** keys, int cap, void* key, uint32_t hash)
{
    int probe = hash & (cap - 1);
    while(keys[probe] != key && keys[probe] != object_slot_empty)
    {
        probe = (probe + 1) & (cap - 1);
    }

    return probe;
}

static void json_object_grow(JsonElement* object_element, int cap)
{
    AFT_ASSERT(object_element->kind == JSON_ELEMENT_KIND_OBJECT);

    JsonObject* object = &object_element->object;

    int prior_cap = object->cap;

    JsonElement** keys = json_allocate(object->allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    JsonElement** values = json_allocate(object->allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    uint32_t* hashes = json_allocate(object->allocator, sizeof(uint32_t) * cap).memory;

    for(int slot_index = 0;
            slot_index < prior_cap;
            slot_index += 1)
    {
        JsonElement* key = object->keys[slot_index];

        if(key == object_slot_empty)
        {
            continue;
        }

        uint32_t hash = object->hashes[slot_index];
        int slot = find_slot(keys, cap, key, hash);
        keys[slot] = key;
        hashes[slot] = hash;
        values[slot] = object->values[slot_index];
    }

    // Copy over the overflow pair.
    keys[cap] = object->keys[prior_cap];
    values[cap] = object->values[prior_cap];

    JsonMemoryBlock key_block = {object->keys, sizeof(JsonElement*) * (prior_cap + 1)};
    JsonMemoryBlock value_block = {object->keys, sizeof(JsonElement*) * (prior_cap + 1)};
    JsonMemoryBlock hash_block = {object->keys, sizeof(uint32_t) * prior_cap};
    json_deallocate(object->allocator, key_block);
    json_deallocate(object->allocator, value_block);
    json_deallocate(object->allocator, hash_block);

    object->keys = keys;
    object->values = values;
    object->hashes = hashes;
    object->cap = cap;
}

void json_object_add(JsonElement* object_element, JsonElement* original_key, JsonElement* original_value)
{
    AFT_ASSERT(object_element->kind == JSON_ELEMENT_KIND_OBJECT);

    JsonObject* object = &object_element->object;

    JsonElement* key = json_allocate(object->allocator, sizeof(JsonElement)).memory;
    JsonElement* value = json_allocate(object->allocator, sizeof(JsonElement)).memory;
    *key = *original_key;
    *value = *original_value;

    if(key == object_slot_empty)
    {
        int overflow_index = object->cap;
        object->keys[overflow_index] = key;
        object->values[overflow_index] = value;

        if(object->keys[overflow_index] != key)
        {
            object->count += 1;
        }

        return;
    }

    int load_limit = (3 * object->cap) / 4;
    if(object->count >= load_limit)
    {
        json_object_grow(object_element, 2 * object->cap);
    }

    uint32_t hash = hash_key(key);
    int slot = find_slot(object->keys, object->cap, key, hash);
    if(object->keys[slot] != key)
    {
        object->count += 1;
    }

    object->keys[slot] = key;
    object->values[slot] = value;
    object->hashes[slot] = hash;
}

void json_object_create(JsonElement* element, int cap, void* allocator)
{
    element->kind = JSON_ELEMENT_KIND_OBJECT;

    JsonObject* object = &element->object;

    if(cap <= 0)
    {
        cap = 16;
    }
    else
    {
        cap = next_power_of_two(cap);
    }

    object->cap = cap;
    object->count = 0;
    object->allocator = allocator;

    object->keys = json_allocate(allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    object->values = json_allocate(allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    object->hashes = json_allocate(allocator, sizeof(uint32_t) * cap).memory;

    int overflow_index = cap;
    object->keys[overflow_index] = object_overflow_empty;
    object->values[overflow_index] = NULL;
}

void json_object_destroy(JsonElement* element)
{
    JsonObject* object = &element->object;

    for(JsonObjectIterator it = json_object_iterator_start(object);
            json_object_iterator_is_not_end(it);
            it = json_object_iterator_next(it))
    {
        JsonElement* key = json_object_iterator_get_key(it);
        JsonElement* value = json_object_iterator_get_value(it);

        JsonMemoryBlock key_block = {key, sizeof(JsonElement)};
        JsonMemoryBlock value_block = {value, sizeof(JsonElement)};
        json_deallocate(object->allocator, key_block);
        json_deallocate(object->allocator, value_block);
    }

    JsonMemoryBlock key_block = {object->keys, sizeof(JsonElement*) * (object->cap + 1)};
    JsonMemoryBlock value_block = {object->values, sizeof(JsonElement*) * (object->cap + 1)};
    JsonMemoryBlock hash_block = {object->hashes, sizeof(uint32_t) * object->cap};
    json_deallocate(object->allocator, key_block);
    json_deallocate(object->allocator, value_block);
    json_deallocate(object->allocator, hash_block);

    object->cap = 0;
    object->count = 0;
    object->allocator = NULL;
}

MaybeJsonElementPointer json_object_get(const JsonElement* element, JsonElement* key)
{
    AFT_ASSERT(element->kind == JSON_ELEMENT_KIND_OBJECT);

    const JsonObject* object = &element->object;

    MaybeJsonElementPointer result = {0};

    if(key == object_slot_empty)
    {
        int overflow_index = object->cap;
        if(object->keys[overflow_index] == object_overflow_empty)
        {
            result.valid = false;
        }
        else
        {
            result.value = object->values[overflow_index];
            result.valid = true;
        }

        return result;
    }

    uint32_t hash = hash_key(key);
    int slot = find_slot(object->keys, object->cap, key, hash);

    bool got = object->keys[slot] == key;
    if(got)
    {
        result.value = object->values[slot];
        result.valid = true;
    }

    return result;
}

JsonElement* json_object_iterator_get_key(JsonObjectIterator it)
{
    AFT_ASSERT(it.index >= 0 && it.index <= it.object->cap);

    return it.object->keys[it.index];
}

JsonElement* json_object_iterator_get_value(JsonObjectIterator it)
{
    AFT_ASSERT(it.index >= 0 && it.index <= it.object->cap);

    return it.object->values[it.index];
}

bool json_object_iterator_is_not_end(JsonObjectIterator it)
{
    return it.index != object_iterator_end_index;
}

JsonObjectIterator json_object_iterator_next(JsonObjectIterator it)
{
    JsonObjectIterator result;
    result.object = it.object;

    int index = it.index;
    do
    {
        index += 1;
        if(index >= it.object->cap)
        {
            if(index == it.object->cap
                    && it.object->keys[it.object->cap] != object_overflow_empty)
            {
                result.index = it.object->cap;
                return result;
            }

            result.index = object_iterator_end_index;
            return result;
        }
    } while(it.object->keys[index] == object_slot_empty);

    result.index = index;
    return result;
}

JsonObjectIterator json_object_iterator_start(const JsonObject* object)
{
    JsonObjectIterator iterator;
    iterator.object = object;

    if(object->count > 0)
    {
        iterator.index = 0;
    }
    else
    {
        iterator.index = object_iterator_end_index;
    }

    return json_object_iterator_next(iterator);
}

JsonResult json_parse(AftStringSlice* slice, void* allocator)
{
    JsonResult result;
    result.error = JSON_ERROR_NONE;
    result.root = NULL;
    result.allocator = allocator;

    Parser parser = {0};
    parser.lexer.slice = slice;
    parser.allocator = allocator;

    next_token(&parser.lexer);
    JsonElement element = parse_value(&parser);

    JsonMemoryBlock block = json_allocate(allocator, sizeof(JsonElement));
    if(!block.memory)
    {
        json_element_destroy(&element);
        result.error = JSON_ERROR_ALLOCATION_FAILURE;
        return result;
    }

    result.root = block.memory;
    *result.root = element;

    if(parser.error != JSON_ERROR_NONE)
    {
        json_result_destroy(&result);
        result.error = parser.error;
    }

    return result;
}

void json_result_destroy(JsonResult* result)
{
    if(result->root)
    {
        json_element_destroy(result->root);

        JsonMemoryBlock block = {result->root, sizeof(JsonElement)};
        json_deallocate(result->allocator, block);

        result->root = NULL;
    }
}

static void add_indentation(AftString* string, int indent_level, int spaces_per_indent)
{
    int spaces = spaces_per_indent * indent_level;

    for(int space = 0; space < spaces; space += 1)
    {
        aft_string_append_char(string, ' ');
    }
}

static void serialize_element(Serializer* serializer, const JsonElement* element);

static void serialize_array(Serializer* serializer, const JsonElement* element)
{
    if(element->array.count == 0)
    {
        aft_string_append_c_string(serializer->string, "[]");
        return;
    }

    aft_string_append_c_string(serializer->string, "[\n");

    serializer->indent_level += 1;

    for(int element_index = 0;
            element_index < element->array.count;
            element_index += 1)
    {
        add_indentation(serializer->string, serializer->indent_level, serializer->spaces_per_indent);

        serialize_element(serializer, &element->array.elements[element_index]);

        if(element_index < element->array.count - 1)
        {
            aft_string_append_c_string(serializer->string, ",\n");
        }
        else
        {
            aft_string_append_c_string(serializer->string, "\n");
        }
    }

    serializer->indent_level -= 1;

    add_indentation(serializer->string, serializer->indent_level, serializer->spaces_per_indent);

    aft_string_append_char(serializer->string, ']');
}

static void serialize_string(Serializer* serializer, const JsonElement* element);

static void serialize_object(Serializer* serializer, const JsonElement* element)
{
    if(element->object.count == 0)
    {
        aft_string_append_c_string(serializer->string, "{}");
        return;
    }

    aft_string_append_c_string(serializer->string, "{\n");

    serializer->indent_level += 1;

    int element_index = 0;

    for(JsonObjectIterator it = json_object_iterator_start(&element->object);
            json_object_iterator_is_not_end(it);
            it = json_object_iterator_next(it))
    {
        add_indentation(serializer->string, serializer->indent_level, serializer->spaces_per_indent);

        serialize_string(serializer, json_object_iterator_get_key(it));
        aft_string_append_c_string(serializer->string, ": ");
        serialize_element(serializer, json_object_iterator_get_value(it));

        if(element_index < element->object.count - 1)
        {
            aft_string_append_c_string(serializer->string, ",\n");
        }
        else
        {
            aft_string_append_c_string(serializer->string, "\n");
        }

        element_index += 1;
    }

    serializer->indent_level -= 1;

    add_indentation(serializer->string, serializer->indent_level, serializer->spaces_per_indent);

    aft_string_append_char(serializer->string, '}');
}

static void serialize_string(Serializer* serializer, const JsonElement* element)
{
    aft_string_append_char(serializer->string, '"');

    const char* contents = aft_string_get_contents_const(&element->string);
    int count = aft_string_get_count(&element->string);

    for(int char_index = 0;
            char_index < count;
            char_index += 1)
    {
        switch(contents[char_index])
        {
            case '\b':
            {
                aft_string_append_c_string(serializer->string, "\\b");
                break;
            }
            case '\t':
            {
                aft_string_append_c_string(serializer->string, "\\t");
                break;
            }
            case '\n':
            {
                aft_string_append_c_string(serializer->string, "\\n");
                break;
            }
            case '\f':
            {
                aft_string_append_c_string(serializer->string, "\\f");
                break;
            }
            case '\r':
            {
                aft_string_append_c_string(serializer->string, "\\r");
                break;
            }
            case '"':
            {
                aft_string_append_c_string(serializer->string, "\\\"");
                break;
            }
            case '\\':
            {
                aft_string_append_c_string(serializer->string, "\\\\");
                break;
            }
            default:
            {
                aft_string_append_char(serializer->string, contents[char_index]);
                break;
            }
        }
    }

    aft_string_append_char(serializer->string, '"');
}

static void serialize_element(Serializer* serializer, const JsonElement* element)
{
    switch(element->kind)
    {
        case JSON_ELEMENT_KIND_ARRAY:
        {
            serialize_array(serializer, element);
            break;
        }
        case JSON_ELEMENT_KIND_FALSE:
        {
            aft_string_append_c_string(serializer->string, "false");
            break;
        }
        case JSON_ELEMENT_KIND_OBJECT:
        {
            serialize_object(serializer, element);
            break;
        }
        case JSON_ELEMENT_KIND_NUMBER:
        {
            char hell[25];
            sprintf(hell, "%g", element->number);
            aft_string_append_c_string(serializer->string, hell);
            break;
        }
        case JSON_ELEMENT_KIND_NULL:
        {
            aft_string_append_c_string(serializer->string, "null");
            break;
        }
        case JSON_ELEMENT_KIND_STRING:
        {
            serialize_string(serializer, element);
            break;
        }
        case JSON_ELEMENT_KIND_TRUE:
        {
            aft_string_append_c_string(serializer->string, "true");
            break;
        }
    }
}

AftMaybeString json_serialize(const JsonElement* element, void* allocator)
{
    AftMaybeString result;
    result.valid = false;
    aft_string_initialise_with_allocator(&result.value, allocator);

    Serializer serializer =
    {
        .spaces_per_indent = 4,
        .string = &result.value,
    };
    serialize_element(&serializer, element);

    result.valid = true;

    return result;
}
