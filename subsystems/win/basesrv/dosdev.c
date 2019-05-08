/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/dosdev.c
 * PURPOSE:         DOS Devices Management
 * PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static RTL_CRITICAL_SECTION BaseDefineDosDeviceCritSec;

/* PRIVATE FUNCTIONS **********************************************************/

VOID BaseInitDefineDosDevice(VOID)
{
    RtlInitializeCriticalSection(&BaseDefineDosDeviceCritSec);
}

VOID BaseCleanupDefineDosDevice(VOID)
{
    RtlDeleteCriticalSection(&BaseDefineDosDeviceCritSec);
}

NTSTATUS
GetCallerLuid(PLUID CallerLuid)
{
    NTSTATUS Status;
    HANDLE TokenHandle;
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenInformation;

    /* We need an output buffer */
    if (CallerLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Open thread token */
    TokenHandle = 0;
    ReturnLength = 0;
    Status = NtOpenThreadToken(NtCurrentThread(),
                               READ_CONTROL | TOKEN_QUERY,
                               FALSE, &TokenHandle);
    /* If we fail, open process token */
    if (Status == STATUS_NO_TOKEN)
    {
        Status = NtOpenProcessToken(NtCurrentProcess(),
                                    READ_CONTROL | TOKEN_QUERY,
                                    &TokenHandle);
    }

    /* In case of a success get caller LUID and copy it back */
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryInformationToken(TokenHandle,
                                         TokenStatistics,
                                         &TokenInformation,
                                         sizeof(TokenInformation),
                                         &ReturnLength);
        if (NT_SUCCESS(Status))
        {
            RtlCopyLuid(CallerLuid, &TokenInformation.AuthenticationId);
        }
    }

    /* Close token handle */
    if (TokenHandle != 0)
    {
        NtClose(TokenHandle);
    }

    return Status;
}

NTSTATUS
IsGlobalSymbolicLink(HANDLE LinkHandle,
                     PBOOLEAN IsGlobal)
{
    NTSTATUS Status;
    DWORD ReturnLength;
    UNICODE_STRING GlobalString;
    OBJECT_NAME_INFORMATION NameInfo, *PNameInfo;

    /* We need both parameters */
    if (LinkHandle == 0 || IsGlobal == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PNameInfo = NULL;
    _SEH2_TRY
    {
        /* Query handle information */
        Status = NtQueryObject(LinkHandle,
                               ObjectNameInformation,
                               &NameInfo,
                               0,
                               &ReturnLength);
        /* Only failure we tolerate is length mismatch */
        if (NT_SUCCESS(Status) || Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            /* Allocate big enough buffer */
            PNameInfo = RtlAllocateHeap(BaseSrvHeap, 0, ReturnLength);
            if (PNameInfo == NULL)
            {
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }

            /* Query again handle information */
            Status = NtQueryObject(LinkHandle,
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
                *IsGlobal = RtlPrefixUnicodeString(&GlobalString, &PNameInfo->Name, FALSE);
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

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvDefineDosDevice)
{
    NTSTATUS Status;
    PBASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.DefineDosDeviceRequest;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;
    UNICODE_STRING DeviceName = {0};
    UNICODE_STRING LinkTarget = {0};
    ULONG Length;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PSID SystemSid;
    PSID WorldSid;
    PWSTR lpBuffer;
    WCHAR Letter;
    SHORT AbsLetter;
    BOOLEAN DriveLetter = FALSE;
    BOOLEAN RemoveDefinition;
    BOOLEAN HandleTarget;
    BOOLEAN HandleSMB = FALSE;
    BOOLEAN IsGlobal = FALSE;
    ULONG CchLengthLeft;
    ULONG CchLength;
    ULONG TargetLength;
    PWSTR TargetBuffer;
    PWSTR CurrentBuffer;
    /* We store them on the stack, they are known in advance */
    union {
        SECURITY_DESCRIPTOR SecurityDescriptor;
        UCHAR Buffer[20];
    } SecurityDescriptor;
    union {
        ACL Dacl;
        UCHAR Buffer[256];
    } Dacl;
    ACCESS_MASK AccessMask;
    LUID CallerLuid;
    WCHAR * CurrentPtr;
    WCHAR CurrentChar;
    PWSTR OrigPtr;
    PWSTR InterPtr;
    BOOLEAN RemoveFound;

#if 0
    /* FIXME: Check why it fails.... */
    if (!CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&DefineDosDeviceRequest->DeviceName,
                                  DefineDosDeviceRequest->DeviceName.Length,
                                  1) ||
        (DefineDosDeviceRequest->DeviceName.Length & 1) != 0 ||
        !CsrValidateMessageBuffer(ApiMessage,
                                  (PVOID*)&DefineDosDeviceRequest->TargetPath,
                                  (DefineDosDeviceRequest->TargetPath.Length != 0 ? sizeof(UNICODE_NULL) : 0) + DefineDosDeviceRequest->TargetPath.Length,
                                  1) ||
        (DefineDosDeviceRequest->TargetPath.Length & 1) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }
#endif

    DPRINT1("BaseSrvDefineDosDevice entered, Flags:%d, DeviceName:%wZ (%d), TargetPath:%wZ (%d)\n",
           DefineDosDeviceRequest->Flags,
           &DefineDosDeviceRequest->DeviceName,
           DefineDosDeviceRequest->DeviceName.Length,
           &DefineDosDeviceRequest->TargetPath,
           DefineDosDeviceRequest->TargetPath.Length);

    /*
     * Allocate a buffer big enough to contain:
     * - device name
     * - targets
     */
    lpBuffer = RtlAllocateHeap(BaseSrvHeap, 0, 0x2000);
    if (lpBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    /* Enter our critical section */
    Status = RtlEnterCriticalSection(&BaseDefineDosDeviceCritSec);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlEnterCriticalSection() failed (Status %lx)\n",
                Status);
        RtlFreeHeap(BaseSrvHeap, 0, lpBuffer);
        return Status;
    }

    LinkHandle = 0;
    /* Does the caller wants to remove definition? */
    RemoveDefinition = !!(DefineDosDeviceRequest->Flags & DDD_REMOVE_DEFINITION);
    _SEH2_TRY
    {
        /* First of all, check if that's a drive letter device amongst LUID mappings */
        if (BaseStaticServerData->LUIDDeviceMapsEnabled && !(DefineDosDeviceRequest->Flags & DDD_NO_BROADCAST_SYSTEM))
        {
            if (DefineDosDeviceRequest->DeviceName.Buffer != NULL &&
                DefineDosDeviceRequest->DeviceName.Length == 2 * sizeof(WCHAR) &&
                DefineDosDeviceRequest->DeviceName.Buffer[1] == L':')
            {
                Letter = DefineDosDeviceRequest->DeviceName.Buffer[0];

                /* Handle both lower cases and upper cases */
                AbsLetter = Letter - L'a';
                if (AbsLetter < 26 && AbsLetter >= 0)
                {
                    Letter = RtlUpcaseUnicodeChar(Letter);
                }

                AbsLetter = Letter - L'A';
                if (AbsLetter < 26)
                {
                    /* That's a letter! */
                    DriveLetter = TRUE;
                }
            }
        }

        /* We can only broadcast drive letters in case of LUID mappings */
        if (DefineDosDeviceRequest->Flags & DDD_LUID_BROADCAST_DRIVE &&
            !DriveLetter)
        {
            Status = STATUS_INVALID_PARAMETER;
            _SEH2_LEAVE;
        }

        /* First usage of our buffer: create device name */
        CchLength = _snwprintf(lpBuffer, 0x1000, L"\\??\\%wZ", &DefineDosDeviceRequest->DeviceName);
        CchLengthLeft = 0x1000 - 1 - CchLength; /* UNICODE_NULL */
        CurrentBuffer = lpBuffer + CchLength + 1; /* UNICODE_NULL */
        RtlInitUnicodeString(&DeviceName, lpBuffer);

        /* And prepare to open it */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        /* Assume it's OK and has a target to deal with */
        HandleTarget = TRUE;

        /* Move to the client context if the mapping was local */
        if (!CsrImpersonateClient(NULL))
        {
            Status = STATUS_BAD_IMPERSONATION_LEVEL;
            _SEH2_LEAVE;
        }

        /* While impersonating the caller, also get its LUID */
        if (DriveLetter)
        {
            Status = GetCallerLuid(&CallerLuid);
            if (NT_SUCCESS(Status))
            {
                HandleSMB = TRUE;
            }
        }

        /* Now, open the device */
        Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                          DELETE | SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes);

        /* And get back to our context */
        CsrRevertToSelf();

        /* In case of LUID broadcast, do nothing but return to trigger broadcast */
        if (DefineDosDeviceRequest->Flags & DDD_LUID_BROADCAST_DRIVE)
        {
            /* Zero handle in case of a failure */
            if (!NT_SUCCESS(Status))
            {
                LinkHandle = 0;
            }

            /* If removal was asked, and no object found: the remval was successful */
            if (RemoveDefinition && Status == STATUS_OBJECT_NAME_NOT_FOUND)
            {
                Status = STATUS_SUCCESS;
            }

            /* We're done here, nothing more to do */
            _SEH2_LEAVE;
        }

        /* If device was not found */
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* No handle */
            LinkHandle = 0;

            /* If we were asked to remove... */
            if (RemoveDefinition)
            {
                /*
                 * If caller asked to pop first entry, nothing specific,
                 * then, we can consider this as a success
                 */
                if (DefineDosDeviceRequest->TargetPath.Length == 0)
                {
                    Status = STATUS_SUCCESS;
                }

                /* We're done, nothing to change */
                _SEH2_LEAVE;
            }

            /* There's no target to handle */
            HandleTarget = FALSE;

            /*
             * We'll consider, that's a success
             * Failing to open the device doesn't prevent
             * from creating it later on to create
             * the linking.
             */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Unexpected failure, forward to caller */
            if (!NT_SUCCESS(Status))
            {
                _SEH2_LEAVE;
            }

            /* If LUID mapping enabled */
            if (BaseStaticServerData->LUIDDeviceMapsEnabled)
            {
                /* Check if that's global link */
                Status = IsGlobalSymbolicLink(LinkHandle, &IsGlobal);
                if (!NT_SUCCESS(Status))
                {
                    _SEH2_LEAVE;
                }

                /* If so, change our device name namespace to GLOBAL?? for link creation */
                if (IsGlobal)
                {
                    CchLength = _snwprintf(lpBuffer, 0x1000, L"\\GLOBAL??\\%wZ", &DefineDosDeviceRequest->DeviceName);
                    CchLengthLeft = 0x1000 - 1 - CchLength; /* UNICODE_NULL */
                    CurrentBuffer = lpBuffer + CchLength + 1; /* UNICODE_NULL */

                    DeviceName.Length = CchLength * sizeof(WCHAR);
                    DeviceName.MaximumLength = CchLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
                }
            }
        }

        /* If caller provided a target */
        if (DefineDosDeviceRequest->TargetPath.Length != 0)
        {
            /* Make sure it's null terminated */
            DefineDosDeviceRequest->TargetPath.Buffer[DefineDosDeviceRequest->TargetPath.Length / sizeof(WCHAR)] = UNICODE_NULL;

            /* Compute its size */
            TargetLength = wcslen(DefineDosDeviceRequest->TargetPath.Buffer);

            /* And make sure it fits our buffer */
            if (TargetLength + 1 >= CchLengthLeft)
            {
                Status = STATUS_INVALID_PARAMETER;
                _SEH2_LEAVE;
            }

            /* Copy it to our internal buffer */
            RtlMoveMemory(CurrentBuffer, DefineDosDeviceRequest->TargetPath.Buffer, TargetLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            TargetBuffer = CurrentBuffer;

            /* Update our buffer status */
            CchLengthLeft -= (TargetLength + 1);
            CurrentBuffer += (TargetLength + 1);
        }
        /* Otherwise, zero everything */
        else
        {
            TargetBuffer = NULL;
            TargetLength = 0;
        }

        /* If we opened the device, then, handle its current target */
        if (HandleTarget)
        {
            /* Query it with our internal buffer */
            LinkTarget.Length = 0;
            LinkTarget.MaximumLength = CchLengthLeft * sizeof(WCHAR);
            LinkTarget.Buffer = CurrentBuffer;

            Status = NtQuerySymbolicLinkObject(LinkHandle,
                                               &LinkTarget,
                                               &Length);
            /* If we overflow, give up */
            if (Length == LinkTarget.MaximumLength)
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            /* In case of a failure, bye bye */
            if (!NT_SUCCESS(Status))
            {
                _SEH2_LEAVE;
            }

            /*
             * Properly null it for MULTI_SZ if needed
             * Always update max length with
             * the need size
             * This is needed to hand relatively "small"
             * strings to Ob and avoid killing ourselves
             * on the next query
             */
            CchLength = Length / sizeof(WCHAR);
            if (CchLength < 2 ||
                CurrentBuffer[CchLength - 2] != UNICODE_NULL ||
                CurrentBuffer[CchLength - 1] != UNICODE_NULL)
            {
                CurrentBuffer[CchLength] = UNICODE_NULL;
                LinkTarget.MaximumLength = Length + sizeof(UNICODE_NULL);
            }
            else
            {
                LinkTarget.MaximumLength = Length;
            }
        }
        /* There's no target, and we're asked to remove, so null target */
        else if (RemoveDefinition)
        {
            RtlInitUnicodeString(&LinkTarget, NULL);
        }
        /* There's a target provided - new device, update buffer */
        else
        {
            RtlInitUnicodeString(&LinkTarget, &CurrentBuffer[-TargetLength - 1]);
        }

        /*
         * We no longer need old symlink, just drop it, we'll recreate it now
         * with updated target.
         * The benefit of it is that if caller asked us to drop last target, then
         * the device is removed and not dangling
         */
        if (LinkHandle != 0)
        {
            Status = NtMakeTemporaryObject(LinkHandle);
            NtClose(LinkHandle);
            LinkHandle = 0;
        }

        /* At this point, we must have no failure */
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        /*
         * If we have to remove definition, let's start to browse our
         * target to actually drop it.
         */
        if (RemoveDefinition)
        {
            /* We'll browse our multi sz string */
            RemoveFound = FALSE;
            CurrentPtr = LinkTarget.Buffer;
            InterPtr = LinkTarget.Buffer;
            while (*CurrentPtr != UNICODE_NULL)
            {
                CchLength = 0;
                OrigPtr = CurrentPtr;
                /* First, find next string */
                while (TRUE)
                {
                    CurrentChar = *CurrentPtr;
                    ++CurrentPtr;

                    if (CurrentChar == UNICODE_NULL)
                    {
                        break;
                    }

                    ++CchLength;
                }

                /* This check is a bit tricky, but dead useful:
                 * If on the previous loop, we found the caller provided target
                 * in our list, then, we'll move current entry over the found one
                 * So that, it gets deleted.
                 * Also, if we don't find caller entry in our entries, then move
                 * current entry in the string if a previous one got deleted
                 */
                if (RemoveFound ||
                    ((!(DefineDosDeviceRequest->Flags & DDD_EXACT_MATCH_ON_REMOVE) ||
                      TargetLength != CchLength || _wcsicmp(OrigPtr, TargetBuffer) != 0) &&
                     ((DefineDosDeviceRequest->Flags & DDD_EXACT_MATCH_ON_REMOVE) ||
                      (TargetLength != 0 && _wcsnicmp(OrigPtr, TargetBuffer, TargetLength) != 0))))
                {
                    if (InterPtr != OrigPtr)
                    {
                        RtlMoveMemory(InterPtr, OrigPtr, sizeof(WCHAR) * CchLength + sizeof(UNICODE_NULL));
                    }

                    InterPtr += (CchLength + 1);
                }
                else
                {
                    /* Match case! Remember for next loop turn and to delete it */
                    RemoveFound = TRUE;
                }
            }

            /*
             * Drop last entry, as required (pop)
             * If there was a match previously, everything
             * is already moved, so we're just nulling
             * the end of the string
             * If there was no match, this is the pop
             */
            *InterPtr = UNICODE_NULL;
            ++InterPtr;

            /* Compute new target length */
            TargetLength = wcslen(LinkTarget.Buffer) * sizeof(WCHAR);
            /*
             * If it's empty, quit
             * Beware, here, we quit with STATUS_SUCCESS, and that's expected!
             * In case we dropped last target entry, then, it's empty
             * and there's no need to recreate the device we deleted previously
             */
            if (TargetLength == 0)
            {
                _SEH2_LEAVE;
            }

            /* Update our target string */
            LinkTarget.Length = TargetLength;
            LinkTarget.MaximumLength = (ULONG_PTR)InterPtr - (ULONG_PTR)LinkTarget.Buffer;
        }
        /* If that's not a removal, just update the target to include new target */
        else if (HandleTarget)
        {
            LinkTarget.Buffer = LinkTarget.Buffer - TargetLength - 1;
            LinkTarget.Length = TargetLength * sizeof(WCHAR);
            LinkTarget.MaximumLength += (TargetLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            TargetLength *= sizeof(WCHAR);
        }
        /* No changes */
        else
        {
            TargetLength = LinkTarget.Length;
        }

        /* Make sure we don't create empty symlink */
        if (TargetLength == 0)
        {
            _SEH2_LEAVE;
        }

        /* Initialize our SIDs for symlink ACLs */
        Status = RtlAllocateAndInitializeSid(&WorldAuthority,
                                             1,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             &WorldSid);
        if (!NT_SUCCESS(Status))
        {
            _SEH2_LEAVE;
        }

        Status = RtlAllocateAndInitializeSid(&SystemAuthority,
                                             1,
                                             SECURITY_RESTRICTED_CODE_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             SECURITY_NULL_RID,
                                             &SystemSid);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeSid(WorldSid);
            _SEH2_LEAVE;
        }

        /* Initialize our SD (on stack) */
        RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                    SECURITY_DESCRIPTOR_REVISION);

        /* And our ACL (still on stack) */
        RtlCreateAcl(&Dacl.Dacl, sizeof(Dacl), ACL_REVISION);

        /*
         * For access mask, if we have no session ID, or if
         * protection mode is disabled, make them wide open
         */
        if (SessionId == 0 ||
            (ProtectionMode & 3) == 0)
        {
            AccessMask = DELETE | SYMBOLIC_LINK_QUERY;
        }
        else
        {
            AccessMask = SYMBOLIC_LINK_QUERY;
        }

        /* Setup the ACL */
        RtlAddAccessAllowedAce(&Dacl.Dacl, ACL_REVISION2, AccessMask, WorldSid);
        RtlAddAccessAllowedAce(&Dacl.Dacl, ACL_REVISION2, AccessMask, SystemSid);

        /* Drop SIDs */
        RtlFreeSid(WorldSid);
        RtlFreeSid(SystemSid);

        /* Link DACL to the SD */
        RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, &Dacl.Dacl, TRUE);

        /* And set it in the OA used for creation */
        ObjectAttributes.SecurityDescriptor = &SecurityDescriptor;

        /*
         * If LUID and not global, we need to impersonate the caller
         * to make it local.
         */
        if (BaseStaticServerData->LUIDDeviceMapsEnabled)
        {
            if (!IsGlobal)
            {
                if (!CsrImpersonateClient(NULL))
                {
                    Status = STATUS_BAD_IMPERSONATION_LEVEL;
                    _SEH2_LEAVE;
                }
            }
        }
        /* The object will be permanent */
        else
        {
            ObjectAttributes.Attributes |= OBJ_PERMANENT;
        }

        /* (Re)Create the symbolic link/device */
        Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &LinkTarget);

        /* Revert to self if required */
        if (BaseStaticServerData->LUIDDeviceMapsEnabled && !IsGlobal)
        {
            CsrRevertToSelf();
        }

        /* In case of a success, make object permanent for LUID links */
        if (NT_SUCCESS(Status))
        {
            if (BaseStaticServerData->LUIDDeviceMapsEnabled)
            {
                Status = NtMakePermanentObject(LinkHandle);
            }

            /* Close the link */
            NtClose(LinkHandle);

            /*
             * Specific failure case here:
             * We were asked to remove something
             * but we didn't find the something
             * (we recreated the symlink hence the fail here!)
             * so fail with appropriate status
             */
            if (RemoveDefinition && !RemoveFound)
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }

        /* We closed link, don't double close */
        LinkHandle = 0;
    }
    _SEH2_FINALLY
    {
        /* If we need to close the link, do it now */
        if (LinkHandle != 0)
        {
             NtClose(LinkHandle);
        }

        /* Free our internal buffer */
        RtlFreeHeap(BaseSrvHeap, 0, lpBuffer);

        /* Handle SMB */
        if (DriveLetter && Status == STATUS_SUCCESS && HandleSMB)
        {
            UNIMPLEMENTED;
        }

        /* Done! */
        RtlLeaveCriticalSection(&BaseDefineDosDeviceCritSec);
    }
    _SEH2_END;

    return Status;
}

/* EOF */
