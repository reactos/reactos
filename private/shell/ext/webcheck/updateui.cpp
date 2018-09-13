#include "private.h"
#include "offline.h"
#include "updateui.h"

//xnotfmgr - most of this file can probably get nuked

#define MAX_CAPTION     128

#undef  TF_THISMODULE
#define TF_THISMODULE   TF_UPDATEAGENT

typedef CLSID   COOKIE, *PCOOKIE;

#define SUBITEM_SIZE    4
#define SUBITEM_URL     3
#define SUBITEM_STATUS  2
#define SUBITEM_IMAGE   1

ColInfoType colDlg[] = {
    {0, IDS_NAME_COL,   30, LVCFMT_LEFT},
    {1, 0,              3,  LVCFMT_LEFT}, 
    {2, IDS_STATUS_COL, 10, LVCFMT_LEFT},
    {3, IDS_URL_COL,    40, LVCFMT_LEFT},
    {4, IDS_SIZE_COL,   7,  LVCFMT_RIGHT}
};

#define ILI_SUCCEEDED       0
#define ILI_FAILED          1
#define ILI_UPDATING        2
#define ILI_PENDING         3
#define ILI_SKIPPED         4
#define ILI_SITE            5
#define ILI_CHANNEL         6
#define ILI_DESKTOP         7

const int g_aIconResourceID[] = {
        IDI_STAT_SUCCEEDED,
        IDI_STAT_FAILED, 
        IDI_STAT_UPDATING,
        IDI_STAT_PENDING, 
        IDI_STAT_SKIPPED,
        IDI_WEBDOC,
        IDI_CHANNEL,
        IDI_DESKTOPITEM
};

#define MAX_DLG_COL     ARRAYSIZE(colDlg)

//struct for saving window state in registry
typedef struct _PROG_PERSIST_STATE
{
    short cbSize;
    char bDetails;
    char bAdjustWindowPos;
    RECT rWindow;
    int colOrder [MAX_DLG_COL];
    int colWidth [MAX_DLG_COL];
} PROG_PERSIST_STATE;
extern void ResizeDialog(HWND hDlg, BOOL bShowDetail);  //in update.cpp

const TCHAR c_szProgressWindowSettings[] = TEXT("Progress Preferences");

const UINT CookieSeg = 32;

CCookieItemMap::CCookieItemMap()
{
    _map = NULL;
}

CCookieItemMap::~CCookieItemMap()
{
    SAFELOCALFREE (_map);
}

STDMETHODIMP CCookieItemMap::Init(UINT size)
{
    //  Free old junk.
    SAFELOCALFREE (_map);

    _lParamNext = 0;
    _count = 0;

    if (size == 0)
        _capacity = CookieSeg;
    else
        _capacity = size;


    _map = (CookieItemMapEntry * )MemAlloc(LPTR, sizeof (CookieItemMapEntry) * _capacity);
    if (!_map)  {
        DBG("Failed to allocate memory");
        _capacity = 0;
        return E_OUTOFMEMORY;
    }

    
    return S_OK;
}

STDMETHODIMP CCookieItemMap::ResetMap(void)
{
    _count = 0;
    return S_OK;
}

STDMETHODIMP CCookieItemMap::FindCookie(LPARAM lParam, CLSID * pCookie)
{
    ASSERT(pCookie);

    UINT    i;
    for (i = 0; i < _count; i ++)   {
        if (_map[i]._id == lParam)    {
            * pCookie = _map[i]._cookie;
            return S_OK;
        }
    }

    *pCookie = CLSID_NULL;
    return E_FAIL;
}

STDMETHODIMP CCookieItemMap::FindLParam(CLSID * pCookie, LPARAM * pLParam)
{
    ASSERT(pCookie && pLParam);

    UINT    i;
    for (i = 0; i < _count; i ++)   {
        if (_map[i]._cookie == *pCookie)    {
            * pLParam = _map[i]._id;
            return S_OK;
        }
    }

    *pLParam = (LPARAM)-1;
    return E_FAIL;
}

STDMETHODIMP CCookieItemMap::AddCookie(CLSID * pCookie, LPARAM * pLParam)
{
    HRESULT hr = FindLParam(pCookie, pLParam);
    if (S_OK == hr)
        return S_FALSE;

    ASSERT(_count <= _capacity);
    ASSERT(CookieSeg);

    if (_count == _capacity)   {
        UINT    newSize = CookieSeg + _capacity;
        void * newBuf = MemReAlloc(_map, newSize * sizeof(CookieItemMapEntry), LHND);

        if (!newBuf)    {
            DBG("AddCookie::Failed to reallocate buffer");
            return E_OUTOFMEMORY;
        }

        _map = (CookieItemMapEntry *)newBuf;
        _capacity = newSize;
    }

    _map[_count]._cookie = *pCookie;
    _map[_count]._id = _lParamNext;
    _count ++;

    *pLParam = _lParamNext;
    _lParamNext ++;
    return S_OK;
}

STDMETHODIMP CCookieItemMap::DelCookie(CLSID * pCookie)
{
    ASSERT(pCookie);

    UINT    i;
    for (i = 0; i < _count; i ++)   {
        if (_map[i]._cookie == *pCookie)    {
            if (i == (_count - 1))  {
                _count --;
                return S_OK;
            } else  {
                _count --;
                _map[i]._cookie = _map[_count]._cookie;
                _map[i]._id = _map[_count]._id;
                return S_OK;
            }
        }
    }

    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
// Other members
//

int CALLBACK CUpdateDialog::SortUpdatingToTop  (LPARAM lParam1,
                                                LPARAM lParam2,
                                                LPARAM lParamSort)
{
    //lparams are cookies; lparamsort is source object
    CUpdateDialog * pUpdater = (CUpdateDialog*)lParamSort;
    if (!pUpdater)
        return 0;
    if (!pUpdater->m_pController)
        return 0;

    CLSID cookie;
    pUpdater->cookieMap.FindCookie (lParam1, &cookie);
    PReportMap pEntry1 = pUpdater->m_pController->FindReportEntry (&cookie);
    pUpdater->cookieMap.FindCookie (lParam2, &cookie);
    PReportMap pEntry2 = pUpdater->m_pController->FindReportEntry (&cookie);
    
    //in progress precedes all else
    if (pEntry1->status == ITEM_STAT_UPDATING)
        return (pEntry2->status == ITEM_STAT_UPDATING ? 0 : -1);

    if (pEntry2->status == ITEM_STAT_UPDATING)
        return 1;

    //queued precedes succeeded or skipped
    if (pEntry1->status == ITEM_STAT_QUEUED || pEntry1->status == ITEM_STAT_PENDING)
        return ((pEntry2->status == ITEM_STAT_QUEUED
              || pEntry2->status == ITEM_STAT_PENDING) ? 0 : -1);

    if (pEntry2->status == ITEM_STAT_QUEUED || pEntry2->status == ITEM_STAT_PENDING)
        return 1;

    return 0;   //don't care
}


BOOL CUpdateDialog::SelectFirstUpdatingSubscription()
{
    LV_ITEM lvi = {0};
    lvi.iSubItem = SUBITEM_IMAGE;
    lvi.mask = LVIF_IMAGE;

    int cItems = ListView_GetItemCount (m_hLV);
    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
    {
        ListView_GetItem (m_hLV, &lvi);
        if (lvi.iImage == ILI_UPDATING)
        {
            ListView_SetItemState (m_hLV, lvi.iItem, LVIS_SELECTED, LVIS_SELECTED);
            return TRUE;
        }
    }

    return FALSE;
}


DWORD CUpdateDialog::SetSiteDownloadSize (PCOOKIE pCookie, DWORD dwNewSize)
{
    //returns previous size, for bookkeeping purposes
    HRESULT             hr;

    TCHAR               szKSuffix[10];
    //  Need enough room for DWORD as string + K suffix
    TCHAR               szBuf[ARRAYSIZE(szKSuffix) + 11];

    if (dwNewSize == -1)            //shouldn't happen anymore but if it does,
        return -1;                  //deal gracefully

    ASSERT(pCookie);
    LPARAM itemParam;
    hr = cookieMap.FindLParam(pCookie, &itemParam);
    if (S_OK != hr)
    {
        return dwNewSize;
    }

    LV_ITEM     lvi = {0};
    LV_FINDINFO lvfi = {0};

    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = itemParam;

    lvi.iItem = ListView_FindItem(m_hLV, -1, &lvfi);
    if (lvi.iItem == -1)
        return dwNewSize;

    lvi.iSubItem = SUBITEM_SIZE;
    lvi.mask = LVIF_TEXT;
    lvi.pszText = szBuf;
    lvi.cchTextMax = sizeof(szBuf);

    ListView_GetItem(m_hLV, &lvi);
    DWORD dwOldSize = StrToInt (szBuf);

    MLLoadString (IDS_SIZE_KB, szKSuffix, ARRAYSIZE(szKSuffix));
    wnsprintf (szBuf, ARRAYSIZE(szBuf), "%d%s", dwNewSize, szKSuffix);
    ListView_SetItem(m_hLV, &lvi);

    return dwOldSize;
}


//  BUGBUG: This method is actually called from the second thread(only). So far
//  I haven't found any sync problem yet. We only change the internal state
//  of this object after it's creation in this method, so we won't mess
//  it up. About UI, there is a chance when we try to disable 'Skip'
//  button, we may come across another request from the primary thread. Since
//  these 2 requests are both designated to disable the button, it won't
//  matter anyway.

STDMETHODIMP CUpdateDialog::RefreshStatus(PCOOKIE pCookie, LPTSTR name, STATUS newStat, LPTSTR extraInfo)
{
    HRESULT             hr;
    TCHAR               szBuf[MAX_URL];
    
    ASSERT(pCookie);
    LPARAM itemParam;
    hr = cookieMap.FindLParam(pCookie, &itemParam);
    if (S_OK != hr) {
        if (name)  {
            hr = AddItem(pCookie, name, newStat);
            if (S_OK != hr) {
                return E_FAIL;
            }
            hr = cookieMap.FindLParam(pCookie, &itemParam);
            if (S_OK != hr) {
                ASSERT(0);
                return E_FAIL;
            }
        } else  {
            return hr;
        }
    }

    LV_ITEM     lvi = {0};
    LV_FINDINFO lvfi = {0};

    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = itemParam;

    lvi.iItem = ListView_FindItem(m_hLV, -1, &lvfi);
    if (lvi.iItem == -1)
        return E_FAIL;

    lvi.iSubItem = SUBITEM_STATUS;
    lvi.mask = LVIF_TEXT;
    
    ASSERT ((UINT)newStat <= ITEM_STAT_ABORTED);
    if (newStat == ITEM_STAT_UPDATING && extraInfo != NULL) //url available, use it
    {
        TCHAR szFormat[MAX_URL];
        MLLoadString (IDS_ITEM_STAT_UPDATING_URL, szFormat, ARRAYSIZE(szFormat));
        wnsprintf (szBuf, ARRAYSIZE(szBuf), szFormat, extraInfo);
    }
    else
    {
        MLLoadString(IDS_ITEM_STAT_IDLE + newStat, szBuf, ARRAYSIZE(szBuf));
    }

    lvi.pszText = szBuf;
    ListView_SetItem(m_hLV, &lvi);

    lvi.iSubItem = SUBITEM_IMAGE;
    lvi.mask = LVIF_IMAGE;
    
    switch (newStat)  {
        case ITEM_STAT_QUEUED:
        case ITEM_STAT_PENDING:
            lvi.iImage = ILI_PENDING;
            break;
        case ITEM_STAT_UPDATING:
            lvi.iImage = ILI_UPDATING;
            //move to top of list -- t-mattgi
            //this happens in sort callback function -- just force resort
            //after we update the LV control
            break;
        case ITEM_STAT_SUCCEEDED:
            lvi.iImage = ILI_SUCCEEDED;
            if (ListView_GetItemState(m_hLV, lvi.iItem, LVIS_SELECTED))
                Button_Enable(GetDlgItem(m_hDlg, IDCMD_SKIP), FALSE);
            break;
        case ITEM_STAT_SKIPPED:
            if (ListView_GetItemState(m_hLV, lvi.iItem, LVIS_SELECTED))
                Button_Enable(GetDlgItem(m_hDlg, IDCMD_SKIP), FALSE);
            lvi.iImage = ILI_SKIPPED;
            break;
        default:
            lvi.iImage = ILI_FAILED;
            break;
    }
    ListView_SetItem(m_hLV, &lvi);

    //force resort since item statuses changed
    ListView_SortItems (m_hLV, SortUpdatingToTop, this);

    return hr;
}

CUpdateDialog::CUpdateDialog()
{
    m_bInitialized = FALSE;
}

CUpdateDialog::~CUpdateDialog()
{
}

STDMETHODIMP CUpdateDialog::CleanUp()
{
    if (! m_ThreadID || !m_bInitialized)
        return S_OK;

    if (m_hDlg)
    {
        PersistStateToRegistry (m_hDlg);

        DestroyWindow(m_hDlg);
        m_hDlg = NULL;
    }
    PostThreadMessage(m_ThreadID, WM_QUIT, 0, 0);
    return S_OK;
}

STDMETHODIMP CUpdateDialog::Init(HWND hParent, CUpdateController * pController)
{
    ASSERT(m_ThreadID);
    ASSERT(g_hInst);
    ASSERT(pController);
    if (m_bInitialized) {
        ASSERT(0);
        return S_FALSE;
    }

    if (FAILED(cookieMap.Init()))
        return E_FAIL;

    HWND hDlg, hLV;

    m_pController = pController;
    hDlg = CreateDialogParam(MLGetHinst(), MAKEINTRESOURCE(IDD_PROGRESS), hParent, UpdateDlgProc, (LPARAM)this);
    if (!hDlg)
        return E_FAIL;

    hLV = GetDlgItem(hDlg, IDL_SUBSCRIPTION);
    if (!hLV)   {
        EndDialog(hDlg, FALSE);
        return E_FAIL;
    }

    HIMAGELIST  hImage;
    HICON       hIcon;

    hImage = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CXSMICON),
                              ILC_MASK,    
                              ARRAYSIZE(g_aIconResourceID),
                              0);

    if (hImage == NULL) {
        TraceMsg(TF_ALWAYS, TEXT("CUpdateDialog::Init - Failed to create ImageList"));
        return E_FAIL;
    }

    for (int i = 0; i < ARRAYSIZE(g_aIconResourceID); i ++) {
        if (g_aIconResourceID[i] == IDI_DESKTOPITEM)
        {
            hinstSrc = MLGetHinst();
        }
        else
        {
            hinstSrc = g_hInst;
        }
        
        hIcon = LoadIcon(hinstSrc, MAKEINTRESOURCE(g_aIconResourceID[i]));
        if (hIcon == NULL)  {
            ImageList_Destroy(hImage);
            DBG("CUpdateDialog::Init - Failed to load icon");
            return E_FAIL;
        }
        ImageList_AddIcon(hImage, hIcon);
        DestroyIcon(hIcon);
    }
                            
    ListView_SetImageList(hLV, hImage, LVSIL_SMALL);

    LV_COLUMN   lvc;
    TEXTMETRIC  tm;
    HDC         hdc;

    hdc = GetDC(hDlg);
    if (!hdc)   {
        EndDialog(hDlg, FALSE);
        return E_FAIL;
    }
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hDlg, hdc);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;

    PROG_PERSIST_STATE state;
    GetPersistentStateFromRegistry(state, tm.tmAveCharWidth);
    
    for (UINT ui = 0; ui < MAX_DLG_COL; ui ++)
    {
        int colIndex;

        TCHAR   szCaption[MAX_CAPTION];
        if (colDlg[ui].ids)
            MLLoadString(colDlg[ui].ids, szCaption, MAX_CAPTION);
        else
            szCaption[0] = (TCHAR)0;

        lvc.pszText = szCaption;
        lvc.cx = state.colWidth[ui];
        lvc.fmt = colDlg[ui].iFmt;
        colIndex = ListView_InsertColumn(hLV, ui, & lvc);
        if ( -1 == colIndex)    {
            ASSERT(0);
            EndDialog(hDlg, FALSE);
            return E_FAIL;
        }
    }

    ListView_SetColumnOrderArray(hLV, MAX_DLG_COL, state.colOrder);

    SendMessage (hLV, LVM_SETEXTENDEDLISTVIEWSTYLE,
        LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES,
        LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES);

    SendMessage (hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon (g_hInst, MAKEINTRESOURCE (IDI_DOWNLOAD)));
    SendMessage (hDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon (g_hInst, MAKEINTRESOURCE (IDI_DOWNLOAD)));

    if (state.bAdjustWindowPos)
    {
        //adjust size of *details view* to stored state; if we're not in
        //details view, we have to go there temporarily.  (The non-details
        //view will pick up the position from the details view when we switch back.)
        m_bDetail = TRUE;
        ResizeDialog (hDlg, m_bDetail);

        //don't move dialog, just resize it and center it
        //convert right, bottom coordinates to width, height
        state.rWindow.right -= state.rWindow.left;
        state.rWindow.bottom -= state.rWindow.top;
        //calculate left, top to center dialog
        state.rWindow.left = (GetSystemMetrics (SM_CXSCREEN) - state.rWindow.right) / 2;
        state.rWindow.top = (GetSystemMetrics (SM_CYSCREEN) - state.rWindow.bottom) / 2;
        MoveWindow (hDlg, state.rWindow.left, state.rWindow.top,
                    state.rWindow.right, state.rWindow.bottom, TRUE);

        //REVIEW: this centers the details view, then if they don't want details,
        //leaves the small dialog with its upper left where the upper left of the
        //big dialog is when it's centered.  I could center it in whatever view
        //it's really in, but if the details view is resized to a fairly large window
        //and we bring it up centered in non-details, then when they click details
        //the position will be the same and the window will potentially extend offscreen
        //to the right and bottom.

        //set back to non-details view if that was how it was last used
        if (!state.bDetails)
        {
            m_bDetail = state.bDetails;
            ResizeDialog (hDlg, m_bDetail);
        }
    }
    
    m_bInitialized = TRUE;
    m_hDlg = hDlg;
    m_hLV  = hLV;
    m_hParent = hParent;
    m_cDlKBytes = 0;
    m_cDlDocs = 0;

    return S_OK;
}


BOOL CUpdateDialog::PersistStateToRegistry (HWND hDlg)
{
    PROG_PERSIST_STATE state;

    state.cbSize = sizeof(state);
    state.bDetails = m_bDetail;
    state.bAdjustWindowPos = TRUE;
    //save position and size from *details view* -- if we're not there,
    //we'll have to switch temporarily.
    BOOL bTempDetail = m_bDetail;
    if (!bTempDetail)
    {
        ShowWindow (hDlg, SW_HIDE);
        m_bDetail = TRUE;
        ResizeDialog (hDlg, m_bDetail);
    }
    GetWindowRect (hDlg, &state.rWindow);
    if (!bTempDetail)
    {
        m_bDetail = FALSE;
        ResizeDialog (hDlg, m_bDetail);
        ShowWindow (hDlg, SW_SHOW);
    }

    HWND hLV = GetDlgItem (hDlg, IDL_SUBSCRIPTION);
    ListView_GetColumnOrderArray (hLV, MAX_DLG_COL, state.colOrder);
    for (int i=0; i<MAX_DLG_COL; i++)
        state.colWidth[i] = ListView_GetColumnWidth (hLV, i);

    HKEY key;
    DWORD dwDisposition;
    if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_CURRENT_USER, c_szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                         KEY_WRITE, NULL, &key, &dwDisposition))
        return FALSE;

    RegSetValueEx (key, c_szProgressWindowSettings, 0, REG_BINARY, (LPBYTE)&state, sizeof(state));

    RegCloseKey (key);

    return TRUE;
}

BOOL CUpdateDialog::GetPersistentStateFromRegistry (PROG_PERSIST_STATE& state, int iCharWidth)
{
    HKEY key;
    DWORD dwType;
    DWORD dwSize = sizeof(state);
    RegOpenKeyEx (HKEY_CURRENT_USER, c_szRegKey, 0, KEY_READ, &key);
    
    LONG result = RegQueryValueEx (key, c_szProgressWindowSettings,
            0, &dwType, (LPBYTE)&state, &dwSize);

    if (ERROR_SUCCESS != result || dwType != REG_BINARY || dwSize != sizeof(state))
    {
        state.cbSize = 0;       //flag as error
    }

    if (state.cbSize != sizeof(state))  //error or incorrect registry format/version
    {   //state not saved in registry; use defaults
        int i;

        state.bDetails = FALSE;
        state.bAdjustWindowPos = FALSE;

        state.colOrder[0] = 1;
        state.colOrder[1] = 0;
        for (i=2; i<MAX_DLG_COL; i++)
            state.colOrder[i] = i;

        for (i=0; i<MAX_DLG_COL; i++)
            state.colWidth[i] = colDlg[i].cchCol * iCharWidth;
    }

    RegCloseKey (key);
    return TRUE;
}


STDMETHODIMP CUpdateDialog::Show(BOOL bShow)
{
    if (!m_bInitialized)    {
        ASSERT(0);
        return E_FAIL;
    } 

    ASSERT(m_hDlg);

    ShowWindow(m_hDlg, (bShow)?SW_SHOW:SW_HIDE);
    ShowWindow(m_hLV, (bShow)?SW_SHOW:SW_HIDE);
    return NOERROR;
}

STDMETHODIMP CUpdateDialog::ResetDialog(void)
{
    if (!m_bInitialized)    {
        return S_OK;
    }

    ASSERT(m_hLV);
    ListView_DeleteAllItems(m_hLV);
    cookieMap.ResetMap();
    m_bInitialized = FALSE;

    return S_OK;
}

STDMETHODIMP CUpdateDialog::IItem2Cookie(const int iItem, CLSID * pCookie)
{
    LV_ITEM item = {0};
    HRESULT hr;
    ASSERT(pCookie);

    item.iItem = iItem;
    item.iSubItem = 0;
    item.mask = LVIF_PARAM;

    if (!ListView_GetItem(m_hLV, &item))    {
        return E_FAIL;
    }
    
    hr = cookieMap.FindCookie(item.lParam, pCookie);
    ASSERT(SUCCEEDED(hr));

    return hr;
}

STDMETHODIMP CUpdateDialog::GetSelectedCookies(CLSID * pCookies, UINT * pCount)
{
    if (!m_bInitialized)
        return E_INVALIDARG;
    
    ASSERT(pCookies && pCount);
    int     index = -1;

    *pCount = 0;

    index = ListView_GetNextItem(m_hLV, index, LVNI_ALL | LVNI_SELECTED);
    while (-1 != index) {
        if (FAILED(IItem2Cookie(index, pCookies + *pCount)))    {
            ASSERT(0);
            return S_FALSE;
        }

        index = ListView_GetNextItem(m_hLV, index, LVNI_ALL | LVNI_SELECTED);
        *pCount = *pCount + 1;
    }

    return S_OK;
}

STDMETHODIMP CUpdateDialog::GetSelectionCount(UINT * pCount)
{
    if (!m_bInitialized)
        return E_INVALIDARG;

    ASSERT(pCount);
    * pCount = ListView_GetSelectedCount(m_hLV);

    return S_OK;
}

STDMETHODIMP CUpdateDialog::AddItem(CLSID * pCookie, LPTSTR name, STATUS stat)
{
    HRESULT             hr;
    TCHAR               szBuf[MAX_URL];

    LV_ITEM lvi = {0};
    BOOL    bNew;

    lvi.iSubItem = 0;
    hr = cookieMap.AddCookie(pCookie, &(lvi.lParam));
    if (S_OK == hr) {
        bNew = TRUE;
    } else if (S_FALSE == hr)   {
        bNew = FALSE;
    } else  {
        return hr;
    }

    lvi.pszText = name;
    if (bNew)   {
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        switch (m_pController->GetSubscriptionType(pCookie))
        {
        case SUBSTYPE_CHANNEL:
            lvi.iImage = ILI_CHANNEL;
            break;
        case SUBSTYPE_DESKTOPURL:
        case SUBSTYPE_DESKTOPCHANNEL:
            lvi.iImage = ILI_DESKTOP;
            break;
        case SUBSTYPE_URL:
        default:
            lvi.iImage = ILI_SITE;
            break;
        }
        lvi.iItem = ListView_InsertItem(m_hLV, &lvi);
        if (lvi.iItem == -1)
            return E_FAIL;
        if (lvi.iItem == 0) {
            ListView_SetItemState(m_hLV, 0, LVIS_SELECTED, LVIS_SELECTED);
        }
    } else  {
        LV_FINDINFO lvfi = {0};

        lvfi.flags = LVFI_PARAM;
        lvfi.lParam = lvi.lParam;
    
        lvi.iItem = ListView_FindItem(m_hLV, -1, &lvfi);
        if (lvi.iItem == -1)
            return E_FAIL;

        lvi.mask = LVIF_TEXT;
        ListView_SetItem(m_hLV, &lvi);
    }

    //add subitem for status icon
    lvi.mask = LVIF_IMAGE;
    lvi.iSubItem ++;    //  Icon field.
    switch (stat)  {
        case ITEM_STAT_QUEUED:
        case ITEM_STAT_PENDING:
            lvi.iImage = ILI_PENDING;
            break;
        case ITEM_STAT_UPDATING:
            lvi.iImage = ILI_UPDATING;
            break;
        case ITEM_STAT_SUCCEEDED:
            lvi.iImage = ILI_SUCCEEDED;
            if (ListView_GetItemState(m_hLV, lvi.iItem, LVIS_SELECTED))
                Button_Enable(GetDlgItem(m_hDlg, IDCMD_SKIP), FALSE);
            break;
        case ITEM_STAT_SKIPPED:
            lvi.iImage = ILI_SKIPPED;
            if (ListView_GetItemState(m_hLV, lvi.iItem, LVIS_SELECTED))
                Button_Enable(GetDlgItem(m_hDlg, IDCMD_SKIP), FALSE);
        default:
            lvi.iImage = ILI_FAILED;
            break;
    }
    ListView_SetItem(m_hLV, &lvi);

    //add subitem for status text
    lvi.mask = LVIF_TEXT;
    ASSERT ((UINT)stat <= ITEM_STAT_SUCCEEDED);
    MLLoadString(IDS_ITEM_STAT_IDLE + stat, szBuf, ARRAYSIZE(szBuf));
    lvi.pszText = szBuf;
    lvi.iSubItem ++;
    ListView_SetItem(m_hLV, &lvi);

    //add subitem for URL
    PReportMap prm = m_pController->FindReportEntry (pCookie);
    lvi.pszText = prm->url;
    lvi.iSubItem++;
    ListView_SetItem(m_hLV, &lvi);

    //add subitem for size
    lvi.pszText = TEXT("");
    lvi.iSubItem++;
    ListView_SetItem(m_hLV, &lvi);

    return S_OK;
}
