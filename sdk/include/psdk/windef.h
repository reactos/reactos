/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#ifndef _WINDEF_
#define _WINDEF_
#pragma once

#define _WINDEF_H // wine ...

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4255)
#endif

#if (defined(_LP64) || defined(__LP64__)) && !defined(_M_AMD64)
#ifndef __ROS_LONG64__
#define __ROS_LONG64__
#endif
#endif

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif

#ifndef WIN32
#define WIN32
#endif

#if defined(_MAC) && !defined(_WIN32)
#define _WIN32
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINVER
#define WINVER 0x0502
#endif

#ifndef BASETYPES
#define BASETYPES
#ifndef __ROS_LONG64__
typedef unsigned long ULONG;
#else
typedef unsigned int ULONG;
#endif
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;
typedef int INT;
#endif  /* BASETYPES */

#undef MAX_PATH
#define MAX_PATH 260

#ifndef NULL
#ifdef __cplusplus
#ifndef _WIN64
#define NULL 0
#else
#define NULL 0LL
#endif  /* W64 */
#else
#define NULL ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef _NO_W32_PSEUDO_MODIFIERS
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif
#ifndef OPTIONAL
#define OPTIONAL
#endif
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#ifndef __declspec
#define __declspec(e) __attribute__((e))
#endif
#ifndef _declspec
#define _declspec(e) __attribute__((e))
#endif
#elif defined(__WATCOMC__)
#define PACKED
#else
#define PACKED
#endif

#undef far
#undef near
#undef pascal

#define far
#define near
#define pascal __stdcall

#define cdecl
#ifndef CDECL
#define CDECL
#endif

#if !defined(__x86_64__) //defined(_STDCALL_SUPPORTED)
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#define WINAPIV __cdecl
#define APIENTRY WINAPI
#define APIPRIVATE WINAPI
#define PASCAL WINAPI
#else
#define CALLBACK
#define WINAPI
#define WINAPIV
#define APIENTRY WINAPI
#define APIPRIVATE
#define PASCAL pascal
#endif

#undef FAR
#undef NEAR
#define FAR
#define NEAR

#ifndef CONST
#define CONST const
#endif

#ifndef _DEF_WINBOOL_
#define _DEF_WINBOOL_
typedef int WINBOOL;
#pragma push_macro("BOOL")
#undef BOOL
#if !defined(__OBJC__) && !defined(__OBJC_BOOL) && !defined(__objc_INCLUDE_GNU)
typedef int BOOL;
#endif
#define BOOL WINBOOL
typedef BOOL *PBOOL;
typedef BOOL *LPBOOL;
#pragma pop_macro("BOOL")
#endif /* _DEF_WINBOOL_ */

typedef unsigned char BYTE;
typedef unsigned short WORD;
#ifndef __ROS_LONG64__
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef BYTE *PBYTE;
typedef BYTE *LPBYTE;
typedef int *PINT;
typedef int *LPINT;
typedef WORD *PWORD;
typedef WORD *LPWORD;
#ifndef __ROS_LONG64__
typedef long *LPLONG;
#else
typedef int *LPLONG;
#endif
typedef DWORD *PDWORD;
typedef DWORD *LPDWORD;
typedef void *LPVOID;
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef CONST void *LPCVOID;
#endif
//typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;
typedef unsigned int *LPUINT;




#ifndef NT_INCLUDED
#include <winnt.h>
#endif

//#include <specstrings.h>

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
#ifndef _HRESULT_DEFINED
typedef LONG HRESULT;
#define _HRESULT_DEFINED
#endif

#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#define MAKEWORD(bLow, bHigh)   ((WORD)(((BYTE)((DWORD_PTR)(bLow) & 0xff  )) |  (((WORD)((BYTE)((DWORD_PTR)(bHigh) & 0xff)))   << 8 )))
#define MAKELONG(wLow, wHigh)   ((LONG)(((WORD)((DWORD_PTR)(wLow) & 0xffff)) | (((DWORD)((WORD)((DWORD_PTR)(wHigh) & 0xffff))) << 16)))
#define LOWORD(l)               ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)               ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define LOBYTE(w)               ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)               ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))

#ifndef WIN_INTERNAL
DECLARE_HANDLE (HWND);
//DECLARE_HANDLE (HHOOK);
#ifdef WINABLE
DECLARE_HANDLE (HEVENT);
#endif
#endif

typedef WORD ATOM;

typedef HANDLE *SPHANDLE;
typedef HANDLE *LPHANDLE;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef HANDLE GLOBALHANDLE;
typedef HANDLE LOCALHANDLE;

typedef INT_PTR (WINAPI *FARPROC)();
typedef INT_PTR (WINAPI *NEARPROC)();
typedef INT_PTR (WINAPI *PROC)();

typedef void *HGDIOBJ;

DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;

DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HCOLORSPACE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HGLRC);
DECLARE_HANDLE(HDESK);
DECLARE_HANDLE(HENHMETAFILE);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HSTR);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWINSTA);
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HMONITOR);
DECLARE_HANDLE(HWINEVENTHOOK);
DECLARE_HANDLE(HUMPD);

DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

typedef enum DPI_AWARENESS {
  DPI_AWARENESS_INVALID = -1,
  DPI_AWARENESS_UNAWARE = 0,
  DPI_AWARENESS_SYSTEM_AWARE,
  DPI_AWARENESS_PER_MONITOR_AWARE
} DPI_AWARENESS;

#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED    ((DPI_AWARENESS_CONTEXT)-5)

typedef int HFILE;
typedef HICON HCURSOR;
typedef DWORD COLORREF;
typedef DWORD *LPCOLORREF;

#define HFILE_ERROR ((HFILE)-1)

typedef struct tagRECT {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT,*PRECT,*NPRECT,*LPRECT;

typedef const RECT *LPCRECT;

typedef struct _RECTL {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECTL,*PRECTL,*LPRECTL;

typedef const RECTL *LPCRECTL;

typedef struct tagPOINT {
	LONG x;
	LONG y;
} POINT,*PPOINT,*NPPOINT,*LPPOINT;

typedef struct _POINTL {
  LONG x;
  LONG y;
} POINTL,*PPOINTL;

typedef struct tagSIZE {
	LONG cx;
	LONG cy;
} SIZE,*PSIZE,*LPSIZE;

typedef SIZE SIZEL;
typedef SIZE *PSIZEL,*LPSIZEL;

typedef struct tagPOINTS {
	SHORT x;
	SHORT y;
} POINTS,*PPOINTS,*LPPOINTS;

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME,*PFILETIME,*LPFILETIME;
#define _FILETIME_

#define DM_UPDATE 1
#define DM_COPY 2
#define DM_PROMPT 4
#define DM_MODIFY 8

#define DM_IN_BUFFER DM_MODIFY
#define DM_IN_PROMPT DM_PROMPT
#define DM_OUT_BUFFER DM_COPY
#define DM_OUT_DEFAULT DM_UPDATE

#define DC_FIELDS 1
#define DC_PAPERS 2
#define DC_PAPERSIZE 3
#define DC_MINEXTENT 4
#define DC_MAXEXTENT 5
#define DC_BINS 6
#define DC_DUPLEX 7
#define DC_SIZE 8
#define DC_EXTRA 9
#define DC_VERSION 10
#define DC_DRIVER 11
#define DC_BINNAMES 12
#define DC_ENUMRESOLUTIONS 13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE 15
#define DC_PAPERNAMES 16
#define DC_ORIENTATION 17
#define DC_COPIES 18

/* needed by header files generated by WIDL */
#ifdef __WINESRC__
#define WINE_NO_UNICODE_MACROS
#endif

#ifdef WINE_NO_UNICODE_MACROS
# define WINELIB_NAME_AW(func) \
    func##_must_be_suffixed_with_W_or_A_in_this_context \
    func##_must_be_suffixed_with_W_or_A_in_this_context
#else  /* WINE_NO_UNICODE_MACROS */
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##W
# else
#  define WINELIB_NAME_AW(func) func##A
# endif
#endif  /* WINE_NO_UNICODE_MACROS */

#ifdef WINE_NO_UNICODE_MACROS
# define DECL_WINELIB_TYPE_AW(type)  /* nothing */
#else
# define DECL_WINELIB_TYPE_AW(type)  typedef WINELIB_NAME_AW(type) type;
#endif

#ifndef __WATCOMC__
#ifndef _export
#define _export
#endif
#ifndef __export
#define __export
#endif
#endif

#if 0
#ifdef __GNUC__
#define PACKED __attribute__((packed))
//#ifndef _fastcall
//#define _fastcall __attribute__((fastcall))
//#endif
//#ifndef __fastcall
//#define __fastcall __attribute__((fastcall))
//#endif
//#ifndef _stdcall
//#define _stdcall __attribute__((stdcall))
//#endif
//#ifndef __stdcall
//#define __stdcall __attribute__((stdcall))
//#endif
//#ifndef _cdecl
//#define _cdecl __attribute__((cdecl))
//#endif
//#ifndef __cdecl
//#define __cdecl __attribute__((cdecl))
//#endif
#ifndef __declspec
#define __declspec(e) __attribute__((e))
#endif
#ifndef _declspec
#define _declspec(e) __attribute__((e))
#endif
#elif defined(__WATCOMC__)
#define PACKED
#else
#define PACKED
#define _cdecl
#define __cdecl
#endif
#endif

#if 1 // needed by shlwapi.h
#ifndef __ms_va_list
# if defined(__x86_64__) && defined (__GNUC__)
#  define __ms_va_list __builtin_ms_va_list
#  define __ms_va_start(list,arg) __builtin_ms_va_start(list,arg)
#  define __ms_va_end(list) __builtin_ms_va_end(list)
# else
#  define __ms_va_list va_list
#  define __ms_va_start(list,arg) va_start(list,arg)
#  define __ms_va_end(list) va_end(list)
# endif
#endif
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WINDEF_ */

