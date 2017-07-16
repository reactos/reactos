/*
 * Copyright 2015-2017 Mark Jansen (mark.jansen@reactos.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS
#include "windows.h"
#include "ntndk.h"
#define IN_APPHELP
#include "shimlib.h"
#include <strsafe.h>
/* Make sure we don't include apphelp logging */
#define APPHELP_NOSDBPAPI
#include "apphelp.h"
#include "shimeng.h"


typedef FARPROC(WINAPI* GETPROCADDRESSPROC)(HINSTANCE, LPCSTR);


FARPROC WINAPI StubGetProcAddress(HINSTANCE hModule, LPCSTR lpProcName);
BOOL WINAPI SE_IsShimDll(PVOID BaseAddress);


extern HMODULE g_hInstance;
ULONG g_ShimEngDebugLevel = 0xffffffff;
BOOL g_bComPlusImage = FALSE;
BOOL g_bShimDuringInit = FALSE;
BOOL g_bInternalHooksUsed = FALSE;
static ARRAY g_pShimInfo;  /* SHIMMODULE */
static ARRAY g_pHookArray; /* HOOKMODULEINFO */

HOOKAPIEX g_IntHookEx[] =
{
    {
        "kernel32.dll",     /* LibraryName */
        "GetProcAddress",   /* FunctionName */
        StubGetProcAddress, /* ReplacementFunction*/
        NULL,               /* OriginalFunction */
        { NULL },           /* ModuleLink */
        { NULL }            /* ApiLink */
    },
};




static BOOL ARRAY_EnsureSize(PARRAY Array, DWORD ItemSize, DWORD GrowWith)
{
    PVOID pNewData;
    DWORD Count;

    if (Array->MaxSize > Array->Size)
        return TRUE;

    Count = Array->Size + GrowWith;
    pNewData = SeiAlloc(Count * ItemSize);

    if (!pNewData)
    {
        SHIMENG_FAIL("Failed to allocate %d bytes\n", Count * ItemSize);
        return FALSE;
    }
    Array->MaxSize = Count;

    if (Array->Data)
    {
        memcpy(pNewData, Array->Data, Array->Size * ItemSize);
        SeiFree(Array->Data);
    }
    Array->Data = pNewData;

    return TRUE;
}



VOID SeiInitDebugSupport(VOID)
{
    static const UNICODE_STRING DebugKey = RTL_CONSTANT_STRING(L"SHIMENG_DEBUG_LEVEL");
    UNICODE_STRING DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = 0;
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


PVOID SeiGetModuleFromAddress(PVOID addr)
{
    PVOID hModule = NULL;
    RtlPcToFileHeader(addr, &hModule);
    return hModule;
}



/* TODO: Guard against recursive calling / calling init multiple times! */
VOID NotifyShims(DWORD dwReason, PVOID Info)
{
    PSHIMMODULE Data;
    DWORD n;

    Data = g_pShimInfo.Data;
    for (n = 0; n < g_pShimInfo.Size; ++n)
    {
        if (!Data[n].pNotifyShims)
            continue;

        Data[n].pNotifyShims(dwReason, Info);
    }
}



VOID SeiCheckComPlusImage(PVOID BaseAddress)
{
    ULONG ComSectionSize;
    g_bComPlusImage = RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &ComSectionSize) != NULL;

    SHIMENG_INFO("COM+ executable %s\n", g_bComPlusImage ? "TRUE" : "FALSE");
}


PSHIMMODULE SeiGetShimInfo(PVOID BaseAddress)
{
    PSHIMMODULE Data;
    DWORD n;

    Data = g_pShimInfo.Data;
    for (n = 0; n < g_pShimInfo.Size; ++n)
    {
        if (Data[n].BaseAddress == BaseAddress)
            return &Data[n];
    }
    return NULL;
}

PSHIMMODULE SeiCreateShimInfo(PCWSTR DllName, PVOID BaseAddress)
{
    static const ANSI_STRING GetHookAPIs = RTL_CONSTANT_STRING("GetHookAPIs");
    static const ANSI_STRING NotifyShims = RTL_CONSTANT_STRING("NotifyShims");
    PSHIMMODULE Data;
    PVOID pGetHookAPIs, pNotifyShims;

    if (!NT_SUCCESS(LdrGetProcedureAddress(BaseAddress, (PANSI_STRING)&GetHookAPIs, 0, &pGetHookAPIs)) ||
        !NT_SUCCESS(LdrGetProcedureAddress(BaseAddress, (PANSI_STRING)&NotifyShims, 0, &pNotifyShims)))
    {
        SHIMENG_WARN("Failed to resolve entry points for %S\n", DllName);
        return NULL;
    }

    if (!ARRAY_EnsureSize(&g_pShimInfo, sizeof(SHIMMODULE), 5))
        return NULL;

    Data = g_pShimInfo.Data;
    Data += g_pShimInfo.Size;
    g_pShimInfo.Size++;

    RtlCreateUnicodeString(&Data->Name, DllName);
    Data->BaseAddress = BaseAddress;

    Data->pGetHookAPIs = pGetHookAPIs;
    Data->pNotifyShims = pNotifyShims;

    Data->HookApis.Data = NULL;
    Data->HookApis.Size = 0;
    Data->HookApis.MaxSize = 0;

    return Data;
}

VOID SeiAppendHookInfo(PSHIMMODULE pShimInfo, PHOOKAPIEX pHookApi, DWORD dwHookCount)
{
    PHOOKAPIPOINTERS Data;
    if (!ARRAY_EnsureSize(&pShimInfo->HookApis, sizeof(HOOKAPIPOINTERS), 5))
        return;

    Data = pShimInfo->HookApis.Data;
    Data += pShimInfo->HookApis.Size;

    Data->pHookApi = pHookApi;
    Data->dwHookCount = dwHookCount;
    pShimInfo->HookApis.Size++;
}

PHOOKMODULEINFO SeiFindHookModuleInfo(PUNICODE_STRING ModuleName, PVOID BaseAddress)
{
    DWORD n;
    PHOOKMODULEINFO Data = g_pHookArray.Data;

    for (n = 0; n < g_pHookArray.Size; ++n)
    {
        if (BaseAddress && BaseAddress == Data[n].BaseAddress)
            return &Data[n];

        if (!BaseAddress && RtlEqualUnicodeString(ModuleName, &Data[n].Name, TRUE))
            return &Data[n];
    }

    return NULL;
}

PHOOKMODULEINFO SeiFindHookModuleInfoForImportDescriptor(PBYTE DllBase, PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor)
{
    UNICODE_STRING DllName;
    PVOID DllHandle;
    NTSTATUS Success;

    if (!RtlCreateUnicodeStringFromAsciiz(&DllName, (PCSZ)(DllBase + ImportDescriptor->Name)))
    {
        SHIMENG_FAIL("Unable to convert dll name to unicode\n");
        return NULL;
    }

    Success = LdrGetDllHandle(NULL, NULL, &DllName, &DllHandle);
    RtlFreeUnicodeString(&DllName);

    if (!NT_SUCCESS(Success))
    {
        SHIMENG_FAIL("Unable to get module handle for %wZ\n", &DllName);
        return NULL;
    }

    return SeiFindHookModuleInfo(NULL, DllHandle);
}

static LPCWSTR SeiGetStringPtr(PDB pdb, TAGID tag, TAG type)
{
    TAGID tagEntry = SdbFindFirstTag(pdb, tag, type);
    if (tagEntry == TAGID_NULL)
        return NULL;

    return SdbGetStringTagPtr(pdb, tagEntry);
}

static DWORD SeiGetDWORD(PDB pdb, TAGID tag, TAG type)
{
    TAGID tagEntry = SdbFindFirstTag(pdb, tag, type);
    if (tagEntry == TAGID_NULL)
        return 0;

    return SdbReadDWORDTag(pdb, tagEntry, TAGID_NULL);
}


static VOID SeiAddShim(TAGREF trShimRef, PARRAY pShimRef)
{
    TAGREF* Data;
    if (!ARRAY_EnsureSize(pShimRef, sizeof(TAGREF), 10))
        return;

    Data = pShimRef->Data;
    Data[pShimRef->Size] = trShimRef;
    pShimRef->Size++;
}

static VOID SeiSetLayerEnvVar(LPCWSTR wszLayer)
{
    NTSTATUS Status;
    UNICODE_STRING VarName = RTL_CONSTANT_STRING(L"__COMPAT_LAYER");
    UNICODE_STRING Value;

    RtlInitUnicodeString(&Value, wszLayer);

    Status = RtlSetEnvironmentVariable(NULL, &VarName, &Value);
    if (NT_SUCCESS(Status))
        SHIMENG_INFO("Set env var %wZ=%wZ\n", &VarName, &Value);
    else
        SHIMENG_FAIL("Failed to set %wZ: 0x%x\n", &VarName, Status);
}

#define MAX_LAYER_LENGTH            256

static VOID SeiBuildShimRefArray(HSDB hsdb, SDBQUERYRESULT* pQuery, PARRAY pShimRef)
{
    WCHAR wszLayerEnvVar[MAX_LAYER_LENGTH] = { 0 };
    DWORD n;

    for (n = 0; n < pQuery->dwExeCount; ++n)
    {
        PDB pdb;
        TAGID tag;
        if (SdbTagRefToTagID(hsdb, pQuery->atrExes[n], &pdb, &tag))
        {
            LPCWSTR ExeName = SeiGetStringPtr(pdb, tag, TAG_NAME);
            TAGID ShimRef = SdbFindFirstTag(pdb, tag, TAG_SHIM_REF);

            if (ExeName)
                SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Exe(%S))\n", ExeName);

            while (ShimRef != TAGID_NULL)
            {
                TAGREF trShimRef;
                if (SdbTagIDToTagRef(hsdb, pdb, ShimRef, &trShimRef))
                    SeiAddShim(trShimRef, pShimRef);


                ShimRef = SdbFindNextTag(pdb, tag, ShimRef);
            }

            /* Handle FLAG_REF */
        }
    }


    for (n = 0; n < pQuery->dwLayerCount; ++n)
    {
        PDB pdb;
        TAGID tag;
        if (SdbTagRefToTagID(hsdb, pQuery->atrLayers[n], &pdb, &tag))
        {
            LPCWSTR LayerName = SeiGetStringPtr(pdb, tag, TAG_NAME);
            TAGID ShimRef = SdbFindFirstTag(pdb, tag, TAG_SHIM_REF);
            if (LayerName)
            {
                HRESULT hr;
                SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Layer(%S))\n", LayerName);
                if (wszLayerEnvVar[0])
                    StringCchCatW(wszLayerEnvVar, _countof(wszLayerEnvVar), L" ");
                hr = StringCchCatW(wszLayerEnvVar, _countof(wszLayerEnvVar), LayerName);
                if (!SUCCEEDED(hr))
                {
                    SHIMENG_FAIL("Unable to append %S\n", LayerName);
                }
            }

            while (ShimRef != TAGID_NULL)
            {
                TAGREF trShimRef;
                if (SdbTagIDToTagRef(hsdb, pdb, ShimRef, &trShimRef))
                    SeiAddShim(trShimRef, pShimRef);

                ShimRef = SdbFindNextTag(pdb, tag, ShimRef);
            }

            /* Handle FLAG_REF */
        }
    }
    if (wszLayerEnvVar[0])
        SeiSetLayerEnvVar(wszLayerEnvVar);
}


VOID SeiAddHooks(PHOOKAPIEX hooks, DWORD dwHookCount)
{
    DWORD n;
    UNICODE_STRING UnicodeModName;
    WCHAR Buf[512];

    RtlInitEmptyUnicodeString(&UnicodeModName, Buf, sizeof(Buf));

    for (n = 0; n < dwHookCount; ++n)
    {
        ANSI_STRING AnsiString;
        PVOID DllHandle;
        PHOOKAPIEX hook = hooks + n;
        PHOOKMODULEINFO HookModuleInfo;
        PSINGLE_LIST_ENTRY Entry;

        RtlInitAnsiString(&AnsiString, hook->LibraryName);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeModName, &AnsiString, FALSE)))
        {
            SHIMENG_FAIL("Unable to convert %s to Unicode\n", hook->LibraryName);
            continue;
        }

        RtlInitAnsiString(&AnsiString, hook->FunctionName);
        if (NT_SUCCESS(LdrGetDllHandle(NULL, 0, &UnicodeModName, &DllHandle)))
        {
            PVOID ProcAddress;


            if (!NT_SUCCESS(LdrGetProcedureAddress(DllHandle, &AnsiString, 0, &ProcAddress)))
            {
                SHIMENG_FAIL("Unable to retrieve %s!%s\n", hook->LibraryName, hook->FunctionName);
                continue;
            }

            HookModuleInfo = SeiFindHookModuleInfo(NULL, DllHandle);
            hook->OriginalFunction = ProcAddress;
        }
        else
        {
            HookModuleInfo = SeiFindHookModuleInfo(&UnicodeModName, NULL);
            DllHandle = NULL;
        }

        if (!HookModuleInfo)
        {
            if (!ARRAY_EnsureSize(&g_pHookArray, sizeof(HOOKMODULEINFO), 5))
                continue;

            HookModuleInfo = g_pHookArray.Data;
            HookModuleInfo += g_pHookArray.Size;
            g_pHookArray.Size++;

            HookModuleInfo->BaseAddress = DllHandle;
            RtlCreateUnicodeString(&HookModuleInfo->Name, UnicodeModName.Buffer);
        }

        Entry = &HookModuleInfo->ModuleLink;

        while (Entry && Entry->Next)
        {
            PHOOKAPIEX HookApi = CONTAINING_RECORD(Entry->Next, HOOKAPIEX, ModuleLink);

            int CmpResult = strcmp(hook->FunctionName, HookApi->FunctionName);
            if (CmpResult == 0)
            {
                /* Multiple hooks on one function? --> use ApiLink */
                SHIMENG_FAIL("Multiple hooks on one API is not yet supported!\n");
                ASSERT(0);
            }
            else if (CmpResult < 0)
            {
                /* Break out of the loop to have the entry inserted 'in place' */
                break;
            }

            Entry = Entry->Next;
        }
        /* If Entry is not NULL, the item is not inserted yet, so link it at the end. */
        if (Entry)
        {
            hook->ModuleLink.Next = Entry->Next;
            Entry->Next = &hook->ModuleLink;
        }
    }
}


FARPROC WINAPI StubGetProcAddress(HINSTANCE hModule, LPCSTR lpProcName)
{
    char szOrdProcName[10] = "";
    LPCSTR lpPrintName = lpProcName;
    PVOID Addr = _ReturnAddress();
    PHOOKMODULEINFO HookModuleInfo;
    FARPROC proc = ((GETPROCADDRESSPROC)g_IntHookEx[0].OriginalFunction)(hModule, lpProcName);

    if (!HIWORD(lpProcName))
    {
        sprintf(szOrdProcName, "#%lu", (DWORD)lpProcName);
        lpPrintName = szOrdProcName;
    }

    Addr = SeiGetModuleFromAddress(Addr);
    if (SE_IsShimDll(Addr))
    {
        SHIMENG_MSG("Not touching GetProcAddress for shim dll (%p!%s)", hModule, lpPrintName);
        return proc;
    }

    SHIMENG_MSG("(GetProcAddress(%p!%s) => %p\n", hModule, lpProcName, lpPrintName);

    HookModuleInfo = SeiFindHookModuleInfo(NULL, hModule);

    /* FIXME: Ordinal not yet supported */
    if (HookModuleInfo && HIWORD(lpProcName))
    {
        PSINGLE_LIST_ENTRY Entry;

        Entry = HookModuleInfo->ModuleLink.Next;

        while (Entry)
        {
            PHOOKAPIEX HookApi = CONTAINING_RECORD(Entry, HOOKAPIEX, ModuleLink);

            int CmpResult = strcmp(lpProcName, HookApi->FunctionName);
            if (CmpResult == 0)
            {
                SHIMENG_MSG("Redirecting %p to %p\n", proc, HookApi->ReplacementFunction);
                proc = HookApi->ReplacementFunction;
                break;
            }
            else if (CmpResult < 0)
            {
                SHIMENG_MSG("Not found %s\n", lpProcName);
                /* We are not going to find it anymore.. */
                break;
            }

            Entry = Entry->Next;
        }
    }

    return proc;
}

VOID SeiResolveAPIs(VOID)
{
    PSHIMMODULE Data;
    DWORD mod, n;

    Data = g_pShimInfo.Data;

    /* Enumerate all Shim modules */
    for (mod = 0; mod < g_pShimInfo.Size; ++mod)
    {
        PHOOKAPIPOINTERS pShims = Data[mod].HookApis.Data;
        DWORD dwShimCount = Data[mod].HookApis.Size;

        /* Enumerate all Shims */
        for (n = 0; n < dwShimCount; ++n)
        {
            PHOOKAPIEX hooks = pShims[n].pHookApi;
            DWORD dwHookCount = pShims[n].dwHookCount;

            SeiAddHooks(hooks, dwHookCount);
        }
    }
}

VOID SeiAddInternalHooks(DWORD dwNumHooks)
{
    if (dwNumHooks == 0)
    {
        g_bInternalHooksUsed = FALSE;
        return;
    }

    SeiAddHooks(g_IntHookEx, _countof(g_IntHookEx));
    g_bInternalHooksUsed = TRUE;
}

VOID SeiPatchNewImport(PIMAGE_THUNK_DATA FirstThunk, PHOOKAPIEX HookApi, PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG OldProtection = 0;
    PVOID Ptr;
    ULONG Size;
    NTSTATUS Status;

    SHIMENG_INFO("Hooking API \"%s!%s\" for DLL \"%wZ\"\n", HookApi->LibraryName, HookApi->FunctionName, &LdrEntry->BaseDllName);

    Ptr = &FirstThunk->u1.Function;
    Size = sizeof(FirstThunk->u1.Function);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Ptr, &Size, PAGE_EXECUTE_READWRITE, &OldProtection);

    if (!NT_SUCCESS(Status))
    {
        SHIMENG_FAIL("Unable to unprotect 0x%p\n", &FirstThunk->u1.Function);
        return;
    }

    SHIMENG_INFO("changing 0x%p to 0x%p\n", FirstThunk->u1.Function, HookApi->ReplacementFunction);
#ifdef _WIN64
    FirstThunk->u1.Function = (ULONGLONG)HookApi->ReplacementFunction;
#else
    FirstThunk->u1.Function = (DWORD)HookApi->ReplacementFunction;
#endif

    Size = sizeof(FirstThunk->u1.Function);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Ptr, &Size, OldProtection, &OldProtection);

    if (!NT_SUCCESS(Status))
    {
        SHIMENG_WARN("Unable to reprotect 0x%p\n", &FirstThunk->u1.Function);
    }
}

/* Level(INFO) [SeiPrintExcludeInfo] Module "kernel32.dll" excluded for shim VistaRTMVersionLie, API "NTDLL.DLL!RtlGetVersion", because it is in System32/WinSXS. */

VOID SeiHookImports(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG Size;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PBYTE DllBase = LdrEntry->DllBase;

    if (SE_IsShimDll(DllBase) || g_hInstance == LdrEntry->DllBase)
    {
        SHIMENG_INFO("Skipping shim module 0x%p \"%wZ\"\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        return;
    }

    ImportDescriptor = RtlImageDirectoryEntryToData(DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size);
    if (!ImportDescriptor)
    {
        SHIMENG_INFO("Skipping module 0x%p \"%wZ\" due to no iat found\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        return;
    }

    SHIMENG_INFO("Hooking module 0x%p \"%wZ\"\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);

    for ( ;ImportDescriptor->Name && ImportDescriptor->OriginalFirstThunk; ImportDescriptor++)
    {
        PHOOKMODULEINFO HookModuleInfo;

        /* Do we have hooks for this module? */
        HookModuleInfo = SeiFindHookModuleInfoForImportDescriptor(DllBase, ImportDescriptor);

        if (HookModuleInfo)
        {
            PIMAGE_THUNK_DATA OriginalThunk, FirstThunk;
            PSINGLE_LIST_ENTRY Entry;

            Entry = HookModuleInfo->ModuleLink.Next;

            while (Entry)
            {
                DWORD dwFound = 0;
                OriginalThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->OriginalFirstThunk);
                FirstThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->FirstThunk);

                for (;OriginalThunk->u1.AddressOfData && FirstThunk->u1.Function && Entry; OriginalThunk++, FirstThunk++)
                {
                    PHOOKAPIEX HookApi = CONTAINING_RECORD(Entry, HOOKAPIEX, ModuleLink);

                    if (!IMAGE_SNAP_BY_ORDINAL32(OriginalThunk->u1.AddressOfData))
                    {
                        PIMAGE_IMPORT_BY_NAME ImportName;

                        ImportName = (PIMAGE_IMPORT_BY_NAME)(DllBase + OriginalThunk->u1.AddressOfData);
                        if (!strcmp((PCSTR)ImportName->Name, HookApi->FunctionName))
                        {
                            SeiPatchNewImport(FirstThunk, HookApi, LdrEntry);

                            /* Sadly, iat does not have to be sorted, and can even contain duplicate entries. */
                            dwFound++;
                        }
                    }
                    else
                    {
                        SHIMENG_FAIL("Ordinals not yet supported\n");
                        ASSERT(0);
                    }
                }

                if (Entry)
                {
                    if (dwFound != 1)
                    {
                        /* One entry not found. */
                        PHOOKAPIEX HookApi = CONTAINING_RECORD(Entry, HOOKAPIEX, ModuleLink);
                        if (!dwFound)
                            SHIMENG_INFO("Entry \"%s!%s\" not found for \"%wZ\"\n", HookApi->LibraryName, HookApi->FunctionName, &LdrEntry->BaseDllName);
                        else
                            SHIMENG_INFO("Entry \"%s!%s\" found %d times for \"%wZ\"\n", HookApi->LibraryName, HookApi->FunctionName, dwFound, &LdrEntry->BaseDllName);
                    }

                    Entry = Entry->Next;
                }
            }
        }
    }
}


VOID PatchNewModules(PPEB Peb)
{
    PLIST_ENTRY ListHead, ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;

    while (ListHead != ListEntry)
    {
        LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        SeiHookImports(LdrEntry);

        ListEntry = ListEntry->Flink;
    }
}


/*
Level(INFO) Using USER apphack flags 0x2080000
*/

VOID SeiInit(PUNICODE_STRING ProcessImage, HSDB hsdb, SDBQUERYRESULT* pQuery)
{
    DWORD n;
    ARRAY ShimRef;
    TAGREF* Data;
    DWORD dwTotalHooks = 0;

    PPEB Peb = NtCurrentPeb();

    ShimRef.Data = NULL;
    ShimRef.Size = ShimRef.MaxSize = 0;

    SeiCheckComPlusImage(Peb->ImageBaseAddress);

    /* TODO:
    if (pQuery->trApphelp)
        SeiDisplayAppHelp(?pQuery->trApphelp?);
    */

    SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(ExePath(%wZ))\n", ProcessImage);
    SeiBuildShimRefArray(hsdb, pQuery, &ShimRef);
    SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Complete)\n");

    SHIMENG_INFO("Got %d shims\n", ShimRef.Size);
    /* TODO:
    SeiBuildGlobalInclExclList()
    */

    Data = ShimRef.Data;

    for (n = 0; n < ShimRef.Size; ++n)
    {
        PDB pdb;
        TAGID ShimRef;
        if (SdbTagRefToTagID(hsdb, Data[n], &pdb, &ShimRef))
        {
            LPCWSTR ShimName, DllName, CommandLine = NULL;
            TAGID ShimTag;
            WCHAR FullNameBuffer[MAX_PATH];
            UNICODE_STRING UnicodeDllName;
            PVOID BaseAddress;
            PSHIMMODULE pShimInfo = NULL;
            ANSI_STRING AnsiCommandLine = RTL_CONSTANT_STRING("");
            PHOOKAPIEX pHookApi;
            DWORD dwHookCount;

            ShimName = SeiGetStringPtr(pdb, ShimRef, TAG_NAME);
            if (!ShimName)
            {
                SHIMENG_FAIL("Failed to retrieve the name for 0x%x\n", Data[n]);
                continue;
            }

            CommandLine = SeiGetStringPtr(pdb, ShimRef, TAG_COMMAND_LINE);
            if (CommandLine && *CommandLine)
            {
                RtlInitUnicodeString(&UnicodeDllName, CommandLine);
                if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiCommandLine, &UnicodeDllName, TRUE)))
                {
                    SHIMENG_INFO("COMMAND LINE %s for %S", AnsiCommandLine.Buffer, ShimName);
                }
                else
                {
                    AnsiCommandLine.Buffer = "";
                    CommandLine = NULL;
                }
            }

            ShimTag = SeiGetDWORD(pdb, ShimRef, TAG_SHIM_TAGID);
            if (!ShimTag)
            {
                SHIMENG_FAIL("Failed to resolve %S to a shim\n", ShimName);
                continue;
            }

            if (!SdbGetAppPatchDir(NULL, FullNameBuffer, _countof(FullNameBuffer)))
            {
                SHIMENG_WARN("Failed to get the AppPatch dir\n");
                continue;
            }

            DllName = SeiGetStringPtr(pdb, ShimTag, TAG_DLLFILE);
            if (DllName == NULL ||
                !SUCCEEDED(StringCchCatW(FullNameBuffer, _countof(FullNameBuffer), L"\\")) ||
                !SUCCEEDED(StringCchCatW(FullNameBuffer, _countof(FullNameBuffer), DllName)))
            {
                SHIMENG_WARN("Failed to build a full path for %S\n", ShimName);
                continue;
            }

            RtlInitUnicodeString(&UnicodeDllName, FullNameBuffer);
            if (NT_SUCCESS(LdrGetDllHandle(NULL, NULL, &UnicodeDllName, &BaseAddress)))
            {
                pShimInfo = SeiGetShimInfo(BaseAddress);
            }
            else if (!NT_SUCCESS(LdrLoadDll(NULL, NULL, &UnicodeDllName, &BaseAddress)))
            {
                SHIMENG_WARN("Failed to load %wZ for %S\n", &UnicodeDllName, ShimName);
                continue;
            }
            if (!pShimInfo)
            {
                pShimInfo = SeiCreateShimInfo(DllName, BaseAddress);
                if (!pShimInfo)
                {
                    SHIMENG_FAIL("Failed to allocate ShimInfo for %S\n", DllName);
                    continue;
                }
            }

            SHIMENG_INFO("Shim DLL 0x%p \"%wZ\" loaded\n", BaseAddress, &UnicodeDllName);
            SHIMENG_INFO("Using SHIM \"%S!%S\"\n", DllName, ShimName);

            pHookApi = pShimInfo->pGetHookAPIs(AnsiCommandLine.Buffer, ShimName, &dwHookCount);
            SHIMENG_INFO("GetHookAPIs returns %d hooks for DLL \"%wZ\" SHIM \"%S\"\n", dwHookCount, &UnicodeDllName, ShimName);
            if (dwHookCount)
                SeiAppendHookInfo(pShimInfo, pHookApi, dwHookCount);

            if (CommandLine && *CommandLine)
                RtlFreeAnsiString(&AnsiCommandLine);

            dwTotalHooks += dwHookCount;
            /*SeiBuildInclExclList*/
        }
    }

    SeiAddInternalHooks(dwTotalHooks);
    SeiResolveAPIs();
    PatchNewModules(Peb);
}



BOOL SeiGetShimData(PUNICODE_STRING ProcessImage, PVOID pShimData, HSDB* pHsdb, SDBQUERYRESULT* pQuery)
{
    static const UNICODE_STRING ForbiddenShimmingApps[] = {
        RTL_CONSTANT_STRING(L"ntsd.exe"),
        RTL_CONSTANT_STRING(L"windbg.exe"),
#if WINVER >= 0x600
        RTL_CONSTANT_STRING(L"slsvc.exe"),
#endif
    };
    static const UNICODE_STRING BackSlash = RTL_CONSTANT_STRING(L"\\");
    static const UNICODE_STRING ForwdSlash = RTL_CONSTANT_STRING(L"/");
    UNICODE_STRING ProcessName;
    USHORT Back, Forward;
    HSDB hsdb;
    DWORD n;

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, ProcessImage, &BackSlash, &Back)))
        Back = 0;

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, ProcessImage, &ForwdSlash, &Forward)))
        Forward = 0;

    if (Back < Forward)
        Back = Forward;

    if (Back)
        Back += sizeof(WCHAR);

    ProcessName.Buffer = ProcessImage->Buffer + Back / sizeof(WCHAR);
    ProcessName.Length = ProcessImage->Length - Back;
    ProcessName.MaximumLength = ProcessImage->MaximumLength - Back;

    for (n = 0; n < _countof(ForbiddenShimmingApps); ++n)
    {
        if (RtlEqualUnicodeString(&ProcessName, ForbiddenShimmingApps + n, TRUE))
        {
            SHIMENG_MSG("Not shimming %wZ\n", ForbiddenShimmingApps + n);
            return FALSE;
        }
    }

    /* We should probably load all db's here, but since we do not support that yet... */
    hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
    if (hsdb)
    {
        if (SdbUnpackAppCompatData(hsdb, ProcessImage->Buffer, pShimData, pQuery))
        {
            *pHsdb = hsdb;
            return TRUE;
        }
        SdbReleaseDatabase(hsdb);
    }
    return FALSE;
}



VOID NTAPI SE_InstallBeforeInit(PUNICODE_STRING ProcessImage, PVOID pShimData)
{
    HSDB hsdb = NULL;
    SDBQUERYRESULT QueryResult = { { 0 } };
    SHIMENG_MSG("(%wZ, %p)\n", ProcessImage, pShimData);

    if (!SeiGetShimData(ProcessImage, pShimData, &hsdb, &QueryResult))
    {
        SHIMENG_FAIL("Failed to get shim data\n");
        return;
    }

    g_bShimDuringInit = TRUE;
    SeiInit(ProcessImage, hsdb, &QueryResult);
    g_bShimDuringInit = FALSE;

    SdbReleaseDatabase(hsdb);
}

VOID NTAPI SE_InstallAfterInit(PUNICODE_STRING ProcessImage, PVOID pShimData)
{
    NotifyShims(SHIM_NOTIFY_ATTACH, NULL);
}

VOID NTAPI SE_ProcessDying(VOID)
{
    SHIMENG_MSG("()\n");
    NotifyShims(SHIM_NOTIFY_DETACH, NULL);
}

VOID WINAPI SE_DllLoaded(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    SHIMENG_INFO("%sINIT. loading DLL \"%wZ\"\n", g_bShimDuringInit ? "" : "AFTER ", &LdrEntry->BaseDllName);
    NotifyShims(SHIM_REASON_DLL_LOAD, LdrEntry);
}

VOID WINAPI SE_DllUnloaded(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    SHIMENG_MSG("(%p)\n", LdrEntry);
    NotifyShims(SHIM_REASON_DLL_UNLOAD, LdrEntry);
}

BOOL WINAPI SE_IsShimDll(PVOID BaseAddress)
{
    SHIMENG_MSG("(%p)\n", BaseAddress);

    return SeiGetShimInfo(BaseAddress) != NULL;
}

