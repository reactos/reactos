//
// strtok_s.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines strtok_s(), which tokenizes a string via repeated calls.  See strtok()
// for more details.  This more secure function uses a caller-provided context
// instead of the thread-local tokenization state.
//
#include <string.h>
#include <corecrt_internal_securecrt.h>



// This common implementation is used by both strtok() and strtok_s()
extern "C" char* __cdecl __acrt_strtok_s_novalidation(
    _Inout_opt_z_                 char*       string,
    _In_z_                        char const* control,
    _Inout_ _Deref_prepost_opt_z_ char**      context
    )
{
    // Clear control map.  The control characters are stored in a bitmap, one
    // bit per character.  The null character is always a control character.
    unsigned char map[32];
    for (int count = 0; count < 32; count++)
    {
        map[count] = 0;
    }

    // Set bits in delimiter table
    unsigned char const* unsigned_control = reinterpret_cast<unsigned char const*>(control);
    do
    {
        map[*unsigned_control >> 3] |= (1 << (*unsigned_control & 7));
    }
    while (*unsigned_control++);

    // If string is null, set the iterator to the saved pointer (i.e., continue
    // breaking tokens out of the string from the last strtok call):
    char* it = string != nullptr
        ? string
        : *context;

    unsigned char*& unsigned_it = reinterpret_cast<unsigned char*&>(it);

    // Find beginning of token (skip over leading delimiters). Note that
    // there is no token iff this loop sets it to point to the terminal
    // null (*it == '\0')
    while ((map[*unsigned_it >> 3] & (1 << (*unsigned_it & 7))) && *it)
    {
        ++it;
    }

    char* const token_first = it;

    // Find the end of the token. If it is not the end of the string,
    // put a null there.
    for (; *it; ++it)
    {
        if (map[*unsigned_it >> 3] & (1 << (*unsigned_it & 7)))
        {
            *it++ = '\0';
            break;
        }
    }

    // Update the saved pointer:
    *context = it;

    // Determine if a token has been found.
    return it != token_first ? token_first : nullptr;
}



extern "C" char* __cdecl strtok_s(char* string, char const* control, char** context)
{
    _VALIDATE_POINTER_ERROR_RETURN(context, EINVAL, nullptr);
    _VALIDATE_POINTER_ERROR_RETURN(control, EINVAL, nullptr);
    _VALIDATE_CONDITION_ERROR_RETURN(string != nullptr || *context != nullptr, EINVAL, nullptr);

    return __acrt_strtok_s_novalidation(string, control, context);
}
