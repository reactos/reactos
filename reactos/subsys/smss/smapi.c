/* $Id: smapi.c,v 1.9 2002/09/08 10:23:46 chorns Exp $
 *
 * Reactos Session Manager
 *
 *
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <napi/lpc.h>

#include "smss.h"

#define NDEBUG

/* GLOBAL VARIABLES *********************************************************/

static HANDLE SmApiPort = INVALID_HANDLE_VALUE;

/* FUNCTIONS ****************************************************************/


VOID STDCALL
SmApiThread(HANDLE Port)
{
  NTSTATUS Status;
  ULONG Unknown;
  PLPC_MESSAGE Reply = NULL;
  LPC_MESSAGE Message;

#ifndef NDEBUG
  DisplayString(L"SmApiThread: running\n");
#endif

  while (TRUE)
    {
#ifndef NDEBUG
      DisplayString(L"SmApiThread: waiting for message\n");
#endif

      Status = NtReplyWaitReceivePort(Port,
				      &Unknown,
				      Reply,
				      &Message);
      if (NT_SUCCESS(Status))
	{
#ifndef NDEBUG
	  DisplayString(L"SmApiThread: message received\n");
#endif

	  if (Message.MessageType == LPC_CONNECTION_REQUEST)
	    {
//	      SmHandleConnectionRequest (Port, &Message);
	      Reply = NULL;
	    }
	  else if (Message.MessageType == LPC_DEBUG_EVENT)
	    {
//	      DbgSsHandleKmApiMsg (&Message, 0);
	      Reply = NULL;
	    }
	  else if (Message.MessageType == LPC_PORT_CLOSED)
	    {
	      Reply = NULL;
	    }
	  else
	    {
//	      Reply = &Message;
	    }
	}
    }
}


NTSTATUS
SmCreateApiPort(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  NTSTATUS Status;

  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
		      (PTHREAD_START_ROUTINE)SmApiThread,
		      (PVOID)SmApiPort,
		      NULL,
		      NULL);

  RtlCreateUserThread(NtCurrentProcess(),
		      NULL,
		      FALSE,
		      0,
		      NULL,
		      NULL,
		      (PTHREAD_START_ROUTINE)SmApiThread,
		      (PVOID)SmApiPort,
		      NULL,
		      NULL);

  return(Status);
}

/* EOF */
