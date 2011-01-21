/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_decl);

static void dump_wined3dvertexelement(const WINED3DVERTEXELEMENT *element) {
    TRACE("     format: %s (%#x)\n", debug_d3dformat(element->format), element->format);
    TRACE(" input_slot: %u\n", element->input_slot);
    TRACE("     offset: %u\n", element->offset);
    TRACE("output_slot: %u\n", element->output_slot);
    TRACE("     method: %s (%#x)\n", debug_d3ddeclmethod(element->method), element->method);
    TRACE("      usage: %s (%#x)\n", debug_d3ddeclusage(element->usage), element->usage);
    TRACE("  usage_idx: %u\n", element->usage_idx);
}

/* *******************************************
   IWineD3DVertexDeclaration IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexDeclarationImpl_QueryInterface(IWineD3DVertexDeclaration *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DVertexDeclaration)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexDeclarationImpl_AddRef(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IWineD3DVertexDeclarationImpl_Release(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->elements);
        This->parent_ops->wined3d_object_destroyed(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexDeclaration parts follow
   ******************************************* */

static void * WINAPI IWineD3DVertexDeclarationImpl_GetParent(IWineD3DVertexDeclaration *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DVertexDeclarationImpl *)iface)->parent;
}

static BOOL declaration_element_valid_ffp(const WINED3DVERTEXELEMENT *element)
{
    switch(element->usage)
    {
        case WINED3DDECLUSAGE_POSITION:
        case WINED3DDECLUSAGE_POSITIONT:
            switch(element->format)
            {
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_BLENDWEIGHT:
            switch(element->format)
            {
                case WINED3DFMT_R32_FLOAT:
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_B8G8R8A8_UNORM:
                case WINED3DFMT_R8G8B8A8_UINT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_NORMAL:
            switch(element->format)
            {
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_TEXCOORD:
            switch(element->format)
            {
                case WINED3DFMT_R32_FLOAT:
                case WINED3DFMT_R32G32_FLOAT:
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_R16G16_SINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R16G16_FLOAT:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_COLOR:
            switch(element->format)
            {
                case WINED3DFMT_R32G32B32_FLOAT:
                case WINED3DFMT_R32G32B32A32_FLOAT:
                case WINED3DFMT_B8G8R8A8_UNORM:
                case WINED3DFMT_R8G8B8A8_UINT:
                case WINED3DFMT_R16G16B16A16_SINT:
                case WINED3DFMT_R8G8B8A8_UNORM:
                case WINED3DFMT_R16G16B16A16_SNORM:
                case WINED3DFMT_R16G16B16A16_UNORM:
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    return TRUE;
                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

static const IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexDeclarationImpl_QueryInterface,
    IWineD3DVertexDeclarationImpl_AddRef,
    IWineD3DVertexDeclarationImpl_Release,
    /* IWineD3DVertexDeclaration */
    IWineD3DVertexDeclarationImpl_GetParent,
};

HRESULT vertexdeclaration_init(IWineD3DVertexDeclarationImpl *declaration, IWineD3DDeviceImpl *device,
        const WINED3DVERTEXELEMENT *elements, UINT element_count,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    WORD preloaded = 0; /* MAX_STREAMS, 16 */
    unsigned int i;

    if (TRACE_ON(d3d_decl))
    {
        for (i = 0; i < element_count; ++i)
        {
            dump_wined3dvertexelement(elements + i);
        }
    }

    declaration->lpVtbl = &IWineD3DVertexDeclaration_Vtbl;
    declaration->ref = 1;
    declaration->parent = parent;
    declaration->parent_ops = parent_ops;
    declaration->device = device;
    declaration->elements = HeapAlloc(GetProcessHeap(), 0, sizeof(*declaration->elements) * element_count);
    if (!declaration->elements)
    {
        ERR("Failed to allocate elements memory.\n");
        return E_OUTOFMEMORY;
    }
    declaration->element_count = element_count;

    /* Do some static analysis on the elements to make reading the
     * declaration more comfortable for the drawing code. */
    for (i = 0; i < element_count; ++i)
    {
        struct wined3d_vertex_declaration_element *e = &declaration->elements[i];

        e->format = wined3d_get_format(gl_info, elements[i].format);
        e->ffp_valid = declaration_element_valid_ffp(&elements[i]);
        e->input_slot = elements[i].input_slot;
        e->offset = elements[i].offset;
        e->output_slot = elements[i].output_slot;
        e->method = elements[i].method;
        e->usage = elements[i].usage;
        e->usage_idx = elements[i].usage_idx;

        if (e->usage == WINED3DDECLUSAGE_POSITIONT) declaration->position_transformed = TRUE;

        /* Find the streams used in the declaration. The vertex buffers have
         * to be loaded when drawing, but filter tesselation pseudo streams. */
        if (e->input_slot >= MAX_STREAMS) continue;

        if (!e->format->gl_vtx_format)
        {
            FIXME("The application tries to use an unsupported format (%s), returning E_FAIL.\n",
                    debug_d3dformat(elements[i].format));
            HeapFree(GetProcessHeap(), 0, declaration->elements);
            return E_FAIL;
        }

        if (e->offset & 0x3)
        {
            WARN("Declaration element %u is not 4 byte aligned(%u), returning E_FAIL.\n", i, e->offset);
            HeapFree(GetProcessHeap(), 0, declaration->elements);
            return E_FAIL;
        }

        if (!(preloaded & (1 << e->input_slot)))
        {
            declaration->streams[declaration->num_streams] = e->input_slot;
            ++declaration->num_streams;
            preloaded |= 1 << e->input_slot;
        }

        if (elements[i].format == WINED3DFMT_R16G16_FLOAT || elements[i].format == WINED3DFMT_R16G16B16A16_FLOAT)
        {
            if (!gl_info->supported[ARB_HALF_FLOAT_VERTEX]) declaration->half_float_conv_needed = TRUE;
        }
    }

    return WINED3D_OK;
}
