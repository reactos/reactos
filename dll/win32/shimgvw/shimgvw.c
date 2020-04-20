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

#define NDEBUG
#include <debug.h>

#include "shimgvw.h"

HINSTANCE hInstance;
SHIMGVW_SETTINGS shiSettings;
SHIMGVW_FILENODE *currentFile;
GpImage *image = NULL;
WNDPROC PrevProc = NULL;

HWND hDispWnd, hToolBar;

/* zooming */
#define MIN_ZOOM 10
#define MAX_ZOOM 1600
UINT ZoomPercents = 100;
static const UINT ZoomSteps[] =
{
    10, 25, 50, 100, 200, 400, 800, 1600
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
    KillTimer(hDispWnd, ANIME_TIMER_ID);

    if (!image)
        return FALSE;

    GdipImageGetFrameDimensionsCount(image, &nDimCount);
    if (nDimCount)
    {
        dims = (GUID *)calloc(nDimCount, sizeof(GUID));
        if (dims)
        {
            GdipImageGetFrameDimensionsList(image, dims, nDimCount);
            GdipImageGetFrameCount(image, dims, &result);
            m_nFrameCount = result;
            free(dims);
        }
    }

    result = 0;
    GdipGetPropertyItemSize(image, PropertyTagFrameDelay, &result);
    cbItem = result;
    if (cbItem)
    {
        m_pDelayItem = (PropertyItem *)malloc(cbItem);
        GdipGetPropertyItem(image, PropertyTagFrameDelay, cbItem, m_pDelayItem);
    }

    result = 0;
    GdipGetPropertyItemSize(image, PropertyTagLoopCount, &result);
    cbItem = result;
    if (cbItem)
    {
        pItem = (PropertyItem *)malloc(cbItem);
        if (pItem)
        {
            if (GdipGetPropertyItem(image, PropertyTagLoopCount, cbItem, pItem) == Ok)
            {
                m_nLoopCount = *(WORD *)pItem->value;
            }
            free(pItem);
        }
    }

    if (m_pDelayItem)
    {
        SetTimer(hDispWnd, ANIME_TIMER_ID, 0, NULL);
    }

    return m_pDelayItem != NULL;
}

static void Anime_SetFrameIndex(UINT nFrameIndex)
{
    if (nFrameIndex < m_nFrameCount)
    {
        GUID guid = FrameDimensionTime;
        if (Ok != GdipImageSelectActiveFrame(image, &guid, nFrameIndex))
        {
            guid = FrameDimensionPage;
            GdipImageSelectActiveFrame(image, &guid, nFrameIndex);
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

static void ZoomInOrOut(BOOL bZoomIn)
{
    INT i;

    if (image == NULL)
        return;

    if (bZoomIn)    /* zoom in */
    {
        /* find next step */
        for (i = 0; i < ARRAYSIZE(ZoomSteps); ++i)
        {
            if (ZoomPercents < ZoomSteps[i])
                break;
        }
        if (i == ARRAYSIZE(ZoomSteps))
            ZoomPercents = MAX_ZOOM;
        else
            ZoomPercents = ZoomSteps[i];

        /* update tool bar buttons */
        SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMM, TRUE);
        if (ZoomPercents >= MAX_ZOOM)
            SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMP, FALSE);
        else
            SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMP, TRUE);
    }
    else            /* zoom out */
    {
        /* find previous step */
        for (i = ARRAYSIZE(ZoomSteps); i > 0; )
        {
            --i;
            if (ZoomSteps[i] < ZoomPercents)
                break;
        }
        if (i < 0)
            ZoomPercents = MIN_ZOOM;
        else
            ZoomPercents = ZoomSteps[i];

        /* update tool bar buttons */
        SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMP, TRUE);
        if (ZoomPercents <= MIN_ZOOM)
            SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMM, FALSE);
        else
            SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_ZOOMM, TRUE);
    }

    /* redraw */
    InvalidateRect(hDispWnd, NULL, TRUE);
}

static void ResetZoom(void)
{
    RECT Rect;
    UINT ImageWidth, ImageHeight;

    if (image == NULL)
        return;

    /* get disp window size and image size */
    GetClientRect(hDispWnd, &Rect);
    GdipGetImageWidth(image, &ImageWidth);
    GdipGetImageHeight(image, &ImageHeight);

    /* compare two aspect rates. same as
       (ImageHeight / ImageWidth < Rect.bottom / Rect.right) in real */
    if (ImageHeight * Rect.right < Rect.bottom * ImageWidth)
    {
        if (Rect.right < ImageWidth)
        {
            /* it's large, shrink it */
            ZoomPercents = (Rect.right * 100) / ImageWidth;
        }
        else
        {
            /* it's small. show as original size */
            ZoomPercents = 100;
        }
    }
    else
    {
        if (Rect.bottom < ImageHeight)
        {
            /* it's large, shrink it */
            ZoomPercents = (Rect.bottom * 100) / ImageHeight;
        }
        else
        {
            /* it's small. show as original size */
            ZoomPercents = 100;
        }
    }
}

/* ToolBar Buttons */
static const TBBUTTON Buttons [] =
{   /* iBitmap,     idCommand,   fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_PREV,   IDC_PREV,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_NEXT,   IDC_NEXT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ZOOMP,  IDC_ZOOMP,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ZOOMM,  IDC_ZOOMM,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ROT1,   IDC_ROT1,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ROT2,   IDC_ROT2,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_SAVE,   IDC_SAVE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_PRINT,  IDC_PRINT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
};

static void pLoadImage(LPWSTR szOpenFileName)
{
    /* check file presence */
    if (GetFileAttributesW(szOpenFileName) == 0xFFFFFFFF)
    {
        DPRINT1("File %s not found!\n", szOpenFileName);
        return;
    }

    /* load now */
    GdipLoadImageFromFile(szOpenFileName, &image);
    if (!image)
    {
        DPRINT1("GdipLoadImageFromFile() failed\n");
        return;
    }
    Anime_LoadInfo();

    if (szOpenFileName && szOpenFileName[0])
        SHAddToRecentDocs(SHARD_PATHW, szOpenFileName);

    /* reset zoom */
    ResetZoom();

    /* redraw */
    InvalidateRect(hDispWnd, NULL, TRUE);
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

    if (image == NULL)
        return;

    GdipGetImageEncodersSize(&num, &size);
    codecInfo = malloc(size);
    if (!codecInfo)
    {
        DPRINT1("malloc() failed in pSaveImageAs()\n");
        return;
    }

    GdipGetImageEncoders(num, size, codecInfo);
    GdipGetImageRawFormat(image, &rawFormat);

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
    sfn.hInstance   = hInstance;
    sfn.lpstrFile   = szSaveFileName;
    sfn.lpstrFilter = szFilterMask;
    sfn.nMaxFile    = MAX_PATH;
    sfn.Flags       = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

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
            KillTimer(hDispWnd, ANIME_TIMER_ID);

            DPRINT1("FIXME: save animation\n");
            if (GdipSaveImageToFile(image, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
            {
                DPRINT1("GdipSaveImageToFile() failed\n");
            }

            SetTimer(hDispWnd, ANIME_TIMER_ID, 0, NULL);
        }
        else
        {
            /* save non-animation */
            if (GdipSaveImageToFile(image, szSaveFileName, &codecInfo[sfn.nFilterIndex - 1].Clsid, NULL) != Ok)
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
    SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_SAVE, bEnable);
    SendMessage(hToolBar, TB_ENABLEBUTTON, IDC_PRINT, bEnable);
}

static VOID
pLoadImageFromNode(SHIMGVW_FILENODE *node, HWND hwnd)
{
    WCHAR szTitleBuf[800];
    WCHAR szResStr[512];
    LPWSTR pchFileTitle;

    if (image)
    {
        GdipDisposeImage(image);
        image = NULL;
    }

    if (node == NULL)
    {
        EnableToolBarButtons(FALSE);
        return;
    }

    pLoadImage(node->FileName);

    LoadStringW(hInstance, IDS_APPTITLE, szResStr, ARRAYSIZE(szResStr));
    if (image != NULL)
    {
        pchFileTitle = PathFindFileNameW(node->FileName);
        StringCbPrintfW(szTitleBuf, sizeof(szTitleBuf),
                        L"%ls%ls%ls", szResStr, L" - ", pchFileTitle);
        SetWindowTextW(hwnd, szTitleBuf);
    }
    else
    {
        SetWindowTextW(hwnd, szResStr);
    }

    EnableToolBarButtons(image != NULL);
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

    if (image == NULL)
    {
        FillRect(hdc, &rect, white);

        LoadStringW(hInstance, IDS_NOPREVIEW, szText, ARRAYSIZE(szText));

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        hFontOld = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        DrawTextW(hdc, szText, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER |
                                          DT_NOPREFIX);
        SelectObject(hdc, hFontOld);
    }
    else
    {
        GdipGetImageWidth(image, &ImageWidth);
        GdipGetImageHeight(image, &ImageHeight);

        ZoomedWidth = (ImageWidth * ZoomPercents) / 100;
        ZoomedHeight = (ImageHeight * ZoomPercents) / 100;

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
        DPRINT("ZoomPercents = %d, ZoomedWidth = %d, ZoomedHeight = %d\n",
               ZoomPercents, ZoomedWidth, ZoomedWidth);

        if (ZoomPercents % 100 == 0)
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
        GdipGetImageFlags(image, &uFlags);

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

        GdipDrawImageRectI(graphics, image, x, y, ZoomedWidth, ZoomedHeight);
    }
    GdipDeleteGraphics(graphics);
    EndPaint(hwnd, &ps);
}

static BOOL
ImageView_LoadSettings()
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(SHIMGVW_SETTINGS);
        if (RegQueryValueEx(hKey, _T("Settings"), NULL, NULL, (LPBYTE)&shiSettings, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}

static VOID
ImageView_SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    HKEY hKey;

    ShowWindow(hwnd, SW_HIDE);
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    shiSettings.Left = wp.rcNormalPosition.left;
    shiSettings.Top  = wp.rcNormalPosition.top;
    shiSettings.Right  = wp.rcNormalPosition.right;
    shiSettings.Bottom = wp.rcNormalPosition.bottom;
    shiSettings.Maximized = (IsZoomed(hwnd) || (wp.flags & WPF_RESTORETOMAXIMIZED));

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("Settings"), 0, REG_BINARY, (LPBYTE)&shiSettings, sizeof(SHIMGVW_SETTINGS));
        RegCloseKey(hKey);
    }
}

static BOOL
ImageView_CreateToolBar(HWND hwnd)
{
    INT numButtons = sizeof(Buttons) / sizeof(Buttons[0]);

    hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                              WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                              0, 0, 0, 0, hwnd,
                              0, hInstance, NULL);
    if(hToolBar != NULL)
    {
        HIMAGELIST hImageList;

        SendMessage(hToolBar, TB_SETEXTENDEDSTYLE,
                    0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE,
                    sizeof(Buttons[0]), 0);

        hImageList = ImageList_Create(TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, ILC_MASK | ILC_COLOR24, 1, 1);

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREVICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMPICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMMICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_SAVEICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PRINTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT1ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT2ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        if (hImageList == NULL) return FALSE;

        ImageList_Destroy((HIMAGELIST)SendMessage(hToolBar, TB_SETIMAGELIST,
                                                  0, (LPARAM)hImageList));

        SendMessage(hToolBar, TB_ADDBUTTONS,
                    numButtons, (LPARAM)Buttons);

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
    return CallWindowProc(PrevProc, hwnd, Message, wParam, lParam);
}

static VOID
ImageView_InitControls(HWND hwnd)
{
    MoveWindow(hwnd, shiSettings.Left, shiSettings.Top,
               shiSettings.Right - shiSettings.Left,
               shiSettings.Bottom - shiSettings.Top, TRUE);

    if (shiSettings.Maximized) ShowWindow(hwnd, SW_MAXIMIZE);

    hDispWnd = CreateWindowEx(0, WC_STATIC, _T(""),
                              WS_CHILD | WS_VISIBLE,
                              0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    SetClassLongPtr(hDispWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
    PrevProc = (WNDPROC) SetWindowLongPtr(hDispWnd, GWLP_WNDPROC, (LPARAM) ImageView_DispWndProc);

    ImageView_CreateToolBar(hwnd);
}

static VOID
ImageView_OnMouseWheel(HWND hwnd, INT x, INT y, INT zDelta, UINT fwKeys)
{
    if (zDelta != 0)
    {
        ZoomInOrOut(zDelta > 0);
    }
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

        case WM_KEYDOWN:
            switch (LOWORD(wParam))
            {
                case VK_LEFT:
                    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PREV, BN_CLICKED), (LPARAM)NULL);
                    break;

                case VK_RIGHT:
                    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_NEXT, BN_CLICKED), (LPARAM)NULL);
                    break;
            }
            break;

        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_PREV:
                {
                    currentFile = currentFile->Prev;
                    pLoadImageFromNode(currentFile, hwnd);
                }
                break;

                case IDC_NEXT:
                {
                    currentFile = currentFile->Next;
                    pLoadImageFromNode(currentFile, hwnd);
                }
                break;

                case IDC_ZOOMP:
                    ZoomInOrOut(TRUE);
                    break;

                case IDC_ZOOMM:
                    ZoomInOrOut(FALSE);
                    break;

                case IDC_SAVE:
                    pSaveImageAs(hwnd);
                    break;

                case IDC_PRINT:
                    pPrintImage(hwnd);
                    break;

                case IDC_ROT1:
                {
                    if (image)
                    {
                        GdipImageRotateFlip(image, Rotate270FlipNone);
                        ImageView_UpdateWindow(hwnd);
                    }
                    break;
                }

                case IDC_ROT2:
                {
                    if (image)
                    {
                        GdipImageRotateFlip(image, Rotate90FlipNone);
                        ImageView_UpdateWindow(hwnd);
                    }
                    break;
                }
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
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT)lParam;
                    idButton = (UINT)lpttt->hdr.idFrom;
                    lpttt->hinst = hInstance;

                    switch (idButton)
                    {
                        case IDC_PREV:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PREV_PIC);
                        break;
                        case IDC_NEXT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEXT_PIC);
                        break;
                        case IDC_ZOOMP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_IN);
                        break;
                        case IDC_ZOOMM:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_OUT);
                        break;
                        case IDC_SAVE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SAVEAS);
                        break;
                        case IDC_PRINT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PRINT);
                        break;
                        case IDC_ROT1:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_COUNCW);
                        break;
                        case IDC_ROT2:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_CLOCKW);
                        break;
                    }
                    return TRUE;
                }
            }
            break;
        }
        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;
            if (pRect->right-pRect->left < 350)
                pRect->right = pRect->left + 350;

            if (pRect->bottom-pRect->top < 290)
                pRect->bottom = pRect->top + 290;
            return TRUE;
        }
        case WM_SIZE:
        {
            RECT rc;
            SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
            GetWindowRect(hToolBar, &rc);
            MoveWindow(hDispWnd, 1, 1, LOWORD(lParam) - 1, HIWORD(lParam) - (rc.bottom - rc.top) - 1, TRUE);
            /* is it maximized or restored? */
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
            {
                /* reset zoom */
                ResetZoom();
            }
            return 0L;
        }
        case WM_DESTROY:
        {
            ImageView_SaveSettings(hwnd);
            SetWindowLongPtr(hDispWnd, GWLP_WNDPROC, (LPARAM) PrevProc);
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, Message, wParam, lParam);
}

LONG WINAPI
ImageView_CreateWindow(HWND hwnd, LPWSTR szFileName)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    WNDCLASS WndClass = {0};
    TCHAR szBuf[512];
    WCHAR szInitialFile[MAX_PATH];
    HWND hMainWnd;
    MSG msg;

    if (!ImageView_LoadSettings())
    {
        shiSettings.Maximized = FALSE;
        shiSettings.Left      = 0;
        shiSettings.Top       = 0;
        shiSettings.Right     = 520;
        shiSettings.Bottom    = 400;
    }

    // Initialize GDI+
    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs      = FALSE;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    pLoadImage(szFileName);

    // Create the window
    WndClass.lpszClassName  = _T("shimgvw_window");
    WndClass.lpfnWndProc    = ImageView_WndProc;
    WndClass.hInstance      = hInstance;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    WndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground  = NULL;   /* less flicker */

    if (!RegisterClass(&WndClass)) return -1;

    LoadString(hInstance, IDS_APPTITLE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    hMainWnd = CreateWindow(_T("shimgvw_window"), szBuf,
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CAPTION,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            0, 0, NULL, NULL, hInstance, NULL);

    // make sure the path has no quotes on it
    wcscpy(szInitialFile, szFileName);
    PathUnquoteSpacesW(szInitialFile);

    currentFile = pBuildFileList(szInitialFile);
    if (currentFile)
    {
        pLoadImageFromNode(currentFile, hMainWnd);
    }

    // Show it
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // Message Loop
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    pFreeFileList(currentFile);

    if (image)
    {
        GdipDisposeImage(image);
        image = NULL;
    }

    Anime_FreeInfo();

    GdiplusShutdown(gdiplusToken);
    return -1;
}

VOID WINAPI
ImageView_FullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_Fullscreen(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_FullscreenA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, (char*)path, strlen((char*)path)+1, szFile, MAX_PATH))
    {
        ImageView_CreateWindow(hwnd, (LPWSTR)szFile);
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
            hInstance = hinstDLL;
            break;
    }

    return TRUE;
}
