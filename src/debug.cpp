// Copyright 2008 Paul Hodge

#include "common_headers.h"

#include "circa.h"

namespace circa {

#if DEBUG_CHECK_FOR_BAD_POINTERS
std::set<Term*> *DEBUG_GOOD_POINTER_SET = NULL;
#endif

void register_good_pointer(Term* term)
{
    if (DEBUG_GOOD_POINTER_SET == NULL)
        DEBUG_GOOD_POINTER_SET = new std::set<Term*>();
    DEBUG_GOOD_POINTER_SET->insert(term);
}

void unregister_good_pointer(Term* term)
{
    DEBUG_GOOD_POINTER_SET->erase(term);
}

bool is_bad_pointer(Term* term)
{
    if (DEBUG_GOOD_POINTER_SET == NULL)
        DEBUG_GOOD_POINTER_SET = new std::set<Term*>();
    return DEBUG_GOOD_POINTER_SET->find(term) == DEBUG_GOOD_POINTER_SET->end();
}

void assert_good_pointer(Term* term)
{
#if DEBUG_CHECK_FOR_BAD_POINTERS
    if (is_bad_pointer(term))
        throw std::runtime_error("assert_good_pointer failed (bad term pointer)");
#endif
}

void sanity_check_term(Term* term)
{
    assert_good_pointer(term);
}

void sanity_check_the_world()
{
    for (int i=0; i < KERNEL->numTerms(); i++) {
        Term* term = KERNEL->get(i);
        sanity_check_term(term);
    }
}

} // namespace circa
