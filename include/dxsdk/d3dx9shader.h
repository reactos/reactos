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

typedef enum _D3DXREGISTER_SET
{
    D3DXRS_BOOL,
    D3DXRS_INT4,
    D3DXRS_FLOAT4,
    D3DXRS_SAMPLER,
    D3DXRS_FORCE_DWORD = 0x7fffffff
} D3DXREGISTER_SET, *LPD3DXREGISTER_SET;

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

typedef struct _D3DXCONSTANTTABLE_DESC
{
    LPCSTR Creator;
    DWORD Version;
    UINT Constants;
} D3DXCONSTANTTABLE_DESC, *LPD3DXCONSTANTTABLE_DESC;

typedef struct _D3DXCONSTANT_DESC
{
    LPCSTR Name;
    D3DXREGISTER_SET RegisterSet;
    UINT RegisterIndex;
    UINT RegisterCount;
    D3DXPARAMETER_CLASS Class;
    D3DXPARAMETER_TYPE Type;
    UINT Rows;
    UINT Columns;
    UINT Elements;
    UINT StructMembers;
    UINT Bytes;
    LPCVOID DefaultValue;
} D3DXCONSTANT_DESC, *LPD3DXCONSTANT_DESC;

DEFINE_GUID(IID_ID3DXConstantTable, 0x9dca3190, 0x38b9, 0x4fc3, 0x92, 0xe3, 0x39, 0xc6, 0xdd, 0xfb, 0x35, 0x8b);

#undef INTERFACE
#define INTERFACE ID3DXConstantTable

DECLARE_INTERFACE_(ID3DXConstantTable, ID3DXBuffer)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBuffer methods ***/
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
    /*** ID3DXConstantTable methods ***/
    STDMETHOD(GetDesc)(THIS_ D3DXCONSTANTTABLE_DESC *pDesc) PURE;
    STDMETHOD(GetConstantDesc)(THIS_ D3DXHANDLE hConstant, D3DXCONSTANT_DESC *pConstantDesc, UINT *pCount) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstant)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantByName)(THIS_ D3DXHANDLE hConstant, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantElement)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
    STDMETHOD(SetDefaults)(THIS_ LPDIRECT3DDEVICE9 pDevice) PURE;
    STDMETHOD(SetValue)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, BOOL b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, INT n) PURE;
    STDMETHOD(SetIntArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, FLOAT f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define ID3DXConstantTable_QueryInterface(p,a,b)                      (p)->lpVtbl->QueryInterface(p,a,b)
#define ID3DXConstantTable_AddRef(p)                                  (p)->lpVtbl->AddRef(p)
#define ID3DXConstantTable_Release(p)                                 (p)->lpVtbl->Release(p)
/*** ID3DXBuffer methods ***/
#define ID3DXConstantTable_GetBufferPointer(p)                        (p)->lpVtbl->GetBufferPointer(p)
#define ID3DXConstantTable_GetBufferSize(p)                           (p)->lpVtbl->GetBufferSize(p)
/*** ID3DXConstantTable methods ***/
#define ID3DXConstantTable_GetDesc(p,a)                               (p)->lpVtbl->GetDesc(p,a)
#define ID3DXConstantTable_GetConstantDesc(p,a,b,c)                   (p)->lpVtbl->GetConstantDesc(p,a,b,c)
#define ID3DXConstantTable_GetConstant(p,a,b)                         (p)->lpVtbl->GetConstant(p,a,b)
#define ID3DXConstantTable_GetConstantByName(p,a,b)                   (p)->lpVtbl->GetConstantByName(p,a,b)
#define ID3DXConstantTable_GetConstantElement(p,a,b)                  (p)->lpVtbl->GetConstantElement(p,a,b)
#define ID3DXConstantTable_SetDefaults(p,a)                           (p)->lpVtbl->SetDefaults(p,a)
#define ID3DXConstantTable_SetValue(p,a,b,c,d)                        (p)->lpVtbl->SetValue(p,a,b,c,d)
#define ID3DXConstantTable_SetBool(p,a,b,c)                           (p)->lpVtbl->SetBool(p,a,b,c)
#define ID3DXConstantTable_SetBoolArray(p,a,b,c,d)                    (p)->lpVtbl->SetBoolArray(p,a,b,c,d)
#define ID3DXConstantTable_SetInt(p,a,b,c)                            (p)->lpVtbl->SetInt(p,a,b,c)
#define ID3DXConstantTable_SetIntArray(p,a,b,c,d)                     (p)->lpVtbl->SetIntArray(p,a,b,c,d)
#define ID3DXConstantTable_SetFloat(p,a,b,c)                          (p)->lpVtbl->SetFloat(p,a,b,c)
#define ID3DXConstantTable_SetFloatArray(p,a,b,c,d)                   (p)->lpVtbl->SetFloatArray(p,a,b,c,d)
#define ID3DXConstantTable_SetVector(p,a,b,c)                         (p)->lpVtbl->SetVector(p,a,b,c)
#define ID3DXConstantTable_SetVectorArray(p,a,b,c,d)                  (p)->lpVtbl->SetVectorArray(p,a,b,c,d)
#define ID3DXConstantTable_SetMatrix(p,a,b,c)                         (p)->lpVtbl->SetMatrix(p,a,b,c)
#define ID3DXConstantTable_SetMatrixArray(p,a,b,c,d)                  (p)->lpVtbl->SetMatrixArray(p,a,b,c,d)
#define ID3DXConstantTable_SetMatrixPointerArray(p,a,b,c,d)           (p)->lpVtbl->SetMatrixPointerArray(p,a,b,c,d)
#define ID3DXConstantTable_SetMatrixTranspose(p,a,b,c)                (p)->lpVtbl->SetMatrixTranspose(p,a,b,c)
#define ID3DXConstantTable_SetMatrixTransposeArray(p,a,b,c,d)         (p)->lpVtbl->SetMatrixTransposeArray(p,a,b,c,d)
#define ID3DXConstantTable_SetMatrixTransposePointerArray(p,a,b,c,d)  (p)->lpVtbl->SetMatrixTransposePointerArray(p,a,b,c,d)
#else
/*** IUnknown methods ***/
#define ID3DXConstantTable_QueryInterface(p,a,b)                      (p)->QueryInterface(a,b)
#define ID3DXConstantTable_AddRef(p)                                  (p)->AddRef()
#define ID3DXConstantTable_Release(p)                                 (p)->Release()
/*** ID3DXBuffer methods ***/
#define ID3DXConstantTable_GetBufferPointer(p)                        (p)->GetBufferPointer()
#define ID3DXConstantTable_GetBufferSize(p)                           (p)->GetBufferSize()
/*** ID3DXConstantTable methods ***/
#define ID3DXConstantTable_GetDesc(p,a)                               (p)->GetDesc(a)
#define ID3DXConstantTable_GetConstantDesc(p,a,b,c)                   (p)->GetConstantDesc(a,b,c)
#define ID3DXConstantTable_GetConstant(p,a,b)                         (p)->GetConstant(a,b)
#define ID3DXConstantTable_GetConstantByName(p,a,b)                   (p)->GetConstantByName(a,b)
#define ID3DXConstantTable_GetConstantElement(p,a,b)                  (p)->GetConstantElement(a,b)
#define ID3DXConstantTable_SetDefaults(p,a)                           (p)->SetDefaults(a)
#define ID3DXConstantTable_SetValue(p,a,b,c,d)                        (p)->SetValue(a,b,c,d)
#define ID3DXConstantTable_SetBool(p,a,b,c)                           (p)->SetBool(a,b,c)
#define ID3DXConstantTable_SetBoolArray(p,a,b,c,d)                    (p)->SetBoolArray(a,b,c,d)
#define ID3DXConstantTable_SetInt(p,a,b,c)                            (p)->SetInt(a,b,c)
#define ID3DXConstantTable_SetIntArray(p,a,b,c,d)                     (p)->SetIntArray(a,b,c,d)
#define ID3DXConstantTable_SetFloat(p,a,b,c)                          (p)->SetFloat(a,b,c)
#define ID3DXConstantTable_SetFloatArray(p,a,b,c,d)                   (p)->SetFloatArray(a,b,c,d)
#define ID3DXConstantTable_SetVector(p,a,b,c)                         (p)->SetVector(a,b,c)
#define ID3DXConstantTable_SetVectorArray(p,a,b,c,d)                  (p)->SetVectorArray(a,b,c,d)
#define ID3DXConstantTable_SetMatrix(p,a,b,c)                         (p)->SetMatrix(a,b,c)
#define ID3DXConstantTable_SetMatrixArray(p,a,b,c,d)                  (p)->SetMatrixArray(a,b,c,d)
#define ID3DXConstantTable_SetMatrixPointerArray(p,a,b,c,d)           (p)->SetMatrixPointerArray(a,b,c,d)
#define ID3DXConstantTable_SetMatrixTranspose(p,a,b,c)                (p)->>SetMatrixTranspose(a,b,c)
#define ID3DXConstantTable_SetMatrixTransposeArray(p,a,b,c,d)         (p)->SetMatrixTransposeArray(a,b,c,d)
#define ID3DXConstantTable_SetMatrixTransposePointerArray(p,a,b,c,d)  (p)->SetMatrixTransposePointerArray(a,b,c,d)
#endif

typedef struct ID3DXConstantTable *LPD3DXCONSTANTTABLE;

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
HRESULT WINAPI D3DXFindShaderComment(CONST DWORD* byte_code, DWORD fourcc, LPCVOID* data, UINT* size);

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

HRESULT WINAPI D3DXGetShaderConstantTableEx(CONST DWORD* byte_code,
                                            DWORD flags,
                                            LPD3DXCONSTANTTABLE* constant_table);

HRESULT WINAPI D3DXGetShaderConstantTable(CONST DWORD* byte_code,
                                          LPD3DXCONSTANTTABLE* constant_table);

#ifdef __cplusplus
}
#endif

#endif /* __D3DX9SHADER_H__ */
