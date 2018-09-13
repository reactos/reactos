//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dwn.hxx
//
//  Contents:   
//
//-------------------------------------------------------------------------

#ifndef I_DWN_HXX_
#define I_DWN_HXX_
#pragma INCMSG("--- Beg 'dwn.hxx'")

#if !defined(_DOWNLOAD_DIRECTORY_)
#error "dwn.hxx is private to the download directory"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#if defined(PRODUCT_PROF) && !defined(_MAC)
extern "C" void _stdcall StartCAP(void);
extern "C" void _stdcall StopCAP(void);
extern "C" void _stdcall SuspendCAP(void);
extern "C" void _stdcall ResumeCAP(void);
#else
#define StartCAP()
#define StopCAP()
#define SuspendCAP()
#define ResumeCAP()
#endif

// Forward --------------------------------------------------------------------

class CDwnStm;
class CDwnInfo;
class CDwnLoad;
class CDwnTaskExec;
class CDwnTask;
class CDwnBindData;

MtExtern(Dwn)
MtExtern(CDwnStm)
MtExtern(CDwnTaskExec)
MtExtern(CDwnTask)
MtExtern(CDwnBindData)
MtExtern(CDwnInfo_arySink_pv)

// Definitions ----------------------------------------------------------------

enum {
    DBF_PROGRESS    = 0x0001,   // Progress is available
    DBF_REDIRECT    = 0x0002,   // Binding was redirected to a different URL
    DBF_RESPONSE    = 0x0004,   // Response code is available
    DBF_HEADERS     = 0x0008,   // HTTP/FILE information is available
    DBF_MIME        = 0x0010,   // Mime type is available
    DBF_DATA        = 0x0020,   // Data is available for reading
    DBF_DONE        = 0x0040    // Binding is complete (perhaps aborted)
};

// Functions ------------------------------------------------------------------

HRESULT     NewDwnBindData(DWNLOADINFO * pdli, CDwnBindData ** ppDwnBindData, DWORD dwFlagsExtra);
HRESULT     StartDwnTask(CDwnTask * pDwnTask);
void        KillDwnTaskExec();
void        DwnCacheDeinit();
void        AnsiToWideTrivial(const CHAR * pchA, WCHAR * pchW, LONG cch);
MIMEINFO *  GetMimeInfoFromData(void * pb, ULONG cb, const TCHAR *pchProposed);
HRESULT     CreateStreamOnDwnStm(CDwnStm * pDwnStm, IStream ** ppStream);
BOOL        GetFileLastModTime(TCHAR * pchFile, FILETIME * pftLastMod);
BOOL        GetUrlLastModTime(TCHAR * pchUrl, UINT uScheme, DWORD dwBindf, FILETIME * pftLastMod);

// Types ----------------------------------------------------------------------

struct DWNPROG
{
    DWORD       dwMax;
    DWORD       dwPos;
    DWORD       dwStatus;
};

// Critical Sections ----------------------------------------------------------

class CDwnCrit : public CRITICAL_SECTION
{

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:
                    
                        CDwnCrit();
                       ~CDwnCrit();
    CRITICAL_SECTION *  GetPcs()    { return(this); }

    #if DBG==1
                        CDwnCrit(LPCSTR pszName, UINT csType);
    void                Enter();
    void                Leave();
    BOOL                IsEntered() { return(_dwThread == GetCurrentThreadId()); }

private:

    LPCSTR              _pszName;
    UINT                _cLevel;
    DWORD               _dwThread;
    ULONG               _cEnter;
    CDwnCrit *          _pDwnCritNext;

    #else

    void                Enter()     { ::EnterCriticalSection(GetPcs()); }
    void                Leave()     { ::LeaveCriticalSection(GetPcs()); }

    #endif
};

#define EXTERN_CRITICAL(x)      extern CDwnCrit x

#if DBG==1
#define DEFINE_CRITICAL(x, y)   CDwnCrit x(#x, y)
#else
#define DEFINE_CRITICAL(x, y)   CDwnCrit x
#endif

EXTERN_CRITICAL(g_csDwnBindSig);
EXTERN_CRITICAL(g_csDwnBindTerm);
EXTERN_CRITICAL(g_csDwnBindPend);
EXTERN_CRITICAL(g_csDwnCache);
EXTERN_CRITICAL(g_csDwnStm);
EXTERN_CRITICAL(g_csHtmSrc);
EXTERN_CRITICAL(g_csDwnTaskExec);
EXTERN_CRITICAL(g_csImgTaskExec);
EXTERN_CRITICAL(g_csImgTransBlt);
EXTERN_CRITICAL(g_csDwnDoc);
EXTERN_CRITICAL(g_csDwnPost);

// CDwnStm --------------------------------------------------------------------

class CDwnStm : public CDwnChan
{

public:

    typedef CDwnChan super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnStm))
    
                    CDwnStm(UINT cbBuf = 8192);
    virtual        ~CDwnStm();

    HRESULT         SetSeekable();

    HRESULT         Write(void * pv, ULONG cb);
    HRESULT         WriteBeg(void ** ppv, ULONG * pcb);
    void            WriteEnd(ULONG cb);
    void            WriteEof(HRESULT hrEof);

    HRESULT         Read(void * pv, ULONG cb, ULONG * pcb);
    HRESULT         ReadBeg(void ** ppv, ULONG * pcb);
    void            ReadEnd(ULONG cb);
    BOOL            ReadEof(HRESULT * phrEof);

    HRESULT         CopyStream(IStream * pstm, ULONG * pcbCopy);
    HRESULT         Seek(ULONG ib);
    ULONG           Size()          { return(_cbWrite); }

    BOOL            IsPending()     { return(_cbRead == _cbWrite && !_fEof); }
    BOOL            IsEof()         { return(_fEof && (_hrEof || (_cbRead == _cbWrite))); }
    BOOL            IsEofWritten()  { return(_fEof); }

#if DBG==1 || defined(PERFTAGS)
    virtual char *  GetOnMethodCallName() { return("CDwnStm::OnMethodCall"); }
#endif

private:

    struct BUF
    {
        BUF *   pbufNext;               // Pointer to next buffer
        ULONG   ib;                     // Index of next byte to write
        ULONG   cb;                     // Count of bytes in the buffer
        BYTE    ab[1];                  // The bytes (size is cb)
    };

    BUF *           _pbufHead;          // First buffer in queue
    BUF *           _pbufTail;          // Last buffer in queue
    BUF *           _pbufWrite;         // Buffer being written (tail or NULL)
    BUF *           _pbufRead;          // Buffer being read
    ULONG           _cbBuf;             // Size of new buffers
    ULONG           _ibRead;            // Offset into pbufRead for reader
    ULONG           _cbRead;            // Count of bytes read
    ULONG           _cbWrite;           // Count of bytes written
    BOOL            _fEof;              // Async flag (don't combine!)
    BOOL            _fSeekable;         // Async flag (don't combine!)
    HRESULT         _hrEof;             // Error to return at EOF

};

// CDwnInfo -------------------------------------------------------------------

class CDwnInfo : public CBaseFT
{
    typedef CBaseFT super;
    friend class CDwnCtx;
    friend class CDwnLoad;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    // CBaseFT methods

                    CDwnInfo();
    virtual        ~CDwnInfo();
    virtual ULONG   Release();
    virtual void    Passivate();

    #if DBG==1
    void            EnterCriticalSection()      { ((CDwnCrit *)GetPcs())->Enter(); }
    void            LeaveCriticalSection()      { ((CDwnCrit *)GetPcs())->Leave(); }
    BOOL            EnteredCriticalSection()    { return(((CDwnCrit *)GetPcs())->IsEntered()); }
    #endif

    // CDwnInfo methods

    static  HRESULT Create(UINT dt, DWNLOADINFO * pdli, CDwnInfo ** ppDwnInfo);
    virtual HRESULT Init(DWNLOADINFO * pdli);
    virtual HRESULT NewDwnCtx(CDwnCtx ** ppDwnCtx) = 0;
    virtual HRESULT GetFile(LPTSTR * ppch);
    virtual void    OnLoadDone(HRESULT hrErr) {}
    virtual void    Abort(HRESULT hrErr, CDwnLoad ** ppDwnLoad);
    virtual void    Reset() {};
    virtual HRESULT NewDwnLoad(CDwnLoad ** ppDwnLoad) = 0;
    virtual ULONG   ComputeCacheSize()                  { return(0); }
    virtual BOOL    AttachEarly(UINT dt, DWORD dwRefresh, DWORD dwFlags, DWORD dwBindf)
                                                        { return(FALSE); }
    virtual BOOL    CanAttachLate(CDwnInfo * pDwnInfo)  { return(FALSE); }
    virtual void    AttachLate(CDwnInfo * pDwnInfo)     { Assert(0); }
    virtual UINT    GetType() = 0;
    virtual DWORD   GetProgSinkClass() = 0;
    
    BOOL            AttachByLastMod(CDwnLoad * pDwnLoad, FILETIME * pft, BOOL fDoAttach);
    BOOL            AttachByLastModEx(BOOL fScanActive, UINT uScheme);
    FILETIME        GetLastMod()                    { return(_ftLastMod); }
    void            SetLastMod(FILETIME ft)         { _ftLastMod = ft; }
    BOOL            IsLastModSet()                  { return(_ftLastMod.dwLowDateTime != 0 || _ftLastMod.dwHighDateTime != 0); }
    void            DelProgSinks();
    void            StartProgress();
    HRESULT         AddProgSink(IProgSink * pProgSink);
    void            DelProgSink(IProgSink * pProgSink);
    HRESULT         SetProgress(DWORD dwFlags, DWORD dwState, LPCTSTR pch, DWORD dwIds, DWORD dwPos, DWORD dwMax);
    void            AddDwnCtx(CDwnCtx * pDwnCtx);
    void            DelDwnCtx(CDwnCtx * pDwnCtx);
    LPCTSTR         GetUrl()                        { return(_cstrUrl); }
    HRESULT         SetUrl(LPCTSTR pch)             { RRETURN(_cstrUrl.Set(pch)); }
    MIMEINFO *      GetMimeInfo()                   { return(_pmi); }
    void            SetMimeInfo(MIMEINFO * pmi)     { _pmi = pmi; }
    void            Signal(WORD wChg);
    void            SetLoad(CDwnCtx * pDwnCtx, BOOL fLoad, BOOL fReload, DWNLOADINFO * pdli);
    void            OnLoadDone(CDwnLoad * pDwnLoad, HRESULT hrErr);
    DWORD           GetBindf()                      { return(_dwBindf); }
    void            SetBindf(DWORD dwBindf)         { _dwBindf = dwBindf; }
    DWORD           GetRefresh()                    { return(_dwRefresh); }
    void            SetRefresh(DWORD dwRefresh)     { _dwRefresh = dwRefresh; }
    DWORD           GetLru()                        { return(GetBindf()); }
    void            SetLru(DWORD dwLru)             { SetBindf(dwLru); }
    DWORD           GetCacheSize()                  { return(GetRefresh()); }
    void            SetCacheSize(DWORD cbSize)      { SetRefresh(cbSize); }
    CDwnInfo *      GetDwnInfoLock()                { return(_pDwnInfoLock); }
    DWORD           GetSecFlags()                   { return(_dwSecFlags); }
    void            SetSecFlags(DWORD dwSecFlags)   { _dwSecFlags = dwSecFlags; }
    BOOL            TstFlags(DWORD dwFlags)         { return(!!(_dwFlags & dwFlags)); }
    DWORD           GetFlags(DWORD dwFlags)         { return(_dwFlags & dwFlags); }
    void            SetFlags(DWORD dwFlags)         { _dwFlags |= dwFlags; }
    void            ClrFlags(DWORD dwFlags)         { _dwFlags &= ~dwFlags; }
    void            UpdFlags(DWORD dwM, DWORD dwF)  { _dwFlags = (_dwFlags & ~dwM) | dwF; }

protected:

    CDwnCrit            _cs;
    CStr                _cstrUrl;
    FILETIME            _ftLastMod;
    DWORD               _dwBindf;
    DWORD               _dwRefresh;
    DWORD               _dwFlags;
    CDwnLoad *          _pDwnLoad;
    CDwnCtx *           _pDwnCtxHead;
    UINT                _cLoad;
    MIMEINFO *          _pmi;
    CDwnInfo *          _pDwnInfoLock;
    DWORD               _dwSecFlags;
#if DBG == 1
    BOOL                _fPassive;
#endif
    
    struct SINKENTRY
    {
        IProgSink * pProgSink;
        ULONG       ulRefs;
        DWORD       dwCookie;
    };

    DECLARE_CDataAry(CArySink, SINKENTRY, Mt(Mem), Mt(CDwnInfo_arySink_pv))

    CArySink        _arySink;

};

// CDwnLoad ----------------------------------------------------------------

class CDwnLoad : public CBaseFT
{
    typedef CBaseFT super;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    #if DBG==1
    void            EnterCriticalSection()      { ((CDwnCrit *)GetPcs())->Enter(); }
    void            LeaveCriticalSection()      { ((CDwnCrit *)GetPcs())->Leave(); }
    BOOL            EnteredCriticalSection()    { return(((CDwnCrit *)GetPcs())->IsEntered()); }
    #endif

    virtual HRESULT Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo) = 0;
    LPCTSTR         GetProgText();
    LPCTSTR         GetUrl()                    { return(_pDwnInfo ? _pDwnInfo->GetUrl() : g_Zero.ach); }
    void            OnDone(HRESULT hrErr);
    void            SetCallback();
    HRESULT         RequestProgress(IProgSink * pProgSink, DWORD dwCookie);

    // Callback methods

    void            OnBindCallback(DWORD dwFlags);
    virtual HRESULT OnBindProgress(DWNPROG * pDwnProg);
    virtual HRESULT OnBindRedirect(LPCTSTR pch, LPCTSTR pchMethod) { return(S_OK); }
    virtual HRESULT OnBindHeaders()                 { return(S_OK); }
    virtual HRESULT OnBindMime(MIMEINFO * pmi)      { _pDwnInfo->SetMimeInfo(pmi); return(S_OK); }
    virtual HRESULT OnBindData()                    { return(S_OK); }
    virtual void    OnBindDone(HRESULT hrErr)       { OnDone(hrErr); }

protected:

                   ~CDwnLoad();

    HRESULT         Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo,
                        UINT idsLoad, DWORD dwFlagsExtra);
    virtual void    Passivate();

    CDwnInfo *      _pDwnInfo;
    CDwnBindData *  _pDwnBindData;
    UINT            _idsLoad;
    LONG            _cDone;
    DWORD           _dwState;
    DWORD           _dwPos;
    DWORD           _dwMax;
    DWORD           _dwIds;
    CStr            _cstrText;
    BOOL            _fPassive;
    BOOL            _fDwnBindTerm;
    IDownloadNotify *_pDownloadNotify;
    HRESULT         _hrErr;

};

// CDwnTaskExec ---------------------------------------------------------------

class CDwnTaskExec : public CExecFT
{
    typedef CExecFT super;
    friend class CDwnTask;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnTaskExec))

                    CDwnTaskExec(CRITICAL_SECTION * pcs);
                   ~CDwnTaskExec();
    HRESULT         Launch();
    void            Shutdown();
    void            AddTask(CDwnTask * pDwnTask);
    void            SetTask(CDwnTask * pDwnTask, BOOL fActive);
    void            DelTask(CDwnTask * pDwnTask);

protected:

    virtual HRESULT ThreadInit();
    virtual void    ThreadExec();
    virtual void    ThreadTerm();
    virtual void    ThreadTimeout();
    BOOL            IsTaskTimeout();
    virtual void    Passivate();

    #if DBG==1
    virtual void    Invariant();
    #endif

    enum TASKACTION
    {
        TA_NONE     = 0,
        TA_DELETE   = 1,
        TA_ACTIVATE = 2,
        TA_BLOCK    = 3
    };

    // Data members

    BOOL            _fShutdown;         // TRUE means shutdown asap
    CDwnTask *      _pDwnTaskHead;      // Head of all tasks
    CDwnTask *      _pDwnTaskTail;      // Tail of all tasks
    CDwnTask *      _pDwnTaskCur;       // Current task for round-robin
    CDwnTask *      _pDwnTaskRun;       // Task currently running
    TASKACTION      _ta;                // Action when current task returns
    LONG            _cDwnTask;          // Count of enqueued tasks
    LONG            _cDwnTaskActive;    // Count of active tasks
    DWORD           _dwTickIdle;        // Time last task was deleted
    DWORD           _dwTickTimeout;     // Timeout interval for self-destruct
    DWORD           _dwTickRun;         // Time task started running
    DWORD           _dwTickSlice;       // Amount of time to run task
    EVENT_HANDLE    _hevWait;           // Event for waiting

    #ifdef PRODUCT_PROF
    BOOL            _fCapOn;
    #endif

};

// CDwnTask -------------------------------------------------------------------

class CDwnTask : public CBaseFT
{
    typedef CBaseFT super;
    friend class CDwnTaskExec;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnTask))

    void            SetBlocked(BOOL fBlocked)   { if (_pDwnTaskExec) _pDwnTaskExec->SetTask(this, !fBlocked); }
    virtual void    Terminate()                 { if (_pDwnTaskExec) _pDwnTaskExec->DelTask(this); }
    BOOL            IsTimeout()                 { return(_fActive && _pDwnTaskExec->IsTaskTimeout()); }

protected:

    virtual void    Run() = 0;
    virtual void    Passivate();

    // Data members

    CDwnTaskExec *  _pDwnTaskExec;      // Task manager owning this task
    CDwnTask *      _pDwnTaskPrev;      // Previous task link
    CDwnTask *      _pDwnTaskNext;      // Next task link
    BOOL            _fActive;           // TRUE if task is ready to run
    BOOL            _fEnqueued;         // TRUE if task is enqueued

};

// CDwnBindData ---------------------------------------------------------------

class CDwnBindData :
    public CDwnBindInfo,
    public IInternetProtocolSink
{
    typedef CDwnBindInfo super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDwnBindData))

    virtual        ~CDwnBindData();

    // Binding

    void            Bind(DWNLOADINFO * pdli, DWORD dwFlagsExtra);
    void            Terminate(HRESULT hrErr);

    // Callback

    void            SetCallback(CDwnLoad * pDwnLoad);
    void            Disconnect();

    // Callback from CDwnDoc

    void            OnDwnDocCallback(void * pvArg);

    // Results

    virtual UINT    GetScheme()         { return(_uScheme); }
    void            GetProgress(DWNPROG * pDwnProg)
                                        { memcpy(pDwnProg, &_DwnProg, sizeof(DWNPROG)); }
    LPCTSTR         GetRedirect()       { return(_cstrRedirect); }
    LPCTSTR         GetMethod()         { return(_cstrMethod); }
    DWORD           GetStatusCode()     { return(_dwStatusCode); }
    LPCTSTR         GetContentType()    { return(_cstrContentType); }
    LPCTSTR         GetRefresh()        { return(_cstrRefresh); }
    FILETIME        GetLastMod()        { return(_ftLastMod); }
    DWORD           GetReqFlags()       { return(_dwReqFlags); }
    DWORD           GetSecFlags()       { return(_dwSecFlags); }
    MIMEINFO *      GetMimeInfo()       { return(_pmi); }
    void            SetMimeInfo(MIMEINFO * pmi)
                                        { _pmi = pmi; }
    MIMEINFO *      GetRawMimeInfoPtr() { return _pRawMimeInfo; }
    HRESULT         GetBindResult()     { return(_hrErr); }
    LPCTSTR         GetFileLock(HANDLE * phLock, BOOL *pfPreTransform);
    CDwnStm *       GetBuffer();
    HRESULT         Peek(void * pv, ULONG cb, ULONG * pcb);
    HRESULT         Read(void * pv, ULONG cb, ULONG * pcb);
    BOOL            IsEof();
    BOOL            IsPending();
    BOOL            IsFullyAvail()      { return(_fFullyAvail); }
    BOOL            FromMimeFilter()    { return(_fMimeFilter); }
    void            GiveRawEcho(BYTE **ppb, ULONG *pcb);
    void            GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppb);

    virtual BOOL    IsBindOnApt()       { return(_fBindOnApt); }

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID, void **);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

private:

    // IBindStatusCallback methods

    STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pib);
    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError);
    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed);

    // IHttpNegotiate methods

    STDMETHOD(OnResponse)(DWORD dwResponseCode, LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders);

    // IInternetBindInfo methods

    STDMETHOD(GetBindInfo)(DWORD * pdwBindf, BINDINFO * pbindinfo);

    // IInternetProtocolSink methods

    STDMETHOD(Switch)(PROTOCOLDATA * ppd);
    STDMETHOD(ReportProgress)(ULONG ulStatusCode, LPCWSTR szStatusText);
    STDMETHOD(ReportData)(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
    STDMETHOD(ReportResult)(HRESULT hrResult, DWORD dwError, LPCWSTR szResult);

    // Internal

    HRESULT         ReadFromData(void * pv, ULONG cb, ULONG * pcb);
    HRESULT         ReadFromBind(void * pv, ULONG cb, ULONG * pcb);
    void            BufferData();
    void            Signal(WORD wSig);
    void            SignalRedirect(LPCTSTR pszText, IUnknown *punkBinding);
    void            SignalFile(LPCTSTR pszText);
    void            SignalHeaders(IUnknown * punkBinding);
    void            SignalProgress(DWORD dwPos, DWORD dwMax, DWORD dwStatus);
    void            SignalData();
    void            SignalDone(HRESULT hrErr);
    HRESULT         SetBindOnApt();
    NV_DECLARE_ONCALL_METHOD(TerminateOnApt, terminateonapt, (DWORD_PTR dwContext));
    void            TerminateBind();
    virtual void    Passivate();
    void            SetEof();
    void            SetPending(BOOL fPending);

    #if DBG==1
    virtual BOOL    CheckThread();
    #endif

    // Data members

    union
    {
        struct
        {
            DWORD               dwTid;
            THREADSTATE *       pts;
            IBindCtx *          pbc;
            IBinding *          pbinding;
            IStream *           pstm;
            IUnknown *          punkForRel;
            BYTE                fTermPosted;
            BYTE                fTermReceived;
            #ifdef OBJCNTCHK
            DWORD               dwObjCnt;
            #endif
        } _u;

        struct
        {
            IInternetProtocol * pInetProt;
        } _o;
    };

    IServiceProvider *  _psp;               // External service provider
    CDwnStm *           _pDwnStm;           // Stream to hold buffered data
    BYTE *              _pbPeek;            // Peek buffer (size is first DWORD)
    DWORD               _dwFlags;           // DWNF_* flags
    DWNPROG             _DwnProg;           // Current progress state
    WORD                _wSigAll;           // All DBF_* flags ever set
    WORD                _wSig;              // Pending DBF_* flags to signal
    DWORD               _dwSigTid;          // Currently signalling thread id
    CDwnLoad *          _pDwnLoad;          // Callback object
    MIMEINFO *          _pmi;               // MIMEINFO of the data
    CStr                _cstrRedirect;      // HTTP redirected URL
    CStr                _cstrMethod;        // HTTP redirected method
    DWORD               _dwStatusCode;      // HTTP response code
    CStr                _cstrContentType;   // HTTP content type header
    CStr                _cstrRefresh;       // HTTP refresh header
    FILETIME            _ftLastMod;         // HTTP/FILE last modified time
    ULONG               _cbRawEcho;         // Byte length of the following field
    BYTE *              _pbRawEcho;         // HTTP echo headers for 449 response
    INTERNET_SECURITY_CONNECTION_INFO *_pSecConInfo;      // Security info structure
    DWORD               _dwReqFlags;        // Reqflag bits (INTERNET_REQFLAG_FROM_CACHE)
    DWORD               _dwSecFlags;        // Secflag bits (SECURITY_FLAG_SECURE)
    CStr                _cstrFile;          // HTTP/FILE file path
    HANDLE              _hLock;             // HTTP cache file locking handle
    UINT                _uScheme;           // URL scheme for this binding
    HRESULT             _hrErr;             // Bind result
    BOOL                _fBindOnApt;        // Binding is on apartment thread
    BOOL                _fFullyAvail;       // All data is available locally
    BOOL                _fSigHeaders;       // TRUE if headers signalled
    BOOL                _fSigMime;          // TRUE if MIME signalled
    BOOL                _fSigData;          // TRUE if data signalled
    BOOL                _fPending;          // Data is stalled
    BOOL                _fEof;              // All data has arrived
    BOOL                _fBindAbort;        // Binding has been aborted
    BOOL                _fBindDone;         // Binding is reported complete
    BOOL                _fBindTerm;         // Binding has been terminated
    BOOL                _fMimeFilter;       // BINDSTATUS_MIMEFILTER
    MIMEINFO *          _pRawMimeInfo;      // Raw mime info - should be verified by sniffer

#if DBG==1 || defined(PERFTAGS)
    ULONG               _cbBind;            // Bytes read in ReadFromBind
#endif

};

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'dwn.hxx'")
#else
#pragma INCMSG("*** Dup 'dwn.hxx'")
#endif
