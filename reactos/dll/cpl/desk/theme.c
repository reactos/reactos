/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/theme.c
 * PURPOSE:         Handling themes
 *
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "desk.h"

static BOOL g_PresetLoaded = FALSE;
INT g_TemplateCount = 0;

static INT g_ColorList[NUM_COLORS];

static const TCHAR g_CPColors[] = TEXT("Control Panel\\Colors");
static const TCHAR g_CPANewSchemes[] = TEXT("Control Panel\\Appearance\\New Schemes");
static const TCHAR g_SelectedStyle[] = TEXT("SelectedStyle");

/******************************************************************************/

SCHEME_PRESET g_ColorSchemes[MAX_TEMPLATES];

/* This is the list of names for the colors stored in the registry */
static const TCHAR *g_RegColorNames[NUM_COLORS] =
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
static const int g_SizeMetric[NUM_SIZES] =
{
	SM_CXBORDER,        /* 00: SIZE_BORDER_X */
	SM_CYBORDER,        /* 01: SIZE_BORDER_Y */
	SM_CYSIZE,          /* 02: SIZE_CAPTION_Y */
	SM_CXICON,          /* 03: SIZE_ICON_X */
	SM_CYICON,          /* 04: SIZE_ICON_Y */
	SM_CXICONSPACING,   /* 05: SIZE_ICON_SPC_X */
	SM_CYICONSPACING,   /* 06: SIZE_ICON_SPC_Y */
	SM_CXMENUSIZE,      /* 07: SIZE_MENU_SIZE_X */
	SM_CYMENU,          /* 08: SIZE_MENU_Y */
	SM_CXVSCROLL,       /* 09: SIZE_SCROLL_X */
	SM_CYHSCROLL,       /* 10: SIZE_SCROLL_Y */
	SM_CYSMSIZE,        /* 11: SIZE_SMCAPTION_Y */
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

VOID LoadCurrentScheme(COLOR_SCHEME* scheme)
{
	INT i;
	NONCLIENTMETRICS NonClientMetrics;

	/* Load colors */
	for (i = 0; i < NUM_COLORS; i++)
	{
		g_ColorList[i] = i;
		scheme->crColor[i] = (COLORREF)GetSysColor(i);
	}

	/* Load sizes */
	for (i = 0; i < NUM_SIZES; i++)
	{
		scheme->Size[i] = GetSystemMetrics(g_SizeMetric[i]);
	}

	/* Load fonts */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	scheme->lfFont[FONT_CAPTION] = NonClientMetrics.lfCaptionFont;
	scheme->lfFont[FONT_SMCAPTION] = NonClientMetrics.lfSmCaptionFont;
	scheme->lfFont[FONT_MENU] = NonClientMetrics.lfMenuFont;
	scheme->lfFont[FONT_INFO] = NonClientMetrics.lfStatusFont;
	scheme->lfFont[FONT_DIALOG] = NonClientMetrics.lfMessageFont;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &scheme->lfFont[FONT_ICON], 0);

	/* Effects */
	/* "Use the following transition effect for menus and tooltips" */
	SystemParametersInfo(SPI_GETMENUANIMATION, sizeof(BOOL), &scheme->Effects.bMenuAnimation, 0);
	SystemParametersInfo(SPI_GETMENUFADE, sizeof(BOOL), &scheme->Effects.bMenuFade, 0);
	/* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
	 * Just keep them in sync for now:
	 */
	scheme->Effects.bTooltipAnimation  = scheme->Effects.bMenuAnimation;
	scheme->Effects.bTooltipFade	   = scheme->Effects.bMenuFade;

	/* Show content of windows during dragging */
	SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &scheme->Effects.bDragFullWindows, 0);

	/* "Hide underlined letters for keyboard navigation until I press the Alt key" */
	SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &scheme->Effects.bKeyboardCues, 0);
}

BOOL LoadSchemeFromReg(COLOR_SCHEME* scheme, INT SchemeId)
{
	INT i;
	TCHAR strSelectedStyle[4];
	TCHAR strSizeName[20] = TEXT("Sizes\\0");
	TCHAR strValueName[10];
	HKEY hkNewSchemes, hkScheme, hkSize;
	DWORD dwType, dwLength;
	UINT64 iSize;
	BOOL Ret = FALSE;

	if (!g_PresetLoaded)
		LoadSchemePresetEntries(strSelectedStyle);

	if (SchemeId == -1)
		return FALSE;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, g_CPANewSchemes, 0, KEY_READ, &hkNewSchemes) == ERROR_SUCCESS)
	{
		if (RegOpenKeyEx(hkNewSchemes, g_ColorSchemes[SchemeId].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
		{
			lstrcpyn(&strSizeName[6], g_ColorSchemes[SchemeId].strSizeName, 3);
			if (RegOpenKeyEx(hkScheme, strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
			{
				Ret = TRUE;

				dwLength = sizeof(DWORD);
				if (RegQueryValueEx(hkSize, TEXT("FlatMenus"), NULL, &dwType, (LPBYTE)&scheme->bFlatMenus, &dwLength) != ERROR_SUCCESS ||
					dwType != REG_DWORD)
				{
					/* Failed to read registry value */
					scheme->bFlatMenus = FALSE;
				}

				for (i = 0; i < NUM_COLORS; i++)
				{
					wsprintf(strValueName, TEXT("Color #%d"), i);
					dwLength = sizeof(COLORREF);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&scheme->crColor[i], &dwLength) != ERROR_SUCCESS ||
						dwType != REG_DWORD)
					{
						/* Failed to read registry value, initialize with current setting for now */
						scheme->crColor[i] = GetSysColor(i);
					}
				}

				for (i = 0; i < NUM_FONTS; i++)
				{
					wsprintf(strValueName, TEXT("Font #%d"), i);
					dwLength = sizeof(LOGFONT);
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&scheme->lfFont[i], &dwLength) != ERROR_SUCCESS ||
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
					if (RegQueryValueEx(hkSize, strValueName, NULL, &dwType, (LPBYTE)&iSize, &dwLength) != ERROR_SUCCESS ||
						dwType != REG_QWORD || dwLength != sizeof(UINT64))
					{
						/* Failed to read registry value, initialize with current setting for now */
						scheme->Size[i] = GetSystemMetrics(g_SizeMetric[i]);
					}
					else
						scheme->Size[i] = (INT)iSize;
				}
				RegCloseKey(hkSize);
			}
			RegCloseKey(hkScheme);
		}
		RegCloseKey(hkNewSchemes);
	}

	return Ret;
}

VOID ApplyScheme(COLOR_SCHEME* scheme, INT SchemeId)
{
	INT i, Result;
	HKEY hKey;
	TCHAR clText[16];
	NONCLIENTMETRICS NonClientMetrics;
	ICONMETRICS IconMetrics;

	/* Apply Colors from global variable */
	SetSysColors(NUM_COLORS, g_ColorList, scheme->crColor);

	/* Save colors to registry */
	Result = RegCreateKeyEx(HKEY_CURRENT_USER, g_CPColors, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (Result == ERROR_SUCCESS)
	{
		for (i = 0; i < NUM_COLORS; i++)
		{
			DWORD red   = GetRValue(scheme->crColor[i]);
			DWORD green = GetGValue(scheme->crColor[i]);
			DWORD blue  = GetBValue(scheme->crColor[i]);
			wsprintf(clText, TEXT("%d %d %d"), red, green, blue);
			RegSetValueEx(hKey, g_RegColorNames[i], 0, REG_SZ, (BYTE *)clText, (lstrlen(clText) + 1) * sizeof(TCHAR));
		}
		RegCloseKey(hKey);
	}

	/* Apply non client metrics */
	NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
	NonClientMetrics.lfCaptionFont = scheme->lfFont[FONT_CAPTION];
	NonClientMetrics.lfSmCaptionFont = scheme->lfFont[FONT_SMCAPTION];
	NonClientMetrics.lfMenuFont = scheme->lfFont[FONT_MENU];
	NonClientMetrics.lfStatusFont = scheme->lfFont[FONT_INFO];
	NonClientMetrics.lfMessageFont = scheme->lfFont[FONT_DIALOG];
	NonClientMetrics.iBorderWidth = scheme->Size[SIZE_BORDER_X];
	NonClientMetrics.iScrollWidth = scheme->Size[SIZE_SCROLL_X];
	NonClientMetrics.iScrollHeight = scheme->Size[SIZE_SCROLL_Y];
	NonClientMetrics.iCaptionWidth = scheme->Size[SIZE_CAPTION_Y];
	NonClientMetrics.iCaptionHeight = scheme->Size[SIZE_CAPTION_Y];
	NonClientMetrics.iSmCaptionWidth = scheme->Size[SIZE_SMCAPTION_Y];
	NonClientMetrics.iSmCaptionHeight = scheme->Size[SIZE_SMCAPTION_Y];
	NonClientMetrics.iMenuWidth = scheme->Size[SIZE_MENU_SIZE_X];
	NonClientMetrics.iMenuHeight = scheme->Size[SIZE_MENU_Y];
	SystemParametersInfo(SPI_SETNONCLIENTMETRICS, 
						 sizeof(NONCLIENTMETRICS), 
						 &NonClientMetrics, 
						 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	/* Apply icon metrics */
	IconMetrics.cbSize = sizeof(ICONMETRICS);
	SystemParametersInfo(SPI_GETICONMETRICS, sizeof(ICONMETRICS), &IconMetrics, 0);
	IconMetrics.iHorzSpacing = scheme->Size[SIZE_ICON_SPC_X];
	IconMetrics.iVertSpacing = scheme->Size[SIZE_ICON_SPC_Y];
	IconMetrics.lfFont = scheme->lfFont[FONT_ICON];
	SystemParametersInfo(SPI_SETICONMETRICS, 
						 sizeof(ICONMETRICS), 
						 &IconMetrics, 
						 SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

	/* Effects, save only when needed: */
	/* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
	 * Just keep them in sync for now.
	 */
	scheme->Effects.bTooltipAnimation  = scheme->Effects.bMenuAnimation;
	scheme->Effects.bTooltipFade = scheme->Effects.bMenuFade;
	SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, scheme->Effects.bDragFullWindows, (PVOID)&scheme->Effects.bDragFullWindows, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
	SystemParametersInfo(SPI_SETKEYBOARDCUES, 0, IntToPtr(scheme->Effects.bKeyboardCues), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
	//SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING, 0, (PVOID)&scheme->Effects.bActiveWindowTracking, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETMENUANIMATION, 0, (PVOID)&scheme->Effects.bMenuAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETCOMBOBOXANIMATION, 0, (PVOID)&scheme->Effects.bComboBoxAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (PVOID)&scheme->Effects.bListBoxSmoothScrolling, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)&scheme->Effects.bGradientCaptions, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETACTIVEWNDTRKZORDER, 0, (PVOID)&scheme->Effects.bActiveWndTrkZorder, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETHOTTRACKING, 0, (PVOID)&scheme->Effects.bHotTracking, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETMENUFADE, 0, (PVOID)&scheme->Effects.bMenuFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETSELECTIONFADE, 0, (PVOID)&scheme->Effects.bSelectionFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETTOOLTIPANIMATION, 0, (PVOID)&scheme->Effects.bTooltipAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETTOOLTIPFADE, 0, (PVOID)&scheme->Effects.bTooltipFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETCURSORSHADOW, 0, (PVOID)&scheme->Effects.bCursorShadow, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
	//SystemParametersInfo(SPI_SETUIEFFECTS, 0, (PVOID)&scheme->Effects.bUiEffects, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

	/* Save SchemeId */
	Result = RegOpenKeyEx(HKEY_CURRENT_USER, g_CPANewSchemes, 0, KEY_ALL_ACCESS, &hKey);
	if (Result == ERROR_SUCCESS)
	{
		if (SchemeId == -1)
			clText[0] = TEXT('\0');
		else
			lstrcpy(clText, g_ColorSchemes[SchemeId].strKeyName);
		RegSetValueEx(hKey, g_SelectedStyle, 0, REG_SZ, (BYTE *)clText, (lstrlen(clText) + 1) * sizeof(TCHAR));
		RegCloseKey(hKey);
	}
}

BOOL SaveScheme(COLOR_SCHEME* scheme, LPCTSTR strLegacyName)
{
	/* FIXME: Implement */
	return FALSE;
}

INT LoadSchemePresetEntries(LPTSTR pszSelectedStyle)
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
		while((RegEnumKeyEx(hkNewSchemes, iStyle, g_ColorSchemes[iTemplateIndex].strKeyName, &dwLength,
							NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iTemplateIndex < MAX_TEMPLATES))
		{
			/* Is it really a template or one of the other entries */
			if (dwLength <= 4)
			{
				if (RegOpenKeyEx(hkNewSchemes, g_ColorSchemes[iTemplateIndex].strKeyName, 0, KEY_READ, &hkScheme) == ERROR_SUCCESS)
				{
					if (RegOpenKeyEx(hkScheme, TEXT("Sizes"), 0, KEY_READ, &hkSizes) == ERROR_SUCCESS)
					{
						iSize = 0;
						dwLength = 3;
						while((RegEnumKeyEx(hkSizes, iSize, g_ColorSchemes[iTemplateIndex].strSizeName, &dwLength,
											NULL, NULL, NULL, &ftLastWriteTime) == ERROR_SUCCESS) && (iSize <= 4))
						{
							if(RegOpenKeyEx(hkSizes, g_ColorSchemes[iTemplateIndex].strSizeName, 0, KEY_READ, &hkSize) == ERROR_SUCCESS)
							{
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("DisplayName"), NULL, &dwType, (LPBYTE)&g_ColorSchemes[iTemplateIndex].strDisplayName, &dwLength);
								dwLength = MAX_TEMPLATENAMELENTGH;
								RegQueryValueEx(hkSize, TEXT("LegacyName"), NULL, &dwType, (LPBYTE)&g_ColorSchemes[iTemplateIndex].strLegacyName, &dwLength);
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

typedef HRESULT (WINAPI * ENUMTHEMESTYLE) (LPCWSTR, LPWSTR, DWORD, PTHEMENAMES);

BOOL AddThemeStyles(LPCWSTR pszThemeFileName, HDSA* Styles, int* count,  ENUMTHEMESTYLE enumTheme)
{
    DWORD index = 0;
    THEMENAMES names;
    THEME_STYLE StyleName;

    *Styles = DSA_Create(sizeof(THEMENAMES),1);
    *count = 0;

    while (SUCCEEDED (enumTheme (pszThemeFileName, NULL, index++, &names)))
    {
        StyleName.StlyeName = _wcsdup(names.szName);
        StyleName.DisplayName = _wcsdup(names.szDisplayName);
        (*count)++;
        DSA_InsertItem(*Styles, *count, &StyleName);
    }

    return TRUE;
}

BOOL CALLBACK EnumThemeProc(LPVOID lpReserved, 
                            LPCWSTR pszThemeFileName,
                            LPCWSTR pszThemeName, 
                            LPCWSTR pszToolTip, LPVOID lpReserved2,
                            LPVOID lpData)
{
    THEME theme;
    GLOBALS *g = (GLOBALS *) lpData;

    theme.themeFileName = _wcsdup(pszThemeFileName);
    theme.displayName = _wcsdup(pszThemeName);
    AddThemeStyles( pszThemeFileName, &theme.Sizes, &theme.SizesCount, (ENUMTHEMESTYLE)EnumThemeSizes);
    AddThemeStyles( pszThemeFileName, &theme.Colors, &theme.ColorsCount, (ENUMTHEMESTYLE)EnumThemeColors);

    DSA_InsertItem(g->Themes, DSA_APPEND , &theme);
    g->ThemesCount++;

    return TRUE;
}

void LoadThemes(GLOBALS *g)
{
    WCHAR themesPath[MAX_PATH];
    HRESULT hret;
    THEME ClassicTheme;
    WCHAR szThemeFileName[MAX_PATH];
    WCHAR szColorBuff[MAX_PATH];
    WCHAR szSizeBuff[MAX_PATH];

    /* Initialize themes dsa */
    g->Themes = DSA_Create(sizeof(THEME),5);

    /* Insert the classic theme */
    memset(&ClassicTheme, 0, sizeof(THEME));
    ClassicTheme.displayName = _wcsdup(L"Classic Theme");
    DSA_InsertItem(g->Themes, 0, &ClassicTheme);
    g->ThemesCount = 1;

    /* Retrieve the name of the current theme */
    hret = GetCurrentThemeName(szThemeFileName, 
                               MAX_PATH, 
                               szColorBuff, 
                               MAX_PATH, 
                               szSizeBuff, 
                               MAX_PATH);

    if (FAILED (hret)) 
    {
        g->pszThemeFileName = NULL;
        g->pszColorName = NULL;
        g->pszSizeName = NULL;
    }
    else
    {
        /* Cache the name of the active theme */
        g->pszThemeFileName = _wcsdup(szThemeFileName);
        g->pszColorName = _wcsdup(szColorBuff);
        g->pszSizeName = _wcsdup(szSizeBuff);
    }
    /* Get path to themes folder */
    hret = SHGetFolderPathW (NULL, CSIDL_RESOURCES, NULL, SHGFP_TYPE_CURRENT, themesPath);
    if (FAILED (hret)) 
        return;
    lstrcatW (themesPath, L"\\Themes");

    /* Enumerate themes */
    hret = EnumThemes(themesPath, EnumThemeProc, g);
}

HRESULT ActivateTheme(PTHEME pTheme, int iColor, int iSize)
{
    PTHEME_STYLE pThemeColor;
    PTHEME_STYLE pThemeSize;
    HTHEMEFILE hThemeFile = 0;
    HRESULT hret;

    if(pTheme->themeFileName)
    {
        pThemeColor = (PTHEME_STYLE)DSA_GetItemPtr(pTheme->Colors, iColor);
        pThemeSize = (PTHEME_STYLE)DSA_GetItemPtr(pTheme->Sizes, iSize);

        hret = OpenThemeFile(pTheme->themeFileName, 
                             pThemeColor->StlyeName, 
                             pThemeSize->StlyeName, 
                             &hThemeFile, 
                             0);

        if(!SUCCEEDED(hret))
        {
            return hret;
        }

    }

    hret = ApplyTheme(hThemeFile, "", 0);

    if(pTheme->themeFileName)
    {
        hret = CloseThemeFile(hThemeFile);
    }

    return hret;
}

int CALLBACK CleanUpThemeStlyeCallback(void *p, void *pData)
{
    PTHEME_STYLE pStyle = (PTHEME_STYLE)p;

    free(pStyle->DisplayName);
    free(pStyle->StlyeName);

    return TRUE;
}

int CALLBACK CleanUpThemeCallback(void *p, void *pData)
{
    PTHEME pTheme = (PTHEME)p;

    free(pTheme->displayName);
    free(pTheme->themeFileName);
    DSA_DestroyCallback(pTheme->Colors, CleanUpThemeStlyeCallback, NULL);
    DSA_DestroyCallback(pTheme->Sizes, CleanUpThemeStlyeCallback, NULL);

    return TRUE;
}

void CleanupThemes(GLOBALS *g)
{
    free(g->pszThemeFileName);
    free(g->pszColorName);
    free(g->pszSizeName);

    DSA_DestroyCallback(g->Themes, CleanUpThemeCallback, NULL);
}
