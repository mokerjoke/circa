// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include <vector>

#include "common_headers.h"

#pragma once

// inspection.h
//
// Pure functions for inspecting and performing queries on compiled code.

namespace circa {


// Check the function and inputs of 'user', returns whether they are actually
// using 'usee'.
bool is_actually_using(Term* user, Term* usee);

int user_count(Term* term);

// Return the static type of 'term'.
Type* declared_type(Term* term);

// Set / get whether this term is a 'statement'. This affects source code reproduction.
void set_is_statement(Term* term, bool value);
bool is_statement(Term* term);

// Various queries on a call site.
bool is_comment(Term* term);
bool is_empty_comment(Term* term);
bool is_value(Term* term);
bool is_hidden(Term* term);
bool has_empty_name(Term* term);
bool is_copying_call(Term* term);

bool is_an_unknown_identifier(Term* term);

// Checks if term->nestedContents is a major branch. A 'major' branch has its own stack
// frame when executed.
bool is_major_branch(Term* term);
bool is_major_branch(Branch* branch);
bool is_minor_branch(Branch* branch);

bool has_variable_args(Branch* branch);
int get_locals_count(Branch* branch);

// Input & output placeholders
Term* get_input_placeholder(Branch* branch, int index);
Term* get_output_placeholder(Branch* branch, int index);
int count_input_placeholders(Branch* branch);
int count_output_placeholders(Branch* branch);
int input_placeholder_index(Term* inputPlaceholder);
bool is_input_placeholder(Term* term);
bool is_output_placeholder(Term* term);

// Extra outputs
Term* get_output_term(Term* term, int index);
Term* get_extra_output(Term* term, int index);
Term* find_extra_output_for_state(Term* term);

// Stateful inputs & outputs
bool term_is_state_input(Term* term, int index);
Term* find_state_input(Branch* branch);
bool has_state_input(Branch* branch);
Term* find_state_output(Branch* branch);
bool has_state_output(Branch* branch);
bool is_state_input(Term* placeholder);
bool is_state_output(Term* placeholder);

// Accessors that use a term's 'details' branch, which may be the nested branch,
// or it might be the function's branch, depending on the function.
Branch* term_get_function_details(Term* call);
Term* term_get_input_placeholder(Term* call, int index);
int term_count_input_placeholders(Term* term);
Term* term_get_output_placeholder(Term* call, int index);
bool term_has_variable_args(Term* term);

// Return a count of 'actual' output terms (includes the term plus any adjacent
// extra_output terms).
int count_actual_output_terms(Term* term);

// Preceding term in the same branch (may be NULL).
Term* preceding_term(Term* term);

// Following term in the same branch (may be NULL).
Term* following_term(Term* term);

Term* find_input_placeholder_with_name(Branch* branch, const char* name);
Term* find_output_placeholder_with_name(Branch* branch, const char* name);

Term* find_last_non_comment_expression(Branch* branch);
Term* find_term_with_function(Branch* branch, Term* func);
Term* find_input_placeholder_with_name(Branch* branch, const char* name);
Term* find_input_with_function(Term* target, Term* func);
Term* find_user_with_function(Term* target, Term* func);

// Search upwards starting at 'term', and returns the parent (or the term itself) found
// in 'branch'. Returns NULL if not found.
Term* find_parent_term_in_branch(Term* term, Branch* branch);

bool has_an_error_listener(Term* term);

// Format the term's global id as a string that looks like: $ab3
std::string global_id(Term* term);

// Returns the UniqueName, a name that's unique within the term's branch.
const char* unique_name(Term* term);

std::string get_short_local_name(Term* term);

std::string branch_namespace_to_string(Branch* branch);

// Print compiled code in a raw format
struct RawOutputPrefs
{
    int indentLevel;
    bool showAllIDs;
    bool showProperties;
    bool showBytecode;
    RawOutputPrefs() : indentLevel(0), showAllIDs(false), showProperties(false),
        showBytecode(false) {}
};


void print_branch(std::ostream& out, Branch* branch, RawOutputPrefs* prefs);
void print_term(std::ostream& out, Term* term, RawOutputPrefs* prefs);
void print_term(std::ostream& out, Term* term);

// Convenient overloads for raw format printing
void print_branch(std::ostream& out, Branch* branch);
void print_branch_with_properties(std::ostream& out, Branch* branch);
std::string get_branch_raw(Branch* branch);
std::string get_term_to_string_extended(Term*);
std::string get_term_to_string_extended_with_props(Term*);

// Print a short source-code location for this term.
std::string get_short_location(Term* term);

// Print the source file that this term came from, if any.
std::string get_source_filename(Term* term);

void list_names_that_this_branch_rebinds(Branch* branch, std::vector<std::string> &names);

// Get a list of the set of terms which descend from 'inputs', and which have 'outputs'
// as descendants.
TermList get_involved_terms(TermList inputs, TermList outputs);

typedef bool (*NamedTermVisitor) (Term* term, const char* name, caValue* context);
void visit_name_accessible_terms(Term* location, NamedTermVisitor visitor, caValue* context);

}
