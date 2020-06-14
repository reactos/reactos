/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/gui.cpp
 * PURPOSE:     GUI classes for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana           (gigaherz@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"
#include "asyncinet.h"

#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atltypes.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>
#include <rosctrls.h>
#include <gdiplus.h>
#include <math.h>

using namespace Gdiplus;

#define SEARCH_TIMER_ID 'SR'
#define LISTVIEW_ICON_SIZE 24
#define TREEVIEW_ICON_SIZE 24

// default broken-image icon size
#define BROKENIMG_ICON_SIZE 96

// the boundary of w/h ratio of snapshot preview window
#define SNPSHT_MAX_ASPECT_RAT 2.5

// padding between snapshot preview and richedit (in pixel)
#define INFO_DISPLAY_PADDING 10

// minimum width of richedit
#define RICHEDIT_MIN_WIDTH 160


// user-defined window message
#define WM_DOWNLOAD_COMPLETE (WM_USER + 1) // notify download complete. wParam is error code, and lParam is a pointer to SNPSHT_DOWNLOAD_PARAM
#define WM_RESIZE_CHILDREN   (WM_USER + 2) // ask parent window to resize children.

enum SNPSHT_STATUS
{
    SNPSHTPREV_EMPTY,      // show nothing
    SNPSHTPREV_LOADING,    // image is loading (most likely downloading)
    SNPSHTPREV_IMAGE,       // display image from a file
    SNPSHTPREV_FAILED      // image can not be shown (download failure or wrong image)
};

#define TIMER_LOADING_ANIMATION 1 // Timer ID

#define LOADING_ANIMATION_PERIOD 3 // Animation cycling period (in seconds)
#define LOADING_ANIMATION_FPS 18 // Animation Frame Per Second


#define PI 3.1415927

typedef struct __SnapshotDownloadParam
{
    LONGLONG ID;
    HANDLE hFile;
    HWND hwndNotify;
    ATL::CStringW DownloadFileName;
}SNPSHT_DOWNLOAD_PARAM;

INT GetSystemColorDepth()
{
    DEVMODEW pDevMode;
    INT ColorDepth;

    pDevMode.dmSize = sizeof(pDevMode);
    pDevMode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &pDevMode))
    {
        /* TODO: Error message */
        return ILC_COLOR;
    }

    switch (pDevMode.dmBitsPerPel)
    {
    case 32: ColorDepth = ILC_COLOR32; break;
    case 24: ColorDepth = ILC_COLOR24; break;
    case 16: ColorDepth = ILC_COLOR16; break;
    case  8: ColorDepth = ILC_COLOR8;  break;
    case  4: ColorDepth = ILC_COLOR4;  break;
    default: ColorDepth = ILC_COLOR;   break;
    }

    return ColorDepth;
}

class CAppRichEdit :
    public CUiWindow<CRichEdit>
{
private:
    VOID LoadAndInsertText(UINT uStringID,
                           const ATL::CStringW& szText,
                           DWORD StringFlags,
                           DWORD TextFlags)
    {
        ATL::CStringW szLoadedText;
        if (!szText.IsEmpty() && szLoadedText.LoadStringW(uStringID))
        {
            InsertText(szLoadedText, StringFlags);
            InsertText(szText, TextFlags);
        }
    }

    VOID LoadAndInsertText(UINT uStringID,
                                       DWORD StringFlags)
    {
        ATL::CStringW szLoadedText;
        if (szLoadedText.LoadStringW(uStringID))
        {
            InsertText(L"\n", 0);
            InsertText(szLoadedText, StringFlags);
            InsertText(L"\n", 0);
        }
    }

    VOID InsertVersionInfo(CAvailableApplicationInfo* Info)
    {
        if (Info->IsInstalled())
        {
            if (Info->HasInstalledVersion())
            {
                if (Info->HasUpdate())
                    LoadAndInsertText(IDS_STATUS_UPDATE_AVAILABLE, CFE_ITALIC);
                else
                    LoadAndInsertText(IDS_STATUS_INSTALLED, CFE_ITALIC);

                LoadAndInsertText(IDS_AINFO_VERSION, Info->m_szInstalledVersion, CFE_BOLD, 0);
            }
            else
            {
                LoadAndInsertText(IDS_STATUS_INSTALLED, CFE_ITALIC);
            }
        }
        else
        {
            LoadAndInsertText(IDS_STATUS_NOTINSTALLED, CFE_ITALIC);
        }

        LoadAndInsertText(IDS_AINFO_AVAILABLEVERSION, Info->m_szVersion, CFE_BOLD, 0);
    }

    VOID InsertLicenseInfo(CAvailableApplicationInfo* Info)
    {
        ATL::CStringW szLicense;
        switch (Info->m_LicenseType)
        {
        case LICENSE_OPENSOURCE:
            szLicense.LoadStringW(IDS_LICENSE_OPENSOURCE);
            break;
        case LICENSE_FREEWARE:
            szLicense.LoadStringW(IDS_LICENSE_FREEWARE);
            break;
        case LICENSE_TRIAL:
            szLicense.LoadStringW(IDS_LICENSE_TRIAL);
            break;
        default:
            LoadAndInsertText(IDS_AINFO_LICENSE, Info->m_szLicense, CFE_BOLD, 0);
            return;
        }

        szLicense += L" (" + Info->m_szLicense + L")";
        LoadAndInsertText(IDS_AINFO_LICENSE, szLicense, CFE_BOLD, 0);
    }

    VOID InsertLanguageInfo(CAvailableApplicationInfo* Info)
    {
        if (!Info->HasLanguageInfo())
        {
            return;
        }

        const INT nTranslations = Info->m_LanguageLCIDs.GetSize();
        ATL::CStringW szLangInfo;
        ATL::CStringW szLoadedTextAvailability;
        ATL::CStringW szLoadedAInfoText;

        szLoadedAInfoText.LoadStringW(IDS_AINFO_LANGUAGES);

        if (Info->HasNativeLanguage())
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_AVAILABLE_TRANSLATION);
            if (nTranslations > 1)
            {
                ATL::CStringW buf;
                buf.LoadStringW(IDS_LANGUAGE_MORE_PLACEHOLDER);
                szLangInfo.Format(buf, nTranslations - 1);
            }
            else
            {
                szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
                szLangInfo = L" (" + szLangInfo + L")";
            }
        }
        else if (Info->HasEnglishLanguage())
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_ENGLISH_TRANSLATION);
            if (nTranslations > 1)
            {
                ATL::CStringW buf;
                buf.LoadStringW(IDS_LANGUAGE_AVAILABLE_PLACEHOLDER);
                szLangInfo.Format(buf, nTranslations - 1);
            }
            else
            {
                szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
                szLangInfo = L" (" + szLangInfo + L")";
            }
        }
        else
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_NO_TRANSLATION);
        }

        InsertText(szLoadedAInfoText, CFE_BOLD);
        InsertText(szLoadedTextAvailability, NULL);
        InsertText(szLangInfo, CFE_ITALIC);
    }

public:
    BOOL ShowAvailableAppInfo(CAvailableApplicationInfo* Info)
    {
        if (!Info) return FALSE;

        SetText(Info->m_szName, CFE_BOLD);
        InsertVersionInfo(Info);
        InsertLicenseInfo(Info);
        InsertLanguageInfo(Info);

        LoadAndInsertText(IDS_AINFO_SIZE, Info->m_szSize, CFE_BOLD, 0);
        LoadAndInsertText(IDS_AINFO_URLSITE, Info->m_szUrlSite, CFE_BOLD, CFE_LINK);
        LoadAndInsertText(IDS_AINFO_DESCRIPTION, Info->m_szDesc, CFE_BOLD, 0);
        LoadAndInsertText(IDS_AINFO_URLDOWNLOAD, Info->m_szUrlDownload, CFE_BOLD, CFE_LINK);

        return TRUE;
    }

    BOOL ShowInstalledAppInfo(PINSTALLED_INFO Info)
    {
        ATL::CStringW szText;
        ATL::CStringW szInfo;

        if (!Info || !Info->hSubKey)
            return FALSE;

        Info->GetApplicationString(L"DisplayName", szText);
        SetText(szText, CFE_BOLD);
        InsertText(L"\n", 0);

#define GET_INFO(a, b, c, d) \
    if (Info->GetApplicationString(a, szInfo)) \
    { \
        LoadAndInsertText(b, szInfo, c, d); \
    }

        GET_INFO(L"DisplayVersion", IDS_INFO_VERSION, CFE_BOLD, 0);
        GET_INFO(L"Publisher", IDS_INFO_PUBLISHER, CFE_BOLD, 0);
        GET_INFO(L"RegOwner", IDS_INFO_REGOWNER, CFE_BOLD, 0);
        GET_INFO(L"ProductID", IDS_INFO_PRODUCTID, CFE_BOLD, 0);
        GET_INFO(L"HelpLink", IDS_INFO_HELPLINK, CFE_BOLD, CFM_LINK);
        GET_INFO(L"HelpTelephone", IDS_INFO_HELPPHONE, CFE_BOLD, 0);
        GET_INFO(L"Readme", IDS_INFO_README, CFE_BOLD, 0);
        GET_INFO(L"Contact", IDS_INFO_CONTACT, CFE_BOLD, 0);
        GET_INFO(L"URLUpdateInfo", IDS_INFO_UPDATEINFO, CFE_BOLD, CFM_LINK);
        GET_INFO(L"URLInfoAbout", IDS_INFO_INFOABOUT, CFE_BOLD, CFM_LINK);
        GET_INFO(L"Comments", IDS_INFO_COMMENTS, CFE_BOLD, 0);
        GET_INFO(L"InstallDate", IDS_INFO_INSTALLDATE, CFE_BOLD, 0);
        GET_INFO(L"InstallLocation", IDS_INFO_INSTLOCATION, CFE_BOLD, 0);
        GET_INFO(L"InstallSource", IDS_INFO_INSTALLSRC, CFE_BOLD, 0);
        GET_INFO(L"UninstallString", IDS_INFO_UNINSTALLSTR, CFE_BOLD, 0);
        GET_INFO(L"InstallSource", IDS_INFO_INSTALLSRC, CFE_BOLD, 0);
        GET_INFO(L"ModifyPath", IDS_INFO_MODIFYPATH, CFE_BOLD, 0);

        return TRUE;
    }

    VOID SetWelcomeText()
    {
        ATL::CStringW szText;

        szText.LoadStringW(IDS_WELCOME_TITLE);
        SetText(szText, CFE_BOLD);

        szText.LoadStringW(IDS_WELCOME_TEXT);
        InsertText(szText, 0);

        szText.LoadStringW(IDS_WELCOME_URL);
        InsertText(szText, CFM_LINK);
    }
};

int SnapshotDownloadCallback(
    pASYNCINET AsyncInet,
    UINT iReason,
    WPARAM wParam,
    LPARAM lParam,
    VOID* Extension
    )
{
    SNPSHT_DOWNLOAD_PARAM* DownloadParam = (SNPSHT_DOWNLOAD_PARAM*)Extension;
    switch (iReason)
    {
    case ASYNCINET_DATA:
        DWORD BytesWritten;
        WriteFile(DownloadParam->hFile, (LPCVOID)wParam, (DWORD)lParam, &BytesWritten, NULL);
        break;
    case ASYNCINET_COMPLETE:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_DOWNLOAD_COMPLETE, (WPARAM)ERROR_SUCCESS, (LPARAM)DownloadParam);
        break;
    case ASYNCINET_CANCELLED:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_DOWNLOAD_COMPLETE, (WPARAM)ERROR_CANCELLED, (LPARAM)DownloadParam);
        break;
    case ASYNCINET_ERROR:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_DOWNLOAD_COMPLETE, wParam, (LPARAM)DownloadParam);
        break;
    }
    return 0;
}

class CAppSnapshotPreview :
    public CWindowImpl<CAppSnapshotPreview>
{
private:

    SNPSHT_STATUS SnpshtPrevStauts = SNPSHTPREV_EMPTY;
    Image* pImage = NULL;
    HICON hBrokenImgIcon = NULL;
    BOOL bLoadingTimerOn = FALSE;
    int LoadingAnimationFrame = 0;
    int BrokenImgSize = BROKENIMG_ICON_SIZE;
    pASYNCINET AsyncInet = NULL;
    LONGLONG ContentID = 0; // used to determine whether image has been switched when download complete. Increase by 1 each time the content of this window changed
    ATL::CStringW TempImagePath; // currently displayed temp file

    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (Msg)
        {
        case WM_CREATE:
            hBrokenImgIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_BROKEN_IMAGE), IMAGE_ICON, BrokenImgSize, BrokenImgSize, 0);
            break;
        case WM_SIZE:
        {
            if (BrokenImgSize != min(min(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), BROKENIMG_ICON_SIZE))
            {
                BrokenImgSize = min(min(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), BROKENIMG_ICON_SIZE);

                if (hBrokenImgIcon)
                {
                    DeleteObject(hBrokenImgIcon);
                    hBrokenImgIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_BROKEN_IMAGE), IMAGE_ICON, BrokenImgSize, BrokenImgSize, 0);
                }
            }
            break;
        }
        case WM_DOWNLOAD_COMPLETE:
        {
            SNPSHT_DOWNLOAD_PARAM* DownloadParam = (SNPSHT_DOWNLOAD_PARAM*)lParam;
            AsyncInet = NULL;
            switch (wParam)
            {
            case ERROR_SUCCESS:
                if (ContentID == DownloadParam->ID)
                {
                    DisplayFile(DownloadParam->DownloadFileName);
                    // send a message to trigger resizing
                    ::SendMessageW(::GetParent(m_hWnd), WM_RESIZE_CHILDREN, 0, 0);
                    InvalidateRect(0, 0);
                    TempImagePath = DownloadParam->DownloadFileName; // record tmp file path in order to delete it when cleanup
                }
                else
                {
                    // the picture downloaded is already outdated. delete it.
                    DeleteFileW(DownloadParam->DownloadFileName);
                }
                break;
            case ERROR_CANCELLED:
                DeleteFileW(DownloadParam->DownloadFileName);
                break;
            default:
                DisplayFailed();
                // send a message to trigger resizing
                ::SendMessageW(::GetParent(m_hWnd), WM_RESIZE_CHILDREN, 0, 0);
                InvalidateRect(0, 0);
                DeleteFileW(DownloadParam->DownloadFileName);
                break;
            }
            delete DownloadParam;
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(&ps);
            CRect rect;
            GetClientRect(&rect);

            PaintOnDC(hdc,
                rect.Width(),
                rect.Height(),
                ps.fErase);

            EndPaint(&ps);
            break;
        }
        case WM_PRINTCLIENT:
        {
            if (lParam & PRF_CHECKVISIBLE)
            {
                if (!IsWindowVisible()) break;
            }
            CRect rect;
            GetClientRect(&rect);

            PaintOnDC((HDC)wParam,
                rect.Width(),
                rect.Height(),
                lParam & PRF_ERASEBKGND);
            break;
        }
        case WM_ERASEBKGND:
        {
            return TRUE; // do not erase to avoid blinking
        }
        case WM_TIMER:
        {
            switch (wParam)
            {
            case TIMER_LOADING_ANIMATION:
                LoadingAnimationFrame++;
                LoadingAnimationFrame %= (LOADING_ANIMATION_PERIOD * LOADING_ANIMATION_FPS);
                HDC hdc = GetDC();
                CRect rect;
                GetClientRect(&rect);

                PaintOnDC(hdc,
                    rect.Width(),
                    rect.Height(),
                    TRUE);
                ReleaseDC(hdc);
            }
            break;
        }
        case WM_DESTROY:
        {
            PreviousDisplayCleanup();
            DeleteObject(hBrokenImgIcon);
            hBrokenImgIcon = NULL;
            break;
        }
        }
        return FALSE;
    }

    VOID DisplayLoading()
    {
        SetStatus(SNPSHTPREV_LOADING);
        if (bLoadingTimerOn)
        {
            KillTimer(TIMER_LOADING_ANIMATION);
        }
        LoadingAnimationFrame = 0;
        bLoadingTimerOn = TRUE;
        SetTimer(TIMER_LOADING_ANIMATION, 1000 / LOADING_ANIMATION_FPS, 0);
    }

    BOOL DisplayFile(LPCWSTR lpszFileName)
    {
        PreviousDisplayCleanup();
        SetStatus(SNPSHTPREV_IMAGE);
        pImage = Bitmap::FromFile(lpszFileName, 0);
        if (pImage->GetLastStatus() != Ok)
        {
            DisplayFailed();
            return FALSE;
        }
        return TRUE;
    }

    VOID SetStatus(SNPSHT_STATUS Status)
    {
        SnpshtPrevStauts = Status;
    }

    VOID PaintOnDC(HDC hdc, int width, int height, BOOL bDrawBkgnd)
    {
        // use an off screen dc to avoid blinking
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hdcMem, hBitmap);

        if (bDrawBkgnd)
        {
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, (HGDIOBJ)GetSysColorBrush(COLOR_BTNFACE));
            PatBlt(hdcMem, 0, 0, width, height, PATCOPY);
            SelectObject(hdcMem, hOldBrush);
        }

        switch (SnpshtPrevStauts)
        {
        case SNPSHTPREV_EMPTY:
        {

        }
        break;

        case SNPSHTPREV_LOADING:
        {
            Graphics graphics(hdcMem);
            Color color(255, 0, 0);
            SolidBrush dotBrush(Color(255, 100, 100, 100));

            graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);

            // Paint three dot
            float DotWidth = GetLoadingDotWidth(width, height);
            graphics.FillEllipse((Brush*)(&dotBrush),
                (REAL)width / 2.0 - min(width, height) * 2.0 / 16.0 - DotWidth / 2.0,
                (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame + LOADING_ANIMATION_FPS / 4, width, height) - DotWidth / 2.0,
                DotWidth,
                DotWidth);

            graphics.FillEllipse((Brush*)(&dotBrush),
                (REAL)width / 2.0 - DotWidth / 2.0,
                (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame, width, height) - DotWidth / 2.0,
                DotWidth,
                DotWidth);

            graphics.FillEllipse((Brush*)(&dotBrush),
                (REAL)width / 2.0 + min(width, height) * 2.0 / 16.0 - DotWidth / 2.0,
                (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame - LOADING_ANIMATION_FPS / 4, width, height) - DotWidth / 2.0,
                DotWidth,
                DotWidth);
        }
        break;

        case SNPSHTPREV_IMAGE:
        {
            if (pImage)
            {
                // always draw entire image inside the window.
                Graphics graphics(hdcMem);
                float ZoomRatio = min(((float)width / (float)pImage->GetWidth()), ((float)height / (float)pImage->GetHeight()));
                float ZoomedImgWidth = ZoomRatio * (float)pImage->GetWidth();
                float ZoomedImgHeight = ZoomRatio * (float)pImage->GetHeight();

                graphics.DrawImage(pImage,
                    ((float)width - ZoomedImgWidth) / 2.0, ((float)height - ZoomedImgHeight) / 2.0,
                    ZoomedImgWidth, ZoomedImgHeight);
            }
        }
        break;

        case SNPSHTPREV_FAILED:
        {
            DrawIconEx(hdcMem,
                (width - BrokenImgSize) / 2,
                (height - BrokenImgSize) / 2,
                hBrokenImgIcon,
                BrokenImgSize,
                BrokenImgSize,
                NULL,
                NULL,
                DI_NORMAL | DI_COMPAT);
        }
        break;
        }

        // copy the content form off-screen dc to hdc
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        DeleteDC(hdcMem);
        DeleteObject(hBitmap);
    }

    float GetLoadingDotWidth(int width, int height)
    {
        return min(width, height) / 20.0;
    }

    float GetFrameDotShift(int Frame, int width, int height)
    {
        return min(width, height) *
            (1.0 / 16.0) *
            (2.0 / (2.0 - sqrt(3.0))) *
            (max(sin((float)Frame * 2 * PI / (LOADING_ANIMATION_PERIOD * LOADING_ANIMATION_FPS)), sqrt(3.0) / 2.0) - sqrt(3.0) / 2.0);
    }

public:
    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
        static ATL::CWndClassInfo wc =
        {
            {
                sizeof(WNDCLASSEX),
                csStyle,
                StartWindowProc,
                0,
                0,
                NULL,
                0,
                LoadCursorW(NULL, IDC_ARROW),
                (HBRUSH)(COLOR_BTNFACE + 1),
                0,
                L"RAppsSnapshotPreview",
                NULL
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    HWND Create(HWND hParent)
    {
        RECT r = { 0,0,0,0 };

        return CWindowImpl::Create(hParent, r, L"", WS_CHILD | WS_VISIBLE);
    }

    VOID PreviousDisplayCleanup()
    {
        if (bLoadingTimerOn)
        {
            KillTimer(TIMER_LOADING_ANIMATION);
            bLoadingTimerOn = FALSE;
        }
        LoadingAnimationFrame = 0;
        if (pImage)
        {
            delete pImage;
            pImage = NULL;
        }
        if (AsyncInet)
        {
            AsyncInetCancel(AsyncInet);
            AsyncInet = NULL;
        }
        if (!TempImagePath.IsEmpty())
        {
            DeleteFileW(TempImagePath.GetString());
            TempImagePath.Empty();
        }
    }

    VOID DisplayEmpty()
    {
        InterlockedIncrement64(&ContentID);
        SetStatus(SNPSHTPREV_EMPTY);
        PreviousDisplayCleanup();
    }

    BOOL DisplayImage(LPCWSTR lpszLocation)
    {
        LONGLONG ID = InterlockedIncrement64(&ContentID);
        PreviousDisplayCleanup();

        if (PathIsURLW(lpszLocation))
        {
            DisplayLoading();

            SNPSHT_DOWNLOAD_PARAM* DownloadParam = new SNPSHT_DOWNLOAD_PARAM;
            if (!DownloadParam) return FALSE;

            DownloadParam->hwndNotify = m_hWnd;
            DownloadParam->ID = ID;
            // generate a filename
            ATL::CStringW SnapshotFolder = CAvailableApps::m_Strings.szAppsPath;
            PathAppendW(SnapshotFolder.GetBuffer(MAX_PATH), L"snapshots");
            SnapshotFolder.ReleaseBuffer();
            if (!GetTempFileNameW(SnapshotFolder.GetString(), L"img",
                0, DownloadParam->DownloadFileName.GetBuffer(MAX_PATH)))
            {
                DownloadParam->DownloadFileName.ReleaseBuffer();
                delete DownloadParam;
                DisplayFailed();
                return FALSE;
            }
            DownloadParam->hFile = CreateFileW(DownloadParam->DownloadFileName.GetBuffer(),
                GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            DownloadParam->DownloadFileName.ReleaseBuffer();
            if (INVALID_HANDLE_VALUE == DownloadParam->hFile)
            {
                delete DownloadParam;
                DisplayFailed();
                return FALSE;
            }

            AsyncInet = AsyncInetDownloadW(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, lpszLocation, TRUE, SnapshotDownloadCallback, DownloadParam);
            if (!AsyncInet)
            {
                CloseHandle(DownloadParam->hFile);
                DeleteFileW(DownloadParam->DownloadFileName.GetBuffer());
                delete DownloadParam;
                DisplayFailed();
                return FALSE;
            }
            return TRUE;
        }
        else
        {
            return DisplayFile(lpszLocation);
        }
    }

    VOID DisplayFailed()
    {
        InterlockedIncrement64(&ContentID);
        SetStatus(SNPSHTPREV_FAILED);
        PreviousDisplayCleanup();
    }

    int GetRequestedWidth(int Height) // calculate requested window width by given height
    {
        switch (SnpshtPrevStauts)
        {
        case SNPSHTPREV_EMPTY:
            return 0;
        case SNPSHTPREV_LOADING:
            return 200;
        case SNPSHTPREV_IMAGE:
            if (pImage)
            {
                // return the width needed to display image inside the window.
                // and always keep window w/h ratio inside [ 1/SNPSHT_MAX_ASPECT_RAT, SNPSHT_MAX_ASPECT_RAT ]
                return (int)floor((float)Height *
                    max(min((float)pImage->GetWidth() / (float)pImage->GetHeight(), (float)SNPSHT_MAX_ASPECT_RAT), 1.0/ (float)SNPSHT_MAX_ASPECT_RAT));
            }
            return 0;
        case SNPSHTPREV_FAILED:
            return 200;
        default:
            return 0;
        }
    }

    ~CAppSnapshotPreview()
    {
        PreviousDisplayCleanup();
    }
};

class CAppInfoDisplay :
    public CUiWindow<CWindowImpl<CAppInfoDisplay>>
{
    LPWSTR pLink = NULL;

private:
    BOOL ProcessWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (message)
        {
        case WM_CREATE:
        {
            RichEdit = new CAppRichEdit();
            RichEdit->Create(hwnd);

            SnpshtPrev = new CAppSnapshotPreview();
            SnpshtPrev->Create(hwnd);
            break;
        }
        case WM_SIZE:
        {
            ResizeChildren(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        }
        case WM_RESIZE_CHILDREN:
        {
            ResizeChildren();
            break;
        }
        case WM_COMMAND:
        {
            OnCommand(wParam, lParam);
            break;
        }
        case WM_NOTIFY:
        {
            NMHDR* NotifyHeader = (NMHDR*)lParam;
            if (NotifyHeader->hwndFrom == RichEdit->m_hWnd)
            {
                switch (NotifyHeader->code)
                {
                case EN_LINK:
                    OnLink((ENLINK*)lParam);
                    break;
                }
            }
            break;
        }
        }

        return FALSE;
    }

    VOID ResizeChildren()
    {
        CRect rect;
        GetWindowRect(&rect);
        ResizeChildren(rect.Width(), rect.Height());
    }

    VOID ResizeChildren(int Width, int Height)
    {
        int SnpshtWidth = SnpshtPrev->GetRequestedWidth(Height);

        // make sure richedit always have room to display
        SnpshtWidth = min(SnpshtWidth, Width - INFO_DISPLAY_PADDING - RICHEDIT_MIN_WIDTH);

        DWORD dwError = ERROR_SUCCESS;
        HDWP hDwp = BeginDeferWindowPos(2);

        if (hDwp)
        {
            hDwp = ::DeferWindowPos(hDwp, SnpshtPrev->m_hWnd, NULL,
                0, 0, SnpshtWidth, Height, 0);

            if (hDwp)
            {
                // hide the padding if snapshot window width == 0
                int RicheditPosX = SnpshtWidth ? (SnpshtWidth + INFO_DISPLAY_PADDING) : 0;

                hDwp = ::DeferWindowPos(hDwp, RichEdit->m_hWnd, NULL,
                    RicheditPosX, 0, Width - RicheditPosX, Height, 0);

                if (hDwp)
                {
                    EndDeferWindowPos(hDwp);
                }
                else
                {
                    dwError = GetLastError();
                }
            }
            else
            {
                dwError = GetLastError();
            }
        }
        else
        {
            dwError = GetLastError();
        }


#if DBG
        ATLASSERT(dwError == ERROR_SUCCESS);
#endif

        UpdateWindow();
    }

    VOID OnLink(ENLINK* Link)
    {
        switch (Link->msg)
        {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            if (pLink) HeapFree(GetProcessHeap(), 0, pLink);

            pLink = (LPWSTR)HeapAlloc(GetProcessHeap(), 0,
                (max(Link->chrg.cpMin, Link->chrg.cpMax) -
                    min(Link->chrg.cpMin, Link->chrg.cpMax) + 1) * sizeof(WCHAR));
            if (!pLink)
            {
                /* TODO: Error message */
                return;
            }

            RichEdit->SendMessageW(EM_SETSEL, Link->chrg.cpMin, Link->chrg.cpMax);
            RichEdit->SendMessageW(EM_GETSELTEXT, 0, (LPARAM)pLink);

            ShowPopupMenuEx(m_hWnd, m_hWnd, IDR_LINKMENU, -1);
        }
        break;
        }
    }

public:

    CAppRichEdit * RichEdit;
    CAppSnapshotPreview * SnpshtPrev;

    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
        static ATL::CWndClassInfo wc =
        {
            {
                sizeof(WNDCLASSEX),
                csStyle,
                StartWindowProc,
                0,
                0,
                NULL,
                NULL,
                NULL,
                (HBRUSH)(COLOR_BTNFACE + 1),
                NULL,
                L"RAppsAppInfo",
                NULL
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    HWND Create(HWND hwndParent)
    {
        RECT r = { 0,0,0,0 };

        return CWindowImpl::Create(hwndParent, r, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    }

    BOOL ShowAvailableAppInfo(CAvailableApplicationInfo* Info)
    {
        ATL::CStringW SnapshotLocation;
        if (Info->RetrieveSnapshot(0, SnapshotLocation))
        {
            SnpshtPrev->DisplayImage(SnapshotLocation);
        }
        else
        {
            SnpshtPrev->DisplayEmpty();
        }
        ResizeChildren();
        return RichEdit->ShowAvailableAppInfo(Info);
    }

    BOOL ShowInstalledAppInfo(PINSTALLED_INFO Info)
    {
        SnpshtPrev->DisplayEmpty();
        ResizeChildren();
        return RichEdit->ShowInstalledAppInfo(Info);
    }

    VOID SetWelcomeText()
    {
        SnpshtPrev->DisplayEmpty();
        ResizeChildren();
        RichEdit->SetWelcomeText();
    }

    VOID OnCommand(WPARAM wParam, LPARAM lParam)
    {
        WORD wCommand = LOWORD(wParam);

        switch (wCommand)
        {
        case ID_OPEN_LINK:
            
            ShellExecuteW(m_hWnd, L"open", pLink, NULL, NULL, SW_SHOWNOACTIVATE);
            HeapFree(GetProcessHeap(), 0, pLink);
            pLink = NULL;
            break;

        case ID_COPY_LINK:
            CopyTextToClipboard(pLink);
            HeapFree(GetProcessHeap(), 0, pLink);
            pLink = NULL;
            break;

        }
    }

};

class CMainToolbar :
    public CUiWindow< CToolbar<> >
{
    const INT m_iToolbarHeight;
    DWORD m_dButtonsWidthMax;

    WCHAR szInstallBtn[MAX_STR_LEN];
    WCHAR szUninstallBtn[MAX_STR_LEN];
    WCHAR szModifyBtn[MAX_STR_LEN];
    WCHAR szSelectAll[MAX_STR_LEN];

    VOID AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex)
    {
        HICON hImage;

        if (!(hImage = (HICON) LoadImageW(hInst,
                                          MAKEINTRESOURCE(ImageIndex),
                                          IMAGE_ICON,
                                          m_iToolbarHeight,
                                          m_iToolbarHeight,
                                          0)))
        {
            /* TODO: Error message */
        }

        ImageList_AddIcon(hImageList, hImage);
        DeleteObject(hImage);
    }

    HIMAGELIST InitImageList()
    {
        HIMAGELIST hImageList;

        /* Create the toolbar icon image list */
        hImageList = ImageList_Create(m_iToolbarHeight,//GetSystemMetrics(SM_CXSMICON),
                                      m_iToolbarHeight,//GetSystemMetrics(SM_CYSMICON),
                                      ILC_MASK | GetSystemColorDepth(),
                                      1, 1);
        if (!hImageList)
        {
            /* TODO: Error message */
            return NULL;
        }

        AddImageToImageList(hImageList, IDI_INSTALL);
        AddImageToImageList(hImageList, IDI_UNINSTALL);
        AddImageToImageList(hImageList, IDI_MODIFY);
        AddImageToImageList(hImageList, IDI_CHECK_ALL);
        AddImageToImageList(hImageList, IDI_REFRESH);
        AddImageToImageList(hImageList, IDI_UPDATE_DB);
        AddImageToImageList(hImageList, IDI_SETTINGS);
        AddImageToImageList(hImageList, IDI_EXIT);

        return hImageList;
    }

public:
    CMainToolbar() : m_iToolbarHeight(24)
    {
    }

    VOID OnGetDispInfo(LPTOOLTIPTEXT lpttt)
    {
        UINT idButton = (UINT) lpttt->hdr.idFrom;

        switch (idButton)
        {
        case ID_EXIT:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_EXIT);
            break;

        case ID_INSTALL:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_INSTALL);
            break;

        case ID_UNINSTALL:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UNINSTALL);
            break;

        case ID_MODIFY:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_MODIFY);
            break;

        case ID_SETTINGS:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_SETTINGS);
            break;

        case ID_REFRESH:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_REFRESH);
            break;

        case ID_RESETDB:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UPDATE_DB);
            break;
        }
    }

    HWND Create(HWND hwndParent)
    {
        /* Create buttons */
        TBBUTTON Buttons[] =
        {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
            {  0, ID_INSTALL,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szInstallBtn      },
            {  1, ID_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szUninstallBtn    },
            {  2, ID_MODIFY,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szModifyBtn       },
            {  3, ID_CHECK_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szSelectAll       },
            { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
            {  4, ID_REFRESH,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            {  5, ID_RESETDB,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
            {  6, ID_SETTINGS,  TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            {  7, ID_EXIT,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
        };

        LoadStringW(hInst, IDS_INSTALL, szInstallBtn, _countof(szInstallBtn));
        LoadStringW(hInst, IDS_UNINSTALL, szUninstallBtn, _countof(szUninstallBtn));
        LoadStringW(hInst, IDS_MODIFY, szModifyBtn, _countof(szModifyBtn));
        LoadStringW(hInst, IDS_SELECT_ALL, szSelectAll, _countof(szSelectAll));

        m_hWnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST,
                                 0, 0, 0, 0,
                                 hwndParent,
                                 0, hInst, NULL);

        if (!m_hWnd)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        SendMessageW(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        SetButtonStructSize();

        /* Set image list */
        HIMAGELIST hImageList = InitImageList();

        if (!hImageList)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        ImageList_Destroy(SetImageList(hImageList));

        AddButtons(_countof(Buttons), Buttons);

        /* Remember ideal width to use as a max width of buttons */
        SIZE size;
        GetIdealSize(FALSE, &size);
        m_dButtonsWidthMax = size.cx;

        return m_hWnd;
    }

    VOID HideButtonCaption()
    {
        DWORD dCurrentExStyle = (DWORD) SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
        SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle | TBSTYLE_EX_MIXEDBUTTONS);
    }

    VOID ShowButtonCaption()
    {
        DWORD dCurrentExStyle = (DWORD) SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
        SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle & ~TBSTYLE_EX_MIXEDBUTTONS);
    }

    DWORD GetMaxButtonsWidth() const
    {
        return m_dButtonsWidthMax;
    }
};

class CAppsListView :
    public CUiWindow<CListView>
{
    struct SortContext
    {
        CAppsListView * lvw;
        INT iSubItem;
    };

    BOOL bHasAllChecked;
    BOOL bIsAscending;
    BOOL bHasCheckboxes;

    INT nLastHeaderID;

public:
    CAppsListView() :
        bHasAllChecked(FALSE),
        bIsAscending(TRUE),
        bHasCheckboxes(FALSE),
        nLastHeaderID(-1)
    {
    }

    VOID SetCheckboxesVisible(BOOL bIsVisible)
    {
        if (bIsVisible)
        {
            SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
        }
        else
        {
            SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
        }

        bHasCheckboxes = bIsVisible;
    }

    VOID ColumnClick(LPNMLISTVIEW pnmv)
    {
        HWND hHeader;
        HDITEMW hColumn;
        INT nHeaderID = pnmv->iSubItem;

        if ((GetWindowLongPtr(GWL_STYLE) & ~LVS_NOSORTHEADER) == 0)
            return;

        hHeader = (HWND) SendMessage(LVM_GETHEADER, 0, 0);
        ZeroMemory(&hColumn, sizeof(hColumn));

        /* If the sorting column changed, remove the sorting style from the old column */
        if ((nLastHeaderID != -1) && (nLastHeaderID != nHeaderID))
        {
            hColumn.mask = HDI_FORMAT;
            Header_GetItem(hHeader, nLastHeaderID, &hColumn);
            hColumn.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            Header_SetItem(hHeader, nLastHeaderID, &hColumn);
        }

        /* Set the sorting style to the new column */
        hColumn.mask = HDI_FORMAT;
        Header_GetItem(hHeader, nHeaderID, &hColumn);

        hColumn.fmt &= (bIsAscending ? ~HDF_SORTDOWN : ~HDF_SORTUP);
        hColumn.fmt |= (bIsAscending ? HDF_SORTUP : HDF_SORTDOWN);
        Header_SetItem(hHeader, nHeaderID, &hColumn);

        /* Sort the list, using the current values of nHeaderID and bIsAscending */
        SortContext ctx = {this, nHeaderID};
        SortItems(s_CompareFunc, &ctx);

        /* Save new values */
        nLastHeaderID = nHeaderID;
        bIsAscending = !bIsAscending;
    }

    BOOL AddColumn(INT Index, ATL::CStringW& Text, INT Width, INT Format)
    {
        return AddColumn(Index, const_cast<LPWSTR>(Text.GetString()), Width, Format);
    }

    BOOL AddColumn(INT Index, LPWSTR lpText, INT Width, INT Format)
    {
        LVCOLUMNW Column;

        ZeroMemory(&Column, sizeof(Column));

        Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        Column.iSubItem = Index;
        Column.pszText = lpText;
        Column.cx = Width;
        Column.fmt = Format;

        return (InsertColumn(Index, &Column) == -1) ? FALSE : TRUE;
    }

    INT AddItem(INT ItemIndex, INT IconIndex, LPCWSTR lpText, LPARAM lParam)
    {
        LVITEMW Item;

        ZeroMemory(&Item, sizeof(Item));

        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        Item.pszText = const_cast<LPWSTR>(lpText);
        Item.lParam = lParam;
        Item.iItem = ItemIndex;
        Item.iImage = IconIndex;

        return InsertItem(&Item);
    }

    static INT CALLBACK s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        SortContext * ctx = ((SortContext*) lParamSort);
        return ctx->lvw->CompareFunc(lParam1, lParam2, ctx->iSubItem);
    }

    INT CompareFunc(LPARAM lParam1, LPARAM lParam2, INT iSubItem)
    {
        ATL::CStringW Item1, Item2;
        LVFINDINFOW IndexInfo;
        INT Index;

        IndexInfo.flags = LVFI_PARAM;

        IndexInfo.lParam = lParam1;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item1.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item1.ReleaseBuffer();

        IndexInfo.lParam = lParam2;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item2.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item2.ReleaseBuffer();

        return bIsAscending ? Item1.Compare(Item2) : Item2.Compare(Item1);
    }

    HWND Create(HWND hwndParent)
    {
        RECT r = {205, 28, 465, 250};
        DWORD style = WS_CHILD | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
        HMENU menu = GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATIONMENU)), 0);

        HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE, menu);

        if (hwnd)
        {
            SetCheckboxesVisible(FALSE);
        }

        return hwnd;
    }

    BOOL GetCheckState(INT item)
    {
        return (BOOL) (GetItemState(item, LVIS_STATEIMAGEMASK) >> 12) - 1;
    }

    VOID SetCheckState(INT item, BOOL fCheck)
    {
        if (bHasCheckboxes)
        {
            SetItemState(item, INDEXTOSTATEIMAGEMASK((fCheck) ? 2 : 1), LVIS_STATEIMAGEMASK);
            SetSelected(item, fCheck);
        }
    }

    VOID SetSelected(INT item, BOOL value)
    {
        if (item < 0)
        {
            for (INT i = 0; i >= 0; i = GetNextItem(i, LVNI_ALL))
            {
                CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(i);
                if (pAppInfo)
                {
                    pAppInfo->m_IsSelected = value;
                }
            }
        }
        else
        {
            CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(item);
            if (pAppInfo)
            {
                pAppInfo->m_IsSelected = value;
            }
        }
    }

    VOID CheckAll()
    {
        if (bHasCheckboxes)
        {
            bHasAllChecked = !bHasAllChecked;
            SetCheckState(-1, bHasAllChecked);
        }
    }

    ATL::CSimpleArray<CAvailableApplicationInfo> GetCheckedItems()
    {
        if (!bHasCheckboxes)
        {
            return ATL::CSimpleArray<CAvailableApplicationInfo>();
        }

        ATL::CSimpleArray<CAvailableApplicationInfo> list;
        for (INT i = 0; i >= 0; i = GetNextItem(i, LVNI_ALL))
        {
            if (GetCheckState(i) != FALSE)
            {
                CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(i);
                list.Add(*pAppInfo);
            }
        }
        return list;
    }

    CAvailableApplicationInfo* GetSelectedData()
    {
        INT item = GetSelectionMark();
        return (CAvailableApplicationInfo*) GetItemData(item);
    }
};

class CSideTreeView :
    public CUiWindow<CTreeView>
{
    HIMAGELIST hImageTreeView;

public:
    CSideTreeView() :
        CUiWindow(),
        hImageTreeView(ImageList_Create(TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE,
                                        GetSystemColorDepth() | ILC_MASK,
                                        0, 1))
    {
    }

    HTREEITEM AddItem(HTREEITEM hParent, ATL::CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam)
    {
        return CUiWindow<CTreeView>::AddItem(hParent, const_cast<LPWSTR>(Text.GetString()), Image, SelectedImage, lParam);
    }

    HTREEITEM AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
    {
        ATL::CStringW szText;
        INT Index;
        HICON hIcon;

        hIcon = (HICON) LoadImageW(hInst,
                                   MAKEINTRESOURCE(IconIndex),
                                   IMAGE_ICON,
                                   TREEVIEW_ICON_SIZE,
                                   TREEVIEW_ICON_SIZE,
                                   LR_CREATEDIBSECTION);
        if (hIcon)
        {
            Index = ImageList_AddIcon(hImageTreeView, hIcon);
            DestroyIcon(hIcon);
        }

        szText.LoadStringW(TextIndex);
        return AddItem(hRootItem, szText, Index, Index, TextIndex);
    }

    HIMAGELIST SetImageList()
    {
        return CUiWindow<CTreeView>::SetImageList(hImageTreeView, TVSIL_NORMAL);
    }

    VOID DestroyImageList()
    {
        if (hImageTreeView)
            ImageList_Destroy(hImageTreeView);
    }

    ~CSideTreeView()
    {
        DestroyImageList();
    }
};

class CSearchBar :
    public CWindow
{
public:
    const INT m_Width;
    const INT m_Height;

    CSearchBar() : m_Width(200), m_Height(22)
    {
    }

    VOID SetText(LPCWSTR lpszText)
    {
        SendMessageW(SB_SETTEXT, SBT_NOBORDERS, (LPARAM) lpszText);
    }

    HWND Create(HWND hwndParent)
    {
        ATL::CStringW szBuf;
        m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", NULL,
                                 WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                                 0, 0, m_Width, m_Height,
                                 hwndParent, (HMENU) NULL,
                                 hInst, 0);

        SendMessageW(WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
        szBuf.LoadStringW(IDS_SEARCH_TEXT);
        SetWindowTextW(szBuf);
        return m_hWnd;
    }

};

class CMainWindow :
    public CWindowImpl<CMainWindow, CWindow, CFrameWinTraits>
{
    CUiPanel* m_ClientPanel;
    CUiSplitPanel* m_VSplitter;
    CUiSplitPanel* m_HSplitter;

    CMainToolbar* m_Toolbar;
    CAppsListView* m_ListView;

    CSideTreeView* m_TreeView;
    CUiWindow<CStatusBar>* m_StatusBar;
    CAppInfoDisplay* m_AppInfo;

    CUiWindow<CSearchBar>* m_SearchBar;
    CAvailableApps m_AvailableApps;

    INT nSelectedApps;

    BOOL bSearchEnabled;
    BOOL bUpdating;

    ATL::CStringW szSearchPattern;
    INT SelectedEnumType;

public:
    CMainWindow() :
        m_ClientPanel(NULL),
        bSearchEnabled(FALSE),
        SelectedEnumType(ENUM_ALL_INSTALLED)
    {
    }

private:
    VOID InitApplicationsList()
    {
        ATL::CStringW szText;

        /* Add columns to ListView */
        szText.LoadStringW(IDS_APP_NAME);
        m_ListView->AddColumn(0, szText, 250, LVCFMT_LEFT);

        szText.LoadStringW(IDS_APP_INST_VERSION);
        m_ListView->AddColumn(1, szText, 90, LVCFMT_RIGHT);

        szText.LoadStringW(IDS_APP_DESCRIPTION);
        m_ListView->AddColumn(3, szText, 300, LVCFMT_LEFT);

        // Unnesesary since the list updates on every TreeView selection
        // UpdateApplicationsList(ENUM_ALL_COMPONENTS);
    }

    VOID InitCategoriesList()
    {
        HTREEITEM hRootItemInstalled, hRootItemAvailable;

        hRootItemInstalled = m_TreeView->AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
        m_TreeView->AddCategory(hRootItemInstalled, IDS_APPLICATIONS, IDI_APPS);
        m_TreeView->AddCategory(hRootItemInstalled, IDS_UPDATES, IDI_APPUPD);

        m_TreeView->AddCategory(TVI_ROOT, IDS_SELECTEDFORINST, IDI_SELECTEDFORINST);

        hRootItemAvailable = m_TreeView->AddCategory(TVI_ROOT, IDS_AVAILABLEFORINST, IDI_CATEGORY);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_AUDIO, IDI_CAT_AUDIO);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_VIDEO, IDI_CAT_VIDEO);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_GRAPHICS, IDI_CAT_GRAPHICS);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_GAMES, IDI_CAT_GAMES);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_INTERNET, IDI_CAT_INTERNET);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_OFFICE, IDI_CAT_OFFICE);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_DEVEL, IDI_CAT_DEVEL);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_EDU, IDI_CAT_EDU);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_ENGINEER, IDI_CAT_ENGINEER);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_FINANCE, IDI_CAT_FINANCE);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_SCIENCE, IDI_CAT_SCIENCE);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_TOOLS, IDI_CAT_TOOLS);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_DRIVERS, IDI_CAT_DRIVERS);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_LIBS, IDI_CAT_LIBS);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_THEMES, IDI_CAT_THEMES);
        m_TreeView->AddCategory(hRootItemAvailable, IDS_CAT_OTHER, IDI_CAT_OTHER);

        m_TreeView->SetImageList();
        m_TreeView->Expand(hRootItemInstalled, TVE_EXPAND);
        m_TreeView->Expand(hRootItemAvailable, TVE_EXPAND);
        m_TreeView->SelectItem(hRootItemAvailable);
    }

    BOOL CreateStatusBar()
    {
        m_StatusBar = new CUiWindow<CStatusBar>();
        m_StatusBar->m_VerticalAlignment = UiAlign_RightBtm;
        m_StatusBar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_StatusBar);

        return m_StatusBar->Create(m_hWnd, (HMENU) IDC_STATUSBAR) != NULL;
    }

    BOOL CreateToolbar()
    {
        m_Toolbar = new CMainToolbar();
        m_Toolbar->m_VerticalAlignment = UiAlign_LeftTop;
        m_Toolbar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_Toolbar);

        return m_Toolbar->Create(m_hWnd) != NULL;
    }

    BOOL CreateTreeView()
    {
        m_TreeView = new CSideTreeView();
        m_TreeView->m_VerticalAlignment = UiAlign_Stretch;
        m_TreeView->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->First().Append(m_TreeView);

        return m_TreeView->Create(m_hWnd) != NULL;
    }

    BOOL CreateListView()
    {
        m_ListView = new CAppsListView();
        m_ListView->m_VerticalAlignment = UiAlign_Stretch;
        m_ListView->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->First().Append(m_ListView);

        return m_ListView->Create(m_hWnd) != NULL;
    }

    BOOL CreateRichEdit()
    {
        m_AppInfo = new CAppInfoDisplay();
        m_AppInfo->m_VerticalAlignment = UiAlign_Stretch;
        m_AppInfo->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->Second().Append(m_AppInfo);

        return m_AppInfo->Create(m_hWnd) != NULL;
    }

    BOOL CreateVSplitter()
    {
        m_VSplitter = new CUiSplitPanel();
        m_VSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_VSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->m_DynamicFirst = FALSE;
        m_VSplitter->m_Horizontal = FALSE;
        m_VSplitter->m_MinFirst = 0;
        m_VSplitter->m_MinSecond = 320;
        m_VSplitter->m_Pos = 240;
        m_ClientPanel->Children().Append(m_VSplitter);

        return m_VSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateHSplitter()
    {
        m_HSplitter = new CUiSplitPanel();
        m_HSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_HSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->m_DynamicFirst = TRUE;
        m_HSplitter->m_Horizontal = TRUE;
        m_HSplitter->m_Pos = INT_MAX; //set INT_MAX to use lowest possible position (m_MinSecond)
        m_HSplitter->m_MinFirst = 10;
        m_HSplitter->m_MinSecond = 140;
        m_VSplitter->Second().Append(m_HSplitter);

        return m_HSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateSearchBar()
    {
        m_SearchBar = new CUiWindow<CSearchBar>();
        m_SearchBar->m_VerticalAlignment = UiAlign_LeftTop;
        m_SearchBar->m_HorizontalAlignment = UiAlign_RightBtm;
        m_SearchBar->m_Margin.top = 4;
        m_SearchBar->m_Margin.right = 6;

        return m_SearchBar->Create(m_Toolbar->m_hWnd) != NULL;
    }

    BOOL CreateLayout()
    {
        BOOL b = TRUE;
        bUpdating = TRUE;

        m_ClientPanel = new CUiPanel();
        m_ClientPanel->m_VerticalAlignment = UiAlign_Stretch;
        m_ClientPanel->m_HorizontalAlignment = UiAlign_Stretch;

        // Top level
        b = b && CreateStatusBar();
        b = b && CreateToolbar();
        b = b && CreateSearchBar();
        b = b && CreateVSplitter();

        // Inside V Splitter
        b = b && CreateHSplitter();
        b = b && CreateTreeView();

        // Inside H Splitter
        b = b && CreateListView();
        b = b && CreateRichEdit();

        if (b)
        {
            RECT rTop;
            RECT rBottom;

            /* Size status bar */
            m_StatusBar->SendMessageW(WM_SIZE, 0, 0);

            /* Size tool bar */
            m_Toolbar->AutoSize();

            ::GetWindowRect(m_Toolbar->m_hWnd, &rTop);
            ::GetWindowRect(m_StatusBar->m_hWnd, &rBottom);

            m_VSplitter->m_Margin.top = rTop.bottom - rTop.top;
            m_VSplitter->m_Margin.bottom = rBottom.bottom - rBottom.top;
        }

        bUpdating = FALSE;
        return b;
    }

    BOOL InitControls()
    {
        if (CreateLayout())
        {

            InitApplicationsList();
            InitCategoriesList();

            nSelectedApps = 0;
            UpdateStatusBarText();

            return TRUE;
        }

        return FALSE;
    }

    VOID ShowAppInfo(INT Index)
    {
        if (IsInstalledEnum(SelectedEnumType))
        {
            if (Index == -1)
                Index = m_ListView->GetSelectionMark();

            PINSTALLED_INFO Info = (PINSTALLED_INFO) m_ListView->GetItemData(Index);

            m_AppInfo->ShowInstalledAppInfo(Info);
        }
        else if (IsAvailableEnum(SelectedEnumType))
        {
            if (Index == -1)
                return;

            CAvailableApplicationInfo* Info = (CAvailableApplicationInfo*) m_ListView->GetItemData(Index);

            m_AppInfo->ShowAvailableAppInfo(Info);
        }
    }

    VOID OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (wParam == SIZE_MINIMIZED)
            return;

        /* Size status bar */
        m_StatusBar->SendMessage(WM_SIZE, 0, 0);

        /* Size tool bar */
        m_Toolbar->AutoSize();

        /* Automatically hide captions */
        DWORD dToolbarTreshold = m_Toolbar->GetMaxButtonsWidth();
        DWORD dSearchbarMargin = (LOWORD(lParam) - m_SearchBar->m_Width);

        if (dSearchbarMargin > dToolbarTreshold)
        {
            m_Toolbar->ShowButtonCaption();
        }
        else if (dSearchbarMargin < dToolbarTreshold)
        {
            m_Toolbar->HideButtonCaption();
        }

        RECT r = {0, 0, LOWORD(lParam), HIWORD(lParam)};
        HDWP hdwp = NULL;
        INT count = m_ClientPanel->CountSizableChildren();

        hdwp = BeginDeferWindowPos(count);
        if (hdwp)
        {
            hdwp = m_ClientPanel->OnParentSize(r, hdwp);
            if (hdwp)
            {
                EndDeferWindowPos(hdwp);
            }

        }

        // TODO: Sub-layouts for children of children
        count = m_SearchBar->CountSizableChildren();
        hdwp = BeginDeferWindowPos(count);
        if (hdwp)
        {
            hdwp = m_SearchBar->OnParentSize(r, hdwp);
            if (hdwp)
            {
                EndDeferWindowPos(hdwp);
            }
        }

    }

    VOID RemoveSelectedAppFromRegistry()
    {
        PINSTALLED_INFO Info;
        WCHAR szFullName[MAX_PATH] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
        ATL::CStringW szMsgText, szMsgTitle;
        INT ItemIndex = m_ListView->GetNextItem(-1, LVNI_FOCUSED);

        if (!IsInstalledEnum(SelectedEnumType))
            return;

        Info = reinterpret_cast<PINSTALLED_INFO>(m_ListView->GetItemData(ItemIndex));
        if (!Info || !Info->hSubKey || (ItemIndex == -1)) 
            return;

        if (!szMsgText.LoadStringW(IDS_APP_REG_REMOVE) ||
            !szMsgTitle.LoadStringW(IDS_INFORMATION))
            return;

        if (MessageBoxW(szMsgText, szMsgTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            ATL::CStringW::CopyChars(szFullName,
                                     MAX_PATH,
                                     Info->szKeyName.GetString(),
                                     MAX_PATH - wcslen(szFullName));

            if (RegDeleteKeyW(Info->hRootKey, szFullName) == ERROR_SUCCESS)
            {
                m_ListView->DeleteItem(ItemIndex);
                return;
            }

            if (!szMsgText.LoadStringW(IDS_UNABLE_TO_REMOVE))
                return;

            MessageBoxW(szMsgText.GetString(), NULL, MB_OK | MB_ICONERROR);
        }
    }

    BOOL UninstallSelectedApp(BOOL bModify)
    {
        WCHAR szAppName[MAX_STR_LEN];

        if (!IsInstalledEnum(SelectedEnumType))
            return FALSE;

        INT ItemIndex = m_ListView->GetNextItem(-1, LVNI_FOCUSED);
        if (ItemIndex == -1)
            return FALSE;

        m_ListView->GetItemText(ItemIndex, 0, szAppName, _countof(szAppName));
        WriteLogMessage(EVENTLOG_SUCCESS, MSG_SUCCESS_REMOVE, szAppName);

        PINSTALLED_INFO ItemInfo = (PINSTALLED_INFO)m_ListView->GetItemData(ItemIndex);
        return UninstallApplication(ItemInfo, bModify);
    }
    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (Msg)
        {
        case WM_CREATE:
            if (!InitControls())
                ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_DESTROY:
        {
            ShowWindow(SW_HIDE);
            SaveSettings(hwnd);

            FreeLogs();
            m_AvailableApps.FreeCachedEntries();

            if (IsInstalledEnum(SelectedEnumType))
                FreeInstalledAppList();

            delete m_ClientPanel;

            PostQuitMessage(0);
            return 0;
        }

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR) lParam;

            switch (data->code)
            {
            case TVN_SELCHANGED:
            {
                if (data->hwndFrom == m_TreeView->m_hWnd)
                {
                    switch (((LPNMTREEVIEW) lParam)->itemNew.lParam)
                    {
                    case IDS_INSTALLED:
                        UpdateApplicationsList(ENUM_ALL_INSTALLED);
                        break;

                    case IDS_APPLICATIONS:
                        UpdateApplicationsList(ENUM_INSTALLED_APPLICATIONS);
                        break;

                    case IDS_UPDATES:
                        UpdateApplicationsList(ENUM_UPDATES);
                        break;

                    case IDS_AVAILABLEFORINST:
                        UpdateApplicationsList(ENUM_ALL_AVAILABLE);
                        break;

                    case IDS_CAT_AUDIO:
                        UpdateApplicationsList(ENUM_CAT_AUDIO);
                        break;

                    case IDS_CAT_DEVEL:
                        UpdateApplicationsList(ENUM_CAT_DEVEL);
                        break;

                    case IDS_CAT_DRIVERS:
                        UpdateApplicationsList(ENUM_CAT_DRIVERS);
                        break;

                    case IDS_CAT_EDU:
                        UpdateApplicationsList(ENUM_CAT_EDU);
                        break;

                    case IDS_CAT_ENGINEER:
                        UpdateApplicationsList(ENUM_CAT_ENGINEER);
                        break;

                    case IDS_CAT_FINANCE:
                        UpdateApplicationsList(ENUM_CAT_FINANCE);
                        break;

                    case IDS_CAT_GAMES:
                        UpdateApplicationsList(ENUM_CAT_GAMES);
                        break;

                    case IDS_CAT_GRAPHICS:
                        UpdateApplicationsList(ENUM_CAT_GRAPHICS);
                        break;

                    case IDS_CAT_INTERNET:
                        UpdateApplicationsList(ENUM_CAT_INTERNET);
                        break;

                    case IDS_CAT_LIBS:
                        UpdateApplicationsList(ENUM_CAT_LIBS);
                        break;

                    case IDS_CAT_OFFICE:
                        UpdateApplicationsList(ENUM_CAT_OFFICE);
                        break;

                    case IDS_CAT_OTHER:
                        UpdateApplicationsList(ENUM_CAT_OTHER);
                        break;

                    case IDS_CAT_SCIENCE:
                        UpdateApplicationsList(ENUM_CAT_SCIENCE);
                        break;

                    case IDS_CAT_TOOLS:
                        UpdateApplicationsList(ENUM_CAT_TOOLS);
                        break;

                    case IDS_CAT_VIDEO:
                        UpdateApplicationsList(ENUM_CAT_VIDEO);
                        break;

                    case IDS_CAT_THEMES:
                        UpdateApplicationsList(ENUM_CAT_THEMES);
                        break;

                    case IDS_SELECTEDFORINST:
                        UpdateApplicationsList(ENUM_CAT_SELECTED);
                        break;
                    }
                }

                HMENU mainMenu = ::GetMenu(hwnd);
                HMENU lvwMenu = ::GetMenu(m_ListView->m_hWnd);

                /* Disable/enable items based on treeview selection */
                if (IsSelectedNodeInstalled())
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_ENABLED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_ENABLED);

                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, TRUE);
                }
                else
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_GRAYED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_GRAYED);

                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, FALSE);
                }
            }
            break;

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW pnic = (LPNMLISTVIEW) lParam;

                if (pnic->hdr.hwndFrom == m_ListView->m_hWnd)
                {
                    /* Check if this is a valid item
                    * (technically, it can be also an unselect) */
                    INT ItemIndex = pnic->iItem;
                    if (ItemIndex == -1 ||
                        ItemIndex >= ListView_GetItemCount(pnic->hdr.hwndFrom))
                    {
                        break;
                    }

                    /* Check if the focus has been moved to another item */
                    if ((pnic->uChanged & LVIF_STATE) &&
                        (pnic->uNewState & LVIS_FOCUSED) &&
                        !(pnic->uOldState & LVIS_FOCUSED))
                    {
                        ShowAppInfo(ItemIndex);
                    }
                    /* Check if the item is checked */
                    if ((pnic->uNewState & LVIS_STATEIMAGEMASK) && !bUpdating)
                    {
                        BOOL checked = m_ListView->GetCheckState(pnic->iItem);
                        /* FIXME: HAX!
                        - preventing decremention below zero as a safeguard for ReactOS
                          In ReactOS this action is triggered whenever user changes *selection*, but should be only when *checkbox* state toggled
                          Maybe LVIS_STATEIMAGEMASK is set incorrectly
                        */
                        nSelectedApps +=
                            (checked)
                            ? 1
                            : ((nSelectedApps > 0)
                               ? -1
                               : 0);

                        /* Update item's selection status */
                        m_ListView->SetSelected(pnic->iItem, checked);

                        UpdateStatusBarText();
                    }
                }
            }
            break;

            case LVN_COLUMNCLICK:
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                m_ListView->ColumnClick(pnmv);
            }
            break;

            case NM_CLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    ShowAppInfo(-1);
                }
            }
            break;

            case NM_DBLCLK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    /* this won't do anything if the program is already installed */
                    SendMessageW(hwnd, WM_COMMAND, ID_INSTALL, 0);
                }
            }
            break;

            case NM_RCLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    ShowPopupMenu(m_ListView->m_hWnd, 0, ID_INSTALL);
                }
            }
            break;

            

            case TTN_GETDISPINFO:
                m_Toolbar->OnGetDispInfo((LPTOOLTIPTEXT) lParam);
                break;
            }
        }
        break;

        case WM_SIZE:
            OnSize(hwnd, wParam, lParam);
            break;

        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT) lParam;

            if (pRect->right - pRect->left < 565)
                pRect->right = pRect->left + 565;

            if (pRect->bottom - pRect->top < 300)
                pRect->bottom = pRect->top + 300;

            return TRUE;
        }

        case WM_SYSCOLORCHANGE:
        {
            /* Forward WM_SYSCOLORCHANGE to common controls */
            m_ListView->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_TreeView->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_Toolbar->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_ListView->SendMessageW(EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
        }
        break;

        case WM_TIMER:
            if (wParam == SEARCH_TIMER_ID)
            {
                ::KillTimer(hwnd, SEARCH_TIMER_ID);
                if (bSearchEnabled)
                    UpdateApplicationsList(-1);
            }
            break;
        }

        return FALSE;
    }

    BOOL IsSelectedNodeInstalled()
    {
        HTREEITEM hSelectedItem = m_TreeView->GetSelection();
        TV_ITEM tItem;

        tItem.mask = TVIF_PARAM | TVIF_HANDLE;
        tItem.hItem = hSelectedItem;
        m_TreeView->GetItem(&tItem);
        switch (tItem.lParam)
        {
        case IDS_INSTALLED:
        case IDS_APPLICATIONS:
        case IDS_UPDATES:
            return TRUE;
        default:
            return FALSE;
        }
    }

    VOID ShowAboutDlg()
    {
        ATL::CStringW szApp;
        ATL::CStringW szAuthors;
        HICON hIcon;

        szApp.LoadStringW(IDS_APPTITLE);
        szAuthors.LoadStringW(IDS_APP_AUTHORS);
        hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
        ShellAboutW(m_hWnd, szApp, szAuthors, hIcon);
        DestroyIcon(hIcon);
    }

    VOID OnCommand(WPARAM wParam, LPARAM lParam)
    {
        WORD wCommand = LOWORD(wParam);

        if (lParam == (LPARAM) m_SearchBar->m_hWnd)
        {
            ATL::CStringW szBuf;

            switch (HIWORD(wParam))
            {
            case EN_SETFOCUS:
            {
                ATL::CStringW szWndText;

                szBuf.LoadStringW(IDS_SEARCH_TEXT);
                m_SearchBar->GetWindowTextW(szWndText);
                if (szBuf == szWndText)
                {
                    bSearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(L"");
                }
            }
            break;

            case EN_KILLFOCUS:
            {
                m_SearchBar->GetWindowTextW(szBuf);
                if (szBuf.IsEmpty())
                {
                    szBuf.LoadStringW(IDS_SEARCH_TEXT);
                    bSearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(szBuf.GetString());
                }
            }
            break;

            case EN_CHANGE:
            {
                ATL::CStringW szWndText;

                if (!bSearchEnabled)
                {
                    bSearchEnabled = TRUE;
                    break;
                }

                szBuf.LoadStringW(IDS_SEARCH_TEXT);
                m_SearchBar->GetWindowTextW(szWndText);
                if (szBuf == szWndText)
                {
                    szSearchPattern.Empty();
                }
                else
                {
                    szSearchPattern = szWndText;
                }

                DWORD dwDelay;
                SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
                SetTimer(SEARCH_TIMER_ID, dwDelay);
            }
            break;
            }

            return;
        }

        switch (wCommand)
        {
        case ID_SETTINGS:
            CreateSettingsDlg(m_hWnd);
            break;

        case ID_EXIT:
            PostMessageW(WM_CLOSE, 0, 0);
            break;

        case ID_SEARCH:
            m_SearchBar->SetFocus();
            break;

        case ID_INSTALL:
            if (IsAvailableEnum(SelectedEnumType))
            {
                if (nSelectedApps > 0)
                {
                    DownloadListOfApplications(m_AvailableApps.GetSelected(), FALSE);
                    UpdateApplicationsList(-1);
                    m_ListView->SetSelected(-1, FALSE);
                }
                else if (DownloadApplication(m_ListView->GetSelectedData(), FALSE))
                {
                    UpdateApplicationsList(-1);
                }

            }
            break;

        case ID_UNINSTALL:
            if (UninstallSelectedApp(FALSE))
                UpdateApplicationsList(-1);
            break;

        case ID_MODIFY:
            if (UninstallSelectedApp(TRUE))
                UpdateApplicationsList(-1);
            break;

        case ID_REGREMOVE:
            RemoveSelectedAppFromRegistry();
            break;

        case ID_REFRESH:
            UpdateApplicationsList(-1);
            break;

        case ID_RESETDB:
            CAvailableApps::ForceUpdateAppsDB();
            UpdateApplicationsList(-1);
            break;

        case ID_HELP:
            MessageBoxW(L"Help not implemented yet", NULL, MB_OK);
            break;

        case ID_ABOUT:
            ShowAboutDlg();
            break;

        case ID_CHECK_ALL:
            m_ListView->CheckAll();
            break;
        }
    }

    VOID FreeInstalledAppList()
    {
        INT Count = m_ListView->GetItemCount() - 1;
        PINSTALLED_INFO Info;

        while (Count >= 0)
        {
            Info = (PINSTALLED_INFO) m_ListView->GetItemData(Count);
            if (Info)
            {
                RegCloseKey(Info->hSubKey);
                delete Info;
            }
            Count--;
        }
    }

    static BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle)
    {
        if (!*szNeedle)
            return TRUE;
        /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
        return StrStrIW(szHaystack, szNeedle) != NULL;
    }

    BOOL CALLBACK EnumInstalledAppProc(INT ItemIndex, ATL::CStringW &m_szName, PINSTALLED_INFO Info)
    {
        PINSTALLED_INFO ItemInfo;
        ATL::CStringW szText;
        INT Index;

        if (!SearchPatternMatch(m_szName.GetString(), szSearchPattern))
        {
            RegCloseKey(Info->hSubKey);
            return TRUE;
        }

        ItemInfo = new INSTALLED_INFO(*Info);
        if (!ItemInfo)
        {
            RegCloseKey(Info->hSubKey);
            return FALSE;
        }

        Index = m_ListView->AddItem(ItemIndex, 0, m_szName.GetString(), (LPARAM) ItemInfo);

        /* Get version info */
        ItemInfo->GetApplicationString(L"DisplayVersion", szText);
        m_ListView->SetItemText(Index, 1, szText.GetString());

        /* Get comments */
        ItemInfo->GetApplicationString(L"Comments", szText);
        m_ListView->SetItemText(Index, 2, szText.GetString());

        return TRUE;
    }

    BOOL EnumAvailableAppProc(CAvailableApplicationInfo* Info, LPCWSTR szFolderPath)
    {
        INT Index;
        HICON hIcon = NULL;

        HIMAGELIST hImageListView = (HIMAGELIST)m_ListView->SendMessage(LVM_GETIMAGELIST, LVSIL_SMALL, 0);

        if (!SearchPatternMatch(Info->m_szName.GetString(), szSearchPattern) &&
            !SearchPatternMatch(Info->m_szDesc.GetString(), szSearchPattern))
        {
            return TRUE;
        }

        /* Load icon from file */
        ATL::CStringW szIconPath = szFolderPath;
        PathAppendW(szIconPath.GetBuffer(MAX_PATH), L"icons");
        PathAppendW(szIconPath.GetBuffer(), Info->m_szName.GetString());
        PathAddExtensionW(szIconPath.GetBuffer(), L".ico");
        szIconPath.ReleaseBuffer();

        hIcon = (HICON) LoadImageW(NULL,
                                   szIconPath.GetString(),
                                   IMAGE_ICON,
                                   LISTVIEW_ICON_SIZE,
                                   LISTVIEW_ICON_SIZE,
                                   LR_LOADFROMFILE);

        if (!hIcon || GetLastError() != ERROR_SUCCESS)
        {
            /* Load default icon */
            hIcon = (HICON) LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
        }

        Index = ImageList_AddIcon(hImageListView, hIcon);
        DestroyIcon(hIcon);

        Index = m_ListView->AddItem(Info->m_Category, Index, Info->m_szName.GetString(), (LPARAM) Info);
        m_ListView->SetImageList(hImageListView, LVSIL_SMALL);
        m_ListView->SetItemText(Index, 1, Info->m_szVersion.GetString());
        m_ListView->SetItemText(Index, 2, Info->m_szDesc.GetString());
        m_ListView->SetCheckState(Index, Info->m_IsSelected);

        return TRUE;
    }

    static BOOL CALLBACK s_EnumInstalledAppProc(INT ItemIndex, ATL::CStringW &m_szName, PINSTALLED_INFO Info, PVOID param)
    {
        CMainWindow* pThis = (CMainWindow*)param;
        return pThis->EnumInstalledAppProc(ItemIndex, m_szName, Info);
    }

    static BOOL CALLBACK s_EnumAvailableAppProc(CAvailableApplicationInfo* Info, LPCWSTR szFolderPath, PVOID param)
    {
        CMainWindow* pThis = (CMainWindow*)param;
        return pThis->EnumAvailableAppProc(Info, szFolderPath);
    }

    VOID UpdateStatusBarText()
    {
        if (m_StatusBar)
        {
            ATL::CStringW szBuffer;

            szBuffer.Format(IDS_APPS_COUNT, m_ListView->GetItemCount(), nSelectedApps);
            m_StatusBar->SetText(szBuffer);
        }
    }

    VOID UpdateApplicationsList(INT EnumType)
    {
        ATL::CStringW szBuffer1, szBuffer2;
        HIMAGELIST hImageListView;
        BOOL bWasInInstalled = IsInstalledEnum(SelectedEnumType);

        bUpdating = TRUE;
        m_ListView->SetRedraw(FALSE);

        if (EnumType < 0)
        {
            EnumType = SelectedEnumType;
        }

        //if previous one was INSTALLED purge the list
        //TODO: make the Installed category a separate class to avoid doing this
        if (bWasInInstalled)
        {
            FreeInstalledAppList();
        }

        m_ListView->DeleteAllItems();

        // Create new ImageList
        hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
                                          LISTVIEW_ICON_SIZE,
                                          GetSystemColorDepth() | ILC_MASK,
                                          0, 1);
        HIMAGELIST hImageListBuf = m_ListView->SetImageList(hImageListView, LVSIL_SMALL);
        if (hImageListBuf)
        {
            ImageList_Destroy(hImageListBuf);
        }

        if (IsInstalledEnum(EnumType))
        {
            if (!bWasInInstalled)
            {
                m_ListView->SetCheckboxesVisible(FALSE);
            }

            HICON hIcon = (HICON) LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
            ImageList_AddIcon(hImageListView, hIcon);
            DestroyIcon(hIcon);

            // Enum installed applications and updates
            EnumInstalledApplications(EnumType, TRUE, s_EnumInstalledAppProc, this);
            EnumInstalledApplications(EnumType, FALSE, s_EnumInstalledAppProc, this);
        }
        else if (IsAvailableEnum(EnumType))
        {
            if (bWasInInstalled)
            {
                m_ListView->SetCheckboxesVisible(TRUE);
            }

            // Enum available applications
            m_AvailableApps.Enum(EnumType, s_EnumAvailableAppProc, this);
        }

        SelectedEnumType = EnumType;
        UpdateStatusBarText();
        m_AppInfo->SetWelcomeText();

        // Set automatic column width for program names if the list is not empty
        if (m_ListView->GetItemCount() > 0)
        {
            ListView_SetColumnWidth(m_ListView->GetWindow(), 0, LVSCW_AUTOSIZE);
        }

        bUpdating = FALSE;
        m_ListView->SetRedraw(TRUE);
    }

public:
    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
        static ATL::CWndClassInfo wc =
        {
            {
                sizeof(WNDCLASSEX),
                csStyle,
                StartWindowProc,
                0,
                0,
                NULL,
                LoadIconW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDI_MAIN)),
                LoadCursorW(NULL, IDC_ARROW),
                (HBRUSH) (COLOR_BTNFACE + 1),
                MAKEINTRESOURCEW(IDR_MAINMENU),
                L"RAppsWnd",
                NULL
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    HWND Create()
    {
        ATL::CStringW szWindowName;
        szWindowName.LoadStringW(IDS_APPTITLE);

        RECT r = {
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Left : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Top : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Width : 680),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450)
        };
        r.right += r.left;
        r.bottom += r.top;

        return CWindowImpl::Create(NULL, r, szWindowName.GetString(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    }

    void HandleTabOrder(int direction)
    {
        HWND Controls[] = { m_Toolbar->m_hWnd, m_SearchBar->m_hWnd, m_TreeView->m_hWnd, m_ListView->m_hWnd, m_AppInfo->m_hWnd };
        // When there is no control found, go to the first or last (depending on tab vs shift-tab)
        int current = direction > 0 ? 0 : (_countof(Controls) - 1);
        HWND hActive = ::GetFocus();
        for (size_t n = 0; n < _countof(Controls); ++n)
        {
            if (hActive == Controls[n])
            {
                current = n + direction;
                break;
            }
        }

        if (current < 0)
            current = (_countof(Controls) - 1);
        else if ((UINT)current >= _countof(Controls))
            current = 0;

        ::SetFocus(Controls[current]);
    }
};

VOID ShowMainWindow(INT nShowCmd)
{
    HACCEL KeyBrd;
    MSG Msg;

    CMainWindow* wnd = new CMainWindow();
    if (!wnd)
        return;

    hMainWnd = wnd->Create();
    if (!hMainWnd)
        return;

    /* Maximize it if we must */
    wnd->ShowWindow((SettingsInfo.bSaveWndPos && SettingsInfo.Maximized) ? SW_MAXIMIZE : nShowCmd);
    wnd->UpdateWindow();

    /* Load the menu hotkeys */
    KeyBrd = LoadAcceleratorsW(NULL, MAKEINTRESOURCEW(HOTKEYS));

    /* Message Loop */
    while (GetMessageW(&Msg, NULL, 0, 0))
    {
        if (!TranslateAcceleratorW(hMainWnd, KeyBrd, &Msg))
        {
            if (Msg.message == WM_CHAR &&
                Msg.wParam == VK_TAB)
            {
                // Move backwards if shift is held down
                int direction = (GetKeyState(VK_SHIFT) & 0x8000) ? -1 : 1;

                wnd->HandleTabOrder(direction);
                continue;
            }

            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }
    }

    delete wnd;
}
