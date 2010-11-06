/*
 * Copyright (C) 2002 Raphael Junqueira
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Tony Wasserka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#ifndef __WINE_D3DX9_36_PRIVATE_H
#define __WINE_D3DX9_36_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "d3dx9.h"

/* for internal use */
typedef enum _FormatType {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_UNKNOWN
} FormatType;

typedef struct _PixelFormatDesc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    FormatType type;
} PixelFormatDesc;

HRESULT map_view_of_file(LPCWSTR filename, LPVOID *buffer, DWORD *length);
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, LPVOID *buffer, DWORD *length);

const PixelFormatDesc *get_format_info(D3DFORMAT format);


extern const ID3DXBufferVtbl D3DXBuffer_Vtbl;

/* ID3DXBUFFER */
typedef struct ID3DXBufferImpl
{
    /* IUnknown fields */
    const ID3DXBufferVtbl *lpVtbl;
    LONG           ref;

    /* ID3DXBuffer fields */
    DWORD         *buffer;
    DWORD          bufferSize;
} ID3DXBufferImpl;


/* ID3DXFont */
typedef struct ID3DXFontImpl
{
    /* IUnknown fields */
    const ID3DXFontVtbl *lpVtbl;
    LONG ref;

    /* ID3DXFont fields */
    IDirect3DDevice9 *device;
    D3DXFONT_DESCW desc;

    HDC hdc;
    HFONT hfont;
} ID3DXFontImpl;

/* ID3DXMatrixStack */
typedef struct ID3DXMatrixStackImpl
{
  /* IUnknown fields */
  const ID3DXMatrixStackVtbl *lpVtbl;
  LONG                   ref;

  /* ID3DXMatrixStack fields */
  unsigned int current;
  unsigned int stack_size;
  D3DXMATRIX *stack;
} ID3DXMatrixStackImpl;

/*ID3DXSprite */
typedef struct _SPRITE {
    LPDIRECT3DTEXTURE9 texture;
    UINT texw, texh;
    RECT rect;
    D3DXVECTOR3 center;
    D3DXVECTOR3 pos;
    D3DCOLOR color;
} SPRITE;

typedef struct ID3DXSpriteImpl
{
    /* IUnknown fields */
    const ID3DXSpriteVtbl *lpVtbl;
    LONG ref;

    /* ID3DXSprite fields */
    IDirect3DDevice9 *device;
    IDirect3DVertexDeclaration9 *vdecl;
    IDirect3DStateBlock9 *stateblock;
    D3DXMATRIX transform;
    D3DXMATRIX view;
    DWORD flags;
    BOOL ready;

    /* Store the relevant caps to prevent multiple GetDeviceCaps calls */
    DWORD texfilter_caps;
    DWORD maxanisotropy;
    DWORD alphacmp_caps;

    SPRITE *sprites;
    int sprite_count;      /* number of sprites to be drawn */
    int allocated_sprites; /* number of (pre-)allocated sprites */
} ID3DXSpriteImpl;


#endif /* __WINE_D3DX9_36_PRIVATE_H */
