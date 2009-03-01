
/* Some definitions for appearance page */
#define SIZE_BORDER_X 0
#define SIZE_BORDER_Y 1
#define SIZE_CAPTION_Y 2
#define SIZE_ICON_X 3
#define SIZE_ICON_Y 4
#define SIZE_ICON_SPC_X 5
#define SIZE_ICON_SPC_Y 6
#define SIZE_MENU_X 7
#define SIZE_MENU_Y 8
#define SIZE_SCROLL_X 9
#define SIZE_SCROLL_Y 10
#define SIZE_SMCAPTION_Y 11

#define FONT_CAPTION 0
#define FONT_SMCAPTION 1
#define FONT_HILIGHT 2
#define FONT_MENU 2
#define FONT_ICON 3
#define FONT_INFO 4
#define FONT_DIALOG 5

#define NUM_ELEMENTS 22
#define NUM_FONTS 6
#define NUM_SIZES 13
#define NUM_COLORS 31
#define MAX_TEMPLATES 50
#define MAX_COLORNAMELENGTH 30
#define MAX_TEMPLATENAMELENTGH 80

/* Some typedefs for appearance */
typedef struct
{
	COLORREF crColor[NUM_COLORS];
	LOGFONT lfFont[NUM_FONTS];
	UINT64 Size[NUM_SIZES];
	INT Id;
	BOOL bFlatMenus;
	BOOL bHasChanged;
	BOOL bIsCustom;
} THEME;

typedef struct
{
	TCHAR strKeyName[4];
	TCHAR strSizeName[4];
	TCHAR strDisplayName[MAX_TEMPLATENAMELENTGH];
	TCHAR strLegacyName[MAX_TEMPLATENAMELENTGH];
	INT NumSizes;
} THEME_PRESET;

typedef struct
{
	int Size;
	int Size2;
	int Color1;
	int Color2;
	int Font;
	int FontColor;
} ASSIGNMENT;

/* This is the global structure used to store the current values.
   A pointer of this get's passed to the functions either directly
   or by passing hwnd and getting the pointer by GetWindowLongPtr */
typedef struct tagGLOBALS
{
	THEME_PRESET ThemeTemplates[MAX_TEMPLATES];
	THEME Theme;
	THEME ThemeAdv;
	INT ColorList[NUM_COLORS];
	HBITMAP hbmpColor[3];
	INT CurrentElement;
	COLORREF crCOLOR_BTNFACE;
	COLORREF crCOLOR_BTNSHADOW;
	COLORREF crCOLOR_BTNTEXT;
	COLORREF crCOLOR_BTNHIGHLIGHT;
	HFONT hBoldFont;
	HFONT hItalicFont;
} GLOBALS;


extern const ASSIGNMENT g_Assignment[NUM_ELEMENTS];
extern const TCHAR g_RegColorNames[NUM_COLORS][MAX_COLORNAMELENGTH];
extern const INT g_SizeMetric[NUM_SIZES];

/* prototypes for appearance.c */
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prototypes for advappearancedlg.c */
INT_PTR CALLBACK AdvAppearanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
