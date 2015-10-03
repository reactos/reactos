/* PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/rapps/loaddlg.c
 * PURPOSE:     Displaying a download dialog
 * COPYRIGHT:   Copyright 2001 John R. Sheets (for CodeWeavers)
 *              Copyright 2004 Mike McCormack (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp (gvg@reactos.org)
 *              Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
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
#include <wininet.h>
#include <shellapi.h>
#include <windowsx.h>

static PAPPLICATION_INFO AppInfo;

typedef struct _IBindStatusCallbackImpl
{
    const IBindStatusCallbackVtbl *vtbl;
    LONG ref;
    HWND hDialog;
    BOOL *pbCancelled;
    BOOL UrlHasBeenCopied;
    WCHAR ProgressText[MAX_PATH];
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

    Item = GetDlgItem(This->hDialog, IDC_DOWNLOAD_PROGRESS);
    if (Item && ulProgressMax)
    {
        WCHAR szProgress[100];
        WCHAR szProgressMax[100];
        UINT uiPercentage = ((ULONGLONG)ulProgress * 100) / ulProgressMax;

        /* send the current progress to the progress bar */
        SendMessageW(Item, PBM_SETPOS, uiPercentage, 0);

        /* format the bits and bytes into pretty and accesible units... */
        StrFormatByteSizeW(ulProgress, szProgress, sizeof(szProgress));
        StrFormatByteSizeW(ulProgressMax, szProgressMax, sizeof(szProgressMax));

        /* ...and post all of it to our subclassed progress bar text subroutine */
        swprintf(This->ProgressText, L"%u%% — %s / %s", uiPercentage, szProgress, szProgressMax);
        SendMessageW(Item, WM_SETTEXT, 0, (LPARAM)This->ProgressText);
    }

    Item = GetDlgItem(This->hDialog, IDC_DOWNLOAD_STATUS);
    if (Item && szStatusText && wcslen(szStatusText) > 0 && This->UrlHasBeenCopied == FALSE)
    {
        DWORD len = wcslen(szStatusText) * sizeof(WCHAR);
        PWSTR buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);

        if (buf)
        {
            /* beautify our url for display purposes */
            InternetCanonicalizeUrl(szStatusText, buf, &len, ICU_DECODE | ICU_NO_ENCODE);

            /* paste it into our dialog, free the temp buffer
               and don't do it again in this instance */
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM)buf);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        else
        {
            /* our computer is old and rusty and does not have enough ram for this,
               use the ugly version and call it a day */
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM)szStatusText);
        }

        This->UrlHasBeenCopied = TRUE;
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
    if (!This)
        return NULL;

    This->vtbl = &dlVtbl;
    This->ref = 1;
    This->hDialog = Dlg;
    This->pbCancelled = pbCancelled;

    return (IBindStatusCallback*) This;
}

#ifdef USE_CERT_PINNING
static BOOL CertIsValid(HINTERNET hInternet, LPWSTR lpszHostName)
{
    HINTERNET hConnect;
    HINTERNET hRequest;
    DWORD certInfoLength;
    BOOL Ret = FALSE;
    INTERNET_CERTIFICATE_INFOW certInfo;

    hConnect = InternetConnectW(hInternet, lpszHostName, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0);
    if (hConnect)
    {
        hRequest = HttpOpenRequestW(hConnect, L"HEAD", NULL, NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
        if (hRequest != NULL)
        {
            Ret = HttpSendRequestW(hRequest, L"", 0, NULL, 0);
            if (Ret)
            {
                certInfoLength = sizeof(INTERNET_CERTIFICATE_INFOW);
                Ret = InternetQueryOptionW(hRequest,
                                           INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                                           &certInfo,
                                           &certInfoLength);
                if (Ret)
                {
                    if (certInfo.lpszEncryptionAlgName)
                        LocalFree(certInfo.lpszEncryptionAlgName);
                    if (certInfo.lpszIssuerInfo)
                    {
                        if (strcmp((LPSTR)certInfo.lpszIssuerInfo, CERT_ISSUER_INFO) != 0)
                            Ret = FALSE;
                        LocalFree(certInfo.lpszIssuerInfo);
                    }
                    if (certInfo.lpszProtocolName)
                        LocalFree(certInfo.lpszProtocolName);
                    if (certInfo.lpszSignatureAlgName)
                        LocalFree(certInfo.lpszSignatureAlgName);
                    if (certInfo.lpszSubjectInfo)
                    {
                        if (strcmp((LPSTR)certInfo.lpszSubjectInfo, CERT_SUBJECT_INFO) != 0)
                            Ret = FALSE;
                        LocalFree(certInfo.lpszSubjectInfo);
                    }
                }
            }
            InternetCloseHandle(hRequest);
        }
    }
    return Ret;
}
#endif

static
DWORD WINAPI
ThreadFunc(LPVOID Context)
{
    IBindStatusCallback *dl = NULL;
    WCHAR path[MAX_PATH];
    PWSTR p, q;
    HWND Dlg = (HWND) Context;
    DWORD dwContentLen, dwBytesWritten, dwBytesRead, dwStatus;
    DWORD dwCurrentBytesRead = 0;
    DWORD dwStatusLen = sizeof(dwStatus);
    BOOL bCancelled = FALSE;
    BOOL bTempfile = FALSE;
    BOOL bCab = FALSE;
    HINTERNET hOpen = NULL;
    HINTERNET hFile = NULL;
    HANDLE hOut = INVALID_HANDLE_VALUE;
    unsigned char lpBuffer[4096];
    const LPWSTR lpszAgent = L"RApps/1.0";
    URL_COMPONENTS urlComponents;
    size_t urlLength, filenameLength;

    /* build the path for the download */
    p = wcsrchr(AppInfo->szUrlDownload, L'/');
    q = wcsrchr(AppInfo->szUrlDownload, L'?');

    /* do we have a final slash separator? */
    if (!p)
        goto end;

    /* prepare the tentative length of the filename, maybe we've to remove part of it later on */
    filenameLength = wcslen(p) * sizeof(WCHAR);

    /* do we have query arguments in the target URL after the filename? account for them
      (e.g. https://example.org/myfile.exe?no_adware_plz) */
    if (q && q > p && (q - p) > 0)
        filenameLength -= wcslen(q - 1) * sizeof(WCHAR);

    /* is this URL an update package for RAPPS? if so store it in a different place */
    if (wcscmp(AppInfo->szUrlDownload, APPLICATION_DATABASE_URL) == 0)
    {
        bCab = TRUE;
        if (!GetStorageDirectory(path, _countof(path)))
            goto end;
    }
    else
    {
        if (FAILED(StringCbCopyW(path, sizeof(path),  SettingsInfo.szDownloadDir)))
            goto end;
    }

    /* is the path valid? can we access it? */
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(path, NULL))
            goto end;
    }

    /* append a \ to the provided file system path, and the filename portion from the URL after that */
    if (FAILED(StringCbCatW(path, sizeof(path), L"\\")))
        goto end;
    if (FAILED(StringCbCatNW(path, sizeof(path), p + 1, filenameLength)))
        goto end;

    /* create an async download context for it */
    bTempfile = TRUE;
    dl = CreateDl(Context, &bCancelled);

    if (dl == NULL)
        goto end;

    /* FIXME: this should just be using the system-wide proxy settings */
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

    hFile = InternetOpenUrlW(hOpen, AppInfo->szUrlDownload, NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);
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
    urlComponents.lpszScheme = malloc(urlComponents.dwSchemeLength);
    urlComponents.dwHostNameLength = urlLength*sizeof(WCHAR);
    urlComponents.lpszHostName = malloc(urlComponents.dwHostNameLength);

    if(!InternetCrackUrlW(AppInfo->szUrlDownload, urlLength+1, ICU_DECODE | ICU_ESCAPE, &urlComponents))
        goto end;

    if(urlComponents.nScheme == INTERNET_SCHEME_HTTP || urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
        HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwStatus, 0);

    if(urlComponents.nScheme == INTERNET_SCHEME_FTP)
        dwContentLen = FtpGetFileSize(hFile, &dwStatus);

#ifdef USE_CERT_PINNING
    /* are we using HTTPS to download the RAPPS update package? check if the certificate is original */
    if ((urlComponents.nScheme == INTERNET_SCHEME_HTTPS) &&
        (wcscmp(AppInfo->szUrlDownload, APPLICATION_DATABASE_URL) == 0) &&
        (!CertIsValid(hOpen, urlComponents.lpszHostName)))
    {
        WCHAR szMsgText[MAX_STR_LEN];

        if (!LoadStringW(hInst, IDS_CERT_DOES_NOT_MATCH, szMsgText, sizeof(szMsgText) / sizeof(WCHAR)))
            goto end;

        MessageBoxW(Dlg, szMsgText, NULL, MB_OK | MB_ICONERROR);
        goto end;
    }
#endif

    free(urlComponents.lpszScheme);
    free(urlComponents.lpszHostName);

    hOut = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

    if (hOut == INVALID_HANDLE_VALUE)
        goto end;

    do
    {
        if (!InternetReadFile(hFile, lpBuffer, _countof(lpBuffer), &dwBytesRead)) goto end;
        if (!WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL)) goto end;
        dwCurrentBytesRead += dwBytesRead;
        IBindStatusCallback_OnProgress(dl, dwCurrentBytesRead, dwContentLen, 0, AppInfo->szUrlDownload);
    }
    while (dwBytesRead && !bCancelled);

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

    if (dl)
        IBindStatusCallback_Release(dl);

    if (bTempfile)
    {
        if (bCancelled || (SettingsInfo.bDelInstaller && !bCab))
            DeleteFileW(path);
    }

    EndDialog(Dlg, 0);

    return 0;
}


LRESULT CALLBACK
DownloadProgressProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static WCHAR szProgressText[MAX_STR_LEN] = {0};

    switch (uMsg)
    {
        case WM_SETTEXT:
        {
            if (lParam)
                wcscpy(szProgressText, (WCHAR *)lParam);
        }

        case WM_ERASEBKGND:
        case WM_PAINT:
        {
            PAINTSTRUCT  ps;
            HDC hDC = BeginPaint(hWnd, &ps), hdcMem;
            HBITMAP hbmMem;
            HANDLE hOld;
            RECT myRect;
            UINT win_width, win_height;

            GetClientRect(hWnd, &myRect);

            /* grab the progress bar rect size */
            win_width  = myRect.right - myRect.left;
            win_height = myRect.bottom - myRect.top;

            /* create an off-screen DC for double-buffering */
            hdcMem = CreateCompatibleDC(hDC);
            hbmMem = CreateCompatibleBitmap(hDC, win_width, win_height);

            hOld = SelectObject(hdcMem, hbmMem);

            /* call the original draw code and redirect it to our memory buffer */
            DefSubclassProc(hWnd, uMsg, (WPARAM)hdcMem, lParam);

            /* draw our nifty progress text over it */
            SelectFont(hdcMem, GetStockFont(DEFAULT_GUI_FONT));
            DrawShadowText(hdcMem, szProgressText, wcslen(szProgressText),
                           &myRect,
                           DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE,
                           GetSysColor(COLOR_CAPTIONTEXT),
                           GetSysColor(COLOR_3DSHADOW),
                           1, 1);

            /* transfer the off-screen DC to the screen */
            BitBlt(hDC, 0, 0, win_width, win_height, hdcMem, 0, 0, SRCCOPY);

            /* free the off-screen DC */
            SelectObject(hdcMem, hOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }

        /* Raymond Chen says that we should safely unsubclass all the things!
          (http://blogs.msdn.com/b/oldnewthing/archive/2003/11/11/55653.aspx) */
        case WM_NCDESTROY:
        {
            ZeroMemory(szProgressText, MAX_STR_LEN);
            RemoveWindowSubclass(hWnd, DownloadProgressProc, uIdSubclass);
        }

        default:
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}

static
INT_PTR CALLBACK
DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE Thread;
    DWORD ThreadId;
    HWND Item;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HICON hIconSm = NULL, hIconBg = NULL;

            hIconBg = (HICON)GetClassLongPtr(hMainWnd, GCLP_HICON);
            hIconSm = (HICON)GetClassLongPtr(hMainWnd, GCLP_HICONSM);

            if (hIconBg && hIconSm)
            {
                SendMessageW(Dlg, WM_SETICON, ICON_BIG, (LPARAM) hIconBg);
                SendMessageW(Dlg, WM_SETICON, ICON_SMALL, (LPARAM) hIconSm);
            }

            SetWindowLongPtrW(Dlg, GWLP_USERDATA, 0);
            Item = GetDlgItem(Dlg, IDC_DOWNLOAD_PROGRESS);
            if (Item)
            {
                /* initialize the default values for our nifty progress bar
                   and subclass it so that it learns to print a status text */
                SendMessageW(Item, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                SendMessageW(Item, PBM_SETPOS, 0, 0);

                SetWindowSubclass(Item, DownloadProgressProc, 0, 0);
            }

            /* add a neat placeholder until the download URL is retrieved */
            Item = GetDlgItem(Dlg, IDC_DOWNLOAD_STATUS);
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) L"• • •");

            Thread = CreateThread(NULL, 0, ThreadFunc, Dlg, 0, &ThreadId);
            if (!Thread)
                return FALSE;

            CloseHandle(Thread);
            return TRUE;
        }
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

    WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_INSTALL, AppInfo->szName);

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

