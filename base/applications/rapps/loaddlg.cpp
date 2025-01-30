/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displaying a download dialog
 * COPYRIGHT:   Copyright 2001 John R. Sheets (for CodeWeavers)
 *              Copyright 2004 Mike McCormack (for CodeWeavers)
 *              Copyright 2005 Ge van Geldorp (gvg@reactos.org)
 *              Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2015 Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
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

#include <debug.h>

#include <ui/rosctrls.h>
#include <windowsx.h>
#include <shlwapi_undoc.h>
#include <process.h>
#undef SubclassWindow

#include "rosui.h"
#include "dialogs.h"
#include "misc.h"
#include "unattended.h"

#ifdef USE_CERT_PINNING
#define CERT_ISSUER_INFO_PREFIX "US\r\nLet's Encrypt\r\nR"
#define CERT_ISSUER_INFO_OLD "US\r\nLet's Encrypt\r\nR3"
#define CERT_ISSUER_INFO_NEW "US\r\nLet's Encrypt\r\nR11"
#define CERT_SUBJECT_INFO "rapps.reactos.org"

static bool
IsTrustedPinnedCert(LPCSTR Subject, LPCSTR Issuer)
{
    if (strcmp(Subject, CERT_SUBJECT_INFO))
        return false;
#ifdef CERT_ISSUER_INFO_PREFIX
    return Issuer == StrStrA(Issuer, CERT_ISSUER_INFO_PREFIX);
#else
    return !strcmp(Issuer, CERT_ISSUER_INFO_OLD) || !strcmp(Issuer, CERT_ISSUER_INFO_NEW);
#endif
}
#endif // USE_CERT_PINNING

enum DownloadType
{
    DLTYPE_APPLICATION,
    DLTYPE_DBUPDATE,
    DLTYPE_DBUPDATE_UNOFFICIAL
};

enum DownloadStatus
{
    DLSTATUS_WAITING = IDS_STATUS_WAITING,
    DLSTATUS_DOWNLOADING = IDS_STATUS_DOWNLOADING,
    DLSTATUS_WAITING_INSTALL = IDS_STATUS_DOWNLOADED,
    DLSTATUS_INSTALLING = IDS_STATUS_INSTALLING,
    DLSTATUS_INSTALLED = IDS_STATUS_INSTALLED,
    DLSTATUS_FINISHED = IDS_STATUS_FINISHED
};

CStringW
LoadStatusString(DownloadStatus StatusParam)
{
    CStringW szString;
    szString.LoadStringW(StatusParam);
    return szString;
}

#define FILENAME_VALID_CHAR ( \
    PATH_CHAR_CLASS_LETTER      | \
    PATH_CHAR_CLASS_DOT         | \
    PATH_CHAR_CLASS_SEMICOLON   | \
    PATH_CHAR_CLASS_COMMA       | \
    PATH_CHAR_CLASS_SPACE       | \
    PATH_CHAR_CLASS_OTHER_VALID)

VOID
UrlUnescapeAndMakeFileNameValid(CStringW& str)
{
    WCHAR szPath[MAX_PATH];
    DWORD cchPath = _countof(szPath);
    UrlUnescapeW(const_cast<LPWSTR>((LPCWSTR)str), szPath, &cchPath, 0);

    for (PWCHAR pch = szPath; *pch; ++pch)
    {
        if (!PathIsValidCharW(*pch, FILENAME_VALID_CHAR))
            *pch = L'_';
    }

    str = szPath;
}

static void
SetFriendlyUrl(HWND hWnd, LPCWSTR pszUrl)
{
    CStringW buf;
    DWORD cch = (DWORD)(wcslen(pszUrl) + 1);
    if (InternetCanonicalizeUrlW(pszUrl, buf.GetBuffer(cch), &cch, ICU_DECODE | ICU_NO_ENCODE))
    {
        buf.ReleaseBuffer();
        pszUrl = buf;
    }
    SetWindowTextW(hWnd, pszUrl);
}

struct DownloadInfo
{
    DownloadInfo() :  DLType(DLTYPE_APPLICATION), IType(INSTALLER_UNKNOWN), SizeInBytes(0)
    {
    }
    DownloadInfo(const CAppInfo &AppInfo) : DLType(DLTYPE_APPLICATION)
    {
        AppInfo.GetDownloadInfo(szUrl, szSHA1, SizeInBytes);
        szName = AppInfo.szDisplayName;
        IType = AppInfo.GetInstallerType();
        if (IType == INSTALLER_GENERATE)
            szPackageName = AppInfo.szIdentifier;

        CConfigParser *cfg = static_cast<const CAvailableApplicationInfo&>(AppInfo).GetConfigParser();
        if (cfg)
            cfg->GetString(DB_SAVEAS, szFileName);
    }

    bool Equal(const DownloadInfo &other) const
    {
        return DLType == other.DLType && !lstrcmpW(szUrl, other.szUrl);
    }

    DownloadType DLType;
    InstallerType IType;
    CStringW szUrl;
    CStringW szName;
    CStringW szSHA1;
    CStringW szPackageName;
    CStringW szFileName;
    ULONG SizeInBytes;
};

class CDownloaderProgress : public CWindowImpl<CDownloaderProgress, CWindow, CControlWinTraits>
{
    CStringW m_szProgressText;

  public:
    CDownloaderProgress()
    {
    }

    VOID
    SetMarquee(BOOL Enable)
    {
        if (Enable)
            ModifyStyle(0, PBS_MARQUEE, 0);
        else
            ModifyStyle(PBS_MARQUEE, 0, 0);

        SendMessage(PBM_SETMARQUEE, Enable, 0);
    }

    VOID
    SetProgress(ULONG ulProgress, ULONG ulProgressMax)
    {
        WCHAR szProgress[100];

        /* format the bits and bytes into pretty and accessible units... */
        StrFormatByteSizeW(ulProgress, szProgress, _countof(szProgress));

        /* use our subclassed progress bar text subroutine */
        CStringW ProgressText;

        if (ulProgressMax)
        {
            /* total size is known */
            WCHAR szProgressMax[100];
            UINT uiPercentage = ((ULONGLONG)ulProgress * 100) / ulProgressMax;

            /* send the current progress to the progress bar */
            if (!IsWindow())
                return;
            SendMessage(PBM_SETPOS, uiPercentage, 0);

            /* format total download size */
            StrFormatByteSizeW(ulProgressMax, szProgressMax, _countof(szProgressMax));

            /* generate the text on progress bar */
            ProgressText.Format(L"%u%% \x2014 %ls / %ls", uiPercentage, szProgress, szProgressMax);
        }
        else
        {
            /* send the current progress to the progress bar */
            if (!IsWindow())
                return;
            SendMessage(PBM_SETPOS, 0, 0);

            /* total size is not known, display only current size */
            ProgressText.Format(L"%ls...", szProgress);
        }

        /* and finally display it */
        if (!IsWindow())
            return;
        SetWindowText(ProgressText.GetString());
    }

    LRESULT
    OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        return TRUE;
    }

    LRESULT
    OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        PAINTSTRUCT ps;
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
        DefWindowProc(uMsg, (WPARAM)hdcMem, lParam);

        /* draw our nifty progress text over it */
        SelectFont(hdcMem, GetStockFont(DEFAULT_GUI_FONT));
        DrawShadowText(
            hdcMem, m_szProgressText.GetString(), m_szProgressText.GetLength(), &myRect,
            DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE, GetSysColor(COLOR_CAPTIONTEXT),
            GetSysColor(COLOR_3DDKSHADOW), 1, 1);

        /* transfer the off-screen DC to the screen */
        BitBlt(hDC, 0, 0, win_width, win_height, hdcMem, 0, 0, SRCCOPY);

        /* free the off-screen DC */
        SelectObject(hdcMem, hOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(&ps);
        return 0;
    }

    LRESULT
    OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        PCWSTR pszText = (PCWSTR)lParam;
        if (pszText)
        {
            if (m_szProgressText != pszText)
            {
                m_szProgressText = pszText;
                InvalidateRect(NULL, TRUE);
            }
        }
        else
        {
            if (!m_szProgressText.IsEmpty())
            {
                m_szProgressText.Empty();
                InvalidateRect(NULL, TRUE);
            }
        }
        return TRUE;
    }

    BEGIN_MSG_MAP(CDownloaderProgress)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETTEXT, OnSetText)
    END_MSG_MAP()
};

class CDowloadingAppsListView : public CListView
{
  public:
    HWND
    Create(HWND hwndParent)
    {
        RECT r;
        ::GetClientRect(hwndParent, &r);
        r.top = (2 * r.top + 1 * r.bottom) / 3; /* The vertical position at ratio 1 : 2 */
        const INT MARGIN = 10;
        ::InflateRect(&r, -MARGIN, -MARGIN);

        const DWORD style = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER |
                            LVS_NOCOLUMNHEADER;

        HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE);

        AddColumn(0, 150, LVCFMT_LEFT);
        AddColumn(1, 120, LVCFMT_LEFT);

        return hwnd;
    }

    VOID
    LoadList(ATL::CSimpleArray<DownloadInfo> arrInfo, UINT Start = 0)
    {
        const INT base = GetItemCount();
        for (INT i = Start; i < arrInfo.GetSize(); ++i)
        {
            AddRow(base + i - Start, arrInfo[i].szName, DLSTATUS_WAITING);
        }
    }

    VOID
    SetDownloadStatus(INT ItemIndex, DownloadStatus Status)
    {
        CStringW szBuffer = LoadStatusString(Status);
        SetItemText(ItemIndex, 1, szBuffer.GetString());
    }

    BOOL
    AddItem(INT ItemIndex, LPWSTR lpText)
    {
        LVITEMW Item;

        ZeroMemory(&Item, sizeof(Item));

        Item.mask = LVIF_TEXT | LVIF_STATE;
        Item.pszText = lpText;
        Item.iItem = ItemIndex;

        return InsertItem(&Item);
    }

    VOID
    AddRow(INT RowIndex, LPCWSTR szAppName, const DownloadStatus Status)
    {
        CStringW szStatus = LoadStatusString(Status);
        AddItem(RowIndex, const_cast<LPWSTR>(szAppName));
        SetDownloadStatus(RowIndex, Status);
    }

    BOOL
    AddColumn(INT Index, INT Width, INT Format)
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
static BOOL
CertGetSubjectAndIssuer(HINTERNET hFile, CLocalPtr<char> &subjectInfo, CLocalPtr<char> &issuerInfo)
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
    if (!InternetQueryOptionA(hFile, INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT, &certInfo, &certInfoLength))
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

static inline VOID
MessageBox_LoadString(HWND hOwnerWnd, INT StringID)
{
    CStringW szMsgText;
    if (szMsgText.LoadStringW(StringID))
    {
        MessageBoxW(hOwnerWnd, szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
    }
}

static BOOL
ShowLastError(HWND hWndOwner, BOOL bInetError, DWORD dwLastError)
{
    CLocalPtr<WCHAR> lpMsg;

    if (!FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS |
                (bInetError ? FORMAT_MESSAGE_FROM_HMODULE : FORMAT_MESSAGE_FROM_SYSTEM),
            (bInetError ? GetModuleHandleW(L"wininet.dll") : NULL), dwLastError, LANG_USER_DEFAULT, (LPWSTR)&lpMsg, 0,
            NULL))
    {
        DPRINT1("FormatMessageW unexpected failure (err %d)\n", GetLastError());
        return FALSE;
    }

    MessageBoxW(hWndOwner, lpMsg, NULL, MB_OK | MB_ICONERROR);
    return TRUE;
}

// Download dialog (loaddlg.cpp)
HWND g_hDownloadWnd = NULL;

class CDownloadManager :
    public CComCoClass<CDownloadManager, &CLSID_NULL>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IUnknown
{
public:
    enum {
        WM_ISCANCELLED = WM_APP, // Return BOOL
        WM_SETSTATUS, // wParam DownloadStatus
        WM_GETINSTANCE, // Return CDownloadManager*
        WM_GETNEXT, // Return DownloadInfo* or NULL
    };

    CDownloadManager() : m_hDlg(NULL), m_Threads(0), m_Index(0), m_bCancelled(FALSE) {}

    static CDownloadManager*
    CreateInstanceHelper(BOOL Modal)
    {
        if (!Modal)
        {
            CDownloadManager* pExisting = CDownloadManager::FindInstance();
            if (pExisting)
            {
                pExisting->AddRef();
                return pExisting;
            }
        }
        CComPtr<CDownloadManager> obj;
        if (FAILED(ShellObjectCreator(obj)))
            return NULL;
        obj->m_bModal = Modal;
        return obj.Detach();
    }

    static BOOL
    CreateInstance(BOOL Modal, CComPtr<CDownloadManager> &Obj)
    {
        CDownloadManager *p = CreateInstanceHelper(Modal);
        if (!p)
            return FALSE;
        Obj.Attach(p);
        return TRUE;
    }

    static CDownloadManager*
    FindInstance()
    {
        if (g_hDownloadWnd)
            return (CDownloadManager*)SendMessageW(g_hDownloadWnd, WM_GETINSTANCE, 0, 0);
        return NULL;
    }

    BOOL
    IsCancelled()
    {
        return !IsWindow(m_hDlg) || SendMessageW(m_hDlg, WM_ISCANCELLED, 0, 0);
    }

    void StartWorkerThread();
    void Add(const DownloadInfo &Info);
    void Show();
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void UpdateProgress(ULONG ulProgress, ULONG ulProgressMax);
    static unsigned int CALLBACK ThreadFunc(void*ThreadParam);
    void PerformDownloadAndInstall(const DownloadInfo &Info);

    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CDownloadManager)
    BEGIN_COM_MAP(CDownloadManager)
    END_COM_MAP()

protected:
    HWND m_hDlg;
    UINT m_Threads;
    UINT m_Index;
    BOOL m_bCancelled;
    BOOL m_bModal;
    WCHAR m_szCaptionFmt[100];
    ATL::CSimpleArray<DownloadInfo> m_List;
    CDowloadingAppsListView m_ListView;
    CDownloaderProgress m_ProgressBar;
};

void
CDownloadManager::StartWorkerThread()
{
    AddRef(); // To keep m_List alive in thread
    unsigned int ThreadId;
    HANDLE Thread = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, this, 0, &ThreadId);
    if (Thread)
        CloseHandle(Thread);
    else
        Release();
}

void
CDownloadManager::Add(const DownloadInfo &Info)
{
    const UINT count = m_List.GetSize(), start = count;
    for (UINT i = 0; i < count; ++i)
    {
        if (Info.Equal(m_List[i]))
            return; // Already in the list
    }
    m_List.Add(Info);
    if (m_hDlg)
        m_ListView.LoadList(m_List, start);
}

void
CDownloadManager::Show()
{
    if (m_bModal)
        DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG), hMainWnd, DlgProc, (LPARAM)this);
    else if (!m_hDlg || !IsWindow(m_hDlg))
        CreateDialogParamW(hInst, MAKEINTRESOURCEW(IDD_DOWNLOAD_DIALOG), hMainWnd, DlgProc, (LPARAM)this);
}

INT_PTR CALLBACK
CDownloadManager::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDownloadManager* pThis = (CDownloadManager*)GetWindowLongPtrW(hDlg, DWLP_USER);
    if (!pThis)
    {
        if (uMsg != WM_INITDIALOG)
            return FALSE;
        SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
        pThis = (CDownloadManager*)lParam;
    }
    return pThis->RealDlgProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR
CDownloadManager::RealDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            g_Busy++;
            AddRef();
            m_hDlg = hDlg;
            if (!m_bModal)
                g_hDownloadWnd = hDlg;

            HICON hIconSm, hIconBg;
            if (hMainWnd)
            {
                hIconBg = (HICON)GetClassLongPtrW(hMainWnd, GCLP_HICON);
                hIconSm = (HICON)GetClassLongPtrW(hMainWnd, GCLP_HICONSM);
            }
            if (!hMainWnd || (!hIconBg || !hIconSm))
            {
                hIconBg = hIconSm = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
            }
            SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBg);
            SendMessageW(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

            GetWindowTextW(hDlg, m_szCaptionFmt, _countof(m_szCaptionFmt));
            CStringW buf;
            buf = m_szCaptionFmt;
            buf.Replace(L"%ls", L"");
            SetWindowTextW(hDlg, buf); // "Downloading..."

            HWND hItem = GetDlgItem(hDlg, IDC_DOWNLOAD_PROGRESS);
            if (hItem)
            {
                // initialize the default values for our nifty progress bar
                // and subclass it so that it learns to print a status text
                m_ProgressBar.SubclassWindow(hItem);
                m_ProgressBar.SendMessageW(PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                m_ProgressBar.SendMessageW(PBM_SETPOS, 0, 0);
                if (m_List.GetSize() > 0)
                    m_ProgressBar.SetProgress(0, m_List[0].SizeInBytes);
            }

            if (!m_ListView.Create(hDlg))
                return FALSE;
            m_ListView.LoadList(m_List);

            ShowWindow(hDlg, SW_SHOW);
            StartWorkerThread();
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                m_bCancelled = TRUE;
                PostMessageW(hDlg, WM_CLOSE, 0, 0);
            }
            return FALSE;

        case WM_CLOSE:
            m_bCancelled = TRUE;
            if (m_ProgressBar)
                m_ProgressBar.UnsubclassWindow(TRUE);
            return m_bModal ? ::EndDialog(hDlg, 0) : ::DestroyWindow(hDlg);

        case WM_DESTROY:
            if (g_hDownloadWnd == hDlg)
                g_hDownloadWnd = NULL;
            g_Busy--;
            if (hMainWnd)
                PostMessage(hMainWnd, WM_NOTIFY_OPERATIONCOMPLETED, 0, 0);
            Release();
            break;

        case WM_ISCANCELLED:
            return SetDlgMsgResult(hDlg, uMsg, m_bCancelled);

        case WM_SETSTATUS:
            m_ListView.SetDownloadStatus(m_Index - 1, (DownloadStatus)wParam);
            break;

        case WM_GETINSTANCE:
            return SetDlgMsgResult(hDlg, uMsg, (INT_PTR)this);

        case WM_GETNEXT:
        {
            DownloadInfo *pItem = NULL;
            if (!m_bCancelled && m_Index < (SIZE_T)m_List.GetSize())
                pItem = &m_List[m_Index++];
            return SetDlgMsgResult(hDlg, uMsg, (INT_PTR)pItem);
        }
    }
    return FALSE;
}

void
CDownloadManager::UpdateProgress(ULONG ulProgress, ULONG ulProgressMax)
{
    m_ProgressBar.SetProgress(ulProgress, ulProgressMax);
}

unsigned int CALLBACK
CDownloadManager::ThreadFunc(void* ThreadParam)
{
    CDownloadManager *pThis = (CDownloadManager*)ThreadParam;
    HWND hDlg = pThis->m_hDlg;
    for (;;)
    {
        DownloadInfo *pItem = (DownloadInfo*)SendMessageW(hDlg, WM_GETNEXT, 0, 0);
        if (!pItem)
            break;
        pThis->PerformDownloadAndInstall(*pItem);
    }
    SendMessageW(hDlg, WM_CLOSE, 0, 0);
    return pThis->Release();
}

void
CDownloadManager::PerformDownloadAndInstall(const DownloadInfo &Info)
{
    const HWND hDlg = m_hDlg;
    const HWND hStatus = GetDlgItem(m_hDlg, IDC_DOWNLOAD_STATUS);
    SetFriendlyUrl(hStatus, Info.szUrl);

    m_ProgressBar.SetMarquee(FALSE);
    m_ProgressBar.SendMessageW(PBM_SETPOS, 0, 0);
    m_ProgressBar.SetProgress(0, Info.SizeInBytes);

    CStringW str;
    CPathW Path;
    PCWSTR p;

    ULONG dwContentLen, dwBytesWritten, dwBytesRead, dwStatus, dwStatusLen;
    ULONG dwCurrentBytesRead = 0;
    BOOL bTempfile = FALSE, bCancelled = FALSE;

    HINTERNET hOpen = NULL;
    HINTERNET hFile = NULL;
    HANDLE hOut = INVALID_HANDLE_VALUE;

    
    LPCWSTR lpszAgent = L"RApps/1.1";
    const DWORD dwUrlConnectFlags =
        INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION;
    URL_COMPONENTSW urlComponents;
    size_t urlLength;
    unsigned char lpBuffer[4096];

    // Change caption to show the currently downloaded app
    switch (Info.DLType)
    {
        case DLTYPE_APPLICATION:
            str.Format(m_szCaptionFmt, Info.szName.GetString());
            break;
        case DLTYPE_DBUPDATE:
            str.LoadStringW(IDS_DL_DIALOG_DB_DOWNLOAD_DISP);
            break;
        case DLTYPE_DBUPDATE_UNOFFICIAL:
            str.LoadStringW(IDS_DL_DIALOG_DB_UNOFFICIAL_DOWNLOAD_DISP);
            break;
    }
    SetWindowTextW(hDlg, str);

    // is this URL an update package for RAPPS? if so store it in a different place
    if (Info.DLType != DLTYPE_APPLICATION)
    {
        if (!GetStorageDirectory(Path))
        {
            ShowLastError(hMainWnd, FALSE, GetLastError());
            goto end;
        }
    }
    else
    {
        Path = SettingsInfo.szDownloadDir;
    }

    // build the path for the download
    p = wcsrchr(Info.szUrl.GetString(), L'/');

    // do we have a final slash separator?
    if (!p)
    {
        MessageBox_LoadString(hMainWnd, IDS_UNABLE_PATH);
        goto end;
    }

    // is the path valid? can we access it?
    if (GetFileAttributesW(Path) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(Path, NULL))
        {
            ShowLastError(hMainWnd, FALSE, GetLastError());
            goto end;
        }
    }

    switch (Info.DLType)
    {
        case DLTYPE_DBUPDATE:
        case DLTYPE_DBUPDATE_UNOFFICIAL:
            Path += APPLICATION_DATABASE_NAME;
            break;
        case DLTYPE_APPLICATION:
        {
            CStringW name = Info.szFileName;
            if (name.IsEmpty())
                name = p + 1; // use the filename retrieved from URL
            UrlUnescapeAndMakeFileNameValid(name);
            Path += name;
            break;
        }
    }

    if ((Info.DLType == DLTYPE_APPLICATION) && Info.szSHA1[0] &&
        GetFileAttributesW(Path) != INVALID_FILE_ATTRIBUTES)
    {
        // only open it in case of total correctness
        if (VerifyInteg(Info.szSHA1.GetString(), Path))
            goto run;
    }

    // Download it
    SendMessageW(hDlg, WM_SETSTATUS, DLSTATUS_DOWNLOADING, 0);
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
            hOpen = InternetOpenW(
                lpszAgent, INTERNET_OPEN_TYPE_PROXY, SettingsInfo.szProxyServer, SettingsInfo.szNoProxyFor, 0);
            break;
    }

    if (!hOpen)
    {
        ShowLastError(hMainWnd, TRUE, GetLastError());
        goto end;
    }

    bTempfile = TRUE;
    dwContentLen = 0;
    dwStatusLen = sizeof(dwStatus);
    ZeroMemory(&urlComponents, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);

    urlLength = Info.szUrl.GetLength();
    urlComponents.dwSchemeLength = urlLength + 1;
    urlComponents.lpszScheme = (LPWSTR)malloc(urlComponents.dwSchemeLength * sizeof(WCHAR));

    if (!InternetCrackUrlW(Info.szUrl, urlLength + 1, ICU_DECODE | ICU_ESCAPE, &urlComponents))
    {
        ShowLastError(hMainWnd, TRUE, GetLastError());
        goto end;
    }

    if (urlComponents.nScheme == INTERNET_SCHEME_HTTP || urlComponents.nScheme == INTERNET_SCHEME_HTTPS)
    {
        hFile = InternetOpenUrlW(hOpen, Info.szUrl, NULL, 0, dwUrlConnectFlags, 0);
        if (!hFile)
        {
            if (!ShowLastError(hMainWnd, TRUE, GetLastError()))
            {
                /* Workaround for CORE-17377 */
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD2);
            }
            goto end;
        }

        // query connection
        if (!HttpQueryInfoW(hFile, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwStatusLen, NULL))
        {
            ShowLastError(hMainWnd, TRUE, GetLastError());
            goto end;
        }

        if (dwStatus != HTTP_STATUS_OK)
        {
            MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD);
            goto end;
        }

        // query content length
        HttpQueryInfoW(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwStatusLen, NULL);
    }
    else if (urlComponents.nScheme == INTERNET_SCHEME_FTP)
    {
        // force passive mode on FTP
        hFile =
            InternetOpenUrlW(hOpen, Info.szUrl, NULL, 0, dwUrlConnectFlags | INTERNET_FLAG_PASSIVE, 0);
        if (!hFile)
        {
            if (!ShowLastError(hMainWnd, TRUE, GetLastError()))
            {
                /* Workaround for CORE-17377 */
                MessageBox_LoadString(hMainWnd, IDS_UNABLE_TO_DOWNLOAD2);
            }
            goto end;
        }

        dwContentLen = FtpGetFileSize(hFile, &dwStatus);
    }
    else if (urlComponents.nScheme == INTERNET_SCHEME_FILE)
    {
        // Add support for the file scheme so testing locally is simpler
        WCHAR LocalFilePath[MAX_PATH];
        DWORD cchPath = _countof(LocalFilePath);
        // Ideally we would use PathCreateFromUrlAlloc here, but that is not exported (yet)
        HRESULT hr = PathCreateFromUrlW(Info.szUrl, LocalFilePath, &cchPath, 0);
        if (SUCCEEDED(hr))
        {
            if (CopyFileW(LocalFilePath, Path, FALSE))
            {
                goto run;
            }
            else
            {
                ShowLastError(hMainWnd, FALSE, GetLastError());
                goto end;
            }
        }
        else
        {
            ShowLastError(hMainWnd, FALSE, hr);
            goto end;
        }
    }

    if (!dwContentLen)
    {
        // Someone was nice enough to add this, let's use it
        if (Info.SizeInBytes)
        {
            dwContentLen = Info.SizeInBytes;
        }
        else
        {
            // content-length is not known, enable marquee mode
            m_ProgressBar.SetMarquee(TRUE);
        }
    }

    free(urlComponents.lpszScheme);

#ifdef USE_CERT_PINNING
    // are we using HTTPS to download the RAPPS update package? check if the certificate is original
    if ((urlComponents.nScheme == INTERNET_SCHEME_HTTPS) && (Info.DLType == DLTYPE_DBUPDATE))
    {
        CLocalPtr<char> subjectName, issuerName;
        CStringA szMsgText;
        bool bAskQuestion = false;
        if (!CertGetSubjectAndIssuer(hFile, subjectName, issuerName))
        {
            szMsgText.LoadStringW(IDS_UNABLE_TO_QUERY_CERT);
            bAskQuestion = true;
        }
        else if (!IsTrustedPinnedCert(subjectName, issuerName))
        {
            szMsgText.Format(IDS_MISMATCH_CERT_INFO, (LPCSTR)subjectName, (LPCSTR)issuerName);
            bAskQuestion = true;
        }

        if (bAskQuestion)
        {
            if (MessageBoxA(hDlg, szMsgText, NULL, MB_YESNO | MB_ICONERROR) != IDYES)
            {
                goto end;
            }
        }
    }
#endif

    hOut = CreateFileW(Path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        ShowLastError(hDlg, FALSE, GetLastError());
        goto end;
    }

    dwCurrentBytesRead = 0;
    do
    {
        bCancelled = IsCancelled();
        if (bCancelled)
            break;

        if (!InternetReadFile(hFile, lpBuffer, _countof(lpBuffer), &dwBytesRead))
        {
            ShowLastError(hDlg, TRUE, GetLastError());
            goto end;
        }

        if (!WriteFile(hOut, &lpBuffer[0], dwBytesRead, &dwBytesWritten, NULL))
        {
            ShowLastError(hDlg, FALSE, GetLastError());
            goto end;
        }

        dwCurrentBytesRead += dwBytesRead;
        UpdateProgress(dwCurrentBytesRead, dwContentLen);
        
    } while (dwBytesRead);

    CloseHandle(hOut);
    hOut = INVALID_HANDLE_VALUE;

    if (bCancelled)
    {
        DPRINT1("Operation cancelled\n");
        goto end;
    }

    if (!dwContentLen)
    {
        // set progress bar to 100%
        m_ProgressBar.SetMarquee(FALSE);

        dwContentLen = dwCurrentBytesRead;
        UpdateProgress(dwCurrentBytesRead, dwContentLen);
    }

    /* if this thing isn't a RAPPS update and it has a SHA-1 checksum
    verify its integrity by using the native advapi32.A_SHA1 functions */
    if ((Info.DLType == DLTYPE_APPLICATION) && Info.szSHA1[0] != 0)
    {
        CStringW szMsgText;

        // change a few strings in the download dialog to reflect the verification process
        if (!szMsgText.LoadStringW(IDS_INTEG_CHECK_TITLE))
        {
            DPRINT1("Unable to load string\n");
            goto end;
        }

        SetWindowTextW(hDlg, szMsgText);
        SetWindowTextW(hStatus, Path);

        // this may take a while, depending on the file size
        if (!VerifyInteg(Info.szSHA1, Path))
        {
            if (!szMsgText.LoadStringW(IDS_INTEG_CHECK_FAIL))
            {
                DPRINT1("Unable to load string\n");
                goto end;
            }

            MessageBoxW(hDlg, szMsgText, NULL, MB_OK | MB_ICONERROR);
            goto end;
        }
    }

run:
    SendMessageW(hDlg, WM_SETSTATUS, DLSTATUS_WAITING_INSTALL, 0);

    // run it
    if (Info.DLType == DLTYPE_APPLICATION)
    {
        CStringW app, params;
        SHELLEXECUTEINFOW shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.lpVerb = L"open";
        shExInfo.lpFile = Path;
        shExInfo.lpParameters = L"";
        shExInfo.nShow = SW_SHOW;

        if (Info.IType == INSTALLER_GENERATE)
        {
            params = L"/" + CStringW(CMD_KEY_GENINST) + L" \"" +
                     Info.szPackageName + L"\" \"" +
                     CStringW(shExInfo.lpFile) + L"\"";
            shExInfo.lpParameters = params;
            shExInfo.lpFile = app.GetBuffer(MAX_PATH);
            GetModuleFileNameW(NULL, const_cast<LPWSTR>(shExInfo.lpFile), MAX_PATH);
            app.ReleaseBuffer();
        }

        /* FIXME: Do we want to log installer status? */
        WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_INSTALL, Info.szName);

        if (ShellExecuteExW(&shExInfo))
        {
            // reflect installation progress in the titlebar
            // TODO: make a separate string with a placeholder to include app name?
            CStringW szMsgText = LoadStatusString(DLSTATUS_INSTALLING);
            SetWindowTextW(hDlg, szMsgText);

            SendMessageW(hDlg, WM_SETSTATUS, DLSTATUS_INSTALLING, 0);

            // TODO: issue an install operation separately so that the apps could be downloaded in the background
            WaitForSingleObject(shExInfo.hProcess, INFINITE);
            CloseHandle(shExInfo.hProcess);
        }
        else
        {
            ShowLastError(hMainWnd, FALSE, GetLastError());
        }
    }

end:
    if (hOut != INVALID_HANDLE_VALUE)
        CloseHandle(hOut);

    if (hFile)
        InternetCloseHandle(hFile);
    InternetCloseHandle(hOpen);

    if (bTempfile)
    {
        if (bCancelled || (SettingsInfo.bDelInstaller && Info.DLType == DLTYPE_APPLICATION))
            DeleteFileW(Path);
    }

    SendMessageW(hDlg, WM_SETSTATUS, DLSTATUS_FINISHED, 0);
}

BOOL
DownloadListOfApplications(const CAtlList<CAppInfo *> &AppsList, BOOL bIsModal)
{
    if (AppsList.IsEmpty())
        return FALSE;

    CComPtr<CDownloadManager> pDM;
    if (!CDownloadManager::CreateInstance(bIsModal, pDM))
        return FALSE;

    for (POSITION it = AppsList.GetHeadPosition(); it;)
    {
        const CAppInfo *Info = AppsList.GetNext(it);
        pDM->Add(DownloadInfo(*Info));
    }
    pDM->Show();
    return TRUE;
}

BOOL
DownloadApplication(CAppInfo *pAppInfo)
{
    const bool bModal = false;
    if (!pAppInfo)
        return FALSE;

    CAtlList<CAppInfo*> list;
    list.AddTail(pAppInfo);
    return DownloadListOfApplications(list, bModal);
}

VOID
DownloadApplicationsDB(LPCWSTR lpUrl, BOOL IsOfficial)
{
    const bool bModal = true;
    CComPtr<CDownloadManager> pDM;
    if (!CDownloadManager::CreateInstance(bModal, pDM))
        return;

    DownloadInfo DatabaseDLInfo;
    DatabaseDLInfo.szUrl = lpUrl;
    DatabaseDLInfo.szName.LoadStringW(IDS_DL_DIALOG_DB_DISP);
    DatabaseDLInfo.DLType = IsOfficial ? DLTYPE_DBUPDATE : DLTYPE_DBUPDATE_UNOFFICIAL;

    pDM->Add(DatabaseDLInfo);
    pDM->Show();
}
