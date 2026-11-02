////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __UDF_PHYS_LIB__H__
#define __UDF_PHYS_LIB__H__

NTSTATUS
UDFReadWriteSectorsDirect(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite
    );

NTSTATUS
UDFReadWriteSectors(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN ReturnError,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite
    );

extern NTSTATUS UDFPrepareForWriteOperation(
    IN PVCB Vcb,
    IN ULONG Lba,
    IN ULONG BCount);

NTSTATUS
UDFDetermineVolumeLayout(
    PIRP_CONTEXT IrpContext,
    PDEVICE_OBJECT DeviceObject,
    PVCB Vcb,
    PULONG SessionStart,
    PULONG SessionEnd
    );

NTSTATUS
UDFGetDiskInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject, // the target device object
    IN PVCB Vcb                     // Volume control block from this DevObj
    ); 

NTSTATUS
UDFPrepareForReadOperation(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    );

// read physical sectors
NTSTATUS
UDFReadSectors(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,// Translate Logical to Physical
    IN ULONG Lba,
    IN ULONG BCount,
    IN BOOLEAN Direct,
    OUT PCHAR Buffer
    );


// read unaligned data
NTSTATUS
UDFReadData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,   // Translate Logical to Physical
    IN LONGLONG Offset,
    IN ULONG Length,
    IN BOOLEAN Direct,
    OUT PCHAR Buffer
    );

/* Before a normal write, persist OPEN LVID before any subsequent payload. */
NTSTATUS UDFOpenIntegrityForWrite(IN PIRP_CONTEXT IrpContext,
                                  IN PVCB Vcb,
                                  IN BOOLEAN Direct);
NTSTATUS UDFBarrierFlushDevice(IN PDEVICE_OBJECT TargetDeviceObject);

// write physical sectors
NTSTATUS UDFWriteSectors(IN PIRP_CONTEXT IrpContext,
                         IN PVCB Vcb,
                         IN BOOLEAN Translate,      // Translate Logical to Physical
                         IN ULONG Lba,
                         IN ULONG WBCount,
                         IN BOOLEAN Direct,         // setting this flag delays flushing of given
                                                    // data to indefinite term
                         IN PCHAR Buffer,
                         OUT PSIZE_T WrittenBytes);

// write data at unaligned offset & length
NTSTATUS
UDFWriteData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,      // Translate Logical to Physical
    IN LONGLONG Offset,
    IN SIZE_T Length,
    IN BOOLEAN Direct,         // setting this flag delays flushing of given
                               // data to indefinite term
    IN PCHAR Buffer,
    OUT PSIZE_T WrittenBytes
);

// This macro copies an unaligned src longword to a dst longword,
// performing an little/big endian swap.

#define SwapCopyUchar4(Dst, Src) \
    (*(UNALIGNED ULONG*)(Dst) = _byteswap_ulong(*(UNALIGNED ULONG*)(Src)))

uint32
UDFFixFPAddress(
    IN PVCB Vcb,
    IN uint32 Lba
    );

#endif //__UDF_PHYS_LIB__H__
