#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"

#ifndef WINNT

#define ICOL_DOCNAME    0
#define ICOL_STATUS     1
#define ICOL_OWNER      2
#define ICOL_PROGRESS   3
#define ICOL_TIME       4
#define ICOL_NUM        5

typedef struct _QueueInfo
{
    JOB_INFO_2 *pJobs;
    DWORD dwJobs;

    IDPRINTER idp;              // name of printer
    HANDLE hPrinter;            // handle to printer

    HWND hDlg;                  // dialog handle
    HWND hwndLV;                // listview handle
    HWND hwndSB;                // status bar handle
    BOOL fStatusBar;            // TRUE iff status bar is shown

    int cx, cy;                 // window extents for WM_SIZE

    BOOL bDragSource;
    BOOL bMinimized;
    // POINT ptDragAnchor;

    HICON hLargeIcon;           // icons for the sysmenu / alt-tab list
    HICON hSmallIcon;           // these need to be freed on window destroy

} QUEUEINFO, *PQUEUEINFO;

typedef struct _PosInfo
{
    WINDOWPLACEMENT wp;
    int nWidths[ICOL_NUM];
    BOOL fStatusBar;
} POSINFO;

typedef struct _InitInfo
{
    LPCTSTR lpszPrinterName;
    int nCmdShow;
} INITINFO;

extern TCHAR const c_szRAW[];

BOOL Printer_ModifyJob(HWND hDlg, DWORD dwCommand);


#endif // WINNT

//---------------------------------------------------------------------------
//
// Helper functions
//

typedef struct _IDPRINTJOB
{
    USHORT              cb;
    SHCNF_PRINTJOB_DATA data;
    USHORT              uTerm;
} IDPRINTJOB, *LPIDPRINTJOB;
typedef const IDPRINTJOB *LPCIDPRINTJOB;

void Printjob_FillPidl(LPIDPRINTJOB pidl, LPSHCNF_PRINTJOB_DATA pData)
{
    pidl->cb = FIELD_OFFSET(IDPRINTJOB, uTerm);
    if (pData)
    {
        pidl->data = *pData;
    }
    else
    {
        ZeroMemory(&(pidl->data), SIZEOF(SHCNF_PRINTJOB_DATA));
    }
    pidl->uTerm = 0;
}

LPITEMIDLIST Printjob_GetPidl(LPCTSTR szName, LPSHCNF_PRINTJOB_DATA pData)
{
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlParent = Printers_GetPidl(NULL, szName);
    if (pidlParent)
    {
        IDPRINTJOB idj;

        Printjob_FillPidl(&idj, pData);

        pidl = ILCombine(pidlParent, (LPITEMIDLIST)&idj);

        ILFree(pidlParent);
    }

    return pidl;
}

#ifndef WINNT

/********************************************************************/
/* IDropTarget and IDataObject for drag & drop re-ordering of queue */
/********************************************************************/

// print job descriptor

typedef struct
{
    DWORD   dwJobId;
    int     iItem;
} PRINTJOBDESC;


// data object for print job

typedef struct
{
    IDataObject dtobj;
    UINT        cRef;

    PRINTJOBDESC pj;    // the data is here
} CPJData;


UINT g_cfPrintJob = 0;

extern const IDataObjectVtbl c_CPJDataVtbl;          // forward

void RegisterPrintJobFormat()
{
    if (!g_cfPrintJob)
        g_cfPrintJob = RegisterClipboardFormat(TEXT("PrintJob"));
}


//
// print job data object for drag/drop
//
STDMETHODIMP CPJData_CreateInstance(const PRINTJOBDESC *ppj, IDataObject **ppdtobj)
{
    CPJData *this = (void*)LocalAlloc(LPTR, SIZEOF(CPJData));
    if (this)
    {
        this->dtobj.lpVtbl = &c_CPJDataVtbl;
        this->cRef = 1;
        *ppdtobj = &this->dtobj;
        this->pj = *ppj;

        RegisterPrintJobFormat();

        return NOERROR;
    }
    else
    {
        *ppdtobj = NULL;
        return E_OUTOFMEMORY;
    }
}

//===========================================================================

STDMETHODIMP CPJData_QueryInterface(IDataObject *pdtobj, REFIID riid, LPVOID *ppvObj)
{
    CPJData *this = IToClass(CPJData, dtobj, pdtobj);

    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPJData_AddRef(IDataObject *pdtobj)
{
    CPJData *this = IToClass(CPJData, dtobj, pdtobj);

    this->cRef++;
    return this->cRef;
}

STDMETHODIMP_(ULONG) CPJData_Release(IDataObject *pdtobj)
{
    CPJData *this = IToClass(CPJData, dtobj, pdtobj);

    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP CPJData_GetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    CPJData *this = IToClass(CPJData, dtobj, pdtobj);
    HRESULT hres = E_INVALIDARG;

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;
    pmedium->tymed = TYMED_HGLOBAL;

    if ((g_cfPrintJob == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
    {
        pmedium->hGlobal = GlobalAlloc(GPTR, SIZEOF(PRINTJOBDESC));
        if (pmedium->hGlobal)
        {
            *((PRINTJOBDESC *)pmedium->hGlobal) = this->pj;
            hres = NOERROR;     // success
        }
        else
            hres = E_OUTOFMEMORY;
    }
    return hres;
}

STDMETHODIMP CPJData_GetDataHere(IDataObject *pdtobj, LPFORMATETC pformatetc, LPSTGMEDIUM pmedium )
{
    return E_NOTIMPL;
}

STDMETHODIMP CPJData_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn)
{
    // CPJData *this = IToClass(CPJData, dtobj, pdtobj);
    if ((g_cfPrintJob == pformatetcIn->cfFormat) && (TYMED_HGLOBAL & pformatetcIn->tymed))
    {
        return NOERROR;
    }
    return S_FALSE;
}

STDMETHODIMP CPJData_GetCanonicalFormatEtc(IDataObject *pdtobj, LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP CPJData_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return E_INVALIDARG;
}

STDMETHODIMP CPJData_EnumFormatEtc(IDataObject *pdtobj, DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc)
{
    return S_FALSE;
}

STDMETHODIMP CPJData_Advise(IDataObject *pdtobj, FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CPJData_Unadvise(IDataObject *pdtobj, DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CPJData_EnumAdvise(IDataObject *pdtobj, LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

const IDataObjectVtbl c_CPJDataVtbl = {
    CPJData_QueryInterface, CPJData_AddRef, CPJData_Release,
    CPJData_GetData,
    CPJData_GetDataHere,
    CPJData_QueryGetData,
    CPJData_GetCanonicalFormatEtc,
    CPJData_SetData,
    CPJData_EnumFormatEtc,
    CPJData_Advise,
    CPJData_Unadvise,
    CPJData_EnumAdvise
};

//---------------------------------------------------------------------------
//
// IDropTarget stuff
//

//=============================================================================
// CPQDropTarget : class definition
//=============================================================================
typedef struct {        // dvdt
    IDropTarget         dt;
    UINT                cRef;
    PQUEUEINFO          pqi;
    DWORD               grfKeyStateLast;
    DWORD               dwEffect;       // drop action set in DragEnter
    AUTO_SCROLL_DATA    asd;            // for auto scrolling
} CPQDropTarget;

//=============================================================================
// CPQDropTarget : Constructor
//=============================================================================

extern const IDropTargetVtbl c_CPQDropTarget;        // forward

STDMETHODIMP CPQDropTarget_CreateInstance(PQUEUEINFO pqi, IDropTarget **ppdtgt)
{
    HRESULT hres = E_OUTOFMEMORY;
    CPQDropTarget *pdvdt = (void*)LocalAlloc(LPTR, SIZEOF(CPQDropTarget));
    if (pdvdt)
    {
        pdvdt->dt.lpVtbl = &c_CPQDropTarget;
        pdvdt->cRef = 1;

        pdvdt->pqi = pqi;

        *ppdtgt = &pdvdt->dt;
        hres = NOERROR;
    }
    return hres;
}

//=============================================================================
// CPQDropTarget : member
//=============================================================================

STDMETHODIMP CPQDropTarget_QueryInterface(IDropTarget *pdt, REFIID riid, LPVOID *ppvObj)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);

    // If we just want the same interface or an unknown one, return
    // this guy
    //
    if (IsEqualIID(riid, &IID_IDropTarget) || IsEqualIID(riid, &IID_IUnknown))
    {
        *((IDropTarget **)ppvObj) = &this->dt;
        this->cRef++;
        return(NOERROR);
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPQDropTarget_AddRef(IDropTarget *pdt)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);
    this->cRef++;
    return(this->cRef);
}

STDMETHODIMP_(ULONG) CPQDropTarget_Release(IDropTarget *pdt)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);

    this->cRef--;
    if (this->cRef > 0)
        return this->cRef;

    LocalFree((HLOCAL)this);

    return 0;
}

STDMETHODIMP CPQDropTarget_DragEnter(IDropTarget *pdt, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);
    FORMATETC fmte = {g_cfPrintJob, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    DebugMsg(DM_TRACE, TEXT("sh - TR CPQDropTarget::DragEnter"));

    this->grfKeyStateLast = grfKeyState;

    RegisterPrintJobFormat();

    // default action
    *pdwEffect = DROPEFFECT_NONE;

    // We accept jobs from this queue:
    // - the only "job" object that we accept is ours, so do a quick check
    //   (this also keeps us from supporting another datatype in idataobject!)
    // - there's only one outstanding drag:
    //   if this pdt's queue was the source of the drag, we can accept it
    if (this->pqi->bDragSource && (NOERROR == pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte)))
    {
        // this is a print job from our window, accept this
        *pdwEffect = DROPEFFECT_MOVE;
    }
    else
    {
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        // We also accept files for printing

        if (NOERROR == pdtobj->lpVtbl->QueryGetData(pdtobj, &fmte))
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
    }

    // store dwEffect for DragOver
    this->dwEffect = *pdwEffect;

    DAD_DragEnter(this->pqi->hwndLV);

    DAD_InitScrollData(&this->asd);

    return NOERROR;
}

int PQ_GetDropTarget(CPQDropTarget *this, long y)
{
    LV_HITTESTINFO info;
    int iItem;

    info.pt.x = 0;
    info.pt.y = y;
    ScreenToClient(this->pqi->hwndLV, &info.pt);

    // using 10 for x forces us into "document name" column, allowing us to
    // get a "hit" even if we are on some other column.
    info.pt.x = 10;

    iItem = ListView_HitTest(this->pqi->hwndLV, &info);
    if (iItem == -1)
    {
        // another hack: since 100 above forces us into column 1, we know that
        // a -1 return is not from being too far to the left or to the right.
        // And you can't go off the top (above item 0) without falling off the
        // listview.  So a -1 return means we're below the last item, so put
        // us on the last item:
        iItem = ListView_GetItemCount(this->pqi->hwndLV) - 1;
    }

    return iItem;
}

STDMETHODIMP CPQDropTarget_DragOver(IDropTarget *pdt, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    HRESULT hres = NOERROR;
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);
    POINT pt = { ptl.x, ptl.y };        // in screen coords

    ScreenToClient(this->pqi->hwndLV, &pt);

    // assume coords of our window match listview
    DAD_AutoScroll(this->pqi->hwndLV, &this->asd, &pt);

    // effect was stored in DragEnter
    *pdwEffect = this->dwEffect;

    // If this->dwEffect is DROPACTION_MOVE, then
    // it might be nice to show where the job will move to:
    //            v
    //  - Use a " - " cursor to show where this job will be inserted
    //            ^

    DAD_DragMove(pt);

    return hres;
}

STDMETHODIMP CPQDropTarget_DragLeave(IDropTarget *pdt)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);

    DebugMsg(DM_TRACE, TEXT("sh - TR CPQDropTarget::DragLeave"));

    DAD_DragLeave();
    LVUtil_DragSelectItem(this->pqi->hwndLV, -1);

    return NOERROR;
}


STDMETHODIMP CPQDropTarget_Drop(IDropTarget *pdt, IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CPQDropTarget *this = IToClass(CPQDropTarget, dt, pdt);
    HRESULT hres = NOERROR;

    DebugMsg(DM_TRACE, TEXT("sh - TR CPQDropTarget::Drop"));

    if (this->grfKeyStateLast & MK_LBUTTON)
    {
        *pdwEffect = this->dwEffect;
    }
    else
    {
        HMENU hmenu;

        // pop up a menu to choose from.
        if (this->dwEffect == DROPEFFECT_MOVE)
        {
            // we're moving a job
            hmenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_MOVEONLYDD);
        }
        else // this->dwEffect == DROPEFFECT_COPY
        {
            // we're printing a file
            ASSERT(this->dwEffect == DROPEFFECT_COPY);

            // To use MENU_PRINTOBJ_DD we assume DDIDM_COPY == DROPEFFECT_COPY
            hmenu = SHLoadPopupMenu(HINST_THISDLL, MENU_PRINTOBJ_DD);
        }

        if (hmenu)
        {
            SetMenuDefaultItem(hmenu, this->dwEffect, MF_BYCOMMAND);

            *pdwEffect = TrackPopupMenu(
                    hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    ptl.x, ptl.y, 0, this->pqi->hwndLV, NULL);

            DestroyMenu(hmenu);
            DebugMsg(DM_TRACE, TEXT("CPQDropTarget_Drop *pdwEffect = %x, this->dwEffect=%x"), *pdwEffect, this->dwEffect);
        }
    }

    // Do the drop!
    if (*pdwEffect == DROPEFFECT_COPY)
    {
        // we're printing the file pdtobj to printer idp
        hres = PrintObj_DropPrint(pdtobj, this->pqi->hDlg, *pdwEffect, (LPCITEMIDLIST)&(this->pqi->idp), CPrintObj_DropThreadProc);
    }
    else if (*pdwEffect == DROPEFFECT_MOVE)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {g_cfPrintJob, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)))
        {
            int iItem;
            BOOL bRet;
            PRINTJOBDESC *pidpj = (PRINTJOBDESC *)GlobalLock(medium.hGlobal);

            ASSERT(pidpj->dwJobId == this->pqi->pJobs[pidpj->iItem].JobId);

            iItem = PQ_GetDropTarget(this, ptl.y);

            DebugMsg(DM_TRACE, TEXT("Drop %d on %d"), pidpj->iItem, iItem);

            bRet = FALSE;

            this->pqi->pJobs[pidpj->iItem].Position = iItem + 1;
            bRet = SetJob(this->pqi->hPrinter, pidpj->dwJobId, 2,
                        (LPBYTE)&(this->pqi->pJobs[pidpj->iItem]), 0);

            if (bRet)
            {
                PostMessage(this->pqi->hDlg, WM_TIMER, 1, 0L);
            }
            else
            {
                MessageBeep((UINT)-1);
            }
            GlobalUnlock(medium.hGlobal);
            ReleaseStgMedium(&medium);
        }
    }
    // Make sure to unlock the window for updating
    CPQDropTarget_DragLeave(pdt);

    return hres;
}

const IDropTargetVtbl c_CPQDropTarget = {
    CPQDropTarget_QueryInterface, CPQDropTarget_AddRef, CPQDropTarget_Release,
    CPQDropTarget_DragEnter,
    CPQDropTarget_DragOver,
    CPQDropTarget_DragLeave,
    CPQDropTarget_Drop
};



//---------------------------------------------------------------------------
//
// More D&D functions
//

LRESULT PRQ_BeginDrag(HWND hDlg, PQUEUEINFO pqi, NM_LISTVIEW *lpnm)
{
    int iJob = ListView_GetNextItem(pqi->hwndLV, -1, LVNI_SELECTED);
    LPITEMIDLIST pidlParent;

    ASSERT(iJob >= 0);

    pidlParent = Printers_GetPidl(NULL, pqi->idp.cName);
    if (pidlParent)
    {
        POINT ptOffset = lpnm->ptAction;             // hwndLV client coords
        DWORD dwEffect = DROPEFFECT_MOVE;
        PRINTJOBDESC pjd;
        BOOL fPaused;

        // set up relative pidl for job
        pjd.dwJobId = pqi->pJobs[iJob].JobId;
        pjd.iItem = iJob;

        // pause the job so it won't start printing during drag operation
        fPaused = !(pqi->pJobs[iJob].Status & JOB_STATUS_PAUSED) &&
                Printer_ModifyJob(hDlg, JOB_CONTROL_PAUSE);

        // Somebody began dragging in our window, so store that fact

        // save away the anchor point
        // pqi->ptDragAnchor = lpnm->ptAction;
        // LVUtil_ClientToLV(pqi->hwndLV, &pqi->ptDragAnchor);

        ClientToScreen(pqi->hwndLV, &ptOffset);     // now in screen

        if (DAD_SetDragImageFromListView(pqi->hwndLV, ptOffset))
        {
            IDataObject *pdtobj;
            if (SUCCEEDED(CPJData_CreateInstance(&pjd, &pdtobj)))
            {
                pqi->bDragSource = TRUE;

                SHDoDragDrop(hDlg, pdtobj, NULL, dwEffect, &dwEffect);
                pdtobj->lpVtbl->Release(pdtobj);

                pqi->bDragSource = FALSE;
            }

            DAD_SetDragImage(NULL, NULL);
        }

        // All done dragging
        if (fPaused)
        {
            Printer_ModifyJob(hDlg, JOB_CONTROL_RESUME);
        }

        ILFree(pidlParent);
    }

    return 0L;
}


/********************************************************************/
/* Printer Queue helper functions                                   */
/********************************************************************/

void SystemTimeString(LPTSTR pszText, SYSTEMTIME *pst)
{
    FILETIME ftUTC, ftLocal;
    SYSTEMTIME st;
    TCHAR szTmp[64];

    // Convert from UTC to Local time
    SystemTimeToFileTime(pst, &ftUTC);
    FileTimeToLocalFileTime(&ftUTC, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);

    // Generate string
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, pszText, ARRAYSIZE(szTmp));
    lstrcat(pszText, c_szSpace);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szTmp, ARRAYSIZE(szTmp));
    lstrcat(pszText, szTmp);
}


const struct
{
    int iCol;
    int ids;    // Id of string for title
    int iFmt;   // The format of the column;
} c_PQCols[] = {
    {ICOL_DOCNAME , IDS_PRQ_DOCNAME , LVCFMT_LEFT},
    {ICOL_STATUS  , IDS_PRQ_STATUS  , LVCFMT_LEFT},
    {ICOL_OWNER   , IDS_PRQ_OWNER   , LVCFMT_LEFT},
    {ICOL_PROGRESS, IDS_PRQ_PROGRESS, LVCFMT_LEFT},
    {ICOL_TIME    , IDS_PRQ_TIME    , LVCFMT_LEFT},
};

void PrinterQueue_AddColumns(HWND hLV, int *nPos)
{

    LV_COLUMN col;
    TCHAR szColName[258];
    int i;

    for (i=0; i<ARRAYSIZE(c_PQCols); ++i)
    {
        LoadString(HINST_THISDLL, c_PQCols[i].ids, szColName, ARRAYSIZE(szColName));

        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt  = c_PQCols[i].iFmt;
        col.pszText = (LPTSTR)szColName;
        col.cchTextMax = 0;
        col.cx = nPos[i];
        col.iSubItem = c_PQCols[i].iCol;

        ListView_InsertColumn(hLV, c_PQCols[i].iCol, &col);
    }
}


const TCHAR szPrinterPositions[] = REGSTR_PATH_EXPLORER TEXT("\\Printers") ;
const POSINFO c_PQPos = {
    SIZEOF(WINDOWPLACEMENT), 0, SW_SHOW, 0, 0, 0, 0, 50, 100, 612, 285,
    185, 80, 80, 80, 125, TRUE
};

BOOL PrinterQueue_Init(HWND hDlg, INITINFO *pii)
{
    POSINFO sPos;
    HWND hLV;
    PQUEUEINFO pqi;
    IDropTarget *lpdt;
    HKEY hkey;
    HANDLE hPrinter;
    HWND hSB;
    HIMAGELIST himl;
    int border[] = {0, -1, 2};

    // Let the printer hwnd dpa point to this hDlg instead of stub parent.
    PrintDef_UpdateHwnd(pii->lpszPrinterName, hDlg);

    hLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, c_szNULL,
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|LVS_REPORT,
        0, 0, 100, 100, hDlg, NULL, HINST_THISDLL, NULL);
    if (!hLV)
    {
        goto Error1;
    }

    hSB = CreateStatusWindow(
        WS_CHILD | SBARS_SIZEGRIP | CCS_NOHILITE | WS_CLIPSIBLINGS,
        NULL, hDlg, FCIDM_STATUS);
    if (!hSB)
    {
        goto Error1;
    }
    SendMessage(hSB, SB_SETBORDERS, 0, (LPARAM)(LPINT)border);

    himl = ImageList_Create(g_cxSmIcon, g_cySmIcon, ILC_MASK, 1, 4);
    if (himl)
    {
        HICON hIcon;
        int iIndex;

        ImageList_SetBkColor(himl, GetSysColor(COLOR_WINDOW));

        hIcon = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_DOCUMENT),
                        IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
        iIndex = ImageList_AddIcon(himl, hIcon);
        ASSERT(iIndex == 0);
        DestroyIcon(hIcon);

        SendMessage(hLV, LVM_SETIMAGELIST, (WPARAM)LVSIL_SMALL, (LPARAM)himl);
    }

    hPrinter = Printer_OpenPrinter(pii->lpszPrinterName);
    if (!hPrinter)
    {
        goto Error1;
    }

    pqi = (PQUEUEINFO)LocalAlloc(LPTR, SIZEOF(QUEUEINFO));
    if (!pqi)
    {
        Printer_ClosePrinter(hPrinter);
Error1:
        PostQuitMessage(0);
        return(TRUE);
    }

    pqi->hPrinter = hPrinter;
    pqi->hDlg = hDlg;
    pqi->hwndLV = hLV;
    Printers_FillPidl(&pqi->idp, pii->lpszPrinterName);

    SetWindowPtr(hDlg, DWLP_USER, pqi);

    sPos = c_PQPos;
    if (ERROR_SUCCESS ==
        RegOpenKey(HKEY_CURRENT_USER, szPrinterPositions, &hkey))
    {
        DWORD dwSize = SIZEOF(sPos);
        DWORD dwType;
        if (ERROR_SUCCESS !=
            SHQueryValueEx(hkey, (LPTSTR)(pii->lpszPrinterName), NULL, &dwType, (LPBYTE)&sPos, &dwSize)
            || dwSize != SIZEOF(sPos)
            || sPos.wp.length != SIZEOF(WINDOWPLACEMENT))
        {
            sPos = c_PQPos;
        }
        RegCloseKey(hkey);
    }

    pqi->hwndSB = hSB;
    pqi->fStatusBar = sPos.fStatusBar;
    ShowWindow(pqi->hwndSB, pqi->fStatusBar ? SW_SHOW : SW_HIDE);

    PrinterQueue_AddColumns(hLV, sPos.nWidths);

    Printer_LoadIcons(pqi->idp.cName, &(pqi->hLargeIcon), &(pqi->hSmallIcon));
    SendMessage(pqi->hDlg, WM_SETICON, TRUE, (LPARAM)(pqi->hLargeIcon));
    SendMessage(pqi->hDlg, WM_SETICON, FALSE, (LPARAM)(pqi->hSmallIcon));

    PostMessage(hDlg, WM_TIMER, 1, 0L);

    sPos.wp.showCmd = pii->nCmdShow;
    SetWindowPlacement(hDlg, &sPos.wp);

    if (SUCCEEDED(CPQDropTarget_CreateInstance(pqi, &lpdt)))
    {
        RegisterDragDrop(hLV, lpdt);
    }

    return(TRUE);
}


void PrinterQueue_Destroy(HWND hDlg)
{
    PQUEUEINFO pqi;
    POSINFO sPos;
    int i;
    HKEY hkey;

    pqi = (PQUEUEINFO)GetWindowPtr(hDlg, DWLP_USER);
    if (!pqi)
    {
        // Nothing was initialized, so nothing to clean up
        return;
    }

    SendMessage(pqi->hDlg, WM_SETICON, TRUE, 0);
    SendMessage(pqi->hDlg, WM_SETICON, FALSE, 0);
    DestroyIcon(pqi->hLargeIcon);
    DestroyIcon(pqi->hSmallIcon);

    RevokeDragDrop(pqi->hwndLV);

    sPos.wp.length = SIZEOF(WINDOWPLACEMENT);
    GetWindowPlacement(hDlg, &sPos.wp);

    // Get the column widths
    for (i=0; i<ICOL_NUM; ++i)
    {
        sPos.nWidths[i] = ListView_GetColumnWidth(pqi->hwndLV, i);
    }
    sPos.fStatusBar = pqi->fStatusBar;

    // Save the position info
    if (ERROR_SUCCESS ==
        RegCreateKey(HKEY_CURRENT_USER, szPrinterPositions, &hkey))
    {
        RegSetValueEx(hkey, pqi->idp.cName, 0, REG_BINARY,
                (LPBYTE)&sPos, SIZEOF(sPos));
        RegCloseKey(hkey);
    }

    if (pqi->hPrinter)
    {
        Printer_ClosePrinter(pqi->hPrinter);
    }

    if (pqi->pJobs)
    {
        LocalFree((HLOCAL)pqi->pJobs);
    }
    LocalFree(pqi);
    SetWindowPtr(hDlg, DWLP_USER, 0L);
}

#endif

// Printer_BitsToString maps bits into a string representation, putting
// the string idsSep in between each found bit.
// Returns the size of the created string.
UINT Printer_BitsToString(
    DWORD          bits,       // the bitfield we're looking at
    UINT           idsSep,     // string id of separator
    LPCSTATUSSTUFF pSS,        // a 0 terminated mapping of bits to string ids
    LPTSTR         lpszBuf,    // output buffer
    UINT           cchMax)     // size of output buffer
{
    UINT cchBuf = 0;
    UINT i = 0;
    UINT cchSep = 0;
    TCHAR szSep[20];

    if (LoadString(HINST_THISDLL, idsSep, szSep, ARRAYSIZE(szSep)))
        cchSep = lstrlen(szSep);

    for ( ; pSS->bit != 0 ; ++i, ++pSS)
    {
        if (bits & pSS->bit)
        {
            TCHAR szTmp[258];

            if (LoadString(HINST_THISDLL, pSS->uStringID, szTmp, ARRAYSIZE(szTmp)))
            {
                UINT cchTmp = lstrlen(szTmp);

                if (cchBuf + cchSep + cchTmp < cchMax)
                {
                    if (cchBuf)
                    {
                        lstrcat(lpszBuf, szSep);
                        cchBuf += cchSep;
                    }
                    lstrcat(lpszBuf, szTmp);
                    cchBuf += cchTmp;
                }
            }
        }
    }

    return(cchBuf);
}

#ifndef WINNT

extern const STATUSSTUFF ssPrinterStatus[]; // defined in printer.c

void Printer_PrinterStatus(PQUEUEINFO pqi)
{
    LPPRINTER_INFO_2 pPrinter;
    LPTSTR  lpszPrinterName;
    TCHAR   szBuf[256];

    // Default window title is the current printer name
    lpszPrinterName = pqi->idp.cName;

    pPrinter = Printer_GetPrinterInfo(pqi->hPrinter, 2);
    if (pPrinter)
    {
        UINT bufLen;
        UINT nameLen;

        if (lstrcmp(pqi->idp.cName, pPrinter->pPrinterName))
        {
            // WHOA! The user renamed this printer
            Printers_FillPidl(&pqi->idp, pPrinter->pPrinterName);

            // BUGBUG: We don't update the hdsaPrintDef (which keeps track
            // of which printers are open) with the new printer name.  So
            // the user can open another window for this printer OR rename a
            // different printer to this one's old name and then get this
            // window when trying to open the other printer.
        }

        if (pPrinter->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
        {
            // HACK: Use this free bit for "Work Offline"
            pPrinter->Status |= PRINTER_HACK_WORK_OFFLINE;
        }

        if (pPrinter->Status)
        {
            lstrcpy(szBuf, lpszPrinterName);
            nameLen = bufLen = lstrlen(szBuf);
            LoadString(HINST_THISDLL, IDS_PRQSTATUS_SEPARATOR,
                szBuf+bufLen, ARRAYSIZE(szBuf)-bufLen);
            bufLen = lstrlen(szBuf);

            Printer_BitsToString(pPrinter->Status, IDS_PRQSTATUS_SEPARATOR,
                ssPrinterStatus, szBuf+bufLen, ARRAYSIZE(szBuf)-bufLen);

            lpszPrinterName = szBuf;
        }

        LocalFree((HLOCAL)pPrinter);
    }

    SetWindowText(pqi->hDlg, lpszPrinterName);
}


// Printer_AddJobs sets the number of items in hLV to be iJobsWanted.
void Printer_AddJobs(HWND hLV, int iJobsWanted)
{
    LV_ITEM item;
    int iJobs;

    // And to the listview
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE ;
    item.iSubItem = 0;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.state = 0;
    item.iImage = -1;

    iJobs = ListView_GetItemCount(hLV);

    for ( ; iJobs<iJobsWanted; ++iJobs)
    {
        item.iItem = iJobs;
        item.lParam = (LPARAM)iJobs;
        if (ListView_InsertItem(hLV, &item) < 0)
        {
            break;
        }
    }

    for ( ; iJobs>iJobsWanted; --iJobs)
    {
        ListView_DeleteItem(hLV, iJobs-1);
    }
}


BOOL Printers_EnumJobsCB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
#ifndef BUGBUG_BOBDAY
    return(EnumJobs(hPrinter, 0, 0x7fffffffL, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
#else
    return(EnumJobs(hPrinter, 1, 0x7fffffffL, dwLevel, pEnum, dwSize, lpdwNeeded, lpdwNum));
#endif
}

void PrinterQueue_Check(HWND hDlg)
{
    PQUEUEINFO pqi = (PQUEUEINFO)GetWindowPtr(hDlg, DWLP_USER);
    DWORD dwJobs;
    int iJob, iSel;
    DWORD dwSelID;
    LPTSTR pszJobsInQueue;

    ASSERT(hDlg == pqi->hDlg);

    if (pqi->bDragSource || pqi->bMinimized)
    {
        // If we are dragging from this window, don't update the queue info!
        // If we did, we'd need to put semaphores around this function and
        // all the drag&drop stuff.  This is much easier.

        // If we are minimized, we don't need to poll either

        return;
    }

    // remember which job is selected
    dwSelID = (DWORD)-1;
    iSel = ListView_GetNextItem(pqi->hwndLV, -1, LVNI_SELECTED);
    if (iSel >= 0)
    {
        dwSelID = pqi->pJobs[iSel].JobId;
    }

    // update title
    Printer_PrinterStatus(pqi);

    // get new job state
    if (pqi->pJobs)
        LocalFree((HLOCAL)pqi->pJobs);
    pqi->pJobs = Printer_EnumProps(pqi->hPrinter, 2, &dwJobs, Printers_EnumJobsCB, NULL);
    if (pqi->pJobs == NULL)
    {
        pqi->dwJobs = dwJobs = 0;
    }

    // Make sure we set pqi->dwJobs before we call anything that might
    // recurse back into PrintQueue_DlgProc where we might fault!
    pqi->dwJobs = dwJobs;

    // Make sure we set pqi->dwJobs before we call anything that might
    // recurse back into PrintQueue_DlgProc where we might fault!
    pqi->dwJobs = dwJobs;

    pszJobsInQueue = ShellConstructMessageString(HINST_THISDLL,
                        MAKEINTRESOURCE(IDS_PRQ_JOBSINQUEUE), dwJobs);
    if (pszJobsInQueue)
    {
        // a-msadek; needed only for BiDi Win95 loc
        // Mirroring will take care of that over NT5 & BiDi Win98
        if(g_bBiDiW95Loc)
        {
            SendMessage(pqi->hwndSB, SB_SETTEXT, (WPARAM)SBT_RTLREADING, (LPARAM)pszJobsInQueue);
        }
        else
        {
            SendMessage(pqi->hwndSB, SB_SETTEXT, (WPARAM)0, (LPARAM)pszJobsInQueue);
        }    
        LocalFree(pszJobsInQueue);
    }

    Printer_AddJobs(pqi->hwndLV, (int)dwJobs);

    // Find the previously selected job and select it again
    iSel = -1;
    for (iJob = 0; iJob < (int)dwJobs; iJob++)
    {
        if (pqi->pJobs[iJob].JobId == dwSelID)
        {
            iSel = iJob;
        }
    }
    ListView_SetItemState(pqi->hwndLV, iSel, iSel == -1 ? 0 : LVIS_SELECTED, LVIS_SELECTED);

    InvalidateRect(pqi->hwndLV, NULL, TRUE);
}


BOOL Printer_ModifyJob(HWND hDlg, DWORD dwCommand)
{
    PQUEUEINFO pqi = (PQUEUEINFO)GetWindowPtr(hDlg, DWLP_USER);
    int iJob = -1;
    DWORD dwJobID;
    BOOL bRet = TRUE;

    while (bRet &&
           (iJob = ListView_GetNextItem(pqi->hwndLV, iJob, LVNI_SELECTED)) >= 0)
    {
        dwJobID = pqi->pJobs[iJob].JobId;

        bRet = SetJob(pqi->hPrinter, dwJobID, 0, NULL, dwCommand);
    }

    // Let the listbox refresh right now
    PostMessage(hDlg, WM_TIMER, 1, 0L);

    return(bRet);
}


#if 0 // not used
BOOL PrinterDlg_ModifyPrinter(PQUEUEINFO pqi, DWORD dwCommand)
{
    BOOL fRet = SetPrinter(pqi->hPrinter, 0, NULL, dwCommand);

    // Let the listbox refresh right now
    PostMessage(pqi->hDlg, WM_TIMER, 1, 0L);

    return fRet;
}
#endif


void PrinterQueue_InitPrinterMenu(HMENU hmPrinter, PQUEUEINFO pqi)
{
    QCMINFO qcm = {hmPrinter, 0, ID_PRINTER_START, 0xffff};
    int i;

    // remove previously merged items
    for (i = GetMenuItemCount(hmPrinter) ; i > 3 ; i--)
    {
        DeleteMenu(hmPrinter, 0, MF_BYPOSITION);
    }

    // merge in context menu
    Printer_MergeMenu(NULL, &qcm, pqi->idp.cName, TRUE);

    // remove Open
    DeleteMenu(hmPrinter, 0, MF_BYPOSITION);
    DeleteMenu(hmPrinter, 0, MF_BYPOSITION);
}


void PrinterQueue_InitDocMenu(HMENU hmDoc, PQUEUEINFO pqi, int iSel)
{
    UINT i = (iSel >= 0 && pqi->pJobs[iSel].Status & JOB_STATUS_PAUSED) ?
                MF_CHECKED : 0 ;

    Printer_CheckMenuItem(hmDoc, i, ID_DOCUMENT_RESUME, ID_DOCUMENT_PAUSE);

    Printer_EnableMenuItems(hmDoc, iSel >= 0);
}

void PrinterQueue_InitViewMenu(HMENU hmView, PQUEUEINFO pqi)
{
    // This seems a bit bogus to me.  We can simply un/check the menu item
    // whenever the user makes the change.  That won't work for the
    // Printer and Document menu's, since they may change asynchronous to
    // user input.  To be consistent, let's follow the same model here.
    CheckMenuItem(hmView, ID_VIEW_STATUSBAR,
        pqi->fStatusBar ? MF_BYCOMMAND|MF_CHECKED : MF_BYCOMMAND|MF_UNCHECKED);
}

//
// load a popup from a main menu
//
HMENU _LoadSubPopupMenu(UINT id, UINT uSubOffset)
{
    HMENU hmParent, hmPopup;

    hmParent = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(id));
    if (!hmParent)
        return NULL;

    hmPopup = GetSubMenu(hmParent, uSubOffset);
    RemoveMenu(hmParent, uSubOffset, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return hmPopup;
}



const STATUSSTUFF ssPRQStatus[] =
{
    JOB_STATUS_DELETING, IDS_PRQSTATUS_PENDING_DELETION,
    JOB_STATUS_ERROR   , IDS_PRQSTATUS_ERROR   ,
    JOB_STATUS_OFFLINE , IDS_PRQSTATUS_OFFLINE ,
    JOB_STATUS_PAPEROUT, IDS_PRQSTATUS_PAPER_OUT,
    JOB_STATUS_PAUSED  , IDS_PRQSTATUS_PAUSED  ,
    JOB_STATUS_PRINTED , IDS_PRQSTATUS_PRINTED ,
    JOB_STATUS_PRINTING, IDS_PRQSTATUS_PRINTING,
    JOB_STATUS_SPOOLING, IDS_PRQSTATUS_SPOOLING,
    JOB_STATUS_USER_INTERVENTION, IDS_PRQSTATUS_USER_INTERVENTION,
    0, 0
} ;

void PrintQueue_Notify(HWND hDlg, NMHDR *lphdr)
{
    PQUEUEINFO pqi = (PQUEUEINFO)GetWindowPtr(hDlg, DWLP_USER);

    if (!pqi)
    {
        return;
    }

    switch (lphdr->code)
    {
    case NM_RCLICK:
    {
        int iSel;
        HMENU hmContext;
        POINT pt;

        iSel = ListView_GetNextItem(pqi->hwndLV, -1, LVNI_SELECTED);
        hmContext = _LoadSubPopupMenu(MENU_PRINTERQUEUE, iSel >= 0 ? 1 : 0);
        if (!hmContext)
        {
            break;
        }

        if (iSel < 0)
        {
            // We need to remove the "Close" menu item
            // (and separator)
            iSel = GetMenuItemCount(hmContext) - 2;
            DeleteMenu(hmContext, iSel, MF_BYPOSITION);
            DeleteMenu(hmContext, iSel, MF_BYPOSITION);

            PrinterQueue_InitPrinterMenu(hmContext, pqi);
        }
        else
        {
            PrinterQueue_InitDocMenu(hmContext, pqi, iSel);
        }

        GetMsgPos(&pt);

        // The command will just get stuck in the regular queue and
        // handled at that time
        TrackPopupMenu(hmContext, TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y,
            0, hDlg, NULL);

        DestroyMenu(hmContext);
        // Tell the listview that we handled the context menu
        // so don't try to display some generic ctx menu for us
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
        break;
    }

    case LVN_GETDISPINFO:
    {
        LV_DISPINFO *lpdi = (LV_DISPINFO *)lphdr;
        JOB_INFO_2 *pJob;

        if (lpdi->item.mask & LVIF_IMAGE)
        {
            lpdi->item.iImage = 0;
        }

        if (!(lpdi->item.mask & LVIF_TEXT))
        {
            // we only have info for LVIF_TEXT
            break;
        }

        // Why is this using lParam? iItem tells us the selection...
        // Stress page faulted referencing pJob+lpdi->item.lParam.
        // Shouldn't have happened, so put these asserts here and
        // bail if lParam is bad.
        ASSERT(lpdi->item.lParam == lpdi->item.iItem);

        pJob = pqi->pJobs;
        if (!pJob || (DWORD)lpdi->item.lParam >= pqi->dwJobs)
        {
            return;
        }
        pJob += lpdi->item.lParam;

        switch (lpdi->item.iSubItem)
        {
        case ICOL_DOCNAME:
            if (pJob->pDocument)
                lpdi->item.pszText = pJob->pDocument;
            break;

        case ICOL_STATUS:
            if (pJob->pStatus && *(pJob->pStatus))
            {
                lstrcpyn(lpdi->item.pszText, pJob->pStatus, lpdi->item.cchTextMax);
            }
            else
            {
                Printer_BitsToString(pJob->Status, IDS_PRQSTATUS_SEPARATOR,
                    ssPRQStatus, lpdi->item.pszText, lpdi->item.cchTextMax);
            }
            break;

        case ICOL_OWNER:
            if (pJob->pUserName)
                lpdi->item.pszText = pJob->pUserName;
            break;

        case ICOL_PROGRESS:
        {
            LPTSTR pszFormat = NULL;

            // If TotalPages == 0 and Size != 0, PagesPrinted is SizePrinted

            if (pJob->Status&JOB_STATUS_PRINTING)
            {
                if (pJob->TotalPages == 0)
                {
                    TCHAR szSize[32];
                    TCHAR szPrinted[32];

                    ShortSizeFormat(pJob->Size, szSize);
                    ShortSizeFormat(pJob->PagesPrinted, szPrinted);

                    pszFormat = ShellConstructMessageString(HINST_THISDLL,
                                    MAKEINTRESOURCE(IDS_PRQ_BYTESPRINTED),
                                    szPrinted, szSize);
                }
                else
                {
                    pszFormat = ShellConstructMessageString(HINST_THISDLL,
                                    MAKEINTRESOURCE(IDS_PRQ_PAGESPRINTED),
                                    pJob->PagesPrinted, pJob->TotalPages);
                }
            }
            else
            {
                if (pJob->TotalPages == 0)
                {
                    ShortSizeFormat(pJob->Size, lpdi->item.pszText);
                }
                else
                {
                    pszFormat = ShellConstructMessageString(HINST_THISDLL,
                                    MAKEINTRESOURCE(IDS_PRQ_PAGES),
                                    pJob->TotalPages);
                }
            }
            if (pszFormat)
            {
                lstrcpyn(lpdi->item.pszText, pszFormat, lpdi->item.cchTextMax);
                LocalFree(pszFormat);
            }
            break;
        }

        case ICOL_TIME:
            SystemTimeString(lpdi->item.pszText, &pJob->Submitted);
            break;

        default:
            break;
        }
        break;
    }

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        PRQ_BeginDrag(hDlg, pqi, (NM_LISTVIEW *)lphdr);
        break;

    default:
        break;
    }
}

void PrinterQueue_OnSize(PQUEUEINFO pqi)
{
    int dyList;
    int dyStatus = 0;

    if (pqi->fStatusBar)
    {
        RECT rc;
        //
        // REVIEW: Is this the right way to do this?
        //
        GetWindowRect(pqi->hwndSB, &rc);
        dyStatus = min(pqi->cy, rc.bottom - rc.top - 2);
    }
    dyList = pqi->cy - dyStatus;

    SetWindowPos(pqi->hwndLV, NULL, 0, 0, pqi->cx, dyList, SWP_NOZORDER);
    SetWindowPos(pqi->hwndSB, NULL, dyList, 0, pqi->cx, dyStatus, SWP_NOZORDER);
}


extern TCHAR const c_szWindowsHlp[];

BOOL CALLBACK PrinterQueue_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PQUEUEINFO pqi = (PQUEUEINFO)GetWindowPtr(hDlg, DWLP_USER);

    INSTRUMENT_WNDPROC(SHCNFI_PRINTERQUEUE_DLGPROC, hDlg, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        return(PrinterQueue_Init(hDlg, (INITINFO *)lParam));

    case WM_DESTROY:
        PrinterQueue_Destroy(hDlg);
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            pqi->bMinimized = TRUE;
        }
        else
        {
            if (pqi->bMinimized)
            {
                pqi->bMinimized = FALSE;

                // Restoring from a minimized state -- update queue
                PostMessage(hDlg, WM_TIMER, 1, 0L);
            }

            pqi->cx = GET_X_LPARAM(lParam);
            pqi->cy = GET_Y_LPARAM(lParam);
            PrinterQueue_OnSize(pqi);
        }
        break;

    case WM_CONTEXTMENU:
        if (hDlg == (HWND)wParam &&
            SendMessage(hDlg, WM_NCHITTEST, 0, lParam) == HTSYSMENU)
        {
            HMENU hmContext = _LoadSubPopupMenu(MENU_PRINTERQUEUE, 0);
            if (hmContext)
            {
                int idCmd;

                PrinterQueue_InitPrinterMenu(hmContext, pqi);

                idCmd = TrackPopupMenu(hmContext,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hDlg, NULL);

                switch(idCmd)
                {
                case 0:
                    break;
                case IDCANCEL:
                    PostQuitMessage(0);
                    break;
                default:
                    Printer_InvokeCommand(hDlg, NULL, &(pqi->idp), idCmd-ID_PRINTER_START, 0, NULL);
                    break;
                }

                DestroyMenu(hmContext);

                return TRUE;
            }
        }
        return FALSE;

    case WM_TIMER:
        PrinterQueue_Check(hDlg);

        // Check the queue every 10 seconds
        SetTimer(hDlg, 1, 10000, NULL);
        break;

    case WM_INITMENU:
        if ((HMENU)wParam != GetMenu(hDlg))
        {
            break;
        }

        PrinterQueue_InitPrinterMenu(GetSubMenu((HMENU)wParam, 0), pqi);

        PrinterQueue_InitDocMenu(GetSubMenu((HMENU)wParam, 1), pqi,
            ListView_GetNextItem(pqi->hwndLV, -1, LVNI_SELECTED));

        PrinterQueue_InitViewMenu(GetSubMenu((HMENU)wParam, 2), pqi);

        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            PostQuitMessage(0);
            break;

        case ID_DOCUMENT_PAUSE:
            if (!Printer_ModifyJob(hDlg, JOB_CONTROL_PAUSE))
                goto WarnOnError;
            break;

        case ID_DOCUMENT_RESUME:
            if (!Printer_ModifyJob(hDlg, JOB_CONTROL_RESUME))
                goto WarnOnError;
            break;

        case ID_DOCUMENT_DELETE:
            if (!Printer_ModifyJob(hDlg, JOB_CONTROL_CANCEL))
            {
WarnOnError:;
#ifndef WINNT
                Printer_WarnOnError(hDlg, pqi->idp.cName, IDS_SECURITYDENIED_JOB);
#endif
            }
            break;

//      case ID_VIEW_TOOLBAR:
//          break;

        case ID_VIEW_STATUSBAR:
            pqi->fStatusBar = 1-pqi->fStatusBar;
            ShowWindow(pqi->hwndSB, pqi->fStatusBar ? SW_SHOW : SW_HIDE);
            PrinterQueue_OnSize(pqi);
            break;

        // even though this isn't on the menu any more, printobj.c sends it:
        // we also have an accelerator for it:
        case ID_VIEW_REFRESH:
            PostMessage(hDlg, WM_TIMER, 1, 0L);
            break;

        // from cabinet\command.c: DoAboutChicago
        case ID_HELP_ABOUT:
        {
            TCHAR szChicago[64];
            LoadString(HINST_THISDLL, IDS_WINDOWS, szChicago, ARRAYSIZE(szChicago));
            ShellAbout(hDlg, szChicago, NULL, NULL);
            break;
        }

        // from defviewx.c: SFVIDM_HELP_TOPIC
        case ID_HELP_CONTENTS:
            //
            // REVIEW: Should we jump to a topic which describes current
            //  viewed folder?
            //
            WinHelp(hDlg, c_szWindowsHlp, HELP_FINDER, 0);
            break;

        default:
            // Must have been merged in from the printer's context menu
            Printer_InvokeCommand(hDlg, NULL, &(pqi->idp), wParam-ID_PRINTER_START, 0, NULL);
            break;
        }
        break;

    case WM_NOTIFY:
        PrintQueue_Notify(hDlg, (NMHDR *)lParam);
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

void Printer_ViewQueue(HWND hwndStub, LPCTSTR lpszCmdLine, int nCmdShow, LPARAM lParam)
{
    LPPRINTER_INFO_5 pPrinter;
    BOOL fFile = FALSE;

    pPrinter = Printer_GetPrinterInfoStr(lpszCmdLine, 5);
    if (pPrinter)
    {
        fFile = !lstrcmp(pPrinter->pPortName, c_szFileColon);
        LocalFree((HLOCAL)pPrinter);
    }

    if (fFile)
    {
        ShellMessageBox(HINST_THISDLL, hwndStub,
            MAKEINTRESOURCE(IDS_CANTVIEW_FILEPRN), lpszCmdLine,
            MB_OK|MB_ICONINFORMATION);
    }
    else if (pPrinter)
    {
        INITINFO ii;
        HWND hwndDlg;

        ii.lpszPrinterName = lpszCmdLine;
        ii.nCmdShow = nCmdShow;

        // Call CreateDialog and do our own message loop w/ TranslateAccelerator
        hwndDlg = CreateDialogParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_PRN_QUEUE), hwndStub,
            PrinterQueue_DlgProc, (LPARAM)(LPTSTR)&ii);
        if (hwndDlg)
        {
            HACCEL hAccel;
            MSG msg;

            hAccel = LoadAccelerators(HINST_THISDLL, MAKEINTRESOURCE(ACCEL_PRN_QUEUE));
            while (GetMessage(&msg, NULL, 0, 0))
            {
                if (!TranslateAccelerator(hwndDlg, hAccel, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            DestroyWindow(hwndDlg);
        }
    }
    else
    {
        // If you rename a printer and then try to open a link to that
        // printer, we hit this case.  An error message is appropriate?
        ShellMessageBox(HINST_THISDLL, hwndStub,
            MAKEINTRESOURCE(IDS_PRINTERNAME_CHANGED), lpszCmdLine,
            MB_OK|MB_ICONINFORMATION);
    }
}

#endif
