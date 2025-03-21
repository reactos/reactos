/*
 * PROJECT:     ReactOS Picture and Fax Viewer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Utility routines
 * COPYRIGHT:   Copyright 2025 Whindmar Saksit <whindsaks@proton.me>
 */

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
    for (UINT i = 0, c = GetMenuItemCount(hMenu); i < c; ++i)
    {
        WCHAR buf[200];
        UINT id = GetMenuItemIdByPos(hMenu, i);
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
            else if (!lstrcmpiW(L"cut", buf) || !lstrcmpiW(L"paste", buf) || !lstrcmpiW(L"pastelink", buf) ||
                     !lstrcmpiW(L"delete", buf) || !lstrcmpiW(L"link", buf))
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

HRESULT
GetUIObjectOfPath(HWND hwnd, PCWSTR File, REFIID riid, void **ppv)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    IShellFolder *pSF;
    PCUITEMID_CHILD pidlItem;
    PIDLIST_ABSOLUTE pidl = ILCreateFromPath(File);
    *ppv = NULL;
    if (pidl && SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &pSF), &pidlItem)))
    {
        hr = IShellFolder_GetUIObjectOf(pSF, hwnd, 1, &pidlItem, riid, NULL, ppv);
        IShellFolder_Release(pSF);
    }
    SHFree(pidl);
    return hr;
}

void
DoShellContextMenuOnFile(HWND hwnd, PCWSTR File, LPARAM lParam)
{
    IContextMenu *pCM;
    HRESULT hr = GetUIObjectOfPath(hwnd, File, IID_PPV_ARG(IContextMenu, &pCM));
    if (SUCCEEDED(hr))
    {
        DoShellContextMenu(hwnd, pCM, File, lParam);
        IContextMenu_Release(pCM);
    }
}

typedef struct _ENABLECOMMANDDATA
{
    HWND hwnd;
    PCWSTR Verb;
    UINT CmdId;
    UINT ImageId;
    WCHAR File[ANYSIZE_ARRAY];
} ENABLECOMMANDDATA;

static DWORD CALLBACK
EnableCommandIfVerbExistsProc(LPVOID ThreadParam)
{
    enum { first = 1, last = 0x7fff };
    ENABLECOMMANDDATA *pData = ThreadParam;
    IContextMenu *pCM;
    HRESULT hr = GetUIObjectOfPath(pData->hwnd, pData->File, IID_PPV_ARG(IContextMenu, &pCM));
    if (SUCCEEDED(hr))
    {
        HMENU hMenu = CreatePopupMenu();
        hr = IContextMenu_QueryContextMenu(pCM, hMenu, 0, first, last, CMF_NORMAL);
        if (SUCCEEDED(hr))
        {
            for (UINT i = 0, c = GetMenuItemCount(hMenu); i < c; ++i)
            {
                WCHAR buf[200];
                UINT id = GetMenuItemIdByPos(hMenu, i);
                if (id == (UINT)-1)
                    continue;

                *buf = UNICODE_NULL;
                hr = IContextMenu_GetCommandString(pCM, id - first, GCS_VERBW, NULL, (char*)buf, _countof(buf));
                if (SUCCEEDED(hr) && !lstrcmpiW(buf, pData->Verb))
                {
                    PostMessageW(pData->hwnd, WM_UPDATECOMMANDSTATE, MAKELONG(pData->CmdId, TRUE), pData->ImageId);
                    break;
                }
            }
        }
        DestroyMenu(hMenu);
        IContextMenu_Release(pCM);
    }
    SHFree(pData);
    return 0;
}

void
EnableCommandIfVerbExists(UINT ImageId, HWND hwnd, UINT CmdId, PCWSTR Verb, PCWSTR File)
{
    const SIZE_T cch = lstrlenW(File) + 1;
    ENABLECOMMANDDATA *pData = SHAlloc(FIELD_OFFSET(ENABLECOMMANDDATA, File[cch]));
    if (pData)
    {
        pData->hwnd = hwnd;
        pData->Verb = Verb; // Note: This assumes the string is valid for the lifetime of the thread.
        pData->CmdId = CmdId;
        pData->ImageId = ImageId;
        CopyMemory(pData->File, File, cch * sizeof(*File));
        SHCreateThread(EnableCommandIfVerbExistsProc, pData, CTF_COINIT | CTF_INSIST, NULL);
    }
}

void
ShellExecuteVerb(HWND hwnd, PCWSTR Verb, PCWSTR File, BOOL Quit)
{
    SHELLEXECUTEINFOW sei = { sizeof(sei), SEE_MASK_INVOKEIDLIST | SEE_MASK_ASYNCOK };
    if (!*File)
        return;

    sei.hwnd = hwnd;
    sei.lpVerb = Verb;
    sei.lpFile = File;
    sei.nShow = SW_SHOW;
    if (!ShellExecuteExW(&sei))
    {
        DPRINT1("ShellExecuteExW(%ls, %ls) failed with code %ld\n", Verb, File, GetLastError());
    }
    else if (Quit)
    {
        // Destroy the window to quit the application
        DestroyWindow(hwnd);
    }
}

UINT
ErrorBox(HWND hwnd, UINT Error)
{
    return SHELL_ErrorBox(hwnd, Error);
}

void
DisplayHelp(HWND hwnd)
{
    ErrorBox(hwnd, ERROR_NOT_SUPPORTED);
}
