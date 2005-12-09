/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

INT
WSPAPI
WSPAddressToString(IN LPSOCKADDR lpsaAddress,
                   IN DWORD dwAddressLength,
                   IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                   OUT LPWSTR lpszAddressString,
                   IN OUT LPDWORD lpdwAddressStringLength,
                   OUT LPINT lpErrno)
{
    return 0;
}

INT
WSPAPI
WSPStringToAddress(IN LPWSTR AddressString,
                   IN INT AddressFamily,
                   IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                   OUT LPSOCKADDR lpAddress,
                   IN OUT LPINT lpAddressLength,
                   OUT LPINT lpErrno)
{
    return 0;
}