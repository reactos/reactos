/* $Id: read.c,v 1.4 2002/10/29 04:45:48 rex Exp $
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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <psx/debug.h>
#include <psx/errno.h>

ssize_t __read(int fildes, void *buf, size_t nbyte, off_t *offset)
{
 HANDLE          hFile;
 NTSTATUS        nErrCode;
 IO_STATUS_BLOCK isbStatus;

 /* get the file handle for the specified file descriptor */
 if(fcntl(fildes, F_GETFH, &hFile) == -1)
  return (-1);

 if(offset != NULL)
 {
  /* NT always moves the file pointer, while Unix pread() must not: we have to
     duplicate the handle and work on the duplicate */ /* FIXME? better save
     the file position and restore it later? */
  HANDLE hDupFile;

  /* duplicate the handle */
  nErrCode = NtDuplicateObject
  (
   NtCurrentProcess(),
   hFile,
   NtCurrentProcess(),
   &hDupFile,
   0,
   0,
   DUPLICATE_SAME_ACCESS
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode))
  {
   errno = __status_to_errno(nErrCode);
   return (0);
  }

  /* read data from file at the specified offset */
  nErrCode = NtReadFile
  (
   hDupFile,
   NULL,
   NULL,
   NULL,
   &isbStatus,
   buf,
   nbyte,
   (PLARGE_INTEGER)offset,
   NULL
  );

  NtClose(hDupFile);

  /* failure */
  if(!NT_SUCCESS(nErrCode))
  {
   errno = __status_to_errno(nErrCode);
   return (0);
  }
 }
 else
 {
  /* read data from file at the current offset */
  nErrCode = NtReadFile
  (
   hFile,
   NULL,
   NULL,
   NULL,
   &isbStatus,
   buf,
   nbyte,
   NULL,
   NULL
  );

  /* failure */
  if(!NT_SUCCESS(nErrCode))
  {
   errno = __status_to_errno(nErrCode);
   return (0);
  }
 }

 return ((ssize_t)isbStatus.Information);
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
 return (__read(fildes, buf, nbyte, NULL));
}

ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
 return (__read(fildes, buf, nbyte, &offset));
}

/* EOF */

