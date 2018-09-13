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
//#include <fsmenu.h>
//#include "resource.h"
//#include <shlwapi.h>
//#include "clsobj.h"

#include <mluisupp.h>

#define MAXEXTSIZE              (PATH_CCH_EXT+2)

#ifndef CMF_DVFILE
#define CMF_DVFILE       0x00010000     // "File" pulldown
#endif

#undef TF_SHDLIFE
#define TF_SHDLIFE 0


class CSendToMenu : public IContextMenu3, IShellExtInit
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
    
    UINT    _cRef;
    HMENU   _hmenu;
    UINT    _idCmdFirst;
    // UINT    _idCmdLast;
    BOOL    _bFirstTime;
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
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IContextMenu) || 
        IsEqualIID(riid, IID_IContextMenu2) || 
        IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppvObj = SAFECAST(this, IContextMenu3 *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG CSendToMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CSendToMenu::Release()
{
    _cRef--;
    
    if (_cRef > 0)
        return _cRef;
    
    delete this;
    return 0;
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
        
        MLLoadString(IDS_SENDLINKTO, szSendLinkTo, ARRAYSIZE(szSendLinkTo));
        MLLoadString(IDS_SENDPAGETO, szSendPageTo, ARRAYSIZE(szSendPageTo));
        
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
        
#ifdef NEW_FILE_MENU_STUFF
        FM_GetPidlFromMenuCommand(g_hmenuFileMenu, LOWORD(pici->lpVerb), &pidlFolder, &pidlItem);
#else
        FileMenu_GetLastSelectedItemPidls(_hmenu, &pidlFolder, &pidlItem);
#endif
        if (pidlFolder && pidlItem)
        {
            IShellFolder *psfDesktop;
            hres = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hres))
            {
                IShellFolder *psf;
                hres = psfDesktop->BindToObject(pidlFolder, NULL, IID_IShellFolder, (void**)&psf);
                if (SUCCEEDED(hres))
                {
                    IDropTarget *pdrop;
                    hres = psf->GetUIObjectOf(pici->hwnd, 1, (LPCITEMIDLIST *)&pidlItem, IID_IDropTarget, 0, (void**)&pdrop);
                    if (SUCCEEDED(hres))
                    {
                        DWORD grfKeyState;

                        if (GetAsyncKeyState(VK_SHIFT) < 0)
                            grfKeyState = MK_SHIFT | MK_LBUTTON;        // move
                        else if (GetAsyncKeyState(VK_CONTROL) < 0)
                            grfKeyState = MK_CONTROL | MK_LBUTTON;      // copy
                        else if (GetAsyncKeyState(VK_MENU) < 0)
                            grfKeyState = MK_ALT | MK_LBUTTON;          // link
                        else
                            grfKeyState = MK_LBUTTON;

                        hres = SHSimulateDrop(pdrop, _pdtobj, grfKeyState, NULL, NULL);

                        if (hres == S_FALSE)
                            ShellMessageBox(MLGetHinst(), pici->hwnd, MAKEINTRESOURCE(IDS_SENDTO_ERRORMSG),
                                            MAKEINTRESOURCE(IDS_CABINET), MB_OK|MB_ICONEXCLAMATION);
                        pdrop->Release();
                    }
                    psf->Release();
                }
                psfDesktop->Release();
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

HRESULT CSendToMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
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
                if(_hmenu == NULL)
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

BOOL _IsShortcut(LPCTSTR pszFile)
{
    SHFILEINFO sfi;
    return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES) && (sfi.dwAttributes & SFGAO_LINK);
}

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
    
    ULONG _cRef;
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

STDMETHODIMP CTargetMenu::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IContextMenu) || 
        IsEqualIID(riid, IID_IContextMenu2) || 
        IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppvObj = SAFECAST(this, IContextMenu3 *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CTargetMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

STDMETHODIMP_(ULONG) CTargetMenu::Release()
{
    _cRef--;
    
    if (_cRef > 0)
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
            _IsShortcut(szShortcut))
        {
            IShellLink *psl;
            
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
            if (SUCCEEDED(hres))
            {
                IPersistFile *ppf;
                hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
                if (SUCCEEDED(hres)) 
                {
                    WCHAR wszFile[MAX_PATH];
                    
                    StrToOleStrN(wszFile, ARRAYSIZE(wszFile), szShortcut, -1);
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
        // MLLoadString(IDS_OPENCONTAINER, szString, ARRAYSIZE(szString));
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
            // MLLoadString(IDS_TARGETMENU, szString, ARRAYSIZE(szString));
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
} ;


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
    
    LPVOID operator[](int i);
    
    BOOL Init(UINT uSize, UINT uJump);
    
    BOOL Add(LPVOID pv);
    
    void Sort(PFNDPACOMPARE pfnCompare, LPARAM lParam)
    {
        m_pfnCompare = pfnCompare;
        m_lParam = lParam;
        
        DPA_Sort(m_dpa, ArrayCompare, (LPARAM)this);
    }
    
private:
    static int CALLBACK ArrayCompare(LPVOID pv1, LPVOID pv2, LPARAM lParam);
    
    HDPA m_dpa;
    HDSA m_dsa;
    
    PFNDPACOMPARE m_pfnCompare;
    LPARAM LPVOID;
} ;


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


BOOL CVoidArray::Add(LPVOID pv)
{
    int iItem = DSA_InsertItem(m_dsa, DPA_LAST, pv);
    if (iItem < 0)
    {
        return(FALSE);
    }
    
    if (DPA_InsertPtr(m_dpa, DPA_LAST, (LPVOID)iItem) < 0)
    {
        DSA_DeleteItem(m_dsa, iItem);
        return(FALSE);
    }
    
    return(TRUE);
}


int CALLBACK CVoidArray::ArrayCompare(LPVOID pv1, LPVOID pv2, LPARAM lParam)
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
} ;


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
    BOOL Add(CContentItemData *pv) {return(CVoidArray::Add((LPVOID)pv));}
    
private:
    static int CALLBACK DefaultSort(LPVOID pv1, LPVOID pv2, LPARAM lParam);
    
    HRESULT GetShellFolder(IShellFolder **ppsf);
    BOOL IsLocal();
    
    LPCITEMIDLIST CContentItemDataArray;
} ;


CContentItemDataArray::~CContentItemDataArray()
{
    for (int i=0; ; ++i)
    {
        CContentItemData *pID = (*this)[i];
        if (!pID)
            break;
        pID->Free();
    }
}


int CALLBACK CContentItemDataArray::DefaultSort(LPVOID pv1, LPVOID pv2, LPARAM lParam)
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
        return(hRes);
    }
    CEnsureRelease erDesktop(psfDesktop);
    
    return(psfDesktop->BindToObject(m_pidlFolder, NULL, IID_IShellFolder, (LPVOID *)ppsf));
}


BOOL CContentItemDataArray::IsLocal()
{
    char szPath[MAX_PATH];
    
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
    
    for ( ; ; )
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
} ;

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
} ;


class CDCTemp
{
public:
    CDCTemp(HDC hDC) : m_hDC(hDC) {}
    ~CDCTemp() {if (m_hDC) DeleteDC(m_hDC);}
    
    operator HDC() {return(m_hDC);}
    
private:
    HDC m_hDC;
} ;


class CRefMenuFont
{
public:
    CRefMenuFont(CMenuDraw *pmd) {m_pmd = pmd->InitMenuFont() ? pmd : NULL;}
    ~CRefMenuFont() {if (m_pmd) m_pmd->ReleaseMenuFont();}
    
    operator BOOL() {return(m_pmd != NULL);}
    
private:
    CMenuDraw *m_pmd;
} ;


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
        return(TRUE);
    }
    
    return(FALSE);
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
} ;

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
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IContextMenu) || 
        IsEqualIID(riid, IID_IContextMenu2))
    {
        *ppvObj = SAFECAST(this, IContextMenu2 *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG CContentMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CContentMenu::Release()
{
    _cRef--;
    
    if (_cRef > 0)
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
    return(m_pidlFolder ? NOERROR : E_OUTOFMEMORY);
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
    MLLoadStringA(IDS_CONTENTSMENU, szTitle, sizeof(szTitle));
    
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
        ShellMessageBox(MLGetHinst(), lpici->hwnd, MAKEINTRESOURCE(IDS_PRESSMOD),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    case IDC_SHIFTMORE:
        ShellMessageBox(MLGetHinst(), lpici->hwnd, MAKEINTRESOURCE(IDS_SHIFTMORE),
            MAKEINTRESOURCE(IDS_THISDLL), MB_OK|MB_ICONINFORMATION);
        break;
        
    default:
        if (m_hmItems)
        {
            CContentItemInfo mii(MIIM_DATA);
            
            if (!mii.GetMenuItemInfo(m_hmItems, uID, FALSE))
            {
                return(E_INVALIDARG);
            }
            
            CContentItemData *pData = mii.GetItemData();
            if (!pData || !pData->m_pidl)
            {
                return(E_INVALIDARG);
            }
            
            LPITEMIDLIST pidlAbs = ILCombine(m_pidlFolder, pData->m_pidl);
            if (!pidlAbs)
            {
                return(E_OUTOFMEMORY);
            }
            
            SHELLEXECUTEINFO sei;
            
            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_INVOKEIDLIST;
            sei.lpVerb = NULL;
            sei.hwnd         = lpici->hwnd;
            sei.lpParameters = lpici->lpParameters;
            sei.lpDirectory  = lpici->lpDirectory;
            sei.nShow        = lpici->nShow;
            sei.lpIDList = (LPVOID)pidlAbs;
            
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
    {
        return(E_UNEXPECTED);
    }
    
    if (m_hmItems)
    {
        return(NOERROR);
    }
    
    if (m_pIDs)
    {
        return(E_UNEXPECTED);
    }
    
    m_pIDs = new CContentItemDataArray(m_pidlFolder);
    if (!m_pIDs)
    {
        return(E_OUTOFMEMORY);
    }
    
    HRESULT hRes = m_pIDs->Init();
    if (FAILED(hRes))
    {
        return(hRes);
    }
    
    BOOL bGotAll = (hRes == S_OK);
    
    m_hmItems = CreatePopupMenu();
    if (!m_hmItems)
    {
        return(E_OUTOFMEMORY);
    }
    
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
    
    for (int i=0; ; ++i)
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


typedef struct 
{
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
    LPVOID lpData;
    DWORD cbData;
    HKEY hkeyNew;
} NEWFILEINFO, *LPNEWFILEINFO;


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

class CNewMenu : public IContextMenu3, IShellExtInit,IObjectWithSite
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
    
    //IObjectWithSite
    STDMETHOD(SetSite)(IUnknown*);
    STDMETHOD(GetSite)(REFIID,void**);
    
    int    _cRef;
    HMENU   _hmenu;
    UINT    _idCmdFirst;
    HIMAGELIST _himlSystemImageList;
    // UINT    _idCmdLast;
    IDataObject *_pdtobj;
    IShellView2*    _pShellView2;
    LPCITEMIDLIST   _pidlFolder;
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
    HRESULT CopyTemplate(HWND hwnd, LPTSTR szPath, LPNEWFILEINFO lpnfi);
};

extern "C" void DisplaySystemError(HWND hwnd,DWORD dwError, ...)
{
    LPTSTR szMessage;
    va_list ArgList;

    va_start(ArgList, dwError);
    DWORD dw = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,dwError,GetSystemDefaultLangID(),(LPTSTR)&szMessage,0,&ArgList);
    va_end(ArgList);
    if (dw)
    {
        ShellMessageBox(MLGetHinst(),hwnd,szMessage, MAKEINTRESOURCE(IDS_NEWFILE_ERROR_TITLE),
            (MB_OK | MB_ICONEXCLAMATION),NULL);
        LocalFree(szMessage);
    }
}

void GetConfigFlags(HKEY hkey, DWORD * pdwFlags)
{
    TCHAR szTemp[MAX_PATH];
    DWORD cbData = ARRAYSIZE(szTemp);
    
    *pdwFlags = SNCF_DEFAULT;
    
    if (RegQueryValueEx(hkey, TEXT("NoExtension"), 0, NULL, (BYTE *)szTemp, &cbData) == ERROR_SUCCESS) 
    {
        *pdwFlags |= SNCF_NOEXT;
    }
}

BOOL GetNewFileInfoForKey(HKEY hkeyExt, LPNEWFILEINFO lpnfi, DWORD * pdwFlags)
{
    BOOL fRet = FALSE;
    HKEY hKey; // this gets the \\.ext\progid  key
    HKEY hkeyNew;
    TCHAR szProgID[80];
    LONG lSize = SIZEOF(szProgID);
    
    // open the Newcommand
    if (RegQueryValue(hkeyExt, NULL,  szProgID, &lSize) != ERROR_SUCCESS) {
        return FALSE;
    }
    
    if (RegOpenKey(hkeyExt, szProgID, &hKey) != ERROR_SUCCESS) {
        hKey = hkeyExt;
    }
    
    if (RegOpenKey(hKey, TEXT("ShellNew"), &hkeyNew) == ERROR_SUCCESS) {
        DWORD dwType;
        DWORD cbData;
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
        
        if (cbData = SIZEOF(szTemp), (RegQueryValueEx(hkeyNew, TEXT("NullFile"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS)) 
        {
            fRet = TRUE;
            if (lpnfi)
                lpnfi->type = NEWTYPE_NULL;
        } 
        else if (cbData = SIZEOF(szTemp), (RegQueryValueEx(hkeyNew, TEXT("FileName"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            fRet = TRUE;
            if (lpnfi) {
                lpnfi->type = NEWTYPE_FILE;
                lpnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the file easily
                ASSERT((LPTSTR*)lpnfi->lpData == NULL);
                lpnfi->lpData = StrDup(szTemp);
                
                hkeyNew = NULL;
            }
        } 
        else if (cbData = SIZEOF(szTemp), (RegQueryValueEx(hkeyNew, TEXT("command"), 0, &dwType, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) 
            && ((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ))) 
        {
            
            fRet = TRUE;
            if (lpnfi) {
                lpnfi->type = NEWTYPE_COMMAND;
                lpnfi->hkeyNew = hkeyNew; // store this away so we can find out which one held the command easily
                ASSERT((LPTSTR*)lpnfi->lpData == NULL);
                lpnfi->lpData = StrDup(szTemp);
                hkeyNew = NULL;
            }
        } 
        else if ((RegQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, NULL, &cbData) == ERROR_SUCCESS) && cbData) 
        {
            // yes!  the data for a new file is stored in the registry
            fRet = TRUE;
            // do they want the data?
            if (lpnfi)
            {
                lpnfi->type = NEWTYPE_DATA;
                lpnfi->cbData = cbData;
                lpnfi->lpData = (void*)LocalAlloc(LPTR, cbData);
#ifdef UNICODE
                if (lpnfi->lpData)
                {
                    if (dwType == REG_SZ)
                    {
                        
                        //
                        //  Get the Unicode data from the registry.
                        //
                        LPWSTR pszTemp = (LPWSTR)LocalAlloc(LPTR, cbData);
                        if (pszTemp)
                        {
                            RegQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (LPBYTE)pszTemp, &cbData);
                            
                            lpnfi->cbData = SHUnicodeToAnsi(pszTemp, (LPSTR)lpnfi->lpData, cbData);
                            if (lpnfi->cbData == 0)
                            {
                                LocalFree(lpnfi->lpData);
                                lpnfi->lpData = NULL;
                            }
                            
                            LocalFree(pszTemp);
                        }
                        else
                        {
                            LocalFree(lpnfi->lpData);
                            lpnfi->lpData = NULL;
                        }
                    }
                    else
                    {
                        RegQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (BYTE*)lpnfi->lpData, &cbData);
                    }
                }
#else
                if (lpnfi->lpData)
                {
                    RegQueryValueEx(hkeyNew, TEXT("Data"), 0, &dwType, (BYTE*)lpnfi->lpData, &cbData);
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

BOOL GetNewFileInfoForExtension(LPNEWOBJECTINFO lpnoi, LPNEWFILEINFO lpnfi, HKEY* phKey, LPINT piIndex)
{
    TCHAR szValue[80];
    LONG lSize = SIZEOF(szValue);
    HKEY hkeyNew;
    BOOL fRet = FALSE;;
    
    
    if (phKey && ((*phKey) == (HKEY)-1)) {
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
        if (RegQueryValue(HKEY_CLASSES_ROOT, szSubKey, szValue, &lSize) == ERROR_SUCCESS) {
            
            wsprintf(szSubKey,TEXT("CLSID\\%s"), szValue);
            lSize = SIZEOF(szValue);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, &hkeyNew) == ERROR_SUCCESS) {
                
                fRet = GetNewFileInfoForKey(hkeyNew, lpnfi, &lpnoi->dwFlags);
                RegCloseKey(hkeyNew);
            }
        }
        
        // otherwise check under the type extension... do the extension, not the type
        // so that multi-ext to 1 type will work right
        if (!fRet && (RegOpenKey(HKEY_CLASSES_ROOT, lpnoi->szExt, &hkeyNew) == ERROR_SUCCESS)) {
            fRet = GetNewFileInfoForKey(hkeyNew, lpnfi, &lpnoi->dwFlags);
            RegCloseKey(hkeyNew);
        }
        
        if (phKey) {
            // if we're iterating, then we've got to open the key now...
            wsprintf(szSubKey, TEXT("%s\\%s\\ShellNew\\FileName"), lpnoi->szExt, lpnoi->szClass);
            if (RegOpenKey(HKEY_CLASSES_ROOT, szSubKey, phKey) == ERROR_SUCCESS) {
                
                *piIndex = 0;
                
                // if we didn't find one of the default ones above, 
                // try it now
                // otherwise just return success or failure on fRet
                if (!fRet) {
                    
                    goto Iterate;
                } 
            } else {
                *phKey = (HKEY)-1;
            }
        }
    } else if (!phKey && lpnoi->szUserFile[0]) {
        
        // there's no key, so just return info about szUserFile
        lpnfi->type = NEWTYPE_FILE;
        lpnfi->lpData = StrDup(lpnoi->szUserFile);
        lpnfi->hkeyNew = NULL;
        
        fRet = TRUE;
    } else if (phKey) {
        DWORD dwSize;
        DWORD dwData;
        DWORD dwType;
        // we're iterating through...
        
Iterate:
        
        dwSize = ARRAYSIZE(lpnoi->szUserFile);
        dwData = ARRAYSIZE(lpnoi->szMenuText);
        
        if (RegEnumValue(*phKey, *piIndex, lpnoi->szUserFile, &dwSize, NULL,
            &dwType, (LPBYTE)lpnoi->szMenuText, &dwData) == ERROR_SUCCESS) {
            (*piIndex)++;
            // if there's something more than the null..
            if (dwData <= 1) { 
                lstrcpy(lpnoi->szMenuText, PathFindFileName(lpnoi->szUserFile));
                PathRemoveExtension(lpnoi->szMenuText);
            }
            fRet = TRUE;
        } else {
            RegCloseKey(*phKey);
            *phKey = (HKEY)-1;
            fRet = FALSE;
        }
    }
    
    return fRet;
    
}

HFILE WINAPI Win32_lcreat(LPCTSTR lpszFileName, int fnAttrib)
{
#ifdef UNICODE
    HFILE handle = (HFILE)CreateFile( lpszFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        fnAttrib & FILE_ATTRIBUTE_VALID_FLAGS,
        NULL);
#else
    HFILE handle = _lcreat(lpszFileName, fnAttrib);
#endif
    if (HFILE_ERROR != handle)
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, lpszFileName, NULL);
    
    return handle;
}

BOOL CreateWriteCloseFile(HWND hwnd, LPTSTR szFileName, LPVOID lpData, DWORD cbData)
{
    HFILE hfile = Win32_lcreat(szFileName, 0);
    if (hfile != HFILE_ERROR) 
    {
        if (cbData) 
        {
            _lwrite(hfile, (char*)lpData, cbData);
        }
        _lclose(hfile);
        return TRUE;
    } 
    else 
    {
        PathRemoveExtension(szFileName);
        
        //SHSysErrorMessageBox(hwnd, NULL, IDS_CANNOTCREATEFILE,
        //    GetLastError(), PathFindFileName(szFileName),
        //    MB_OK | MB_ICONEXCLAMATION);
        DisplaySystemError(NULL,GetLastError(), PathFindFileName(szFileName));
    }
    return FALSE;
}

CNewMenu::CNewMenu() : _cRef(1) 
{
    TraceMsg(TF_SHDLIFE, "ctor CNewMenu %x", this);
    ASSERT(_lpnoiLast == NULL);
    
}

CNewMenu::~CNewMenu()
{
    TraceMsg(TF_SHDLIFE, "dtor CNewMenu %x", this);
    int i;

    if (_hmenu)
    {
        for (i = GetMenuItemCount(_hmenu) - 1 ; i >= 0 ; i--) 
        {
            LPNEWOBJECTINFO lpNewObjInfo = GetItemData(_hmenu, i);
            if(lpNewObjInfo != NULL) 
                LocalFree(lpNewObjInfo);
            //
            // Since we own the sub menu items, delete them. 
            //  However, do not delete the top level menu
            //
            DeleteMenu(_hmenu, i, MF_BYPOSITION);
        }
    }
    
    if (_pdtobj)
        _pdtobj->Release();

    //Safety Net: Release my site in case I manage to get 
    // Released without my site SetSite(NULL) first.
    ATOMICRELEASE(_pShellView2);
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
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IContextMenu) || 
        IsEqualIID(riid, IID_IContextMenu2) ||  
        IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppvObj = SAFECAST(this, IContextMenu3 *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite *);
    }
    else 
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG CNewMenu::AddRef()
{
    _cRef++;
    TraceMsg(TF_SHDLIFE, "CNewMenu::AddRef = %x", _cRef);
    return _cRef;
}

ULONG CNewMenu::Release()
{
    _cRef--;
    TraceMsg(TF_SHDLIFE, "CNewMenu::Release = %x", _cRef);
    
    if (_cRef > 0)
        return _cRef;
    
    delete this;
    return 0;
}

HRESULT CNewMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    MENUITEMINFO mfi;
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return NOERROR;
    if(uFlags & CMF_DVFILE)
        _bMenuBar = TRUE;
    else
        _bMenuBar = FALSE;
    
    _idCmdFirst = idCmdFirst+2;
    TCHAR szNewMenu[80];
    MLLoadString(IDS_NEWMENU, szNewMenu, ARRAYSIZE(szNewMenu));
    
    
    //  HACK: I assume that they are querying doring a WM_INITMENUPOPUP or equivelant
    GetCursorPos(&_ptNewItem);
    
    
    _hmenu = CreatePopupMenu();
    mfi.cbSize = sizeof(MENUITEMINFO);
    mfi.fMask = MIIM_ID|MIIM_TYPE;
    mfi.wID = idCmdFirst+1;
    mfi.fType = MFT_STRING;
    mfi.dwTypeData = szNewMenu;
    
    InsertMenuItem(_hmenu,0,TRUE,&mfi);
    
    mfi.fMask = MIIM_ID|MIIM_SUBMENU|MIIM_TYPE|MIIM_DATA;
    mfi.fType = MFT_STRING;
    mfi.wID = idCmdFirst;
    mfi.hSubMenu = _hmenu;
    mfi.dwTypeData = szNewMenu;
    
    InsertMenuItem(hmenu,indexMenu,TRUE,&mfi);

    _hmenu = NULL;
    return ResultFromShort(_idCmdFirst - idCmdFirst + 1);
}

#define HACKERISH //When this is defined, the behavior looks nicer, 
                    //but the code sucks and is hard to understand.

HRESULT CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szFileSpec[MAX_PATH+80];   // Add some slop incase we overflow

    NEWFILEINFO nfi;
    DWORD dwError;
    HRESULT hres = E_FAIL;
    LPITEMIDLIST    pidlNewObj;
    
    nfi.lpData = NULL;
    nfi.hkeyNew = NULL;
    if (_lpnoiLast == NULL)
        return E_FAIL;
    
    SHGetPathFromIDList(_pidlFolder,szPath);
    
    
    if (IsLFNDrive(szPath)) 
    {
        switch(_lpnoiLast->dwFlags)
        {
        case NEWTYPE_FOLDER:
            MLLoadString(IDS_FOLDERLONGPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
            break;
        case NEWTYPE_LINK:
            MLLoadString(IDS_NEWLINKTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
            break;
        default:
            MLLoadString(IDS_NEWFILEPREFIX, szFileSpec, ARRAYSIZE(szFileSpec));
            lstrcat(szFileSpec, _lpnoiLast->szMenuText);
            SHStripMneumonic(szFileSpec);
        
            if ( !(_lpnoiLast->dwFlags & SNCF_NOEXT) )
                lstrcat(szFileSpec, _lpnoiLast->szExt);
            break;
        }
    } 
    else 
    {
        switch(_lpnoiLast->dwFlags)
        {
        case NEWTYPE_FOLDER:
            MLLoadString(IDS_FOLDERTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
            break;
        case NEWTYPE_LINK:
            MLLoadString(IDS_NEWLINKTEMPLATE, szFileSpec, ARRAYSIZE(szFileSpec));
            break;
        default:

            // BUGBUG: The following code should use MLLoadString() and 
            // SHConstructMessageString() to construct the message
            // We need to add a wrapper in shlwapi around _ConstructMessageString()
            // and clean up shell32 stuff by using this instead of 
            // ShellConstructMessageString()

            // If we are running on a mirrored BiDi localized system,
            // then flip the order of concatenation so that the
            // string is read properly for Arabic. [samera]
            //
            if (IS_BIDI_LOCALIZED_SYSTEM())
            {
                TCHAR szTemp[MAX_PATH+80];       // Add some slop incase we overflow
                szTemp[0] = 0;
                lstrcpy(szTemp, szFileSpec);
                wnsprintf(szFileSpec, ARRAYSIZE(szFileSpec), TEXT("%s %s"), _lpnoiLast->szMenuText, szTemp);
            }
            else
            {
                lstrcat(szFileSpec, _lpnoiLast->szMenuText);
            }
            SHStripMneumonic(szFileSpec);
        
            if ( !(_lpnoiLast->dwFlags & SNCF_NOEXT) )
                lstrcat(szFileSpec, _lpnoiLast->szExt);
            break;
        }
        
        PathCleanupSpec(szPath, szFileSpec);
    }
    
    if (!PathYetAnotherMakeUniqueName(szPath, szPath, szFileSpec, szFileSpec))
    {
        dwError = ERROR_FILENAME_EXCED_RANGE;
        goto Error;
    }
    
    
    switch(_lpnoiLast->dwFlags)
    {
    case NEWTYPE_FOLDER:
        {//Special Case: Handle Folder creation here.
            if (CreateDirectory(szPath,NULL))
            {
#ifdef HACKERISH
                SHChangeNotify(SHCNE_MKDIR,SHCNF_PATH,szPath,NULL);
#endif                
                hres = NOERROR;
            }
            else
                goto Error;
            break;
        }
    case NEWTYPE_LINK:
        {//Special Case: Lookup Command in Registry under key
            //HKCR/.lnk/ShellNew/Command
            TCHAR szCommand[MAX_PATH];
            DWORD dwLength = ARRAYSIZE(szCommand);
            DWORD dwType;
            CreateWriteCloseFile(pici->hwnd,szPath,NULL,0);
            if(ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT,TEXT(".lnk\\ShellNew"),
                TEXT("Command"),&dwType,szCommand,&dwLength))
            {
                hres = RunCommand(pici->hwnd,szPath,szCommand);
            }
            break;
        }
    default:
        break;
    }
    
    if (FAILED(hres) && !GetNewFileInfoForExtension(_lpnoiLast, &nfi, NULL, NULL))
    {
        dwError = ERROR_BADKEY;
        goto Error;
    }
    
    if(FAILED(hres))
    {
        switch (nfi.type) 
        {
        case NEWTYPE_NULL:
            if (!CreateWriteCloseFile(pici->hwnd, szPath, NULL, 0)) 
            {
                // do some sort of error
                hres = E_FAIL;
            }
            else
                hres = NOERROR;
            break;
            
        case NEWTYPE_DATA:
            if (!CreateWriteCloseFile(pici->hwnd, szPath, nfi.lpData, nfi.cbData))
                hres = E_FAIL;
            else
                hres = NOERROR;
            break;
            
        case NEWTYPE_FILE:
            hres = CopyTemplate(pici->hwnd, szPath, &nfi);
            if (hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                dwError = ERROR_FILE_NOT_FOUND;
                goto Error;
            }
            break;
            
        case NEWTYPE_COMMAND:
            hres = RunCommand(pici->hwnd, szPath, (LPTSTR)nfi.lpData);
            if(hres == S_FALSE)
                hres = NOERROR;
            break;
        default:
            hres = E_FAIL;
            break;
        }
    }
    
    if (SUCCEEDED(hres))
    {
       
#if defined (HACKERISH) && 0 // see #if 0 below for explanation
        //"SHCNE_FREESPACE instead of SHCNE_CREATE?" You ask?
        // Let me explain: It prevents the icon from being displayed
        // a split second before it is moved to the new location...
        // HACK? Yep. But, it works. A better way may be to turn off 
        // listview Drawing.; Hence the HACKERISH define....
        //
#if 0
        // Unfortunately, the hack doesn't work on UNCs or places that
        // don't have drive letters because there is no freespace for
        // things without drive letters.  So this hack doesn't work after
        // all.  We'll live with the icon flicker.
#endif
        SHChangeNotify(SHCNE_FREESPACE,SHCNF_PATH | SHCNF_FLUSH,szPath,NULL);
#else
        SHChangeNotify(SHCNE_CREATE,SHCNF_PATH | SHCNF_FLUSH,szPath,NULL);
#endif
        SHChangeNotifyHandleEvents();
        if(_pShellView2)
        {
            pidlNewObj = ILCreateFromPath(szPath);
            if (pidlNewObj)
            {
#ifndef HACKERISH
                IShellFolderView* pFolderView = NULL;
                if(SUCCEEDED(_pShellView2->QueryInterface(IID_IShellFolderView,(void**)&pFolderView)))
                {
                    pFolderView->SetRedraw(FALSE);
                }
#endif
                if(!_bMenuBar)
                {
                    DWORD dwFlags = SVSI_SELECT | SVSI_TRANSLATEPT;
#ifdef HACKERISH
                    if(!(_lpnoiLast->dwFlags & NEWTYPE_LINK))
                        dwFlags |= SVSI_EDIT;
#endif
                    
                    _pShellView2->SelectAndPositionItem(ILFindLastID(pidlNewObj), dwFlags, &_ptNewItem);
                }
#ifdef HACKERISH
                else
                    _pShellView2->SelectItem(ILFindLastID(pidlNewObj),SVSI_EDIT|SVSI_SELECT);
#endif

#ifndef HACKERISH
                if(pFolderView)
                {
                    pFolderView->SetRedraw(TRUE);
                    pFolderView->Release();
                    pFolderView = NULL;
                }
                _pShellView2->SelectItem(ILFindLastID(pidlNewObj),SVSI_EDIT|SVSI_SELECT);
#endif
                ILFree(pidlNewObj);
            }
        }
    } 
    if (nfi.lpData)
        LocalFree((HLOCAL)nfi.lpData);
    
    if (nfi.hkeyNew)
        RegCloseKey(nfi.hkeyNew);
    return hres;
    
Error:
    
    DisplaySystemError(NULL,GetLastError(), PathFindFileName(szPath));
    return E_FAIL;
    
    
    
}

HRESULT CNewMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

//Defined in fsmenu.obj
BOOL _MenuCharMatch(LPCTSTR lpsz, TCHAR ch, BOOL fIgnoreAmpersand);

HRESULT CNewMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

HRESULT CNewMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        {
            if(_hmenu == NULL)
            {
                _hmenu = (HMENU)wParam;
            }
            
            InitMenuPopup(_hmenu);
        }
        break;
        
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
            
            DrawItem(pdi);
        }
        break;
        
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
            MeasureItem(pmi);
            
        }
        break;
    case WM_MENUCHAR:
        {
            int c = GetMenuItemCount(_hmenu);
            for (int i = 0; i < c; i++) 
            {
                LPNEWOBJECTINFO lpnoi = GetItemData(_hmenu, i);
                if(lpnoi && _MenuCharMatch(lpnoi->szMenuText,(TCHAR)LOWORD(wParam),FALSE))
                {
                    _lpnoiLast = lpnoi;
                    if(lResult) *lResult = MAKELONG(i,MNC_EXECUTE);
                    return S_OK;
                }
            }
            if(lResult) *lResult = MAKELONG(0,MNC_IGNORE);
            return S_FALSE;
            
        }
        
    }
    return NOERROR;
}

HRESULT CNewMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    if (_pdtobj)
        _pdtobj->Release();
    
    
    _pidlFolder = pidlFolder;
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
            ImageList_Draw(_himlSystemImageList, lpnoi->iImage, lpdi->hDC, x, y, ILD_TRANSPARENT);
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

BOOL GetClassDisplayName(LPTSTR szClass,LPTSTR szDisplayName,DWORD cchDisplayName)
{
    DWORD dwType;
    cchDisplayName *= sizeof(TCHAR);
    if(SHGetValue(HKEY_CLASSES_ROOT,szClass,TEXT(""), &dwType,
        (BYTE*)szDisplayName,&cchDisplayName) == ERROR_SUCCESS)
    {
        TCHAR szTemp[MAX_PATH+40]; //Saw this in shell32/fsassoc.h
        DWORD cbExe;
        wsprintf(szTemp,TEXT("%s\\%s"),szClass,TEXT("shell\\open\\command"));
        
        //I just want to see if there is an open command for this class.
        if(SHGetValue(HKEY_CLASSES_ROOT,szTemp,TEXT(""), 
            &dwType,NULL,&cbExe) == ERROR_SUCCESS)
        {
            if(szDisplayName[0] == TEXT('\0'))
                return FALSE;

            if(cbExe > 0)
                return TRUE;
        }
    }
    return FALSE;
}

BOOL CNewMenu::InitMenuPopup(HMENU hmenu)
{
    UINT iStart = 3;
    NEWOBJECTINFO noi;
    LPNEWOBJECTINFO lpnoi = GetItemData(hmenu, iStart); //Position 0 is New Folder, 1 shortcut
    TraceMsg(TF_SHDLIFE, "CNewMenu::InitMenuPopup");
    if (lpnoi)  // already initialized.
        return FALSE;
    
    //Remove the place holder.
    DeleteMenu(hmenu,0,MF_BYPOSITION);
    
    //Insert Special Owner Draw Menuitems
    noi.szExt[0] = '\0';
    noi.szClass[0] = '\0';
    MLLoadString(IDS_NEWFOLDER, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
    noi.dwFlags = NEWTYPE_FOLDER;
    noi.szUserFile[0] = 0;
    noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"),II_FOLDER,0); //Shange to indicate Folder
    lpnoi = (LPNEWOBJECTINFO)LocalAlloc(LPTR, SIZEOF(NEWOBJECTINFO));
    if (lpnoi)
    {
        *lpnoi = noi;
        if(!AppendMenu(hmenu, MF_OWNERDRAW, _idCmdFirst-2, (LPTSTR)lpnoi))
        {
            LocalFree((void*)lpnoi);
        }
    }
    
    MLLoadString(IDS_NEWLINK, noi.szMenuText, ARRAYSIZE(noi.szMenuText));
    noi.iImage = noi.iImage = Shell_GetCachedImageIndex(TEXT("shell32.dll"),II_LINK,0); //Shange to indicate Link
    noi.dwFlags = NEWTYPE_LINK;
    lpnoi = (LPNEWOBJECTINFO)LocalAlloc(LPTR, SIZEOF(NEWOBJECTINFO));
    if (lpnoi)
    {
        *lpnoi = noi;
        if(!AppendMenu(hmenu, MF_OWNERDRAW, _idCmdFirst-1, (LPTSTR)lpnoi))
        {
            LocalFree((void*)lpnoi);
        }
    }
    
    //Seperator
    AppendMenu(hmenu,MF_SEPARATOR,0,NULL);
    
    
    
    TCHAR szExt[MAXEXTSIZE];
    int i;
    
    for (i = 0; RegEnumKey(HKEY_CLASSES_ROOT, i, szExt, ARRAYSIZE(szExt)) 
        == ERROR_SUCCESS; i++)
    {
        TCHAR szClass[CCH_KEYMAX];
        TCHAR szDisplayName[CCH_KEYMAX];
        LONG lSize = SIZEOF(szClass);
        
        // find .ext that have proper class descriptions with them.
        if ((szExt[0] == TEXT('.')) &&
            RegQueryValue(HKEY_CLASSES_ROOT, szExt, szClass, &lSize) == ERROR_SUCCESS 
            && (lSize > 0) 
            && GetClassDisplayName(szClass, 
            szDisplayName, ARRAYSIZE(szDisplayName)))
        {
            HKEY hkeyIterate = NULL;
            int iIndex = 0;
            
            lstrcpy(noi.szExt, szExt);
            lstrcpy(noi.szClass, szClass);
            lstrcpy(noi.szMenuText, szDisplayName);
            noi.dwFlags = 0;
            noi.szUserFile[0] = 0;
            noi.iImage = -1;
            
            while (GetNewFileInfoForExtension(&noi, NULL, &hkeyIterate, &iIndex)) 
            {
                lpnoi = (LPNEWOBJECTINFO)
                    LocalAlloc(LPTR, SIZEOF(NEWOBJECTINFO));
                if (lpnoi)
                {
                    SHFILEINFO sfi;
                    *lpnoi = noi;
                    
                    if(0 != (_himlSystemImageList = (HIMAGELIST)SHGetFileInfo(szExt,FILE_ATTRIBUTE_NORMAL,
                        &sfi,sizeof(SHFILEINFO),SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON)))
                    {
                        //lpnoi->himlSmallIcons = sfi.hIcon;
                        lpnoi->iImage = sfi.iIcon;
                    }
                    else
                    {
                        //lpnoi->himlSmallIcons = INVALID_HANDLE_VALUE;
                        lpnoi->iImage = -1;
                    }
                    
                    if(!AppendMenu(hmenu, MF_OWNERDRAW, _idCmdFirst, (LPTSTR)lpnoi))
                    {
                        LocalFree((void*)lpnoi);
                    }
                }
            }
        }
    }
    TraceMsg(TF_SHDLIFE, "sh TR - QueryContextMenu: filed (%x, %d)",
        hmenu, GetMenuItemCount(hmenu));
    
    // remove dups.
    // need to call GetMenuItemCount each time because
    // we're removing things...
    for (i = iStart; i < GetMenuItemCount(hmenu); i++) 
    {
        int j;
        LPNEWOBJECTINFO lpnoi = GetItemData(hmenu, i);
        for (j = GetMenuItemCount(hmenu) - 1; j > i; j--) {
            LPNEWOBJECTINFO lpnoi2 = GetItemData(hmenu, j);
            if (!lstrcmpi(lpnoi->szMenuText, lpnoi2->szMenuText)) 
            {
                DeleteMenu(hmenu, j, MF_BYPOSITION);
                LocalFree(lpnoi2);
            }
        }
    }
    
    TraceMsg(TF_SHDLIFE, "sh TR - QueryContextMenu: dup removed (%x, %d)",
        hmenu, GetMenuItemCount(hmenu));
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
    if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
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
    ExpandEnvironmentStrings(pszRun,szCommand,MAX_PATH);
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
        pszArgs = ProcessArgs(pszArgs, (DWORD)hwnd, pszPath);
    }
    else
    {
        // App wants %2 = hwnd and %1 = filename
        pszArgs = ProcessArgs(pszArgs, pszPath, (DWORD)hwnd);
    }

    
    if (pszArgs) 
    {
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

HRESULT CNewMenu::CopyTemplate(HWND hwnd, LPTSTR szPath, LPNEWFILEINFO lpnfi)
{
    TCHAR szSrc[MAX_PATH + 1];
    TCHAR szFileName[MAX_PATH +1];
    // now do the actual restore.
    SHFILEOPSTRUCT sFileOp =
    {
        hwnd,
            FO_COPY,
            szSrc,
            szPath,
            FOF_NOCONFIRMATION | FOF_MULTIDESTFILES | FOF_SILENT,
    } ;
    
    lstrcpy(szFileName, (LPTSTR)lpnfi->lpData);
    if (PathIsFileSpec(szFileName)) {
        if (!SHGetSpecialFolderPath(NULL, szSrc, CSIDL_TEMPLATES, FALSE))
            return FALSE;
        
        PathAppend(szSrc, szFileName);
    } else {
        lstrcpy(szSrc, szFileName);
    }
    
    if (!PathFileExists(szSrc)) {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    
    szSrc[lstrlen(szSrc) + 1] = TEXT('\0'); // double null terminate;
    return ((SHFileOperation(&sFileOp) == 0) && 
        !sFileOp.fAnyOperationsAborted) ? S_OK : E_FAIL;
}

HRESULT CNewMenu::SetSite(IUnknown* pUnk)
{
    ATOMICRELEASE(_pShellView2);
    TraceMsg(TF_SHDLIFE, "CNewMenu::SetSite = 0x%x", pUnk);
    
    if(pUnk)
        return pUnk->QueryInterface(IID_IShellView2,(void**)&_pShellView2);

    return NOERROR;
}

HRESULT CNewMenu::GetSite(REFIID riid,void** ppvObj)
{
    if(_pShellView2)
        return _pShellView2->QueryInterface(riid,ppvObj);
    else
    {
        ASSERT(ppvObj != NULL);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}
