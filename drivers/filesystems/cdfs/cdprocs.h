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

#ifdef _MSC_VER
#pragma warning( disable: 4127 ) // conditional expression is constant

#pragma warning( push )
#pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union
#pragma warning( disable: 4214 ) // nonstandard extension used : bit field types
#endif

#include <ntifs.h>

#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#ifdef __REACTOS__
#include <pseh/pseh2.h>
#endif

#ifndef INLINE
#define INLINE __inline
#endif

#include "nodetype.h"
#include "cd.h"
#include "cdstruc.h"
#include "cddata.h"

#ifdef CDFS_TELEMETRY_DATA

#include <winmeta.h>
#include <TraceLoggingProvider.h>
#include <telemetry\MicrosoftTelemetry.h>

#endif // CDFS_TELEMETRY_DATA

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifdef __REACTOS__
// Downgrade unsupported NT6.2+ features.
#undef MdlMappingNoExecute
#define MdlMappingNoExecute 0
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#endif

//**** x86 compiler bug ****

#if defined(_M_IX86)
#undef Int64ShraMod32
#define Int64ShraMod32(a, b) ((LONGLONG)(a) >> (b))
#endif

#ifndef Min
#define Min(a, b)   ((a) < (b) ? (a) : (b))
#endif

#ifndef Max
#define Max(a, b)   ((a) > (b) ? (a) : (b))
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
//      _In_ PIRP_CONTEXT IrpContext,
//      _In_ TYPE_OF_OPEN TypeOfOpen,
//      _In_ ACCESS_MASK DesiredAccess
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

_Requires_lock_held_(_Global_critical_region_)
VOID
CdLookupAllocation (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG FileOffset,
    _Out_ PLONGLONG DiskOffset,
    _Out_ PULONG ByteCount
    );

VOID
CdAddAllocationFromDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ ULONG McbEntryOffset,
    _In_ LONGLONG StartingFileOffset,
    _In_ PDIRENT Dirent
    );

VOID
CdAddInitialAllocation (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ ULONG StartingBlock,
    _In_ LONGLONG DataLength
    );

VOID
CdTruncateAllocation (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ LONGLONG StartingFileOffset
    );

_At_(Fcb->NodeByteSize, _In_range_(>=, FIELD_OFFSET( FCB, FcbType )))
VOID
CdInitializeMcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_updates_bytes_(Fcb->NodeByteSize) PFCB Fcb
    );

_At_(Fcb->NodeByteSize, _In_range_(>=, FIELD_OFFSET( FCB, FcbType )))
_When_(Fcb->NodeTypeCode == CDFS_NTC_FCB_PATH_TABLE, _At_(Fcb->NodeByteSize, _In_range_(==, SIZEOF_FCB_INDEX)))
_When_(Fcb->NodeTypeCode == CDFS_NTC_FCB_INDEX, _At_(Fcb->NodeByteSize, _In_range_(==, SIZEOF_FCB_INDEX)))
_When_(Fcb->NodeTypeCode == CDFS_NTC_FCB_DATA, _At_(Fcb->NodeByteSize, _In_range_(==, SIZEOF_FCB_DATA)))
VOID
CdUninitializeMcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_updates_bytes_(Fcb->NodeByteSize) PFCB Fcb
    );


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

VOID
CdCreateInternalStream (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _Inout_ PFCB Fcb,
    _In_ PUNICODE_STRING Name
    );

VOID
CdDeleteInternalStream (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb
    );

NTSTATUS
CdCompleteMdl (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdPurgeVolume (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ BOOLEAN DismountUnderway
    );

static /* ReactOS Change: GCC "multiple definition" */
INLINE /* GCC only accepts __inline as the first modifier */
VOID
CdVerifyOrCreateDirStreamFile (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    )
{
    //
    //  Unsafe test to see if call / lock neccessary.
    //
    
    if (NULL == Fcb->FileObject) {
        
        CdCreateInternalStream( IrpContext,
                                Fcb->Vcb,
                                Fcb, 
                                &Fcb->FileNamePrefix.ExactCaseName.FileName);
    }
}


//
//  VOID
//  CdUnpinData (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PBCB *Bcb
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

_Requires_lock_held_(_Global_critical_region_)
VOID
CdFreeDirCache (
    _In_ PIRP_CONTEXT IrpContext
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdNonCachedRead (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdNonCachedXARead (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdVolumeDasdWrite (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount
    );

BOOLEAN
CdReadSectors (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ LONGLONG StartingOffset,
    _In_ ULONG ByteCount,
    _In_ BOOLEAN ReturnError,
    _Out_writes_bytes_(ByteCount) PVOID Buffer,
    _In_ PDEVICE_OBJECT TargetDeviceObject
    );

NTSTATUS
CdCreateUserMdl (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG BufferLength,
    _In_ BOOLEAN RaiseOnError,
    _In_ LOCK_OPERATION Operation
    );

NTSTATUS
FASTCALL
CdPerformDevIoCtrl (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT Device,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl,
    _In_ BOOLEAN OverrideVerify,
    _Out_opt_ PIO_STATUS_BLOCK Iosb
    );

NTSTATUS
CdPerformDevIoCtrlEx (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT Device,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl,
    _In_ BOOLEAN OverrideVerify,
    _Out_opt_ PIO_STATUS_BLOCK Iosb
    );

NTSTATUS
CdHijackIrpAndFlushDevice (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ PDEVICE_OBJECT TargetDeviceObject
    );


//
//  VOID
//  CdMapUserBuffer (
//      _In_ PIRP_CONTEXT IrpContext
//      _Out_ PVOID UserBuffer
//      );
//
//  Returns pointer to sys address.  Will raise on failure.
//
//
//  VOID
//  CdLockUserBuffer (
//      _Inout_ PIRP_CONTEXT IrpContext,
//      _In_ ULONG BufferLength
//      );
//

#define CdMapUserBuffer(IC, UB) {                                               \
            *(UB) = (PVOID) ( ((IC)->Irp->MdlAddress == NULL) ?                 \
                    (IC)->Irp->UserBuffer :                                     \
                    (MmGetSystemAddressForMdlSafe( (IC)->Irp->MdlAddress, NormalPagePriority | MdlMappingNoExecute)));   \
            if (NULL == *(UB))  {                         \
                CdRaiseStatus( (IC), STATUS_INSUFFICIENT_RESOURCES);            \
            }                                                                   \
        }                                                                       
        

#define CdLockUserBuffer(IC,BL,OP) {                        \
    if ((IC)->Irp->MdlAddress == NULL) {                    \
        (VOID) CdCreateUserMdl( (IC), (BL), TRUE, (OP) );   \
    }                                                       \
}


//
//  Dirent support routines, implemented in DirSup.c
//

VOID
CdLookupDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ ULONG DirentOffset,
    _Out_ PDIRENT_ENUM_CONTEXT DirContext
    );

BOOLEAN
CdLookupNextDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PDIRENT_ENUM_CONTEXT CurrentDirContext,
    _Inout_ PDIRENT_ENUM_CONTEXT NextDirContext
    );

_At_(Dirent->CdTime, _Post_notnull_)
VOID
CdUpdateDirentFromRawDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PDIRENT_ENUM_CONTEXT DirContext,
    _Inout_ PDIRENT Dirent
    );

VOID
CdUpdateDirentName (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PDIRENT Dirent,
    _In_ ULONG IgnoreCase
    );

_Success_(return != FALSE) BOOLEAN
CdFindFile (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PCD_NAME Name,
    _In_ BOOLEAN IgnoreCase,
    _Inout_ PFILE_ENUM_CONTEXT FileContext,
    _Out_ PCD_NAME *MatchingName
    );

BOOLEAN
CdFindDirectory (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PCD_NAME Name,
    _In_ BOOLEAN IgnoreCase,
    _Inout_ PFILE_ENUM_CONTEXT FileContext
    );

_At_(FileContext->ShortName.FileName.MaximumLength, _In_range_(>=, BYTE_COUNT_8_DOT_3))
BOOLEAN
CdFindFileByShortName (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PCD_NAME Name,
    _In_ BOOLEAN IgnoreCase,
    _In_ ULONG ShortNameDirentOffset,
    _Inout_ PFILE_ENUM_CONTEXT FileContext
    );

BOOLEAN
CdLookupNextInitialFileDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _Inout_ PFILE_ENUM_CONTEXT FileContext
    );

VOID
CdLookupLastFileDirent (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ PFILE_ENUM_CONTEXT FileContext
    );

VOID
CdCleanupFileContext (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFILE_ENUM_CONTEXT FileContext
    );

//
//  VOID
//  CdInitializeFileContext (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Out_ PFILE_ENUM_CONTEXT FileContext
//      );
//
//
//  VOID
//  CdInitializeDirent (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Out_ PDIRENT Dirent
//      );
//
//  VOID
//  CdInitializeDirContext (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Out_ PDIRENT_ENUM_CONTEXT DirContext
//      );
//
//  VOID
//  CdCleanupDirent (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PDIRENT Dirent
//      );
//
//  VOID
//  CdCleanupDirContext (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PDIRENT_ENUM_CONTEXT DirContext
//      );
//
//  VOID
//  CdLookupInitialFileDirent (
//      _In_ PIRP_CONTEXT IrpContext,
//      _In_ PFCB Fcb,
//      _Out_ PFILE_ENUM_CONTEXT FileContext,
//      _In_ ULONG DirentOffset
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

_When_(TypeOfOpen == UnopenedFileObject, _At_(Fcb, _In_opt_))
_When_(TypeOfOpen != UnopenedFileObject, _At_(Fcb, _In_))
VOID
CdSetFileObject (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFILE_OBJECT FileObject,
    _In_ TYPE_OF_OPEN TypeOfOpen,
    PFCB Fcb,
    _In_opt_ PCCB Ccb
    );

_When_(return == UnopenedFileObject, _At_(*Fcb, _Post_null_))
_When_(return != UnopenedFileObject, _At_(Fcb, _Outptr_))
_When_(return == UnopenedFileObject, _At_(*Ccb, _Post_null_))
_When_(return != UnopenedFileObject, _At_(Ccb, _Outptr_))
TYPE_OF_OPEN
CdDecodeFileObject (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFILE_OBJECT FileObject,
    PFCB *Fcb,
    PCCB *Ccb
    );

TYPE_OF_OPEN
CdFastDecodeFileObject (
    _In_ PFILE_OBJECT FileObject,
    _Out_ PFCB *Fcb
    );


//
//  Name support routines, implemented in NameSup.c
//

_Post_satisfies_(_Old_(CdName->FileName.Length) >=
                 CdName->FileName.Length + CdName->VersionString.Length)
VOID
CdConvertNameToCdName (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PCD_NAME CdName
    );

VOID
CdConvertBigToLittleEndian (
    _In_ PIRP_CONTEXT IrpContext,
    _In_reads_bytes_(ByteCount) PCHAR BigEndian,
    _In_ ULONG ByteCount,
    _Out_writes_bytes_(ByteCount) PCHAR LittleEndian
    );

VOID
CdUpcaseName (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PCD_NAME Name,
    _Inout_ PCD_NAME UpcaseName
    );

VOID
CdDissectName (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PUNICODE_STRING RemainingName,
    _Out_ PUNICODE_STRING FinalName
    );

BOOLEAN
CdIsLegalName (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PUNICODE_STRING FileName
    );

BOOLEAN
CdIs8dot3Name (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ UNICODE_STRING FileName
    );

VOID
CdGenerate8dot3Name (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PUNICODE_STRING FileName,
    _In_ ULONG DirentOffset,
    _Out_writes_bytes_to_(BYTE_COUNT_8_DOT_3, *ShortByteCount) PWCHAR ShortFileName,
    _Out_ PUSHORT ShortByteCount
    );

BOOLEAN
CdIsNameInExpression (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PCD_NAME CurrentName,
    _In_ PCD_NAME SearchExpression,
    _In_ ULONG  WildcardFlags,
    _In_ BOOLEAN CheckVersion
    );

ULONG
CdShortNameDirentOffset (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PUNICODE_STRING Name
    );

FSRTL_COMPARISON_RESULT
CdFullCompareNames (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PUNICODE_STRING NameA,
    _In_ PUNICODE_STRING NameB
    );


//
//  Filesystem control operations.  Implemented in Fsctrl.c
//

_Requires_lock_held_(_Global_critical_region_)
_Requires_lock_held_(Vcb->VcbResource)
NTSTATUS
CdLockVolumeInternal (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_opt_ PFILE_OBJECT FileObject
    );

NTSTATUS
CdUnlockVolumeInternal (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_opt_ PFILE_OBJECT FileObject
    );


//
//  Path table enumeration routines.  Implemented in PathSup.c
//

VOID
CdLookupPathEntry (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ ULONG PathEntryOffset,
    _In_ ULONG Ordinal,
    _In_ BOOLEAN VerifyBounds,
    _Inout_ PCOMPOUND_PATH_ENTRY CompoundPathEntry
    );

BOOLEAN
CdLookupNextPathEntry (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PPATH_ENUM_CONTEXT PathContext,
    _Inout_ PPATH_ENTRY PathEntry
    );

_Success_(return != FALSE)
BOOLEAN
CdFindPathEntry (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB ParentFcb,
    _In_ PCD_NAME DirName,
    _In_ BOOLEAN IgnoreCase,
    _Inout_ PCOMPOUND_PATH_ENTRY CompoundPathEntry
    );

VOID
CdUpdatePathEntryName (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PPATH_ENTRY PathEntry,
    _In_ BOOLEAN IgnoreCase
    );

//
//  VOID
//  CdInitializeCompoundPathEntry (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Out_ PCOMPOUND_PATH_ENTRY CompoundPathEntry
//      );
//
//  VOID
//  CdCleanupCompoundPathEntry (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Out_ PCOMPOUND_PATH_ENTRY CompoundPathEntry
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
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ PCD_NAME Name,
    _In_ BOOLEAN IgnoreCase,
    _In_ BOOLEAN ShortNameMatch,
    _Inout_ PFCB ParentFcb
    );

VOID
CdRemovePrefix (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
CdFindPrefix (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB *CurrentFcb,
    _Inout_ PUNICODE_STRING RemainingName,
    _In_ BOOLEAN IgnoreCase
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

_Requires_lock_held_(_Global_critical_region_)
_When_(Type == AcquireExclusive && return != FALSE, _Acquires_exclusive_lock_(*Resource))
_When_(Type == AcquireShared && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(Type == AcquireSharedStarveExclusive && return != FALSE, _Acquires_shared_lock_(*Resource))
_When_(IgnoreWait == FALSE, _Post_satisfies_(return == TRUE))
BOOLEAN
CdAcquireResource (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PERESOURCE Resource,
    _In_ BOOLEAN IgnoreWait,
    _In_ TYPE_OF_ACQUIRE Type
    );

//
//  BOOLEAN
//  CdAcquireCdData (
//      _In_ PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdReleaseCdData (
//      _In_ PIRP_CONTEXT IrpContext
//    );
//
//  BOOLEAN
//  CdAcquireVcbExclusive (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PVCB Vcb,
//      _In_ BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdAcquireVcbShared (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PVCB Vcb,
//      _In_ BOOLEAN IgnoreWait
//      );
//
//  VOID
//  CdReleaseVcb (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PVCB Vcb
//      );
//
//  VOID
//  CdAcquireAllFiles (
//      _In_ PIRP_CONTEXT,
//      _In_ PVCB Vcb
//      );
//
//  VOID
//  CdReleaseAllFiles (
//      _In_ PIRP_CONTEXT,
//      _In_ PVCB Vcb
//      );
//
//  VOID
//  CdAcquireFileExclusive (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb,
//      );
//
//  VOID
//  CdAcquireFileShared (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//
//  VOID
//  CdReleaseFile (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//    );
//
//  BOOLEAN
//  CdAcquireFcbExclusive (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb,
//      _In_ BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdAcquireFcbShared (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb,
//      _In_ BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  CdReleaseFcb (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
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
//      _In_ PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdUnlockVcb (
//      _In_ PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdLockFcb (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//
//  VOID
//  CdUnlockFcb (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//


#define CdAcquireCacheForRead( IC)                                                      \
    ExAcquireResourceSharedLite( &(IC)->Vcb->SectorCacheResource, TRUE)
    
#define CdAcquireCacheForUpdate( IC)                                                    \
    ExAcquireResourceExclusiveLite( &(IC)->Vcb->SectorCacheResource, TRUE)
    
#define CdReleaseCache( IC)                                                             \
    ExReleaseResourceLite( &(IC)->Vcb->SectorCacheResource);

#define CdConvertCacheToShared( IC)                                                     \
    ExConvertExclusiveToSharedLite( &(IC)->Vcb->SectorCacheResource);

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
    NT_ASSERT( NULL == (V)->VcbLockThread);                                             \
    (V)->VcbLockThread = PsGetCurrentThread()

#define CdUnlockVcb(IC,V)                                                               \
    NT_ASSERT( NULL != (V)->VcbLockThread);                                             \
    (V)->VcbLockThread = NULL;                                                          \
    ExReleaseFastMutex( &(V)->VcbMutex )

#if defined(_PREFAST_)

_Success_(return)
_IRQL_saves_global_(OldIrql, FastMutex)
BOOLEAN DummySaveIrql(_Inout_ PFAST_MUTEX FastMutex);

_Success_(return)
_IRQL_restores_global_(OldIrql, FastMutex)
BOOLEAN DummyRestoreIrql(_Inout_ PFAST_MUTEX FastMutex);
#endif // _PREFAST_

#define CdLockFcb(IC,F) {                                                               \
    PVOID _CurrentThread = PsGetCurrentThread();                                        \
    if (_CurrentThread != (F)->FcbLockThread) {                                         \
        ExAcquireFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
        NT_ASSERT( (F)->FcbLockCount == 0 );                                            \
        _Analysis_assume_( (F)->FcbLockCount == 0 );                                    \
        (F)->FcbLockThread = _CurrentThread;                                            \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        _Analysis_assume_lock_held_( (F)->FcbNonpaged->FcbMutex );                      \
        _Analysis_assume_(FALSE != DummySaveIrql(&(F)->FcbNonpaged->FcbMutex));   \
    }                                                                                   \
    (F)->FcbLockCount += 1;                                                             \
}

#define CdUnlockFcb(IC,F) {                                                             \
    (F)->FcbLockCount -= 1;                                                             \
    if ((F)->FcbLockCount == 0) {                                                       \
        (F)->FcbLockThread = NULL;                                                      \
        ExReleaseFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        _Analysis_assume_lock_not_held_( (F)->FcbNonpaged->FcbMutex );                  \
        _Analysis_assume_(FALSE != DummyRestoreIrql(&(F)->FcbNonpaged->FcbMutex)); \
    }                                                                                   \
}

//
//  The following macro is used to retrieve the oplock structure within
//  the Fcb. This structure was moved to the advanced Fcb header
//  in Win8.
//

#if (NTDDI_VERSION >= NTDDI_WIN8)

#define CdGetFcbOplock(F)   &(F)->Header.Oplock

#else

#define CdGetFcbOplock(F)   &(F)->Oplock

#endif

BOOLEAN
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdNoopAcquire (
    _In_ PVOID Fcb,
    _In_ BOOLEAN Wait
    );

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdNoopRelease (
    _In_ PVOID Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
_When_(return!=0, _Acquires_shared_lock_(*Fcb->Resource))
BOOLEAN
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdAcquireForCache (
    _Inout_ PFCB Fcb,
    _In_ BOOLEAN Wait
    );

_Requires_lock_held_(_Global_critical_region_)
_Releases_lock_(*Fcb->Resource)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdReleaseFromCache (
    _Inout_ PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdFilterCallbackAcquireForCreateSection (
    _In_ PFS_FILTER_CALLBACK_DATA CallbackData,
    _Unreferenced_parameter_ PVOID *CompletionContext
    );

_Function_class_(FAST_IO_RELEASE_FILE)
_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdReleaseForCreateSection (
    _In_ PFILE_OBJECT FileObject
    );


//
//  In-memory structure support routines.  Implemented in StrucSup.c
//

VOID
CdInitializeVcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_ __drv_aliasesMem PDEVICE_OBJECT TargetDeviceObject,
    _In_ __drv_aliasesMem PVPB Vpb,
    _In_ __drv_aliasesMem PCDROM_TOC_LARGE CdromToc,
    _In_ ULONG TocLength,
    _In_ ULONG TocTrackCount,
    _In_ ULONG TocDiskFlags,
    _In_ ULONG BlockFactor,
    _In_ ULONG MediaChangeCount
    );

VOID
CdUpdateVcbFromVolDescriptor (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_reads_bytes_opt_(SECTOR_SIZE) PCHAR RawIsoVd
    );

VOID
CdDeleteVcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb
    );

PFCB
CdCreateFcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FILE_ID FileId,
    _In_ NODE_TYPE_CODE NodeTypeCode,
    _Out_opt_ PBOOLEAN FcbExisted
    );

VOID
CdInitializeFcbFromPathEntry (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_opt_ PFCB ParentFcb,
    _In_ PPATH_ENTRY PathEntry
    );

VOID
CdInitializeFcbFromFileContext (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ PFCB ParentFcb,
    _In_ PFILE_ENUM_CONTEXT FileContext
    );

PCCB
CdCreateCcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb,
    _In_ ULONG Flags
    );

VOID
CdDeleteCcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ __drv_freesMem( Pool ) PCCB Ccb
    );

_When_(RaiseOnError || return, _At_(Fcb->FileLock, _Post_notnull_))
_When_(RaiseOnError, _At_(IrpContext, _Pre_notnull_))
BOOLEAN
CdCreateFileLock (
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ BOOLEAN RaiseOnError
    );

VOID
CdDeleteFileLock (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFILE_LOCK FileLock
    );

_Ret_valid_ PIRP_CONTEXT
CdCreateIrpContext (
    _In_ PIRP Irp,
    _In_ BOOLEAN Wait
    );

VOID
CdCleanupIrpContext (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ BOOLEAN Post
    );

VOID
CdInitializeStackIrpContext (
    _Out_ PIRP_CONTEXT IrpContext,
    _In_ PIRP_CONTEXT_LITE IrpContextLite
    );

//
//  PIRP_CONTEXT_LITE
//  CdCreateIrpContextLite (
//      _In_ PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdFreeIrpContextLite (
//      _Inout_ PIRP_CONTEXT_LITE IrpContextLite
//      );
//

#define CdCreateIrpContextLite(IC)  \
    ExAllocatePoolWithTag( CdNonPagedPool, sizeof( IRP_CONTEXT_LITE ), TAG_IRP_CONTEXT_LITE )

#define CdFreeIrpContextLite(ICL)  \
    CdFreePool( &(ICL) )

_Requires_lock_held_(_Global_critical_region_)
VOID
CdTeardownStructures (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB StartingFcb,
    _Out_ PBOOLEAN RemovedStartingFcb
    );

//
//  VOID
//  CdIncrementCleanupCounts (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//
//  VOID
//  CdDecrementCleanupCounts (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//
//  VOID
//  CdIncrementReferenceCounts (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb,
//      _In_ ULONG ReferenceCount
//      _In_ ULONG UserReferenceCount
//      );
//
//  VOID
//  CdDecrementReferenceCounts (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb,
//      _In_ ULONG ReferenceCount
//      _In_ ULONG UserReferenceCount
//      );
//
//  VOID
//  CdIncrementFcbReference (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
//      );
//
//  VOID
//  CdDecrementFcbReference (
//      _In_ PIRP_CONTEXT IrpContext,
//      _Inout_ PFCB Fcb
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

#define CdFreeIoContext(IO)     CdFreePool( (PVOID) &(IO) ) /* ReactOS Change: GCC "passing argument 1 from incompatible pointer type" */

PFCB
CdLookupFcbTable (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ FILE_ID FileId
    );

PFCB
CdGetNextFcb (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PVCB Vcb,
    _In_ PVOID *RestartKey
    );

NTSTATUS
CdProcessToc (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PCDROM_TOC_LARGE CdromToc,
    _Inout_ PULONG Length,
    _Out_ PULONG TrackCount,
    _Inout_ PULONG DiskFlags
    );

//
//  For debugging purposes we sometimes want to allocate our structures from nonpaged
//  pool so that in the kernel debugger we can walk all the structures.
//

#define CdPagedPool                 PagedPool
#define CdNonPagedPool              NonPagedPoolNx
#define CdNonPagedPoolCacheAligned  NonPagedPoolNxCacheAligned


//
//  Verification support routines.  Contained in verfysup.c
//

static /* ReactOS Change: GCC "multiple definition" */
INLINE
BOOLEAN
CdOperationIsDasdOpen (
    _In_ PIRP_CONTEXT IrpContext
    )
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->Irp);
    
    return ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
            (IrpSp->FileObject->FileName.Length == 0) &&
            (IrpSp->FileObject->RelatedFileObject == NULL));
}

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdPerformVerify (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ PDEVICE_OBJECT DeviceToVerify
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
CdCheckForDismount (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb,
    _In_ BOOLEAN Force
    );

BOOLEAN
CdMarkDevForVerifyIfVcbMounted (
    _Inout_ PVCB Vcb
    );

VOID
CdVerifyVcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb
    );

BOOLEAN
CdVerifyFcbOperation (
    _In_opt_ PIRP_CONTEXT IrpContext,
    _In_ PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
CdDismountVcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PVCB Vcb
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
//      _In_ PIRP_CONTEXT IrpContext,
//      _In_ NTSTATUS Status
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

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdFsdPostRequest (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdPrePostIrp (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
CdOplockComplete (
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );


//
//  Miscellaneous support routines
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

/* GCC complains about multi-line comments.
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
*/

//
//      CAST
//      Add2Ptr (
//          _In_ PVOID Pointer,
//          _In_ ULONG Increment
//          _In_ (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          _In_ PVOID BasePtr,
//          _In_ PVOID OffsetPtr
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

static /* ReactOS Change: GCC "multiple definition" */
INLINE
ULONG
SectorsFromLlBytes( 
    ULONGLONG Bytes
) {

    return (ULONG)(Bytes >> SECTOR_SHIFT);
}

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
//  This macro copies an unaligned src longword to a dst longword,
//  performing an little/big endian swap.
//

#define SwapCopyUchar4(Dst,Src) {                                        \
    *((UNALIGNED UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src) + 3);     \
    *((UNALIGNED UCHAR1 *)(Dst) + 1) = *((UNALIGNED UCHAR1 *)(Src) + 2); \
    *((UNALIGNED UCHAR1 *)(Dst) + 2) = *((UNALIGNED UCHAR1 *)(Src) + 1); \
    *((UNALIGNED UCHAR1 *)(Dst) + 3) = *((UNALIGNED UCHAR1 *)(Src));     \
}

VOID
CdLbnToMmSsFf (
    _In_ ULONG Blocks,
    _Out_writes_(3) PUCHAR Msf
    );

//
//  Following routines handle entry in and out of the filesystem.  They are
//  contained in CdData.c
//

_IRQL_requires_max_(APC_LEVEL)
__drv_dispatchType(DRIVER_DISPATCH)
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
__drv_dispatchType(IRP_MJ_READ)
__drv_dispatchType(IRP_MJ_WRITE)
__drv_dispatchType(IRP_MJ_QUERY_INFORMATION)
__drv_dispatchType(IRP_MJ_SET_INFORMATION)
__drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)
__drv_dispatchType(IRP_MJ_DIRECTORY_CONTROL)
__drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL)
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
__drv_dispatchType(IRP_MJ_LOCK_CONTROL)
__drv_dispatchType(IRP_MJ_CLEANUP)
__drv_dispatchType(IRP_MJ_PNP)
__drv_dispatchType(IRP_MJ_SHUTDOWN)
NTSTATUS
NTAPI
CdFsdDispatch (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    );

// DRIVER_DISPATCH CdFsdDispatch;

LONG
CdExceptionFilter (
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PEXCEPTION_POINTERS ExceptionPointer
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdProcessException (
    _In_opt_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp,
    _In_ NTSTATUS ExceptionCode
    );

VOID
CdCompleteRequest (
    _Inout_opt_ PIRP_CONTEXT IrpContext,
    _Inout_opt_ PIRP Irp,
    _In_ NTSTATUS Status
    );

//
//  VOID
//  CdRaiseStatus (
//      _In_ PRIP_CONTEXT IrpContext,
//      _In_ NT_STATUS Status
//      );
//
//  VOID
//  CdNormalizeAndRaiseStatus (
//      _In_ PRIP_CONTEXT IrpContext,
//      _In_ NT_STATUS Status
//      );
//

#if 0
#define AssertVerifyDevice(C, S)                                                    \
    NT_ASSERT( (C) == NULL ||                                                          \
            FlagOn( (C)->Flags, IRP_CONTEXT_FLAG_IN_FSP ) ||                        \
            !((S) == STATUS_VERIFY_REQUIRED &&                                      \
              IoGetDeviceToVerify( PsGetCurrentThread() ) == NULL ));

#define AssertVerifyDeviceIrp(I)                                                    \
    NT_ASSERT( (I) == NULL ||                                                          \
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
CdRaiseStatusEx (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ NTSTATUS Status,
    _In_ BOOLEAN NormalizeStatus,
    _In_opt_ ULONG FileId,
    _In_opt_ ULONG Line
    );

#else

#ifdef __REACTOS__
static //  make windows gcc release build compile
#endif
INLINE
DECLSPEC_NORETURN
VOID
CdRaiseStatusEx(
    _In_ PIRP_CONTEXT IrpContext,
    _In_ NTSTATUS Status,
    _In_ BOOLEAN NormalizeStatus,
    _In_ ULONG Fileid,
    _In_ ULONG Line
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

//  _Success_(return != FALSE)
//  BOOLEAN
//  CdFastQueryBasicInfo (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ BOOLEAN Wait,
//      _Out_ PFILE_BASIC_INFORMATION Buffer,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_QUERY_BASIC_INFO CdFastQueryBasicInfo;

//  _Success_(return != FALSE)
//  BOOLEAN
//  CdFastQueryStdInfo (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ BOOLEAN Wait,
//      _Out_ PFILE_STANDARD_INFORMATION Buffer,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_QUERY_STANDARD_INFO CdFastQueryStdInfo;

//  BOOLEAN
//  CdFastLock (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ PLARGE_INTEGER FileOffset,
//      _In_ PLARGE_INTEGER Length,
//      _In_ PEPROCESS ProcessId,
//      _In_ ULONG Key,
//      _In_ BOOLEAN FailImmediately,
//      _In_ BOOLEAN ExclusiveLock,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_LOCK CdFastLock;

//  BOOLEAN
//  CdFastUnlockSingle (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ PLARGE_INTEGER FileOffset,
//      _In_ PLARGE_INTEGER Length,
//      _In_ PEPROCESS ProcessId,
//      _In_ ULONG Key,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_UNLOCK_SINGLE CdFastUnlockSingle;

//  BOOLEAN
//  CdFastUnlockAll (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ PEPROCESS ProcessId,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_UNLOCK_ALL CdFastUnlockAll;

//  BOOLEAN
//  CdFastUnlockAllByKey (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ PVOID ProcessId,
//      _In_ ULONG Key,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_UNLOCK_ALL_BY_KEY CdFastUnlockAllByKey;

//  BOOLEAN
//  CdFastIoCheckIfPossible (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ PLARGE_INTEGER FileOffset,
//      _In_ ULONG Length,
//      _In_ BOOLEAN Wait,
//      _In_ ULONG LockKey,
//      _In_ BOOLEAN CheckForReadOperation,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_CHECK_IF_POSSIBLE CdFastIoCheckIfPossible;

//  _Success_(return != FALSE)
//  BOOLEAN
//  CdFastQueryNetworkInfo (
//      _In_ PFILE_OBJECT FileObject,
//      _In_ BOOLEAN Wait,
//      _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer,
//      _Out_ PIO_STATUS_BLOCK IoStatus,
//      _In_ PDEVICE_OBJECT DeviceObject
//      );

FAST_IO_QUERY_NETWORK_OPEN_INFO CdFastQueryNetworkInfo;

//
//  Following are the routines to handle the top level thread logic.
//

VOID
CdSetThreadContext (
    _Inout_ PIRP_CONTEXT IrpContext,
    _In_ PTHREAD_CONTEXT ThreadContext
    );


//
//  VOID
//  CdRestoreThreadContext (
//      _Inout_ PIRP_CONTEXT IrpContext
//      );
//

#define CdRestoreThreadContext(IC)                              \
    (IC)->ThreadContext->Cdfs = 0;                              \
    IoSetTopLevelIrp( (IC)->ThreadContext->SavedTopLevelIrp );  \
    (IC)->ThreadContext = NULL

ULONG
CdSerial32 (
    _In_reads_bytes_(ByteCount) PCHAR Buffer,
    _In_ ULONG ByteCount
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
      !FsRtlOplockIsFastIoPossible( CdGetFcbOplock(F) )) ?                          \
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

//  VOID
//  CdFspDispatch (                             //  implemented in FspDisp.c
//      _Inout_ PIRP_CONTEXT IrpContext
//      );

WORKER_THREAD_ROUTINE CdFspDispatch;

VOID
CdFspClose (                                //  implemented in Close.c
    _In_opt_ PVCB Vcb
    );

//
//  The following routines are the entry points for the different operations
//  based on the IrpSp major functions.
//

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonCreate (                            //  Implemented in Create.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonClose (                             //  Implemented in Close.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonRead (                              //  Implemented in Read.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonWrite (                             //  Implemented in Write.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonQueryInfo (                         //  Implemented in FileInfo.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonSetInfo (                           //  Implemented in FileInfo.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonQueryVolInfo (                      //  Implemented in VolInfo.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonDirControl (                        //  Implemented in DirCtrl.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonFsControl (                         //  Implemented in FsCtrl.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

NTSTATUS
CdCommonDevControl (                        //  Implemented in DevCtrl.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

NTSTATUS
CdCommonLockControl (                       //  Implemented in LockCtrl.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonCleanup (                           //  Implemented in Cleanup.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonPnp (                               //  Implemented in Pnp.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
CdCommonShutdown (                         //  Implemented in Shutdown.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
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

#ifndef __REACTOS__
#define try_return(S) { S; goto try_exit; }
#define try_leave(S) { S; leave; }
#else
#define try_return(S) { S; goto try_exit; }
#define try_leave(S) { S; _SEH2_LEAVE; }
#endif

//
//  Encapsulate safe pool freeing
//
/* ReactOS Change: GCC "passing argument 1 of CdFreePool from incompatible pointer type" */
#define CdFreePool(x) _CdFreePool((PVOID*)(x))

static /* ReactOS Change: GCC "multiple definition" */
INLINE
VOID
_CdFreePool(
    _Inout_ _At_(*Pool, __drv_freesMem(Mem) _Post_null_) PVOID *Pool
    )
{
    if (*Pool != NULL) {

        ExFreePool(*Pool);
        *Pool = NULL;
    }
}

#ifdef CDFS_TELEMETRY_DATA

//
//  CDFS Telemetry.  Current implementation uses the Telemetry TraceLogging APIs.
//
//  The Telemetry TraceLoggingWrite() routines use a lot of stack space. We must
//  therefor wrap all our telemetry points with our own routines, and add a guard to
//  make sure there's enough stack space to call these routines.
//
//  These telemetry routines should not be called on high-performance code paths.
//

TRACELOGGING_DECLARE_PROVIDER( CdTelemetryProvider );

VOID
CdInitializeTelemetry (
    VOID
    );

DECLSPEC_NOINLINE
VOID
CdTelemetryMount (
        __in PGUID VolumeGuid,
        __in NTSTATUS Status,
        __in PVCB Vcb
        );

//
//  Every additional argument passed to TraceLoggingWrite() consumes an additional
//  16 to 32 bytes extra stack space. Having 512 bytes reserved space should be
//  sufficient for up to 20 arguments or so. This will be less of course if our
//  wrapper routines also declare their own local variables.
//

#define CDFS_TELEMETRY_STACK_THRESHOLD_DEFAULT  512    // for "small" telemetry points
#define CDFS_TELEMETRY_STACK_THRESHOLD_LARGE    2048   // for "large" telemetry points

INLINE
BOOLEAN
CdTelemetryGuard (
    __in ULONG StackSpaceNeeded )
/*++

Routine Description:

    This routine returns TRUE only when:

      1)  There is an ETW listener, AND
      2)  There is enough free stack space to safely call the Telemetry TraceLogging APIs

    We'll also count how many times there wasn't enough stack space, and include this
    value as part of the periodic cdfs Telemetry.

Arguments:

    StackSpaceNeeded - Stack space needed in bytes

--*/
{
    ASSERT( IoGetRemainingStackSize() >= StackSpaceNeeded );

    if (CdTelemetryProvider->LevelPlus1 <= 5) {

        //
        //  Bail out early if there are no ETW listeners
        //

        return FALSE;
    }

    if (IoGetRemainingStackSize() < StackSpaceNeeded) {

        //
        //  Count how many times it was unsafe to generate telemetry because of
        //  not enough stack space.
        //

        InterlockedIncrement( &CdTelemetryData.MissedTelemetryPoints );

        return FALSE;
    }

    return TRUE;
}

#define CdTelemetryMountSafe( VolumeGuid, Status, Vcb ) \
    if (CdTelemetryGuard( CDFS_TELEMETRY_STACK_THRESHOLD_LARGE )) { \
        CdTelemetryMount( VolumeGuid, Status, Vcb ); \
    }

#if DBG
#define CDFS_TELEMETRY_PERIODIC_INTERVAL  CdTelemetryData.PeriodicInterval
#else
#define CDFS_TELEMETRY_PERIODIC_INTERVAL  INTERVAL_ONE_DAY
#endif

#else  // CDFS_TELEMETRY_DATA

//
//  When CDFS_TELEMETRY_DATA is not defined then the CdTelemetry___Safe() routines
//  expand to nothing.  This minimizes the cdfs.sys binary footprint.  This also
//  means that the places where these Safe() routines are called do not
//  have to have to be surrounded by  #ifdef CDFS_TELEMETRY_DATA .. #endif
//


#define CdTelemetryMountSafe( ... )           NOTHING

#endif  // CDFS_TELEMETRY_DATA

#endif // _CDPROCS_


