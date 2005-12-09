/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

PVOID pfnIcfOpenPort;
PICF_CONNECT pfnIcfConnect;
PVOID pfnIcfDisconnect;
HINSTANCE IcfDllHandle;

WSPPROC_TABLE SockProcTable =
{
    &WSPAccept,
    &WSPAddressToString,
    &WSPAsyncSelect,
    &WSPBind,
    &WSPCancelBlockingCall,
    &WSPCleanup,
    &WSPCloseSocket,
    &WSPConnect,
    &WSPDuplicateSocket,
    &WSPEnumNetworkEvents,
    &WSPEventSelect,
    &WSPGetOverlappedResult,
    &WSPGetPeerName,
    &WSPGetSockName,
    &WSPGetSockOpt,
    &WSPGetQOSByName,
    &WSPIoctl,
    &WSPJoinLeaf,
    &WSPListen,
    &WSPRecv,
    &WSPRecvDisconnect,
    &WSPRecvFrom,
    &WSPSelect,
    &WSPSend,
    &WSPSendDisconnect,
    &WSPSendTo,
    &WSPSetSockOpt,
    &WSPShutdown,
    &WSPSocket,
    &WSPStringToAddress
};

LONG SockWspStartupCount;
WSPUPCALLTABLE SockUpcallTableHack;
LPWSPUPCALLTABLE SockUpcallTable;

/* FUNCTIONS *****************************************************************/

VOID
WSPAPI
NewIcfConnection(IN PSOCK_ICF_DATA IcfData)
{
    /* Load the ICF DLL */
    IcfData->DllHandle = LoadLibraryW(L"hhnetcfg.dll");
    if (IcfData->DllHandle)
    {
        /* Get the entrypoints */
        IcfData->IcfOpenDynamicFwPort = GetProcAddress(IcfData->DllHandle,
                                                       "IcfOpenDynamicFwPort");
        IcfData->IcfConnect = (PICF_CONNECT)GetProcAddress(IcfData->DllHandle,
                                                           "IcfConnect");
        IcfData->IcfDisconnect = GetProcAddress(IcfData->DllHandle,
                                                "IcfDisconnect");

        /* Now call IcfConnect */
        if (!IcfData->IcfConnect(IcfData))
        {
            /* We failed, release the library */
            FreeLibrary(IcfData->DllHandle);
        }
    }
}

VOID
WSPAPI
InitializeIcfConnection(IN PSOCK_ICF_DATA IcfData)
{
    /* Make sure we have an ICF Handle */
    if (IcfData->IcfHandle)
    {
        /* Save the function pointers and dll handle */
        IcfDllHandle = IcfData->DllHandle;
        pfnIcfOpenPort = IcfData->IcfOpenDynamicFwPort;
        pfnIcfConnect = IcfData->IcfConnect;
        pfnIcfDisconnect = IcfData->IcfDisconnect;
    }
}

VOID
WSPAPI
CloseIcfConnection(IN PSOCK_ICF_DATA IcfData)
{
    /* Make sure we have an ICF Handle */
    if (IcfData->IcfHandle)
    {
        /* Call IcfDisconnect */
        IcfData->IcfConnect(IcfData);

        /* Release the library */
        FreeLibrary(IcfData->DllHandle);
    }
}

INT
WSPAPI
WSPStartup(IN WORD wVersionRequested,
           OUT LPWSPDATA lpWSPData,
           IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
           IN WSPUPCALLTABLE UpcallTable,
           OUT LPWSPPROC_TABLE lpProcTable)
{
    CHAR DllPath[MAX_PATH];
    HINSTANCE DllHandle;
    SOCK_ICF_DATA IcfData;
    NT_PRODUCT_TYPE ProductType;

    /* Call the generic mswsock initialization routine */
    if (!MSWSOCK_Initialize()) return WSAENOBUFS;

    /* Check if we have TEB data yet */
    if (!NtCurrentTeb()->WinSockData)
    {
        /* We don't have thread data yet, initialize it */
        if (!MSAFD_SockThreadInitialize()) return WSAENOBUFS;
    }

    /* Check the version number */
    if (wVersionRequested != MAKEWORD(2,2)) return WSAVERNOTSUPPORTED;

    /* Get ICF entrypoints */
    NewIcfConnection(&IcfData);

    /* Acquire the global lock */
    SockAcquireRwLockExclusive(&SocketGlobalLock);

    /* Check if we've never initialized before */
    if (!SockWspStartupCount)
    {
        /* Check if we have a context table by now */
        if (!SockContextTable)
        {
            /* Create it */
            if (WahCreateHandleContextTable(&SockContextTable) != NO_ERROR)
            {
                /* Fail */
                SockReleaseRwLockExclusive(&SocketGlobalLock);
                CloseIcfConnection(&IcfData);
                return WSASYSCALLFAILURE;
            }
        }

        /* Bias our load count so we won't be killed with pending APCs */
        GetModuleFileName(SockModuleHandle, DllPath, MAX_PATH);
        DllHandle = LoadLibrary(DllPath);
        if (!DllHandle)
        {
            /* Weird error, release and return */
            SockReleaseRwLockExclusive(&SocketGlobalLock);
            CloseIcfConnection(&IcfData);
            return WSASYSCALLFAILURE;
        }

        /* Initialize ICF */
        InitializeIcfConnection(&IcfData);

        /* Set our Upcall Table */
        SockUpcallTableHack = UpcallTable;
        SockUpcallTable = &SockUpcallTableHack;
    }

    /* Increase startup count */
    SockWspStartupCount++;

    /* Return our version */
    lpWSPData->wVersion = MAKEWORD(2, 2);
    lpWSPData->wHighVersion = MAKEWORD(2, 2);
    wcscpy(lpWSPData->szDescription, L"Microsoft Windows Sockets Version 2.");

    /* Return our Internal Table */
    *lpProcTable = SockProcTable;

    /* Check if this is a SAN GUID */
    if (IsEqualGUID(&lpProtocolInfo->ProviderId, &SockTcpProviderInfo.ProviderId))
    {
        /* Get the product type and check if this is a server OS */
        RtlGetNtProductType(&ProductType);
        if (ProductType != NtProductWinNt)
        {
            /* Get the SAN TCP/IP Catalog ID */
            SockSanGetTcpipCatalogId();

            /* Initialize SAN if it's enabled */
            if (SockSanEnabled) SockSanInitialize();
        }
    }

    /* Release the lock and return */
    SockReleaseRwLockExclusive(&SocketGlobalLock);

    /* Return to caller */
    return NO_ERROR;
}

INT
WSPAPI
WSPCleanup(OUT LPINT lpErrno)
{
    /* FIXME: Clean up */
    *lpErrno = NO_ERROR;
    return 0;
}
