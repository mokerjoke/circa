// Copyright 2008 Paul Hodge

namespace write_text_file_function {

    void evaluate(Term* caller)
    {
        std::string filename = as_string(caller->inputs[0]);
        std::string contents = as_string(caller->inputs[1]);
        std::ofstream file;
        file.open(filename.c_str(), std::ios::out);
        file << contents;
        file.close();
    }

    void setup(Branch& kernel)
    {
        Term* main_func = import_c_function(kernel, evaluate,
                "function write-text-file(string, string)");
        as_function(main_func).pureFunction = false;
    }
}
