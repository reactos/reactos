/* $Id: dlopen.c,v 1.1 2002/02/20 07:06:50 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/dlfcn/dlopen.c
 * PURPOSE:     Gain access to an executable object file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntdll/ldr.h>
#include <wchar.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <psx/dlfcn.h>
#include <psx/errno.h>
#include <psx/debug.h>

void *__wdlopen(const wchar_t *, int);

void *dlopen(const char *file, int mode)
{
 /* TODO: ANSI-to-Unicode stubs like this need to be standardized in some way */
 void *         pRetVal;
 ANSI_STRING    strFile;
 UNICODE_STRING wstrFile;

 RtlInitAnsiString(&strFile, (LPSTR)file);
 RtlAnsiStringToUnicodeString (&wstrFile, &strFile, TRUE);

 pRetVal = __wdlopen((wchar_t *)wstrFile.Buffer, mode);

 free(wstrFile.Buffer);

 return pRetVal;

}

void *__wdlopen(const wchar_t *file, int mode)
{
 NTSTATUS       nErrCode;
 UNICODE_STRING wstrNativePath;
 struct __dlobj * pdloObject;

 if(file == 0)
 {
  /* POSIX dynamic linking allows for global symbol matching */
  pdloObject = (struct __dlobj *)malloc(sizeof(struct __dlobj));

  TODO("\
move the global symbol matching semantics into the PE Loader \
(NTDLL Ldr family of calls), and just return a pointer to the module \
image\
");

  pdloObject->global = 1;
  return (pdloObject);
 }

 FIXME("\
LdrLoadDll() only accepts DOS paths - probably because the POSIX \
standard didn't specify dynamic linking interfaces at the time Windows NT \
was born \
");

#if 0
 /* TODO: LdrLoadNtDll() or LdrLoadDllEx() (the former is preferrable, since
    the latter is more likely to be implemented with different purposes from
    Microsoft), accepting native NT paths */

 if(wcschr(file, '/'L) != NULL)
 {
  /* TODO: RtlPosixPathNameToNtPathName_U */
  if(!RtlPosixPathNameToNtPathName_U((LPWSTR)file, &wstrNativePath, NULL, NULL))
  {
   __dl_set_last_error(ENOENT);
   return (NULL);
  }
 }
 else
 {
  RtlInitUnicodeString(&wstrNativePath, (LPWSTR)file);
 }
#endif

 RtlInitUnicodeString(&wstrNativePath, (LPWSTR)file);

 pdloObject = (struct __dlobj *)malloc(sizeof(struct __dlobj));
 pdloObject->global = 0;

 WARN("\
mode flags are not fully supported by Windows NT, due to the \
completely different semantics of dynamical linking (for \
example the global symbol matching is unsupported). This \
implementation will then behave as if mode was set to \
(RTLD_NOW | RTLD_GLOBAL), and fail if other flags are set.\
");

 if(__dl_get_scope_flag(mode) == RTLD_LOCAL)
 {
  __dl_set_last_error(EINVAL);
  return (NULL);
 }

 /* load the DLL */
 nErrCode = LdrLoadDll
 (
  NULL,
  0,
  &wstrNativePath,
  (PVOID*)&pdloObject->handle
 );

 if(!NT_SUCCESS(nErrCode))
 {
  __dl_set_last_error(__status_to_errno(nErrCode));
  return (NULL);
 }

 return (pdloObject);

}

/* EOF */

