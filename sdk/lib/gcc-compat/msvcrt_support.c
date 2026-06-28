/*
 * Copyright (c) 2026 Terascale Functionalists
 * SPDX-License-Identifier: MIT
 *
 * MinGW-w64 support-library CRT shims for pre-NT6 msvcrt builds.
 */

#ifndef _CRTBLD
#define _CRTBLD
#endif

#ifndef CRTDLL
#define CRTDLL
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <io.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#undef _iswalnum_l
#undef _iswalpha_l
#undef _iswcntrl_l
#undef _iswdigit_l
#undef _iswgraph_l
#undef _iswlower_l
#undef _iswprint_l
#undef _iswpunct_l
#undef _iswspace_l
#undef _iswupper_l
#undef _iswxdigit_l
#undef _iswcsym_l
#undef _iswcsymf_l

size_t __cdecl
strnlen(const char *String, size_t MaxCount)
{
    size_t Index;

    for (Index = 0; Index < MaxCount; Index++)
    {
        if (String[Index] == '\0')
            break;
    }

    return Index;
}

size_t __cdecl
wcsnlen(const wchar_t *String, size_t MaxCount)
{
    size_t Index;

    for (Index = 0; Index < MaxCount; Index++)
    {
        if (String[Index] == L'\0')
            break;
    }

    return Index;
}

wctype_t __cdecl
wctype(const char *Property)
{
    static const struct
    {
        const char *Name;
        wctype_t Mask;
    } Properties[] =
    {
        { "alnum", _ALPHA | _DIGIT },
        { "alpha", _ALPHA },
        { "cntrl", _CONTROL },
        { "digit", _DIGIT },
        { "graph", _ALPHA | _DIGIT | _PUNCT },
        { "lower", _LOWER },
        { "print", _ALPHA | _BLANK | _DIGIT | _PUNCT },
        { "punct", _PUNCT },
        { "space", _SPACE },
        { "upper", _UPPER },
        { "xdigit", _HEX },
    };
    unsigned int Index;

    if (Property == NULL)
        return 0;

    for (Index = 0; Index < sizeof(Properties) / sizeof(Properties[0]); Index++)
    {
        const char *Left = Property;
        const char *Right = Properties[Index].Name;

        while (*Left != '\0' && *Left == *Right)
        {
            Left++;
            Right++;
        }

        if (*Left == '\0' && *Right == '\0')
            return Properties[Index].Mask;
    }

    return 0;
}

int __cdecl
fstat64(int FileDescriptor, struct _stat64 *Stat)
{
    return _fstat64(FileDescriptor, Stat);
}

__int64 __cdecl
lseek64(int FileDescriptor, __int64 Offset, int Origin)
{
    return _lseeki64(FileDescriptor, Offset, Origin);
}

__int64 __cdecl
ftello64(FILE *Stream)
{
    fpos_t Position;

    if (fgetpos(Stream, &Position) != 0)
        return -1;

    return Position;
}

int __cdecl
fseeko64(FILE *Stream, __int64 Offset, int Origin)
{
    fpos_t Position;

    switch (Origin)
    {
        case SEEK_SET:
            Position = Offset;
            break;

        case SEEK_CUR:
            Position = ftello64(Stream);
            if (Position == -1)
                return -1;
            Position += Offset;
            break;

        case SEEK_END:
            if (fseek(Stream, 0, SEEK_END) != 0)
                return -1;
            Position = ftello64(Stream);
            if (Position == -1)
                return -1;
            Position += Offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    return fsetpos(Stream, &Position);
}

int __cdecl
_fseeki64(FILE *Stream, __int64 Offset, int Origin)
{
    return fseeko64(Stream, Offset, Origin);
}

__int64 __cdecl
_ftelli64(FILE *Stream)
{
    return ftello64(Stream);
}

float __cdecl
strtof(const char *String, char **EndPointer)
{
    return (float)strtod(String, EndPointer);
}

double __cdecl
_strtod_l(const char *String, char **EndPointer, _locale_t Locale)
{
    (void)Locale;
    return strtod(String, EndPointer);
}

long long __cdecl
wcstoll(const wchar_t *String, wchar_t **EndPointer, int Base)
{
    return _wcstoi64(String, EndPointer, Base);
}

unsigned long long __cdecl
wcstoull(const wchar_t *String, wchar_t **EndPointer, int Base)
{
    return _wcstoui64(String, EndPointer, Base);
}

size_t __cdecl
mbrlen(const char *String, size_t Count, mbstate_t *State)
{
    wchar_t WideChar;

    if (String == NULL)
        return mbrtowc(NULL, NULL, 0, State);

    return mbrtowc(&WideChar, String, Count, State);
}

errno_t __cdecl
wcrtomb_s(size_t *ReturnValue,
          char *Destination,
          size_t DestinationCount,
          wchar_t WideChar,
          mbstate_t *State)
{
    char Buffer[MB_LEN_MAX];
    size_t Result;

    if (ReturnValue != NULL)
        *ReturnValue = (size_t)-1;

    if ((Destination == NULL && DestinationCount != 0) ||
        (Destination != NULL && DestinationCount == 0))
    {
        errno = EINVAL;
        return EINVAL;
    }

    Result = wcrtomb(Buffer, WideChar, State);
    if (Result == (size_t)-1)
        return errno != 0 ? errno : EILSEQ;

    if (Destination != NULL)
    {
        if (Result > DestinationCount)
        {
            errno = ERANGE;
            return ERANGE;
        }

        memcpy(Destination, Buffer, Result);
    }

    if (ReturnValue != NULL)
        *ReturnValue = Result;

    return 0;
}

errno_t __cdecl
strerror_s(char *Buffer, size_t BufferCount, int ErrorNumber)
{
    const char *Message;
    size_t Length;

    if (Buffer == NULL || BufferCount == 0)
    {
        errno = EINVAL;
        return EINVAL;
    }

    Message = strerror(ErrorNumber);
    if (Message == NULL)
        Message = "Unknown error";

    Length = strlen(Message);
    if (Length >= BufferCount)
    {
        memcpy(Buffer, Message, BufferCount - 1);
        Buffer[BufferCount - 1] = '\0';
        errno = ERANGE;
        return ERANGE;
    }

    memcpy(Buffer, Message, Length + 1);
    return 0;
}

int *__cdecl
__sys_nerr(void)
{
    static int SystemErrorCount = 43;

    return &SystemErrorCount;
}

int __cdecl
_configthreadlocale(int Flag)
{
    switch (Flag)
    {
        case 0:
        case -1:
        case _ENABLE_PER_THREAD_LOCALE:
        case _DISABLE_PER_THREAD_LOCALE:
            return _DISABLE_PER_THREAD_LOCALE;

        default:
            errno = EINVAL;
            return -1;
    }
}

_locale_t __cdecl
_create_locale(int Category, const char *Locale)
{
    static _locale_tstruct CLocale;

    if (Category < LC_MIN || Category > LC_MAX || Locale == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    return &CLocale;
}

void __cdecl
_free_locale(_locale_t Locale)
{
    (void)Locale;
}

size_t __cdecl
_strftime_l(char *Buffer,
            size_t BufferCount,
            const char *Format,
            const struct tm *Time,
            _locale_t Locale)
{
    (void)Locale;
    return strftime(Buffer, BufferCount, Format, Time);
}

int __cdecl
_strcoll_l(const char *String1, const char *String2, _locale_t Locale)
{
    (void)Locale;
    return strcoll(String1, String2);
}

size_t __cdecl
_strxfrm_l(char *Destination, const char *Source, size_t Count, _locale_t Locale)
{
    (void)Locale;
    return strxfrm(Destination, Source, Count);
}

int __cdecl
_wcscoll_l(const wchar_t *String1, const wchar_t *String2, _locale_t Locale)
{
    (void)Locale;
    return wcscoll(String1, String2);
}

size_t __cdecl
_wcsxfrm_l(wchar_t *Destination, const wchar_t *Source, size_t Count, _locale_t Locale)
{
    (void)Locale;
    return wcsxfrm(Destination, Source, Count);
}

int __cdecl
_tolower_l(int Character, _locale_t Locale)
{
    (void)Locale;
    return tolower(Character);
}

int __cdecl
_toupper_l(int Character, _locale_t Locale)
{
    (void)Locale;
    return toupper(Character);
}

int __cdecl
_iswctype_l(wint_t Character, wctype_t Type, _locale_t Locale)
{
    (void)Locale;
    return iswctype(Character, Type);
}

int __cdecl
_iswalnum_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _ALPHA | _DIGIT, Locale);
}

int __cdecl
_iswalpha_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _ALPHA, Locale);
}

int __cdecl
_iswcntrl_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _CONTROL, Locale);
}

int __cdecl
_iswdigit_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _DIGIT, Locale);
}

int __cdecl
_iswgraph_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _ALPHA | _DIGIT | _PUNCT, Locale);
}

int __cdecl
_iswlower_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _LOWER, Locale);
}

int __cdecl
_iswprint_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _ALPHA | _BLANK | _DIGIT | _PUNCT, Locale);
}

int __cdecl
_iswpunct_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _PUNCT, Locale);
}

int __cdecl
_iswspace_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _SPACE, Locale);
}

int __cdecl
_iswupper_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _UPPER, Locale);
}

int __cdecl
_iswxdigit_l(wint_t Character, _locale_t Locale)
{
    return _iswctype_l(Character, _HEX, Locale);
}

wint_t __cdecl
_towlower_l(wint_t Character, _locale_t Locale)
{
    (void)Locale;
    return towlower(Character);
}

wint_t __cdecl
_towupper_l(wint_t Character, _locale_t Locale)
{
    (void)Locale;
    return towupper(Character);
}

int __cdecl
_mbtowc_l(wchar_t *Destination, const char *Source, size_t Count, _locale_t Locale)
{
    (void)Locale;
    return mbtowc(Destination, Source, Count);
}

int __cdecl
__ms_fwprintf(FILE *Stream, const wchar_t *Format, ...)
{
    int Result;
    va_list Arguments;

    va_start(Arguments, Format);
    Result = vfwprintf(Stream, Format, Arguments);
    va_end(Arguments);

    return Result;
}

int __cdecl
__msvcrt_iswctype(wint_t Character, wctype_t Type)
{
    return iswctype(Character, Type);
}

void __cdecl
__msvcrt_assert(const char *Expression, const char *File, unsigned int Line)
{
    _assert(Expression, File, Line);
}

#ifdef _M_IX86
void *_imp__mbrtowc = mbrtowc;
void *_imp__mbrlen = mbrlen;
void *_imp__wcrtomb = wcrtomb;
void *_imp__wcrtomb_s = wcrtomb_s;
void *_imp__wctype = wctype;
void *_imp__fseeko64 = fseeko64;
void *_imp___fseeki64 = _fseeki64;
void *_imp__ftello64 = ftello64;
void *_imp___ftelli64 = _ftelli64;
void *_imp__strerror_s = strerror_s;
void *_imp____sys_nerr = __sys_nerr;
void *_imp___configthreadlocale = _configthreadlocale;
void *_imp___create_locale = _create_locale;
void *_imp___free_locale = _free_locale;
void *_imp___strftime_l = _strftime_l;
void *_imp___strcoll_l = _strcoll_l;
void *_imp___strtod_l = _strtod_l;
void *_imp___strxfrm_l = _strxfrm_l;
void *_imp___tolower_l = _tolower_l;
void *_imp___toupper_l = _toupper_l;
void *_imp___iswalnum_l = _iswalnum_l;
void *_imp___iswalpha_l = _iswalpha_l;
void *_imp___iswcntrl_l = _iswcntrl_l;
void *_imp___iswdigit_l = _iswdigit_l;
void *_imp___iswgraph_l = _iswgraph_l;
void *_imp___iswlower_l = _iswlower_l;
void *_imp___iswprint_l = _iswprint_l;
void *_imp___iswpunct_l = _iswpunct_l;
void *_imp___iswspace_l = _iswspace_l;
void *_imp___iswupper_l = _iswupper_l;
void *_imp___iswxdigit_l = _iswxdigit_l;
void *_imp___towlower_l = _towlower_l;
void *_imp___towupper_l = _towupper_l;
void *_imp___mbtowc_l = _mbtowc_l;
void *_imp___wcscoll_l = _wcscoll_l;
void *_imp___wcsxfrm_l = _wcsxfrm_l;
void *_imp____msvcrt_iswctype = __msvcrt_iswctype;
void *_imp____msvcrt_assert = __msvcrt_assert;
#else
void *__imp_mbrtowc = mbrtowc;
void *__imp_mbrlen = mbrlen;
void *__imp_wcrtomb = wcrtomb;
void *__imp_wcrtomb_s = wcrtomb_s;
void *__imp_wctype = wctype;
void *__imp_fseeko64 = fseeko64;
void *__imp__fseeki64 = _fseeki64;
void *__imp_ftello64 = ftello64;
void *__imp__ftelli64 = _ftelli64;
void *__imp_strerror_s = strerror_s;
void *__imp___sys_nerr = __sys_nerr;
void *__imp__configthreadlocale = _configthreadlocale;
void *__imp__create_locale = _create_locale;
void *__imp__free_locale = _free_locale;
void *__imp__strftime_l = _strftime_l;
void *__imp__strcoll_l = _strcoll_l;
void *__imp__strtod_l = _strtod_l;
void *__imp__strxfrm_l = _strxfrm_l;
void *__imp__tolower_l = _tolower_l;
void *__imp__toupper_l = _toupper_l;
void *__imp__iswalnum_l = _iswalnum_l;
void *__imp__iswalpha_l = _iswalpha_l;
void *__imp__iswcntrl_l = _iswcntrl_l;
void *__imp__iswdigit_l = _iswdigit_l;
void *__imp__iswgraph_l = _iswgraph_l;
void *__imp__iswlower_l = _iswlower_l;
void *__imp__iswprint_l = _iswprint_l;
void *__imp__iswpunct_l = _iswpunct_l;
void *__imp__iswspace_l = _iswspace_l;
void *__imp__iswupper_l = _iswupper_l;
void *__imp__iswxdigit_l = _iswxdigit_l;
void *__imp__towlower_l = _towlower_l;
void *__imp__towupper_l = _towupper_l;
void *__imp__mbtowc_l = _mbtowc_l;
void *__imp__wcscoll_l = _wcscoll_l;
void *__imp__wcsxfrm_l = _wcsxfrm_l;
void *__imp___msvcrt_iswctype = __msvcrt_iswctype;
void *__imp___msvcrt_assert = __msvcrt_assert;
#endif
