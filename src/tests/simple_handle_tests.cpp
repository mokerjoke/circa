// Copyright (c) Paul Hodge. See LICENSE file for license terms.

#include <circa.h>

#include "types/simple_handle.h"

namespace circa {
namespace simple_handle_tests {

const int numSlots = 3;
List g_slots;
Type handle_type;

int get_free_slot()
{
    for (int i=0; i < numSlots; i++)
        if (!as_bool(g_slots[i]))
            return i;
    test_assert(false);
    return -1;
}

void on_release_func(int handle)
{
    test_assert(as_bool(g_slots[handle]));
    set_bool(g_slots[handle], false);
}

void assign(TaggedValue* value, int handle)
{
    test_assert(!as_bool(g_slots[handle]));
    set_bool(g_slots[handle], true);
    simple_handle_t::set(&handle_type, value, handle);
}

CA_FUNCTION(alloc_handle)
{
    if (INPUT(0)->value_type != &handle_type)
        assign(OUTPUT, get_free_slot());
    else
        copy(INPUT(0), OUTPUT);
}

void setup(Branch& branch)
{
    g_slots.resize(numSlots);
    for (int i=0; i < numSlots; i++)
        set_bool(g_slots[i], false);

    simple_handle_t::setup_type(&handle_type);
    set_opaque_pointer(&handle_type.parameter, (void*) on_release_func);

    branch.compile("def alloc_handle(any s) -> any;");
    install_function(branch["alloc_handle"], alloc_handle);
}


void test_simple()
{
    Branch branch;
    setup(branch);

    test_equals(&g_slots, "[false, false, false]");

    TaggedValue handle;

    assign(&handle, 0);
    test_equals(&g_slots, "[true, false, false]");

    set_null(&handle);

    test_equals(&g_slots, "[false, false, false]");

    assign(&handle, 0);
    test_equals(&g_slots, "[true, false, false]");
    assign(&handle, 1);
    test_equals(&g_slots, "[false, true, false]");
    assign(&handle, 2);
    test_equals(&g_slots, "[false, false, true]");

    TaggedValue handle2;
    TaggedValue handle3;
    assign(&handle2, 0);
    test_equals(&g_slots, "[true, false, true]");
    assign(&handle3, 1);
    test_equals(&g_slots, "[true, true, true]");
    set_null(&handle2);
    test_equals(&g_slots, "[false, true, true]");
    set_null(&handle);
    test_equals(&g_slots, "[false, true, false]");
}


void test_with_state()
{
    Branch branch;
    setup(branch);

    test_equals(&g_slots, "[false, false, false]");

    branch.compile("state s");
    branch.compile("alloc_handle(@s)");

    EvalContext context;
    evaluate_branch_no_preserve_locals(&context, branch);

    test_equals(&g_slots, "[true, false, false]");

    evaluate_branch_no_preserve_locals(&context, branch);
    test_equals(&g_slots, "[true, false, false]");

    reset(&context.state);
    reset_locals(branch);
    test_equals(&g_slots, "[false, false, false]");
}

void test_deleted_state()
{
    Branch branch;
    setup(branch);

    branch.compile("state s");
    branch.compile("alloc_handle(@s)");

    branch.compile("state t");
    branch.compile("alloc_handle(@t)");

    EvalContext context;
    evaluate_branch_no_preserve_locals(&context, branch);

    test_equals(&g_slots, "[true, true, false]");

    clear_branch(&branch);
    branch.compile("state t");
    strip_abandoned_state(branch, &context.state);
    
    test_equals(&g_slots, "[false, true, false]");
}

void test_in_subroutine_state()
{
    Branch branch;
    setup(branch);

    branch.compile("def hi(any input) { state s = input }");
    branch.compile("state s");
    branch.compile("alloc_handle(@s)");
    branch.compile("hi(s)");

    EvalContext context;
    evaluate_branch_no_preserve_locals(&context, branch);

    test_equals(&g_slots, "[true, false, false]");

    set_null(&context.state);
    reset_locals(branch);

    test_equals(&g_slots, "[false, false, false]");
}

void test_included_file_changed()
{
    Branch branch;
    setup(branch);

    FakeFileSystem files;
    files["f"] = "state s; alloc_handle(@s)";

    branch.compile("include('f')");

    EvalContext context;
    evaluate_branch_no_preserve_locals(&context, branch);

    test_equals(&g_slots, "[true, false, false]");

    files.set("f", "");
    evaluate_branch_no_preserve_locals(&context, branch);

    test_equals(&g_slots, "[false, false, false]");
}

void register_tests()
{
    REGISTER_TEST_CASE(simple_handle_tests::test_simple);
    REGISTER_TEST_CASE(simple_handle_tests::test_with_state);
    REGISTER_TEST_CASE(simple_handle_tests::test_deleted_state);
    REGISTER_TEST_CASE(simple_handle_tests::test_in_subroutine_state);
    REGISTER_TEST_CASE(simple_handle_tests::test_included_file_changed);
}

}
}
