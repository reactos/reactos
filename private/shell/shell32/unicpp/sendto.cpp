//
// Usage:
//
//  1. CoCreateInstance(CLSID_SendToMenu, IID_IContextMenu2) at some point.
//  2. QI(IID_IShellExtInit), hand in the data object to work with
//
//  3. pcm->QueryContextMenu with the id range to use, the send to code
//     will insert itself on the menu
//
//  3. Make it sure that hmenu outlives the CSendToMenu object.
//
//  4. Forward WM_INITMENUPOPUP, WM_DRAWITEM and WM_MEASUREITEM after
//    filtering them based on menuitem id or hmenu via HandleMenuMsg.
//
//  5. When WM_COMMAND is in the range specified by the return value of
//    QueryContextMenu, call InvokeCommand with the right id
//
//  6. Destructing this object is the only way to un-associate the hmenu
//    from this object. Therefore, this object MUST be released BEFORE
//    destructing hmenu.
//

#include "stdafx.h"
#pragma hdrstop

#include <runtask.h>
#include "datautil.h"
#include "idlcomm.h"

#define MAXEXTSIZE              (PATH_CCH_EXT+2)

#ifndef CMF_DVFILE
#define CMF_DVFILE       0x00010000     // "File" pulldown
#endif

#ifdef TF_SHDLIFE
#undef TF_SHDLIFE
#endif
#define TF_SHDLIFE 0

class CSendToMenu : public IContextMenu3, IShellExtInit, IOleWindow
{
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    // IOleWindow
    STDMETHOD(GetWindow)(HWND *phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) {return E_NOTIMPL;};
    
    LONG    _cRef;
    HMENU   _hmenu;
    UINT    _idCmdFirst;
    // UINT    _idCmdLast;
    BOOL    _bFirstTime;
    HWND    _hwnd;
    IDataObject *_pdtobj;
    CSendToMenu();
    ~CSendToMenu();
    
    friend HRESULT CSendToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);
};

CSendToMenu::CSendToMenu() : _cRef(1) 
{
    DllAddRef();
    TraceMsg(TF_SHDLIFE, "ctor CSendToMenu %x", this);
}

CSendToMenu::~CSendToMenu()
{
    TraceMsg(TF_SHDLIFE, "dtor CSendToMenu %x", this);
    
    if (_hmenu)
        FileMenu_DeleteAllItems(_hmenu);
    
    if (_pdtobj)
        _pdtobj->Release();
    DllRelease();
}

HRESULT CSendToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    // aggregation checking is handled in class factory
    
    CSendToMenu * psendto = new CSendToMenu();
    if (psendto) 
    {
        HRESULT hres = psendto->QueryInterface(riid, ppvOut);
        psendto->Release();
        return hres;
    }
    
    return E_OUTOFMEMORY;
}

HRESULT CSendToMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSendToMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CSendToMenu, IOleWindow),                        // IID_IOleWindow
        QITABENT(CSendToMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CSendToMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CSendToMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CSendToMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CSendToMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CSendToMenu::GetWindow(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;

    if (phwnd)
    {
        *phwnd = _hwnd;
        hr = S_OK;
    }

    return hr;
}

HRESULT CSendToMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return NOERROR;
    
    UINT idMax = idCmdFirst;
    
    _hmenu = CreatePopupMenu();
    if (_hmenu)
    {
        TCHAR szSendLinkTo[80];
        TCHAR szSendPageTo[80];
        MENUITEMINFO mii;
        
        // add a dummy item so we are identified at WM_INITMENUPOPUP time
        
        LoadString(g_hinst, IDS_SENDLINKTO, szSendLinkTo, ARRAYSIZE(szSendLinkTo));
        LoadString(g_hinst, IDS_SENDPAGETO, szSendPageTo, ARRAYSIZE(szSendPageTo));
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szSendLinkTo;
        mii.wID = idCmdFirst + 1;
        
        if (InsertMenuItem(_hmenu, 0, TRUE, &mii))
        {
            _idCmdFirst = idCmdFirst + 1;   // remember this for later
            // _idCmdLast  = idCmdLast;    // 
            
            mii.fType = MFT_STRING;
            mii.dwTypeData = szSendLinkTo;
            mii.wID = idCmdFirst;
            mii.fState = MF_DISABLED | MF_GRAYED;
            mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
            mii.hSubMenu = _hmenu;
            
            if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
            {
                idMax += 0x40;      // reserve space for this many items
                _bFirstTime = TRUE; // fill this at WM_INITMENUPOPUP time
                
                //                InsertMenu(hmenu, indexMenu + 1, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);
            }
            else
            {
                _hmenu = NULL;
            }
        }
    }
    _hmenu = NULL;
    return ResultFromShort(idMax - idCmdFirst);
}

HRESULT CSendToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
    
    if (_pdtobj)
    {
        LPITEMIDLIST pidlFolder = NULL;
        LPITEMIDLIST pidlItem = NULL;
        
        FileMenu_GetLastSelectedItemPidls(_hmenu, &pidlFolder, &pidlItem);
        if (pidlFolder && pidlItem)
        {
            IShellFolder *psf;
            hres = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (void**)&psf);
            if (SUCCEEDED(hres))
            {
                IDropTarget *pdrop;
                hres = psf->GetUIObjectOf(pici->hwnd, 1, (LPCITEMIDLIST *)&pidlItem, IID_IDropTarget, 0, (void**)&pdrop);
                if (SUCCEEDED(hres))
                {
                    DWORD grfKeyState = MK_LBUTTON; // default

                    if (GetAsyncKeyState(VK_CONTROL) < 0)
                        grfKeyState |= MK_CONTROL;

                    if (GetAsyncKeyState(VK_SHIFT) < 0)
                        grfKeyState |= MK_SHIFT;

                    if (GetAsyncKeyState(VK_MENU) < 0)
                        grfKeyState |= MK_ALT;          // menu's don't really allow this

                    if (grfKeyState == MK_LBUTTON)
                    {
                        // no modifieres, change default to COPY
                        grfKeyState = MK_LBUTTON | MK_CONTROL;
                        DataObj_SetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_COPY);
                    }

                    _hwnd = pici->hwnd;
                    IUnknown_SetSite(pdrop, SAFECAST(this, IOleWindow *));  // Let them have access to our HWND.
                    hres = SHSimulateDrop(pdrop, _pdtobj, grfKeyState, NULL, NULL);
                    IUnknown_SetSite(pdrop, NULL);

                    if (hres == S_FALSE)
                        ShellMessageBox(g_hinst, pici->hwnd, MAKEINTRESOURCE(IDS_SENDTO_ERRORMSG),
                                        MAKEINTRESOURCE(IDS_CABINET), MB_OK|MB_ICONEXCLAMATION);
                    pdrop->Release();
                }
                psf->Release();
            }
            
            ILFree(pidlItem);
            ILFree(pidlFolder);
        }
        else
            hres = E_FAIL;
    }
    else
        hres = E_INVALIDARG;
    return hres;
}

HRESULT CSendToMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

HRESULT CSendToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

HRESULT CSendToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lres)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        {
            if (_bFirstTime)
            {
                //In case of Shell_MergeMenus
                if (_hmenu == NULL)
                {
                    _hmenu = (HMENU)wParam;
                }
                _bFirstTime = FALSE;
                
                // delete the dummy entry
                DeleteMenu(_hmenu, 0, MF_BYPOSITION);
                
                FileMenu_CreateFromMenu(_hmenu, (COLORREF)-1, 0, NULL, 0, FMF_NONE);
                
                LPITEMIDLIST pidlSendTo = SHCloneSpecialIDList(NULL, CSIDL_SENDTO, TRUE);
                if (pidlSendTo) 
                {
                    FMCOMPOSE fmc = {0};
                    
                    fmc.cbSize     = SIZEOF(fmc);
                    fmc.id         = _idCmdFirst;
                    fmc.dwMask     = FMC_PIDL | FMC_FILTER;
                    fmc.pidlFolder = pidlSendTo;
                    fmc.dwFSFilter = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
                    
                    FileMenu_Compose(_hmenu, FMCM_REPLACE, &fmc);
                    
                    ILFree(pidlSendTo);
                }
            }
            else if (_hmenu != (HMENU)wParam)
            {
                // secondary cascade menu
                FileMenu_InitMenuPopup((HMENU)wParam);
            }
        }
        break;
        
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
            
            if (pdi->CtlType == ODT_MENU && pdi->itemID == _idCmdFirst) 
            {
                FileMenu_DrawItem(NULL, pdi);
            }
        }
        break;
        
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
            if (pmi->CtlType == ODT_MENU && pmi->itemID == _idCmdFirst) 
            {
                FileMenu_MeasureItem(NULL, pmi);
            }
        }
        break;

    case WM_MENUCHAR:
        {
            TCHAR ch = (TCHAR)LOWORD(wParam);
            HMENU hmenu = (HMENU)lParam;
            *lres = FileMenu_HandleMenuChar(hmenu, ch);
        }
        break;
    }
    return NOERROR;
}

HRESULT CSendToMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    if (_pdtobj)
        _pdtobj->Release();
    
    _pdtobj = pdtobj;
    
    if (_pdtobj)
        _pdtobj->AddRef();
    
    return NOERROR;
}

#define TARGETMENU
#ifdef TARGETMENU

HRESULT _BindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast)
{
    HRESULT hres;
    LPITEMIDLIST pidlLast = ILFindLastID(pidl);
    if (pidlLast)
    {
        IShellFolder *psfDesktop;
        SHGetDesktopFolder(&psfDesktop);
        
        // Special case for the object in the root
        if (pidlLast == pidl)
        {
            // REVIEW: should this be CreateViewObject?
            hres = psfDesktop->QueryInterface(riid, ppv);
        }
        else
        {
            USHORT uSave = pidlLast->mkid.cb;
            pidlLast->mkid.cb = 0;
            
            hres = psfDesktop->BindToObject(pidl, NULL, riid, ppv);
            
            pidlLast->mkid.cb = uSave;
        }
    }
    else
    {
        hres = E_INVALIDARG;
    }
    
    if (ppidlLast)
        *ppidlLast = pidlLast;
    
    return hres;
}

class CTargetMenu : public IShellExtInit, public IContextMenu3
{
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);

    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    HRESULT GetTargetMenu();
    
    ~CTargetMenu();
    CTargetMenu();
    
    LONG _cRef;
    HMENU _hmenu;
    UINT _idCmdFirst;
    BOOL _bFirstTime;
    
    IDataObject *_pdtobj;
    LPITEMIDLIST _pidlTargetParent;
    LPITEMIDLIST _pidlTarget;
    IContextMenu *_pcmTarget;
    
//    friend HRESULT CTargetMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
};

CTargetMenu::CTargetMenu() : _cRef(1) 
{
    TraceMsg(TF_SHDLIFE, "ctor CTargetMenu %x", this);
}

CTargetMenu::~CTargetMenu()
{
    if (_pidlTargetParent)
        ILFree(_pidlTargetParent);
    
    if (_pcmTarget)
        _pcmTarget->Release();
}

STDMETHODIMP CTargetMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTargetMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CTargetMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CTargetMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CTargetMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CTargetMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CTargetMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

STDMETHODIMP CTargetMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    if (_pdtobj)
        _pdtobj->Release();
    
    _pdtobj = pdtobj;
    
    if (_pdtobj)
        _pdtobj->AddRef();
    
    return NOERROR;
}

HRESULT CTargetMenu::GetTargetMenu()
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hres = _pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hres))
    {
        TCHAR szShortcut[MAX_PATH];
        
        if (DragQueryFile((HDROP)medium.hGlobal, 0, szShortcut, ARRAYSIZE(szShortcut)) &&
            PathIsShortcut(szShortcut))
        {
            IShellLink *psl;
            
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
            if (SUCCEEDED(hres))
            {
                IPersistFile *ppf;
                hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
                if (SUCCEEDED(hres)) 
                {
                    WCHAR wszFile[MAX_PATH];
                    
                    SHTCharToUnicode(szShortcut, wszFile, ARRAYSIZE(wszFile));
                    hres = ppf->Load(wszFile, 0);
                    if (SUCCEEDED(hres)) 
                    {
                        hres = psl->GetIDList(&_pidlTargetParent);
                        if (SUCCEEDED(hres) && _pidlTargetParent) 
                        {
                            IShellFolder *psf;
                            
                            hres = _BindToIDListParent(_pidlTargetParent, IID_IShellFolder, (void **)&psf, (LPCITEMIDLIST *)&_pidlTarget);
                            if (SUCCEEDED(hres)) 
                            {
                                hres = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&_pidlTarget, IID_IContextMenu, 0, (void **)&_pcmTarget);
                                if (SUCCEEDED(hres)) 
                                {
                                    ILRemoveLastID(_pidlTargetParent);
                                    hres = NOERROR;
                                }
                                psf->Release();
                            }
                        }
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        }
        ReleaseStgMedium(&medium);
    }
    return hres;
}

#define IDS_CONTENTSMENU    3
#define IDS_TARGETMENU      4

#define IDC_OPENCONTAINER   1
#define IDC_TARGET_LAST     1
#define NUM_TARGET_CMDS     0x40

STDMETHODIMP CTargetMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
        return NOERROR;
    
    UINT idMax = idCmdFirst;
    
    _hmenu = CreatePopupMenu();
    if (_hmenu)
    {
        TCHAR szString[80];
        MENUITEMINFO mii;
        
        // Add an open container menu item...
        // LoadString(g_hinst, IDS_OPENCONTAINER, szString, ARRAYSIZE(szString));
        lstrcpy(szString, TEXT("Open Container"));
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szString;
        mii.wID = idCmdFirst + IDC_OPENCONTAINER;
        
        if (InsertMenuItem(_hmenu, 0, TRUE, &mii))
        {
            _idCmdFirst = idCmdFirst;
            
            SetMenuDefaultItem(_hmenu, 0, TRUE);
            InsertMenu(hmenu, 1, MF_BYPOSITION | MF_SEPARATOR, (UINT)-1, NULL);
            
            
            // Insert our context menu....
            // LoadString(g_hinst, IDS_TARGETMENU, szString, ARRAYSIZE(szString));
            lstrcpy(szString, TEXT("Target"));
            
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
            mii.fType = MFT_STRING;
            mii.dwTypeData = szString;
            mii.wID = idCmdFirst;
            mii.fState = MF_DISABLED|MF_GRAYED;
            mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mii.hSubMenu = _hmenu;
            
            if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
            {
                idMax += NUM_TARGET_CMDS;    // reserve space for this many items
                _bFirstTime = TRUE; // fill this at WM_INITMENUPOPUP time
            }
            else
            {
                DestroyMenu(_hmenu);
                _hmenu = NULL;
            }
        }
    }
    return ResultFromShort(idMax - idCmdFirst);
}


STDMETHODIMP CTargetMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici) 
{
    UINT idCmd = LOWORD(lpici->lpVerb);
    
    switch (idCmd) 
    {
    case IDC_OPENCONTAINER:
        SHELLEXECUTEINFOA sei;
        
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_INVOKEIDLIST;
        sei.lpVerb = NULL;
        sei.hwnd         = lpici->hwnd;
        sei.lpParameters = lpici->lpParameters;
        sei.lpDirectory  = lpici->lpDirectory;
        sei.nShow        = lpici->nShow;
        sei.lpIDList     = _pidlTargetParent;
        
        
        SHWaitForFileToOpen(_pidlTargetParent, WFFO_ADD, 0L);
        if (ShellExecuteExA(&sei))
        {
            SHWaitForFileToOpen(_pidlTargetParent, WFFO_REMOVE | WFFO_WAIT, WFFO_WAITTIME);
            HWND hwndCabinet = FindWindow(TEXT("CabinetWClass"), NULL);
            if (hwndCabinet)
                SendMessage(hwndCabinet, CWM_SELECTITEM, SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_DESELECTOTHERS, (LPARAM)_pidlTarget);
        }
        else 
            SHWaitForFileToOpen(_pidlTargetParent, WFFO_REMOVE, 0L);
        break;
        
    default:
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
                lpici->fMask,
                lpici->hwnd,
                (LPCSTR)MAKEINTRESOURCE(idCmd - IDC_OPENCONTAINER),
                lpici->lpParameters, 
                lpici->lpDirectory,
                lpici->nShow,
        };
        
        return _pcmTarget->InvokeCommand(&ici);
    }
    return NOERROR;
}

HRESULT CTargetMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTargetMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}


STDMETHODIMP CTargetMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pres)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        if (_hmenu == (HMENU)wParam)
        {
            if (_bFirstTime)
            {
                _bFirstTime = FALSE;
                
                if (SUCCEEDED(GetTargetMenu()))
                    _pcmTarget->QueryContextMenu(_hmenu, IDC_TARGET_LAST, 
                    _idCmdFirst + IDC_TARGET_LAST, 
                    _idCmdFirst - IDC_TARGET_LAST + NUM_TARGET_CMDS, CMF_NOVERBS);
            }
            break;
        }
        
        // fall through... to pass on sub menu WM_INITMENUPOPUPs
        
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (_pcmTarget)
        {
            IContextMenu2 *pcm2;
            if (SUCCEEDED(_pcmTarget->QueryInterface(IID_IContextMenu2, (void **)&pcm2)))
            {
                pcm2->HandleMenuMsg(uMsg, wParam, lParam);
                pcm2->Release();
            }
        }
        break;
    }
    
    return NOERROR;
}

#if 0
STDAPI CTargetMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    
    CTargetMenu * psendto = new CTargetMenu();
    if (psendto) 
    {
        *ppunk = SAFECAST(psendto, IContextMenu2 *);
        return NOERROR;
    }
    
    return E_OUTOFMEMORY;
}
#endif

#endif  // TARGETMENU


#ifdef CONTENT

#define IDC_ITEMFIRST (IDC_PRESSMOD + 10)
#define MENU_TIMEOUT (1000)

#define DPA_LAST    0x7fffffff

class CHIDA
{
public:
    CHIDA(HGLOBAL hIDA, IUnknown *punk)
    {
        m_punk = punk;
        m_hIDA = hIDA;
        m_pIDA = (LPIDA)GlobalLock(hIDA);
    }
    ~CHIDA() 
    {
        GlobalUnlock(m_hObj);
        
        if (m_punk) 
            m_punk->Release();
        else
            GlobalFree(m_hIDA);
        
    }
    
    LPCITEMIDLIST operator [](UINT nIndex)
    {
        if (nIndex > m_pIDA->cidl)
            return(NULL);
        
        return (LPCITEMIDLIST)(((BYTE *)m_pIDA) + m_pIDA->aoffset[nIndex]);
    }
    
private:
    HGLOBAL m_hIDA;
    LPIDA m_pIDA;
    IUnknown *m_punk;
};


class CVoidArray
{
public:
    CVoidArray() : m_dpa(NULL), m_dsa(NULL) {}
    ~CVoidArray()
    {
        if (m_dpa)
        {
            DPA_Destroy(m_dpa);
        }
        if (m_dsa)
        {
            DSA_Destroy(m_dsa);
        }
    }
    
    void *operator[](int i);
    
    BOOL Init(UINT uSize, UINT uJump);
    
    BOOL Add(void *pv);
    
    void Sort(PFNDPACOMPARE pfnCompare, LPARAM lParam)
    {
        m_pfnCompare = pfnCompare;
        m_lParam = lParam;
        
        DPA_Sort(m_dpa, ArrayCompare, (LPARAM)this);
    }
    
private:
    static int CALLBACK ArrayCompare(void *pv1, void *pv2, LPARAM lParam);
    
    HDPA m_dpa;
    HDSA m_dsa;
    
    PFNDPACOMPARE m_pfnCompare;
    LPARAM void *;
};


m_lParam CVoidArray::operator[](int i)
{
    if (!m_dpa || i>=DPA_GetPtrCount(m_dpa))
    {
        return(NULL);
    }
    
    return(DSA_GetItemPtr(m_dsa, (int)DPA_GetPtr(m_dpa, i)));
}


BOOL CVoidArray::Init(UINT uSize, UINT uJump)
{
    m_dpa = DPA_Create(uJump);
    m_dsa = DSA_Create(uSize, uJump);
    return(m_dpa && m_dsa);
}


BOOL CVoidArray::Add(void *pv)
{
    int iItem = DSA_InsertItem(m_dsa, DPA_LAST, pv);
    if (iItem < 0)
    {
        return(FALSE);
    }
    
    if (DPA_InsertPtr(m_dpa, DPA_LAST, (void *)iItem) < 0)
    {
        DSA_DeleteItem(m_dsa, iItem);
        return(FALSE);
    }
    
    return(TRUE);
}


int CALLBACK CVoidArray::ArrayCompare(void *pv1, void *pv2, LPARAM lParam)
{
    CVoidArray *pThis = (CVoidArray *)lParam;
    
    return(pThis->m_pfnCompare(DSA_GetItemPtr(pThis->m_dsa, (int)pv1),
        DSA_GetItemPtr(pThis->m_dsa, (int)pv2), pThis->m_lParam));
}


class CContentItemData
{
public:
    CContentItemData() : m_dwDummy(0) {Empty();}
    ~CContentItemData() {Free();}
    
    void Free();
    void Empty() {m_pidl=NULL; m_hbm=NULL;}
    
    // Here to work around a Tray menu bug
    DWORD m_dwDummy;
    LPITEMIDLIST m_pidl;
    HBITMAP m_hbm;
};


void CContentItemData::Free()
{
    if (m_pidl) ILFree(m_pidl);
    if (m_hbm) DeleteObject(m_hbm);
    Empty();
}


class CContentItemDataArray : public CVoidArray
{
public:
    CContentItemDataArray(LPCITEMIDLIST pidlFolder) : m_pidlFolder(pidlFolder) {}
    ~CContentItemDataArray();
    
    CContentItemData * operator[](int i) {return((CContentItemData*)(*(CVoidArray*)this)[i]);}
    
    HRESULT Init();
    BOOL Add(CContentItemData *pv) {return(CVoidArray::Add((void *)pv));}
    
private:
    static int CALLBACK DefaultSort(void *pv1, void *pv2, LPARAM lParam);
    
    HRESULT GetShellFolder(IShellFolder **ppsf);
    BOOL IsLocal();
    
    LPCITEMIDLIST CContentItemDataArray;
};


CContentItemDataArray::~CContentItemDataArray()
{
    for (int i=0;; ++i)
    {
        CContentItemData *pID = (*this)[i];
        if (!pID)
            break;
        pID->Free();
    }
}


int CALLBACK CContentItemDataArray::DefaultSort(void *pv1, void *pv2, LPARAM lParam)
{
    IShellFolder *psfFolder = (IShellFolder *)lParam;
    CContentItemData *pID1 = (CContentItemData *)pv1;
    CContentItemData *pID2 = (CContentItemData *)pv2;
    
    HRESULT hRes = psfFolder->CompareIDs(0, pID1->m_pidl, pID2->m_pidl);
    if (FAILED(hRes))
    {
        return(0);
    }
    
    return((short)ShortFromResult(hRes));
}


HRESULT CContentItemDataArray::GetShellFolder(IShellFolder **ppsf)
{
    IShellFolder *psfDesktop;
    HRESULT hRes = CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder,
        (void **)&psfDesktop);
    if (FAILED(hRes))
    {
        return hRes;
    }
    CEnsureRelease erDesktop(psfDesktop);
    
    return psfDesktop->BindToObject(m_pidlFolder, NULL, IID_IShellFolder, (void **)ppsf);
}


BOOL CContentItemDataArray::IsLocal()
{
    TCHAR szPath[MAX_PATH];
    
    if (!SHGetPathFromIDList(m_pidlFolder, szPath))
    {
        return(FALSE);
    }
    
    CharUpper(szPath);
    return(DriveType(szPath[0]-'A') == DRIVE_FIXED);
}


HRESULT CContentItemDataArray::Init()
{
    if (!IsLocal() && !(GetKeyState(VK_SHIFT)&0x8000))
    {
        return(S_FALSE);
    }
    
    if (!CVoidArray::Init(sizeof(CContentItemData), 16))
    {
        return(E_OUTOFMEMORY);
    }
    
    IShellFolder *psfFolder;
    HRESULT hRes = GetShellFolder(&psfFolder);
    if (FAILED(hRes))
    {
        return(hRes);
    }
    CEnsureRelease erFolder(psfFolder);
    
    IEnumIDList *penumFolder;
    hRes = psfFolder->EnumObjects(NULL,
        SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN, &penumFolder);
    if (FAILED(hRes))
    {
        return(hRes);
    }
    CEnsureRelease erEnumFolder(penumFolder);
    
    ULONG cNum;
    
    DWORD dwStart = 0;
    hRes = S_OK;
    
    for (;;)
    {
        CContentItemData cID;
        if (penumFolder->Next(1, &cID.m_pidl, &cNum)!=S_OK || cNum!=1)
        {
            // Just in case
            cID.Empty();
            break;
        }
        
        if (!dwStart)
        {
            dwStart = GetTickCount();
        }
        else if (!(GetAsyncKeyState(VK_SHIFT)&0x8000)
            && GetTickCount()-dwStart>MENU_TIMEOUT)
        {
            // Only go for 2 seconds after the first Next call
            hRes = S_FALSE;
            break;
        }
        
        CMenuDraw mdItem(m_pidlFolder, cID.m_pidl);
        cID.m_hbm = mdItem.CreateBitmap(TRUE);
        if (!cID.m_hbm)
        {
            continue;
        }
        
        if (!Add(&cID))
        {
            break;
        }
        
        // Like a Detach(); Make sure we do not free stuff
        cID.Empty();
    }
    
    Sort(DefaultSort, (LPARAM)psfFolder);
    
    return(hRes);
}


class CContentItemInfo : public tagMENUITEMINFOA
{
public:
    CContentItemInfo(UINT fMsk) {fMask=fMsk; cbSize=sizeof(MENUITEMINFO);}
    ~CContentItemInfo() {}
    
    BOOL GetMenuItemInfo(HMENU hm, int nID, BOOL bByPos)
    {
        return(::GetMenuItemInfo(hm, nID, bByPos, this));
    }
    
    CContentItemData *GetItemData() {return((CContentItemData*)dwItemData);}
    void SetItemData(CContentItemData *pd) {dwItemData=(DWORD)pd; fMask|=MIIM_DATA;}
    
    HBITMAP GetBitmap() {return(fType&MFT_BITMAP ? dwTypeData : NULL);}
    void SetBitmap(HBITMAP hb) {dwTypeData=(LPSTR)hb; fType|=MFT_BITMAP;}
};

#define CXIMAGEGAP 6
#define CYIMAGEGAP 4

class CWindowDC
{
public:
    CWindowDC(HWND hWnd) : m_hWnd(hWnd) {m_hDC=GetDC(hWnd);}
    ~CWindowDC() {ReleaseDC(m_hWnd, m_hDC);}
    
    operator HDC() {return(m_hDC);}
    
private:
    HDC m_hDC;
    HWND m_hWnd;
};


class CDCTemp
{
public:
    CDCTemp(HDC hDC) : m_hDC(hDC) {}
    ~CDCTemp() {if (m_hDC) DeleteDC(m_hDC);}
    
    operator HDC() {return(m_hDC);}
    
private:
    HDC m_hDC;
};


class CRefMenuFont
{
public:
    CRefMenuFont(CMenuDraw *pmd) {m_pmd = pmd->InitMenuFont() ? pmd : NULL;}
    ~CRefMenuFont() {if (m_pmd) m_pmd->ReleaseMenuFont();}
    
    operator BOOL() {return(m_pmd != NULL);}
    
private:
    CMenuDraw *m_pmd;
};


BOOL CMenuDraw::InitMenuFont()
{
    if (m_cRefFont.GetRef())
    {
        m_cRefFont.AddRef();
        return(TRUE);
    }
    
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm),
        (void far *)(LPNONCLIENTMETRICS)&ncm, FALSE);
    
    m_hfMenu = CreateFontIndirect(&ncm.lfMenuFont);
    if (m_hfMenu)
    {
        m_cRefFont.AddRef();
        return TRUE;
    }
    return FALSE;
}


void CMenuDraw::ReleaseMenuFont()
{
    if (m_cRefFont.Release())
    {
        return;
    }
    
    DeleteObject(m_hfMenu);
    m_hfMenu = NULL;
}


BOOL CMenuDraw::InitStringAndIcon()
{
    if (!m_pidlAbs)
    {
        return(FALSE);
    }
    
    if (m_pszString)
    {
        return(TRUE);
    }
    
    SHFILEINFO sfi;
    
    if (!SHGetFileInfo((LPCSTR)m_pidlAbs, 0, &sfi, sizeof(sfi),
        SHGFI_DISPLAYNAME | SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL))
    {
        return(FALSE);
    }
    
    if (!Str_SetPtr(&m_pszString, sfi.szDisplayName))
    {
        DestroyIcon(sfi.hIcon);
    }
    
    m_hiItem = sfi.hIcon;
    
    return(TRUE);
}


BOOL CMenuDraw::GetName(LPSTR szName)
{
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    lstrcpyn(szName, m_pszString, MAX_PATH);
    
    return(TRUE);
}


BOOL CMenuDraw::GetExtent(SIZE *pSize, BOOL bFull)
{
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    HDC hDC = GetDC(NULL);
    HFONT hfOld = (HFONT)SelectObject(hDC, m_hfMenu);
    
    BOOL bRet = GetTextExtentPoint32(hDC, m_pszString, lstrlen(m_pszString), pSize);
    
    if (hfOld)
    {
        SelectObject(hDC, hfOld);
    }
    ReleaseDC(NULL, hDC);
    
    if (bRet)
    {
        int cxIcon = GetSystemMetrics(SM_CXSMICON);
        int cyIcon = GetSystemMetrics(SM_CYSMICON);
        
        pSize->cy = max(pSize->cy, cyIcon);
        
        if (bFull)
        {
            pSize->cx += CXIMAGEGAP + GetSystemMetrics(SM_CXSMICON) + CXIMAGEGAP + CXIMAGEGAP;
            pSize->cy += CYIMAGEGAP;
        }
        else
        {
            pSize->cx = cxIcon;
        }
    }
    
    return(bRet);
}


BOOL CMenuDraw::DrawItem(HDC hDC, RECT *prc, BOOL bFull)
{
    RECT rc = *prc;
    int cxIcon = GetSystemMetrics(SM_CXSMICON);
    int cyIcon = GetSystemMetrics(SM_CYSMICON);
    
    FillRect(hDC, prc, GetSysColorBrush(COLOR_MENU));
    
    if (!InitStringAndIcon())
    {
        return(FALSE);
    }
    
    if (bFull)
    {
        rc.left += CXIMAGEGAP;
    }
    
    DrawIconEx(hDC, rc.left, rc.top + (rc.bottom-rc.top-cyIcon)/2, m_hiItem,
        0, 0, 0, NULL, DI_NORMAL);
    
    if (!bFull)
    {
        // All done
        return(TRUE);
    }
    
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    rc.left += cxIcon + CXIMAGEGAP;
    HFONT hfOld = SelectObject(hDC, m_hfMenu);
    
    COLORREF crOldBk = SetBkColor  (hDC, GetSysColor(COLOR_MENU));
    COLORREF crOldTx = SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
    
    DrawText(hDC, m_pszString, -1, &rc,
        DT_VCENTER|DT_SINGLELINE|DT_LEFT|DT_NOCLIP);
    
    SetBkColor  (hDC, GetSysColor(crOldBk));
    SetTextColor(hDC, GetSysColor(crOldTx));
    
    if (hfOld)
    {
        SelectObject(hDC, hfOld);
    }
    
    return(TRUE);
}


HBITMAP CMenuDraw::CreateBitmap(BOOL bFull)
{
    SIZE size;
    
    // Reference font here so we do not create it twice
    CRefMenuFont crFont(this);
    if (!(BOOL)crFont)
    {
        return(FALSE);
    }
    
    if (!GetExtent(&size, bFull))
    {
        return(NULL);
    }
    
    CWindowDC wdcScreen(NULL);
    CDCTemp cdcTemp(CreateCompatibleDC(wdcScreen));
    if (!(HDC)cdcTemp)
    {
        return(NULL);
    }
    
    HBITMAP hbmItem = CreateCompatibleBitmap(wdcScreen, size.cx, size.cy);
    if (!hbmItem)
    {
        return(NULL);
    }
    
    HBITMAP hbmOld = (HBITMAP)SelectObject(cdcTemp, hbmItem);
    RECT rc = { 0, 0, size.cx, size.cy };
    BOOL bDrawn = DrawItem(cdcTemp, &rc, bFull);
    
    SelectObject(cdcTemp, hbmOld);
    
    if (!bDrawn)
    {
        DeleteObject(hbmItem);
        hbmItem = NULL;
    }
    
    return(hbmItem);
}


class CContentMenu : public IShellExtInit, public IContextMenu2
{
    CContentMenu();
    ~CContentMenu();
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    static HMENU LoadPopupMenu(UINT id, UINT uSubMenu);
    HRESULT InitMenu();
    
    LPITEMIDLIST m_pidlFolder;
    HMENU m_hmItems;
    
    friend STDAPI CContentMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    
    class CContentItemDataArray *m_pIDs;
};

CContentMenu::CContentMenu() : m_pidlFolder(0), m_hmItems(NULL), m_pIDs(NULL)
{
}

CContentMenu::~CContentMenu()
{
    if (m_pidlFolder)
        ILFree(m_pidlFolder);
    
    if (m_hmItems)
        DestroyMenu(m_hmItems);
    
    if (m_pIDs)
        delete m_pIDs;
}

HRESULT CContentMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CContentMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CContentMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CContentMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CSendToMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CContentMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CContentMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}


STDMETHODIMP CContentMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    if (!pdtobj)
    {
        return(E_INVALIDARG);
    }
    
    UINT cfHIDA = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    FORMATETC fmte = {(USHORT)cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    HRESULT hRes = pdtobj->GetData(&fmte, &medium);
    if (FAILED(hRes))
    {
        return(hRes);
    }
    
    CHIDA chSel(medium.hGlobal, medium.pUnkForRelease);
    
    if (m_pidlFolder)
    {
        ILFree(m_pidlFolder);
    }
    
    m_pidlFolder = ILCombine(chSel[0], chSel[1]);
    return m_pidlFolder ? NOERROR : E_OUTOFMEMORY;
}


// *** IContextMenu methods ***
STDMETHODIMP CContentMenu::QueryContextMenu(
                                            HMENU hmenu,
                                            UINT indexMenu,
                                            UINT idCmdFirst,
                                            UINT idCmdLast,
                                            UINT uFlags)
{
    MENUITEMINFO mii;
    UINT idMax = idCmdFirst + IDC_PRESSMOD;
    
    if (uFlags & CMF_DEFAULTONLY)
        return NOERROR;
    
    HMENU hmenuSub =  CreatePopupMenu();
    if (!hmenuSub)
        return E_OUTOFMEMORY;
    
    char szTitle[80];
    LoadString(g_hinst, IDS_CONTENTSMENU, szTitle, sizeof(szTitle));
    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = szTitle;
    mii.wID = idCmdFirst + IDC_PRESSMOD;
    mii.fState = MF_DISABLED|MF_GRAYED;
    idMax = mii.wID + 1;
    
    HRESULT hRes = InitMenu();
    if (SUCCEEDED(hRes))
    {
        UINT idM = Shell_MergeMenus(hmenuSub, m_hmItems, 0, idCmdFirst, idCmdLast, 0);
        
        if (GetMenuItemCount(hmenuSub) > 0)
        {
            mii.fMask = MIIM_TYPE | MIIM_SUBMENU;
            mii.hSubMenu = hmenuSub;
            idMax = idM;
        }
    }
    
    if (InsertMenuItem(hmenu, indexMenu, TRUE, &mii) && (mii.fMask & MIIM_SUBMENU))
    {
    }
    else
        DestroyMenu(hmenuSub);
    
    return(ResultFromShort(idMax - idCmdFirst));
}


STDMETHODIMP CContentMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        return(E_INVALIDARG);
    }
    
    UINT uID = (UINT)LOWORD((DWORD)lpici->lpVerb);
    switch (uID)
    {
    case IDC_PRESSMOD:
        ShellMessageBox(g_hinst, lpici->hwnd, MAKEINTRESOURCE(IDS_PRESSMOD),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    case IDC_SHIFTMORE:
        ShellMessageBox(g_hinst, lpici->hwnd, MAKEINTRESOURCE(IDS_SHIFTMORE),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    default:
        if (m_hmItems)
        {
            CContentItemInfo mii(MIIM_DATA);
            
            if (!mii.GetMenuItemInfo(m_hmItems, uID, FALSE))
                return E_INVALIDARG;
            
            CContentItemData *pData = mii.GetItemData();
            if (!pData || !pData->m_pidl)
                return E_INVALIDARG;
            
            LPITEMIDLIST pidlAbs = ILCombine(m_pidlFolder, pData->m_pidl);
            if (!pidlAbs)
                return E_OUTOFMEMORY;
            
            SHELLEXECUTEINFO sei;
            
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_INVOKEIDLIST;
            sei.lpVerb = NULL;
            sei.hwnd         = lpici->hwnd;
            sei.lpParameters = lpici->lpParameters;
            sei.lpDirectory  = lpici->lpDirectory;
            sei.nShow        = lpici->nShow;
            sei.lpIDList = (void *)pidlAbs;
            
            ShellExecuteEx(&sei);
            
            ILFree(pidlAbs);
            
            break;
        }
        
        return(E_UNEXPECTED);
    }
    
    return(NOERROR);
}


STDMETHODIMP CContentMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}


STDMETHODIMP CContentMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return E_NOTIMPL;
}


HRESULT CContentMenu::InitMenu()
{
    if (!m_pidlFolder)
        return E_UNEXPECTED;
    
    if (m_hmItems)
        return NOERROR;
    
    if (m_pIDs)
        return E_UNEXPECTED;
    
    m_pIDs = new CContentItemDataArray(m_pidlFolder);
    if (!m_pIDs)
        return E_OUTOFMEMORY;
    
    HRESULT hRes = m_pIDs->Init();
    if (FAILED(hRes))
        return hRes;
    
    BOOL bGotAll = (hRes == S_OK);
    
    m_hmItems = CreatePopupMenu();
    if (!m_hmItems)
        return E_OUTOFMEMORY;
    
    UINT cy = 0;
    BOOL bBitmaps = TRUE;
    
    if (!bGotAll)
    {
        HMENU hmenuMerge = LoadPopupMenu(MENU_ITEMCONTEXT, 1);
        if (hmenuMerge)
        {
            Shell_MergeMenus(m_hmItems, hmenuMerge, 0, 0, 1000, 0);
            DestroyMenu(hmenuMerge);
        }
        else
            return E_OUTOFMEMORY;
    }
    
    for (int i=0;; ++i)
    {
        CContentItemData * pID = (*m_pIDs)[i];
        if (!pID)
        {
            // All done
            break;
        }
        
        UINT id = IDC_ITEMFIRST + i;
        
        BITMAP bm;
        GetObject(pID->m_hbm, sizeof(bm), &bm);
        
        cy += bm.bmHeight;
        
        if (i==0 && !bGotAll)
        {
            // Account for the menu item we added above
            cy += cy;
        }
        
        CContentItemInfo mii(MIIM_ID | MIIM_TYPE | MIIM_DATA);
        mii.fType = 0;
        mii.SetItemData(pID);
        mii.wID = id;
        
        if (cy >= (UINT)GetSystemMetrics(SM_CYSCREEN)*4/5)
        {
            // Put in a menu break when we fill 80% the screen
            mii.fType |= MFT_MENUBARBREAK;
            cy = bm.bmHeight;
            // bBitmaps = FALSE;
        }
        
        mii.SetBitmap(pID->m_hbm);
        
        if (!InsertMenuItem(m_hmItems, DPA_LAST, TRUE, &mii))
        {
            return(E_OUTOFMEMORY);
        }
    }
    
    return(m_hmItems ? NOERROR: HMENU);
}


HMENU CContentMenu::LoadPopupMenu(UINT id, UINT uSubMenu)
{
    HMENU hmParent = LoadMenu(g_hinst, MAKEINTRESOURCE(id));
    if (!hmParent)
        return(NULL);
    
    HMENU hmPopup = GetSubMenu(hmParent, uSubMenu);
    RemoveMenu(hmParent, uSubMenu, MF_BYPOSITION);
    DestroyMenu(hmParent);
    
    return(hmPopup);
}

// 57D5ECC0-A23F-11CE-AE65-08002B2E1262
DEFINE_GUID(CLSID_ContentMenu, 0x57D5ECC0L, 0xA23F, 0x11CE, 0xAE, 0x65, 0x08, 0x00, 0x2B, 0x2E, 0x12, 0x62);

STDAPI CContentMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    
    CContentMenu *pmenu = new CContentMenu();
    if (pmenu) 
    {
        *ppunk = SAFECAST(pmenu, IContextMenu2 *);
        return NOERROR;
    }
    
    return E_OUTOFMEMORY;
}

#endif // CONTENT

#define CXIMAGEGAP      6

#define SRCSTENCIL              0x00B8074AL

//
//This is included by shell32/shellprv.h I'm not sure where this is in shdocvw
#define CCH_KEYMAX  64

#define MSAAHACK 0xAA0DF00DL

typedef struct 
{
    struct tagMSAAHACK
    {
        DWORD dwSignature;        // Hack for MSAA
        DWORD dwStrLen;
        LPTSTR pszString;
    } msaaHack;

    TCHAR chPrefix;
    TCHAR szMenuText[CCH_KEYMAX];
    TCHAR szExt[MAXEXTSIZE];
    TCHAR szClass[CCH_KEYMAX];
    DWORD dwFlags;
    int iImage;
    TCHAR szUserFile[CCH_KEYMAX];
} NEWOBJECTINFO, * LPNEWOBJECTINFO;





typedef struct 
{
    int type;
    void *lpData;
    DWORD cbData;
    HKEY hkeyNew;
} NEWFILEINFO, *LPNEWFILEINFO;

typedef struct {
    ULONG       cbStruct;
    ULONG       ver;
    SYSTEMTIME  lastupdate;
} SHELLNEW_CACHE_STAMP;

// ShellNew config flags
#define SNCF_DEFAULT    0x0000
#define SNCF_NOEXT      0x0001
#define SNCF_USERFILES  0x0002

#define NEWTYPE_DATA    0x0003
#define NEWTYPE_FILE    0x0004
#define NEWTYPE_NULL    0x0005
#define NEWTYPE_COMMAND 0x0006
#define NEWTYPE_FOLDER  0x0007
#define NEWTYPE_LINK    0x0008

#define NEWITEM_FOLDER  0
#define NEWITEM_LINK    1
#define NEWITEM_MAX     2

class CNewMenu : public IContextMenu3, IShellExtInit, IObjectWithSite
{
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);
    
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown*);
    STDMETHOD(GetSite)(REFIID,void**);
    
    LONG            _cRef;
    HMENU           _hmenu;
    UINT            _idCmdFirst;
    HIMAGELIST      _himlSystemImageList;
    // UINT         _idCmdLast;
    IDataObject *_pdtobj;
    IShellView2*    _pShellView2;
    LPITEMIDLIST    _pidlFolder;
    POINT           _ptNewItem;
    BOOL            _bMenuBar;
    
    CNewMenu();
    ~CNewMenu();
    LPNEWOBJECTINFO _lpnoiLast;
    
    friend HRESULT CNewMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut);

private:
    //Handle Menu messages submitted to HandleMenuMsg
    void DrawItem(DRAWITEMSTRUCT *lpdi);
    LRESULT MeasureItem(MEASUREITEMSTRUCT *lpmi);
    BOOL InitMenuPopup(HMENU hMenu);
    
    //Internal Helpers
    LPNEWOBJECTINFO GetItemData(HMENU hmenu, UINT iItem);
    HRESULT RunCommand(HWND hwnd, LPTSTR pszPath, LPTSTR pszRun);
    HRESULT CopyTemplate(LPCTSTR szPath, NEWFILEINFO *pnfi);

    // Generates it from the Fragment and _pidlFolder
    BOOL GeneratePidlFromPath(LPTSTR pszPath, LPITEMIDLIST* ppidl);

    HRESULT _MatchMenuItem(TCHAR ch, LRESULT* plRes);
    
    HRESULT ConsolidateMenuItems(BOOL bForce);
    void    WaitForConsolidation(BOOL bWait = TRUE);

    HANDLE               _hConsolidationEvent;
};

void GetConfigFlags(HKEY hkey, DWORD * pdwFlags)
{
    TCHAR szTemp[MAX_PATH];
    DWORD cbData = ARRAYSIZE(szTemp);
    
    *pdwFlags = SNCF_DEFAULT;
    
    if (SHQueryValueEx(hkey, TEXT("NoExtension"), 0, NULL, (BYTE *)szTemp, &cbData) == ERROR_SUCCESS) 
    {
        *pdwFlags |= SNCF_NOEXT;
    }
}

BOOL GetNewFileInfoForKey(HKEY hkeyExt, NEWFILEINFO *pnfi, DWORD * pdwFlags)
{
    BOOL fRet = FALSE;
    HKEY hKey; // this gets the \\.ext\progid  key
    HKEY hkeyNew;
    TCHAR szProgID[80];
    LONG lSize = SIZEOF(szProgID);
    
    // open the Newcommand
    if (SHRegQueryValue(hkeyExt, NULL,  szProgID, &lSize) != ERROR_SUCCESS) {
        return FALSE;
    }
    
    if (RegOpenKey(hkeyExt, szProgID, &hKey) != ERROR_SUCCESS) {
        hKey = hkeyExt;
    }
    
    if (RegOpenKey(hKey, TEXT("ShellNew"), &hkeyNew) == ERROR_SUCCESS) {
        DWORD dwType, cbData;
        TCHAR szTemp[MAX_PATH];
        HKEY hkeyConfig;
        
        // Are there any config flags?
        if (pdwFlags) {
            
            if (RegOpenKey(hkeyNew, TEXT("Config"), &hkeyConfig) == ERROR_SUCCESS) {
                GetConfigFlags(hkeyConfig, pdwFlags);
                RegCloseKey(hkeyConfig);
            } else
                *pdwFlags = 0;
        }
        
        if (cbData = SIZEOF(szTemp), (SHQueryValueEx(hkeyNew, TEXT("NullFile"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS)) 
        {
            fRet = TRUE;
            if (pnfi)
            {
                pnfi->type = NEWTYPE_NULL;
                pnfi->cbData = 0;
                pnfi->lpData = NULL;
            }
        } 
        else if (cbData = SIZEOF(szTemp), (SHQueryValueEx(hkeyNew, TEXT("FileName"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            fRet = TRUE;
            if (pnfi) {
                pnfi->type = NEWTYPE_FILE;
                pnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the file easily
                ASSERT((LPTSTR*)pnfi->lpData == NULL);
                pnfi->lpData = StrDup(szTemp);
                
                hkeyNew = NULL;
            }
        } 
        else if (cbData = SIZEOF(szTemp), (SHQueryValueEx(hkeyNew, TEXT("command"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            
            fRet = TRUE;
            if (pnfi) {
                pnfi->type = NEWTYPE_COMMAND;
                pnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the command easily
                ASSERT((LPTSTR*)pnfi->lpData == NULL);
                pnfi->lpData = StrDup(szTemp);
                hkeyNew = NULL;
            }
        } 
        else if ((SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, NULL, &cbData) == ERROR_SUCCESS) && cbData) 
        {
            // yes!  the data for a new file is stored in the registry
            fRet = TRUE;
            // do they want the data?
            if (pnfi)
            {
                pnfi->type = NEWTYPE_DATA;
                pnfi->cbData = cbData;
                pnfi->lpData = (void*)LocalAlloc(LPTR, cbData);
#ifdef UNICODE
                if (pnfi->lpData)
                {
                    if (dwType == REG_SZ)
                    {
                        
                        //
                        //  Get the Unicode data from the registry.
                        //
                        LPWSTR pszTemp = (LPWSTR)LocalAlloc(LPTR, cbData);
                        if (pszTemp)
                        {
                            SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (LPBYTE)pszTemp, &cbData);
                            
                            pnfi->cbData = SHUnicodeToAnsi(pszTemp, (LPSTR)pnfi->lpData, cbData);
                            if (pnfi->cbData == 0)
                            {
                                LocalFree(pnfi->lpData);
                                pnfi->lpData = NULL;
                            }
                            
                            LocalFree(pszTemp);
                        }
                        else
                        {
                            LocalFree(pnfi->lpData);
                            pnfi->lpData = NULL;
                        }
                    }
                    else
                    {
                        SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (BYTE*)pnfi->lpData, &cbData);
                    }
                }
#else
                if (pnfi->lpData)
                {
                    SHQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (BYTE*)pnfi->lpData, &cbData);
                }
#endif
            }
        }
        
        
        if (hkeyNew)
            RegCloseKey(hkeyNew);
    }
    
    if (hKey != hkeyExt) {
        RegCloseKey(hKey);
    }
    return fRet;
}

BOOL GetNewFileInfoForExtension(LPNEWOBJECTINFO lpnoi, NEWFILEINFO *pnfi, HKEY* phKey, LPINT piIndex)
{
    TCHAR szValue[80];
    LONG lSize = SIZEOF(szValue);
    HKEY hkeyNew;
    BOOL fRet = FALSE;;
    
    if (phKey && ((*phKey) == (HKEY)-1))
    {
        // we're done
        return FALSE;
    }
    
    // do the ShellNew key stuff if there's no phKey passed in (which means
    // use the info in lpnoi to get THE one) and there's no UserFile specified.
    // 
    // if there IS a UserFile specified, then it's a file, and that szUserFile points to it..
    if (!phKey && !lpnoi->szUserFile[0] ||
        (phKey && !*phKey)) 
    {
        // check the new keys under the class id (if any)
        TCHAR szSubKey[128];
        wsprintf(szSubKey, TEXT("%s\\CLSID"), lpnoi->szClass);
        lSize = SIZEOF(szValue);
        if (SHRegQueryValue(HKEY_CLASSES_ROOT, szSubKey, szValue, &lSize) == ERROR_SUCCESS)
        {
            wsprintf(szSubKey,TEXT("CLSID\\%s"), szValue);
            lSize = SIZEOF(szValue);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, &hkeyNew) == ERROR_SUCCESS)
            {
                fRet = GetNewFileInfoForKey(hkeyNew, pnfi, &lpnoi->dwFlags);
                RegCloseKey(hkeyNew);
            }
        }
        
        // otherwise check under the type extension... do the extension, not the type
        // so that multi-ext to 1 type will work right
        if (!fRet && (RegOpenKey(HKEY_CLASSES_ROOT, lpnoi->szExt, &hkeyNew) == ERROR_SUCCESS))
        {
            fRet = GetNewFileInfoForKey(hkeyNew, pnfi, &lpnoi->dwFlags);
            RegCloseKey(hkeyNew);
        }
        
        if (phKey)
        {
            // if we're iterating, then we've got to open the key now...
            wsprintf(szSubKey, TEXT("%s\\%s\\ShellNew\\FileName"), lpnoi->szExt, lpnoi->szClass);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, phKey) == ERROR_SUCCESS)
            {
                *piIndex = 0;
                
                // if we didn't find one of the default ones above, 
                // try it now
                // otherwise just return success or failure on fRet
                if (!fRet)
                {
                    goto Iterate;
                } 
            }
            else
            {
                *phKey = (HKEY)-1;
            }
        }
    }
    else if (!phKey && lpnoi->szUserFile[0])
    {
        // there's no key, so just return info about szUserFile
        pnfi->type = NEWTYPE_FILE;
        pnfi->lpData = StrDup(lpnoi->szUserFile);
        pnfi->hkeyNew = NULL;
        
        fRet = TRUE;
    }
    else if (phKey)
    {
        DWORD dwSize;
        DWORD dwData;
        DWORD dwType;
        // we're iterating through...
        
Iterate:
        
        dwSize = ARRAYSIZE(lpnoi->szUserFile);
        dwData = ARRAYSIZE(lpnoi->szMenuText);
        
        if (RegEnumValue(*phKey, *piIndex, lpnoi->szUserFile, &dwSize, NULL,
            &dwType, (LPBYTE)lpnoi->szMenuText, &dwData) == ERROR_SUCCESS)
        {
            (*piIndex)++;
            // if there's something more than the null..
            if (dwData <= 1)
            { 
                lstrcpy(lpnoi->szMenuText, PathFindFileName(lpnoi->szUserFile));
                PathRemoveExtension(lpnoi->szMenuText);
            }
            fRet = TRUE;
        }
        else
        {
            RegCloseKey(*phKey);
            *phKey = (HKEY)-1;
            fRet = FALSE;
        }
    }
    
    return fRet;
    
}

CNewMenu::CNewMenu() 
    :   _cRef(1),
        _hConsolidationEvent(INVALID_HANDLE_VALUE)
{
    DllAddRef();
    TraceMsg(TF_SHDLIFE, "ctor CNewMenu %x", this);
    ASSERT(_lpnoiLast == NULL);
}

CNewMenu::~CNewMenu()
{
    TraceMsg(TF_SHDLIFE, "dtor CNewMenu %x", this);
    int i;

    if (_hmenu)
    {
        for (i = GetMenuItemCount(_hmenu) - 1; i >= 0; i--) 
        {
            LPNEWOBJECTINFO lpNewObjInfo = GetItemData(_hmenu, i);
            if (lpNewObjInfo != NULL) 
                LocalFree(lpNewObjInfo);
            //
            // Since we own the sub menu items, delete them. 
            //  However, do not delete the top level menu
            //
            DeleteMenu(_hmenu, i, MF_BYPOSITION);
        }
    }
    
    ILFree(_pidlFolder);

    if (_pdtobj)
        _pdtobj->Release();

    //Safety Net: Release my site in case I manage to get 
    // Released without my site SetSite(NULL) first.
    if (_pShellView2)
        _pShellView2->Release();

    //  Wait for our consolidation thread to complete
    if (INVALID_HANDLE_VALUE != _hConsolidationEvent)
    {
        // Only put up the hourglass if we really need to wait
        if (WaitForSingleObject(_hConsolidationEvent, 0) == WAIT_TIMEOUT) {
            DECLAREWAITCURSOR;
            SetWaitCursor();
            WaitForSingleObject(_hConsolidationEvent, INFINITE);
            ResetWaitCursor();
        }
        CloseHandle(_hConsolidationEvent);
    }

    DllRelease();
}

HRESULT CNewMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    // aggregation checking is handled in class factory
    
    CNewMenu * pShellNew = new CNewMenu();
    if (pShellNew) 
    {
        HRESULT hres = pShellNew->QueryInterface(riid, ppvOut);
        pShellNew->Release();
        return hres;
    }
    
    return E_OUTOFMEMORY;
}

HRESULT CNewMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CNewMenu, IShellExtInit),                     // IID_IShellExtInit
        QITABENT(CNewMenu, IContextMenu3),                     // IID_IContextMenu3
        QITABENTMULTI(CNewMenu, IContextMenu2, IContextMenu3), // IID_IContextMenu2
        QITABENTMULTI(CNewMenu, IContextMenu, IContextMenu3),  // IID_IContextMenu
        QITABENT(CNewMenu, IObjectWithSite),                   // IID_IObjectWithSite
        { 0 }
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CNewMenu::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CNewMenu::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CNewMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    MENUITEMINFO mfi = {0};
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return NOERROR;
    if (uFlags & CMF_DVFILE)
        _bMenuBar = TRUE;
    else
        _bMenuBar = FALSE;
    
    ConsolidateMenuItems(FALSE);

    _idCmdFirst = idCmdFirst+2;
    TCHAR szNewMenu[80];
    LoadString(g_hinst, IDS_NEWMENU, szNewMenu, ARRAYSIZE(szNewMenu));

    // HACK: I assume that they are querying during a WM_INITMENUPOPUP or equivalent
    GetCursorPos(&_ptNewItem);
    
    _hmenu = CreatePopupMenu();
    mfi.cbSize = sizeof(MENUITEMINFO);
    mfi.fMask = MIIM_ID | MIIM_TYPE;
    mfi.wID = idCmdFirst+1;
    mfi.fType = MFT_STRING;
    mfi.dwTypeData = szNewMenu;
    
    InsertMenuItem(_hmenu, 0, TRUE, &mfi);
    
    ZeroMemory(&mfi, sizeof (mfi));
    mfi.cbSize = sizeof(MENUITEMINFO);
    mfi.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA;
    mfi.fType = MFT_STRING;
    mfi.wID = idCmdFirst;
    mfi.hSubMenu = _hmenu;
    mfi.dwTypeData = szNewMenu;
    mfi.dwItemData = 0;
    
    InsertMenuItem(hmenu, indexMenu, TRUE, &mfi);

    _hmenu = NULL;
    return ResultFromShort(_idCmdFirst - idCmdFirst + 1);
}

// This is almost the same as ILCreatePidlFromPath, but
// uses only the filename from the full path pszPath and
// the _pidlFolder to generate the pidl. This is used because
// when creating an item in Desktop\My Documents, it used to create a
// full pidl c:\documents and Settings\lamadio\My Documents\New folder
// instead of the pidl desktop\my documents\New Folder.
BOOL CNewMenu::GeneratePidlFromPath(LPTSTR pszPath, LPITEMIDLIST* ppidl)
{
    IShellFolder* psf;

    *ppidl = NULL;  // Out param

    if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, _pidlFolder, (LPVOID*)&psf)))
    {
        ULONG chEaten;
        LPITEMIDLIST pidlItem;

#ifdef UNICODE
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, PathFindFileName(pszPath), &chEaten, &pidlItem, NULL)))
#else
        WCHAR wszPath[MAX_PATH];
        SHTCharToUnicode(PathFindFileName(pszPath), wszPath, ARRAYSIZE(wszPath));
        if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, wszPath, &chEaten, &pidlItem, NULL)))
#endif
        {
            *ppidl = ILCombine(_pidlFolder, pidlItem);
            ILFree(pidlItem);
        }

        psf->Release();
    }

    return BOOLFROMPTR(*ppidl);
}

HRESULT CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szFileSpec[MAX_PATH+80];   // Add some slop incase we overflow
    TCHAR szTemp[MAX_PATH+80];       // Add some slop incase we overflow
    NEWFILEINFO nfi;
    HRESULT hr = E_FAIL;
    DWORD dwFlags;
    BOOL bLFN;
    DWORD dwErrorMode;


    // Check if context menu is invoked with verbs instead of command id
    if (IS_INTRESOURCE(pici->lpVerb))
    {
        if (_lpnoiLast)
            dwFlags = _lpnoiLast->dwFlags;
        else if (pici->lpVerb == MAKEINTRESOURCEA(NEWITEM_FOLDER))
            dwFlags = NEWTYPE_FOLDER;
        else if (pici->lpVerb == MAKEINTRESOURCEA(NEWITEM_LINK))
            dwFlags = NEWTYPE_LINK;
        else
            return E_FAIL;
    }
    else
    {
        // We are invoked with a command string
        if (!lstrcmpiA("NewFolder", pici->lpVerb))
            dwFlags = NEWTYPE_FOLDER;
        else if (!lstrcmpiA("link", pici->lpVerb))
            dwFlags = NEWTYPE_LINK;
        else
            return E_FAIL;
    }

    dwErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    nfi.lpData = NULL;
    nfi.hkeyNew = NULL;

    //See if the pidl is folder shortcut and if so get the target path.
    SHGetTargetFolderPath(_pidlFolder, szPath, ARRAYSIZE(szPath));

    bLFN = IsLFNDrive(szPath);

    switch (dwFlags)
    {
    case NEWTYPE_FOLDER:
        LoadString(g_hinst, bLFN ? IDS_FOLDERLONGPLATE : IDS_FOLDERTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
        break;

    case NEWTYPE_LINK:
        LoadString(g_hinst, bLFN ? IDS_NEWLINKTEMPLATE : IDS_NEWLINKTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
        break;

    default:
        if (bLFN)
            LoadString(g_hinst, IDS_NEWFILEPREFIX, szFileSpec, ARRAYSIZE(szFileSpec));
        else
            szFileSpec[0] = 0;

        //
        // If we are running on a mirrored BiDi localized system,
        // then flip the order of concatenation so that the
        // string is read properly for Arabic. [samera]
        //
        if (IS_BIDI_LOCALIZED_SYSTEM())
        {
            lstrcpy(szTemp, szFileSpec);
            wsprintf(szFileSpec, TEXT("%s %s"), _lpnoiLast->szMenuText, szTemp);
        }
        else
        {
            lstrcat(szFileSpec, _lpnoiLast->szMenuText);
        }
        SHStripMneumonic(szFileSpec);

        if (!(dwFlags & SNCF_NOEXT))
            lstrcat(szFileSpec, _lpnoiLast->szExt);
        break;
    }

    PathCleanupSpec(szPath, szFileSpec);
    
    if (PathYetAnotherMakeUniqueName(szPath, szPath, szFileSpec, szFileSpec))
    {
        BOOL fCreateDir = FALSE;

        switch (dwFlags)
        {
        case NEWTYPE_FOLDER:
            {
                int err = SHCreateDirectoryEx(pici->hwnd, szPath, NULL);
                fCreateDir = TRUE;
                hr = HRESULT_FROM_WIN32(err);
            }
            break;

        case NEWTYPE_LINK:
            // Lookup Command in Registry under key HKCR/.lnk/ShellNew/Command
            if (CreateWriteCloseFile(pici->hwnd, szPath, NULL, 0))
            {
                hr = NOERROR;
                TCHAR szCommand[MAX_PATH];
                DWORD dwLength = ARRAYSIZE(szCommand);
                if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, 
                    TEXT(".lnk\\ShellNew"), TEXT("Command"), NULL, szCommand, &dwLength))
                {
                    hr = RunCommand(pici->hwnd, szPath, szCommand);
                }
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED); // above posts UI
            break;

        default:
            if (GetNewFileInfoForExtension(_lpnoiLast, &nfi, NULL, NULL))
            {
                switch (nfi.type) 
                {
                case NEWTYPE_FILE:
                    hr = CopyTemplate(szPath, &nfi);
                    if (SUCCEEDED(hr) || (
                        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr) &&
                        (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr)))
                        break;

                    // Template file not found.
                    // Fall through to create an empty file...
                    // ERROR_PATH_NOT_FOUND will occur when an explicit path
                    // is specified for a template, but that path doesn't exist

                case NEWTYPE_NULL:
                case NEWTYPE_DATA:
                    if (CreateWriteCloseFile(pici->hwnd, szPath, NULL, 0))
                        hr = NOERROR;
                    else
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);   // above displays UI
                    break;
        
                case NEWTYPE_COMMAND:
                    hr = RunCommand(pici->hwnd, szPath, (LPTSTR)nfi.lpData);
                    if (hr == S_FALSE)
                        hr = NOERROR;
                    break;

                default:
                    hr = E_FAIL;
                    break;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_BADKEY);
            }
            break;
        }
    
        LPITEMIDLIST pidlCreatedItem;
        if (SUCCEEDED(hr) &&
            GeneratePidlFromPath(szPath, &pidlCreatedItem))
        {
            SHChangeNotify(fCreateDir ? SHCNE_MKDIR : SHCNE_CREATE, 
                SHCNF_FLUSH, pidlCreatedItem, NULL);
            if (_pShellView2)
            {
                if (!_bMenuBar)
                {
                    DWORD dwFlagsSelFlags = SVSI_SELECT | SVSI_TRANSLATEPT;

                    if (!(dwFlags & NEWTYPE_LINK))
                        dwFlagsSelFlags |= SVSI_EDIT;

                    _pShellView2->SelectAndPositionItem(ILFindLastID(pidlCreatedItem), dwFlagsSelFlags, &_ptNewItem);
                }
                else
                    _pShellView2->SelectItem(ILFindLastID(pidlCreatedItem), SVSI_EDIT | SVSI_SELECT);
            }
            ILFree(pidlCreatedItem);
        } 
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    if (nfi.lpData)
        LocalFree((HLOCAL)nfi.lpData);
    
    if (nfi.hkeyNew)
        RegCloseKey(nfi.hkeyNew);

    if (FAILED(hr) && (HRESULT_CODE(hr) != ERROR_CANCELLED))
        SHSysErrorMessageBox(pici->hwnd, NULL, IDS_CANNOTCREATEFILE,
                HRESULT_CODE(hr), PathFindFileName(szPath), MB_OK | MB_ICONEXCLAMATION);

    SetErrorMode(dwErrorMode);

    return hr;
}

HRESULT CNewMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    switch (uType)
    {
    case GCS_HELPTEXT:
        if (idCmd < NEWITEM_MAX) {
            LoadString(g_hinst, (UINT)(IDS_NEWHELP_FIRST + idCmd), (LPTSTR)pszName, cchMax);
            return S_OK;
        }
        break;
#ifdef UNICODE
    case GCS_HELPTEXTA:
        if (idCmd < NEWITEM_MAX) {
            LoadStringA(g_hinst, (UINT)(IDS_NEWHELP_FIRST + idCmd), pszName, cchMax);
            return S_OK;
        }
        break;
#endif
    }

    return E_NOTIMPL;
}

//Defined in fsmenu.obj
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

HRESULT CNewMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

HRESULT CNewMenu::_MatchMenuItem(TCHAR ch, LRESULT* plRes)
{
    // Why?
    if (plRes == NULL)
        return S_FALSE;

    int iLastSelectedItem = -1;
    int iNextMatch = -1;
    BOOL fMoreThanOneMatch = FALSE;
    int c = GetMenuItemCount(_hmenu);

    // Pass 1: Locate the Selected Item
    for (int i = 0; i < c; i++) 
    {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
        {
            if (mii.fState & MFS_HILITE)
            {
                iLastSelectedItem = i;
                break;
            }
        }
    }

    // Pass 2: Starting from the selected item, locate the first item with the matching name.
    for (i = iLastSelectedItem + 1; i < c; i++) 
    {
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_STATE;
        if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
        {
            LPNEWOBJECTINFO lpnoi = (LPNEWOBJECTINFO)mii.dwItemData;
            if (lpnoi && _MenuCharMatch(lpnoi->szMenuText, ch, FALSE))
            {
                _lpnoiLast = lpnoi;
                
                if (iNextMatch != -1)
                {
                    fMoreThanOneMatch = TRUE;
                    break;                      // We found all the info we need
                }
                else
                {
                    iNextMatch = i;
                }
            }
        }
    }

    // Pass 3: If we did not find a match, or if there was only one match
    // Search from the first item, to the Selected Item
    if (iNextMatch == -1 || fMoreThanOneMatch == FALSE)
    {
        for (i = 0; i <= iLastSelectedItem; i++) 
        {
            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_STATE;
            if (GetMenuItemInfo(_hmenu, i, MF_BYPOSITION, &mii))
            {
                LPNEWOBJECTINFO lpnoi = (LPNEWOBJECTINFO)mii.dwItemData;
                if (lpnoi && _MenuCharMatch(lpnoi->szMenuText, ch, FALSE))
                {
                    _lpnoiLast = lpnoi;
                    if (iNextMatch != -1)
                    {
                        fMoreThanOneMatch = TRUE;
                        break;
                    }
                    else
                    {
                        iNextMatch = i;
                    }
                }
            }
        }
    }

    if (iNextMatch != -1)
    {
        *plRes = MAKELONG(iNextMatch, fMoreThanOneMatch? MNC_SELECT : MNC_EXECUTE);
    }
    else
    {
        *plRes = MAKELONG(0, MNC_IGNORE);
    }

    return S_OK;
}

HRESULT CNewMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        if (_hmenu == NULL)
        {
            _hmenu = (HMENU)wParam;
        }
        
        InitMenuPopup(_hmenu);
        break;
        
    case WM_DRAWITEM:
        DrawItem((DRAWITEMSTRUCT *)lParam);
        break;
        
    case WM_MEASUREITEM:
        MeasureItem((MEASUREITEMSTRUCT *)lParam);
        break;

    case WM_MENUCHAR:
        return _MatchMenuItem((TCHAR)LOWORD(wParam), lResult);
        
    }
    return NOERROR;
}

HRESULT CNewMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    ASSERT(_pidlFolder == NULL);
    _pidlFolder = ILClone(pidlFolder);
   
    if (_pdtobj)
        _pdtobj->Release();

    _pdtobj = pdtobj;

    if (_pdtobj)
        _pdtobj->AddRef();
    
    return NOERROR;
}


void CNewMenu::DrawItem(DRAWITEMSTRUCT *lpdi)
{
    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        DWORD dwRop;
        int x, y;
        SIZE sz;
        LPNEWOBJECTINFO lpnoi = (LPNEWOBJECTINFO)lpdi->itemData;
        
        // Draw the image (if there is one).
        
        GetTextExtentPoint(lpdi->hDC, lpnoi->szMenuText, lstrlen(lpnoi->szMenuText), &sz);
        
        if (lpdi->itemState & ODS_SELECTED)
        {
            SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            // REVIEW HACK - keep track of the last selected item.
            _lpnoiLast = lpnoi;
            dwRop = SRCSTENCIL;
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            dwRop = SRCAND;
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_MENU));
        }
        
        RECT rc = lpdi->rcItem;
        rc.left += +2*CXIMAGEGAP+g_cxSmIcon;
        
        
        DrawText(lpdi->hDC,lpnoi->szMenuText,lstrlen(lpnoi->szMenuText),
            &rc,DT_SINGLELINE|DT_VCENTER);
        if (lpnoi->iImage != -1)
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
            ImageList_Draw(g_himlIconsSmall, lpnoi->iImage, lpdi->hDC, x, y, ILD_TRANSPARENT);
        } 
        else 
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
        }
    }
}

LRESULT CNewMenu::MeasureItem(MEASUREITEMSTRUCT *lpmi)
{
    LRESULT lres = FALSE;
    LPNEWOBJECTINFO lpnoi = (LPNEWOBJECTINFO)lpmi->itemData;
    if (lpnoi)
    {
        // Get the rough height of an item so we can work out when to break the
        // menu. User should really do this for us but that would be useful.
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            // REVIEW cache out the menu font?
            NONCLIENTMETRICS ncm;
            ncm.cbSize = SIZEOF(NONCLIENTMETRICS);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
            {
                HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
                if (hfont)
                {
                    SIZE sz;
                    HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                    GetTextExtentPoint(hdc, lpnoi->szMenuText, lstrlen(lpnoi->szMenuText), &sz);
                    lpmi->itemHeight = max (g_cySmIcon+CXIMAGEGAP/2, ncm.iMenuHeight);
                    lpmi->itemWidth = g_cxSmIcon + 2*CXIMAGEGAP + sz.cx;
                    //lpmi->itemWidth = 2*CXIMAGEGAP + sz.cx;
                    SelectObject(hdc, hfontOld);
                    DeleteObject(hfont);
                    lres = TRUE;
                }
            }
            ReleaseDC(NULL, hdc);
        }
    }
    else
    {
        TraceMsg(TF_SHDLIFE, "fm_mi: Filemenu is invalid.");
    }
    
    return lres;
}

BOOL GetClassDisplayName(LPTSTR pszClass,LPTSTR pszDisplayName,DWORD cchDisplayName)
{
    DWORD cch;

    return (SUCCEEDED(AssocQueryString(0, ASSOCSTR_COMMAND, pszClass, TEXT("open"), NULL, &cch)) 
    && SUCCEEDED(AssocQueryString(0, ASSOCSTR_FRIENDLYDOCNAME, pszClass, NULL, pszDisplayName, &cchDisplayName)));
}

//-------------------------------------------------------------------------//
//  New Menu item consolidation worker task
class CNewMenuConsolidator : public CRunnableTask
//-------------------------------------------------------------------------//
{
public:
    virtual STDMETHODIMP RunInitRT(void);

private:
    CNewMenuConsolidator(HANDLE hCompletionEvent)
        :   CRunnableTask(RTF_DEFAULT), 
            _hCompletionEvent(hCompletionEvent) {
        DllAddRef();
    }
    ~CNewMenuConsolidator() {
        DllRelease();
    }

    HANDLE _hCompletionEvent;
    static const GUID _taskid;
    friend class CNewMenu;     // I hate friends
};

const GUID CNewMenuConsolidator::_taskid = 
    { 0xf87a1f28, 0xc7f, 0x11d2, { 0xbe, 0x1d, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };
//-------------------------------------------------------------------------//

#define REGSTR_SESSION_SHELLNEW STRREG_DISCARDABLE STRREG_POSTSETUP TEXT("\\ShellNew")
#define REGVAL_SESSION_SHELLNEW_TIMESTAMP TEXT("~reserved~")
#define REGVAL_SESSION_SHELLNEW_LANG TEXT("Language")
#define SHELLNEW_CONSOLIDATION_EVENT TEXT("ShellNewConsolidation")

#define SHELLNEW_CACHE_CURRENTVERSION  MAKELONG(1, 1)
             
//  Constructs a current New submenu cache stamp.
void CNewMenu_MakeCacheStamp(SHELLNEW_CACHE_STAMP* pStamp)
{
    pStamp->cbStruct = sizeof(*pStamp);
    pStamp->ver = SHELLNEW_CACHE_CURRENTVERSION;
    GetLocalTime(&pStamp->lastupdate);
}

//   Determines whether the New submenu cache needs to be rebuilt.
BOOL CNewMenu_ShouldUpdateCache(SHELLNEW_CACHE_STAMP* pStamp)
{
    //  Correct version?
    return !(sizeof(*pStamp) == pStamp->cbStruct &&
              SHELLNEW_CACHE_CURRENTVERSION == pStamp->ver);
}

//  Gathers up shellnew entries from HKCR into a distinct registry location 
//  for faster enumeration of the New submenu items.
//
//  We'll do a first time cache initialization only if we have to before showing
//  the menu, but will always rebuild the cache following display of the menu.
HRESULT CNewMenu::ConsolidateMenuItems(BOOL bForce)
{
    HKEY          hkeyShellNew = NULL;
    BOOL          bUpdate = TRUE;   // unless we discover otherwise
    HRESULT       hr = S_OK;

    
    if (INVALID_HANDLE_VALUE == _hConsolidationEvent)
    {
        if (INVALID_HANDLE_VALUE == (_hConsolidationEvent = 
                CreateEvent(NULL, TRUE /*manual reset*/, TRUE /*signaled*/, 
                             SHELLNEW_CONSOLIDATION_EVENT)))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ASSERT(FALSE);
            return hr;
        }
    }
    
    //  begin synchronize
    WaitForConsolidation(TRUE);
    
    //  If we're not being told to unconditionally update the cache and
    //  we validate that we've already established one, then we get out of doing any
    //  work.
    if (!bForce &&
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW,
                                       0L, KEY_ALL_ACCESS, &hkeyShellNew))
    {
        SHELLNEW_CACHE_STAMP stamp;
        ULONG cbVal = sizeof(stamp);
        if (ERROR_SUCCESS == SHQueryValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_TIMESTAMP, NULL,
                                              NULL, (LPBYTE)&stamp, &cbVal) &&
            sizeof(stamp) == cbVal)
        {
            bUpdate = CNewMenu_ShouldUpdateCache(&stamp);
        }

#ifdef WINNT
        LCID lcid;
        ULONG cblcid = sizeof(lcid);

        if (!bUpdate &&
            ERROR_SUCCESS == SHQueryValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_LANG, NULL,
                                              NULL, (LPBYTE)&lcid, &cblcid) &&
            sizeof(lcid) == cblcid)
        {
            bUpdate = (GetSystemDefaultUILanguage() != lcid); // if the languages are different, then update
        }
#endif
        RegCloseKey(hkeyShellNew);
    }
    
    if (bUpdate)
    {
        IShellTaskScheduler* pScheduler;
        //  Queue our task.
        IRunnableTask*  pTask;

        if (SUCCEEDED((hr = CoCreateInstance(CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC, 
                                            IID_IShellTaskScheduler, 
                                            (void **)&pScheduler))))
        {
            if (NULL != (pTask = (IRunnableTask*)new CNewMenuConsolidator(_hConsolidationEvent)))
            {
                hr = pScheduler->AddTask(pTask, CNewMenuConsolidator::_taskid, 
                                          (LPARAM)_hConsolidationEvent, 
                                          ITSAT_DEFAULT_PRIORITY);
                pTask->Release();
            }
            pScheduler->Release();

            if (SUCCEEDED(hr))
                return hr;     // the thread will signal us when he completes,
                        // so don't leave the synchronized block.
        }
    }

    // end synchronize
    WaitForConsolidation(FALSE); 
    
    return hr;
}

//  Waits on or enables access to the ShellNew cache.
//  Although it won't keep two CNewMenu instances from racing each other,
//  (which should never happen anyway), serialized access to the
//  cache will prevent a race between the primary and consolidation threads.
void CNewMenu::WaitForConsolidation(BOOL bWait /*TRUE*/)
{
    if (INVALID_HANDLE_VALUE != _hConsolidationEvent)
    {
        if (bWait)
        {
            WaitForSingleObject(_hConsolidationEvent, INFINITE);
            ResetEvent(_hConsolidationEvent);
        }
        else
            SetEvent(_hConsolidationEvent);
    }
}

//  Consolidation worker.
STDMETHODIMP CNewMenuConsolidator::RunInitRT()
{
    HKEY        hkeyShellNew = NULL;
    TCHAR       szExt[MAXEXTSIZE];
    ULONG       dwErr = NOERROR, 
                dwDisposition;
    int         i;
    
    //  Delete the existing cache; we'll build it from scratch each time.
    while (ERROR_SUCCESS == (dwErr = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW,
                                                     0L, NULL, 0, KEY_ALL_ACCESS, NULL, 
                                                     &hkeyShellNew, &dwDisposition)) &&
            REG_CREATED_NEW_KEY != dwDisposition)
    {
        //  Key already existed, so delete it, and loop to reopen.
        RegCloseKey(hkeyShellNew);
        SHDeleteKey(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW);
        hkeyShellNew = NULL;
    }

    if (ERROR_SUCCESS == dwErr)
    {
        // Enumerate each subkey of HKCR, looking for New menu items.
        for(i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szExt, ARRAYSIZE(szExt)) 
            == ERROR_SUCCESS; i++)
        {
            TCHAR szClass[CCH_KEYMAX];
            TCHAR szDisplayName[CCH_KEYMAX];
            LONG  cbVal = SIZEOF(szClass);
    
            // find .ext that have proper class descriptions with them.
            if ((szExt[0] == TEXT('.')) &&
                SHRegQueryValue(HKEY_CLASSES_ROOT, szExt, szClass, &cbVal) == ERROR_SUCCESS 
                && (cbVal > 0) 
                && GetClassDisplayName(szClass, szDisplayName, ARRAYSIZE(szDisplayName)))
            {
                NEWOBJECTINFO noi;
                HKEY          hkeyIterate = NULL;
                int           iIndex = 0;

                memset(&noi, 0, sizeof(noi));
                lstrcpy(noi.szExt, szExt);
                lstrcpy(noi.szClass, szClass);
                lstrcpy(noi.szMenuText, szDisplayName);
                noi.dwFlags = 0;
                noi.szUserFile[0] = 0;
                noi.iImage = -1;

                //  Retrieve all additional information for the key.
                while (GetNewFileInfoForExtension(&noi, NULL, &hkeyIterate, &iIndex)) 
                {
                    //  Stick it in the cache.
                    RegSetValueEx(hkeyShellNew, noi.szMenuText, NULL, REG_BINARY, 
                                   (LPBYTE)&noi, sizeof(noi));
                }
            }
        }
    
        //  Stamp the cache.
        SHELLNEW_CACHE_STAMP stamp;
        CNewMenu_MakeCacheStamp(&stamp);
        RegSetValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_TIMESTAMP,
                       NULL, REG_BINARY, (LPBYTE)&stamp, sizeof(stamp));
#ifdef WINNT
        LCID lcid = GetSystemDefaultUILanguage();

        RegSetValueEx(hkeyShellNew, REGVAL_SESSION_SHELLNEW_LANG,
                       NULL, REG_DWORD, (LPBYTE)&lcid, sizeof(lcid));
#endif
    }

    if (NULL != hkeyShellNew)
        RegCloseKey(hkeyShellNew);

    SetEvent(_hCompletionEvent);
    return HRESULT_FROM_WIN32(dwErr);
}

BOOL InsertNewMenuItem(HMENU hmenu, UINT idCmd, LPNEWOBJECTINFO lpnoi)
{
    LPNEWOBJECTINFO pnoi = (LPNEWOBJECTINFO)LocalAlloc(LPTR, SIZEOF(NEWOBJECTINFO));

    if (pnoi)
    {
        *pnoi = *lpnoi;
        MENUITEMINFO mii    = {0};
        mii.cbSize        = sizeof(mii);
        mii.fMask         = MIIM_TYPE | MIIM_DATA | MIIM_ID;
        mii.fType         = MFT_OWNERDRAW;
        mii.fState        = MFS_ENABLED;
        mii.wID           = idCmd;
        mii.dwItemData    = (DWORD_PTR)pnoi;
        mii.dwTypeData    = (LPTSTR)pnoi;

        pnoi->msaaHack.dwSignature   = MSAAHACK;
        pnoi->msaaHack.dwStrLen      = lstrlen(pnoi->szMenuText);
        if (StrChr(pnoi->szMenuText, TEXT('&')) == NULL)
        {
            pnoi->chPrefix = TEXT('&');
            pnoi->msaaHack.pszString     = &pnoi->chPrefix;
            pnoi->msaaHack.dwStrLen      = lstrlen((LPTSTR)&pnoi->chPrefix);
        }
        else
        {
            pnoi->msaaHack.pszString     = pnoi->szMenuText;
            pnoi->msaaHack.dwStrLen      = lstrlen(pnoi->szMenuText);
        }
        
        
        if (!InsertMenuItem(hmenu, -1, TRUE, &mii))
        {
            LocalFree((void*)pnoi);
            return TRUE;
        }
    }

    return FALSE;
}

//  WM_INITMENUPOPUP handler
BOOL CNewMenu::InitMenuPopup(HMENU hmenu)
{
    UINT iStart = 3;
    NEWOBJECTINFO noi;
    if (GetItemData(hmenu, iStart))  //Position 0 is New Folder, 1 shortcut, 2 sep 
        return FALSE;                //already initialized. No need to do anything
    
    //Remove the place holder.
    DeleteMenu(hmenu,0,MF_BYPOSITION);
    
    //Insert New Folder menu item
    noi.szExt[0] = '\0';
    noi.szClass[0] = '\0';
    LoadString(g_hinst, IDS_NEWFOLDER, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
    noi.dwFlags = NEWTYPE_FOLDER;
    noi.szUserFile[0] = 0;
    noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_FOLDER, 0); //Shange to indicate Folder

    InsertNewMenuItem(hmenu, _idCmdFirst-NEWITEM_MAX+NEWITEM_FOLDER, &noi);
    
    //Insert New Shortcut menu item
    LoadString(g_hinst, IDS_NEWLINK, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
    noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"), II_LINK, 0); //Shange to indicate Link
    noi.dwFlags = NEWTYPE_LINK;

    InsertNewMenuItem(hmenu, _idCmdFirst-NEWITEM_MAX+NEWITEM_LINK, &noi);
    
    //Insert menu item separator
    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);

    // This may take a while, so put up the hourglass
    DECLAREWAITCURSOR;
    SetWaitCursor();

    //  Retrieve extension menu items from cache:

    //  begin synchronize
    WaitForConsolidation(TRUE);

    HKEY hkeyShellNew;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_SESSION_SHELLNEW, 
                                       0L, KEY_ALL_ACCESS, &hkeyShellNew))
    {
        TCHAR szVal[CCH_KEYMAX];
        ULONG cbVal = ARRAYSIZE(szVal);
        ULONG cbData = sizeof(noi);
        ULONG dwType = REG_BINARY;
        
        for (int i = 0, cnt = 0; 
             ERROR_SUCCESS == RegEnumValue(hkeyShellNew, i, szVal, &cbVal, 0L,
                                            &dwType, (LPBYTE)&noi, &cbData);
             i++)
        {
            if (lstrcmp(szVal, REGVAL_SESSION_SHELLNEW_TIMESTAMP) != 0 &&
                sizeof(noi) == cbData && 
                REG_BINARY == dwType)
            {
                SHFILEINFO sfi;
                _himlSystemImageList = (HIMAGELIST)SHGetFileInfo(noi.szExt, FILE_ATTRIBUTE_NORMAL,
                                                                 &sfi, sizeof(SHFILEINFO), 
                                                                 SHGFI_USEFILEATTRIBUTES | 
                                                                 SHGFI_SYSICONINDEX | 
                                                                 SHGFI_SMALLICON);
                if (_himlSystemImageList)
                {
                    //lpnoi->himlSmallIcons = sfi.hIcon;
                    noi.iImage = sfi.iIcon;
                }
                else
                {
                    //lpnoi->himlSmallIcons = INVALID_HANDLE_VALUE;
                    noi.iImage = -1;
                }
                
                if (InsertNewMenuItem(hmenu, _idCmdFirst, &noi))
                    cnt++;
            }
            cbVal = ARRAYSIZE(szVal);
            cbData = sizeof(noi);
            dwType = REG_BINARY;
        }

        RegCloseKey(hkeyShellNew);
    }

    //  end synchronize
    WaitForConsolidation(FALSE);

    //  consolidate menu items following display.
    ConsolidateMenuItems(TRUE);

    TraceMsg(TF_SHDLIFE, "sh TR - QueryContextMenu: dup removed (%x, %d)",
        hmenu, GetMenuItemCount(hmenu));

    ResetWaitCursor();

    return TRUE;
}

LPNEWOBJECTINFO CNewMenu::GetItemData(HMENU hmenu, UINT iItem)
{
    MENUITEMINFO mii;
    
    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_DATA | MIIM_STATE;
    mii.cch = 0;     // just in case...
    
    if (GetMenuItemInfo(hmenu, iItem, TRUE, &mii))
        return (LPNEWOBJECTINFO)mii.dwItemData;
    
    return NULL;
}

LPTSTR ProcessArgs(LPTSTR szArgs,...)
{
    LPTSTR szRet;
    va_list ArgList;
    va_start(ArgList,szArgs);
    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        szArgs, 0, 0, (LPTSTR)&szRet, 0, &ArgList))
    {
        TraceMsg(TF_SHDLIFE,"sh tr - ProcessArgs failed");
        return NULL;
    }
    va_end(ArgList);
    return szRet;
}


HRESULT CNewMenu::RunCommand(HWND hwnd, LPTSTR pszPath, LPTSTR pszRun)
{
    HRESULT hres;
    SHELLEXECUTEINFO ei = { 0 };
    TCHAR szCommand[MAX_PATH];
    TCHAR szRun[MAX_PATH];
    LPTSTR pszArgs;
    
//    lstrcpy(szCommand, pszRun);
    SHExpandEnvironmentStrings(pszRun,szCommand,MAX_PATH);
    lstrcpy(szRun,szCommand);
    PathRemoveArgs(szCommand);
    
    //
    //  Mondo hackitude-o-rama.
    //
    //  Win95, IE3, SDK:  %1 - filename
    //
    //  IE4:              %1 - hwnd, %2 = filename
    //
    //  So IE4 broken Win95 compat and broke compat with the SDK.
    //  For IE5 we restore compat with Win95 and the SDK, while
    //  still generating an IE4-style command if we detect that the
    //  registry key owner tested with IE4 rather than following the
    //  instructions in the SDK.
    //
    //  The algorithm is like this:
    //
    //  If we see a "%2", then use %1 - hwnd, %2 - filename
    //  Otherwise, use             %1 - filename, %2 - hwnd
    //

    LPTSTR ptszPercent2;

    pszArgs = PathGetArgs(szRun);
    ptszPercent2 = StrStr(pszArgs, TEXT("%2"));
    if (ptszPercent2 && ptszPercent2[2] != TEXT('!'))
    {
        // App wants %1 = hwnd and %2 = filename
        pszArgs = ProcessArgs(pszArgs, (DWORD_PTR)hwnd, pszPath);
    }
    else
    {
        // App wants %2 = hwnd and %1 = filename
        pszArgs = ProcessArgs(pszArgs, pszPath, (DWORD_PTR)hwnd);
    }

    
    if (pszArgs) 
    {
        HMONITOR hMon = MonitorFromPoint(_ptNewItem, MONITOR_DEFAULTTONEAREST);
        if (hMon)
        {
            ei.fMask |= SEE_MASK_HMONITOR;
            ei.hMonitor = (HANDLE)hMon;
        }
        ei.hwnd            = hwnd;
        ei.lpFile          = szCommand;
        ei.lpParameters    = pszArgs;
        ei.nShow           = SW_SHOWNORMAL;
        ei.cbSize          = sizeof(SHELLEXECUTEINFO);

        if (ShellExecuteEx(&ei)) 
        {
            // Return S_FALSE because ShellExecuteEx is not atomic
            hres = S_FALSE;
        }
        else
            hres = E_FAIL;
        
        LocalFree(pszArgs);
    } 
    else
        hres = E_OUTOFMEMORY;
    
    return hres;
}

HRESULT CNewMenu::CopyTemplate(LPCTSTR pszTarget, NEWFILEINFO *pnfi)
{
    TCHAR szSrcFolder[MAX_PATH], szSrc[MAX_PATH];

    szSrc[0] = 0;

    // failure here is OK, we will
    if (SHGetSpecialFolderPath(NULL, szSrcFolder, CSIDL_TEMPLATES, FALSE))
    {
        PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
        if (!PathFileExistsAndAttributes(szSrc, NULL))
        {
            szSrc[0] = 0;
        }
    }

    if (szSrc[0] == 0)
    {
        if (SHGetSpecialFolderPath(NULL, szSrcFolder, CSIDL_COMMON_TEMPLATES, FALSE))
        {
            PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
            if (!PathFileExistsAndAttributes(szSrc, NULL))
            {
                szSrc[0] = 0;
            }
        }
    }

    if (szSrc[0] == 0)
    {
        // work around CSIDL_TEMPLATES not being setup right or
        // templates that are left in the old %windir%\shellnew location

        GetWindowsDirectory(szSrcFolder, ARRAYSIZE(szSrcFolder));
        PathAppend(szSrcFolder, TEXT("ShellNew"));

        // note: if the file spec is fully qualified szSrcFolder is ignored
        PathCombine(szSrc, szSrcFolder, (LPTSTR)pnfi->lpData);
    }

    if (CopyFile(szSrc, pszTarget, FALSE))
    {
        TouchFile(pszTarget);
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszTarget, NULL);
        return S_OK;
    }

    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT CNewMenu::SetSite(IUnknown* pUnk)
{
    ATOMICRELEASE(_pShellView2);

    if (pUnk)
        return pUnk->QueryInterface(IID_IShellView2, (void**)&_pShellView2);

    return NOERROR;
}

HRESULT CNewMenu::GetSite(REFIID riid, void**ppvObj)
{
    if (_pShellView2)
        return _pShellView2->QueryInterface(riid, ppvObj);
    else
    {
        ASSERT(ppvObj != NULL);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}
