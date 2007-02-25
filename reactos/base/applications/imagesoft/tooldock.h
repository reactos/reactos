
typedef enum
{
    TOP_DOCK = 0,
    LEFT_DOCK,
    RIGHT_DOCK,
    BOTTOM_DOCK,
    NO_DOCK
} DOCK_POSITION;

typedef struct _DOCKBAR
{
    UINT BarId;
    LPCTSTR lpName;
    UINT DisplayTextId;
    DOCK_POSITION Position;
} DOCKBAR, *PDOCKBAR;

struct _TOOLBAR_DOCKS;

typedef BOOL (CALLBACK *PDOCKBAR_CREATECLIENT)(struct _TOOLBAR_DOCKS *TbDocks,
                                               const DOCKBAR *Dockbar,
                                               PVOID Context,
                                               HWND hParent,
                                               HWND *hwnd);
typedef BOOL (CALLBACK *PDOCKBAR_DESTROYCLIENT)(struct _TOOLBAR_DOCKS *TbDocks,
                                                const DOCKBAR *Dockbar,
                                                PVOID Context,
                                                HWND hwnd);
typedef BOOL (CALLBACK *PDOCKBAR_INSERTBAND)(struct _TOOLBAR_DOCKS *TbDocks,
                                             const DOCKBAR *Dockbar,
                                             PVOID Context,
                                             UINT *Index,
                                             LPREBARBANDINFO rbi);
typedef VOID (CALLBACK *PDOCKBAR_DOCKBAND)(struct _TOOLBAR_DOCKS *TbDocks,
                                           const DOCKBAR *Dockbar,
                                           PVOID Context,
                                           DOCK_POSITION DockFrom,
                                           DOCK_POSITION DockTo,
                                           LPREBARBANDINFO rbi);
typedef VOID (CALLBACK *PDOCKBAR_CHEVRONPUSHED)(struct _TOOLBAR_DOCKS *TbDocks,
                                                const DOCKBAR *Dockbar,
                                                PVOID Context,
                                                HWND hwndChild,
                                                LPNMREBARCHEVRON lpnm);

typedef struct _DOCKBAR_ITEM_CALLBACKS
{
    PDOCKBAR_CREATECLIENT CreateClient;
    PDOCKBAR_DESTROYCLIENT DestroyClient;
    PDOCKBAR_INSERTBAND InsertBand;
    PDOCKBAR_DOCKBAND DockBand;
    PDOCKBAR_CHEVRONPUSHED ChevronPushed;
} DOCKBAR_ITEM_CALLBACKS, *PDOCKBAR_ITEM_CALLBACKS;

typedef struct _DOCKBAR_ITEM
{
    struct _DOCKBAR_ITEM *Next;
    DOCKBAR DockBar;
    PVOID Context;
    HWND hWndTool;
    HWND hWndClient;
    DOCK_POSITION PrevDock;
    UINT PrevBandIndex;
    const DOCKBAR_ITEM_CALLBACKS *Callbacks;
} DOCKBAR_ITEM, *PDOCKBAR_ITEM;

typedef VOID (CALLBACK *PDOCKBAR_PARENTRESIZE)(PVOID Context,
                                               WORD cx,
                                               WORD cy);

#define DOCKS_COUNT 4
typedef struct _TOOLBAR_DOCKS
{
    HWND hParent;
    PVOID Context;
    HWND hRebar[DOCKS_COUNT];
    RECT rcRebar[DOCKS_COUNT];
    RECT rcClient;
    PDOCKBAR_ITEM Items;
    PDOCKBAR_PARENTRESIZE ParentResize;
    PDOCKBAR_ITEM Dragging;
    UINT DraggingBandId;
    TCHAR szTempText[255];
} TOOLBAR_DOCKS, *PTOOLBAR_DOCKS;

VOID TbdInitializeDocks(PTOOLBAR_DOCKS TbDocks,
                        HWND hWndParent,
                        PVOID Context,
                        PDOCKBAR_PARENTRESIZE ParentResizeProc);
INT TbdAdjustUpdateClientRect(PTOOLBAR_DOCKS TbDocks,
                              PRECT rcClient);
HDWP TbdDeferDocks(HDWP hWinPosInfo,
                   PTOOLBAR_DOCKS TbDocks);
BOOL TbdAddToolbar(PTOOLBAR_DOCKS TbDocks,
                   const DOCKBAR *Dockbar,
                   PVOID Context,
                   const DOCKBAR_ITEM_CALLBACKS *DockbarCallbacks);
BOOL TbdDockBarIdFromClientWindow(PTOOLBAR_DOCKS TbDocks,
                                  HWND hWndClient,
                                  UINT *Id);
BOOL TbdHandleNotifications(PTOOLBAR_DOCKS TbDocks,
                            LPNMHDR pnmh,
                            LRESULT *Result);
VOID TbdHandleEnabling(PTOOLBAR_DOCKS TbDocks,
                       HWND hWnd,
                       BOOL Enable);
VOID TbdHandleActivation(PTOOLBAR_DOCKS TbDocks,
                         HWND hWnd,
                         WPARAM *wParam,
                         LPARAM *lParam);
VOID TbdShowFloatingToolbars(PTOOLBAR_DOCKS TbDocks,
                             BOOL Show);
BOOL TbdInitImpl(VOID);
VOID TbdUninitImpl(VOID);
