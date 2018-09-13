// Copyright (c) 1996-1999 Microsoft Corporation

// ===========================================================================
// File: O L E A C C _ P. H
// 
//  Constants, Definitions, Types, and Classes private to the OLEACC
// implementation. This header file is part of the OLEACC project. OLEACC.H
// (included here) is machine-generated from OLEACC.IDL via the MIDL compiler.
// 
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
// ===========================================================================
#define INC_OLE2

#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional

#include <windows.h>

#if (_WIN32_WINNT < 0x0403)		// on Win95 compile, we need stuff in winable.h and userole.h,
#include <winable.h>			// but for NT build, this is included in winuserp.h
#include <userole.h>			// TODO? Change win95 to be more like NT to keep in ssync 
#else							// more easily!
#include <winuserp.h>
#endif

#include <limits.h>
#include "oleacc.h"
#include "strtable.h"

//
// Constants
//
#define HEAP_SHARED     0x04000000      // Win95 only
#define HEAP_GLOBAL     0x80000000      // Win95 only

const LARGE_INTEGER bZero = {0,0};

#define ARRAYSIZE(n)    (sizeof(n)/sizeof(n[0]))


#ifdef _DEBUG

// Having this as an inline avoids the "const expr in conditional" warning
// Use of template avoids cast problems if 'BOOL foo' used instead
template<class T> inline
void Assert( T foo )
{
    if( ! foo )
        DebugBreak();
}

// AssertStr used for unconditional break - eg.  AssertStr("Shouldn't get here");
#define AssertStr(str)  DebugBreak()

#else
#define Assert(foo)
#define AssertStr(str)
#endif


//-----------------------------------------------------------------
// [v-jaycl, 4/1/97] Number of registered handlers to support.
//	  TODO: This should be dynamic and not hard-wired.
//-----------------------------------------------------------------
#define	TOTAL_REG_HANDLERS			100

typedef BOOL (STDAPICALLTYPE *LPFNGETGUITHREADINFO)(DWORD, PGUITHREADINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETCURSORINFO)(LPCURSORINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETWINDOWINFO)(HWND, LPWINDOWINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETTITLEBARINFO)(HWND, LPTITLEBARINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETSCROLLBARINFO)(HWND, LONG, LPSCROLLBARINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETCOMBOBOXINFO)(HWND, LPCOMBOBOXINFO);
typedef HWND (STDAPICALLTYPE *LPFNGETANCESTOR)(HWND, UINT);
typedef HWND (STDAPICALLTYPE *LPFNREALCHILDWINDOWFROMPOINT)(HWND, POINT);
typedef UINT (STDAPICALLTYPE *LPFNREALGETWINDOWCLASS)(HWND, LPTSTR, UINT);
typedef BOOL (STDAPICALLTYPE *LPFNGETALTTABINFO)(HWND, int, LPALTTABINFO, LPTSTR, UINT);
typedef BOOL (STDAPICALLTYPE *LPFNGETMENUBARINFO)(HWND, LONG, LONG, LPMENUBARINFO);
typedef DWORD (STDAPICALLTYPE* LPFNGETLISTBOXINFO)(HWND);
typedef BOOL (STDAPICALLTYPE *LPFNSENDINPUT)(UINT, LPINPUT, INT);
typedef BOOL (STDAPICALLTYPE *LPFNBLOCKINPUT)(BOOL);
typedef LPVOID (STDAPICALLTYPE* LPFNMAPLS)(LPVOID);
typedef VOID (STDAPICALLTYPE* LPFNUNMAPLS)(LPVOID);
typedef DWORD (STDAPICALLTYPE* LPFNGETMODULEFILENAME)(HMODULE,LPTSTR,DWORD); 
typedef PVOID (STDAPICALLTYPE* LPFNINTERLOCKCMPEXCH)(PVOID *,PVOID,PVOID);
typedef LPVOID (STDAPICALLTYPE* LPFNVIRTUALALLOCEX)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
typedef BOOL (STDAPICALLTYPE* LPFNVIRTUALFREEEX)(HANDLE,LPVOID,DWORD,DWORD);


//
// Variables
//
extern HINSTANCE    hinstDll;       // instance of the OLEACC library
extern HINSTANCE	hinstResDll;	// instance of the resource library
#ifdef _X86_ 
extern HANDLE       hheapShared;    // handle to the shared heap (Windows '95 only)
extern BOOL         fWindows95;     // running on Windows '95?
#endif // _X86_
extern BOOL         fCreateDefObjs; // running with new USER32?
extern LPFNGETGUITHREADINFO     lpfnGuiThreadInfo;
extern LPFNGETCURSORINFO        lpfnCursorInfo;
extern LPFNGETWINDOWINFO        lpfnWindowInfo;
extern LPFNGETTITLEBARINFO      lpfnTitleBarInfo;
extern LPFNGETSCROLLBARINFO     lpfnScrollBarInfo;
extern LPFNGETCOMBOBOXINFO      lpfnComboBoxInfo;
extern LPFNGETANCESTOR          lpfnGetAncestor;
extern LPFNREALCHILDWINDOWFROMPOINT lpfnRealChildWindowFromPoint;
extern LPFNREALGETWINDOWCLASS   lpfnRealGetWindowClass;
extern LPFNGETALTTABINFO        lpfnAltTabInfo;
extern LPFNGETMENUBARINFO       lpfnMenuBarInfo;
extern LPFNGETLISTBOXINFO       lpfnGetListBoxInfo;
extern LPFNSENDINPUT            lpfnSendInput;
extern LPFNBLOCKINPUT           lpfnBlockInput;
extern LPFNMAPLS                lpfnMapLS;
extern LPFNUNMAPLS              lpfnUnMapLS;
extern LPFNGETMODULEFILENAME	lpfnGetModuleFileName;
extern LPFNINTERLOCKCMPEXCH     lpfnInterlockedCompareExchange;
extern LPFNVIRTUALALLOCEX       lpfnVirtualAllocEx;
extern LPFNVIRTUALFREEEX        lpfnVirtualFreeEx;

extern void InitWindowClasses(void);
extern void UnInitWindowClasses(void);

// SharedAlloc ZEROES OUT THE ALLOCATED MEMORY
extern LPVOID   SharedAlloc(UINT cbSize,HWND hwnd,HANDLE *pProcessHandle);
extern VOID     SharedFree(LPVOID lpv,HANDLE hProcess);
extern BOOL     SharedRead(LPVOID lpvSharedSource,LPVOID lpvDest,DWORD cbSize,HANDLE hProcess);
extern BOOL     SharedWrite(LPVOID lpvSource,LPVOID lpvSharedDest,DWORD cbSize,HANDLE hProcess);



//
// Win64 compatibility
//

#ifndef _BASETSD_H_
// <basetsd.h> not included (ie. using pre-win64-sdk) so need to
// define _PTR types here.

typedef UINT UINT_PTR;
typedef DWORD DWORD_PTR;
#define PtrToInt  (int)

#endif

// inlines for SendMessage - saves having casts all over the place.
//
// SendMessageUINT - used when expecting a 32-bit return value - eg. text
//     length, number of elements, size of small (<4G) structures, etc.
//     (ie. almost all windows API messages)
//
// SendMessagePTR - used when expecting a pointer (32 or 64) return value
//     (ie. WM_GETOBJECT)

inline INT SendMessageINT( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Signed int, in keeping with LRESULT, which is also signed...
    return (INT)SendMessage( hWnd, uMsg, wParam, lParam );
}

inline DWORD_PTR SendMessagePTR( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return SendMessage( hWnd, uMsg, wParam, lParam );
}


