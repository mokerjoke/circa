// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#pragma once

namespace circa {

namespace common_type_callbacks {
    int shallow_hash_func(caValue* value);
} // namespace common_functions_t

namespace null_t {
    void setup_type(Type* type);
} // namespace null_t

namespace opaque_pointer_t {
    void setup_type(Type* type);
} // namespace opaque_pointer_t

}
