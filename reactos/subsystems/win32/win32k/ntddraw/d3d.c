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

/************************************************************************/
/* NtGdiDdCanCreateD3DBuffer                                            */
/************************************************************************/
DWORD
STDCALL
NtGdiDdCanCreateD3DBuffer(HANDLE hDirectDraw,
                          PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
    PGD_DDCANCREATED3DBUFFER pfnDdCanCreateD3DBuffer = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCanCreateD3DBuffer, pfnDdCanCreateD3DBuffer);

    if (pfnDdCanCreateD3DBuffer == NULL)
    {
        DPRINT1("Warring no pfnDdCanCreateD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCanCreateD3DBuffer");
    return pfnDdCanCreateD3DBuffer(hDirectDraw,puCanCreateSurfaceData);
}

/************************************************************************/
/* NtGdiD3dContextCreate                                                */
/************************************************************************/
BOOL 
STDCALL
NtGdiD3dContextCreate(HANDLE hDirectDrawLocal,
                      HANDLE hSurfColor,
                      HANDLE hSurfZ,
                      D3DNTHAL_CONTEXTCREATEDATA* pdcci)
{
    PGD_D3DCONTEXTCREATE pfnD3dContextCreate = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxD3dContextCreate, pfnD3dContextCreate);

    if (pfnD3dContextCreate == NULL)
    {
        DPRINT1("Warring no pfnD3dContextCreate");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dContextCreate");
    return pfnD3dContextCreate(hDirectDrawLocal, hSurfColor, hSurfZ, pdcci);
}

/************************************************************************/
/* NtGdiD3dContextDestroy                                               */
/************************************************************************/
DWORD
STDCALL
NtGdiD3dContextDestroy(LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData)
{
    PGD_D3DCONTEXTDESTROY pfnD3dContextDestroy = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxD3dContextDestroy, pfnD3dContextDestroy);

    if ( pfnD3dContextDestroy == NULL)
    {
        DPRINT1("Warring no pfnD3dContextDestroy");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys D3dContextDestroy");
    return pfnD3dContextDestroy(pContextDestroyData);
}

/************************************************************************/
/* NtGdiD3dContextDestroyAll                                            */
/************************************************************************/
DWORD
STDCALL
NtGdiD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad)
{
    PGD_D3DCONTEXTDESTROYALL pfnD3dContextDestroyAll = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxD3dContextDestroyAll, pfnD3dContextDestroyAll);

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




