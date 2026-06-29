//
// wcstok.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcstok(), which tokenizes a string via repeated calls.  See strtok()
// for more details.
//
#include <corecrt_internal.h>
#include <string.h>



_Check_return_
extern "C" wchar_t* __cdecl __acrt_wcstok_s_novalidation(
    _Inout_opt_z_                 wchar_t*       string,
    _In_z_                        wchar_t const* control,
    _Inout_ _Deref_prepost_opt_z_ wchar_t**      context
    );



extern "C" wchar_t* __cdecl wcstok(
    wchar_t*       const string,
    wchar_t const* const control,
    wchar_t**      const context
    )
{
    wchar_t** const context_to_use = context != nullptr
        ? context
        : &__acrt_getptd()->_wcstok_token;

    return __acrt_wcstok_s_novalidation(string, control, context_to_use);
}
