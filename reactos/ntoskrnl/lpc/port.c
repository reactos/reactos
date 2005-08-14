/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/port.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

POBJECT_TYPE	LpcPortObjectType = 0;
ULONG		LpcpNextMessageId = 0; /* 0 is not a valid ID */
FAST_MUTEX	LpcpLock; /* global internal sync in LPC facility */

static GENERIC_MAPPING LpcpPortMapping = 
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    0,
    PORT_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/


NTSTATUS INIT_FUNCTION
LpcpInitSystem (VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT("Creating Port Object Type\n");
  
    /* Create the Port Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Port");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EPORT);
    ObjectTypeInitializer.GenericMapping = LpcpPortMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = LpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = LpcpDeletePort;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &LpcPortObjectType);
    
    LpcpNextMessageId = 0;
    ExInitializeFastMutex (& LpcpLock);

    return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	NiInitializePort/3
 *
 * DESCRIPTION
 *	Initialize the EPORT object attributes. The Port
 *	object enters the inactive state.
 *
 * ARGUMENTS
 *	Port	Pointer to an EPORT object to initialize.
 *	Type	connect (RQST), or communication port (COMM)
 *	Parent	OPTIONAL connect port a communication port
 *		is created from
 *
 * RETURN VALUE
 *	STATUS_SUCCESS if initialization succedeed. An error code
 *	otherwise.
 */
NTSTATUS STDCALL
LpcpInitializePort (IN OUT  PEPORT Port,
		  IN      USHORT Type,
		  IN      PEPORT Parent OPTIONAL)
{
  if ((Type != EPORT_TYPE_SERVER_RQST_PORT) &&
      (Type != EPORT_TYPE_SERVER_COMM_PORT) &&
      (Type != EPORT_TYPE_CLIENT_COMM_PORT))
  {
	  return STATUS_INVALID_PARAMETER_2;
  }
  memset (Port, 0, sizeof(EPORT));
  KeInitializeSpinLock (& Port->Lock);
  KeInitializeSemaphore( &Port->Semaphore, 0, MAXLONG );
  Port->RequestPort = Parent;
  Port->OtherPort = NULL;
  Port->QueueLength = 0;
  Port->ConnectQueueLength = 0;
  Port->Type = Type;
  Port->State = EPORT_INACTIVE;
  InitializeListHead (& Port->QueueListHead);
  InitializeListHead (& Port->ConnectQueueListHead);

  return (STATUS_SUCCESS);
}


/* MISCELLANEA SYSTEM SERVICES */


/**********************************************************************
 * NAME							SYSTEM
 *	NtImpersonateClientOfPort/2
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle,
 *	ClientMessage
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
NtImpersonateClientOfPort (HANDLE		PortHandle,
			   PLPC_MESSAGE	ClientMessage)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/* EOF */
