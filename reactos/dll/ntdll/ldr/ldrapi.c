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

#define LDR_LOCK_HELD 0x2
#define LDR_LOCK_FREE 0x1

LONG LdrpLoaderLockAcquisitonCount;

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
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
    BOOLEAN InInit = FALSE; // FIXME
    //BOOLEAN InInit = LdrpInLdrInit;

    DPRINT("LdrLockLoaderLock(%x %p %p)\n", Flags, Result, Cookie);

    /* Zero out the outputs */
    if (Result) *Result = 0;
    if (Cookie) *Cookie = 0;

    /* Validate the flags */
    if (Flags & ~(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS |
                  LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY))
    {
        /* Flags are invalid, check how to fail */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
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
    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS)
    {
        /* Check if we should enter or simply try */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
        {
            /* Do a try */
            if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
            {
                /* It's locked */
                *Result = LDR_LOCK_HELD;
                goto Quickie;
            }
            else
            {
                /* It worked */
                *Result = LDR_LOCK_FREE;
            }
        }
        else
        {
            /* Do a enter */
            RtlEnterCriticalSection(&LdrpLoaderLock);

            /* See if result was requested */
            if (Result) *Result = LDR_LOCK_FREE;
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
                    *Result = LDR_LOCK_HELD;
                    _SEH2_YIELD(return STATUS_SUCCESS);
                }
                else
                {
                    /* It worked */
                    *Result = LDR_LOCK_FREE;
                }
            }
            else
            {
                /* Do an enter */
                RtlEnterCriticalSection(&LdrpLoaderLock);

                /* See if result was requested */
                if (Result) *Result = LDR_LOCK_FREE;
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

/* EOF */
