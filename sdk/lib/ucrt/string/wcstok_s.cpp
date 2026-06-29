//
// wcstok_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines wcstok_s(), which tokenizes a string via repeated calls.  See strtok()
// for more details.  This more secure function uses a caller-provided context
// instead of the thread-local tokenization state.
//
#include <corecrt_internal_securecrt.h>
#include <string.h>



// This common implementation is used by both strtok() and strtok_s()
extern "C" wchar_t* __cdecl __acrt_wcstok_s_novalidation(
    _Inout_opt_z_                 wchar_t*       string,
    _In_z_                        wchar_t const* control,
    _Inout_ _Deref_prepost_opt_z_ wchar_t**      context
    )
{
    // If string is null, set the iterator to the saved pointer (i.e., continue
    // breaking tokens out of the string from the last strtok call):
    wchar_t* string_it = string != nullptr
        ? string
        : *context;


    // Find beginning of token (skip over leading delimiters). Note that
    // there is no token iff this loop sets string to point to the terminal
    // null (*string == '\0')
    while (*string_it)
    {
        wchar_t const* control_it = control;
        for (; *control_it && *control_it != *string_it; ++control_it)
        {
        }

        if (!*control_it)
            break;

        ++string_it;
    }

    wchar_t* const token_first = string_it;

    // Find the end of the token.  If it is not the end of the string, put a
    // null character there:
    for (; *string_it; ++string_it)
    {
        wchar_t const* control_it = control;
        for (; *control_it && *control_it != *string_it; ++control_it)
        {
        }

        if (*control_it)
        {
            *string_it++ = '\0';
            break;
        }
    }

    // Update the saved pointer:
    *context = string_it;

    // Determine if a token has been found.
    return string_it != token_first ? token_first : nullptr;
}



extern "C" wchar_t* __cdecl wcstok_s(
    wchar_t*       const string,
    wchar_t const* const control,
    wchar_t**      const context
    )
{
    _VALIDATE_POINTER_ERROR_RETURN(context, EINVAL, nullptr);
    _VALIDATE_POINTER_ERROR_RETURN(control, EINVAL, nullptr);
    _VALIDATE_CONDITION_ERROR_RETURN(string != nullptr || *context != nullptr, EINVAL, nullptr);

    return __acrt_wcstok_s_novalidation(string, control, context);
}
