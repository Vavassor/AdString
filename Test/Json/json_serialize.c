#include "json.h"

#include <assert.h>


#define ASSERT(expression) \
    assert(expression)


typedef struct Serializer
{
    AftString* string;
    int indent_level;
    int spaces_per_indent;
} Serializer;


static void serialize_element(Serializer* serializer, const JsonElement* element);
static void serialize_string(Serializer* serializer, const JsonElement* element);


static void add_indentation(AftString* string, int indent_level, int spaces_per_indent)
{
    int spaces = spaces_per_indent * indent_level;

    for(int space = 0; space < spaces; space += 1)
    {
        aft_string_append_char(string, ' ');
    }
}

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
            AftMaybeString result = aft_ascii_from_double(element->number);
            ASSERT(result.valid);
            aft_string_append(serializer->string, &result.value);
            aft_string_destroy(&result.value);
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
