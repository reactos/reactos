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
    return S_OK;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::SetSite(IUnknown *pUnkSite)
{
    m_site = pUnkSite;
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
        Shell_MergeMenus(hMenu, GetSubMenu(hMenuPart, 0), indexMenu, 0, 0xFFFF, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS);
        indexMenu += GetMenuItemCount(GetSubMenu(hMenuPart, 0));
        DestroyMenu(hMenuPart);
    }
    else
    {
        ERR("Failed to load menu from resource!\n");
    }

    /* Query the shell folder to add any items it wants to add in the background context menu */
    hMenuPart = CreatePopupMenu();
    if (hMenuPart)
    {
        hr = m_psf->CreateViewObject(NULL, IID_PPV_ARG(IContextMenu, &m_folderCM));
        if (SUCCEEDED(hr))
        {
            InsertMenuA(hMenu, indexMenu++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            UINT SeparatorIndex = indexMenu;
            int count = GetMenuItemCount(hMenu);

            hr = m_folderCM->QueryContextMenu(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
            if (SUCCEEDED(hr))
            {
                //Shell_MergeMenus(hMenu, hMenuPart, indexMenu, 0, UINT_MAX, MM_ADDSEPARATOR| MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS);
                //DestroyMenu(hMenuPart);
            }
            else
            {
                WARN("QueryContextMenu failed!\n");
            }

            /* If no item was added after the separator, remove it */
            if (count == GetMenuItemCount(hMenu))
                DeleteMenu(hMenu, SeparatorIndex, MF_BYPOSITION);

        }
        else
        {
            WARN("GetUIObjectOf didn't give any context menu!\n");   
        }          
    }
    else
    {
        ERR("CreatePopupMenu failed!\n");
    }

    return S_OK;
}

HRESULT
WINAPI
CDefViewBckgrndMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
    /* The commands that are handled by the def view are forwarded to it */
    switch (LOWORD(lpcmi->lpVerb))
    {
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
        if (!m_site)
            return E_FAIL;

        /* Get a pointer to the shell browser */
        CComPtr<IShellView> psv;
        HRESULT hr = IUnknown_QueryService(m_site, SID_IFolderView, IID_PPV_ARG(IShellView, &psv));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        HWND hwndSV = NULL;
        if (SUCCEEDED(psv->GetWindow(&hwndSV)))
            SendMessageW(hwndSV, WM_COMMAND, MAKEWPARAM(LOWORD(lpcmi->lpVerb), 0), 0);
        return S_OK;
    }

    /* Unknown commands are added by the folder context menu so forward the invocation */
    return m_folderCM->InvokeCommand(lpcmi);

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
