/* $Id: dlclose.c,v 1.4 2002/10/29 04:45:28 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/dlfcn/dlclose.c
 * PURPOSE:     Close a dlopen() object
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <psx/debug.h>
#include <psx/dlfcn.h>
#include <psx/errno.h>

int dlclose(void *handle)
{
 if(handle == 0)
 {
  ERR("invalid handle passed to dlclose");

  __dl_set_last_error(EFAULT); /* FIXME? maybe EINVAL? */
  return (-1);
 }

 if(((struct __dlobj *)handle)->global)
 {
  TODO("global symbol matching not implemented");

  __dl_set_last_error(EINVAL);
  return (-1);
 }
 else
 {
  NTSTATUS nErrCode = LdrUnloadDll(((struct __dlobj *)handle)->handle);

  if(!NT_SUCCESS(nErrCode))
  {
   ERR("LdrUnloadDll(%#x) failed with status %d", ((struct __dlobj *)handle)->handle, nErrCode);

   free(handle);
   __dl_set_last_error(__status_to_errno(nErrCode));
   return (-1);
  }
 }

 free(handle);

 return (0);

}

/* EOF */

