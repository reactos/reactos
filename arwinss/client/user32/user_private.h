/*
 * USER private definitions
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_USER_PRIVATE_H
#define __WINE_USER_PRIVATE_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include <windowsx.h>
#include "wine/winternl.h"

#define GET_WORD(ptr)  (*(const WORD *)(ptr))
#define GET_DWORD(ptr) (*(const DWORD *)(ptr))

#ifndef __REACTOS__
#define WM_SYSTIMER	    0x0118
#define WM_POPUPSYSTEMMENU  0x0313
#else
// Hack to silence IDC_ARROW cast errors
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE(i) ((ULONG_PTR)((WORD)(i)))
#endif

/* internal messages codes */
enum wine_internal_message
{
    WM_WINE_DESTROYWINDOW = 0x80000000,
    WM_WINE_SETWINDOWPOS,
    WM_WINE_SHOWWINDOW,
    WM_WINE_SETPARENT,
    WM_WINE_SETWINDOWLONG,
    WM_WINE_ENABLEWINDOW,
    WM_WINE_SETACTIVEWINDOW,
    WM_WINE_KEYBOARD_LL_HOOK,
    WM_WINE_MOUSE_LL_HOOK,
    WM_WINE_FIRST_DRIVER_MSG = 0x80001000,  /* range of messages reserved for the USER driver */
    WM_WINE_LAST_DRIVER_MSG = 0x80001fff
};

typedef struct tagUSER_DRIVER {
    /* keyboard functions */
    HKL    (CDECL *pActivateKeyboardLayout)(HKL, UINT);
    void   (CDECL *pBeep)(void);
    SHORT  (CDECL *pGetAsyncKeyState)(INT);
    INT    (CDECL *pGetKeyNameText)(LONG, LPWSTR, INT);
    HKL    (CDECL *pGetKeyboardLayout)(DWORD);
    BOOL   (CDECL *pGetKeyboardLayoutName)(LPWSTR);
    HKL    (CDECL *pLoadKeyboardLayout)(LPCWSTR, UINT);
    UINT   (CDECL *pMapVirtualKeyEx)(UINT, UINT, HKL);
    UINT   (CDECL *pSendInput)(UINT, LPINPUT, int);
    INT    (CDECL *pToUnicodeEx)(UINT, UINT, const BYTE *, LPWSTR, int, UINT, HKL);
    BOOL   (CDECL *pUnloadKeyboardLayout)(HKL);
    SHORT  (CDECL *pVkKeyScanEx)(WCHAR, HKL);
    /* cursor/icon functions */
    void   (CDECL *pCreateCursorIcon)(HCURSOR);
    void   (CDECL *pDestroyCursorIcon)(HCURSOR);
    void   (CDECL *pSetCursor)(HCURSOR);
    BOOL   (CDECL *pGetCursorPos)(LPPOINT);
    BOOL   (CDECL *pSetCursorPos)(INT,INT);
    BOOL   (CDECL *pClipCursor)(LPCRECT);
    /* screen saver functions */
    BOOL   (CDECL *pGetScreenSaveActive)(void);
    void   (CDECL *pSetScreenSaveActive)(BOOL);
    /* clipboard functions */
    INT    (CDECL *pAcquireClipboard)(HWND);                     /* Acquire selection */
    BOOL   (CDECL *pCountClipboardFormats)(void);                /* Count available clipboard formats */
    void   (CDECL *pEmptyClipboard)(BOOL);                       /* Empty clipboard data */
    void   (CDECL *pEndClipboardUpdate)(void);                   /* End clipboard update */
    UINT   (CDECL *pEnumClipboardFormats)(UINT);                 /* Enumerate clipboard formats */
    HANDLE (CDECL *pGetClipboardData)(UINT);                     /* Get specified selection data */
    INT    (CDECL *pGetClipboardFormatName)(UINT, LPWSTR, UINT); /* Get a clipboard format name */
    BOOL   (CDECL *pIsClipboardFormatAvailable)(UINT);           /* Check if specified format is available */
    UINT   (CDECL *pRegisterClipboardFormat)(LPCWSTR);           /* Register a clipboard format */
    BOOL   (CDECL *pSetClipboardData)(UINT, HANDLE, BOOL);       /* Set specified selection data */
    /* display modes */
    LONG   (CDECL *pChangeDisplaySettingsEx)(LPCWSTR,LPDEVMODEW,HWND,DWORD,LPVOID);
    BOOL   (CDECL *pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
    BOOL   (CDECL *pEnumDisplaySettingsEx)(LPCWSTR,DWORD,LPDEVMODEW,DWORD);
    BOOL   (CDECL *pGetMonitorInfo)(HMONITOR,MONITORINFO*);
    /* windowing functions */
    BOOL   (CDECL *pCreateDesktopWindow)(HWND);
    BOOL   (CDECL *pCreateWindow)(HWND);
    void   (CDECL *pDestroyWindow)(HWND);
    void   (CDECL *pGetDC)(HDC,HWND,HWND,const RECT *,const RECT *,DWORD);
    DWORD  (CDECL *pMsgWaitForMultipleObjectsEx)(DWORD,const HANDLE*,DWORD,DWORD,DWORD);
    void   (CDECL *pReleaseDC)(HWND,HDC);
    BOOL   (CDECL *pScrollDC)(HDC, INT, INT, const RECT *, const RECT *, HRGN, LPRECT);
    void   (CDECL *pSetCapture)(HWND,UINT);
    void   (CDECL *pSetFocus)(HWND);
    void   (CDECL *pSetLayeredWindowAttributes)(HWND,COLORREF,BYTE,DWORD);
    void   (CDECL *pSetParent)(HWND,HWND,HWND);
    int    (CDECL *pSetWindowRgn)(HWND,HRGN,BOOL);
    void   (CDECL *pSetWindowIcon)(HWND,UINT,HICON);
    void   (CDECL *pSetWindowStyle)(HWND,INT,STYLESTRUCT*);
    void   (CDECL *pSetWindowText)(HWND,LPCWSTR);
    UINT   (CDECL *pShowWindow)(HWND,INT,RECT*,UINT);
    LRESULT (CDECL *pSysCommand)(HWND,WPARAM,LPARAM);
    LRESULT (CDECL *pWindowMessage)(HWND,UINT,WPARAM,LPARAM);
    void   (CDECL *pWindowPosChanging)(HWND,HWND,UINT,const RECT *,const RECT *,RECT *);
    void   (CDECL *pWindowPosChanged)(HWND,HWND,UINT,const RECT *,const RECT *,const RECT *,const RECT *);
} USER_DRIVER;

extern const USER_DRIVER *USER_Driver DECLSPEC_HIDDEN;

extern void USER_unload_driver(void) DECLSPEC_HIDDEN;

struct received_message_info;

enum user_obj_type
{
    USER_WINDOW = 1,  /* window */
    USER_MENU,        /* menu */
    USER_ACCEL,       /* accelerator */
    USER_ICON,        /* icon or cursor */
    USER_DWP          /* DeferWindowPos structure */
};

struct user_object
{
    HANDLE             handle;
    enum user_obj_type type;
};

#define OBJ_OTHER_PROCESS ((void *)1)  /* returned by get_user_handle_ptr on unknown handles */

HANDLE alloc_user_handle( struct user_object *ptr, enum user_obj_type type ) DECLSPEC_HIDDEN;
void *get_user_handle_ptr( HANDLE handle, enum user_obj_type type ) DECLSPEC_HIDDEN;
void release_user_handle_ptr( void *ptr ) DECLSPEC_HIDDEN;
void *free_user_handle( HANDLE handle, enum user_obj_type type ) DECLSPEC_HIDDEN;

/* type of message-sending functions that need special WM_CHAR handling */
enum wm_char_mapping
{
    WMCHAR_MAP_POSTMESSAGE,
    WMCHAR_MAP_SENDMESSAGE,
    WMCHAR_MAP_SENDMESSAGETIMEOUT,
    WMCHAR_MAP_RECVMESSAGE,
    WMCHAR_MAP_DISPATCHMESSAGE,
    WMCHAR_MAP_CALLWINDOWPROC,
    WMCHAR_MAP_COUNT,
    WMCHAR_MAP_NOMAPPING = WMCHAR_MAP_COUNT
};

/* data to store state for A/W mappings of WM_CHAR */
struct wm_char_mapping_data
{
    BYTE lead_byte[WMCHAR_MAP_COUNT];
    MSG  get_msg;
};

/* this is the structure stored in TEB->Win32ClientInfo */
/* no attempt is made to keep the layout compatible with the Windows one */
struct user_thread_info
{
    HANDLE                        server_queue;           /* Handle to server-side queue */
    DWORD                         recursion_count;        /* SendMessage recursion counter */
    BOOL                          hook_unicode;           /* Is current hook unicode? */
    HHOOK                         hook;                   /* Current hook */
    struct received_message_info *receive_info;           /* Message being currently received */
    struct wm_char_mapping_data  *wmchar_data;            /* Data for WM_CHAR mappings */
    DWORD                         GetMessageTimeVal;      /* Value for GetMessageTime */
    DWORD                         GetMessagePosVal;       /* Value for GetMessagePos */
    ULONG_PTR                     GetMessageExtraInfoVal; /* Value for GetMessageExtraInfo */
    UINT                          active_hooks;           /* Bitmap of active hooks */
    HWND                          top_window;             /* Desktop window */
    HWND                          msg_window;             /* HWND_MESSAGE parent window */

    ULONG                         pad[11];                /* Available for more data */
};

struct hook_extra_info
{
    HHOOK handle;
    LPARAM lparam;
};

static inline struct user_thread_info *get_user_thread_info(void)
{
    return (struct user_thread_info *)NtCurrentTeb()->Win32ClientInfo;
}

/* check if hwnd is a broadcast magic handle */
static inline BOOL is_broadcast( HWND hwnd )
{
    return (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST);
}

extern HMODULE user32_module DECLSPEC_HIDDEN;
extern HBRUSH SYSCOLOR_55AABrush DECLSPEC_HIDDEN;

struct dce;

extern BOOL CLIPBOARD_ReleaseOwner(void) DECLSPEC_HIDDEN;
extern BOOL FOCUS_MouseActivate( HWND hwnd ) DECLSPEC_HIDDEN;
extern BOOL set_capture_window( HWND hwnd, UINT gui_flags, HWND *prev_ret );
extern void free_dce( struct dce *dce, HWND hwnd ) DECLSPEC_HIDDEN;
extern void invalidate_dce( HWND hwnd, const RECT *rect ) DECLSPEC_HIDDEN;
extern void erase_now( HWND hwnd, UINT rdw_flags ) DECLSPEC_HIDDEN;
extern void *get_hook_proc( void *proc, const WCHAR *module );
extern LRESULT call_current_hook( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam ) DECLSPEC_HIDDEN;
extern BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;
extern LRESULT MSG_SendInternalMessageTimeout( DWORD dest_pid, DWORD dest_tid,
                                               UINT msg, WPARAM wparam, LPARAM lparam,
                                               UINT flags, UINT timeout, PDWORD_PTR res_ptr ) DECLSPEC_HIDDEN;
extern HPEN SYSCOLOR_GetPen( INT index ) DECLSPEC_HIDDEN;
extern void SYSPARAMS_Init(void) DECLSPEC_HIDDEN;
extern void USER_CheckNotLock(void) DECLSPEC_HIDDEN;
extern BOOL USER_IsExitingThread( DWORD tid ) DECLSPEC_HIDDEN;

extern BOOL USER_SetWindowPos( WINDOWPOS * winpos ) DECLSPEC_HIDDEN;

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );

extern WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode ) DECLSPEC_HIDDEN;
extern WNDPROC WINPROC_AllocProc( WNDPROC func, BOOL unicode ) DECLSPEC_HIDDEN;
extern BOOL WINPROC_IsUnicode( WNDPROC proc, BOOL def_val ) DECLSPEC_HIDDEN;

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg,
                                     enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;

extern INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern BOOL WINPROC_call_window( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                 LRESULT *result, BOOL unicode, enum wm_char_mapping mapping ) DECLSPEC_HIDDEN;

/* message spy definitions */

#define SPY_DISPATCHMESSAGE       0x0100
#define SPY_SENDMESSAGE           0x0101
#define SPY_DEFWNDPROC            0x0102

#define SPY_RESULT_OK             0x0001
#define SPY_RESULT_DEFWND         0x0002

extern const char *SPY_GetClassLongOffsetName( INT offset ) DECLSPEC_HIDDEN;
extern const char *SPY_GetMsgName( UINT msg, HWND hWnd ) DECLSPEC_HIDDEN;
extern const char *SPY_GetVKeyName(WPARAM wParam) DECLSPEC_HIDDEN;
extern void SPY_EnterMessage( INT iFlag, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern void SPY_ExitMessage( INT iFlag, HWND hwnd, UINT msg,
                             LRESULT lReturn, WPARAM wParam, LPARAM lParam ) DECLSPEC_HIDDEN;
extern int SPY_Init(void) DECLSPEC_HIDDEN;

extern ATOM UserAddAtomA(LPCSTR lpString);
extern ATOM UserAddAtomW(LPCWSTR lpString);
extern ATOM UserDeleteAtom(ATOM nAtom);
extern ATOM UserFindAtomA(LPCSTR lpString);
extern ATOM UserFindAtomW(LPCWSTR lpString);
extern UINT UserGetAtomNameA(ATOM nAtom, LPSTR lpBuffer, int nSize);
extern UINT UserGetAtomNameW(ATOM nAtom, LPWSTR lpBuffer, int nSize);

#include "pshpack1.h"

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
} ICONRESDIR;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    { ICONRESDIR icon;
      CURSORDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1];
} CURSORICONDIR;

#include "poppack.h"

extern BOOL get_icon_size( HICON handle, SIZE *size ) DECLSPEC_HIDDEN;

NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
);

#ifdef assert
#undef assert
#endif
#define assert(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }

#ifndef ASSERT
#define ASSERT(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }
#endif

extern void CDECL __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set );
extern void CDECL __wine_set_visible_region( HDC hdc, HRGN hrgn, const RECT *vis_rect );

#endif /* __WINE_USER_PRIVATE_H */
