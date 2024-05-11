/***
*_strerr.c - routine for indexing into system error list
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*   Returns system error message index by errno; conforms to the
*   XENIX standard, much compatibility with 1983 uniforum draft standard.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>



/***
*char *_strerror(message) - get system error message
*
*Purpose:
*   builds an error message consisting of the users error message
*   (the message parameter), followed by ": ", followed by the system
*   error message (index through errno), followed by a newline.  If
*   message is nullptr or a null string, returns a pointer to just
*   the system error message.
*
*Entry:
*   char *message - user's message to prefix system error message
*
*Exit:
*   returns pointer to static memory containing error message.
*   returns nullptr if malloc() fails in multi-thread versions.
*
*Exceptions:
*
*******************************************************************************/

static char*& __cdecl get_strerror_buffer(__acrt_ptd* const ptd, char) throw()
{
    return ptd->_strerror_buffer;
}

static wchar_t*& __cdecl get_strerror_buffer(__acrt_ptd* const ptd, wchar_t) throw()
{
    return ptd->_wcserror_buffer;
}

static errno_t __cdecl append_message(
    _Inout_updates_z_(buffer_count) char* const buffer,
    size_t                                const buffer_count,
    char const*                           const message
    ) throw()
{
    return strncat_s(buffer, buffer_count, message, buffer_count - strlen(buffer) - 2);
}

static errno_t __cdecl append_message(
    _Inout_updates_z_(buffer_count) wchar_t* const buffer,
    size_t                                   const buffer_count,
    char const*                              const message
    ) throw()
{
    size_t const buffer_length = wcslen(buffer);
    return mbstowcs_s(
        nullptr,
        buffer + buffer_length,
        buffer_count - buffer_length,
        message,
        buffer_count - buffer_length - 2);
}

template <typename Character>
_Ret_z_
_Success_(return != 0)
static Character* __cdecl common_strerror(
    Character const* const message
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    errno_t const original_errno_value = errno;

    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (!ptd)
        return nullptr;

    Character*& buffer = get_strerror_buffer(ptd, Character());
    if (!buffer)
        buffer = _calloc_crt_t(Character, strerror_buffer_count).detach();

    if (!buffer)
        return nullptr;

    buffer[0] = '\0';

    // CRT_REFACTOR TODO AppCRT/DesktopCRT Dependencies
    // (We should be using mbs* functions here).
    if (message && message[0] != '\0')
    {
        Character const colon[] = { ':', ' ', '\0' };

        // Leave room for the ": \n\0"
        _ERRCHECK(traits::tcsncat_s(buffer, strerror_buffer_count, message, strerror_buffer_count - 4));
        _ERRCHECK(traits::tcscat_s(buffer, strerror_buffer_count, colon));
    }

    char const* const system_message = _get_sys_err_msg(original_errno_value);
    _ERRCHECK(append_message(buffer, strerror_buffer_count, system_message));

    Character const newline[] = { '\n', '\0' };
    _ERRCHECK(traits::tcscat_s(buffer, strerror_buffer_count, newline));

    return buffer;
}

extern "C" char* __cdecl _strerror(char const* const message)
{
    return common_strerror(message);
}

extern "C" wchar_t* __cdecl __wcserror(wchar_t const* const message)
{
    return common_strerror(message);
}



/***
*errno_t _strerror_s(buffer, sizeInTChars, message) - get system error message
*
*Purpose:
*   builds an error message consisting of the users error message
*   (the message parameter), followed by ": ", followed by the system
*   error message (index through errno), followed by a newline.  If
*   message is nullptr or a null string, returns a pointer to just
*   the system error message.
*
*Entry:
*   TCHAR * buffer - Destination buffer.
*   size_t sizeInTChars - Size of the destination buffer.
*   TCHAR * message - user's message to prefix system error message
*
*Exit:
*   The error code.
*
*Exceptions:
*   Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

size_t const minimum_message_length = 5;

template <typename Character>
static errno_t __cdecl common_strerror_s(
    _Out_writes_z_(buffer_count)    Character*          const   buffer,
                                    size_t              const   buffer_count,
                                    Character const*    const   message
    ) throw()
{
    using traits = __crt_char_traits<Character>;

    errno_t const original_errno_value = errno;

    _VALIDATE_RETURN_ERRCODE(buffer != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(buffer_count > 0,  EINVAL);
    buffer[0] = '\0';

    if (message &&
        message[0] != '\0' &&
        traits::tcslen(message) < (buffer_count - 2 - minimum_message_length))
    {
        Character const colon[] = { ':', ' ', '\0' };
        _ERRCHECK(traits::tcscpy_s(buffer, buffer_count, message));
        _ERRCHECK(traits::tcscat_s(buffer, buffer_count, colon));
    }

    // Append the error message at the end of the buffer:
    return traits::tcserror_s(
        buffer       + traits::tcslen(buffer),
        buffer_count - traits::tcslen(buffer),
        original_errno_value);
}

extern "C" errno_t __cdecl _strerror_s(
    char*       const buffer,
    size_t      const buffer_count,
    char const* const message
    )
{
    return common_strerror_s(buffer, buffer_count, message);
}

extern "C" errno_t __cdecl __wcserror_s(
    wchar_t*       const buffer,
    size_t         const buffer_count,
    wchar_t const* const message
    )
{
    return common_strerror_s(buffer, buffer_count, message);
}
