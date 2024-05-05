/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrinit.c
 * PURPOSE:         User-Mode Process/Thread Startup
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#include <compat_undoc.h>
#include <compatguid_undoc.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

HANDLE ImageExecOptionsKey;
HANDLE Wow64ExecOptionsKey;
UNICODE_STRING ImageExecOptionsString = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");
UNICODE_STRING Wow64OptionsString = RTL_CONSTANT_STRING(L"");
UNICODE_STRING NtDllString = RTL_CONSTANT_STRING(L"ntdll.dll");
UNICODE_STRING Kernel32String = RTL_CONSTANT_STRING(L"kernel32.dll");
const UNICODE_STRING LdrpDotLocal = RTL_CONSTANT_STRING(L".Local");

BOOLEAN LdrpInLdrInit;
LONG LdrpProcessInitialized;
BOOLEAN LdrpLoaderLockInit;
BOOLEAN LdrpLdrDatabaseIsSetup;
BOOLEAN LdrpShutdownInProgress;
HANDLE LdrpShutdownThreadId;

BOOLEAN LdrpDllValidation;

PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
PUNICODE_STRING LdrpTopLevelDllBeingLoaded;
WCHAR StringBuffer[156];
extern PTEB LdrpTopLevelDllBeingLoadedTeb; // defined in rtlsupp.c!
PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer;
PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;

static NTSTATUS (WINAPI *Kernel32ProcessInitPostImportFunction)(VOID);
static BOOL (WINAPI *Kernel32BaseQueryModuleData)(IN LPSTR ModuleName, IN LPSTR Unk1, IN PVOID Unk2, IN PVOID Unk3, IN PVOID Unk4);

RTL_BITMAP TlsBitMap;
RTL_BITMAP TlsExpansionBitMap;
RTL_BITMAP FlsBitMap;
BOOLEAN LdrpImageHasTls;
LIST_ENTRY LdrpTlsList;
ULONG LdrpNumberOfTlsEntries;
ULONG LdrpNumberOfProcessors;
PVOID NtDllBase;
extern LARGE_INTEGER RtlpTimeout;
extern BOOLEAN RtlpTimeoutDisable;
PVOID LdrpHeap;
LIST_ENTRY LdrpHashTable[LDR_HASH_TABLE_ENTRIES];
LIST_ENTRY LdrpDllNotificationList;
HANDLE LdrpKnownDllObjectDirectory;
UNICODE_STRING LdrpKnownDllPath;
WCHAR LdrpKnownDllPathBuffer[128];
UNICODE_STRING LdrpDefaultPath;

PEB_LDR_DATA PebLdr;

RTL_CRITICAL_SECTION_DEBUG LdrpLoaderLockDebug;
RTL_CRITICAL_SECTION LdrpLoaderLock =
{
    &LdrpLoaderLockDebug,
    -1,
    0,
    0,
    0,
    0
};
RTL_CRITICAL_SECTION FastPebLock;

BOOLEAN ShowSnaps;

ULONG LdrpFatalHardErrorCount;
ULONG LdrpActiveUnloadCount;

//extern LIST_ENTRY RtlCriticalSectionList;

VOID NTAPI RtlpInitializeVectoredExceptionHandling(VOID);
VOID NTAPI RtlpInitDeferredCriticalSection(VOID);
VOID NTAPI RtlInitializeHeapManager(VOID);

ULONG RtlpDisableHeapLookaside; // TODO: Move to heap.c
ULONG RtlpShutdownProcessFlags; // TODO: Use it

NTSTATUS LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders, PVOID ImageBase);
NTSTATUS NTAPI RtlpInitializeActCtx(PVOID* pOldShimData);
extern BOOLEAN RtlpUse16ByteSLists;

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(
    _In_ PUNICODE_STRING SubKey,
    _In_ BOOLEAN Wow64,
    _Out_ PHANDLE NewKeyHandle)
{
    PHANDLE RootKeyLocation;
    HANDLE RootKey;
    UNICODE_STRING SubKeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PWCHAR p1;

    /* Check which root key to open */
    if (Wow64)
        RootKeyLocation = &Wow64ExecOptionsKey;
    else
        RootKeyLocation = &ImageExecOptionsKey;

    /* Get the current key */
    RootKey = *RootKeyLocation;

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               Wow64 ?
                               &Wow64OptionsString : &ImageExecOptionsString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the root key */
    Status = ZwOpenKey(&RootKey, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Write the key handle */
        if (InterlockedCompareExchangePointer(RootKeyLocation, RootKey, NULL) != NULL)
        {
            /* Someone already opened it, use it instead */
            NtClose(RootKey);
            RootKey = *RootKeyLocation;
        }

        /* Extract the name */
        SubKeyString = *SubKey;
        p1 = (PWCHAR)((ULONG_PTR)SubKeyString.Buffer + SubKeyString.Length);
        while (SubKeyString.Length)
        {
            if (p1[-1] == L'\\') break;
            p1--;
            SubKeyString.Length -= sizeof(*p1);
        }
        SubKeyString.Buffer = p1;
        SubKeyString.Length = SubKey->Length - SubKeyString.Length;

        /* Setup the object attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyString,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);

        /* Open the setting key */
        Status = ZwOpenKey((PHANDLE)NewKeyHandle, GENERIC_READ, &ObjectAttributes);
    }

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength)
{
    ULONG KeyInfo[256];
    UNICODE_STRING ValueNameString, IntegerString;
    ULONG KeyInfoSize, ResultSize;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyInfo;
    BOOLEAN FreeHeap = FALSE;
    NTSTATUS Status;

    /* Build a string for the value name */
    Status = RtlInitUnicodeStringEx(&ValueNameString, ValueName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Query the value */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueNameString,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyInfo),
                             &ResultSize);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Our local buffer wasn't enough, allocate one */
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                      KeyValueInformation->DataLength;
        KeyValueInformation = RtlAllocateHeap(RtlGetProcessHeap(),
                                              0,
                                              KeyInfoSize);
        if (KeyValueInformation != NULL)
        {
            /* Try again */
            Status = ZwQueryValueKey(KeyHandle,
                                     &ValueNameString,
                                     KeyValuePartialInformation,
                                     KeyValueInformation,
                                     KeyInfoSize,
                                     &ResultSize);
            FreeHeap = TRUE;
        }
        else
        {
            /* Give up this time */
            Status = STATUS_NO_MEMORY;
        }
    }

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Handle binary data */
        if (KeyValueInformation->Type == REG_BINARY)
        {
            /* Check validity */
            if ((Buffer) && (KeyValueInformation->DataLength <= BufferSize))
            {
                /* Copy into buffer */
                RtlMoveMemory(Buffer,
                              &KeyValueInformation->Data,
                              KeyValueInformation->DataLength);
            }
            else
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
        else if (KeyValueInformation->Type == REG_DWORD)
        {
            /* Check for valid type */
            if (KeyValueInformation->Type != Type)
            {
                /* Error */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else
            {
                /* Check validity */
                if ((Buffer) &&
                    (BufferSize == sizeof(ULONG)) &&
                    (KeyValueInformation->DataLength <= BufferSize))
                {
                    /* Copy into buffer */
                    RtlMoveMemory(Buffer,
                                  &KeyValueInformation->Data,
                                  KeyValueInformation->DataLength);
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Copy the result length */
                if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type != REG_SZ)
        {
            /* We got something weird */
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else
        {
            /*  String, check what you requested */
            if (Type == REG_DWORD)
            {
                /* Validate */
                if (BufferSize != sizeof(ULONG))
                {
                    /* Invalid size */
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    /* OK, we know what you want... */
                    IntegerString.Buffer = (PWSTR)KeyValueInformation->Data;
                    IntegerString.Length = (USHORT)KeyValueInformation->DataLength -
                                           sizeof(WCHAR);
                    IntegerString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger(&IntegerString, 0, (PULONG)Buffer);
                }
            }
            else
            {
                /* Validate */
                if (KeyValueInformation->DataLength > BufferSize)
                {
                    /* Invalid */
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    /* Set the size */
                    BufferSize = KeyValueInformation->DataLength;
                }

                /* Copy the string */
                RtlMoveMemory(Buffer, &KeyValueInformation->Data, BufferSize);
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
    }

    /* Check if buffer was in heap */
    if (FreeHeap) RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(
    _In_ PUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength,
    _In_ BOOLEAN Wow64)
{
    NTSTATUS Status;
    HANDLE KeyHandle;

    /* Open a handle to the key */
    Status = LdrOpenImageFileOptionsKey(SubKey, Wow64, &KeyHandle);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Query the data */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            ValueName,
                                            Type,
                                            Buffer,
                                            BufferSize,
                                            ReturnedLength);

        /* Close the key */
        NtClose(KeyHandle);
    }

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    _In_ PUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength)
{
    /* Call the newer function */
    return LdrQueryImageFileExecutionOptionsEx(SubKey,
                                               ValueName,
                                               Type,
                                               Buffer,
                                               BufferSize,
                                               ReturnedLength,
                                               FALSE);
}

VOID
NTAPI
LdrpEnsureLoaderLockIsHeld(VOID)
{
    // Ignored atm
}

PVOID
NTAPI
LdrpFetchAddressOfSecurityCookie(PVOID BaseAddress, ULONG SizeOfImage)
{
    PIMAGE_LOAD_CONFIG_DIRECTORY ConfigDir;
    ULONG DirSize;
    PVOID Cookie = NULL;

    /* Check NT header first */
    if (!RtlImageNtHeader(BaseAddress)) return NULL;

    /* Get the pointer to the config directory */
    ConfigDir = RtlImageDirectoryEntryToData(BaseAddress,
                                             TRUE,
                                             IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                             &DirSize);

    /* Check for sanity */
    if (!ConfigDir ||
        (DirSize != 64 && ConfigDir->Size != DirSize) ||
        (ConfigDir->Size < 0x48))
        return NULL;

    /* Now get the cookie */
    Cookie = (PVOID)ConfigDir->SecurityCookie;

    /* Check this cookie */
    if ((PCHAR)Cookie <= (PCHAR)BaseAddress ||
        (PCHAR)Cookie >= (PCHAR)BaseAddress + SizeOfImage)
    {
        Cookie = NULL;
    }

    /* Return validated security cookie */
    return Cookie;
}

PVOID
NTAPI
LdrpInitSecurityCookie(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PULONG_PTR Cookie;
    LARGE_INTEGER Counter;
    ULONG_PTR NewCookie;

    /* Fetch address of the cookie */
    Cookie = LdrpFetchAddressOfSecurityCookie(LdrEntry->DllBase, LdrEntry->SizeOfImage);

    if (Cookie)
    {
        /* Check if it's a default one */
        if ((*Cookie == DEFAULT_SECURITY_COOKIE) ||
            (*Cookie == 0xBB40))
        {
            /* Make up a cookie from a bunch of values which may uniquely represent
               current moment of time, environment, etc */
            NtQueryPerformanceCounter(&Counter, NULL);

            NewCookie = Counter.LowPart ^ Counter.HighPart;
            NewCookie ^= (ULONG_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
            NewCookie ^= (ULONG_PTR)NtCurrentTeb()->ClientId.UniqueThread;

            /* Loop like it's done in KeQueryTickCount(). We don't want to call it directly. */
            while (SharedUserData->SystemTime.High1Time != SharedUserData->SystemTime.High2Time)
            {
                YieldProcessor();
            };

            /* Calculate the milliseconds value and xor it to the cookie */
            NewCookie ^= Int64ShrlMod32(UInt32x32To64(SharedUserData->TickCountMultiplier, SharedUserData->TickCount.LowPart), 24) +
                (SharedUserData->TickCountMultiplier * (SharedUserData->TickCount.High1Time << 8));

            /* Make the cookie 16bit if necessary */
            if (*Cookie == 0xBB40) NewCookie &= 0xFFFF;

            /* If the result is 0 or the same as we got, just subtract one from the existing value
               and that's it */
            if ((NewCookie == 0) || (NewCookie == *Cookie))
            {
                NewCookie = *Cookie - 1;
            }

            /* Set the new cookie value */
            *Cookie = NewCookie;
        }
    }

    return Cookie;
}

VOID
NTAPI
LdrpInitializeThread(IN PCONTEXT Context)
{
    PPEB Peb = NtCurrentPeb();
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry, ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    NTSTATUS Status;
    PVOID EntryPoint;

    DPRINT("LdrpInitializeThread() called for %wZ (%p/%p)\n",
            &LdrpImageEntry->BaseDllName,
            NtCurrentTeb()->RealClientId.UniqueProcess,
            NtCurrentTeb()->RealClientId.UniqueThread);

    /* Allocate an Activation Context Stack */
    DPRINT("ActivationContextStack %p\n", NtCurrentTeb()->ActivationContextStackPointer);
    Status = RtlAllocateActivationContextStack(&NtCurrentTeb()->ActivationContextStackPointer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warning: Unable to allocate ActivationContextStack\n");
    }

    /* Make sure we are not shutting down */
    if (LdrpShutdownInProgress) return;

    /* Allocate TLS */
    LdrpAllocateTls();

    /* Start at the beginning */
    ListHead = &Peb->Ldr->InMemoryOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        /* Make sure it's not ourselves */
        if (Peb->ImageBaseAddress != LdrEntry->DllBase)
        {
            /* Check if we should call */
            if (!(LdrEntry->Flags & LDRP_DONT_CALL_FOR_THREADS))
            {
                /* Get the entrypoint */
                EntryPoint = LdrEntry->EntryPoint;

                /* Check if we are ready to call it */
                if ((EntryPoint) &&
                    (LdrEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) &&
                    (LdrEntry->Flags & LDRP_IMAGE_DLL))
                {
                    /* Set up the Act Ctx */
                    ActCtx.Size = sizeof(ActCtx);
                    ActCtx.Format = 1;
                    RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

                    /* Activate the ActCtx */
                    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                           LdrEntry->EntryPointActivationContext);

                    _SEH2_TRY
                    {
                        /* Check if it has TLS */
                        if (LdrEntry->TlsIndex)
                        {
                            /* Make sure we're not shutting down */
                            if (!LdrpShutdownInProgress)
                            {
                                /* Call TLS */
                                LdrpCallTlsInitializers(LdrEntry, DLL_THREAD_ATTACH);
                            }
                        }

                        /* Make sure we're not shutting down */
                        if (!LdrpShutdownInProgress)
                        {
                            /* Call the Entrypoint */
                            DPRINT("%wZ - Calling entry point at %p for thread attaching, %p/%p\n",
                                   &LdrEntry->BaseDllName, LdrEntry->EntryPoint,
                                   NtCurrentTeb()->RealClientId.UniqueProcess,
                                   NtCurrentTeb()->RealClientId.UniqueThread);
                            LdrpCallInitRoutine(LdrEntry->EntryPoint,
                                                LdrEntry->DllBase,
                                                DLL_THREAD_ATTACH,
                                                NULL);
                        }
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_THREAD_ATTACH) for %wZ\n",
                                _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
                    }
                    _SEH2_END;

                    /* Deactivate the ActCtx */
                    RtlDeactivateActivationContextUnsafeFast(&ActCtx);
                }
            }
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Check for TLS */
    if (LdrpImageHasTls && !LdrpShutdownInProgress)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        _SEH2_TRY
        {
            /* Do TLS callbacks */
            LdrpCallTlsInitializers(LdrpImageEntry, DLL_THREAD_ATTACH);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    DPRINT("LdrpInitializeThread() done\n");
}

NTSTATUS
NTAPI
LdrpRunInitializeRoutines(IN PCONTEXT Context OPTIONAL)
{
    PLDR_DATA_TABLE_ENTRY LocalArray[16];
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry, *LdrRootEntry, OldInitializer;
    PVOID EntryPoint;
    ULONG Count, i;
    //ULONG BreakOnInit;
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    ULONG BreakOnDllLoad;
    PTEB OldTldTeb;
    BOOLEAN DllStatus;

    DPRINT("LdrpRunInitializeRoutines() called for %wZ (%p/%p)\n",
        &LdrpImageEntry->BaseDllName,
        NtCurrentTeb()->RealClientId.UniqueProcess,
        NtCurrentTeb()->RealClientId.UniqueThread);

    /* Check the Loader Lock */
    LdrpEnsureLoaderLockIsHeld();

     /* Get the number of entries to call */
    if ((Count = LdrpClearLoadInProgress()))
    {
        /* Check if we can use our local buffer */
        if (Count > 16)
        {
            /* Allocate space for all the entries */
            LdrRootEntry = RtlAllocateHeap(LdrpHeap,
                                           0,
                                           Count * sizeof(*LdrRootEntry));
            if (!LdrRootEntry) return STATUS_NO_MEMORY;
        }
        else
        {
            /* Use our local array */
            LdrRootEntry = LocalArray;
        }
    }
    else
    {
        /* Don't need one */
        LdrRootEntry = NULL;
    }

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("[%p,%p] LDR: Real INIT LIST for Process %wZ\n",
                NtCurrentTeb()->RealClientId.UniqueThread,
                NtCurrentTeb()->RealClientId.UniqueProcess,
                &Peb->ProcessParameters->ImagePathName);
    }

    /* Loop in order */
    ListHead = &Peb->Ldr->InInitializationOrderModuleList;
    NextEntry = ListHead->Flink;
    i = 0;
    while (NextEntry != ListHead)
    {
        /* Get the Data Entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        /* Check if we have a Root Entry */
        if (LdrRootEntry)
        {
            /* Check flags */
            if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
            {
                /* Setup the Cookie for the DLL */
                LdrpInitSecurityCookie(LdrEntry);

                /* Check for valid entrypoint */
                if (LdrEntry->EntryPoint)
                {
                    /* Write in array */
                    ASSERT(i < Count);
                    LdrRootEntry[i] = LdrEntry;

                    /* Display debug message */
                    if (ShowSnaps)
                    {
                        DPRINT1("[%p,%p] LDR: %wZ init routine %p\n",
                                NtCurrentTeb()->RealClientId.UniqueThread,
                                NtCurrentTeb()->RealClientId.UniqueProcess,
                                &LdrEntry->FullDllName,
                                LdrEntry->EntryPoint);
                    }
                    i++;
                }
            }
        }

        /* Set the flag */
        LdrEntry->Flags |= LDRP_ENTRY_PROCESSED;
        NextEntry = NextEntry->Flink;
    }

    Status = STATUS_SUCCESS;

    /* If we got a context, then we have to call Kernel32 for TS support */
    if (Context)
    {
        /* Check if we have one */
        if (Kernel32ProcessInitPostImportFunction)
        {
            /* Call it */
            Status = Kernel32ProcessInitPostImportFunction();
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("LDR: LdrpRunInitializeRoutines - Failed running kernel32 post-import function, Status=0x%08lx\n", Status);
            }
        }
        /* Clear it */
        Kernel32ProcessInitPostImportFunction = NULL;
    }

    /* No root entry? return */
    if (!LdrRootEntry)
        return Status;

    /* Set the TLD TEB */
    OldTldTeb = LdrpTopLevelDllBeingLoadedTeb;
    LdrpTopLevelDllBeingLoadedTeb = NtCurrentTeb();

    /* Loop */
    i = 0;
    while (i < Count)
    {
        /* Get an entry */
        LdrEntry = LdrRootEntry[i];

        /* FIXME: Verify NX Compat */

        /* Move to next entry */
        i++;

        /* Get its entrypoint */
        EntryPoint = LdrEntry->EntryPoint;

        /* Are we being debugged? */
        BreakOnDllLoad = 0;
        if (Peb->BeingDebugged || Peb->ReadImageFileExecOptions)
        {
            /* Check if we should break on load */
            Status = LdrQueryImageFileExecutionOptions(&LdrEntry->BaseDllName,
                                                       L"BreakOnDllLoad",
                                                       REG_DWORD,
                                                       &BreakOnDllLoad,
                                                       sizeof(ULONG),
                                                       NULL);
            if (!NT_SUCCESS(Status)) BreakOnDllLoad = 0;

            /* Reset status back to STATUS_SUCCESS */
            Status = STATUS_SUCCESS;
        }

        /* Break if aksed */
        if (BreakOnDllLoad)
        {
            /* Check if we should show a message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ loaded.", &LdrEntry->BaseDllName);
                DPRINT1(" - About to call init routine at %p\n", EntryPoint);
            }

            /* Break in debugger */
            DbgBreakPoint();
        }

        /* Make sure we have an entrypoint */
        if (EntryPoint)
        {
            /* Save the old Dll Initializer and write the current one */
            OldInitializer = LdrpCurrentDllInitializer;
            LdrpCurrentDllInitializer = LdrEntry;

            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = 1;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                   LdrEntry->EntryPointActivationContext);

            _SEH2_TRY
            {
                /* Check if it has TLS */
                if (LdrEntry->TlsIndex && Context)
                {
                    /* Call TLS */
                    LdrpCallTlsInitializers(LdrEntry, DLL_PROCESS_ATTACH);
                }

                /* Call the Entrypoint */
                if (ShowSnaps)
                {
                    DPRINT1("%wZ - Calling entry point at %p for DLL_PROCESS_ATTACH\n",
                            &LdrEntry->BaseDllName, EntryPoint);
                }
                DllStatus = LdrpCallInitRoutine(EntryPoint,
                                                LdrEntry->DllBase,
                                                DLL_PROCESS_ATTACH,
                                                Context);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DllStatus = FALSE;
                DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_PROCESS_ATTACH) for %wZ\n",
                        _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
            }
            _SEH2_END;

            /* Deactivate the ActCtx */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);

            /* Save the Current DLL Initializer */
            LdrpCurrentDllInitializer = OldInitializer;

            /* Mark the entry as processed */
            LdrEntry->Flags |= LDRP_PROCESS_ATTACH_CALLED;

            /* Fail if DLL init failed */
            if (!DllStatus)
            {
                DPRINT1("LDR: DLL_PROCESS_ATTACH for dll \"%wZ\" (InitRoutine: %p) failed\n",
                    &LdrEntry->BaseDllName, EntryPoint);

                Status = STATUS_DLL_INIT_FAILED;
                goto Quickie;
            }
        }
    }

    /* Loop in order */
    ListHead = &Peb->Ldr->InInitializationOrderModuleList;
    NextEntry = NextEntry->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the Data Entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        /* FIXME: Verify NX Compat */
        // LdrpCheckNXCompatibility()

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Check for TLS */
    if (LdrpImageHasTls && Context)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        _SEH2_TRY
        {
            /* Do TLS callbacks */
            LdrpCallTlsInitializers(LdrpImageEntry, DLL_PROCESS_ATTACH);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

Quickie:
    /* Restore old TEB */
    LdrpTopLevelDllBeingLoadedTeb = OldTldTeb;

    /* Check if the array is in the heap */
    if (LdrRootEntry != LocalArray)
    {
        /* Free the array */
        RtlFreeHeap(LdrpHeap, 0, LdrRootEntry);
    }

    /* Return to caller */
    DPRINT("LdrpRunInitializeRoutines() done\n");
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrShutdownProcess(VOID)
{
    PPEB Peb = NtCurrentPeb();
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry, ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PVOID EntryPoint;

    DPRINT("LdrShutdownProcess() called for %wZ\n", &LdrpImageEntry->BaseDllName);
    if (LdrpShutdownInProgress) return STATUS_SUCCESS;

    /* Tell the Shim Engine */
    if (g_ShimsEnabled)
    {
        VOID(NTAPI *SE_ProcessDying)();
        SE_ProcessDying = RtlDecodeSystemPointer(g_pfnSE_ProcessDying);
        SE_ProcessDying();
    }

    /* Tell the world */
    if (ShowSnaps)
    {
        DPRINT1("\n");
    }

    /* Set the shutdown variables */
    LdrpShutdownThreadId = NtCurrentTeb()->RealClientId.UniqueThread;
    LdrpShutdownInProgress = TRUE;

    /* Enter the Loader Lock */
    RtlEnterCriticalSection(&LdrpLoaderLock);

    /* Cleanup trace logging data (Etw) */
    if (SharedUserData->TraceLogging)
    {
        /* FIXME */
        DPRINT1("We don't support Etw yet.\n");
    }

    /* Start at the end */
    ListHead = &Peb->Ldr->InInitializationOrderModuleList;
    NextEntry = ListHead->Blink;
    while (NextEntry != ListHead)
    {
        /* Get the current entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        NextEntry = NextEntry->Blink;

        /* Make sure it's not ourselves */
        if (Peb->ImageBaseAddress != LdrEntry->DllBase)
        {
            /* Get the entrypoint */
            EntryPoint = LdrEntry->EntryPoint;

            /* Check if we are ready to call it */
            if (EntryPoint &&
                (LdrEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) &&
                LdrEntry->Flags)
            {
                /* Set up the Act Ctx */
                ActCtx.Size = sizeof(ActCtx);
                ActCtx.Format = 1;
                RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

                /* Activate the ActCtx */
                RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                       LdrEntry->EntryPointActivationContext);

                _SEH2_TRY
                {
                    /* Check if it has TLS */
                    if (LdrEntry->TlsIndex)
                    {
                        /* Call TLS */
                        LdrpCallTlsInitializers(LdrEntry, DLL_PROCESS_DETACH);
                    }

                    /* Call the Entrypoint */
                    DPRINT("%wZ - Calling entry point at %p for thread detaching\n",
                           &LdrEntry->BaseDllName, LdrEntry->EntryPoint);
                    LdrpCallInitRoutine(EntryPoint,
                                        LdrEntry->DllBase,
                                        DLL_PROCESS_DETACH,
                                        (PVOID)1);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_PROCESS_DETACH) for %wZ\n",
                            _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
                }
                _SEH2_END;

                /* Deactivate the ActCtx */
                RtlDeactivateActivationContextUnsafeFast(&ActCtx);
            }
        }
    }

    /* Check for TLS */
    if (LdrpImageHasTls)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        _SEH2_TRY
        {
            /* Do TLS callbacks */
            LdrpCallTlsInitializers(LdrpImageEntry, DLL_PROCESS_DETACH);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* FIXME: Do Heap detection and Etw final shutdown */

    /* Release the lock */
    RtlLeaveCriticalSection(&LdrpLoaderLock);
    DPRINT("LdrpShutdownProcess() done\n");

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrShutdownThread(VOID)
{
    PPEB Peb = NtCurrentPeb();
    PTEB Teb = NtCurrentTeb();
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry, ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PVOID EntryPoint;

    DPRINT("LdrShutdownThread() called for %wZ\n",
            &LdrpImageEntry->BaseDllName);

    /* Cleanup trace logging data (Etw) */
    if (SharedUserData->TraceLogging)
    {
        /* FIXME */
        DPRINT1("We don't support Etw yet.\n");
    }

    /* Get the Ldr Lock */
    RtlEnterCriticalSection(&LdrpLoaderLock);

    /* Start at the end */
    ListHead = &Peb->Ldr->InInitializationOrderModuleList;
    NextEntry = ListHead->Blink;
    while (NextEntry != ListHead)
    {
        /* Get the current entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        NextEntry = NextEntry->Blink;

        /* Make sure it's not ourselves */
        if (Peb->ImageBaseAddress != LdrEntry->DllBase)
        {
            /* Check if we should call */
            if (!(LdrEntry->Flags & LDRP_DONT_CALL_FOR_THREADS) &&
                (LdrEntry->Flags & LDRP_PROCESS_ATTACH_CALLED) &&
                (LdrEntry->Flags & LDRP_IMAGE_DLL))
            {
                /* Get the entrypoint */
                EntryPoint = LdrEntry->EntryPoint;

                /* Check if we are ready to call it */
                if (EntryPoint)
                {
                    /* Set up the Act Ctx */
                    ActCtx.Size = sizeof(ActCtx);
                    ActCtx.Format = 1;
                    RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

                    /* Activate the ActCtx */
                    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                           LdrEntry->EntryPointActivationContext);

                    _SEH2_TRY
                    {
                        /* Check if it has TLS */
                        if (LdrEntry->TlsIndex)
                        {
                            /* Make sure we're not shutting down */
                            if (!LdrpShutdownInProgress)
                            {
                                /* Call TLS */
                                LdrpCallTlsInitializers(LdrEntry, DLL_THREAD_DETACH);
                            }
                        }

                        /* Make sure we're not shutting down */
                        if (!LdrpShutdownInProgress)
                        {
                            /* Call the Entrypoint */
                            DPRINT("%wZ - Calling entry point at %p for thread detaching\n",
                                   &LdrEntry->BaseDllName, LdrEntry->EntryPoint);
                            LdrpCallInitRoutine(EntryPoint,
                                                LdrEntry->DllBase,
                                                DLL_THREAD_DETACH,
                                                NULL);
                        }
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_THREAD_DETACH) for %wZ\n",
                                _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
                    }
                    _SEH2_END;

                    /* Deactivate the ActCtx */
                    RtlDeactivateActivationContextUnsafeFast(&ActCtx);
                }
            }
        }
    }

    /* Check for TLS */
    if (LdrpImageHasTls)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        _SEH2_TRY
        {
            /* Do TLS callbacks */
            LdrpCallTlsInitializers(LdrpImageEntry, DLL_THREAD_DETACH);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do nothing */
        }
        _SEH2_END;

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* Free TLS */
    LdrpFreeTls();
    RtlLeaveCriticalSection(&LdrpLoaderLock);

    /* Check for expansion slots */
    if (Teb->TlsExpansionSlots)
    {
        /* Free expansion slots */
        RtlFreeHeap(RtlGetProcessHeap(), 0, Teb->TlsExpansionSlots);
    }

    /* Check for FLS Data */
    if (Teb->FlsData)
    {
        /* Mimic BaseRundownFls */
        ULONG n, FlsHighIndex;
        PRTL_FLS_DATA pFlsData;
        PFLS_CALLBACK_FUNCTION lpCallback;

        pFlsData = Teb->FlsData;

        RtlAcquirePebLock();
        FlsHighIndex = NtCurrentPeb()->FlsHighIndex;
        RemoveEntryList(&pFlsData->ListEntry);
        RtlReleasePebLock();

        for (n = 1; n <= FlsHighIndex; ++n)
        {
            lpCallback = NtCurrentPeb()->FlsCallback[n];
            if (lpCallback && pFlsData->Data[n])
            {
                lpCallback(pFlsData->Data[n]);
            }
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, pFlsData);
        Teb->FlsData = NULL;
    }

    /* Check for Fiber data */
    if (Teb->HasFiberData)
    {
        /* Free Fiber data*/
        RtlFreeHeap(RtlGetProcessHeap(), 0, Teb->NtTib.FiberData);
        Teb->NtTib.FiberData = NULL;
    }

    /* Free the activation context stack */
    RtlFreeThreadActivationContextStack();
    DPRINT("LdrShutdownThread() done\n");

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpInitializeTls(VOID)
{
    PLIST_ENTRY NextEntry, ListHead;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PLDRP_TLS_DATA TlsData;
    ULONG Size;

    /* Initialize the TLS List */
    InitializeListHead(&LdrpTlsList);

    /* Loop all the modules */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        NextEntry = NextEntry->Flink;

        /* Get the TLS directory */
        TlsDirectory = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_TLS,
                                                    &Size);

        /* Check if we have a directory */
        if (!TlsDirectory) continue;

        /* Check if the image has TLS */
        if (!LdrpImageHasTls) LdrpImageHasTls = TRUE;

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: Tls Found in %wZ at %p\n",
                    &LdrEntry->BaseDllName,
                    TlsDirectory);
        }

        /* Allocate an entry */
        TlsData = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(LDRP_TLS_DATA));
        if (!TlsData) return STATUS_NO_MEMORY;

        /* Lock the DLL and mark it for TLS Usage */
        LdrEntry->LoadCount = -1;
        LdrEntry->TlsIndex = -1;

        /* Save the cached TLS data */
        TlsData->TlsDirectory = *TlsDirectory;
        InsertTailList(&LdrpTlsList, &TlsData->TlsLinks);

        /* Update the index */
        *(PLONG)TlsData->TlsDirectory.AddressOfIndex = LdrpNumberOfTlsEntries;
        TlsData->TlsDirectory.Characteristics = LdrpNumberOfTlsEntries++;
    }

    /* Done setting up TLS, allocate entries */
    return LdrpAllocateTls();
}

NTSTATUS
NTAPI
LdrpAllocateTls(VOID)
{
    PTEB Teb = NtCurrentTeb();
    PLIST_ENTRY NextEntry, ListHead;
    PLDRP_TLS_DATA TlsData;
    SIZE_T TlsDataSize;
    PVOID *TlsVector;

    /* Check if we have any entries */
    if (!LdrpNumberOfTlsEntries)
        return STATUS_SUCCESS;

    /* Allocate the vector array */
    TlsVector = RtlAllocateHeap(RtlGetProcessHeap(),
                                    0,
                                    LdrpNumberOfTlsEntries * sizeof(PVOID));
    if (!TlsVector) return STATUS_NO_MEMORY;
    Teb->ThreadLocalStoragePointer = TlsVector;

    /* Loop the TLS Array */
    ListHead = &LdrpTlsList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry */
        TlsData = CONTAINING_RECORD(NextEntry, LDRP_TLS_DATA, TlsLinks);
        NextEntry = NextEntry->Flink;

        /* Allocate this vector */
        TlsDataSize = TlsData->TlsDirectory.EndAddressOfRawData -
                      TlsData->TlsDirectory.StartAddressOfRawData;
        TlsVector[TlsData->TlsDirectory.Characteristics] = RtlAllocateHeap(RtlGetProcessHeap(),
                                                                           0,
                                                                           TlsDataSize);
        if (!TlsVector[TlsData->TlsDirectory.Characteristics])
        {
            /* Out of memory */
            return STATUS_NO_MEMORY;
        }

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: TlsVector %p Index %lu = %p copied from %x to %p\n",
                    TlsVector,
                    TlsData->TlsDirectory.Characteristics,
                    &TlsVector[TlsData->TlsDirectory.Characteristics],
                    TlsData->TlsDirectory.StartAddressOfRawData,
                    TlsVector[TlsData->TlsDirectory.Characteristics]);
        }

        /* Copy the data */
        RtlCopyMemory(TlsVector[TlsData->TlsDirectory.Characteristics],
                      (PVOID)TlsData->TlsDirectory.StartAddressOfRawData,
                      TlsDataSize);
    }

    /* Done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
LdrpFreeTls(VOID)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDRP_TLS_DATA TlsData;
    PVOID *TlsVector;
    PTEB Teb = NtCurrentTeb();

    /* Get a pointer to the vector array */
    TlsVector = Teb->ThreadLocalStoragePointer;
    if (!TlsVector) return;

    /* Loop through it */
    ListHead = &LdrpTlsList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        TlsData = CONTAINING_RECORD(NextEntry, LDRP_TLS_DATA, TlsLinks);
        NextEntry = NextEntry->Flink;

        /* Free each entry */
        if (TlsVector[TlsData->TlsDirectory.Characteristics])
        {
            RtlFreeHeap(RtlGetProcessHeap(),
                        0,
                        TlsVector[TlsData->TlsDirectory.Characteristics]);
        }
    }

    /* Free the array itself */
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                TlsVector);
}

NTSTATUS
NTAPI
LdrpInitializeExecutionOptions(PUNICODE_STRING ImagePathName, PPEB Peb, PHANDLE OptionsKey)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG ExecuteOptions, MinimumStackCommit = 0, GlobalFlag;

    /* Return error if we were not provided a pointer where to save the options key handle */
    if (!OptionsKey) return STATUS_INVALID_HANDLE;

    /* Zero initialize the options key pointer */
    *OptionsKey = NULL;

    /* Open the options key */
    Status = LdrOpenImageFileOptionsKey(ImagePathName, 0, &KeyHandle);

    /* Save it if it was opened successfully */
    if (NT_SUCCESS(Status))
        *OptionsKey = KeyHandle;

    if (KeyHandle)
    {
        /* There are image specific options, read them starting with NXCOMPAT */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            L"ExecuteOptions",
                                            4,
                                            &ExecuteOptions,
                                            sizeof(ExecuteOptions),
                                            0);

        if (NT_SUCCESS(Status))
        {
            /* TODO: Set execution options for the process */
            /*
            if (ExecuteOptions == 0)
                ExecuteOptions = 1;
            else
                ExecuteOptions = 2;
            ZwSetInformationProcess(NtCurrentProcess(),
                                    ProcessExecuteFlags,
                                    &ExecuteOptions,
                                    sizeof(ULONG));*/

        }

        /* Check if this image uses large pages */
        if (Peb->ImageUsesLargePages)
        {
            /* TODO: If it does, open large page key */
            UNIMPLEMENTED;
        }

        /* Get various option values */
        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"DisableHeapLookaside",
                                   REG_DWORD,
                                   &RtlpDisableHeapLookaside,
                                   sizeof(RtlpDisableHeapLookaside),
                                   NULL);

        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"ShutdownFlags",
                                   REG_DWORD,
                                   &RtlpShutdownProcessFlags,
                                   sizeof(RtlpShutdownProcessFlags),
                                   NULL);

        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"MinimumStackCommitInBytes",
                                   REG_DWORD,
                                   &MinimumStackCommit,
                                   sizeof(MinimumStackCommit),
                                   NULL);

        /* Update PEB's minimum stack commit if it's lower */
        if (Peb->MinimumStackCommit < MinimumStackCommit)
            Peb->MinimumStackCommit = MinimumStackCommit;

        /* Set the global flag */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            L"GlobalFlag",
                                            REG_DWORD,
                                            &GlobalFlag,
                                            sizeof(GlobalFlag),
                                            NULL);

        if (NT_SUCCESS(Status))
            Peb->NtGlobalFlag = GlobalFlag;
        else
            GlobalFlag = 0;

        /* Call AVRF if necessary */
        if (Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))
        {
            Status = LdrpInitializeApplicationVerifierPackage(KeyHandle, Peb, TRUE, FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("AVRF: LdrpInitializeApplicationVerifierPackage failed with %08X\n", Status);
            }
        }
    }
    else
    {
        /* There are no image-specific options, so perform global initialization */
        if (Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))
        {
            /* Initialize app verifier package */
            Status = LdrpInitializeApplicationVerifierPackage(KeyHandle, Peb, TRUE, FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("AVRF: LdrpInitializeApplicationVerifierPackage failed with %08X\n", Status);
            }
        }
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
LdrpValidateImageForMp(IN PLDR_DATA_TABLE_ENTRY LdrDataTableEntry)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
LdrpDisableProcessCompatGuidDetection(VOID)
{
    UNICODE_STRING PolicyKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\Software\\Policies\\Microsoft\\Windows\\AppCompat");
    UNICODE_STRING DisableDetection = RTL_CONSTANT_STRING(L"DisableCompatGuidDetection");
    OBJECT_ATTRIBUTES PolicyKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&PolicyKey, OBJ_CASE_INSENSITIVE);
    KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    ULONG ResultLength;
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &PolicyKeyAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryValueKey(KeyHandle,
                                 &DisableDetection,
                                 KeyValuePartialInformation,
                                 &KeyInfo,
                                 sizeof(KeyInfo),
                                 &ResultLength);
        NtClose(KeyHandle);
        if ((NT_SUCCESS(Status)) &&
            (KeyInfo.Type == REG_DWORD) &&
            (KeyInfo.DataLength == sizeof(ULONG)) &&
            (KeyInfo.Data[0] == TRUE))
        {
            return TRUE;
        }
    }
    return FALSE;
}


VOID
NTAPI
LdrpInitializeProcessCompat(PVOID pProcessActctx, PVOID* pOldShimData)
{
    static const struct
    {
        const GUID* Guid;
        const DWORD Version;
    } KnownCompatGuids[] = {
        { &COMPAT_GUID_WIN10, _WIN32_WINNT_WIN10 },
        { &COMPAT_GUID_WIN81, _WIN32_WINNT_WINBLUE },
        { &COMPAT_GUID_WIN8, _WIN32_WINNT_WIN8 },
        { &COMPAT_GUID_WIN7, _WIN32_WINNT_WIN7 },
        { &COMPAT_GUID_VISTA, _WIN32_WINNT_VISTA },
    };

    ULONG Buffer[(sizeof(COMPATIBILITY_CONTEXT_ELEMENT) * 10 + sizeof(ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION)) / sizeof(ULONG)];
    ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION* ContextCompatInfo;
    SIZE_T SizeRequired;
    NTSTATUS Status;
    DWORD n, cur;
    ReactOS_ShimData* pShimData = *pOldShimData;

    if (pShimData)
    {
        if (pShimData->dwMagic != REACTOS_SHIMDATA_MAGIC ||
            pShimData->dwSize != sizeof(ReactOS_ShimData))
        {
            DPRINT1("LdrpInitializeProcessCompat: Corrupt pShimData (0x%x, %u)\n", pShimData->dwMagic, pShimData->dwSize);
            return;
        }
        if (pShimData->dwRosProcessCompatVersion)
        {
            if (pShimData->dwRosProcessCompatVersion == REACTOS_COMPATVERSION_IGNOREMANIFEST)
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion set to ignore manifest\n");
            }
            else
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion already set to 0x%x\n", pShimData->dwRosProcessCompatVersion);
            }
            return;
        }
    }

    SizeRequired = sizeof(Buffer);
    Status = RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_NO_ADDREF,
                                                  pProcessActctx,
                                                  NULL,
                                                  CompatibilityInformationInActivationContext,
                                                  Buffer,
                                                  sizeof(Buffer),
                                                  &SizeRequired);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrpInitializeProcessCompat: Unable to query process actctx with status %x\n", Status);
        return;
    }

    ContextCompatInfo = (ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION*)Buffer;
    /* No Compatibility elements present, bail out */
    if (ContextCompatInfo->ElementCount == 0)
        return;

    /* Search for known GUIDs, starting from oldest to newest.
       Note that on Windows it is somewhat reversed, starting from the latest known
       version, going down. But we are not Windows, trying to allow a lower version,
       we are ReactOS trying to fake a higher version. So we interpret what Windows
       does as "try the closest version to the actual version", so we start with the
       lowest version, which is closest to Windows 2003, which we mostly are. */
    for (cur = RTL_NUMBER_OF(KnownCompatGuids) - 1; cur != -1; --cur)
    {
        for (n = 0; n < ContextCompatInfo->ElementCount; ++n)
        {
            if (ContextCompatInfo->Elements[n].Type == ACTCTX_COMPATIBILITY_ELEMENT_TYPE_OS &&
                RtlCompareMemory(&ContextCompatInfo->Elements[n].Id, KnownCompatGuids[cur].Guid, sizeof(GUID)) == sizeof(GUID))
            {
                if (LdrpDisableProcessCompatGuidDetection())
                {
                    DPRINT1("LdrpInitializeProcessCompat: Not applying automatic fix for winver 0x%x due to policy\n", KnownCompatGuids[cur].Version);
                    return;
                }

                /* If this process did not need shim data before, allocate and store it */
                if (pShimData == NULL)
                {
                    PPEB Peb = NtCurrentPeb();

                    ASSERT(Peb->pShimData == NULL);
                    pShimData = RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*pShimData));

                    if (!pShimData)
                    {
                        DPRINT1("LdrpInitializeProcessCompat: Unable to allocated %u bytes\n", sizeof(*pShimData));
                        return;
                    }

                    pShimData->dwSize = sizeof(*pShimData);
                    pShimData->dwMagic = REACTOS_SHIMDATA_MAGIC;

                    Peb->pShimData = pShimData;
                    *pOldShimData = pShimData;
                }

                /* Store the lowest found version, and bail out. */
                pShimData->dwRosProcessCompatVersion = KnownCompatGuids[cur].Version;
                DPRINT1("LdrpInitializeProcessCompat: Found guid for winver 0x%x in manifest from %wZ\n",
                        KnownCompatGuids[cur].Version,
                        &(NtCurrentPeb()->ProcessParameters->ImagePathName));
                return;
            }
        }
    }
}

VOID
NTAPI
LdrpInitializeDotLocalSupport(PRTL_USER_PROCESS_PARAMETERS ProcessParameters)
{
    UNICODE_STRING ImagePathName = ProcessParameters->ImagePathName;
    WCHAR LocalBuffer[MAX_PATH];
    UNICODE_STRING DotLocal;
    NTSTATUS Status;
    ULONG RequiredSize;

    RequiredSize = ImagePathName.Length + LdrpDotLocal.Length + sizeof(UNICODE_NULL);
    if (RequiredSize <= sizeof(LocalBuffer))
    {
        RtlInitEmptyUnicodeString(&DotLocal, LocalBuffer, sizeof(LocalBuffer));
    }
    else if (RequiredSize <= UNICODE_STRING_MAX_BYTES)
    {
        DotLocal.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, RequiredSize);
        DotLocal.Length = 0;
        DotLocal.MaximumLength = RequiredSize;
        if (!DotLocal.Buffer)
            DPRINT1("LDR: Failed to allocate memory for .local check\n");
    }
    else
    {
        DotLocal.Buffer = NULL;
        DotLocal.Length = 0;
        DotLocal.MaximumLength = 0;
        DPRINT1("LDR: String too big for .local check\n");
    }

    if (DotLocal.Buffer)
    {
        Status = RtlAppendUnicodeStringToString(&DotLocal, &ImagePathName);
        ASSERT(NT_SUCCESS(Status));
        if (NT_SUCCESS(Status))
        {
            Status = RtlAppendUnicodeStringToString(&DotLocal, &LdrpDotLocal);
            ASSERT(NT_SUCCESS(Status));
        }

        if (NT_SUCCESS(Status))
        {
            if (RtlDoesFileExists_UStr(&DotLocal))
            {
                ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH;
            }
        }
        else
        {
            DPRINT1("LDR: Failed to append: 0x%lx\n", Status);
        }

        if (DotLocal.Buffer != LocalBuffer)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, DotLocal.Buffer);
        }
    }
}


NTSTATUS
NTAPI
LdrpInitializeProcess(IN PCONTEXT Context,
                      IN PVOID SystemArgument1)
{
    RTL_HEAP_PARAMETERS HeapParameters;
    ULONG ComSectionSize;
    ANSI_STRING BaseProcessInitPostImportName = RTL_CONSTANT_STRING("BaseProcessInitPostImport");
    ANSI_STRING BaseQueryModuleDataName = RTL_CONSTANT_STRING("BaseQueryModuleData");
    PVOID OldShimData;
    OBJECT_ATTRIBUTES ObjectAttributes;
    //UNICODE_STRING LocalFileName, FullImageName;
    HANDLE SymLinkHandle;
    //ULONG DebugHeapOnly;
    UNICODE_STRING CommandLine, NtSystemRoot, ImagePathName, FullPath, ImageFileName, KnownDllString;
    PPEB Peb = NtCurrentPeb();
    BOOLEAN IsDotNetImage = FALSE;
    BOOLEAN FreeCurDir = FALSE;
    //HANDLE CompatKey;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    //LPWSTR ImagePathBuffer;
    ULONG ConfigSize;
    UNICODE_STRING CurrentDirectory;
    HANDLE OptionsKey;
    ULONG HeapFlags;
    PIMAGE_NT_HEADERS NtHeader;
    LPWSTR NtDllName = NULL;
    NTSTATUS Status, ImportStatus;
    NLSTABLEINFO NlsTable;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;
    PTEB Teb = NtCurrentTeb();
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    ULONG i;
    PWSTR ImagePath;
    ULONG DebugProcessHeapOnly = 0;
    PLDR_DATA_TABLE_ENTRY NtLdrEntry;
    PWCHAR Current;
    ULONG ExecuteOptions = 0;
    PVOID ViewBase;

    /* Set a NULL SEH Filter */
    RtlSetUnhandledExceptionFilter(NULL);

    /* Get the image path */
    ImagePath = Peb->ProcessParameters->ImagePathName.Buffer;

    /* Check if it's not normalized */
    if (!(Peb->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED))
    {
        /* Normalize it*/
        ImagePath = (PWSTR)((ULONG_PTR)ImagePath + (ULONG_PTR)Peb->ProcessParameters);
    }

    /* Create a unicode string for the Image Path */
    ImagePathName.Length = Peb->ProcessParameters->ImagePathName.Length;
    ImagePathName.MaximumLength = ImagePathName.Length + sizeof(WCHAR);
    ImagePathName.Buffer = ImagePath;

    /* Get the NT Headers */
    NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);

    /* Get the execution options */
    Status = LdrpInitializeExecutionOptions(&ImagePathName, Peb, &OptionsKey);

    /* Check if this is a .NET executable */
    if (RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                     &ComSectionSize))
    {
        /* Remember this for later */
        IsDotNetImage = TRUE;
    }

    /* Save the NTDLL Base address */
    NtDllBase = SystemArgument1;

    /* If this is a Native Image */
    if (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE)
    {
        /* Then do DLL Validation */
        LdrpDllValidation = TRUE;
    }

    /* Save the old Shim Data */
    OldShimData = Peb->pShimData;

    /* ReactOS specific: do not clear it. (Windows starts doing the same in later versions) */
    //Peb->pShimData = NULL;

    /* Save the number of processors and CS timeout */
    LdrpNumberOfProcessors = Peb->NumberOfProcessors;
    RtlpTimeout = Peb->CriticalSectionTimeout;

    /* Normalize the parameters */
    ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters);
    if (ProcessParameters)
    {
        /* Save the Image and Command Line Names */
        ImageFileName = ProcessParameters->ImagePathName;
        CommandLine = ProcessParameters->CommandLine;
    }
    else
    {
        /* It failed, initialize empty strings */
        RtlInitUnicodeString(&ImageFileName, NULL);
        RtlInitUnicodeString(&CommandLine, NULL);
    }

    /* Initialize NLS data */
    RtlInitNlsTables(Peb->AnsiCodePageData,
                     Peb->OemCodePageData,
                     Peb->UnicodeCaseTableData,
                     &NlsTable);

    /* Reset NLS Translations */
    RtlResetRtlTranslations(&NlsTable);

    /* Get the Image Config Directory */
    LoadConfig = RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                              &ConfigSize);

    /* Setup the Heap Parameters */
    RtlZeroMemory(&HeapParameters, sizeof(HeapParameters));
    HeapFlags = HEAP_GROWABLE;
    HeapParameters.Length = sizeof(HeapParameters);

    /* Check if we have Configuration Data */
#define VALID_CONFIG_FIELD(Name) (ConfigSize >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, Name))
    /* The 'original' load config ends after SecurityCookie */
    if ((LoadConfig) && ConfigSize && (VALID_CONFIG_FIELD(SecurityCookie) || ConfigSize == LoadConfig->Size))
    {
        if (ConfigSize != sizeof(IMAGE_LOAD_CONFIG_DIRECTORY))
            DPRINT1("WARN: Accepting different LOAD_CONFIG size!\n");
        else
            DPRINT1("Applying LOAD_CONFIG\n");

        if (VALID_CONFIG_FIELD(GlobalFlagsSet) && LoadConfig->GlobalFlagsSet)
            Peb->NtGlobalFlag |= LoadConfig->GlobalFlagsSet;

        if (VALID_CONFIG_FIELD(GlobalFlagsClear) && LoadConfig->GlobalFlagsClear)
            Peb->NtGlobalFlag &= ~LoadConfig->GlobalFlagsClear;

        /* Convert the default CS timeout from milliseconds to 100ns units */
        if (VALID_CONFIG_FIELD(CriticalSectionDefaultTimeout) && LoadConfig->CriticalSectionDefaultTimeout)
            RtlpTimeout.QuadPart = Int32x32To64(LoadConfig->CriticalSectionDefaultTimeout, -10000);

        if (VALID_CONFIG_FIELD(DeCommitFreeBlockThreshold) && LoadConfig->DeCommitFreeBlockThreshold)
            HeapParameters.DeCommitFreeBlockThreshold = LoadConfig->DeCommitFreeBlockThreshold;

        if (VALID_CONFIG_FIELD(DeCommitTotalFreeThreshold) && LoadConfig->DeCommitTotalFreeThreshold)
            HeapParameters.DeCommitTotalFreeThreshold = LoadConfig->DeCommitTotalFreeThreshold;

        if (VALID_CONFIG_FIELD(MaximumAllocationSize) && LoadConfig->MaximumAllocationSize)
            HeapParameters.MaximumAllocationSize = LoadConfig->MaximumAllocationSize;

        if (VALID_CONFIG_FIELD(VirtualMemoryThreshold) && LoadConfig->VirtualMemoryThreshold)
            HeapParameters.VirtualMemoryThreshold = LoadConfig->VirtualMemoryThreshold;

        if (VALID_CONFIG_FIELD(ProcessHeapFlags) && LoadConfig->ProcessHeapFlags)
            HeapFlags = LoadConfig->ProcessHeapFlags;
    }
#undef VALID_CONFIG_FIELD

    /* Check for custom affinity mask */
    if (Peb->ImageProcessAffinityMask)
    {
        /* Set it */
        Status = NtSetInformationProcess(NtCurrentProcess(),
                                         ProcessAffinityMask,
                                         &Peb->ImageProcessAffinityMask,
                                         sizeof(Peb->ImageProcessAffinityMask));
    }

    /* Check if verbose debugging (ShowSnaps) was requested */
    ShowSnaps = Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS;

    /* Start verbose debugging messages right now if they were requested */
    if (ShowSnaps)
    {
        DPRINT1("LDR: PID: 0x%p started - '%wZ'\n",
                Teb->ClientId.UniqueProcess,
                &CommandLine);
    }

    /* If the CS timeout is longer than 1 hour, disable it */
    if (RtlpTimeout.QuadPart < Int32x32To64(3600, -10000000))
        RtlpTimeoutDisable = TRUE;

    /* Initialize Critical Section Data */
    RtlpInitDeferredCriticalSection();

    /* Initialize VEH Call lists */
    RtlpInitializeVectoredExceptionHandling();

    /* Set TLS/FLS Bitmap data */
    Peb->FlsBitmap = &FlsBitMap;
    Peb->TlsBitmap = &TlsBitMap;
    Peb->TlsExpansionBitmap = &TlsExpansionBitMap;

    /* Initialize FLS Bitmap */
    RtlInitializeBitMap(&FlsBitMap,
                        Peb->FlsBitmapBits,
                        FLS_MAXIMUM_AVAILABLE);
    RtlSetBit(&FlsBitMap, 0);
    InitializeListHead(&Peb->FlsListHead);

    /* Initialize TLS Bitmap */
    RtlInitializeBitMap(&TlsBitMap,
                        Peb->TlsBitmapBits,
                        TLS_MINIMUM_AVAILABLE);
    RtlSetBit(&TlsBitMap, 0);
    RtlInitializeBitMap(&TlsExpansionBitMap,
                        Peb->TlsExpansionBitmapBits,
                        TLS_EXPANSION_SLOTS);
    RtlSetBit(&TlsExpansionBitMap, 0);

    /* Initialize the Hash Table */
    for (i = 0; i < LDR_HASH_TABLE_ENTRIES; i++)
    {
        InitializeListHead(&LdrpHashTable[i]);
    }

    /* Initialize the Loader Lock */
    // FIXME: What's the point of initing it manually, if two lines lower
    //        a call to RtlInitializeCriticalSection() is being made anyway?
    //InsertTailList(&RtlCriticalSectionList, &LdrpLoaderLock.DebugInfo->ProcessLocksList);
    //LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;
    RtlInitializeCriticalSection(&LdrpLoaderLock);
    LdrpLoaderLockInit = TRUE;

    /* Check if User Stack Trace Database support was requested */
    if (Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB)
    {
        DPRINT1("We don't support user stack trace databases yet\n");
    }

    /* Setup Fast PEB Lock */
    RtlInitializeCriticalSection(&FastPebLock);
    Peb->FastPebLock = &FastPebLock;
    //Peb->FastPebLockRoutine = (PPEBLOCKROUTINE)RtlEnterCriticalSection;
    //Peb->FastPebUnlockRoutine = (PPEBLOCKROUTINE)RtlLeaveCriticalSection;

    /* Setup Callout Lock and Notification list */
    //RtlInitializeCriticalSection(&RtlpCalloutEntryLock);
    InitializeListHead(&LdrpDllNotificationList);

    /* For old executables, use 16-byte aligned heap */
    if ((NtHeader->OptionalHeader.MajorSubsystemVersion <= 3) &&
        (NtHeader->OptionalHeader.MinorSubsystemVersion < 51))
    {
        HeapFlags |= HEAP_CREATE_ALIGN_16;
    }

    /* Setup the Heap */
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap(HeapFlags,
                                     NULL,
                                     NtHeader->OptionalHeader.SizeOfHeapReserve,
                                     NtHeader->OptionalHeader.SizeOfHeapCommit,
                                     NULL,
                                     &HeapParameters);

    if (!Peb->ProcessHeap)
    {
        DPRINT1("Failed to create process heap\n");
        return STATUS_NO_MEMORY;
    }

    /* Allocate an Activation Context Stack */
    Status = RtlAllocateActivationContextStack(&Teb->ActivationContextStackPointer);
    if (!NT_SUCCESS(Status)) return Status;

    RtlZeroMemory(&HeapParameters, sizeof(HeapParameters));
    HeapFlags = HEAP_GROWABLE | HEAP_CLASS_1;
    HeapParameters.Length = sizeof(HeapParameters);
    LdrpHeap = RtlCreateHeap(HeapFlags, 0, 0x10000, 0x6000, 0, &HeapParameters);
    if (!LdrpHeap)
    {
        DPRINT1("Failed to create loader private heap\n");
        return STATUS_NO_MEMORY;
    }

    /* Check for Debug Heap */
    if (OptionsKey)
    {
        /* Query the setting */
        Status = LdrQueryImageFileKeyOption(OptionsKey,
                                            L"DebugProcessHeapOnly",
                                            REG_DWORD,
                                            &DebugProcessHeapOnly,
                                            sizeof(ULONG),
                                            NULL);

        if (NT_SUCCESS(Status))
        {
            /* Reset DPH if requested */
            if (RtlpPageHeapEnabled && DebugProcessHeapOnly)
            {
                RtlpDphGlobalFlags &= ~DPH_FLAG_DLL_NOTIFY;
                RtlpPageHeapEnabled = FALSE;
            }
        }
    }

    /* Build the NTDLL Path */
    FullPath.Buffer = StringBuffer;
    FullPath.Length = 0;
    FullPath.MaximumLength = sizeof(StringBuffer);
    RtlInitUnicodeString(&NtSystemRoot, SharedUserData->NtSystemRoot);
    RtlAppendUnicodeStringToString(&FullPath, &NtSystemRoot);
    RtlAppendUnicodeToString(&FullPath, L"\\System32\\");

    /* Open the Known DLLs directory */
    RtlInitUnicodeString(&KnownDllString, L"\\KnownDlls");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KnownDllString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&LdrpKnownDllObjectDirectory,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);

    /* Check if it exists */
    if (NT_SUCCESS(Status))
    {
        /* Open the Known DLLs Path */
        RtlInitUnicodeString(&KnownDllString, L"KnownDllPath");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KnownDllString,
                                   OBJ_CASE_INSENSITIVE,
                                   LdrpKnownDllObjectDirectory,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&SymLinkHandle,
                                          SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Query the path */
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            Status = ZwQuerySymbolicLinkObject(SymLinkHandle, &LdrpKnownDllPath, NULL);
            NtClose(SymLinkHandle);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("LDR: %s - failed call to ZwQuerySymbolicLinkObject with status %x\n", "", Status);
                return Status;
            }
        }
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Assume System32 */
        LdrpKnownDllObjectDirectory = NULL;
        RtlInitUnicodeString(&LdrpKnownDllPath, StringBuffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);
    }

    /* If we have process parameters, get the default path and current path */
    if (ProcessParameters)
    {
        /* Check if we have a Dll Path */
        if (ProcessParameters->DllPath.Length)
        {
            /* Get the path */
            LdrpDefaultPath = *(PUNICODE_STRING)&ProcessParameters->DllPath;
        }
        else
        {
            /* We need a valid path */
            DPRINT1("No valid DllPath was given!\n");
            LdrpInitFailure(STATUS_INVALID_PARAMETER);
        }

        /* Set the current directory */
        CurrentDirectory = ProcessParameters->CurrentDirectory.DosPath;

        /* Check if it's empty or invalid */
        if ((!CurrentDirectory.Buffer) ||
            (CurrentDirectory.Buffer[0] == UNICODE_NULL) ||
            (!CurrentDirectory.Length))
        {
            /* Allocate space for the buffer */
            CurrentDirectory.Buffer = RtlAllocateHeap(Peb->ProcessHeap,
                                                      0,
                                                      3 * sizeof(WCHAR) +
                                                      sizeof(UNICODE_NULL));
            if (!CurrentDirectory.Buffer)
            {
                DPRINT1("LDR: LdrpInitializeProcess - unable to allocate current working directory buffer\n");
                // FIXME: And what?
            }

            /* Copy the drive of the system root */
            RtlMoveMemory(CurrentDirectory.Buffer,
                          SharedUserData->NtSystemRoot,
                          3 * sizeof(WCHAR));
            CurrentDirectory.Buffer[3] = UNICODE_NULL;
            CurrentDirectory.Length = 3 * sizeof(WCHAR);
            CurrentDirectory.MaximumLength = CurrentDirectory.Length + sizeof(WCHAR);

            FreeCurDir = TRUE;
            DPRINT("Using dynamically allocd curdir\n");
        }
        else
        {
            /* Use the local buffer */
            DPRINT("Using local system root\n");
        }
    }

    /* Setup Loader Data */
    Peb->Ldr = &PebLdr;
    InitializeListHead(&PebLdr.InLoadOrderModuleList);
    InitializeListHead(&PebLdr.InMemoryOrderModuleList);
    InitializeListHead(&PebLdr.InInitializationOrderModuleList);
    PebLdr.Length = sizeof(PEB_LDR_DATA);
    PebLdr.Initialized = TRUE;

    /* Allocate a data entry for the Image */
    LdrpImageEntry = LdrpAllocateDataTableEntry(Peb->ImageBaseAddress);

    /* Set it up */
    LdrpImageEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrpImageEntry->DllBase);
    LdrpImageEntry->LoadCount = -1;
    LdrpImageEntry->EntryPointActivationContext = 0;
    LdrpImageEntry->FullDllName = ImageFileName;

    if (IsDotNetImage)
        LdrpImageEntry->Flags = LDRP_COR_IMAGE;
    else
        LdrpImageEntry->Flags = 0;

    /* Check if the name is empty */
    if (!ImageFileName.Buffer[0])
    {
        /* Use the same Base name */
        LdrpImageEntry->BaseDllName = LdrpImageEntry->FullDllName;
    }
    else
    {
        /* Find the last slash */
        Current = ImageFileName.Buffer;
        while (*Current)
        {
            if (*Current++ == '\\')
            {
                /* Set this path */
                NtDllName = Current;
            }
        }

        /* Did we find anything? */
        if (!NtDllName)
        {
            /* Use the same Base name */
            LdrpImageEntry->BaseDllName = LdrpImageEntry->FullDllName;
        }
        else
        {
            /* Setup the name */
            LdrpImageEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)ImageFileName.Buffer + ImageFileName.Length - (ULONG_PTR)NtDllName);
            LdrpImageEntry->BaseDllName.MaximumLength = LdrpImageEntry->BaseDllName.Length + sizeof(WCHAR);
            LdrpImageEntry->BaseDllName.Buffer = (PWSTR)((ULONG_PTR)ImageFileName.Buffer +
                                                     (ImageFileName.Length - LdrpImageEntry->BaseDllName.Length));
        }
    }

    /* Processing done, insert it */
    LdrpInsertMemoryTableEntry(LdrpImageEntry);
    LdrpImageEntry->Flags |= LDRP_ENTRY_PROCESSED;

    /* Now add an entry for NTDLL */
    NtLdrEntry = LdrpAllocateDataTableEntry(SystemArgument1);
    NtLdrEntry->Flags = LDRP_IMAGE_DLL;
    NtLdrEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(NtLdrEntry->DllBase);
    NtLdrEntry->LoadCount = -1;
    NtLdrEntry->EntryPointActivationContext = 0;

    NtLdrEntry->FullDllName.Length = FullPath.Length;
    NtLdrEntry->FullDllName.MaximumLength = FullPath.MaximumLength;
    NtLdrEntry->FullDllName.Buffer = StringBuffer;
    RtlAppendUnicodeStringToString(&NtLdrEntry->FullDllName, &NtDllString);

    NtLdrEntry->BaseDllName.Length = NtDllString.Length;
    NtLdrEntry->BaseDllName.MaximumLength = NtDllString.MaximumLength;
    NtLdrEntry->BaseDllName.Buffer = NtDllString.Buffer;

    /* Processing done, insert it */
    LdrpNtDllDataTableEntry = NtLdrEntry;
    LdrpInsertMemoryTableEntry(NtLdrEntry);

    /* Let the world know */
    if (ShowSnaps)
    {
        DPRINT1("LDR: NEW PROCESS\n");
        DPRINT1("     Image Path: %wZ (%wZ)\n", &LdrpImageEntry->FullDllName, &LdrpImageEntry->BaseDllName);
        DPRINT1("     Current Directory: %wZ\n", &CurrentDirectory);
        DPRINT1("     Search Path: %wZ\n", &LdrpDefaultPath);
    }

    /* Link the Init Order List */
    InsertHeadList(&Peb->Ldr->InInitializationOrderModuleList,
                   &LdrpNtDllDataTableEntry->InInitializationOrderLinks);

    /* Initialize Wine's active context implementation for the current process */
    RtlpInitializeActCtx(&OldShimData);

    /* Set the current directory */
    Status = RtlSetCurrentDirectory_U(&CurrentDirectory);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, check if we should free it */
        if (FreeCurDir) RtlFreeUnicodeString(&CurrentDirectory);

        /* Set it to the NT Root */
        CurrentDirectory = NtSystemRoot;
        RtlSetCurrentDirectory_U(&CurrentDirectory);
    }
    else
    {
        /* We're done with it, free it */
        if (FreeCurDir) RtlFreeUnicodeString(&CurrentDirectory);
    }

    /* Check if we should look for a .local file */
    if (ProcessParameters && !(ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_LOCAL_DLL_PATH))
    {
        LdrpInitializeDotLocalSupport(ProcessParameters);
    }

    /* Check if the Application Verifier was enabled */
    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
    {
        Status = AVrfInitializeVerifier();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: AVrfInitializeVerifier failed (ntstatus 0x%x)\n", Status);
            return Status;
        }

    }

    if (IsDotNetImage)
    {
        /* FIXME */
        DPRINT1("We don't support .NET applications yet\n");
    }

    if (NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ||
        NtHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        PVOID Kernel32BaseAddress;
        PVOID FunctionAddress;

        Status = LdrLoadDll(NULL, NULL, &Kernel32String, &Kernel32BaseAddress);

        if (!NT_SUCCESS(Status))
        {
            if (ShowSnaps)
                DPRINT1("LDR: Unable to load %wZ, Status=0x%08lx\n", &Kernel32String, Status);
            return Status;
        }

        Status = LdrGetProcedureAddress(Kernel32BaseAddress,
                                        &BaseProcessInitPostImportName,
                                        0,
                                        &FunctionAddress);

        if (!NT_SUCCESS(Status))
        {
            if (ShowSnaps)
                DPRINT1("LDR: Unable to find post-import process init function, Status=0x%08lx\n", Status);
            return Status;
        }
        Kernel32ProcessInitPostImportFunction = FunctionAddress;

        Status = LdrGetProcedureAddress(Kernel32BaseAddress,
                                        &BaseQueryModuleDataName,
                                        0,
                                        &FunctionAddress);

        if (!NT_SUCCESS(Status))
        {
            if (ShowSnaps)
                DPRINT1("LDR: Unable to find BaseQueryModuleData, Status=0x%08lx\n", Status);
            return Status;
        }
        Kernel32BaseQueryModuleData = FunctionAddress;
    }

    /* Walk the IAT and load all the DLLs */
    ImportStatus = LdrpWalkImportDescriptor(LdrpDefaultPath.Buffer, LdrpImageEntry);

    /* Check if relocation is needed */
    if (Peb->ImageBaseAddress != (PVOID)NtHeader->OptionalHeader.ImageBase)
    {
        DPRINT1("LDR: Performing EXE relocation\n");

        /* Change the protection to prepare for relocation */
        ViewBase = Peb->ImageBaseAddress;
        Status = LdrpSetProtection(ViewBase, FALSE);
        if (!NT_SUCCESS(Status)) return Status;

        /* Do the relocation */
        Status = LdrRelocateImageWithBias(ViewBase,
                                          0LL,
                                          NULL,
                                          STATUS_SUCCESS,
                                          STATUS_CONFLICTING_ADDRESSES,
                                          STATUS_INVALID_IMAGE_FORMAT);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LdrRelocateImageWithBias() failed\n");
            return Status;
        }

        /* Check if a start context was provided */
        if (Context)
        {
            DPRINT1("WARNING: Relocated EXE Context");
            UNIMPLEMENTED; // We should support this
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        /* Restore the protection */
        Status = LdrpSetProtection(ViewBase, TRUE);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Lock the DLLs */
    ListHead = &Peb->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        NtLdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        NtLdrEntry->LoadCount = -1;
        NextEntry = NextEntry->Flink;
    }

    /* Phase 0 is done */
    LdrpLdrDatabaseIsSetup = TRUE;

    /* Check whether all static imports were properly loaded and return here */
    if (!NT_SUCCESS(ImportStatus)) return ImportStatus;

#if (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)
    /* Initialize the keyed event for condition variables */
    RtlpInitializeKeyedEvent();
#endif

    /* Initialize TLS */
    Status = LdrpInitializeTls();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: LdrpProcessInitialization failed to initialize TLS slots; status %x\n",
                Status);
        return Status;
    }

    /* FIXME Mark the DLL Ranges for Stack Traces later */

    /* Notify the debugger now */
    if (Peb->BeingDebugged)
    {
        /* Break */
        DbgBreakPoint();

        /* Update show snaps again */
        ShowSnaps = Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS;
    }

    /* Validate the Image for MP Usage */
    if (LdrpNumberOfProcessors > 1) LdrpValidateImageForMp(LdrpImageEntry);

    /* Check NX options and set them */
    if (SharedUserData->NXSupportPolicy == NX_SUPPORT_POLICY_ALWAYSON)
    {
        ExecuteOptions = MEM_EXECUTE_OPTION_DISABLE |
                         MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION |
                         MEM_EXECUTE_OPTION_PERMANENT;
    }
    else if (SharedUserData->NXSupportPolicy == NX_SUPPORT_POLICY_ALWAYSOFF)
    {
        ExecuteOptions = MEM_EXECUTE_OPTION_ENABLE | MEM_EXECUTE_OPTION_PERMANENT;
    }
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessExecuteFlags,
                                     &ExecuteOptions,
                                     sizeof(ExecuteOptions));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: Could not set process execute flags 0x%x; status %x\n",
                ExecuteOptions, Status);
    }

    // FIXME: Should be done by Application Compatibility features,
    // by reading the registry, etc...
    // For now, this is the old code from ntdll!RtlGetVersion().
    RtlInitEmptyUnicodeString(&Peb->CSDVersion, NULL, 0);
    if (((Peb->OSCSDVersion >> 8) & 0xFF) != 0)
    {
        WCHAR szCSDVersion[128];
        LONG i;
        USHORT Length = (USHORT)ARRAYSIZE(szCSDVersion) - 1;
        i = _snwprintf(szCSDVersion, Length,
                       L"Service Pack %d",
                       ((Peb->OSCSDVersion >> 8) & 0xFF));
        if (i < 0)
        {
            /* Null-terminate if it was overflowed */
            szCSDVersion[Length] = UNICODE_NULL;
        }

        Length *= sizeof(WCHAR);
        Peb->CSDVersion.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                 0,
                                                 Length + sizeof(UNICODE_NULL));
        if (Peb->CSDVersion.Buffer)
        {
            Peb->CSDVersion.Length = Length;
            Peb->CSDVersion.MaximumLength = Length + sizeof(UNICODE_NULL);

            RtlCopyMemory(Peb->CSDVersion.Buffer,
                          szCSDVersion,
                          Peb->CSDVersion.MaximumLength);
            Peb->CSDVersion.Buffer[Peb->CSDVersion.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    /* Check if we had Shim Data */
    if (OldShimData)
    {
        /* Load the Shim Engine */
        Peb->AppCompatInfo = NULL;
        LdrpLoadShimEngine(OldShimData, &ImagePathName, OldShimData);
    }
    else
    {
        /* Check for Application Compatibility Goo */
        //LdrQueryApplicationCompatibilityGoo(hKey);
        DPRINT("Querying app compat hacks is missing!\n");
    }

    /*
     * FIXME: Check for special images, SecuROM, SafeDisc and other NX-
     * incompatible images.
     */

    /* Now call the Init Routines */
    Status = LdrpRunInitializeRoutines(Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: LdrpProcessInitialization failed running initialization routines; status %x\n",
                Status);
        return Status;
    }

    /* Notify Shim Engine */
    if (g_ShimsEnabled)
    {
        VOID(NTAPI *SE_InstallAfterInit)(PUNICODE_STRING, PVOID);
        SE_InstallAfterInit = RtlDecodeSystemPointer(g_pfnSE_InstallAfterInit);
        SE_InstallAfterInit(&ImagePathName, OldShimData);
    }

    /* Check if we have a user-defined Post Process Routine */
    if (NT_SUCCESS(Status) && Peb->PostProcessInitRoutine)
    {
        /* Call it */
        Peb->PostProcessInitRoutine();
    }

    /* Close the key if we have one opened */
    if (OptionsKey) NtClose(OptionsKey);

    /* Return status */
    return Status;
}

VOID
NTAPI
LdrpInitFailure(NTSTATUS Status)
{
    ULONG Response;
    PPEB Peb = NtCurrentPeb();

    /* Print a debug message */
    DPRINT1("LDR: Process initialization failure for %wZ; NTSTATUS = %08lx\n",
            &Peb->ProcessParameters->ImagePathName, Status);

    /* Raise a hard error */
    if (!LdrpFatalHardErrorCount)
    {
        ZwRaiseHardError(STATUS_APP_INIT_FAILURE, 1, 0, (PULONG_PTR)&Status, OptionOk, &Response);
    }
}

VOID
NTAPI
LdrpInit(PCONTEXT Context,
         PVOID SystemArgument1,
         PVOID SystemArgument2)
{
    LARGE_INTEGER Timeout;
    PTEB Teb = NtCurrentTeb();
    NTSTATUS Status, LoaderStatus = STATUS_SUCCESS;
    MEMORY_BASIC_INFORMATION MemoryBasicInfo;
    PPEB Peb = NtCurrentPeb();

    DPRINT("LdrpInit() %p/%p\n",
        NtCurrentTeb()->RealClientId.UniqueProcess,
        NtCurrentTeb()->RealClientId.UniqueThread);

#ifdef _WIN64
    /* Set the SList header usage */
    RtlpUse16ByteSLists = SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE128];
#endif /* _WIN64 */

    /* Check if we have a deallocation stack */
    if (!Teb->DeallocationStack)
    {
        /* We don't, set one */
        Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                      Teb->NtTib.StackLimit,
                                      MemoryBasicInformation,
                                      &MemoryBasicInfo,
                                      sizeof(MEMORY_BASIC_INFORMATION),
                                      NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            LdrpInitFailure(Status);
            RtlRaiseStatus(Status);
            return;
        }

        /* Set the stack */
        Teb->DeallocationStack = MemoryBasicInfo.AllocationBase;
    }

    /* Now check if the process is already being initialized */
    while (_InterlockedCompareExchange(&LdrpProcessInitialized,
                                      1,
                                      0) == 1)
    {
        /* Set the timeout to 30 milliseconds */
        Timeout.QuadPart = Int32x32To64(30, -10000);

        /* Make sure the status hasn't changed */
        while (LdrpProcessInitialized == 1)
        {
            /* Do the wait */
            ZwDelayExecution(FALSE, &Timeout);
        }
    }

    /* Check if we have already setup LDR data */
    if (!Peb->Ldr)
    {
        /* Setup the Loader Lock */
        Peb->LoaderLock = &LdrpLoaderLock;

        /* Let other code know we're initializing */
        LdrpInLdrInit = TRUE;

        /* Protect with SEH */
        _SEH2_TRY
        {
            /* Initialize the Process */
            LoaderStatus = LdrpInitializeProcess(Context,
                                                 SystemArgument1);

            /* Check for success and if MinimumStackCommit was requested */
            if (NT_SUCCESS(LoaderStatus) && Peb->MinimumStackCommit)
            {
                /* Enforce the limit */
                //LdrpTouchThreadStack(Peb->MinimumStackCommit);
                UNIMPLEMENTED;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Fail with the SEH error */
            LoaderStatus = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* We're not initializing anymore */
        LdrpInLdrInit = FALSE;

        /* Check if init worked */
        if (NT_SUCCESS(LoaderStatus))
        {
            /* Set the process as Initialized */
            _InterlockedIncrement(&LdrpProcessInitialized);
        }
    }
    else
    {
        /* Loader data is there... is this a fork() ? */
        if(Peb->InheritedAddressSpace)
        {
            /* Handle the fork() */
            //LoaderStatus = LdrpForkProcess();
            LoaderStatus = STATUS_NOT_IMPLEMENTED;
            UNIMPLEMENTED;
        }
        else
        {
            /* This is a new thread initializing */
            LdrpInitializeThread(Context);
        }
    }

    /* All done, test alert the thread */
    NtTestAlert();

    /* Return */
    if (!NT_SUCCESS(LoaderStatus))
    {
        /* Fail */
        LdrpInitFailure(LoaderStatus);
        RtlRaiseStatus(LoaderStatus);
    }
}

/* EOF */
