/* $Id: teb.c,v 1.2 2003/08/07 01:51:42 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/teb.c
 * PURPOSE:         
 */

#include <ddk/ntddk.h>
#include <napi/teb.h>

PTEB STDCALL
_NtCurrentTeb() { return NtCurrentTeb(); }

/* EOF */
