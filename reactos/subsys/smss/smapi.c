/* $Id$
 *
 * smapi.c - \SmApiPort LPC port message management
 *
 * Reactos Session Manager
 *
 */

#include "smss.h"
#include <rosrtl/string.h>

//#define NDEBUG
#include <debug.h>

/* GLOBAL VARIABLES *********************************************************/

static HANDLE SmApiPort = INVALID_HANDLE_VALUE;

/* SM API *******************************************************************/

SMAPI(SmInvalid)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
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
	SmExecPgm	/* smapiexec.c */
};

#if !defined(__USE_NT_LPC__)
NTSTATUS STDCALL
SmpHandleConnectionRequest (PSM_PORT_MESSAGE Request);
#endif


/**********************************************************************
 * NAME
 * 	SmpApiConnectedThread/1
 *
 * DESCRIPTION
 * 	Entry point for the listener thread of LPC port "\SmApiPort".
 */
VOID STDCALL
SmpApiConnectedThread(PVOID dummy)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PVOID		Unknown = NULL;
	PLPC_MESSAGE	Reply = NULL;
	SM_PORT_MESSAGE	Request = {{0}};

	DPRINT("SM: %s running\n",__FUNCTION__);

	while (TRUE)
	{
		DPRINT("SM: %s: waiting for message\n",__FUNCTION__);

		Status = NtReplyWaitReceivePort(SmApiPort,
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
				if ((Request.ApiIndex) &&
					(Request.ApiIndex < (sizeof SmApi / sizeof SmApi[0])))
				{
					Status = SmApi[Request.ApiIndex](&Request);
				      	Reply = (PLPC_MESSAGE) & Request;
				} else {
					Request.Status = STATUS_NOT_IMPLEMENTED;
					Reply = (PLPC_MESSAGE) & Request;
				}
			}
		}
	}
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
#if defined(__USE_NT_LPC__)
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  ClientData = NULL;
	PVOID            Context = NULL;
	
	DPRINT("SM: %s called\n",__FUNCTION__);

	/*
	 * SmCreateClient/2 is called here explicitly to *fail*.
	 * If it succeeds, there is something wrong in the
	 * connection request. An environment subsystem *never*
	 * registers twice. (Security issue: maybe we will
	 * write this event into the security log).
	 */
	Status = SmCreateClient (Request, & ClientData);
	if(STATUS_SUCCESS == Status)
	{
		/* OK: the client is an environment subsystem
		 * willing to manage a free image type.
		 * Accept it.
		 */
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      (PLPC_MESSAGE) Request,
					      TRUE, //accept
					      NULL,
					      NULL);
		if(NT_SUCCESS(Status))
		{
			Status = NtCompleteConnectPort(ClientData->ApiPort);
		}
		return STATUS_SUCCESS;
	} else {
		/* Reject the subsystem */
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      (PLPC_MESSAGE) Request,
					      FALSE, //reject
					      NULL,
					      NULL);
	}
#else /* ReactOS LPC */
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  ClientData = NULL;
	
	DPRINT("SM: %s called\n",__FUNCTION__);

	Status = SmCreateClient (Request, & ClientData);
	if(STATUS_SUCCESS == Status)
	{
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      SmApiPort,
					      NULL,
					      TRUE, //accept
					      NULL,
					      NULL);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtAcceptConnectPort() failed (Status=0x%08lx)\n",
					__FUNCTION__, Status);
			return Status;
		} else {
			Status = NtCompleteConnectPort(ClientData->ApiPort);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("SM: %s: NtCompleteConnectPort() failed (Status=0x%08lx)\n",
					__FUNCTION__, Status);
				return Status;
			}
			Status = RtlCreateUserThread(NtCurrentProcess(),
					     NULL,
					     FALSE,
					     0,
					     NULL,
					     NULL,
					     (PTHREAD_START_ROUTINE) SmpApiConnectedThread,
					     ClientData->ApiPort,
					     & ClientData->ApiPortThread,
					     NULL);
			if (!NT_SUCCESS(Status))
			{
				DPRINT1("SM: %s: Unable to create server thread (Status=0x%08lx)\n",
					__FUNCTION__, Status);
				return Status;
			}
		}
		return STATUS_SUCCESS;
	} else {
		/* Reject the subsystem */
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      SmApiPort,
					      NULL,
					      FALSE, //reject
					      NULL,
					      NULL);
	}
#endif /* defined __USE_NT_LPC__ */
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
