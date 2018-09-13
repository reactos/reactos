//*******************************************************************************************
//
// Filename : Sfview.cpp
//	
//				Implementation file for CSFView
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "Pch.H"

#include "SFView.H"
#include "SFVWnd.H"

#include "Resource.H"

#include "ThisGuid.H"

#include "SFView.H"

struct SFSTATE_HDR
{
    CLSID clsThis;
    SFSTATE sfState;
    UINT nCols;
} ;


CSFView::CSFView(IShellFolder *psf, IShellFolderViewCallback *psfvcb) :
    m_psf(psf), m_erFolder(psf), m_erCB(psfvcb), m_pCDB(NULL), m_cView(this),
    m_uState(SVUIA_DEACTIVATE), m_pcmSel(NULL), m_cAccel(IDA_MAIN)
{
    m_psfvcb = psfvcb;
    if (psfvcb)
    {
        psfvcb->AddRef();
    }
    
    psf->AddRef();
    
    m_aParamSort = DPA_Create(4);
    m_sfState.lParamSort = 0;
}


CSFView::~CSFView()
{
    ReleaseSelContextMenu();
}


STDMETHODIMP CSFView::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const IID *apiid[] = { &IID_IShellView, NULL };
    LPUNKNOWN aobj[] = { (IShellView *)this };
    
    return(QIHelper(riid, ppvObj, apiid, aobj));
}


STDMETHODIMP_(ULONG) CSFView::AddRef()
{
    return(AddRefHelper());
}


STDMETHODIMP_(ULONG) CSFView::Release()
{
    return(ReleaseHelper());
}


STDMETHODIMP CSFView::GetWindow(HWND * lphwnd)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CSFView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return(E_NOTIMPL);
}


//*****************************************************************************
//
// CSFView::TranslateAccelerator
//
// Purpose:
//         Handle the accelerator keystrokes
//
//
// Parameters:
//        LPMSG lpmsg    -    message structure
//
//
// Comments:
//
//*****************************************************************************

STDMETHODIMP CSFView::TranslateAccelerator(LPMSG lpmsg)
{
    return(m_cAccel.TranslateAccelerator(m_cView, lpmsg) ? S_OK : S_FALSE);
}


STDMETHODIMP CSFView::EnableModeless(BOOL fEnable)
{
    return(E_NOTIMPL);
}


//*****************************************************************************
//
// CSFView:UIActivate
//
// Purpose:
//        The explorer calls this member function whenever the activation         
//  state of the view window is changed by a certain event that is           
//  NOT caused by the shell view itself.
//
//
// Parameters:
//
//        UINT uState    -    UI activate flag
//
// Comments:
//
//*****************************************************************************

STDMETHODIMP CSFView::UIActivate(UINT uState)
{
    if (uState)
    {
        OnActivate(uState);
    }
    else
    {
        OnDeactivate();
    }
    
    return S_OK;
}


STDMETHODIMP CSFView::Refresh()
{
    FillList(FALSE);
    return(0);              //BUGBUG: what to return?
}


//*****************************************************************************
//
// CSFView::CreateViewWindow
//
// Purpose:
//
//        called by IShellBrowser to create a contents pane window
//
// Parameters:
//    
//    IShellView  *lpPrevView    -    previous view
//    LPCFOLDERSETTINGS lpfs     -    folder settings for the view
//    IShellBrowser *psb         -    pointer to the shell browser
//    RECT * prcView             -    view Rectangle
//    HWND * phWnd               -    pointer to Window handle
//
//
// Comments:
//
//*****************************************************************************

STDMETHODIMP CSFView::CreateViewWindow(IShellView  *lpPrevView,
                                       LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                                       RECT * prcView, HWND  *phWnd)
{
    *phWnd = NULL;
    
    if ((HWND)m_cView)
    {
        return(E_UNEXPECTED);
    }
    
    m_fs = *lpfs;
    m_psb = psb;
    
    // get the main window handle from shell browser
    
    psb->GetWindow(&m_hwndMain);
    
    // bring up the contents pane
    
    if (!m_cView.DoModeless(IDD_VIEW, m_hwndMain))
    {
        return(E_OUTOFMEMORY);
    }
    
    *phWnd = m_cView;
    
    // map the current view mode into menu id and set the contents pane
    // view mode accordingly
    
    OnCommand(NULL, GET_WM_COMMAND_MPS(GetMenuIDFromViewMode(), 0, 0));
    
    AddColumns();
    
    RestoreViewState();
    
    // size the contents pane
    SetWindowPos(m_cView, NULL, prcView->left, prcView->top,
        prcView->right-prcView->left, prcView->bottom-prcView->top,
        SWP_NOZORDER|SWP_SHOWWINDOW);
    
    FillList(TRUE);
    
    return(NOERROR);
}


STDMETHODIMP CSFView::DestroyViewWindow()
{
    if (!(HWND)m_cView)
    {
        return(E_UNEXPECTED);
    }
    
    m_cView.DestroyWindow();
    
    return(NOERROR);
}


STDMETHODIMP CSFView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
    *lpfs = m_fs;
    
    return(NOERROR);
}


STDMETHODIMP CSFView::AddPropertySheetPages(DWORD dwReserved,
                                            LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CSFView::SaveViewState()
{
    SFSTATE_HDR hdr;
    LPSTREAM pstm;
    
    HRESULT hres = m_psb->GetViewStateStream(STGM_WRITE, &pstm);
    if (FAILED(hres))
    {
        return(hres);
    }
    CEnsureRelease erStr(pstm);
    
    pstm->Write(&hdr, sizeof(hdr), NULL);
    
    hdr.clsThis = CLSID_ThisDll;
    hdr.sfState = m_sfState;
    hdr.nCols = SaveColumns(pstm);
    
    ULARGE_INTEGER libCurPosition;
    LARGE_INTEGER dlibMove;
    dlibMove.HighPart = 0;
    dlibMove.LowPart = 0;
    pstm->Seek(dlibMove, STREAM_SEEK_SET, &libCurPosition);
    
    hres = pstm->Write(&hdr, sizeof(hdr), NULL);
    
    return(hres);
}


STDMETHODIMP CSFView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
    return(E_NOTIMPL);
}


STDMETHODIMP CSFView::GetItemObject(UINT uItem, REFIID riid,
                                    void **ppv)
{
    return(E_NOTIMPL);
}


int CSFView::AddObject(LPCITEMIDLIST pidl)
{
    // Check the commdlg hook to see if we should include this
    // object.
    if (IncludeObject(pidl) != S_OK)
    {
        return(-1);
    }
    
    return(m_cView.AddObject(pidl));
}


int CALLBACK CSFView::CompareIDs(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    PFNDPACOMPARE pfnCheckAPI = CompareIDs;
    
    CSFView *pThis = (CSFView *)lParam;
    
    HRESULT hres = pThis->m_psf->CompareIDs(pThis->m_sfState.lParamSort,
        (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
    
    
    return (hres);
}

//*****************************************************************************
//
// CSFView::FillList
//
// Purpose:
//
//        Enumerates the objects in the namespace and fills up the
//        data structures
//
//
// Comments:
//
//*****************************************************************************

HRESULT CSFView::FillList(BOOL bInteractive)
{
    m_cView.DeleteAllItems();
    
    // Setup the enum flags.
    DWORD dwEnumFlags = SHCONTF_NONFOLDERS;
    if (ShowAllObjects())
    {
        dwEnumFlags |= SHCONTF_INCLUDEHIDDEN ;
    }
    
    if (!(m_fs.fFlags & FWF_NOSUBFOLDERS))
    {
        dwEnumFlags |= SHCONTF_FOLDERS;
    }
    
    // Create an enum object and get the IEnumIDList ptr
    LPENUMIDLIST peIDL;
    HRESULT hres = m_psf->EnumObjects(bInteractive ? m_hwndMain : NULL,
        dwEnumFlags, &peIDL);
    
    // Note the return may be S_FALSE which indicates no enumerator.
    // That's why we shouldn't use if (FAILED(hres))
    if (hres != S_OK)
    {
        if (hres == S_FALSE)
        {
            return(NOERROR);
        }
        
        return(hres);
    }
    CEnsureRelease erEnum(peIDL);
    
    HDPA hdpaNew = DPA_Create(16);
    if (!hdpaNew)
    {
        return(E_OUTOFMEMORY);
    }
    
    LPITEMIDLIST pidl;
    ULONG celt;
    
    // Enumerate the idlist and insert into the DPA
    
    while (peIDL->Next(1, &pidl, &celt) == S_OK)
    {
        if (DPA_InsertPtr(hdpaNew, 0x7fff, pidl) == -1)
        {
            m_cMalloc.Free(pidl);
        }
    }
    
    DPA_Sort(hdpaNew, CompareIDs, (LPARAM)this);
    
    int cNew = DPA_GetPtrCount(hdpaNew);
    for (int i=0; i<cNew; ++i)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(hdpaNew, i);
        if (AddObject(pidl) < 0)
        {
            m_cMalloc.Free(pidl);
        }
    }
    
    return(NOERROR);
}


//*****************************************************************************
//
// CSFView::AddColumns
//
// Purpose:
//
//        Adds columns to the contents pane listview
//
// Comments:
//
//*****************************************************************************
void CSFView::AddColumns()
{
    UINT cxChar = m_cView.CharWidth();
    
    // add columns to the listview in the contents pane
    for (int i=0; ; ++i)
    {
        SFVCB_GETDETAILSOF_DATA gdo;
        gdo.pidl = NULL;
        
        // get the first column
        
        HRESULT hres = CallCB(SFVCB_GETDETAILSOF, i, (LPARAM)&gdo);
        if (hres != S_OK)
        {
            if (i != 0)
            {
                break;
            }
            
            // If there is no first column, fake one up
            gdo.fmt = LVCFMT_LEFT;
            gdo.cChar = 40;
            gdo.lParamSort = 0;
            gdo.str.uType = STRRET_CSTR;
            LoadString(g_ThisDll.GetInstance(), IDS_NAME, gdo.str.cStr, sizeof(gdo.str.cStr));
        }
        
        char szText[MAX_PATH];
        // init the column info for the details view ...
        LV_COLUMN col;
        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt = gdo.fmt;
        col.cx = gdo.cChar * cxChar;
        col.pszText = szText;
        col.cchTextMax = sizeof(szText);
        col.iSubItem = i;
        
        StrRetToStr(szText, sizeof(szText), &gdo.str, NULL);
        
        // insert the column into the list view
        if (m_cView.InsertColumn(i, &col)>=0 && m_aParamSort)
        {
            DPA_InsertPtr(m_aParamSort, 0x7fff, (LPVOID)gdo.lParamSort);
        }
        
        if (hres != S_OK)
        {
            break;
        }
    }
}


//
// Save (and check) column header information
// Returns TRUE if the columns are the default width, FALSE otherwise
// Side effect: the stream pointer is left right after the last column
//
BOOL CSFView::SaveColumns(LPSTREAM pstm)
{
    UINT cxChar = m_cView.CharWidth();
    BOOL bDefaultCols = TRUE;
    
    for (int i=0; ; ++i)
    {
        SFVCB_GETDETAILSOF_DATA gdo;
        gdo.pidl = NULL;
        
        if (CallCB(SFVCB_GETDETAILSOF, i, (LPARAM)&gdo) != S_OK)
        {
            break;
        }
        
        LV_COLUMN col;
        col.mask = LVCF_WIDTH;
        
        if (!m_cView.GetColumn(i, &col))
        {
            // There is some problem, so just assume
            // default column widths
            bDefaultCols = TRUE;
            break;
        }
        
        if (col.cx != (int)(gdo.cChar * cxChar))
        {
            bDefaultCols = FALSE;
        }
        
        // HACK: I don't really care about column widths larger
        // than 64K
        if (FAILED(pstm->Write(&col.cx, sizeof(USHORT), NULL)))
        {
            // There is some problem, so just assume
            // default column widths
            bDefaultCols = TRUE;
            break;
        }
    }
    
    return(bDefaultCols ? 0 : i);
}


void CSFView::RestoreColumns(LPSTREAM pstm, int nCols)
{
    for (int i=0; i<nCols; ++i)
    {
        LV_COLUMN col;
        col.mask = LVCF_WIDTH;
        
        if (FAILED(pstm->Read(&col.cx, sizeof(USHORT), NULL)))
        {
            break;
        }
        
        m_cView.SetColumn(i, &col);
    }
}


void CSFView::RestoreViewState()
{
    SFSTATE_HDR hdr;
    
    LPSTREAM pstm;
    
    MergeToolBar();
    
    // get the stream for storing view specific info
    if (FAILED(m_psb->GetViewStateStream(STGM_READ, &pstm)))
    {
        return;
    }
    CEnsureRelease erStr(pstm);
    
    if (FAILED(pstm->Read(&hdr, sizeof(hdr), NULL)))
    {
        return;
    }
    
    // Validate the header
    if (hdr.clsThis != CLSID_ThisDll)
    {
        return;
    }
    
    m_sfState = hdr.sfState;
    RestoreColumns(pstm, hdr.nCols);
}


void CSFView::CheckToolbar()
{
    UINT idCmdCurView = GetMenuIDFromViewMode();
    
    for (UINT idCmd=IDC_VIEW_ICON; idCmd<=IDC_VIEW_DETAILS; ++idCmd)
    {
        m_psb->SendControlMsg(FCW_TOOLBAR, TB_CHECKBUTTON, idCmd,
            (LPARAM)(idCmd == idCmdCurView), NULL);
    }
}


void CSFView::MergeToolBar()
{
    enum
    {
        IN_STD_BMP = 0x4000,
            IN_VIEW_BMP = 0x8000,
    } ;
    static const TBBUTTON c_tbDefault[] =
    {
        { STD_COPY | IN_STD_BMP, IDC_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, -1},
        { 0,    0,	    TBSTATE_ENABLED, TBSTYLE_SEP, {0,0}, 0, -1 },
        // the bitmap indexes here are relative to the view bitmap
        { VIEW_LARGEICONS | IN_VIEW_BMP, IDC_VIEW_ICON,	        TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
        { VIEW_SMALLICONS | IN_VIEW_BMP, IDC_VIEW_SMALLICON, 	TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
        { VIEW_LIST       | IN_VIEW_BMP, IDC_VIEW_LIST,         TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
        { VIEW_DETAILS    | IN_VIEW_BMP, IDC_VIEW_DETAILS,      TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    } ;
    
    LRESULT iStdBMOffset;
    LRESULT iViewBMOffset;
    TBADDBITMAP ab;
    ab.hInst = HINST_COMMCTRL;		// hinstCommctrl
    ab.nID   = IDB_STD_SMALL_COLOR;	// std bitmaps
    m_psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &iStdBMOffset);
    
    ab.nID   = IDB_VIEW_SMALL_COLOR;	// std view bitmaps
    m_psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &iViewBMOffset);
    
    TBBUTTON tbActual[ARRAYSIZE(c_tbDefault)];
    
    for (int i=0; i<ARRAYSIZE(c_tbDefault); ++i)
    {
        tbActual[i] = c_tbDefault[i];
        if (!(tbActual[i].fsStyle & TBSTYLE_SEP))
        {
            if (tbActual[i].iBitmap & IN_VIEW_BMP)
            {
                tbActual[i].iBitmap = (tbActual[i].iBitmap & ~IN_VIEW_BMP) + iViewBMOffset;
            }
            else if (tbActual[i].iBitmap & IN_STD_BMP)
            {
                tbActual[i].iBitmap = (tbActual[i].iBitmap & ~IN_STD_BMP) + iStdBMOffset;
            }
        }
    }
    
    m_psb->SetToolbarItems(tbActual, ARRAYSIZE(c_tbDefault), FCT_MERGE);
    
    CheckToolbar();
}


HRESULT CreateShellFolderView(IShellFolder *psf, IShellFolderViewCallback *psfvcb,
                              LPSHELLVIEW * ppsv)
{
    CSFView *pSFView = new CSFView(psf, psfvcb);
    if (!pSFView)
    {
        return(E_OUTOFMEMORY);
    }
    
    pSFView->AddRef();
    HRESULT hRes = pSFView->QueryInterface(IID_IShellView, (void **)ppsv);
    pSFView->Release();
    
    return(hRes);
}
