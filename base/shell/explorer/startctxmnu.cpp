/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

/*
 * Start menu button context menu
 */

class CStartMenuBtnCtxMenu :
    public CComCoClass<CStartMenuBtnCtxMenu>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IContextMenu
{
    /* AddStartContextMenuItems uses ID_SHELL_CMD IDs directly and relies on idCmdFirst being 0.
     * CTrayWindow::TrackCtxMenu must pass 0 because DeleteMenu ID_SHELL_CMD_UNDO_ACTION would
     * delete the wrong item if it used 1. m_Inner->QueryContextMenu is not aware of this game
     * so we have to reserve the entire ID_SHELL_CMD range for ourselves here. */
    enum { INNERIDOFFSET = ID_SHELL_CMD_LAST + 1 };
    static BOOL IsShellCmdId(UINT_PTR id) { return id < INNERIDOFFSET; }

    CComPtr<ITrayWindow>  m_TrayWnd;
    CComPtr<IContextMenu> m_Inner;
    CComPtr<IShellFolder> m_Folder;

    HWND m_Owner;
    LPITEMIDLIST m_FolderPidl;

    HRESULT CreateContextMenuFromShellFolderPidl(HMENU hPopup, UINT idCmdFirst, UINT idCmdLast)
    {
        HRESULT hRet;

        hRet = m_Folder->GetUIObjectOf(m_Owner, 1, (LPCITEMIDLIST *) &m_FolderPidl, IID_NULL_PPV_ARG(IContextMenu, &m_Inner));
        if (SUCCEEDED(hRet))
        {
            if (hPopup != NULL)
            {
                hRet = m_Inner->QueryContextMenu(
                    hPopup,
                    0,
                    idCmdFirst,
                    idCmdLast,
                    CMF_VERBSONLY);

                if (SUCCEEDED(hRet))
                {
                    return hRet;
                }
            }
        }
        return E_FAIL;
    }

    VOID AddStartContextMenuItems(IN HMENU hPopup)
    {
        WCHAR szBuf[MAX_PATH];
        HRESULT hRet;
        C_ASSERT(ID_SHELL_CMD_FIRST != 0);
         /* If this ever asserts, let m_Inner use 1..ID_SHELL_CMD_FIRST-1 instead */
        C_ASSERT(ID_SHELL_CMD_LAST < 0xffff / 2);

        /* Add the "Open All Users" menu item */
        if (LoadStringW(hExplorerInstance,
                        IDS_PROPERTIES,
                        szBuf,
                        _countof(szBuf)))
        {
            AppendMenu(hPopup,
                       MF_STRING,
                       ID_SHELL_CMD_PROPERTIES,
                       szBuf);
        }

        if (!SHRestricted(REST_NOCOMMONGROUPS))
        {
            /* Check if we should add menu items for the common start menu */
            hRet = SHGetFolderPath(m_Owner,
                                   CSIDL_COMMON_STARTMENU,
                                   NULL,
                                   SHGFP_TYPE_CURRENT,
                                   szBuf);
            if (SUCCEEDED(hRet) && hRet != S_FALSE)
            {
                /* The directory exists, but only show the items if the
                user can actually make any changes to the common start
                menu. This is most likely only the case if the user
                has administrative rights! */
                if (IsUserAnAdmin())
                {
                    AppendMenu(hPopup,
                               MF_SEPARATOR,
                               0,
                               NULL);

                    /* Add the "Open All Users" menu item */
                    if (LoadStringW(hExplorerInstance,
                                    IDS_OPEN_ALL_USERS,
                                    szBuf,
                                    _countof(szBuf)))
                    {
                        AppendMenu(hPopup,
                                   MF_STRING,
                                   ID_SHELL_CMD_OPEN_ALL_USERS,
                                   szBuf);
                    }

                    /* Add the "Explore All Users" menu item */
                    if (LoadStringW(hExplorerInstance,
                                    IDS_EXPLORE_ALL_USERS,
                                    szBuf,
                                    _countof(szBuf)))
                    {
                        AppendMenu(hPopup,
                                   MF_STRING,
                                   ID_SHELL_CMD_EXPLORE_ALL_USERS,
                                   szBuf);
                    }
                }
            }
        }
    }

public:
    HRESULT Initialize(ITrayWindow * pTrayWnd, IN HWND hWndOwner)
    {
        m_TrayWnd = pTrayWnd;
        m_Owner = hWndOwner;
        return S_OK;
    }

    STDMETHODIMP
    QueryContextMenu(HMENU hPopup,
                     UINT indexMenu,
                     UINT idCmdFirst,
                     UINT idCmdLast,
                     UINT uFlags) override
    {
        LPITEMIDLIST pidlStart;
        CComPtr<IShellFolder> psfDesktop;
        HRESULT hRet = S_OK;
        UINT idInnerFirst = idCmdFirst + INNERIDOFFSET;

        psfDesktop = NULL;
        m_Inner = NULL;

        pidlStart = SHCloneSpecialIDList(m_Owner, CSIDL_STARTMENU, TRUE);
        if (pidlStart != NULL)
        {
            m_FolderPidl = ILClone(ILFindLastID(pidlStart));
            ILRemoveLastID(pidlStart);

            if (m_FolderPidl != NULL)
            {
                hRet = SHGetDesktopFolder(&psfDesktop);
                if (SUCCEEDED(hRet))
                {
                    hRet = psfDesktop->BindToObject(pidlStart, NULL, IID_PPV_ARG(IShellFolder, &m_Folder));
                    if (SUCCEEDED(hRet))
                    {
                        hRet = CreateContextMenuFromShellFolderPidl(hPopup, idInnerFirst, idCmdLast);
                    }
                }
            }

            ILFree(pidlStart);
        }
        if (idCmdLast - idCmdFirst >= ID_SHELL_CMD_LAST - ID_SHELL_CMD_FIRST)
        {
            AddStartContextMenuItems(hPopup);
            hRet = SUCCEEDED(hRet) ? hRet + idInnerFirst : idInnerFirst;
        }
        return hRet;
    }

    STDMETHODIMP
    InvokeCommand(LPCMINVOKECOMMANDINFO lpici) override
    {
        UINT uiCmdId = PtrToUlong(lpici->lpVerb);
        if (!IsShellCmdId(uiCmdId))
        {
            CMINVOKECOMMANDINFOEX cmici = { sizeof(cmici) };

            /* Setup and invoke the shell command */
            cmici.hwnd = m_Owner;
            cmici.nShow = SW_NORMAL;
            cmici.fMask = CMIC_MASK_UNICODE;
            WCHAR szVerbW[MAX_PATH];
            if (IS_INTRESOURCE(lpici->lpVerb))
            {
                cmici.lpVerb = MAKEINTRESOURCEA(uiCmdId - INNERIDOFFSET);
                cmici.lpVerbW = MAKEINTRESOURCEW(uiCmdId - INNERIDOFFSET);
            }
            else
            {
                cmici.lpVerb = lpici->lpVerb;
                SHAnsiToUnicode(lpici->lpVerb, szVerbW, _countof(szVerbW));
                cmici.lpVerbW = szVerbW;
            }

            CHAR szDirA[MAX_PATH];
            WCHAR szDirW[MAX_PATH];
            if (SHGetPathFromIDListW(m_FolderPidl, szDirW))
            {
                SHUnicodeToAnsi(szDirW, szDirA, _countof(szDirA));
                cmici.lpDirectory = szDirA;
                cmici.lpDirectoryW = szDirW;
            }

            return m_Inner->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmici);
        }
        m_TrayWnd->ExecContextMenuCmd(uiCmdId);
        return S_OK;
    }

    STDMETHODIMP
    GetCommandString(
        UINT_PTR idCmd,
        UINT uType,
        UINT *pwReserved,
        LPSTR pszName,
        UINT cchMax) override
    {
        if (!IsShellCmdId(idCmd) && m_Inner)
            return m_Inner->GetCommandString(idCmd, uType, pwReserved, pszName, cchMax);
        return E_NOTIMPL;
    }

    CStartMenuBtnCtxMenu()
    {
    }

    virtual ~CStartMenuBtnCtxMenu()
    {
        if (m_FolderPidl)
            ILFree(m_FolderPidl);
    }

    BEGIN_COM_MAP(CStartMenuBtnCtxMenu)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
    END_COM_MAP()
};

HRESULT CStartMenuBtnCtxMenu_CreateInstance(ITrayWindow * m_TrayWnd, IN HWND m_Owner, IContextMenu ** ppCtxMenu)
{
    return ShellObjectCreatorInit<CStartMenuBtnCtxMenu>(m_TrayWnd, m_Owner, IID_PPV_ARG(IContextMenu, ppCtxMenu));
}
