//
// lsearch_s.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _lsearch_s(), which performs a linear search over an array, appending
// the key to the end of the array if it is not found.
//
#ifdef __USE_CONTEXT
    #error __USE_CONTEXT should be undefined
#endif

#define __USE_CONTEXT
#include "lsearch.cpp"
