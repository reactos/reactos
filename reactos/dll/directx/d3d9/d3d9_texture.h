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

typedef struct _Direct3DBaseTexture9_INT
{
/* 0x0000 */    struct IDirect3DBaseTexture9Vtbl* lpVtbl;
/* 0x0004 */    DWORD dwUnknown04;
/* 0x0008 */    Direct3DResource9_INT BaseResource;
/* 0x004c */    DWORD dwUnknown4c;
/* 0x0050 */    DWORD dwUnknown50;
/* 0x0054 */    DWORD Usage;
/* 0x0058 */    WORD MipMapLevels;
/* 0x005a */    WORD dUnknown5a;
/* 0x005c */    WORD MipMapLevels2;
/* 0x005e */    WORD dUnknown5e;
} Direct3DBaseTexture9_INT;

#endif // _D3D9_TEXTURE_H_
