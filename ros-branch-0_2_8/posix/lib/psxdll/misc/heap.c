/* $Id: heap.c,v 1.3 2002/10/29 04:45:33 rex Exp $
 *
 * FILE: reactos/subsys/psx/lib/psxdll/misc/heap.c
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * PURPOSE:     Support routines for crt0.c
 * UPDATE HISTORY:
 *               2001-05-06
 */
#define NTOS_MODE_USER
#include <ntos.h>
#include <napi/teb.h>
HANDLE STDCALL GetProcessHeap (VOID)
{
	return (HANDLE)NtCurrentPeb()->ProcessHeap;
}
/* EOF */
