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
AppearancePage_ShowColorScemes(GLOBALS *g, HWND hwndColor, INT ThemeId)
{
	int i, iListIndex;

	SendMessage(hwndColor, CB_RESETCONTENT , 0, 0);

	if(g->bThemeActive == FALSE)
	{
		for(i = 0; i < g_TemplateCount; i++)
		{
			iListIndex = SendMessage(hwndColor, CB_ADDSTRING, 0, (LPARAM)g_ColorSchemes[i].strLegacyName);
			SendMessage(hwndColor, CB_SETITEMDATA, iListIndex, i);
			if (lstrcmp(g_ColorSchemes[i].strKeyName, g->strSelectedStyle) == 0)
			{
				g->SchemeId = i;
				SendMessage(hwndColor, CB_SETCURSEL, (WPARAM)iListIndex, 0);
			}
		}
	}
	else
	{
		PTHEME pTheme = (PTHEME)DSA_GetItemPtr(g->Themes, ThemeId);
		for(i = 0; i < pTheme->ColorsCount; i++)
		{
			PTHEME_STYLE pStyleName;
			pStyleName = (PTHEME_STYLE)DSA_GetItemPtr(pTheme->Colors, i);
			iListIndex = SendMessage(hwndColor, CB_ADDSTRING, 0, (LPARAM)pStyleName->DisplayName);
			SendMessage(hwndColor, CB_SETITEMDATA, iListIndex, i);
			if(i == 0 || (g->pszColorName && wcscmp(pStyleName->StlyeName, g->pszColorName) == 0))
			{
				g->SchemeId = i;
				SendMessage(hwndColor, CB_SETCURSEL, (WPARAM)iListIndex, 0);
			}
		}
	}
}

static INT_PTR
AppearancePage_OnInit(HWND hwndDlg)
{
	INT i, /*TemplateCount,*/ iListIndex;
	HWND hwndColor, hwndTheme;
	GLOBALS *g;

	g = (GLOBALS*)LocalAlloc(LPTR, sizeof(GLOBALS));
	if (g == NULL)
		return FALSE;

	SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);

	LoadCurrentScheme(&g->Scheme);
	g->SchemeAdv = g->Scheme;
	g->bThemeChanged = FALSE;
	g->bSchemeChanged = FALSE;
	g->hBoldFont = g->hItalicFont = NULL;
	g->hbmpColor[0] = g->hbmpColor[1] = g->hbmpColor[2] = NULL;
	g->bInitializing = FALSE;
	g->bThemeActive = FALSE;

	LoadThemes(g);

	/*TemplateCount = */ LoadSchemePresetEntries(g->strSelectedStyle);

	hwndColor = GetDlgItem(hwndDlg, IDC_APPEARANCE_COLORSCHEME);
	g->SchemeId = -1;
	g->bInitializing = TRUE;

	hwndTheme = GetDlgItem(hwndDlg, IDC_APPEARANCE_VISUAL_STYLE);
	for(i = 0; i < g->ThemesCount; i++)
	{
		PTHEME pTheme = (PTHEME)DSA_GetItemPtr(g->Themes, i);
		iListIndex = SendMessage(hwndTheme, CB_ADDSTRING, 0, (LPARAM)pTheme->displayName);
		SendMessage(hwndTheme, CB_SETITEMDATA, iListIndex, i);
		if((!pTheme->themeFileName && !IsThemeActive()) || 
		   (pTheme->themeFileName && g->pszThemeFileName && wcscmp(pTheme->themeFileName, g->pszThemeFileName) == 0 ))
		{
			g->ThemeId = i;
			g->bThemeActive = (pTheme->themeFileName != NULL);
			SendMessage(hwndTheme, CB_SETCURSEL, (WPARAM)iListIndex, 0);
			AppearancePage_ShowColorScemes(g, hwndColor, i);
		}

	}

	g->bInitializing = FALSE;

	return FALSE;
}

static VOID
AppearancePage_OnDestroy(HWND hwndDlg, GLOBALS *g)
{
	LocalFree(g);
}

static INT
GetSelectedId(HWND hwndDlg, int nIDDlgItem)
{
	HWND hwndCombo;
	INT sel;

	hwndCombo = GetDlgItem(hwndDlg, nIDDlgItem);
	sel = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
	if (sel == CB_ERR)
		return -1;
	return (INT)SendMessage(hwndCombo, CB_GETITEMDATA, (WPARAM)sel, 0);
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
			switch (LOWORD(wParam))
			{
				case IDC_APPEARANCE_EFFECTS:
					if (DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_EFFAPPEARANCE),
									   hwndDlg, EffAppearanceDlgProc, (LPARAM)g) == IDOK)
					{
						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
						g->Scheme = g->SchemeAdv;
						g->bSchemeChanged = TRUE;
						// Effects dialog doesn't change the color scheme, therefore the following lines are commented out, until fixed finally
						//g->SchemeId = -1;	/* Customized */
						//SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_SETCURSEL, (WPARAM)-1, 0);
						//SetDlgItemText(hwndDlg, IDC_APPEARANCE_COLORSCHEME, TEXT(""));
						SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
					}
					break;

				case IDC_APPEARANCE_ADVANCED:
					if (DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_ADVAPPEARANCE),
									   hwndDlg, AdvAppearanceDlgProc, (LPARAM)g) == IDOK)
					{
						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
						g->bSchemeChanged = TRUE;
						g->Scheme = g->SchemeAdv;
						g->SchemeId = -1;	/* Customized */
						g_GlobalData.desktop_color = g->Scheme.crColor[COLOR_DESKTOP];

						SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_SETCURSEL, (WPARAM)-1, 0);
						SetDlgItemText(hwndDlg, IDC_APPEARANCE_COLORSCHEME, TEXT(""));

						SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
					}
					break;

				case IDC_APPEARANCE_COLORSCHEME:
					if (HIWORD(wParam) == CBN_SELCHANGE && !g->bInitializing)
					{
						INT SchemeId = GetSelectedId(hwndDlg, IDC_APPEARANCE_COLORSCHEME);

						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

						if(g->bThemeActive == FALSE)
						{
							COLOR_SCHEME Scheme;

							g->bSchemeChanged = TRUE;
							if (SchemeId != -1 && LoadSchemeFromReg(&Scheme, SchemeId))
							{
								g->Scheme = Scheme;
								g_GlobalData.desktop_color = g->Scheme.crColor[COLOR_DESKTOP];
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&Scheme);
							}
						}
						else
						{
							g->bThemeChanged = TRUE;
						}
					}
					break;
				case IDC_APPEARANCE_VISUAL_STYLE:
					if (HIWORD(wParam) == CBN_SELCHANGE && !g->bInitializing)
					{
						INT ThemeId = GetSelectedId(hwndDlg, IDC_APPEARANCE_VISUAL_STYLE);
						HWND hwndColor = GetDlgItem(hwndDlg, IDC_APPEARANCE_COLORSCHEME);

						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

						g->bThemeActive = (ThemeId!=0);
						g->bThemeChanged = TRUE;
						AppearancePage_ShowColorScemes(g, hwndColor, ThemeId);
					}
					break;
			}
			break;

		case WM_NOTIFY:
			lpnm = (LPNMHDR)lParam;
			switch (lpnm->code)
			{
				case PSN_APPLY:

					g->ThemeId = GetSelectedId(hwndDlg, IDC_APPEARANCE_VISUAL_STYLE);
					g->SchemeId = GetSelectedId(hwndDlg, IDC_APPEARANCE_COLORSCHEME);

					if(g->bSchemeChanged)
					{
						ApplyScheme(&g->Scheme, g->SchemeId);
					}

					if(g->bThemeChanged)
					{
						PTHEME pTheme = (PTHEME)DSA_GetItemPtr(g->Themes, g->ThemeId);
						ActivateTheme(pTheme, g->SchemeId, 0);
					}

					SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
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
						SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
					}
					break;
			}
			break;
	}

	return FALSE;
}
