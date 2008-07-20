/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    CdProcs.h

Abstract:

    This module defines all of the globally used procedures in the Cdfs
    file system.


--*/

#ifndef _CDPROCS_
#define _CDPROCS_

#include <ntifs.h>

#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddscsi.h>

#ifndef INLINE
#define INLINE __inline
#endif

#include "nodetype.h"
#include "Cd.h"
#include "CdStruc.h"
#include "CdData.h"


//**** x86 compiler bug ****

#if defined(_M_IX86)
#undef Int64ShraMod32
#define Int64ShraMod32(a, b) ((LONGLONG)(a) >> (b))
#endif

//
//  Here are the different pool tags.
//

#define TAG_CCB                 'ccdC'      //  Ccb
#define TAG_CDROM_TOC           'ctdC'      //  TOC
#define TAG_DIRENT_NAME         'nddC'      //  CdName in dirent
#define TAG_ENUM_EXPRESSION     'eedC'      //  Search expression for enumeration
#define TAG_FCB_DATA            'dfdC'      //  Data Fcb
#define TAG_FCB_INDEX           'ifdC'      //  Index Fcb
#define TAG_FCB_NONPAGED        'nfdC'      //  Nonpaged Fcb
#define TAG_FCB_TABLE           'tfdC'      //  Fcb Table entry
#define TAG_FILE_NAME           'nFdC'      //  Filename buffer
#define TAG_GEN_SHORT_NAME      'sgdC'      //  Generated short name
#define TAG_IO_BUFFER           'fbdC'      //  Temporary IO buffer
#define TAG_IO_CONTEXT          'oidC'      //  Io context for async reads
#define TAG_IRP_CONTEXT         'cidC'      //  Irp Context
#define TAG_IRP_CONTEXT_LITE    'lidC'      //  Irp Context lite
#define TAG_MCB_ARRAY           'amdC'      //  Mcb array
#define TAG_PATH_ENTRY_NAME     'nPdC'      //  CdName in path entry
#define TAG_PREFIX_ENTRY        'epdC'      //  Prefix Entry
#define TAG_PREFIX_NAME         'npdC'      //  Prefix Entry name
#define TAG_SPANNING_PATH_TABLE 'psdC'      //  Buffer for spanning path table
#define TAG_UPCASE_NAME         'nudC'      //  Buffer for upcased name
#define TAG_VOL_DESC            'dvdC'      //  Buffer for volume descriptor
#define TAG_VPB                 'pvdC'      //  Vpb allocated in filesystem

//
//  Tag all of our allocations if tagging is turned on
//

#ifdef POOL_TAGGING

#undef FsRtlAllocatePool
#undef FsRtlAllocatePoolWithQuota
#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'sfdC')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'sfdC')

#endif // POOL_TAGGING


//
//  File access check routine, implemented in AcChkSup.c
//

//
//  BOOLEAN
//  CdIllegalFcbAccess (
//      IN PIRP_CONTEXT IrpContext,
//      IN TYPE_OF_OPEN TypeOfOpen,
//      IN ACCESS_MASK DesiredAccess
//      );
//

#define CdIllegalFcbAccess(IC,T,DA) (                           \
           BooleanFlagOn( (DA),                                 \
                          ((T) != UserVolumeOpen ?              \
                           (FILE_WRITE_ATTRIBUTES           |   \
                            FILE_WRITE_DATA                 |   \
                            FILE_WRITE_EA                   |   \
                            FILE_ADD_FILE                   |   \
                            FILE_ADD_SUBDIRECTORY           |   \
                            FILE_APPEND_DATA) : 0)          |   \
                          FILE_DELETE_CHILD                 |   \
                          DELETE                            |   \
                          WRITE_DAC ))


//
//  Allocation support routines, implemented in AllocSup.c
//
//  These routines are for querying allocation on individual streams.
//

VOID
CdLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    OUT PLONGLONG DiskOffset,
    OUT PULONG ByteCount
    );

VOID
CdAddAllocationFromDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG McbEntryOffset,
    IN LONGLONG StartingFileOffset,
    IN PDIRENT Dirent
    );

VOID
CdAddInitialAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG StartingBlock,
    IN LONGLONG DataLength
    );

VOID
CdTruncateAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingFileOffset
    );

VOID
CdInitializeMcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
CdUninitializeMcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

VOID
CdCreateInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    );

VOID
CdDeleteInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

NTSTATUS
CdCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdPurgeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DismountUnderway
    );

//
//  VOID
//  CdUnpinData (
//      IN PIRP_CONTEXT IrpContext,
//      IN OUT PBCB *Bcb
//      );
//

#define CdUnpinData(IC,B)   \
    if (*(B) != NULL) { CcUnpinData( *(B) ); *(B) = NULL; }


//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

NTSTATUS
CdNonCachedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount
    );

NTSTATUS
CdNonCachedXARead (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount
    );

BOOLEAN
CdReadSectors (
    IN PIRP_CONTEXT IrpContext,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN RaiseOnError,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

NTSTATUS
CdCreateUserMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BufferLength,
    IN BOOLEAN RaiseOnError
    );

NTSTATUS
CdPerformDevIoCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT Device,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    );

//
//  VOID
//  CdMapUserBuffer (
//      IN PIRP_CONTEXT IrpContext
//      OUT PVOID UserBuffer
//      );
//
//  Returns pointer to sys address.  Will raise on failure.
//
//
//  VOID
//  CdLockUserBuffer (
//      IN PIRP_CONTEXT IrpContext,
//      IN ULONG BufferLength
//      );
//

#define CdMapUserBuffer(IC, UB) {                                               \
            *(UB) = (PVOID) ( ((IC)->Irp->MdlAddress == NULL) ?                 \
                    (IC)->Irp->UserBuffer :                                     \
                    (MmGetSystemAddressForMdlSafe( (IC)->Irp->MdlAddress, NormalPagePriority)));   \
            if (NULL == *(UB))  {                         \
                CdRaiseStatus( (IC), STATUS_INSUFFICIENT_RESOURCES);            \
            }                                                                   \
        }                                                                       
        

#define CdLockUserBuffer(IC,BL) {                   \
    if ((IC)->Irp->MdlAddress == NULL) {            \
        (VOID) CdCreateUserMdl( (IC), (BL), TRUE ); \
    }                                               \
}


//
//  Dirent support routines, implemented in DirSup.c
//

VOID
CdLookupDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG DirentOffset,
    OUT PDIRENT_ENUM_CONTEXT DirContext
    );

BOOLEAN
CdLookupNextDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIRENT_ENUM_CONTEXT CurrentDirContext,
    OUT PDIRENT_ENUM_CONTEXT NextDirContext
    );

VOID
CdUpdateDirentFromRawDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIRENT_ENUM_CONTEXT DirContext,
    IN OUT PDIRENT Dirent
    );

VOID
CdUpdateDirentName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN ULONG IgnoreCase
    );

BOOLEAN
CdFindFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN OUT PFILE_ENUM_CONTEXT FileContext,
    OUT PCD_NAME *MatchingName
    );

BOOLEAN
CdFindDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    );

BOOLEAN
CdFindFileByShortName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN ULONG ShortNameDirentOffset,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    );

BOOLEAN
CdLookupNextInitialFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_ENUM_CONTEXT FileContext
    );

VOID
CdLookupLastFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_ENUM_CONTEXT FileContext
    );

VOID
CdCleanupFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_ENUM_CONTEXT FileContext
    );

//
//  VOID
//  CdInitializeFileContext (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFILE_ENUM_CONTEXT FileContext
//      );
//
//
//  VOID
//  CdInitializeDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDIRENT Dirent
//      );
//
//  VOID
//  CdInitializeDirContext (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDIRENT_ENUM_CONTEXT DirContext
//      );
//
//  VOID
//  CdCleanupDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDIRENT Dirent
//      );
//
//  VOID
//  CdCleanupDirContext (
//      IN PIRP_CONTEXT IrpContext,
//      IN PDIRENT_ENUM_CONTEXT DirContext
//      );
//
//  VOID
//  CdLookupInitialFileDirent (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFILE_ENUM_CONTEXT FileContext,
//      IN ULONG DirentOffset
//      );
//

#define CdInitializeFileContext(IC,FC) {                                \
    RtlZeroMemory( FC, sizeof( FILE_ENUM_CONTEXT ));                    \
    (FC)->PriorDirent = &(FC)->Dirents[0];                              \
    (FC)->InitialDirent = &(FC)->Dirents[1];                            \
    (FC)->CurrentDirent = &(FC)->Dirents[2];                            \
    (FC)->ShortName.FileName.MaximumLength = BYTE_COUNT_8_DOT_3;        \
    (FC)->ShortName.FileName.Buffer = (FC)->ShortNameBuffer;            \
}

#define CdInitializeDirent(IC,D)                                \
    RtlZeroMemory( D, sizeof( DIRENT ))

#define CdInitializeDirContext(IC,DC)                           \
    RtlZeroMemory( DC, sizeof( DIRENT_ENUM_CONTEXT ))

#define CdCleanupDirent(IC,D)  {                                \
    if (FlagOn( (D)->Flags, DIRENT_FLAG_ALLOC_BUFFER )) {       \
        CdFreePool( &(D)->CdFileName.FileName.Buffer );          \
    }                                                           \
}

#define CdCleanupDirContext(IC,DC)                              \
    CdUnpinData( (IC), &(DC)->Bcb )

#define CdLookupInitialFileDirent(IC,F,FC,DO)                       \
    CdLookupDirent( IC,                                             \
                    F,                                              \
                    DO,                                             \
                    &(FC)->InitialDirent->DirContext );             \
    CdUpdateDirentFromRawDirent( IC,                                \
                                 F,                                 \
                                 &(FC)->InitialDirent->DirContext,  \
                                 &(FC)->InitialDirent->Dirent )


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

//
//  Type of opens.  FilObSup.c depends on this order.
//

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 0,
    StreamFileOpen,
    UserVolumeOpen,
    UserDirectoryOpen,
    UserFileOpen,
    BeyondValidType

} TYPE_OF_OPEN;
typedef TYPE_OF_OPEN *PTYPE_OF_OPEN;

VOID
CdSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PFCB Fcb OPTIONAL,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
CdDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb,
    OUT PCCB *Ccb
    );

TYPE_OF_OPEN
CdFastDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb
    );


//
//  Name support routines, implemented in NameSup.c
//

VOID
CdConvertNameToCdName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PCD_NAME CdName
    );

VOID
CdConvertBigToLittleEndian (
    IN PIRP_CONTEXT IrpContext,
    IN PCHAR BigEndian,
    IN ULONG ByteCount,
    OUT PCHAR LittleEndian
    );

VOID
CdUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_NAME Name,
    IN OUT PCD_NAME UpcaseName
    );

VOID
CdDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING FinalName
    );

BOOLEAN
CdIs8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING FileName
    );

VOID
CdGenerate8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN ULONG DirentOffset,
    OUT PWCHAR ShortFileName,
    OUT PUSHORT ShortByteCount
    );

BOOLEAN
CdIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PCD_NAME CurrentName,
    IN PCD_NAME SearchExpression,
    IN ULONG  WildcardFlags,
    IN BOOLEAN CheckVersion
    );

ULONG
CdShortNameDirentOffset (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    );

FSRTL_COMPARISON_RESULT
CdFullCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    );


//
//  Filesystem control operations.  Implemented in Fsctrl.c
//

NTSTATUS
CdLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

NTSTATUS
CdUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );


//
//  Path table enumeration routines.  Implemented in PathSup.c
//

VOID
CdLookupPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG PathEntryOffset,
    IN ULONG Ordinal,
    IN BOOLEAN VerifyBounds,
    IN OUT PCOMPOUND_PATH_ENTRY CompoundPathEntry
    );

BOOLEAN
CdLookupNextPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPATH_ENUM_CONTEXT PathContext,
    IN OUT PPATH_ENTRY PathEntry
    );

BOOLEAN
CdFindPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PCD_NAME DirName,
    IN BOOLEAN IgnoreCase,
    IN OUT PCOMPOUND_PATH_ENTRY CompoundPathEntry
    );

VOID
CdUpdatePathEntryName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PPATH_ENTRY PathEntry,
    IN BOOLEAN IgnoreCase
    );

//
//  VOID
//  CdInitializeCompoundPathEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCOMPOUND_PATH_ENTRY CompoundPathEntry
//      );
//
//  VOID
//  CdCleanupCompoundPathEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCOMPOUND_PATH_ENTRY CompoundPathEntry
//      );
//

#define CdInitializeCompoundPathEntry(IC,CP)                                    \
    RtlZeroMemory( CP, sizeof( COMPOUND_PATH_ENTRY ))

#define CdCleanupCompoundPathEntry(IC,CP)     {                                 \
    CdUnpinData( (IC), &(CP)->PathContext.Bcb );                                \
    if ((CP)->PathContext.AllocatedData) {                                      \
        CdFreePool( &(CP)->PathContext.Data );                                   \
    }                                                                           \
    if (FlagOn( (CP)->PathEntry.Flags, PATH_ENTRY_FLAG_ALLOC_BUFFER )) {        \
        CdFreePool( &(CP)->PathEntry.CdDirName.FileName.Buffer );                \
    }                                                                           \
}


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

VOID
CdInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCD_NAME Name,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortNameMatch,
    IN PFCB ParentFcb
    );

VOID
CdRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
CdFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *CurrentFcb,
    IN OUT PUNICODE_STRING RemainingName,
    IN BOOLEAN IgnoreCase
    );


//
//  Synchronization routines.  Implemented in Resrcsup.c
//
//  The following routines/macros are used to synchronize the in-memory structures.
//
//      Routine/Macro               Synchronizes                            Subsequent
//
//      CdAcquireCdData             Volume Mounts/Dismounts,Vcb Queue       CdReleaseCdData
//      CdAcquireVcbExclusive       Vcb for open/close                      CdReleaseVcb
//      CdAcquireVcbShared          Vcb for open/close                      CdReleaseVcb
//      CdAcquireAllFiles           Locks out operations to all files       CdReleaseAllFiles
//      CdAcquireFileExclusive      Locks out file operations               CdReleaseFile
//      CdAcquireFileShared         Files for file operations               CdReleaseFile
//      CdAcquireFcbExclusive       Fcb for open/close                      CdReleaseFcb
//      CdAcquireFcbShared          Fcb for open/close                      CdReleaseFcb
//      CdLockCdData                Fields in CdData                        CdUnlockCdData
//      CdLockVcb                   Vcb fields, FcbReference, FcbTable      CdUnlockVcb
//      CdLockFcb                   Fcb fields, prefix table, Mcb           CdUnlockFcb
//

typedef enum _TYPE_OF_ACQUIRE {
    
    AcquireExclusive,
    AcquireShared,
    AcquireSharedStarveExclusive

} TYPE_OF_ACQUIRE, *PTYPE_OF_ACQUIRE;

BOOLEAN
CdAcquireResource (
    IN PIRP_CONTEXT IrpContext,
    IN PERESOURCE Resource,
    IN BOOLEAN IgnoreWait,
    IN TYPE_OF_ACQUIRE Type
    );

//
//  BOOLEAN
//  CdAcquireCdData (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdReleaseCdData (
//      IN PIRP_CONTEXT IrpContext
//    );
//
//  BOOLEAN
//  CdAcquireVcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdAcquireVcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  VOID
//  CdReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  CdAcquireAllFiles (
//      IN PIRP_CONTEXT,
//      IN PVCB Vcb
//      );
//
//  VOID
//  CdReleaseAllFiles (
//      IN PIRP_CONTEXT,
//      IN PVCB Vcb
//      );
//
//  VOID
//  CdAcquireFileExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      );
//
//  VOID
//  CdAcquireFileShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdReleaseFile (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//    );
//
//  BOOLEAN
//  CdAcquireFcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdAcquireFcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdLockCdData (
//      );
//
//  VOID
//  CdUnlockCdData (
//      );
//
//  VOID
//  CdLockVcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdUnlockVcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdLockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdUnlockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define CdAcquireCdData(IC)                                                             \
    ExAcquireResourceExclusiveLite( &CdData.DataResource, TRUE )

#define CdReleaseCdData(IC)                                                             \
    ExReleaseResourceLite( &CdData.DataResource )

#define CdAcquireVcbExclusive(IC,V,I)                                                   \
    CdAcquireResource( (IC), &(V)->VcbResource, (I), AcquireExclusive )

#define CdAcquireVcbShared(IC,V,I)                                                      \
    CdAcquireResource( (IC), &(V)->VcbResource, (I), AcquireShared )

#define CdReleaseVcb(IC,V)                                                              \
    ExReleaseResourceLite( &(V)->VcbResource )

#define CdAcquireAllFiles(IC,V)                                                         \
    CdAcquireResource( (IC), &(V)->FileResource, FALSE, AcquireExclusive )

#define CdReleaseAllFiles(IC,V)                                                         \
    ExReleaseResourceLite( &(V)->FileResource )

#define CdAcquireFileExclusive(IC,F)                                                    \
    CdAcquireResource( (IC), (F)->Resource, FALSE, AcquireExclusive )

#define CdAcquireFileShared(IC,F)                                                       \
    CdAcquireResource( (IC), (F)->Resource, FALSE, AcquireShared )

#define CdAcquireFileSharedStarveExclusive(IC,F)                                        \
    CdAcquireResource( (IC), (F)->Resource, FALSE, AcquireSharedStarveExclusive )

#define CdReleaseFile(IC,F)                                                             \
    ExReleaseResourceLite( (F)->Resource )

#define CdAcquireFcbExclusive(IC,F,I)                                                   \
    CdAcquireResource( (IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireExclusive )

#define CdAcquireFcbShared(IC,F,I)                                                      \
    CdAcquireResource( (IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireShared )

#define CdReleaseFcb(IC,F)                                                              \
    ExReleaseResourceLite( &(F)->FcbNonpaged->FcbResource )

#define CdLockCdData()                                                                  \
    ExAcquireFastMutex( &CdData.CdDataMutex );                                          \
    CdData.CdDataLockThread = PsGetCurrentThread()

#define CdUnlockCdData()                                                                \
    CdData.CdDataLockThread = NULL;                                                     \
    ExReleaseFastMutex( &CdData.CdDataMutex )

#define CdLockVcb(IC,V)                                                                 \
    ExAcquireFastMutex( &(V)->VcbMutex );                                               \
    ASSERT( NULL == (V)->VcbLockThread);                                                \
    (V)->VcbLockThread = PsGetCurrentThread()

#define CdUnlockVcb(IC,V)                                                               \
    ASSERT( NULL != (V)->VcbLockThread);                                                \
    (V)->VcbLockThread = NULL;                                                          \
    ExReleaseFastMutex( &(V)->VcbMutex )

#define CdLockFcb(IC,F) {                                                               \
    PVOID _CurrentThread = PsGetCurrentThread();                                        \
    if (_CurrentThread != (F)->FcbLockThread) {                                         \
        ExAcquireFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
        ASSERT( (F)->FcbLockCount == 0 );                                               \
        (F)->FcbLockThread = _CurrentThread;                                            \
    }                                                                                   \
    (F)->FcbLockCount += 1;                                                             \
}

#define CdUnlockFcb(IC,F) {                                                             \
    (F)->FcbLockCount -= 1;                                                             \
    if ((F)->FcbLockCount == 0) {                                                       \
        (F)->FcbLockThread = NULL;                                                      \
        ExReleaseFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
    }                                                                                   \
}

BOOLEAN
CdNoopAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
CdNoopRelease (
    IN PVOID Fcb
    );

BOOLEAN
CdAcquireForCache (
    IN PFCB Fcb,
    IN BOOLEAN Wait
    );

VOID
CdReleaseFromCache (
    IN PFCB Fcb
    );

VOID
CdAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
CdReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    );


//
//  In-memory structure support routines.  Implemented in StrucSup.c
//

VOID
CdInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PCDROM_TOC CdromToc,
    IN ULONG TocLength,
    IN ULONG TocTrackCount,
    IN ULONG TocDiskFlags,
    IN ULONG BlockFactor,
    IN ULONG MediaChangeCount
    );

VOID
CdUpdateVcbFromVolDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PCHAR RawIsoVd OPTIONAL
    );

VOID
CdDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

PFCB
CdCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId,
    IN NODE_TYPE_CODE NodeTypeCode,
    OUT PBOOLEAN FcbExisted OPTIONAL
    );

VOID
CdInitializeFcbFromPathEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PPATH_ENTRY PathEntry
    );

VOID
CdInitializeFcbFromFileContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PFILE_ENUM_CONTEXT FileContext
    );

PCCB
CdCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags
    );

VOID
CdDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

BOOLEAN
CdCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    );

VOID
CdDeleteFileLock (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_LOCK FileLock
    );

PIRP_CONTEXT
CdCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
CdCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    );

VOID
CdInitializeStackIrpContext (
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite
    );

//
//  PIRP_CONTEXT_LITE
//  CdCreateIrpContextLite (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdFreeIrpContextLite (
//      IN PIRP_CONTEXT_LITE IrpContextLite
//      );
//

#define CdCreateIrpContextLite(IC)  \
    ExAllocatePoolWithTag( CdNonPagedPool, sizeof( IRP_CONTEXT_LITE ), TAG_IRP_CONTEXT_LITE )

#define CdFreeIrpContextLite(ICL)  \
    CdFreePool( &(ICL) )

VOID
CdTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartingFcb,
    OUT PBOOLEAN RemovedStartingFcb
    );

//
//  VOID
//  CdIncrementCleanupCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdDecrementCleanupCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdIncrementReferenceCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ULONG ReferenceCount
//      IN ULONG UserReferenceCount
//      );
//
//  VOID
//  CdDecrementReferenceCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ULONG ReferenceCount
//      IN ULONG UserReferenceCount
//      );
//
//  VOID
//  CdIncrementFcbReference (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  CdDecrementFcbReference (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define CdIncrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup += 1;                       \
    (F)->Vcb->VcbCleanup += 1;                  \
}

#define CdDecrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup -= 1;                       \
    (F)->Vcb->VcbCleanup -= 1;                  \
}

#define CdIncrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference += (C);                   \
    (F)->FcbUserReference += (UC);              \
    (F)->Vcb->VcbReference += (C);              \
    (F)->Vcb->VcbUserReference += (UC);         \
}

#define CdDecrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference -= (C);                   \
    (F)->FcbUserReference -= (UC);              \
    (F)->Vcb->VcbReference -= (C);              \
    (F)->Vcb->VcbUserReference -= (UC);         \
}

//
//  PCD_IO_CONTEXT
//  CdAllocateIoContext (
//      );
//
//  VOID
//  CdFreeIoContext (
//      PCD_IO_CONTEXT IoContext
//      );
//

#define CdAllocateIoContext()                           \
    FsRtlAllocatePoolWithTag( CdNonPagedPool,           \
                              sizeof( CD_IO_CONTEXT ),  \
                              TAG_IO_CONTEXT )

#define CdFreeIoContext(IO)     CdFreePool( &(IO) )

PFCB
CdLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_ID FileId
    );

PFCB
CdGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID *RestartKey
    );

NTSTATUS
CdProcessToc (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PCDROM_TOC CdromToc,
    IN OUT PULONG Length,
    OUT PULONG TrackCount,
    OUT PULONG DiskFlags
    );

//
//  For debugging purposes we sometimes want to allocate our structures from nonpaged
//  pool so that in the kernel debugger we can walk all the structures.
//

#define CdPagedPool                 PagedPool
#define CdNonPagedPool              NonPagedPool
#define CdNonPagedPoolCacheAligned  NonPagedPoolCacheAligned


//
//  Verification support routines.  Contained in verfysup.c
//


INLINE
BOOLEAN
CdOperationIsDasdOpen(
    IN PIRP_CONTEXT IrpContext
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->Irp);
    
    return ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
            (IrpSp->FileObject->FileName.Length == 0) &&
            (IrpSp->FileObject->RelatedFileObject == NULL));
}


NTSTATUS
CdPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceToVerify
    );

BOOLEAN
CdCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB,
    IN BOOLEAN Force
    );

VOID
CdVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
CdVerifyFcbOperation (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb
    );

BOOLEAN
CdDismountVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );


//
//  Macros to abstract device verify flag changes.
//

#define CdUpdateMediaChangeCount( V, C)  (V)->MediaChangeCount = (C)
#define CdUpdateVcbCondition( V, C)      (V)->VcbCondition = (C)

#define CdMarkRealDevForVerify( DO)  SetFlag( (DO)->Flags, DO_VERIFY_VOLUME)
                                     
#define CdMarkRealDevVerifyOk( DO)   ClearFlag( (DO)->Flags, DO_VERIFY_VOLUME)


#define CdRealDevNeedsVerify( DO)    BooleanFlagOn( (DO)->Flags, DO_VERIFY_VOLUME)

//
//  BOOLEAN
//  CdIsRawDevice (
//      IN PIRP_CONTEXT IrpContext,
//      IN NTSTATUS Status
//      );
//

#define CdIsRawDevice(IC,S) (           \
    ((S) == STATUS_DEVICE_NOT_READY) || \
    ((S) == STATUS_NO_MEDIA_IN_DEVICE)  \
)


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

NTSTATUS
CdFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdPrePostIrp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdOplockComplete (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  Miscellaneous support routines
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

//#ifndef BooleanFlagOn
//#define BooleanFlagOn(F,SF) (    \
//    (BOOLEAN)(((F) & (SF)) != 0) \
//)
//#endif

//#ifndef SetFlag
//#define SetFlag(Flags,SingleFlag) { \
//    (Flags) |= (SingleFlag);        \
//}
//#endif

//#ifndef ClearFlag
//#define ClearFlag(Flags,SingleFlag) { \
//    (Flags) &= ~(SingleFlag);         \
//}
//#endif

//
//      CAST
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          IN (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG_PTR)(OFFSET) - (ULONG_PTR)(BASE)))

//
//  This macro takes a pointer (or ulong) and returns its rounded up word
//  value
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up longword
//  value
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  The following macros round up and down to sector boundaries.
//

#define SectorAlign(L) (                                                \
    ((((ULONG)(L)) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))           \
)

#define LlSectorAlign(L) (                                              \
    ((((LONGLONG)(L)) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))        \
)

#define SectorTruncate(L) (                                             \
    ((ULONG)(L)) & ~(SECTOR_SIZE - 1)                                   \
)

#define LlSectorTruncate(L) (                                           \
    ((LONGLONG)(L)) & ~(SECTOR_SIZE - 1)                                \
)

#define BytesFromSectors(L) (                                           \
    ((ULONG) (L)) << SECTOR_SHIFT                                       \
)

#define SectorsFromBytes(L) (                                           \
    ((ULONG) (L)) >> SECTOR_SHIFT                                       \
)

#define LlBytesFromSectors(L) (                                         \
    Int64ShllMod32( (LONGLONG)(L), SECTOR_SHIFT )                       \
)

#define LlSectorsFromBytes(L) (                                         \
    Int64ShraMod32( (LONGLONG)(L), SECTOR_SHIFT )                       \
)

#define SectorOffset(L) (                                               \
    ((ULONG)(ULONG_PTR) (L)) & SECTOR_MASK                              \
)

#define SectorBlockOffset(V,LB) (                                       \
    ((ULONG) (LB)) & ((V)->BlocksPerSector - 1)                         \
)

#define BytesFromBlocks(V,B) (                                          \
    (ULONG) (B) << (V)->BlockToByteShift                                \
)

#define LlBytesFromBlocks(V,B) (                                        \
    Int64ShllMod32( (LONGLONG) (B), (V)->BlockToByteShift )             \
)

#define BlockAlign(V,L) (                                               \
    ((ULONG)(L) + (V)->BlockMask) & (V)->BlockInverseMask               \
)

//
//  Carefully make sure the mask is sign extended to 64bits
//

#define LlBlockAlign(V,L) (                                                     \
    ((LONGLONG)(L) + (V)->BlockMask) & (LONGLONG)((LONG)(V)->BlockInverseMask)  \
)

#define BlockOffset(V,L) (                                              \
    ((ULONG) (L)) & (V)->BlockMask                                      \
)

#define RawSectorAlign( B) ((((B)+(RAW_SECTOR_SIZE - 1)) / RAW_SECTOR_SIZE) * RAW_SECTOR_SIZE)

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

typedef union _USHORT2 {
    USHORT Ushort[2];
    ULONG  ForceAlignment;
} USHORT2, *PUSHORT2;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                           \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));  \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                           \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));  \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                           \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src));  \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//  accessing the source on a word boundary.
//

#define CopyUshort2(Dst,Src) {                          \
    *((USHORT2 *)(Dst)) = *((UNALIGNED USHORT2 *)(Src));\
    }


//
//  Following routines handle entry in and out of the filesystem.  They are
//  contained in CdData.c
//

NTSTATUS
CdFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

LONG
CdExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
CdProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

VOID
CdCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    );

//
//  VOID
//  CdRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//      );
//
//  VOID
//  CdNormalizeAndRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//      );
//

#if 0
#define AssertVerifyDevice(C, S)                                                    \
    ASSERT( (C) == NULL ||                                                          \
            FlagOn( (C)->Flags, IRP_CONTEXT_FLAG_IN_FSP ) ||                        \
            !((S) == STATUS_VERIFY_REQUIRED &&                                      \
              IoGetDeviceToVerify( PsGetCurrentThread() ) == NULL ));

#define AssertVerifyDeviceIrp(I)                                                    \
    ASSERT( (I) == NULL ||                                                          \
            !(((I)->IoStatus.Status) == STATUS_VERIFY_REQUIRED &&                   \
              ((I)->Tail.Overlay.Thread == NULL ||                                  \
                IoGetDeviceToVerify( (I)->Tail.Overlay.Thread ) == NULL )));
#else
#define AssertVerifyDevice(C, S)
#define AssertVerifyDeviceIrp(I)
#endif


#ifdef CD_SANITY

DECLSPEC_NORETURN
VOID
CdRaiseStatusEx(
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN BOOLEAN NormalizeStatus,
    IN OPTIONAL ULONG FileId,
    IN OPTIONAL ULONG Line
    );

#else

INLINE
DECLSPEC_NORETURN
VOID
CdRaiseStatusEx(
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN BOOLEAN NormalizeStatus,
    IN ULONG Fileid,
    IN ULONG Line
    )
{
    if (NormalizeStatus)  {

        IrpContext->ExceptionStatus = FsRtlNormalizeNtstatus( Status, STATUS_UNEXPECTED_IO_ERROR);
    }
    else {

        IrpContext->ExceptionStatus = Status;
    }

    IrpContext->RaisedAtLineFile = (Fileid << 16) | Line;

    ExRaiseStatus( IrpContext->ExceptionStatus );
}

#endif

#define CdRaiseStatus( IC, S)               CdRaiseStatusEx( (IC), (S), FALSE, BugCheckFileId, __LINE__);
#define CdNormalizeAndRaiseStatus( IC, S)   CdRaiseStatusEx( (IC), (S), TRUE, BugCheckFileId, __LINE__);

//
//  Following are the fast entry points.
//

BOOLEAN
CdFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
CdFastQueryNetworkInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  Following are the routines to handle the top level thread logic.
//

VOID
CdSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    );


//
//  VOID
//  CdRestoreThreadContext (
//      IN PIRP_CONTEXT IrpContext
//      );
//

#define CdRestoreThreadContext(IC)                              \
    (IC)->ThreadContext->Cdfs = 0;                              \
    IoSetTopLevelIrp( (IC)->ThreadContext->SavedTopLevelIrp );  \
    (IC)->ThreadContext = NULL

ULONG
CdSerial32 (
    IN PCHAR Buffer,
    IN ULONG ByteCount
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(I)   IoIsOperationSynchronous(I)

//
//  The following macro is used to set the fast i/o possible bits in the
//  FsRtl header.
//
//      FastIoIsNotPossible - If the Fcb is bad or there are oplocks on the file.
//
//      FastIoIsQuestionable - If there are file locks.
//
//      FastIoIsPossible - In all other cases.
//
//

#define CdIsFastIoPossible(F) ((BOOLEAN)                                            \
    ((((F)->Vcb->VcbCondition != VcbMounted ) ||                                    \
      !FsRtlOplockIsFastIoPossible( &(F)->Oplock )) ?                               \
                                                                                    \
     FastIoIsNotPossible :                                                          \
                                                                                    \
     ((((F)->FileLock != NULL) && FsRtlAreThereCurrentFileLocks( (F)->FileLock )) ? \
                                                                                    \
        FastIoIsQuestionable :                                                      \
                                                                                    \
        FastIoIsPossible))                                                          \
)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
CdFspDispatch (                             //  implemented in FspDisp.c
    IN PIRP_CONTEXT IrpContext
    );

VOID
CdFspClose (                                //  implemented in Close.c
    IN PVCB Vcb OPTIONAL
    );

//
//  The following routines are the entry points for the different operations
//  based on the IrpSp major functions.
//

NTSTATUS
CdCommonCreate (                            //  Implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonClose (                             //  Implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonRead (                              //  Implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonQueryInfo (                         //  Implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonSetInfo (                           //  Implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonQueryVolInfo (                      //  Implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonDirControl (                        //  Implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonFsControl (                         //  Implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonDevControl (                        //  Implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonLockControl (                       //  Implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonCleanup (                           //  Implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonPnp (                               //  Implemented in Pnp.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }
#define try_leave(S) { S; leave; }

//
//  Encapsulate safe pool freeing
//

INLINE
VOID
CdFreePool(
    IN PVOID *Pool
    )
{
    if (*Pool != NULL) {

        ExFreePool(*Pool);
        *Pool = NULL;
    }
}

#endif // _CDPROCS_


