// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#include <circa.h>

namespace circa {
namespace include_function {

    void preload_script(EvalContext* cxt, Term* term)
    {
        TaggedValue* fileSignature = term->input(0);
        Branch& contents = term->nestedContents;

        std::string filename = term->input(1)->asString();

        // Reload if the filename or modified-time has changed
        if (file_changed_function::check(cxt, term, fileSignature, filename))
        {
            Branch previous_contents;
            duplicate_branch(contents, previous_contents);

            contents.clear();

            if (!storage::file_exists(filename.c_str())) {
                error_occurred(cxt, term, "File not found: "+filename);
                return;
            }

            parse_script(contents, filename);

            if (has_static_errors(contents)) {
                error_occurred(cxt, term, get_static_errors_formatted(contents));

                // New script has errors. If we have an existing script, then revert
                // to that.
                if (previous_contents.length() != 0) {
                    contents.clear();
                    duplicate_branch(previous_contents, contents);
                }
                return;
            }

            //std::cout << "### Previous:" << std::endl;
            //dump_branch(previous_contents);
            //std::cout << "### New:" << std::endl;
            //dump_branch(contents);

            if (previous_contents.length() > 0)
                migrate_stateful_values(previous_contents, contents);

            if (term->owningBranch != NULL)
                expose_all_names(contents, *term->owningBranch);
        }
    }

    CA_FUNCTION(evaluate_include)
    {
        preload_script(CONTEXT, CALLER);

        if (CONTEXT->errorOccurred)
            return;

        Branch& contents = CALLER->nestedContents;
        evaluate_branch(CONTEXT, contents);
    }

    CA_FUNCTION(load_script)
    {
        preload_script(CONTEXT, CALLER);
    }

    void setup(Branch& kernel)
    {
        INCLUDE_FUNC = import_function(kernel, evaluate_include,
                "include(state FileSignature, string filename)");

        function_t::set_exposed_name_path(INCLUDE_FUNC, ".");

        Term* load_script_f = import_function(kernel, load_script,
            "load_script(state FileSignature, string filename)");

        function_t::set_exposed_name_path(load_script_f, ".");
    }
}
} // namespace circa