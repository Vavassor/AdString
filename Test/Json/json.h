#ifndef JSON_H_
#define JSON_H_

#include <AftString/aft_string.h>

typedef enum JsonElementKind
{
    JSON_ELEMENT_KIND_INVALID,
    JSON_ELEMENT_KIND_ARRAY,
    JSON_ELEMENT_KIND_FALSE,
    JSON_ELEMENT_KIND_OBJECT,
    JSON_ELEMENT_KIND_NULL,
    JSON_ELEMENT_KIND_NUMBER,
    JSON_ELEMENT_KIND_STRING,
    JSON_ELEMENT_KIND_TRUE,
} JsonElementKind;

typedef enum JsonError
{
    JSON_ERROR_NONE,
    JSON_ERROR_ALLOCATION_FAILURE,
    JSON_ERROR_INVALID_TOKEN,
} JsonError;


typedef struct JsonElement JsonElement;

typedef struct JsonArray
{
    JsonElement* elements;
    void* allocator;
    int cap;
    int count;
} JsonArray;

typedef struct JsonObject
{
    JsonElement** values;
    JsonElement** keys;
    uint32_t* hashes;
    void* allocator;
    int cap;
    int count;
} JsonObject;

typedef struct JsonObjectIterator
{
    const JsonObject* object;
    int index;
} JsonObjectIterator;

struct JsonElement
{
    union
    {
        JsonObject object;
        JsonArray array;
        AftString string;
        double number;
    };
    JsonElementKind kind;
};

typedef struct JsonResult
{
    JsonElement root;
    JsonError error;
} JsonResult;

typedef struct MaybeJsonElementPointer
{
    JsonElement* value;
    bool valid;
} MaybeJsonElementPointer;


void json_array_add(JsonElement* array_element, const JsonElement* element);

void json_element_destroy(JsonElement* element);

void json_object_add(JsonElement* object_element, JsonElement* key, JsonElement* value);
void json_object_create(JsonElement* element, int cap, void* allocator);
MaybeJsonElementPointer json_object_get(const JsonElement* element, JsonElement* key);

JsonElement* json_object_iterator_get_key(JsonObjectIterator it);
JsonElement* json_object_iterator_get_value(JsonObjectIterator it);
bool json_object_iterator_is_not_end(JsonObjectIterator it);
JsonObjectIterator json_object_iterator_next(JsonObjectIterator it);
JsonObjectIterator json_object_iterator_start(const JsonObject* object);

JsonResult json_deserialize(AftStringSlice* slice, void* allocator);
AftMaybeString json_serialize(const JsonElement* element, void* allocator);

#endif // JSON_H_
