/*
 * Basic type definitions for 16 bit variations on Windows types.
 * These types are provided mostly to insure compatibility with
 * 16 bit windows code.
 *
 * Copyright (C) the Wine project
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINDEF16_H
#define __WINE_WINDEF16_H

/* #include "windef.h" */

/* Standard data types */

typedef unsigned short  BOOL16;
typedef DWORD           SEGPTR;

typedef UINT16          HANDLE16;
typedef HANDLE16       *LPHANDLE16;

typedef UINT16          WPARAM16;
typedef INT16          *LPINT16;
typedef UINT16         *LPUINT16;

#define MAKESEGPTR(seg,off) ((SEGPTR)MAKELONG(off,seg))

#define HFILE_ERROR16   ((HFILE16)-1)

#define DECLARE_HANDLE16(a) \
	typedef HANDLE16 a##16; \
	typedef a##16 *P##a##16; \
	typedef a##16 *NP##a##16; \
	typedef a##16 *LP##a##16

DECLARE_HANDLE16(HACMDRIVERID);
DECLARE_HANDLE16(HACMDRIVER);
DECLARE_HANDLE16(HACMOBJ);
DECLARE_HANDLE16(HACMSTREAM);
DECLARE_HANDLE16(HMETAFILEPICT);

DECLARE_HANDLE16(HACCEL);
DECLARE_HANDLE16(HBITMAP);
DECLARE_HANDLE16(HBRUSH);
DECLARE_HANDLE16(HCOLORSPACE);
DECLARE_HANDLE16(HCURSOR);
DECLARE_HANDLE16(HDC);
DECLARE_HANDLE16(HDROP);
DECLARE_HANDLE16(HDRVR);
DECLARE_HANDLE16(HDWP);
DECLARE_HANDLE16(HENHMETAFILE);
DECLARE_HANDLE16(HFILE);
DECLARE_HANDLE16(HFONT);
DECLARE_HANDLE16(HICON);
DECLARE_HANDLE16(HINSTANCE);
DECLARE_HANDLE16(HKEY);
DECLARE_HANDLE16(HMENU);
DECLARE_HANDLE16(HMETAFILE);
DECLARE_HANDLE16(HMIDI);
DECLARE_HANDLE16(HMIDIIN);
DECLARE_HANDLE16(HMIDIOUT);
DECLARE_HANDLE16(HMIDISTRM);
DECLARE_HANDLE16(HMIXER);
DECLARE_HANDLE16(HMIXEROBJ);
DECLARE_HANDLE16(HMMIO);
DECLARE_HANDLE16(HPALETTE);
DECLARE_HANDLE16(HPEN);
DECLARE_HANDLE16(HQUEUE);
DECLARE_HANDLE16(HRGN);
DECLARE_HANDLE16(HRSRC);
DECLARE_HANDLE16(HTASK);
DECLARE_HANDLE16(HWAVE);
DECLARE_HANDLE16(HWAVEIN);
DECLARE_HANDLE16(HWAVEOUT);
DECLARE_HANDLE16(HWINSTA);
DECLARE_HANDLE16(HDESK);
DECLARE_HANDLE16(HWND);
DECLARE_HANDLE16(HKL);
DECLARE_HANDLE16(HIC);
DECLARE_HANDLE16(HRASCONN);
#undef DECLARE_HANDLE16

typedef HINSTANCE16 HMODULE16;
typedef HANDLE16 HGDIOBJ16;
typedef HANDLE16 HGLOBAL16;
typedef HANDLE16 HLOCAL16;

#include "pshpack1.h"

/* The SIZE structure */

typedef struct
{
    INT16  cx;
    INT16  cy;
} SIZE16, *PSIZE16, *LPSIZE16;

/* The POINT structure */

typedef struct
{
    INT16  x;
    INT16  y;
} POINT16, *PPOINT16, *LPPOINT16;

/* The RECT structure */

typedef struct
{
    INT16  left;
    INT16  top;
    INT16  right;
    INT16  bottom;
} RECT16, *LPRECT16;

#include "poppack.h"

/* Callback function pointers types */

typedef LRESULT (CALLBACK *DRIVERPROC16)(DWORD,HDRVR16,UINT16,LPARAM,LPARAM);
typedef BOOL16  (CALLBACK *DLGPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef INT16   (CALLBACK *EDITWORDBREAKPROC16)(LPSTR,INT16,INT16,INT16);
typedef LRESULT (CALLBACK *FARPROC16)();
typedef INT16   (CALLBACK *PROC16)();
typedef BOOL16  (CALLBACK *GRAYSTRINGPROC16)(HDC16,LPARAM,INT16);
typedef LRESULT (CALLBACK *HOOKPROC16)(INT16,WPARAM16,LPARAM);
typedef BOOL16  (CALLBACK *PROPENUMPROC16)(HWND16,SEGPTR,HANDLE16);
typedef VOID    (CALLBACK *TIMERPROC16)(HWND16,UINT16,UINT16,DWORD);
typedef LRESULT (CALLBACK *WNDENUMPROC16)(HWND16,LPARAM);
typedef LRESULT (CALLBACK *WNDPROC16)(HWND16,UINT16,WPARAM16,LPARAM);

#endif /* __WINE_WINDEF16_H */
