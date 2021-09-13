/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security Reference Monitor Server
 * COPYRIGHT:       Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 *                  Copyright Pierre Schweitzer <pierre@reactos.org>
 *                  Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE DEFINITIONS ********************************************************/

typedef struct _SEP_LOGON_SESSION_TERMINATED_NOTIFICATION
{
    struct _SEP_LOGON_SESSION_TERMINATED_NOTIFICATION *Next;
    PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine;
} SEP_LOGON_SESSION_TERMINATED_NOTIFICATION, *PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION;

VOID
NTAPI
SepRmCommandServerThread(
    _In_ PVOID StartContext);

static
NTSTATUS
SepCleanupLUIDDeviceMapDirectory(
    _In_ PLUID LogonLuid);

static
NTSTATUS
SepRmCreateLogonSession(
    _In_ PLUID LogonLuid);


/* GLOBALS ********************************************************************/

extern LUID SeSystemAuthenticationId;
extern LUID SeAnonymousAuthenticationId;

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
PSEP_LOGON_SESSION_REFERENCES SepLogonSessions = NULL;
PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION SepLogonNotifications = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * A private registry helper that returns the desired value
 * data based on the specifics requested by the caller.
 *
 * @param[in] KeyName
 * Name of the key.
 *
 * @param[in] ValueName
 * Name of the registry value.
 *
 * @param[in] ValueType
 * The type of the registry value.
 *
 * @param[in] DataLength
 * The data length, in bytes, representing the size of the registry value.
 *
 * @param[out] ValueData
 * The requested value data provided by the function.
 *
 * @return
 * Returns STATUS_SUCCESS if the operations have completed successfully,
 * otherwise a failure NTSTATUS code is returned.
 */
NTSTATUS
NTAPI
SepRegQueryHelper(
    _In_ PCWSTR KeyName,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ ULONG DataLength,
    _Out_ PVOID ValueData)
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

/**
 * @brief
 * Manages the phase 0 initialization of the security reference
 * monitoring module of the kernel.
 *
 * @return
 * Returns TRUE when phase 0 initialization has completed without
 * problems, FALSE otherwise.
 */
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

/**
 * @brief
 * Manages the phase 1 initialization of the security reference
 * monitoring module of the kernel.
 *
 * @return
 * Returns TRUE when phase 1 initialization has completed without
 * problems, FALSE otherwise.
 */
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

/**
 * @brief
 * Initializes the local security authority audit bounds.
 *
 * @return
 * Nothing.
 */
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

/**
 * @brief
 * Sets an audit event for future security auditing monitoring.
 *
 * @param[in,out] Message
 * The reference monitoring API message. It is used to determine
 * if the right API message number is provided, RmAuditSetCommand
 * in this case.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
SepRmSetAuditEvent(
    _Inout_ PSEP_RM_API_MESSAGE Message)
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

/**
 * @brief
 * Inserts a logon session into an access token specified by the
 * caller.
 *
 * @param[in,out] Token
 * An access token where the logon session is about to be inserted
 * in.
 *
 * @return
 * STATUS_SUCCESS is returned if the logon session has been inserted into
 * the token successfully. STATUS_NO_SUCH_LOGON_SESSION is returned when no logon
 * session has been found with the matching ID of the token and as such
 * we've failed to add the logon session to the token. STATUS_INSUFFICIENT_RESOURCES
 * is returned if memory pool allocation for the new session has failed.
 */
NTSTATUS
NTAPI
SepRmInsertLogonSessionIntoToken(
    _Inout_ PTOKEN Token)
{
    PSEP_LOGON_SESSION_REFERENCES LogonSession;
    PAGED_CODE();

    /* Ensure that our token is not some plain garbage */
    ASSERT(Token);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    for (LogonSession = SepLogonSessions;
         LogonSession != NULL;
         LogonSession = LogonSession->Next)
    {
        /*
         * The insertion of a logon session into the token has to be done
         * only IF the authentication ID of the token matches with the ID
         * of the logon itself.
         */
        if (RtlEqualLuid(&LogonSession->LogonId, &Token->AuthenticationId))
        {
            break;
        }
    }

    /* If we reach this then we cannot proceed further */
    if (LogonSession == NULL)
    {
        DPRINT1("SepRmInsertLogonSessionIntoToken(): Couldn't insert the logon session into the specific access token!\n");
        KeReleaseGuardedMutex(&SepRmDbLock);
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    /*
     * Allocate the session that we are going
     * to insert it to the token.
     */
    Token->LogonSession = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(SEP_LOGON_SESSION_REFERENCES),
                                                TAG_LOGON_SESSION);
    if (Token->LogonSession == NULL)
    {
        DPRINT1("SepRmInsertLogonSessionIntoToken(): Couldn't allocate new logon session into the memory pool!\n");
        KeReleaseGuardedMutex(&SepRmDbLock);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Begin copying the logon session references data from the
     * session whose ID matches with the token authentication ID to
     * the new session we've allocated blocks of pool memory for it.
     */
    Token->LogonSession->Next = LogonSession->Next;
    Token->LogonSession->LogonId = LogonSession->LogonId;
    Token->LogonSession->ReferenceCount = LogonSession->ReferenceCount;
    Token->LogonSession->Flags = LogonSession->Flags;
    Token->LogonSession->pDeviceMap = LogonSession->pDeviceMap;
    InsertHeadList(&LogonSession->TokenList, &Token->LogonSession->TokenList);

    /* Release the database lock and we're done */
    KeReleaseGuardedMutex(&SepRmDbLock);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Removes a logon session from an access token.
 *
 * @param[in,out] Token
 * An access token whose logon session is to be removed from it.
 *
 * @return
 * STATUS_SUCCESS is returned if the logon session has been removed from
 * the token successfully. STATUS_NO_SUCH_LOGON_SESSION is returned when no logon
 * session has been found with the matching ID of the token and as such
 * we've failed to remove the logon session from the token.
 */
NTSTATUS
NTAPI
SepRmRemoveLogonSessionFromToken(
    _Inout_ PTOKEN Token)
{
    PSEP_LOGON_SESSION_REFERENCES LogonSession;
    PAGED_CODE();

    /* Ensure that our token is not some plain garbage */
    ASSERT(Token);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    for (LogonSession = SepLogonSessions;
         LogonSession != NULL;
         LogonSession = LogonSession->Next)
    {
        /*
         * Remove the logon session only when the IDs of the token and the
         * logon match.
         */
        if (RtlEqualLuid(&LogonSession->LogonId, &Token->AuthenticationId))
        {
            break;
        }
    }

    /* They don't match */
    if (LogonSession == NULL)
    {
        DPRINT1("SepRmRemoveLogonSessionFromToken(): Couldn't remove the logon session from the access token!\n");
        KeReleaseGuardedMutex(&SepRmDbLock);
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    /* Now it's time to delete the logon session from the token */
    RemoveEntryList(&Token->LogonSession->TokenList);
    ExFreePoolWithTag(Token->LogonSession, TAG_LOGON_SESSION);

    /* Release the database lock and we're done */
    KeReleaseGuardedMutex(&SepRmDbLock);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Creates a logon session. The security reference monitoring (SRM)
 * module of Executive uses this as an internal kernel data for
 * respective logon sessions management within the kernel,
 * as in form of a SEP_LOGON_SESSION_REFERENCES data structure.
 *
 * @param[in] LogonLuid
 * A logon ID represented as a LUID. This LUID is used to create
 * our logon session and add it to the sessions database.
 *
 * @return
 * Returns STATUS_SUCCESS if the logon has been created successfully.
 * STATUS_LOGON_SESSION_EXISTS is returned if a logon session with
 * the pointed logon ID in the call already exists.
 * STATUS_INSUFFICIENT_RESOURCES is returned if logon session allocation
 * has failed because of lack of memory pool resources.
 */
static
NTSTATUS
SepRmCreateLogonSession(
    _In_ PLUID LogonLuid)
{
    PSEP_LOGON_SESSION_REFERENCES CurrentSession, NewSession;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("SepRmCreateLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    /* Allocate a new session structure */
    NewSession = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(SEP_LOGON_SESSION_REFERENCES),
                                       TAG_LOGON_SESSION);
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
        ExFreePoolWithTag(NewSession, TAG_LOGON_SESSION);
    }

    return Status;
}

/**
 * @brief
 * Deletes a logon session from the logon sessions database.
 *
 * @param[in] LogonLuid
 * A logon ID represented as a LUID. This LUID is used to point
 * the exact logon session saved within the database.
 *
 * @return
 * STATUS_SUCCESS is returned if the logon session has been deleted successfully.
 * STATUS_NO_SUCH_LOGON_SESSION is returned if the logon session with the submitted
 * LUID doesn't exist. STATUS_BAD_LOGON_SESSION_STATE is returned if the logon session
 * is still in use and we're not allowed to delete it, or if a system or anonymous session
 * is submitted and we're not allowed to delete them as they're internal parts of the system.
 * Otherwise a failure NTSTATUS code is returned.
 */
static
NTSTATUS
SepRmDeleteLogonSession(
    _In_ PLUID LogonLuid)
{
    PSEP_LOGON_SESSION_REFERENCES SessionToDelete;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("SepRmDeleteLogonSession(%08lx:%08lx)\n",
           LogonLuid->HighPart, LogonLuid->LowPart);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop over the existing logon sessions */
    for (SessionToDelete = SepLogonSessions;
         SessionToDelete != NULL;
         SessionToDelete = SessionToDelete->Next)
    {
        /*
         * Does the actual logon session exist in the
         * saved logon sessions database with the LUID
         * provided?
         */
        if (RtlEqualLuid(&SessionToDelete->LogonId, LogonLuid))
        {
            /* Did the caller supply one of these internal sessions? */
            if (RtlEqualLuid(&SessionToDelete->LogonId, &SeSystemAuthenticationId) ||
                RtlEqualLuid(&SessionToDelete->LogonId, &SeAnonymousAuthenticationId))
            {
                /* These logons are critical stuff, we can't delete them */
                DPRINT1("SepRmDeleteLogonSession(): We're not allowed to delete anonymous/system sessions!\n");
                Status = STATUS_BAD_LOGON_SESSION_STATE;
                goto Leave;
            }
            else
            {
                /* We found the logon as exactly as we wanted, break the loop */
                break;
            }
        }
    }

    /*
     * If we reach this then that means we've exhausted all the logon
     * sessions and couldn't find one with the desired LUID.
     */
    if (SessionToDelete == NULL)
    {
        DPRINT1("SepRmDeleteLogonSession(): The logon session with this LUID doesn't exist!\n");
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Leave;
    }

    /* Is somebody still using this logon session? */
    if (SessionToDelete->ReferenceCount != 0)
    {
        /* The logon session is still in use, we cannot delete it... */
        DPRINT1("SepRmDeleteLogonSession(): The logon session is still in use!\n");
        Status = STATUS_BAD_LOGON_SESSION_STATE;
        goto Leave;
    }

    /* If we have a LUID device map, clean it */
    if (SessionToDelete->pDeviceMap != NULL)
    {
        Status = SepCleanupLUIDDeviceMapDirectory(LogonLuid);
        if (!NT_SUCCESS(Status))
        {
            /*
             * We had one job on cleaning the device map directory
             * of the logon session but we failed, quit...
             */
            DPRINT1("SepRmDeleteLogonSession(): Failed to clean the LUID device map directory of the logon (Status: 0x%lx)\n", Status);
            goto Leave;
        }

        /* And dereference the device map of the logon */
        ObfDereferenceDeviceMap(SessionToDelete->pDeviceMap);
    }

    /* If we're here then we've deleted the logon session successfully */
    DPRINT("SepRmDeleteLogonSession(): Logon session deleted with success!\n");
    Status = STATUS_SUCCESS;
    ExFreePoolWithTag(SessionToDelete, TAG_LOGON_SESSION);

Leave:
    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);
    return Status;
}

/**
 * @brief
 * References a logon session.
 *
 * @param[in] LogonLuid
 * A valid LUID that points to the logon session in the database that
 * we're going to reference it.
 *
 * @return
 * Returns STATUS_SUCCESS if the logon has been referenced.
 * STATUS_NO_SUCH_LOGON_SESSION is returned if the session couldn't be
 * found otherwise.
 */
NTSTATUS
SepRmReferenceLogonSession(
    _In_ PLUID LogonLuid)
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

/**
 * @brief
 * Cleans the DOS device map directory of a logon
 * session.
 *
 * @param[in] LogonLuid
 * A logon session ID where its DOS device map directory
 * is to be cleaned.
 *
 * @return
 * Returns STATUS_SUCCESS if the device map directory has been
 * successfully cleaned from the logon session. STATUS_INVALID_PARAMETER
 * is returned if the caller hasn't submitted any logon ID. STATUS_NO_MEMORY
 * is returned if buffer allocation for links has failed. A failure
 * NTSTATUS code is returned otherwise.
 */
static
NTSTATUS
SepCleanupLUIDDeviceMapDirectory(
    _In_ PLUID LogonLuid)
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

/**
 * @brief
 * De-references a logon session. If the session has a reference
 * count of 0 by the time the function has de-referenced the logon,
 * that means the session is no longer used and can be safely deleted
 * from the logon sessions database.
 *
 * @param[in] LogonLuid
 * A logon session ID to de-reference.
 *
 * @return
 * Returns STATUS_SUCCESS if the logon session has been de-referenced
 * without issues. STATUS_NO_SUCH_LOGON_SESSION is returned if no
 * such logon exists otherwise.
 */
NTSTATUS
SepRmDereferenceLogonSession(
    _In_ PLUID LogonLuid)
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

                /* FIXME: Alert LSA and filesystems that a logon is about to be deleted */
            }

            return STATUS_SUCCESS;
        }
    }

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    return STATUS_NO_SUCH_LOGON_SESSION;
}

/**
 * @brief
 * Main SRM server thread initialization function. It deals
 * with security manager and LSASS port connection, thus
 * thereby allowing communication between the kernel side
 * (the SRM) and user mode side (the LSASS) of the security
 * world of the operating system.
 *
 * @return
 * Returns TRUE if command server connection between SRM
 * and LSASS has succeeded, FALSE otherwise.
 */
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

/**
 * @brief
 * Manages the SRM server API commands, that is, receiving such API
 * command messages from the user mode side of the security standpoint,
 * the LSASS.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepRmCommandServerThread(
    _In_ PVOID StartContext)
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

/**
 * @brief
 * Retrieves the DOS device map from a logon session.
 *
 * @param[in] LogonId
 * A valid logon session ID.
 *
 * @param[out] DeviceMap
 * The returned device map buffer from the logon session.
 *
 * @return
 * Returns STATUS_SUCCESS if the device map could be gathered
 * from the logon session. STATUS_INVALID_PARAMETER is returned if
 * one of the parameters aren't initialized (that is, the caller has
 * submitted a NULL pointer variable). STATUS_NO_SUCH_LOGON_SESSION is
 * returned if no such session could be found. A failure NTSTATUS code
 * is returned otherwise.
 */
NTSTATUS
NTAPI
SeGetLogonIdDeviceMap(
    _In_ PLUID LogonId,
    _Out_ PDEVICE_MAP *DeviceMap)
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

/**
 * @brief
 * Marks a logon session for future termination, given its logon ID. This triggers
 * a callout (the registered callback) when the logon is no longer used by anyone,
 * that is, no token is still referencing the speciffied logon session.
 *
 * @param[in] LogonId
 * The ID of the logon session.
 *
 * @return
 * STATUS_SUCCESS if the logon session is marked for termination notification successfully,
 * STATUS_NOT_FOUND if the logon session couldn't be found otherwise.
 */
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(
    _In_ PLUID LogonId)
{
    PSEP_LOGON_SESSION_REFERENCES SessionToMark;
    PAGED_CODE();

    DPRINT("SeMarkLogonSessionForTerminationNotification(%08lx:%08lx)\n",
           LogonId->HighPart, LogonId->LowPart);

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop over the existing logon sessions */
    for (SessionToMark = SepLogonSessions;
         SessionToMark != NULL;
         SessionToMark = SessionToMark->Next)
    {
        /* Does the logon with the given ID exist? */
        if (RtlEqualLuid(&SessionToMark->LogonId, LogonId))
        {
            /* We found it */
            break;
        }
    }

    /*
     * We've exhausted all the remaining logon sessions and
     * couldn't find one with the provided ID.
     */
    if (SessionToMark == NULL)
    {
        DPRINT1("SeMarkLogonSessionForTerminationNotification(): Logon session couldn't be found!\n");
        KeReleaseGuardedMutex(&SepRmDbLock);
        return STATUS_NOT_FOUND;
    }

    /* Mark the logon session for termination */
    SessionToMark->Flags |= SEP_LOGON_SESSION_TERMINATION_NOTIFY;
    DPRINT("SeMarkLogonSessionForTerminationNotification(): Logon session marked for termination with success!\n");

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Registers a callback that will be called once a logon session
 * terminates.
 *
 * @param[in] CallbackRoutine
 * Callback routine address.
 *
 * @return
 * Returns STATUS_SUCCESS if the callback routine was registered
 * successfully. STATUS_INVALID_PARAMETER is returned if the caller
 * did not provide a callback routine. STATUS_INSUFFICIENT_RESOURCES
 * is returned if the callback notification data couldn't be allocated
 * because of lack of memory pool resources.
 */
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(
    _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION Notification;
    PAGED_CODE();

    /* Fail, if we don not have a callback routine */
    if (CallbackRoutine == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Allocate a new notification item */
    Notification = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(SEP_LOGON_SESSION_TERMINATED_NOTIFICATION),
                                         TAG_LOGON_NOTIFICATION);
    if (Notification == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Set the callback routine */
    Notification->CallbackRoutine = CallbackRoutine;

    /* Insert the new notification item into the list */
    Notification->Next = SepLogonNotifications;
    SepLogonNotifications = Notification;

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Un-registers a callback routine, previously registered by
 * SeRegisterLogonSessionTerminatedRoutine function.
 *
 * @param[in] CallbackRoutine
 * Callback routine address to un-register.
 *
 * @return
 * Returns STATUS_SUCCESS if the callback routine was un-registered
 * successfully. STATUS_INVALID_PARAMETER is returned if the caller
 * did not provide a callback routine. STATUS_NOT_FOUND is returned
 * if the callback notification item couldn't be found.
 */
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(
    _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine)
{
    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION Current, Previous = NULL;
    NTSTATUS Status;
    PAGED_CODE();

    /* Fail, if we don not have a callback routine */
    if (CallbackRoutine == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Acquire the database lock */
    KeAcquireGuardedMutex(&SepRmDbLock);

    /* Loop all registered notification items */
    for (Current = SepLogonNotifications;
         Current != NULL;
         Current = Current->Next)
    {
        /* Check if the callback routine matches the provided one */
        if (Current->CallbackRoutine == CallbackRoutine)
            break;

        Previous = Current;
    }

    if (Current == NULL)
    {
        Status = STATUS_NOT_FOUND;
    }
    else
    {
        /* Remove the current notification item from the list */
        if (Previous == NULL)
            SepLogonNotifications = Current->Next;
        else
            Previous->Next = Current->Next;

        /* Free the current notification item */
        ExFreePoolWithTag(Current,
                          TAG_LOGON_NOTIFICATION);

        Status = STATUS_SUCCESS;
    }

    /* Release the database lock */
    KeReleaseGuardedMutex(&SepRmDbLock);

    return Status;
}

/* EOF */
