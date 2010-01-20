// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include "common_headers.h"

#include "term.h"
#include "tagged_value_accessors.h"

namespace circa {

Term* alloc_term()
{
    // This function is not very useful now, but in the future we may use
    // a pool for term objects. So, don't use "new Term()", call this function.
    Term* term = new Term();
    set_null(term->value);
    return term;
}

} // namespace circa
