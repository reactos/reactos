/* $Id: access.c,v 1.3 2002/10/29 04:45:46 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/access.c
 * PURPOSE:     Determine accessibility of a file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              13/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <unistd.h>
#include <fcntl.h>
#include <psx/errno.h>

int access(const char *path, int amode)
{
 OBJECT_ATTRIBUTES oaFileAttribs;
 IO_STATUS_BLOCK isbStatus;
 ACCESS_MASK amDesiredAccess = 0;
 NTSTATUS nErrCode;
 HANDLE hFile;
 
 if(amode != F_OK)
 {
  if(amode && R_OK) amDesiredAccess |= GENERIC_READ;
  if(amode && W_OK) amDesiredAccess |= GENERIC_WRITE;
  if(amode && X_OK) amDesiredAccess |= GENERIC_EXECUTE;
 }
 
 nErrCode = NtCreateFile
 (
  &hFile,
  amDesiredAccess,
  &oaFileAttribs,
  &isbStatus,
  0,
  0,
  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
  FILE_OPEN,
  0,
  0,
  0
 );
 
 if(NT_SUCCESS(nErrCode))
 {
  NtClose(hFile);
  return (0);
 }
 
 errno = __status_to_errno(nErrCode);
 return (-1);
}

/* EOF */

