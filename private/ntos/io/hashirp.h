/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    hashirp.h

Abstract:

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:


--*/


#ifndef _HASHIRP_H_
#define _HASHIRP_H_

#define IRP_TRACKING_HASH_SIZE   256
#define IRP_TRACKING_HASH_PRIME  131

#ifndef NO_SPECIAL_IRP

extern ULONG IovpIrpTrackingSpewLevel;

typedef enum _IOV_REFERENCE_TYPE {

    IOVREFTYPE_PACKET = 0,
    IOVREFTYPE_POINTER

} IOV_REFERENCE_TYPE;

VOID
FASTCALL
IovpTrackingDataInit(
    VOID
    );

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataFindAndLock(
    IN PIRP     Irp
    );

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataCreateAndLock(
    IN PIRP           Irp
    );

VOID
FASTCALL
IovpTrackingDataFree(
    IN  PIOV_REQUEST_PACKET   IrpTrackingData
    );

PIOV_REQUEST_PACKET
FASTCALL
IovpTrackingDataFindPointer(
    IN  PIRP                Irp,
    OUT PLIST_ENTRY         *HashHead
    );

VOID
FASTCALL
IovpTrackingDataAcquireLock(
    IN  PIOV_REQUEST_PACKET   IrpTrackingData
    );

VOID
FASTCALL
IovpTrackingDataReleaseLock(
    IN  PIOV_REQUEST_PACKET   IrpTrackingData
    );

VOID
FASTCALL
IovpTrackingDataReference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    );

VOID
FASTCALL
IovpTrackingDataDereference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    );

VOID
FASTCALL
IovpWatermarkIrp(
    IN PIRP  Irp,
    IN ULONG Flags
    );

PIOV_SESSION_DATA
FASTCALL
IovpTrackingDataGetCurrentSessionData(
    IN PIOV_REQUEST_PACKET IovPacket
    );

PVOID
FASTCALL
IovpProtectedIrpMakeUntouchable(
    IN  PIRP    Irp OPTIONAL,
    IN  BOOLEAN Permanent
    );

VOID
FASTCALL
IovpProtectedIrpMakeTouchable(
    IN  PIRP  Irp,
    IN  PVOID *RestoreHandle
    );

VOID
FASTCALL
IovpProtectedIrpFree(
    IN  PIRP   Irp OPTIONAL,
    IN  PVOID *RestoreHandle
    );

PIRP
FASTCALL
IovpProtectedIrpAllocate(
    IN CCHAR    StackSize,
    IN BOOLEAN  ChargeQuota,
    IN PETHREAD QuotaThread OPTIONAL
    );

#endif // NO_SPECIAL_IRP

#endif // _HASHIRP_H_

