/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_WIN_H
#define __WINE_WIN_H


typedef HANDLE HTASK;
typedef HANDLE HQUEUE;

#define STRING2ATOMA(str) HIWORD(str) ? GlobalFindAtomA(str) : (ATOM)LOWORD(str)
#define STRING2ATOMW(str) HIWORD(str) ? GlobalFindAtomW(str) : (ATOM)LOWORD(str)

#include <user32/class.h>
#include <user32/heapdup.h>
#include <user32/dce.h>

#define WND_MAGIC     0x444e4957  /* 'WIND' */

 
  /* PAINT_RedrawWindow() control flags */
#define RDW_C_USEHRGN		0x0001
#define RDW_C_DELETEHRGN	0x0002

struct tagDCE;
struct tagDC;
struct tagCLASS;

typedef struct tagWND
{
    struct tagWND *next;          /* Next sibling */
    struct tagWND *child;         /* First child */
    struct tagWND *parent;        /* Window parent (from CreateWindow) */
    struct tagWND *owner;         /* Window owner */
    struct tagCLASS  *class;         /* Window class */
    void	  *winproc;       /* Window procedure */
    HANDLE	  hmemTaskQ;      /* This should be hThread */
    DWORD         dwMagic;       /* Magic number (must be WND_MAGIC) */
    HWND          hwndSelf;      /* Handle of this window */
    HINSTANCE     hInstance;     /* Window hInstance (from CreateWindow) */
    RECT          rectClient;    /* Client area rel. to parent client area */
    RECT          rectWindow;    /* Whole window rel. to parent client area */
    void          *text;          /* Window text */
    void          *pVScroll;      /* Vertical scroll-bar info */
    void          *pHScroll;      /* Horizontal scroll-bar info */
    void          *pProp;         /* Pointer to properties list */
    struct tagDCE *dce;           /* Window DCE (if CS_OWNDC or CS_CLASSDC) */
    HRGN          hrgnUpdate;    /* Update region */
    HRGN          hrgnWindow;   /* region where the os permits drawing == max update region */
    HWND          hwndLastActive;/* Last active popup hwnd */
    DWORD         dwStyle;       /* Window style (from CreateWindow) */
    DWORD         dwExStyle;     /* Extended style (from CreateWindowEx) */
    UINT          wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD         helpContext;   /* Help context ID */
    WORD          flags;         /* Misc. flags (see below) */
    HMENU         hSysMenu;      /* window's copy of System Menu */
    DWORD         userdata;      /* User private data */
    DWORD         wExtra[1];     /* Window extra bytes */
} WND;

typedef struct tagCREATESTRUCTA { 
  LPVOID    lpCreateParams;  
  HINSTANCE hInstance;       
  HMENU     hMenu;           
  HWND      hWndParent;      
  int       cy;              
  int       cx;              
  int       y;               
  int       x;               
  LONG      style;           
  LPCSTR    lpszName;        
  LPCSTR    lpszClass;       
  DWORD     dwExStyle;       
} CREATESTRUCTA, *LPCREATESTRUCTA; 

typedef struct tagCREATESTRUCTW { 
  LPVOID    lpCreateParams;  
  HINSTANCE hInstance;       
  HMENU     hMenu;           
  HWND      hWndParent;      
  int       cy;              
  int       cx;              
  int       y;               
  int       x;               
  LONG      style;           
  LPCWSTR   lpszName;        
  LPCWSTR   lpszClass;       
  DWORD     dwExStyle;       
} CREATESTRUCTW, *LPCREATESTRUCTW; 

typedef struct
{
    CREATESTRUCTA *lpcs;
    HWND           hwndInsertAfter;
} CBT_CREATEWNDA, *LPCBT_CREATEWNDA;

typedef struct
{
    CREATESTRUCTW *lpcs;
    HWND           hwndInsertAfter;
} CBT_CREATEWNDW, *LPCBT_CREATEWNDW;

typedef struct _STARTUPINFOW { 
  DWORD   cb; 
  LPWSTR  lpReserved; 
  LPWSTR  lpDesktop; 
  LPWSTR  lpTitle; 
  DWORD   dwX; 
  DWORD   dwY; 
  DWORD   dwXSize; 
  DWORD   dwYSize; 
  DWORD   dwXCountChars; 
  DWORD   dwYCountChars; 
  DWORD   dwFillAttribute; 
  DWORD   dwFlags; 
  WORD    wShowWindow; 
  WORD    cbReserved2; 
  LPBYTE  lpReserved2; 
  HANDLE  hStdInput; 
  HANDLE  hStdOutput; 
  HANDLE  hStdError; 
} STARTUPINFOW, *LPSTARTUPINFOW; 



typedef struct
{
    RECT	   rectNormal;
    POINT	   ptIconPos;
    POINT	   ptMaxPos;
    HWND	   hwndIconTitle;
} INTERNALPOS, *LPINTERNALPOS;

  /* WND flags values */
#define WIN_NEEDS_BEGINPAINT   0x0001 /* WM_PAINT sent to window */
#define WIN_NEEDS_ERASEBKGND   0x0002 /* WM_ERASEBKGND must be sent to window*/
#define WIN_NEEDS_NCPAINT      0x0004 /* WM_NCPAINT must be sent to window */
#define WIN_RESTORE_MAX        0x0008 /* Maximize when restoring */
#define WIN_INTERNAL_PAINT     0x0010 /* Internal WM_PAINT message pending */
/* Used to have WIN_NO_REDRAW  0x0020 here */
#define WIN_NEED_SIZE          0x0040 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED        0x0080 /* last WM_NCACTIVATE was positive */
#define WIN_MANAGED            0x0100 /* Window managed by the X wm */
#define WIN_ISDIALOG           0x0200 /* Window is a dialog */
#define WIN_ISWIN            0x0400 /* Understands Win32 messages */
#define WIN_SAVEUNDER_OVERRIDE 0x0800

  /* BuildWinArray() flags */
#define BWA_SKIPDISABLED	0x0001
#define BWA_SKIPHIDDEN		0x0002
#define BWA_SKIPOWNED		0x0004
#define BWA_SKIPICONIC		0x0008


#define SWP_DEFERERASE      0x2000

  /* Offsets for GetWindowLong() and GetWindowWord() */

#define GWW_ID              (-12)
#define GWW_HWNDPARENT      (-8)
#define GWW_HINSTANCE       (-6)
#define GWL_WNDPROC         (-4)


  /* Window functions */
HANDLE WIN_CreateWindowEx( CREATESTRUCTW *cs, ATOM atomName );
#define WIN_FindWndPtr(hwnd) (WND *)hwnd
//WND*   WIN_FindWndPtr( HWND hwnd );
WND*   WIN_GetDesktop(void);
void   WIN_DumpWindow( HWND hwnd );
void   WIN_WalkWindows( HWND hwnd, int indent );
WINBOOL WIN_UnlinkWindow( HWND hwnd );
WINBOOL WIN_LinkWindow( HWND hwnd, HWND hwndInsertAfter );
HWND WIN_FindWinToRepaint( HWND hwnd, HQUEUE hQueue );
WINBOOL WIN_ResetQueueWindows( WND* wnd, HQUEUE hQueue, HQUEUE hNew);
WINBOOL WIN_CreateDesktopWindow(void);
HWND WIN_GetTopParent( HWND hwnd );
WND*   WIN_GetTopParentPtr( WND* pWnd );
WINBOOL WIN_IsWindowDrawable(WND*, WINBOOL );
HINSTANCE WIN_GetWindowInstance( HWND hwnd );
WND**  WIN_BuildWinArray( WND *wndPtr, UINT bwa, UINT* pnum );

void WIN_UpdateNCArea(WND* wnd, WINBOOL bUpdate);

void WIN_SendDestroyMsg( WND* pWnd );
WINBOOL WIN_DestroyWindow( WND* wndPtr );
HWND WIN_FindWindow( HWND parent, HWND child, ATOM className,
                              LPCWSTR title );
LONG WIN_GetWindowLong( HWND hwnd, INT offset );
LONG WIN_SetWindowLong( HWND hwnd, INT offset, LONG newval );

WINBOOL WIN_EnumChildWindows( WND **ppWnd, ENUMWINDOWSPROC func,
                                    LPARAM lParam );


#endif  /* __WINE_WIN_H */
