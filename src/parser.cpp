// Copyright 2008 Andrew Fischer

#include "common_headers.h"

#include "circa.h"

namespace circa {
namespace parser {

using namespace circa::tokenizer;

Ref compile(Branch* branch, ParsingStep step, std::string const& input)
{
    // if branch is NULL, use a temporary branch
    bool temporaryBranch = false;
    if (branch == NULL) {
        branch = new Branch();
        temporaryBranch = true;
    }

    TokenStream tokens(input);
    Ref result = step(*branch, tokens);
    remove_compilation_attrs(*branch);
    wrap_up_branch(*branch);

    if (temporaryBranch) {
        branch->clear();
        delete branch;
    }

    return result;
}

Term* compile_statement(Branch& branch, std::string const& input)
{
    TokenStream tokens(input);

    return statement(branch, tokens);
}

Term* evaluate_statement(Branch& branch, std::string const& input)
{
    int previousLastIndex = branch.length();

    Term* result = compile_statement(branch, input);

    // Evaluate all terms that were just created
    for (int i=previousLastIndex; i < branch.length(); i++)
        evaluate_term(branch[i]);

    return result;
}

// This structure stores the syntax hints for list-like syntax. It exists because
// you usually don't have a comprehension term while you are parsing the list
// arguments, so you need to temporarily store syntax hints until you create one.
struct ListSyntaxHints {
    struct Input {
        int index;
        std::string field;
        std::string value;
        Input(int i, std::string const& f, std::string const& v)
            : index(i), field(f), value(v) {}
    };

    void set(int index, std::string const& field, std::string const& value)
    {
        mPending.push_back(Input(index, field, value));
    }

    void append(int index, std::string const& field, std::string const& value)
    {
        // try to find a matching entry
        std::vector<Input>::iterator it;

        for (it = mPending.begin(); it != mPending.end(); ++it) {
            if (it->index == index && it->field == field) {
                it->value += value;
                return;
            }
        }

        // otherwise make a new one
        set(index, field, value);
    }

    void apply(Term* term)
    {
        std::vector<Input>::const_iterator it;
        for (it = mPending.begin(); it != mPending.end(); ++it)
            get_input_syntax_hint(term, it->index, it->field) = it->value;
    }

    std::vector<Input> mPending;
};

// Consumes a list of terms that are separated by spaces, or commas, or semicolons.
// The result terms are appened to list_out. The syntax hints are appended to
// hints_out. (you should apply these syntax hints to the resulting comprehension
// term)
//
// This is used to parse the syntax of function arguments, member function arguments,
// and literal lists.
void consume_list_arguments(Branch& branch, TokenStream& tokens,
        RefList& list_out, ListSyntaxHints& hints_out)
{
    int index = 0;
    while (!tokens.nextIs(RPAREN) && !tokens.nextIs(RBRACKET) && !tokens.finished()) {

        hints_out.set(index, "preWhitespace", possible_whitespace_or_newline(tokens));
        Term* term = infix_expression(branch, tokens);
        hints_out.set(index, "postWhitespace", possible_whitespace_or_newline(tokens));

        list_out.append(term);

        if (tokens.nextIs(COMMA))
            hints_out.append(index, "postWhitespace", tokens.consume());
        else if (tokens.nextIs(SEMICOLON))
            hints_out.append(index, "postWhitespace", tokens.consume());

        index++;
    }
}

// Parsing functions:

Term* statement_list(Branch& branch, TokenStream& tokens)
{
    Term* term = NULL;

    while (!tokens.finished())
        term = statement(branch, tokens);

    return term;
}

Term* statement(Branch& branch, TokenStream& tokens)
{
    std::string preWhitespace = possible_whitespace(tokens);

    Term* result = NULL;

    // Comment
    if (tokens.nextIs(COMMENT)) {
        result = comment_statement(branch, tokens);
        assert(result != NULL);
    }

    // Blank line
    else if (tokens.finished() || tokens.nextIs(NEWLINE)) {
        result = blank_line(branch, tokens);
        assert(result != NULL);
    }

    // Function decl
    else if (tokens.nextIs(DEF)) {
        result = function_decl(branch, tokens);
    }

    // Type decl
    else if (tokens.nextIs(TYPE)) {
        result = type_decl(branch, tokens);
    }

    // If block
    else if (tokens.nextIs(IF)) {
        result = if_block(branch, tokens);
    }

    // For block
    else if (tokens.nextIs(FOR)) {
        result = for_block(branch, tokens);
    }

    // Stateful value decl
    else if (tokens.nextIs(STATE)) {
        result = stateful_value_decl(branch, tokens);
    }

    // Return statement
    else if (tokens.nextIs(RETURN)) {
        result = return_statement(branch, tokens);
    }

    // Expression statement
    else {
        result = expression_statement(branch, tokens);
        assert(result != NULL);
    }

    prepend_whitespace(result, preWhitespace);

    append_whitespace(result, possible_whitespace(tokens));

    // Consume a newline or ;
    if (tokens.nextIs(NEWLINE))
        append_whitespace(result, tokens.consume());
    else if (tokens.nextIs(SEMICOLON))
        append_whitespace(result, tokens.consume());

    return result;
}

Term* comment_statement(Branch& branch, TokenStream& tokens)
{
    std::string commentText = tokens.consume(COMMENT);

    Term* result = apply(&branch, COMMENT_FUNC, RefList());
    result->stringProp("comment") = commentText;

    return result;
}

Term* blank_line(Branch& branch, TokenStream& tokens)
{
    if (!tokens.finished())
        tokens.consume(NEWLINE);

    Term* result = apply(&branch, COMMENT_FUNC, RefList());
    result->stringProp("comment") = "\n";
    return result;
}

Term* function_from_header(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    if (tokens.nextIs(DEF))
        tokens.consume(DEF);

    possible_whitespace(tokens);

    if (!tokens.nextIs(IDENTIFIER)
            // A few builtin functions have names which are keywords:
            && !tokens.nextIs(FOR) && !tokens.nextIs(IF))
        return compile_error_for_line(branch, tokens, startPosition);

    std::string functionName = tokens.consume();

    possible_whitespace(tokens);

    if (!tokens.nextIs(LPAREN))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume();

    Term* result = create_value(&branch, FUNCTION_TYPE, functionName);
    result->stringProp("syntaxHints:declarationStyle") = "literal";
    Function& func = as_function(result);

    func.name = functionName;
    func.outputType = VOID_TYPE;

    while (!tokens.nextIs(RPAREN) && !tokens.finished())
    {
        bool isHiddenStateArgument = false;

        possible_whitespace(tokens);

        // check for 'state' keyword
        if (tokens.nextIs(STATE)) {
            tokens.consume(STATE);
            possible_whitespace(tokens);
            isHiddenStateArgument = true;
        }

        if (!tokens.nextIs(IDENTIFIER))
            return compile_error_for_line(result, tokens, startPosition);

        std::string type = tokens.consume();
        possible_whitespace(tokens);

        std::string name;
        
        if (tokens.nextIs(IDENTIFIER)) {
            name = tokens.consume();
            possible_whitespace(tokens);
        } else {
            name = get_placeholder_name_for_index(func.inputProperties.size());
        }

        Term* typeTerm = find_type(branch, type);

        func.appendInput(typeTerm, name);

        if (isHiddenStateArgument)
            func.hiddenStateType = typeTerm;

        // Variable args when ... is appended
        if (tokens.nextIs(ELLIPSIS)) {
            tokens.consume();
            func.variableArgs = true;
        }

        if (!tokens.nextIs(RPAREN)) {
            if (!tokens.nextIs(COMMA))
                return compile_error_for_line(result, tokens, startPosition);

            tokens.consume(COMMA);
        }
    }

    assert(tokens.nextIs(RPAREN));
    tokens.consume();

    possible_whitespace(tokens);

    if (tokens.nextIs(COLON)) {
        tokens.consume(COLON);
        possible_whitespace(tokens);

        if (!tokens.nextIs(IDENTIFIER))
            return compile_error_for_line(result, tokens, startPosition);

        std::string outputTypeName = tokens.consume();
        Term* outputType = find_type(branch, outputTypeName);
        func.outputType = outputType;
    }

    possible_whitespace(tokens);
    possible_newline(tokens);

    return result;
}

Term* function_decl(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    Term* result = create_value(&branch, SUBROUTINE_TYPE);

    Branch& subBranch = as_branch(result);

    Term* functionDef = function_from_header(subBranch, tokens);

    if (has_compile_error(functionDef)) {
        std::string message = functionDef->stringProp("message");
        change_function(result, UNRECOGNIZED_EXPRESSION_FUNC);
        result->stringProp("message") = message;
        return result;
    }

    // Remove the name that function_from_header applied
    rename(functionDef, get_name_for_attribute("function-def"));

    initialize_subroutine(result);

    consume_branch_until_end(subBranch, tokens);
    remove_compilation_attrs(subBranch);

    if (!tokens.nextIs(END))
        return compile_error_for_line(result, tokens, startPosition);

    tokens.consume(END);

    Function& func = as_function(functionDef);
    branch.bindName(result, func.name);

    assert(is_value(result));
    assert(is_subroutine(result));

    return result;
}

Term* type_decl(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    if (tokens.nextIs(TYPE))
        tokens.consume();

    possible_whitespace(tokens);

    if (!tokens.nextIs(IDENTIFIER))
        return compile_error_for_line(branch, tokens, startPosition);

    std::string name = tokens.consume();

    Term* result = create_value(&branch, TYPE_TYPE, name);
    Type& type = as_type(result);
    initialize_compound_type(type);
    type.name = name;

    possible_whitespace_or_newline(tokens);

    if (!tokens.nextIs(LBRACE))
        return compile_error_for_line(result, tokens, startPosition);

    tokens.consume();

    while (!tokens.nextIs(RBRACE)) {
        possible_whitespace_or_newline(tokens);

        if (tokens.nextIs(RBRACE))
            break;

        if (!tokens.nextIs(IDENTIFIER))
            return compile_error_for_line(result, tokens, startPosition);

        std::string fieldTypeName = tokens.consume(IDENTIFIER);
        possible_whitespace(tokens);

        if (!tokens.nextIs(IDENTIFIER))
            return compile_error_for_line(result, tokens, startPosition);

        std::string fieldName = tokens.consume(IDENTIFIER);
        possible_whitespace_or_newline(tokens);

        Term* fieldType = find_type(branch, fieldTypeName);

        type.addField(fieldType, fieldName);

        if (tokens.nextIs(COMMA))
            tokens.consume(COMMA);
    }

    tokens.consume(RBRACE);

    return result;
}

Term* if_block(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    tokens.consume(IF);
    possible_whitespace(tokens);

    Term* condition = infix_expression(branch, tokens);
    assert(condition != NULL);

    recursively_mark_terms_as_occuring_inside_an_expression(condition);

    possible_whitespace(tokens);

    if (!tokens.nextIs(NEWLINE))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(NEWLINE);

    Term* result = apply(&branch, IF_FUNC, RefList(condition));
    alloc_value(result);
    Branch& innerBranch = as_branch(result);

    consume_branch_until_end(innerBranch, tokens);
    remove_compilation_attrs(innerBranch);

    Term* elseResult = NULL;

    // possibly consume an 'else' block
    if (tokens.nextIs(ELSE)) {
        tokens.consume(ELSE);

        Term* notCondition = apply(&branch, NOT_FUNC, RefList(condition));
        elseResult = apply(&branch, IF_FUNC, RefList(notCondition));
        alloc_value(elseResult);
        Branch& elseInnerBranch = as_branch(elseResult);

        consume_branch_until_end(elseInnerBranch, tokens);
        remove_compilation_attrs(elseInnerBranch);
    }

    if (!tokens.nextIs(END))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(END);

    // Create the joining branch
    Term* joining = apply(&branch, BRANCH_FUNC, RefList(), "#joining");
    alloc_value(joining);
    Branch& joiningBranch = *get_inner_branch(joining);

    // Get a list of all names bound in this branch
    std::set<std::string> boundNames;

    {
        TermNamespace::const_iterator it;
        for (it = innerBranch.names.begin(); it != innerBranch.names.end(); ++it)
            boundNames.insert(it->first);

        if (elseResult != NULL) {
            Branch& elseInnerBranch = as_branch(elseResult);
            for (it = elseInnerBranch.names.begin(); it != elseInnerBranch.names.end(); ++it)
                boundNames.insert(it->first);
        }
    }

    // Ignore any names which are not bound in the outer branch
    {
        std::set<std::string>::iterator it;
        for (it = boundNames.begin(); it != boundNames.end();)
        {
            std::string const& name = *it;

            if (find_named(&branch, name) == NULL)
                boundNames.erase(it++);

            // Also ignore hidden names
            else if ((name[0] == '#') && (name != OUTPUT_PLACEHOLDER_NAME))
                boundNames.erase(it++);
            else
                ++it;
        }
    }

    // For each name, create a joining term
    {
        std::set<std::string>::const_iterator it;
        for (it = boundNames.begin(); it != boundNames.end(); ++it)
        {
            std::string const& name = *it;

            Term* outerVersion = find_named(&branch, name);
            Term* positiveVersion = outerVersion;
            Term* negativeVersion = outerVersion;

            if (innerBranch.contains(name))
                positiveVersion = innerBranch[name];

            if (elseResult != NULL && as_branch(elseResult).contains(name))
                negativeVersion = as_branch(elseResult)[name];

            Term* joining = apply(&joiningBranch, "if_expr",
                    RefList(condition, positiveVersion, negativeVersion));

            // Bind these names in the outer branch
            branch.bindName(joining, name);
        }
    }

    return result;
}

Term* for_block(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    tokens.consume(FOR);
    possible_whitespace(tokens);

    if (!tokens.nextIs(IDENTIFIER))
        return compile_error_for_line(branch, tokens, startPosition);

    std::string iterator_name = tokens.consume(IDENTIFIER);
    possible_whitespace(tokens);

    if (!tokens.nextIs(IN))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(IN);
    possible_whitespace(tokens);

    Term* listExpr = infix_expression(branch, tokens);
    recursively_mark_terms_as_occuring_inside_an_expression(listExpr);
    possible_whitespace(tokens);

    if (!tokens.nextIs(NEWLINE))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(NEWLINE);

    Term* forTerm = apply(&branch, FOR_FUNC, RefList(listExpr));
    alloc_value(forTerm);

    Branch& innerBranch = get_for_loop_code(forTerm);

    // Create iterator variable
    Term* iterator = create_value(&innerBranch, ANY_TYPE, iterator_name);
    source_set_hidden(iterator, true);

    setup_for_loop_pre_code(forTerm);

    consume_branch_until_end(innerBranch, tokens);

    if (!tokens.nextIs(END))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(END);

    setup_for_loop_post_code(forTerm);

    return forTerm;
}

Term* stateful_value_decl(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    tokens.consume(STATE);
    possible_whitespace(tokens);

    if (!tokens.nextIs(IDENTIFIER))
        return compile_error_for_line(branch, tokens, startPosition);

    std::string name = tokens.consume(IDENTIFIER);
    possible_whitespace(tokens);

    Term* type = ANY_TYPE;

    // type annotation
    std::string typeName;
    if (tokens.nextIs(COLON)) {
        tokens.consume();
        possible_whitespace(tokens);

        if (!tokens.nextIs(IDENTIFIER))
            return compile_error_for_line(branch, tokens, startPosition);

        typeName = tokens.consume(IDENTIFIER);
        possible_whitespace(tokens);

        type = find_type(branch, typeName);
    }

    Term* result = NULL;

    if (tokens.nextIs(EQUALS)) {

        // state i = initial_value
        tokens.consume();
        possible_whitespace(tokens);
        Term* initialValue = infix_expression(branch, tokens);
        recursively_mark_terms_as_occuring_inside_an_expression(initialValue);
        result = apply(&branch, ONE_TIME_ASSIGN_FUNC, RefList(initialValue));
        alloc_value(result);

    } else {
        // state i
        result = create_value(&branch, type);
    }

    branch.bindName(result, name);
    set_stateful(result, true);

    return result;
}

int search_line_for_token(TokenStream& tokens, int target)
{
    int lookahead = 0;

    while (!tokens.nextIs(NEWLINE, lookahead) && lookahead < tokens.length())
    {
        if (tokens.nextIs(target, lookahead))
            return lookahead;

        lookahead++;
    }

    return -1;
}

Term* expression_statement(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    // scan this line for an = operator
    int equals_operator_loc = search_line_for_token(tokens, EQUALS);

    StringList names;

    std::string preEqualsSpace = " ";
    std::string postEqualsSpace = " ";

    if (equals_operator_loc != -1) {
        // Parse name binding(s)

        while (true) {
            if (!tokens.nextIs(IDENTIFIER))
                return compile_error_for_line(branch, tokens, startPosition);

            std::string name = tokens.consume(IDENTIFIER);

            names.append(name);

            if (!tokens.nextIs(DOT))
                break;

            tokens.consume(DOT);
        }

        preEqualsSpace = possible_whitespace(tokens);

        if (!tokens.nextIs(EQUALS))
            return compile_error_for_line(branch, tokens, startPosition);

        tokens.consume(EQUALS);
        postEqualsSpace = possible_whitespace(tokens);
    }

    Term* result = infix_expression(branch, tokens);

    for (int i=0; i < result->numInputs(); i++)
        recursively_mark_terms_as_occuring_inside_an_expression(result->input(i));

    // If this item is just an identifier, and we're trying to rename it,
    // create an implicit call to 'copy'.
    if (result->name != "" && names.length() > 0) {
        result = apply(&branch, COPY_FUNC, RefList(result));
    }

    if (preEqualsSpace != " ")
        result->stringProp("syntaxHints:preEqualsSpace") = preEqualsSpace;
    if (postEqualsSpace != " ")
        result->stringProp("syntaxHints:postEqualsSpace") = postEqualsSpace;

    std::string pendingRebind = pop_pending_rebind(branch);

    if (pendingRebind != "")
        branch.bindName(result, pendingRebind);

    if (names.length() == 1)
        branch.bindName(result, names[0]);
    else if (names.length() == 2) {

        // Field assignment
        Term* object = branch[names[0]];
        result = apply(&branch, SET_FIELD_BY_NAME_FUNC, RefList(object, result));
        result->stringProp("field-name") = names[1];

        branch.bindName(result, names[0]);

    } else if (names.length() > 2) {
        throw std::runtime_error("not yet supported: bind names with more than two qualified names.");
    }

    return result;
}

Term* return_statement(Branch& branch, TokenStream& tokens)
{
    tokens.consume(RETURN);
    possible_whitespace(tokens);

    Term* result = infix_expression(branch, tokens);
    branch.bindName(result, OUTPUT_PLACEHOLDER_NAME);
    
    return result;
}

const int HIGHEST_INFIX_PRECEDENCE = 9;

int get_infix_precedence(int match)
{
    switch(match) {
        case tokenizer::DOT:
            return 9;
        case tokenizer::COLON:
            return 8;
        case tokenizer::STAR:
        case tokenizer::SLASH:
        case tokenizer::PERCENT:
            return 7;
        case tokenizer::PLUS:
        case tokenizer::MINUS:
            return 6;
        case tokenizer::LTHAN:
        case tokenizer::LTHANEQ:
        case tokenizer::GTHAN:
        case tokenizer::GTHANEQ:
        case tokenizer::DOUBLE_EQUALS:
        case tokenizer::NOT_EQUALS:
            return 5;
        case tokenizer::DOUBLE_AMPERSAND:
        case tokenizer::DOUBLE_VERTICAL_BAR:
            return 4;
        case tokenizer::EQUALS:
        case tokenizer::PLUS_EQUALS:
        case tokenizer::MINUS_EQUALS:
        case tokenizer::STAR_EQUALS:
        case tokenizer::SLASH_EQUALS:
            return 2;
        case tokenizer::RIGHT_ARROW:
            return 1;
        case tokenizer::COLON_EQUALS:
            return 0;
        default:
            return -1;
    }
}

bool is_infix_operator_rebinding(std::string const& infix)
{
    return (infix == "+=" || infix == "-=" || infix == "*=" || infix == "/=");
}

std::string get_function_for_infix(std::string const& infix)
{
    if (infix == "+") return "add";
    else if (infix == "-") return "sub";
    else if (infix == "*") return "mult";
    else if (infix == "/") return "div";
    else if (infix == "%") return "mod";
    else if (infix == "<") return "less_than";
    else if (infix == "<=") return "less_than_eq";
    else if (infix == ">") return "greater_than";
    else if (infix == ">=") return "greater_than_eq";
    else if (infix == "==") return "equals";
    else if (infix == "||") return "or";
    else if (infix == "&&") return "and";
    else if (infix == ":=") return "assign";
    else if (infix == "+=") return "add";
    else if (infix == "-=") return "sub";
    else if (infix == "*=") return "mult";
    else if (infix == "/=") return "div";
    else if (infix == ":") return "annotate_type";
    else return "#unrecognized";
}

Term* infix_expression(Branch& branch, TokenStream& tokens)
{
    return infix_expression_nested(branch, tokens, 0);
}

Term* infix_expression_nested(Branch& branch, TokenStream& tokens, int precedence)
{
    int startPosition = tokens.getPosition();

    if (precedence > HIGHEST_INFIX_PRECEDENCE)
        return atom(branch, tokens);

    std::string preWhitespace = possible_whitespace(tokens);

    Term* leftExpr = infix_expression_nested(branch, tokens, precedence+1);

    prepend_whitespace(leftExpr, preWhitespace);

    while (!tokens.finished() && get_infix_precedence(tokens.nextNonWhitespace()) == precedence) {

        std::string preOperatorWhitespace = possible_whitespace(tokens);

        std::string operatorStr = tokens.consume();

        std::string postOperatorWhitespace = possible_whitespace(tokens);

        Term* result = NULL;

        if (operatorStr == ".") {
            // dot concatenated call

            if (!tokens.nextIs(IDENTIFIER))
                return compile_error_for_line(branch, tokens, startPosition);

            std::string rhsIdent = tokens.consume(IDENTIFIER);

            // Try to find this function
            Term* function = NULL;
           
            // Check member functions first
            Type& lhsType = as_type(leftExpr->type);

            // Field access is not very robust right now. We currently decide at compile-time
            // whether to do a member function call or a get_field, and this decision is
            // not perfect. The proper thing would be to always do get_field and then allow
            // for a call to a by-value function.

            // Another problem is that we allow for the syntax value.function, where 'function'
            // is not defined on value's type, but instead is just available from our local
            // scope. This is totally confusing and is incompatible with dynamic name-based field
            // access.

            // First, look for this field as a member function
            if (lhsType.memberFunctions.contains(rhsIdent)) {
                function = lhsType.memberFunctions[rhsIdent];

                // Consume inputs
                RefList inputs(leftExpr);
                ListSyntaxHints listHints;

                if (tokens.nextIs(LPAREN)) {
                    tokens.consume();
                    consume_list_arguments(branch, tokens, inputs, listHints);

                    if (!tokens.nextIs(RPAREN))
                        return compile_error_for_line(branch, tokens, startPosition);

                    tokens.consume(RPAREN);
                }

                result = apply(&branch, function, inputs);

                // If this is a modifying member function, then rebind the name to this
                // result.
                // Note: currently this check is flawed. The only check we do to see if this
                // is a modifying member function, is if the result type is the same. There
                // should be a more explicit way of storing this.
                if ((result->type == inputs[0]->type) && leftExpr->name != "")
                    branch.bindName(result, leftExpr->name);

            // Next, if this type defines this field
            } else if (lhsType.findFieldIndex(rhsIdent) != -1) {

                result = apply(&branch, GET_FIELD_BY_NAME_FUNC, RefList(leftExpr));
                result->stringProp("field-name") = rhsIdent;
                specialize_type(result, lhsType[rhsIdent].type);

                result->stringProp("syntaxHints:functionName") = rhsIdent;

            // Finally, look for this function in our local scope
            } else {
                function = find_function(branch, rhsIdent);

                // Consume inputs
                RefList inputs(leftExpr);

                if (tokens.nextIs(LPAREN)) {
                    tokens.consume();

                    while (!tokens.nextIs(RPAREN)) {
                        possible_whitespace(tokens);
                        Term* input = infix_expression(branch, tokens);
                        inputs.append(input);
                        possible_whitespace(tokens);

                        if (!tokens.nextIs(RPAREN))
                            tokens.consume(COMMA);
                    }
                    tokens.consume(RPAREN);
                }

                result = apply(&branch, function, inputs);
            }

            result->stringProp("syntaxHints:declarationStyle") = "dot-concat";

        } else if (operatorStr == "->") {
            if (!tokens.nextIs(IDENTIFIER))
                return compile_error_for_line(branch, tokens, startPosition);

            std::string functionName = tokens.consume(IDENTIFIER);

            RefList inputs(leftExpr);

            result = find_and_apply(branch, functionName, inputs);

            result->stringProp("syntaxHints:declarationStyle") = "arrow-concat";
            result->stringProp("syntaxHints:functionName") = functionName;

            get_input_syntax_hint(result, 0, "postWhitespace") = preOperatorWhitespace;
            get_input_syntax_hint(result, 1, "preWhitespace") = postOperatorWhitespace;

        } else {
            Term* rightExpr = infix_expression_nested(branch, tokens, precedence+1);

            std::string functionName = get_function_for_infix(operatorStr);
            bool isRebinding = is_infix_operator_rebinding(operatorStr);

            if (isRebinding && leftExpr->name == "")
                throw std::runtime_error("Left side of " + functionName + " must be an identifier");

            result = find_and_apply(branch, functionName, RefList(leftExpr, rightExpr));
            result->stringProp("syntaxHints:declarationStyle") = "infix";
            result->stringProp("syntaxHints:functionName") = operatorStr;

            get_input_syntax_hint(result, 0, "postWhitespace") = preOperatorWhitespace;
            get_input_syntax_hint(result, 1, "preWhitespace") = postOperatorWhitespace;

            if (isRebinding)
                branch.bindName(result, leftExpr->name);
        }

        leftExpr = result;
    }

    return leftExpr;
}

Term* atom(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();
    Term* result = NULL;

    // function call?
    if (tokens.nextIs(IDENTIFIER) && tokens.nextIs(LPAREN, 1))
        result = function_call(branch, tokens);

    // literal integer?
    else if (tokens.nextIs(INTEGER))
        result = literal_integer(branch, tokens);

    // literal string?
    else if (tokens.nextIs(STRING))
        result = literal_string(branch, tokens);

    // literal hex?
    else if (tokens.nextIs(HEX_INTEGER))
        result = literal_hex(branch, tokens);

    // literal float?
    else if (tokens.nextIs(FLOAT))
        result = literal_float(branch, tokens);

    // literal list?
    else if (tokens.nextIs(LBRACKET))
        result = literal_list(branch, tokens);

    // literal branch?
    // disabled
    //else if (tokens.nextIs(LBRACE))

    // identifier?
    else if (tokens.nextIs(IDENTIFIER) || tokens.nextIs(AMPERSAND))
        result = identifier(branch, tokens);

    // parenthesized expression?
    else if (tokens.nextIs(LPAREN)) {
        tokens.consume(LPAREN);
        result = infix_expression(branch, tokens);

        if (!tokens.nextIs(RPAREN))
            return compile_error_for_line(result, tokens, startPosition);
        tokens.consume(RPAREN);
        result->intProp("syntaxHints:parens") += 1;
    }
    else {

        return compile_error_for_line(branch, tokens, startPosition);
    }

    return result;
}

Term* function_call(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    std::string functionName = tokens.consume(IDENTIFIER);
    tokens.consume(LPAREN);

    RefList inputs;

    ListSyntaxHints listHints;
    consume_list_arguments(branch, tokens, inputs, listHints);

    if (!tokens.nextIs(RPAREN))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(RPAREN);
    
    Term* result = find_and_apply(branch, functionName, inputs);

    result->stringProp("syntaxHints:declarationStyle") = "function-call";
    result->stringProp("syntaxHints:functionName") = functionName;

    listHints.apply(result);

    return result;
}

Term* literal_integer(Branch& branch, TokenStream& tokens)
{
    assert(tokens.nextIs(INTEGER));
    Token tok = tokens.consumet();
    int value = strtoul(tok.text.c_str(), NULL, 0);
    Term* term = int_value(&branch, value);
    term->stringProp("syntaxHints:declarationStyle") = "literal";
    term->stringProp("syntaxHints:integerFormat") = "dec";
    return term;
}

Term* literal_hex(Branch& branch, TokenStream& tokens)
{
    assert(tokens.nextIs(HEX_INTEGER));
    Token tok = tokens.consumet();
    int value = strtoul(tok.text.c_str(), NULL, 0);
    Term* term = int_value(&branch, value);
    term->stringProp("syntaxHints:declarationStyle") = "literal";
    term->stringProp("syntaxHints:integerFormat") = "hex";
    include_location(term, tok);
    return term;
}

Term* literal_float(Branch& branch, TokenStream& tokens)
{
    assert(tokens.nextIs(FLOAT));
    Token tok = tokens.consumet();
    std::string text = tok.text;

    // be lazy and parse the actual number with atof
    float value = atof(text.c_str());

    // figure out how many decimal places this number has
    bool foundDecimal = false;
    int decimalFigures = 0;

    for (unsigned int i=0; i < text.length(); i++) {
        if (text[i] == '.')
            foundDecimal = true;
        else if (foundDecimal)
            decimalFigures++;
    }

    Term* term = float_value(&branch, value);
    term->floatProp("syntaxHints:decimalFigures") = decimalFigures;

    float mutability = 0.0;

    if (tokens.nextIs(QUESTION)) {
        tokens.consume();
        mutability = 1.0;
    }

    term->addProperty("mutability", FLOAT_TYPE)->asFloat() = mutability;
    term->stringProp("syntaxHints:declarationStyle") = "literal";
    include_location(term, tok);
    return term;
}

Term* literal_string(Branch& branch, TokenStream& tokens)
{
    assert(tokens.nextIs(STRING));

    Token tok = tokens.consumet();

    std::string text = tok.text;

    // strip quote marks
    text = text.substr(1, text.length()-2);

    Term* term = string_value(&branch, text);
    term->stringProp("syntaxHints:declarationStyle") = "literal";
    include_location(term, tok);
    return term;
}

Term* literal_list(Branch& branch, TokenStream& tokens)
{
    int startPosition = tokens.getPosition();

    tokens.consume(LBRACKET);

    RefList terms;
    ListSyntaxHints listHints;
    consume_list_arguments(branch, tokens, terms, listHints);

    if (!tokens.nextIs(RBRACKET))
        return compile_error_for_line(branch, tokens, startPosition);

    tokens.consume(RBRACKET);

    Term* result = apply(&branch, LIST_FUNC, terms);
    listHints.apply(result);
    return result;
}

Term* identifier(Branch& branch, TokenStream& tokens)
{
    bool rebind = false;
    if (tokens.nextIs(AMPERSAND)) {
        tokens.consume();
        rebind = true;
    }

    assert(tokens.nextIs(IDENTIFIER));

    Token id = tokens.consumet();

    if (rebind)
        push_pending_rebind(branch, id.text);

    Term* result = find_named(&branch, id.text);

    // If not found, create an instance of unknown_identifier
    if (result == NULL) {
        result = apply(&branch, UNKNOWN_IDENTIFIER_FUNC, RefList());
        source_set_hidden(result, true);
        result->stringProp("message") = id.text;
        branch.bindName(result, id.text);
    }

    assert(result != NULL);
    include_location(result, id);

    return result;
}

std::string possible_whitespace(TokenStream& tokens)
{
    if (tokens.nextIs(WHITESPACE))
        return tokens.consume(WHITESPACE);
    else
        return "";
}

std::string possible_newline(TokenStream& tokens)
{
    if (tokens.nextIs(NEWLINE))
        return tokens.consume(NEWLINE);
    else
        return "";
}

std::string possible_whitespace_or_newline(TokenStream& tokens)
{
    std::stringstream output;

    while (tokens.nextIs(NEWLINE) || tokens.nextIs(WHITESPACE))
        output << tokens.consume();

    return output.str();
}

void consume_branch_until_end(Branch& branch, TokenStream& tokens)
{
    while (!tokens.finished()) {
        std::string prespace = possible_whitespace(tokens);

        if (tokens.nextIs(END) || tokens.nextIs(ELSE)) {
            break;
        } else {
            Term* term = statement(branch, tokens);
            prepend_whitespace(term, prespace);
        }
    }

    wrap_up_branch(branch);
}

} // namespace parser
} // namespace circa
