/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/session.c
 * PURPOSE:     Logon session management routines
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "lsasrv.h"

typedef struct _LSAP_LOGON_SESSION
{
    LIST_ENTRY Entry;
    LUID LogonId;
    ULONG LogonType;
    ULONG Session;
    LARGE_INTEGER LogonTime;
    PSID Sid;
    UNICODE_STRING UserName;
    UNICODE_STRING LogonDomain;
    UNICODE_STRING AuthenticationPackage;
    UNICODE_STRING LogonServer;
    UNICODE_STRING DnsDomainName;
    UNICODE_STRING Upn;
} LSAP_LOGON_SESSION, *PLSAP_LOGON_SESSION;


/* GLOBALS *****************************************************************/

LIST_ENTRY SessionListHead;
ULONG SessionCount;

/* FUNCTIONS ***************************************************************/

VOID
LsapInitLogonSessions(VOID)
{
    InitializeListHead(&SessionListHead);
    SessionCount = 0;
}


static
PLSAP_LOGON_SESSION
LsapGetLogonSession(IN PLUID LogonId)
{
    PLIST_ENTRY SessionEntry;
    PLSAP_LOGON_SESSION CurrentSession;

    SessionEntry = SessionListHead.Flink;
    while (SessionEntry != &SessionListHead)
    {
        CurrentSession = CONTAINING_RECORD(SessionEntry,
                                           LSAP_LOGON_SESSION,
                                           Entry);
        if (RtlEqualLuid(&CurrentSession->LogonId, LogonId))
            return CurrentSession;

        SessionEntry = SessionEntry->Flink;
    }

    return NULL;
}


NTSTATUS
LsapSetLogonSessionData(IN PLUID LogonId)
{
    PLSAP_LOGON_SESSION Session;

    TRACE("LsapSetLogonSessionData(%p)\n", LogonId);

    Session = LsapGetLogonSession(LogonId);
    if (Session == NULL)
        return STATUS_NO_SUCH_LOGON_SESSION;


    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
LsapCreateLogonSession(IN PLUID LogonId)
{
    PLSAP_LOGON_SESSION Session;
    NTSTATUS Status;

    TRACE("LsapCreateLogonSession(%p)\n", LogonId);

    /* Fail, if a session already exists */
    if (LsapGetLogonSession(LogonId) != NULL)
        return STATUS_LOGON_SESSION_COLLISION;

    /* Allocate a new session entry */
    Session = RtlAllocateHeap(RtlGetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              sizeof(LSAP_LOGON_SESSION));
    if (Session == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the session entry */
    RtlCopyLuid(&Session->LogonId, LogonId);

    TRACE("LsapCreateLogonSession(<0x%lx,0x%lx>)\n",
          LogonId->HighPart, LogonId->LowPart);

    /* Tell ntoskrnl to create a new logon session */
    Status = LsapRmCreateLogonSession(LogonId);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session);
        return Status;
    }

    /* Insert the new session into the session list */
    InsertHeadList(&SessionListHead, &Session->Entry);
    SessionCount++;

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
LsapDeleteLogonSession(IN PLUID LogonId)
{
    PLSAP_LOGON_SESSION Session;
    NTSTATUS Status;

    TRACE("LsapDeleteLogonSession(%p)\n", LogonId);

    /* Fail, if the session does not exist */
    Session = LsapGetLogonSession(LogonId);
    if (Session == NULL)
        return STATUS_NO_SUCH_LOGON_SESSION;

    TRACE("LsapDeleteLogonSession(<0x%lx,0x%lx>)\n",
          LogonId->HighPart, LogonId->LowPart);

    /* Tell ntoskrnl to delete the logon session */
    Status = LsapRmDeleteLogonSession(LogonId);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Remove the session entry from the list */
    RemoveEntryList(&Session->Entry);
    SessionCount--;

    /* Free the session data */
    if (Session->Sid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->Sid);

    if (Session->UserName.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->UserName.Buffer);

    if (Session->LogonDomain.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->LogonDomain.Buffer);

    if (Session->AuthenticationPackage.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->AuthenticationPackage.Buffer);

    if (Session->LogonServer.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->LogonServer.Buffer);

    if (Session->DnsDomainName.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->DnsDomainName.Buffer);

    if (Session->Upn.Buffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Session->Upn.Buffer);

    /* Free the session entry */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Session);

    return STATUS_SUCCESS;
}


NTSTATUS
LsapEnumLogonSessions(IN OUT PLSA_API_MSG RequestMsg)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    PLIST_ENTRY SessionEntry;
    PLSAP_LOGON_SESSION CurrentSession;
    PLUID SessionList;
    ULONG i, Length, MemSize;
    PVOID ClientBaseAddress = NULL;
    NTSTATUS Status;

    TRACE("LsapEnumLogonSessions(%p)\n", RequestMsg);

    Length = SessionCount * sizeof(LUID);
    SessionList = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  Length);
    if (SessionList == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    i = 0;
    SessionEntry = SessionListHead.Flink;
    while (SessionEntry != &SessionListHead)
    {
        CurrentSession = CONTAINING_RECORD(SessionEntry,
                                           LSAP_LOGON_SESSION,
                                           Entry);

        RtlCopyLuid(&SessionList[i],
                    &CurrentSession->LogonId);

        SessionEntry = SessionEntry->Flink;
        i++;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                           &ObjectAttributes,
                           &RequestMsg->h.ClientId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtOpenProcess() failed (Status %lx)\n", Status);
        goto done;
    }

    TRACE("Length: %lu\n", Length);

    MemSize = Length;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &ClientBaseAddress,
                                     0,
                                     &MemSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
        goto done;
    }

    TRACE("MemSize: %lu\n", MemSize);
    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    Status = NtWriteVirtualMemory(ProcessHandle,
                                  ClientBaseAddress,
                                  SessionList,
                                  Length,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtWriteVirtualMemory() failed (Status %lx)\n", Status);
        goto done;
    }

    RequestMsg->EnumLogonSessions.Reply.LogonSessionCount = SessionCount;
    RequestMsg->EnumLogonSessions.Reply.LogonSessionBuffer = ClientBaseAddress;

done:
    if (ProcessHandle != NULL)
        NtClose(ProcessHandle);

    if (SessionList != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, SessionList);

    return Status;
}


NTSTATUS
LsapGetLogonSessionData(IN OUT PLSA_API_MSG RequestMsg)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    PLSAP_LOGON_SESSION Session;
    PSECURITY_LOGON_SESSION_DATA LocalSessionData;
    PVOID ClientBaseAddress = NULL;
    ULONG Length, MemSize;
    LPWSTR Ptr;
    NTSTATUS Status;

    TRACE("LsapGetLogonSessionData(%p)\n", RequestMsg);

    TRACE("LogonId: %lx\n", RequestMsg->GetLogonSessionData.Request.LogonId.LowPart);
    Session = LsapGetLogonSession(&RequestMsg->GetLogonSessionData.Request.LogonId);
    if (Session == NULL)
        return STATUS_NO_SUCH_LOGON_SESSION;

    Length = sizeof(SECURITY_LOGON_SESSION_DATA);
/*
             Session->UserName.MaximumLength +
             Session->LogonDomain.MaximumLength +
             Session->AuthenticationPackage.MaximumLength +
             Session->LogonServer.MaximumLength +
             Session->DnsDomainName.MaximumLength +
             Session->Upn.MaximumLength;

    if (Session->Sid != NULL)
        RtlLengthSid(Session->Sid);
*/

    TRACE("Length: %lu\n", Length);

    LocalSessionData = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       Length);
    if (LocalSessionData == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Ptr = (LPWSTR)((ULONG_PTR)LocalSessionData + sizeof(SECURITY_LOGON_SESSION_DATA));
    TRACE("LocalSessionData: %p  Ptr: %p\n", LocalSessionData, Ptr);

    LocalSessionData->Size = sizeof(SECURITY_LOGON_SESSION_DATA);

    RtlCopyLuid(&LocalSessionData->LogonId,
                &RequestMsg->GetLogonSessionData.Request.LogonId);

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                           &ObjectAttributes,
                           &RequestMsg->h.ClientId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtOpenProcess() failed (Status %lx)\n", Status);
        goto done;
    }

    MemSize = Length;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &ClientBaseAddress,
                                     0,
                                     &MemSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
        goto done;
    }

    TRACE("MemSize: %lu\n", MemSize);
    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    Status = NtWriteVirtualMemory(ProcessHandle,
                                  ClientBaseAddress,
                                  LocalSessionData,
                                  Length,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtWriteVirtualMemory() failed (Status %lx)\n", Status);
        goto done;
    }

    RequestMsg->GetLogonSessionData.Reply.SessionDataBuffer = ClientBaseAddress;

done:
    if (ProcessHandle != NULL)
        NtClose(ProcessHandle);

    if (LocalSessionData != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, LocalSessionData);

    return Status;
}

/* EOF */
