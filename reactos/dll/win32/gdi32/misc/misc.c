/*
 *  ReactOS GDI lib
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS gdi32.dll
 * FILE:            lib/gdi32/misc/misc.c
 * PURPOSE:         Miscellaneous functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2004/09/04  Created
 */

#include "precomp.h"

PGDI_TABLE_ENTRY GdiHandleTable = NULL;
HANDLE CurrentProcessId = NULL;
DWORD GDI_BatchLimit = 1;


BOOL
STDCALL
GdiAlphaBlend(
            HDC hDCDst,
            int DstX,
            int DstY,
            int DstCx,
            int DstCy,
            HDC hDCSrc,
            int SrcX,
            int SrcY,
            int SrcCx,
            int SrcCy,
            BLENDFUNCTION BlendFunction
            )
{
   if ( hDCSrc == NULL ) return FALSE;

   if (GDI_HANDLE_GET_TYPE(hDCSrc) == GDI_OBJECT_TYPE_METADC) return FALSE;
      
   return NtGdiAlphaBlend(
                      hDCDst,
                        DstX,
                        DstY,
                       DstCx,
                       DstCy,
                      hDCSrc,
                        SrcX,
                        SrcY,
                       SrcCx,
                       SrcCy,
               BlendFunction,
                           0 );
}

/*
 * @implemented
 */
HGDIOBJ 
STDCALL
GdiFixUpHandle(HGDIOBJ hGdiObj)
{
 if (((ULONG_PTR)(hGdiObj)) & GDI_HANDLE_UPPER_MASK ) return hGdiObj;
 PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);
 return hGdiObj = (HGDIOBJ)(((LONG_PTR)(hGdiObj)) |
                     (Entry->Type << GDI_ENTRY_UPPER_SHIFT)); // Rebuild handle for Object
}

/*
 * @implemented
 */
PVOID
STDCALL
GdiQueryTable(VOID)
{
  return (PVOID)GdiHandleTable;
}

BOOL GdiIsHandleValid(HGDIOBJ hGdiObj)
{
  PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);
  if((Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 && 
     (Entry->Type << GDI_ENTRY_UPPER_SHIFT) == GDI_HANDLE_GET_UPPER(hGdiObj))
  {
    HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
    if(pid == NULL || pid == CurrentProcessId)
    {
      return TRUE;
    }
  }
  return FALSE;
}

BOOL GdiGetHandleUserData(HGDIOBJ hGdiObj, PVOID *UserData)
{
  PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);
  if((Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 && 
     (Entry->Type << GDI_ENTRY_UPPER_SHIFT) == GDI_HANDLE_GET_UPPER(hGdiObj))
  {
    HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
    if(pid == NULL || pid == CurrentProcessId)
    {
      *UserData = Entry->UserData;
      return TRUE;
    }
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
}

PLDC GdiGetLDC(HDC hDC)
{
    PDC_ATTR Dc_Attr;
    if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr))
      return NULL;
    return Dc_Attr->pvLDC;  
}

/*
 * @implemented
 */
DWORD
STDCALL
GdiSetBatchLimit(DWORD	Limit)
{
  DWORD OldLimit = GDI_BatchLimit;
    if ((!Limit) || (Limit > GDI_BATCH_LIMIT)) return Limit;
    GdiFlush();
    GDI_BatchLimit = Limit;
    return OldLimit;
}


/*
 * @implemented
 */
DWORD
STDCALL
GdiGetBatchLimit()
{
    return GDI_BatchLimit;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiReleaseDC(HDC hdc)
{
    return 0;
}

INT STDCALL
ExtEscape(
	HDC hDC,
	int nEscape,
	int cbInput,
	LPCSTR lpszInData,
	int cbOutput,
	LPSTR lpszOutData
)
{
	return NtGdiExtEscape(hDC, NULL, 0, nEscape, cbInput, (LPSTR)lpszInData, cbOutput, lpszOutData);
}

/*
 * @implemented
 */
VOID
STDCALL
GdiSetLastError(DWORD dwErrCode)
{
	SetLastError(dwErrCode);
}
