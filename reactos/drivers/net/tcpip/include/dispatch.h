/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/dispatch.h
 * PURPOSE:     Dispatch routine prototypes
 */
#ifndef __DISPATCH_H
#define __DISPATCH_H


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

#endif /* __DISPATCH_H */

/* EOF */
