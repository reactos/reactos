// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#ifndef _FREEBT_RWR_H
#define _FREEBT_RWR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FREEBT_RW_CONTEXT
{
    PURB					Urb;
    PMDL					Mdl;
    ULONG					Length;				// remaining to xfer
    ULONG					Numxfer;			// cumulate xfer
    ULONG_PTR				VirtualAddress;		// va for next segment of xfer.

} FREEBT_RW_CONTEXT, * PFREEBT_RW_CONTEXT;

NTSTATUS NTAPI FreeBT_DispatchRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI FreeBT_ReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS NTAPI FreeBT_DispatchWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS NTAPI FreeBT_WriteCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

#ifdef __cplusplus
};
#endif

#endif
