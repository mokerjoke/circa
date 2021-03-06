// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "circa/internal/for_hosted_funcs.h"

namespace circa {
namespace comparison_function {

    CA_START_FUNCTIONS;

    CA_DEFINE_FUNCTION(less_than_i, "less_than_i(int,int) -> bool")
    {
        set_bool(OUTPUT, INT_INPUT(0) < INT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(less_than_f, "less_than_f(number,number) -> bool")
    {
        set_bool(OUTPUT, FLOAT_INPUT(0) < FLOAT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(less_than_eq_i, "less_than_eq_i(int,int) -> bool")
    {
        set_bool(OUTPUT, INT_INPUT(0) <= INT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(less_than_eq_f, "less_than_eq_f(number,number) -> bool")
    {
        set_bool(OUTPUT, FLOAT_INPUT(0) <= FLOAT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(greater_than_i, "greater_than_i(int,int) -> bool")
    {
        set_bool(OUTPUT, INT_INPUT(0) > INT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(greater_than_f, "greater_than_f(number,number) -> bool")
    {
        set_bool(OUTPUT, FLOAT_INPUT(0) > FLOAT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(greater_than_eq_i, "greater_than_eq_i(int,int) -> bool")
    {
        set_bool(OUTPUT, INT_INPUT(0) >= INT_INPUT(1));
    }

    CA_DEFINE_FUNCTION(greater_than_eq_f, "greater_than_eq_f(number,number) -> bool")
    {
        set_bool(OUTPUT, FLOAT_INPUT(0) >= FLOAT_INPUT(1));
    }

    void setup(Branch* kernel)
    {
        CA_SETUP_FUNCTIONS(kernel);
    }
}
}
