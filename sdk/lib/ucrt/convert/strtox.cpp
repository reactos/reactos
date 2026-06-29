//
// strtox.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The many string-to-integer conversion functions, including strtol, strtoul,
// strtoll, strtoull, wcstol, wcstoul, wcstoll, wcstoull, and various variations
// of these functions.  All of these share a common implementation that converts
// a narrow or wide character string to a 32-bit or 64-bit signed or unsigned
// integer.
//
// The base must be zero or in the range [2,36].  If the base zero is passed,
// the functions attempt to determine the base from the prefix of the string: a
// prefix of "0" indicates an octal string, a prefix of "0x" or "0X" indicates a
// hexadecimal string, and any other character indicates base 10.
//
// If the end_ptr is non-null, then if the number is successfully parsed, the
// *end_ptr is set to point to the last digit of the number (note:  it does NOT
// point one-past-the-end, STL style).
//
// The string format must be:
//
//     [whitespace] [sign] [0|0x] [digits/letters]
//
// If the string does not start with a valid number, zero is returned and end_ptr
// is set to point to the initial character in the string.
//
// If the string starts with a valid number that is representable in the target
// integer type, the number is returned and end_ptr is set to point to the last
// character of the number.
//
// If the string starts with a valid number that is not representable in the
// target integer type, end_ptr is set to point to the last character of the
// number and a sentinel value is returned:
//
//                              32-bit      64-bit
//     Unsigned / too large:    UINT_MAX    _UI64_MAX
//     Signed   / too large:    INT_MAX     _I64_MAX
//     Signed   / too small:    INT_MIN     _I64_MIN
//
// See also atox.cpp, which defines the "simple" functions--the atox and wtox
// families of functions.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal.h>
#include <corecrt_internal_strtox.h>
#include <corecrt_wstdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Narrow Strings => 32-bit Integers
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" long __cdecl strtol(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<long>(string, end_ptr, base, nullptr);
}

extern "C" long __cdecl _strtol_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<long>(string, end_ptr, base, locale);
}



extern "C" unsigned long __cdecl strtoul(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long>(string, end_ptr, base, nullptr);
}

extern "C" unsigned long __cdecl _strtoul_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long>(string, end_ptr, base, locale);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Narrow Strings => 64-bit Integers
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" __int64 __cdecl _strtoi64(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, end_ptr, base, nullptr);
}

extern "C" long long __cdecl strtoll(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<long long>(string, end_ptr, base, nullptr);
}

extern "C" intmax_t __cdecl strtoimax(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<intmax_t>(string, end_ptr, base, nullptr);
}



extern "C" __int64 __cdecl _strtoi64_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, end_ptr, base, locale);
}

extern "C" long long __cdecl _strtoll_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<long long>(string, end_ptr, base, locale);
}

extern "C" intmax_t __cdecl _strtoimax_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<intmax_t>(string, end_ptr, base, locale);
}



extern "C" unsigned __int64 __cdecl _strtoui64(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned __int64>(string, end_ptr, base, nullptr);
}

extern "C" unsigned long long __cdecl strtoull(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long long>(string, end_ptr, base, nullptr);
}

extern "C" uintmax_t __cdecl strtoumax(
    char const* const string,
    char**      const end_ptr,
    int         const base
    )
{
    return __crt_strtox::parse_integer_from_string<uintmax_t>(string, end_ptr, base, nullptr);
}



extern "C" unsigned __int64 __cdecl _strtoui64_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned __int64>(string, end_ptr, base, locale);
}

extern "C" unsigned long long __cdecl _strtoull_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long long>(string, end_ptr, base, locale);
}

extern "C" uintmax_t __cdecl _strtoumax_l(
    char const* const string,
    char**      const end_ptr,
    int         const base,
    _locale_t   const locale
    )
{
    return __crt_strtox::parse_integer_from_string<uintmax_t>(string, end_ptr, base, locale);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Wide Strings => 32-bit Integers
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" long __cdecl wcstol(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<long>(string, end_ptr, base, nullptr);
}

extern "C" long __cdecl _wcstol_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<long>(string, end_ptr, base, locale);
}

extern "C" unsigned long __cdecl wcstoul(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long>(string, end_ptr, base, nullptr);
}

extern "C" unsigned long __cdecl _wcstoul_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long>(string, end_ptr, base, locale);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Wide Strings => 64-bit Integers
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" __int64 __cdecl _wcstoi64(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, end_ptr, base, nullptr);
}

extern "C" long long __cdecl wcstoll(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<long long>(string, end_ptr, base, nullptr);
}

extern "C" intmax_t __cdecl wcstoimax(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<intmax_t>(string, end_ptr, base, nullptr);
}



extern "C" __int64 __cdecl _wcstoi64_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<__int64>(string, end_ptr, base, locale);
}

extern "C" long long __cdecl _wcstoll_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<long long>(string, end_ptr, base, locale);
}

extern "C" intmax_t __cdecl _wcstoimax_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<intmax_t>(string, end_ptr, base, locale);
}



extern "C" unsigned __int64 __cdecl _wcstoui64(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned __int64>(string, end_ptr, base, nullptr);
}

extern "C" unsigned long long __cdecl wcstoull(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long long>(string, end_ptr, base, nullptr);
}

extern "C" uintmax_t __cdecl wcstoumax(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base
    )
{
    return __crt_strtox::parse_integer_from_string<uintmax_t>(string, end_ptr, base, nullptr);
}



extern "C" unsigned __int64 __cdecl _wcstoui64_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned __int64>(string, end_ptr, base, locale);
}

extern "C" unsigned long long __cdecl _wcstoull_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<unsigned long long>(string, end_ptr, base, locale);
}

extern "C" uintmax_t __cdecl _wcstoumax_l(
    wchar_t const* const string,
    wchar_t**      const end_ptr,
    int            const base,
    _locale_t      const locale
    )
{
    return __crt_strtox::parse_integer_from_string<uintmax_t>(string, end_ptr, base, locale);
}
