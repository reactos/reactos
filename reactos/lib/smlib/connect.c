/* $Id: connect.c 14015 2005-03-13 17:00:19Z ea $
 *
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       reactos/lib/smlib/connect.c
 * PURPOSE:    Connect to the API LPC port exposed by the SM
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <sm/api.h>
#include <sm/helper.h>
#include <pe.h>

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
 *	hSbApiPort: LPC port handle (checked, but not used);
 *	dwSubsystem: a valid IMAGE_SUBSYSTEM_xxx value;
 *	phSmApiPort: a pointer to a HANDLE, which will be
 *		filled with a valid client-side LPC comm port.
 *	
 * RETURN VALUE
 * 	If all three optional values are omitted, an LPC status.
 * 	STATUS_INVALID_PARAMETER_MIX if PortName is defined and
 * 	both hSbApiPort and dwSubsystem are 0.
 */
NTSTATUS STDCALL
SmConnectApiPort (IN      PUNICODE_STRING  pSbApiPortName  OPTIONAL,
		  IN      HANDLE           hSbApiPort      OPTIONAL,
		  IN      DWORD            dwSubsystem     OPTIONAL,
		  IN OUT  PHANDLE          phSmApiPort)
{
  UNICODE_STRING              SmApiPortName;
  SECURITY_QUALITY_OF_SERVICE SecurityQos;
  NTSTATUS                    Status = STATUS_SUCCESS;
  SM_CONNECT_DATA             ConnectData = {0,{0}};
  ULONG                       ConnectDataLength = 0;

  DPRINT("SMLIB: %s called\n", __FUNCTION__);

  if (pSbApiPortName)
  {
    if (pSbApiPortName->Length > (sizeof pSbApiPortName->Buffer[0] * SM_SB_NAME_MAX_LENGTH))
    {
	  return STATUS_INVALID_PARAMETER_1;
    }
    if (NULL == hSbApiPort || IMAGE_SUBSYSTEM_UNKNOWN == dwSubsystem)
    {
      return STATUS_INVALID_PARAMETER_MIX;
    }
    RtlZeroMemory (& ConnectData, sizeof ConnectData);
    ConnectData.Subsystem = dwSubsystem;
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
  SecurityQos.ContextTrackingMode = TRUE;
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
