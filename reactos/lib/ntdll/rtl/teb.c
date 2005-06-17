/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/teb.c
 * PURPOSE:
 */

#define NDEBUG
#include <ntdll.h>


PTEB STDCALL
_NtCurrentTeb() { return NtCurrentTeb(); }



/* EOF */
