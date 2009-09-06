// Copyright (c) 2007-2009 Paul Hodge. All rights reserved.

#include "circa.h"

namespace circa {
namespace get_index_function {

    void evaluate(Term* caller)
    {
        Branch& input = caller->input(0)->asBranch();
        int index = caller->input(1)->asInt();

        if (index >= input.length()) {
            std::stringstream err;
            err << "Index " << index << " is out of range";
            error_occurred(caller, err.str());
            return;
        }
    
        assign_value(input[index], caller);
    }

    std::string toSourceString(Term* term)
    {
        std::stringstream out;

        out << term->input(0)->name;
        out << "[";
        out << get_source_of_input(term, 1);
        out << "]";
        return out.str();
    }

    Term* specializeType(Term* caller)
    {
        // Type inference is hacky due to the lack of parametrized list types.
        RefList inputTypes;

        Branch& inputList = caller->input(0)->asBranch();

        for (int i=0; i < inputList.length(); i++)
            inputTypes.append(inputList[i]->type);

        return find_common_type(inputTypes);
    }

    void setup(Branch& kernel)
    {
        GET_INDEX_FUNC = import_function(kernel, evaluate, "get_index(Branch, int) : any");
        function_t::get_specialize_type(GET_INDEX_FUNC) = specializeType;
        function_t::get_to_source_string(GET_INDEX_FUNC) = toSourceString;
    }
}
}
