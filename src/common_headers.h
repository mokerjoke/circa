// Copyright (c) 2007-2010 Paul Hodge. All rights reserved.

#pragma once

#ifdef WINDOWS

#undef max
#undef min

#define _USE_MATH_DEFINES
#include <math.h>
#include <direct.h> 

#endif // WINDOWS

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>

namespace circa {

struct Branch;
struct EvalContext;
struct FeedbackOperation;
struct Function;
struct FunctionAttrs;
struct List;
struct PathExpression;
struct RawOutputPrefs;
struct Ref;
struct RefList;
struct ReferenceMap;
struct StaticTypeQuery;
struct StyledSource;
struct TaggedValue;
struct Term;
struct Type;
struct TypeRef;

typedef bool (*TermVisitor)(Term* term, TaggedValue* context);

// Function-related typedefs:
typedef void (*EvaluateFunc)(EvalContext* context, Term* caller);
#if 0
typedef void (*EvaluateFunc2)(EvalContext* cxt, Term* term, Function* f,
        RefList const& inputs, TaggedValue* output);
#endif
typedef Term* (*SpecializeTypeFunc)(Term* caller);
typedef void (*FormatSource)(StyledSource* source, Term* term);
typedef bool (*CheckInvariants)(Term* term, std::string* output);

} // namespace circa
