#include "stdafx.h"
#pragma hdrstop

#include "_security.h"
#include <urlmon.h>

#ifdef POSTSPLIT

#define COPYMOVETO_REGKEY   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define COPYMOVETO_SUBKEY   TEXT("CopyMoveTo")
#define COPYMOVETO_VALUE    TEXT("LastFolder")

class CCopyMoveToMenu   : public IContextMenu3
                        , public IShellExtInit
                        , public CObjectWithSite
{
public:
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
    
private:
    BOOL    m_bMoveTo;
    LONG    m_cRef;
    HMENU   m_hmenu;
    UINT    m_idCmdFirst;
    BOOL    m_bFirstTime;
    LPITEMIDLIST m_pidlSource;
    IDataObject * m_pdtobj;

    CCopyMoveToMenu(BOOL bMoveTo = FALSE);
    ~CCopyMoveToMenu();
    
    HRESULT _DoDragDrop(LPCMINVOKECOMMANDINFO pici, LPCITEMIDLIST pidlFolder);
    BOOL _DidZoneCheckPass(LPCITEMIDLIST pidlFolder);

    friend HRESULT CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
    friend HRESULT CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
};

CCopyMoveToMenu::CCopyMoveToMenu(BOOL bMoveTo) : m_cRef(1), m_bMoveTo(bMoveTo)
{
    DllAddRef();

    // Assert that the member variables are zero initialized during construction
    ASSERT(!m_pidlSource);
}

CCopyMoveToMenu::~CCopyMoveToMenu()
{
    Pidl_Set(&m_pidlSource, NULL);
    ATOMICRELEASE(m_pdtobj);

    DllRelease();
}

HRESULT CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    CCopyMoveToMenu *pcopyto = new CCopyMoveToMenu();
    if (pcopyto)
    {
        HRESULT hres = pcopyto->QueryInterface(riid, ppvOut);
        pcopyto->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    CCopyMoveToMenu *pmoveto = new CCopyMoveToMenu(TRUE);
    if (pmoveto)
    {
        HRESULT hres = pmoveto->QueryInterface(riid, ppvOut);
        pmoveto->Release();
        return hres;
    }

    *ppvOut = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CCopyMoveToMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown)       ||
        IsEqualIID(riid, IID_IContextMenu)   ||
        IsEqualIID(riid, IID_IContextMenu2)  ||
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

ULONG CCopyMoveToMenu::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CCopyMoveToMenu::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CCopyMoveToMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // if they want the default menu only (CMF_DEFAULTONLY) OR 
    // this is being called for a shortcut (CMF_VERBSONLY)
    // we don't want to be on the context menu
    
    if (uFlags & (CMF_DEFAULTONLY | CMF_VERBSONLY))
        return NOERROR;

    UINT  idCmd = idCmdFirst;
    TCHAR szMenuItem[80];

    m_idCmdFirst = idCmdFirst;
    LoadString(g_hinst, m_bMoveTo? IDS_CMTF_MOVETO: IDS_CMTF_COPYTO, szMenuItem, ARRAYSIZE(szMenuItem));

    InsertMenu(hmenu, indexMenu++, MF_BYPOSITION, idCmd++, szMenuItem);

    return ResultFromShort(idCmd-idCmdFirst);
}

struct BROWSEINFOINITSTRUCT
{
    LPITEMIDLIST *ppidl;
    BOOL          bMoveTo;
    IDataObject  *pdtobj;
};

int BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    int idResource = 0;

    switch (msg)
    {
    case BFFM_INITIALIZED:
        SendMessage(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)*(((BROWSEINFOINITSTRUCT *)lpData)->ppidl));
        break;

    case BFFM_VALIDATEFAILEDW:
        idResource = IDS_PathNotFoundW;
        // FALL THRU...
    case BFFM_VALIDATEFAILEDA:
        if (0 == idResource)    // Make sure we didn't come from BFFM_VALIDATEFAILEDW
            idResource = IDS_PathNotFoundA;

        ShellMessageBox(g_hinst, hwnd,
            MAKEINTRESOURCE(idResource),
            MAKEINTRESOURCE(IDS_CMTF_COPYORMOVE_DLG_TITLE),
            MB_OK|MB_ICONERROR, (LPVOID)lParam);
        return 1;   // 1:leave dialog up for another try...
        /*NOTREACHED*/

    case BFFM_SELCHANGED:
        if (lParam)
        {
            BROWSEINFOINITSTRUCT *pbiis = (BROWSEINFOINITSTRUCT *)lpData;

            if (pbiis)
            {
                IShellFolder *psf;
                BOOL bEnableOK = FALSE;

                if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, (LPITEMIDLIST)lParam, (LPVOID*)&psf)))
                {
                    IDropTarget *pdt;
                    
                    if (SUCCEEDED(psf->CreateViewObject(hwnd, IID_IDropTarget, (void **)&pdt)))
                    {
                        POINTL pt = {0, 0};
                        DWORD  dwEffect;
                        DWORD  grfKeyState;

                        if (pbiis->bMoveTo)
                        {
                            dwEffect = DROPEFFECT_MOVE;
                            grfKeyState = MK_SHIFT | MK_LBUTTON;
                        }
                        else
                        {
                            dwEffect = DROPEFFECT_COPY;
                            grfKeyState = MK_CONTROL | MK_LBUTTON;
                        }

                        if (SUCCEEDED(pdt->DragEnter(pbiis->pdtobj, grfKeyState, pt, &dwEffect)))
                        {
                            if (dwEffect)
                            {
                                bEnableOK = TRUE;
                            }
                            pdt->DragLeave();
                        }
                        pdt->Release();
                    }
                    psf->Release();
                }
                SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)bEnableOK);
            }
        }
        break;
    }

    return 0;
}


BOOL CCopyMoveToMenu::_DidZoneCheckPass(LPCITEMIDLIST pidlFolder)
{
    BOOL fPass = TRUE;
    IInternetSecurityMgrSite * pisms = NULL;

    // We plan on doing UI and we need to go modal during the UI.
    ASSERT(_punkSite && pidlFolder);
    if (_punkSite)
    {
        _punkSite->QueryInterface(IID_IInternetSecurityMgrSite, (void **) &pisms);
    }

    if (S_OK != ZoneCheckPidl(pidlFolder, URLACTION_SHELL_FILE_DOWNLOAD, (PUAF_FORCEUI_FOREGROUND | PUAF_WARN_IF_DENIED), pisms))
    {
        fPass = FALSE;
    }

    ATOMICRELEASE(pisms);
    return fPass;
}


HRESULT CCopyMoveToMenu::_DoDragDrop(LPCMINVOKECOMMANDINFO pici, LPCITEMIDLIST pidlFolder)
{
    IShellFolder *psf;
    HRESULT hres = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (LPVOID*)&psf);

    // This should always succeed because the caller (SHBrowseForFolder) should
    // have weeded out the non-folders.
    if (EVAL(SUCCEEDED(hres)))
    {
        IDropTarget *pdrop;

        hres = psf->CreateViewObject(pici->hwnd, IID_IDropTarget, (void**)&pdrop);
        if (SUCCEEDED(hres))    // Will fail for some targets. (Like Nethood->Entire Network)
        {
            DWORD grfKeyState;

            if (m_bMoveTo)
                grfKeyState = MK_SHIFT | MK_LBUTTON;
            else
                grfKeyState = MK_CONTROL | MK_LBUTTON;

            // May fail if items aren't compatible for drag/drop. (Nethood is one example)
            hres = SHSimulateDrop(pdrop, m_pdtobj, grfKeyState, NULL, NULL);
            pdrop->Release();
        }

        psf->Release();
    }

    if (hres != S_OK)
    {
        // Go modal during the UI.
        IUnknown_EnableModless(_punkSite, FALSE);
        ShellMessageBox(g_hinst, pici->hwnd, MAKEINTRESOURCE(IDS_CMTF_ERRORMSG),
                        MAKEINTRESOURCE(IDS_CABINET), MB_OK|MB_ICONEXCLAMATION);
        IUnknown_EnableModless(_punkSite, TRUE);
    }

    return hres;
}


HRESULT CCopyMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
    
    if (m_pdtobj)
    {
        HKEY         hkey    = NULL;
        IStream      *pstrm  = NULL;
        LPITEMIDLIST pidlSelectedFolder = NULL;
        LPITEMIDLIST pidlFolder = NULL;
        TCHAR        szTitle[100];
        UINT         id = m_bMoveTo ? IDS_CMTF_MOVE_DLG_TITLE : IDS_CMTF_COPY_DLG_TITLE;
        BROWSEINFOINITSTRUCT biis =
        {   // passing the address of pidl because it is not init-ed yet
            // but it will be before call to SHBrowseForFolder so save one assignment
            &pidlSelectedFolder,
            m_bMoveTo,
            m_pdtobj,
        };
        BROWSEINFO   bi =
        {
            pici->hwnd, 
            NULL, 
            NULL, 
            szTitle,
            BIF_EDITBOX | BIF_VALIDATE | BIF_USENEWUI, 
            BrowseCallback,
            (LPARAM)&biis
        };

        EVAL(LoadString(g_hinst, id, szTitle, ARRAYSIZE(szTitle)));

        if (RegOpenKeyEx(HKEY_CURRENT_USER, COPYMOVETO_REGKEY, 0, KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
        {
            pstrm = OpenRegStream(hkey, COPYMOVETO_SUBKEY, COPYMOVETO_VALUE, STGM_READWRITE);
            if (pstrm)  // OpenRegStream will fail if the reg key is empty.
                ILLoadFromStream(pstrm, &pidlSelectedFolder);
        }

        if (_DidZoneCheckPass(m_pidlSource))
        {
            // Go modal during the UI.
            IUnknown_EnableModless(_punkSite, FALSE);
            pidlFolder = SHBrowseForFolder(&bi);
            IUnknown_EnableModless(_punkSite, TRUE);
            if (pidlFolder)
            {
                hres = _DoDragDrop(pici, pidlFolder);
            }
            else
                hres = E_FAIL;
        }
        else
            hres = E_FAIL;

        if (pstrm)
        {
            if (S_OK == hres)
            {
                TCHAR szFolder[MAX_PATH];
                
                if (SUCCEEDED(SHGetNameAndFlags(pidlFolder, SHGDN_FORPARSING, szFolder, SIZECHARS(szFolder), NULL))
                    && !PathIsUNC(szFolder)
                    && !IsRemoteDrive(PathGetDriveNumber(szFolder)))
                {
                    ULARGE_INTEGER uli;

                    // rewind the stream to the beginning so that when we
                    // add a new pidl it does not get appended to the first one
                    pstrm->Seek(g_li0, STREAM_SEEK_SET, &uli);
                    ILSaveToStream(pstrm, pidlFolder);
                }
            }

            pstrm->Release();
        }

        if (hkey)
        {
            RegCloseKey(hkey);
        }

        ILFree(pidlFolder); // ILFree() works for NULL pidls.
        ILFree(pidlSelectedFolder); // ILFree() works for NULL pidls.
    }
    else
        hres = E_INVALIDARG;

    return hres;
}

HRESULT CCopyMoveToMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

HRESULT CCopyMoveToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

HRESULT CCopyMoveToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lres)
{
    switch(uMsg)
    {
        //case WM_INITMENUPOPUP:
        //    break;

        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
            
                if (pdi->CtlType == ODT_MENU && pdi->itemID == m_idCmdFirst) 
                {
                    FileMenu_DrawItem(NULL, pdi);
                }
            }
            break;
        case WM_MEASUREITEM:
            {
                MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
                if (pmi->CtlType == ODT_MENU && pmi->itemID == m_idCmdFirst) 
                {
                    FileMenu_MeasureItem(NULL, pmi);
                }
            }
            break;
    }
    return NOERROR;
}

HRESULT CCopyMoveToMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    HRESULT hres = S_OK;

    if (!pdtobj)
        return E_INVALIDARG;

    IUnknown_Set((IUnknown **) &m_pdtobj, (IUnknown *) pdtobj);
    ASSERT(m_pdtobj);

    // BUGBUG (jeffreys) pidlFolder is now NULL when pdtobj is non-NULL
    // See comments above the call to HDXA_AppendMenuItems2 in
    // defcm.cpp!CDefFolderMenu::QueryContextMenu.  Raid #232106
    if (!pidlFolder)
    {
        hres = PidlFromDataObject(m_pdtobj, &m_pidlSource);
    }
    else if (!Pidl_Set(&m_pidlSource, pidlFolder))
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

#endif //POSTSPLIT
