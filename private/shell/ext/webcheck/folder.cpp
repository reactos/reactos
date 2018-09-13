#include "private.h"
#include "offl_cpp.h"
#include <htmlhelp.h>
#include <shdocvw.h>

#include <mluisupp.h>

// {F5175861-2688-11d0-9C5E-00AA00A45957}
const GUID CLSID_OfflineFolder = 
{ 0xf5175861, 0x2688, 0x11d0, { 0x9c, 0x5e, 0x0, 0xaa, 0x0, 0xa4, 0x59, 0x57 } };

// {F5175860-2688-11d0-9C5E-00AA00A45957}
const GUID IID_IOfflineObject = 
{ 0xf5175860, 0x2688, 0x11d0, { 0x9c, 0x5e, 0x0, 0xaa, 0x0, 0xa4, 0x59, 0x57 } };

// Column definition for the Cache Folder DefView
    
ColInfoType s_AllItems_cols[] = {
        {ICOLC_SHORTNAME,   IDS_NAME_COL,           20, LVCFMT_LEFT},
        {ICOLC_LAST,        IDS_LAST_COL,           14, LVCFMT_LEFT},
        {ICOLC_STATUS,      IDS_STATUS_COL,         14, LVCFMT_LEFT},
        {ICOLC_URL,         IDS_URL_COL,            20, LVCFMT_LEFT}, 
        {ICOLC_ACTUALSIZE,  IDS_SIZE_COL,           10, LVCFMT_LEFT}};

ColInfoType * colInfo = s_AllItems_cols;

LPMYPIDL _CreateFolderPidl(IMalloc *pmalloc, DWORD dwSize);

HRESULT OfflineFolderView_InitMenuPopup(HWND hwnd, UINT idCmdFirst, int nIndex, HMENU hMenu)
{
    UINT platform = WhichPlatform();

    if (platform != PLATFORM_INTEGRATED)    {
        MENUITEMINFO    mInfo = {0};
        mInfo.cbSize = sizeof(MENUITEMINFO);
        mInfo.fMask = MIIM_STATE;
        if (IsGlobalOffline())  {
            mInfo.fState = MFS_CHECKED;
        } else  {
            mInfo.fState = MFS_UNCHECKED;
        }
        SetMenuItemInfo(hMenu, RSVIDM_WORKOFFLINE + idCmdFirst, FALSE, &mInfo);
    }

    return NOERROR;
}

HRESULT OfflineFolderView_MergeMenu(LPQCMINFO pqcm)
{
    HMENU hmenu = NULL;
    UINT  platform = WhichPlatform();
    
    if (platform == PLATFORM_INTEGRATED) {
        hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(MENU_OFFLINE_TOP));
    } else  {
        hmenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(MENU_OFFLINE_BRONLY));
    }

    if (hmenu)
    {
        MergeMenuHierarchy(pqcm->hmenu, hmenu, pqcm->idCmdFirst, pqcm->idCmdLast, TRUE);
        DestroyMenu(hmenu);
    }

    return S_OK;
}

extern HRESULT CancelAllDownloads();

HRESULT OfflineFolderView_Command(HWND hwnd, UINT uID)
{
    switch (uID) {
    case RSVIDM_SORTBYNAME:
        ShellFolderView_ReArrange(hwnd, ICOLC_SHORTNAME);
        break;
    case RSVIDM_UPDATEALL:
        SendUpdateRequests(hwnd, NULL, 0);
        break;
    case RSVIDM_WORKOFFLINE:
        SetGlobalOffline(!IsGlobalOffline());
        break;
    case RSVIDM_HELP:
        SHHtmlHelpOnDemandWrap(hwnd, TEXT("iexplore.chm > iedefault"), 0, (DWORD_PTR) TEXT("subs_upd.htm"), ML_CROSSCODEPAGE);
        break;

    case RSVIDM_UPDATE:
        {
            LPMYPIDL *  pidlsSel = NULL;
            UINT    count = 0;

            count = (UINT) ShellFolderView_GetSelectedObjects
                                (hwnd, (LPITEMIDLIST*) &pidlsSel);

            if ((!pidlsSel) || !count)
                break;

            CLSID   * cookies = (CLSID *)MemAlloc(LPTR, count * sizeof(CLSID));
            UINT    validCount = 0;
            for (UINT i = 0; i < count; i ++)   {
                if (IS_VALID_MYPIDL(pidlsSel[i]))
                    cookies[validCount++] = pidlsSel[i]->ooe.m_Cookie;
            }
            if (validCount)
                SendUpdateRequests(hwnd, cookies, validCount);
            MemFree(cookies);
            cookies = NULL;
            break;
        }
    default:
        return E_FAIL;
    }
    return NOERROR;
}

//  We should make this a generic function for all types of items, even
//  for the third party items they should support these properties.

HRESULT Generic_GetDetails(PDETAILSINFO pdi, UINT iColumn)
{
    LPMYPIDL pooi = (LPMYPIDL)pdi->pidl;
    POOEntry pooe = NULL;
    TCHAR timeSTR[128];

    pdi->str.uType = STRRET_CSTR;
    pdi->str.cStr[0] = '\0';

    pooe = &(pooi->ooe);
    switch (iColumn)
    {
    case ICOLC_SHORTNAME:
        SHTCharToAnsi(NAME(pooe), pdi->str.cStr, sizeof(pdi->str.cStr));
        break;
    case ICOLC_URL:
        SHTCharToAnsi(URL(pooe), pdi->str.cStr, sizeof(pdi->str.cStr));
        break;
    case ICOLC_LAST:
        DATE2DateTimeString(pooe->m_LastUpdated, timeSTR);
        SHTCharToAnsi(timeSTR, pdi->str.cStr, sizeof(pdi->str.cStr));
        break;
    case ICOLC_STATUS:
        SHTCharToAnsi(STATUS(pooe), pdi->str.cStr, sizeof(pdi->str.cStr));
        break;
    case ICOLC_ACTUALSIZE:
        StrFormatByteSizeA(pooe->m_ActualSize * 1024, pdi->str.cStr, sizeof(pdi->str.cStr));
        break;
    }

    return S_OK;
}

HRESULT OfflineFolderView_OnGetDetailsOf(HWND hwnd, UINT iColumn, PDETAILSINFO pdi)
{
    LPMYPIDL pooi = (LPMYPIDL)pdi->pidl;

    if (iColumn > ICOLC_ACTUALSIZE)
        return E_NOTIMPL;

    if (!pooi)
    {
        pdi->str.uType = STRRET_CSTR;
        pdi->str.cStr[0] = '\0';
        MLLoadStringA(colInfo[iColumn].ids, pdi->str.cStr, sizeof(pdi->str.cStr));
        pdi->fmt = colInfo[iColumn].iFmt;
        pdi->cxChar = colInfo[iColumn].cchCol;
        return S_OK;
    }

    UINT    colId = colInfo[iColumn].iCol;
    return Generic_GetDetails(pdi, colId);
}

HRESULT OfflineFolderView_OnColumnClick(HWND hwnd, UINT iColumn)
{
    ShellFolderView_ReArrange(hwnd, colInfo[iColumn].iCol);
    return NOERROR;
}

const TBBUTTON c_tbOffline[] = {
    { 0, RSVIDM_UPDATE,       TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 1, RSVIDM_UPDATEALL,    TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 0,  0,                  TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, -1 },
    };

HRESULT OfflineFolderView_OnGetButtons(HWND hwnd, UINT idCmdFirst, LPTBBUTTON ptButton)
{
    UINT i;
    LONG_PTR iBtnOffset;
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwnd);
    TBADDBITMAP ab;

    // add the toolbar button bitmap, get it's offset
    ab.hInst =g_hInst;
    ab.nID   = IDB_TB_SMALL;        // std bitmaps
    psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 2, (LPARAM)&ab, &iBtnOffset);

    for (i = 0; i < ARRAYSIZE(c_tbOffline); i++)
    {
        ptButton[i] = c_tbOffline[i];

        if (!(c_tbOffline[i].fsStyle & TBSTYLE_SEP))
        {
            ptButton[i].idCommand += idCmdFirst;
            ptButton[i].iBitmap += (int)iBtnOffset;
        }
    }

    return S_OK;
}

HRESULT OfflineFolderView_OnGetButtonInfo(TBINFO * ptbInfo)
{
    ptbInfo->uFlags = TBIF_PREPEND;
    ptbInfo->cbuttons = ARRAYSIZE(c_tbOffline);
    return S_OK;
}

HRESULT OfflineFolderView_DidDragDrop(HWND hwnd,IDataObject *pdo,DWORD dwEffect)
{
    if ((dwEffect & (DROPEFFECT_MOVE |DROPEFFECT_COPY)) == DROPEFFECT_MOVE)
    {
        COfflineObjectItem *pOOItem;
        
        if (SUCCEEDED(pdo->QueryInterface(IID_IOfflineObject, (void **)&pOOItem)))
        {
            BOOL fDel = ConfirmDelete(hwnd, pOOItem->_cItems, pOOItem->_ppooi);
            if (!fDel)  {
                pOOItem->Release();
                return S_FALSE;
            }

            for (UINT i = 0; i < pOOItem->_cItems; i++)
            {
                if (SUCCEEDED(DoDeleteSubscription(&(pOOItem->_ppooi[i]->ooe)))) {
                    _GenerateEvent(SHCNE_DELETE, 
                            (LPITEMIDLIST)pOOItem->_ppooi[i], 
                            NULL);
                }
            }

            pOOItem->Release();
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT CALLBACK OfflineFolderView_ViewCallback(
     IShellView *psvOuter,
     IShellFolder *psf,
     HWND hwnd,
     UINT uMsg,
     WPARAM wParam,
     LPARAM lParam)
{
    HRESULT hres = NOERROR;

    switch (uMsg)
    {
    case DVM_GETHELPTEXT:
    case DVM_GETTOOLTIPTEXT:
    {
        UINT id = LOWORD(wParam);
        UINT cchBuf = HIWORD(wParam);
        if (g_fIsWinNT)   
        {
            WCHAR * pszBuf = (WCHAR *)lParam;
            MLLoadStringW(id + IDS_SB_FIRST, pszBuf, cchBuf);
        }
        else  
        {
            CHAR * pszBuf = (CHAR *)lParam;
            MLLoadStringA(id + IDS_SB_FIRST, pszBuf, cchBuf);
        }
    }   
        break;

    case DVM_DIDDRAGDROP:
        hres = OfflineFolderView_DidDragDrop(hwnd,(IDataObject *)lParam,(DWORD)wParam);
        break;
      
    case DVM_INITMENUPOPUP:
        hres = OfflineFolderView_InitMenuPopup(hwnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;

    case DVM_INVOKECOMMAND:
        OfflineFolderView_Command(hwnd, (UINT)wParam);
        break;

    case DVM_COLUMNCLICK:
        hres = OfflineFolderView_OnColumnClick(hwnd, (UINT)wParam);
        break;

    case DVM_GETDETAILSOF:
        hres = OfflineFolderView_OnGetDetailsOf(hwnd, (UINT)wParam, (PDETAILSINFO)lParam);
        break;

    case DVM_MERGEMENU:
        hres = OfflineFolderView_MergeMenu((LPQCMINFO)lParam);
        break;

    case DVM_DEFVIEWMODE:
        *(FOLDERVIEWMODE *)lParam = FVM_DETAILS;
        break;

    case DVM_GETBUTTONINFO:
        hres = OfflineFolderView_OnGetButtonInfo((TBINFO *)lParam);
        break;

    case DVM_GETBUTTONS:
        hres = OfflineFolderView_OnGetButtons(hwnd, LOWORD(wParam), (TBBUTTON *)lParam);
        break;

    default:
        hres = E_FAIL;
    }

    return hres;
}

HRESULT OfflineFolderView_CreateInstance(COfflineFolder *pOOFolder, LPCITEMIDLIST pidl, void **ppvOut)
{
    CSFV csfv;

    csfv.cbSize = sizeof(csfv);
    csfv.pshf = (IShellFolder *)pOOFolder;
    csfv.psvOuter = NULL;
    csfv.pidl = pidl;
    csfv.lEvents = SHCNE_DELETE | SHCNE_CREATE | SHCNE_RENAMEITEM | SHCNE_UPDATEITEM | SHCNE_UPDATEDIR;
    csfv.pfnCallback = OfflineFolderView_ViewCallback;
    csfv.fvm = (FOLDERVIEWMODE)0;         // Have defview restore the folder view mode

    return SHCreateShellFolderViewEx(&csfv, (IShellView**)ppvOut); // &this->psv);
}



//////////////////////////////////////////////////////////////////////////////
//
// COfflineFolderEnum Object
//
//////////////////////////////////////////////////////////////////////////////


COfflineFolderEnum::COfflineFolderEnum(DWORD grfFlags)
{
    TraceMsg(TF_SUBSFOLDER, "hcfe - COfflineFolderEnum() called");
    
    m_cRef = 1;
    DllAddRef();

    m_grfFlags = grfFlags;
}

IMalloc *COfflineFolderEnum::s_pMalloc = NULL;

void COfflineFolderEnum::EnsureMalloc()
{
    if (NULL == s_pMalloc)
    {
        SHGetMalloc(&s_pMalloc);
    }

    ASSERT(NULL != s_pMalloc);
}


COfflineFolderEnum::~COfflineFolderEnum()
{
    ASSERT(m_cRef == 0);         // we should always have a zero ref count here

    SAFERELEASE(m_pFolder);
    SAFEDELETE(m_pCookies);

    TraceMsg(TF_SUBSFOLDER, "hcfe - ~COfflineFolderEnum() called.");
    DllRelease();
}

HRESULT COfflineFolderEnum::Initialize(COfflineFolder *pFolder)
{
    HRESULT hr = S_OK;

    ASSERT(pFolder);
    
    if (NULL != pFolder)
    {
        m_pFolder = pFolder;
        m_pFolder->AddRef();

        hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            ISubscriptionMgr2 *pSubsMgr2;
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER, 
                                  IID_ISubscriptionMgr2, (void **)&pSubsMgr2);
            if (SUCCEEDED(hr))
            {
                IEnumSubscription *pes;

                hr = pSubsMgr2->EnumSubscriptions(0, &pes);
                if (SUCCEEDED(hr))
                {
                    pes->GetCount(&m_nCount);

                    if (m_nCount > 0)
                    {
                        m_pCookies = new SUBSCRIPTIONCOOKIE[m_nCount];

                        if (NULL != m_pCookies)
                        {
                            hr = pes->Next(m_nCount, m_pCookies, &m_nCount);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    pes->Release();
                }
                pSubsMgr2->Release();
            }

            CoUninitialize();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr;
}

HRESULT COfflineFolderEnum_CreateInstance(DWORD grfFlags, COfflineFolder *pFolder, 
                                          LPENUMIDLIST *ppeidl)
{
    HRESULT hr;

    *ppeidl = NULL;

    COfflineFolderEnum *pOOFE = new COfflineFolderEnum(grfFlags);
    
    if (NULL != pOOFE)
    {
        hr = pOOFE->Initialize(pFolder);

        if (SUCCEEDED(hr))
        {
            *ppeidl = pOOFE;
        }
        else
        {
            pOOFE->Release();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}


//////////////////////////////////
//
// IUnknown Methods...
//

HRESULT COfflineFolderEnum::QueryInterface(REFIID iid,void **ppv)
{
//    TraceMsg(TF_SUBSFOLDER, "COfflineFolderEnum - QI called.");
    
    if ((iid == IID_IEnumIDList) || (iid == IID_IUnknown))
    {
        *ppv = (IEnumIDList *)this;
        AddRef();
        return S_OK;
    }
    
    *ppv = NULL;
    return E_NOINTERFACE;
}

ULONG COfflineFolderEnum::AddRef(void)
{
    return ++m_cRef;
}

ULONG COfflineFolderEnum::Release(void)
{
    if (0L != --m_cRef)
        return m_cRef;

    delete this;
    return 0;
}

LPMYPIDL COfflineFolderEnum::NewPidl(DWORD dwSize)
{
    LPMYPIDL pidl;

//  TraceMsg(TF_MEMORY, "NewPidl called");

    EnsureMalloc();

    pidl = _CreateFolderPidl(s_pMalloc, dwSize);

//  TraceMsg(TF_MEMORY, "\tNewPidl returned with 0x%x", pidl);

    return pidl;
}

void COfflineFolderEnum::FreePidl(LPMYPIDL pidl)
{
    ASSERT(NULL != pidl);

//  TraceMsg(TF_MEMORY, "FreePidl on (0x%x) called", pidl);

    EnsureMalloc();

    s_pMalloc->Free(pidl);
}

// IEnumIDList Methods 

HRESULT COfflineFolderEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    ULONG nCopied;
    DWORD dwBuffSize;
    OOEBuf ooeBuf;

    if ((0 == celt) || 
        ((celt > 1) && (NULL == pceltFetched)) ||
        (NULL == rgelt))
    {
        return E_INVALIDARG;
    }

    memset(&ooeBuf, 0, sizeof(ooeBuf));

    for (nCopied = 0; (S_OK == hr) && (m_nCurrent < m_nCount) && (nCopied < celt); 
         m_nCurrent++, nCopied++)
    {
        rgelt[nCopied] = NULL;
        hr = LoadOOEntryInfo(&ooeBuf, &m_pCookies[m_nCurrent], &dwBuffSize);

        if (SUCCEEDED(hr))
        {
            if (IsNativeAgent(ooeBuf.clsidDest))
            {
                CLSID   cookie;
                HRESULT hrTmp = ReadCookieFromInetDB(ooeBuf.m_URL, &cookie);
                if (S_OK != hrTmp)
                {
                    hrTmp = WriteCookieToInetDB(ooeBuf.m_URL,&(ooeBuf.m_Cookie), FALSE);
                    ASSERT(SUCCEEDED(hrTmp));
                }
            }

            LPMYPIDL pooi = NewPidl(dwBuffSize);
            if (pooi)
            {
                CopyToMyPooe(&ooeBuf, &(pooi->ooe));  //  Always succeeds!
                rgelt[nCopied] = (LPITEMIDLIST)pooi;
            }
            else 
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = (celt == nCopied) ? S_OK : S_FALSE;
    }
    else
    {
        for (ULONG i = 0; i < nCopied; i++)
        {
            FreePidl((LPMYPIDL)rgelt[i]);
        }
    }

    if (NULL != pceltFetched)
    {
        *pceltFetched = SUCCEEDED(hr) ? nCopied : 0;
    }
    
    return hr;
}

HRESULT COfflineFolderEnum::Skip(ULONG celt)
{
    HRESULT hr;
    
    m_nCurrent += celt;

    if (m_nCurrent > (m_nCount - 1))
    {
        m_nCurrent = m_nCount;  //  Passed the last one
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }
    
    return hr;
}

HRESULT COfflineFolderEnum::Reset()
{
    m_nCurrent = 0;

    return S_OK;
}

HRESULT COfflineFolderEnum::Clone(IEnumIDList **ppenum)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////
//
// COfflineFolder Object
//
//////////////////////////////////////////////////////////////////////////////

COfflineFolder::COfflineFolder(void) 
{
    TraceMsg(TF_SUBSFOLDER, "Folder - COfflineFolder() called.");
    _cRef = 1;
    viewMode = 0;
    colInfo = s_AllItems_cols;
    DllAddRef();
}       

COfflineFolder::~COfflineFolder()
{
    Assert(_cRef == 0);                 // should always have zero
    TraceMsg(TF_SUBSFOLDER, "Folder - ~COfflineFolder() called.");
        
    DllRelease();
}    

//////////////////////////////////
//
// IUnknown Methods...
//
HRESULT COfflineFolder::QueryInterface(REFIID iid, void **ppvObj)
{
    *ppvObj = NULL;     // null the out param
    
    if (iid == IID_IUnknown) {
         *ppvObj = (void *)this;
    }
    else if (iid == IID_IShellFolder) {
         *ppvObj = (void *)(IShellFolder *)this;
    }
    else if ((iid == IID_IPersistFolder) || (iid == IID_IPersist) || (iid == IID_IPersistFolder2)) {
         *ppvObj = (void *)(IPersistFolder *)this;
    }
    else if (iid == IID_IContextMenu)
    {
         *ppvObj = (void *)(IContextMenu *)this;
    }
    else if (iid == IID_IShellView)
    {
         return OfflineFolderView_CreateInstance(this, _pidl, ppvObj);
    }
    else if (iid == IID_IOfflineObject)
    {
         *ppvObj = (void *)this;
    }
    else if (iid == IID_IDropTarget)
    {
        // BUGBUG: Implementation of IDropTarget didn't follow the COM rules.
        //  We create following object by aggregattion but QI on it for IUnknown
        //  won't get us ptr THIS.
        COfflineDropTarget * podt = new COfflineDropTarget(GetDesktopWindow());
        if (podt)
        {
            HRESULT hr = podt->QueryInterface(iid, ppvObj);
            podt->Release();
        }
        else
        {
            return E_OUTOFMEMORY;
        }     
    }

    if (*ppvObj) 
    {
        ((IUnknown *)*ppvObj)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG COfflineFolder::AddRef()
{
    return ++_cRef;
}

ULONG COfflineFolder::Release()
{
    if (0L != --_cRef)
        return _cRef;

    delete this;
    return 0;   
}

//////////////////////////////////
//
// IShellFolder methods...
//
HRESULT COfflineFolder::ParseDisplayName(HWND hwndOwner, LPBC pbcReserved,
                        LPOLESTR lpszDisplayName, ULONG *pchEaten,
                        LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - ParseDisplayName.");
    *ppidl = NULL;
    return E_FAIL;
}


HRESULT COfflineFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags,
                        LPENUMIDLIST *ppenumIDList)
{
//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - EnumObjects.");
    return COfflineFolderEnum_CreateInstance(grfFlags, this, ppenumIDList);
}


HRESULT COfflineFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                        REFIID riid, void **ppvOut)
{
//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - BindToObject.");
    *ppvOut = NULL;
    return E_FAIL;
}

HRESULT COfflineFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                        REFIID riid, void **ppvObj)
{
//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - BindToStorage.");
    *ppvObj = NULL;
    return E_NOTIMPL;
}

HRESULT COfflineFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet;

//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - CompareIDs(%d).", lParam);

    if (!IS_VALID_MYPIDL(pidl1) || !IS_VALID_MYPIDL(pidl2))
        return E_FAIL;

    switch (lParam) {
        case ICOLC_SHORTNAME:
            iRet = _CompareShortName((LPMYPIDL)pidl1, (LPMYPIDL)pidl2);
            break;
        case ICOLC_URL:
            iRet = _CompareURL((LPMYPIDL)pidl1, (LPMYPIDL)pidl2);
            break;
        case ICOLC_STATUS:
            iRet = _CompareStatus((LPMYPIDL)pidl1, (LPMYPIDL)pidl2);
            break;
        case ICOLC_LAST:
            iRet = _CompareLastUpdate((LPMYPIDL)pidl1, (LPMYPIDL)pidl2);
            break;
        case ICOLC_ACTUALSIZE:
            iRet = (((LPMYPIDL)pidl1)->ooe.m_ActualSize - ((LPMYPIDL)pidl2)->ooe.m_ActualSize);
            break;
        default:
            iRet = -1;
            break;
    }
    return ResultFromShort((SHORT)iRet);
}


HRESULT COfflineFolder::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppvOut)
{
    HRESULT hres;

//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - CreateViewObject() called.");

    if (riid == IID_IShellView)
    {
        hres = OfflineFolderView_CreateInstance(this, _pidl, ppvOut);
    }
    else if (riid == IID_IContextMenu)
    {
        COfflineFolder * pof = new COfflineFolder();

        if (pof)
        {
            hres = pof->Initialize(this->_pidl);
            if (SUCCEEDED(hres))
                hres = pof->QueryInterface(riid, ppvOut);
            pof->Release();
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else if (riid == IID_IDropTarget)
    {
        COfflineDropTarget * podt = new COfflineDropTarget(hwndOwner);

        if (podt)
        {
            hres = podt->QueryInterface(riid, ppvOut);
            podt->Release();
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }
    else if (riid == IID_IShellDetails)
    {
        COfflineDetails *pod = new COfflineDetails(hwndOwner);
        if (NULL != pod)
        {
            hres = pod->QueryInterface(IID_IShellDetails, ppvOut);
            pod->Release();
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        DBGIID("COfflineFolder::CreateViewObject() failed", riid);
        *ppvOut = NULL;         // null the out param
        hres = E_NOINTERFACE;
    }
    
    return hres;    
}

HRESULT COfflineFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                        ULONG * prgfInOut)
{
    // BUGBUG: Should we initialize this for each item in here?  In other words,
    // if cidl > 1, then we should initialize each entry in the prgInOut array
    
    UINT    attr = SFGAO_CANCOPY | SFGAO_CANDELETE | SFGAO_CANRENAME |
                    SFGAO_HASPROPSHEET;
    *prgfInOut = attr;
    
    return NOERROR;
}

HRESULT COfflineFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                        REFIID riid, UINT * prgfInOut, void **ppvOut)
{
    HRESULT hres;

//    TraceMsg(TF_SUBSFOLDER, "Folder:ISF - GetUIObjectOf.");
    if ((riid == IID_IContextMenu) || (riid == IID_IDataObject) || 
        (riid == IID_IExtractIcon) || (riid == IID_IQueryInfo))
    {
        hres = COfflineObjectItem_CreateInstance(this, cidl, apidl, riid, ppvOut);
    }
    else if (riid == IID_IDropTarget)
    {
        hres = CreateViewObject(hwndOwner, IID_IDropTarget, ppvOut);
    }
    else 
    {
        *ppvOut = NULL;
        hres = E_FAIL;
        DBGIID("Unsupported interface in COfflineFolder::GetUIObjectOf()", riid);
    }
    return hres;    
}

HRESULT COfflineFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName)
{
//    TraceMsg(TF_SUBSFOLDER, "Folde:ISF - GetDisplayNameOf.");
    
    lpName->uType = STRRET_CSTR;

    if (!IS_VALID_MYPIDL(pidl))
    {
        lpName->uType = 0;
        return E_FAIL;
    }

    SHTCharToAnsi(NAME(&(((LPMYPIDL)pidl)->ooe)), lpName->cStr, sizeof(lpName->cStr));
    return NOERROR;    
}

HRESULT COfflineFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                        LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut)
{
    OOEBuf  ooeBuf;
    POOEntry    pooe = NULL;
//    TraceMsg(TF_SUBSFOLDER, "Folde:ISF - SetNameOf.");
    
    if (ppidlOut)  {
        *ppidlOut = NULL;               // null the out param
    }

    if (!IS_VALID_MYPIDL(pidl))
        return E_FAIL;

    HRESULT hr;
    WCHAR szTempName[MAX_PATH];

    ASSERT(lpszName);

    StrCpyNW(szTempName, lpszName, ARRAYSIZE(szTempName));

    PathRemoveBlanks(szTempName);

    if (szTempName[0])
    {   
        memset(&ooeBuf, 0, sizeof(ooeBuf));

        pooe = &(((LPMYPIDL)pidl)->ooe);
        CopyToOOEBuf(pooe, &ooeBuf);
        MyOleStrToStrN(ooeBuf.m_Name, MAX_NAME, szTempName);

        ooeBuf.dwFlags = PROP_WEBCRAWL_NAME;
        hr = SaveBufferChange(&ooeBuf, FALSE);
        
        if (ppidlOut)   {
            DWORD   dwSize = BufferSize(&ooeBuf);
            *ppidlOut = (LPITEMIDLIST)COfflineFolderEnum::NewPidl(dwSize);
            if (*ppidlOut)  {
                pooe = &(((LPMYPIDL)(*ppidlOut))->ooe);
                CopyToMyPooe(&ooeBuf, pooe);
            }
        }
    }
    else
    {
        WCMessageBox(hwndOwner, IDS_NONULLNAME, IDS_RENAME, MB_OK | MB_ICONSTOP);
        hr = E_FAIL;
    }
    return hr;    
}

//////////////////////////////////
//
// IPersistFolder Methods...
//
HRESULT COfflineFolder::GetClassID(LPCLSID lpClassID)
{
//    TraceMsg(TF_SUBSFOLDER, "hcf - pf - GetClassID.");
    
    *lpClassID = CLSID_OfflineFolder;
    return S_OK;
}


HRESULT COfflineFolder::Initialize(LPCITEMIDLIST pidlInit)
{
    if (_pidl)
        ILFree(_pidl);

    _pidl = ILClone(pidlInit);

    if (!_pidl)
        return E_OUTOFMEMORY;

    return NOERROR;
}

HRESULT COfflineFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (_pidl)
    {
        *ppidl = ILClone(_pidl);
        return *ppidl ? NOERROR : E_OUTOFMEMORY;
    }

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}

//////////////////////////////////
//
// IContextMenu Methods...
//
HRESULT COfflineFolder::QueryContextMenu
(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst,
    UINT idCmdLast, 
    UINT uFlags)
{
    USHORT cItems = 0;

//    TraceMsg(TF_SUBSFOLDER, "Folder:IContextMenu- QueryContextMenu.");
    if (uFlags == CMF_NORMAL)
    {
        HMENU hmenuHist = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(CONTEXT_MENU_OFFLINE));
        if (hmenuHist)
        {
            cItems = (USHORT) MergeMenuHierarchy(hmenu, hmenuHist, idCmdFirst, idCmdLast, TRUE);

            DestroyMenu(hmenuHist);
        }
    }
    
    return ResultFromShort(cItems);    // number of menu items    
}

STDMETHODIMP COfflineFolder::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
//    TraceMsg(TF_SUBSFOLDER, "Folder:IContextMenu - InvokeCommand.");
    
    int idCmd = _GetCmdID(pici->lpVerb);
    
    if (idCmd != RSVIDM_PASTE)
        return OfflineFolderView_Command(pici->hwnd, idCmd);
     
    IDataObject * dataSrc = NULL;
    IDropTarget * pDropTrgt = NULL;
    HRESULT hr;

    hr = OleGetClipboard(&(dataSrc));

    if (SUCCEEDED(hr))
        hr = this->QueryInterface(IID_IDropTarget, (void **) &pDropTrgt);

    if (SUCCEEDED(hr))  {
        DWORD dwPrefEffect = DROPEFFECT_COPY;
        POINTL pt = {0, 0};

        hr = pDropTrgt->DragEnter(dataSrc, 0/*keystate*/, pt, &dwPrefEffect);
        if (SUCCEEDED(hr))  {
            hr = pDropTrgt->Drop(dataSrc, 0, pt, &dwPrefEffect);
        }
    }

    if (dataSrc)
        SAFERELEASE(dataSrc);
    if (pDropTrgt)
        SAFERELEASE(pDropTrgt);

    return hr;
}

STDMETHODIMP COfflineFolder::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved,
                                LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_FAIL;

//    TraceMsg(TF_SUBSFOLDER, "Folder:IContextMenu - GetCommandString.");
    if (uFlags == GCS_HELPTEXTA)
    {
        MLLoadStringA((UINT)idCmd + IDS_SB_FIRST, pszName, cchMax);
        hres = NOERROR;
    }
    return hres;
}

COfflineDetails::COfflineDetails(HWND hwndOwner)
{
    ASSERT(NULL != hwndOwner);
    m_hwndOwner = hwndOwner;
    m_cRef = 1;
}

STDMETHODIMP COfflineDetails::QueryInterface(REFIID riid, void **ppv)
{
    if ((IID_IUnknown == riid) || (IID_IShellDetails == riid))
    {
        *ppv = (IShellDetails *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) COfflineDetails::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) COfflineDetails::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP COfflineDetails::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails)
{
    HRESULT hr;

    if (iColumn > ICOLC_ACTUALSIZE)
        return E_NOTIMPL;

    if (NULL == pDetails)
    {
        return E_INVALIDARG;
    }
    
    if (NULL != pidl)
    {
        DETAILSINFO di = { pidl };
        hr = Generic_GetDetails(&di, colInfo[iColumn].iCol);
        pDetails->fmt = di.fmt;
        pDetails->cxChar = di.cxChar;
        memcpy(&pDetails->str, &di.str, sizeof(di.str));
    }
    else
    {
        pDetails->str.uType = STRRET_CSTR;
        pDetails->str.cStr[0] = '\0';
        MLLoadStringA(colInfo[iColumn].ids, pDetails->str.cStr, sizeof(pDetails->str.cStr));
        pDetails->fmt = colInfo[iColumn].iFmt;
        pDetails->cxChar = colInfo[iColumn].cchCol;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP COfflineDetails::ColumnClick(UINT iColumn)
{
    ShellFolderView_ReArrange(m_hwndOwner, colInfo[iColumn].iCol);
    return S_OK;
}


LPMYPIDL _CreateFolderPidl(IMalloc *pmalloc, DWORD dwSize)
{
    LPMYPIDL pooi = (LPMYPIDL)pmalloc->Alloc(sizeof(MYPIDL) + dwSize + sizeof(USHORT));
    if (pooi)
    {
        memset(pooi, 0, sizeof(MYPIDL) + dwSize + sizeof(USHORT));
        pooi->cb = (USHORT)(dwSize + sizeof(MYPIDL));
        pooi->usSign = (USHORT)MYPIDL_MAGIC;
//      TraceMsg(TF_MEMORY, "CreatePidl %d", sizeof(MYPIDL) + dwSize);
    }
    return pooi;
}
