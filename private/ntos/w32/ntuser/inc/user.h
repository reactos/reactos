/*++ build version: 0002    // increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: user.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains stuff shared by all the modules of the USER.DLL.
*
* History:
* 09-18-90 DarrinM      Created.
* 04-27-91 DarrinM      Merged in USERCALL.H, removed some dead wood.
\***************************************************************************/

#ifndef _USER_
#define _USER_

/******************************WOW64***NOTE********************************\
* Note: Win32k Memory shared with User-Mode and Wow64
*
* For Wow64 (Win32 apps on Win64) we build a 32-bit version
* of user32.dll & gdi32.dll which can run against the 64-bit kernel
* with no changes to the 64-bit kernel code.
*
* For the 32 on 64 bit dlls all data structures which are shared with
* win32k must be 64-bit. These data structures include the shared
* sections, as well as members of the TEB.
* These shared data structures are now declared so that they can be
* built as 32 bit in a 32 bit dll, 64 bit in a 64 bit dll, and now
* 64 bit in a 32 bit dll.
*
* The following rules should be followed when declaring
* shared data structures:
*
*     Pointers in shared data structures use the KPTR_MODIFIER in their
*     declaration.
*
*     Handles in shared data structures are declared KHxxx.
*
*     xxx_PTR changes to KERNEL_xxx_PTR.
*
*     Pointers to basic types are declared as KPxxx;
*
* Also on Wow64 every thread has both a 32-bit TEB and a 64-bit TEB.
* GetCurrentTeb() returns the current 32-bit TEB while the kernel
* will allways reference the 64-bit TEB.
*
* All client side references to shared data in the TEB should use
* the new GetCurrentTebShared() macro which returns the 64-bit TEB
* for Wow64 builds and returns GetCurrentTeb() for regular builds.
* The exception to this rule is LastErrorValue, which should allways
* be referenced through GetCurrentTeb().
*
* Ex:
*
* DECLARE_HANDLE(HFOO);
*
* typedef struct _MY_STRUCT *PMPTR;
*
* struct _SHARED_STRUCT
* {
*     struct _SHARED_STRUCT *   pNext;
*     PMPTR                     pmptr;
*     HFOO                      hFoo;
*     UINT_PTR                  cb;
*     PBYTE                     pb;
*     PVOID                     pv;
*
*     DWORD                     dw;
*     USHORT                    us;
* } SHARED_STRUCT;
*
*
* Changes to:
*
*
* DECLARE_HANDLE(HFOO);
* DECLARE_KHANDLE(HFOO);
*
* typedef struct _MY_STRUCT * KPTR_MODIFIER   PMPTR;
*
* struct _SHARED_STRUCT
* {
*     struct _SHARED_STRUCT * KPTR_MODIFIER   pNext;
*     PMPTR                     pmptr;
*     KHFOO                     hFoo;
*     KERNEL_UINT_PTR           cb;
*     KPBYTE                    pb;
*     KERNEL_PVOID              pv;
*
*     DWORD                     dw;
*     USHORT                    us;
* } SHARED_STRUCT;
*
\***************************************************************************/
#include "w32wow64.h"

DECLARE_KHANDLE(HIMC);

/*
 * Enable warnings that are turned off default for NT but we want on
 */
#ifndef RC_INVOKED       // RC can't handle #pragmas
    #pragma warning(error:4100)   // Unreferenced formal parameter
    #pragma warning(error:4101)   // Unreferenced local variable
    // #pragma warning(error:4702)   // Unreachable code
    #pragma warning(error:4705)   // Statement has no effect
#endif // RC_INVOKED

#if !defined(FASTCALL)
    #if defined(_X86_)
        #define FASTCALL    _fastcall
    #else // defined(_X86_)
        #define FASTCALL
    #endif // defined(_X86_)
#endif // !defined(FASTCALL)

#ifdef UNICODE
    #define UTCHAR WCHAR
#else // UINCODE
    #define UTCHAR UCHAR
#endif // UINCODE

/*
 * These types are needed before they are fully defined.
 */
typedef struct tagWINDOWSTATION     * KPTR_MODIFIER PWINDOWSTATION;
typedef struct _LOCKRECORD          * KPTR_MODIFIER PLR;
typedef struct _TL                  * KPTR_MODIFIER PTL;
typedef struct tagDESKTOP           * KPTR_MODIFIER PDESKTOP;
typedef struct tagTDB               * KPTR_MODIFIER PTDB;
typedef struct tagSVR_INSTANCE_INFO *PSVR_INSTANCE_INFO;
typedef struct _MOVESIZEDATA        *PMOVESIZEDATA;
typedef struct tagCURSOR            * KPTR_MODIFIER PCURSOR;
typedef struct tagPOPUPMENU         * KPTR_MODIFIER PPOPUPMENU;
typedef struct tagQMSG              * KPTR_MODIFIER PQMSG;
typedef struct tagWND               * KPTR_MODIFIER PWND;
typedef struct _ETHREAD             *PETHREAD;
typedef struct tagDESKTOPINFO       * KPTR_MODIFIER PDESKTOPINFO;
typedef struct tagDISPLAYINFO       * KPTR_MODIFIER PDISPLAYINFO;
typedef struct tagCLIENTTHREADINFO  * KPTR_MODIFIER PCLIENTTHREADINFO;
typedef struct tagDCE               * KPTR_MODIFIER PDCE;
typedef struct tagSPB               * KPTR_MODIFIER PSPB;
typedef struct tagQ                 * KPTR_MODIFIER PQ;
typedef struct tagTHREADINFO        * KPTR_MODIFIER PTHREADINFO;
typedef struct tagPROCESSINFO       * KPTR_MODIFIER PPROCESSINFO;
typedef struct tagWOWTHREADINFO     *PWOWTHREADINFO;
typedef struct tagPERUSERDATA       *PPERUSERDATA;
typedef struct tagPERUSERSERVERINFO *PPERUSERSERVERINFO;
typedef struct tagTERMINAL          *PTERMINAL;
typedef struct _CLIENTINFO          *PCLIENTINFO;
typedef struct tagMENU              * KPTR_MODIFIER PMENU;
typedef struct tagHOOK              * KPTR_MODIFIER PHOOK;
typedef struct _HANDLEENTRY         * KPTR_MODIFIER PHE;
typedef struct tagSERVERINFO        * KPTR_MODIFIER PSERVERINFO;
typedef struct _CALLPROCDATA        * KPTR_MODIFIER PCALLPROCDATA;
typedef struct tagCLS               * KPTR_MODIFIER PCLS;
typedef struct tagMONITOR           * KPTR_MODIFIER PMONITOR;
DECLARE_HANDLE(HQ);

/*
 * This name is used both in kernel\server.c and ntuser\server\exitwin.c
 */
#define ICON_PROP_NAME  L"SysIC"

/*
 * Define DbgPrint to be something bogus on free builds so we won't
 * include it accidentally.
 */
#if DBG
#else
#define DbgPrint UserDbgPrint
#endif

typedef struct tagMBSTRING
{
    WCHAR szName[15];
    UINT  uID;
    UINT  uStr;
} MBSTRING;

/*
 * SIZERECT is a rectangle represented by a top-left coordinate, width,
 * and height.
 *
 * Hungarian is "src".
 */
typedef struct tagSIZERECT {
    int x;
    int y;
    int cx;
    int cy;
} SIZERECT, *PSIZERECT, *LPSIZERECT;

typedef const SIZERECT * PCSIZERECT;
typedef const SIZERECT * LPCSIZERECT;


void RECTFromSIZERECT(PRECT prc, PCSIZERECT psrc);
void SIZERECTFromRECT(PSIZERECT psrc, LPCRECT prc);

/*
 * Use these macros to unpack things packed by MAKELPARAM.
 */

#define LOSHORT(l)          ((short)LOWORD(l))
#define HISHORT(l)          ((short)HIWORD(l))

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)    ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))
#endif

#ifdef _USERK_
    #define GetClientInfo() (((PTHREADINFO)(W32GetCurrentThread()))->pClientInfo)
#else
    // We don't grab it this way in the kernel in case it is a kernel only thread
    #define GetClientInfo() ((PCLIENTINFO)((NtCurrentTebShared())->Win32ClientInfo))
#endif

/* Used by xxxSleepTask */
#define HEVENT_REMOVEME ((HANDLE)IntToPtr( 0xFFFFFFFF ))


/*
 * Access to system metrics, colors, and brushes.
 */
#define SYSMET(i)             ((int)gpsi->aiSysMet[SM_##i])
#define SYSMETRTL(i)          ((int)gpsi->aiSysMet[SM_##i])
#define SYSRGB(i)             gpsi->argbSystem[COLOR_##i]
#define SYSRGBRTL(i)          gpsi->argbSystem[COLOR_##i]
#define SYSHBR(i)             gpsi->ahbrSystem[COLOR_##i]
#define SYSHBRUSH(i)          gpsi->ahbrSystem[i]

#ifdef _USERK_
    #define SYSMETFROMPROCESS(i)  gpsi->aiSysMet[SM_##i]
#endif

/***************************************************************************\
* These cool constants can be used to specify rops
\***************************************************************************/

#define DESTINATION (DWORD)0x00AA0000
#define SOURCE      (DWORD)0x00CC0000
#define PATTERN     (DWORD)0x00F00000

/**************************
*  Chicago equates
***************************/
#define BI_CHECKBOX       0
#define BI_RADIOBUTTON    1
#define BI_3STATE         2

#define NUM_BUTTON_TYPES  3
#define NUM_BUTTON_STATES 4

/*
 * Total number of strings used as button strings in MessageBoxes
 */
#define  MAX_MB_STRINGS    11


/*
 * Rectangle macros.  Inlining these is both faster and smaller
 */
#define CopyRect        CopyRectInl
#define EqualRect       EqualRectInl
#define SetRectEmpty    SetRectEmptyInl

__inline void
CopyRectInl(LPRECT prcDest, LPCRECT prcSrc)
{
    *prcDest = *prcSrc;
}

__inline DWORD
EqualRectInl(LPCRECT prc1, LPCRECT prc2)
{
    return RtlEqualMemory(prc1, prc2, sizeof(*prc1));
}

__inline void
SetRectEmptyInl(LPRECT prc)
{
    RtlZeroMemory(prc, sizeof(*prc));
}

/***************************************************************************\
* ANSI/Unicode function names
*
* For non-API Client/Server stubs, an "A" or "W" suffix must be added.
* (API function names are generated by running wcshdr.exe over winuser.x)
*
\***************************************************************************/
#ifdef UNICODE
    #define TEXT_FN(fn) fn##W
#else // UNICODE
    #define TEXT_FN(fn) fn##A
#endif // UNICODE

#ifdef UNICODE
    #define BYTESTOCHARS(cb) ((cb) / sizeof(TCHAR))
    #define CHARSTOBYTES(cch) ((cch) * sizeof(TCHAR))
#else // UNICODE
    #define BYTESTOCHARS(cb) (cb)
    #define CHARSTOBYTES(cch) (cch)
#endif // UNICODE

/*
 * Internal window class names
 */
#define DESKTOPCLASS    MAKEINTATOM(0x8001)
#define DIALOGCLASS     MAKEINTATOM(0x8002)
#define SWITCHWNDCLASS  MAKEINTATOM(0x8003)
#define ICONTITLECLASS  MAKEINTATOM(0x8004)
#define INFOCLASS       MAKEINTATOM(0x8005)
#define TOOLTIPCLASS    MAKEINTATOM(0x8006)
#define GHOSTCLASS      MAKEINTATOM(0x8007)
#define MENUCLASS       MAKEINTATOM(0x8000)     /* Public Knowledge */

//
// System timer IDs
//
#define IDSYS_LAYER         0x0000FFF5L
#define IDSYS_FADE          0x0000FFF6L
#define IDSYS_WNDTRACKING   0x0000FFF7L
#define IDSYS_FLASHWND      0x0000FFF8L
#define IDSYS_MNAUTODISMISS 0x0000FFF9L
#define IDSYS_MOUSEHOVER    0x0000FFFAL
#define IDSYS_MNANIMATE     0x0000FFFBL
#define IDSYS_MNDOWN        MFMWFP_DOWNARROW /* 0xFFFFFFFC */
#define IDSYS_LBSEARCH      0x0000FFFCL
#define IDSYS_MNUP          MFMWFP_UPARROW   /* 0xFFFFFFFD */
#define IDSYS_STANIMATE     0x0000FFFDL
#define IDSYS_MNSHOW        0x0000FFFEL
#define IDSYS_SCROLL        0x0000FFFEL
#define IDSYS_MNHIDE        0x0000FFFFL
#define IDSYS_CARET         0x0000FFFFL


/*
 * Special case string token codes.  These must be the same as in the resource
 * compiler's RC.H file.
 */
/*
 * NOTE: Order is assumed and much be this way for applications to be
 * compatable with windows 2.0
 */
#define CODEBIT             0x80
#define BUTTONCODE          0x80
#define EDITCODE            0x81
#define STATICCODE          0x82
#define LISTBOXCODE         0x83
#define SCROLLBARCODE       0x84
#define COMBOBOXCODE        0x85
#define MDICLIENTCODE       0x86
#define COMBOLISTBOXCODE    0x87

/*
 * Internal window classes. These numbers serve as indices into the
 * atomSysClass table so that we can get the atoms for the various classes.
 * The order of the control classes (through COMBOLISTBOXCLASS) is assumed
 * to be the same as the class codes above.
 */
#define ICLS_BUTTON         0
#define ICLS_EDIT           1
#define ICLS_STATIC         2
#define ICLS_LISTBOX        3
#define ICLS_SCROLLBAR      4
#define ICLS_COMBOBOX       5       // End of special dlgmgr indices

#define ICLS_MDICLIENT      6
#define ICLS_COMBOLISTBOX   7
#define ICLS_DDEMLEVENT     8
#define ICLS_DDEMLMOTHER    9
#define ICLS_DDEML16BIT     10
#define ICLS_DDEMLCLIENTA   11
#define ICLS_DDEMLCLIENTW   12
#define ICLS_DDEMLSERVERA   13
#define ICLS_DDEMLSERVERW   14
#define ICLS_IME            15

#define ICLS_CTL_MAX        16       // Number of public control classes


#define ICLS_DESKTOP        16
#define ICLS_DIALOG         17
#define ICLS_MENU           18
#define ICLS_SWITCH         19
#define ICLS_ICONTITLE      20
#define ICLS_TOOLTIP        21
#define ICLS_MAX            22  // Number of system classes

/*
 * Directory name for windowstations and desktops
 */
#define WINSTA_DIR  L"\\Windows\\WindowStations"
#define WINSTA_SESSION_DIR  L"\\Sessions\\xxxxxxxxxxx\\Windows\\WindowStations"
#define WINSTA_NAME L"Service-0x0000-0000$"
#define MAX_SESSION_PATH   256
#define SESSION_ROOT L"\\Sessions"

/***************************************************************************\
* Normal Stuff
*
* Nice normal typedefs, defines, prototypes, etc that everyone wants to share.
*
\***************************************************************************/

/*
 * Define size limit of callback data.  Below or equal to this limit, put data
 * on the client-side stack.  Above this limit allocate virtual memory
 * for the data
 */
#define CALLBACKSTACKLIMIT  (KERNEL_PAGE_SIZE / 2)

/*
 * Capture buffer definition for callbacks
 */
typedef struct _CAPTUREBUF {
    DWORD cbCallback;
    DWORD cbCapture;
    DWORD cCapturedPointers;
    PBYTE pbFree;
    DWORD offPointers;
    PVOID pvVirtualAddress;
} CAPTUREBUF, *PCAPTUREBUF;

/*
 * Callback return status
 */
typedef struct _CALLBACKSTATUS {
    KERNEL_ULONG_PTR retval;
    DWORD cbOutput;
    KERNEL_PVOID pOutput;
} CALLBACKSTATUS, *PCALLBACKSTATUS;

#define IS_PTR(p)       ((((ULONG_PTR)(p)) & ~MAXUSHORT) != 0)
#define PTR_TO_ID(p)    ((USHORT)(((ULONG_PTR)(p)) & MAXUSHORT))

//
// Strings are counted 16-bit character strings. If they are
// NULL terminated, Length does not include trailing NULL.
//
typedef struct _LARGE_STRING {
    ULONG Length;
    ULONG MaximumLength : 31;
    ULONG bAnsi : 1;
    KERNEL_PVOID Buffer;
} LARGE_STRING, *PLARGE_STRING;

typedef struct _LARGE_ANSI_STRING {
    ULONG Length;
    ULONG MaximumLength : 31;
    ULONG bAnsi : 1;
    KPSTR Buffer;
} LARGE_ANSI_STRING, *PLARGE_ANSI_STRING;

typedef struct _LARGE_UNICODE_STRING {
    ULONG Length;
    ULONG MaximumLength : 31;
    ULONG bAnsi : 1;
    KPWSTR Buffer;
} LARGE_UNICODE_STRING, *PLARGE_UNICODE_STRING;

/*
 * String macros
 */
__inline BOOL IsEmptyString(PVOID p, ULONG bAnsi)
{
    return (BOOL)!(bAnsi ? *(LPSTR)p : *(LPWSTR)p);
}
__inline void NullTerminateString(PVOID p, ULONG bAnsi)
{
    if (bAnsi) *(LPSTR)p = (CHAR)0; else *(LPWSTR)p = (WCHAR)0;
}
__inline UINT StringLength(PVOID p, ULONG bAnsi)
{
    return (bAnsi ? strlen((LPSTR)p) : wcslen((LPWSTR)p));
}

typedef struct _CTLCOLOR {
    COLORREF crText;
    COLORREF crBack;
    int iBkMode;
} CTLCOLOR, *PCTLCOLOR;


/*
 * This is used by the cool client side DrawIcon code
 */
typedef struct _DRAWICONEXDATA {
    HBITMAP hbmMask;
    HBITMAP hbmColor;
    int cx;
    int cy;
} DRAWICONEXDATA;

/*
 * Static items stored in the TEB
 */
typedef struct _CALLBACKWND {
    KHWND   hwnd;
    PWND    pwnd;
} CALLBACKWND, *PCALLBACKWND;

#define CVKKEYCACHE                 32
#define CBKEYCACHE                  (CVKKEYCACHE >> 2)

#define CVKASYNCKEYCACHE            32
#define CBASYNCKEYCACHE             (CVKASYNCKEYCACHE >> 2)

/*
 * The offset to cSpins must match WIN32_CLIENT_INFO_SPIN_COUNT defined
 * in ntpsapi.h.  GDI uses this offset to reset the spin count.
 * WARNING! This struct cannot be made larger without changing the TEB struct:
 * It must fit in ULONG_PTR Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH]; (ntpsapi.h)
 * (ifdef FE_SB, sizeof(CLIENTINFO) == 0x7c == 4 * WIN32_CLIENT_INFO_LENGTH)
 */
typedef struct _CLIENTINFO {
    KERNEL_ULONG_PTR    CI_flags;           // Needs to be first because CSR sets this
    KERNEL_ULONG_PTR    cSpins;             // GDI resets this
    DWORD               dwExpWinVer;
    DWORD               dwCompatFlags;
    DWORD               dwCompatFlags2;
    DWORD               dwTIFlags;
    PDESKTOPINFO        pDeskInfo;
    KERNEL_ULONG_PTR    ulClientDelta;
    PHOOK               phkCurrent;
    DWORD               fsHooks;
    CALLBACKWND         CallbackWnd;
    DWORD               dwHookCurrent;
    int                 cInDDEMLCallback;
    PCLIENTTHREADINFO   pClientThreadInfo;
    KERNEL_ULONG_PTR    dwHookData;
    DWORD               dwKeyCache;
    BYTE                afKeyState[CBKEYCACHE];
    DWORD               dwAsyncKeyCache;
    BYTE                afAsyncKeyState[CBASYNCKEYCACHE];
    BYTE                afAsyncKeyStateRecentDown[CBASYNCKEYCACHE];
    KHKL                hKL;
    WORD                CodePage;

    BYTE                achDbcsCF[2]; // Save ANSI DBCS LeadByte character code
                                      // in this field for ANSI to Unicode.
                                      // Uses SendMessageA/PostMessageA from CLIENT
                                      // to SERVER (index 0)
                                      //  And...
                                      // Uses SendMessageA/DispatchMessageA
                                      // for CLIENT to CLIENT (index 1)
    KERNEL_MSG          msgDbcsCB;    // Save ANSI DBCS character message in
                                      // this field for convert Unicode to ANSI.
                                      // Uses GetMessageA/PeekMessageA from
                                      // SERVER to CLIENT
#ifdef LATER
    EVENTMSG            eventCached;  // Cached Event for Journal Hook
#endif
} CLIENTINFO, *PCLIENTINFO;


#define CI_IN_SYNC_TRANSACTION 0x00000001
#define CI_PROCESSING_QUEUE    0x00000002
#define CI_16BIT               0x00000004
#define CI_INITIALIZED         0x00000008
#define CI_INTERTHREAD_HOOK    0x00000010
#define CI_REGISTERCLASSES     0x00000020
#define CI_INPUTCONTEXT_REINIT 0x00000040

//
// THREAD_CODEPAGE()
//
// Returns the CodePage based on the current keyboard layout.
//
#ifdef _USERK_
    #define THREAD_CODEPAGE() (PtiCurrent()->pClientInfo->CodePage)
#else // _USERK_
    #define THREAD_CODEPAGE() (GetClientInfo()->CodePage)
#endif // _USERK_

// WMCR_IR_DBCSCHAR and DBCS Macros
/*
 * Flags used for the WM_CHAR  HIWORD of wParam for DBCS messaging.
 *  (LOWORD of wParam will have character codepoint)
 */
#define WMCR_IR_DBCSCHAR       0x80000000
/*
 * Macros to determine this is DBCS message or not.
 */
#define IS_DBCS_MESSAGE(DbcsChar) (((DWORD)(DbcsChar)) & 0x0000FF00)

/*
 * Macros for IR_DBCSCHAR format to/from regular format.
 */
#define MAKE_IR_DBCSCHAR(DbcsChar) \
        (IS_DBCS_MESSAGE((DbcsChar)) ?                                     \
            (MAKEWPARAM(MAKEWORD(HIBYTE((DbcsChar)),LOBYTE((DbcsChar))),0)) : \
            ((WPARAM)((DbcsChar) & 0x00FF))                                   \
        )

#define MAKE_WPARAM_DBCSCHAR(DbcsChar) \
        (IS_DBCS_MESSAGE((DbcsChar)) ?                                     \
            (MAKEWPARAM(MAKEWORD(HIBYTE((DbcsChar)),LOBYTE((DbcsChar))),0)) : \
            ((WPARAM)((DbcsChar) & 0x00FF))                                   \
        )

#define DBCS_CHARSIZE   (2)

#define IS_DBCS_ENABLED()  (TEST_SRVIF(SRVIF_DBCS))
#define _IS_IME_ENABLED()  (TEST_SRVIF(SRVIF_IME))
#ifdef _IMMCLI_
    #define IS_IME_ENABLED()   (gpsi && _IS_IME_ENABLED())
#else   // _IMMCLI_
    #define IS_IME_ENABLED()   _IS_IME_ENABLED()
#endif  // _IMMCLI_

#define IS_DBCS_HKL()       (IS_ANY_DBCS_CODEPAGE(THREAD_CODEPAGE()))
#define IS_DBCS_INPUT()     (IS_DBCS_ENABLED() || IS_DBCS_HKL())


#define CP_JAPANESE     (932)
#define CP_KOREAN       (949)
#define CP_CHINESE_SIMP (936)
#define CP_CHINESE_TRAD (950)

#define IS_DBCS_CODEPAGE(wCodePage) \
            ((wCodePage) == CP_JAPANESE || \
             (wCodePage) == CP_KOREAN || \
             (wCodePage) == CP_CHINESE_TRAD || \
             (wCodePage) == CP_CHINESE_SIMP)

#define IS_DBCS_CHARSET(charset) \
            ((charset) == SHIFTJIS_CHARSET || \
             (charset) == HANGEUL_CHARSET || \
             (charset) == CHINESEBIG5_CHARSET || \
             (charset) == GB2312_CHARSET)

#define IS_JPN_1BYTE_KATAKANA(c)   ((c) >= 0xa1 && (c) <= 0xdf)


// IMM dynamic loading support
#define IMM_MAGIC_CALLER_ID     (0x19650412)

BOOL User32InitializeImmEntryTable(DWORD dwMagic);

#define IS_MIDEAST_ENABLED()   (TEST_SRVIF(SRVIF_MIDEAST))

/*
 * Flags used for the WM_CLIENTSHUTDOWN wParam.
 */
#define WMCS_EXIT             0x0001
#define WMCS_QUERYEND         0x0002
#define WMCS_SHUTDOWN         0x0004
#define WMCS_CONTEXTLOGOFF    0x0008
#define WMCS_ENDTASK          0x0010
#define WMCS_CONSOLE          0x0020
#define WMCS_NODLGIFHUNG      0x0040
#define WMCS_NORETRY          0x0080
#define WMCS_LOGOFF           ENDSESSION_LOGOFF  /* from winuser.w */

/*
 * WM_CLIENTSHUTDOWN return value
 */
#define WMCSR_ALLOWSHUTDOWN     1
#define WMCSR_DONE              2
#define WMCSR_CANCEL            3

/*
 * We don't need 64-bit intermediate precision so we use this macro
 * instead of calling MulDiv.
 */
#define MultDiv(x, y, z)        (((INT)(x) * (INT)(y) + (INT)(z) / 2) / (INT)(z))

typedef DWORD  ICH;
typedef ICH *LPICH;

typedef struct _PROPSET {
    HANDLE hData;
    ATOM atom;
} PROPSET, *PPROPSET;

/*
 * Old MENUHBM used to be here. They are now public for NT5 and defined
 *   in winuser.w as HBMMENU_*
 */

/*
 * Internal menu flags stored in pMenu->fFlags.
 * High order bits are used for public MNS_ flags defined in winuser.w
 */
#define MFISPOPUP               0x00000001
#define MFMULTIROW              0x00000002
#define MFUNDERLINE             0x00000004
#define MFWINDOWDC              0x00000008  /* Window DC vs Client area DC when drawing*/
#define MFINACTIVE              0x00000010
#define MFRTL                   0x00000020
#define MFDESKTOP               0x00000040 /* Set on the desktop menu AND its submenus */
#define MFSYSMENU               0x00000080 /* Set on desktop menu but NOT on its submenus */
#define MFAPPSYSMENU            0x00000100 /* Set on (sub)menu we return to the app via GetSystemMenu */
#define MFLAST                  0x00000100

#if (MNS_LAST <= MFLAST)
    #error MNS_ AND MF defines conflict
#endif // (MNS_LAST <= MFLAST)

// Event stuff --------------------------------------------

typedef struct tagEVENT_PACKET {
    DWORD EventType;    // == apropriate afCmd filter flag
    WORD  fSense;       // TRUE means flag on is passed.
    WORD  cbEventData;  // size of data starting at Data field.
    DWORD Data;         // event specific data - must be last
} EVENT_PACKET, *PEVENT_PACKET;

// Window long offsets in mother window     (szDDEMLMOTHERCLASS)

#define GWLP_INSTANCE_INFO  0       // PCL_INSTANCE_INFO


// Window long offsets in client window     (szDDEMLCLIENTCLASS)

#define GWLP_PCI            0
#define GWL_CONVCONTEXT     GWLP_PCI + sizeof(PVOID)
#define GWL_CONVSTATE       GWL_CONVCONTEXT + sizeof(CONVCONTEXT)   // See CLST_ flags
#define GWLP_SHINST         GWL_CONVSTATE + sizeof(LONG)
#define GWLP_CHINST         GWLP_SHINST + sizeof(HANDLE)

#define CLST_CONNECTED              0
#define CLST_SINGLE_INITIALIZING    1
#define CLST_MULT_INITIALIZING      2

// Window long offsets in server window     (szDDEMLSERVERCLASS)

#define GWLP_PSI            0

// Window long offsets in event window      (szDDEMLEVENTCLASS)

#define GWLP_PSII           0


/*
 * DrawFrame defines
 */
#define DF_SHIFT0           0x0000
#define DF_SHIFT1           0x0001
#define DF_SHIFT2           0x0002
#define DF_SHIFT3           0x0003
#define DF_PATCOPY          0x0000
#define DF_PATINVERT        0x0004
#define DF_SHIFTMASK (DF_SHIFT0 | DF_SHIFT1 | DF_SHIFT2 | DF_SHIFT3)
#define DF_ROPMASK   (DF_PATCOPY | DF_PATINVERT)
#define DF_HBRMASK   ~(DF_SHIFTMASK | DF_ROPMASK)

#define DF_SCROLLBAR        (COLOR_SCROLLBAR << 3)
#define DF_BACKGROUND       (COLOR_BACKGROUND << 3)
#define DF_ACTIVECAPTION    (COLOR_ACTIVECAPTION << 3)
#define DF_INACTIVECAPTION  (COLOR_INACTIVECAPTION << 3)
#define DF_MENU             (COLOR_MENU << 3)
#define DF_WINDOW           (COLOR_WINDOW << 3)
#define DF_WINDOWFRAME      (COLOR_WINDOWFRAME << 3)
#define DF_MENUTEXT         (COLOR_MENUTEXT << 3)
#define DF_WINDOWTEXT       (COLOR_WINDOWTEXT << 3)
#define DF_CAPTIONTEXT      (COLOR_CAPTIONTEXT << 3)
#define DF_ACTIVEBORDER     (COLOR_ACTIVEBORDER << 3)
#define DF_INACTIVEBORDER   (COLOR_INACTIVEBORDER << 3)
#define DF_APPWORKSPACE     (COLOR_APPWORKSPACE << 3)
#define DF_3DSHADOW         (COLOR_3DSHADOW << 3)
#define DF_3DFACE           (COLOR_3DFACE << 3)
#define DF_GRAY             (COLOR_MAX << 3)


/*
 * CreateWindowEx internal flags for dwExStyle
 */

#define WS_EX_MDICHILD      0x00000040L         // Internal
#define WS_EX_ANSICREATOR   0x80000000L         // Internal

/*
 * These flags are used in the internal version of NtUserFindWindowEx
 */
#define FW_BOTH 0
#define FW_16BIT 1
#define FW_32BIT 2

/*
 * Calculate the size of a field in a structure of type type.
 */
#define FIELD_SIZE(type, field)     (sizeof(((type *)0)->field))

#define FLASTKEY 0x80

/*
 * Special types we've fabricated for special thunks.
 */
typedef struct {
    POINT point1;
    POINT point2;
    POINT point3;
    POINT point4;
    POINT point5;
} POINT5, *LPPOINT5;

typedef struct {
    DWORD dwRecipients;
    DWORD dwFlags;
} BROADCASTSYSTEMMSGPARAMS, *LPBROADCASTSYSTEMMSGPARAMS;
/*
 * Server side address constants. When we want to call a server side proc,
 * we pass an index indentifying the function, rather than the server side
 * address itself. More robust.  The functions between WNDPROCSTART/END
 * have client side sutbs which map to this routines.
 *
 * Adding a new FNID (This is just what I figured out...so fix it if wrong or incomplete)
 * -Decide what range it should be in:
 *      FNID_WNDPROCSTART to FNID_WNDPROCEND: Server side proc with client
 *          stub
 *      FIND_CONTROLSTART to FNID_CONTROLEND: Client side controls with no
 *          server side proc
 *      After FNID_CONTROLEND: other, like server side only procs or client
 *          side only....
 * -Make sure to adjust FNID_*START and FNID_*END appropriately.
 * -If the ID is to be associated with a window class, and it is for all
 *      windows of the class, make sure that the InternalRegisterClassEx call
 *      receives the id as a parameter.
 * -If in FNID_WNDPROCSTART-END range, make the proper STOCID call in InitFunctionTables.
 * -Add proper FNID call in InitFunctionTables.
 * -If the class has a client side worker function (pcls->lpfnWorker) or you expect
 *   apps to send messages to it or call its window proc directly, define
 *   a message table in kernel\server.c and initialize it in InitMessageTables.
 * -If there is a client side for this proc, you probably need to add it to
 *   PFNCLIENT.
 * -Add the debug-only text description of this FNID to in gapszFNID in globals.c
 * -See if you need to modify aiClassWow in client\client.c
 * -Modify the gaFNIDtoICLS table in kernel\ntstubs.c
 */
#define FNID_START                  0x0000029A
#define FNID_WNDPROCSTART           0x0000029A

#define FNID_SCROLLBAR              0x0000029A      // xxxSBWndProc;
#define FNID_ICONTITLE              0x0000029B      // xxxDefWindowProc;
#define FNID_MENU                   0x0000029C      // xxxMenuWindowProc;
#define FNID_DESKTOP                0x0000029D      // xxxDesktopWndProc;
#define FNID_DEFWINDOWPROC          0x0000029E      // xxxDefWindowProc;

#define FNID_WNDPROCEND             0x0000029E      // see PatchThreadWindows
#define FNID_CONTROLSTART           0x0000029F

#define FNID_BUTTON                 0x0000029F      // No server side proc
#define FNID_COMBOBOX               0x000002A0      // No server side proc
#define FNID_COMBOLISTBOX           0x000002A1      // No server side proc
#define FNID_DIALOG                 0x000002A2      // No server side proc
#define FNID_EDIT                   0x000002A3      // No server side proc
#define FNID_LISTBOX                0x000002A4      // No server side proc
#define FNID_MDICLIENT              0x000002A5      // No server side proc
#define FNID_STATIC                 0x000002A6      // No server side proc

#define FNID_IME                    0x000002A7      // No server side proc
#define FNID_CONTROLEND             0x000002A7

#define FNID_HKINLPCWPEXSTRUCT      0x000002A8
#define FNID_HKINLPCWPRETEXSTRUCT   0x000002A9
#define FNID_DEFFRAMEPROC           0x000002AA      // No server side proc
#define FNID_DEFMDICHILDPROC        0x000002AB      // No server side proc
#define FNID_MB_DLGPROC             0x000002AC      // No server side proc
#define FNID_MDIACTIVATEDLGPROC     0x000002AD      // No server side proc
#define FNID_SENDMESSAGE            0x000002AE

#define FNID_SENDMESSAGEFF          0x000002AF
#define FNID_SENDMESSAGEEX          0x000002B0
#define FNID_CALLWINDOWPROC         0x000002B1
#define FNID_SENDMESSAGEBSM         0x000002B2
#define FNID_SWITCH                 0x000002B3      // Just used by GetTopMostInserAfter
#define FNID_TOOLTIP                0x000002B4
#define FNID_END                    0x000002B4

/*
 * The size of the server side function table is defined as a power of two
 * so a simple "and" operation can be used to determine if a function index
 * is legal or not. Unused entries in the table are fill with a routine that
 * catches invalid functions that have indices within range, but are not
 * implemented.
 */

#define FNID_ARRAY_SIZE             32

#if (FNID_END - FNID_START + 1) > FNID_ARRAY_SIZE
    #error"The size of the function array is greater than the allocated storage"
#endif // (FNID_END - FNID_START + 1) > FNID_ARRAY_SIZE

#define FNID_DDE_BIT                0x00002000    // Used by RegisterClassExWOW
#define FNID_CLEANEDUP_BIT          0x00004000
#define FNID_DELETED_BIT            0x00008000
#define FNID_STATUS_BITS            (FNID_CLEANEDUP_BIT | FNID_DELETED_BIT)

#define FNID(s)     (gpsi->mpFnidPfn[((DWORD)(s) - FNID_START) & (FNID_ARRAY_SIZE - 1)])
#define STOCID(s)   (gpsi->aStoCidPfn[(DWORD)((s) & ~FNID_STATUS_BITS) - FNID_START])
#define CBFNID(s)   (gpsi->mpFnid_serverCBWndProc[(DWORD)((s) & ~FNID_STATUS_BITS) - FNID_START])
#define GETFNID(pwnd)       ((pwnd)->fnid & ~FNID_STATUS_BITS)

#ifndef BUILD_WOW6432
typedef LRESULT (APIENTRY * WNDPROC_PWND)(PWND, UINT, WPARAM, LPARAM);
typedef LRESULT (APIENTRY * WNDPROC_PWNDEX)(PWND, UINT, WPARAM, LPARAM, ULONG_PTR);
#else
typedef KERNEL_PVOID WNDPROC_PWND;
typedef KERNEL_PVOID WNDPROC_PWNDEX;
#endif
typedef BOOL (APIENTRY * WNDENUMPROC_PWND)(PWND, LPARAM);
typedef VOID (APIENTRY * TIMERPROC_PWND)(PWND, UINT, UINT_PTR, LPARAM);

/*
 * Structure passed by client during process initialization that holds some
 * client-side callback addresses.
 */
typedef struct _PFNCLIENT {
    KPROC pfnScrollBarWndProc;       // and must be paired Unicode then ANSI
    KPROC pfnTitleWndProc;
    KPROC pfnMenuWndProc;
    KPROC pfnDesktopWndProc;
    KPROC pfnDefWindowProc;

// Below not in FNID_WNDPROCSTART FNID_WNDPROCEND range

    KPROC pfnButtonWndProc;
    KPROC pfnComboBoxWndProc;
    KPROC pfnComboListBoxProc;
    KPROC pfnDialogWndProc;
    KPROC pfnEditWndProc;
    KPROC pfnListBoxWndProc;
    KPROC pfnMDIClientWndProc;
    KPROC pfnStaticWndProc;
    KPROC pfnImeWndProc;

// Below not in FNID_CONTROLSTART FNID_CONTROLEND range

    KPROC pfnHkINLPCWPSTRUCT;    // client-side callback for hook thunks
    KPROC pfnHkINLPCWPRETSTRUCT; // client-side callback for hook thunks
    KPROC pfnDispatchHook;
    KPROC pfnDispatchMessage;
    KPROC pfnMB_DlgProc;
    KPROC pfnMDIActivateDlgProc;
} PFNCLIENT, *PPFNCLIENT;

typedef struct _PFNCLIENTWORKER {
    KPROC pfnButtonWndProc;
    KPROC pfnComboBoxWndProc;
    KPROC pfnComboListBoxProc;
    KPROC pfnDialogWndProc;
    KPROC pfnEditWndProc;
    KPROC pfnListBoxWndProc;
    KPROC pfnMDIClientWndProc;
    KPROC pfnStaticWndProc;
    KPROC pfnImeWndProc;
} PFNCLIENTWORKER, *PPFNCLIENTWORKER;

#ifdef BUILD_WOW6432

extern const PFNCLIENT   pfnClientA;
extern const PFNCLIENT   pfnClientW;
extern const PFNCLIENTWORKER   pfnClientWorker;

#define FNID_TO_CLIENT_PFNA_CLIENT(s) ((ULONG_PTR)(*(((KERNEL_ULONG_PTR *)&pfnClientA) + (s - FNID_START))))
#define FNID_TO_CLIENT_PFNW_CLIENT(s) ((ULONG_PTR)(*(((KERNEL_ULONG_PTR *)&pfnClientW) + (s - FNID_START))))
#define FNID_TO_CLIENT_PFNWORKER(s)   ((ULONG_PTR)(*(((KERNEL_ULONG_PTR *)&pfnClientWorker) + (s - FNID_CONTROLSTART))))

WNDPROC_PWND MapKernelClientFnToClientFn(WNDPROC_PWND lpfnWndProc);

#else

#define FNID_TO_CLIENT_PFNA_CLIENT FNID_TO_CLIENT_PFNA_KERNEL
#define FNID_TO_CLIENT_PFNW_CLIENT FNID_TO_CLIENT_PFNW_KERNEL
#define FNID_TO_CLIENT_PFNWORKER(s) (*(((KERNEL_ULONG_PTR *)&gpsi->apfnClientWorker) + (s - FNID_CONTROLSTART)))

#define MapKernelClientFnToClientFn(lpfnWndProc) (lpfnWndProc)

#endif

#define FNID_TO_CLIENT_PFNA_KERNEL(s) (*(((KERNEL_ULONG_PTR *)&gpsi->apfnClientA) + (s - FNID_START)))
#define FNID_TO_CLIENT_PFNW_KERNEL(s) (*(((KERNEL_ULONG_PTR *)&gpsi->apfnClientW) + (s - FNID_START)))

#define FNID_TO_CLIENT_PFNA FNID_TO_CLIENT_PFNA_KERNEL
#define FNID_TO_CLIENT_PFNW FNID_TO_CLIENT_PFNW_KERNEL

/*
 * Object types
 *
 * NOTE: Changing this table means changing hard-coded arrays that depend
 * on the index number (in security.c and in debug.c)
 */
#define TYPE_FREE           0           // must be zero!
#define TYPE_WINDOW         1           // in order of use for C code lookups
#define TYPE_MENU           2
#define TYPE_CURSOR         3
#define TYPE_SETWINDOWPOS   4
#define TYPE_HOOK           5
#define TYPE_CLIPDATA       6           // clipboard data
#define TYPE_CALLPROC       7
#define TYPE_ACCELTABLE     8
#define TYPE_DDEACCESS      9
#define TYPE_DDECONV        10
#define TYPE_DDEXACT        11          // DDE transaction tracking info.
#define TYPE_MONITOR        12
#define TYPE_KBDLAYOUT      13          // Keyboard Layout handle (HKL) object.
#define TYPE_KBDFILE        14          // Keyboard Layout file object.
#define TYPE_WINEVENTHOOK   15          // WinEvent hook (EVENTHOOK)
#define TYPE_TIMER          16
#define TYPE_INPUTCONTEXT   17          // Input Context info structure

#define TYPE_CTYPES         18          // Count of TYPEs; Must be LAST + 1

#define TYPE_GENERIC        255         // used for generic handle validation

/* OEM Bitmap Information Structure */
typedef struct tagOEMBITMAPINFO
{
    int     x;
    int     y;
    int     cx;
    int     cy;
} OEMBITMAPINFO, *POEMBITMAPINFO;

// For the following OBI_ defines :
//
// a  pushed   state bitmap should be at +1 from it's normal state bitmap
// an inactive state bitmap should be at +2 from it's normal state bitmap
// A small caption bitmap should be +2 from the normal bitmap

#define DOBI_NORMAL         0
#define DOBI_PUSHED         1
#define DOBI_HOT            2
#define DOBI_INACTIVE       3

#define DOBI_CHECK      1   // checkbox/radio/3state button states
#define DOBI_DOWN       2
#define DOBI_CHECKDOWN  3

#define DOBI_CAPON      0   // caption states
#define DOBI_CAPOFF     1

// shared bitmap mappings
#define DOBI_3STATE         8   // offset from checkbox to 3state
#define DOBI_MBAR OBI_CLOSE_MBAR    // offset to menu bar equivalent

#define OBI_CLOSE            0      // caption close button
#define OBI_CLOSE_D          1
#define OBI_CLOSE_H          2
#define OBI_CLOSE_I          3
#define OBI_REDUCE           4      // caption minimize button
#define OBI_REDUCE_D         5
#define OBI_REDUCE_H         6
#define OBI_REDUCE_I         7
#define OBI_RESTORE          8      // caption restore button
#define OBI_RESTORE_D        9
#define OBI_RESTORE_H       10
#define OBI_HELP            11
#define OBI_HELP_D          12
#define OBI_HELP_H          13
#define OBI_ZOOM            14      // caption maximize button
#define OBI_ZOOM_D          15
#define OBI_ZOOM_H          16
#define OBI_ZOOM_I          17
#define OBI_CLOSE_MBAR      18      // menu bar close button
#define OBI_CLOSE_MBAR_D    19
#define OBI_CLOSE_MBAR_H    20
#define OBI_CLOSE_MBAR_I    21
#define OBI_REDUCE_MBAR     22      // menu bar minimize button
#define OBI_REDUCE_MBAR_D   23
#define OBI_REDUCE_MBAR_H   24
#define OBI_REDUCE_MBAR_I   25
#define OBI_RESTORE_MBAR    26      // menu bar restore button
#define OBI_RESTORE_MBAR_D  27
#define OBI_RESTORE_MBAR_H  28
#define OBI_CAPCACHE1       29      // caption icon cache entry #1
#define OBI_CAPCACHE1_I     30
#define OBI_CAPCACHE2       31      // caption icon cache entry #2
#define OBI_CAPCACHE2_I     32
#define OBI_CAPCACHE3       33      // caption icon cache entry #3
#define OBI_CAPCACHE3_I     34
#define OBI_CAPCACHE4       35      // caption icon cache entry #4
#define OBI_CAPCACHE4_I     36
#define OBI_CAPCACHE5       37      // caption icon cache entry #5
#define OBI_CAPCACHE5_I     38
#define OBI_CAPBTNS         39      // caption buttons cache
#define OBI_CAPBTNS_I       40
#define OBI_CLOSE_PAL       41      // small caption close button
#define OBI_CLOSE_PAL_D     42
#define OBI_CLOSE_PAL_H     43
#define OBI_CLOSE_PAL_I     44
#define OBI_NCGRIP          45      // bottom/right size grip
#define OBI_UPARROW         46      // up scroll arrow
#define OBI_UPARROW_D       47
#define OBI_UPARROW_H       48
#define OBI_UPARROW_I       49
#define OBI_DNARROW         50      // down scroll arrow
#define OBI_DNARROW_D       51
#define OBI_DNARROW_H       52
#define OBI_DNARROW_I       53
#define OBI_RGARROW         54      // right scroll arrow
#define OBI_RGARROW_D       55
#define OBI_RGARROW_H       56
#define OBI_RGARROW_I       57
#define OBI_LFARROW         58      // left scroll arrow
#define OBI_LFARROW_D       59
#define OBI_LFARROW_H       60
#define OBI_LFARROW_I       61
#define OBI_MENUARROW       62      // menu hierarchy arrow
#define OBI_MENUCHECK       63      // menu check mark
#define OBI_MENUBULLET      64      // menu bullet mark
#define OBI_MENUARROWUP     65
#define OBI_MENUARROWUP_H   66
#define OBI_MENUARROWUP_I   67
#define OBI_MENUARROWDOWN   68
#define OBI_MENUARROWDOWN_H 69
#define OBI_MENUARROWDOWN_I 70
#define OBI_RADIOMASK       71      // radio button mask
#define OBI_CHECK           72      // check box
#define OBI_CHECK_C         73
#define OBI_CHECK_D         74
#define OBI_CHECK_CD        75
#define OBI_CHECK_CDI       76
#define OBI_RADIO           77      // radio button
#define OBI_RADIO_C         78
#define OBI_RADIO_D         79
#define OBI_RADIO_CD        80
#define OBI_RADIO_CDI       81
#define OBI_3STATE          82      // 3-state button
#define OBI_3STATE_C        83
#define OBI_3STATE_D        84
#define OBI_3STATE_CD       85
#define OBI_3STATE_CDI      86
#define OBI_POPUPFIRST      87      // System popupmenu bitmaps.
#define OBI_CLOSE_POPUP     87
#define OBI_RESTORE_POPUP   88
#define OBI_ZOOM_POPUP      89
#define OBI_REDUCE_POPUP    90
#define OBI_NCGRIP_L        91
#define OBI_MENUARROW_L     92
#define OBI_COUNT           93      // bitmap count

/*
 * One global instance of this structure is allocated into memory that is
 * mapped into all clients' address space.  Client-side functions will
 * read this data to avoid calling the server.
 */

#define NCHARS   256
#define NCTRLS   0x20

#define PUSIF_PALETTEDISPLAY            0x00000001  /* Is the display palettized? */
#define PUSIF_SNAPTO                    0x00000002  /* Is SnapTo enabled? */
#define PUSIF_COMBOBOXANIMATION         0x00000004  /* Must match UPBOOLMask(SPI_GETCOMBOBOXANIMATION) */
#define PUSIF_LISTBOXSMOOTHSCROLLING    0x00000008  /* Must match UPBOOLMask(SPI_GETLISTBOXSMOOTHSCROLLING) */
#define PUSIF_KEYBOARDCUES              0x00000020  /* Must match UPBOOLMask(SPI_GETKEYBOARDCUES) */

#define PUSIF_UIEFFECTS                 0x80000000  /* Must match UPBOOLMask(SPI_GETUIEFFECTS) */

#define TEST_PUSIF(f)               TEST_FLAG(gpsi->PUSIFlags, f)
#define TEST_BOOL_PUSIF(f)          TEST_BOOL_FLAG(gpsi->PUSIFlags, f)
#define SET_PUSIF(f)                SET_FLAG(gpsi->PUSIFlags, f)
#define CLEAR_PUSIF(f)              CLEAR_FLAG(gpsi->PUSIFlags, f)
#define SET_OR_CLEAR_PUSIF(f, fSet) SET_OR_CLEAR_FLAG(gpsi->PUSIFlags, f, fSet)
#define TOGGLE_PUSIF(f)             TOGGLE_FLAG(gpsi->PUSIFlags, f)

#define TEST_EffectPUSIF(f)  \
    ((gpsi->PUSIFlags & (f | PUSIF_UIEFFECTS)) == (f | PUSIF_UIEFFECTS))

/*
 * Some UI effects have an "inverted" disabled value (ie, disabled is TRUE)
 */
#define TEST_EffectInvertPUSIF(f) (TEST_PUSIF(f) || !TEST_PUSIF(PUSIF_UIEFFECTS))

#define TEST_KbdCuesPUSIF (!gpsi->bKeyboardPref                         \
                        && !TEST_EffectInvertPUSIF(PUSIF_KEYBOARDCUES)  \
                        && !(GetAppCompatFlags2(VER40) & GACF2_KCOFF))


typedef struct tagPERUSERSERVERINFO {
    /*
     * All of this information should be mapped to the server, but put in
     * the desktop section so it can vary from desktop to desktop.
     */

    int         aiSysMet[SM_CMETRICS];
    COLORREF    argbSystem[COLOR_MAX];
    KHBRUSH     ahbrSystem[COLOR_MAX];
    KHBRUSH     hbrGray;
    POINT       ptCursor;
    DWORD       dwLastRITEventTickCount;
    int         nEvents;

    int         gclBorder;              /* # of logical units in window frame */

    UINT        dtScroll;
    UINT        dtLBSearch;
    UINT        dtCaretBlink;
    UINT        ucWheelScrollLines;    /* # of lines to scroll when wheel is rolled */

    int         wMaxLeftOverlapChars;
    int         wMaxRightOverlapChars;

    /*
     * these are here to lose a thunk for GetDialogBaseUnits
     */
    int         cxSysFontChar;
    int         cySysFontChar;
    int         cxMsgFontChar;
    int         cyMsgFontChar;
    TEXTMETRIC  tmSysFont;

    /*
     * values to allow HasCaptionIcon to be in user32
     */
    KHICON      hIconSmWindows;
    KHICON      hIcoWindows;

    KHFONT      hCaptionFont;
    KHFONT      hMsgFont;

    /*
     * These are needed for various user-mode performance hacks.
     */
    DWORD       dwKeyCache;
    DWORD       dwAsyncKeyCache;
    DWORD       cCaptures;

    /*
     * Information about the current state of the display which needs to
     * be shared with the client side. The information here corresponds
     * to the display in gpDispInfo. Note that much of this information
     * is only for the primary monitor.
     */
    OEMBITMAPINFO oembmi[OBI_COUNT];  /* OEM bitmap information */
    RECT          rcScreen;           /* rectangle of the virtual screen */
    WORD          BitCount;           /* Planes * Depth */
    WORD          dmLogPixels;        /* logical pixels per inch, both X and Y */
    BYTE          Planes;             /* Planes */
    BYTE          BitsPixel;          /* Depth */

    DWORD         PUSIFlags;          // PUSIF_ flags
    UINT          uCaretWidth;        /* caret width in edits */
    LANGID        UILangID;           // Default UI language

    BOOL        bLastRITWasKeyboard : 1;
    BOOL        bKeyboardPref : 1;    /* To propagate ACCF_KEYBOARDPREF to client side */


} PERUSERSERVERINFO, *PPERUSERSERVERINFO;

#define SRVIF_CHECKED                   0x0001
#define SRVIF_WINEVENTHOOKS             0x0002
#define SRVIF_DBCS                      0x0004
#define SRVIF_IME                       0x0008
#define SRVIF_MIDEAST                   0x0010

#define TEST_SRVIF(f)                   TEST_FLAG(gpsi->wSRVIFlags, f)
#define TEST_BOOL_SRVIF(f)              TEST_BOOL_FLAG(gpsi->wSRVIFlags, f)
#define SET_SRVIF(f)                    SET_FLAG(gpsi->wSRVIFlags, f)
#define CLEAR_SRVIF(f)                  CLEAR_FLAG(gpsi->wSRVIFlags, f)
#define SET_OR_CLEAR_SRVIF(f, fSet)     SET_OR_CLEAR_FLAG(gpsi->wSRVIFlags, f, fSet)
#define TOGGLE_SRVIF(f)                 TOGGLE_FLAG(gpsi->wSRVIFlags, f)

typedef struct tagSERVERINFO {      // si
    WORD    wRIPFlags;              // RIPF_ flags
    WORD    wSRVIFlags;             // SRVIF_ flags
    WORD    wRIPPID;                // PID of process to apply RIP flags to (zero means all)
    WORD    wRIPError;              // Error to break on (zero means all errors are treated equal)

    KERNEL_ULONG_PTR cHandleEntries;    // count of handle entries in array

    /*
     * Array of server-side function pointers.
     * Client passes servers function ID so they can be easily validated;
     * this array maps function ID into server-side function address.
     * The order of these are enforced by the FNID_ constants, and must match
     * the client-side mpFnidClientPfn[] order as well.
     */
    WNDPROC_PWNDEX mpFnidPfn[FNID_ARRAY_SIZE]; // function mapping table
    WNDPROC_PWND aStoCidPfn[(FNID_WNDPROCEND - FNID_START) + 1];

    // mapping of fnid to min bytes need by public windproc user
    WORD mpFnid_serverCBWndProc[(FNID_END - FNID_START) + 1];

    /*
     * Client side functions pointer structure.
     */
    struct _PFNCLIENT apfnClientA;
    struct _PFNCLIENT apfnClientW;
    struct _PFNCLIENTWORKER apfnClientWorker;

    DWORD cbHandleTable;

    /*
     * Class atoms to allow fast checks on the client.
     */
    ATOM atomSysClass[ICLS_MAX];   // Atoms for control classes

    DWORD dwDefaultHeapBase;            // so WOW can do handle validation
    DWORD dwDefaultHeapSize;

    UINT uiShellMsg;         // message for shell hooks

    UINT  wMaxBtnSize;   /* Size of the longest button string in any MessageBox */

    MBSTRING MBStrings[MAX_MB_STRINGS];

    /*
     * values to allow HasCaptionIcon to be in user32
     */
    ATOM atomIconSmProp;
    ATOM atomIconProp;

    ATOM atomContextHelpIdProp;

    char acOemToAnsi[NCHARS];
    char acAnsiToOem[NCHARS];

    /*
     * Per user settings. We use _HYDRA_'s PERUSERSERVERINO struct
     * to avoid defining fields in two places.
     */
    PERUSERSERVERINFO;

#if DEBUGTAGS
    DWORD adwDBGTAGFlags[DBGTAG_Max + 1];
#endif // DEBUGTAGS

} SERVERINFO;

/*
 * Quick test for any Window Event Hooks.
 */
#ifdef _USERK_
#define FWINABLE() gpWinEventHooks
#else
#define FWINABLE() TEST_SRVIF(SRVIF_WINEVENTHOOKS)
#endif

/* MessageBox String pointers from offset in the gpsi struct */
#define GETGPSIMBPSTR(u) (LPWSTR) (gpsi->MBStrings[(u)].szName)

typedef struct _WNDMSG {
    UINT maxMsgs;
    KPBYTE abMsgs;
} WNDMSG, *PWNDMSG;

typedef struct tagSHAREDINFO {
    PSERVERINFO     psi;
    PHE             aheList;         /* handle table pointer                */
    PDISPLAYINFO    pDispInfo;       /* global displayinfo                  */
    KERNEL_UINT_PTR ulSharedDelta;   /* delta between client and kernel mapping of ...*/
                                     /* shared memory section. Only valid/used in client.*/

    WNDMSG          awmControl[FNID_END - FNID_START + 1];

    WNDMSG          DefWindowMsgs;
    WNDMSG          DefWindowSpecMsgs;
} SHAREDINFO, *PSHAREDINFO;

typedef struct _USERCONNECT {
    IN  ULONG ulVersion;
    OUT ULONG ulCurrentVersion;
    IN  DWORD dwDispatchCount;
    OUT SHAREDINFO siClient;
} USERCONNECT, *PUSERCONNECT;

#define USER_MAJOR_VERSION  0x0005
#define USER_MINOR_VERSION  0x0000

#define USERCURRENTVERSION   MAKELONG(USER_MINOR_VERSION, USER_MAJOR_VERSION)

/*
 * Options used for NtUserSetSysColors
 */
#define SSCF_NOTIFY             0x00000001
#define SSCF_FORCESOLIDCOLOR    0x00000002
#define SSCF_SETMAGICCOLORS     0x00000004
#define SSCF_16COLORS           0x00000008

/*
 * Structure used for GetClipboardData, where we can have
 * extra information returned from the kernel.
 */
typedef struct tagGETCLIPBDATA {

    UINT   uFmtRet;          // Identifies returned format.
    BOOL   fGlobalHandle;    // Indicates if handle is global.
    union {
        HANDLE hLocale;      // Locale (text-type formats only).
        HANDLE hPalette;     // Palette (bitmap-type formats only).
    };

} GETCLIPBDATA, *PGETCLIPBDATA;

/*
 * Structure used for SetClipboardData, where we can have
 * extra information passed to the kernel.
 */
typedef struct tagSETCLIPBDATA {

    BOOL fGlobalHandle;      // Indicates if handle is global.
    BOOL fIncSerialNumber;   // Indicates if we should increment serial#

} SETCLIPBDATA, *PSETCLIPBDATA;

/*
 * HM Object definition control flags
 */
#define OCF_THREADOWNED         0x01
#define OCF_PROCESSOWNED        0x02
#define OCF_MARKPROCESS         0x04
#define OCF_USEPOOLQUOTA        0x08
#define OCF_DESKTOPHEAP         0x10
#define OCF_USEPOOLIFNODESKTOP  0x20
#define OCF_SHAREDHEAP          0x40
#if DBG
#define OCF_VARIABLESIZE        0x80
#else
#define OCF_VARIABLESIZE        0
#endif

/*
 * From HANDTABL.C
 */
/*
 * Static information about each handle type.
 */
typedef void (*FnDestroyUserObject)(void *);

typedef struct tagHANDLETYPEINFO {
#if DBG
    LPCSTR              szObjectType;
    UINT                uSize;
#endif
    FnDestroyUserObject fnDestroy;
    DWORD               dwAllocTag;
    BYTE                bObjectCreateFlags;
} HANDLETYPEINFO, *PHANDLETYPEINFO;

/*
 * The following is the header of all objects managed in the handle list.
 * (allocated as part of the object for easy access).  All object
 * headers must start with the members of a HEAD structure.
 */
typedef struct _HEAD {
    KHANDLE h;
    DWORD   cLockObj;
} HEAD, *PHEAD;

/*
 * sizeof(THROBJHEAD) must be equal to sizeof(PROCOBJHEAD)
 * This is to make sure that DESKHEAD fields are always at the same offset.
 */
typedef struct _THROBJHEAD {
    HEAD;
    PTHREADINFO pti;
} THROBJHEAD, *PTHROBJHEAD;

typedef struct _PROCOBJHEAD {
    HEAD;
    DWORD hTaskWow;
} PROCOBJHEAD, *PPROCOBJHEAD;

typedef struct _PROCMARKHEAD {
    PROCOBJHEAD;
    PPROCESSINFO ppi;
} PROCMARKHEAD, *PPROCMARKHEAD;

typedef struct _DESKHEAD {
    PDESKTOP rpdesk;
    KPBYTE   pSelf;
} DESKHEAD, *PDESKHEAD;

/*
 * This type is for HM casting only. Use THRDESKHEAD or PROCDESKHEAD instead.
 */
typedef struct _DESKOBJHEAD {
    HEAD;
    KERNEL_PVOID pOwner;
    DESKHEAD;
} DESKOBJHEAD, *PDESKOBJHEAD;

typedef struct _THRDESKHEAD {
    THROBJHEAD;
    DESKHEAD;
} THRDESKHEAD, *PTHRDESKHEAD;

typedef struct _PROCDESKHEAD {
    PROCOBJHEAD;
    DESKHEAD;
} PROCDESKHEAD, *PPROCDESKHEAD;



#define HANDLEF_DESTROY        0x01
#define HANDLEF_INDESTROY      0x02
#define HANDLEF_MARKED_OK      0x10
#define HANDLEF_GRANTED        0x20
#define HANDLEF_POOL           0x40     // for the mother desktop window
#define HANDLEF_VALID          0x7F

/*
 * The following is a handle table entry.
 *
 * Note that by keeping a pointer to the owning entity (process or
 * thread), cleanup will touch only those objects that belong to
 * the entity being destroyed.  This helps keep the working set
 * size down.  Look at DestroyProcessesObjects() for an example.
 */
typedef struct _HANDLEENTRY {
    PHEAD       phead;                  /* pointer to the real object */
    KERNEL_PVOID pOwner;                 /* pointer to owning entity (pti or ppi) */
    BYTE        bType;                  /* type of object */
    BYTE        bFlags;                 /* flags - like destroy flag */
    WORD        wUniq;                  /* uniqueness count */

#if DBG
    PLR         plr;                    /* lock record pointer */
#endif // DBG

} HANDLEENTRY;

/*
 * Change HMINDEXBITS for bits that make up table index in handle
 * Change HMUNIQSHIFT for count of bits to shift uniqueness left.
 * Change HMUNIQBITS for bits that make up uniqueness.
 *
 * Currently 64K handles can be created, w/16 bits of uniqueness.
 */
#define HMINDEXBITS             0x0000FFFF      // bits where index is stored
#define HMUNIQSHIFT             16              // bits to shift uniqueness
#define HMUNIQBITS              0xFFFF          // valid uniqueness bits

#ifdef _USERK_
#define HMHandleFromIndex(i)    LongToHandle((LONG)(i) | ((LONG)gSharedInfo.aheList[i].wUniq << HMUNIQSHIFT))
#define HMObjectFlags(p)        (gahti[HMObjectType(p)].bObjectCreateFlags)
#endif

#define HMIndexFromHandle(h)    ((ULONG)(((ULONG_PTR)(h)) & HMINDEXBITS))
#define _HMPheFromObject(p)      (&gSharedInfo.aheList[HMIndexFromHandle((((PHEAD)p)->h))])
#define _HMObjectFromHandle(h)  ((KERNEL_PVOID)(gSharedInfo.aheList[HMIndexFromHandle(h)].phead))
#define HMUniqFromHandle(h)     ((WORD)((((ULONG_PTR)h) >> HMUNIQSHIFT) & HMUNIQBITS))
#define HMObjectType(p)         (HMPheFromObject(p)->bType)

#define HMIsMarkDestroy(p)      (HMPheFromObject(p)->bFlags & HANDLEF_DESTROY)

/*
 * Validation, handle mapping, etc.
 */
#define HMRevalidateHandle(h)       HMValidateHandleNoSecure(h, TYPE_GENERIC)
#define HMRevalidateCatHandle(h)    HMValidateCatHandleNoSecure(h, TYPE_GENERIC)

#define HMRevalidateHandleNoRip(h)  HMValidateHandleNoRip(h, TYPE_GENERIC)
#define RevalidateHmenu(hmenuX)     HMValidateHandleNoRip(hmenuX, TYPE_MENU)

#define _PtoHq(p)       ((HANDLE)(((PHEAD)p)->h))
#define _PtoH(p)        ((HANDLE)((p) == NULL ? NULL : _PtoHq(p)))
#define _HW(pwnd)       ((HWND)_PtoH(pwnd))
#define _HWCCX(ccxPwnd) ((HWND)_PtoH(ccxPwnd))
#define _HWq(pwnd)      ((HWND)_PtoHq(pwnd))

#if DBG && defined(_USERK_)

PHE DBGHMPheFromObject (PVOID p);
PVOID DBGHMObjectFromHandle (HANDLE h);
PVOID DBGHMCatObjectFromHandle (HANDLE h);
HANDLE DBGPtoH (PVOID p);
HANDLE DBGPtoHq (PVOID p);
HWND DBGHW (PWND pwnd);
HWND DBGHWCCX (PWND pwnd);
HWND DBGHWq (PWND pwnd);

#define HMPheFromObject(p)      DBGHMPheFromObject((p))
#define HMObjectFromHandle(h)   DBGHMObjectFromHandle((HANDLE)(h))
#define HMCatObjectFromHandle(h) DBGHMCatObjectFromHandle((HANDLE)(h))
#define PtoH(p)                 DBGPtoH((PVOID)(p))
#define PtoHq(p)                DBGPtoHq((PVOID)(p))
#define HW(pwnd)                DBGHW((PWND)(pwnd))
#define HWCCX(ccxPwnd)          DBGHWCCX((PWND)(ccxPwnd))
#define HWq(pwnd)               DBGHWq((PWND)(pwnd))

#else

#define HMPheFromObject(p)      _HMPheFromObject(p)
#define HMObjectFromHandle(h)   _HMObjectFromHandle(h)
#define HMCatObjectFromHandle(h) _HMObjectFromHandle(h)
#define PtoH(p)                 _PtoH(p)
#define PtoHq(p)                _PtoHq(p)
#define HW(pwnd)                _HW(pwnd)
#define HWCCX(ccxPwnd)          _HW(ccxPwnd)
#define HWq(pwnd)               _HWq(pwnd)

#endif /* #else #if DBG && defined(_USERK_) */

/*
 * Inline functions / macros to access HM object head fields
 */
#define _GETPTI(p)      (((PTHROBJHEAD)p)->pti)
#define _GETPDESK(p)    (((PDESKOBJHEAD)p)->rpdesk)
#define _GETPPI(p)      (((PPROCMARKHEAD)p)->ppi)

#if DBG && defined(_USERK_)
extern CONST HANDLETYPEINFO gahti[];
extern SHAREDINFO gSharedInfo;
__inline PTHREADINFO GETPTI (PVOID p)
{
    UserAssert(HMObjectFlags(p) & OCF_THREADOWNED);
    return _GETPTI(p);
}
__inline PDESKTOP GETPDESK (PVOID p)
{
    UserAssert(HMObjectFlags(p) & OCF_DESKTOPHEAP);
    return _GETPDESK(p);
}
__inline PPROCESSINFO GETPPI (PVOID p)
{
    UserAssert(HMObjectFlags(p) & OCF_MARKPROCESS);
    return _GETPPI(p);
}

#else

#define GETPTI(p)       _GETPTI(p)
#define GETPDESK(p)     _GETPDESK(p)
#define GETPPI(p)       _GETPPI(p)

#endif /* #else #if DBG && defined(_USERK_) */

#define GETPWNDPPI(p) (GETPTI(p)->ppi)


/*
 * NOTE!: there is code in exitwin.c that assumes HMIsMarkDestroy is defined as
 *      (HMPheFromObject(p)->bFlags & HANDLEF_DESTROY)
 */

#define CPD_ANSI_TO_UNICODE     0x0001      /* CPD represents ansi to U transition */
#define CPD_UNICODE_TO_ANSI     0x0002
#define CPD_TRANSITION_TYPES    (CPD_ANSI_TO_UNICODE|CPD_UNICODE_TO_ANSI)

#define CPD_CLASS               0x0010      /* Get CPD for a class */
#define CPD_WND                 0x0020
#define CPD_DIALOG              0x0040
#define CPD_WNDTOCLS            0x0080

#define CPDHANDLE_HI            ((ULONG_PTR)~HMINDEXBITS)
#define MAKE_CPDHANDLE(h)       (HMIndexFromHandle(h) | CPDHANDLE_HI)
#define ISCPDTAG(x)             (((ULONG_PTR)(x) & CPDHANDLE_HI) == CPDHANDLE_HI)

/*
 * Call Proc Handle Info
 */
typedef struct _CALLPROCDATA {
    PROCDESKHEAD                 head;
    PCALLPROCDATA                spcpdNext;
    KERNEL_ULONG_PTR             pfnClientPrevious;
    WORD                         wType;
} CALLPROCDATA;

/*
 * Class styles
 */
#define CFVREDRAW         0x0001
#define CFHREDRAW         0x0002
#define CFKANJIWINDOW     0x0004
#define CFDBLCLKS         0x0008
#define CFSERVERSIDEPROC  0x0010    // documented as reserved in winuser.h
#define CFOWNDC           0x0020
#define CFCLASSDC         0x0040
#define CFPARENTDC        0x0080
#define CFNOKEYCVT        0x0101
#define CFNOCLOSE         0x0102
#define CFLVB             0x0104
#define CFSAVEBITS        0x0108
#define CFOEMCHARS        0x0140
#define CFIME             0x0201

/*
 * Offset from the beginning of the CLS structure to the WNDCLASS section.
 */
#define CFOFFSET             (FIELD_OFFSET(CLS, style))

#define TestCF(hwnd, flag)   (*((BYTE *)((PWND)(hwnd))->pcls + CFOFFSET + HIBYTE(flag)) & LOBYTE(flag))
#define SetCF(hwnd, flag)    (*((BYTE *)((PWND)(hwnd))->pcls + CFOFFSET + HIBYTE(flag)) |= LOBYTE(flag))
#define ClrCF(pcls, flag)    (*((BYTE *)((PWND)(hwnd))->pcls + CFOFFSET + HIBYTE(flag)) &= ~LOBYTE(flag))

#define TestCF2(pcls, flag)  (*((BYTE *)(pcls) + CFOFFSET + (int)HIBYTE(flag)) & LOBYTE(flag))
#define SetCF2(pcls, flag)   (*((BYTE *)(pcls) + CFOFFSET + (int)HIBYTE(flag)) |= LOBYTE(flag))
#define ClrCF2(pcls, flag)   (*((BYTE *)(pcls) + CFOFFSET + (int)HIBYTE(flag)) &= ~LOBYTE(flag))

#define PWCFromPCLS(pcls)  ((PWC)((PBYTE)(pcls) + sizeof(CLS) + (pcls)->cbclsExtra))

/* Window class structure */
typedef struct tagCOMMON_WNDCLASS
{
    /*
     * We'll add cWndReferenceCount here so COMMON_WNDCLASS and WNDCLASSEX have
     * the same layout. Otherwise padding will screw us up on 64-bit platforms.
     */
    int           cWndReferenceCount; /* The number of windows registered
                                         with this class */
    UINT          style;
    WNDPROC_PWND  lpfnWndProc;       // HI BIT on means WOW PROC
    int           cbclsExtra;
    int           cbwndExtra;
    KHANDLE       hModule;
    PCURSOR       spicn;
    PCURSOR       spcur;
    KHBRUSH       hbrBackground;
    KLPWSTR       lpszMenuName;
    KLPSTR        lpszAnsiClassName;
    PCURSOR       spicnSm;
} COMMON_WNDCLASS;
/*
 * Class Menu names structure. For performance reasons (GetClassInfo)
 *  we keep two client side copies of wndcls.lpszMenu and another kernel side copy.
 * This structure is used to pass menu names info between client and kernel.
 */
typedef struct tagCLSMENUNAME
{
    KLPSTR              pszClientAnsiMenuName;
    KLPWSTR             pwszClientUnicodeMenuName;
    PUNICODE_STRING     pusMenuName;
} CLSMENUNAME, *PCLSMENUNAME;

/*
 * This is the window class structure.  All window classes are linked
 * together in a master list pointed to by pclsList.
 *
 * RED ALERT! Do not add any fields after the COMMON_WNDCLASS structure;
 *            CFOFFSET depends on this.
 */

typedef struct tagCLS {
    /* NOTE: The order of the following fields is assumed. */
    PCLS                        pclsNext;
    ATOM                        atomClassName;
    WORD                        fnid;               // record window proc used by this hwnd
                                                    // access through GETFNID
    PDESKTOP                    rpdeskParent;/* Parent desktop */
    PDCE                        pdce;            /* PDCE to DC associated with class */
    WORD                        hTaskWow;
    WORD                        CSF_flags;           /* internal class flags */
    KLPSTR                      lpszClientAnsiMenuName;     /* string or resource ID */
    KLPWSTR                     lpszClientUnicodeMenuName;  /* string or resource ID */

    PCALLPROCDATA               spcpdFirst;       /* Pointer to first CallProcData element (or 0) */
    PCLS                        pclsBase;        /* Pointer to base class */
    PCLS                        pclsClone;       /* Pointer to clone class list */

    COMMON_WNDCLASS;
    /*
     * WARNING:
     * CFOFFSET expects COMMON_WNDCLASS to be last fields in CLS
     */
} CLS, **PPCLS;

/*
 * This class flag is used to distinguish classes that were registered
 * by the server (most system classes) from those registered by the client.
 * Note -- flags are a WORD in the class structure now.
 */
#define CSF_SERVERSIDEPROC      0x0001
#define CSF_ANSIPROC            0x0002
#define CSF_WOWDEFERDESTROY     0x0004
#define CSF_SYSTEMCLASS         0x0008
#define CSF_WOWCLASS            0x0010  // extra words at end for wow info
#define CSF_WOWEXTRA            0x0020
#define CSF_CACHEDSMICON        0x0040
#define CSF_WIN40COMPAT         0x0080
#define CSF_VALID               (CSF_ANSIPROC | CSF_WIN40COMPAT)

/*
 * SBDATA are the values for one scrollbar
 */

typedef struct tagSBDATA {
    int    posMin;
    int    posMax;
    int    page;
    int    pos;
} SBDATA, *PSBDATA;

/*
 * SBINFO is the set of values that hang off of a window structure, if the
 * window has scrollbars.
 */
typedef struct tagSBINFO {
    int WSBflags;
    SBDATA Horz;
    SBDATA Vert;
} SBINFO, * KPTR_MODIFIER PSBINFO;

/*
 * Window Property structure
 */
typedef struct tagPROP {
    KHANDLE hData;
    ATOM atomKey;
    WORD fs;
} PROP, *PPROP;

#define PROPF_INTERNAL   0x0001
#define PROPF_STRING     0x0002
#define PROPF_NOPOOL     0x0004


/*
 * Window Property List structure
 */
typedef struct tagPROPLIST {
    UINT cEntries;
    UINT iFirstFree;
    PROP aprop[1];
} PROPLIST, * KPTR_MODIFIER PPROPLIST;

/*
 * NOTE -- this structure has been sorted (roughly) in order of use
 * of the fields.   The x86 code set allows cheaper access to fields
 * that are in the first 0x80 bytes of a structure.  Please attempt
 * to ensure that frequently-used fields are below this boundary.
 *          FritzS
 */

typedef struct tagWND {          // wnd
    THRDESKHEAD   head;

    WW;         // WOW-USER common fields. Defined in wowuserp.h
                //  The presence of "state" at the start of this structure is assumed
                //  by the STATEOFFSET macro.

    PWND                 spwndNext;    // Handle to the next window
    PWND                 spwndParent;  // Backpointer to the parent window.
    PWND                 spwndChild;   // Handle to child
    PWND                 spwndOwner;   // Popup window owner field

    RECT                 rcWindow;     // Window outer rectangle
    RECT                 rcClient;     // Client rectangle

    WNDPROC_PWND         lpfnWndProc;   // Can be WOW address or standard address

    PCLS                 pcls;         // Pointer to window class

    KHRGN                hrgnUpdate;   // Accumulated paint region

    PPROPLIST            ppropList;    // Pointer to property list
    PSBINFO              pSBInfo;   // Words used for scrolling

    PMENU                spmenuSys;  // Handle to system menu
    PMENU                spmenu;     // Menu handle or ID

    KHRGN                hrgnClip;     // Clipping region for this window

    LARGE_UNICODE_STRING strName;
    int                  cbwndExtra;   // Extra bytes in window
    PWND                 spwndLastActive; // Last active in owner/ownee list
    KHIMC                hImc;         // Associated input context handle
    KERNEL_ULONG_PTR     dwUserData;   // Reserved for random application data
} WND;

#define NEEDSPAINT(pwnd)    (pwnd->hrgnUpdate != NULL || TestWF(pwnd, WFINTERNALPAINT))

/*
 * Combo Box stuff
 */
typedef struct tagCBox {
    PWND    spwnd;      /* Window for the combo box */
    PWND    spwndParent;/* Parent of the combo box */
    RECT    editrc;            /* Rectangle for the edit control/static text
                                  area */
    RECT    buttonrc;          /* Rectangle where the dropdown button is */

    int     cxCombo;            // Width of sunken area
    int     cyCombo;            // Height of sunken area
    int     cxDrop;             // 0x24 Width of dropdown
    int     cyDrop;             // Height of dropdown or shebang if simple

    PWND    spwndEdit;  /* Edit control window handle */
    PWND    spwndList;  /* List box control window handle */

    UINT    CBoxStyle:2;         /* Combo box style */
    UINT    fFocus:1;          /* Combo box has focus? */
    UINT    fNoRedraw:1;       /* Stop drawing? */
    UINT    fMouseDown:1;      /* Was the popdown button just clicked and
                                   mouse still down? */
    UINT    fButtonPressed:1; /* Is the dropdown button in an inverted state?
                                */
    UINT    fLBoxVisible:1;    /* Is list box visible? (dropped down?) */
    UINT    OwnerDraw:2;       /* Owner draw combo box if nonzero. value
                                * specifies either fixed or varheight
                                */
    UINT    fKeyboardSelInListBox:1; /* Is the user keyboarding through the
                                      * listbox. So that we don't hide the
                                      * listbox on selchanges caused by the
                                      * user keyboard through it but we do
                                      * hide it if the mouse causes the
                                      * selchange.
                                      */
    UINT    fExtendedUI:1;     /* Are we doing TandyT's UI changes on this
                                * combo box?
                                */
    UINT    fCase:2;

    UINT    f3DCombo:1;         // 3D or flat border?
    UINT    fNoEdit:1;         /* True if editing is not allowed in the edit
                                * window.
                                */
#ifdef COLOR_HOTTRACKING
    UINT    fButtonHotTracked:1; /* Is the dropdown hot-tracked? */
#endif // COLOR_HOTTRACKING
    UINT    fRightAlign:1;     /* used primarily for MidEast right align */
    UINT    fRtoLReading:1;    /* used only for MidEast, text rtol reading order */
    HANDLE  hFont;             /* Font for the combo box */
    LONG    styleSave;         /* Temp to save the style bits when creating
                                * window.  Needed because we strip off some
                                * bits and pass them on to the listbox or
                                * edit box.
                                */
} CBOX, * KPTR_MODIFIER PCBOX;

typedef struct tagCOMBOWND {
    WND wnd;
    PCBOX pcbox;
} COMBOWND, *PCOMBOWND;

/*
 * List Box
 */
typedef struct _SCROLLPOS {
    INT cItems;
    UINT iPage;
    INT iPos;
    UINT fMask;
    INT iReturn;
} SCROLLPOS, *PSCROLLPOS;

typedef struct tagLBIV {
    PWND    spwndParent;    /* parent window */
    PWND    spwnd;          /* lbox ctl window */
    INT     iTop;           /* index of top item displayed          */
    INT     iSel;           /* index of current item selected       */
    INT     iSelBase;       /* base sel for multiple selections     */
    INT     cItemFullMax;   /* cnt of Fully Visible items. Always contains
                               result of CItemInWindow(plb, FALSE) for fixed
                               height listboxes. Contains 1 for var height
                               listboxes. */
    INT     cMac;           /* cnt of items in listbox              */
    INT     cMax;           /* cnt of total # items allocated for rgpch.
                               Not all are necessarly in use    */
    PBYTE   rgpch;          /* pointer to array of string offsets    */
    LPWSTR  hStrings;       /* string storage handle                */
    INT     cchStrings;     /* Size in bytes of hStrings            */
    INT     ichAlloc;       /* Pointer to end of hStrings (end of last valid
                               string) */
    INT     cxChar;         /* Width of a character                 */
    INT     cyChar;         /* height of line                       */
    INT     cxColumn;       /* width of a column in multicolumn listboxes */
    INT     itemsPerColumn; /* for multicolumn listboxes */
    INT     numberOfColumns; /* for multicolumn listboxes */
    POINT   ptPrev;         /* coord of last tracked mouse pt. used for auto
                               scrolling the listbox during timer's */

    UINT    OwnerDraw:2;      /* Owner draw styles. Non-zero if ownerdraw. */
    UINT     fRedraw:1;      /* if TRUE then do repaints             */
    UINT     fDeferUpdate:1; /* */
    UINT    wMultiple:2;      /* SINGLESEL allows a single item to be selected.
                             * MULTIPLESEL allows simple toggle multi-selection
                             * EXTENDEDSEL allows extended multi selection;
                             */

    UINT     fSort:1;        /* if TRUE the sort list                */
    UINT     fNotify:1;      /* if TRUE then Notify parent           */
    UINT     fMouseDown:1;   /* if TRUE then process mouse moves/mouseup */
    UINT     fCaptured:1;    // if TRUE then process mouse messages
    UINT     fCaret:1;       /* flashing caret allowed               */
    UINT     fDoubleClick:1; /* mouse down in double click           */
    UINT     fCaretOn:1;     /* if TRUE then caret is on             */
    UINT     fAddSelMode:1;  /* if TRUE, then it is in ADD selection mode */
    UINT     fHasStrings:1;  /* True if the listbox has a string associated
                             * with each item else it has an app suppled LONG
                             * value and is ownerdraw
                             */
    UINT     fHasData:1;    /* if FALSE, then lb doesn't keep any line data
                             * beyond selection state, but instead calls back
                             * to the client for each line's definition.
                             * Forces OwnerDraw==OWNERDRAWFIXED, !fSort,
                             * and !fHasStrings.
                             */
    UINT     fNewItemState:1; /* select/deselect mode? for multiselection lb
                              */
    UINT     fUseTabStops:1; /* True if the non-ownerdraw listbox should handle
                             * tabstops
                             */
    UINT     fMultiColumn:1; /* True if this is a multicolumn listbox */
    UINT     fNoIntegralHeight:1; /* True if we don't want to size the listbox
                                  * an integral lineheight
                                  */
    UINT     fWantKeyboardInput:1; /* True if we should pass on WM_KEY & CHAR
                                   * so that the app can go to special items
                                   * with them.
                                   */
    UINT     fDisableNoScroll:1;   /* True if the listbox should
                                    * automatically Enable/disable
                                    * it's scroll bars. If false, the scroll
                                    * bars will be hidden/Shown automatically
                                    * if they are present.
                                    */
    UINT    fHorzBar:1; // TRUE if WS_HSCROLL specified at create time

    UINT    fVertBar:1; // TRUE if WS_VSCROLL specified at create time
    UINT    fFromInsert:1;  // TRUE if client drawing should be deferred during delete/insert ops
    UINT    fNoSel:1;

    UINT    fHorzInitialized : 1;   // Horz scroll cache initialized
    UINT    fVertInitialized : 1;   // Vert scroll cache initialized

    UINT    fSized : 1;             // Listbox was resized.
    UINT    fIgnoreSizeMsg : 1;     // If TRUE, ignore WM_SIZE message

    UINT    fInitialized : 1;

    UINT    fRightAlign:1;     // used primarily for MidEast right align
    UINT    fRtoLReading:1;    // used only for MidEast, text rtol reading order
    UINT    fSmoothScroll:1;   // allow just one smooth-scroll per scroll cycle

    int     xRightOrigin;      // For horizontal scrolling. The current x origin

    INT     iLastSelection; /* Used for cancelable selection. Last selection
                             * in listbox for combo box support
                             */
    INT     iMouseDown;     /* For multiselection mouse click & drag extended
                             * selection. It is the ANCHOR point for range
                             * selections
                             */
    INT     iLastMouseMove; /* selection of listbox items */
    /*
     * IanJa/Win32: Tab positions remain int for 32-bit API ??
     */
    LPINT   iTabPixelPositions; /* List of positions for tabs */
    HANDLE  hFont;          /* User settable font for listboxes */
    int     xOrigin;        /* For horizontal scrolling. The current x origin */
    int     maxWidth;       /* Maximum width of listbox in pixels for
                               horizontal scrolling purposes */
    PCBOX   pcbox;          /* Combo box pointer */
    HDC     hdc;            /* hdc currently in use */
    DWORD   dwLocaleId;     /* Locale used for sorting strings in list box */
    int     iTypeSearch;
    LPWSTR  pszTypeSearch;
    SCROLLPOS HPos;
    SCROLLPOS VPos;
} LBIV, *PLBIV;

typedef struct tagLBWND {
    WND wnd;
    PLBIV pLBIV;
} LBWND, *PLBWND;

/*
 * kernel side input context structure.
 */
typedef struct tagIMC {    /* hImc */
    THRDESKHEAD     head;
    struct tagIMC   *pImcNext;
    ULONG_PTR       dwClientImcData;        // Client side data
    HWND            hImeWnd;                // in use Ime Window
} IMC, *PIMC;


/*
 * Hook structure.
 */
#undef HOOKBATCH
typedef struct tagHOOK {   /* hk */
    THRDESKHEAD     head;
    PHOOK           phkNext;
    int             iHook;              // WH_xxx hook type
    KERNEL_ULONG_PTR offPfn;
    UINT            flags;              // HF_xxx flags
    int             ihmod;
    PTHREADINFO     ptiHooked;          // Thread hooked.
    PDESKTOP        rpdesk;             // Global hook pdesk. Only used when
                                        //  hook is locked and owner is destroyed
#ifdef HOOKBATCH
    DWORD           cEventMessages;     // Number of events in the cache
    DWORD           iCurrentEvent;      // Current cache event
    DWORD           CacheTimeOut;       // Timeout between keys
    PEVENTMSG       aEventCache;        // The array of Events
#endif // HOOKBATCH
} HOOK;

/*
 * Hook defines.
 */
#define HF_GLOBAL          0x0001
#define HF_ANSI            0x0002
#define HF_NEEDHC_SKIP     0x0004
#define HF_HUNG            0x0008      // Hook Proc hung don't call if system
#define HF_HOOKFAULTED     0x0010      // Hook Proc faulted
#define HF_NOPLAYBACKDELAY 0x0020      // Ignore requested delay
#define HF_WX86KNOWNDLL    0x0040      // Hook Module is x86 machine type
#define HF_DESTROYED       0x0080      // Set by FreeHook
#if DBG
#define HF_INCHECKWHF      0x0100      // fsHooks is being updated
#define HF_FREED           0x0200      // Object has been freed.
#define HF_DBGUSED         0x03FF      // Update if adding a flag
#endif

/*
 * Macro to convert the WH_* index into a bit position for
 * the fsHooks fields of SERVERINFO and THREADINFO.
 */
#define WHF_FROM_WH(n)     (1 << (n + 1))

/*
 * Flags for IsHooked().
 */
#define WHF_MSGFILTER       WHF_FROM_WH(WH_MSGFILTER)
#define WHF_JOURNALRECORD   WHF_FROM_WH(WH_JOURNALRECORD)
#define WHF_JOURNALPLAYBACK WHF_FROM_WH(WH_JOURNALPLAYBACK)
#define WHF_KEYBOARD        WHF_FROM_WH(WH_KEYBOARD)
#define WHF_GETMESSAGE      WHF_FROM_WH(WH_GETMESSAGE)
#define WHF_CALLWNDPROC     WHF_FROM_WH(WH_CALLWNDPROC)
#define WHF_CALLWNDPROCRET  WHF_FROM_WH(WH_CALLWNDPROCRET)
#define WHF_CBT             WHF_FROM_WH(WH_CBT)
#define WHF_SYSMSGFILTER    WHF_FROM_WH(WH_SYSMSGFILTER)
#define WHF_MOUSE           WHF_FROM_WH(WH_MOUSE)
#define WHF_HARDWARE        WHF_FROM_WH(WH_HARDWARE)
#define WHF_DEBUG           WHF_FROM_WH(WH_DEBUG)
#define WHF_SHELL           WHF_FROM_WH(WH_SHELL)
#define WHF_FOREGROUNDIDLE  WHF_FROM_WH(WH_FOREGROUNDIDLE)

/*
 * Windowstation and desktop enum list structure.
 */
typedef struct tagNAMELIST {
    DWORD cb;
    DWORD cNames;
    WCHAR awchNames[1];
} NAMELIST, *PNAMELIST;

#define MONF_VISIBLE         0x01   // monitor is visible on desktop
#define MONF_PALETTEDISPLAY  0x02   // monitor has palette

#ifndef _USERSRV_
/*
 * Monitor information structure.
 *
 *     This structure defines the attributes of a single monitor
 *     in a virtual display.
 */
typedef struct tagMONITOR {
    HEAD                        head;            // object handle stuff

    PMONITOR                    pMonitorNext;    // next monitor in free or used list
    DWORD                       dwMONFlags;      // flags
    RECT                        rcMonitor;       // location of monitor in virtual screen coordinates
    RECT                        rcWork;          // work area of monitor in virtual screen coordinates
    KHRGN                       hrgnMonitor;     // monitor region in virtual screen coordinates
    short                       cFullScreen;     // number of fullscreen apps on this monitor
    short                       cWndStack;       // number of tiled top-level windows
    KHANDLE                     hDev;            // hdev associated with this monitor
} MONITOR;
#endif

/*
 * Display Information Structure.
 *
 *   This structure defines the display attributes for the
 *   desktop.  This is usually maintained in the DESKTOP
 *   structure. The current display in use is pointed to
 *   by gpDispInfo.
 *
 *   CONSIDER: How many of these fields need to be actually kept
 *   in a DISPLAYINFO that is not in use, rather than just be put
 *   in gpsi or a kernel-side global?
 */
#ifndef _USERSRV_

typedef struct tagDISPLAYINFO {
    // device stuff
    KHANDLE       hDev;
    KERNEL_PVOID  pmdev;
    KHANDLE       hDevInfo;

    // useful dcs
    KHDC          hdcScreen;        // Device-Context for screen
    KHDC          hdcBits;          // Holds system-bitmap resource

    // Graystring resources
    KHDC          hdcGray;          // GrayString DC.
    KHBITMAP      hbmGray;          // GrayString Bitmap Surface.
    int           cxGray;           // width of gray bitmap
    int           cyGray;           // height of gray bitmap

    // random stuff
    PDCE          pdceFirst;       // list of dcs
    PSPB          pspbFirst;       // list of spbs

    // Monitors on this device
    ULONG         cMonitors;        // number of MONF_VISIBLE monitors attached to desktop
    PMONITOR      pMonitorPrimary;  // the primary monitor (display)
    PMONITOR      pMonitorFirst;    // monitor in use list

    // device characteristics
    RECT          rcScreen;         // Rectangle of entire desktop surface
    KHRGN         hrgnScreen;       // region describing virtual screen
    WORD          dmLogPixels;      // pixels per inch
    WORD          BitCountMax;      // Maximum bitcount across all monitors

    BOOL          fDesktopIsRect:1;   // Is the desktop a simple rectangle?
    BOOL          fAnyPalette:1;      // Are any of the monitors paletized?

    // NOTE: if you need more flags, make fDesktopIsRect a flags field instead.

} DISPLAYINFO;

/*
 * Multimonitor function in rtl\mmrtl.c
 */
PMONITOR _MonitorFromPoint(POINT pt, DWORD dwFlags);
PMONITOR _MonitorFromRect(LPCRECT lprc, DWORD dwFlags);
PMONITOR _MonitorFromWindow(PWND pwnd, DWORD dwFlags);
#endif

#define HDCBITS() gpDispInfo->hdcBits

#define DTF_NEEDSPALETTECHANGED      0x00000001
#define DTF_NEEDSREDRAW              0x00000002

#define CWINHOOKS       (WH_MAX - WH_MIN + 1)

/*
 * VWPL - Volatile Window Pointer List (see rare.c)
 * VPWLs are manipulate with the functions:
 *    VWPLAdd(), VWPLRemove() and VWPLNext()
 */
typedef struct {
    DWORD       cPwnd;       // number of pwnds in apwnd[]
    DWORD       cElem;       // number of elements in apwnd[]
    DWORD       cThreshhold; // (re)allocation increment/decrement
    PWND        aPwnd[0];    // array of pwnds
} VWPL, * KPTR_MODIFIER PVWPL;

/*
 * Desktop Information Structure.
 *
 *   This structure contains information regading the
 *   desktop.  This is viewable from both the client and
 *   kernel processes.
 */
typedef struct tagDESKTOPINFO {

    KERNEL_PVOID  pvDesktopBase;          // For handle validation
    KERNEL_PVOID  pvDesktopLimit;         // ???
    PWND          spwnd;                 // Desktop window
    DWORD         fsHooks;                // Deskop global hooks
    PHOOK         aphkStart[CWINHOOKS];  // List of hooks
    PWND          spwndShell;            // Shell window
    PPROCESSINFO  ppiShellProcess;        // Shell Process
    PWND          spwndBkGnd;            // Shell background window
    PWND          spwndTaskman;          // Task-Manager window
    PWND          spwndProgman;          // Program-Manager window
    PVWPL         pvwplShellHook;         // see (De)RegisterShellHookWindow
    int           cntMBox;                // ???
} DESKTOPINFO;


#define CURSOR_ALWAYSDESTROY    0
#define CURSOR_CALLFROMCLIENT   1
#define CURSOR_THREADCLEANUP    2

typedef struct tagCURSOR_ACON {
    PROCMARKHEAD    head;
    PCURSOR         pcurNext;
    UNICODE_STRING   strName;
    ATOM             atomModName;
    WORD             rt;
} CURSOR_ACON;

typedef struct CURSOR_COMMON {
    CURSINFO;                          // CURSINFO includes the flags

    DWORD            bpp;
    DWORD            cx;
    DWORD            cy;
} CURSOR_COMMON;

typedef struct ACON_COMMON {
    int            cpcur;              // Count of image frames
    int            cicur;              // Count of steps in animation sequence
    PCURSOR * KPTR_MODIFIER aspcur;    // Array of image frame pointers
    DWORD * KPTR_MODIFIER aicur;       // Array of frame indices (seq-table)
    JIF * KPTR_MODIFIER ajifRate;      // Array of time offsets
    int            iicur;              // Current step in animation
} ACON_COMMON;

typedef struct tagCURSOR {
    CURSOR_ACON;                       // common cursor/acon elements -
                                       // See SetSystemImage()
    CURSOR_COMMON;
} CURSOR;

typedef struct tagACON {               // acon
    CURSOR_ACON;                       // common cursor/acon elements -
                                       // See SetSystemImage()
    /*
     * CURSORF_flags must be the first element to follow CURSOR_ACON. This
     * way all members up to and including CURSORF_flags are the same in
     * tagCURSOR and tagACON which is needed for SetSystemImage. See more
     * comments for CI_FIRST in wingdi.w.
     */
    DWORD CURSORF_flags;               // same as CI_FIRST in CURSINFO

    ACON_COMMON;
} ACON, *PACON;

#define PICON PCURSOR

typedef struct tagCURSORDATA {
    KLPWSTR  lpName;
    KLPWSTR  lpModName;
    WORD    rt;
    WORD    dummy;

    CURSOR_COMMON;

    ACON_COMMON;
} CURSORDATA, *PCURSORDATA;


typedef struct tagCURSORFIND {

    KHCURSOR hcur;
    DWORD   rt;
    DWORD   cx;
    DWORD   cy;
    DWORD   bpp;

} CURSORFIND, *PCURSORFIND;

#define MSGFLAG_MASK                0xFFFE0000
#define MSGFLAG_WOW_RESERVED        0x00010000      // Used by WOW
#define MSGFLAG_DDE_MID_THUNK       0x80000000      // DDE tracking thunk
#define MSGFLAG_DDE_SPECIAL_SEND    0x40000000      // WOW bad DDE app hack
#define MSGFLAG_SPECIAL_THUNK       0x10000000      // server->client thunk needs special handling

#define WIDTHBYTES(i) \
    ((((i) + 31) & ~31) >> 3)

#define BITMAPWIDTHSIZE(cx, cy, planes, bpp) \
    (WIDTHBYTES((cx * bpp)) * (cy) * (planes))

/*
 * Window Style and State Masks -
 *
 * High byte of word is byte index from the start of the state field
 * in the WND structure, low byte is the mask to use on the byte.
 * These masks assume the order of the state and style fields of a
 * window instance structure.
 *
 * This is how the Test/Set/Clr/MaskWF value ranges map to the corresponding
 * fields in the window structure.
 *
 *   offset                 WND field
 *   0 - 3                  state        - private
 *   4 - 7                  state2       - private
 *   8 - B                  ExStyle      - public, exposed in SetWindowLong(GWL_EXSTYLE)
 *   C - F                  style        - public, exposed in SetWindowLong(GWL_STYLE)
 *                                         C-D are reserved for window class designer.
 *                                         E-F are reserved for WS_ styles.
 *
 * NOTE: Be sure to add the flag to the wFlags array in kd\userexts.c!!!
 */

/*
 * State flags, from 0x0000 to 0x0780.
 */

/*
 * DON'T MOVE ANY ONE OF THE FOLLOWING WFXPRESENT FLAGS,
 * BECAUSE WFFRAMEPRESENTMASK DEPENDS ON THEIR VALUES
 */
#define WFMPRESENT              0x0001
#define WFVPRESENT              0x0002
#define WFHPRESENT              0x0004
#define WFCPRESENT              0x0008
#define WFFRAMEPRESENTMASK      0x000F

#define WFSENDSIZEMOVE          0x0010
#define WFMSGBOX                0x0020  // used to maintain count of msg boxes on screen
#define WFFRAMEON               0x0040
#define WFHASSPB                0x0080
#define WFNONCPAINT             0x0101
#define WFSENDERASEBKGND        0x0102
#define WFERASEBKGND            0x0104
#define WFSENDNCPAINT           0x0108
#define WFINTERNALPAINT         0x0110
#define WFUPDATEDIRTY           0x0120
#define WFHIDDENPOPUP           0x0140
#define WFMENUDRAW              0x0180

/*
 * NOTE -- WFDIALOGWINDOW is used in WOW.  DO NOT CHANGE without
 *   changing WD_DIALOG_WINDOW in winuser.w
 */
#define WFDIALOGWINDOW          0x0201

#define WFTITLESET              0x0202
#define WFSERVERSIDEPROC        0x0204
#define WFANSIPROC              0x0208
#define WFBEINGACTIVATED        0x0210  // prevent recursion in xxxActivateThis Window
#define WFHASPALETTE            0x0220
#define WFPAINTNOTPROCESSED     0x0240  // WM_PAINT message not processed
#define WFSYNCPAINTPENDING      0x0280
#define WFGOTQUERYSUSPENDMSG    0x0301
#define WFGOTSUSPENDMSG         0x0302
#define WFTOGGLETOPMOST         0x0304  // Toggle the WS_EX_TOPMOST bit ChangeStates

/*
 * DON'T MOVE REDRAWIFHUNGFLAGS WITHOUT ADJUSTING WFANYHUNGREDRAW
 */
#define WFREDRAWIFHUNG          0x0308
#define WFREDRAWFRAMEIFHUNG     0x0310
#define WFANYHUNGREDRAW         0x0318

#define WFANSICREATOR           0x0320
#define WFREALLYMAXIMIZABLE     0x0340  // The window fills the work area or monitor when maximized
#define WFDESTROYED             0x0380
#define WFWMPAINTSENT           0x0401
#define WFDONTVALIDATE          0x0402
#define WFSTARTPAINT            0x0404
#define WFOLDUI                 0x0408
#define WFCEPRESENT             0x0410  // Client edge present
#define WFBOTTOMMOST            0x0420  // Bottommost window
#define WFFULLSCREEN            0x0440
#define WFINDESTROY             0x0480

/*
 * DON'T MOVE ANY ONE OF THE FOLLOWING WFWINXXCOMPAT FLAGS,
 * BECAUSE WFWINCOMPATMASK DEPENDS ON THEIR VALUES
 */
#define WFWIN31COMPAT           0x0501  // Win 3.1 compatible window
#define WFWIN40COMPAT           0x0502  // Win 4.0 compatible window
#define WFWIN50COMPAT           0x0504  // Win 5.0 compatibile window
#define WFWINCOMPATMASK         0x0507  // Compatibility flag mask

#define WFMAXFAKEREGIONAL       0x0508  // Window has a fake region for maxing on 1 monitor

// Active Accessibility (Window Event) state
#define WFCLOSEBUTTONDOWN       0x0510
#define WFZOOMBUTTONDOWN        0x0520
#define WFREDUCEBUTTONDOWN      0x0540
#define WFHELPBUTTONDOWN        0x0580
#define WFLINEUPBUTTONDOWN      0x0601  // Line up/left scroll button down
#define WFPAGEUPBUTTONDOWN      0x0602  // Page up/left scroll area down
#define WFPAGEDNBUTTONDOWN      0x0604  // Page down/right scroll area down
#define WFLINEDNBUTTONDOWN      0x0608  // Line down/right scroll area down
#define WFSCROLLBUTTONDOWN      0x0610  // Any scroll button down?
#define WFVERTSCROLLTRACK       0x0620  // Vertical or horizontal scroll track...

#define WFALWAYSSENDNCPAINT     0x0640  // Always send WM_NCPAINT to children
#define WFPIXIEHACK             0x0680  // Send (HRGN)1 to WM_NCPAINT (see PixieHack)

/*
 * WFFULLSCREENBASE MUST HAVE LOWORD OF 0. See SetFullScreen macro.
 */
#define WFFULLSCREENBASE        0x0700  // Fullscreen flags take up 0x0701
#define WFFULLSCREENMASK        0x0707  // and 0x0702 and 0x0704
#define WEFTRUNCATEDCAPTION     0x0708  // The caption text was truncated -> caption tootip

#define WFNOANIMATE             0x0710  // ???
#define WFSMQUERYDRAGICON       0x0720  // ??? Small icon comes from WM_QUERYDRAGICON
#define WFSHELLHOOKWND          0x0740  // ???
#define WFISINITIALIZED         0x0780  // Window is initialized -- checked by WoW32

/*
 * Add more state flags here, up to 0x0780.
 * Look for empty slots above before adding to the end.
 * Be sure to add the flag to the wFlags array in kd\userexts.c
 */

/*
 * Window Extended Style, from 0x0800 to 0x0B80.
 */
#define WEFDLGMODALFRAME        0x0801  // WS_EX_DLGMODALFRAME
#define WEFDRAGOBJECT           0x0802  // ???
#define WEFNOPARENTNOTIFY       0x0804  // WS_EX_NOPARENTNOTIFY
#define WEFTOPMOST              0x0808  // WS_EX_TOPMOST
#define WEFACCEPTFILES          0x0810  // WS_EX_ACCEPTFILES
#define WEFTRANSPARENT          0x0820  // WS_EX_TRANSPARENT
#define WEFMDICHILD             0x0840  // WS_EX_MDICHILD
#define WEFTOOLWINDOW           0x0880  // WS_EX_TOOLWINDOW
#define WEFWINDOWEDGE           0x0901  // WS_EX_WINDOWEDGE
#define WEFCLIENTEDGE           0x0902  // WS_EX_CLIENTEDGE
#define WEFEDGEMASK             0x0903  // WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE
#define WEFCONTEXTHELP          0x0904  // WS_EX_CONTEXTHELP


// intl styles
#define WEFRIGHT                0x0910  // WS_EX_RIGHT
#define WEFRTLREADING           0x0920  // WS_EX_RTLREADING
#define WEFLEFTSCROLL           0x0940  // WS_EX_LEFTSCROLLBAR


#define WEFCONTROLPARENT        0x0A01  // WS_EX_CONTROLPARENT
#define WEFSTATICEDGE           0x0A02  // WS_EX_STATICEDGE
#define WEFAPPWINDOW            0x0A04  // WS_EX_APPWINDOW
#define WEFLAYERED              0x0A08  // WS_EX_LAYERED

#ifdef USE_MIRRORING
#define WEFNOINHERITLAYOUT      0x0A10  // WS_EX_NOINHERITLAYOUT
#define WEFLAYOUTVBHRESERVED    0x0A20  // WS_EX_LAYOUTVBHRESERVED
#define WEFLAYOUTRTL            0x0A40  // WS_EX_LAYOUTRTL
#define WEFLAYOUTBTTRESERVED    0x0A80  // WS_EX_LAYOUTBTTRESERVED
#endif

/*
 * To delay adding a new state3 DWORD in the WW structure, we're using
 * the extended style bits for now.  If we'll need more of these, we'll
 * add the new DWORD and move these ones around
 */
#define WEFPUIFOCUSHIDDEN         0x0B80  // focus indicators hidden
#define WEFPUIACCELHIDDEN         0x0B40  // keyboard acceleraors hidden

/*
 * Add more Window Extended Style flags here, up to 0x0B80.
 * Be sure to add the flag to the wFlags array in kd\userexts.c
 */
#ifdef REDIRECTION
#define WEFREDIRECTED           0x0B01   // WS_EX_REDIRECTED
#endif // REDIRECTION

#define WEFNOACTIVATE           0x0B08   // WS_EX_NOACTIVATE

/*
 * Window styles, from 0x0E00 to 0x0F80.
 */
#define WFMAXBOX                0x0E01  // WS_MAXIMIZEBOX
#define WFTABSTOP               0x0E01  // WS_TABSTOP
#define WFMINBOX                0x0E02  // WS_MAXIMIZEBOX
#define WFGROUP                 0x0E02  // WS_GROUP
#define WFSIZEBOX               0x0E04  // WS_THICKFRAME, WS_SIZEBOX
#define WFSYSMENU               0x0E08  // WS_SYSMENU
#define WFHSCROLL               0x0E10  // WS_HSCROLL
#define WFVSCROLL               0x0E20  // WS_VSCROLL
#define WFDLGFRAME              0x0E40  // WS_DLGFRAME
#define WFTOPLEVEL              0x0E40  // ???
#define WFBORDER                0x0E80  // WS_BORDER
#define WFBORDERMASK            0x0EC0  // WS_BORDER | WS_DLGFRAME
#define WFCAPTION               0x0EC0  // WS_CAPTION

#define WFTILED                 0x0F00  // WS_OVERLAPPED, WS_TILED
#define WFMAXIMIZED             0x0F01  // WS_MAXIMIZE
#define WFCLIPCHILDREN          0x0F02  // WS_CLIPCHILDREN
#define WFCLIPSIBLINGS          0x0F04  // WS_CLIPSIBLINGS
#define WFDISABLED              0x0F08  // WS_DISABLED
#define WFVISIBLE               0x0F10  // WS_VISIBLE
#define WFMINIMIZED             0x0F20  // WS_MINIMIZE
#define WFCHILD                 0x0F40  // WS_CHILD
#define WFPOPUP                 0x0F80  // WS_POPUP
#define WFTYPEMASK              0x0FC0  // WS_CHILD | WS_POPUP
#define WFICONICPOPUP           0x0FC0  // WS_CHILD | WS_POPUP
#define WFICONIC                WFMINIMIZED
/*
 * No more Window style flags are available, use Extended window styles.
 */

/*
 * Window Styles for built-in classes, from 0x0C00 to 0x0D80.
 */

// Buttons
#define BFTYPEMASK              0x0C0F

#define BFRIGHTBUTTON           0x0C20
#define BFICON                  0x0C40
#define BFBITMAP                0x0C80
#define BFIMAGEMASK             0x0CC0

#define BFLEFT                  0x0D01
#define BFRIGHT                 0x0D02
#define BFCENTER                0x0D03
#define BFHORZMASK              0x0D03
#define BFTOP                   0x0D04
#define BFBOTTOM                0x0D08
#define BFVCENTER               0x0D0C
#define BFVERTMASK              0x0D0C
#define BFALIGNMASK             0x0D0F

#define BFPUSHLIKE              0x0D10
#define BFMULTILINE             0x0D20
#define BFNOTIFY                0x0D40
#define BFFLAT                  0x0D80

#define ISBSTEXTOROD(pwnd) (!TestWF(pwnd, BFBITMAP) && !TestWF(pwnd, BFICON))

// Combos
#define CBFSIMPLE               0x0C01
#define CBFDROPDOWN             0x0C02
#define CBFDROPDOWNLIST         0x0C03

#define CBFEDITABLE             0x0C01
#define CBFDROPPABLE            0x0C02
#define CBFDROPTYPE             0x0C03

#define CBFOWNERDRAWFIXED       0x0C10
#define CBFOWNERDRAWVAR         0x0C20
#define CBFOWNERDRAW            0x0C30

#define CBFAUTOHSCROLL          0x0C40
#define CBFOEMCONVERT           0x0C80
#define CBFSORT                 0x0D01
#define CBFHASSTRINGS           0x0D02
#define CBFNOINTEGRALHEIGHT     0x0D04
#define CBFDISABLENOSCROLL      0x0D08
#define CBFBUTTONUPTRACK        0x0D10

#define CBFUPPERCASE            0x0D20
#define CBFLOWERCASE            0x0D40

// Dialogs
#define DFSYSMODAL              0x0C02
#define DF3DLOOK                0x0C04
#define DFNOFAILCREATE          0x0C10
#define DFLOCALEDIT             0x0C20
#define WFNOIDLEMSG             0x0D01
#define DFCONTROL               0x0D04

// Edits
#define EFMULTILINE             0x0C04
#define EFUPPERCASE             0x0C08
#define EFLOWERCASE             0x0C10
#define EFPASSWORD              0x0C20
#define EFAUTOVSCROLL           0x0C40
#define EFAUTOHSCROLL           0x0C80
#define EFNOHIDESEL             0x0D01
#define EFCOMBOBOX              0x0D02
#define EFOEMCONVERT            0x0D04
#define EFREADONLY              0x0D08
#define EFWANTRETURN            0x0D10
#define EFNUMBER                0x0D20

// Scrollbars
#define SBFSIZEBOXTOPLEFT       0x0C02
#define SBFSIZEBOXBOTTOMRIGHT   0x0C04
#define SBFSIZEBOX              0x0C08
#define SBFSIZEGRIP             0x0C10

// Statics
#define SFTYPEMASK              0x0C1F
#define SFNOPREFIX              0x0C80
#define SFNOTIFY                0x0D01
#define SFCENTERIMAGE           0x0D02
#define SFRIGHTJUST             0x0D04
#define SFREALSIZEIMAGE         0x0D08
#define SFSUNKEN                0x0D10
#define SFEDITCONTROL           0x0D20
#define SFELLIPSISMASK          0x0DC0
#define SFWIDELINESPACING       0x0C20


/*
 *
 */
#define SYS_ALTERNATE           0x2000
#define SYS_PREVKEYSTATE        0x4000

/*** AWESOME HACK ALERT!!!
 *
 * The low byte of the WF?PRESENT state flags must NOT be the
 * same as the low byte of the WFBORDER and WFCAPTION flags,
 * since these are used as paint hint masks.  The masks are calculated
 * with the MaskWF macro below.
 *
 * The magnitude of this hack compares favorably with that of the national debt.
 *
 * STATEOFFSET is the offset into the WND structure of the state field.
 * The state field is actually part of the WW structure defined in wowuserp.h
 * which is embedded in the WND structure.
 */
#define STATEOFFSET (FIELD_OFFSET(WND, state))


/*
 * Redefine LOBYTE to get rid of compiler warning C4309:
 * 'cast' : truncation of constant value
 */
#ifdef LOBYTE
    #undef LOBYTE
#endif

#define LOBYTE(w)            ((BYTE)((w) & 0x00FF))

#define TestWF(hwnd, flag)   (*(((BYTE *)(hwnd)) + STATEOFFSET + (int)HIBYTE(flag)) & LOBYTE(flag))
#define SetWF(hwnd, flag)    (*(((BYTE *)(hwnd)) + STATEOFFSET + (int)HIBYTE(flag)) |= LOBYTE(flag))
#define ClrWF(hwnd, flag)    (*(((BYTE *)(hwnd)) + STATEOFFSET + (int)HIBYTE(flag)) &= ~LOBYTE(flag))
#define MaskWF(flag)         ((WORD)( (HIBYTE(flag) & 1) ? LOBYTE(flag) << 8 : LOBYTE(flag)))


#define TestwndChild(hwnd)   (TestWF(hwnd, WFTYPEMASK) == LOBYTE(WFCHILD))
#define TestwndIPopup(hwnd)  (TestWF(hwnd, WFTYPEMASK) == LOBYTE(WFICONICPOPUP))
#define TestwndTiled(hwnd)   (TestWF(hwnd, WFTYPEMASK) == LOBYTE(WFTILED))
#define TestwndNIPopup(hwnd) (TestWF(hwnd, WFTYPEMASK) == LOBYTE(WFPOPUP))
#define TestwndPopup(hwnd)   (TestwndNIPopup(hwnd) || TestwndIPopup(hwnd))
#define TestwndHI(hwnd)      (TestwndTiled(hwnd) || TestwndIPopup(hwnd))

#define GetChildParent(pwnd) (TestwndChild(pwnd) ? pwnd->spwndParent : (PWND)NULL)
#define GetWindowCreator(pwnd) (TestwndChild(pwnd) ? pwnd->spwndParent : pwnd->spwndOwner)

#define TestwndFrameOn(pwnd) (TestWF(pwnd, WFFRAMEON) && (GETPTI(pwnd)->pq == gpqForeground))

#define GetFullScreen(pwnd)        (TestWF(pwnd, WFFULLSCREENMASK))
#define SetFullScreen(pwnd, state) (ClrWF(pwnd, WFFULLSCREENMASK), \
                                    SetWF(pwnd, WFFULLSCREENBASE | (state & WFFULLSCREENMASK)))

//#define FTrueVis(pwnd)       (pwnd->fs & WF_TRUEVIS)
#define FTrueVis(pwnd)       (_IsWindowVisible(pwnd))
#define _IsWindowEnabled(pwnd) (TestWF(pwnd, WFDISABLED)  == 0)
#define _IsIconic(pwnd)        (TestWF(pwnd, WFMINIMIZED) != 0)
#define _IsZoomed(pwnd)        (TestWF(pwnd, WFMAXIMIZED) != 0)

WORD VersionFromWindowFlag(PWND pwnd);

#define SV_UNSET        0x0000
#define SV_SET          0x0001
#define SV_CLRFTRUEVIS  0x0002

/*
 * System menu IDs
 */
#define ID_SYSMENU              0x10
#define ID_CLOSEMENU            0x20
#define CHILDSYSMENU            ID_CLOSEMENU
#define ID_DIALOGSYSMENU        0x30
#define ID_HSCROLLMENU          0x40
#define ID_VSCROLLMENU          0x50

/*
 * Menu Item Structure
 */
typedef struct tagITEM {
    UINT                fType;          // Item Type  Flags
    UINT                fState;         // Item State Flags
    UINT                wID;
    PMENU               spSubMenu;      /* Handle to a popup */
    KHANDLE             hbmpChecked;    /* Bitmap for an on  check */
    KHANDLE             hbmpUnchecked;  /* Bitmap for an off check */
    KLPWSTR             lpstr;          //item's text
    DWORD               cch;            /* String: WCHAR count */
    KERNEL_ULONG_PTR    dwItemData;
    DWORD               xItem;
    DWORD               yItem;
    DWORD               cxItem;
    DWORD               cyItem;
    DWORD               dxTab;
    DWORD               ulX;            /* String: Underline start */
    DWORD               ulWidth;        /* String: underline width */
    KHBITMAP            hbmp;           // item's bitmap
    int                 cxBmp;          // bitmap width
    int                 cyBmp;          // bitmap height
} ITEM, * KPTR_MODIFIER PITEM, * KPTR_MODIFIER LPITEM;

/*
 * MENULIST structure, holds the PMENUs that contain a submenu
 * We store a list of menus in MENU.pParentMenus as a menu
 * can be submenu in more items
 */
typedef struct tagMENULIST {
    struct tagMENULIST   *pNext;
    PMENU       pMenu;
} MENULIST, * KPTR_MODIFIER PMENULIST;

/*
 * Scroll menu arrow flags
 */
#define MSA_OFF         0
#define MSA_ON          1
#define MSA_ATTOP       2
#define MSA_ATBOTTOM    3

/*
 * Menu Structure
 */
typedef struct tagMENU {
    PROCDESKHEAD    head;
    DWORD           fFlags;         /* Menu Flags */
    int             iItem;          /* Contains the position of the selected
                                       item in the menu. -1 if no selection */
    UINT            cAlloced;       // Number of items that can fit in rgItems
    UINT            cItems;         /* Number of items in rgItems */

    DWORD           cxMenu;
    DWORD           cyMenu;
    DWORD           cxTextAlign;    /* Text align offset for popups*/
    PWND            spwndNotify;     /* The owner hwnd of this menu */
    PITEM           rgItems;        /* The list of items in this menu */
    PMENULIST       pParentMenus;   // The list of parents (menus that have this as submenu)
    DWORD           dwContextHelpId;// Context help Id for the whole menu
    DWORD           cyMax;          /* max menu height after which menu scrolls */
    KERNEL_ULONG_PTR dwMenuData;     /* app-supplied menu data */

    KHBRUSH         hbrBack;        // background brush for menu
    int             iTop;           // Scroll top
    int             iMaxTop;        // Scroll MaxTop
    DWORD           dwArrowsOn:2;   // Scroll flags
} MENU, * KPTR_MODIFIER PMENU;


/*
 *  Items used for WinHelp and Context Sensitive help support
 */

#define ID_HELPMENU            4

// WINHELP4 invoked type
enum {
        TYPE_NORMAL,
        TYPE_POPUP,
        TYPE_TCARD
};

typedef struct tagDLGENUMDATA {
    PWND    pwndDialog;
    PWND    pwndControl;
    POINT   ptCurHelp;
} DLGENUMDATA, *PDLGENUMDATA;

BOOL CALLBACK EnumPwndDlgChildProc(PWND pwnd, LPARAM lParam);
BOOL FIsParentDude(PWND pwnd);


#define MNF_DONTSKIPSEPARATORS      0x0001

/*
 * The following masks can be used along with the wDisableFlags field of SB
 * to find if the Up/Left or Down/Right arrow or Both are disabled;
 * Now it is possible to selectively Enable/Disable just one or both the
 * arrows in a scroll bar control;
 */
#define LTUPFLAG    0x0001  // Left/Up arrow disable flag.
#define RTDNFLAG    0x0002  // Right/Down arrow disable flag.

typedef struct tagSBCALC {
    SBDATA;               /* this must be first -- we cast structure pointers */
    int    pxTop;
    int    pxBottom;
    int    pxLeft;
    int    pxRight;
    int    cpxThumb;
    int    pxUpArrow;
    int    pxDownArrow;
    int    pxStart;         /* Initial position of thumb */
    int    pxThumbBottom;
    int    pxThumbTop;
    int    cpx;
    int    pxMin;
} SBCALC, *PSBCALC;

typedef struct tagSBTRACK {
    DWORD  fHitOld : 1;
    DWORD  fTrackVert : 1;
    DWORD  fCtlSB : 1;
    DWORD  fTrackRecalc: 1;
    PWND   spwndTrack;
    PWND   spwndSB;
    PWND   spwndSBNotify;
    RECT   rcTrack;
    VOID   (*xxxpfnSB)(PWND, UINT, WPARAM, LPARAM, PSBCALC);
    UINT   cmdSB;
    UINT_PTR hTimerSB;
    int    dpxThumb;        /* Offset from mouse point to start of thumb box */
    int    pxOld;           /* Previous position of thumb */
    int    posOld;
    int    posNew;
    int    nBar;
    PSBCALC pSBCalc;
} SBTRACK, *PSBTRACK;

/*
 * How many times a thread can spin through get/peek message without idling
 * before the system puts the app in the background.
 */
#define CSPINBACKGROUND 100

#define CCHTITLEMAX     256

#define SW_MDIRESTORE   0xCC    /* special xxxMinMaximize() command for MDI */

/*
 * This is used by CreateWindow() - the 16 bit version of CW_USEDEFAULT,
 * that we still need to support.
 */
#define CW2_USEDEFAULT      0x8000
#define CW_FLAGS_DIFFHMOD   0x80000000


/*
 * Menu commands
 */
//#define MENUBIT             (0x8000)
//#define MENUUP              (0x8000 | VK_UP)
//#define MENUDOWN            (0x8000 | VK_DOWN)
//#define MENULEFT            (0x8000 | VK_LEFT)
//#define MENURIGHT           (0x8000 | VK_RIGHT)
//#define MENUEXECUTE         TEXT('\r')      /* Return character */
#define MENUSYSMENU         TEXT(' ')       /* Space character */
#define MENUCHILDSYSMENU    TEXT('-')       /* Hyphen */

#define MF_ALLSTATE         0x00FF
#define MF_MAINMENU         0xFFFF
#define MFMWFP_OFFMENU      0
#define MFMWFP_MAINMENU     0x0000FFFF
#define MFMWFP_NOITEM       0xFFFFFFFF
#define MFMWFP_UPARROW      0xFFFFFFFD  /* Warning: Also used to define IDSYS_MNUP */
#define MFMWFP_DOWNARROW    0xFFFFFFFC  /* Warning: Also used to define IDSYS_MNDOWN */
#define MFMWFP_MINVALID     0xFFFFFFFC
#define MFMWFP_ALTMENU      0xFFFFFFFB
#define MFMWFP_FIRSTITEM    0


/*
 * NOTE: SMF() can only be used on single bit flags (NOT MRGFDISABLED!).
 */
#define SetMF(pmenu, flag)    ((pmenu)->fFlags |=  (flag))
#define ClearMF(pmenu, flag)  ((pmenu)->fFlags &= ~(flag))
#define TestMF(pmenu, flag)   ((pmenu)->fFlags &   (flag))

#define SetMFS(pitem, flag)   ((pitem)->fState |=  (flag))
#define TestMFS(pitem, flag)  ((pitem)->fState &   (flag))
#define ClearMFS(pitem, flag) ((pitem)->fState &= ~(flag))

#define SetMFT(pitem, flag)   ((pitem)->fType |=  (flag))
#define TestMFT(pitem, flag)  ((pitem)->fType &   (flag))
#define ClearMFT(pitem, flag) ((pitem)->fType &= ~(flag))

/*
 * Dialog structure (dlg). The window-words for the dialog structure must
 * be EXACTLY 30 bytes long! This is because Windows 3.0 exported a constant
 * called DLGWINDOWEXTRA that resolved to 30. Although we could redefine this
 * for 32-bit windows apps, we cannot redefine it for 16 bit apps (it is
 * a difficult problem). So instead we peg the window-words at 30 bytes
 * exactly, and allocate storage for the other information.
 */
typedef struct _DLG {
    DLGPROC lpfnDlg;
    DWORD   flags;          /* Various useful flags -- see definitions below */
    int     cxChar;
    int     cyChar;
    KHWND   hwndFocusSave;
    UINT    fEnd      : 1;
    UINT    fDisabled : 1;
    KERNEL_INT_PTR result;         /* DialogBox result */
    KHANDLE  hData;          /* Global handle for edit ctl storage. */
    KHFONT   hUserFont;      /* Handle of the font mentioned by the user in template*/
#ifdef SYSMODALWINDOWS
    KHWND    hwndSysModalSave;  /* Previous sysmodal window saved here */
#endif
} DLG, * KPTR_MODIFIER PDLG;

typedef struct _DIALOG {
    WND             wnd;
    KERNEL_LRESULT  resultWP;       /* window proc result -- DWL_MSGRESULT (+0) */
    PDLG            pdlg;
    KERNEL_LONG_PTR unused;        /* DWL_USER (+8) */
    BYTE            reserved[DLGWINDOWEXTRA - sizeof(KERNEL_LRESULT) - sizeof(PDLG) - sizeof(KERNEL_LONG_PTR)];
} DIALOG, *PDIALOG;

#define PDLG(pwnd) (((PDIALOG)pwnd)->pdlg)

/*
 * Flags definitions for DLG.flags
 */
#define DLGF_ANSI           0x01    /* lpfnDlg is an ANSI proc */

/*
 * MDI typedefs
 */
typedef struct tagMDI {
    UINT    cKids;
    HWND    hwndMaxedChild;
    HWND    hwndActiveChild;
    HMENU   hmenuWindow;
    UINT    idFirstChild;
    UINT    wScroll;
    LPWSTR  pTitle;
    UINT    iChildTileLevel;
} MDI, * KPTR_MODIFIER PMDI;

typedef struct tagMDIWND {
    WND     wnd;
    UINT    dwReserved;         // quattro pro 1.0 stores stuff here!!
    PMDI    pmdi;
} MDIWND, *PMDIWND;

#define GWLP_MDIDATA        (FIELD_OFFSET(MDIWND, pmdi) - sizeof(WND))

#define TIF_INCLEANUP               (UINT)0x00000001
#define TIF_16BIT                   (UINT)0x00000002
#define TIF_SYSTEMTHREAD            (UINT)0x00000004
#define TIF_CSRSSTHREAD             (UINT)0x00000008
#define TIF_TRACKRECTVISIBLE        (UINT)0x00000010
#define TIF_ALLOWFOREGROUNDACTIVATE (UINT)0x00000020
#define TIF_DONTATTACHQUEUE         (UINT)0x00000040
#define TIF_DONTJOURNALATTACH       (UINT)0x00000080
#define TIF_WOW64                   (UINT)0x00000100 /* Thread is in a emulated 32bit process */
#define TIF_INACTIVATEAPPMSG        (UINT)0x00000200
#define TIF_SPINNING                (UINT)0x00000400
#define TIF_PALETTEAWARE            (UINT)0x00000800
#define TIF_SHAREDWOW               (UINT)0x00001000
#define TIF_FIRSTIDLE               (UINT)0x00002000
#define TIF_WAITFORINPUTIDLE        (UINT)0x00004000
#define TIF_MOVESIZETRACKING        (UINT)0x00008000
#define TIF_VDMAPP                  (UINT)0x00010000
#define TIF_DOSEMULATOR             (UINT)0x00020000
#define TIF_GLOBALHOOKER            (UINT)0x00040000
#define TIF_DELAYEDEVENT            (UINT)0x00080000
#define TIF_MSGPOSCHANGED           (UINT)0x00100000
                                       // 0x00200000 Unused. was TIF_SHUTDOWNCOMPLETE
#define TIF_IGNOREPLAYBACKDELAY     (UINT)0x00400000
#define TIF_ALLOWOTHERACCOUNTHOOK   (UINT)0x00800000
#define TIF_GUITHREADINITIALIZED    (UINT)0x02000000
#define TIF_DISABLEIME              (UINT)0x04000000
#define TIF_INGETTEXTLENGTH         (UINT)0x08000000
#define TIF_ANSILENGTH              (UINT)0x10000000

#define TIF_DISABLEHOOKS            (UINT)0x20000000

#define TIF_RESTRICTED              (UINT)0x40000000

/*
 * Client Thread Information Structure.
 *
 *   This structure contains information regarding the
 *   thread.  This is viewable from both the client and
 *   kernel processes.
 */
typedef struct tagCLIENTTHREADINFO {
    UINT        CTIF_flags;
    WORD        fsChangeBits;           // Bits changes since last compared
    WORD        fsWakeBits;             // Bits currently available
    WORD        fsWakeBitsJournal;      // Bits saved while journalling
    WORD        fsWakeMask;             // Bits looking for when asleep
    LONG        timeLastRead;           // Time of last input read
} CLIENTTHREADINFO;

#define CTIF_SYSQUEUELOCKED         (UINT)0x00000001
#define CTIF_INSENDMESSAGE          (UINT)0x00000002

/*
 * First check for a 0, 0 filter which means we want all input.
 * If inverted message range, filter is exclusive.
 */
#define CheckMsgFilter(wMsg, wMsgFilterMin, wMsgFilterMax)                 \
    (   ((wMsgFilterMin) == 0 && (wMsgFilterMax) == 0xFFFFFFFF)            \
     || (  ((wMsgFilterMin) > (wMsgFilterMax))                             \
         ? (((wMsg) <  (wMsgFilterMax)) || ((wMsg) >  (wMsgFilterMin)))    \
         : (((wMsg) >= (wMsgFilterMin)) && ((wMsg) <= (wMsgFilterMax)))))

UINT    CalcWakeMask(UINT wMsgFilterMin, UINT wMsgFilterMax, UINT fsWakeMaskFilter);

/*
 * GetInputBits
 * This function checks if the specified input (fsWakeMask) has arrived (fsChangeBits)
 *  or it's available (fsWakeBits)
 */
__inline WORD GetInputBits (PCLIENTTHREADINFO pcti, WORD fsWakeMask, BOOL fAvailable)
{
    return (pcti->fsChangeBits  | (fAvailable ? pcti->fsWakeBits : 0)) & fsWakeMask;
}


typedef struct tagCARET {
    struct tagWND *spwnd;
    UINT    fVisible : 1;
    UINT    fOn      : 1;
    int     iHideLevel;
    int     x;
    int     y;
    int     cy;
    int     cx;
    HBITMAP hBitmap;
    UINT_PTR hTimer;
    DWORD   tid;
} CARET, *PCARET;

#define XPixFromXDU(x, cxChar)       MultDiv(x, cxChar, 4)
#define YPixFromYDU(y, cyChar)       MultDiv(y, cyChar, 8)
#define XDUFromXPix(x, cxChar)       MultDiv(x, 4, cxChar)
#define YDUFromYPix(y, cyChar)       MultDiv(y, 8, cyChar)


/*
 * Flags for the Q structure.
 */
#define QF_UPDATEKEYSTATE         (UINT)0x00001

#define QF_FMENUSTATUSBREAK       (UINT)0x00004
#define QF_FMENUSTATUS            (UINT)0x00008
#define QF_FF10STATUS             (UINT)0x00010
#define QF_MOUSEMOVED             (UINT)0x00020
#define QF_ACTIVATIONCHANGE       (UINT)0x00040 // This flag is examined in the
                                                // menu loop code so that we
                                                // exit from menu mode if
                                                // another window was activated
                                                // while we were tracking
                                                // menus. This flag is set
                                                // whenever we activate a new
                                                // window.

#define QF_TABSWITCHING           (UINT)0x00080 // This bit is used as a
                                                // safety check when alt-
                                                // tabbing between apps.  It
                                                // tells us when to expect
                                                // a tab-switch in dwp.c.

#define QF_KEYSTATERESET          (UINT)0x00100
#define QF_INDESTROY              (UINT)0x00200
#define QF_LOCKNOREMOVE           (UINT)0x00400
#define QF_FOCUSNULLSINCEACTIVE   (UINT)0x00800
#define QF_DIALOGACTIVE           (UINT)0x04000
#define QF_EVENTDEACTIVATEREMOVED (UINT)0x08000

#define QF_CAPTURELOCKED             0x00100000
#define QF_ACTIVEWNDTRACKING         0x00200000

/*
 * Constants for Round Frame balloons
 */
#define RNDFRM_CORNER 10
#define RNDFRM_BORDER 3

/*
 * Constants for GetRealClientRect
 */
#define GRC_SCROLLS     0x0001
#define GRC_MINWNDS     0x0002
#define GRC_FULLSCREEN  0x0004

/*
 * Scroll bar info structure
 */
typedef struct tagSBWND {
    WND    wnd;
    BOOL   fVert;
#ifdef COLOR_HOTTRACKING
    int    ht;
#endif // COLOR_HOTTRACKING
    UINT   wDisableFlags;       /* Indicates which arrow is disabled; */
    SBCALC SBCalc;
} SBWND, *PSBWND, *LPSBWND;

//
// Special regions
//
#define HRGN_EMPTY          ((HRGN)0)
#define HRGN_FULL           ((HRGN)1)
#define HRGN_MONITOR        ((HRGN)2)
#define HRGN_SPECIAL_LAST   HRGN_MONITOR

/*
 * SendMsgTimeout client/server transition struct
 */
typedef struct tagSNDMSGTIMEOUT {   /* smto */
    UINT fuFlags;                       // how to send the message, SMTO_BLOCK, SMTO_ABORTIFHUNG
    UINT uTimeout;                      // time-out duration
    ULONG_PTR lSMTOReturn;              // return value TRUE or FALSE
    ULONG_PTR lSMTOResult;              // result value for lpdwResult
} SNDMSGTIMEOUT, *PSNDMSGTIMEOUT;


#ifndef _USERK_
#define ConnectIfNecessary() \
{ \
    if ((NtCurrentTebShared()->Win32ThreadInfo == NULL) \
            && !NtUserGetThreadState(UserThreadConnect)) { \
        return 0; \
    } \
}
#endif

/*
 *  Button data structures (use to be in usercli.h)
 */
typedef struct tagBUTN {
    PWND spwnd;
    UINT buttonState;   // Leave this a word for compatibility with SetWindowWord( 0L )
    HANDLE hFont;
    HANDLE hImage;
    UINT fPaintKbdCuesOnly : 1;
} BUTN, *PBUTN;

typedef struct tagBUTNWND {
    WND wnd;
    PBUTN pbutn;
} BUTNWND, *PBUTNWND;

/*
 * IME control data structures
 */
typedef struct tagIMEUI {
    PWND  spwnd;
    KHIMC hIMC;
    KHWND hwndIMC;
    KHKL  hKL;
    KHWND hwndUI;               // To keep handle for UI window.
    int   nCntInIMEProc;        // Non-zero if hwnd has called into ImeWndProc.
    BOOL  fShowStatus:1;        // TRUE if don't want to show IME's window.
    BOOL  fActivate:1;          // TRUE if hwnd has called into ImeWndProc.
    BOOL  fDestroy:1;           // TRUE if hwnd has called into ImeWndProc.
    BOOL  fDefault:1;           // TRUE if this is the default IME.
    BOOL  fChildThreadDef:1;    // TRUE if this is the default IME which
                                // thread has only child window.
    BOOL  fCtrlShowStatus:1;    // Control status of show status bar.
    BOOL  fFreeActiveEvent:1;   // Control status of show status bar.
} IMEUI, *PIMEUI;

typedef struct tagIMEWND {
    WND wnd;
    PIMEUI pimeui;
} IMEWND, *PIMEWND;

/*
 * SysErrorBox is a 3.1 API that has no 32-bit equivalent.  It's
 * implemented for WOW in harderr.c.
 */
#define MAX_SEB_STYLES  11  /* number of SEB_* values */

/*
 * The next values should be in the same order
 * with the ones in IDOK and STR_OK lists
 */
#define  SEB_OK         0  /* Button with "OK".     */
#define  SEB_CANCEL     1  /* Button with "Cancel"  */
#define  SEB_ABORT      2  /* Button with "&Abort"   */
#define  SEB_RETRY      3  /* Button with "&Retry"   */
#define  SEB_IGNORE     4  /* Button with "&Ignore"  */
#define  SEB_YES        5  /* Button with "&Yes"     */
#define  SEB_NO         6  /* Button with "&No"      */
#define  SEB_CLOSE      7  /* Button with "&Close"   */
#define  SEB_HELP       8  /* Button with "&Help"    */
#define  SEB_TRYAGAIN   9  /* Button with "&Try Again"  */
#define  SEB_CONTINUE   10 /* Button with "&Continue"   */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

typedef struct _MSGBOXDATA {            // mbd
    MSGBOXPARAMS;                       // Must be 1st item in structure
    PWND     pwndOwner;                 // Converted hwndOwner
    WORD     wLanguageId;
    INT    * pidButton;                 // Array of button IDs
    LPWSTR * ppszButtonText;            // Array of button text strings
    UINT     cButtons;                  // Number of buttons
    UINT     DefButton;
    UINT     CancelId;
} MSGBOXDATA, *PMSGBOXDATA, *LPMSGBOXDATA;

LPWSTR MB_GetString(UINT wBtn);
int    SoftModalMessageBox(LPMSGBOXDATA lpmb);

DWORD GetContextHelpId(PWND pwnd);

PITEM  MNLookUpItem(PMENU pMenu, UINT wCmd, BOOL fByPosition, PMENU *ppMenuItemIsOn);
BOOL xxxMNCanClose(PWND pwnd);
PMENU xxxGetSysMenuHandle(PWND pwnd);
PWND    GetPrevPwnd(PWND pwndList, PWND pwndFind);
BOOL   _RegisterServicesProcess(DWORD dwProcessId);

#ifdef _USERK_
#define RTLMENU PMENU
#define xxxRtlSetMenuInfo xxxSetMenuInfo
#define xxxRtlSetMenuItemInfo(rtlMenu, uId, pmii) \
            xxxSetMenuItemInfo(rtlMenu, uId, FALSE, pmii, NULL)
#else
#define RTLMENU HMENU
#define xxxRtlSetMenuInfo NtUserThunkedMenuInfo
#define xxxRtlSetMenuItemInfo(rtlMenu, uId, pmii) \
            NtUserThunkedMenuItemInfo(rtlMenu, uId, FALSE, FALSE, pmii, NULL)
#endif
RTLMENU xxxLoadSysMenu (UINT uMenuId);


BOOL _FChildVisible(PWND pwnd);

#define CH_PREFIX TEXT('&')
//
// Japan support both Kanji and English mnemonic characters,
// toggled from control panel.  Both mnemonics are embedded in menu
// resource templates.  The following prefixes guide their parsing.
//
#define CH_ENGLISHPREFIX 0x1E
#define CH_KANJIPREFIX   0x1F


BOOL RtlWCSMessageWParamCharToMB(DWORD msg, WPARAM *pWParam);
BOOL RtlMBMessageWParamCharToWCS(DWORD msg, WPARAM *pWParam);

VOID RtlInitLargeAnsiString(PLARGE_ANSI_STRING plstr, LPCSTR psz,
        UINT cchLimit);
VOID RtlInitLargeUnicodeString(PLARGE_UNICODE_STRING plstr, LPCWSTR psz,
        UINT cchLimit);

DWORD RtlGetExpWinVer(HANDLE hmod);

/***************************************************************************\
*
* International multi-keyboard layout/font support
*
\***************************************************************************/

#define DT_CHARSETDRAW  1
#define DT_CHARSETINIT  2
#define DT_CHARSETDONE  3
#define DT_GETNEXTWORD  4

typedef void (FAR *LPFNTEXTDRAW)(HDC, int, int, LPWSTR, int, DWORD);

typedef  struct   {
    RECT     rcFormat;          // Format rectangle.
    int      cxTabLength;       // Tab length in pixels.
    int      iXSign;
    int      iYSign;
    int      cyLineHeight;      // Height of a line based on DT_EXTERNALLEADING
    int      cxMaxWidth;        // Width of the format rectangle.
    int      cxMaxExtent;       // Width of the longest line drawn.
    int      cxRightMargin;     // Right margin in pixels (with proper sign)
    LPFNTEXTDRAW  lpfnTextDraw; // pointer to PSTextOut or PSMTextOut based
                                // on DT_NOPREFIX flag.
    int      cxOverhang;        // Character overhang.
    BOOL     bCharsetDll;       // redirect to intl DLL, not textout
    int      iCharset;          // ANSI charset value
} DRAWTEXTDATA, *LPDRAWTEXTDATA;

typedef LONG (*FPLPKTABBEDTEXTOUT)
               (HDC, int, int, LPCWSTR, int, int, CONST INT *, int, BOOL, int, int, int);

typedef void (*FPLPKPSMTEXTOUT)
               (HDC, int, int, LPWSTR, int, DWORD);

typedef int  (*FPLPKDRAWTEXTEX)
               (HDC, int, int, LPCWSTR, int, BOOL, UINT, LPDRAWTEXTDATA, UINT, int);

extern FPLPKTABBEDTEXTOUT fpLpkTabbedTextOut;
extern FPLPKPSMTEXTOUT    fpLpkPSMTextOut;
extern FPLPKDRAWTEXTEX    fpLpkDrawTextEx;


// The number of characters in the ellipsis string (string defined in rtl\drawtext.c).
#define CCHELLIPSIS  3

int DrawTextExWorker(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc,
                     UINT dwDTformat, LPDRAWTEXTPARAMS lpDTparams, int iCharset);


/***************************************************************************\
*
* Language pack edit control callouts.
*
* Functions are accessed through the pLpkEditCallout pointer in the ED
* structure. pLpkEditCallout points to a structure containing a pointer
* to each callout routine. These are typedef'd here.
*
* (In Windows95 this was achieved through a single function pointer
* - lpfnCharset - which was written in assembler and called from over 30
* places with different parameters. Since for NT the Lpk is written in C,
* the ED structure now points to a list of function pointers, each properly
* typedef'd, improving performance, enabling typechecking and avoiding
* varargs discrepancies between architectures.)
*
\***************************************************************************/

typedef struct tagED *PED;

typedef BOOL LpkEditCreate        (PED ped, HWND hWnd);

typedef int  LpkEditIchToXY       (PED ped, HDC hDC, PSTR pText, ICH cch, ICH ichPos);

typedef ICH  LpkEditMouseToIch    (PED ped, HDC hDC, PSTR pText, ICH cch, INT iX);

typedef ICH  LpkEditCchInWidth    (PED ped, HDC hdc, PSTR pText, ICH cch, int width);

typedef INT  LpkEditGetLineWidth  (PED ped, HDC hdc, PSTR pText, ICH cch);

typedef void LpkEditDrawText      (PED ped, HDC hdc, PSTR pText, INT cch, INT iMinSel, INT iMaxSel, INT iY);

typedef BOOL LpkEditHScroll       (PED ped, HDC hdc, PSTR pText);

typedef ICH  LpkEditMoveSelection (PED ped, HDC hdc, PSTR pText, ICH ich, BOOL fLeft);

typedef int  LpkEditVerifyText    (PED ped, HDC hdc, PSTR pText, ICH ichInsert, PSTR pInsertText, ICH cchInsert);

typedef void LpkEditNextWord      (PED ped, HDC hdc, PSTR pText, ICH ichStart, BOOL fLeft, ICH *pichMin, ICH *pichMax);

typedef void LpkEditSetMenu       (PED ped, HMENU hMenu);

typedef int  LpkEditProcessMenu   (PED ped, UINT idMenuItem);

typedef int  LpkEditCreateCaret   (PED ped, HDC hdc, INT nWidth, INT nHeight, UINT hkl);

typedef ICH  LpkEditAdjustCaret   (PED ped, HDC hdc, PSTR pText, ICH ich);


typedef struct tagLPKEDITCALLOUT {
    LpkEditCreate        *EditCreate;
    LpkEditIchToXY       *EditIchToXY;
    LpkEditMouseToIch    *EditMouseToIch;
    LpkEditCchInWidth    *EditCchInWidth;
    LpkEditGetLineWidth  *EditGetLineWidth;
    LpkEditDrawText      *EditDrawText;
    LpkEditHScroll       *EditHScroll;
    LpkEditMoveSelection *EditMoveSelection;
    LpkEditVerifyText    *EditVerifyText;
    LpkEditNextWord      *EditNextWord;
    LpkEditSetMenu       *EditSetMenu;
    LpkEditProcessMenu   *EditProcessMenu;
    LpkEditCreateCaret   *EditCreateCaret;
    LpkEditAdjustCaret   *EditAdjustCaret;
} LPKEDITCALLOUT, *PLPKEDITCALLOUT;

extern PLPKEDITCALLOUT    fpLpkEditControl;

/***************************************************************************\
*
*  Structure for client-side thread-info.
*   dwHookCurrent HIWORD is current hook filter type (eg: WH_GETMESSAGE)
*                 LOWORD is TRUE if current hook is ANSI, FALSE if Unicode
*
\***************************************************************************/


/*
 * Hook thunks.
 */
#ifdef REDIRECTION
LRESULT CALLBACK fnHkINLPPOINT(DWORD nCode,
        WPARAM wParam, LPPOINT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
#endif // REDIRECTION

LRESULT CALLBACK fnHkINLPRECT(DWORD nCode,
        WPARAM wParam, LPRECT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINDWORD(DWORD nCode,
        WPARAM wParam, LPARAM lParam,
        ULONG_PTR xParam, PROC xpfnProc, LPDWORD lpFlags);
LRESULT CALLBACK fnHkINLPMSG(DWORD nCode,
        WPARAM wParam, LPMSG lParam,
        ULONG_PTR xParam, PROC xpfnProc, BOOL bAnsi, LPDWORD lpFlags);
LRESULT CALLBACK fnHkOPTINLPEVENTMSG(DWORD nCode,
        WPARAM wParam, LPEVENTMSGMSG lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINLPDEBUGHOOKSTRUCT(DWORD nCode,
        WPARAM wParam, LPDEBUGHOOKINFO lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINLPMOUSEHOOKSTRUCTEX(DWORD nCode,
        WPARAM wParam, LPMOUSEHOOKSTRUCTEX lParam,
        ULONG_PTR xParam, PROC xpfnProc, LPDWORD lpFlags);
LRESULT CALLBACK fnHkINLPKBDLLHOOKSTRUCT(DWORD nCode,
        WPARAM wParam, LPKBDLLHOOKSTRUCT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINLPMSLLHOOKSTRUCT(DWORD nCode,
        WPARAM wParam, LPMSLLHOOKSTRUCT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINLPCBTACTIVATESTRUCT(DWORD nCode,
        WPARAM wParam, LPCBTACTIVATESTRUCT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
LRESULT CALLBACK fnHkINLPCBTCSTRUCT(UINT msg,
        WPARAM wParam, LPCBT_CREATEWND pcbt,
        PROC xpfnProc, BOOL bAnsi);
LRESULT CALLBACK fnHkINLPCBTMDICCSTRUCT(UINT msg,
        WPARAM wParam, LPCBT_CREATEWND pcbt,
        PROC xpfnProc, BOOL bAnsi);

#ifdef REDIRECTION
LRESULT CALLBACK fnHkINLPHTHOOKSTRUCT(DWORD nCode,
        WPARAM wParam, LPHTHOOKSTRUCT lParam,
        ULONG_PTR xParam, PROC xpfnProc);
#endif // REDIRECTION

/***************************************************************************\
*
* Definitions for client/server-specific data referenced by rtl routines.
*
\***************************************************************************/

extern HBRUSH   ghbrWhite;
extern HBRUSH   ghbrBlack;


ULONG_PTR GetCPD(PVOID pWndOrCls, DWORD options, ULONG_PTR dwData);

BOOL TestWindowProcess(PWND pwnd);
DWORD GetAppCompatFlags(PTHREADINFO pti);
DWORD GetAppCompatFlags2(WORD wVer);
DWORD GetAppImeCompatFlags(PTHREADINFO pti);
PWND _GetDesktopWindow(VOID);
PWND _GetMessageWindow(VOID);

/***************************************************************************\
*
* Shared function prototypes
*
\***************************************************************************/


PVOID FASTCALL HMValidateHandle(HANDLE h, BYTE btype);
PVOID FASTCALL HMValidateCatHandleNoRip(HANDLE h, BYTE btype);
PVOID FASTCALL HMValidateHandleNoRip(HANDLE h, BYTE btype);
KERNEL_PVOID FASTCALL HMValidateHandleNoDesktop(HANDLE h, BYTE btype);
PVOID FASTCALL HMValidateSharedHandle(HANDLE h, BYTE bType);

PVOID FASTCALL HMValidateCatHandleNoSecure(HANDLE h, BYTE bType);
PVOID FASTCALL HMValidateCatHandleNoSecureCCX(HANDLE h, BYTE bType, PCLIENTINFO ccxPci);
PVOID FASTCALL HMValidateHandleNoSecure(HANDLE h, BYTE bType);

ULONG_PTR MapClientNeuterToClientPfn(PCLS pcls, KERNEL_ULONG_PTR dw, BOOL bAnsi);
ULONG_PTR MapServerToClientPfn(KERNEL_ULONG_PTR dw, BOOL bAnsi);

BOOL IsSysFontAndDefaultMode(HDC hdc);

int GetCharDimensions(HDC hDC, TEXTMETRICW *lpTextMetrics, LPINT lpcy);

int   GetWindowBorders(LONG lStyle, DWORD dwExStyle, BOOL fWindow, BOOL fClient);
PWND  SizeBoxHwnd(PWND pwnd);
VOID  _GetClientRect(PWND pwnd, LPRECT prc);

#ifndef _USERSRV_
void GetRealClientRect(PWND pwnd, LPRECT prc, UINT uFlags, PMONITOR pMonitor);
#endif

VOID  _GetWindowRect(PWND pwnd, LPRECT prc);
PWND  _GetLastActivePopup(PWND pwnd);
BOOL  _IsChild(PWND pwndParent, PWND pwnd);
BOOL  _AdjustWindowRectEx(LPRECT lprc, LONG style, BOOL fMenu, DWORD dwExStyle);
BOOL  NeedsWindowEdge(DWORD dwStyle, DWORD dwExStyle, BOOL fNewApp);
VOID  _ClientToScreen(PWND pwnd, PPOINT ppt);
VOID  _ScreenToClient(PWND pwnd, PPOINT ppt);
int   _MapWindowPoints(PWND pwndFrom, PWND pwndTo, LPPOINT lppt, DWORD cpt);
BOOL  _IsWindowVisible(PWND pwnd);
BOOL  _IsDescendant(PWND pwndParent, PWND pwndChild);
BOOL  IsVisible(PWND pwnd);
PWND  _GetWindow(PWND pwnd, UINT cmd);
PWND  _GetParent(PWND pwnd);
int   FindNCHit(PWND pwnd, LONG lPt);
SHORT _GetKeyState(int vk);
PHOOK PhkNextValid(PHOOK phk);

#define GRECT_CLIENT        0x0001
#define GRECT_WINDOW        0x0002
#define GRECT_RECTMASK      0x0003

#define GRECT_CLIENTCOORDS  0x0010
#define GRECT_WINDOWCOORDS  0x0020
#define GRECT_PARENTCOORDS  0x0040
#define GRECT_COORDMASK     0x0070

void GetRect(PWND pwnd, LPRECT lprc, UINT uCoords);

PPROP _FindProp(PWND pwnd, PCWSTR pszKey, BOOL fInternal);
HANDLE _GetProp(PWND pwnd, PCWSTR pszKey, BOOL fInternal);
BOOL _HasCaptionIcon(PWND pwnd);
PWND GetTopLevelWindow(PWND pwnd);

BOOL _SBGetParms(PWND pwnd, int code, PSBDATA pw, LPSCROLLINFO lpsi);
BOOL PSMGetTextExtent(HDC hdc, LPCWSTR lpstr, int cch, PSIZE psize);

LONG   GetPrefixCount(LPCWSTR lpstr, int cb, LPWSTR lpstrCopy, int cbCopy);
PMENU _GetSubMenu(PMENU pMenu, int nPos);
DWORD _GetMenuDefaultItem(PMENU pMenu, BOOL fByPosition, UINT uFlags);
UINT _GetMenuState(PMENU pMenu, UINT wID, UINT dwFlags);

BOOL APIENTRY CopyInflateRect(LPRECT prcDst, CONST RECT *prcSrc, int cx, int cy);
BOOL APIENTRY CopyOffsetRect(LPRECT prcDst, CONST RECT *prcSrc, int cx, int cy);

DWORD FindCharPosition(LPWSTR lpString, WCHAR ch);
LPWSTR  TextAlloc(LPCWSTR lpsz);
UINT  TextCopy(PLARGE_UNICODE_STRING pstr, LPWSTR lpstr, UINT size);
DWORD wcsncpycch(LPWSTR pwsDest, LPCWSTR pwszSrc, DWORD cch);
DWORD strncpycch(LPSTR pszDest, LPCSTR pszSrc, DWORD cch);


#define TextPointer(h) ((LPWSTR)h)

BOOL DrawFrame(HDC hdc, LPRECT prect, int clFrame, int cmd);
void DrawPushButton(HDC hdc, LPRECT lprc, UINT state, UINT flags);
BOOL ClientFrame(HDC hDC, CONST RECT *pRect, HBRUSH hBrush, DWORD patOp);

HBITMAP OwnerLoadBitmap(
    HANDLE hInstLoad,
    LPWSTR lpName,
    HANDLE hOwner);

PCURSOR ClassSetSmallIcon(
    PCLS pcls,
    PCURSOR pcursor,
    BOOL fServerCreated);

#define DO_DROPFILE 0x454C4946L

#define ISTS() (!!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)))

/*
 * Structure for DoConnect system call.
 */
typedef struct _DOCONNECTDATA {
    BOOL   fMouse;
    BOOL   fINetClient;
    BOOL   fInitialProgram;
    BOOL   fHideTitleBar;
    HANDLE IcaVideoChannel;
    HANDLE IcaBeepChannel;
    HANDLE IcaMouseChannel;
    HANDLE IcaKeyboardChannel;
    HANDLE IcaThinwireChannel;
    WCHAR  WinStationName[32];
    WCHAR  ProtocolName[10];
    WCHAR  AudioDriverName[10];
    BOOL   fClientDoubleClickSupport;
    BOOL   fEnableWindowsKey;
    DWORD  drBitsPerPel;
    DWORD  drPelsWidth;
    DWORD  drPelsHeight;
    DWORD  drDisplayFrequency;
    USHORT drProtocolType;
    CLIENTKEYBOARDTYPE  ClientKeyboardType;
} DOCONNECTDATA, *PDOCONNECTDATA;

/*
 * Structure for DoReconnect system call.
 */
typedef struct _DORECONNECTDATA {
    BOOL   fMouse;
    BOOL   fINetClient;
    WCHAR  WinStationName[32];
    BOOL   fClientDoubleClickSupport;
    BOOL   fEnableWindowsKey;
    DWORD  drBitsPerPel;
    DWORD  drPelsWidth;
    DWORD  drPelsHeight;
    DWORD  drDisplayFrequency;
    USHORT drProtocolType;
    BOOL   fChangeDisplaySettings;
    CLIENTKEYBOARDTYPE  ClientKeyboardType;
} DORECONNECTDATA, *PDORECONNECTDATA;

/*
 * EndTask, ExitWindows, hung app, etc time outs
 */
#define CMSSLEEP                250
#define CMSHUNGAPPTIMEOUT       (5 * 1000)
#define CMSHUNGTOKILLCOUNT       4
#define CMSWAITTOKILLTIMEOUT    (CMSHUNGTOKILLCOUNT * CMSHUNGAPPTIMEOUT)
#define CMSAPPSTARTINGTIMEOUT   (6 * CMSHUNGAPPTIMEOUT) /* Some setup apps are pretty slow. bug 195832 */
#define CMS_QANIMATION          165
#define CMS_FLASHWND            500
#define CMS_MENUFADE            175
#define CMS_SELECTIONFADE       350
#define CMS_TOOLTIP             135
#define PROCESSTERMINATETIMEOUT (90 * 1000)

void KernelBP(void);

/*
 * Message table definitions
 */
typedef struct tagMSG_TABLE_ENTRY {
    BYTE iFunction:6;
    BYTE bThunkMessage:1;
    BYTE bSyncOnlyMessage:1;
} MSG_TABLE_ENTRY;

extern CONST MSG_TABLE_ENTRY MessageTable[];

#define TESTSYNCONLYMESSAGE(msg, wParam) (((msg) < WM_USER) ?       \
        (   (MessageTable[msg].bSyncOnlyMessage) ||                 \
            (((msg) == WM_DEVICECHANGE) && ((wParam) & 0x8000))) :  \
        0)


/*
 * Drag and Drop menus.
 * MNDragOver output info
 */
typedef struct tagMNDRAGOVERINFO
{
    DWORD dwFlags;
    HMENU hmenu;
    UINT uItemIndex;
    HWND hwndNotify;
} MNDRAGOVERINFO, * PMNDRAGOVERINFO;

#ifdef _USERK_
typedef struct tagMOUSECURSOR {
    BYTE bAccelTableLen;
    BYTE bAccelTable[128];
    BYTE bConstantTableLen;
    BYTE bConstantTable[128];
} MOUSECURSOR;
#endif

typedef struct tagINTERNALSETHIGHCONTRAST {
    UINT    cbSize;
    DWORD   dwFlags;
    UNICODE_STRING usDefaultScheme;
} INTERNALSETHIGHCONTRAST, *LPINTERNALSETHIGHCONTRAST;


#endif // _USER_
