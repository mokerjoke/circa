#!/usr/bin/env python

import os, string, Utils

TEMPLATE = string.Template("""
// Copyright 2008 Andrew Fischer

namespace $namespace_name {

    void evaluate(Term* caller)
    {
$evaluate
    }

$feedback_propogate_defn
    void setup(Branch& kernel)
    {
        Term* main_func = import_c_function(kernel, evaluate,
                "$header");
        as_function(main_func).pureFunction = $is_pure;

$feedback_propogate_setup
    }
}
""")

FEEDBACK_PROPOGATE_DEFN_TEMPLATE = string.Template("""
    void feedback_propogate(Term* caller)
    {
$feedback_propogate
    }
""")

FEEDBACK_PROPOGATE_SETUP_TEMPLATE = string.Template("""
        Term* fp_func = import_c_function(kernel, feedback_propogate,
                "function ${name}-feedback-propogate(any,any)");
        as_function(fp_func).stateType = BRANCH_TYPE;
        as_function(main_func).feedbackPropogateFunction = fp_func;
""")

ALL_FUNCTIONS = []

def convert_to_cpp_identifier(ident):
    return ident.replace('-', '_')

def get_namespace_for_function(functionName):
    return convert_to_cpp_identifier(functionName) + "_function"

def generate_function(functionName):

    root_path = os.path.join('..','src','builtin_functions')
    py_source_path = os.path.join(root_path, functionName + '.source.py')
    output_path = os.path.join(root_path, 'generated', functionName + '.cpp')

    if not os.path.exists(py_source_path):
        raise Exception("couldn't find file: "+py_source_path)

    configs = {'name':functionName,
               'namespace_name':convert_to_cpp_identifier(functionName) + "_function",
               'source_file':py_source_path}

    execfile(py_source_path, configs)

    def normalize(s, indent):
        return Utils.normalize_indent(
                    Utils.strip_surrounding_blank_lines(
                        Utils.strip_extra_blank_lines(s)), indent)
    def normalize4(s):
        return normalize(s, '    ')
    def normalize8(s):
        return normalize(s, '        ')

    configs['is_pure'] = 'true' if configs['pure'] else 'false'
    configs['evaluate'] = normalize8(configs['evaluate'])

    if 'feedback_propogate' in configs:
        configs['feedback_propogate'] = Utils.strip_surrounding_blank_lines(
                configs['feedback_propogate'])
        configs['feedback_propogate_defn'] = (
            normalize4(FEEDBACK_PROPOGATE_DEFN_TEMPLATE.substitute(configs)))
        configs['feedback_propogate_setup'] = (
            normalize8(FEEDBACK_PROPOGATE_SETUP_TEMPLATE.substitute(configs)))
    else:
        configs['feedback_propogate_defn'] = ""
        configs['feedback_propogate_setup'] = ""

    result = TEMPLATE.substitute(configs)

    output = open(output_path, 'w')
    output.write(result)
    output.close()

    ALL_FUNCTIONS.append(functionName)

def write_registry():
    out = open('../src/builtin_functions/generated/setup_all.cpp', 'w')
    out.write('// Generated file\n')
    out.write('\n')
    for function in ALL_FUNCTIONS:
        out.write('#include "%s.cpp"\n' % function)
    out.write('\n')
    out.write('void setup_generated_functions(Branch& kernel)\n')
    out.write('{\n')
    for function in ALL_FUNCTIONS:
        out.write('    '+get_namespace_for_function(function)+'::setup(kernel);\n')
    out.write('}\n')
    out.close()

generate_function("add")
generate_function("and")
generate_function("concat")
generate_function("if-expr")
generate_function("list-append")
generate_function("list-apply")
generate_function("mult")
generate_function("or")
generate_function("print")
generate_function("range")
generate_function("read-text-file")
generate_function("to-string")
generate_function("tokenize")
generate_function("write-text-file")

write_registry()
