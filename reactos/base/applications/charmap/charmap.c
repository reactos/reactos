#include <precomp.h>

#define ID_ABOUT    0x1

HINSTANCE hInstance;

/* Font-enumeration callback */
static int CALLBACK
EnumFontNames(ENUMLOGFONTEX *lpelfe,
              NEWTEXTMETRICEX *lpntme,
              DWORD FontType,
              LPARAM lParam)
{
    HWND hwndCombo = (HWND)lParam;
    TCHAR *pszName  = lpelfe->elfLogFont.lfFaceName;

    /* make sure font doesn't already exist in our list */
    if(SendMessage(hwndCombo,
                   CB_FINDSTRING,
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


/* Initialize the font-list by enumeration all system fonts */
static VOID
FillFontStyleComboList(HWND hwndCombo)
{
    HDC hdc;
    LOGFONT lf;

    /* FIXME: for fun, draw each font in its own style */
    HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hwndCombo,
                WM_SETFONT,
                (WPARAM)hFont,
                0);

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = _T('\0');   // all fonts
    lf.lfPitchAndFamily = 0;

    hdc = GetDC(hwndCombo);

    /* store the list of fonts in the combo */
    EnumFontFamiliesEx(hdc,
                       &lf,
                       (FONTENUMPROC)EnumFontNames,
                       (LPARAM)hwndCombo,
                       0);

    ReleaseDC(hwndCombo,
              hdc);

    /* set default to Arial */
    SendMessage(hwndCombo,
                CB_SELECTSTRING,
                -1,
                (LPARAM)_T("Arial"));
}


static VOID
ChangeMapFont(HWND hDlg)
{
    HWND hCombo;
    HWND hMap;
    LPTSTR lpFontName;
    INT Len;

    hCombo = GetDlgItem(hDlg, IDC_FONTCOMBO);

    Len = GetWindowTextLength(hCombo);

    if (Len != 0)
    {
        lpFontName = HeapAlloc(GetProcessHeap(),
                               0,
                               (Len + 1) * sizeof(TCHAR));

        if (lpFontName)
        {
            SendMessage(hCombo,
                        WM_GETTEXT,
                        31,
                        (LPARAM)lpFontName);

            hMap = GetDlgItem(hDlg, IDC_FONTMAP);

            SendMessage(hMap,
                        FM_SETFONT,
                        0,
                        (LPARAM)lpFontName);
        }
    }
}


static BOOL CALLBACK
DlgProc(HWND hDlg,
        UINT Message,
        WPARAM wParam,
        LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            HICON hSmIcon;
            HICON hBgIcon;
            HMENU hSysMenu;

            hSmIcon = LoadImage(hInstance,
                                MAKEINTRESOURCE(IDI_ICON),
                                IMAGE_ICON,
                                16,
                                16,
                                0);
            hBgIcon = LoadImage(hInstance,
                                MAKEINTRESOURCE(IDI_ICON),
                                IMAGE_ICON,
                                32,
                                32,
                                0);

            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hSmIcon);
            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_BIG,
                        (LPARAM)hBgIcon);

            FillFontStyleComboList(GetDlgItem(hDlg,
                                              IDC_FONTCOMBO));

            ChangeMapFont(hDlg);

            hSysMenu = GetSystemMenu(hDlg,
                                     FALSE);
            if (hSysMenu != NULL)
            {
                LPCTSTR lpAboutText = NULL;

                if (LoadString(hInstance,
                               IDS_ABOUT,
                               (LPTSTR)&lpAboutText,
                               0))
                {
                    AppendMenu(hSysMenu,
                               MF_SEPARATOR,
                               0,
                               NULL);
                    AppendMenu(hSysMenu,
                               MF_STRING,
                               ID_ABOUT,
                               lpAboutText);
                }
            }
            return TRUE;
        }
        break;

        case WM_CLOSE:
        {
            EndDialog(hDlg, 0);
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_FONTCOMBO:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeMapFont(hDlg);
                    }
                }
                break;

                case IDOK:
                    EndDialog(hDlg, 0);
                break;
            }
        }
        break;

        case WM_SYSCOMMAND:
        {
            switch(wParam)
            {
                case ID_ABOUT:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_ABOUTBOX),
                              hDlg,
                              AboutDialogProc);
                break;
            }
        }
        break;

        default:
            return FALSE;
    }

    return FALSE;
}


INT WINAPI
WinMain(HINSTANCE hInst,
        HINSTANCE hPrev,
        LPSTR Cmd,
        int iCmd)
{
    INITCOMMONCONTROLSEX iccx;
    INT Ret = 1;

    hInstance = hInst;

    iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&iccx);

    if (RegisterMapClasses(hInstance))
    {
        Ret = DialogBox(hInstance,
                        MAKEINTRESOURCE(IDD_CHARMAP),
                        NULL,
                        (DLGPROC)DlgProc) >= 0;

        UnregisterMapClasses(hInstance);
    }

    return Ret;
}
