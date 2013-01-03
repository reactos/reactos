/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS/Win32 Base enviroment Subsystem Server
 * FILE:            subsystems/win/basesrv/server.c
 * PURPOSE:         Server APIs
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "basesrv.h"

#define NDEBUG
#include <debug.h>

CSR_API(BaseSrvCreateProcess)
{
    NTSTATUS Status;
    PBASE_CREATE_PROCESS CreateProcessRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CreateProcessRequest;
    HANDLE ProcessHandle, ThreadHandle;
    PCSR_THREAD CsrThread;
    PCSR_PROCESS Process;
    ULONG Flags = 0, VdmPower = 0, DebugFlags = 0;

    /* Get the current client thread */
    CsrThread = CsrGetClientThread();
    ASSERT(CsrThread != NULL);

    Process = CsrThread->Process;

    /* Extract the flags out of the process handle */
    Flags = (ULONG_PTR)CreateProcessRequest->ProcessHandle & 3;
    CreateProcessRequest->ProcessHandle = (HANDLE)((ULONG_PTR)CreateProcessRequest->ProcessHandle & ~3);

    /* Duplicate the process handle */
    Status = NtDuplicateObject(Process->ProcessHandle,
                               CreateProcessRequest->ProcessHandle,
                               NtCurrentProcess(),
                               &ProcessHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to duplicate process handle\n");
        return Status;
    }

    /* Duplicate the thread handle */
    Status = NtDuplicateObject(Process->ProcessHandle,
                               CreateProcessRequest->ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to duplicate process handle\n");
        NtClose(ProcessHandle);
        return Status;
    }

    /* See if this is a VDM process */
    if (VdmPower)
    {
        /* Request VDM powers */
        Status = NtSetInformationProcess(ProcessHandle,
                                         ProcessWx86Information,
                                         &VdmPower,
                                         sizeof(VdmPower));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get VDM powers\n");
            NtClose(ProcessHandle);
            NtClose(ThreadHandle);
            return Status;
        }
    }

    /* Flags conversion. FIXME: More need conversion */
    if (CreateProcessRequest->CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        DebugFlags |= CsrProcessCreateNewGroup;
    }
    if ((Flags & 2) == 0)
    {
        /* We are launching a console process */
        DebugFlags |= CsrProcessIsConsoleApp;
    }

    /* FIXME: SxS Stuff */

    /* Call CSRSRV to create the CSR_PROCESS structure and the first CSR_THREAD */
    Status = CsrCreateProcess(ProcessHandle,
                              ThreadHandle,
                              &CreateProcessRequest->ClientId,
                              Process->NtSession,
                              DebugFlags,
                              NULL);
    if (Status == STATUS_THREAD_IS_TERMINATING)
    {
        DPRINT1("Thread already dead\n");

        /* Set the special reply value so we don't reply this message back */
        *ReplyCode = CsrReplyDeadClient;

        return Status;
    }

    /* Check for other failures */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process/thread structures: %lx\n", Status);
        return Status;
    }

    /* FIXME: Should notify user32 */

    /* FIXME: VDM vodoo */

    /* Return the result of this operation */
    return Status;
}

CSR_API(BaseSrvCreateThread)
{
    NTSTATUS Status;
    PBASE_CREATE_THREAD CreateThreadRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CreateThreadRequest;
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    PCSR_PROCESS CsrProcess;

    /* Get the current CSR thread */
    CurrentThread = CsrGetClientThread();
    if (!CurrentThread)
    {
        DPRINT1("Server Thread TID: [%lx.%lx]\n",
                CreateThreadRequest->ClientId.UniqueProcess,
                CreateThreadRequest->ClientId.UniqueThread);
        return STATUS_SUCCESS; // server-to-server
    }

    /* Get the CSR Process for this request */
    CsrProcess = CurrentThread->Process;
    if (CsrProcess->ClientId.UniqueProcess !=
        CreateThreadRequest->ClientId.UniqueProcess)
    {
        /* This is a remote thread request -- is it within the server itself? */
        if (CreateThreadRequest->ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            /* Accept this without any further work */
            return STATUS_SUCCESS;
        }

        /* Get the real CSR Process for the remote thread's process */
        Status = CsrLockProcessByClientId(CreateThreadRequest->ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Duplicate the thread handle so we can own it */
    Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                               CreateThreadRequest->ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (NT_SUCCESS(Status))
    {
        /* Call CSRSRV to tell it about the new thread */
        Status = CsrCreateThread(CsrProcess,
                                 ThreadHandle,
                                 &CreateThreadRequest->ClientId,
                                 TRUE);
    }

    /* Unlock the process and return */
    if (CsrProcess != CurrentThread->Process) CsrUnlockProcess(CsrProcess);
    return Status;
}

CSR_API(BaseSrvGetTempFile)
{
    static UINT CsrGetTempFileUnique = 0;
    PBASE_GET_TEMP_FILE GetTempFile = &((PBASE_API_MESSAGE)ApiMessage)->Data.GetTempFile;

    /* Return 16-bits ID */
    GetTempFile->UniqueID = (++CsrGetTempFileUnique & 0xFFFF);

    DPRINT("Returning: %u\n", GetTempFile->UniqueID);

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvExitProcess)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread != NULL);

    /* Set the special reply value so we don't reply this message back */
    *ReplyCode = CsrReplyDeadClient;

    /* Remove the CSR_THREADs and CSR_PROCESS */
    return CsrDestroyProcess(&CsrThread->ClientId,
                             (NTSTATUS)((PBASE_API_MESSAGE)ApiMessage)->Data.ExitProcessRequest.uExitCode);
}

CSR_API(BaseSrvGetProcessShutdownParam)
{
    PBASE_GET_PROCESS_SHUTDOWN_PARAMS GetShutdownParametersRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.GetShutdownParametersRequest;
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    GetShutdownParametersRequest->Level = CsrThread->Process->ShutdownLevel;
    GetShutdownParametersRequest->Flags = CsrThread->Process->ShutdownFlags;

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvSetProcessShutdownParam)
{
    PBASE_SET_PROCESS_SHUTDOWN_PARAMS SetShutdownParametersRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.SetShutdownParametersRequest;
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    CsrThread->Process->ShutdownLevel = SetShutdownParametersRequest->Level;
    CsrThread->Process->ShutdownFlags = SetShutdownParametersRequest->Flags;

    return STATUS_SUCCESS;
}


/***
 *** Sound sentry
 ***/

typedef BOOL (WINAPI *PUSER_SOUND_SENTRY)(VOID);
BOOL NTAPI FirstSoundSentry(VOID);

PUSER_SOUND_SENTRY _UserSoundSentry = FirstSoundSentry;

BOOL
NTAPI
FailSoundSentry(VOID)
{
    /* In case the function can't be found/is unimplemented */
    return FALSE;
}

BOOL
NTAPI
FirstSoundSentry(VOID)
{
    UNICODE_STRING DllString = RTL_CONSTANT_STRING(L"winsrv");
    STRING FuncString = RTL_CONSTANT_STRING("_UserSoundSentry");
    HANDLE DllHandle;
    NTSTATUS Status;
    PUSER_SOUND_SENTRY NewSoundSentry = FailSoundSentry;

    /* Load winsrv manually */
    Status = LdrGetDllHandle(NULL, NULL, &DllString, &DllHandle);
    if (NT_SUCCESS(Status))
    {
        /* If it was found, get SoundSentry export */
        Status = LdrGetProcedureAddress(DllHandle,
                                        &FuncString,
                                        0,
                                        (PVOID*)&NewSoundSentry);
    }
    
    /* Set it as the callback for the future, and call it */
    _UserSoundSentry = NewSoundSentry;
    return _UserSoundSentry();
}

CSR_API(BaseSrvSoundSentryNotification)
{
    /* Call the API and see if it succeeds */
    return _UserSoundSentry() ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
}


/***
 *** Dos Devices (C) Pierre Schweitzer (pierre.schweitzer@reactos.org)
 ***/

typedef struct tagBASE_DOS_DEVICE_HISTORY_ENTRY
{
    UNICODE_STRING Device;
    UNICODE_STRING Target;
    LIST_ENTRY Entry;
} BASE_DOS_DEVICE_HISTORY_ENTRY, *PBASE_DOS_DEVICE_HISTORY_ENTRY;

LIST_ENTRY DosDeviceHistory;
RTL_CRITICAL_SECTION BaseDefineDosDeviceCritSec;

VOID BaseCleanupDefineDosDevice(VOID)
{
    PLIST_ENTRY Entry, ListHead;
    PBASE_DOS_DEVICE_HISTORY_ENTRY HistoryEntry;

    (void) RtlDeleteCriticalSection(&BaseDefineDosDeviceCritSec);

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
                (void) RtlFreeHeap(BaseSrvHeap,
                                   0,
                                   HistoryEntry->Target.Buffer);
            if (HistoryEntry->Device.Buffer)
                (void) RtlFreeHeap(BaseSrvHeap,
                                   0,
                                   HistoryEntry->Device.Buffer);
            (void) RtlFreeHeap(BaseSrvHeap,
                               0,
                               HistoryEntry);
        }
    }
}

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

    DPRINT("CsrDefineDosDevice entered, Flags:%d, DeviceName:%wZ, TargetName:%wZ\n",
           DefineDosDeviceRequest->dwFlags,
           &DefineDosDeviceRequest->DeviceName,
           &DefineDosDeviceRequest->TargetName);

    Matched = AddHistory = FALSE;
    HistoryEntry = NULL;
    AdminSid = SystemSid = WorldSid = NULL;
    SecurityDescriptor = NULL;
    ListHead = &DosDeviceHistory;
    dwFlags = DefineDosDeviceRequest->dwFlags;

    /* Validate the flags */
    if ( (dwFlags & 0xFFFFFFF0) ||
        ((dwFlags & DDD_EXACT_MATCH_ON_REMOVE) &&
            ! (dwFlags & DDD_REMOVE_DEFINITION)) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = RtlEnterCriticalSection(&BaseDefineDosDeviceCritSec);
    if (! NT_SUCCESS(Status))
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
        if (! NT_SUCCESS(Status))
            _SEH2_LEAVE;

        RequestLinkTarget = &DefineDosDeviceRequest->TargetName;
        lpBuffer = (PWSTR) RtlAllocateHeap(BaseSrvHeap,
                                           HEAP_ZERO_MEMORY,
                                           RequestDeviceName.MaximumLength + 5 * sizeof(WCHAR));
        if (! lpBuffer)
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
            if (! NT_SUCCESS(Status) &&
                Status == STATUS_BUFFER_TOO_SMALL)
            {
                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = Length;
                LinkTarget.Buffer = (PWSTR)
                    RtlAllocateHeap(BaseSrvHeap,
                                    HEAP_ZERO_MEMORY,
                                    Length);
                if (! LinkTarget.Buffer)
                {
                    DPRINT1("Failed to allocate memory\n");
                    Status = STATUS_NO_MEMORY;
                    _SEH2_LEAVE;
                }

                Status = NtQuerySymbolicLinkObject(LinkHandle,
                                                   &LinkTarget,
                                                   &Length);
            }

            if (! NT_SUCCESS(Status))
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
                        Matched = ! RtlCompareUnicodeString(RequestLinkTarget,
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
                else if (Matched && ! IsListEmpty(ListHead))
                {
                    /* Fetch the first history entry we come across for the device name */
                    /* This will become the current symlink target for the device name */
                    Matched = FALSE;
                    Entry = ListHead->Flink;
                    while (Entry != ListHead)
                    {
                        HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                            CONTAINING_RECORD(Entry,
                                              BASE_DOS_DEVICE_HISTORY_ENTRY,
                                              Entry);
                        Matched =
                            ! RtlCompareUnicodeString(&RequestDeviceName,
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
                    if (! Matched)
                        RequestLinkTarget = NULL;
                }
                else if (! Matched)
                {
                    /* Locate a previous symlink target as we did not get a hit earlier */
                    /* If we find one we need to remove it */
                    Entry = ListHead->Flink;
                    while (Entry != ListHead)
                    {
                        HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                            CONTAINING_RECORD(Entry,
                                              BASE_DOS_DEVICE_HISTORY_ENTRY,
                                              Entry);
                        Matched =
                            ! RtlCompareUnicodeString(&RequestDeviceName,
                                                      &HistoryEntry->Device,
                                                      FALSE);
                        if (! Matched)
                        {
                            HistoryEntry = NULL;
                            Entry = Entry->Flink;
                            continue;
                        }

                        Matched = FALSE;
                        if (dwFlags & DDD_EXACT_MATCH_ON_REMOVE)
                        {
                            if (! RtlCompareUnicodeString(RequestLinkTarget,
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
                    if (! Matched)
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
            if (! NT_SUCCESS(Status))
            {
                DPRINT1("NtMakeTemporaryObject(%wZ) failed (Status %lx)\n",
                     &DeviceName, Status);
                _SEH2_LEAVE;
            }

            Status = NtClose(LinkHandle);
            LinkHandle = NULL;
            if (! NT_SUCCESS(Status))
            {
                DPRINT1("NtClose(%wZ) failed (Status %lx)\n",
                     &DeviceName, Status);
                _SEH2_LEAVE;
            }
        }

        /* Don't create symlink if we don't have a target */
        if (! RequestLinkTarget || RequestLinkTarget->Length == 0)
            _SEH2_LEAVE;

        if (AddHistory)
        {
            HistoryEntry = (PBASE_DOS_DEVICE_HISTORY_ENTRY)
                RtlAllocateHeap(BaseSrvHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(BASE_DOS_DEVICE_HISTORY_ENTRY));
            if (! HistoryEntry)
            {
                DPRINT1("Failed to allocate memory\n");
                Status = STATUS_NO_MEMORY;
                _SEH2_LEAVE;
            }

            HistoryEntry->Target.Buffer =
                RtlAllocateHeap(BaseSrvHeap,
                                HEAP_ZERO_MEMORY,
                                LinkTarget.Length);
            if (! HistoryEntry->Target.Buffer)
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
            if (! HistoryEntry->Device.Buffer)
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
        if (! SecurityDescriptor)
        {
            DPRINT1("Failed to allocate memory\n");
            Status = STATUS_NO_MEMORY;
            _SEH2_LEAVE;
        }

        Dacl = (PACL)((ULONG_PTR)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
        Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("RtlCreateSecurityDescriptor() failed (Status %lx)\n",
                 Status);
            _SEH2_LEAVE;
        }

        Status = RtlCreateAcl(Dacl,
                              Length,
                              ACL_REVISION);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("RtlCreateAcl() failed (Status %lx)\n",
                 Status);
            _SEH2_LEAVE;
        }

        (void) RtlAddAccessAllowedAce(Dacl,
                                      ACL_REVISION,
                                      GENERIC_ALL,
                                      SystemSid);
        (void) RtlAddAccessAllowedAce(Dacl,
                                      ACL_REVISION,
                                      GENERIC_ALL,
                                      AdminSid);
        (void) RtlAddAccessAllowedAce(Dacl,
                                      ACL_REVISION,
                                      STANDARD_RIGHTS_READ,
                                      WorldSid);

        Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                              TRUE,
                                              Dacl,
                                              FALSE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("RtlSetDaclSecurityDescriptor() failed (Status %lx)\n",
                 Status);
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
            if (! NT_SUCCESS(Status))
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
        (void) RtlLeaveCriticalSection(&BaseDefineDosDeviceCritSec);
        if (DeviceName.Buffer)
            (void) RtlFreeHeap(BaseSrvHeap,
                               0,
                               DeviceName.Buffer);
        if (LinkTarget.Buffer)
            (void) RtlFreeHeap(BaseSrvHeap,
                               0,
                               LinkTarget.Buffer);
        if (SecurityDescriptor)
            (void) RtlFreeHeap(BaseSrvHeap,
                               0,
                               SecurityDescriptor);
        if (LinkHandle)
            (void) NtClose(LinkHandle);
        if (SystemSid)
            (void) RtlFreeSid(SystemSid);
        if (AdminSid)
            (void) RtlFreeSid(AdminSid);
        if (WorldSid)
            (void) RtlFreeSid(WorldSid);
        RtlFreeUnicodeString(&RequestDeviceName);
        if (HistoryEntry)
        {
            if (HistoryEntry->Target.Buffer)
                (void) RtlFreeHeap(BaseSrvHeap,
                                   0,
                                   HistoryEntry->Target.Buffer);
            if (HistoryEntry->Device.Buffer)
                (void) RtlFreeHeap(BaseSrvHeap,
                                   0,
                                   HistoryEntry->Device.Buffer);
            (void) RtlFreeHeap(BaseSrvHeap,
                               0,
                               HistoryEntry);
        }
    }
    _SEH2_END

    DPRINT("CsrDefineDosDevice Exit, Statux: 0x%x\n", Status);
    return Status;
}






/* PUBLIC API *****************************************************************/

NTSTATUS NTAPI BaseSetProcessCreateNotify(IN BASE_PROCESS_CREATE_NOTIFY_ROUTINE ProcessCreateNotifyProc)
{
    DPRINT("BASESRV: %s(%08lx) called\n", __FUNCTION__, ProcessCreateNotifyProc);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
