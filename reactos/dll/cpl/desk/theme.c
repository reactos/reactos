/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/theme.c
 * PURPOSE:         Handling themes
 *
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "desk.h"
#include "theme.h"

static BOOL g_PresetLoaded = FALSE;
static INT g_TemplateCount = 0;

static INT g_ColorList[NUM_COLORS];

static const TCHAR g_CPColors[] = TEXT("Control Panel\\Colors");
static const TCHAR g_CPANewSchemes[] = TEXT("Control Panel\\Appearance\\New Schemes");
static const TCHAR g_SelectedStyle[] = TEXT("SelectedStyle");

/******************************************************************************/

THEME_PRESET g_ThemeTemplates[MAX_TEMPLATES];

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
	SM_CXBORDER,        /* 00: SIZE_BORDER_X */
	SM_CYBORDER,        /* 01: SIZE_BORDER_Y */
	SM_CYCAPTION,       /* 02: SIZE_CAPTION_Y */
	SM_CXICON,          /* 03: SIZE_ICON_X */
	SM_CYICON,          /* 04: SIZE_ICON_Y */
	SM_CXICONSPACING,   /* 05: SIZE_ICON_SPC_X */
	SM_CYICONSPACING,   /* 06: SIZE_ICON_SPC_Y */
	SM_CXMENUSIZE,      /* 07: SIZE_MENU_SIZE_X */
	SM_CYMENU,          /* 08: SIZE_MENU_Y */
	SM_CXVSCROLL,       /* 09: SIZE_SCROLL_X */
	SM_CYHSCROLL,       /* 10: SIZE_SCROLL_Y */
	SM_CYSMCAPTION,     /* 11: SIZE_SMCAPTION_Y */
	SM_CXEDGE,          /* 12: SIZE_EDGE_X */
	SM_CYEDGE,          /* 13: SIZE_EDGE_Y */
	SM_CYSIZEFRAME,     /* 14: SIZE_FRAME_Y */
	SM_CXMENUCHECK,     /* 15: SIZE_MENU_CHECK_X */
	SM_CYMENUCHECK,     /* 16: SIZE_MENU_CHECK_Y */
	SM_CYMENUSIZE,      /* 17: SIZE_MENU_SIZE_Y */
	SM_CXSIZE,          /* 18: SIZE_SIZE_X */
	SM_CYSIZE           /* 19: SIZE_SIZE_Y */
};

/******************************************************************************/

VOID LoadCurrentTheme(THEME* theme)
{
	INT i;
	NONCLIENTMETRICS NonClientMetrics;

	/* Load colors */
	for (i = 0; i < NUM_COLORS; i++)
	{
		g_ColorList[i] = i;
		theme->crColor[i] = (COLORREF)GetSysColor(i);
	}

	/* Load sizes */
	for (i = 0; i < NUM_SIZES; i++)
	{
		theme->Size[i] = GetSystemMetrics(g_SizeMetric[i]);
	}

	/* Load fonts */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	theme->lfFont[FONT_CAPTION] = NonClientMetrics.lfCaptionFont;
	theme->lfFont[FONT_SMCAPTION] = NonClientMetrics.lfSmCaptionFont;
	theme->lfFont[FONT_MENU] = NonClientMetrics.lfMenuFont;
	theme->lfFont[FONT_INFO] = NonClientMetrics.lfStatusFont;
	theme->lfFont[FONT_DIALOG] = NonClientMetrics.lfMessageFont;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &theme->lfFont[FONT_ICON], 0);

	/* Effects */
	/* "Use the following transition effect for menus and tooltips" */
	SystemParametersInfo(SPI_GETMENUANIMATION, sizeof(BOOL), &theme->Effects.bMenuAnimation, 0);
	SystemParametersInfo(SPI_GETMENUFADE, sizeof(BOOL), &theme->Effects.bMenuFade, 0);
	/* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
	 * Just keep them in sync for now:
	 */
	theme->Effects.bTooltipAnimation  = theme->Effects.bMenuAnimation;
	theme->Effects.bTooltipFade	   = theme->Effects.bMenuFade;

	/* show content of windows during dragging */
	SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &theme->Effects.bDragFullWindows, 0);

	/* "Hide underlined letters for keyboard navigation until I press the Alt key" */
	SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &theme->Effects.bKeyboardCues, 0);
}

BOOL LoadThemeFromReg(THEME* theme, INT ThemeId)
{
	INT i;
	TCHAR strSelectedStyle[4];
	TCHAR strSizeName[20] = {TEXT("Sizes\\0")};
	TCHAR strValueName[10];
	HKEY hkNewSchemes, hkScheme, hkSize;
	DWORD dwType, dwLength;
	BOOL Ret = FALSE;

	if (!g_PresetLoaded)
		LoadThemePresetEntries(strSelectedStyle);

	if (ThemeId == -1)
		return FALSE;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, g_CPANewSchemes, 0, KEY_READ, &hkNewSchemes) == ERROR_SUCCESS)
	{
		if (RegOpenKeyEx(hkNewSchemes, g_ThemeTemplates[ThemeId].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
		{
			lstrcpyn(&strSizeName[6], g_ThemeTemplates[ThemeId].strSizeName, 3);
			if (RegOpenKeyEx(hkScheme, strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
			{
				Ret = TRUE;

				dwLength = sizeof(DWORD);
				if (RegQueryValueEx(hkSize, TEXT("FlatMenus"), NULL, &dwType, (LPBYTE)&theme->bFlatMenus, &dwLength) != ERROR_SUCCESS ||
					dwType != REG_DWORD)
				{
					/* Failed to read registry value */
					theme->bFlatMenus = FALSE;
				}

				for (i = 0; i < NUM_COLORS; i++)
				{
					wsprintf(strValueName, TEXT("Color #%d"), i);
					dwLength = sizeof(COLORREF);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&theme->crColor[i], &dwLength) != ERROR_SUCCESS ||
						dwType != REG_DWORD)
					{
						/* Failed to read registry value, initialize with current setting for now */
						theme->crColor[i] = GetSysColor(i);
					}
				}

				for (i = 0; i < NUM_FONTS; i++)
				{
					wsprintf(strValueName, TEXT("Font #%d"), i);
					dwLength = sizeof(LOGFONT);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&theme->lfFont[i], &dwLength) != ERROR_SUCCESS ||
						dwType != REG_BINARY || dwLength != sizeof(LOGFONT))
					{
						/* Failed to read registry value */
						Ret = FALSE;
					}
				}

				for (i = 0; i < NUM_SIZES; i++)
				{
					wsprintf(strValueName, TEXT("Size #%d"), i);
					dwLength = sizeof(UINT64);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&theme->Size[i], &dwLength) != ERROR_SUCCESS ||
						dwType != REG_QWORD || dwLength != sizeof(UINT64))
					{
						/* Failed to read registry value, initialize with current setting for now */
						theme->Size[i] = GetSystemMetrics(g_SizeMetric[i]);
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
_UpdateUserPref(UINT SpiGet, UINT SpiSet, BOOL *pbFlag)
{
	SystemParametersInfo(SpiSet, 0, (PVOID)pbFlag, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
}
#define UPDATE_USERPREF(NAME,pbFlag) _UpdateUserPref(SPI_GET ## NAME, SPI_SET ## NAME, pbFlag)

VOID ApplyTheme(THEME* theme, INT ThemeId)
{
	INT i, Result;
	HKEY hKey;
	DWORD dwDisposition;
	TCHAR clText[16];
	NONCLIENTMETRICS NonClientMetrics;
	ICONMETRICS IconMetrics;

	/* Apply Colors from global variable */
	SetSysColors(NUM_COLORS, g_ColorList, theme->crColor);

	/* Save colors to registry */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, g_CPColors, 0, KEY_ALL_ACCESS, &hKey);
	if (Result != ERROR_SUCCESS)
	{
		/* Could not open the key, try to create it */
		Result = RegCreateKeyEx(HKEY_CURRENT_USER, g_CPColors, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	}

	if (Result == ERROR_SUCCESS)
	{
		for (i = 0; i < NUM_COLORS; i++)
		{
			DWORD red   = GetRValue(theme->crColor[i]);
			DWORD green = GetGValue(theme->crColor[i]);
			DWORD blue  = GetBValue(theme->crColor[i]);
			wsprintf(clText, TEXT("%d %d %d"), red, green, blue);
			RegSetValueEx(hKey, g_RegColorNames[i], 0, REG_SZ, (BYTE *)clText, (lstrlen(clText) + 1) * sizeof(TCHAR));
		}
		RegCloseKey(hKey);
	}

	/* Apply non client metrics */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	NonClientMetrics.lfCaptionFont = theme->lfFont[FONT_CAPTION];
	NonClientMetrics.lfSmCaptionFont = theme->lfFont[FONT_SMCAPTION];
	NonClientMetrics.lfMenuFont = theme->lfFont[FONT_MENU];
	NonClientMetrics.lfStatusFont = theme->lfFont[FONT_INFO];
	NonClientMetrics.lfMessageFont = theme->lfFont[FONT_DIALOG];
	NonClientMetrics.iBorderWidth = theme->Size[SIZE_BORDER_X];
	NonClientMetrics.iScrollWidth = theme->Size[SIZE_SCROLL_X];
	NonClientMetrics.iScrollHeight = theme->Size[SIZE_SCROLL_Y];
	NonClientMetrics.iCaptionWidth = theme->Size[SIZE_CAPTION_Y];
	NonClientMetrics.iCaptionHeight = theme->Size[SIZE_CAPTION_Y];
	NonClientMetrics.iSmCaptionWidth = theme->Size[SIZE_SMCAPTION_Y];
	NonClientMetrics.iSmCaptionHeight = theme->Size[SIZE_SMCAPTION_Y];
	NonClientMetrics.iMenuWidth = theme->Size[SIZE_MENU_SIZE_X];
	NonClientMetrics.iMenuHeight = theme->Size[SIZE_MENU_Y];
	SystemParametersInfo(SPI_SETNONCLIENTMETRICS, 
						 sizeof(NONCLIENTMETRICS), 
						 &NonClientMetrics, 
						 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	/* Apply icon metrics */
	IconMetrics.cbSize = sizeof(ICONMETRICS);
	SystemParametersInfo(SPI_GETICONMETRICS, sizeof(ICONMETRICS), &IconMetrics, 0);
	IconMetrics.iHorzSpacing = theme->Size[SIZE_ICON_SPC_X];
	IconMetrics.iVertSpacing = theme->Size[SIZE_ICON_SPC_Y];
	IconMetrics.lfFont = theme->lfFont[FONT_ICON];
	SystemParametersInfo(SPI_SETICONMETRICS, 
						 sizeof(ICONMETRICS), 
						 &IconMetrics, 
						 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	/* Effects, save only when needed: */
	/* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
	 * Just keep them in sync for now.
	 */
	theme->Effects.bTooltipAnimation  = theme->Effects.bMenuAnimation;
	theme->Effects.bTooltipFade	   = theme->Effects.bMenuFade;
	SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, theme->Effects.bDragFullWindows, (PVOID)&theme->Effects.bDragFullWindows, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
	SystemParametersInfo(SPI_SETKEYBOARDCUES, 0, IntToPtr(theme->Effects.bKeyboardCues), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
	//UPDATE_USERPREF(ACTIVEWINDOWTRACKING, &theme->Effects.bActiveWindowTracking);
	//UPDATE_USERPREF(MENUANIMATION, &theme->Effects.bMenuAnimation);
	//UPDATE_USERPREF(COMBOBOXANIMATION, &theme->Effects.bComboBoxAnimation);
	//UPDATE_USERPREF(LISTBOXSMOOTHSCROLLING, &theme->Effects.bListBoxSmoothScrolling);
	//UPDATE_USERPREF(GRADIENTCAPTIONS, &theme->Effects.bGradientCaptions);
	//UPDATE_USERPREF(ACTIVEWNDTRKZORDER, &theme->Effects.bActiveWndTrkZorder);
	//UPDATE_USERPREF(HOTTRACKING, &theme->Effects.bHotTracking);
	UPDATE_USERPREF(MENUFADE, &theme->Effects.bMenuFade);
	//UPDATE_USERPREF(SELECTIONFADE, &theme->Effects.bSelectionFade);
	UPDATE_USERPREF(TOOLTIPANIMATION, &theme->Effects.bTooltipAnimation);
	UPDATE_USERPREF(TOOLTIPFADE, &theme->Effects.bTooltipFade);
	//UPDATE_USERPREF(CURSORSHADOW, &theme->Effects.bCursorShadow);
	//UPDATE_USERPREF(UIEFFECTS, &theme->Effects.bUiEffects);

	/* Save ThemeId */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, g_CPANewSchemes, 0, KEY_ALL_ACCESS, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		if (ThemeId == -1)
			clText[0] = TEXT('\0');
		else
			lstrcpy(clText, g_ThemeTemplates[ThemeId].strKeyName);
		RegSetValueEx(hKey, g_SelectedStyle, 0, REG_SZ, (BYTE *)clText, (lstrlen(clText) + 1) * sizeof(TCHAR));
		RegCloseKey(hKey);
	}
}

BOOL SaveTheme(THEME* theme, LPCTSTR strLegacyName)
{
	/* FIXME: implement */
	return FALSE;
}

INT LoadThemePresetEntries(LPTSTR pszSelectedStyle)
{
	HKEY hkNewSchemes, hkScheme, hkSizes, hkSize;
	FILETIME ftLastWriteTime;
	DWORD dwLength, dwType;
	DWORD dwDisposition;
	INT iStyle, iSize, iTemplateIndex;
	INT Result;

	lstrcpy(pszSelectedStyle, TEXT(""));

	iTemplateIndex = 0;
	Result = RegCreateKeyEx(HKEY_CURRENT_USER, g_CPANewSchemes, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkNewSchemes, &dwDisposition);
	if (Result == ERROR_SUCCESS)
	{
		/* First find out the currently selected template */
		dwLength = 4 * sizeof(TCHAR);
		RegQueryValueEx(hkNewSchemes, g_SelectedStyle, NULL, &dwType, (LPBYTE)pszSelectedStyle, &dwLength);

		/* Check if already loaded */
		if (g_PresetLoaded)
		{
			RegCloseKey(hkNewSchemes);
			return g_TemplateCount;
		}

		iStyle = 0;
		dwLength = MAX_TEMPLATENAMELENTGH;
		while((RegEnumKeyEx(hkNewSchemes, iStyle, g_ThemeTemplates[iTemplateIndex].strKeyName, &dwLength,
							NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iTemplateIndex < MAX_TEMPLATES))
		{
			/* is it really a template or one of the other entries */
			if (dwLength <= 4)
			{
				if (RegOpenKeyEx(hkNewSchemes, g_ThemeTemplates[iTemplateIndex].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
				{
					if (RegOpenKeyEx(hkScheme, TEXT("Sizes"), 0, KEY_READ, &hkSizes) == ERROR_SUCCESS)
					{
						iSize = 0;
						dwLength = 3;
						while((RegEnumKeyEx(hkSizes, iSize, g_ThemeTemplates[iTemplateIndex].strSizeName, &dwLength,
											NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iSize <= 4))
						{
							if(RegOpenKeyEx(hkSizes, g_ThemeTemplates[iTemplateIndex].strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
							{
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("DisplayName"), NULL, &dwType, (LPBYTE)&g_ThemeTemplates[iTemplateIndex].strDisplayName, &dwLength);
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("LegacyName"), NULL, &dwType, (LPBYTE)&g_ThemeTemplates[iTemplateIndex].strLegacyName, &dwLength);
								RegCloseKey(hkSize);
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
		g_PresetLoaded = TRUE;
		g_TemplateCount = iTemplateIndex;
	}
	return iTemplateIndex;
}
