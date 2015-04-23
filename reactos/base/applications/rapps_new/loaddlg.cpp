/* PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/rapps/loaddlg.c
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "rapps.h"

#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <wininet.h>
#include <shellutils.h>

static PAPPLICATION_INFO AppInfo;
static HICON hIcon = NULL;

class CDownloadDialog :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBindStatusCallback
{
    HWND m_hDialog;
    PBOOL m_pbCancelled;

public:
    ~CDownloadDialog()
    {
        DestroyWindow(m_hDialog);
    }
    
    HRESULT Initialize(HWND Dlg, BOOL *pbCancelled)
    {
        m_hDialog = Dlg;
        m_pbCancelled = pbCancelled;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnStartBinding(
        DWORD dwReserved,
        IBinding *pib)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetPriority(
        LONG *pnPriority)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnLowResource(
        DWORD reserved)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnProgress(
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR szStatusText)
    {
        HWND Item;
        LONG r;
        WCHAR OldText[100];

        Item = GetDlgItem(m_hDialog, IDC_DOWNLOAD_PROGRESS);
        if (Item && ulProgressMax)
        {
            SendMessageW(Item, PBM_SETPOS, MulDiv(ulProgress, 100, ulProgressMax), 0);
        }

        Item = GetDlgItem(m_hDialog, IDC_DOWNLOAD_STATUS);
        if (Item && szStatusText)
        {
            SendMessageW(Item, WM_GETTEXT, sizeof(OldText) / sizeof(OldText[0]), (LPARAM) OldText);
            if (sizeof(OldText) / sizeof(OldText[0]) - 1 <= wcslen(OldText) || 0 != wcscmp(OldText, szStatusText))
            {
                SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) szStatusText);
            }
        }

        SetLastError(0);
        r = GetWindowLongPtrW(m_hDialog, GWLP_USERDATA);
        if (0 != r || 0 != GetLastError())
        {
            *m_pbCancelled = TRUE;
            return E_ABORT;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(
        HRESULT hresult,
        LPCWSTR szError)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(
        DWORD *grfBINDF,
        BINDINFO *pbindinfo)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC *pformatetc,
        STGMEDIUM *pstgmed)
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(
        REFIID riid,
        IUnknown *punk)
    {
        return S_OK;
    }

    BEGIN_COM_MAP(CDownloadDialog)
        COM_INTERFACE_ENTRY_IID(IID_IBindStatusCallback, IBindStatusCallback)
    END_COM_MAP()
};

extern "C"
HRESULT WINAPI CDownloadDialog_Constructor(HWND Dlg, BOOL *pbCancelled, REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreatorInit<CDownloadDialog>(Dlg, pbCancelled, riid, ppv);
}

static
DWORD WINAPI
ThreadFunc(LPVOID Context)
{
    CComPtr<IBindStatusCallback> dl;
    WCHAR path[MAX_PATH];
    LPWSTR p;
    HWND Dlg = (HWND) Context;
    ULONG dwContentLen, dwBytesWritten, dwBytesRead, dwStatus;
    ULONG dwCurrentBytesRead = 0;
    ULONG dwStatusLen = sizeof(dwStatus);
    BOOL bCancelled = FALSE;
    BOOL bTempfile = FALSE;
    BOOL bCab = FALSE;
    HINTERNET hOpen = NULL;
    HINTERNET hFile = NULL;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    unsigned char lpBuffer[4096];
    PCWSTR lpszAgent = L"RApps/1.0";
    URL_COMPONENTS urlComponents;
    size_t urlLength;

    /* built the path for the download */
    p = wcsrchr(AppInfo->szUrlDownload, L'/');

    if (!p)
        goto end;

        if (wcscmp(AppInfo->szUrlDownload, APPLICATION_DATABASE_URL) == 0)
        {
            bCab = TRUE;
            if (!GetStorageDirectory(path, sizeof(path) / sizeof(path[0])))
                goto end;
        }
        else
        {
            if (FAILED(StringCbCopyW(path, sizeof(path),  SettingsInfo.szDownloadDir)))
                goto end;
        }


    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(path, NULL))
            goto end;
    }

    if (FAILED(StringCbCatW(path, sizeof(path), L"\\")))
        goto end;
    if (FAILED(StringCbCatW(path, sizeof(path), p + 1)))
        goto end;

    /* download it */
    bTempfile = TRUE;
    CDownloadDialog_Constructor(Dlg, &bCancelled, IID_PPV_ARG(IBindStatusCallback, &dl));

    if (dl == NULL)
        goto end;

    switch(SettingsInfo.Proxy)
    {
        case 0: /* preconfig */
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            break;
        case 1: /* direct (no proxy) */
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            break;
        case 2: /* use proxy */
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PROXY, SettingsInfo.szProxyServer, SettingsInfo.szNoProxyFor, 0);
            break;
        default: /* preconfig */
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            break;
    }

    if (!hOpen)
        goto end;

    hFile = InternetOpenUrlW(hOpen, AppInfo->szUrlDownload, NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_KEEP_CONNECTION, 0);
    if (!hFile)
        goto end;

    if (!HttpQueryInfoW(hFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwStatusLen, NULL))
        goto end;

    if(dwStatus != HTTP_STATUS_OK)
    {
        WCHAR szMsgText[MAX_STR_LEN];

        if (!LoadStringW(hInst, IDS_UNABLE_TO_DOWNLOAD, szMsgText, sizeof(szMsgText) / sizeof(WCHAR)))
            goto end;

        MessageBoxW(hMainWnd, szMsgText, NULL, MB_OK | MB_ICONERROR);
        goto end;
    }

    dwStatusLen = sizeof(dwStatus);

    memset(&urlComponents, 0, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);

    if(FAILED(StringCbLengthW(AppInfo->szUrlDownload, sizeof(AppInfo->szUrlDownload), &urlLength)))
        goto end;
    
    urlComponents.dwSchemeLength = urlLength*sizeof(WCHAR);
    urlComponents.lpszScheme = (PWSTR)malloc(urlComponents.dwSchemeLength);
    
    if(!InternetCrackUrlW(AppInfo->szUrlDownload, urlLength+1, ICU_DECODE | ICU_ESCAPE, &urlComponents))
        goto end;
    
    if(urlComponents.nScheme == INTERNET_SCHEME_HTTP || urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
        HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwStatus, 0);

    if(urlComponents.nScheme == INTERNET_SCHEME_FTP)
        dwContentLen = FtpGetFileSize(hFile, &dwStatus);

    free(urlComponents.lpszScheme);

    hOut = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

    if (hOut == INVALID_HANDLE_VALUE)
        goto end;

    do
    {
        if (!InternetReadFile(hFile, lpBuffer, _countof(lpBuffer), &dwBytesRead)) goto end;
        if (!WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL)) goto end;
        dwCurrentBytesRead += dwBytesRead;
        dl->OnProgress(dwCurrentBytesRead, dwContentLen, 0, AppInfo->szUrlDownload);
    }
    while (dwBytesRead);

    CloseHandle(hOut);
    hOut = INVALID_HANDLE_VALUE;

    if (bCancelled)
        goto end;

    ShowWindow(Dlg, SW_HIDE);

    /* run it */
    if (!bCab)
        ShellExecuteW( NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL );

end:
    if (hOut != INVALID_HANDLE_VALUE)
        CloseHandle(hOut);

    InternetCloseHandle(hFile);
    InternetCloseHandle(hOpen);

    if (bTempfile)
    {
        if (bCancelled || (SettingsInfo.bDelInstaller && !bCab))
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

            hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
            if (hIcon)
            {
                SendMessageW(Dlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
                SendMessageW(Dlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
            }

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
            if (hIcon) DestroyIcon(hIcon);
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

    WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_INSTALL, AppInfo->szName);

    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
               hMainWnd,
               DownloadDlgProc);

    return TRUE;
}

VOID
DownloadApplicationsDB(LPCWSTR lpUrl)
{
    APPLICATION_INFO IntInfo;

    ZeroMemory(&IntInfo, sizeof(APPLICATION_INFO));
    if (FAILED(StringCbCopyW(IntInfo.szUrlDownload,
                             sizeof(IntInfo.szUrlDownload),
                             lpUrl)))
    {
        return;
    }

    AppInfo = &IntInfo;

    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
               hMainWnd,
               DownloadDlgProc);
}

