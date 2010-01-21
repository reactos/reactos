/*
 * Copyright 2008 Luis Busquets
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

#include "d3dx9.h"

#ifndef __D3DX9SHADER_H__
#define __D3DX9SHADER_H__

#define D3DXSHADER_DEBUG                          0x1
#define D3DXSHADER_SKIPVALIDATION                 0x2
#define D3DXSHADER_SKIPOPTIMIZATION               0x4
#define D3DXSHADER_PACKMATRIX_ROWMAJOR            0x8
#define D3DXSHADER_PACKMATRIX_COLUMNMAJOR         0x10
#define D3DXSHADER_PARTIALPRECISION               0x20
#define D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT        0x40
#define D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT        0x80
#define D3DXSHADER_NO_PRESHADER                   0x100
#define D3DXSHADER_AVOID_FLOW_CONTROL             0x200
#define D3DXSHADER_PREFER_FLOW_CONTROL            0x400
#define D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY 0x1000
#define D3DXSHADER_IEEE_STRICTNESS                0x2000

#define D3DXSHADER_OPTIMIZATION_LEVEL0            0x4000
#define D3DXSHADER_OPTIMIZATION_LEVEL1            0x0
#define D3DXSHADER_OPTIMIZATION_LEVEL2            0xC000
#define D3DXSHADER_OPTIMIZATION_LEVEL3            0x8000

#define D3DXSHADER_USE_LEGACY_D3DX9_31_DLL        0x10000

typedef LPCSTR D3DXHANDLE;

typedef enum D3DXPARAMETER_CLASS
{
    D3DXPC_SCALAR,
    D3DXPC_VECTOR,
    D3DXPC_MATRIX_ROWS,
    D3DXPC_MATRIX_COLUMNS,
    D3DXPC_OBJECT,
    D3DXPC_STRUCT,
    D3DXPC_FORCE_DWORD = 0x7fffffff,
} D3DXPARAMETER_CLASS, *LPD3DXPARAMETER_CLASS;

typedef enum D3DXPARAMETER_TYPE
{
    D3DXPT_VOID,
    D3DXPT_BOOL,
    D3DXPT_INT,
    D3DXPT_FLOAT,
    D3DXPT_STRING,
    D3DXPT_TEXTURE,
    D3DXPT_TEXTURE1D,
    D3DXPT_TEXTURE2D,
    D3DXPT_TEXTURE3D,
    D3DXPT_TEXTURECUBE,
    D3DXPT_SAMPLER,
    D3DXPT_SAMPLER1D,
    D3DXPT_SAMPLER2D,
    D3DXPT_SAMPLER3D,
    D3DXPT_SAMPLERCUBE,
    D3DXPT_PIXELSHADER,
    D3DXPT_VERTEXSHADER,
    D3DXPT_PIXELFRAGMENT,
    D3DXPT_VERTEXFRAGMENT,
    D3DXPT_UNSUPPORTED,
    D3DXPT_FORCE_DWORD = 0x7fffffff,
} D3DXPARAMETER_TYPE, *LPD3DXPARAMETER_TYPE;

typedef struct _D3DXMACRO {
    LPCSTR Name;
    LPCSTR Definition;
} D3DXMACRO, *LPD3DXMACRO;

typedef enum _D3DXINCLUDE_TYPE
{
    D3DXINC_LOCAL,
    D3DXINC_SYSTEM,
    D3DXINC_FORCE_DWORD = 0x7fffffff,
} D3DXINCLUDE_TYPE, *LPD3DXINCLUDE_TYPE;

#undef INTERFACE
#define INTERFACE ID3DXInclude

DECLARE_INTERFACE(ID3DXInclude)
{
    STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE include_type, LPCSTR filename, LPCVOID parent_data, LPCVOID *data, UINT *bytes) PURE;
    STDMETHOD(Close)(THIS_ LPCVOID data) PURE;
};

#define ID3DXInclude_Open(p,a,b,c,d,e)  (p)->lpVtbl->Open(p,a,b,c,d,e)
#define ID3DXInclude_Close(p,a)         (p)->lpVtbl->Close(p,a)

typedef struct ID3DXInclude *LPD3DXINCLUDE;

#ifdef __cplusplus
extern "C" {
#endif

LPCSTR WINAPI D3DXGetPixelShaderProfile(LPDIRECT3DDEVICE9 device);
UINT WINAPI D3DXGetShaderSize(const DWORD *byte_code);
DWORD WINAPI D3DXGetShaderVersion(const DWORD *byte_code);
LPCSTR WINAPI D3DXGetVertexShaderProfile(LPDIRECT3DDEVICE9 device);

HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages);

HRESULT WINAPI D3DXAssembleShaderFromFileW(LPCWSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages);

HRESULT WINAPI D3DXAssembleShaderFromResourceA(HMODULE module,
                                               LPCSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages);

HRESULT WINAPI D3DXAssembleShaderFromResourceW(HMODULE module,
                                               LPCWSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages);

HRESULT WINAPI D3DXAssembleShader(LPCSTR data,
                                  UINT data_len,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  DWORD flags,
                                  LPD3DXBUFFER* shader,
                                  LPD3DXBUFFER* error_messages);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9SHADER_H__ */
