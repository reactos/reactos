/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/font.c
 * PURPOSE:         Font dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>


/*
 * Current active font, corresponding to the active console font,
 * and used for painting the text samples.
 */
FONT_PREVIEW FontPreview = {NULL, 0, 0};


/*
 * Standard font pixel/point heights for TrueType fonts
 */
static const SHORT TrueTypePoints[] =
{
    5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72
};

typedef struct _FONTSIZE_LIST_CTL
{
    LIST_CTL RasterSizeList;    // ListBox for Raster font sizes; needs to handle bisection.
    HWND hWndTTSizeList;        // ComboBox for TrueType font sizes.
    BOOL bIsTTSizeDirty;        // TRUE or FALSE depending on whether we have edited the edit zone.
    BOOL UseRasterOrTTList;     // TRUE: Use the Raster size list; FALSE: Use the TrueType size list.
    BOOL TTSizePixelUnit;       // TRUE: Size in pixels (default); FALSE: Size in points.
    LONG CurrentRasterSize;
    LONG CurrentTTSize;         // In whatever unit (pixels or points) currently selected.
} FONTSIZE_LIST_CTL, *PFONTSIZE_LIST_CTL;

/* Used by FontTypeChange() only */
static INT   CurrentSelFont  = LB_ERR;
static DWORD CurrentFontType = (DWORD)-1;   // Invalid font type

/* Detect whether the current code page has changed */
static UINT CurrentCodePage = INVALID_CP;


VOID
RefreshFontPreview(
    IN FONT_PREVIEW* Preview,
    IN PCONSOLE_STATE_INFO pConInfo)
{
    HFONT hFont = CreateConsoleFont(pConInfo);
    if (!hFont)
    {
        DPRINT1("RefreshFontPreview: CreateConsoleFont() failed\n");
        return;
    }

    if (Preview->hFont) DeleteObject(Preview->hFont);
    Preview->hFont = hFont;
    GetFontCellSize(NULL, hFont, &Preview->CharHeight, &Preview->CharWidth);
}

VOID
UpdateFontPreview(
    IN FONT_PREVIEW* Preview,
    IN HFONT hFont,
    IN UINT  CharWidth,
    IN UINT  CharHeight)
{
    if (Preview->hFont) DeleteObject(Preview->hFont);
    Preview->hFont = hFont;
    Preview->CharWidth  = CharWidth;
    Preview->CharHeight = CharHeight;
}

// PLIST_GETCOUNT
static INT
RasterSizeList_GetCount(
    IN PLIST_CTL ListCtl)
{
    return (INT)SendMessageW(ListCtl->hWndList, LB_GETCOUNT, 0, 0);
}

// PLIST_GETDATA
static ULONG_PTR
RasterSizeList_GetData(
    IN PLIST_CTL ListCtl,
    IN INT Index)
{
    return (ULONG_PTR)SendMessageW(ListCtl->hWndList, LB_GETITEMDATA, (WPARAM)Index, 0);
}


INT
LogicalSizeToPointSize(
    IN HDC hDC OPTIONAL,
    IN UINT LogicalSize)
{
    INT PointSize;
    HDC hOrgDC = hDC;

    if (!hDC)
        hDC = GetDC(NULL);

    // LogicalSize = tm.tmHeight - tm.tmInternalLeading;
    PointSize = MulDiv(LogicalSize, 72, GetDeviceCaps(hDC, LOGPIXELSY));

    if (!hOrgDC)
        ReleaseDC(NULL, hDC);

    return PointSize;
}

INT
PointSizeToLogicalSize(
    IN HDC hDC OPTIONAL,
    IN INT PointSize)
{
    INT LogicalSize;
    HDC hOrgDC = hDC;

    if (!hDC)
        hDC = GetDC(NULL);

    LogicalSize = MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);

    if (!hOrgDC)
        ReleaseDC(NULL, hDC);

    return LogicalSize;
}


static VOID
FontSizeList_SelectFontSize(
    IN PFONTSIZE_LIST_CTL SizeList,
    IN ULONG FontSize)
{
    INT nSel;
    WCHAR szFontSize[100];

    //
    // FIXME: Check whether FontSize == 0
    // (or in the case of raster font maybe, whether HIWORD(FontSize) == Height == 0) ??
    //

    /* Find and select the best font size in the list corresponding to the current size */
    if (SizeList->UseRasterOrTTList)
    {
        INT idx;

        /* Raster font size (in pixels) */
        SizeList->CurrentRasterSize = FontSize;

        nSel = BisectListSortedByValue(&SizeList->RasterSizeList, FontSize, NULL, FALSE);
        idx  = (INT)SendMessageW(SizeList->RasterSizeList.hWndList, LB_GETCOUNT, 0, 0);
        if (nSel == LB_ERR)
        {
            /* Not found, select the first element of the list */
            nSel = 0;
        }
        else if (nSel >= idx)
        {
            /*
             * We got an index beyond the end of the list (as per Bisect* functionality),
             * so instead, select the last element of the list.
             */
            nSel = idx - 1;
        }
        SendMessageW(SizeList->RasterSizeList.hWndList, LB_SETCURSEL, (WPARAM)nSel, 0);
    }
    else
    {
        /* TrueType font size (in pixels or points) */
        SizeList->CurrentTTSize = FontSize;

        // _ultow(szFontSize, FontSize, 10);
        StringCchPrintfW(szFontSize, ARRAYSIZE(szFontSize), L"%d", FontSize);

        /* Find the font size in the list, or add it both in the ComboBox list, sorted by size value (string), and its edit box */
        nSel = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_FINDSTRINGEXACT, 0, (LPARAM)szFontSize);
        if (nSel == CB_ERR)
        {
            nSel = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_ADDSTRING, -1, (LPARAM)szFontSize);
            // ComboBox_SetText(...)
            SetWindowTextW(SizeList->hWndTTSizeList, szFontSize);
            SizeList->bIsTTSizeDirty = TRUE;
        }
        SendMessageW(SizeList->hWndTTSizeList, CB_SETCURSEL, (WPARAM)nSel, 0);
    }
}

static LONG
FontSizeList_GetSelectedFontSize(
    IN PFONTSIZE_LIST_CTL SizeList)
{
    INT nSel;
    LONG FontSize;
    PWCHAR pszNext = NULL;
    WCHAR szFontSize[100];

    if (SizeList->UseRasterOrTTList)
    {
        /* Raster font size (in pixels) */

        nSel = (INT)SendMessageW(SizeList->RasterSizeList.hWndList, LB_GETCURSEL, 0, 0);
        if (nSel == LB_ERR) return 0;

        FontSize = (LONG)SizeList->RasterSizeList.GetData(&SizeList->RasterSizeList, nSel);
        if (FontSize == LB_ERR) return 0;

        SizeList->CurrentRasterSize = FontSize;
    }
    else
    {
        /* TrueType font size (in pixels or points) */

        if (!SizeList->bIsTTSizeDirty)
        {
            /*
             * The user just selected an existing size, read the ComboBox selection.
             *
             * See: https://support.microsoft.com/en-us/help/66365/how-to-process-a-cbn-selchange-notification-message (DEAD_LINK)
             * for more details.
             */
            INT Length;

            nSel = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_GETCURSEL, 0, 0);
            if (nSel == CB_ERR) return 0;

            Length = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_GETLBTEXTLEN, nSel, 0);
            ASSERT((Length != LB_ERR) && (Length < ARRAYSIZE(szFontSize)));

            Length = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_GETLBTEXT, nSel, (LPARAM)szFontSize);
            ASSERT((Length != LB_ERR) && (Length < ARRAYSIZE(szFontSize)));
            szFontSize[Length] = L'\0';

            /* Validate the font size */
            FontSize = wcstoul(szFontSize, &pszNext, 10);
            if ((FontSize == 0) || (*pszNext))
                return 0;
        }
        else
        {
            /* Read the ComboBox edit string, as the user has entered a custom size */
            // ComboBox_GetText(...)
            GetWindowTextW(SizeList->hWndTTSizeList, szFontSize, ARRAYSIZE(szFontSize));

            /* Validate the font size */
            FontSize = wcstoul(szFontSize, &pszNext, 10);
            if ((FontSize == 0) || (*pszNext))
                return 0;

            /* Find if the font size already exists in the list; if not, add it */
            nSel = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_FINDSTRINGEXACT, 0, (LPARAM)szFontSize);
            if (nSel == CB_ERR)
            {
                nSel = (INT)SendMessageW(SizeList->hWndTTSizeList, CB_ADDSTRING, -1, (LPARAM)szFontSize);
                //// ComboBox_SetText(...)
                //SetWindowTextW(SizeList->hWndTTSizeList, szFontSize);
                //SizeList->bIsTTSizeDirty = TRUE;
            }
            SendMessageW(SizeList->hWndTTSizeList, CB_SETCURSEL, (WPARAM)nSel, 0);
        }

        SizeList->bIsTTSizeDirty = FALSE;

        SizeList->CurrentTTSize = FontSize;

        /*
         * If the font size is given in points, instead of pixels,
         * convert it into logical size.
         */
        if (!SizeList->TTSizePixelUnit)
            FontSize = -PointSizeToLogicalSize(NULL, FontSize);
    }

    return FontSize;
}


static VOID
AddFontToList(
    IN HWND hWndList,
    IN LPCWSTR pszFaceName,
    IN DWORD FontType)
{
    INT iItem;

    /* Make sure the font doesn't already exist in the list */
    if (SendMessageW(hWndList, LB_FINDSTRINGEXACT, 0, (LPARAM)pszFaceName) != LB_ERR)
        return;

    /* Add the font */
    iItem = (INT)SendMessageW(hWndList, LB_ADDSTRING, 0, (LPARAM)pszFaceName);
    if (iItem == LB_ERR)
    {
        DPRINT1("Failed to add font '%S'\n", pszFaceName);
        return;
    }

    DPRINT1("Add font '%S'\n", pszFaceName);

    /* Store this information in the list-item's userdata area */
    // SendMessageW(hWndList, LB_SETITEMDATA, idx, MAKELPARAM(fFixed, fTrueType));
    SendMessageW(hWndList, LB_SETITEMDATA, iItem, (LPARAM)FontType);
}

typedef struct _FACE_NAMES_PROC_PARAM
{
    HWND hWndList;
    UINT CodePage;
} FACE_NAMES_PROC_PARAM, *PFACE_NAMES_PROC_PARAM;

static BOOL CALLBACK
EnumFaceNamesProc(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD  FontType,
    IN LPARAM lParam)
{
    PFACE_NAMES_PROC_PARAM Param = (PFACE_NAMES_PROC_PARAM)lParam;

    if (IsValidConsoleFont2(lplf, lpntm, FontType, Param->CodePage))
    {
        /* Add the face name to the list */
        AddFontToList(Param->hWndList, lplf->lfFaceName, FontType);
    }

    /* Continue enumerating the faces */
    return TRUE;
}

static BOOL CALLBACK
EnumFontSizesProc(
    IN PLOGFONTW lplf,
    IN PNEWTEXTMETRICW lpntm,
    IN DWORD  FontType,
    IN LPARAM lParam)
{
    PFONTSIZE_LIST_CTL SizeList = (PFONTSIZE_LIST_CTL)lParam;
    UINT iItem, iDupItem;
    WCHAR szFontSize[100];

    if (FontType != TRUETYPE_FONTTYPE)
    {
        WPARAM FontSize;

        /*
         * Format:
         * Width  = FontSize.X = LOWORD(FontSize);
         * Height = FontSize.Y = HIWORD(FontSize);
         */

        StringCchPrintfW(szFontSize, ARRAYSIZE(szFontSize), L"%d x %d", lplf->lfWidth, lplf->lfHeight);
        FontSize = MAKEWPARAM(lplf->lfWidth, lplf->lfHeight);

        /* Add the font size into the list, sorted by size value. Avoid any duplicates. */
        /* Store this information in the list-item's userdata area */
        iDupItem = LB_ERR;
        iItem = BisectListSortedByValue(&SizeList->RasterSizeList, FontSize, &iDupItem, TRUE);
        if (iItem == LB_ERR)
            iItem = 0;
        if (iDupItem == LB_ERR)
        {
            iItem = (UINT)SendMessageW(SizeList->RasterSizeList.hWndList, LB_INSERTSTRING, iItem, (LPARAM)szFontSize);
            if (iItem != LB_ERR && iItem != LB_ERRSPACE)
                iItem = SendMessageW(SizeList->RasterSizeList.hWndList, LB_SETITEMDATA, iItem, FontSize);
        }

        return TRUE;
    }
    else
    {
        /* TrueType or vectored font: list all the hardcoded font points */
        ULONG i;
        for (i = 0; i < ARRAYSIZE(TrueTypePoints); ++i)
        {
            // _ultow(szFontSize, TrueTypePoints[i], 10);
            StringCchPrintfW(szFontSize, ARRAYSIZE(szFontSize), L"%d", TrueTypePoints[i]);

            /* Add the font size into the list, sorted by size value (string). Avoid any duplicates. */
            if (SendMessageW(SizeList->hWndTTSizeList, CB_FINDSTRINGEXACT, 0, (LPARAM)szFontSize) == CB_ERR)
                iItem = (UINT)SendMessageW(SizeList->hWndTTSizeList, CB_INSERTSTRING, -1, (LPARAM)szFontSize);
        }

        /* Stop the enumeration now */
        return FALSE;
    }
}

static VOID
FaceNameList_Initialize(
    IN HWND hWndList,
    IN UINT CodePage)
{
    FACE_NAMES_PROC_PARAM Param;
    HDC hDC;
    LOGFONTW lf;
    INT idx;

    /* Reset the face names list */
    SendMessageW(hWndList, LB_RESETCONTENT, 0, 0);

    Param.hWndList = hWndList;
    Param.CodePage = CodePage;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET; // CodePageToCharSet(CodePage);
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFaceNamesProc, (LPARAM)&Param, 0);
    ReleaseDC(NULL, hDC);

    idx = (INT)SendMessageW(hWndList, LB_GETCOUNT, 0, 0);
    if (idx != LB_ERR && idx != 0)
    {
        /* We have found some faces and filled the list, we are fine! */
        return;
    }

    /* No fonts were found. Manually add default ones into the list. */
    DPRINT1("The ideal console fonts were not found; manually add default ones.\n");

    AddFontToList(hWndList, L"Terminal", RASTER_FONTTYPE);
#if 0
    // TODO: insert only the *single* default TT font, that should
    // be found in the TT font cache with the codepage number 0.
    AddFontToList(hWndList, L"Lucida Console", TRUETYPE_FONTTYPE);
#endif
}

static VOID
FaceNameList_SelectFont(
    IN HWND hDlg,
    IN HWND hWndList,
    IN PFONTSIZE_LIST_CTL SizeList,
    IN LPCWSTR FaceName,
    IN ULONG FontFamily,
    IN ULONG FontWeight,
    IN COORD FontSize)
{
    INT iItem;

    iItem = (INT)SendMessageW(hWndList, LB_FINDSTRINGEXACT, 0, (LPARAM)FaceName);
    if (iItem == LB_ERR)
        iItem = (INT)SendMessageW(hWndList, LB_FINDSTRINGEXACT, 0, (LPARAM)L"Terminal");
    if (iItem == LB_ERR)
        iItem = 0;
    SendMessageW(hWndList, LB_SETCURSEL, (WPARAM)iItem, 0);

    if (FontWeight >= FW_BOLD)
        CheckDlgButton(hDlg, IDC_CHECK_BOLD_FONTS, BST_CHECKED);
    else
        CheckDlgButton(hDlg, IDC_CHECK_BOLD_FONTS, BST_UNCHECKED);

    /* Select the current font size */
    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    SizeList->CurrentRasterSize = MAKELONG(FontSize.X, FontSize.Y);
    SizeList->CurrentTTSize = FontSize.Y;
    // FontSizeList_SelectFontSize(SizeList, SizeList->CurrentRasterSize);

    // return iItem;
}

static VOID
UpdateFontSizeList(
    IN HWND hDlg,
    IN PFONTSIZE_LIST_CTL SizeList)
{
    HWND hDlgItem;

    if (SizeList->UseRasterOrTTList)
    {
        /*
         * Raster font: show the Raster size list, and
         * hide the TrueType size list and the units.
         */

        // EnableDlgItem(hDlg, IDC_CHECK_BOLD_FONTS, FALSE);

        hDlgItem = GetDlgItem(hDlg, IDC_RADIO_PIXEL_UNIT);
        ShowWindow(hDlgItem, SW_HIDE);
        EnableWindow(hDlgItem, FALSE);

        hDlgItem = GetDlgItem(hDlg, IDC_RADIO_POINT_UNIT);
        ShowWindow(hDlgItem, SW_HIDE);
        EnableWindow(hDlgItem, FALSE);

        hDlgItem = SizeList->hWndTTSizeList;
        ShowWindow(hDlgItem, SW_HIDE);
        EnableWindow(hDlgItem, FALSE);

        hDlgItem = SizeList->RasterSizeList.hWndList;
        EnableWindow(hDlgItem, TRUE);
        ShowWindow(hDlgItem, SW_SHOW);
    }
    else
    {
        /*
         * TrueType font: show the TrueType size list
         * and the units, and hide the Raster size list.
         */

        // EnableDlgItem(hDlg, IDC_CHECK_BOLD_FONTS, TRUE);

        hDlgItem = SizeList->RasterSizeList.hWndList;
        ShowWindow(hDlgItem, SW_HIDE);
        EnableWindow(hDlgItem, FALSE);

        hDlgItem = SizeList->hWndTTSizeList;
        EnableWindow(hDlgItem, TRUE);
        ShowWindow(hDlgItem, SW_SHOW);

        hDlgItem = GetDlgItem(hDlg, IDC_RADIO_PIXEL_UNIT);
        EnableWindow(hDlgItem, TRUE);
        ShowWindow(hDlgItem, SW_SHOW);

        hDlgItem = GetDlgItem(hDlg, IDC_RADIO_POINT_UNIT);
        EnableWindow(hDlgItem, TRUE);
        ShowWindow(hDlgItem, SW_SHOW);
    }
}

static BOOL
FontSizeChange(
    IN HWND hDlg,
    IN PFONTSIZE_LIST_CTL SizeList,
    IN OUT PCONSOLE_STATE_INFO pConInfo);

static BOOL
FontTypeChange(
    IN HWND hDlg,
    IN PFONTSIZE_LIST_CTL SizeList,
    IN OUT PCONSOLE_STATE_INFO pConInfo)
{
    HWND hFontList = GetDlgItem(hDlg, IDC_LBOX_FONTTYPE);
    INT Length, nSel;
    LPWSTR FaceName;
    DWORD FontType;
    LPCWSTR FontGrpBoxLabelTpl = NULL;
    WCHAR FontGrpBoxLabel[260];

    nSel = (INT)SendMessageW(hFontList, LB_GETCURSEL, 0, 0);
    if (nSel == LB_ERR) return FALSE;

    /*
     * This is disabled, because there can be external parameters
     * that may have changed (e.g. ConInfo->FontWeight, code page, ...)
     * and that we don't control here, and that need a font refresh.
     */
#if 0
    /* Check whether the selection has changed */
    if (nSel == CurrentSelFont)
        return FALSE;
#endif

    Length = (INT)SendMessageW(hFontList, LB_GETTEXTLEN, nSel, 0);
    if (Length == LB_ERR) return FALSE;

    FaceName = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         (Length + 1) * sizeof(WCHAR));
    if (FaceName == NULL) return FALSE;

    Length = (INT)SendMessageW(hFontList, LB_GETTEXT, nSel, (LPARAM)FaceName);
    ASSERT(Length != LB_ERR);
    FaceName[Length] = L'\0';

    StringCchCopyW(pConInfo->FaceName, ARRAYSIZE(pConInfo->FaceName), FaceName);
    DPRINT("pConInfo->FaceName = '%S'\n", pConInfo->FaceName);

    /*
     * Retrieve the read-only font group box label string template,
     * and set the group box label to the name of the selected font.
     */
    Length = LoadStringW(hApplet, IDS_GROUPBOX_FONT_NAME, (LPWSTR)&FontGrpBoxLabelTpl, 0);
    if (FontGrpBoxLabelTpl && Length > 0)
    {
        StringCchCopyNW(FontGrpBoxLabel, ARRAYSIZE(FontGrpBoxLabel), FontGrpBoxLabelTpl, Length);
        StringCchCatW(FontGrpBoxLabel, ARRAYSIZE(FontGrpBoxLabel), FaceName);
        SetDlgItemTextW(hDlg, IDC_GROUPBOX_FONT_NAME, FontGrpBoxLabel);
    }

    HeapFree(GetProcessHeap(), 0, FaceName);

    /*
     * Reset the font size list, only:
     * - if we have changed the type of font, or
     * - if the font type is the same and is RASTER but the font has changed.
     * Otherwise, if the font type is not RASTER and has not changed,
     * we always display the TrueType default sizes and we don't need to
     * recreate the list when we change between different TrueType fonts.
     */
    FontType = (DWORD)SendMessageW(hFontList, LB_GETITEMDATA, nSel, 0);
    if (FontType != LB_ERR)
    {
        SizeList->UseRasterOrTTList = (FontType == RASTER_FONTTYPE);

        /* Display the correct font size list (if needed) */
        if (CurrentFontType != FontType)
            UpdateFontSizeList(hDlg, SizeList);

        /* Enumerate the available sizes for the selected font */
        if ((CurrentFontType != FontType) ||
            (FontType == RASTER_FONTTYPE && CurrentSelFont != nSel))
        {
            LOGFONTW lf;
            HDC hDC;

            if (SizeList->UseRasterOrTTList)
                SendMessageW(SizeList->RasterSizeList.hWndList, LB_RESETCONTENT, 0, 0);
            else
                SendMessageW(SizeList->hWndTTSizeList, CB_RESETCONTENT, 0, 0);

            ZeroMemory(&lf, sizeof(lf));
            lf.lfCharSet = DEFAULT_CHARSET; // CodePageToCharSet(pConInfo->CodePage);
            // lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
            StringCchCopyW(lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), pConInfo->FaceName);

            hDC = GetDC(NULL);
            EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFontSizesProc, (LPARAM)SizeList, 0);
            ReleaseDC(NULL, hDC);

            /* Re-select the current font size */
            if (SizeList->UseRasterOrTTList)
                FontSizeList_SelectFontSize(SizeList, SizeList->CurrentRasterSize);
            else
                FontSizeList_SelectFontSize(SizeList, SizeList->CurrentTTSize);
        }
    }
    else
    {
        /* We failed, display the raster fonts size list */
        SizeList->UseRasterOrTTList = TRUE;
        UpdateFontSizeList(hDlg, SizeList);
    }
    CurrentFontType = FontType;
    CurrentSelFont  = nSel;

    FontSizeChange(hDlg, SizeList, pConInfo);
    return TRUE;
}

static BOOL
FontSizeChange(
    IN HWND hDlg,
    IN PFONTSIZE_LIST_CTL SizeList,
    IN OUT PCONSOLE_STATE_INFO pConInfo)
{
    LONG FontSize;
    UINT CharWidth, CharHeight;
    HFONT hFont;
    WCHAR szFontSize[100];

    /*
     * Retrieve the current selected font size.
     * - If SizeList->UseRasterOrTTList is TRUE, or if it is FALSE but
     *   if SizeList->TTSizePixelUnit is TRUE, then the font size is in pixels;
     * - If SizeList->TTSizePixelUnit is FALSE, then the font size is in points.
     */
    FontSize = FontSizeList_GetSelectedFontSize(SizeList);
    if (FontSize == 0)
        return FALSE; // We have got an invalid font size...

    /*
     * For TrueType fonts we set the requested width to zero
     * so as to obtain a default aspect-ratio width.
     */
    CharHeight = (UINT)(SizeList->UseRasterOrTTList ? HIWORD(FontSize) : FontSize);
    CharWidth  = (UINT)(SizeList->UseRasterOrTTList ? LOWORD(FontSize) : 0);

    hFont = CreateConsoleFont2((LONG)CharHeight, (LONG)CharWidth, pConInfo);
    if (!hFont)
    {
        DPRINT1("FontSizeChange: CreateConsoleFont2() failed\n");
        return FALSE;
    }

    /* Retrieve the real character size in pixels */
    GetFontCellSize(NULL, hFont, &CharHeight, &CharWidth);

    /*
     * Update the font preview as well, and store the font handle. It will be
     * freed at later update or when the font preview is refreshed or reset.
     * For TrueType fonts, the preview will show the actual character width.
     */
    UpdateFontPreview(&FontPreview, hFont, CharWidth, CharHeight);

    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    pConInfo->FontSize.X = (SHORT)(SizeList->UseRasterOrTTList ? CharWidth : 0);
    pConInfo->FontSize.Y = (SHORT)CharHeight;

    DPRINT("pConInfo->FontSize = (%d x %d) ; (CharWidth x CharHeight) = (%d x %d)\n",
           pConInfo->FontSize.X, pConInfo->FontSize.Y, CharWidth, CharHeight);

    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_FONT_WINDOW_PREVIEW), NULL, TRUE);
    InvalidateRect(GetDlgItem(hDlg, IDC_STATIC_SELECT_FONT_PREVIEW), NULL, TRUE);

    StringCchPrintfW(szFontSize, ARRAYSIZE(szFontSize), L"%d", CharWidth);
    SetDlgItemText(hDlg, IDC_FONT_SIZE_X, szFontSize);
    StringCchPrintfW(szFontSize, ARRAYSIZE(szFontSize), L"%d", CharHeight);
    SetDlgItemText(hDlg, IDC_FONT_SIZE_Y, szFontSize);

    return TRUE;
}


INT_PTR
CALLBACK
FontProc(HWND hDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    PFONTSIZE_LIST_CTL SizeList;

    SizeList = (PFONTSIZE_LIST_CTL)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            SizeList = (PFONTSIZE_LIST_CTL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*SizeList));
            if (!SizeList)
            {
                EndDialog(hDlg, 0);
                return (INT_PTR)TRUE;
            }
            SizeList->RasterSizeList.hWndList = GetDlgItem(hDlg, IDC_LBOX_FONTSIZE);
            SizeList->RasterSizeList.GetCount = RasterSizeList_GetCount;
            SizeList->RasterSizeList.GetData  = RasterSizeList_GetData;
            SizeList->hWndTTSizeList = GetDlgItem(hDlg, IDC_CBOX_FONTSIZE);
            SizeList->bIsTTSizeDirty = FALSE;
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)SizeList);

            /* By default show the raster font size list */
            SizeList->UseRasterOrTTList = TRUE;

            /* By default show the font sizes in pixel units */
            CheckRadioButton(hDlg, IDC_RADIO_PIXEL_UNIT, IDC_RADIO_POINT_UNIT, IDC_RADIO_PIXEL_UNIT);
            SizeList->TTSizePixelUnit = TRUE;

            UpdateFontSizeList(hDlg, SizeList);

            DPRINT1("ConInfo->FaceName = '%S'\n", ConInfo->FaceName);

            /* Face names list and current font selection will be done during PSN_SETACTIVE notification */
            // CurrentCodePage = INVALID_CP;

            return TRUE;
        }

        case WM_DESTROY:
        {
            if (SizeList)
                HeapFree(GetProcessHeap(), 0, SizeList);
            return (INT_PTR)TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (drawItem->CtlID == IDC_STATIC_SELECT_FONT_PREVIEW)
                PaintText(drawItem, ConInfo, Screen);

            return TRUE;
        }

        case WM_DISPLAYCHANGE:
        {
            /* Retransmit to the preview window */
            SendDlgItemMessageW(hDlg, IDC_STATIC_FONT_WINDOW_PREVIEW,
                                WM_DISPLAYCHANGE, wParam, lParam);
            break;
        }

        case WM_FONTCHANGE:
        {
            /* The pool of font resources has changed, re-enumerate the fonts */
            HWND hFontList = GetDlgItem(hDlg, IDC_LBOX_FONTTYPE);

            /* Initialize the font list */
            FaceNameList_Initialize(hFontList, ConInfo->CodePage);

            /* Select the current font */
            FaceNameList_SelectFont(hDlg, hFontList,
                                    SizeList,
                                    ConInfo->FaceName,
                                    ConInfo->FontFamily,
                                    ConInfo->FontWeight,
                                    ConInfo->FontSize);

            /* Refresh everything */
            FontTypeChange(hDlg, SizeList, ConInfo);
            break;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                {
                    ApplyConsoleInfo(hDlg);
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    /* Check whether the current code page has changed.
                     * If so, re-enumerate the fonts. */
                    if (CurrentCodePage != ConInfo->CodePage)
                    {
                        HWND hFontList;

                        /* Save the new code page */
                        CurrentCodePage = ConInfo->CodePage;

                        hFontList = GetDlgItem(hDlg, IDC_LBOX_FONTTYPE);

                        /* Initialize the font list */
                        FaceNameList_Initialize(hFontList, ConInfo->CodePage);

                        /* Select the current font */
                        FaceNameList_SelectFont(hDlg, hFontList,
                                                SizeList,
                                                ConInfo->FaceName,
                                                ConInfo->FontFamily,
                                                ConInfo->FontWeight,
                                                ConInfo->FontSize);

                        /* Refresh everything */
                        FontTypeChange(hDlg, SizeList, ConInfo);
                    }

                    /* Fall back to default behaviour */
                    break;
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            if (HIWORD(wParam) == LBN_SELCHANGE /* || CBN_SELCHANGE */)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_LBOX_FONTTYPE:
                    {
                        /* Change the property sheet state only if the font has really changed */
                        if (FontTypeChange(hDlg, SizeList, ConInfo))
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                        break;
                    }

                    case IDC_LBOX_FONTSIZE:
                    case IDC_CBOX_FONTSIZE:
                    {
                        /* Change the property sheet state only if the font has really changed */
                        if (FontSizeChange(hDlg, SizeList, ConInfo))
                            PropSheet_Changed(GetParent(hDlg), hDlg);
                        break;
                    }
                }
            }
            /* NOTE: CBN_EDITUPDATE is sent first, and is followed by CBN_EDITCHANGE */
            else if (HIWORD(wParam) == CBN_EDITUPDATE && LOWORD(wParam) == IDC_CBOX_FONTSIZE)
            {
                ULONG FontSize;
                PWCHAR pszNext = NULL;
                WCHAR szFontSize[100];
                WCHAR szMessage[260];

                /* Read the ComboBox edit string, as the user has entered a custom size */
                GetWindowTextW(SizeList->hWndTTSizeList, szFontSize, ARRAYSIZE(szFontSize));

                /* Validate the font size */
                FontSize = wcstoul(szFontSize, &pszNext, 10);
                if ((FontSize == 0) || (*pszNext))
                {
                    // FIXME: Localize!
                    StringCchPrintfW(szMessage, ARRAYSIZE(szMessage), L"\"%s\" is not a valid font size.", szFontSize);
                    MessageBoxW(hDlg, szMessage, L"Error", MB_ICONINFORMATION | MB_OK);
                }
                /**/SizeList->bIsTTSizeDirty = TRUE;/**/
            }
            else if (HIWORD(wParam) == CBN_KILLFOCUS && LOWORD(wParam) == IDC_CBOX_FONTSIZE)
            {
                /* Change the property sheet state only if the font has really changed */
                if (FontSizeChange(hDlg, SizeList, ConInfo))
                    PropSheet_Changed(GetParent(hDlg), hDlg);
            }
            else
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                case IDC_CHECK_BOLD_FONTS:
                {
                    if (IsDlgButtonChecked(hDlg, IDC_CHECK_BOLD_FONTS) == BST_CHECKED)
                        ConInfo->FontWeight = FW_BOLD;
                    else
                        ConInfo->FontWeight = FW_NORMAL;

                    FontTypeChange(hDlg, SizeList, ConInfo);
                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case IDC_RADIO_PIXEL_UNIT:
                case IDC_RADIO_POINT_UNIT:
                {
                    SizeList->TTSizePixelUnit = (LOWORD(wParam) == IDC_RADIO_PIXEL_UNIT);

                    /* The call is valid only for TrueType fonts */
                    if (CurrentFontType != TRUETYPE_FONTTYPE)
                        break;

                    /* Change the property sheet state only if the font has really changed */
                    if (FontSizeChange(hDlg, SizeList, ConInfo))
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }
                }
            }

            break;
        }

        default:
            break;
    }

    return FALSE;
}
