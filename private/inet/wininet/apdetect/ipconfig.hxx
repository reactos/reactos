/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ipconfig.hxx

Abstract:

    CIpconfig class definition

Author:

    Richard L Firth (rfirth) 29-Oct-1996

Environment:

    Win32 user-mode DLL

Revision History:

    29-Oct-1996 rfirth
        Created

    15-Jul-1998 arthurbi
        Resurrected from the dead

--*/

#ifndef IPCONFIG_H_
#define IPCONFIG_H_

//
// manifests
//

#define HOST_INADDR_ANY         0x00000000
#define HOST_INADDR_NONE        0xffffffff
#define HOST_INADDR_LOOPBACK    0x0100007f

#define KEY_TCP             1
#define KEY_NBT             2

//#define MAX_ADAPTER_NAME_LENGTH         128 // arb.
#define MAX_ADAPTER_ADDRESS_LENGTH      8   // arb.

//
// macros
//

#define IS_VALID_NON_LOOPBACK_IP_ADDRESS(address) \
    (((address) != HOST_INADDR_ANY) \
    && ((address) != HOST_INADDR_NONE) \
    && ((address) != HOST_INADDR_LOOPBACK))

//
// class definitions
//

//
// CIpAddress - IP address and associated subnet mask
//

class CIpAddress {

friend class CAdapterInfo;
friend class CIpAddressList;

private:

    CIpAddress * m_Next;
    BOOL m_bFound;
    DWORD m_dwIpAddress;
    DWORD m_dwIpMask;
    DWORD m_dwContext;

public:

    CIpAddress(
        IN DWORD dwIpAddress = INADDR_ANY,
        IN DWORD dwIpMask = INADDR_ANY,
        IN DWORD dwContext = 0
        ) {
        m_Next = NULL;
        m_bFound = TRUE;
        m_dwIpAddress = dwIpAddress;
        m_dwIpMask = dwIpMask;
        m_dwContext = dwContext;
    }

    ~CIpAddress() {
        /* NOTHING */
    }

    VOID SetFound(BOOL bFound) {
        m_bFound = bFound;
    }

    BOOL
    GetAddress(
        OUT LPBYTE lpbAddress,
        IN OUT LPDWORD lpdwAddressLength
        );

    BOOL IsFound(VOID) const {
        return m_bFound;
    }

    DWORD IpAddress(VOID) const {
        return m_dwIpAddress;
    }

    DWORD IpMask(VOID) const {
        return m_dwIpMask;
    }

    DWORD Context(VOID) const {
        return m_dwContext;
    }
};

//
// CIpAddressList - singly-linked list of IP addresses
//

class CIpAddressList {
                 
    friend class CAdapterInterface;

private:

    CIpAddress * m_List;

public:

    CIpAddressList(VOID) {
        m_List = NULL;
    }

    ~CIpAddressList() {
        Clear();
    }

    CIpAddress *
    Find(
        IN DWORD dwIpAddress,
        IN DWORD dwIpMask = INADDR_ANY
        );

    BOOL 
    IsContextInList(
        IN DWORD dwContext
        );

    VOID
    Add(
        CIpAddress * pAddress
        );

    BOOL
    Add(
        IN DWORD dwIpAddress = INADDR_ANY,
        IN DWORD dwIpMask = INADDR_ANY,
        IN DWORD dwContext = 0
        );

    BOOL
    GetAddress(
        IN OUT LPDWORD lpdwIndex,
        OUT LPBYTE lpbAddress,
        IN OUT LPDWORD lpdwAddressLength
        );

    VOID SetFound(BOOL bFound) {
        for (CIpAddress * pEntry = m_List;
             pEntry != NULL;
             pEntry = pEntry->m_Next) {

            pEntry->SetFound(bFound);
        }
    }

    VOID Clear(VOID) {

        CIpAddress * pEntry;

        while ((pEntry = m_List) != NULL) {
            m_List = pEntry->m_Next;
            delete pEntry;
        }
    }

    BOOL IsEmpty(VOID) {
        return (m_List == NULL) ? TRUE : FALSE;
    }

    BOOL
    ThrowOutUnfoundEntries(
        VOID
        );
};

//
// CAdapterInterface - singly-linked list of interface descriptors
//

class CAdapterInterface {

friend class CIpConfig;

private:

    LIST_ENTRY m_List;    
    LPSTR m_lpszDescription;    
    DWORD m_dwDescriptionLength;
    BYTE m_dwPhysicalAddressType;
    LPBYTE m_lpPhysicalAddress;
    DWORD m_dwPhysicalAddressLength;
    LPSTR m_lpszAdapterName;
    DWORD m_dwIndex;
    DWORD m_dwType;
    DWORD m_dwSpeed;
    union {
        struct {
            unsigned Found  : 1;
            unsigned DialUp : 1;
            unsigned Dhcp   : 1;
        } Bits;
        DWORD Word;
    } m_Flags;
    CIpAddressList m_IpList;
    //CIpAddressList m_RouterList; // do we still care about this one? no, we don't need it
    //CIpAddressList m_DnsList; // was never used to begin with
    CIpAddressList m_DhcpList;

public:

    CAdapterInterface(
        IN DWORD dwIndex = 0,
        IN DWORD dwType = 0,
        IN DWORD dwSpeed = (DWORD)-1,
        IN LPSTR lpszDescription = NULL,
        IN DWORD dwDescriptionLength = 0,
        IN LPBYTE lpPhysicalAddress = NULL,
        IN DWORD dwPhysicalAddressLength = 0
        );

    ~CAdapterInterface();

    BOOL IsFound(VOID) {
        return m_Flags.Bits.Found ? TRUE : FALSE;
    }

    VOID SetFound(BOOL bFound) {
        m_Flags.Bits.Found = bFound ? 1 : 0;
    }

    BOOL IsDialUp(VOID) {
        return m_Flags.Bits.DialUp ? TRUE : FALSE;
    }

    VOID SetDialUp(VOID) {
        m_Flags.Bits.DialUp = 1;
    }

    BOOL IsDhcp(VOID) {
        return m_Flags.Bits.Dhcp ? TRUE : FALSE;
    }

    VOID SetDhcp(VOID) {
        m_Flags.Bits.Dhcp = 1;
    }

    VOID SetNotFound(VOID) {
        SetFound(FALSE);
        m_IpList.SetFound(FALSE);
        //m_RouterList.SetFound(FALSE);
        //m_DnsList.SetFound(FALSE);
    }

    BOOL FindIpAddress(DWORD dwIpAddress) {
        return (m_IpList.Find(dwIpAddress) != NULL) ? TRUE : FALSE;
    }

    BOOL IsHardwareAddress(LPBYTE lpHardwareAddress) {
        return ( m_lpPhysicalAddress ? 
                    !memcmp(m_lpPhysicalAddress,
                            lpHardwareAddress,
                            m_dwPhysicalAddressLength
                            ) 
                    : FALSE );
    }

    BOOL IsContextInIPAddrList(IN DWORD dwContext) {        
        return (m_IpList.IsContextInList(dwContext));
    }

    BOOL AddDhcpServer(DWORD dwIpAddress) {
        return m_DhcpList.Add(dwIpAddress);
    }

    VOID ClearDhcpServerList(VOID) {
        m_DhcpList.Clear();
    }

    BOOL SetAdapterName(LPSTR lpszNewAdapterName) {
        m_lpszAdapterName = NewString(lpszNewAdapterName);
        return (m_lpszAdapterName ? TRUE : FALSE);
    }

    LPSTR GetAdapterName(VOID) {
        return m_lpszAdapterName;
    }

    BOOL CopyAdapterInfoToDhcpContext(PDHCP_CONTEXT pDhcpContext);

    BOOL
    DhcpDoInformNT5(
        IN OUT LPSTR lpszAutoProxyUrl,
        IN DWORD dwAutoProxyUrlLength
        );
};

//
// CIpConfig - maintains all info about IP interfaces on this machine
//

class CIpConfig {

private:

    LIST_ENTRY m_List;
    DWORD m_dwNumberOfInterfaces;
    TRI_STATE m_Loaded;
    CIpAddressList m_DnsList;

    DWORD
    LoadEntryPoints(
        VOID
        );

    DWORD
    UnloadEntryPoints(
        VOID
        );

    BOOL EntryPointsLoaded(VOID) const {
        return m_Loaded;
    }

    BOOL
    GetAdapterList(
        OUT LPBOOL lpbChanged = NULL
        );

    BOOL
    GetAdapterListOnNT5(
        OUT LPBOOL lpbChanged = NULL
        );

    CAdapterInterface *
    FindOrCreateInterface(
        IN DWORD dwIndex,
        IN DWORD dwType,
        IN DWORD dwSpeed,
        IN LPSTR lpszDescription,
        IN DWORD dwDescriptionLength,
        IN LPBYTE lpPhysicalAddress,
        IN DWORD dwPhysicalAddressLength
        );

    CAdapterInterface *
    FindInterface(
        IN DWORD dwIndex
        );

    VOID SetNotFound(VOID) {

        //
        // ASSUMES: address of list entry in CAdapterInterface is same as
        //          address of CAdapterInterface
        //

        for (CAdapterInterface * pEntry = (CAdapterInterface *)m_List.Flink;
             pEntry != (CAdapterInterface *)&m_List.Flink;
             pEntry = (CAdapterInterface *)pEntry->m_List.Flink) {

            pEntry->SetNotFound();
        }
        m_DnsList.SetFound(FALSE);
    }

    BOOL
    ThrowOutUnfoundEntries(
        VOID
        );

    BOOL
    ThrowOutUnfoundAddresses(
        VOID
        );

public:

    CIpConfig(VOID);

    ~CIpConfig();

    BOOL
    GetRouterAddress(
        IN LPBYTE lpbInterfaceAddress OPTIONAL,
        IN DWORD dwInterfaceAddressLength,
        IN OUT LPDWORD lpdwIndex,
        OUT LPBYTE lpbAddress,
        IN OUT LPDWORD lpdwAddressLength
        );

    BOOL
    GetDnsAddress(
        IN LPBYTE lpbInterfaceAddress OPTIONAL,
        IN DWORD dwInterfaceAddressLength,
        IN OUT LPDWORD lpdwIndex,
        OUT LPBYTE lpbAddress,
        IN OUT LPDWORD lpdwAddressLength
        );

    BOOL
    IsKnownIpAddress(
        IN LPBYTE lpbInterfaceAddress OPTIONAL,
        IN DWORD dwInterfaceAddressLength,
        IN LPBYTE lpbAddress,
        IN DWORD dwAddressLength
        );

    BOOL
    Refresh(
        VOID
        );

    VOID
    GetAdapterInfo(
        VOID
        );

    BOOL
    DoInformsOnEachInterface(
        IN OUT LPSTR lpszAutoProxyUrl,
        IN     DWORD dwAutoProxyUrlLength
        );
};

//
// Function declarations
//

LPSTR* GetBoundAdapterList(HKEY BindingsSectionKey);
BOOL OpenAdapterKey(DWORD KeyType, LPSTR Name, PHKEY Key);
BOOL ReadRegistryDword(HKEY Key, LPSTR ParameterName, LPDWORD Value);
BOOL ReadRegistryString(HKEY Key, LPSTR ParameterName, LPSTR String, LPDWORD Length);
UINT GetDhcpServerFromDhcp(IN OUT CAdapterInterface * paiInterface);    

BOOL DhcpDoInform(                                     // send an inform packet if necessary
    IN      CAdapterInterface *    pAdapterInterface,
    IN      BOOL                   fBroadcast,    // Do we broadcast this inform, or unicast to server?
    OUT     LPSTR                  lpszAutoProxyUrl,
    IN      DWORD                  dwAutoProxyUrlLength
    );

DWORD 
QueryWellKnownDnsName(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN     DWORD dwAutoProxyUrlLength
    );


//
// global data
//

//extern CIpConfig * GlobalIpConfig;
extern const char SERVICES_KEY_NAME[];

#endif // IPCONFIG_H_
