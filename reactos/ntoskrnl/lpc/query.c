/* $Id: query.c,v 1.4 2002/09/07 15:12:58 chorns Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/query.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	NtQueryInformationPort@20
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle		[IN]
 *	PortInformationClass	[IN]
 *	PortInformation		[OUT]
 *	PortInformationLength	[IN]
 *	ReturnLength		[OUT]
 *
 * RETURN VALUE
 *	STATUS_SUCCESS if the call succedeed. An error code
 *	otherwise.
 *
 * NOTES
 * 	P. Dabak reports that this system service seems to return
 * 	no information.
 */
NTSTATUS STDCALL
NtQueryInformationPort (IN	HANDLE	PortHandle,
			IN	CINT	PortInformationClass,	
			OUT	PVOID	PortInformation,    
			IN	ULONG	PortInformationLength,
			OUT	PULONG	ReturnLength)
{
  NTSTATUS	Status;
  PEPORT		Port;
  
  Status = ObReferenceObjectByHandle (PortHandle,
				      PORT_ALL_ACCESS,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID *) & Port,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtQueryInformationPort() = %x\n", Status);
      return (Status);
    }
  /*
   * FIXME: NT does nothing here!
   */
  ObDereferenceObject (Port);
  return STATUS_SUCCESS;
}


/* EOF */
