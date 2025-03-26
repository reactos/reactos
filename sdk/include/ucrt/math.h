//
// math.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The C Standard Library <math.h> header.  This header consists of two parts:
// <corecrt_math.h> contains the math library; <corecrt_math_defines.h> contains
// the nonstandard but useful constant definitions.  The headers are divided in
// this way for modularity (to support the C++ modules feature).
//
#include <corecrt_math.h>

#ifdef _USE_MATH_DEFINES
    #include <corecrt_math_defines.h>
#endif
