/* $Id: capture.c,v 1.1 2001/06/11 20:36:44 ea Exp $
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

BOOLEAN STDCALL CsrFreeCaptureBuffer (PVOID CaptureBuffer)
{
    return RtlFreeHeap (hCaptureHeap, 0, CaptureBuffer);
}

/* EOF */
