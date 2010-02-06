/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        nspinstl.c
 * PURPOSE:     Namespace Provider Installation
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
INT
WSPAPI
WSCEnableNSProvider(IN LPGUID lpProviderId,
                    IN BOOL fEnable)
{
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WSCInstallNameSpace(IN LPWSTR lpszIdentifier,
                    IN LPWSTR lpszPathName,
                    IN DWORD dwNameSpace,
                    IN DWORD dwVersion,
                    IN LPGUID lpProviderId)
{
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WSCUnInstallNameSpace(IN LPGUID lpProviderId)
{
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WSCUpdateProvider(LPGUID lpProviderId,
                  const WCHAR FAR * lpszProviderDllPath,
                  const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
                  DWORD dwNumberOfEntries,
                  LPINT lpErrno)
{
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WSCWriteNameSpaceOrder(LPGUID lpProviderId,
                       DWORD dwNumberOfEntries)
{
    UNIMPLEMENTED;
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}
