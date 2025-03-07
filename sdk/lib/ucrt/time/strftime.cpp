//
// strftime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The strftime family of functions, which format time data into a string, and
// related functionality.
//
#include <corecrt_internal_time.h>
#include <locale.h>

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Day and Month Name and Time Locale Information Fetching Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" char* __cdecl _Getdays_l(_locale_t const locale)
{
    _LocaleUpdate locale_update(locale);
    __crt_lc_time_data const* const time_data = locale_update.GetLocaleT()->locinfo->lc_time_curr;

    size_t length = 0;
    for (size_t n = 0; n < 7; ++n)
    {
        length += strlen(time_data->wday_abbr[n]) + strlen(time_data->wday[n]) + 2;
    }

    __crt_unique_heap_ptr<char> buffer(_malloc_crt_t(char, length + 1));
    if (buffer.get() == nullptr)
        return nullptr;


    char* it = buffer.get();
    for (size_t n = 0; n < 7; ++n)
    {
        *it++ = ':';
        _ERRCHECK(strcpy_s(it, (length + 1) - (it - buffer.get()), time_data->wday_abbr[n]));
        it += strlen(it);
        *it++ = ':';
        _ERRCHECK(strcpy_s(it, (length + 1) - (it - buffer.get()), time_data->wday[n]));
        it += strlen(it);
    }
    *it++ = '\0';

    return buffer.detach();
}

extern "C" char* __cdecl _Getdays()
{
    return _Getdays_l(nullptr);
}



extern "C" char* __cdecl _Getmonths_l(_locale_t const locale)
{
    _LocaleUpdate locale_update(locale);
    __crt_lc_time_data const* time_data = locale_update.GetLocaleT()->locinfo->lc_time_curr;

    size_t length = 0;
    for (size_t n = 0; n < 12; ++n)
    {
        length += strlen(time_data->month_abbr[n]) + strlen(time_data->month[n]) + 2;
    }

    __crt_unique_heap_ptr<char> buffer(_malloc_crt_t(char, length + 1));
    if (buffer.get() == nullptr)
        return nullptr;

    char* it = buffer.get();
    for (size_t n = 0; n < 12; ++n)
    {
        *it++ = ':';
        _ERRCHECK(strcpy_s(it, (length + 1) - (it - buffer.get()), time_data->month_abbr[n]));
        it += strlen(it);
        *it++ = ':';
        _ERRCHECK(strcpy_s(it, (length + 1) - (it - buffer.get()), time_data->month[n]));
        it += strlen(it);
    }
    *it++ = '\0';

    return buffer.detach();
}



extern "C" char* __cdecl _Getmonths()
{
    return _Getmonths_l(nullptr);
}

extern "C" void* __cdecl _W_Gettnames();

extern "C" void* __cdecl _Gettnames()
{
    return _W_Gettnames();
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// The strftime family of functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions format a time as a string using a given locale.  They place
// characters into the user's output buffer, expanding time format directives as
// described in the provided control string.  The lc_time_arg and locale are
// used for locale data.
//
// If the total number of characters that need to be written (including the null
// terminator) is less than the max_size, then the number of characters written
// (not including the null terminator) is returned.  Otherwise, zero is returned.
//
// These functions simply delegate to the corresponding wide string functions
// (e.g. _wcsftime).
_Success_(return > 0)
extern "C" size_t __cdecl _Strftime_l (
    _Out_writes_z_(maxsize) char * const string,
    _In_ size_t                    const maxsize,
    _In_z_ const char *            const format,
    _In_ const tm *                const timeptr,
    _In_ void *                    const lc_time_arg,
    _In_opt_ _locale_t             const locale
    )
{
    _LocaleUpdate locale_update(locale);
    unsigned int const lc_time_cp = locale_update.GetLocaleT()->locinfo->lc_time_cp;

    _VALIDATE_RETURN(string != nullptr, EINVAL, 0)
    _VALIDATE_RETURN(maxsize != 0,      EINVAL, 0)
    *string = '\0';

    _VALIDATE_RETURN(format != nullptr,  EINVAL, 0)
    _VALIDATE_RETURN(timeptr != nullptr, EINVAL, 0)

    __crt_internal_win32_buffer<wchar_t> wformat;

    errno_t const cvt1 = __acrt_mbs_to_wcs_cp(format, wformat, lc_time_cp);

    if (cvt1 != 0) {
        return 0;
    }

    // Allocate a new wide-char output string with the same size as the char*
    // string one passed in as argument
    __crt_unique_heap_ptr<wchar_t> const wstring(_malloc_crt_t(wchar_t, maxsize));
    if (wstring.get() == nullptr)
    {
        // malloc should set the errno, if any
        return 0;
    }

    size_t const wcsftime_result = _Wcsftime_l(wstring.get(), maxsize, wformat.data(), timeptr, lc_time_arg, locale);
    if (wcsftime_result == 0)
    {
        return 0;
    }

    __crt_no_alloc_win32_buffer<char> copy_back(string, maxsize);
    errno_t const cvt2 = __acrt_wcs_to_mbs_cp(wstring.get(), copy_back, lc_time_cp);

    if (cvt2 != 0) {
        return 0;
    }

    return copy_back.size();
}

extern "C" size_t __cdecl _Strftime(
    char*       const string,
    size_t      const max_size,
    char const* const format,
    tm const*   const timeptr,
    void*       const lc_time_arg
    )
{
    return _Strftime_l(string, max_size, format, timeptr, lc_time_arg, nullptr);
}

extern "C" size_t __cdecl _strftime_l(
    char*       const string,
    size_t      const max_size,
    char const* const format,
    tm const*   const timeptr,
    _locale_t   const locale
    )
{
    return _Strftime_l(string, max_size, format, timeptr, nullptr, locale);
}
extern "C" size_t __cdecl strftime(
    char*       const string,
    size_t      const max_size,
    char const* const format,
    tm const*   const timeptr
    )
{
    return _Strftime_l(string, max_size, format, timeptr, nullptr, nullptr);
}
