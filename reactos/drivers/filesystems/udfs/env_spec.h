////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: sys_spec.h
*
* Module: UDF File System Driver (Kernel mode execution only)
*
* Description:
*   The main include file for the UDF file system driver.
*
* Author: Alter
*
*************************************************************************/

#ifndef _UDF_ENV_SPEC_H_
#define _UDF_ENV_SPEC_H_

extern NTSTATUS NTAPI UDFPhReadSynchronous(
                   PDEVICE_OBJECT      DeviceObject,
                   PVOID           Buffer,
                   ULONG           Length,
                   LONGLONG        Offset,
                   PULONG          ReadBytes,
                   ULONG           Flags);

extern NTSTATUS NTAPI UDFPhWriteSynchronous(
                   PDEVICE_OBJECT  DeviceObject,   // the physical device object
                   PVOID           Buffer,
                   ULONG           Length,
                   LONGLONG        Offset,
                   PULONG          WrittenBytes,
                   ULONG           Flags);
/*
extern NTSTATUS UDFPhWriteVerifySynchronous(
                   PDEVICE_OBJECT  DeviceObject,   // the physical device object
                   PVOID           Buffer,
                   ULONG           Length,
                   LONGLONG        Offset,
                   PULONG          WrittenBytes,
                   ULONG           Flags);
*/
#define UDFPhWriteVerifySynchronous UDFPhWriteSynchronous

extern NTSTATUS NTAPI
UDFTSendIOCTL(
    IN ULONG IoControlCode,
    IN PVCB Vcb,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    );

extern NTSTATUS NTAPI UDFPhSendIOCTL(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer ,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer ,
    IN ULONG OutputBufferLength,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL);
/*
// This routine performs low-level write (asynchronously if possible)
extern NTSTATUS UDFTWriteAsync(
    IN PVOID _Vcb,
    IN PVOID Buffer,     // Target buffer
    IN ULONG Length,
    IN ULONG LBA,
    OUT PULONG WrittenBytes,
    IN BOOLEAN FreeBuffer);

extern VOID UDFBGWrite(
    IN PVOID Context);
*/

/*#define UDFNotifyFullReportChange(V,FI,E,A)  \
    FsRtlNotifyFullReportChange( (V)->NotifyIRPMutex, &((V)->NextNotifyIRP),        \
                                 ((FI)->ParentFile) ? (PSTRING)&((FI)->Fcb->FCBName->ObjectName) : (PSTRING)&(UDFGlobalData.UnicodeStrRoot),  \
                                 ((FI)->ParentFile) ? ((FI)->ParentFile->Fcb->FCBName->ObjectName.Length + sizeof(WCHAR)) : 0,  \
                                 NULL,NULL,                                         \
                                 E, A,                                              \
                                 NULL);*/

#ifdef UDF_DBG
VOID UDFNotifyFullReportChange(PVCB V,
                               PUDF_FILE_INFO FI,
                               ULONG E,
                               ULONG A);
VOID UDFNotifyVolumeEvent(IN PFILE_OBJECT FileObject,
                          IN ULONG EventCode);
#else // UDF_DBG
__inline VOID UDFNotifyFullReportChange(
    PVCB V,
    PUDF_FILE_INFO FI,
    ULONG E,
    ULONG A
    )
{
    FsRtlNotifyFullReportChange( (V)->NotifyIRPMutex, &((V)->NextNotifyIRP),
                                 (PSTRING)&((FI)->Fcb->FCBName->ObjectName),
                                 ((FI)->ParentFile) ? ((FI)->ParentFile->Fcb->FCBName->ObjectName.Length + sizeof(WCHAR)) : 0,
                                 NULL,NULL,
                                 E, A,
                                 NULL);
}

#define UDFNotifyVolumeEvent(FileObject, EventCode) \
    {/*if(FsRtlNotifyVolumeEvent) FsRtlNotifyVolumeEvent(FileObject, EventCode)*/;}

#endif // UDF_DBG


#define CollectStatistics(VCB, Field) {                                      \
    ((VCB)->Statistics[KeGetCurrentProcessorNumber()].Common.##Field) ++;    \
}

#define CollectStatisticsEx(VCB, Field, a) {                                 \
    ((VCB)->Statistics[KeGetCurrentProcessorNumber()].Common.##Field) += a;  \
}

#define CollectStatistics2(VCB, Field) {                                     \
    ((VCB)->Statistics[KeGetCurrentProcessorNumber()].Fat.##Field) ++;       \
}

#define CollectStatistics2Ex(VCB, Field, a) {                                \
    ((VCB)->Statistics[KeGetCurrentProcessorNumber()].Fat.##Field) += a;     \
}

NTSTATUS NTAPI UDFAsyncCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                         IN PIRP Irp,
                                         IN PVOID Contxt);

NTSTATUS NTAPI UDFSyncCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                                        IN PIRP Irp,
                                        IN PVOID Contxt);

NTSTATUS NTAPI UDFSyncCompletionRoutine2(IN PDEVICE_OBJECT DeviceObject,
                                         IN PIRP Irp,
                                         IN PVOID Contxt);

#define UDFGetDevType(DevObj)    (DevObj->DeviceType)

#define OSGetCurrentThread()     PsGetCurrentThread()

#define GetCurrentPID()   ((ULONG)PsGetCurrentProcessId())


#endif  // _UDF_ENV_SPEC_H_
