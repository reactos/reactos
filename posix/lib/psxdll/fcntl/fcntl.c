/* $Id: fcntl.c,v 1.4 2002/03/21 22:41:53 hyperion Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/fcntl/fcntl.c
 * PURPOSE:     File control
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              13/02/2002: Created
 *              15/02/2002: Implemented fcntl() (KJK::Hyperion)
 */

#include <ddk/ntddk.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <psx/errno.h>
#include <psx/stdlib.h>
#include <psx/fdtable.h>
#include <psx/pdata.h>
#include <psx/debug.h>

int fcntl(int fildes, int cmd, ...)
{
 __fdtable_t *pftFdTable;
 __fildes_t  *pfdDescriptor;
 NTSTATUS     nErrCode;
 int          nRetVal;
 int          nThirdArg;
 void        *pThirdArg;
 va_list      vlArgs;

 /* lock the environment */
 __PdxAcquirePdataLock();

 /* get the file descriptors table */
 pftFdTable = &__PdxGetProcessData()->FdTable;

 /* fildes is an invalid, closed or uninitialized descriptor */
 if
 (
  fildes < 0 ||
  fildes >= OPEN_MAX ||
  __fdtable_entry_isavail(pftFdTable, fildes) == 0 ||
  __fdtable_entry_get(pftFdTable, fildes) == 0
 )
 {
  errno = EBADF;
  __PdxReleasePdataLock();
  return (-1);
 }

 /* get the file descriptor referenced by fildes */
 pfdDescriptor = __fdtable_entry_get(pftFdTable, fildes);
 
 /* get third argument as integer */
 va_start(vlArgs, cmd);
 nThirdArg = va_arg(vlArgs, int);
 va_end(vlArgs);

 /* get third argument as pointer */
 va_start(vlArgs, cmd);
 pThirdArg = va_arg(vlArgs, void *);
 va_end(vlArgs);

 /* initialize return value */
 nRetVal = -1;

 switch(cmd)
 {
  case F_DUPFD:
  {
   int         nDupFileNo;
   __fildes_t *pfdDupDescriptor;

   /* allocate the duplicated descriptor */
   nDupFileNo = __fdtable_entry_add(pftFdTable, nThirdArg, 0, &pfdDupDescriptor);

   if(nDupFileNo)
    break;

   /* copy the open flags */
   pfdDupDescriptor->OpenFlags = pfdDescriptor->OpenFlags;

   /* clear the FD_CLOEXEC flag */
   pfdDupDescriptor->FdFlags = pfdDescriptor->FdFlags & ~FD_CLOEXEC;

   /* duplicate the extra data */
   if(pfdDescriptor->ExtraDataSize != 0 && pfdDescriptor->ExtraData != 0)
   {
    /* allocate space for the duplicated extra data */
    pfdDupDescriptor->ExtraDataSize = pfdDescriptor->ExtraDataSize;
    pfdDupDescriptor->ExtraData = __malloc(pfdDupDescriptor->ExtraDataSize);

    /* failure */
    if(pfdDupDescriptor->ExtraData == 0)
    {
     errno = ENOMEM;
     break;
    }

    /* copy the extra data */
    memcpy(pfdDupDescriptor->ExtraData, pfdDescriptor->ExtraData, pfdDupDescriptor->ExtraDataSize);
    INFO
    (
     "copied %u bytes from 0x%08X into 0x%08X",
     pfdDupDescriptor->ExtraDataSize,
     pfdDescriptor->ExtraData,
     pfdDupDescriptor->ExtraData
    );
   }

   /* duplicate the handle */
   nErrCode = NtDuplicateObject
   (
    NtCurrentProcess(),
    pfdDescriptor->FileHandle,
    NtCurrentProcess(),
    &pfdDupDescriptor->FileHandle,
    0,
    0,
    DUPLICATE_SAME_ACCESS /* | DUPLICATE_SAME_ATTRIBUTES */
   );

   /* failure */
   if(!NT_SUCCESS(nErrCode))
   {
    __free(pfdDupDescriptor->ExtraData);
    errno = __status_to_errno(nErrCode);
    break;
   }

   INFO
   (
    "duplicated handle 0x%08X into handle 0x%08X",
    pfdDescriptor->FileHandle,
    pfdDupDescriptor->FileHandle
   );

   /* return the duplicated file number */
   nRetVal = nDupFileNo;
   break;
  }

  case F_GETFD:
  {
   nRetVal = pfdDescriptor->FdFlags;
   break;
  }

  case F_SETFD:
  {
   pfdDescriptor->FdFlags = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETFL:
  {
   nRetVal = pfdDescriptor->OpenFlags;
   break;
  }

  case F_SETFL:
  {
   pfdDescriptor->OpenFlags = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETLK:
  {
   errno = EINVAL;
   break;
  }

  case F_SETLK:
  {
   errno = EINVAL;
   break;
  }

  case F_SETLKW:
  {
   errno = EINVAL;
   break;
  }

  case F_NEWFD:
  {
   /* allocate a new descriptor */
   nRetVal = __fdtable_entry_add(pftFdTable, fildes, (__fildes_t *)pThirdArg, 0);
   break;
  }

  case F_DELFD:
  {
   /* invalid return pointer */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   memcpy((__fildes_t *)pThirdArg, pfdDescriptor, sizeof(*pfdDescriptor));

   /* remove file descriptor */
   nRetVal = __fdtable_entry_remove(pftFdTable, fildes);

  }

  case F_GETALL:
  {
   /* invalid return pointer */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* return a copy of the file descriptor */
   memcpy((__fildes_t *)pThirdArg, pfdDescriptor, sizeof(*pfdDescriptor));
   nRetVal = 0;
  }

  case F_SETALL:
  {
   /* invalid file descriptor to copy attributes from */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* copy the attributes of file descriptor from the provided descriptor */
   memcpy(pfdDescriptor, pThirdArg, sizeof(*pfdDescriptor));
   nRetVal = 0;
  }

  case F_GETXP:
  {
   /* invalid return pointer */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* return a pointer to the extra data associated to the descriptor */
   *((void **)pThirdArg) = pfdDescriptor->ExtraData;
   nRetVal = 0;
   break;
  }

  case F_SETXP:
  {
   /* set the pointer to the extra data associated */
   pfdDescriptor->ExtraData = pThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETXS:
  {
   nRetVal = pfdDescriptor->ExtraDataSize;
   break;
  }

  case F_SETXS:
  {
   pfdDescriptor->ExtraDataSize = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETFH:
  {
   /* invalid return pointer */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* return the handle associated to the descriptor */
   *((void **)pThirdArg) = pfdDescriptor->FileHandle;
   nRetVal = 0;
   break;
  }

  case F_SETFH:
  {
   pfdDescriptor->FileHandle = pThirdArg;
   nRetVal = 0;
   break;
  }

  default:
   errno = EINVAL;
 }

 /* unlock the environment */
 __PdxReleasePdataLock();

 return (nRetVal);
}

/* EOF */