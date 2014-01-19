/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/theme.c
 * PURPOSE:         Handling themes
 *
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "desk.h"

#include <shlwapi.h>
#include <uxtheme.h>
#include <uxundoc.h>
#include <vssym32.h>

static const WCHAR g_CPColors[] = L"Control Panel\\Colors";
static const WCHAR g_CPANewSchemes[] = L"Control Panel\\Appearance\\New Schemes";
static const WCHAR g_CPMetrics[] = L"Control Panel\\Desktop\\WindowMetrics";
static const WCHAR g_SelectedStyle[] = L"SelectedStyle";

/******************************************************************************/

/* This is the list of names for the colors stored in the registry */
static const WCHAR *g_RegColorNames[NUM_COLORS] = {
    L"Scrollbar",             /* 00 = COLOR_SCROLLBAR */
    L"Background",            /* 01 = COLOR_DESKTOP */
    L"ActiveTitle",           /* 02 = COLOR_ACTIVECAPTION  */
    L"InactiveTitle",         /* 03 = COLOR_INACTIVECAPTION */
    L"Menu",                  /* 04 = COLOR_MENU */
    L"Window",                /* 05 = COLOR_WINDOW */
    L"WindowFrame",           /* 06 = COLOR_WINDOWFRAME */
    L"MenuText",              /* 07 = COLOR_MENUTEXT */
    L"WindowText",            /* 08 = COLOR_WINDOWTEXT */
    L"TitleText",             /* 09 = COLOR_CAPTIONTEXT */
    L"ActiveBorder",          /* 10 = COLOR_ACTIVEBORDER */
    L"InactiveBorder",        /* 11 = COLOR_INACTIVEBORDER */
    L"AppWorkSpace",          /* 12 = COLOR_APPWORKSPACE */
    L"Hilight",               /* 13 = COLOR_HIGHLIGHT */
    L"HilightText",           /* 14 = COLOR_HIGHLIGHTTEXT */
    L"ButtonFace",            /* 15 = COLOR_BTNFACE */
    L"ButtonShadow",          /* 16 = COLOR_BTNSHADOW */
    L"GrayText",              /* 17 = COLOR_GRAYTEXT */
    L"ButtonText",            /* 18 = COLOR_BTNTEXT */
    L"InactiveTitleText",     /* 19 = COLOR_INACTIVECAPTIONTEXT */
    L"ButtonHilight",         /* 20 = COLOR_BTNHIGHLIGHT */
    L"ButtonDkShadow",        /* 21 = COLOR_3DDKSHADOW */
    L"ButtonLight",           /* 22 = COLOR_3DLIGHT */
    L"InfoText",              /* 23 = COLOR_INFOTEXT */
    L"InfoWindow",            /* 24 = COLOR_INFOBK */
    L"ButtonAlternateFace",   /* 25 = COLOR_ALTERNATEBTNFACE */
    L"HotTrackingColor",      /* 26 = COLOR_HOTLIGHT */
    L"GradientActiveTitle",   /* 27 = COLOR_GRADIENTACTIVECAPTION */
    L"GradientInactiveTitle", /* 28 = COLOR_GRADIENTINACTIVECAPTION */
    L"MenuHilight",           /* 29 = COLOR_MENUHILIGHT */
    L"MenuBar",               /* 30 = COLOR_MENUBAR */
};

/******************************************************************************/

VOID 
SchemeSetMetric(IN COLOR_SCHEME *scheme, int id, int value)
{
    switch(id)
    {
        case SIZE_BORDER_WIDTH: scheme->ncMetrics.iBorderWidth = value; break;
        case SIZE_SCROLL_WIDTH: scheme->ncMetrics.iScrollWidth = value; break;
        case SIZE_SCROLL_HEIGHT: scheme->ncMetrics.iScrollHeight = value; break;
        case SIZE_CAPTION_WIDTH: scheme->ncMetrics.iCaptionWidth = value; break;
        case SIZE_CAPTION_HEIGHT: scheme->ncMetrics.iCaptionHeight = value; break;
        case SIZE_SM_CAPTION_WIDTH: scheme->ncMetrics.iSmCaptionWidth = value; break;
        case SIZE_SM_CAPTION_HEIGHT: scheme->ncMetrics.iSmCaptionHeight = value; break;
        case SIZE_MENU_WIDTH: scheme->ncMetrics.iMenuWidth = value; break;
        case SIZE_MENU_HEIGHT: scheme->ncMetrics.iMenuHeight = value; break;
        case SIZE_ICON: scheme->iIconSize = value; break;
        case SIZE_ICON_SPACE_X: scheme->icMetrics.iHorzSpacing = value; break;
        case SIZE_ICON_SPACE_Y: scheme->icMetrics.iVertSpacing = value; break;
    }
}

int 
SchemeGetMetric(IN COLOR_SCHEME *scheme, int id)
{
    switch (id)
    {
        case SIZE_BORDER_WIDTH: return scheme->ncMetrics.iBorderWidth;
        case SIZE_SCROLL_WIDTH: return scheme->ncMetrics.iScrollWidth;
        case SIZE_SCROLL_HEIGHT: return scheme->ncMetrics.iScrollHeight;
        case SIZE_CAPTION_WIDTH: return scheme->ncMetrics.iCaptionWidth; 
        case SIZE_CAPTION_HEIGHT: return scheme->ncMetrics.iCaptionHeight; 
        case SIZE_SM_CAPTION_WIDTH: return scheme->ncMetrics.iSmCaptionWidth; 
        case SIZE_SM_CAPTION_HEIGHT: return scheme->ncMetrics.iSmCaptionHeight; 
        case SIZE_MENU_WIDTH: return scheme->ncMetrics.iMenuWidth; 
        case SIZE_MENU_HEIGHT: return scheme->ncMetrics.iMenuHeight; 
        case SIZE_ICON: return scheme->iIconSize;
        case SIZE_ICON_SPACE_X: return scheme->icMetrics.iHorzSpacing;
        case SIZE_ICON_SPACE_Y: return scheme->icMetrics.iVertSpacing;
    }
    return 0;
}

PLOGFONTW
SchemeGetFont(IN COLOR_SCHEME *scheme, int id)
{
    switch (id)
    {
        case FONT_CAPTION: return &scheme->ncMetrics.lfCaptionFont;
        case FONT_SMCAPTION: return &scheme->ncMetrics.lfSmCaptionFont;
        case FONT_MENU: return &scheme->ncMetrics.lfMenuFont;
        case FONT_STATUS: return &scheme->ncMetrics.lfStatusFont;
        case FONT_MESSAGE: return &scheme->ncMetrics.lfMessageFont;
        case FONT_ICON: return &scheme->icMetrics.lfFont;
    }
    return NULL;
}

/*
 * LoadCurrentScheme: Populates the passed scheme based on the current system settings
 */
BOOL
LoadCurrentScheme(OUT COLOR_SCHEME *scheme)
{
    INT i, Result;
    HKEY hKey;
    BOOL ret;

    /* Load colors */
    for (i = 0; i < NUM_COLORS; i++)
    {
        scheme->crColor[i] = (COLORREF)GetSysColor(i);
    }

    /* Load non client metrics */
    scheme->ncMetrics.cbSize = sizeof(NONCLIENTMETRICSW);
    ret = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 
                                sizeof(NONCLIENTMETRICSW), 
                                &scheme->ncMetrics, 
                                0);
    if (!ret) return FALSE;

    /* Load icon metrics */
    scheme->icMetrics.cbSize = sizeof(ICONMETRICSW);
    ret = SystemParametersInfoW(SPI_GETICONMETRICS, 
                                sizeof(ICONMETRICSW), 
                                &scheme->icMetrics, 
                                0);
    if (!ret) return FALSE;

    /* Load flat menu style */
    ret = SystemParametersInfoW(SPI_GETFLATMENU, 
                                0, 
                                &scheme->bFlatMenus, 
                                0);
    if (!ret) return FALSE;

    /* Effects */
    /* "Use the following transition effect for menus and tooltips" */
    ret = SystemParametersInfoW(SPI_GETMENUANIMATION, 
                                0, 
                                &scheme->Effects.bMenuAnimation, 
                                0);
    if (!ret) return FALSE;

    ret = SystemParametersInfoW(SPI_GETMENUFADE, 
                                0, 
                                &scheme->Effects.bMenuFade, 
                                0);
    if (!ret) return FALSE;

    /* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
     * Just keep them in sync for now:
     */
    scheme->Effects.bTooltipAnimation  = scheme->Effects.bMenuAnimation;
    scheme->Effects.bTooltipFade       = scheme->Effects.bMenuFade;

    /* Show content of windows during dragging */
    ret = SystemParametersInfoW(SPI_GETDRAGFULLWINDOWS, 
                                0, 
                                &scheme->Effects.bDragFullWindows, 
                                0);
    if (!ret) return FALSE;

    /* "Hide underlined letters for keyboard navigation until I press the Alt key" */
    ret = SystemParametersInfoW(SPI_GETKEYBOARDCUES, 
                                0, 
                                &scheme->Effects.bKeyboardCues, 
                                0);
    if (!ret) return FALSE;

    /* Read the icon size from registry */
    Result = RegOpenKeyW(HKEY_CURRENT_USER, g_CPMetrics, &hKey);
    if(Result == ERROR_SUCCESS)
    {
        scheme->iIconSize = SHRegGetIntW(hKey, L"Shell Icon Size", 32);
        RegCloseKey(hKey);
    }

    return TRUE;
}

/*
 * LoadSchemeFromReg: Populates the passed scheme with values retireved from registry 
 */
BOOL
LoadSchemeFromReg(OUT COLOR_SCHEME *scheme, IN PTHEME_SELECTION pSelectedTheme)
{
    INT i;
    WCHAR strValueName[10], strSchemeKey[MAX_PATH];
    HKEY hkScheme = NULL;
    DWORD dwType, dwLength;
    UINT64 iSize;
    BOOL Ret = TRUE;
    LONG result;

    wsprintf(strSchemeKey, L"%s\\%s\\Sizes\\%s", 
             g_CPANewSchemes, 
             pSelectedTheme->Color->StyleName, 
             pSelectedTheme->Size->StyleName);

    result = RegOpenKeyW(HKEY_CURRENT_USER, strSchemeKey, &hkScheme);
    if (result != ERROR_SUCCESS) return FALSE;

    scheme->bFlatMenus = SHRegGetIntW(hkScheme, L"FlatMenus", 0);

    for (i = 0; i < NUM_COLORS; i++)
    {
        wsprintf(strValueName, L"Color #%d", i);
        dwLength = sizeof(COLORREF);
        result = RegQueryValueExW(hkScheme, 
                                  strValueName, 
                                  NULL, 
                                  &dwType, 
                                  (LPBYTE)&scheme->crColor[i], 
                                  &dwLength);
        if (result != ERROR_SUCCESS || dwType != REG_DWORD)
        {
            /* Failed to read registry value, initialize with current setting for now */
            scheme->crColor[i] = GetSysColor(i);
        }
    }

    for (i = 0; i < NUM_FONTS; i++)
    {
        PLOGFONTW lpfFont = SchemeGetFont(scheme, i);

        wsprintf(strValueName, L"Font #%d", i);
        dwLength = sizeof(LOGFONT);
        result = RegQueryValueExW(hkScheme, 
                                  strValueName, 
                                  NULL, 
                                  &dwType, 
                                  (LPBYTE)lpfFont, 
                                  &dwLength);
        if (result != ERROR_SUCCESS || dwType != REG_BINARY || 
            dwLength != sizeof(LOGFONT))
        {
            /* Failed to read registry value */
            Ret = FALSE;
        }
    }

    for (i = 0; i < NUM_SIZES; i++)
    {
        wsprintf(strValueName, L"Size #%d", i);
        dwLength = sizeof(UINT64);
        result = RegQueryValueExW(hkScheme, 
                                  strValueName, 
                                  NULL, 
                                  &dwType, 
                                  (LPBYTE)&iSize, 
                                  &dwLength);
        if (result != ERROR_SUCCESS || dwType != REG_QWORD || 
            dwLength != sizeof(UINT64))
        {
            /* Failed to read registry value, initialize with current setting for now */
        }
        else
        {
            SchemeSetMetric(scheme, i, (int)iSize);
        }
    }

    RegCloseKey(hkScheme);
    
    return Ret;
}

/*
    ApplyScheme: Applies the selected scheme and stores its id in the registry if needed
 */
VOID
ApplyScheme(IN COLOR_SCHEME *scheme, IN PTHEME_SELECTION pSelectedTheme)
{
    INT i, Result;
    HKEY hKey;
    WCHAR clText[16], *StyleName;
    INT ColorList[NUM_COLORS];

    /* Apply system colors  */
    for (i = 0; i < NUM_COLORS; i++)
        ColorList[i] = i;
    SetSysColors(NUM_COLORS, ColorList, scheme->crColor);

    /* Save colors to registry */
    Result = RegCreateKeyW(HKEY_CURRENT_USER, g_CPColors, &hKey);
    if (Result == ERROR_SUCCESS)
    {
        for (i = 0; i < NUM_COLORS; i++)
        {
            wsprintf(clText, 
                     L"%d %d %d", 
                     GetRValue(scheme->crColor[i]), 
                     GetGValue(scheme->crColor[i]), 
                     GetBValue(scheme->crColor[i]));
            
            RegSetValueExW(hKey, 
                           g_RegColorNames[i], 
                           0, 
                           REG_SZ, 
                           (BYTE *)clText, 
                           (lstrlen(clText) + 1) * sizeof(WCHAR));
        }
        RegCloseKey(hKey);
    }

    /* Apply non client metrics */
    SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, 
                          sizeof(NONCLIENTMETRICS), 
                          &scheme->ncMetrics, 
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    /* Apply icon metrics */
    SystemParametersInfoW(SPI_SETICONMETRICS, 
                          sizeof(ICONMETRICS), 
                          &scheme->icMetrics, 
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    /* Effects, save only when needed: */
    /* FIXME: XP seems to use grayed checkboxes to reflect differences between menu and tooltips settings
     * Just keep them in sync for now.
     */
    scheme->Effects.bTooltipAnimation  = scheme->Effects.bMenuAnimation;
    scheme->Effects.bTooltipFade = scheme->Effects.bMenuFade;
    SystemParametersInfoW(SPI_SETDRAGFULLWINDOWS, scheme->Effects.bDragFullWindows, (PVOID)&scheme->Effects.bDragFullWindows, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    SystemParametersInfoW(SPI_SETKEYBOARDCUES, 0, IntToPtr(scheme->Effects.bKeyboardCues), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    //SystemParametersInfoW(SPI_SETACTIVEWINDOWTRACKING, 0, (PVOID)&scheme->Effects.bActiveWindowTracking, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETMENUANIMATION, 0, (PVOID)&scheme->Effects.bMenuAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETCOMBOBOXANIMATION, 0, (PVOID)&scheme->Effects.bComboBoxAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (PVOID)&scheme->Effects.bListBoxSmoothScrolling, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)&scheme->Effects.bGradientCaptions, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETACTIVEWNDTRKZORDER, 0, (PVOID)&scheme->Effects.bActiveWndTrkZorder, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETHOTTRACKING, 0, (PVOID)&scheme->Effects.bHotTracking, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETMENUFADE, 0, (PVOID)&scheme->Effects.bMenuFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETSELECTIONFADE, 0, (PVOID)&scheme->Effects.bSelectionFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETTOOLTIPANIMATION, 0, (PVOID)&scheme->Effects.bTooltipAnimation, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    SystemParametersInfoW(SPI_SETTOOLTIPFADE, 0, (PVOID)&scheme->Effects.bTooltipFade, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETCURSORSHADOW, 0, (PVOID)&scheme->Effects.bCursorShadow, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    //SystemParametersInfoW(SPI_SETUIEFFECTS, 0, (PVOID)&scheme->Effects.bUiEffects, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

    /* Save SchemeId in the registry */
    if (pSelectedTheme->Theme != NULL && pSelectedTheme->ThemeActive == FALSE)
    {
        StyleName = pSelectedTheme->Color->StyleName;
        SHSetValueW(HKEY_CURRENT_USER, 
                    g_CPANewSchemes,
                    g_SelectedStyle, 
                    REG_SZ, 
                    StyleName, 
                    (lstrlenW(StyleName) + 1) * sizeof(WCHAR));
    }
}

static THEME*
CreateTheme(LPCWSTR pszName, LPCWSTR pszDisplayName)
{
    PTHEME pTheme;

    pTheme = (PTHEME) malloc(sizeof(THEME));
    if (pTheme == NULL) return NULL;
        
    pTheme->DisplayName = _wcsdup(pszDisplayName);
    if (pTheme->DisplayName == NULL)
    {
        free(pTheme);
        return NULL;
    }

    pTheme->ColoursList = NULL;
    pTheme->NextTheme = NULL;
    pTheme->SizesList = NULL;

    if (pszName == NULL)
    {
        pTheme->ThemeFileName = NULL;
        return pTheme;
    }

    pTheme->ThemeFileName = _wcsdup(pszName);
    if (pTheme->ThemeFileName == NULL)
    {
        free(pTheme->DisplayName);
        free(pTheme);
        return NULL;
    }
    
    return pTheme;
}

static PTHEME_STYLE
CreateStyle(LPCWSTR pszName, LPCWSTR pszDisplayName)
{
    PTHEME_STYLE pStyle;

    pStyle = (PTHEME_STYLE) malloc(sizeof(THEME_STYLE));
    if (pStyle == NULL) return NULL;

    pStyle->StyleName = _wcsdup(pszName);
    if (pStyle->StyleName == NULL)
    {
        free(pStyle);
        return NULL;
    }

    pStyle->DisplayName = _wcsdup(pszDisplayName);
    if (pStyle->DisplayName == NULL)
    {
        free(pStyle->StyleName);
        free(pStyle);
        return NULL;
    }

    pStyle->ChildStyle = NULL;
    pStyle->NextStyle = NULL;

    return pStyle;
}

static void
CleanupStyles(IN PTHEME_STYLE pStylesList)
{
    PTHEME_STYLE pStyle, pStyleOld;

    pStyle = pStylesList;
    while (pStyle)
    {
        if (pStyle->ChildStyle) CleanupStyles(pStyle->ChildStyle);
        if (pStyle->DisplayName) free(pStyle->DisplayName);
        if (pStyle->StyleName) free(pStyle->StyleName);
        
        pStyleOld = pStyle;
        pStyle = pStyle->NextStyle;
        free(pStyleOld);
    }
}

void
CleanupThemes(IN PTHEME pThemeList)
{
    PTHEME pTheme, pThemeOld;

    pTheme = pThemeList;
    while (pTheme)
    {
        CleanupStyles(pTheme->ColoursList);
        if (pTheme->SizesList) CleanupStyles(pTheme->SizesList);
        if (pTheme->DisplayName) free(pTheme->DisplayName);
        if (pTheme->ThemeFileName) free(pTheme->ThemeFileName);

        pThemeOld = pTheme;
        pTheme = pTheme->NextTheme;
        free(pThemeOld);
    }
}

static PTHEME_STYLE 
FindStyle(IN PTHEME_STYLE pStylesList, IN PCWSTR StyleName)
{
    PTHEME_STYLE pStyle;

    for (pStyle = pStylesList; pStyle; pStyle = pStyle->NextStyle)
    {
        if (_wcsicmp(pStyle->StyleName, StyleName) == 0)
        {
            return pStyle;
        }
    }

    /* If we can't find the style requested, return the first one */
    return pStylesList;
}

/*
 * LoadSchemeSizes: Returns a list of sizes from the registry key of a scheme
 */
static PTHEME_STYLE 
LoadSchemeSizes(IN HKEY hkScheme)
{
    HKEY hkSizes, hkSize;
    INT Result;
    INT iStyle;
    WCHAR wstrSizeName[5], wstrDisplayName[50];
    THEME_STYLE *List = NULL, *pCurrentStyle;

    Result = RegOpenKeyW(hkScheme, L"Sizes",  &hkSizes);
    if (Result != ERROR_SUCCESS) return NULL;

    iStyle = 0;
    while ((RegEnumKeyW(hkSizes, iStyle, wstrSizeName, 5) == ERROR_SUCCESS))
    {
        iStyle++;

        Result = RegOpenKeyW(hkSizes, wstrSizeName, &hkSize);
        if (Result != ERROR_SUCCESS) continue;
        
        Result = RegLoadMUIStringW(hkSize, 
                                   L"DisplayName", 
                                   wstrDisplayName, 
                                   sizeof(wstrDisplayName), 
                                   NULL, 
                                   0, 
                                   NULL);
        if (Result != ERROR_SUCCESS)
        {
            Result = RegLoadMUIStringW(hkSize, 
                                       L"LegacyName", 
                                       wstrDisplayName, 
                                       sizeof(wstrDisplayName), 
                                       NULL, 
                                       0, 
                                       NULL);
        }
        
        if (Result == ERROR_SUCCESS)
            pCurrentStyle = CreateStyle(wstrSizeName, wstrDisplayName);
        else
            pCurrentStyle = CreateStyle(wstrSizeName, wstrSizeName);

        if (pCurrentStyle  != NULL)
        {
            pCurrentStyle->NextStyle = List;
            List = pCurrentStyle;
        }

        RegCloseKey(hkSize);
    }

    RegCloseKey(hkSizes);
    return List;
}

/*
    LoadClassicColorSchemes: Returns a list of classic theme colours from the registry key of a scheme
*/
static THEME_STYLE* 
LoadClassicColorSchemes(VOID)
{
    INT Result;
    HKEY hkNewSchemes, hkScheme;
    INT iStyle;
    WCHAR wstrStyleName[5], wstrDisplayName[50];
    THEME_STYLE *List = NULL, *pCurrentStyle;

    Result = RegOpenKeyW(HKEY_CURRENT_USER, g_CPANewSchemes,  &hkNewSchemes);
    if (Result != ERROR_SUCCESS) return NULL;

    iStyle = 0;
    while ((RegEnumKeyW(hkNewSchemes, iStyle, wstrStyleName, 5) == ERROR_SUCCESS))
    {
        iStyle++;

        Result = RegOpenKeyW(hkNewSchemes, wstrStyleName,  &hkScheme);
        if (Result != ERROR_SUCCESS) continue;
        
        Result = RegLoadMUIStringW(hkScheme, 
                                   L"DisplayName",
                                   wstrDisplayName, 
                                   sizeof(wstrDisplayName), 
                                   NULL, 
                                   0, 
                                   NULL);
        if (Result != ERROR_SUCCESS)
        {
            Result = RegLoadMUIStringW(hkScheme, 
                                       L"LegacyName", 
                                       wstrDisplayName, 
                                       sizeof(wstrDisplayName), 
                                       NULL, 
                                       0, 
                                       NULL);
        }

        if (Result == ERROR_SUCCESS)
            pCurrentStyle = CreateStyle(wstrStyleName, wstrDisplayName);
        else
            pCurrentStyle = CreateStyle(wstrStyleName, wstrStyleName);

        if (pCurrentStyle != NULL)
        {
            pCurrentStyle->NextStyle = List;
            pCurrentStyle->ChildStyle = LoadSchemeSizes(hkScheme);
            if(pCurrentStyle->ChildStyle == NULL)
                CleanupStyles(pCurrentStyle);
            else
                List = pCurrentStyle;
        }

        RegCloseKey(hkScheme);
    }

    RegCloseKey(hkNewSchemes);
    return List;
}

typedef HRESULT (WINAPI *ENUMTHEMESTYLE) (LPCWSTR, LPWSTR, DWORD, PTHEMENAMES);

static THEME_STYLE* 
EnumThemeStyles(IN LPCWSTR pszThemeFileName, IN ENUMTHEMESTYLE pfnEnumTheme)
{
    DWORD index = 0;
    THEMENAMES names;
    THEME_STYLE *List = NULL, **ppPrevStyle, *pCurrentStyle;

    ppPrevStyle = &List;

    while (SUCCEEDED(pfnEnumTheme (pszThemeFileName, NULL, index++, &names)))
    {
        pCurrentStyle = CreateStyle(names.szName, names.szDisplayName);
        if(pCurrentStyle == NULL) break;

        *ppPrevStyle = pCurrentStyle;
        ppPrevStyle = &pCurrentStyle->NextStyle;
    }

    return List;
}

BOOL CALLBACK 
EnumThemeProc(IN LPVOID lpReserved, 
              IN LPCWSTR pszThemeFileName,
              IN LPCWSTR pszThemeName, 
              IN LPCWSTR pszToolTip, 
              IN LPVOID lpReserved2,
              IN OUT LPVOID lpData)
{
    PTHEME *List, pTheme;

    List = (PTHEME*)lpData;

    pTheme = CreateTheme(pszThemeFileName, pszThemeName);
    if (pTheme == NULL) return FALSE;

    pTheme->SizesList = EnumThemeStyles( pszThemeFileName, (ENUMTHEMESTYLE)EnumThemeSizes);
    pTheme->ColoursList = EnumThemeStyles( pszThemeFileName, (ENUMTHEMESTYLE)EnumThemeColors);
    if(pTheme->SizesList == NULL || pTheme->ColoursList == NULL)
    {
        CleanupThemes(pTheme);
        return FALSE;
    }

    pTheme->NextTheme = *List;
    *List = pTheme;

    return TRUE;
}

/* 
    LoadThemes: Returns a list that contains tha classic theme and 
                 the visual styles of the system
*/
PTHEME
LoadThemes()
{
    HRESULT hret;
    PTHEME pClassicTheme;
    WCHAR strClassicTheme[40];
    WCHAR szThemesPath[MAX_PATH], *pszClassicTheme;
    int res;

    /* Insert the classic theme */
    res = LoadString(hApplet, IDS_CLASSIC_THEME, strClassicTheme, 40);
    pszClassicTheme = (res > 0 ? strClassicTheme : L"Classic Theme");
    pClassicTheme = CreateTheme(NULL, pszClassicTheme);
    if (pClassicTheme == NULL) return NULL;
    pClassicTheme->ColoursList = LoadClassicColorSchemes();

    /* Get path to themes folder */
    ZeroMemory(szThemesPath, sizeof(szThemesPath));
    hret = SHGetFolderPathW (NULL, CSIDL_RESOURCES, NULL, SHGFP_TYPE_DEFAULT, szThemesPath);
    if (FAILED(hret)) return pClassicTheme;
    lstrcatW (szThemesPath, L"\\Themes");

    /* Enumerate themes */
    hret = EnumThemes( szThemesPath, EnumThemeProc, &pClassicTheme->NextTheme);
    if (FAILED(hret))
    {
        pClassicTheme->NextTheme = NULL;
        if (pClassicTheme->ColoursList == NULL)
        {
            free(pClassicTheme->DisplayName);
            free(pClassicTheme);
            return NULL;
        }
    }

    return pClassicTheme;
}

/* 
 *  GetActiveTheme: Gets the active theme and populates pSelectedTheme 
 *                   with entries from the list of loaded themes 
 */
BOOL
GetActiveTheme(IN PTHEME pThemeList, OUT PTHEME_SELECTION pSelectedTheme)
{
    WCHAR szThemeFileName[MAX_PATH];
    WCHAR szColorBuff[MAX_PATH];
    WCHAR szSizeBuff[MAX_PATH];
    PTHEME pTheme;
    HRESULT hret;
    
    ZeroMemory(pSelectedTheme, sizeof(THEME_SELECTION));

    /* Retrieve the name of the current theme */
    hret = GetCurrentThemeName(szThemeFileName, 
                               MAX_PATH, 
                               szColorBuff, 
                               MAX_PATH, 
                               szSizeBuff, 
                               MAX_PATH);
    if (FAILED(hret))  return FALSE;

    for (pTheme = pThemeList; pTheme; pTheme = pTheme->NextTheme)
    {
        if(pTheme->ThemeFileName && 
           _wcsicmp(pTheme->ThemeFileName, szThemeFileName) == 0)
        {
            break;
        }
    }

    if (pTheme == NULL) return FALSE;

    pSelectedTheme->ThemeActive = TRUE;
    pSelectedTheme->Theme = pTheme;
    pSelectedTheme->Color = FindStyle(pTheme->ColoursList, szColorBuff);
    pSelectedTheme->Size = FindStyle(pTheme->SizesList, szSizeBuff);

    return TRUE;
}

/* 
    GetActiveTheme: Gets the active classic theme and populates pSelectedTheme 
                    with entries from the list of loaded themes 
*/
BOOL
GetActiveClassicTheme(IN PTHEME pThemeList, OUT PTHEME_SELECTION pSelectedTheme)
{
    INT Result;
    WCHAR szSelectedClassicScheme[5], szSelectedClassicSize[5];
    HKEY hkNewSchemes;
    DWORD dwType, dwDisplayNameLength;
    PTHEME_STYLE pCurrentStyle, pCurrentSize;

    ZeroMemory(pSelectedTheme, sizeof(THEME_SELECTION));
    
    /* Assume failure */
    szSelectedClassicScheme[0] = 0;
    szSelectedClassicSize[0] = 0;

    Result = RegOpenKeyW(HKEY_CURRENT_USER, g_CPANewSchemes, &hkNewSchemes);
    if (Result != ERROR_SUCCESS) return FALSE;

    dwType = REG_SZ;
    dwDisplayNameLength = sizeof(szSelectedClassicScheme);
    Result = RegQueryValueEx(hkNewSchemes, L"SelectedStyle", NULL, &dwType, 
                             (LPBYTE)&szSelectedClassicScheme, &dwDisplayNameLength);
    if (Result == ERROR_SUCCESS)
    {
        dwType = REG_SZ;
        dwDisplayNameLength = sizeof(szSelectedClassicSize);
        Result = SHGetValue(hkNewSchemes, szSelectedClassicScheme, L"SelectedSize",
                            &dwType, szSelectedClassicSize, &dwDisplayNameLength);
    }

    RegCloseKey(hkNewSchemes);

    pCurrentStyle = FindStyle(pThemeList->ColoursList, szSelectedClassicScheme);
    pCurrentSize = FindStyle(pCurrentStyle->ChildStyle, szSelectedClassicSize);

    pSelectedTheme->Theme = pThemeList;
    pSelectedTheme->Color = pCurrentStyle;
    pSelectedTheme->Size = pCurrentSize;

    return TRUE;
}

BOOL
ActivateTheme(IN PTHEME_SELECTION pSelectedTheme)
{
    HTHEMEFILE hThemeFile = 0;
    HRESULT hret;

    if (pSelectedTheme->ThemeActive)
    {
        hret = OpenThemeFile(pSelectedTheme->Theme->ThemeFileName, 
                             pSelectedTheme->Color->StyleName, 
                             pSelectedTheme->Size->StyleName, 
                             &hThemeFile, 
                             0);

        if (!SUCCEEDED(hret)) return FALSE;
    }

    hret = ApplyTheme(hThemeFile, "", 0);

    if (pSelectedTheme->ThemeActive)
    {
       CloseThemeFile(hThemeFile);
    }

    return SUCCEEDED(hret);
}

BOOL
LoadSchemeFromTheme(OUT PCOLOR_SCHEME scheme, IN PTHEME_SELECTION pSelectedTheme)
{
    HTHEMEFILE hThemeFile = 0;
    HRESULT hret;
    HTHEME hTheme;
    int i;

    hret = OpenThemeFile(pSelectedTheme->Theme->ThemeFileName, 
                         pSelectedTheme->Color->StyleName, 
                         pSelectedTheme->Size->StyleName, 
                         &hThemeFile, 
                         0);

    if (!SUCCEEDED(hret)) return FALSE;
  
    hTheme = OpenThemeDataFromFile(hThemeFile, hCPLWindow, L"WINDOW", 0);
    if (hTheme == NULL) return FALSE;

    /* Load colors */
    for (i = 0; i < NUM_COLORS; i++)
    {
        scheme->crColor[i] = GetThemeSysColor(hTheme,i);
    }

    /* Load sizes */
    /* I wonder why GetThemeSysInt doesn't work here */
    scheme->ncMetrics.iBorderWidth = GetThemeSysSize(hTheme, SM_CXFRAME);
    scheme->ncMetrics.iScrollWidth = GetThemeSysSize(hTheme, SM_CXVSCROLL);
    scheme->ncMetrics.iScrollHeight = GetThemeSysSize(hTheme, SM_CYHSCROLL);
    scheme->ncMetrics.iCaptionWidth = GetThemeSysSize(hTheme, SM_CXSIZE);
    scheme->ncMetrics.iCaptionHeight = GetThemeSysSize(hTheme, SM_CYSIZE);
    scheme->ncMetrics.iSmCaptionWidth = GetThemeSysSize(hTheme, SM_CXSMSIZE);
    scheme->ncMetrics.iSmCaptionHeight = GetThemeSysSize(hTheme, SM_CYSMSIZE);
    scheme->ncMetrics.iMenuWidth = GetThemeSysSize(hTheme, SM_CXMENUSIZE);
    scheme->ncMetrics.iMenuHeight = GetThemeSysSize(hTheme, SM_CYMENUSIZE);

    /* Load fonts */
    GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &scheme->ncMetrics.lfCaptionFont);
    GetThemeSysFont(hTheme, TMT_SMALLCAPTIONFONT, &scheme->ncMetrics.lfSmCaptionFont);
    GetThemeSysFont(hTheme, TMT_MENUFONT, &scheme->ncMetrics.lfMenuFont );
    GetThemeSysFont(hTheme, TMT_STATUSFONT, &scheme->ncMetrics.lfStatusFont);
    GetThemeSysFont(hTheme, TMT_MSGBOXFONT, &scheme->ncMetrics.lfMessageFont);
    GetThemeSysFont(hTheme, TMT_ICONTITLEFONT, &scheme->icMetrics.lfFont);
    
    CloseThemeData(hTheme);

    return TRUE;
}

BOOL
DrawThemePreview(IN HDC hdcMem, IN PCOLOR_SCHEME scheme, IN PTHEME_SELECTION pSelectedTheme, IN PRECT prcWindow)
{
    HBRUSH hbrBack;
    HRESULT hres;

    hbrBack = CreateSolidBrush(scheme->crColor[COLOR_DESKTOP]);

    FillRect(hdcMem, prcWindow, hbrBack);
    DeleteObject(hbrBack);

    InflateRect(prcWindow, -10, -10);

    hres = DrawNCPreview(hdcMem, 
                         DNCP_DRAW_ALL,
                         prcWindow,
                         pSelectedTheme->Theme->ThemeFileName, 
                         pSelectedTheme->Color->StyleName, 
                         pSelectedTheme->Size->StyleName, 
                         &scheme->ncMetrics, 
                         scheme->crColor);

    return SUCCEEDED(hres);
}
