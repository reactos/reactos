/***
*a_str.c - A version of GetStringType.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Use either GetStringTypeA or GetStringTypeW depending on which is
*       unstubbed.
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <locale.h>

/***
*int __cdecl __acrt_GetStringTypeA - Get type information about an ANSI string.
*
*Purpose:
*       Internal support function. Assumes info in ANSI string format. Tries
*       to use NLS API call GetStringTypeA if available and uses GetStringTypeW
*       if it must. If neither are available it fails and returns FALSE.
*
*Entry:
*       DWORD    dwInfoType  - see NT\Chicago docs
*       PCCH     lpSrcStr    - char (byte) string for which character types
*                              are requested
*       int      cchSrc      - char (byte) count of lpSrcStr (including nullptr
*                              if any)
*       LPWORD   lpCharType  - word array to receive character type information
*                              (must be twice the size of lpSrcStr)
*       int      code_page   - for MB/WC conversion. If 0, use __lc_codepage
*       BOOL     bError      - TRUE if MB_ERR_INVALID_CHARS set on call to
*                              MultiByteToWideChar when GetStringTypeW used.
*
*Exit:
*       Success: TRUE
*       Failure: FALSE
*
*Exceptions:
*
*******************************************************************************/

extern "C" BOOL __cdecl __acrt_GetStringTypeA(
    _locale_t       const locale,
    DWORD           const info_type,
    LPCSTR          const string,
    int             const string_size_in_bytes,
    unsigned short* const char_type,
    int             const code_page,
    BOOL            const error
    )
{
    _LocaleUpdate locale_update(locale);

    // Convert string and return the requested information. Note that
    // we are converting to a wide character string so there is not a
    // one-to-one correspondence between number of multibyte chars in the
    // input string and the number of wide chars in the buffer. However,
    // there had *better be* a one-to-one correspondence between the
    // number of multibyte characters and the number of WORDs in the
    // return buffer.

    // Use __lc_codepage for conversion if code_page not specified:
    int const actual_code_page = code_page != 0
        ? code_page
        : locale_update.GetLocaleT()->locinfo->_public._locale_lc_codepage;

    // Find out how big a buffer we need:
    int const required_extent = __acrt_MultiByteToWideChar(
        actual_code_page,
        error ? (MB_PRECOMPOSED | MB_ERR_INVALID_CHARS) : MB_PRECOMPOSED,
        string,
        string_size_in_bytes,
        nullptr,
        0);

    if (required_extent == 0)
        return FALSE;

    // Allocate enough space for the wide character string:
    __crt_scoped_stack_ptr<wchar_t> buffer(_malloca_crt_t(wchar_t, required_extent));
    if (buffer.get() == nullptr)
        return FALSE;

    memset(buffer.get(), 0, sizeof(wchar_t) * required_extent);

    // Do the conversion:
    int const actual_extent = __acrt_MultiByteToWideChar(
        actual_code_page,
        MB_PRECOMPOSED,
        string,
        string_size_in_bytes,
        buffer.get(),
        required_extent);

    if (actual_extent == 0)
        return FALSE;

    return GetStringTypeW(info_type, buffer.get(), actual_extent, char_type);
}
