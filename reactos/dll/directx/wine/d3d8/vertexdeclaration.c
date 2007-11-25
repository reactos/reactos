/*
 * IDirect3DVertexDeclaration8 implementation
 *
 * Copyright 2007 Henri Verbeet
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

/* IDirect3DVertexDeclaration8 is internal to our implementation.
 * It's not visible in the API. */

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IUnknown */
static HRESULT WINAPI IDirect3DVertexDeclaration8Impl_QueryInterface(IDirect3DVertexDeclaration8 *iface, REFIID riid, void **obj_ptr)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj_ptr);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration8))
    {
        IUnknown_AddRef(iface);
        *obj_ptr = iface;
        return S_OK;
    }

    *obj_ptr = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_AddRef(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedIncrement(&This->ref_count);
    TRACE("(%p) : AddRef increasing to %d\n", This, ref_count);

    return ref_count;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_Release(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedDecrement(&This->ref_count);
    TRACE("(%p) : Releasing to %d\n", This, ref_count);

    if (!ref_count) {
        IWineD3DVertexDeclaration_Release(This->wined3d_vertex_declaration);
        HeapFree(GetProcessHeap(), 0, This->elements);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref_count;
}

static const char *debug_d3dvsdt_type(D3DVSDT_TYPE d3dvsdt_type)
{
    switch (d3dvsdt_type)
    {
#define D3DVSDT_TYPE_TO_STR(u) case u: return #u
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT1);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT2);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT3);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT4);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_D3DCOLOR);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_UBYTE4);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_SHORT2);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_SHORT4);
#undef D3DVSDT_TYPE_TO_STR
        default:
            FIXME("Unrecognized D3DVSDT_TYPE %#x\n", d3dvsdt_type);
            return "unrecognized";
    }
}

static const char *debug_d3dvsde_register(D3DVSDE_REGISTER d3dvsde_register)
{
    switch (d3dvsde_register)
    {
#define D3DVSDE_REGISTER_TO_STR(u) case u: return #u
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_POSITION);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_BLENDWEIGHT);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_BLENDINDICES);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_NORMAL);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_PSIZE);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_DIFFUSE);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_SPECULAR);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD0);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD1);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD2);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD3);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD4);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD5);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD6);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD7);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_POSITION2);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_NORMAL2);
#undef D3DVSDE_REGISTER_TO_STR
        default:
            FIXME("Unrecognized D3DVSDE_REGISTER %#x\n", d3dvsde_register);
            return "unrecognized";
    }
}

static size_t parse_token(const DWORD* pToken)
{
    const DWORD token = *pToken;
    size_t tokenlen = 1;

    switch ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT) { /* maybe a macro to inverse ... */
        case D3DVSD_TOKEN_NOP:
            TRACE(" 0x%08x NOP()\n", token);
            break;

        case D3DVSD_TOKEN_STREAM:
            if (token & D3DVSD_STREAMTESSMASK)
            {
                TRACE(" 0x%08x STREAM_TESS()\n", token);
            } else {
                TRACE(" 0x%08x STREAM(%u)\n", token, ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT));
            }
            break;

        case D3DVSD_TOKEN_STREAMDATA:
            if (token & 0x10000000)
            {
                TRACE(" 0x%08x SKIP(%u)\n", token, ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT));
            } else {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
                DWORD reg = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
                TRACE(" 0x%08x REG(%s, %s)\n", token, debug_d3dvsde_register(reg), debug_d3dvsdt_type(type));
            }
            break;

        case D3DVSD_TOKEN_TESSELLATOR:
            if (token & 0x10000000)
            {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
                DWORD reg = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
                TRACE(" 0x%08x TESSUV(%s) as %s\n", token, debug_d3dvsde_register(reg), debug_d3dvsdt_type(type));
            } else {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)    >> D3DVSD_DATATYPESHIFT);
                DWORD regout = ((token & D3DVSD_VERTEXREGMASK)   >> D3DVSD_VERTEXREGSHIFT);
                DWORD regin = ((token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT);
                TRACE(" 0x%08x TESSNORMAL(%s, %s) as %s\n", token, debug_d3dvsde_register(regin),
                        debug_d3dvsde_register(regout), debug_d3dvsdt_type(type));
            }
            break;

        case D3DVSD_TOKEN_CONSTMEM:
            {
                DWORD count = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
                tokenlen = (4 * count) + 1;
            }
            break;

        case D3DVSD_TOKEN_EXT:
            {
                DWORD count = ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT);
                DWORD extinfo = ((token & D3DVSD_EXTINFOMASK)    >> D3DVSD_EXTINFOSHIFT);
                TRACE(" 0x%08x EXT(%u, %u)\n", token, count, extinfo);
                /* todo ... print extension */
                tokenlen = count + 1;
            }
            break;

        case D3DVSD_TOKEN_END:
            TRACE(" 0x%08x END()\n", token);
            break;

        default:
            TRACE(" 0x%08x UNKNOWN\n", token);
            /* argg error */
    }

    return tokenlen;
}

void load_local_constants(const DWORD *d3d8_elements, IWineD3DVertexShader *wined3d_vertex_shader)
{
    const DWORD *token = d3d8_elements;

    while (*token != D3DVSD_END())
    {
        if (((*token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT) == D3DVSD_TOKEN_CONSTMEM)
        {
            DWORD count = ((*token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT);
            DWORD constant_idx = ((*token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT);
            HRESULT hr;

            if (TRACE_ON(d3d8))
            {
                DWORD i;
                for (i = 0; i < count; ++i)
                {
                    TRACE("c[%u] = (%8f, %8f, %8f, %8f)\n",
                            constant_idx,
                            *(const float *)(token + i * 4 + 1),
                            *(const float *)(token + i * 4 + 2),
                            *(const float *)(token + i * 4 + 3),
                            *(const float *)(token + i * 4 + 4));
                }
            }
            hr = IWineD3DVertexShader_SetLocalConstantsF(wined3d_vertex_shader, constant_idx, (const float *)token+1, count);
            if (FAILED(hr)) ERR("Failed setting shader constants\n");
        }

        token += parse_token(token);
    }
}

/* NOTE: Make sure these are in the correct numerical order. (see /include/wined3d_types.h) */
static const size_t wined3d_type_sizes[WINED3DDECLTYPE_UNUSED] = {
    /*WINED3DDECLTYPE_FLOAT1*/    1 * sizeof(float),
    /*WINED3DDECLTYPE_FLOAT2*/    2 * sizeof(float),
    /*WINED3DDECLTYPE_FLOAT3*/    3 * sizeof(float),
    /*WINED3DDECLTYPE_FLOAT4*/    4 * sizeof(float),
    /*WINED3DDECLTYPE_D3DCOLOR*/  4 * sizeof(BYTE),
    /*WINED3DDECLTYPE_UBYTE4*/    4 * sizeof(BYTE),
    /*WINED3DDECLTYPE_SHORT2*/    2 * sizeof(short int),
    /*WINED3DDECLTYPE_SHORT4*/    4 * sizeof(short int),
    /*WINED3DDECLTYPE_UBYTE4N*/   4 * sizeof(BYTE),
    /*WINED3DDECLTYPE_SHORT2N*/   2 * sizeof(short int),
    /*WINED3DDECLTYPE_SHORT4N*/   4 * sizeof(short int),
    /*WINED3DDECLTYPE_USHORT2N*/  2 * sizeof(short int),
    /*WINED3DDECLTYPE_USHORT4N*/  4 * sizeof(short int),
    /*WINED3DDECLTYPE_UDEC3*/     3 * sizeof(short int),
    /*WINED3DDECLTYPE_DEC3N*/     3 * sizeof(short int),
    /*WINED3DDECLTYPE_FLOAT16_2*/ 2 * sizeof(short int),
    /*WINED3DDECLTYPE_FLOAT16_4*/ 4 * sizeof(short int)
};

typedef struct {
    BYTE usage;
    BYTE usage_idx;
} wined3d_usage_t;

static const wined3d_usage_t wined3d_usage_lookup[] = {
    /*D3DVSDE_POSITION*/     {WINED3DDECLUSAGE_POSITION,     0},
    /*D3DVSDE_BLENDWEIGHT*/  {WINED3DDECLUSAGE_BLENDWEIGHT,  0},
    /*D3DVSDE_BLENDINDICES*/ {WINED3DDECLUSAGE_BLENDINDICES, 0},
    /*D3DVSDE_NORMAL*/       {WINED3DDECLUSAGE_NORMAL,       0},
    /*D3DVSDE_PSIZE*/        {WINED3DDECLUSAGE_PSIZE,        0},
    /*D3DVSDE_DIFFUSE*/      {WINED3DDECLUSAGE_COLOR,        0},
    /*D3DVSDE_SPECULAR*/     {WINED3DDECLUSAGE_COLOR,        1},
    /*D3DVSDE_TEXCOORD0*/    {WINED3DDECLUSAGE_TEXCOORD,     0},
    /*D3DVSDE_TEXCOORD1*/    {WINED3DDECLUSAGE_TEXCOORD,     1},
    /*D3DVSDE_TEXCOORD2*/    {WINED3DDECLUSAGE_TEXCOORD,     2},
    /*D3DVSDE_TEXCOORD3*/    {WINED3DDECLUSAGE_TEXCOORD,     3},
    /*D3DVSDE_TEXCOORD4*/    {WINED3DDECLUSAGE_TEXCOORD,     4},
    /*D3DVSDE_TEXCOORD5*/    {WINED3DDECLUSAGE_TEXCOORD,     5},
    /*D3DVSDE_TEXCOORD6*/    {WINED3DDECLUSAGE_TEXCOORD,     6},
    /*D3DVSDE_TEXCOORD7*/    {WINED3DDECLUSAGE_TEXCOORD,     7},
    /*D3DVSDE_POSITION2*/    {WINED3DDECLUSAGE_POSITION,     1},
    /*D3DVSDE_NORMAL2*/      {WINED3DDECLUSAGE_NORMAL,       1},
};

/* TODO: find out where rhw (or positionT) is for declaration8 */
size_t convert_to_wined3d_declaration(const DWORD *d3d8_elements, DWORD *d3d8_elements_size, WINED3DVERTEXELEMENT **wined3d_elements)
{
    const DWORD *token = d3d8_elements;
    WINED3DVERTEXELEMENT *element;
    D3DVSD_TOKENTYPE token_type;
    unsigned int element_count = 0;
    DWORD stream = 0;
    int offset = 0;

    TRACE("d3d8_elements %p, wined3d_elements %p\n", d3d8_elements, wined3d_elements);

    /* 128 should be enough for anyone... */
    *wined3d_elements = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 128 * sizeof(WINED3DVERTEXELEMENT));
    while (D3DVSD_END() != *token)
    {
        token_type = ((*token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);

        if (token_type == D3DVSD_TOKEN_STREAM && !(*token & D3DVSD_STREAMTESSMASK))
        {
            stream = ((*token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT);
            offset = 0;
        } else if (token_type == D3DVSD_TOKEN_STREAMDATA && !(token_type & 0x10000000)) {
            DWORD type = ((*token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT);
            DWORD reg  = ((*token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);

            TRACE("Adding element %d:\n", element_count);

            element = *wined3d_elements + element_count++;
            element->Stream = stream;
            element->Method = WINED3DDECLMETHOD_DEFAULT;
            element->Usage = wined3d_usage_lookup[reg].usage;
            element->UsageIndex = wined3d_usage_lookup[reg].usage_idx;
            element->Type = type;
            element->Offset = offset;
            element->Reg = reg;

            offset += wined3d_type_sizes[type];
        } else if (token_type == D3DVSD_TOKEN_STREAMDATA && (token_type & 0x10000000)) {
            TRACE(" 0x%08x SKIP(%u)\n", token_type, ((token_type & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT));
            offset += sizeof(DWORD) * ((token_type & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT);
        }

        if (element_count >= 127) {
            ERR("More than 127 elements?\n");
            break;
        }

        token += parse_token(token);
    }

    /* END */
    element = *wined3d_elements + element_count++;
    element->Stream = 0xFF;
    element->Type = WINED3DDECLTYPE_UNUSED;

    *d3d8_elements_size = (++token - d3d8_elements) * sizeof(DWORD);

    return element_count;
}

const IDirect3DVertexDeclaration8Vtbl Direct3DVertexDeclaration8_Vtbl =
{
    IDirect3DVertexDeclaration8Impl_QueryInterface,
    IDirect3DVertexDeclaration8Impl_AddRef,
    IDirect3DVertexDeclaration8Impl_Release
};
