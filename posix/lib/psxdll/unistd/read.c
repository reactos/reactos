/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/read.c
 * PURPOSE:     Read from a file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <unistd.h>
#include <sys/types.h>

ssize_t read(int fildes, void *buf, size_t nbyte)
{
}

ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
 HANDLE          hFile;
 NTSTATUS        nErrCode;
 IO_STATUS_BLOCK isbStatus;

 /* get the file handle for the specified file descriptor */
 if(fcntl(fildes, F_GETFH, &hFile) == -1)
  return (-1);

 /* read data from file */
 nErrCode = NtReadFile
 (
  hFile,
  NULL,
  NULL,
  NULL,
  &isbStatus,
  buf,
  nbyte,
  (PLARGE_INTEGER)&off_t,
  NULL
 );

 return ((ssize_t)isbStatus.Information);
}

/* EOF */

