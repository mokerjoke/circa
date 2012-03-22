// Copyright (c) Paul Hodge. See LICENSE file for license terms.

#pragma once

#include "common_headers.h"
#include "dict.h"
#include "list.h"
#include "loops.h"
#include "gc.h"
#include "tagged_value.h"
#include "term_list.h"

namespace circa {

struct Frame
{
    List registers;
    Branch* branch;
    int pc;
    int nextPc;
    int startPc;
    int endPc;

    // Used in for-loop
    bool loop;
};

struct EvalContext
{
    CircaObject header;

    // Error information:
    bool errorOccurred;
    Term* errorTerm;

    // Persistent state
    Value state;

    // Intra-program messages
    Dict messages;

    // List of values that are being passed from the EvalContext owner to the script.
    List argumentList;

    // Current execution stack
    int numFrames;
    Frame* stack;

    // Debugging flag- print all steps to stdout.
    bool trace;

    EvalContext();
    ~EvalContext();

private:
    // Disabled calls
    EvalContext(EvalContext const&) {}
    EvalContext& operator=(EvalContext const&) { return *this; }
};

void eval_context_print_multiline(std::ostream& out, EvalContext* context);
void eval_context_setup_type(Type* type);

// Stack frames
Frame* get_frame(EvalContext* context, int depth);
Frame* get_frame_from_bottom(EvalContext* context, int index);
Frame* push_frame(EvalContext* context, Branch* branch, List* registers);
Frame* push_frame(EvalContext* context, Branch* branch);
void push_frame_with_inputs(EvalContext* context, Branch* branch, List* inputs);
void pop_frame(EvalContext* context);
void finish_frame(EvalContext* context);
Frame* top_frame(EvalContext* context);
Branch* top_branch(EvalContext* context);
void reset_stack(EvalContext* context);

// Evaluate a single term. This is not usually called directly, it's called
// by the interpreter.
void evaluate_single_term(EvalContext* context, Term* term);

void evaluate_branch(EvalContext* context, Branch* branch);

void evaluate_save_locals(EvalContext* context, Branch* branch);

// Shorthand to call evaluate_branch with a new EvalContext:
void evaluate_branch(Branch* branch);

// Evaluate only the terms between 'start' and 'end'.
void evaluate_range(EvalContext* context, Branch* branch, int start, int end);

// Evaluate 'term' and every term that it depends on.
void evaluate_minimum(EvalContext* context, Term* term, caValue* result);

// Parse input and immediately evaluate it, returning the result value.
caValue* evaluate(EvalContext* context, Branch* branch, std::string const& input);
caValue* evaluate(Branch* branch, Term* function, List* inputs);
caValue* evaluate(Term* function, List* inputs);

caValue* get_input(EvalContext* context, Term* term);
caValue* get_input(EvalContext* context, int index);
void consume_input(EvalContext* context, Term* term, caValue* dest);
void consume_input(EvalContext* context, int index, caValue* dest);
bool consume_cast(EvalContext* context, int index, Type* type, caValue* dest);
int num_inputs(EvalContext* context);
void consume_inputs_to_list(EvalContext* context, List* list);
caValue* get_output(EvalContext* context, int index);

Term* current_term(EvalContext* context);
Branch* current_branch(EvalContext* context);
caValue* get_register(EvalContext* context, Term* term);

// Create an output value for the current term, using the declared type's
// initialize function.
void create_output(EvalContext* context);

// Signal that a runtime error has occurred.
void raise_error(EvalContext* context, Term* term, caValue* output, const char* msg);
void raise_error(EvalContext* context, const char* msg);
void raise_error(EvalContext* context, std::string const& msg);

void print_runtime_error_formatted(EvalContext* context, std::ostream& output);

// Returns whether evaluation has been interrupted, such as with a 'return' or
// 'break' statement, or a runtime error.
bool error_occurred(EvalContext* context);

void clear_error(EvalContext* cxt);

std::string context_get_error_message(EvalContext* cxt);
void context_print_error_stack(std::ostream& out, EvalContext* cxt);

void run_interpreter(EvalContext* context);

} // namespace circa
