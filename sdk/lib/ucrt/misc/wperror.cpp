/***
*wperror.c - print system error message (wchar_t version)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wperror() - print wide system error message
*       System error message are indexed by errno.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <corecrt_internal_lowio.h>
#include <io.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***
*void _wperror(wmessage) - print system error message
*
*Purpose:
*       prints user's error message, then follows it with ": ", then the system
*       error message, then a newline.  All output goes to stderr.  If user's
*       message is nullptr or a null string, only the system error message is
*       printer.  If errno is weird, prints "Unknown error".
*
*Entry:
*       const wchar_t *wmessage - users message to prefix system error message
*
*Exit:
*       Prints message; no return value.
*
*Exceptions:
*
*******************************************************************************/

extern "C" void __cdecl _wperror(wchar_t const* const wide_user_prefix)
{
    // If the user did not provide a prefix, we can just call perror:
    if (wide_user_prefix == nullptr || wide_user_prefix[0] == L'\0')
        return perror(nullptr);

    // Otherwise, we need to convert the prefix into a narrow string then pass
    // it to perror:
    size_t required_count = 0;
    _ERRCHECK_EINVAL_ERANGE(wcstombs_s(&required_count, nullptr, 0, wide_user_prefix, INT_MAX));
    if (required_count == 0)
        return;

    __crt_unique_heap_ptr<char> const narrow_user_prefix(_calloc_crt_t(char, required_count));
    if (narrow_user_prefix.get() == nullptr)
        return;

    errno_t const conversion_result = _ERRCHECK_EINVAL_ERANGE(wcstombs_s(
        nullptr,
        narrow_user_prefix.get(),
        required_count,
        wide_user_prefix,
        _TRUNCATE));

    if (conversion_result != 0)
        return;

    return perror(narrow_user_prefix.get());
}
