// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#pragma once

namespace circa {

caValue* get_handle_value(caValue* handle);
void* get_handle_value_opaque_pointer(caValue* handle);
void set_handle_value(caValue* handle, Type* type, caValue* value, ReleaseFunc releaseFunc);
void set_handle_value(caValue* handle, caValue* value, ReleaseFunc releaseFunc);
void set_handle_value_opaque_pointer(caValue* handle, Type* type, void* value,
    ReleaseFunc releaseFunc);
void handle_set_release_func(caValue* handle, ReleaseFunc releaseFunc);
void setup_handle_type(Type* type);

bool is_handle(caValue* value);

// If 'value' is a handle, return the value that the handle points to, otherwise
// just return 'value'.
caValue* dereference_handle(caValue* value);

} // namespace circa
