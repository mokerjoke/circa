// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

#include "common_headers.h"

#include "circa/file.h"

#include "branch.h"
#include "building.h"
#include "kernel.h"
#include "code_iterators.h"
#include "dll_loading.h"
#include "evaluation.h"
#include "file_utils.h"
#include "function.h"
#include "importing_macros.h"
#include "inspection.h"
#include "list.h"
#include "parser.h"
#include "stateful_code.h"
#include "source_repro.h"
#include "static_checking.h"
#include "string_type.h"
#include "names.h"
#include "tagged_value.h"
#include "term.h"
#include "type.h"
#include "update_cascades.h"

namespace circa {

static unsigned int g_nextBranchID = 1;

void on_branch_created(Branch* branch)
{
    // No-op, used for debugging.
}

void assert_valid_branch(Branch const* obj)
{
    // this once did something
}

Branch::Branch()
  : owningTerm(NULL),
    version(0),
    inProgress(false),
    stateType(NULL),
    emptyEvaluation(false)
{
    id = g_nextBranchID++;
    gc_register_new_object((CircaObject*) this, &BRANCH_T, true);

    on_branch_created(this);
}

Branch::~Branch()
{
    clear_branch(this);
    gc_on_object_deleted((CircaObject*) this);
}

Branch* alloc_branch_gc()
{
    Branch* branch = new Branch();
    gc_mark_object_referenced(&branch->header);
    gc_set_object_is_root(&branch->header, false);
    return branch;
}

void branch_list_references(CircaObject* object, GCReferenceList* list, GCColor color)
{
    Branch* branch = (Branch*) object;

    // Follow each term
    for (int i=0; i < branch->length(); i++) {
        Term* term = branch->get(i);
        gc_mark(list, (CircaObject*) term->type, color);
        gc_mark(list, (CircaObject*) term->nestedContents, color);
    }
}

std::string branch_to_string(caValue* val)
{
    Branch* branch = as_branch(val);
    if (branch == NULL)
        return "Branch#null";
    else {
        std::stringstream s;
        s << "Branch#";
        s << branch->id;
        return s.str();
    }
}

void branch_setup_type(Type* type)
{
    type->name = name_from_string("Branch");
    type->toString = branch_to_string;
    type->gcListReferences = branch_list_references;
}

int Branch::length()
{
    assert_valid_branch(this);
    return _terms.length();
}

bool Branch::contains(std::string const& name)
{
    return get(name) != NULL;
}

Term* Branch::get(int index)
{
    assert_valid_branch(this);
    ca_test_assert(index < length());
    return _terms[index];
}
Term* Branch::getSafe(int index)
{
    if (index >= length())
        return NULL;
    return _terms[index];
}

Term* Branch::getFromEnd(int index)
{
    return get(length() - index - 1);
}

Term* Branch::last()
{
    if (length() == 0) return NULL;
    else return _terms[length()-1];
}

int Branch::getIndex(Term* term)
{
    ca_assert(term != NULL);
    ca_assert(term->owningBranch == this);
    assert_valid_term(term);

    return term->index;
}

void Branch::append(Term* term)
{
    assert_valid_branch(this);
    _terms.append(term);
    if (term != NULL) {
        assert_valid_term(term);
        ca_assert(term->owningBranch == NULL);
        term->owningBranch = this;
        term->index = _terms.length()-1;
    }
}

Term* Branch::appendNew()
{
    assert_valid_branch(this);
    Term* term = alloc_term();
    ca_assert(term != NULL);
    _terms.append(term);
    term->owningBranch = this;
    term->index = _terms.length()-1;
    return term;
}

void Branch::set(int index, Term* term)
{
    assert_valid_branch(this);
    ca_assert(index <= length());

    // No-op if this is the same term.
    if (_terms[index] == term)
        return;

    setNull(index);
    _terms.setAt(index, term);
    if (term != NULL) {
        assert_valid_term(term);
        ca_assert(term->owningBranch == NULL || term->owningBranch == this);
        term->owningBranch = this;
        term->index = index;
    }
}

void Branch::setNull(int index)
{
    assert_valid_branch(this);
    ca_assert(index <= length());
    Term* term = _terms[index];
    if (term != NULL)
        erase_term(term);
}

void Branch::insert(int index, Term* term)
{
    assert_valid_term(term);
    assert_valid_branch(this);
    ca_assert(index >= 0);
    ca_assert(index <= _terms.length());

    _terms.append(NULL);
    for (int i=_terms.length()-1; i > index; i--) {
        _terms.setAt(i, _terms[i-1]);
        _terms[i]->index = i;
    }
    _terms.setAt(index, term);

    if (term != NULL) {
        ca_assert(term->owningBranch == NULL);
        term->owningBranch = this;
        term->index = index;
    }
}

void Branch::move(Term* term, int index)
{
    ca_assert(term->owningBranch == this);

    if (term->index == index)
        return;

    int dir = term->index < index ? 1 : -1;

    for (int i=term->index; i != index; i += dir) {
        _terms.setAt(i, _terms[i+dir]);
        if (_terms[i] != NULL)
            _terms[i]->index = i;
    }
    _terms.setAt(index, term);
    term->index = index;
}

void Branch::moveToEnd(Term* term)
{
    assert_valid_term(term);
    ca_assert(term != NULL);
    ca_assert(term->owningBranch == this);
    ca_assert(term->index >= 0);
    int index = getIndex(term);
    _terms.append(term);
    _terms.setAt(index, NULL);
    term->index = _terms.length()-1;
}

void Branch::remove(int index)
{
    remove_term(get(index));
}

void Branch::remove(std::string const& name)
{
    if (!names.contains(name))
        return;

    Term* term = names[name];
    remove_term(term);
}

void Branch::removeNulls()
{
    int numDeleted = 0;
    for (int i=0; i < _terms.length(); i++) {
        if (_terms[i] == NULL) {
            numDeleted++;
        } else if (numDeleted > 0) {
            _terms.setAt(i - numDeleted, _terms[i]);
            _terms[i - numDeleted]->index = i - numDeleted;
        }
    }

    if (numDeleted > 0)
        _terms.resize(_terms.length() - numDeleted);
}

void Branch::removeNameBinding(Term* term)
{
    if (!has_empty_name(term) && names[term->name] == term)
        names.remove(term->name);
}

void Branch::shorten(int newLength)
{
    for (int i=newLength; i < length(); i++)
        set(i, NULL);

    removeNulls();
}

void
Branch::clear()
{
    clear_branch(this);
}

Term* Branch::findFirstBinding(Name name)
{
    for (int i = 0; i < _terms.length(); i++) {
        if (_terms[i] == NULL)
            continue;
        if (_terms[i]->nameSymbol == name)
            return _terms[i];
    }

    return NULL;
}

void Branch::bindName(Term* term, Name name)
{
    assert_valid_term(term);
    if (!has_empty_name(term) && term->nameSymbol != name) {
        internal_error(std::string("term already has a name: ") + term->nameStr());
    }

    names.bind(term, name_to_string(name));
    term->nameSymbol = name;
    term->name = name_to_string(name);
    update_unique_name(term);
}

void Branch::remapPointers(TermMap const& map)
{
    names.remapPointers(map);

    for (int i = 0; i < _terms.length(); i++) {
        Term* term = _terms[i];
        if (term != NULL)
            remap_pointers(term, map);
    }
}

std::string Branch::toString()
{
    std::stringstream out;
    out << "[";
    for (int i=0; i < length(); i++) {
        Term* term = get(i);
        if (i > 0) out << ", ";
        if (!has_empty_name(term))
            out << term->nameStr() << ": ";
        out << term->toString();
    }
    out << "]";
    return out.str();
}

Term*
Branch::compile(std::string const& code)
{
    return parser::compile(this, parser::statement_list, code);
}

Term*
Branch::eval(std::string const& code)
{
    return parser::evaluate(this, parser::statement_list, code);
}

bool is_namespace(Term* term)
{
    return term->function == FUNCS.namespace_func;
}

bool is_namespace(Branch* branch)
{
    return branch->owningTerm != NULL && is_namespace(branch->owningTerm);
}

bool has_nested_contents(Term* term)
{
    return term->nestedContents != NULL;
}

Branch* nested_contents(Term* term)
{
    if (term->nestedContents == NULL) {
        term->nestedContents = new Branch();
        term->nestedContents->owningTerm = term;
    }
    return term->nestedContents;
}

void remove_nested_contents(Term* term)
{
    if (term->nestedContents == NULL)
        return;

    Branch* branch = term->nestedContents;
    clear_branch(term->nestedContents);

    // Delete this Branch immediately, if it's not referenced.
    if (!branch->header.referenced)
        delete term->nestedContents;

    term->nestedContents = NULL;
}

caValue* branch_get_source_filename(Branch* branch)
{
    List* fileOrigin = branch_get_file_origin(branch);

    if (fileOrigin == NULL)
        return NULL;

    return fileOrigin->get(1);
}

std::string get_branch_source_filename(Branch* branch)
{
    caValue* val = branch_get_source_filename(branch);
    
    if (val == NULL || !is_string(val))
        return "";
    else
        return as_string(val);
}

Branch* get_outer_scope(Branch* branch)
{
    if (branch->owningTerm == NULL)
        return NULL;
    return branch->owningTerm->owningBranch;
}

void pre_erase_term(Term* term)
{
    // If this term declares a Type, then clear the Type.declaringTerm pointer
    // before it becomes invalid.
    if (is_type(term) && as_type(term_value(term))->declaringTerm == term)
        as_type(term_value(term))->declaringTerm = NULL;

    // Ditto for Function
    if (is_function(term) && as_function(term_value(term))->declaringTerm == term)
        as_function(term_value(term))->declaringTerm = NULL;
}

void erase_term(Term* term)
{
    assert_valid_term(term);

    pre_erase_term(term);

    set_null(term_value(term));
    set_inputs(term, TermList());
    change_function(term, NULL);
    term->type = NULL;
    remove_nested_contents(term);

    // for each user, clear that user's input list of this term
    remove_from_any_user_lists(term);
    clear_from_dependencies_of_users(term);

    if (term->owningBranch != NULL) {
        // remove name binding if necessary
        term->owningBranch->removeNameBinding(term);

        // index may be invalid if something bad has happened
        ca_assert(term->index < term->owningBranch->length());
        term->owningBranch->_terms.setAt(term->index, NULL);

        term->owningBranch = NULL;
        term->index = -1;
    }

    dealloc_term(term);
}

void clear_branch(Branch* branch)
{
    assert_valid_branch(branch);
    set_null(&branch->staticErrors);
    branch->stateType = NULL;

    branch->names.clear();
    branch->inProgress = false;

    // Iterate through the branch and tear down any term references, so that we
    // don't have to worry about stale pointers later.
    for (BranchIterator it(branch); it.unfinished(); ++it) {
        if (*it == NULL)
            continue;

        pre_erase_term(*it);
        set_inputs(*it, TermList());
        remove_from_any_user_lists(*it);
        change_function(*it, NULL);
    }

    for (int i= branch->_terms.length() - 1; i >= 0; i--) {
        Term* term = branch->get(i);
        if (term == NULL)
            continue;

        if (term->nestedContents)
            clear_branch(term->nestedContents);
    }

    for (int i = branch->_terms.length() - 1; i >= 0; i--) {
        Term* term = branch->get(i);
        if (term == NULL)
            continue;

        // Delete any leftover users, mark them as repairable.
        for (int userIndex = 0; userIndex < term->users.length(); userIndex++) {
            Term* user = term->users[userIndex];
            for (int depIndex = 0; depIndex < user->numDependencies(); depIndex++) {
                if (user->dependency(depIndex) == term) {
                    // mark_repairable_link(user, term->name, depIndex);
                    user->setDependency(depIndex, NULL);
                }
            }
        }

        erase_term(term);
    }

    branch->_terms.clear();
}

Term* find_term_by_id(Branch* branch, int id)
{
    for (BranchIterator it(branch); !it.finished(); it.advance()) {
        if (*it == NULL)
            continue;

        if (it->id == id)
            return *it;
    }

    return NULL;
}

void duplicate_branch_nested(TermMap& newTermMap, Branch* source, Branch* dest)
{
    // Duplicate every term
    for (int index=0; index < source->length(); index++) {
        Term* source_term = source->get(index);

        Term* dest_term = create_duplicate(dest, source_term, source_term->name);

        newTermMap[source_term] = dest_term;

        // duplicate nested contents
        clear_branch(nested_contents(dest_term));
        duplicate_branch_nested(newTermMap,
                nested_contents(source_term), nested_contents(dest_term));
    }
}

void duplicate_branch(Branch* source, Branch* dest)
{
    assert_valid_branch(source);
    assert_valid_branch(dest);

    TermMap newTermMap;

    duplicate_branch_nested(newTermMap, source, dest);

    // Remap pointers
    for (int i=0; i < dest->length(); i++)
        remap_pointers(dest->get(i), newTermMap);

    // Include/overwrite names
    dest->names.append(source->names);
    dest->names.remapPointers(newTermMap);
}

Name load_script(Branch* branch, const char* filename)
{
    // Store the file origin
    caValue* origin = &branch->origin;
    set_list(origin, 3);
    set_name(list_get(origin, 0), name_File);
    set_string(list_get(origin, 1), filename);
    set_int(list_get(origin, 2), circa_file_get_version(filename));

    // Read the text file
    circa::Value contents;
    circa_read_file(filename, &contents);

    if (is_null(&contents)) {
        Term* msg = create_string(branch, "file not found");
        apply(branch, FUNCS.static_error, TermList(msg));
        return name_Failure;
    }

    parser::compile(branch, parser::statement_list, as_cstring(&contents));

    post_module_load(branch);

    return name_Success;
}

void post_module_load(Branch* branch)
{
    // Post-load steps
    dll_loading_check_for_patches_on_loaded_branch(branch);
}

Branch* include_script(Branch* branch, const char* filename)
{
    ca_assert(branch != NULL);
    Term* filenameTerm = create_string(branch, filename);
    Term* includeFunc = apply(branch, FUNCS.include_func, TermList(filenameTerm));
    return nested_contents(includeFunc);
}

Branch* load_script_term(Branch* branch, const char* filename)
{
    ca_assert(branch != NULL);
    Term* filenameTerm = create_string(branch, filename);
    Term* includeFunc = apply(branch, FUNCS.load_script, TermList(filenameTerm));
    return nested_contents(includeFunc);
}

#if 0
void save_script(Branch* branch)
{
    std::string text = get_branch_source_text(branch);
    List* fileOrigin = branch_get_file_origin(branch);
    if (fileOrigin == NULL)
        return;

    write_text_file(as_cstring(fileOrigin->get(1)), text.c_str());
}

void persist_branch_to_file(Branch* branch)
{
    std::string filename = get_branch_source_filename(branch);
    std::string contents = get_branch_source_text(branch);
    write_text_file(filename.c_str(), contents.c_str());
}
#endif

std::string get_source_file_location(Branch* branch)
{
    // Search upwards until we find a branch that has source-file defined.
    while (branch != NULL && get_branch_source_filename(branch) == "") {
        if (branch->owningTerm == NULL)
            branch = NULL;
        else
            branch = branch->owningTerm->owningBranch;
    }

    if (branch == NULL)
        return "";

    caValue* sourceFilename = branch_get_source_filename(branch);

    if (sourceFilename == NULL)
        return "";

    Value directory;
    circa_get_directory_for_filename(sourceFilename, &directory);

    return as_string(&directory);
}

List* branch_get_file_origin(Branch* branch)
{
    if (!is_list(&branch->origin))
        return NULL;

    List* list = (List*) &branch->origin;

    if (list->length() != 3)
        return NULL;

    if (as_name(list->get(0)) != name_File)
        return NULL;

    return list;
}

bool check_and_update_file_origin(Branch* branch, const char* filename)
{
    int version = circa_file_get_version(filename);

    caValue* origin = branch_get_file_origin(branch);

    if (origin == NULL) {
        origin = &branch->origin;
        set_list(origin, 3);
        set_name(list_get(origin, 0), name_File);
        set_string(list_get(origin, 1), filename);
        set_int(list_get(origin, 2), version);
        return true;
    }

    if (!equals_string(list_get(origin, 1), filename)) {
        touch(origin);
        set_string(list_get(origin, 1), filename);
        set_int(list_get(origin, 2), version);
        return true;
    }

    if (!equals_int(list_get(origin, 2), version)) {
        touch(origin);
        set_int(list_get(origin, 2), version);
        return true;
    }

    return false;
}

Branch* load_latest_branch(Branch* branch)
{
    caValue* fileOrigin = branch_get_file_origin(branch);
    if (fileOrigin == NULL)
        return branch;

    std::string filename = as_string(list_get(fileOrigin, 1));

    bool fileChanged = check_and_update_file_origin(branch, filename.c_str());

    if (!fileChanged)
        return branch;

    Branch* newBranch = alloc_branch_gc();
    load_script(newBranch, filename.c_str());

    update_static_error_list(newBranch);

    // New branch starts off with the old branch's version, plus 1.
    newBranch->version = branch->version + 1;

    return newBranch;
}

void append_internal_error(caValue* result, int index, std::string const& message)
{
    const int INTERNAL_ERROR_TYPE = 1;

    caValue* error = list_append(result);
    set_list(error, 3);
    set_int(list_get(error, 0), INTERNAL_ERROR_TYPE);
    set_int(list_get(error, 1), index);
    set_string(list_get(error, 2), message);
}

void branch_check_invariants(caValue* result, Branch* branch)
{
    set_list(result, 0);

    for (int i=0; i < branch->length(); i++) {
        Term* term = branch->get(i);

        if (term == NULL) {
            append_internal_error(result, i, "NULL pointer");
            continue;
        }

        // Check that the term's index is correct
        if (term->index != i) {
            std::stringstream msg;
            msg << "Wrong index (found " << term->index << ", expected " << i << ")";
            append_internal_error(result, i, msg.str());
        }

        // Check that owningBranch is correct
        if (term->owningBranch != branch)
            append_internal_error(result, i, "Wrong owningBranch");
    }
} 

bool branch_check_invariants_print_result(Branch* branch, std::ostream& out)
{
    circa::Value result;
    branch_check_invariants(&result, branch);

    if (list_length(&result) == 0)
        return true;

    out << list_length(&result) << " errors found in branch " << &branch
        << std::endl;

    for (int i=0; i < list_length(&result); i++) {
        List* error = List::checkCast(list_get(&result,i));
        out << "[" << as_int(error->get(1)) << "] ";
        out << as_cstring(error->get(2));
        out << std::endl;
    }

    out << "contents:" << std::endl;
    print_branch(out, branch);

    return false;
}

void branch_link_missing_functions(Branch* branch, Branch* source)
{
    for (BranchIterator it(branch); it.unfinished(); it.advance()) {
        Term* term = *it;
        if (term->function == NULL || term->function == FUNCS.unknown_function) {
            std::string funcName = term->stringProp("syntax:functionName", "");

            if (funcName == "")
                continue;

            // try to find this function
            Term* func = find_local_name(source, funcName.c_str(), NAME_LOOKUP_FUNCTION);

            if (func != NULL)
                change_function(term, func);
        }
    }
}

} // namespace circa
