/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/d3d.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

/* Comment 
 *   NtGdiDdLock and NtGdiDdLockD3D at end calls to same api in the dxg.sys 
 *   NtGdiDdUnlock and NtGdiDdUnlockD3D at end calls to same api in the dxg.sys 
 */

#include <w32k.h>
#include <debug.h>


/*++
* @name NtGdiDdCanCreateD3DBuffer
* @implemented
*
* The function NtGdiDdCanCreateD3DBuffer checks if you can create a 
* surface for DirectX. It is redirected to dxg.sys.
*
* @param HANDLE hDirectDraw
* The handle we got from NtGdiDdCreateDirectDrawObject
*
* @param PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
* This contains information to check if the driver can create the buffers,
* surfaces, textures and vertexes, and how many of each the driver can create.

*
* @return 
* Depending on if the driver supports this API or not, DDHAL_DRIVER_HANDLED 
* or DDHAL_DRIVER_NOTHANDLED is returned.
* To check if the function has been successful, do a full check. 
* A full check is done by checking if the return value is DDHAL_DRIVER_HANDLED 
* and puCanCreateSurfaceData->ddRVal is set to DD_OK.
*
* @remarks.
* dxg.sys NtGdiDdCanCreateD3DBuffer and NtGdiDdCanCreateSurface call are redirect to dxg.sys
* inside the dxg.sys they ar redirect to same functions. examine the driver list functions
* table, the memory address you will see they are pointed to same memory address.
*
* Before call to this api please set the puCanCreateSurfaceData->ddRVal to a error value example DDERR_NOTUSPORTED.
* for the ddRVal will other wise be unchange if some error happen inside the driver. 
* puCanCreateSurfaceData->lpDD  is pointer to DDRAWI_DIRECTDRAW_GBL, MSDN say it is PDD_DIRECTDRAW_GLOBAL it is not.
* puCanCreateSurfaceData->lpDD->hDD need also be fill in with the handle we got from NtGdiDdCreateDirectDrawObject
*
*--*/

DWORD
STDCALL
NtGdiDdCanCreateD3DBuffer(HANDLE hDirectDraw,
                          PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    PGD_DDCANCREATED3DBUFFER pfnDdCanCreateD3DBuffer = (PGD_DDCANCREATED3DBUFFER)gpDxFuncs[DXG_INDEX_DxDdCanCreateD3DBuffer].pfn;

    if (pfnDdCanCreateD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdCanCreateD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCanCreateD3DBuffer");
    return pfnDdCanCreateD3DBuffer(hDirectDraw,puCanCreateSurfaceData);
}

/*++
* @name NtGdiD3dContextCreate
* @implemented
*
* The Function NtGdiDdCanCreateD3DBuffer check if you can create a 
* surface for directx it redirect the call to dxg.sys.
*
* @param HANDLE hDirectDrawLocal
* The handle we got from NtGdiDdCreateDirectDrawObject
*
* @param HANDLE hSurfColor
* Handle to DD_SURFACE_LOCAL to be use as target rendring
*
* @param HANDLE hSurfZ
* Handle to a DD_SURFACE_LOCAL it is the Z deep buffer, accdoing MSDN if it set to NULL nothing should happen.
*
* @param D3DNTHAL_CONTEXTCREATEDATA* hSurfZ
* the buffer to create the context data
*
* @return 
* DDHAL_DRIVER_HANDLED or DDHAL_DRIVER_NOTHANDLED if the driver support this api.
* Todo full check if we sussess is to check the return value DDHAL_DRIVER_HANDLED
* puCanCreateSurfaceData->ddRVal are set to DD_OK.
*
*
* @remarks.
* dxg.sys NtGdiD3dContextCreate call are redirect to 
* same functions in the dxg.sys. So they are working exacly same. everthing else is lying if they
* are diffent.
*
* Before call to this api please set the hSurfZ->ddRVal to a error value example DDERR_NOTUSPORTED.
* for the ddRVal will other wise be unchange if some error happen inside the driver. 
* hSurfZ->dwhContext need also be fill in with the handle we got from NtGdiDdCreateDirectDrawObject
*
*--*/
BOOL 
STDCALL
NtGdiD3dContextCreate(HANDLE hDirectDrawLocal,
                      HANDLE hSurfColor,
                      HANDLE hSurfZ,
                      D3DNTHAL_CONTEXTCREATEDATA* pdcci)
{
    PGD_D3DCONTEXTCREATE pfnD3dContextCreate = (PGD_D3DCONTEXTCREATE)gpDxFuncs[DXG_INDEX_DxD3dContextCreate].pfn;

    if (pfnD3dContextCreate == NULL)
    {
        DPRINT1("Warring no pfnD3dContextCreate");
        return FALSE;
    }

    DPRINT1("Calling on dxg.sys D3dContextCreate");
    return pfnD3dContextCreate(hDirectDrawLocal, hSurfColor, hSurfZ, pdcci);
}

/*++
* @name NtGdiD3dContextDestroy
* @implemented
*
* The Function NtGdiD3dContextDestroy Destory the context data we got from NtGdiD3dContextCreate
*
* @param LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
* The context data we want to destory
*
* @remarks.
* dxg.sys NtGdiD3dContextDestroy call are redirect to 
* same functions in the dxg.sys. So they are working exacly same. everthing else is lying if they
* are diffent.
*
* Before call to this api please set the pContextDestroyData->ddRVal to a error value example DDERR_NOTUSPORTED.
* for the ddRVal will other wise be unchange if some error happen inside the driver.
* pContextDestroyData->dwhContext need also be fill in with the handle we got from NtGdiDdCreateDirectDrawObject
*
*--*/
DWORD
STDCALL
NtGdiD3dContextDestroy(LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData)
{
    PGD_D3DCONTEXTDESTROY pfnD3dContextDestroy = (PGD_D3DCONTEXTDESTROY)gpDxFuncs[DXG_INDEX_DxD3dContextDestroy].pfn;

    if ( pfnD3dContextDestroy == NULL)
    {
        DPRINT1("Warring no pfnD3dContextDestroy");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dContextDestroy");
    return pfnD3dContextDestroy(pContextDestroyData);
}

/*++
* @name NtGdiD3dContextDestroyAll
* @implemented
*
* The Function NtGdiD3dContextDestroyAll Destory the all context data we got in a process
* The data have been alloc with NtGdiD3dContextCreate
*
* @param LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad
* The context data we want to destory
*
* @remarks.
* dxg.sys NtGdiD3dContextDestroyAll call are redirect to 
* same functions in the dxg.sys. So they are working exacly same. everthing else is lying if they
* are diffent.
*
* Before call to this api please set the pdcad->ddRVal to a error value example DDERR_NOTUSPORTED.
* for the ddRVal will other wise be unchange if some error happen inside the driver.
* pdcad->dwPID need also be fill in with the Process ID we need destore the data for
*
* Waring MSDN is wrong about this api. it say it queare free memory and it does not accpect
* any parama, last time checked in msdn 19/10-2007
*--*/
DWORD
STDCALL
NtGdiD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad)
{
    PGD_D3DCONTEXTDESTROYALL pfnD3dContextDestroyAll = (PGD_D3DCONTEXTDESTROYALL)gpDxFuncs[DXG_INDEX_DxD3dContextDestroyAll].pfn;

    if (pfnD3dContextDestroyAll == NULL)
    {
        DPRINT1("Warring no pfnD3dContextDestroyAll");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dContextDestroyAll");
    return pfnD3dContextDestroyAll(pdcad);
}

/************************************************************************/
/* NtGdiDdCreateD3DBuffer                                               */
/************************************************************************/
DWORD
STDCALL
NtGdiDdCreateD3DBuffer(HANDLE hDirectDraw,
                       HANDLE *hSurface,
                       DDSURFACEDESC *puSurfaceDescription,
                       DD_SURFACE_GLOBAL *puSurfaceGlobalData,
                       DD_SURFACE_LOCAL *puSurfaceLocalData,
                       DD_SURFACE_MORE *puSurfaceMoreData,
                       PDD_CREATESURFACEDATA puCreateSurfaceData,
                       HANDLE *puhSurface)
{
    PGD_DDCREATED3DBUFFER pfnDdCreateD3DBuffer = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateD3DBuffer, pfnDdCreateD3DBuffer);

    if (pfnDdCreateD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdCreateD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCreateD3DBuffer");
    return pfnDdCreateD3DBuffer(hDirectDraw, hSurface,
                                puSurfaceDescription, puSurfaceGlobalData,
                                puSurfaceLocalData, puSurfaceMoreData,
                                puCreateSurfaceData, puhSurface);
}

/************************************************************************/
/* NtGdiDdDestroyD3DBuffer                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdDestroyD3DBuffer(HANDLE hSurface)
{
    PGD_DXDDDESTROYD3DBUFFER pfnDdDestroyD3DBuffer = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDestroyD3DBuffer, pfnDdDestroyD3DBuffer);

    if (pfnDdDestroyD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdDestroyD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDestroyD3DBuffer");
    return pfnDdDestroyD3DBuffer(hSurface);
}

/************************************************************************/
/* NtGdiD3dDrawPrimitives2                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiD3dDrawPrimitives2(HANDLE hCmdBuf,
                        HANDLE hVBuf,
                        LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
                        FLATPTR *pfpVidMemCmd,
                        DWORD *pdwSizeCmd,
                        FLATPTR *pfpVidMemVtx,
                        DWORD *pdwSizeVtx)
{
    PGD_D3DDRAWPRIMITIVES2 pfnD3dDrawPrimitives2 = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxD3dDrawPrimitives2, pfnD3dDrawPrimitives2);

    if (pfnD3dDrawPrimitives2 == NULL)
    {
        DPRINT1("Warring no pfnD3dDrawPrimitives2");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dDrawPrimitives2");
    return pfnD3dDrawPrimitives2(hCmdBuf,hVBuf,pded,pfpVidMemCmd,pdwSizeCmd,pfpVidMemVtx,pdwSizeVtx);
}


/************************************************************************/
/* NtGdiD3dValidateTextureStageState                                    */
/************************************************************************/
DWORD
STDCALL
NtGdiDdLockD3D(HANDLE hSurface,
               PDD_LOCKDATA puLockData)
{
    PGD_DXDDLOCKD3D pfnDdLockD3D  = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdLockD3D, pfnDdLockD3D);

    if (pfnDdLockD3D == NULL)
    {
        DPRINT1("Warring no pfnDdLockD3D");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdLockD3D");
    return pfnDdLockD3D(hSurface, puLockData);
}

/************************************************************************/
/* NtGdiD3dValidateTextureStageState                                    */
/************************************************************************/
DWORD
STDCALL
NtGdiD3dValidateTextureStageState(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    PGD_D3DVALIDATETEXTURESTAGESTATE pfnD3dValidateTextureStageState = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxD3dValidateTextureStageState, pfnD3dValidateTextureStageState);

    if (pfnD3dValidateTextureStageState == NULL)
    {
        DPRINT1("Warring no pfnD3dValidateTextureStageState");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dValidateTextureStageState");
    return pfnD3dValidateTextureStageState(pData);
}

/************************************************************************/
/* NtGdiDdUnlockD3D                                                     */
/************************************************************************/
DWORD
STDCALL
NtGdiDdUnlockD3D(HANDLE hSurface,
                 PDD_UNLOCKDATA puUnlockData)
{
    PGD_DXDDUNLOCKD3D pfnDdUnlockD3D = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdUnlockD3D, pfnDdUnlockD3D);

    if (pfnDdUnlockD3D == NULL)
    {
        DPRINT1("Warring no pfnDdUnlockD3D");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdUnlockD3D");
    return pfnDdUnlockD3D(hSurface, puUnlockData);

}




