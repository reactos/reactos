/* $Id$
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

NTSTATUS STDCALL
SmCompleteSession (HANDLE hSmApiPort, HANDLE hSbApiPort, HANDLE hApiPort)
{
  NTSTATUS         Status;
  SM_PORT_MESSAGE  SmReqMsg;
    
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
  DbgPrint ("%s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
