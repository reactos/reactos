//
// atox.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The "simple" string conversion functions:  atoi, atol, atoll, and their wide
// string functions.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal_strtox.h>
#include <stdlib.h>



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Narrow Strings => Various Integers (Simple Functions, wtox)
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" int __cdecl atoi(char const* const string)
{
    // Note:  We parse as a long to avoid an extra specialization of parse_integer_from_string
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, nullptr);
}

extern "C" int __cdecl _atoi_l(char const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, locale);
}

extern "C" long __cdecl atol(char const* const string)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, nullptr);
}

extern "C" long __cdecl _atol_l(char const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, locale);
}

extern "C" long long __cdecl atoll(char const* const string)
{
    return __crt_strtox::parse_integer_from_string<long long>(string, nullptr, 10, nullptr);
}

extern "C" long long __cdecl _atoll_l(char const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long long>(string, nullptr, 10, locale);
}

extern "C" __int64 __cdecl _atoi64(char const* const string)
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, nullptr, 10, nullptr);
}

extern "C" __int64 __cdecl _atoi64_l(char const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, nullptr, 10, locale);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Wide Strings => Various Integers (Simple Functions, wtox)
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" int __cdecl _wtoi(wchar_t const* const string)
{
    // Note:  We parse as a long to avoid an extra specialization of parse_integer_from_string
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, nullptr);
}

extern "C" int __cdecl _wtoi_l(wchar_t const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, locale);
}

extern "C" long __cdecl _wtol(wchar_t const* const string)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, nullptr);
}

extern "C" long __cdecl _wtol_l(wchar_t const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long>(string, nullptr, 10, locale);
}

extern "C" long long __cdecl _wtoll(wchar_t const* const string)
{
    return __crt_strtox::parse_integer_from_string<long long>(string, nullptr, 10, nullptr);
}

extern "C" long long __cdecl _wtoll_l(wchar_t const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<long long>(string, nullptr, 10, locale);
}

extern "C" __int64 __cdecl _wtoi64(wchar_t const* const string)
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, nullptr, 10, nullptr);
}

extern "C" __int64 __cdecl _wtoi64_l(wchar_t const* const string, _locale_t const locale)
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, nullptr, 10, locale);
}
