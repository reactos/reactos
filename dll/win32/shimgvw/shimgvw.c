/*
 * PROJECT:         ReactOS Picture and Fax Viewer
 * FILE:            dll/win32/shimgvw/shimgvw.c
 * PURPOSE:         shimgvw.dll
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define INITGUID

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <windowsx.h>
#include <objbase.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <tchar.h>
#include <shlobj.h>
#include <strsafe.h>
#include <shlwapi.h>
#include <shellapi.h>

#define NDEBUG
#include <debug.h>

#include "shimgvw.h"

HINSTANCE g_hInstance;
SHIMGVW_SETTINGS g_Settings;
SHIMGVW_FILENODE *g_currentFile;
GpImage *g_pImage = NULL;
WNDPROC g_PrevProc = NULL;

HWND g_hDispWnd = NULL;
HWND g_hToolBar = NULL;

/* zooming */
static UINT s_ZoomPercents = 100;

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
    { 15,             0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0 }

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

static const TB_BUTTON_CONFIG s_BtnConfig[] =
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

/* animation */
UINT            m_nFrameIndex = 0;
UINT            m_nFrameCount = 0;
UINT            m_nLoopIndex = 0;
UINT            m_nLoopCount = (UINT)-1;
PropertyItem   *m_pDelayItem = NULL;

#define ANIME_TIMER_ID  9999

static void Anime_FreeInfo(void)
{
    if (m_pDelayItem)
    {
        free(m_pDelayItem);
        m_pDelayItem = NULL;
    }
    m_nFrameIndex = 0;
    m_nFrameCount = 0;
    m_nLoopIndex = 0;
    m_nLoopCount = (UINT)-1;
}

static BOOL Anime_LoadInfo(void)
{
    GUID *dims;
    UINT nDimCount = 0;
    UINT cbItem;
    UINT result;
    PropertyItem *pItem;

    Anime_FreeInfo();
    KillTimer(g_hDispWnd, ANIME_TIMER_ID);

    if (!g_pImage)
        return FALSE;

    GdipImageGetFrameDimensionsCount(g_pImage, &nDimCount);
    if (nDimCount)
    {
        dims = (GUID *)calloc(nDimCount, sizeof(GUID));
        if (dims)
        {
            GdipImageGetFrameDimensionsList(g_pImage, dims, nDimCount);
            GdipImageGetFrameCount(g_pImage, dims, &result);
            m_nFrameCount = result;
            free(dims);
        }
    }

    result = 0;
    GdipGetPropertyItemSize(g_pImage, PropertyTagFrameDelay, &result);
    cbItem = result;
    if (cbItem)
    {
        m_pDelayItem = (PropertyItem *)malloc(cbItem);
        GdipGetPropertyItem(g_pImage, PropertyTagFrameDelay, cbItem, m_pDelayItem);
    }

    result = 0;
    GdipGetPropertyItemSize(g_pImage, PropertyTagLoopCount, &result);
    cbItem = result;
    if (cbItem)
    {
        pItem = (PropertyItem *)malloc(cbItem);
        if (pItem)
        {
            if (GdipGetPropertyItem(g_pImage, PropertyTagLoopCount, cbItem, pItem) == Ok)
            {
                m_nLoopCount = *(WORD *)pItem->value;
            }
            free(pItem);
        }
    }

    if (m_pDelayItem)
    {
        SetTimer(g_hDispWnd, ANIME_TIMER_ID, 0, NULL);
    }

    return m_pDelayItem != NULL;
}

static void Anime_SetFrameIndex(UINT nFrameIndex)
{
    if (nFrameIndex < m_nFrameCount)
    {
        GUID guid = FrameDimensionTime;
        if (Ok != GdipImageSelectActiveFrame(g_pImage, &guid, nFrameIndex))
        {
            guid = FrameDimensionPage;
            GdipImageSelectActiveFrame(g_pImage, &guid, nFrameIndex);
        }
    }
    m_nFrameIndex = nFrameIndex;
}

DWORD Anime_GetFrameDelay(UINT nFrameIndex)
{
    if (nFrameIndex < m_nFrameCount && m_pDelayItem)
    {
        return ((DWORD *)m_pDelayItem->value)[m_nFrameIndex] * 10;
    }
    return 0;
}

BOOL Anime_Step(DWORD *pdwDelay)
{
    *pdwDelay = INFINITE;
    if (m_nLoopCount == (UINT)-1)
        return FALSE;

    if (m_nFrameIndex + 1 < m_nFrameCount)
    {
        *pdwDelay = Anime_GetFrameDelay(m_nFrameIndex);
        Anime_SetFrameIndex(m_nFrameIndex);
        ++m_nFrameIndex;
        return TRUE;
    }

    if (m_nLoopCount == 0 || m_nLoopIndex < m_nLoopCount)
    {
        *pdwDelay = Anime_GetFrameDelay(m_nFrameIndex);
        Anime_SetFrameIndex(m_nFrameIndex);
        m_nFrameIndex = 0;
        ++m_nLoopIndex;
        return TRUE;
    }

    return FALSE;
}

static void UpdateZoom(UINT NewZoom, BOOL bEnableBestFit, BOOL bEnableRealSize)
{
    BOOL bEnableZoomIn, bEnableZoomOut;

    /* If zoom has not been changed, ignore it */
    if (s_ZoomPercents == NewZoom)
        return;

    s_ZoomPercents = NewZoom;

    /* Check if a zoom button of the toolbar must be grayed */
    bEnableZoomIn = bEnableZoomOut = TRUE;

    if (NewZoom >= MAX_ZOOM)
    {
        bEnableZoomIn = FALSE;
    }
    else if (NewZoom <= MIN_ZOOM)
    {
        bEnableZoomOut = FALSE;
    }

    /* Update the state of the zoom buttons */
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_ZOOM_OUT, bEnableZoomOut);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_ZOOM_IN,  bEnableZoomIn);

    /* Redraw the display window */
    InvalidateRect(g_hDispWnd, NULL, FALSE);

    /* Update toolbar buttons */
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_BEST_FIT, bEnableBestFit);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_REAL_SIZE, bEnableRealSize);
}

static void ZoomInOrOut(BOOL bZoomIn)
{
    UINT i, NewZoom;

    if (g_pImage == NULL)
        return;

    if (bZoomIn)    /* zoom in */
    {
        /* find next step */
        for (i = 0; i < _countof(s_ZoomSteps); ++i)
        {
            if (s_ZoomPercents < s_ZoomSteps[i])
                break;
        }
        if (i == _countof(s_ZoomSteps))
            NewZoom = MAX_ZOOM;
        else
            NewZoom = s_ZoomSteps[i];
    }
    else            /* zoom out */
    {
        /* find previous step */
        for (i = _countof(s_ZoomSteps); i > 0; )
        {
            --i;
            if (s_ZoomSteps[i] < s_ZoomPercents)
                break;
        }
        if (i < 0)
            NewZoom = MIN_ZOOM;
        else
            NewZoom = s_ZoomSteps[i];
    }

    /* Update toolbar and refresh screen */
    UpdateZoom(NewZoom, TRUE, TRUE);
}

static void ResetZoom(void)
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

static void pLoadImage(LPCWSTR szOpenFileName)
{
    /* check file presence */
    if (GetFileAttributesW(szOpenFileName) == 0xFFFFFFFF)
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
    Anime_LoadInfo();

    if (szOpenFileName && szOpenFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szOpenFileName);

    /* Reset zoom and redraw display */
    ResetZoom();
}

static void pSaveImageAs(HWND hwnd)
{
    OPENFILENAMEW sfn;
    ImageCodecInfo *codecInfo;
    WCHAR szSaveFileName[MAX_PATH];
    WCHAR *szFilterMask;
    GUID rawFormat;
    UINT num;
    UINT size;
    size_t sizeRemain;
    UINT j;
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
    sizeRemain = sizeRemain + (sizeof(WCHAR) * 2);

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
    sfn.hInstance   = g_hInstance;
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
        if (m_pDelayItem)
        {
            /* save animation */
            KillTimer(g_hDispWnd, ANIME_TIMER_ID);

            DPRINT1("FIXME: save animation\n");
            if (GdipSaveImageToFile(g_pImage, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
            {
                DPRINT1("GdipSaveImageToFile() failed\n");
            }

            SetTimer(g_hDispWnd, ANIME_TIMER_ID, 0, NULL);
        }
        else
        {
            /* save non-animation */
            if (GdipSaveImageToFile(g_pImage, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
            {
                DPRINT1("GdipSaveImageToFile() failed\n");
            }
        }
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
EnableToolBarButtons(BOOL bEnable)
{
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_SAVEAS, bEnable);
    SendMessageW(g_hToolBar, TB_ENABLEBUTTON, IDC_PRINT, bEnable);
}

static VOID
pLoadImageFromNode(SHIMGVW_FILENODE *node, HWND hwnd)
{
    WCHAR szTitleBuf[800];
    WCHAR szResStr[512];
    LPWSTR pchFileTitle;

    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    if (node == NULL)
    {
        EnableToolBarButtons(FALSE);
        return;
    }

    pLoadImage(node->FileName);

    LoadStringW(g_hInstance, IDS_APPTITLE, szResStr, _countof(szResStr));

    pchFileTitle = PathFindFileNameW(node->FileName);
    if (pchFileTitle && *pchFileTitle) 
    {
        StringCbPrintfW(szTitleBuf, sizeof(szTitleBuf),
                        L"%ls%ls%ls", szResStr, L" - ", pchFileTitle);
        SetWindowTextW(hwnd, szTitleBuf);
    }
    else
    {
        SetWindowTextW(hwnd, szResStr);
    }

    EnableToolBarButtons(g_pImage != NULL);

    /* Redraw the display window */
    InvalidateRect(hwnd, NULL, FALSE);
}

static SHIMGVW_FILENODE*
pBuildFileList(LPWSTR szFirstFile)
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
                    if (wcscmp(szFirstFile, conductor->FileName) == 0)
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

static VOID
ImageView_UpdateWindow(HWND hwnd)
{
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
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
ImageView_DrawImage(HWND hwnd)
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

        ZoomedWidth = (ImageWidth * s_ZoomPercents) / 100;
        ZoomedHeight = (ImageHeight * s_ZoomPercents) / 100;

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
        DPRINT("s_ZoomPercents = %d, ZoomedWidth = %d, ZoomedHeight = %d\n",
               s_ZoomPercents, ZoomedWidth, ZoomedWidth);

        if (s_ZoomPercents % 100 == 0)
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

static BOOL
ImageView_LoadSettings(VOID)
{
    HKEY hKey;
    DWORD dwSize;
    LONG nError;

    g_Settings.Maximized = FALSE;
    g_Settings.X         = CW_USEDEFAULT;
    g_Settings.Y         = CW_USEDEFAULT;
    g_Settings.Width     = 520;
    g_Settings.Height    = 400;

    nError = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\shimgvw", 0, KEY_READ, &hKey);
    if (nError)
        return FALSE;

    dwSize = sizeof(g_Settings);
    nError = RegQueryValueExW(hKey, L"Settings", NULL, NULL, (LPBYTE)&g_Settings, &dwSize);
    RegCloseKey(hKey);

    return !nError;
}

static VOID
ImageView_SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    HKEY hKey;
    RECT *prc;

    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    prc = &wp.rcNormalPosition;
    g_Settings.X = prc->left;
    g_Settings.Y = prc->top;
    g_Settings.Width = prc->right - prc->left;
    g_Settings.Height = prc->bottom - prc->top;
    g_Settings.Maximized = IsZoomed(hwnd);

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("Settings"), 0, REG_BINARY, (LPBYTE)&g_Settings, sizeof(SHIMGVW_SETTINGS));
        RegCloseKey(hKey);
    }
}

static BOOL
ImageView_CreateToolBar(HWND hwnd)
{
    g_hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                                WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                                0, 0, 0, 0, hwnd,
                                0, g_hInstance, NULL);
    if (g_hToolBar != NULL)
    {
        HIMAGELIST hImageList;

        SendMessageW(g_hToolBar, TB_SETEXTENDEDSTYLE,
                     0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessageW(g_hToolBar, TB_BUTTONSTRUCTSIZE,
                     sizeof(s_Buttons[0]), 0);

        hImageList = ImageList_Create(TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, ILC_MASK | ILC_COLOR24, 1, 1);
        if (hImageList == NULL) return FALSE;

        for (UINT n = 0; n < _countof(s_BtnConfig); n++)
        {
            ImageList_AddMasked(hImageList, LoadImageW(g_hInstance, MAKEINTRESOURCEW(s_BtnConfig[n].idb), IMAGE_BITMAP,
                                TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));
        }

        ImageList_Destroy((HIMAGELIST)SendMessageW(g_hToolBar, TB_SETIMAGELIST,
                                                   0, (LPARAM)hImageList));

        SendMessageW(g_hToolBar, TB_ADDBUTTONS, _countof(s_Buttons), (LPARAM)s_Buttons);

        return TRUE;
    }

    return FALSE;
}

static void ImageView_OnTimer(HWND hwnd)
{
    DWORD dwDelay;

    KillTimer(hwnd, ANIME_TIMER_ID);
    InvalidateRect(hwnd, NULL, FALSE);

    if (Anime_Step(&dwDelay))
    {
        SetTimer(hwnd, ANIME_TIMER_ID, dwDelay, NULL);
    }
}

LRESULT CALLBACK
ImageView_DispWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_PAINT:
        {
            ImageView_DrawImage(hwnd);
            return 0L;
        }
        case WM_TIMER:
        {
            if (wParam == ANIME_TIMER_ID)
            {
                ImageView_OnTimer(hwnd);
                return 0;
            }
            break;
        }
    }
    return CallWindowProcW(g_PrevProc, hwnd, Message, wParam, lParam);
}

static VOID
ImageView_InitControls(HWND hwnd)
{
    g_hDispWnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_STATIC, L"",
                                 WS_CHILD | WS_VISIBLE,
                                 0, 0, 0, 0, hwnd, NULL, g_hInstance, NULL);

    SetClassLongPtr(g_hDispWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
    g_PrevProc = (WNDPROC) SetWindowLongPtr(g_hDispWnd, GWLP_WNDPROC, (LPARAM) ImageView_DispWndProc);

    ImageView_CreateToolBar(hwnd);
}

static VOID
ImageView_OnMouseWheel(HWND hwnd, INT x, INT y, INT zDelta, UINT fwKeys)
{
    if (zDelta != 0)
    {
        if (GetKeyState(VK_CONTROL) < 0)
            ZoomInOrOut(zDelta > 0);
    }
}

static VOID
ImageView_OnSize(HWND hwnd, UINT state, INT cx, INT cy)
{
    RECT rc;

    SendMessageW(g_hToolBar, TB_AUTOSIZE, 0, 0);

    GetWindowRect(g_hToolBar, &rc);

    MoveWindow(g_hDispWnd, 0, 0, cx, cy - (rc.bottom - rc.top), TRUE);

    /* is it maximized or restored? */
    if (state == SIZE_MAXIMIZED || state == SIZE_RESTORED)
    {
        /* reset zoom */
        ResetZoom();
    }
}

static LRESULT
ImageView_Delete(HWND hwnd)
{
    WCHAR szCurFile[MAX_PATH + 1], szNextFile[MAX_PATH];
    SHFILEOPSTRUCT FileOp = { hwnd, FO_DELETE };

    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    /* FileOp.pFrom must be double-null-terminated */
    GetFullPathNameW(g_currentFile->FileName, _countof(szCurFile) - 1, szCurFile, NULL);
    szCurFile[_countof(szCurFile) - 2] = UNICODE_NULL; /* Avoid buffer overrun */
    szCurFile[lstrlenW(szCurFile) + 1] = UNICODE_NULL;

    GetFullPathNameW(g_currentFile->Next->FileName, _countof(szNextFile), szNextFile, NULL);
    szNextFile[_countof(szNextFile) - 1] = UNICODE_NULL; /* Avoid buffer overrun */

    FileOp.pFrom = szCurFile;
    FileOp.fFlags = FOF_ALLOWUNDO;
    if (SHFileOperation(&FileOp) != 0)
        return 0;

    pFreeFileList(g_currentFile);
    g_currentFile = NULL;

    g_currentFile = pBuildFileList(szNextFile);
    pLoadImageFromNode(g_currentFile, hwnd);

    return 1;
}

static LRESULT
ImageView_Modify(HWND hwnd)
{
    int nChars = GetFullPathNameW(g_currentFile->FileName, 0, NULL, NULL);
    LPWSTR pszPathName;
    SHELLEXECUTEINFOW sei;

    if (!nChars)
    {
        DPRINT1("ImageView_Modify: failed to get full path name.\n");
        return 1;
    }

    pszPathName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, nChars * sizeof(WCHAR));
    if (pszPathName == NULL)
    {
        DPRINT1("HeapAlloc() failed in ImageView_Modify()\n");
        return 1;
    }

    GetFullPathNameW(g_currentFile->FileName, nChars, pszPathName, NULL);

    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = NULL;
    sei.lpVerb = L"edit";
    sei.lpFile = pszPathName;
    sei.lpParameters = NULL;
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.hInstApp = NULL;

    if (!ShellExecuteExW(&sei))
    {
        DPRINT1("ImageView_Modify: ShellExecuteExW() failed with code %08X\n", (int)GetLastError());
    }

    HeapFree(GetProcessHeap(), 0, pszPathName);

    return 0;
}

LRESULT CALLBACK
ImageView_WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
        {
            ImageView_InitControls(hwnd);
            return 0L;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_PREV_PIC:
                    g_currentFile = g_currentFile->Prev;
                    pLoadImageFromNode(g_currentFile, hwnd);
                    break;

                case IDC_NEXT_PIC:
                    g_currentFile = g_currentFile->Next;
                    pLoadImageFromNode(g_currentFile, hwnd);
                    break;

                case IDC_BEST_FIT:
                    ResetZoom();
                    break;

                case IDC_REAL_SIZE:
                    UpdateZoom(100, TRUE, FALSE);
                    return 0;

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
                        ImageView_UpdateWindow(hwnd);
                    }
                    break;

                case IDC_ROT_COUNCW:
                    if (g_pImage)
                    {
                        GdipImageRotateFlip(g_pImage, Rotate90FlipNone);
                        ImageView_UpdateWindow(hwnd);
                    }
                    break;

                case IDC_DELETE:
                    return ImageView_Delete(hwnd);

                case IDC_MODIFY:
                    return ImageView_Modify(hwnd);
            }
        }
        break;

        case WM_MOUSEWHEEL:
            ImageView_OnMouseWheel(hwnd,
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                (SHORT)HIWORD(wParam), (UINT)LOWORD(wParam));
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXTW lpttt;

                    lpttt = (LPTOOLTIPTEXTW)lParam;
                    lpttt->hinst = g_hInstance;

                    lpttt->lpszText = MAKEINTRESOURCEW(s_BtnConfig[lpttt->hdr.idFrom - IDC_TOOL_BASE].ids);
                    return 0;
                }
            }
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *pMMI = (MINMAXINFO*)lParam;
            pMMI->ptMinTrackSize.x = 350;
            pMMI->ptMinTrackSize.y = 290;
            return 0;
        }
        case WM_SIZE:
        {
            ImageView_OnSize(hwnd, (UINT)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_DESTROY:
        {
            ImageView_SaveSettings(hwnd);
            SetWindowLongPtr(g_hDispWnd, GWLP_WNDPROC, (LPARAM)g_PrevProc);
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProcW(hwnd, Message, wParam, lParam);
}

LONG WINAPI
ImageView_CreateWindow(HWND hwnd, LPCWSTR szFileName)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    WNDCLASSW WndClass = {0};
    WCHAR szBuf[512];
    WCHAR szInitialFile[MAX_PATH];
    HWND hMainWnd;
    MSG msg;
    HACCEL hKbdAccel;
    HRESULT hComRes;
    INITCOMMONCONTROLSEX Icc = { .dwSize = sizeof(Icc), .dwICC = ICC_WIN95_CLASSES };

    InitCommonControlsEx(&Icc);

    hComRes = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hComRes != S_OK && hComRes != S_FALSE)
    {
        DPRINT1("Warning, CoInitializeEx failed with code=%08X\n", (int)hComRes);
    }

    ImageView_LoadSettings();

    // Initialize GDI+
    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs      = FALSE;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    pLoadImage(szFileName);

    // Create the window
    WndClass.lpszClassName  = WC_SHIMGVW;
    WndClass.lpfnWndProc    = ImageView_WndProc;
    WndClass.hInstance      = g_hInstance;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    WndClass.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    WndClass.hbrBackground  = NULL;   /* less flicker */

    if (!RegisterClassW(&WndClass)) return -1;

    LoadStringW(g_hInstance, IDS_APPTITLE, szBuf, _countof(szBuf));
    hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE, WC_SHIMGVW, szBuf,
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
                               g_Settings.X, g_Settings.Y,
                               g_Settings.Width, g_Settings.Height,
                               NULL, NULL, g_hInstance, NULL);
    if (g_Settings.Maximized)
        ShowWindow(hMainWnd, SW_MAXIMIZE);

    // make sure the path has no quotes on it
    StringCbCopyW(szInitialFile, sizeof(szInitialFile), szFileName);
    PathUnquoteSpacesW(szInitialFile);

    g_currentFile = pBuildFileList(szInitialFile);
    if (g_currentFile)
    {
        pLoadImageFromNode(g_currentFile, hMainWnd);
    }

    /* Create accelerator table for keystrokes */
    hKbdAccel = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR));

    // Show it
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // Message Loop
    for (;;)
    {
        if (GetMessageW(&msg, NULL, 0, 0) <= 0)
            break;

        if (!TranslateAcceleratorW(hMainWnd, hKbdAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    /* Destroy accelerator table */
    DestroyAcceleratorTable(hKbdAccel);

    pFreeFileList(g_currentFile);

    if (g_pImage)
    {
        GdipDisposeImage(g_pImage);
        g_pImage = NULL;
    }

    Anime_FreeInfo();

    GdiplusShutdown(gdiplusToken);

    /* Release COM resources */
    if (SUCCEEDED(hComRes))
        CoUninitialize();

    return -1;
}

VOID WINAPI
ImageView_FullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, path);
}

VOID WINAPI
ImageView_Fullscreen(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, path);
}

VOID WINAPI
ImageView_FullscreenA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, path, -1, szFile, _countof(szFile)))
    {
        ImageView_CreateWindow(hwnd, szFile);
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
        case DLL_THREAD_ATTACH:
            g_hInstance = hinstDLL;
            break;
    }

    return TRUE;
}
