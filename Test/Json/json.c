#include "json.h"

#include <assert.h>
#include <stdio.h>

#define AFT_ASSERT(expression) assert(expression)


static JsonElement* const object_slot_empty = 0;
static JsonElement* const object_overflow_empty = ((void*) 1);
static const int object_iterator_end_index = -1;


static void json_object_destroy(JsonElement* element);


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

    JsonElement** keys = aft_allocate(object->allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    JsonElement** values = aft_allocate(object->allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    uint32_t* hashes = aft_allocate(object->allocator, sizeof(uint32_t) * cap).memory;

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

    AftMemoryBlock key_block = {object->keys, sizeof(JsonElement*) * (prior_cap + 1)};
    AftMemoryBlock value_block = {object->values, sizeof(JsonElement*) * (prior_cap + 1)};
    AftMemoryBlock hash_block = {object->hashes, sizeof(uint32_t) * prior_cap};
    aft_deallocate(object->allocator, key_block);
    aft_deallocate(object->allocator, value_block);
    aft_deallocate(object->allocator, hash_block);

    object->keys = keys;
    object->values = values;
    object->hashes = hashes;
    object->cap = cap;
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

        AftMemoryBlock block = aft_allocate(array->allocator, sizeof(JsonElement) * cap);
        JsonElement* elements = block.memory;

        if(old_cap > 0)
        {
            for(int element_index = 0;
                    element_index < array->count;
                    element_index += 1)
            {
                elements[element_index] = array->elements[element_index];
            }

            AftMemoryBlock old_block = {array->elements, old_cap * sizeof(JsonElement)};
            aft_deallocate(array->allocator, old_block);
        }

        array->elements = elements;
        array->cap = cap;
    }

    array->elements[array->count] = *element;
    array->count += 1;
}

void json_element_destroy(JsonElement* element)
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

            AftMemoryBlock block = {element->array.elements, sizeof(JsonElement) * element->array.cap};
            aft_deallocate(element->array.allocator, block);
            break;
        }
        case JSON_ELEMENT_KIND_OBJECT:
        {
            json_object_destroy(element);
            break;
        }
        case JSON_ELEMENT_KIND_STRING:
        {
            aft_string_destroy(&element->string);
            break;
        }
    }
}

void json_object_add(JsonElement* object_element, JsonElement* original_key, JsonElement* original_value)
{
    AFT_ASSERT(object_element->kind == JSON_ELEMENT_KIND_OBJECT);

    JsonObject* object = &object_element->object;

    JsonElement* key = aft_allocate(object->allocator, sizeof(JsonElement)).memory;
    JsonElement* value = aft_allocate(object->allocator, sizeof(JsonElement)).memory;
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

    object->keys = aft_allocate(allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    object->values = aft_allocate(allocator, sizeof(JsonElement*) * (cap + 1)).memory;
    object->hashes = aft_allocate(allocator, sizeof(uint32_t) * cap).memory;

    int overflow_index = cap;
    object->keys[overflow_index] = object_overflow_empty;
    object->values[overflow_index] = NULL;
}

static void json_object_destroy(JsonElement* element)
{
    AFT_ASSERT(element->kind == JSON_ELEMENT_KIND_OBJECT);

    JsonObject* object = &element->object;

    for(JsonObjectIterator it = json_object_iterator_start(object);
            json_object_iterator_is_not_end(it);
            it = json_object_iterator_next(it))
    {
        JsonElement* key = json_object_iterator_get_key(it);
        JsonElement* value = json_object_iterator_get_value(it);

        json_element_destroy(key);
        json_element_destroy(value);

        AftMemoryBlock key_block = {key, sizeof(JsonElement)};
        AftMemoryBlock value_block = {value, sizeof(JsonElement)};
        aft_deallocate(object->allocator, key_block);
        aft_deallocate(object->allocator, value_block);
    }

    AftMemoryBlock key_block = {object->keys, sizeof(JsonElement*) * (object->cap + 1)};
    AftMemoryBlock value_block = {object->values, sizeof(JsonElement*) * (object->cap + 1)};
    AftMemoryBlock hash_block = {object->hashes, sizeof(uint32_t) * object->cap};
    aft_deallocate(object->allocator, key_block);
    aft_deallocate(object->allocator, value_block);
    aft_deallocate(object->allocator, hash_block);

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
    iterator.index = -1;

    return json_object_iterator_next(iterator);
}
