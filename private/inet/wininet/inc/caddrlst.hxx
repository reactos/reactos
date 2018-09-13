/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    caddrlst.hxx

Abstract:

    Contains CAddressList class definition

    Contents:

Author:

    Richard L Firth (rfirth) 21-Apr-1997

Revision History:

    21-Apr-1997 rfirth
        Created

--*/

//
// manifests
//

#define CSADDR_BUFFER_LENGTH    (sizeof(CSADDR_INFO) + 128)

//
// types
//

//
// RESOLVED_ADDRESS - a CSADDR_INFO with additional IsBad field which is set
// when we fail to connect to the address
//

typedef struct {
    CSADDR_INFO AddrInfo;
    BOOL IsValid;
} RESOLVED_ADDRESS, * LPRESOLVED_ADDRESS;

//
// forward references
//

class CFsm_ResolveHost;

//
// classes
//

//
// CAddressList - maintains list of resolved addresses for a host name/port
// combination
//

class CAddressList {

private:

    CRITICAL_SECTION m_CritSec;     // grab this before updating
    DWORD m_ResolutionId;           // determines when OK to resolve
    INT m_CurrentAddress;           // index of current (good) address
    INT m_AddressCount;             // number of addresses in list
    INT m_BadAddressCount;          // number addresses already tried & failed
    LPRESOLVED_ADDRESS m_Addresses; // list of resolved addresses

    DWORD
    IPAddressToAddressList(
        IN DWORD ipAddr
        );

    DWORD
    HostentToAddressList(
        IN LPHOSTENT lpHostent
        );

public:

    CAddressList() {
        InitializeCriticalSection(&m_CritSec);
        m_ResolutionId = 0;
        m_CurrentAddress = 0;
        m_AddressCount = 0;
        m_BadAddressCount = 0;
        m_Addresses = NULL;
    }

    ~CAddressList() {
        FreeList();
        DeleteCriticalSection(&m_CritSec);
    }

    VOID Acquire(VOID) {
        EnterCriticalSection(&m_CritSec);
    }

    VOID Release(VOID) {
        LeaveCriticalSection(&m_CritSec);
    }

    VOID
    FreeList(
        VOID
        );

    DWORD
    SetList(
        IN DWORD dwIpAddress
        );

    DWORD
    SetList(
        IN LPHOSTENT lpHostent
        );

    BOOL
    GetNextAddress(
        IN OUT LPDWORD lpdwResolutionId,
        IN OUT LPDWORD lpdwIndex,
        IN INTERNET_PORT nPort,
        OUT LPCSADDR_INFO lpResolvedAddress
        );

    VOID
    InvalidateAddress(
        IN DWORD dwResolutionId,
        IN DWORD dwAddressIndex
        );

    DWORD
    ResolveHost(
        IN LPSTR lpszHostName,
        IN OUT LPDWORD lpdwResolutionId,
        IN DWORD dwFlags
        );

    DWORD
    ResolveHost_Fsm(
        IN CFsm_ResolveHost * Fsm
        );

    VOID NextAddress(VOID) {
        ++m_CurrentAddress;
        if (m_CurrentAddress == m_AddressCount) {
            m_CurrentAddress = 0;
        }
    }

    LPCSADDR_INFO CurrentAddress(VOID) {
        return &m_Addresses[m_CurrentAddress].AddrInfo;
    }

    LPSOCKADDR LocalSockaddr() {
        return CurrentAddress()->LocalAddr.lpSockaddr;
    }

    INT LocalSockaddrLength() {
        return CurrentAddress()->LocalAddr.iSockaddrLength;
    }

    INT LocalFamily(VOID) {
        return (INT)LocalSockaddr()->sa_family;
    }

    INT LocalPort() {
        return ((LPSOCKADDR_IN)LocalSockaddr())->sin_port;
    }

    VOID SetLocalPort(INTERNET_PORT Port) {
        ((LPSOCKADDR_IN)LocalSockaddr())->sin_port = Port;
    }

    LPSOCKADDR RemoteSockaddr() {
        return CurrentAddress()->RemoteAddr.lpSockaddr;
    }

    INT RemoteSockaddrLength() {
        return CurrentAddress()->RemoteAddr.iSockaddrLength;
    }

    INT RemoteFamily(VOID) {
        return (INT)RemoteSockaddr()->sa_family;
    }

    INT RemotePort() {
        return ((LPSOCKADDR_IN)RemoteSockaddr())->sin_port;
    }

    INT SocketType(VOID) {
        return CurrentAddress()->iSocketType;
    }

    INT Protocol(VOID) {
        return CurrentAddress()->iProtocol;
    }

    BOOL IsCurrentAddressValid(VOID) {
        return m_Addresses[m_CurrentAddress].IsValid;
    }
};
 

