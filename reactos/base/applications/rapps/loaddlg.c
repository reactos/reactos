/* PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/rapps/download.c
 * PURPOSE:     Displaying a download dialog
 * COPYRIGHT:   Copyright 2001 John R. Sheets (for CodeWeavers)
 *              Copyright 2004 Mike McCormack (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp (gvg@reactos.org)
 *              Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 */
/*
 * Based on Wine dlls/shdocvw/shdocvw_main.c
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS
#define WIN32_NO_STATUS

#include "rapps.h"
#include "resource.h"

#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <urlmon.h>

static PAPPLICATION_INFO AppInfo;

typedef struct _IBindStatusCallbackImpl
{
    const IBindStatusCallbackVtbl *vtbl;
    LONG ref;
    HWND hDialog;
    BOOL *pbCancelled;
} IBindStatusCallbackImpl;

static
HRESULT WINAPI
dlQueryInterface(IBindStatusCallback* This, REFIID riid, void** ppvObject)
{
    if (!ppvObject) return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IBindStatusCallback))
    {
        IBindStatusCallback_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG WINAPI
dlAddRef(IBindStatusCallback* iface)
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl*) iface;
    return InterlockedIncrement(&This->ref);
}

static
ULONG WINAPI
dlRelease(IBindStatusCallback* iface)
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl*) iface;
    DWORD ref = InterlockedDecrement(&This->ref);

    if (!ref)
    {
        DestroyWindow(This->hDialog);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static
HRESULT WINAPI
dlOnStartBinding(IBindStatusCallback* iface, DWORD dwReserved, IBinding* pib)
{
    return S_OK;
}

static
HRESULT WINAPI
dlGetPriority(IBindStatusCallback* iface, LONG* pnPriority)
{
    return S_OK;
}

static
HRESULT WINAPI
dlOnLowResource( IBindStatusCallback* iface, DWORD reserved)
{
    return S_OK;
}

static
HRESULT WINAPI
dlOnProgress(IBindStatusCallback* iface,
             ULONG ulProgress,
             ULONG ulProgressMax,
             ULONG ulStatusCode,
             LPCWSTR szStatusText)
{
    IBindStatusCallbackImpl *This = (IBindStatusCallbackImpl *) iface;
    HWND Item;
    LONG r;
    WCHAR OldText[100];

    Item = GetDlgItem(This->hDialog, IDC_DOWNLOAD_PROGRESS);
    if (Item && ulProgressMax)
    {
        SendMessageW(Item, PBM_SETPOS, ((ULONGLONG)ulProgress * 100) / ulProgressMax, 0);
    }

    Item = GetDlgItem(This->hDialog, IDC_DOWNLOAD_STATUS);
    if (Item && szStatusText)
    {
        SendMessageW(Item, WM_GETTEXT, sizeof(OldText) / sizeof(OldText[0]), (LPARAM) OldText);
        if (sizeof(OldText) / sizeof(OldText[0]) - 1 <= wcslen(OldText) || 0 != wcscmp(OldText, szStatusText))
        {
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) szStatusText);
        }
    }

    SetLastError(0);
    r = GetWindowLongPtrW(This->hDialog, GWLP_USERDATA);
    if (0 != r || 0 != GetLastError())
    {
        *This->pbCancelled = TRUE;
        return E_ABORT;
    }

    return S_OK;
}

static
HRESULT WINAPI
dlOnStopBinding(IBindStatusCallback* iface, HRESULT hresult, LPCWSTR szError)
{
    return S_OK;
}

static
HRESULT WINAPI
dlGetBindInfo(IBindStatusCallback* iface, DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    return S_OK;
}

static
HRESULT WINAPI
dlOnDataAvailable(IBindStatusCallback* iface, DWORD grfBSCF,
                DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    return S_OK;
}

static
HRESULT WINAPI
dlOnObjectAvailable(IBindStatusCallback* iface, REFIID riid, IUnknown* punk)
{
    return S_OK;
}

static const IBindStatusCallbackVtbl dlVtbl =
{
    dlQueryInterface,
    dlAddRef,
    dlRelease,
    dlOnStartBinding,
    dlGetPriority,
    dlOnLowResource,
    dlOnProgress,
    dlOnStopBinding,
    dlGetBindInfo,
    dlOnDataAvailable,
    dlOnObjectAvailable
};

static IBindStatusCallback*
CreateDl(HWND Dlg, BOOL *pbCancelled)
{
    IBindStatusCallbackImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IBindStatusCallbackImpl));
    if (!This) return NULL;

    This->vtbl = &dlVtbl;
    This->ref = 1;
    This->hDialog = Dlg;
    This->pbCancelled = pbCancelled;

    return (IBindStatusCallback*) This;
}

static
DWORD WINAPI
ThreadFunc(LPVOID Context)
{
    IBindStatusCallback *dl;
    WCHAR path[MAX_PATH];
    LPWSTR p;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HWND Dlg = (HWND) Context;
    DWORD r;
    BOOL bCancelled = FALSE;
    BOOL bTempfile = FALSE;
    HKEY hKey = NULL;
    DWORD dwSize = MAX_PATH;

    /* built the path for the download */
    p = wcsrchr(AppInfo->szUrlDownload, L'/');
    if (!p) goto end;

    /* Create default download path */
    if (GetWindowsDirectoryW(path, sizeof(path) / sizeof(WCHAR)))
    {
        WCHAR DPath[MAX_PATH];
        int i;
        for (i = 0; i < 4; i++)
        {
            if (i == 3)
            {
                DPath[i] = '\0';
                break;
            }
            DPath[i] = path[i];
        }
        LoadStringW(hInst, IDS_DOWNLOAD_FOLDER, path, sizeof(path) / sizeof(WCHAR));
        wcscat(DPath, path);
        wcscpy(path, DPath);
    }

    if (wcslen(AppInfo->szUrlDownload) > 4)
    {
        if (AppInfo->szUrlDownload[wcslen(AppInfo->szUrlDownload) - 4] == '.' &&
            AppInfo->szUrlDownload[wcslen(AppInfo->szUrlDownload) - 3] == 'c' &&
            AppInfo->szUrlDownload[wcslen(AppInfo->szUrlDownload) - 2] == 'a' &&
            AppInfo->szUrlDownload[wcslen(AppInfo->szUrlDownload) - 1] == 'b')
        {
            if (!GetCurrentDirectoryW(MAX_PATH, path))
                goto end;
        }
        else
        {
            if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                            L"Software\\ReactOS\\rappmgr",
                            &hKey) == ERROR_SUCCESS)
            {
                if ((RegQueryValueExW(hKey,
                                      L"DownloadFolder",
                                      NULL,
                                      NULL,
                                      (LPBYTE)&path,
                                      &dwSize) != ERROR_SUCCESS) && (path[0] == 0))
                {
                    RegCloseKey(hKey);
                    goto end;
                }
                RegCloseKey(hKey);
            }
        }
    }
    else goto end;

    if (GetFileAttributesW(path) == 0xFFFFFFFF)
    {
        if (!CreateDirectoryW(path, NULL))
            goto end;
    }

    wcscat(path, L"\\");
    wcscat(path, p + 1);

    /* download it */
    bTempfile = TRUE;
    dl = CreateDl(Context, &bCancelled);
    r = URLDownloadToFileW(NULL, AppInfo->szUrlDownload, path, 0, dl);
    if (dl) IBindStatusCallback_Release(dl);
    if (S_OK != r)
    {
        MessageBoxW(0, L"Download error!", NULL, 0);
        goto end;
    }
    else if (bCancelled)
    {
        goto end;
    }
    ShowWindow(Dlg, SW_HIDE);

    /* run it */
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    r = CreateProcessW(path, NULL, NULL, NULL, 0, 0, NULL, NULL, &si, &pi);
    if (0 == r)
    {
        goto end;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    end:
        if (bTempfile)
        {
            if (bCancelled)
                DeleteFileW(path);
        }
    EndDialog(Dlg, 0);
    return 0;
}

static
INT_PTR CALLBACK
DownloadDlgProc(HWND Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE Thread;
    DWORD ThreadId;
    HWND Item;

    switch (Msg)
    {
        case WM_INITDIALOG:

            SetWindowLongPtrW(Dlg, GWLP_USERDATA, 0);
            Item = GetDlgItem(Dlg, IDC_DOWNLOAD_PROGRESS);
            if (Item)
            {
                SendMessageW(Item, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                SendMessageW(Item, PBM_SETPOS, 0, 0);
            }

            Thread = CreateThread(NULL, 0, ThreadFunc, Dlg, 0, &ThreadId);
            if (!Thread) return FALSE;
            CloseHandle(Thread);
            return TRUE;

        case WM_COMMAND:
            if (wParam == IDCANCEL)
            {
                SetWindowLongPtrW(Dlg, GWLP_USERDATA, 1);
                PostMessageW(Dlg, WM_CLOSE, 0, 0);
            }
            return FALSE;

        case WM_CLOSE:
            EndDialog(Dlg, 0);
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL
DownloadApplication(INT Index)
{
    if (!IS_AVAILABLE_ENUM(SelectedEnumType))
        return FALSE;

    AppInfo = (PAPPLICATION_INFO) ListViewGetlParam(Index);
    if (!AppInfo) return FALSE;

    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
               hMainWnd,
               DownloadDlgProc);

    return TRUE;
}

VOID
DownloadApplicationsDB(LPWSTR lpUrl)
{
    APPLICATION_INFO IntInfo;

    ZeroMemory(&IntInfo, sizeof(APPLICATION_INFO));
    wcscpy(IntInfo.szUrlDownload, lpUrl);

    AppInfo = &IntInfo;

    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
               hMainWnd,
               DownloadDlgProc);
}

