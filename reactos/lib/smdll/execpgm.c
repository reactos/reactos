/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/smdll/execpgm.c
 * PURPOSE:         Call SM API SM_API_EXECPGM
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>
#include <string.h>

NTSTATUS STDCALL
SmExecPgm (HANDLE hSmApiPort, PUNICODE_STRING Pgm)
{
  NTSTATUS         Status;
  SM_PORT_MESSAGE  SmReqMsg;


  /* Check Pgm's length */
  if (Pgm->Length > (sizeof (Pgm->Buffer[0]) * SM_EXEXPGM_MAX_LENGTH))
  {
    return STATUS_INVALID_PARAMETER;
  }
  /* Marshal Pgm in the LPC message */
  RtlZeroMemory (& SmReqMsg, sizeof SmReqMsg);
  SmReqMsg.ExecPgm.NameLength = Pgm->Length;
  RtlCopyMemory (SmReqMsg.ExecPgm.Name, Pgm->Buffer, Pgm->Length);
		
  /* SM API to invoke */
  SmReqMsg.ApiIndex = SM_API_EXECUTE_PROGRAMME;

  /* LPC message */
  SmReqMsg.Header.MessageType = LPC_NEW_MESSAGE;
  SmReqMsg.Header.DataSize    = SM_PORT_DATA_SIZE(SmReqMsg.ExecPgm);
  SmReqMsg.Header.MessageSize = SM_PORT_MESSAGE_SIZE;

  /* Call SM and wait for a reply */
  Status = NtRequestWaitReplyPort (hSmApiPort, (PLPC_MESSAGE) & SmReqMsg, (PLPC_MESSAGE) & SmReqMsg);
  if (NT_SUCCESS(Status))
  {
    return SmReqMsg.Status;
  }
  DbgPrint ("%s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
