/* $Id: connect.c,v 1.4 2001/01/29 00:13:21 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/connect.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME							EXPORTED
 * 	NtConnectPort@32
 * 	
 * DESCRIPTION
 *	Connect to a named port and wait for the other side to 
 *	accept the connection.
 *
 * ARGUMENTS
 *	ConnectedPort
 *	PortName
 *	Qos
 *	WriteMap
 *	ReadMap
 *	MaxMessageSize
 *	ConnectInfo
 *	UserConnectInfoLength
 * 
 * RETURN VALUE
 * 
 */
NTSTATUS STDCALL
NtConnectPort (PHANDLE				ConnectedPort,
	       PUNICODE_STRING			PortName,
	       PSECURITY_QUALITY_OF_SERVICE	Qos,
	       PLPC_SECTION_WRITE		WriteMap,
	       PLPC_SECTION_READ		ReadMap,
	       PULONG				MaxMessageSize,
	       PVOID				ConnectInfo,
	       PULONG				UserConnectInfoLength)
{
  NTSTATUS	Status;
  PEPORT		NamedPort;
  PEPORT		OurPort;
  HANDLE		OurPortHandle;
  PLPC_MESSAGE	Request;
  PQUEUEDMESSAGE	Reply;
  ULONG		ConnectInfoLength;
  KIRQL		oldIrql;
   
  DPRINT("PortName %x\n", PortName);
  DPRINT("NtConnectPort(PortName %S)\n", PortName->Buffer);
  
  /*
   * Copy in user parameters
   */
  memcpy (&ConnectInfoLength, UserConnectInfoLength, 
	  sizeof (*UserConnectInfoLength));

  /*
   * Get access to the port
   */
  Status = ObReferenceObjectByName (PortName,
				    0,
				    NULL,
				    PORT_ALL_ACCESS,  /* DesiredAccess */
				    ExPortType,
				    UserMode,
				    NULL,
				    (PVOID*)&NamedPort);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to reference named port (status %x)\n", Status);
      return (Status);
    }
  /*
   * Create a port to represent our side of the connection
   */
  OurPort = ObCreateObject (&OurPortHandle,
			    PORT_ALL_ACCESS,
			    NULL,
			    ExPortType);
  NiInitializePort(OurPort);
  /*
   * Create a request message
   */
  DPRINT("Creating request message\n");
  
  Request = ExAllocatePool (NonPagedPool,
			    (sizeof (LPC_MESSAGE) + ConnectInfoLength));
   
  Request->DataSize = ConnectInfoLength;
  Request->MessageSize = sizeof(LPC_MESSAGE) + ConnectInfoLength;
  Request->SharedSectionSize = 0;
  if ((ConnectInfo != NULL) && (ConnectInfoLength > 0))
    {
      memcpy ((PVOID) (Request + 1), ConnectInfo, ConnectInfoLength);
    }
  /*
   * Queue the message to the named port
   */
  DPRINT("Queuing message\n");
  
  EiReplyOrRequestPort (NamedPort,
			Request,
			LPC_CONNECTION_REQUEST,
			OurPort);
  KeSetEvent (&NamedPort->Event, IO_NO_INCREMENT, FALSE);
   
  DPRINT("Waiting for connection completion\n");
  
  /*
   * Wait for them to accept our connection
   */
  KeWaitForSingleObject (&OurPort->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);

  DPRINT("Received connection completion\n");
  KeAcquireSpinLock (&OurPort->Lock, &oldIrql);
  Reply = EiDequeueMessagePort (OurPort);
  KeReleaseSpinLock (&OurPort->Lock, oldIrql);
  memcpy (ConnectInfo, Reply->MessageData, Reply->Message.DataSize); 
  *UserConnectInfoLength = Reply->Message.DataSize;
  
  if (Reply->Message.MessageType == LPC_CONNECTION_REFUSED)
    {
      ObDereferenceObject (NamedPort);
      ObDereferenceObject (OurPort);
      ZwClose (OurPortHandle);
      ExFreePool (Request);
      ExFreePool (Reply);
      return (STATUS_UNSUCCESSFUL);
    }
  
  OurPort->State = EPORT_CONNECTED_CLIENT;
  *ConnectedPort = OurPortHandle;   
  ExFreePool (Reply);
  ExFreePool (Request);
  
  DPRINT("Exited successfully\n");
  
  return (STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtAcceptConnectPort@24
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ServerPortHandle
 *	NamedPortHandle
 *	LpcMessage
 *	AcceptIt
 *	WriteMap
 *	ReadMap
 *
 * RETURN VALUE
 *
 */
EXPORTED NTSTATUS STDCALL
NtAcceptConnectPort (PHANDLE			ServerPortHandle,
		     HANDLE			NamedPortHandle,
		     PLPC_MESSAGE		LpcMessage,
		     BOOLEAN			AcceptIt,
		     PLPC_SECTION_WRITE	WriteMap,
		     PLPC_SECTION_READ	ReadMap)
{
  NTSTATUS	Status;
  PEPORT		NamedPort;
  PEPORT		OurPort = NULL;
  PQUEUEDMESSAGE	ConnectionRequest;
  KIRQL		oldIrql;
  
  Status = ObReferenceObjectByHandle (NamedPortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&NamedPort,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return (Status);
    }
  /*
   * Create a port object for our side of the connection
   */
  if (AcceptIt == 1)
    {
      OurPort = ObCreateObject (ServerPortHandle,
				PORT_ALL_ACCESS,
				NULL,
				ExPortType);
      NiInitializePort(OurPort);
    }
  /*
   * Dequeue the connection request
   */
  KeAcquireSpinLock (&NamedPort->Lock, & oldIrql);
  ConnectionRequest = EiDequeueConnectMessagePort (NamedPort);
  KeReleaseSpinLock (&NamedPort->Lock, oldIrql);
  
  if (AcceptIt != 1)
    {	
      EiReplyOrRequestPort (ConnectionRequest->Sender, 
			    LpcMessage, 
			    LPC_CONNECTION_REFUSED,
			    NamedPort);
      KeSetEvent (&ConnectionRequest->Sender->Event,
		  IO_NO_INCREMENT,
		  FALSE);
      ObDereferenceObject (ConnectionRequest->Sender);
      ExFreePool (ConnectionRequest);	
      ObDereferenceObject (NamedPort);
      return (STATUS_SUCCESS);
    }
  /*
   * Connect the two ports
   */
  OurPort->OtherPort = ConnectionRequest->Sender;
  OurPort->OtherPort->OtherPort = OurPort;
  EiReplyOrRequestPort (ConnectionRequest->Sender, 
			LpcMessage, 
			LPC_REPLY,
			OurPort);
  ExFreePool (ConnectionRequest);
   
  ObDereferenceObject (OurPort);   
  ObDereferenceObject (NamedPort);
  
  return (STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	NtSecureConnectPort@36
 * 	
 * DESCRIPTION
 *	Connect to a named port and wait for the other side to 
 *	accept the connection. Possibly verify that the server
 *	matches the ServerSid (trusted server).
 *	Present in w2k+.
 *
 * ARGUMENTS
 *	ConnectedPort
 *	PortName
 *	Qos
 *	WriteMap
 *	ServerSid
 *	ReadMap
 *	MaxMessageSize
 *	ConnectInfo
 *	UserConnectInfoLength
 * 
 * RETURN VALUE
 * 
 */
NTSTATUS STDCALL
NtSecureConnectPort (OUT    PHANDLE				ConnectedPort,
		     IN     PUNICODE_STRING			PortName,
		     IN     PSECURITY_QUALITY_OF_SERVICE	Qos,
		     IN OUT PLPC_SECTION_WRITE			WriteMap		OPTIONAL,
		     IN     PSID				ServerSid		OPTIONAL,
		     IN OUT PLPC_SECTION_READ			ReadMap			OPTIONAL,
		     OUT    PULONG				MaxMessageSize		OPTIONAL,
		     IN OUT PVOID				ConnectInfo		OPTIONAL,
		     IN OUT PULONG				UserConnectInfoLength	OPTIONAL)
{
	return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
