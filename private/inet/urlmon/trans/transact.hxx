//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       Transact.hxx
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

#ifndef _TRANSACT_HXX_
#define _TRANSACT_HXX_

#include <urlmon.hxx>
#include "cbinding.hxx"


#ifdef DBG
#define AssertDo(a)  (a)
#else   // !DEBUG
#define ASSERTDATA
#define AssertDo(a) (a)
#define WITH_EXCEPTION
#endif  // DBG

#ifdef unix
#undef offsetof
#endif /* unix */
#define offsetof(s,m) ( (SIZE_T) &(((s *)0)->m) )
#define GETPPARENT(pmemb, struc, membname) (\
                (struc*)(((char*)(pmemb))-offsetof(struc, membname)))

//#define BREAK_ONERROR(hrBreak) if (FAILED(hrBreak)) { break; }

#define DATASNIFSIZE_MIN        256
#define DATASNIFSIZEDOCFILE_MIN 2048
#define DATASNIFSIZEDOCFILE_MAX 4096


class CTransactionMgr;
class CTransPacket;
class CTransaction;
class CTransData;
#define PI_FILTER_MODE_DATA         0x00000100

class CTransactionMgr : public CLifePtr
{
public:
        CTransactionMgr();
        virtual ~CTransactionMgr();

        HRESULT AddTransaction(CTransaction *pCTrans);
        HRESULT RemoveTransaction(CTransaction *pCTrans);

private:
        CTransaction    *_pCTransFirst;           // First in the linked list
        CTransaction    *_pCTransLast;            // First in the linked list
};

// These two structures are used to pass data from the inloader callbacks
// to the wndproc of the hidden window in the main thread.

class CTransPacket : public PROTOCOLDATA
{
public:

    CTransPacket(PROTOCOLDATA *pSI);
    CTransPacket(BINDSTATUS NMsg, HRESULT hrRet = NOERROR, LPCWSTR szStr = NULL, DWORD cbAvailable = 0, DWORD cbTotal = 0, DWORD dwResult = 0);

    ~CTransPacket();

    LPWSTR DupWStr(LPSTR szStr)
    {
        DWORD dwLen = strlen(szStr) + 1;
        LPWSTR wzStr = new WCHAR [dwLen];
        if (wzStr)
        {
            A2W(szStr, wzStr,dwLen);
        }
        return wzStr;
    }

    CTransPacket *GetNext()
        {   return _pCTPNext; }

    void SetNext(CTransPacket *pCTP)
        {   _pCTPNext = pCTP; }
    BINDSTATUS GetNotMsg()
        {   return _NotMsg; }
    BOOL IsLastNotMsg()
        {   return (_NotMsg == BINDSTATUS_ENDDOWNLOADDATA || _NotMsg == BINDSTATUS_ERROR); }
    BOOL IsAsyncNotMsg()
        {   return (_NotMsg == BINDSTATUS_INTERNALASYNC); }


//private:
    DWORD   _dwCurrentSize;
    DWORD   _dwTotalSize;
    DWORD   _dwResult;
    LPWSTR  _pwzStr;
    HRESULT _hrResult;

private:
    BINDSTATUS    _NotMsg;
    CTransPacket *_pCTPNext;    //the next packet
};

typedef enum
{
     TransSt_None               = 0
    ,TransSt_Initialized        = 1
    ,TransSt_OperationStarted   = 2
    ,TransSt_OperationFinished  = 3
    ,TransSt_Aborted            = 4
} TransactionState;

typedef enum
{
     TransData_None               = 0
    ,TransData_Initialized        = 1
    ,TransData_ReadingStarted     = 2
    ,TransData_ReadingFinished    = 3
    ,TransData_ProtoTerminated    = 4
} TransDataState;


typedef enum
{
     DataSink_Unknown               = 0
    ,DataSink_StreamNoCopyData      = 1
    ,DataSink_File
    ,DataSink_Storage
    ,DataSink_StreamOnFile
    ,DataSink_StreamBindToObject
    ,DataSink_GenericStream
} DataSink;

typedef enum
{
     TS_None      = 0
    ,TS_Prepared  = 1
    ,TS_Completed = 2
   
} ThreadSwitchState;

#include "prothndl.hxx"

class CTransaction : public IOInetProtocolSink
                    ,public IOInetBindInfo
                    ,public IServiceProvider
                    ,public IAuthenticate
                    ,public IOInetProtocol
                    ,public IOInetPriority
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //
    // IOInetProtocolSink
    //
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

    // IAuthenticate methods
    STDMETHODIMP Authenticate(
        HWND* phwnd,
        LPWSTR *pszUsername,
        LPWSTR *pszPassword);

    // IOInetBindInfo methods
    STDMETHODIMP GetBindInfo(
        DWORD *grfBINDF,
        BINDINFO * pbindinfo);

    STDMETHODIMP GetBindString(
        ULONG ulStringType,
        LPOLESTR *ppwzStr,
        ULONG cEl,
        ULONG *pcElFetched
        );

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID rsid, REFIID iid, void ** ppvObj);
    
    //
    // IOInetProtocol
    //
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

    STDMETHODIMP Read(
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
    // end IOInetProtocol

    // OInetPriority
    STDMETHODIMP SetPriority(LONG nPriority);

    STDMETHODIMP GetPriority(LONG * pnPriority);


    static HRESULT Create(LPBC pBC, DWORD grfFlags, IUnknown *pUnkOuter, IUnknown **ppUnk, CTransaction **ppCTrans);

    LPWSTR GetUrl()
    {
        UrlMkAssert((_pwzUrl != NULL));
        return _pwzUrl;
    }

    LPWSTR  GetRedirectUrl()
    {
        return _pwzRedirectUrl;
    }

    LPWSTR  SetRedirectUrl(LPWSTR pwzStr)
    {
        TransAssert((_pProt));

        if (_pwzRedirectUrl)
        {
            delete _pwzRedirectUrl;
        }

        _pwzRedirectUrl = OLESTRDuplicate(pwzStr);

        return _pwzRedirectUrl;
    }
    

    REFCLSID GetProtocolClassID()
    {
        if (_pProt)
        {
            TransAssert((_clsidProtocol != CLSID_NULL ));

            return (REFCLSID)_clsidProtocol;
        }
        else
        {
            return CLSID_NULL;
        }
    }


    HRESULT QueryOption(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf);
    HRESULT QueryInfo(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved);

    //
    // manage clients interfaces
    //
    HRESULT AddClientOInet(IOInetProtocolSink *pOInetProtSink, IOInetBindInfo *pOInetBindInfo);
    HRESULT RemoveClientOInet();
   
    HRESULT RestartOperation(LPWSTR pwzURL, DWORD dwCase);
    HRESULT Redirect(LPWSTR pwzUrl);
    HRESULT CompleteOperation(BOOL fNested = FALSE);
    HRESULT MyPeekMessage(MSG *pMsg, HWND hwnd, UINT min, UINT max, WORD wFlag);

    // These two methods are used to implement the linked list of
    // internet transactions that is owned by the internet transaction list
    // object.
    CTransaction *GetNextTransaction()
        { return _pCTransNext; }
    void SetNextTransaction(CTransaction *pCTransNext)
        { _pCTransNext = pCTransNext; }

    STDMETHOD_(HWND, GetNotificationWnd)()
        {
            if (!_hwndNotify)
            {
                _hwndNotify = GetThreadNotificationWnd();
            }
            return _hwndNotify;
        }

    void SetNext(CTransaction *pCTransNext)
        { _pCTransNext = pCTransNext; }

    void SetBinding(CBinding *pCBdg)
        { _pClntProtSink = pCBdg; }


    HRESULT PrepareThreadTransfer();
    HRESULT ThreadTransfer();


    TransactionState GetState()
        {   return _State;  }

    TransactionState SetState(TransactionState newTS )
        {
            TransactionState  tsTemp = _State;
            _State = newTS;
            return tsTemp;
        }

    OperationState GetOperationState()
        {   return _OperationState;  }

    OperationState SetOperationState(OperationState newTS)
        {
            OperationState  tsTemp = _OperationState;
            _OperationState = newTS;
            return tsTemp;
        }

    ULONG BdgAddRef()   {   return _cBdgRefs++; }
    ULONG BdgRelease()  {   return --_cBdgRefs; }
    
    STDMETHOD (OnINetCallback)(BOOL fFromMsgQueue = FALSE);
    STDMETHOD (OnINetInternalCallback)(CTransPacket *pCTPIn);
    STDMETHOD (PreDispatch)();
    STDMETHOD (PostDispatch)();

    void AddCTransPacket(CTransPacket *pCTP, BOOL fTail = TRUE);
    CTransPacket *GetNextCTransPacket();
    CTransPacket *GetCurrentCTransPacket()
    {
        return _pCTPCur;
    }

    BOOL GotCTransPacket();
    STDMETHODIMP DispatchPacket(CTransPacket *pCTPIn);
    STDMETHODIMP DispatchReport(BINDSTATUS NotMsg, DWORD grfBSCF, DWORD dwCurrentSize, DWORD dwTotalSize, LPCWSTR pwzStr = 0, DWORD dwError = 0, HRESULT hresult = NOERROR);
    STDMETHODIMP OnDataReceived(DWORD *pgrfBSC, DWORD *pcbBytesAvailable, DWORD *pdwTotalSize);
    STDMETHODIMP OnAttach(LPCWSTR pwzURL, IOInetBindInfo *pOInetBindInfo, IOInetProtocolSink *pOInetBindSink, DWORD grfOptions, DWORD_PTR dwReserved);
    STDMETHODIMP LoadHandler(LPCWSTR pwzURL, COInetProt *pCProt, DWORD dwMode);

    HRESULT GetHResult()
        {
            return _hrResult;
        }
    BINDINFO *GetTransBindInfo();
    DWORD GetTransBindFlags()
        {   return _grfBINDF;   }

    void SetTransBindFlags(DWORD dwF)
        {   _grfBINDF = dwF;    }

    BOOL IsFreeThreaded()
    {
        return !(_dwOInetBdgFlags & OIBDG_APARTMENTTHREADED);
    }

    BOOL IsApartmentThread()
    {
        TransAssert((_dwThreadId != 0));
        return (_dwThreadId == GetCurrentThreadId());
    }

    DWORD GetBSCF()
    {
        return _grfBSCF;
    }

    VOID UpdateVerifiedMimeType(LPCWSTR pwzMime);


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
    
    //
    // the pUnkInner of the aggregated APP (protocol)
    IUnknown        *_pUnkInner;     // the inner object if the protocol supports aggregation
    
    IUnknown *GetIUnkInner(BOOL fReleaseOuter = FALSE)
    {
        if (fReleaseOuter)
        {
            TransAssert((_CRefs > 0));

            _CRefs--;
        }
        return &_Unknown;
    }

private:
    CTransaction(DWORD grfFlags, LPBYTE pByte, ULONG cbSizeBuffer, IUnknown *pUnkOuter);
    ~CTransaction();


#if DBG==1
    WORD  GetTotalPostedMsgId()
    {
        return _wTotalPostedMsg;
    }
#else
    #define GetTotalPostedMsgId() 0
#endif


public:

private:
    union
    {
        ULONG _flags;
#ifndef unix
        struct
        {
#endif /* !unix */
            unsigned _fAborted                 : 1;
#ifndef unix
        };
#endif /* !unix */
    };

    CRefCount           _CRefs;          // the total refcount of this object
    // File that the data is being downloaded to
    LPWSTR              _pwzRedirectUrl;
    LPWSTR              _pwzUrl;
    LPWSTR              _pwzProtClsId;
    LONG                _nPriority;
    
    // client pointer
    IOInetProtocolSink *_pClntProtSink;
    IOInetBindInfo     *_pClntBindInfo;

    IBindCtx           *_pBndCtx;
    CTransaction       *_pCTransNext;   // Next transaction in the linked list
    
    IOInetProtocol     *_pProt;

    IWinInetHttpInfo   *_pInetHttpInfo;
    IWinInetInfo       *_pInetInfo;

    CLSID               _clsidProtocol;

    DWORD               _dwPacketsTotal;

    HWND                _hwndNotify;    // notification window for this
    CTransactionMgr    *_pCTransMgr;
    TransactionState    _State;
    OperationState      _OperationState;
    ULONG               _cBdgRefs;
    DWORD               _dwThreadId;
    DWORD               _dwProcessId;
    DWORD               _grfBSCF;

    // data to handle the packets
    CTransPacket       *_pCTPHead;      // frist on
    CTransPacket       *_pCTPTail;      //
    CTransPacket       *_pCTPCur;       // the current one which gets dispatched

    CTransPacket       *_pCTPTransfer;  // used on transfering to another thread
    DWORD               _grfInternalFlags;

    CRefCount           _cPacketsInList;
    CRefCount           _cPostedMsg;
    BOOL                _fDispatch;
    BOOL                _fDispatchData;
    ThreadSwitchState   _ThreadTransferState;
    
    BOOL                _fResultReported;
    BOOL                _fResultDispatched;
    DWORD               _dwDispatchLevel;
    BOOL                _fTerminated;
    BOOL                _fTerminating;
    BOOL                _fReceivedTerminate;
    BOOL                _fCallTerminate;
    BOOL                _fDocFile;
    BOOL                _fMimeVerified;
    BOOL                _fResultReceived;

    BOOL                _fMimeHandlerEnabled;
    BOOL                _fEncodingHandlerEnabled;
    BOOL                _fClsInstallerHandlerEnabled;
    BOOL                _fMimeHandlerLoaded;
    BOOL                _fAttached;
    BOOL                _fLocked;
    BOOL                _fModalLoopRunning;
    BOOL                _fForceAsyncReportResult;

    // special (delayed) abort handling
    BOOL                _fStarting;
    BOOL                _fReceivedAbort;
    HRESULT             _hrAbort;
    DWORD               _dwAbort;
    DWORD               _dwTerminateOptions;

    HRESULT             _hrResult;
    DWORD               _dwResult;
    LPWSTR              _pwzResult;
    LPWSTR              _pwzFileName;
    LPWSTR              _pwzMimeSuggested;

    CMutexSem           _mxs;           // used in Read, Seek, Abort and package list in case of apartment threaded
    CMutexSem           _mxsBind;       // used in method Bind

    BINDINFO           *_pBndInfo;
    DWORD               _grfBINDF;
    DWORD               _dwOInetBdgFlags;

    ULONG               _ulCurrentSize;
    ULONG               _ulTotalSize;

    LPBYTE              _pBuffer;           // DNLD_BUFFER_SIZE  size buffer
    ULONG               _cbBufferSize;
    ULONG               _cbTotalBytesRead;
    ULONG               _cbBufferFilled;    //how much of the buffer is in use

    //
    ULONG               _cbDataSize;
    ULONG               _cbReadReturn;
    ULONG               _cbDataSniffMin;
    ULONG               _cbBytesReported;   // how much was reported
    //

    LARGE_INTEGER       _dlibReadPos;

    COInetProt          _CProtEmbed;        // to embed
    COInetProt          _CProtEncoding;     // encoding 
    COInetProt          _CProtClsInstaller; // encoding 
    COInetProt          _COInetProtPost;    // for mime verification on worker thread
    BOOL                _fProtEmbed;

#if DBG==1
    WORD                _wTotalPostedMsg;
#endif
};

class CTransData : public ITransactionData
{
private:
    CTransData(CTransaction *pTrans, LPBYTE pByte, DWORD dwSizeBuffer, BOOL fBindToObject);
    ~CTransData();

public:
    STDMETHODIMP SetFileName(LPWSTR szFile);
    LPWSTR GetFileName();


    STDMETHODIMP GetData(FORMATETC **ppformatetc, STGMEDIUM **ppmedium, DWORD grfBSCF);
    STDMETHODIMP FindFormatETC();
    STDMETHODIMP SetClipFormat(CLIPFORMAT  cfFormat);

    STDMETHODIMP ReadHere(LPBYTE pBuffer, DWORD cbBytes, DWORD *dwRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);

    STDMETHODIMP OnDataReceived(DWORD grfBSC, DWORD cbBytesRead, DWORD dwTotalSize, DWORD *pcbNewAvailable);
    STDMETHODIMP OnDataInBuffer(BYTE *pBuffer, DWORD cbBytesRead, DWORD dwBytesTotal);
    STDMETHODIMP GetReadBuffer(BYTE **ppBuffer, DWORD *pcbBytes);

    STDMETHODIMP OnStart(IOInetProtocol *pCINet);
    STDMETHODIMP OnTerminate();

    STDMETHODIMP IsObjectReady();
    STDMETHODIMP InProgress();

    STDMETHODIMP_(BOOL) IsRemoteObjectReady()
    {
        return _fRemoteReady;
    }

    STDMETHODIMP GetClassID(CLSID clsidIn, CLSID *pclsid);

    STDMETHODIMP SetMimeType(LPCWSTR pszMine);
    LPCWSTR GetMimeType();

    STDMETHODIMP GetAcceptStr(LPWSTR *ppwzStr, ULONG *pcElements);
    STDMETHODIMP GetAcceptMimes(LPWSTR *ppStr, ULONG cel, ULONG *pcElements);

    STDMETHODIMP PrepareThreadTransfer();

    DataSink GetDataSink();
    DataSink SetDataSink(DWORD dwBindF);
    DataSink SwitchDataSink(DataSink dsNew);

    void OnEndofData();

    BOOL IsFileRequired();
    BOOL IsFromCache()
    {
        return _fCache;
    }
    void  SetFromCacheFlag(BOOL fCache)
    {
        _fCache = fCache;
    }

    void ResetCINet(IOInetProtocol *pCINet)
    {
        if (_pProt)
        {
            _pProt->Release();
        }

        _pProt = pCINet;

        if (_pProt)
        {
            _pProt->AddRef();
        }
    }

    ULONG GetDataSize()
    {
        return _cbDataSize;
    }

    DWORD GetBindFlags()
    {
        return _grfBindF;
    }
    LPWSTR GetUrl()
    {
        UrlMkAssert((FALSE));
        return NULL;
    }

    VOID SetFileAsStmFile()
    {
        _fFileAsStmOnFile = TRUE; 
    }

    BOOL IsEOFOnSwitchSink()
    {
        return _fEOFOnSwitchSink;
    }


public:

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    // ITransactionData methods
    STDMETHODIMP GetTransactionData(LPCWSTR pwzUrl, LPOLESTR *ppwzFilename, LPOLESTR *ppwzMime,
                                    DWORD *pdwSizeTotal, DWORD *pdwSizeAvailable, DWORD  dwReserved);

    static HRESULT Create(LPCWSTR pwzUrl, DWORD grfBindF, REFIID riid,  IBindCtx *pBndCtx, BOOL fBindToObject, CTransData **ppCTD);
    STDMETHODIMP Initialize(LPCWSTR pwzUrl, DWORD grfBindF, REFIID riid, IBindCtx *pBndCtx);

private:
    CRefCount       _CRefs;    // refcount class
    TransDataState  _TransDataState;
    IOInetProtocol  *_pProt;

    STGMEDIUM      *_pStgMed;
    FORMATETC       _formatetc;     // The format that we are sending
    CLIPFORMAT      _cfFormat;
    IEnumFORMATETC *_pEnumFE;
    IBindCtx       *_pBndCtx;
    IID            *_piidRes;
    DWORD           _grfMode;
    DWORD           _dwAttached;
    DWORD           _grfBindF;
    DWORD           _grfBSC;
    LPWSTR          _pwzUrl;

    unsigned        _fBindToObject      : 1;
    unsigned        _fMimeTypeVerified  : 1;
    unsigned        _fDocFile           : 1;
    unsigned        _fInitialized       : 1;
    unsigned        _fRemoteReady       : 1;
    unsigned        _fCache             : 1;
    unsigned        _fLocked            : 1;
    unsigned        _fFileAsStmOnFile   : 1;
    unsigned        _fEOFOnSwitchSink   : 1;

    LPBYTE          _lpBuffer;          // DNLD_BUFFER_SIZE  size buffer
    ULONG           _cbBufferSize;
    ULONG           _cbDataSize;
    ULONG           _cbTotalBytesRead;
    ULONG           _cbReadReturn;
    ULONG           _cbBufferFilled;    //how much of the buffer is in use
    ULONG           _cbDataSniffMin;
    ULONG           _cbBytesReported;   // how much was reported
    DataSink        _ds;

    HANDLE          _hFile;
    WCHAR           _wzMime[SZMIMESIZE_MAX];
    WCHAR           _wzFileName[MAX_PATH];

};


//+---------------------------------------------------------------------------
//
//  Class:      CModalLoop ()
//
//  Purpose:
//
//  Interface:  CModalLoop --
//              ~CModalLoop --
//              QueryInterface --
//              AddRef --
//              Release --
//              HandleInComingCall --
//              RetryRejectedCall --
//              MessagePending --
//              _CRefs --
//              _pMsgFlter --
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CModalLoop : public IMessageFilter
{
public:
    CModalLoop(HRESULT *phr);
    ~CModalLoop();


    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    STDMETHOD_(DWORD, HandleInComingCall)(
        DWORD dwCallType,HTASK htaskCaller,
        DWORD dwTickCount,LPINTERFACEINFO lpInterfaceInfo);

    STDMETHOD_(DWORD, RetryRejectedCall)(
        HTASK htaskCallee,DWORD dwTickCount,
        DWORD dwRejectType);

    STDMETHOD_(DWORD, MessagePending)(
        HTASK htaskCallee,
        DWORD dwTickCount,
        DWORD dwPendingType);

    HRESULT HandlePendingMessage(
        DWORD  dwPendingType,
        DWORD  dwPendingRecursion,
        DWORD  dwReserved);

private:
    CRefCount               _CRefs;         // refcount class
    IMessageFilter          *_pMsgFlter;
};


CTransactionMgr * GetThreadTransactionMgr(BOOL fCreate = TRUE);
#define ChkHResult(hr) { if (FAILED(hr)) { UrlMkAssert((FALSE)); goto End; } }

extern char     szContent[];
extern char     szClassID[];
extern char     szFlags[];
extern char     szClass[];
extern char     szExtension[];
extern const GUID CLSID_PluginHost;

// private flags for bad header
#define MIMESNIFF_BADHEADER 0x00010000

HRESULT GetMimeFileExtension(LPSTR pszMime, LPSTR pszExt, DWORD cbSize);
HRESULT GetClassFromExt(LPSTR pszExt, CLSID *pclsid);
HRESULT IsDocFile(LPVOID pBuffer, DWORD cbSize);

HRESULT GetPlugInClsID(LPSTR pszExt, LPSTR pszName, LPSTR pszMime, CLSID *pclsid);
HRESULT FindMediaTypeFormat(LPCWSTR pwzType, CLIPFORMAT *cfType, DWORD *pdwFormat);
HRESULT GetMimeFlags(LPCWSTR pwzMime, DWORD *pdwFlags);

#endif //_TRANSACT_HXX_


 

