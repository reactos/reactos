/* $Id: compses.c 13731 2005-02-23 23:37:06Z ea $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smlib/compses.c
 * PURPOSE:         Call SM API SM_API_COMPLETE_SESSION
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>

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
  SmReqMsg.CompSes.hApiPort   = hApiPort;
  SmReqMsg.CompSes.hSbApiPort = hSbApiPort;

  /* SM API to invoke */
  SmReqMsg.ApiIndex = SM_API_COMPLETE_SESSION;

  /* Port message */
  SmReqMsg.Header.MessageType = LPC_NEW_MESSAGE;
  SmReqMsg.Header.DataSize    = SM_PORT_DATA_SIZE(SmReqMsg.CompSes);
  SmReqMsg.Header.MessageSize = SM_PORT_MESSAGE_SIZE;
  Status = NtRequestWaitReplyPort (hSmApiPort, (PLPC_MESSAGE) & SmReqMsg, (PLPC_MESSAGE) & SmReqMsg);
  if (NT_SUCCESS(Status))
  {
    return SmReqMsg.Status;
  }
  DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
