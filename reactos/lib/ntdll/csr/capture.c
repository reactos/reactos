/* $Id: capture.c,v 1.6 2003/07/11 13:50:22 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/capture.c
 * PURPOSE:         CSRSS Capture API
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/csr.h>
#include <string.h>

#include <csrss/csrss.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

static HANDLE hCaptureHeap = INVALID_HANDLE_VALUE; /* FIXME: use the general NTDLL heap */

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
VOID STDCALL
CsrCaptureMessageString (DWORD Unknown0,
			 DWORD Unknown1,
			 DWORD Unknown2,
			 DWORD Unknown3,
			 DWORD Unknown4)
{
}

/*
 * @unimplemented
 */
VOID STDCALL
CsrAllocateCapturePointer(ULONG Unknown0,
			  ULONG Unknown1,
			  ULONG Unknown2)
{

}

/*
 * @unimplemented
 */
VOID STDCALL CsrAllocateMessagePointer (DWORD Unknown0,
					DWORD Unknown1,
					DWORD Unknown2)
{
}

/*
 * @unimplemented
 */
VOID STDCALL
CsrCaptureMessageBuffer(ULONG Unknown0,
			ULONG Unknown1,
			ULONG Unknown2,
			ULONG Unknown3)
{

}

/*
 * @unimplemented
 */
BOOLEAN STDCALL CsrFreeCaptureBuffer (PVOID CaptureBuffer)
{
    /* FIXME: use NTDLL own heap */
    return RtlFreeHeap (hCaptureHeap, 0, CaptureBuffer);
}

/*
 * @implemented
 */
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
