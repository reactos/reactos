/* $Id: session.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 * 
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/ob/session.c
 * DESCRIPTION: terminal
 * DATE       : 2002-04-04
 * AUTHOR     : Emanuele Aliberti <eal@users.sf.net>
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include <psxss.h>

#define   LOCK_ALL_SESSIONS RtlEnterCriticalSection(& Sessions.Lock)
#define UNLOCK_ALL_SESSIONS RtlLeaveCriticalSection(& Sessions.Lock)
#define   LOCK_THIS_SESSION RtlEnterCriticalSection(& Session->Lock)
#define UNLOCK_THIS_SESSION RtlLeaveCriticalSection(& Session->Lock)


/* A double-linked list for the PSX_SESSION instances */

static struct
{
    ULONG            NextFreeId;
    LONG             Count;
    PPSX_SESSION     List;
    CRITICAL_SECTION Lock;

} Sessions;

/**** FUNCTIONS ******************************************************/

/**********************************************************************
 *    PsxInitializeSessions/0
 *
 * DESCRIPTION
 *    Initialize the PSX session manager.
 * ARGUMENTS
 *    None.
 * RETURN VALUE
 *    None.
 */
NTSTATUS STDCALL
PsxInitializeSessions (VOID)
{
    debug_print (L"PSXSS: ->"__FUNCTION__);
    /* Initalize the attributes */
    Sessions.NextFreeId = 0;
    Sessions.Count = 0;
    Sessions.List = NULL;
    RtlInitializeCriticalSection (& Sessions.Lock);
    return STATUS_SUCCESS;
}
/**********************************************************************
 *    PsxCreateSessionObjects/1					PRIVATE
 *
 */
PRIVATE NTSTATUS STDCALL
PsxCreateSessionObjects (
   IN     PLPC_MAX_MESSAGE pRequest,
   IN OUT PPSX_SESSION     pSession
   )
{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           Oa;
    WCHAR                       NameBuffer [NAME_BUFFER_SIZE];
    UNICODE_STRING              Name;
    SECURITY_QUALITY_OF_SERVICE Sqos;
    PSX_CONNECT_PORT_DATA	ConnectData;
    ULONG			ConnectDataSize = sizeof ConnectData;

    /* Port */
    swprintf (
        NameBuffer,
        PSX_NS_SESSION_PORT_TEMPLATE,
        PSX_NS_SUBSYSTEM_DIRECTORY_NAME,
        PSX_NS_SESSION_DIRECTORY_NAME,
        pRequest->Header.Cid.UniqueProcess
        );
    debug_print (L"PSXSS: "__FUNCTION__": %s", NameBuffer);
    RtlInitUnicodeString (& Name, NameBuffer);
    InitializeObjectAttributes (& Oa, & Name, 0, NULL, NULL);
    RtlZeroMemory (& Sqos, sizeof Sqos);
    ConnectData.ConnectionType = PSX_CONNECTION_TYPE_SERVER;
    ConnectData.Version        = PSX_LPC_PROTOCOL_VERSION;
    ConnectData.PortIdentifier = 0;
    Status = NtConnectPort (
              & pSession->TerminalChannel.hPort,
              & Name,
              & Sqos,
	      NULL,
	      NULL,
	      NULL,
	      (PVOID) & ConnectData,
	      & ConnectDataSize
	      );
    if (!NT_SUCCESS(Status))
    {
        debug_print (L"PSXSS: "__FUNCTION__": NtConnectPort failed with %08x\n", Status);
        return Status;
    }
    /* TODO:  */
    /* Section */
    swprintf (
        NameBuffer,
        PSX_NS_SESSION_DATA_TEMPLATE,
        PSX_NS_SUBSYSTEM_DIRECTORY_NAME,
        PSX_NS_SESSION_DIRECTORY_NAME,
        pRequest->Header.Cid.UniqueProcess
    );
    debug_print (L"PSXSS: "__FUNCTION__": %s", NameBuffer);
    RtlInitUnicodeString (& Name, NameBuffer); 
    InitializeObjectAttributes (& Oa, & Name, 0, 0, 0);
    Status = NtOpenSection (
                & pSession->TerminalChannel.Section.Handle,
                SECTION_ALL_ACCESS, /* DesiredAccess */
                & Oa
                );
    if (!NT_SUCCESS(Status))
    {
        NtClose (pSession->TerminalChannel.hPort);
        debug_print (L"PSXSS: "__FUNCTION__": NtOpenSection failed with %08x\n", Status);
        return Status;
    }
    pSession->TerminalChannel.Section.BaseAddress = NULL;
    pSession->TerminalChannel.Section.ViewSize = PSX_TERMINAL_SECTION_SIZE;
    Status = NtMapViewOfSection (
                pSession->TerminalChannel.Section.Handle,
                NtCurrentProcess(),
                & pSession->TerminalChannel.Section.BaseAddress,
                0, /* ZeroBits */
                0, /* Commitsize */
                0, /* SectionOffset */
                & pSession->TerminalChannel.Section.ViewSize,
                ViewUnmap,
                0, /* AllocationType */
                PAGE_READWRITE /* Protect 4 */
                );
    if (!NT_SUCCESS(Status))
    {
        NtClose (pSession->TerminalChannel.hPort);
        NtClose (pSession->TerminalChannel.Section.Handle);
        debug_print (L"PSXSS: "__FUNCTION__": NtMapViewOfSection failed with %08x\n", Status);
        return Status;
    }
    return Status;
}
/**********************************************************************
 *    PsxCreateSession/3
 *
 * DESCRIPTION
 *    Create a new PSX_SESSION object and insert it in the
 *    PSX sessions table.
 * ARGUMENTS
 *    MessageHeader
 *    Id
 * RETURN VALUE
 *    A status value on error; otherwise STATUS_SUCCESS.
 */
NTSTATUS STDCALL
PsxCreateSession (
    IN    PLPC_MAX_MESSAGE    pRequest,
    IN    HANDLE              hConnectedPort,
    IN    ULONG               ulPortIdentifier
    )
{
    PPSX_SESSION Session = NULL;

    debug_print (L"PSXSS: ->"__FUNCTION__);
    /* Create the PSX_SESSION object */
    Session = RtlAllocateHeap (Server.Heap, 0, sizeof (PSX_SESSION));
    if (NULL == Session)
    {
        debug_print (L"PSXSS: "__FUNCTION__": failed to create a new session object");
        return STATUS_MEMORY_NOT_ALLOCATED;
    }
    RtlZeroMemory (Session, sizeof (PSX_SESSION));
    /* Initialiaze the new PSX_SESSION object */
    Session->SessionChannel.hPort = hConnectedPort;
    Session->SessionChannel.ulPortIdentifier =  ulPortIdentifier;
    LOCK_ALL_SESSIONS;
    Session->Id = Sessions.NextFreeId ++;
    UNLOCK_ALL_SESSIONS;
    Session->Status = SESSION_STATUS_INITIALIZATION;
    Session->Heap =
        RtlCreateHeap (
                HEAP_GROWABLE,
                NULL,
                65536,
                65536,
                NULL,
                NULL
                );
    if (INVALID_HANDLE_VALUE == Session->Heap)
    {
        RtlFreeHeap (Server.Heap, 0, Session);
        debug_print (L"PSX: "__FUNCTION__": failed to create a new heap for session # %d", Session->Id);
        return STATUS_MEMORY_NOT_ALLOCATED;
    }
    RtlInitializeCriticalSection (& Session->Lock);
    /* TODO: open the terminal's shared section */
    /* TODO: connect to the terminal's port */        
    /* Inset the new PSX_SESSION object in the sessions table */
    LOCK_ALL_SESSIONS;
    if (NULL == Sessions.List)
    {
        Sessions.List = Session;
        Session->Previous = Session;
    }
    Session->Next           = Sessions.List; /* Last one points to the top one */
    Session->Previous       = Sessions.List->Previous;
    Sessions.List->Previous = Session; /* Top one now points to the new one (tail) */
    ++ Sessions.Count;
    UNLOCK_ALL_SESSIONS;
    /* DONE */
    debug_print (L""__FUNCTION__": session # %d created", Session->Id);
    Session->Status = SESSION_STATUS_READY;
    return STATUS_SUCCESS;
}
/**********************************************************************
 *    PsxTerminateSession/1
 *
 * DESCRIPTION
 *    Remove a PSX_SESSION object from the PSX sessions table and
 *    destroy it.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *    A status value on error; otherwise STATUS_SUCCESS.
 */
NTSTATUS STDCALL
PsxTerminateSession (
    IN    PPSX_SESSION Session
    )
{
    LONG         Id;
    PPSX_SESSION Previous = NULL;
    PPSX_SESSION Next = NULL;

    /* Release any resource managed by the session */
    RtlDestroyHeap (Session->Heap);
    /* Remove the session object from the sessions table */
    LOCK_ALL_SESSIONS;
    Id = Session->Id;
    Previous = Session->Previous;
    Next     = Session->Next;
    /* TODO: handle the case of no session left */
    Next->Previous = Previous;
    Previous->Next = Next;
    -- Sessions.Count;
    UNLOCK_ALL_SESSIONS;
    /* Delete the old PSX_SESSION object */
    RtlFreeHeap (Server.Heap, 0, Session);
    /* DONE */
    debug_print(L"PSX: session # %d deleted", Id);
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
PsxWriteTerminalSession (
    IN     PPSX_SESSION Session,
    IN     PVOID        Buffer,
    IN     ULONG        Size,
    IN OUT PULONG       Written
    )
{
    LOCK_THIS_SESSION;
    /* TODO: lock this session's section for writing */
    /* TODO: copy the data in this session's section */
    /* TODO: request a WRITE operation to the session terminal */
    /* TODO: unlock this session's section */
    UNLOCK_THIS_SESSION;
}

NTSTATUS STDCALL
PsxReadTerminalSession (
    IN     PPSX_SESSION Session,
       OUT PVOID        Buffer,
    IN     ULONG        Size,
    IN OUT PULONG       Read
    )
{
    LOCK_THIS_SESSION;
    /* TODO: lock this session's section for reading */
    /* TODO: request a READ operation to the session terminal */
    /* TODO: copy the data from this session's section */
    /* TODO: unlock this session's section */
    UNLOCK_THIS_SESSION;
}
/* EOF */
