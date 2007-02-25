/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smlib/compses.c
 * PURPOSE:         Call SM API SM_API_COMPLETE_SESSION
 */
#include "precomp.h"

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	SmCompleteSession/3
 *
 * DESCRIPTION
 * 	This function is called by an environment subsystem server to
 * 	tell the SM it finished initialization phase and is ready to
 * 	manage processes it registered for (SmConnectApiPort).
 *
 * ARGUMENTS
 * 	hSmApiPort: port handle returned by SmConnectApiPort;
 * 	hSbApiPort: call back API port of the subsystem (handle);
 * 	hApiPort  : API port of the subsystem (handle).
 *
 * RETURN VALUE
 * 	Success status as handed by the SM reply; otherwise a failure
 * 	status code.
 */
NTSTATUS STDCALL
SmCompleteSession (IN HANDLE hSmApiPort,
		   IN HANDLE hSbApiPort,
		   IN HANDLE hApiPort)
{
  NTSTATUS         Status;
  SM_PORT_MESSAGE  SmReqMsg;
    
  DPRINT("SMLIB: %s called\n", __FUNCTION__);

  /* Marshal Ses in the LPC message */
  SmReqMsg.Request.CompSes.hApiPort   = hApiPort;
  SmReqMsg.Request.CompSes.hSbApiPort = hSbApiPort;

  /* SM API to invoke */
  SmReqMsg.SmHeader.ApiIndex = SM_API_COMPLETE_SESSION;

  /* Port message */
  SmReqMsg.Header.u2.s2.Type = LPC_NEW_MESSAGE;
  SmReqMsg.Header.u1.s1.DataLength    = SM_PORT_DATA_SIZE(SmReqMsg.Request);
  SmReqMsg.Header.u1.s1.TotalLength = SM_PORT_MESSAGE_SIZE;
  Status = NtRequestWaitReplyPort (hSmApiPort, (PPORT_MESSAGE) & SmReqMsg, (PPORT_MESSAGE) & SmReqMsg);
  if (NT_SUCCESS(Status))
  {
    return SmReqMsg.SmHeader.Status;
  }
  DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
