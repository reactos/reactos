//------------------------------------------------------------------
// Copyright (c)1997 Microsoft Corporation, All Right Reserved
//
// rprocess.h
//
//------------------------------------------------------------------

#ifndef _RPROCESS_H_
#define _RPROCESS_H_

#define INC_OLE2
#include <ole2.h>
#include <ole2ver.h>
#include <ws2_if.h>

typedef void (*PFNDESTROYED)(void);

#ifndef PPVOID
  typedef LPVOID * PPVOID;
#endif

#ifdef INITGUIDS
#include <initguid.h>
#endif //INITGUIDS

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------

#define ALIGN4(address) \
        ( ((address)%4)? (address)+4-((address)%4) : (address) )

//--------------------------------------------------------------------
// Restricted Process WinSock Helpers
//
//    Object:    {570da105-3c30-11d1-8bf1-0000f8754035}
//    Interface: {3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//    IDL Proxy: {3f7ec550-80a3-11d1-b222-00a0c90c91fe}
//--------------------------------------------------------------------

DEFINE_GUID(CLSID_RestrictedProcess,
            0x570da105, 
            0x3c30, 0x11d1, 0x8b, 0xf1, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35);

DEFINE_GUID(CLSID_RestrictedProcessProxy,
            0x3f7ec550, 
            0x80a3, 0x11d1, 0xb2, 0x22, 0x00, 0xa0, 0xc9, 0x0c, 0x91, 0xfe);

#if FALSE

... IID_IRestrictedProcess is already defined in ws2_if.h ...

DEFINE_GUID(IID_IRestrictedProcess,
            0x3ae0b7e0, 
            0x3c19, 0x11d1, 0x8b, 0xf1, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35);
#endif

//-----------------------------------------------------------------------
// CRestrictedProcess
//-----------------------------------------------------------------------

class CImpIRestrictedProcess;

typedef CImpIRestrictedProcess *PCImpIRestrictedProcess;

class CRestrictedProcess : public IUnknown
    {
    friend class CImpIRestrictedProcess;

    protected:
        ULONG           m_cRef;         //Object reference count
        LPUNKNOWN       m_pUnkOuter;    //Controlling unknown

        PFNDESTROYED    m_pfnDestroy;   //To call on closure

        PCImpIRestrictedProcess      m_pImpIRestrictedProcess;

    public:
        CRestrictedProcess(LPUNKNOWN, PFNDESTROYED);
        ~CRestrictedProcess(void);

        BOOL Init(void);

        //Non-delegating object IUnknown
        STDMETHODIMP         QueryInterface(REFIID, PPVOID);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
    };

typedef CRestrictedProcess *PCRestrictedProcess;

//-----------------------------------------------------------------------
// class CSocketsDictionary
//
// Dictionary of socket mappings from client handle (key) to the
// server handle (item).
//-----------------------------------------------------------------------

NEW_BASETYPE_DICTIONARY(CSockets,SOCKET,SOCKET);

//-----------------------------------------------------------------------
// class CImpRestrictedProcess
//
//-----------------------------------------------------------------------
class CImpIRestrictedProcess : public IRestrictedProcess
    {
    protected:
        ULONG                m_cRef;       // Interface reference count.
        PCRestrictedProcess  m_pObj;       // Back pointer to the object.
        LPUNKNOWN            m_pUnkOuter;  // For delegation.

    private:
        RTL_CRITICAL_SECTION m_cs;         // Synchronization.
        HANDLE               m_hHelper;    // Winsock2 helper handle.
        SOCKET               m_Socket;     // The real socket.
        CSocketsDictionary  *m_pSocketDict;// Socket dictionary maps socket 
                                           // handles between the restricted 
                                           // client and the helper process.
        HOSTENT             *m_pHostEnt;

        PSID         GetSidFromAddr( char *pAddr,
                                     long  lAddrLen );

        STDMETHODIMP IsValidAddr( char  *pAddr,
                                  long   lAddrLen );

        VOID         UnpackList(PCHAR ** List, PCHAR Base);
        VOID         UnpackHostEnt( HOSTENT *pHost );
        STDMETHODIMP CacheHostEnt( BLOB *pBlob );

    public:
        CImpIRestrictedProcess( PCRestrictedProcess, LPUNKNOWN );
        ~CImpIRestrictedProcess(void);

        // IUnknown members:

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IRestrictedProcess members:

        STDMETHODIMP RP_HelperInit();

        STDMETHODIMP RP_WahOpenHandleHelper( IN  DWORD  dwTargetPid,
                                             OUT DWORD *pdwHelperHandle,
                                             OUT DWORD *pdwStatus );

        STDMETHODIMP RP_WahCreateSocketHandle( IN  DWORD  dwTargetPid,
                                               IN  DWORD  dwHelperHandle,
                                               OUT DWORD *pdwSocket,
                                               OUT DWORD *pdwStatus );

        STDMETHODIMP RP_WSPSocket( IN long   af,
                                   IN long   type,
                                   IN long   protocol,
                                   IN RPC_WSAPROTOCOL_INFOW *pProtocolInfo,
                                   IN RPC_GROUP g,
                                   IN DWORD     dwFlags,
                                   IN OUT long *pError  );

        STDMETHODIMP RP_WSPConnect( IN RPC_SOCKADDR_IN *pAddr,
                                    IN long        lAddrLen,
                                    IN RPC_WSABUF *pCallerData,
                                    IN OUT RPC_WSABUF *pCalleeData,
                                    IN RPC_QOS    *pSQOS,
                                    IN RPC_QOS    *pGQOS,
                                    IN DWORD       dwTargetPid,
                                    OUT DWORD     *pTargetSocekt,
                                    OUT long      *plError );

        STDMETHODIMP RP_SetSockOpt( IN int   iLevel,
                                    IN int   iOptname,
                                    IN unsigned char *pOptval,
                                    IN int   iOptlen,
                                    OUT int *piError );

        STDMETHODIMP RP_GetSockOpt( IN int   iLevel,
                                    IN int   iOptname,
                                    OUT unsigned char *pOptval,
                                    OUT int *pOptlen,
                                    OUT int *piError );

        STDMETHODIMP RP_HelperListen( DWORD  Socket,
                                      int    iBacklog,
                                      int   *piRet,
                                      int   *piErrno );

        STDMETHODIMP RP_HelperWSALookupServiceBeginW(
                          IN  RPC_WSAQUERYSETW *pRestrictions,
                          IN  DWORD  dwControlFlags,
                          IN OUT DWORD *pdwLookupHandle,
                          IN OUT int   *piRet,
                          IN OUT int   *piErrno );

        STDMETHODIMP RP_HelperWSALookupServiceNextW(
                          IN  DWORD  dwLookupHandle,
                          IN  DWORD  dwControlFlags,
                          IN OUT DWORD *pdwBufferLength,
                          IN OUT UCHAR *pResults,
                          IN OUT int   *piRet,
                          IN OUT int   *piErrno );

        STDMETHODIMP RP_HelperWSALookupServiceEnd(
                          IN     DWORD  dwLookupHandle,
                          IN OUT int   *piRet,
                          IN OUT int   *piErrno );

    };

#endif //_RPROCESS_H_

