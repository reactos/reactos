/* $Id:
*/
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/unistd/close.c
 * PURPOSE:     Close a file descriptor
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              13/02/2002: Created
 */

#include <ddk/ntddk.h>
#include <unistd.h>
#include <fcntl.h>
#include <psx/errno.h>
#include <psx/stdlib.h>
#include <psx/fdtable.h>

int close(int fildes)
{
 __fildes_t fdDescriptor;
 NTSTATUS   nErrCode;

 if(fcntl(fildes, F_DELFD, &fdDescriptor) == -1)
  return (-1);

 __free(fdDescriptor.ExtraData);

 nErrCode = NtClose(fdDescriptor.FileHandle);

 if(!NT_SUCCESS(nErrCode))
 {
  errno = __status_to_errno(nErrCode);
  return (-1);
 }

 return (0);
}

/* EOF */

