#include "stdafx.h"
#pragma hdrstop
//#include "clsobj.h"
//#include <fsmenu.h>
//#include "resource.h"
//#include <shlwapi.h>

#include <mluisupp.h>

#define COPYMOVETO_REGKEY   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer")
#define COPYMOVETO_SUBKEY   TEXT("CopyMoveTo")
#define COPYMOVETO_VALUE    TEXT("LastFolder")

class CCopyMoveToMenu : public IContextMenu3, IShellExtInit
{
public:
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
    
private:
    BOOL    m_bMoveTo;
    LONG    m_cRef;
    HMENU   m_hmenu;
    UINT    m_idCmdFirst;
    BOOL    m_bFirstTime;
    IDataObject *m_pdtobj;
    CCopyMoveToMenu(BOOL bMoveTo = FALSE);
    ~CCopyMoveToMenu();
    
    friend HRESULT CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
    friend HRESULT CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
};

CCopyMoveToMenu::CCopyMoveToMenu(BOOL bMoveTo) : m_cRef(1), m_bMoveTo(bMoveTo)
{
    DllAddRef();
}

CCopyMoveToMenu::~CCopyMoveToMenu()
{
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
    InterlockedDecrement(&m_cRef);
    if (m_cRef > 0)
    {
        return m_cRef;
    }

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
    MLLoadString(m_bMoveTo? IDS_CMTF_MOVETO: IDS_CMTF_COPYTO, szMenuItem, ARRAYSIZE(szMenuItem));

    InsertMenu(hmenu, indexMenu++, MF_BYPOSITION, idCmd++, szMenuItem);

    return ResultFromShort(idCmd-idCmdFirst);
}

int BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    switch (msg)
    {
        case BFFM_INITIALIZED:
            // we passed ppidl as lpData so pass on just pidl
            SendMessage(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)*((LPITEMIDLIST*)lpData));
            break;
    }

    return 0;
}

HRESULT CCopyMoveToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
    
    if (m_pdtobj)
    {
        HKEY         hkey    = NULL;
        IStream      *pstrm;
        LPITEMIDLIST pidlSelectedFolder = NULL;
        LPITEMIDLIST pidlFolder;
        TCHAR        szTitle[100];
        BROWSEINFO   bi =
        {
            pici->hwnd, 
            NULL, 
            NULL, 
            szTitle,
            BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE, 
            BrowseCallback,
            (LPARAM)&pidlSelectedFolder
        };

        MLLoadString(IDS_CMTF_DLG_TITLE, szTitle, ARRAYSIZE(szTitle));

        if (RegOpenKeyEx(HKEY_CURRENT_USER, COPYMOVETO_REGKEY, 0, KEY_READ | KEY_WRITE, &hkey) == ERROR_SUCCESS)
        {
            LARGE_INTEGER li = {0, 0};
            ULARGE_INTEGER uli;

            pstrm = OpenRegStream(hkey, COPYMOVETO_SUBKEY, COPYMOVETO_VALUE, STGM_READWRITE);
            ILLoadFromStream(pstrm, &pidlSelectedFolder);
            // rewind the stream to the beginning so that when we
            // add a new pidl it does not get appended to the first one
            pstrm->Seek(li, STREAM_SEEK_SET, &uli);
        }

        pidlFolder = SHBrowseForFolder(&bi);
        if (pidlFolder)
        {
            IShellFolder *psf;

            hres = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (LPVOID*)&psf);

            if (SUCCEEDED(hres))
            {
                IDropTarget *pdrop;

                hres = psf->CreateViewObject(pici->hwnd, IID_IDropTarget, (void**)&pdrop);
                if (SUCCEEDED(hres))
                {
                    DWORD grfKeyState;

                    if (m_bMoveTo)
                        grfKeyState = MK_SHIFT | MK_LBUTTON;
                    else
                        grfKeyState = MK_CONTROL | MK_LBUTTON;

                    hres = SHSimulateDrop(pdrop, m_pdtobj, grfKeyState, NULL, NULL);

                    // change send to message to copymoveto error message
                    if (hres == S_FALSE)
                        ShellMessageBox(MLGetHinst(), pici->hwnd, MAKEINTRESOURCE(IDS_CMTF_ERRORMSG),
                        MAKEINTRESOURCE(IDS_CMTF_DLG_TITLE), MB_OK|MB_ICONEXCLAMATION);
                    pdrop->Release();
                }
                psf->Release();
            }
            if (pstrm)
            {
                ILSaveToStream(pstrm, pidlFolder);
                pstrm->Release();
            }
            if (hkey)
            {
                RegCloseKey(hkey);
            }
            ILFree(pidlFolder);
        }
        else
            hres = E_FAIL;
    }
    else
        hres = E_INVALIDARG;

    return hres;
}

HRESULT CCopyMoveToMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
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
    if (m_pdtobj)
        m_pdtobj->Release();
    
    m_pdtobj = pdtobj;
    
    if (m_pdtobj)
        m_pdtobj->AddRef();
    
    return NOERROR;
}

