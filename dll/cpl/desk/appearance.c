/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/appearance.c
 * PURPOSE:         Appearance property page
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 *                  Timo Kreuzer (timo[dot]kreuzer[at]web[dot]de
 */

#include "desk.h"
#include "appearance.h"

/******************************************************************************/

/* This const assigns the color and metric numbers to the elements from the elements list */

/* Size 1 (width)	Size 2 (height)	Color 1					Color 2							Font			Fontcolor */
const ASSIGNMENT g_Assignment[NUM_ELEMENTS] =
{ {-1,				-1,				COLOR_DESKTOP,			-1,								-1,				-1},				/* -Desktop */
  {SIZE_CAPTION_Y,	-1,				COLOR_INACTIVECAPTION,	COLOR_GRADIENTINACTIVECAPTION,	FONT_CAPTION,	-1},				/* inactive window caption */
  {SIZE_BORDER_X,	SIZE_BORDER_Y,	COLOR_INACTIVEBORDER,	-1,								-1,				-1},  				/* inactive window border */
  {SIZE_CAPTION_Y,	-1,				COLOR_ACTIVECAPTION,	COLOR_GRADIENTACTIVECAPTION,	FONT_CAPTION,	COLOR_CAPTIONTEXT},	/* -active window caption */
  {SIZE_BORDER_X,	SIZE_BORDER_Y,	COLOR_ACTIVEBORDER,		-1,								-1,				-1},				/* active window border */
  {SIZE_MENU_X,		SIZE_MENU_Y,	COLOR_MENU,				-1, 							FONT_MENU,		COLOR_MENUTEXT},	/* menu */
  {SIZE_MENU_X,		SIZE_MENU_Y,	COLOR_HIGHLIGHT,		-1,								FONT_HILIGHT,	COLOR_HIGHLIGHTTEXT},/* marked element */
  {-1,				-1,				COLOR_WINDOW,			-1 /*COLOR_WINDOWFRAME*/,				-1,				COLOR_WINDOWTEXT},	/* window */
  {SIZE_SCROLL_X,	SIZE_SCROLL_Y,	COLOR_SCROLLBAR,		-1,								-1,				-1},				/* scroll bar */
  {-1,				-1,				COLOR_3DFACE,			-1,								-1,				COLOR_BTNTEXT},		/* 3d objects */
  {SIZE_SMCAPTION_Y,-1,				-1,						-1,								FONT_SMCAPTION,	-1},				/* palette window caption */
  {-1,				-1,				-1,						-1,								-1,				-1},				/* symbol caption FIXME: Access? */
  {SIZE_CAPTION_Y,	-1,				-1,						-1,								-1,				-1},				/* caption bar */
  {-1,				-1,				-1,						-1,								-1,				COLOR_GRAYTEXT},	/* inactive menu item FIXME: Access? */
  {-1,				-1,				-1,						-1,								FONT_DIALOG,	COLOR_WINDOWTEXT},	/* dialog */
  {-1,				-1,				-1,						-1,								-1,				-1},				/* scrollbar controls FIXME: Access? */
  {-1,				-1,				COLOR_APPWORKSPACE,		-1,								-1,				-1},				/* application background */
  {-1,				-1,				-1,						-1,								-1,				-1},				/* small caption bar FIXME: Access? */
  {SIZE_ICON_SPC_X,	-1,				-1,						-1,								-1,				-1},				/* symbol distance horiz. */
  {SIZE_ICON_SPC_Y,	-1,				-1,						-1,								-1,				-1},				/* symbol distance vert. */
  {-1,				-1,				COLOR_INFOBK,			-1,								FONT_INFO,		COLOR_INFOTEXT},	/* quickinfo */
  {SIZE_ICON_X,		SIZE_ICON_Y,	-1,						-1,								FONT_ICON,		-1}};				/* symbol */

/* This is the list of names for the colors stored in the registry */
const TCHAR g_RegColorNames[NUM_COLORS][MAX_COLORNAMELENGTH] =
	{TEXT("Scrollbar"),				/* 00 = COLOR_SCROLLBAR */
	TEXT("Background"),				/* 01 = COLOR_DESKTOP */
	TEXT("ActiveTitle"),			/* 02 = COLOR_ACTIVECAPTION  */
	TEXT("InactiveTitle"),			/* 03 = COLOR_INACTIVECAPTION */
	TEXT("Menu"),					/* 04 = COLOR_MENU */
	TEXT("Window"),					/* 05 = COLOR_WINDOW */
	TEXT("WindowFrame"),			/* 06 = COLOR_WINDOWFRAME */
	TEXT("MenuText"), 				/* 07 = COLOR_MENUTEXT */
	TEXT("WindowText"), 			/* 08 = COLOR_WINDOWTEXT */
	TEXT("TitleText"), 				/* 09 = COLOR_CAPTIONTEXT */
	TEXT("ActiveBorder"), 			/* 10 = COLOR_ACTIVEBORDER */
	TEXT("InactiveBorder"), 		/* 11 = COLOR_INACTIVEBORDER */
	TEXT("AppWorkSpace"), 			/* 12 = COLOR_APPWORKSPACE */
	TEXT("Hilight"), 				/* 13 = COLOR_HIGHLIGHT */
	TEXT("HilightText"), 			/* 14 = COLOR_HIGHLIGHTTEXT */
	TEXT("ButtonFace"), 			/* 15 = COLOR_BTNFACE */
	TEXT("ButtonShadow"), 			/* 16 = COLOR_BTNSHADOW */
	TEXT("GrayText"), 				/* 17 = COLOR_GRAYTEXT */
	TEXT("ButtonText"), 			/* 18 = COLOR_BTNTEXT */
	TEXT("InactiveTitleText"), 		/* 19 = COLOR_INACTIVECAPTIONTEXT */
	TEXT("ButtonHilight"), 			/* 20 = COLOR_BTNHIGHLIGHT */
	TEXT("ButtonDkShadow"), 		/* 21 = COLOR_3DDKSHADOW */
	TEXT("ButtonLight"), 			/* 22 = COLOR_3DLIGHT */
	TEXT("InfoText"), 				/* 23 = COLOR_INFOTEXT */
	TEXT("InfoWindow"), 			/* 24 = COLOR_INFOBK */
	TEXT("ButtonAlternateFace"),	/* 25 = COLOR_ALTERNATEBTNFACE */
	TEXT("HotTrackingColor"), 		/* 26 = COLOR_HOTLIGHT */
	TEXT("GradientActiveTitle"),	/* 27 = COLOR_GRADIENTACTIVECAPTION */
	TEXT("GradientInactiveTitle"),	/* 28 = COLOR_GRADIENTINACTIVECAPTION */
	TEXT("MenuHilight"), 			/* 29 = COLOR_MENUHILIGHT */
	TEXT("MenuBar"), 				/* 30 = COLOR_MENUBAR */
};

/* This is the list of used metrics and their numbers */
const int g_SizeMetric[NUM_SIZES] =
{
	SM_CXBORDER,
	SM_CYBORDER,
	SM_CYCAPTION,
	SM_CXICON,
	SM_CYICON,
	SM_CXICONSPACING,
	SM_CYICONSPACING,
	SM_CXMENUSIZE,
	SM_CYMENU,
	SM_CXVSCROLL,
	SM_CYHSCROLL,
	SM_CYSMCAPTION
};

/******************************************************************************/

static VOID
LoadCurrentTheme(GLOBALS* g)
{
	INT i;
	NONCLIENTMETRICS NonClientMetrics;

	g->Theme.bHasChanged = FALSE;
	/* FIXME: it may be custom! */
	g->Theme.bIsCustom = FALSE;

	/* Load colors */
	for (i = 0; i <= 30; i++)
	{
		g->ColorList[i] = i;
		g->Theme.crColor[i] = (COLORREF)GetSysColor(i);
	}

	/* Load sizes */
	for (i = 0; i <= 11; i++)
	{
		g->Theme.Size[i] = GetSystemMetrics(g_SizeMetric[i]);
	}

	/* Load fonts */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	g->Theme.lfFont[FONT_CAPTION] = NonClientMetrics.lfCaptionFont;
	g->Theme.lfFont[FONT_SMCAPTION] = NonClientMetrics.lfSmCaptionFont;
	g->Theme.lfFont[FONT_MENU] = NonClientMetrics.lfMenuFont;
	g->Theme.lfFont[FONT_INFO] = NonClientMetrics.lfStatusFont;
	g->Theme.lfFont[FONT_DIALOG] = NonClientMetrics.lfMessageFont;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &g->Theme.lfFont[FONT_ICON], 0);
}


static BOOL
LoadThemeFromReg(GLOBALS* g, INT iPreset)
{
	INT i;
	TCHAR strSizeName[20] = {TEXT("Sizes\\0")};
	TCHAR strValueName[10];
	HKEY hkNewSchemes, hkScheme, hkSize;
	DWORD dwType, dwLength;
	BOOL Ret = FALSE;

	if(RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance\\New Schemes"),
		0, KEY_READ, &hkNewSchemes) == ERROR_SUCCESS)
	{
		if(RegOpenKeyEx(hkNewSchemes, g->ThemeTemplates[iPreset].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
		{
			lstrcpyn(&strSizeName[6],g->ThemeTemplates[iPreset].strSizeName, 3);
			if(RegOpenKeyEx(hkScheme, strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
			{
				Ret = TRUE;

				dwLength = sizeof(DWORD);
				if (RegQueryValueEx(hkSize, TEXT("FlatMenus"), NULL, &dwType, (LPBYTE)&g->Theme.bFlatMenus, &dwLength) != ERROR_SUCCESS ||
				    dwType != REG_DWORD || dwLength != sizeof(DWORD))
				{
					/* Failed to read registry value */
					g->Theme.bFlatMenus = FALSE;
					Ret = FALSE;
				}

				for (i = 0; i <= 30; i++)
				{
					wsprintf(strValueName, TEXT("Color #%d"), i);
					dwLength = sizeof(COLORREF);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&g->Theme.crColor[i], &dwLength) != ERROR_SUCCESS ||
					    dwType != REG_DWORD || dwLength != sizeof(COLORREF))
					{
						/* Failed to read registry value, initialize with current setting for now */
						g->Theme.crColor[i] = GetSysColor(i);
						Ret = FALSE;
					}
				}
				for (i = 0; i <= 5; i++)
				{
					wsprintf(strValueName, TEXT("Font #%d"), i);
					dwLength = sizeof(LOGFONT);
					g->Theme.lfFont[i].lfFaceName[0] = 'x';
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&g->Theme.lfFont[i], &dwLength) != ERROR_SUCCESS ||
					    dwType != REG_BINARY || dwLength != sizeof(LOGFONT))
					{
						/* Failed to read registry value */
						Ret = FALSE;
					}
				}
				for (i = 0; i <= 8; i++)
				{
					wsprintf(strValueName, TEXT("Size #%d"), i);
					dwLength = sizeof(UINT64);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&g->Theme.Size[i], &dwLength) != ERROR_SUCCESS ||
					    dwType != REG_QWORD || dwLength != sizeof(UINT64))
					{
						/* Failed to read registry value, initialize with current setting for now */
						g->Theme.Size[i] = GetSystemMetrics(g_SizeMetric[i]);
						Ret = FALSE;
					}
				}
				RegCloseKey(hkScheme);
			}
			RegCloseKey(hkScheme);
		}
		RegCloseKey(hkNewSchemes);
	}

	return Ret;
}


static VOID
ApplyTheme(GLOBALS* g)
{
	INT i, Result;
	HKEY hKey;
	DWORD dwDisposition = 0;
	TCHAR clText[16] = {0};
	NONCLIENTMETRICS NonClientMetrics;
	HFONT hMyFont;
	LOGFONT lfButtonFont;

	if (!g->Theme.bHasChanged)
		return;

	/* Update some globals */
	g->crCOLOR_BTNFACE = g->Theme.crColor[COLOR_BTNFACE];
	g->crCOLOR_BTNTEXT = g->Theme.crColor[COLOR_BTNTEXT];
	g->crCOLOR_BTNSHADOW = g->Theme.crColor[COLOR_BTNSHADOW];
	g->crCOLOR_BTNHIGHLIGHT = g->Theme.crColor[COLOR_BTNHIGHLIGHT];
	lfButtonFont = g->Theme.lfFont[FONT_DIALOG];

	/* Create new font for bold button */
	lfButtonFont.lfWeight = FW_BOLD;
	lfButtonFont.lfItalic = FALSE;
	hMyFont = CreateFontIndirect(&lfButtonFont);
	if (hMyFont)
	{
		if (g->hBoldFont)
			DeleteObject(g->hBoldFont);
		g->hBoldFont = hMyFont;
	}

	/* Create new font for italic button */
	lfButtonFont.lfWeight = FW_REGULAR;
	lfButtonFont.lfItalic = TRUE;
	hMyFont = CreateFontIndirect(&lfButtonFont);
	if (hMyFont)
	{
		if (g->hItalicFont)
			DeleteObject(g->hItalicFont);
		g->hItalicFont = hMyFont;
	}

	/* Apply Colors from global variable */
	SetSysColors(30, &g->ColorList[0], &g->Theme.crColor[0]);

	/* Save colors to registry */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Colors"), 0, KEY_ALL_ACCESS, &hKey);
	if (Result != ERROR_SUCCESS)
	{
		/* Could not open the key, try to create it */
		Result = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Colors"), 0, NULL, 0, KEY_ALL_ACCESS, NULL,&hKey, &dwDisposition);
	}

	if (Result == ERROR_SUCCESS)
	{
		for (i = 0; i <= 30; i++)
		{
			DWORD red   = GetRValue(g->Theme.crColor[i]);
			DWORD green = GetGValue(g->Theme.crColor[i]);
			DWORD blue  = GetBValue(g->Theme.crColor[i]);
			wsprintf(clText, TEXT("%d %d %d"), red, green, blue);
			RegSetValueEx(hKey, g_RegColorNames[i], 0, REG_SZ, (BYTE *)clText, lstrlen( clText )*sizeof(TCHAR) + sizeof(TCHAR));
		}

		RegCloseKey(hKey);
	}

	/* Apply the fonts */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	NonClientMetrics.lfCaptionFont = g->Theme.lfFont[FONT_CAPTION];
	NonClientMetrics.lfSmCaptionFont = g->Theme.lfFont[FONT_SMCAPTION];
	NonClientMetrics.lfMenuFont = g->Theme.lfFont[FONT_MENU];
	NonClientMetrics.lfStatusFont = g->Theme.lfFont[FONT_INFO];
	NonClientMetrics.lfMessageFont = g->Theme.lfFont[FONT_DIALOG];
	SystemParametersInfo(SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	SystemParametersInfo(SPI_SETICONTITLELOGFONT, sizeof(LOGFONT), &g->Theme.lfFont[FONT_ICON], 0);

	/* FIXME: Apply size metrics */

	/* Save fonts and size metrics to registry */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop\\WindowMetrics"), 0, KEY_ALL_ACCESS, &hKey);
	if (Result != ERROR_SUCCESS)
	{
		/* Could not open the key, try to create it */
		Result = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop\\WindowMetrics"), 0, NULL, 0, KEY_ALL_ACCESS, NULL,&hKey, &dwDisposition);
	}

	if (Result == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, TEXT("CaptionFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_CAPTION], sizeof(LOGFONT));
		RegSetValueEx(hKey, TEXT("SmCaptionFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_SMCAPTION], sizeof(LOGFONT));
		RegSetValueEx(hKey, TEXT("IconFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_ICON], sizeof(LOGFONT));
		RegSetValueEx(hKey, TEXT("MenuFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_MENU], sizeof(LOGFONT));
		RegSetValueEx(hKey, TEXT("StatusFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_INFO], sizeof(LOGFONT));
		RegSetValueEx(hKey, TEXT("MessageFont"), 0, REG_BINARY, (BYTE *)&g->Theme.lfFont[FONT_DIALOG], sizeof(LOGFONT));

		/* Save size metrics to registry */
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_BORDER_X]);
		RegSetValueEx(hKey, TEXT("BorderWidth"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_CAPTION_Y]);
		RegSetValueEx(hKey, TEXT("CaptionWidth"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_CAPTION_Y]);
		RegSetValueEx(hKey, TEXT("CaptionHeight"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_SMCAPTION_Y]);
		RegSetValueEx(hKey, TEXT("SmCaptionWidth"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_SMCAPTION_Y]);
		RegSetValueEx(hKey, TEXT("SmCaptionHeight"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_ICON_SPC_X]);
		RegSetValueEx(hKey, TEXT("IconSpacing"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_ICON_SPC_Y]);
		RegSetValueEx(hKey, TEXT("IconVerticalSpacing"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_MENU_X]);
		RegSetValueEx(hKey, TEXT("MenuWidth"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_MENU_Y]);
		RegSetValueEx(hKey, TEXT("MenuHeight"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_SCROLL_X]);
		RegSetValueEx(hKey, TEXT("ScrollWidth"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), -15 * g->Theme.Size[SIZE_SCROLL_Y]);
		RegSetValueEx(hKey, TEXT("ScrollHeight"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));
		wsprintf(clText, TEXT("%d"), g->Theme.Size[SIZE_ICON_X]);
		RegSetValueEx(hKey, TEXT("Shell Icon Sizet"), 0, REG_SZ, (BYTE *)clText, sizeof(clText));

		RegCloseKey(hKey);
	}
}


static INT_PTR
AppearancePage_OnInit(HWND hwndDlg, GLOBALS *g)
{
	HKEY hkNewSchemes, hkScheme, hkSizes, hkSize;
	FILETIME ftLastWriteTime;
	TCHAR strSelectedStyle[4];
	DWORD dwLength, dwType;
	DWORD dwDisposition = 0;
	INT iStyle, iSize, iTemplateIndex, iListIndex = 0;
	INT Result;

	g = (GLOBALS*)malloc(sizeof(GLOBALS));
	if (g == NULL)
	{
		return FALSE;
	}

	SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)g);

	LoadCurrentTheme(g);

	/* Fill color schemes combo */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance\\New Schemes"),
		0, KEY_READ, &hkNewSchemes);
	if (Result != ERROR_SUCCESS)
	{
		/* Could not open the key, try to create it */
		Result = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance\\New Schemes"), 0, NULL, 0, KEY_ALL_ACCESS, NULL,&hkNewSchemes, &dwDisposition);
		if (Result == ERROR_SUCCESS)
		{
			/* FIXME: We have created it new, so let's put somethig there */
		}
	}
	if (Result == ERROR_SUCCESS)
	{
		/* First find out the currently selected template */
		dwLength = 8;
		RegQueryValueEx(hkNewSchemes, TEXT("SelectedStyle"), NULL, &dwType, (LPBYTE)&strSelectedStyle, &dwLength);
		iTemplateIndex = 0;
		iStyle = 0;
		dwLength = MAX_TEMPLATENAMELENTGH;
		while((RegEnumKeyEx(hkNewSchemes, iStyle, g->ThemeTemplates[iTemplateIndex].strKeyName, &dwLength,
			NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iTemplateIndex < MAX_TEMPLATES))
		{
			/* is it really a template or one of the other entries */
			if (dwLength < 5)
			{
				if (RegOpenKeyEx(hkNewSchemes, g->ThemeTemplates[iTemplateIndex].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
				{
					if(RegOpenKeyEx(hkScheme, TEXT("Sizes"), 0, KEY_READ, &hkSizes) == ERROR_SUCCESS)
					{
						iSize = 0;
						dwLength = 3;
						while((RegEnumKeyEx(hkSizes, iSize, g->ThemeTemplates[iTemplateIndex].strSizeName, &dwLength,
							NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iSize <= 4))
						{
							if(RegOpenKeyEx(hkSizes, g->ThemeTemplates[iTemplateIndex].strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
							{
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("DisplayName"), NULL, &dwType, (LPBYTE)&g->ThemeTemplates[iTemplateIndex].strDisplayName, &dwLength);
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("LegacyName"), NULL, &dwType, (LPBYTE)&g->ThemeTemplates[iTemplateIndex].strLegacyName, &dwLength);
								RegCloseKey(hkSize);
							}
							iListIndex = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_ADDSTRING, 0, (LPARAM)g->ThemeTemplates[iTemplateIndex].strLegacyName);
							SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_SETITEMDATA, iListIndex, iTemplateIndex);
							if (lstrcmp(g->ThemeTemplates[iTemplateIndex].strKeyName, strSelectedStyle) == 0)
							{
								SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_SETCURSEL, (WPARAM)iListIndex, 0);
							}
							iSize++;
							iTemplateIndex++;
							dwLength = 3;
						}
						RegCloseKey(hkSizes);
					}
					RegCloseKey(hkScheme);
				}
			}
			iStyle++;
			dwLength = MAX_TEMPLATENAMELENTGH;
		}
		RegCloseKey(hkNewSchemes);
	}
	SendMessage(GetDlgItem(hwndDlg, IDC_APPEARANCE_COLORSCHEME), LB_SETCURSEL, 0, 0);

	return FALSE;
}


static INT_PTR
AppearancePage_OnDestroy(HWND hwndDlg, GLOBALS *g)
{
	free(g);
	return TRUE;
}


INT_PTR CALLBACK
AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT i, index;
	GLOBALS *g;
	LPNMHDR lpnm;

	g = (GLOBALS*)GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch (uMsg)
	{
		case WM_INITDIALOG:
			return AppearancePage_OnInit(hwndDlg, g);

		case WM_DESTROY:
			return AppearancePage_OnDestroy(hwndDlg, g);

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_APPEARANCE_ADVANCED:
					DialogBoxParam(hApplet, (LPCTSTR)IDD_ADVAPPEARANCE,
						hwndDlg, AdvAppearanceDlgProc, (LPARAM)g);

					/* Was anything changed in the advanced appearance dialog? */
					if (memcmp(&g->Theme, &g->ThemeAdv, sizeof(THEME)) != 0)
					{
						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
						g->Theme = g->ThemeAdv;
						g->Theme.bHasChanged = TRUE;
					}
					break;

				case IDC_APPEARANCE_COLORSCHEME:
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
						g->Theme.bHasChanged = TRUE;
						i = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_GETCURSEL, 0, 0);
						index = SendDlgItemMessage(hwndDlg, IDC_APPEARANCE_COLORSCHEME, CB_GETITEMDATA, (WPARAM)i, 0);
						LoadThemeFromReg(g, index);
					}
					break;

				default:
					return FALSE;
			}
			return TRUE;

		case WM_NOTIFY:
			lpnm = (LPNMHDR)lParam;
			switch (lpnm->code)
			{
				case PSN_APPLY:
					if (g->Theme.bHasChanged)
					{
						ApplyTheme(g);
					}
					return TRUE;

				default:
					return FALSE;
			}
			return TRUE;

		default:
			return FALSE;
	}

	return TRUE;
}
