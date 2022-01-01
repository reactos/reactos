#ifndef __WIN32K_NTUSER_H
#define __WIN32K_NTUSER_H

struct _PROCESSINFO;
struct _THREADINFO;
struct _DESKTOP;
struct _WND;
struct tagPOPUPMENU;

#define FIRST_USER_HANDLE 0x0020 /* first possible value for low word of user handle */
#define LAST_USER_HANDLE 0xffef /* last possible value for low word of user handle */

#define HANDLEENTRY_DESTROY 1
#define HANDLEENTRY_INDESTROY 2

typedef struct _USER_HANDLE_ENTRY
{
    void *ptr; /* pointer to object */
    union
    {
        PVOID pi;
        struct _THREADINFO *pti; /* pointer to Win32ThreadInfo */
        struct _PROCESSINFO *ppi; /* pointer to W32ProcessInfo */
    };
    unsigned char type; /* object type (0 if free) */
    unsigned char flags;
    unsigned short generation; /* generation counter */
} USER_HANDLE_ENTRY, *PUSER_HANDLE_ENTRY;

typedef struct _USER_HANDLE_TABLE
{
    PUSER_HANDLE_ENTRY handles;
    PUSER_HANDLE_ENTRY freelist;
    int nb_handles;
    int allocated_handles;
} USER_HANDLE_TABLE, *PUSER_HANDLE_TABLE;

typedef enum _HANDLE_TYPE
{
    TYPE_FREE = 0,
    TYPE_WINDOW = 1,
    TYPE_MENU = 2,
    TYPE_CURSOR = 3,
    TYPE_SETWINDOWPOS = 4,
    TYPE_HOOK = 5,
    TYPE_CLIPDATA = 6,
    TYPE_CALLPROC = 7,
    TYPE_ACCELTABLE = 8,
    TYPE_DDEACCESS = 9,
    TYPE_DDECONV = 10,
    TYPE_DDEXACT = 11,
    TYPE_MONITOR = 12,
    TYPE_KBDLAYOUT = 13,
    TYPE_KBDFILE = 14,
    TYPE_WINEVENTHOOK = 15,
    TYPE_TIMER = 16,
    TYPE_INPUTCONTEXT = 17,
    TYPE_HIDDATA = 18,
    TYPE_DEVICEINFO = 19,
    TYPE_TOUCHINPUTINFO = 20,
    TYPE_GESTUREINFOOBJ = 21,
    TYPE_CTYPES,
    TYPE_GENERIC = 255
} HANDLE_TYPE, *PHANDLE_TYPE;

typedef enum _USERTHREADINFOCLASS
{
    UserThreadShutdownInformation,
    UserThreadFlags,
    UserThreadTaskName,
    UserThreadWOWInformation,
    UserThreadHungStatus,
    UserThreadInitiateShutdown,
    UserThreadEndShutdown,
    UserThreadUseActiveDesktop,
    UserThreadUseDesktop,
    UserThreadRestoreDesktop,
    UserThreadCsrApiPort,
} USERTHREADINFOCLASS;

typedef struct _LARGE_UNICODE_STRING
{
    ULONG Length;
    ULONG MaximumLength:31;
    ULONG bAnsi:1;
    PWSTR Buffer;
} LARGE_UNICODE_STRING, *PLARGE_UNICODE_STRING;

typedef struct _LARGE_STRING
{
    ULONG Length;
    ULONG MaximumLength:31;
    ULONG bAnsi:1;
    PVOID Buffer;
} LARGE_STRING, *PLARGE_STRING;


/* Based on ANSI_STRING */
typedef struct _LARGE_ANSI_STRING
{
    ULONG Length;
    ULONG MaximumLength:31;
    ULONG bAnsi:1;
    PCHAR Buffer;
} LARGE_ANSI_STRING, *PLARGE_ANSI_STRING;

VOID
NTAPI
RtlInitLargeAnsiString(
    IN OUT PLARGE_ANSI_STRING,
    IN PCSZ,
    IN INT);

VOID
NTAPI
RtlInitLargeUnicodeString(
    IN OUT PLARGE_UNICODE_STRING,
    IN PCWSTR,
    IN INT);

BOOL
NTAPI
RtlLargeStringToUnicodeString(
    PUNICODE_STRING,
    PLARGE_STRING);

#define NB_HOOKS (WH_MAXHOOK - WH_MINHOOK + 1)

typedef struct _DESKTOPINFO
{
    PVOID pvDesktopBase;
    PVOID pvDesktopLimit;
    struct _WND *spwnd;
    DWORD fsHooks;
    LIST_ENTRY aphkStart[NB_HOOKS];

    HWND hTaskManWindow;
    HWND hProgmanWindow;
    HWND hShellWindow;
    struct _WND *spwndShell;
    struct _WND *spwndBkGnd;

    struct _PROCESSINFO *ppiShellProcess;

    union
    {
        UINT Dummy;
        struct
        {
            UINT LastInputWasKbd:1;
        };
    };

    WCHAR szDesktopName[1];
} DESKTOPINFO, *PDESKTOPINFO;

#define CTI_THREADSYSLOCK 0x0001
#define CTI_INSENDMESSAGE 0x0002

typedef struct _CLIENTTHREADINFO
{
    DWORD CTI_flags;
    WORD fsChangeBits;
    WORD fsWakeBits;
    WORD fsWakeBitsJournal;
    WORD fsWakeMask;
    ULONG timeLastRead; // Last time the message queue was read.
    DWORD dwcPumpHook;
} CLIENTTHREADINFO, *PCLIENTTHREADINFO;

typedef struct _HEAD
{
    HANDLE h;
    DWORD cLockObj;
} HEAD, *PHEAD;

typedef struct _THROBJHEAD
{
    HEAD;
    struct _THREADINFO *pti;
} THROBJHEAD, *PTHROBJHEAD;

typedef struct _THRDESKHEAD
{
    THROBJHEAD;
    struct _DESKTOP *rpdesk;
    PVOID pSelf;
} THRDESKHEAD, *PTHRDESKHEAD;

typedef struct tagIMC
{
    THRDESKHEAD    head;
    struct tagIMC *pImcNext;
    ULONG_PTR      dwClientImcData;
    HWND           hImeWnd;
} IMC, *PIMC;

typedef struct _PROCDESKHEAD
{
    HEAD;
    DWORD_PTR hTaskWow;
    struct _DESKTOP *rpdesk;
    PVOID pSelf;
} PROCDESKHEAD, *PPROCDESKHEAD;

typedef struct _PROCMARKHEAD
{
    HEAD;
    ULONG hTaskWow;
    struct _PROCESSINFO *ppi;
} PROCMARKHEAD, *PPROCMARKHEAD;

#define UserHMGetHandle(obj) ((obj)->head.h)

/* Window Client Information structure */
struct _ETHREAD;

#define WEF_SETBYWNDPTI 0x0001

typedef struct tagHOOK
{
    THRDESKHEAD head;
    struct tagHOOK *phkNext; /* This is for user space. */
    int HookId; /* Hook table index */
    ULONG_PTR offPfn;
    ULONG flags; /* Some internal flags */
    INT_PTR ihmod;
    struct _THREADINFO *ptiHooked;
    struct _DESKTOP *rpdesk;
    /* ReactOS */
    LIST_ENTRY Chain; /* Hook chain entry */
    HOOKPROC Proc; /* Hook function */
    BOOLEAN Ansi; /* Is it an Ansi hook? */
    UNICODE_STRING ModuleName; /* Module name for global hooks */
} HOOK, *PHOOK;

typedef struct tagCLIPBOARDDATA
{
    HEAD head;
    DWORD cbData;
    BYTE Data[0];
} CLIPBOARDDATA, *PCLIPBOARDDATA;

/* THREADINFO Flags */
#define TIF_INCLEANUP               0x00000001
#define TIF_16BIT                   0x00000002
#define TIF_SYSTEMTHREAD            0x00000004
#define TIF_CSRSSTHREAD             0x00000008
#define TIF_TRACKRECTVISIBLE        0x00000010
#define TIF_ALLOWFOREGROUNDACTIVATE 0x00000020
#define TIF_DONTATTACHQUEUE         0x00000040
#define TIF_DONTJOURNALATTACH       0x00000080
#define TIF_WOW64                   0x00000100
#define TIF_INACTIVATEAPPMSG        0x00000200
#define TIF_SPINNING                0x00000400
#define TIF_PALETTEAWARE            0x00000800
#define TIF_SHAREDWOW               0x00001000
#define TIF_FIRSTIDLE               0x00002000
#define TIF_WAITFORINPUTIDLE        0x00004000
#define TIF_MOVESIZETRACKING        0x00008000
#define TIF_VDMAPP                  0x00010000
#define TIF_DOSEMULATOR             0x00020000
#define TIF_GLOBALHOOKER            0x00040000
#define TIF_DELAYEDEVENT            0x00080000
#define TIF_MSGPOSCHANGED           0x00100000
#define TIF_SHUTDOWNCOMPLETE        0x00200000
#define TIF_IGNOREPLAYBACKDELAY     0x00400000
#define TIF_ALLOWOTHERACCOUNTHOOK   0x00800000
#define TIF_GUITHREADINITIALIZED    0x02000000
#define TIF_DISABLEIME              0x04000000
#define TIF_INGETTEXTLENGTH         0x08000000
#define TIF_ANSILENGTH              0x10000000
#define TIF_DISABLEHOOKS            0x20000000

typedef struct _CALLBACKWND
{
    HWND hWnd;
    struct _WND *pWnd;
    PVOID pActCtx;
} CALLBACKWND, *PCALLBACKWND;

#define CI_TRANSACTION       0x00000001
#define CI_QUEUEMSG          0x00000002
#define CI_WOW               0x00000004
#define CI_INITTHREAD        0x00000008
#define CI_CURTHPRHOOK       0x00000010
#define CI_CLASSESREGISTERED 0x00000020
#define CI_IMMACTIVATE       0x00000040
#define CI_TFSDISABLED       0x00000400

typedef struct _CLIENTINFO
{
    ULONG_PTR CI_flags;
    ULONG_PTR cSpins;
    DWORD dwExpWinVer;
    DWORD dwCompatFlags;
    DWORD dwCompatFlags2;
    DWORD dwTIFlags; /* ThreadInfo TIF_Xxx flags for User space. */
    PDESKTOPINFO pDeskInfo;
    ULONG_PTR ulClientDelta;
    PHOOK phkCurrent;
    ULONG fsHooks;
    CALLBACKWND CallbackWnd;
    DWORD dwHookCurrent;
    INT cInDDEMLCallback;
    PCLIENTTHREADINFO pClientThreadInfo;
    ULONG_PTR dwHookData;
    DWORD dwKeyCache;
    BYTE afKeyState[8];
    DWORD dwAsyncKeyCache;
    BYTE afAsyncKeyState[8];
    BYTE afAsyncKeyStateRecentDow[8];
    HKL hKL;
    USHORT CodePage;
    UCHAR achDbcsCF[2];
    MSG msgDbcsCB;
    LPDWORD lpdwRegisteredClasses;
    ULONG Win32ClientInfo3[26];
/* It's just a pointer reference not to be used w the structure in user space. */
    struct _PROCESSINFO *ppi;
} CLIENTINFO, *PCLIENTINFO;

/* Make sure it fits into the TEB */
C_ASSERT(sizeof(CLIENTINFO) <= sizeof(((PTEB)0)->Win32ClientInfo));

#define GetWin32ClientInfo() ((PCLIENTINFO)(NtCurrentTeb()->Win32ClientInfo))

typedef struct tagDDEPACK
{
    UINT_PTR uiLo;
    UINT_PTR uiHi;
} DDEPACK, *PDDEPACK;

#define HRGN_NULL    ((HRGN)0) /* NULL empty region */
#define HRGN_WINDOW  ((HRGN)1) /* region from window rcWindow */
#define HRGN_MONITOR ((HRGN)2) /* region from monitor region. */

/* Menu Item fType. */
#define MFT_RTOL 0x6000

/* Menu Item fState. */
#define MFS_HBMMENUBMP 0x20000000

typedef struct tagITEM
{
    UINT fType;
    UINT fState;
    UINT wID;
    struct tagMENU *spSubMenu; /* Pop-up menu. */
    HANDLE hbmpChecked;
    HANDLE hbmpUnchecked;
    USHORT *Xlpstr; /* Item text pointer. */
    ULONG cch;
    DWORD_PTR dwItemData;
    ULONG xItem; /* Item position. left */
    ULONG yItem; /*     "          top */
    ULONG cxItem; /* Item Size Width */
    ULONG cyItem; /*     "     Height */
    ULONG dxTab; /* X position of text after Tab */
    ULONG ulX; /* underline.. start position */
    ULONG ulWidth; /* underline.. width */
    HBITMAP hbmp; /* bitmap */
    INT cxBmp; /* Width Maximum size of the bitmap items in MIIM_BITMAP state */
    INT cyBmp; /* Height " */
    /* ReactOS */
    UNICODE_STRING lpstr;
} ITEM, *PITEM;

typedef struct tagMENULIST
{
    struct tagMENULIST *pNext;
    struct tagMENU *pMenu;
} MENULIST, *PMENULIST;

/* Menu fFlags, upper byte is MNS_X style flags. */
#define MNF_POPUP      0x0001
#define MNF_UNDERLINE  0x0004
#define MNF_INACTIVE   0x0010
#define MNF_RTOL       0x0020
#define MNF_DESKTOPMN  0x0040
#define MNF_SYSDESKMN  0x0080
#define MNF_SYSSUBMENU 0x0100
/* Hack */
#define MNF_SYSMENU    0x0200

/* (other FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM 0xffff

typedef struct tagMENU
{
    PROCDESKHEAD head;
    ULONG fFlags; /* [Style flags | Menu flags] */
    INT iItem; /* nPos of selected item, if -1 not selected. AKA focused item */
    UINT cAlloced; /* Number of allocated items. Inc's of 8 */
    UINT cItems; /* Number of items in the menu */
    ULONG cxMenu; /* Width of the whole menu */
    ULONG cyMenu; /* Height of the whole menu */
    ULONG cxTextAlign; /* Offset of text when items have both bitmaps and text */
    struct _WND *spwndNotify; /* window receiving the messages for ownerdraw */
    PITEM rgItems; /* Array of menu items */
    struct tagMENULIST *pParentMenus; /* If this is SubMenu, list of parents. */
    DWORD dwContextHelpId;
    ULONG cyMax; /* max height of the whole menu, 0 is screen height */
    DWORD_PTR dwMenuData; /* application defined value */
    HBRUSH hbrBack; /* brush for menu background */
    INT iTop; /* Current scroll position Top */
    INT iMaxTop; /* Current scroll position Max Top */
    DWORD dwArrowsOn:2; /* Arrows: 0 off, 1 on, 2 to the top, 3 to the bottom. */
    /* ReactOS */
    LIST_ENTRY ListEntry;
    HWND hWnd; /* Window containing the menu, use POPUPMENU */
    BOOL TimeToHide;
} MENU, *PMENU;

typedef struct tagPOPUPMENU
{
    ULONG fIsMenuBar:1;
    ULONG fHasMenuBar:1;
    ULONG fIsSysMenu:1;
    ULONG fIsTrackPopup:1;
    ULONG fDroppedLeft:1;
    ULONG fHierarchyDropped:1;
    ULONG fRightButton:1;
    ULONG fToggle:1;
    ULONG fSynchronous:1;
    ULONG fFirstClick:1;
    ULONG fDropNextPopup:1;
    ULONG fNoNotify:1;
    ULONG fAboutToHide:1;
    ULONG fShowTimer:1;
    ULONG fHideTimer:1;
    ULONG fDestroyed:1;
    ULONG fDelayedFree:1;
    ULONG fFlushDelayedFree:1;
    ULONG fFreed:1;
    ULONG fInCancel:1;
    ULONG fTrackMouseEvent:1;
    ULONG fSendUninit:1;
    ULONG fRtoL:1;
    // ULONG fDesktopMenu:1;
    ULONG iDropDir:5;
    ULONG fUseMonitorRect:1;
    struct _WND *spwndNotify;
    struct _WND *spwndPopupMenu;
    struct _WND *spwndNextPopup;
    struct _WND *spwndPrevPopup;
    PMENU spmenu;
    PMENU spmenuAlternate;
    struct _WND *spwndActivePopup;
    struct tagPOPUPMENU *ppopupmenuRoot;
    struct tagPOPUPMENU *ppmDelayedFree;
    UINT posSelectedItem;
    UINT posDropped;
} POPUPMENU, *PPOPUPMENU;

typedef struct _REGISTER_SYSCLASS
{
    /* This is a reactos specific class used to initialize the
       system window classes during user32 initialization */
    PWSTR ClassName;
    UINT Style;
    WNDPROC ProcW;
    UINT ExtraBytes;
    HICON hCursor;
    HBRUSH hBrush;
    WORD fiId;
    WORD iCls;
} REGISTER_SYSCLASS, *PREGISTER_SYSCLASS;

typedef struct _CLSMENUNAME
{
    LPSTR pszClientAnsiMenuName;
    LPWSTR pwszClientUnicodeMenuName;
    PUNICODE_STRING pusMenuName;
} CLSMENUNAME, *PCLSMENUNAME;

typedef struct tagSBDATA
{
    INT posMin;
    INT posMax;
    INT page;
    INT pos;
} SBDATA, *PSBDATA;

typedef struct tagSBINFO
{
    INT WSBflags;
    SBDATA Horz;
    SBDATA Vert;
} SBINFO, *PSBINFO;

typedef struct tagSBCALC
{
    INT posMin;
    INT posMax;
    INT page;
    INT pos;
    INT pxTop;
    INT pxBottom;
    INT pxLeft;
    INT pxRight;
    INT cpxThumb;
    INT pxUpArrow;
    INT pxDownArrow;
    INT pxStart;
    INT pxThumbBottom;
    INT pxThumbTop;
    INT cpx;
    INT pxMin;
} SBCALC, *PSBCALC;

typedef enum _GETCPD
{
    UserGetCPDA2U = 0x01, /* " Unicode " */
    UserGetCPDU2A = 0X02, /* " Ansi " */
    UserGetCPDClass = 0X10,
    UserGetCPDWindow = 0X20,
    UserGetCPDDialog = 0X40,
    UserGetCPDWndtoCls = 0X80
} GETCPD, *PGETCPD;

typedef struct _CALLPROCDATA
{
    PROCDESKHEAD head;
    struct _CALLPROCDATA *spcpdNext;
    WNDPROC pfnClientPrevious;
    GETCPD wType;
} CALLPROCDATA, *PCALLPROCDATA;

#define CSF_SERVERSIDEPROC  0x0001
#define CSF_ANSIPROC        0x0002
#define CSF_WOWDEFERDESTROY 0x0004
#define CSF_SYSTEMCLASS     0x0008
#define CSF_WOWCLASS        0x0010
#define CSF_WOWEXTRA        0x0020
#define CSF_CACHEDSMICON    0x0040
#define CSF_WIN40COMPAT     0x0080

typedef struct _CLS
{
    struct _CLS *pclsNext;
    RTL_ATOM atomClassName;
    ATOM atomNVClassName;
    DWORD fnid;
    struct _DESKTOP *rpdeskParent;
    PVOID pdce;
    DWORD CSF_flags;
    PSTR  lpszClientAnsiMenuName; /* For client use */
    PWSTR lpszClientUnicodeMenuName; /* "   "      " */
    PCALLPROCDATA spcpdFirst;
    struct _CLS *pclsBase;
    struct _CLS *pclsClone;
    ULONG cWndReferenceCount;
    UINT style;
    WNDPROC lpfnWndProc;
    INT cbclsExtra;
    INT cbwndExtra;
    HINSTANCE hModule;
    struct _CURICON_OBJECT *spicn;
    struct _CURICON_OBJECT *spcur;
    HBRUSH hbrBackground;
    PWSTR lpszMenuName; /* kernel use */
    PSTR lpszAnsiClassName; /* " */
    struct _CURICON_OBJECT *spicnSm;
    ////
    UINT Unicode:1; // !CSF_ANSIPROC
    UINT Global:1; // CS_GLOBALCLASS or CSF_SERVERSIDEPROC
    UINT MenuNameIsString:1;
    UINT NotUsed:29;
} CLS, *PCLS;

typedef struct _SBINFOEX
{
    SCROLLBARINFO ScrollBarInfo;
    SCROLLINFO ScrollInfo;
} SBINFOEX, *PSBINFOEX;

/* State Flags !Not ALL Implemented! */
#define WNDS_HASMENU                 0X00000001
#define WNDS_HASVERTICALSCROOLLBAR   0X00000002
#define WNDS_HASHORIZONTALSCROLLBAR  0X00000004
#define WNDS_HASCAPTION              0X00000008
#define WNDS_SENDSIZEMOVEMSGS        0X00000010
#define WNDS_MSGBOX                  0X00000020
#define WNDS_ACTIVEFRAME             0X00000040
#define WNDS_HASSPB                  0X00000080
#define WNDS_NONCPAINT               0X00000100
#define WNDS_SENDERASEBACKGROUND     0X00000200
#define WNDS_ERASEBACKGROUND         0X00000400
#define WNDS_SENDNCPAINT             0X00000800
#define WNDS_INTERNALPAINT           0X00001000
#define WNDS_UPDATEDIRTY             0X00002000
#define WNDS_HIDDENPOPUP             0X00004000
#define WNDS_FORCEMENUDRAW           0X00008000
#define WNDS_DIALOGWINDOW            0X00010000
#define WNDS_HASCREATESTRUCTNAME     0X00020000
#define WNDS_SERVERSIDEWINDOWPROC    0x00040000 /* Call proc inside win32k. */
#define WNDS_ANSIWINDOWPROC          0x00080000
#define WNDS_BEINGACTIVATED          0x00100000
#define WNDS_HASPALETTE              0x00200000
#define WNDS_PAINTNOTPROCESSED       0x00400000
#define WNDS_SYNCPAINTPENDING        0x00800000
#define WNDS_RECEIVEDQUERYSUSPENDMSG 0x01000000
#define WNDS_RECEIVEDSUSPENDMSG      0x02000000
#define WNDS_TOGGLETOPMOST           0x04000000
#define WNDS_REDRAWIFHUNG            0x08000000
#define WNDS_REDRAWFRAMEIFHUNG       0x10000000
#define WNDS_ANSICREATOR             0x20000000
#define WNDS_MAXIMIZESTOMONITOR      0x40000000
#define WNDS_DESTROYED               0x80000000

#define WNDSACTIVEFRAME              0x00000006

/* State2 Flags !Not ALL Implemented! */
#define WNDS2_WMPAINTSENT               0X00000001
#define WNDS2_ENDPAINTINVALIDATE        0X00000002
#define WNDS2_STARTPAINT                0X00000004
#define WNDS2_OLDUI                     0X00000008
#define WNDS2_HASCLIENTEDGE             0X00000010
#define WNDS2_BOTTOMMOST                0X00000020
#define WNDS2_FULLSCREEN                0X00000040
#define WNDS2_INDESTROY                 0X00000080
#define WNDS2_WIN31COMPAT               0X00000100
#define WNDS2_WIN40COMPAT               0X00000200
#define WNDS2_WIN50COMPAT               0X00000400
#define WNDS2_MAXIMIZEDMONITORREGION    0X00000800
#define WNDS2_CLOSEBUTTONDOWN           0X00001000
#define WNDS2_MAXIMIZEBUTTONDOWN        0X00002000
#define WNDS2_MINIMIZEBUTTONDOWN        0X00004000
#define WNDS2_HELPBUTTONDOWN            0X00008000
#define WNDS2_SCROLLBARLINEUPBTNDOWN    0X00010000
#define WNDS2_SCROLLBARPAGEUPBTNDOWN    0X00020000
#define WNDS2_SCROLLBARPAGEDOWNBTNDOWN  0X00040000
#define WNDS2_SCROLLBARLINEDOWNBTNDOWN  0X00080000
#define WNDS2_ANYSCROLLBUTTONDOWN       0X00100000
#define WNDS2_SCROLLBARVERTICALTRACKING 0X00200000
#define WNDS2_FORCENCPAINT              0X00400000
#define WNDS2_FORCEFULLNCPAINTCLIPRGN   0X00800000
#define WNDS2_FULLSCREENMODE            0X01000000
#define WNDS2_CAPTIONTEXTTRUNCATED      0X08000000
#define WNDS2_NOMINMAXANIMATERECTS      0X10000000
#define WNDS2_SMALLICONFROMWMQUERYDRAG  0X20000000
#define WNDS2_SHELLHOOKREGISTERED       0X40000000
#define WNDS2_WMCREATEMSGPROCESSED      0X80000000

/* ExStyles2 */
#define WS_EX2_CLIPBOARDLISTENER        0X00000001
#define WS_EX2_LAYEREDINVALIDATE        0X00000002
#define WS_EX2_REDIRECTEDFORPRINT       0X00000004
#define WS_EX2_LINKED                   0X00000008
#define WS_EX2_LAYEREDFORDWM            0X00000010
#define WS_EX2_LAYEREDLIMBO             0X00000020
#define WS_EX2_HIGHTDPI_UNAWAR          0X00000040
#define WS_EX2_VERTICALLYMAXIMIZEDLEFT  0X00000080
#define WS_EX2_VERTICALLYMAXIMIZEDRIGHT 0X00000100
#define WS_EX2_HASOVERLAY               0X00000200
#define WS_EX2_CONSOLEWINDOW            0X00000400
#define WS_EX2_CHILDNOACTIVATE          0X00000800

#define WPF_MININIT 0x0008
#define WPF_MAXINIT 0x0010

typedef struct _WND
{
    THRDESKHEAD head;
#if 0
    WW ww;
#else
    /* These fields should be moved in the WW at some point. */
    /* Plese do not change them to keep the same layout with WW. */
    DWORD state;
    DWORD state2;
    /* Extended style. */
    DWORD ExStyle;
    /* Style. */
    DWORD style;
    /* Handle of the module that created the window. */
    HINSTANCE hModule;
    DWORD fnid;
#endif
    struct _WND *spwndNext;
    struct _WND *spwndPrev;
    struct _WND *spwndParent;
    struct _WND *spwndChild;
    struct _WND *spwndOwner;
    RECT rcWindow;
    RECT rcClient;
    WNDPROC lpfnWndProc;
    /* Pointer to the window class. */
    PCLS pcls;
    HRGN hrgnUpdate;
    /* Property list head.*/
    LIST_ENTRY PropListHead;
    ULONG PropListItems;
    /* Scrollbar info */
    PSBINFO pSBInfo;
    /* system menu handle. */
    HMENU SystemMenu;
    //PMENU spmenuSys;
    /* Window menu handle or window id */
    UINT_PTR IDMenu; // Use spmenu
    //PMENU spmenu;
    HRGN hrgnClip;
    HRGN hrgnNewFrame;
    /* Window name. */
    LARGE_UNICODE_STRING strName;
    /* Size of the extra data associated with the window. */
    ULONG cbwndExtra;
    struct _WND *spwndLastActive;
    HIMC hImc; // Input context associated with this window.
    LONG_PTR dwUserData;
    PVOID pActCtx;
    //PD3DMATRIX pTransForm;
    struct _WND *spwndClipboardListener;
    DWORD ExStyle2;

    /* ReactOS */
    struct
    {
        RECT NormalRect;
        POINT IconPos;
        POINT MaxPos;
        UINT flags; /* WPF_ flags. */
    } InternalPos;

    UINT Unicode:1; /* !(WNDS_ANSICREATOR|WNDS_ANSIWINDOWPROC) ? */
    UINT InternalPosInitialized:1;
    UINT HideFocus:1; /* WS_EX_UISTATEFOCUSRECTHIDDEN ? */
    UINT HideAccel:1; /* WS_EX_UISTATEKBACCELHIDDEN ? */

    /* Scrollbar info */
    PSBINFOEX pSBInfoex; // convert to PSBINFO
    /* Entry in the list of thread windows. */
    LIST_ENTRY ThreadListEntry;

    PVOID DialogPointer;
} WND, *PWND;

#define PWND_BOTTOM ((PWND)1)

typedef struct _SBWND
{
    WND wnd;
    BOOL fVert;
    UINT wDisableFlags;
    SBCALC SBCalc;
} SBWND, *PSBWND;

typedef struct _MDIWND
{
  WND wnd;
  DWORD dwReserved;
  PVOID pmdi;
} MDIWND, *PMDIWND;

#define GWLP_MDIWND 4

typedef struct _MENUWND
{
    WND wnd;
    PPOPUPMENU ppopupmenu;
} MENUWND, *PMENUWND;

typedef struct _PFNCLIENT
{
    WNDPROC pfnScrollBarWndProc;
    WNDPROC pfnTitleWndProc;
    WNDPROC pfnMenuWndProc;
    WNDPROC pfnDesktopWndProc;
    WNDPROC pfnDefWindowProc;
    WNDPROC pfnMessageWindowProc;
    WNDPROC pfnSwitchWindowProc;
    WNDPROC pfnButtonWndProc;
    WNDPROC pfnComboBoxWndProc;
    WNDPROC pfnComboListBoxProc;
    WNDPROC pfnDialogWndProc;
    WNDPROC pfnEditWndProc;
    WNDPROC pfnListBoxWndProc;
    WNDPROC pfnMDIClientWndProc;
    WNDPROC pfnStaticWndProc;
    WNDPROC pfnImeWndProc;
    WNDPROC pfnGhostWndProc;
    WNDPROC pfnHkINLPCWPSTRUCT;
    WNDPROC pfnHkINLPCWPRETSTRUCT;
    WNDPROC pfnDispatchHook;
    WNDPROC pfnDispatchDefWindowProc;
    WNDPROC pfnDispatchMessage;
    WNDPROC pfnMDIActivateDlgProc;
} PFNCLIENT, *PPFNCLIENT;

/*
  Wine Common proc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL Unicode );
  Windows uses Ansi == TRUE, Wine uses Unicode == TRUE.
 */

typedef LRESULT
(CALLBACK *WNDPROC_EX)(
    HWND,
    UINT,
    WPARAM,
    LPARAM,
    BOOL);

typedef struct _PFNCLIENTWORKER
{
    WNDPROC_EX pfnButtonWndProc;
    WNDPROC_EX pfnComboBoxWndProc;
    WNDPROC_EX pfnComboListBoxProc;
    WNDPROC_EX pfnDialogWndProc;
    WNDPROC_EX pfnEditWndProc;
    WNDPROC_EX pfnListBoxWndProc;
    WNDPROC_EX pfnMDIClientWndProc;
    WNDPROC_EX pfnStaticWndProc;
    WNDPROC_EX pfnImeWndProc;
    WNDPROC_EX pfnGhostWndProc;
    WNDPROC_EX pfnCtfHookProc;
} PFNCLIENTWORKER, *PPFNCLIENTWORKER;

typedef LONG_PTR
(NTAPI *PFN_FNID)(
    PWND,
    UINT,
    WPARAM,
    LPARAM,
    ULONG_PTR);

/* FNID's for NtUserSetWindowFNID, NtUserMessageCall */
#define FNID_FIRST                  0x029A
#define FNID_SCROLLBAR              0x029A
#define FNID_ICONTITLE              0x029B
#define FNID_MENU                   0x029C
#define FNID_DESKTOP                0x029D
#define FNID_DEFWINDOWPROC          0x029E
#define FNID_MESSAGEWND             0x029F
#define FNID_SWITCH                 0x02A0
#define FNID_BUTTON                 0x02A1
#define FNID_COMBOBOX               0x02A2
#define FNID_COMBOLBOX              0x02A3
#define FNID_DIALOG                 0x02A4
#define FNID_EDIT                   0x02A5
#define FNID_LISTBOX                0x02A6
#define FNID_MDICLIENT              0x02A7
#define FNID_STATIC                 0x02A8
#define FNID_IME                    0x02A9
#define FNID_GHOST                  0x02AA
#define FNID_CALLWNDPROC            0x02AB
#define FNID_CALLWNDPROCRET         0x02AC
#define FNID_HKINLPCWPEXSTRUCT      0x02AD
#define FNID_HKINLPCWPRETEXSTRUCT   0x02AE
#define FNID_MB_DLGPROC             0x02AF
#define FNID_MDIACTIVATEDLGPROC     0x02B0
#define FNID_SENDMESSAGE            0x02B1
#define FNID_SENDMESSAGEFF          0x02B2
/* Kernel has option to use TimeOut or normal msg send, based on type of msg. */
#define FNID_SENDMESSAGEWTOOPTION   0x02B3
#define FNID_SENDMESSAGECALLPROC    0x02B4
#define FNID_BROADCASTSYSTEMMESSAGE 0x02B5
#define FNID_TOOLTIPS               0x02B6
#define FNID_SENDNOTIFYMESSAGE      0x02B7
#define FNID_SENDMESSAGECALLBACK    0x02B8

#define FNID_LAST                   FNID_SENDMESSAGECALLBACK

#define FNID_NUM                    (FNID_LAST - FNID_FIRST + 1)
#define FNID_NUMSERVERPROC          (FNID_SWITCH - FNID_FIRST + 1)

#define FNID_DDEML   0x2000 /* Registers DDEML */
#define FNID_DESTROY 0x4000 /* This is sent when WM_NCDESTROY or in the support routine. */
                            /* Seen during WM_CREATE on error exit too. */
#define FNID_FREED   0x8000 /* Window being Freed... */

#define ICLASS_TO_MASK(iCls) (1 << ((iCls)))

#define GETPFNCLIENTA(fnid) \
 (WNDPROC)(*(((ULONG_PTR *)&gpsi->apfnClientA) + (fnid - FNID_FIRST)))
#define GETPFNCLIENTW(fnid) \
 (WNDPROC)(*(((ULONG_PTR *)&gpsi->apfnClientW) + (fnid - FNID_FIRST)))

#define GETPFNSERVER(fnid) gpsi->aStoCidPfn[fnid - FNID_FIRST]

/* ICLS's for NtUserGetClassName FNID to ICLS, NtUserInitializeClientPfnArrays */
#define ICLS_BUTTON       0
#define ICLS_EDIT         1
#define ICLS_STATIC       2
#define ICLS_LISTBOX      3
#define ICLS_SCROLLBAR    4
#define ICLS_COMBOBOX     5
#define ICLS_MDICLIENT    6
#define ICLS_COMBOLBOX    7
#define ICLS_DDEMLEVENT   8
#define ICLS_DDEMLMOTHER  9
#define ICLS_DDEML16BIT   10
#define ICLS_DDEMLCLIENTA 11
#define ICLS_DDEMLCLIENTW 12
#define ICLS_DDEMLSERVERA 13
#define ICLS_DDEMLSERVERW 14
#define ICLS_IME          15
#define ICLS_GHOST        16
#define ICLS_DESKTOP      17
#define ICLS_DIALOG       18
#define ICLS_MENU         19
#define ICLS_SWITCH       20
#define ICLS_ICONTITLE    21
#define ICLS_TOOLTIPS     22
#if (_WIN32_WINNT <= 0x0501)
#define ICLS_UNKNOWN      22
#define ICLS_NOTUSED      23
#else
#define ICLS_SYSSHADOW    23
#define ICLS_HWNDMESSAGE  24
#define ICLS_NOTUSED      25
#endif
#define ICLS_END          31

#define COLOR_LAST COLOR_MENUBAR
#define MAX_MB_STRINGS 11

#define SRVINFO_DBCSENABLED 0x0002
#define SRVINFO_IMM32       0x0004
#define SRVINFO_APIHOOK     0x0010
#define SRVINFO_CICERO_ENABLED 0x0020
#define SRVINFO_KBDPREF     0x0080

#define NUM_SYSCOLORS 31

typedef struct tagOEMBITMAPINFO
{
    INT x;
    INT y;
    INT cx;
    INT cy;
} OEMBITMAPINFO, *POEMBITMAPINFO;

typedef enum _OBI_TYPES
{
    OBI_CLOSE = 0,
    OBI_UPARROW = 46,
    OBI_UPARROWI = 49,
    OBI_DNARROW = 50,
    OBI_DNARROWI = 53,
    OBI_MNARROW = 62,
    OBI_CTYPES = 93
} OBI_TYPES;

typedef struct tagMBSTRING
{
    WCHAR szName[16];
    UINT uID;
    UINT uStr;
} MBSTRING, *PMBSTRING;

typedef struct tagDPISERVERINFO
{
    INT gclBorder;      /* 000 */
    HFONT hCaptionFont; /* 004 */
    HFONT hMsgFont;     /* 008 */
    INT cxMsgFontChar;  /* 00C */
    INT cyMsgFontChar;  /* 010 */
    UINT wMaxBtnSize;   /* 014 */
} DPISERVERINFO, *PDPISERVERINFO;

/* PUSIFlags: */
#define PUSIF_PALETTEDISPLAY         0x01
#define PUSIF_SNAPTO                 0x02
#define PUSIF_COMBOBOXANIMATION      0x04
#define PUSIF_LISTBOXSMOOTHSCROLLING 0x08
#define PUSIF_KEYBOARDCUES           0x20

typedef struct _PERUSERSERVERINFO
{
    INT aiSysMet[SM_CMETRICS];
    ULONG argbSystemUnmatched[NUM_SYSCOLORS];
    COLORREF argbSystem[NUM_SYSCOLORS];
    HBRUSH ahbrSystem[NUM_SYSCOLORS];
    HBRUSH hbrGray;
    POINT ptCursor;
    POINT ptCursorReal;
    DWORD dwLastRITEventTickCount;
    INT nEvents;
    UINT dtScroll;
    UINT dtLBSearch;
    UINT dtCaretBlink;
    UINT ucWheelScrollLines;
    UINT ucWheelScrollChars;
    INT wMaxLeftOverlapChars;
    INT wMaxRightOverlapChars;
    INT cxSysFontChar;
    INT cySysFontChar;
    TEXTMETRICW tmSysFont;
    DPISERVERINFO dpiSystem;
    HICON hIconSmWindows;
    HICON hIconWindows;
    DWORD dwKeyCache;
    DWORD dwAsyncKeyCache;
    ULONG cCaptures;
    OEMBITMAPINFO oembmi[OBI_CTYPES];
    RECT rcScreenReal;
    USHORT BitCount;
    USHORT dmLogPixels;
    BYTE Planes;
    BYTE BitsPixel;
    ULONG PUSIFlags;
    UINT uCaretWidth;
    USHORT UILangID;
    DWORD dwLastSystemRITEventTickCountUpdate;
    ULONG adwDBGTAGFlags[35];
    DWORD dwTagCount;
    DWORD dwRIPFlags;
} PERUSERSERVERINFO, *PPERUSERSERVERINFO;

typedef struct tagSERVERINFO
{
    DWORD dwSRVIFlags;
    ULONG_PTR cHandleEntries;
    PFN_FNID mpFnidPfn[FNID_NUM];
    WNDPROC aStoCidPfn[FNID_NUMSERVERPROC];
    USHORT mpFnid_serverCBWndProc[FNID_NUM];
    PFNCLIENT apfnClientA;
    PFNCLIENT apfnClientW;
    PFNCLIENTWORKER apfnClientWorker;
    ULONG cbHandleTable;
    ATOM atomSysClass[ICLS_NOTUSED+1];
    DWORD dwDefaultHeapBase;
    DWORD dwDefaultHeapSize;
    UINT uiShellMsg;
    MBSTRING MBStrings[MAX_MB_STRINGS];
    ATOM atomIconSmProp;
    ATOM atomIconProp;
    ATOM atomContextHelpIdProp;
    ATOM atomFrostedWindowProp;
    CHAR acOemToAnsi[256];
    CHAR acAnsiToOem[256];
    DWORD dwInstalledEventHooks;
    PERUSERSERVERINFO;
} SERVERINFO, *PSERVERINFO;

#ifdef _M_IX86
C_ASSERT(sizeof(SERVERINFO) <= PAGE_SIZE);
#endif


/* Server event activity bits. */
#define SRV_EVENT_MENU            0x0001
#define SRV_EVENT_END_APPLICATION 0x0002
#define SRV_EVENT_RUNNING         0x0004
#define SRV_EVENT_NAMECHANGE      0x0008
#define SRV_EVENT_VALUECHANGE     0x0010
#define SRV_EVENT_STATECHANGE     0x0020
#define SRV_EVENT_LOCATIONCHANGE  0x0040
#define SRV_EVENT_CREATE          0x8000

typedef struct _PROPLISTITEM
{
    ATOM Atom;
    HANDLE Data;
} PROPLISTITEM, *PPROPLISTITEM;

#define PROPERTY_FLAG_SYSTEM 1

typedef struct _PROPERTY
{
    LIST_ENTRY PropListEntry;
    HANDLE Data;
    ATOM Atom;
    WORD fs;
} PROPERTY, *PPROPERTY;

typedef struct _BROADCASTPARM
{
    DWORD flags;
    DWORD recipients;
    HDESK hDesk;
    HWND hWnd;
    LUID luid;
} BROADCASTPARM, *PBROADCASTPARM;

struct _THREADINFO *GetW32ThreadInfo(VOID);
struct _PROCESSINFO *GetW32ProcessInfo(VOID);

typedef struct _WNDMSG
{
    DWORD maxMsgs;
    PINT abMsgs;
} WNDMSG, *PWNDMSG;

typedef struct _SHAREDINFO
{
    PSERVERINFO psi;         /* Global Server Info */
    PVOID aheList;           /* Handle Entry List */
    PVOID pDispInfo;         /* Global PDISPLAYINFO pointer */
    ULONG_PTR ulSharedDelta; /* Shared USER mapped section delta */
    WNDMSG awmControl[FNID_NUM];
    WNDMSG DefWindowMsgs;
    WNDMSG DefWindowSpecMsgs;
} SHAREDINFO, *PSHAREDINFO;

/* See also the USERSRV_API_CONNECTINFO #define in include/reactos/subsys/win/winmsg.h */
typedef struct _USERCONNECT
{
    ULONG ulVersion;
    ULONG ulCurrentVersion;
    DWORD dwDispatchCount;
    SHAREDINFO siClient;
} USERCONNECT, *PUSERCONNECT;

/* WinNT 5.0 compatible user32 / win32k */
#define USER_VERSION MAKELONG(0x0000, 0x0005)

#if defined(_M_IX86)
C_ASSERT(sizeof(USERCONNECT) == 0x124);
#endif

typedef struct tagGETCLIPBDATA
{
    UINT uFmtRet;
    BOOL fGlobalHandle;
    union
    {
        HANDLE hLocale;
        HANDLE hPalette;
    };
} GETCLIPBDATA, *PGETCLIPBDATA;

typedef struct tagSETCLIPBDATA
{
    BOOL fGlobalHandle;
    BOOL fIncSerialNumber;
} SETCLIPBDATA, *PSETCLIPBDATA;

/* Used with NtUserSetCursorIconData, last parameter. */
typedef struct tagCURSORDATA
{
    LPWSTR lpName;
    LPWSTR lpModName;
    USHORT rt;
    USHORT dummy;
    ULONG CURSORF_flags;
    SHORT xHotspot;
    SHORT yHotspot;
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    HBITMAP hbmAlpha;
    RECT rcBounds;
    HBITMAP hbmUserAlpha; /* Could be in W7U, not in W2k */
    ULONG bpp;
    ULONG cx;
    ULONG cy;
    UINT cpcur;
    UINT cicur;
    struct tagCURSORDATA *aspcur;
    DWORD *aicur;
    INT *ajifRate;
    UINT iicur;
} CURSORDATA, *PCURSORDATA; /* !dso CURSORDATA */

/* CURSORF_flags: */
#define CURSORF_FROMRESOURCE 0x0001
#define CURSORF_GLOBAL       0x0002
#define CURSORF_LRSHARED     0x0004
#define CURSORF_ACON         0x0008
#define CURSORF_WOWCLEANUP   0x0010
#define CURSORF_ACONFRAME    0x0040
#define CURSORF_SECRET       0x0080
#define CURSORF_LINKED       0x0100
#define CURSORF_CURRENT      0x0200

typedef struct tagIMEINFOEX
{
    HKL hkl;
    IMEINFO ImeInfo;
    WCHAR wszUIClass[16];
    ULONG fdwInitConvMode;
    INT fInitOpen;
    INT fLoadFlag;
    DWORD dwProdVersion;
    DWORD dwImeWinVersion;
    WCHAR wszImeDescription[50];
    WCHAR wszImeFile[80];
    struct
    {
        INT fSysWow64Only:1;
        INT fCUASLayer:1;
    };
} IMEINFOEX, *PIMEINFOEX;

typedef enum IMEINFOEXCLASS /* unconfirmed: buggy */
{
    ImeInfoExKeyboardLayout,
    ImeInfoExImeWindow,
    ImeInfoExImeFileName
} IMEINFOEXCLASS;

#define IS_IME_HKL(hkl) ((((ULONG_PTR)(hkl)) & 0xF0000000) == 0xE0000000)

typedef struct tagIMEUI
{
    PWND spwnd;
    HIMC hIMC;
    HWND hwndIMC;
    HKL hKL;
    HWND hwndUI;
    INT nCntInIMEProc;
    struct {
        UINT fShowStatus:1;
        UINT fActivate:1;
        UINT fDestroy:1;
        UINT fDefault:1;
        UINT fChildThreadDef:1;
        UINT fCtrlShowStatus:1;
        UINT fFreeActiveEvent:1;
    };
} IMEUI, *PIMEUI;

/* Window Extra data container. */
typedef struct _IMEWND
{
    WND;
    PIMEUI pimeui;
} IMEWND, *PIMEWND;

typedef struct tagTRANSMSG
{
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *PTRANSMSG, *LPTRANSMSG;

typedef struct tagTRANSMSGLIST
{
    UINT     uMsgCount;
    TRANSMSG TransMsg[ANYSIZE_ARRAY];
} TRANSMSGLIST, *PTRANSMSGLIST, *LPTRANSMSGLIST;

#define DEFINE_IME_ENTRY(type, name, params, extended) typedef type (WINAPI *FN_##name) params;
#include "imetable.h"
#undef DEFINE_IME_ENTRY

typedef struct IMEDPI
{
    struct IMEDPI *pNext;
    HINSTANCE      hInst;
    HKL            hKL;
    IMEINFO        ImeInfo;
    UINT           uCodePage;
    WCHAR          szUIClass[16];
    DWORD          cLockObj;
    DWORD          dwFlags;
#define DEFINE_IME_ENTRY(type, name, params, extended) FN_##name name;
#include "imetable.h"
#undef DEFINE_IME_ENTRY
} IMEDPI, *PIMEDPI;

#ifndef _WIN64
C_ASSERT(offsetof(IMEDPI, pNext) == 0x0);
C_ASSERT(offsetof(IMEDPI, hInst) == 0x4);
C_ASSERT(offsetof(IMEDPI, hKL) == 0x8);
C_ASSERT(offsetof(IMEDPI, ImeInfo) == 0xc);
C_ASSERT(offsetof(IMEDPI, uCodePage) == 0x28);
C_ASSERT(offsetof(IMEDPI, szUIClass) == 0x2c);
C_ASSERT(offsetof(IMEDPI, cLockObj) == 0x4c);
C_ASSERT(offsetof(IMEDPI, dwFlags) == 0x50);
C_ASSERT(offsetof(IMEDPI, ImeInquire) == 0x54);
C_ASSERT(offsetof(IMEDPI, ImeConversionList) == 0x58);
C_ASSERT(offsetof(IMEDPI, ImeRegisterWord) == 0x5c);
C_ASSERT(offsetof(IMEDPI, ImeUnregisterWord) == 0x60);
C_ASSERT(offsetof(IMEDPI, ImeGetRegisterWordStyle) == 0x64);
C_ASSERT(offsetof(IMEDPI, ImeEnumRegisterWord) == 0x68);
C_ASSERT(offsetof(IMEDPI, ImeConfigure) == 0x6c);
C_ASSERT(offsetof(IMEDPI, ImeDestroy) == 0x70);
C_ASSERT(offsetof(IMEDPI, ImeEscape) == 0x74);
C_ASSERT(offsetof(IMEDPI, ImeProcessKey) == 0x78);
C_ASSERT(offsetof(IMEDPI, ImeSelect) == 0x7c);
C_ASSERT(offsetof(IMEDPI, ImeSetActiveContext) == 0x80);
C_ASSERT(offsetof(IMEDPI, ImeToAsciiEx) == 0x84);
C_ASSERT(offsetof(IMEDPI, NotifyIME) == 0x88);
C_ASSERT(offsetof(IMEDPI, ImeSetCompositionString) == 0x8c);
C_ASSERT(offsetof(IMEDPI, ImeGetImeMenuItems) == 0x90);
C_ASSERT(offsetof(IMEDPI, CtfImeInquireExW) == 0x94);
C_ASSERT(offsetof(IMEDPI, CtfImeSelectEx) == 0x98);
C_ASSERT(offsetof(IMEDPI, CtfImeEscapeEx) == 0x9c);
C_ASSERT(offsetof(IMEDPI, CtfImeGetGuidAtom) == 0xa0);
C_ASSERT(offsetof(IMEDPI, CtfImeIsGuidMapEnable) == 0xa4);
C_ASSERT(sizeof(IMEDPI) == 0xa8);
#endif

/* flags for IMEDPI.dwFlags */
#define IMEDPI_FLAG_UNKNOWN 0x1
#define IMEDPI_FLAG_LOCKED 0x2

/* unconfirmed */
typedef struct tagCLIENTIMC
{
    HANDLE hInputContext;   /* LocalAlloc'ed LHND */
    LONG cLockObj;
    DWORD dwFlags;
    DWORD unknown;
    RTL_CRITICAL_SECTION cs;
    UINT uCodePage;
    HKL hKL;
    BOOL bUnknown4;
} CLIENTIMC, *PCLIENTIMC;

#ifndef _WIN64
C_ASSERT(offsetof(CLIENTIMC, hInputContext) == 0x0);
C_ASSERT(offsetof(CLIENTIMC, cLockObj) == 0x4);
C_ASSERT(offsetof(CLIENTIMC, dwFlags) == 0x8);
C_ASSERT(offsetof(CLIENTIMC, cs) == 0x10);
C_ASSERT(offsetof(CLIENTIMC, uCodePage) == 0x28);
C_ASSERT(offsetof(CLIENTIMC, hKL) == 0x2c);
C_ASSERT(sizeof(CLIENTIMC) == 0x34);
#endif

/* flags for CLIENTIMC */
#define CLIENTIMC_WIDE 0x1
#define CLIENTIMC_UNKNOWN5 0x2
#define CLIENTIMC_UNKNOWN4 0x20
#define CLIENTIMC_UNKNOWN1 0x40
#define CLIENTIMC_UNKNOWN3 0x80
#define CLIENTIMC_UNKNOWN2 0x100

DWORD
NTAPI
NtUserAssociateInputContext(HWND hWnd, HIMC hIMC, DWORD dwFlags);

NTSTATUS
NTAPI
NtUserBuildHimcList(DWORD dwThreadId, DWORD dwCount, HIMC *phList, LPDWORD pdwCount);

DWORD
NTAPI
NtUserCalcMenuBar(
    HWND   hwnd,
    DWORD  x,
    DWORD  width,
    DWORD  y,
    LPRECT prc);

DWORD
NTAPI
NtUserCheckMenuItem(
    HMENU hmenu,
    UINT uIDCheckItem,
    UINT uCheck);

DWORD
NTAPI
NtUserCtxDisplayIOCtl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
APIENTRY
NtUserDbgWin32HeapFail(
    DWORD Unknown0,
    DWORD Unknown1);

DWORD
APIENTRY
NtUserDbgWin32HeapStat(
    DWORD Unknown0,
    DWORD Unknown1);

BOOL
NTAPI
NtUserDeleteMenu(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags);

BOOL
NTAPI
NtUserDestroyMenu(
    HMENU hMenu);

DWORD
NTAPI
NtUserDrawMenuBarTemp(
    HWND hWnd,
    HDC hDC,
    PRECT hRect,
    HMENU hMenu,
    HFONT hFont);

UINT
NTAPI
NtUserEnableMenuItem(
    HMENU hMenu,
    UINT uIDEnableItem,
    UINT uEnable);

BOOL
NTAPI
NtUserEndMenu(VOID);

BOOL
NTAPI
NtUserGetMenuBarInfo(
    HWND hwnd,
    LONG idObject,
    LONG idItem,
    PMENUBARINFO pmbi);

UINT
NTAPI
NtUserGetMenuIndex(
    HMENU hMenu,
    HMENU hSubMenu);

BOOL
NTAPI
NtUserGetMenuItemRect(
    HWND hWnd,
    HMENU hMenu,
    UINT uItem,
    LPRECT lprcItem);

HMENU
NTAPI
NtUserGetSystemMenu(
    HWND hWnd,
    BOOL bRevert);

BOOL
NTAPI
NtUserHiliteMenuItem(
    HWND hWnd,
    HMENU hMenu,
    UINT uItemHilite,
    UINT uHilite);

int
NTAPI
NtUserMenuItemFromPoint(
    HWND hWnd,
    HMENU hMenu,
    DWORD X,
    DWORD Y);

BOOL
NTAPI
NtUserRemoveMenu(
    HMENU hMenu,
    UINT uPosition,
    UINT uFlags);

BOOL
NTAPI
NtUserSetMenu(
    HWND hWnd,
    HMENU hMenu,
    BOOL bRepaint);

BOOL
NTAPI
NtUserSetMenuContextHelpId(
    HMENU hmenu,
    DWORD dwContextHelpId);

BOOL
NTAPI
NtUserSetMenuDefaultItem(
    HMENU hMenu,
    UINT uItem,
    UINT fByPos);

BOOL
NTAPI
NtUserSetMenuFlagRtoL(
    HMENU hMenu);

BOOL
NTAPI
NtUserSetSystemMenu(
    HWND hWnd,
    HMENU hMenu);

BOOL
NTAPI
NtUserThunkedMenuInfo(
    HMENU hMenu,
    LPCMENUINFO lpcmi);

BOOL
NTAPI
NtUserThunkedMenuItemInfo(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    BOOL bInsert,
    LPMENUITEMINFOW lpmii,
    PUNICODE_STRING lpszCaption);

BOOL
NTAPI
NtUserTrackPopupMenuEx(
    HMENU hmenu,
    UINT fuFlags,
    int x,
    int y,
    HWND hwnd,
    LPTPMPARAMS lptpm);

HKL
NTAPI
NtUserActivateKeyboardLayout(
    HKL hKl,
    ULONG Flags);

DWORD
NTAPI
NtUserAlterWindowStyle(
    HWND hWnd,
    DWORD Index,
    LONG NewValue);

BOOL
NTAPI
NtUserAttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach);

HDC NTAPI
NtUserBeginPaint(
    HWND hWnd,
    PAINTSTRUCT *lPs);

BOOL
NTAPI
NtUserBitBltSysBmp(
    HDC hdc,
    INT nXDest,
    INT nYDest,
    INT nWidth,
    INT nHeight,
    INT nXSrc,
    INT nYSrc,
    DWORD dwRop);

BOOL
NTAPI
NtUserBlockInput(
    BOOL BlockIt);

NTSTATUS
NTAPI
NtUserBuildHwndList(
    HDESK hDesktop,
    HWND hwndParent,
    BOOLEAN bChildren,
    ULONG dwThreadId,
    ULONG lParam,
    HWND *pWnd,
    ULONG *pBufSize);

NTSTATUS
NTAPI
NtUserBuildNameList(
    HWINSTA hWinSta,
    ULONG dwSize,
    PVOID lpBuffer,
    PULONG pRequiredSize);

NTSTATUS
NTAPI
NtUserBuildPropList(
    HWND hWnd,
    LPVOID Buffer,
    DWORD BufferSize,
    DWORD *Count);

/* apfnSimpleCall indices from Windows XP SP 2 */
/* TODO: Check for differences in Windows 2000, 2003 and 2008 */
#define WIN32K_VERSION NTDDI_WINXPSP2 /* FIXME: this should go somewhere else */

enum SimpleCallRoutines
{
    NOPARAM_ROUTINE_CREATEMENU,
    NOPARAM_ROUTINE_CREATEMENUPOPUP,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
    NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
    NOPARAM_ROUTINE_CREATESYSTEMTHREADS,
    NOPARAM_ROUTINE_DESTROY_CARET,
#endif
    NOPARAM_ROUTINE_ENABLEPROCWNDGHSTING,
#if (WIN32K_VERSION < NTDDI_VISTA)
    NOPARAM_ROUTINE_MSQCLEARWAKEMASK,
    NOPARAM_ROUTINE_ALLOWFOREGNDACTIVATION,
    NOPARAM_ROUTINE_DESTROY_CARET,
#endif
    NOPARAM_ROUTINE_GETDEVICECHANGEINFO,
    NOPARAM_ROUTINE_GETIMESHOWSTATUS,
    NOPARAM_ROUTINE_GETINPUTDESKTOP,
    NOPARAM_ROUTINE_GETMSESSAGEPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    NOPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#else
    NOPARAM_ROUTINE_GETREMOTEPROCESSID,
#endif
    NOPARAM_ROUTINE_HIDECURSORNOCAPTURE,
    NOPARAM_ROUTINE_LOADCURSANDICOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    NOPARAM_ROUTINE_LOADUSERAPIHOOK,
    NOPARAM_ROUTINE_PREPAREFORLOGOFF, /* 0x0f */
#endif
    NOPARAM_ROUTINE_RELEASECAPTURE,
    NOPARAM_ROUTINE_RESETDBLCLICK,
    NOPARAM_ROUTINE_ZAPACTIVEANDFOUS,
    NOPARAM_ROUTINE_REMOTECONSHDWSTOP,
    NOPARAM_ROUTINE_REMOTEDISCONNECT,
    NOPARAM_ROUTINE_REMOTELOGOFF,
    NOPARAM_ROUTINE_REMOTENTSECURITY,
    NOPARAM_ROUTINE_REMOTESHDWSETUP,
    NOPARAM_ROUTINE_REMOTESHDWSTOP,
    NOPARAM_ROUTINE_REMOTEPASSTHRUENABLE,
    NOPARAM_ROUTINE_REMOTEPASSTHRUDISABLE,
    NOPARAM_ROUTINE_REMOTECONNECTSTATE,
    NOPARAM_ROUTINE_UPDATEPERUSERIMMENABLING,
    NOPARAM_ROUTINE_USERPWRCALLOUTWORKER,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    NOPARAM_ROUTINE_WAKERITFORSHTDWN,
#endif
    NOPARAM_ROUTINE_INIT_MESSAGE_PUMP,
    NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP,
#if (WIN32K_VERSION < NTDDI_VISTA)
    NOPARAM_ROUTINE_LOADUSERAPIHOOK,
#endif
    ONEPARAM_ROUTINE_BEGINDEFERWNDPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    ONEPARAM_ROUTINE_GETSENDMSGRECVR,
#endif
    ONEPARAM_ROUTINE_WINDOWFROMDC,
    ONEPARAM_ROUTINE_ALLOWSETFOREGND,
    ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT,
#if (WIN32K_VERSION < NTDDI_VISTA)
    ONEPARAM_ROUTINE_CREATESYSTEMTHREADS,
#endif
    ONEPARAM_ROUTINE_CSDDEUNINITIALIZE,
    ONEPARAM_ROUTINE_DIRECTEDYIELD,
    ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS,
#if (WIN32K_VERSION < NTDDI_VISTA)
    ONEPARAM_ROUTINE_GETCURSORPOS,
#endif
    ONEPARAM_ROUTINE_GETINPUTEVENT,
    ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT,
    ONEPARAM_ROUTINE_GETKEYBOARDTYPE,
    ONEPARAM_ROUTINE_GETPROCDEFLAYOUT,
    ONEPARAM_ROUTINE_GETQUEUESTATUS,
    ONEPARAM_ROUTINE_GETWINSTAINFO,
#if (WIN32K_VERSION < NTDDI_VISTA)
    ONEPARAM_ROUTINE_HANDLESYSTHRDCREATFAIL,
#endif
    ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW,
    ONEPARAM_ROUTINE_LOADFONTS,
    ONEPARAM_ROUTINE_MAPDEKTOPOBJECT,
    ONEPARAM_ROUTINE_MESSAGEBEEP,
    ONEPARAM_ROUTINE_PLAYEVENTSOUND,
    ONEPARAM_ROUTINE_POSTQUITMESSAGE,
#if (WIN32K_VERSION < NTDDI_VISTA)
    ONEPARAM_ROUTINE_PREPAREFORLOGOFF,
#endif
    ONEPARAM_ROUTINE_REALIZEPALETTE,
    ONEPARAM_ROUTINE_REGISTERLPK,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    ONEPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
    ONEPARAM_ROUTINE_REMOTERECONNECT,
    ONEPARAM_ROUTINE_REMOTETHINWIRESTATUS,
    ONEPARAM_ROUTINE_RELEASEDC,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    ONEPARAM_ROUTINE_REMOTENOTIFY,
#endif
    ONEPARAM_ROUTINE_REPLYMESSAGE,
    ONEPARAM_ROUTINE_SETCARETBLINKTIME,
    ONEPARAM_ROUTINE_SETDBLCLICKTIME,
#if (WIN32K_VERSION < NTDDI_VISTA)
    ONEPARAM_ROUTINE_SETIMESHOWSTATUS,
#endif
    ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO,
    ONEPARAM_ROUTINE_SETPROCDEFLAYOUT,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    ONEPARAM_ROUTINE_SETWATERMARKSTRINGS,
#endif
    ONEPARAM_ROUTINE_SHOWCURSOR,
    ONEPARAM_ROUTINE_SHOWSTARTGLASS,
    ONEPARAM_ROUTINE_SWAPMOUSEBUTTON,
    X_ROUTINE_WOWMODULEUNLOAD,
#if (WIN32K_VERSION < NTDDI_VISTA)
    X_ROUTINE_REMOTENOTIFY,
#endif
    HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW,
    HWND_ROUTINE_DWP_GETENABLEDPOPUP,
    HWND_ROUTINE_GETWNDCONTEXTHLPID,
    HWND_ROUTINE_REGISTERSHELLHOOKWINDOW,
    HWND_ROUTINE_SETMSGBOX,
    HWNDOPT_ROUTINE_SETPROGMANWINDOW,
    HWNDOPT_ROUTINE_SETTASKMANWINDOW,
    HWNDPARAM_ROUTINE_GETCLASSICOCUR,
    HWNDPARAM_ROUTINE_CLEARWINDOWSTATE,
    HWNDPARAM_ROUTINE_KILLSYSTEMTIMER,
    HWNDPARAM_ROUTINE_SETDIALOGPOINTER,
    HWNDPARAM_ROUTINE_SETVISIBLE,
    HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID,
    HWNDPARAM_ROUTINE_SETWINDOWSTATE,
    HWNDLOCK_ROUTINE_WINDOWHASSHADOW, /* correct prefix ? */
    HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS,
    HWNDLOCK_ROUTINE_DRAWMENUBAR,
    HWNDLOCK_ROUTINE_CHECKIMESHOWSTATUSINTHRD,
    HWNDLOCK_ROUTINE_GETSYSMENUHANDLE,
    HWNDLOCK_ROUTINE_REDRAWFRAME,
    HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK,
    HWNDLOCK_ROUTINE_SETDLGSYSMENU,
    HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW,
    HWNDLOCK_ROUTINE_SETSYSMENU,
    HWNDLOCK_ROUTINE_UPDATECKIENTRECT,
    HWNDLOCK_ROUTINE_UPDATEWINDOW,
    X_ROUTINE_IMESHOWSTATUSCHANGE,
    TWOPARAM_ROUTINE_ENABLEWINDOW,
    TWOPARAM_ROUTINE_REDRAWTITLE,
    TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS,
    TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW,
    TWOPARAM_ROUTINE_UPDATEWINDOWS,
    TWOPARAM_ROUTINE_VALIDATERGN,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    TWOPARAM_ROUTINE_CHANGEWNDMSGFILTER,
    TWOPARAM_ROUTINE_GETCURSORPOS,
#endif
    TWOPARAM_ROUTINE_GETHDEVNAME,
    TWOPARAM_ROUTINE_INITANSIOEM,
    TWOPARAM_ROUTINE_NLSSENDIMENOTIFY,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    TWOPARAM_ROUTINE_REGISTERGHSTWND,
#endif
    TWOPARAM_ROUTINE_REGISTERLOGONPROCESS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    TWOPARAM_ROUTINE_REGISTERSBLFROSTWND,
#else
    TWOPARAM_ROUTINE_REGISTERSYSTEMTHREAD,
#endif
    TWOPARAM_ROUTINE_REGISTERUSERHUNGAPPHANDLERS,
    TWOPARAM_ROUTINE_SHADOWCLEANUP,
    TWOPARAM_ROUTINE_REMOTESHADOWSTART,
    TWOPARAM_ROUTINE_SETCARETPOS,
    TWOPARAM_ROUTINE_SETCURSORPOS,
#if (WIN32K_VERSION >= NTDDI_VISTA)
    TWOPARAM_ROUTINE_SETPHYSCURSORPOS,
#endif
    TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK,
    TWOPARAM_ROUTINE_WOWCLEANUP
};

DWORD
NTAPI
NtUserCallHwnd(
    HWND hWnd,
    DWORD Routine);

BOOL
NTAPI
NtUserCallHwndLock(
    HWND hWnd,
    DWORD Routine);

HWND
NTAPI
NtUserCallHwndOpt(
    HWND hWnd,
    DWORD Routine);

DWORD
NTAPI
NtUserCallHwndParam(
    HWND hWnd,
    DWORD_PTR Param,
    DWORD Routine);

DWORD
NTAPI
NtUserCallHwndParamLock(
    HWND hWnd,
    DWORD_PTR Param,
    DWORD Routine);

BOOL
NTAPI
NtUserCallMsgFilter(
    LPMSG msg,
    INT code);

LRESULT
NTAPI
NtUserCallNextHookEx(
    int Code,
    WPARAM wParam,
    LPARAM lParam,
    BOOL Ansi);

DWORD_PTR
NTAPI
NtUserCallNoParam(
    DWORD Routine);

DWORD_PTR
NTAPI
NtUserCallOneParam(
    DWORD_PTR Param,
    DWORD Routine);

DWORD_PTR
NTAPI
NtUserCallTwoParam(
    DWORD_PTR Param1,
    DWORD_PTR Param2,
    DWORD Routine);

BOOL
NTAPI
NtUserChangeClipboardChain(
    HWND hWndRemove,
    HWND hWndNewNext);

LONG
NTAPI
NtUserChangeDisplaySettings(
    PUNICODE_STRING lpszDeviceName,
    LPDEVMODEW lpDevMode,
    DWORD dwflags,
    LPVOID lParam);

BOOL
NTAPI
NtUserCheckDesktopByThreadId(
    DWORD dwThreadId);

BOOL
NTAPI
NtUserCheckWindowThreadDesktop(
    HWND hwnd,
    DWORD dwThreadId,
    ULONG ReturnValue);

DWORD
NTAPI
NtUserCheckImeHotKey(
    DWORD dwUnknown1,
    LPARAM dwUnknown2);

HWND NTAPI
NtUserChildWindowFromPointEx(
    HWND Parent,
    LONG x,
    LONG y,
    UINT Flags);

BOOL
NTAPI
NtUserClipCursor(
    RECT *lpRect);

BOOL
NTAPI
NtUserCloseClipboard(VOID);

BOOL
NTAPI
NtUserCloseDesktop(
    HDESK hDesktop);

BOOL
NTAPI
NtUserCloseWindowStation(
    HWINSTA hWinSta);

/* Console commands for NtUserConsoleControl */
typedef enum _CONSOLECONTROL
{
    ConsoleCtrlDesktopConsoleThread = 0,
    GuiConsoleWndClassAtom = 1,
    ConsoleMakePalettePublic = 5,
    ConsoleAcquireDisplayOwnership,
} CONSOLECONTROL, *PCONSOLECONTROL;

typedef struct _DESKTOP_CONSOLE_THREAD
{
    HDESK DesktopHandle;
    ULONG_PTR ThreadId;
} DESKTOP_CONSOLE_THREAD, *PDESKTOP_CONSOLE_THREAD;

NTSTATUS
APIENTRY
NtUserConsoleControl(
    IN CONSOLECONTROL ConsoleCtrl,
    IN PVOID ConsoleCtrlInfo,
    IN ULONG ConsoleCtrlInfoLength);

HANDLE
NTAPI
NtUserConvertMemHandle(
    PVOID pData,
    DWORD cbData);

ULONG
NTAPI
NtUserCopyAcceleratorTable(
    HACCEL Table,
    LPACCEL Entries,
    ULONG EntriesCount);

DWORD
NTAPI
NtUserCountClipboardFormats(VOID);

HACCEL
NTAPI
NtUserCreateAcceleratorTable(
    LPACCEL Entries,
    ULONG EntriesCount);

BOOL
NTAPI
NtUserCreateCaret(
    HWND hWnd,
    HBITMAP hBitmap,
    int nWidth,
    int nHeight);

HDESK
NTAPI
NtUserCreateDesktop(
    POBJECT_ATTRIBUTES poa,
    PUNICODE_STRING lpszDesktopDevice,
    LPDEVMODEW lpdmw,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess);

HIMC
NTAPI
NtUserCreateInputContext(ULONG_PTR dwClientImcData);

NTSTATUS
NTAPI
NtUserCreateLocalMemHandle(
    HANDLE hMem,
    PVOID pData,
    DWORD cbData,
    DWORD *pcbData);

HWND
NTAPI
NtUserCreateWindowEx(
    DWORD dwExStyle,
    PLARGE_STRING plstrClassName,
    PLARGE_STRING plstrClsVersion,
    PLARGE_STRING plstrWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam,
    DWORD dwFlags,
    PVOID acbiBuffer);

HWINSTA
NTAPI
NtUserCreateWindowStation(
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK dwDesiredAccess,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6);

BOOL
NTAPI
NtUserDdeGetQualityOfService(
    IN HWND hwndClient,
    IN HWND hWndServer,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev);

DWORD
NTAPI
NtUserDdeInitialize(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4);

BOOL
NTAPI
NtUserDdeSetQualityOfService(
    IN HWND hwndClient,
    IN PSECURITY_QUALITY_OF_SERVICE pqosNew,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev);

HDWP
NTAPI
NtUserDeferWindowPos(
    HDWP WinPosInfo,
    HWND Wnd,
    HWND WndInsertAfter,
    int x,
    int y,
    int cx,
    int cy,
    UINT Flags);

BOOL
NTAPI
NtUserDefSetText(
    HWND WindowHandle,
    PLARGE_STRING WindowText);

BOOLEAN
NTAPI
NtUserDestroyAcceleratorTable(
    HACCEL Table);

BOOL
NTAPI
NtUserDestroyCursor(
  _In_ HANDLE Handle,
  _In_ BOOL bForce);

BOOL
NTAPI
NtUserDestroyInputContext(HIMC hIMC);

BOOLEAN
NTAPI
NtUserDestroyWindow(
    HWND Wnd);

BOOL
NTAPI
NtUserDisableThreadIme(
    DWORD dwThreadID);

LRESULT
NTAPI
NtUserDispatchMessage(
    PMSG pMsg);

BOOL
NTAPI
NtUserDragDetect(
    HWND hWnd,
    POINT pt);

DWORD
NTAPI
NtUserDragObject(
    HWND hwnd1,
    HWND hwnd2,
    UINT u1,
    DWORD dw1,
    HCURSOR hc1);

BOOL
NTAPI
NtUserDrawAnimatedRects(
    HWND hwnd,
    INT idAni,
    RECT *lprcFrom,
    RECT *lprcTo);

BOOL
NTAPI
NtUserDrawCaption(
    HWND hWnd,
    HDC hDc,
    LPCRECT lpRc,
    UINT uFlags);

BOOL
NTAPI
NtUserDrawCaptionTemp(
    HWND hWnd,
    HDC hDC,
    LPCRECT lpRc,
    HFONT hFont,
    HICON hIcon,
    const PUNICODE_STRING str,
    UINT uFlags);

/* Used with NtUserDrawIconEx, last parameter. */
typedef struct _DRAWICONEXDATA
{
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    int cx;
    int cy;
} DRAWICONEXDATA, *PDRAWICONEXDATA;

BOOL
NTAPI
NtUserDrawIconEx(
    HDC hdc,
    int xLeft,
    int yTop,
    HICON hIcon,
    int cxWidth,
    int cyWidth,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags,
    BOOL bMetaHDC,
    PVOID pDIXData);

BOOL
NTAPI
NtUserEmptyClipboard(VOID);

BOOL
NTAPI
NtUserEnableScrollBar(
    HWND hWnd,
    UINT wSBflags,
    UINT wArrows);

BOOL
NTAPI
NtUserEndDeferWindowPosEx(
    HDWP WinPosInfo,
    BOOL bAsync);

BOOL
NTAPI
NtUserEndPaint(
    HWND hWnd,
    CONST PAINTSTRUCT *lPs);

BOOL
NTAPI
NtUserEnumDisplayDevices(
    PUNICODE_STRING lpDevice, /* device name */
    DWORD iDevNum, /* display device */
    PDISPLAY_DEVICEW lpDisplayDevice, /* device information */
    DWORD dwFlags); /* reserved */

/*
BOOL
NTAPI
NtUserEnumDisplayMonitors(
    HDC hdc,
    LPCRECT lprcClip,
    MONITORENUMPROC lpfnEnum,
    LPARAM dwData);
*/
/* FIXME:  The call below is ros-specific and should be rewritten to use the same params as the correct call above. */
INT
NTAPI
NtUserEnumDisplayMonitors(
    OPTIONAL IN HDC hDC,
    OPTIONAL IN LPCRECT pRect,
    OPTIONAL OUT HMONITOR *hMonitorList,
    OPTIONAL OUT LPRECT monitorRectList,
    OPTIONAL IN DWORD listSize);


NTSTATUS
NTAPI
NtUserEnumDisplaySettings(
    PUNICODE_STRING lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODEW lpDevMode, /* FIXME is this correct? */
    DWORD dwFlags);

DWORD
NTAPI
NtUserEvent(
    DWORD Unknown0);

INT
NTAPI
NtUserExcludeUpdateRgn(
    HDC hDC,
    HWND hWnd);

BOOL
NTAPI
NtUserFillWindow(
    HWND hWndPaint,
    HWND hWndPaint1,
    HDC hDC,
    HBRUSH hBrush);

HWND
NTAPI
NtUserFindWindowEx(
    HWND hwndParent,
    HWND hwndChildAfter,
    PUNICODE_STRING ucClassName,
    PUNICODE_STRING ucWindowName,
    DWORD dwUnknown);

BOOL
NTAPI
NtUserFlashWindowEx(
    IN PFLASHWINFO pfwi);

BOOL
NTAPI
NtUserGetAltTabInfo(
    HWND hwnd,
    INT iItem,
    PALTTABINFO pati,
    LPWSTR pszItemText,
    UINT cchItemText,
    BOOL Ansi);

HWND
NTAPI
NtUserGetAncestor(
    HWND hWnd,
    UINT Flags);

DWORD
NTAPI
NtUserGetAppImeLevel(HWND hWnd);

SHORT
NTAPI
NtUserGetAsyncKeyState(
    INT Key);

_Success_(return != 0)
_At_(pustrName->Buffer, _Out_z_bytecap_post_bytecount_(pustrName->MaximumLength, return * 2 + 2))
ULONG
APIENTRY
NtUserGetAtomName(
    _In_ ATOM atom,
    _Inout_ PUNICODE_STRING pustrName);

UINT
NTAPI
NtUserGetCaretBlinkTime(VOID);

BOOL
NTAPI
NtUserGetCaretPos(
    LPPOINT lpPoint);

BOOL
NTAPI
NtUserGetClassInfo(
    HINSTANCE hInstance,
    PUNICODE_STRING ClassName,
    LPWNDCLASSEXW wcex,
    LPWSTR *ppszMenuName,
    BOOL Ansi);

INT
NTAPI
NtUserGetClassName(
    HWND hWnd,
    BOOL Real, /* 0 GetClassNameW, 1 RealGetWindowClassA/W */
    PUNICODE_STRING ClassName);

HANDLE
NTAPI
NtUserGetClipboardData(
    UINT fmt,
    PGETCLIPBDATA pgcd);

INT
NTAPI
NtUserGetClipboardFormatName(
    UINT uFormat,
    LPWSTR lpszFormatName,
    INT cchMaxCount);

HWND
NTAPI
NtUserGetClipboardOwner(VOID);

DWORD
NTAPI
NtUserGetClipboardSequenceNumber(VOID);

HWND
NTAPI
NtUserGetClipboardViewer(VOID);

BOOL
NTAPI
NtUserGetClipCursor(
    RECT *lpRect);

BOOL
NTAPI
NtUserGetComboBoxInfo(
    HWND hWnd,
    PCOMBOBOXINFO pcbi);

HBRUSH
NTAPI
NtUserGetControlBrush(
    HWND hwnd,
    HDC  hdc,
    UINT ctlType);

HBRUSH
NTAPI
NtUserGetControlColor(
    HWND hwndParent,
    HWND hwnd,
    HDC hdc,
    UINT CtlMsg);

ULONG_PTR
NTAPI
NtUserGetCPD(
    HWND hWnd,
    GETCPD Flags,
    ULONG_PTR Proc);

HCURSOR
NTAPI
NtUserGetCursorFrameInfo(
    HCURSOR hCursor,
    DWORD istep,
    INT *rate_jiffies,
    DWORD *num_steps);

BOOL
NTAPI
NtUserGetCursorInfo(
    PCURSORINFO pci);

HDC
NTAPI
NtUserGetDC(
    HWND hWnd);

HDC
NTAPI
NtUserGetDCEx(
    HWND hWnd,
    HANDLE hRegion,
    ULONG Flags);

UINT
NTAPI
NtUserGetDoubleClickTime(VOID);

HWND
NTAPI
NtUserGetForegroundWindow(VOID);

DWORD
NTAPI
NtUserGetGuiResources(
    HANDLE hProcess,
    DWORD uiFlags);

BOOL
NTAPI
NtUserGetGUIThreadInfo(
    DWORD idThread,
    LPGUITHREADINFO lpgui);

_Success_(return != FALSE)
BOOL
NTAPI
NtUserGetIconInfo(
    _In_ HANDLE hCurIcon,
    _Out_opt_ PICONINFO IconInfo,
    _Inout_opt_ PUNICODE_STRING lpInstName,
    _Inout_opt_ PUNICODE_STRING lpResName,
    _Out_opt_ LPDWORD pbpp,
    _In_ BOOL bInternal);

BOOL
NTAPI
NtUserGetIconSize(
    HANDLE Handle,
    UINT istepIfAniCur,
    LONG *plcx,
    LONG *plcy);

BOOL NTAPI
NtUserGetImeHotKey(IN DWORD dwHotKey,
                   OUT LPUINT lpuModifiers,
                   OUT LPUINT lpuVKey,
                   OUT LPHKL lphKL);

BOOL
NTAPI
NtUserGetImeInfoEx(
    PIMEINFOEX pImeInfoEx,
    IMEINFOEXCLASS SearchType);

DWORD
NTAPI
NtUserGetInternalWindowPos(
    HWND hwnd,
    LPRECT rectWnd,
    LPPOINT ptIcon);

HKL
NTAPI
NtUserGetKeyboardLayout(
    DWORD dwThreadid);

UINT
NTAPI
NtUserGetKeyboardLayoutList(
    ULONG nItems,
    HKL *pHklBuff);

BOOL
NTAPI
NtUserGetKeyboardLayoutName(
    LPWSTR lpszName);

DWORD
NTAPI
NtUserGetKeyboardState(
    LPBYTE Unknown0);

DWORD
NTAPI
NtUserGetKeyboardType(
    DWORD TypeFlag);

DWORD
NTAPI
NtUserGetKeyNameText(
    LONG lParam,
    LPWSTR lpString,
    int nSize);

SHORT
NTAPI
NtUserGetKeyState(
    INT VirtKey);

BOOL
NTAPI
NtUserGetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags);

DWORD
NTAPI
NtUserGetListBoxInfo(
    HWND hWnd);

BOOL
APIENTRY
NtUserGetMessage(
    PMSG pMsg,
    HWND hWnd,
    UINT MsgFilterMin,
    UINT MsgFilterMax);

DWORD
NTAPI
NtUserGetMouseMovePointsEx(
    UINT cbSize,
    LPMOUSEMOVEPOINT lppt,
    LPMOUSEMOVEPOINT lpptBuf,
    int nBufPoints,
    DWORD resolution);

BOOL
NTAPI
NtUserGetObjectInformation(
    HANDLE hObject,
    DWORD nIndex,
    PVOID pvInformation,
    DWORD nLength,
    PDWORD nLengthNeeded);

HWND
NTAPI
NtUserGetOpenClipboardWindow(VOID);

INT
NTAPI
NtUserGetPriorityClipboardFormat(
    UINT *paFormatPriorityList,
    INT cFormats);

HWINSTA
NTAPI
NtUserGetProcessWindowStation(VOID);

DWORD
NTAPI
NtUserGetRawInputBuffer(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader);

DWORD
NTAPI
NtUserGetRawInputData(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader);

DWORD
NTAPI
NtUserGetRawInputDeviceInfo(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize);

DWORD
NTAPI
NtUserGetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize);

DWORD
NTAPI
NtUserGetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize);

BOOL
NTAPI
NtUserGetScrollBarInfo(
    HWND hWnd,
    LONG idObject,
    PSCROLLBARINFO psbi);

HDESK
NTAPI
NtUserGetThreadDesktop(
    DWORD dwThreadId,
    HDESK hConsoleDesktop);

enum ThreadStateRoutines
{
    THREADSTATE_GETTHREADINFO,
    THREADSTATE_INSENDMESSAGE,
    THREADSTATE_FOCUSWINDOW,
    THREADSTATE_ACTIVEWINDOW,
    THREADSTATE_CAPTUREWINDOW,
    THREADSTATE_PROGMANWINDOW,
    THREADSTATE_TASKMANWINDOW,
    THREADSTATE_GETMESSAGETIME,
    THREADSTATE_GETINPUTSTATE,
    THREADSTATE_UPTIMELASTREAD,
    THREADSTATE_FOREGROUNDTHREAD,
    THREADSTATE_GETCURSOR,
    THREADSTATE_GETMESSAGEEXTRAINFO,
    THREADSTATE_UNKNOWN13
};

DWORD_PTR
NTAPI
NtUserGetThreadState(
    DWORD Routine);

BOOLEAN
NTAPI
NtUserGetTitleBarInfo(
    HWND hwnd,
    PTITLEBARINFO pti);

BOOL
NTAPI
NtUserGetUpdateRect(
    HWND hWnd,
    LPRECT lpRect,
    BOOL fErase);

INT
NTAPI
NtUserGetUpdateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase);

HDC
NTAPI
NtUserGetWindowDC(
    HWND hWnd);

BOOL
NTAPI
NtUserGetWindowPlacement(
    HWND hWnd,
    WINDOWPLACEMENT *lpwndpl);

PCLS
NTAPI
NtUserGetWOWClass(
    HINSTANCE hInstance,
    PUNICODE_STRING ClassName);

DWORD
NTAPI
NtUserHardErrorControl(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

BOOL
NTAPI
NtUserImpersonateDdeClientWindow(
    HWND hWndClient,
    HWND hWndServer);

NTSTATUS
NTAPI
NtUserInitialize(
    DWORD dwWinVersion,
    HANDLE hPowerRequestEvent,
    HANDLE hMediaRequestEvent);

NTSTATUS
NTAPI
NtUserInitializeClientPfnArrays(
    PPFNCLIENT pfnClientA,
    PPFNCLIENT pfnClientW,
    PPFNCLIENTWORKER pfnClientWorker,
    HINSTANCE hmodUser);

DWORD
NTAPI
NtUserInitTask(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5,
    DWORD Unknown6,
    DWORD Unknown7,
    DWORD Unknown8,
    DWORD Unknown9,
    DWORD Unknown10,
    DWORD Unknown11);

INT
NTAPI
NtUserInternalGetWindowText(
    HWND hWnd,
    LPWSTR lpString,
    INT nMaxCount);

BOOL
NTAPI
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpRect,
    BOOL bErase);

BOOL
NTAPI
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase);

BOOL
NTAPI
NtUserIsClipboardFormatAvailable(
    UINT format);

BOOL
NTAPI
NtUserKillTimer(
    HWND hWnd,
    UINT_PTR uIDEvent);

HKL
NTAPI
NtUserLoadKeyboardLayoutEx(
    IN HANDLE Handle,
    IN DWORD offTable,
    IN PUNICODE_STRING puszKeyboardName,
    IN HKL hKL,
    IN PUNICODE_STRING puszKLID,
    IN DWORD dwKLID,
    IN UINT Flags);

BOOL
NTAPI
NtUserLockWindowStation(
    HWINSTA hWindowStation);

BOOL
NTAPI
NtUserLockWindowUpdate(
    HWND hWnd);

BOOL
NTAPI
NtUserLockWorkStation(VOID);

UINT
NTAPI
NtUserMapVirtualKeyEx(
    UINT keyCode,
    UINT transType,
    DWORD keyboardId,
    HKL dwhkl);

typedef struct tagDOSENDMESSAGE
{
    UINT uFlags;
    UINT uTimeout;
    ULONG_PTR Result;
}
DOSENDMESSAGE, *PDOSENDMESSAGE;

BOOL
NTAPI
NtUserMessageCall(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR ResultInfo,
    DWORD dwType, /* FNID_XX types */
    BOOL Ansi);

DWORD
NTAPI
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, /* Wine SW_ commands */
    BOOL Hide);

DWORD
NTAPI
NtUserMNDragLeave(VOID);

DWORD
NTAPI
NtUserMNDragOver(
    DWORD Unknown0,
    DWORD Unknown1);

DWORD
NTAPI
NtUserModifyUserStartupInfoFlags(
    DWORD Unknown0,
    DWORD Unknown1);

BOOL
NTAPI
NtUserMoveWindow(
    HWND hWnd,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    BOOL bRepaint
);

DWORD
NTAPI
NtUserNotifyIMEStatus(HWND hwnd, BOOL fOpen, DWORD dwConversion);

BOOL
NTAPI
NtUserNotifyProcessCreate(
    HANDLE NewProcessId,
    HANDLE ParentThreadId,
    ULONG dwUnknown,
    ULONG CreateFlags);

VOID
NTAPI
NtUserNotifyWinEvent(
    DWORD Event,
    HWND hWnd,
    LONG idObject,
    LONG idChild);

BOOL
NTAPI
NtUserOpenClipboard(
    HWND hWnd,
    DWORD Unknown1);

HDESK
NTAPI
NtUserOpenDesktop(
    POBJECT_ATTRIBUTES ObjectAttributes,
    DWORD dwFlags,
    ACCESS_MASK dwDesiredAccess);

HDESK
NTAPI
NtUserOpenInputDesktop(
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess);

HWINSTA
NTAPI
NtUserOpenWindowStation(
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK dwDesiredAccess);

BOOL
NTAPI
NtUserPaintDesktop(
    HDC hDC);

DWORD
NTAPI
NtUserPaintMenuBar(
    HWND hWnd,
    HDC hDC,
    ULONG left,    // x,
    ULONG right,   // width, // Scale the edge thickness, offset?
    ULONG top,     // y,
    BOOL bActive); // DWORD Flags); DC_ACTIVE or WS_ACTIVECAPTION, by checking WNDS_ACTIVEFRAME and foreground.

BOOL
APIENTRY
NtUserPeekMessage(
    PMSG pMsg,
    HWND hWnd,
    UINT MsgFilterMin,
    UINT MsgFilterMax,
    UINT RemoveMsg);

BOOL
NTAPI
NtUserPostMessage(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
NTAPI
NtUserPostThreadMessage(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

BOOL
NTAPI
NtUserPrintWindow(
    HWND hwnd,
    HDC hdcBlt,
    UINT nFlags);

NTSTATUS
NTAPI
NtUserProcessConnect(
    IN HANDLE ProcessHandle,
    OUT PUSERCONNECT pUserConnect,
    IN ULONG Size); /* sizeof(USERCONNECT) */

NTSTATUS
NTAPI
NtUserQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN USERTHREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength);

DWORD_PTR
NTAPI
NtUserQueryInputContext(
    HIMC hIMC,
    DWORD dwType);

DWORD
NTAPI
NtUserQuerySendMessage(
    DWORD Unknown0);

DWORD
NTAPI
NtUserQueryUserCounters(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4);

#define QUERY_WINDOW_UNIQUE_PROCESS_ID 0x00
#define QUERY_WINDOW_UNIQUE_THREAD_ID  0x01
#define QUERY_WINDOW_ACTIVE            0x02
#define QUERY_WINDOW_FOCUS             0x03
#define QUERY_WINDOW_ISHUNG            0x04
#define QUERY_WINDOW_REAL_ID           0x05
#define QUERY_WINDOW_FOREGROUND        0x06
#define QUERY_WINDOW_DEFAULT_IME       0x07
#define QUERY_WINDOW_DEFAULT_ICONTEXT  0x08
#define QUERY_WINDOW_ACTIVE_IME        0x09

DWORD_PTR
NTAPI
NtUserQueryWindow(
    HWND hWnd,
    DWORD Index);

BOOL
NTAPI
NtUserRealInternalGetMessage(
    LPMSG lpMsg,
    HWND hWnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bGMSG);

HWND
NTAPI
NtUserRealChildWindowFromPoint(
    HWND Parent,
    LONG x,
    LONG y);

BOOL
NTAPI
NtUserRealWaitMessageEx(
    DWORD dwWakeMask,
    UINT uTimeout);

BOOL
NTAPI
NtUserRedrawWindow(
    HWND hWnd,
    CONST RECT *lprcUpdate,
    HRGN hrgnUpdate,
    UINT flags);

RTL_ATOM
NTAPI
NtUserRegisterClassExWOW(
    WNDCLASSEXW* lpwcx,
    PUNICODE_STRING pustrClassName,
    PUNICODE_STRING pustrCVersion,
    PCLSMENUNAME pClassMenuName,
    DWORD fnID,
    DWORD Flags,
    LPDWORD pWow);

BOOL
NTAPI
NtUserRegisterRawInputDevices(
    IN PCRAWINPUTDEVICE pRawInputDevices,
    IN UINT uiNumDevices,
    IN UINT cbSize);

BOOL
NTAPI
NtUserRegisterUserApiHook(
    PUNICODE_STRING m_dllname1,
    PUNICODE_STRING m_funname1,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

BOOL
NTAPI
NtUserRegisterHotKey(
    HWND hWnd,
    int id,
    UINT fsModifiers,
    UINT vk);

DWORD
NTAPI
NtUserRegisterTasklist(
    DWORD Unknown0);

UINT
NTAPI
NtUserRegisterWindowMessage(
    PUNICODE_STRING MessageName);

DWORD
NTAPI
NtUserRemoteConnect(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3);

DWORD
NTAPI
NtUserRemoteRedrawRectangle(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

DWORD
NTAPI
NtUserRemoteRedrawScreen(VOID);

DWORD
NTAPI
NtUserRemoteStopScreenUpdates(VOID);

HANDLE
NTAPI
NtUserRemoveProp(
    HWND hWnd,
    ATOM Atom);

HDESK
NTAPI
NtUserResolveDesktop(
    IN HANDLE ProcessHandle,
    IN PUNICODE_STRING DesktopPath,
    IN BOOL bInherit,
    OUT HWINSTA* phWinSta);

DWORD
NTAPI
NtUserResolveDesktopForWOW(
    DWORD Unknown0);

BOOL
NTAPI
NtUserSBGetParms(
    HWND hwnd,
    int fnBar,
    PSBDATA pSBData,
    LPSCROLLINFO lpsi);

BOOL
NTAPI
NtUserScrollDC(
    HDC hDC,
    int dx,
    int dy,
    CONST RECT *lprcScroll,
    CONST RECT *lprcClip ,
    HRGN hrgnUpdate,
    LPRECT lprcUpdate);

DWORD
NTAPI
NtUserScrollWindowEx(
    HWND hWnd,
    INT dx,
    INT dy,
    const RECT *rect,
    const RECT *clipRect,
    HRGN hrgnUpdate,
    LPRECT rcUpdate,
    UINT flags);

UINT
NTAPI
NtUserSendInput(
    UINT nInputs,
    LPINPUT pInput,
    INT cbSize);

HWND
NTAPI
NtUserSetActiveWindow(
    HWND Wnd);

DWORD
NTAPI
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2);

HWND
NTAPI
NtUserSetCapture(
    HWND Wnd);

ULONG_PTR
NTAPI
NtUserSetClassLong(
    HWND hWnd,
    INT Offset,
    ULONG_PTR dwNewLong,
    BOOL Ansi);

WORD
NTAPI
NtUserSetClassWord(
    HWND hWnd,
    INT nIndex,
    WORD wNewWord);

HANDLE
NTAPI
NtUserSetClipboardData(
    UINT fmt,
    HANDLE hMem,
    PSETCLIPBDATA scd);

HWND
NTAPI
NtUserSetClipboardViewer(
    HWND hWndNewViewer);

HPALETTE
NTAPI
NtUserSelectPalette(
    HDC hDC,
    HPALETTE hpal,
    BOOL ForceBackground);

DWORD
NTAPI
NtUserSetConsoleReserveKeys(
    DWORD Unknown0,
    DWORD Unknown1);

HCURSOR
NTAPI
NtUserSetCursor(
    HCURSOR hCursor);

BOOL
NTAPI
NtUserSetCursorContents(
    HANDLE Handle,
    PICONINFO IconInfo);

BOOL
NTAPI
NtUserSetCursorIconData(
    _In_ HCURSOR hCursor,
    _In_opt_ PUNICODE_STRING pustrModule,
    _In_opt_ PUNICODE_STRING puSrcName,
    _In_ const CURSORDATA *pCursorData);

typedef struct _tagFINDEXISTINGCURICONPARAM
{
    BOOL bIcon;
    LONG cx;
    LONG cy;
} FINDEXISTINGCURICONPARAM;

HICON
NTAPI
NtUserFindExistingCursorIcon(
    _In_ PUNICODE_STRING pustrModule,
    _In_ PUNICODE_STRING pustrRsrc,
    _In_ FINDEXISTINGCURICONPARAM *param);

DWORD
NTAPI
NtUserSetDbgTag(
    DWORD Unknown0,
    DWORD Unknown1);

DWORD
APIENTRY
NtUserSetDbgTagCount(
    DWORD Unknown0);

HWND
NTAPI
NtUserSetFocus(
    HWND hWnd);

DWORD
NTAPI
NtUserSetImeHotKey(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4);

DWORD
NTAPI
NtUserSetImeInfoEx(
    PIMEINFOEX pImeInfoEx);

DWORD
NTAPI
NtUserSetImeOwnerWindow(PIMEINFOEX pImeInfoEx, BOOL fFlag);

DWORD
NTAPI
NtUserSetInformationProcess(
    DWORD dwUnknown1,
    DWORD dwUnknown2,
    DWORD dwUnknown3,
    DWORD dwUnknown4);

NTSTATUS
NTAPI
NtUserSetInformationThread(
    IN HANDLE ThreadHandle,
    IN USERTHREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength);

DWORD
NTAPI
NtUserSetInternalWindowPos(
    HWND hwnd,
    UINT showCmd,
    LPRECT rect,
    LPPOINT pt);

BOOL
NTAPI
NtUserSetKeyboardState(
    LPBYTE lpKeyState);

BOOL
NTAPI
NtUserSetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF crKey,
    BYTE bAlpha,
    DWORD dwFlags);

BOOL
NTAPI
NtUserSetLogonNotifyWindow(
    HWND hWnd);

BOOL
NTAPI
NtUserSetObjectInformation(
    HANDLE hObject,
    DWORD nIndex,
    PVOID pvInformation,
    DWORD nLength);

HWND
NTAPI
NtUserSetParent(
    HWND hWndChild,
    HWND hWndNewParent);

BOOL
NTAPI
NtUserSetProcessWindowStation(
    HWINSTA hWindowStation);

BOOL
NTAPI
NtUserSetProp(
    HWND hWnd,
    ATOM Atom,
    HANDLE Data);

DWORD
NTAPI
NtUserSetRipFlags(
    DWORD Unknown0);

DWORD
NTAPI
NtUserSetScrollInfo(
    HWND hwnd,
    int fnBar,
    LPCSCROLLINFO lpsi,
    BOOL bRedraw);

BOOL
NTAPI
NtUserSetShellWindowEx(
    HWND hwndShell,
    HWND hwndShellListView);

BOOL
NTAPI
NtUserSetSysColors(
    int cElements,
    IN CONST INT *lpaElements,
    IN CONST COLORREF *lpaRgbValues,
    FLONG Flags);

BOOL
NTAPI
NtUserSetSystemCursor(
    HCURSOR hcur,
    DWORD id);

BOOL
NTAPI
NtUserSetThreadDesktop(
    HDESK hDesktop);

DWORD
NTAPI
NtUserSetThreadState(
    DWORD Unknown0,
    DWORD Unknown1);

UINT_PTR
NTAPI
NtUserSetSystemTimer(
    HWND hWnd,
    UINT_PTR nIDEvent,
    UINT uElapse,
    TIMERPROC lpTimerFunc);

DWORD
NTAPI
NtUserSetThreadLayoutHandles(HKL hNewKL, HKL hOldKL);

UINT_PTR
NTAPI
NtUserSetTimer(
    HWND hWnd,
    UINT_PTR nIDEvent,
    UINT uElapse,
    TIMERPROC lpTimerFunc);

BOOL
NTAPI
NtUserSetWindowFNID(
    HWND hWnd,
    WORD fnID);

LONG
NTAPI
NtUserSetWindowLong(
    HWND hWnd,
    DWORD Index,
    LONG NewValue,
    BOOL Ansi);

#ifdef _WIN64
LONG_PTR
NTAPI
NtUserSetWindowLongPtr(
    HWND hWnd,
    DWORD Index,
    LONG_PTR NewValue,
    BOOL Ansi);
#endif // _WIN64

BOOL
NTAPI
NtUserSetWindowPlacement(
    HWND hWnd,
    WINDOWPLACEMENT *lpwndpl);

BOOL
NTAPI
NtUserSetWindowPos(
    HWND hWnd,
    HWND hWndInsertAfter,
    int X,
    int Y,
    int cx,
    int cy,
    UINT uFlags);

INT
NTAPI
NtUserSetWindowRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bRedraw);

HHOOK
NTAPI
NtUserSetWindowsHookAW(
    int idHook,
    HOOKPROC lpfn,
    BOOL Ansi);

HHOOK
NTAPI
NtUserSetWindowsHookEx(
    HINSTANCE Mod,
    PUNICODE_STRING ModuleName,
    DWORD ThreadId,
    int HookId,
    HOOKPROC HookProc,
    BOOL Ansi);

BOOL
NTAPI
NtUserSetWindowStationUser(
    IN HWINSTA hWindowStation,
    IN PLUID pluid,
    IN PSID psid OPTIONAL,
    IN DWORD size);

WORD
NTAPI
NtUserSetWindowWord(
    HWND hWnd,
    INT Index,
    WORD NewVal);

HWINEVENTHOOK
NTAPI
NtUserSetWinEventHook(
    UINT eventMin,
    UINT eventMax,
    HMODULE hmodWinEventProc,
    PUNICODE_STRING puString,
    WINEVENTPROC lpfnWinEventProc,
    DWORD idProcess,
    DWORD idThread,
    UINT dwflags);

BOOL
NTAPI
NtUserShowCaret(
    HWND hWnd);

BOOL
NTAPI
NtUserHideCaret(
    HWND hWnd);

DWORD
NTAPI
NtUserShowScrollBar(
    HWND hWnd,
    int wBar,
    DWORD bShow);

BOOL
NTAPI
NtUserShowWindow(
    HWND hWnd,
    LONG nCmdShow);

BOOL
NTAPI
NtUserShowWindowAsync(
    HWND hWnd,
    LONG nCmdShow);

BOOL
NTAPI
NtUserSoundSentry(VOID);

BOOL
NTAPI
NtUserSwitchDesktop(
    HDESK hDesktop);

BOOL
NTAPI
NtUserSystemParametersInfo(
    UINT uiAction,
    UINT uiParam,
    PVOID pvParam,
    UINT fWinIni);

DWORD
NTAPI
NtUserTestForInteractiveUser(
    DWORD dwUnknown1);

INT
NTAPI
NtUserToUnicodeEx(
    UINT wVirtKey,
    UINT wScanCode,
    PBYTE lpKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wFlags,
    HKL dwhkl);

BOOL
NTAPI
NtUserTrackMouseEvent(
    LPTRACKMOUSEEVENT lpEventTrack);

int
NTAPI
NtUserTranslateAccelerator(
    HWND Window,
    HACCEL Table,
    LPMSG Message);

BOOL
NTAPI
NtUserTranslateMessage(
    LPMSG lpMsg,
    UINT flags );

BOOL
NTAPI
NtUserUnhookWindowsHookEx(
    HHOOK Hook);

BOOL
NTAPI
NtUserUnhookWinEvent(
    HWINEVENTHOOK hWinEventHook);

BOOL
NTAPI
NtUserUnloadKeyboardLayout(
    HKL hKl);

BOOL
NTAPI
NtUserUnlockWindowStation(
    HWINSTA hWindowStation);

BOOL
NTAPI
NtUserUnregisterClass(
    PUNICODE_STRING ClassNameOrAtom,
    HINSTANCE hInstance,
    PCLSMENUNAME pClassMenuName);

BOOL
NTAPI
NtUserUnregisterHotKey(
    HWND hWnd,
    int id);

BOOL
NTAPI
NtUserUnregisterUserApiHook(VOID);

BOOL
NTAPI
NtUserUpdateInputContext(
    HIMC hIMC,
    DWORD dwType,
    DWORD_PTR dwValue);

DWORD
NTAPI
NtUserUpdateInstance(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2);

BOOL
NTAPI
NtUserUpdateLayeredWindow(
    HWND hwnd,
    HDC hdcDst,
    POINT *pptDst,
    SIZE *psize,
    HDC hdcSrc,
    POINT *pptSrc,
    COLORREF crKey,
    BLENDFUNCTION *pblend,
    DWORD dwFlags,
    RECT *prcDirty);

BOOL
NTAPI
NtUserUpdatePerUserSystemParameters(
    DWORD dwReserved,
    BOOL bEnable);

BOOL
NTAPI
NtUserUserHandleGrantAccess(
    IN HANDLE hUserHandle,
    IN HANDLE hJob,
    IN BOOL bGrant);

BOOL
NTAPI
NtUserValidateHandleSecure(
    HANDLE hHdl);

BOOL
NTAPI
NtUserValidateRect(
    HWND hWnd,
    CONST RECT *lpRect);

BOOL
APIENTRY
NtUserValidateTimerCallback(
    LPARAM lParam);

DWORD
NTAPI
NtUserVkKeyScanEx(
    WCHAR wChar,
    HKL KeyboardLayout,
    BOOL bUsehHK);

DWORD
NTAPI
NtUserWaitForInputIdle(
    IN HANDLE hProcess,
    IN DWORD dwMilliseconds,
    IN BOOL bSharedWow); /* Always FALSE */

DWORD
NTAPI
NtUserWaitForMsgAndEvent(
    DWORD Unknown0);

BOOL
NTAPI
NtUserWaitMessage(VOID);

DWORD
NTAPI
NtUserWin32PoolAllocationStats(
    DWORD Unknown0,
    DWORD Unknown1,
    DWORD Unknown2,
    DWORD Unknown3,
    DWORD Unknown4,
    DWORD Unknown5);

HWND
NTAPI
NtUserWindowFromPoint(
    LONG X,
    LONG Y);

DWORD
NTAPI
NtUserYieldTask(VOID);

/* NtUserBad
 * ReactOS-specific NtUser calls and their related structures, both which shouldn't exist.
 */

#define NOPARAM_ROUTINE_ISCONSOLEMODE             0xffff0001
#define ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING     0xfffe000d
#define ONEPARAM_ROUTINE_GETDESKTOPMAPPING        0xfffe000e
#define TWOPARAM_ROUTINE_SETMENUBARHEIGHT         0xfffd0050
#define TWOPARAM_ROUTINE_SETGUITHRDHANDLE         0xfffd0051
#define HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOWMOUSE 0xfffd0052

#define MSQ_STATE_CAPTURE   0x1
#define MSQ_STATE_ACTIVE    0x2
#define MSQ_STATE_FOCUS     0x3
#define MSQ_STATE_MENUOWNER 0x4
#define MSQ_STATE_MOVESIZE  0x5
#define MSQ_STATE_CARET     0x6

#define TWOPARAM_ROUTINE_ROS_UPDATEUISTATE   0x1004
#define HWNDPARAM_ROUTINE_ROS_NOTIFYWINEVENT 0x1005

BOOL
NTAPI
NtUserGetMonitorInfo(
    IN HMONITOR hMonitor,
    OUT LPMONITORINFO pMonitorInfo);

/* Should be done in usermode */

HMONITOR
NTAPI
NtUserMonitorFromPoint(
    IN POINT point,
    IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromRect(
    IN LPCRECT pRect,
    IN DWORD dwFlags);

HMONITOR
NTAPI
NtUserMonitorFromWindow(
    IN HWND hWnd,
    IN DWORD dwFlags);

typedef struct _SETSCROLLBARINFO
{
    int nTrackPos;
    int reserved;
    DWORD rgstate[CCHILDREN_SCROLLBAR + 1];
} SETSCROLLBARINFO, *PSETSCROLLBARINFO;

BOOL
NTAPI
NtUserSetScrollBarInfo(
    HWND hwnd,
    LONG idObject,
    SETSCROLLBARINFO *info);

#endif /* __WIN32K_NTUSER_H */

/* EOF */
