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

typedef struct _D3D9Texture
{
/* 0x0000 */    D3D9Resource BaseResource;
/* 0x0044 */    DWORD dwUnknown44;
/* 0x0048 */    DWORD dwUnknown48;
/* 0x004c */    DWORD dwUnknown4c;
/* 0x0050 */    DWORD dwUnknown50;
/* 0x0054 */    DWORD dwUnknown54;
/* 0x0058 */    DWORD dwUnknown58;
/* 0x005c */    DWORD dwUnknown5c;
/* 0x0060 */    DWORD dwUnknown60;
/* 0x0064 */    DWORD dwUnknown64;
} D3D9Texture;

#endif // _D3D9_TEXTURE_H_
