/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS kernel
 * FILE:        include/services/services.h
 * PURPOSE:     Private interface between SERVICES.EXE and ADVAPI32.DLL
 * PROGRAMMER:  Eric Kohl
 */

#ifndef __SERVICES_SERVICES_H__
#define __SERVICES_SERVICES_H__

#define SERVICE_CONTROL_START 0

DECLARE_HANDLE(CLIENT_HANDLE);

typedef struct _SCM_CONTROL_PACKET
{
    DWORD dwControl;
    CLIENT_HANDLE hClient;
    DWORD dwSize;
    WCHAR szArguments[1];
} SCM_CONTROL_PACKET, *PSCM_CONTROL_PACKET;

#endif /* __SERVICES_SERVICES_H__ */

/* EOF */
