/*
 * Copyright 2008-2009 Henri Verbeet for CodeWeavers
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

#ifndef __WINE_D3D11_PRIVATE_H
#define __WINE_D3D11_PRIVATE_H

#include "wine/debug.h"
#include "wine/heap.h"

#include <assert.h>

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "d3d11_4.h"
#ifdef D3D11_INIT_GUID
#include "initguid.h"
#endif
#include "wine/wined3d.h"
#include "wine/winedxgi.h"
#include "wine/rbtree.h"




DEFINE_GUID(IID_ID3D10Texture3D, 0x9b7e4c05, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10ShaderResourceView, 0x9b7e4c07, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);

DEFINE_GUID(IID_ID3D10DepthStencilView, 0x9b7e4c09, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11UnorderedAccessView, 0x28acf509, 0x7f5c, 0x48f6, 0x86,0x11, 0xf3,0x16,0x01,0x0a,0x63,0x80);
DEFINE_GUID(IID_ID3D11Texture1D, 0xf8fb5c27, 0xc6b3, 0x4f75, 0xa4,0xc8, 0x43,0x9a,0xf2,0xef,0x56,0x4c);

DEFINE_GUID(IID_ID3D11ShaderResourceView, 0xb0e06fe0, 0x8192, 0x4e1a, 0xb1,0xca, 0x36,0xd7,0x41,0x47,0x10,0xb2);
DEFINE_GUID(IID_ID3D11RenderTargetView, 0xdfdba067, 0x0b8d, 0x4865, 0x87,0x5b, 0xd7,0xb4,0x51,0x6c,0xc1,0x64);

DEFINE_GUID(IID_ID3D11DepthStencilView, 0x9fdac92a, 0x1876, 0x48c3, 0xaf,0xad, 0x25,0xb9,0x4f,0x84,0xa9,0xb6);
DEFINE_GUID(IID_ID3D11View, 0x839d1216, 0xbb2e, 0x412b, 0xb7,0xf4, 0xa9,0xdb,0xeb,0xe0,0x8e,0xd1);
DEFINE_GUID(IID_ID3D10Texture2D, 0x9b7e4c04, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11Texture3D, 0x037e866e, 0xf56d, 0x4357, 0xa8,0xaf, 0x9d,0xab,0xbe,0x6e,0x25,0x0e);
DEFINE_GUID(IID_ID3D11Texture2D, 0x6f15aaf2, 0xd208, 0x4e89, 0x9a,0xb4, 0x48,0x95,0x35,0xd3,0x4f,0x9c);
DEFINE_GUID(IID_IDXGISurface, 0xcafcb56c, 0x6ac3, 0x4889, 0xbf,0x47, 0x9e,0x23,0xbb,0xd2,0x60,0xec);
DEFINE_GUID(IID_IDXGISurface1, 0x4ae63092, 0x6327, 0x4c1b, 0x80,0xae, 0xbf,0xe1,0x2e,0xa3,0x2b,0x86);
DEFINE_GUID(IID_ID3D10BlendState1, 0xedad8d99, 0x8a35, 0x4d6d, 0x85,0x66, 0x2e,0xa2,0x76,0xcd,0xe1,0x61);
DEFINE_GUID(IID_ID3D10SamplerState, 0x9b7e4c0c, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10RasterizerState, 0xa2a07292, 0x89af, 0x4345, 0xbe,0x2e, 0xc5,0x3d,0x9f,0xbb,0x6e,0x9f);
DEFINE_GUID(IID_ID3D10DepthStencilState, 0x2b4b1cc8, 0xa4ad, 0x41f8, 0x83,0x22, 0xca,0x86,0xfc,0x3e,0xc6,0x75);
DEFINE_GUID(IID_ID3D10BlendState, 0xedad8d19, 0x8a35, 0x4d6d, 0x85,0x66, 0x2e,0xa2,0x76,0xcd,0xe1,0x61);
DEFINE_GUID(IID_ID3D11SamplerState, 0xda6fea51, 0x564c, 0x4487, 0x98,0x10, 0xf0,0xd0,0xf9,0xb4,0xe3,0xa5);
DEFINE_GUID(IID_ID3D11RasterizerState, 0x9bb4ab81, 0xab1a, 0x4d8f, 0xb5,0x06, 0xfc,0x04,0x20,0x0b,0x6e,0xe7);
DEFINE_GUID(IID_ID3D11DepthStencilState, 0x03823efb, 0x8d8f, 0x4e1c, 0x9a,0xa2, 0xf6,0x4b,0xb2,0xcb,0xfd,0xf1);
DEFINE_GUID(IID_ID3D11DeviceContext, 0xc0bfa96c, 0xe089, 0x44fb, 0x8e,0xaf, 0x26,0xf8,0x79,0x61,0x90,0xda);
DEFINE_GUID(IID_ID3D11BlendState, 0xcc86fabe, 0xda55, 0x401d, 0x85,0xe7, 0xe3,0xc9,0xde,0x28,0x77,0xe9);
DEFINE_GUID(IID_ID3D10PixelShader, 0x4968b601, 0x9d00, 0x4cde, 0x83,0x46, 0x8e,0x7f,0x67,0x58,0x19,0xb6);
DEFINE_GUID(IID_ID3D10GeometryShader, 0x6316be88, 0x54cd, 0x4040, 0xab,0x44, 0x20,0x46,0x1b,0xc8,0x1f,0x68);
DEFINE_GUID(IID_ID3D11VertexShader, 0x3b301d64, 0xd678, 0x4289, 0x88,0x97, 0x22,0xf8,0x92,0x8b,0x72,0xf3);
DEFINE_GUID(IID_ID3D11PixelShader, 0xea82e40d, 0x51dc, 0x4f33, 0x93,0xd4, 0xdb,0x7c,0x91,0x25,0xae,0x8c);

DEFINE_GUID(IID_ID3D11GeometryShader, 0x38325b96, 0xeffb, 0x4022, 0xba,0x02, 0x2e,0x79,0x5b,0x70,0x27,0x5c);

DEFINE_GUID(IID_ID3D11DomainShader, 0xf582c508, 0x0f36, 0x490c, 0x99,0x77, 0x31,0xee,0xce,0x26,0x8c,0xfa);
DEFINE_GUID(IID_ID3D11ComputeShader, 0x4f5b196e, 0xc2bd, 0x495e, 0xbd,0x01, 0x1f,0xde,0xd3,0x8e,0x49,0x69);
DEFINE_GUID(IID_ID3D10InputLayout, 0x9b7e4c0b, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11InputLayout, 0xe4819ddc, 0x4cf0, 0x4025, 0xbd,0x26, 0x5d,0xe8,0x2a,0x3e,0x07,0xb7);

DEFINE_GUID(IID_ID3D11HullShader, 0x8e5c6061, 0x628a, 0x4c8e, 0x82,0x64, 0xbb,0xe4,0x5c,0xb3,0xd5,0xdd);

DEFINE_GUID(IID_ID3D10Multithread, 0x9b7e4e00, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11DeviceContext1, 0xbb2c6faa, 0xb5fb, 0x4082, 0x8e,0x6b, 0x38,0x8b,0x8c,0xfa,0x90,0xe1);
DEFINE_GUID(IID_ID3D11Device1, 0xa04bfb29, 0x08ef, 0x43d6, 0xa4,0x9c, 0xa9,0xbd,0xbd,0xcb,0xe6,0x86);
DEFINE_GUID(IID_ID3D11Device2, 0x9d06dffa, 0xd1e5, 0x4d07, 0x83,0xa8, 0x1b,0xb1,0x23,0xf2,0xf8,0x41);
DEFINE_GUID(IID_ID3D11Multithread, 0x9b7e4e00, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Texture1D,0x9B7E4C03,0x342C,0x4106,0xA1,0x9F,0x4F,0x27,0x04,0xF6,0x89,0xF0);

DEFINE_GUID(IID_ID3D11Device, 0xdb6f6ddb, 0xac77, 0x4e88, 0x82,0x53, 0x81,0x9d,0xf9,0xbb,0xf1,0x40);
DEFINE_GUID(IID_IDXGIDevice, 0x54ec77fa, 0x1377, 0x44e6, 0x8c,0x32, 0x88,0xfd,0x5f,0x44,0xc8,0x4c);
DEFINE_GUID(IID_IDXGIFactory, 0x7b7166ec, 0x21c7, 0x44ae, 0xb2,0x1a, 0xc9,0xae,0x32,0x1a,0xe3,0x69);
DEFINE_GUID(IID_IDXGIDeviceSubObject, 0x3d3e0379, 0xf9de, 0x4d58, 0xbb,0x6c, 0x18,0xd6,0x29,0x92,0xf1,0xa6);
DEFINE_GUID(IID_ID3D11ClassLinkage, 0xddf57cba, 0x9543, 0x46e4, 0xa1,0x2b, 0xf2,0x07,0xa0,0xfe,0x7f,0xed);
DEFINE_GUID(IID_ID3D10VertexShader, 0x9b7e4c0a, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11Texture2D1, 0x51218251, 0x1e33, 0x4617, 0x9c,0xcb, 0x4d,0x3a,0x43,0x67,0xe7,0xbb);
DEFINE_GUID(IID_ID3D11BlendState1, 0xcc86fabe, 0xda55, 0x401d, 0x85,0xe7, 0xe3,0xc9,0xde,0x28,0x77,0xe9);
DEFINE_GUID(IID_ID3D11UnorderedAccessView1, 0x7b3b6153, 0xa886, 0x4544, 0xab,0x37, 0x65,0x37,0xc8,0x50,0x04,0x03);
DEFINE_GUID(IID_ID3D10View, 0xc902b03f, 0x60a7, 0x49ba, 0x99,0x36, 0x2a,0x3a,0xb3,0x7a,0x7e,0x33);
DEFINE_GUID(IID_ID3D10RenderTargetView, 0x9b7e4c08, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10ShaderResourceView1, 0x9b7e4c87, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Buffer, 0x9b7e4c02, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Resource, 0x9b7e4c01, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11Buffer, 0x48570b85, 0xd1ee, 0x4fcd, 0xa2,0x50, 0xeb,0x35,0x07,0x22,0xb0,0x37);
DEFINE_GUID(IID_ID3D11Resource, 0xdc8e63f3, 0xd12b, 0x4952, 0xb4,0x7b, 0x5e,0x45,0x02,0x6a,0x86,0x2d);
DEFINE_GUID(IID_ID3D10Predicate, 0x9b7e4c10, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Query, 0x9b7e4c0e, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Asynchronous, 0x9b7e4c0d, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D11Predicate, 0x9eb576dd, 0x9f77, 0x4d86, 0x81,0xaa, 0x8b,0xab,0x5f,0xe4,0x90,0xe2);
DEFINE_GUID(IID_ID3D11Query, 0xd6c00747, 0x87b7, 0x425e, 0xb8,0x4d, 0x44,0xd1,0x08,0x56,0x0a,0xfd);
DEFINE_GUID(IID_ID3D11Query1, 0x631b4766, 0x36dc, 0x461d, 0x8d,0xb6, 0xc4,0x7e,0x13,0xe6,0x09,0x16);
DEFINE_GUID(IID_ID3D11Asynchronous, 0x4b35d0cd, 0x1e15, 0x4258, 0x9c,0x98, 0x1b,0x13,0x33,0xf6,0xdd,0x3b);
DEFINE_GUID(IID_ID3D11DeviceChild, 0x1841e5c8, 0x16b0, 0x489b, 0xbc,0xc8, 0x44,0xcf,0xb0,0xd5,0xde,0xae);
DEFINE_GUID(IID_ID3D10Device1, 0x9b7e4c8f, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10DeviceChild, 0x9b7e4c00, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);
DEFINE_GUID(IID_ID3D10Device, 0x9b7e4c0f, 0x342c, 0x4106, 0xa1,0x9f, 0x4f,0x27,0x04,0xf6,0x89,0xf0);

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_AON9 MAKE_TAG('A', 'o', 'n', '9')
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')
#define TAG_OSG5 MAKE_TAG('O', 'S', 'G', '5')
#define TAG_OSGN MAKE_TAG('O', 'S', 'G', 'N')
#define TAG_PCSG MAKE_TAG('P', 'C', 'S', 'G')
#define TAG_SHDR MAKE_TAG('S', 'H', 'D', 'R')
#define TAG_SHEX MAKE_TAG('S', 'H', 'E', 'X')

struct d3d_device;

/* TRACE helper functions */
const char *debug_d3d10_primitive_topology(D3D10_PRIMITIVE_TOPOLOGY topology) DECLSPEC_HIDDEN;
const char *debug_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;
const char *debug_float4(const float *values) DECLSPEC_HIDDEN;

DXGI_FORMAT dxgi_format_from_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_dxgi_format(DXGI_FORMAT format) DECLSPEC_HIDDEN;
void d3d11_primitive_topology_from_wined3d_primitive_type(enum wined3d_primitive_type primitive_type,
        unsigned int patch_vertex_count, D3D11_PRIMITIVE_TOPOLOGY *topology) DECLSPEC_HIDDEN;
void wined3d_primitive_type_from_d3d11_primitive_topology(D3D11_PRIMITIVE_TOPOLOGY topology,
        enum wined3d_primitive_type *type, unsigned int *patch_vertex_count) DECLSPEC_HIDDEN;
unsigned int wined3d_getdata_flags_from_d3d11_async_getdata_flags(unsigned int d3d11_flags) DECLSPEC_HIDDEN;
DWORD wined3d_usage_from_d3d11(enum D3D11_USAGE usage) DECLSPEC_HIDDEN;
struct wined3d_resource *wined3d_resource_from_d3d11_resource(ID3D11Resource *resource) DECLSPEC_HIDDEN;
struct wined3d_resource *wined3d_resource_from_d3d10_resource(ID3D10Resource *resource) DECLSPEC_HIDDEN;
DWORD wined3d_map_flags_from_d3d11_map_type(D3D11_MAP map_type) DECLSPEC_HIDDEN;
DWORD wined3d_clear_flags_from_d3d11_clear_flags(UINT clear_flags) DECLSPEC_HIDDEN;
unsigned int wined3d_access_from_d3d11(D3D11_USAGE usage, UINT cpu_access) DECLSPEC_HIDDEN;

enum D3D11_USAGE d3d11_usage_from_d3d10_usage(enum D3D10_USAGE usage) DECLSPEC_HIDDEN;
enum D3D10_USAGE d3d10_usage_from_d3d11_usage(enum D3D11_USAGE usage) DECLSPEC_HIDDEN;
UINT d3d11_bind_flags_from_d3d10_bind_flags(UINT bind_flags) DECLSPEC_HIDDEN;
UINT d3d10_bind_flags_from_d3d11_bind_flags(UINT bind_flags) DECLSPEC_HIDDEN;
UINT d3d11_cpu_access_flags_from_d3d10_cpu_access_flags(UINT cpu_access_flags) DECLSPEC_HIDDEN;
UINT d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(UINT cpu_access_flags) DECLSPEC_HIDDEN;
UINT d3d11_resource_misc_flags_from_d3d10_resource_misc_flags(UINT resource_misc_flags) DECLSPEC_HIDDEN;
UINT d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(UINT resource_misc_flags) DECLSPEC_HIDDEN;

BOOL validate_d3d11_resource_access_flags(D3D11_RESOURCE_DIMENSION resource_dimension,
        D3D11_USAGE usage, UINT bind_flags, UINT cpu_access_flags,
        D3D_FEATURE_LEVEL feature_level) DECLSPEC_HIDDEN;

HRESULT d3d_get_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT *data_size, void *data) DECLSPEC_HIDDEN;
HRESULT d3d_set_private_data(struct wined3d_private_store *store,
        REFGUID guid, UINT data_size, const void *data) DECLSPEC_HIDDEN;
HRESULT d3d_set_private_data_interface(struct wined3d_private_store *store,
        REFGUID guid, const IUnknown *object) DECLSPEC_HIDDEN;

static inline unsigned int wined3d_bind_flags_from_d3d11(UINT bind_flags)
{
    return bind_flags;
}

static inline UINT d3d11_bind_flags_from_wined3d(unsigned int bind_flags)
{
    return bind_flags;
}

/* ID3D11Texture1D, ID3D10Texture1D */
struct d3d_texture1d
{
    ID3D11Texture1D ID3D11Texture1D_iface;
    ID3D10Texture1D ID3D10Texture1D_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    IUnknown *dxgi_surface;
    struct wined3d_texture *wined3d_texture;
    D3D11_TEXTURE1D_DESC desc;
    ID3D11Device2 *device;
};

HRESULT d3d_texture1d_create(struct d3d_device *device, const D3D11_TEXTURE1D_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture1d **texture) DECLSPEC_HIDDEN;
struct d3d_texture1d *unsafe_impl_from_ID3D11Texture1D(ID3D11Texture1D *iface) DECLSPEC_HIDDEN;
struct d3d_texture1d *unsafe_impl_from_ID3D10Texture1D(ID3D10Texture1D *iface) DECLSPEC_HIDDEN;

/* ID3D11Texture2D, ID3D10Texture2D */
struct d3d_texture2d
{
    ID3D11Texture2D ID3D11Texture2D_iface;
    ID3D10Texture2D ID3D10Texture2D_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    IUnknown *dxgi_surface;
    struct wined3d_texture *wined3d_texture;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Device2 *device;
};

static inline struct d3d_texture2d *impl_from_ID3D11Texture2D(ID3D11Texture2D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture2d, ID3D11Texture2D_iface);
}

HRESULT d3d_texture2d_create(struct d3d_device *device, const D3D11_TEXTURE2D_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture2d **texture) DECLSPEC_HIDDEN;
struct d3d_texture2d *unsafe_impl_from_ID3D11Texture2D(ID3D11Texture2D *iface) DECLSPEC_HIDDEN;
struct d3d_texture2d *unsafe_impl_from_ID3D10Texture2D(ID3D10Texture2D *iface) DECLSPEC_HIDDEN;

/* ID3D11Texture3D, ID3D10Texture3D */
struct d3d_texture3d
{
    ID3D11Texture3D ID3D11Texture3D_iface;
    ID3D10Texture3D ID3D10Texture3D_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_texture *wined3d_texture;
    D3D11_TEXTURE3D_DESC desc;
    ID3D11Device2 *device;
};

HRESULT d3d_texture3d_create(struct d3d_device *device, const D3D11_TEXTURE3D_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture3d **texture) DECLSPEC_HIDDEN;
struct d3d_texture3d *unsafe_impl_from_ID3D11Texture3D(ID3D11Texture3D *iface) DECLSPEC_HIDDEN;
struct d3d_texture3d *unsafe_impl_from_ID3D10Texture3D(ID3D10Texture3D *iface) DECLSPEC_HIDDEN;

/* ID3D11Buffer, ID3D10Buffer */
struct d3d_buffer
{
    ID3D11Buffer ID3D11Buffer_iface;
    ID3D10Buffer ID3D10Buffer_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_buffer *wined3d_buffer;
    D3D11_BUFFER_DESC desc;
    ID3D11Device2 *device;
};

HRESULT d3d_buffer_create(struct d3d_device *device, const D3D11_BUFFER_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_buffer **buffer) DECLSPEC_HIDDEN;
struct d3d_buffer *unsafe_impl_from_ID3D11Buffer(ID3D11Buffer *iface) DECLSPEC_HIDDEN;
struct d3d_buffer *unsafe_impl_from_ID3D10Buffer(ID3D10Buffer *iface) DECLSPEC_HIDDEN;

/* ID3D11DepthStencilView, ID3D10DepthStencilView */
struct d3d_depthstencil_view
{
    ID3D11DepthStencilView ID3D11DepthStencilView_iface;
    ID3D10DepthStencilView ID3D10DepthStencilView_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_rendertarget_view *wined3d_view;
    D3D11_DEPTH_STENCIL_VIEW_DESC desc;
    ID3D11Resource *resource;
    ID3D11Device2 *device;
};

HRESULT d3d_depthstencil_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_DEPTH_STENCIL_VIEW_DESC *desc, struct d3d_depthstencil_view **view) DECLSPEC_HIDDEN;
struct d3d_depthstencil_view *unsafe_impl_from_ID3D11DepthStencilView(ID3D11DepthStencilView *iface) DECLSPEC_HIDDEN;
struct d3d_depthstencil_view *unsafe_impl_from_ID3D10DepthStencilView(ID3D10DepthStencilView *iface) DECLSPEC_HIDDEN;

/* ID3D11RenderTargetView, ID3D10RenderTargetView */
struct d3d_rendertarget_view
{
    ID3D11RenderTargetView ID3D11RenderTargetView_iface;
    ID3D10RenderTargetView ID3D10RenderTargetView_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_rendertarget_view *wined3d_view;
    D3D11_RENDER_TARGET_VIEW_DESC desc;
    ID3D11Resource *resource;
    ID3D11Device2 *device;
};

HRESULT d3d_rendertarget_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_RENDER_TARGET_VIEW_DESC *desc, struct d3d_rendertarget_view **view) DECLSPEC_HIDDEN;
struct d3d_rendertarget_view *unsafe_impl_from_ID3D11RenderTargetView(ID3D11RenderTargetView *iface) DECLSPEC_HIDDEN;
struct d3d_rendertarget_view *unsafe_impl_from_ID3D10RenderTargetView(ID3D10RenderTargetView *iface) DECLSPEC_HIDDEN;

/* ID3D11ShaderResourceView, ID3D10ShaderResourceView1 */
struct d3d_shader_resource_view
{
    ID3D11ShaderResourceView ID3D11ShaderResourceView_iface;
    ID3D10ShaderResourceView1 ID3D10ShaderResourceView1_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader_resource_view *wined3d_view;
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    ID3D11Resource *resource;
    ID3D11Device2 *device;
};

HRESULT d3d_shader_resource_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *desc, struct d3d_shader_resource_view **view) DECLSPEC_HIDDEN;
struct d3d_shader_resource_view *unsafe_impl_from_ID3D11ShaderResourceView(
        ID3D11ShaderResourceView *iface) DECLSPEC_HIDDEN;
struct d3d_shader_resource_view *unsafe_impl_from_ID3D10ShaderResourceView(
        ID3D10ShaderResourceView *iface) DECLSPEC_HIDDEN;

/* ID3D11UnorderedAccessView */
struct d3d11_unordered_access_view
{
    ID3D11UnorderedAccessView ID3D11UnorderedAccessView_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_unordered_access_view *wined3d_view;
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    ID3D11Resource *resource;
    ID3D11Device2 *device;
};

HRESULT d3d11_unordered_access_view_create(struct d3d_device *device, ID3D11Resource *resource,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC *desc, struct d3d11_unordered_access_view **view) DECLSPEC_HIDDEN;
struct d3d11_unordered_access_view *unsafe_impl_from_ID3D11UnorderedAccessView(
        ID3D11UnorderedAccessView *iface) DECLSPEC_HIDDEN;

/* ID3D11InputLayout, ID3D10InputLayout */
struct d3d_input_layout
{
    ID3D11InputLayout ID3D11InputLayout_iface;
    ID3D10InputLayout ID3D10InputLayout_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_vertex_declaration *wined3d_decl;
    ID3D11Device2 *device;
};

HRESULT d3d_input_layout_create(struct d3d_device *device,
        const D3D11_INPUT_ELEMENT_DESC *element_descs, UINT element_count,
        const void *shader_byte_code, SIZE_T shader_byte_code_length,
        struct d3d_input_layout **layout) DECLSPEC_HIDDEN;
struct d3d_input_layout *unsafe_impl_from_ID3D11InputLayout(ID3D11InputLayout *iface) DECLSPEC_HIDDEN;
struct d3d_input_layout *unsafe_impl_from_ID3D10InputLayout(ID3D10InputLayout *iface) DECLSPEC_HIDDEN;

/* ID3D11VertexShader, ID3D10VertexShader */
struct d3d_vertex_shader
{
    ID3D11VertexShader ID3D11VertexShader_iface;
    ID3D10VertexShader ID3D10VertexShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d_vertex_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_vertex_shader **shader) DECLSPEC_HIDDEN;
struct d3d_vertex_shader *unsafe_impl_from_ID3D11VertexShader(ID3D11VertexShader *iface) DECLSPEC_HIDDEN;
struct d3d_vertex_shader *unsafe_impl_from_ID3D10VertexShader(ID3D10VertexShader *iface) DECLSPEC_HIDDEN;

/* ID3D11HullShader */
struct d3d11_hull_shader
{
    ID3D11HullShader ID3D11HullShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d11_hull_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_hull_shader **shader) DECLSPEC_HIDDEN;
struct d3d11_hull_shader *unsafe_impl_from_ID3D11HullShader(ID3D11HullShader *iface) DECLSPEC_HIDDEN;

/* ID3D11DomainShader */
struct d3d11_domain_shader
{
    ID3D11DomainShader ID3D11DomainShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d11_domain_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_domain_shader **shader) DECLSPEC_HIDDEN;
struct d3d11_domain_shader *unsafe_impl_from_ID3D11DomainShader(ID3D11DomainShader *iface) DECLSPEC_HIDDEN;

/* ID3D11GeometryShader, ID3D10GeometryShader */
struct d3d_geometry_shader
{
    ID3D11GeometryShader ID3D11GeometryShader_iface;
    ID3D10GeometryShader ID3D10GeometryShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d_geometry_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        const D3D11_SO_DECLARATION_ENTRY *so_entries, unsigned int so_entry_count,
        const unsigned int *buffer_strides, unsigned int buffer_stride_count, unsigned int rasterizer_stream,
        struct d3d_geometry_shader **shader) DECLSPEC_HIDDEN;
struct d3d_geometry_shader *unsafe_impl_from_ID3D11GeometryShader(ID3D11GeometryShader *iface) DECLSPEC_HIDDEN;
struct d3d_geometry_shader *unsafe_impl_from_ID3D10GeometryShader(ID3D10GeometryShader *iface) DECLSPEC_HIDDEN;

/* ID3D11PixelShader, ID3D10PixelShader */
struct d3d_pixel_shader
{
    ID3D11PixelShader ID3D11PixelShader_iface;
    ID3D10PixelShader ID3D10PixelShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d_pixel_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d_pixel_shader **shader) DECLSPEC_HIDDEN;
struct d3d_pixel_shader *unsafe_impl_from_ID3D11PixelShader(ID3D11PixelShader *iface) DECLSPEC_HIDDEN;
struct d3d_pixel_shader *unsafe_impl_from_ID3D10PixelShader(ID3D10PixelShader *iface) DECLSPEC_HIDDEN;

/* ID3D11ComputeShader */
struct d3d11_compute_shader
{
    ID3D11ComputeShader ID3D11ComputeShader_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_shader *wined3d_shader;
    ID3D11Device2 *device;
};

HRESULT d3d11_compute_shader_create(struct d3d_device *device, const void *byte_code, SIZE_T byte_code_length,
        struct d3d11_compute_shader **shader) DECLSPEC_HIDDEN;
struct d3d11_compute_shader *unsafe_impl_from_ID3D11ComputeShader(ID3D11ComputeShader *iface) DECLSPEC_HIDDEN;

/* ID3D11ClassLinkage */
struct d3d11_class_linkage
{
    ID3D11ClassLinkage ID3D11ClassLinkage_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    ID3D11Device2 *device;
};

HRESULT d3d11_class_linkage_create(struct d3d_device *device,
        struct d3d11_class_linkage **class_linkage) DECLSPEC_HIDDEN;

/* ID3D11BlendState, ID3D10BlendState1 */
struct d3d_blend_state
{
    ID3D11BlendState ID3D11BlendState_iface;
    ID3D10BlendState1 ID3D10BlendState1_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_blend_state *wined3d_state;
    D3D11_BLEND_DESC desc;
    struct wine_rb_entry entry;
    ID3D11Device2 *device;
};

static inline struct d3d_blend_state *impl_from_ID3D11BlendState(ID3D11BlendState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_blend_state, ID3D11BlendState_iface);
}

HRESULT d3d_blend_state_create(struct d3d_device *device, const D3D11_BLEND_DESC *desc,
        struct d3d_blend_state **state) DECLSPEC_HIDDEN;
struct d3d_blend_state *unsafe_impl_from_ID3D11BlendState(ID3D11BlendState *iface) DECLSPEC_HIDDEN;
struct d3d_blend_state *unsafe_impl_from_ID3D10BlendState(ID3D10BlendState *iface) DECLSPEC_HIDDEN;

/* ID3D11DepthStencilState, ID3D10DepthStencilState */
struct d3d_depthstencil_state
{
    ID3D11DepthStencilState ID3D11DepthStencilState_iface;
    ID3D10DepthStencilState ID3D10DepthStencilState_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    D3D11_DEPTH_STENCIL_DESC desc;
    struct wine_rb_entry entry;
    ID3D11Device2 *device;
};

static inline struct d3d_depthstencil_state *impl_from_ID3D11DepthStencilState(ID3D11DepthStencilState *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_depthstencil_state, ID3D11DepthStencilState_iface);
}

HRESULT d3d_depthstencil_state_create(struct d3d_device *device, const D3D11_DEPTH_STENCIL_DESC *desc,
        struct d3d_depthstencil_state **state) DECLSPEC_HIDDEN;
struct d3d_depthstencil_state *unsafe_impl_from_ID3D11DepthStencilState(
        ID3D11DepthStencilState *iface) DECLSPEC_HIDDEN;
struct d3d_depthstencil_state *unsafe_impl_from_ID3D10DepthStencilState(
        ID3D10DepthStencilState *iface) DECLSPEC_HIDDEN;

/* ID3D11RasterizerState, ID3D10RasterizerState */
struct d3d_rasterizer_state
{
    ID3D11RasterizerState ID3D11RasterizerState_iface;
    ID3D10RasterizerState ID3D10RasterizerState_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_rasterizer_state *wined3d_state;
    D3D11_RASTERIZER_DESC desc;
    struct wine_rb_entry entry;
    ID3D11Device2 *device;
};

HRESULT d3d_rasterizer_state_create(struct d3d_device *device, const D3D11_RASTERIZER_DESC *desc,
        struct d3d_rasterizer_state **state) DECLSPEC_HIDDEN;
struct d3d_rasterizer_state *unsafe_impl_from_ID3D11RasterizerState(ID3D11RasterizerState *iface) DECLSPEC_HIDDEN;
struct d3d_rasterizer_state *unsafe_impl_from_ID3D10RasterizerState(ID3D10RasterizerState *iface) DECLSPEC_HIDDEN;

/* ID3D11SamplerState, ID3D10SamplerState */
struct d3d_sampler_state
{
    ID3D11SamplerState ID3D11SamplerState_iface;
    ID3D10SamplerState ID3D10SamplerState_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_sampler *wined3d_sampler;
    D3D11_SAMPLER_DESC desc;
    struct wine_rb_entry entry;
    ID3D11Device2 *device;
};

HRESULT d3d_sampler_state_create(struct d3d_device *device, const D3D11_SAMPLER_DESC *desc,
        struct d3d_sampler_state **state) DECLSPEC_HIDDEN;
struct d3d_sampler_state *unsafe_impl_from_ID3D11SamplerState(ID3D11SamplerState *iface) DECLSPEC_HIDDEN;
struct d3d_sampler_state *unsafe_impl_from_ID3D10SamplerState(ID3D10SamplerState *iface) DECLSPEC_HIDDEN;

/* ID3D11Query, ID3D10Query */
struct d3d_query
{
    ID3D11Query ID3D11Query_iface;
    ID3D10Query ID3D10Query_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
    struct wined3d_query *wined3d_query;
    BOOL predicate;
    D3D11_QUERY_DESC desc;
    ID3D11Device2 *device;
};

HRESULT d3d_query_create(struct d3d_device *device, const D3D11_QUERY_DESC *desc, BOOL predicate,
        struct d3d_query **query) DECLSPEC_HIDDEN;
struct d3d_query *unsafe_impl_from_ID3D11Query(ID3D11Query *iface) DECLSPEC_HIDDEN;
struct d3d_query *unsafe_impl_from_ID3D10Query(ID3D10Query *iface) DECLSPEC_HIDDEN;
struct d3d_query *unsafe_impl_from_ID3D11Asynchronous(ID3D11Asynchronous *iface) DECLSPEC_HIDDEN;

/* ID3D11DeviceContext - immediate context */
struct d3d11_immediate_context
{
    ID3D11DeviceContext1 ID3D11DeviceContext1_iface;
    ID3D11Multithread ID3D11Multithread_iface;
    LONG refcount;

    struct wined3d_private_store private_store;
};

/* ID3D11Device, ID3D10Device1 */
struct d3d_device
{
    IUnknown IUnknown_inner;
    ID3D11Device2 ID3D11Device2_iface;
    ID3D10Device1 ID3D10Device1_iface;
    ID3D10Multithread ID3D10Multithread_iface;
    IWineDXGIDeviceParent IWineDXGIDeviceParent_iface;
    IUnknown *outer_unk;
    LONG refcount;

    D3D_FEATURE_LEVEL feature_level;

    struct d3d11_immediate_context immediate_context;

    struct wined3d_device_parent device_parent;
    struct wined3d_device *wined3d_device;

    struct wine_rb_tree blend_states;
    struct wine_rb_tree depthstencil_states;
    struct wine_rb_tree rasterizer_states;
    struct wine_rb_tree sampler_states;

    struct d3d_depthstencil_state *depth_stencil_state;
    UINT stencil_ref;
};

static inline struct d3d_device *impl_from_ID3D11Device2(ID3D11Device2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, ID3D11Device2_iface);
}

static inline struct d3d_device *impl_from_ID3D10Device(ID3D10Device1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, ID3D10Device1_iface);
}

void d3d_device_init(struct d3d_device *device, void *outer_unknown) DECLSPEC_HIDDEN;

/* Layered device */
enum dxgi_device_layer_id
{
    DXGI_DEVICE_LAYER_DEBUG1        = 0x8,
    DXGI_DEVICE_LAYER_THREAD_SAFE   = 0x10,
    DXGI_DEVICE_LAYER_DEBUG2        = 0x20,
    DXGI_DEVICE_LAYER_SWITCH_TO_REF = 0x30,
    DXGI_DEVICE_LAYER_D3D10_DEVICE  = 0xffffffff,
};

struct layer_get_size_args
{
    DWORD unknown0;
    DWORD unknown1;
    DWORD *unknown2;
    DWORD *unknown3;
    IDXGIAdapter *adapter;
    WORD interface_major;
    WORD interface_minor;
    WORD version_build;
    WORD version_revision;
};

struct dxgi_device_layer
{
    enum dxgi_device_layer_id id;
    HRESULT (WINAPI *init)(enum dxgi_device_layer_id id, DWORD *count, DWORD *values);
    UINT (WINAPI *get_size)(enum dxgi_device_layer_id id, struct layer_get_size_args *args, DWORD unknown0);
    HRESULT (WINAPI *create)(enum dxgi_device_layer_id id, void **layer_base, DWORD unknown0,
            void *device_object, REFIID riid, void **device_layer);
};

HRESULT WINAPI DXGID3D10CreateDevice(HMODULE d3d10core, IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count, void **device);
HRESULT WINAPI DXGID3D10RegisterLayers(const struct dxgi_device_layer *layers, UINT layer_count);

#endif /* __WINE_D3D11_PRIVATE_H */
