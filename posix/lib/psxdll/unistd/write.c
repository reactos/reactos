/* $Id: write.c,v 1.3 2002/03/21 22:46:30 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/unistd/write.c
 * PURPOSE:     Write on a file
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              15/02/2002: Created
 *              21/03/2002: Implemented write() (KJK::Hyperion <noog@libero.it>)
 */

#include <ddk/ntddk.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <psx/debug.h>
#include <psx/errno.h>
#include <psx/fdtable.h>

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
 __fildes_t      fdDescriptor;
 NTSTATUS        nErrCode;
 IO_STATUS_BLOCK isbStatus;

 if(nbyte == 0)
  return (0);

 if(fcntl(fildes, F_GETALL, &fdDescriptor) == -1)
  return (0);

 if((fdDescriptor.OpenFlags && O_APPEND) == O_APPEND)
 {
  TODO("move file pointer to the end");
 }

 nErrCode = NtWriteFile
 (
  fdDescriptor.FileHandle,
  NULL,
  NULL,
  NULL,
  &isbStatus,
  (PVOID)buf,
  nbyte,
  NULL,
  NULL
 );

 if(!NT_SUCCESS(nErrCode))
 {
  errno = __status_to_errno(nErrCode);
  return (0);
 }

 return (isbStatus.Information);
}

/* EOF */

