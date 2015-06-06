////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

#ifndef __UDF_PHYS_LIB__H__
#define __UDF_PHYS_LIB__H__

#ifndef UDF_FORMAT_MEDIA
extern BOOLEAN open_as_device;
extern BOOLEAN opt_invalidate_volume;
extern ULONG LockMode;
#endif //UDF_FORMAT_MEDIA

extern NTSTATUS UDFSyncCache(
    IN PVCB Vcb
    );

OSSTATUS
__fastcall
UDFTIOVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* IOBytes,
    IN uint32 Flags
    );

extern OSSTATUS
UDFTWriteVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* WrittenBytes,
    IN uint32 Flags
    );

extern OSSTATUS
UDFTReadVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* ReadBytes,
    IN uint32 Flags
    );

extern OSSTATUS UDFTRead(PVOID           _Vcb,
                         PVOID           Buffer,     // Target buffer
                         ULONG           Length,
                         ULONG           LBA,
                         PULONG          ReadBytes,
                         ULONG           Flags = 0);

extern OSSTATUS UDFTWrite(IN PVOID _Vcb,
                   IN PVOID Buffer,     // Target buffer
                   IN ULONG Length,
                   IN ULONG LBA,
                   OUT PULONG WrittenBytes,
                   IN ULONG Flags = 0);

#define PH_TMP_BUFFER          1
#define PH_VCB_IN_RETLEN       2
#define PH_LOCK_CACHE          0x10000000

#define PH_EX_WRITE            0x80000000
#define PH_IO_LOCKED           0x20000000


extern
OSSTATUS
UDFDoOPC(
    IN PVCB Vcb
    );

extern OSSTATUS UDFPrepareForWriteOperation(
    IN PVCB Vcb,
    IN ULONG Lba,
    IN ULONG BCount);

extern OSSTATUS UDFReadDiscTrackInfo(PDEVICE_OBJECT DeviceObject, // the target device object
                                     PVCB           Vcb);         // Volume Control Block for ^ DevObj

extern OSSTATUS UDFReadAndProcessFullToc(PDEVICE_OBJECT DeviceObject, // the target device object
                                         PVCB           Vcb);

extern OSSTATUS UDFUseStandard(PDEVICE_OBJECT DeviceObject, // the target device object
                               PVCB           Vcb);         // Volume control block fro this DevObj

extern OSSTATUS UDFGetBlockSize(PDEVICE_OBJECT DeviceObject, // the target device object
                                PVCB           Vcb);         // Volume control block fro this DevObj

extern OSSTATUS UDFGetDiskInfo(IN PDEVICE_OBJECT DeviceObject, // the target device object
                               IN PVCB           Vcb);         // Volume control block from this DevObj

extern VOID     UDFEjectReqWaiter(IN PVOID Context);

extern VOID     UDFStopEjectWaiter(PVCB Vcb);

extern OSSTATUS UDFPrepareForReadOperation(IN PVCB Vcb,
                                           IN uint32 Lba,
                                           IN uint32 BCount
                                           );
//#define UDFPrepareForReadOperation(a,b) (STATUS_SUCCESS)

extern VOID     UDFUpdateNWA(PVCB Vcb,
                             ULONG LBA,
                             ULONG BCount,
                             OSSTATUS RC);

extern OSSTATUS UDFDoDismountSequence(IN PVCB Vcb,
                                      IN PPREVENT_MEDIA_REMOVAL_USER_IN Buf,
                                      IN BOOLEAN Eject);

// read physical sectors
/*OSSTATUS UDFReadSectors(IN PVCB Vcb,
                        IN BOOLEAN Translate,// Translate Logical to Physical
                        IN ULONG Lba,
                        IN ULONG BCount,
                        IN BOOLEAN Direct,
                        OUT PCHAR Buffer,
                        OUT PULONG ReadBytes);*/
#define UDFReadSectors(Vcb, Translate, Lba, BCount, Direct, Buffer, ReadBytes)                 \
    (( WCacheIsInitialized__(&((Vcb)->FastCache)) && (KeGetCurrentIrql() < DISPATCH_LEVEL)) ?              \
        (WCacheReadBlocks__(&((Vcb)->FastCache), Vcb, Buffer, Lba, BCount, ReadBytes, Direct)) : \
        (UDFTRead(Vcb, Buffer, (BCount)<<((Vcb)->BlockSizeBits), Lba, ReadBytes, 0)))


// read data inside physical sector
extern OSSTATUS UDFReadInSector(IN PVCB Vcb,
                         IN BOOLEAN Translate,       // Translate Logical to Physical
                         IN ULONG Lba,
                         IN ULONG i,                 // offset in sector
                         IN ULONG l,                 // transfer length
                         IN BOOLEAN Direct,
                         OUT PCHAR Buffer,
                         OUT PULONG ReadBytes);
// read unaligned data
extern OSSTATUS UDFReadData(IN PVCB Vcb,
                     IN BOOLEAN Translate,   // Translate Logical to Physical
                     IN LONGLONG Offset,
                     IN ULONG Length,
                     IN BOOLEAN Direct,
                     OUT PCHAR Buffer,
                     OUT PULONG ReadBytes);

#ifndef UDF_READ_ONLY_BUILD
// write physical sectors
OSSTATUS UDFWriteSectors(IN PVCB Vcb,
                         IN BOOLEAN Translate,      // Translate Logical to Physical
                         IN ULONG Lba,
                         IN ULONG WBCount,
                         IN BOOLEAN Direct,         // setting this flag delays flushing of given
                                                    // data to indefinite term
                         IN PCHAR Buffer,
                         OUT PULONG WrittenBytes);
// write directly to cached sector
OSSTATUS UDFWriteInSector(IN PVCB Vcb,
                          IN BOOLEAN Translate,       // Translate Logical to Physical
                          IN ULONG Lba,
                          IN ULONG i,                 // offset in sector
                          IN ULONG l,                 // transfer length
                          IN BOOLEAN Direct,
                          OUT PCHAR Buffer,
                          OUT PULONG WrittenBytes);
// write data at unaligned offset & length
OSSTATUS UDFWriteData(IN PVCB Vcb,
                      IN BOOLEAN Translate,      // Translate Logical to Physical
                      IN LONGLONG Offset,
                      IN ULONG Length,
                      IN BOOLEAN Direct,         // setting this flag delays flushing of given
                                                 // data to indefinite term
                      IN PCHAR Buffer,
                      OUT PULONG WrittenBytes);
#endif //UDF_READ_ONLY_BUILD

OSSTATUS UDFResetDeviceDriver(IN PVCB Vcb,
                              IN PDEVICE_OBJECT TargetDeviceObject,
                              IN BOOLEAN Unlock);


#endif //__UDF_PHYS_LIB__H__