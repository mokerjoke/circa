
#include "common_headers.h"

#include "bootstrapping.h"
#include "branch.h"
#include "builtins.h"
#include "common.h"
#include "errors.h"
#include "operations.h"
#include "subroutine.h"

namespace circa {
namespace struct_tests {

void test_simple()
{
    Branch *branch = new Branch();
    Term* my_struct_def = create_constant(branch, get_global("StructDefinition"));
    my_struct_def = apply_function(branch, get_global("struct-definition-set-name"),
            TermList(my_struct_def, constant_string(branch, "my-struct")));
    execute(my_struct_def);

    my_struct_def = apply_function(branch, get_global("add-field"),
            TermList(my_struct_def, constant_string(branch, "myFloat"), FLOAT_TYPE));
    execute(my_struct_def);
    my_struct_def = apply_function(branch, get_global("add-field"),
            TermList(my_struct_def, constant_string(branch, "myString"), STRING_TYPE));
    execute(my_struct_def);

    Term* my_instance = apply_function(branch, my_struct_def, TermList());
    execute(my_instance);

    my_instance = apply_function(branch, get_global("set-field"),
            TermList(my_instance, constant_string(branch, "myFloat"),
                constant_float(branch, 2)));
    execute(my_instance);
    branch->bindName(my_instance, "my_instance");

    Term* hopefully_two = quick_exec_function(branch,
            "hopefully-two = get-field(my_instance, 'myFloat)");

    test_assert(as_float(hopefully_two) == 2);
}

void test_simple2()
{
    Branch branch;
    quick_exec_function(&branch,
        "my-struct-def = define-struct('my-struct-def, list(float, string, Subroutine))");
    Term* myinst = quick_exec_function(&branch, "my-inst = my-struct-def()");
    myinst->toString();
}

void function_body_whatever(Term*) {}

void test_as_function_output()
{
    Branch *branch = new Branch();

    Term* return_type = quick_exec_function(branch, "define-struct('rt, list(string,float))");
    quick_create_function(branch, "my-func", function_body_whatever,
            TermList(FLOAT_TYPE), return_type);

    Term* func_inst = quick_exec_function(branch, "x = my-func(5)");

    test_assert(func_inst->type == return_type);
    test_assert(quick_exec_function(branch, "get-index(x, 0)")->type == STRING_TYPE);
    test_assert(quick_exec_function(branch, "get-index(x, 1)")->type == FLOAT_TYPE);
}

void test_rename_field()
{
    Branch *branch = new Branch();
    quick_exec_function(branch, "s = define-struct('s, list(float,List))");
    quick_exec_function(branch, "s = struct-definition-rename-field(s, 0, 'FirstField)");
    quick_exec_function(branch, "s = struct-definition-rename-field(s, 1, 'SecondField)");
    quick_exec_function(branch, "inst = s()");
    Term* firstField = quick_exec_function(branch, "get-field(inst, 'FirstField)");

    test_assert(firstField != NULL);
    test_assert(firstField->type == FLOAT_TYPE);
}

} // namespace struct_tests

void register_struct_tests()
{
    REGISTER_TEST_CASE(struct_tests::test_simple);
    REGISTER_TEST_CASE(struct_tests::test_simple2);
    REGISTER_TEST_CASE(struct_tests::test_as_function_output);
    REGISTER_TEST_CASE(struct_tests::test_rename_field);
}

} // namespace circa
