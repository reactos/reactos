/* $Id: capture.c,v 1.2 2001/06/17 09:24:04 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/capture.c
 * PURPOSE:         CSRSS Capure API
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <string.h>

#include <csrss/csrss.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

static HANDLE hCaptureHeap = INVALID_HANDLE_VALUE;

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
CsrAllocateCapturePointer(ULONG Unknown0,
			  ULONG Unknown1,
			  ULONG Unknown2)
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
