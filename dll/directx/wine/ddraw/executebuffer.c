/* Direct3D ExecuteBuffer
 * Copyright (c) 1998-2004 Lionel ULMER
 * Copyright (c) 2002-2004 Christian Costa
 * Copyright (c) 2006      Stefan Dösinger
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * _dump_executedata
 * _dump_D3DEXECUTEBUFFERDESC
 *
 * Debug functions which write the executebuffer data to the console
 *
 *****************************************************************************/

static void _dump_executedata(const D3DEXECUTEDATA *lpData) {
    TRACE("dwSize : %lu\n", lpData->dwSize);
    TRACE("Vertex      Offset : %lu  Count  : %lu\n", lpData->dwVertexOffset, lpData->dwVertexCount);
    TRACE("Instruction Offset : %lu  Length : %lu\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
    TRACE("HVertex     Offset : %lu\n", lpData->dwHVertexOffset);
}

static void _dump_D3DEXECUTEBUFFERDESC(const D3DEXECUTEBUFFERDESC *lpDesc) {
    TRACE("dwSize       : %lu\n", lpDesc->dwSize);
    TRACE("dwFlags      : %#lx\n", lpDesc->dwFlags);
    TRACE("dwCaps       : %#lx\n", lpDesc->dwCaps);
    TRACE("dwBufferSize : %lu\n", lpDesc->dwBufferSize);
    TRACE("lpData       : %p\n", lpDesc->lpData);
}

HRESULT d3d_execute_buffer_execute(struct d3d_execute_buffer *buffer, struct d3d_device *device)
{
    DWORD is = buffer->data.dwInstructionOffset;
    char *instr = (char *)buffer->desc.lpData + is;
    unsigned int i, primitive_size;
    struct wined3d_map_desc map_desc;
    struct wined3d_box box = {0};
    HRESULT hr;

    TRACE("ExecuteData :\n");
    if (TRACE_ON(ddraw))
        _dump_executedata(&(buffer->data));

    for (;;)
    {
        D3DINSTRUCTION *current = (D3DINSTRUCTION *)instr;
        BYTE size;
        WORD count;

        count = current->wCount;
        size = current->bSize;
        instr += sizeof(*current);
        primitive_size = 0;

        switch (current->bOpcode)
        {
            case D3DOP_POINT:
            {
                const D3DPOINT *p = (D3DPOINT *)instr;
                wined3d_device_context_set_primitive_type(device->immediate_context, WINED3D_PT_POINTLIST, 0);
                wined3d_stateblock_set_stream_source(device->state, 0,
                        buffer->dst_vertex_buffer, 0, sizeof(D3DTLVERTEX));
                wined3d_stateblock_set_vertex_declaration(device->state,
                        ddraw_find_decl(device->ddraw, D3DFVF_TLVERTEX));

                d3d_device_apply_state(device, FALSE);
                for (i = 0; i < count; ++i)
                    wined3d_device_context_draw(device->immediate_context, p[i].wFirst, p[i].wCount, 0, 0);

                instr += sizeof(*p) * count;
                break;
            }

            case D3DOP_LINE:
                primitive_size = 2;
                wined3d_device_context_set_primitive_type(device->immediate_context, WINED3D_PT_LINELIST, 0);
                /* Drop through. */
            case D3DOP_TRIANGLE:
            {
                WORD *indices;
                unsigned int index_pos = buffer->index_pos, index_count;
                TRACE("TRIANGLE         (%d)\n", count);

                if (!count)
                    break;

                if (!primitive_size)
                {
                    wined3d_device_context_set_primitive_type(device->immediate_context, WINED3D_PT_TRIANGLELIST, 0);
                    primitive_size = 3;
                }

                index_count = count * primitive_size;
                if (buffer->index_size < index_count)
                {
                    unsigned int new_size = max(buffer->index_size * 2, index_count);
                    struct wined3d_buffer *new_buffer;
                    struct wined3d_buffer_desc desc;

                    desc.byte_width = new_size * sizeof(*indices);
                    desc.usage = WINED3DUSAGE_DYNAMIC | WINED3DUSAGE_STATICDECL;
                    desc.bind_flags = WINED3D_BIND_INDEX_BUFFER;
                    desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_W;
                    desc.misc_flags = 0;
                    desc.structure_byte_stride = 0;

                    if (FAILED(hr = wined3d_buffer_create(device->wined3d_device, &desc,
                            NULL, NULL, &ddraw_null_wined3d_parent_ops, &new_buffer)))
                        return hr;

                    buffer->index_size = new_size;
                    if (buffer->index_buffer)
                        wined3d_buffer_decref(buffer->index_buffer);
                    buffer->index_buffer = new_buffer;
                    index_pos = 0;
                }
                else if (buffer->index_size - index_count < index_pos)
                {
                    index_pos = 0;
                }

                box.left = index_pos * sizeof(*indices);
                box.right = (index_pos + index_count) * sizeof(*indices);
                if (FAILED(hr = wined3d_resource_map(wined3d_buffer_get_resource(buffer->index_buffer), 0, &map_desc,
                        &box, WINED3D_MAP_WRITE | (index_pos ? WINED3D_MAP_NOOVERWRITE : WINED3D_MAP_DISCARD))))
                    return hr;
                indices = map_desc.data;

                for (i = 0; i < count; ++i)
                {
                    D3DTRIANGLE *ci = (D3DTRIANGLE *)instr;
                    TRACE("  v1: %d  v2: %d  v3: %d\n",ci->v1, ci->v2, ci->v3);
                    TRACE("  Flags : ");
                    if (TRACE_ON(ddraw))
                    {
                        /* Wireframe */
                        if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)
                            TRACE("EDGEENABLE1 ");
                        if (ci->wFlags & D3DTRIFLAG_EDGEENABLE2)
                            TRACE("EDGEENABLE2 ");
                        if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)
                            TRACE("EDGEENABLE3 ");
                        /* Strips / Fans */
                        if (ci->wFlags == D3DTRIFLAG_EVEN)
                            TRACE("EVEN ");
                        if (ci->wFlags == D3DTRIFLAG_ODD)
                            TRACE("ODD ");
                        if (ci->wFlags == D3DTRIFLAG_START)
                            TRACE("START ");
                        if ((ci->wFlags > 0) && (ci->wFlags < 30))
                            TRACE("STARTFLAT(%u) ", ci->wFlags);
                        TRACE("\n");
                    }

                    switch (primitive_size)
                    {
                        case 3:
                            indices[(i * primitive_size) + 2] = ci->v3;
                            /* Drop through. */
                        case 2:
                            indices[(i * primitive_size) + 1] = ci->v2;
                            indices[(i * primitive_size)    ] = ci->v1;
                    }
                    instr += size;
                }

                wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->index_buffer), 0);

                wined3d_stateblock_set_stream_source(device->state, 0,
                        buffer->dst_vertex_buffer, 0, sizeof(D3DTLVERTEX));
                wined3d_stateblock_set_vertex_declaration(device->state,
                        ddraw_find_decl(device->ddraw, D3DFVF_TLVERTEX));
                wined3d_stateblock_set_index_buffer(device->state, buffer->index_buffer, WINED3DFMT_R16_UINT);
                d3d_device_apply_state(device, FALSE);
                wined3d_device_context_draw_indexed(device->immediate_context, 0, index_pos, index_count, 0, 0);

                buffer->index_pos = index_pos + index_count;
                break;
            }

            case D3DOP_MATRIXLOAD:
                WARN("MATRIXLOAD-s     (%u)\n", count);
                instr += count * size;
                break;

            case D3DOP_MATRIXMULTIPLY:
                TRACE("MATRIXMULTIPLY   (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DMATRIXMULTIPLY *ci = (D3DMATRIXMULTIPLY *)instr;
                    struct wined3d_matrix *a, *b, *c;

                    a = ddraw_get_object(NULL, ci->hDestMatrix - 1, DDRAW_HANDLE_MATRIX);
                    b = ddraw_get_object(NULL, ci->hSrcMatrix1 - 1, DDRAW_HANDLE_MATRIX);
                    c = ddraw_get_object(NULL, ci->hSrcMatrix2 - 1, DDRAW_HANDLE_MATRIX);

                    if (!a || !b || !c)
                    {
                        ERR("Invalid matrix handle (a %#lx -> %p, b %#lx -> %p, c %#lx -> %p).\n",
                                ci->hDestMatrix, a, ci->hSrcMatrix1, b, ci->hSrcMatrix2, c);
                    }
                    else
                    {
                        TRACE("dst %p, src1 %p, src2 %p.\n", a, b, c);
                        multiply_matrix(a, c, b);
                    }

                    instr += size;
                }
                break;

            case D3DOP_STATETRANSFORM:
                TRACE("STATETRANSFORM   (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DSTATE *ci = (D3DSTATE *)instr;
                    D3DMATRIX *m;

                    m = ddraw_get_object(NULL, ci->dwArg[0] - 1, DDRAW_HANDLE_MATRIX);
                    if (!m)
                    {
                        ERR("Invalid matrix handle %#lx.\n", ci->dwArg[0]);
                    }
                    else
                    {
                        if (ci->dtstTransformStateType == D3DTRANSFORMSTATE_WORLD)
                            device->world = ci->dwArg[0];
                        if (ci->dtstTransformStateType == D3DTRANSFORMSTATE_VIEW)
                            device->view = ci->dwArg[0];
                        if (ci->dtstTransformStateType == D3DTRANSFORMSTATE_PROJECTION)
                            device->proj = ci->dwArg[0];
                        IDirect3DDevice3_SetTransform(&device->IDirect3DDevice3_iface,
                                ci->dtstTransformStateType, m);
                    }

                    instr += size;
                }
                break;

            case D3DOP_STATELIGHT:
                TRACE("STATELIGHT       (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DSTATE *ci = (D3DSTATE *)instr;

                    if (FAILED(IDirect3DDevice3_SetLightState(&device->IDirect3DDevice3_iface,
                            ci->dlstLightStateType, ci->dwArg[0])))
                        WARN("Failed to set light state.\n");

                    instr += size;
                }
                break;

            case D3DOP_STATERENDER:
                TRACE("STATERENDER      (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DSTATE *ci = (D3DSTATE *)instr;

                    if (FAILED(IDirect3DDevice3_SetRenderState(&device->IDirect3DDevice3_iface,
                            ci->drstRenderStateType, ci->dwArg[0])))
                        WARN("Failed to set render state.\n");

                    instr += size;
                }
                break;

            case D3DOP_PROCESSVERTICES:
                TRACE("PROCESSVERTICES  (%d)\n", count);

                for (i = 0; i < count; ++i)
                {
                    D3DPROCESSVERTICES *ci = (D3DPROCESSVERTICES *)instr;
                    DWORD op = ci->dwFlags & D3DPROCESSVERTICES_OPMASK;

                    TRACE("  start %u, dest %u, count %lu, flags %#lx.\n",
                            ci->wStart, ci->wDest, ci->dwCount, ci->dwFlags);

                    if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
                        FIXME("D3DPROCESSVERTICES_UPDATEEXTENTS not implemented.\n");
                    if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
                        FIXME("D3DPROCESSVERTICES_NOCOLOR not implemented.\n");

                    switch (op)
                    {
                        case D3DPROCESSVERTICES_TRANSFORMLIGHT:
                        case D3DPROCESSVERTICES_TRANSFORM:
                            wined3d_stateblock_set_stream_source(device->state, 0,
                                    buffer->src_vertex_buffer, buffer->src_vertex_pos * sizeof(D3DVERTEX), sizeof(D3DVERTEX));
                            wined3d_stateblock_set_render_state(device->state, WINED3D_RS_LIGHTING,
                                    op == D3DPROCESSVERTICES_TRANSFORMLIGHT && !!device->material);
                            wined3d_stateblock_set_vertex_declaration(device->state,
                                    ddraw_find_decl(device->ddraw, op == D3DPROCESSVERTICES_TRANSFORMLIGHT
                                    ? D3DFVF_VERTEX : D3DFVF_LVERTEX));
                            d3d_device_sync_surfaces(device);
                            wined3d_device_process_vertices(device->wined3d_device, device->state, ci->wStart,
                                    ci->wDest, ci->dwCount, buffer->dst_vertex_buffer, NULL, 0, D3DFVF_TLVERTEX);
                            break;

                        case D3DPROCESSVERTICES_COPY:
                            box.left = (buffer->src_vertex_pos + ci->wStart) * sizeof(D3DTLVERTEX);
                            box.right = box.left + ci->dwCount * sizeof(D3DTLVERTEX);
                            box.top = box.front = 0;
                            box.bottom = box.back = 1;
                            wined3d_device_context_copy_sub_resource_region(device->immediate_context,
                                    wined3d_buffer_get_resource(buffer->dst_vertex_buffer), 0,
                                    ci->wDest * sizeof(D3DTLVERTEX), 0, 0,
                                    wined3d_buffer_get_resource(buffer->src_vertex_buffer), 0, &box, 0);
                            break;

                        default:
                            FIXME("Unhandled vertex processing op %#lx.\n", op);
                            break;
                    }

                    instr += size;
                }
                break;

            case D3DOP_TEXTURELOAD:
                TRACE("TEXTURELOAD    (%u)\n", count);

                for (i = 0; i < count; ++i)
                {
                    D3DTEXTURELOAD *ci = (D3DTEXTURELOAD *)instr;
                    struct ddraw_surface *dst, *src;

                    instr += size;

                    if (!(dst = ddraw_get_object(NULL, ci->hDestTexture - 1, DDRAW_HANDLE_SURFACE)))
                    {
                        WARN("Invalid destination texture handle %#lx.\n", ci->hDestTexture);
                        continue;
                    }
                    if (!(src = ddraw_get_object(NULL, ci->hSrcTexture - 1, DDRAW_HANDLE_SURFACE)))
                    {
                        WARN("Invalid source texture handle %#lx.\n", ci->hSrcTexture);
                        continue;
                    }

                    IDirect3DTexture2_Load(&dst->IDirect3DTexture2_iface, &src->IDirect3DTexture2_iface);
                }
                break;

            case D3DOP_EXIT:
                TRACE("EXIT             (%u)\n", count);
                instr += size;
                goto end_of_buffer;

            case D3DOP_BRANCHFORWARD:
                TRACE("BRANCHFORWARD    (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DBRANCH *ci = (D3DBRANCH *)instr;

                    if ((buffer->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue)
                    {
                        if (!ci->bNegate)
                        {
                            TRACE(" Branch to %ld\n", ci->dwOffset);
                            if (ci->dwOffset) {
                                instr = (char*)current + ci->dwOffset;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (ci->bNegate)
                        {
                            TRACE(" Branch to %ld\n", ci->dwOffset);
                            if (ci->dwOffset) {
                                instr = (char*)current + ci->dwOffset;
                                break;
                            }
                        }
                    }

                    instr += size;
                }
                break;

            case D3DOP_SPAN:
                WARN("SPAN-s           (%u)\n", count);
                instr += count * size;
                break;

            case D3DOP_SETSTATUS:
                TRACE("SETSTATUS        (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    buffer->data.dsStatus = *(D3DSTATUS *)instr;
                    instr += size;
                }
                break;

            default:
                ERR("Unhandled OpCode %#x.\n",current->bOpcode);
                instr += count * size;
                break;
        }
    }

end_of_buffer:
    return D3D_OK;
}

static inline struct d3d_execute_buffer *impl_from_IDirect3DExecuteBuffer(IDirect3DExecuteBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_execute_buffer, IDirect3DExecuteBuffer_iface);
}

static HRESULT WINAPI d3d_execute_buffer_QueryInterface(IDirect3DExecuteBuffer *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(&IID_IDirect3DExecuteBuffer, iid)
            || IsEqualGUID(&IID_IUnknown, iid))
    {
        IDirect3DExecuteBuffer_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::AddRef
 *
 * A normal AddRef method, nothing special
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_execute_buffer_AddRef(IDirect3DExecuteBuffer *iface)
{
    struct d3d_execute_buffer *buffer = impl_from_IDirect3DExecuteBuffer(iface);
    ULONG ref = InterlockedIncrement(&buffer->ref);

    TRACE("%p increasing refcount to %lu.\n", buffer, ref);

    return ref;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Release
 *
 * A normal Release method, nothing special
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI d3d_execute_buffer_Release(IDirect3DExecuteBuffer *iface)
{
    struct d3d_execute_buffer *buffer = impl_from_IDirect3DExecuteBuffer(iface);
    ULONG ref = InterlockedDecrement(&buffer->ref);

    TRACE("%p decreasing refcount to %lu.\n", buffer, ref);

    if (!ref)
    {
        if (buffer->need_free)
            free(buffer->desc.lpData);
        if (buffer->index_buffer)
            wined3d_buffer_decref(buffer->index_buffer);
        if (buffer->dst_vertex_buffer)
        {
            wined3d_buffer_decref(buffer->src_vertex_buffer);
            wined3d_buffer_decref(buffer->dst_vertex_buffer);
        }
        free(buffer);
    }

    return ref;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Initialize
 *
 * Initializes the Execute Buffer. This method exists for COM compliance
 * Nothing to do here.
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_Initialize(IDirect3DExecuteBuffer *iface,
        IDirect3DDevice *device, D3DEXECUTEBUFFERDESC *desc)
{
    TRACE("iface %p, device %p, desc %p.\n", iface, device, desc);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Lock
 *
 * Locks the buffer, so the app can write into it.
 *
 * Params:
 *  Desc: Pointer to return the buffer description. This Description contains
 *        a pointer to the buffer data.
 *
 * Returns:
 *  This implementation always returns D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_Lock(IDirect3DExecuteBuffer *iface, D3DEXECUTEBUFFERDESC *desc)
{
    struct d3d_execute_buffer *buffer = impl_from_IDirect3DExecuteBuffer(iface);
    DWORD dwSize;

    TRACE("iface %p, desc %p.\n", iface, desc);

    dwSize = desc->dwSize;
    memcpy(desc, &buffer->desc, dwSize);

    if (TRACE_ON(ddraw))
    {
        TRACE("  Returning description :\n");
        _dump_D3DEXECUTEBUFFERDESC(desc);
    }
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Unlock
 *
 * Unlocks the buffer. We don't have anything to do here
 *
 * Returns:
 *  This implementation always returns D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_Unlock(IDirect3DExecuteBuffer *iface)
{
    TRACE("iface %p.\n", iface);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::SetExecuteData
 *
 * Sets the execute data. This data is used to describe the buffer's content
 *
 * Params:
 *  Data: Pointer to a D3DEXECUTEDATA structure containing the data to
 *  assign
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_OUTOFMEMORY if the vertex buffer allocation failed
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_SetExecuteData(IDirect3DExecuteBuffer *iface, D3DEXECUTEDATA *data)
{
    struct d3d_execute_buffer *buffer = impl_from_IDirect3DExecuteBuffer(iface);
    struct wined3d_map_desc map_desc;
    struct wined3d_box box = {0};
    HRESULT hr;
    DWORD buf_size = buffer->desc.dwBufferSize, copy_size;

    TRACE("iface %p, data %p.\n", iface, data);

    if (data->dwSize != sizeof(*data))
    {
        WARN("data->dwSize is %lu, returning DDERR_INVALIDPARAMS.\n", data->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    /* Skip past previous vertex data. */
    buffer->src_vertex_pos += buffer->data.dwVertexCount;

    if (buffer->vertex_size < data->dwVertexCount)
    {
        unsigned int new_size = max(data->dwVertexCount, buffer->vertex_size * 2);
        struct wined3d_buffer *src_buffer, *dst_buffer;
        struct wined3d_buffer_desc desc;

        desc.byte_width = new_size * sizeof(D3DVERTEX);
        desc.usage = 0;
        desc.bind_flags = WINED3D_BIND_VERTEX_BUFFER;
        desc.access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
        desc.misc_flags = 0;
        desc.structure_byte_stride = 0;

        if (FAILED(hr = wined3d_buffer_create(buffer->d3ddev->wined3d_device, &desc,
                NULL, NULL, &ddraw_null_wined3d_parent_ops, &src_buffer)))
            return hr;

        desc.byte_width = new_size * sizeof(D3DTLVERTEX);
        desc.usage = WINED3DUSAGE_STATICDECL;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_W;

        if (FAILED(hr = wined3d_buffer_create(buffer->d3ddev->wined3d_device, &desc,
                NULL, NULL, &ddraw_null_wined3d_parent_ops, &dst_buffer)))
        {
            wined3d_buffer_decref(src_buffer);
            return hr;
        }

        if (buffer->dst_vertex_buffer)
        {
            wined3d_buffer_decref(buffer->src_vertex_buffer);
            wined3d_buffer_decref(buffer->dst_vertex_buffer);
        }
        buffer->src_vertex_buffer = src_buffer;
        buffer->dst_vertex_buffer = dst_buffer;
        buffer->vertex_size = new_size;
        buffer->src_vertex_pos = 0;
    }
    else if (buffer->vertex_size - data->dwVertexCount < buffer->src_vertex_pos)
    {
        buffer->src_vertex_pos = 0;
    }

    if (data->dwVertexCount && (!buf_size || data->dwVertexOffset < buf_size))
    {
        box.left = buffer->src_vertex_pos * sizeof(D3DVERTEX);
        box.right = box.left + data->dwVertexCount * sizeof(D3DVERTEX);
        if (FAILED(hr = wined3d_resource_map(wined3d_buffer_get_resource(buffer->src_vertex_buffer),
                0, &map_desc, &box, WINED3D_MAP_WRITE)))
            return hr;

        copy_size = data->dwVertexCount * sizeof(D3DVERTEX);
        if (buf_size)
            copy_size = min(copy_size, buf_size - data->dwVertexOffset);

        memcpy(map_desc.data, ((BYTE *)buffer->desc.lpData) + data->dwVertexOffset, copy_size);

        wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->src_vertex_buffer), 0);
    }

    memcpy(&buffer->data, data, data->dwSize);

    if (TRACE_ON(ddraw))
        _dump_executedata(data);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::GetExecuteData
 *
 * Returns the data in the execute buffer
 *
 * Params:
 *  Data: Pointer to a D3DEXECUTEDATA structure used to return data
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_GetExecuteData(IDirect3DExecuteBuffer *iface, D3DEXECUTEDATA *data)
{
    struct d3d_execute_buffer *buffer = impl_from_IDirect3DExecuteBuffer(iface);

    TRACE("iface %p, data %p.\n", iface, data);

    /* Tests show that dwSize is ignored. */
    memcpy(data, &buffer->data, sizeof(*data));

    if (TRACE_ON(ddraw))
    {
        TRACE("Returning data :\n");
        _dump_executedata(data);
    }

    return DD_OK;
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Validate
 *
 * DirectX 5 SDK: "The IDirect3DExecuteBuffer::Validate method is not
 * currently implemented"
 *
 * Params:
 *  ?
 *
 * Returns:
 *  DDERR_UNSUPPORTED, because it's not implemented in Windows.
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_Validate(IDirect3DExecuteBuffer *iface,
        DWORD *offset, LPD3DVALIDATECALLBACK callback, void *context, DWORD reserved)
{
    TRACE("iface %p, offset %p, callback %p, context %p, reserved %#lx.\n",
            iface, offset, callback, context, reserved);

    WARN("Not implemented.\n");

    return DDERR_UNSUPPORTED; /* Unchecked */
}

/*****************************************************************************
 * IDirect3DExecuteBuffer::Optimize
 *
 * DirectX5 SDK: "The IDirect3DExecuteBuffer::Optimize method is not
 * currently supported"
 *
 * Params:
 *  Dummy: Seems to be an unused dummy ;)
 *
 * Returns:
 *  DDERR_UNSUPPORTED, because it's not implemented in Windows.
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_Optimize(IDirect3DExecuteBuffer *iface, DWORD reserved)
{
    TRACE("iface %p, reserved %#lx.\n", iface, reserved);

    WARN("Not implemented.\n");

    return DDERR_UNSUPPORTED; /* Unchecked */
}

static const struct IDirect3DExecuteBufferVtbl d3d_execute_buffer_vtbl =
{
    d3d_execute_buffer_QueryInterface,
    d3d_execute_buffer_AddRef,
    d3d_execute_buffer_Release,
    d3d_execute_buffer_Initialize,
    d3d_execute_buffer_Lock,
    d3d_execute_buffer_Unlock,
    d3d_execute_buffer_SetExecuteData,
    d3d_execute_buffer_GetExecuteData,
    d3d_execute_buffer_Validate,
    d3d_execute_buffer_Optimize,
};

HRESULT d3d_execute_buffer_init(struct d3d_execute_buffer *execute_buffer,
        struct d3d_device *device, D3DEXECUTEBUFFERDESC *desc)
{
    execute_buffer->IDirect3DExecuteBuffer_iface.lpVtbl = &d3d_execute_buffer_vtbl;
    execute_buffer->ref = 1;
    execute_buffer->d3ddev = device;

    /* Initializes memory */
    memcpy(&execute_buffer->desc, desc, desc->dwSize);

    /* No buffer given */
    if (!(execute_buffer->desc.dwFlags & D3DDEB_LPDATA))
        execute_buffer->desc.lpData = NULL;

    /* No buffer size given */
    if (!(execute_buffer->desc.dwFlags & D3DDEB_BUFSIZE))
        execute_buffer->desc.dwBufferSize = 0;

    /* Create buffer if asked */
    if (!execute_buffer->desc.lpData && execute_buffer->desc.dwBufferSize)
    {
        execute_buffer->need_free = TRUE;
        if (!(execute_buffer->desc.lpData = calloc(1, execute_buffer->desc.dwBufferSize)))
        {
            ERR("Failed to allocate execute buffer data.\n");
            return DDERR_OUTOFMEMORY;
        }
    }

    execute_buffer->desc.dwFlags |= D3DDEB_LPDATA;

    return D3D_OK;
}

struct d3d_execute_buffer *unsafe_impl_from_IDirect3DExecuteBuffer(IDirect3DExecuteBuffer *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d_execute_buffer_vtbl);

    return impl_from_IDirect3DExecuteBuffer(iface);
}
