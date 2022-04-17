/*
 * PROJECT:     shell32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/shell32/CDefViewBckgrndMenu.cpp
 * PURPOSE:     background context menu of the CDefView
 * PROGRAMMERS: Giannis Adamopoulos
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

class CDefViewBckgrndMenu :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu3,
    public IObjectWithSite
{
    private:
        CComPtr<IUnknown> m_site;
        CComPtr<IShellFolder> m_psf;
        CComPtr<IContextMenu> m_folderCM;

        UINT m_idCmdFirst;
        UINT m_LastFolderCMId;

        BOOL _bIsDesktopBrowserMenu();
        BOOL _bCanPaste();
    public:
        CDefViewBckgrndMenu();
        ~CDefViewBckgrndMenu();
        HRESULT Initialize(IShellFolder* psf);

        // IContextMenu
        virtual HRESULT WINAPI QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        virtual HRESULT WINAPI InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
        virtual HRESULT WINAPI GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen);

        // IContextMenu2
        virtual HRESULT WINAPI HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

        // IContextMenu3
        virtual HRESULT WINAPI HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

        // IObjectWithSite
        virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
        virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

        BEGIN_COM_MAP(CDefViewBckgrndMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu2, IContextMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu3, IContextMenu3)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        END_COM_MAP()
};

CDefViewBckgrndMenu::CDefViewBckgrndMenu()
{
    m_idCmdFirst = 0;
    m_LastFolderCMId = 0;
}

CDefViewBckgrndMenu::~CDefViewBckgrndMenu()
{
}

BOOL CDefViewBckgrndMenu::_bIsDesktopBrowserMenu()
{
    if (!m_site)
        return FALSE;

    /* Get a pointer to the shell browser */
    CComPtr<IShellView> psv;
    HRESULT hr = IUnknown_QueryService(m_site, SID_IFolderView, IID_PPV_ARG(IShellView, &psv));
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    FOLDERSETTINGS FolderSettings;
    hr = psv->GetCurrentInfo(&FolderSettings);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    return ((FolderSettings.fFlags & FWF_DESKTOP) == FWF_DESKTOP);
}

BOOL CDefViewBckgrndMenu::_bCanPaste()
{
    /* If the folder doesn't have a drop target we can't paste */
    CComPtr<IDropTarget> pdt;
    HRESULT hr = m_psf->CreateViewObject(NULL, IID_PPV_ARG(IDropTarget, &pdt));
    if (FAILED(hr))
        return FALSE;

    /* We can only paste if CFSTR_SHELLIDLIST is present in the clipboard */
    CComPtr<IDataObject> pDataObj;
    hr = OleGetClipboard(&pDataObj);
    if (FAILED(hr))
        return FALSE;

    STGMEDIUM medium;
    FORMATETC formatetc;

    /* Set the FORMATETC structure*/
    InitFormatEtc(formatetc, RegisterClipboardFormatW(CFSTR_SHELLIDLIST), TYMED_HGLOBAL);
    hr = pDataObj->GetData(&formatetc, &medium);
    if (FAILED(hr))
        return FALSE;

    ReleaseStgMedium(&medium);
    return TRUE;
}

HRESULT
CDefViewBckgrndMenu::Initialize(IShellFolder* psf)
{
    m_psf = psf;

    /* Get the context menu of the folder. Do it here because someone may call
       InvokeCommand without calling QueryContextMenu. It is fine if this fails */
    m_psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &m_folderCM));

    return S_OK;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::SetSite(IUnknown *pUnkSite)
{
    m_site = pUnkSite;

    if(m_folderCM)
        IUnknown_SetSite(m_folderCM, pUnkSite);

    return S_OK;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_site)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT
WINAPI
CDefViewBckgrndMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr;
    HMENU hMenuPart;
    UINT cIds = 0;

    /* This is something the implementations of IContextMenu should never really do.
       However CDefViewBckgrndMenu is more or less an overengineering result, its code could really be part of the
       CDefView. Given this, I think that abusing the interface here is not that bad since only CDefView is the ony
       user of this class. Here we need to do two things to keep things as simple as possible.
       First we want the menu part added by the shell folder to be the first to add so as to make as few id translations
       as possible. Second, we want to add the default part of the background menu without shifted ids, so as
       to let the CDefView fill some parts like filling the arrange modes or checking the view mode. In order
       for that to work we need to save idCmdFirst because our caller will pass id offsets to InvokeCommand.
       This makes it impossible to concatenate the CDefViewBckgrndMenu with other menus since it abuses IContextMenu
       but as stated above, its sole user is CDefView and should really be that way. */
    m_idCmdFirst = idCmdFirst;

    /* Let the shell folder add any items it wants to add in the background context menu */
    if (m_folderCM)
    {
        hr = m_folderCM->QueryContextMenu(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
        if (SUCCEEDED(hr))
        {
            m_LastFolderCMId = LOWORD(hr);
            cIds = m_LastFolderCMId;
        }
        else
        {
            WARN("QueryContextMenu failed!\n");
        }
    }
    else
    {
        WARN("GetUIObjectOf didn't give any context menu!\n");
    }

    /* Load the default part of the background context menu */
    hMenuPart = LoadMenuW(shell32_hInstance, L"MENU_002");
    if (hMenuPart)
    {
        /* Don't show the view submenu for the desktop */
        if (_bIsDesktopBrowserMenu())
        {
            DeleteMenu(hMenuPart, FCIDM_SHVIEW_VIEW, MF_BYCOMMAND);
        }

        /* Disable the paste options if it is not possible */
        if (!_bCanPaste())
        {
            EnableMenuItem(hMenuPart, FCIDM_SHVIEW_INSERT, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenuPart, FCIDM_SHVIEW_INSERTLINK, MF_BYCOMMAND | MF_GRAYED);
        }

        /* merge general background context menu in */
        Shell_MergeMenus(hMenu, GetSubMenu(hMenuPart, 0), indexMenu, 0, idCmdLast, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS | MM_ADDSEPARATOR);
        DestroyMenu(hMenuPart);
    }
    else
    {
        ERR("Failed to load menu from resource!\n");
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cIds);
}

HRESULT
WINAPI
CDefViewBckgrndMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    UINT idCmd = LOWORD(lpcmi->lpVerb);

    if (HIWORD(lpcmi->lpVerb) && !strcmp(lpcmi->lpVerb, CMDSTR_VIEWLISTA))
    {
        idCmd = FCIDM_SHVIEW_LISTVIEW;
    }
    else if (HIWORD(lpcmi->lpVerb) && !strcmp(lpcmi->lpVerb, CMDSTR_VIEWDETAILSA))
    {
        idCmd = FCIDM_SHVIEW_REPORTVIEW;
    }
    else if(HIWORD(lpcmi->lpVerb) != 0 || idCmd < m_LastFolderCMId)
    {
        if (m_folderCM)
        {
            return m_folderCM->InvokeCommand(lpcmi);
        }
        WARN("m_folderCM is NULL!\n");
        return E_NOTIMPL;
    }
    else
    {
        /* The default part of the background menu doesn't have shifted ids so we need to convert the id offset to the real id */
        idCmd += m_idCmdFirst;
    }

    /* The commands that are handled by the def view are forwarded to it */
    switch (idCmd)
    {
    case FCIDM_SHVIEW_INSERT:
    case FCIDM_SHVIEW_INSERTLINK:
        if (m_folderCM)
        {
            lpcmi->lpVerb = MAKEINTRESOURCEA(idCmd);
            return m_folderCM->InvokeCommand(lpcmi);
        }
        WARN("m_folderCM is NULL!\n");
        return E_NOTIMPL;
    case FCIDM_SHVIEW_BIGICON:
    case FCIDM_SHVIEW_SMALLICON:
    case FCIDM_SHVIEW_LISTVIEW:
    case FCIDM_SHVIEW_REPORTVIEW:
    case 0x30: /* FIX IDS in resource files */
    case 0x31:
    case 0x32:
    case 0x33:
    case FCIDM_SHVIEW_AUTOARRANGE:
    case FCIDM_SHVIEW_SNAPTOGRID:
    case FCIDM_SHVIEW_REFRESH:
    case FCIDM_SHVIEW_ALIGNTOGRID:
        if (!m_site)
            return E_FAIL;

        /* Get a pointer to the shell browser */
        CComPtr<IShellView> psv;
        HRESULT hr = IUnknown_QueryService(m_site, SID_IFolderView, IID_PPV_ARG(IShellView, &psv));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        HWND hwndSV = NULL;
        if (SUCCEEDED(psv->GetWindow(&hwndSV)))
            SendMessageW(hwndSV, WM_COMMAND, MAKEWPARAM(idCmd, 0), 0);
        return S_OK;
    }

    ERR("Got unknown command id %ul\n", LOWORD(lpcmi->lpVerb));
    return E_FAIL;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *lpReserved, LPSTR lpszName, UINT uMaxNameLen)
{
    if (m_folderCM)
    {
        return m_folderCM->GetCommandString(idCommand, uFlags, lpReserved, lpszName, uMaxNameLen);
    }

    return E_NOTIMPL;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(m_folderCM)
    {
        CComPtr<IContextMenu2> pfolderCM2;
        HRESULT hr = m_folderCM->QueryInterface(IID_PPV_ARG(IContextMenu2, &pfolderCM2));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return pfolderCM2->HandleMenuMsg(uMsg, wParam, lParam);
    }

    return E_NOTIMPL;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    if(m_folderCM)
    {
        CComPtr<IContextMenu3> pfolderCM3;
        HRESULT hr = m_folderCM->QueryInterface(IID_PPV_ARG(IContextMenu3, &pfolderCM3));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return pfolderCM3->HandleMenuMsg2(uMsg, wParam, lParam, plResult);
    }

    return E_NOTIMPL;
}


HRESULT
CDefViewBckgrndMenu_CreateInstance(IShellFolder* psf, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CDefViewBckgrndMenu>(psf, riid, ppv);
}
