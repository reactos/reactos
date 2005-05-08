/* $Id$
 *
 * smapi.c - \SmApiPort LPC port message management
 *
 * Reactos Session Manager
 *
 */

#include "smss.h"
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES *********************************************************/

static HANDLE SmApiPort = INVALID_HANDLE_VALUE;

/* SM API *******************************************************************/

SMAPI(SmInvalid)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->SmHeader.Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}

/* SM API Table */
typedef NTSTATUS (FASTCALL * SM_PORT_API)(PSM_PORT_MESSAGE);

SM_PORT_API SmApi [] =
{
	SmInvalid,	/* unused */
	SmCompSes,	/* smapicomp.c */
	SmInvalid,	/* obsolete */
	SmInvalid,	/* unknown */
	SmExecPgm,	/* smapiexec.c */
	SmQryInfo	/* smapyqry.c */
};

/* TODO: optimize this address computation (it should be done 
 * with a macro) */
PSM_CONNECT_DATA FASTCALL SmpGetConnectData (PSM_PORT_MESSAGE Request)
{
	PLPC_MAX_MESSAGE LpcMaxMessage = (PLPC_MAX_MESSAGE) Request;
	return (PSM_CONNECT_DATA) & LpcMaxMessage->Data[0];
}

#if !defined(__USE_NT_LPC__)
NTSTATUS STDCALL
SmpHandleConnectionRequest (PSM_PORT_MESSAGE Request);
#endif

/**********************************************************************
 * SmpCallback/2
 *
 * The SM calls back a previously connected subsystem process to
 * authorizes it to bootstrap (initialize). The SM connects to a
 * named LPC port which name was sent in the connection data by the
 * candidate subsystem server process.
 */
static NTSTATUS
SmpCallbackServer (PSM_PORT_MESSAGE Request,
		   PSM_CLIENT_DATA ClientData)
{
	NTSTATUS          Status = STATUS_SUCCESS;
	PSM_CONNECT_DATA  ConnectData = SmpGetConnectData (Request);
	UNICODE_STRING    CallbackPortName;
	ULONG             CallbackPortNameLength = SM_SB_NAME_MAX_LENGTH; /* TODO: compute length */
	SB_CONNECT_DATA   SbConnectData;
	ULONG             SbConnectDataLength = sizeof SbConnectData;
	
	DPRINT("SM: %s called\n", __FUNCTION__);

	if(IMAGE_SUBSYSTEM_NATIVE == ConnectData->SubSystemId)
	{
		DPRINT("SM: %s: we do not need calling back SM!\n",
				__FUNCTION__);
		return STATUS_SUCCESS;
	}
	RtlCopyMemory (ClientData->SbApiPortName,
		       ConnectData->SbName,
		       CallbackPortNameLength);
	RtlInitUnicodeString (& CallbackPortName,
			      ClientData->SbApiPortName);

	SbConnectData.SmApiMax = (sizeof SmApi / sizeof SmApi[0]);
	Status = NtConnectPort (& ClientData->SbApiPort,
				& CallbackPortName,
				NULL,
				NULL,
				NULL,
				NULL,
				& SbConnectData,
				& SbConnectDataLength);
	return Status;
}

/**********************************************************************
 * NAME
 * 	SmpApiConnectedThread/1
 *
 * DESCRIPTION
 * 	Entry point for the listener thread of LPC port "\SmApiPort".
 */
VOID STDCALL
SmpApiConnectedThread(PVOID pConnectedPort)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PVOID		Unknown = NULL;
	PLPC_MESSAGE	Reply = NULL;
	SM_PORT_MESSAGE	Request = {{0}};
	HANDLE          ConnectedPort = * (PHANDLE) pConnectedPort;

	DPRINT("SM: %s called\n", __FUNCTION__);

	while (TRUE)
	{
		DPRINT("SM: %s: waiting for message\n",__FUNCTION__);

		Status = NtReplyWaitReceivePort(ConnectedPort,
						(PULONG) & Unknown,
						Reply,
						(PLPC_MESSAGE) & Request);
		if (NT_SUCCESS(Status))
		{
			DPRINT("SM: %s: message received (type=%d)\n",
				__FUNCTION__,
				PORT_MESSAGE_TYPE(Request));

			switch (Request.Header.MessageType)
			{
			case LPC_CONNECTION_REQUEST:
				SmpHandleConnectionRequest (&Request);
				Reply = NULL;
				break;
			case LPC_DEBUG_EVENT:
//				DbgSsHandleKmApiMsg (&Request, 0);
				Reply = NULL;
				break;
			case LPC_PORT_CLOSED:
			      Reply = NULL;
			      break;
			default:
				if ((Request.SmHeader.ApiIndex) &&
					(Request.SmHeader.ApiIndex < (sizeof SmApi / sizeof SmApi[0])))
				{
					Status = SmApi[Request.SmHeader.ApiIndex](&Request);
				      	Reply = (PLPC_MESSAGE) & Request;
				} else {
					Request.SmHeader.Status = STATUS_NOT_IMPLEMENTED;
					Reply = (PLPC_MESSAGE) & Request;
				}
			}
		} else {
			/* LPC failed */
			break;
		}
	}
	NtClose (ConnectedPort);
	NtTerminateThread (NtCurrentThread(), Status);
}

/**********************************************************************
 * NAME
 *	SmpHandleConnectionRequest/1
 *
 * ARGUMENTS
 * 	Request: LPC connection request message
 *
 * REMARKS
 * 	Quoted in http://support.microsoft.com/kb/258060/EN-US/
 */
NTSTATUS STDCALL
SmpHandleConnectionRequest (PSM_PORT_MESSAGE Request)
{
	PSM_CONNECT_DATA ConnectData = SmpGetConnectData (Request);
	NTSTATUS         Status = STATUS_SUCCESS;
	BOOL             Accept = FALSE;
	PSM_CLIENT_DATA  ClientData = NULL;
	HANDLE           hClientDataApiPort = (HANDLE) 0;
	PHANDLE          ClientDataApiPort = & hClientDataApiPort;
	HANDLE           hClientDataApiPortThread = (HANDLE) 0;
	PHANDLE          ClientDataApiPortThread = & hClientDataApiPortThread;
	PVOID            Context = NULL;
	
	DPRINT("SM: %s called:\n  SubSystemID=%d\n  SbName=\"%S\"\n",
			__FUNCTION__, ConnectData->SubSystemId, ConnectData->SbName);

	if(sizeof (SM_CONNECT_DATA) == Request->Header.DataSize)
	{
		if(IMAGE_SUBSYSTEM_UNKNOWN == ConnectData->SubSystemId)
		{
			/*
			 * This is not a call to register an image set,
			 * but a simple connection request from a process
			 * that will use the SM API.
			 */
			DPRINT("SM: %s: simple request\n", __FUNCTION__);
			ClientDataApiPort = & hClientDataApiPort;
			ClientDataApiPortThread = & hClientDataApiPortThread;
			Accept = TRUE;
		} else {
			DPRINT("SM: %s: request to register an image set\n", __FUNCTION__);
			/*
			 *  Reject GUIs classes: only odd subsystem IDs are
			 *  allowed to register here (tty mode images).
			 */
			if(1 == (ConnectData->SubSystemId % 2))
			{
				DPRINT("SM: %s: id = %d\n", __FUNCTION__, ConnectData->SubSystemId);
				/*
				 * SmCreateClient/2 is called here explicitly to *fail*.
				 * If it succeeds, there is something wrong in the
				 * connection request. An environment subsystem *never*
				 * registers twice. (security issue)
				 */
				Status = SmCreateClient (Request, & ClientData);
				if(STATUS_SUCCESS == Status)
				{
					DPRINT("SM: %s: ClientData = 0x%08lx\n",
						__FUNCTION__, ClientData);
					/*
					 * OK: the client is an environment subsystem
					 * willing to manage a free image type.
					 */
					ClientDataApiPort = & ClientData->ApiPort;
					ClientDataApiPortThread = & ClientData->ApiPortThread;
					/*
					 * Call back the candidate environment subsystem
					 * server (use the port name sent in in the 
					 * connection request message).
					 */
					Status = SmpCallbackServer (Request, ClientData);
					if(NT_SUCCESS(Status))
					{
						DPRINT("SM: %s: SmpCallbackServer OK\n",
							__FUNCTION__);
						Accept = TRUE;
					} else {
						DPRINT("SM: %s: SmpCallbackServer failed (Status=%08lx)\n",
							__FUNCTION__, Status);
						Status = SmDestroyClient (ConnectData->SubSystemId);
					}
				}
			}
		}
	}
	DPRINT("SM: %s: before NtAcceptConnectPort\n", __FUNCTION__);
#if defined(__USE_NT_LPC__)
	Status = NtAcceptConnectPort (ClientDataApiPort,
				      Context,
				      (PLPC_MESSAGE) Request,
				      Accept,
				      NULL,
				      NULL);
#else /* ReactOS LPC */
	Status = NtAcceptConnectPort (ClientDataApiPort,
				      SmApiPort, // ROS LPC requires the listen port here
				      Context,
				      Accept,
				      NULL,
				      NULL);
#endif
	if(Accept)
	{
		if(!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtAcceptConnectPort() failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
			return Status;
		} else {
			DPRINT("SM: %s: completing conn req\n", __FUNCTION__);
			Status = NtCompleteConnectPort (*ClientDataApiPort);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("SM: %s: NtCompleteConnectPort() failed (Status=0x%08lx)\n",
					__FUNCTION__, Status);
				return Status;
			}
#if !defined(__USE_NT_LPC__) /* ReactOS LPC */
			DPRINT("SM: %s: server side comm port thread (ROS LPC)\n", __FUNCTION__);
			Status = RtlCreateUserThread(NtCurrentProcess(),
					     NULL,
					     FALSE,
					     0,
					     NULL,
					     NULL,
					     (PTHREAD_START_ROUTINE) SmpApiConnectedThread,
					     ClientDataApiPort,
					     ClientDataApiPortThread,
					     NULL);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("SM: %s: Unable to create server thread (Status=0x%08lx)\n",
					__FUNCTION__, Status);
				return Status;
			}
#endif
		}
		Status = STATUS_SUCCESS;
	}
	DPRINT("SM: %s done\n", __FUNCTION__);
	return Status;
}

/**********************************************************************
 * NAME
 * 	SmpApiThread/1
 *
 * DECRIPTION
 * 	Due to differences in LPC implementation between NT and ROS,
 * 	we need a thread to listen for connection request that
 * 	creates a new thread for each connected port. This is not
 * 	necessary in NT LPC, because server side connected ports are
 * 	never used to receive requests. 
 */
VOID STDCALL
SmpApiThread (HANDLE ListeningPort)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	LPC_MAX_MESSAGE	Request = {{0}};

	DPRINT("SM: %s called\n", __FUNCTION__);
    
	while (TRUE)
	{
		Status = NtListenPort (ListeningPort, & Request.Header);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtListenPort() failed! (Status==x%08lx)\n", __FUNCTION__, Status);
			break;
		}
		Status = SmpHandleConnectionRequest ((PSM_PORT_MESSAGE) & Request);
		if(!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: SmpHandleConnectionRequest failed (Status=0x%08lx)\n",
				__FUNCTION__, Status);
			break;
		}
	}
	/* Cleanup */
	NtClose(ListeningPort);
	/* DIE */
	NtTerminateThread(NtCurrentThread(), Status);
}


/* LPC PORT INITIALIZATION **************************************************/


/**********************************************************************
 * NAME
 * 	SmCreateApiPort/0
 *
 * DECRIPTION
 */
NTSTATUS
SmCreateApiPort(VOID)
{
  OBJECT_ATTRIBUTES  ObjectAttributes = {0};
  UNICODE_STRING     UnicodeString = {0};
  NTSTATUS           Status = STATUS_SUCCESS;

  RtlRosInitUnicodeStringFromLiteral(&UnicodeString,
		       L"\\SmApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&SmApiPort,
			&ObjectAttributes,
			0,
			0,
			0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  /*
   * Create one thread for the named LPC
   * port \SmApiPort
   */
  RtlCreateUserThread(NtCurrentProcess(),
		      NULL,
		      FALSE,
		      0,
		      NULL,
		      NULL,
		      (PTHREAD_START_ROUTINE)SmpApiThread,
		      (PVOID)SmApiPort,
		      NULL,
		      NULL);

  return(Status);
}

/* EOF */
