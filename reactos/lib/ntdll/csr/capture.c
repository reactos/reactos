/* $Id: capture.c,v 1.4 2002/09/07 15:12:38 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/capture.c
 * PURPOSE:         CSRSS Capture API
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>
#include <string.h>
#include <csrss/csrss.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static HANDLE hCaptureHeap = INVALID_HANDLE_VALUE; /* FIXME: use the general NTDLL heap */

/* FUNCTIONS *****************************************************************/

PVOID
STDCALL CsrAllocateCaptureBuffer (
		DWORD	Unknown0,
		DWORD	Unknown1,
		DWORD	Unknown2
		)
{
	/* FIXME: implement it! */
	return NULL;	
}

VOID STDCALL
CsrCaptureMessageString (DWORD Unknown0,
			 DWORD Unknown1,
			 DWORD Unknown2,
			 DWORD Unknown3,
			 DWORD Unknown4)
{
}

VOID STDCALL
CsrAllocateCapturePointer(ULONG Unknown0,
			  ULONG Unknown1,
			  ULONG Unknown2)
{

}

VOID STDCALL CsrAllocateMessagePointer (DWORD Unknown0,
					DWORD Unknown1,
					DWORD Unknown2)
{
}

VOID STDCALL
CsrCaptureMessageBuffer(ULONG Unknown0,
			ULONG Unknown1,
			ULONG Unknown2,
			ULONG Unknown3)
{

}

BOOLEAN STDCALL CsrFreeCaptureBuffer (PVOID CaptureBuffer)
{
    /* FIXME: use NTDLL own heap */
    return RtlFreeHeap (hCaptureHeap, 0, CaptureBuffer);
}

PLARGE_INTEGER STDCALL
CsrCaptureTimeout(LONG Milliseconds,
		  PLARGE_INTEGER Timeout)
{
   if (Milliseconds == -1)
     return NULL;
   
   Timeout->QuadPart = Milliseconds * -100000;
   return Timeout;
}

/* EOF */
