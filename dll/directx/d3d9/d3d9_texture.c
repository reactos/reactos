/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_texture.c
 * PURPOSE:         d3d9.dll internal texture surface functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_texture.h"

#define LOCK_D3DDEVICE9()   D3D9BaseObject_LockDevice(&This->BaseResource.BaseObject)
#define UNLOCK_D3DDEVICE9() D3D9BaseObject_UnlockDevice(&This->BaseResource.BaseObject)

/* Convert a IDirect3DBaseTexture9 pointer safely to the internal implementation struct */
Direct3DBaseTexture9_INT* IDirect3DBaseTexture9ToImpl(LPDIRECT3DBASETEXTURE9 iface)
{
    if (NULL == iface)
        return NULL;

    return (Direct3DBaseTexture9_INT*)((ULONG_PTR)iface - FIELD_OFFSET(Direct3DBaseTexture9_INT, lpVtbl));
}

void InitDirect3DBaseTexture9(Direct3DBaseTexture9_INT* pBaseTexture,
                              IDirect3DBaseTexture9Vtbl* pVtbl,
                              DWORD Usage,
                              UINT Levels,
                              D3DFORMAT Format,
                              D3DPOOL Pool,
                              struct _Direct3DDevice9_INT* pDevice,
                              enum REF_TYPE RefType)
{
    InitDirect3DResource9(&pBaseTexture->BaseResource, Pool, pDevice, RefType);

    pBaseTexture->lpVtbl = pVtbl;
    pBaseTexture->Format = Format;
    pBaseTexture->wPaletteIndex = 0xFFFF;
    pBaseTexture->Usage = Usage;
    pBaseTexture->MipMapLevels = Levels;
    pBaseTexture->MipMapLevels2 = Levels;

    pBaseTexture->FilterType = D3DTEXF_LINEAR;
    pBaseTexture->bIsAutoGenMipMap = (Usage & D3DUSAGE_AUTOGENMIPMAP) != 0;
}

/*++
* @name IDirect3DBaseTexture9::GetAutoGenFilterType
* @implemented
*
* The function D3D9Texture_GetAutoGenFilterType returns filter type
* that is used when automated mipmaping is set.
*
* @param LPDIRECT3DBASETEXTURE9 iface
* Pointer to a IDirect3DBaseTexture9 interface
*
* @return D3DTEXTUREFILTERTYPE
* Filter type used when automated mipmaping is set for the specified texture.
*
*/
D3DTEXTUREFILTERTYPE WINAPI D3D9Texture_GetAutoGenFilterType(LPDIRECT3DBASETEXTURE9 iface)
{
    D3DTEXTUREFILTERTYPE FilterType;
    Direct3DBaseTexture9_INT* This = IDirect3DBaseTexture9ToImpl(iface);
    LOCK_D3DDEVICE9();

    FilterType = This->dwFilterType;

    UNLOCK_D3DDEVICE9();
    return FilterType;
}

/*++
* @name IDirect3DBaseTexture9::GetLOD
* @implemented
*
* The function D3D9Texture_GetLOD returns the number
* max LODs for the specified texture, if it's managed.
*
* @param LPDIRECT3DBASETEXTURE9 iface
* Pointer to a IDirect3DBaseTexture9 interface
*
* @return DWORD
* Returns the number of LODs in the specified texture if it's managed.
*
*/
DWORD WINAPI D3D9Texture_GetLOD(LPDIRECT3DBASETEXTURE9 iface)
{
    DWORD MaxLOD = 0;
    Direct3DBaseTexture9_INT* This = IDirect3DBaseTexture9ToImpl(iface);
    LOCK_D3DDEVICE9();

    if (This->BaseResource.bIsManaged)
        MaxLOD = This->MaxLOD;

    UNLOCK_D3DDEVICE9();
    return MaxLOD;
}

/*++
* @name IDirect3DBaseTexture9::GetLevelCount
* @implemented
*
* The function D3D9Texture_GetLevelCount returns the number of mip map levels
* in the specified texture.
*
* @param LPDIRECT3DBASETEXTURE9 iface
* Pointer to a IDirect3DBaseTexture9 interface
*
* @return DWORD
* Returns the number of levels in the specified texture.
*
*/
DWORD WINAPI D3D9Texture_GetLevelCount(LPDIRECT3DBASETEXTURE9 iface)
{
    DWORD MipMapLevels;
    Direct3DBaseTexture9_INT* This = IDirect3DBaseTexture9ToImpl(iface);
    LOCK_D3DDEVICE9();

    MipMapLevels = This->MipMapLevels;

    UNLOCK_D3DDEVICE9();
    return MipMapLevels;
}
