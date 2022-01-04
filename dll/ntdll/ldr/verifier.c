/*
 * PROJECT:     ReactOS NT User Mode Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Verifier support routines
 * COPYRIGHT:   Copyright 2011 Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */


#include <ntdll.h>
#include <reactos/verifier.h>

#define NDEBUG
#include <debug.h>

extern PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
ULONG AVrfpVerifierFlags = 0;
WCHAR AVrfpVerifierDllsString[256] = { 0 };
ULONG AVrfpDebug = 0;
BOOL AVrfpInitialized = FALSE;
RTL_CRITICAL_SECTION AVrfpVerifierLock;
LIST_ENTRY AVrfpVerifierProvidersList;

#define VERIFIER_DLL_FLAGS_RESOLVED  1


typedef struct _VERIFIER_PROVIDER
{
    LIST_ENTRY ListEntry;
    UNICODE_STRING DllName;
    PVOID BaseAddress;
    PVOID EntryPoint;

    // Provider data
    PRTL_VERIFIER_DLL_DESCRIPTOR ProviderDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;
    RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK ProviderNtdllHeapFreeCallback;
} VERIFIER_PROVIDER, *PVERIFIER_PROVIDER;




VOID
NTAPI
AVrfReadIFEO(HANDLE KeyHandle)
{
    NTSTATUS Status;

    Status = LdrQueryImageFileKeyOption(KeyHandle,
                                        L"VerifierDlls",
                                        REG_SZ,
                                        AVrfpVerifierDllsString,
                                        sizeof(AVrfpVerifierDllsString) - sizeof(WCHAR),
                                        NULL);

    if (!NT_SUCCESS(Status))
        AVrfpVerifierDllsString[0] = UNICODE_NULL;

    Status = LdrQueryImageFileKeyOption(KeyHandle,
                                        L"VerifierFlags",
                                        REG_DWORD,
                                        &AVrfpVerifierFlags,
                                        sizeof(AVrfpVerifierFlags),
                                        NULL);
    if (!NT_SUCCESS(Status))
        AVrfpVerifierFlags = RTL_VRF_FLG_HANDLE_CHECKS | RTL_VRF_FLG_FAST_FILL_HEAP | RTL_VRF_FLG_LOCK_CHECKS;

    Status = LdrQueryImageFileKeyOption(KeyHandle,
                                        L"VerifierDebug",
                                        REG_DWORD,
                                        &AVrfpDebug,
                                        sizeof(AVrfpDebug),
                                        NULL);
    if (!NT_SUCCESS(Status))
        AVrfpDebug = 0;
}


NTSTATUS
NTAPI
LdrpInitializeApplicationVerifierPackage(HANDLE KeyHandle, PPEB Peb, BOOLEAN SystemWide, BOOLEAN ReadAdvancedOptions)
{
    /* If global flags request DPH, perform some additional actions */
    if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)
    {
        // TODO: Read advanced DPH flags from the registry if requested
        if (ReadAdvancedOptions)
        {
            UNIMPLEMENTED;
        }

        /* Enable page heap */
        RtlpPageHeapEnabled = TRUE;
    }

    AVrfReadIFEO(KeyHandle);

    return STATUS_SUCCESS;
}

BOOLEAN
AVrfpIsVerifierProviderDll(PVOID BaseAddress)
{
    PLIST_ENTRY Entry;
    PVERIFIER_PROVIDER Provider;

    for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
    {
        Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

        if (BaseAddress == Provider->BaseAddress)
            return TRUE;
    }

    return FALSE;
}

SIZE_T
AVrfpCountThunks(PIMAGE_THUNK_DATA Thunk)
{
    SIZE_T Count = 0;
    while (Thunk[Count].u1.Function)
        Count++;
    return Count;
}

VOID
AVrfpSnapDllImports(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    ULONG Size;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    PBYTE DllBase = LdrEntry->DllBase;

    ImportDescriptor = RtlImageDirectoryEntryToData(DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &Size);
    if (!ImportDescriptor)
    {
        //SHIMENG_INFO("Skipping module 0x%p \"%wZ\" due to no iat found\n", LdrEntry->DllBase, &LdrEntry->BaseDllName);
        return;
    }

    for (; ImportDescriptor->Name && ImportDescriptor->OriginalFirstThunk; ImportDescriptor++)
    {
        PIMAGE_THUNK_DATA FirstThunk;
        PVOID UnprotectedPtr = NULL;
        SIZE_T UnprotectedSize = 0;
        ULONG OldProtection = 0;
        FirstThunk = (PIMAGE_THUNK_DATA)(DllBase + ImportDescriptor->FirstThunk);

        /* Walk all imports */
        for (;FirstThunk->u1.Function; FirstThunk++)
        {
            PLIST_ENTRY Entry;
            PVERIFIER_PROVIDER Provider;

            for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
            {
                PRTL_VERIFIER_DLL_DESCRIPTOR DllDescriptor;

                Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);
                for (DllDescriptor = Provider->ProviderDlls; DllDescriptor && DllDescriptor->DllName; ++DllDescriptor)
                {
                    PRTL_VERIFIER_THUNK_DESCRIPTOR ThunkDescriptor;

                    for (ThunkDescriptor = DllDescriptor->DllThunks; ThunkDescriptor && ThunkDescriptor->ThunkName; ++ThunkDescriptor)
                    {
                        /* Just compare function addresses, the loader will have handled forwarders and ordinals for us */
                        if ((PVOID)FirstThunk->u1.Function != ThunkDescriptor->ThunkOldAddress)
                            continue;

                        if (!UnprotectedPtr)
                        {
                            PVOID Ptr = &FirstThunk->u1.Function;
                            SIZE_T Size = sizeof(FirstThunk->u1.Function) * AVrfpCountThunks(FirstThunk);
                            NTSTATUS Status;

                            UnprotectedPtr = Ptr;
                            UnprotectedSize = Size;

                            Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                            &Ptr,
                                                            &Size,
                                                            PAGE_EXECUTE_READWRITE,
                                                            &OldProtection);

                            if (!NT_SUCCESS(Status))
                            {
                                DbgPrint("AVRF: Unable to unprotect IAT to modify thunks (status %08X).\n", Status);
                                UnprotectedPtr = NULL;
                                continue;
                            }
                        }

                        if (ThunkDescriptor->ThunkNewAddress == NULL)
                        {
                            DbgPrint("AVRF: internal error: New thunk for %s is null.\n", ThunkDescriptor->ThunkName);
                            continue;
                        }
                        FirstThunk->u1.Function = (SIZE_T)ThunkDescriptor->ThunkNewAddress;
                        if (AVrfpDebug & RTL_VRF_DBG_SHOWSNAPS)
                            DbgPrint("AVRF: Snapped (%wZ: %s) with (%wZ: %p).\n",
                                        &LdrEntry->BaseDllName,
                                        ThunkDescriptor->ThunkName,
                                        &Provider->DllName,
                                        ThunkDescriptor->ThunkNewAddress);
                    }
                }
            }
        }

        if (UnprotectedPtr)
        {
            PVOID Ptr = UnprotectedPtr;
            SIZE_T Size = UnprotectedSize;
            NTSTATUS Status;

            UnprotectedPtr = Ptr;
            UnprotectedSize = Size;

            Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            &Ptr,
                                            &Size,
                                            OldProtection,
                                            &OldProtection);
            if (!NT_SUCCESS(Status))
            {
                DbgPrint("AVRF: Unable to reprotect IAT to modify thunks (status %08X).\n", Status);
            }
        }
    }
}


VOID
AvrfpResolveThunks(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PLIST_ENTRY Entry;
    PVERIFIER_PROVIDER Provider;

    if (!AVrfpInitialized)
        return;

    for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
    {
        PRTL_VERIFIER_DLL_DESCRIPTOR DllDescriptor;

        Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

        for (DllDescriptor = Provider->ProviderDlls; DllDescriptor && DllDescriptor->DllName; ++DllDescriptor)
        {
            PRTL_VERIFIER_THUNK_DESCRIPTOR ThunkDescriptor;

            if ((DllDescriptor->DllFlags & VERIFIER_DLL_FLAGS_RESOLVED) ||
                _wcsicmp(DllDescriptor->DllName, LdrEntry->BaseDllName.Buffer))
                continue;

            if (AVrfpDebug & RTL_VRF_DBG_SHOWVERIFIEDEXPORTS)
                DbgPrint("AVRF: pid 0x%X: found dll descriptor for `%wZ' with verified exports\n",
                            NtCurrentTeb()->ClientId.UniqueProcess,
                            &LdrEntry->BaseDllName);

            for (ThunkDescriptor = DllDescriptor->DllThunks; ThunkDescriptor && ThunkDescriptor->ThunkName; ++ThunkDescriptor)
            {
                if (!ThunkDescriptor->ThunkOldAddress)
                {
                    ANSI_STRING ThunkName;

                    RtlInitAnsiString(&ThunkName, ThunkDescriptor->ThunkName);
                    /* We cannot call the public api, because that would run init routines! */
                    if (NT_SUCCESS(LdrpGetProcedureAddress(LdrEntry->DllBase, &ThunkName, 0, &ThunkDescriptor->ThunkOldAddress, FALSE)))
                    {
                        if (AVrfpDebug & RTL_VRF_DBG_SHOWFOUNDEXPORTS)
                            DbgPrint("AVRF: (%wZ) %Z export found.\n", &LdrEntry->BaseDllName, &ThunkName);
                    }
                    else
                    {
                        if (AVrfpDebug & RTL_VRF_DBG_SHOWFOUNDEXPORTS)
                            DbgPrint("AVRF: warning: did not find `%Z' export in %wZ.\n", &ThunkName, &LdrEntry->BaseDllName);
                    }
                }
            }

            DllDescriptor->DllFlags |= VERIFIER_DLL_FLAGS_RESOLVED;
        }
    }

    AVrfpSnapDllImports(LdrEntry);
}



VOID
NTAPI
AVrfDllLoadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PLIST_ENTRY Entry;

    if (!(NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER))
        return;

    RtlEnterCriticalSection(&AVrfpVerifierLock);
    if (!AVrfpIsVerifierProviderDll(LdrEntry->DllBase))
    {
        AvrfpResolveThunks(LdrEntry);

        for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
        {
            PVERIFIER_PROVIDER Provider;
            RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;

            Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

            ProviderDllLoadCallback = Provider->ProviderDllLoadCallback;
            if (ProviderDllLoadCallback)
            {
                ProviderDllLoadCallback(LdrEntry->BaseDllName.Buffer,
                                        LdrEntry->DllBase,
                                        LdrEntry->SizeOfImage,
                                        LdrEntry);
            }
        }
    }
    RtlLeaveCriticalSection(&AVrfpVerifierLock);
}

VOID
NTAPI
AVrfDllUnloadNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PLIST_ENTRY Entry;

    if (!(NtCurrentPeb()->NtGlobalFlag & FLG_APPLICATION_VERIFIER))
        return;

    RtlEnterCriticalSection(&AVrfpVerifierLock);
    if (!AVrfpIsVerifierProviderDll(LdrEntry->DllBase))
    {
        for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
        {
            PVERIFIER_PROVIDER Provider;
            RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;

            Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

            ProviderDllUnloadCallback = Provider->ProviderDllUnloadCallback;
            if (ProviderDllUnloadCallback)
            {
                ProviderDllUnloadCallback(LdrEntry->BaseDllName.Buffer,
                                          LdrEntry->DllBase,
                                          LdrEntry->SizeOfImage,
                                          LdrEntry);
            }
        }
    }
    RtlLeaveCriticalSection(&AVrfpVerifierLock);
}


VOID
NTAPI
AVrfPageHeapDllNotification(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    /* Check if page heap dll notification is turned on */
    if (!(RtlpDphGlobalFlags & DPH_FLAG_DLL_NOTIFY))
        return;

    /* We don't support this flag currently */
    UNIMPLEMENTED;
}


VOID
NTAPI
AVrfpResnapInitialModules(VOID)
{
    PLIST_ENTRY ListHead, ListEntry;

    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for (ListEntry = ListHead->Flink; ListHead != ListEntry; ListEntry = ListEntry->Flink)
    {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (AVrfpIsVerifierProviderDll(LdrEntry->DllBase))
        {
            if (AVrfpDebug & RTL_VRF_DBG_SHOWSNAPS)
                DbgPrint("AVRF: skipped resnapping provider %wZ ...\n", &LdrEntry->BaseDllName);
        }
        else
        {
            if (AVrfpDebug & RTL_VRF_DBG_SHOWSNAPS)
                DbgPrint("AVRF: resnapping %wZ ...\n", &LdrEntry->BaseDllName);

            AvrfpResolveThunks(LdrEntry);
        }
    }
}

PVOID
NTAPI
AvrfpFindDuplicateThunk(PLIST_ENTRY EndEntry, PWCHAR DllName, PCHAR ThunkName)
{
    PLIST_ENTRY Entry;

    for (Entry = AVrfpVerifierProvidersList.Flink; Entry != EndEntry; Entry = Entry->Flink)
    {
        PVERIFIER_PROVIDER Provider;
        PRTL_VERIFIER_DLL_DESCRIPTOR DllDescriptor;

        Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

        if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING_DEBUG)
            DbgPrint("AVRF: chain: searching in %wZ\n", &Provider->DllName);

        for (DllDescriptor = Provider->ProviderDlls; DllDescriptor && DllDescriptor->DllName; ++DllDescriptor)
        {
            PRTL_VERIFIER_THUNK_DESCRIPTOR ThunkDescriptor;

            if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING_DEBUG)
                DbgPrint("AVRF: chain: dll: %ws\n", DllDescriptor->DllName);

            if (_wcsicmp(DllDescriptor->DllName, DllName))
                continue;

            for (ThunkDescriptor = DllDescriptor->DllThunks; ThunkDescriptor && ThunkDescriptor->ThunkName; ++ThunkDescriptor)
            {
                if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING_DEBUG)
                    DbgPrint("AVRF: chain: thunk: %s == %s ?\n", ThunkDescriptor->ThunkName, ThunkName);

                if (!_stricmp(ThunkDescriptor->ThunkName, ThunkName))
                {
                    if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING_DEBUG)
                        DbgPrint("AVRF: Found duplicate for (%ws: %s) in %wZ\n",
                                    DllDescriptor->DllName, ThunkDescriptor->ThunkName, &Provider->DllName);

                    return ThunkDescriptor->ThunkNewAddress;
                }
            }
        }
    }
    return NULL;
}


VOID
NTAPI
AVrfpChainDuplicateThunks(VOID)
{
    PLIST_ENTRY Entry;
    PVERIFIER_PROVIDER Provider;

    for (Entry = AVrfpVerifierProvidersList.Flink; Entry != &AVrfpVerifierProvidersList; Entry = Entry->Flink)
    {
        PRTL_VERIFIER_DLL_DESCRIPTOR DllDescriptor;
        PRTL_VERIFIER_THUNK_DESCRIPTOR ThunkDescriptor;

        Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);

        for (DllDescriptor = Provider->ProviderDlls; DllDescriptor && DllDescriptor->DllName; ++DllDescriptor)
        {
            for (ThunkDescriptor = DllDescriptor->DllThunks; ThunkDescriptor && ThunkDescriptor->ThunkName; ++ThunkDescriptor)
            {
                PVOID Ptr;

                if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING_DEBUG)
                    DbgPrint("AVRF: Checking %wZ for duplicate (%ws: %s)\n",
                             &Provider->DllName, DllDescriptor->DllName, ThunkDescriptor->ThunkName);

                Ptr = AvrfpFindDuplicateThunk(Entry, DllDescriptor->DllName, ThunkDescriptor->ThunkName);
                if (Ptr)
                {
                    if (AVrfpDebug & RTL_VRF_DBG_SHOWCHAINING)
                        DbgPrint("AVRF: Chaining (%ws: %s) to %wZ\n", DllDescriptor->DllName, ThunkDescriptor->ThunkName, &Provider->DllName);

                    ThunkDescriptor->ThunkOldAddress = Ptr;
                }
            }
        }
    }
}

NTSTATUS
NTAPI
AVrfpLoadAndInitializeProvider(PVERIFIER_PROVIDER Provider)
{
    WCHAR StringBuffer[MAX_PATH + 11];
    UNICODE_STRING DllPath;
    PRTL_VERIFIER_PROVIDER_DESCRIPTOR Descriptor;
    PIMAGE_NT_HEADERS ImageNtHeader;
    NTSTATUS Status;

    RtlInitEmptyUnicodeString(&DllPath, StringBuffer, sizeof(StringBuffer));
    RtlAppendUnicodeToString(&DllPath, SharedUserData->NtSystemRoot);
    RtlAppendUnicodeToString(&DllPath, L"\\System32\\");

    if (AVrfpDebug & RTL_VRF_DBG_SHOWSNAPS)
        DbgPrint("AVRF: verifier dll `%wZ'\n", &Provider->DllName);

    Status = LdrLoadDll(DllPath.Buffer, NULL, &Provider->DllName, &Provider->BaseAddress);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("AVRF: %wZ: failed to load provider `%wZ' (status %08X) from %wZ\n",
                 &LdrpImageEntry->BaseDllName,
                 &Provider->DllName,
                 Status,
                 &DllPath);
        return Status;
    }

    /* Prevent someone funny from specifying his own application as provider */
    ImageNtHeader = RtlImageNtHeader(Provider->BaseAddress);
    if (!ImageNtHeader ||
        !(ImageNtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        DbgPrint("AVRF: provider %wZ is not a DLL image\n", &Provider->DllName);
        return STATUS_DLL_INIT_FAILED;
    }

    Provider->EntryPoint = LdrpFetchAddressOfEntryPoint(Provider->BaseAddress);
    if (!Provider->EntryPoint)
    {
        DbgPrint("AVRF: cannot find an entry point for provider %wZ\n", &Provider->DllName);
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    _SEH2_TRY
    {
        if (LdrpCallInitRoutine(Provider->EntryPoint,
                                Provider->BaseAddress,
                                DLL_PROCESS_VERIFIER,
                                &Descriptor))
        {
            if (Descriptor && Descriptor->Length == sizeof(RTL_VERIFIER_PROVIDER_DESCRIPTOR))
            {
                /* Copy the data */
                Provider->ProviderDlls = Descriptor->ProviderDlls;
                Provider->ProviderDllLoadCallback = Descriptor->ProviderDllLoadCallback;
                Provider->ProviderDllUnloadCallback = Descriptor->ProviderDllUnloadCallback;
                Provider->ProviderNtdllHeapFreeCallback = Descriptor->ProviderNtdllHeapFreeCallback;

                /* Update some info for the provider */
                Descriptor->VerifierImage = LdrpImageEntry->BaseDllName.Buffer;
                Descriptor->VerifierFlags = AVrfpVerifierFlags;
                Descriptor->VerifierDebug = AVrfpDebug;

                /* We don't have these yet */
                DPRINT1("AVRF: RtlpGetStackTraceAddress MISSING\n");
                DPRINT1("AVRF: RtlpDebugPageHeapCreate MISSING\n");
                DPRINT1("AVRF: RtlpDebugPageHeapDestroy MISSING\n");
                Descriptor->RtlpGetStackTraceAddress = NULL;
                Descriptor->RtlpDebugPageHeapCreate = NULL;
                Descriptor->RtlpDebugPageHeapDestroy = NULL;
                Status = STATUS_SUCCESS;
            }
            else
            {
                DbgPrint("AVRF: provider %wZ passed an invalid descriptor @ %p\n", &Provider->DllName, Descriptor);
                Status = STATUS_INVALID_PARAMETER_4;
            }
        }
        else
        {
            DbgPrint("AVRF: provider %wZ did not initialize correctly\n", &Provider->DllName);
            Status = STATUS_DLL_INIT_FAILED;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
        return Status;


    if (AVrfpDebug & RTL_VRF_DBG_LISTPROVIDERS)
        DbgPrint("AVRF: initialized provider %wZ (descriptor @ %p)\n", &Provider->DllName, Descriptor);

    /* Done loading providers, allow dll notifications */
    AVrfpInitialized = TRUE;

    AVrfpChainDuplicateThunks();
    AVrfpResnapInitialModules();

    /* Manually call with DLL_PROCESS_ATTACH, since the process is not done initializing */
    _SEH2_TRY
    {
        if (!LdrpCallInitRoutine(Provider->EntryPoint,
                                 Provider->BaseAddress,
                                 DLL_PROCESS_ATTACH,
                                 NULL))
        {
            DbgPrint("AVRF: provider %wZ did not initialize correctly\n", &Provider->DllName);
            Status = STATUS_DLL_INIT_FAILED;
        }

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}


NTSTATUS
NTAPI
AVrfInitializeVerifier(VOID)
{
    NTSTATUS Status;
    PVERIFIER_PROVIDER Provider;
    PLIST_ENTRY Entry;
    WCHAR* Ptr, *Next;

    Status = RtlInitializeCriticalSection(&AVrfpVerifierLock);
    InitializeListHead(&AVrfpVerifierProvidersList);

    if (!NT_SUCCESS(Status))
        return Status;

    DbgPrint("AVRF: %wZ: pid 0x%X: flags 0x%X: application verifier enabled\n",
             &LdrpImageEntry->BaseDllName, NtCurrentTeb()->ClientId.UniqueProcess, AVrfpVerifierFlags);

    Provider = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VERIFIER_PROVIDER));
    if (!Provider)
        return STATUS_NO_MEMORY;

    RtlInitUnicodeString(&Provider->DllName, L"verifier.dll");
    InsertTailList(&AVrfpVerifierProvidersList, &Provider->ListEntry);

    Next = AVrfpVerifierDllsString;

    do
    {
        while (*Next == L' ' || *Next == L'\t')
            Next++;

        Ptr = Next;

        while (*Next != ' ' && *Next != '\t' && *Next)
            Next++;

        if (*Next)
            *(Next++) = '\0';
        else
            Next = NULL;

        if (*Ptr)
        {
            Provider = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VERIFIER_PROVIDER));
            if (!Provider)
                return STATUS_NO_MEMORY;
            RtlInitUnicodeString(&Provider->DllName, Ptr);
            InsertTailList(&AVrfpVerifierProvidersList, &Provider->ListEntry);
        }
    } while (Next);

    Entry = AVrfpVerifierProvidersList.Flink;
    while (Entry != &AVrfpVerifierProvidersList)
    {
        Provider = CONTAINING_RECORD(Entry, VERIFIER_PROVIDER, ListEntry);
        Entry = Entry->Flink;

        Status = AVrfpLoadAndInitializeProvider(Provider);
        if (!NT_SUCCESS(Status))
        {
            RemoveEntryList(&Provider->ListEntry);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Provider);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("AVRF: %wZ: pid 0x%X: application verifier will be disabled due to an initialization error.\n",
                 &LdrpImageEntry->BaseDllName, NtCurrentTeb()->ClientId.UniqueProcess);
        NtCurrentPeb()->NtGlobalFlag &= ~FLG_APPLICATION_VERIFIER;
    }

    return Status;
}

