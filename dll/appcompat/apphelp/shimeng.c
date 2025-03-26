/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Shim engine core
 * COPYRIGHT:   Copyright 2015-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#define WIN32_NO_STATUS
#include "ntndk.h"
#define IN_APPHELP
#include "shimlib.h"
#include <strsafe.h>
/* Make sure we don't include apphelp logging */
#define APPHELP_NOSDBPAPI
#include "apphelp.h"
#include "shimeng.h"



FARPROC WINAPI StubGetProcAddress(HINSTANCE hModule, LPCSTR lpProcName);
BOOL WINAPI SE_IsShimDll(PVOID BaseAddress);

static const UNICODE_STRING Ntdll = RTL_CONSTANT_STRING(L"ntdll.dll");
static const UNICODE_STRING Kernel32 = RTL_CONSTANT_STRING(L"kernel32.dll");
static const UNICODE_STRING Verifier = RTL_CONSTANT_STRING(L"verifier.dll");

extern HMODULE g_hInstance;
static UNICODE_STRING g_WindowsDirectory;
static UNICODE_STRING g_System32Directory;
static UNICODE_STRING g_SxsDirectory;
static UNICODE_STRING g_LoadingShimDll;
ULONG g_ShimEngDebugLevel = 0xffffffff;
BOOL g_bComPlusImage = FALSE;
BOOL g_bShimDuringInit = FALSE;
BOOL g_bShimEngInitialized = FALSE;
BOOL g_bInternalHooksUsed = FALSE;
static ARRAY g_pShimInfo;   /* PSHIMMODULE */
static ARRAY g_pHookArray;  /* HOOKMODULEINFO */
static ARRAY g_InExclude;   /* INEXCLUDE */

typedef FARPROC(WINAPI* GETPROCADDRESSPROC)(HINSTANCE, LPCSTR);
/* If we have setup a hook for a function, we should also redirect GetProcAddress for this function */
HOOKAPIEX g_IntHookEx[] =
{
    {
        "kernel32.dll",     /* LibraryName */
        "GetProcAddress",   /* FunctionName */
        StubGetProcAddress, /* ReplacementFunction*/
        NULL,               /* OriginalFunction */
        NULL,               /* pShimInfo */
        NULL                /* ApiLink */
    },
};

static inline BOOL ARRAY_InitWorker(PARRAY Array, DWORD ItemSize)
{
    Array->Data__ = NULL;
    Array->Size__ = Array->MaxSize__ = 0;
    Array->ItemSize__ = ItemSize;

    return TRUE;
}

static inline BOOL ARRAY_EnsureSize(PARRAY Array, DWORD ItemSize, DWORD GrowWith)
{
    PVOID pNewData;
    DWORD Count;

    ASSERT(Array);
    ASSERT(ItemSize == Array->ItemSize__);

    if (Array->MaxSize__ > Array->Size__)
        return TRUE;

    Count = Array->Size__ + GrowWith;
    pNewData = SeiAlloc(Count * ItemSize);

    if (!pNewData)
    {
        SHIMENG_FAIL("Failed to allocate %d bytes\n", Count * ItemSize);
        return FALSE;
    }
    Array->MaxSize__ = Count;

    if (Array->Data__)
    {
        memcpy(pNewData, Array->Data__, Array->Size__ * ItemSize);
        SeiFree(Array->Data__);
    }
    Array->Data__ = pNewData;

    return TRUE;
}

static inline PVOID ARRAY_AppendWorker(PARRAY Array, DWORD ItemSize, DWORD GrowWith)
{
    PBYTE pData;

    if (!ARRAY_EnsureSize(Array, ItemSize, GrowWith))
        return NULL;

    pData = Array->Data__;
    pData += (Array->Size__ * ItemSize);
    Array->Size__++;

    return pData;
}

static inline PVOID ARRAY_AtWorker(PARRAY Array, DWORD ItemSize, DWORD n)
{
    PBYTE pData;

    ASSERT(Array);
    ASSERT(ItemSize == Array->ItemSize__);
    ASSERT(n < Array->Size__);

    pData = Array->Data__;
    return pData + (n * ItemSize);
}


#define ARRAY_Init(Array, TypeOfArray)      ARRAY_InitWorker((Array), sizeof(TypeOfArray))
#define ARRAY_Append(Array, TypeOfArray)    (TypeOfArray*)ARRAY_AppendWorker((Array), sizeof(TypeOfArray), 5)
#define ARRAY_At(Array, TypeOfArray, at)    (TypeOfArray*)ARRAY_AtWorker((Array), sizeof(TypeOfArray), at)
#define ARRAY_Size(Array)                   (Array)->Size__


VOID SeiInitDebugSupport(VOID)
{
    static const UNICODE_STRING DebugKey = RTL_CONSTANT_STRING(L"SHIMENG_DEBUG_LEVEL");
    UNICODE_STRING DebugValue;
    NTSTATUS Status;
    ULONG NewLevel = SEI_MSG;   /* Show some basic info in the logs, unless configured different */
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

static
BOOL SeiIsOrdinalName(LPCSTR lpProcName)
{
    return (ULONG_PTR)lpProcName <= MAXUSHORT;
}

LPCSTR SeiPrintFunctionName(LPCSTR lpProcName, char szOrdProcFmt[10])
{
    if (SeiIsOrdinalName(lpProcName))
    {
        StringCchPrintfA(szOrdProcFmt, 10, "#%Iu", (ULONG_PTR)lpProcName);
        return szOrdProcFmt;
    }
    return lpProcName;
}

int SeiCompareFunctionName(LPCSTR lpProcName1, LPCSTR lpProcName2)
{
    BOOL Ord1 = SeiIsOrdinalName(lpProcName1);
    BOOL Ord2 = SeiIsOrdinalName(lpProcName2);

    /* One is an ordinal, the other not */
    if (Ord1 != Ord2)
        return 1;

    /* Compare ordinals */
    if (Ord1)
        return (ULONG_PTR)lpProcName1 != (ULONG_PTR)lpProcName2;

    /* Compare names */
    return strcmp(lpProcName1, lpProcName2);
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
    DWORD n;

    for (n = 0; n < ARRAY_Size(&g_pShimInfo); ++n)
    {
        PSHIMMODULE pShimModule = *ARRAY_At(&g_pShimInfo, PSHIMMODULE, n);
        if (!pShimModule->pNotifyShims)
            continue;

        pShimModule->pNotifyShims(dwReason, Info);
    }
}



VOID SeiCheckComPlusImage(PVOID BaseAddress)
{
    ULONG ComSectionSize;
    g_bComPlusImage = RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &ComSectionSize) != NULL;

    SHIMENG_INFO("COM+ executable %s\n", g_bComPlusImage ? "TRUE" : "FALSE");
}


PSHIMMODULE SeiGetShimModuleInfo(PVOID BaseAddress)
{
    DWORD n;

    for (n = 0; n < ARRAY_Size(&g_pShimInfo); ++n)
    {
        PSHIMMODULE pShimModule = *ARRAY_At(&g_pShimInfo, PSHIMMODULE, n);

        if (pShimModule->BaseAddress == BaseAddress)
            return pShimModule;
    }
    return NULL;
}

PSHIMMODULE SeiCreateShimModuleInfo(PCWSTR DllName, PVOID BaseAddress)
{
    static const ANSI_STRING GetHookAPIs = RTL_CONSTANT_STRING("GetHookAPIs");
    static const ANSI_STRING NotifyShims = RTL_CONSTANT_STRING("NotifyShims");
    PSHIMMODULE* pData, Data;
    PVOID pGetHookAPIs, pNotifyShims;

    if (!NT_SUCCESS(LdrGetProcedureAddress(BaseAddress, (PANSI_STRING)&GetHookAPIs, 0, &pGetHookAPIs)) ||
        !NT_SUCCESS(LdrGetProcedureAddress(BaseAddress, (PANSI_STRING)&NotifyShims, 0, &pNotifyShims)))
    {
        SHIMENG_WARN("Failed to resolve entry points for %S\n", DllName);
        return NULL;
    }

    pData = ARRAY_Append(&g_pShimInfo, PSHIMMODULE);
    if (!pData)
        return NULL;

    *pData = SeiAlloc(sizeof(SHIMMODULE));

    Data = *pData;

    RtlCreateUnicodeString(&Data->Name, DllName);
    Data->BaseAddress = BaseAddress;

    Data->pGetHookAPIs = pGetHookAPIs;
    Data->pNotifyShims = pNotifyShims;

    ARRAY_Init(&Data->EnabledShims, PSHIMINFO);

    return Data;
}

PSHIMINFO SeiAppendHookInfo(PSHIMMODULE pShimModuleInfo, PHOOKAPIEX pHookApi, DWORD dwHookCount, PCWSTR ShimName)
{
    PSHIMINFO* pData, Data;

    pData = ARRAY_Append(&pShimModuleInfo->EnabledShims, PSHIMINFO);
    if (!pData)
        return NULL;

    *pData = SeiAlloc(sizeof(SHIMINFO));
    Data = *pData;

    if (!Data)
        return NULL;

    Data->ShimName = SdbpStrDup(ShimName);
    if (!Data->ShimName)
        return NULL;

    Data->pHookApi = pHookApi;
    Data->dwHookCount = dwHookCount;
    Data->pShimModule = pShimModuleInfo;
    ARRAY_Init(&Data->InExclude, INEXCLUDE);
    return Data;
}

PHOOKMODULEINFO SeiFindHookModuleInfo(PUNICODE_STRING ModuleName, PVOID BaseAddress)
{
    DWORD n;

    if (ModuleName == NULL && BaseAddress == NULL)
    {
        BaseAddress = NtCurrentPeb()->ImageBaseAddress;
    }

    for (n = 0; n < ARRAY_Size(&g_pHookArray); ++n)
    {
        PHOOKMODULEINFO pModuleInfo = ARRAY_At(&g_pHookArray, HOOKMODULEINFO, n);

        if (BaseAddress && BaseAddress == pModuleInfo->BaseAddress)
            return pModuleInfo;

        if (!BaseAddress && RtlEqualUnicodeString(ModuleName, &pModuleInfo->Name, TRUE))
            return pModuleInfo;
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

    if (!NT_SUCCESS(Success))
    {
        SHIMENG_FAIL("Unable to get module handle for %wZ (%p)\n", &DllName, DllBase);
        RtlFreeUnicodeString(&DllName);

        return NULL;
    }
    RtlFreeUnicodeString(&DllName);

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

    return SdbReadDWORDTag(pdb, tagEntry, 0);
}

static QWORD SeiGetQWORD(PDB pdb, TAGID tag, TAG type)
{
    TAGID tagEntry = SdbFindFirstTag(pdb, tag, type);
    if (tagEntry == TAGID_NULL)
        return 0;

    return SdbReadQWORDTag(pdb, tagEntry, 0);
}

static VOID SeiAddShim(TAGREF trShimRef, PARRAY pShimRef)
{
    TAGREF* Data;

    Data = ARRAY_Append(pShimRef, TAGREF);
    if (!Data)
        return;

    *Data = trShimRef;
}

static VOID SeiAddFlag(PDB pdb, TAGID tiFlagRef, PFLAGINFO pFlagInfo)
{
    ULARGE_INTEGER Flag;

    /* Resolve the FLAG_REF to the real FLAG node */
    TAGID FlagTag = SeiGetDWORD(pdb, tiFlagRef, TAG_FLAG_TAGID);

    if (FlagTag == TAGID_NULL)
        return;

    pFlagInfo->AppCompatFlags.QuadPart |= SeiGetQWORD(pdb, FlagTag, TAG_FLAG_MASK_KERNEL);
    pFlagInfo->AppCompatFlagsUser.QuadPart |= SeiGetQWORD(pdb, FlagTag, TAG_FLAG_MASK_USER);
    Flag.QuadPart = SeiGetQWORD(pdb, FlagTag, TAG_FLAG_PROCESSPARAM);
    pFlagInfo->ProcessParameters_Flags |= Flag.LowPart;
}

/* Propagate layers to child processes */
static VOID SeiSetLayerEnvVar(LPCWSTR wszLayer)
{
    NTSTATUS Status;
    UNICODE_STRING VarName = RTL_CONSTANT_STRING(L"__COMPAT_LAYER");
    UNICODE_STRING Value;

    RtlInitUnicodeString(&Value, wszLayer);

    Status = RtlSetEnvironmentVariable(NULL, &VarName, &Value);
    if (NT_SUCCESS(Status))
        SHIMENG_INFO("%wZ=%wZ\n", &VarName, &Value);
    else
        SHIMENG_FAIL("Failed to set %wZ: 0x%x\n", &VarName, Status);
}

#define MAX_LAYER_LENGTH            256

/* Translate all Exe and Layer entries to Shims, and propagate all layers */
static VOID SeiBuildShimRefArray(HSDB hsdb, SDBQUERYRESULT* pQuery, PARRAY pShimRef, PFLAGINFO pFlagInfo)
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
            TAGID FlagRef = SdbFindFirstTag(pdb, tag, TAG_FLAG_REF);

            if (ExeName)
                SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Exe(%S))\n", ExeName);

            while (ShimRef != TAGID_NULL)
            {
                TAGREF trShimRef;
                if (SdbTagIDToTagRef(hsdb, pdb, ShimRef, &trShimRef))
                    SeiAddShim(trShimRef, pShimRef);

                ShimRef = SdbFindNextTag(pdb, tag, ShimRef);
            }

            while (FlagRef != TAGID_NULL)
            {
                SeiAddFlag(pdb, FlagRef, pFlagInfo);

                FlagRef = SdbFindNextTag(pdb, tag, FlagRef);
            }
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
            TAGID FlagRef = SdbFindFirstTag(pdb, tag, TAG_FLAG_REF);

            if (LayerName)
            {
                HRESULT hr;
                SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Layer(%S))\n", LayerName);
                if (wszLayerEnvVar[0])
                    StringCchCatW(wszLayerEnvVar, ARRAYSIZE(wszLayerEnvVar), L" ");
                hr = StringCchCatW(wszLayerEnvVar, ARRAYSIZE(wszLayerEnvVar), LayerName);
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

            while (FlagRef != TAGID_NULL)
            {
                SeiAddFlag(pdb, FlagRef, pFlagInfo);

                FlagRef = SdbFindNextTag(pdb, tag, FlagRef);
            }
        }
    }
    if (wszLayerEnvVar[0])
        SeiSetLayerEnvVar(wszLayerEnvVar);
}

/* Given the hooks from one shim, find the relevant modules and store the combination of module + hook */
VOID SeiAddHooks(PHOOKAPIEX hooks, DWORD dwHookCount, PSHIMINFO pShim)
{
    DWORD n, j;
    UNICODE_STRING UnicodeModName;
    WCHAR Buf[512];

    RtlInitEmptyUnicodeString(&UnicodeModName, Buf, sizeof(Buf));

    for (n = 0; n < dwHookCount; ++n)
    {
        ANSI_STRING AnsiString;
        PVOID DllHandle;
        PHOOKAPIEX hook = hooks + n;
        PHOOKAPIEX* pHookApi;
        PHOOKMODULEINFO HookModuleInfo;

        RtlInitAnsiString(&AnsiString, hook->LibraryName);
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeModName, &AnsiString, FALSE)))
        {
            SHIMENG_FAIL("Unable to convert %s to Unicode\n", hook->LibraryName);
            continue;
        }

        if (NT_SUCCESS(LdrGetDllHandle(NULL, 0, &UnicodeModName, &DllHandle)))
        {
            HookModuleInfo = SeiFindHookModuleInfo(NULL, DllHandle);
        }
        else
        {
            HookModuleInfo = SeiFindHookModuleInfo(&UnicodeModName, NULL);
            DllHandle = NULL;
        }

        if (!HookModuleInfo)
        {
            HookModuleInfo = ARRAY_Append(&g_pHookArray, HOOKMODULEINFO);
            if (!HookModuleInfo)
                continue;

            HookModuleInfo->BaseAddress = DllHandle;
            ARRAY_Init(&HookModuleInfo->HookApis, PHOOKAPIEX);
            RtlCreateUnicodeString(&HookModuleInfo->Name, UnicodeModName.Buffer);
        }

        hook->pShimInfo = pShim;

        for (j = 0; j < ARRAY_Size(&HookModuleInfo->HookApis); ++j)
        {
            PHOOKAPIEX HookApi = *ARRAY_At(&HookModuleInfo->HookApis, PHOOKAPIEX, j);
            int CmpResult = SeiCompareFunctionName(hook->FunctionName, HookApi->FunctionName);
            if (CmpResult == 0)
            {
                while (HookApi->ApiLink)
                {
                    HookApi = HookApi->ApiLink;
                }
                HookApi->ApiLink = hook;
                hook = NULL;
                break;
            }
        }
        /* No place found yet, append it */
        if (hook)
        {
            pHookApi = ARRAY_Append(&HookModuleInfo->HookApis, PHOOKAPIEX);
            if (pHookApi)
                *pHookApi = hook;
        }
    }
}

/* Check if we should fake the return from GetProcAddress (because we also redirected the iat for this module) */
FARPROC WINAPI StubGetProcAddress(HINSTANCE hModule, LPCSTR lpProcName)
{
    PVOID Addr = _ReturnAddress();
    PHOOKMODULEINFO HookModuleInfo;
    FARPROC proc = ((GETPROCADDRESSPROC)g_IntHookEx[0].OriginalFunction)(hModule, lpProcName);
    char szOrdProcFmt[10];

    Addr = SeiGetModuleFromAddress(Addr);
    if (SE_IsShimDll(Addr))
    {
        SHIMENG_MSG("Not touching GetProcAddress for shim dll (%p!%s)", hModule, SeiPrintFunctionName(lpProcName, szOrdProcFmt));
        return proc;
    }

    SHIMENG_INFO("(GetProcAddress(%p!%s) => %p\n", hModule, SeiPrintFunctionName(lpProcName, szOrdProcFmt), proc);

    HookModuleInfo = SeiFindHookModuleInfo(NULL, hModule);

    if (HookModuleInfo)
    {
        DWORD n;
        for (n = 0; n < ARRAY_Size(&HookModuleInfo->HookApis); ++n)
        {
            PHOOKAPIEX HookApi = *ARRAY_At(&HookModuleInfo->HookApis, PHOOKAPIEX, n);
            int CmpResult = SeiCompareFunctionName(lpProcName, HookApi->FunctionName);
            if (CmpResult == 0)
            {
                SHIMENG_MSG("Redirecting %p to %p\n", proc, HookApi->ReplacementFunction);
                proc = HookApi->ReplacementFunction;
                break;
            }
        }
    }

    return proc;
}

VOID SeiResolveAPI(PHOOKMODULEINFO HookModuleInfo)
{
    DWORD n;
    ANSI_STRING AnsiString;

    ASSERT(HookModuleInfo->BaseAddress != NULL);

    for (n = 0; n < ARRAY_Size(&HookModuleInfo->HookApis); ++n)
    {
        NTSTATUS Status;
        PVOID ProcAddress;
        PHOOKAPIEX HookApi = *ARRAY_At(&HookModuleInfo->HookApis, PHOOKAPIEX, n);

        if (!SeiIsOrdinalName(HookApi->FunctionName))
        {
            RtlInitAnsiString(&AnsiString, HookApi->FunctionName);
            Status = LdrGetProcedureAddress(HookModuleInfo->BaseAddress, &AnsiString, 0, &ProcAddress);
        }
        else
        {
            Status = LdrGetProcedureAddress(HookModuleInfo->BaseAddress, NULL, (ULONG_PTR)HookApi->FunctionName, &ProcAddress);
        }

        if (!NT_SUCCESS(Status))
        {
            char szOrdProcFmt[10];
            LPCSTR lpFunctionName = SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt);
            SHIMENG_FAIL("Unable to retrieve %s!%s\n", HookApi->LibraryName, lpFunctionName);
            continue;
        }

        HookApi->OriginalFunction = ProcAddress;
        if (HookApi->ApiLink)
        {
            SHIMENG_MSG("TODO: Figure out how to handle conflicting In/Exports with ApiLink!\n");
        }
        while (HookApi->ApiLink)
        {
            HookApi->ApiLink->OriginalFunction = HookApi->OriginalFunction;
            HookApi->OriginalFunction = HookApi->ApiLink->ReplacementFunction;
            HookApi = HookApi->ApiLink;
        }
    }
}

/* Walk all shim modules / enabled shims, and add their hooks */
VOID SeiResolveAPIs(VOID)
{
    DWORD n;

    for (n = 0; n < ARRAY_Size(&g_pHookArray); ++n)
    {
        PHOOKMODULEINFO pModuleInfo = ARRAY_At(&g_pHookArray, HOOKMODULEINFO, n);

        /* Is this module loaded? */
        if (pModuleInfo->BaseAddress)
        {
            SeiResolveAPI(pModuleInfo);
        }
    }
}

VOID SeiCombineHookInfo(VOID)
{
    DWORD mod, n;

    /* Enumerate all Shim modules */
    for (mod = 0; mod < ARRAY_Size(&g_pShimInfo); ++mod)
    {
        PSHIMMODULE pShimModule = *ARRAY_At(&g_pShimInfo, PSHIMMODULE, mod);
        DWORD dwShimCount = ARRAY_Size(&pShimModule->EnabledShims);

        /* Enumerate all Shims */
        for (n = 0; n < dwShimCount; ++n)
        {
            PSHIMINFO pShim = *ARRAY_At(&pShimModule->EnabledShims, PSHIMINFO, n);

            PHOOKAPIEX hooks = pShim->pHookApi;
            DWORD dwHookCount = pShim->dwHookCount;

            SeiAddHooks(hooks, dwHookCount, pShim);
        }
    }
}

/* If we hooked something, we should also redirect GetProcAddress */
VOID SeiAddInternalHooks(DWORD dwNumHooks)
{
    if (dwNumHooks == 0)
    {
        g_bInternalHooksUsed = FALSE;
        return;
    }

    SeiAddHooks(g_IntHookEx, ARRAYSIZE(g_IntHookEx), NULL);
    g_bInternalHooksUsed = TRUE;
}

/* Patch one function in the iat */
VOID SeiPatchNewImport(PIMAGE_THUNK_DATA FirstThunk, PHOOKAPIEX HookApi, PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG OldProtection = 0;
    PVOID Ptr;
    SIZE_T Size;
    NTSTATUS Status;
    char szOrdProcFmt[10];

    SHIMENG_INFO("Hooking API \"%s!%s\" for DLL \"%wZ\"\n", HookApi->LibraryName, SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt), &LdrEntry->BaseDllName);

    Ptr = &FirstThunk->u1.Function;
    Size = sizeof(FirstThunk->u1.Function);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Ptr, &Size, PAGE_EXECUTE_READWRITE, &OldProtection);

    if (!NT_SUCCESS(Status))
    {
        SHIMENG_FAIL("Unable to unprotect 0x%p\n", &FirstThunk->u1.Function);
        return;
    }

    SHIMENG_INFO("changing 0x%p to 0x%p\n", FirstThunk->u1.Function, HookApi->ReplacementFunction);
    FirstThunk->u1.Function = (ULONG_PTR)HookApi->ReplacementFunction;

    Size = sizeof(FirstThunk->u1.Function);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Ptr, &Size, OldProtection, &OldProtection);

    if (!NT_SUCCESS(Status))
    {
        SHIMENG_WARN("Unable to reprotect 0x%p\n", &FirstThunk->u1.Function);
    }
}


PINEXCLUDE SeiFindInExclude(PARRAY InExclude, PCUNICODE_STRING DllName)
{
    DWORD n;

    for (n = 0; n < ARRAY_Size(InExclude); ++n)
    {
        PINEXCLUDE InEx = ARRAY_At(InExclude, INEXCLUDE, n);

        if (RtlEqualUnicodeString(&InEx->Module, DllName, TRUE))
            return InEx;
    }

    return NULL;
}

BOOL SeiIsExcluded(PLDR_DATA_TABLE_ENTRY LdrEntry, PHOOKAPIEX HookApi)
{
    PSHIMINFO pShimInfo = HookApi->pShimInfo;
    PINEXCLUDE InExclude;
    BOOL IsExcluded = FALSE;
    char szOrdProcFmt[10];

    if (!pShimInfo)
    {
        /* Internal hook, do not exclude it */
        return FALSE;
    }

    /* By default, everything from System32 or WinSxs is excluded */
    if (RtlPrefixUnicodeString(&g_System32Directory, &LdrEntry->FullDllName, TRUE) ||
        RtlPrefixUnicodeString(&g_SxsDirectory, &LdrEntry->FullDllName, TRUE))
        IsExcluded = TRUE;

    InExclude = SeiFindInExclude(&pShimInfo->InExclude, &LdrEntry->BaseDllName);
    if (InExclude)
    {
        /* If it is on the 'exclude' list, bail out */
        if (!InExclude->Include)
        {
            SHIMENG_INFO("Module '%wZ' excluded for shim %S, API '%s!%s', because it on in the exclude list.\n",
                         &LdrEntry->BaseDllName, pShimInfo->ShimName, HookApi->LibraryName, SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt));

            return TRUE;
        }
        /* If it is on the 'include' list, override System32 / Winsxs check. */
        if (IsExcluded)
        {
            SHIMENG_INFO("Module '%wZ' included for shim %S, API '%s!%s', because it is on the include list.\n",
                         &LdrEntry->BaseDllName, pShimInfo->ShimName, HookApi->LibraryName, SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt));

        }
        IsExcluded = FALSE;
    }

    if (IsExcluded)
    {
        SHIMENG_INFO("Module '%wZ' excluded for shim %S, API '%s!%s', because it is in System32/WinSXS.\n",
                     &LdrEntry->BaseDllName, pShimInfo->ShimName, HookApi->LibraryName, SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt));
    }

    return IsExcluded;
}

VOID SeiAppendInExclude(PARRAY dest, PCWSTR ModuleName, BOOL IsInclude)
{
    PINEXCLUDE InExclude;
    UNICODE_STRING ModuleNameU;
    RtlInitUnicodeString(&ModuleNameU, ModuleName);

    InExclude = SeiFindInExclude(dest, &ModuleNameU);
    if (InExclude)
    {
        InExclude->Include = IsInclude;
        return;
    }

    InExclude = ARRAY_Append(dest, INEXCLUDE);
    if (InExclude)
    {
        PCWSTR ModuleNameCopy = SdbpStrDup(ModuleName);
        RtlInitUnicodeString(&InExclude->Module, ModuleNameCopy);
        InExclude->Include = IsInclude;
    }
}

/* Read the INEXCLUD tags from a given parent tag
FIXME:
    Some observed tags:
        '*' with include
        '$' with include, followed by '*' without include
    Include list logging, referring to: (MODE: EA)
*/
VOID SeiReadInExclude(PDB pdb, TAGID parent, PARRAY dest)
{
    TAGID InExcludeTag;

    InExcludeTag = SdbFindFirstTag(pdb, parent, TAG_INEXCLUD);

    while (InExcludeTag != TAGID_NULL)
    {
        PCWSTR ModuleName;
        TAGID ModuleTag = SdbFindFirstTag(pdb, InExcludeTag, TAG_MODULE);
        TAGID IncludeTag = SdbFindFirstTag(pdb, InExcludeTag, TAG_INCLUDE);

        ModuleName = SdbGetStringTagPtr(pdb, ModuleTag);
        if (ModuleName)
        {
            SeiAppendInExclude(dest, ModuleName, IncludeTag != TAGID_NULL);
        }
        else
        {
            SHIMENG_WARN("INEXCLUDE without Module: 0x%x\n", InExcludeTag);
        }

        InExcludeTag = SdbFindNextTag(pdb, parent, InExcludeTag);
    }
}

VOID SeiBuildGlobalInclExclList(HSDB hsdb)
{
    PDB pdb;
    TAGREF tr = TAGREF_ROOT;
    TAGID root, db, library;

    if (!SdbTagRefToTagID(hsdb, tr, &pdb, &root))
    {
        SHIMENG_WARN("Unable to resolve database root\n");
        return;
    }
    db = SdbFindFirstTag(pdb, root, TAG_DATABASE);
    if (db == TAGID_NULL)
    {
        SHIMENG_WARN("Unable to resolve database\n");
        return;
    }
    library = SdbFindFirstTag(pdb, db, TAG_LIBRARY);
    if (library == TAGID_NULL)
    {
        SHIMENG_WARN("Unable to resolve library\n");
        return;
    }

    SeiReadInExclude(pdb, library, &g_InExclude);
}

VOID SeiBuildInclExclList(PDB pdb, TAGID ShimTag, PSHIMINFO pShimInfo)
{
    DWORD n;

    /* First duplicate the global in/excludes */
    for (n = 0; n < ARRAY_Size(&g_InExclude); ++n)
    {
        PINEXCLUDE InEx = ARRAY_At(&g_InExclude, INEXCLUDE, n);
        SeiAppendInExclude(&pShimInfo->InExclude, InEx->Module.Buffer, InEx->Include);
    }

    /* Now read this shim's in/excludes (possibly overriding the global ones) */
    SeiReadInExclude(pdb, ShimTag, &pShimInfo->InExclude);
}

/* Given one loaded module, redirect (hook) all functions from the iat that are registered by shims */
VOID SeiHookImports(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG Size;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PBYTE DllBase = LdrEntry->DllBase;

    if (SE_IsShimDll(DllBase) ||
        g_hInstance == LdrEntry->DllBase ||
        RtlEqualUnicodeString(&g_LoadingShimDll, &LdrEntry->BaseDllName, TRUE))
    {
        SHIMENG_INFO("Skipping shim module 0x%p \"%wZ\"\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        return;
    }

    if (LdrEntry->Flags & LDRP_COMPAT_DATABASE_PROCESSED)
    {
        SHIMENG_INFO("Skipping module 0x%p \"%wZ\" because it was already processed\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
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
            DWORD n;

            for (n = 0; n < ARRAY_Size(&HookModuleInfo->HookApis); ++n)
            {
                DWORD dwFound = 0;
                PHOOKAPIEX HookApi = *ARRAY_At(&HookModuleInfo->HookApis, PHOOKAPIEX, n);

                /* Check if this module should be excluded from being hooked (system32/winsxs, global or shim exclude) */
                if (SeiIsExcluded(LdrEntry, HookApi))
                {
                    continue;
                }

                OriginalThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->OriginalFirstThunk);
                FirstThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->FirstThunk);

                /* Walk all imports */
                for (;OriginalThunk->u1.AddressOfData && FirstThunk->u1.Function; OriginalThunk++, FirstThunk++)
                {
                    if (!IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Function))
                    {
                        if (!SeiIsOrdinalName(HookApi->FunctionName))
                        {
                            PIMAGE_IMPORT_BY_NAME ImportName;

                            ImportName = (PIMAGE_IMPORT_BY_NAME)(DllBase + OriginalThunk->u1.Function);
                            if (!strcmp((PCSTR)ImportName->Name, HookApi->FunctionName))
                            {
                                SeiPatchNewImport(FirstThunk, HookApi, LdrEntry);

                                /* Sadly, iat does not have to be sorted, and can even contain duplicate entries. */
                                dwFound++;
                            }
                        }
                    }
                    else
                    {
                        if (SeiIsOrdinalName(HookApi->FunctionName))
                        {
                            if ((PCSTR)IMAGE_ORDINAL(OriginalThunk->u1.Function) == HookApi->FunctionName)
                            {
                                SeiPatchNewImport(FirstThunk, HookApi, LdrEntry);
                                dwFound++;
                            }
                        }
                    }
                }

                if (dwFound != 1)
                {
                    char szOrdProcFmt[10];
                    LPCSTR FuncName = SeiPrintFunctionName(HookApi->FunctionName, szOrdProcFmt);

                    /* One entry not found. */
                    if (!dwFound)
                        SHIMENG_INFO("Entry \"%s!%s\" not found for \"%wZ\"\n", HookApi->LibraryName, FuncName, &LdrEntry->BaseDllName);
                    else
                        SHIMENG_INFO("Entry \"%s!%s\" found %d times for \"%wZ\"\n", HookApi->LibraryName, FuncName, dwFound, &LdrEntry->BaseDllName);
                }
            }
        }
    }

    /* Mark this module as processed. */
    LdrEntry->Flags |= LDRP_COMPAT_DATABASE_PROCESSED;
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


VOID SeiInitPaths(VOID)
{
#define SYSTEM32  L"\\system32"
#define WINSXS  L"\\winsxs"

    PWSTR WindowsDirectory = SdbpStrDup(SharedUserData->NtSystemRoot);
    RtlInitUnicodeString(&g_WindowsDirectory, WindowsDirectory);

    g_System32Directory.MaximumLength = g_WindowsDirectory.Length + SdbpStrsize(SYSTEM32);
    g_System32Directory.Buffer = SdbpAlloc(g_System32Directory.MaximumLength);
    RtlCopyUnicodeString(&g_System32Directory, &g_WindowsDirectory);
    RtlAppendUnicodeToString(&g_System32Directory, SYSTEM32);

    g_SxsDirectory.MaximumLength = g_WindowsDirectory.Length + SdbpStrsize(WINSXS);
    g_SxsDirectory.Buffer = SdbpAlloc(g_SxsDirectory.MaximumLength);
    RtlCopyUnicodeString(&g_SxsDirectory, &g_WindowsDirectory);
    RtlAppendUnicodeToString(&g_SxsDirectory, WINSXS);

#undef SYSTEM32
#undef WINSXS
}

VOID SeiSetEntryProcessed(PPEB Peb)
{
    PLIST_ENTRY ListHead, Entry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    ListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Entry = ListHead->Flink;
    while (Entry != ListHead)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        Entry = Entry->Flink;

        if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Ntdll, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Kernel32, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Verifier, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &g_LoadingShimDll, TRUE) ||
            SE_IsShimDll(LdrEntry->DllBase) ||
            (LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
        {
            SHIMENG_WARN("Don't mess with 0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        }
        else
        {
            SHIMENG_WARN("Touching        0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
            LdrEntry->Flags |= (LDRP_ENTRY_PROCESSED | LDRP_SHIMENG_SUPPRESSED_ENTRY);
        }
    }

    ListHead = &NtCurrentPeb()->Ldr->InMemoryOrderModuleList;
    Entry = ListHead->Flink;
    SHIMENG_INFO("In memory:\n");
    while (Entry != ListHead)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        Entry = Entry->Flink;

        SHIMENG_INFO("    0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
    }

    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ListHead->Flink;
    SHIMENG_INFO("In load:\n");
    while (Entry != ListHead)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        Entry = Entry->Flink;

        SHIMENG_INFO("    0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
    }
}

VOID SeiResetEntryProcessed(PPEB Peb)
{
    PLIST_ENTRY ListHead, Entry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    ListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Entry = ListHead->Flink;
    while (Entry != ListHead)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        Entry = Entry->Flink;

        if (SE_IsShimDll(LdrEntry->DllBase) ||
            g_hInstance == LdrEntry->DllBase ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Ntdll, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Kernel32, TRUE) ||
            RtlEqualUnicodeString(&LdrEntry->BaseDllName, &Verifier, TRUE) ||
            !(LdrEntry->Flags & LDRP_SHIMENG_SUPPRESSED_ENTRY))
        {
            SHIMENG_WARN("Don't mess with 0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        }
        else
        {
            SHIMENG_WARN("Resetting       0x%p '%wZ'\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
            LdrEntry->Flags &= ~(LDRP_ENTRY_PROCESSED | LDRP_SHIMENG_SUPPRESSED_ENTRY);
        }
    }
}

VOID SeiInit(LPCWSTR ProcessImage, HSDB hsdb, SDBQUERYRESULT* pQuery, BOOLEAN ProcessInit)
{
    DWORD n;
    ARRAY ShimRefArray;
    DWORD dwTotalHooks = 0;
    FLAGINFO ShimFlags;

    PPEB Peb = NtCurrentPeb();

    /* We should only be called once! */
    ASSERT(g_pShimInfo.ItemSize__ == 0);

    ARRAY_Init(&ShimRefArray, TAGREF);
    ARRAY_Init(&g_pShimInfo, PSHIMMODULE);
    ARRAY_Init(&g_pHookArray, HOOKMODULEINFO);
    ARRAY_Init(&g_InExclude, INEXCLUDE);
    RtlZeroMemory(&ShimFlags, sizeof(ShimFlags));

    SeiInitPaths();

    SeiCheckComPlusImage(Peb->ImageBaseAddress);

    if (ProcessInit)
    {
        /* Mark all modules loaded until now as 'LDRP_ENTRY_PROCESSED' so that their entrypoint is not called while we are loading shims */
        SeiSetEntryProcessed(Peb);
    }

    /* TODO:
    if (pQuery->trApphelp)
        SeiDisplayAppHelp(?pQuery->trApphelp?);
    */

    SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(ExePath(%S))\n", ProcessImage);
    SeiBuildShimRefArray(hsdb, pQuery, &ShimRefArray, &ShimFlags);
    if (ShimFlags.AppCompatFlags.QuadPart)
    {
        SeiDbgPrint(SEI_MSG, NULL, "Using KERNEL apphack flags 0x%I64x\n", ShimFlags.AppCompatFlags.QuadPart);
        Peb->AppCompatFlags.QuadPart |= ShimFlags.AppCompatFlags.QuadPart;
    }
    if (ShimFlags.AppCompatFlagsUser.QuadPart)
    {
        SeiDbgPrint(SEI_MSG, NULL, "Using USER apphack flags 0x%I64x\n", ShimFlags.AppCompatFlagsUser.QuadPart);
        Peb->AppCompatFlagsUser.QuadPart |= ShimFlags.AppCompatFlagsUser.QuadPart;
    }
    if (ShimFlags.ProcessParameters_Flags)
    {
        SeiDbgPrint(SEI_MSG, NULL, "Using ProcessParameters flags 0x%x\n", ShimFlags.ProcessParameters_Flags);
        Peb->ProcessParameters->Flags |= ShimFlags.ProcessParameters_Flags;
    }
    SeiDbgPrint(SEI_MSG, NULL, "ShimInfo(Complete)\n");

    SHIMENG_INFO("Got %d shims\n", ARRAY_Size(&ShimRefArray));
    SeiBuildGlobalInclExclList(hsdb);

    /* Walk all shims referenced (in layers + exes), and load their modules */
    for (n = 0; n < ARRAY_Size(&ShimRefArray); ++n)
    {
        PDB pdb;
        TAGID ShimRef;

        TAGREF tr = *ARRAY_At(&ShimRefArray, TAGREF, n);

        if (SdbTagRefToTagID(hsdb, tr, &pdb, &ShimRef))
        {
            LPCWSTR ShimName, DllName, CommandLine = NULL;
            TAGID ShimTag;
            WCHAR FullNameBuffer[MAX_PATH];
            UNICODE_STRING UnicodeDllName;
            PVOID BaseAddress;
            PSHIMMODULE pShimModuleInfo = NULL;
            ANSI_STRING AnsiCommandLine = RTL_CONSTANT_STRING("");
            PSHIMINFO pShimInfo = NULL;
            PHOOKAPIEX pHookApi;
            DWORD dwHookCount;

            ShimName = SeiGetStringPtr(pdb, ShimRef, TAG_NAME);
            if (!ShimName)
            {
                SHIMENG_FAIL("Failed to retrieve the name for 0x%x\n", tr);
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

            if (!SUCCEEDED(SdbGetAppPatchDir(NULL, FullNameBuffer, ARRAYSIZE(FullNameBuffer))))
            {
                SHIMENG_WARN("Failed to get the AppPatch dir\n");
                continue;
            }

            DllName = SeiGetStringPtr(pdb, ShimTag, TAG_DLLFILE);
            if (DllName == NULL ||
                !SUCCEEDED(StringCchCatW(FullNameBuffer, ARRAYSIZE(FullNameBuffer), L"\\")) ||
                !SUCCEEDED(StringCchCatW(FullNameBuffer, ARRAYSIZE(FullNameBuffer), DllName)))
            {
                SHIMENG_WARN("Failed to build a full path for %S\n", ShimName);
                continue;
            }

            RtlInitUnicodeString(&g_LoadingShimDll, DllName);
            RtlInitUnicodeString(&UnicodeDllName, FullNameBuffer);
            if (NT_SUCCESS(LdrGetDllHandle(NULL, NULL, &UnicodeDllName, &BaseAddress)))
            {
                /* This shim dll was already loaded, let's find it */
                pShimModuleInfo = SeiGetShimModuleInfo(BaseAddress);
            }
            else if (!NT_SUCCESS(LdrLoadDll(NULL, NULL, &UnicodeDllName, &BaseAddress)))
            {
                SHIMENG_WARN("Failed to load %wZ for %S\n", &UnicodeDllName, ShimName);
                continue;
            }
            RtlInitUnicodeString(&g_LoadingShimDll, NULL);
            /* No shim module found (or we just loaded it) */
            if (!pShimModuleInfo)
            {
                pShimModuleInfo = SeiCreateShimModuleInfo(DllName, BaseAddress);
                if (!pShimModuleInfo)
                {
                    SHIMENG_FAIL("Failed to allocate ShimInfo for %S\n", DllName);
                    continue;
                }
            }

            SHIMENG_INFO("Shim DLL 0x%p \"%wZ\" loaded\n", BaseAddress, &UnicodeDllName);
            SHIMENG_INFO("Using SHIM \"%S!%S\"\n", DllName, ShimName);

            /* Ask this shim what hooks it needs (and pass along the commandline) */
            dwHookCount = 0;
            pHookApi = pShimModuleInfo->pGetHookAPIs(AnsiCommandLine.Buffer, ShimName, &dwHookCount);
            SHIMENG_INFO("GetHookAPIs returns %d hooks for DLL \"%wZ\" SHIM \"%S\"\n", dwHookCount, &UnicodeDllName, ShimName);
            if (dwHookCount && pHookApi)
                pShimInfo = SeiAppendHookInfo(pShimModuleInfo, pHookApi, dwHookCount, ShimName);
            else
                dwHookCount = 0;

            /* If this shim has hooks, create the include / exclude lists */
            if (pShimInfo)
                SeiBuildInclExclList(pdb, ShimTag, pShimInfo);

            if (CommandLine && *CommandLine)
                RtlFreeAnsiString(&AnsiCommandLine);

            dwTotalHooks += dwHookCount;
        }
    }

    SeiAddInternalHooks(dwTotalHooks);
    SeiCombineHookInfo();
    SeiResolveAPIs();
    PatchNewModules(Peb);

    if (ProcessInit)
    {
        /* Remove the 'LDRP_ENTRY_PROCESSED' flag from entries we modified, so that the loader can continue to process them */
        SeiResetEntryProcessed(Peb);
    }
    g_bShimEngInitialized = TRUE;
}


/* Load the database + unpack the shim data (if this process is allowed) */
BOOL SeiGetShimData(PUNICODE_STRING ProcessImage, PVOID pShimData, HSDB* pHsdb, SDBQUERYRESULT* pQuery)
{
    static const UNICODE_STRING ForbiddenShimmingApps[] = {
        RTL_CONSTANT_STRING(L"ntsd.exe"),
        RTL_CONSTANT_STRING(L"windbg.exe"),
#if WINVER >= 0x600
        RTL_CONSTANT_STRING(L"slsvc.exe"),
#endif
    };
    static const UNICODE_STRING PathDividerFind = RTL_CONSTANT_STRING(L"\\/");
    UNICODE_STRING ProcessName;
    USHORT PathDivider;
    HSDB hsdb;
    DWORD n;

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END, ProcessImage, &PathDividerFind, &PathDivider)))
        PathDivider = 0;

    if (PathDivider)
        PathDivider += sizeof(WCHAR);

    ProcessName.Buffer = ProcessImage->Buffer + PathDivider / sizeof(WCHAR);
    ProcessName.Length = ProcessImage->Length - PathDivider;
    ProcessName.MaximumLength = ProcessImage->MaximumLength - PathDivider;

    for (n = 0; n < ARRAYSIZE(ForbiddenShimmingApps); ++n)
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
    SHIMENG_INFO("(%wZ, %p)\n", ProcessImage, pShimData);

    if (!SeiGetShimData(ProcessImage, pShimData, &hsdb, &QueryResult))
    {
        SHIMENG_FAIL("Failed to get shim data\n");
        return;
    }

    g_bShimDuringInit = TRUE;
    SeiInit(ProcessImage->Buffer, hsdb, &QueryResult, TRUE);
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
    PHOOKMODULEINFO HookModuleInfo;
    SHIMENG_INFO("%sINIT. loading DLL \"%wZ\"\n", g_bShimDuringInit ? "" : "AFTER ", &LdrEntry->BaseDllName);

    HookModuleInfo = SeiFindHookModuleInfo(&LdrEntry->BaseDllName, NULL);
    if (HookModuleInfo)
    {
        ASSERT(HookModuleInfo->BaseAddress == NULL);
        HookModuleInfo->BaseAddress = LdrEntry->DllBase;
        SeiResolveAPI(HookModuleInfo);
    }

    SeiHookImports(LdrEntry);

    NotifyShims(SHIM_REASON_DLL_LOAD, LdrEntry);
}

VOID WINAPI SE_DllUnloaded(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    SHIMENG_INFO("(%p)\n", LdrEntry);

    /* Should we unhook here? */

    NotifyShims(SHIM_REASON_DLL_UNLOAD, LdrEntry);
}

BOOL WINAPI SE_IsShimDll(PVOID BaseAddress)
{
    SHIMENG_INFO("(%p)\n", BaseAddress);

    return SeiGetShimModuleInfo(BaseAddress) != NULL;
}

/* 'Private' ntdll function */
BOOLEAN
NTAPI
LdrInitShimEngineDynamic(IN PVOID BaseAddress);


BOOL WINAPI SE_DynamicShim(LPCWSTR ProcessImage, HSDB hsdb, PVOID pQueryResult, LPCSTR Module, LPDWORD lpdwDynamicToken)
{
    if (g_bShimEngInitialized)
    {
        SHIMENG_MSG("ReactOS HACK(CORE-13283): ShimEng already initialized!\n");
        return TRUE;
    }

    g_bShimDuringInit = TRUE;
    SeiInit(ProcessImage, hsdb, pQueryResult, FALSE);
    g_bShimDuringInit = FALSE;

    LdrInitShimEngineDynamic(g_hInstance);

    return TRUE;
}

