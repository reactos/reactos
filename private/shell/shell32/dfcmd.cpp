#include "shellprv.h"
#pragma  hdrstop

#include "dspsprt.h"
#include "dfcmd.h"
#include "docfindx.h"
#include "docfind.h"
#include "cowsite.h"
#include "cobjsafe.h"
#include "cnctnpt.h"
#include "stdenum.h"
#include "exdisp.h"
#include "exdispid.h"
#include "shldisp.h"
#include "shdispid.h"
#include "dataprv.h"
#include "ids.h"

extern "C" 
{
#include "views.h"
}


//=============================================================================
// CDFCommand class
//=============================================================================

#define WM_DF_SEARCHPROGRESS        (WM_USER + 42)
#define WM_DF_ASYNCPROGRESS         (WM_USER + 43)
#define WM_DF_SEARCHSTART           (WM_USER + 44)
#define WM_DF_SEARCHCOMPLETE        (WM_USER + 45)
#define WM_DF_FSNOTIFY              (WM_USER + 46)
#define WM_DF_ASYNCTOSYNC           (WM_USER + 47)

STDAPI CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvObj);
STDAPI CDocFindFolder_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);

typedef struct 
{
    BSTR        szName;
    VARIANT     vValue;
} DFCommandConstraint;


class CDFCommand : public ISearchCommandExt,
                   public CImpIDispatch, 
                   public CObjectWithSite, 
                   public CObjectSafety, 
                   public IConnectionPointContainer,
                   public IProvideClassInfo2,
                   public CShellFolderData,
                   public IRowsetWatchNotify,
                   public IDocFindControllerNotify
{    
    friend HRESULT CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvObj);

public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);        
    STDMETHOD_(ULONG, Release)(void);

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);


    // *** IConnectionPointContainer methods ***    
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP);

    // *** IProvideClassInfo methods ***    
    STDMETHOD(GetClassInfo)(ITypeInfo **ppTI);

    // *** IProvideClassInfo2 methods ***    
    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID *pGUID);

    // *** IObjectWithSite ***
    STDMETHOD(SetSite)(IUnknown *punkSite);

    // *** ISearchCommandExt ***
    STDMETHOD(ClearResults)(void);
    STDMETHOD(NavigateToSearchResults)(void);
    STDMETHOD(get_ProgressText)(BSTR *pbs);
    STDMETHOD(SaveSearch)(void);
    STDMETHOD(RestoreSearch)(void);
    STDMETHOD(GetErrorInfo)(BSTR *pbs,  int *phr);
    STDMETHOD(SearchFor)(int iFor);
    STDMETHOD(GetScopeInfo)(BSTR bsScope, int *pdwScopeInfo);
    STDMETHOD(RestoreSavedSearch)(VARIANT *pvarFile);

    STDMETHOD(Execute)(VARIANT *RecordsAffected, VARIANT *Parameters, long Options);
    STDMETHOD(AddConstraint)(BSTR Name, VARIANT Value);        
    STDMETHOD(GetNextConstraint)(VARIANT_BOOL fReset, DFConstraint **ppdfc);

    // *** IRowsetWatchNotify ***
    STDMETHODIMP OnChange(IRowset *prowset, DBWATCHNOTIFY eChangeReason);

    // *** IDocFindControllerNotify ***
    STDMETHOD(DoSortOnColumn)(UINT iCol, BOOL fSameCol);
    STDMETHOD(StopSearch)(void);
    STDMETHOD(GetItemCount)(UINT *pcItems);
    STDMETHOD(SetItemCount)(UINT cItems);
    STDMETHOD(ViewDestroyed)();

private:
    CDFCommand();
    ~CDFCommand();
    HRESULT      Init(void);
    HRESULT      _GetSearchIDList(LPITEMIDLIST *ppidl);
    HRESULT      _SetEmptyText( UINT nID ) ;
    void         _SelectResults();
    STDMETHODIMP _Clear() ;

    struct ExecThreadParams {
        CDFCommand    *pThis;
        IDFEnum       *pdfenum;
    };

    struct DeferUpdateDir {
        struct DeferUpdateDir  *pdudNext;
        LPITEMIDLIST            pidl;
        BOOL                    fRecurse;
    };

    // Internal class to handle notifications from top level browser
    class CWBEvents2: public DWebBrowserEvents
    {
    public:
        STDMETHOD(QueryInterface) (REFIID riid, void **ppvObject);
        STDMETHOD_(ULONG, AddRef)(void)        
                { return _pcdfc->AddRef();}
        STDMETHOD_(ULONG, Release)(void)
                { return _pcdfc->Release();}
    
        // *** (DwebBrowserEvents)IDispatch methods ***
        STDMETHOD(GetTypeInfoCount)(UINT * pctinfo)
                { return E_NOTIMPL;}
        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
                { return E_NOTIMPL;}
        STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
                { return E_NOTIMPL;}
        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

        // Some helper functions...
        void SetOwner(CDFCommand *pcdfc)
            { _pcdfc = pcdfc; }  // Don't addref as part of larger object... }
        void SetWaiting(BOOL fWait)
            {_fWaitingForNavigate = fWait;}

    protected:
        // Internal variables...
        CDFCommand      *_pcdfc;     // pointer to top object... could cast, but...
        BOOL            _fWaitingForNavigate;   // Are we waiting for the navigate to search resluts?
    };

    friend class CWBEvents2;
    CWBEvents2              _cwbe;
    IConnectionPoint        *_pcpBrowser;   // hold onto browsers connection point;
    unsigned long           _dwCookie;      // Cookie returned by Advise

    // Internal functions...
    STDMETHODIMP            _UpdateFilter(IDocFindFileFilter *pdfff);
    void                    _ClearConstraints();
    static DWORD CALLBACK   _Execute_ThreadProc(LPVOID lpThreadParams);
    STDMETHODIMP            _Execute_Start(BOOL fNavigateIfFail, int iCol, LPITEMIDLIST pidlUpdate);
    STDMETHODIMP            _Execute_Cancel();
    STDMETHODIMP            _Execute_Init(ExecThreadParams **ppParams, int iCol, LPITEMIDLIST pidlUpdate);
    static HRESULT          _ExecParams_Free(ExecThreadParams *pExecThreadParams);
    STDMETHODIMP            _ExecData_Init();
    HRESULT                 _ExecData_ValidateRightSearchResults(IShellFolder *psf);
    STDMETHODIMP            _ExecData_Release();
    BOOL                    _Execute_SetupBrowserCP();
    void cdecl              _NotifyProgressText(UINT ids,...);
    static LRESULT CALLBACK s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void                    _PTN_SearchProgress(void);
    void                    _PTN_AsyncProgress(int nPercentComplete, DWORD cAsync);
    void                    _PTN_AsyncToSync(void);
    void                    _PTN_SearchComplete(HRESULT hr, BOOL fAbort);
    void                    _handleFSChange(LONG code, LPITEMIDLIST *ppidl);
    void                    _DeferHandleUpdateDir(LPITEMIDLIST pidl, BOOL bRecurse);
    void                    _ClearDeferUpdateDirList();
    HRESULT                 _SetLastError(HRESULT hr);
    inline BOOL             _SearchForComputer() {return (_iSearchFor == 1);};
    IUnknown*               _GetObjectToPersist();
    HRESULT                 _ForcedUnadvise(void);
    void                    _PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // These are the things that the second thread will use during it's processing...
    struct UpdateParams {
        CRITICAL_SECTION    csSearch;
        HWND                hwndThreadNotify;
        HDPA                hdpa;
        DWORD               dwTimeLastNotify;   
        BOOL                fFilesAdded : 1;
        BOOL                fDirChanged : 1;
        BOOL                fUpdatePosted : 1;
    } _updateParams; // Pass callback params through this object to avoid alloc/free cycle

    struct {
        IShellFolder        *psf;
        IShellFolderView    *psfv;
        IDocFindFolder      *pdfFolder;    
        TCHAR               szProgressText[MAX_PATH];
    } _execData;

public:

private:
    LONG                _cRef;
    HDSA                _hdsaConstraints;
    DWORD               _cExecInProgress;
    BITBOOL             _fAsyncNotifyReceived:1;
    BITBOOL             _fDeferRestore:1;
    BITBOOL             _fDeferRestoreTried:1;
    BOOL                _fContinue;
    BOOL                _fNew;
    CConnectionPoint    _cpEvents;
    IDefViewFrame       *_pdvResultsTargetFrame;    
    OLEDBSimpleProviderListener *_pListener;
    HDPA                _hdpaItemsToAdd1;
    HDPA                _hdpaItemsToAdd2;
    TCHAR               _szProgressText[MAX_PATH+40];   // progress text leave room for chars...
    LPITEMIDLIST        _pidlUpdate;                    // Are we processing an updatedir?
    LPITEMIDLIST        _pidlRestore;                   // pidl to do restore from...
    struct DeferUpdateDir *_pdudFirst;                  // Do we have any defered update dirs?
    HRESULT             _hrLastError;                   // the last error reported.
    int                 _iSearchFor;                    // Searching for file or printers?
    UINT                _uStatusMsgIndex;               // Files or computers found...
    CRITICAL_SECTION    _csThread;
    
    DFBSAVEINFO         _dfbsi;
};


class CDFConstraint: public DFConstraint,
                   public CImpIDispatch 
{    
    friend HRESULT CDFConstraint_CreateInstance(BSTR bName, VARIANT vValue);

public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);        
    STDMETHOD_(ULONG, Release)(void);

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // *** DFConstraint ***
    STDMETHOD(get_Name)(BSTR *pbs);
    STDMETHOD(get_Value)(VARIANT *pvar);


    CDFConstraint(BSTR bstr, VARIANT var);
private:
    ~CDFConstraint();
    LONG                _cRef;
    BSTR                _bstr;
    VARIANT             _var;
};

//
// CDFCommand ctor/dtor implementation
//

CDFCommand::CDFCommand() 
: CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_ISearchCommandExt)
{
    _cRef                 = 1;
    _fAsyncNotifyReceived = 0;
    _fContinue = TRUE;
    ASSERT(NULL == _pidlRestore);

    ASSERT(_cExecInProgress == 0);

    InitializeCriticalSection(&_updateParams.csSearch);
    InitializeCriticalSection(&_csThread);

    _cpEvents.SetOwner((ISearchCommandExt *)this, &DIID_DSearchCommandEvents);
}

HRESULT CDFCommand::Init(void)
{
    _hdsaConstraints = DSA_Create(sizeof(DFCommandConstraint), 4);
    if (!_hdsaConstraints)
        return E_OUTOFMEMORY;
    return S_OK;
}

CDFCommand::~CDFCommand()
{

    _ClearConstraints();
    DSA_Destroy(_hdsaConstraints);
    ATOMICRELEASE(_pdvResultsTargetFrame);
    _ExecData_Release();

    DeleteCriticalSection(&_updateParams.csSearch);
    DeleteCriticalSection(&_csThread);

    // Make sure we have removed all outstanding update dirs...
    _ClearDeferUpdateDirList();

    if (_updateParams.hwndThreadNotify) {
        // make sure no outstanding fsnotifies registered.
        SHChangeNotifyDeregisterWindow(_updateParams.hwndThreadNotify);
        DestroyWindow(_updateParams.hwndThreadNotify);
    }

    if (_hdpaItemsToAdd1)
        DPA_Destroy(_hdpaItemsToAdd1);

    if (_hdpaItemsToAdd2)
        DPA_Destroy(_hdpaItemsToAdd2);

    ILFree(_pidlRestore) ;
}

//
// CDFParameter IUnknown implementation
//

STDMETHODIMP CDFCommand::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDFCommand, ISearchCommandExt),             // ISearchCommandExt
        QITABENTMULTI(CDFCommand, IDispatch, ISearchCommandExt),   //IID_IDispatch
        QITABENT(CDFCommand, IProvideClassInfo2),       //IID_IProvideClassInfo2
        QITABENTMULTI(CDFCommand, IProvideClassInfo,IProvideClassInfo2 ),   //IID_IProvideClassInfo
        QITABENT(CDFCommand, IObjectWithSite),              //IID_IOBjectWithSite
        QITABENT(CDFCommand, IObjectSafety),                // IID_IObjectSafety
        QITABENT(CDFCommand, IConnectionPointContainer),    // IID_IConnectionPointContainer
        QITABENT(CDFCommand, ISearchCommandExt),            // IID_ISearchCommandExt
        QITABENT(CDFCommand, OLEDBSimpleProvider),          // IID_IOLEDBSimpleProvider
#ifdef WINNT
        QITABENT(CDFCommand, IRowsetWatchNotify),           // IID_IRowsetWatchNotify
#endif
        QITABENT(CDFCommand, IDocFindControllerNotify),     // IID_IDocFindControllerNotify
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDFCommand::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceMsg(TF_DOCFIND, "CDFCommand.AddRef %d",_cRef);
    return _cRef;
}

STDMETHODIMP_(ULONG) CDFCommand::Release()
{
    TraceMsg(TF_DOCFIND, "CDFCommand.Release %d",_cRef-1);
    if (InterlockedDecrement(&_cRef)) {
        return _cRef;
    }
    delete this;
    return 0;
}


//
// CDFCommand IDispatch implementation
//

STDMETHODIMP CDFCommand::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CDFCommand::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CDFCommand::GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CDFCommand::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

//
// CDFCommand ADOCommand implementation
//

STDMETHODIMP CDFCommand::AddConstraint(BSTR szName, VARIANT vValue)
{
    DFCommandConstraint dfcc;        
    HRESULT hr;

    dfcc.szName = SysAllocString(szName);
    if (dfcc.szName == NULL)
        return E_OUTOFMEMORY;

    VariantInit(&dfcc.vValue);
    if (FAILED(hr = VariantCopy(&dfcc.vValue, &vValue)))
    {
        SysFreeString(dfcc.szName);
        return hr;
    }

    if (DSA_InsertItem(_hdsaConstraints, DSA_APPEND, &dfcc) == DSA_ERR)
    {
        SysFreeString(dfcc.szName);
        VariantClear(&dfcc.vValue);
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP CDFCommand::GetNextConstraint(VARIANT_BOOL fReset, DFConstraint **ppdfc)
{
    // Let them think we are at the end...
    HRESULT hr;
    IDocFindFileFilter *pdfff;

    *ppdfc = NULL;

    if (SUCCEEDED(hr = _execData.pdfFolder->GetDocFindFilter(&pdfff)))
    {
        BSTR bName;
        VARIANT var;
        VariantInit(&var);
        VARIANT_BOOL fFound;
        hr = pdfff->GetNextConstraint(fReset, &bName, &var, &fFound);
        if (SUCCEEDED(hr))
        {
            if (!fFound)
            {
                // need a simple way to signal end list, how about an empty name string?
                // BUGBUG:: maybe should find way to detect a NULL object?
                bName = SysAllocString(L"");
            }
            CDFConstraint *pdfc = new CDFConstraint(bName, var);
            if (pdfc)
            {
                hr = pdfc->QueryInterface(IID_DFConstraint, (void**)ppdfc);
                pdfc->Release();
            }
            else
            {
                // error release stuff we allocated.
                hr = E_OUTOFMEMORY;
                SysFreeString(bName);
                VariantClear(&var);
            }
        }
        pdfff->Release();
    }
    return hr;
}




HRESULT CDFCommand::_UpdateFilter(IDocFindFileFilter *pdfff)
{
    HRESULT         hr;
    DFCommandConstraint *pdfcc;
    LONG            cNumParams;
    int             iItem;

    if (!pdfff)
        return E_INVALIDARG;

    hr = S_OK;
    UINT uMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    pdfff->ResetFieldsToDefaults();
    cNumParams = DSA_GetItemCount(_hdsaConstraints); 
    for (iItem = 0; iItem < cNumParams; iItem++)
    {
        pdfcc = (DFCommandConstraint *)DSA_GetItemPtr(_hdsaConstraints, iItem);
        if (pdfcc)
        {
            hr = pdfff->UpdateField(pdfcc->szName, pdfcc->vValue);
        }
    }

    // And clear out the constraint list...
    _ClearConstraints();
    SetErrorMode(uMode);
    return hr;
}

void CDFCommand::_ClearConstraints()
{
    DFCommandConstraint *pdfcc;
    LONG            cNumParams;
    int             iItem;

    cNumParams = DSA_GetItemCount(_hdsaConstraints); 
    for (iItem = 0; iItem < cNumParams; iItem++)
    {
        pdfcc = (DFCommandConstraint *)DSA_GetItemPtr(_hdsaConstraints, iItem);
        if (pdfcc)
        {
            SysFreeString(pdfcc->szName);
            VariantClear(&pdfcc->vValue);
        }
    }
    DSA_DeleteAllItems(_hdsaConstraints);
}

void cdecl CDFCommand::_NotifyProgressText(UINT ids,...)
{
    va_list ArgList;
    va_start(ArgList, ids);
    LPTSTR psz = _ConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(ids), &ArgList);
    va_end(ArgList);

    if (psz)
    {
        LPTSTR pszDst;

        // a-msadek; needed only for BiDi Win95 loc
        // Mirroring will take care of that over NT5 & BiDi Win98
        if(g_bBiDiW95Loc)
        {
            _szProgressText[0] = _szProgressText[1] = TEXT('\t');
            pszDst = &_szProgressText[2];
        } 
        else
            pszDst = &_szProgressText[0];

        StrCpyN(pszDst, psz, ARRAYSIZE(_szProgressText)-2);

        LocalFree(psz);
    }
    else
    {
        _szProgressText[0] = TEXT('\0');
    }

    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_PROGRESSTEXT);
}

STDAPI CDocFindCommand_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvObj)
{
    HRESULT    hr;

    *ppvObj = NULL;
    CDFCommand *pdfCmd = new CDFCommand();
    if (pdfCmd)
    {
        hr = pdfCmd->Init();
        if (SUCCEEDED(hr))
            hr = pdfCmd->QueryInterface(riid, ppvObj);
        pdfCmd->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;    
}

void CDFCommand::_PTN_SearchProgress(void)
{
    HRESULT      hr = S_OK;
    int iItem;
    HDPA         hdpa;
    BOOL         fDirChanged;

    hdpa = _updateParams.hdpa;

    if (hdpa) 
    {
        // Ok lets swap things out from under other thread so that we can process it and still
        // let the other thread run...
        EnterCriticalSection(&_updateParams.csSearch);
        if (_updateParams.hdpa == _hdpaItemsToAdd2)
            _updateParams.hdpa = _hdpaItemsToAdd1;
        else
            _updateParams.hdpa = _hdpaItemsToAdd2;

        // say that we don't have any thing here such that other thread will reset up...
        _updateParams.fFilesAdded = FALSE;
        fDirChanged = _updateParams.fDirChanged;
        _updateParams.fDirChanged = FALSE;
        LeaveCriticalSection(&_updateParams.csSearch);

        int cItemsToAdd = DPA_GetPtrCount(hdpa);
        int i;
        int iItemStart;
        LPITEMIDLIST pidl;

        if (!_execData.pdfFolder)
            return;
            
        _execData.pdfFolder->GetItemCount(&iItem);
        iItemStart = iItem + 1;     // needed for notifies 1 based.

        if (cItemsToAdd)
        {
            if (_fContinue)
            {
                // Are we in an updatedir?  If so then need to do merge, else...
                if (_pidlUpdate) 
                {
                    // Now the harder part see if items in list, already if so we unmark the item
                    // for delete else if not there maybe add it...
                    // Note, we can merge this with the code in DocFindX, which once we are done
                    // will be removed...
                    int iLastMatch = -1;
                    int j;
                    LPSHELLFOLDER psfItem;
                    ESFItem *pesfi;
                    int cItems = iItem;        
                    int iItemCheck;
                    for (i = 0; i < cItemsToAdd; i++) 
                    {
                        pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, i);
                        psfItem = DocFind_GetObjectsIFolder(_execData.pdfFolder, NULL, pidl);
                        // Now see if the item is in the DPA
                        if (psfItem) 
                        {
                            for (j=cItems-1, iItemCheck = iLastMatch + 1; j > 0; j--) 
                            {
                                if (iItemCheck >= cItems)
                                    iItemCheck = 0;
                                _execData.pdfFolder->GetItem(j, &pesfi);
                                if (pesfi && DF_IFOLDER(pidl) == DF_IFOLDER(&pesfi->idl)) 
                                {
                                    if (psfItem->CompareIDs(0, pidl, &pesfi->idl) == (HRESULT)0)
                                        break;
                                }
                            }
                            if (j == 0) 
                            {
                                // Not already in the list so add it...
                                if (SUCCEEDED(hr = _execData.pdfFolder->AddPidl(iItem, pidl, -1, NULL)))
                                    iItem++;
                            } 
                            else 
                            {
                                // Item still there - remove possible delete flag...
                                pesfi->dwState &= ~CDFITEM_STATE_MAYBEDELETE;
                                iLastMatch = iItemCheck;
                            }
                        }

                        ILFree(pidl);   // The AddPidl does a clone of the pidl...
                    }
                    if (iItem && _execData.psfv)
                    {
                         hr = _execData.psfv->SetObjectCount(iItem, SFVSOC_NOSCROLL);
                    }
                } 
                else 
                {
                    if (_pListener)
                        _pListener->aboutToInsertRows(iItemStart, cItemsToAdd);
                    
                    for (i = 0; i < cItemsToAdd; i++) 
                    {
                        if (SUCCEEDED(hr = _execData.pdfFolder->AddPidl(iItem, 
                                pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, i), -1, NULL)))
                            iItem++;
                        ILFree(pidl);   // The AddPidl does a clone of the pidl...
                    }
        
        
                    if (iItem >= iItemStart)
                    {
                        if (_execData.psfv)
                        {
                            hr = _execData.psfv->SetObjectCount(iItem, SFVSOC_NOSCROLL);
                        }
                
                        _execData.pdfFolder->SetItemsChangedSinceSort();
                        _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_UPDATE);
                    }
                    if (_pListener) 
                    {
                        _pListener->insertedRows(iItemStart, cItemsToAdd);
                        _pListener->rowsAvailable(iItemStart, cItemsToAdd);
                    }
                }
            }
            else  // _fContinue
            {
                for (i = 0; i < cItemsToAdd; i++)
                {
                    pidl = (LPITEMIDLIST)DPA_FastGetPtr(hdpa, i);
                    ASSERT(pidl);
                    ILFree(pidl);
                }
            }
            DPA_DeleteAllPtrs(hdpa);
        }

        if (fDirChanged) 
        {
            _NotifyProgressText(IDS_SEARCHING, _execData.szProgressText);
        }
    }

    _updateParams.dwTimeLastNotify = GetTickCount();
    _updateParams.fUpdatePosted = FALSE;
}

void CDFCommand::_PTN_AsyncProgress(int nPercentComplete, DWORD cAsync)
{
    if (!_execData.pdfFolder)
        return;
    // Async case try just setting the count...
    _execData.pdfFolder->SetAsyncCount(cAsync);
    if (_execData.psfv) 
    {
#ifdef CI_ROWSETNOTIFY
        // ci no longer supports async notifications so we have to validate items
        // all the time..
        if (_fAsyncNotifyReceived) 
#endif
        {
            // -1 for the first item means verify visible items only
            _execData.pdfFolder->ValidateItems(-1, -1, FALSE);
#ifdef CI_ROWSETNOTIFY
            _fAsyncNotifyReceived = FALSE;
#endif
        }
        _execData.psfv->SetObjectCount(cAsync, SFVSOC_NOSCROLL);
    }

    _execData.pdfFolder->SetItemsChangedSinceSort();
    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_UPDATE);
    _NotifyProgressText(IDS_SEARCHINGASYNC, cAsync, nPercentComplete);
}

void CDFCommand::_PTN_AsyncToSync()
{
    if (_execData.pdfFolder)
        _execData.pdfFolder->CacheAllAsyncItems();
}

void CDFCommand::_PTN_SearchComplete(HRESULT hr, BOOL fAbort)
{
    int iItem;
    // weird connection point corruption can happen here.  somehow the number of sinks is 0 but 
    // some of the array entries are non null thus causing fault.  this problem does not want to 
    // repro w/ manual testing or debug binaries, only sometimes after an automation run.  when
    // it happens it is too late to figure out what happened so just patch it here.
    if (_cpEvents._HasSinks())
        _cpEvents.InvokeDispid(fAbort? DISPID_SEARCHCOMMAND_ABORT : DISPID_SEARCHCOMMAND_COMPLETE);
    // someone clicked on new button -- cannot set no files found text in listview
    // because we'll overwrite enter search criteria to begin
    if (!_fNew)
        _SetEmptyText( IDS_FINDVIEWEMPTY ) ;
    _SetLastError(hr);

    // _execData.pdfFolder is NULL when Searh is complete by navigating away from the search page
    if (!_execData.pdfFolder)
    {
        // do clean up of hdpaToItemsToadd1 and 2
        // make sure all items in buffer 1 and 2 are empty
        HDPA hdpa = _hdpaItemsToAdd1;
        int cItems;
        int i;
        if (hdpa)
        {
            EnterCriticalSection(&_updateParams.csSearch);
            cItems = DPA_GetPtrCount(hdpa);
            for (i = 0; i < cItems; i++) 
            {
                ILFree((LPITEMIDLIST)DPA_GetPtr(hdpa, i));
            }
            DPA_DeleteAllPtrs(hdpa);
            LeaveCriticalSection(&_updateParams.csSearch);
        }

        hdpa = _hdpaItemsToAdd2;
        if (hdpa)
        {
            EnterCriticalSection(&_updateParams.csSearch);
            cItems = DPA_GetPtrCount(hdpa);
            for (i = 0; i < cItems; i++) 
            {
                ILFree((LPITEMIDLIST)DPA_GetPtr(hdpa, i));
            }
            DPA_DeleteAllPtrs(hdpa);
            LeaveCriticalSection(&_updateParams.csSearch);
        }

        return;
    }
    // if we have a _pidlUpdate are completing an update
    if (_pidlUpdate)
    {
        ESFItem *pesfi;
        int i, cPidf;
        UINT uItem;


        _execData.pdfFolder->GetItemCount(&i);
        for (; i-- > 0; )
        {
            // Pidl at start of structure...
            _execData.pdfFolder->GetItem(i, &pesfi);
            if (pesfi && pesfi->dwState & CDFITEM_STATE_MAYBEDELETE)
            {
                _execData.psfv->RemoveObject(&pesfi->idl, &uItem);
            }
        }                  

        ILFree(_pidlUpdate);
        _pidlUpdate = NULL;

        // clear the update dir flags
        _execData.pdfFolder->GetFolderListItemCount(&cPidf);
        for (i = 0; i < cPidf; i++ )
        {
            DFFolderListItem *pdffli;
            
            if (SUCCEEDED(_execData.pdfFolder->GetFolderListItem(i, &pdffli)))
                pdffli->fUpdateDir = FALSE;
        }
    }

    // Tell everyone the final count and that we are done...
    // But first check if there are any cached up Updatedirs to be processed...
    if (_pdudFirst) 
    {
        // first unlink the first one...
        struct DeferUpdateDir *pdud = _pdudFirst;
        _pdudFirst = pdud->pdudNext;

        if (DFB_handleUpdateDir(_execData.pdfFolder, pdud->pidl, pdud->fRecurse)) 
        {
            // Need to spawn sub-search on this...
            _Execute_Start(FALSE, -1, pdud->pidl);
        }
        ILFree(pdud->pidl);
        LocalFree((HLOCAL)pdud);
    } 
    else 
    {
        if (_cExecInProgress)
            _cExecInProgress--;
        if (_execData.psfv) 
        {
#ifdef CI_ROWSETNOTIFY
            // ci no longer supports async notifications so we have to validate items
            // all the time..
            if (_fAsyncNotifyReceived) 
#endif
            {
                // validate all the items we pulled in already
                _execData.pdfFolder->ValidateItems(0, -1, TRUE);
#ifdef CI_ROWSETNOTIFY
                _fAsyncNotifyReceived = FALSE;
#endif
            }
        }
        _execData.pdfFolder->GetItemCount(&iItem);
        _NotifyProgressText(_uStatusMsgIndex, iItem);
        if (!fAbort)
            _SelectResults();
    }
}


void CDFCommand::_handleFSChange(LONG code, LPITEMIDLIST *ppidl)
{
    LPITEMIDLIST pidlT;
    UINT idsMsg;
    UINT cItems;
    IDFEnum *pdfEnumAsync;

    if (!_execData.pdfFolder)
        _ExecData_Init();
    // If we are running async then for now ignore notifications...
    // Unless we have cached all of the items...
    if (!_execData.pdfFolder)
        return; // we do not have anything to listen...

    _execData.pdfFolder->GetAsyncEnum(&pdfEnumAsync);

    // see if we want to process the notificiation or not.
    switch (code)
    {
    case SHCNE_RENAMEFOLDER:    // With trashcan this is what we see...
    case SHCNE_RENAMEITEM:    // With trashcan this is what we see...
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
    case SHCNE_UPDATEITEM:
        break;
    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        // Process this one out of place
        DFB_UpdateOrMaybeAddPidl(_execData.pdfFolder, _execData.psfv, code, *ppidl, NULL);
        break;

    case SHCNE_UPDATEDIR:
        {
            // BUGBUG:: How to handle updatedir when there is an async enum involved...
            // for now punt
            TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_UPDATEDIR, pidl=0x%X",*ppidl);
            BOOL bRecurse = (ppidl[1] != NULL);
            if (!pdfEnumAsync) 
            {
                if (_cExecInProgress) 
                {
                    _DeferHandleUpdateDir(*ppidl, bRecurse);
                } 
                else 
                {
                    if (DFB_handleUpdateDir(_execData.pdfFolder, *ppidl, bRecurse)) 
                    {
                        // Need to spawn sub-search on this...
                        _Execute_Start(FALSE, -1, *ppidl);
                    }
                }
            }
        }
        return;

    default:
        return;     // we are not interested in this event
    }

    //
    // Now we need to see if the item might be in our list
    // First we need to extract off the last part of the id list
    // and see if the contained id entry is in our list.  If so we
    // need to see if can get the defview find the item and update it.
    //

    _execData.pdfFolder->MapFSPidlToDFPidl(*ppidl, FALSE, &pidlT);

    switch (code)
    {
    case SHCNE_RMDIR:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_RMDIR, pidl=0x%X",*ppidl);
        DFB_handleRMDir(_execData.pdfFolder, _execData.psfv, *ppidl);
        // Fall through to see if we should delete the item itself...
        goto RMObj;

    case SHCNE_DELETE:
        TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_DELETE, pidl=0x%X",*ppidl);
RMObj:
        if (pidlT != NULL)
        {
            _execData.psfv->RemoveObject(pidlT, &idsMsg);
        }
        break;

    case SHCNE_RENAMEFOLDER:
        // BUGBUG:: On rename directory we should see if the old one is in our
        // range and the new one is not and then call the HandleRMDir function
    case SHCNE_RENAMEITEM:
        {
            if (pidlT)
            {
                LPITEMIDLIST pidl1;
                LPITEMIDLIST pidl2;
                // If the two items dont have the same parent, we will go ahead
                // and remove it...
                _execData.pdfFolder->GetParentsPIDL(pidlT, &pidl1);
                pidl2 = ILClone(ppidl[1]);
                if (pidl1 && pidl2)
                {
                    ILRemoveLastID(pidl2);
                    if (!ILIsEqual(pidl1, pidl2))
                    {
                        _execData.psfv->RemoveObject(pidlT, &idsMsg);

                        // And maybe add it back to the end... of the list
                        DFB_UpdateOrMaybeAddPidl(_execData.pdfFolder, _execData.psfv, code, ppidl[1], NULL);
                    }
                    else
                    {
                        // The object is in same folder so must be rename...
                        // And maybe add it back to the end... of the list
                        DFB_UpdateOrMaybeAddPidl(_execData.pdfFolder, _execData.psfv, code, ppidl[1], pidlT);
                    }
                }
                ILFree(pidl2);
            }
            else
                DFB_UpdateOrMaybeAddPidl(_execData.pdfFolder, _execData.psfv, code, ppidl[1], NULL);
        }
        break;

    case SHCNE_UPDATEITEM:
        {
            TraceMsg(TF_DOCFIND, "DocFind got notify SHCNE_UPDATEITEM, pidl=0x%X",*ppidl);
            // We need to do a find first and convert to pidl...
            if (pidlT)
                DFB_UpdateOrMaybeAddPidl(_execData.pdfFolder, _execData.psfv, code, pidlT, pidlT);
        }
        break;
    }


    // Update the count...
    _execData.psfv->GetObjectCount(&cItems);
    _NotifyProgressText(_uStatusMsgIndex, cItems);

    ILFree(pidlT);
}                          


void CDFCommand::_DeferHandleUpdateDir(LPITEMIDLIST pidl, BOOL bRecurse)
{
    // Ok we need to add a defer
    struct DeferUpdateDir *pdud = _pdudFirst;
    struct DeferUpdateDir *pdudPrev = NULL;

    // See if we already have some items in the list which are lower down in the tree if so we
    // can replace it.  Or is there one that is higher up, in which case we can ignore it...
    while (pdud) {
        if (ILIsParent(pdud->pidl, pidl, FALSE))
            return;     // Already one in the list that will handle this one...
        if (ILIsParent(pidl, pdud->pidl, FALSE))
            break;
        pdudPrev = pdud;
        pdud = pdud->pdudNext;
    }

    // See if we found one that we can replace...
    if (pdud) {
        LPITEMIDLIST pidlT = ILClone(pidl);
        if (pidlT) {
            ILFree(pdud->pidl);
            pdud->pidl = pidlT;

            // See if there are others...
            pdudPrev = pdud;
            pdud = pdud->pdudNext;
            while (pdud) {
                if (ILIsParent(pidl, pdud->pidl, FALSE)) {
                    // Yep lets trash this one.
                    ILFree(pdud->pidl);
                    pdudPrev->pdudNext = pdud->pdudNext;
                    pdud = pdudPrev;    // Let it fall through to setup to look at next...
                }
                pdudPrev = pdud;
                pdud = pdud->pdudNext;
            }
        }
    }
    else {
        // Nope simply add us in to the start of the list.
        pdud = (struct DeferUpdateDir*)LocalAlloc(LPTR, sizeof(struct DeferUpdateDir));
        if (!pdud)
            return; // Ooop could not alloc...
        pdud->pidl = ILClone(pidl);
        if (!pdud->pidl) {
            LocalFree((HLOCAL)pdud);
            return;
        }
        pdud->fRecurse = bRecurse;
        pdud->pdudNext = _pdudFirst;
        _pdudFirst = pdud;
    }
}

void CDFCommand::_ClearDeferUpdateDirList()
{
    // Cancel any Pending updatedirs also.    
    while (_pdudFirst) {
        struct DeferUpdateDir *pdud = _pdudFirst;
        _pdudFirst = pdud->pdudNext;
        ILFree(pdud->pidl);
        LocalFree((HLOCAL)pdud);
    }
}



LRESULT CALLBACK CDFCommand::s_ThreadNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDFCommand* pThis = (CDFCommand*)GetWindowLongPtr(hwnd, 0);
    LRESULT lRes = 0L;
    
    if (uMsg < WM_USER)
        return(::DefWindowProc(hwnd, uMsg, wParam, lParam));
    else    
    {
        switch (uMsg) {
        case WM_DF_FSNOTIFY:
            {
                LPSHChangeNotificationLock  pshcnl;
                LPITEMIDLIST *ppidl;
                LONG lEvent;
    
                pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
                if (pshcnl)
                {
                    pThis->_handleFSChange(lEvent, ppidl);
                    SHChangeNotification_Unlock(pshcnl);
                }
            }
            break;
            
        case WM_DF_SEARCHPROGRESS:
            pThis->_PTN_SearchProgress();
            pThis->Release();
            break;

        case WM_DF_ASYNCPROGRESS:
            pThis->_PTN_AsyncProgress((int)wParam, (DWORD)lParam);
            pThis->Release();
            break;

        case WM_DF_SEARCHSTART:
            pThis->_cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_START);
            pThis->_SetEmptyText(IDS_FINDVIEWEMPTYBUSY);
            pThis->Release();
            break;

        case WM_DF_SEARCHCOMPLETE:
            pThis->_PTN_SearchComplete((HRESULT)wParam, (BOOL)lParam);
            pThis->Release();
            break;
        case WM_DF_ASYNCTOSYNC:
            pThis->_PTN_AsyncToSync();
            pThis->Release();
            break;
        }
    }
    return lRes;
}    


DWORD CALLBACK CDFCommand::_Execute_ThreadProc(LPVOID lpThreadParams)
{
    HRESULT                     hr;
    LPITEMIDLIST                pidl;
    INT                         state;
    INT                         cFoldersSearched = 0;
    INT                         cFoldersSearchedPrev;
    INT                         cItemsSearched;
    ExecThreadParams            *pParams = ((ExecThreadParams *)lpThreadParams);
    CDFCommand                  *&pThis = pParams->pThis;
    BOOL                        fAbort = FALSE;
    BOOL                        fQueryIsAsync;

    EnterCriticalSection(&pThis->_csThread);
    // previous thread might have exited but we're still processing search complete message
    if (pThis->_cExecInProgress > 1) 
        Sleep(1000); // give it a chance to finish

    // Don't have the enum bring up error dialog on trying to hit a: drive...
    SetErrorMode(SEM_FAILCRITICALERRORS);

    if (EVAL(SUCCEEDED(hr = CoInitialize(NULL)))) 
    {
        TraceMsg(TF_DOCFIND, "CDFCommand: starting exec.\n");
        pThis->_updateParams.hdpa = NULL;
        pThis->_updateParams.fFilesAdded = FALSE;
        pThis->_updateParams.fDirChanged = FALSE;
        pThis->_updateParams.fUpdatePosted = FALSE;

        pThis->_PostMessage(WM_DF_SEARCHSTART, 0, 0);

        // Now see if this is an Sync or an Async version of the search...

        if (fQueryIsAsync = pParams->pdfenum->FQueryIsAsync())
        {
            DBCOUNTITEM dwTotalAsync;
            BOOL fDone;
            int nPercentComplete;
            while((hr = pParams->pdfenum->GetAsyncCount(&dwTotalAsync, &nPercentComplete, &fDone)==S_OK))
            {
                if (!pThis->_fContinue) 
                {
                    TraceMsg(TF_DOCFIND, "CDFCommand: cancel exec.\n");
                    fAbort = TRUE;
                    break;
                }

                pThis->_PostMessage(WM_DF_ASYNCPROGRESS, (WPARAM)nPercentComplete, (LPARAM)dwTotalAsync);

                // If we are done we can simply let the ending callback tell of the new count...
                if (fDone) 
                {
                    TraceMsg(TF_DOCFIND, "CDFCommand: done exec.\n");
                    break;
                }

                DWORD dwSleep = 300;
                if (pThis->_execData.pdfFolder)
                {
                    if (pThis->_execData.pdfFolder->IsSlow())
                        dwSleep = 3000;
                }
                Sleep(dwSleep); // wait between looking again...
            }
        }

        // 42 is special value to say mixed query...
        if (!fQueryIsAsync || (fQueryIsAsync == DF_QUERYISMIXED))
        {
            // this is not necessary -- just slows down the transition
            // from ci to grep search
#if 0
            if (fQueryIsAsync == DF_QUERYISMIXED)
            {
                pThis->_PostMessage(WM_DF_ASYNCTOSYNC, 0, 0);
            }
#endif

            cFoldersSearchedPrev = 0;
            pThis->_updateParams.hdpa = pThis->_hdpaItemsToAdd1;    // Assume first one now...
            pThis->_updateParams.dwTimeLastNotify = GetTickCount();

            while (((hr = pParams->pdfenum->Next(&pidl, &cItemsSearched, &cFoldersSearched, &pThis->_fContinue, &state, NULL))==S_OK)) {
                if (state == GNF_DONE) 
                {   // we ran out of people to search
                    TraceMsg(TF_DOCFIND, "CDFCommand: done exec.\n");
                    break;
                }

                if (!pThis->_fContinue) 
                {                        
                    fAbort = TRUE;
                    TraceMsg(TF_DOCFIND, "CDFCommand: cancel exec.\n");
                    break;
                }

                // See if we should abort
                if (state == GNF_MATCH)
                {   
                    // BUGBUG:: need to handle maximum number case...
                    EnterCriticalSection(&pThis->_updateParams.csSearch);
                    DPA_AppendPtr(pThis->_updateParams.hdpa, pidl);
                    pThis->_updateParams.fFilesAdded = TRUE;
                    LeaveCriticalSection(&pThis->_updateParams.csSearch);
                }
                if (cFoldersSearchedPrev != cFoldersSearched){
                    pThis->_updateParams.fDirChanged = TRUE;
                    cFoldersSearchedPrev = cFoldersSearched;
                }

                if (!pThis->_updateParams.fUpdatePosted 
                && (pThis->_updateParams.fDirChanged || pThis->_updateParams.fFilesAdded)) 
                {
                    if (GetTickCount() > (pThis->_updateParams.dwTimeLastNotify + 200)) 
                    {
                        pThis->_updateParams.fUpdatePosted = TRUE;
                        pThis->_PostMessage(WM_DF_SEARCHPROGRESS, 0, 0);
                    }
                }
            }

            if (!pThis->_updateParams.fUpdatePosted && pThis->_updateParams.fFilesAdded) 
            {
                pThis->_updateParams.fUpdatePosted = TRUE;
                pThis->_PostMessage(WM_DF_SEARCHPROGRESS, 0, 0);
            }
        }

        if (hr != S_OK) 
        {
            TraceMsg(TF_DOCFIND, "CDFCommand: cancel exec.\n");
            fAbort = TRUE;
        }

        CoUninitialize();
    }

    pThis->_PostMessage(WM_DF_SEARCHCOMPLETE, (WPARAM)hr, (LPARAM)fAbort);

    pThis->_fContinue = TRUE;
    LeaveCriticalSection(&pThis->_csThread);
    _ExecParams_Free(pParams);
    
    return (DWORD)hr;
}

STDMETHODIMP CDFCommand::_Execute_Cancel()
{
    _ClearDeferUpdateDirList();

    if (DSA_GetItemCount(_hdsaConstraints) == 0) {
        _fContinue = FALSE; // Cancel current query if we have a null paramter collection
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP CDFCommand::_Execute_Init(ExecThreadParams **ppParams, int iCol, LPITEMIDLIST pidlUpdate)
{
    HRESULT             hr;
    IDocFindFileFilter  *pdfff;
    DWORD               dwFlags = 0;

    *ppParams = new ExecThreadParams;
    if (!(*ppParams))
        return E_OUTOFMEMORY;

    ExecThreadParams *&pParams = *ppParams;

    // Clear any previous registrations...
    SHChangeNotifyDeregisterWindow(_updateParams.hwndThreadNotify);

    //
    // Prepare to execute the query
    //
    hr = _execData.pdfFolder->GetDocFindFilter(&pdfff);
    if (SUCCEEDED(hr)) 
    {
        // We do not need to update the filter if this is done as part of an FSNOTIFY or a Sort...
        if ((iCol >= 0) || pidlUpdate || SUCCEEDED(hr = _UpdateFilter(pdfff))) 
        {
            _execData.szProgressText[0] = 0; 
            pdfff->DeclareFSNotifyInterest(_updateParams.hwndThreadNotify, WM_DF_FSNOTIFY);
            pdfff->GetStatusMessageIndex(0, &_uStatusMsgIndex);
            hr = pdfff->PrepareToEnumObjects(&dwFlags);
            if (SUCCEEDED(hr)) 
            {
                hr = pdfff->EnumObjects(_execData.psf, pidlUpdate, dwFlags, iCol, 
                        _execData.szProgressText, SAFECAST_IROWSETTWATCHNOTIFY(this), &(pParams->pdfenum));
            }
        }
        pdfff->Release();
    }

    //
    // Fill in the exec params
    //

    pParams->pThis = this;
    AddRef();   // ExecParams_Free will release this interface addref...

    if (FAILED(hr)) 
    {
        _ExecParams_Free(pParams);        
        *ppParams = NULL;
    } 

    return hr;
}

STDMETHODIMP CDFCommand::_ExecParams_Free(ExecThreadParams *pParams)
{
    if (!pParams)
        return S_OK;

    // Don't use atomic release as this a pointer to a class not an interface, which can screw up...
    CDFCommand *pThis = pParams->pThis;
    pParams->pThis = NULL;
    pThis->Release();

    ATOMICRELEASE(pParams->pdfenum);

    delete pParams;
    
    return S_OK;
}

STDMETHODIMP CDFCommand::_ExecData_Release()
{
    ATOMICRELEASE(_execData.psf);
    ATOMICRELEASE(_execData.psfv);
    if (_execData.pdfFolder)
        _execData.pdfFolder->SetControllerNotifyObject(NULL);   // release back pointer to us...
    ATOMICRELEASE(_execData.pdfFolder);
    _cExecInProgress = 0; // we must be in process of shutting down at least...
    
    return S_OK;
}


HRESULT CDFCommand::_ExecData_ValidateRightSearchResults(IShellFolder *psf)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlFolder;
    if (S_OK == SHGetIDListFromUnk((IUnknown *)psf, &pidlFolder))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(_GetSearchIDList(&pidl)))
        {
            if (ILIsEqual(pidlFolder, pidl))
                hr = S_OK;
            ILFree(pidl);
        }
        ILFree(pidlFolder);
    }
    return hr;
}

STDMETHODIMP CDFCommand::_ExecData_Init()
{
    // OK, Make sure we are navigated to the search results...
    // NavigateToSearchResults();

    _ExecData_Release();

    IShellBrowser  *psb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);
    if (SUCCEEDED(hr)) 
    {
        IShellView *psv;
        hr = psb->QueryActiveShellView(&psv);
        if (SUCCEEDED(hr)) 
        {
            ATOMICRELEASE(_pdvResultsTargetFrame);

            hr = psv->QueryInterface(IID_IDefViewFrame, (void **)&_pdvResultsTargetFrame);
            if (SUCCEEDED(hr)) 
            {
                IShellFolder *psf;
                hr = _pdvResultsTargetFrame->GetShellFolder(&psf);
                if (SUCCEEDED(hr)) 
                {
                    IDocFindFolder *pdfFolder;
                    hr = psf->QueryInterface(IID_IDocFindFolder, (void **)&pdfFolder);
                    if (SUCCEEDED(hr)) 
                    {
                        hr = _ExecData_ValidateRightSearchResults(psf);
                        if (SUCCEEDED(hr)) 
                        {
                            IShellFolderView *psfv;
                            hr = _pdvResultsTargetFrame->QueryInterface(IID_IShellFolderView, (void **)&psfv);
                            if (SUCCEEDED(hr)) 
                            {
                                IUnknown_Set((IUnknown **)&_execData.pdfFolder, pdfFolder);
                                IUnknown_Set((IUnknown **)&_execData.psf, psf);
                                IUnknown_Set((IUnknown **)&(_execData.psfv), psfv);
                                _execData.pdfFolder->SetControllerNotifyObject(SAFECAST(this, IDocFindControllerNotify*));
                                psfv->Release();
                            }
                        }
                        pdfFolder->Release();
                    }
                    psf->Release();
                }
            }
            psv->Release();
        }
        psb->Release();
    }                    

    if (FAILED(hr))
        _ExecData_Release();
    else
        SetShellFolder(_execData.psf);
    
    return hr;
}

BOOL CDFCommand::_Execute_SetupBrowserCP()
{
    HRESULT hr;
    if (!_dwCookie) {
        _cwbe.SetOwner(this);   // make sure our owner is set...

        IServiceProvider *pspTLB;
        IConnectionPointContainer *pcpc;

        // OK now lets register ourself with the Defview to get any events that they may generate...
        if (SUCCEEDED(hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, 
                IID_IServiceProvider, (void**)&pspTLB))) {
            if (SUCCEEDED(hr = pspTLB->QueryService(IID_IExpDispSupport, IID_IConnectionPointContainer, (void **)&pcpc))) {
                hr = ConnectToConnectionPoint(SAFECAST(&_cwbe,IDispatch*), DIID_DWebBrowserEvents2,
                                              TRUE, pcpc, &_dwCookie, &_pcpBrowser);
                pcpc->Release();
            }
            pspTLB->Release();
        }
    }
    if (_dwCookie) {
        _cwbe.SetWaiting(TRUE);
        return TRUE;
    }

    return FALSE;
}

STDMETHODIMP CDFCommand::_Execute_Start(BOOL fNavigateIfFail, int iCol, LPITEMIDLIST pidlUpdate)
{
    HRESULT             hr;
    ExecThreadParams    *pExecThreadParams;
    HANDLE              hThread;
    ULONG               lThreadId;

    if (!_hdpaItemsToAdd1) 
    {    // First time through...
        _hdpaItemsToAdd1 = DPA_CreateEx(64, GetProcessHeap());
        if (!_hdpaItemsToAdd1)
            return E_OUTOFMEMORY;
    }

    if (!_hdpaItemsToAdd2) 
    {    // First time through...
        _hdpaItemsToAdd2 = DPA_CreateEx(64, GetProcessHeap());
        if (!_hdpaItemsToAdd2)
            return E_OUTOFMEMORY;
    }

    if (!_updateParams.hwndThreadNotify) 
    {
        _updateParams.hwndThreadNotify = SHCreateWorkerWindow(s_ThreadNotifyWndProc, NULL, 0, 0, (HMENU)0, this);
        if (!_updateParams.hwndThreadNotify) 
            return E_OUTOFMEMORY;
    }

    if (FAILED(hr = _ExecData_Init())) 
    {
        if (fNavigateIfFail) 
        {
            if (_Execute_SetupBrowserCP())
                NavigateToSearchResults();
        }
        return hr;
    }

    if (SUCCEEDED(hr = _Execute_Init(&pExecThreadParams, iCol, pidlUpdate))) 
    {
        // See if we should be saving away the selection...
        if (iCol >= 0)
            _execData.pdfFolder->RememberSelectedItems();

        // If this is an update then we need to remember our IDList else clear list...
        if (pidlUpdate) 
        {
            _pidlUpdate = ILClone(pidlUpdate);
        } 
        else 
        {
        // Tell defview to delete everything. - Use our Clear function to save code
            _Clear();
        }

        _execData.pdfFolder->SetAsyncEnum(pExecThreadParams->pdfenum);

        // Start the query
        _cExecInProgress++;
        // _fContinue = TRUE;went exiting threads should set this to true
        _fNew = FALSE;
    
        if (hThread = CreateThread(NULL, 0, _Execute_ThreadProc, pExecThreadParams, 0, &lThreadId)) 
        {     
            hr = S_OK;
            CloseHandle(hThread);
        } 
        else 
        {
            _cExecInProgress--;
            _ExecParams_Free(pExecThreadParams);
            _SetEmptyText( IDS_FINDVIEWEMPTY ) ;
        }
    }
    else
        hr = _SetLastError(hr);

    return hr; 
}

HRESULT CDFCommand::_SetLastError(HRESULT hr) 
{
    if (HRESULT_FACILITY(hr) == FACILITY_SEARCHCOMMAND) {
        _hrLastError = hr;
        hr = S_FALSE; // Don't error out script...
        _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_ERROR);
    }
    return hr;
}

STDMETHODIMP CDFCommand::Execute(VARIANT *RecordsAffected, VARIANT *Parameters, long Options)
{
    if (Options == 0)
        return _Execute_Cancel();

    return _Execute_Start(TRUE, -1, NULL);
}

//
// IConnectionPointContainer implementation
//

STDMETHODIMP CDFCommand::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, _cpEvents.CastToIConnectionPoint());

}

STDMETHODIMP CDFCommand::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT *ppCP)
{
    ASSERT( ppCP );

    if (!ppCP)
        return E_POINTER;

    if (IsEqualIID(iid, DIID_DSearchCommandEvents) || IsEqualIID(iid, IID_IDispatch)) {
        *ppCP = _cpEvents.CastToIConnectionPoint();
    } else {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    (*ppCP)->AddRef();
    return S_OK;
}


//
// IProvideClassInfo2 methods
//

STDMETHODIMP CDFCommand::GetClassInfo(ITypeInfo **ppTI)
{
    return GetTypeInfoFromLibId(0, LIBID_Shell32, 1, 0, CLSID_DocFindCommand, ppTI);
}

STDMETHODIMP CDFCommand::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
    ASSERT(pGUID);

    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID) {
        *pGUID = DIID_DSearchCommandEvents;
        return S_OK;
    }
    
    *pGUID = GUID_NULL;
    return E_FAIL;
}


STDMETHODIMP CDFCommand::SetSite(IUnknown *punkSite)
{
    if (!punkSite) 
    {
        if (!_cExecInProgress) 
        {
            _ExecData_Release();
        }
        _fContinue = FALSE; // Cancel existing queries

        // See if we have a connection point... If so unadvise now...
        if (_dwCookie) 
        {
            _pcpBrowser->Unadvise(_dwCookie);
            ATOMICRELEASE(_pcpBrowser);
            _dwCookie = 0;
        }

        // Bug #199671
        // Trident won't call UnAdvise and they except ActiveX Controls
        // to use IOleControl::Close() to do their own UnAdvise, and hope
        // nobody will need events after that.  I don't impl IOleControl so
        // we need to do the same thing during IObjectWithSite::SetSite(NULL)
        // and hope someone won't want to reparent us.  This is awkward but
        // saves Trident some perf so we will tolerate it.
        EVAL(SUCCEEDED(_cpEvents.UnadviseAll()));
    }

    return CObjectWithSite::SetSite(punkSite);
}

void CDFCommand::_SelectResults()
{
    if( _execData.psfv )
    {
        //  If there are any items...
        UINT cItems = 0;
        if( SUCCEEDED(_execData.psfv->GetObjectCount( &cItems )) && cItems > 0 )
        {
            IShellView* psv;
            if( SUCCEEDED(_execData.psfv->QueryInterface( IID_IShellView, (void**)&psv )) )
            {
                //  If none are selected (don't want to rip the user's selection out of his hand)...
                UINT cSelected = 0;
                if( SUCCEEDED(_execData.psfv->GetSelectedCount( &cSelected )) && cSelected == 0 )
                {
                    //  Retrieve the pidl for the first item in the list...
                    LPITEMIDLIST pidlFirst = NULL;
                    if( SUCCEEDED(_execData.psfv->GetObject( &pidlFirst,  0 )) )
                    {
                        //  Give it the focus
                        psv->SelectItem( pidlFirst, SVSI_FOCUSED | SVSI_ENSUREVISIBLE );
                    }
                }

                //  Activate the view.
                psv->UIActivate( SVUIA_ACTIVATE_FOCUS );
                psv->Release();
            }
        }
    }
}

STDMETHODIMP CDFCommand::ClearResults(void)
{
    HRESULT hr = _Clear() ;   

    if (SUCCEEDED(hr))
    {
        _fNew = TRUE;
        _SetEmptyText(IDS_FINDVIEWEMPTYINIT);
    }

    return hr ;
}

STDMETHODIMP CDFCommand::_Clear()
{
    // Tell defview to delete everything.
    UINT u;
    if (_execData.psfv)
        _execData.psfv->RemoveObject(NULL, &u);

    // And cleanup our folderList
    if (_execData.pdfFolder)
    {
        _execData.pdfFolder->ClearItemList();
        _execData.pdfFolder->ClearFolderList();
    }
    return S_OK;
}


HRESULT CDFCommand::_SetEmptyText( UINT nIDEmptyText )
{
    if( _execData.pdfFolder )
    {
        TCHAR szEmptyText[128] ;
        EVAL( LoadString( HINST_THISDLL, nIDEmptyText, 
                          szEmptyText, ARRAYSIZE(szEmptyText) ) ) ;
        return _execData.pdfFolder->SetEmptyText( szEmptyText ) ;
    }
    return E_FAIL ;
}


HRESULT CDFCommand::_GetSearchIDList(LPITEMIDLIST *ppidl)
{
    return SHILCreateFromPath(_SearchForComputer() ? 
        TEXT("::{1f4de370-d627-11d1-ba4f-00a0c91eedba}")  : // CLSID_ComputerFindFolder
        TEXT("::{e17d4fc0-5564-11d1-83f2-00a0c90dc849}"),   // CLSID_DocFindFolder
        ppidl, NULL);
}

STDMETHODIMP CDFCommand::NavigateToSearchResults(void)
{
    IShellBrowser  *psb;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);
    if (SUCCEEDED(hr)) 
    {
        LPITEMIDLIST pidl;
        hr = _GetSearchIDList(&pidl);
        if (SUCCEEDED(hr))
        {
            hr = psb->BrowseObject(pidl,  SBSP_SAMEBROWSER | SBSP_ABSOLUTE | SBSP_WRITENOHISTORY);
            ILFree(pidl);
        }
        psb->Release();
    }
    return hr;
}

IUnknown* CDFCommand::_GetObjectToPersist()
{
    IUnknown *punk = NULL;
    
    // We could grovel immediately above our CDFCommand for the object to persist,
    // or go to the top and walk down one level.  That is a tad more robust just
    // in case our cdfcommand object is within a frame
    //
    IShellBrowser * psbFrame;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopFrameBrowser, IID_IShellBrowser, (void**)&psbFrame)))
    {
        IShellView* psv;

        if (SUCCEEDED(psbFrame->QueryActiveShellView(&psv)))
        {
            psv->GetItemObject(SVGIO_BACKGROUND, IID_IOleObject, (void**)&punk);
            psv->Release();
        }

        psbFrame->Release();
    }

    return punk;
}

void CDFCommand::_PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{    
    AddRef();  // to be released after processing of the message bellow
    if (!PostMessage(_updateParams.hwndThreadNotify, uMsg, wParam, lParam))
    {
        Release();
    }
}

STDMETHODIMP CDFCommand::SaveSearch(void)
{
    IDocFindFileFilter * pdfff;
    HRESULT hr = _execData.pdfFolder->GetDocFindFilter(&pdfff);
    if (SUCCEEDED(hr))
    {
        IShellBrowser* psb;
        hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb);
        if (SUCCEEDED(hr))
        {
            IShellView* psv;
            hr = psb->QueryActiveShellView(&psv);
            if (SUCCEEDED(hr))
            {
                HWND hwnd;
                IUnknown* punk;

                IUnknown_GetWindow(_punkSite, &hwnd);

                punk = _GetObjectToPersist();

                DFB_Save(pdfff, hwnd, &_dfbsi, psv, _execData.pdfFolder, punk);

                ATOMICRELEASE(punk);
                psv->Release();
            }
            psb->Release();
        }
        pdfff->Release();
    }

    return hr;
}


STDMETHODIMP CDFCommand::RestoreSearch(void)
{
    // let script know that a restore happened...
    _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_RESTORE);
    return S_OK;
}

STDMETHODIMP CDFCommand::StopSearch(void)
{
    if (_cExecInProgress)
        return _Execute_Cancel();

    return S_OK;
}

STDMETHODIMP CDFCommand::GetItemCount(UINT *pcItems)
{
    ASSERT(pcItems);
    if (_execData.psfv)
    {
        return _execData.psfv->GetObjectCount(pcItems);
    }
    return E_FAIL;
}

STDMETHODIMP CDFCommand::SetItemCount(UINT cItems)
{
    if (_execData.psfv)
    {
        return _execData.psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
    }
    return E_FAIL;
}

STDMETHODIMP CDFCommand::ViewDestroyed()
{
    _ExecData_Release();
    return S_OK;
}

STDMETHODIMP CDFCommand::get_ProgressText(BSTR *pbs)
{

    *pbs = SysAllocStringT(_szProgressText);
    return *pbs ? S_OK : E_OUTOFMEMORY;
}

//------ error string mappings ------//
static const UINT error_strings[] =
{
    SCEE_CONSTRAINT,   IDS_DOCFIND_CONSTRAINT,
    SCEE_PATHNOTFOUND, IDS_DOCFIND_PATHNOTFOUND,
    SCEE_INDEXSEARCH,  IDS_DOCFIND_SCOPEERROR,
    SCEE_CASESENINDEX, IDS_DOCFIND_CI_NOT_CASE_SEN,
};

STDMETHODIMP CDFCommand::GetErrorInfo(BSTR *pbs,  int *phr)
{
    int     nCode     = HRESULT_CODE(_hrLastError);
    UINT    uSeverity = HRESULT_SEVERITY(_hrLastError);

    if( phr )
        *phr = nCode;
    
    if (pbs)
    {    
        UINT nIDString = 0;
        *pbs = NULL;

        for( int i=0; i<ARRAYSIZE(error_strings); i+=2 )
        {
            if( error_strings[i] == (UINT)nCode )
            {
                nIDString =  error_strings[i+1];
                break ;
            }
        }

        if( nIDString )
        {
            WCHAR wszMsg[MAX_PATH];
            EVAL(LoadStringW( HINST_THISDLL, nIDString, wszMsg, ARRAYSIZE(wszMsg) ));
            *pbs = SysAllocString( wszMsg );
        }
        else
            *pbs = SysAllocString( L"" );
    }
    
    return S_OK;
}


STDMETHODIMP CDFCommand::SearchFor(int iFor)
{
    _iSearchFor = iFor;
    return S_OK;
}

STDMETHODIMP CDFCommand::GetScopeInfo(BSTR bsScope, int *pdwScopeInfo)
{
    *pdwScopeInfo = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CDFCommand::RestoreSavedSearch( IN VARIANT *pvarFile )
{
    if( pvarFile != NULL )
    {
        if( pvarFile->vt != VT_EMPTY )
        {
            LPITEMIDLIST pidl = VariantToIDList( pvarFile ) ; 
            if( pidl )
            {
                ILFree( _pidlRestore ) ;
                _pidlRestore = pidl ;
            }
        }
        VariantClear( pvarFile ) ;
    }

    if( _pidlRestore )
    {
        IShellBrowser  *psb;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb))) 
        {
            // Warning:: We check for shell view simply to see how the search pane was
            // loaded.  If this fails it is because we were loaded on the CoCreateInstance
            // of the browser window and as such it is a race condition to know if the
            // properties were set or not.  So in this case wait until we get a 
            // navigate complete.  This lets us know for sure if a save file was passed
            // in or not.
            IShellView     *psv;
            if (SUCCEEDED(psb->QueryActiveShellView(&psv))) 
            {
                psv->Release();

                IWebBrowser2* pwb;
                if (EVAL(SUCCEEDED(IUnknown_QueryService(psb, SID_SWebBrowserApp, 
                        IID_IWebBrowser2, (void **) &pwb))))
                {
                    if (SUCCEEDED(_ExecData_Init()))
                    {
                        _execData.pdfFolder->RestoreSearchFromSaveFile(_pidlRestore, _execData.psfv);
                        _cpEvents.InvokeDispid(DISPID_SEARCHCOMMAND_RESTORE);
                        ILFree( _pidlRestore ) ;
                        _pidlRestore = NULL ;

                    }
                    pwb->Release();
                }
            }
            else
            {
                // appears to be race condition to load
                if (!_fDeferRestoreTried)
                {
                    TraceMsg(TF_WARNING, "CDFCommand::MaybeRestoreSearch - QueryActiveShellView failed...");
                    _fDeferRestore = TRUE;
                    if (!_Execute_SetupBrowserCP())
                        _fDeferRestore = FALSE;
                }
            }
            psb->Release();
        }
    }
    return S_OK;
}

STDMETHODIMP CDFCommand::OnChange(IRowset *prowset, DBWATCHNOTIFY eChangeReason)
{
    _fAsyncNotifyReceived = TRUE;
    return S_OK;
}

STDMETHODIMP CDFCommand::DoSortOnColumn(UINT iCol, BOOL fSameCol)
{

    IDFEnum *pdfEnumAsync;

    if (SUCCEEDED(_execData.pdfFolder->GetAsyncEnum(&pdfEnumAsync)) && pdfEnumAsync)
    {
        // If the search is still running we will restart with the other column else we
        // will make sure all of the items have been cached and let the default processing happen
        if (!fSameCol  && _cExecInProgress)
        {
            // We should try to sort on the right column...
            _Execute_Start(FALSE, iCol, NULL);
            return S_FALSE; // tell system to not do default processing.
        }

        _execData.pdfFolder->CacheAllAsyncItems();
    }
    return S_OK;    // let it do default processing.

}

// Implemention of our IDispatch to hookup to the top level browsers connnection point...
STDMETHODIMP CDFCommand::CWBEvents2::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    if ( riid == IID_IUnknown || riid == IID_IDispatch || riid == DIID_DWebBrowserEvents2
         || riid == DIID_DWebBrowserEvents){
        *ppvObj = (LPVOID)this;
        AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }
    return NOERROR;
}

STDMETHODIMP CDFCommand::CWBEvents2::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    if (_fWaitingForNavigate) {
        TraceMsg(TF_WARNING, "CDFCommand::CWBEvents2::Invoke dispid=%d.",dispidMember);
        if ((dispidMember == DISPID_NAVIGATECOMPLETE) || (dispidMember == DISPID_DOCUMENTCOMPLETE)) {
            // Assume this is ours... Should maybe check parameters...
            _fWaitingForNavigate = FALSE;

            // Now see if it is a case where we are to restore the search...
            if (_pcdfc->_fDeferRestore)
            {
                _pcdfc->_fDeferRestore = FALSE;
                _pcdfc->_fDeferRestoreTried = TRUE;
                _pcdfc->RestoreSavedSearch( NULL );
            }
            else
                return _pcdfc->_Execute_Start(FALSE, -1, NULL);
        }
    }
    return S_OK;
}

//---------------------------------------------------------------------------------
// Now implement our simple constraint object
//
// CDFConstraint ctor/dtor implementation
//

CDFConstraint::CDFConstraint(BSTR bstr, VARIANT var) 
: CImpIDispatch(&LIBID_Shell32, 1, 0, &IID_DFConstraint)
{
    _cRef       = 1;
    _bstr = bstr;
    _var = var;
}

CDFConstraint::~CDFConstraint()
{
    SysFreeString(_bstr);
    VariantClear(&_var);
}

//
// CDFParameter IUnknown implementation
//

STDMETHODIMP CDFConstraint::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDFConstraint, DFConstraint),                  // IID_DFConstraint
        QITABENTMULTI(CDFConstraint, IDispatch, DFConstraint),  //IID_IDispatch
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDFConstraint::AddRef()
{
    InterlockedIncrement(&_cRef);
    TraceMsg(TF_DOCFIND, "CDFConstraint.AddRef %d",_cRef);
    return _cRef;
}

STDMETHODIMP_(ULONG) CDFConstraint::Release()
{
    TraceMsg(TF_DOCFIND, "CDFConstraint.Release %d",_cRef-1);
    if (InterlockedDecrement(&_cRef)) {
        return _cRef;
    }
    delete this;
    return 0;
}

STDMETHODIMP CDFConstraint::GetTypeInfoCount(UINT * pctinfo)
{ 
    return CImpIDispatch::GetTypeInfoCount(pctinfo); 
}

STDMETHODIMP CDFConstraint::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo)
{ 
    return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); 
}

STDMETHODIMP CDFConstraint::GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{ 
    return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CDFConstraint::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

STDMETHODIMP CDFConstraint::get_Name(BSTR *pbs)
{
    *pbs = SysAllocString(_bstr);
    return *pbs? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDFConstraint::get_Value(VARIANT *pvar)
{
    VariantInit(pvar);
    return VariantCopy(pvar, &_var);
}
