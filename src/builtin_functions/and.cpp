// Copyright (c) 2007-2009 Andrew Fischer. All rights reserved.

#include <circa.h>

namespace circa {
namespace and_function {

    void evaluate(Term* caller)
    {
        as_bool(caller) = as_bool(caller->input(0)) && as_bool(caller->input(1));
    }

    void setup(Branch& kernel)
    {
        import_function(kernel, evaluate, "and(bool a, bool b) : bool;"
                "'Return whether a and b are both true' end");
    }
}
}
