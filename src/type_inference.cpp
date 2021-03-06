// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "common_headers.h"

#include "kernel.h"
#include "building.h"
#include "evaluation.h"
#include "inspection.h"
#include "list.h"
#include "names.h"
#include "term.h"
#include "type.h"
#include "type_inference.h"

namespace circa {

Type* find_common_type(caValue* list)
{
    if (list_length(list) == 0)
        return &ANY_T;

    // Check if every type in this list is the same.
    bool all_equal = true;
    for (int i=1; i < list_length(list); i++) {
        if (as_type(list_get(list,0)) != as_type(list_get(list,i))) {
            all_equal = false;
            break;
        }
    }

    if (all_equal)
        return as_type(list_get(list,0));

    // Special case, allow ints to go into floats
    bool all_are_ints_or_floats = true;
    for (int i=0; i < list_length(list); i++) {
        if ((as_type(list_get(list,i)) != &INT_T) && (as_type(list_get(list,i)) != &FLOAT_T)) {
            all_are_ints_or_floats = false;
            break;
        }
    }

    if (all_are_ints_or_floats)
        return &FLOAT_T;

    // Another special case, if all types are lists then use LIST_T
    bool all_are_lists = true;
    for (int i=0; i < list_length(list); i++) {
        if (!is_list_based_type(as_type(list_get(list,i))))
            all_are_lists = false;
    }

    if (all_are_lists)
        return &LIST_T;

    // Otherwise give up
    return &ANY_T;
}

Type* find_common_type(Type* type1, Type* type2)
{
    if (type1 == NULL)
        return type2;

    if (type1 == type2)
        return type1;

    if (type1 == &ANY_T || type2 == &ANY_T)
        return &ANY_T;

    if (is_list_based_type(type1) && is_list_based_type(type2))
        return &LIST_T;

    return &ANY_T;
}

Type* find_common_type(Type* type1, Type* type2, Type* type3)
{
    List list;
    set_type_list(&list, type1, type2, type3);
    return find_common_type(&list);
}

Type* infer_type_of_get_index(Term* input)
{
    if (input == NULL)
        return &ANY_T;

    switch (list_get_parameter_type(&input->type->parameter)) {
    case LIST_UNTYPED:
        return &ANY_T;
    case LIST_TYPED_UNSIZED:
        return list_get_repeated_type_from_type(input->type);
    case LIST_TYPED_SIZED:
    case LIST_TYPED_SIZED_NAMED:
        return find_common_type(list_get_type_list_from_type(input->type));
    case LIST_INVALID_PARAMETER:
    default:
        return &ANY_T;
    }
}

Term* statically_infer_length_func(Branch* branch, Term* term)
{
    Term* input = term->input(0);
    if (input->function == FUNCS.copy)
        return statically_infer_length_func(branch, input->input(0));

    if (input->function == FUNCS.list)
        return create_int(branch, input->numInputs());

    if (input->function == FUNCS.list_append) {
        Term* leftLength = apply(branch, FUNCS.length, TermList(input->input(0)));
        return apply(branch, FUNCS.add, TermList(
                    statically_infer_length_func(branch, leftLength),
                    create_int(branch, 1)));
    }

    // Give up
    std::cout << "statically_infer_length_func didn't understand: "
        << input->function->name << std::endl;
    return create_symbol_value(branch, name_Unknown);
}

Term* statically_infer_result(Branch* branch, Term* term)
{
    if (term->function == FUNCS.length)
        return statically_infer_length_func(branch, term);

#if 0
    Disabled, this approach might be flawed
    if (term->function == TYPE_FUNC)
        return statically_infer_type(branch, term->input(0));
#endif

    // Function not recognized
    std::cout << "statically_infer_result didn't recognize: "
        << term->function->name << std::endl;
    return create_symbol_value(branch, name_Unknown);
}

void statically_infer_result(Term* term, caValue* result)
{
    Branch scratch;

    Term* resultTerm = statically_infer_result(&scratch, term);

    if (is_value(resultTerm))
        copy(term_value(resultTerm), result);
    else {
        Stack context;
        evaluate_minimum(&context, resultTerm, result);
    }
}

Type* create_typed_unsized_list_type(Type* elementType)
{
    Type* type = create_type();
    list_t::setup_type(type);
    set_type(&type->parameter, elementType);
    std::string name = std::string("List<") + name_to_string(elementType->name) + ">";
    type->name = name_from_string(name.c_str());
    return type;
}

} // namespace circa
