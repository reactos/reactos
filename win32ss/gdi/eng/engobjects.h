/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 /*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Internal Objects
 * FILE:              win32ss/gdi/eng/engobjects.h
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 21/8/1999: Created
 */

#pragma once

/* Structure of internal gdi objects that win32k manages for ddi engine:
   |---------------------------------|
   |         Public part             |
   |      accessed from engine       |
   |---------------------------------|
   |        Private part             |
   |       managed by gdi            |
   |_________________________________|

---------------------------------------------------------------------------*/

typedef struct _RWNDOBJ {
  PVOID   pvConsumer;
  RECTL   rclClient;
  SURFOBJ *psoOwner;
} RWNDOBJ;

/* EXtended CLip and Window Region Objects */
#ifdef __cplusplus
typedef struct _XCLIPOBJ : _CLIPOBJ, _RWNDOBJ
{
#else
typedef struct _XCLIPOBJ
{
  CLIPOBJ;
  RWNDOBJ;
#endif
  struct _REGION *pClipRgn;    /* prgnRao_ or (prgnVis_ if (prgnRao_ == z)) */
  //
  RECTL   rclClipRgn;
  //PVOID   pscanClipRgn; /* Ptr to regions rect buffer based on iDirection. */
  RECTL*  Rects;
  DWORD   cScan;
  DWORD   reserved;
  ULONG   EnumPos;
  LONG    lscnSize;
  ULONG   EnumMax;
  ULONG   iDirection;
  ULONG   iType;
  DWORD   reserved1;
  LONG    lUpDown;
  DWORD   reserved2;
  BOOL    bAll;
  //
  DWORD   RectCount;   /* count/mode based on # of rect in regions scan. */
  PVOID   pDDA;        /* Pointer to a large drawing structure. */
} XCLIPOBJ, *PXCLIPOBJ;

/*
  EngCreateClip allocates XCLIPOBJ and REGION, pco->co.pClipRgn = &pco->ro.
  {
    XCLIPOBJ co;
    REGION   ro;
  }
 */

extern XCLIPOBJ gxcoTrivial;

#ifdef __cplusplus
typedef struct _EWNDOBJ : _XCLIPOBJ
{
#else
typedef struct _EWNDOBJ
{
    XCLIPOBJ;
#endif
    /* Extended WNDOBJ part */
    HWND              Hwnd;
    WNDOBJCHANGEPROC  ChangeProc;
    FLONG             Flags;
    int               PixelFormat;
} EWNDOBJ, *PEWNDOBJ;


/*ei What is this for? */
typedef struct _DRVFUNCTIONSGDI {
  HDEV  hdev;
  DRVFN Functions[INDEX_LAST];
} DRVFUNCTIONSGDI;

typedef struct _FLOATGDI {
  ULONG Dummy;
} FLOATGDI;

typedef struct _SHARED_MEM {
  PVOID         Buffer;
  ULONG         BufferSize;
  BOOL          IsMapping;
  LONG          RefCount;
} SHARED_MEM, *PSHARED_MEM;

typedef struct _SHARED_FACE_CACHE {
    UINT OutlineRequiredSize;
    UNICODE_STRING FontFamily;
    UNICODE_STRING FullName;
} SHARED_FACE_CACHE, *PSHARED_FACE_CACHE;

typedef struct _SHARED_FACE {
  FT_Face       Face;
  LONG          RefCount;
  PSHARED_MEM   Memory;
  SHARED_FACE_CACHE EnglishUS;
  SHARED_FACE_CACHE UserLanguage;
} SHARED_FACE, *PSHARED_FACE;

typedef struct _FONTGDI {
  FONTOBJ       FontObj;
  ULONG         iUnique;
  FLONG         flType;

  DHPDEV        dhpdev;
  PSHARED_FACE  SharedFace;

  LONG          lMaxNegA;
  LONG          lMaxNegC;
  LONG          lMinWidthD;

  LPWSTR        Filename;
  BYTE          RequestUnderline;
  BYTE          RequestStrikeOut;
  BYTE          RequestItalic;
  LONG          RequestWeight;
  BYTE          OriginalItalic;
  LONG          OriginalWeight;
  BYTE          CharSet;
} FONTGDI, *PFONTGDI;

typedef struct _PATHGDI {
  PATHOBJ PathObj;
} PATHGDI;

typedef struct _XFORMGDI {
  ULONG Dummy;
  /* XFORMOBJ has no public members */
} XFORMGDI;

/* As the *OBJ structures are located at the beginning of the *GDI structures
   we can simply typecast the pointer */
#define ObjToGDI(ClipObj, Type) (Type##GDI *)(ClipObj)
#define GDIToObj(ClipGDI, Type) (Type##OBJ *)(ClipGDI)
