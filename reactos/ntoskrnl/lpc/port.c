/* $Id: port.c,v 1.1 2000/06/04 17:27:39 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/port.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *
 *	2000-06-04 (ea)
 *		ntoskrnl/nt/port.c moved in ntoskrnl/lpc/port.c
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

POBJECT_TYPE	ExPortType = NULL;
ULONG		EiNextLpcMessageId = 0;

/* FUNCTIONS *****************************************************************/


NTSTATUS NiInitPort (VOID)
{
   ExPortType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&ExPortType->TypeName,L"Port");
   
   ExPortType->MaxObjects = ULONG_MAX;
   ExPortType->MaxHandles = ULONG_MAX;
   ExPortType->TotalObjects = 0;
   ExPortType->TotalHandles = 0;
   ExPortType->PagedPoolCharge = 0;
   ExPortType->NonpagedPoolCharge = sizeof(EPORT);
   ExPortType->Dump = NULL;
   ExPortType->Open = NULL;
   ExPortType->Close = NiClosePort;
   ExPortType->Delete = NiDeletePort;
   ExPortType->Parse = NULL;
   ExPortType->Security = NULL;
   ExPortType->QueryName = NULL;
   ExPortType->OkayToClose = NULL;
   ExPortType->Create = NiCreatePort;
   
   EiNextLpcMessageId = 0;
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	NiInitializePort
 *	
 * DESCRIPTION
 *	Initialize the EPORT object attributes. The Port
 *	object enters the inactive state.
 *
 * ARGUMENTS
 *	Port	Pointer to an EPORT object to initialize.
 *
 * RETURN VALUE
 *	STATUS_SUCCESS if initialization succedeed. An error code
 *	otherwise.
 */
NTSTATUS
STDCALL
NiInitializePort (
	IN OUT	PEPORT	Port
	)
{
	memset (Port, 0, sizeof(EPORT));
	KeInitializeSpinLock (& Port->Lock);
	KeInitializeEvent (& Port->Event, SynchronizationEvent, FALSE);
	Port->OtherPort = NULL;
	Port->QueueLength = 0;
	Port->ConnectQueueLength = 0;
	Port->State = EPORT_INACTIVE;
	InitializeListHead (& Port->QueueListHead);
	InitializeListHead (& Port->ConnectQueueListHead);
   
	return (STATUS_SUCCESS);
}


/* MISCELLANEA SYSTEM SERVICES */


/**********************************************************************
 * NAME							SYSTEM
 *	NtImpersonateClientOfPort@8
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle,
 *	ClientMessage
 *
 * RETURN VALUE
 * 
 */
NTSTATUS
STDCALL
NtImpersonateClientOfPort (
	HANDLE		PortHandle,
	PLPC_MESSAGE	ClientMessage
	)
{
	UNIMPLEMENTED;
}



/* EOF */
