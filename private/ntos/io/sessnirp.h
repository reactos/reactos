/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sessnirp.h

Abstract:

Author:

    Adrian J. Oney (adriao) 11-Feb-1999

Revision History:


--*/


#ifndef _SESSIONIRP_H_
#define _SESSIONIRP_H_

#ifndef NO_SPECIAL_IRP

PIOV_SESSION_DATA
FASTCALL
IovpSessionDataCreate(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    );

VOID
FASTCALL
IovpSessionDataAdvance(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    );

VOID
FASTCALL
IovpSessionDataReference(
    IN      PIOV_SESSION_DATA IovSessionData
    );

VOID
FASTCALL
IovpSessionDataDereference(
    IN      PIOV_SESSION_DATA IovSessionData
    );

VOID
FASTCALL
IovpSessionDataClose(
    IN      PIOV_SESSION_DATA IovSessionData
    );

VOID
IovpSessionDataDeterminePolicy(
    IN      PIOV_REQUEST_PACKET IovRequestPacket,
    IN      PDEVICE_OBJECT      DeviceObject,
    OUT     PBOOLEAN            Trackable,
    OUT     PBOOLEAN            UseSurrogateIrp
    );

BOOLEAN
FASTCALL
IovpSessionDataAttachSurrogate(
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    IN      PIOV_SESSION_DATA    IovSessionData
    );

VOID
FASTCALL
IovpSessionDataFinalizeSurrogate(
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      PIRP                 Irp
    );

#endif // NO_SPECIAL_IRP

#endif // _SESSIONIRP_H_

