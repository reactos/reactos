//-----------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved.
//
// rprocess.cxx
//
//-----------------------------------------------------------------------

#define  UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <winsock2.h>
#include <svcguid.h>
#include <mbstring.h>
#include <ws2spi.h>
#include <ws2help.h>
#include <ws2_if.h>
#include <ws2_rp.h>
#include "dict.h"
#include "rprocess.h"

static GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
static GUID AddressGuid  = SVCID_INET_HOSTADDRBYINETSTRING;

//-----------------------------------------------------------------------
// CRestrictedProcess::CRestrictedProcess()
//
// Parameters:
//   pUnkOuter       LPUNKNOWN of a controlling unknown.
//   pfnDestroy      PFNDESTROYED to call when an object is destroyed.
//-----------------------------------------------------------------------
CRestrictedProcess::CRestrictedProcess( LPUNKNOWN    pUnkOuter,
                                        PFNDESTROYED pfnDestroy )
    {
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;
    m_pImpIRestrictedProcess = 0;
    return;
    }

//-----------------------------------------------------------------------
// CRestrictedProcess::~CRestrictedProcess(void)
//-----------------------------------------------------------------------
CRestrictedProcess::~CRestrictedProcess(void)
    {
    if (m_pImpIRestrictedProcess) delete m_pImpIRestrictedProcess;
    return;
    }

//-----------------------------------------------------------------------
// CRestrictedProcess::Init
//
//  Performs all intiailization of a CRestrictedProcess that's prone to failure.
//
//  Return Value:
//    BOOL    TRUE if the function is successful,
//            FALSE otherwise (out of memory).
//-----------------------------------------------------------------------
BOOL CRestrictedProcess::Init(void)
    {
    int     iRet;
    WORD    wVersion;
    WSADATA wsaData;

    IUnknown *pUnkOuter = m_pUnkOuter;

    if (!pUnkOuter)
       {
       pUnkOuter = this;
       }

    m_pImpIRestrictedProcess = new CImpIRestrictedProcess(this,pUnkOuter);
    if (!m_pImpIRestrictedProcess)
       {
       return FALSE;
       }

    wVersion = MAKEWORD(1,1);
    iRet = WSAStartup( wVersion, &wsaData );

    return TRUE;
    }

//----------------------------------------------------------------
// CRestrictedProcess::QueryInterface()
//
//----------------------------------------------------------------
STDMETHODIMP CRestrictedProcess::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    //
    // The only calls for IUnknown are either in a nonaggregated
    // case or when created in an aggregation, so in either case
    // always return our IUnknown for IID_IUnknown.
    //
    if (IID_IUnknown==riid)
        *ppv=this;

    if (IID_IRestrictedProcess == riid)
        *ppv = m_pImpIRestrictedProcess;

    if (*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


//----------------------------------------------------------------
// CRestrictedProcess::AddRef()
//----------------------------------------------------------------
STDMETHODIMP_(ULONG) CRestrictedProcess::AddRef(void)
    {
    return ++m_cRef;
    }


//----------------------------------------------------------------
// CRestrictedProcess::Release()
//----------------------------------------------------------------
STDMETHODIMP_(ULONG) CRestrictedProcess::Release(void)
    {
    if (0 != --m_cRef)
        return m_cRef;

    //
    // Tell the housing that an object is going away so it can
    // shut down if appropriate.
    //
    if (NULL!=m_pfnDestroy)
        (*m_pfnDestroy)();

    delete this;
    return 0;
    }

//----------------------------------------------------------------
//----------------------------------------------------------------


//----------------------------------------------------------------
//  CImpIRestrictedProcess::CImpIRestrictedProcess()
//----------------------------------------------------------------
CImpIRestrictedProcess::CImpIRestrictedProcess(
                                PCRestrictedProcess pObj,
                                LPUNKNOWN           pUnkOuter )
{
    m_cRef = 0;
    m_pObj = pObj;
    m_pUnkOuter = pUnkOuter;
    m_hHelper = 0;
    m_Socket = INVALID_SOCKET;
    m_pSocketDict = 0;
    m_pHostEnt = 0;
    return;
}

//----------------------------------------------------------------
//  CImpIRestrictedProcess::~CImpIRestrictedProcess()
//----------------------------------------------------------------
CImpIRestrictedProcess::~CImpIRestrictedProcess( void )
{
    if (m_pSocketDict)
       {
       delete m_pSocketDict;
       }

    if (m_pHostEnt)
       {
       delete m_pHostEnt;
       }

    return;
}

//----------------------------------------------------------------
//  CImpIRestrictedProcess::QueryInterface()
//----------------------------------------------------------------
STDMETHODIMP CImpIRestrictedProcess::QueryInterface( REFIID  riid,
                                                     LPVOID *ppv )
    {
    return m_pUnkOuter->QueryInterface(riid, ppv);
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::AddRef()
//----------------------------------------------------------------
STDMETHODIMP_(ULONG) CImpIRestrictedProcess::AddRef(void)
    {
    ++m_cRef;
    return m_pUnkOuter->AddRef();
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::Release()
//----------------------------------------------------------------
STDMETHODIMP_(ULONG) CImpIRestrictedProcess::Release(void)
    {
    --m_cRef;
    return m_pUnkOuter->Release();
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_HelperInit()
//
//----------------------------------------------------------------
STDMETHODIMP CImpIRestrictedProcess::RP_HelperInit()
    {
    int       iRet;
    WORD      wVersion;
    WSADATA   wsaData;
    NTSTATUS  NtStatus;

    wVersion = MAKEWORD(1,1);
    iRet = WSAStartup( wVersion, &wsaData );

    m_pSocketDict = new CSocketsDictionary;
    if (!m_pSocketDict)
       {
       return E_OUTOFMEMORY;
       }

    NtStatus = RtlInitializeCriticalSection(&m_cs);
    if (!NT_SUCCESS(NtStatus))
       {
       delete m_pSocketDict;
       return E_UNEXPECTED;
       }

    return NOERROR;
    }


//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_WahOpenHandleHelper()
//
//  In order to use WPUCreateSocketHandle(), this function must be
//  remoted because it creates a file handle...
//
//  BUGBUG: Make hSourceProcess and hTargetProcess part of the
//          object...
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_WahOpenHandleHelper( IN  DWORD  dwTargetPid,
                                                OUT DWORD *pdwHelperHandle,
                                                OUT DWORD *pdwStatus )
    {
    BOOL      fInherit;
    DWORD     dwSourcePid;
    DWORD     dwAccess;
    DWORD     dwOptions;
    HANDLE    hSourceProcess;
    HANDLE    hSourceHandle;
    HANDLE    hTargetProcess;


    *pdwHelperHandle = 0;

    *pdwStatus = WahOpenHandleHelper(&m_hHelper);
    if (*pdwStatus)
        {
        return NOERROR;
        }

    // Get a handle to our own process (to be used by DuplicateHandle()).
    dwSourcePid = GetCurrentProcessId();
    hSourceProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwSourcePid );
    if (!hSourceProcess)
       {
       *pdwStatus = GetLastError();
       WahCloseHandleHelper(m_hHelper);
       m_hHelper = 0;
       return NOERROR;
       }

    // Get a handle to the restricted process
    hTargetProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwTargetPid );
    if (!hTargetProcess)
       {
       *pdwStatus = GetLastError();
       WahCloseHandleHelper(m_hHelper);
       m_hHelper = 0;
       CloseHandle(hSourceProcess);
       return NOERROR;
       }

    // Ok, duplicate the helper handle into the restricted client.
    dwAccess = 0;
    fInherit = FALSE;
    dwOptions = DUPLICATE_SAME_ACCESS;
    if (!DuplicateHandle(hSourceProcess,
                         (HANDLE)m_hHelper,
                         hTargetProcess,
                         (HANDLE*)pdwHelperHandle,
                         dwAccess,
                         fInherit,
                         dwOptions ))
       {
       *pdwStatus = GetLastError();
       WahCloseHandleHelper(m_hHelper);
       m_hHelper = 0;
       }
    else
       {
       m_hHelper = (HANDLE)*pdwHelperHandle;
       }

    // Done with the process handles.
    CloseHandle(hSourceProcess);
    CloseHandle(hTargetProcess);

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_WahCreateSocketHandle()
//
//  In order to use WPUCreateSocketHandle(), this function must be
//  remoted because it creates a file handle...
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_WahCreateSocketHandle( IN  DWORD  dwTargetPid,
                                                  IN  DWORD  dwHelperHandle,
                                                  OUT DWORD *pdwSocket,
                                                  OUT DWORD *pdwStatus )
    {
    BOOL      fInherit;
    DWORD     dwSourcePid;
    DWORD     dwAccess;
    DWORD     dwOptions;
    HANDLE    hSourceProcess;
    HANDLE    hSourceHandle;
    HANDLE    hTargetProcess;
    SOCKET    Socket;

    *pdwStatus = WahCreateSocketHandle( (HANDLE)m_hHelper,
                                        (SOCKET*)&Socket );
    if (*pdwStatus == NO_ERROR)
        {
        return NOERROR;
        }

    // Get a handle to our own process (to be used by DuplicateHandle()).
    dwSourcePid = GetCurrentProcessId();
    hSourceProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwSourcePid );
    if (!hSourceProcess)
       {
       *pdwStatus = GetLastError();
       WahCloseSocketHandle(m_hHelper,Socket);
       return NOERROR;
       }

    // Get a handle to the restricted process
    hTargetProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwTargetPid );
    if (!hTargetProcess)
       {
       *pdwStatus = GetLastError();
       WahCloseSocketHandle(m_hHelper,Socket);
       CloseHandle(hSourceProcess);
       return NOERROR;
       }

    // Ok, duplicate the helper handle into the restricted client.
    dwAccess = 0;
    fInherit = FALSE;
    dwOptions = DUPLICATE_SAME_ACCESS;
    if (!DuplicateHandle(hSourceProcess,
                         (HANDLE)Socket,
                         hTargetProcess,
                         (HANDLE*)pdwSocket,
                         dwAccess,
                         fInherit,
                         dwOptions ))
       {
       *pdwStatus = GetLastError();
       }

    // Close local copies of the helper handle and the socket,
    // both of these are now in the child process.
    WahCloseSocketHandle(m_hHelper,Socket);
    WahCloseHandleHelper(m_hHelper);
    m_hHelper = 0;

    // Done with the process handles.
    CloseHandle(hSourceProcess);
    CloseHandle(hTargetProcess);

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_WSPSocket()
//
//  Note that the newly created socket isn't retruned to the
//  restricted process until the completion of the connect().
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_WSPSocket(
                                 IN long   af,
                                 IN long   type,
                                 IN long   protocol,
                                 IN RPC_WSAPROTOCOL_INFOW *pProtocolInfo,
                                 IN RPC_GROUP g,
                                 IN DWORD     dwFlags,
                                 IN OUT long *pError )
    {
    SOCKET  SourceSocket;
    DWORD   dwKey;
    HRESULT hr;
    NTSTATUS NtStatus;


    *pError = 0;

    if ( (af != AF_INET) || (type != SOCK_STREAM))
       {
       *pError = WSAEACCES;
       return NOERROR;
       }

    if ((protocol != IPPROTO_TCP)
        && (protocol != IPPROTO_IP))
       {
       *pError = WSAEACCES;
       return NOERROR;
       }

    m_Socket = WSASocketW( af,
                           type,
                           protocol,
                           (WSAPROTOCOL_INFOW*)pProtocolInfo,
                           (GROUP)g,
                           dwFlags );
    if (m_Socket == INVALID_SOCKET)
       {
       *pError = WSAGetLastError();
       return NOERROR;
       }

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_WSPConnect()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_WSPConnect(
                                 IN  RPC_SOCKADDR_IN *pInAddr,
                                 IN  long        lAddrLen,
                                 IN  RPC_WSABUF *pCallerData,
                                 OUT RPC_WSABUF *pCalleeData,
                                 IN  RPC_QOS    *pSQOS,
                                 IN  RPC_QOS    *pGQOS,
                                 IN  DWORD       dwTargetPid,
                                 OUT DWORD      *pTargetSocket,
                                 OUT long       *pError )
    {
    int       iRet;
    HRESULT   hr;
    BOOL      fInherit;
    DWORD     dwSourcePid;
    DWORD     dwAccess;
    DWORD     dwOptions;
    NTSTATUS  NtStatus;
    HANDLE    hSourceProcess;
    HANDLE    hSourceHandle;
    HANDLE    hTargetProcess;

    *pError = NO_ERROR;

    dwSourcePid = GetCurrentProcessId();
    if (!dwSourcePid)
       {
       *pError = GetLastError();
       return NOERROR;
       }


    if (m_Socket == INVALID_SOCKET)
       {
       *pError = WSAEACCES;
       closesocket(m_Socket);
       m_Socket = INVALID_SOCKET;
       return NOERROR;
       }

    //
    // Check to make sure the restricted client is connecting only
    // to a server that we allow.
    //
    hr = IsValidAddr( (char*)&(pInAddr->sin_addr), sizeof(ULONG) );
    #ifdef DBG_ALWAYS_RESTRICTED
    DbgPrint("RP_WSPConnect(): IsValidAdder(): %s\n",
             (hr == S_OK)? "TRUE" : "FALSE" );
    hr = S_OK;
    #endif
    if (S_OK != hr)
       {
       *pError = WSAEACCES;
       closesocket(m_Socket);
       m_Socket = INVALID_SOCKET;
       return NOERROR;
       }

    // Ok, do the connect() on behalf of the restricted client
    iRet = WSAConnect( m_Socket,
                       (struct sockaddr *)pInAddr,
                       lAddrLen,
                       (WSABUF*)pCallerData,
                       (WSABUF*)pCalleeData,
                       (QOS*)pSQOS,
                       (QOS*)pGQOS );

    if (iRet == SOCKET_ERROR)
       {
       *pError = WSAGetLastError();
       closesocket(m_Socket);
       m_Socket = INVALID_SOCKET;
       return NOERROR;
       }

    // Get a handle to our own process (to be used by DuplicateHandle()).
    hSourceProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwSourcePid );
    if (!hSourceProcess)
       {
       *pError = GetLastError();
       closesocket(m_Socket);
       m_Socket = INVALID_SOCKET;
       return NOERROR;
       }

    // Get a handle to the restricted process
    hTargetProcess = OpenProcess( PROCESS_DUP_HANDLE, TRUE, dwTargetPid );
    if (!hTargetProcess)
       {
       *pError = GetLastError();
       closesocket(m_Socket);
       m_Socket = INVALID_SOCKET;
       CloseHandle(hSourceProcess);
       return NOERROR;
       }

    // Ok, duplicate the socket into the restricted client.
    dwAccess = 0;
    fInherit = FALSE;
    dwOptions = DUPLICATE_SAME_ACCESS;
    if (!DuplicateHandle(hSourceProcess,
                         (HANDLE)m_Socket,
                         hTargetProcess,
                         (HANDLE*)pTargetSocket,
                         dwAccess,
                         fInherit,
                         dwOptions ))
       {
       *pError = GetLastError();
       }

    // No longer need a local copy of the open socket.
    closesocket(m_Socket);
    m_Socket = INVALID_SOCKET;

    // Done with the process handles.
    CloseHandle(hSourceProcess);
    CloseHandle(hTargetProcess);

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_SetSockOpt()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_SetSockOpt( IN int   level,
                                       IN int   optname,
                                       IN unsigned char *optval,
                                       IN int   optlen,
                                       OUT int *piError )
    {
    int iRet = setsockopt( m_Socket,
                           level,
                           optname,
                           (const char *)optval,
                           optlen );

    if (iRet == SOCKET_ERROR)
        {
        *piError = WSAGetLastError();
        }
    else
        {
        *piError = NO_ERROR;
        }

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_GetSockOpt()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_GetSockOpt( IN int   iLevel,
                                       IN int   iOptname,
                                       IN unsigned char *pOptval,
                                       OUT int *piOptlen,
                                       OUT int *piError )
    {
    int iRet = getsockopt( m_Socket,
                           iLevel,
                           iOptname,
                           (char *)pOptval,
                           piOptlen );

    if (iRet == SOCKET_ERROR)
        {
        *piError = WSAGetLastError();
        }
    else
        {
        *piError = NO_ERROR;
        }

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_HelperWSALookupServiceBeginW()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_HelperWSALookupServiceBeginW(
                          IN  RPC_WSAQUERYSETW *pRestrictions,
                          IN  DWORD  dwControlFlags,
                          OUT DWORD *pdwLookupHandle,
                          OUT int   *piRet,
                          OUT int   *piErrno )
    {
    *piRet = SOCKET_ERROR;
    *piErrno = WSAEOPNOTSUPP;

    if ((!pRestrictions)||(!pRestrictions->lpServiceClassId))
       {
       *piErrno = WSAEINVAL;
       return NOERROR;
       }

    if ( (0
          != memcmp(pRestrictions->lpServiceClassId,&HostnameGuid,sizeof(GUID)))
       &&
         (0
          != memcmp(pRestrictions->lpServiceClassId,&AddressGuid,sizeof(GUID))))
       {
       *piErrno = WSAEACCES;  // or WSAEOPNOTSUPP?
       return NOERROR;
       }

    DWORD  dwFlagsMask = ~( LUP_RETURN_NAME
                          | LUP_RETURN_TYPE
                          | LUP_RETURN_VERSION
                          | LUP_RETURN_COMMENT
                          | LUP_RETURN_ADDR
                          | LUP_RETURN_BLOB );

    if (dwFlagsMask & dwControlFlags)
       {
       *piErrno = WSAEACCES;  // or WSAEOPNOTSUPP?
       return NOERROR;
       }

    *piRet = WSALookupServiceBeginW( (WSAQUERYSETW *)pRestrictions,
                                     dwControlFlags,
                                     (HANDLE*)pdwLookupHandle );

    if (*piRet == SOCKET_ERROR)
       {
       *piErrno = WSAGetLastError();
       }
    else
       {
       *piErrno = NO_ERROR;
       }

    return NOERROR;
    }

#define TO_OFFSET(base,offset,typ) \
        ((typ)((char*)(base) - (DWORD_PTR)(offset)))

//----------------------------------------------------------------
//  QuerySetPointersToOffsets()
//
//----------------------------------------------------------------
void QuerySetPointersToOffsets( WSAQUERYSETW *pqs )
    {
    if (pqs->lpszServiceInstanceName)
       {
       pqs->lpszServiceInstanceName = TO_OFFSET(pqs->lpszServiceInstanceName,pqs,LPWSTR);
       }

    if (pqs->lpServiceClassId)
       {
       pqs->lpServiceClassId = TO_OFFSET(pqs->lpServiceClassId,pqs,LPGUID);
       }

    if (pqs->lpVersion)
       {
       pqs->lpVersion = TO_OFFSET(pqs->lpVersion,pqs,LPWSAVERSION);
       }

    if (pqs->lpszComment)
       {
       pqs->lpszComment = TO_OFFSET(pqs->lpszComment,pqs,LPWSTR);
       }

    if (pqs->lpNSProviderId)
       {
       pqs->lpNSProviderId = TO_OFFSET(pqs->lpNSProviderId,pqs,LPGUID);
       }

    if (pqs->lpszContext)
       {
       pqs->lpszContext = TO_OFFSET(pqs->lpszContext,pqs,LPWSTR);
       }

    if (pqs->lpafpProtocols)
       {
       pqs->lpafpProtocols = TO_OFFSET(pqs->lpafpProtocols,pqs,LPAFPROTOCOLS);
       }

    if (pqs->lpszQueryString)
       {
       pqs->lpszQueryString = TO_OFFSET(pqs->lpszQueryString,pqs,LPWSTR);
       }

    if (pqs->lpcsaBuffer)
       {
       pqs->lpcsaBuffer = TO_OFFSET(pqs->lpcsaBuffer,pqs,LPCSADDR_INFO);
       }

    if (pqs->lpBlob)
       {
       pqs->lpBlob->pBlobData = TO_OFFSET(pqs->lpBlob->pBlobData,pqs,BYTE*);
       pqs->lpBlob = TO_OFFSET(pqs->lpBlob,pqs,LPBLOB);
       }
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_HelperWSALookupServiceNextW()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_HelperWSALookupServiceNextW(
                          IN  DWORD  dwLookupHandle,
                          IN  DWORD  dwControlFlags,
                          IN OUT DWORD *pdwBufferLength,
                          IN OUT UCHAR *pResults,  // WSAQUERYSETW*
                          IN OUT int   *piRet,
                          IN OUT int   *piErrno )
    {
    if (!pdwBufferLength)
       {
       *piRet = SOCKET_ERROR;
       *piErrno = WSAEINVAL;
       return NOERROR;
       }

    memset(pResults,0,*pdwBufferLength);

    *piRet = WSALookupServiceNextW( (HANDLE)dwLookupHandle,
                                    dwControlFlags,
                                    pdwBufferLength,
                                    (WSAQUERYSETW*)pResults );
    if (*piRet == SOCKET_ERROR)
       {
       *piErrno = WSAGetLastError();
       memset(pResults,0,*pdwBufferLength);
       }
    else
       {
       *piErrno = NO_ERROR;
       }

    //
    // Cache a copy of the HOSTENT data for use in a connect.
    //
    CacheHostEnt( ((WSAQUERYSETW*)pResults)->lpBlob );

    //
    // Make sure the HOSTENT is for an allowed site.
    //
    if (m_pHostEnt)
       {
       if (!IsValidAddr( m_pHostEnt->h_addr,
                         m_pHostEnt->h_length) )
          {
          delete m_pHostEnt;
          m_pHostEnt = 0;
          *piErrno = WSANO_DATA;
          memset(pResults,0,*pdwBufferLength);
          }
       }
    else
       {
       *piErrno = WSA_NOT_ENOUGH_MEMORY;
       memset(pResults,0,*pdwBufferLength);
       }

    //
    // Convert the pointers into offsets (for marshalling back to
    // the restricted process.
    //
    QuerySetPointersToOffsets( (WSAQUERYSETW*)pResults );

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_HelperWSALookupServiceEnd()
//
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_HelperWSALookupServiceEnd(
                          IN  DWORD  dwLookupHandle,
                          OUT int   *piRet,
                          OUT int   *piErrno )
    {
    *piRet = WSALookupServiceEnd( (HANDLE)dwLookupHandle );

    if (*piRet == SOCKET_ERROR)
       {
       *piErrno = WSAGetLastError();
       }
    else
       {
       *piErrno = NO_ERROR;
       }

    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::RP_HelperListen()
//
//  Listen is not allowed for restricted processes.
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::RP_HelperListen( DWORD    Socket,
                                         int      iBacklog,
                                         int     *piRet,
                                         int     *piErrno )
    {
    *piRet = SOCKET_ERROR;
    *piErrno = WSAEOPNOTSUPP;
    return NOERROR;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::GetSidFromAddr()
//
//  Convert the specified address (pAddr) into a SID. This is
//  done by checking the cache to see if the cached HOSTENT is
//  for the specified address. If yes, then convert the host DNS
//  name into a SID and returning it.
//----------------------------------------------------------------
PSID
CImpIRestrictedProcess::GetSidFromAddr( char *pAddr,
                                        long  lAddrLen )
    {
#   define  MAX_IP_HOST_NAME  256
    int   i = 0;
    PSID  pSid = 0;
    char *pListAddr;
    char  szHostName[MAX_IP_HOST_NAME];
    WCHAR wsHostName[MAX_IP_HOST_NAME];

    if (!m_pHostEnt)
       {
       //
       // If We don't already have a cached HOSTENT, then
       // try to resove one.
       //
       m_pHostEnt = gethostbyaddr( pAddr,
                                   lAddrLen,
                                   AF_INET );
       }

    if ((m_pHostEnt) && (lAddrLen == m_pHostEnt->h_length))
       {
       while (pListAddr=m_pHostEnt->h_addr_list[i++])
          {
          if (!memcmp(pAddr,pListAddr,lAddrLen))
             {
             break;
             }
          }

       if (pListAddr)
           {
           strcpy(szHostName,"http://");
           strcat(szHostName,m_pHostEnt->h_name);

           if (MultiByteToWideChar(CP_ACP,
                                   MB_PRECOMPOSED,
                                   szHostName,
                                   1+strlen(szHostName),
                                   wsHostName,
                                   MAX_IP_HOST_NAME))
              {
              pSid = GetSiteSidFromUrl(wsHostName);
              }
           }
       }

    return pSid;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::IsValidAddr()
//
//  Restricted processes have tokens which contain a SID which
//  represents the only valid site that they can connect to.
//  Compare the specified address with the restricted site in
//  the process token.
//
//  pAddr    -- The Address in network byte order.
//
//  lAddrLen -- The length (in bytes) of pAddr (should be 4).
//
//  Return S_OK iff the specified address is the same as that
//  in the restricted token.
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::IsValidAddr( char  *pAddr,
                                     long   lAddrLen )
    {
    HRESULT hr;
    BOOL    fIsValid;

    //
    // Impersonate the client and get a copy of its process token
    //
    hr = CoImpersonateClient();
    if (FAILED(hr))
       {
       return hr;
       }

    HANDLE hToken;
    HANDLE hThread = GetCurrentThread();  // Don't need to call CloseHandle()...
    DWORD  dwAccess = TOKEN_QUERY;
    DWORD  dwError;

    if (!OpenThreadToken(hThread,dwAccess,TRUE,&hToken))
       {
       dwError = GetLastError();
       hr = CoRevertToSelf();
       return E_ACCESSDENIED;
       }

    #ifdef DBG_ANY_CONNECTION

    // Test mode, allows any connection...
    hr = S_OK;
    fIsValid = TRUE;

    #else

    //
    // Get the SID representing the valid site that the client can
    // connect to. Then construct another SID from the pAddr that
    // the client is using. If they are equal then everything is Ok.
    // If not, then the address the client specified (pAddr) is not
    // a valid address to work with.
    //
    PSID pSid = GetSiteSidFromToken(hToken);

    if ((!pSid) || (!IsValidSid(pSid)))
       {
       hr = CoRevertToSelf();
       return E_ACCESSDENIED;
       }

    hr = CoRevertToSelf();

    PSID pAddrSid = GetSidFromAddr(pAddr,lAddrLen);

    if ((pAddrSid) && IsValidSid(pAddrSid))
       {
       fIsValid = EqualSid(pSid,pAddrSid);
       FreeSid(pSid);
       FreeSid(pAddrSid);
       }
    else
       {
       dwError = GetLastError();
       CloseHandle(hToken);

       if ((pSid) && IsValidSid(pSid))
          FreeSid(pSid);

       if ((pAddrSid) && IsValidSid(pAddrSid))
          FreeSid(pAddrSid);

       return E_ACCESSDENIED;
       }

    #endif

    CloseHandle(hToken);

    return (fIsValid)? S_OK : S_FALSE;
    }

//----------------------------------------------------------------
//  CImpIRestrictedProcess::UnpackList()
//
//  Utility function used by UnpackHostEnt() to convert a list
//  of offsets into pointers.
//----------------------------------------------------------------
VOID
CImpIRestrictedProcess::UnpackList( PCHAR ** List, PCHAR Base )
    {
    if(*List)
       {
       PCHAR * Addr;

       Addr = *List = (PCHAR *)( ((DWORD_PTR)*List + Base) );
       while(*Addr)
          {
          *Addr = (PCHAR)(((DWORD_PTR)*Addr + Base));
          Addr++;
          }
       }
    }


//----------------------------------------------------------------
//  CImpIRestrictedProcess::UnpackHostEnt()
//
//  Routine to convert a HOSTENT returned in a BLOB to one with
//  usable pointers. The structure is converted in-place.
//
//----------------------------------------------------------------
VOID
CImpIRestrictedProcess::UnpackHostEnt( HOSTENT *pHost )
{
     PCHAR pch;

     pch = (PCHAR)pHost;

     if(pHost->h_name)
        {
        pHost->h_name = (PCHAR)((DWORD_PTR)pHost->h_name + pch);
        }

     UnpackList(&pHost->h_aliases, pch);
     UnpackList(&pHost->h_addr_list, pch);
}

//----------------------------------------------------------------
//  CImpIRestrictedProcess::CacheHostEnt()
//
//  The pBlob contains a HOSTENT (but with the pointers converted
//  to offsets). Make a copy of it into a local cache (m_pHostEnt)
//  and unpack it into a true HOSTENT.
//----------------------------------------------------------------
STDMETHODIMP
CImpIRestrictedProcess::CacheHostEnt( BLOB *pBlob )
    {
    if (m_pHostEnt)
       {
       delete m_pHostEnt;
       }

    m_pHostEnt = (HOSTENT*)new char [pBlob->cbSize];
    if (!m_pHostEnt)
       {
       return E_OUTOFMEMORY;
       }

    memcpy(m_pHostEnt,pBlob->pBlobData,pBlob->cbSize);

    UnpackHostEnt(m_pHostEnt);

    return S_OK;
    }

#if FALSE
//----------------------------------------------------------------
//  CImpIRestrictedProcess::CopyHostEnt()
//
//----------------------------------------------------------------
HOSTENT *
CImpIRestrictedProcess::CopyHostEnt( HOSTENT *pHost )
    {
    int      i;
    DWORD    dwSize = 0;
    DWORD    dwNameSize = 0;
    DWORD    dwAliasSize = 0;
    HOSTENT *pHostCopy = 0;

    //
    // Calculate the memory needed to hold the HOSTENT:
    //
    dwSize = sizeof(HOSTENT);
    if (pHost->h_name)
       {
       dwNameSize = 1 + _mbslen( (UCHAR*)(pHost->h_name) );
       dwNameSize = ALIGN4(dwNameSize);
       dwSize += dwNameSize;
       }

    UCHAR **ppsz = (UCHAR**)(pHost->h_aliases);
    DWORD   dwnAliases = 0;
    if (ppsz)
       {
       dwSize += sizeof(char*);
       while (*ppsz)
          {
          dwAliasSize = 1 + _mbslen(*ppsz);
          dwSize += sizeof(char*) + ALIGN4(dwAliasSize);
          dwnAliases++;
          ppsz++;
          }
       }

    UCHAR **ppAddr = (UCHAR**)(pHost->h_addr_list);
    DWORD   dwnAddr = 0;
    if (ppAddr)
       {
       dwSize += sizeof(char*);
       while (*ppAddr)
          {
          dwSize += sizeof(char*) + pHost->h_length;
          dwnAddr++;
          ppAddr++;
          }
       }

    pHostCopy = (HOSTENT*)new char [dwSize];
    if (!pHostCopy)
       {
       return 0;   // out of memory.
       }

    memset(pHostCopy,0,dwSize);

    char *pEnd = (char*)pHostCopy + sizeof(HOSTENT);

    pHostCopy->h_name = pEnd;
    _mbscpy( (UCHAR*)(pHostCopy->h_name), (UCHAR*)(pHost->h_name) );

    pHostCopy->h_aliases = (char**)pEnd + dwNameSize;

    pEnd += dwNameSize + (1+dwnAliases)*sizeof(char*);
    for (i=0; i<dwnAliases; i++)
       {
       pHostCopy->h_aliases[i] = pEnd;
       _mbscpy( (UCHAR*)(pHostCopy->h_aliases[i]),
                (UCHAR*)(pHost->h_aliases[i]) );
       pEnd += 1 + _mbslen( (UCHAR*)(pHost->h_aliases[i]) );
       pEnd = (char*)ALIGN4( (DWORD)pEnd );
       }

    pHostCopy->h_addrtype = pHost->h_addrtype;
    pHostCopy->h_length = pHost->h_length;

    pHostCopy->h_addr_list = (char**)pEnd;

    pEnd += (1+dwnAddr)*(pHost->h_length);

    for (i=0; i<dwnAddr; i++)
       {
       pHostCopy->h_addr_list[i] = pEnd;
       memcpy(pHostCopy->h_addr_list[i],pHost->h_addr_list[i],pHost->h_length);
       pEnd += pHost->h_length;
       }

    return pHostCopy;
    }
#endif
