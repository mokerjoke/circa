// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "tagged_value_accessors.h"

#include "list.h"

namespace circa {

TaggedValue* List::append()
{
    _items.resize(_items.size() + 1);
    return get(_items.size() - 1);
}

TaggedValue* List::append(Type* type)
{
    TaggedValue* value = append();
    change_type(value, type);
    return value;
}

TaggedValue* List::get(int index)
{
    return &_items[index];
}

TaggedValue* List::operator[](int index)
{
    return get(index);
}

#if 0
bool is_list(TaggedValue* value)
{
    return value->value_type == LIST_T:
}

List& as_list(TaggedValue* value)
{
    assert(is_list(value));
    return *((List*) get_pointer(value));
}
#endif

}