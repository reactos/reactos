/***
*wcrtomb.cpp - Convert wide character to multibyte character, with locale.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a wide character into the equivalent multibyte character.
*
*******************************************************************************/
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <corecrt_internal_securecrt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

using namespace __crt_mbstring;

/***
*errno_t _wcrtomb_internal() - Helper function to convert wide character to multibyte character.
*
*Purpose:
*       Convert a wide character into the equivalent multi-byte character,
*       according to the specified LC_CTYPE category, or the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       int *return_value      = the number of chars written (-1 in error case)
*       char *destination           = pointer to multibyte character
*       size_t destination_count  = size of the destinarion buffer
*       wchar_t wchar       = source wide character
*       mbstate_t *state      = pointer to state (not used)
*       _locale_t locale  = locale info
*
*Exit:
*       Returns:
*       Value of errno if errors, 0 otherwise. *return_value is set to -1 in error case.
*
*Exceptions:
*
*******************************************************************************/

_Success_(return == 0)
static errno_t __cdecl _wcrtomb_internal(
                                            int*               const return_value,
    __out_bcount_z_opt(destination_count)   char*              const destination,
                                            size_t             const destination_count,
                                            wchar_t            const wchar,
                                            mbstate_t*         const state,
    _Inout_                                 __crt_cached_ptd_host&   ptd
    )
{
    _ASSERTE(destination != nullptr && destination_count > 0);

    _locale_t const locale = ptd.get_locale();

    _ASSERTE(
        locale->locinfo->_public._locale_mb_cur_max == 1 ||
        locale->locinfo->_public._locale_mb_cur_max == 2 ||
        locale->locinfo->_public._locale_lc_codepage == CP_UTF8);

    if (state)
    {
        state->_Wchar = 0;
    }

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        // Unlike c16rtomb. wctomb/wcrtomb have no ability to process a partial code point.
        // So, we could call c16rtomb and check for a lone surrogate or other error, or for simplicity
        // We can instead just call c32rtomb and check for any error. I choose the latter.
        static mbstate_t local_state{};
        int result = static_cast<int>(__crt_mbstring::__c32rtomb_utf8(destination, static_cast<char32_t>(wchar), (state != nullptr ? state : &local_state), ptd));
        if (return_value != nullptr)
        {
            *return_value = result;
        }
        if (result <= 4)
        {
            return 0;
        }
        else
        {
            return ptd.get_errno().value_or(0);
        }
    }

    if (!locale->locinfo->locale_name[LC_CTYPE])
    {
        if (wchar > 255) // Validate high byte
        {
            if (return_value)
                *return_value = -1;

            return ptd.get_errno().set(EILSEQ);
        }

        *destination = static_cast<char>(wchar);
        if (return_value)
        {
            *return_value = 1;
        }

        return 0;
    }

    BOOL default_used{};
    int const size = __acrt_WideCharToMultiByte(
        locale->locinfo->_public._locale_lc_codepage,
        0,
        &wchar,
        1,
        destination,
        static_cast<int>(destination_count),
        nullptr,
        &default_used);

    if (size == 0 || default_used)
    {
        if (return_value)
        {
            *return_value = -1;
        }

        return ptd.get_errno().set(EILSEQ);
    }

    if (return_value)
    {
        *return_value = size;
    }

    return 0;
}

/***
*errno_t wcrtomb_s(retValue, destination, destination_count, wchar, state) - translate wchar_t to multibyte, restartably
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

static errno_t __cdecl wcrtomb_s_internal(
    size_t*            const return_value,
    char*              const destination,
    size_t             const destination_count,
    wchar_t            const wchar,
    mbstate_t*         const state,
    __crt_cached_ptd_host&   ptd
    )
{
    // Note that we do not force destination_count > 0 in the destination !=
    // nullptr case because we do not need to add a null terminator, due to
    // the fact that the destination will receive a character and not a string.
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, (destination == nullptr && destination_count == 0) || (destination != nullptr), EINVAL);

    errno_t e = 0;
    int     int_return_value = -1;
    if (destination == nullptr)
    {
        char buf[MB_LEN_MAX];
        e = _wcrtomb_internal(&int_return_value, buf, MB_LEN_MAX, wchar, state, ptd);
    }
    else
    {
        e = _wcrtomb_internal(&int_return_value, destination, destination_count, wchar, state, ptd);
    }

    if (return_value != nullptr)
    {
        *return_value = static_cast<size_t>(int_return_value);
    }

    return e;
}

extern "C" errno_t __cdecl wcrtomb_s(
    size_t*    const return_value,
    char*      const destination,
    size_t     const destination_count,
    wchar_t    const wchar,
    mbstate_t* const state
    )
{
    __crt_cached_ptd_host ptd;
    return wcrtomb_s_internal(return_value, destination, destination_count, wchar, state, ptd);
}

extern "C" size_t __cdecl wcrtomb(
    char*      const destination,
    wchar_t    const wchar,
    mbstate_t* const state
    )
{
    size_t return_value = static_cast<size_t>(-1);
    wcrtomb_s(&return_value, destination, (destination == nullptr ? 0 : MB_LEN_MAX), wchar, state);
    return return_value;
}

/***
*errno_t wcsrtombs_s(retValue, destination, destination_count, pwcs, n, state) - translate wide char string to multibyte
*       string
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

/* Helper shared by secure and non-secure functions. */

static size_t __cdecl _wcsrtombs_internal(
    _Pre_maybenull_ _Post_z_    char*                   destination,
    _Inout_ _Deref_prepost_z_   wchar_t const** const   source,
    _In_                        size_t                  n,
    _Out_opt_                   mbstate_t*      const   state,
    _Inout_                     __crt_cached_ptd_host&  ptd
    ) throw()
{
    /* validation section */
    _UCRT_VALIDATE_RETURN(ptd, source != nullptr, EINVAL, (size_t)-1);

    _locale_t const locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        return __wcsrtombs_utf8(destination, source, n, state, ptd);
    }

    char buf[MB_LEN_MAX];
    int i = 0;
    size_t nc = 0;
    wchar_t const* wcs = *source;

    if (!destination)
    {
        for (; ; nc += i, ++wcs)
        {
            /* translate but don't store */
            _wcrtomb_internal(&i, buf, MB_LEN_MAX, *wcs, state, ptd);
            if (i <= 0)
            {
                return static_cast<size_t>(-1);
            }
            else if (buf[i - 1] == '\0')
            {
                return nc + i - 1;
            }
        }
    }

    for (; 0 < n; nc += i, ++wcs, destination += i, n -= i)
    {
        /* translate and store */
        char *t = nullptr;

        if (n < (size_t)locale->locinfo->_public._locale_mb_cur_max)
        {
            t = buf;
        }
        else
        {
            t = destination;
        }

        _wcrtomb_internal(&i, t, MB_LEN_MAX, *wcs, state, ptd);
        if (i <= 0)
        {
            /* encountered invalid sequence */
            nc = (size_t)-1;
            break;
        }

        if (destination == t)
        {
            /* do nothing */
        }
        else if (n < static_cast<size_t>(i))
        {
            break;  // Won't all fit
        }
        else
        {
            memcpy_s(destination, n, buf, i);
        }

        if (destination[i - 1] == '\0')
        {
            // Encountered terminating null
            *source = 0;
            return nc + i - 1;
        }
    }

    *source = wcs;
    return nc;
}

extern "C" size_t __cdecl wcsrtombs(
    char*           const destination,
    wchar_t const** const source,
    size_t          const n,
    mbstate_t*      const state
    )
{
    __crt_cached_ptd_host ptd;
    return _wcsrtombs_internal(destination, source, n, state, ptd);
}

/***
*errno_t wcstombs_s() - Convert wide char string to multibyte char string.
*
*Purpose:
*       Convert a wide char string into the equivalent multibyte char string,
*       according to the LC_CTYPE category of the current locale.
*
*       The destination string is always null terminated.
*
*Entry:
*       size_t *return_value = Number of bytes modified including the terminating nullptr
*                           This pointer can be nullptr.
*       char *destination = pointer to destination multibyte char string
*       size_t destination_count = size of the destination buffer
*       const wchar_t *source = pointer to source wide character string
*       size_t n = maximum number of bytes to store in s (not including the terminating nullptr)
*       mbstate_t *state = pointer to state
*
*Exit:
*       The error code.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" errno_t __cdecl wcsrtombs_s(
    size_t*         const return_value,
    char*           const destination,
    size_t          const destination_count,
    wchar_t const** const source,
    size_t          const n,
    mbstate_t*      const state
    )
{
    __crt_cached_ptd_host ptd;

    if (return_value != nullptr)
    {
        *return_value = static_cast<size_t>(-1);
    }

    _UCRT_VALIDATE_RETURN_ERRCODE(
        ptd,
        (destination == nullptr && destination_count == 0) ||
        (destination != nullptr && destination_count >  0),
    EINVAL);

    if (destination != nullptr)
    {
        _RESET_STRING(destination, destination_count);
    }

    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, source != nullptr, EINVAL);

    size_t retsize = _wcsrtombs_internal(destination, source, (n > destination_count ? destination_count : n), state, ptd);
    if (retsize == static_cast<size_t>(-1))
    {
        if (destination != nullptr)
        {
            _RESET_STRING(destination, destination_count);
        }

        return ptd.get_errno().value_or(0);
    }

    ++retsize; // Account for the null terminator

    if (destination != nullptr)
    {
        // Return error if the string does not fit:
        if (retsize > destination_count)
        {
            _RESET_STRING(destination, destination_count);
            _UCRT_VALIDATE_RETURN_ERRCODE(ptd, retsize <= destination_count, ERANGE);
        }

        // Ensure the string is null terminated:
        destination[retsize - 1] = '\0';
    }

    if (return_value != nullptr)
    {
        *return_value = retsize;
    }

    return 0;
}



// Converts a wide character into a one-byte character
extern "C" int __cdecl wctob(wint_t const wchar)
{
    __crt_cached_ptd_host ptd;

    if (wchar == WEOF)
    {
        return EOF;
    }

    int  return_value = -1;
    char local_buffer[MB_LEN_MAX];

    mbstate_t state{};
    errno_t const e = _wcrtomb_internal(&return_value, local_buffer, MB_LEN_MAX, wchar, &state, ptd);
    if (e == 0 && return_value == 1)
    {
        return local_buffer[0];
    }

    return EOF;
}

size_t __cdecl __crt_mbstring::__wcsrtombs_utf8(char* dst, const wchar_t** src, size_t len, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    const wchar_t* current_src = *src;
    char buf[MB_LEN_MAX];

    if (dst != nullptr)
    {
        char* current_dest = dst;

        // Wide chars are actually UTF-16, so a code point might take 2 input units (a surrogate pair)
        // In case of a failure, keep track of where the current code point began, which might be the previous
        // wchar for a surrogate pair
        const wchar_t* start_of_code_point = current_src;
        for (;;)
        {
            // If we don't have at least 4 MB_CUR_LEN bytes available in the buffer
            // the next char isn't guaranteed to fit, so put it into a temp buffer
            char* temp;
            if (len < 4)
            {
                temp = buf;
            }
            else
            {
                temp = current_dest;
            }
            const size_t retval = __c16rtomb_utf8(temp, *current_src, ps, ptd);

            if (retval == __crt_mbstring::INVALID)
            {
                // Set src to the beginning of the invalid char
                // If this was the second half of a surrogate pair, return the beginning of the surrogate pair
                *src = start_of_code_point;
                return retval;
            }

            if (temp == current_dest)
            {
                // We wrote in-place. Nothing to do.
            }
            else if (len < retval)
            {
                // Won't fit, so bail out
                // If this was the second half of a surrogate pair, make sure we return that location
                current_src = start_of_code_point;
                break;
            }
            else
            {
                // Will fit in remaining buffer, so let's copy it over
                memcpy(current_dest, temp, retval);
            }

            if (retval > 0 && current_dest[retval - 1] == '\0')
            {
                // Reached null terminator, so break out, but don't count that last terminating byte
                current_src = nullptr;
                current_dest += retval - 1;
                break;
            }

            ++current_src;
            if (retval > 0)
            {
                start_of_code_point = current_src;
            }

            len -= retval;
            current_dest += retval;
        }
        *src = current_src;
        return current_dest - dst;
    }
    else
    {
        size_t total_count = 0;
        for (;;)
        {
            const size_t retval = __c16rtomb_utf8(buf, *current_src, ps, ptd);
            if (retval == __crt_mbstring::INVALID)
            {
                return retval;
            }
            else if (retval > 0 && buf[retval - 1] == '\0')
            {
                // Hit null terminator. Don't count it in the return value.
                total_count += retval - 1;
                break;
            }
            total_count += retval;
            ++current_src;
        }
        return total_count;
    }
}
