/* $Id: dlsym.c,v 1.4 2002/10/29 04:45:30 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/dlfcn/dlsym.c
 * PURPOSE:     Obtain the address of a symbol from a dlopen() object
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <dlfcn.h>
#include <psx/dlfcn.h>
#include <psx/errno.h>
#include <psx/debug.h>

void *__dlsymn(void *, unsigned long int);
void *__dlsym(void *, int, const char *, unsigned long int);

void *dlsym(void *handle, const char *name)
{
 return (__dlsym(handle, 1, name, 0));
}

void *__dlsymn(void *handle, unsigned long int ordinal)
{
 return (__dlsym(handle, 0, 0, ordinal));
}

void *__dlsym(void *handle, int by_name, const char *name, unsigned long int ordinal)
{
 struct __dlobj * pdloObject;

 void *   pProcAddr;
 NTSTATUS nErrCode;

 if(handle == RTLD_NEXT)
 {
  FIXME("implement RTLD_NEXT semantics");
  return (NULL);
 }

 pdloObject = (struct __dlobj *) handle;

 if(pdloObject->global)
 {
  FIXME("implement global symbol matching");
  return (NULL);
 }

 if(by_name)
 {
  ANSI_STRING strName;

  RtlInitAnsiString(&strName, (LPSTR)name);

  nErrCode = LdrGetProcedureAddress
  (
   pdloObject->handle,
   &strName,
   0,
   (PVOID *)&pProcAddr
  );

 }
 else
 {
  nErrCode = LdrGetProcedureAddress
  (
   pdloObject->handle,
   NULL,
   ordinal,
   (PVOID *)&pProcAddr
  );
 }

 if(!NT_SUCCESS(nErrCode))
 {
  __dl_set_last_error(__status_to_errno(nErrCode));
  return (NULL);
 }

 return pProcAddr;

}

/* EOF */

