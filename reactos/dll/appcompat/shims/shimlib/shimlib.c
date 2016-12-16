/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Shim library
 * FILE:            dll/appcompat/shims/shimlib/shimlib.c
 * PURPOSE:         Shim helper functions
 * PROGRAMMER:      Mark Jansen
 */

#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>

typedef struct UsedShim
{
    SLIST_ENTRY Entry;
    PSHIMREG pShim;
#if (WINVER > _WIN32_WINNT_WS03)
    BOOL bInitCalled;
#endif
} UsedShim, *pUsedShim;


static HANDLE g_ShimLib_Heap;
static PSLIST_HEADER g_UsedShims;

void ShimLib_Init(HINSTANCE hInstance)
{
    g_ShimLib_Heap = HeapCreate(0, 0x10000, 0);

    g_UsedShims = (PSLIST_HEADER)ShimLib_ShimMalloc(sizeof(SLIST_HEADER));
    RtlInitializeSListHead(g_UsedShims);
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

BOOL ShimLib_StrAEqualsW(PCSTR szString, PCWSTR wszString)
{
    while (*szString == *wszString)
    {
        if (!*szString)
            return TRUE;

        szString++; wszString++;
    }
    return FALSE;
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


_SHMALLOC(".shm") SHIMREG _shim_start = { 0 };
_SHMALLOC(".shm$ZZZ") SHIMREG _shim_end = { 0 };


/* Generic GetHookAPIs function.
   The macro's from <setup_shim.inl> and <implement_shim.inl> will register a list of all apis that should be hooked
   for a specific shim
   This helper function will return the correct shim, and call the init function */
PHOOKAPI WINAPI ShimLib_GetHookAPIs(IN LPCSTR szCommandLine, IN LPCWSTR wszShimName, OUT PDWORD pdwHookCount)
{
    PSHIMREG ps = &_shim_start;
    ps++;
    for (; ps != &_shim_end; ps++)
    {
        if (ps->GetHookAPIs != NULL && ps->ShimName != NULL)
        {
            if (ShimLib_StrAEqualsW(ps->ShimName, wszShimName))
            {
                pUsedShim shim = (pUsedShim)ShimLib_ShimMalloc(sizeof(UsedShim));
                shim->pShim = ps;
#if (WINVER > _WIN32_WINNT_WS03)
                shim->bInitCalled = FALSE;
#endif
                RtlInterlockedPushEntrySList(g_UsedShims, &(shim->Entry));

                return ps->GetHookAPIs(SHIM_NOTIFY_ATTACH, szCommandLine, pdwHookCount);
            }
        }
    }
    return NULL;
}


BOOL WINAPI ShimLib_NotifyShims(DWORD fdwReason, PVOID ptr)
{
    PSLIST_ENTRY pEntry = RtlFirstEntrySList(g_UsedShims);

    if (fdwReason < SHIM_REASON_INIT)
        fdwReason += (SHIM_REASON_INIT - SHIM_NOTIFY_ATTACH);

    while (pEntry)
    {
        pUsedShim pUsed = CONTAINING_RECORD(pEntry, UsedShim, Entry);
        _PVNotify Notify = pUsed->pShim->Notify;
#if (WINVER > _WIN32_WINNT_WS03)
        if (pUsed->bInitCalled && fdwReason == SHIM_REASON_INIT)
            Notify = NULL;
#endif
        if (Notify)
            Notify(fdwReason, ptr);
#if (WINVER > _WIN32_WINNT_WS03)
        if (fdwReason == SHIM_REASON_INIT)
            pUsed->bInitCalled = TRUE;
#endif

        pEntry = pEntry->Next;
    }

    return TRUE;
}
