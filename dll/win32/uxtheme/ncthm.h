
typedef struct _WND_CONTEXT
{
    BOOL HasAppDefinedRgn;
    BOOL HasThemeRgn;
    BOOL UpdatingRgn;
} WND_CONTEXT, *PWND_CONTEXT;

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
The following values specify all possible vutton states
Note that not all of them are documented but it is easy to 
find them by opening a theme file
*/
typedef enum {
    BUTTON_NORMAL = 1 ,
    BUTTON_HOT ,
    BUTTON_PRESSED ,
    BUTTON_DISABLED ,
    BUTTON_INACTIVE
} THEME_BUTTON_STATES;

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

void 
ThemeDrawScrollBar(PDRAW_CONTEXT pcontext, INT Bar, POINT* pt);

VOID
NC_TrackScrollBar(HWND Wnd, WPARAM wParam, POINT Pt);

void
ThemeInitDrawContext(PDRAW_CONTEXT pcontext,
                     HWND hWnd,
                     HRGN hRgn);

void
ThemeCleanupDrawContext(PDRAW_CONTEXT pcontext);

PWND_CONTEXT 
ThemeGetWndContext(HWND hWnd);

extern ATOM atWindowTheme;
extern ATOM atWndContrext;

