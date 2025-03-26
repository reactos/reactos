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

LIST_ENTRY LdrpUnloadHead;
LONG LdrpLoaderLockAcquisitionCount;
BOOLEAN LdrpShowRecursiveLoads, LdrpBreakOnRecursiveDllLoads;
UNICODE_STRING LdrApiDefaultExtension = RTL_CONSTANT_STRING(L".DLL");
ULONG AlternateResourceModuleCount;
extern PLDR_MANIFEST_PROBER_ROUTINE LdrpManifestProberRoutine;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrFindCreateProcessManifest(IN ULONG Flags,
                             IN PVOID Image,
                             IN PVOID IdPath,
                             IN ULONG IdPathLength,
                             IN PVOID OutDataEntry)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrDestroyOutOfProcessImage(IN PVOID Image)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrCreateOutOfProcessImage(IN ULONG Flags,
                           IN HANDLE ProcessHandle,
                           IN HANDLE DllHandle,
                           IN PVOID Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrAccessOutOfProcessResource(IN PVOID Unknown,
                              IN PVOID Image,
                              IN PVOID Unknown1,
                              IN PVOID Unknown2,
                              IN PVOID Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE Routine)
{
    LdrpManifestProberRoutine = Routine;
}

BOOLEAN
NTAPI
LdrAlternateResourcesEnabled(VOID)
{
    /* ReactOS does not support this */
    return FALSE;
}

FORCEINLINE
ULONG_PTR
LdrpMakeCookie(VOID)
{
    /* Generate a cookie */
    return (((ULONG_PTR)NtCurrentTeb()->RealClientId.UniqueThread & 0xFFF) << 16) |
            (_InterlockedIncrement(&LdrpLoaderLockAcquisitionCount) & 0xFFFF);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_opt_ ULONG_PTR Cookie)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("LdrUnlockLoaderLock(%x %Ix)\n", Flags, Cookie);

    /* Check for valid flags */
    if (Flags & ~LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
    {
        /* Flags are invalid, check how to fail */
        if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
        }

        /* A normal failure */
        return STATUS_INVALID_PARAMETER_1;
    }

    /* If we don't have a cookie, just return */
    if (!Cookie) return STATUS_SUCCESS;

    /* Validate the cookie */
    if ((Cookie & 0xF0000000) ||
        ((Cookie >> 16) ^ (HandleToUlong(NtCurrentTeb()->RealClientId.UniqueThread) & 0xFFF)))
    {
        DPRINT1("LdrUnlockLoaderLock() called with an invalid cookie!\n");

        /* Invalid cookie, check how to fail */
        if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* The caller wants us to raise status */
            RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
        }

        /* A normal failure */
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Ready to release the lock */
    if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
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
LdrLockLoaderLock(
    _In_ ULONG Flags,
    _Out_opt_ PULONG Disposition,
    _Out_opt_ PULONG_PTR Cookie)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN InInit = LdrpInLdrInit;

    DPRINT("LdrLockLoaderLock(%x %p %p)\n", Flags, Disposition, Cookie);

    /* Zero out the outputs */
    if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID;
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
    if ((Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY) && !(Disposition))
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
                *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
            }
            else
            {
                /* It worked */
                *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                *Cookie = LdrpMakeCookie();
            }
        }
        else
        {
            /* Do a enter */
            RtlEnterCriticalSection(&LdrpLoaderLock);

            /* See if result was requested */
            if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
            *Cookie = LdrpMakeCookie();
        }
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
                    *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
                }
                else
                {
                    /* It worked */
                    *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                    *Cookie = LdrpMakeCookie();
                }
            }
            else
            {
                /* Do an enter */
                RtlEnterCriticalSection(&LdrpLoaderLock);

                /* See if result was requested */
                if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                *Cookie = LdrpMakeCookie();
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* We should use the LDR Filter instead */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DECLSPEC_HOTPATCH
LdrLoadDll(
    _In_opt_ PWSTR SearchPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *BaseAddress)
{
    WCHAR StringBuffer[MAX_PATH];
    UNICODE_STRING StaticString, DynamicString;
    BOOLEAN RedirectedDll = FALSE;
    NTSTATUS Status;
    ULONG_PTR Cookie;
    PUNICODE_STRING OldTldDll;
    PTEB Teb = NtCurrentTeb();

    /* Initialize the strings */
    RtlInitEmptyUnicodeString(&StaticString, StringBuffer, sizeof(StringBuffer));
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);

    Status = LdrpApplyFileNameRedirection(DllName, &LdrApiDefaultExtension, &StaticString, &DynamicString, &DllName, &RedirectedDll);

    /* Lock the loader lock */
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &Cookie);

    /* Check if there's a TLD DLL being loaded */
    OldTldDll = LdrpTopLevelDllBeingLoaded;
    _SEH2_TRY
    {

        if (OldTldDll)
        {
            /* This is a recursive load, do something about it? */
            if ((ShowSnaps) || (LdrpShowRecursiveLoads) || (LdrpBreakOnRecursiveDllLoads))
            {
                /* Print out debug messages */
                DPRINT1("[%p, %p] LDR: Recursive DLL Load\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread);
                DPRINT1("[%p, %p]      Previous DLL being loaded \"%wZ\"\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread,
                        OldTldDll);
                DPRINT1("[%p, %p]      DLL being requested \"%wZ\"\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread,
                        DllName);

                /* Was it initializing too? */
                if (!LdrpCurrentDllInitializer)
                {
                    DPRINT1("[%p, %p] LDR: No DLL Initializer was running\n",
                            Teb->RealClientId.UniqueProcess,
                            Teb->RealClientId.UniqueThread);
                }
                else
                {
                    DPRINT1("[%p, %p]      DLL whose initializer was currently running \"%wZ\"\n",
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
        if (NT_SUCCESS(Status))
        {
            Status = STATUS_SUCCESS;
        }
        else if ((Status != STATUS_NO_SUCH_FILE) &&
                 (Status != STATUS_DLL_NOT_FOUND) &&
                 (Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                 (Status != STATUS_DLL_INIT_FAILED))
        {
            DbgPrintEx(DPFLTR_LDR_ID,
                       DPFLTR_WARNING_LEVEL,
                       "LDR: %s - failing because LdrpLoadDll(%wZ) returned status %x\n",
                       __FUNCTION__,
                       DllName,
                       Status);
        }
    }
    _SEH2_FINALLY
    {
        /* Restore the old TLD DLL */
        LdrpTopLevelDllBeingLoaded = OldTldDll;

        /* Release the lock */
        LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);
    }
    _SEH2_END;

    /* Do we have a redirect string? */
    if (DynamicString.Buffer)
        RtlFreeUnicodeString(&DynamicString);

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID Address,
    _Out_ PLDR_DATA_TABLE_ENTRY *Module)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_NT_HEADERS NtHeader;
    PPEB_LDR_DATA Ldr = NtCurrentPeb()->Ldr;
    ULONG_PTR DllBase, DllEnd;

    DPRINT("LdrFindEntryForAddress(Address %p)\n", Address);

    /* Nothing to do */
    if (!Ldr) return STATUS_NO_MORE_ENTRIES;

    /* Get the current entry */
    LdrEntry = Ldr->EntryInProgress;
    if (LdrEntry)
    {
        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
        if (NtHeader)
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
        }
    }

    /* Loop the module list */
    ListHead = &Ldr->InMemoryOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry and NT Headers */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
        if (NtHeader)
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
    DbgPrintEx(DPFLTR_LDR_ID,
               DPFLTR_WARNING_LEVEL,
               "LDR: %s() exiting 0x%08lx\n",
               __FUNCTION__,
               STATUS_NO_MORE_ENTRIES);
    return STATUS_NO_MORE_ENTRIES;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandleEx(
    _In_ ULONG Flags,
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_opt_ PVOID *DllHandle)
{
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING RedirectName, DynamicString, RawDllName;
    PUNICODE_STRING pRedirectName, CompareName;
    PWCHAR p1, p2, p3;
    BOOLEAN Locked, RedirectedDll;
    ULONG_PTR Cookie;
    ULONG LoadFlag, Length;

    /* Initialize the strings */
    RtlInitEmptyUnicodeString(&DynamicString, NULL, 0);
    RtlInitEmptyUnicodeString(&RawDllName, NULL, 0);
    RedirectName = *DllName;
    pRedirectName = &RedirectName;

    /* Initialize state */
    RedirectedDll = Locked = FALSE;
    LdrEntry = NULL;
    Cookie = 0;

    /* Clear the handle */
    if (DllHandle) *DllHandle = NULL;

    /* Check for a valid flag combination */
    if ((Flags & ~(LDR_GET_DLL_HANDLE_EX_PIN | LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT)) ||
        (!DllHandle && !(Flags & LDR_GET_DLL_HANDLE_EX_PIN)))
    {
        DPRINT1("Flags are invalid or no DllHandle given\n");
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* If not initializing */
    if (!LdrpInLdrInit)
    {
        /* Acquire the lock */
        Status = LdrLockLoaderLock(0, NULL, &Cookie);
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Remember we own it */
        Locked = TRUE;
    }

    Status = LdrpApplyFileNameRedirection(
        pRedirectName, &LdrApiDefaultExtension, NULL, &DynamicString, &pRedirectName, &RedirectedDll);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrpApplyFileNameRedirection FAILED: (Status 0x%x)\n", Status);
        goto Quickie;
    }

    /* Set default failure code */
    Status = STATUS_DLL_NOT_FOUND;

    /* Use the cache if we can */
    if (LdrpGetModuleHandleCache)
    {
        /* Check if we were redirected */
        if (RedirectedDll)
        {
            /* Check the flag */
            if (!(LdrpGetModuleHandleCache->Flags & LDRP_REDIRECTED))
            {
                goto DontCompare;
            }

            /* Use the right name */
            CompareName = &LdrpGetModuleHandleCache->FullDllName;
        }
        else
        {
            /* Check the flag */
            if (LdrpGetModuleHandleCache->Flags & LDRP_REDIRECTED)
            {
                goto DontCompare;
            }

            /* Use the right name */
            CompareName = &LdrpGetModuleHandleCache->BaseDllName;
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
            goto Quickie;
        }
    }

DontCompare:
    /* Find the name without the extension */
    p1 = pRedirectName->Buffer;
    p2 = NULL;
    p3 = &p1[pRedirectName->Length / sizeof(WCHAR)];
    while (p1 != p3)
    {
        if (*p1++ == L'.')
        {
            p2 = p1;
        }
        else if (*p1 == L'\\')
        {
            p2 = NULL;
        }
    }

    /* Check if no extension was found or if we got a slash */
    if (!(p2) || (*p2 == L'\\') || (*p2 == L'/'))
    {
        /* Check that we have space to add one */
        Length = pRedirectName->Length +
                 LdrApiDefaultExtension.Length + sizeof(UNICODE_NULL);
        if (Length >= UNICODE_STRING_MAX_BYTES)
        {
            /* No space to add the extension */
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }

        /* Setup the string */
        RawDllName.MaximumLength = Length;
        ASSERT(Length >= sizeof(UNICODE_NULL));
        RawDllName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            RawDllName.MaximumLength);
        if (!RawDllName.Buffer)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Copy the string and add extension */
        RtlCopyUnicodeString(&RawDllName, pRedirectName);
        RtlAppendUnicodeStringToString(&RawDllName, &LdrApiDefaultExtension);
    }
    else
    {
        /* Check if there's something in the name */
        Length = pRedirectName->Length;
        if (Length)
        {
            /* Check and remove trailing period */
            if (pRedirectName->Buffer[Length / sizeof(WCHAR) - sizeof(ANSI_NULL)] == '.')
            {
                /* Decrease the size */
                pRedirectName->Length -= sizeof(WCHAR);
            }
        }

        /* Setup the string */
        RawDllName.MaximumLength = pRedirectName->Length + sizeof(WCHAR);
        RawDllName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            RawDllName.MaximumLength);
        if (!RawDllName.Buffer)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Copy the string */
        RtlCopyUnicodeString(&RawDllName, pRedirectName);
    }

    /* Display debug string */
    if (ShowSnaps)
    {
        DPRINT1("LDR: LdrGetDllHandleEx, searching for %wZ from %ws\n",
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
Quickie:
    /* The success path must have a valid loader entry */
    ASSERT((LdrEntry != NULL) == NT_SUCCESS(Status));

    /* Check if we got an entry and success */
    DPRINT("Got LdrEntry->BaseDllName %wZ\n", LdrEntry ? &LdrEntry->BaseDllName : NULL);
    if ((LdrEntry) && (NT_SUCCESS(Status)))
    {
        /* Check if the DLL is locked */
        if ((LdrEntry->LoadCount != 0xFFFF) &&
            !(Flags & LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT))
        {
            /* Check what to do with the load count */
            if (Flags & LDR_GET_DLL_HANDLE_EX_PIN)
            {
                /* Pin it */
                LdrEntry->LoadCount = 0xFFFF;
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

        /* Check if the caller is requesting the handle */
        if (DllHandle) *DllHandle = LdrEntry->DllBase;
    }

    /* Free string if needed */
    if (DynamicString.Buffer) RtlFreeUnicodeString(&DynamicString);

    /* Free the raw DLL Name if needed */
    if (RawDllName.Buffer)
    {
        /* Free the heap-allocated buffer */
        RtlFreeHeap(RtlGetProcessHeap(), 0, RawDllName.Buffer);
        RawDllName.Buffer = NULL;
    }

    /* Release lock */
    if (Locked)
    {
        LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS,
                            Cookie);
    }

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle)
{
    /* Call the newer API */
    return LdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
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
LdrGetProcedureAddress(
    _In_ PVOID BaseAddress,
    _In_opt_ _When_(Ordinal == 0, _Notnull_) PANSI_STRING Name,
    _In_opt_ _When_(Name == NULL, _In_range_(>, 0)) ULONG Ordinal,
    _Out_ PVOID *ProcedureAddress)
{
    /* Call the internal routine and tell it to execute DllInit */
    return LdrpGetProcedureAddress(BaseAddress, Name, Ordinal, ProcedureAddress, TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    _In_ HANDLE FileHandle,
    _In_ PLDR_CALLBACK Callback,
    _In_ PVOID CallbackContext,
    _Out_ PUSHORT ImageCharacteristics)
{
    FILE_STANDARD_INFORMATION FileStandardInfo;
    PIMAGE_IMPORT_DESCRIPTOR ImportData;
    PIMAGE_SECTION_HEADER LastSection = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_NT_HEADERS NtHeader;
    HANDLE SectionHandle;
    SIZE_T ViewSize;
    PVOID ViewBase;
    BOOLEAN Result, NoActualCheck;
    NTSTATUS Status;
    PVOID ImportName;
    ULONG Size;
    DPRINT("LdrVerifyImageMatchesChecksum() called\n");

    /* If the handle has the magic KnownDll flag, skip actual checksums */
    NoActualCheck = ((ULONG_PTR)FileHandle & 1);

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
    ViewSize = 0;
    ViewBase = NULL;
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
        /* Check if this is the KnownDll hack */
        if (NoActualCheck)
        {
            /* Don't actually do it */
            Result = TRUE;
        }
        else
        {
            /* Verify the checksum */
            Result = LdrVerifyMappedImageMatchesChecksum(ViewBase,
                                                         FileStandardInfo.EndOfFile.LowPart,
                                                         FileStandardInfo.EndOfFile.LowPart);
        }

        /* Check if a callback was supplied */
        if ((Result) && (Callback))
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
    return Result ? Status : STATUS_IMAGE_CHECKSUM_MISMATCH;
}

NTSTATUS
NTAPI
LdrQueryProcessModuleInformationEx(
    _In_opt_ ULONG ProcessId,
    _Reserved_ ULONG Reserved,
    _Out_writes_bytes_to_(Size, *ReturnedSize) PRTL_PROCESS_MODULES ModuleInformation,
    _In_ ULONG Size,
    _Out_opt_ PULONG ReturnedSize)
{
    PLIST_ENTRY ModuleListHead, InitListHead;
    PLIST_ENTRY Entry, InitEntry;
    PLDR_DATA_TABLE_ENTRY Module, InitModule;
    PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UsedSize = FIELD_OFFSET(RTL_PROCESS_MODULES, Modules);
    ANSI_STRING AnsiString;
    PCHAR p;

    DPRINT("LdrQueryProcessModuleInformation() called\n");

    /* Acquire loader lock */
    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    _SEH2_TRY
    {
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
                    InitModule = CONTAINING_RECORD(InitEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

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
LdrQueryProcessModuleInformation(
    _Out_writes_bytes_to_(Size, *ReturnedSize) PRTL_PROCESS_MODULES ModuleInformation,
    _In_ ULONG Size,
    _Out_opt_ PULONG ReturnedSize)
{
    /* Call Ex version of the API */
    return LdrQueryProcessModuleInformationEx(0, 0, ModuleInformation, Size, ReturnedSize);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _Reserved_ ULONG ReservedFlag,
    _In_ PLDR_ENUM_CALLBACK EnumProc,
    _In_opt_ PVOID Context)
{
    PLIST_ENTRY ListHead, ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status;
    ULONG_PTR Cookie;
    BOOLEAN Stop = FALSE;

    /* Check parameters */
    if ((ReservedFlag) || !(EnumProc)) return STATUS_INVALID_PARAMETER;

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
            break;
        }

        /* Advance to the next module */
        ListEntry = ListEntry->Flink;
    }

    /* Release loader lock */
    Status = LdrUnlockLoaderLock(0, Cookie);
    ASSERT(NT_SUCCESS(Status));

    /* Reset any successful status to STATUS_SUCCESS,
     * but leave failure to the caller */
    if (NT_SUCCESS(Status))
        Status = STATUS_SUCCESS;

    /* Return any possible failure status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status;
    BOOLEAN LockHeld;
    ULONG_PTR Cookie;
    DPRINT("LdrDisableThreadCalloutsForDll (BaseAddress %p)\n", BaseAddress);

    /* Don't do it during shutdown */
    if (LdrpShutdownInProgress) return STATUS_SUCCESS;

    /* Check if we should grab the lock */
    LockHeld = FALSE;
    if (!LdrpInLdrInit)
    {
        /* Grab the lock */
        Status = LdrLockLoaderLock(0, NULL, &Cookie);
        if (!NT_SUCCESS(Status)) return Status;
        LockHeld = TRUE;
    }

    /* Make sure the DLL is valid and get its entry */
    Status = STATUS_DLL_NOT_FOUND;
    if (LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
    {
        /* Get if it has a TLS slot */
        if (!LdrEntry->TlsIndex)
        {
            /* It doesn't, so you're allowed to call this */
            LdrEntry->Flags |= LDRP_DONT_CALL_FOR_THREADS;
            Status = STATUS_SUCCESS;
        }
    }

    /* Check if the lock was held */
    if (LockHeld)
    {
        /* Release it */
        LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);
    }

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrAddRefDll(
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR Cookie;
    BOOLEAN Locked = FALSE;

    /* Check for invalid flags */
    if (Flags & ~(LDR_ADDREF_DLL_PIN))
    {
        /* Fail with invalid parameter status if so */
        Status = STATUS_INVALID_PARAMETER;
        goto quickie;
    }

    /* Acquire the loader lock if not in init phase */
    if (!LdrpInLdrInit)
    {
        /* Acquire the lock */
        Status = LdrLockLoaderLock(0, NULL, &Cookie);
        if (!NT_SUCCESS(Status)) goto quickie;
        Locked = TRUE;
    }

    /* Get this module's data table entry */
    if (LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
    {
        if (!LdrEntry)
        {
            /* Shouldn't happen */
            Status = STATUS_INTERNAL_ERROR;
            goto quickie;
        }

        /* If this is not a pinned module */
        if (LdrEntry->LoadCount != 0xFFFF)
        {
            /* Update its load count */
            if (Flags & LDR_ADDREF_DLL_PIN)
            {
                /* Pin it by setting load count to -1 */
                LdrEntry->LoadCount = 0xFFFF;
                LdrpUpdateLoadCount2(LdrEntry, LDRP_UPDATE_PIN);
            }
            else
            {
                /* Increase its load count by one */
                LdrEntry->LoadCount++;
                LdrpUpdateLoadCount2(LdrEntry, LDRP_UPDATE_REFCOUNT);
            }

            /* Clear load in progress */
            LdrpClearLoadInProgress();
        }
    }
    else
    {
        /* There was an error getting this module's handle, return invalid param status */
        Status = STATUS_INVALID_PARAMETER;
    }

quickie:
    /* Check for error case */
    if (!NT_SUCCESS(Status))
    {
        /* Print debug information */
        if ((ShowSnaps) || ((Status != STATUS_NO_SUCH_FILE) &&
                            (Status != STATUS_DLL_NOT_FOUND) &&
                            (Status != STATUS_OBJECT_NAME_NOT_FOUND)))
        {
            DPRINT1("LDR: LdrAddRefDll(%p) 0x%08lx\n", BaseAddress, Status);
        }
    }

    /* Release the lock if needed */
    if (Locked) LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID BaseAddress)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();
    PLDR_DATA_TABLE_ENTRY LdrEntry, CurrentEntry;
    PVOID EntryPoint;
    PLIST_ENTRY NextEntry;
    LIST_ENTRY UnloadList;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PVOID CorImageData;
    ULONG ComSectionSize;

    /* Get the LDR Lock */
    if (!LdrpInLdrInit) RtlEnterCriticalSection(Peb->LoaderLock);

    /* Increase the unload count */
    LdrpActiveUnloadCount++;

    /* Skip unload */
    if (LdrpShutdownInProgress) goto Quickie;

    /* Make sure the DLL is valid and get its entry */
    if (!LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
    {
        Status = STATUS_DLL_NOT_FOUND;
        goto Quickie;
    }

    /* Check the current Load Count */
    if (LdrEntry->LoadCount != 0xFFFF)
    {
        /* Decrease it */
        LdrEntry->LoadCount--;

        /* If it's a dll */
        if (LdrEntry->Flags & LDRP_IMAGE_DLL)
        {
            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                   LdrEntry->EntryPointActivationContext);

            /* Update the load count */
            LdrpUpdateLoadCount2(LdrEntry, LDRP_UPDATE_DEREFCOUNT);

            /* Release the context */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }
    }
    else
    {
        /* The DLL is locked */
        goto Quickie;
    }

    /* Show debug message */
    if (ShowSnaps) DPRINT1("LDR: UNINIT LIST\n");

    /* Check if this is our only unload and initialize the list if so */
    if (LdrpActiveUnloadCount == 1) InitializeListHead(&LdrpUnloadHead);

    /* Loop the modules to build the list */
    NextEntry = Peb->Ldr->InInitializationOrderModuleList.Blink;
    while (NextEntry != &Peb->Ldr->InInitializationOrderModuleList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderLinks);
        NextEntry = NextEntry->Blink;

        /* Remove flag */
        LdrEntry->Flags &= ~LDRP_UNLOAD_IN_PROGRESS;

        /* If the load count is now 0 */
        if (!LdrEntry->LoadCount)
        {
            /* Show message */
            if (ShowSnaps)
            {
                DPRINT1("(%lu) [%ws] %ws (%lx) deinit %p\n",
                        LdrpActiveUnloadCount,
                        LdrEntry->BaseDllName.Buffer,
                        LdrEntry->FullDllName.Buffer,
                        (ULONG)LdrEntry->LoadCount,
                        LdrEntry->EntryPoint);
            }

            /* Call Shim Engine and notify */
            if (g_ShimsEnabled)
            {
                VOID (NTAPI* SE_DllUnloaded)(PVOID) = RtlDecodeSystemPointer(g_pfnSE_DllUnloaded);
                SE_DllUnloaded(LdrEntry);
            }

            /* Unlink it */
            CurrentEntry = LdrEntry;
            RemoveEntryList(&CurrentEntry->InInitializationOrderLinks);
            RemoveEntryList(&CurrentEntry->InMemoryOrderLinks);
            RemoveEntryList(&CurrentEntry->HashLinks);

            /* If there's more then one active unload */
            if (LdrpActiveUnloadCount > 1)
            {
                /* Flush the cached DLL handle and clear the list */
                LdrpLoadedDllHandleCache = NULL;
                CurrentEntry->InMemoryOrderLinks.Flink = NULL;
            }

            /* Add the entry on the unload list */
            InsertTailList(&LdrpUnloadHead, &CurrentEntry->HashLinks);
        }
    }

    /* Only call the entrypoints once */
    if (LdrpActiveUnloadCount > 1) goto Quickie;

    /* Now loop the unload list and create our own */
    InitializeListHead(&UnloadList);
    CurrentEntry = NULL;
    NextEntry = LdrpUnloadHead.Flink;
    while (NextEntry != &LdrpUnloadHead)
    {
        /* Get the current entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, HashLinks);

        LdrpRecordUnloadEvent(LdrEntry);

        /* Set the entry and clear it from the list */
        CurrentEntry = LdrEntry;
        LdrpLoadedDllHandleCache = NULL;
        CurrentEntry->InMemoryOrderLinks.Flink = NULL;

        /* Move it from the global to the local list */
        RemoveEntryList(&CurrentEntry->HashLinks);
        InsertTailList(&UnloadList, &CurrentEntry->HashLinks);

        /* Get the entrypoint */
        EntryPoint = LdrEntry->EntryPoint;

        /* Check if we should call it */
        if ((EntryPoint) && (LdrEntry->Flags & LDRP_PROCESS_ATTACH_CALLED))
        {
            /* Show message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Calling deinit %p\n", EntryPoint);
            }

            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                   LdrEntry->EntryPointActivationContext);

            /* Call the entrypoint */
            _SEH2_TRY
            {
                LdrpCallInitRoutine(LdrEntry->EntryPoint,
                                    LdrEntry->DllBase,
                                    DLL_PROCESS_DETACH,
                                    NULL);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_PROCESS_DETACH) for %wZ\n",
                        _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
            }
            _SEH2_END;

            /* Release the context */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }

        /* Remove it from the list */
        RemoveEntryList(&CurrentEntry->InLoadOrderLinks);
        CurrentEntry = NULL;
        NextEntry = LdrpUnloadHead.Flink;
    }

    /* Now loop our local list */
    NextEntry = UnloadList.Flink;
    while (NextEntry != &UnloadList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, HashLinks);
        NextEntry = NextEntry->Flink;
        CurrentEntry = LdrEntry;

        /* Notify Application Verifier */
        if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAIL_CHECK)
        {
            AVrfDllUnloadNotification(LdrEntry);
        }

        /* Show message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: Unmapping [%ws]\n", LdrEntry->BaseDllName.Buffer);
        }

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)
        /* Send shutdown notification */
        LdrpSendDllNotifications(CurrentEntry, LDR_DLL_NOTIFICATION_REASON_UNLOADED);
#endif

        /* Check if this is a .NET executable */
        CorImageData = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                                    &ComSectionSize);
        if (CorImageData)
        {
            /* FIXME */
            DPRINT1(".NET Images are not supported yet\n");
        }

        /* Check if we should unmap*/
        if (!(CurrentEntry->Flags & LDR_COR_OWNS_UNMAP))
        {
            /* Unmap the DLL */
            Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                          CurrentEntry->DllBase);
            ASSERT(NT_SUCCESS(Status));
        }

        /* Unload the alternate resource module, if any */
        LdrUnloadAlternateResourceModule(CurrentEntry->DllBase);

        /* Check if a Hotpatch is active */
        if (LdrEntry->PatchInformation)
        {
            /* FIXME */
            DPRINT1("We don't support Hotpatching yet\n");
        }

        /* Deallocate the Entry */
        LdrpFinalizeAndDeallocateDataTableEntry(CurrentEntry);

        /* If this is the cached entry, invalidate it */
        if (LdrpGetModuleHandleCache == CurrentEntry)
        {
            LdrpGetModuleHandleCache = NULL;
        }
    }

Quickie:
    /* Decrease unload count */
    LdrpActiveUnloadCount--;
    if (!LdrpInLdrInit) RtlLeaveCriticalSection(Peb->LoaderLock);

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDllShutdownInProgress(VOID)
{
    /* Return the internal global */
    return LdrpShutdownInProgress;
}

/*
 * @implemented
 */
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    _In_ ULONG_PTR Address,
    _In_ ULONG Count,
    _In_ PUSHORT TypeOffset,
    _In_ LONG_PTR Delta)
{
    return LdrProcessRelocationBlockLongLong(Address, Count, TypeOffset, Delta);
}

/* FIXME: Add to ntstatus.mc */
#define STATUS_MUI_FILE_NOT_FOUND        ((NTSTATUS)0xC00B0001L)

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(
    _In_ PVOID Module,
    _In_ PWSTR Buffer)
{
    /* Is MUI Support enabled? */
    if (!LdrAlternateResourcesEnabled()) return STATUS_SUCCESS;

    UNIMPLEMENTED;
    return STATUS_MUI_FILE_NOT_FOUND;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(
    _In_ PVOID BaseAddress)
{
    ULONG_PTR Cookie;

    /* Acquire the loader lock */
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &Cookie);

    /* Check if there's any alternate resources loaded */
    if (AlternateResourceModuleCount)
    {
        UNIMPLEMENTED;
    }

    /* Release the loader lock */
    LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, Cookie);

    /* All done */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
LdrFlushAlternateResourceModules(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 * See https://www.kernelmode.info/forum/viewtopic.php?t=991
 */
NTSTATUS
NTAPI
LdrSetAppCompatDllRedirectionCallback(
    _In_ ULONG Flags,
    _In_ PLDR_APP_COMPAT_DLL_REDIRECTION_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
LdrInitShimEngineDynamic(IN PVOID BaseAddress)
{
    ULONG_PTR Cookie;
    NTSTATUS Status = LdrLockLoaderLock(0, NULL, &Cookie);
    if (NT_SUCCESS(Status))
    {
        if (!g_pShimEngineModule)
        {
            g_pShimEngineModule = BaseAddress;
            LdrpGetShimEngineInterface();
        }
        LdrUnlockLoaderLock(0, Cookie);
        return TRUE;
    }
    return FALSE;
}

/* EOF */
