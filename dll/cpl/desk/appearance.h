#pragma once

/* Some definitions for theme */
#define SIZE_BORDER_WIDTH       0
#define SIZE_SCROLL_WIDTH       1
#define SIZE_SCROLL_HEIGHT      2
#define SIZE_CAPTION_WIDTH      3
#define SIZE_CAPTION_HEIGHT     4
#define SIZE_SM_CAPTION_WIDTH   5
#define SIZE_SM_CAPTION_HEIGHT  6
#define SIZE_MENU_WIDTH         7
#define SIZE_MENU_HEIGHT        8

#define SIZE_ICON_SPACE_X       9
#define SIZE_ICON_SPACE_Y       10
#define SIZE_ICON               11

#define FONT_CAPTION            0
#define FONT_SMCAPTION          1
#define FONT_MENU               2
#define FONT_STATUS             3
#define FONT_MESSAGE            4
#define FONT_ICON               5

#define NUM_ELEMENTS            18
#define NUM_FONTS               6
#define NUM_SIZES               9
#define NUM_COLORS              31
#define MAX_TEMPLATES           50
#define MAX_TEMPLATENAMELENTGH  80

/* Some typedefs for theme */

/* Most (but not all) fields below correspond to HKCU\Control Panel\Desktop\UserPreferencesMask */
typedef struct
{
    BOOL bActiveWindowTracking;
    BOOL bMenuAnimation;
    BOOL bComboBoxAnimation;
    BOOL bListBoxSmoothScrolling;
    BOOL bGradientCaptions;
    BOOL bKeyboardCues;
    BOOL bActiveWndTrkZorder;
    BOOL bHotTracking;
    BOOL bMenuFade;
    BOOL bSelectionFade;
    BOOL bTooltipAnimation;
    BOOL bTooltipFade;
    BOOL bCursorShadow;
    BOOL bDropShadow;
    BOOL bUiEffects;
    BOOL bFontSmoothing;
    UINT uiFontSmoothingType;
    BOOL bDragFullWindows;
} EFFECTS;

typedef struct
{
    NONCLIENTMETRICSW ncMetrics;
    ICONMETRICSW icMetrics;
    COLORREF crColor[NUM_COLORS];
    INT iIconSize;
    BOOL bFlatMenus;
    EFFECTS Effects;
} COLOR_SCHEME, *PCOLOR_SCHEME;

/*
 * The classic theme has several different 'colours' and every colour has
 * several sizes. On visual styles however a theme has different colours
 * and different sizes. In other words the user can select a combination
 * of colour and size.
 * That means that for the classic theme THEME.SizesList is unused and
 * every color has some child styles that correspond its sizes.
 * The themes for visual styles however will use both ColoursList and SizesList
 * and ChildStyle will not be used.
 */

/* struct for holding theme colors and sizes */
typedef struct _THEME_STYLE
{
    struct _THEME_STYLE *NextStyle;
    struct _THEME_STYLE *ChildStyle;
    PWSTR StyleName;
    PWSTR DisplayName;
} THEME_STYLE, *PTHEME_STYLE;

typedef struct _THEME
{
    struct _THEME *NextTheme;
    PWSTR ThemeFileName;
    PWSTR DisplayName;
    THEME_STYLE *ColoursList;
    THEME_STYLE *SizesList;
} THEME, *PTHEME;

typedef struct _THEME_SELECTION
{
    BOOL ThemeActive;
    PTHEME Theme;
    PTHEME_STYLE Color;
    PTHEME_STYLE Size;
} THEME_SELECTION, *PTHEME_SELECTION;

/*
 * This is the global structure used to store the current values.
 * A pointer of this get's passed to the functions either directly
 * or by passing hwnd and getting the pointer by GetWindowLongPtr.
 */
typedef struct tagGLOBALS
{
    PTHEME pThemes;

    /*
     * Keep a copy of the selected classic theme in order to select this
     * when user selects the classic theme (and not a horrible random theme )
     */
    THEME_SELECTION ClassicTheme;
    THEME_SELECTION ActiveTheme;

    COLOR_SCHEME Scheme;
    COLOR_SCHEME SchemeAdv;
    BOOL bThemeChanged;
    BOOL bSchemeChanged;
    HBITMAP hbmpColor[3];
    INT CurrentElement;
    HFONT hBoldFont;
    HFONT hItalicFont;
    BOOL bInitializing;

    HBITMAP hbmpThemePreview;
    HDC hdcThemePreview;
} GLOBALS;

/* prototypes for theme.c */
VOID SchemeSetMetric(COLOR_SCHEME *scheme, int id, int value);
int SchemeGetMetric(COLOR_SCHEME *scheme, int id);
PLOGFONTW SchemeGetFont(COLOR_SCHEME *scheme, int id);
PTHEME LoadTheme(IN LPCWSTR pszThemeFileName,IN LPCWSTR pszThemeName);
PTHEME LoadThemes(VOID);
BOOL FindOrAppendTheme(IN PTHEME pThemeList, IN LPCWSTR pwszThemeFileName, IN LPCWSTR pwszColorBuff, IN LPCWSTR pwszSizeBuff, OUT PTHEME_SELECTION pSelectedTheme);
BOOL GetActiveTheme(PTHEME pThemeList, PTHEME_SELECTION pSelectedTheme);
BOOL GetActiveClassicTheme(PTHEME pThemeList, PTHEME_SELECTION pSelectedTheme);
BOOL LoadCurrentScheme(PCOLOR_SCHEME scheme);
BOOL LoadSchemeFromReg(PCOLOR_SCHEME scheme, PTHEME_SELECTION pSelectedTheme);
BOOL LoadSchemeFromTheme(PCOLOR_SCHEME scheme, PTHEME_SELECTION pSelectedTheme);
VOID ApplyScheme(PCOLOR_SCHEME scheme, PTHEME_SELECTION pSelectedTheme);
BOOL ActivateTheme(PTHEME_SELECTION pSelectedTheme);
void CleanupThemes(IN PTHEME pThemeList);
BOOL DrawThemePreview(HDC hdcMem, PCOLOR_SCHEME scheme, PTHEME_SELECTION pSelectedTheme, PRECT prcWindow);
BOOL ActivateThemeFile(LPCWSTR pwszFile);

/* prototypes for appearance.c */
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for advappdlg.c */
INT_PTR CALLBACK AdvAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for effappdlg.c */
INT_PTR CALLBACK EffAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
