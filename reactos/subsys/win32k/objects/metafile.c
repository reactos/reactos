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
/* $Id: metafile.c,v 1.10 2004/05/10 17:07:20 weiden Exp $ */
#include <w32k.h>

HENHMETAFILE
STDCALL
NtGdiCloseEnhMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
NtGdiCloseMetaFile(HDC  hDC)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
NtGdiCopyEnhMetaFile(HENHMETAFILE  Src,
                                  LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
NtGdiCopyMetaFile(HMETAFILE  Src,
                            LPCWSTR  File)
{
  UNIMPLEMENTED;
}

HDC
STDCALL
NtGdiCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
  UNIMPLEMENTED;
}

HDC
STDCALL
NtGdiCreateMetaFile(LPCWSTR  File)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiDeleteEnhMetaFile(HENHMETAFILE  emf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiDeleteMetaFile(HMETAFILE  mf)
{
  UNIMPLEMENTED;
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
}

BOOL
STDCALL
NtGdiEnumMetaFile(HDC  hDC,
                       HMETAFILE  mf,
                       MFENUMPROC  MetaFunc,
                       LPARAM  lParam)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiGdiComment(HDC  hDC,
                     UINT  Size,
                     CONST LPBYTE  Data)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
NtGdiGetEnhMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetEnhMetaFileBits(HENHMETAFILE  hemf,
                             UINT  BufSize,
                             LPBYTE  Buffer)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetEnhMetaFileDescription(HENHMETAFILE  hemf,
                                    UINT  BufSize,
                                    LPWSTR  Description)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetEnhMetaFileHeader(HENHMETAFILE  hemf,
                               UINT  BufSize,
                               LPENHMETAHEADER  emh)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetEnhMetaFilePaletteEntries(HENHMETAFILE  hemf,
                                       UINT  Entries,
                                       LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
NtGdiGetMetaFile(LPCWSTR  MetaFile)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetMetaFileBitsEx(HMETAFILE  hmf,
                            UINT  Size,
                            LPVOID  Data)
{
  UNIMPLEMENTED;
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
}

BOOL
STDCALL
NtGdiPlayEnhMetaFile(HDC  hDC,
                          HENHMETAFILE  hemf,
                          CONST PRECT  Rect)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiPlayEnhMetaFileRecord(HDC  hDC,
                                LPHANDLETABLE  Handletable,
                                CONST ENHMETARECORD *EnhMetaRecord,
                                UINT  Handles)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiPlayMetaFile(HDC  hDC,
                       HMETAFILE  hmf)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiPlayMetaFileRecord(HDC  hDC,
                             LPHANDLETABLE  Handletable,
                             LPMETARECORD  MetaRecord,
                             UINT  Handles)
{
  UNIMPLEMENTED;
}

HENHMETAFILE
STDCALL
NtGdiSetEnhMetaFileBits(UINT  BufSize,
                                     CONST PBYTE  Data)
{
  UNIMPLEMENTED;
}

HMETAFILE
STDCALL
NtGdiSetMetaFileBitsEx(UINT  Size,
                                 CONST PBYTE  Data)
{
  UNIMPLEMENTED;
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
}

/* EOF */
