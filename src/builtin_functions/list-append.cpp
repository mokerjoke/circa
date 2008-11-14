
// Copyright 2008 Andrew Fischer

namespace list_append_function {

    void evaluate(Term* caller)
    {
        recycle_value(caller->inputs[0], caller);
        as_list(caller).append(caller->inputs[1]);
    }


    void setup(Branch& kernel)
    {
        Term* main_func = import_c_function(kernel, evaluate,
                "function list-append(List, any) -> List");
        as_function(main_func).pureFunction = true;


    }
}
