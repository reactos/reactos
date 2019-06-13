/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/srm.c
 * PURPOSE:         Security Reference Monitor Server
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern LUID SeSystemAuthenticationId;
extern LUID SeAnonymousAuthenticationId;

/* PRIVATE DEFINITIONS ********************************************************/

#define SEP_LOGON_SESSION_TAG 'sLeS'

typedef struct _SEP_LOGON_SESSION_REFERENCES
{
    struct _SEP_LOGON_SESSION_REFERENCES *Next;
    LUID LogonId;
    ULONG ReferenceCount;
    ULONG Flags;
    PDEVICE_MAP pDeviceMap;
    LIST_ENTRY TokenList;
} SEP_LOGON_SESSION_REFERENCES, *PSEP_LOGON_SESSION_REFERENCES;

VOID
NTAPI
SepRmCommandServerThread(
    PVOID StartContext);

static
NTSTATUS
SepRmCreateLogonSession(
    PLUID LogonLuid);


/* GLOBALS ********************************************************************/

HANDLE SeRmCommandPort;
HANDLE SeLsaInitEvent;

PVOID SepCommandPortViewBase;
PVOID SepCommandPortViewRemoteBase;
ULONG_PTR SepCommandPortViewBaseOffset;

static HANDLE SepRmCommandMessagePort;

BOOLEAN SepAdtAuditingEnabled;
ULONG SepAdtMinListLength = 0x2000;
ULONG SepAdtMaxListLength = 0x3000;

#define POLICY_AUDIT_EVENT_TYPE_COUNT 9 // (AuditCategoryAccountLogon - AuditCategorySystem + 1)
UCHAR SeAuditingState[POLICY_AUDIT_EVENT_TYPE_COUNT];

KGUARDED_MUTEX SepRmDbLock;
PSEP_LOGON_SESSION_REFERENCES SepLogonSessions;


/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
SepRegQueryHelper(
    PCWSTR KeyName,
    PCWSTR ValueName,
    ULONG ValueType,
    ULONG DataLength,
    PVOID ValueData)
{
    UNICODE_STRING ValueNameString;
    UNICODE_STRING KeyNameString;
    ULONG ResultLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle = NULL;
    struct
    {
        KEY_VALUE_PARTIAL_INFORMATION Partial;
        UCHAR Buffer[64];
    } KeyValueInformation;
    NTSTATUS Status, CloseStatus;
    PAGED_CODE();

    RtlInitUnicodeString(&KeyNameString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyNameString,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlInitUnicodeString(&ValueNameString, ValueName);
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueNameString,
                             KeyValuePartialInformation,
                             &KeyValueInformation.Partial,
                             sizeof(KeyValueInformation),
                             &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((KeyValueInformation.Partial.Type != ValueType) ||
        (KeyValueInformation.Partial.DataLength != DataLength))
    {
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Cleanup;
    }


    if (ValueType == REG_BINARY)
    {
        RtlCopyMemory(ValueData, KeyValueInformation.Partial.Data, DataLength);
    }
    else if (ValueType == REG_DWORD)
    {
        *(PULONG)ValueData = *(PULONG)KeyValueInformation.Partial.Data;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

Cleanup:
    CloseStatus = ZwClose(KeyHandle);
    ASSERT(NT_SUCCESS( CloseStatus ));

    return Status;
}


BOOLEAN
NTAPI
SeRmInitPhase0(VOID)
{
    NTSTATUS Status;

    /* Initialize the database lock */
    KeInitializeGuardedMutex(&SepRmDbLock);

    /* Create the system logon session */
    Status = SepRmCreateLogonSession(&SeSystemAuthenticationId);
    if (!NT_VERIFY(NT_SUCCESS(Status)))
    {
        return FALSE;
    }

    /* Create the anonymous logon session */
    Status = SepRmCreateLogonSession(&SeAnonymousAuthenticationId);
    if (!NT_VERIFY(NT_SUCCESS(Status)))
    {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTAPI
SeRmInitPhase1(VOID)
{
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    NTSTATUS Status;

    /* Create the SeRm command port */
    RtlInitUnicodeString(&Name, L"\\SeRmCommandPort");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);
    Status = ZwCreatePort(&SeRmCommandPort,
                          &ObjectAttributes,
                          sizeof(ULONG),
                          PORT_MAXIMUM_MESSAGE_LENGTH,
                          2 * PAGE_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security: Rm Create Command Port failed 0x%lx\n", Status);
        return FALSE;
    }

    /* Create SeLsaInitEvent */
    RtlInitUnicodeString(&Name, L"\\SeLsaInitEvent");
    InitializeObjectAttributes(&ObjectAttributes, &Name, 0, NULL, NULL);
    Status = ZwCreateEvent(&SeLsaInitEvent,
                           GENERIC_WRITE,
                           &ObjectAttributes,
                           NotificationEvent,
                           FALSE);
    if (!NT_VERIFY((NT_SUCCESS(Status))))
    {
        DPRINT1("Security: LSA init event creation failed.0x%xl\n", Status);
        return FALSE;
    }

    /* Create the SeRm server thread */
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  NULL,
                                  SepRmCommandServerThread,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security: Rm Server Thread creation failed 0x%lx\n", Status);
        return FALSE;
    }

    ObCloseHandle(ThreadHandle, KernelMode);

    return TRUE;
}

static
VOID
SepAdtInitializeBounds(VOID)
{
    struct
    {
        ULONG MaxLength;
        ULONG MinLength;
    } ListBounds;
    NTSTATUS Status;
    PAGED_CODE();

    Status = SepRegQueryHelper(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa",
                               L"Bounds",
                               REG_BINARY,
                               sizeof(ListBounds),
                               &ListBounds);
    if (!NT_SUCCESS(Status))
    {
        /* No registry values, so keep hardcoded defaults */
        return;
    }

    /* Check if the bounds are valid */
    if ((ListBounds.MaxLength < ListBounds.MinLength) ||
        (ListBounds.MinLength < 16) ||
        (ListBounds.MaxLength - ListBounds.MinLength < 16))
    {
        DPRINT1("ListBounds are invalid: %u, %u\n",
                ListBounds.MinLength, ListBounds.MaxLength);
        return;
    }

    /* Set the new bounds globally */
    SepAdtMinListLength = ListBounds.MinLength;
    SepAdtMaxListLength = ListBounds.MaxLength;
}


static
NTSTATUS
SepRmSetAuditEvent(
    PSEP_RM_API_MESSAGE Message)
{
    ULONG i;
    PAGED_CODE();

    /* First re-initialize the bounds from the registry */
    SepAdtInitializeBounds();

    /* Make sure we have the right message and clear */
    ASSERT(Message->ApiNumber == RmAuditSetCommand);
    Message->ApiNumber = 0;

    /* Store the enable flag in the global variable */
    SepAdtAuditingEnabled = Message->u.SetAuditEvent.Enabled;

    /* Loop all audit event types */
    for (i = 0; i < POLICY_AUDIT_EVENT_TYPE_COUNT; i++)
    {
        /* Save the provided flags in the global array */
        SeAuditingState[i] = (UCHAR)Message->u.SetAuditEvent.Flags[i];
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
SepRmCreateLogonSession(
    PLUID LogonLuid)
{
    PSEP_LOGON_SESSION_REFERENCES CurrentSession, NewSession;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("SepRmCreateLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    /* Allocate a new session structure */
    NewSession = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(SEP_LOGON_SESSION_REFERENCES),
                                       SEP_LOGON_SESSION_TAG);
    if (NewSession == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize it */
    NewSession->LogonId = *LogonLuid;
    NewSession->ReferenceCount = 0;
    NewSession->Flags = 0;
    NewSession->pDeviceMap = NULL;
    InitializeListHead(&NewSession->TokenList);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop all existing sessions */
    for (CurrentSession = SepLogonSessions;
         CurrentSession != NULL;
         CurrentSession = CurrentSession->Next)
    {
        /* Check if the LUID matches the new one */
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonLuid))
        {
            Status = STATUS_LOGON_SESSION_EXISTS;
            goto Leave;
        }
    }

    /* Insert the new session */
    NewSession->Next = SepLogonSessions;
    SepLogonSessions = NewSession;

    Status = STATUS_SUCCESS;

Leave:
    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(NewSession, SEP_LOGON_SESSION_TAG);
    }

    return Status;
}

static
NTSTATUS
SepRmDeleteLogonSession(
    PLUID LogonLuid)
{
    DPRINT("SepRmDeleteLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    UNIMPLEMENTED;
    NT_ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
SepRmReferenceLogonSession(
    PLUID LogonLuid)
{
    PSEP_LOGON_SESSION_REFERENCES CurrentSession;

    PAGED_CODE();

    DPRINT("SepRmReferenceLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop all existing sessions */
    for (CurrentSession = SepLogonSessions;
         CurrentSession != NULL;
         CurrentSession = CurrentSession->Next)
    {
        /* Check if the LUID matches the new one */
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonLuid))
        {
            /* Reference the session */
            ++CurrentSession->ReferenceCount;
            DPRINT("ReferenceCount: %lu\n", CurrentSession->ReferenceCount);

            /* Release the database lock */
            KeReleaseGuardedMutex(&SepRmDbLock);

            return STATUS_SUCCESS;
        }
    }

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    return STATUS_NO_SUCH_LOGON_SESSION;
}


NTSTATUS
SepCleanupLUIDDeviceMapDirectory(
    PLUID LogonLuid)
{
    BOOLEAN UseCurrentProc;
    KAPC_STATE ApcState;
    WCHAR Buffer[63];
    UNICODE_STRING DirectoryName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE DirectoryHandle, LinkHandle;
    PHANDLE LinksBuffer;
    POBJECT_DIRECTORY_INFORMATION DirectoryInfo;
    ULONG LinksCount, LinksSize, DirInfoLength, ReturnLength, Context, CurrentLinks, i;
    BOOLEAN RestartScan;

    PAGED_CODE();

    /* We need a logon LUID */
    if (LogonLuid == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Use current process */
    UseCurrentProc = ObReferenceObjectSafe(PsGetCurrentProcess());
    if (UseCurrentProc)
    {
        ObDereferenceObject(PsGetCurrentProcess());
    }
    /* Unless it's gone, then use system process */
    else
    {
        KeStackAttachProcess(&PsInitialSystemProcess->Pcb, &ApcState);
    }

    /* Initialize our directory name */
    _snwprintf(Buffer,
               sizeof(Buffer) / sizeof(WCHAR),
               L"\\Sessions\\0\\DosDevices\\%08x-%08x",
               LogonLuid->HighPart,
               LogonLuid->LowPart);
    RtlInitUnicodeString(&DirectoryName, Buffer);

    /* And open it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&DirectoryHandle,
                                   DIRECTORY_QUERY,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        if (!UseCurrentProc)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        return Status;
    }

    /* Some initialization needed for browsing all our links... */
    Context = 0;
    DirectoryInfo = NULL;
    DirInfoLength = 0;
    /* In our buffer, we'll store at max 100 HANDLE */
    LinksCount = 100;
    CurrentLinks = 0;
    /* Which gives a certain size */
    LinksSize = LinksCount * sizeof(HANDLE);

    /*
     * This label is hit if we need to store more than a hundred
     * of links. In that case, we jump here after having cleaned
     * and deleted previous buffer.
     * All handles have been already closed
     */
AllocateLinksAgain:
    LinksBuffer = ExAllocatePoolWithTag(PagedPool,
                                        LinksSize,
                                        TAG_SE_HANDLES_TAB);
    if (LinksBuffer == NULL)
    {
        /*
         * Failure path: no need to clear handles:
         * already closed and the buffer is already gone
         */
        ZwClose(DirectoryHandle);

        /*
         * On the first round, DirectoryInfo is NULL,
         * if we grow LinksBuffer, it has been allocated
         */
        if (DirectoryInfo != NULL)
        {
            ExFreePoolWithTag(DirectoryInfo, TAG_SE_DIR_BUFFER);
        }

        if (!UseCurrentProc)
        {
            KeUnstackDetachProcess(&ApcState);
        }

        return STATUS_NO_MEMORY;
    }

    /*
     * We always restart scan, but on the first loop
     * if we couldn't fit everything in our buffer,
     * then, we continue scan.
     * But we restart if link buffer was too small
     */
    for (RestartScan = TRUE; ; RestartScan = FALSE)
    {
        /*
         * Loop until our buffer is big enough to store
         * one entry
         */
        while (TRUE)
        {
            Status = ZwQueryDirectoryObject(DirectoryHandle,
                                            DirectoryInfo,
                                            DirInfoLength,
                                            TRUE,
                                            RestartScan,
                                            &Context,
                                            &ReturnLength);
            /* Only handle buffer growth in that loop */
            if (Status != STATUS_BUFFER_TOO_SMALL)
            {
                break;
            }

            /* Get output length as new length */
            DirInfoLength = ReturnLength;
            /* Delete old buffer if any */
            if (DirectoryInfo != NULL)
            {
                ExFreePoolWithTag(DirectoryInfo, 'bDeS');
            }

            /* And reallocate a bigger one */
            DirectoryInfo = ExAllocatePoolWithTag(PagedPool,
                                                  DirInfoLength,
                                                  TAG_SE_DIR_BUFFER);
            /* Fail if we cannot allocate */
            if (DirectoryInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

        /* If querying the entry failed, quit */
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* We only look for symbolic links, the rest, we ignore */
        if (wcscmp(DirectoryInfo->TypeName.Buffer, L"SymbolicLink"))
        {
            continue;
        }

        /* If our link buffer is out of space, reallocate */
        if (CurrentLinks >= LinksCount)
        {
            /* First, close the links */
            for (i = 0; i < CurrentLinks; ++i)
            {
                ZwClose(LinksBuffer[i]);
            }

            /* Allow 20 more HANDLEs */
            LinksCount += 20;
            CurrentLinks = 0;
            ExFreePoolWithTag(LinksBuffer, TAG_SE_HANDLES_TAB);
            LinksSize = LinksCount * sizeof(HANDLE);

            /* And reloop again */
            goto AllocateLinksAgain;
        }

        /* Open the found link */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DirectoryInfo->Name,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   DirectoryHandle,
                                   NULL);
        if (NT_SUCCESS(ZwOpenSymbolicLinkObject(&LinkHandle,
                                                SYMBOLIC_LINK_ALL_ACCESS,
                                                &ObjectAttributes)))
        {
            /* If we cannot make it temporary, just close the link handle */
            if (!NT_SUCCESS(ZwMakeTemporaryObject(LinkHandle)))
            {
                ZwClose(LinkHandle);
            }
            /* Otherwise, store it to defer deletion */
            else
            {
                LinksBuffer[CurrentLinks] = LinkHandle;
                ++CurrentLinks;
            }
        }
    }

    /* No more entries means we handled all links, that's not a failure */
    if (Status == STATUS_NO_MORE_ENTRIES)
    {
        Status = STATUS_SUCCESS;
    }

    /* Close all the links we stored, this will like cause their deletion */
    for (i = 0; i < CurrentLinks; ++i)
    {
        ZwClose(LinksBuffer[i]);
    }
    /* And free our links buffer */
    ExFreePoolWithTag(LinksBuffer, TAG_SE_HANDLES_TAB);

    /* Free our directory info buffer - it might be NULL if we failed realloc */
    if (DirectoryInfo != NULL)
    {
        ExFreePoolWithTag(DirectoryInfo, TAG_SE_DIR_BUFFER);
    }

    /* Close our session directory */
    ZwClose(DirectoryHandle);

    /* And detach from system */
    if (!UseCurrentProc)
    {
        KeUnstackDetachProcess(&ApcState);
    }

    return Status;
}


NTSTATUS
SepRmDereferenceLogonSession(
    PLUID LogonLuid)
{
    ULONG RefCount;
    PDEVICE_MAP DeviceMap;
    PSEP_LOGON_SESSION_REFERENCES CurrentSession;

    DPRINT("SepRmDereferenceLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop all existing sessions */
    for (CurrentSession = SepLogonSessions;
         CurrentSession != NULL;
         CurrentSession = CurrentSession->Next)
    {
        /* Check if the LUID matches the new one */
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonLuid))
        {
            /* Dereference the session */
            RefCount = --CurrentSession->ReferenceCount;
            DPRINT("ReferenceCount: %lu\n", CurrentSession->ReferenceCount);

            /* Release the database lock */
            KeReleaseGuardedMutex(&SepRmDbLock);

            /* We're done with the session */
            if (RefCount == 0)
            {
                /* Get rid of the LUID device map */
                DeviceMap = CurrentSession->pDeviceMap;
                if (DeviceMap != NULL)
                {
                    CurrentSession->pDeviceMap = NULL;
                    SepCleanupLUIDDeviceMapDirectory(LogonLuid);
                    ObfDereferenceDeviceMap(DeviceMap);
                }
            }

            return STATUS_SUCCESS;
        }
    }

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    return STATUS_NO_SUCH_LOGON_SESSION;
}


BOOLEAN
NTAPI
SepRmCommandServerThreadInit(VOID)
{
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SEP_RM_API_MESSAGE Message;
    UNICODE_STRING PortName;
    REMOTE_PORT_VIEW RemotePortView;
    PORT_VIEW PortView;
    LARGE_INTEGER SectionSize;
    HANDLE SectionHandle;
    HANDLE PortHandle;
    NTSTATUS Status;
    BOOLEAN Result;

    SectionHandle = NULL;
    PortHandle = NULL;

    /* Assume success */
    Result = TRUE;

    /* Wait until LSASS is ready */
    Status = ZwWaitForSingleObject(SeLsaInitEvent, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Waiting for LSA Init Event failed 0x%lx\n", Status);
        goto Cleanup;
    }

    /* We don't need this event anymore */
    ObCloseHandle(SeLsaInitEvent, KernelMode);

    /* Initialize the connection message */
    Message.Header.u1.s1.TotalLength = sizeof(Message);
    Message.Header.u1.s1.DataLength = 0;

    /* Only LSASS can connect, so handle the connection right now */
    Status = ZwListenPort(SeRmCommandPort, &Message.Header);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Listen to Command Port failed 0x%lx\n", Status);
        goto Cleanup;
    }

    /* Set the Port View structure length */
    RemotePortView.Length = sizeof(RemotePortView);

    /* Accept the connection */
    Status = ZwAcceptConnectPort(&SepRmCommandMessagePort,
                                 NULL,
                                 &Message.Header,
                                 TRUE,
                                 NULL,
                                 &RemotePortView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Accept Connect to Command Port failed 0x%lx\n", Status);
        goto Cleanup;
    }

    /* Complete the connection */
    Status = ZwCompleteConnectPort(SepRmCommandMessagePort);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Complete Connect to Command Port failed 0x%lx\n", Status);
        goto Cleanup;
    }

    /* Create a section for messages */
    SectionSize.QuadPart = PAGE_SIZE;
    Status = ZwCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Create Memory Section for LSA port failed: %X\n", Status);
        goto Cleanup;
    }

    /* Setup the PORT_VIEW structure */
    PortView.Length = sizeof(PortView);
    PortView.SectionHandle = SectionHandle;
    PortView.SectionOffset = 0;
    PortView.ViewSize = SectionSize.LowPart;
    PortView.ViewBase = NULL;
    PortView.ViewRemoteBase = NULL;

    /* Setup security QOS */
    SecurityQos.Length = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel = SecurityImpersonation;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Connect to LSASS */
    RtlInitUnicodeString(&PortName, L"\\SeLsaCommandPort");
    Status = ZwConnectPort(&PortHandle,
                           &PortName,
                           &SecurityQos,
                           &PortView,
                           NULL,
                           0,
                           0,
                           0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Security Rm Init: Connect to LSA Port failed 0x%lx\n", Status);
        goto Cleanup;
    }

    /* Remember section base and view offset */
    SepCommandPortViewBase = PortView.ViewBase;
    SepCommandPortViewRemoteBase = PortView.ViewRemoteBase;
    SepCommandPortViewBaseOffset = (ULONG_PTR)SepCommandPortViewRemoteBase -
                                   (ULONG_PTR)SepCommandPortViewBase;

    DPRINT("SepRmCommandServerThreadInit: done\n");

Cleanup:
    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        if (PortHandle != NULL)
        {
            ObCloseHandle(PortHandle, KernelMode);
        }

        Result = FALSE;
    }

    /* Did we create a section? */
    if (SectionHandle != NULL)
    {
        ObCloseHandle(SectionHandle, KernelMode);
    }

    return Result;
}

VOID
NTAPI
SepRmCommandServerThread(
    PVOID StartContext)
{
    SEP_RM_API_MESSAGE Message;
    PPORT_MESSAGE ReplyMessage;
    HANDLE DummyPortHandle;
    NTSTATUS Status;

    /* Initialize the server thread */
    if (!SepRmCommandServerThreadInit())
    {
        DPRINT1("Security: Terminating Rm Command Server Thread\n");
        return;
    }

    /* No reply yet */
    ReplyMessage = NULL;

    /* Start looping */
    while (TRUE)
    {
        /* Wait for a message */
        Status = ZwReplyWaitReceivePort(SepRmCommandMessagePort,
                                        NULL,
                                        ReplyMessage,
                                        &Message.Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get message: 0x%lx", Status);
            ReplyMessage = NULL;
            continue;
        }

        /* Check if this is a connection request */
        if (Message.Header.u2.s2.Type == LPC_CONNECTION_REQUEST)
        {
            /* Reject connection request */
            ZwAcceptConnectPort(&DummyPortHandle,
                                NULL,
                                &Message.Header,
                                FALSE,
                                NULL,
                                NULL);

            /* Start over */
            ReplyMessage = NULL;
            continue;
        }

        /* Check if the port died */
        if ((Message.Header.u2.s2.Type == LPC_PORT_CLOSED) ||
            (Message.Header.u2.s2.Type == LPC_CLIENT_DIED))
        {
            /* LSASS is dead, so let's quit as well */
            break;
        }

        /* Check if this is an actual request */
        if (Message.Header.u2.s2.Type != LPC_REQUEST)
        {
            DPRINT1("SepRmCommandServerThread: unexpected message type: 0x%lx\n",
                    Message.Header.u2.s2.Type);

            /* Restart without replying */
            ReplyMessage = NULL;
            continue;
        }

        ReplyMessage = &Message.Header;

        switch (Message.ApiNumber)
        {
            case RmAuditSetCommand:
                Status = SepRmSetAuditEvent(&Message);
                break;

            case RmCreateLogonSession:
                Status = SepRmCreateLogonSession(&Message.u.LogonLuid);
                break;

            case RmDeleteLogonSession:
                Status = SepRmDeleteLogonSession(&Message.u.LogonLuid);
                break;

            default:
                DPRINT1("SepRmDispatchRequest: invalid API number: 0x%lx\n",
                        Message.ApiNumber);
                ReplyMessage = NULL;
        }

        Message.u.ResultStatus = Status;
    }

    /* Close the port handles */
    ObCloseHandle(SepRmCommandMessagePort, KernelMode);
    ObCloseHandle(SeRmCommandPort, KernelMode);
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeGetLogonIdDeviceMap(
    IN PLUID LogonId,
    OUT PDEVICE_MAP * DeviceMap
    )
{
    NTSTATUS Status;
    WCHAR Buffer[63];
    PDEVICE_MAP LocalMap;
    HANDLE DirectoryHandle, LinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSEP_LOGON_SESSION_REFERENCES CurrentSession;
    UNICODE_STRING DirectoryName, LinkName, TargetName;

    PAGED_CODE();

    if  (LogonId == NULL ||
         DeviceMap == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop all existing sessions */
    for (CurrentSession = SepLogonSessions;
         CurrentSession != NULL;
         CurrentSession = CurrentSession->Next)
    {
        /* Check if the LUID matches the provided one */
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonId))
        {
            break;
        }
    }

    /* No session found, fail */
    if (CurrentSession == NULL)
    {
        /* Release the database lock */
        KeReleaseGuardedMutex(&SepRmDbLock);

        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    /* The found session has a device map, return it! */
    if (CurrentSession->pDeviceMap != NULL)
    {
        *DeviceMap = CurrentSession->pDeviceMap;

        /* Release the database lock */
        KeReleaseGuardedMutex(&SepRmDbLock);

        return STATUS_SUCCESS;
    }

    /* At that point, we'll setup a new device map for the session */
    LocalMap = NULL;

    /* Reference the session so that it doesn't go away */
    CurrentSession->ReferenceCount += 1;

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    /* Create our object directory given the LUID */
    _snwprintf(Buffer,
               sizeof(Buffer) / sizeof(WCHAR),
               L"\\Sessions\\0\\DosDevices\\%08x-%08x",
               LogonId->HighPart,
               LogonId->LowPart);
    RtlInitUnicodeString(&DirectoryName, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryName,
                               OBJ_KERNEL_HANDLE | OBJ_OPENIF | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwCreateDirectoryObject(&DirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Create the associated device map */
        Status = ObSetDirectoryDeviceMap(&LocalMap, DirectoryHandle);
        if (NT_SUCCESS(Status))
        {
            /* Make Global point to \Global?? in the directory */
            RtlInitUnicodeString(&LinkName, L"Global");
            RtlInitUnicodeString(&TargetName, L"\\Global??");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &LinkName,
                                       OBJ_KERNEL_HANDLE | OBJ_OPENIF | OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                       DirectoryHandle,
                                       NULL);
            Status = ZwCreateSymbolicLinkObject(&LinkHandle,
                                                SYMBOLIC_LINK_ALL_ACCESS,
                                                &ObjectAttributes,
                                                &TargetName);
            if (!NT_SUCCESS(Status))
            {
                ObfDereferenceDeviceMap(LocalMap);
            }
            else
            {
                ZwClose(LinkHandle);
            }
        }

        ZwClose(DirectoryHandle);
    }

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* If we succeed... */
    if (NT_SUCCESS(Status))
    {
        /* The session now has a device map? We raced with someone else */
        if (CurrentSession->pDeviceMap != NULL)
        {
            /* Give up on our new device map */
            ObfDereferenceDeviceMap(LocalMap);
        }
        /* Otherwise use our newly allocated device map */
        else
        {
            CurrentSession->pDeviceMap = LocalMap;
        }

        /* Return the device map */
        *DeviceMap = CurrentSession->pDeviceMap;
    }
    /* Zero output */
    else
    {
        *DeviceMap = NULL;
    }

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    /* We're done with the session */
    SepRmDereferenceLogonSession(&CurrentSession->LogonId);

    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(
    IN PLUID LogonId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
