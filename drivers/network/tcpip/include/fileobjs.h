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
extern LIST_ENTRY ConnectionEndpointListHead;
extern KSPIN_LOCK ConnectionEndpointListLock;


NTSTATUS FileOpenAddress(
  PTDI_REQUEST Request,
  PTA_IP_ADDRESS AddrList,
  USHORT Protocol,
  PVOID Options);

NTSTATUS FileCloseAddress(
  PTDI_REQUEST Request);

NTSTATUS FileFreeAddress(
  PTDI_REQUEST Request);

NTSTATUS FileOpenConnection(
  PTDI_REQUEST Request,
  PVOID ClientContext);

PCONNECTION_ENDPOINT FileFindConnectionByContext( PVOID Context );

NTSTATUS FileCloseConnection(
  PTDI_REQUEST Request);

NTSTATUS FileFreeConnection(
  PTDI_REQUEST Request);

NTSTATUS FileOpenControlChannel(
  PTDI_REQUEST Request);

NTSTATUS FileFreeControlChannel(
  PTDI_REQUEST Request);

#endif /* __FILEOBJS_H */

/* EOF */
