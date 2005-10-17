/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/srv.c
 * PURPOSE:         Get CSR.EXE PID
 *
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

DWORD ProcessId = 0; // TODO: set it on startup

DWORD STDCALL CsrGetProcessId (VOID)
{
	return ProcessId;
}

/* EOF */
