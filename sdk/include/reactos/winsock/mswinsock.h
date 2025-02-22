/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/reactos/winsock/mswinsock.h
 * PURPOSE:     Ancillary Function Driver DLL header
 */
#ifndef __MSWINSOCK_H
#define __MSWINSOCK_H

typedef DWORD (* LPFN_NSPAPI)(VOID);
typedef struct _NS_ROUTINE {
    DWORD        dwFunctionCount;
    LPFN_NSPAPI *alpfnFunctions;
    DWORD        dwNameSpace;
    DWORD        dwPriority;
} NS_ROUTINE, *PNS_ROUTINE, * FAR LPNS_ROUTINE;

#endif

