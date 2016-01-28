/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <rtcapi.h>
#include <assert.h>
#include <internal.h>

#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:__RTC_Initialize=__RTC_NoInitialize")
#elif defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM)
#pragma comment(linker, "/alternatename:_RTC_Initialize=_RTC_NoInitialize")
#else
#error Unsupported platform
#endif

void _pei386_runtime_relocator(void)
{
}

int __mingw_init_ehandler(void)
{
    /* Nothing to do */
    return 1;
}

BOOL
WINAPI
_CRT_INIT0(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
}

int
__cdecl
Catch_RTC_Failure(
    int errType,
    const wchar_t *file,
    int line,
    const wchar_t *module,
    const wchar_t *format,
    ...)
{
    /* FIXME: better failure routine */
    __debugbreak();
    return 0;
}

extern
void
__cdecl
_RTC_NoInitialize(void)
{
    /* Do nothing, if RunTmChk.lib is not pulled in */
}

_RTC_error_fnW
__cdecl
_CRT_RTC_INITW(
    void *_Res0,
    void **_Res1,
    int _Res2,
    int _Res3,
    int _Res4)
{
    return &Catch_RTC_Failure;
}

static int initialized = 0;

void
__main(void)
{
    if (!initialized)
    {
        initialized = 1;

        _RTC_Initialize();
    }
}


