////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: UDF.h
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   The main include file for the UDF file system driver.
*
*************************************************************************/

#ifndef _UDF_UDF_H_
#define _UDF_UDF_H_

/**************** OPTIONS *****************/

//#define UDF_TRACK_UNICODE_STR

//#define DEMO

//#define UDF_LIMIT_NAME_LEN

//#define UDF_LIMIT_DIR_SIZE

#ifdef UDF_LIMIT_NAME_LEN
  #define UDF_X_NAME_LEN (20)
  #define UDF_X_PATH_LEN (25)
#else //UDF_LIMIT_NAME_LEN
  #define UDF_X_NAME_LEN UDF_NAME_LEN
  #define UDF_X_PATH_LEN UDF_PATH_LEN
#endif //UDF_LIMIT_NAME_LEN

#define IFS_40
//#define PRETEND_NTFS

//#define UDF_ASYNC_IO

//#define UDF_ENABLE_SECURITY

#define UDF_HANDLE_EAS

#define UDF_HDD_SUPPORT

#define UDF_ALLOW_FRAG_AD

#ifndef UDF_LIMIT_DIR_SIZE
    #define UDF_DEFAULT_DIR_PACK_THRESHOLD (128)
#else // UDF_LIMIT_DIR_SIZE
    #define UDF_DEFAULT_DIR_PACK_THRESHOLD (16)
#endif // UDF_LIMIT_DIR_SIZE

#ifdef DEMO
    #define UDF_DEMO_VOLUME_LABEL L"UDF Demo"
#endif //DEMO

#define UDF_DEFAULT_READAHEAD_GRAN 0x10000
#define UDF_DEFAULT_SPARSE_THRESHOLD (256*PACKETSIZE_UDF)

#define ALLOW_SPARSE

#define UDF_PACK_DIRS

#define MOUNT_ERR_THRESHOLD   256

#define UDF_VALID_FILE_ATTRIBUTES \
   (FILE_ATTRIBUTE_READONLY   | \
    FILE_ATTRIBUTE_HIDDEN     | \
    FILE_ATTRIBUTE_SYSTEM     | \
    FILE_ATTRIBUTE_DIRECTORY  | \
    FILE_ATTRIBUTE_ARCHIVE    | \
    /*FILE_ATTRIBUTE_DEVICE   | */ \
    FILE_ATTRIBUTE_NORMAL     | \
    FILE_ATTRIBUTE_TEMPORARY  | \
    FILE_ATTRIBUTE_SPARSE_FILE)

//#define UDF_DISABLE_SYSTEM_CACHE_MANAGER

//#define UDF_CDRW_EMULATION_ON_ROM

#define UDF_DELAYED_CLOSE

#ifdef UDF_DELAYED_CLOSE
#define UDF_FE_ALLOCATION_CHARGE
#endif //UDF_DELAYED_CLOSE

#define UDF_ALLOW_RENAME_MOVE

#define UDF_ALLOW_HARD_LINKS

#ifdef UDF_ALLOW_HARD_LINKS
//#define UDF_ALLOW_LINKS_TO_STREAMS
#endif //UDF_ALLOW_HARD_LINKS

//#define UDF_ALLOW_PRETEND_DELETED

#define UDF_DEFAULT_BM_FLUSH_TIMEOUT 16         // seconds
#define UDF_DEFAULT_TREE_FLUSH_TIMEOUT 5        // seconds

#define UDF_DEFAULT_FSP_THREAD_PER_CPU  (4)
#define UDF_FSP_THREAD_PER_CPU (Vcb->ThreadsPerCpu)
#define FSP_PER_DEVICE_THRESHOLD (UDFGlobalData.CPU_Count*UDF_FSP_THREAD_PER_CPU)

/************* END OF OPTIONS **************/

// some constant definitions
#define UDF_PANIC_IDENTIFIER        (0x86427531)

// Common include files - should be in the include dir of the MS supplied IFS Kit
#ifndef _CONSOLE
extern "C" {
#pragma pack(push, 8)
#include "ntifs.h"
#include "ntifs_ex.h"
#pragma pack(pop)
}
#endif //_CONSOLE

#include <pseh/pseh2.h>

#include "Include/check_env.h"

#define PEXTENDED_IO_STACK_LOCATION  PIO_STACK_LOCATION

#ifndef NDEBUG
#define UDF_DBG
#endif

#define VALIDATE_STRUCTURES
// the following include files should be in the inc sub-dir associated with this driver

#define OS_SUCCESS(a)     NT_SUCCESS(a)
#define OSSTATUS          NTSTATUS

#ifndef _CONSOLE
#include "ntdddisk.h"
#include <devioctl.h>
#include "Include/CrossNt/CrossNt.h"
#endif //_CONSOLE

#include <stddef.h>
#include <string.h>
#include <stdio.h>
//#include "ecma_167.h"
//#include "osta_misc.h"
#include "wcache.h"
#include "CDRW/cdrw_usr.h"

#include "Include/regtools.h"

#ifdef _CONSOLE
#include "udf_info/udf_rel.h"
#include "Include/udf_common.h"
#else
#include "struct.h"
#endif //_CONSOLE

// global variables - minimize these
extern UDFData              UDFGlobalData;

#ifndef _CONSOLE
#include "env_spec.h"
#include "dldetect.h"
#include "udf_dbg.h"
#else
#include "Include/env_spec_w32.h"
#endif //_CONSOLE

#include "sys_spec.h"

#include "udf_info/udf_info.h"

#ifndef _CONSOLE
#include "protos.h"
#endif //_CONSOLE

#include "Include/phys_lib.h"
#include "errmsg.h"
//#include "Include/tools.h"
#include "Include/protect.h"
#include "udfpubl.h"
//#include "ntifs.h"
#include "mem.h"
#include "Include/key_lib.h"

extern CCHAR   DefLetter[];

// try-finally simulation
#define try_return(S)   { S; goto try_exit; }
#define try_return1(S)  { S; goto try_exit1; }
#define try_return2(S)  { S; goto try_exit2; }

// some global (helpful) macros
#define UDFSetFlag(Flag, Value) ((Flag) |= (Value))
#define UDFClearFlag(Flag, Value)   ((Flag) &= ~(Value))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG)(OFFSET) - (ULONG)(BASE)))

#define UDFQuadAlign(Value)         ((((uint32)(Value)) + 7) & 0xfffffff8)

// to perform a bug-check (panic), the following macro is used
#define UDFPanic(arg1, arg2, arg3)                  \
    (KeBugCheckEx(UDF_PANIC_IDENTIFIER, UDF_BUG_CHECK_ID | __LINE__, (uint32)(arg1), (uint32)(arg2), (uint32)(arg3)))
// small check for illegal open mode (desired access) if volume is
// read only (on standard CD-ROM device or another like this)
#define UdfIllegalFcbAccess(Vcb,DesiredAccess) ((   \
    (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) && \
     (FlagOn( (DesiredAccess),                       \
            FILE_WRITE_DATA         |   \
            FILE_ADD_FILE           |   \
            FILE_APPEND_DATA        |   \
            FILE_ADD_SUBDIRECTORY   |   \
            FILE_WRITE_EA           |   \
            FILE_DELETE_CHILD       |   \
            FILE_WRITE_ATTRIBUTES   |   \
            DELETE                  |   \
            WRITE_OWNER             |   \
            WRITE_DAC ))                \
       ) || (                           \
    !(Vcb->WriteSecurity) &&            \
     (FlagOn( (DesiredAccess),          \
            WRITE_OWNER             |   \
            0 /*WRITE_DAC*/ ))                \
))


// 
#if !defined(UDF_DBG) && !defined(PRINT_ALWAYS)

#ifndef _CONSOLE
#define UDFAcquireResourceExclusive(Resource,CanWait)  \
    (ExAcquireResourceExclusiveLite((Resource),(CanWait))) 
#define UDFAcquireResourceShared(Resource,CanWait) \
    (ExAcquireResourceSharedLite((Resource),(CanWait))) 
// a convenient macro (must be invoked in the context of the thread that acquired the resource)
#define UDFReleaseResource(Resource)    \
    (ExReleaseResourceForThreadLite((Resource), ExGetCurrentResourceThread()))
#define UDFDeleteResource(Resource)    \
    (ExDeleteResourceLite((Resource)))
#define UDFConvertExclusiveToSharedLite(Resource) \
    (ExConvertExclusiveToSharedLite((Resource)))
#define UDFInitializeResourceLite(Resource) \
    (ExInitializeResourceLite((Resource)))
#define UDFAcquireSharedStarveExclusive(Resource,CanWait) \
    (ExAcquireSharedStarveExclusive((Resource),(CanWait)))
#define UDFAcquireSharedWaitForExclusive(Resource,CanWait) \
    (ExAcquireSharedWaitForExclusive((Resource),(CanWait)))

#define UDFInterlockedIncrement(addr) \
    (InterlockedIncrement((addr)))
#define UDFInterlockedDecrement(addr) \
    (InterlockedDecrement((addr)))
#define UDFInterlockedExchangeAdd(addr,i) \
    (InterlockedExchangeAdd((addr),(i)))

#endif //_CONSOLE

#define UDF_CHECK_PAGING_IO_RESOURCE(NTReqFCB) 
#define UDF_CHECK_EXVCB_RESOURCE(Vcb) 
#define UDF_CHECK_BITMAP_RESOURCE(Vcb) 


#else //UDF_DBG

#ifndef _CONSOLE
#define UDFAcquireResourceExclusive(Resource,CanWait)  \
    (UDFDebugAcquireResourceExclusiveLite((Resource),(CanWait),UDF_BUG_CHECK_ID,__LINE__))

#define UDFAcquireResourceShared(Resource,CanWait) \
    (UDFDebugAcquireResourceSharedLite((Resource),(CanWait),UDF_BUG_CHECK_ID,__LINE__))
// a convenient macro (must be invoked in the context of the thread that acquired the resource)
#define UDFReleaseResource(Resource)    \
    (UDFDebugReleaseResourceForThreadLite((Resource), ExGetCurrentResourceThread(),UDF_BUG_CHECK_ID,__LINE__))

#define UDFDeleteResource(Resource)    \
    (UDFDebugDeleteResource((Resource), ExGetCurrentResourceThread(),UDF_BUG_CHECK_ID,__LINE__))
#define UDFConvertExclusiveToSharedLite(Resource) \
    (UDFDebugConvertExclusiveToSharedLite((Resource), ExGetCurrentResourceThread(),UDF_BUG_CHECK_ID,__LINE__))
#define UDFInitializeResourceLite(Resource) \
    (UDFDebugInitializeResourceLite((Resource), ExGetCurrentResourceThread(),UDF_BUG_CHECK_ID,__LINE__))
#define UDFAcquireSharedStarveExclusive(Resource,CanWait) \
    (UDFDebugAcquireSharedStarveExclusive((Resource), (CanWait), UDF_BUG_CHECK_ID,__LINE__))
#define UDFAcquireSharedWaitForExclusive(Resource,CanWait) \
    (UDFDebugAcquireSharedWaitForExclusive((Resource), (CanWait), UDF_BUG_CHECK_ID,__LINE__))

#define UDFInterlockedIncrement(addr) \
    (UDFDebugInterlockedIncrement((addr), UDF_BUG_CHECK_ID,__LINE__))
#define UDFInterlockedDecrement(addr) \
    (UDFDebugInterlockedDecrement((addr), UDF_BUG_CHECK_ID,__LINE__))
#define UDFInterlockedExchangeAdd(addr,i) \
    (UDFDebugInterlockedExchangeAdd((addr),(i), UDF_BUG_CHECK_ID,__LINE__))

#endif //_CONSOLE

#define UDF_CHECK_PAGING_IO_RESOURCE(NTReqFCB) \
    ASSERT(!ExIsResourceAcquiredExclusiveLite(&(NTReqFCB->PagingIoResource))); \
    ASSERT(!ExIsResourceAcquiredSharedLite(&(NTReqFCB->PagingIoResource))); 

#define UDF_CHECK_EXVCB_RESOURCE(Vcb) \
    ASSERT( ExIsResourceAcquiredExclusiveLite(&(Vcb->VCBResource)) );

#define UDF_CHECK_BITMAP_RESOURCE(Vcb)
/* \
    ASSERT( (ExIsResourceAcquiredExclusiveLite(&(Vcb->VCBResource)) ||  \
             ExIsResourceAcquiredSharedLite(&(Vcb->VCBResource))) ); \
    ASSERT(ExIsResourceAcquiredExclusiveLite(&(Vcb->BitMapResource1))); \
*/
#endif //UDF_DBG

#define UDFRaiseStatus(IC,S) {                              \
    (IC)->SavedExceptionCode = (S);                         \
    ExRaiseStatus( (S) );                                   \
}

#define UDFNormalizeAndRaiseStatus(IC,S) {                                          \
    (IC)->SavedExceptionCode = FsRtlNormalizeNtstatus((S),STATUS_UNEXPECTED_IO_ERROR); \
    ExRaiseStatus( (IC)->SavedExceptionCode );                                         \
}

#define UDFIsRawDevice(RC) (           \
    ((RC) == STATUS_DEVICE_NOT_READY) || \
    ((RC) == STATUS_NO_MEDIA_IN_DEVICE)  \
)


// each file has a unique bug-check identifier associated with it.
//  Here is a list of constant definitions for these identifiers
#define UDF_FILE_INIT                                   (0x00000001)
#define UDF_FILE_FILTER                                 (0x00000002)
#define UDF_FILE_CREATE                                 (0x00000003)
#define UDF_FILE_CLEANUP                                (0x00000004)
#define UDF_FILE_CLOSE                                  (0x00000005)
#define UDF_FILE_READ                                   (0x00000006)
#define UDF_FILE_WRITE                                  (0x00000007)
#define UDF_FILE_INFORMATION                            (0x00000008)
#define UDF_FILE_FLUSH                                  (0x00000009)
#define UDF_FILE_VOL_INFORMATION                        (0x0000000A)
#define UDF_FILE_DIR_CONTROL                            (0x0000000B)
#define UDF_FILE_FILE_CONTROL                           (0x0000000C)
#define UDF_FILE_DEVICE_CONTROL                         (0x0000000D)
#define UDF_FILE_SHUTDOWN                               (0x0000000E)
#define UDF_FILE_LOCK_CONTROL                           (0x0000000F)
#define UDF_FILE_SECURITY                               (0x00000010)
#define UDF_FILE_EXT_ATTR                               (0x00000011)
#define UDF_FILE_MISC                                   (0x00000012)
#define UDF_FILE_FAST_IO                                (0x00000013)
#define UDF_FILE_FS_CONTROL                             (0x00000014)
#define UDF_FILE_PHYSICAL                               (0x00000015)
#define UDF_FILE_PNP                                    (0x00000016)
#define UDF_FILE_VERIFY_FS_CONTROL                      (0x00000017)
#define UDF_FILE_ENV_SPEC                               (0x00000018)
#define UDF_FILE_SYS_SPEC                               (0x00000019)
#define UDF_FILE_PHYS_EJECT                             (0x0000001A)

#define UDF_FILE_DLD                                    (0x00000200)
#define UDF_FILE_MEM                                    (0x00000201)
#define UDF_FILE_MEMH                                   (0x00000202)
#define UDF_FILE_WCACHE                                 (0x00000203)

#define UDF_FILE_UDF_INFO                               (0x00000100)
#define UDF_FILE_UDF_INFO_ALLOC                         (0x00000101)
#define UDF_FILE_UDF_INFO_DIR                           (0x00000102)
#define UDF_FILE_UDF_INFO_MOUNT                         (0x00000103)
#define UDF_FILE_UDF_INFO_EXTENT                        (0x00000104)
#define UDF_FILE_UDF_INFO_REMAP                         (0x00000105)
//#define UDF_FILE_UDF_INFO_                           (0x0000010x)

#define UDF_FILE_PROTECT                                (0x00000300)
//#define UDF_FILE_PROTECT_                                (0x0000030x)
    
#define SystemAllocatePool(hernya,size) ExAllocatePoolWithTag(hernya, size, 'Snwd')
#define SystemFreePool(addr) ExFreePool((PVOID)(addr))

//Device names

#include "Include/udf_reg.h"

#include <ddk/mountmgr.h>

#endif  // _UDF_UDF_H_

