#include "priv.h"
#include "guids.h"
//#include "stdafx.h"
//#pragma hdrstop

#define IDC_TESTMEN 0
static const TCHAR c_szGUID[] = TEXT(".{0AFACED1-E828-11D1-9187-B532F1E9575D}");
//static const TCHAR c_szGUID[] = TEXT(".bla");

STDAPI CFShortcutMenu_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut, BOOL IsFS);

class CFShortcutMenu : public IShellExtInit, public IContextMenu
{
    UINT _cRef;
    TCHAR m_szFileName[MAX_PATH];//name of the file being selected | played with
    BOOL m_IsSF; //is a shortcut to a folder ... .
    BOOL m_IsFS; //is a foldershortcut
    // if it is neither of these it is just a folder or a shortcut of some other kind.
    
    HRESULT ConvertSFToFS();//shortcut to folder To Folder Shortcut.
    HRESULT ConvertFSToSF();//Folder Shortcut to Shortcut to folder.
    /*
    static HMENU LoadPopupMenu(UINT id, UINT uSubMenu);
    HRESULT InitMenu();
    
   
    HMENU m_hmItems;
    */
    //friend STDAPI CFShortcutMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    
    virtual ~CFShortcutMenu();
public:
    CFShortcutMenu(BOOL IsFS);
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
     
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
} ;

CFShortcutMenu::CFShortcutMenu(BOOL IsFS) : _cRef(1)
{   
    m_IsSF=0;
    m_IsFS=IsFS;//set the default to inactivity
    DllAddRef();
}

CFShortcutMenu::~CFShortcutMenu()
{
    DllRelease();

}


HRESULT CFShortcutMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        *ppvObj= SAFECAST(this, IContextMenu *);
    }
    else    
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG CFShortcutMenu::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CFShortcutMenu::Release()
{
    _cRef--;
    
    if (_cRef > 0)
        return _cRef;
    
    delete this;
    return 0;
}


STDMETHODIMP CFShortcutMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{   
    TCHAR szPathName[MAX_PATH];
    if (!pdtobj)
    {
        return(E_INVALIDARG);
    }
    
    FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;
    HRESULT hRes = pdtobj->GetData(&fmte, &medium);
    if (FAILED(hRes))
    {
        return(hRes);
    }

    if (DragQueryFile((HDROP)medium.hGlobal, (UINT)(-1), NULL,0)>0)
    {
        DragQueryFile((HDROP)medium.hGlobal, 0, m_szFileName, sizeof(m_szFileName));
        if (!m_IsFS)
        {
            IShellLink *psl;
            hRes=CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **) &psl);
            if (SUCCEEDED(hRes))
            {
                IPersistFile *ppf;
                hRes=psl->QueryInterface (IID_IPersistFile, (void **)&ppf);
                if (SUCCEEDED(hRes))
                {
                    WORD wsz [MAX_PATH];
                    MultiByteToWideChar(CP_ACP, 0, m_szFileName, -1, wsz, MAX_PATH); 
                            //not sure this is really necessary.
                    hRes=ppf->Load(wsz, STGM_READ);
                    if (SUCCEEDED(hRes))
                    {
                        hRes=psl->Resolve(NULL, SLR_NO_UI);
                        if (SUCCEEDED(hRes))
                        {
                            // BUGBUG (t-tmcel): I have NO IDEA wtf this is. its in the UI book. 6.18.98
                            WIN32_FIND_DATA wfd;
                            hRes = psl->GetPath(szPathName, MAX_PATH, (WIN32_FIND_DATA *) &wfd, SLGP_SHORTPATH);
                            if (SUCCEEDED(hRes))
                            {
                                m_IsSF = PathIsDirectory(szPathName);
                                hRes = S_OK;
                            }
                        }
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        }
    }
    else
    {
        hRes=E_FAIL;// no items to play with
    }

    return hRes;
}


STDMETHODIMP CFShortcutMenu::QueryContextMenu(
                                            HMENU hmenu,
                                            UINT indexMenu,
                                            UINT idCmdFirst,
                                            UINT idCmdLast,
                                            UINT uFlags)
{
    MENUITEMINFO mii;
    UINT idMax = idCmdFirst + IDC_TESTMEN;
    
    if (uFlags & CMF_DEFAULTONLY)//this menu only allows the defaults.
        return NOERROR;

    if (!(m_IsFS || m_IsSF))
        return NOERROR; //should not get a new menu item.
    
    HMENU hmenuSub = CreatePopupMenu();
    if (!hmenuSub)
        return E_OUTOFMEMORY;
    
    TCHAR szMenuTitle[30];
    
    if (m_IsFS)
        LoadString(g_hinst, IDS_FStoSF, szMenuTitle, 30);
    else
        LoadString(g_hinst, IDS_SFtoFS, szMenuTitle, 30);

    mii.dwTypeData = szMenuTitle;    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.wID = idCmdFirst + IDC_TESTMEN;//need a new const to throw here
    mii.fState = MFS_ENABLED;
    idMax = mii.wID + 1;
    
    if (!InsertMenuItem(hmenu, indexMenu, TRUE, &mii))
    {
        return E_FAIL;
    }
    
    return ResultFromShort(1);
}


HRESULT CFShortcutMenu::ConvertSFToFS()// shortcut to folder To Folder Shortcut .
{
    //assumes that the stored path is a .lnk file (a folder shortcut).
    //if all goes well, it should exit with a folder shortcut to a folder.lnk that has replaced the old.
    HRESULT hres = E_FAIL;
    TCHAR szName[MAX_PATH];


    StrCpyN(szName,m_szFileName, SIZECHARS(szName));
    PathRemoveExtension(szName);
    StrCat(szName, c_szGUID);

    if (CreateDirectory(szName, NULL))
    {
        StrCat(szName, TEXT("\\target.lnk"));
        CopyFileEx(m_szFileName, szName, NULL, NULL, 0, 0);
        if (DeleteFile(m_szFileName))
            hres = S_OK;
    }

    return hres;
}


HRESULT CFShortcutMenu::ConvertFSToSF()// Folder Shortcut To shortcut to folder .
{
    HRESULT hres = E_FAIL;
    //assumes that the stored path is a .GUID file (a folder shortcut).
    //if all goes well, it should exit with a new shortcut to a folder that has replaced the old.
    TCHAR szLinkName[MAX_PATH];
    TCHAR szName[MAX_PATH];

    StrCpyN(szName, m_szFileName, SIZECHARS(szLinkName));
    StrCpyN(szLinkName,m_szFileName, SIZECHARS(szLinkName));
    StrCat(szLinkName, TEXT("\\target.lnk"));
    PathRemoveExtension(szName);
    StrCat(szName, TEXT(".lnk"));

    CopyFileEx(szLinkName, szName, NULL, NULL, 0, 0);
    if (DeleteFile(szLinkName) && RemoveDirectory(m_szFileName))
        hres = S_OK;

    return hres;
}

STDMETHODIMP CFShortcutMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hres = E_FAIL;
    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        return(E_INVALIDARG);
    }
    
    if (IDC_TESTMEN == (UINT)LOWORD((DWORD)lpici->lpVerb))
    {
        if (m_IsFS)
        {
            hres = ConvertFSToSF();
        }
        else if (m_IsSF)
        {
            hres = ConvertSFToFS();
        }
    }

    if (SUCCEEDED(hres))
        return hres;

    TCHAR szBoxTitle[20];
    TCHAR szBoxText[40];
    LoadString(g_hinst, IDS_Error, szBoxTitle, 20);
     
    if (m_IsSF)
        LoadString(g_hinst, IDS_ProblemFS, szBoxText, 40);
    else
        LoadString(g_hinst, IDS_ProblemSF, szBoxText, 40);
    MessageBox(NULL, szBoxText, szBoxTitle, 0);
    return(E_FAIL);
}


STDMETHODIMP CFShortcutMenu::GetCommandString(UINT idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDAPI CFShortcutMenu_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut, BOOL IsFS)
{
    HRESULT hres;
    CFShortcutMenu *pdocp = new CFShortcutMenu(IsFS);
    if (pdocp)
    {
        hres = pdocp->QueryInterface(riid, pcpOut);
        pdocp->Release();
    }
    else
    {
        *pcpOut = NULL;
        hres = E_OUTOFMEMORY;
    }

    return hres;
}
