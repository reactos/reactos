/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/shimlib/shimlib.c
 * PURPOSE:         Shim helper functions
 * PROGRAMMER:      Mark Jansen
 */

#include <windows.h>
#include <shimlib.h>
#include <strsafe.h>

HINSTANCE g_hinstDll;
static HANDLE g_ShimLib_Heap;

void ShimLib_Init(HINSTANCE hInstance)
{
    g_hinstDll = hInstance;
    g_ShimLib_Heap = HeapCreate(0, 0x10000, 0);
}

void ShimLib_Deinit()
{
    // Is this a good idea?
    HeapDestroy(g_ShimLib_Heap);
}

PVOID ShimLib_ShimMalloc(SIZE_T dwSize)
{
    return HeapAlloc(g_ShimLib_Heap, 0, dwSize);
}

void ShimLib_ShimFree(PVOID pData)
{
    HeapFree(g_ShimLib_Heap, 0, pData);
}

PCSTR ShimLib_StringDuplicateA(PCSTR szString)
{
    SIZE_T Length = lstrlenA(szString);
    PSTR NewString = ShimLib_ShimMalloc(Length+1);
    return lstrcpyA(NewString, szString);
}

#if defined(_MSC_VER)

#if defined(_M_IA64) || defined(_M_AMD64)
#define _ATTRIBUTES read
#else
#define _ATTRIBUTES read
#endif


#pragma section(".shm",long,read)
#pragma section(".shm$AAA",long,read)
#pragma section(".shm$ZZZ",long,read)
#endif

#ifdef _MSC_VER
#pragma comment(linker, "/merge:.shm=.rdata")
#endif


_SHMALLOC(".shm") _PVSHIM _shim_start = 0;
_SHMALLOC(".shm$ZZZ") _PVSHIM _shim_end = 0;


/* Generic GetHookAPIs function.
   The macro's from <setup_shim.inl> and <implement_shim.inl> will register a list of all apis that should be hooked
   for a specific shim
   This helper function will return the correct shim, and call the init function */
PHOOKAPI WINAPI ShimLib_GetHookAPIs(IN LPCSTR szCommandLine, IN LPCWSTR wszShimName, OUT PDWORD pdwHookCount)
{
    uintptr_t ps = (uintptr_t)&_shim_start;
    ps += sizeof(uintptr_t);
    for (; ps != (uintptr_t)&_shim_end; ps += sizeof(uintptr_t))
    {
        _PVSHIM* pfunc = (_PVSHIM *)ps;
        if (*pfunc != NULL)
        {
            PVOID res = (*pfunc)(wszShimName);
            if (res)
            {
                PHOOKAPI (WINAPI* PFN)(DWORD, PCSTR, PDWORD) = res;
                return (*PFN)(SHIM_REASON_ATTACH, szCommandLine, pdwHookCount);
            }
        }
    }
    return NULL;
}

