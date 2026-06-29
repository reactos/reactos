//
// mktemp.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines _tmktemp() and _tmktemp_s(), which create unique file names.
//
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_securecrt.h>
#include <mbctype.h>
#include <process.h>
#include <stddef.h>
#include <stdio.h>



// These functions test whether we should continue scanning for X's.
static bool common_mktemp_s_continue(char const* const string, char const* const current) throw()
{
    return !_ismbstrail(
        reinterpret_cast<unsigned char const*>(string),
        reinterpret_cast<unsigned char const*>(current));
}

static bool common_mktemp_s_continue(wchar_t const* const, wchar_t const* const) throw()
{
    return true;
}



// Creates a unique file name given a template string of the form "fnamXXXXXX".
// The "XXXXXX" sequence is replaced by a letter and the five-digit thread
// identifier.
//
// The template string is modified in-place.  Returns 0 on success; returns an
// errno error code on failure.
template <typename Character>
_Success_(return == 0)
static errno_t __cdecl common_mktemp_s(
    _Inout_updates_z_(buffer_size_in_chars) Character* const    template_string,
                                            size_t     const    buffer_size_in_chars
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN_ERRCODE(template_string != nullptr && buffer_size_in_chars > 0, EINVAL);

    size_t const template_string_length = traits::tcsnlen(template_string, buffer_size_in_chars);
    if (template_string_length >= buffer_size_in_chars)
    {
        _RESET_STRING(template_string, buffer_size_in_chars);
        _RETURN_DEST_NOT_NULL_TERMINATED(template_string, buffer_size_in_chars);
    }
    _FILL_STRING(template_string, buffer_size_in_chars, template_string_length + 1);

    if(template_string_length < 6 || buffer_size_in_chars <= template_string_length)
    {
        _RESET_STRING(template_string, buffer_size_in_chars);
        _VALIDATE_RETURN_ERRCODE(("Incorrect Input for mktemp", 0), EINVAL);
    }

    // The Win32 thread identifier is unique across all threads in all processes.
    // Note, however, that unlike *NIX process identifiers, which are not reused
    // until all values up to 32K have been used, Win32 thread identifiers are
    // frequenty reused and usually have small numbers.
    unsigned number = GetCurrentThreadId();

    // 'string' points to the null-terminator:
    Character* string = template_string + template_string_length;

    size_t template_length = 0;

    // Replace the last five 'X' characters with the number:
    while (--string >= template_string 
        && common_mktemp_s_continue(template_string, string)
        && *string == 'X'
        && template_length < 5)
    {
        ++template_length;
        *string = static_cast<Character>((number % 10) + '0');
        number /= 10;
    }

    // Too few X's?
    if (*string != 'X' || template_length < 5)
    {
        _RESET_STRING(template_string, buffer_size_in_chars);
        _VALIDATE_RETURN_ERRCODE(("Incorrect Input for mktemp", 0), EINVAL);
    }

    // Finally, add the letter:
    Character letter = 'a';

    *string = letter++;

    errno_t const saved_errno = errno;
    errno = 0;

    // Test each letter, from a-z, until we find a name that is not yet used:
    while (traits::taccess_s(template_string, 0) == 0)
    {
        if (letter == 'z' + 1)
        {
            _RESET_STRING(template_string, buffer_size_in_chars);
            errno = EEXIST;
            return errno;
        }

        *string = letter++;
        errno = 0;
    }

    // Restore the old value of errno and return success:
    errno = saved_errno;
    return 0;
}

extern "C" errno_t __cdecl _mktemp_s(
    char*  const template_string,
    size_t const buffer_size_in_chars
    )
{
    return common_mktemp_s(template_string, buffer_size_in_chars);
}

extern "C" errno_t __cdecl _wmktemp_s(
    wchar_t* const template_string,
    size_t   const buffer_size_in_chars
    )
{
    return common_mktemp_s(template_string, buffer_size_in_chars);
}



// Creates a unique file name given a template string of the form "fnamXXXXXX".
// The "XXXXXX" sequence is replaced by a letter and the five-digit thread
// identifier.  The template string is modified in-place.
//
// On success, returns the pointer to the modified template string.  On failure,
// nullptr is returned (e.g. if the template string is malformed or there are no
// more unique names).
template <typename Character>
static Character* __cdecl common_mktemp(
    Character* const template_string
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(template_string != nullptr, EINVAL, nullptr);

    errno_t const result = common_mktemp_s(
        template_string,
        static_cast<size_t>(traits::tcslen(template_string) + 1));

    return result == 0 ? template_string : nullptr;
}

extern "C" char* __cdecl _mktemp(char* const template_string)
{
    return common_mktemp(template_string);
}

extern "C" wchar_t* __cdecl _wmktemp(wchar_t* const template_string)
{
    return common_mktemp(template_string);
}
