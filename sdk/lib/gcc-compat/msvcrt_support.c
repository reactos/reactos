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
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>
#include <wchar.h>
#include <wctype.h>

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

void __cdecl
__msvcrt_assert(const char *Expression, const char *File, unsigned int Line)
{
    _assert(Expression, File, Line);
}

#ifdef _M_IX86
void *_imp__mbrtowc = mbrtowc;
void *_imp__wcrtomb = wcrtomb;
void *_imp__wctype = wctype;
void *_imp__fseeko64 = fseeko64;
void *_imp__ftello64 = ftello64;
void *_imp____msvcrt_assert = __msvcrt_assert;
#else
void *__imp_mbrtowc = mbrtowc;
void *__imp_wcrtomb = wcrtomb;
void *__imp_wctype = wctype;
void *__imp_fseeko64 = fseeko64;
void *__imp_ftello64 = ftello64;
void *__imp___msvcrt_assert = __msvcrt_assert;
#endif
