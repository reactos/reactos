//
// strtod.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definitions of the functions that convert strings to floating point numbers.
// These functions, the strtox and wcstox functions for floating point types,
// recogize an optional sequence of tabs and space, then an optional sign, then
// a sequence of digits optionally containing a decimal point, then an optional
// e or E followed by an optionally signed integer, and converts all of this to
// a floating point number.  The first unrecognized character ends the string.
// The *end_ptr pointer is updated to point to the last character of the string
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal_strtox.h>
#include <stdlib.h>



template <typename FloatingType, typename Character>
static FloatingType __cdecl common_strtod_l(
    _In_z_                      Character const*    const   string,
    _Out_opt_ _Deref_post_z_    Character**         const   end_ptr,
                                _locale_t           const   locale
    ) throw()
{
    if (end_ptr)
    {
        *end_ptr = const_cast<Character*>(string);
    }

    _VALIDATE_RETURN(string != nullptr, EINVAL, 0.0);

    _LocaleUpdate locale_update(locale);

    FloatingType result{};
    SLD_STATUS const status = __crt_strtox::parse_floating_point(
        locale_update.GetLocaleT(),
        __crt_strtox::make_c_string_character_source(string, end_ptr),
        &result);

    if (status == SLD_OVERFLOW || status == SLD_UNDERFLOW)
    {
        errno = ERANGE;
    }

    return result;
}

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Narrow Strings => Floating Point
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" float __cdecl strtof(
    char const* const string,
    char**      const end_ptr
    )
{
    return common_strtod_l<float>(string, end_ptr, nullptr);
}

extern "C" float __cdecl _strtof_l(
    char const* const string,
    char**      const end_ptr,
    _locale_t   const locale
    )
{
    return common_strtod_l<float>(string, end_ptr, locale);
}

extern "C" double __cdecl strtod(
    char const* const string,
    char**      const end_ptr
    )
{
    return common_strtod_l<double>(string, end_ptr, nullptr);
}

extern "C" double __cdecl _strtod_l(
    char const* const string,
    char**      const end_ptr,
    _locale_t   const locale
    )
{
    return common_strtod_l<double>(string, end_ptr, locale);
}

extern "C" long double __cdecl strtold(
    char const* const string,
    char**      const end_ptr
    )
{
    return common_strtod_l<double>(string, end_ptr, nullptr);
}


extern "C" long double __cdecl _strtold_l(
    char const* const string,
    char**      const end_ptr,
    _locale_t   const locale
    )
{
    return common_strtod_l<double>(string, end_ptr, locale);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Wide Strings => Floating Point
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" float __cdecl wcstof(
    wchar_t const* const string,
    wchar_t**      const end_ptr
    )
{
    return common_strtod_l<float>(string, end_ptr, nullptr);
}

extern "C" float __cdecl _wcstof_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    _locale_t      const locale
    )
{
    return common_strtod_l<float>(string, end_ptr, locale);
}

extern "C" double __cdecl wcstod(
    wchar_t const* const string,
    wchar_t**      const end_ptr
    )
{
    return common_strtod_l<double>(string, end_ptr, nullptr);
}

extern "C" double __cdecl _wcstod_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    _locale_t      const locale
    )
{
    return common_strtod_l<double>(string, end_ptr, locale);
}

extern "C" long double __cdecl wcstold(
    wchar_t const* const string,
    wchar_t**      const end_ptr
    )
{
    return common_strtod_l<double>(string, end_ptr, nullptr);
}


extern "C" long double __cdecl _wcstold_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    _locale_t      const locale
    )
{
    return common_strtod_l<double>(string, end_ptr, locale);
}
