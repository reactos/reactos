/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smlib/execpgm.c
 * PURPOSE:         Call SM API SM_API_EXECPGM
 */
#include "precomp.h"

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
  SmReqMsg.Header.u2.s2.Type = LPC_NEW_MESSAGE;
  SmReqMsg.Header.u1.s1.DataLength    = SM_PORT_DATA_SIZE(SmReqMsg.Request);
  SmReqMsg.Header.u1.s1.TotalLength = SM_PORT_MESSAGE_SIZE;

  DPRINT("SMLIB: %s:\n"
	  "  u2.s2.Type = %d\n"
	  "  u1.s1.DataLength    = %d\n"
	  "  u1.s1.TotalLength = %d\n"
	  "  sizeof(PORT_MESSAGE)==%d\n",
	  __FUNCTION__,
	  SmReqMsg.Header.u2.s2.Type,
	  SmReqMsg.Header.u1.s1.DataLength,
	  SmReqMsg.Header.u1.s1.TotalLength,
	  sizeof(PORT_MESSAGE));

  /* Call SM and wait for a reply */
  Status = NtRequestWaitReplyPort (hSmApiPort, (PPORT_MESSAGE) & SmReqMsg, (PPORT_MESSAGE) & SmReqMsg);
  if (NT_SUCCESS(Status))
  {
    return SmReqMsg.SmHeader.Status;
  }
  DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
