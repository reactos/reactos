//**************************************************************************
// wusercli.h : prototypes for thunks that may be handled on 16bit side
//
//**************************************************************************

ULONG FASTCALL WU32DefHookProc(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetKeyState(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetKeyboardState(PVDMFRAME pFrame);


#define WU32CLIENTTOSCREEN             WU32ClientToScreen
#define WU32GETCLASSNAME               IT(GetClassName)
#define WU32GETCLIENTRECT              WU32GetClientRect
#define WU32GETCURSORPOS               WU32GetCursorPos
#define WU32GETDESKTOPWINDOW           WU32GetDesktopWindow
#define WU32GETDLGITEM                 WU32GetDlgItem
#define WU32GETMENU                    WU32GetMenu
#define WU32GETMENUITEMCOUNT           WU32GetMenuItemCount
#define WU32GETMENUITEMID              IT(GetMenuItemID)
#define WU32GETMENUSTATE               IT(GetMenuState)
#define WU32GETNEXTWINDOW              IT(GetWindow)
#define WU32GETPARENT                  IT(GetParent)
#define WU32GETSUBMENU                 IT(GetSubMenu)
#define WU32GETSYSCOLOR                WU32GetSysColor
#define WU32GETSYSTEMMETRICS           WU32GetSystemMetrics
#define WU32GETTICKCOUNT               WU32GetTickCount
#define WU32GETTOPWINDOW               WU32GetTopWindow
#define WU32GETWINDOW                  IT(GetWindow)
#define WU32GETWINDOWRECT              WU32GetWindowRect
#define WU32ISCHILD                    WU32IsChild
#define WU32ISICONIC                   WU32IsIconic
#define WU32ISWINDOW                   WU32IsWindow 
#define WU32ISWINDOWENABLED            WU32IsWindowEnabled
#define WU32ISWINDOWVISIBLE            IT(IsWindowVisible)
#define WU32ISZOOMED                   WU32IsZoomed
#define WU32SCREENTOCLIENT             WU32ScreenToClient

ULONG FASTCALL WU32ClientToScreen(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetClientRect(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetCursorPos(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetDesktopWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetDlgItem(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetMenu(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetMenuItemCount(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetMenuItemID(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetSysColor(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetSystemMetrics(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetTopWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetWindowRect(PVDMFRAME pFrame);
ULONG FASTCALL WU32ScreenToClient(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsChild(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsIconic(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsWindow(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsWindowEnabled(PVDMFRAME pFrame);
ULONG FASTCALL WU32IsZoomed(PVDMFRAME pFrame);
ULONG FASTCALL WU32GetTickCount(PVDMFRAME pFrame);

