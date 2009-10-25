/* $Id$
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       reactos/lib/smlib/connect.c
 * PURPOSE:    Connect to the API LPC port exposed by the SM
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	SmConnectApiPort/4
 *
 * DESCRIPTION
 *	Connect to SM API port and register a session "begin" port (Sb)
 *	or to issue API requests to SmApiPort.
 *
 * ARGUMENTS
 *	pSbApiPortName: name of the Sb port the calling subsystem
 *		server already created in the system name space;
 *	hSbApiPort: LPC port handle (checked, but not used: the
 *		subsystem is required to have already created
 *		the callback port before it connects to the SM);
 *	wSubsystem: a valid IMAGE_SUBSYSTEM_xxx value;
 *	phSmApiPort: a pointer to a HANDLE, which will be
 *		filled with a valid client-side LPC comm port.
 *
 *	There should be only two ways to call this API:
 *	a) subsystems willing to register with SM will use it
 *	   with full parameters (the function checks them);
 *	b) regular SM clients, will set to 0 the 1st, the 2nd,
 *	   and the 3rd parameter.
 *
 * RETURN VALUE
 * 	If all three optional values are omitted, an LPC status.
 * 	STATUS_INVALID_PARAMETER_MIX if PortName is defined and
 * 	both hSbApiPort and wSubsystem are 0.
 */
NTSTATUS WINAPI
SmConnectApiPort (IN      PUNICODE_STRING  pSbApiPortName  OPTIONAL,
		  IN      HANDLE           hSbApiPort      OPTIONAL,
		  IN      WORD             wSubSystemId    OPTIONAL,
		  IN OUT  PHANDLE          phSmApiPort)
{
  UNICODE_STRING              SmApiPortName;
  SECURITY_QUALITY_OF_SERVICE SecurityQos;
  NTSTATUS                    Status = STATUS_SUCCESS;
  SM_CONNECT_DATA             ConnectData = {0,0,{0}};
  ULONG                       ConnectDataLength = 0;

  DPRINT("SMLIB: %s called\n", __FUNCTION__);

  if (pSbApiPortName)
  {
    if (pSbApiPortName->Length > (sizeof pSbApiPortName->Buffer[0] * SM_SB_NAME_MAX_LENGTH))
    {
	  return STATUS_INVALID_PARAMETER_1;
    }
    if (NULL == hSbApiPort || IMAGE_SUBSYSTEM_UNKNOWN == wSubSystemId)
    {
      return STATUS_INVALID_PARAMETER_MIX;
    }
    RtlZeroMemory (& ConnectData, sizeof ConnectData);
    ConnectData.Unused = 0;
    ConnectData.SubSystemId = wSubSystemId;
    if (pSbApiPortName->Length > 0)
    {
      RtlCopyMemory (& ConnectData.SbName,
		     pSbApiPortName->Buffer,
		     pSbApiPortName->Length);
    }
  }
  ConnectDataLength = sizeof ConnectData;

  SecurityQos.Length              = sizeof (SecurityQos);
  SecurityQos.ImpersonationLevel  = SecurityIdentification;
  SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
  SecurityQos.EffectiveOnly       = TRUE;

  RtlInitUnicodeString (& SmApiPortName, SM_API_PORT_NAME);

  Status = NtConnectPort (
             phSmApiPort,
             & SmApiPortName,
             & SecurityQos,
             NULL,
             NULL,
             NULL,
             & ConnectData,
             & ConnectDataLength
             );
  if (NT_SUCCESS(Status))
  {
    return STATUS_SUCCESS;
  }
  DPRINT("SMLIB: %s failed (Status=0x%08lx)\n", __FUNCTION__, Status);
  return Status;
}

/* EOF */
