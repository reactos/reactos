/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/font.c
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

    /* Measure real char width more precisely if possible. */
    if (GetTextExtentPoint32W(drawItem->hDC, L"R", 1, &CharSize))
        GuiData->CharWidth = CharSize.cx;
}
#endif


BOOL CALLBACK
EnumFontFamExProc(PLOGFONTW    lplf,
                  PNEWTEXTMETRICW lpntm,
                  DWORD  FontType,
                  LPARAM lParam)
{
    HWND hwndCombo = (HWND)lParam;
    LPWSTR pszName = lplf->lfFaceName;

    BOOL fFixed;
    BOOL fTrueType;

    /* Record the font's attributes (Fixedwidth and Truetype) */
    fFixed    = ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH);
    fTrueType = (lplf->lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;

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
        DPRINT1("TrueType font '%S' rejected because it's not FF_MODERN (lfPitchAndFamily = %d)\n", pszName, lplf->lfPitchAndFamily);
        return TRUE;
    }

    /* Reject non-TrueType fonts that are not OEM */
#if 0
    if ((FontType != TRUETYPE_FONTTYPE) && (lplf->lfCharSet != OEM_CHARSET))
    {
        DPRINT1("Non-TrueType font '%S' rejected because it's not OEM_CHARSET %d\n", pszName, lplf->lfCharSet);
        return TRUE;
    }
#else // Improved criterium
    if ((FontType != TRUETYPE_FONTTYPE) &&
        ((lplf->lfCharSet != ANSI_CHARSET) && (lplf->lfCharSet != DEFAULT_CHARSET) && (lplf->lfCharSet != OEM_CHARSET)))
    {
        DPRINT1("Non-TrueType font '%S' rejected because it's not ANSI_CHARSET or DEFAULT_CHARSET or OEM_CHARSET (lfCharSet = %d)\n", pszName, lplf->lfCharSet);
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
        SendMessageW(hwndCombo, LB_SETITEMDATA, idx, MAKEWPARAM(fFixed, fTrueType));
    }

    return TRUE;
}

INT_PTR
CALLBACK
FontProc(HWND hwndDlg,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam)
{
    PCONSOLE_PROPS pConInfo = (PCONSOLE_PROPS)GetWindowLongPtr(hwndDlg, DWLP_USER);
    PGUI_CONSOLE_INFO GuiInfo = (pConInfo ? pConInfo->TerminalInfo.TermInfo : NULL);

    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HDC  hDC;
            HWND hwndCombo;
            LOGFONTW lf;
            INT idx;

            pConInfo = (PCONSOLE_PROPS)((LPPROPSHEETPAGE)lParam)->lParam;
            GuiInfo  = pConInfo->TerminalInfo.TermInfo;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);

            ZeroMemory(&lf, sizeof(lf));
            lf.lfCharSet  = DEFAULT_CHARSET; // OEM_CHARSET;
            // lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
            // lf.lfFaceName = L"";

            hDC = GetDC(NULL);
            hwndCombo = GetDlgItem(hwndDlg, IDC_LBOX_FONTTYPE);
            EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFontFamExProc, (LPARAM)hwndCombo, 0);
            ReleaseDC(NULL, hDC);

            DPRINT1("GuiInfo->FaceName = '%S'\n", GuiInfo->FaceName);
            idx = (INT)SendMessageW(hwndCombo, LB_FINDSTRINGEXACT, 0, (LPARAM)GuiInfo->FaceName);
            if (idx != LB_ERR)
            {
                SendMessageW(hwndCombo, LB_SETCURSEL, (WPARAM)idx, 0);
            }

            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawItem = (LPDRAWITEMSTRUCT)lParam;

            if (drawItem->CtlID == IDC_STATIC_FONT_WINDOW_PREVIEW)
            {
                PaintConsole(drawItem, pConInfo);
            }
            else if (drawItem->CtlID == IDC_STATIC_SELECT_FONT_PREVIEW)
            {
                PaintText(drawItem, pConInfo, Screen);
            }
            return TRUE;
        }

        default:
            break;
    }

    return FALSE;
}
