/* $Id:
 */
/*
 * psx/pdata.h
 *
 * POSIX subsystem process environment data structure
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
 UNICODE_STRING  NativePathBuffer;
 UNICODE_STRING  CurDir;
 UNICODE_STRING  RootPath;
 HANDLE          RootHandle;
 __fdtable_t    *FdTable;
} __PDX_PDATA, * __PPDX_PDATA;

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

