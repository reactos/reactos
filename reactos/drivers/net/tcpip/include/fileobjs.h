/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/fileobjs.h
 * PURPOSE:     File object routine prototypes
 */
#ifndef __FILEOBJS_H
#define __FILEOBJS_H


extern LIST_ENTRY AddressFileListHead;
extern KSPIN_LOCK AddressFileListLock;


NTSTATUS FileOpenAddress(
    PTDI_REQUEST Request,
    PTA_ADDRESS_IP AddrList,
    USHORT Protocol,
    PVOID Options);

NTSTATUS FileCloseAddress(
    PTDI_REQUEST Request);

NTSTATUS FileCloseConnection(
    PTDI_REQUEST Request);

NTSTATUS FileCloseControlChannel(
    PTDI_REQUEST Request);

#endif /* __FILEOBJS_H */

/* EOF */
