/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

LONG gWSM_NSPStartupRef;
LONG gWSM_NSPCallRef;
GUID gNLANamespaceGuid = NLA_NAMESPACE_GUID;

/* FUNCTIONS *****************************************************************/

INT 
WINAPI
WSM_NSPStartup(IN LPGUID lpProviderId,
               IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
    /* Go away */
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}