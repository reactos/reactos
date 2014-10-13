/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#include <windows.h>
#include <stdlib.h>

void _pei386_runtime_relocator(void)
{
}

int __mingw_init_ehandler(void)
{
    /* Nothing to do */
    return 1;
}

void
__do_global_dtors(void)
{

}

void
__do_global_ctors(void)
{

}

BOOL
WINAPI
_CRT_INIT0(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
}

void Catch_RTC_Failure(int errType, const wchar_t *file, int line, 
                       const wchar_t *module, const wchar_t *format, ...)
{
    /* FIXME: better failure routine */
    __debugbreak();
}

void* __cdecl _CRT_RTC_INITW(void *res0, void **res1, int res2, int res3, int res4)
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
        __do_global_ctors ();
    }
}


