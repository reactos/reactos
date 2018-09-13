#include "private.h"
#include "offl_cpp.h"

#include <mluisupp.h>

// registered clipboard formats
UINT g_cfFileDescriptor = 0;
UINT g_cfFileContents = 0;
UINT g_cfPrefDropEffect = 0;
UINT g_cfURL = 0;

HICON g_webCrawlerIcon = NULL;
HICON g_channelIcon = NULL;
HICON g_desktopIcon = NULL;

#define MAX_ITEM_OPEN 10

//////////////////////////////////////////////////////////////////////////////
// COfflineObjectItem Object
//////////////////////////////////////////////////////////////////////////////

void LoadDefaultIcons()
{
    if (g_webCrawlerIcon == NULL) 
    {
        g_webCrawlerIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SUBSCRIBE));
        g_channelIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CHANNEL));
        g_desktopIcon = LoadIcon(MLGetHinst(), MAKEINTRESOURCE(IDI_DESKTOPITEM));
    }
}

COfflineObjectItem::COfflineObjectItem() 
{
    TraceMsg(TF_SUBSFOLDER, "hci - COfflineObjectItem() called.");
    DllAddRef();
    _cRef = 1;
}        

COfflineObjectItem::~COfflineObjectItem()
{
    Assert(_cRef == 0);                 // we should have zero ref count here

    TraceMsg(TF_SUBSFOLDER, "hci - ~COfflineObjectItem() called.");
    
    SAFERELEASE(m_pUIHelper);
    SAFERELEASE(_pOOFolder);

    if (_ppooi)
    {
        for (UINT i = 0; i < _cItems; i++) 
        {
            if (_ppooi[i])
                ILFree((LPITEMIDLIST)_ppooi[i]);
        }
        MemFree((HLOCAL)_ppooi);
    }
    
    DllRelease();
}

HRESULT COfflineObjectItem::Initialize(COfflineFolder *pOOFolder, UINT cidl, LPCITEMIDLIST *ppidl)
{
    _ppooi = (LPMYPIDL *)MemAlloc(LPTR, cidl * sizeof(LPMYPIDL));
    if (!_ppooi)
        return E_OUTOFMEMORY;
    
    _cItems     = cidl;

    for (UINT i = 0; i < cidl; i++)
    {
        // we need to clone the whole array, so if one of them fails, we'll
        // destroy the ones we've already created
        _ppooi[i] = (LPMYPIDL)ILClone(ppidl[i]);
        if (!_ppooi[i]) {
            UINT j = 0;

            for (; j < i; j++)  {
                ILFree((LPITEMIDLIST)_ppooi[j]);
                _ppooi[j] = NULL;
            }

            MemFree((HLOCAL)_ppooi);
            return E_OUTOFMEMORY;
        }
    }   
    
    _pOOFolder = pOOFolder;
    _pOOFolder->AddRef();      // we're going to hold onto this pointer, so
                               // we need to AddRef it.

    //  If there is only one item here, we initialize UI helper.
    if (_cItems == 1)
    {
        ASSERT(!m_pUIHelper);
        POOEntry pooe = &(_ppooi[0]->ooe);

        HRESULT hr = CoInitialize(NULL);

        ASSERT(SUCCEEDED(hr));

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(*(&(pooe->clsidDest)), NULL, CLSCTX_INPROC_SERVER, 
                                  IID_IUnknown, (void **)&m_pUIHelper);

            ASSERT(SUCCEEDED(hr));
            ASSERT(m_pUIHelper);

            if (SUCCEEDED(hr))
            {
                ISubscriptionAgentShellExt *psase;
                
                hr = m_pUIHelper->QueryInterface(IID_ISubscriptionAgentShellExt, (void **)&psase);
                if (SUCCEEDED(hr))
                {
                    WCHAR wszURL[MAX_URL + 1];
                    WCHAR wszName[MAX_NAME + 1];

                    MyStrToOleStrN(wszURL, ARRAYSIZE(wszURL), URL(pooe));
                    MyStrToOleStrN(wszName, ARRAYSIZE(wszName), NAME(pooe));

                    psase->Initialize(&pooe->m_Cookie, wszURL, wszName, (SUBSCRIPTIONTYPE)-1);
                    psase->Release();
                }
            }
            CoUninitialize();
        }
    }
    return S_OK;
}        

HRESULT COfflineObjectItem_CreateInstance
(
    COfflineFolder *pOOFolder,
    UINT cidl, 
    LPCITEMIDLIST *ppidl, 
    REFIID riid, 
    void **ppvOut
)
{
    COfflineObjectItem *pOOItem;
    HRESULT hr;

    *ppvOut = NULL;                 // null the out param

    if (!_ValidateIDListArray(cidl, ppidl))
        return E_FAIL;

#ifdef UNICODE
    if (((riid == IID_IExtractIconA) || (riid == IID_IExtractIconW)) && (cidl != 1))
#else
    if ((riid == IID_IExtractIcon) && (cidl != 1))
#endif
        return E_FAIL;      //  What do you need this icon for?

    pOOItem = new COfflineObjectItem;
    if (!pOOItem)
        return E_OUTOFMEMORY;

    hr = pOOItem->Initialize(pOOFolder, cidl, ppidl);
    if (SUCCEEDED(hr))
    {
        hr = pOOItem->QueryInterface(riid, ppvOut);
    }
    pOOItem->Release();

    if (g_cfPrefDropEffect == 0)
    {
        g_cfFileDescriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR); // "FileContents"
        g_cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);     // "FileDescriptor"
        g_cfPrefDropEffect = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
        g_cfURL = RegisterClipboardFormat(CFSTR_SHELLURL);
    }
    
    return hr;
}

// IUnknown Methods...

HRESULT COfflineObjectItem::QueryInterface(REFIID iid, LPVOID *ppvObj)
{
//    TraceMsg(TF_ALWAYS, TEXT("hci - QueryInterface() called."));
    
    *ppvObj = NULL;     // null the out param
    
    if (iid == IID_IUnknown) {
        TraceMsg(TF_SUBSFOLDER, "  getting IUnknown");
        *ppvObj = (LPVOID)this;
    }
    else if (iid == IID_IContextMenu) {
        TraceMsg(TF_SUBSFOLDER, "   getting IContextMenu");
        *ppvObj = (LPVOID)(IContextMenu *)this;
    }
    else if (iid == IID_IQueryInfo)  {
        TraceMsg(TF_SUBSFOLDER, "  getting IQueryInfo");
        *ppvObj = (LPVOID)(IQueryInfo *)this;
    }
    else if (iid == IID_IDataObject) {
        TraceMsg(TF_SUBSFOLDER, "  getting IDataObject");
        *ppvObj = (LPVOID)(IDataObject *)this;
    }
#ifdef UNICODE
    else if ((iid == IID_IExtractIconA) || (iid == IID_IExtractIconW)) {
#else
    else if (iid == IID_IExtractIcon) {
#endif
        if (m_pUIHelper)    {
            TraceMsg(TF_SUBSFOLDER, "  getting IExtractIcon from UIHelper");
            if (S_OK == m_pUIHelper->QueryInterface(iid, ppvObj))
                return S_OK;
            else
                TraceMsg(TF_SUBSFOLDER, "  failed to get IExtractIcon from UIHelper");
        } 
        TraceMsg(TF_SUBSFOLDER, "  getting default IExtractIcon");
#ifdef UNICODE
        *ppvObj = iid == IID_IExtractIconA ? 
            (LPVOID)(IExtractIconA *)this :
            (LPVOID)(IExtractIconW *)this;
#else
        *ppvObj = (LPVOID)(IExtractIcon*)this;
#endif
    }
    else if (iid == IID_IOfflineObject) {
        TraceMsg(TF_SUBSFOLDER, "  getting IOfflineObject");
        *ppvObj = (LPVOID)this;
    }        
    
    if (*ppvObj) 
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return S_OK;
    }

    DBGIID("COfflineObjectItem::QueryInterface() failed", iid);
    return E_NOINTERFACE;
}

ULONG COfflineObjectItem::AddRef()
{
    return ++_cRef;
}

ULONG COfflineObjectItem::Release()
{
    if (0L != --_cRef)
        return _cRef;

    delete this;
    return 0;   
}


// IContextMenu Methods

HRESULT COfflineObjectItem::QueryContextMenu
(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst,
    UINT idCmdLast, 
    UINT uFlags
)
{
    UINT cItems;

    TraceMsg(TF_SUBSFOLDER, "Item::QueryContextMenu() called.");
    
    ///////////////////////////////////////////////////////////
    //  BUGBUG: May also need some category specific code here.

#ifdef DEBUG
    int imi = GetMenuItemCount(hmenu);
    while (--imi >= 0)
    {
        MENUITEMINFO mii = {
            sizeof(MENUITEMINFO), MIIM_ID | MIIM_SUBMENU | MIIM_TYPE, 
            0, 0, 0, 0, 0, 0, 0, 0, 0};
        if (GetMenuItemInfo(hmenu, imi, TRUE, &mii)) {
            ;
        }
    }
#endif
    if ((uFlags & CMF_VERBSONLY) || (uFlags & CMF_DVFILE))
        cItems = MergePopupMenu(&hmenu, POPUP_CONTEXT_VERBSONLY, 0, indexMenu, 
                                idCmdFirst - RSVIDM_FIRST, idCmdLast);
    else
    {
        if (_ppooi[0]->ooe.bChannel &&
            SHRestricted2(REST_NoEditingChannels, URL(&(_ppooi[0]->ooe)), 0))
        {
            cItems = MergePopupMenu(&hmenu, POPUP_RESTRICTED_CONTEXT, 0, indexMenu,
                                    idCmdFirst - RSVIDM_FIRST, idCmdLast);
        }
        else
        {
            cItems = MergePopupMenu(&hmenu, POPUP_OFFLINE_CONTEXT, 0, indexMenu,
                                    idCmdFirst - RSVIDM_FIRST, idCmdLast);
        }
        if (_cItems > 1)
            EnableMenuItem(hmenu, RSVIDM_PROPERTIES + idCmdFirst - RSVIDM_FIRST, MF_BYCOMMAND | MF_GRAYED);
    }

    if (SHRestricted2(REST_NoManualUpdates, URL(&(_ppooi[0]->ooe)), 0))
        EnableMenuItem(hmenu, RSVIDM_UPDATE + idCmdFirst - RSVIDM_FIRST, MF_BYCOMMAND | MF_GRAYED); 
    
    SetMenuDefaultItem(hmenu, indexMenu, MF_BYPOSITION);

    return ResultFromShort(cItems);    // number of menu items    
}


STDMETHODIMP COfflineObjectItem::InvokeCommand
(
    LPCMINVOKECOMMANDINFO pici
)
{
    UINT i;
    int idCmd = _GetCmdID(pici->lpVerb);
    HRESULT hres = S_OK;
    CLSID   * pClsid = NULL;
    int     updateCount = 0;

    TraceMsg(TF_SUBSFOLDER, "hci - cm - InvokeCommand() called.");

    if (idCmd == RSVIDM_DELETE)
    {
        BOOL fRet = ConfirmDelete(pici->hwnd, _cItems, _ppooi);
        if (!fRet)
            return S_FALSE;
    } else if (idCmd == RSVIDM_UPDATE)  {
        pClsid = (CLSID *)MemAlloc(LPTR, sizeof(CLSID) * _cItems);
        if (!pClsid)
            return E_OUTOFMEMORY;
    }
        
    for (i = 0; i < _cItems; i++)
    {
        if (_ppooi[i]) 
        {
            SUBSCRIPTIONTYPE    subType; 
            switch (idCmd)
            {
            case RSVIDM_OPEN:
                if (i >= MAX_ITEM_OPEN)
                {
                    hres = S_FALSE;
                    goto Done;
                }

                subType = GetItemCategory(&(_ppooi[i]->ooe)); 
                switch (subType)   {
                case SUBSTYPE_URL:
                case SUBSTYPE_CHANNEL:
                case SUBSTYPE_DESKTOPURL:
                case SUBSTYPE_DESKTOPCHANNEL:
                    hres = _LaunchApp(pici->hwnd,URL(&(_ppooi[i]->ooe))); 
                    break;
                default:
                    break;
                }
                break;

            case RSVIDM_COPY:
                OleSetClipboard((IDataObject *)this);
                goto Done;

            case RSVIDM_DELETE:
                hres = DoDeleteSubscription(&(_ppooi[i]->ooe));
                if (SUCCEEDED(hres))
                    _GenerateEvent(SHCNE_DELETE,(LPITEMIDLIST)(_ppooi[i]),NULL);
                break;

            case RSVIDM_PROPERTIES: 
                { 
                    POOEntry pooe = &(_ppooi[i]->ooe);
                    OOEBuf ooeBuf;
                    int iRet;

                    pooe->dwFlags = 0;
                    CopyToOOEBuf(pooe, &ooeBuf);

                    subType = GetItemCategory(&(_ppooi[i]->ooe)); 
                    switch (subType)   {
                    case SUBSTYPE_URL:
                    case SUBSTYPE_CHANNEL:
                    case SUBSTYPE_DESKTOPCHANNEL:
                    case SUBSTYPE_DESKTOPURL:
                    case SUBSTYPE_EXTERNAL:
                        iRet = _CreatePropSheet(pici->hwnd,&ooeBuf);
                        break;
                    default:
                        goto Done;
                    }

                    if (iRet <= 0) 
                        goto Done;

                    LPMYPIDL newPidl = NULL;
                    hres = LoadSubscription(ooeBuf.m_URL, &newPidl);

                    if (FAILED(hres))   {
                        ASSERT(0);
                    } else  {
                        ILFree((LPITEMIDLIST)_ppooi[i]);
                        _ppooi[i] = newPidl;
                    }
                }
                goto Done;

            case RSVIDM_UPDATE: 
                {
                    POOEntry pooe = &(_ppooi[i]->ooe);
                    pClsid[updateCount] = pooe->m_Cookie;
                    updateCount ++;
                } 
                break;

            default:
                hres = E_FAIL;
                break;
            }
        }
    }

    if (idCmd == RSVIDM_UPDATE) {
        hres = SendUpdateRequests(pici->hwnd, pClsid, updateCount);
        MemFree(pClsid);
    }
Done:
    return hres;
}


STDMETHODIMP COfflineObjectItem::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT *pwReserved,
                                LPSTR pszName, UINT cchMax)
{
    HRESULT hres = E_FAIL;

//    TraceMsg(TF_ALWAYS, TEXT("OOI/IContextMenu - GetCommandString() called."));

    if (uFlags == GCS_VERBA)
    {
        LPCSTR pszSrc = NULL;

        switch(idCmd)
        {
            case RSVIDM_OPEN:
                pszSrc = c_szOpen;
                break;

            case RSVIDM_COPY:
                pszSrc = c_szCopy;
                break;

            case RSVIDM_DELETE:
                pszSrc = c_szDelete;
                break;

            case RSVIDM_PROPERTIES:
                pszSrc = c_szProperties;
                break;
        }
        
        if (pszSrc)
        {
            lstrcpynA(pszName, pszSrc, cchMax);
            hres = NOERROR;
        }
    }
    
    else if (uFlags == GCS_HELPTEXTA)
    {
        switch(idCmd)
        {
            case RSVIDM_OPEN:
            case RSVIDM_RENAME:
            case RSVIDM_UPDATE:
            case RSVIDM_COPY:
            case RSVIDM_DELETE:
            case RSVIDM_PROPERTIES:
                MLLoadStringA((UINT)(IDS_SB_FIRST+idCmd), pszName, cchMax);
                hres = NOERROR;
                break;

            default:
                break;
        }
    }
    return hres;
}

// IQueryInfo Method
HRESULT COfflineObjectItem::GetInfoTip(DWORD dwFlags, WCHAR ** ppwsz)
{
    *ppwsz = NULL;
    int clen = lstrlen(STATUS(&(_ppooi[0]->ooe)))+1;
    *ppwsz = (LPOLESTR)SHAlloc(clen*sizeof(WCHAR)) ;
    if (!(*ppwsz))  {
        return E_OUTOFMEMORY;
    }
    MyStrToOleStrN(*ppwsz, clen, STATUS(&(_ppooi[0]->ooe)));
    return S_OK;
}

HRESULT COfflineObjectItem::GetInfoFlags(DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

// IDataObject Methods...

HRESULT COfflineObjectItem::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    HRESULT hres;

#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wsprintf(szName, TEXT("#%d"), pFEIn->cfFormat);

    TraceMsg(TF_SUBSFOLDER, "COfflineObjectItem - GetData(%s)", szName);
#endif

    pSTM->hGlobal = NULL;
    pSTM->pUnkForRelease = NULL;

    if ((pFEIn->cfFormat == g_cfPrefDropEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreatePrefDropEffect(pSTM);

    else if ((pFEIn->cfFormat == g_cfFileDescriptor) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateFileDescriptor(pSTM);

    else if ((pFEIn->cfFormat == g_cfFileContents) && (pFEIn->tymed & TYMED_ISTREAM))
        hres = _CreateFileContents(pSTM, pFEIn->lindex);

    else if ((pFEIn->cfFormat == g_cfURL || pFEIn->cfFormat == CF_TEXT) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = _CreateURL(pSTM);
  
    else
        hres = DATA_E_FORMATETC;
    
    return hres;

}

HRESULT COfflineObjectItem::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
//    TraceMsg(TF_ALWAYS, TEXT("COfflineObjectItem - GetDataHere() called."));
    return E_NOTIMPL;
}

HRESULT COfflineObjectItem::QueryGetData(LPFORMATETC pFEIn)
{
#ifdef DEBUG
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wsprintf(szName, TEXT("#%d"), pFEIn->cfFormat);

#endif

    if (pFEIn->cfFormat == g_cfPrefDropEffect   ||
        pFEIn->cfFormat == g_cfFileDescriptor   ||
        pFEIn->cfFormat == g_cfFileContents     ||
        pFEIn->cfFormat == g_cfURL              ||
        pFEIn->cfFormat == CF_TEXT
       )  {
#ifdef DEBUG
        TraceMsg(TF_ALWAYS, "\t%s format supported.", szName);
#endif
        return NOERROR;
    }
    
    return S_FALSE;
}

HRESULT COfflineObjectItem::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
//    TraceMsg(TF_ALWAYS, TEXT("COfflineObjectItem - GetCanonicalFormatEtc() called."));
    return DATA_S_SAMEFORMATETC;
}

HRESULT COfflineObjectItem::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    TraceMsg(TF_SUBSFOLDER, "COfflineObjectItem - SetData() called.");
    return E_NOTIMPL;
}

HRESULT COfflineObjectItem::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum)
{
    FORMATETC objectfmte[5] = {
        {(CLIPFORMAT) g_cfFileDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(CLIPFORMAT) g_cfFileContents,   NULL, DVASPECT_CONTENT, -1, TYMED_ISTREAM },
        {(CLIPFORMAT) g_cfPrefDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(CLIPFORMAT) g_cfURL,            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(CLIPFORMAT) CF_TEXT,            NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }
    };
    HRESULT hres;

    TraceMsg(TF_SUBSFOLDER, "COfflineObjectItem - EnumFormatEtc() called.");

    hres = SHCreateStdEnumFmtEtc(ARRAYSIZE(objectfmte), objectfmte, ppEnum);
    TraceMsg(TF_SUBSFOLDER, "\t- EnumFormatEtc() return %d.", hres);
    return  hres;
}

HRESULT COfflineObjectItem::DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
    LPDWORD pdwConnection)
{
    TraceMsg(TF_SUBSFOLDER, "COfflineObjectItem - DAdvise() called.");
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT COfflineObjectItem::DUnadvise(DWORD dwConnection)
{
//    TraceMsg(TF_SUBSFOLDER, TEXT("COfflineObjectItem - DUnAdvise() called."));
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT COfflineObjectItem::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
//    TraceMsg(TF_ALWAYS, TEXT("COfflineObjectItem - EnumAdvise() called."));
    return OLE_E_ADVISENOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////////
//
// Helper Routines
//
//////////////////////////////////////////////////////////////////////////////

    
HRESULT COfflineObjectItem::_CreatePrefDropEffect(LPSTGMEDIUM pSTM)
{    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;

    TraceMsg(TF_SUBSFOLDER, "OOI/CreatePrefDropEffect");
    pSTM->hGlobal = MemAlloc(LPTR, sizeof(DWORD));

    //  BUGBUG: Need category specific code.

    DWORD prefEffect = DROPEFFECT_COPY;

    if (pSTM->hGlobal)
    {
        *((LPDWORD)pSTM->hGlobal) = prefEffect;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}

HRESULT COfflineObjectItem::_CreateURL(LPSTGMEDIUM pSTM)
{
    LPTSTR pszURL = URL(&(((LPMYPIDL)_ppooi[0])->ooe));    
    int cchAlloc = (lstrlen(pszURL) + 1) * 2;
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    pSTM->hGlobal = MemAlloc(LPTR, cchAlloc);

    if (pSTM->hGlobal)
    {
        SHTCharToAnsi(pszURL, (LPSTR)pSTM->hGlobal, cchAlloc);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

HRESULT COfflineObjectItem::_CreateFileContents(LPSTGMEDIUM pSTM, LONG lindex)
{
    HRESULT hr;
    LONG iIndex;
    
    // make sure the index is in a valid range.
    if (lindex == -1)
    {
        if (_cItems == 1)
            lindex = 0;
        else
            return E_FAIL;
    }

    Assert((unsigned)lindex < _cItems);
    Assert(lindex >= 0);

    iIndex = lindex;
    
    pSTM->tymed = TYMED_ISTREAM;
    pSTM->pUnkForRelease = NULL;
    
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pSTM->pstm);
    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER li = {0L, 0L};
        IUniformResourceLocator *purl;

        hr = SHCoCreateInstance(NULL, &CLSID_InternetShortcut, NULL,
            IID_IUniformResourceLocator, (void **)&purl);
        if (SUCCEEDED(hr))
        {
            hr = purl->SetURL(URL(&(_ppooi[iIndex]->ooe)), TRUE);
            if (SUCCEEDED(hr))
            {
                IPersistStream *pps;
                hr = purl->QueryInterface(IID_IPersistStream, (LPVOID *)&pps);
                if (SUCCEEDED(hr))
                {
                    hr = pps->Save(pSTM->pstm, TRUE);
                    pps->Release();
                }
            }
            purl->Release();
        }               
        pSTM->pstm->Seek(li, STREAM_SEEK_SET, NULL);
    }

    return hr;
}

HRESULT COfflineObjectItem::_CreateFileDescriptor(LPSTGMEDIUM pSTM)
{
    FILEGROUPDESCRIPTOR *pfgd;
    
    // render the file descriptor
    // we only allocate for _cItems-1 file descriptors because the filegroup
    // descriptor has already allocated space for 1.
    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    pfgd = (FILEGROUPDESCRIPTOR *)MemAlloc(LPTR, sizeof(FILEGROUPDESCRIPTOR) + (_cItems-1) * sizeof(FILEDESCRIPTOR));
    if (pfgd == NULL)
    {
        TraceMsg(TF_ALWAYS, "ooi -   Couldn't alloc file descriptor");
        return E_OUTOFMEMORY;
    }
    pfgd->cItems = _cItems;

    for (UINT i = 0; i < _cItems; i++)
    {
        FILEDESCRIPTOR *pfd = &(pfgd->fgd[i]);
        StrCpyN(pfd->cFileName, NAME(&(_ppooi[i]->ooe)), ARRAYSIZE(pfd->cFileName));
        int len = lstrlen(pfd->cFileName);
        StrCpyN(pfd->cFileName + len, TEXT(".URL"), ARRAYSIZE(pfd->cFileName) - len);
    }

    pSTM->hGlobal = pfgd;
    
    return S_OK;
}

#ifdef UNICODE
// IExtractIconA members
HRESULT COfflineObjectItem::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    return IExtractIcon_GetIconLocationThunk((IExtractIconW *)this, uFlags, szIconFile, cchMax, piIndex, pwFlags);
}

HRESULT COfflineObjectItem::Extract(LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    return IExtractIcon_ExtractThunk((IExtractIconW *)this, pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}
#endif


HRESULT COfflineObjectItem::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags)
{
    ASSERT (piIndex && pwFlags);

    StrCpyN(szIconFile, TEXT("not used"), cchMax);

    SUBSCRIPTIONTYPE    subType = GetItemCategory(&(_ppooi[0]->ooe)); 
    switch (subType)   {
    case SUBSTYPE_URL:
        *piIndex = 10;
         break;
    case SUBSTYPE_CHANNEL:
        *piIndex = 11;
        break;
    case SUBSTYPE_DESKTOPCHANNEL:
    case SUBSTYPE_DESKTOPURL:
        *piIndex = 12;
        break;
    default:
        *piIndex = 13;   //  Unknown!
        break;
    }
    *pwFlags |= GIL_NOTFILENAME | GIL_PERINSTANCE | GIL_DONTCACHE;

    return NOERROR;
}

HRESULT COfflineObjectItem::Extract(LPCTSTR szIconFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize)
{
    * phiconLarge = * phiconSmall = NULL;
    
    LoadDefaultIcons();

    SUBSCRIPTIONTYPE    subType = GetItemCategory(&(_ppooi[0]->ooe)); 
    switch (subType)   {
    case SUBSTYPE_URL:
        * phiconLarge = * phiconSmall = g_webCrawlerIcon;
        break;
    case SUBSTYPE_CHANNEL:
        * phiconLarge = * phiconSmall = g_channelIcon;
        break;
    case SUBSTYPE_DESKTOPURL:
    case SUBSTYPE_DESKTOPCHANNEL:
        * phiconLarge = * phiconSmall = g_desktopIcon;
        break;
    default:
        break;
    }
    if (!(*phiconLarge))   {
        if (_ppooi[0]->ooe.bDesktop)
            * phiconLarge = * phiconSmall = g_desktopIcon;
        else if (_ppooi[0]->ooe.bChannel)
            * phiconLarge = * phiconSmall = g_channelIcon;
    }
    return NOERROR;
}
