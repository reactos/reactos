/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/dosdev.c
 * PURPOSE:         Dos device functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 *                  Pierre Schweitzer
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>
#include <dbt.h>
DEBUG_CHANNEL(kernel32file);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
IsGlobalDeviceMap(
    HANDLE DirectoryHandle,
    PBOOLEAN IsGlobal)
{
    NTSTATUS Status;
    DWORD ReturnLength;
    UNICODE_STRING GlobalString;
    OBJECT_NAME_INFORMATION NameInfo, *PNameInfo;

    /* We need both parameters */
    if (DirectoryHandle == 0 || IsGlobal == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PNameInfo = NULL;
    _SEH2_TRY
    {
        /* Query handle information */
        Status = NtQueryObject(DirectoryHandle,
                               ObjectNameInformation,
                               &NameInfo,
                               0,
                               &ReturnLength);
        /* Only failure we tolerate is length mismatch */
        if (NT_SUCCESS(Status) || Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            /* Allocate big enough buffer */
            PNameInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, ReturnLength);
            if (PNameInfo == NULL)
            {
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }

            /* Query again handle information */
            Status = NtQueryObject(DirectoryHandle,
                                   ObjectNameInformation,
                                   PNameInfo,
                                   ReturnLength,
                                   &ReturnLength);

            /*
             * If it succeed, check we have Global??
             * If so, return success
             */
            if (NT_SUCCESS(Status))
            {
                RtlInitUnicodeString(&GlobalString, L"\\GLOBAL??");
                *IsGlobal = RtlEqualUnicodeString(&GlobalString, &PNameInfo->Name, FALSE);
                Status = STATUS_SUCCESS;
            }
        }
    }
    _SEH2_FINALLY
    {
        if (PNameInfo != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, PNameInfo);
        }
    }
    _SEH2_END;

    return Status;
}

/*
 * @implemented
 */
DWORD
FindSymbolicLinkEntry(
    PWSTR NameToFind,
    PWSTR NamesList,
    DWORD TotalEntries,
    PBOOLEAN Found)
{
    WCHAR Current;
    DWORD Entries;
    PWSTR PartialNamesList;

    /* We need all parameters to be set */
    if (NameToFind == NULL || NamesList == NULL || Found == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    /* Assume failure */
    *Found = FALSE;

    /* If no entries, job done, nothing found */
    if (TotalEntries == 0)
    {
        return ERROR_SUCCESS;
    }

    /* Start browsing the names list */
    Entries = 0;
    PartialNamesList = NamesList;
    /* As long as we didn't find the name... */
    while (wcscmp(NameToFind, PartialNamesList) != 0)
    {
        /* We chomped an entry! */
        ++Entries;

        /* We're out of entries, bail out not to overrun */
        if (Entries > TotalEntries)
        {
            /*
             * Even though we found nothing,
             * the function ran fine
             */
            return ERROR_SUCCESS;
        }

        /* Jump to the next string */
        do
        {
            Current = *PartialNamesList;
            ++PartialNamesList;
        } while (Current != UNICODE_NULL);
    }

    /*
     * We're here because the loop stopped:
     * it means we found the name in the list
     */
    *Found = TRUE;
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
BOOL
WINAPI
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
    BOOL Result;
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    PWSTR TargetPathBuffer;
    UNICODE_STRING TargetPathU;
    PUNICODE_STRING DeviceNameU;

    /* Convert DeviceName using static unicode string */
    RtlInitAnsiString(&AnsiString, lpDeviceName);
    DeviceNameU = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString(DeviceNameU, &AnsiString, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /*
         * If the static unicode string is too small,
         * it's because the name is too long...
         * so, return appropriate status!
         */
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }

        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Convert target path if existing */
    if (lpTargetPath != NULL)
    {
        RtlInitAnsiString(&AnsiString, lpTargetPath);
        Status = RtlAnsiStringToUnicodeString(&TargetPathU, &AnsiString, TRUE);
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            return FALSE;
        }

        TargetPathBuffer = TargetPathU.Buffer;
    }
    else
    {
        TargetPathBuffer = NULL;
    }

    /* Call W */
    Result = DefineDosDeviceW(dwFlags, DeviceNameU->Buffer, TargetPathBuffer);

    /* Free target path if allocated */
    if (TargetPathBuffer != NULL)
    {
        RtlFreeUnicodeString(&TargetPathU);
    }

    return Result;
}


/*
 * @implemented
 */
BOOL
WINAPI
DefineDosDeviceW(
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPCWSTR lpTargetPath
    )
{
    ULONG ArgumentCount;
    ULONG BufferSize;
    BASE_API_MESSAGE ApiMessage;
    PBASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest = &ApiMessage.Data.DefineDosDeviceRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    UNICODE_STRING NtTargetPathU;
    UNICODE_STRING DeviceNameU;
    HANDLE hUser32;
    DEV_BROADCAST_VOLUME dbcv;
    DWORD dwRecipients;
    typedef long (WINAPI *BSM_type)(DWORD, LPDWORD, UINT, WPARAM, LPARAM);
    BSM_type BSM_ptr;
    BOOLEAN LUIDDeviceMapsEnabled;
    WCHAR Letter;
    WPARAM wParam;

    /* Get status about local device mapping */
    LUIDDeviceMapsEnabled = BaseStaticServerData->LUIDDeviceMapsEnabled;

    /* Validate input & flags */
    if ((dwFlags & 0xFFFFFFE0) ||
        ((dwFlags & DDD_EXACT_MATCH_ON_REMOVE) &&
        !(dwFlags & DDD_REMOVE_DEFINITION)) ||
        (lpTargetPath == NULL && !(dwFlags & (DDD_LUID_BROADCAST_DRIVE | DDD_REMOVE_DEFINITION))) ||
        ((dwFlags & DDD_LUID_BROADCAST_DRIVE) &&
         (lpDeviceName == NULL || lpTargetPath != NULL || dwFlags & (DDD_NO_BROADCAST_SYSTEM | DDD_EXACT_MATCH_ON_REMOVE | DDD_RAW_TARGET_PATH) || !LUIDDeviceMapsEnabled)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Initialize device unicode string to ease its use */
    RtlInitUnicodeString(&DeviceNameU, lpDeviceName);

    /* The buffer for CSR call will contain it */
    BufferSize = DeviceNameU.MaximumLength;
    ArgumentCount = 1;

    /* If we don't have target path, use empty string */
    if (lpTargetPath == NULL)
    {
        RtlInitUnicodeString(&NtTargetPathU, NULL);
    }
    else
    {
        /* Else, use it raw if asked to */
        if (dwFlags & DDD_RAW_TARGET_PATH)
        {
            RtlInitUnicodeString(&NtTargetPathU, lpTargetPath);
        }
        else
        {
            /* Otherwise, use it converted */
            if (!RtlDosPathNameToNtPathName_U(lpTargetPath,
                                              &NtTargetPathU,
                                              NULL,
                                              NULL))
            {
                WARN("RtlDosPathNameToNtPathName_U() failed\n");
                BaseSetLastNTError(STATUS_OBJECT_NAME_INVALID);
                return FALSE;
            }
        }

        /* This target path will be the second arg */
        ArgumentCount = 2;
        BufferSize += NtTargetPathU.MaximumLength;
    }

    /* Allocate the capture buffer for our strings */
    CaptureBuffer = CsrAllocateCaptureBuffer(ArgumentCount,
                                             BufferSize);
    if (CaptureBuffer == NULL)
    {
        if (!(dwFlags & DDD_RAW_TARGET_PATH))
        {
            RtlFreeUnicodeString(&NtTargetPathU);
        }

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Set the flags */
    DefineDosDeviceRequest->Flags = dwFlags;

    /* Allocate a buffer for the device name */
    DefineDosDeviceRequest->DeviceName.MaximumLength = CsrAllocateMessagePointer(CaptureBuffer,
                                                                                 DeviceNameU.MaximumLength,
                                                                                 (PVOID*)&DefineDosDeviceRequest->DeviceName.Buffer);
    /* And copy it while upcasing it */
    RtlUpcaseUnicodeString(&DefineDosDeviceRequest->DeviceName, &DeviceNameU, FALSE);

    /* If we have a target path, copy it too, and free it if allocated */
    if (NtTargetPathU.Length != 0)
    {
        DefineDosDeviceRequest->TargetPath.MaximumLength = CsrAllocateMessagePointer(CaptureBuffer,
                                                                                     NtTargetPathU.MaximumLength,
                                                                                     (PVOID*)&DefineDosDeviceRequest->TargetPath.Buffer);
        RtlCopyUnicodeString(&DefineDosDeviceRequest->TargetPath, &NtTargetPathU);

        if (!(dwFlags & DDD_RAW_TARGET_PATH))
        {
            RtlFreeUnicodeString(&NtTargetPathU);
        }
    }
    /* Otherwise, null initialize the string */
    else
    {
        RtlInitUnicodeString(&DefineDosDeviceRequest->TargetPath, NULL);
    }

    /* Finally,  call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepDefineDosDevice),
                        sizeof(*DefineDosDeviceRequest));
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return failure if any */
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        WARN("CsrClientCallServer() failed (Status %lx)\n", ApiMessage.Status);
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /* Here is the success path, we will always return true */

    /* Should broadcast the event? Only do if not denied and if drive letter */
    if (!(dwFlags & DDD_NO_BROADCAST_SYSTEM) &&
        DeviceNameU.Length == 2 * sizeof(WCHAR) &&
        DeviceNameU.Buffer[1] == L':')
    {
        /* Make sure letter is valid and there are no local device mappings */
        Letter = RtlUpcaseUnicodeChar(DeviceNameU.Buffer[0]) - L'A';
        if (Letter < 26 && !LUIDDeviceMapsEnabled)
        {
            /* Rely on user32 for broadcasting */
            hUser32 = LoadLibraryW(L"user32.dll");
            if (hUser32 != 0)
            {
                /* Get the function pointer */
                BSM_ptr = (BSM_type)GetProcAddress(hUser32, "BroadcastSystemMessageW");
                if (BSM_ptr)
                {
                    /* Set our target */
                    dwRecipients = BSM_APPLICATIONS;

                    /* And initialize our structure */
                    dbcv.dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
                    dbcv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
                    dbcv.dbcv_reserved = 0;

                    /* Set the volume which had the event */
                    dbcv.dbcv_unitmask = 1 << Letter;
                    dbcv.dbcv_flags = DBTF_NET;

                    /* And properly set the event (removal or arrival?) */
                    wParam = (dwFlags & DDD_REMOVE_DEFINITION) ? DBT_DEVICEREMOVECOMPLETE : DBT_DEVICEARRIVAL;

                    /* And broadcast! */
                    BSM_ptr(BSF_SENDNOTIFYMESSAGE | BSF_FLUSHDISK,
                            &dwRecipients,
                            WM_DEVICECHANGE,
                            wParam,
                            (LPARAM)&dbcv);
                }

                /* We're done! */
                FreeLibrary(hUser32);
            }
        }
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    )
{
    NTSTATUS Status;
    USHORT CurrentPosition;
    ANSI_STRING AnsiString;
    UNICODE_STRING TargetPathU;
    PUNICODE_STRING DeviceNameU;
    DWORD RetLength, CurrentLength, Length;
    PWSTR DeviceNameBuffer, TargetPathBuffer;

    /* If we want a specific device name, convert it */
    if (lpDeviceName != NULL)
    {
        /* Convert DeviceName using static unicode string */
        RtlInitAnsiString(&AnsiString, lpDeviceName);
        DeviceNameU = &NtCurrentTeb()->StaticUnicodeString;
        Status = RtlAnsiStringToUnicodeString(DeviceNameU, &AnsiString, FALSE);
        if (!NT_SUCCESS(Status))
        {
            /*
             * If the static unicode string is too small,
             * it's because the name is too long...
             * so, return appropriate status!
             */
            if (Status == STATUS_BUFFER_OVERFLOW)
            {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }

            BaseSetLastNTError(Status);
            return FALSE;
        }

        DeviceNameBuffer = DeviceNameU->Buffer;
    }
    else
    {
        DeviceNameBuffer = NULL;
    }

    /* Allocate the output buffer for W call */
    TargetPathBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, ucchMax * sizeof(WCHAR));
    if (TargetPathBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* Call W */
    Length = QueryDosDeviceW(DeviceNameBuffer, TargetPathBuffer, ucchMax);
    /* We'll return that length in case of a success */
    RetLength = Length;

    /* Handle the case where we would fill output buffer completly */
    if (Length != 0 && Length == ucchMax)
    {
        /* This will be our work length (but not the one we return) */
        --Length;
        /* Already 0 the last char */
        lpTargetPath[Length] = ANSI_NULL;
    }

    /* If we had an output, start the convert loop */
    if (Length != 0)
    {
        /*
         * We'll have to loop because TargetPathBuffer may contain
         * several strings (NULL separated)
         * We'll start at position 0
         */
        CurrentPosition = 0;
        while (CurrentPosition < Length)
        {
            /* Get the maximum length */
            CurrentLength = min(Length - CurrentPosition, MAXUSHORT / 2);

            /* Initialize our output string */
            AnsiString.Length = 0;
            AnsiString.MaximumLength = CurrentLength + sizeof(ANSI_NULL);
            AnsiString.Buffer = &lpTargetPath[CurrentPosition];

            /* Initialize input string that will be converted */
            TargetPathU.Length = CurrentLength * sizeof(WCHAR);
            TargetPathU.MaximumLength = CurrentLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
            TargetPathU.Buffer = &TargetPathBuffer[CurrentPosition];

            /* Convert to ANSI */
            Status = RtlUnicodeStringToAnsiString(&AnsiString, &TargetPathU, FALSE);
            if (!NT_SUCCESS(Status))
            {
                BaseSetLastNTError(Status);
                /* In case of a failure, forget about everything */
                RetLength = 0;

                goto Leave;
            }

            /* Move to the next string */
            CurrentPosition += CurrentLength;
        }
    }

Leave:
    /* Free our intermediate buffer and leave */
    RtlFreeHeap(RtlGetProcessHeap(), 0, TargetPathBuffer);

    return RetLength;
}


/*
 * @implemented
 */
DWORD
WINAPI
QueryDosDeviceW(
    LPCWSTR lpDeviceName,
    LPWSTR lpTargetPath,
    DWORD ucchMax
    )
{
    PWSTR Ptr;
    PVOID Buffer;
    NTSTATUS Status;
    USHORT i, TotalEntries;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle, DeviceHandle;
    BOOLEAN IsGlobal, GlobalNeeded, Found;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    OBJECT_DIRECTORY_INFORMATION NullEntry = {{0}};
    ULONG ReturnLength, NameLength, Length, Context, BufferLength;

    /* Open the '\??' directory */
    RtlInitUnicodeString(&UnicodeString, L"\\??");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        WARN("NtOpenDirectoryObject() failed (Status %lx)\n", Status);
        BaseSetLastNTError(Status);
        return 0;
    }

    Buffer = NULL;
    _SEH2_TRY
    {
        if (lpDeviceName != NULL)
        {
            /* Open the lpDeviceName link object */
            RtlInitUnicodeString(&UnicodeString, lpDeviceName);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       DirectoryHandle,
                                       NULL);
            Status = NtOpenSymbolicLinkObject(&DeviceHandle,
                                              SYMBOLIC_LINK_QUERY,
                                              &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                WARN("NtOpenSymbolicLinkObject() failed (Status %lx)\n", Status);
                _SEH2_LEAVE;
            }

            /*
             * Make sure we don't overrun the output buffer, so convert our DWORD
             * size to USHORT size properly
             */
            Length = (ucchMax <= MAXULONG / sizeof(WCHAR)) ? (ucchMax * sizeof(WCHAR)) : MAXULONG;

            /* Query link target */
            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = Length <= MAXUSHORT ? Length : MAXUSHORT;
            UnicodeString.Buffer = lpTargetPath;

            ReturnLength = 0;
            Status = NtQuerySymbolicLinkObject(DeviceHandle,
                                               &UnicodeString,
                                               &ReturnLength);
            NtClose(DeviceHandle);
            if (!NT_SUCCESS(Status))
            {
                WARN("NtQuerySymbolicLinkObject() failed (Status %lx)\n", Status);
                _SEH2_LEAVE;
            }

            TRACE("ReturnLength: %lu\n", ReturnLength);
            TRACE("TargetLength: %hu\n", UnicodeString.Length);
            TRACE("Target: '%wZ'\n", &UnicodeString);

            Length = ReturnLength / sizeof(WCHAR);
            /* Make sure we null terminate output buffer */
            if (Length == 0 || lpTargetPath[Length - 1] != UNICODE_NULL)
            {
                if (Length >= ucchMax)
                {
                    TRACE("Buffer is too small\n");
                    Status = STATUS_BUFFER_TOO_SMALL;
                    _SEH2_LEAVE;
                }

                /* Append null-character */
                lpTargetPath[Length] = UNICODE_NULL;
                Length++;
            }

            if (Length < ucchMax)
            {
                /* Append null-character */
                lpTargetPath[Length] = UNICODE_NULL;
                Length++;
            }

            _SEH2_LEAVE;
        }

        /*
         * If LUID device maps are enabled,
         * ?? may not point to BaseNamedObjects
         * It may only be local DOS namespace.
         * And thus, it might be required to browse
         * Global?? for global devices
         */
        GlobalNeeded = FALSE;
        if (BaseStaticServerData->LUIDDeviceMapsEnabled)
        {
            /* Assume ?? == Global?? */
            IsGlobal = TRUE;
            /* Check if it's the case */
            Status = IsGlobalDeviceMap(DirectoryHandle, &IsGlobal);
            if (NT_SUCCESS(Status) && !IsGlobal)
            {
                /* It's not, we'll have to browse Global?? too! */
                GlobalNeeded = TRUE;
            }
        }

        /*
         * Make sure we don't overrun the output buffer, so convert our DWORD
         * size to USHORT size properly
         */
        BufferLength = (ucchMax <= MAXULONG / sizeof(WCHAR)) ? (ucchMax * sizeof(WCHAR)) : MAXULONG;
        Length = 0;
        Ptr = lpTargetPath;

        Context = 0;
        TotalEntries = 0;

        /*
         * We'll query all entries at once, with a rather big buffer
         * If it's too small, we'll grow it by 2.
         * Limit the number of attempts to 3.
         */
        for (i = 0; i < 3; ++i)
        {
            /* Allocate the query buffer */
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
            if (Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            /* Perform the query */
            Status = NtQueryDirectoryObject(DirectoryHandle,
                                            Buffer,
                                            BufferLength,
                                            FALSE,
                                            TRUE,
                                            &Context,
                                            &ReturnLength);
            /* Only failure accepted is: no more entries */
            if (!NT_SUCCESS(Status))
            {
                if (Status != STATUS_NO_MORE_ENTRIES)
                {
                    _SEH2_LEAVE;
                }

                /*
                 * Which is a success! But break out,
                 * it means our query returned no results
                 * so, nothing to parse.
                 */
                Status = STATUS_SUCCESS;
                break;
            }

            /* In case we had them all, start browsing for devices */
            if (Status != STATUS_MORE_ENTRIES)
            {
                DirInfo = Buffer;

                /* Loop until we find the nul entry (terminating entry) */
                while (TRUE)
                {
                    /* It's an entry full of zeroes */
                    if (RtlCompareMemory(&NullEntry, DirInfo, sizeof(NullEntry)) == sizeof(NullEntry))
                    {
                        break;
                    }

                    /* Only handle symlinks */
                    if (!wcscmp(DirInfo->TypeName.Buffer, L"SymbolicLink"))
                    {
                        TRACE("Name: '%wZ'\n", &DirInfo->Name);

                        /* Get name length in chars to only comparisons */
                        NameLength = DirInfo->Name.Length / sizeof(WCHAR);

                        /* Make sure we don't overrun output buffer */
                        if (Length > ucchMax ||
                            NameLength > ucchMax - Length ||
                            ucchMax - NameLength - Length < sizeof(WCHAR))
                        {
                            Status = STATUS_BUFFER_TOO_SMALL;
                            _SEH2_LEAVE;
                        }

                        /* Copy and NULL terminate string */
                        memcpy(Ptr, DirInfo->Name.Buffer, DirInfo->Name.Length);
                        Ptr[NameLength] = UNICODE_NULL;

                        Ptr += (NameLength + 1);
                        Length += (NameLength + 1);

                        /*
                         * Keep the entries count, in case we would have to
                         * handle GLOBAL?? too
                         */
                        ++TotalEntries;
                    }

                    /* Move to the next entry */
                    ++DirInfo;
                }

                /*
                 * No need to loop again here, we got all the entries
                 * Note: we don't free the buffer here, because we may
                 * need it for GLOBAL??, so we save a few cycles here.
                 */
                break;
            }

            /* Failure path here, we'll need bigger buffer */
            RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
            Buffer = NULL;

            /* We can't have bigger than that one, so leave */
            if (BufferLength == MAXULONG)
            {
                break;
            }

            /* Prevent any overflow while computing new size */
            if (MAXULONG - BufferLength < BufferLength)
            {
                BufferLength = MAXULONG;
            }
            else
            {
                BufferLength *= 2;
            }
        }

        /*
         * Out of the hot loop, but with more entries left?
         * that's an error case, leave here!
         */
        if (Status == STATUS_MORE_ENTRIES)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            _SEH2_LEAVE;
        }

        /* Now, if we had to handle GLOBAL??, go for it! */
        if (BaseStaticServerData->LUIDDeviceMapsEnabled && NT_SUCCESS(Status) && GlobalNeeded)
        {
            NtClose(DirectoryHandle);
            DirectoryHandle = 0;

            RtlInitUnicodeString(&UnicodeString, L"\\GLOBAL??");
            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            Status = NtOpenDirectoryObject(&DirectoryHandle,
                                           DIRECTORY_QUERY,
                                           &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                WARN("NtOpenDirectoryObject() failed (Status %lx)\n", Status);
                _SEH2_LEAVE;
            }

            /*
             * We'll query all entries at once, with a rather big buffer
             * If it's too small, we'll grow it by 2.
             * Limit the number of attempts to 3.
             */
            for (i = 0; i < 3; ++i)
            {
                /* If we had no buffer from previous attempt, allocate one */
                if (Buffer == NULL)
                {
                    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
                    if (Buffer == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        _SEH2_LEAVE;
                    }
                }

                /* Perform the query */
                Status = NtQueryDirectoryObject(DirectoryHandle,
                                                Buffer,
                                                BufferLength,
                                                FALSE,
                                                TRUE,
                                                &Context,
                                                &ReturnLength);
                /* Only failure accepted is: no more entries */
                if (!NT_SUCCESS(Status))
                {
                    if (Status != STATUS_NO_MORE_ENTRIES)
                    {
                        _SEH2_LEAVE;
                    }

                    /*
                     * Which is a success! But break out,
                     * it means our query returned no results
                     * so, nothing to parse.
                     */
                    Status = STATUS_SUCCESS;
                    break;
                }

                /* In case we had them all, start browsing for devices */
                if (Status != STATUS_MORE_ENTRIES)
                {
                    DirInfo = Buffer;

                    /* Loop until we find the nul entry (terminating entry) */
                    while (TRUE)
                    {
                        /* It's an entry full of zeroes */
                        if (RtlCompareMemory(&NullEntry, DirInfo, sizeof(NullEntry)) == sizeof(NullEntry))
                        {
                            break;
                        }

                        /* Only handle symlinks */
                        if (!wcscmp(DirInfo->TypeName.Buffer, L"SymbolicLink"))
                        {
                            TRACE("Name: '%wZ'\n", &DirInfo->Name);

                            /*
                             * Now, we previously already browsed ??, and we
                             * don't want to devices twice, so we'll check
                             * the output buffer for duplicates.
                             * We'll add our entry only if we don't have already
                             * returned it.
                             */
                            if (FindSymbolicLinkEntry(DirInfo->Name.Buffer,
                                                      lpTargetPath,
                                                      TotalEntries,
                                                      &Found) == ERROR_SUCCESS &&
                                !Found)
                            {
                                /* Get name length in chars to only comparisons */
                                NameLength = DirInfo->Name.Length / sizeof(WCHAR);

                                /* Make sure we don't overrun output buffer */
                                if (Length > ucchMax ||
                                    NameLength > ucchMax - Length ||
                                    ucchMax - NameLength - Length < sizeof(WCHAR))
                                {
                                    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
                                    NtClose(DirectoryHandle);
                                    BaseSetLastNTError(STATUS_BUFFER_TOO_SMALL);
                                    return 0;
                                }

                                /* Copy and NULL terminate string */
                                memcpy(Ptr, DirInfo->Name.Buffer, DirInfo->Name.Length);
                                Ptr[NameLength] = UNICODE_NULL;

                                Ptr += (NameLength + 1);
                                Length += (NameLength + 1);
                            }
                        }

                        /* Move to the next entry */
                        ++DirInfo;
                    }

                    /* No need to loop again here, we got all the entries */
                    break;
                }

                /* Failure path here, we'll need bigger buffer */
                RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
                Buffer = NULL;

                /* We can't have bigger than that one, so leave */
                if (BufferLength == MAXULONG)
                {
                    break;
                }

                /* Prevent any overflow while computing new size */
                if (MAXULONG - BufferLength < BufferLength)
                {
                    BufferLength = MAXULONG;
                }
                else
                {
                    BufferLength *= 2;
                }
            }

            /*
             * Out of the hot loop, but with more entries left?
             * that's an error case, leave here!
             */
            if (Status == STATUS_MORE_ENTRIES)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                _SEH2_LEAVE;
            }
        }

        /* If we failed somewhere, just leave */
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        /* If we returned no entries, time to write the empty string */
        if (Length == 0)
        {
            /* Unless output buffer is too small! */
            if (ucchMax <= 0)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                _SEH2_LEAVE;
            }

            /* Emptry string is one char (terminator!) */
            *Ptr = UNICODE_NULL;
            ++Ptr;
            Length = 1;
        }

        /*
         * If we have enough room, we need to double terminate the buffer:
         * that's a MULTI_SZ buffer, its end is marked by double NULL.
         * One was already added during the "copy string" process.
         * If we don't have enough room: that's a failure case.
         */
        if (Length < ucchMax)
        {
            *Ptr = UNICODE_NULL;
            ++Ptr;
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    _SEH2_FINALLY
    {
        if (DirectoryHandle != 0)
        {
            NtClose(DirectoryHandle);
        }

        if (Buffer != NULL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
        }

        if (!NT_SUCCESS(Status))
        {
            Length = 0;
            BaseSetLastNTError(Status);
        }
    }
    _SEH2_END;

    return Length;
}

/* EOF */
