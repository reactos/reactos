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

// TODO: Convert into an IContextMenu

typedef struct _STARTMNU_CTMENU_CTX
{
    IContextMenu *pcm;
    LPITEMIDLIST pidl;
} STARTMNU_CTMENU_CTX, *PSTARTMNU_CTMENU_CTX;

static HMENU
CreateStartContextMenu(IN HWND hWndOwner,
                       IN PVOID *ppcmContext,
                       IN PVOID Context  OPTIONAL);

static VOID
OnStartContextMenuCommand(IN HWND hWndOwner,
                          IN UINT uiCmdId,
                          IN PVOID pcmContext  OPTIONAL,
                          IN PVOID Context  OPTIONAL);

const TRAYWINDOW_CTXMENU StartMenuBtnCtxMenu = {
    CreateStartContextMenu,
    OnStartContextMenuCommand
};

static HMENU
CreateContextMenuFromShellFolderPidl(IN HWND hWndOwner,
                                     IN OUT IShellFolder *psf,
                                     IN OUT LPITEMIDLIST pidl,
                                     OUT IContextMenu **ppcm)
{
    CComPtr<IContextMenu> pcm;
    HRESULT hRet;
    HMENU hPopup;

    hRet = psf->GetUIObjectOf(hWndOwner, 1, (LPCITEMIDLIST *) &pidl, IID_NULL_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hRet))
    {
        hPopup = CreatePopupMenu();

        if (hPopup != NULL)
        {
            hRet = pcm->QueryContextMenu(
                hPopup,
                0,
                ID_SHELL_CMD_FIRST,
                ID_SHELL_CMD_LAST,
                CMF_VERBSONLY);

            if (SUCCEEDED(hRet))
            {
                *ppcm = pcm;
                return hPopup;
            }

            DestroyMenu(hPopup);
        }
    }

    return NULL;
}

static VOID
OnStartContextMenuCommand(IN HWND hWndOwner,
                          IN UINT uiCmdId,
                          IN PVOID pcmContext  OPTIONAL,
                          IN PVOID Context  OPTIONAL)
{
    PSTARTMNU_CTMENU_CTX psmcmc = (PSTARTMNU_CTMENU_CTX) pcmContext;

    if (uiCmdId != 0)
    {
        if ((uiCmdId >= ID_SHELL_CMD_FIRST) && (uiCmdId <= ID_SHELL_CMD_LAST))
        {
            CMINVOKECOMMANDINFO cmici = { 0 };
            CHAR szDir[MAX_PATH];

            /* Setup and invoke the shell command */
            cmici.cbSize = sizeof(cmici);
            cmici.hwnd = hWndOwner;
            cmici.lpVerb = (LPCSTR) MAKEINTRESOURCE(uiCmdId - ID_SHELL_CMD_FIRST);
            cmici.nShow = SW_NORMAL;

            /* FIXME: Support Unicode!!! */
            if (SHGetPathFromIDListA(psmcmc->pidl,
                szDir))
            {
                cmici.lpDirectory = szDir;
            }

            psmcmc->pcm->InvokeCommand(&cmici);
        }
        else
        {
            ITrayWindow * TrayWnd = (ITrayWindow *) Context;
            TrayWnd->ExecContextMenuCmd(uiCmdId);
        }
    }

    psmcmc->pcm->Release();

    HeapFree(hProcessHeap, 0, psmcmc);
}

static VOID
AddStartContextMenuItems(IN HWND hWndOwner, IN HMENU hPopup)
{
    WCHAR szBuf[MAX_PATH];
    HRESULT hRet;

    /* Add the "Open All Users" menu item */
    if (LoadString(hExplorerInstance,
        IDS_PROPERTIES,
        szBuf,
        sizeof(szBuf) / sizeof(szBuf[0])))
    {
        AppendMenu(hPopup,
                   MF_STRING,
                   ID_SHELL_CMD_PROPERTIES,
                   szBuf);
    }

    if (!SHRestricted(REST_NOCOMMONGROUPS))
    {
        /* Check if we should add menu items for the common start menu */
        hRet = SHGetFolderPath(hWndOwner,
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
                if (LoadString(hExplorerInstance,
                    IDS_OPEN_ALL_USERS,
                    szBuf,
                    sizeof(szBuf) / sizeof(szBuf[0])))
                {
                    AppendMenu(hPopup,
                               MF_STRING,
                               ID_SHELL_CMD_OPEN_ALL_USERS,
                               szBuf);
                }

                /* Add the "Explore All Users" menu item */
                if (LoadString(hExplorerInstance,
                    IDS_EXPLORE_ALL_USERS,
                    szBuf,
                    sizeof(szBuf) / sizeof(szBuf[0])))
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

static HMENU
CreateStartContextMenu(IN HWND hWndOwner,
                       IN PVOID *ppcmContext,
                       IN PVOID Context  OPTIONAL)
{
    LPITEMIDLIST pidlStart, pidlLast;
    CComPtr<IShellFolder> psfStart;
    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IContextMenu> pcm;
    HRESULT hRet;
    HMENU hPopup;

    pidlStart = SHCloneSpecialIDList(hWndOwner,
                                     CSIDL_STARTMENU,
                                     TRUE);

    if (pidlStart != NULL)
    {
        pidlLast = ILClone(ILFindLastID(pidlStart));
        ILRemoveLastID(pidlStart);

        if (pidlLast != NULL)
        {
            hRet = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hRet))
            {
                hRet = psfDesktop->BindToObject(pidlStart, NULL, IID_PPV_ARG(IShellFolder, &psfStart));
                if (SUCCEEDED(hRet))
                {
                    hPopup = CreateContextMenuFromShellFolderPidl(hWndOwner,
                                                                  psfStart,
                                                                  pidlLast,
                                                                  &pcm);

                    if (hPopup != NULL)
                    {
                        PSTARTMNU_CTMENU_CTX psmcmc;

                        psmcmc = (PSTARTMNU_CTMENU_CTX) HeapAlloc(hProcessHeap, 0, sizeof(*psmcmc));
                        if (psmcmc != NULL)
                        {
                            psmcmc->pcm = pcm;
                            psmcmc->pidl = pidlLast;

                            AddStartContextMenuItems(hWndOwner,
                                                     hPopup);

                            *ppcmContext = psmcmc;
                            return hPopup;
                        }
                        else
                        {
                            DestroyMenu(hPopup);
                            hPopup = NULL;
                        }
                    }
                }
            }

            ILFree(pidlLast);
        }

        ILFree(pidlStart);
    }

    return NULL;
}
