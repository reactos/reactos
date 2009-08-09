/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_texture.h
 * PURPOSE:         d3d9.dll internal texture surface structures
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_TEXTURE_H_
#define _D3D9_TEXTURE_H_

#include "d3d9_resource.h"
#include "d3d9_basetexture.h"

struct IDirect3DBaseTexture9Vtbl;

void InitDirect3DBaseTexture9(Direct3DBaseTexture9_INT* pBaseTexture,
                              IDirect3DBaseTexture9Vtbl* pVtbl,
                              DWORD Usage,
                              UINT Levels,
                              D3DFORMAT Format,
                              D3DPOOL Pool,
                              struct _Direct3DDevice9_INT* pDevice,
                              enum REF_TYPE RefType);


D3DTEXTUREFILTERTYPE WINAPI D3D9Texture_GetAutoGenFilterType(struct IDirect3DBaseTexture9* iface);
DWORD WINAPI D3D9Texture_GetLOD(struct IDirect3DBaseTexture9* iface);
DWORD WINAPI D3D9Texture_GetLevelCount(struct IDirect3DBaseTexture9* iface);

#endif // _D3D9_TEXTURE_H_
