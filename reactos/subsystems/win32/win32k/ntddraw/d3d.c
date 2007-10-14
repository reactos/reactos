/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/d3d.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */


#include <w32k.h>
#include <reactos/drivers/directx/dxg.h>

//#define NDEBUG
#include <debug.h>

/********************************************************************************/
/*                D3D interface from DXG.SYS                                    */
/********************************************************************************/

extern PDRVFN gpDxFuncs;



typedef DWORD (NTAPI *PGD_D3DVALIDATETEXTURESTAGESTATE)(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (NTAPI *PGD_D3DDRAWPRIMITIVES2)(HANDLE, HANDLE, LPD3DNTHAL_DRAWPRIMITIVES2DATA, FLATPTR *, DWORD *, FLATPTR *, DWORD *);
typedef DWORD (NTAPI *PGD_DDCREATED3DBUFFER)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
typedef BOOL (NTAPI *PGD_D3DCONTEXTCREATE)(HANDLE, HANDLE, HANDLE, LPD3DNTHAL_CONTEXTCREATEDATA);
typedef DWORD (NTAPI *PGD_D3DCONTEXTDESTROY)(LPD3DNTHAL_CONTEXTDESTROYDATA);
typedef DWORD (NTAPI *PGD_D3DCONTEXTDESTROYALL)(LPD3DNTHAL_CONTEXTDESTROYALLDATA);

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

