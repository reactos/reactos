/* $Id$
 *
 * Reactos Session Manager
 *
 *
 */

/*#include <ddk/ntddk.h>
#include <ntdll/rtl.h>*/
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <rosrtl/string.h>
#include "smss.h"

#define NDEBUG

/* GLOBAL VARIABLES *********************************************************/

static HANDLE SmApiPort = INVALID_HANDLE_VALUE;

/* SM API *******************************************************************/

#define SMAPI(n) \
NTSTATUS FASTCALL n (PSM_PORT_MESSAGE Request)

SMAPI(SmInvalid)
{
	DbgPrint("SMSS: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}

SMAPI(SmCompSes)
{
	DbgPrint("SMSS: %s called\n",__FUNCTION__);
	Request->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_SUCCESS;
}
SMAPI(SmExecPgm)
{
	DbgPrint("SMSS: %s called\n",__FUNCTION__);
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
	DbgPrint("SMSS: %s called\n",__FUNCTION__);
	return STATUS_SUCCESS;
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
	ULONG		Unknown = 0;
	PLPC_MESSAGE	Reply = NULL;
	SM_PORT_MESSAGE	Request = {{0}};

	DbgPrint("SMSS: %s running.\n",__FUNCTION__);

	while (TRUE)
	{
		DbgPrint("SMSS: %s: waiting for message\n",__FUNCTION__);

		Status = NtReplyWaitReceivePort(Port,
						& Unknown,
						Reply,
						(PLPC_MESSAGE) & Request);
		if (NT_SUCCESS(Status))
		{
			DbgPrint("SMSS: %s: message received\n",__FUNCTION__);

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
