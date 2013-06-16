/*
 * Copyright (C) 2009 David Adam
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
 */

#include <d3dx9.h>

#ifndef __WINE_D3DX9MESH_H
#define __WINE_D3DX9MESH_H

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateBuffer(DWORD, LPD3DXBUFFER*);
UINT    WINAPI D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, DWORD stream_idx);
UINT    WINAPI D3DXGetFVFVertexSize(DWORD);
BOOL    WINAPI D3DXBoxBoundProbe(CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *);
BOOL    WINAPI D3DXSphereBoundProbe(CONST D3DXVECTOR3 *,FLOAT,CONST D3DXVECTOR3 *,CONST D3DXVECTOR3 *);
HRESULT WINAPI D3DXComputeBoundingBox(CONST D3DXVECTOR3 *, DWORD, DWORD, D3DXVECTOR3 *, D3DXVECTOR3 *);
HRESULT WINAPI D3DXComputeBoundingSphere(CONST D3DXVECTOR3 *, DWORD, DWORD, D3DXVECTOR3 *, FLOAT *);
BOOL    WINAPI D3DXIntersectTri(CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3*, FLOAT *, FLOAT *, FLOAT *);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_D3DX9MESH_H */
