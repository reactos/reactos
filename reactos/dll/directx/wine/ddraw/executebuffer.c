/* Direct3D ExecuteBuffer
 * Copyright (c) 1998-2004 Lionel ULMER
 * Copyright (c) 2002-2004 Christian Costa
 * Copyright (c) 2006      Stefan Dösinger
 *
 * This file contains the implementation of IDirect3DExecuteBuffer.
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

/*****************************************************************************
 * _dump_executedata
 * _dump_D3DEXECUTEBUFFERDESC
 *
 * Debug functions which write the executebuffer data to the console
 *
 *****************************************************************************/

static void _dump_executedata(const D3DEXECUTEDATA *lpData) {
    TRACE("dwSize : %d\n", lpData->dwSize);
    TRACE("Vertex      Offset : %d  Count  : %d\n", lpData->dwVertexOffset, lpData->dwVertexCount);
    TRACE("Instruction Offset : %d  Length : %d\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
    TRACE("HVertex     Offset : %d\n", lpData->dwHVertexOffset);
}

static void _dump_D3DEXECUTEBUFFERDESC(const D3DEXECUTEBUFFERDESC *lpDesc) {
    TRACE("dwSize       : %d\n", lpDesc->dwSize);
    TRACE("dwFlags      : %x\n", lpDesc->dwFlags);
    TRACE("dwCaps       : %x\n", lpDesc->dwCaps);
    TRACE("dwBufferSize : %d\n", lpDesc->dwBufferSize);
    TRACE("lpData       : %p\n", lpDesc->lpData);
}

static void transform_vertex(D3DTLVERTEX *dst, const D3DMATRIX *mat,
        const D3DVIEWPORT *vp, float x, float y, float z)
{
    dst->u1.sx = (x * mat->_11) + (y * mat->_21) + (z * mat->_31) + mat->_41;
    dst->u2.sy = (x * mat->_12) + (y * mat->_22) + (z * mat->_32) + mat->_42;
    dst->u3.sz = (x * mat->_13) + (y * mat->_23) + (z * mat->_33) + mat->_43;
    dst->u4.rhw = (x * mat->_14) + (y * mat->_24) + (z * mat->_34) + mat->_44;

    dst->u1.sx = dst->u1.sx / dst->u4.rhw * vp->dvScaleX + vp->dwX + vp->dwWidth / 2;
    dst->u2.sy = -dst->u2.sy / dst->u4.rhw * vp->dvScaleY + vp->dwY + vp->dwHeight / 2;
    dst->u3.sz /= dst->u4.rhw;
    dst->u4.rhw = 1.0f / dst->u4.rhw;
}

HRESULT d3d_execute_buffer_execute(struct d3d_execute_buffer *buffer,
        struct d3d_device *device, struct d3d_viewport *viewport)
{
    DWORD vs = buffer->data.dwVertexOffset;
    DWORD is = buffer->data.dwInstructionOffset;
    char *instr = (char *)buffer->desc.lpData + is;
    unsigned int i;

    if (viewport->active_device != device)
    {
        WARN("Viewport %p active device is %p.\n",
                viewport, viewport->active_device);
        return DDERR_INVALIDPARAMS;
    }

    /* Activate the viewport */
    viewport_activate(viewport, FALSE);

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
	instr += sizeof(D3DINSTRUCTION);
	
	switch (current->bOpcode) {
	    case D3DOP_POINT: {
	        WARN("POINT-s          (%d)\n", count);
		instr += count * size;
	    } break;

	    case D3DOP_LINE: {
	        WARN("LINE-s           (%d)\n", count);
		instr += count * size;
	    } break;

            case D3DOP_TRIANGLE:
            {
                D3DTLVERTEX *tl_vx = buffer->vertex_data;
		TRACE("TRIANGLE         (%d)\n", count);

                if (buffer->nb_indices < count * 3)
                {
                    buffer->nb_indices = count * 3;
                    HeapFree(GetProcessHeap(), 0, buffer->indices);
                    buffer->indices = HeapAlloc(GetProcessHeap(), 0, sizeof(*buffer->indices) * buffer->nb_indices);
                }

                for (i = 0; i < count; ++i)
                {
                    D3DTRIANGLE *ci = (D3DTRIANGLE *)instr;
		    TRACE("  v1: %d  v2: %d  v3: %d\n",ci->u1.v1, ci->u2.v2, ci->u3.v3);
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
                    buffer->indices[(i * 3)    ] = ci->u1.v1;
                    buffer->indices[(i * 3) + 1] = ci->u2.v2;
                    buffer->indices[(i * 3) + 2] = ci->u3.v3;
                    instr += size;
                }
                if (count)
                    IDirect3DDevice7_DrawIndexedPrimitive(&device->IDirect3DDevice7_iface,
                            D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, tl_vx, buffer->nb_vertices,
                            buffer->indices, count * 3, 0);
	    } break;

	    case D3DOP_MATRIXLOAD:
	        WARN("MATRIXLOAD-s     (%d)\n", count);
	        instr += count * size;
	        break;

            case D3DOP_MATRIXMULTIPLY:
                TRACE("MATRIXMULTIPLY   (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DMATRIXMULTIPLY *ci = (D3DMATRIXMULTIPLY *)instr;
                    D3DMATRIX *a, *b, *c;

                    a = ddraw_get_object(&device->handle_table, ci->hDestMatrix - 1, DDRAW_HANDLE_MATRIX);
                    b = ddraw_get_object(&device->handle_table, ci->hSrcMatrix1 - 1, DDRAW_HANDLE_MATRIX);
                    c = ddraw_get_object(&device->handle_table, ci->hSrcMatrix2 - 1, DDRAW_HANDLE_MATRIX);

                    if (!a || !b || !c)
                    {
                        ERR("Invalid matrix handle (a %#x -> %p, b %#x -> %p, c %#x -> %p).\n",
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

                    m = ddraw_get_object(&device->handle_table, ci->u2.dwArg[0] - 1, DDRAW_HANDLE_MATRIX);
                    if (!m)
                    {
                        ERR("Invalid matrix handle %#x.\n", ci->u2.dwArg[0]);
                    }
                    else
                    {
                        if (ci->u1.dtstTransformStateType == D3DTRANSFORMSTATE_WORLD)
                            device->world = ci->u2.dwArg[0];
                        if (ci->u1.dtstTransformStateType == D3DTRANSFORMSTATE_VIEW)
                            device->view = ci->u2.dwArg[0];
                        if (ci->u1.dtstTransformStateType == D3DTRANSFORMSTATE_PROJECTION)
                            device->proj = ci->u2.dwArg[0];
                        IDirect3DDevice7_SetTransform(&device->IDirect3DDevice7_iface,
                                ci->u1.dtstTransformStateType, m);
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
                            ci->u1.dlstLightStateType, ci->u2.dwArg[0])))
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
                            ci->u1.drstRenderStateType, ci->u2.dwArg[0])))
                        WARN("Failed to set render state.\n");

                    instr += size;
                }
                break;

            case D3DOP_PROCESSVERTICES:
            {
                /* TODO: Share code with d3d_vertex_buffer7_ProcessVertices()
                 * and / or wined3d_device_process_vertices(). */
                D3DMATRIX view_mat, world_mat, proj_mat, mat;

                TRACE("PROCESSVERTICES  (%d)\n", count);

                /* Get the transform and world matrix */
                /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
                wined3d_device_get_transform(device->wined3d_device,
                        D3DTRANSFORMSTATE_VIEW, (struct wined3d_matrix *)&view_mat);
                wined3d_device_get_transform(device->wined3d_device,
                        D3DTRANSFORMSTATE_PROJECTION, (struct wined3d_matrix *)&proj_mat);
                wined3d_device_get_transform(device->wined3d_device,
                        WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)&world_mat);

                if (TRACE_ON(ddraw))
                {
                    TRACE("  Projection Matrix:\n");
                    dump_D3DMATRIX(&proj_mat);
                    TRACE("  View Matrix:\n");
                    dump_D3DMATRIX(&view_mat);
                    TRACE("  World Matrix:\n");
                    dump_D3DMATRIX(&world_mat);
                }

                multiply_matrix(&mat, &view_mat, &world_mat);
                multiply_matrix(&mat, &proj_mat, &mat);

                for (i = 0; i < count; ++i)
                {
                    D3DPROCESSVERTICES *ci = (D3DPROCESSVERTICES *)instr;
                    D3DTLVERTEX *dst = (D3DTLVERTEX *)buffer->vertex_data + ci->wDest;
                    DWORD op = ci->dwFlags & D3DPROCESSVERTICES_OPMASK;

                    TRACE("  start %u, dest %u, count %u, flags %#x.\n",
                            ci->wStart, ci->wDest, ci->dwCount, ci->dwFlags);

                    if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
                        FIXME("D3DPROCESSVERTICES_UPDATEEXTENTS not implemented.\n");
                    if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
                        FIXME("D3DPROCESSVERTICES_NOCOLOR not implemented.\n");

                    switch (op)
                    {
                        case D3DPROCESSVERTICES_TRANSFORMLIGHT:
                        {
                            const D3DVERTEX *src = (D3DVERTEX *)((char *)buffer->desc.lpData + vs) + ci->wStart;
                            unsigned int vtx_idx;
                            static unsigned int once;

                            if (!once++)
                                FIXME("Lighting not implemented.\n");

                            for (vtx_idx = 0; vtx_idx < ci->dwCount; ++vtx_idx)
                            {
                                transform_vertex(&dst[vtx_idx], &mat, &viewport->viewports.vp1,
                                        src[vtx_idx].u1.x, src[vtx_idx].u2.y, src[vtx_idx].u3.z);
                                /* No lighting yet */
                                dst[vtx_idx].u5.color = 0xffffffff; /* Opaque white */
                                dst[vtx_idx].u6.specular = 0xff000000; /* No specular and no fog factor */
                                dst[vtx_idx].u7.tu = src[vtx_idx].u7.tu;
                                dst[vtx_idx].u8.tv = src[vtx_idx].u8.tv;
                            }
                            break;
                        }

                        case D3DPROCESSVERTICES_TRANSFORM:
                        {
                            const D3DLVERTEX *src = (D3DLVERTEX *)((char *)buffer->desc.lpData + vs) + ci->wStart;
                            unsigned int vtx_idx;

                            for (vtx_idx = 0; vtx_idx < ci->dwCount; ++vtx_idx)
                            {
                                transform_vertex(&dst[vtx_idx], &mat, &viewport->viewports.vp1,
                                        src[vtx_idx].u1.x, src[vtx_idx].u2.y, src[vtx_idx].u3.z);
                                dst[vtx_idx].u5.color = src[vtx_idx].u4.color;
                                dst[vtx_idx].u6.specular = src[vtx_idx].u5.specular;
                                dst[vtx_idx].u7.tu = src[vtx_idx].u6.tu;
                                dst[vtx_idx].u8.tv = src[vtx_idx].u7.tv;
                            }
                            break;
                        }

                        case D3DPROCESSVERTICES_COPY:
                        {
                            const D3DTLVERTEX *src = (D3DTLVERTEX *)((char *)buffer->desc.lpData + vs) + ci->wStart;

                            memcpy(dst, src, ci->dwCount * sizeof(*dst));
                            break;
                        }

                        default:
                            FIXME("Unhandled vertex processing op %#x.\n", op);
                            break;
                    }

                    instr += size;
                }
                break;
            }

	    case D3DOP_TEXTURELOAD: {
	        WARN("TEXTURELOAD-s    (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_EXIT: {
	        TRACE("EXIT             (%d)\n", count);
		/* We did this instruction */
		instr += size;
		/* Exit this loop */
		goto end_of_buffer;
	    } break;

            case D3DOP_BRANCHFORWARD:
                TRACE("BRANCHFORWARD    (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    D3DBRANCH *ci = (D3DBRANCH *)instr;

                    if ((buffer->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue)
                    {
                        if (!ci->bNegate)
                        {
                            TRACE(" Branch to %d\n", ci->dwOffset);
                            if (ci->dwOffset) {
                                instr = (char*)current + ci->dwOffset;
                                break;
                            }
			}
		    } else {
		        if (ci->bNegate) {
                            TRACE(" Branch to %d\n", ci->dwOffset);
                            if (ci->dwOffset) {
                                instr = (char*)current + ci->dwOffset;
                                break;
                            }
			}
		    }

		    instr += size;
                }
                break;

	    case D3DOP_SPAN: {
	        WARN("SPAN-s           (%d)\n", count);

		instr += count * size;
	    } break;

            case D3DOP_SETSTATUS:
                TRACE("SETSTATUS        (%d)\n", count);
                for (i = 0; i < count; ++i)
                {
                    buffer->data.dsStatus = *(D3DSTATUS *)instr;
                    instr += size;
                }
                break;

	    default:
	        ERR("Unhandled OpCode %d !!!\n",current->bOpcode);
	        /* Try to save ... */
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

    TRACE("%p increasing refcount to %u.\n", buffer, ref);

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

    TRACE("%p decreasing refcount to %u.\n", buffer, ref);

    if (!ref)
    {
        if (buffer->need_free)
            HeapFree(GetProcessHeap(), 0, buffer->desc.lpData);
        HeapFree(GetProcessHeap(), 0, buffer->vertex_data);
        HeapFree(GetProcessHeap(), 0, buffer->indices);
        HeapFree(GetProcessHeap(), 0, buffer);
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
    DWORD nbvert;

    TRACE("iface %p, data %p.\n", iface, data);

    memcpy(&buffer->data, data, data->dwSize);

    /* Get the number of vertices in the execute buffer */
    nbvert = buffer->data.dwVertexCount;

    /* Prepares the transformed vertex buffer */
    HeapFree(GetProcessHeap(), 0, buffer->vertex_data);
    buffer->vertex_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbvert * sizeof(D3DTLVERTEX));
    buffer->nb_vertices = nbvert;

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
    DWORD dwSize;

    TRACE("iface %p, data %p.\n", iface, data);

    dwSize = data->dwSize;
    memcpy(data, &buffer->data, dwSize);

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
    TRACE("iface %p, offset %p, callback %p, context %p, reserved %#x.\n",
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
    TRACE("iface %p, reserved %#x.\n", iface, reserved);

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
        execute_buffer->desc.lpData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, execute_buffer->desc.dwBufferSize);
        if (!execute_buffer->desc.lpData)
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
