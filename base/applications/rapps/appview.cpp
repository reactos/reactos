/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Application view class and other classes used by it
 * COPYRIGHT:   Copyright 2020 He Yang            (1160386205@qq.com)
 */

#include "rapps.h"
#include "appview.h"
#include "gui.h"

// **** CAppRichEdit ****

VOID CAppRichEdit::LoadAndInsertText(UINT uStringID,
    const ATL::CStringW &szText,
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

VOID CAppRichEdit::LoadAndInsertText(UINT uStringID,
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

VOID CAppRichEdit::InsertVersionInfo(CAvailableApplicationInfo *Info)
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

VOID CAppRichEdit::InsertLicenseInfo(CAvailableApplicationInfo *Info)
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

VOID CAppRichEdit::InsertLanguageInfo(CAvailableApplicationInfo *Info)
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

BOOL CAppRichEdit::ShowAvailableAppInfo(CAvailableApplicationInfo *Info)
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

inline VOID CAppRichEdit::InsertTextWithString(UINT StringID, DWORD StringFlags, const ATL::CStringW &Text, DWORD TextFlags)
{
    if (!Text.IsEmpty())
    {
        LoadAndInsertText(StringID, Text, StringFlags, TextFlags);
    }
}

BOOL CAppRichEdit::ShowInstalledAppInfo(CInstalledApplicationInfo *Info)
{
    if (!Info) return FALSE;

    SetText(Info->szDisplayName, CFE_BOLD);
    InsertText(L"\n", 0);

    InsertTextWithString(IDS_INFO_VERSION, CFE_BOLD, Info->szDisplayVersion, 0);
    InsertTextWithString(IDS_INFO_PUBLISHER, CFE_BOLD, Info->szPublisher, 0);
    InsertTextWithString(IDS_INFO_REGOWNER, CFE_BOLD, Info->szRegOwner, 0);
    InsertTextWithString(IDS_INFO_PRODUCTID, CFE_BOLD, Info->szProductID, 0);
    InsertTextWithString(IDS_INFO_HELPLINK, CFE_BOLD, Info->szHelpLink, CFM_LINK);
    InsertTextWithString(IDS_INFO_HELPPHONE, CFE_BOLD, Info->szHelpTelephone, 0);
    InsertTextWithString(IDS_INFO_README, CFE_BOLD, Info->szReadme, 0);
    InsertTextWithString(IDS_INFO_CONTACT, CFE_BOLD, Info->szContact, 0);
    InsertTextWithString(IDS_INFO_UPDATEINFO, CFE_BOLD, Info->szURLUpdateInfo, CFM_LINK);
    InsertTextWithString(IDS_INFO_INFOABOUT, CFE_BOLD, Info->szURLInfoAbout, CFM_LINK);
    InsertTextWithString(IDS_INFO_COMMENTS, CFE_BOLD, Info->szComments, 0);
    InsertTextWithString(IDS_INFO_INSTALLDATE, CFE_BOLD, Info->szInstallDate, 0);
    InsertTextWithString(IDS_INFO_INSTLOCATION, CFE_BOLD, Info->szInstallLocation, 0);
    InsertTextWithString(IDS_INFO_INSTALLSRC, CFE_BOLD, Info->szInstallSource, 0);
    InsertTextWithString(IDS_INFO_UNINSTALLSTR, CFE_BOLD, Info->szUninstallString, 0);
    InsertTextWithString(IDS_INFO_MODIFYPATH, CFE_BOLD, Info->szModifyPath, 0);

    return TRUE;
}

VOID CAppRichEdit::SetWelcomeText()
{
    ATL::CStringW szText;

    szText.LoadStringW(IDS_WELCOME_TITLE);
    SetText(szText, CFE_BOLD);

    szText.LoadStringW(IDS_WELCOME_TEXT);
    InsertText(szText, 0);

    szText.LoadStringW(IDS_WELCOME_URL);
    InsertText(szText, CFM_LINK);
}
// **** CAppRichEdit ****


int ScrnshotDownloadCallback(
    pASYNCINET AsyncInet,
    ASYNC_EVENT Event,
    WPARAM wParam,
    LPARAM lParam,
    VOID *Extension
)
{
    ScrnshotDownloadParam *DownloadParam = (ScrnshotDownloadParam *)Extension;
    switch (Event)
    {
    case ASYNCINET_DATA:
        DWORD BytesWritten;
        WriteFile(DownloadParam->hFile, (LPCVOID)wParam, (DWORD)lParam, &BytesWritten, NULL);
        break;
    case ASYNCINET_COMPLETE:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_RAPPS_DOWNLOAD_COMPLETE, (WPARAM)ERROR_SUCCESS, (LPARAM)DownloadParam);
        break;
    case ASYNCINET_CANCELLED:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_RAPPS_DOWNLOAD_COMPLETE, (WPARAM)ERROR_CANCELLED, (LPARAM)DownloadParam);
        break;
    case ASYNCINET_ERROR:
        CloseHandle(DownloadParam->hFile);
        SendMessage(DownloadParam->hwndNotify, WM_RAPPS_DOWNLOAD_COMPLETE, wParam, (LPARAM)DownloadParam);
        break;
    default:
        ATLASSERT(FALSE);
        break;
    }
    return 0;
}


// **** CAppScrnshotPreview ****

BOOL CAppScrnshotPreview::ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId)
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
    case WM_RAPPS_DOWNLOAD_COMPLETE:
    {
        ScrnshotDownloadParam *DownloadParam = (ScrnshotDownloadParam *)lParam;
        AsyncInetRelease(AsyncInet);
        AsyncInet = NULL;
        switch (wParam)
        {
        case ERROR_SUCCESS:
            if (ContentID == DownloadParam->ID)
            {
                DisplayFile(DownloadParam->DownloadFileName);
                // send a message to trigger resizing
                ::SendMessageW(::GetParent(m_hWnd), WM_RAPPS_RESIZE_CHILDREN, 0, 0);
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
            ::SendMessageW(::GetParent(m_hWnd), WM_RAPPS_RESIZE_CHILDREN, 0, 0);
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

VOID CAppScrnshotPreview::DisplayLoading()
{
    SetStatus(SCRNSHOT_PREV_LOADING);
    if (bLoadingTimerOn)
    {
        KillTimer(TIMER_LOADING_ANIMATION);
    }
    LoadingAnimationFrame = 0;
    bLoadingTimerOn = TRUE;
    SetTimer(TIMER_LOADING_ANIMATION, 1000 / LOADING_ANIMATION_FPS, 0);
}

VOID CAppScrnshotPreview::DisplayFailed()
{
    InterlockedIncrement64(&ContentID);
    SetStatus(SCRNSHOT_PREV_FAILED);
    PreviousDisplayCleanup();
}

BOOL CAppScrnshotPreview::DisplayFile(LPCWSTR lpszFileName)
{
    PreviousDisplayCleanup();
    SetStatus(SCRNSHOT_PREV_IMAGE);
    pImage = Bitmap::FromFile(lpszFileName, 0);
    if (pImage->GetLastStatus() != Ok)
    {
        DisplayFailed();
        return FALSE;
    }
    return TRUE;
}

VOID CAppScrnshotPreview::SetStatus(SCRNSHOT_STATUS Status)
{
    ScrnshotPrevStauts = Status;
}

VOID CAppScrnshotPreview::PaintOnDC(HDC hdc, int width, int height, BOOL bDrawBkgnd)
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

    switch (ScrnshotPrevStauts)
    {
    case SCRNSHOT_PREV_EMPTY:
    {

    }
    break;

    case SCRNSHOT_PREV_LOADING:
    {
        Graphics graphics(hdcMem);
        Color color(255, 0, 0);
        SolidBrush dotBrush(Color(255, 100, 100, 100));

        graphics.SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);

        // Paint three dot
        float DotWidth = GetLoadingDotWidth(width, height);
        graphics.FillEllipse((Brush *)(&dotBrush),
            (REAL)width / 2.0 - min(width, height) * 2.0 / 16.0 - DotWidth / 2.0,
            (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame + LOADING_ANIMATION_FPS / 4, width, height) - DotWidth / 2.0,
            DotWidth,
            DotWidth);

        graphics.FillEllipse((Brush *)(&dotBrush),
            (REAL)width / 2.0 - DotWidth / 2.0,
            (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame, width, height) - DotWidth / 2.0,
            DotWidth,
            DotWidth);

        graphics.FillEllipse((Brush *)(&dotBrush),
            (REAL)width / 2.0 + min(width, height) * 2.0 / 16.0 - DotWidth / 2.0,
            (REAL)height / 2.0 - GetFrameDotShift(LoadingAnimationFrame - LOADING_ANIMATION_FPS / 4, width, height) - DotWidth / 2.0,
            DotWidth,
            DotWidth);
    }
    break;

    case SCRNSHOT_PREV_IMAGE:
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

    case SCRNSHOT_PREV_FAILED:
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

float CAppScrnshotPreview::GetLoadingDotWidth(int width, int height)
{
    return min(width, height) / 20.0;
}

float CAppScrnshotPreview::GetFrameDotShift(int Frame, int width, int height)
{
    return min(width, height) *
        (1.0 / 16.0) *
        (2.0 / (2.0 - sqrt(3.0))) *
        (max(sin((float)Frame * 2 * PI / (LOADING_ANIMATION_PERIOD * LOADING_ANIMATION_FPS)), sqrt(3.0) / 2.0) - sqrt(3.0) / 2.0);
}

ATL::CWndClassInfo &CAppScrnshotPreview::GetWndClassInfo()
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
            L"RAppsScrnshotPreview",
            NULL
        },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

HWND CAppScrnshotPreview::Create(HWND hParent)
{
    RECT r = { 0,0,0,0 };

    return CWindowImpl::Create(hParent, r, L"", WS_CHILD | WS_VISIBLE);
}

VOID CAppScrnshotPreview::PreviousDisplayCleanup()
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
    }
    if (!TempImagePath.IsEmpty())
    {
        DeleteFileW(TempImagePath.GetString());
        TempImagePath.Empty();
    }
}

VOID CAppScrnshotPreview::DisplayEmpty()
{
    InterlockedIncrement64(&ContentID);
    SetStatus(SCRNSHOT_PREV_EMPTY);
    PreviousDisplayCleanup();
}

BOOL CAppScrnshotPreview::DisplayImage(LPCWSTR lpszLocation)
{
    LONGLONG ID = InterlockedIncrement64(&ContentID);
    PreviousDisplayCleanup();

    if (PathIsURLW(lpszLocation))
    {
        DisplayLoading();

        ScrnshotDownloadParam *DownloadParam = new ScrnshotDownloadParam;
        if (!DownloadParam) return FALSE;

        DownloadParam->hwndNotify = m_hWnd;
        DownloadParam->ID = ID;
        // generate a filename
        ATL::CStringW ScrnshotFolder = CAvailableApps::m_Strings.szAppsPath;
        PathAppendW(ScrnshotFolder.GetBuffer(MAX_PATH), L"screenshots");
        ScrnshotFolder.ReleaseBuffer();

        if (!PathIsDirectoryW(ScrnshotFolder.GetString()))
        {
            CreateDirectoryW(ScrnshotFolder.GetString(), NULL);
        }

        if (!GetTempFileNameW(ScrnshotFolder.GetString(), L"img",
            0, DownloadParam->DownloadFileName.GetBuffer(MAX_PATH)))
        {
            DownloadParam->DownloadFileName.ReleaseBuffer();
            delete DownloadParam;
            DisplayFailed();
            return FALSE;
        }
        DownloadParam->DownloadFileName.ReleaseBuffer();

        DownloadParam->hFile = CreateFileW(DownloadParam->DownloadFileName.GetString(),
            GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (DownloadParam->hFile == INVALID_HANDLE_VALUE)
        {
            delete DownloadParam;
            DisplayFailed();
            return FALSE;
        }

        AsyncInet = AsyncInetDownload(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, lpszLocation, TRUE, ScrnshotDownloadCallback, DownloadParam);
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

int CAppScrnshotPreview::GetRequestedWidth(int Height) // calculate requested window width by given height
{
    switch (ScrnshotPrevStauts)
    {
    case SCRNSHOT_PREV_EMPTY:
        return 0;
    case SCRNSHOT_PREV_LOADING:
        return 200;
    case SCRNSHOT_PREV_IMAGE:
        if (pImage)
        {
            // return the width needed to display image inside the window.
            // and always keep window w/h ratio inside [ 1/SCRNSHOT_MAX_ASPECT_RAT, SCRNSHOT_MAX_ASPECT_RAT ]
            return (int)floor((float)Height *
                max(min((float)pImage->GetWidth() / (float)pImage->GetHeight(), (float)SCRNSHOT_MAX_ASPECT_RAT), 1.0 / (float)SCRNSHOT_MAX_ASPECT_RAT));
        }
        return 0;
    case SCRNSHOT_PREV_FAILED:
        return 200;
    default:
        return 0;
    }
}

CAppScrnshotPreview::~CAppScrnshotPreview()
{
    PreviousDisplayCleanup();
}
// **** CAppScrnshotPreview ****


// **** CAppInfoDisplay ****

BOOL CAppInfoDisplay::ProcessWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId)
{
    theResult = 0;
    switch (message)
    {
    case WM_CREATE:
    {
        RichEdit = new CAppRichEdit();
        RichEdit->Create(hwnd);

        ScrnshotPrev = new CAppScrnshotPreview();
        ScrnshotPrev->Create(hwnd);
        break;
    }
    case WM_SIZE:
    {
        ResizeChildren(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    }
    case WM_RAPPS_RESIZE_CHILDREN:
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
        NMHDR *NotifyHeader = (NMHDR *)lParam;
        if (NotifyHeader->hwndFrom == RichEdit->m_hWnd)
        {
            switch (NotifyHeader->code)
            {
            case EN_LINK:
                OnLink((ENLINK *)lParam);
                break;
            }
        }
        break;
    }
    }

    return FALSE;
}

VOID CAppInfoDisplay::ResizeChildren()
{
    CRect rect;
    GetWindowRect(&rect);
    ResizeChildren(rect.Width(), rect.Height());
}

VOID CAppInfoDisplay::ResizeChildren(int Width, int Height)
{
    int ScrnshotWidth = ScrnshotPrev->GetRequestedWidth(Height);

    // make sure richedit always have room to display
    ScrnshotWidth = min(ScrnshotWidth, Width - INFO_DISPLAY_PADDING - RICHEDIT_MIN_WIDTH);

    DWORD dwError = ERROR_SUCCESS;
    HDWP hDwp = BeginDeferWindowPos(2);

    if (hDwp)
    {
        hDwp = ::DeferWindowPos(hDwp, ScrnshotPrev->m_hWnd, NULL,
            0, 0, ScrnshotWidth, Height, 0);

        if (hDwp)
        {
            // hide the padding if scrnshot window width == 0
            int RicheditPosX = ScrnshotWidth ? (ScrnshotWidth + INFO_DISPLAY_PADDING) : 0;

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

VOID CAppInfoDisplay::OnLink(ENLINK *Link)
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

ATL::CWndClassInfo &CAppInfoDisplay::GetWndClassInfo()
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

HWND CAppInfoDisplay::Create(HWND hwndParent)
{
    RECT r = { 0,0,0,0 };

    return CWindowImpl::Create(hwndParent, r, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

BOOL CAppInfoDisplay::ShowAvailableAppInfo(CAvailableApplicationInfo *Info)
{
    ATL::CStringW ScrnshotLocation;
    if (Info->RetrieveScrnshot(0, ScrnshotLocation))
    {
        ScrnshotPrev->DisplayImage(ScrnshotLocation);
    }
    else
    {
        ScrnshotPrev->DisplayEmpty();
    }
    ResizeChildren();
    return RichEdit->ShowAvailableAppInfo(Info);
}

BOOL CAppInfoDisplay::ShowInstalledAppInfo(CInstalledApplicationInfo *Info)
{
    ScrnshotPrev->DisplayEmpty();
    ResizeChildren();
    return RichEdit->ShowInstalledAppInfo(Info);
}

VOID CAppInfoDisplay::SetWelcomeText()
{
    ScrnshotPrev->DisplayEmpty();
    ResizeChildren();
    RichEdit->SetWelcomeText();
}

VOID CAppInfoDisplay::OnCommand(WPARAM wParam, LPARAM lParam)
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

CAppInfoDisplay::~CAppInfoDisplay()
{
    delete RichEdit;
    delete ScrnshotPrev;
}
// **** CAppInfoDisplay ****


// **** CAppsListView ****

CAppsListView::CAppsListView() :
    bHasCheckboxes(FALSE),
    nLastHeaderID(-1)
{
}

VOID CAppsListView::SetCheckboxesVisible(BOOL bIsVisible)
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

VOID CAppsListView::ColumnClick(LPNMLISTVIEW pnmv)
{
    HWND hHeader;
    HDITEMW hColumn;
    INT nHeaderID = pnmv->iSubItem;

    if ((GetWindowLongPtr(GWL_STYLE) & ~LVS_NOSORTHEADER) == 0)
        return;

    hHeader = (HWND)SendMessage(LVM_GETHEADER, 0, 0);
    ZeroMemory(&hColumn, sizeof(hColumn));

    /* If the sorting column changed, remove the sorting style from the old column */
    if ((nLastHeaderID != -1) && (nLastHeaderID != nHeaderID))
    {
        bIsAscending = TRUE; // also reset sorting method to ascending
        hColumn.mask = HDI_FORMAT;
        Header_GetItem(hHeader, nLastHeaderID, &hColumn);
        hColumn.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        Header_SetItem(hHeader, nLastHeaderID, &hColumn);
    }

    /* Set the sorting style to the new column */
    hColumn.mask = HDI_FORMAT;
    Header_GetItem(hHeader, nHeaderID, &hColumn);

    hColumn.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
    hColumn.fmt |= (bIsAscending ? HDF_SORTUP : HDF_SORTDOWN);
    Header_SetItem(hHeader, nHeaderID, &hColumn);

    /* Sort the list, using the current values of nHeaderID and bIsAscending */
    SortContext ctx = { this, nHeaderID };
    SortItems(s_CompareFunc, &ctx);

    /* Save new values */
    nLastHeaderID = nHeaderID;
    bIsAscending = !bIsAscending;
}

BOOL CAppsListView::AddColumn(INT Index, ATL::CStringW &Text, INT Width, INT Format)
{
    return AddColumn(Index, const_cast<LPWSTR>(Text.GetString()), Width, Format);
}

int CAppsListView::AddColumn(INT Index, LPWSTR lpText, INT Width, INT Format)
{
    LVCOLUMNW Column;

    ZeroMemory(&Column, sizeof(Column));

    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.iSubItem = Index;
    Column.pszText = lpText;
    Column.cx = Width;
    Column.fmt = Format;

    return SendMessage(LVM_INSERTCOLUMN, Index, (LPARAM)(&Column));
}

void CAppsListView::DeleteColumn(INT Index)
{
    SendMessage(LVM_DELETECOLUMN, Index, 0);
    return;
}

INT CAppsListView::AddItem(INT ItemIndex, INT IconIndex, LPCWSTR lpText, LPARAM lParam)
{
    LVITEMW Item;

    ZeroMemory(&Item, sizeof(Item));

    Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    Item.pszText = const_cast<LPWSTR>(lpText);
    Item.lParam = lParam;
    Item.iItem = ItemIndex;
    Item.iImage = IconIndex;

    if (IconIndex >= 0)
    {
        Item.iImage = IconIndex;
        Item.mask |= LVIF_IMAGE;
    }
    return InsertItem(&Item);
}

HIMAGELIST CAppsListView::GetImageList(int iImageList)
{
    return (HIMAGELIST)SendMessage(LVM_GETIMAGELIST, iImageList, 0);
}

INT CALLBACK CAppsListView::s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    SortContext *ctx = ((SortContext *)lParamSort);
    return ctx->lvw->CompareFunc(lParam1, lParam2, ctx->iSubItem);
}

INT CAppsListView::CompareFunc(LPARAM lParam1, LPARAM lParam2, INT iSubItem)
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

HWND CAppsListView::Create(HWND hwndParent)
{
    RECT r = { 205, 28, 465, 250 };
    DWORD style = WS_CHILD | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;

    HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE);

    if (hwnd)
    {
        SetCheckboxesVisible(FALSE);
    }

    HIMAGELIST hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
        LISTVIEW_ICON_SIZE,
        GetSystemColorDepth() | ILC_MASK,
        0, 1);
    SetImageList(hImageListView, LVSIL_SMALL);

    return hwnd;
}

BOOL CAppsListView::GetCheckState(INT item)
{
    return (BOOL)(GetItemState(item, LVIS_STATEIMAGEMASK) >> 12) - 1;
}

VOID CAppsListView::SetCheckState(INT item, BOOL fCheck)
{
    if (bHasCheckboxes)
    {
        SetItemState(item, INDEXTOSTATEIMAGEMASK((fCheck) ? 2 : 1), LVIS_STATEIMAGEMASK);
    }
}

VOID CAppsListView::CheckAll()
{
    if (bHasCheckboxes)
    {
        if (CheckedItemCount == ItemCount)
        {
            // clear all
            SetCheckState(-1, FALSE);
        }
        else
        {
            // check all
            SetCheckState(-1, TRUE);
        }
    }
}

PVOID CAppsListView::GetFocusedItemData()
{
    INT item = GetSelectionMark();
    if (item == -1)
    {
        return (PVOID)0;
    }
    return (PVOID)GetItemData(item);
}

BOOL CAppsListView::SetDisplayMode(APPLICATION_VIEW_MODE Mode)
{
    if (!DeleteAllItems()) return FALSE;
    ApplicationViewMode = Mode;

    bIsAscending = TRUE;

    ItemCount = 0;
    CheckedItemCount = 0;

    // delete old columns
    while (ColumnCount)
    {
        DeleteColumn(--ColumnCount);
    }

    ImageList_RemoveAll(GetImageList(LVSIL_SMALL));

    // add new columns
    ATL::CStringW szText;
    switch (Mode)
    {
    case ApplicationViewInstalledApps:

        /* Add columns to ListView */
        szText.LoadStringW(IDS_APP_NAME);
        AddColumn(ColumnCount++, szText, 250, LVCFMT_LEFT);

        szText.LoadStringW(IDS_APP_INST_VERSION);
        AddColumn(ColumnCount++, szText, 90, LVCFMT_RIGHT);

        szText.LoadStringW(IDS_APP_DESCRIPTION);
        AddColumn(ColumnCount++, szText, 300, LVCFMT_LEFT);

        // disable checkboxes
        SetCheckboxesVisible(FALSE);
        break;

    case ApplicationViewAvailableApps:

        /* Add columns to ListView */
        szText.LoadStringW(IDS_APP_NAME);
        AddColumn(ColumnCount++, szText, 250, LVCFMT_LEFT);

        szText.LoadStringW(IDS_APP_INST_VERSION);
        AddColumn(ColumnCount++, szText, 90, LVCFMT_RIGHT);

        szText.LoadStringW(IDS_APP_DESCRIPTION);
        AddColumn(ColumnCount++, szText, 300, LVCFMT_LEFT);

        // enable checkboxes
        SetCheckboxesVisible(TRUE);
        break;

    case ApplicationViewEmpty:
    default:
        break;
    }


    return TRUE;
}

BOOL CAppsListView::AddInstalledApplication(CInstalledApplicationInfo *InstAppInfo, LPVOID CallbackParam)
{
    if (ApplicationViewMode != ApplicationViewInstalledApps)
    {
        return FALSE;
    }

    HICON hIcon = (HICON)LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
    HIMAGELIST hImageList = GetImageList(LVSIL_SMALL);
    int IconIndex = ImageList_AddIcon(hImageList, hIcon);
    DestroyIcon(hIcon);

    int Index = AddItem(ItemCount, IconIndex, InstAppInfo->szDisplayName, (LPARAM)CallbackParam);
    SetItemText(Index, 1, InstAppInfo->szDisplayVersion.IsEmpty() ? L"---" : InstAppInfo->szDisplayVersion);
    SetItemText(Index, 2, InstAppInfo->szComments.IsEmpty() ? L"---" : InstAppInfo->szComments);

    ItemCount++;
    return TRUE;
}

BOOL CAppsListView::AddAvailableApplication(CAvailableApplicationInfo *AvlbAppInfo, BOOL InitCheckState, LPVOID CallbackParam)
{
    if (ApplicationViewMode != ApplicationViewAvailableApps)
    {
        return FALSE;
    }

    /* Load icon from file */
    HICON hIcon = NULL;
    ATL::CStringW szIconPath;
    if (AvlbAppInfo->RetrieveIcon(szIconPath))
    {
        hIcon = (HICON)LoadImageW(NULL,
            szIconPath.GetString(),
            IMAGE_ICON,
            LISTVIEW_ICON_SIZE,
            LISTVIEW_ICON_SIZE,
            LR_LOADFROMFILE);
    }

    if (!hIcon || GetLastError() != ERROR_SUCCESS)
    {
        /* Load default icon */
        hIcon = (HICON)LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
    }

    HIMAGELIST hImageList = GetImageList(LVSIL_SMALL);

    int IconIndex = ImageList_AddIcon(hImageList, hIcon);
    DestroyIcon(hIcon);

    int Index = AddItem(ItemCount, IconIndex, AvlbAppInfo->m_szName, (LPARAM)CallbackParam);

    if (InitCheckState)
    {
        SetCheckState(Index, TRUE);
    }

    SetItemText(Index, 1, AvlbAppInfo->m_szVersion);
    SetItemText(Index, 2, AvlbAppInfo->m_szDesc);

    ItemCount++;
    return TRUE;
}

// this function is called when parent window receiving an notification about checkstate changing
VOID CAppsListView::ItemCheckStateNotify(int iItem, BOOL bCheck)
{
    if (bCheck)
    {
        CheckedItemCount++;
    }
    else
    {
        CheckedItemCount--;
    }
}
// **** CAppsListView ****


// **** CApplicationView ****

BOOL CApplicationView::ProcessWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT &theResult, DWORD dwMapId)
{
    theResult = 0;
    switch (message)
    {
    case WM_CREATE:
    {
        BOOL bSuccess = TRUE;
        m_Panel = new CUiPanel();
        m_Panel->m_VerticalAlignment = UiAlign_Stretch;
        m_Panel->m_HorizontalAlignment = UiAlign_Stretch;

        bSuccess &= CreateHSplitter();
        bSuccess &= CreateListView();
        bSuccess &= CreateAppInfoDisplay();

        if (!bSuccess)
        {
            return -1; // creation failure
        }
    }
    break;

    case WM_NOTIFY:
    {
        LPNMHDR pNotifyHeader = (LPNMHDR)lParam;
        if (pNotifyHeader->hwndFrom == m_ListView->GetWindow())
        {
            switch (pNotifyHeader->code)
            {
            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW pnic = (LPNMLISTVIEW)lParam;

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
                    ItemGetFocus((LPVOID)pnic->lParam);
                }

                /* Check if the item is checked/unchecked */
                if (pnic->uChanged & LVIF_STATE)
                {
                    int iOldState = STATEIMAGETOINDEX(pnic->uOldState);
                    int iNewState = STATEIMAGETOINDEX(pnic->uNewState);

                    if (iOldState == STATEIMAGE_UNCHECKED && iNewState == STATEIMAGE_CHECKED)
                    {
                        // this item is just checked
                        m_ListView->ItemCheckStateNotify(pnic->iItem, TRUE);
                        ItemCheckStateChanged(TRUE, (LPVOID)pnic->lParam);
                    }
                    else if (iOldState == STATEIMAGE_CHECKED && iNewState == STATEIMAGE_UNCHECKED)
                    {
                        // this item is just unchecked
                        m_ListView->ItemCheckStateNotify(pnic->iItem, FALSE);
                        ItemCheckStateChanged(FALSE, (LPVOID)pnic->lParam);
                    }
                }
            }
            break;

            case LVN_COLUMNCLICK:
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                m_ListView->ColumnClick(pnmv);
            }
            break;

            case NM_DBLCLK:
            {
                LPNMITEMACTIVATE Item = (LPNMITEMACTIVATE)lParam;
                if (Item->iItem != -1)
                {
                    /* this won't do anything if the program is already installed */

                    if (ApplicationViewMode == ApplicationViewAvailableApps)
                    {
                        m_MainWindow->InstallApplication((CAvailableApplicationInfo *)m_ListView->GetItemData(Item->iItem));
                    }
                }
            }
            break;

            case NM_RCLICK:
            {
                if (((LPNMLISTVIEW)lParam)->iItem != -1)
                {
                    ShowPopupMenuEx(m_hWnd, m_hWnd, 0, ID_INSTALL);
                }
            }
            break;
            }
        }
    }
    break;

    case WM_SYSCOLORCHANGE:
    {
        /* Forward WM_SYSCOLORCHANGE to common controls */
        m_ListView->SendMessageW(WM_SYSCOLORCHANGE, wParam, lParam);
        m_ListView->SendMessageW(EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    }
    break;

    case WM_SIZE:
    {
        OnSize(hwnd, wParam, lParam);
        break;
    }

    case WM_COMMAND:
    {
        OnCommand(wParam, lParam);
    }
    break;
    }
    return FALSE;
}

BOOL CApplicationView::CreateHSplitter()
{
    m_HSplitter = new CUiSplitPanel();
    m_HSplitter->m_VerticalAlignment = UiAlign_Stretch;
    m_HSplitter->m_HorizontalAlignment = UiAlign_Stretch;
    m_HSplitter->m_DynamicFirst = TRUE;
    m_HSplitter->m_Horizontal = TRUE;
    m_HSplitter->m_Pos = INT_MAX; //set INT_MAX to use lowest possible position (m_MinSecond)
    m_HSplitter->m_MinFirst = 10;
    m_HSplitter->m_MinSecond = 140;
    m_Panel->Children().Append(m_HSplitter);

    return m_HSplitter->Create(m_hWnd) != NULL;
}

BOOL CApplicationView::CreateListView()
{
    m_ListView = new CAppsListView();
    m_ListView->m_VerticalAlignment = UiAlign_Stretch;
    m_ListView->m_HorizontalAlignment = UiAlign_Stretch;
    m_HSplitter->First().Append(m_ListView);

    return m_ListView->Create(m_hWnd) != NULL;
}

BOOL CApplicationView::CreateAppInfoDisplay()
{
    m_AppsInfo = new CAppInfoDisplay();
    m_AppsInfo->m_VerticalAlignment = UiAlign_Stretch;
    m_AppsInfo->m_HorizontalAlignment = UiAlign_Stretch;
    m_HSplitter->Second().Append(m_AppsInfo);

    return m_AppsInfo->Create(m_hWnd) != NULL;
}

VOID CApplicationView::OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_MINIMIZED)
        return;

    RECT r = { 0, 0, LOWORD(lParam), HIWORD(lParam) };
    HDWP hdwp = NULL;
    INT count = m_Panel->CountSizableChildren();

    hdwp = BeginDeferWindowPos(count);
    if (hdwp)
    {
        hdwp = m_Panel->OnParentSize(r, hdwp);
        if (hdwp)
        {
            EndDeferWindowPos(hdwp);
        }
    }
}

VOID CApplicationView::OnCommand(WPARAM wParam, LPARAM lParam)
{
    WORD wCommand = LOWORD(wParam);

    switch (wCommand)
    {
    case ID_INSTALL:
        m_MainWindow->InstallApplication((CAvailableApplicationInfo *)GetFocusedItemData());
        break;

    case ID_UNINSTALL:
        m_MainWindow->SendMessageW(WM_COMMAND, ID_UNINSTALL, 0);
        break;

    case ID_MODIFY:
        m_MainWindow->SendMessageW(WM_COMMAND, ID_MODIFY, 0);
        break;

    case ID_REGREMOVE:
        m_MainWindow->SendMessageW(WM_COMMAND, ID_REGREMOVE, 0);
        break;

    case ID_REFRESH:
        m_MainWindow->SendMessageW(WM_COMMAND, ID_REFRESH, 0);
        break;

    case ID_RESETDB:
        m_MainWindow->SendMessageW(WM_COMMAND, ID_RESETDB, 0);
        break;
    }
}

CApplicationView::CApplicationView(CMainWindow *MainWindow)
    : m_MainWindow(MainWindow)
{
}

CApplicationView::~CApplicationView()
{
    delete m_ListView;
    delete m_AppsInfo;
    delete m_HSplitter;
}

ATL::CWndClassInfo &CApplicationView::GetWndClassInfo()
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
            L"RAppsApplicationView",
            NULL
        },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

HWND CApplicationView::Create(HWND hwndParent)
{
    RECT r = { 0,0,0,0 };

    HMENU menu = GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATIONMENU)), 0);

    return CWindowImpl::Create(hwndParent, r, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, menu);
}

BOOL CApplicationView::SetDisplayMode(APPLICATION_VIEW_MODE Mode)
{
    if (!m_ListView->SetDisplayMode(Mode))
    {
        return FALSE;
    }
    ApplicationViewMode = Mode;
    m_AppsInfo->SetWelcomeText();

    HMENU hMenu = ::GetMenu(m_hWnd);
    switch (Mode)
    {
    case ApplicationViewEmpty:
    default:
        EnableMenuItem(hMenu, ID_REGREMOVE, MF_GRAYED);
        EnableMenuItem(hMenu, ID_INSTALL, MF_GRAYED);
        EnableMenuItem(hMenu, ID_UNINSTALL, MF_GRAYED);
        EnableMenuItem(hMenu, ID_MODIFY, MF_GRAYED);
        break;

    case ApplicationViewInstalledApps:
        EnableMenuItem(hMenu, ID_REGREMOVE, MF_ENABLED);
        EnableMenuItem(hMenu, ID_INSTALL, MF_GRAYED);
        EnableMenuItem(hMenu, ID_UNINSTALL, MF_ENABLED);
        EnableMenuItem(hMenu, ID_MODIFY, MF_ENABLED);
        break;

    case ApplicationViewAvailableApps:
        EnableMenuItem(hMenu, ID_REGREMOVE, MF_GRAYED);
        EnableMenuItem(hMenu, ID_INSTALL, MF_ENABLED);
        EnableMenuItem(hMenu, ID_UNINSTALL, MF_GRAYED);
        EnableMenuItem(hMenu, ID_MODIFY, MF_GRAYED);
        break;
    }
    return TRUE;
}

BOOL CApplicationView::AddInstalledApplication(CInstalledApplicationInfo *InstAppInfo, LPVOID param)
{
    if (ApplicationViewMode != ApplicationViewInstalledApps)
    {
        return FALSE;
    }
    return m_ListView->AddInstalledApplication(InstAppInfo, param);
}

BOOL CApplicationView::AddAvailableApplication(CAvailableApplicationInfo *AvlbAppInfo, BOOL InitCheckState, LPVOID param)
{
    if (ApplicationViewMode != ApplicationViewAvailableApps)
    {
        return FALSE;
    }
    return m_ListView->AddAvailableApplication(AvlbAppInfo, InitCheckState, param);
}

void CApplicationView::CheckAll()
{
    m_ListView->CheckAll();
    return;
}

PVOID CApplicationView::GetFocusedItemData()
{
    return m_ListView->GetFocusedItemData();
}

int CApplicationView::GetItemCount()
{
    return m_ListView->GetItemCount();
}

VOID CApplicationView::AppendTabOrderWindow(int Direction, ATL::CSimpleArray<HWND> &TabOrderList)
{
    m_ListView->AppendTabOrderWindow(Direction, TabOrderList);
    m_AppsInfo->AppendTabOrderWindow(Direction, TabOrderList);

    return;
}

// this function is called when a item of listview get focus.
// CallbackParam is the param passed to listview when adding the item (the one getting focus now).
BOOL CApplicationView::ItemGetFocus(LPVOID CallbackParam)
{
    switch (ApplicationViewMode)
    {
    case ApplicationViewInstalledApps:
        return m_AppsInfo->ShowInstalledAppInfo((CInstalledApplicationInfo *)CallbackParam);

    case ApplicationViewAvailableApps:
        return m_AppsInfo->ShowAvailableAppInfo((CAvailableApplicationInfo *)CallbackParam);

    case ApplicationViewEmpty:
    default:
        m_AppsInfo->SetWelcomeText();
        return FALSE;
    }
}

// this function is called when a item of listview is checked/unchecked
// CallbackParam is the param passed to listview when adding the item (the one getting focus now).
BOOL CApplicationView::ItemCheckStateChanged(BOOL bChecked, LPVOID CallbackParam)
{
    m_MainWindow->ItemCheckStateChanged(bChecked, CallbackParam);
    return TRUE;
}
// **** CApplicationView ****

