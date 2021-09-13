#ifndef _UXTHEME_PCH_
#define _UXTHEME_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <windowsx.h>
#include <undocuser.h>
#include <undocgdi.h>
#include <uxtheme.h>
#include <uxundoc.h>
#include <vfwmsgs.h>
#include <tmschema.h>

#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <ndk/rtltypes.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

#define TMT_ENUM 200

#define MAX_THEME_APP_NAME 60
#define MAX_THEME_CLASS_NAME 60
#define MAX_THEME_VALUE_NAME 60

typedef struct _THEME_PROPERTY {
    int iPrimitiveType;
    int iPropertyId;
    PROPERTYORIGIN origin;

    LPCWSTR lpValue;
    DWORD dwValueLen;

    struct _THEME_PROPERTY *next;
} THEME_PROPERTY, *PTHEME_PROPERTY;

typedef struct _THEME_PARTSTATE {
    int iPartId;
    int iStateId;
    PTHEME_PROPERTY properties;

    struct _THEME_PARTSTATE *next;
} THEME_PARTSTATE, *PTHEME_PARTSTATE;

struct _THEME_FILE;

typedef struct _THEME_CLASS {
    HMODULE hTheme;
    struct _THEME_FILE* tf;
    WCHAR szAppName[MAX_THEME_APP_NAME];
    WCHAR szClassName[MAX_THEME_CLASS_NAME];
    PTHEME_PARTSTATE partstate;
    struct _THEME_CLASS *overrides;

    struct _THEME_CLASS *next;
} THEME_CLASS, *PTHEME_CLASS;

typedef struct _THEME_IMAGE {
    WCHAR name[MAX_PATH];
    HBITMAP image;
    BOOL hasAlpha;

    struct _THEME_IMAGE *next;
} THEME_IMAGE, *PTHEME_IMAGE;

typedef struct _THEME_FILE {
    DWORD dwRefCount;
    HMODULE hTheme;
    WCHAR szThemeFile[MAX_PATH];
    LPWSTR pszAvailColors;
    LPWSTR pszAvailSizes;

    LPWSTR pszSelectedColor;
    LPWSTR pszSelectedSize;

    PTHEME_CLASS classes;
    PTHEME_PROPERTY metrics;
    PTHEME_IMAGE images;
} THEME_FILE, *PTHEME_FILE;

typedef struct _UXINI_FILE *PUXINI_FILE;

typedef struct _UXTHEME_HANDLE
{
    RTL_HANDLE_TABLE_ENTRY Handle;
    PTHEME_CLASS pClass;
} UXTHEME_HANDLE, *PUXTHEME_HANDLE;

PTHEME_CLASS ValidateHandle(HTHEME hTheme);

HRESULT UXTHEME_LoadImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, BOOL glyph,
                          HBITMAP *hBmp, RECT *bmpRect, BOOL* hasImageAlpha);

BOOL MSSTYLES_LookupProperty(LPCWSTR pszPropertyName, int *dwPrimitive, int *dwId);
BOOL MSSTYLES_LookupEnum(LPCWSTR pszValueName, int dwEnum, int *dwValue);
BOOL MSSTYLES_LookupPartState(LPCWSTR pszClass, LPCWSTR pszPart, LPCWSTR pszState, int *iPartId, int *iStateId);

HRESULT MSSTYLES_OpenThemeFile(LPCWSTR lpThemeFile, LPCWSTR pszColorName, LPCWSTR pszSizeName, PTHEME_FILE *tf);
HRESULT MSSTYLES_ReferenceTheme(PTHEME_FILE tf);
void MSSTYLES_CloseThemeFile(PTHEME_FILE tf);
void MSSTYLES_ParseThemeIni(PTHEME_FILE tf);
PTHEME_CLASS MSSTYLES_OpenThemeClass(PTHEME_FILE tf, LPCWSTR pszAppName, LPCWSTR pszClassList);
HRESULT MSSTYLES_CloseThemeClass(PTHEME_CLASS tc);
PUXINI_FILE MSSTYLES_GetThemeIni(PTHEME_FILE tf);
PTHEME_PARTSTATE MSSTYLES_FindPartState(PTHEME_CLASS tc, int iPartId, int iStateId, PTHEME_CLASS *tcNext);
PTHEME_PROPERTY MSSTYLES_FindProperty(PTHEME_CLASS tc, int iPartId, int iStateId, int iPropertyPrimitive, int iPropertyId);
PTHEME_PROPERTY MSSTYLES_FindMetric(PTHEME_FILE tf, int iPropertyPrimitive, int iPropertyId);
HBITMAP MSSTYLES_LoadBitmap(PTHEME_CLASS tc, LPCWSTR lpFilename, BOOL* hasAlpha);

HRESULT MSSTYLES_GetPropertyBool(PTHEME_PROPERTY tp, BOOL *pfVal);
HRESULT MSSTYLES_GetPropertyColor(PTHEME_PROPERTY tp, COLORREF *pColor);
HRESULT MSSTYLES_GetPropertyFont(PTHEME_PROPERTY tp, HDC hdc, LOGFONTW *pFont);
HRESULT MSSTYLES_GetPropertyInt(PTHEME_PROPERTY tp, int *piVal);
HRESULT MSSTYLES_GetPropertyIntList(PTHEME_PROPERTY tp, INTLIST *pIntList);
HRESULT MSSTYLES_GetPropertyPosition(PTHEME_PROPERTY tp, POINT *pPoint);
HRESULT MSSTYLES_GetPropertyString(PTHEME_PROPERTY tp, LPWSTR pszBuff, int cchMaxBuffChars);
HRESULT MSSTYLES_GetPropertyRect(PTHEME_PROPERTY tp, RECT *pRect);
HRESULT MSSTYLES_GetPropertyMargins(PTHEME_PROPERTY tp, RECT *prc, MARGINS *pMargins);

PUXINI_FILE UXINI_LoadINI(HMODULE hTheme, LPCWSTR lpName);
void UXINI_CloseINI(PUXINI_FILE uf);
LPCWSTR UXINI_GetNextSection(PUXINI_FILE uf, DWORD *dwLen);
BOOL UXINI_FindSection(PUXINI_FILE uf, LPCWSTR lpName);
LPCWSTR UXINI_GetNextValue(PUXINI_FILE uf, DWORD *dwNameLen, LPCWSTR *lpValue, DWORD *dwValueLen);
BOOL UXINI_FindValue(PUXINI_FILE uf, LPCWSTR lpName, LPCWSTR *lpValue, DWORD *dwValueLen);

  /* Scroll-bar hit testing */
enum SCROLL_HITTEST
{
    SCROLL_NOWHERE,      /* Outside the scroll bar */
    SCROLL_TOP_ARROW,    /* Top or left arrow */
    SCROLL_TOP_RECT,     /* Rectangle between the top arrow and the thumb */
    SCROLL_THUMB,        /* Thumb rectangle */
    SCROLL_BOTTOM_RECT,  /* Rectangle between the thumb and the bottom arrow */
    SCROLL_BOTTOM_ARROW  /* Bottom or right arrow */
};

/* The window context stores data for the window needed through the life of the window */
typedef struct _WND_DATA
{
    HTHEME hthemeWindow;
    HTHEME hthemeScrollbar;

    RECT rcCaptionButtons[4];
    UINT lastHitTest;
    BOOL HasAppDefinedRgn;
    BOOL HasThemeRgn;
    BOOL UpdatingRgn;
    BOOL DirtyThemeRegion;
    HBRUSH hTabBackgroundBrush;
    HBITMAP hTabBackgroundBmp;

    BOOL SCROLL_trackVertical;
    enum SCROLL_HITTEST SCROLL_trackHitTest;
    BOOL SCROLL_MovingThumb;  /* Is the moving thumb being displayed? */
    HWND SCROLL_TrackingWin;
    INT  SCROLL_TrackingBar;
    INT  SCROLL_TrackingPos;
    INT  SCROLL_TrackingVal;
} WND_DATA, *PWND_DATA;

/* The draw context stores data that are needed by the drawing operations in the non client area of the window */
typedef struct _DRAW_CONTEXT
{
    HWND hWnd;
    HDC hDC;
    HTHEME theme;
    HTHEME scrolltheme;
    HTHEME hPrevTheme;
    WINDOWINFO wi;
    BOOL Active; /* wi.dwWindowStatus isn't correct for mdi child windows */
    HRGN hRgn;
    int CaptionHeight;

    /* for double buffering */
    HDC hDCScreen;
    HBITMAP hbmpOld;
} DRAW_CONTEXT, *PDRAW_CONTEXT;

typedef enum
{
    CLOSEBUTTON,
    MAXBUTTON,
    MINBUTTON,
    HELPBUTTON
} CAPTIONBUTTON;

/*
The following values specify all possible button states
Note that not all of them are documented but it is easy to
find them by opening a theme file
*/
typedef enum {
    BUTTON_NORMAL = 1 ,
    BUTTON_HOT ,
    BUTTON_PRESSED ,
    BUTTON_DISABLED ,
    BUTTON_INACTIVE ,
    BUTTON_INACTIVE_HOT ,
    BUTTON_INACTIVE_PRESSED ,
    BUTTON_INACTIVE_DISABLED
} THEME_BUTTON_STATES;

#define HT_ISBUTTON(ht) ((ht) == HTMINBUTTON || (ht) == HTMAXBUTTON || (ht) == HTCLOSE || (ht) == HTHELP)

#define HASSIZEGRIP(Style, ExStyle, ParentStyle, WindowRect, ParentClientRect) \
            ((!(Style & WS_CHILD) && (Style & WS_THICKFRAME) && !(Style & WS_MAXIMIZE))  || \
             ((Style & WS_CHILD) && (ParentStyle & WS_THICKFRAME) && !(ParentStyle & WS_MAXIMIZE) && \
             (WindowRect.right - WindowRect.left == ParentClientRect.right) && \
             (WindowRect.bottom - WindowRect.top == ParentClientRect.bottom)))

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))

#define BUTTON_GAP_SIZE 2

#define MENU_BAR_ITEMS_SPACE (12)

#define SCROLL_TIMER   0                /* Scroll timer id */

  /* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 0

  /* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_FIRST_DELAY   200

  /* Delay (in ms) between scroll repetitions */
#define SCROLL_REPEAT_DELAY  50

/* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 6

/* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4

LRESULT CALLBACK ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC DefWndProc);
void ThemeCalculateCaptionButtonsPos(HWND hWnd, HTHEME htheme);
void  ThemeDrawScrollBar(PDRAW_CONTEXT pcontext, INT Bar, POINT* pt);
VOID NC_TrackScrollBar(HWND Wnd, WPARAM wParam, POINT Pt);
void ThemeInitDrawContext(PDRAW_CONTEXT pcontext, HWND hWnd, HRGN hRgn);
void ThemeCleanupDrawContext(PDRAW_CONTEXT pcontext);
PWND_DATA ThemeGetWndData(HWND hWnd);
HTHEME GetNCCaptionTheme(HWND hWnd, DWORD style);
HTHEME GetNCScrollbarTheme(HWND hWnd, DWORD style);

extern HINSTANCE hDllInst;
extern ATOM atWindowTheme;
extern ATOM atWndContext;
extern BOOL g_bThemeHooksActive;

void UXTHEME_InitSystem(HINSTANCE hInst);
void UXTHEME_LoadTheme(BOOL bLoad);
BOOL CALLBACK UXTHEME_broadcast_theme_changed (HWND hWnd, LPARAM enable);

/* No alpha blending */
#define ALPHABLEND_NONE             0
/* "Cheap" binary alpha blending - but possibly faster */
#define ALPHABLEND_BINARY           1
/* Full alpha blending */
#define ALPHABLEND_FULL             2

#endif /* _UXTHEME_PCH_ */
