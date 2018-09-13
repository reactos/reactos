//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CNet.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _CNET_HXX_
#define _CNET_HXX_

#include <wininet.h>
const ULONG SENDBUFFER_MAX = 8192;
const ULONG MAX_ACP_ENCHEADERS_SIZE=128; 

#ifdef unix
#undef offsetof
#endif /* unix */
#define offsetof(s,m) ( (SIZE_T) &(((s *)0)->m) )
//+----------------------------------------------------------------------------
//
//      Macro:
//              GETPPARENT
//
//      Synopsis:
//              Given a pointer to something contained by a struct (or
//              class,) the type name of the containing struct (or class),
//              and the name of the member being pointed to, return a pointer
//              to the container.
//
//      Arguments:
//              [pmemb] -- pointer to member of struct (or class.)
//              [struc] -- type name of containing struct (or class.)
//              [membname] - name of member within the struct (or class.)
//
//      Returns:
//              pointer to containing struct (or class)
//
//      Notes:
//              Assumes all pointers are FAR.
//
//      History:
//
//-----------------------------------------------------------------------------
#define GETPPARENT(pmemb, struc, membname) (\
                (struc*)(((char*)(pmemb))-offsetof(struc, membname)))


#define INET_E_OK                       NOERROR
#define INET_E_DONE                     _HRESULT_TYPEDEF_(0x800A0001L)
#define AUTHENTICATE_MAX 3      // number of authentication retries

typedef enum
{
     INetState_None              = 0
    ,INetState_START             = 1
    ,INetState_OPEN_REQUEST
    ,INetState_CONNECT_REQUEST
    ,INetState_PROTOPEN_REQUEST
    ,INetState_SEND_REQUEST
    ,INetState_AUTHENTICATE
    ,INetState_DISPLAY_UI
    ,INetState_READ
    ,INetState_DATA_AVAILABLE
    ,INetState_READ_DIRECT
    ,INetState_DATA_AVAILABLE_DIRECT
    ,INetState_INETSTATE_CHANGE
    ,INetState_DONE
    ,INetState_ABORT
    ,INetState_ERROR
} INetState;


typedef enum
{
     HandleState_None           = 0
    ,HandleState_Pending        = 1
    ,HandleState_Initialized
    ,HandleState_Aborted
    ,HandleState_Closed
} HandleState;


class CUrl;
class CAuthData;
class CINet;
class CINetEmbdFilter;
class CINetProtImpl; 




//+---------------------------------------------------------------------------
//
//  Class:      CINet ()
//
//  Purpose:    class to interact with WinINet
//
//  Interface:  dwFlags --
//              hThread --
//              dwThreadID --
//              state --
//              uError --
//              ipPort --
//              postData --
//              dwProto --
//              _hServer --
//              _hRequest --
//              _dwIsA --
//              _cbDataSize --
//              _cbTotalBytesRead --
//              _cbReadReturn --
//              _pCTransData --
//              _pCTrans --
//              _hrINet --
//              _dwState --
//              _hr --
//              INetAsyncStart --
//              OnINetStart --
//              INetAsyncOpen --
//              OnINetAsyncOpen --
//              OnINetOpen --
//              INetAsyncConnect --
//              OnINetConnect --
//              INetAsyncOpenRequest --
//              OnINetOpenRequest --
//              INetAsyncSendRequest --
//              OnINetSendRequest --
//              INetQueryInfo --
//              OnINetRead --
//              INetRead --
//              GetTransBindInfo --
//              CINetCallback --
//              TransitState --
//              OnINetInternal --
//              OnINetError --
//              ReportResultAndStop --
//              TerminateRequest --
//              FindTagInHeader --
//              ReportNotification --
//              szBaseURL --
//              szBaseURL --
//              OperationOnAparmentThread --
//              QueryInfoOnResponse --
//              CINet --
//              _dwIsA --
//              ~CINet --
//              QueryStatusOnResponse --
//              QueryHeaderOnResponse --
//              RedirectRequest --
//              AuthenticationRequest --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINet : public IOInetProtocol, public IWinInetHttpInfo, public IOInetThreadSwitch, public IOInetPriority, public CUrl
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Start(
        LPCWSTR szUrl,
        IOInetProtocolSink *pProtSink,
        IOInetBindInfo *pOIBindInfo,
        DWORD grfSTI,
        DWORD_PTR dwReserved
        );

    STDMETHODIMP Continue(
        PROTOCOLDATA *pStateInfo
        );

    STDMETHODIMP Abort(
        HRESULT hrReason,
        DWORD dwOptions
        );

    STDMETHODIMP Terminate(
        DWORD dwOptions
        );

    STDMETHODIMP Suspend();

    STDMETHODIMP Resume();

    STDMETHODIMP SetPriority(LONG nPriority);

    STDMETHODIMP GetPriority(LONG * pnPriority);

    virtual STDMETHODIMP Read(
        void *pv,
        ULONG cb,
        ULONG *pcbRead);

    STDMETHODIMP Seek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP LockRequest(
        DWORD dwOptions);

    STDMETHODIMP UnlockRequest();

    // IWinInetInfo
    STDMETHODIMP QueryOption(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf);

    // IWinInetHttpInfo
    STDMETHODIMP QueryInfo(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved);

    // IOInetThreadSwitch
    STDMETHODIMP Prepare();

    STDMETHODIMP Continue();

    static DWORD CALLBACK InternetAuthNotifyCallback(
                           DWORD_PTR dwContext,             // the this pointer
                           DWORD dwAction,              // action to be taken
                           LPVOID lpReserved);          // unused


    // My Interface
    STDMETHODIMP MyContinue(
        PROTOCOLDATA *pStateInfo
        );

    STDMETHODIMP MyAbort(
        HRESULT hrReason,
        DWORD dwOptions
        );

    STDMETHODIMP MyTerminate(
        DWORD dwOptions
        );

    STDMETHODIMP MySuspend();

    STDMETHODIMP MyResume();

    STDMETHODIMP MySetPriority(LONG nPriority);

    STDMETHODIMP MyGetPriority(LONG * pnPriority);

    virtual STDMETHODIMP MyRead(
        void *pv,
        ULONG cb,
        ULONG *pcbRead);

    STDMETHODIMP MySeek(
        LARGE_INTEGER dlibMove,
        DWORD dwOrigin,
        ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP MyLockRequest(
        DWORD dwOptions);

    STDMETHODIMP MyUnlockRequest();


public: //CINet data
//private:
//protected:

    DWORD           _dwFlags;
    DWORD           _dwThreadID;
    HRESULT         _hrError;     // error code
    INetState       _INState;

    LPWSTR          _pwzUrl;
    LPSTR           _pszUserAgentStr;

    // wininet handles
    HINTERNET       _hServer;
    HINTERNET       _hRequest;

    DWORD           _dwIsA;
    ULONG           _cbDataSize;
    ULONG           _cbTotalBytesRead;
    ULONG           _cbReadReturn;
    ULONG           _cbReadyToRead;
    ULONG           _cbRedirect;
    ULONG           _cbAuthenticate;
    ULONG           _cbProxyAuthenticate;
    ULONG           _cbSizeLastReportData;
    DWORD           _bscf;
    BOOL            _fProxyAuth;

    IOInetProtocolSink  *_pCTrans;
    IOInetBindInfo      *_pOIBindInfo;
    IServiceProvider    *_pServiceProvider;
    IWindowForBindingUI *_pWindow;      // Pointer to IWindowForBindingUI

    // Filter Handler, can handle cascade filters
    CINetEmbdFilter*     _pEmbdFilter;

    HRESULT         _hrINet;
    INetState       _dwState;
    HRESULT         _hrPending;
    CRefCount       _cReadCount;
    CMutexSem       _mxs;
    DWORD           _dwCacheFlags;
    DWORD           _dwConnectFlags;
    DWORD           _dwOpenFlags;
    DWORD           _dwSendRequestResult;
    LPVOID          _lpvExtraSendRequestResult;
    BOOL            _fRedirected;
    BOOL            _fP2PRedirected;
    BOOL            _fLocked;
    BOOL            _fDoSimpleRetry;
    BOOL            _fCompleted;
    BOOL            _fSendAgain;
    BOOL            _fSendRequestAgain;
    BOOL            _fForceSwitch;

    BOOL            _fFilenameReported;
    HANDLE          _hFile;
    HANDLE          _hLockHandle;

    REFCLSID        _pclsidProtocol;
    DWORD           _dwResult;
    LPSTR           _pszResult;

    HandleState     _HandleStateServer;
    HandleState     _HandleStateRequest;

    CAuthData      *_pCAuthData;
    HWND            _hwndAuth;

    //new
    DWORD           _grfBindF;
    BINDINFO        _BndInfo;
    LONG            _nPriority;

    // read direct stuff
    INTERNET_BUFFERS  _inetBufferSend;
    INTERNET_BUFFERS  _inetBufferReceived;


private:
    CRefCount       _CRefs;          // the total refcount of this object
    CRefCount       _CRefsHandles;   // the number of handles in use by this object

    union
    {
        ULONG _flags;
#ifndef unix
        struct
        {
#endif /* unix */
            unsigned _fAborted  : 1;
            unsigned _fDone     : 1;
#ifndef unix
        };
#endif /* unix */
    };


public:

    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        ~CPrivUnknown() {}
        CPrivUnknown() : _CRefs() {}

    private:
        CRefCount   _CRefs;          // the total refcount of this object
    };

    friend class CPrivUnknown;
    CPrivUnknown     _Unknown;

    IUnknown        *_pUnkOuter;

public:
    CINet(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINet();

    IUnknown *GetIUnkInner(BOOL fReleaseOuter = FALSE)
    {
        if (fReleaseOuter)
        {
            PProtAssert((_CRefs > 0));

            _CRefs--;
        }
        return &_Unknown;
    }

protected:

    HRESULT INetAsyncStart();
    HRESULT OnINetStart();
    virtual HRESULT INetAsyncOpen();
    HRESULT OnINetAsyncOpen(DWORD dwResult);
    HRESULT OnINetOpen(DWORD dwResult);
    virtual HRESULT INetAsyncConnect();
    HRESULT OnINetConnect(DWORD dwResult);
    virtual HRESULT INetAsyncOpenRequest();
    HRESULT OnINetOpenRequest(DWORD dwResult);
    virtual HRESULT INetAsyncSendRequest();
    HRESULT OnINetSendRequest( DWORD dwResult);
    HRESULT OnINetSuspendSendRequest( DWORD dwResult, LPVOID lpvExtraResult);
    HRESULT INetResumeAsyncRequest(DWORD dwResultCode);

    virtual HRESULT INetStateChange();
    HRESULT OnINetStateChange(DWORD dwResult);

    HRESULT INetQueryInfo();

    HRESULT INetAuthenticate();
    HRESULT OnINetAuthenticate(DWORD dwResult);
    
    HRESULT INetDisplayUI();

    HRESULT OnINetRead(DWORD dwResult);
    HRESULT INetRead();
    virtual HRESULT INetWrite();

    

    HRESULT INetDataAvailable();
    HRESULT OnINetDataAvailable( DWORD dwResult);
    HRESULT INetReportAvailableData();

    HRESULT OnINetReadDirect(DWORD dwResult);
    HRESULT INetReadDirect();
    HRESULT ReadDirect(BYTE *pBuffer, DWORD cbBytes, DWORD *pcbBytes);


    HRESULT GetBindResult(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved);
    virtual HRESULT SetBindResult(DWORD dwResult, HRESULT hrSuggested = NOERROR);

    void SetCNetBindResult(DWORD dwResult, LPSTR szResult = NULL)
    {
        _dwResult = dwResult;
        PProtAssert((_pszResult == NULL));
        _pszResult = szResult;
    }
    BOOL IsApartmentThread()
    {
       PProtAssert((_dwThreadID != 0));
       return (_dwThreadID == GetCurrentThreadId());
    }

    void TransitState(DWORD dwState, BOOL fAsync = FALSE);
    void OnINetInternal(DWORD_PTR dwState);
    HRESULT ReportResultAndStop(HRESULT hr,ULONG ulProgress = 0, ULONG ulProgressMax = 0,LPWSTR pwzStr = 0);
    HRESULT ReportNotification(BINDSTATUS NMsg, LPSTR szStr = NULL);

    void TerminateRequest();
    virtual HRESULT LockFile(BOOL fRetrieve = FALSE);
    virtual HRESULT UnlockFile();
    virtual HRESULT INetSeek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);

    HRESULT SetStatePending(HRESULT hrNew);
    HRESULT GetStatePending();
    REFCLSID GetProtocolClassID()
    {
        return _pclsidProtocol;
    }

    static VOID CALLBACK CINetCallback(
                           IN HINTERNET   hInternet,                 //
                           IN DWORD_PTR   dwContext,                 // could be LPDLD
                           IN DWORD       dwInternetStatus,
                           IN LPVOID      lpvStatusInformation,
                           IN DWORD       dwStatusInformationLength);


    LPSTR FindTagInHeader(LPCSTR lpBuffer, LPCSTR lpszTag);
    HRESULT ReadDataHere(BYTE *pBuffer, DWORD cbBytes, DWORD *pcbBytes);

    HRESULT GetUrlCacheFilename(LPSTR szFilename, DWORD dwSize);
    BOOL OperationOnAparmentThread(DWORD dwState);
    HRESULT QueryInfoOnResponse();
    HRESULT OnRedirect(LPSTR szNewUrl);
    HRESULT HResultFromInternetError(DWORD dwStatus);

    INetState SetINetState(INetState inState);
    INetState GetINetState();
    void  SetByteCountReadyToRead(LONG cbReadyReadNow);
    ULONG GetByteCountReadyToRead();
    BOOL  UTF8Enabled();


#ifdef unix
// If this was not public we get a warning from
// cnet.cxx line that:
// cnet.cxx", line 238: Warning (Anachronism): static CINet::operator delete(void*, unsigned) is not accessible from CreateKnownProtocolInstance(unsigned long, const _GUID&, IUnknown*, const _GUID&, IUnknown**).

public:
#endif /* unix */
    void operator delete( void *pBuffer, size_t size)
    {
        memset(pBuffer,0, size);
        ::delete pBuffer;
    }
#ifdef unix
protected:
#endif /* unix */

    DWORD IsA() {  return _dwIsA;   };

    virtual HRESULT QueryStatusOnResponse();
    virtual HRESULT QueryStatusOnResponseDefault(DWORD dwStat);
    virtual HRESULT QueryHeaderOnResponse();
    virtual HRESULT RedirectRequest(LPSTR lpszBuffer, DWORD *pdwBuffSize );
    virtual HRESULT AuthenticationRequest();

    //NEW
    DWORD GetBindFlags()
    {
        return _grfBindF;
    }
    BINDINFO *GetBindInfo()
    {
        return &_BndInfo;
    }

    LPWSTR GetUrl()
    {
        return _pwzUrl;
    }

    LPSTR GetUserAgentString();
    
    BOOL IsUpLoad();


    HRESULT QueryService(REFIID riid, void ** ppvObj)
    {
        HRESULT hr = E_NOINTERFACE;
        if (!_pServiceProvider)
        {
            hr = _pCTrans->QueryInterface(IID_IServiceProvider, (void **)&_pServiceProvider);
        }
        if (_pServiceProvider)
        {
            hr = _pServiceProvider->QueryService(riid, riid, ppvObj);
        }
        return hr;
    }

    STDMETHODIMP_(ULONG) PrivAddRef(BOOL fAddRefHandle = FALSE)
    {
        if (fAddRefHandle)
        {
            _CRefsHandles++;
        }
        return _Unknown.AddRef();
    }
    STDMETHODIMP_(ULONG) PrivRelease(BOOL fReleaseHandle = FALSE)
    {
        if (fReleaseHandle)
        {
            _CRefsHandles--;
        }
        return _Unknown.Release();
    }



};

class CAuthData : public INTERNET_AUTH_NOTIFY_DATA
{
public:
    CAuthData(CINet *pCINet)
    {
        PProtAssert((pCINet));
        cbStruct = sizeof(INTERNET_AUTH_NOTIFY_DATA);
        dwOptions = 0;
        pfnNotify = pCINet->InternetAuthNotifyCallback;
        dwContext = (DWORD_PTR) pCINet;
        pCINet->AddRef();
    }
    ~CAuthData()
    {
        PProtAssert((dwContext));
        ((CINet *)dwContext)->Release();
    }
    friend CINet;
};

class CStateInfo : public PROTOCOLDATA
{
    public:
    CStateInfo(DWORD dwSt, DWORD dwFl, LPVOID pD = NULL, DWORD cb = 0)
    {
         grfFlags   = dwFl;
         dwState    = dwSt;
         pData      = pD;
         cbData     = cb;
    }
    ~CStateInfo()
    {
        if (pData && cbData)
        {
            delete pData;
        }
    }

    private:
};

//+---------------------------------------------------------------------------
//
//  Class:      CINetFile ()
//
//  Purpose:    Simple file protocol
//
//  Interface:  CINetFile --
//              ~CINetFile --
//              INetAsyncOpen --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetFile : public CINet
{
public:

    CINetFile(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetFile();
    virtual HRESULT INetAsyncOpen();
    virtual HRESULT LockFile(BOOL fRetrieve = FALSE);
    virtual HRESULT UnlockFile();
    virtual STDMETHODIMP Read(void *pBuffer, DWORD cbBytes, DWORD *pcbBytes);
private:

    LPWSTR  GetObjectNameW()
    {
        if( _wzFileName[0] == '\0' )
        {
            DWORD dwLen = MAX_PATH;
            PathCreateFromUrlW(_pwzUrl, _wzFileName, &dwLen, 0);
        }

        return _wzFileName;
    }

    HANDLE _hFile;      // handle to open file for locking the file
    WCHAR  _wzFileName[MAX_PATH];
};

//+---------------------------------------------------------------------------
//
//  Class:      CINetHttp ()
//
//  Purpose:    Http protocoll class
//
//  Interface:  INetAsyncOpenRequest --
//              INetAsyncSendRequest --
//              QueryStatusOnResponse --
//              QueryHeaderOnResponse --
//              RedirectRequest --
//              GetVerb --
//              GetDataToSend --
//              GetAdditionalHeader --
//              CallHttpNegBeginningTransaction --
//              CallHttpNegOnResponse --
//              CINetHttp --
//              ~CINetHttp --
//              _pszHeader --
//              _pHttpNeg --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetHttp : public CINet
{
public:

    STDMETHODIMP Terminate(DWORD dwOptions);

    virtual HRESULT INetAsyncOpenRequest();
    virtual HRESULT INetAsyncSendRequest();

    virtual HRESULT QueryStatusOnResponse();
    virtual HRESULT QueryStatusOnResponseDefault(DWORD dwStat);
    virtual HRESULT QueryHeaderOnResponse();
    virtual HRESULT RedirectRequest(LPSTR lpszBuffer, DWORD *pdwBuffSize );

    LPSTR   GetVerb();
    HRESULT GetDataToSend(LPVOID *ppBuffer, DWORD *pdwSize);
    HRESULT GetAdditionalHeader();
    HRESULT ErrorHandlingRequest(DWORD dwstatus, LPSTR szBuffer);
    HRESULT HResultFromHttpStatus(DWORD dwStatus);
    HRESULT HttpSecurity(DWORD dwProblem);
    HRESULT HttpNegBeginningTransaction();
    HRESULT HttpNegOnHeadersAvailable(DWORD dwResponseCode, LPSTR szHeaders);
    HRESULT HttpNegOnError(DWORD dwResponseCode, LPSTR szResponseHeaders);
    HRESULT HttpSecurityProblem(HWND* phwnd, DWORD dwProblem);
    HRESULT ZonesSecurityCheck(HWND hWnd, DWORD dwProblem, DWORD *pdwError);
    // new definitions for file upload

    HRESULT INetAsyncSendRequestEx();
    virtual HRESULT INetWrite();
    HRESULT GetNextSendBuffer(INTERNET_BUFFERS *pIB, IStream *pStm, BOOL fFirst = FALSE);


    CINetHttp(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetHttp();

private:
    LPSTR               _pszHeader;
    LPSTR               _pszSendHeader;
    LPSTR               _pwzAddHeader;
    IHttpNegotiate      *_pHttpNeg;     // point to HttpNeg holder
    IHttpSecurity       *_pHttSecurity; // Pointer to

    // file upload stuff
    char             *_pBuffer;   //[SENDBUFFER_MAX];
    DWORD             _dwBufferSize;
    DWORD             _dwBytesSent;
    IStream          *_pStm;            // the stream we read from
    LPSTR             _pszVerb;
    DWORD             _dwSendTotalSize;
    BOOL              _fSendEnd     : 1;
    BOOL              _f2ndCacheKeySet;
};


//+---------------------------------------------------------------------------
//
//  Class:      CINetHttpS ()
//
//  Purpose:
//
//  Interface:  CINetHttpS --
//              ~CINetHttpS --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetHttpS : public CINetHttp
{
public:

    // constructor does some special initialization
    CINetHttpS(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetHttpS();

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CINetSimple ()
//
//  Purpose:    Class for simple protocolls such as ftp and gopher
//
//  Interface:  INetAsyncOpenRequest --
//              INetAsyncSendRequest --
//              INetAsyncConnect --
//              CINetSimple --
//              ~CINetSimple --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetSimple : public CINet
{
public:
    // CINetSimple implements the following methods
    virtual HRESULT INetAsyncOpenRequest();
    virtual HRESULT INetAsyncSendRequest();
    virtual HRESULT INetAsyncConnect();

    virtual HRESULT QueryStatusOnResponse();
    virtual HRESULT QueryStatusOnResponseDefault(DWORD dwStat);
    virtual HRESULT QueryHeaderOnResponse();
    
    CINetSimple(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetSimple();

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CINetFtp ()
//
//  Purpose:    class for ftp
//
//  Interface:  CINetFtp --  does all special ftp intialization
//              ~CINetFtp --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetFtp : public CINetSimple
{
public:
    // constructor does some special initialization
    CINetFtp(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    HRESULT INetAsyncSendRequest();
    virtual ~CINetFtp();

private:

};

//+---------------------------------------------------------------------------
//
//  Class:      CINetGopher ()
//
//  Purpose:
//
//  Interface:  CINetGopher --   does all gopher special initialization
//              ~CINetGopher --
//
//  History:    2-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetGopher : public CINetSimple
{
public:
    // constructor does some special initialization
    CINetGopher(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetGopher();

private:

};

class CINetStream : public CINet
{
public:
    CINetStream(REFCLSID rclsid, IUnknown *pUnkOuter = 0);
    virtual ~CINetStream();
    virtual HRESULT INetAsyncOpen();
    virtual STDMETHODIMP Read(void *pBuffer, DWORD cbBytes, DWORD *pcbBytes);
    virtual HRESULT INetSeek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
    virtual HRESULT LockFile(BOOL fRetrieve = FALSE);
    virtual HRESULT UnlockFile();
private:
    IStream *_pstm;

};


//+---------------------------------------------------------------------------
//
//  Class:      CINetProtImpl()
//
//  Purpose:    the implementation of CINet's protocol interface 
//
//  Interface:  supports OINetProtocol methods.
//
//  History:    11-17-97   DanpoZ(Danpo Zhang)   Created
//
//  Notes:      1. this object is ONLY used by CINetEmbdFilter, 
//                 the IUnknown interface thus is not implemented
//                 when CINetEmbdFilter get destroied, this object
//                 will be destroied as well.
//            
//              2. All the IOInetProtocol methods gets delegated
//                 to CINet->MyXXXXX() method, where XXXXX are the
//                 names of the IOInetProtocol method
//
//----------------------------------------------------------------------------
class CINetProtImpl : public IInternetProtocol
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj)
    {
        return _pCINet->QueryInterface(iid, ppvObj);
    }
    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return 1;
    }
    STDMETHODIMP_(ULONG) Release(void)
    {
        return 1;
    }

    STDMETHODIMP Start( LPCWSTR, IOInetProtocolSink*, IOInetBindInfo*,
        DWORD, DWORD_PTR )
    {
        return NOERROR;
    }

    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo)
    {
        return _pCINet->MyContinue(pStateInfo);
    }

    STDMETHODIMP Abort(HRESULT hrReason,DWORD dwOptions)
    {
        return _pCINet->MyAbort(hrReason, dwOptions); 
    }

    STDMETHODIMP Terminate(DWORD dwOptions)
    {
        return _pCINet->MyTerminate(dwOptions);
    }

    STDMETHODIMP Suspend()
    {
        return _pCINet->MySuspend();
    }

    STDMETHODIMP Resume()
    {
        return _pCINet->MyResume();
    }

    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead)
    {
        return _pCINet->MyRead(pv, cb, pcbRead);
    }

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                        ULARGE_INTEGER *plibNewPosition)
    {
        return _pCINet->MySeek(dlibMove, dwOrigin, plibNewPosition);
    }

    STDMETHODIMP LockRequest(DWORD dwOptions)
    {
        return _pCINet->MyLockRequest(dwOptions);
    }

    STDMETHODIMP UnlockRequest()
    {
        return _pCINet->MyUnlockRequest();
    }

    CINetProtImpl(CINet* pCINet)
    {
        _pCINet = pCINet;
    }
    virtual ~CINetProtImpl()
    {
    }

    void Destroy()
    {
        delete this;
    }
friend class CINetEmbdFilter;
private:
    CINet*                  _pCINet;        // Filters' Imp  
};

//+---------------------------------------------------------------------------
//
//  Class:      CINetEmbdFilter()
//
//  Purpose:    Default protocol filter for CINet, all the filter
//              stacking and chaining are implemented thorugh this
//              object
//
//  Interface:  supports OINetProtocol and OINetProtocolSink methods.
//              However, this is not a COM object
//
//  History:    11-17-97   DanpoZ(Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CINetEmbdFilter 
{
public:
    CINetEmbdFilter(CINet* pCINet, IOInetProtocolSink* pSnk)
    {
        // Construct is called via CINet::Start()
        _pProtSink = pSnk;
        _pProtSink->AddRef();

        // Get Prot, only CINet can generate that
        _pProt = new CINetProtImpl(pCINet);

        _pProtImp = NULL;       
        _pProtSinkOrig = NULL;

        // number of filter stacked
        _cbStacked = 0;     
    }
    virtual ~CINetEmbdFilter()
    {
        //
        // dtor is called via CINet::~CINet()
        // _pProtSink is released on Terminate, otherwise, CTrans
        // won't be destroyed, thus CINet never get destory        
        // ole style destroy, since delete _pProt won't fire
        // CINetProtImpl's dtor
        // 
        ((CINetProtImpl*)_pProt)->Destroy();

        _pProt = NULL;
        _pProtSink = NULL;
    }
    
    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo)
    {
        return _pProt->Continue(pStateInfo);
    }

    STDMETHODIMP Abort(HRESULT hrReason,DWORD dwOptions)
    {
        return _pProt->Abort(hrReason, dwOptions); 
    }

    STDMETHODIMP Terminate(DWORD dwOptions)
    {
        HRESULT hr = NOERROR;
        hr = _pProt->Terminate(dwOptions);

        // unload all the filters
        _pProt->Release();
        _pProtSink->Release();

        // all filters are gone now
        // reconnect Imp for the last call (usually UnlockRequest)
        if( _pProtImp )
        {
            _pProt =  _pProtImp;
        }


        // we need to release this original pointer (CTrans)
        if( _pProtSinkOrig )
        {
            _pProtSink = _pProtSinkOrig;
            _pProtSinkOrig->Release();
        }
        
        return hr;
    }

    STDMETHODIMP Suspend()
    {
        return _pProt->Suspend();
    }

    STDMETHODIMP Resume()
    {
        return _pProt->Resume();
    }

    STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead)
    {
        return _pProt->Read(pv, cb, pcbRead);
    }

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,
                        ULARGE_INTEGER *plibNewPosition)
    {
        return _pProt->Seek(dlibMove, dwOrigin, plibNewPosition);
    }

    STDMETHODIMP LockRequest(DWORD dwOptions)
    {
        return _pProt->LockRequest(dwOptions);
    }

    STDMETHODIMP UnlockRequest()
    {
        return _pProt->UnlockRequest();
    }

    //
    // IOInetProtocolSink methods
    STDMETHODIMP Switch(PROTOCOLDATA *pStateInfo)
    {
        return _pProtSink->Switch(pStateInfo);
    }

    STDMETHODIMP ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        return _pProtSink->ReportProgress(ulStatusCode, szStatusText);
    }

    STDMETHODIMP ReportData( DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
    {
        return _pProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
    }

    STDMETHODIMP ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR wzResult)
    {
        return _pProtSink->ReportResult(hrResult, dwError, wzResult);
    }
    

    // Plug in filters
    // BUG, only support 1, pProt is OK, the logic for pSink needs some
    // work so that it can be STACKED...
    // 
    STDMETHODIMP StackFilter(
        LPCWSTR             pwzUrl,
        IOInetProtocol*     pProt, 
        IOInetProtocolSink* pProtSink,
        IOInetBindInfo*     pBindInfo )
    {
        HRESULT hr = E_FAIL;

        // keep the original prot and sink 
        if( !_pProtImp )
        {
            //
            // _pProtImp should always be the last pProt on the Prot Chain
            // we only need to change once
            // no matter how many times ::StackFilter() gets called.
            //
            _pProtImp = _pProt;       
        }

        // If another filter already stacked, we have to
        // move the additional ref count on the protocol 
        // interface of that filter
        if( _pProt != _pProtImp )
        {
            _pProt->Release();
        }

        // the new filter is plugged on top of _pProt and 
        // under _pProtSinkOrig 
        IOInetProtocol* pProtUnder = _pProt;
        _pProt = pProt;
        
        PROTOCOLFILTERDATA FilterData = 
            {sizeof(PROTOCOLFILTERDATA), 0 ,0, 0,0};
        FilterData.pProtocolSink = 0;
        FilterData.pProtocol = 0;
        FilterData.pUnk = 0;
        FilterData.dwFilterFlags = 0;
        FilterData.pProtocol = pProtUnder;

        //
        // since we are stacking, so the newly stacked filter should 
        // ALWAYS points to _pProtSinkOrig
        //
        hr = _pProt->Start(
            pwzUrl, _pProtSinkOrig, pBindInfo, PI_FILTER_MODE | PI_FORCE_ASYNC, 
            (DWORD_PTR) &FilterData ); 

        _cbStacked++;

        return hr;
    }

    //
    // Before SwitchSink get called, the sink of the top filter
    // is pointing to _pProtSinkOrig
    // After SwitchSink, the sink of the top filter is pointing
    // to the sink we just passed in (which is the sink for the new
    // top level filter)
    //
    STDMETHODIMP SwitchSink(IOInetProtocolSink* pProtSink)
    {
        HRESULT hr = NOERROR;
        if( _pProtSinkOrig )   
        {
            // _pProt is pointing to the top level filter
            IOInetProtocolSinkStackable* pSinkStackable = NULL;
            hr = _pProt->QueryInterface(
                IID_IOInetProtocolSinkStackable, (void**)&pSinkStackable);
            if( hr == NOERROR )
            {
                pSinkStackable->SwitchSink(pProtSink);
                pSinkStackable->Release();
            }
        }
        else
        {
            //
            // there is no filter existing at this moment
            // we need toremember the _pProtSinkOrig, which should
            // always be the LAST sink on the chain 
            //
            _pProtSinkOrig = _pProtSink;

            //
            // switch the existing sink to the filter's sink
            // the filter's sink will be set via StackFilter 
            // method later
            //
            _pProtSink = pProtSink;
        }
        return hr;
    }

    STDMETHODIMP CommitSwitch()
    {
        return NOERROR;
    }

    STDMETHODIMP RollbackSwitch()
    {
        return NOERROR;
    }

    ULONG FilterStacked()
    {
        return  _cbStacked;
    } 

private:
    IInternetProtocolSink*  _pProtSink;     // Filter's sink 
    IInternetProtocol*      _pProt;         // Filter's Prot 

    IInternetProtocolSink*  _pProtSinkOrig; // Filter's original sink 
    IInternetProtocol*      _pProtImp;      // Filter's original Prot 

    ULONG                   _cbStacked;     // number of filter stacked
};

// global variables
extern char vszHttp[];
extern char vszFtp[];
extern char vszGopher[];
extern char vszFile[];
extern char vszLocal[];
extern char vszHttps[];
extern char vszLocationTag[];
extern LPSTR ppszAcceptTypes[];
extern char vszStream[];

BOOL AppendToString(LPSTR* pszDest, LPDWORD pcbDest, 
                    LPDWORD pcbAlloced, LPSTR szSrc, DWORD cbSrc);

//LPCSTR GetUserAgentString();

#define HTTP_STATUS_BEGIN   HTTP_STATUS_OK
#define HTTP_STATUS_END     HTTP_STATUS_VERSION_NOT_SUP



#endif // _CNET_HXX_
 

