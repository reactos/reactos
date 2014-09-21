/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/drivers/ws2ifsl/wshdrv.h
 * PURPOSE:     WinSock 2 Helper Driver header
 */

#ifndef __WSHDRV_H
#define __WSHDRV_H

typedef struct _WSH_EA_DATA
{
    HANDLE FileHandle;
    PVOID Context;
} WSH_EA_DATA, *PWAH_EA_DATA;

typedef struct _WAH_EA_DATA2
{
    HANDLE ThreadHandle;
    PVOID RequestRoutine;
    PVOID CancelRoutine;
    PVOID ApcContext;
    ULONG Reserved;
} WAH_EA_DATA2, *PWAH_EA_DATA2;

#define IOCTL_WS2IFSL_SET_HANDLE 0x12B00

#endif /* __WSHDRV_H */
