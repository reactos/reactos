/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    UdfProcs.h

Abstract:

    This module defines all of the globally used procedures in the Udfs
    file system.

Author:

    Dan Lovinger    [DanLo]   29-May-1996

Revision History:

--*/

#ifndef _UDFPROCS_
#define _UDFPROCS_

#include <ntifs.h>

#include <ntddcdrm.h>
#include <ntddcdvd.h>
#include <ntdddisk.h>

#ifndef INLINE
#define INLINE __inline
#endif

#include "nodetype.h"
#include "Udf.h"
#include "UdfStruc.h"
#include "UdfData.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_STRUCSUP)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_STRUCSUP)


//
//  Miscellaneous support routines/macros
//

//
//  Yet another declaration of Min/Max
//

#define Min(a, b)   ((a) < (b) ? (a) : (b))
#define Max(a, b)   ((a) > (b) ? (a) : (b))

//
//  Yet another declaration of the basic bit fiddlers
//

#define FlagMask(F,SF) (                \
    ((F) & (SF))                        \
)

#define BooleanFlagOn(F,SF) (           \
    (BOOLEAN)(FlagOn(F, SF) != 0)       \
)

#define BooleanFlagOff(F,SF) (          \
    (BOOLEAN)(FlagOn(F, SF)) == 0)      \
)

#define SetFlag(Flags,SingleFlag) (     \
    (Flags) |= (SingleFlag)             \
)

#define ClearFlag(Flags,SingleFlag) (   \
    (Flags) &= ~(SingleFlag)            \
)

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

#define Add2Ptr(PTR,INC,CAST) ((CAST)((ULONG_PTR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG)(OFFSET) - (ULONG)(BASE)))

//
//  Generic truncation/align/offset/remainder macros for power-of-two units.
//
//  The offset and remainder functions range from zero to (unit - 1).  The
//  re-offset in the remainder performs this work.
//

#define GenericTruncate(B, U) (                                             \
    (B) & ~((U) - 1)                                                        \
)

#define GenericAlign(B, U) (                                                \
    GenericTruncate((B) + (U) - 1, U)                                       \
)

#define GenericOffset(B, U) (                                               \
    (B) & ((U) - 1)                                                         \
)

#define GenericRemainder(B, U) (                                            \
    GenericOffset( (U) - GenericOffset((B), (U)), (U) )                     \
)


#define GenericTruncatePtr(B, U) (                                          \
    (PVOID)(((ULONG_PTR)(B)) & ~((U) - 1))                                  \
)

#define GenericAlignPtr(B, U) (                                             \
    GenericTruncatePtr((B) + (U) - 1, (U))                                  \
)

#define GenericOffsetPtr(B, U) (                                            \
    (ULONG)(((ULONG_PTR)(B)) & ((U) - 1))                                   \
)

#define GenericRemainderPtr(B, U) (                                         \
    (ULONG)GenericOffset( (U) - GenericOffsetPtr((B), (U)), (U) )           \
)

//
//  Useful compositions of the defaults for common types.
//

#define WordAlign(B) GenericAlign((B), 2)

#define LongAlign(B) GenericAlign((B), 4)

#define QuadAlign(B) GenericAlign((B), 8)


#define WordOffset(B) GenericOffset((B), 2)

#define LongOffset(B) GenericOffset((B), 4)

#define QuadOffset(B) GenericOffset((B), 8)


#define WordAlignPtr(P) GenericAlignPtr((P), 2)

#define LongAlignPtr(P) GenericAlignPtr((P), 4)

#define QuadAlignPtr(P) GenericAlignPtr((P), 8)


#define WordOffsetPtr(P) GenericOffsetPtr((P), 2)

#define LongOffsetPtr(P) GenericOffsetPtr((P), 4)

#define QuadOffsetPtr(P) GenericOffsetPtr((P), 8)


//
//  Macros to round up and down on sector and logical block boundaries.  Although
//  UDF 1.01 specifies that a physical sector is the logical block size we will
//  be general and treat sectors and logical blocks as distinct.  Since UDF may
//  at some point relax the restriction, these definitions will be the only
//  acknowledgement outside of the mount path (which merely checks the volume's
//  conformance).
//

//
//  Sector
//

#define SectorAlignN(SECTORSIZE, L) (                                           \
    ((((ULONG)(L)) + ((SECTORSIZE) - 1)) & ~((SECTORSIZE) - 1))                 \
)

#define SectorAlign(V, L) (                                                     \
    ((((ULONG)(L)) + (((V)->SectorSize) - 1)) & ~(((V)->SectorSize) - 1))       \
)

#define LlSectorAlign(V, L) (                                                   \
    ((((LONGLONG)(L)) + (((V)->SectorSize) - 1)) & ~(((LONGLONG)(V)->SectorSize) - 1)) \
)

#define SectorTruncate(V, L) (                                                  \
    ((ULONG)(L)) & ~(((V)->SectorSize) - 1)                                     \
)

#define LlSectorTruncate(V, L) (                                                \
    ((LONGLONG)(L)) & ~(((LONGLONG)(V)->SectorSize) - 1)                        \
)

#define BytesFromSectors(V, L) (                                                \
    ((ULONG) (L)) << ((V)->SectorShift)                                         \
)

#define SectorsFromBytes(V, L) (                                                \
    ((ULONG) (L)) >> ((V)->SectorShift)                                         \
)

#define LlBytesFromSectors(V, L) (                                              \
    Int64ShllMod32( (ULONGLONG)(L), ((V)->SectorShift) )                        \
)

#define LlSectorsFromBytes(V, L) (                                              \
    Int64ShrlMod32( (ULONGLONG)(L), ((V)->SectorShift) )                        \
)

#define SectorsFromBlocks(V, B) (B)

#define SectorSize(V) ((V)->SectorSize)

#define SectorOffset(V, L) (                                                    \
    ((ULONG) (L)) & (((V)->SectorSize) - 1)                                     \
)

//
//  Logical Block
//

#define BlockAlignN(BLOCKSIZE, L) (                                             \
    SectorAlighN((BLOCKSIZE), (L))                                              \
)

#define BlockAlign(V, L) (                                                      \
    SectorAlign((V), (L))                                                       \
)

#define LlBlockAlign(V, L) (                                                    \
    LlSectorAlign((V), (L))                                                     \
)

#define BlockTruncate(V, L) (                                                   \
    SectorTruncate((V), (L))                                                    \
)

#define LlBlockTruncate(V, L) (                                                 \
    LlSectorTruncate((V), (L))                                                  \
)

#define BytesFromBlocks(V, L) (                                                 \
    BytesFromSectors((V), (L))                                                  \
)

#define BlocksFromBytes(V, L) (                                                 \
    SectorsFromBytes((V), (L))                                                  \
)

#define LlBytesFromBlocks(V, L) (                                               \
    LlBytesFromSectors((V), (L))                                                \
)

#define LlBlocksFromBytes(V, L) (                                               \
    LlSectorsFromBytes((V), (L))                                                \
)

#define BlocksFromSectors(V, S) (S)

#define BlockSize(V) (SectorSize(V))

#define BlockOffset(V, L) (                                                     \
    SectorOffset((V), (L))                                                      \
)

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in various structures.
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
//  This macro copies an unaligned src word to a dst word,
//  performing an little/big endian swap.
//

#define SwapCopyUchar2(Dst,Src) {                                       \
    *((UNALIGNED UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src) + 1);    \
    *((UNALIGNED UCHAR1 *)(Dst) + 1) = *((UNALIGNED UCHAR1 *)(Src));    \
}

//
//  This macro copies an unaligned src longword to an aligned dst longword
//

#define CopyUchar4(Dst,Src) {                           \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src));  \
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

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//  accessing the source on a word boundary.
//

#define CopyUshort2(Dst,Src) {                          \
    *((USHORT2 *)(Dst)) = *((UNALIGNED USHORT2 *)(Src));\
    }

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

#define UdfIsFastIoPossible(F) ((BOOLEAN)                                           \
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
//  The following macros encapsulate the common work of raising exceptions while storing
//  the exception in the IrpContext.
//

INLINE
DECLSPEC_NORETURN
VOID
UdfRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status
    )
{
    IrpContext->ExceptionStatus = Status;
    DebugBreakOnStatus( Status );
    ExRaiseStatus( Status );
}

INLINE
VOID
UdfNormalizeAndRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status
    )
{
    IrpContext->ExceptionStatus = FsRtlNormalizeNtstatus( Status, STATUS_UNEXPECTED_IO_ERROR );
    ExRaiseStatus( IrpContext->ExceptionStatus );
}

//
//  The following is a convenience macro to execute a little code before making
//  a shortcircuit out of a surrounding try-finally clause.  This is usually to
//  set a status value.
//
//  Note that our compilers support the leave keyword now and we don't have to
//  use the old try_exit: labels and goto.
//

#define try_leave(S) { S; leave; }

//
//  For debugging purposes we sometimes want to allocate our structures from nonpaged
//  pool so that in the kernel debugger we can walk all the structures.
//

#define UdfPagedPool                 PagedPool
#define UdfNonPagedPool              NonPagedPool
#define UdfNonPagedPoolCacheAligned  NonPagedPoolCacheAligned

//
//  Encapsulate safe pool freeing
//

INLINE
VOID
UdfFreePool(
    IN PVOID *Pool
    )
{
    if (*Pool != NULL) {

        ExFreePool(*Pool);
        *Pool = NULL;
    }
}

//
//  Encapsulate counted string compares with uncounted fields.  Thanks to a
//  very smart compiler, we have to carefully tell it that no matter what it
//  thinks, it *cannot* do anything other than a bytewise compare.
//

INLINE
BOOLEAN
UdfEqualCountedString(
    IN PSTRING String,
    IN PCHAR Field
    )
{
    return (RtlEqualMemory( (CHAR UNALIGNED *)String->Buffer,
                            (CHAR UNALIGNED *)Field,
                            String->Length )                    != 0);
}


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

} TYPE_OF_OPEN, *PTYPE_OF_OPEN;


//
//  Following routines handle entry in and out of the filesystem.  They are
//  contained in UdfData.c.  We also get some very generic utility functions
//  here that aren't associated with any particular datastructure.
//

NTSTATUS
UdfFsdDispatch (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

LONG
UdfExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
UdfProcessException (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

VOID
UdfCompleteRequest (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
    );

//
//  Following are the routines to handle the top level thread logic.
//

VOID
UdfSetThreadContext (
    IN PIRP_CONTEXT IrpContext,
    IN PTHREAD_CONTEXT ThreadContext
    );

INLINE
VOID
UdfRestoreThreadContext (
     IN PIRP_CONTEXT IrpContext
     )
{
    IrpContext->ThreadContext->Udfs = 0;
    IoSetTopLevelIrp( IrpContext->ThreadContext->SavedTopLevelIrp );
    IrpContext->ThreadContext = NULL;
}

//
//  Following are some generic utility functions we have to carry along for the ride
//

ULONG
UdfSerial32 (
    IN PCHAR Buffer,
    IN ULONG ByteCount
    );

VOID
UdfInitializeCrc16 (
    ULONG Polynomial
    );

USHORT
UdfComputeCrc16 (
	IN PUCHAR Buffer,
	IN ULONG ByteCount
    );

USHORT
UdfComputeCrc16Uni (
    PWCHAR Buffer,
    ULONG CharCount
    );

ULONG
UdfHighBit (
    ULONG Word
    );

//
//  Following are the fast entry points.
//

BOOLEAN
UdfFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UdfFastIoCheckIfPossible (
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
UdfFastLock (
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
UdfFastQueryNetworkInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UdfFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UdfFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UdfFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
UdfFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );


//
//  File access check routine, implemented in AcChkSup.c
//

INLINE
BOOLEAN
UdfIllegalFcbAccess (
    IN PIRP_CONTEXT IrpContext,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine simply asserts that the access is legal for a readonly filesystem.
    
Arguments:

    TypeOfOpen - type of open for the Fcb in question.
    
    DesiredAccess - mask of access the caller is trying for.

Return Value:

    BOOLEAN True if illegal access, false otherwise.

--*/

{
    return BooleanFlagOn( DesiredAccess,
                          (TypeOfOpen != UserVolumeOpen ?
                           (FILE_WRITE_ATTRIBUTES           |
                            FILE_WRITE_DATA                 |
                            FILE_WRITE_EA                   |
                            FILE_ADD_FILE                   |                     
                            FILE_ADD_SUBDIRECTORY           |
                            FILE_APPEND_DATA) : 0)          |
                          FILE_DELETE_CHILD                 |
                          DELETE                            |
                          WRITE_DAC );
}


//
//  Sector lookup routines, implemented in AllocSup.c
//

BOOLEAN
UdfLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG FileOffset,
    OUT PLONGLONG DiskOffset,
    OUT PULONG ByteCount
    );

VOID
UdfDeletePcb (
    IN PPCB Pcb
    );

NTSTATUS
UdfInitializePcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PPCB *Pcb,
    IN PNSR_LVOL LVD
    );

VOID
UdfAddToPcb (
    IN PPCB Pcb,
    IN PNSR_PART PartitionDescriptor
);

NTSTATUS
UdfCompletePcb(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PPCB Pcb );

BOOLEAN
UdfEquivalentPcb (
    IN PIRP_CONTEXT IrpContext,
    IN PPCB Pcb1,
    IN PPCB Pcb2
    );

ULONG
UdfLookupPsnOfExtent (
    IN PIRP_CONTEXT IrpContext,    
    IN PVCB Vcb,
    IN USHORT Reference,
    IN ULONG Lbn,
    IN ULONG Len
    );

ULONG
UdfLookupMetaVsnOfExtent (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN USHORT Reference,
    IN ULONG Lbn,
    IN ULONG Len,
    IN BOOLEAN ExactEnd
    );


//
//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

VOID
UdfCreateInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    );

VOID
UdfDeleteInternalStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );


NTSTATUS
UdfCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
UdfMapMetadataView (
    IN PIRP_CONTEXT IrpContext,
    IN PMAPPED_PVIEW View,
    IN PVCB Vcb,
    IN USHORT Partition,
    IN ULONG Lbn,
    IN ULONG Length
    );

NTSTATUS
UdfPurgeVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DismountUnderway
    );

//  VOID
//  UdfUnpinView (
//      IN PIRP_CONTEXT IrpContext,
//      IN PMAPPED_VIEW View
//      );
//

#define UdfUnpinView(IC,V)   \
    if (((V)->Bcb) != NULL) { CcUnpinData( ((V)->Bcb) ); ((V)->Bcb) = NULL; ((V)->View) = NULL; }

//  VOID
//  UdfUnpinData (
//      IN PIRP_CONTEXT IrpContext,
//      IN OUT PBCB *Bcb
//      );
//

#define UdfUnpinData(IC,B)   \
    if (*(B) != NULL) { CcUnpinData( *(B) ); *(B) = NULL; }


//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device reads and other communcation.
//  They do not affect any data structures.
//

NTSTATUS
UdfPerformDevIoCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT Device,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    );

NTSTATUS
UdfReadSectors (
    IN PIRP_CONTEXT IrpContext,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN ReturnError,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

NTSTATUS
UdfNonCachedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount
    );

NTSTATUS
UdfCreateUserMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BufferLength,
    IN BOOLEAN RaiseOnError
    );

//
//  VOID
//  UdfMapUserBuffer (
//      IN PIRP_CONTEXT IrpContext,
//      OUT PVOID Buffer
//      );
//
//  Will raise on failure.
//
//  VOID
//  UdfLockUserBuffer (
//      IN PIRP_CONTEXT IrpContext,
//      IN ULONG BufferLength
//      );
//

#define UdfMapUserBuffer(IC,UB) {                                                   \
            *(UB) = ((PVOID) (((IC)->Irp->MdlAddress == NULL) ?                     \
                             (IC)->Irp->UserBuffer :                                \
                             MmGetSystemAddressForMdlSafe( (IC)->Irp->MdlAddress, NormalPagePriority )));   \
            if (NULL == *(UB))  {                                                    \
                UdfRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES);         \
            }                                                                       \
        }
        
#define UdfLockUserBuffer(IC,BL) {                   \
    if ((IC)->Irp->MdlAddress == NULL) {             \
        (VOID) UdfCreateUserMdl( (IC), (BL), TRUE ); \
    }                                                \
}

//
//  Udf*RawBufferSize and Udf*RawReadSize calculate how big a buffer must be
//  to do a direct read of a given sector aligned structure (UdfReadSectors)
//  and how much data the read must recover.  Reads must write into whole-page
//  sized buffers and be in whole-sector units.
//
//  Note that although all descriptors are constrained to fit in one logical
//  block, it is not always going to be neccesary to read the entire logical
//  block to get the descriptor.  The underlying restriction is the physical
//  sector.
//

INLINE
ULONG
UdfRawBufferSize (
    IN PVCB Vcb,
    IN ULONG StructureSize
    )
{
    return (ULONG)ROUND_TO_PAGES( SectorAlign( Vcb, StructureSize ));
}

INLINE
ULONG
UdfRawReadSize (
    IN PVCB Vcb,
    IN ULONG StructureSize
    )
{
    return SectorAlign( Vcb, StructureSize );
}

INLINE
ULONG
UdfRawBufferSizeN (
    IN ULONG SectorSize,
    IN ULONG StructureSize
    )
{
    return (ULONG)ROUND_TO_PAGES( SectorAlignN( SectorSize, StructureSize ));
}

INLINE
ULONG
UdfRawReadSizeN (
    IN ULONG SectorSize,
    IN ULONG StructureSize
    )
{
    return SectorAlignN( SectorSize, StructureSize );
}


//
//  The following routines are used to read on-disk directory structures, implemented
//  in DirSup.c
//

VOID
UdfInitializeDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext
    );

VOID
UdfCleanupDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext
    );

BOOLEAN
UdfLookupInitialDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN PLONGLONG InitialOffset OPTIONAL
    );

BOOLEAN
UdfLookupNextDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PDIR_ENUM_CONTEXT DirContext
    );

VOID
UdfUpdateDirNames (
    IN PIRP_CONTEXT IrpContext,
    IN PDIR_ENUM_CONTEXT DirContext,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
UdfFindDirEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN ShortName,
    IN PDIR_ENUM_CONTEXT DirContext
    );


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

VOID
UdfSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PFCB Fcb OPTIONAL,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
UdfDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb,
    OUT PCCB *Ccb
    );

TYPE_OF_OPEN
UdfFastDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PFCB *Fcb
    );


//
//  FSCTL request support routines. Contained in FsCtrl.c
//

VOID
UdfStoreVolumeDescriptorIfPrevailing (
    IN OUT PNSR_VD_GENERIC *StoredVD,
    IN OUT PNSR_VD_GENERIC NewVD
    );


//
//  Name mangling routines.  Implemented in Namesup.c
//

VOID
UdfDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING FinalName
    );

BOOLEAN
UdfIs8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING FileName
    );

BOOLEAN
UdfCandidateShortName (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    );

VOID
UdfGenerate8dot3Name (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    OUT PUNICODE_STRING ShortFileName
    );

VOID
UdfConvertCS0DstringToUnicode (
    IN PIRP_CONTEXT IrpContext,
    IN PUCHAR Dstring,
    IN UCHAR Length OPTIONAL,
    IN UCHAR FieldLength OPTIONAL,
    IN OUT PUNICODE_STRING Name
    );

BOOLEAN
UdfCheckLegalCS0Dstring (
    PIRP_CONTEXT IrpContext,
    PUCHAR Dstring,
    UCHAR Length OPTIONAL,
    UCHAR FieldLength OPTIONAL,
    BOOLEAN ReturnOnError
    );

VOID
UdfRenderNameToLegalUnicode (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING RenderedName
    );

BOOLEAN
UdfIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING CurrentName,
    IN PUNICODE_STRING SearchExpression,
    IN BOOLEAN Wild
    );

FSRTL_COMPARISON_RESULT
UdfFullCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING NameA,
    IN PUNICODE_STRING NameB
    );

INLINE
VOID
UdfUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name,
    IN OUT PUNICODE_STRING UpcaseName
    )

/*++

Routine Description:

    This routine upcases a name with an assertion of success.
    
Arguments:

    Name - an name to upcase
    
    Length - a place to put the upcased name (can be the same as Name)
    
Return Value:

    None.
    
--*/

{
    NTSTATUS Status;

    //
    //  Upcase the string using the correct upcase routine.
    //

    Status = RtlUpcaseUnicodeString( UpcaseName,
                                     Name,
                                     FALSE );

    //
    //  This should never fail.
    //

    ASSERT( Status == STATUS_SUCCESS );

    return;
}

INLINE
USHORT
UdfCS0DstringUnicodeSize (
    PIRP_CONTEXT IrpContext,
    PCHAR Dstring,
    UCHAR Length
    )

/*++

Routine Description:

    This routine computes the number of bytes required for the UNICODE representation
    of a CS0 Dstring (1/7.2.12)
    
Arguments:

    Dstring - a dstring
    
    Length - length of the dstring
    
Return Value:

    ULONG number of bytes.
    
--*/

{
    return (16 / *Dstring) * (Length - 1);
}

INLINE
BOOLEAN
UdfIsCharacterLegal (
    IN WCHAR Character
    )

/*++

Routine Description:

    This routine checks that a given UNICODE character is legal.
    
Arguments:

    Character - a character to check
    
Return Value:

    BOOLEAN True if a legal character, False otherwise.
    
--*/

{
    if (Character < 0xff && !FsRtlIsAnsiCharacterLegalHpfs( Character, FALSE )) {

        return FALSE;
    }

    return TRUE;
}

INLINE
BOOLEAN
UdfCS0DstringContainsLegalCharacters (
    IN PCHAR Dstring,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine inspects a CS0 dstring for illegal characters.  The assumption is
    made that the string is legal CS0.
    
Arguments:

    Name - a name to check
    
Return Value:

    BOOLEAN True if legal characters are found, False otherwise.
    
--*/

{
    ULONG Step;
    WCHAR Char;
    PCHAR Bound = Dstring + Length;

    //
    //  Determine how big a step we take in the string according to the
    //  "compression" applied.
    //
    
    if (*Dstring == 16) {

        Step = sizeof( WCHAR );
    
    } else {

        Step = sizeof( CHAR );
    }

    //
    //  Advance past the compression marker and loop over the string.
    //
    
    for (Dstring++; Dstring < Bound; Dstring += Step) {

        //
        //  Perform the endianess swapcopy to convert from UDF bigendian CS0 to our
        //  little endian wide characters.
        //
        
        SwapCopyUchar2( &Char, Dstring );
        
        if (!UdfIsCharacterLegal( Char )) {

            DebugTrace(( 0, Dbg, "UdfCS0DstringContainsLegalCharacters, Char %04x @ %08x\n", (WCHAR) Char, Dstring ));

            return FALSE;
        }
    }

    return TRUE;
}


//
//  Filesystem control operations.  Implemented in Fsctrl.c
//

NTSTATUS
UdfLockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

NTSTATUS
UdfUnlockVolumeInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );


//
//  Routines to handle the prefix trees attached to directories, used to quickly travel common
//  bits of the hierarchy.  Implemented in PrefxSup.c
//

PLCB
UdfFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *CurrentFcb,
    IN OUT PUNICODE_STRING RemainingName,
    IN BOOLEAN IgnoreCase
    );

VOID            
UdfInitializeLcbFromDirContext (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PDIR_ENUM_CONTEXT DirContext
    );

PLCB
UdfInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING Name,
    IN BOOLEAN ShortNameMatch,
    IN BOOLEAN IgnoreCase,
    IN PFCB ParentFcb
    );

VOID
UdfRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    );


//
//  Synchronization routines.  Implemented in Resrcsup.c
//
//  The following routines/macros are used to synchronize the in-memory structures.
//
//      Routine/Macro               Synchronizes                            Subsequent
//
//      UdfAcquireUdfData           Volume Mounts/Dismounts,Vcb Queue       UdfReleaseUdfData
//      UdfAcquireVcbExclusive      Vcb for open/close                      UdfReleaseVcb
//      UdfAcquireVcbShared         Vcb for open/close                      UdfReleaseVcb
//      UdfAcquireAllFiles          Locks out operations to all files       UdfReleaseAllFiles
//      UdfAcquireFileExclusive     Locks out file operations               UdfReleaseFile
//      UdfAcquireFileShared        Files for file operations               UdfReleaseFile
//      UdfAcquireFcbExclusive      Fcb for open/close                      UdfReleaseFcb
//      UdfAcquireFcbShared         Fcb for open/close                      UdfReleaseFcb
//      UdfLockUdfData              Fields in UdfData                       UdfUnlockUdfData
//      UdfLockVcb                  Vcb fields, FcbReference, FcbTable      UdfUnlockVcb
//      UdfLockFcb                  Fcb fields, prefix table, Mcb           UdfUnlockFcb
//

typedef enum _TYPE_OF_ACQUIRE {
    
    AcquireExclusive,
    AcquireShared,
    AcquireSharedStarveExclusive

} TYPE_OF_ACQUIRE, *PTYPE_OF_ACQUIRE;

BOOLEAN
UdfAcquireResource (
    IN PIRP_CONTEXT IrpContext,
    IN PERESOURCE Resource,
    IN BOOLEAN IgnoreWait,
    IN TYPE_OF_ACQUIRE Type
    );

//
//  BOOLEAN
//  UdfAcquireUdfData (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfReleaseUdfData (
//      IN PIRP_CONTEXT IrpContext
//    );
//
//  BOOLEAN
//  UdfAcquireVcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  UdfAcquireVcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  VOID
//  UdfReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  UdfAcquireAllFiles (
//      IN PIRP_CONTEXT,
//      IN PVCB Vcb
//      );
//
//  VOID
//  UdfReleaseAllFiles (
//      IN PIRP_CONTEXT,
//      IN PVCB Vcb
//      );
//
//  VOID
//  UdfAcquireFileExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      );
//
//  VOID
//  UdfAcquireFileShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfReleaseFile (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//    );
//
//  BOOLEAN
//  UdfAcquireFcbExclusive (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  UdfAcquireFcbShared (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN BOOLEAN IgnoreWait
//      );
//
//  BOOLEAN
//  UdfReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfLockUdfData (
//      );
//
//  VOID
//  UdfUnlockUdfData (
//      );
//
//  VOID
//  UdfLockVcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfUnlockVcb (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfLockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfUnlockFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define UdfAcquireUdfData(IC)                                                           \
    ExAcquireResourceExclusive( &UdfData.DataResource, TRUE )

#define UdfReleaseUdfData(IC)                                                           \
    ExReleaseResource( &UdfData.DataResource )

#define UdfAcquireVcbExclusive(IC,V,I)                                                  \
    UdfAcquireResource( (IC), &(V)->VcbResource, (I), AcquireExclusive )

#define UdfAcquireVcbShared(IC,V,I)                                                     \
    UdfAcquireResource( (IC), &(V)->VcbResource, (I), AcquireShared )

#define UdfReleaseVcb(IC,V)                                                             \
    ExReleaseResource( &(V)->VcbResource )

#define UdfAcquireAllFiles(IC,V)                                                        \
    UdfAcquireResource( (IC), &(V)->FileResource, FALSE, AcquireExclusive )

#define UdfReleaseAllFiles(IC,V)                                                        \
    ExReleaseResource( &(V)->FileResource )

#define UdfAcquireFileExclusive(IC,F)                                                   \
    UdfAcquireResource( (IC), (F)->Resource, FALSE, AcquireExclusive )

#define UdfAcquireFileShared(IC,F)                                                      \
    UdfAcquireResource( (IC), (F)->Resource, FALSE, AcquireShared )

#define UdfAcquireFileSharedStarveExclusive(IC,F)                                       \
    UdfAcquireResource( (IC), (F)->Resource, FALSE, AcquireSharedStarveExclusive )

#define UdfReleaseFile(IC,F)                                                            \
    ExReleaseResource( (F)->Resource )

#define UdfAcquireFcbExclusive(IC,F,I)                                                  \
    UdfAcquireResource( (IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireExclusive )

#define UdfAcquireFcbShared(IC,F,I)                                                     \
    UdfAcquireResource( (IC), &(F)->FcbNonpaged->FcbResource, (I), AcquireShared )

#define UdfReleaseFcb(IC,F)                                                             \
    ExReleaseResource( &(F)->FcbNonpaged->FcbResource )

#define UdfLockUdfData()                                                                \
    ExAcquireFastMutex( &UdfData.UdfDataMutex );                                        \
    UdfData.UdfDataLockThread = PsGetCurrentThread()

#define UdfUnlockUdfData()                                                              \
    UdfData.UdfDataLockThread = NULL;                                                   \
    ExReleaseFastMutex( &UdfData.UdfDataMutex )

#define UdfLockVcb(IC,V)                                                                \
    ExAcquireFastMutex( &(V)->VcbMutex );                                               \
    (V)->VcbLockThread = PsGetCurrentThread()

#define UdfUnlockVcb(IC,V)                                                              \
    (V)->VcbLockThread = NULL;                                                          \
    ExReleaseFastMutex( &(V)->VcbMutex )

#define UdfLockFcb(IC,F) {                                                              \
    PVOID _CurrentThread = PsGetCurrentThread();                                        \
    if (_CurrentThread != (F)->FcbLockThread) {                                         \
        ExAcquireFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
        ASSERT( (F)->FcbLockCount == 0 );                                               \
        (F)->FcbLockThread = _CurrentThread;                                            \
    }                                                                                   \
    (F)->FcbLockCount += 1;                                                             \
}

#define UdfUnlockFcb(IC,F) {                                                            \
    (F)->FcbLockCount -= 1;                                                             \
    if ((F)->FcbLockCount == 0) {                                                       \
        (F)->FcbLockThread = NULL;                                                      \
        ExReleaseFastMutex( &(F)->FcbNonpaged->FcbMutex );                              \
    }                                                                                   \
}

BOOLEAN
UdfNoopAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
UdfNoopRelease (
    IN PVOID Fcb
    );

BOOLEAN
UdfAcquireForCache (
    IN PFCB Fcb,
    IN BOOLEAN Wait
    );

VOID
UdfReleaseFromCache (
    IN PFCB Fcb
    );

VOID
UdfAcquireForCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
UdfReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
    );


//
//  Structure support routines, implemented in StrucSup.c
//
//  These routines perform in-memory structure manipulations. They do *not* operate
//  on disk structures.
//

BOOLEAN
UdfInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDISK_GEOMETRY DiskGeometry,
    IN ULONG MediaChangeCount
    );

VOID
UdfUpdateVcbPhase0 (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

VOID
UdfUpdateVcbPhase1 (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PNSR_FSD Fsd
    );

VOID
UdfDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

PIRP_CONTEXT
UdfCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
UdfCleanupIrpContext (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Post
    );

VOID
UdfInitializeStackIrpContext (
    OUT PIRP_CONTEXT IrpContext,
    IN PIRP_CONTEXT_LITE IrpContextLite
    );

//
//  PIRP_CONTEXT_LITE
//  UdfCreateIrpContextLite (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  UdfFreeIrpContextLite (
//      IN PIRP_CONTEXT_LITE IrpContextLite
//      );
//

#define UdfCreateIrpContextLite(IC)  \
    ExAllocatePoolWithTag( UdfNonPagedPool, sizeof( IRP_CONTEXT_LITE ), TAG_IRP_CONTEXT_LITE )

#define UdfFreeIrpContextLite(ICL)  \
    ExFreePool( ICL )

//
//  PUDF_IO_CONTEXT
//  UdfAllocateIoContext (
//      );
//
//  VOID
//  UdfFreeIoContext (
//      PUDF_IO_CONTEXT IoContext
//      );
//

#define UdfAllocateIoContext()                           \
    FsRtlAllocatePoolWithTag( UdfNonPagedPool,           \
                              sizeof( UDF_IO_CONTEXT ),  \
                              TAG_IO_CONTEXT )

#define UdfFreeIoContext(IO)     ExFreePool( IO )

//
//  VOID
//  UdfIncrementCleanupCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfDecrementCleanupCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfIncrementReferenceCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ULONG ReferenceCount
//      IN ULONG UserReferenceCount
//      );
//
//  VOID
//  UdfDecrementReferenceCounts (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN ULONG ReferenceCount
//      IN ULONG UserReferenceCount
//      );
//
//  VOID
//  UdfIncrementFcbReference (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//
//  VOID
//  UdfDecrementFcbReference (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define UdfIncrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup += 1;                       \
    (F)->Vcb->VcbCleanup += 1;                  \
}

#define UdfDecrementCleanupCounts(IC,F) {        \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbCleanup -= 1;                       \
    (F)->Vcb->VcbCleanup -= 1;                  \
}

#define UdfIncrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference += (C);                   \
    (F)->FcbUserReference += (UC);              \
    (F)->Vcb->VcbReference += (C);              \
    (F)->Vcb->VcbUserReference += (UC);         \
}

#define UdfDecrementReferenceCounts(IC,F,C,UC) { \
    ASSERT_LOCKED_VCB( (F)->Vcb );              \
    (F)->FcbReference -= (C);                   \
    (F)->FcbUserReference -= (UC);              \
    (F)->Vcb->VcbReference -= (C);              \
    (F)->Vcb->VcbUserReference -= (UC);         \
}

VOID
UdfTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB StartingFcb,
    IN BOOLEAN Recursive,
    OUT PBOOLEAN RemovedStartingFcb
    );

PFCB
UdfLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_ID FileId
    );

PFCB
UdfGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID *RestartKey
    );

PFCB
UdfCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN FILE_ID FileId,
    IN NODE_TYPE_CODE NodeTypeCode,
    OUT PBOOLEAN FcbExisted OPTIONAL
    );

VOID
UdfDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
UdfInitializeFcbFromIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PICB_SEARCH_CONTEXT IcbContext
    );

PCCB
UdfCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG Flags
    );

VOID
UdfDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

ULONG
UdfFindInParseTable (
    IN PPARSE_KEYVALUE ParseTable,
    IN PCHAR Id,
    IN ULONG MaxIdLen
    );

BOOLEAN
UdfVerifyDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PDESTAG Descriptor,
    IN USHORT Tag,
    IN ULONG Size,
    IN ULONG Lbn,
    IN BOOLEAN ReturnError
    );

VOID
UdfInitializeIcbContextFromFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PFCB Fcb
    );

VOID
UdfInitializeIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PVCB Vcb,
    IN USHORT IcbType,
    IN USHORT Partition,
    IN ULONG Lbn,
    IN ULONG Length
    );

INLINE
VOID
UdfFastInitializeIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext
    )
{

    RtlZeroMemory( IcbContext, sizeof( ICB_SEARCH_CONTEXT ));
}

VOID
UdfLookupActiveIcb (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext
    );


VOID
UdfCleanupIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext
    );

VOID
UdfInitializeAllocations (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PICB_SEARCH_CONTEXT IcbContext
    );

VOID
UdfUpdateTimestampsFromIcbContext (
    IN PIRP_CONTEXT IrpContext,
    IN PICB_SEARCH_CONTEXT IcbContext,
    IN PTIMESTAMP_BUNDLE Timestamps
    );

BOOLEAN
UdfCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb,
    IN BOOLEAN RaiseOnError
    );

//
//  The following macro converts from UDF time to NT time.
//

INLINE
VOID
UdfConvertUdfTimeToNtTime (
    IN PIRP_CONTEXT IrpContext,
    IN PTIMESTAMP UdfTime,
    OUT PLARGE_INTEGER NtTime
    )
{
    TIME_FIELDS TimeField;
    
    TimeField.Year = UdfTime->Year;
    TimeField.Month = UdfTime->Month;
    TimeField.Day = UdfTime->Day;
    TimeField.Hour = UdfTime->Hour;
    TimeField.Minute = UdfTime->Minute;
    TimeField.Second = UdfTime->Second;
    
    //
    //  This is where it gets hairy.  For some unholy reason, ISO 13346 timestamps
    //  carve the right of the decimal point up into three fields of precision
    //  10-2, 10-4, and 10-6, each ranging from 0-99. Lawdy.
    //
    //  To make it easier, since they cannot cause a wrap into the next second,
    //  just save it all up and add it in after the conversion.
    //
    
    TimeField.Milliseconds = 0; 
    
    if (UdfTime->Type <= 1 &&
        ((UdfTime->Zone >= TIMESTAMP_Z_MIN && UdfTime->Zone <= TIMESTAMP_Z_MAX) ||
         UdfTime->Zone == TIMESTAMP_Z_NONE) &&
        RtlTimeFieldsToTime( &TimeField, NtTime )) {

        //
        //  Now fold in the remaining sub-second "precision".  Read as coversions
        //  through the 10-3 units, then into our 10-7 base. (centi->milli->micro,
        //  etc).
        //
    
        NtTime->QuadPart += ((UdfTime->CentiSecond * (10 * 1000)) +
                             (UdfTime->Usec100 * 100) +
                             UdfTime->Usec) * 10;

        //
        //  Perform TZ normalization if this is a local time with
        //  specified timezone.
        //

        if (UdfTime->Type == 1 && UdfTime->Zone != TIMESTAMP_Z_NONE) {
            
            NtTime->QuadPart += Int32x32To64( -UdfTime->Zone, (60 * 10 * 1000 * 1000) );
        }
    
    } else {

        //
        //  Epoch.  Malformed timestamp.
        //

        NtTime->QuadPart = 0;
    }
}

//
//  An equivalence test for Entity IDs.
//

INLINE
BOOLEAN
UdfEqualEntityId (
    IN PREGID RegID,
    IN PSTRING Id,
    IN OPTIONAL PSTRING Suffix
    )
{

    return (UdfEqualCountedString( Id, RegID->Identifier ) &&

#ifndef UDF_SUPPORT_NONSTANDARD_ENTITY_STRINGTERM
            
            //
            //  Allow disabling of the check that the identifier
            //  seems to be padded with zero.
            //
            //  Reason: a couple samples that are otherwise useful
            //      padded some identifiers with junk.
            //

            ((Id->Length == sizeof(RegID->Identifier) ||
              RegID->Identifier[Id->Length] == '\0') ||
             
             !DebugTrace(( 0, Dbg,
                           "UdfEqualEntityId, RegID seems to be terminated with junk!\n" ))) &&
#endif

            ((Suffix == NULL) || UdfEqualCountedString( Suffix, RegID->Suffix )));
}

//
//  A Domain Identifier RegID is considered to be contained if the
//  text string identifier matches and the revision is less than or
//  equal.  This is the convenient way to check that a Domain ID
//  indicates a set of structures will be intelligible to a given
//  implementation level.
//

INLINE
BOOLEAN
UdfDomainIdentifierContained (
    IN PREGID RegID,
    IN PSTRING Domain,
    IN USHORT RevisionMin,
    IN USHORT RevisionMax
    )
{
    PUDF_SUFFIX_DOMAIN DomainSuffix = (PUDF_SUFFIX_DOMAIN) RegID->Suffix;

#ifdef UDF_SUPPORT_NONSTANDARD_ALLSTOR
    
    //
    //  Disable checking of the UDF revision.
    //
    //  Reason: first drop of Allstor media recorded the version number as
    //      a decimal number, not hex (102 = 0x66)
    //

    return (UdfEqualEntityId( RegID, Domain, NULL ));

#else

    return ((DomainSuffix->UdfRevision <= RevisionMax && DomainSuffix->UdfRevision >= RevisionMin) &&
            UdfEqualEntityId( RegID, Domain, NULL ));
#endif
}

//
//  In like fashion, we define containment for a UDF Identifier RegID.
//

INLINE
BOOLEAN
UdfUdfIdentifierContained (
    IN PREGID RegID,
    IN PSTRING Type,
    IN USHORT RevisionMin,
    IN USHORT RevisionMax,
    IN UCHAR OSClass,
    IN UCHAR OSIdentifier
    )
{
    PUDF_SUFFIX_UDF UdfSuffix = (PUDF_SUFFIX_UDF) RegID->Suffix;

    return ((UdfSuffix->UdfRevision <= RevisionMax && UdfSuffix->UdfRevision >= RevisionMin) &&
            (OSClass == OSCLASS_INVALID || UdfSuffix->OSClass == OSClass) &&
            (OSIdentifier == OSIDENTIFIER_INVALID || UdfSuffix->OSIdentifier == OSIdentifier) &&
            UdfEqualEntityId( RegID, Type, NULL ));
}


//
//  Verification support routines.  Contained in verfysup.c
//

BOOLEAN
UdfCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Force
    );

BOOLEAN
UdfDismountVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
UdfVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
UdfVerifyFcbOperation (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb
    );

//
//  BOOLEAN
//  UdfIsRawDevice (
//      IN PIRP_CONTEXT IrpContext,
//      IN NTSTATUS Status
//      );
//

#define UdfIsRawDevice(IC,S) (           \
    ((S) == STATUS_DEVICE_NOT_READY) ||  \
    ((S) == STATUS_NO_MEDIA_IN_DEVICE)   \
)


//
//  Volume Mapped Control Blocks routines, implemented in VmcbSup.c
//

VOID
UdfInitializeVmcb (
    IN PVMCB Vmcb,
    IN POOL_TYPE PoolType,
    IN ULONG MaximumLbn,
    IN ULONG LbSize
    );

VOID
UdfUninitializeVmcb (
    IN PVMCB Vmcb
    );

VOID
UdfResetVmcb (
    IN PVMCB Vmcb
    );

VOID
UdfSetMaximumLbnVmcb (
    IN PVMCB Vmcb,
    IN ULONG MaximumLbn
    );

BOOLEAN
UdfVmcbVbnToLbn (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL
    );

BOOLEAN
UdfVmcbLbnToVbn (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    OUT PVBN Vbn,
    OUT PULONG SectorCount OPTIONAL
    );

BOOLEAN
UdfAddVmcbMapping (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN BOOLEAN ExactEnd,
    OUT PVBN Vbn,
    OUT PULONG AlignedSectorCount
    );

VOID
UdfRemoveVmcbMapping (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    );


//
//  Routines to verify the correspondance of the underlying media, implemented in
//  verfysup.c
//

NTSTATUS
UdfPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceToVerify
    );


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

NTSTATUS
UdfFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
UdfPrePostIrp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
UdfOplockComplete (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  Charspecs are small containers that specify a CS<N> type and a text
//  string specifying a version, etc.  This is a convenient way of bottling
//  up equivalence checks of a charspec.
//

INLINE
BOOLEAN
UdfEqualCharspec (
    IN PCHARSPEC Charspec,
    IN PSTRING Identifier,
    IN UCHAR Type
    )
{
    return ((Charspec->Type == Type) && UdfEqualCountedString( Identifier, Charspec->Info));
}


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
UdfFspDispatch (                            //  implemented in FspDisp.c
    IN PIRP_CONTEXT IrpContext
    );

VOID
UdfFspClose (                               //  Implemented in Close.c
    IN PVCB Vcb OPTIONAL
    );

//
//  The following routines are the entry points for the different operations
//  based on the IrpSp major functions.
//

NTSTATUS
UdfCommonCleanup (                          //  Implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfCommonClose (                            //  Implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfCommonCreate (                           //  Implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in DevCtrl.c
UdfCommonDevControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in DirCtrl.c
UdfCommonDirControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
UdfCommonFsControl (                        //  Implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in LockCtrl.c
UdfCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in Pnp.c
UdfCommonPnp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in FileInfo.c
UdfCommonQueryInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in VolInfo.c
UdfCommonQueryVolInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in Read.c
UdfCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS                                    //  Implemented in FileInfo.c
UdfCommonSetInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  Clean up our internal-to-the-header definitions so they do not leak out.
//

#undef BugCheckFileId
#undef Dbg


#endif // _UDFPROCS_
