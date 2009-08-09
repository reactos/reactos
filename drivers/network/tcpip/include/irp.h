/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/irp.h
 * PURPOSE:     IRP routines
 */
#ifndef __IRP_H
#define __IRP_H

VOID IRPRemember( PIRP Irp, PCHAR File, UINT Line );
NTSTATUS IRPFinish( PIRP Irp, NTSTATUS Status );

#endif/*__IRP_H*/
