/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/dispatch.h
 * PURPOSE:     Dispatch routine prototypes
 */

#pragma once

typedef struct _DISCONNECT_TYPE {
    UINT Type;
    PVOID Context;
    PIRP Irp;
} DISCONNECT_TYPE, *PDISCONNECT_TYPE;

NTSTATUS DispTdiAccept(
    PIRP Irp);

NTSTATUS DispTdiAssociateAddress(
    PIRP Irp);

NTSTATUS DispTdiConnect(
    PIRP Irp);

NTSTATUS DispTdiDisassociateAddress(
    PIRP Irp);

NTSTATUS DispTdiDisconnect(
    PIRP Irp);

NTSTATUS DispTdiListen(
    PIRP Irp);

NTSTATUS DispTdiQueryInformation(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS DispTdiReceive(
    PIRP Irp);

NTSTATUS DispTdiReceiveDatagram(
    PIRP Irp);

NTSTATUS DispTdiSend(
    PIRP Irp);

NTSTATUS DispTdiSendDatagram(
    PIRP Irp);

NTSTATUS DispTdiSetEventHandler(
    PIRP Irp);

NTSTATUS DispTdiSetInformation(
    PIRP Irp);

NTSTATUS DispTdiQueryInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS DispTdiSetInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS DispTdiSetIPAddress(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS DispTdiDeleteIPAddress(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

NTSTATUS DispTdiQueryIpHwAddress(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp);

VOID DispDoDisconnect(
    PVOID Data);

NTSTATUS IRPFinish( PIRP Irp, NTSTATUS Status );

/* EOF */
