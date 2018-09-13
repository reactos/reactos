/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ipconfig.cxx

Abstract:

    CIpConfig class implementation

    Contents:
        CIpAddress::GetAddress

        CIpAddressList::Find
        CIpAddressList::Add(CIpAddress *)
        CIpAddressList::Add(DWORD, DWORD, DWORD)
        CIpAddressList::GetAddress
        CIpAddressList::ThrowOutUnfoundEntries

        CAdapterInterface::CAdapterInterface
        CAdapterInterface::~CAdapterInterface

        CIpConfig::CIpConfig
        CIpConfig::~CIpConfig
        CIpConfig::GetRouterAddress
        CIpConfig::GetDnsAddress
        CIpConfig::IsKnownIpAddress
        CIpConfig::Refresh
        (CIpConfig::GetAdapterList)
        (CIpConfig::LoadEntryPoints)
        (CIpConfig::UnloadEntryPoints)
        (CIpConfig::FindOrCreateInterface)
        (CIpConfig::FindInterface)
        (CIpConfig::ThrowOutUnfoundEntries)

        WsControl
        (WinNtWsControl)
        (OpenTcpipDriverHandle)
        (CloseTcpipDriverHandle)
        (GetEntityList)
        [InternetMapEntity]
        [InternetMapInterface]

Author:

    Richard L Firth (rfirth) 29-Oct-1996

Environment:

    Win32 user-mode DLL

Notes:

    In order to operate correctly, we require the Microsoft Winsock implementation
    (WSOCK32.DLL) and the Microsoft TCP/IP stack to be loaded

Revision History:

    29-Oct-1996 rfirth
        Created

    15-Jul-1998 arthurbi
        Resurrected from the dead

--*/

#include <wininetp.h>
#include "aproxp.h"

//
// manifests
//

#define MAX_ADAPTER_DESCRIPTION_LENGTH  128 // arbitrary
//#define DEFAULT_MINIMUM_ENTITIES        MAX_TDI_ENTITIES

//
// macros
//

//
// IS_INTERESTING_ADAPTER - TRUE if the type of this adapter (IFEntry) is NOT
// loopback. Loopback (corresponding to local host) is the only one we filter
// out right now
//

#define IS_INTERESTING_ADAPTER(p)   (!((p)->if_type == IF_TYPE_LOOPBACK))
#define IS_INTERESTING_ADAPTER_NT5(p) (!((p)->Type == IF_LOOPBACK_ADAPTERTYPE))

//
// globals
//

const char SERVICES_KEY_NAME[] = "SYSTEM\\CurrentControlSet\\Services";

HKEY TcpipLinkageKey = NULL;//     = INVALID_HANDLE_VALUE;
HKEY ServicesKey = NULL;  //       = INVALID_HANDLE_VALUE;

//
// private prototypes
//

PRIVATE
DWORD
WinNtWsControl(
    DWORD dwProtocol,
    DWORD dwRequest,
    LPVOID lpInputBuffer,
    LPDWORD lpdwInputBufferLength,
    LPVOID lpOutputBuffer,
    LPDWORD lpdwOutputBufferLength
    );

PRIVATE
DWORD
OpenTcpipDriverHandle(
    VOID
    );

PRIVATE
VOID
CloseTcpipDriverHandle(
    VOID
    );

PRIVATE
DWORD
GetEntityList(
    OUT TDIEntityID * * lplpEntities
    );

//
// private debug prototypes
//

PRIVATE
LPSTR
InternetMapEntity(
    IN INT EntityId
    );

PRIVATE
LPSTR
InternetMapInterface(
    IN DWORD InterfaceType
    );

PRIVATE
LPSTR
InternetMapInterfaceOnNT5(
    IN DWORD InterfaceType
    );


//
// private data
//

//
// NTDLL info - if the platform is NT then we use the following entry points in
// NTDLL.DLL to talk to the TCP/IP device driver
//

PRIVATE VOID (* _I_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR) = NULL;
PRIVATE NTSTATUS (* _I_NtCreateFile)(PHANDLE,
                                     ACCESS_MASK,
                                     POBJECT_ATTRIBUTES,
                                     PIO_STATUS_BLOCK,
                                     PLARGE_INTEGER,
                                     ULONG,
                                     ULONG,
                                     ULONG,
                                     ULONG,
                                     PVOID,
                                     ULONG
                                     ) = NULL;
PRIVATE ULONG (* _I_RtlNtStatusToDosError)(NTSTATUS) = NULL;

PRIVATE DLL_ENTRY_POINT NtDllEntryPoints[] = {
    DLL_ENTRY_POINT_ELEMENT(RtlInitUnicodeString),
    DLL_ENTRY_POINT_ELEMENT(NtCreateFile),
    DLL_ENTRY_POINT_ELEMENT(RtlNtStatusToDosError)
};

PRIVATE DLL_INFO NtDllInfo = DLL_INFO_INIT("NTDLL.DLL", NtDllEntryPoints);

//
// WSOCK32 info - if the platform is Windows 95 then we use the WsControl entry
// point in WSOCK32.DLL to access the TCP/IP device driver. If we are running
// over non-MS Winsock or TCP/IP then we cannot get the IpConfig info (unless
// the same features are supported)
//

PRIVATE DWORD (PASCAL FAR * _I_WsControl)(DWORD,
                                          DWORD,
                                          LPVOID,
                                          LPDWORD,
                                          LPVOID,
                                          LPDWORD
                                          ) = NULL;

PRIVATE DLL_ENTRY_POINT WsControlEntryPoint[] = {
    DLL_ENTRY_POINT_ELEMENT(WsControl)
};

PRIVATE DLL_INFO WsControlInfo = DLL_INFO_INIT("WSOCK32.DLL", WsControlEntryPoint);
PRIVATE HANDLE TcpipDriverHandle = INVALID_HANDLE_VALUE;

//
// Iphlpapi - Ip Helper APIs only found on NT 5 and Win 98, must dynaload,
//   Used to gather information on what adapters are avaible on the machine
//

PRIVATE DWORD (PASCAL FAR * _I_GetAdaptersInfo)(PIP_ADAPTER_INFO,
                                          PULONG
                                          ) = NULL;

PRIVATE DLL_ENTRY_POINT IpHlpApiEntryPoints[] = {
    DLL_ENTRY_POINT_ELEMENT(GetAdaptersInfo)
};

PRIVATE DLL_INFO IpHlpApiDllInfo = DLL_INFO_INIT("IPHLPAPI.DLL", IpHlpApiEntryPoints);

//
// DhcpcSvc - DHCP dll, Only found on Win'98 and NT 5.  This function does almost all the
//   work for us using the native DHCP services found on these cool new OSes.
//

PRIVATE DWORD (PASCAL FAR * _I_DhcpRequestOptions)(LPWSTR,
                            BYTE  *,
                            DWORD,
                            BYTE **,
                            DWORD *,
                            BYTE **,
                            DWORD *
                          ) = NULL;

PRIVATE DLL_ENTRY_POINT DhcpcSvcEntryPoints[] = {
    DLL_ENTRY_POINT_ELEMENT(DhcpRequestOptions)
};

PRIVATE DLL_INFO DhcpcSvcDllInfo = DLL_INFO_INIT("DHCPCSVC.DLL", DhcpcSvcEntryPoints);


//
// global data
//

// none.

//
// methods
//

//
// public CIpAddress methods
//


BOOL
CIpAddress::GetAddress(
    OUT LPBYTE lpbAddress,
    IN OUT LPDWORD lpdwAddressLength
    )

/*++

Routine Description:

    Returns the IP address from this CIpAddress

Arguments:

    lpbAddress          - pointer to returned address

    lpdwAddressLength   - size of IP address

Return Value:

    BOOL
        TRUE    - address copied

        FALSE   - address not copied (buffer not large enough)

--*/

{
    if (*lpdwAddressLength >= sizeof(DWORD)) {
        *(LPDWORD)lpbAddress = m_dwIpAddress;
        *lpdwAddressLength = sizeof(DWORD);
        return TRUE;
    }
    return FALSE;
}


//
// public CIpAddressList methods
//

BOOL
CIpAddressList::IsContextInList(
    IN DWORD dwContext
    )
{
   for (CIpAddress * pEntry = m_List; pEntry != NULL; pEntry = pEntry->m_Next) {
       if (pEntry->Context() == dwContext) {
           return TRUE;
       }
   }
   return FALSE;
}




CIpAddress *
CIpAddressList::Find(
    IN DWORD dwIpAddress,
    IN DWORD dwIpMask
    )

/*++

Routine Description:

    Finds the CIpAddress object corresponding to (dwIpAddress, dwIpMask)

Arguments:

    dwIpAddress - IP address to find

    dwIpMask    - IP address mask, or INADDR_ANY (0) if we don't care

Return Value:

    CIpAddress *
        Success - pointer to found object

        Failure - NULL

--*/

{
    for (CIpAddress * pEntry = m_List; pEntry != NULL; pEntry = pEntry->m_Next) {
        if ((pEntry->IpAddress() == dwIpAddress)
        && ((dwIpMask == INADDR_ANY) || (pEntry->IpMask() == dwIpMask))) {
            break;
        }
    }
    return pEntry;
}


VOID
CIpAddressList::Add(
    IN CIpAddress * pAddress
    )

/*++

Routine Description:

    Adds an IP address entry to the list

Arguments:

    pAddress    - pointer to CIpAddress to add

Return Value:

    None.

--*/

{
    INET_ASSERT(pAddress->m_Next == NULL);

    CIpAddress * pEntry = (CIpAddress *)&m_List;

    while (pEntry->m_Next != NULL) {
        pEntry = pEntry->m_Next;
    }
    pEntry->m_Next = pAddress;
}


BOOL
CIpAddressList::Add(
    IN DWORD dwIpAddress,
    IN DWORD dwIpMask,
    IN DWORD dwContext
    )

/*++

Routine Description:

    Adds an IP address entry to the list

Arguments:

    dwIpAddress - IP address to add

    dwIpMask    - IP subnet mask

    dwContext   - unique interface context value

Return Value:

    BOOL
        TRUE    - item added

        FALSE   - out of memory

--*/

{
    CIpAddress * pIpAddress = new CIpAddress(dwIpAddress, dwIpMask, dwContext);

    if (pIpAddress != NULL) {
        Add(pIpAddress);
        return TRUE;
    }
    return FALSE;
}


BOOL
CIpAddressList::GetAddress(
    IN OUT LPDWORD lpdwIndex,
    OUT LPBYTE lpbAddress,
    IN OUT LPDWORD lpdwAddressLength
    )

/*++

Routine Description:

    Returns the *lpdwIndex'th address from the list

Arguments:

    lpdwIndex           - which address to return. Updated on output

    lpbAddress          - pointer to returned address

    lpdwAddressLength   - pointer to returned address length

Return Value:

    BOOL
        TRUE    - address returned

        FALSE   - address not returned

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpAddressList::GetAddress",
                 "%#x [%d], %#x, %#x [%d]",
                 lpdwIndex,
                 *lpdwIndex,
                 lpbAddress,
                 lpdwAddressLength,
                 *lpdwAddressLength
                 ));

    CIpAddress * p = m_List;

    for (DWORD i = 0; (i < *lpdwIndex) && (p != NULL); ++i) {
        p = p->m_Next;
    }

    BOOL found;

    if (p != NULL) {
        found = p->GetAddress(lpbAddress, lpdwAddressLength);
        if (found) {
            ++*lpdwIndex;
        }
    } else {
        found = FALSE;
    }

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
BOOL
CIpAddressList::ThrowOutUnfoundEntries(
    VOID
    )

/*++

Routine Description:

    Throws out (deletes) any addresses that are marked not-found

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - interfaces thrown out

        FALSE   -      "     not "   "

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpAddressList::ThrowOutUnfoundEntries",
                 NULL
                 ));

    CIpAddress * pLast = (CIpAddress *)&m_List;
    CIpAddress * pEntry;
    BOOL bThrownOut = FALSE;

    for (pEntry = m_List; pEntry != NULL; pEntry = pEntry->m_Next) {
        if (!pEntry->IsFound()) {
            pLast->m_Next = pEntry->m_Next;
            delete pEntry;
            bThrownOut = TRUE;
            pEntry = pLast;
        } else {
            pLast = pEntry;
        }
    }

    DEBUG_LEAVE(bThrownOut);

    return bThrownOut;
}

//
// public CAdapterInterface methods
//


CAdapterInterface::CAdapterInterface(
    IN DWORD dwIndex,
    IN DWORD dwType,
    IN DWORD dwSpeed,
    IN LPSTR lpszDescription,
    IN DWORD dwDescriptionLength,
    IN LPBYTE lpPhysicalAddress,
    IN DWORD dwPhysicalAddressLength
    )

/*++

Routine Description:

    CAdapterInterface constructor

Arguments:

    dwIndex             - unique adapter interface index

    dwType              - type of interface

    dwSpeed             - speed of interface

    lpszDescription     - pointer to descriptive name of adapter

    dwDescriptionLength - length of lpszDescription

    lpPhysicalAddress   -
    dwPhysicalAddressLength -


Return Value:

    None.

--*/

{
    if ((lpszDescription != NULL) && (dwDescriptionLength != 0)) {
        m_lpszDescription = new char[dwDescriptionLength + 1];
        if (m_lpszDescription != NULL) {
            memcpy(m_lpszDescription, lpszDescription, dwDescriptionLength);
        } else {
            dwDescriptionLength = 0;
        }
    }

    if ((lpPhysicalAddress != NULL) && (dwPhysicalAddressLength != 0)) {
        m_lpPhysicalAddress = new BYTE[dwPhysicalAddressLength];
        if ( m_lpPhysicalAddress != NULL ) {
            memcpy(m_lpPhysicalAddress, lpPhysicalAddress, dwPhysicalAddressLength);
        }
        else {
            dwPhysicalAddressLength = 0;
        }
    }

    switch( dwType )
    {
        case IF_TYPE_ETHERNET:
            m_dwPhysicalAddressType  = HARDWARE_TYPE_10MB_EITHERNET;
            break;

        case IF_TYPE_TOKENRING:
        case IF_TYPE_FDDI:
            m_dwPhysicalAddressType = HARDWARE_TYPE_IEEE_802;
            break;

        case IF_TYPE_OTHER:
            m_dwPhysicalAddressType = HARDWARE_ARCNET;
            break;

        case IF_TYPE_PPP:
            m_dwPhysicalAddressType = HARDWARE_PPP;
            break;

        default:
            //DhcpPrint(("Invalid HW Type, %ld.\n", IFE.if_type ));
            INET_ASSERT( FALSE );
            m_dwPhysicalAddressType = HARDWARE_ARCNET;
            break;
    }

    m_dwPhysicalAddressLength = dwPhysicalAddressLength;
    m_dwDescriptionLength = dwDescriptionLength;
    m_lpszAdapterName = NULL;
    m_dwIndex = dwIndex;
    m_dwType = dwType;
    m_dwSpeed = dwSpeed;
    m_Flags.Word = 0;
    SetFound(TRUE);
}


CAdapterInterface::~CAdapterInterface(
    VOID
    )

/*++

Routine Description:

    CAdapterInterface destructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (m_lpszDescription != NULL) {
        delete m_lpszDescription;
    }

    if (m_lpPhysicalAddress != NULL) {
        delete m_lpPhysicalAddress;
    }

    if ( m_lpszAdapterName != NULL) {
        FREE_MEMORY(m_lpszAdapterName);
    }
}


BOOL
CAdapterInterface::DhcpDoInformNT5(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN DWORD dwAutoProxyUrlLength
    )

/*++

Routine Description:

     For a given Interface, this nifly little method uses the new wizbang NT 5/Win'98 specific API
       to do the DHCP Inform request and determine an auto-proxy Url that we can use.

     Kinda of nice when we're on NT 5, otherwise we need to pull in the kitchen sink equivlent of
       DHCP code that has been ripped off from the NT 4/Win'95 code base

Arguments:

    lpszAutoProxyUrl  - a piece of memory where we can stuff our new auto-proxy URL

    dwAutoProxyUrlLength - size of the space to store the string above

Return Value:

    BOOL
        TRUE    - successfully talked to server and got Url

        FALSE   - failed to allocate memory or failure talking to TCP/IP or failure to get an Url needed to continue

--*/

{
    DWORD error;
    BYTE bRequestOptions[] = { OPTION_WPAD_URL, OPTION_SUBNET_MASK, OPTION_ROUTER_ADDRESS,
        OPTION_DOMAIN_NAME_SERVERS, OPTION_HOST_NAME, OPTION_DOMAIN_NAME };
    LPBYTE pbOptionList = NULL, pbReturnOptions = NULL;
    DWORD dwOptionListSize, dwReturnOptionSize;
    WCHAR wszAdapterName[(MAX_ADAPTER_NAME_LENGTH + 6)];
    int len;

    len = MultiByteToWideChar(
        CP_ACP,
        0, // flags
        GetAdapterName(),
        -1, // assume null-terminated
        wszAdapterName,
        (MAX_ADAPTER_NAME_LENGTH + 6)
        );

    if ( len == 0 ) {
        return FALSE;  // failed to convert string
    }

    if ( _I_DhcpRequestOptions == NULL )
    {
        error = LoadDllEntryPoints(&DhcpcSvcDllInfo, 0);

        if ( error != ERROR_SUCCESS ) {
            return FALSE;
        }
    }

    error = _I_DhcpRequestOptions(
                wszAdapterName,         // adapter name
                bRequestOptions,        // array of byte codes, each represnts an option
                sizeof(bRequestOptions),// size of array above
                &pbOptionList,          // allocated array of option ids returned from server
                &dwOptionListSize,      // size of above allocated array
                &pbReturnOptions,       // allocated array of option values
                &dwReturnOptionSize
                );

    if ( error == ERROR_SUCCESS )
    {
        DWORD dwNextInc = 0;

        //
        // option ids are returned as byte codes in an array while
        //  the option data contains a byte size descritor followed
        //  by the option data itself which is a second array.
        //
        //  pbReturnedOptions = <OPTION_ID_1><OPTION_ID_2> ... <OPTION_ID_N>
        //  pbOptionValue = (<OPTION_LEN_1><OPTION_DATA_1>) ...
        //          (<OPTION_LEN_N><OPTION_DATA_N>)
        //
        //  pbOptionId is the byte code of the option itself
        //  pbOptionValue points to the option length, or the data following it.
        //
        //

        for ( LPBYTE pbOptionId = pbReturnOptions, pbOptionValue = pbOptionList;
                (pbOptionId < (pbReturnOptions+dwReturnOptionSize) &&
                    pbOptionValue < (pbOptionList+dwOptionListSize));
                 pbOptionId++, pbOptionValue+=(dwNextInc+1) )
        {
            dwNextInc = *pbOptionValue;
            pbOptionValue++; // advance past the size/length byte

            if (*pbOptionId == OPTION_WPAD_URL &&
                dwAutoProxyUrlLength > dwNextInc &&
                lpszAutoProxyUrl)
            {
                strncpy(lpszAutoProxyUrl, (char *) pbOptionValue, dwNextInc);
                return TRUE;
            }
        }
    }

    //
    // BUGBUG [arthurbi] pbOptionList & pbReturnOptions - need to be freed
    //   but the documentation says it needs to be freed by DhcpFreeMemory?!?
    //

    return FALSE;
}



BOOL
CAdapterInterface::CopyAdapterInfoToDhcpContext(
    PDHCP_CONTEXT pDhcpContext
    )
{
    memset ((void *) pDhcpContext, 0, sizeof(DHCP_CONTEXT));

    // hardware address, length, and type
    pDhcpContext->HardwareAddressType = m_dwPhysicalAddressType;
    pDhcpContext->HardwareAddress = m_lpPhysicalAddress;
    pDhcpContext->HardwareAddressLength = m_dwPhysicalAddressLength;

    if (m_IpList.m_List) {
        // Selected IpAddress, NetworkOrder. htonl
        // note: assumed to be in network order
        pDhcpContext->IpAddress = ((m_IpList.m_List)->IpAddress());
        pDhcpContext->IpInterfaceContext = ((m_IpList.m_List)->Context());
    }

    if (m_DhcpList.m_List) {
        // Selected DHCP server address. Network Order. htonl
        // note: assumed to be in network order
        pDhcpContext->DhcpServerAddress = ((m_DhcpList.m_List)->IpAddress());
    }

    pDhcpContext->ClientIdentifier.fSpecified = FALSE;
    pDhcpContext->T2Time = 0;
    // when was the last time an inform was sent?
    pDhcpContext->LastInformSent = 0;
    // seconds passed since boot.
    pDhcpContext->SecondsSinceBoot = 0;

    // the list of options to send and the list of options received
    InitializeListHead(&pDhcpContext->RecdOptionsList);
    InitializeListHead(&pDhcpContext->SendOptionsList);

    // the class this adapter belongs to

    if (  m_lpszAdapterName )
    {
        pDhcpContext->ClassId = (unsigned char *) m_lpszAdapterName;
        pDhcpContext->ClassIdLength = lstrlen(m_lpszAdapterName);
    }
    else
    {
        pDhcpContext->ClassId = NULL;
        pDhcpContext->ClassIdLength = 0;
    }

    // Message buffer to send and receive DHCP message.
    pDhcpContext->MessageBuffer = (PDHCP_MESSAGE) pDhcpContext->szMessageBuffer;
    memset(pDhcpContext->szMessageBuffer, 0, sizeof(pDhcpContext->szMessageBuffer));

    //LocalInfo = (PLOCAL_CONTEXT_INFO)((*pDhcpContext)->LocalInformation);
    //LocalInfo->IpInterfaceContext = IpInterfaceContext;
    //LocalInfo->IpInterfaceInstance = IpInterfaceInstance;
    // IpInterfaceInstance is filled in make context

    pDhcpContext->Socket = INVALID_SOCKET;
    pDhcpContext->State.Plumbed = TRUE;
    pDhcpContext->State.ServerReached = FALSE;
    pDhcpContext->State.AutonetEnabled= FALSE;
    pDhcpContext->State.HasBeenLooked = FALSE;
    pDhcpContext->State.DhcpEnabled   = FALSE;
    pDhcpContext->State.AutoMode      = FALSE;
    pDhcpContext->State.MediaState    = FALSE;
    pDhcpContext->State.MDhcp         = FALSE;
    pDhcpContext->State.PowerResumed  = FALSE;
    pDhcpContext->State.Broadcast     = FALSE;

    return TRUE;
}



//
// public CIpConfig methods
//


CIpConfig::CIpConfig(
    VOID
    )

/*++

Routine Description:

    CIpConfig constructor - initializes the object & loads the requird DLLs if
    not already loaded

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "CIpConfig::CIpConfig",
                 NULL
                 ));

    InitializeListHead(&m_List);
    m_dwNumberOfInterfaces = 0;
    m_Loaded = TRI_STATE_UNKNOWN;

    DWORD error = LoadEntryPoints();

    if (error == ERROR_SUCCESS) {
#ifndef unix
        GetAdapterList();
#endif /* unix */
    }

    DEBUG_LEAVE(0);
}


CIpConfig::~CIpConfig()

/*++

Routine Description:

    CIpConfig destructor - destroys this object and unloads (or reduces the
    reference count on) the DLLs

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "CIpConfig::~CIpConfig",
                 NULL
                 ));

    while (!IsListEmpty(&m_List)) {

        PLIST_ENTRY pEntry = RemoveHeadList(&m_List);

        //
        // BUGBUG - need CONTAINING_RECORD() if m_List is not @ start of
        //          CAdapterInterface
        //

        CAdapterInterface * pInterface = (CAdapterInterface *)pEntry;

        delete pInterface;
    }

    UnloadEntryPoints();
    CloseTcpipDriverHandle();

    DEBUG_LEAVE(0);
}


BOOL
CIpConfig::GetRouterAddress(
    IN LPBYTE lpbInterfaceAddress OPTIONAL,
    IN DWORD dwInterfaceAddressLength,
    IN OUT LPDWORD lpdwIndex,
    OUT LPBYTE lpbAddress,
    IN OUT LPDWORD lpdwAddressLength
    )

/*++

Routine Description:

    Returns the *lpdwIndex'th router address belonging to the interface
    corresponding to the address in lpbInterfaceAddress

Arguments:

    lpbInterfaceAddress         - pointer to interface address

    dwInterfaceAddressLength    - length of interface address

    lpdwIndex                   - index of router address to return

    lpbAddress                  - returned router address

    lpdwAddressLength           - length of router address

Return Value:

    BOOL
        TRUE    - *lpdwIndex'th router address returned for requested interface

        FALSE   - requested address not returned

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::GetRouterAddress",
                 "%#x, %d, %#x [%d], %#x, %#x [%d]",
                 lpbInterfaceAddress,
                 dwInterfaceAddressLength,
                 lpdwIndex,
                 *lpdwIndex,
                 lpbAddress,
                 lpdwAddressLength,
                 *lpdwAddressLength
                 ));

    //
    // for now, we default to 1st interface
    //

    INET_ASSERT(lpbInterfaceAddress == NULL);
    INET_ASSERT(dwInterfaceAddressLength == sizeof(DWORD));

    BOOL found;

    //
    // no one uses this any more
    //

    INET_ASSERT(FALSE);

    //if (!IsListEmpty(&m_List)) {
    //    found = ((CAdapterInterface *)m_List.Flink)->m_RouterList.GetAddress(
    //                lpdwIndex,
    //                lpbAddress,
    //                lpdwAddressLength
    //                );
    //} else {
        found = FALSE;
    //}

    DEBUG_LEAVE(found);

    return found;
}


BOOL
CIpConfig::GetDnsAddress(
    IN LPBYTE lpbInterfaceAddress OPTIONAL,
    IN DWORD dwInterfaceAddressLength,
    IN OUT LPDWORD lpdwIndex,
    OUT LPBYTE lpbAddress,
    IN OUT LPDWORD lpdwAddressLength
    )

/*++

Routine Description:

    Returns the *lpdwIndex'th DNS address belonging to the interface
    corresponding to the address in lpbInterfaceAddress

Arguments:

    lpbInterfaceAddress         - pointer to interface address

    dwInterfaceAddressLength    - length of interface address

    lpdwIndex                   - index of DNS address to return

    lpbAddress                  - returned DNS address

    lpdwAddressLength           - length of DNS address

Return Value:

    BOOL
        TRUE    - *lpdwIndex'th DNS address returned for requested interface

        FALSE   - requested address not returned

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::GetDnsAddress",
                 "%#x, %d, %#x [%d], %#x, %#x [%d]",
                 lpbInterfaceAddress,
                 dwInterfaceAddressLength,
                 lpdwIndex,
                 *lpdwIndex,
                 lpbAddress,
                 lpdwAddressLength,
                 *lpdwAddressLength
                 ));

    //
    // for now, we only return the global DNS info
    //

    INET_ASSERT(lpbInterfaceAddress == NULL);
    INET_ASSERT(dwInterfaceAddressLength == sizeof(DWORD));

    BOOL found;

    if (!m_DnsList.IsEmpty()) {
        found = m_DnsList.GetAddress(lpdwIndex,
                                     lpbAddress,
                                     lpdwAddressLength
                                     );
    } else {
        found = FALSE;
    }

    DEBUG_LEAVE(found);

    return found;
}


BOOL
CIpConfig::IsKnownIpAddress(
    IN LPBYTE lpbInterfaceAddress OPTIONAL,
    IN DWORD dwInterfaceAddressLength,
    IN LPBYTE lpbAddress,
    IN DWORD dwAddressLength
    )

/*++

Routine Description:

    Return TRUE if lpbAddress is a known interface address

Arguments:

    lpbInterfaceAddress         - pointer to interface address

    dwInterfaceAddressLength    - length of interface address

    lpbAddress                  - pointer to address to check

    dwAddressLength             - length of address

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::IsKnownIpAddress",
                 "%#x, %d, %#x, %d",
                 lpbInterfaceAddress,
                 dwInterfaceAddressLength,
                 lpbAddress,
                 dwAddressLength
                 ));

    BOOL found = FALSE;

    for (CAdapterInterface * pEntry = (CAdapterInterface *)m_List.Flink;
         pEntry != (CAdapterInterface *)&m_List.Flink;
         pEntry = (CAdapterInterface *)pEntry->m_List.Flink) {

        if (pEntry->FindIpAddress(*(LPDWORD)lpbAddress)) {
            found = TRUE;
            break;
        }
    }

    DEBUG_LEAVE(found);

    return found;
}


BOOL
CIpConfig::Refresh(
    VOID
    )

/*++

Routine Description:

    Refreshes the interface information - re-reads the interfaces and IP
    addresses

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - interfaces or IP address changed

        FALSE   - nothing changed

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::Refresh",
                 NULL
                 ));

    BOOL bChanged;

    GetAdapterList(&bChanged);

    if (bChanged) {
//dprintf("flushing hostent cache\n");
//        FlushHostentCache();
    }

    DEBUG_LEAVE(bChanged);

    return bChanged;
}

//
// private CIpConfig methods
//


PRIVATE
BOOL
CIpConfig::GetAdapterList(
    OUT LPBOOL lpbChanged
    )

/*++

Routine Description:

    Builds a list of interfaces corresponding to physical and logical adapters,
      Uses Win'95 and NT 4 private VxD driver/registry entry points to get this data.

Arguments:

    lpbChanged  - if present, returns interface changed state

Return Value:

    BOOL
        TRUE    - successfully built list

        FALSE   - failed to allocate memory or failure talking to TCP/IP

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::GetAdapterList",
                 "%#x",
                 lpbChanged
                 ));

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    TDIObjectID id;
    UINT numberOfEntities;
    TDIEntityID* pEntity;
    TDIEntityID* entityList = NULL;
    IPRouteEntry* routeTable = NULL;
    LPVOID buffer = NULL;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    BOOL ok = FALSE;
    UINT i; // major loop index
    UINT j; // minor loop index
    BOOL bChanged = FALSE;

    //
    // default is interfaces unchanged
    //

    if (lpbChanged) {
        *lpbChanged = FALSE;
    }

    //
    // On NT 5 we override and use a different method for
    //   getting network settings.
    //

    if ( GlobalPlatformVersion5 ) {
        return GetAdapterListOnNT5();
    }

    //
    // get the list of entities supported by TCP/IP then make 2 passes on the
    // list. Pass 1 scans for IF_ENTITY's (interface entities perhaps?) which
    // describe adapter instances (physical and virtual). Once we have our list
    // of adapters, on pass 2 we look for CL_NL_ENTITY's (connection-less
    // network layer entities peut-etre?) which will give us the list of IP
    // addresses for the adapters we found in pass 1
    //

    numberOfEntities = GetEntityList(&entityList);
    if (numberOfEntities == 0) {

        INET_ASSERT(entityList == NULL);

        DEBUG_PRINT(UTIL,
                    ERROR,
                    ("GetAdapterList: failed to get entity list\n"
                    ));

        goto quit;
    }

    //
    // first off, mark all the current interfaces (if any), including current
    // IP addresses, as not found
    //

    SetNotFound();

    //
    // pass 1
    //

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity) {

        DEBUG_PRINT(UTIL,
                    INFO,
                    ("Pass 1: Entity %#x (%s) Instance #%d\n",
                    pEntity->tei_entity,
                    InternetMapEntity(pEntity->tei_entity),
                    pEntity->tei_instance
                    ));

        if (pEntity->tei_entity != IF_ENTITY) {

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("Entity %#x (%s) Instance #%d not IF_ENTITY - skipping\n",
                        pEntity->tei_entity,
                        InternetMapEntity(pEntity->tei_entity),
                        pEntity->tei_instance
                        ));

            continue;
        }

        //
        // IF_ENTITY: this entity/instance describes an adapter
        //

        DWORD isMib;
        BYTE info[sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
        IFEntry* pIfEntry = (IFEntry*)info;
        int len;

        //
        // find out if this entity supports MIB requests
        //

        memset(&req, 0, sizeof(req));

        id.toi_entity = *pEntity;
        id.toi_class = INFO_CLASS_GENERIC;
        id.toi_type = INFO_TYPE_PROVIDER;
        id.toi_id = ENTITY_TYPE_ID;

        req.ID = id;

        inputLen = sizeof(req);
        outputLen = sizeof(isMib);

        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)&isMib,
                           &outputLen
                           );

        //
        // BUGBUG - this returns 0 as outputLen
        //

//        if ((status != TDI_SUCCESS) || (outputLen != sizeof(isMib))) {
        if (status != TDI_SUCCESS) {

            //
            // unexpected results - bail out
            //

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("WsControl(ENTITY_TYPE_ID): status = %d, outputLen = %d\n",
                        status,
                        outputLen
                        ));

            goto error_exit;
        }
        if (isMib != IF_MIB) {

            //
            // entity doesn't support MIB requests - try another
            //

            DEBUG_PRINT(UTIL,
                        WARNING,
                        ("Entity %#x, Instance #%d doesn't support MIB (%#x)\n",
                        id.toi_entity.tei_entity,
                        id.toi_entity.tei_instance,
                        isMib
                        ));

            continue;
        }

        //
        // MIB requests supported - query the adapter info
        //

        id.toi_class = INFO_CLASS_PROTOCOL;
        id.toi_id = IF_MIB_STATS_ID;

        memset(&req, 0, sizeof(req));
        req.ID = id;

        inputLen = sizeof(req);
        outputLen = sizeof(info);

        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)&info,
                           &outputLen
                           );
        if (status != TDI_SUCCESS) {

            //
            // unexpected results - bail out
            //

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("WsControl(IF_MIB_STATS_ID) returns %d\n",
                        status
                        ));

            goto error_exit;
        }

        //
        // we only want physical adapters
        //

        if (!IS_INTERESTING_ADAPTER(pIfEntry)) {

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("ignoring adapter #%d [%s]\n",
                        pIfEntry->if_index,
                        InternetMapInterface(pIfEntry->if_type)
                        ));

            continue;
        }

        //
        // got this adapter info ok. Find or create an interface object and fill
        // in what we can
        //

        CAdapterInterface * pInterface;

        len = min(MAX_ADAPTER_ADDRESS_LENGTH, (size_t)pIfEntry->if_physaddrlen);

        pInterface = FindOrCreateInterface(pIfEntry->if_index,
                                           pIfEntry->if_type,
                                           pIfEntry->if_speed,
                                           (LPSTR)pIfEntry->if_descr,
                                           pIfEntry->if_descrlen,
                                           (LPBYTE)pIfEntry->if_physaddr,
                                           (DWORD) len
                                           );
        if (pInterface == NULL) {

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("failed to allocate memory for CAdapterInterface\n"
                        ));

            goto error_exit;
        }
    }

    //
    // pass 2
    //

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity) {

        DEBUG_PRINT(UTIL,
                    INFO,
                    ("Pass 2: Entity %#x (%s) Instance %d\n",
                    pEntity->tei_entity,
                    InternetMapEntity(pEntity->tei_entity),
                    pEntity->tei_instance
                    ));

        if (pEntity->tei_entity != CL_NL_ENTITY) {

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("Entity %#x (%s) Instance %d - not CL_NL_ENTITY - skipping\n",
                        pEntity->tei_entity,
                        InternetMapEntity(pEntity->tei_entity),
                        pEntity->tei_instance
                        ));

            continue;
        }

        IPSNMPInfo info;
        DWORD type;

        //
        // first off, see if this network layer entity supports IP
        //

        memset(&req, 0, sizeof(req));

        id.toi_entity = *pEntity;
        id.toi_class = INFO_CLASS_GENERIC;
        id.toi_type = INFO_TYPE_PROVIDER;
        id.toi_id = ENTITY_TYPE_ID;

        req.ID = id;

        inputLen = sizeof(req);
        outputLen = sizeof(type);

        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)&type,
                           &outputLen
                           );

        //
        // BUGBUG - this returns 0 as outputLen
        //

//        if ((status != TDI_SUCCESS) || (outputLen != sizeof(type))) {
        if (status != TDI_SUCCESS) {

            //
            // unexpected results - bail out
            //

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("WsControl(ENTITY_TYPE_ID): status = %d, outputLen = %d\n",
                        status,
                        outputLen
                        ));

            goto error_exit;
        }
        if (type != CL_NL_IP) {

            //
            // nope, not IP - try next one
            //

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("CL_NL_ENTITY #%d not CL_NL_IP - skipping\n",
                        pEntity->tei_instance
                        ));

            continue;
        }

        //
        // okay, this NL provider supports IP. Let's get them addresses: First
        // we find out how many by getting the SNMP stats and looking at the
        // number of addresses supported by this interface
        //

        memset(&req, 0, sizeof(req));

        id.toi_class = INFO_CLASS_PROTOCOL;
        id.toi_id = IP_MIB_STATS_ID;

        req.ID = id;

        inputLen = sizeof(req);
        outputLen = sizeof(info);

        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)&info,
                           &outputLen
                           );
        if ((status != TDI_SUCCESS) || (outputLen != sizeof(info))) {

            //
            // unexpected results - bail out
            //

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("WsControl(IP_MIB_STATS_ID): status = %d, outputLen = %d\n",
                        status,
                        outputLen
                        ));

            goto error_exit;
        }

        //
        // get the IP addresses & subnet masks
        //

        if (info.ipsi_numaddr != 0) {

            //
            // this interface has some addresses. What are they?
            //

            UINT numberOfAddresses;
            IPAddrEntry* pAddr;

            outputLen = info.ipsi_numaddr * sizeof(IPAddrEntry);
            buffer = (LPVOID)ALLOCATE_MEMORY(LMEM_FIXED, outputLen);
            if (buffer == NULL) {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(UTIL,
                            ERROR,
                            ("failed to allocate %d bytes\n",
                            outputLen
                            ));

                goto error_exit;
            }

            memset(&req, 0, sizeof(req));

            id.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

            req.ID = id;

            inputLen = sizeof(req);

            status = WsControl(IPPROTO_TCP,
                               WSCNTL_TCPIP_QUERY_INFO,
                               (LPVOID)&req,
                               &inputLen,
                               (LPVOID)buffer,
                               &outputLen
                               );
            if (status != TDI_SUCCESS) {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(UTIL,
                            ERROR,
                            ("WsControl(IP_MIB_ADDRTABLE_ENTRY_ID): status = %d, outputLen = %d\n",
                            status,
                            outputLen
                            ));

                goto error_exit;
            }

            //
            // now loop through this list of IP addresses, applying them
            // to the correct adapter
            //

            numberOfAddresses = min((UINT)(outputLen / sizeof(IPAddrEntry)),
                                    (UINT)info.ipsi_numaddr
                                    );

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("%d IP addresses\n",
                        numberOfAddresses
                        ));

            pAddr = (IPAddrEntry *)buffer;
            for (j = 0; j < numberOfAddresses; ++j, ++pAddr) {

                DEBUG_PRINT(UTIL,
                            INFO,
                            ("IP address %d.%d.%d.%d, index %d, context %d\n",
                            ((LPBYTE)&pAddr->iae_addr)[0] & 0xff,
                            ((LPBYTE)&pAddr->iae_addr)[1] & 0xff,
                            ((LPBYTE)&pAddr->iae_addr)[2] & 0xff,
                            ((LPBYTE)&pAddr->iae_addr)[3] & 0xff,
                            pAddr->iae_index,
                            pAddr->iae_context
                            ));

                CAdapterInterface * pInterface = FindInterface(pAddr->iae_index);

                if (pInterface != NULL) {

                    CIpAddress * pIpAddress;

                    pIpAddress = pInterface->m_IpList.Find(pAddr->iae_addr,
                                                           pAddr->iae_mask
                                                           );
                    if (pIpAddress == NULL) {
                        pInterface->m_IpList.Add(pAddr->iae_addr,
                                                 pAddr->iae_mask,
                                                 pAddr->iae_context
                                                 );

                        //
                        // added an address - interface is changed
                        //
//dprintf("adding IP address %d.%d.%d.%d - changed\n",
//        ((LPBYTE)&pAddr->iae_addr)[0] & 0xff,
//        ((LPBYTE)&pAddr->iae_addr)[1] & 0xff,
//        ((LPBYTE)&pAddr->iae_addr)[2] & 0xff,
//        ((LPBYTE)&pAddr->iae_addr)[3] & 0xff
//        );
                        bChanged = TRUE;
                    } else {

                        INET_ASSERT(pAddr->iae_context == pIpAddress->Context());

                        pIpAddress->SetFound(TRUE);
                    }
                }
            }

            INET_ASSERT(buffer);

            FREE_MEMORY(buffer);

            buffer = NULL;
        }

        //
        // get the gateway server IP address(es)
        //

        //
        // We don't need this information any more
        //

#if 0
        if (info.ipsi_numroutes != 0) {

            IPRouteEntry* pRoute;

            memset(&req, 0, sizeof(req));

            id.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

            req.ID = id;

            inputLen = sizeof(req);

            //
            // Warning: platform specifics; Win95 structure size is different
            // than NT 4.0
            //

            //
            // BUGBUG - this will probably have to be checked for version # on
            //          Memphis
            //

            int structLength = (GlobalPlatformType == PLATFORM_TYPE_WINNT)
                             ? sizeof(IPRouteEntry) : sizeof(IPRouteEntry95);

            outputLen = structLength * info.ipsi_numroutes;

            //
            // the route table may have grown since we got the SNMP stats
            //

            for (j = 0; j < 4; ++j) {

                DWORD previousOutputLen = outputLen;

                routeTable = (IPRouteEntry*)ResizeBuffer(routeTable,
                                                         outputLen,
                                                         FALSE
                                                         );
                if (routeTable == NULL) {
                    goto error_exit;
                }

                status = WsControl(IPPROTO_TCP,
                                   WSCNTL_TCPIP_QUERY_INFO,
                                   (LPVOID)&req,
                                   &inputLen,
                                   (LPVOID)routeTable,
                                   &outputLen
                                   );
                if (status != TDI_SUCCESS) {

                    //
                    // unexpected results - bail out
                    //

                    DEBUG_PRINT(UTIL,
                                ERROR,
                                ("WsControl(IP_MIB_RTTABLE_ENTRY_ID): status = %d, outputLen = %d\n",
                                status,
                                outputLen
                                ));

                    goto error_exit;
                }
                if (outputLen <= previousOutputLen) {
                    break;
                }
            }

            UINT numberOfRoutes = (UINT)(outputLen / sizeof(IPRouteEntry));

            for (j = 0, pRoute = routeTable; j < numberOfRoutes; ++j) {

                //
                // the gateway address has a destination of 0.0.0.0
                //

                if (pRoute->ire_dest == INADDR_ANY) {

                    CAdapterInterface * pInterface;

                    pInterface = FindInterface(pRoute->ire_index);
                    if (pInterface != NULL) {

                        DEBUG_PRINT(UTIL,
                                    INFO,
                                    ("router address %d.%d.%d.%d, index %d\n",
                                    ((LPBYTE)&pRoute->ire_nexthop)[0] & 0xff,
                                    ((LPBYTE)&pRoute->ire_nexthop)[1] & 0xff,
                                    ((LPBYTE)&pRoute->ire_nexthop)[2] & 0xff,
                                    ((LPBYTE)&pRoute->ire_nexthop)[3] & 0xff,
                                    pRoute->ire_index
                                    ));

                        CIpAddress * pIpAddress;

                        pIpAddress = pInterface->m_RouterList.Find(pRoute->ire_nexthop);
                        if (pIpAddress == NULL) {
                            pInterface->m_RouterList.Add(pRoute->ire_nexthop);

                            //
                            // added a router address - interface is changed
                            //

//dprintf("adding router address %d.%d.%d.%d - changed\n",
//        ((LPBYTE)&pRoute->ire_nexthop)[0] & 0xff,
//        ((LPBYTE)&pRoute->ire_nexthop)[1] & 0xff,
//        ((LPBYTE)&pRoute->ire_nexthop)[2] & 0xff,
//        ((LPBYTE)&pRoute->ire_nexthop)[3] & 0xff
//        );
                            bChanged = TRUE;
                        } else {
                            pIpAddress->SetFound(TRUE);
                        }
                    }
                } else {

                    DEBUG_PRINT(UTIL,
                                INFO,
                                ("rejecting router address %d.%d.%d.%d, index %d\n",
                                ((LPBYTE)&pRoute->ire_nexthop)[0] & 0xff,
                                ((LPBYTE)&pRoute->ire_nexthop)[1] & 0xff,
                                ((LPBYTE)&pRoute->ire_nexthop)[2] & 0xff,
                                ((LPBYTE)&pRoute->ire_nexthop)[3] & 0xff,
                                pRoute->ire_index
                                ));

                }

                pRoute = (IPRouteEntry *)((LPBYTE)pRoute + structLength);
            }

            INET_ASSERT(routeTable);

            FREE_MEMORY(routeTable);

            routeTable = NULL;
        }
#endif // if 0, disable gathering of router addresses

    }

    //
    // add the DNS servers, read from registry or DHCP depending on platform.
    // Even if we don't get any DNS servers, we deem that this function has
    // succeeded
    //

    char dnsBuffer[1024];   // arbitrary (how many DNS entries?)
    UINT error;

    error = SockGetSingleValue(CONFIG_NAME_SERVER,
                               (LPBYTE)dnsBuffer,
                               sizeof(dnsBuffer)
                               );
    if (error == ERROR_SUCCESS) {
        //m_DnsList.Clear();

        char ipString[4 * 4];
        LPSTR p = dnsBuffer;
        DWORD buflen = (DWORD)lstrlen(dnsBuffer);

        do {
            if (SkipWhitespace(&p, &buflen)) {

                int i = 0;

                while ((*p != '\0')
                       && (*p != ',')
                       && (buflen != 0)
                       && (i < sizeof(ipString))
                       && !isspace(*p)) {
                    ipString[i++] = *p++;
                    --buflen;
                }
                ipString[i] = '\0';

                DWORD ipAddress = _I_inet_addr(ipString);

                if (IS_VALID_NON_LOOPBACK_IP_ADDRESS(ipAddress)) {

                    CIpAddress * pIpAddress;

                    pIpAddress = m_DnsList.Find(ipAddress);
                    if (pIpAddress == NULL) {
                        m_DnsList.Add(ipAddress);

                        //
                        // added a DNS address - interface is changed
                        //

//dprintf("adding DNS address %d.%d.%d.%d - changed\n",
//        ((LPBYTE)&ipAddress)[0] & 0xff,
//        ((LPBYTE)&ipAddress)[1] & 0xff,
//        ((LPBYTE)&ipAddress)[2] & 0xff,
//        ((LPBYTE)&ipAddress)[3] & 0xff
//        );
                        bChanged = TRUE;
                    } else {
                        pIpAddress->SetFound(TRUE);
                    }
                }
                while ((*p == ',') && (buflen != 0)) {
                    ++p;
                    --buflen;
                }
            } else {
                break;
            }
        } while (TRUE);
    }

    //
    // Refresh registry settings of DHCP server stuff
    //  and figure out what DHCP server we have
    //

    GetAdapterInfo();

    //
    // throw out any adapter interfaces which were not found this time. This may
    // happen if we support PnP devices that are unplugged
    //

    BOOL bThrownOut;

    bThrownOut = ThrowOutUnfoundEntries();
    if (!bChanged) {
        bChanged = bThrownOut;
    }

    INET_ASSERT(entityList != NULL);

    FREE_MEMORY(entityList);

    ok = TRUE;

    //
    // return the change state of the interfaces, if required
    //

    if (lpbChanged) {
        *lpbChanged = bChanged;
    }

quit:

    if (routeTable != NULL) {
        FREE_MEMORY(routeTable);
    }

    if (buffer != NULL) {
        FREE_MEMORY(buffer);
    }

    DEBUG_LEAVE(ok);

    return ok;

error_exit:

    //
    // here because of an error. Throw out all interfaces
    //

    SetNotFound();
    ThrowOutUnfoundEntries();

    INET_ASSERT(!ok);

    goto quit;
}



PRIVATE
BOOL
CIpConfig::GetAdapterListOnNT5(
    OUT LPBOOL lpbChanged
    )

/*++

Routine Description:

    Builds a list of interfaces corresponding to physical and logical adapters
     using the new NT 5 and Win98 APIs.

    BUGBUG [arthurbi] - turned off for Win '98 because we were getting erronous
      data values from the APIs (we currently fall back to old Win'95 code on '98).

Arguments:

    lpbChanged  - if present, returns interface changed state

Return Value:

    BOOL
        TRUE    - successfully built list

        FALSE   - failed to allocate memory or failure talking to TCP/IP

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::GetAdapterListOnNT5",
                 "%#x",
                 lpbChanged
                 ));

    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    BOOL ok = FALSE;
    UINT i; // major loop index
    UINT j; // minor loop index
    BOOL bChanged = FALSE;
    int len;

    IP_ADAPTER_INFO AdapterInfo[15];
    PIP_ADAPTER_INFO pAdapterInfo;
    DWORD dwError;
    ULONG uSize;

    //
    // Load the IPHLPAPI DLL, cause we need this function find adapter info on NT 5/Win98
    //

    if ( _I_GetAdaptersInfo == NULL )
    {
        DEBUG_PRINT(UTIL,
            ERROR,
            ("GetAdapterListOnNT5: IPHLPAPI dll could not be found with correct entry point\n"
            ));

        goto quit;
    }

    //
    // get the list of adapters supported by TCP/IP
    //

    uSize = sizeof(IP_ADAPTER_INFO)*15;
    pAdapterInfo = AdapterInfo;
    dwError = _I_GetAdaptersInfo(pAdapterInfo, &uSize);

    if ( dwError != ERROR_SUCCESS )
    {
        //
        // BUGBUG [arthurbi] handle the case where we have more than 15 adapters,
        //    need to be brave and start allocating memory for a change.
        //

        DEBUG_PRINT(UTIL,
            ERROR,
            ("GetAdapterListOnNT5: failed to get adapters list\n"
            ));

        goto quit;
    }

    //
    // first off, mark all the current interfaces (if any), including current
    // IP addresses, as not found
    //

    SetNotFound();

    //
    // pass 1
    //

    for (pAdapterInfo = AdapterInfo; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next)
    {

        DEBUG_PRINT(UTIL,
                    INFO,
                    ("Adapter Pass: [#%u] Adapter name=%s, description=%s\n",
                    pAdapterInfo->Index,
                    pAdapterInfo->AdapterName,
                    pAdapterInfo->Description
                    ));

        //
        // we only want physical adapters
        //

        if (!IS_INTERESTING_ADAPTER_NT5(pAdapterInfo)) {

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("ignoring adapter #%u [%s]\n",
                        pAdapterInfo->Index,
                        InternetMapInterfaceOnNT5(pAdapterInfo->Type)
                        ));

            continue;
        }

        //
        // got this adapter info ok. Find or create an interface object and fill
        // in what we can
        //

        CAdapterInterface * pInterface;

        len = min(MAX_ADAPTER_ADDRESS_LENGTH, (size_t)pAdapterInfo->AddressLength);


        pInterface = FindOrCreateInterface(pAdapterInfo->Index,
                                           pAdapterInfo->Type,
                                           0,                   // speed
                                           pAdapterInfo->Description,
                                           lstrlen(pAdapterInfo->Description),
                                           pAdapterInfo->Address,
                                           (DWORD) len
                                           );
        if (pInterface == NULL) {

            DEBUG_PRINT(UTIL,
                        ERROR,
                        ("failed to allocate memory for CAdapterInterface\n"
                        ));

            goto error_exit;
        }

        //
        // Update the Adapter Name, this is the critical glue to make the new NT 5 DHCP Apis work,
        //   as they need this Adapter name as an ID to work.
        //

        if ( pInterface->GetAdapterName() == NULL )  {
            pInterface->SetAdapterName(pAdapterInfo->AdapterName);
        } else {
            INET_ASSERT(lstrcmpi(pInterface->GetAdapterName(), pAdapterInfo->AdapterName) == 0 );
        }

        //
        // Update the IP address found in the structure, as we're not getting anything back with this filled in.
        //

        if (  pAdapterInfo->CurrentIpAddress == NULL )
        {
            pAdapterInfo->CurrentIpAddress = &pAdapterInfo->IpAddressList;
        }
        else
        {
            INET_ASSERT(FALSE);  // want to know about this case.
        }

        //
        // Gather the IP addresses from the structure, doing all the necessary,
        //  IP string to network-ordered DWORD thingie usable for winsock.
        //
        //  BUGBUG [arthurbi] do we really need to do this anymore? As the
        //    the new NT 5 APIs can handle themselves without IP addresses...
        //


        if ( pAdapterInfo->CurrentIpAddress->IpAddress.String &&
             pAdapterInfo->CurrentIpAddress->IpMask.String )
        {
            DWORD dwAddress = _I_inet_addr(pAdapterInfo->CurrentIpAddress->IpAddress.String);
            DWORD dwMask = _I_inet_addr(pAdapterInfo->CurrentIpAddress->IpMask.String);
            DWORD dwContext = pAdapterInfo->CurrentIpAddress->Context;

            if ( dwAddress   != INADDR_NONE &&
                 dwMask      != INADDR_NONE  )
            {

                DEBUG_PRINT(UTIL,
                            INFO,
                            ("IP address %d.%d.%d.%d, index %d, context %d\n",
                            ((LPBYTE)&dwAddress)[0] & 0xff,
                            ((LPBYTE)&dwAddress)[1] & 0xff,
                            ((LPBYTE)&dwAddress)[2] & 0xff,
                            ((LPBYTE)&dwAddress)[3] & 0xff,
                            pAdapterInfo->Index,
                            dwContext
                            ));

                INET_ASSERT(pInterface != NULL);

                CIpAddress * pIpAddress;

                pIpAddress = pInterface->m_IpList.Find(dwAddress,
                                                       dwMask
                                                       );
                if (pIpAddress == NULL) {
                    pInterface->m_IpList.Add(dwAddress,
                                             dwMask,
                                             dwContext
                                             );

                    //
                    // added an address - interface is changed
                    //

                    bChanged = TRUE;
                } else {

                    INET_ASSERT(dwContext == pIpAddress->Context());

                    pIpAddress->SetFound(TRUE);
                }
            }
        }

        //
        // Gather DHCP server addresses to use, once again do we need this info on NT 5?
        //

        if ( pAdapterInfo->DhcpEnabled )
        {
            PIP_ADDR_STRING pDhcpServer;
            INET_ASSERT(pInterface != NULL);

            for ( pDhcpServer = &pAdapterInfo->DhcpServer; pDhcpServer; pDhcpServer = pDhcpServer->Next )
            {
                CIpAddress * pDhcpAddress;

                DWORD dwAddress = _I_inet_addr(pDhcpServer->IpAddress.String);
                DWORD dwMask = _I_inet_addr(pDhcpServer->IpMask.String);
                DWORD dwContext = pDhcpServer->Context;

                if ( dwAddress   != INADDR_NONE )
                {
                    CIpAddress * pIpAddress;

                    pInterface->SetDhcp();

                    pIpAddress = pInterface->m_DhcpList.Find(dwAddress,
                                                           dwMask
                                                           );
                    if (pIpAddress == NULL)
                    {
                        pInterface->m_DhcpList.Add(dwAddress,
                                                  dwMask,
                                                  dwContext
                                                  );

                        //
                        // added an address - interface is changed
                        //

                        bChanged = TRUE;
                    } else {

                        INET_ASSERT(dwContext == pIpAddress->Context());

                        pIpAddress->SetFound(TRUE);
                    }
                }

            }
        }
    }

    //
    // add the DNS servers, read from registry or DHCP depending on platform.
    // Even if we don't get any DNS servers, we deem that this function has
    // succeeded
    //

    char dnsBuffer[1024];   // arbitrary (how many DNS entries?)
    UINT error;

    error = SockGetSingleValue(CONFIG_NAME_SERVER,
                               (LPBYTE)dnsBuffer,
                               sizeof(dnsBuffer)
                               );
    if (error == ERROR_SUCCESS) {
        //m_DnsList.Clear();

        char ipString[4 * 4];
        LPSTR p = dnsBuffer;
        DWORD buflen = (DWORD)lstrlen(dnsBuffer);

        do {
            if (SkipWhitespace(&p, &buflen)) {

                int i = 0;

                while ((*p != '\0')
                       && (*p != ',')
                       && (buflen != 0)
                       && (i < sizeof(ipString))
                       && !isspace(*p)) {
                    ipString[i++] = *p++;
                    --buflen;
                }
                ipString[i] = '\0';

                DWORD ipAddress = _I_inet_addr(ipString);

                if (IS_VALID_NON_LOOPBACK_IP_ADDRESS(ipAddress)) {

                    CIpAddress * pIpAddress;

                    pIpAddress = m_DnsList.Find(ipAddress);
                    if (pIpAddress == NULL) {
                        m_DnsList.Add(ipAddress);

                        //
                        // added a DNS address - interface is changed
                        //

                        bChanged = TRUE;
                    } else {
                        pIpAddress->SetFound(TRUE);
                    }
                }
                while ((*p == ',') && (buflen != 0)) {
                    ++p;
                    --buflen;
                }
            } else {
                break;
            }
        } while (TRUE);
    }

    //
    // throw out any adapter interfaces which were not found this time. This may
    // happen if we support PnP devices that are unplugged
    //
    //  Do we need to still do this ???
    //

    BOOL bThrownOut;

    bThrownOut = ThrowOutUnfoundEntries();
    if (!bChanged) {
        bChanged = bThrownOut;
    }

    ok = TRUE;

    //
    // return the change state of the interfaces, if required
    //

    if (lpbChanged) {
        *lpbChanged = bChanged;
    }

quit:

    DEBUG_LEAVE(ok);

    return ok;

error_exit:

    //
    // here because of an error. Throw out all interfaces
    //

    SetNotFound();
    ThrowOutUnfoundEntries();

    INET_ASSERT(!ok);

    goto quit;
}


BOOL
CIpConfig::DoInformsOnEachInterface(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN     DWORD dwAutoProxyUrlLength
    )
{
    for (CAdapterInterface * pEntry = (CAdapterInterface *)m_List.Flink;
         pEntry != (CAdapterInterface *)&m_List.Flink;
         pEntry =  (CAdapterInterface *)pEntry->m_List.Flink)
    {
        if ( pEntry->IsDhcp() )
        {
            BOOL fSuccess;

            if ( GlobalPlatformVersion5 )
            {
                fSuccess = pEntry->DhcpDoInformNT5(
                                lpszAutoProxyUrl,
                                dwAutoProxyUrlLength
                                );
            }
            else
            {
                fSuccess = DhcpDoInform(     // send an inform packet if necessary
                        pEntry,
                        FALSE,
                        lpszAutoProxyUrl,
                        dwAutoProxyUrlLength
                        );
            }

            if ( fSuccess ) {
                return TRUE;
            }
        }
    }

    return FALSE;
}


/*******************************************************************************
 *
 *  GetAdapterInfo
 *
 *  Gets a list of all adapters to which TCP/IP is bound and reads the per-
 *  adapter information that we want to display. Most of the information now
 *  comes from the TCP/IP stack itself. In order to keep the 'short' names that
 *  exist in the registry to refer to the individual adapters, we read the names
 *  from the registry then match them to the adapters returned by TCP/IP by
 *  matching the IPInterfaceContext value with the adapter which owns the IP
 *  address with that context value
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to linked list of ADAPTER_INFO structures
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID
CIpConfig::GetAdapterInfo()
{
    LPSTR* boundAdapterNames = NULL;
    DWORD err = ERROR_SUCCESS;

    if (GlobalPlatformType == PLATFORM_TYPE_WINNT)
    {
        if ( ServicesKey == NULL )
        {
            err = REGOPENKEY(HKEY_LOCAL_MACHINE,
                             SERVICES_KEY_NAME,
                             &ServicesKey
                             );
        }

        if ( err == ERROR_SUCCESS && TcpipLinkageKey == NULL )
        {
            err = REGOPENKEY(ServicesKey,
                             "Tcpip\\Linkage",
                             //"Tcpip\\Parameters\\Interfaces",
                             &TcpipLinkageKey
                             );
        }

        if (err == ERROR_SUCCESS && (boundAdapterNames = GetBoundAdapterList(TcpipLinkageKey)))
        {
            int i;

            //
            // apply the short name to the right adapter info by comparing
            // the IPInterfaceContext value in the adapter\Parameters\Tcpip
            // section with the context values read from the stack for the
            // IP addresses
            //

            for (i = 0; boundAdapterNames[i]; ++i) {

                LPSTR name;
                DWORD context;
                HKEY key;
                BOOL found;

                name = boundAdapterNames[i];

                if (!OpenAdapterKey(KEY_TCP, name, &key)) {
                    DEBUG_PRINT(UTIL, ERROR, ("GetAdapterInfo cannot open %s\n",
                                 name ));

                    goto quit;
                }
                if (!ReadRegistryDword(key,
                                       "IPInterfaceContext",
                                       &context
                                       )) {
                    DEBUG_PRINT(UTIL, ERROR, ("GetAdapterInfo: IPInterfaceContext failed\n"));
                    goto quit;
                }
                REGCLOSEKEY(key);

                //
                // now search through the list of adapters, looking for the one
                // that has the IP address with the same context value as that
                // just read. When found, apply the short name to that adapter
                //

                for (CAdapterInterface * pEntry = (CAdapterInterface *)m_List.Flink;
                     pEntry != (CAdapterInterface *)&m_List.Flink;
                     pEntry =  (CAdapterInterface *)pEntry->m_List.Flink)
                {
                    if ( pEntry->IsContextInIPAddrList(context) )
                    {
                        pEntry->SetAdapterName(name);
                        GetDhcpServerFromDhcp(pEntry);
                        break;
                    }
                }
            }

        } else {
            DEBUG_PRINT(UTIL, ERROR, ("GetAdapterInfo failed\n"));
        }
    }
    else
    {
        //
        // Win95: search through the list of adapters, gather DHCP server names
        //  for each.
        //

        for (CAdapterInterface * pEntry = (CAdapterInterface *)m_List.Flink;
             pEntry != (CAdapterInterface *)&m_List.Flink;
             pEntry =  (CAdapterInterface *)pEntry->m_List.Flink)
        {
            GetDhcpServerFromDhcp(pEntry);
        }

    }

quit:

    if (boundAdapterNames != NULL )
    {
        FREE_MEMORY(boundAdapterNames);
    }

    return;
}



PRIVATE
DWORD
CIpConfig::LoadEntryPoints(
    VOID
    )

/*++

Routine Description:

    Loads NTDLL.DLL entry points if Windows NT else (if Windows 95) loads
    WsControl from WSOCK32.DLL

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CIpConfig::LoadEntryPoints",
                 NULL
                 ));

    DWORD error = ERROR_SUCCESS;

    if (m_Loaded == TRI_STATE_UNKNOWN) {
        error = LoadDllEntryPoints((GlobalPlatformType == PLATFORM_TYPE_WINNT)
                                   ? &NtDllInfo : &WsControlInfo, 0);
        m_Loaded = (error == ERROR_SUCCESS) ? TRI_STATE_TRUE : TRI_STATE_FALSE;
    }

    if (GlobalPlatformVersion5 && (_I_GetAdaptersInfo == NULL))
    {
        error = LoadDllEntryPoints(&IpHlpApiDllInfo, 0);
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
CIpConfig::UnloadEntryPoints(
    VOID
    )

/*++

Routine Description:

    Unloads NTDLL.DLL if platform is Windows NT

Arguments:

    None.

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CIpConfig::UnloadEntryPoints",
                 NULL
                 ));

    DWORD error = ERROR_SUCCESS;

    if (m_Loaded == TRI_STATE_TRUE) {
        error = UnloadDllEntryPoints((GlobalPlatformType == PLATFORM_TYPE_WINNT)
                                     ? &NtDllInfo : &WsControlInfo, FALSE);
        if (error == ERROR_SUCCESS) {
            m_Loaded = TRI_STATE_UNKNOWN;
        }
    }

    if (GlobalPlatformVersion5)
    {
        if (_I_GetAdaptersInfo != NULL) {
            error = UnloadDllEntryPoints(&IpHlpApiDllInfo, FALSE);
        }

        if ( _I_DhcpRequestOptions != NULL ) {
            error = UnloadDllEntryPoints(&DhcpcSvcDllInfo, FALSE);
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
CAdapterInterface *
CIpConfig::FindOrCreateInterface(
    IN DWORD dwIndex,
    IN DWORD dwType,
    IN DWORD dwSpeed,
    IN LPSTR lpszDescription,
    IN DWORD dwDescriptionLength,
    IN LPBYTE lpPhysicalAddress,
    IN DWORD dwPhysicalAddressLength
    )

/*++

Routine Description:

    Returns a pointer to the CAdapterInterface object corresponding to dwIndex.
    If none found in the list, a new entry is created

Arguments:

    dwIndex             - unique interface identifier to find or create

    dwType              - type of adapter

    dwSpeed             - adapter media speed

    lpszDescription     - name of this interface

    dwDescriptionLength - length of the name

Return Value:

    CAdapterInterface *
        Success - pointer to found or created object

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Pointer,
                 "CIpConfig::FindOrCreateInterface",
                 "%d, %s (%d), %d, %.*q, %d, %x, (%u)",
                 dwIndex,
                 InternetMapInterface(dwType),
                 dwType,
                 dwSpeed,
                 dwDescriptionLength,
                 lpszDescription,
                 dwDescriptionLength,
                 lpPhysicalAddress,
                 dwPhysicalAddressLength
                 ));

    CAdapterInterface * pInterface = FindInterface(dwIndex);

    if (pInterface == NULL) {
        pInterface = new CAdapterInterface(dwIndex,
                                           dwType,
                                           dwSpeed,
                                           lpszDescription,
                                           dwDescriptionLength,
                                           lpPhysicalAddress,
                                           dwPhysicalAddressLength
                                           );
        if (pInterface != NULL) {
            InsertHeadList(&m_List, &pInterface->m_List);
            ++m_dwNumberOfInterfaces;
        }
    }

    DEBUG_LEAVE(pInterface);

    return pInterface;
}


PRIVATE
CAdapterInterface *
CIpConfig::FindInterface(
    IN DWORD dwIndex
    )

/*++

Routine Description:

    Returns a pointer to the CAdapterInterface object corresponding to dwIndex

Arguments:

    dwIndex - unique interface identifier to find

Return Value:

    CAdapterInterface *
        Success - pointer to found object

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Pointer,
                 "CIpConfig::FindInterface",
                 "%d",
                 dwIndex
                 ));

    CAdapterInterface * pInterface = NULL;

    for (PLIST_ENTRY pEntry = m_List.Flink;
        pEntry != (PLIST_ENTRY)&m_List;
        pEntry = pEntry->Flink) {

        if (((CAdapterInterface *)pEntry)->m_dwIndex == dwIndex) {
            ((CAdapterInterface *)pEntry)->SetFound(TRUE);

            //
            // ASSUMES: pEntry == &CAdapterInterface
            //

            pInterface = (CAdapterInterface *)pEntry;
            break;
        }
    }

    DEBUG_LEAVE(pInterface);

    return pInterface;
}


PRIVATE
BOOL
CIpConfig::ThrowOutUnfoundEntries(
    VOID
    )

/*++

Routine Description:

    Throws out (deletes) any entries that are marked not-found

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - interfaces thrown out

        FALSE   -      "     not "   "

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "CIpConfig::ThrowOutUnfoundEntries",
                 NULL
                 ));

    //
    // ASSUMES: CAdapterInterface.m_List.Flink is first element in structure
    //

    PLIST_ENTRY pPrevious = (PLIST_ENTRY)&m_List.Flink;
    PLIST_ENTRY pEntry = m_List.Flink;
    BOOL bThrownOut = FALSE;

    while (pEntry != (PLIST_ENTRY)&m_List) {

        CAdapterInterface * pInterface = (CAdapterInterface *)pEntry;

        if (!pInterface->IsFound()) {

            DEBUG_PRINT(UTIL,
                        WARNING,
                        ("adapter index %d (%q) not located in list\n",
                        pInterface->m_dwIndex,
                        pInterface->m_lpszDescription
                        ));

            RemoveEntryList(&pInterface->m_List);
            --m_dwNumberOfInterfaces;

            INET_ASSERT((int)m_dwNumberOfInterfaces >= 0);

            delete pInterface;
            bThrownOut = TRUE;
        } else {

            //
            // throw out any IP addresses
            //

            bThrownOut |= pInterface->m_IpList.ThrowOutUnfoundEntries();
            //bThrownOut |= pInterface->m_RouterList.ThrowOutUnfoundEntries();
            //bThrownOut |= pInterface->m_DnsList.ThrowOutUnfoundEntries();
            pPrevious = pEntry;
        }
        pEntry = pPrevious->Flink;
    }

    DEBUG_LEAVE(bThrownOut);

    return bThrownOut;
}

//
// public functions
//


DWORD
WsControl(
    IN DWORD dwProtocol,
    IN DWORD dwRequest,
    IN LPVOID lpInputBuffer,
    IN OUT LPDWORD lpdwInputBufferLength,
    OUT LPVOID lpOutputBuffer,
    IN OUT LPDWORD lpdwOutputBufferLength
    )

/*++

Routine Description:

    Makes device-dependent driver call based on O/S

Arguments:

    dwProtocol              - ignored

    dwRequest               - ignored

    lpInputBuffer           - pointer to request buffer

    lpdwInputBufferLength   - pointer to DWORD: IN = request buffer length

    lpOutputBuffer          - pointer to output buffer

    lpdwOutputBufferLength  - pointer to DWORD: IN = length of output buffer;
                                               OUT = length of returned data

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "WsControl",
                 "%d, %d, %#x, %#x [%d], %#x, %#x [%d]",
                 dwProtocol,
                 dwRequest,
                 lpInputBuffer,
                 lpdwInputBufferLength,
                 *lpdwInputBufferLength,
                 lpOutputBuffer,
                 lpdwOutputBufferLength,
                 *lpdwOutputBufferLength
                 ));

    DWORD error;

    if (GlobalPlatformType == PLATFORM_TYPE_WINNT) {
        error = WinNtWsControl(dwProtocol,
                               dwRequest,
                               lpInputBuffer,
                               lpdwInputBufferLength,
                               lpOutputBuffer,
                               lpdwOutputBufferLength
                               );
    } else {
        error = _I_WsControl(dwProtocol,
                             dwRequest,
                             lpInputBuffer,
                             lpdwInputBufferLength,
                             lpOutputBuffer,
                             lpdwOutputBufferLength
                             );
    }

    DEBUG_LEAVE(error);

    return error;
}

//
// private functions
//


PRIVATE
DWORD
WinNtWsControl(
    DWORD dwProtocol,
    DWORD dwRequest,
    LPVOID lpInputBuffer,
    LPDWORD lpdwInputBufferLength,
    LPVOID lpOutputBuffer,
    LPDWORD lpdwOutputBufferLength
    )

/*++

Routine Description:

    Handles WsControl() functionality on NT platform. Assumes NTDLL.DLL has
    already been loaded

Arguments:

    dwProtocol              - unused

    dwRequest               - unused

    lpInputBuffer           - contains driver request structure

    lpdwInputBufferLength   - pointer to length of InputBuffer

    lpOutputBuffer          - pointer to buffer where results written

    lpdwOutputBufferLength  - pointer to length of OutputBuffer. Updated with
                              returned data length on successful return

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "WinNtWsControl",
                 "%d, %d, %#x, %#x [%d], %#x, %#x [%d]",
                 dwProtocol,
                 dwRequest,
                 lpInputBuffer,
                 lpdwInputBufferLength,
                 *lpdwInputBufferLength,
                 lpOutputBuffer,
                 lpdwOutputBufferLength,
                 *lpdwOutputBufferLength
                 ));

    UNREFERENCED_PARAMETER(dwProtocol);
    UNREFERENCED_PARAMETER(dwRequest);

    DWORD error;

    if (TcpipDriverHandle == INVALID_HANDLE_VALUE) {
        error = OpenTcpipDriverHandle();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    DWORD bytesReturned;
    BOOL ok;

    ok = DeviceIoControl(TcpipDriverHandle,
                         IOCTL_TCP_QUERY_INFORMATION_EX,
                         lpInputBuffer,
                         *lpdwInputBufferLength,
                         lpOutputBuffer,
                         *lpdwOutputBufferLength,
                         &bytesReturned,
                         NULL
                         );
    if (!ok) {
        error = GetLastError();
    } else {
        *lpdwOutputBufferLength = bytesReturned;
        error = ERROR_SUCCESS;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
OpenTcpipDriverHandle(
    VOID
    )

/*++

Routine Description:

    Opens handle to TCP/IP device driver

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DWORD error = ERROR_SUCCESS;

    if (TcpipDriverHandle == INVALID_HANDLE_VALUE) {

        OBJECT_ATTRIBUTES objectAttributes;
        IO_STATUS_BLOCK iosb;
        UNICODE_STRING string;
        NTSTATUS status;

        _I_RtlInitUnicodeString(&string, DD_TCP_DEVICE_NAME);

        InitializeObjectAttributes(&objectAttributes,
                                   &string,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL
                                   );
        status = _I_NtCreateFile(&TcpipDriverHandle,
                                 SYNCHRONIZE | GENERIC_EXECUTE,
                                 &objectAttributes,
                                 &iosb,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN_IF,
                                 FILE_SYNCHRONOUS_IO_NONALERT,
                                 NULL,
                                 0
                                 );
        if (!NT_SUCCESS(status)) {
            error = _I_RtlNtStatusToDosError(status);
        }
    }
    return error;
}


PRIVATE
VOID
CloseTcpipDriverHandle(
    VOID
    )

/*++

Routine Description:

    Closes TCP/IP device driver handle

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (TcpipDriverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(TcpipDriverHandle);
        TcpipDriverHandle = INVALID_HANDLE_VALUE;
    }
}


PRIVATE
DWORD
GetEntityList(
    OUT TDIEntityID * * lplpEntities
    )

/*++

Routine Description:

    Allocates a buffer for, and retrieves, the list of entities supported by the
    TCP/IP device driver

Arguments:

    lplpEntities    - pointer to allocated returned list of entities. Caller
                      must free

Return Value:

    UINT    - number of entities returned

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Int,
                 "GetEntityList",
                 "%#x",
                 lplpEntities
                 ));

    TCP_REQUEST_QUERY_INFORMATION_EX req;

    memset(&req, 0, sizeof(req));

    req.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    req.ID.toi_entity.tei_instance = 0;
    req.ID.toi_class = INFO_CLASS_GENERIC;
    req.ID.toi_type = INFO_TYPE_PROVIDER;
    req.ID.toi_id = ENTITY_LIST_ID;

    DWORD inputLen = sizeof(req);
    DWORD outputLen = sizeof(TDIEntityID) * DEFAULT_MINIMUM_ENTITIES;
    TDIEntityID * pEntity = NULL;
    DWORD status = TDI_SUCCESS;

    //
    // this is over-engineered - its very unlikely that we'll ever get >32
    // entities returned, never mind >64K's worth
    //
    // Go round this loop a maximum of 4 times - length of list shouldn't
    // change between calls. Stops us getting stuck in infinite loop if
    // something bad happens with outputLen
    //

    for (int i = 0; i < 4; ++i) {

        DWORD previousOutputLen = outputLen;

        pEntity = (TDIEntityID *)ResizeBuffer(pEntity, outputLen, FALSE);
        if (pEntity == NULL) {
            outputLen = 0;
            break;
        }

        status = WsControl(IPPROTO_TCP,
                           WSCNTL_TCPIP_QUERY_INFO,
                           (LPVOID)&req,
                           &inputLen,
                           (LPVOID)pEntity,
                           &outputLen
                           );

        //
        // TDI_SUCCESS is returned if all data is not returned: driver
        // communicates all/partial data via outputLen
        //

        if (status == TDI_SUCCESS) {

            DEBUG_PRINT(UTIL,
                        INFO,
                        ("GENERIC_ENTITY required length = %d\n",
                        outputLen
                        ));

            if (outputLen && (outputLen <= previousOutputLen)) {
                break;
            }
        } else {
            outputLen = 0;
        }
    }

    if ((status != TDI_SUCCESS) && (pEntity != NULL)) {
        ResizeBuffer(pEntity, 0, FALSE);
    }

    DEBUG_PRINT(UTIL,
                INFO,
                ("%d entities returned in %#x\n",
                (outputLen / sizeof(TDIEntityID)),
                pEntity
                ));

    *lplpEntities = pEntity;

    DEBUG_LEAVE((UINT)(outputLen / sizeof(TDIEntityID)));

    return (UINT)(outputLen / sizeof(TDIEntityID));
}

//
// private debug functions
//

#if INET_DEBUG

PRIVATE
LPSTR
InternetMapEntity(
    IN INT EntityId
    ) {
    switch (EntityId) {
    case CO_TL_ENTITY:
        return "CO_TL_ENTITY";

    case CL_TL_ENTITY:
        return "CL_TL_ENTITY";

    case ER_ENTITY:
        return "ER_ENTITY";

    case CO_NL_ENTITY:
        return "CO_NL_ENTITY";

    case CL_NL_ENTITY:
        return "CL_NL_ENTITY";

    case AT_ENTITY:
        return "AT_ENTITY";

    case IF_ENTITY:
        return "IF_ENTITY";

    }
    return "*** UNKNOWN ENTITY ***";
}

PRIVATE
LPSTR
InternetMapInterface(
    IN DWORD InterfaceType
    ) {
    switch (InterfaceType) {
    case IF_TYPE_OTHER:
        return "other";

    case IF_TYPE_ETHERNET:
        return "ethernet";

    case IF_TYPE_TOKENRING:
        return "token ring";

    case IF_TYPE_FDDI:
        return "FDDI";

    case IF_TYPE_PPP:
        return "PPP";

    case IF_TYPE_LOOPBACK:
        return "loopback";

    case IF_TYPE_SLIP:
        return "SLIP";
    }
    return "???";
}

PRIVATE
LPSTR
InternetMapInterfaceOnNT5(
    IN DWORD InterfaceType
    ) {
    switch (InterfaceType) {
    case IF_OTHER_ADAPTERTYPE:
        return "other";

    case IF_ETHERNET_ADAPTERTYPE:
        return "ethernet";

    case IF_TOKEN_RING_ADAPTERTYPE:
        return "token ring";

    case IF_FDDI_ADAPTERTYPE:
        return "FDDI";

    case IF_PPP_ADAPTERTYPE:
        return "PPP";

    case IF_LOOPBACK_ADAPTERTYPE:
        return "loopback";

    case IF_SLIP_ADAPTERTYPE:
        return "SLIP";
    }
    return "???";
}


#endif



