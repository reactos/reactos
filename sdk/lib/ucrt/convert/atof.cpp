//
// atof.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definitions of the functions that convert strings to floating point numbers.
// Note that the str- and wcs-prefixed functions are defined elsewhere.  The
// functions defined here are provided for legacy support.
//
// The _atoxxx family of functions convert a string into a floating point
// number and return either zero (success), or _UNDERFLOW or _OVERFLOW.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal_strtox.h>



template <typename FloatingType, typename Character>
static int __cdecl common_atodbl_l(
    FloatingType*    const result,
    Character const* const string,
    _locale_t        const locale
    ) throw()
{
    _VALIDATE_RETURN(result != nullptr, EINVAL, _DOMAIN);

    _LocaleUpdate locale_update(locale);
    SLD_STATUS const status = __crt_strtox::parse_floating_point(
        locale_update.GetLocaleT(),
        __crt_strtox::make_c_string_character_source(string, nullptr),
        result);

    switch (status)
    {
    case SLD_OVERFLOW:  return _OVERFLOW;
    case SLD_UNDERFLOW: return _UNDERFLOW;
    default:            return 0;
    }
}

extern "C" int __cdecl _atoflt_l(_CRT_FLOAT* const result, char const* const string, _locale_t const locale)
{
    return common_atodbl_l(&result->f, string, locale);
}

extern "C" int __cdecl _atoflt(_CRT_FLOAT* const result, char const* const string)
{
    return common_atodbl_l(&result->f, string, nullptr);
}

extern "C" int __cdecl _atodbl_l(_CRT_DOUBLE* const result, char* const string, _locale_t const locale)
{
    return common_atodbl_l(&result->x, string, locale);
}

extern "C" int __cdecl _atodbl(_CRT_DOUBLE* const result, char* const string)
{
    return common_atodbl_l(&result->x, string, nullptr);
}



template <typename Character>
static double __cdecl common_atof_l(Character const* const string, _locale_t const locale) throw()
{
    _VALIDATE_RETURN(string != nullptr, EINVAL, 0.0);

    _LocaleUpdate locale_update(locale);

    double result{};
    __crt_strtox::parse_floating_point(
        locale_update.GetLocaleT(),
        __crt_strtox::make_c_string_character_source(string, nullptr),
        &result);
    return result;
}

extern "C" double __cdecl _atof_l(char const* const string, _locale_t const locale)
{
    return common_atof_l(string, locale);
}

extern "C" double __cdecl atof(char const* const string)
{
    return common_atof_l(string, nullptr);
}

extern "C" double __cdecl _wtof_l(wchar_t const* const string, _locale_t const locale)
{
    return common_atof_l(string, locale);
}

extern "C" double __cdecl _wtof(wchar_t const* const string)
{
    return common_atof_l(string, nullptr);
}
