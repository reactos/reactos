//------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  inst_lsp.cpp
//
//  Installation program for the MS Restricted Process Layered 
//  Service Provider.
//
//  Author:
//
//    Edward Reus (edwardr)      11/06/97
//
//  Revision History:
//
//    Edward Reus ...
//
//------------------------------------------------------------------------

#define UNICODE

#include <ole2.h>
#include <ole2ver.h>
#include "ws2spi.h"
#include "inst_lsp.h"


//------------------------------------------------------------------------
//  InstallRestrictedLSP()
//
//------------------------------------------------------------------------
int InstallRestrictedLSP( DWORD *pdwCatalogId )
    {
    WSAPROTOCOL_INFOW  ProtocolInfoW;
    int         iStatus;
    int         iError = ERROR_SUCCESS;

    memset(&ProtocolInfoW,0,sizeof(WSAPROTOCOL_INFOW));

    // Create a PROTOCOL_INFO to install for our provider DLL.
    // Note: We don't need  to  fill  in  chain  for a 
    // LAYERED_PROTOCOL or BASE_PROTOCOL.
    ProtocolInfoW.dwServiceFlags1 = 0;
    ProtocolInfoW.dwServiceFlags2 = 0;
    ProtocolInfoW.dwServiceFlags3 = 0;
    ProtocolInfoW.dwServiceFlags4 = 0;
    ProtocolInfoW.dwProviderFlags = PFL_HIDDEN;
    ProtocolInfoW.ProviderId      = LayeredProviderGuid;  
    ProtocolInfoW.dwCatalogEntryId = 0;   // Assigned by system.
    ProtocolInfoW.ProtocolChain.ChainLen = LAYERED_PROTOCOL;
    ProtocolInfoW.iVersion = 0;
    ProtocolInfoW.iAddressFamily = AF_INET;
    ProtocolInfoW.iMaxSockAddr = 16;
    ProtocolInfoW.iMinSockAddr = 16;
    ProtocolInfoW.iSocketType = SOCK_STREAM;
    ProtocolInfoW.iProtocol = IPPROTO_TCP;
    ProtocolInfoW.iProtocolMaxOffset = 0;
    ProtocolInfoW.iNetworkByteOrder = BIGENDIAN;
    ProtocolInfoW.iSecurityScheme = SECURITY_PROTOCOL_NONE;
    ProtocolInfoW.dwMessageSize = 0;     // Stream Oriented
    ProtocolInfoW.dwProviderReserved = 0;
    wcscpy( ProtocolInfoW.szProtocol, RESTRICTED_LSP_NAME );

    iStatus = WSCInstallProvider( (GUID*)&LayeredProviderGuid,
                                  LSP_PATH,
                                  &ProtocolInfoW,
                                  1,
                                  &iError );
    *pdwCatalogId = ProtocolInfoW.dwCatalogEntryId;
    
    return iError;
    }

//------------------------------------------------------------------------
//  InstallNewChain()
//
//------------------------------------------------------------------------
int InstallNewChain( LPWSAPROTOCOL_INFOW BaseProtocolInfoBuff,
                     DWORD               dwLayeredProviderCatalogId )
    {
    int               iStatus = ERROR_SUCCESS;
    int               iError = ERROR_SUCCESS;
    WSAPROTOCOL_INFOW ProtocolChainProtoInfo;
    

    // See if we are layering on top of base provider or a chain

    if (BaseProtocolInfoBuff->ProtocolChain.ChainLen == BASE_PROTOCOL)
        {
        // Layering on top of a base protocol:

        ProtocolChainProtoInfo = *BaseProtocolInfoBuff;

        ProtocolChainProtoInfo.ProviderId = RestrictedProviderChainId;
        
        if ( (wcslen(BaseProtocolInfoBuff->szProtocol) + wcslen(CHAIN_PREFIX))
           > WSAPROTOCOL_LEN )
            {
            // The name will be too long use a simpler one...
            wcscpy( ProtocolChainProtoInfo.szProtocol,
                    SIMPLE_CHAIN_NAME );
            }
        else
            {
            wcscpy( ProtocolChainProtoInfo.szProtocol,
                    CHAIN_PREFIX );
            wcscat( ProtocolChainProtoInfo.szProtocol,
                    BaseProtocolInfoBuff->szProtocol );
            }
     
        ProtocolChainProtoInfo.ProtocolChain.ChainLen = 2;
        ProtocolChainProtoInfo.ProtocolChain.ChainEntries[0]
                = dwLayeredProviderCatalogId;
        ProtocolChainProtoInfo.ProtocolChain.ChainEntries[1]
                = BaseProtocolInfoBuff->dwCatalogEntryId;
            
        iStatus = WSCInstallProvider( (GUID*)&RestrictedProviderChainId,
                                      LSP_PATH,
                                      &ProtocolChainProtoInfo,
                                      1,
                                      &iError );
        }
    else
        {
        // Layering on top of an existing chain:

        if (BaseProtocolInfoBuff->ProtocolChain.ChainLen >= MAX_PROTOCOL_CHAIN)
            {
            // The chain is going to be too long...
            return SOCKET_ERROR;
            }

        ProtocolChainProtoInfo = *BaseProtocolInfoBuff;

        ProtocolChainProtoInfo.ProviderId = RestrictedProviderChainId;

        if ( (wcslen(BaseProtocolInfoBuff->szProtocol) + wcslen(CHAIN_PREFIX))
           > WSAPROTOCOL_LEN )
            {
            // The name will be too long use a simpler one...
            wcscpy( ProtocolChainProtoInfo.szProtocol,
                    SIMPLE_CHAIN_NAME );
            }
        else
            {
            wcscpy( ProtocolChainProtoInfo.szProtocol,
                    CHAIN_PREFIX );
            wcscat( ProtocolChainProtoInfo.szProtocol,
                    BaseProtocolInfoBuff->szProtocol );
            }

        ProtocolChainProtoInfo.ProtocolChain.ChainLen
                = 1 + BaseProtocolInfoBuff->ProtocolChain.ChainLen;

        ProtocolChainProtoInfo.ProtocolChain.ChainEntries[0]
                = dwLayeredProviderCatalogId;

        for (int i=0; i<BaseProtocolInfoBuff->ProtocolChain.ChainLen; i++)
            {
            ProtocolChainProtoInfo.ProtocolChain.ChainEntries[1+i]
                = BaseProtocolInfoBuff->ProtocolChain.ChainEntries[i];
            }

        iStatus = WSCInstallProvider( (GUID*)&RestrictedProviderChainId,
                                      LSP_PATH,
                                      &ProtocolChainProtoInfo,
                                      1,
                                      &iError );
        }

    return iStatus;
    }

//------------------------------------------------------------------------
//  LSPAlreadyInstalled()
//
//  Check to see if the restricted network LSP is alreay installed.
//------------------------------------------------------------------------
DWORD LSPAlreadyInstalled( BOOL *pfIsInstalled )
    {
    int                  i;
    int                  iEnumResult;
    int                  iError;
    DWORD                dwStatus = NO_ERROR;
    int                  aiProtocols[2];
    WSAPROTOCOL_INFOW   *pProtocolInfoBuff = NULL;
    DWORD                dwProtocolInfoBuffSize = 0;


    // Enumerate TCP/IP providers and chains

    *pfIsInstalled = FALSE;

    aiProtocols[0] = IPPROTO_TCP;
    aiProtocols[1] = 0;

    // Call WSCEnumProtocols with a zero length buffer so we know what
    // size to send in to get all the installed PROTOCOL_INFO structs. 
    iEnumResult = WSCEnumProtocols( aiProtocols,
                                    pProtocolInfoBuff,
                                    &dwProtocolInfoBuffSize,
                                    &iError );

    if ((iEnumResult == SOCKET_ERROR) && (iError == WSAENOBUFS))
        {
        pProtocolInfoBuff = (WSAPROTOCOL_INFOW*)new char [dwProtocolInfoBuffSize];
        if (!pProtocolInfoBuff)
            {
            // Out of memory...
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        iEnumResult = WSCEnumProtocols( aiProtocols,
                                        pProtocolInfoBuff,
                                        &dwProtocolInfoBuffSize,
                                        &iError );
            
        if (iEnumResult != SOCKET_ERROR)
            {
            // Look through the list and see if the Restricted Network LSP
            // is already there.
            for (i=0; i<iEnumResult; i++)
                {
                if (!memcmp(&pProtocolInfoBuff[i].ProviderId,
                            &LayeredProviderGuid,
                            sizeof(GUID)))
                    {
                    *pfIsInstalled = TRUE;
                    break;
                    }
                }
            }
        }

    delete pProtocolInfoBuff;

    return dwStatus;
    }

//------------------------------------------------------------------------
//  InstallLSP()
//
//  Install the msrlsp into the Winsock2 WSP chains.
//
//------------------------------------------------------------------------
void InstallLSP()
    {
    int                  i;
    int                  EnumResult;
    int                  iError;
    LONG                 lStatus;
    DWORD                CatalogEntryId;
    LPWSAPROTOCOL_INFOW  ProtocolInfoBuff = NULL;
    DWORD                ProtocolInfoBuffSize = 0;
    BOOL                 EntryIdFound;


    // Install the WSAPROTOCOL_INFOW for the layered provider.
    lStatus = InstallRestrictedLSP( &CatalogEntryId );

    if (NO_ERROR == lStatus)
        {
        //
        // Enumerate the installed providers and chains
        //

        // Call WSCEnumProtocols with a zero length buffer so we know what
        // size to  send in to get all the installed PROTOCOL_INFO
        // structs. 
        WSCEnumProtocols( NULL,
                          ProtocolInfoBuff,
                          & ProtocolInfoBuffSize,
                          &iError);

        ProtocolInfoBuff = (LPWSAPROTOCOL_INFOW)new char [ProtocolInfoBuffSize];

        if (ProtocolInfoBuff)
            {
            EnumResult = WSCEnumProtocols(
                NULL,                     // lpiProtocols
                ProtocolInfoBuff,         // lpProtocolBuffer
                & ProtocolInfoBuffSize,   // lpdwBufferLength
                &iError );
            
            if (SOCKET_ERROR != EnumResult)
                {
                // Find our provider entry to get our catalog entry ID
                EntryIdFound = FALSE;
                for (i=0; i<EnumResult; i++)
                    {
                    if (!memcmp(&ProtocolInfoBuff[i].ProviderId,
                                &LayeredProviderGuid,
                                sizeof(GUID)))
                        {
                        
                        CatalogEntryId =
                            ProtocolInfoBuff[i].dwCatalogEntryId;
                        EntryIdFound = TRUE;
                        break;
                        }
                    }

                if (EntryIdFound)
                    {
                    for (i=0; i<EnumResult; i++)
                        {
                        if (  (ProtocolInfoBuff[i].iAddressFamily == AF_INET)
                           && (ProtocolInfoBuff[i].iProtocol == IPPROTO_TCP) )
                            {
                            InstallNewChain( &ProtocolInfoBuff[i],
                                             CatalogEntryId );
                            break;
                            }
                        }
                    }
                }
            }
        }
    }

//------------------------------------------------------------------------
//  UninstallLSP()
//
//  This is called if the msrlsp is already installed and we want to
//  remove it from the Winsock2 WSP chains.
//------------------------------------------------------------------------
void UninstallLSP()
    {
    int  iError;
    int  iStatus;

    iStatus = WSCDeinstallProvider( (GUID*)&RestrictedProviderChainId,&iError );

    iStatus = WSCDeinstallProvider( (GUID*)&LayeredProviderGuid, &iError);
    }

#if FALSE
//------------------------------------------------------------------------
//  main()
//
//------------------------------------------------------------------------
int __cdecl main( int argc, char** argv )
    {
    BOOL  fIsInstalled;
    DWORD dwStatus = LSPAlreadyInstalled(&fIsInstalled);

    if (fIsInstalled)
        {
        printf("LSP already installed: TRUE\n");
        UninstallLSP();
        }
    else
        {
        printf("Install LSP\n");
        InstallLSP();
        }
    
    return 0;
    }
#endif

