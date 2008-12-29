// Copyright 2008 Andrew Fischer

#include "builtins.h"
#include "circa.h"

namespace circa {
namespace alias_function {

    void evaluate(Term* caller)
    {
        caller->value = caller->input(0)->value;
        caller->ownsValue = false;
        caller->type = caller->input(0)->type;
    }

    void setup(Branch& kernel)
    {
        ALIAS_FUNC = import_c_function(kernel, evaluate,
                "function alias(any) -> any");
        as_function(ALIAS_FUNC).pureFunction = true;
    }
}
}
