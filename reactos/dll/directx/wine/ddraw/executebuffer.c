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

#include <config.h>
//#include "wine/port.h"

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

HRESULT d3d_execute_buffer_execute(struct d3d_execute_buffer *buffer,
        struct d3d_device *device, struct d3d_viewport *viewport)
{
    DWORD vs = buffer->data.dwVertexOffset;
    DWORD is = buffer->data.dwInstructionOffset;
    char *instr = (char *)buffer->desc.lpData + is;

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

	    case D3DOP_TRIANGLE: {
                DWORD i;
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
                IDirect3DDevice7_DrawIndexedPrimitive(&device->IDirect3DDevice7_iface,
                        D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, tl_vx, buffer->nb_vertices,
                        buffer->indices, count * 3, 0);
	    } break;

	    case D3DOP_MATRIXLOAD:
	        WARN("MATRIXLOAD-s     (%d)\n", count);
	        instr += count * size;
	        break;

	    case D3DOP_MATRIXMULTIPLY: {
                DWORD  i;
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
	    } break;

	    case D3DOP_STATETRANSFORM: {
                DWORD i;
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
	    } break;

	    case D3DOP_STATELIGHT: {
                DWORD i;
		TRACE("STATELIGHT       (%d)\n", count);

                for (i = 0; i < count; ++i)
                {
                    D3DSTATE *ci = (D3DSTATE *)instr;

		    TRACE("(%08x,%08x)\n", ci->u1.dlstLightStateType, ci->u2.dwArg[0]);

		    if (!ci->u1.dlstLightStateType || (ci->u1.dlstLightStateType > D3DLIGHTSTATE_COLORVERTEX))
			ERR("Unexpected Light State Type %d\n", ci->u1.dlstLightStateType);
                    else if (ci->u1.dlstLightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */)
                    {
                        struct d3d_material *m;

                        m = ddraw_get_object(&device->handle_table, ci->u2.dwArg[0] - 1, DDRAW_HANDLE_MATERIAL);
                        if (!m)
                            ERR("Invalid material handle %#x.\n", ci->u2.dwArg[0]);
                        else
                            material_activate(m);
                    }
                    else if (ci->u1.dlstLightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */)
                    {
			switch (ci->u2.dwArg[0]) {
			    case D3DCOLOR_MONO:
				ERR("DDCOLOR_MONO should not happen!\n");
				break;
			    case D3DCOLOR_RGB:
				/* We are already in this mode */
				break;
			    default:
				ERR("Unknown color model!\n");
			}
		    } else {
			D3DRENDERSTATETYPE rs = 0;
			switch (ci->u1.dlstLightStateType) {

			    case D3DLIGHTSTATE_AMBIENT:       /* 2 */
				rs = D3DRENDERSTATE_AMBIENT;
				break;
			    case D3DLIGHTSTATE_FOGMODE:       /* 4 */
				rs = D3DRENDERSTATE_FOGVERTEXMODE;
				break;
			    case D3DLIGHTSTATE_FOGSTART:      /* 5 */
				rs = D3DRENDERSTATE_FOGSTART;
				break;
			    case D3DLIGHTSTATE_FOGEND:        /* 6 */
				rs = D3DRENDERSTATE_FOGEND;
				break;
			    case D3DLIGHTSTATE_FOGDENSITY:    /* 7 */
				rs = D3DRENDERSTATE_FOGDENSITY;
				break;
			    case D3DLIGHTSTATE_COLORVERTEX:   /* 8 */
				rs = D3DRENDERSTATE_COLORVERTEX;
				break;
			    default:
				break;
			}

                        IDirect3DDevice7_SetRenderState(&device->IDirect3DDevice7_iface, rs, ci->u2.dwArg[0]);
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_STATERENDER: {
                DWORD i;
                IDirect3DDevice2 *d3d_device2 = &device->IDirect3DDevice2_iface;
		TRACE("STATERENDER      (%d)\n", count);

                for (i = 0; i < count; ++i)
                {
                    D3DSTATE *ci = (D3DSTATE *)instr;

                    IDirect3DDevice2_SetRenderState(d3d_device2, ci->u1.drstRenderStateType, ci->u2.dwArg[0]);

		    instr += size;
		}
	    } break;

            case D3DOP_PROCESSVERTICES:
            {
                /* TODO: Share code with IDirect3DVertexBuffer::ProcessVertices and / or
                 * IWineD3DDevice::ProcessVertices
                 */
                DWORD i;
                D3DMATRIX view_mat, world_mat, proj_mat;
                TRACE("PROCESSVERTICES  (%d)\n", count);

                /* Get the transform and world matrix */
                /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
                wined3d_device_get_transform(device->wined3d_device,
                        D3DTRANSFORMSTATE_VIEW, (struct wined3d_matrix *)&view_mat);
                wined3d_device_get_transform(device->wined3d_device,
                        D3DTRANSFORMSTATE_PROJECTION, (struct wined3d_matrix *)&proj_mat);
                wined3d_device_get_transform(device->wined3d_device,
                        WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)&world_mat);

                for (i = 0; i < count; ++i)
                {
                    D3DPROCESSVERTICES *ci = (D3DPROCESSVERTICES *)instr;

                    TRACE("  Start : %d Dest : %d Count : %d\n",
			  ci->wStart, ci->wDest, ci->dwCount);
		    TRACE("  Flags : ");
                    if (TRACE_ON(ddraw))
                    {
		        if (ci->dwFlags & D3DPROCESSVERTICES_COPY)
			    TRACE("COPY ");
			if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
			    TRACE("NOCOLOR ");
			if (ci->dwFlags == D3DPROCESSVERTICES_OPMASK)
			    TRACE("OPMASK ");
			if (ci->dwFlags & D3DPROCESSVERTICES_TRANSFORM)
			    TRACE("TRANSFORM ");
			if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT)
			    TRACE("TRANSFORMLIGHT ");
			if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
			    TRACE("UPDATEEXTENTS ");
			TRACE("\n");
		    }

		    /* This is where doing Direct3D on top on OpenGL is quite difficult.
		       This method transforms a set of vertices using the CURRENT state
		       (lighting, projection, ...) but does not rasterize them.
		       They will only be put on screen later (with the POINT / LINE and
		       TRIANGLE op-codes). The problem is that you can have a triangle
		       with each point having been transformed using another state...

		       In this implementation, I will emulate only ONE thing : each
		       vertex can have its own "WORLD" transformation (this is used in the
		       TWIST.EXE demo of the 5.2 SDK). I suppose that all vertices of the
		       execute buffer use the same state.

		       If I find applications that change other states, I will try to do a
		       more 'fine-tuned' state emulation (but I may become quite tricky if
		       it changes a light position in the middle of a triangle).

		       In this case, a 'direct' approach (i.e. without using OpenGL, but
		       writing our own 3D rasterizer) would be easier. */

		    /* The current method (with the hypothesis that only the WORLD matrix
		       will change between two points) is like this :
		       - I transform 'manually' all the vertices with the current WORLD
		         matrix and store them in the vertex buffer
		       - during the rasterization phase, the WORLD matrix will be set to
		         the Identity matrix */

		    /* Enough for the moment */
		    if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT) {
		        unsigned int nb;
                        D3DVERTEX *src = ((D3DVERTEX *)((char *)buffer->desc.lpData + vs)) + ci->wStart;
                        D3DTLVERTEX *dst = ((D3DTLVERTEX *)buffer->vertex_data) + ci->wDest;
                        D3DVIEWPORT *Viewport = &viewport->viewports.vp1;
			D3DMATRIX mat;
			
                        if (TRACE_ON(ddraw))
                        {
			    TRACE("  Projection Matrix : (%p)\n", &proj_mat);
			    dump_D3DMATRIX(&proj_mat);
			    TRACE("  View       Matrix : (%p)\n", &view_mat);
			    dump_D3DMATRIX(&view_mat);
			    TRACE("  World Matrix : (%p)\n", &world_mat);
			    dump_D3DMATRIX(&world_mat);
			}

                        multiply_matrix(&mat,&view_mat,&world_mat);
                        multiply_matrix(&mat,&proj_mat,&mat);

			for (nb = 0; nb < ci->dwCount; nb++) {
			    /* No lighting yet */
			    dst->u5.color = 0xFFFFFFFF; /* Opaque white */
			    dst->u6.specular = 0xFF000000; /* No specular and no fog factor */

			    dst->u7.tu  = src->u7.tu;
			    dst->u8.tv  = src->u8.tv;

			    /* Now, the matrix multiplication */
			    dst->u1.sx = (src->u1.x * mat._11) + (src->u2.y * mat._21) + (src->u3.z * mat._31) + (1.0 * mat._41);
			    dst->u2.sy = (src->u1.x * mat._12) + (src->u2.y * mat._22) + (src->u3.z * mat._32) + (1.0 * mat._42);
			    dst->u3.sz = (src->u1.x * mat._13) + (src->u2.y * mat._23) + (src->u3.z * mat._33) + (1.0 * mat._43);
			    dst->u4.rhw = (src->u1.x * mat._14) + (src->u2.y * mat._24) + (src->u3.z * mat._34) + (1.0 * mat._44);

			    dst->u1.sx = dst->u1.sx / dst->u4.rhw * Viewport->dvScaleX
				       + Viewport->dwX + Viewport->dwWidth / 2;
			    dst->u2.sy = (-dst->u2.sy) / dst->u4.rhw * Viewport->dvScaleY
				       + Viewport->dwY + Viewport->dwHeight / 2;
			    dst->u3.sz /= dst->u4.rhw;
			    dst->u4.rhw = 1 / dst->u4.rhw;

			    src++;
			    dst++;

			}
		    } else if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORM) {
		        unsigned int nb;
                        D3DLVERTEX *src = ((D3DLVERTEX *)((char *)buffer->desc.lpData + vs)) + ci->wStart;
                        D3DTLVERTEX *dst = ((D3DTLVERTEX *)buffer->vertex_data) + ci->wDest;
                        D3DVIEWPORT *Viewport = &viewport->viewports.vp1;
			D3DMATRIX mat;
			
                        if (TRACE_ON(ddraw))
                        {
			    TRACE("  Projection Matrix : (%p)\n", &proj_mat);
			    dump_D3DMATRIX(&proj_mat);
			    TRACE("  View       Matrix : (%p)\n",&view_mat);
			    dump_D3DMATRIX(&view_mat);
			    TRACE("  World Matrix : (%p)\n", &world_mat);
			    dump_D3DMATRIX(&world_mat);
			}

			multiply_matrix(&mat,&view_mat,&world_mat);
			multiply_matrix(&mat,&proj_mat,&mat);

			for (nb = 0; nb < ci->dwCount; nb++) {
			    dst->u5.color = src->u4.color;
			    dst->u6.specular = src->u5.specular;
			    dst->u7.tu = src->u6.tu;
			    dst->u8.tv = src->u7.tv;

			    /* Now, the matrix multiplication */
			    dst->u1.sx = (src->u1.x * mat._11) + (src->u2.y * mat._21) + (src->u3.z * mat._31) + (1.0 * mat._41);
			    dst->u2.sy = (src->u1.x * mat._12) + (src->u2.y * mat._22) + (src->u3.z * mat._32) + (1.0 * mat._42);
			    dst->u3.sz = (src->u1.x * mat._13) + (src->u2.y * mat._23) + (src->u3.z * mat._33) + (1.0 * mat._43);
			    dst->u4.rhw = (src->u1.x * mat._14) + (src->u2.y * mat._24) + (src->u3.z * mat._34) + (1.0 * mat._44);

			    dst->u1.sx = dst->u1.sx / dst->u4.rhw * Viewport->dvScaleX
				       + Viewport->dwX + Viewport->dwWidth / 2;
			    dst->u2.sy = (-dst->u2.sy) / dst->u4.rhw * Viewport->dvScaleY
				       + Viewport->dwY + Viewport->dwHeight / 2;

			    dst->u3.sz /= dst->u4.rhw;
			    dst->u4.rhw = 1 / dst->u4.rhw;

			    src++;
			    dst++;
			}
                    }
                    else if (ci->dwFlags == D3DPROCESSVERTICES_COPY)
                    {
                        D3DTLVERTEX *src = ((D3DTLVERTEX *)((char *)buffer->desc.lpData + vs)) + ci->wStart;
                        D3DTLVERTEX *dst = ((D3DTLVERTEX *)buffer->vertex_data) + ci->wDest;

			memcpy(dst, src, ci->dwCount * sizeof(D3DTLVERTEX));
		    } else {
		        ERR("Unhandled vertex processing flag %#x.\n", ci->dwFlags);
		    }

		    instr += size;
		}
	    } break;

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

	    case D3DOP_BRANCHFORWARD: {
                DWORD i;
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
	    } break;

	    case D3DOP_SPAN: {
	        WARN("SPAN-s           (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_SETSTATUS: {
                DWORD i;
		TRACE("SETSTATUS        (%d)\n", count);

                for (i = 0; i < count; ++i)
                {
                    buffer->data.dsStatus = *(D3DSTATUS *)instr;
                    instr += size;
                }
	    } break;

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

/*****************************************************************************
 * IDirect3DExecuteBuffer::QueryInterface
 *
 * Well, a usual QueryInterface function. Don't know fur sure which
 * interfaces it can Query.
 *
 * Params:
 *  riid: The interface ID queried for
 *  obj: Address to return the interface pointer at
 *
 * Returns:
 *  D3D_OK in case of a success (S_OK? Think it's the same)
 *  OLE_E_ENUM_NOMORE if the interface wasn't found.
 *   (E_NOINTERFACE?? Don't know what I really need)
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_execute_buffer_QueryInterface(IDirect3DExecuteBuffer *iface, REFIID riid, void **obj)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DExecuteBuffer_AddRef(iface);
	*obj = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obj);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DExecuteBuffer, riid ) ) {
        IDirect3DExecuteBuffer_AddRef(iface);
        *obj = iface;
	TRACE("  Creating IDirect3DExecuteBuffer interface %p\n", *obj);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", iface, debugstr_guid(riid));
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
