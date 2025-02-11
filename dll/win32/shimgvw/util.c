#include "shimgvw.h"
#include <windowsx.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shellutils.h>
#include <shlwapi_undoc.h>

IContextMenu *g_pContextMenu = NULL;

static int
GetMenuItemIdByPos(HMENU hMenu, UINT Pos)
{
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); /* USER32 version agnostic */
    mii.fMask = MIIM_ID;
    mii.cch = 0;
    return GetMenuItemInfoW(hMenu, Pos, TRUE, &mii) ? mii.wID : -1;
}

static BOOL
IsMenuSeparator(HMENU hMenu, UINT Pos)
{
    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); /* USER32 version agnostic */
    mii.fMask = MIIM_FTYPE;
    mii.cch = 0;
    return GetMenuItemInfoW(hMenu, Pos, TRUE, &mii) && (mii.fType & MFT_SEPARATOR);
}

static BOOL
IsSelfShellVerb(PCWSTR Assoc, PCWSTR Verb)
{
    WCHAR buf[MAX_PATH * 3];
    DWORD cch = _countof(buf);
    HRESULT hr = AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_COMMAND, Assoc, Verb, buf, &cch);
    return hr == S_OK && *Assoc == L'.' && StrStrW(buf, L",ImageView_Fullscreen");
}

static void
ModifyShellContextMenu(IContextMenu *pCM, HMENU hMenu, UINT CmdIdFirst, PCWSTR Assoc)
{
    HRESULT hr;
    UINT id, i;
    for (i = 0; i < GetMenuItemCount(hMenu); ++i)
    {
        WCHAR buf[200];
        id = GetMenuItemIdByPos(hMenu, i);
        if (id == (UINT)-1)
            continue;

        *buf = UNICODE_NULL;
        /* Note: We just ask for the wide string because all the items we care about come from shell32 and it handles both */
        hr = IContextMenu_GetCommandString(pCM, id - CmdIdFirst, GCS_VERBW, NULL, (char*)buf, _countof(buf));
        if (SUCCEEDED(hr))
        {
            UINT remove = FALSE;
            if (IsSelfShellVerb(Assoc, buf))
                ++remove;
            else if (!lstrcmpiW(L"cut", buf) || !lstrcmpiW(L"copy", buf) || !lstrcmpiW(L"link", buf))
                ++remove;

            if (remove && DeleteMenu(hMenu, i, MF_BYPOSITION))
            {
                if (i-- > 0)
                {
                    if (IsMenuSeparator(hMenu, i) && IsMenuSeparator(hMenu, i + 1))
                        DeleteMenu(hMenu, i, MF_BYPOSITION);
                }
            }
        }
    }

    while (IsMenuSeparator(hMenu, 0) && DeleteMenu(hMenu, 0, MF_BYPOSITION)) {}
}

static LRESULT CALLBACK
ShellContextMenuWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    if (FAILED(SHForwardContextMenuMsg((IUnknown*)g_pContextMenu, uMsg, wParam, lParam, &lRes, TRUE)))
        lRes = DefWindowProc(hwnd, uMsg, wParam, lParam);
    return lRes;
}

static void
DoShellContextMenu(HWND hwnd, IContextMenu *pCM, PCWSTR File, LPARAM lParam)
{
    enum { first = 1, last = 0x7fff };
    HRESULT hr;
    HMENU hMenu = CreatePopupMenu();
    UINT cmf = GetKeyState(VK_SHIFT) < 0 ? CMF_EXTENDEDVERBS : 0;

    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    if ((int)lParam == -1)
    {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        pt.x = (rect.left + rect.right) / 2;
        pt.y = rect.top;
    }

    g_pContextMenu = pCM;
    hwnd = SHCreateWorkerWindowW(ShellContextMenuWindowProc, hwnd, 0, WS_VISIBLE | WS_CHILD, NULL, 0);
    if (!hwnd)
        goto die;
    hr = IContextMenu_QueryContextMenu(pCM, hMenu, 0, first, last, cmf | CMF_NODEFAULT);
    if (SUCCEEDED(hr))
    {
        UINT id;
        ModifyShellContextMenu(pCM, hMenu, first, PathFindExtensionW(File));
        id = TrackPopupMenuEx(hMenu, TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
        if (id)
        {
            UINT flags = (GetKeyState(VK_SHIFT) < 0 ? CMIC_MASK_SHIFT_DOWN : 0) |
                         (GetKeyState(VK_CONTROL) < 0 ? CMIC_MASK_CONTROL_DOWN : 0);
            CMINVOKECOMMANDINFO ici = { sizeof(ici), flags, hwnd, MAKEINTRESOURCEA(id - first) };
            ici.nShow = SW_SHOW;
            hr = IContextMenu_InvokeCommand(pCM, &ici);
        }
    }
    DestroyWindow(hwnd);
die:
    DestroyMenu(hMenu);
    g_pContextMenu = NULL;
}

void
DoShellContextMenuOnFile(HWND hwnd, PCWSTR File, LPARAM lParam)
{
    HRESULT hr;
    IShellFolder *pSF;
    PCUITEMID_CHILD pidlItem;
    PIDLIST_ABSOLUTE pidl = ILCreateFromPath(File);
    if (pidl && SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &pSF), &pidlItem)))
    {
        IContextMenu *pCM;
        hr = IShellFolder_GetUIObjectOf(pSF, hwnd, 1, &pidlItem, &IID_IContextMenu, NULL, (void**)&pCM);
        if (SUCCEEDED(hr))
        {
            DoShellContextMenu(hwnd, pCM, File, lParam);
            IContextMenu_Release(pCM);
        }
        IShellFolder_Release(pSF);
    }
    SHFree(pidl);
}

void DisplayHelp(HWND hwnd)
{
    SHELL_ErrorBoxHelper(hwnd, ERROR_NOT_SUPPORTED);
}
