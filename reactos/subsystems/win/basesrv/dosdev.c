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

typedef struct _BASE_DOS_DEVICE_HISTORY_ENTRY
{
    LIST_ENTRY     Entry;
    UNICODE_STRING Device;
    UNICODE_STRING Target;
} BASE_DOS_DEVICE_HISTORY_ENTRY, *PBASE_DOS_DEVICE_HISTORY_ENTRY;

static RTL_CRITICAL_SECTION BaseDefineDosDeviceCritSec;
static LIST_ENTRY DosDeviceHistory;

/* PRIVATE FUNCTIONS **********************************************************/

VOID BaseInitDefineDosDevice(VOID)
{
    RtlInitializeCriticalSection(&BaseDefineDosDeviceCritSec);
    InitializeListHead(&DosDeviceHistory);
}

VOID BaseCleanupDefineDosDevice(VOID)
{
    PLIST_ENTRY Entry, ListHead;
    PBASE_DOS_DEVICE_HISTORY_ENTRY HistoryEntry;

    RtlDeleteCriticalSection(&BaseDefineDosDeviceCritSec);

    ListHead = &DosDeviceHistory;
    Entry = ListHead->Flink;
    while (Entry != ListHead)
    {
        HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
            CONTAINING_RECORD(Entry,
                              BASE_DOS_DEVICE_HISTORY_ENTRY,
                              Entry);
        Entry = Entry->Flink;

        if (HistoryEntry)
        {
            if (HistoryEntry->Target.Buffer)
            {
                RtlFreeHeap(BaseSrvHeap,
                            0,
                            HistoryEntry->Target.Buffer);
            }
            if (HistoryEntry->Device.Buffer)
            {
                RtlFreeHeap(BaseSrvHeap,
                            0,
                            HistoryEntry->Device.Buffer);
            }
            RtlFreeHeap(BaseSrvHeap,
                        0,
                        HistoryEntry);
        }
    }
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvDefineDosDevice)
{
    NTSTATUS Status;
    PBASE_DEFINE_DOS_DEVICE DefineDosDeviceRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.DefineDosDeviceRequest;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle = NULL;
    UNICODE_STRING DeviceName = {0};
    UNICODE_STRING RequestDeviceName = {0};
    UNICODE_STRING LinkTarget = {0};
    PUNICODE_STRING RequestLinkTarget;
    ULONG Length;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;
    PSID AdminSid;
    PSID SystemSid;
    PSID WorldSid;
    ULONG SidLength;
    PBASE_DOS_DEVICE_HISTORY_ENTRY HistoryEntry;
    PLIST_ENTRY Entry;
    PLIST_ENTRY ListHead;
    BOOLEAN Matched, AddHistory;
    DWORD dwFlags;
    PWSTR lpBuffer;

    DPRINT("BaseSrvDefineDosDevice entered, Flags:%d, DeviceName:%wZ, TargetPath:%wZ\n",
           DefineDosDeviceRequest->Flags,
           &DefineDosDeviceRequest->DeviceName,
           &DefineDosDeviceRequest->TargetPath);

    Matched = AddHistory = FALSE;
    HistoryEntry = NULL;
    AdminSid = SystemSid = WorldSid = NULL;
    SecurityDescriptor = NULL;
    ListHead = &DosDeviceHistory;
    dwFlags = DefineDosDeviceRequest->Flags;

    /* Validate the flags */
    if ( (dwFlags & 0xFFFFFFF0) ||
        ((dwFlags & DDD_EXACT_MATCH_ON_REMOVE) &&
            !(dwFlags & DDD_REMOVE_DEFINITION)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlEnterCriticalSection(&BaseDefineDosDeviceCritSec);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlEnterCriticalSection() failed (Status %lx)\n",
                Status);
        return Status;
    }

    _SEH2_TRY
    {
        Status =
            RtlUpcaseUnicodeString(&RequestDeviceName,
                                   &DefineDosDeviceRequest->DeviceName,
                                   TRUE);
        if (!NT_SUCCESS(Status))
            _SEH2_LEAVE;

        RequestLinkTarget = &DefineDosDeviceRequest->TargetPath;
        lpBuffer = (PWSTR)RtlAllocateHeap(BaseSrvHeap,
                                          HEAP_ZERO_MEMORY,
                                          RequestDeviceName.MaximumLength + 5 * sizeof(WCHAR));
        if (!lpBuffer)
        {
            DPRINT1("Failed to allocate memory\n");
            Status = STATUS_NO_MEMORY;
            _SEH2_LEAVE;
        }

        swprintf(lpBuffer,
                 L"\\??\\%wZ",
                 &RequestDeviceName);
        RtlInitUnicodeString(&DeviceName,
                             lpBuffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                          DELETE | 0x1,
                                          &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            Status = NtQuerySymbolicLinkObject(LinkHandle,
                                               &LinkTarget,
                                               &Length);
            if (!NT_SUCCESS(Status) &&
                Status == STATUS_BUFFER_TOO_SMALL)
            {
                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = Length;
                LinkTarget.Buffer = (PWSTR)
                    RtlAllocateHeap(BaseSrvHeap,
                                    HEAP_ZERO_MEMORY,
                                    Length);
                if (!LinkTarget.Buffer)
                {
                    DPRINT1("Failed to allocate memory\n");
                    Status = STATUS_NO_MEMORY;
                    _SEH2_LEAVE;
                }

                Status = NtQuerySymbolicLinkObject(LinkHandle,
                                                   &LinkTarget,
                                                   &Length);
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtQuerySymbolicLinkObject(%wZ) failed (Status %lx)\n",
                     &DeviceName, Status);
                _SEH2_LEAVE;
            }

            if ((dwFlags & DDD_REMOVE_DEFINITION))
            {
                /* If no target name specified we remove the current symlink target */
                if (RequestLinkTarget->Length == 0)
                    Matched = TRUE;
                else
                {
                    if (dwFlags & DDD_EXACT_MATCH_ON_REMOVE)
                        Matched = !RtlCompareUnicodeString(RequestLinkTarget,
                                                           &LinkTarget,
                                                           TRUE);
                    else
                        Matched = RtlPrefixUnicodeString(RequestLinkTarget,
                                                         &LinkTarget,
                                                         TRUE);
                }

                if (Matched && IsListEmpty(ListHead))
                {
                    /* Current symlink target macthed and there is nothing to revert to */
                    RequestLinkTarget = NULL;
                }
                else if (Matched && !IsListEmpty(ListHead))
                {
                    /*
                     * Fetch the first history entry we come across for the device name.
                     * This will become the current symlink target for the device name.
                     */
                    Matched = FALSE;
                    Entry = ListHead->Flink;
                    while (Entry != ListHead)
                    {
                        HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                            CONTAINING_RECORD(Entry,
                                              BASE_DOS_DEVICE_HISTORY_ENTRY,
                                              Entry);
                        Matched =
                            !RtlCompareUnicodeString(&RequestDeviceName,
                                                     &HistoryEntry->Device,
                                                     FALSE);
                        if (Matched)
                        {
                            RemoveEntryList(&HistoryEntry->Entry);
                            RequestLinkTarget = &HistoryEntry->Target;
                            break;
                        }
                        Entry = Entry->Flink;
                        HistoryEntry = NULL;
                    }

                    /* Nothing to revert to so delete the symlink */
                    if (!Matched)
                        RequestLinkTarget = NULL;
                }
                else if (!Matched)
                {
                    /*
                     * Locate a previous symlink target as we did not get
                     * a hit earlier. If we find one we need to remove it.
                     */
                    Entry = ListHead->Flink;
                    while (Entry != ListHead)
                    {
                        HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                            CONTAINING_RECORD(Entry,
                                              BASE_DOS_DEVICE_HISTORY_ENTRY,
                                              Entry);
                        Matched =
                            !RtlCompareUnicodeString(&RequestDeviceName,
                                                     &HistoryEntry->Device,
                                                     FALSE);
                        if (!Matched)
                        {
                            HistoryEntry = NULL;
                            Entry = Entry->Flink;
                            continue;
                        }

                        Matched = FALSE;
                        if (dwFlags & DDD_EXACT_MATCH_ON_REMOVE)
                        {
                            if (!RtlCompareUnicodeString(RequestLinkTarget,
                                                         &HistoryEntry->Target,
                                                         TRUE))
                            {
                                Matched = TRUE;
                            }
                        }
                        else if (RtlPrefixUnicodeString(RequestLinkTarget,
                                                        &HistoryEntry->Target,
                                                        TRUE))
                        {
                            Matched = TRUE;
                        }

                        if (Matched)
                        {
                            RemoveEntryList(&HistoryEntry->Entry);
                            break;
                        }
                        Entry = Entry->Flink;
                        HistoryEntry = NULL;
                    }

                    /* Leave existing symlink as is */
                    if (!Matched)
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    else
                        Status = STATUS_SUCCESS;
                    _SEH2_LEAVE;
                }
            }
            else
            {
                AddHistory = TRUE;
            }

            Status = NtMakeTemporaryObject(LinkHandle);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtMakeTemporaryObject(%wZ) failed (Status %lx)\n",
                     &DeviceName, Status);
                _SEH2_LEAVE;
            }

            Status = NtClose(LinkHandle);
            LinkHandle = NULL;
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtClose(%wZ) failed (Status %lx)\n",
                     &DeviceName, Status);
                _SEH2_LEAVE;
            }
        }

        /* Don't create symlink if we don't have a target */
        if (!RequestLinkTarget || RequestLinkTarget->Length == 0)
            _SEH2_LEAVE;

        if (AddHistory)
        {
            HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                RtlAllocateHeap(BaseSrvHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(BASE_DOS_DEVICE_HISTORY_ENTRY));
            if (!HistoryEntry)
            {
                DPRINT1("Failed to allocate memory\n");
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }

            HistoryEntry->Target.Buffer =
                RtlAllocateHeap(BaseSrvHeap,
                                HEAP_ZERO_MEMORY,
                                LinkTarget.Length);
            if (!HistoryEntry->Target.Buffer)
            {
                DPRINT1("Failed to allocate memory\n");
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }
            HistoryEntry->Target.Length =
                HistoryEntry->Target.MaximumLength =
                    LinkTarget.Length;
            RtlCopyUnicodeString(&HistoryEntry->Target,
                                 &LinkTarget);

            HistoryEntry->Device.Buffer =
                RtlAllocateHeap(BaseSrvHeap,
                                HEAP_ZERO_MEMORY,
                                RequestDeviceName.Length);
            if (!HistoryEntry->Device.Buffer)
            {
                DPRINT1("Failed to allocate memory\n");
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }
            HistoryEntry->Device.Length =
                HistoryEntry->Device.MaximumLength =
                    RequestDeviceName.Length;
            RtlCopyUnicodeString(&HistoryEntry->Device,
                                 &RequestDeviceName);

            /* Remember previous symlink target for this device */
            InsertHeadList(ListHead,
                           &HistoryEntry->Entry);
            HistoryEntry = NULL;
        }

        RtlAllocateAndInitializeSid(&WorldAuthority,
                                    1,
                                    SECURITY_WORLD_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    &WorldSid);

        RtlAllocateAndInitializeSid(&SystemAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    &SystemSid);

        RtlAllocateAndInitializeSid(&SystemAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    &AdminSid);

        SidLength = RtlLengthSid(SystemSid) +
                    RtlLengthSid(AdminSid) +
                    RtlLengthSid(WorldSid);
        Length = sizeof(ACL) + SidLength + 3 * sizeof(ACCESS_ALLOWED_ACE);

        SecurityDescriptor = RtlAllocateHeap(BaseSrvHeap,
                                             0,
                                             SECURITY_DESCRIPTOR_MIN_LENGTH + Length);
        if (!SecurityDescriptor)
        {
            DPRINT1("Failed to allocate memory\n");
            Status = STATUS_NO_MEMORY;
            _SEH2_LEAVE;
        }

        Dacl = (PACL)((ULONG_PTR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
        Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlCreateSecurityDescriptor() failed (Status %lx)\n", Status);
            _SEH2_LEAVE;
        }

        Status = RtlCreateAcl(Dacl,
                              Length,
                              ACL_REVISION);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlCreateAcl() failed (Status %lx)\n", Status);
            _SEH2_LEAVE;
        }

        RtlAddAccessAllowedAce(Dacl,
                               ACL_REVISION,
                               GENERIC_ALL,
                               SystemSid);
        RtlAddAccessAllowedAce(Dacl,
                               ACL_REVISION,
                               GENERIC_ALL,
                               AdminSid);
        RtlAddAccessAllowedAce(Dacl,
                               ACL_REVISION,
                               STANDARD_RIGHTS_READ,
                               WorldSid);

        Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                              TRUE,
                                              Dacl,
                                              FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlSetDaclSecurityDescriptor() failed (Status %lx)\n", Status);
            _SEH2_LEAVE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   SecurityDescriptor);
        Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            RequestLinkTarget);
        if (NT_SUCCESS(Status))
        {
            Status = NtMakePermanentObject(LinkHandle);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtMakePermanentObject(%wZ) failed (Status %lx)\n",
                        &DeviceName, Status);
            }
        }
        else
        {
            DPRINT1("NtCreateSymbolicLinkObject(%wZ) failed (Status %lx)\n",
                 &DeviceName, Status);
        }
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&BaseDefineDosDeviceCritSec);
        if (DeviceName.Buffer)
        {
            RtlFreeHeap(BaseSrvHeap,
                        0,
                        DeviceName.Buffer);
        }
        if (LinkTarget.Buffer)
        {
            RtlFreeHeap(BaseSrvHeap,
                        0,
                        LinkTarget.Buffer);
        }
        if (SecurityDescriptor)
        {
            RtlFreeHeap(BaseSrvHeap,
                        0,
                        SecurityDescriptor);
        }

        if (LinkHandle) NtClose(LinkHandle);
        if (SystemSid)  RtlFreeSid(SystemSid);
        if (AdminSid)   RtlFreeSid(AdminSid);
        if (WorldSid)   RtlFreeSid(WorldSid);

        RtlFreeUnicodeString(&RequestDeviceName);

        if (HistoryEntry)
        {
            if (HistoryEntry->Target.Buffer)
            {
                RtlFreeHeap(BaseSrvHeap,
                            0,
                            HistoryEntry->Target.Buffer);
            }
            if (HistoryEntry->Device.Buffer)
            {
                RtlFreeHeap(BaseSrvHeap,
                            0,
                            HistoryEntry->Device.Buffer);
            }
            RtlFreeHeap(BaseSrvHeap,
                        0,
                        HistoryEntry);
        }
    }
    _SEH2_END

    DPRINT("BaseSrvDefineDosDevice exit, Status: 0x%x\n", Status);
    return Status;
}

/* EOF */
