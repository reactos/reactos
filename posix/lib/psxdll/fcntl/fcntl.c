/* $Id: fcntl.c,v 1.7 2002/10/29 04:45:31 rex Exp $
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
 INFO("environment locked");

 /* get the file descriptors table */
 pftFdTable = &__PdxGetProcessData()->FdTable;
 INFO("file descriptors table at 0x%08X", pftFdTable);

 /* fildes is an invalid descriptor, or it's a closed or uninitialized
    descriptor and the requested operation is not the creation of a new
    descriptor */
 if
 (
  fildes < 0 ||
  fildes >= OPEN_MAX ||
  (
   (cmd != F_NEWFD) &&
   (
    __fdtable_entry_isavail(pftFdTable, fildes) == 0 ||
    __fdtable_entry_get(pftFdTable, fildes) == 0
   )
  )
 )
 {
  INFO("invalid file descriptor");
  errno = EBADF;
  __PdxReleasePdataLock();
  return (-1);
 }

 /* get the file descriptor referenced by fildes */
 pfdDescriptor = __fdtable_entry_get(pftFdTable, fildes);
 INFO("file descriptor %d at 0x%08X", fildes, pftFdTable);
 
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
   
   INFO("requested operation: F_DUPFD");

   /* allocate the duplicated descriptor */
   nDupFileNo = __fdtable_entry_add(pftFdTable, nThirdArg, 0, &pfdDupDescriptor);

   if(nDupFileNo)
   {
    ERR("__fdtable_entry_add() failed, errno %d", errno);
    break;
   }

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
   INFO("requested operation: F_GETFD");
   nRetVal = pfdDescriptor->FdFlags;
   break;
  }

  case F_SETFD:
  {
   INFO("requested operation: F_SETFD");
   pfdDescriptor->FdFlags = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETFL:
  {
   INFO("requested operation: F_GETFL");
   nRetVal = pfdDescriptor->OpenFlags;
   break;
  }

  case F_SETFL:
  {
   INFO("requested operation: F_SETFL");
   pfdDescriptor->OpenFlags = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETLK:
  {
   INFO("requested operation: F_GETLK");
   errno = EINVAL;
   break;
  }

  case F_SETLK:
  {
   INFO("requested operation: F_SETLK");
   errno = EINVAL;
   break;
  }

  case F_SETLKW:
  {
   INFO("requested operation: F_SETLKW");
   errno = EINVAL;
   break;
  }

  case F_NEWFD:
  {
   INFO("requested operation: F_NEWFD");
   /* allocate a new descriptor */
   nRetVal = __fdtable_entry_add(pftFdTable, fildes, (__fildes_t *)pThirdArg, 0);
   break;
  }

  case F_DELFD:
  {
   INFO("requested operation: F_DELFD");
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
   INFO("requested operation: F_GETALL");
   /* invalid return pointer */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* return a copy of the file descriptor */
   memcpy((__fildes_t *)pThirdArg, pfdDescriptor, sizeof(*pfdDescriptor));
   nRetVal = 0;
   
   break;
  }

  case F_SETALL:
  {
   INFO("requested operation: F_SETALL");
   /* invalid file descriptor to copy attributes from */
   if(pThirdArg == 0)
   {
    errno = EINVAL;
    break;
   }

   /* copy the attributes of file descriptor from the provided descriptor */
   memcpy(pfdDescriptor, pThirdArg, sizeof(*pfdDescriptor));
   nRetVal = 0;
   
   break;
  }

  case F_GETXP:
  {
   INFO("requested operation: F_GETXP");
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
   INFO("requested operation: F_SETXP");
   /* set the pointer to the extra data associated */
   pfdDescriptor->ExtraData = pThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETXS:
  {
   INFO("requested operation: F_GETXS");
   nRetVal = pfdDescriptor->ExtraDataSize;
   break;
  }

  case F_SETXS:
  {
   INFO("requested operation: F_SETXS");
   pfdDescriptor->ExtraDataSize = nThirdArg;
   nRetVal = 0;
   break;
  }

  case F_GETFH:
  {
   INFO("requested operation: F_GETFH");
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
   INFO("requested operation: F_SETFH");
   pfdDescriptor->FileHandle = pThirdArg;
   nRetVal = 0;
   break;
  }

  default:
   INFO("invalid operation requested");
   errno = EINVAL;
 }

 /* unlock the environment */
 __PdxReleasePdataLock();
 INFO("environment unlocked");

 return (nRetVal);
}

/* EOF */