/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_texture.c
 * PURPOSE:         d3d9.dll internal texture surface functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_texture.h"

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
    pBaseTexture->MipMapLevels =
        pBaseTexture->MipMapLevels2 = (WORD)Levels;

    pBaseTexture->FilterType = D3DTEXF_LINEAR;
    pBaseTexture->bIsAutoGenMipMap = (Usage & D3DUSAGE_AUTOGENMIPMAP) != 0;
}
