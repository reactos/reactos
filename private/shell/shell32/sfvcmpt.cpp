#include "shellprv.h"
#include <shellp.h>
#include <sfview.h>
#include "sfviewp.h"

//
//  punkFolder is an object that ultimately supports IShellFolder.
//
CBaseShellFolderViewCB::CBaseShellFolderViewCB(IUnknown* punkFolder, LPCITEMIDLIST pidl, LONG lEvents)
    : m_cRef(1), m_hwndMain(NULL), m_lEvents(lEvents)
{
    // Use QueryInterface instead of IUnknown_Set for two reasons.
    //
    // 1. Helps us track leaks with QISTUB.
    // 2. To make sure we really have an IShellFolder.
    //
    EVAL(SUCCEEDED(punkFolder->QueryInterface(IID_IShellFolder, (LPVOID*)&m_pshf)));

    m_pidl = pidl ? ILClone(pidl) : NULL;   // Bitbuck1.cpp passes NULL!
}

CBaseShellFolderViewCB::~CBaseShellFolderViewCB()
{
    if (m_pshf)
        m_pshf->Release();

    if (m_pidl)
        ILFree((LPITEMIDLIST)m_pidl);
}


STDMETHODIMP CBaseShellFolderViewCB::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CBaseShellFolderViewCB, IShellFolderViewCB),   // IID_IShellFolderViewCB
        QITABENT(CBaseShellFolderViewCB, IObjectWithSite),      // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CBaseShellFolderViewCB::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CBaseShellFolderViewCB::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CBaseShellFolderViewCB::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = RealMessage(uMsg, wParam, lParam);
    if (SUCCEEDED(hres))
        return hres;

    switch (uMsg)
    {
    case SFVM_HWNDMAIN:
        m_hwndMain = (HWND)lParam;
        break;

    case SFVM_GETNOTIFY:
        *(LPCITEMIDLIST*)wParam = m_pidl;
        *(LONG*)lParam = m_lEvents;
        break;

    default:
        return hres;
    }

    return NOERROR;
}


class CWrapOldCallback : public CBaseShellFolderViewCB
{
public:
    CWrapOldCallback(LPCSFV pcsfv);

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    //*** IObjectWithSite overload ***
    virtual STDMETHODIMP SetSite(IUnknown *punkSite);

private:
//#define TEST_MACROS
#ifdef TEST_MACROS
    STDMETHODIMP TestMacros(UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT TestMergeMenu(DWORD pv, QCMINFO*lP) {OutputDebugString(TEXT("TestMergeMenu\r\n"));return(NOERROR);}
    HRESULT TestInvokeCommand(DWORD pv, UINT wP) {OutputDebugString(TEXT("TestInvokeCommand\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetHelpText(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP) {OutputDebugString(TEXT("TestGetHelpText\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetTooltipText(DWORD pv, UINT wPl, UINT wPh, LPTSTR lP) {OutputDebugString(TEXT("TestGetTooltipText\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetButtonInfo(DWORD pv, TBINFO*lP) {OutputDebugString(TEXT("TestGetButtonInfo\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetButtons(DWORD pv, UINT wPl, UINT wPh, TBBUTTON*lP) {OutputDebugString(TEXT("TestGetButtons\r\n"));return(E_NOTIMPL);}
    HRESULT TestInitMenuPopup(DWORD pv, UINT wPl, UINT wPh, HMENU lP) {OutputDebugString(TEXT("TestInitMenuPopup\r\n"));return(E_NOTIMPL);}
    HRESULT TestSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP) {OutputDebugString(TEXT("TestSelChange\r\n"));return(E_NOTIMPL);}
    HRESULT TestDrawItem(DWORD pv, UINT wP, DRAWITEMSTRUCT*lP) {OutputDebugString(TEXT("TestDrawItem\r\n"));return(E_NOTIMPL);}
    HRESULT TestMeasureItem(DWORD pv, UINT wP, MEASUREITEMSTRUCT*lP) {OutputDebugString(TEXT("TestMeasureItem\r\n"));return(E_NOTIMPL);}
    HRESULT TestExitMenuLoop(DWORD pv) {OutputDebugString(TEXT("TestExitMenuLoop\r\n"));return(E_NOTIMPL);}
    HRESULT TestPreRelease(DWORD pv) {OutputDebugString(TEXT("TestPreRelease\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetCCHMax(DWORD pv, LPCITEMIDLIST wP, UINT*lP) {OutputDebugString(TEXT("TestGetCCHMax\r\n"));return(E_NOTIMPL);}
    HRESULT TestFSNotify(DWORD pv, LPCITEMIDLIST*wP, LONG lP) {OutputDebugString(TEXT("TestFSNotify\r\n"));return(E_NOTIMPL);}
    HRESULT TestWindowCreated(DWORD pv, HWND wP) {OutputDebugString(TEXT("TestWindowCreated\r\n"));return(E_NOTIMPL);}
    HRESULT TestWindowDestroy(DWORD pv, HWND wP) {OutputDebugString(TEXT("TestWindowDestroy\r\n"));return(E_NOTIMPL);}
    HRESULT TestRefresh(DWORD pv, BOOL wP) {OutputDebugString(TEXT("TestRefresh\r\n"));return(E_NOTIMPL);}
    HRESULT TestSetFocus(DWORD pv) {OutputDebugString(TEXT("TestSetFocus\r\n"));return(E_NOTIMPL);}
    HRESULT TestQueryCopyHook(DWORD pv) {OutputDebugString(TEXT("TestQueryCopyHook\r\n"));return(E_NOTIMPL);}
    HRESULT TestNotifyCopyHook(DWORD pv, COPYHOOKINFO*lP) {OutputDebugString(TEXT("TestNotifyCopyHook\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetDetailsOf(DWORD pv, UINT wP, DETAILSINFO*lP) {OutputDebugString(TEXT("TestGetDetailsOf\r\n"));return(E_NOTIMPL);}
    HRESULT TestColumnClick(DWORD pv, UINT wP) {OutputDebugString(TEXT("TestColumnClick\r\n"));return(E_NOTIMPL);}
    HRESULT TestQueryFSNotify(DWORD pv, SHChangeNotifyEntry*lP) {OutputDebugString(TEXT("TestQueryFSNotify\r\n"));return(E_NOTIMPL);}
    HRESULT TestDefItemCount(DWORD pv, UINT*lP) {OutputDebugString(TEXT("TestDefItemCount\r\n"));return(E_NOTIMPL);}
    HRESULT TestDefViewMode(DWORD pv, FOLDERVIEWMODE*lP) {OutputDebugString(TEXT("TestDefViewMode\r\n"));return(E_NOTIMPL);}
    HRESULT TestUnMergeMenu(DWORD pv, HMENU lP) {OutputDebugString(TEXT("TestUnMergeMenu\r\n"));return(E_NOTIMPL);}
    HRESULT TestInsertItem(DWORD pv, LPCITEMIDLIST wP) {OutputDebugString(TEXT("TestInsertItem\r\n"));return(E_NOTIMPL);}
    HRESULT TestDeleteItem(DWORD pv, LPCITEMIDLIST wP) {OutputDebugString(TEXT("TestDeleteItem\r\n"));return(E_NOTIMPL);}
    HRESULT TestUpdateStatusBar(DWORD pv, BOOL wP) {OutputDebugString(TEXT("TestUpdateStatusBar\r\n"));return(E_NOTIMPL);}
    HRESULT TestBackgroundEnum(DWORD pv) {OutputDebugString(TEXT("TestBackgroundEnum\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetWorkingDir(DWORD pv, UINT wP, LPTSTR lP) {OutputDebugString(TEXT("TestGetWorkingDir\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetColSaveStream(DWORD pv, WPARAM wP, IStream**lP) {OutputDebugString(TEXT("TestGetColSaveStream\r\n"));return(E_NOTIMPL);}
    HRESULT TestSelectAll(DWORD pv) {OutputDebugString(TEXT("TestSelectAll\r\n"));return(E_NOTIMPL);}
    HRESULT TestDidDragDrop(DWORD pv, DWORD wP, IDataObject*lP) {OutputDebugString(TEXT("TestDidDragDrop\r\n"));return(E_NOTIMPL);}
    HRESULT TestHwndMain(DWORD pv, HWND lP) {OutputDebugString(TEXT("TestHwndMain\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetNotify(DWORD pv, LPITEMIDLIST*wP, LONG*lP) {OutputDebugString(TEXT("TestGetNotify\r\n"));return(E_NOTIMPL);}
    HRESULT TestSetISFV(DWORD pv, IShellFolderView*lP) {OutputDebugString(TEXT("TestSetISFV\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews**lP) {OutputDebugString(TEXT("TestGetViews\r\n"));return(E_NOTIMPL);}
    HRESULT TestTHISIDLIST(DWORD pv, LPITEMIDLIST*lP) {OutputDebugString(TEXT("TestTHISIDLIST\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetItemIDList(DWORD pv, UINT iItem, LPITEMIDLIST *ppidl) {OutputDebugString(TEXT("TestGetItemIDList\r\n"));return(E_NOTIMPL);}
    HRESULT TestSetItemIDList(DWORD pv, UINT iItem, LPITEMIDLIST pidl) {OutputDebugString(TEXT("TestSetItemIDList\r\n"));return(E_NOTIMPL);}
    HRESULT TestIndexOfItemIDList(DWORD pv, int * piItem, LPITEMIDLIST pidl) {OutputDebugString(TEXT("TestIndexOfItemIDList\r\n"));return(E_NOTIMPL);}
    HRESULT TestODFindItem(DWORD pv, int * piItem, NM_FINDITEM* pnmfi) {OutputDebugString(TEXT("TestODFindItem\r\n"));return(E_NOTIMPL);}
    HRESULT TestAddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA *ppagedata) {OutputDebugString(TEXT("TestAddPropertyPages\r\n"));return(E_NOTIMPL);}
    HRESULT TestArrange(DWORD v, LPARAM lp) {OutputDebugString(TEXT("TestArrange\r\n"));return(E_NOTIMPL);}
    HRESULT TestQueryStandardViews(DWORD pv, BOOL *pfAllowStandardViews) {OutputDebugString(TEXT("TestQueryStandardViews\r\n"));return(E_NOTIMPL);}
    HRESULT TestQueryReuseExtView(DWORD pv, BOOL *pfReuseAllowed) {OutputDebugString(TEXT("TestQueryReuseExtView\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetEmptyText(DWORD pv, UINT u, LPTSTR psz) {OutputDebugString(TEXT("TestGetEmptyText\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetItemIconIndex(DWORD pv, UINT iItem, int *piIcon) {OutputDebugString(TEXT("TestGetItemIconIndex\r\n"));return(E_NOTIMPL);}
    HRESULT TestSize(DWORD pv, UINT cx, UINT cy) {OutputDebugString(TEXT("TestSize\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetZone(DWORD pv, DWORD *pdwZone) {OutputDebugString(TEXT("TestGetZone\r\n"));return(E_NOTIMPL);}
    HRESULT TestGetPane(DWORD pv, DWORD dwPaneID, DWORD *pdwPane) {OutputDebugString(TEXT("TestGetPane\r\n"));return(E_NOTIMPL);}
    HRESULT TestSupportsIdentity(DWORD pv){OutputDebugString(TEXT("TestSupportsIdentity\r\n"));return(E_NOTIMPL);}
#endif // TEST_MACROS

private:
    IShellView* m_psvOuter;
    LPFNVIEWCALLBACK m_pfnCB;

    UINT m_fvm;
    LPARAM m_lSelChangeInfo;
} ;


CWrapOldCallback::CWrapOldCallback(LPCSFV pcsfv)
    : CBaseShellFolderViewCB(pcsfv->pshf, pcsfv->pidl, pcsfv->lEvents)
{
    m_psvOuter  = pcsfv->psvOuter;
    m_fvm = pcsfv->fvm;
    m_pfnCB = pcsfv->pfnCallback;
}

// Some older clients may not support IObjectWithSite::SetSite
// For compat send them the old SFVM_SETISFV message
HRESULT CWrapOldCallback::SetSite(IUnknown *punkSite)
{
    HRESULT hr = CBaseShellFolderViewCB::SetSite( punkSite );
    MessageSFVCB( SFVM_SETISFV, 0, (LPARAM)punkSite );
    return hr;
}


#ifdef TEST_MACROS
STDMETHODIMP CWrapOldCallback::TestMacros(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, TestMergeMenu);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, TestInvokeCommand);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, TestGetHelpText);
    HANDLE_MSG(0, SFVM_GETTOOLTIPTEXT, TestGetTooltipText);
    HANDLE_MSG(0, SFVM_GETBUTTONINFO, TestGetButtonInfo);
    HANDLE_MSG(0, SFVM_GETBUTTONS, TestGetButtons);
    HANDLE_MSG(0, SFVM_INITMENUPOPUP, TestInitMenuPopup);
    HANDLE_MSG(0, SFVM_SELCHANGE, TestSelChange);
    HANDLE_MSG(0, SFVM_DRAWITEM, TestDrawItem);
    HANDLE_MSG(0, SFVM_MEASUREITEM, TestMeasureItem);
    HANDLE_MSG(0, SFVM_EXITMENULOOP, TestExitMenuLoop);
    HANDLE_MSG(0, SFVM_PRERELEASE, TestPreRelease);
    HANDLE_MSG(0, SFVM_GETCCHMAX, TestGetCCHMax);
    HANDLE_MSG(0, SFVM_FSNOTIFY, TestFSNotify);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, TestWindowCreated);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, TestWindowDestroy);
    HANDLE_MSG(0, SFVM_REFRESH, TestRefresh);
    HANDLE_MSG(0, SFVM_SETFOCUS, TestSetFocus);
    HANDLE_MSG(0, SFVM_QUERYCOPYHOOK, TestQueryCopyHook);
    HANDLE_MSG(0, SFVM_NOTIFYCOPYHOOK, TestNotifyCopyHook);
    HANDLE_MSG(0, SFVM_GETDETAILSOF, TestGetDetailsOf);
    HANDLE_MSG(0, SFVM_COLUMNCLICK, TestColumnClick);
    HANDLE_MSG(0, SFVM_QUERYFSNOTIFY, TestQueryFSNotify);
    HANDLE_MSG(0, SFVM_DEFITEMCOUNT, TestDefItemCount);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, TestDefViewMode);
    HANDLE_MSG(0, SFVM_UNMERGEMENU, TestUnMergeMenu);
    HANDLE_MSG(0, SFVM_INSERTITEM, TestInsertItem);
    HANDLE_MSG(0, SFVM_DELETEITEM, TestDeleteItem);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, TestUpdateStatusBar);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, TestBackgroundEnum);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, TestGetWorkingDir);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, TestGetColSaveStream);
    HANDLE_MSG(0, SFVM_SELECTALL, TestSelectAll);
    HANDLE_MSG(0, SFVM_DIDDRAGDROP, TestDidDragDrop);
    HANDLE_MSG(0, SFVM_HWNDMAIN, TestHwndMain);
    HANDLE_MSG(0, SFVM_GETNOTIFY, TestGetNotify);
    HANDLE_MSG(0, SFVM_SETISFV, TestSetISFV);
    HANDLE_MSG(0, SFVM_GETVIEWS, TestGetViews);
    HANDLE_MSG(0, SFVM_THISIDLIST, TestTHISIDLIST);
    HANDLE_MSG(0, SFVM_GETITEMIDLIST, TestGetItemIDList);
    HANDLE_MSG(0, SFVM_SETITEMIDLIST, TestSetItemIDList);
    HANDLE_MSG(0, SFVM_INDEXOFITEMIDLIST, TestIndexOfItemIDList);
    HANDLE_MSG(0, SFVM_SUPPORTSIDENTITY, TestSupportsIdentity);
    HANDLE_MSG(0, SFVM_ODFINDITEM, TestODFindItem);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, TestAddPropertyPages);
    HANDLE_MSG(0, SFVM_ARRANGE, TestArrange);
    HANDLE_MSG(0, SFVM_QUERYSTANDARDVIEWS, TestQueryStandardViews);
    HANDLE_MSG(0, SFVM_QUERYREUSEEXTVIEW, TestQueryReuseExtView);
    HANDLE_MSG(0, SFVM_GETEMPTYTEXT, TestGetEmptyText);
    HANDLE_MSG(0, SFVM_GETITEMICONINDEX, TestGetItemIconIndex);
    HANDLE_MSG(0, SFVM_SIZE, TestSize);
    HANDLE_MSG(0, SFVM_GETZONE, TestGetZone);
    HANDLE_MSG(0, SFVM_GETPANE, TestGetPane);
    }

    return E_NOTIMPL;
}
#endif // TEST_MACROS


STDMETHODIMP CWrapOldCallback::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DVSELCHANGEINFO dvsci;
#ifdef TEST_MACROS
    TestMacros(uMsg, wParam, lParam);
#endif // TEST_MACROS

    switch (uMsg)
    {
    case SFVM_DEFVIEWMODE:
        if (m_fvm)
            *(UINT*)lParam = m_fvm;
        break;

    case SFVM_SELCHANGE:
    {
        SFVM_SELCHANGE_DATA* pSelChange = (SFVM_SELCHANGE_DATA*)lParam;

        dvsci.uNewState = pSelChange->uNewState;
        dvsci.uOldState = pSelChange->uOldState;
        dvsci.plParam = &m_lSelChangeInfo;
        dvsci.lParamItem = pSelChange->lParamItem;
        lParam = (LPARAM)&dvsci;
        break;
    }

    case SFVM_INSERTITEM:
    case SFVM_DELETEITEM:
    case SFVM_WINDOWCREATED:
        dvsci.plParam = &m_lSelChangeInfo;
        dvsci.lParamItem = lParam;
        lParam = (LPARAM)&dvsci;
        break;

    case SFVM_REFRESH:
    case SFVM_SELECTALL:
    case SFVM_UPDATESTATUSBAR:
    case SFVM_SETFOCUS:
    case SFVM_PRERELEASE:
        lParam = m_lSelChangeInfo;
        break;

    default:
        break;
    }

    // NOTE: The DVM_ messages are the same as the SFVM_ message
    return m_pfnCB(m_psvOuter, m_pshf, m_hwndMain, uMsg, wParam, lParam);
}


LRESULT _ShellFolderViewMessage(IShellFolderView* psfv, UINT uMsg, LPARAM lParam)
{
    UINT uScratch;

    switch (uMsg)
    {
    case SFVM_REARRANGE:
        psfv->Rearrange(lParam);
        break;

    case SFVM_ARRANGEGRID:
        psfv->ArrangeGrid();
        break;

    case SFVM_AUTOARRANGE:
        psfv->AutoArrange();
        break;

    case SFVM_GETAUTOARRANGE:
        return psfv->GetAutoArrange() == S_OK;

    // BUGBUG: not used?
    case SFVM_GETARRANGEPARAM:
        psfv->GetArrangeParam(&lParam);
        return lParam;

    case SFVM_ADDOBJECT:
        if (SUCCEEDED(psfv->AddObject((LPITEMIDLIST)lParam, &uScratch))
             && (int)uScratch >= 0)
        {
            // New semantics make a copy of the IDList
            ILFree((LPITEMIDLIST)lParam);
            return uScratch;
        }
        return -1;

    case SFVM_GETOBJECTCOUNT:
        return SUCCEEDED(psfv->GetObjectCount(&uScratch)) ? uScratch : -1;

    case SFVM_GETOBJECT:
    {
        LPITEMIDLIST pidl;

        return SUCCEEDED(psfv->GetObject(&pidl, (UINT)lParam)) ? (LPARAM)pidl : NULL;
    }

    case SFVM_REMOVEOBJECT:
        return SUCCEEDED(psfv->RemoveObject((LPITEMIDLIST)lParam, &uScratch)) ? uScratch : -1;

    case SFVM_UPDATEOBJECT:
    {
        LPITEMIDLIST *ppidl = (LPITEMIDLIST*)lParam;

        if (SUCCEEDED(psfv->UpdateObject(ppidl[0], ppidl[1], &uScratch))
            && (int)uScratch >= 0)
        {
            // New semantics make a copy of the IDList
            ILFree(ppidl[1]);
            return uScratch;
        }
        return -1;
    }

    case SFVM_REFRESHOBJECT:
    {
        LPITEMIDLIST *ppidl = (LPITEMIDLIST*)lParam;

        return SUCCEEDED(psfv->RefreshObject(ppidl[0], &uScratch)) ? uScratch : -1;
    }

    case SFVM_SETREDRAW:
        psfv->SetRedraw(BOOLFROMPTR(lParam));
        break;

    case SFVM_GETSELECTEDOBJECTS:
        return SUCCEEDED(psfv->GetSelectedObjects((LPCITEMIDLIST**)lParam, &uScratch)) ? uScratch : -1;

    case SFVM_GETSELECTEDCOUNT:
        return SUCCEEDED(psfv->GetSelectedCount(&uScratch)) ? uScratch : -1;

    case SFVM_ISDROPONSOURCE:
        return psfv->IsDropOnSource((LPDROPTARGET)lParam) == S_OK;

    case SFVM_MOVEICONS:
        psfv->MoveIcons((LPDATAOBJECT)lParam);
        break;

    case SFVM_GETDROPPOINT:
        return psfv->GetDropPoint((LPPOINT)lParam) == S_OK;

    case SFVM_GETDRAGPOINT:
        return psfv->GetDragPoint((LPPOINT)lParam) == S_OK;

    case SFVM_SETITEMPOS:
    {
        SFV_SETITEMPOS* psip = (SFV_SETITEMPOS*)lParam;

        psfv->SetItemPos(psip->pidl, &psip->pt);
        break;
    }

    case SFVM_ISBKDROPTARGET:
        return psfv->IsBkDropTarget((LPDROPTARGET)lParam) == S_OK;

    case SFVM_SETCLIPBOARD:
        psfv->SetClipboard(lParam == DFM_CMD_MOVE);
        break;

    case SFVM_SETPOINTS:
        psfv->SetPoints((LPDATAOBJECT)lParam);
        return 0;

    case SFVM_GETITEMSPACING:
        return psfv->GetItemSpacing((LPITEMSPACING)lParam) == S_OK;

    default:
        // -1L is the default return value
        return 0;
    }

    return 1;
}

IShellFolderView* ShellFolderViewFromWindow(HWND hwnd)
{
    IShellFolderView* psfv = NULL;

    // HPCView sometimes gets confused and passes HWND_BROADCAST as its
    // window.  We can't let this reach FileCabinet_GetIShellBrowser or
    // we end up broadcasting the CWM_GETISHELLBROWSER message and screwing
    // up everybody in the system.  (Not to mention that it will return TRUE,
    // indicating a successful broadcast, and then we fault thinking that
    // it's a vtbl.)

    if (hwnd && hwnd != HWND_BROADCAST)
    {
        IShellBrowser* psb = FileCabinet_GetIShellBrowser(hwnd);

        // Use !IS_INTRESOURCE() to protect against blatanly bogus values
        // that clearly aren't pointers to objects.
        if (!IS_INTRESOURCE(psb))
        {
            IShellView* psv;
            if (SUCCEEDED(psb->QueryActiveShellView(&psv)))
            {
                psv->QueryInterface(IID_IShellFolderView, (void **)&psfv);
                psv->Release();
            }
        }
    }
    return psfv;
}


// old msg based way of programming defview (pre dates IShellFolderView)

STDAPI_(LRESULT) SHShellFolderView_Message(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    LRESULT lret = 0;
    IShellFolderView* psfv = ShellFolderViewFromWindow(hwnd);
    if (psfv)
    {
        lret = _ShellFolderViewMessage(psfv, uMsg, lParam);
        psfv->Release();
    }
    return lret;
}


STDAPI SHCreateShellFolderViewEx(LPCSFV pcsfv, IShellView **ppsv)
{
    SFV_CREATE sfvc;

    sfvc.cbSize = SIZEOF(sfvc);
    sfvc.pshf = pcsfv->pshf;
    sfvc.psvOuter = pcsfv->psvOuter;

    sfvc.psfvcb = pcsfv->pfnCallback ? new CWrapOldCallback(pcsfv) : NULL;

    HRESULT hres = SHCreateShellFolderView(&sfvc, ppsv);

    if (sfvc.psfvcb)
        sfvc.psfvcb->Release();

    return hres;
}

STDAPI_(void) InitializeStatus(IUnknown *psite)
{
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb)))
    {
        LONG_PTR nParts = 0, n;

        psb->SendControlMsg(FCW_STATUS, SB_GETPARTS, 0, 0, &nParts);

        for (n = 0; n < nParts; n ++)
        {
            psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, n, (LPARAM)TEXT(""), NULL);
            psb->SendControlMsg(FCW_STATUS, SB_SETICON, n, (LPARAM)NULL, NULL);
        }
        psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, 0, 0, NULL);
        psb->Release();
    }
}

//
//  The status bar partitioning has undergone several changes.  Here's
//  what we've got right now:
//
//      Pane 0 = Selection - all remaining space
//      Pane 1 = Size      - just big enough to say 9,999 bytes (11 chars)
//      Pane 2 = Zone      - just big enough to hold longest zone
//

STDAPI_(void) ResizeStatus(IUnknown *psite, UINT cx)
{
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb)))
    {
        HWND hwndStatus;
        if (SUCCEEDED(psb->GetControlWindow(FCW_STATUS, &hwndStatus)) && hwndStatus)
        {
            RECT rc;
            int ciParts[3];
            int ciBorders[3];
            int cxPart;
            GetClientRect(hwndStatus, &rc);

            // Must also take status bar borders into account.
            psb->SendControlMsg(FCW_STATUS, SB_GETBORDERS, 0, (LPARAM)ciBorders, NULL);

            // We build the panes from right to left.
            ciParts[2] = -1;

            // The Zones part
            cxPart = ciBorders[0] + ZoneComputePaneSize(hwndStatus) + ciBorders[2];
            ciParts[1] = rc.right - cxPart;

            // The Size part
            HDC hdc = GetDC(hwndStatus);
            HFONT hfPrev = SelectFont(hdc, GetWindowFont(hwndStatus));
            SIZE siz;
            GetTextExtentPoint32(hdc, TEXT("0"), 1, &siz);
            SelectObject(hdc, hfPrev);
            ReleaseDC(hwndStatus, hdc);
            
            cxPart = ciBorders[0] + siz.cx * (11 + 2); // "+2" for slop
            ciParts[0] = ciParts[1] - cxPart;

            //
            //  If we underflowed, then give up and just give everybody
            //  one third.
            //
            if (ciParts[0] < 0)
            {
                ciParts[0] = rc.right / 3;
                ciParts[1] = 2 * ciParts[0];
            }

            psb->SendControlMsg(FCW_STATUS, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts, NULL);
        }
        psb->Release();
    }
}

STDAPI_(void) SetStatusText(IUnknown *psite, LPCTSTR *ppszText, int iStart, int iEnd)
{
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb)))
    {
        for (; iStart <= iEnd; iStart++) 
        {
            LPCTSTR psz;

            if (ppszText) 
            {
                psz = *ppszText;
                ppszText++;
            } 
            else 
                psz = c_szNULL;

            // a-msadek; needed only for BiDi Win95 loc
            // Mirroring will take care of that over NT5 & BiDi Win98
            if (g_bBiDiW95Loc)
                psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, SBT_RTLREADING | (WPARAM)iStart, (LPARAM)psz, NULL);
            else
                psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, (WPARAM)iStart, (LPARAM)psz, NULL);
        }
        psb->Release();
    }
}
