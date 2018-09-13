/****************************************************************************
 *
 *  Win32.h
 *
 *  Windows 16/32 porting helper file
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef INC_OLE2
#define INC_OLE2
#endif

#ifndef INC_WIN32_H
#define INC_WIN32_H  // Protect against double inclusion of this file

#if !defined(WIN32) && defined(_WIN32)
    #define WIN32
#endif

#if !defined(DEBUG) && defined(_DEBUG)
    #define DEBUG
#endif


#ifdef WIN32

// Set up a single define to allow CHICAGO or NT (Daytona)
#ifdef UNICODE // Could use WINVER <= 0x400 ...
    #define DAYTONA
    #undef CHICAGO
#else
    #ifndef CHICAGO
        #define CHICAGO
    #endif
    #undef  DAYTONA
#endif // UNICODE

#ifndef RC_INVOKED
    #pragma warning(disable:4103)
#endif
    #define	_INC_OLE

#else  // 16 bit compilation - must be Chicago

    #ifndef CHICAGO
    #define	CHICAGO
    #endif

#endif  // WIN32

#if !defined(_INC_WINDOWS) || !defined(_WINDOWS)
#include <windows.h>
#include <windowsx.h>
#if WINVER >= 0x400
#include <winerror.h>
#endif
#include <mmsystem.h>
#endif // INC_WINDOWS...

#ifndef EXTERN_C
#ifdef __cplusplus
	#define EXTERN_C extern "C"
#else
	#define EXTERN_C extern
#endif
#endif

// Not just for Daytona, things like mciavi define WINVER=0x30a
#ifndef WS_EX_RIGHT
        #define WS_EX_RIGHT             0x00001000L     // ;Internal 4.0
        #define WS_EX_LEFT              0x00000000L     // ;Internal 4.0
        #define WS_EX_RTLREADING        0x00002000L     // ;Internal 4.0
        #define WS_EX_LTRREADING        0x00000000L     // ;Internal 4.0
        #define WS_EX_LEFTSCROLLBAR     0x00004000L     // ;Internal 4.0
        #define WS_EX_RIGHTSCROLLBAR    0x00000000L     // ;Internal 4.0
#endif

// Win 16 and Win 32 use different macros to turn code debugging on/off
// Map the 16 bit version to the NT conventions.
// In particular on NT, debug builds are identified by DBG==1  (and retail
// with DBG==0), hence the NT only source uses #if DBG.  Note: #ifdef DBG
// is ALWAYS true on NT.
// Chicago (and VFW 16 bit code) uses #ifdef DEBUG.  The complex of
// instructions below should ensure that whether this is Chicago or NT that
//    a debug build implies DEBUG defined and DBG==1
//    a retail build implies DEBUG NOT defined, and DBG==0

#ifdef WIN32
#ifndef DBG
#ifdef DEBUG
    #define DBG 1
#else
    #define DBG 0
#endif
#endif

#undef DEBUG

#if DBG
    #define DEBUG
    #define STATICFN
    #define STATICDT
#else
    #define STATICFN static
    #define STATICDT static
#endif

#else    // !WIN32
    #define STATICFN static
    #define STATICDT static
#endif //WIN32

#define FASTCALL  __fastcall
#define _FASTCALL __fastcall

#if !defined NUMELMS
#define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif


#ifdef WIN32

/* --- Win32 version --------------------------------------------------- */

        #include <string.h>
        #include <memory.h>

        #undef CDECL
#ifdef MIPS
	#define __stdcall
	#define _stdcall
	#define _cdecl
	#define __cdecl
        #define CDECL
#else
        #define CDECL _cdecl
#endif

    //typedef BITMAPINFOHEADER FAR *LPBITMAPINFOHEADER;

	#define far
	#define _far
	#define __far
	#define HUGE_T
	#define HUGE
	#define huge
	#define _huge
	#define __huge
	#define near
	#define _near
        #define __near
        #define _fastcall
        #define __fastcall
	#define _loadds
	#define __loadds
	#define _LOADDS
	#define LOADDS
        #define _export     //should _export be blank?
	#define __export
	#define EXPORT
        #define _based(x)
	#define __based(x)
        #define _based32(x)
        #define __based32(x)

        #ifdef _X86_
        // __inline provides speed improvements for x86 platforms.  Unfortunately
        // the MIPS compiler does not have inline support.  Alpha is unknown, so
        // we do not assume and play it safe.
        #define INLINE   __inline
        #define inline   __inline
        #define _inline  __inline
        #else
        #define INLINE
        #define inline
        #define _inline
        #define __inline
        #endif

	//typedef RGBQUAD FAR *LPRGBQUAD;

        #ifdef DAYTONA
        typedef LRESULT (*DRIVERPROC)(DWORD, HDRVR, UINT, WPARAM, LPARAM);
        #endif

        #define GetCurrentTask()    (HTASK)GetCurrentThreadId()

        #define WF_PMODE        0x0001
        #define WF_CPU286       0x0002
        #define WF_CPU386       0x0004
        #define WF_CPU486       0x0008
        #define WF_80x87        0x0400
        #define WF_PAGING       0x0800

        #define GetWinFlags()   (WF_PMODE|WF_CPU486|WF_PAGING|WF_80x87)
	
        //#define hmemcpy  memcpy
        #define _fmemcpy memcpy
        #define _fmemset memset
	//#define lstrcpyn(dest, source, cb)  ( strncpy(dest, source, cb), ((char *) dest) [cb - 1] = 0)

	//!!! should use LARGE_INTEGER stuff.
	#define muldiv32                MulDiv
	#define muldivru32(a,b,c)       (long)(((double)(a) * (double)(b) + (double)((c)-1)) / (double)(c))
	#define muldivrd32(a,b,c)       (long)(((double)(a) * (double)(b)) / (double)(c))

	#define IsTask(x) ((x) != 0)
	#define IsGDIObject(obj) (GetObjectType((HGDIOBJ)(obj)) != 0)
	
	#define SZCODEA CHAR
	#define SZCODE  TCHAR
        typedef TCHAR * NPTSTR;

#ifndef UNICODE
// !!!!!!! need lstrcpyW, lstrlenW, wsprintfW, lstrcpynW for Chicago!
#endif
	
#else

/* --- Win16 version --------------------------------------------------- */

        #include <string.h>
        #include <memory.h>

	#define SZCODEA SZCODE
        typedef char  TCHAR;
        typedef WORD  WCHAR;
        typedef NPSTR NPTSTR;
        typedef LPSTR LPTSTR;
	typedef LPSTR LPOLESTR;
	typedef LPCSTR LPCOLESTR;
	typedef char  OLECHAR;
	
	typedef int   INT;

	#define _LOADDS _loadds
	#define LOADDS	_loadds
	#define EXPORT	_export

	#define HUGE	_huge

        #define DRIVERS_SECTION "Drivers"
        #define MCI_SECTION "MCI"
        #define MCIAVI_SECTION "MCIAVI"
	#define TEXT(sz) sz

	// stuff in muldiv32.asm
	EXTERN_C  LONG FAR PASCAL muldiv32(LONG,LONG,LONG);
	EXTERN_C  LONG FAR PASCAL muldivru32(LONG,LONG,LONG);
        EXTERN_C  LONG FAR PASCAL muldivrd32(LONG,LONG,LONG);

        #define UNALIGNED
        #define INLINE __inline
        #define CharPrev AnsiPrev

        /*
         * define these so we can explicitly use Ansi or Unicode versions
         * in the NT code, and the standard entry point for Win16.
         */
        #define SetWindowTextA                  SetWindowText
        #define GetProfileStringA               GetProfileString
        #define GetPrivateProfileStringA        GetPrivateProfileString
        #define GetProfileIntA                  GetProfileInt
        #define GetModuleHandleA                GetModuleHandle
        #define GetModuleFileNameA              GetModuleFileName
        #define wvsprintfA                      wvsprintf
        #define wsprintfA                       wsprintf
        #define lstrcmpA                        lstrcmp
        #define lstrcmpiA                       lstrcmpi
        #define lstrcpyA                        lstrcpy
        #define lstrcatA                        lstrcat
        #define lstrlenA                        lstrlen
        #define LoadStringA                     LoadString
        #define LoadStringW                     LoadString
        #define OutputDebugStringA              OutputDebugString
        #define MessageBoxA                     MessageBox

	// Needed for writing OLE-style code that builds 16/32....
	#define lstrcpyW lstrcpy
	#define lstrcpynW lstrcpyn
	#define lstrlenW lstrlen

	#define SZCODE char _based(_segname("_CODE"))
	#define LPCWSTR      LPCSTR
	#define LPCTSTR      LPCSTR
	#define LPWSTR       LPSTR
        #define PTSTR        PSTR

/****** Alternate porting layer macros ****************************************/

#ifndef GET_WPARAM

    /* USER MESSAGES: */

    #define GET_WPARAM(wp, lp)                      (wp)
    #define GET_LPARAM(wp, lp)                      (lp)

    #define GET_WM_ACTIVATE_STATE(wp, lp)               (wp)
    #define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)          (BOOL)HIWORD(lp)
    #define GET_WM_ACTIVATE_HWND(wp, lp)                (HWND)LOWORD(lp)
    #define GET_WM_ACTIVATE_MPS(s, fmin, hwnd)   \
            (WPARAM)(s), MAKELONG(hwnd, fmin)

    #define GET_WM_CHARTOITEM_CHAR(wp, lp)              (CHAR)(wp)
    #define GET_WM_CHARTOITEM_POS(wp, lp)               HIWORD(lp)
    #define GET_WM_CHARTOITEM_HWND(wp, lp)              (HWND)LOWORD(lp)
    #define GET_WM_CHARTOITEM_MPS(ch, pos, hwnd) \
            (WPARAM)(ch), MAKELONG(hwnd, pos)

    #define GET_WM_COMMAND_ID(wp, lp)                   (wp)
    #define GET_WM_COMMAND_HWND(wp, lp)                 (HWND)LOWORD(lp)
    #define GET_WM_COMMAND_CMD(wp, lp)                  HIWORD(lp)
    #define GET_WM_COMMAND_MPS(id, hwnd, cmd)    \
            (WPARAM)(id), MAKELONG(hwnd, cmd)

    #define WM_CTLCOLORMSGBOX       0x0132
    #define WM_CTLCOLOREDIT         0x0133
    #define WM_CTLCOLORLISTBOX      0x0134
    #define WM_CTLCOLORBTN          0x0135
    #define WM_CTLCOLORDLG          0x0136
    #define WM_CTLCOLORSCROLLBAR    0x0137
    #define WM_CTLCOLORSTATIC       0x0138

    #define GET_WM_CTLCOLOR_HDC(wp, lp, msg)            (HDC)(wp)
    #define GET_WM_CTLCOLOR_HWND(wp, lp, msg)           (HWND)LOWORD(lp)
    #define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)           HIWORD(lp)
    #define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type) \
            (WPARAM)(hdc), MAKELONG(hwnd, type)


    #define GET_WM_MENUSELECT_CMD(wp, lp)               (wp)
    #define GET_WM_MENUSELECT_FLAGS(wp, lp)             LOWORD(lp)
    #define GET_WM_MENUSELECT_HMENU(wp, lp)             (HMENU)HIWORD(lp)
    #define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)  \
            (WPARAM)(cmd), MAKELONG(f, hmenu)

    // Note: the following are for interpreting MDIclient to MDI child messages.
    #define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (BOOL)(wp)
    #define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)HIWORD(lp)
    #define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)LOWORD(lp)
    // Note: the following is for sending to the MDI client window.
    #define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA)\
            (WPARAM)(hwndA), 0

    #define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW) 0, MAKELONG(hmenuF, hmenuW)

    #define GET_WM_MENUCHAR_CHAR(wp, lp)                (CHAR)(wp)
    #define GET_WM_MENUCHAR_HMENU(wp, lp)               (HMENU)LOWORD(lp)
    #define GET_WM_MENUCHAR_FMENU(wp, lp)               (BOOL)HIWORD(lp)
    #define GET_WM_MENUCHAR_MPS(ch, hmenu, f)    \
            (WPARAM)(ch), MAKELONG(hmenu, f)

    #define GET_WM_PARENTNOTIFY_MSG(wp, lp)             (wp)
    #define GET_WM_PARENTNOTIFY_ID(wp, lp)              HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)       (HWND)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_X(wp, lp)               (INT)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_Y(wp, lp)               (INT)HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd) \
            (WPARAM)(msg), MAKELONG(hwnd, id)
    #define GET_WM_PARENTNOTIFY2_MPS(msg, x, y) \
            (WPARAM)(msg), MAKELONG(x, y)

    #define GET_WM_VKEYTOITEM_CODE(wp, lp)              (wp)
    #define GET_WM_VKEYTOITEM_ITEM(wp, lp)              (INT)HIWORD(lp)
    #define GET_WM_VKEYTOITEM_HWND(wp, lp)              (HWND)LOWORD(lp)
    #define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) \
            (WPARAM)(code), MAKELONG(hwnd, item)

    #define GET_EM_SETSEL_START(wp, lp)                 LOWORD(lp)
    #define GET_EM_SETSEL_END(wp, lp)                   HIWORD(lp)
    #define GET_EM_SETSEL_MPS(iStart, iEnd) \
            0, MAKELONG(iStart, iEnd)

    #define GET_EM_LINESCROLL_MPS(vert, horz)     \
            0, MAKELONG(vert, horz)

    #define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)       (HWND)LOWORD(lp)

    #define GET_WM_HSCROLL_CODE(wp, lp)                 (wp)
    #define GET_WM_HSCROLL_POS(wp, lp)                  LOWORD(lp)
    #define GET_WM_HSCROLL_HWND(wp, lp)                 (HWND)HIWORD(lp)
    #define GET_WM_HSCROLL_MPS(code, pos, hwnd)    \
            (WPARAM)(code), MAKELONG(pos, hwnd)

    #define GET_WM_VSCROLL_CODE(wp, lp)                 (wp)
    #define GET_WM_VSCROLL_POS(wp, lp)                  LOWORD(lp)
    #define GET_WM_VSCROLL_HWND(wp, lp)                 (HWND)HIWORD(lp)
    #define GET_WM_VSCROLL_MPS(code, pos, hwnd)    \
            (WPARAM)(code), MAKELONG(pos, hwnd)

#endif  // !GET_WPARAM

#endif  // !WIN32

#endif
