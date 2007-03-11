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
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    PDD_DIRECTDRAW pDirectDraw;
	PDD_DIRECTDRAW_GLOBAL lgpl;

	DPRINT1("NtGdiDdCreateSurface\n");

	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

	if (pDirectDraw != NULL) 
	{       					
	
		/* 
		   FIXME Get the darn surface handle and and put all 
		   surface struct to it 

		   FIXME rewrite the darn code complete 

		   FIXME fill the puCreateSurfaceData correct

		   FIXME loading back info from it right 
	     */

		if ((pDirectDraw->DD.dwFlags & DDHAL_CB32_CREATESURFACE))
		{					        	        
           /* backup the orignal PDev and info */
		   lgpl = puCreateSurfaceData->lpDD;

		   /* use our cache version instead */
		   puCreateSurfaceData->lpDD = &pDirectDraw->Global;

		   /* make the call */
		   ddRVal = pDirectDraw->DD.CreateSurface(puCreateSurfaceData);	

		   /* But back the orignal PDev */
	       puCreateSurfaceData->lpDD = lgpl;

	    }
	       
	    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);	
	}
	
	return ddRVal;
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/* status : OK working as it should                                     */
/************************************************************************/


DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData
)
{
	DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
	PDD_DIRECTDRAW_GLOBAL lgpl;
    PDD_DIRECTDRAW pDirectDraw;

	DPRINT1("NtGdiDdWaitForVerticalBlank\n");

	pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
	
	if (pDirectDraw != NULL) 
	{	
		if (pDirectDraw->DD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
		{
			lgpl = puWaitForVerticalBlankData->lpDD;	
			puWaitForVerticalBlankData->lpDD = &pDirectDraw->Global;        	

  	        ddRVal = pDirectDraw->DD.WaitForVerticalBlank(puWaitForVerticalBlankData);
	
	        puWaitForVerticalBlankData->lpDD = lgpl;            
	     }
		 GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
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
            ProbeForRead(&CanCreateSurfaceData.lpDDSurfaceDesc, sizeof(DDSURFACEDESC), 1);
            RtlCopyMemory(&desc,&CanCreateSurfaceData.lpDDSurfaceDesc, sizeof(DD_CANCREATESURFACEDATA));
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
            }
        }
        GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
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
	PDD_DIRECTDRAW_GLOBAL lgpl;	

	PDD_DIRECTDRAW pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

	DPRINT1("NtGdiDdGetScanLine\n");

	if (pDirectDraw != NULL)
	{

		if (pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE)
		{	
			lgpl = puGetScanLineData->lpDD;	
			puGetScanLineData->lpDD = &pDirectDraw->Global;

	        ddRVal = pDirectDraw->DD.GetScanLine(puGetScanLineData);	

	        puGetScanLineData->lpDD = lgpl;
		}

		GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
	}

  return ddRVal;
}
