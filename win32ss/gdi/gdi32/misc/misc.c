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
 * FILE:            win32ss/gdi/gdi32/misc/misc.c
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
extern PGDIHANDLECACHE GdiHandleCache;

/*
 * @implemented
 */
BOOL
WINAPI
GdiFlush(VOID)
{
    NtGdiFlush();
    return TRUE;
}

/*
 * @unimplemented
 */
INT
WINAPI
Escape(
    _In_ HDC hdc,
    _In_ INT nEscape,
    _In_ INT cbInput,
    _In_ LPCSTR lpvInData,
    _Out_ LPVOID lpvOutData)
{
    INT retValue = SP_ERROR;
    ULONG ulObjType;

    ulObjType = GDI_HANDLE_GET_TYPE(hdc);

    if (ulObjType == GDILoObjType_LO_METADC16_TYPE)
    {
        return METADC16_Escape(hdc, nEscape, cbInput, lpvInData, lpvOutData);
    }

    switch (nEscape)
    {
        case ABORTDOC:
            /* Note: Windows checks if the handle has any user data for the ABORTDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            else
            {
                retValue = AbortDoc(hdc);
            }
            break;

        case DRAFTMODE:
        case FLUSHOUTPUT:
        case SETCOLORTABLE:
            /* Note 1: DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE are outdated */
            /* Note 2: Windows checks if the handle has any user data for the DRAFTMODE, FLUSHOUTPUT, SETCOLORTABLE commands
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
            }
            retValue = FALSE;
            break;

        case SETABORTPROC:
            /* Note: Windows checks if the handle has any user data for the SETABORTPROC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            retValue = SetAbortProc(hdc, (ABORTPROC)lpvInData);
            break;

        case GETCOLORTABLE:
            retValue = GetSystemPaletteEntries(hdc, (UINT)*lpvInData, 1, (LPPALETTEENTRY)lpvOutData);
            if (!retValue)
            {
                retValue = SP_ERROR;
            }
            break;

        case ENDDOC:
            /* Note: Windows checks if the handle has any user data for the ENDDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }
            retValue = EndDoc(hdc);
            break;

        case GETSCALINGFACTOR:
            /* Note GETSCALINGFACTOR is outdated have been replace by GetDeviceCaps */
            if (ulObjType == GDI_OBJECT_TYPE_DC)
            {
                if (lpvOutData)
                {
                    PPOINT ptr = (PPOINT)lpvOutData;
                    ptr->x = 0;
                    ptr->y = 0;
                }
            }
            retValue = FALSE;
            break;

        case GETEXTENDEDTEXTMETRICS:
            retValue = GetETM(hdc, (EXTTEXTMETRIC *)lpvOutData) != 0;
            break;

        case STARTDOC:
        {
            DOCINFOA di;

            /* Note: Windows checks if the handle has any user data for the STARTDOC command
             * ReactOS copies this behavior to be compatible with windows 2003
             */
            if (GdiGetDcAttr(hdc) == NULL)
            {
                GdiSetLastError(ERROR_INVALID_HANDLE);
                retValue = FALSE;
            }

            di.cbSize = sizeof(DOCINFOA);
            di.lpszOutput = 0;
            di.lpszDatatype = 0;
            di.fwType = 0;
            di.lpszDocName = lpvInData;

            /* NOTE : doc for StartDocA/W at msdn http://msdn2.microsoft.com/en-us/library/ms535793(VS.85).aspx */
            retValue = StartDocA(hdc, &di);

            /* Check if StartDocA failed */
            if (retValue < 0)
            {
                {
                    retValue = GetLastError();

                    /* Translate StartDocA error code to STARTDOC error code
                     * see msdn http://msdn2.microsoft.com/en-us/library/ms535472.aspx
                     */
                    switch(retValue)
                    {
                    case ERROR_NOT_ENOUGH_MEMORY:
                        retValue = SP_OUTOFMEMORY;
                        break;

                    case ERROR_PRINT_CANCELLED:
                        retValue = SP_USERABORT;
                        break;

                    case ERROR_DISK_FULL:
                        retValue = SP_OUTOFDISK;
                        break;

                    default:
                        retValue = SP_ERROR;
                        break;
                    }
                }
            }
        }
        break;

        default:
            UNIMPLEMENTED;
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }

    return retValue;
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

INT
WINAPI
NamedEscape(HDC hdc,
            PWCHAR pDriver,
            INT iEsc,
            INT cjIn,
            LPSTR pjIn,
            INT cjOut,
            LPSTR pjOut)
{
    /* FIXME metadc, metadc are done most in user mode, and we do not support it
     * Windows 2000/XP/Vista ignore the current hdc, that are being pass and always set hdc to NULL
     * when it calls to NtGdiExtEscape from NamedEscape
     */
    return NtGdiExtEscape(NULL,pDriver,wcslen(pDriver),iEsc,cjIn,pjIn,cjOut,pjOut);
}

/*
 * @implemented
 */
int
WINAPI
DrawEscape(HDC  hDC,
           INT nEscape,
           INT cbInput,
           LPCSTR lpszInData)
{
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
        return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);

    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC)
    {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( pLDC )
        {
            if (pLDC->Flags & LDC_META_PRINT)
            {
//           if (nEscape != QUERYESCSUPPORT)
//              return EMFDRV_WriteEscape(hDC, nEscape, cbInput, lpszInData, EMR_DRAWESCAPE);

                return NtGdiDrawEscape(hDC, nEscape, cbInput, (LPSTR) lpszInData);
            }
        }
        SetLastError(ERROR_INVALID_HANDLE);
    }
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GdiDrawStream(HDC dc, ULONG l, VOID *v) // See Bug 4784
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @implemented
 */
BOOL
WINAPI
GdiValidateHandle(HGDIOBJ hobj)
{
    PGDI_TABLE_ENTRY Entry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hobj);
    if ( (Entry->Type & GDI_ENTRY_BASETYPE_MASK) != 0 &&
            ( (Entry->Type << GDI_ENTRY_UPPER_SHIFT) & GDI_HANDLE_TYPE_MASK ) ==
            GDI_HANDLE_GET_TYPE(hobj) )
    {
        HANDLE pid = (HANDLE)((ULONG_PTR)Entry->ProcessId & ~0x1);
        if(pid == NULL || pid == CurrentProcessId)
        {
            return TRUE;
        }
    }
    return FALSE;

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
GdiGetBatchLimit(VOID)
{
    return GDI_BatchLimit;
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

/*
 * @unimplemented
 */
BOOL
WINAPI
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
DEVMODEW *
WINAPI
GdiConvertToDevmodeW(const DEVMODEA *dmA)
{
    DEVMODEW *dmW;
    WORD dmW_size, dmA_size;

    dmA_size = dmA->dmSize;

    /* this is the minimal dmSize that XP accepts */
    if (dmA_size < FIELD_OFFSET(DEVMODEA, dmFields))
        return NULL;

    if (dmA_size > sizeof(DEVMODEA))
        dmA_size = sizeof(DEVMODEA);

    dmW_size = dmA_size + CCHDEVICENAME;
    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
        dmW_size += CCHFORMNAME;

    dmW = HeapAlloc(GetProcessHeap(), 0, dmW_size + dmA->dmDriverExtra);
    if (!dmW) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmDeviceName, CCHDEVICENAME,
                        dmW->dmDeviceName, CCHDEVICENAME);
    /* copy slightly more, to avoid long computations */
    memcpy(&dmW->dmSpecVersion, &dmA->dmSpecVersion, dmA_size - CCHDEVICENAME);

    if (dmA_size >= FIELD_OFFSET(DEVMODEA, dmFormName) + CCHFORMNAME)
    {
        MultiByteToWideChar(CP_ACP, 0, (const char*) dmA->dmFormName, CCHFORMNAME,
                            dmW->dmFormName, CCHFORMNAME);
        if (dmA_size > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            memcpy(&dmW->dmLogPixels, &dmA->dmLogPixels, dmA_size - FIELD_OFFSET(DEVMODEA, dmLogPixels));
    }

    if (dmA->dmDriverExtra)
        memcpy((char *)dmW + dmW_size, (const char *)dmA + dmA_size, dmA->dmDriverExtra);

    dmW->dmSize = dmW_size;

    return dmW;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiRealizationInfo(HDC hdc,
                   PREALIZATION_INFO pri)
{
    // ATM we do not support local font data and Language Pack.
    return NtGdiGetRealizationInfo(hdc, pri, (HFONT) NULL);
}


/*
 * @unimplemented
 */
VOID WINAPI GdiInitializeLanguagePack(DWORD InitParam)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

BOOL
WINAPI
GdiAddGlsBounds(HDC hdc,LPRECT prc)
{
    //FIXME: Lookup what 0x8000 means
    return NtGdiSetBoundsRect(hdc, prc, 0x8000 |  DCB_ACCUMULATE ) ? TRUE : FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GdiAddGlsRecord(HDC hdc,
                DWORD unknown1,
                LPCSTR unknown2,
                LPRECT unknown3)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

