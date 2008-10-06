// Copyright 2008 Andrew Fischer

#ifndef CIRCA_COMPILATION_INCLUDED
#define CIRCA_COMPILATION_INCLUDED

#include "common_headers.h"

namespace circa {

bool hasCompileErrors(Branch& branch);
std::vector<std::string> getCompileErrors(Branch& branch);
void printCompileErrors(Branch& branch, std::ostream& output);

} // namespace circa

#endif
