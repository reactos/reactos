/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/create.c
 * PURPOSE:         Communication mechanism
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/**********************************************************************
 * NAME
 * 	LpcpVerifyCreateParameters/5
 *
 * DESCRIPTION
 *	Verify user parameters in NtCreatePort and in
 *	NtCreateWaitablePort.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
STATIC NTSTATUS STDCALL 
LpcpVerifyCreateParameters (IN	PHANDLE			PortHandle,
			    IN	POBJECT_ATTRIBUTES	ObjectAttributes,
			    IN	ULONG			MaxConnectInfoLength,
			    IN	ULONG			MaxDataLength,
			    IN	ULONG			MaxPoolUsage)
{
  if (NULL == PortHandle)
    {
      return (STATUS_INVALID_PARAMETER_1);
    }
  if (NULL == ObjectAttributes)
    {
      return (STATUS_INVALID_PARAMETER_2);
    }
  if ((ObjectAttributes->Attributes    & OBJ_OPENLINK)
      || (ObjectAttributes->Attributes & OBJ_OPENIF)
      || (ObjectAttributes->Attributes & OBJ_EXCLUSIVE)
      || (ObjectAttributes->Attributes & OBJ_PERMANENT)
      || (ObjectAttributes->Attributes & OBJ_INHERIT))
  {
    return (STATUS_INVALID_PORT_ATTRIBUTES);
  }
  if (MaxConnectInfoLength > PORT_MAX_DATA_LENGTH)
    {
      return (STATUS_INVALID_PARAMETER_3);
    }
  if (MaxDataLength > PORT_MAX_MESSAGE_LENGTH)
    {
      return (STATUS_INVALID_PARAMETER_4);
    }
  /* TODO: some checking is done also on MaxPoolUsage
   * to avoid choking the executive */
  return (STATUS_SUCCESS);
}


/**********************************************************************
 * NAME
 * 	NiCreatePort/4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
NiCreatePort (PVOID			ObjectBody,
	      PVOID			Parent,
	      PWSTR			RemainingPath,
	      POBJECT_ATTRIBUTES	ObjectAttributes)
{
  if (RemainingPath == NULL)
    {
      return (STATUS_SUCCESS);
    }
  
  if (wcschr(RemainingPath+1, '\\') != NULL)
    {
      return (STATUS_UNSUCCESSFUL);
    }
  
  return (STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	NtCreatePort/5
 * 	
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle,
 *	ObjectAttributes,
 *	MaxConnectInfoLength,
 *	MaxDataLength,
 *	MaxPoolUsage: size of NP zone the NP part of msgs is kept in
 * 
 * RETURN VALUE
 */
/*EXPORTED*/ NTSTATUS STDCALL 
NtCreatePort (PHANDLE		      PortHandle,
	      POBJECT_ATTRIBUTES    ObjectAttributes,
	      ULONG	       MaxConnectInfoLength,
	      ULONG			MaxDataLength,
	      ULONG			MaxPoolUsage)
{
  PEPORT		Port;
  NTSTATUS	Status;
  
  DPRINT("NtCreatePort() Name %x\n", ObjectAttributes->ObjectName->Buffer);
  
  /* Verify parameters */
  Status = LpcpVerifyCreateParameters (PortHandle,
				       ObjectAttributes,
				       MaxConnectInfoLength,
				       MaxDataLength,
				       MaxPoolUsage);
  if (STATUS_SUCCESS != Status)
    {
      return (Status);
    }

  /* Ask Ob to create the object */
  Status = ObCreateObject (ExGetPreviousMode(),
			   LpcPortObjectType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(EPORT),
			   0,
			   0,
			   (PVOID*)&Port);
  if (!NT_SUCCESS(Status))
    {
      return (Status);
    }

  Status = ObInsertObject ((PVOID)Port,
			   NULL,
			   PORT_ALL_ACCESS,
			   0,
			   NULL,
			   PortHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Port);
      return (Status);
    }

  Status = LpcpInitializePort (Port, EPORT_TYPE_SERVER_RQST_PORT, NULL);
  Port->MaxConnectInfoLength = PORT_MAX_DATA_LENGTH;
  Port->MaxDataLength = PORT_MAX_MESSAGE_LENGTH;
  Port->MaxPoolUsage = MaxPoolUsage;
  
  ObDereferenceObject (Port);
  
  return (Status);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	NtCreateWaitablePort/5
 * 	
 * DESCRIPTION
 *	Waitable ports can be connected to with NtSecureConnectPort.
 *	No port interface can be used with waitable ports but
 *	NtReplyWaitReceivePort and NtReplyWaitReceivePortEx.
 * 	Present only in w2k+.
 *
 * ARGUMENTS
 *	PortHandle,
 *	ObjectAttributes,
 *	MaxConnectInfoLength,
 *	MaxDataLength,
 *	MaxPoolUsage
 * 
 * RETURN VALUE
 */
/*EXPORTED*/ NTSTATUS STDCALL
NtCreateWaitablePort (OUT	PHANDLE			PortHandle,
		      IN	POBJECT_ATTRIBUTES	ObjectAttributes,
		      IN	ULONG			MaxConnectInfoLength,
		      IN	ULONG			MaxDataLength,
		      IN	ULONG			MaxPoolUsage)
{
  NTSTATUS Status;
  
  /* Verify parameters */
  Status = LpcpVerifyCreateParameters (PortHandle,
				       ObjectAttributes,
				       MaxConnectInfoLength,
				       MaxDataLength,
				       MaxPoolUsage);
  if (STATUS_SUCCESS != Status)
    {
      return (Status);
    }
  /* TODO */
  return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
