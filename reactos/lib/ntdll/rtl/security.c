/* $Id: security.c,v 1.2 1999/12/27 15:06:00 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl security functions
 * FILE:              lib/ntdll/rtl/security.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    22/07/99: Added RtlLengthSecurityDescriptor stub
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

ULONG
STDCALL
RtlLengthSecurityDescriptor (
	PSECURITY_DESCRIPTOR	SecurityDescriptor
	)
{
   UNIMPLEMENTED;
}

