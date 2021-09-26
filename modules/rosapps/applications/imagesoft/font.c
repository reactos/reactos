#include <precomp.h>

int CALLBACK
EnumFontSizes(ENUMLOGFONTEX *lpelfe,
              NEWTEXTMETRICEX *lpntme,
              DWORD FontType,
              LPARAM lParam)
{
    static int ttsizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
    TCHAR ach[100];

    BOOL fTrueType = (lpelfe->elfLogFont.lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;

    HWND hwndCombo = (HWND)lParam;
    INT  i, idx;

    if (fTrueType)
    {
        for (i = 0; i < ARRAYSIZE(ttsizes); i++)
        {
            wsprintf(ach, _T("%d"), ttsizes[i]);

            idx = (INT)SendMessage(hwndCombo,
                                   CB_ADDSTRING,
                                   0,
                                   (LPARAM)ach);

            SendMessage(hwndCombo,
                        CB_SETITEMDATA,
                        idx,
                        ttsizes[i]);
        }

        return 0;
    }

    return 1;
}


/* Font-enumeration callback */
int CALLBACK
EnumFontNames(ENUMLOGFONTEX *lpelfe,
              NEWTEXTMETRICEX *lpntme,
              DWORD FontType,
              LPARAM lParam)
{
    HWND hwndCombo = (HWND)lParam;
    TCHAR *pszName  = lpelfe->elfLogFont.lfFaceName;

    /* make sure font doesn't already exist in our list */
    if(SendMessage(hwndCombo,
                   CB_FINDSTRINGEXACT,
                   0,
                   (LPARAM)pszName) == CB_ERR)
    {
        INT idx;
        BOOL fFixed;
        BOOL fTrueType;

        /* add the font */
        idx = (INT)SendMessage(hwndCombo,
                               CB_ADDSTRING,
                               0,
                               (LPARAM)pszName);

        /* record the font's attributes (Fixedwidth and Truetype) */
        fFixed = (lpelfe->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ? TRUE : FALSE;
        fTrueType = (lpelfe->elfLogFont.lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;

        /* store this information in the list-item's userdata area */
        SendMessage(hwndCombo,
                    CB_SETITEMDATA,
                    idx,
                    MAKEWPARAM(fFixed, fTrueType));
    }

    return 1;
}


VOID
FillFontSizeComboList(HWND hwndCombo)
{
    LOGFONT lf = { 0 };
    HDC hdc = GetDC(hwndCombo);

    /* default size */
    INT cursize = 12;
    INT i, count, nearest = 0;

    HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

    SendMessage(hwndCombo,
                WM_SETFONT,
                (WPARAM)hFont,
                0);

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;

    /* empty the list */
    SendMessage(hwndCombo,
                CB_RESETCONTENT,
                0,
                0);

    /* enumerate font sizes */
    EnumFontFamiliesEx(hdc,
                       &lf,
                       (FONTENUMPROC)EnumFontSizes,
                       (LPARAM)hwndCombo,
                       0);

    /* set selection to first item */
    count = (INT)SendMessage(hwndCombo,
                             CB_GETCOUNT,
                             0,
                             0);

    for(i = 0; i < count; i++)
    {
        INT n = (INT)SendMessage(hwndCombo,
                                 CB_GETITEMDATA,
                                 i,
                                 0);

        if (n <= cursize)
            nearest = i;
    }

    SendMessage(hwndCombo,
                CB_SETCURSEL,
                nearest,
                0);

    ReleaseDC(hwndCombo,
              hdc);
}


/* Initialize the font-list by enumeration all system fonts */
VOID
FillFontStyleComboList(HWND hwndCombo)
{
    HDC hdc = GetDC(hwndCombo);
    LOGFONT lf;

    /* FIXME: draw each font in its own style */
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwndCombo,
                WM_SETFONT,
                (WPARAM)hFont,
                0);

    /* FIXME: set this in relation to the widest string */
    SendMessage(hwndCombo, CB_SETDROPPEDWIDTH, 150, 0);

    lf.lfCharSet = ANSI_CHARSET;   // DEFAULT_CHARSET;
    lf.lfFaceName[0] = _T('\0');   // all fonts
    lf.lfPitchAndFamily = 0;

    /* store the list of fonts in the combo */
    EnumFontFamiliesEx(hdc,
                       &lf,
                       (FONTENUMPROC)EnumFontNames,
                       (LPARAM)hwndCombo, 0);

    ReleaseDC(hwndCombo,
              hdc);

    /* set default to Arial */
    SendMessage(hwndCombo,
                CB_SELECTSTRING,
                -1,
                (LPARAM)_T("Arial"));


}
