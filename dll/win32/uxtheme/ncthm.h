
typedef struct _DRAW_CONTEXT
{
    HWND hWnd;
    HDC hDC;
    HTHEME theme; 
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

#define HASSIZEGRIP(Style, ExStyle, ParentStyle, WindowRect, ParentClientRect) \
            ((!(Style & WS_CHILD) && (Style & WS_THICKFRAME) && !(Style & WS_MAXIMIZE))  || \
             ((Style & WS_CHILD) && (ParentStyle & WS_THICKFRAME) && !(ParentStyle & WS_MAXIMIZE) && \
             (WindowRect.right - WindowRect.left == ParentClientRect.right) && \
             (WindowRect.bottom - WindowRect.top == ParentClientRect.bottom)))

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))

#define BUTTON_GAP_SIZE 2

#define MENU_BAR_ITEMS_SPACE (12)
