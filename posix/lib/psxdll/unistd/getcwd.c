/* $Id: getcwd.c,v 1.2 2002/02/20 09:17:58 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/getcwd.c
 * PURPOSE:     Get the pathname of the current working directory
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              01/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <unistd.h>
#include <stddef.h>
#include <wchar.h>
#include <psx/errno.h>
#include <psx/stdlib.h>
#include <psx/pdata.h>

wchar_t *_Wgetcwd(wchar_t *buf, size_t size)
{
 PUNICODE_STRING pwstrCurDir;

 __PdxAcquirePdataLock();

 pwstrCurDir = __PdxGetCurDir();

 if(size < (pwstrCurDir->Length / sizeof(WCHAR)))
 {
  __PdxReleasePdataLock();
  errno = ERANGE;
  return (0);
 }
 else
 {
  wcsncpy(buf, pwstrCurDir->Buffer, pwstrCurDir->Length);
  __PdxReleasePdataLock();
  return (buf);
 }
}

char *getcwd(char *buf, size_t size)
{
 PUNICODE_STRING pwstrCurDir;

 __PdxAcquirePdataLock();

 pwstrCurDir = __PdxGetCurDir();

 if(size < (pwstrCurDir->Length / sizeof(WCHAR)))
 {
  __PdxReleasePdataLock();
  errno = ERANGE;
  return (0);
 }
 else
 {
  ANSI_STRING strBuffer;
  NTSTATUS    nErrCode;

  strBuffer.Length = 0;
  strBuffer.MaximumLength = size;
  strBuffer.Buffer = buf;

  nErrCode = RtlUnicodeStringToAnsiString
  (
   &strBuffer,
   pwstrCurDir,
   FALSE
  );

  __PdxReleasePdataLock();

  if(!NT_SUCCESS(nErrCode))
  {
   errno = __status_to_errno(nErrCode);
   return (0);
  }

  return (buf);
 }

 __PdxReleasePdataLock();
}

/* EOF */

