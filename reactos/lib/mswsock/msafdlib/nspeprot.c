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
SockLoadTransportList(PWSTR *TransportList)
{
    ULONG TransportListSize = 0;
    HKEY KeyHandle;
    INT ErrorCode;
    
    /* Open the Transports Key */
    ErrorCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"SYSTEM\\CurrentControlSet\\Services\\Winsock\\Parameters",  
                              0, 
                              KEY_READ, 
                              &KeyHandle);

    /* Check for error */
    if (ErrorCode != NO_ERROR) return ErrorCode;
    
    /* Get the Transport List Size */
    ErrorCode = RegQueryValueExW(KeyHandle,
                                 L"Transports",
                                 NULL,
                                 NULL,
                                 NULL,
                                 &TransportListSize);

    /* Check for error */
    if ((ErrorCode != ERROR_MORE_DATA) && (ErrorCode != NO_ERROR))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Allocate Memory for the Transport List */
    *TransportList = SockAllocateHeapRoutine(SockPrivateHeap,
                                             0,
                                             TransportListSize);

    /* Check for error */
    if (!(*TransportList))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Get the Transports */
    ErrorCode = RegQueryValueExW(KeyHandle, 
                                 L"Transports", 
                                 NULL, 
                                 NULL, 
                                 (LPBYTE)*TransportList, 
                                 &TransportListSize);

    /* Check for error */
    if ((ErrorCode != ERROR_MORE_DATA) && (ErrorCode != NO_ERROR))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Close key and return */
    RegCloseKey(KeyHandle);
    return NO_ERROR;
}