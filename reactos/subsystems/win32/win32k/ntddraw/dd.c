/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dd.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/7-2006  Magnus Olsen
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define DdHandleTable GdiHandleTable

/* 
   DdMapMemory, DdDestroyDriver are not exported as NtGdi Call 
   This file is compelete for DD_CALLBACKS setup

   ToDO fix the NtGdiDdCreateSurface, shall we fix it 
   from GdiEntry or gdientry callbacks for DdCreateSurface
   have we miss some thing there 
*/

/************************************************************************/
/* NtGdiDdCreateSurface                                                 */
/* status : Bugs out                                                    */
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
    HANDLE hsurface;
    PDD_SURFACE phsurface;

    PDD_SURFACE_LOCAL pLocal;
    PDD_SURFACE_MORE pMore;
    PDD_SURFACE_GLOBAL pGlobal;

    /* FIXME alloc so mayne we need */
    PHANDLE *myhSurface[1];

    /* GCC4  warnns on value are unisitaed,
       but they are initated in seh 
    */
    myhSurface[0] = 0;

    DPRINT1("NtGdiDdCreateSurface\n");

    _SEH_TRY
    {
        ProbeForRead(hSurface,  sizeof(HANDLE), 1);
        ProbeForRead(hSurface[0],  sizeof(HANDLE), 1);
        myhSurface[0] = hSurface[0];
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


    /* Create a surface */
    hsurface =  GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DD_SURFACE);
      if (!hsurface)
    {
        return ddRVal;
    }

    phsurface = GDIOBJ_LockObj(DdHandleTable, hsurface, GDI_OBJECT_TYPE_DD_SURFACE);
    if (!phsurface)
    {
        return ddRVal;
    }

    DPRINT1("Copy puCreateSurfaceData to kmode CreateSurfaceData\n");
    _SEH_TRY
    {
        ProbeForRead(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        RtlCopyMemory( &phsurface->CreateSurfaceData, puCreateSurfaceData,
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


    /* FIXME unlock phsurface free phsurface at fail*/
    /* FIXME unlock hsurface free phsurface at fail*/
    /* FIXME add support for more that one surface create */
    /* FIXME alloc memory if it more that one surface */

    pLocal = &phsurface->Local;
    pMore = &phsurface->More;
    pGlobal = &phsurface->Global;

    /* FIXME we only support one surface for now */
    phsurface->CreateSurfaceData.dwSCnt = 1;

    
    phsurface->lcllist[0] = 0;
    phsurface->lcllist[1] = 0;
    phsurface->CreateSurfaceData.lplpSList = (PDD_SURFACE_LOCAL *) &phsurface->lcllist;

    i = 0;
    // for (i = 0; i < phsurface->CreateSurfaceData.dwSCnt; i++)
    //{
        phsurface->lcl.hDDSurface = (ULONG_PTR)myhSurface[i];
        phsurface->lcl.ddsCaps.dwCaps = pLocal->ddsCaps.dwCaps;
        phsurface->lcl.dwFlags =pLocal->dwFlags;

        phsurface->gpl.wWidth = pGlobal->wWidth;
        phsurface->gpl.wHeight = pGlobal->wHeight;
        phsurface->gpl.lPitch = pGlobal->lPitch;
        phsurface->gpl.fpVidMem = pGlobal->fpVidMem;
        phsurface->gpl.dwBlockSizeX = pGlobal->dwBlockSizeX;
        phsurface->gpl.dwBlockSizeY = pGlobal->dwBlockSizeY;
        RtlCopyMemory( &phsurface->gpl.ddpfSurface , &pGlobal->ddpfSurface, sizeof(DDPIXELFORMAT));

        /* FIXME more ?? */
        if (pMore)
        {
            phsurface->more.ddsCapsEx.dwCaps2 = pMore->ddsCapsEx.dwCaps2;
            phsurface->more.ddsCapsEx.dwCaps3 = pMore->ddsCapsEx.dwCaps3;
            phsurface->more.ddsCapsEx.dwCaps4 = pMore->ddsCapsEx.dwCaps4;
            phsurface->lcl.dbnOverlayNode.object_int = (LPDDRAWI_DDRAWSURFACE_INT)pMore->dwSurfaceHandle;
        }

        phsurface->lcllist[0] = (PDD_SURFACE_LOCAL)&phsurface->lcl;
        /* FIXME count up everthing to next position */
   // }


    /* FIXME support for more that one surface */

    DPRINT1("setup CreateSurfaceData \n");
    /* setup DD_CREATESURFACEDATA CreateSurfaceData for the driver */
    phsurface->lcl.lpGbl = &phsurface->gpl;
    phsurface->lcl.lpSurfMore = &phsurface->more;

    /* FIXME all phsurface->lcl should be in a array then add to lplpSList */
    //phsurface->CreateSurfaceData.lplpSList = (PDD_SURFACE_LOCAL  *)&phsurface->lcl;
    phsurface->CreateSurfaceData.lpDDSurfaceDesc = &phsurface->desc;
    phsurface->CreateSurfaceData.CreateSurface = NULL;
    phsurface->CreateSurfaceData.ddRVal = DDERR_GENERIC;
    phsurface->CreateSurfaceData.lpDD = &pDirectDraw->Global;

    /* is this correct the 3d drv whant this data */
    // phsurface->lcl.lpGbl->lpDD = &pDirectDraw->Global;




    /* the CreateSurface crash with lcl convering */
    DPRINT1("DDHAL_CB32_CREATESURFACE\n");
    if ((pDirectDraw->DD.dwFlags & DDHAL_CB32_CREATESURFACE))
    {
        DPRINT1("0x%04x",pDirectDraw->DD.CreateSurface);

        ddRVal = pDirectDraw->DD.CreateSurface(&phsurface->CreateSurfaceData);
    }

    /* FIXME copy data from DDRAWI_  sturct to DD_ struct */

    /* FIXME fillin the return handler */
    DPRINT1("GDIOBJ_UnlockObjByPtr\n");
    GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
    DPRINT1("GDIOBJ_UnlockObjByPtr\n");
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);


    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,phsurface->CreateSurfaceData.ddRVal);
    return ddRVal;
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/* status : OK working as it should                                     */
/************************************************************************/


DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    NTSTATUS Status = FALSE;
    DD_WAITFORVERTICALBLANKDATA WaitForVerticalBlankData;

    DPRINT1("NtGdiDdWaitForVerticalBlank\n");

    _SEH_TRY
    {
            ProbeForRead(puWaitForVerticalBlankData, sizeof(DD_WAITFORVERTICALBLANKDATA), 1);
            RtlCopyMemory(&WaitForVerticalBlankData,puWaitForVerticalBlankData, sizeof(DD_WAITFORVERTICALBLANKDATA));
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
                ProbeForWrite(puWaitForVerticalBlankData,  sizeof(DD_WAITFORVERTICALBLANKDATA), 1);
                puWaitForVerticalBlankData->ddRVal  = WaitForVerticalBlankData.ddRVal;
                puWaitForVerticalBlankData->bIsInVB = WaitForVerticalBlankData.bIsInVB;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }
    return ddRVal;
}


/************************************************************************/
/* CanCreateSurface                                                     */
/* status : OK working as it should                                     */
/************************************************************************/

DWORD STDCALL NtGdiDdCanCreateSurface(
    HANDLE hDirectDrawLocal,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_CANCREATESURFACEDATA CanCreateSurfaceData;
    DDSURFACEDESC           desc;
    NTSTATUS Status = FALSE;
    PDD_DIRECTDRAW pDirectDraw = NULL;

    DPRINT1("NtGdiDdCanCreateSurface\n");

    _SEH_TRY
    {
            ProbeForRead(puCanCreateSurfaceData,  sizeof(DD_CANCREATESURFACEDATA), 1);
            RtlCopyMemory(&CanCreateSurfaceData,puCanCreateSurfaceData, sizeof(DD_CANCREATESURFACEDATA));

            /* FIXME can be version 2 of DDSURFACEDESC */
            ProbeForRead(puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC), 1);
            RtlCopyMemory(&desc,puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
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
            if (pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
            {
                CanCreateSurfaceData.ddRVal = DDERR_GENERIC;
                CanCreateSurfaceData.lpDD = &pDirectDraw->Global;
                CanCreateSurfaceData.lpDDSurfaceDesc = &desc;
                ddRVal = pDirectDraw->DD.CanCreateSurface(&CanCreateSurfaceData);

                _SEH_TRY
                {
                     ProbeForWrite(puCanCreateSurfaceData, sizeof(DD_CANCREATESURFACEDATA), 1);
                     puCanCreateSurfaceData->ddRVal = CanCreateSurfaceData.ddRVal;

                     /* FIXME can be version 2 of DDSURFACEDESC */
                     ProbeForWrite(puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC), 1);
                     RtlCopyMemory(puCanCreateSurfaceData->lpDDSurfaceDesc,&desc, sizeof(DDSURFACEDESC));

                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }

  return ddRVal;
}

/************************************************************************/
/* GetScanLine                                                          */
/* status : not implement, was undoc in msdn now it is doc              */
/************************************************************************/
DWORD STDCALL 
NtGdiDdGetScanLine( HANDLE hDirectDrawLocal, PDD_GETSCANLINEDATA puGetScanLineData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_GETSCANLINEDATA GetScanLineData;
    PDD_DIRECTDRAW pDirectDraw;
    NTSTATUS Status = FALSE;

    DPRINT1("NtGdiDdGetScanLine\n");

    _SEH_TRY
    {
            ProbeForRead(puGetScanLineData,  sizeof(DD_GETSCANLINEDATA), 1);
            RtlCopyMemory(&GetScanLineData,puGetScanLineData, sizeof(DD_GETSCANLINEDATA));
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(NT_SUCCESS(Status))
    {
        pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);;
        if (pDirectDraw != NULL)
        {
            if (pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE)
            {
                GetScanLineData.ddRVal = DDERR_GENERIC;
                GetScanLineData.lpDD = &pDirectDraw->Global;
                ddRVal = pDirectDraw->DD.GetScanLine(&GetScanLineData);

                _SEH_TRY
                {
                    ProbeForWrite(puGetScanLineData,  sizeof(DD_GETSCANLINEDATA), 1);
                    puGetScanLineData->dwScanLine = GetScanLineData.dwScanLine;
                    puGetScanLineData->ddRVal     = GetScanLineData.ddRVal;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }

  return ddRVal;
}
