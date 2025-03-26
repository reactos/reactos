/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    FatProcs.h

Abstract:

    This module defines all of the globally used procedures in the FAT
    file system.


--*/

#ifndef _FATPROCS_
#define _FATPROCS_

#ifdef _MSC_VER
#pragma warning( disable: 4127 ) // conditional expression is constant

#pragma warning( push )
#pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union
#pragma warning( disable: 4214 ) // nonstandard extension used : bit field types
#endif

#include <ntifs.h>


#include <ntddscsi.h>
#include <scsi.h>
#include <ntddcdrm.h>
#include <ntdddisk.h>
#include <ntddstor.h>
#include <ntintsafe.h>
#ifdef __REACTOS__
#include <pseh/pseh2.h>
#include <dbgbitmap.h>
#endif


#ifdef __REACTOS__
// Downgrade unsupported NT6.2+ features.
#undef MdlMappingNoExecute
#define MdlMappingNoExecute 0
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#undef POOL_NX_ALLOCATION
#define POOL_NX_ALLOCATION 0

// Moved up: needed in 'fatstruc.h'.
typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    VirtualVolumeFile,
    DirectoryFile,
    EaFile,
} TYPE_OF_OPEN;
#endif

#include "nodetype.h"
#include "fat.h"
#include "lfn.h"
#include "fatstruc.h"
#include "fatdata.h"



#ifdef _MSC_VER
#pragma warning( pop )
#endif

#ifndef INLINE
#define INLINE __inline
#endif

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

#ifndef MAX_ULONG
#define MAX_ULONG ((ULONG)-1)
#endif

#ifndef MAX_USHORT
#define MAX_USHORT ((USHORT)-1)
#endif

//
//  We must explicitly tag our allocations.
//

#undef FsRtlAllocatePool
#undef FsRtlAllocatePoolWithQuota

//
//  A function that returns finished denotes if it was able to complete the
//  operation (TRUE) or could not complete the operation (FALSE) because the
//  wait value stored in the irp context was false and we would have had
//  to block for a resource or I/O
//

typedef BOOLEAN FINISHED;

//
//  Size (characters) of stack allocated name component buffers in
//  the create/rename paths.
//

#define FAT_CREATE_INITIAL_NAME_BUF_SIZE    32


//
//  Some string buffer handling functions,  implemented in strucsup.c
//

VOID
FatFreeStringBuffer (
    _Inout_ PVOID String
    );

VOID
FatExtendString(
    _Inout_ PVOID String,
    _In_ USHORT DesiredBufferSize,
    _In_ BOOLEAN FreeOldBuffer,
    __out_opt PBOOLEAN NeedsFree
    );

VOID
FatEnsureStringBufferEnough (
    _Inout_ PVOID String,
    _In_ USHORT DesiredBufferSize
    );

BOOLEAN
FatAddMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    IN LBO Lbo,
    IN ULONG SectorCount
    );

BOOLEAN
FatLookupMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount OPTIONAL,
    OUT PULONG Index OPTIONAL
    );

BOOLEAN
FatLookupLastMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    OUT PVBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG Index OPTIONAL
    );

BOOLEAN
FatGetNextMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN ULONG RunIndex,
    OUT PVBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount
    );

VOID
FatRemoveMcbEntry (
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN VBO Vbo,
    IN ULONG SectorCount
    );


//
//  File access check routine, implemented in AcChkSup.c
//

BOOLEAN
FatCheckFileAccess (
    PIRP_CONTEXT IrpContext,
    IN UCHAR DirentAttributes,
    IN PACCESS_MASK DesiredAccess
    );

BOOLEAN
FatCheckManageVolumeAccess (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PACCESS_STATE AccessState,
    _In_ KPROCESSOR_MODE ProcessorMode
    );

NTSTATUS
FatExplicitDeviceAccessGranted (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE ProcessorMode
    );


//
//  Allocation support routines, implemented in AllocSup.c
//

static
INLINE
BOOLEAN
FatIsIoRangeValid (
    IN PVCB Vcb,
    IN LARGE_INTEGER Start,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine enforces the restriction that object space must be
    representable in 32 bits.

Arguments:

    Vcb - the volume the range is on

    Start - starting byte (zero based) of the range

    Length - size of the range

    HeaderSize - if the file has a header

Return Value:

    BOOLEAN - if, considering the cluster size, the neccesary size of
        the object to contain the range can be represented in 32 bits.

--*/

{

    UNREFERENCED_PARAMETER( Vcb );

    //
    //  The only restriction on a FAT object is that the filesize must
    //  fit in 32bits, i.e. <= 0xffffffff. This then implies that the
    //  range of valid byte offsets is [0, fffffffe].
    //
    //  Two phases which check for illegality
    //
    //      - if the high 32bits are nonzero
    //      - if the length would cause a 32bit overflow
    //

    return !(Start.HighPart ||
             Start.LowPart + Length < Start.LowPart);
}


//
//  This strucure is used by FatLookupFatEntry to remember a pinned page
//  of fat.
//

typedef struct _FAT_ENUMERATION_CONTEXT {

    VBO VboOfPinnedPage;
    PBCB Bcb;
    PVOID PinnedPage;

} FAT_ENUMERATION_CONTEXT, *PFAT_ENUMERATION_CONTEXT;

VOID
FatLookupFatEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FatIndex,
    IN OUT PULONG FatEntry,
    IN OUT PFAT_ENUMERATION_CONTEXT Context
    );

VOID
FatSetupAllocationSupport (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
FatTearDownAllocationSupport (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatLookupFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount,
    OUT PBOOLEAN Allocated,
    OUT PBOOLEAN EndOnMax,
    OUT PULONG Index OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatAddFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN ULONG AllocationSize
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatTruncateFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN ULONG AllocationSize
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatLookupFileAllocationSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatAllocateDiskSpace (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG AbsoluteClusterHint,
    IN OUT PULONG ByteCount,
    IN BOOLEAN ExactMatchRequired,
    OUT PLARGE_MCB Mcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeallocateDiskSpace (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb,
    IN BOOLEAN ZeroOnDeallocate
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSplitAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN VBO SplitAtVbo,
    OUT PLARGE_MCB RemainingMcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatMergeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN PLARGE_MCB SecondMcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSetFatEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FatIndex,
    IN FAT_ENTRY FatEntry
    );

UCHAR
FatLogOf(
    IN ULONG Value
    );


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

VOID
FatReadVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatPrepareWriteVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Reversible,
    IN BOOLEAN Zero
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatReadDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    IN BOOLEAN Pin,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    OUT PNTSTATUS Status
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatPrepareWriteDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Zero,
    IN BOOLEAN Reversible,
    OUT PNTSTATUS Status
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatOpenDirectoryFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    );

PFILE_OBJECT
FatOpenEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB EaFcb
    );

VOID
FatCloseEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushFirst
    );


_Requires_lock_held_(_Global_critical_region_)
VOID
FatSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PVCB Vcb OPTIONAL,
    IN BOOLEAN Reversible
    );

VOID
FatRepinBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb
    );

VOID
FatUnpinRepinnedBcbs (
    IN PIRP_CONTEXT IrpContext
    );

FINISHED
FatZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingZero,
    IN ULONG ByteCount
    );

NTSTATUS
FatCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
FatPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount,
    OUT PBCB *Bcb
    );

NTSTATUS
FatPrefetchPages (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN ULONG StartingPage,
    IN ULONG PageCount
    );

//
// VOID
// FatUnpinBcb (
//     IN PIRP_CONTEXT IrpContext,
//     IN OUT PBCB Bcb,
//     );
//

//
//  This macro unpins a Bcb, in the checked build make sure all
//  requests unpin all Bcbs before leaving.
//

#if DBG

#define FatUnpinBcb(IRPCONTEXT,BCB) {       \
    if ((BCB) != NULL) {                    \
        CcUnpinData((BCB));                 \
        NT_ASSERT( (IRPCONTEXT)->PinCount );\
        (IRPCONTEXT)->PinCount -= 1;        \
        (BCB) = NULL;                       \
    }                                       \
}

#else

#define FatUnpinBcb(IRPCONTEXT,BCB) { \
    if ((BCB) != NULL) {              \
        CcUnpinData((BCB));           \
        (BCB) = NULL;                 \
    }                                 \
}

#endif // DBG

VOID
FatInitializeCacheMap (
    _In_ PFILE_OBJECT FileObject,
    _In_ PCC_FILE_SIZES FileSizes,
    _In_ BOOLEAN PinAccess,
    _In_ PCACHE_MANAGER_CALLBACKS Callbacks,
    _In_ PVOID LazyWriteContext
    );

VOID
FatSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    );


//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

VOID
FatPagingFileIo (
    IN PIRP Irp,
    IN PFCB Fcb
    );


_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB FcbOrDcb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount,
    IN ULONG UserByteCount,
    IN ULONG StreamFlags
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatNonCachedNonAlignedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB FcbOrDcb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    );

VOID
FatMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PIRP Irp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    );

VOID
FatSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    );

VOID
FatWaitSync (
    IN PIRP_CONTEXT IrpContext
    );

VOID
FatLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

PVOID
FatBufferUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN ULONG BufferLength
    );

PVOID
FatMapUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp
    );

NTSTATUS
FatToggleMediaEjectDisable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN PreventRemoval
    );

NTSTATUS
FatPerformDevIoCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT Device,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    );

PMDL
FatBuildZeroMdl (
    __in PIRP_CONTEXT IrpContext,
    __in ULONG Length
    );


//
//  Dirent support routines, implemented in DirSup.c
//

//
//  Tunneling is a deletion precursor (all tunneling cases do
//  not involve deleting dirents, however)
//

VOID
FatTunnelFcbOrDcb (
    IN PFCB FcbOrDcb,
    IN PCCB Ccb OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
ULONG
FatCreateNewDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN ULONG DirentsNeeded,
    IN BOOLEAN RescanDir
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatInitializeDirectoryDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT ParentDirent
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN PDELETE_CONTEXT DeleteContext OPTIONAL,
    IN BOOLEAN DeleteEa
    );


_Requires_lock_held_(_Global_critical_region_)
VOID
FatLocateDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN PCCB Ccb,
    IN VBO OffsetToStartSearchFrom,
    IN OUT PULONG Flags,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset,
    OUT PBOOLEAN FileNameDos OPTIONAL,
    IN OUT PUNICODE_STRING Lfn OPTIONAL,
    IN OUT PUNICODE_STRING OrigLfn OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatLocateSimpleOemDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDirectory,
    IN POEM_STRING FileName,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
FatLfnDirentExists (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PUNICODE_STRING Lfn,
    IN PUNICODE_STRING LfnTmp
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatLocateVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb,
    OUT PVBO ByteOffset
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetDirentFromFcbOrDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN BOOLEAN ReturnOnFailure,
    OUT PDIRENT *Dirent,
    OUT PBCB *Bcb
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
FatIsDirectoryEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB TargetDcb,
    IN ULONG LfnOffset,
    IN ULONG DirentOffset,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn
    );


VOID
FatConstructDirent (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING FileName,
    IN BOOLEAN ComponentReallyLowercase,
    IN BOOLEAN ExtensionReallyLowercase,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN USHORT Attributes,
    IN BOOLEAN ZeroAndSetTimeFields,
    IN PLARGE_INTEGER SetCreationTime OPTIONAL
    );

VOID
FatConstructLabelDirent (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN POEM_STRING Label
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSetFileSizeInDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PULONG AlternativeFileSize OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSetFileSizeInDirentNoRaise (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PULONG AlternativeFileSize OPTIONAL
    );


_Requires_lock_held_(_Global_critical_region_)
VOID
FatUpdateDirentFromFcb (
   IN PIRP_CONTEXT IrpContext,
   IN PFILE_OBJECT FileObject,
   IN PFCB FcbOrDcb,
   IN PCCB Ccb
   );


//
//  Generate a relatively unique static 64bit ID from a FAT Fcb/Dcb
//
//  ULONGLONG
//  FatDirectoryKey (FcbOrDcb);
//

#define FatDirectoryKey(FcbOrDcb)  ((ULONGLONG)((FcbOrDcb)->CreationTime.QuadPart ^ (FcbOrDcb)->FirstClusterOfFile))


//
//  The following routines are used to access and manipulate the
//  clusters containing EA data in the ea data file.  They are
//  implemented in EaSup.c
//

//
//  VOID
//  FatUpcaseEaName (
//      IN PIRP_CONTEXT IrpContext,
//      IN POEM_STRING EaName,
//      OUT POEM_STRING UpcasedEaName
//      );
//

#define FatUpcaseEaName( IRPCONTEXT, NAME, UPCASEDNAME ) \
    RtlUpperString( UPCASEDNAME, NAME )

_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetEaLength (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDIRENT Dirent,
    OUT PULONG EaLength
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetNeedEaCount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDIRENT Dirent,
    OUT PULONG NeedEaCount
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatCreateEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PUCHAR Buffer,
    IN ULONG Length,
    IN POEM_STRING FileName,
    OUT PUSHORT EaHandle
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT EaHandle,
    IN POEM_STRING FileName
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetEaFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    OUT PDIRENT *EaDirent,
    OUT PBCB *EaBcb,
    IN BOOLEAN CreateFile,
    IN BOOLEAN ExclusiveFcb
    );

VOID
FatReadEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT EaHandle,
    IN POEM_STRING FileName,
    IN BOOLEAN ReturnEntireSet,
    OUT PEA_RANGE EaSetRange
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatDeleteEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PBCB EaBcb,
    OUT PDIRENT EaDirent,
    IN USHORT EaHandle,
    IN POEM_STRING Filename
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatAddEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG EaSetLength,
    IN PBCB EaBcb,
    OUT PDIRENT EaDirent,
    OUT PUSHORT EaHandle,
    OUT PEA_RANGE EaSetRange
    );

VOID
FatDeletePackedEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_SET_HEADER EaSetHeader,
    IN OUT PULONG PackedEasLength,
    IN ULONG Offset
    );

VOID
FatAppendPackedEa (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_SET_HEADER *EaSetHeader,
    IN OUT PULONG PackedEasLength,
    IN OUT PULONG AllocationLength,
    IN PFILE_FULL_EA_INFORMATION FullEa,
    IN ULONG BytesPerCluster
    );

ULONG
FatLocateNextEa (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    IN ULONG PreviousOffset
    );

BOOLEAN
FatLocateEaByName (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_EA FirstPackedEa,
    IN ULONG PackedEasLength,
    IN POEM_STRING EaName,
    OUT PULONG Offset
    );

BOOLEAN
FatIsEaNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN OEM_STRING Name
    );

VOID
FatPinEaRange (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT VirtualEaFile,
    IN PFCB EaFcb,
    IN OUT PEA_RANGE EaRange,
    IN ULONG StartingVbo,
    IN ULONG Length,
    IN NTSTATUS ErrorStatus
    );

VOID
FatMarkEaRangeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT EaFileObject,
    IN OUT PEA_RANGE EaRange
    );

VOID
FatUnpinEaRange (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PEA_RANGE EaRange
    );

//
//  The following macro computes the size of a full ea (not including
//  padding to bring it to a longword.  A full ea has a 4 byte offset,
//  folowed by 1 byte flag, 1 byte name length, 2 bytes value length,
//  the name, 1 null byte, and the value.
//
//      ULONG
//      SizeOfFullEa (
//          IN PFILE_FULL_EA_INFORMATION FullEa
//          );
//

#define SizeOfFullEa(EA) (4+1+1+2+(EA)->EaNameLength+1+(EA)->EaValueLength)


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

#ifndef __REACTOS__
typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    VirtualVolumeFile,
    DirectoryFile,
    EaFile,
} TYPE_OF_OPEN;
#endif

typedef enum _FAT_FLUSH_TYPE {

    NoFlush = 0,
    Flush,
    FlushAndInvalidate,
    FlushWithoutPurge

} FAT_FLUSH_TYPE;

VOID
FatSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcbOrDcb,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
FatDecodeFileObject (
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PVCB *Vcb,
    _Outptr_ PFCB *FcbOrDcb,
    _Outptr_ PCCB *Ccb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatPurgeReferencedFileObjects (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN FAT_FLUSH_TYPE FlushType
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatForceCacheMiss (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FAT_FLUSH_TYPE FlushType
    );


//
//  File system control routines, implemented in FsCtrl.c
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatFlushAndCleanVolume(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN FAT_FLUSH_TYPE FlushType
    );

BOOLEAN
FatIsBootSectorFat (
    IN PPACKED_BOOT_SECTOR BootSector
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

NTSTATUS
FatUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );


//
//  Name support routines, implemented in NameSup.c
//

//
//  BOOLEAN
//  FatAreNamesEqual (
//      IN PIRP_CONTEXT IrpContext,
//      IN OEM_STRING ConstantNameA,
//      IN OEM_STRING ConstantNameB
//      )
//
//  /*++
//
//  Routine Description:
//
//      This routine simple returns whether the two names are exactly equal.
//      If the two names are known to be constant, this routine is much
//      faster than FatIsDbcsInExpression.
//
//  Arguments:
//
//      ConstantNameA - Constant name.
//
//      ConstantNameB - Constant name.
//
//  Return Value:
//
//      BOOLEAN - TRUE if the two names are lexically equal.
//

#define FatAreNamesEqual(IRPCONTEXT,NAMEA,NAMEB) (      \
    ((ULONG)(NAMEA).Length == (ULONG)(NAMEB).Length) && \
    (RtlEqualMemory( &(NAMEA).Buffer[0],                \
                     &(NAMEB).Buffer[0],                \
                     (NAMEA).Length ))                  \
)

//
//  BOOLEAN
//  FatIsNameShortOemValid (
//      IN PIRP_CONTEXT IrpContext,
//      IN OEM_STRING Name,
//      IN BOOLEAN CanContainWildCards,
//      IN BOOLEAN PathNamePermissible,
//      IN BOOLEAN LeadingBackslashPermissible
//      )
//
//  /*++
//
//  Routine Description:
//
//      This routine scans the input name and verifies that if only
//      contains valid characters
//
//  Arguments:
//
//      Name - Supplies the input name to check.
//
//      CanContainWildCards - Indicates if the name can contain wild cards
//          (i.e., * and ?).
//
//  Return Value:
//
//          BOOLEAN - Returns TRUE if the name is valid and FALSE otherwise.
//
//  --*/
//
//  The FatIsNameLongOemValid and FatIsNameLongUnicodeValid are similar.
//

#define FatIsNameShortOemValid(IRPCONTEXT,NAME,CAN_CONTAIN_WILD_CARDS,PATH_NAME_OK,LEADING_BACKSLASH_OK) ( \
    FsRtlIsFatDbcsLegal((NAME),                   \
                        (CAN_CONTAIN_WILD_CARDS), \
                        (PATH_NAME_OK),           \
                        (LEADING_BACKSLASH_OK))   \
)

#define FatIsNameLongOemValid(IRPCONTEXT,NAME,CAN_CONTAIN_WILD_CARDS,PATH_NAME_OK,LEADING_BACKSLASH_OK) ( \
    FsRtlIsHpfsDbcsLegal((NAME),                  \
                        (CAN_CONTAIN_WILD_CARDS), \
                        (PATH_NAME_OK),           \
                        (LEADING_BACKSLASH_OK))   \
)

static
INLINE
BOOLEAN
FatIsNameLongUnicodeValid (
    PIRP_CONTEXT IrpContext,
    PUNICODE_STRING Name,
    BOOLEAN CanContainWildcards,
    BOOLEAN PathNameOk,
    BOOLEAN LeadingBackslashOk
    )
{
    ULONG i;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( LeadingBackslashOk );
    UNREFERENCED_PARAMETER( PathNameOk );

    //
    //  I'm not bothering to do the whole thing, just enough to make this call look
    //  the same as the others.
    //

    NT_ASSERT( !PathNameOk && !LeadingBackslashOk );

    for (i=0; i < Name->Length/sizeof(WCHAR); i++) {

        if ((Name->Buffer[i] < 0x80) &&
            !(FsRtlIsAnsiCharacterLegalHpfs(Name->Buffer[i], CanContainWildcards))) {

            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
FatIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN OEM_STRING Expression,
    IN OEM_STRING Name
    );

VOID
FatStringTo8dot3 (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ OEM_STRING InputString,
    _Out_writes_bytes_(11) PFAT8DOT3 Output8dot3
    );

VOID
Fat8dot3ToString (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PDIRENT Dirent,
    _In_ BOOLEAN RestoreCase,
    _Out_ POEM_STRING OutputString
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatGetUnicodeNameFromFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PUNICODE_STRING Lfn
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSetFullFileNameInFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
FatSetFullNameInFcb (
    _In_ PIRP_CONTEXT IrpContext,
    _Inout_ PFCB Fcb,
    _In_ PUNICODE_STRING FinalName
    );

VOID
FatUnicodeToUpcaseOem (
    IN PIRP_CONTEXT IrpContext,
    IN POEM_STRING OemString,
    IN PUNICODE_STRING UnicodeString
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatSelectNames (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Parent,
    IN POEM_STRING OemName,
    IN PUNICODE_STRING UnicodeName,
    IN OUT POEM_STRING ShortName,
    IN PUNICODE_STRING SuggestedShortName OPTIONAL,
    IN OUT BOOLEAN *AllLowerComponent,
    IN OUT BOOLEAN *AllLowerExtension,
    IN OUT BOOLEAN *CreateLfn
    );

VOID
FatEvaluateNameCase (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name,
    IN OUT BOOLEAN *AllLowerComponent,
    IN OUT BOOLEAN *AllLowerExtension,
    IN OUT BOOLEAN *CreateLfn
    );

BOOLEAN
FatSpaceInName (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING UnicodeName
    );

VOID
FatUnicodeRestoreShortNameCase(
    IN PUNICODE_STRING ShortNameWithCase,
    IN BOOLEAN LowerCase8,
    IN BOOLEAN LowerCase3
    );


//
//  Resources support routines/macros, implemented in ResrcSup.c
//
//  The following routines/macros are used for gaining shared and exclusive
//  access to the global/vcb data structures.  The routines are implemented
//  in ResrcSup.c.  There is a global resources that everyone tries to take
//  out shared to do their work, with the exception of mount/dismount which
//  take out the global resource exclusive.  All other resources only work
//  on their individual item.  For example, an Fcb resource does not take out
//  a Vcb resource.  But the way the file system is structured we know
//  that when we are processing an Fcb other threads cannot be trying to remove
//  or alter the Fcb, so we do not need to acquire the Vcb.
//
//  The procedures/macros are:
//
//          Macro          FatData    Vcb        Fcb         Subsequent macros
//
//  AcquireExclusiveGlobal Read/Write None       None        ReleaseGlobal
//
//  AcquireSharedGlobal    Read       None       None        ReleaseGlobal
//
//  AcquireExclusiveVcb    Read       Read/Write None        ReleaseVcb
//
//  AcquireSharedVcb       Read       Read       None        ReleaseVcb
//
//  AcquireExclusiveFcb    Read       None       Read/Write  ConvertToSharFcb
//                                                           ReleaseFcb
//
//  AcquireSharedFcb       Read       None       Read        ReleaseFcb
//
//  ConvertToSharedFcb     Read       None       Read        ReleaseFcb
//
//  ReleaseGlobal
//
//  ReleaseVcb
//
//  ReleaseFcb
//

//
//  FINISHED
//  FatAcquireExclusiveGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  FINISHED
//  FatAcquireSharedGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//

#define FatAcquireExclusiveGlobal(IRPCONTEXT) (                                                                     \
    ExAcquireResourceExclusiveLite( &FatData.Resource, BooleanFlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT) )  \
)

#define FatAcquireSharedGlobal(IRPCONTEXT) (                                                                        \
    ExAcquireResourceSharedLite( &FatData.Resource, BooleanFlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT) )     \
)

//
//  The following macro must only be called when Wait is TRUE!
//
//  FatAcquireExclusiveVolume (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  FatReleaseVolume (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define FatAcquireExclusiveVolume(IRPCONTEXT,VCB) {                                     \
    PFCB __FFFFFcb = NULL;                                                                    \
    NT_ASSERT(FlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT));                      \
    (VOID)FatAcquireExclusiveVcb( (IRPCONTEXT), (VCB) );                                \
    while ( (__FFFFFcb = FatGetNextFcbBottomUp((IRPCONTEXT), __FFFFFcb, (VCB)->RootDcb)) != NULL) { \
        (VOID)FatAcquireExclusiveFcb((IRPCONTEXT), __FFFFFcb );                               \
    }                                                                                   \
}

#define FatReleaseVolume(IRPCONTEXT,VCB) {                                              \
    PFCB __FFFFFcb = NULL;                                                                    \
    NT_ASSERT(FlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT));                      \
    while ( (__FFFFFcb = FatGetNextFcbBottomUp((IRPCONTEXT), __FFFFFcb, (VCB)->RootDcb)) != NULL) { \
        (VOID)ExReleaseResourceLite( __FFFFFcb->Header.Resource );                            \
    }                                                                                   \
    FatReleaseVcb((IRPCONTEXT), (VCB));                                                 \
}

//
//  Macro to enable easy tracking of Vcb state transitions.
//

#ifdef FASTFATDBG
#define FatSetVcbCondition( V, X) {                                            \
            DebugTrace(0,DEBUG_TRACE_VERFYSUP,"%d -> ",(V)->VcbCondition);     \
            DebugTrace(0,DEBUG_TRACE_VERFYSUP,"%x\n",(X));                     \
            (V)->VcbCondition = (X);                                           \
        }
#else
#define FatSetVcbCondition( V, X)       (V)->VcbCondition = (X)
#endif

//
//  These macros can be used to determine what kind of FAT we have for an
//  initialized Vcb.  It is somewhat more elegant to use these (visually).
//

#define FatIsFat32(VCB) ((BOOLEAN)((VCB)->AllocationSupport.FatIndexBitSize == 32))
#define FatIsFat16(VCB) ((BOOLEAN)((VCB)->AllocationSupport.FatIndexBitSize == 16))
#define FatIsFat12(VCB) ((BOOLEAN)((VCB)->AllocationSupport.FatIndexBitSize == 12))


_Requires_lock_held_(_Global_critical_region_)
_When_(return != FALSE && NoOpCheck != FALSE, _Acquires_exclusive_lock_(Vcb->Resource))
FINISHED
FatAcquireExclusiveVcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN NoOpCheck
    );

#define FatAcquireExclusiveVcb( IC, V)          FatAcquireExclusiveVcb_Real( IC, V, FALSE)
#define FatAcquireExclusiveVcbNoOpCheck( IC, V) FatAcquireExclusiveVcb_Real( IC, V, TRUE)

_Requires_lock_held_(_Global_critical_region_)
_When_(return != 0, _Acquires_shared_lock_(Vcb->Resource))
FINISHED
FatAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
_Acquires_exclusive_lock_(*Fcb->Header.Resource)
FINISHED
FatAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
_Acquires_shared_lock_(*Fcb->Header.Resource)
FINISHED
FatAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
_When_(return != 0, _Acquires_shared_lock_(*Fcb->Header.Resource))
FINISHED
FatAcquireSharedFcbWaitForEx (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

#define FatVcbAcquiredExclusive(IRPCONTEXT,VCB) (                   \
    ExIsResourceAcquiredExclusiveLite(&(VCB)->Resource)  ||         \
    ExIsResourceAcquiredExclusiveLite(&FatData.Resource)            \
)

#define FatFcbAcquiredShared(IRPCONTEXT,FCB) (                      \
    ExIsResourceAcquiredSharedLite((FCB)->Header.Resource)          \
)

#define FatFcbAcquiredExclusive(IRPCONTEXT,FCB) (                   \
    ExIsResourceAcquiredExclusiveLite((FCB)->Header.Resource)       \
)

#define FatAcquireDirectoryFileMutex(VCB) {                         \
    NT_ASSERT(KeAreApcsDisabled());                                 \
    ExAcquireFastMutexUnsafe(&(VCB)->DirectoryFileCreationMutex);   \
}

#define FatReleaseDirectoryFileMutex(VCB) {                         \
    NT_ASSERT(KeAreApcsDisabled());                                 \
    ExReleaseFastMutexUnsafe(&(VCB)->DirectoryFileCreationMutex);   \
}

//
//  The following are cache manager call backs

BOOLEAN
FatAcquireVolumeForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
FatReleaseVolumeFromClose (
    IN PVOID Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
NTAPI
FatAcquireFcbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI
FatReleaseFcbFromLazyWrite (
    IN PVOID Null
    );

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
NTAPI
FatAcquireFcbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
NTAPI
FatReleaseFcbFromReadAhead (
    IN PVOID Null
    );

_Function_class_(FAST_IO_ACQUIRE_FOR_CCFLUSH)
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatAcquireForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_RELEASE_FOR_CCFLUSH)
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatReleaseForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NTAPI
FatNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
NTAPI
FatNoOpRelease (
    IN PVOID Fcb
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
NTAPI
FatFilterCallbackAcquireForCreateSection (
    IN PFS_FILTER_CALLBACK_DATA CallbackData,
    OUT PVOID *CompletionContext
    );

//
//  VOID
//  FatConvertToSharedFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define FatConvertToSharedFcb(IRPCONTEXT,Fcb) {                 \
    ExConvertExclusiveToSharedLite( (Fcb)->Header.Resource );   \
    }

//
//  VOID
//  FatReleaseGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  FatReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  FatReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define FatDeleteResource(RESRC) {                      \
    ExDeleteResourceLite( (RESRC) );                    \
}

#define FatReleaseGlobal(IRPCONTEXT) {                  \
    ExReleaseResourceLite( &(FatData.Resource) );       \
    }

#define FatReleaseVcb(IRPCONTEXT,Vcb) {                 \
    ExReleaseResourceLite( &((Vcb)->Resource) );        \
    }

#define FatReleaseFcb(IRPCONTEXT,Fcb) {                 \
    ExReleaseResourceLite( (Fcb)->Header.Resource );    \
    }

//
//  The following macro is used to retrieve the oplock structure within
//  the Fcb. This structure was moved to the advanced Fcb header
//  in Win8.
//

#if (NTDDI_VERSION >= NTDDI_WIN8)

#define FatGetFcbOplock(F)  &(F)->Header.Oplock

#else

#define FatGetFcbOplock(F)  &(F)->Specific.Fcb.Oplock

#endif


//
//  In-memory structure support routine, implemented in StrucSup.c
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDEVICE_OBJECT FsDeviceObject
    );

VOID
FatTearDownVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
FatDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

PFCB
FatCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL,
    IN PUNICODE_STRING OrigLfn OPTIONAL,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN SingleResource
    );

PDCB
FatCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN ULONG LfnOffsetWithinDirectory,
    IN ULONG DirentOffsetWithinDirectory,
    IN PDIRENT Dirent,
    IN PUNICODE_STRING Lfn OPTIONAL
    );

VOID
FatDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB *Fcb
    );

PCCB
FatCreateCcb (
    IN PIRP_CONTEXT IrpContext
    );

VOID
FatDeallocateCcbStrings(
        IN PCCB Ccb
        );

VOID
FatDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB *Ccb
    );

PIRP_CONTEXT
FatCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
FatDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
    );

#ifdef FASTFATDBG
#define FatDeleteIrpContext(IRPCONTEXT) {   \
    FatDeleteIrpContext_Real((IRPCONTEXT)); \
    (IRPCONTEXT) = NULL;                    \
}
#else
#define FatDeleteIrpContext(IRPCONTEXT) {   \
    FatDeleteIrpContext_Real((IRPCONTEXT)); \
}
#endif // FASTFAT_DBG

PFCB
FatGetNextFcbTopDown (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    );

PFCB
FatGetNextFcbBottomUp (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    );

//
//  These two macros just make the code a bit cleaner.
//

#define FatGetFirstChild(DIR) ((PFCB)(                          \
    IsListEmpty(&(DIR)->Specific.Dcb.ParentDcbQueue) ? NULL :   \
    CONTAINING_RECORD((DIR)->Specific.Dcb.ParentDcbQueue.Flink, \
                      DCB,                                      \
                      ParentDcbLinks.Flink)))

#define FatGetNextSibling(FILE) ((PFCB)(                     \
    &(FILE)->ParentDcb->Specific.Dcb.ParentDcbQueue.Flink == \
    (PVOID)(FILE)->ParentDcbLinks.Flink ? NULL :             \
    CONTAINING_RECORD((FILE)->ParentDcbLinks.Flink,          \
                      FCB,                                   \
                      ParentDcbLinks.Flink)))

_Requires_lock_held_(_Global_critical_region_)
BOOLEAN
FatCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    IN BOOLEAN Force
    );

VOID
FatConstructNamesInFcb (
    IN PIRP_CONTEXT IrpContext,
    PFCB Fcb,
    PDIRENT Dirent,
    PUNICODE_STRING Lfn OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatCheckFreeDirentBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    );

ULONG
FatVolumeUncleanCount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
FatPreallocateCloseContext (
    IN PVCB Vcb
    );

PCLOSE_CONTEXT
FatAllocateCloseContext(
    IN PVCB Vcb
    );

//
//  BOOLEAN
//  FatIsRawDevice (
//      IN PIRP_CONTEXT IrpContext,
//      IN NTSTATUS Status
//      );
//

#define FatIsRawDevice(IC,S) (          \
    ((S) == STATUS_DEVICE_NOT_READY) || \
    ((S) == STATUS_NO_MEDIA_IN_DEVICE)  \
)


//
//  Routines to support managing file names Fcbs and Dcbs.
//  Implemented in SplaySup.c
//

VOID
FatInsertName (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PFILE_NAME_NODE Name
    );

VOID
FatRemoveNames (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PFCB
FatFindFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRTL_SPLAY_LINKS *RootNode,
    IN PSTRING Name,
    OUT PBOOLEAN FileNameDos OPTIONAL
    );

BOOLEAN
FatIsHandleCountZero (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

typedef enum _COMPARISON {
    IsLessThan,
    IsGreaterThan,
    IsEqual
} COMPARISON;

COMPARISON
FatCompareNames (
    IN PSTRING NameA,
    IN PSTRING NameB
    );

//
//  Do a macro here to check for a common case.
//

#define CompareNames(NAMEA,NAMEB) (                        \
    *(PUCHAR)(NAMEA)->Buffer != *(PUCHAR)(NAMEB)->Buffer ? \
    *(PUCHAR)(NAMEA)->Buffer < *(PUCHAR)(NAMEB)->Buffer ?  \
    IsLessThan : IsGreaterThan :                           \
    FatCompareNames((PSTRING)(NAMEA), (PSTRING)(NAMEB))    \
)

//
//  Time conversion support routines, implemented in TimeSup.c
//

_Success_(return != FALSE)
BOOLEAN
FatNtTimeToFatTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PLARGE_INTEGER NtTime,
    _In_ BOOLEAN Rounding,
    _Out_ PFAT_TIME_STAMP FatTime,
    _Out_opt_ PUCHAR TenMsecs
    );

LARGE_INTEGER
FatFatTimeToNtTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FAT_TIME_STAMP FatTime,
    _In_ UCHAR TenMilliSeconds
    );

LARGE_INTEGER
FatFatDateToNtTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FAT_DATE FatDate
    );

FAT_TIME_STAMP
FatGetCurrentFatTime (
    _In_ PIRP_CONTEXT IrpContext
    );


//
//  Low level verification routines, implemented in VerfySup.c
//
//  The first routine is called to help process a verify IRP.  Its job is
//  to walk every Fcb/Dcb and mark them as need to be verified.
//
//  The other routines are used by every dispatch routine to verify that
//  an Vcb/Fcb/Dcb is still good.  The routine walks as much of the opened
//  file/directory tree as necessary to make sure that the path is still valid.
//  The function result indicates if the procedure needed to block for I/O.
//  If the structure is bad the procedure raise the error condition
//  STATUS_FILE_INVALID, otherwise they simply return to their caller
//

typedef enum _FAT_VOLUME_STATE {
    VolumeClean,
    VolumeDirty,
    VolumeDirtyWithSurfaceTest
} FAT_VOLUME_STATE, *PFAT_VOLUME_STATE;

VOID
FatMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FCB_CONDITION FcbCondition,
    IN BOOLEAN Recursive
    );

VOID
FatVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );


KDEFERRED_ROUTINE FatCleanVolumeDpc;

VOID
NTAPI
FatCleanVolumeDpc (
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatMarkVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FAT_VOLUME_STATE VolumeState
    );

WORKER_THREAD_ROUTINE FatFspMarkVolumeDirtyWithRecover;

VOID
NTAPI
FatFspMarkVolumeDirtyWithRecover (
    PVOID Parameter
    );

VOID
FatCheckDirtyBit (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
FatQuickVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
FatVerifyOperationIsLegal (
    IN PIRP_CONTEXT IrpContext
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatPerformVerify (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PIRP Irp,
    _In_ PDEVICE_OBJECT Device
    );


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
NTAPI
FatOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
NTAPI
FatPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
#ifdef __REACTOS__
NTAPI
#endif
FatAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatFsdPostRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  Miscellaneous support routines
//

//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

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

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                               \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));      \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                               \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));      \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                               \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src));      \
    }

#define CopyU4char(Dst,Src) {                               \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src));      \
    }

//
//  VOID
//  FatNotifyReportChange (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN ULONG Filter,
//      IN ULONG Action
//      );
//

#define FatNotifyReportChange(I,V,F,FL,A) {                                                         \
    if ((F)->FullFileName.Buffer == NULL) {                                                         \
        FatSetFullFileNameInFcb((I),(F));                                                           \
    }                                                                                               \
    NT_ASSERT( (F)->FullFileName.Length != 0 );                                                     \
    NT_ASSERT( (F)->FinalNameLength != 0 );                                                         \
    NT_ASSERT( (F)->FullFileName.Length > (F)->FinalNameLength );                                   \
    NT_ASSERT( (F)->FullFileName.Buffer[((F)->FullFileName.Length - (F)->FinalNameLength)/sizeof(WCHAR) - 1] == L'\\' ); \
    FsRtlNotifyFullReportChange( (V)->NotifySync,                                                   \
                                 &(V)->DirNotifyList,                                               \
                                 (PSTRING)&(F)->FullFileName,                                       \
                                 (USHORT) ((F)->FullFileName.Length -                               \
                                           (F)->FinalNameLength),                                   \
                                 (PSTRING)NULL,                                                     \
                                 (PSTRING)NULL,                                                     \
                                 (ULONG)FL,                                                         \
                                 (ULONG)A,                                                          \
                                 (PVOID)NULL );                                                     \
}


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect a volume device object, with the exception of the file system
//  control function which can also take a file system device object), and
//  a pointer to the IRP.  They either perform the function at the FSD level
//  or post the request to the FSP work queue for FSP level processing.
//

_Function_class_(IRP_MJ_CLEANUP)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdCleanup (                         //  implemented in Cleanup.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_CLOSE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdClose (                           //  implemented in Close.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_CREATE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdCreate (                          //  implemented in Create.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );


_Function_class_(IRP_MJ_DEVICE_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdDeviceControl (                   //  implemented in DevCtrl.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_DIRECTORY_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdDirectoryControl (                //  implemented in DirCtrl.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_QUERY_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdQueryEa (                         //  implemented in Ea.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_SET_EA)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdSetEa (                           //  implemented in Ea.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_QUERY_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdQueryInformation (                //  implemented in FileInfo.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_SET_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdSetInformation (                  //  implemented in FileInfo.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_FLUSH_BUFFERS)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdFlushBuffers (                    //  implemented in Flush.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_FILE_SYSTEM_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdFileSystemControl (               //  implemented in FsCtrl.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_LOCK_CONTROL)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdLockControl (                     //  implemented in LockCtrl.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_PNP)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdPnp (                            //  implemented in Pnp.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_READ)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdRead (                            //  implemented in Read.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_SHUTDOWN)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdShutdown (                        //  implemented in Shutdown.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_QUERY_VOLUME_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdQueryVolumeInformation (          //  implemented in VolInfo.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_SET_VOLUME_INFORMATION)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdSetVolumeInformation (            //  implemented in VolInfo.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

_Function_class_(IRP_MJ_WRITE)
_Function_class_(DRIVER_DISPATCH)
NTSTATUS
NTAPI
FatFsdWrite (                           //  implemented in Write.c
    _In_ PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    _Inout_ PIRP Irp
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(IRP) IoIsOperationSynchronous(Irp)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//


WORKER_THREAD_ROUTINE FatFspDispatch;

VOID
NTAPI
FatFspDispatch (                        //  implemented in FspDisp.c
    _In_ PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding FatFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonCleanup (                      //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonClose (                        //  implemented in Close.c
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN PCCB Ccb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN Wait,
    IN BOOLEAN TopLevel,
    OUT PBOOLEAN VcbDeleted OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatFspClose (                           //  implemented in Close.c
    IN PVCB Vcb OPTIONAL
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonCreate (                       //  implemented in Create.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonCreateOnNewStack (        //  implemented in Create.c
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
VOID
FatCommonCreateCallout (              //  implemented in Create.c
    _In_ PFAT_CALLOUT_PARAMETERS CalloutParameters
    );

#endif

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonDirectoryControl (             //  implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonDeviceControl (                //  implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatCommonQueryEa (                      //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatCommonSetEa (                        //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonQueryInformation (             //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonSetInformation (               //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonFlushBuffers (                 //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonFileSystemControl (            //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonLockControl (                  //  implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonPnp (                          //  implemented in Pnp.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonRead (                         //  implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonShutdown (                     //  implemented in Shutdown.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonQueryVolumeInfo (              //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonSetVolumeInfo (                //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatCommonWrite (                        //  implemented in Write.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  The following is implemented in Flush.c, and does what is says.
//

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatFlushFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FAT_FLUSH_TYPE FlushType
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatFlushDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN FAT_FLUSH_TYPE FlushType
    );

NTSTATUS
FatFlushFat (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatFlushVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FAT_FLUSH_TYPE FlushType
    );

NTSTATUS
FatHijackIrpAndFlushDevice (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

VOID
FatFlushFatEntries (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Cluster,
    IN ULONG Count
);

VOID
FatFlushDirentForFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
);



//
//  The following procedure is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this macro allows either the Irp or the IrpContext to be
//  null, however the only legal order to do this in is:
//
//      FatCompleteRequest( NULL, Irp, Status );  // completes Irp & preserves context
//      ...
//      FatCompleteRequest( IrpContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" IrpContext off to
//  the Fsp for post processing, such as read ahead.
//

VOID
FatCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#define FatCompleteRequest(IRPCONTEXT,IRP,STATUS) { \
    FatCompleteRequest_Real(IRPCONTEXT,IRP,STATUS); \
}

BOOLEAN
FatIsIrpTopLevel (
    IN PIRP Irp
    );

//
//  The Following routine makes a popup
//

_Requires_lock_held_(_Global_critical_region_)
VOID
FatPopUpFileCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

//
//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//
_Function_class_(FAST_IO_CHECK_IF_POSSIBLE)
BOOLEAN
NTAPI
FatFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_QUERY_BASIC_INFO)
BOOLEAN
NTAPI
FatFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_QUERY_STANDARD_INFO)
BOOLEAN
NTAPI
FatFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_QUERY_NETWORK_OPEN_INFO)
BOOLEAN
NTAPI
FatFastQueryNetworkOpenInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_LOCK)
BOOLEAN
NTAPI
FatFastLock (
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

_Function_class_(FAST_IO_UNLOCK_SINGLE)
BOOLEAN
NTAPI
FatFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_UNLOCK_ALL)
BOOLEAN
NTAPI
FatFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

_Function_class_(FAST_IO_UNLOCK_ALL_BY_KEY)
BOOLEAN
NTAPI
FatFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );


VOID
FatExamineFatEntries(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG StartIndex OPTIONAL,
    IN ULONG EndIndex OPTIONAL,
    IN BOOLEAN SetupWindows,
    IN PFAT_WINDOW SwitchToWindow OPTIONAL,
    IN PULONG BitMapBuffer OPTIONAL
    );

BOOLEAN
FatScanForDataTrack(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

//
//  The following macro is used to determine is a file has been deleted.
//
//      BOOLEAN
//      IsFileDeleted (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb
//          );
//

#define IsFileDeleted(IRPCONTEXT,FCB)                      \
    (FlagOn((FCB)->FcbState, FCB_STATE_DELETE_ON_CLOSE) && \
     ((FCB)->UncleanCount == 0))

//
//  The following macro is used by the dispatch routines to determine if
//  an operation is to be done with or without Write Through.
//
//      BOOLEAN
//      IsFileWriteThrough (
//          IN PFILE_OBJECT FileObject,
//          IN PVCB Vcb
//          );
//

#define IsFileWriteThrough(FO,VCB) (             \
    BooleanFlagOn((FO)->Flags, FO_WRITE_THROUGH) \
)

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the nonpaged fcb.  It checks that this is actually
//  an FCB (as opposed to a DCB) so that directory oplock code works properly.
//
//
//      BOOLEAN
//      FatIsFastIoPossible (
//          IN PFCB Fcb
//          );
//


#define FatIsFastIoPossible(FCB) ((BOOLEAN)                                     \
    ((((FCB)->FcbCondition != FcbGood) ||                                       \
      (NodeType( (FCB) ) != FAT_NTC_FCB) ||                                     \
      !FsRtlOplockIsFastIoPossible( FatGetFcbOplock(FCB) )) ?                   \
        FastIoIsNotPossible                                                     \
    :                                                                           \
        (!FsRtlAreThereCurrentFileLocks( &(FCB)->Specific.Fcb.FileLock ) &&     \
         ((FCB)->NonPaged->OutstandingAsyncWrites == 0) &&                      \
         !FlagOn( (FCB)->Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED ) ?      \
            FastIoIsPossible                                                    \
        :                                                                       \
            FastIoIsQuestionable                                                \
        )                                                                       \
    )                                                                           \
)


#if (NTDDI_VERSION >= NTDDI_WIN8)

//
//  Detect whether this file can have an oplock on it.  As of Windows 8 file and
//  directories can have oplocks.
//

#define FatIsNodeTypeOplockable(N) (    \
    ((N) == FAT_NTC_FCB) ||             \
    ((N) == FAT_NTC_ROOT_DCB) ||        \
    ((N) == FAT_NTC_DCB)                \
)

#else

#define FatIsNodeTypeOplockable(N) (    \
    ((N) == FAT_NTC_FCB)                \
)

#endif

#define FatIsFileOplockable(F) (                \
    FatIsNodeTypeOplockable( NodeType( (F) ))   \
)

//
//  The following macro is used to detemine if the file object is opened
//  for read only access (i.e., it is not also opened for write access or
//  delete access).
//
//      BOOLEAN
//      IsFileObjectReadOnly (
//          IN PFILE_OBJECT FileObject
//          );
//

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))


//
//  The following two macro are used by the Fsd/Fsp exception handlers to
//  process an exception.  The first macro is the exception filter used in the
//  Fsd/Fsp to decide if an exception should be handled at this level.
//  The second macro decides if the exception is to be finished off by
//  completing the IRP, and cleaning up the Irp Context, or if we should
//  bugcheck.  Exception values such as STATUS_FILE_INVALID (raised by
//  VerfySup.c) cause us to complete the Irp and cleanup, while exceptions
//  such as accvio cause us to bugcheck.
//
//  The basic structure for fsd/fsp exception handling is as follows:
//
//  FatFsdXxx(...)
//  {
//      try {
//
//          ...
//
//      } except(FatExceptionFilter( IrpContext, GetExceptionCode() )) {
//
//          Status = FatProcessException( IrpContext, Irp, GetExceptionCode() );
//      }
//
//      Return Status;
//  }
//
//  To explicitly raise an exception that we expect, such as
//  STATUS_FILE_INVALID, use the below macro FatRaiseStatus().  To raise a
//  status from an unknown origin (such as CcFlushCache()), use the macro
//  FatNormalizeAndRaiseStatus.  This will raise the status if it is expected,
//  or raise STATUS_UNEXPECTED_IO_ERROR if it is not.
//
//  If we are vicariously handling exceptions without using FatProcessException(),
//  if there is the possibility that we raised that exception, one *must*
//  reset the IrpContext so a subsequent raise in the course of handling this
//  request that is *not* explicit, i.e. like a pagein error, does not get
//  spoofed into believing that the first raise status is the reason the second
//  occured.  This could have really serious consequences.
//
//  It is an excellent idea to always FatResetExceptionState in these cases.
//
//  Note that when using these two macros, the original status is placed in
//  IrpContext->ExceptionStatus, signaling FatExceptionFilter and
//  FatProcessException that the status we actually raise is by definition
//  expected.
//

ULONG
FatExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

#if DBG
ULONG
FatBugCheckExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionPointer
    );
#endif

_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
FatProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

//
//  VOID
//  FatRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//
//

#if DBG
#ifdef _MSC_VER
#define DebugBreakOnStatus(S) {                                                      \
__pragma(warning(push))                                                              \
__pragma(warning(disable:4127))                                                      \
    if (FatTestRaisedStatus) {                                                       \
        if ((S) == STATUS_DISK_CORRUPT_ERROR || (S) == STATUS_FILE_CORRUPT_ERROR) {  \
__pragma(warning(pop))                                                               \
            DbgPrint( "FAT: Breaking on interesting raised status (0x%08x)\n", (S) );\
            DbgPrint( "FAT: Set FatTestRaisedStatus @ 0x%p to 0 to disable\n",       \
                      &FatTestRaisedStatus );                                        \
            NT_ASSERT(FALSE);                                                        \
        }                                                                            \
    }                                                                                \
}
#else
#define DebugBreakOnStatus(S) {                                                      \
    if (FatTestRaisedStatus) {                                                       \
        if ((S) == STATUS_DISK_CORRUPT_ERROR || (S) == STATUS_FILE_CORRUPT_ERROR) {  \
            DbgPrint( "FAT: Breaking on interesting raised status (0x%08x)\n", (S) );\
            DbgPrint( "FAT: Set FatTestRaisedStatus @ 0x%p to 0 to disable\n",       \
                      &FatTestRaisedStatus );                                        \
            NT_ASSERT(FALSE);                                                        \
        }                                                                            \
    }                                                                                \
}
#endif
#else
#define DebugBreakOnStatus(S)
#endif

#define FatRaiseStatus(IRPCONTEXT,STATUS) {             \
    (IRPCONTEXT)->ExceptionStatus = (STATUS);           \
    DebugBreakOnStatus( (STATUS) )                      \
    ExRaiseStatus( (STATUS) );                          \
}

#define FatResetExceptionState( IRPCONTEXT ) {          \
    (IRPCONTEXT)->ExceptionStatus = STATUS_SUCCESS;     \
}

//
//  VOID
//  FatNormalAndRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//

#define FatNormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                         \
    (IRPCONTEXT)->ExceptionStatus = (STATUS);                                   \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
}


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
#define try_leave(S) { S; _SEH2_LEAVE; }


CLUSTER_TYPE
FatInterpretClusterType (
    IN PVCB Vcb,
    IN FAT_ENTRY Entry
    );


//
//  These routines define the FileId for FAT.  Lacking a fixed/uniquifiable
//  notion, we simply come up with one which is unique in a given snapshot
//  of the volume.  As long as the parent directory is not moved or compacted,
//  it may even be permanent.
//

//
//  The internal information used to identify the fcb/dcb on the
//  volume is the byte offset of the dirent of the file on disc.
//  Our root always has fileid 0.  FAT32 roots are chains and can
//  use the LBO of the cluster, 12/16 roots use the lbo in the Vcb.
//

#define FatGenerateFileIdFromDirentOffset(ParentDcb,DirentOffset)                                   \
    ((ParentDcb) ? ((NodeType(ParentDcb) != FAT_NTC_ROOT_DCB || FatIsFat32((ParentDcb)->Vcb)) ?     \
                  FatGetLboFromIndex( (ParentDcb)->Vcb,                                             \
                                      (ParentDcb)->FirstClusterOfFile ) :                           \
                  (ParentDcb)->Vcb->AllocationSupport.RootDirectoryLbo) +                           \
                 (DirentOffset)                                                                     \
                  :                                                                                 \
                 0)

//
//

#define FatGenerateFileIdFromFcb(Fcb)                                                               \
        FatGenerateFileIdFromDirentOffset( (Fcb)->ParentDcb, (Fcb)->DirentOffsetWithinDirectory )

//
//  Wrap to handle the ./.. cases appropriately.  Note that we commute NULL parent to 0. This would
//  only occur in an illegal root ".." entry.
//

#define FATDOT    ((ULONG)0x2020202E)
#define FATDOTDOT ((ULONG)0x20202E2E)

#define FatGenerateFileIdFromDirentAndOffset(Dcb,Dirent,DirentOffset)                               \
    ((*((PULONG)(Dirent)->FileName)) == FATDOT ? FatGenerateFileIdFromFcb(Dcb) :                    \
     ((*((PULONG)(Dirent)->FileName)) == FATDOTDOT ? ((Dcb)->ParentDcb ?                            \
                                                       FatGenerateFileIdFromFcb((Dcb)->ParentDcb) : \
                                                       0) :                                         \
      FatGenerateFileIdFromDirentOffset(Dcb,DirentOffset)))


//
//  BOOLEAN
//  FatDeviceIsFatFsdo(
//      IN PDEVICE_OBJECT D
//      );
//
//  Evaluates to TRUE if the supplied device object is one of the file system devices
//  we created at initialisation.
//

#define FatDeviceIsFatFsdo( D)  (((D) == FatData.DiskFileSystemDeviceObject) || ((D) == FatData.CdromFileSystemDeviceObject))


//
//  BlockAlign(): Aligns P on the next V boundary.
//  BlockAlignTruncate(): Aligns P on the prev V boundary.
//

#define BlockAlign(P,V) ((ASSERT( V != 0)), (((P)) + (V-1) & (0-(V))))
#define BlockAlignTruncate(P,V) ((P) & (0-(V)))

#define IsDirectory(FcbOrDcb) ((NodeType((FcbOrDcb)) == FAT_NTC_DCB) || (NodeType((FcbOrDcb)) == FAT_NTC_ROOT_DCB))

#endif // _FATPROCS_


