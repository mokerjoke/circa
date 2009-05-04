// Copyright 2009 Paul Hodge

#ifndef CIRCA_FOR_LOOPS_INCLUDED
#define CIRCA_FOR_LOOPS_INCLUDED

#include "common_headers.h"

namespace circa {

Term* get_for_loop_iterator(Term* forTerm);
Branch& get_for_loop_code(Term* forTerm);
Branch& get_for_loop_state(Term* forTerm, int index);
void evaluate_for_loop(Term* forTerm, Term* listTerm);

} // namespace circa

#endif // CIRCA_FOR_LOOPS_INCLUDED
