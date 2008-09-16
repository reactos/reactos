/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_mipmap.h
 * PURPOSE:         d3d9.dll internal mip map surface structures
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_MIPMAP_H_
#define _D3D9_MIPMAP_H_

#include "d3d9_texture.h"

struct _Direct3DDevice9_INT;

typedef struct _D3D9MipMap
{
/* 0x0000 */    D3D9Texture BaseTexture;
/* 0x0068 */    DWORD dwUnknown68;
/* 0x006c */    DWORD dwUnknown6c;
/* 0x0070 */    DWORD dwUnknown70;
/* 0x0074 */    DWORD dwUnknown74;
} D3D9MipMap;

HRESULT CreateD3D9MipMap(struct _Direct3DDevice9_INT* pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture);

#endif // _D3D9_MIPMAP_H_
