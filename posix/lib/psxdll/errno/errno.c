/* $Id: errno.c,v 1.5 2002/10/29 04:45:31 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/errno/errno.c
 * PURPOSE:     Internal errno implementation
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <psx/pdata.h>
#include <psx/errno.h>
#include <psx/debug.h>

int * __PdxGetThreadErrNum(void)
{
 return &(((__PPDX_TDATA) (NtCurrentTeb()->TlsSlots[__PdxGetProcessData()->TlsIndex]) )->ErrNum);
}

/* EOF */

