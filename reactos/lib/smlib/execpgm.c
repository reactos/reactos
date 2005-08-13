/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smlib/execpgm.c
 * PURPOSE:         Call SM API SM_API_EXECPGM
 */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <sm/helper.h>

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	SmExecuteProgram/2
 *
 * DESCRIPTION
 *	This function is used to make the SM start an environment
 *	subsystem server process.
 *
 * ARGUMENTS
 * 	hSmApiPort: port handle returned by SmConnectApiPort;
 * 	Pgm       : name of the subsystem (to be used by the SM to
 * 	            lookup the image name from the registry).
 * 	            Valid names are: DEBUG, WINDOWS, POSIX, OS2,
 * 	            and VMS.
 *	
 * RETURN VALUE
 * 	Success status as handed by the SM reply; otherwise a failure
 * 	status code.
 */
NTSTATUS STDCALL
SmExecuteProgram (IN HANDLE          hSmApiPort,
		  IN PUNICODE_STRING Pgm)
{
  NTSTATUS         Status;
  SM_PORT_MESSAGE  SmReqMsg;


  DPRINT("SMLIB: %s(%08lx,'%S') called\n",
	__FUNCTION__, hSmApiPort, Pgm->Buffer);

  /* Check Pgm's length */
  if (Pgm->Length > (sizeof (Pgm->Buffer[0]) * SM_EXEXPGM_MAX_LENGTH))
  {
    return STATUS_INVALID_PARAMETER;
  }
  /* Marshal Pgm in the LPC message */
  RtlZeroMemory (& SmReqMsg, sizeof SmReqMsg);
  SmReqMsg.Request.ExecPgm.NameLength = Pgm->Length;
  RtlCopyMemory (SmReqMsg.Request.ExecPgm.Name,
		 Pgm->Buffer,
		 Pgm->Length);
		
  /* SM API to invoke */
  SmReqMsg.SmHeader.ApiIndex = SM_API_EXECUTE_PROGRAMME;

  /* LPC message */
  SmReqMsg.Header.MessageType = LPC_NEW_MESSAGE;
  SmReqMsg.Header.DataSize    = SM_PORT_DATA_SIZE(SmReqMsg.Request);
  SmReqMsg.Header.MessageSize = SM_PORT_MESSAGE_SIZE;

  DPRINT("SMLIB: %s:\n"
	  "  MessageType = %d\n"
	  "  DataSize    = %d\n"
	  "  MessageSize = %d\n"
	  "  sizeof(LPC_MESSAGE)==%d\n",
	  __FUNCTION__,
	  SmReqMsg.Header.MessageType,
	  SmReqMsg.Header.DataSize,
	  SmReqMsg.Header.MessageSize,
	  sizeof(LPC_MESSAGE));

  /* Call SM and wait for a reply */
  Status = NtRequestWaitReplyPort (hSmApiPort, (PLPC_MESSAGE) & SmReqMsg, (PLPC_MESSAGE) & SmReqMsg);
  if (NT_SUCCESS(Status))
  {
    return SmReqMsg.SmHeader.Status;
  }
  DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
