/* $Id: pdata.h,v 1.3 2002/03/07 05:48:35 hyperion Exp $
 */
/*
 * psx/pdata.h
 *
 * POSIX+ subsystem process environment data structure
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __PSX_PDATA_H_INCLUDED__
#define __PSX_PDATA_H_INCLUDED__

/* INCLUDES */
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <limits.h>
#include <psx/fdtable.h>

/* OBJECTS */

/* TYPES */
typedef struct __tagPDX_PDATA
{
 BOOL            Spawned;          /* TRUE if process has been created through __PdxSpawnPosixProcess() */
 int             ArgCount;         /* count of arguments passed to exec() */
 char          **ArgVect;          /* array of arguments passed to exesc() */
 char         ***Environment;      /* pointer to user-provided environ variable */
 UNICODE_STRING  NativePathBuffer; /* static buffer used by low-level calls for pathname conversions */
 UNICODE_STRING  CurDir;           /* current working directory */
 UNICODE_STRING  RootPath;         /* NT path to the process's root directory */
 HANDLE          RootHandle;       /* handle to the process's root directory */
 __fdtable_t     FdTable;          /* file descriptors table */
} __PDX_PDATA, * __PPDX_PDATA;

/* serialized process data block, used by __PdxSpawnPosixProcess() and __PdxExecThunk().
   The layout of buffers inside the Buffer byte array is as following:

   ArgVect[0] + null byte
   ArgVect[1] + null byte
   ...
   ArgVect[ArgCount - 1] + null byte
   Environment[0] + null byte
   Environment[1] + null byte
   ...
   Environment[n - 1] + null byte (NOTE: the value of n is stored in ProcessData.Environment)
   CurDir.Buffer
   RootPath.Buffer
   FdTable.Descriptors[0]
   FdTable.Descriptors[1]
   ...
   FdTable.Descriptors[FdTable.AllocatedDescriptors - 1]
   FdTable.Descriptors[x].ExtraData
   FdTable.Descriptors[y].ExtraData
   ...
   padding for page boundary alignment
 */
typedef struct __tagPDX_SERIALIZED_PDATA
{
 __PDX_PDATA ProcessData;
 ULONG       AllocSize;
 BYTE        Buffer[1];
} __PDX_SERIALIZED_PDATA, *__PPDX_SERIALIZED_PDATA;

/* CONSTANTS */

/* PROTOTYPES */

/* MACROS */
#define __PdxAcquirePdataLock() (RtlAcquirePebLock())
#define __PdxReleasePdataLock() (RtlReleasePebLock())

#define __PdxSetProcessData()(PPDATA) ((void)((NtCurrentPeb()->SubSystemData) = (PPDATA)))
#define __PdxGetProcessData()         ((__PPDX_PDATA)(&(NtCurrentPeb()->SubSystemData)))

#define __PdxGetNativePathBuffer() ((PUNICODE_STRING)(&(__PdxGetProcessData()->NativePathBuffer)))
#define __PdxGetCurDir()           ((PUNICODE_STRING)(&(__PdxGetProcessData()->CurDir)))
#define __PdxGetRootPath()         ((PUNICODE_STRING)(&(__PdxGetProcessData()->RootPath)))
#define __PdxGetRootHandle()       ((HANDLE)(__PdxGetProcessData()->RootHandle))
#define __PdxGetFdTable()          ((__fdtable_t *)(__PdxGetProcessData()->FdTable))

#define __PdxSetNativePathBuffer(BUF)  ((void)((__PdxGetProcessData()->NativePathBuffer) = (BUF)))
#define __PdxSetCurDir(CURDIR)         ((void)((__PdxGetProcessData()->CurDir) = (CURDIR)))
#define __PdxSetRootPath(ROOTPATH)     ((void)((__PdxGetProcessData()->RootPath) = (ROOTPATH)))
#define __PdxSetRootHandle(ROOTHANDLE) ((void)((__PdxGetProcessData()->RootHandle) = (ROOTHANDLE)))
#define __PdxSetFdTable(FDTABLE)       ((void)((__PdxGetProcessData()->FdTable) = (FDTABLE)))

#endif /* __PSX_PDATA_H_INCLUDED__ */

/* EOF */

