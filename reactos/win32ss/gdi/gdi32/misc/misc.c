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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS gdi32.dll
 * FILE:            dll/win32/gdi32/misc/misc.c
 * PURPOSE:         Miscellaneous functions
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2004/09/04  Created
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

PGDI_TABLE_ENTRY GdiHandleTable = NULL;
PGDI_SHARED_HANDLE_TABLE GdiSharedHandleTable = NULL;
HANDLE CurrentProcessId = NULL;
DWORD GDI_BatchLimit = 1;


BOOL
WINAPI
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
WINAPI
GdiFixUpHandle(HGDIOBJ hGdiObj)
{
    PGDI_TABLE_ENTRY Entry;

    if (((ULONG_PTR)(hGdiObj)) & GDI_HANDLE_UPPER_MASK )
    {
        return hGdiObj;
    }

    /* FIXME is this right ?? */

    Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);

    /* Rebuild handle for Object */
    return hGdiObj = (HGDIOBJ)(((LONG_PTR)(hGdiObj)) | (Entry->Type << GDI_ENTRY_UPPER_SHIFT));
}

/*
 * @implemented
 */
PVOID
WINAPI
GdiQueryTable(VOID)
{
    return (PVOID)GdiHandleTable;
}

BOOL GdiIsHandleValid(HGDIOBJ hGdiObj)
{
    PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);
// We are only looking for TYPE not the rest here, and why is FullUnique filled up with CRAP!?
// DPRINT1("FullUnique -> %x\n", Entry->FullUnique);
    if((Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 &&
            ( (Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK ) ==
            GDI_HANDLE_GET_TYPE(hGdiObj))
    {
        HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
        if(pid == NULL || pid == CurrentProcessId)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL GdiGetHandleUserData(HGDIOBJ hGdiObj, DWORD ObjectType, PVOID *UserData)
{
    PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hGdiObj);

    /* Check if twe have the correct type */
    if (GDI_HANDLE_GET_TYPE(hGdiObj) != ObjectType ||
        ((Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK) != ObjectType ||
        (Entry->Type & GDI_ENTRY_BASETYPE_MASK) != (ObjectType & GDI_ENTRY_BASETYPE_MASK))
    {
        return FALSE;
    }

    /* Check if we are the owner */
    if ((HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1) != CurrentProcessId)
    {
        return FALSE;
    }

    *UserData = Entry->UserData;
    return TRUE;
}

PLDC
FASTCALL
GdiGetLDC(HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        return NULL;
    }

    /* Return the LDC pointer */
    return pdcattr->pvLDC;
}

BOOL
FASTCALL
GdiSetLDC(HDC hdc, PVOID pvLDC)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        return FALSE;
    }

    /* Set the LDC pointer */
    pdcattr->pvLDC = pvLDC;
    return TRUE;
}


VOID GdiSAPCallback(PLDC pldc)
{
    DWORD Time, NewTime = GetTickCount();

    Time = NewTime - pldc->CallBackTick;

    if ( Time < SAPCALLBACKDELAY) return;

    pldc->CallBackTick = NewTime;

    if ( !pldc->pAbortProc(pldc->hDC, 0) )
    {
        CancelDC(pldc->hDC);
        AbortDoc(pldc->hDC);
    }
}

/*
 * @implemented
 */
DWORD
WINAPI
GdiSetBatchLimit(DWORD	Limit)
{
    DWORD OldLimit = GDI_BatchLimit;

    if ( (!Limit) ||
            (Limit >= GDI_BATCH_LIMIT))
    {
        return Limit;
    }

    GdiFlush();
    GDI_BatchLimit = Limit;
    return OldLimit;
}


/*
 * @implemented
 */
DWORD
WINAPI
GdiGetBatchLimit()
{
    return GDI_BatchLimit;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiReleaseDC(HDC hdc)
{
    return 0;
}

INT
WINAPI
ExtEscape(HDC hDC,
          int nEscape,
          int cbInput,
          LPCSTR lpszInData,
          int cbOutput,
          LPSTR lpszOutData)
{
    return NtGdiExtEscape(hDC, NULL, 0, nEscape, cbInput, (LPSTR)lpszInData, cbOutput, lpszOutData);
}

/*
 * @implemented
 */
VOID
WINAPI
GdiSetLastError(DWORD dwErrCode)
{
    NtCurrentTeb()->LastErrorValue = (ULONG) dwErrCode;
}

BOOL
WINAPI
GdiAddGlsBounds(HDC hdc,LPRECT prc)
{
    //FIXME: Lookup what 0x8000 means
    return NtGdiSetBoundsRect(hdc, prc, 0x8000 |  DCB_ACCUMULATE ) ? TRUE : FALSE;
}

extern PGDIHANDLECACHE GdiHandleCache;

HGDIOBJ
FASTCALL
hGetPEBHandle(HANDLECACHETYPE Type, COLORREF cr)
{
    int Number, Offset, MaxNum, GdiType;
    HANDLE Lock;
    HGDIOBJ Handle = NULL;

    Lock = InterlockedCompareExchangePointer( (PVOID*)&GdiHandleCache->ulLock,
            NtCurrentTeb(),
            NULL );

    if (Lock) return Handle;

    Number = GdiHandleCache->ulNumHandles[Type];

    if (Type == hctBrushHandle)
    {
       Offset = 0;
       MaxNum = CACHE_BRUSH_ENTRIES;
       GdiType = GDILoObjType_LO_BRUSH_TYPE;
    }
    else if (Type == hctPenHandle)
    {
       Offset = CACHE_BRUSH_ENTRIES;
       MaxNum = CACHE_PEN_ENTRIES;
       GdiType = GDILoObjType_LO_PEN_TYPE;
    }
    else if (Type == hctRegionHandle)
    {
       Offset = CACHE_BRUSH_ENTRIES+CACHE_PEN_ENTRIES;
       MaxNum = CACHE_REGION_ENTRIES;
       GdiType = GDILoObjType_LO_REGION_TYPE;
    }
    else // Font is not supported here.
    {
       return Handle;
    }

    if ( Number && Number <= MaxNum )
    {
       PBRUSH_ATTR pBrush_Attr;
       HGDIOBJ *hPtr;
       hPtr = GdiHandleCache->Handle + Offset;
       Handle = hPtr[Number - 1];

       if (GdiGetHandleUserData( Handle, GdiType, (PVOID) &pBrush_Attr))
       {
          if (pBrush_Attr->AttrFlags & ATTR_CACHED)
          {
             DPRINT("Get Handle! Type %d Count %lu PEB 0x%p\n", Type, GdiHandleCache->ulNumHandles[Type], NtCurrentTeb()->ProcessEnvironmentBlock);
             pBrush_Attr->AttrFlags &= ~ATTR_CACHED;
             hPtr[Number - 1] = NULL;
             GdiHandleCache->ulNumHandles[Type]--;
             if ( Type == hctBrushHandle ) // Handle only brush.
             {
                if ( pBrush_Attr->lbColor != cr )
                {
                   pBrush_Attr->lbColor = cr ;
                   pBrush_Attr->AttrFlags |= ATTR_NEW_COLOR;
                }
             }
          }
       }
       else
       {
          Handle = NULL;
       }
    }
    (void)InterlockedExchangePointer((PVOID*)&GdiHandleCache->ulLock, Lock);
    return Handle;
}

