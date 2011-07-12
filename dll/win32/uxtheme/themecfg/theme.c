/*
 * Desktop Integration
 * - Theme configuration code
 * - User Shell Folder mapping
 *
 * Copyright (c) 2005 by Frank Richter
 * Copyright (c) 2006 by Michael Jung
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define COBJMACROS

#include <windows.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "resource.h"

/* UXTHEME functions not in the headers */

typedef struct tagTHEMENAMES
{
    WCHAR szName[MAX_PATH+1];
    WCHAR szDisplayName[MAX_PATH+1];
    WCHAR szTooltip[MAX_PATH+1];
} THEMENAMES, *PTHEMENAMES;

typedef void* HTHEMEFILE;
typedef BOOL (CALLBACK *EnumThemeProc)(LPVOID lpReserved, 
				       LPCWSTR pszThemeFileName,
                                       LPCWSTR pszThemeName, 
				       LPCWSTR pszToolTip, LPVOID lpReserved2,
                                       LPVOID lpData);

HRESULT WINAPI EnumThemeColors (LPCWSTR pszThemeFileName, LPWSTR pszSizeName,
				DWORD dwColorNum, PTHEMENAMES pszColorNames);
HRESULT WINAPI EnumThemeSizes (LPCWSTR pszThemeFileName, LPWSTR pszColorName,
			       DWORD dwSizeNum, PTHEMENAMES pszSizeNames);
HRESULT WINAPI ApplyTheme (HTHEMEFILE hThemeFile, char* unknown, HWND hWnd);
HRESULT WINAPI OpenThemeFile (LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
			      LPCWSTR pszSizeName, HTHEMEFILE* hThemeFile,
			      DWORD unknown);
HRESULT WINAPI CloseThemeFile (HTHEMEFILE hThemeFile);
HRESULT WINAPI EnumThemes (LPCWSTR pszThemePath, EnumThemeProc callback,
                           LPVOID lpData);

/* A struct to keep both the internal and "fancy" name of a color or size */
typedef struct
{
  WCHAR* name;
  WCHAR* fancyName;
} ThemeColorOrSize;

/* wrapper around DSA that also keeps an item count */
typedef struct
{
  HDSA dsa;
  int count;
} WrappedDsa;

static void set_text(HWND dialog, WORD id, const char *text)
{
    SetWindowText(GetDlgItem(dialog, id), text);
}

static char *get_text(HWND dialog, WORD id)
{
    HWND item = GetDlgItem(dialog, id);
    int len = GetWindowTextLength(item) + 1;
    char *result = len ? HeapAlloc(GetProcessHeap(), 0, len) : NULL;
    if (!result || GetWindowText(item, result, len) == 0) return NULL;
    return result;
}

/* Some helper functions to deal with ThemeColorOrSize structs in WrappedDSAs */

static void color_or_size_dsa_add (WrappedDsa* wdsa, const WCHAR* name,
				   const WCHAR* fancyName)
{
    ThemeColorOrSize item;
    
    item.name = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (name) + 1) * sizeof(WCHAR));
    lstrcpyW (item.name, name);

    item.fancyName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (fancyName) + 1) * sizeof(WCHAR));
    lstrcpyW (item.fancyName, fancyName);

    DSA_InsertItem (wdsa->dsa, wdsa->count, &item);
    wdsa->count++;
}

static int CALLBACK dsa_destroy_callback (LPVOID p, LPVOID pData)
{
    ThemeColorOrSize* item = p;
    HeapFree (GetProcessHeap(), 0, item->name);
    HeapFree (GetProcessHeap(), 0, item->fancyName);
    return 1;
}

static void free_color_or_size_dsa (WrappedDsa* wdsa)
{
    DSA_DestroyCallback (wdsa->dsa, dsa_destroy_callback, NULL);
}

static void create_color_or_size_dsa (WrappedDsa* wdsa)
{
    wdsa->dsa = DSA_Create (sizeof (ThemeColorOrSize), 1);
    wdsa->count = 0;
}

static ThemeColorOrSize* color_or_size_dsa_get (WrappedDsa* wdsa, int index)
{
    return DSA_GetItemPtr (wdsa->dsa, index);
}

static int color_or_size_dsa_find (WrappedDsa* wdsa, const WCHAR* name)
{
    int i = 0;
    for (; i < wdsa->count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (wdsa, i);
	if (lstrcmpiW (item->name, name) == 0) break;
    }
    return i;
}

/* A theme file, contains file name, display name, color and size scheme names */
typedef struct
{
    WCHAR* themeFileName;
    WCHAR* fancyName;
    WrappedDsa colors;
    WrappedDsa sizes;
} ThemeFile;

static HDSA themeFiles = NULL;
static int themeFilesCount = 0;

static int CALLBACK theme_dsa_destroy_callback (LPVOID p, LPVOID pData)
{
    ThemeFile* item = p;
    HeapFree (GetProcessHeap(), 0, item->themeFileName);
    HeapFree (GetProcessHeap(), 0, item->fancyName);
    free_color_or_size_dsa (&item->colors);
    free_color_or_size_dsa (&item->sizes);
    return 1;
}

/* Free memory occupied by the theme list */
static void free_theme_files(void)
{
    if (themeFiles == NULL) return;
      
    DSA_DestroyCallback (themeFiles , theme_dsa_destroy_callback, NULL);
    themeFiles = NULL;
    themeFilesCount = 0;
}

typedef HRESULT (WINAPI * EnumTheme) (LPCWSTR, LPWSTR, DWORD, PTHEMENAMES);

/* fill a string list with either colors or sizes of a theme */
static void fill_theme_string_array (const WCHAR* filename, 
				     WrappedDsa* wdsa,
				     EnumTheme enumTheme)
{
    DWORD index = 0;
    THEMENAMES names;

//    WINE_TRACE ("%s %p %p\n", wine_dbgstr_w (filename), wdsa, enumTheme);

    while (SUCCEEDED (enumTheme (filename, NULL, index++, &names)))
    {
//	WINE_TRACE ("%s: %s\n", wine_dbgstr_w (names.szName), 
//            wine_dbgstr_w (names.szDisplayName));
	color_or_size_dsa_add (wdsa, names.szName, names.szDisplayName);
    }
}

/* Theme enumeration callback, adds theme to theme list */
static BOOL CALLBACK myEnumThemeProc (LPVOID lpReserved, 
				      LPCWSTR pszThemeFileName,
				      LPCWSTR pszThemeName, 
				      LPCWSTR pszToolTip, 
				      LPVOID lpReserved2, LPVOID lpData)
{
    ThemeFile newEntry;

    /* fill size/color lists */
    create_color_or_size_dsa (&newEntry.colors);
    fill_theme_string_array (pszThemeFileName, &newEntry.colors, EnumThemeColors);
    create_color_or_size_dsa (&newEntry.sizes);
    fill_theme_string_array (pszThemeFileName, &newEntry.sizes, EnumThemeSizes);

    newEntry.themeFileName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (pszThemeFileName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.themeFileName, pszThemeFileName);
  
    newEntry.fancyName = HeapAlloc (GetProcessHeap(), 0, 
	(lstrlenW (pszThemeName) + 1) * sizeof(WCHAR));
    lstrcpyW (newEntry.fancyName, pszThemeName);
  
    /*list_add_tail (&themeFiles, &newEntry->entry);*/
    DSA_InsertItem (themeFiles, themeFilesCount, &newEntry);
    themeFilesCount++;

    return TRUE;
}

/* Scan for themes */
static void scan_theme_files(void)
{
    static const WCHAR themesSubdir[] = { '\\','T','h','e','m','e','s',0 };
    WCHAR themesPath[MAX_PATH];

    free_theme_files();

    if (FAILED (SHGetFolderPathW (NULL, CSIDL_RESOURCES, NULL, 
        SHGFP_TYPE_CURRENT, themesPath))) return;

    themeFiles = DSA_Create (sizeof (ThemeFile), 1);
    lstrcatW (themesPath, themesSubdir);

    EnumThemes (themesPath, myEnumThemeProc, 0);
}

/* fill the color & size combo boxes for a given theme */
static void fill_color_size_combos (ThemeFile* theme, HWND comboColor, 
                                    HWND comboSize)
{
    int i;

    SendMessageW (comboColor, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < theme->colors.count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (&theme->colors, i);
	SendMessageW (comboColor, CB_ADDSTRING, 0, (LPARAM)item->fancyName);
    }

    SendMessageW (comboSize, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < theme->sizes.count; i++)
    {
	ThemeColorOrSize* item = color_or_size_dsa_get (&theme->sizes, i);
	SendMessageW (comboSize, CB_ADDSTRING, 0, (LPARAM)item->fancyName);
    }
}

/* Select the item of a combo box that matches a theme's color and size 
 * scheme. */
static void select_color_and_size (ThemeFile* theme, 
			    const WCHAR* colorName, HWND comboColor, 
			    const WCHAR* sizeName, HWND comboSize)
{
    SendMessageW (comboColor, CB_SETCURSEL, 
	color_or_size_dsa_find (&theme->colors, colorName), 0);
    SendMessageW (comboSize, CB_SETCURSEL, 
	color_or_size_dsa_find (&theme->sizes, sizeName), 0);
}

/* Fill theme, color and sizes combo boxes with the know themes and select
 * the entries matching the currently active theme. */
static BOOL fill_theme_list (HWND comboTheme, HWND comboColor, HWND comboSize)
{
    WCHAR textNoTheme[256];
    int themeIndex = 0;
    BOOL ret = TRUE;
    int i;
    WCHAR currentTheme[MAX_PATH];
    WCHAR currentColor[MAX_PATH];
    WCHAR currentSize[MAX_PATH];
    ThemeFile* theme = NULL;

    LoadStringW (GetModuleHandle (NULL), IDS_NOTHEME, textNoTheme,
	sizeof(textNoTheme) / sizeof(WCHAR));

    SendMessageW (comboTheme, CB_RESETCONTENT, 0, 0);
    SendMessageW (comboTheme, CB_ADDSTRING, 0, (LPARAM)textNoTheme);

    for (i = 0; i < themeFilesCount; i++)
    {
        ThemeFile* item = DSA_GetItemPtr (themeFiles, i);
	SendMessageW (comboTheme, CB_ADDSTRING, 0, 
	    (LPARAM)item->fancyName);
    }
  
    if (IsThemeActive () && SUCCEEDED (GetCurrentThemeName (currentTheme, 
	    sizeof(currentTheme) / sizeof(WCHAR),
	    currentColor, sizeof(currentColor) / sizeof(WCHAR),
	    currentSize, sizeof(currentSize) / sizeof(WCHAR))))
    {
	/* Determine the index of the currently active theme. */
	BOOL found = FALSE;
	for (i = 0; i < themeFilesCount; i++)
	{
            theme = DSA_GetItemPtr (themeFiles, i);
	    if (lstrcmpiW (theme->themeFileName, currentTheme) == 0)
	    {
		found = TRUE;
		themeIndex = i+1;
		break;
	    }
	}
	if (!found)
	{
	    /* Current theme not found?... add to the list, then... */
//	    WINE_TRACE("Theme %s not in list of enumerated themes\n",
//		wine_dbgstr_w (currentTheme));
	    myEnumThemeProc (NULL, currentTheme, currentTheme, 
		currentTheme, NULL, NULL);
	    themeIndex = themeFilesCount;
            theme = DSA_GetItemPtr (themeFiles, themeFilesCount-1);
	}
	fill_color_size_combos (theme, comboColor, comboSize);
	select_color_and_size (theme, currentColor, comboColor,
	    currentSize, comboSize);
    }
    else
    {
	/* No theme selected */
	ret = FALSE;
    }

    SendMessageW (comboTheme, CB_SETCURSEL, themeIndex, 0);
    return ret;
}

/* Update the color & size combo boxes when the selection of the theme
 * combo changed. Selects the current color and size scheme if the theme
 * is currently active, otherwise the first color and size. */
static BOOL update_color_and_size (int themeIndex, HWND comboColor, 
				   HWND comboSize)
{
    if (themeIndex == 0)
    {
	return FALSE;
    }
    else
    {
	WCHAR currentTheme[MAX_PATH];
	WCHAR currentColor[MAX_PATH];
	WCHAR currentSize[MAX_PATH];
	ThemeFile* theme = DSA_GetItemPtr (themeFiles, themeIndex - 1);
    
	fill_color_size_combos (theme, comboColor, comboSize);
      
	if ((SUCCEEDED (GetCurrentThemeName (currentTheme, 
	    sizeof(currentTheme) / sizeof(WCHAR),
	    currentColor, sizeof(currentColor) / sizeof(WCHAR),
	    currentSize, sizeof(currentSize) / sizeof(WCHAR))))
	    && (lstrcmpiW (currentTheme, theme->themeFileName) == 0))
	{
	    select_color_and_size (theme, currentColor, comboColor,
		currentSize, comboSize);
	}
	else
	{
	    SendMessageW (comboColor, CB_SETCURSEL, 0, 0);
	    SendMessageW (comboSize, CB_SETCURSEL, 0, 0);
	}
    }
    return TRUE;
}

/* Apply a theme from a given theme, color and size combo box item index. */
static void do_apply_theme (int themeIndex, int colorIndex, int sizeIndex)
{
    static char b[] = "\0";

    if (themeIndex == 0)
    {
	/* no theme */
	ApplyTheme (NULL, b, NULL);
    }
    else
    {
        ThemeFile* theme = DSA_GetItemPtr (themeFiles, themeIndex-1);
	const WCHAR* themeFileName = theme->themeFileName;
	const WCHAR* colorName = NULL;
	const WCHAR* sizeName = NULL;
	HTHEMEFILE hTheme;
	ThemeColorOrSize* item;
    
	item = color_or_size_dsa_get (&theme->colors, colorIndex);
	colorName = item->name;
	
	item = color_or_size_dsa_get (&theme->sizes, sizeIndex);
	sizeName = item->name;
	
	if (SUCCEEDED (OpenThemeFile (themeFileName, colorName, sizeName,
	    &hTheme, 0)))
	{
	    ApplyTheme (hTheme, b, NULL);
	    CloseThemeFile (hTheme);
	}
	else
	{
	    ApplyTheme (NULL, b, NULL);
	}
    }
}

int updating_ui;
BOOL theme_dirty;

static void enable_size_and_color_controls (HWND dialog, BOOL enable)
{
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_COLORTEXT), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), enable);
    EnableWindow (GetDlgItem (dialog, IDC_THEME_SIZETEXT), enable);
}
  
static void init_dialog (HWND dialog)
{
    updating_ui = TRUE;
    
    scan_theme_files();
    if (!fill_theme_list (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
    {
        SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        enable_size_and_color_controls (dialog, FALSE);
    }
    else
    {
        enable_size_and_color_controls (dialog, TRUE);
    }
    theme_dirty = FALSE;

    SendDlgItemMessageW(dialog, IDC_SYSPARAM_SIZE_UD, UDM_SETBUDDY, (WPARAM)GetDlgItem(dialog, IDC_SYSPARAM_SIZE), 0);
    SendDlgItemMessageW(dialog, IDC_SYSPARAM_SIZE_UD, UDM_SETRANGE, 0, MAKELONG(100, 8));

    updating_ui = FALSE;
}

static void on_theme_changed(HWND dialog) {
    int index = SendMessageW (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        CB_GETCURSEL, 0, 0);
    if (!update_color_and_size (index, GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
    {
        SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
        enable_size_and_color_controls (dialog, FALSE);
    }
    else
    {
        enable_size_and_color_controls (dialog, TRUE);
    }
    theme_dirty = TRUE;
}

static void apply_theme(HWND dialog)
{
    int themeIndex, colorIndex, sizeIndex;

    if (!theme_dirty) return;

    themeIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
        CB_GETCURSEL, 0, 0);
    colorIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
        CB_GETCURSEL, 0, 0);
    sizeIndex = SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO),
        CB_GETCURSEL, 0, 0);

    do_apply_theme (themeIndex, colorIndex, sizeIndex);
    theme_dirty = FALSE;
}

static struct
{
    int sm_idx, color_idx;
    const char *color_reg;
    int size;
    COLORREF color;
    LOGFONTW lf;
} metrics[] =
{
    {-1,                COLOR_BTNFACE,          "ButtonFace"    }, /* IDC_SYSPARAMS_BUTTON */
    {-1,                COLOR_BTNTEXT,          "ButtonText"    }, /* IDC_SYSPARAMS_BUTTON_TEXT */
    {-1,                COLOR_BACKGROUND,       "Background"    }, /* IDC_SYSPARAMS_DESKTOP */
    {SM_CXMENUSIZE,     COLOR_MENU,             "Menu"          }, /* IDC_SYSPARAMS_MENU */
    {-1,                COLOR_MENUTEXT,         "MenuText"      }, /* IDC_SYSPARAMS_MENU_TEXT */
    {SM_CXVSCROLL,      COLOR_SCROLLBAR,        "Scrollbar"     }, /* IDC_SYSPARAMS_SCROLLBAR */
    {-1,                COLOR_HIGHLIGHT,        "Hilight"       }, /* IDC_SYSPARAMS_SELECTION */
    {-1,                COLOR_HIGHLIGHTTEXT,    "HilightText"   }, /* IDC_SYSPARAMS_SELECTION_TEXT */
    {-1,                COLOR_INFOBK,           "InfoWindow"    }, /* IDC_SYSPARAMS_TOOLTIP */
    {-1,                COLOR_INFOTEXT,         "InfoText"      }, /* IDC_SYSPARAMS_TOOLTIP_TEXT */
    {-1,                COLOR_WINDOW,           "Window"        }, /* IDC_SYSPARAMS_WINDOW */
    {-1,                COLOR_WINDOWTEXT,       "WindowText"    }, /* IDC_SYSPARAMS_WINDOW_TEXT */
    {SM_CXSIZE,         COLOR_ACTIVECAPTION,    "ActiveTitle"   }, /* IDC_SYSPARAMS_ACTIVE_TITLE */
    {-1,                COLOR_CAPTIONTEXT,      "TitleText"     }, /* IDC_SYSPARAMS_ACTIVE_TITLE_TEXT */
    {-1,                COLOR_INACTIVECAPTION,  "InactiveTitle" }, /* IDC_SYSPARAMS_INACTIVE_TITLE */
    {-1,                COLOR_INACTIVECAPTIONTEXT,"InactiveTitleText" }, /* IDC_SYSPARAMS_INACTIVE_TITLE_TEXT */
    {-1,                -1,                     "MsgBoxText"    }, /* IDC_SYSPARAMS_MSGBOX_TEXT */
    {-1,                COLOR_APPWORKSPACE,     "AppWorkSpace"  }, /* IDC_SYSPARAMS_APPWORKSPACE */
    {-1,                COLOR_WINDOWFRAME,      "WindowFrame"   }, /* IDC_SYSPARAMS_WINDOW_FRAME */
    {-1,                COLOR_ACTIVEBORDER,     "ActiveBorder"  }, /* IDC_SYSPARAMS_ACTIVE_BORDER */
    {-1,                COLOR_INACTIVEBORDER,   "InactiveBorder" }, /* IDC_SYSPARAMS_INACTIVE_BORDER */
    {-1,                COLOR_BTNSHADOW,        "ButtonShadow"  }, /* IDC_SYSPARAMS_BUTTON_SHADOW */
    {-1,                COLOR_GRAYTEXT,         "GrayText"      }, /* IDC_SYSPARAMS_GRAY_TEXT */
    {-1,                COLOR_BTNHILIGHT,       "ButtonHilight" }, /* IDC_SYSPARAMS_BUTTON_HILIGHT */
    {-1,                COLOR_3DDKSHADOW,       "ButtonDkShadow" }, /* IDC_SYSPARAMS_BUTTON_DARK_SHADOW */
    {-1,                COLOR_3DLIGHT,          "ButtonLight"   }, /* IDC_SYSPARAMS_BUTTON_LIGHT */
    {-1,                -1, "ButtonAlternateFace" }, /* IDC_SYSPARAMS_BUTTON_ALTERNATE */
    {-1,                COLOR_HOTLIGHT,         "HotTrackingColor" }, /* IDC_SYSPARAMS_HOT_TRACKING */
    {-1,                COLOR_GRADIENTACTIVECAPTION, "GradientActiveTitle" }, /* IDC_SYSPARAMS_ACTIVE_TITLE_GRADIENT */
    {-1,                COLOR_GRADIENTINACTIVECAPTION, "GradientInactiveTitle" }, /* IDC_SYSPARAMS_INACTIVE_TITLE_GRADIENT */
    {-1,                COLOR_MENUHILIGHT,      "MenuHilight"   }, /* IDC_SYSPARAMS_MENU_HILIGHT */
    {-1,                COLOR_MENUBAR,          "MenuBar"       }, /* IDC_SYSPARAMS_MENUBAR */
};

static void save_sys_color(int idx, COLORREF clr)
{
    //char buffer[13];

    //sprintf(buffer, "%d %d %d",  GetRValue (clr), GetGValue (clr), GetBValue (clr));
    //set_reg_key(HKEY_CURRENT_USER, "Control Panel\\Colors", metrics[idx].color_reg, buffer);
}

static void set_color_from_theme(WCHAR *keyName, COLORREF color)
{
    char *keyNameA = NULL;
    int keyNameSize=0, i=0;

    keyNameSize = WideCharToMultiByte(CP_ACP, 0, keyName, -1, keyNameA, 0, NULL, NULL);
    keyNameA = HeapAlloc(GetProcessHeap(), 0, keyNameSize);
    WideCharToMultiByte(CP_ACP, 0, keyName, -1, keyNameA, keyNameSize, NULL, NULL);

    for (i=0; i<sizeof(metrics)/sizeof(metrics[0]); i++)
    {
        if (lstrcmpiA(metrics[i].color_reg, keyNameA)==0)
        {
            metrics[i].color = color;
            save_sys_color(i, color);
            break;
        }
    }
    HeapFree(GetProcessHeap(), 0, keyNameA);
}

static void do_parse_theme(WCHAR *file)
{
    static const WCHAR colorSect[] = {
        'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
        'C','o','l','o','r','s',0};
    WCHAR keyName[MAX_PATH], keyNameValue[MAX_PATH];
    WCHAR *keyNamePtr = NULL;
    char *keyNameValueA = NULL;
    int keyNameValueSize = 0;
    int red = 0, green = 0, blue = 0;
    COLORREF color;

//    WINE_TRACE("%s\n", wine_dbgstr_w(file));

    GetPrivateProfileStringW(colorSect, NULL, NULL, keyName,
                             MAX_PATH, file);

    keyNamePtr = keyName;
    while (*keyNamePtr!=0) {
        GetPrivateProfileStringW(colorSect, keyNamePtr, NULL, keyNameValue,
                                 MAX_PATH, file);

        keyNameValueSize = WideCharToMultiByte(CP_ACP, 0, keyNameValue, -1,
                                               keyNameValueA, 0, NULL, NULL);
        keyNameValueA = HeapAlloc(GetProcessHeap(), 0, keyNameValueSize);
        WideCharToMultiByte(CP_ACP, 0, keyNameValue, -1, keyNameValueA, keyNameValueSize, NULL, NULL);

//        WINE_TRACE("parsing key: %s with value: %s\n",
//                   wine_dbgstr_w(keyNamePtr), wine_dbgstr_w(keyNameValue));

        sscanf(keyNameValueA, "%d %d %d", &red, &green, &blue);

        color = RGB((BYTE)red, (BYTE)green, (BYTE)blue);

        HeapFree(GetProcessHeap(), 0, keyNameValueA);

        set_color_from_theme(keyNamePtr, color);

        keyNamePtr+=lstrlenW(keyNamePtr);
        keyNamePtr++;
    }
}

static void on_theme_install(HWND dialog)
{
  static const WCHAR filterMask[] = {0,'*','.','m','s','s','t','y','l','e','s',';',
      '*','.','t','h','e','m','e',0,0};
  static const WCHAR themeExt[] = {'.','T','h','e','m','e',0};
  const int filterMaskLen = sizeof(filterMask)/sizeof(filterMask[0]);
  OPENFILENAMEW ofn;
  WCHAR filetitle[MAX_PATH];
  WCHAR file[MAX_PATH];
  WCHAR filter[100];
  WCHAR title[100];

  LoadStringW (GetModuleHandle (NULL), IDS_THEMEFILE, 
      filter, sizeof (filter) / sizeof (filter[0]) - filterMaskLen);
  memcpy (filter + lstrlenW (filter), filterMask, 
      filterMaskLen * sizeof (WCHAR));
  LoadStringW (GetModuleHandle (NULL), IDS_THEMEFILE_SELECT, 
      title, sizeof (title) / sizeof (title[0]));

  ofn.lStructSize = sizeof(OPENFILENAMEW);
  ofn.hwndOwner = 0;
  ofn.hInstance = 0;
  ofn.lpstrFilter = filter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter = 0;
  ofn.nFilterIndex = 0;
  ofn.lpstrFile = file;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(file)/sizeof(filetitle[0]);
  ofn.lpstrFileTitle = filetitle;
  ofn.lpstrFileTitle[0] = '\0';
  ofn.nMaxFileTitle = sizeof(filetitle)/sizeof(filetitle[0]);
  ofn.lpstrInitialDir = NULL;
  ofn.lpstrTitle = title;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nFileOffset = 0;
  ofn.nFileExtension = 0;
  ofn.lpstrDefExt = NULL;
  ofn.lCustData = 0;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName = NULL;

  if (GetOpenFileNameW(&ofn))
  {
      static const WCHAR themesSubdir[] = { '\\','T','h','e','m','e','s',0 };
      static const WCHAR backslash[] = { '\\',0 };
      WCHAR themeFilePath[MAX_PATH];
      SHFILEOPSTRUCTW shfop;

      if (FAILED (SHGetFolderPathW (NULL, CSIDL_RESOURCES|CSIDL_FLAG_CREATE, NULL, 
          SHGFP_TYPE_CURRENT, themeFilePath))) return;

      if (lstrcmpiW(PathFindExtensionW(filetitle), themeExt)==0)
      {
          do_parse_theme(file);
          SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
          return;
      }

      PathRemoveExtensionW (filetitle);

      lstrcatW (themeFilePath, themesSubdir);
      lstrcatW (themeFilePath, backslash);
      lstrcatW (themeFilePath, filetitle);

      /* Create the directory */
      SHCreateDirectoryExW (dialog, themeFilePath, NULL);

      /* Append theme file name itself */
      lstrcatW (themeFilePath, backslash);
      lstrcatW (themeFilePath, PathFindFileNameW (file));
      /* SHFileOperation() takes lists as input, so double-nullterminate */
      themeFilePath[lstrlenW (themeFilePath)+1] = 0;
      file[lstrlenW (file)+1] = 0;

      /* Do the copying */
//      WINE_TRACE("copying: %s -> %s\n", wine_dbgstr_w (file), 
//          wine_dbgstr_w (themeFilePath));
      shfop.hwnd = dialog;
      shfop.wFunc = FO_COPY;
      shfop.pFrom = file;
      shfop.pTo = themeFilePath;
      shfop.fFlags = FOF_NOCONFIRMMKDIR;
      if (SHFileOperationW (&shfop) == 0)
      {
          scan_theme_files();
          if (!fill_theme_list (GetDlgItem (dialog, IDC_THEME_THEMECOMBO),
              GetDlgItem (dialog, IDC_THEME_COLORCOMBO),
              GetDlgItem (dialog, IDC_THEME_SIZECOMBO)))
          {
              SendMessageW (GetDlgItem (dialog, IDC_THEME_COLORCOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
              SendMessageW (GetDlgItem (dialog, IDC_THEME_SIZECOMBO), CB_SETCURSEL, (WPARAM)-1, 0);
              enable_size_and_color_controls (dialog, FALSE);
          }
          else
          {
              enable_size_and_color_controls (dialog, TRUE);
          }
      }
//      else
//          WINE_TRACE("copy operation failed\n");
  }
//  else WINE_TRACE("user cancelled\n");
}

/* Information about symbolic link targets of certain User Shell Folders. */
struct ShellFolderInfo {
    int nFolder;
    char szLinkTarget[FILENAME_MAX]; /* in unix locale */
};
/*
static struct ShellFolderInfo asfiInfo[] = {
    { CSIDL_DESKTOP,  "" },
    { CSIDL_PERSONAL, "" },
    { CSIDL_MYPICTURES, "" },
    { CSIDL_MYMUSIC, "" },
    { CSIDL_MYVIDEO, "" }
};*/
/*
static struct ShellFolderInfo *psfiSelected = NULL;
*/
#define NUM_ELEMS(x) (sizeof(x)/sizeof(*(x)))

/* create a unicode string from a string in Unix locale *//*
static WCHAR *strdupU2W(const char *unix_str)
{
    WCHAR *unicode_str;
    int lenW;

    lenW = MultiByteToWideChar(CP_UNIXCP, 0, unix_str, -1, NULL, 0);
    unicode_str = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
    if (unicode_str)
        MultiByteToWideChar(CP_UNIXCP, 0, unix_str, -1, unicode_str, lenW);
    return unicode_str;
}*/
/*
static void init_shell_folder_listview_headers(HWND dialog) {
    LVCOLUMN listColumn;
    RECT viewRect;
    char szShellFolder[64] = "Shell Folder";
    char szLinksTo[64] = "Links to";
    int width;

    LoadString(GetModuleHandle(NULL), IDS_SHELL_FOLDER, szShellFolder, sizeof(szShellFolder));
    LoadString(GetModuleHandle(NULL), IDS_LINKS_TO, szLinksTo, sizeof(szLinksTo));
    
    GetClientRect(GetDlgItem(dialog, IDC_LIST_SFPATHS), &viewRect);
    width = (viewRect.right - viewRect.left) / 4;

    listColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    listColumn.pszText = szShellFolder;
    listColumn.cchTextMax = lstrlen(listColumn.pszText);
    listColumn.cx = width;

    SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_INSERTCOLUMN, 0, (LPARAM) &listColumn);

    listColumn.pszText = szLinksTo;
    listColumn.cchTextMax = lstrlen(listColumn.pszText);
    listColumn.cx = viewRect.right - viewRect.left - width - 1;

    SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_INSERTCOLUMN, 1, (LPARAM) &listColumn);
}*/

/* Reads the currently set shell folder symbol link targets into asfiInfo. */
/*
static void read_shell_folder_link_targets(void) {
    WCHAR wszPath[MAX_PATH];
    HRESULT hr;
    int i;
   
    for (i=0; i<NUM_ELEMS(asfiInfo); i++) {
        asfiInfo[i].szLinkTarget[0] = '\0';
        hr = SHGetFolderPathW(NULL, asfiInfo[i].nFolder|CSIDL_FLAG_DONT_VERIFY, NULL, 
                              SHGFP_TYPE_CURRENT, wszPath);
        if (SUCCEEDED(hr)) {*/
/*            char *pszUnixPath = wine_get_unix_file_name(wszPath);
            if (pszUnixPath) {
                struct stat statPath;
                if (!lstat(pszUnixPath, &statPath) && S_ISLNK(statPath.st_mode)) {
                    int cLen = readlink(pszUnixPath, asfiInfo[i].szLinkTarget, FILENAME_MAX-1);
                    if (cLen >= 0) asfiInfo[i].szLinkTarget[cLen] = '\0';
                }
                HeapFree(GetProcessHeap(), 0, pszUnixPath);
            }
  */      /*} 
    }    
}
*//*
static void update_shell_folder_listview(HWND dialog) {
    int i;
    LVITEMW item;
    LONG lSelected = SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_GETNEXTITEM, (WPARAM)-1, 
                                        MAKELPARAM(LVNI_SELECTED,0));
    
    SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_DELETEALLITEMS, 0, 0);

    for (i=0; i<NUM_ELEMS(asfiInfo); i++) {
        WCHAR buffer[MAX_PATH];
        HRESULT hr;
        LPITEMIDLIST pidlCurrent;

        hr = SHGetFolderLocation(dialog, asfiInfo[i].nFolder, NULL, 0, &pidlCurrent);
        if (SUCCEEDED(hr)) { 
            LPSHELLFOLDER psfParent;
            LPCITEMIDLIST pidlLast;
            hr = SHBindToParent(pidlCurrent, &IID_IShellFolder, (LPVOID*)&psfParent, &pidlLast);
            if (SUCCEEDED(hr)) {
                STRRET strRet;
                hr = IShellFolder_GetDisplayNameOf(psfParent, pidlLast, SHGDN_FORADDRESSBAR, &strRet);
                if (SUCCEEDED(hr)) {
                    hr = StrRetToBufW(&strRet, pidlLast, buffer, MAX_PATH);
                }
                IShellFolder_Release(psfParent);
            }
            ILFree(pidlCurrent);
        }

        if (FAILED(hr)) {
            hr = SHGetFolderPathW(dialog, asfiInfo[i].nFolder|CSIDL_FLAG_DONT_VERIFY, NULL,
                                 SHGFP_TYPE_CURRENT, buffer);
        }
    
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = buffer;
        item.lParam = (LPARAM)&asfiInfo[i];
        SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_INSERTITEMW, 0, (LPARAM)&item);

        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 1;
        item.pszText = strdupU2W(asfiInfo[i].szLinkTarget);
        SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_SETITEMW, 0, (LPARAM)&item);
        HeapFree(GetProcessHeap(), 0, item.pszText);
    }

    if (lSelected >= 0) {
        item.mask = LVIF_STATE;
        item.state = LVIS_SELECTED;
        item.stateMask = LVIS_SELECTED;
        SendDlgItemMessage(dialog, IDC_LIST_SFPATHS, LVM_SETITEMSTATE, (WPARAM)lSelected, 
                           (LPARAM)&item);
    }
}

static void on_shell_folder_selection_changed(HWND hDlg, LPNMLISTVIEW lpnm) {
    if (lpnm->uNewState & LVIS_SELECTED) {
        psfiSelected = (struct ShellFolderInfo *)lpnm->lParam;
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_SFPATH), 1);
        if (strlen(psfiSelected->szLinkTarget)) {
            WCHAR *link;
            CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 1);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 1);
            link = strdupU2W(psfiSelected->szLinkTarget);
            set_textW(hDlg, IDC_EDIT_SFPATH, link);
            HeapFree(GetProcessHeap(), 0, link);
        } else {
            CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 0);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 0);
            set_text(hDlg, IDC_EDIT_SFPATH, "");
        }
    } else {
        psfiSelected = NULL;
        CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
        set_text(hDlg, IDC_EDIT_SFPATH, "");
        EnableWindow(GetDlgItem(hDlg, IDC_LINK_SFPATH), 0);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SFPATH), 0);
        EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_SFPATH), 0);
    }
}

static void on_shell_folder_edit_changed(HWND hDlg) {
    LVITEMW item;
    WCHAR *text = get_textW(hDlg, IDC_EDIT_SFPATH);
    LONG iSel = SendDlgItemMessage(hDlg, IDC_LIST_SFPATHS, LVM_GETNEXTITEM, -1,
                                   MAKELPARAM(LVNI_SELECTED,0));
    
    if (!text || !psfiSelected || iSel < 0) {
        HeapFree(GetProcessHeap(), 0, text);
        return;
    }

    WideCharToMultiByte(CP_UNIXCP, 0, text, -1,
                        psfiSelected->szLinkTarget, FILENAME_MAX, NULL, NULL);

    item.mask = LVIF_TEXT;
    item.iItem = iSel;
    item.iSubItem = 1;
    item.pszText = text;
    SendDlgItemMessage(hDlg, IDC_LIST_SFPATHS, LVM_SETITEMW, 0, (LPARAM)&item);

    HeapFree(GetProcessHeap(), 0, text);

    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
}
*/
static void read_sysparams(HWND hDlg)
{
    WCHAR buffer[256];
    HWND list = GetDlgItem(hDlg, IDC_SYSPARAM_COMBO);
    NONCLIENTMETRICSW nonclient_metrics;
    int i, idx;

    for (i = 0; i < sizeof(metrics) / sizeof(metrics[0]); i++)
    {
        LoadStringW(GetModuleHandle(NULL), i + IDC_SYSPARAMS_BUTTON, buffer,
                    sizeof(buffer) / sizeof(buffer[0]));
        idx = SendMessageW(list, CB_ADDSTRING, 0, (LPARAM)buffer);
        if (idx != CB_ERR) SendMessageW(list, CB_SETITEMDATA, idx, i);

        if (metrics[i].sm_idx != -1)
            metrics[i].size = GetSystemMetrics(metrics[i].sm_idx);
        if (metrics[i].color_idx != -1)
            metrics[i].color = GetSysColor(metrics[i].color_idx);
    }

    nonclient_metrics.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &nonclient_metrics, 0);

    memcpy(&(metrics[IDC_SYSPARAMS_MENU_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfMenuFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_ACTIVE_TITLE_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfCaptionFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_TOOLTIP_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfStatusFont), sizeof(LOGFONTW));
    memcpy(&(metrics[IDC_SYSPARAMS_MSGBOX_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           &(nonclient_metrics.lfMessageFont), sizeof(LOGFONTW));
}

static void apply_sysparams(void)
{
    NONCLIENTMETRICSW nonclient_metrics;
    int i, cnt = 0;
    int colors_idx[sizeof(metrics) / sizeof(metrics[0])];
    COLORREF colors[sizeof(metrics) / sizeof(metrics[0])];

    nonclient_metrics.cbSize = sizeof(nonclient_metrics);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(nonclient_metrics), &nonclient_metrics, 0);

    nonclient_metrics.iMenuWidth = nonclient_metrics.iMenuHeight =
            metrics[IDC_SYSPARAMS_MENU - IDC_SYSPARAMS_BUTTON].size;
    nonclient_metrics.iCaptionWidth = nonclient_metrics.iCaptionHeight =
            metrics[IDC_SYSPARAMS_ACTIVE_TITLE - IDC_SYSPARAMS_BUTTON].size;
    nonclient_metrics.iScrollWidth = nonclient_metrics.iScrollHeight =
            metrics[IDC_SYSPARAMS_SCROLLBAR - IDC_SYSPARAMS_BUTTON].size;

    memcpy(&(nonclient_metrics.lfMenuFont),
           &(metrics[IDC_SYSPARAMS_MENU_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           sizeof(LOGFONTW));
    memcpy(&(nonclient_metrics.lfCaptionFont),
           &(metrics[IDC_SYSPARAMS_ACTIVE_TITLE_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           sizeof(LOGFONTW));
    memcpy(&(nonclient_metrics.lfStatusFont),
           &(metrics[IDC_SYSPARAMS_TOOLTIP_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           sizeof(LOGFONTW));
    memcpy(&(nonclient_metrics.lfMessageFont),
           &(metrics[IDC_SYSPARAMS_MSGBOX_TEXT - IDC_SYSPARAMS_BUTTON].lf),
           sizeof(LOGFONTW));

    SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(nonclient_metrics), &nonclient_metrics,
                          SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);

    for (i = 0; i < sizeof(metrics) / sizeof(metrics[0]); i++)
        if (metrics[i].color_idx != -1)
        {
            colors_idx[cnt] = metrics[i].color_idx;
            colors[cnt++] = metrics[i].color;
        }
    SetSysColors(cnt, colors_idx, colors);
}

static void on_sysparam_change(HWND hDlg)
{
    int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

    index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

    updating_ui = TRUE;

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR_TEXT), metrics[index].color_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), metrics[index].color_idx != -1);
    InvalidateRect(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), NULL, TRUE);

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE_TEXT), metrics[index].sm_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE), metrics[index].sm_idx != -1);
    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_SIZE_UD), metrics[index].sm_idx != -1);
    if (metrics[index].sm_idx != -1)
        SendDlgItemMessageW(hDlg, IDC_SYSPARAM_SIZE_UD, UDM_SETPOS, 0, MAKELONG(metrics[index].size, 0));
    else
        set_text(hDlg, IDC_SYSPARAM_SIZE, "");

    EnableWindow(GetDlgItem(hDlg, IDC_SYSPARAM_FONT),
        index == IDC_SYSPARAMS_MENU_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_ACTIVE_TITLE_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_TOOLTIP_TEXT-IDC_SYSPARAMS_BUTTON ||
        index == IDC_SYSPARAMS_MSGBOX_TEXT-IDC_SYSPARAMS_BUTTON
    );

    updating_ui = FALSE;
}

static void on_draw_item(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH black_brush = 0;
    LPDRAWITEMSTRUCT draw_info = (LPDRAWITEMSTRUCT)lParam;

    if (!black_brush) black_brush = CreateSolidBrush(0);

    if (draw_info->CtlID == IDC_SYSPARAM_COLOR)
    {
        UINT state = DFCS_ADJUSTRECT | DFCS_BUTTONPUSH;

        if (draw_info->itemState & ODS_DISABLED)
            state |= DFCS_INACTIVE;
        else
            state |= draw_info->itemState & ODS_SELECTED ? DFCS_PUSHED : 0;

        DrawFrameControl(draw_info->hDC, &draw_info->rcItem, DFC_BUTTON, state);

        if (!(draw_info->itemState & ODS_DISABLED))
        {
            HBRUSH brush;
            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);
            brush = CreateSolidBrush(metrics[index].color);

            InflateRect(&draw_info->rcItem, -1, -1);
            FrameRect(draw_info->hDC, &draw_info->rcItem, black_brush);
            InflateRect(&draw_info->rcItem, -1, -1);
            FillRect(draw_info->hDC, &draw_info->rcItem, brush);
            DeleteObject(brush);
        }
    }
}

static void on_select_font(HWND hDlg)
{
    CHOOSEFONTW cf;
    int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);
    index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(CHOOSEFONTW);
    cf.hwndOwner = hDlg;
    cf.lpLogFont = &(metrics[index].lf);
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;

    ChooseFontW(&cf);
}

INT_PTR CALLBACK
ThemeDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
//            read_shell_folder_link_targets();
//            init_shell_folder_listview_headers(hDlg);
//            update_shell_folder_listview(hDlg);
            read_sysparams(hDlg);
            break;
        
        case WM_DESTROY:
            free_theme_files();
            break;

        case WM_COMMAND:
            switch(HIWORD(wParam)) {
                case CBN_SELCHANGE: {
                    if (updating_ui) break;
                    switch (LOWORD(wParam))
                    {
                        case IDC_THEME_THEMECOMBO: on_theme_changed(hDlg); break;
                        case IDC_THEME_COLORCOMBO: /* fall through */
                        case IDC_THEME_SIZECOMBO: theme_dirty = TRUE; break;
                        case IDC_SYSPARAM_COMBO: on_sysparam_change(hDlg); return FALSE;
                    }
                    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                    break;
                }
                case EN_CHANGE: {
                    if (updating_ui) break;
                    switch (LOWORD(wParam))
                    {
                        //case IDC_EDIT_SFPATH: on_shell_folder_edit_changed(hDlg); break;
                        case IDC_SYSPARAM_SIZE:
                        {
                            char *text = get_text(hDlg, IDC_SYSPARAM_SIZE);
                            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

                            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);
                            metrics[index].size = atoi(text);
                            HeapFree(GetProcessHeap(), 0, text);

                            SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            break;
                        }
                    }
                    break;
                }
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_THEME_INSTALL:
                            on_theme_install (hDlg);
                            break;

                        case IDC_SYSPARAM_FONT:
                            on_select_font(hDlg);
                            break;
/*
                        case IDC_BROWSE_SFPATH:
                        {
                            WCHAR link[FILENAME_MAX];
                            if (browse_for_unix_folder(hDlg, link)) {
                                WideCharToMultiByte(CP_UNIXCP, 0, link, -1,
                                                    psfiSelected->szLinkTarget, FILENAME_MAX,
                                                    NULL, NULL);
                                update_shell_folder_listview(hDlg);
                                SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;
                        }

                        case IDC_LINK_SFPATH:
                            if (IsDlgButtonChecked(hDlg, IDC_LINK_SFPATH)) {
                                WCHAR link[FILENAME_MAX];
                                if (browse_for_unix_folder(hDlg, link)) {
                                    WideCharToMultiByte(CP_UNIXCP, 0, link, -1,
                                                        psfiSelected->szLinkTarget, FILENAME_MAX,
                                                        NULL, NULL);
                                    update_shell_folder_listview(hDlg);
                                    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                                } else {
                                    CheckDlgButton(hDlg, IDC_LINK_SFPATH, BST_UNCHECKED);
                                }
                            } else {
                                psfiSelected->szLinkTarget[0] = '\0';
                                update_shell_folder_listview(hDlg);
                                SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;    
*/
                        case IDC_SYSPARAM_COLOR:
                        {
                            static COLORREF user_colors[16];
                            CHOOSECOLORW c_color;
                            int index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETCURSEL, 0, 0);

                            index = SendDlgItemMessageW(hDlg, IDC_SYSPARAM_COMBO, CB_GETITEMDATA, index, 0);

                            memset(&c_color, 0, sizeof(c_color));
                            c_color.lStructSize = sizeof(c_color);
                            c_color.lpCustColors = user_colors;
                            c_color.rgbResult = metrics[index].color;
                            c_color.Flags = CC_ANYCOLOR | CC_RGBINIT;
                            c_color.hwndOwner = hDlg;
                            if (ChooseColorW(&c_color))
                            {
                                metrics[index].color = c_color.rgbResult;
                                save_sys_color(index, metrics[index].color);
                                InvalidateRect(GetDlgItem(hDlg, IDC_SYSPARAM_COLOR), NULL, TRUE);
                                SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
                            }
                            break;
                        }
                    }
                    break;
            }
            break;
        
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_KILLACTIVE: {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;
                }
                case PSN_APPLY: {
                    /*apply();*/
                    apply_theme(hDlg);
                    /*apply_shell_folder_changes();*/
                    apply_sysparams();
                    //read_shell_folder_link_targets();
                    //update_shell_folder_listview(hDlg);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                }
//                case LVN_ITEMCHANGED: { 
//                    if (wParam == IDC_LIST_SFPATHS)  
//                        on_shell_folder_selection_changed(hDlg, (LPNMLISTVIEW)lParam);
//                    break;
//               }
                case PSN_SETACTIVE: {
                    init_dialog (hDlg);
                    break;
                }
            }
            break;

        case WM_DRAWITEM:
            on_draw_item(hDlg, wParam, lParam);
            break;

        default:
            break;
    }
    return FALSE;
}
