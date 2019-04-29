/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/loaddlg.cpp
 * PURPOSE:     Displaying a download dialog
 * COPYRIGHT:   Copyright 2001 John R. Sheets             (for CodeWeavers)
 *              Copyright 2004 Mike McCormack             (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp             (gvg@reactos.org)
 *              Copyright 2009 Dmitry Chapyshev           (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov     (sanchaez@reactos.org)
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
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>

#include <rosctrls.h>
#include <windowsx.h>
#undef SubclassWindow

#include "rosui.h"
#include "dialogs.h"
#include "misc.h"

#ifdef USE_CERT_PINNING
#define CERT_ISSUER_INFO "US\r\nLet's Encrypt\r\nLet's Encrypt Authority X3"
#define CERT_SUBJECT_INFO "rapps.reactos.org"
#endif

enum DownloadStatus
{
    DLSTATUS_WAITING = IDS_STATUS_WAITING,
    DLSTATUS_DOWNLOADING = IDS_STATUS_DOWNLOADING,
    DLSTATUS_WAITING_INSTALL = IDS_STATUS_DOWNLOADED,
    DLSTATUS_INSTALLING = IDS_STATUS_INSTALLING,
    DLSTATUS_INSTALLED = IDS_STATUS_INSTALLED,
    DLSTATUS_FINISHED = IDS_STATUS_FINISHED
};

ATL::CStringW LoadStatusString(DownloadStatus StatusParam)
{
    ATL::CStringW szString;
    szString.LoadStringW(StatusParam);
    return szString;
}

struct DownloadInfo
{
    DownloadInfo() {}
    DownloadInfo(const CAvailableApplicationInfo& AppInfo)
        :szUrl(AppInfo.m_szUrlDownload), szName(AppInfo.m_szName), szSHA1(AppInfo.m_szSHA1)
    {
    }

    ATL::CStringW szUrl;
    ATL::CStringW szName;
    ATL::CStringW szSHA1;
};

struct DownloadParam
{
    DownloadParam() : Dialog(NULL), AppInfo(), szCaption(NULL) {}
    DownloadParam(HWND dlg, const ATL::CSimpleArray<DownloadInfo> &info, LPCWSTR caption)
        : Dialog(dlg), AppInfo(info), szCaption(caption)
    {
    }

    HWND Dialog;
    ATL::CSimpleArray<DownloadInfo> AppInfo;
    LPCWSTR szCaption;
};


class CDownloaderProgress
    : public CWindowImpl<CDownloaderProgress, CWindow, CControlWinTraits>
{
    ATL::CStringW m_szProgressText;

public:
    CDownloaderProgress()
    {
    }

    VOID SetMarquee(BOOL Enable)
    {
        if (Enable)
            ModifyStyle(0, PBS_MARQUEE, 0);
        else
            ModifyStyle(PBS_MARQUEE, 0, 0);

        SendMessage(PBM_SETMARQUEE, Enable, 0);
    }

    VOID SetProgress(ULONG ulProgress, ULONG ulProgressMax)
    {
        WCHAR szProgress[100];

        /* format the bits and bytes into pretty and accessible units... */
        StrFormatByteSizeW(ulProgress, szProgress, _countof(szProgress));

        /* use our subclassed progress bar text subroutine */
        ATL::CStringW ProgressText;

        if (ulProgressMax)
        {
            /* total size is known */
            WCHAR szProgressMax[100];
            UINT uiPercentage = ((ULONGLONG) ulProgress * 100) / ulProgressMax;

            /* send the current progress to the progress bar */
            SendMessage(PBM_SETPOS, uiPercentage, 0);

            /* format total download size */
            StrFormatByteSizeW(ulProgressMax, szProgressMax, _countof(szProgressMax));

            /* generate the text on progress bar */
            ProgressText.Format(L"%u%% \x2014 %ls / %ls",
                                uiPercentage,
                                szProgress,
                                szProgressMax);
        }
        else
        {
            /* send the current progress to the progress bar */
            SendMessage(PBM_SETPOS, 0, 0);

            /* total size is not known, display only current size */
            ProgressText.Format(L"%ls...", szProgress);
        }

        /* and finally display it */
        SendMessage(WM_SETTEXT, 0, (LPARAM) ProgressText.GetString());
    }

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        PAINTSTRUCT  ps;
        HDC hDC = BeginPaint(&ps), hdcMem;
        HBITMAP hbmMem;
        HANDLE hOld;
        RECT myRect;
        UINT win_width, win_height;

        GetClientRect(&myRect);

        /* grab the progress bar rect size */
        win_width = myRect.right - myRect.left;
        win_height = myRect.bottom - myRect.top;

        /* create an off-screen DC for double-buffering */
        hdcMem = CreateCompatibleDC(hDC);
        hbmMem = CreateCompatibleBitmap(hDC, win_width, win_height);

        hOld = SelectObject(hdcMem, hbmMem);

        /* call the original draw code and redirect it to our memory buffer */
        DefWindowProc(uMsg, (WPARAM) hdcMem, lParam);

        /* draw our nifty progress text over it */
        SelectFont(hdcMem, GetStockFont(DEFAULT_GUI_FONT));
        DrawShadowText(hdcMem, m_szProgressText.GetString(), m_szProgressText.GetLength(),
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

        EndPaint(&ps);
        return 0;
    }

    LRESULT OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        if (lParam)
        {
            m_szProgressText = (PCWSTR) lParam;
        }
        return 0;
    }

    BEGIN_MSG_MAP(CDownloaderProgress)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnPaint)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SETTEXT, OnSetText)
    END_MSG_MAP()
};

class CDowloadingAppsListView
    : public CListView
{
public:
    HWND Create(HWND hwndParent)
    {
        RECT r = {10, 150, 320, 350};
        const DWORD style = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL
            | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER;

        HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE);

        AddColumn(0, 150, LVCFMT_LEFT);
        AddColumn(1, 120, LVCFMT_LEFT);

        return hwnd;
    }

    VOID LoadList(ATL::CSimpleArray<DownloadInfo> arrInfo)
    {
        for (INT i = 0; i < arrInfo.GetSize(); ++i)
        {
            AddRow(i, arrInfo[i].szName.GetString(), DLSTATUS_WAITING);
        }
    }

    VOID SetDownloadStatus(INT ItemIndex, DownloadStatus Status)
    {
        ATL::CStringW szBuffer = LoadStatusString(Status);
        SetItemText(ItemIndex, 1, szBuffer.GetString());
    }

    BOOL AddItem(INT ItemIndex, LPWSTR lpText)
    {
        LVITEMW Item;

        ZeroMemory(&Item, sizeof(Item));

        Item.mask = LVIF_TEXT | LVIF_STATE;
        Item.pszText = lpText;
        Item.iItem = ItemIndex;

        return InsertItem(&Item);
    }

    VOID AddRow(INT RowIndex, LPCWSTR szAppName, const DownloadStatus Status)
    {
        ATL::CStringW szStatus = LoadStatusString(Status);
        AddItem(RowIndex,
                const_cast<LPWSTR>(szAppName));
        SetDownloadStatus(RowIndex, Status);
    }

    BOOL AddColumn(INT Index, INT Width, INT Format)
    {
        LVCOLUMNW Column;
        ZeroMemory(&Column, sizeof(Column));

        Column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
        Column.iSubItem = Index;
        Column.cx = Width;
        Column.fmt = Format;

        return (InsertColumn(Index, &Column) == -1) ? FALSE : TRUE;
    }
};

#ifdef USE_CERT_PINNING
typedef CHeapPtr<char, CLocalAllocator> CLocalPtr;

static BOOL CertGetSubjectAndIssuer(HINTERNET hFile, CLocalPtr& subjectInfo, CLocalPtr& issuerInfo)
{
    DWORD certInfoLength;
    INTERNET_CERTIFICATE_INFOA certInfo;
    DWORD size, flags;

    size = sizeof(flags);
    if (!InternetQueryOptionA(hFile, INTERNET_OPTION_SECURITY_FLAGS, &flags, &size))
    {
        return FALSE;
    }

    if (!flags & SECURITY_FLAG_SECURE)
    {
        return FALSE;
    }

    /* Despite what the header indicates, the implementation of INTERNET_CERTIFICATE_INFO is not Unicode-aware. */
    certInfoLength = sizeof(certInfo);
    if (!InternetQueryOptionA(hFile,
                              INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT,
                              &certInfo,
                              &certInfoLength))
    {
        return FALSE;
    }

    subjectInfo.Attach(certInfo.lpszSubjectInfo);
    issuerInfo.Attach(certInfo.lpszIssuerInfo);

    if (certInfo.lpszProtocolName)
        LocalFree(certInfo.lpszProtocolName);
    if (certInfo.lpszSignatureAlgName)
        LocalFree(certInfo.lpszSignatureAlgName);
    if (certInfo.lpszEncryptionAlgName)
        LocalFree(certInfo.lpszEncryptionAlgName);

    return certInfo.lpszSubjectInfo && certInfo.lpszIssuerInfo;
}
#endif

inline VOID MessageBox_LoadString(HWND hMainWnd, INT StringID)
{
    ATL::CStringW szMsgText;
    if (szMsgText.LoadStringW(StringID))
    {
        MessageBoxW(hMainWnd, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
    }
}

// Download dialog (loaddlg.cpp)
class CDownloadManager
{
    static ATL::CSimpleArray<DownloadInfo> AppsToInstallList;
    static CDowloadingAppsListView DownloadsListView;
    static CDownloaderProgress ProgressBar;
    static BOOL bCancelled;
    static VOID UpdateProgress(HWND hDlg, ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
public:
    static VOID Add(DownloadInfo info);
    static VOID Download(const DownloadInfo& DLInfo, BOOL bIsModal = FALSE);
    static INT_PTR CALLBACK DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ThreadFunc(LPVOID Context);
    static BOOL DownloadListOfApplications(const ATL::CSimpleArray<CAvailableApplicationInfo>& AppsList, BOOL bIsModal = FALSE);
    static BOOL DownloadApplication(CAvailableApplicationInfo* pAppInfo, BOOL bIsModal = FALSE);
    static VOID DownloadApplicationsDB(LPCWSTR lpUrl);
    static VOID LaunchDownloadDialog(BOOL);
};


// CDownloadManager
ATL::CSimpleArray<DownloadInfo>         CDownloadManager::AppsToInstallList;
CDowloadingAppsListView                 CDownloadManager::DownloadsListView;
CDownloaderProgress                     CDownloadManager::ProgressBar;
BOOL                                    CDownloadManager::bCancelled;

VOID CDownloadManager::Add(DownloadInfo info)
{
    AppsToInstallList.Add(info);
}

VOID CDownloadManager::Download(const DownloadInfo &DLInfo, BOOL bIsModal)
{
    AppsToInstallList.RemoveAll();
    AppsToInstallList.Add(DLInfo);
    LaunchDownloadDialog(bIsModal);
}

INT_PTR CALLBACK CDownloadManager::DownloadDlgProc(HWND Dlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static WCHAR szCaption[MAX_PATH];

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HICON hIconSm, hIconBg;
        ATL::CStringW szTempCaption;

        bCancelled = FALSE;

        hIconBg = (HICON) GetClassLongPtrW(hMainWnd, GCLP_HICON);
        hIconSm = (HICON) GetClassLongPtrW(hMainWnd, GCLP_HICONSM);

        if (hIconBg && hIconSm)
        {
            SendMessageW(Dlg, WM_SETICON, ICON_BIG, (LPARAM) hIconBg);
            SendMessageW(Dlg, WM_SETICON, ICON_SMALL, (LPARAM) hIconSm);
        }

        SetWindowLongW(Dlg, GWLP_USERDATA, 0);
        HWND Item = GetDlgItem(Dlg, IDC_DOWNLOAD_PROGRESS);
        if (Item)
        {
            // initialize the default values for our nifty progress bar
            // and subclass it so that it learns to print a status text
            ProgressBar.SubclassWindow(Item);
            ProgressBar.SendMessage(PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            ProgressBar.SendMessage(PBM_SETPOS, 0, 0);
        }

        // Add a ListView
        HWND hListView = DownloadsListView.Create(Dlg);
        if (!hListView)
        {
            return FALSE;
        }
        DownloadsListView.LoadList(AppsToInstallList);

        // Get a dlg string for later use
        GetWindowTextW(Dlg, szCaption, _countof(szCaption));

        // Hide a placeholder from displaying
        szTempCaption = szCaption;
        szTempCaption.Replace(L"%ls", L"");
        SetWindowText(Dlg, szTempCaption.GetString());

        ShowWindow(Dlg, SW_SHOW);

        // Start download process
        DownloadParam *param = new DownloadParam(Dlg, AppsToInstallList, szCaption);
        DWORD ThreadId;
        HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, (LPVOID) param, 0, &ThreadId);

        if (!Thread)
        {
            return FALSE;
        }

        CloseHandle(Thread);
        AppsToInstallList.RemoveAll();
        return TRUE;
    }

    case WM_COMMAND:
        if (wParam == IDCANCEL)
        {
            bCancelled = TRUE;
            PostMessageW(Dlg, WM_CLOSE, 0, 0);
        }
        return FALSE;

    case WM_CLOSE:
        EndDialog(Dlg, 0);
        //DestroyWindow(Dlg);
        return TRUE;

    default:
        return FALSE;
    }
}

BOOL UrlHasBeenCopied;

VOID CDownloadManager::UpdateProgress(
    HWND hDlg,
    ULONG ulProgress,
    ULONG ulProgressMax,
    ULONG ulStatusCode,
    LPCWSTR szStatusText)
{
    HWND Item;

    ProgressBar.SetProgress(ulProgress, ulProgressMax);

    Item = GetDlgItem(hDlg, IDC_DOWNLOAD_STATUS);
    if (Item && szStatusText && wcslen(szStatusText) > 0 && UrlHasBeenCopied == FALSE)
    {
        SIZE_T len = wcslen(szStatusText) + 1;
        ATL::CStringW buf;
        DWORD dummyLen;

        /* beautify our url for display purposes */
        if (!InternetCanonicalizeUrlW(szStatusText, buf.GetBuffer(len), &dummyLen, ICU_DECODE | ICU_NO_ENCODE))
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
        UrlHasBeenCopied = TRUE;
    }
}

DWORD WINAPI CDownloadManager::ThreadFunc(LPVOID param)
{
    ATL::CStringW Path;
    PWSTR p, q;

    HWND hDlg = static_cast<DownloadParam*>(param)->Dialog;
    HWND Item;
    INT iAppId;

    ULONG dwContentLen, dwBytesWritten, dwBytesRead, dwStatus;
    ULONG dwCurrentBytesRead = 0;
    ULONG dwStatusLen = sizeof(dwStatus);

    BOOL bTempfile = FALSE;
    BOOL bCab = FALSE;

    HINTERNET hOpen = NULL;
    HINTERNET hFile = NULL;
    HANDLE hOut = INVALID_HANDLE_VALUE;

    unsigned char lpBuffer[4096];
    LPCWSTR lpszAgent = L"RApps/1.0";
    URL_COMPONENTS urlComponents;
    size_t urlLength, filenameLength;

    const ATL::CSimpleArray<DownloadInfo> &InfoArray = static_cast<DownloadParam*>(param)->AppInfo;
    LPCWSTR szCaption = static_cast<DownloadParam*>(param)->szCaption;
    ATL::CStringW szNewCaption;

    const DWORD dwUrlConnectFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION;

    if (InfoArray.GetSize() <= 0)
    {
        MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD);
        goto end;
    }

    for (iAppId = 0; iAppId < InfoArray.GetSize(); ++iAppId)
    {
        // Reset progress bar
        Item = GetDlgItem(hDlg, IDC_DOWNLOAD_PROGRESS);
        if (Item)
        {
            ProgressBar.SetMarquee(FALSE);
            ProgressBar.SendMessage(WM_SETTEXT, 0, (LPARAM) L"");
            ProgressBar.SendMessage(PBM_SETPOS, 0, 0);
        }

        // is this URL an update package for RAPPS? if so store it in a different place
        if (InfoArray[iAppId].szUrl == APPLICATION_DATABASE_URL)
        {
            bCab = TRUE;
            if (!GetStorageDirectory(Path))
                goto end;
        }
        else
        {
            bCab = FALSE;
            Path = SettingsInfo.szDownloadDir;
        }

        // Change caption to show the currently downloaded app
        if (!bCab)
        {
            szNewCaption.Format(szCaption, InfoArray[iAppId].szName.GetString());
        }
        else
        {
            szNewCaption.LoadStringW(IDS_DL_DIALOG_DB_DOWNLOAD_DISP);
        }

        SetWindowTextW(hDlg, szNewCaption.GetString());

        // build the path for the download
        p = wcsrchr(InfoArray[iAppId].szUrl.GetString(), L'/');
        q = wcsrchr(InfoArray[iAppId].szUrl.GetString(), L'?');

        // do we have a final slash separator?
        if (!p)
            goto end;

        // prepare the tentative length of the filename, maybe we've to remove part of it later on
        filenameLength = wcslen(p) * sizeof(WCHAR);

        /* do we have query arguments in the target URL after the filename? account for them
        (e.g. https://example.org/myfile.exe?no_adware_plz) */
        if (q && q > p && (q - p) > 0)
            filenameLength -= wcslen(q - 1) * sizeof(WCHAR);

        // is the path valid? can we access it?
        if (GetFileAttributesW(Path.GetString()) == INVALID_FILE_ATTRIBUTES)
        {
            if (!CreateDirectoryW(Path.GetString(), NULL))
                goto end;
        }

        // append a \ to the provided file system path, and the filename portion from the URL after that
        Path += L"\\";
        Path += (LPWSTR) (p + 1);

        if (!bCab && InfoArray[iAppId].szSHA1[0] && GetFileAttributesW(Path.GetString()) != INVALID_FILE_ATTRIBUTES)
        {
            // only open it in case of total correctness
            if (VerifyInteg(InfoArray[iAppId].szSHA1.GetString(), Path))
                goto run;
        }

        // Add the download URL
        SetDlgItemTextW(hDlg, IDC_DOWNLOAD_STATUS, InfoArray[iAppId].szUrl.GetString());

        DownloadsListView.SetDownloadStatus(iAppId, DLSTATUS_DOWNLOADING);

        // download it
        UrlHasBeenCopied = FALSE;
        bTempfile = TRUE;

        /* FIXME: this should just be using the system-wide proxy settings */
        switch (SettingsInfo.Proxy)
        {
        case 0: // preconfig
        default:
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
            break;
        case 1: // direct (no proxy)
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            break;
        case 2: // use proxy
            hOpen = InternetOpenW(lpszAgent, INTERNET_OPEN_TYPE_PROXY, SettingsInfo.szProxyServer, SettingsInfo.szNoProxyFor, 0);
            break;
        }

        if (!hOpen)
            goto end;

        dwStatusLen = sizeof(dwStatus);

        memset(&urlComponents, 0, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);

        urlLength = InfoArray[iAppId].szUrl.GetLength();
        urlComponents.dwSchemeLength = urlLength + 1;
        urlComponents.lpszScheme = (LPWSTR) malloc(urlComponents.dwSchemeLength * sizeof(WCHAR));
        urlComponents.dwHostNameLength = urlLength + 1;
        urlComponents.lpszHostName = (LPWSTR) malloc(urlComponents.dwHostNameLength * sizeof(WCHAR));

        if (!InternetCrackUrlW(InfoArray[iAppId].szUrl, urlLength + 1, ICU_DECODE | ICU_ESCAPE, &urlComponents))
            goto end;

        dwContentLen = 0;

        if (urlComponents.nScheme == INTERNET_SCHEME_HTTP || urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
        {
            hFile = InternetOpenUrlW(hOpen, InfoArray[iAppId].szUrl.GetString(), NULL, 0,
                                     dwUrlConnectFlags,
                                     0);
            if (!hFile)
            {
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD2);
                goto end;
            }

            // query connection
            if (!HttpQueryInfoW(hFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwStatusLen, NULL))
                goto end;

            if (dwStatus != HTTP_STATUS_OK)
            {
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD);
                goto end;
            }

            // query content length
            HttpQueryInfoW(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwStatusLen, NULL);
        }

        if (urlComponents.nScheme == INTERNET_SCHEME_FTP)
        {
            // force passive mode on FTP
            hFile = InternetOpenUrlW(hOpen, InfoArray[iAppId].szUrl.GetString(), NULL, 0,
                                     dwUrlConnectFlags | INTERNET_FLAG_PASSIVE,
                                     0);
            if (!hFile)
            {
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD2);
                goto end;
            }

            dwContentLen = FtpGetFileSize(hFile, &dwStatus);
        }

        if (!dwContentLen)
        {
            // content-length is not known, enable marquee mode
            ProgressBar.SetMarquee(TRUE);
        }

        free(urlComponents.lpszScheme);
        free(urlComponents.lpszHostName);

#ifdef USE_CERT_PINNING
        // are we using HTTPS to download the RAPPS update package? check if the certificate is original
        if ((urlComponents.nScheme == INTERNET_SCHEME_HTTPS) &&
            (wcscmp(InfoArray[iAppId].szUrl, APPLICATION_DATABASE_URL) == 0))
        {
            CLocalPtr subjectName, issuerName;
            CStringW szMsgText;
            bool bAskQuestion = false;
            if (!CertGetSubjectAndIssuer(hFile, subjectName, issuerName))
            {
                szMsgText.LoadStringW(IDS_UNABLE_TO_QUERY_CERT);
                bAskQuestion = true;
            }
            else
            {
                if (strcmp(subjectName, CERT_SUBJECT_INFO) ||
                    strcmp(issuerName, CERT_ISSUER_INFO))
                {
                    szMsgText.Format(IDS_MISMATCH_CERT_INFO, (char*)subjectName, (const char*)issuerName);
                    bAskQuestion = true;
                }
            }

            if (bAskQuestion)
            {
                if (MessageBoxW(hMainWnd, szMsgText.GetString(), NULL, MB_YESNO | MB_ICONERROR) != IDYES)
                {
                    goto end;
                }
            }
        }
#endif

        hOut = CreateFileW(Path.GetString(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);

        if (hOut == INVALID_HANDLE_VALUE)
            goto end;

        dwCurrentBytesRead = 0;
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
            UpdateProgress(hDlg, dwCurrentBytesRead, dwContentLen, 0, InfoArray[iAppId].szUrl.GetString());
        } while (dwBytesRead && !bCancelled);

        CloseHandle(hOut);
        hOut = INVALID_HANDLE_VALUE;

        if (bCancelled)
            goto end;

        if (!dwContentLen)
        {
            // set progress bar to 100%
            ProgressBar.SetMarquee(FALSE);

            dwContentLen = dwCurrentBytesRead;
            UpdateProgress(hDlg, dwCurrentBytesRead, dwContentLen, 0, InfoArray[iAppId].szUrl.GetString());
        }

        /* if this thing isn't a RAPPS update and it has a SHA-1 checksum
        verify its integrity by using the native advapi32.A_SHA1 functions */
        if (!bCab && InfoArray[iAppId].szSHA1[0] != 0)
        {
            ATL::CStringW szMsgText;

            // change a few strings in the download dialog to reflect the verification process
            if (!szMsgText.LoadStringW(IDS_INTEG_CHECK_TITLE))
                goto end;

            SetWindowTextW(hDlg, szMsgText.GetString());
            SendMessageW(GetDlgItem(hDlg, IDC_DOWNLOAD_STATUS), WM_SETTEXT, 0, (LPARAM) Path.GetString());

            // this may take a while, depending on the file size
            if (!VerifyInteg(InfoArray[iAppId].szSHA1.GetString(), Path.GetString()))
            {
                if (!szMsgText.LoadStringW(IDS_INTEG_CHECK_FAIL))
                    goto end;

                MessageBoxW(hDlg, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
                goto end;
            }
        }

run:
        DownloadsListView.SetDownloadStatus(iAppId, DLSTATUS_WAITING_INSTALL);

        // run it
        if (!bCab)
        {
            SHELLEXECUTEINFOW shExInfo = {0};
            shExInfo.cbSize = sizeof(shExInfo);
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExInfo.lpVerb = L"open";
            shExInfo.lpFile = Path.GetString();
            shExInfo.lpParameters = L"";
            shExInfo.nShow = SW_SHOW;

            if (ShellExecuteExW(&shExInfo))
            {
                //reflect installation progress in the titlebar
                //TODO: make a separate string with a placeholder to include app name?
                ATL::CStringW szMsgText = LoadStatusString(DLSTATUS_INSTALLING);
                SetWindowTextW(hDlg, szMsgText.GetString());

                DownloadsListView.SetDownloadStatus(iAppId, DLSTATUS_INSTALLING);

                //TODO: issue an install operation separately so that the apps could be downloaded in the background
                WaitForSingleObject(shExInfo.hProcess, INFINITE);
                CloseHandle(shExInfo.hProcess);
            }
            else
            {
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_INSTALL);
            }
        }

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

        DownloadsListView.SetDownloadStatus(iAppId, DLSTATUS_FINISHED);
    }

    delete static_cast<DownloadParam*>(param);
    SendMessageW(hDlg, WM_CLOSE, 0, 0);
    return 0;
}

//TODO: Reuse the dialog
VOID CDownloadManager::LaunchDownloadDialog(BOOL bIsModal)
{
    if (bIsModal)
    {
        DialogBoxW(hInst,
                   MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
                   hMainWnd,
                   DownloadDlgProc);
    }
    else
    {
        CreateDialogW(hInst,
                      MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG),
                      hMainWnd,
                      DownloadDlgProc);
    }
}
// CDownloadManager


BOOL DownloadListOfApplications(const ATL::CSimpleArray<CAvailableApplicationInfo>& AppsList, BOOL bIsModal)
{
    if (AppsList.GetSize() == 0)
        return FALSE;

    // Initialize shared variables
    for (INT i = 0; i < AppsList.GetSize(); ++i)
    {
        CDownloadManager::Add(AppsList[i]); // implicit conversion to DownloadInfo
    }

    // Create a dialog and issue a download process
    CDownloadManager::LaunchDownloadDialog(bIsModal);

    return TRUE;
}

BOOL DownloadApplication(CAvailableApplicationInfo* pAppInfo, BOOL bIsModal)
{
    if (!pAppInfo)
        return FALSE;

    CDownloadManager::Download(*pAppInfo, bIsModal);
    return TRUE;
}

VOID DownloadApplicationsDB(LPCWSTR lpUrl)
{
    static DownloadInfo DatabaseDLInfo;
    DatabaseDLInfo.szUrl = lpUrl;
    DatabaseDLInfo.szName.LoadStringW(IDS_DL_DIALOG_DB_DISP);
    CDownloadManager::Download(DatabaseDLInfo, TRUE);
}

