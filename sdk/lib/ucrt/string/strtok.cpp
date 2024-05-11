//
// strtok.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines strtok(), which tokenizes a string via repeated calls.
//
// strtok() considers the string to consist of a sequence of zero or more text
// tokens separated by spans of one or more control characters.  The first call,
// with a string specified, returns a pointer to the first character of the
// first token, and will write a null character into the string immediately
// following the returned token.  Subsequent calls with a null string argument
// will work through the string until no tokens remain.  The control string
// may be different from call to call.  When no tokens remain in the string, a
// null pointer is returned.  
//
#include <corecrt_internal.h>
#include <string.h>



extern "C" char* __cdecl __acrt_strtok_s_novalidation(
    _Inout_opt_z_                 char*       string,
    _In_z_                        char const* control,
    _Inout_ _Deref_prepost_opt_z_ char**      context
    );

    

extern "C" char* __cdecl strtok(char* const string, char const* const control)
{
    return __acrt_strtok_s_novalidation(string, control, &__acrt_getptd()->_strtok_token);
}
