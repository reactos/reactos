/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/appearance.c
 * PURPOSE:         Appearance property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Timo Kreuzer (timo[dot]kreuzer[at]web[dot]de)
 */

#include "desk.h"
#include "theme.h"
#include "preview.h"
#include "appearance.h"

/******************************************************************************/

static INT_PTR
AppearancePage_OnInit(HWND hwndDlg)
{
	TCHAR strSelectedStyle[4];
	INT i, TemplateCount, iListIndex;
	HWND hwndCombo;
	GLOBALS *g;

	g = (GLOBALS*)LocalAlloc(LPTR, sizeof(GLOBALS));
	if (g == NULL)
		return FALSE;

	SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);

	LoadCurrentScheme(&g->Scheme);
	g->SchemeAdv = g->Scheme;
	g->bHasChanged = FALSE;
	g->hBoldFont = g->hItalicFont = NULL;
	g->hbmpColor[0] = g->hbmpColor[1] = g->hbmpColor[2] = NULL;
	g->bInitializing = FALSE;

	TemplateCount = LoadSchemePresetEntries(strSelectedStyle);

	hwndCombo = GetDlgItem(hwndDlg, IDC_APPEARANCE_COLORSCHEME);
	g->SchemeId = -1;
	g->bInitializing = TRUE;
	for(i = 0; i < TemplateCount; i++)
	{
		iListIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)g_ColorSchemes[i].strLegacyName);
		SendMessage(hwndCombo, CB_SETITEMDATA, iListIndex, i);
		if (lstrcmp(g_ColorSchemes[i].strKeyName, strSelectedStyle) == 0)
		{
			g->SchemeId = i;
			SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM)iListIndex, 0);
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
GetSelectedThemeId(HWND hwndDlg)
{
	HWND hwndCombo;
	INT sel;

	hwndCombo = GetDlgItem(hwndDlg, IDC_APPEARANCE_COLORSCHEME);
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
						g->bHasChanged = TRUE;
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
						g->bHasChanged = TRUE;
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
						COLOR_SCHEME Scheme;
						INT SchemeId = GetSelectedThemeId(hwndDlg);
						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
						g->bHasChanged = TRUE;
						if (SchemeId != -1 && LoadSchemeFromReg(&Scheme, SchemeId))
						{
							g->Scheme = Scheme;
							g->SchemeId = SchemeId;
							g_GlobalData.desktop_color = g->Scheme.crColor[COLOR_DESKTOP];
							SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&Scheme);
						}
					}
					break;
			}
			break;

		case WM_NOTIFY:
			lpnm = (LPNMHDR)lParam;
			switch (lpnm->code)
			{
				case PSN_APPLY:
					if (g->bHasChanged)
					{
						INT SchemeId = GetSelectedThemeId(hwndDlg);
						ApplyScheme(&g->Scheme, SchemeId);
						g->SchemeId = SchemeId;
						SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_PREVIEW, PVM_UPDATETHEME, 0, (LPARAM)&g->Scheme);
						g->bHasChanged = FALSE;
					}
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
