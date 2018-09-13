//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cbinding.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _CBINDING_HXX_
#define _CBINDING_HXX_

#define FLAG_BTO_STR_LENGTH           6
#define FLAG_BTO_STR_TRUE             L"TRUE"
#define FLAG_BTO_STR_FALSE            L"FALSE"
#define MAX_DWORD_DIGITS              20       

typedef enum
{
     OPS_Initialized        // operation did not start yet
    ,OPS_StartBinding       // downloading in progress
    ,OPS_GetBindInfo        // downloading in progress
    ,OPS_Downloading        // downloading in progress
    ,OPS_ObjectAvailable    //
    ,OPS_Suspend            // operation suspend
    ,OPS_Abort              // operation abort
    ,OPS_Stopped            // operation is done - stop was called
    ,OPS_Succeeded          // operation is done
    ,OPS_INetError          // operation stopped due wininet error

} OperationState;

class CTransactionMgr;
class CTransPacket;
class CTransaction;
class CTransData;
class CBindCtx;

class CBinding : public IBinding
               , public IWinInetHttpInfo
               , public IOInetProtocolSink
               , public IOInetBindInfo
               , public IServiceProvider
{
public:
    CBinding(IUnknown *pUnk);
    ~CBinding();

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IBinding methods
    STDMETHOD(Abort)( void);
    STDMETHOD(Suspend)( void);
    STDMETHOD(Resume)( void);
    STDMETHOD(SetPriority)(LONG nPriority);
    STDMETHOD(GetPriority)(LONG *pnPriority);
    STDMETHOD(GetBindResult)(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved);

    // IWinInetInfo methods
    STDMETHOD(QueryOption)(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf);

    // IWinInetHttpInfo methods
    STDMETHOD(QueryInfo)(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved);

    // *** IServiceProvider ***
    STDMETHOD (QueryService)(REFGUID rsid, REFIID iid, void ** ppvObj);


    //IOInetBindInfo methods
    STDMETHODIMP GetBindInfo(
        DWORD *grfBINDF,
        BINDINFO * pbindinfo);

    STDMETHODIMP GetBindString(
        ULONG ulStringType,
        LPOLESTR *ppwzStr,
        ULONG cEl,
        ULONG *pcElFetched
        );

    STDMETHODIMP Switch(
        PROTOCOLDATA *pStateInfo);

    STDMETHODIMP ReportProgress(
        ULONG ulStatusCode,
        LPCWSTR szStatusText);

    STDMETHODIMP ReportData(
        DWORD grfBSCF,
        ULONG ulProgress,
        ULONG ulProgressMax);

    STDMETHODIMP ReportResult(
        HRESULT hrResult,
        DWORD   dwError,
        LPCWSTR wzResult);
        

// private: // methods
    static HRESULT Create(IUnknown *pUnkOuter, LPCWSTR szUrl, LPBC pbc,
                            REFIID riid, BOOL fBindToObject, CBinding **ppCBdg);
    STDMETHOD(Initialize)(LPCWSTR pUrl, IBindCtx *pbc, DWORD grfBindF, REFIID riid, BOOL fBindToObject);
    LPWSTR GetUrl()
        {
            return _lpwszUrl;
        }
    STDMETHOD_(HWND, GetNotificationWnd)()
        { return _hwndNotify; }
    STDMETHOD_(BOOL, IsAsyncBinding)()
        { return (_pBSCB!=NULL); }

    HRESULT StartBinding(LPCWSTR szUrl, IBindCtx *pbc, REFIID riid, BOOL fBindToObject, LPWSTR *ppwzExtra, LPVOID *ppv);

    STDMETHOD(CompleteTransaction)();
    STDMETHOD(GetRequestedObject)(IBindCtx *pbc, IUnknown **ppUnk);

    STDMETHODIMP_(BOOL) OnTransNotification(BINDSTATUS NotMsg, DWORD dwCurrrentSize, DWORD dwTotalSize, LPWSTR pwzStr, HRESULT hrINet);
    STDMETHOD(OnDataNotification)(DWORD grfBSCF, DWORD dwCurrrentSize, DWORD dwTotalSize, BOOL fLastNotification);
    STDMETHOD(OnObjectAvailable) (DWORD grfBSCF, DWORD dwCurrrentSize, DWORD dwTotalSize, BOOL fLastNotification);

    STDMETHOD(InstallIEFeature)();

    STDMETHOD(InstantiateObject)(CLSID *pclsid, REFIID riidResult, IUnknown **ppUnk, BOOL fFullyAvailable);
    STDMETHOD(ObjectPersistMnkLoad)(IUnknown *pUnk, BOOL fLocal, BOOL fFullyAvailable);
    STDMETHOD(ObjectPersistFileLoad)(IUnknown *pUnk);
    STDMETHOD(CreateObject)(CLSID *pclsid, REFIID riidResult, IUnknown **ppUnk);

    LPWSTR GetFileName();
    BOOL IsAsyncTransaction()
    {
        return (   IsAsyncBinding()
                && (_grfBINDF & BINDF_ASYNCHRONOUS) );
    }
    BOOL IsSyncTransaction()
    {
        return !IsAsyncTransaction();
    }

    IMoniker *GetMoniker()
    {
        return _pMnk;
    }
    void SetMoniker(IMoniker *pMnk)
    {
        if (_pMnk)
        {
             _pMnk->Release();
        }
        _pMnk = pMnk;
        if (_pMnk)
        {
            _pMnk->AddRef();
        }
    }

    IOInetProtocol *GetOInetBinding()
    {
        if (_pOInetBdg)
        {
            return _pOInetBdg;
        }
        else
        {
            return 0;
        }
    }

    REFCLSID GetProtocolClassID()
    {
        TransAssert((_clsidProtocol != CLSID_NULL ));
        return (REFCLSID)_clsidProtocol;
    }

    OperationState GetOperationState()
        {   return _OperationState;  }

    OperationState SetOperationState(OperationState newTS)
        {
            OperationState  tsTemp = _OperationState;
            _OperationState = newTS;
            return tsTemp;
        }

    void SetInstantiateHresult(HRESULT hr)
    {
        _hrInstantiate = hr;
    }

    HRESULT GetInstantiateHresult()
    {
        return _hrInstantiate;
    }

    HRESULT GetHResult()
    {
        return _hrResult;
    }

    STDMETHOD(CallOnStartBinding)(DWORD grfBINDINFOF, IBinding * pib);
    STDMETHOD(CallGetBindInfo)  (DWORD *grfBINDINFOF, BINDINFO *pbindinfo);
    STDMETHOD(CallOnProgress)   (ULONG ulProgress, ULONG ulProgressMax,
                ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(CallOnStopBinding)(HRESULT hrRet, LPCWSTR szStatusText);
    STDMETHOD(CallOnLowResource) (DWORD reserved);
    STDMETHOD(CallGetPriority)  (LONG * pnPriority);

    STDMETHOD(CallOnDataAvailable)(DWORD grfBSC,DWORD dwSize,FORMATETC *pformatetc,STGMEDIUM *pstgmed);
    STDMETHOD(CallOnObjectAvailable)(REFIID riid,IUnknown *punk);

    STDMETHOD(CallAuthenticate)(HWND* phwnd, LPWSTR *pszUsername,LPWSTR *pszPassword);
    STDMETHOD(LocalQueryInterface)(REFIID iid, void **ppvObj);

    DWORD GetTransBindFlags()
    {
        return _grfBINDF;
    }

    BOOL IsRequestedIIDValid(REFIID riid)
    {
        if(    (riid != IID_IStream)
            && (riid != IID_IStorage)
            && (riid != IID_IUnknown) )
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }

private:
    CRefCount               _CRefs;         // refcount class
    DWORD                   _dwThreadId;    // the thread id of this binding
    IUnknown                *_pUnk;         // controlling unknown
    IBindStatusCallback     *_pBSCB;
    IAuthenticate           *_pBasicAuth;   // Pointer to IAuthenticate holder
    IInternetBindInfo       *_pBindInfo;   //

    LONG                _nPriority;     // priority of this binding
    DWORD               _dwState;       // state of operation
    OperationState      _OperationState;

    HWND                _hwndNotify;    // Status window
    DWORD               _grfBINDF;      // Bind flags
    DWORD               _dwLastSize;    // Size at last notification

    LPWSTR              _lpwszUrl;      // Url for which download is proceeding
    LPWSTR              _pwzRedirectUrl;      // Url for which download is proceeding

    CLSID               _clsidIn;       // class found be oinet

    BOOL                _fBindToObject;
    BOOL                _fSentLastNotification;
    BOOL                _fSentFirstNotification;
    BOOL                _fCreateStgMed;
    BOOL                _fLocal;
    BOOL                _fCompleteDownloadHere;
    BOOL                _fForceBindToObjFail;
    BOOL                _fAcceptRanges;
    BOOL                _fClsidFromProt;
    DWORD               _grfInternalFlags;
    BIND_OPTS           _bindopts;
    HRESULT             _hrBindResult;
    HRESULT             _hrInstantiate;
    HRESULT             _hrResult;
    DWORD               _dwBindError;
    LPWSTR              _pwzResult;
    LPWSTR              _wszDllName;

    CTransData         *_pCTransData;
    IOInetProtocol     *_pOInetBdg;
    
    IMoniker           *_pMnk;
    CBindCtx           *_pBndCtx;
    IID                *_piidRes;
    IUnknown           *_pUnkObject;

    CLSID               _clsidProtocol;
    BINDINFO            _BndInfo;
    IWinInetInfo       *_pIWinInetInfo;
    IWinInetHttpInfo   *_pIWinInetHttpInfo;
};

//
// this flags are used to during BindToObject - BindToStorage scenarios
//
typedef enum
{
     BDGFLAGS_ATTACHED      = 0x01000000
    ,BDGFLAGS_PARTIAL       = 0x02000000
    ,BDGFLAGS_LOCALSERVER   = 0x04000000
    ,BDGFLAGS_NOTIFICATIONS = 0x08000000
} BINDINGFLAGS;


class CBindProtocol : public IBindProtocol
{
public:
    CBindProtocol();
    ~CBindProtocol();

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);
    STDMETHOD(CreateBinding)(LPCWSTR url, IBindCtx *pBCtx, IBinding **ppBdg);

private:
    CRefCount   _CRefs;         // refcount class
    IUnknown   *_pUnk;          // controlling unknown

};
typedef enum
{
     Medium_Stream  = 1
    ,Medium_Storage = 2
    ,Medium_Unknown = 3
} Medium;

class CBSC : public IBindStatusCallback
{
public:

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IBindStatusCallback methods ***
    STDMETHOD(OnStartBinding) (DWORD grfBINDINFOF, IBinding * pib);
    STDMETHOD(OnLowResource) (DWORD reserved);
    STDMETHOD(OnProgress) (ULONG ulProgress, ULONG ulProgressMax,
                ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding) (HRESULT hresult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD *grfBINDINFOF, BINDINFO *pbindinfo);
    STDMETHOD(OnDataAvailable)(DWORD grfBSC,DWORD dwSize,FORMATETC *pformatetc,STGMEDIUM *pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid,IUnknown *punk);
    STDMETHOD(GetPriority)(LONG * pnPriority);


public:
    CBSC(Medium Medium);
    CBSC(BOOL fBindToObject);
    ~CBSC();

    IStream *  GetStream()
    {
        return _pStm;
    }

    //IStorage *  GetStorage();
    HRESULT GetRequestedObject(IBindCtx *pbc, void **ppvObj);

private:
    UINT                        _cRef;
    IBinding *                  _pBdg;
    BOOL                        _fBindToObject;
    BOOL                        _fGotStopBinding;
    IStream *                   _pStm;
    IStorage *                  _pStg;
    IUnknown *                  _pUnk;
    Medium                      _Medium;
    HRESULT                     _hrResult;
    WCHAR                       _wzPath[MAX_PATH];
};

HRESULT GetObjectParam(IBindCtx *pbc, LPOLESTR pszKey, REFIID riid, IUnknown **ppUnk);
HRESULT CreateURLBinding(LPWSTR lpszUrl, IBindCtx *pbc, IBinding **ppBdg);
BOOL IsKnownBindProtocolOld(LPCWSTR lpszProto);
DWORD IsKnownProtocol(LPCWSTR wzProtocol);

#endif //_CBINDING_HXX_
