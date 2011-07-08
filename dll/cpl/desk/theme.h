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
#define MAX_COLORNAMELENGTH 30
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
} THEME;

typedef struct
{
	TCHAR strKeyName[4];
	TCHAR strSizeName[4];
	TCHAR strDisplayName[MAX_TEMPLATENAMELENTGH];
	TCHAR strLegacyName[MAX_TEMPLATENAMELENTGH];
} THEME_PRESET;

extern const TCHAR g_RegColorNames[NUM_COLORS][MAX_COLORNAMELENGTH];
extern const INT g_SizeMetric[NUM_SIZES];
extern THEME_PRESET g_ThemeTemplates[MAX_TEMPLATES];

/* prototypes for theme.c */
VOID LoadCurrentTheme(THEME* theme);
BOOL LoadThemeFromReg(THEME* theme, INT ThemeId);
VOID ApplyTheme(THEME* theme, INT ThemeId);
BOOL SaveTheme(THEME* theme, LPCTSTR strLegacyName);
INT LoadThemePresetEntries(LPTSTR pszSelectedStyle);
