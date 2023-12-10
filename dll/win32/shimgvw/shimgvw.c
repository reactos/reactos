/*
 * PROJECT:     ReactOS Picture and Fax Viewer
 * LICENSE:     GPL-2.0 (https://spdx.org/licenses/GPL-2.0)
 * PURPOSE:     Image file browsing and manipulation
 * COPYRIGHT:   Copyright Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2018-2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shimgvw.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>

HINSTANCE g_hInstance;
SHIMGVW_SETTINGS g_Settings;
SHIMGVW_FILENODE *g_pCurrentFile;
GpImage *g_pImage = NULL;
WNDPROC g_fnPrevProc = NULL;

HWND g_hDispWnd = NULL;
HWND g_hToolBar = NULL;

ANIME g_Anime; /* Animation */

/* zooming */
static UINT s_nZoomPercents = 100;

static const UINT s_ZoomSteps[] =
{
    10, 25, 50, 100, 200, 400, 800, 1600
};

#define MIN_ZOOM s_ZoomSteps[0]
#define MAX_ZOOM s_ZoomSteps[_countof(s_ZoomSteps) - 1]

/* ToolBar Buttons */
typedef struct {
    DWORD idb;  /* Index to bitmap */
    DWORD ids;  /* Index to tooltip */
} TB_BUTTON_CONFIG;

    /* iBitmap,       idCommand,   fsState,         fsStyle,     bReserved[2], dwData, iString */
#define DEFINE_BTN_INFO(_name) \
    { TBICON_##_name, IDC_##_name, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 }

#define DEFINE_BTN_SEPARATOR \
    { -1,             0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0 }

/* ToolBar Buttons */
static const TBBUTTON s_Buttons[] =
{
    DEFINE_BTN_INFO(PREV_PIC),
    DEFINE_BTN_INFO(NEXT_PIC),
    DEFINE_BTN_SEPARATOR,
    DEFINE_BTN_INFO(BEST_FIT),
    DEFINE_BTN_INFO(REAL_SIZE),
    DEFINE_BTN_INFO(SLIDE_SHOW),
    DEFINE_BTN_SEPARATOR,
    DEFINE_BTN_INFO(ZOOM_IN),
    DEFINE_BTN_INFO(ZOOM_OUT),
    DEFINE_BTN_SEPARATOR,
    DEFINE_BTN_INFO(ROT_CLOCKW),
    DEFINE_BTN_INFO(ROT_COUNCW),
    DEFINE_BTN_SEPARATOR,
    DEFINE_BTN_INFO(DELETE),
    DEFINE_BTN_INFO(PRINT),
    DEFINE_BTN_INFO(SAVEAS),
    DEFINE_BTN_INFO(MODIFY),
    DEFINE_BTN_SEPARATOR,
    DEFINE_BTN_INFO(HELP_TOC)
};

#define DEFINE_BTN_CONFIG(_name) { IDB_##_name, IDS_TOOLTIP_##_name }

static const TB_BUTTON_CONFIG s_ButtonConfig[] =
{
    DEFINE_BTN_CONFIG(PREV_PIC),
    DEFINE_BTN_CONFIG(NEXT_PIC),
    DEFINE_BTN_CONFIG(BEST_FIT),
    DEFINE_BTN_CONFIG(REAL_SIZE),
    DEFINE_BTN_CONFIG(SLIDE_SHOW),
    DEFINE_BTN_CONFIG(ZOOM_IN),
    DEFINE_BTN_CONFIG(ZOOM_OUT),
    DEFINE_BTN_CONFIG(ROT_CLOCKW),
    DEFINE_BTN_CONFIG(ROT_COUNCW),
    DEFINE_BTN_CONFIG(DELETE),
    DEFINE_BTN_CONFIG(PRINT),
    DEFINE_BTN_CONFIG(SAVEAS),
    DEFINE_BTN_CONFIG(MODIFY),
    DEFINE_BTN_CONFIG(HELP_TOC)
};

static VOID UpdateZoom(UINT NewZoom, BOOL bEnableBestFit, BOOL bEnableRealSize)
{
    BOOL bEnableZoomIn, bEnableZoomOut;

    /* If zoom has not been changed, ignore it */
    if (s_nZoomPercents == NewZoom)
        return;

    s_nZoomPercents = NewZoom;

    /* Check if a zoom button of the toolbar must be grayed */
    bEnableZoomIn = (NewZoom < MAX_ZOOM);
    bEnableZoomOut = (NewZoom > MIN_ZOOM);

    /* Update toolbar buttons */
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_ZOOM_OUT, bEnableZoomOut);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_ZOOM_IN,  bEnableZoomIn);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_BEST_FIT, bEnableBestFit);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_REAL_SIZE, bEnableRealSize);

    /* Redraw the display window */
    InvalidateRect(g_hDispWnd, NULL, FALSE);
}

static VOID ZoomInOrOut(BOOL bZoomIn)
{
    UINT i, NewZoom;

    if (g_pImage == NULL)
        return;

    if (bZoomIn)    /* zoom in */
    {
        /* find next step */
        for (i = 0; i < _countof(s_ZoomSteps); ++i)
        {
            if (s_nZoomPercents < s_ZoomSteps[i])
                break;
        }
        NewZoom = ((i >= _countof(s_ZoomSteps)) ? MAX_ZOOM : s_ZoomSteps[i]);
    }
    else            /* zoom out */
    {
        /* find previous step */
        for (i = _countof(s_ZoomSteps); i > 0; )
        {
            --i;
            if (s_ZoomSteps[i] < s_nZoomPercents)
                break;
        }
        NewZoom = ((i < 0) ? MIN_ZOOM : s_ZoomSteps[i]);
    }

    /* Update toolbar and refresh screen */
    UpdateZoom(NewZoom, TRUE, TRUE);
}

static VOID ResetZoom(VOID)
{
    RECT Rect;
    UINT ImageWidth, ImageHeight, NewZoom;

    if (g_pImage == NULL)
        return;

    /* get disp window size and image size */
    GetClientRect(g_hDispWnd, &Rect);
    GdipGetImageWidth(g_pImage, &ImageWidth);
    GdipGetImageHeight(g_pImage, &ImageHeight);

    /* compare two aspect rates. same as
       (ImageHeight / ImageWidth < Rect.bottom / Rect.right) in real */
    if (ImageHeight * Rect.right < Rect.bottom * ImageWidth)
    {
        if (Rect.right < ImageWidth)
        {
            /* it's large, shrink it */
            NewZoom = (Rect.right * 100) / ImageWidth;
        }
        else
        {
            /* it's small. show as original size */
            NewZoom = 100;
        }
    }
    else
    {
        if (Rect.bottom < ImageHeight)
        {
            /* it's large, shrink it */
            NewZoom = (Rect.bottom * 100) / ImageHeight;
        }
        else
        {
            /* it's small. show as original size */
            NewZoom = 100;
        }
    }

    UpdateZoom(NewZoom, FALSE, TRUE);
}

static VOID pLoadImage(LPCWSTR szOpenFileName)
{
    Anime_FreeInfo(&g_Anime);

    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    /* check file presence */
    if (!szOpenFileName || GetFileAttributesW(szOpenFileName) == 0xFFFFFFFF)
    {
        DPRINT1("File %s not found!\n", szOpenFileName);
        return;
    }

    /* load now */
    GdipLoadImageFromFile(szOpenFileName, &g_pImage);
    if (!g_pImage)
    {
        DPRINT1("GdipLoadImageFromFile() failed\n");
        return;
    }

    Anime_LoadInfo(&g_Anime);

    if (szOpenFileName && szOpenFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szOpenFileName);

    /* Reset zoom and redraw display */
    ResetZoom();
}

static VOID pSaveImageAs(HWND hwnd)
{
    OPENFILENAMEW sfn;
    ImageCodecInfo *codecInfo;
    WCHAR szSaveFileName[MAX_PATH];
    WCHAR *szFilterMask;
    GUID rawFormat;
    UINT num, size, j;
    size_t sizeRemain;
    WCHAR *c;

    if (g_pImage == NULL)
        return;

    GdipGetImageEncodersSize(&num, &size);
    codecInfo = malloc(size);
    if (!codecInfo)
    {
        DPRINT1("malloc() failed in pSaveImageAs()\n");
        return;
    }

    GdipGetImageEncoders(num, size, codecInfo);
    GdipGetImageRawFormat(g_pImage, &rawFormat);

    sizeRemain = 0;
    for (j = 0; j < num; ++j)
    {
        // Every pair needs space for the Description, twice the Extensions, 1 char for the space, 2 for the braces and 2 for the NULL terminators.
        sizeRemain = sizeRemain + (((wcslen(codecInfo[j].FormatDescription) + (wcslen(codecInfo[j].FilenameExtension) * 2) + 5) * sizeof(WCHAR)));
    }

    /* Add two more chars for the last terminator */
    sizeRemain += (sizeof(WCHAR) * 2);

    szFilterMask = malloc(sizeRemain);
    if (!szFilterMask)
    {
        DPRINT1("cannot allocate memory for filter mask in pSaveImageAs()");
        free(codecInfo);
        return;
    }

    ZeroMemory(szSaveFileName, sizeof(szSaveFileName));
    ZeroMemory(szFilterMask, sizeRemain);
    ZeroMemory(&sfn, sizeof(sfn));
    sfn.lStructSize = sizeof(sfn);
    sfn.hwndOwner   = hwnd;
    sfn.lpstrFile   = szSaveFileName;
    sfn.lpstrFilter = szFilterMask;
    sfn.nMaxFile    = _countof(szSaveFileName);
    sfn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    sfn.lpstrDefExt = L"png";

    c = szFilterMask;

    for (j = 0; j < num; ++j)
    {
        StringCbPrintfExW(c, sizeRemain, &c, &sizeRemain, 0, L"%ls (%ls)", codecInfo[j].FormatDescription, codecInfo[j].FilenameExtension);

        /* Skip the NULL character */
        c++;
        sizeRemain -= sizeof(*c);

        StringCbPrintfExW(c, sizeRemain, &c, &sizeRemain, 0, L"%ls", codecInfo[j].FilenameExtension);

        /* Skip the NULL character */
        c++;
        sizeRemain -= sizeof(*c);

        if (IsEqualGUID(&rawFormat, &codecInfo[j].FormatID) != FALSE)
        {
            sfn.nFilterIndex = j + 1;
        }
    }

    if (GetSaveFileNameW(&sfn))
    {
        Anime_Pause(&g_Anime);

        if (GdipSaveImageToFile(g_pImage, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
        {
            DPRINT1("GdipSaveImageToFile() failed\n");
        }

        Anime_Start(&g_Anime, 0);
    }

    free(szFilterMask);
    free(codecInfo);
}

static VOID
pPrintImage(HWND hwnd)
{
    /* FIXME */
}

static VOID
Preview_UpdateUI(HWND hwnd)
{
    BOOL bEnable = (g_pImage != NULL);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_SAVEAS, bEnable);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_PRINT, bEnable);
    InvalidateRect(g_hDispWnd, NULL, FALSE);
}

static BOOL
pLoadImageFromNode(SHIMGVW_FILENODE *node, HWND hwnd)
{
    WCHAR szText[MAX_PATH + 100];
    LPWSTR pchFileTitle;

    pLoadImage(node ? node->FileName : NULL);

    if (node == NULL)
        return FALSE;

    LoadStringW(g_hInstance, IDS_APPTITLE, szText, _countof(szText));

    pchFileTitle = PathFindFileNameW(node->FileName);
    if (pchFileTitle && *pchFileTitle)
    {
        StringCchCatW(szText, _countof(szText), L" - ");
        StringCchCatW(szText, _countof(szText), pchFileTitle);
    }
    SetWindowTextW(hwnd, szText);

    return g_pImage != NULL;
}

static SHIMGVW_FILENODE*
pBuildFileList(LPCWSTR szFirstFile)
{
    HANDLE hFindHandle;
    WCHAR *extension;
    WCHAR szSearchPath[MAX_PATH];
    WCHAR szSearchMask[MAX_PATH];
    WCHAR szFileTypes[MAX_PATH];
    WIN32_FIND_DATAW findData;
    SHIMGVW_FILENODE *currentNode = NULL;
    SHIMGVW_FILENODE *root = NULL;
    SHIMGVW_FILENODE *conductor = NULL;
    ImageCodecInfo *codecInfo;
    UINT num;
    UINT size;
    UINT j;

    StringCbCopyW(szSearchPath, sizeof(szSearchPath), szFirstFile);
    PathRemoveFileSpecW(szSearchPath);

    GdipGetImageDecodersSize(&num, &size);
    codecInfo = malloc(size);
    if (!codecInfo)
    {
        DPRINT1("malloc() failed in pLoadFileList()\n");
        return NULL;
    }

    GdipGetImageDecoders(num, size, codecInfo);

    root = malloc(sizeof(SHIMGVW_FILENODE));
    if (!root)
    {
        DPRINT1("malloc() failed in pLoadFileList()\n");
        free(codecInfo);
        return NULL;
    }

    conductor = root;

    for (j = 0; j < num; ++j)
    {
        StringCbCopyW(szFileTypes, sizeof(szFileTypes), codecInfo[j].FilenameExtension);

        extension = wcstok(szFileTypes, L";");
        while (extension != NULL)
        {
            PathCombineW(szSearchMask, szSearchPath, extension);

            hFindHandle = FindFirstFileW(szSearchMask, &findData);
            if (hFindHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    PathCombineW(conductor->FileName, szSearchPath, findData.cFileName);

                    // compare the name of the requested file with the one currently found.
                    // if the name matches, the current node is returned by the function.
                    if (_wcsicmp(szFirstFile, conductor->FileName) == 0)
                    {
                        currentNode = conductor;
                    }

                    conductor->Next = malloc(sizeof(SHIMGVW_FILENODE));

                    // if malloc fails, make circular what we have and return it
                    if (!conductor->Next)
                    {
                        DPRINT1("malloc() failed in pLoadFileList()\n");

                        conductor->Next = root;
                        root->Prev = conductor;

                        FindClose(hFindHandle);
                        free(codecInfo);
                        return conductor;
                    }

                    conductor->Next->Prev = conductor;
                    conductor = conductor->Next;
                }
                while (FindNextFileW(hFindHandle, &findData) != 0);

                FindClose(hFindHandle);
            }

            extension = wcstok(NULL, L";");
        }
    }

    // we now have a node too much in the list. In case the requested file was not found,
    // we use this node to store the name of it, otherwise we free it.
    if (currentNode == NULL)
    {
        StringCchCopyW(conductor->FileName, MAX_PATH, szFirstFile);
        currentNode = conductor;
    }
    else
    {
        conductor = conductor->Prev;
        free(conductor->Next);
    }

    // link the last node with the first one to make the list circular
    conductor->Next = root;
    root->Prev = conductor;
    conductor = currentNode;

    free(codecInfo);

    return conductor;
}

static VOID
pFreeFileList(SHIMGVW_FILENODE *root)
{
    SHIMGVW_FILENODE *conductor;

    if (!root)
        return;

    root->Prev->Next = NULL;
    root->Prev = NULL;

    while (root)
    {
        conductor = root;
        root = conductor->Next;
        free(conductor);
    }
}

static HBRUSH CreateCheckerBoardBrush(HDC hdc)
{
    static const CHAR pattern[] =
        "\x28\x00\x00\x00\x10\x00\x00\x00\x10\x00\x00\x00\x01\x00\x04\x00\x00\x00"
        "\x00\x00\x80\x00\x00\x00\x23\x2E\x00\x00\x23\x2E\x00\x00\x10\x00\x00\x00"
        "\x00\x00\x00\x00\x99\x99\x99\x00\xCC\xCC\xCC\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11\x11\x11\x11"
        "\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00"
        "\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00"
        "\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11"
        "\x11\x11\x00\x00\x00\x00\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00"
        "\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11"
        "\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11"
        "\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x00\x11\x11\x11\x11";

    return CreateDIBPatternBrushPt(pattern, DIB_RGB_COLORS);
}

static VOID
ZoomWnd_OnPaint(HWND hwnd)
{
    GpGraphics *graphics;
    UINT ImageWidth, ImageHeight;
    INT ZoomedWidth, ZoomedHeight, x, y;
    PAINTSTRUCT ps;
    RECT rect, margin;
    HDC hdc;
    HBRUSH white;
    HGDIOBJ hbrOld;
    UINT uFlags;
    WCHAR szText[128];
    HGDIOBJ hFontOld;

    hdc = BeginPaint(hwnd, &ps);
    if (!hdc)
    {
        DPRINT1("BeginPaint() failed\n");
        return;
    }

    GdipCreateFromHDC(hdc, &graphics);
    if (!graphics)
    {
        DPRINT1("GdipCreateFromHDC() failed\n");
        return;
    }

    GetClientRect(hwnd, &rect);
    white = GetStockObject(WHITE_BRUSH);

    if (g_pImage == NULL)
    {
        FillRect(hdc, &rect, white);

        LoadStringW(g_hInstance, IDS_NOPREVIEW, szText, _countof(szText));

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        hFontOld = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        DrawTextW(hdc, szText, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER |
                                          DT_NOPREFIX);
        SelectObject(hdc, hFontOld);
    }
    else
    {
        GdipGetImageWidth(g_pImage, &ImageWidth);
        GdipGetImageHeight(g_pImage, &ImageHeight);

        ZoomedWidth = (ImageWidth * s_nZoomPercents) / 100;
        ZoomedHeight = (ImageHeight * s_nZoomPercents) / 100;

        x = (rect.right - ZoomedWidth) / 2;
        y = (rect.bottom - ZoomedHeight) / 2;

        // Fill top part
        margin = rect;
        margin.bottom = y - 1;
        FillRect(hdc, &margin, white);
        // Fill bottom part
        margin.top = y + ZoomedHeight + 1;
        margin.bottom = rect.bottom;
        FillRect(hdc, &margin, white);
        // Fill left part
        margin.top = y - 1;
        margin.bottom = y + ZoomedHeight + 1;
        margin.right = x - 1;
        FillRect(hdc, &margin, white);
        // Fill right part
        margin.left = x + ZoomedWidth + 1;
        margin.right = rect.right;
        FillRect(hdc, &margin, white);

        DPRINT("x = %d, y = %d, ImageWidth = %u, ImageHeight = %u\n");
        DPRINT("rect.right = %ld, rect.bottom = %ld\n", rect.right, rect.bottom);
        DPRINT("s_nZoomPercents = %d, ZoomedWidth = %d, ZoomedHeight = %d\n",
               s_nZoomPercents, ZoomedWidth, ZoomedWidth);

        if (s_nZoomPercents % 100 == 0)
        {
            GdipSetInterpolationMode(graphics, InterpolationModeNearestNeighbor);
            GdipSetSmoothingMode(graphics, SmoothingModeNone);
        }
        else
        {
            GdipSetInterpolationMode(graphics, InterpolationModeHighQualityBilinear);
            GdipSetSmoothingMode(graphics, SmoothingModeHighQuality);
        }

        uFlags = 0;
        GdipGetImageFlags(g_pImage, &uFlags);

        if (uFlags & (ImageFlagsHasAlpha | ImageFlagsHasTranslucent))
        {
            HBRUSH hbr = CreateCheckerBoardBrush(hdc);
            hbrOld = SelectObject(hdc, hbr);
            Rectangle(hdc, x - 1, y - 1, x + ZoomedWidth + 1, y + ZoomedHeight + 1);
            SelectObject(hdc, hbrOld);
            DeleteObject(hbr);
        }
        else
        {
            hbrOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, x - 1, y - 1, x + ZoomedWidth + 1, y + ZoomedHeight + 1);
            SelectObject(hdc, hbrOld);
        }

        GdipDrawImageRectI(graphics, g_pImage, x, y, ZoomedWidth, ZoomedHeight);
    }
    GdipDeleteGraphics(graphics);
    EndPaint(hwnd, &ps);
}

static VOID
ImageView_ResetSettings(VOID)
{
    g_Settings.Maximized = FALSE;
    g_Settings.X         = CW_USEDEFAULT;
    g_Settings.Y         = CW_USEDEFAULT;
    g_Settings.Width     = 520;
    g_Settings.Height    = 400;
}

static BOOL
ImageView_LoadSettings(VOID)
{
    HKEY hKey;
    DWORD dwSize;
    LSTATUS nError;

    nError = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\shimgvw", 0, KEY_READ, &hKey);
    if (nError != ERROR_SUCCESS)
        return FALSE;

    dwSize = sizeof(g_Settings);
    nError = RegQueryValueExW(hKey, L"Settings", NULL, NULL, (LPBYTE)&g_Settings, &dwSize);
    RegCloseKey(hKey);

    return ((nError == ERROR_SUCCESS) && (dwSize == sizeof(g_Settings)));
}

static VOID
ImageView_SaveSettings(VOID)
{
    HKEY hKey;
    LSTATUS nError;

    nError = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\shimgvw",
                             0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (nError != ERROR_SUCCESS)
        return;

    RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, (LPBYTE)&g_Settings, sizeof(g_Settings));
    RegCloseKey(hKey);
}

static BOOL
Preview_CreateToolBar(HWND hwnd)
{
    g_hToolBar = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                                 0, 0, 0, 0, hwnd, NULL, g_hInstance, NULL);
    if (g_hToolBar != NULL)
    {
        HIMAGELIST hImageList;

        SendMessageW(g_hToolBar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessageW(g_hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(s_Buttons[0]), 0);

        hImageList = ImageList_Create(TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, ILC_MASK | ILC_COLOR24, 1, 1);
        if (hImageList == NULL)
            return FALSE;

        for (UINT n = 0; n < _countof(s_ButtonConfig); n++)
        {
            ImageList_AddMasked(hImageList, LoadImageW(g_hInstance, MAKEINTRESOURCEW(s_ButtonConfig[n].idb), IMAGE_BITMAP,
                                TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));
        }

        ImageList_Destroy((HIMAGELIST)SendMessageW(g_hToolBar, TB_SETIMAGELIST,
                                                   0, (LPARAM)hImageList));

        SendMessageW(g_hToolBar, TB_ADDBUTTONS, _countof(s_Buttons), (LPARAM)s_Buttons);

        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK
ZoomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
        {
            ZoomWnd_OnPaint(hwnd);
            break;
        }
        case WM_TIMER:
        {
            if (Anime_OnTimer(&g_Anime, wParam))
                InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        default:
            return CallWindowProcW(g_fnPrevProc, hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL
Preview_OnCreate(HWND hwnd, LPCREATESTRUCT pCS)
{
    g_hDispWnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_STATIC, L"",
                                 WS_CHILD | WS_VISIBLE,
                                 0, 0, 0, 0, hwnd, NULL, g_hInstance, NULL);

    SetClassLongPtr(g_hDispWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
    g_fnPrevProc = (WNDPROC)SetWindowLongPtr(g_hDispWnd, GWLP_WNDPROC, (LPARAM)ZoomWndProc);

    Preview_CreateToolBar(hwnd);
    Anime_SetTimerWnd(&g_Anime, g_hDispWnd);

    if (pCS && pCS->lpCreateParams)
    {
        LPCWSTR pszFileName = (LPCWSTR)pCS->lpCreateParams;
        WCHAR szFile[MAX_PATH];

        /* Make sure the path has no quotes on it */
        StringCchCopyW(szFile, _countof(szFile), pszFileName);
        PathUnquoteSpacesW(szFile);

        g_pCurrentFile = pBuildFileList(szFile);
        pLoadImageFromNode(g_pCurrentFile, hwnd);
        Preview_UpdateUI(hwnd);
    }

    return TRUE;
}

static VOID
Preview_OnMouseWheel(HWND hwnd, INT x, INT y, INT zDelta, UINT fwKeys)
{
    if (zDelta != 0)
    {
        if (GetKeyState(VK_CONTROL) < 0)
            ZoomInOrOut(zDelta > 0);
    }
}

static VOID
Preview_OnMoveSize(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    RECT *prc;

    if (IsIconic(hwnd))
        return;

    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    prc = &wp.rcNormalPosition;
    g_Settings.X = prc->left;
    g_Settings.Y = prc->top;
    g_Settings.Width = prc->right - prc->left;
    g_Settings.Height = prc->bottom - prc->top;
    g_Settings.Maximized = IsZoomed(hwnd);
}

static VOID
Preview_OnSize(HWND hwnd, UINT state, INT cx, INT cy)
{
    RECT rc;

    SendMessageW(g_hToolBar, TB_AUTOSIZE, 0, 0);

    GetWindowRect(g_hToolBar, &rc);

    MoveWindow(g_hDispWnd, 0, 0, cx, cy - (rc.bottom - rc.top), TRUE);

    if (!IsIconic(hwnd)) /* Is it not minimized? */
        ResetZoom();

    Preview_OnMoveSize(hwnd);
}

static VOID
Preview_Delete(HWND hwnd)
{
    WCHAR szCurFile[MAX_PATH + 1], szNextFile[MAX_PATH];
    SHFILEOPSTRUCTW FileOp = { hwnd, FO_DELETE };

    if (!g_pCurrentFile)
        return;

    /* FileOp.pFrom must be double-null-terminated */
    GetFullPathNameW(g_pCurrentFile->FileName, _countof(szCurFile) - 1, szCurFile, NULL);
    szCurFile[_countof(szCurFile) - 2] = UNICODE_NULL; /* Avoid buffer overrun */
    szCurFile[lstrlenW(szCurFile) + 1] = UNICODE_NULL;

    GetFullPathNameW(g_pCurrentFile->Next->FileName, _countof(szNextFile), szNextFile, NULL);
    szNextFile[_countof(szNextFile) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

    /* FIXME: Our GdipLoadImageFromFile locks the image file */
    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    /* Confirm file deletion and delete if allowed */
    FileOp.pFrom = szCurFile;
    FileOp.fFlags = FOF_ALLOWUNDO;
    if (SHFileOperationW(&FileOp) != 0)
    {
        DPRINT("Preview_Delete: SHFileOperationW() failed or canceled\n");

        pLoadImage(szCurFile);
        return;
    }

    /* Reload the file list and go next file */
    pFreeFileList(g_pCurrentFile);
    g_pCurrentFile = pBuildFileList(szNextFile);
    pLoadImageFromNode(g_pCurrentFile, hwnd);
}

static VOID
Preview_Edit(HWND hwnd)
{
    WCHAR szPathName[MAX_PATH];
    SHELLEXECUTEINFOW sei;

    if (!g_pCurrentFile)
        return;

    GetFullPathNameW(g_pCurrentFile->FileName, _countof(szPathName), szPathName, NULL);
    szPathName[_countof(szPathName) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"edit";
    sei.lpFile = szPathName;
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei))
    {
        DPRINT1("Preview_Edit: ShellExecuteExW() failed with code %ld\n", GetLastError());
    }
}

static VOID
Preview_OnCommand(HWND hwnd, UINT nCommandID)
{
    switch (nCommandID)
    {
        case IDC_PREV_PIC:
            if (g_pCurrentFile)
            {
                g_pCurrentFile = g_pCurrentFile->Prev;
                pLoadImageFromNode(g_pCurrentFile, hwnd);
                Preview_UpdateUI(hwnd);
            }
            break;

        case IDC_NEXT_PIC:
            if (g_pCurrentFile)
            {
                g_pCurrentFile = g_pCurrentFile->Next;
                pLoadImageFromNode(g_pCurrentFile, hwnd);
                Preview_UpdateUI(hwnd);
            }
            break;

        case IDC_BEST_FIT:
            ResetZoom();
            break;

        case IDC_REAL_SIZE:
            UpdateZoom(100, TRUE, FALSE);
            break;

        case IDC_SLIDE_SHOW:
            DPRINT1("IDC_SLIDE_SHOW unimplemented\n");
            break;

        case IDC_ZOOM_IN:
            ZoomInOrOut(TRUE);
            break;

        case IDC_ZOOM_OUT:
            ZoomInOrOut(FALSE);
            break;

        case IDC_SAVEAS:
            pSaveImageAs(hwnd);
            break;

        case IDC_PRINT:
            pPrintImage(hwnd);
            break;

        case IDC_ROT_CLOCKW:
            if (g_pImage)
            {
                GdipImageRotateFlip(g_pImage, Rotate270FlipNone);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case IDC_ROT_COUNCW:
            if (g_pImage)
            {
                GdipImageRotateFlip(g_pImage, Rotate90FlipNone);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case IDC_DELETE:
            Preview_Delete(hwnd);
            Preview_UpdateUI(hwnd);
            break;

        case IDC_MODIFY:
            Preview_Edit(hwnd);
            Preview_UpdateUI(hwnd);
            break;
    }
}

static LRESULT
Preview_OnNotify(HWND hwnd, LPNMHDR pnmhdr)
{
    switch (pnmhdr->code)
    {
        case TTN_GETDISPINFOW:
        {
            LPTOOLTIPTEXTW lpttt = (LPTOOLTIPTEXTW)pnmhdr;
            lpttt->hinst = g_hInstance;
            lpttt->lpszText = MAKEINTRESOURCEW(s_ButtonConfig[lpttt->hdr.idFrom - IDC_TOOL_BASE].ids);
            break;
        }
    }
    return 0;
}

static VOID
Preview_OnDestroy(HWND hwnd)
{
    pFreeFileList(g_pCurrentFile);
    g_pCurrentFile = NULL;

    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    Anime_FreeInfo(&g_Anime);

    SetWindowLongPtr(g_hDispWnd, GWLP_WNDPROC, (LPARAM)g_fnPrevProc);
    PostQuitMessage(0);
}

LRESULT CALLBACK
PreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            if (!Preview_OnCreate(hwnd, (LPCREATESTRUCT)lParam))
                return -1;
            break;
        }
        case WM_COMMAND:
        {
            Preview_OnCommand(hwnd, LOWORD(wParam));
            break;
        }
        case WM_MOUSEWHEEL:
        {
            Preview_OnMouseWheel(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                                 (SHORT)HIWORD(wParam), (UINT)LOWORD(wParam));
            break;
        }
        case WM_NOTIFY:
        {
            return Preview_OnNotify(hwnd, (LPNMHDR)lParam);
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *pMMI = (MINMAXINFO*)lParam;
            pMMI->ptMinTrackSize.x = 350;
            pMMI->ptMinTrackSize.y = 290;
            break;
        }
        case WM_MOVE:
        {
            Preview_OnMoveSize(hwnd);
            break;
        }
        case WM_SIZE:
        {
            Preview_OnSize(hwnd, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        }
        case WM_DESTROY:
        {
            Preview_OnDestroy(hwnd);
            break;
        }
        default:
        {
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }

    return 0;
}

LONG
ImageView_Main(HWND hwnd, LPCWSTR szFileName)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    WNDCLASSW WndClass;
    WCHAR szTitle[256];
    HWND hMainWnd;
    MSG msg;
    HACCEL hAccel;
    HRESULT hrCoInit;
    INITCOMMONCONTROLSEX Icc = { .dwSize = sizeof(Icc), .dwICC = ICC_WIN95_CLASSES };

    InitCommonControlsEx(&Icc);

    /* Initialize COM */
    hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hrCoInit))
        DPRINT1("Warning, CoInitializeEx failed with code=%08X\n", (int)hrCoInit);

    if (!ImageView_LoadSettings())
        ImageView_ResetSettings();

    /* Initialize GDI+ */
    ZeroMemory(&gdiplusStartupInput, sizeof(gdiplusStartupInput));
    gdiplusStartupInput.GdiplusVersion = 1;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    /* Register window classes */
    ZeroMemory(&WndClass, sizeof(WndClass));
    WndClass.lpszClassName  = WC_PREVIEW;
    WndClass.lpfnWndProc    = PreviewWndProc;
    WndClass.hInstance      = g_hInstance;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    WndClass.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    WndClass.hbrBackground  = NULL;   /* less flicker */
    if (!RegisterClassW(&WndClass))
        return -1;

    /* Create the main window */
    LoadStringW(g_hInstance, IDS_APPTITLE, szTitle, _countof(szTitle));
    hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE, WC_PREVIEW, szTitle,
                               WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
                               g_Settings.X, g_Settings.Y, g_Settings.Width, g_Settings.Height,
                               NULL, NULL, g_hInstance, (LPVOID)szFileName);

    /* Create accelerator table for keystrokes */
    hAccel = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR));

    /* Show the main window now */
    if (g_Settings.Maximized)
        ShowWindow(hMainWnd, SW_SHOWMAXIMIZED);
    else
        ShowWindow(hMainWnd, SW_SHOWNORMAL);

    UpdateWindow(hMainWnd);

    /* Message Loop */
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (TranslateAcceleratorW(hMainWnd, hAccel, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    /* Destroy accelerator table */
    DestroyAcceleratorTable(hAccel);

    ImageView_SaveSettings();

    GdiplusShutdown(gdiplusToken);

    /* Release COM resources */
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return 0;
}

VOID WINAPI
ImageView_FullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_Main(hwnd, path);
}

VOID WINAPI
ImageView_Fullscreen(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_Main(hwnd, path);
}

VOID WINAPI
ImageView_FullscreenA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, path, -1, szFile, _countof(szFile)))
    {
        ImageView_Main(hwnd, szFile);
    }
}

VOID WINAPI
ImageView_PrintTo(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintTo() not implemented\n");
}

VOID WINAPI
ImageView_PrintToA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    DPRINT("ImageView_PrintToA() not implemented\n");
}

VOID WINAPI
ImageView_PrintToW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintToW() not implemented\n");
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hinstDLL;
            break;
    }

    return TRUE;
}
