/* $Id: dlerror.c,v 1.4 2002/10/29 04:45:28 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/dlfcn/dlerror.c
 * PURPOSE:     Gain access to an executable object file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <dlfcn.h>
#include <string.h>
#include <psx/debug.h>
#include <psx/dlfcn.h>
#include <psx/errno.h>

static int __dl_last_error = 0;

void __dl_set_last_error(int err)
{
 FIXME("dlfcn error handling not thread safe");
 __dl_last_error = err;
}

char *dlerror(void)
{
  return strerror(__dl_last_error);
}

/* EOF */

