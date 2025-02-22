/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_basetexture.h
 * PURPOSE:         Work-around for gcc warning, DO NOT USE FOR ANYTHING ELSE!!!
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#ifndef _D3D9_BASETEXTURE_H_
#define _D3D9_BASETEXTURE_H_

#include "d3d9_resource.h"

// Work-around for:
// "warning: 'FilterType' is narrower than values of its type"
#if __GNUC__ >=3
    #pragma GCC system_header
#endif

struct IDirect3DBaseTexture9Vtbl;

#pragma pack(push)
#pragma pack(1)
typedef struct _Direct3DBaseTexture9_INT
{
/* 0x0000 */    IDirect3DBaseTexture9Vtbl* lpVtbl;
/* 0x0004 */    DWORD dwUnknown04;
/* 0x0008 */    Direct3DResource9_INT BaseResource;
/* 0x004c */    DWORD dwUnknown4c;
/* 0x0050 */    D3DFORMAT Format;
/* 0x0054 */    DWORD Usage;
/* 0x0058 */    WORD MipMapLevels;
                union {
/* 0x005a */        D3DTEXTUREFILTERTYPE FilterType : 8;
                    struct
                    {
/* 0x005a */            DWORD dwFilterType      : 8;
/* 0x005b */            BOOL  bIsAutoGenMipMap  : 8;
/* 0x005c */            DWORD MipMapLevels2     : 8;
/* 0x005d */            DWORD MaxLOD            : 8;
                    };
                };
/* 0x005e */    WORD wPaletteIndex;
} Direct3DBaseTexture9_INT;
#pragma pack(pop)

#endif
