/* PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/rapps/loaddlg.cpp
 * PURPOSE:     Displaying a download dialog
 * COPYRIGHT:   Copyright 2001 John R. Sheets             (for CodeWeavers)
 *              Copyright 2004 Mike McCormack             (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp             (gvg@reactos.org)
 *              Copyright 2009 Dmitry Chapyshev           (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov     (chaez.san@gmail.com)
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
#include <windowsx.h>

class CDownloadDialog :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBindStatusCallback
{
    HWND m_hDialog;
    PBOOL m_pbCancelled;
    BOOL m_UrlHasBeenCopied;

public:
    ~CDownloadDialog()
    {
        DestroyWindow(m_hDialog);
    }

    HRESULT Initialize(HWND Dlg, BOOL *pbCancelled)
    {
        m_hDialog = Dlg;
        m_pbCancelled = pbCancelled;
        m_UrlHasBeenCopied = FALSE;
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

        Item = GetDlgItem(m_hDialog, IDC_DOWNLOAD_PROGRESS);
        if (Item && ulProgressMax)
        {
            WCHAR szProgress[100];
            WCHAR szProgressMax[100];
            UINT uiPercentage = ((ULONGLONG) ulProgress * 100) / ulProgressMax;

            /* send the current progress to the progress bar */
            SendMessageW(Item, PBM_SETPOS, uiPercentage, 0);

            /* format the bits and bytes into pretty and accessible units... */
            StrFormatByteSizeW(ulProgress, szProgress, _countof(szProgress));
            StrFormatByteSizeW(ulProgressMax, szProgressMax, _countof(szProgressMax));

            /* ...and post all of it to our subclassed progress bar text subroutine */
            ATL::CStringW m_ProgressText;
            m_ProgressText.Format(L"%u%% \x2014 %ls / %ls",
                                  uiPercentage,
                                  szProgress,
                                  szProgressMax);
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) m_ProgressText.GetString());
        }

        Item = GetDlgItem(m_hDialog, IDC_DOWNLOAD_STATUS);
        if (Item && szStatusText && wcslen(szStatusText) > 0 && m_UrlHasBeenCopied == FALSE)
        {
            DWORD len = wcslen(szStatusText) + 1;
            ATL::CStringW buf;

            /* beautify our url for display purposes */
            if (!InternetCanonicalizeUrlW(szStatusText, buf.GetBuffer(len), &len, ICU_DECODE | ICU_NO_ENCODE))
            {
                /* just use the original */
                buf.ReleaseBuffer();
                buf = szStatusText;
            }
            else
            {
                buf.ReleaseBuffer();
            }

            /* paste it into our dialog and don't do it again in this instance */
            SendMessageW(Item, WM_SETTEXT, 0, (LPARAM) buf.GetString());
            m_UrlHasBeenCopied = TRUE;
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
                certInfoLength = sizeof(certInfo);
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
                        if (strcmp((LPSTR) certInfo.lpszIssuerInfo, CERT_ISSUER_INFO) != 0)
                            Ret = FALSE;
                        LocalFree(certInfo.lpszIssuerInfo);
                    }
                    if (certInfo.lpszProtocolName)
                        LocalFree(certInfo.lpszProtocolName);
                    if (certInfo.lpszSignatureAlgName)
                        LocalFree(certInfo.lpszSignatureAlgName);
                    if (certInfo.lpszSubjectInfo)
                    {
                        if (strcmp((LPSTR) certInfo.lpszSubjectInfo, CERT_SUBJECT_INFO) != 0)
                            Ret = FALSE;
                        LocalFree(certInfo.lpszSubjectInfo);
                    }
                }
            }
            InternetCloseHandle(hRequest);
        }
        InternetCloseHandle(hConnect);
    }
    return Ret;
}
#endif

inline VOID
MessageBox_LoadString(HWND hMainWnd, INT StringID)
{
    ATL::CString szMsgText;
    if (szMsgText.LoadStringW(hInst, StringID))
        MessageBoxW(hMainWnd, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
}

// DownloadManager

PAPPLICATION_INFO DownloadManager::AppInfo = NULL;

INT_PTR CALLBACK DownloadManager::DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HANDLE Thread;
    DWORD ThreadId;
    HWND Item;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HICON hIconSm, hIconBg;
        WCHAR szCaption[MAX_PATH];
        ATL::CStringW szNewCaption;

        hIconBg = (HICON) GetClassLongW(hMainWnd, GCLP_HICON);
        hIconSm = (HICON) GetClassLongW(hMainWnd, GCLP_HICONSM);

        if (hIconBg && hIconSm)
        {
            SendMessageW(Dlg, WM_SETICON, ICON_BIG, (LPARAM) hIconBg);
            SendMessageW(Dlg, WM_SETICON, ICON_SMALL, (LPARAM) hIconSm);
        }

        // Change caption to show the currently downloaded app
        GetWindowTextW(Dlg, szCaption, MAX_PATH);
        szNewCaption.Format(szCaption, AppInfo->szName.GetString());
        SetWindowTextW(Dlg, szNewCaption.GetString());

        SetWindowLongW(Dlg, GWLP_USERDATA, 0);
        Item = GetDlgItem(Dlg, IDC_DOWNLOAD_PROGRESS);
        if (Item)
        {
            // initialize the default values for our nifty progress bar
            // and subclass it so that it learns to print a status text 
            SendMessageW(Item, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessageW(Item, PBM_SETPOS, 0, 0);
            
            SetWindowSubclass(Item, DownloadProgressProc, 0, 0);
        }

        // add a neat placeholder until the download URL is retrieved
        SetDlgItemTextW(Dlg, IDC_DOWNLOAD_STATUS, L"\x2022 \x2022 \x2022");

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
        DestroyWindow(Dlg);
        return TRUE;

    default:
        return FALSE;
    }
}

LRESULT CALLBACK DownloadManager::DownloadProgressProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static ATL::CStringW szProgressText;

    switch (uMsg)
    {
    case WM_SETTEXT:
    {
        if (lParam)
        {
            szProgressText = (PCWSTR) lParam;
        }
        return TRUE;
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
        win_width = myRect.right - myRect.left;
        win_height = myRect.bottom - myRect.top;

        /* create an off-screen DC for double-buffering */
        hdcMem = CreateCompatibleDC(hDC);
        hbmMem = CreateCompatibleBitmap(hDC, win_width, win_height);

        hOld = SelectObject(hdcMem, hbmMem);

        /* call the original draw code and redirect it to our memory buffer */
        DefSubclassProc(hWnd, uMsg, (WPARAM) hdcMem, lParam);

        /* draw our nifty progress text over it */
        SelectFont(hdcMem, GetStockFont(DEFAULT_GUI_FONT));
        DrawShadowText(hdcMem, szProgressText.GetString(), szProgressText.GetLength(),
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
        szProgressText.Empty();
        RemoveWindowSubclass(hWnd, DownloadProgressProc, uIdSubclass);
    }
    /* Fall-through */
    default:
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}

DWORD WINAPI DownloadManager::ThreadFunc(LPVOID Context)
{
    CComPtr<IBindStatusCallback> dl;
    ATL::CStringW Path;
    PWSTR p, q;
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
    LPCWSTR lpszAgent = L"RApps/1.0";
    URL_COMPONENTS urlComponents;
    size_t urlLength, filenameLength;

    if (!AppInfo)
    {
        MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD);
        goto end;
    }

    /* build the path for the download */
    p = wcsrchr(AppInfo->szUrlDownload.GetString(), L'/');
    q = wcsrchr(AppInfo->szUrlDownload.GetString(), L'?');

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
    if (AppInfo->szUrlDownload == APPLICATION_DATABASE_URL)
    {
        bCab = TRUE;
        if (!GetStorageDirectory(Path))
            goto end;
    }
    else
    {
        Path = SettingsInfo.szDownloadDir;
    }

    /* is the path valid? can we access it? */
    if (GetFileAttributesW(Path.GetString()) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(Path.GetString(), NULL))
            goto end;
    }

    /* append a \ to the provided file system path, and the filename portion from the URL after that */
    Path.Format(L"\\%ls", (p + 1));

    if (!bCab && AppInfo->szSHA1[0] && GetFileAttributesW(Path.GetString()) != INVALID_FILE_ATTRIBUTES)
    {
        /* only open it in case of total correctness */
        if (VerifyInteg(AppInfo->szSHA1, Path))
            goto run;
    }

    /* download it */
    bTempfile = TRUE;
    CDownloadDialog_Constructor(Dlg, &bCancelled, IID_PPV_ARG(IBindStatusCallback, &dl));

    if (dl == NULL)
        goto end;

    /* FIXME: this should just be using the system-wide proxy settings */
    switch (SettingsInfo.Proxy)
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
    {
        MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD2);
        goto end;
    }

    if (!HttpQueryInfoW(hFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwStatusLen, NULL))
        goto end;

    if (dwStatus != HTTP_STATUS_OK)
    {
        MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD);
        goto end;
    }

    dwStatusLen = sizeof(dwStatus);

    memset(&urlComponents, 0, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);

    urlLength = AppInfo->szUrlDownload.GetLength();
    urlComponents.dwSchemeLength = urlLength + 1;
    urlComponents.lpszScheme = (LPWSTR) malloc(urlComponents.dwSchemeLength * sizeof(WCHAR));
    urlComponents.dwHostNameLength = urlLength + 1;
    urlComponents.lpszHostName = (LPWSTR) malloc(urlComponents.dwHostNameLength * sizeof(WCHAR));

    if (!InternetCrackUrlW(AppInfo->szUrlDownload, urlLength + 1, ICU_DECODE | ICU_ESCAPE, &urlComponents))
        goto end;

    if (urlComponents.nScheme == INTERNET_SCHEME_HTTP || urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
        HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwStatus, 0);

    if (urlComponents.nScheme == INTERNET_SCHEME_FTP)
        dwContentLen = FtpGetFileSize(hFile, &dwStatus);

#ifdef USE_CERT_PINNING
    /* are we using HTTPS to download the RAPPS update package? check if the certificate is original */
    if ((urlComponents.nScheme == INTERNET_SCHEME_HTTPS) &&
        (wcscmp(AppInfo->szUrlDownload, APPLICATION_DATABASE_URL) == 0) &&
        (!CertIsValid(hOpen, urlComponents.lpszHostName)))
    {
        MessageBox_LoadString(hMainWnd, IDS_CERT_DOES_NOT_MATCH);
        goto end;
    }
#endif

    free(urlComponents.lpszScheme);
    free(urlComponents.lpszHostName);

    hOut = CreateFileW(Path.GetString(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

    if (hOut == INVALID_HANDLE_VALUE)
        goto end;

    do
    {
        if (!InternetReadFile(hFile, lpBuffer, _countof(lpBuffer), &dwBytesRead))
        {
            MessageBox_LoadString(hMainWnd, IDS_INTERRUPTED_DOWNLOAD);
            goto end;
        }

        if (!WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL))
        {
            MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_WRITE);
            goto end;
        }

        dwCurrentBytesRead += dwBytesRead;
        dl->OnProgress(dwCurrentBytesRead, dwContentLen, 0, AppInfo->szUrlDownload);
    } while (dwBytesRead && !bCancelled);

    CloseHandle(hOut);
    hOut = INVALID_HANDLE_VALUE;

    if (bCancelled)
        goto end;

    /* if this thing isn't a RAPPS update and it has a SHA-1 checksum
    verify its integrity by using the native advapi32.A_SHA1 functions */
    if (!bCab && AppInfo->szSHA1[0] != 0)
    {
        ATL::CStringW szMsgText;

        /* change a few strings in the download dialog to reflect the verification process */
        if (!szMsgText.LoadStringW(hInst, IDS_INTEG_CHECK_TITLE))
            goto end;

        SetWindowText(Dlg, szMsgText.GetString());
        SendMessageW(GetDlgItem(Dlg, IDC_DOWNLOAD_STATUS), WM_SETTEXT, 0, (LPARAM) Path.GetString());

        /* this may take a while, depending on the file size */
        if (!VerifyInteg(AppInfo->szSHA1, Path.GetString()))
        {
            if (!szMsgText.LoadStringW(hInst, IDS_INTEG_CHECK_FAIL))
                goto end;

            MessageBoxW(Dlg, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
            goto end;
        }
    }

    ShowWindow(Dlg, SW_HIDE);

run:
    /* run it */
    if (!bCab)
        ShellExecuteW(NULL, L"open", Path.GetString(), NULL, NULL, SW_SHOWNORMAL);

end:
    if (hOut != INVALID_HANDLE_VALUE)
        CloseHandle(hOut);

    InternetCloseHandle(hFile);
    InternetCloseHandle(hOpen);

    if (bTempfile)
    {
        if (bCancelled || (SettingsInfo.bDelInstaller && !bCab))
            DeleteFileW(Path.GetString());
    }

    EndDialog(Dlg, 0);

    return 0;
}

//TODO: Maybe launch this (similar function) in a seperate thread, so the list could be updated
BOOL DownloadManager::DownloadListOfApplications(const ATL::CSimpleArray<PAPPLICATION_INFO>& AppsList)
{
    BOOL bResult = TRUE;

    for (INT i = 0; i < AppsList.GetSize(); ++i)
    {
        bResult = DownloadApplication(AppsList[i]) && bResult;
    }
    return bResult;
}

BOOL DownloadManager::DownloadApplication(PAPPLICATION_INFO pAppInfo)
{
    if (!pAppInfo)
    {
        return FALSE;
    }

    // Create a dialog and issue a download process
    AppInfo = pAppInfo;
    LaunchDownloadDialog();

    WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_INSTALL, AppInfo->szName.GetString());

    return TRUE;
}

VOID DownloadManager::DownloadApplicationsDB(LPCWSTR lpUrl)
{
    APPLICATION_INFO IntInfo;
    IntInfo.szUrlDownload = lpUrl;

    AppInfo = &IntInfo;

    LaunchDownloadDialog();
}

//TODO: Reuse the dialog
VOID DownloadManager::LaunchDownloadDialog()
{
    CreateDialogW(hInst,
                  MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
                  hMainWnd,
                  DownloadDlgProc);
}