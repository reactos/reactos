/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    flunkirp.h

Abstract:

    The module associated with this header asserts specific Irp types (eg PNP)
    are handled correctly by drivers. Generic IRP assertion is done by the
    trackirp.* bits.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:


--*/

#ifndef _FLUNKIRP_H_
#define _FLUNKIRP_H_

#ifndef NO_SPECIAL_IRP

//
// These are in flunkirp.c
//
BOOLEAN
IovpAssertIsNewRequest(
    IN PIO_STACK_LOCATION   IrpLastSp,
    IN PIO_STACK_LOCATION   IrpSp
    );

VOID
IovpAssertNewIrps(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    );

VOID
IovpAssertNewRequest(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    );

BOOLEAN
IovpAssertDoAdvanceStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    );

VOID
IovpAssertIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData
    );

VOID
IovpAssertIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

VOID
IovpAssertFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IrpTrackingData,
    IN PIO_STACK_LOCATION   IrpSp
    );

BOOLEAN
IovpAssertIsValidIrpStatus(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    );

VOID
IovpThrowChaffAtStartedPdoStack(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
IovpThrowBogusSynchronousIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN OUT OPTIONAL ULONG_PTR Information,
    IN OUT ULONG_PTR *InformationOut OPTIONAL,
    IN BOOLEAN IsBogus
    );

LONG
IovpStartObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject
    );

LONG
IovpStopObRefMonitoring(
    IN PDEVICE_OBJECT DeviceObject,
    IN LONG StartSkew
    );

BOOLEAN
IovpIsSystemRestrictedIrp(
    PIO_STACK_LOCATION IrpSp
    );

#endif // NO_SPECIAL_IRP

#endif // _FLUNKIRP_H_

