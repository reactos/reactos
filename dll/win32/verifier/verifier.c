/*
 * PROJECT:     Application verifier
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main entrypoint
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include <ndk/rtlfuncs.h>
#include <reactos/verifier.h>

#if 0
#define PROVIDER_PREFIX "AVRF"
#else
#define PROVIDER_PREFIX "RVRF"
#endif


VOID NTAPI AVrfpDllLoadCallback(PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved);
VOID NTAPI AVrfpDllUnloadCallback(PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved);
VOID NTAPI AVrfpNtdllHeapFreeCallback(PVOID AllocationBase, SIZE_T AllocationSize);

// DPFLTR_VERIFIER_ID


NTSTATUS NTAPI AVrfpLdrGetProcedureAddress(IN PVOID BaseAddress, IN PANSI_STRING Name, IN ULONG Ordinal, OUT PVOID *ProcedureAddress);

static RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpNtdllThunks[] =
{
    { "LdrGetProcedureAddress", NULL, AVrfpLdrGetProcedureAddress },
    { NULL }
};

FARPROC WINAPI AVrfpGetProcAddress(IN HMODULE hModule, IN LPCSTR lpProcName);

static RTL_VERIFIER_THUNK_DESCRIPTOR AVrfpKernel32Thunks[] =
{
    { "GetProcAddress", NULL, AVrfpGetProcAddress },
    { NULL }
};

static RTL_VERIFIER_DLL_DESCRIPTOR AVrfpDllDescriptors[] =
{
    { L"ntdll.dll", 0, NULL, AVrfpNtdllThunks },
    { L"kernel32.dll", 0, NULL, AVrfpKernel32Thunks },
    { NULL }
};

RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfpProvider =
{
    /*.Length =*/ sizeof(AVrfpProvider),
    /*.ProviderDlls =*/ AVrfpDllDescriptors,
    /*.ProviderDllLoadCallback =*/ AVrfpDllLoadCallback,
    /*.ProviderDllUnloadCallback =*/ AVrfpDllUnloadCallback,
    /*.VerifierImage =*/ NULL,
    /*.VerifierFlags =*/ 0,
    /*.VerifierDebug =*/ 0,
    /*.RtlpGetStackTraceAddress =*/ NULL,
    /*.RtlpDebugPageHeapCreate =*/ NULL,
    /*.RtlpDebugPageHeapDestroy =*/ NULL,
    /*.ProviderNtdllHeapFreeCallback =*/ AVrfpNtdllHeapFreeCallback
};



BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_VERIFIER:
        *(PRTL_VERIFIER_PROVIDER_DESCRIPTOR*)lpReserved = &AVrfpProvider;
        break;
    }
    return TRUE;
}

VOID NTAPI AVrfpDllLoadCallback(PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = (PLDR_DATA_TABLE_ENTRY)Reserved;
    DbgPrint(PROVIDER_PREFIX ": %ws @ %p: ep: %p\n", DllName, DllBase, LdrEntry->EntryPoint);
    /* TODO: Hook entrypoint */
}


VOID NTAPI AVrfpDllUnloadCallback(PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved)
{
    DbgPrint(PROVIDER_PREFIX ": unloading %ws\n", DllName);
}

VOID NTAPI AVrfpNtdllHeapFreeCallback(PVOID AllocationBase, SIZE_T AllocationSize)
{
    DbgPrint(PROVIDER_PREFIX ": Heap free 0x%x @ %p\n", AllocationSize, AllocationBase);
    /* TODO: Sanity checks */
}

PVOID AVrfpFindReplacementThunk(PVOID Proc)
{
    PRTL_VERIFIER_DLL_DESCRIPTOR DllDescriptor;
    PRTL_VERIFIER_THUNK_DESCRIPTOR ThunkDescriptor;

    for (DllDescriptor = AVrfpDllDescriptors; DllDescriptor->DllName; ++DllDescriptor)
    {
        for (ThunkDescriptor = DllDescriptor->DllThunks; ThunkDescriptor->ThunkName; ++ThunkDescriptor)
        {
            if (ThunkDescriptor->ThunkOldAddress == Proc)
            {
                return ThunkDescriptor->ThunkNewAddress;
            }
        }
    }
    return Proc;
}


NTSTATUS NTAPI AVrfpLdrGetProcedureAddress(IN PVOID BaseAddress, IN PANSI_STRING Name, IN ULONG Ordinal, OUT PVOID *ProcedureAddress)
{
    NTSTATUS (NTAPI *oLdrGetProcedureAddress)(IN PVOID BaseAddress, IN PANSI_STRING Name, IN ULONG Ordinal, OUT PVOID *ProcedureAddress);
    NTSTATUS Status;
    PVOID Replacement;

    oLdrGetProcedureAddress = AVrfpNtdllThunks[0].ThunkOldAddress;

    Status = oLdrGetProcedureAddress(BaseAddress, Name, Ordinal, ProcedureAddress);
    if (!NT_SUCCESS(Status))
        return Status;

    Replacement = AVrfpFindReplacementThunk(*ProcedureAddress);
    if (Replacement != *ProcedureAddress)
    {
        *ProcedureAddress = Replacement;
        if (AVrfpProvider.VerifierDebug & RTL_VRF_DBG_VERIFIER_SHOWDYNTHUNKS)
            DbgPrint(PROVIDER_PREFIX ": AVrfpLdrGetProcedureAddress (%p, %Z) -> thunk address %p\n", BaseAddress, Name, *ProcedureAddress);
    }

    return Status;
}

FARPROC WINAPI AVrfpGetProcAddress(IN HMODULE hModule, IN LPCSTR lpProcName)
{
    FARPROC (WINAPI* oGetProcAddress)(IN HMODULE hModule, IN LPCSTR lpProcName);
    FARPROC Proc, Replacement;

    if (AVrfpProvider.VerifierDebug & RTL_VRF_DBG_VERIFIER_LOGCALLS)
        DbgPrint(PROVIDER_PREFIX ": AVrfpGetProcAddress (%p, %s)\n", hModule, lpProcName);

    oGetProcAddress = AVrfpKernel32Thunks[0].ThunkOldAddress;
    Proc = oGetProcAddress(hModule, lpProcName);
    if (!Proc)
        return Proc;

    Replacement = AVrfpFindReplacementThunk(Proc);
    if (Replacement != Proc)
    {
        Proc = Replacement;
        if (AVrfpProvider.VerifierDebug & RTL_VRF_DBG_VERIFIER_SHOWDYNTHUNKS)
            DbgPrint(PROVIDER_PREFIX ": AVrfpGetProcAddress (%p, %s) -> thunk address %p\n", hModule, lpProcName, Proc);
    }

    return Proc;
}

