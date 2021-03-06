// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <set>

#include "common_headers.h"

#include "dict.h"
#include "kernel.h"
#include "names.h"
#include "type.h"

namespace circa {

struct DictData {
    struct Slot {
        char* key;
        caValue value;
    };

    int capacity;
    int count;
    Slot slots[0];
    // slots has size [capacity].
};

namespace dict_t {

// How many slots to create for a brand new dictionary.
const int INITIAL_SIZE = 10;

// When reallocating a dictionary, how many slots should initially be filled.
const float INITIAL_LOAD_FACTOR = 0.3;

// The load at which we'll trigger a reallocation.
const float MAX_LOAD_FACTOR = 0.75;

int hash_string(const char* str)
{
    // Dumb and simple hash function
    int result = 0;
    int byte = 0;
    while (*str != 0) {
        result = result ^ (*str << (8 * byte));
        byte = (byte + 1) % 4;
        str++;
    }
    return result;
}

DictData* create_dict(int capacity)
{
    ca_assert(capacity > 0);
    DictData* result = (DictData*) malloc(sizeof(DictData) + capacity * sizeof(DictData::Slot));
    result->capacity = capacity;
    result->count = 0;
    memset(result->slots, 0, capacity * sizeof(DictData::Slot));
    for (int s=0; s < capacity; s++)
        initialize_null(&result->slots[s].value);

    #if VERBOSE_LOG
    std::cout << "create_dict: " << result << std::endl;
    #endif

    return result;
}

DictData* create_dict()
{
    return create_dict(INITIAL_SIZE);
}

void free_dict(DictData* data)
{
    if (data == NULL)
        return;

    for (int i=0; i < data->capacity; i++) {
        free(data->slots[i].key);
        set_null(&data->slots[i].value);
    }
    free(data);

    #if VERBOSE_LOG
    std::cout << "free_dict: " << data << std::endl;
    #endif
}

DictData* grow(DictData* data, int new_capacity)
{
    DictData* new_data = create_dict(new_capacity);

    int existingCapacity = 0;
    if (data != NULL)
        existingCapacity = data->capacity;

    // Move all the keys & values over.
    for (int i=0; i < existingCapacity; i++) {
        DictData::Slot* old_slot = &data->slots[i];

        if (old_slot->key == NULL)
            continue;

        int index = insert(&new_data, old_slot->key);
        swap(&old_slot->value, &new_data->slots[index].value);

        ca_assert(is_null(&old_slot->value));
    }
    return new_data;
}

// Grow this dictionary by the default growth rate. This will result in a new DictData*
// object, don't use the old one after calling this.
void grow(DictData** dataPtr)
{
    int new_capacity = int((*dataPtr)->count / INITIAL_LOAD_FACTOR);
    DictData* oldData = *dataPtr;
    *dataPtr = grow(*dataPtr, new_capacity);
    free_dict(oldData);
}

DictData* duplicate(DictData* original)
{
    if (original == NULL)
        return NULL;

    INCREMENT_STAT(DictHardCopy);

    int new_capacity = int(original->count / INITIAL_LOAD_FACTOR);
    if (new_capacity < INITIAL_SIZE)
        new_capacity = INITIAL_SIZE;

    DictData* dupe = create_dict(new_capacity);

    // Copy all items
    for (int i=0; i < original->capacity; i++) {
        DictData::Slot* slot = &original->slots[i];

        if (slot->key == NULL)
            continue;

        int index = insert(&dupe, slot->key);
        copy(&slot->value, &dupe->slots[index].value);
    }
    return dupe;
}

// Get the 'ideal' slot index, the place we would put this string if there is no
// collision.
int find_ideal_slot_index(DictData* data, const char* str)
{
    ca_assert(data->capacity > 0);
    unsigned int hash = hash_string(str);
    return int(hash % data->capacity);
}

// Insert the given key into the dictionary, returns the index.
// This may create a new DictData* object, so don't use the old DictData* pointer after
// calling this.
int insert(DictData** dataPtr, const char* key)
{
    if (*dataPtr == NULL)
        *dataPtr = create_dict();

    // Check if this key is already here
    int existing = find_key(*dataPtr, key);
    if (existing != -1)
        return existing;

    // Check if it is time to reallocate
    if ((*dataPtr)->count >= MAX_LOAD_FACTOR * ((*dataPtr)->capacity))
        grow(dataPtr);

    DictData* data = *dataPtr;
    int index = find_ideal_slot_index(data, key);

    // Linearly advance if this slot is being used
    int starting_index = index;
    while (data->slots[index].key != NULL) {
        index = (index + 1) % data->capacity;

        // Die if we made it all the way around
        if (index == starting_index) {
            ca_assert(false);
            return 0;
        }
    }

    DictData::Slot* slot = &data->slots[index];
    slot->key = strdup(key);
    data->count++;

    return index;
}

void insert_value(DictData** dataPtr, const char* key, caValue* value)
{
    int index = insert(dataPtr, key);
    copy(value, &(*dataPtr)->slots[index].value);
}

// Check if strings are equal, and don't die if either pointer is NULL.
bool equal_strings(const char* left, const char* right)
{
    if (left == NULL || right == NULL) return false;
    return strcmp(left, right) == 0;
}

int find_key(DictData* data, const char* key)
{
    if (data == NULL)
        return -1;

    int index = find_ideal_slot_index(data, key);
    int starting_index = index;
    while (!equal_strings(key, data->slots[index].key)) {
        index = (index + 1) % data->capacity;

        // If we hit an empty slot, then give up.
        if (data->slots[index].key == NULL)
            return -1;

        // Or, if we looped around to the starting index, give up.
        if (index == starting_index)
            return -1;
    }

    return index;
}

caValue* get_value(DictData* data, const char* key)
{
    int index = find_key(data, key);
    if (index == -1) return NULL;
    return &data->slots[index].value;
}

caValue* get_index(DictData* data, int index)
{
    ca_assert(index >= 0);
    ca_assert(index < data->capacity);
    ca_assert(data->slots[index].key != NULL);
    return &data->slots[index].value;
}

void remove_at(DictData* data, int index)
{
    // Clear out this slot
    free(data->slots[index].key);
    data->slots[index].key = NULL;
    set_null(&data->slots[index].value);
    data->count--;

    // Check if any following keys would prefer to be moved up to this empty slot.
    while (true) {
        int prevIndex = index;
        index = (index+1) % data->capacity;

        DictData::Slot* slot = &data->slots[index];

        if (slot->key == NULL)
            break;

        // If a slot isn't in its ideal index, then we assume that it would rather be in
        // this slot.
        if (find_ideal_slot_index(data, slot->key) != index) {
            DictData::Slot* prevSlot = &data->slots[prevIndex];
            prevSlot->key = slot->key;
            slot->key = NULL;
            swap(&slot->value, &prevSlot->value);
            continue;
        }

        break;
    }
}

void remove(DictData* data, const char* key)
{
    int index = find_key(data, key);
    if (index == -1)
        return;
    remove_at(data, index);
}

int count(DictData* data)
{
    if (data == NULL)
        return 0;
    return data->count;
}

void clear(DictData* data)
{
    for (int i=0; i < data->capacity; i++) {
        DictData::Slot* slot = &data->slots[i];
        if (slot->key == NULL)
            continue;
        free(slot->key);
        slot->key = NULL;
        set_null(&slot->value);
    }
    data->count = 0;

    #if VERBOSE_LOG
    std::cout << "clear_dict: " << data << std::endl;
    #endif
}

struct SortedVisitItem {
    char* key;
    caValue* value;
    SortedVisitItem(char* k, caValue* v) : key(k), value(v) {}
};

struct SortedVisitItemCompare {
    bool operator()(SortedVisitItem const& left, SortedVisitItem const& right) {
        return strcmp(left.key, right.key) < 0;
    }
};

void visit_sorted(DictData* data, DictVisitor visitor, void* context)
{
    if (data == NULL)
        return;

    // This function is horribly inefficent, currently it does a full sort on every
    // key every time this is called. I'm not sure how frequently this function
    // will be used, if it's often then it will be revisited.

    std::set<SortedVisitItem, SortedVisitItemCompare> set;

    for (int i=0; i < data->capacity; i++) {
        DictData::Slot* slot = &data->slots[i];
        if (slot->key == NULL)
            continue;

        set.insert(SortedVisitItem(slot->key, &slot->value));
    }

    std::set<SortedVisitItem, SortedVisitItemCompare>::const_iterator it;
    for (it = set.begin(); it != set.end(); it++)
        visitor(context, it->key, it->value);
}

void debug_print(DictData* data)
{
    printf("dict: %p\n", data);
    printf("count: %d, capacity: %d\n", data->count, data->capacity);
    for (int i=0; i < data->capacity; i++) {
        DictData::Slot* slot = &data->slots[i];
        const char* key = "<null>";
        if (slot->key != NULL) key = slot->key;
        printf("[%d] %s = %s\n", i, key, to_string(&data->slots[i].value).c_str());
    }
}

void iterator_start(DictData* data, caValue* iterator)
{
    if (data == NULL || data->count == 0)
        return set_null(iterator);

    set_int(iterator, 0);

    // Advance if this iterator location isn't valid
    if (data->slots[0].key == NULL)
        iterator_next(data, iterator);
}

void iterator_next(DictData* data, caValue* iterator)
{
    int i = as_int(iterator);

    // Advance to next valid location
    int next = i + 1;
    while ((next < data->capacity) && (data->slots[next].key == NULL))
        next++;

    if (next >= data->capacity)
        set_null(iterator);
    else
        set_int(iterator, next);
}

void iterator_get(DictData* data, caValue* iterator, const char** key, caValue** value)
{
    int i = as_int(iterator);

    *key = data->slots[i].key;
    *value = &data->slots[i].value;
}

void iterator_delete(DictData* data, caValue* iterator)
{
    int i = as_int(iterator);
    remove_at(data, i);
}

namespace tagged_value_wrappers {

    void initialize(Type* type, caValue* value)
    {
        value->value_data.ptr = NULL;
    }
    void release(caValue* value)
    {
        free_dict((DictData*) value->value_data.ptr);
    }
    void copy(Type* type, caValue* source, caValue* dest)
    {
        change_type(dest, type);
        dest->value_data.ptr = duplicate((DictData*) source->value_data.ptr);
    }
    std::string to_string(caValue* value)
    {
        return dict_to_string((DictData*) value->value_data.ptr);
    }
    caValue* get_field(caValue* value, const char* field)
    {
        return dict_t::get_value((DictData*) value->value_data.ptr, field);
    }
    void visit_heap(Type*, caValue* value, Type::VisitHeapCallback callback, caValue* context)
    {
        DictData* data = (DictData*) value->value_data.ptr;
        if (data == NULL)
            return;
        Value relativeIdentifier;
        for (int i=0; i < data->capacity; i++) {
            if (data->slots[i].key == NULL)
                continue;
            set_string(&relativeIdentifier, data->slots[i].key);
            callback(&data->slots[i].value, &relativeIdentifier, context);
        }
    }

} // namespace tagged_value_wrappers

void setup_type(Type* type)
{
    reset_type(type);
    type->initialize = tagged_value_wrappers::initialize;
    type->release = tagged_value_wrappers::release;
    type->copy = tagged_value_wrappers::copy;
    type->toString = tagged_value_wrappers::to_string;
    type->getField = tagged_value_wrappers::get_field;
    type->visitHeap = tagged_value_wrappers::visit_heap;
    type->name = name_from_string("Dict");
}

} // namespace dict_t

Dict::Dict()
  : Value()
{
    create(&DICT_T, this);
}

Dict* Dict::checkCast(caValue* value)
{
    if (value == NULL)
        return NULL;

    if (is_dict(value))
        return (Dict*) value;
    else
        return NULL;
}

Dict* Dict::lazyCast(caValue* value)
{
    if (is_dict(value))
        return (Dict*) value;
    return make_dict(value);
}

Dict*
Dict::cast(caValue* v)
{
    return make_dict(v);
}

std::string Dict::toString()
{
    return dict_to_string((DictData*) this->value_data.ptr);
}

caValue* Dict::get(const char* key)
{
    return dict_t::get_value((DictData*) this->value_data.ptr, key);
}
caValue* Dict::operator[](const char* key)
{
    return get(key);
}
bool Dict::contains(const char* key)
{
    return get(key) != NULL;
}
caValue* Dict::insert(const char* key)
{
    DictData* data = (DictData*) this->value_data.ptr;
    int newIndex = dict_t::insert(&data, key);
    this->value_data.ptr = data;
    return &data->slots[newIndex].value;
}
void Dict::remove(const char* key)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::remove(data, key);
}
void Dict::set(const char* key, caValue* value)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::insert_value(&data, key, value);
    this->value_data.ptr = data;
}
void Dict::clear()
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::clear(data);
}
bool Dict::empty()
{
    DictData* data = (DictData*) this->value_data.ptr;
    return dict_t::count(data) == 0;
}

void Dict::iteratorStart(caValue* iterator)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::iterator_start(data, iterator);
}
void Dict::iteratorNext(caValue* iterator)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::iterator_next(data, iterator);
}
void Dict::iteratorGet(caValue* iterator, const char** key, caValue** value)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::iterator_get(data, iterator, key, value);
}
void Dict::iteratorDelete(caValue* iterator)
{
    DictData* data = (DictData*) this->value_data.ptr;
    dict_t::iterator_delete(data, iterator);
}
bool Dict::iteratorFinished(caValue* iterator)
{
    return is_null(iterator);
}
void Dict::setString(const char* key, const char* value)
{
    caValue* dest = insert(key);
    set_string(dest, value);
}
const char* Dict::getString(const char* key, const char* defaultValue)
{
    caValue* val = get(key);
    if (val == NULL)
        return defaultValue;
    if (is_string(val))
        return as_cstring(val);
    return defaultValue;
}
void Dict::setInt(const char* key, int value)
{
    caValue* dest = insert(key);
    set_int(dest, value);
}
int Dict::getInt(const char* key, int defaultValue)
{
    caValue* val = get(key);
    if (val == NULL)
        return defaultValue;
    if (is_int(val))
        return as_int(val);
    return defaultValue;
}
bool Dict::getBool(const char* key, bool defaultValue)
{
    caValue* val = get(key);
    if (val == NULL)
        return defaultValue;
    if (is_bool(val))
        return as_bool(val);
    return defaultValue;
}
void Dict::setBool(const char* key, bool value)
{
    caValue* dest = insert(key);
    set_bool(dest, value);
}

bool is_dict(caValue* value)
{
    return value->value_type == &DICT_T;
}
Dict* as_dict(caValue* value)
{
    ca_assert(is_dict(value));
    return (Dict*) value;
}

caValue* dict_get(DictData* data, const char* key)
{
    return dict_t::get_value(data, key);
}

caValue* dict_insert(DictData** dataPtr, const char* key)
{
    int index = dict_t::insert(dataPtr, key);
    return &(*dataPtr)->slots[index].value;
}

std::string dict_to_string(DictData* data)
{
    struct Visitor {
        std::stringstream strm;
        bool first;
        Visitor() : first(true) {}
        static void visit(void* context, const char* key, caValue* value)
        {
            Visitor& obj = *((Visitor*) context);
            if (!obj.first)
                obj.strm << ", ";
            obj.first = false;
            obj.strm << key << ": " << to_string(value);
        }
    };

    Visitor visitor;
    visitor.strm << "{";
    dict_t::visit_sorted(data, Visitor::visit, &visitor);
    visitor.strm << "}";
    return visitor.strm.str();
}

Dict* make_dict(caValue* value)
{
    create(&DICT_T, value);
    return (Dict*) value;
}

caValue* dict_get(caValue* dict, const char* field)
{
    ca_assert(is_dict(dict));
    return dict_get((DictData*) dict->value_data.ptr, field);
}

caValue* dict_insert(caValue* dict, const char* field)
{
    ca_assert(is_dict(dict));
    return dict_insert((DictData**) &dict->value_data.ptr, field);
}
int dict_count(caValue* dict)
{
    ca_assert(is_dict(dict));
    return dict_t::count((DictData*) dict->value_data.ptr);
}

} // namespace circa
