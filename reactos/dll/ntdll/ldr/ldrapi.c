/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User Mode Library
 * FILE:            dll/ntdll/ldr/ldrapi.c
 * PURPOSE:         PE Loader Public APIs
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LONG LdrpLoaderLockAcquisitonCount;
BOOLEAN LdrpShowRecursiveLoads;
UNICODE_STRING LdrApiDefaultExtension = RTL_CONSTANT_STRING(L".DLL");

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrUnlockLoaderLock(IN ULONG Flags,
                    IN ULONG Cookie OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("LdrUnlockLoaderLock(%x %x)\n", Flags, Cookie);

    /* Check for valid flags */
    if (Flags & ~1)
    {
        /* Flags are invalid, check how to fail */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
        }
        else
        {
           /* A normal failure */
            return STATUS_INVALID_PARAMETER_1;
        }
    }

    /* If we don't have a cookie, just return */
    if (!Cookie) return STATUS_SUCCESS;

    /* Validate the cookie */
    if ((Cookie & 0xF0000000) ||
        ((Cookie >> 16) ^ ((ULONG)(NtCurrentTeb()->RealClientId.UniqueThread) & 0xFFF)))
    {
        DPRINT1("LdrUnlockLoaderLock() called with an invalid cookie!\n");

        /* Invalid cookie, check how to fail */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
        }
        else
        {
            /* A normal failure */
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    /* Ready to release the lock */
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
    {
        /* Do a direct leave */
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }
    else
    {
        /* Wrap this in SEH, since we're not supposed to raise */
        _SEH2_TRY
        {
            /* Leave the lock */
            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* We should use the LDR Filter instead */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* All done */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLockLoaderLock(IN ULONG Flags,
                  OUT PULONG Result OPTIONAL,
                  OUT PULONG Cookie OPTIONAL)
{
    LONG OldCount;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN InInit = LdrpInLdrInit;

    DPRINT("LdrLockLoaderLock(%x %p %p)\n", Flags, Result, Cookie);

    /* Zero out the outputs */
    if (Result) *Result = 0;
    if (Cookie) *Cookie = 0;

    /* Validate the flags */
    if (Flags & ~(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS |
                  LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY))
    {
        /* Flags are invalid, check how to fail */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
        }

        /* A normal failure */
        return STATUS_INVALID_PARAMETER_1;
    }

    /* Make sure we got a cookie */
    if (!Cookie)
    {
        /* No cookie check how to fail */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_3);
        }

        /* A normal failure */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* If the flag is set, make sure we have a valid pointer to use */
    if ((Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY) && !(Result))
    {
        /* No pointer to return the data to */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
        }

        /* Fail */
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Return now if we are in the init phase */
    if (InInit) return STATUS_SUCCESS;

    /* Check what locking semantic to use */
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
    {
        /* Check if we should enter or simply try */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
        {
            /* Do a try */
            if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
            {
                /* It's locked */
                *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
                goto Quickie;
            }
            else
            {
                /* It worked */
                *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
            }
        }
        else
        {
            /* Do a enter */
            RtlEnterCriticalSection(&LdrpLoaderLock);

            /* See if result was requested */
            if (Result) *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
        }

        /* Increase the acquisition count */
        OldCount = _InterlockedIncrement(&LdrpLoaderLockAcquisitonCount);

        /* Generate a cookie */
        *Cookie = (((ULONG)NtCurrentTeb()->RealClientId.UniqueThread & 0xFFF) << 16) | OldCount;
    }
    else
    {
        /* Wrap this in SEH, since we're not supposed to raise */
        _SEH2_TRY
        {
            /* Check if we should enter or simply try */
            if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
            {
                /* Do a try */
                if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
                {
                    /* It's locked */
                    *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
                    _SEH2_YIELD(return STATUS_SUCCESS);
                }
                else
                {
                    /* It worked */
                    *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                }
            }
            else
            {
                /* Do an enter */
                RtlEnterCriticalSection(&LdrpLoaderLock);

                /* See if result was requested */
                if (Result) *Result = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
            }

            /* Increase the acquisition count */
            OldCount = _InterlockedIncrement(&LdrpLoaderLockAcquisitonCount);

            /* Generate a cookie */
            *Cookie = (((ULONG)NtCurrentTeb()->RealClientId.UniqueThread & 0xFFF) << 16) | OldCount;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* We should use the LDR Filter instead */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

Quickie:
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLoadDll(IN PWSTR SearchPath OPTIONAL,
           IN PULONG DllCharacteristics OPTIONAL,
           IN PUNICODE_STRING DllName,
           OUT PVOID *BaseAddress)
{
    WCHAR StringBuffer[MAX_PATH];
    UNICODE_STRING DllString1, DllString2;
    BOOLEAN RedirectedDll = FALSE;
    NTSTATUS Status;
    ULONG Cookie;
    PUNICODE_STRING OldTldDll;
    PTEB Teb = NtCurrentTeb();

    /* Initialize the strings */
    RtlInitUnicodeString(&DllString2, NULL);
    DllString1.Buffer = StringBuffer;
    DllString1.Length = 0;
    DllString1.MaximumLength = sizeof(StringBuffer);

    /* Check if the SxS Assemblies specify another file */
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      DllName,
                                                      &LdrApiDefaultExtension,
                                                      &DllString1,
                                                      &DllString2,
                                                      &DllName,
                                                      NULL,
                                                      NULL,
                                                      NULL);

    /* Check success */
    if (NT_SUCCESS(Status))
    {
        /* Let Ldrp know */
        RedirectedDll = TRUE;
    }
    else if (Status != STATUS_SXS_KEY_NOT_FOUND)
    {
        /* Unrecoverable SxS failure; did we get a string? */
        if (DllString2.Buffer)
        {
            /* Free the string */
            RtlFreeUnicodeString(&DllString2);
            return Status;
        }
    }

    /* Lock the loader lock */
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &Cookie);

    /* Check if there's a TLD DLL being loaded */
    if ((OldTldDll = LdrpTopLevelDllBeingLoaded))
    {
        /* This is a recursive load, do something about it? */
        if (ShowSnaps || LdrpShowRecursiveLoads)
        {
            /* Print out debug messages */
            DPRINT1("[%lx, %lx] LDR: Recursive DLL Load\n",
                    Teb->RealClientId.UniqueProcess,
                    Teb->RealClientId.UniqueThread);
            DPRINT1("[%lx, %lx]      Previous DLL being loaded \"%wZ\"\n",
                    Teb->RealClientId.UniqueProcess,
                    Teb->RealClientId.UniqueThread,
                    OldTldDll);
            DPRINT1("[%lx, %lx]      DLL being requested \"%wZ\"\n",
                    Teb->RealClientId.UniqueProcess,
                    Teb->RealClientId.UniqueThread,
                    DllName);

            /* Was it initializing too? */
            if (!LdrpCurrentDllInitializer)
            {
                DPRINT1("[%lx, %lx] LDR: No DLL Initializer was running\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread);
            }
            else
            {
                DPRINT1("[%lx, %lx]      DLL whose initializer was currently running \"%wZ\"\n",
                        Teb->ClientId.UniqueProcess,
                        Teb->ClientId.UniqueThread,
                        &LdrpCurrentDllInitializer->BaseDllName);
            }
        }
    }

    /* Set this one as the TLD DLL being loaded*/
    LdrpTopLevelDllBeingLoaded = DllName;

    /* Load the DLL */
    Status = LdrpLoadDll(RedirectedDll,
                         SearchPath,
                         DllCharacteristics,
                         DllName,
                         BaseAddress,
                         TRUE);

    /* Set it to success just to be sure */
    Status = STATUS_SUCCESS;

    /* Restore the old TLD DLL */
    LdrpTopLevelDllBeingLoaded = OldTldDll;

    /* Release the lock */
    LdrUnlockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);

    /* Do we have a redirect string? */
    if (DllString2.Buffer) RtlFreeUnicodeString(&DllString2);

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrFindEntryForAddress(PVOID Address,
                       PLDR_DATA_TABLE_ENTRY *Module)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_NT_HEADERS NtHeader;
    PPEB_LDR_DATA Ldr = NtCurrentPeb()->Ldr;
    ULONG_PTR DllBase, DllEnd;

    DPRINT("LdrFindEntryForAddress(Address %p)\n", Address);

    /* Nothing to do */
    if (!Ldr) return STATUS_NO_MORE_ENTRIES;

    /* Loop the module list */
    ListHead = &Ldr->InMemoryOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry and NT Headers */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderModuleList);
        if ((NtHeader = RtlImageNtHeader(LdrEntry->DllBase)))
        {
            /* Get the Image Base */
            DllBase = (ULONG_PTR)LdrEntry->DllBase;
            DllEnd = DllBase + NtHeader->OptionalHeader.SizeOfImage;

            /* Check if they match */
            if (((ULONG_PTR)Address >= DllBase) &&
                ((ULONG_PTR)Address < DllEnd))
            {
                /* Return it */
                *Module = LdrEntry;
                return STATUS_SUCCESS;
            }

            /* Next Entry */
            NextEntry = NextEntry->Flink;
        }
    }

    /* Nothing found */
    return STATUS_NO_MORE_ENTRIES;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandleEx(IN ULONG Flags,
                  IN PWSTR DllPath OPTIONAL,
                  IN PULONG DllCharacteristics OPTIONAL,
                  IN PUNICODE_STRING DllName,
                  OUT PVOID *DllHandle OPTIONAL)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING RedirectName, DllString1;
    UNICODE_STRING RawDllName;
    PUNICODE_STRING pRedirectName = &RedirectName;
    PUNICODE_STRING CompareName;
    PWCHAR p1, p2, p3;
    BOOLEAN Locked = FALSE;
    BOOLEAN RedirectedDll = FALSE;
    ULONG Cookie;
    ULONG LoadFlag;

    /* Initialize the strings */
    RtlInitUnicodeString(&DllString1, NULL);
    RtlInitUnicodeString(&RawDllName, NULL);
    RedirectName = *DllName;

    /* Clear the handle */
    if (DllHandle) *DllHandle = NULL;

    /* Check for a valid flag */
    if ((Flags & ~3) || (!DllHandle && !(Flags & 2)))
    {
        DPRINT1("Flags are invalid or no DllHandle given\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* If not initializing */
    if (!LdrpInLdrInit)
    {
        /* Acquire the lock */
        Status = LdrLockLoaderLock(0, NULL, &Cookie);
        Locked = TRUE;
    }

    /* Check if the SxS Assemblies specify another file */
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      pRedirectName,
                                                      &LdrApiDefaultExtension,
                                                      NULL,
                                                      &DllString1,
                                                      &pRedirectName,
                                                      NULL,
                                                      NULL,
                                                      NULL);

    /* Check success */
    if (NT_SUCCESS(Status))
    {
        /* Let Ldrp know */
        RedirectedDll = TRUE;
    }
    else if (Status != STATUS_SXS_KEY_NOT_FOUND)
    {
        /* Unrecoverable SxS failure; */
        goto Quickie;
    }

    /* Use the cache if we can */
    if (LdrpGetModuleHandleCache)
    {
        /* Check if we were redirected */
        if (RedirectedDll)
        {
            /* Check the flag */
            if (LdrpGetModuleHandleCache->Flags & LDRP_REDIRECTED)
            {
                /* Use the right name */
                CompareName = &LdrpGetModuleHandleCache->FullDllName;
            }
            else
            {
                goto DontCompare;
            }
        }
        else
        {
            /* Check the flag */
            if (!(LdrpGetModuleHandleCache->Flags & LDRP_REDIRECTED))
            {
                /* Use the right name */
                CompareName = &LdrpGetModuleHandleCache->BaseDllName;
            }
            else
            {
                goto DontCompare;
            }
        }

        /* Check if the name matches */
        if (RtlEqualUnicodeString(pRedirectName,
                                  CompareName,
                                  TRUE))
        {
            /* Skip the rest */
            LdrEntry = LdrpGetModuleHandleCache;

            /* Return success */
            Status = STATUS_SUCCESS;

            goto FoundEntry;
        }
    }

DontCompare:
    /* Find the name without the extension */
    p1 = pRedirectName->Buffer;
    p3 = &p1[pRedirectName->Length / sizeof(WCHAR)];
StartLoop:
    p2 = NULL;
    while (p1 != p3)
    {
        if (*p1++ == L'.')
        {
            p2 = p1;
        }
        else if (*p1 == L'\\')
        {
            goto StartLoop;
        }
    }

    /* Check if no extension was found or if we got a slash */
    if (!p2 || *p2 == L'\\' || *p2 == L'/')
    {
        /* Check that we have space to add one */
        if (pRedirectName->Length + LdrApiDefaultExtension.Length >= MAXLONG)
        {
            /* No space to add the extension */
            return STATUS_NAME_TOO_LONG;
        }

        /* Setup the string */
        RawDllName.MaximumLength = pRedirectName->Length + LdrApiDefaultExtension.Length + sizeof(UNICODE_NULL);
        RawDllName.Length = RawDllName.MaximumLength - sizeof(UNICODE_NULL);
        RawDllName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            RawDllName.MaximumLength);

        /* Copy the buffer */
        RtlMoveMemory(RawDllName.Buffer,
                      pRedirectName->Buffer,
                      pRedirectName->Length);

        /* Add extension */
        RtlMoveMemory((PVOID)((ULONG_PTR)RawDllName.Buffer + pRedirectName->Length),
                      LdrApiDefaultExtension.Buffer,
                      LdrApiDefaultExtension.Length);

        /* Null terminate */
        RawDllName.Buffer[RawDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        /* Check if there's something in the name */
        if (pRedirectName->Length)
        {
            /* Check and remove trailing period */
            if (pRedirectName->Buffer[(pRedirectName->Length - 2) /
                sizeof(WCHAR)] == '.')
            {
                /* Decrease the size */
                pRedirectName->Length -= sizeof(WCHAR);
            }
        }

        /* Setup the string */
        RawDllName.MaximumLength = pRedirectName->Length + sizeof(WCHAR);
        RawDllName.Length = pRedirectName->Length;
        RawDllName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            RawDllName.MaximumLength);

        /* Copy the buffer */
        RtlMoveMemory(RawDllName.Buffer,
                      pRedirectName->Buffer,
                      pRedirectName->Length);

        /* Null terminate */
        RawDllName.Buffer[RawDllName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Display debug string */
    if (ShowSnaps)
    {
        DPRINT1("LDR: LdrGetDllHandle, searching for %wZ from %ws\n",
                &RawDllName,
                DllPath ? ((ULONG_PTR)DllPath == 1 ? L"" : DllPath) : L"");
    }

    /* Do the lookup */
    if (LdrpCheckForLoadedDll(DllPath,
                              &RawDllName,
                              ((ULONG_PTR)DllPath == 1) ? TRUE : FALSE,
                              RedirectedDll,
                              &LdrEntry))
    {
        /* Update cached entry */
        LdrpGetModuleHandleCache = LdrEntry;

        /* Return success */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Make sure to NULL this */
        LdrEntry = NULL;
    }
FoundEntry:
    DPRINT("Got LdrEntry->BaseDllName %wZ\n", LdrEntry ? &LdrEntry->BaseDllName : NULL);

    /* Check if we got an entry */
    if (LdrEntry)
    {
        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Check if the DLL is locked */
            if (LdrEntry->LoadCount != -1)
            {
                /* Check what flag we got */
                if (!(Flags & 1))
                {
                    /* Check what to do with the load count */
                    if (Flags & 2)
                    {
                        /* Pin it */
                        LdrEntry->LoadCount = -1;
                        LoadFlag = LDRP_UPDATE_PIN;
                    }
                    else
                    {
                        /* Increase the load count */
                        LdrEntry->LoadCount++;
                        LoadFlag = LDRP_UPDATE_REFCOUNT;
                    }

                    /* Update the load count now */
                    LdrpUpdateLoadCount2(LdrEntry, LoadFlag);
                    LdrpClearLoadInProgress();
                }
            }

            /* Check if the caller is requesting the handle */
            if (DllHandle) *DllHandle = LdrEntry->DllBase;
        }
    }
Quickie:
    /* Free string if needed */
    if (DllString1.Buffer) RtlFreeUnicodeString(&DllString1);

    /* Free the raw DLL Name if needed */
    if (RawDllName.Buffer)
    {
        /* Free the heap-allocated buffer */
        RtlFreeHeap(RtlGetProcessHeap(), 0, RawDllName.Buffer);
        RawDllName.Buffer = NULL;
    }

    /* Release lock */
    if (Locked) LdrUnlockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandle(IN PWSTR DllPath OPTIONAL,
                IN PULONG DllCharacteristics OPTIONAL,
                IN PUNICODE_STRING DllName,
                OUT PVOID *DllHandle)
{
    /* Call the newer API */
    return LdrGetDllHandleEx(TRUE,
                             DllPath,
                             DllCharacteristics,
                             DllName,
                             DllHandle);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddress(IN PVOID BaseAddress,
                       IN PANSI_STRING Name,
                       IN ULONG Ordinal,
                       OUT PVOID *ProcedureAddress)
{
    /* Call the internal routine and tell it to execute DllInit */
    return LdrpGetProcedureAddress(BaseAddress, Name, Ordinal, ProcedureAddress, TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(IN HANDLE FileHandle,
                              IN PLDR_CALLBACK Callback,
                              IN PVOID CallbackContext,
                              OUT PUSHORT ImageCharacteristics)
{
    FILE_STANDARD_INFORMATION FileStandardInfo;
    PIMAGE_IMPORT_DESCRIPTOR ImportData;
    PIMAGE_SECTION_HEADER LastSection;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_NT_HEADERS NtHeader;
    HANDLE SectionHandle;
    SIZE_T ViewSize = 0;
    PVOID ViewBase = NULL;
    BOOLEAN Result;
    NTSTATUS Status;
    PVOID ImportName;
    ULONG Size;

    DPRINT("LdrVerifyImageMatchesChecksum() called\n");

    /* Create the section */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_EXECUTE,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_COMMIT,
                             FileHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("NtCreateSection() failed (Status 0x%x)\n", Status);
        return Status;
    }

    /* Map the section */
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection() failed (Status 0x%x)\n", Status);
        NtClose(SectionHandle);
        return Status;
    }

    /* Get the file information */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileStandardInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection() failed (Status 0x%x)\n", Status);
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        NtClose(SectionHandle);
        return Status;
    }

    /* Protect with SEH */
    _SEH2_TRY
    {
        /* Verify the checksum */
        Result = LdrVerifyMappedImageMatchesChecksum(ViewBase,
                                                     ViewSize,
                                                     FileStandardInfo.EndOfFile.LowPart);

        /* Check if a callback was supplied */
        if (Result && Callback)
        {
            /* Get the NT Header */
            NtHeader = RtlImageNtHeader(ViewBase);

            /* Check if caller requested this back */
            if (ImageCharacteristics)
            {
                /* Return to caller */
                *ImageCharacteristics = NtHeader->FileHeader.Characteristics;
            }

            /* Get the Import Directory Data */
            ImportData = RtlImageDirectoryEntryToData(ViewBase,
                                                      FALSE,
                                                      IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                      &Size);

            /* Make sure there is one */
            if (ImportData)
            {
                /* Loop the imports */
                while (ImportData->Name)
                {
                    /* Get the name */
                    ImportName = RtlImageRvaToVa(NtHeader,
                                                 ViewBase,
                                                 ImportData->Name,
                                                 &LastSection);

                    /* Notify the callback */
                    Callback(CallbackContext, ImportName);
                    ImportData++;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail the request returning STATUS_IMAGE_CHECKSUM_MISMATCH */
        Result = FALSE;
    }
    _SEH2_END;

    /* Unmap file and close handle */
    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
    NtClose(SectionHandle);

    /* Return status */
    return !Result ? STATUS_IMAGE_CHECKSUM_MISMATCH : Status;
}

NTSTATUS
NTAPI
LdrQueryProcessModuleInformationEx(IN ULONG ProcessId,
                                   IN ULONG Reserved,
                                   IN PRTL_PROCESS_MODULES ModuleInformation,
                                   IN ULONG Size,
                                   OUT PULONG ReturnedSize OPTIONAL)
{
    PLIST_ENTRY ModuleListHead, InitListHead;
    PLIST_ENTRY Entry, InitEntry;
    PLDR_DATA_TABLE_ENTRY Module, InitModule;
    PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UsedSize = sizeof(ULONG);
    ANSI_STRING AnsiString;
    PCHAR p;

    DPRINT("LdrQueryProcessModuleInformation() called\n");

    /* Acquire loader lock */
    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    /* Check if we were given enough space */
    if (Size < UsedSize)
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;
    }
    else
    {
        ModuleInformation->NumberOfModules = 0;
        ModulePtr = &ModuleInformation->Modules[0];
        Status = STATUS_SUCCESS;
    }

    /* Traverse the list of modules */
    _SEH2_TRY
    {
        ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Entry = ModuleListHead->Flink;

        while (Entry != ModuleListHead)
        {
            Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            DPRINT("  Module %wZ\n", &Module->FullDllName);

            /* Increase the used size */
            UsedSize += sizeof(RTL_PROCESS_MODULE_INFORMATION);

            if (UsedSize > Size)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                ModulePtr->ImageBase = Module->DllBase;
                ModulePtr->ImageSize = Module->SizeOfImage;
                ModulePtr->Flags = Module->Flags;
                ModulePtr->LoadCount = Module->LoadCount;
                ModulePtr->MappedBase = NULL;
                ModulePtr->InitOrderIndex = 0;
                ModulePtr->LoadOrderIndex = ModuleInformation->NumberOfModules;

                /* Now get init order index by traversing init list */
                InitListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
                InitEntry = InitListHead->Flink;

                while (InitEntry != InitListHead)
                {
                    InitModule = CONTAINING_RECORD(InitEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);

                    /* Increase the index */
                    ModulePtr->InitOrderIndex++;

                    /* Quit the loop if our module is found */
                    if (InitModule == Module) break;

                    /* Advance to the next entry */
                    InitEntry = InitEntry->Flink;
                }

                /* Prepare ANSI string with the module's name */
                AnsiString.Length = 0;
                AnsiString.MaximumLength = sizeof(ModulePtr->FullPathName);
                AnsiString.Buffer = ModulePtr->FullPathName;
                RtlUnicodeStringToAnsiString(&AnsiString,
                                             &Module->FullDllName,
                                             FALSE);

                /* Calculate OffsetToFileName field */
                p = strrchr(ModulePtr->FullPathName, '\\');
                if (p != NULL)
                    ModulePtr->OffsetToFileName = p - ModulePtr->FullPathName + 1;
                else
                    ModulePtr->OffsetToFileName = 0;

                /* Advance to the next module in the output list */
                ModulePtr++;

                /* Increase number of modules */
                if (ModuleInformation)
                    ModuleInformation->NumberOfModules++;
            }

            /* Go to the next entry in the modules list */
            Entry = Entry->Flink;
        }

        /* Set returned size if it was provided */
        if (ReturnedSize)
            *ReturnedSize = UsedSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Ignoring the exception */
    } _SEH2_END;

    /* Release the lock */
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    DPRINT("LdrQueryProcessModuleInformation() done\n");

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(IN PRTL_PROCESS_MODULES ModuleInformation,
                                 IN ULONG Size,
                                 OUT PULONG ReturnedSize OPTIONAL)
{
    /* Call Ex version of the API */
    return LdrQueryProcessModuleInformationEx(0, 0, ModuleInformation, Size, ReturnedSize);
}

NTSTATUS
NTAPI
LdrEnumerateLoadedModules(BOOLEAN ReservedFlag, PLDR_ENUM_CALLBACK EnumProc, PVOID Context)
{
    PLIST_ENTRY ListHead, ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status;
    ULONG Cookie;
    BOOLEAN Stop = FALSE;

    /* Check parameters */
    if (ReservedFlag || !EnumProc) return STATUS_INVALID_PARAMETER;

    /* Acquire the loader lock */
    Status = LdrLockLoaderLock(0, NULL, &Cookie);
    if (!NT_SUCCESS(Status)) return Status;

    /* Loop all the modules and call enum proc */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;
    while (ListHead != ListEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        /* Call the enumeration proc inside SEH */
        _SEH2_TRY
        {
            EnumProc(LdrEntry, Context, &Stop);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Ignoring the exception */
        } _SEH2_END;

        /* Break if we were asked to stop enumeration */
        if (Stop)
        {
            /* Release loader lock */
            Status = LdrUnlockLoaderLock(0, Cookie);

            /* Reset any successful status to STATUS_SUCCESS, but leave
               failure to the caller */
            if (NT_SUCCESS(Status))
                Status = STATUS_SUCCESS;

            /* Return any possible failure status */
            return Status;
        }

        /* Advance to the next module */
        ListEntry = ListEntry->Flink;
    }

    /* Release loader lock, it must succeed this time */
    Status = LdrUnlockLoaderLock(0, Cookie);
    ASSERT(NT_SUCCESS(Status));

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
