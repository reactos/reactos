/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/appearance.c
 * PURPOSE:         Appearance property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Timo Kreuzer (timo[dot]kreuzer[at]web[dot]de)
 */

#include "desk.h"

/******************************************************************************/

static void
AppearancePage_UpdateThemePreview(HWND hwndDlg, GLOBALS *g)
{
    if (g->ActiveTheme.ThemeActive)
    {
        RECT rcWindow;

        GetClientRect(GetDlgItem(hwndDlg, IDC_APPEARANCE_PREVIEW), &rcWindow);
        if (DrawThemePreview(g->hdcThemePreview, &g->Scheme, &g->ActiveTheme, &rcWindow))
        {
            SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SET_HDC_PREVIEW, 0, (LPARAM)g->hdcThemePreview);
            return;
        }
    }

    SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
    SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_SET_HDC_PREVIEW, 0, 0);
}

static void
AppearancePage_LoadSelectedScheme(HWND hwndDlg, GLOBALS *g)
{
    if (g->ActiveTheme.ThemeActive == FALSE )
    {
        LoadSchemeFromReg(&g->Scheme, &g->ActiveTheme);
    }
    else
    {
        LoadSchemeFromTheme(&g->Scheme, &g->ActiveTheme);
    }

    g_GlobalData.desktop_color = g->Scheme.crColor[COLOR_DESKTOP];
}

static void
AppearancePage_ShowStyles(HWND hwndDlg, int nIDDlgItem, PTHEME_STYLE pStyles, PTHEME_STYLE pActiveStyle)
{
    int iListIndex;
    HWND hwndList = GetDlgItem(hwndDlg, nIDDlgItem);
    PTHEME_STYLE pCurrentStyle;

    SendMessage(hwndList, CB_RESETCONTENT , 0, 0);

    for (pCurrentStyle = pStyles;
         pCurrentStyle;
         pCurrentStyle = pCurrentStyle->NextStyle)
    {
        iListIndex = SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)pCurrentStyle->DisplayName);
        SendMessage(hwndList, CB_SETITEMDATA, iListIndex, (LPARAM)pCurrentStyle);
        if (pCurrentStyle == pActiveStyle)
        {
            SendMessage(hwndList, CB_SETCURSEL, (WPARAM)iListIndex, 0);
        }
    }
}

static void
AppearancePage_ShowColorSchemes(HWND hwndDlg, GLOBALS *g)
{
    AppearancePage_ShowStyles(hwndDlg,
                              IDC_APPEARANCE_COLORSCHEME,
                              g->ActiveTheme.Theme->ColoursList,
                              g->ActiveTheme.Color);
}

static void
AppearancePage_ShowSizes(HWND hwndDlg, GLOBALS *g)
{
    PTHEME_STYLE pSizes;

    if (g->ActiveTheme.Theme->SizesList)
        pSizes = g->ActiveTheme.Theme->SizesList;
    else
        pSizes = g->ActiveTheme.Color->ChildStyle;

    AppearancePage_ShowStyles(hwndDlg,
                              IDC_APPEARANCE_SIZE,
                              pSizes,
                              g->ActiveTheme.Size);
}

static INT_PTR
AppearancePage_OnInit(HWND hwndDlg)
{
    INT iListIndex;
    HWND hwndTheme;
    GLOBALS *g;
    RECT rcPreview;
    HDC hdcScreen;
    PTHEME pTheme;

    g = (GLOBALS*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBALS));
    if (g == NULL)
        return FALSE;

    SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);

    g->bInitializing = TRUE;

    if (!LoadCurrentScheme(&g->Scheme))
        return FALSE;

    g->pThemes = LoadThemes();
    if (g->pThemes)
    {
        BOOL bLoadedTheme = FALSE;

        if (g_GlobalData.pwszAction &&
            g_GlobalData.pwszFile &&
            wcscmp(g_GlobalData.pwszAction, L"OpenMSTheme") == 0)
        {
            bLoadedTheme = FindOrAppendTheme(g->pThemes,
                                             g_GlobalData.pwszFile,
                                             NULL,
                                             NULL,
                                             &g->ActiveTheme);
        }

        if (bLoadedTheme)
        {
            g->bThemeChanged = TRUE;
            g->bSchemeChanged = TRUE;

            PostMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

            AppearancePage_LoadSelectedScheme(hwndDlg, g);
        }
        else
        {
            if (!GetActiveTheme(g->pThemes, &g->ActiveTheme))
            {
                g->ActiveTheme.ThemeActive = FALSE;
            }
        }

        /*
         * Keep a copy of the selected classic theme in order to select this
         * when user selects the classic theme (and not a horrible random theme )
         */
        if (!GetActiveClassicTheme(g->pThemes, &g->ClassicTheme))
        {
            g->ClassicTheme.Theme = g->pThemes;
            g->ClassicTheme.Color = g->pThemes->ColoursList;
            g->ClassicTheme.Size = g->ClassicTheme.Color->ChildStyle;
        }

        if (g->ActiveTheme.ThemeActive == FALSE)
            g->ActiveTheme = g->ClassicTheme;

        GetClientRect(GetDlgItem(hwndDlg, IDC_APPEARANCE_PREVIEW), &rcPreview);

        hdcScreen = GetDC(NULL);
        g->hbmpThemePreview = CreateCompatibleBitmap(hdcScreen, rcPreview.right, rcPreview.bottom);
        g->hdcThemePreview = CreateCompatibleDC(hdcScreen);
        SelectObject(g->hdcThemePreview, g->hbmpThemePreview);
        ReleaseDC(NULL, hdcScreen);

        hwndTheme = GetDlgItem(hwndDlg, IDC_APPEARANCE_VISUAL_STYLE);

        for (pTheme = g->pThemes; pTheme; pTheme = pTheme->NextTheme)
        {
            iListIndex = SendMessage(hwndTheme, CB_ADDSTRING, 0, (LPARAM)pTheme->DisplayName);
            SendMessage(hwndTheme, CB_SETITEMDATA, iListIndex, (LPARAM)pTheme);
            if (pTheme == g->ActiveTheme.Theme)
            {
                SendMessage(hwndTheme, CB_SETCURSEL, (WPARAM)iListIndex, 0);
            }
        }

        if (g->ActiveTheme.Theme)
        {
            AppearancePage_ShowColorSchemes(hwndDlg, g);
            AppearancePage_ShowSizes(hwndDlg, g);
            AppearancePage_UpdateThemePreview(hwndDlg, g);
        }
    }
    g->bInitializing = FALSE;

    return FALSE;
}

static VOID
AppearancePage_OnDestroy(HWND hwndDlg, GLOBALS *g)
{
    HeapFree(GetProcessHeap(), 0, g);
}

static PVOID
GetSelectedData(HWND hwndDlg, int nIDDlgItem)
{
    HWND hwndCombo;
    INT sel;

    hwndCombo = GetDlgItem(hwndDlg, nIDDlgItem);
    sel = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR)
        return NULL;
    return (PVOID)SendMessage(hwndCombo, CB_GETITEMDATA, (WPARAM)sel, 0);
}

INT_PTR CALLBACK
AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    GLOBALS *g;
    LPNMHDR lpnm;

    g = (GLOBALS*)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return AppearancePage_OnInit(hwndDlg);

        case WM_DESTROY:
            AppearancePage_OnDestroy(hwndDlg, g);
            break;

        case WM_COMMAND:
            if (g == NULL || g->bInitializing)
                return FALSE;

            switch (LOWORD(wParam))
            {
                case IDC_APPEARANCE_EFFECTS:
                    if (DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_EFFAPPEARANCE),
                                       hwndDlg, EffAppearanceDlgProc, (LPARAM)g) == IDOK)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        g->Scheme = g->SchemeAdv;
                        g->bSchemeChanged = TRUE;
                    }
                    break;

                case IDC_APPEARANCE_ADVANCED:
                    if (DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_ADVAPPEARANCE),
                                       hwndDlg, AdvAppearanceDlgProc, (LPARAM)g) == IDOK)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        g->bSchemeChanged = TRUE;
                        g->Scheme = g->SchemeAdv;
                        g_GlobalData.desktop_color = g->Scheme.crColor[COLOR_DESKTOP];

                        AppearancePage_UpdateThemePreview(hwndDlg, g);
                    }
                    break;

                case IDC_APPEARANCE_COLORSCHEME:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        g->ActiveTheme.Color = (PTHEME_STYLE)GetSelectedData(hwndDlg, IDC_APPEARANCE_COLORSCHEME);
                        if (g->ActiveTheme.Color->ChildStyle != NULL)
                            g->ActiveTheme.Size = g->ActiveTheme.Color->ChildStyle;

                        g->bSchemeChanged = TRUE;
                        if (g->ActiveTheme.ThemeActive)
                            g->bThemeChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                        AppearancePage_LoadSelectedScheme(hwndDlg, g);
                        AppearancePage_ShowSizes(hwndDlg, g);
                        AppearancePage_UpdateThemePreview(hwndDlg, g);
                    }
                    break;

                case IDC_APPEARANCE_VISUAL_STYLE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PTHEME pTheme  = (PTHEME)GetSelectedData(hwndDlg, IDC_APPEARANCE_VISUAL_STYLE);

                        if (g->ClassicTheme.Theme == pTheme)
                            g->ActiveTheme = g->ClassicTheme;
                        else
                        {
                            g->ActiveTheme.Theme = pTheme;
                            g->ActiveTheme.Size = pTheme->SizesList;
                            g->ActiveTheme.Color = pTheme->ColoursList;
                            g->ActiveTheme.ThemeActive = TRUE;
                        }

                        g->bThemeChanged = TRUE;
                        g->bSchemeChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                        AppearancePage_ShowColorSchemes(hwndDlg, g);
                        AppearancePage_ShowSizes(hwndDlg, g);
                        AppearancePage_LoadSelectedScheme(hwndDlg, g);
                        AppearancePage_UpdateThemePreview(hwndDlg, g);
                    }
                    break;

                case IDC_APPEARANCE_SIZE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        g->ActiveTheme.Size = (PTHEME_STYLE)GetSelectedData(hwndDlg, IDC_APPEARANCE_SIZE);
                        g->bSchemeChanged = TRUE;
                        if (g->ActiveTheme.ThemeActive)
                            g->bThemeChanged = TRUE;

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                        AppearancePage_LoadSelectedScheme(hwndDlg, g);
                        AppearancePage_UpdateThemePreview(hwndDlg, g);
                    }
            }
            break;

        case WM_NOTIFY:
            lpnm = (LPNMHDR)lParam;
            switch (lpnm->code)
            {
                case PSN_APPLY:

                    if (g->bThemeChanged)
                    {
                        ActivateTheme(&g->ActiveTheme);
                    }

                    if (g->bSchemeChanged)
                    {
                        ApplyScheme(&g->Scheme, &g->ActiveTheme);
                        if (g->ActiveTheme.ThemeActive == FALSE)
                            g->ClassicTheme = g->ActiveTheme;
                    }

                    AppearancePage_UpdateThemePreview(hwndDlg, g);
                    g->bThemeChanged = FALSE;
                    g->bSchemeChanged = FALSE;
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)PSNRET_NOERROR);
                    return TRUE;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)FALSE);
                    return TRUE;

                case PSN_SETACTIVE:
                    if (g->Scheme.crColor[COLOR_DESKTOP] != g_GlobalData.desktop_color)
                    {
                        g->Scheme.crColor[COLOR_DESKTOP] = g_GlobalData.desktop_color;
                        AppearancePage_UpdateThemePreview(hwndDlg, g);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
