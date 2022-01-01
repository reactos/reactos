#ifndef _UNDOCUSER_H
#define _UNDOCUSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Built in class atoms */
#define WC_MENU       (MAKEINTATOM(0x8000))
#define WC_DESKTOP    (MAKEINTATOM(0x8001))
#define WC_DIALOG     (MAKEINTATOM(0x8002))
#define WC_SWITCH     (MAKEINTATOM(0x8003))
#define WC_ICONTITLE  (MAKEINTATOM(0x8004))

/* Non SDK Styles */
#define ES_COMBO 0x200 /* Parent is a combobox */
#define WS_MAXIMIZED  WS_MAXIMIZE
#define WS_MINIMIZED  WS_MINIMIZE

/* Non SDK ExStyles */
#define WS_EX_DRAGDETECT               0x00000002
#define WS_EX_MAKEVISIBLEWHENUNGHOSTED 0x00000800
#define WS_EX_FORCELEGACYRESIZENCMETR  0x00800000
#define WS_EX_UISTATEACTIVE            0x04000000
#define WS_EX_REDIRECTED               0x20000000
#define WS_EX_UISTATEKBACCELHIDDEN     0x40000000
#define WS_EX_UISTATEFOCUSRECTHIDDEN   0x80000000
#define WS_EX_SETANSICREATOR           0x80000000 // For WNDS_ANSICREATOR

/* Non SDK Window Message types. */
#define WM_SETVISIBLE       0x00000009
#define WM_ALTTABACTIVE     0x00000029
#define WM_ISACTIVEICON     0x00000035
#define WM_QUERYPARKICON    0x00000036
#define WM_CLIENTSHUTDOWN   0x0000003B
#define WM_COPYGLOBALDATA   0x00000049
#define WM_LOGONNOTIFY      0x0000004C
#define WM_KEYF1            0x0000004D
#define WM_KLUDGEMINRECT    0x0000008B
#define WM_UAHDRAWMENU      0x00000091
#define WM_UAHDRAWITEM      0x00000092 // WM_DRAWITEM
#define WM_UAHINITMENU      0x00000093
#define WM_UAHMEASUREITEM   0x00000094 // WM_MEASUREITEM
#define WM_UAHDRAWMENUNC    0x00000095
#define WM_NCUAHDRAWCAPTION 0x000000AE
#define WM_NCUAHDRAWFRAME   0x000000AF
#define WM_SYSTIMER         0x00000118
#define WM_LBTRACKPOINT     0x00000131
#define WM_CBLOSTTEXTFOCUS  0x00000167
#define LB_CARETON          0x000001a3
#define LB_CARETOFF         0x000001a4
#define MN_SETHMENU         0x000001e0
#define WM_DROPOBJECT       0x0000022A
#define WM_QUERYDROPOBJECT  0x0000022B
#define WM_BEGINDRAG        0x0000022C
#define WM_DRAGLOOP	        0x0000022D
#define WM_DRAGSELECT       0x0000022E
#define WM_DRAGMOVE	        0x0000022F
#define WM_IME_SYSTEM       0x00000287
#define WM_POPUPSYSTEMMENU  0x00000313
#define WM_UAHINIT          0x0000031b
#define WM_CBT              0x000003FF // ReactOS only.
#define WM_MAXIMUM          0x0001FFFF

/* Non SDK DCE types */
#define DCX_USESTYLE     0x00010000
#define DCX_KEEPCLIPRGN  0x00040000
#define DCX_KEEPLAYOUT   0x40000000
#define DCX_PROCESSOWNED 0x80000000

/* Non SDK TPM types.*/
#define TPM_SYSTEM_MENU  0x00000200

/* NtUserCreateWindowEx dwFlags bits. */
#define NUCWE_ANSI       0x00000001
#define NUCWE_SIDEBYSIDE 0x40000000

/* Caret timer ID */
#define IDCARETTIMER (0xffff)
#define ID_TME_TIMER (0xFFFA)

/* SetWindowPos undocumented flags */
#define SWP_NOCLIENTSIZE 0x0800
#define SWP_NOCLIENTMOVE 0x1000
#define SWP_STATECHANGED 0x8000

/* NtUserSetScrollInfo mask to return original position before it is change */
#define SIF_PREVIOUSPOS 4096

/* ScrollWindow uses the window DC, ScrollWindowEx doesn't */
#define SW_SCROLLWNDDCE 0x8000

/* Non SDK Queue state flags. */
#define QS_SMRESULT 0x8000 /* see "Undoc. Windows" */
//
#define QS_EVENT          0x2000
#define QS_SYSEVENT       (QS_EVENT|QS_SENDMESSAGE)
//

//
// Definitions used by WM_CLIENTSHUTDOWN
//
// Client Shutdown messages
#define MCS_ENDSESSION      1
#define MCS_QUERYENDSESSION 2
// Client Shutdown returns
#define MCSR_GOODFORSHUTDOWN  1
#define MCSR_SHUTDOWNFINISHED 2
#define MCSR_DONOTSHUTDOWN    3

//
// Definitions used by WM_LOGONNOTIFY
//
#define LN_LOGOFF             0x0
#define LN_SHELL_EXITED       0x2
#define LN_START_TASK_MANAGER 0x4
#define LN_LOCK_WORKSTATION   0x5
#define LN_UNLOCK_WORKSTATION 0x6
#define LN_MESSAGE_BEEP       0x9
#define LN_START_SCREENSAVE   0xA
#define LN_LOGOFF_CANCELED    0xB

//
// Undocumented flags for ExitWindowsEx
//
#define EWX_SHUTDOWN_CANCELED       0x0080
#define EWX_CALLER_SYSTEM           0x0100
#define EWX_CALLER_WINLOGON         0x0200
#define EWX_CALLER_WINLOGON_LOGOFF  0x1000 // WARNING!! Broken flag.
// All the range 0x0400 to 0x1000 is reserved for Winlogon.
// Flag 0x2000 appears to be a flag set when we call InitiateSystemShutdown* APIs (Winlogon shutdown APIs).
// 0x4000 is also reserved.
#define EWX_NOTIFY      0x8000
#define EWX_NONOTIFY    0x10000

// From WinCE 6.0 Imm.h SDK
// Returns for ImmProcessHotKey
#define IPHK_HOTKEY                     0x0001
#define IPHK_PROCESSBYIME               0x0002
#define IPHK_CHECKCTRL                  0x0004
#define IPHK_SKIPTHISKEY                0x0010

//
// Undocumented flags for DrawCaptionTemp
//
#define DC_NOVISIBLE 0x0800
#define DC_NOSENDMSG 0x2000
#define DC_FRAME     0x8000  // Missing from WinUser.H!

#define DC_DRAWCAPTIONMD  0x10000000
#define DC_REDRAWHUNGWND  0x20000000
#define DC_DRAWFRAMEMD    0x80000000

//
// Undocumented states for DrawFrameControl
//
#define DFCS_MENUARROWUP   0x0008
#define DFCS_MENUARROWDOWN 0x0010

//
// Undocumented flags for CreateProcess
//
#define STARTF_INHERITDESKTOP   0x40000000
#define STARTF_SCREENSAVER      0x80000000

#define MOD_WINLOGON_SAS 0x8000

#define CW_USEDEFAULT16 ((short)0x8000)

#define SBRG_SCROLLBAR     0 /* the scrollbar itself */
#define SBRG_TOPRIGHTBTN   1 /* the top or right button */
#define SBRG_PAGEUPRIGHT   2 /* the page up or page right region */
#define SBRG_SCROLLBOX     3 /* the scroll box */
#define SBRG_PAGEDOWNLEFT  4 /* the page down or page left region */
#define SBRG_BOTTOMLEFTBTN 5 /* the bottom or left button */

BOOL WINAPI UpdatePerUserSystemParameters(DWORD dwReserved, BOOL bEnable);
BOOL WINAPI SetLogonNotifyWindow(HWND Wnd);
BOOL WINAPI KillSystemTimer(HWND,UINT_PTR);
UINT_PTR WINAPI SetSystemTimer(HWND,UINT_PTR,UINT,TIMERPROC);
DWORD_PTR WINAPI SetSysColorsTemp(const COLORREF *, const HBRUSH *, DWORD_PTR);
BOOL WINAPI SetDeskWallPaper(LPCSTR);
VOID WINAPI ScrollChildren(HWND,UINT,WPARAM,LPARAM);
void WINAPI CalcChildScroll(HWND, INT);
BOOL WINAPI RegisterLogonProcess(DWORD,BOOL);
DWORD WINAPI GetAppCompatFlags(HTASK hTask);
DWORD WINAPI GetAppCompatFlags2(HTASK hTask);
LONG WINAPI CsrBroadcastSystemMessageExW(DWORD dwflags,
                                         LPDWORD lpdwRecipients,
                                         UINT uiMessage,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         PBSMINFO pBSMInfo);
BOOL WINAPI CliImmSetHotKey(DWORD dwID, UINT uModifiers, UINT uVirtualKey, HKL hKl);
HWND WINAPI SetTaskmanWindow(HWND);
HWND WINAPI GetTaskmanWindow(VOID);
HWND WINAPI GetProgmanWindow(VOID);
BOOL WINAPI SetShellWindow(HWND);
BOOL WINAPI SetShellWindowEx(HWND, HWND);

BOOL WINAPI DrawCaptionTempA(HWND,HDC,const RECT*,HFONT,HICON,LPCSTR,UINT);
BOOL WINAPI DrawCaptionTempW(HWND,HDC,const RECT*,HFONT,HICON,LPCWSTR,UINT);
BOOL WINAPI PaintMenuBar(HWND hWnd, HDC hDC, ULONG left, ULONG right, ULONG top, BOOL bActive);

#ifdef UNICODE
#define DrawCaptionTemp DrawCaptionTempW
#else
#define DrawCaptionTemp DrawCaptionTempA
#endif

//
// Hard error balloon package
//
typedef struct _BALLOON_HARD_ERROR_DATA
{
    DWORD cbHeaderSize;
    DWORD Status;
    DWORD dwType; /* any combination of the MB_ message box types */
    ULONG_PTR TitleOffset;
    ULONG_PTR MessageOffset;
} BALLOON_HARD_ERROR_DATA, *PBALLOON_HARD_ERROR_DATA;

//
// Undocumented SoftModalMessageBox() API, which constitutes
// the basis of all implementations of the MessageBox*() APIs.
//
typedef struct _MSGBOXDATA
{
    MSGBOXPARAMSW mbp;          // Size: 0x28 (on x86), 0x50 (on x64)
    HWND     hwndOwner;
#if defined(_WIN32) && (_WIN32_WINNT >= _WIN32_WINNT_WIN7) /* (NTDDI_VERSION >= NTDDI_WIN7) */
    DWORD    dwPadding;
#endif
    WORD     wLanguageId;
    INT*     pidButton;         // Array of button IDs
    LPCWSTR* ppszButtonText;    // Array of button text strings
    DWORD    dwButtons;         // Number of buttons
    UINT     uDefButton;        // Default button ID
    UINT     uCancelId;         // Button ID for Cancel action
#if (_WIN32_WINNT >= _WIN32_WINNT_WINXP)    /* (NTDDI_VERSION >= NTDDI_WINXP) */
    DWORD    dwTimeout;         // Message box timeout
#endif
    DWORD    dwReserved0;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)     /* (NTDDI_VERSION >= NTDDI_WIN7) */
    DWORD    dwReserved[4];
#endif
} MSGBOXDATA, *PMSGBOXDATA, *LPMSGBOXDATA;

#if defined(_WIN64)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)     /* (NTDDI_VERSION >= NTDDI_WIN7) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x98);
#elif (_WIN32_WINNT <= _WIN32_WINNT_WS03)   /* (NTDDI_VERSION <= NTDDI_WS03) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x88);
#endif

#else

#if (_WIN32_WINNT <= _WIN32_WINNT_WIN2K)    /* (NTDDI_VERSION <= NTDDI_WIN2KSP4) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x48);
#elif (_WIN32_WINNT >= _WIN32_WINNT_WIN7)   /* (NTDDI_VERSION >= NTDDI_WIN7) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x60);
#else // (_WIN32_WINNT == _WIN32_WINNT_WINXP || _WIN32_WINNT == _WIN32_WINNT_WS03) /* (NTDDI_VERSION == NTDDI_WS03) */
C_ASSERT(sizeof(MSGBOXDATA) == 0x4C);
#endif

#endif /* defined(_WIN64) */

int WINAPI SoftModalMessageBox(IN LPMSGBOXDATA lpMsgBoxData);

int
WINAPI
MessageBoxTimeoutA(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout);

int
WINAPI
MessageBoxTimeoutW(
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout);

#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif

LPCWSTR WINAPI MB_GetString(IN UINT wBtn);

/* dwType for NtUserUpdateInputContext */
typedef enum _UPDATE_INPUT_CONTEXT
{
    UIC_CLIENTIMCDATA = 0,
    UIC_IMEWINDOW
} UPDATE_INPUT_CONTEXT;

//
// User api hook
//

typedef LRESULT(CALLBACK *WNDPROC_OWP)(HWND,UINT,WPARAM,LPARAM,ULONG_PTR,PDWORD);
typedef int (WINAPI *SETWINDOWRGN)(HWND hWnd, HRGN hRgn, BOOL bRedraw);
typedef BOOL (WINAPI *GETSCROLLINFO)(HWND,INT,LPSCROLLINFO);
typedef INT (WINAPI *SETSCROLLINFO)(HWND,int,LPCSCROLLINFO,BOOL);
typedef BOOL (WINAPI *ENABLESCROLLBAR)(HWND,UINT,UINT);
typedef BOOL (WINAPI *ADJUSTWINDOWRECTEX)(LPRECT,DWORD,BOOL,DWORD);
typedef int (WINAPI *GETSYSTEMMETRICS)(int);
typedef BOOL (WINAPI *SYSTEMPARAMETERSINFOA)(UINT,UINT,PVOID,UINT);
typedef BOOL (WINAPI *SYSTEMPARAMETERSINFOW)(UINT,UINT,PVOID,UINT);
typedef BOOL (WINAPI *FORCERESETUSERAPIHOOK)(HINSTANCE);
typedef BOOL (WINAPI *DRAWFRAMECONTROL)(HDC,LPRECT,UINT,UINT);
typedef BOOL (WINAPI *DRAWCAPTION)(HWND,HDC,LPCRECT,UINT);
typedef BOOL (WINAPI *MDIREDRAWFRAME)(HWND,DWORD);
typedef DWORD (WINAPI *GETREALWINDOWOWNER)(HWND);

typedef struct _UAHOWP
{
    BYTE*  MsgBitArray;
    DWORD  Size;
} UAHOWP, *PUAHOWP;

#define UAH_HOOK_MESSAGE(uahowp, msg) uahowp.MsgBitArray[msg/8] |= (1 << (msg % 8));
#define UAH_IS_MESSAGE_HOOKED(uahowp, msg) (uahowp.MsgBitArray[msg/8] & (1 << (msg % 8)))
#define UAHOWP_MAX_SIZE WM_USER/8

typedef struct tagUSERAPIHOOK
{
    DWORD       size;
    WNDPROC     DefWindowProcA;
    WNDPROC     DefWindowProcW;
    UAHOWP      DefWndProcArray;
    GETSCROLLINFO GetScrollInfo;
    SETSCROLLINFO SetScrollInfo;
    ENABLESCROLLBAR EnableScrollBar;
    ADJUSTWINDOWRECTEX AdjustWindowRectEx;
    SETWINDOWRGN SetWindowRgn;
    WNDPROC_OWP PreWndProc;
    WNDPROC_OWP PostWndProc;
    UAHOWP      WndProcArray;
    WNDPROC_OWP PreDefDlgProc;
    WNDPROC_OWP PostDefDlgProc;
    UAHOWP      DlgProcArray;
    GETSYSTEMMETRICS GetSystemMetrics;
    SYSTEMPARAMETERSINFOA SystemParametersInfoA;
    SYSTEMPARAMETERSINFOW SystemParametersInfoW;
    FORCERESETUSERAPIHOOK ForceResetUserApiHook;
    DRAWFRAMECONTROL DrawFrameControl;
    DRAWCAPTION DrawCaption;
    MDIREDRAWFRAME MDIRedrawFrame;
    GETREALWINDOWOWNER GetRealWindowOwner;
} USERAPIHOOK, *PUSERAPIHOOK;

typedef enum _UAPIHK
{
    uahLoadInit,
    uahStop,
    uahShutdown
} UAPIHK, *PUAPIHK;

typedef BOOL(CALLBACK *USERAPIHOOKPROC)(UAPIHK State, PUSERAPIHOOK puah);

typedef struct _USERAPIHOOKINFO
{
    DWORD m_size;
    LPCWSTR m_dllname1;
    LPCWSTR m_funname1;
    LPCWSTR m_dllname2;
    LPCWSTR m_funname2;
} USERAPIHOOKINFO,*PUSERAPIHOOKINFO;

#if (WINVER == _WIN32_WINNT_WINXP)
BOOL WINAPI RegisterUserApiHook(HINSTANCE hInstance, USERAPIHOOKPROC CallbackFunc);
#elif (WINVER == _WIN32_WINNT_WS03)
BOOL WINAPI RegisterUserApiHook(PUSERAPIHOOKINFO puah);
#endif

BOOL WINAPI UnregisterUserApiHook(VOID);

/* dwType for NtUserQueryInputContext */
typedef enum _QUERY_INPUT_CONTEXT
{
    QIC_INPUTPROCESSID = 0,
    QIC_INPUTTHREADID,
    QIC_DEFAULTWINDOWIME,
    QIC_DEFAULTIMC
} QUERY_INPUT_CONTEXT;

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif
