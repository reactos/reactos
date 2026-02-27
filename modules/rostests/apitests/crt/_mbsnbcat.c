/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for _mbsnbcat
 * COPYRIGHT:   Copyright 2025 Stanislav Motylkov <x86corez@gmail.com>
 */

#include <apitest.h>
#include <mbstring.h>
#define WIN32_NO_STATUS

typedef unsigned char *(__cdecl *PFN__mbsncat)(unsigned char*, const unsigned char*, size_t);

extern VOID mbsncat_PerformTests(_In_ LPSTR fname, _In_opt_ PFN__mbsncat func);

START_TEST(_mbsnbcat)
{
#ifndef TEST_STATIC_CRT
    #define _mbsnbcat NULL
#endif
    mbsncat_PerformTests("_mbsnbcat", _mbsnbcat);
}
