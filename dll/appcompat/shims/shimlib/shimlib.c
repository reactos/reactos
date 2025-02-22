/*
 * PROJECT:     ReactOS Shim helper library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Shim helper functions
 * COPYRIGHT:   Copyright 2016-2018 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <shimlib.h>
#include <strsafe.h>
#include <ndk/rtlfuncs.h>

typedef struct UsedShim
{
    SLIST_ENTRY Entry;
    PSHIMREG pShim;
#if (WINVER > _WIN32_WINNT_WS03)
    BOOL bInitCalled;
#endif
} UsedShim, *pUsedShim;


ULONG g_ShimEngDebugLevel = 0xffffffff;
static HINSTANCE g_ShimLib_hInstance;
static HANDLE g_ShimLib_Heap;
static PSLIST_HEADER g_UsedShims;

void ShimLib_Init(HINSTANCE hInstance)
{
    g_ShimLib_hInstance = hInstance;
    g_ShimLib_Heap = HeapCreate(0, 0x10000, 0);

    g_UsedShims = (PSLIST_HEADER)ShimLib_ShimMalloc(sizeof(SLIST_HEADER));
    RtlInitializeSListHead(g_UsedShims);
}

void ShimLib_Deinit(VOID)
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

HINSTANCE ShimLib_Instance(VOID)
{
    return g_ShimLib_hInstance;
}

PCSTR ShimLib_StringNDuplicateA(PCSTR szString, SIZE_T stringLengthIncludingNullTerm)
{
    PSTR NewString = ShimLib_ShimMalloc(stringLengthIncludingNullTerm);
    StringCchCopyA(NewString, stringLengthIncludingNullTerm, szString);
    return NewString;
}

PCSTR ShimLib_StringDuplicateA(PCSTR szString)
{
    return ShimLib_StringNDuplicateA(szString, lstrlenA(szString) + 1);
}

BOOL ShimLib_StrAEqualsWNC(PCSTR szString, PCWSTR wszString)
{
    while (toupper(*szString) == towupper(*wszString))
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


_SHMALLOC(".shm$AAA") SHIMREG _shim_start = { 0 };
_SHMALLOC(".shm$ZZZ") SHIMREG _shim_end = { 0 };


/* Generic GetHookAPIs function.
   The macro's from <setup_shim.inl> and <implement_shim.inl> will register a list of all apis that should be hooked
   for a specific shim
   This helper function will return the correct shim, and call the init function */
PHOOKAPI WINAPI ShimLib_GetHookAPIs(IN LPCSTR szCommandLine, IN LPCWSTR wszShimName, OUT PDWORD pdwHookCount)
{
    PSHIMREG ps = &_shim_start;
    ps++;
    for (; ps < &_shim_end; ps++)
    {
        if (ps->GetHookAPIs != NULL && ps->ShimName != NULL)
        {
            if (ShimLib_StrAEqualsWNC(ps->ShimName, wszShimName))
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


VOID SeiInitDebugSupport(VOID)
{
    static const UNICODE_STRING DebugKey = RTL_CONSTANT_STRING(L"SHIM_DEBUG_LEVEL");
    UNICODE_STRING DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = SEI_MSG;
    WCHAR Buffer[40];

    RtlInitEmptyUnicodeString(&DebugValue, Buffer, sizeof(Buffer));

    Status = RtlQueryEnvironmentVariable_U(NULL, &DebugKey, &DebugValue);

    if (NT_SUCCESS(Status))
    {
        if (!NT_SUCCESS(RtlUnicodeStringToInteger(&DebugValue, 10, &NewLevel)))
            NewLevel = 0;
    }
    g_ShimEngDebugLevel = NewLevel;
}


/**
* Outputs diagnostic info.
*
* @param [in]  Level           The level to log this message with, choose any of [SHIM_ERR,
*                              SHIM_WARN, SHIM_INFO].
* @param [in]  FunctionName    The function this log should be attributed to.
* @param [in]  Format          The format string.
* @param   ...                 Variable arguments providing additional information.
*
* @return  Success: TRUE Failure: FALSE.
*/
BOOL WINAPIV SeiDbgPrint(SEI_LOG_LEVEL Level, PCSTR Function, PCSTR Format, ...)
{
    char Buffer[512];
    char* Current = Buffer;
    const char* LevelStr;
    size_t Length = sizeof(Buffer);
    va_list ArgList;
    HRESULT hr;

    if (g_ShimEngDebugLevel == 0xffffffff)
        SeiInitDebugSupport();

    if (Level > g_ShimEngDebugLevel)
        return FALSE;

    switch (Level)
    {
    case SEI_MSG:
        LevelStr = "MSG ";
        break;
    case SEI_FAIL:
        LevelStr = "FAIL";
        break;
    case SEI_WARN:
        LevelStr = "WARN";
        break;
    case SEI_INFO:
        LevelStr = "INFO";
        break;
    default:
        LevelStr = "USER";
        break;
    }

    if (Function)
        hr = StringCchPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, "[%s] [%s] ", LevelStr, Function);
    else
        hr = StringCchPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, "[%s] ", LevelStr);

    if (!SUCCEEDED(hr))
        return FALSE;

    va_start(ArgList, Format);
    hr = StringCchVPrintfExA(Current, Length, &Current, &Length, STRSAFE_NULL_ON_FAILURE, Format, ArgList);
    va_end(ArgList);
    if (!SUCCEEDED(hr))
        return FALSE;

    DbgPrint("%s", Buffer);
    return TRUE;
}

