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

#pragma warning(disable : 28172)

/**************** OPTIONS *****************/

//#define UDF_LIMIT_NAME_LEN

#ifdef UDF_LIMIT_NAME_LEN
  #define UDF_X_NAME_LEN (20)
  #define UDF_X_PATH_LEN (25)
#else //UDF_LIMIT_NAME_LEN
  #define UDF_X_NAME_LEN UDF_NAME_LEN
  #define UDF_X_PATH_LEN UDF_PATH_LEN
#endif //UDF_LIMIT_NAME_LEN

//#define UDF_ASYNC_IO

#define UDF_ALLOW_FRAG_AD

// Read ahead amount used for normal data files

#define READ_AHEAD_GRANULARITY           (0x10000)

#define UDF_DEFAULT_SPARSE_THRESHOLD (256*PACKETSIZE_UDF)

//#define ALLOW_SPARSE

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

//#define UDF_CDRW_EMULATION_ON_ROM

#define UDF_DELAYED_CLOSE

//#define UDF_ALLOW_HARD_LINKS

#define UDF_DEFAULT_BM_FLUSH_TIMEOUT 16         // seconds
#define UDF_DEFAULT_TREE_FLUSH_TIMEOUT 5        // seconds

/************* END OF OPTIONS **************/

// Common include files - should be in the include dir of the IFS Kit

#pragma warning(disable : 4996)
#pragma warning(disable : 4995)
#include <ntifs.h>
#include <ntddscsi.h>
#include <scsi.h>
#include <ntddcdrm.h>
#include <ntddcdvd.h>
#include "ntdddisk.h"
#include <pseh/pseh2.h>

#include "nodetype.h"

//  Udfs file id is a large integer.

typedef LARGE_INTEGER               FILE_ID;
typedef FILE_ID                     *PFILE_ID;

#ifdef __REACTOS__
// Downgrade unsupported NT6.2+ features.
#undef MdlMappingNoExecute
#define MdlMappingNoExecute 0
#define NonPagedPoolNx NonPagedPool
#endif

// #define NDEBUG
#ifndef NDEBUG
#define UDF_DBG
#endif

#define VALIDATE_STRUCTURES
// the following include files should be in the inc sub-dir associated with this driver

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "Include/regtools.h"
#include "Include/udf_reg.h"
#include "struct.h"
#ifdef UDF_METADATA_BATCH_DIAGNOSTICS
#define UDF_METADATA_COUNT(v,n) InterlockedIncrement(&(v)->MetadataBatchDiag[n])
#else
#define UDF_METADATA_COUNT(v,n) ((void)0)
#endif


// global variables - minimize these
extern UDFData              UdfData;

#include "udf_dbg.h"

#include "Include/Sys_spec_lib.h"

#include "udf_info/udf_info.h"

#include "protos.h"
#include "cow.h"

#include "Include/phys_lib.h"
#include "mem.h"

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

// try-finally simulation
#define try_return(S)   { S; goto try_exit; }
#define try_return1(S)  { S; goto try_exit1; }
#define try_return2(S)  { S; goto try_exit2; }

//  Encapsulate safe pool freeing

inline
VOID
UDFFreePool(
    _Inout_ _At_(*Pool, __drv_freesMem(Mem) _Post_null_) PVOID *Pool
    )
{
    if (*Pool != NULL) {

        ExFreePool(*Pool);
        *Pool = NULL;
    }
}

// some global (helpful) macros

#define UDFQuadAlign(Value)         ((((uint32)(Value)) + 7) & 0xfffffff8)

// small check for illegal open mode (desired access) if volume is
// read only (on standard CD-ROM device or another like this)
inline
BOOLEAN
UDFIllegalFcbAccess(
    IN PVCB Vcb,
    IN ACCESS_MASK DesiredAccess
    )
{
    //TODO: use real TypeOfOpen;
    TYPE_OF_OPEN TypeOfOpen = UserFileOpen;

    ACCESS_MASK WriteMask = (TypeOfOpen != UserVolumeOpen) ?
                            (FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | FILE_WRITE_DATA) : 0;

    WriteMask |= (DELETE | WRITE_DAC);

    return FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_READ_ONLY) &&
           BooleanFlagOn(DesiredAccess, WriteMask);
}

#if !defined(UDF_DBG)
#define UDFPrint(Args)
#else
#define UDFPrint(Args) KdPrint(Args)
#endif
#define UDFPrintErr(Args) KdPrint(Args)

#define UDFAcquireDeviceShared(IrpContext, Vcb, ResourceThreadId) \
    ((void)0) /* No operation - CD/DVD write modes not currently supported */

#define UDFReleaseDevice(IrpContext, Vcb, ResourceThreadId) \
    ((void)0) /* No operation - CD/DVD write modes not currently supported */


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

#define UDFAcquireSharedStarveExclusive(Resource,CanWait) \
    (ExAcquireSharedStarveExclusive((Resource),(CanWait)))

#define UDFAcquireSharedWaitForExclusive(Resource,CanWait) \
    (ExAcquireSharedWaitForExclusive((Resource),(CanWait)))

//
#if !defined(UDF_DBG)

#define UDF_CHECK_PAGING_IO_RESOURCE(NTReqFCB)
#define UDF_CHECK_EXVCB_RESOURCE(Vcb)
#define UDF_CHECK_BITMAP_RESOURCE(Vcb)

#else //UDF_DBG

#define UDF_CHECK_PAGING_IO_RESOURCE(Fcb) \
    ASSERT(!ExIsResourceAcquiredExclusiveLite(&Fcb->FcbNonpaged->FcbPagingIoResource)); \
    ASSERT(!ExIsResourceAcquiredSharedLite(&Fcb->FcbNonpaged->FcbPagingIoResource));

#define UDF_CHECK_EXVCB_RESOURCE(Vcb) \
    ASSERT( ExIsResourceAcquiredExclusiveLite(&(Vcb->VcbResource)) );

#define UDF_CHECK_BITMAP_RESOURCE(Vcb)
/* \
    ASSERT( (ExIsResourceAcquiredExclusiveLite(&(Vcb->VcbResource)) ||  \
             ExIsResourceAcquiredSharedLite(&(Vcb->VcbResource))) ); \
    ASSERT(ExIsResourceAcquiredExclusiveLite(&(Vcb->BitMapResource1))); \
*/
#endif //UDF_DBG

#define UDFRaiseStatus(IC,S) {                              \
    (IC)->ExceptionStatus = (S);                            \
    ExRaiseStatus( (S) );                                   \
}

#define UDFNormalizeAndRaiseStatus(IC,S) {                                          \
    (IC)->ExceptionStatus = FsRtlNormalizeNtstatus((S),STATUS_UNEXPECTED_IO_ERROR); \
    ExRaiseStatus( (IC)->ExceptionStatus );                                         \
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

#define         UDF_PART_DAMAGED_RW                 (0x00)
#define         UDF_PART_DAMAGED_RO                 (0x01)
#define         UDF_PART_DAMAGED_NO                 (0x02)

#define         UDF_FS_NAME_CD              L"\\UdfCd"
#define         UDF_FS_NAME_HDD             L"\\UdfHdd"

#define         UDF_ROOTDIR_NAME            L"\\"

#if DBG
#undef UDF_SANITY
#define UDF_SANITY
#endif

#if DBG

#define ASSERT_STRUCT(S,T)                  NT_ASSERT( SafeNodeType( S ) == (T) )
#define ASSERT_OPTIONAL_STRUCT(S,T)         NT_ASSERT( ((S) == NULL) ||  (SafeNodeType( S ) == (T)) )

#define ASSERT_VCB(V)                       ASSERT_STRUCT( (V), UDF_NODE_TYPE_VCB )
#define ASSERT_OPTIONAL_VCB(V)              ASSERT_OPTIONAL_STRUCT( (V), UDF_NODE_TYPE_VCB )

#define ASSERT_FCB(F)                                           \
    NT_ASSERT( (SafeNodeType(F) == UDF_NODE_TYPE_INDEX) ||      \
               (SafeNodeType(F) == UDF_NODE_TYPE_DATA) )

#define ASSERT_OPTIONAL_FCB(F)                                  \
    NT_ASSERT( ((F) == NULL) ||                                 \
            (SafeNodeType( F ) == UDF_NODE_TYPE_INDEX ) ||      \
            (SafeNodeType( F ) == UDF_NODE_TYPE_DATA ) )

#define ASSERT_FCB_NONPAGED(FN)             ASSERT_STRUCT( (FN), CDFS_NTC_FCB_NONPAGED )
#define ASSERT_OPTIONAL_FCB_NONPAGED(FN)    ASSERT_OPTIONAL_STRUCT( (FN), CDFS_NTC_FCB_NONPAGED )

#define ASSERT_CCB(C)                       ASSERT_STRUCT( (C), UDF_NODE_TYPE_CCB )
#define ASSERT_OPTIONAL_CCB(C)              ASSERT_OPTIONAL_STRUCT( (C), UDF_NODE_TYPE_CCB )

#define ASSERT_IRP_CONTEXT(IC)              ASSERT_STRUCT( (IC), UDF_NODE_TYPE_IRP_CONTEXT )
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC)     ASSERT_OPTIONAL_STRUCT( (IC), UDF_NODE_TYPE_IRP_CONTEXT )

#define ASSERT_IRP(I)                       ASSERT_STRUCT( (I), IO_TYPE_IRP )
#define ASSERT_OPTIONAL_IRP(I)              ASSERT_OPTIONAL_STRUCT( (I), IO_TYPE_IRP )

#define ASSERT_FILE_OBJECT(FO)              ASSERT_STRUCT( (FO), IO_TYPE_FILE )
#define ASSERT_OPTIONAL_FILE_OBJECT(FO)     ASSERT_OPTIONAL_STRUCT( (FO), IO_TYPE_FILE )

#define ASSERT_EXCLUSIVE_RESOURCE(R)        NT_ASSERT( ExIsResourceAcquiredExclusiveLite( R ))

#define ASSERT_SHARED_RESOURCE(R)           NT_ASSERT( ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_RESOURCE_NOT_MINE(R)         NT_ASSERT( !ExIsResourceAcquiredSharedLite( R ))

#define ASSERT_EXCLUSIVE_CDDATA             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &UdfData.GlobalDataResource ))
#define ASSERT_EXCLUSIVE_VCB(V)             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &(V)->VcbResource ))
#define ASSERT_SHARED_VCB(V)                NT_ASSERT( ExIsResourceAcquiredSharedLite( &(V)->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB(F)             NT_ASSERT( ExIsResourceAcquiredExclusiveLite( &(F)->FcbNonpaged->FcbResource ))
#define ASSERT_SHARED_FCB(F)                NT_ASSERT( ExIsResourceAcquiredSharedLite( &(F)->FcbNonpaged->FcbResource ))

#define ASSERT_EXCLUSIVE_FILE(F)            NT_ASSERT( ExIsResourceAcquiredExclusiveLite( (F)->Resource ))
#define ASSERT_SHARED_FILE(F)               NT_ASSERT( ExIsResourceAcquiredSharedLite( (F)->Resource ))

#define ASSERT_LOCKED_VCB(V)                NT_ASSERT( (V)->VcbLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_VCB(V)            NT_ASSERT( (V)->VcbLockThread != PsGetCurrentThread() )

#define ASSERT_LOCKED_FCB(F)                NT_ASSERT( !FlagOn( (F)->FcbState, FCB_STATE_IN_FCB_TABLE) || ((F)->FcbLockThread == PsGetCurrentThread()))
#define ASSERT_NOT_LOCKED_FCB(F)            NT_ASSERT( (F)->FcbLockThread != PsGetCurrentThread() )

#else

#define ASSERT_STRUCT(S,T)              { NOTHING; }
#define ASSERT_OPTIONAL_STRUCT(S,T)     { NOTHING; }
#define ASSERT_VCB(V)                   { NOTHING; }
#define ASSERT_OPTIONAL_VCB(V)          { NOTHING; }
#define ASSERT_FCB(F)                   { NOTHING; }
#define ASSERT_OPTIONAL_FCB(F)          { NOTHING; }
#define ASSERT_FCB_NONPAGED(FN)         { NOTHING; }
#define ASSERT_CCB(C)                   { NOTHING; }
#define ASSERT_OPTIONAL_CCB(C)          { NOTHING; }
#define ASSERT_IRP_CONTEXT(IC)          { NOTHING; }
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC) { NOTHING; }
#define ASSERT_IRP(I)                   { NOTHING; }
#define ASSERT_OPTIONAL_IRP(I)          { NOTHING; }
#define ASSERT_FILE_OBJECT(FO)          { NOTHING; }
#define ASSERT_OPTIONAL_FILE_OBJECT(FO) { NOTHING; }
#define ASSERT_EXCLUSIVE_RESOURCE(R)    { NOTHING; }
#define ASSERT_SHARED_RESOURCE(R)       { NOTHING; }
#define ASSERT_RESOURCE_NOT_MINE(R)     { NOTHING; }
#define ASSERT_EXCLUSIVE_CDDATA         { NOTHING; }
#define ASSERT_EXCLUSIVE_VCB(V)         { NOTHING; }
#define ASSERT_SHARED_VCB(V)            { NOTHING; }
#define ASSERT_EXCLUSIVE_FCB(F)         { NOTHING; }
#define ASSERT_SHARED_FCB(F)            { NOTHING; }
#define ASSERT_EXCLUSIVE_FILE(F)        { NOTHING; }
#define ASSERT_SHARED_FILE(F)           { NOTHING; }
#define ASSERT_LOCKED_VCB(V)            { NOTHING; }
#define ASSERT_NOT_LOCKED_VCB(V)        { NOTHING; }
#define ASSERT_LOCKED_FCB(F)            { NOTHING; }
#define ASSERT_NOT_LOCKED_FCB(F)        { NOTHING; }

#endif

#define IS_ALIGNED_POWER_OF_2(Value) \
    ((Value) != 0 && ((Value) & ((Value) - 1)) == 0)

#define MAX_SECTOR_SIZE          0x1000

#define FID_DIR_MASK  0x80000000            // high order bit means directory.

inline
FILE_ID
UdfGetFidFromLbAddr(lb_addr lbAddr)
{
    FILE_ID FileId;

    FileId.LowPart = lbAddr.logicalBlockNum;
    FileId.HighPart = lbAddr.partitionReferenceNum;

    return FileId;
}

#endif  // _UDF_UDF_H_

