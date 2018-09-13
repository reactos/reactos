
#define FONT_NONE	-1
#define FONT_CAPTION	0
#define FONT_SMCAPTION	1
#define FONT_MENU	2
#define FONT_ICONTITLE	3
#define FONT_STATUS	4
#define FONT_MSGBOX	5

#define NUM_FONTS	6
typedef struct {
    HFONT hfont;
    LOGFONT lf;
} LOOK_FONT;
extern LOOK_FONT g_fonts[];

#define COLOR_NONE	-1
extern HBRUSH g_brushes[];
extern HPALETTE g_hpal3D;
extern BOOL g_bPalette;

#define SIZE_NONE	-1
#define SIZE_FRAME	0
#define SIZE_SCROLL	1
#define SIZE_CAPTION	2
#define SIZE_SMCAPTION	3
#define SIZE_MENU	4
#define SIZE_DXICON     5
#define SIZE_DYICON     6
#define SIZE_ICON       7
#define SIZE_SMICON     8

#define NUM_SIZES	9

typedef struct {
    int CurSize;
    int MinSize;
    int MaxSize;
} LOOK_SIZE;
extern LOOK_SIZE g_sizes[];

typedef struct {
    int iMainColor;
    int iSize;
    BOOL fLinkSizeToFont;
    int iTextColor;
    int iFont;
    int iResId;		// id of name in resource (or -1 if duplicate)
    int iBaseElement;	// index of element that this overlaps (or -1)
    RECT rc;
    int iGradColor;
} LOOK_ELEMENT;

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the array order in lookdlg.c
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
enum _ELEMENTS {
    ELEMENT_APPSPACE = 0,
    ELEMENT_DESKTOP,
    ELEMENT_INACTIVEBORDER,
    ELEMENT_ACTIVEBORDER,
    ELEMENT_INACTIVECAPTION,
    ELEMENT_INACTIVESYSBUT1,
    ELEMENT_INACTIVESYSBUT2,
    ELEMENT_ACTIVECAPTION,
    ELEMENT_ACTIVESYSBUT1,
    ELEMENT_ACTIVESYSBUT2,
    ELEMENT_MENUNORMAL,
    ELEMENT_MENUSELECTED,
    ELEMENT_MENUDISABLED,
    ELEMENT_WINDOW,
    ELEMENT_MSGBOX,
    ELEMENT_MSGBOXCAPTION,
    ELEMENT_MSGBOXSYSBUT,
    ELEMENT_SCROLLBAR,
    ELEMENT_SCROLLUP,
    ELEMENT_SCROLLDOWN,
    ELEMENT_BUTTON,
    ELEMENT_SMCAPTION,
    ELEMENT_ICON,
    ELEMENT_ICONHORZSPACING,
    ELEMENT_ICONVERTSPACING,
    ELEMENT_INFO
};
// BOGUS:  need to get a size from somewhere
#define NUM_ELEMENTS ELEMENT_INFO+1

#if 0
// go fix lookdlg.c if you decide to add this back in
    ELEMENT_SMICON,
#endif


#define CPI_VGAONLY	0x0001
#define CPI_PALETTEOK	0x0002

typedef struct {
    HWND hwndParent;    // parent for any modal dialogs (choosecolor et al)
    HWND hwndOwner;     // control that owns mini color picker
    COLORREF rgb;
    UINT flags;
    HPALETTE hpal;
} COLORPICK_INFO, FAR * LPCOLORPICK_INFO;

extern int cyBorder;
extern int cxBorder;
extern int cyEdge;
extern int cxEdge;

// NOTE: the order in g_elements must match the enum order above
extern LOOK_ELEMENT g_elements[];

void FAR PASCAL Look_SelectElement(HWND hDlg, int iElement, DWORD dwFlags);

BOOL WINAPI ChooseColorMini(LPCOLORPICK_INFO lpcpi);

DWORD FAR PASCAL DarkenColor(DWORD rgb, int n);
DWORD FAR PASCAL BrightenColor(DWORD rgb, int n);


#define LF32toLF(lplf32, lplf)  (*(lplf) = *(lplf32))
#define LFtoLF32(lplf, lplf32)  (*(lplf32) = *(lplf))

// HACK - REMOVE THESE
extern int g_LogDPI;              // logical resolution of display
extern HPALETTE g_hpalVGA;              // only exist if palette device
void GetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm);
void SetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm);
void PASCAL Look_RebuildSysStuff(BOOL fInit);
#define NT40_COLOR_ENDCOLORS         COLOR_GRADIENTINACTIVECAPTION
#define NT40_COLOR_MAX               (NT40_COLOR_ENDCOLORS+1)
#define NT40_COLOR_MAX_REAL          (COLOR_INFOBK + 1)
extern const TCHAR c_szRegPathUserMetrics[];
extern const TCHAR c_szRegValIconSize[];
extern const TCHAR c_szRegValSmallIconSize[];

