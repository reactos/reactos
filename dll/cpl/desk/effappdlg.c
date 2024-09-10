/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * PURPOSE:         Effects appearance dialog
 * PROGRAMMERS:     Jan Roeloffzen <jroeloffzen@hotmail.com>
 *                  Ismael Ferreras Morezuelas <swyterzone+reactos@gmail.com>
 */

#include "desk.h"

/* Update all the controls with the current values for the selected screen element */
static VOID
EffAppearanceDlgUpdateControls(HWND hwndDlg, GLOBALS *g)
{
    WPARAM state;

#define SAVE_CHECKBOX(__CONTROL_ID, __MEMBER)                               \
do { \
    state = SendDlgItemMessageW(hwndDlg, __CONTROL_ID, BM_GETCHECK, 0, 0);  \
    g->SchemeAdv.Effects.__MEMBER = /* Do a XOR of both the conditions */   \
        ((state == BST_CHECKED) != (__CONTROL_ID == IDC_EFFAPPEARANCE_KEYBOARDCUES)); \
} while(0)

#define SAVE_CHECKBOX_SCH(__CONTROL_ID, __MEMBER)                           \
do { \
    state = SendDlgItemMessageW(hwndDlg, __CONTROL_ID, BM_GETCHECK, 0, 0);  \
    g->SchemeAdv.__MEMBER = (state == BST_CHECKED);                         \
} while(0)

#define RSET_COMBOBOX(__CONTROL_ID, __PARENT_MEMBER, __MEMBER)                                          \
do { \
    SendDlgItemMessageW(hwndDlg, __CONTROL_ID, CB_SETCURSEL, (WPARAM)g->SchemeAdv.Effects.__MEMBER, 0); \
    EnableWindow(GetDlgItem(hwndDlg, __CONTROL_ID), g->SchemeAdv.Effects.__PARENT_MEMBER);              \
} while(0)

    /* Animated menu transitions section (checkbox + combo) */
    SAVE_CHECKBOX(IDC_EFFAPPEARANCE_ANIMATION,       bMenuAnimation);
    RSET_COMBOBOX(IDC_EFFAPPEARANCE_ANIMATIONTYPE,   bMenuAnimation, bMenuFade);

    /* Font antialiasing section (checkbox + combo) */
    SAVE_CHECKBOX(IDC_EFFAPPEARANCE_SMOOTHING,       bFontSmoothing);
    RSET_COMBOBOX(IDC_EFFAPPEARANCE_SMOOTHINGTYPE,   bFontSmoothing, uiFontSmoothingType - 1);

    /* Other checkboxes */
    SAVE_CHECKBOX(IDC_EFFAPPEARANCE_SETDROPSHADOW,   bDropShadow);
    SAVE_CHECKBOX(IDC_EFFAPPEARANCE_DRAGFULLWINDOWS, bDragFullWindows);
    SAVE_CHECKBOX(IDC_EFFAPPEARANCE_KEYBOARDCUES,    bKeyboardCues);
    SAVE_CHECKBOX_SCH(IDC_EFFAPPEARANCE_FLATMENUS,   bFlatMenus);

#undef SAVE_CHECKBOX
#undef RSET_COMBOBOX

    g->bSchemeChanged = TRUE;
}

static VOID
EffAppearanceDlgSaveCurrentValues(HWND hwndDlg, GLOBALS *g)
{
    /* The settings get saved at the end of ApplyScheme() in theme.c,
     * when clicking Apply in the main dialog. */
}

static VOID
AddToCombobox(INT Combo, HWND hwndDlg, INT From, INT To)
{
    INT iElement;
    TCHAR tstrText[80];

    for (iElement = From; iElement <= To; iElement++)
    {
        LoadString(hApplet, iElement, (LPTSTR)tstrText, _countof(tstrText));
        SendDlgItemMessage(hwndDlg, Combo, CB_ADDSTRING, 0, (LPARAM)tstrText);
    }
}

/* Initialize the effects appearance dialog from the scheme populated in LoadCurrentScheme(), in theme.c */
static VOID
EffAppearanceDlg_Init(HWND hwndDlg, GLOBALS *g)
{
    WPARAM state;

    /* Copy the current theme values */
    g->SchemeAdv = g->Scheme;

#define INIT_CHECKBOX(__CONTROL_ID, __MEMBER)                           \
do { \
    state = /* Do a XOR of both the conditions */                       \
        ((g->SchemeAdv.Effects.__MEMBER) != (__CONTROL_ID == IDC_EFFAPPEARANCE_KEYBOARDCUES)) \
            ? BST_CHECKED : BST_UNCHECKED;                              \
    SendDlgItemMessageW(hwndDlg, __CONTROL_ID, BM_SETCHECK, state, 0);  \
} while(0)

#define INIT_CHECKBOX_SCH(__CONTROL_ID, __MEMBER)                       \
do { \
    state = /* Do a XOR of both the conditions */                       \
        ((g->SchemeAdv.__MEMBER) == TRUE)                               \
            ? BST_CHECKED : BST_UNCHECKED;                              \
    SendDlgItemMessageW(hwndDlg, __CONTROL_ID, BM_SETCHECK, state, 0);  \
} while(0)

#define FILL_COMBOBOX(__CONTROL_ID, __FIRST_STR, __LAST_STR) \
    AddToCombobox(__CONTROL_ID, hwndDlg, __FIRST_STR, __LAST_STR)

    /* Animated menu transitions section (checkbox + combo) */
    INIT_CHECKBOX(IDC_EFFAPPEARANCE_ANIMATION,       bMenuAnimation);
    FILL_COMBOBOX(IDC_EFFAPPEARANCE_ANIMATIONTYPE,   IDS_SLIDEEFFECT,
                                                     IDS_FADEEFFECT);

    /* Font antialiasing section (checkbox + combo) */
    INIT_CHECKBOX(IDC_EFFAPPEARANCE_SMOOTHING,       bFontSmoothing);
    FILL_COMBOBOX(IDC_EFFAPPEARANCE_SMOOTHINGTYPE,   IDS_STANDARDEFFECT,
                                                     IDS_CLEARTYPEEFFECT);

    /* Other checkboxes */
    INIT_CHECKBOX(IDC_EFFAPPEARANCE_SETDROPSHADOW,   bDropShadow);
    INIT_CHECKBOX(IDC_EFFAPPEARANCE_DRAGFULLWINDOWS, bDragFullWindows);
    INIT_CHECKBOX(IDC_EFFAPPEARANCE_KEYBOARDCUES,    bKeyboardCues);
    INIT_CHECKBOX_SCH(IDC_EFFAPPEARANCE_FLATMENUS,   bFlatMenus);

#undef INIT_CHECKBOX
#undef FILL_COMBOBOX

    /* Update the controls */
    EffAppearanceDlgUpdateControls(hwndDlg, g);
}

INT_PTR CALLBACK
EffAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    GLOBALS* g;

    g = (GLOBALS*)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            g = (GLOBALS*)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);
            EffAppearanceDlg_Init(hwndDlg, g);
            break;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EffAppearanceDlgSaveCurrentValues(hwndDlg, g);
                    EndDialog(hwndDlg, IDOK);
                    break;

                case IDCANCEL:
                    g->SchemeAdv = g->Scheme;
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDC_EFFAPPEARANCE_ANIMATION:
                case IDC_EFFAPPEARANCE_SMOOTHING:
                case IDC_EFFAPPEARANCE_SETDROPSHADOW:
                case IDC_EFFAPPEARANCE_DRAGFULLWINDOWS:
                case IDC_EFFAPPEARANCE_KEYBOARDCUES:
                case IDC_EFFAPPEARANCE_FLATMENUS:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EffAppearanceDlgUpdateControls(hwndDlg, g);
                    break;

                case IDC_EFFAPPEARANCE_ANIMATIONTYPE:
                case IDC_EFFAPPEARANCE_SMOOTHINGTYPE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT Index =
                            SendDlgItemMessageW(hwndDlg, IDC_EFFAPPEARANCE_SMOOTHINGTYPE,
                                                CB_GETCURSEL, 0, 0);

                        g->SchemeAdv.Effects.bMenuFade =
                            SendDlgItemMessageW(hwndDlg, IDC_EFFAPPEARANCE_ANIMATIONTYPE,
                                                CB_GETCURSEL, 0, 0);
                        g->SchemeAdv.Effects.uiFontSmoothingType = (Index == CB_ERR) ? 0 : (Index + 1);

                        EffAppearanceDlgUpdateControls(hwndDlg, g);
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
