/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/effappdlg.c
 * PURPOSE:         Effects appearance dialog
 *
 * PROGRAMMER:      Jan Roeloffzen (jroeloffzen[at]hotmail[dot]com)
 *
 */

#include "desk.h"

/* Update all the controls with the current values for the selected screen element */
static VOID
UpdateControls(HWND hwndDlg, GLOBALS *g)
{
    WPARAM state;
    state = SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_ANIMATION, BM_GETCHECK, 0, 0);
    g->SchemeAdv.Effects.bMenuAnimation = (state == BST_CHECKED) ? TRUE : FALSE;
    EnableWindow(GetDlgItem(hwndDlg, IDC_EFFAPPEARANCE_ANIMATIONTYPE), g->SchemeAdv.Effects.bMenuAnimation);

    // A boolean as an index for a 2-value list:
    SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_ANIMATIONTYPE, CB_SETCURSEL, (WPARAM)g->SchemeAdv.Effects.bMenuFade, 0);

    state = SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_KEYBOARDCUES, BM_GETCHECK, 0, 0);
    g->SchemeAdv.Effects.bKeyboardCues = (state == BST_CHECKED) ? TRUE : FALSE;
    state = SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_DRAGFULLWINDOWS, BM_GETCHECK, 0, 0);
    g->SchemeAdv.Effects.bDragFullWindows = (state == BST_CHECKED) ? TRUE : FALSE;
    g->bSchemeChanged = TRUE;
}


static VOID
SaveCurrentValues(HWND hwndDlg, GLOBALS *g)
{
}

static VOID
AddToCombo(HWND hwndDlg, INT From, INT To, INT Combo)
{
    INT iElement, iListIndex, i=0;
    TCHAR tstrText[80];

    for (iElement = From; iElement<=To; iElement++)
    {
        LoadString(hApplet, iElement, (LPTSTR)tstrText, 80);
        iListIndex = SendDlgItemMessage(hwndDlg, Combo, CB_ADDSTRING, 0, (LPARAM)tstrText);
        SendDlgItemMessage(hwndDlg, Combo, CB_SETITEMDATA, (WPARAM)iListIndex, (LPARAM)i++ );
    }
}

/* Initialize the effects appearance dialog */
static VOID
EffAppearanceDlg_Init(HWND hwndDlg, GLOBALS *g)
{
    WPARAM state;

    /* Copy the current theme values */
    g->SchemeAdv = g->Scheme;

    AddToCombo(hwndDlg, IDS_SLIDEEFFECT, IDS_FADEEFFECT, IDC_EFFAPPEARANCE_ANIMATIONTYPE);

    state = g->SchemeAdv.Effects.bMenuAnimation ? BST_CHECKED : BST_UNCHECKED;
    SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_ANIMATION, BM_SETCHECK, state, 0);

    state = g->SchemeAdv.Effects.bKeyboardCues ? BST_CHECKED : BST_UNCHECKED;
    SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_KEYBOARDCUES, BM_SETCHECK, state, 0);

    state = g->SchemeAdv.Effects.bDragFullWindows ? BST_CHECKED : BST_UNCHECKED;
    SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_DRAGFULLWINDOWS, BM_SETCHECK, state, 0);

    /* Update the controls */
    UpdateControls(hwndDlg, g);
}


static VOID
EffAppearanceDlg_CleanUp(HWND hwndDlg, GLOBALS* g)
{
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
            EffAppearanceDlg_CleanUp(hwndDlg, g);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    SaveCurrentValues(hwndDlg, g);
                    EndDialog(hwndDlg, IDOK);
                    break;

                case IDCANCEL:
                    g->SchemeAdv = g->Scheme;
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDC_EFFAPPEARANCE_ANIMATION:
                case IDC_EFFAPPEARANCE_KEYBOARDCUES:
                case IDC_EFFAPPEARANCE_DRAGFULLWINDOWS:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UpdateControls(hwndDlg, g);
                    }
                    break;

                case IDC_EFFAPPEARANCE_ANIMATIONTYPE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SaveCurrentValues(hwndDlg, g);
                        g->SchemeAdv.Effects.bMenuFade = SendDlgItemMessage(hwndDlg, IDC_EFFAPPEARANCE_ANIMATIONTYPE, CB_GETCURSEL, 0, 0);
                        UpdateControls(hwndDlg, g);
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
