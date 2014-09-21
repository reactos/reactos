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
/* 0x0000 */    Direct3DBaseTexture9_INT BaseTexture;
/* 0x0060 */    struct IDirect3DTexture9Vtbl* lpVtbl;
/* 0x0064 */    LPDWORD dwUnknown64;
/* 0x0068 */    D3DFORMAT Format;
/* 0x006c */    DWORD dwUnknown6c;
/* 0x0070 */    DWORD Usage;
/* 0x0074 */    DWORD dwUnknown74;
/* 0x0078 */    DWORD dwUnknown78;
/* 0x007c */    DWORD dwUnknown7c;
/* 0x0080 */    DWORD dwWidth;
/* 0x0084 */    DWORD dwHeight;
/* 0x0088 */    DWORD dwUnknown88;
/* 0x008c */    DWORD dwUnknown8c;
/* 0x0090 */    DWORD dwUnknown90;
/* 0x0094 */    DWORD dwUnknown94;
/* 0x0098 */    DWORD dwUnknown98;
/* 0x009c */    DWORD dwUnknown9c;
/* 0x00a0 */    DWORD dwUnknowna0;
/* 0x00a4 */    DWORD dwUnknowna4;
/* 0x00a8 */    DWORD dwUnknowna8;
/* 0x00ac */    DWORD dwUnknownac;
/* 0x00b0 */    DWORD dwUnknownb0;
/* 0x00b4 */    DWORD dwUnknownb4;
/* 0x00b8 */    DWORD dwUnknownb8;
/* 0x00bc */    DWORD dwUnknownbc;
/* 0x00c0 */    DWORD dwUnknownc0;
/* 0x00c4 */    DWORD dwUnknownc4;
/* 0x00c8 */    DWORD dwUnknownc8;
/* 0x00cc */    DWORD dwUnknowncc;
/* 0x00d0 */    DWORD dwUnknownd0;
/* 0x00d4 */    DWORD dwUnknownd4;
/* 0x00d8 */    DWORD dwUnknownd8;
/* 0x00dc */    DWORD dwUnknowndc;
/* 0x00e0 */    DWORD dwUnknowne0;
/* 0x00e4 */    DWORD dwUnknowne4;
/* 0x00e8 */    DWORD dwUnknowne8;
/* 0x00ec */    DWORD dwUnknownec;
/* 0x00f0 */    DWORD dwUnknownf0;
/* 0x00f4 */    DWORD dwUnknownf4;
} D3D9MipMap, FAR* LPD3D9MIPMAP;

HRESULT CreateD3D9MipMap(struct _Direct3DDevice9_INT* pDevice, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture);

#endif // _D3D9_MIPMAP_H_
