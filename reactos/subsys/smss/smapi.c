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

#define SMAPI(n) \
NTSTATUS FASTCALL n (PSM_PORT_MESSAGE Request)

SMAPI(SmInvalid)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}

SMAPI(SmCompSes)
{
	DPRINT("SM: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}
SMAPI(SmExecPgm)
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
	SmCompSes,
	SmInvalid,	/* obsolete */
	SmInvalid,	/* unknown */
	SmExecPgm
};


/**********************************************************************
 * NAME
 *	SmpHandleConnectionRequest/2
 *
 * REMARKS
 * 	Quoted in http://support.microsoft.com/kb/258060/EN-US/
 */
NTSTATUS STDCALL
SmpHandleConnectionRequest (HANDLE Port, PSM_PORT_MESSAGE Request)
{
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  ClientData = NULL;
	PVOID            Context = NULL;
	
	DPRINT("SM: %s called\n",__FUNCTION__);

	Status = SmCreateClient (Request, & ClientData);
	if(STATUS_SUCCESS == Status)
	{
#ifdef __USE_NT_LPC__
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      SmApiPort,
					      TRUE, //accept
					      NULL,
					      NULL);
#else
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      (PLPC_MESSAGE) Request,
					      TRUE, //accept
					      NULL,
					      NULL);
#endif
		if(NT_SUCCESS(Status))
		{
			Status = NtCompleteConnectPort(ClientData->ApiPort);
		}
		return STATUS_SUCCESS;
	} else {
		/* Reject the subsystem */
#ifdef __USE_NT_LPC__
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      SmApiPort,
					      FALSE, //reject
					      NULL,
					      NULL);
#else
		Status = NtAcceptConnectPort (& ClientData->ApiPort,
					      Context,
					      (PLPC_MESSAGE) Request,
					      FALSE, //reject
					      NULL,
					      NULL);
#endif
	}
	return Status;
}

/**********************************************************************
 * NAME
 * 	SmpApiThread/1
 *
 * DESCRIPTION
 * 	Entry point for the listener thread of LPC port "\SmApiPort".
 */
VOID STDCALL
SmpApiThread(HANDLE Port)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	PVOID		Unknown = NULL;
	PLPC_MESSAGE	Reply = NULL;
	SM_PORT_MESSAGE	Request = {{0}};

	DPRINT("SM: %s running\n",__FUNCTION__);

	while (TRUE)
	{
		DPRINT("SM: %s: waiting for message\n",__FUNCTION__);

		Status = NtReplyWaitReceivePort(Port,
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
				SmpHandleConnectionRequest (Port, &Request);
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
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  NTSTATUS Status;

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

  /* Create two threads for "\SmApiPort" */
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
