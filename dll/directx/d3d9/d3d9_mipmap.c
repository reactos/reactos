/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_mipmap.c
 * PURPOSE:         d3d9.dll internal mip map surface functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_mipmap.h"
#include "debug.h"
#include "d3d9_texture.h"
#include "d3d9_device.h"
#include "d3d9_helpers.h"
#include <d3d9.h>

#define LOCK_D3DDEVICE9()   D3D9BaseObject_LockDevice(&This->BaseTexture.BaseResource.BaseObject)
#define UNLOCK_D3DDEVICE9() D3D9BaseObject_UnlockDevice(&This->BaseTexture.BaseResource.BaseObject)

/* Convert a IDirect3DTexture9 pointer safely to the internal implementation struct */
LPD3D9MIPMAP IDirect3DTexture9ToImpl(LPDIRECT3DTEXTURE9 iface)
{
    if (NULL == iface)
        return NULL;

    return (LPD3D9MIPMAP)((ULONG_PTR)iface - FIELD_OFFSET(D3D9MipMap, lpVtbl));
}

/* IUnknown */
static HRESULT WINAPI D3D9MipMap_QueryInterface(LPDIRECT3DTEXTURE9 iface, REFIID riid, void** ppvObject)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirect3DTexture9) ||
        IsEqualGUID(riid, &IID_IDirect3DBaseTexture9) ||
        IsEqualGUID(riid, &IID_IDirect3DResource9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return D3D_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI D3D9MipMap_AddRef(LPDIRECT3DTEXTURE9 iface)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    return D3D9BaseObject_AddRef(&This->BaseTexture.BaseResource.BaseObject);
}

ULONG WINAPI D3D9MipMap_Release(LPDIRECT3DTEXTURE9 iface)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    return D3D9BaseObject_Release(&This->BaseTexture.BaseResource.BaseObject);
}

/* IDirect3DResource9 */

/*++
* @name IDirect3DTexture9::GetDevice
* @implemented
*
* The function D3D9MipMap_GetDevice sets the ppDevice argument
* to the device connected to create the swap chain.
*
* @param LPDIRECT3DTEXTURE9 iface
* Pointer to a IDirect3DTexture9 object returned from IDirect3D9Device::CreateTexture()
*
* @param IDirect3DDevice9** ppDevice
* Pointer to a IDirect3DDevice9* structure to be set to the device object.
*
* @return HRESULT
* If the method successfully sets the ppDevice value, the return value is D3D_OK.
* If ppDevice is a bad pointer the return value will be D3DERR_INVALIDCALL.
* If the texture didn't contain any device, the return value will be D3DERR_INVALIDDEVICE.
*
*/
HRESULT WINAPI D3D9MipMap_GetDevice(LPDIRECT3DTEXTURE9 iface, IDirect3DDevice9** ppDevice)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (NULL == ppDevice)
    {
        DPRINT1("Invalid ppDevice parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(D3D9BaseObject_GetDevice(&This->BaseTexture.BaseResource.BaseObject, ppDevice)))
    {
        DPRINT1("Invalid This parameter specified");
        UNLOCK_D3DDEVICE9();
        return D3DERR_INVALIDDEVICE;
    }

    UNLOCK_D3DDEVICE9();
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_SetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_GetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_FreePrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid)
{
    UNIMPLEMENTED
    return D3D_OK;
}

DWORD WINAPI D3D9MipMap_SetPriority(LPDIRECT3DTEXTURE9 iface, DWORD PriorityNew)
{
    UNIMPLEMENTED
    return 0;
}

DWORD WINAPI D3D9MipMap_GetPriority(LPDIRECT3DTEXTURE9 iface)
{
    UNIMPLEMENTED
    return 0;
}

void WINAPI D3D9MipMap_PreLoad(LPDIRECT3DTEXTURE9 iface)
{
    UNIMPLEMENTED
}

/* IDirect3DBaseTexture9 */
D3DRESOURCETYPE WINAPI D3D9MipMap_GetType(LPDIRECT3DTEXTURE9 iface)
{
    UNIMPLEMENTED
    return D3DRTYPE_TEXTURE;
}

DWORD WINAPI D3D9MipMap_SetLOD(LPDIRECT3DTEXTURE9 iface, DWORD LODNew)
{
    UNIMPLEMENTED
    return 0;
}

/*++
* @name IDirect3DTexture9::GetLOD
* @implemented
*
* The function D3D9MipMap_GetLOD returns the number
* max LODs for the specified texture, if it's managed.
*
* @param LPDIRECT3DTEXTURE9 iface
* Pointer to a IDirect3DTexture9 object returned from IDirect3D9Device::CreateTexture().
*
* @return DWORD
* Returns the number of LODs in the specified texture if it's managed.
*
*/
DWORD WINAPI D3D9MipMap_GetLOD(LPDIRECT3DTEXTURE9 iface)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    return D3D9Texture_GetLOD( (IDirect3DBaseTexture9*)&This->BaseTexture.lpVtbl );
}

/*++
* @name IDirect3DTexture9::GetLevelCount
* @implemented
*
* The function D3D9MipMap_GetLevelCount returns the number of mip map levels
* in the specified texture.
*
* @param LPDIRECT3DTEXTURE9 iface
* Pointer to a IDirect3DTexture9 object returned from IDirect3D9Device::CreateTexture().
*
* @return DWORD
* Returns the number of levels in the specified texture.
*
*/
DWORD WINAPI D3D9MipMap_GetLevelCount(LPDIRECT3DTEXTURE9 iface)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    return D3D9Texture_GetLevelCount( (IDirect3DBaseTexture9*)&This->BaseTexture.lpVtbl );
}

HRESULT WINAPI D3D9MipMap_SetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType)
{
    UNIMPLEMENTED
    return D3D_OK;
}

/*++
* @name IDirect3DTexture9::GetAutoGenFilterType
* @implemented
*
* The function D3D9MipMap_GetAutoGenFilterType returns filter type
* that is used when automated mipmaping is set.
*
* @param LPDIRECT3DTEXTURE9 iface
* Pointer to a IDirect3DTexture9 object returned from IDirect3D9Device::CreateTexture().
*
* @return D3DTEXTUREFILTERTYPE
* Filter type used when automated mipmaping is set for the specified texture.
*
*/
D3DTEXTUREFILTERTYPE WINAPI D3D9MipMap_GetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface)
{
    LPD3D9MIPMAP This = IDirect3DTexture9ToImpl(iface);
    return D3D9Texture_GetAutoGenFilterType( (IDirect3DBaseTexture9*)&This->BaseTexture.lpVtbl );
}

void WINAPI D3D9MipMap_GenerateMipSubLevels(LPDIRECT3DTEXTURE9 iface)
{
    UNIMPLEMENTED
}

/* IDirect3DTexture9 */
HRESULT WINAPI D3D9MipMap_GetLevelDesc(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_GetSurfaceLevel(LPDIRECT3DTEXTURE9 iface, UINT Level, IDirect3DSurface9** ppSurfaceLevel)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_LockRect(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_UnlockRect(LPDIRECT3DTEXTURE9 iface, UINT Level)
{
    UNIMPLEMENTED
    return D3D_OK;
}

HRESULT WINAPI D3D9MipMap_AddDirtyRect(LPDIRECT3DTEXTURE9 iface, CONST RECT* pDirtyRect)
{
    UNIMPLEMENTED
    return D3D_OK;
}

#if !defined(__cplusplus) || defined(CINTERFACE)
static IDirect3DTexture9Vtbl D3D9MipMap_Vtbl =
{
    /* IUnknown methods */
    D3D9MipMap_QueryInterface,
    D3D9MipMap_AddRef,
    D3D9MipMap_Release,

    /* IDirect3DBaseTexture9 methods */
    D3D9MipMap_GetDevice,
    D3D9MipMap_SetPrivateData,
    D3D9MipMap_GetPrivateData,
    D3D9MipMap_FreePrivateData,
    D3D9MipMap_SetPriority,
    D3D9MipMap_GetPriority,
    D3D9MipMap_PreLoad,

    /* IDirect3DBaseTexture9 methods */
    D3D9MipMap_GetType,
    D3D9MipMap_SetLOD,
    D3D9MipMap_GetLOD,
    D3D9MipMap_GetLevelCount,
    D3D9MipMap_SetAutoGenFilterType,
    D3D9MipMap_GetAutoGenFilterType,
    D3D9MipMap_GenerateMipSubLevels,

    /* IDirect3DTexture9 methods */
    D3D9MipMap_GetLevelDesc,
    D3D9MipMap_GetSurfaceLevel,
    D3D9MipMap_LockRect,
    D3D9MipMap_UnlockRect,
    D3D9MipMap_AddDirtyRect,
};
#endif // !defined(__cplusplus) || defined(CINTERFACE)

HRESULT CreateD3D9MipMap(DIRECT3DDEVICE9_INT* pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture)
{
    LPD3D9MIPMAP pThisTexture;
    if (FAILED(AlignedAlloc((LPVOID*)&pThisTexture, sizeof(D3D9MipMap))))
    {
        DPRINT1("Could not create D3D9MipMap");
        return E_OUTOFMEMORY;
    }

    InitDirect3DBaseTexture9(&pThisTexture->BaseTexture, (IDirect3DBaseTexture9Vtbl*)&D3D9MipMap_Vtbl, Usage, Levels, Format, Pool, pDevice, RT_EXTERNAL);

    pThisTexture->lpVtbl = &D3D9MipMap_Vtbl;

    pThisTexture->Usage = Usage;
    pThisTexture->dwWidth = Width;
    pThisTexture->dwHeight = Height;
    pThisTexture->Format = Format;

    *ppTexture = (IDirect3DTexture9*)&pThisTexture->lpVtbl;

    UNIMPLEMENTED;
    return D3D_OK;
}
