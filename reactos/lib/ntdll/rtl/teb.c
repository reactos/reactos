/* $Id: teb.c,v 1.1 2003/07/06 23:04:19 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/teb.c
 * PURPOSE:         
 */

#include <ddk/ntddk.h>
#include <napi/teb.h>

PTEB STDCALL _NtCurrentTeb() { return NtCurrentTeb(); }

/* EOF */
