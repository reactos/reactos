/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/teb.c
 * PURPOSE:
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>


PTEB STDCALL
_NtCurrentTeb() { return NtCurrentTeb(); }



/* EOF */
