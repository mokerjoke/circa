// Copyright (c) Paul Hodge. See LICENSE file for license terms.

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace list_function {

    CA_START_FUNCTIONS;

    Type* specializeType(Term* term)
    {
        List inputTypes;
        for (int i=0; i < term->numInputs(); i++)
            set_type(inputTypes.append(), term->input(i)->type);

        return as_type(create_tuple_type(&inputTypes));
    }

    CA_DEFINE_FUNCTION(evaluate, "list(any...) -> List")
    {
        set_list(OUTPUT);
        List* result = List::checkCast(OUTPUT);

        result->resize(NUM_INPUTS);

        for (int i=0; i < NUM_INPUTS; i++)
            copy(INPUT(i), (*result)[i]);
    }

    CA_DEFINE_FUNCTION(repeat, "repeat(any, int) -> List")
    {
        set_list(OUTPUT);
        List* result = List::checkCast(OUTPUT);
        TValue* source = INPUT(0);
        int repeatCount = INT_INPUT(1);

        result->resize(repeatCount);

        for (int i=0; i < repeatCount; i++)
            copy(source, result->get(i));
    }

    CA_DEFINE_FUNCTION(blank_list, "blank_list(int) -> List")
    {
        set_list(OUTPUT);
        List* result = List::checkCast(OUTPUT);
        result->resize(0);
        result->resize(INT_INPUT(0));
    }

    CA_DEFINE_FUNCTION(resize, "resize(List, int) -> List")
    {
        copy(INPUT(0), OUTPUT);
        List* result = List::checkCast(OUTPUT);
        result->resize(INT_INPUT(1));
    }

    void setup(Branch* kernel)
    {
        CA_SETUP_FUNCTIONS(kernel);

        LIST_FUNC = kernel->get("list");

        as_function(LIST_FUNC)->specializeType = specializeType;
    }
}
}
