#include "json.h"

#include <assert.h>


#define ASSERT(expression) \
    assert(expression)


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


static JsonElement parse_string(Parser* parser);
static JsonElement parse_value(Parser* parser);


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
        ASSERT(false);
        return 0;
    }
}

static char32_t four_digit_hex_to_codepoint(AftStringSlice slice)
{
    ASSERT(slice.count == 4);

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

    if(aft_string_slice_count(*lexer->slice) < keyword_count)
    {
        lexer->error = JSON_ERROR_INVALID_TOKEN;
        return false;
    }

    AftStringSlice slice = aft_string_slice(*lexer->slice, 0, keyword_count);
    AftStringSlice keyword_slice = aft_string_slice_from_buffer(keyword, keyword_count);

    if(!aft_string_slice_matches(slice, keyword_slice))
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
    return aft_string_slice_count(*lexer->slice) > 1
            && aft_string_slice_start(*lexer->slice)[0] == c;
}

static bool read_digits(Lexer* lexer)
{
    bool digit_found = false;

    while(aft_string_slice_count(*lexer->slice) != 0)
    {
        const char* start = aft_string_slice_start(*lexer->slice);

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
        int slice_left = aft_string_slice_count(*lexer->slice);

        if(slice_left == 0)
        {
            lexer->error = JSON_ERROR_INVALID_TOKEN;
            return false;
        }

        const char* start = aft_string_slice_start(*lexer->slice);

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
        const char* start = aft_string_slice_start(*lexer->slice);

        if(aft_string_slice_count(*lexer->slice) == 0)
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

        int count = (int) (aft_string_slice_start(*lexer->slice) - start);
        lexer->token.slice = aft_string_slice_from_buffer(start, count);
    }
}

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

    AftMaybeDouble result = aft_ascii_to_double(parser->lexer.token.slice);
    ASSERT(result.valid);
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
    const char* contents = aft_string_slice_start(slice);
    int count = aft_string_slice_count(slice);

    for(int char_index = 0;
            char_index < count;
            char_index += 1)
    {
        char character = contents[char_index];

        bool appended = false;

        if(character == '\\')
        {
            char_index += 1;

            ASSERT(char_index < count);

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
                    ASSERT(count - char_index >= 4);
                    AftStringSlice slice = aft_string_slice_from_buffer(&contents[char_index], 4);
                    char32_t codepoint = four_digit_hex_to_codepoint(slice);
                    appended = aft_utf8_append_codepoint(&element.string, codepoint);
                    char_index += 4;
                    break;
                }
                default:
                {
                    ASSERT(false);
                    break;
                }
            }
        }
        else
        {
            ASSERT(character != '"');

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

JsonResult json_deserialize(AftStringSlice* slice, void* allocator)
{
    JsonResult result;
    result.error = JSON_ERROR_NONE;

    Parser parser = {0};
    parser.lexer.slice = slice;
    parser.allocator = allocator;

    next_token(&parser.lexer);
    result.root = parse_value(&parser);

    if(parser.error != JSON_ERROR_NONE)
    {
        json_element_destroy(&result.root);
        result.error = parser.error;
    }

    return result;
}
