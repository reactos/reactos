/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/console/font.c
 * PURPOSE:         Font dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "console.h"

#define NDEBUG
#include <debug.h>


//
// Some temporary code for future reference...
//
#if 0
/*
 * This code comes from PuTTY
 */
{
    CHOOSEFONT cf;
    LOGFONT lf;
    HDC hdc;
    FontSpec *fs = (FontSpec *)c->data;

    hdc = GetDC(0);
    lf.lfHeight = -MulDiv(fs->height,
              GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(0, hdc);
    lf.lfWidth = lf.lfEscapement = lf.lfOrientation = 0;
    lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = 0;
    lf.lfWeight = (fs->isbold ? FW_BOLD : 0);
    lf.lfCharSet = fs->charset;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    strncpy(lf.lfFaceName, fs->name,
        sizeof(lf.lfFaceName) - 1);
    lf.lfFaceName[sizeof(lf.lfFaceName) - 1] = '\0';

    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = dp->hwnd;
    cf.lpLogFont = &lf;
    cf.Flags = (dp->fixed_pitch_fonts ? CF_FIXEDPITCHONLY : 0) |
            CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

    if (ChooseFont(&cf)) {
            fs = fontspec_new(lf.lfFaceName, (lf.lfWeight == FW_BOLD),
                              cf.iPointSize / 10, lf.lfCharSet);
    dlg_fontsel_set(ctrl, dp, fs);
            fontspec_free(fs);

    ctrl->generic.handler(ctrl, dp, dp->data, EVENT_VALCHANGE);
    }
}

/*
 * This code is from consrv.
 */
{
    if (!GetTextMetricsW(drawItem->hDC, &Metrics))
    {
        DPRINT1("PaintText: GetTextMetrics failed\n");
        SelectObject(drawItem->hDC, OldFont);
        DeleteObject(Font);
        return;
    }
    GuiData->CharWidth  = Metrics.tmMaxCharWidth;
    GuiData->CharHeight = Metrics.tmHeight + Metrics.tmExternalLeading;

    /* Measure real char width more precisely if possible */
    if (GetTextExtentPoint32W(drawItem->hDC, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;
}

/*
 * See also: Display_SetTypeFace in applications/fontview/display.c
 */
#endif


/*
 * Font pixel heights for TrueType fonts
 */
static SHORT TrueTypePoints[] =
{
    // 8, 9, 10, 11, 12, 14, 16, 18, 20,
    // 22, 24, 26, 28, 36, 48, 72
    5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 24, 28, 36, 72
};

static BOOL CALLBACK
EnumFontNamesProc(PLOGFONTW lplf,
                  PNEWTEXTMETRICW lpntm,
                  DWORD  FontType,
                  LPARAM lParam)
{
    HWND hwndCombo = (HWND)lParam;
    LPWSTR pszName = lplf->lfFaceName;

    /* Record the font's attributes (Fixedwidth and Truetype) */
    // BOOL fFixed    = ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH);
    // BOOL fTrueType = (lplf->lfOutPrecision == OUT_STROKE_PRECIS);

    /*
     * According to: http://support.microsoft.com/kb/247815
     * the criteria for console-eligible fonts are:
     * - The font must be a fixed-pitch font.
     * - The font cannot be an italic font.
     * - The font cannot have a negative A or C space.
     * - If it is a TrueType font, it must be FF_MODERN.
     * - If it is not a TrueType font, it must be OEM_CHARSET.
     *
     * Non documented: vertical fonts are forbidden (their name start with a '@').
     *
     * Additional criteria for Asian installations:
     * - If it is not a TrueType font, the face name must be "Terminal".
     * - If it is an Asian TrueType font, it must also be an Asian character set.
     *
     * To install additional TrueType fonts to be available for the console,
     * add entries of type REG_SZ named "0", "00" etc... in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
     * The names of the fonts listed there should match those in:
     * HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Fonts
     */

     /*
      * In ReactOS, we relax some criteria:
      * - We allow fixed-pitch FF_MODERN (Monospace) TrueType fonts
      *   that can be italic and have negative A or C space.
      * - If it is not a TrueType font, it can be from another character set
      *   than OEM_CHARSET.
      * - We do not support Asian criteria at the moment.
      * - We do not look into the magic registry key mentioned above.
      */

    /* Reject variable width fonts */
    if (((lplf->lfPitchAndFamily & 0x03) != FIXED_PITCH)
#if 0 /* Reject italic and TrueType fonts with negative A or C space */
        || (lplf->lfItalic)
        || !(lpntm->ntmFlags & NTM_NONNEGATIVE_AC)
#endif
        )
    {
        DPRINT1("Font '%S' rejected because it%s (lfPitchAndFamily = %d).\n",
                pszName, !(lplf->lfPitchAndFamily & FIXED_PITCH) ? "'s not FIXED_PITCH" : (!(lpntm->ntmFlags & NTM_NONNEGATIVE_AC) ? " has negative A or C space" : " is broken"),
                lplf->lfPitchAndFamily);
        return TRUE;
    }

    /* Reject TrueType fonts that are not FF_MODERN */
    if ((FontType == TRUETYPE_FONTTYPE) && ((lplf->lfPitchAndFamily & 0xF0) != FF_MODERN))
    {
        DPRINT1("TrueType font '%S' rejected because it's not FF_MODERN (lfPitchAndFamily = %d)\n",
                pszName, lplf->lfPitchAndFamily);
        return TRUE;
    }

    /* Reject non-TrueType fonts that are not OEM */
#if 0
    if ((FontType != TRUETYPE_FONTTYPE) && (lplf->lfCharSet != OEM_CHARSET))
    {
        DPRINT1("Non-TrueType font '%S' rejected because it's not OEM_CHARSET %d\n",
                pszName, lplf->lfCharSet);
        return TRUE;
    }
#else // Improved criterium
    if ((FontType != TRUETYPE_FONTTYPE) &&
        ((lplf->lfCharSet != ANSI_CHARSET) && (lplf->lfCharSet != DEFAULT_CHARSET) && (lplf->lfCharSet != OEM_CHARSET)))
    {
        DPRINT1("Non-TrueType font '%S' rejected because it's not ANSI_CHARSET or DEFAULT_CHARSET or OEM_CHARSET (lfCharSet = %d)\n",
                pszName, lplf->lfCharSet);
        return TRUE;
    }
#endif

    /* Reject fonts that are vertical (tategaki) */
    if (pszName[0] == L'@')
    {
        DPRINT1("Font '%S' rejected because it's vertical\n", pszName);
        return TRUE;
    }

#if 0 // For Asian installations only
    /* Reject non-TrueType fonts that are not Terminal */
    if ((FontType != TRUETYPE_FONTTYPE) && (wcscmp(pszName, L"Terminal") != 0))
    {
        DPRINT1("Non-TrueType font '%S' rejected because it's not Terminal\n", pszName);
        return TRUE;
    }

    // TODO: Asian TrueType font must also be an Asian character set.
#endif

    /* Make sure the font doesn't already exist in the list */
    if (SendMessageW(hwndCombo, LB_FINDSTRINGEXACT, 0, (LPARAM)pszName) == LB_ERR)
    {
        /* Add the font */
        INT idx = (INT)SendMessageW(hwndCombo, LB_ADDSTRING, 0, (LPARAM)pszName);

        DPRINT1("Add font '%S' (lfPitchAndFamily = %d)\n", pszName, lplf->lfPitchAndFamily);

        /* Store this information in the list-item's userdata area */
        // SendMessageW(hwndCombo, LB_SETITEMDATA, idx, MAKEWPARAM(fFixed, fTrueType));
        SendMessageW(hwndCombo, LB_SETITEMDATA, idx, (WPARAM)FontType);
    }

    return TRUE;
}

static BOOL CALLBACK
EnumFontSizesProc(PLOGFONTW lplf,
                  PNEWTEXTMETRICW lpntm,
                  DWORD  FontType,
                  LPARAM lParam)
{
    HWND hwndCombo = (HWND)lParam;
    WCHAR FontSize[100];

    if (FontType != TRUETYPE_FONTTYPE)
    {
        // int logsize     = lpntm->tmHeight - lpntm->tmInternalLeading;
        // LONG pointsize  = MulDiv(logsize, 72, GetDeviceCaps(hdc, LOGPIXELSY));

        // swprintf(FontSize, L"%2d (%d x %d)", pointsize, lplf->lfWidth, lplf->lfHeight);
        swprintf(FontSize, L"%d x %d", lplf->lfWidth, lplf->lfHeight);

        /* Make sure the size doesn't already exist in the list */
        if (SendMessageW(hwndCombo, LB_FINDSTRINGEXACT, 0, (LPARAM)FontSize) == LB_ERR)
        {
            /* Add the size */
            INT idx = (INT)SendMessageW(hwndCombo, LB_ADDSTRING, 0, (LPARAM)FontSize);

            /*
             * Store this information in the list-item's userdata area.
             * Format:
             * Width  = FontSize.X = LOWORD(FontSize);
             * Height = FontSize.Y = HIWORD(FontSize);
             */
            SendMessageW(hwndCombo, LB_SETITEMDATA, idx, MAKEWPARAM(lplf->lfWidth, lplf->lfHeight));
        }

        return TRUE;
    }
    else
    {
        ULONG i;
        for (i = 0; i < ARRAYSIZE(TrueTypePoints); ++i)
        {
            swprintf(FontSize, L"%2d", TrueTypePoints[i]);

            /* Make sure the size doesn't already exist in the list */
            if (SendMessageW(hwndCombo, LB_FINDSTRINGEXACT, 0, (LPARAM)FontSize) == LB_ERR)
            {
                /* Add the size */
                INT idx = (INT)SendMessageW(hwndCombo, LB_ADDSTRING, 0, (LPARAM)FontSize);

                /*
                 * Store this information in the list-item's userdata area.
                 * Format:
                 * Width  = FontSize.X = LOWORD(FontSize);
                 * Height = FontSize.Y = HIWORD(FontSize);
                 */
                SendMessageW(hwndCombo, LB_SETITEMDATA, idx, MAKEWPARAM(0, TrueTypePoints[i]));
            }
        }

        return FALSE;
    }
}


static VOID
FontSizeChange(HWND hwndDlg,
               PCONSOLE_STATE_INFO pConInfo);

static VOID
FontTypeChange(HWND hwndDlg,
               PCONSOLE_STATE_INFO pConInfo)
{
    INT Length, nSel;
    LPWSTR FaceName;

    HDC hDC;
    LOGFONTW lf;

    nSel = (INT)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTTYPE,
                                    LB_GETCURSEL, 0, 0);
    if (nSel == LB_ERR) return;

    Length = (INT)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTTYPE,
                                      LB_GETTEXTLEN, nSel, 0);
    if (Length == LB_ERR) return;

    FaceName = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         (Length + 1) * sizeof(WCHAR));
    if (FaceName == NULL) return;

    Length = (INT)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTTYPE,
                                      LB_GETTEXT, nSel, (LPARAM)FaceName);
    FaceName[Length] = '\0';

    Length = min(Length/*wcslen(FaceName) + 1*/, LF_FACESIZE); // wcsnlen
    wcsncpy(pConInfo->FaceName, FaceName, LF_FACESIZE);
    pConInfo->FaceName[Length] = L'\0';
    DPRINT1("pConInfo->FaceName = '%S'\n", pConInfo->FaceName);

    /* Enumerate the available sizes for the selected font */
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet  = DEFAULT_CHARSET; // OEM_CHARSET;
    // lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    wcsncpy(lf.lfFaceName, FaceName, LF_FACESIZE);
    lf.lfFaceName[Length] = L'\0';

    hDC = GetDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFontSizesProc,
                        (LPARAM)GetDlgItem(hwndDlg, IDC_LBOX_FONTSIZE), 0);
    ReleaseDC(NULL, hDC);

    HeapFree(GetProcessHeap(), 0, FaceName);

    // TODO: Select a default font size????
    FontSizeChange(hwndDlg, pConInfo);

    // InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_FONT_WINDOW_PREVIEW), NULL, TRUE);
    // InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SELECT_FONT_PREVIEW), NULL, TRUE);
}

static VOID
FontSizeChange(HWND hwndDlg,
               PCONSOLE_STATE_INFO pConInfo)
{
    INT nSel;
    ULONG FontSize;
    WCHAR FontSizeStr[20];

    nSel = (INT)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTSIZE,
                                    LB_GETCURSEL, 0, 0);
    if (nSel == LB_ERR) return;

    /*
     * Format:
     * Width  = FontSize.X = LOWORD(FontSize);
     * Height = FontSize.Y = HIWORD(FontSize);
     */
    FontSize = (ULONG)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTSIZE,
                                          LB_GETITEMDATA, nSel, 0);
    if (FontSize == LB_ERR) return;

    pConInfo->FontSize.X = LOWORD(FontSize);
    pConInfo->FontSize.Y = HIWORD(FontSize);
    DPRINT1("pConInfo->FontSize = (%d x %d)\n", pConInfo->FontSize.X, pConInfo->FontSize.Y);

    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_FONT_WINDOW_PREVIEW), NULL, TRUE);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SELECT_FONT_PREVIEW), NULL, TRUE);

    swprintf(FontSizeStr, L"%2d", pConInfo->FontSize.X);
    SetWindowText(GetDlgItem(hwndDlg, IDC_FONT_SIZE_X), FontSizeStr);
    swprintf(FontSizeStr, L"%2d", pConInfo->FontSize.Y);
    SetWindowText(GetDlgItem(hwndDlg, IDC_FONT_SIZE_Y), FontSizeStr);
}


INT_PTR
CALLBACK
FontProc(HWND hwndDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HDC  hDC;
            LOGFONTW lf;
            INT idx;

            ZeroMemory(&lf, sizeof(lf));
            lf.lfCharSet  = DEFAULT_CHARSET; // OEM_CHARSET;
            // lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

            hDC = GetDC(NULL);
            EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFontNamesProc,
                                (LPARAM)GetDlgItem(hwndDlg, IDC_LBOX_FONTTYPE), 0);
            ReleaseDC(NULL, hDC);

            DPRINT1("ConInfo->FaceName = '%S'\n", ConInfo->FaceName);
            idx = (INT)SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTTYPE,
                                           LB_FINDSTRINGEXACT, 0, (LPARAM)ConInfo->FaceName);
            if (idx != LB_ERR) SendDlgItemMessageW(hwndDlg, IDC_LBOX_FONTTYPE,
                                                   LB_SETCURSEL, (WPARAM)idx, 0);

            FontTypeChange(hwndDlg, ConInfo);

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (drawItem->CtlID == IDC_STATIC_FONT_WINDOW_PREVIEW)
                PaintConsole(drawItem, ConInfo);
            else if (drawItem->CtlID == IDC_STATIC_SELECT_FONT_PREVIEW)
                PaintText(drawItem, ConInfo, Screen);

            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                {
                    ApplyConsoleInfo(hwndDlg);
                    return TRUE;
                }
            }

            break;
        }

        case WM_COMMAND:
        {
            switch (HIWORD(wParam))
            {
                case LBN_SELCHANGE:
                {
                    switch (LOWORD(wParam))
                    {
                        case IDC_LBOX_FONTTYPE:
                        {
                            FontTypeChange(hwndDlg, ConInfo);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                        }

                        case IDC_LBOX_FONTSIZE:
                        {
                            FontSizeChange(hwndDlg, ConInfo);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            break;
                        }
                    }

                    break;
                }
            }

            break;
        }

        default:
            break;
    }

    return FALSE;
}
