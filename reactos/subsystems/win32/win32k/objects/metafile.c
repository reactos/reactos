/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiCloseMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
  return 0;
}

HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

HDC
STDCALL
NtGdiCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
  UNIMPLEMENTED;
  return 0;
}

HDC
STDCALL
NtGdiCreateMetaFile(LPCWSTR  File)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
NtGdiDeleteEnhMetaFile(HENHMETAFILE  emf)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiDeleteMetaFile(HMETAFILE  mf)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiEnumEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  emf,
                          ENHMFENUMPROC  EnhMetaFunc,
                          LPVOID  Data,
                          CONST LPRECT  Rect)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data)
{
  UNIMPLEMENTED;
  return FALSE;
}

HENHMETAFILE
STDCALL
NtGdiGetEnhMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiGetMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data)
{
  UNIMPLEMENTED;
  return 0;
}

UINT
STDCALL
NtGdiGetWinMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer,
                             INT  MapMode,
                             HDC  Ref)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
STDCALL
NtGdiPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
                             UINT  Handles)
{
  UNIMPLEMENTED;
  return FALSE;
}

HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data)
{
  UNIMPLEMENTED;
  return 0;
}

HMETAFILE
STDCALL
NtGdiSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data)
{
  UNIMPLEMENTED;
  return 0;
}

HENHMETAFILE
STDCALL
NtGdiSetWinMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Buffer,
                                     HDC  Ref,
//                                     CONST METAFILEPICT *mfp)
				     PVOID mfp)
{
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
