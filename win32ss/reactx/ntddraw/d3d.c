/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsystems/win32/win32k/ntddraw/d3d.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

/* Comment 
 *   NtGdiDdLock and NtGdiDdLockD3D ultimately call the same function in dxg.sys 
 *   NtGdiDdUnlock and NtGdiDdUnlockD3D ultimately call the same function in dxg.sys 
 */

#include <win32k.h>
#include <debug.h>


/*++
* @name NtGdiDdCanCreateD3DBuffer
* @implemented
*
* The function NtGdiDdCanCreateD3DBuffer checks if you can create a 
* surface for DirectX. it redirects to dxg.sys in windows XP/2003, 
* dxkrnl.sys in vista and is fully implemented in win32k.sys in windows 2000 and below
*
* @param HANDLE hDirectDraw
* The handle we got from NtGdiDdCreateDirectDrawObject
*
* @param PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
* This contains information to check if the driver can create the buffers,
* surfaces, textures and vertexes, and how many of each the driver can create.

*
* @return 
* Depending on if the driver supports this function or not, DDHAL_DRIVER_HANDLED 
* or DDHAL_DRIVER_NOTHANDLED is returned.
* To check if the function has been successful, do a full check. 
* A full check is done by checking if the return value is DDHAL_DRIVER_HANDLED 
* and puCanCreateSurfaceData->ddRVal is set to DD_OK.
*
* @remarks.
* dxg.sys NtGdiDdCanCreateD3DBuffer and NtGdiDdCanCreateSurface calls are redirected to dxg.sys.
* Inside the dxg.sys they are redirected to the same function. Examine the memory addresses on the driver list functions
* table and you will see they are pointed to the same memory address.
*
* Before calling this function please set the puCanCreateSurfaceData->ddRVal to an error value such as DDERR_NOTUSPORTED,
* for the ddRVal will otherwise be unchanged even if an error occurs inside the driver. 
* puCanCreateSurfaceData->lpDD  is a pointer to DDRAWI_DIRECTDRAW_GBL, not PDD_DIRECTDRAW_GLOBAL as MSDN claims.
* puCanCreateSurfaceData->lpDD->hDD also needs be filled in with the handle we got from NtGdiDdCreateDirectDrawObject.
* puCreateSurfaceData->CanCreateSurface is a pointer to the real functions in the HAL or HEL, that you need fill in.
* Do not forget PDD_CANCREATESURFACEDATA is typecast of LPDDHAL_CANCREATESURFACEDATA and thuse two struct are different size,
* the correct struct is LPDDHAL_CANCREATESURFACEDATA.
*
*--*/

DWORD
APIENTRY
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
* The Function NtGdiD3dContextCreate checks if you can create a 
* context for Directx. It redirects to dxg.sys in windows XP/2003, 
* dxkrnl.sys in vista and is fully implemented in win32k.sys in windows 2000 and below
*
* @param HANDLE hDirectDrawLocal
* The handle we got from NtGdiDdCreateDirectDrawObject
*
* @param HANDLE hSurfColor
* Handle to DD_SURFACE_LOCAL to be used for the rendering target
*
* @param HANDLE hSurfZ
* Handle to a DD_SURFACE_LOCAL. It is the Z deep buffer. According MSDN if it is set to NULL nothing should happen.
*
* @param LPD3DNTHAL_CONTEXTCREATEDATA pdcci
* The buffer to create the context data
*
* @return 
* DDHAL_DRIVER_HANDLED or DDHAL_DRIVER_NOTHANDLED if the driver supports this function.
* A full check is done by checking if the return value is DDHAL_DRIVER_HANDLED 
* and pdcci->ddRVal is set to DD_OK.
*
*
* @remarks.
* dxg.sys NtGdiD3dContextCreate calls are redirected to the same function in the dxg.sys. As such they all work the same way.
*
* Before calling this function please set the pdcci->ddRVal to an error value such as DDERR_NOTSUPORTED,
* for the ddRVal will otherwise be unchanged even if an error occurs inside the driver. 
*
* pdcci->lpDDLcl is a pointer to DDRAWI_DIRECTDRAW_LCL, not DD_DIRECTDRAW_LOCAL  as MSDN claims.
* pdcci->lpDDSLcl is a pointer to DDRAWI_DDRAWSURFACE_LCL, not DD_DDDRAWSURFACE_LOCAL as MSDN claims.
* pdcci->lpDDSZLcl is a pointer to DDRAWI_DDRAWSURFACE_LCL, not DD_DDRAWSURFACE_LOCAL as MSDN claims.
* pdcci->dwhContext also needs be filled in with the handle we receive from NtGdiDdCreateDirectDrawObject.
* pdcci->dwPID the processid it belong to, that you need to fill in.
* Do not forget LPD3DNTHAL_CONTEXTCREATEDATA is typecast of LPD3DHAL_CONTEXTCREATEDATA and thuse two struct are different size,
* the correct struct is LPD3DHAL_CONTEXTCREATEDATA.
*--*/
BOOL 
APIENTRY
NtGdiD3dContextCreate(HANDLE hDirectDrawLocal,
                      HANDLE hSurfColor,
                      HANDLE hSurfZ,
                      LPD3DNTHAL_CONTEXTCREATEDATA pdcci)
{
    PGD_D3DCONTEXTCREATE pfnD3dContextCreate = (PGD_D3DCONTEXTCREATE)gpDxFuncs[DXG_INDEX_DxD3dContextCreate].pfn;

    if (pfnD3dContextCreate == NULL)
    {
		DPRINT1("Warning: no pfnD3dContextCreate");
        return FALSE;
    }

	DPRINT1("Calling dxg.sys D3dContextCreate");
    return pfnD3dContextCreate(hDirectDrawLocal, hSurfColor, hSurfZ, pdcci);
}

/*++
* @name NtGdiD3dContextDestroy
* @implemented
*
* The Function NtGdiD3dContextDestroy destorys the context data we got from NtGdiD3dContextCreate
* It redirects to dxg.sys in windows XP/2003,  dxkrnl.sys in vista and is fully implemented 
* in win32k.sys in windows 2000 and below
*
* @param LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData
* The context data we want to destroy
*
* @remarks.
* dxg.sys NtGdiD3dContextDestroy calls are redirected to the same functions in the dxg.sys. As such they all work the same way.
*
* Before calling this function please set the pContextDestroyData->ddRVal to an error value such DDERR_NOTUSPORTED,
* for the ddRVal will otherwise be unchanged even if an error occurs inside the driver. 
* pContextDestroyData->dwhContext also needs to be filled in with the handle we got from NtGdiDdCreateDirectDrawObject
*
*--*/
DWORD
APIENTRY
NtGdiD3dContextDestroy(LPD3DNTHAL_CONTEXTDESTROYDATA pContextDestroyData)
{
    PGD_D3DCONTEXTDESTROY pfnD3dContextDestroy = (PGD_D3DCONTEXTDESTROY)gpDxFuncs[DXG_INDEX_DxD3dContextDestroy].pfn;

    if ( pfnD3dContextDestroy == NULL)
    {
		DPRINT1("Warning: no pfnD3dContextDestroy");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys D3dContextDestroy");
    return pfnD3dContextDestroy(pContextDestroyData);
}

/*++
* @name NtGdiD3dContextDestroyAll
* @implemented
*
* The Function NtGdiD3dContextDestroyAll destroys all the context data in a process
* The data having been allocated with NtGdiD3dContextCreate
* It redirects to dxg.sys in windows XP/2003,  dxkrnl.sys in vista and is fully implemented 
* in win32k.sys in windows 2000 and below
*
* @param LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad
* The context data we want to destory
*
* @remarks.
* dxg.sys NtGdiD3dContextDestroy calls are redirected to the same function in the dxg.sys. As such they all work the same way.
*
* Before calling this function please set the pdcad->ddRVal to an error value such as DDERR_NOTUSPORTED,
* for the ddRVal will otherwise be unchanged even if an error occurs inside the driver. 
* pdcad->dwPID also needs to be filled in with the ID of the process that needs its context data destroyed.
*
* Warning: MSDN is wrong about this function. It claims the function queries free memory and does not accept
* any parameters. Last time MSDN checked: 19/10-2007
*--*/
DWORD
APIENTRY
NtGdiD3dContextDestroyAll(LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcad)
{
    PGD_D3DCONTEXTDESTROYALL pfnD3dContextDestroyAll = (PGD_D3DCONTEXTDESTROYALL)gpDxFuncs[DXG_INDEX_DxD3dContextDestroyAll].pfn;

    if (pfnD3dContextDestroyAll == NULL)
    {
		DPRINT1("Warning: no pfnD3dContextDestroyAll");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys D3dContextDestroyAll");
    return pfnD3dContextDestroyAll(pdcad);
}

/*++
* @name NtGdiDdCreateD3DBuffer
* @implemented
*
* The function NtGdiDdCreateD3DBuffer creates a surface for DirectX. 
* It redirects to dxg.sys in windows XP/2003,  dxkrnl.sys in vista and is fully implemented 
* in win32k.sys in windows 2000 and below
*
* @param HANDLE hDirectDraw
* The handle we got from NtGdiDdCreateDirectDrawObject
*
* @param  HANDLE *hSurface
* <FILLMEIN>
*
* @param DDSURFACEDESC puSurfaceDescription
* Surface description: what kind of surface it should be. Examples: RGB, compress, deep, and etc
* See DDSURFACEDESC for more information
*
* @param DD_SURFACE_GLOBAL *puSurfaceGlobalData
* <FILLMEIN>
*
* @param DD_SURFACE_LOCAL *puSurfaceLocalData
* <FILLMEIN>
*
* @param DD_SURFACE_MORE *puSurfaceMoreData
* <FILLMEIN>
*
* @param PDD_CREATESURFACEDATA puCreateSurfaceData
* <FILLMEIN>
*
* @param HANDLE *puhSurface
* <FILLMEIN>
*
* @return 
* Depending on if the driver supports this function or not, DDHAL_DRIVER_HANDLED 
* or DDHAL_DRIVER_NOTHANDLED is returned.
* To check if the function was successful, do a full check. 
* A full check is done by checking if the return value is DDHAL_DRIVER_HANDLED 
* and puCanCreateSurfaceData->ddRVal is set to DD_OK.
*
* @remarks.
* dxg.sys NtGdiDdCreateD3DBuffer and NtGdiDdCreateSurface calls are redirected to dxg.sys.
* Inside the dxg.sys they are redirected to the same function. Examine the memory addresses on the driver list functions
* table and you will see they are pointed to the same memory address.
*
* Before calling this function please set the puCreateSurfaceData->ddRVal to an error value such as DDERR_NOTUSPORTED,
* for the ddRVal will otherwise be unchanged even if an error occurs inside the driver. 
* puCreateSurfaceData->lpDD  is a pointer to DDRAWI_DIRECTDRAW_GBL. MSDN claims it is PDD_DIRECTDRAW_GLOBAL but it is not.
* puCreateSurfaceData->lpDD->hDD also needs to be filled in with the handle we got from NtGdiDdCreateDirectDrawObject
* puCreateSurfaceData->CreateSurface is a pointer to the real functions in the HAL or HEL, that you need fill in
*
*--*/
DWORD
APIENTRY
NtGdiDdCreateD3DBuffer(HANDLE hDirectDraw,
                       HANDLE *hSurface,
                       DDSURFACEDESC *puSurfaceDescription,
                       DD_SURFACE_GLOBAL *puSurfaceGlobalData,
                       DD_SURFACE_LOCAL *puSurfaceLocalData,
                       DD_SURFACE_MORE *puSurfaceMoreData,
                       PDD_CREATESURFACEDATA puCreateSurfaceData,
                       HANDLE *puhSurface)
{
    PGD_DDCREATED3DBUFFER pfnDdCreateD3DBuffer = (PGD_DDCREATED3DBUFFER)gpDxFuncs[DXG_INDEX_DxDdCreateD3DBuffer].pfn;

    if (pfnDdCreateD3DBuffer == NULL)
    {
        DPRINT1("Warning: no pfnDdCreateD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys DdCreateD3DBuffer");
    return pfnDdCreateD3DBuffer(hDirectDraw, hSurface,
                                puSurfaceDescription, puSurfaceGlobalData,
                                puSurfaceLocalData, puSurfaceMoreData,
                                puCreateSurfaceData, puhSurface);
}

/************************************************************************/
/* NtGdiDdDestroyD3DBuffer                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdDestroyD3DBuffer(HANDLE hSurface)
{
    PGD_DXDDDESTROYD3DBUFFER pfnDdDestroyD3DBuffer = (PGD_DXDDDESTROYD3DBUFFER)gpDxFuncs[DXG_INDEX_DxDdDestroyD3DBuffer].pfn;  
    
    if (pfnDdDestroyD3DBuffer == NULL)
    {
        DPRINT1("Warning: no pfnDdDestroyD3DBuffer");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdDestroyD3DBuffer");
    return pfnDdDestroyD3DBuffer(hSurface);
}

/************************************************************************/
/* NtGdiD3dDrawPrimitives2                                              */
/************************************************************************/
DWORD
APIENTRY
NtGdiD3dDrawPrimitives2(HANDLE hCmdBuf,
                        HANDLE hVBuf,
                        LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
                        FLATPTR *pfpVidMemCmd,
                        DWORD *pdwSizeCmd,
                        FLATPTR *pfpVidMemVtx,
                        DWORD *pdwSizeVtx)
{
    PGD_D3DDRAWPRIMITIVES2 pfnD3dDrawPrimitives2  = (PGD_D3DDRAWPRIMITIVES2)gpDxFuncs[DXG_INDEX_DxD3dDrawPrimitives2].pfn;  
    
    if (pfnD3dDrawPrimitives2 == NULL)
    {
		DPRINT1("Warning: no pfnD3dDrawPrimitives2");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys D3dDrawPrimitives2");
    return pfnD3dDrawPrimitives2(hCmdBuf,hVBuf,pded,pfpVidMemCmd,pdwSizeCmd,pfpVidMemVtx,pdwSizeVtx);
}


/************************************************************************/
/* NtGdiD3dValidateTextureStageState                                    */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdLockD3D(HANDLE hSurface,
               PDD_LOCKDATA puLockData)
{
    PGD_DXDDLOCKD3D pfnDdLockD3D  = (PGD_DXDDLOCKD3D)gpDxFuncs[DXG_INDEX_DxDdLockD3D].pfn;  
    
    if (pfnDdLockD3D == NULL)
    {
		DPRINT1("Warning: no pfnDdLockD3D");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdLockD3D");
    return pfnDdLockD3D(hSurface, puLockData);
}

/************************************************************************/
/* NtGdiD3dValidateTextureStageState                                    */
/************************************************************************/
DWORD
APIENTRY
NtGdiD3dValidateTextureStageState(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    PGD_D3DVALIDATETEXTURESTAGESTATE pfnD3dValidateTextureStageState = (PGD_D3DVALIDATETEXTURESTAGESTATE)gpDxFuncs[DXG_INDEX_DxD3dValidateTextureStageState].pfn;  
   
    if (pfnD3dValidateTextureStageState == NULL)
    {
		DPRINT1("Warning: no pfnD3dValidateTextureStageState");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys D3dValidateTextureStageState");
    return pfnD3dValidateTextureStageState(pData);
}

/************************************************************************/
/* NtGdiDdUnlockD3D                                                     */
/************************************************************************/
DWORD
APIENTRY
NtGdiDdUnlockD3D(HANDLE hSurface,
                 PDD_UNLOCKDATA puUnlockData)
{
    PGD_DXDDUNLOCKD3D pfnDdUnlockD3D = (PGD_DXDDUNLOCKD3D)gpDxFuncs[DXG_INDEX_DxDdUnlockD3D].pfn;
   
    if (pfnDdUnlockD3D == NULL)
    {
		DPRINT1("Warning: no pfnDdUnlockD3D");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling dxg.sys pfnDdUnlockD3D");
    return pfnDdUnlockD3D(hSurface, puUnlockData);

}




