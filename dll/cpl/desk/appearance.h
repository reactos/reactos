/* Some definitions for theme */
#define SIZE_BORDER_X 0
#define SIZE_BORDER_Y 1
#define SIZE_CAPTION_Y 2
#define SIZE_ICON_X 3
#define SIZE_ICON_Y 4
#define SIZE_ICON_SPC_X 5
#define SIZE_ICON_SPC_Y 6
#define SIZE_MENU_SIZE_X 7
#define SIZE_MENU_Y 8
#define SIZE_SCROLL_X 9
#define SIZE_SCROLL_Y 10
#define SIZE_SMCAPTION_Y 11
#define SIZE_EDGE_X 12
#define SIZE_EDGE_Y 13
#define SIZE_FRAME_Y 14
#define SIZE_MENU_CHECK_X 15
#define SIZE_MENU_CHECK_Y 16
#define SIZE_MENU_SIZE_Y 17
#define SIZE_SIZE_X 18
#define SIZE_SIZE_Y 19

#define FONT_CAPTION 0
#define FONT_SMCAPTION 1
#define FONT_HILIGHT 2
#define FONT_MENU 2
#define FONT_ICON 3
#define FONT_INFO 4
#define FONT_DIALOG 5

#define NUM_ELEMENTS 22
#define NUM_FONTS 6
#define NUM_SIZES 20
#define NUM_COLORS 31
#define MAX_TEMPLATES 50
#define MAX_TEMPLATENAMELENTGH 80

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
	BOOL bUiEffects;
	BOOL bFontSmoothing;
	BOOL bDragFullWindows;
	UINT uiFontSmoothingType;
} EFFECTS;

typedef struct
{
	COLORREF crColor[NUM_COLORS];
	LOGFONT lfFont[NUM_FONTS];
	INT Size[NUM_SIZES];
	BOOL bFlatMenus;
	EFFECTS Effects;
} COLOR_SCHEME;

typedef struct
{
	TCHAR strKeyName[4];
	TCHAR strSizeName[4];
	TCHAR strDisplayName[MAX_TEMPLATENAMELENTGH];
	TCHAR strLegacyName[MAX_TEMPLATENAMELENTGH];
} SCHEME_PRESET;

/* struct for holding theme colors and sizes */
typedef struct _THEME_STYLE
{
	WCHAR* StlyeName;
	WCHAR* DisplayName;
} THEME_STYLE, *PTHEME_STYLE;

typedef struct _THEME
{
	WCHAR* themeFileName;
	WCHAR* displayName;
	HDSA Colors;
	int ColorsCount;
	HDSA Sizes;
	int SizesCount;

} THEME, *PTHEME;

/* This is the global structure used to store the current values.
   A pointer of this get's passed to the functions either directly
   or by passing hwnd and getting the pointer by GetWindowLongPtr */
typedef struct tagGLOBALS
{
	HDSA Themes;
	int ThemesCount;
	BOOL bThemeActive;

	INT ThemeId;
	INT SchemeId;	/* Theme is customized if SchemeId == -1 */
	INT SizeID;
	TCHAR strSelectedStyle[4];

	LPWSTR pszThemeFileName;
	LPWSTR pszColorName;
	LPWSTR pszSizeName;

	COLOR_SCHEME Scheme;
	COLOR_SCHEME SchemeAdv;
	BOOL bThemeChanged;
	BOOL bSchemeChanged;
	HBITMAP hbmpColor[3];
	INT CurrentElement;
	HFONT hBoldFont;
	HFONT hItalicFont;
    BOOL bInitializing;
} GLOBALS;

extern SCHEME_PRESET g_ColorSchemes[MAX_TEMPLATES];
extern INT g_TemplateCount;

/* prototypes for theme.c */
VOID LoadCurrentScheme(COLOR_SCHEME* scheme);
BOOL LoadSchemeFromReg(COLOR_SCHEME* scheme, INT SchemeId);
VOID ApplyScheme(COLOR_SCHEME* scheme, INT SchemeId);
BOOL SaveScheme(COLOR_SCHEME* scheme, LPCTSTR strLegacyName);
INT LoadSchemePresetEntries(LPTSTR pszSelectedStyle);
VOID LoadThemes(GLOBALS *g);
HRESULT ActivateTheme(PTHEME pTheme, int iColor, int iSize);
void CleanupThemes(GLOBALS *g);

/* prototypes for appearance.c */
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for advappdlg.c */
INT_PTR CALLBACK AdvAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for effappdlg.c */
INT_PTR CALLBACK EffAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
