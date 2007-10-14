/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dd.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <w32k.h>

//#define NDEBUG
#include <debug.h>
#include <reactos/drivers/directx/dxg.h>



extern PDRVFN gpDxFuncs;

// PDD_DESTROYDRIVER  DestroyDriver;
// PDD_CREATESURFACE  CreateSurface;
/* see ddsurf.c for  PDD_SETCOLORKEY  SetColorKey; */
// PDD_SETMODE  SetMode;
// PDD_WAITFORVERTICALBLANK  WaitForVerticalBlank;
typedef DWORD (NTAPI *PGD_DDCANCREATESURFACE)(HANDLE hDirectDrawLocal, PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);
// PDD_CREATEPALETTE  CreatePalette;
// PDD_GETSCANLINE  GetScanLine;
// PDD_MAPMEMORY  MapMemory;






#define DXG_GET_INDEX_FUNCTION(INDEX, FUNCTION) \
    if (gpDxFuncs) \
    { \
        for (i = 0; i <= DXG_INDEX_DxDdIoctl; i++) \
        { \
            if (gpDxFuncs[i].iFunc == INDEX)  \
            { \
                FUNCTION = (VOID *)gpDxFuncs[i].pfn;  \
                break;  \
            }  \
        } \
    }


DWORD
STDCALL
NtGdiDdCanCreateSurface(HANDLE hDirectDrawLocal,
                        PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    PGD_DDCANCREATESURFACE pfnDdCanCreateSurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCanCreateSurface, pfnDdCanCreateSurface);

    if (pfnDdCanCreateSurface == NULL)
    {
        DPRINT1("Warring no pfnDdCanCreateSurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCanCreateSurface");
    return pfnDdCanCreateSurface(hDirectDrawLocal,puCanCreateSurfaceData);
}

























#define DdHandleTable GdiHandleTable

/* 
   DdMapMemory, DdDestroyDriver are not exported as NtGdi calls
   This file is complete for DD_CALLBACKS setup

   TODO: Fix the NtGdiDdCreateSurface (something is missing in
   either GdiEntry or GdiEntry's callback for DdCreateSurface)
*/

/************************************************************************/
/* NtGdiDdCreateSurface                                                 */
/* Status : Bugs out                                                    */
/************************************************************************/

DWORD STDCALL NtGdiDdCreateSurface(
    HANDLE hDirectDrawLocal,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
)
{
    INT i;
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    NTSTATUS Status = FALSE;
    PDD_DIRECTDRAW pDirectDraw;
    PDD_SURFACE phsurface;

    PDD_SURFACE_LOCAL pLocal;
    PDD_SURFACE_MORE pMore;
    PDD_SURFACE_GLOBAL pGlobal;

    DD_CREATESURFACEDATA CreateSurfaceData;

    /* FIXME: Alloc as much as needed */
    PHANDLE *myhSurface;

    /* GCC4 gives warnings about uninitialized
       values, but they are initialized in SEH */

    DPRINT1("NtGdiDdCreateSurface\n");

    DPRINT1("Copy puCreateSurfaceData to kmode CreateSurfaceData\n");
    _SEH_TRY
    {
        ProbeForRead(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        RtlCopyMemory( &CreateSurfaceData, puCreateSurfaceData,
                       sizeof( DD_CREATESURFACEDATA ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return ddRVal;
    }

    /* FIXME: There is only support for one surface at the moment.
       This is a hack to prevent more than one surface creation */
    if (CreateSurfaceData.dwSCnt > 1)
    {
        DPRINT1("HACK: Limiting quantity of created surfaces from %d to 1!\n",
            CreateSurfaceData.dwSCnt);
        CreateSurfaceData.dwSCnt = 1;
    }


    DPRINT1("Setup surface in put handler\n");
    myhSurface = ExAllocatePool(PagedPool, CreateSurfaceData.dwSCnt * sizeof(HANDLE));

    _SEH_TRY
    {
        ProbeForRead(hSurface, CreateSurfaceData.dwSCnt * sizeof(HANDLE), 1);
        for (i=0;i<CreateSurfaceData.dwSCnt;i++)
        {
            myhSurface[i] = hSurface[i];
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return ddRVal;
    }

    /* Check if a surface has been created */
    for (i=0;i<CreateSurfaceData.dwSCnt;i++)
    {
        if (!myhSurface[i])
        {
            myhSurface[i] = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DD_SURFACE);
            if (!myhSurface[i])
            {
                /* FIXME: lock myhSurface*/
                /* FIXME: free myhSurface, and the contain */
                /* FIXME: add to attach list */
                return ddRVal;
            }
            else
            {
                /* FIXME: lock myhSurface*/
                /* FIXME: add to attach list */
            }
        }
    }

    /* FIXME: more than one surface is not supported here, once again */
    /* FIXME: we need release  myhSurface before any exits */

    phsurface = GDIOBJ_LockObj(DdHandleTable, myhSurface[0], GDI_OBJECT_TYPE_DD_SURFACE);
    if (!phsurface)
    {
        return ddRVal;
    }

    DPRINT1("Copy puCreateSurfaceData to kmode CreateSurfaceData\n");
    _SEH_TRY
    {
        ProbeForRead(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        RtlCopyMemory( &CreateSurfaceData, puCreateSurfaceData,
                       sizeof( DD_CREATESURFACEDATA ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceGlobalData to kmode phsurface->Global\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceGlobalData,  sizeof(DD_SURFACE_GLOBAL), 1);
        RtlCopyMemory( &phsurface->Global, puSurfaceGlobalData,
                       sizeof( DD_SURFACE_GLOBAL ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceMoreData to kmode phsurface->More\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceMoreData,  sizeof(DD_SURFACE_MORE), 1);
        RtlCopyMemory( &phsurface->More, puSurfaceMoreData,
                       sizeof( DD_SURFACE_MORE ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceLocalData to kmode phsurface->Local\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceLocalData,  sizeof(DD_SURFACE_LOCAL), 1);
        RtlCopyMemory( &phsurface->Local, puSurfaceLocalData,
                       sizeof( DD_SURFACE_LOCAL ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceDescription to kmode phsurface->desc\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceDescription,  sizeof(DDSURFACEDESC), 1);
        RtlCopyMemory( &phsurface->desc, puSurfaceDescription,
                       sizeof( DDSURFACEDESC ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Lock hDirectDrawLocal \n");
    phsurface->hDirectDrawLocal = hDirectDrawLocal;
    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        DPRINT1("fail \n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        return ddRVal;
    }


    /* FIXME: unlock phsurface, free phsurface at failure */
    /* FIXME: unlock hsurface, free phsurface at failure */
    /* FIXME: add support for more than one surface create, once more */
    /* FIXME: alloc memory if it's more than one surface (5th mention
              that this long function doesn't support more than one
              surface creation. I guess it was easier to support it
              than to write such comments 10 times). */

    pLocal = &phsurface->Local;
    pMore = &phsurface->More;
    pGlobal = &phsurface->Global;

    for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
    {
        phsurface->lcllist[i] = (PDD_SURFACE_LOCAL)pLocal;
        pLocal->lpGbl = pGlobal;
        pLocal->lpSurfMore = pMore;

        /* FIXME ?
        pLocal->lpAttachList;
        pLocal->lpAttachListFrom;
        */
        
       /* FIXME: a countup to next pLocal, pMore, pGlobal */

    }

    /* Setup DD_CREATESURFACEDATA CreateSurfaceData for the driver */
    CreateSurfaceData.lplpSList = (PDD_SURFACE_LOCAL *) &phsurface->lcllist;
    CreateSurfaceData.lpDDSurfaceDesc = &phsurface->desc;
    CreateSurfaceData.CreateSurface = NULL;
    CreateSurfaceData.ddRVal = DDERR_GENERIC;
    CreateSurfaceData.lpDD = &pDirectDraw->Global;

    /* CreateSurface must crash with lcl converting */
    if ((pDirectDraw->DD.dwFlags & DDHAL_CB32_CREATESURFACE))
    {
        DPRINT1("0x%04x",pDirectDraw->DD.CreateSurface);

        ddRVal = pDirectDraw->DD.CreateSurface(&CreateSurfaceData);
    }

    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,CreateSurfaceData.ddRVal);

    /* FIXME: support for more that one surface (once more!!!!) */
    _SEH_TRY
    {
        ProbeForWrite(puSurfaceDescription,  sizeof(DDSURFACEDESC), 1);
        RtlCopyMemory( puSurfaceDescription, &phsurface->desc, sizeof( DDSURFACEDESC ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    _SEH_TRY
    {
        ProbeForWrite(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        puCreateSurfaceData->ddRVal =  CreateSurfaceData.ddRVal;
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
    {
        _SEH_TRY
        {
            ProbeForWrite(puSurfaceGlobalData,  sizeof(DD_SURFACE_GLOBAL), 1);
            RtlCopyMemory( puSurfaceGlobalData, &phsurface->Global, sizeof( DD_SURFACE_GLOBAL ) );
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        _SEH_TRY
        {
            ProbeForWrite(puSurfaceLocalData,  sizeof(DD_SURFACE_LOCAL), 1);
            RtlCopyMemory( puSurfaceLocalData, &phsurface->Local, sizeof( DD_SURFACE_LOCAL ) );
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        _SEH_TRY
        {
            ProbeForWrite(puSurfaceMoreData,  sizeof(DD_SURFACE_MORE), 1);
            RtlCopyMemory( puSurfaceMoreData, &phsurface->More, sizeof( DD_SURFACE_MORE ) );

            puSurfaceLocalData->lpGbl = puSurfaceGlobalData;
            puSurfaceLocalData->lpSurfMore = puSurfaceMoreData;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        DPRINT1("GDIOBJ_UnlockObjByPtr\n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        _SEH_TRY
        {
            ProbeForWrite(puhSurface, sizeof(HANDLE), 1);
            puhSurface[i] = myhSurface[i];
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }


    /* FIXME: Fil the return handler */
    DPRINT1("GDIOBJ_UnlockObjByPtr\n");
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

    DPRINT1("Return value is %04x and driver return code is %04x\n",
        ddRVal, CreateSurfaceData.ddRVal);
    return ddRVal;
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/* status : Works as intended                                           */
/************************************************************************/


DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    NTSTATUS Status = FALSE;
    DD_WAITFORVERTICALBLANKDATA WaitForVerticalBlankData;
    LPDDHAL_WAITFORVERTICALBLANKDATA pWaitForVerticalBlankData = (LPDDHAL_WAITFORVERTICALBLANKDATA)puWaitForVerticalBlankData;

    if ((hDirectDrawLocal) &&
        (puWaitForVerticalBlankData))
    {
        RtlZeroMemory(&WaitForVerticalBlankData,sizeof(DD_WAITFORVERTICALBLANKDATA));

        _SEH_TRY
        {
            ProbeForRead(pWaitForVerticalBlankData, sizeof(DDHAL_WAITFORVERTICALBLANKDATA), 1);
            WaitForVerticalBlankData.dwFlags = pWaitForVerticalBlankData->dwFlags;
            WaitForVerticalBlankData.bIsInVB = pWaitForVerticalBlankData->bIsInVB;
            WaitForVerticalBlankData.hEvent = pWaitForVerticalBlankData->hEvent;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
            pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

            if (pDirectDraw != NULL) 
            {
                if (pDirectDraw->DD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
                {
                    WaitForVerticalBlankData.ddRVal = DDERR_GENERIC;
                    WaitForVerticalBlankData.lpDD =  &pDirectDraw->Global;;
                    ddRVal = pDirectDraw->DD.WaitForVerticalBlank(&WaitForVerticalBlankData);
                }
                _SEH_TRY
                {
                    ProbeForWrite(pWaitForVerticalBlankData,  sizeof(DDHAL_WAITFORVERTICALBLANKDATA), 1);
                    pWaitForVerticalBlankData->ddRVal  = WaitForVerticalBlankData.ddRVal;
                    pWaitForVerticalBlankData->bIsInVB = WaitForVerticalBlankData.bIsInVB;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
            }
        }
    }
    return ddRVal;
}




/************************************************************************/
/* GetScanLine                                                          */
/* status : This func is now documented in MSDN, and now it's compatible*/
/*          with Windows 2000 implementation                            */
/************************************************************************/
DWORD STDCALL 
NtGdiDdGetScanLine( HANDLE hDirectDrawLocal, PDD_GETSCANLINEDATA puGetScanLineData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_GETSCANLINEDATA GetScanLineData;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    NTSTATUS Status = FALSE;
    LPDDHAL_GETSCANLINEDATA ourpuGetScanLineData;

    if (hDirectDrawLocal)
    {
        pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);;
    }

    if (pDirectDraw != NULL)
    {
        DPRINT1("GetScanLine\n");
        if (pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE)
        {
            RtlZeroMemory(&GetScanLineData,sizeof(DD_GETSCANLINEDATA));
            GetScanLineData.ddRVal = DDERR_GENERIC;
            GetScanLineData.lpDD = &pDirectDraw->Global;
            ddRVal = pDirectDraw->DD.GetScanLine(&GetScanLineData);

            DPRINT1("GetScanLine\n");
            _SEH_TRY
            {
                ProbeForWrite(puGetScanLineData,  sizeof(DD_GETSCANLINEDATA), 1);
                ourpuGetScanLineData = (LPDDHAL_GETSCANLINEDATA)puGetScanLineData;
                ourpuGetScanLineData->dwScanLine = GetScanLineData.dwScanLine;
                ourpuGetScanLineData->ddRVal     = GetScanLineData.ddRVal;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("GetScanLine\n");
                ddRVal = DDHAL_DRIVER_NOTHANDLED;
            }
        }

        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
    }

  return ddRVal;
}
