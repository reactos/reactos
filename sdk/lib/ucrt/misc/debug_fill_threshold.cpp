//
// debug_fill_threshold.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions to control the debug fill threshold used by the secure string
// functions.  A threshold of 0 disables filling; a threshold of SIZE_MAX
// means to always fill to the maximum.
//
#include <corecrt_internal.h>
#include <stdint.h>



static size_t __acrt_debug_fill_threshold = SIZE_MAX;



extern "C" size_t __cdecl _CrtSetDebugFillThreshold(size_t const new_threshold)
{
    size_t const old_threshold{__acrt_debug_fill_threshold};
    __acrt_debug_fill_threshold = new_threshold;
    return old_threshold;
}

extern "C" size_t __cdecl _CrtGetDebugFillThreshold()
{
    return __acrt_debug_fill_threshold;
}
