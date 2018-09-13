//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       pspooler.hxx
//
//  Contents:   CSpooler (the print spooler)
//
//-------------------------------------------------------------------------

#ifndef I_PSPOOLER_HXX_
#define I_PSPOOLER_HXX_
#pragma INCMSG("--- Beg 'pspooler.hxx'")

#ifndef X_DOWNBASE_HXX_
#define X_DOWNBASE_HXX_
#include "downbase.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#define DEFAULTPRINTERSIZE 1024

///////////////////////
// Print Queue types //
///////////////////////

union PrintObject
{
    CPrintDoc *         pPrintDoc;         // Internal CPrintDoc object
    IPrint *            pPrint;            // External IPrint interface
    IOleCommandTarget * pOleCommandTarget; // External IOleCommandTarget interface
};

// Callback ids sent from CPrintDoc/IPrint to CSpooler.
enum SPOOLER_CALLBACK_ID
{
    SP_FINISHED_LOADING_HTMLDOCUMENT,       // 0: CPrintDoc loaded (no distinction between success or not)
    SP_FINISHED_LOADING_EXTERNALDOCUMENT,   // 1: External doc finished loaded successfully
    SP_FAILED_LOADING_EXTERNALDOCUMENT,     // 2: External doc failed to load successfully (cancel)
    SPOOLER_CALLBACK_ID_Last_Enum
};

enum PRINT_OBJECT_TYPE
{
    POT_CPRINTDOC,          // 0: CPrintDoc
    POT_IPRINT,             // 1: IPrint
    POT_IOLECOMMANDTARGET,  // 2: IOleCommandTarget
    PRINT_OBJECT_TYPE_Last_Enum
};

enum REMOVE_DUMMYJOB_MODE   // see CSpooler::RemoveDummyJob() for semantics
{
    RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY, // 0
    RDJ_REMOVE_DUMMYJOB_IF_POSSIBLE,   // 1
    RDJ_FORCE_REMOVAL_FROM_QUEUE,      // 2
    REMOVE_DUMMYJOB_MODE_Last_Enum
};

///////////////////
// Print Spooler //
///////////////////

MtExtern(CSpooler)
MtExtern(CSpoolerCBindStatusCallback)

class CSpooler : public CExecFT
{
public:
    typedef CExecFT super;

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CSpooler))

                        CSpooler();
    virtual            ~CSpooler();
    void                Shutdown();

    ULONG               Release();

    void                OnLoadComplete(void *pPrintObject, SPOOLER_CALLBACK_ID id);

    HRESULT             EnqueuePrintJob(PRINTINFO *ppiNewPrintInfo, ULONG *pulJobId);
    HRESULT             CancelPrintJob(ULONG ulPrintJobId, BOOL fRecursive = FALSE);
    inline HRESULT      CancelAllPrintJobs() { return CancelPrintJob(0); }

    HRESULT             GetPrintInfo(PRINTINFOBAG *pPrintInfo) const;
    HRESULT             SetPrintInfo(const PRINTINFOBAG &printInfoBag);

    PRINTINFOBAG *      GetCurrentLoadJobPrintInfoBag() { return _ppiCurrentLoadJob->ppibagRootDocument; }
    BOOL                IsEmpty();
    BOOL                ShuttingDown() { return _fShutdown; }
    void                UpdateDefaultPrinter();
    void                SetNotifyWindow(HWND hwnd);
    void                AddTempFile(TCHAR * pchTempFile);

protected:

    virtual void        Passivate();
    virtual HRESULT     ThreadInit();
    virtual void        ThreadTerm();
    virtual void        ThreadExec();
            HRESULT     UpdateStatusBar(void);

private:

    class CBindStatusCallback : public IBindStatusCallback
    {
    public:

        DECLARE_MEMALLOC_NEW_DELETE(Mt(CSpoolerCBindStatusCallback))

        CBindStatusCallback(CSpooler *pSpooler, EVENT_HANDLE hEvent, BOOL fBindModeInvestigateObjectType)
            {
                _pSpooler = pSpooler;
                IncrementObjectCount(&_dwObjCnt);
                _ulRefs = 1;
                _hEvent = hEvent;
                _fBindModeInvestigateObjectType = fBindModeInvestigateObjectType;
                _fLoadFailed = FALSE;
            }
        CBindStatusCallback(CSpooler *pSpooler)
            {
                _pSpooler = pSpooler;
                IncrementObjectCount(&_dwObjCnt);
                _ulRefs = 1;
                _hEvent = 0;
                _fBindModeInvestigateObjectType = FALSE;
                _fLoadFailed = FALSE;
            }
        ~CBindStatusCallback() { DecrementObjectCount(&_dwObjCnt); }

        //  IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj);
        STDMETHOD_(ULONG,AddRef)(void);
        STDMETHOD_(ULONG,Release)(void);

        // IBindStatusCallback methods
        STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pib);
        STDMETHOD(GetPriority)(LONG *pnPriority);
        STDMETHOD(OnLowResource)(DWORD dwReserved);
        STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax,  ULONG ulStatusCode,  LPCWSTR szStatusText);
        STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError);
        STDMETHOD(GetBindInfo)(DWORD *grfBINDF, BINDINFO *pbindinfo);
        STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM  *pstgmed);
        STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk);

        // CBindStatusCallback helper methods
        inline BOOL IsObjectHTMLCompatible() { return _fObjectIsHTMLCompatible; }
        inline BOOL LoadFailed()      { return _fLoadFailed;   }

        CSpooler *          _pSpooler;
    private:
        ULONG               _ulRefs;
        EVENT_HANDLE        _hEvent;
        unsigned int        _fBindModeInvestigateObjectType:1;
        unsigned int        _fObjectIsHTMLCompatible:1;
        unsigned int        _fLoadFailed:1;
#ifdef OBJCNTCHK
        DWORD               _dwObjCnt;
#endif
    };
    friend class CBindStatusCallback;

    // CSpooler helper functions
    HRESULT                 InitiateLoading();
    HRESULT                 InitiatePrinting(EVENT_HANDLE *phEventToRaise);
    HRESULT                 IsDocHTMLCompatible(IMoniker *pmk, BOOL *pfTrident);
    HRESULT                 WrapUpPrintJob(PRINTINFO *ppiPrintJob);
    HRESULT                 WrapUpPrintSpooler();
    void                    EmptyQueue(PrintQueue *pdblQueue);
    void                    DeleteTempFiles(BOOL fShutdown = FALSE);
    BOOL                    DuplicatePrintJob(TCHAR *achNewUrl);
    inline ULONG            NewPrintJobId() { return ++_ulPrintJobId; }
    HRESULT                 PrependChildURLsOfLoadDoc(BOOL fLinks, TCHAR *achAlternateUrl = NULL);
    HRESULT                 LoadErrorDocument(CPrintDoc **ppPrintDoc, PRINTINFO *pPIFailedJob, HRESULT hrError);
    HRESULT                 CancelDownload();

    // Dummy job helper functions
    HRESULT                 AddDummyJob(PRINTINFO *pPrintInfo);
    HRESULT                 CancelDummyJob(DUMMYJOB *pDummyJob);
    HRESULT                 RemoveDummyJob(PRINTINFO *pPrintInfo, DUMMYJOB *pDummyJobIn = NULL, REMOVE_DUMMYJOB_MODE dwFlags = RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY);
    HRESULT                 QueryDummyJob(PRINTINFO *pPrintInfo);

    // Private member variables
    int                     _cInit;                 // thread initialization variable
    THREADSTATE *           _pts;                   // our thread state
    EVENT_HANDLE            _hevWait;               // event to wake up CSpooler::ThreadExec()
    CRITICAL_SECTION        _cs;                    // CS for any internal queue or state data

    PrintQueue              _dblPrintJobs;          // queued print jobs
    PrintQueue              _dblOldPrintJobs;       // old print jobs (print recursion)
    CPrintDoc *             _pPrintDocLoading;      // print doc currently being loaded
    PrintObject             _PrintObjectPrinting;   // print doc currently being printed
    PRINTINFO *             _ppiCurrentLoadJob;     // pointer to job currently being loaded
    PRINTINFO *             _ppiCurrentPrintJob;    // pointer to job currently being printed
    DummyJobList            _dblUnremovedDummyJobs; // list of dummy jobs to be removed from queue
    CPtrAry<CStr *>         _aryFrameTempFiles;     // array of i/frame tempfile names to be deleted after printing

    void *                  _pPrintObjectLoadFinishedEarly; // pointer to object whose load finished early (synchronously)
    SPOOLER_CALLBACK_ID     _idCallbackIdLoadFinishedEarly; // callback type id of job whose load finished early (synchronously)

    PRINTINFOBAG            _PrintInfoBag;          // holds the current data that is manipulated
                                                    // by the user. this data get's copied into the
                                                    // printjobs
    ULONG                   _ulPrintJobId;          // print job id counter
    CBindStatusCallback *   _pBindStatusCallback;   // bind status callback object pointer

    unsigned int            _fLoadJobWaiting:1;     // is any job waiting to be loaded
    unsigned int            _fPrintJobWaiting:1;    // is any job waiting to be printed
    unsigned int            _fCanLoad:1;            // the spooler can load because nothing is being
                                                    // downloaded currently
    unsigned int            _fShutdown:1;           // the spooler is requested to shut itself down ASAP
    unsigned int            _fRemoveTempFilesWhenEmpty:1; // needs to be set in order for us to consider removing these tempfiles
    HWND                    _hwndNotify;            // notify window handle for statusbar

    TCHAR                   _achDefaultPrinter[DEFAULTPRINTERSIZE]; // holds the defaultprinter string
    DWORD                   _dwLoadJobAbortTime;    // the tickcount time in milliseconds used for load job cancellation timeout

#ifdef OBJCNTCHK
    DWORD                   _dwObjCnt;
#endif

};

HRESULT GetSpooler(CSpooler ** ppSpooler);
HRESULT CreatePrintInfo(PRINTINFO **pppiNewPrintJob, PRINTINFOBAG *ppibagRootDocument = NULL);
HRESULT IsSpooler(void);

#pragma INCMSG("--- End 'pspooler.hxx'")
#else
#pragma INCMSG("*** Dup 'pspooler.hxx'")
#endif
