/*
 * IWineD3DDevice implementation
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2007 Stefan Dösinger for CodeWeavers
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2007 Andrew Riedi
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->adapter->gl_info

/* Define the default light parameters as specified by MSDN */
const WINED3DLIGHT WINED3D_default_light = {

    WINED3DLIGHT_DIRECTIONAL, /* Type */
    { 1.0, 1.0, 1.0, 0.0 },   /* Diffuse r,g,b,a */
    { 0.0, 0.0, 0.0, 0.0 },   /* Specular r,g,b,a */
    { 0.0, 0.0, 0.0, 0.0 },   /* Ambient r,g,b,a, */
    { 0.0, 0.0, 0.0 },        /* Position x,y,z */
    { 0.0, 0.0, 1.0 },        /* Direction x,y,z */
    0.0,                      /* Range */
    0.0,                      /* Falloff */
    0.0, 0.0, 0.0,            /* Attenuation 0,1,2 */
    0.0,                      /* Theta */
    0.0                       /* Phi */
};

/* static function declarations */
static void WINAPI IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource);

/* helper macros */
#define D3DMEMCHECK(object, ppResult) if(NULL == object) { *ppResult = NULL; WARN("Out of memory\n"); return WINED3DERR_OUTOFVIDEOMEMORY;}

#define D3DCREATEOBJECTINSTANCE(object, type) { \
    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3D##type##Impl)); \
    D3DMEMCHECK(object, pp##type); \
    object->lpVtbl = &IWineD3D##type##_Vtbl;  \
    object->wineD3DDevice = This; \
    object->parent       = parent; \
    object->ref          = 1; \
    *pp##type = (IWineD3D##type *) object; \
}

#define D3DCREATESHADEROBJECTINSTANCE(object, type) { \
    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3D##type##Impl)); \
    D3DMEMCHECK(object, pp##type); \
    object->lpVtbl = &IWineD3D##type##_Vtbl;  \
    object->parent          = parent; \
    object->baseShader.ref  = 1; \
    object->baseShader.device = (IWineD3DDevice*) This; \
    list_init(&object->baseShader.linked_programs); \
    *pp##type = (IWineD3D##type *) object; \
}

#define  D3DCREATERESOURCEOBJECTINSTANCE(object, type, d3dtype, _size){ \
    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3D##type##Impl)); \
    D3DMEMCHECK(object, pp##type); \
    object->lpVtbl = &IWineD3D##type##_Vtbl;  \
    object->resource.wineD3DDevice   = This; \
    object->resource.parent          = parent; \
    object->resource.resourceType    = d3dtype; \
    object->resource.ref             = 1; \
    object->resource.pool            = Pool; \
    object->resource.format          = Format; \
    object->resource.usage           = Usage; \
    object->resource.size            = _size; \
    list_init(&object->resource.privateData); \
    /* Check that we have enough video ram left */ \
    if (Pool == WINED3DPOOL_DEFAULT) { \
        if (IWineD3DDevice_GetAvailableTextureMem(iface) <= _size) { \
            WARN("Out of 'bogus' video memory\n"); \
            HeapFree(GetProcessHeap(), 0, object); \
            *pp##type = NULL; \
            return WINED3DERR_OUTOFVIDEOMEMORY; \
        } \
        WineD3DAdapterChangeGLRam(This, _size); \
    } \
    object->resource.heapMemory = (0 == _size ? NULL : HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _size + RESOURCE_ALIGNMENT)); \
    if (object->resource.heapMemory == NULL && _size != 0) { \
        FIXME("Out of memory!\n"); \
        HeapFree(GetProcessHeap(), 0, object); \
        *pp##type = NULL; \
        return WINED3DERR_OUTOFVIDEOMEMORY; \
    } \
    object->resource.allocatedMemory = (BYTE *)(((ULONG_PTR) object->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1)); \
    *pp##type = (IWineD3D##type *) object; \
    IWineD3DDeviceImpl_AddResource(iface, (IWineD3DResource *)object) ;\
    TRACE("(%p) : Created resource %p\n", This, object); \
}

#define D3DINITIALIZEBASETEXTURE(_basetexture) { \
    _basetexture.levels     = Levels; \
    _basetexture.filterType = (Usage & WINED3DUSAGE_AUTOGENMIPMAP) ? WINED3DTEXF_LINEAR : WINED3DTEXF_NONE; \
    _basetexture.LOD        = 0; \
    _basetexture.dirty      = TRUE; \
    _basetexture.is_srgb = FALSE; \
    _basetexture.srgb_mode_change_count = 0; \
}

/**********************************************************
 * Global variable / Constants follow
 **********************************************************/
const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  /* When needed for comparisons */

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

static HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DDevice)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DDeviceImpl_AddRef(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG WINAPI IWineD3DDeviceImpl_Release(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %d\n", This, refCount + 1);

    if (!refCount) {
        if (This->fbo) {
            GL_EXTCALL(glDeleteFramebuffersEXT(1, &This->fbo));
        }
        if (This->src_fbo) {
            GL_EXTCALL(glDeleteFramebuffersEXT(1, &This->src_fbo));
        }
        if (This->dst_fbo) {
            GL_EXTCALL(glDeleteFramebuffersEXT(1, &This->dst_fbo));
        }

        if (This->glsl_program_lookup) hash_table_destroy(This->glsl_program_lookup);

        /* TODO: Clean up all the surfaces and textures! */
        /* NOTE: You must release the parent if the object was created via a callback
        ** ***************************/

        if (!list_empty(&This->resources)) {
            FIXME("(%p) Device released with resources still bound, acceptable but unexpected\n", This);
            dumpResources(&This->resources);
        }

        if(This->contexts) ERR("Context array not freed!\n");
        if (This->hardwareCursor) DestroyCursor(This->hardwareCursor);
        This->haveHardwareCursor = FALSE;

        IWineD3D_Release(This->wineD3D);
        This->wineD3D = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("Freed device  %p\n", This);
        This = NULL;
    }
    return refCount;
}

/**********************************************************
 * IWineD3DDevice implementation follows
 **********************************************************/
static HRESULT WINAPI IWineD3DDeviceImpl_GetParent(IWineD3DDevice *iface, IUnknown **pParent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *pParent = This->parent;
    IUnknown_AddRef(This->parent);
    return WINED3D_OK;
}

static void CreateVBO(IWineD3DVertexBufferImpl *object) {
    IWineD3DDeviceImpl *This = object->resource.wineD3DDevice;  /* Needed for GL_EXTCALL */
    GLenum error, glUsage;
    DWORD vboUsage = object->resource.usage;
    if(object->Flags & VBFLAG_VBOCREATEFAIL) {
        WARN("Creating a vbo failed once, not trying again\n");
        return;
    }

    TRACE("Creating an OpenGL vertex buffer object for IWineD3DVertexBuffer %p  Usage(%s)\n", object, debug_d3dusage(vboUsage));

    /* Make sure that a context is there. Needed in a multithreaded environment. Otherwise this call is a nop */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    ENTER_GL();

    /* Make sure that the gl error is cleared. Do not use checkGLcall
      * here because checkGLcall just prints a fixme and continues. However,
      * if an error during VBO creation occurs we can fall back to non-vbo operation
      * with full functionality(but performance loss)
      */
    while(glGetError() != GL_NO_ERROR);

    /* Basically the FVF parameter passed to CreateVertexBuffer is no good
      * It is the FVF set with IWineD3DDevice::SetFVF or the Vertex Declaration set with
      * IWineD3DDevice::SetVertexDeclaration that decides how the vertices in the buffer
      * look like. This means that on each DrawPrimitive call the vertex buffer has to be verified
      * to check if the rhw and color values are in the correct format.
      */

    GL_EXTCALL(glGenBuffersARB(1, &object->vbo));
    error = glGetError();
    if(object->vbo == 0 || error != GL_NO_ERROR) {
        WARN("Failed to create a VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }

    GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, object->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        WARN("Failed to bind the VBO with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }

    /* Don't use static, because dx apps tend to update the buffer
     * quite often even if they specify 0 usage. Because we always keep the local copy
     * we never read from the vbo and can create a write only opengl buffer.
     */
    switch(vboUsage & (WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC) ) {
        case WINED3DUSAGE_WRITEONLY | WINED3DUSAGE_DYNAMIC:
        case WINED3DUSAGE_DYNAMIC:
            TRACE("Gl usage = GL_STREAM_DRAW\n");
            glUsage = GL_STREAM_DRAW_ARB;
            break;
        case WINED3DUSAGE_WRITEONLY:
        default:
            TRACE("Gl usage = GL_DYNAMIC_DRAW\n");
            glUsage = GL_DYNAMIC_DRAW_ARB;
            break;
    }

    /* Reserve memory for the buffer. The amount of data won't change
     * so we are safe with calling glBufferData once with a NULL ptr and
     * calling glBufferSubData on updates
     */
    GL_EXTCALL(glBufferDataARB(GL_ARRAY_BUFFER_ARB, object->resource.size, NULL, glUsage));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        WARN("glBufferDataARB failed with error %s (%#x)\n", debug_glerror(error), error);
        goto error;
    }

    LEAVE_GL();

    return;
    error:
    /* Clean up all vbo init, but continue because we can work without a vbo :-) */
    FIXME("Failed to create a vertex buffer object. Continuing, but performance issues can occur\n");
    if(object->vbo) GL_EXTCALL(glDeleteBuffersARB(1, &object->vbo));
    object->vbo = 0;
    object->Flags |= VBFLAG_VBOCREATEFAIL;
    LEAVE_GL();
    return;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexBuffer(IWineD3DDevice *iface, UINT Size, DWORD Usage, 
                             DWORD FVF, WINED3DPOOL Pool, IWineD3DVertexBuffer** ppVertexBuffer, HANDLE *sharedHandle,
                             IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBufferImpl *object;
    WINED3DFORMAT Format = WINED3DFMT_VERTEXDATA; /* Dummy format for now */
    int dxVersion = ( (IWineD3DImpl *) This->wineD3D)->dxVersion;
    BOOL conv;

    if(Size == 0) {
        WARN("Size 0 requested, returning WINED3DERR_INVALIDCALL\n");
        *ppVertexBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    } else if(Pool == WINED3DPOOL_SCRATCH) {
        /* The d3d9 testsuit shows that this is not allowed. It doesn't make much sense
         * anyway, SCRATCH vertex buffers aren't useable anywhere
         */
        WARN("Vertex buffer in D3DPOOL_SCRATCH requested, returning WINED3DERR_INVALIDCALL\n");
        *ppVertexBuffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, VertexBuffer, WINED3DRTYPE_VERTEXBUFFER, Size)

    TRACE("(%p) : Size=%d, Usage=%d, FVF=%x, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->resource.allocatedMemory, object);
    *ppVertexBuffer = (IWineD3DVertexBuffer *)object;

    object->fvf = FVF;

    /* Observations show that drawStridedSlow is faster on dynamic VBs than converting +
     * drawStridedFast (half-life 2).
     *
     * Basically converting the vertices in the buffer is quite expensive, and observations
     * show that drawStridedSlow is faster than converting + uploading + drawStridedFast.
     * Therefore do not create a VBO for WINED3DUSAGE_DYNAMIC buffers.
     *
     * Direct3D7 has another problem: Its vertexbuffer api doesn't offer a way to specify
     * the range of vertices being locked, so each lock will require the whole buffer to be transformed.
     * Moreover geometry data in dx7 is quite simple, so drawStridedSlow isn't a big hit. A plus
     * is that the vertex buffers fvf can be trusted in dx7. So only create non-converted vbos for
     * dx7 apps.
     * There is a IDirect3DVertexBuffer7::Optimize call after which the buffer can't be locked any
     * more. In this call we can convert dx7 buffers too.
     */
    conv = ((FVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) || (FVF & (WINED3DFVF_DIFFUSE | WINED3DFVF_SPECULAR));
    if( GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT) && Pool != WINED3DPOOL_SYSTEMMEM && !(Usage & WINED3DUSAGE_DYNAMIC) && 
        (dxVersion > 7 || !conv) ) {
        CreateVBO(object);
    }
    return WINED3D_OK;
}

static void CreateIndexBufferVBO(IWineD3DDeviceImpl *This, IWineD3DIndexBufferImpl *object) {
    GLenum error, glUsage;
    TRACE("Creating VBO for Index Buffer %p\n", object);

    /* The following code will modify the ELEMENT_ARRAY_BUFFER binding, make sure it is
     * restored on the next draw
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);

    /* Make sure that a context is there. Needed in a multithreaded environment. Otherwise this call is a nop */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    ENTER_GL();

    while(glGetError());

    GL_EXTCALL(glGenBuffersARB(1, &object->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR || object->vbo == 0) {
        ERR("Creating a vbo failed with error %s (%#x), continuing without vbo for this buffer\n", debug_glerror(error), error);
        goto out;
    }

    GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, object->vbo));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        ERR("Failed to bind index buffer with error %s (%#x), continuing without vbo for this buffer\n", debug_glerror(error), error);
        goto out;
    }

    /* Use static write only usage for now. Dynamic index buffers stay in sysmem, and due to the sysmem
        * copy no readback will be needed
        */
    glUsage = GL_STATIC_DRAW_ARB;
    GL_EXTCALL(glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, object->resource.size, NULL, glUsage));
    error = glGetError();
    if(error != GL_NO_ERROR) {
        ERR("Failed to initialize the index buffer with error %s (%#x)\n", debug_glerror(error), error);
        goto out;
    }
    LEAVE_GL();
    TRACE("Successfully created vbo %d for index buffer %p\n", object->vbo, object);
    return;

out:
    GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0));
    GL_EXTCALL(glDeleteBuffersARB(1, &object->vbo));
    LEAVE_GL();
    object->vbo = 0;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateIndexBuffer(IWineD3DDevice *iface, UINT Length, DWORD Usage, 
                                                    WINED3DFORMAT Format, WINED3DPOOL Pool, IWineD3DIndexBuffer** ppIndexBuffer,
                                                    HANDLE *sharedHandle, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DIndexBufferImpl *object;
    TRACE("(%p) Creating index buffer\n", This);

    /* Allocate the storage for the device */
    D3DCREATERESOURCEOBJECTINSTANCE(object,IndexBuffer,WINED3DRTYPE_INDEXBUFFER, Length)

    if(Pool != WINED3DPOOL_SYSTEMMEM && !(Usage & WINED3DUSAGE_DYNAMIC) && GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT)) {
        CreateIndexBufferVBO(This, object);
    }

    TRACE("(%p) : Len=%d, Use=%x, Format=(%u,%s), Pool=%d - Memory@%p, Iface@%p\n", This, Length, Usage, Format, 
                           debug_d3dformat(Format), Pool, object, object->resource.allocatedMemory);
    *ppIndexBuffer = (IWineD3DIndexBuffer *) object;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice* iface, WINED3DSTATEBLOCKTYPE Type, IWineD3DStateBlock** ppStateBlock, IUnknown *parent) {

    IWineD3DDeviceImpl     *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    int i, j;
    HRESULT temp_result;

    D3DCREATEOBJECTINSTANCE(object, StateBlock)
    object->blockType     = Type;

    for(i = 0; i < LIGHTMAP_SIZE; i++) {
        list_init(&object->lightMap[i]);
    }

    /* Special case - Used during initialization to produce a placeholder stateblock
          so other functions called can update a state block                         */
    if (Type == WINED3DSBT_INIT) {
        /* Don't bother increasing the reference count otherwise a device will never
           be freed due to circular dependencies                                   */
        return WINED3D_OK;
    }
    
    temp_result = allocate_shader_constants(object);
    if (WINED3D_OK != temp_result)
        return temp_result;

    /* Otherwise, might as well set the whole state block to the appropriate values  */
    if (This->stateBlock != NULL)
        stateblock_copy((IWineD3DStateBlock*) object, (IWineD3DStateBlock*) This->stateBlock);
    else
        memset(object->streamFreq, 1, sizeof(object->streamFreq));

    /* Reset the ref and type after kludging it */
    object->wineD3DDevice = This;
    object->ref           = 1;
    object->blockType     = Type;

    TRACE("Updating changed flags appropriate for type %d\n", Type);

    if (Type == WINED3DSBT_ALL) {

        TRACE("ALL => Pretend everything has changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, TRUE);

        /* Lights are not part of the changed / set structure */
        for(j = 0; j < LIGHTMAP_SIZE; j++) {
            struct list *e;
            LIST_FOR_EACH(e, &object->lightMap[j]) {
                PLIGHTINFOEL *light = LIST_ENTRY(e, PLIGHTINFOEL, entry);
                light->changed = TRUE;
                light->enabledChanged = TRUE;
            }
        }
        for(j = 1; j <= WINEHIGHEST_RENDER_STATE; j++) {
            object->contained_render_states[j - 1] = j;
        }
        object->num_contained_render_states = WINEHIGHEST_RENDER_STATE;
        /* TODO: Filter unused transforms between TEXTURE8 and WORLD0? */
        for(j = 1; j <= HIGHEST_TRANSFORMSTATE; j++) {
            object->contained_transform_states[j - 1] = j;
        }
        object->num_contained_transform_states = HIGHEST_TRANSFORMSTATE;
        for(j = 0; j < GL_LIMITS(vshader_constantsF); j++) {
            object->contained_vs_consts_f[j] = j;
        }
        object->num_contained_vs_consts_f = GL_LIMITS(vshader_constantsF);
        for(j = 0; j < MAX_CONST_I; j++) {
            object->contained_vs_consts_i[j] = j;
        }
        object->num_contained_vs_consts_i = MAX_CONST_I;
        for(j = 0; j < MAX_CONST_B; j++) {
            object->contained_vs_consts_b[j] = j;
        }
        object->num_contained_vs_consts_b = MAX_CONST_B;
        for(j = 0; j < GL_LIMITS(pshader_constantsF); j++) {
            object->contained_ps_consts_f[j] = j;
        }
        object->num_contained_ps_consts_f = GL_LIMITS(pshader_constantsF);
        for(j = 0; j < MAX_CONST_I; j++) {
            object->contained_ps_consts_i[j] = j;
        }
        object->num_contained_ps_consts_i = MAX_CONST_I;
        for(j = 0; j < MAX_CONST_B; j++) {
            object->contained_ps_consts_b[j] = j;
        }
        object->num_contained_ps_consts_b = MAX_CONST_B;
        for(i = 0; i < MAX_TEXTURES; i++) {
            for(j = 1; j <= WINED3D_HIGHEST_TEXTURE_STATE; j++) {
                object->contained_tss_states[object->num_contained_tss_states].stage = i;
                object->contained_tss_states[object->num_contained_tss_states].state = j;
                object->num_contained_tss_states++;
            }
        }
        for(i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
            for(j = 1; j <= WINED3D_HIGHEST_SAMPLER_STATE; j++) {
                object->contained_sampler_states[object->num_contained_sampler_states].stage = i;
                object->contained_sampler_states[object->num_contained_sampler_states].state = j;
                object->num_contained_sampler_states++;
            }
        }

        for(i = 0; i < MAX_STREAMS; i++) {
            if(object->streamSource[i]) {
                IWineD3DVertexBuffer_AddRef(object->streamSource[i]);
            }
        }
        if(object->pIndexData) {
            IWineD3DIndexBuffer_AddRef(object->pIndexData);
        }
        if(object->vertexShader) {
            IWineD3DVertexShader_AddRef(object->vertexShader);
        }
        if(object->pixelShader) {
            IWineD3DPixelShader_AddRef(object->pixelShader);
        }

    } else if (Type == WINED3DSBT_PIXELSTATE) {

        TRACE("PIXELSTATE => Pretend all pixel shates have changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, FALSE);

        object->changed.pixelShader = TRUE;

        /* Pixel Shader Constants */
        for (i = 0; i < GL_LIMITS(vshader_constantsF); ++i) {
            object->contained_ps_consts_f[i] = i;
            object->changed.pixelShaderConstantsF[i] = TRUE;
        }
        object->num_contained_ps_consts_f = GL_LIMITS(vshader_constantsF);
        for (i = 0; i < MAX_CONST_B; ++i) {
            object->contained_ps_consts_b[i] = i;
            object->changed.pixelShaderConstantsB[i] = TRUE;
        }
        object->num_contained_ps_consts_b = MAX_CONST_B;
        for (i = 0; i < MAX_CONST_I; ++i) {
            object->contained_ps_consts_i[i] = i;
            object->changed.pixelShaderConstantsI[i] = TRUE;
        }
        object->num_contained_ps_consts_i = MAX_CONST_I;

        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            object->changed.renderState[SavedPixelStates_R[i]] = TRUE;
            object->contained_render_states[i] = SavedPixelStates_R[i];
        }
        object->num_contained_render_states = NUM_SAVEDPIXELSTATES_R;
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                object->changed.textureState[j][SavedPixelStates_T[i]] = TRUE;
                object->contained_tss_states[object->num_contained_tss_states].stage = j;
                object->contained_tss_states[object->num_contained_tss_states].state = SavedPixelStates_T[i];
                object->num_contained_tss_states++;
            }
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++) {
            for (i =0; i < NUM_SAVEDPIXELSTATES_S;i++) {
                object->changed.samplerState[j][SavedPixelStates_S[i]] = TRUE;
                object->contained_sampler_states[object->num_contained_sampler_states].stage = j;
                object->contained_sampler_states[object->num_contained_sampler_states].state = SavedPixelStates_S[i];
                object->num_contained_sampler_states++;
            }
        }
        if(object->pixelShader) {
            IWineD3DPixelShader_AddRef(object->pixelShader);
        }

        /* Pixel state blocks do not contain vertex buffers. Set them to NULL to avoid wrong refcounting
         * on them. This makes releasing the buffer easier
         */
        for(i = 0; i < MAX_STREAMS; i++) {
            object->streamSource[i] = NULL;
        }
        object->pIndexData = NULL;
        object->vertexShader = NULL;

    } else if (Type == WINED3DSBT_VERTEXSTATE) {

        TRACE("VERTEXSTATE => Pretend all vertex shates have changed\n");
        stateblock_savedstates_set((IWineD3DStateBlock*) object, &object->changed, FALSE);

        object->changed.vertexShader = TRUE;

        /* Vertex Shader Constants */
        for (i = 0; i < GL_LIMITS(vshader_constantsF); ++i) {
            object->changed.vertexShaderConstantsF[i] = TRUE;
            object->contained_vs_consts_f[i] = i;
        }
        object->num_contained_vs_consts_f = GL_LIMITS(vshader_constantsF);
        for (i = 0; i < MAX_CONST_B; ++i) {
            object->changed.vertexShaderConstantsB[i] = TRUE;
            object->contained_vs_consts_b[i] = i;
        }
        object->num_contained_vs_consts_b = MAX_CONST_B;
        for (i = 0; i < MAX_CONST_I; ++i) {
            object->changed.vertexShaderConstantsI[i] = TRUE;
            object->contained_vs_consts_i[i] = i;
        }
        object->num_contained_vs_consts_i = MAX_CONST_I;
        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            object->changed.renderState[SavedVertexStates_R[i]] = TRUE;
            object->contained_render_states[i] = SavedVertexStates_R[i];
        }
        object->num_contained_render_states = NUM_SAVEDVERTEXSTATES_R;
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                object->changed.textureState[j][SavedVertexStates_T[i]] = TRUE;
                object->contained_tss_states[object->num_contained_tss_states].stage = j;
                object->contained_tss_states[object->num_contained_tss_states].state = SavedVertexStates_T[i];
                object->num_contained_tss_states++;
            }
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++){
            for (i =0; i < NUM_SAVEDVERTEXSTATES_S;i++) {
                object->changed.samplerState[j][SavedVertexStates_S[i]] = TRUE;
                object->contained_sampler_states[object->num_contained_sampler_states].stage = j;
                object->contained_sampler_states[object->num_contained_sampler_states].state = SavedVertexStates_S[i];
                object->num_contained_sampler_states++;
            }
        }

        for(j = 0; j < LIGHTMAP_SIZE; j++) {
            struct list *e;
            LIST_FOR_EACH(e, &object->lightMap[j]) {
                PLIGHTINFOEL *light = LIST_ENTRY(e, PLIGHTINFOEL, entry);
                light->changed = TRUE;
                light->enabledChanged = TRUE;
            }
        }

        for(i = 0; i < MAX_STREAMS; i++) {
            if(object->streamSource[i]) {
                IWineD3DVertexBuffer_AddRef(object->streamSource[i]);
            }
        }
        if(object->vertexShader) {
            IWineD3DVertexShader_AddRef(object->vertexShader);
        }
        object->pIndexData = NULL;
        object->pixelShader = NULL;
    } else {
        FIXME("Unrecognized state block type %d\n", Type);
    }

    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);
    return WINED3D_OK;
}

/* ************************************
MSDN:
[in] Render targets are not lockable unless the application specifies TRUE for Lockable. Note that lockable render targets reduce performance on some graphics hardware.

Discard
 [in] Set this flag to TRUE to enable z-buffer discarding, and FALSE otherwise. 

If this flag is set, the contents of the depth stencil buffer will be invalid after calling either IDirect3DDevice9::Present or IDirect3DDevice9::SetDepthStencilSurface with a different depth surface.

******************************** */
 
static HRESULT  WINAPI IWineD3DDeviceImpl_CreateSurface(IWineD3DDevice *iface, UINT Width, UINT Height, WINED3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IWineD3DSurface **ppSurface,WINED3DRESOURCETYPE Type, DWORD Usage, WINED3DPOOL Pool, WINED3DMULTISAMPLE_TYPE MultiSample ,DWORD MultisampleQuality, HANDLE* pSharedHandle, WINED3DSURFTYPE Impl, IUnknown *parent) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;    
    IWineD3DSurfaceImpl *object; /*NOTE: impl ref allowed since this is a create function */
    unsigned int Size       = 1;
    const StaticPixelFormatDesc *tableEntry = getFormatDescEntry(Format, NULL, NULL);
    TRACE("(%p) Create surface\n",This);
    
    /** FIXME: Check ranges on the inputs are valid 
     * MSDN
     *   MultisampleQuality
     *    [in] Quality level. The valid range is between zero and one less than the level
     *    returned by pQualityLevels used by IDirect3D9::CheckDeviceMultiSampleType. 
     *    Passing a larger value returns the error WINED3DERR_INVALIDCALL. The MultisampleQuality
     *    values of paired render targets, depth stencil surfaces, and the MultiSample type
     *    must all match.
      *******************************/


    /**
    * TODO: Discard MSDN
    * [in] Set this flag to TRUE to enable z-buffer discarding, and FALSE otherwise.
    *
    * If this flag is set, the contents of the depth stencil buffer will be
    * invalid after calling either IDirect3DDevice9::Present or  * IDirect3DDevice9::SetDepthStencilSurface
    * with a different depth surface.
    *
    *This flag has the same behavior as the constant, D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL, in D3DPRESENTFLAG.
    ***************************/

    if(MultisampleQuality < 0) {
        FIXME("Invalid multisample level %d\n", MultisampleQuality);
        return WINED3DERR_INVALIDCALL; /* TODO: Check that this is the case! */
    }

    if(MultisampleQuality > 0) {
        FIXME("MultisampleQuality set to %d, substituting 0\n", MultisampleQuality);
        MultisampleQuality=0;
    }

    /** FIXME: Check that the format is supported
    *    by the device.
      *******************************/

    /** DXTn mipmaps use the same number of 'levels' down to eg. 8x1, but since
     *  it is based around 4x4 pixel blocks it requires padding, so allocate enough
     *  space!
      *********************************/
    if (WINED3DFMT_UNKNOWN == Format) {
        Size = 0;
    } else if (Format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
       Size = ((max(Width,4) * tableEntry->bpp) * max(Height,4)) >> 1;

    } else if (Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3 ||
               Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
       Size = ((max(Width,4) * tableEntry->bpp) * max(Height,4));
    } else {
       /* The pitch is a multiple of 4 bytes */
        Size = ((Width * tableEntry->bpp) + This->surface_alignment - 1) & ~(This->surface_alignment - 1);
       Size *= Height;
    }

    /** Create and initialise the surface resource **/
    D3DCREATERESOURCEOBJECTINSTANCE(object,Surface,WINED3DRTYPE_SURFACE, Size)
    /* "Standalone" surface */
    IWineD3DSurface_SetContainer((IWineD3DSurface *)object, NULL);

    object->currentDesc.Width      = Width;
    object->currentDesc.Height     = Height;
    object->currentDesc.MultiSampleType    = MultiSample;
    object->currentDesc.MultiSampleQuality = MultisampleQuality;
    object->glDescription.level            = Level;

    /* Flags */
    object->Flags      = 0;
    object->Flags     |= Discard ? SFLAG_DISCARD : 0;
    object->Flags     |= (WINED3DFMT_D16_LOCKABLE == Format) ? SFLAG_LOCKABLE : 0;
    object->Flags     |= Lockable ? SFLAG_LOCKABLE : 0;


    if (WINED3DFMT_UNKNOWN != Format) {
        object->bytesPerPixel = tableEntry->bpp;
    } else {
        object->bytesPerPixel = 0;
    }

    /** TODO: change this into a texture transform matrix so that it's processed in hardware **/

    TRACE("Pool %d %d %d %d\n",Pool, WINED3DPOOL_DEFAULT, WINED3DPOOL_MANAGED, WINED3DPOOL_SYSTEMMEM);

    /** Quick lockable sanity check TODO: remove this after surfaces, usage and lockability have been debugged properly
    * this function is too deep to need to care about things like this.
    * Levels need to be checked too, and possibly Type since they all affect what can be done.
    * ****************************************/
    switch(Pool) {
    case WINED3DPOOL_SCRATCH:
        if(!Lockable)
            FIXME("Create surface called with a pool of SCRATCH and a Lockable of FALSE "
                "which are mutually exclusive, setting lockable to TRUE\n");
                Lockable = TRUE;
    break;
    case WINED3DPOOL_SYSTEMMEM:
        if(!Lockable) FIXME("Create surface called with a pool of SYSTEMMEM and a Lockable of FALSE, "
                                    "this is acceptable but unexpected (I can't know how the surface can be usable!)\n");
    case WINED3DPOOL_MANAGED:
        if(Usage == WINED3DUSAGE_DYNAMIC) FIXME("Create surface called with a pool of MANAGED and a "
                                                "Usage of DYNAMIC which are mutually exclusive, not doing "
                                                "anything just telling you.\n");
    break;
    case WINED3DPOOL_DEFAULT: /*TODO: Create offscreen plain can cause this check to fail..., find out if it should */
        if(!(Usage & WINED3DUSAGE_DYNAMIC) && !(Usage & WINED3DUSAGE_RENDERTARGET)
           && !(Usage && WINED3DUSAGE_DEPTHSTENCIL ) && Lockable)
            WARN("Creating a surface with a POOL of DEFAULT with Lockable true, that doesn't specify DYNAMIC usage.\n");
    break;
    default:
        FIXME("(%p) Unknown pool %d\n", This, Pool);
    break;
    };

    if (Usage & WINED3DUSAGE_RENDERTARGET && Pool != WINED3DPOOL_DEFAULT) {
        FIXME("Trying to create a render target that isn't in the default pool\n");
    }

    /* mark the texture as dirty so that it gets loaded first time around*/
    IWineD3DSurface_AddDirtyRect(*ppSurface, NULL);
    TRACE("(%p) : w(%d) h(%d) fmt(%d,%s) lockable(%d) surf@%p, surfmem@%p, %d bytes\n",
           This, Width, Height, Format, debug_d3dformat(Format),
           (WINED3DFMT_D16_LOCKABLE == Format), *ppSurface, object->resource.allocatedMemory, object->resource.size);

    /* Store the DirectDraw primary surface. This is the first rendertarget surface created */
    if( (Usage & WINED3DUSAGE_RENDERTARGET) && (!This->ddraw_primary) )
        This->ddraw_primary = (IWineD3DSurface *) object;

    /* Look at the implementation and set the correct Vtable */
    switch(Impl) {
        case SURFACE_OPENGL:
            /* Check if a 3D adapter is available when creating gl surfaces */
            if(!This->adapter) {
                ERR("OpenGL surfaces are not available without opengl\n");
                HeapFree(GetProcessHeap(), 0, object->resource.allocatedMemory);
                HeapFree(GetProcessHeap(), 0, object);
                return WINED3DERR_NOTAVAILABLE;
            }
            break;

        case SURFACE_GDI:
            object->lpVtbl = &IWineGDISurface_Vtbl;
            break;

        default:
            /* To be sure to catch this */
            ERR("Unknown requested surface implementation %d!\n", Impl);
            IWineD3DSurface_Release((IWineD3DSurface *) object);
            return WINED3DERR_INVALIDCALL;
    }

    list_init(&object->renderbuffers);

    /* Call the private setup routine */
    return IWineD3DSurface_PrivateSetup( (IWineD3DSurface *) object );

}

static HRESULT  WINAPI IWineD3DDeviceImpl_CreateTexture(IWineD3DDevice *iface, UINT Width, UINT Height, UINT Levels,
                                                 DWORD Usage, WINED3DFORMAT Format, WINED3DPOOL Pool,
                                                 IWineD3DTexture** ppTexture, HANDLE* pSharedHandle, IUnknown *parent,
                                                 D3DCB_CREATESURFACEFN D3DCB_CreateSurface) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DTextureImpl *object;
    unsigned int i;
    UINT tmpW;
    UINT tmpH;
    HRESULT hr;
    unsigned int pow2Width;
    unsigned int pow2Height;
    const GlPixelFormatDesc *glDesc;
    getFormatDescEntry(Format, &GLINFO_LOCATION, &glDesc);


    TRACE("(%p) : Width %d, Height %d, Levels %d, Usage %#x\n", This, Width, Height, Levels, Usage);
    TRACE("Format %#x (%s), Pool %#x, ppTexture %p, pSharedHandle %p, parent %p\n",
            Format, debug_d3dformat(Format), Pool, ppTexture, pSharedHandle, parent);

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, Texture, WINED3DRTYPE_TEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);    
    object->width  = Width;
    object->height = Height;

    /** Non-power2 support **/
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO)) {
        pow2Width = Width;
        pow2Height = Height;
    } else {
        /* Find the nearest pow2 match */
        pow2Width = pow2Height = 1;
        while (pow2Width < Width) pow2Width <<= 1;
        while (pow2Height < Height) pow2Height <<= 1;
    }

    /** FIXME: add support for real non-power-two if it's provided by the video card **/
    /* Precalculated scaling for 'faked' non power of two texture coords */
    object->baseTexture.pow2Matrix[0] =  (((float)Width)  / ((float)pow2Width));
    object->baseTexture.pow2Matrix[5] =  (((float)Height) / ((float)pow2Height));
    object->baseTexture.pow2Matrix[10] = 1.0;
    object->baseTexture.pow2Matrix[15] = 1.0;
    TRACE(" xf(%f) yf(%f)\n", object->baseTexture.pow2Matrix[0], object->baseTexture.pow2Matrix[5]);

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
        if(!GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }
        if(Levels > 1) {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }
        object->baseTexture.levels = 1;
    } else if (Levels == 0) {
        TRACE("calculating levels %d\n", object->baseTexture.levels);
        object->baseTexture.levels++;
        tmpW = Width;
        tmpH = Height;
        while (tmpW > 1 || tmpH > 1) {
            tmpW = max(1, tmpW >> 1);
            tmpH = max(1, tmpH >> 1);
            object->baseTexture.levels++;
        }
        TRACE("Calculated levels = %d\n", object->baseTexture.levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    for (i = 0; i < object->baseTexture.levels; i++)
    {
        /* use the callback to create the texture surface */
        hr = D3DCB_CreateSurface(This->parent, parent, tmpW, tmpH, Format, Usage, Pool, i, WINED3DCUBEMAP_FACE_POSITIVE_X, &object->surfaces[i],NULL);
        if (hr!= WINED3D_OK || ( (IWineD3DSurfaceImpl *) object->surfaces[i])->Flags & SFLAG_OVERSIZE) {
            FIXME("Failed to create surface  %p\n", object);
            /* clean up */
            object->surfaces[i] = NULL;
            IWineD3DTexture_Release((IWineD3DTexture *)object);

            *ppTexture = NULL;
            return hr;
        }

        IWineD3DSurface_SetContainer(object->surfaces[i], (IWineD3DBase *)object);
        TRACE("Created surface level %d @ %p\n", i, object->surfaces[i]);
        /* calculate the next mipmap level */
        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
    }
    object->baseTexture.shader_conversion_group = glDesc->conversion_group;

    TRACE("(%p) : Created  texture %p\n", This, object);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVolumeTexture(IWineD3DDevice *iface,
                                                      UINT Width, UINT Height, UINT Depth,
                                                      UINT Levels, DWORD Usage,
                                                      WINED3DFORMAT Format, WINED3DPOOL Pool,
                                                      IWineD3DVolumeTexture **ppVolumeTexture,
                                                      HANDLE *pSharedHandle, IUnknown *parent,
                                                      D3DCB_CREATEVOLUMEFN D3DCB_CreateVolume) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeTextureImpl *object;
    unsigned int               i;
    UINT                       tmpW;
    UINT                       tmpH;
    UINT                       tmpD;
    const GlPixelFormatDesc *glDesc;

    getFormatDescEntry(Format, &GLINFO_LOCATION, &glDesc);

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }
    if(!GL_SUPPORT(EXT_TEXTURE3D)) {
        WARN("(%p) : Texture cannot be created - no volume texture support\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, VolumeTexture, WINED3DRTYPE_VOLUMETEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);

    TRACE("(%p) : W(%d) H(%d) D(%d), Lvl(%d) Usage(%d), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Levels, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    object->width  = Width;
    object->height = Height;
    object->depth  = Depth;

    /* Is NP2 support for volumes needed? */
    object->baseTexture.pow2Matrix[ 0] = 1.0;
    object->baseTexture.pow2Matrix[ 5] = 1.0;
    object->baseTexture.pow2Matrix[10] = 1.0;
    object->baseTexture.pow2Matrix[15] = 1.0;

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
        if(!GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }
        if(Levels > 1) {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        }
        Levels = 1;
    } else if (Levels == 0) {
        object->baseTexture.levels++;
        tmpW = Width;
        tmpH = Height;
        tmpD = Depth;
        while (tmpW > 1 || tmpH > 1 || tmpD > 1) {
            tmpW = max(1, tmpW >> 1);
            tmpH = max(1, tmpH >> 1);
            tmpD = max(1, tmpD >> 1);
            object->baseTexture.levels++;
        }
        TRACE("Calculated levels = %d\n", object->baseTexture.levels);
    }

    /* Generate all the surfaces */
    tmpW = Width;
    tmpH = Height;
    tmpD = Depth;

    for (i = 0; i < object->baseTexture.levels; i++)
    {
        HRESULT hr;
        /* Create the volume */
        hr = D3DCB_CreateVolume(This->parent, parent, tmpW, tmpH, tmpD, Format, Pool, Usage,
                                (IWineD3DVolume **)&object->volumes[i], pSharedHandle);

        if(FAILED(hr)) {
            ERR("Creating a volume for the volume texture failed(%08x)\n", hr);
            IWineD3DVolumeTexture_Release((IWineD3DVolumeTexture *) object);
            *ppVolumeTexture = NULL;
            return hr;
        }

        /* Set its container to this object */
        IWineD3DVolume_SetContainer(object->volumes[i], (IWineD3DBase *)object);

        /* calcualte the next mipmap level */
        tmpW = max(1, tmpW >> 1);
        tmpH = max(1, tmpH >> 1);
        tmpD = max(1, tmpD >> 1);
    }
    object->baseTexture.shader_conversion_group = glDesc->conversion_group;

    *ppVolumeTexture = (IWineD3DVolumeTexture *) object;
    TRACE("(%p) : Created volume texture %p\n", This, object);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVolume(IWineD3DDevice *iface,
                                               UINT Width, UINT Height, UINT Depth,
                                               DWORD Usage,
                                               WINED3DFORMAT Format, WINED3DPOOL Pool,
                                               IWineD3DVolume** ppVolume,
                                               HANDLE* pSharedHandle, IUnknown *parent) {

    IWineD3DDeviceImpl        *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVolumeImpl        *object; /** NOTE: impl ref allowed since this is a create function **/
    const StaticPixelFormatDesc *formatDesc  = getFormatDescEntry(Format, NULL, NULL);

    if(!GL_SUPPORT(EXT_TEXTURE3D)) {
        WARN("(%p) : Volume cannot be created - no volume texture support\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, Volume, WINED3DRTYPE_VOLUME, ((Width * formatDesc->bpp) * Height * Depth))

    TRACE("(%p) : W(%d) H(%d) D(%d), Usage(%d), Fmt(%u,%s), Pool(%s)\n", This, Width, Height,
          Depth, Usage, Format, debug_d3dformat(Format), debug_d3dpool(Pool));

    object->currentDesc.Width   = Width;
    object->currentDesc.Height  = Height;
    object->currentDesc.Depth   = Depth;
    object->bytesPerPixel       = formatDesc->bpp;

    /** Note: Volume textures cannot be dxtn, hence no need to check here **/
    object->lockable            = TRUE;
    object->locked              = FALSE;
    memset(&object->lockedBox, 0, sizeof(WINED3DBOX));
    object->dirty               = TRUE;

    return IWineD3DVolume_AddDirtyBox((IWineD3DVolume *) object, NULL);
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateCubeTexture(IWineD3DDevice *iface, UINT EdgeLength,
                                                    UINT Levels, DWORD Usage,
                                                    WINED3DFORMAT Format, WINED3DPOOL Pool,
                                                    IWineD3DCubeTexture **ppCubeTexture,
                                                    HANDLE *pSharedHandle, IUnknown *parent,
                                                    D3DCB_CREATESURFACEFN D3DCB_CreateSurface) {

    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DCubeTextureImpl *object; /** NOTE: impl ref allowed since this is a create function **/
    unsigned int             i, j;
    UINT                     tmpW;
    HRESULT                  hr;
    unsigned int pow2EdgeLength  = EdgeLength;
    const GlPixelFormatDesc *glDesc;
    getFormatDescEntry(Format, &GLINFO_LOCATION, &glDesc);

    /* TODO: It should only be possible to create textures for formats 
             that are reported as supported */
    if (WINED3DFMT_UNKNOWN >= Format) {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if (!GL_SUPPORT(ARB_TEXTURE_CUBE_MAP) && Pool != WINED3DPOOL_SCRATCH) {
        WARN("(%p) : Tried to create not supported cube texture\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    D3DCREATERESOURCEOBJECTINSTANCE(object, CubeTexture, WINED3DRTYPE_CUBETEXTURE, 0);
    D3DINITIALIZEBASETEXTURE(object->baseTexture);

    TRACE("(%p) Create Cube Texture\n", This);

    /** Non-power2 support **/

    /* Find the nearest pow2 match */
    pow2EdgeLength = 1;
    while (pow2EdgeLength < EdgeLength) pow2EdgeLength <<= 1;

    object->edgeLength           = EdgeLength;
    /* TODO: support for native non-power 2 */
    /* Precalculated scaling for 'faked' non power of two texture coords */
    object->baseTexture.pow2Matrix[ 0] = ((float)EdgeLength) / ((float)pow2EdgeLength);
    object->baseTexture.pow2Matrix[ 5] = ((float)EdgeLength) / ((float)pow2EdgeLength);
    object->baseTexture.pow2Matrix[10] = ((float)EdgeLength) / ((float)pow2EdgeLength);
    object->baseTexture.pow2Matrix[15] = 1.0;

    /* Calculate levels for mip mapping */
    if (Usage & WINED3DUSAGE_AUTOGENMIPMAP) {
        if(!GL_SUPPORT(SGIS_GENERATE_MIPMAP)) {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL\n");
            HeapFree(GetProcessHeap(), 0, object);
            *ppCubeTexture = NULL;

            return WINED3DERR_INVALIDCALL;
        }
        if(Levels > 1) {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL\n");
            HeapFree(GetProcessHeap(), 0, object);
            *ppCubeTexture = NULL;

            return WINED3DERR_INVALIDCALL;
        }
        Levels = 1;
    } else if (Levels == 0) {
        object->baseTexture.levels++;
        tmpW = EdgeLength;
        while (tmpW > 1) {
            tmpW = max(1, tmpW >> 1);
            object->baseTexture.levels++;
        }
        TRACE("Calculated levels = %d\n", object->baseTexture.levels);
    }

    /* Generate all the surfaces */
    tmpW = EdgeLength;
    for (i = 0; i < object->baseTexture.levels; i++) {

        /* Create the 6 faces */
        for (j = 0; j < 6; j++) {

            hr=D3DCB_CreateSurface(This->parent, parent, tmpW, tmpW, Format, Usage, Pool,
                                   i /* Level */, j, &object->surfaces[j][i],pSharedHandle);

            if(hr!= WINED3D_OK) {
                /* clean up */
                int k;
                int l;
                for (l = 0; l < j; l++) {
                    IWineD3DSurface_Release(object->surfaces[l][i]);
                }
                for (k = 0; k < i; k++) {
                    for (l = 0; l < 6; l++) {
                        IWineD3DSurface_Release(object->surfaces[l][k]);
                    }
                }

                FIXME("(%p) Failed to create surface\n",object);
                HeapFree(GetProcessHeap(),0,object);
                *ppCubeTexture = NULL;
                return hr;
            }
            IWineD3DSurface_SetContainer(object->surfaces[j][i], (IWineD3DBase *)object);
            TRACE("Created surface level %d @ %p,\n", i, object->surfaces[j][i]);
        }
        tmpW = max(1, tmpW >> 1);
    }
    object->baseTexture.shader_conversion_group = glDesc->conversion_group;

    TRACE("(%p) : Created Cube Texture %p\n", This, object);
    *ppCubeTexture = (IWineD3DCubeTexture *) object;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateQuery(IWineD3DDevice *iface, WINED3DQUERYTYPE Type, IWineD3DQuery **ppQuery, IUnknown* parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DQueryImpl *object; /*NOTE: impl ref allowed since this is a create function */
    HRESULT hr = WINED3DERR_NOTAVAILABLE;

    /* Just a check to see if we support this type of query */
    switch(Type) {
    case WINED3DQUERYTYPE_OCCLUSION:
        TRACE("(%p) occlusion query\n", This);
        if (GL_SUPPORT(ARB_OCCLUSION_QUERY))
            hr = WINED3D_OK;
        else
            WARN("Unsupported in local OpenGL implementation: ARB_OCCLUSION_QUERY/NV_OCCLUSION_QUERY\n");
        break;

    case WINED3DQUERYTYPE_EVENT:
        if(!(GL_SUPPORT(NV_FENCE) || GL_SUPPORT(APPLE_FENCE) )) {
            /* Half-Life 2 needs this query. It does not render the main menu correctly otherwise
             * Pretend to support it, faking this query does not do much harm except potentially lowering performance
             */
            FIXME("(%p) Event query: Unimplemented, but pretending to be supported\n", This);
        }
        hr = WINED3D_OK;
        break;

    case WINED3DQUERYTYPE_VCACHE:
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    case WINED3DQUERYTYPE_VERTEXSTATS:
    case WINED3DQUERYTYPE_TIMESTAMP:
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    default:
        FIXME("(%p) Unhandled query type %d\n", This, Type);
    }
    if(NULL == ppQuery || hr != WINED3D_OK) {
        return hr;
    }

    D3DCREATEOBJECTINSTANCE(object, Query)
    object->type         = Type;
    /* allocated the 'extended' data based on the type of query requested */
    switch(Type){
    case WINED3DQUERYTYPE_OCCLUSION:
        object->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineQueryOcclusionData));
        ((WineQueryOcclusionData *)(object->extendedData))->ctx = This->activeContext;

        if(GL_SUPPORT(ARB_OCCLUSION_QUERY)) {
            TRACE("(%p) Allocating data for an occlusion query\n", This);
            GL_EXTCALL(glGenQueriesARB(1, &((WineQueryOcclusionData *)(object->extendedData))->queryId));
            break;
        }
    case WINED3DQUERYTYPE_EVENT:
        object->extendedData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineQueryEventData));
        ((WineQueryEventData *)(object->extendedData))->ctx = This->activeContext;

        if(GL_SUPPORT(APPLE_FENCE)) {
            GL_EXTCALL(glGenFencesAPPLE(1, &((WineQueryEventData *)(object->extendedData))->fenceId));
            checkGLcall("glGenFencesAPPLE");
        } else if(GL_SUPPORT(NV_FENCE)) {
            GL_EXTCALL(glGenFencesNV(1, &((WineQueryEventData *)(object->extendedData))->fenceId));
            checkGLcall("glGenFencesNV");
        }
        break;

    case WINED3DQUERYTYPE_VCACHE:
    case WINED3DQUERYTYPE_RESOURCEMANAGER:
    case WINED3DQUERYTYPE_VERTEXSTATS:
    case WINED3DQUERYTYPE_TIMESTAMP:
    case WINED3DQUERYTYPE_TIMESTAMPDISJOINT:
    case WINED3DQUERYTYPE_TIMESTAMPFREQ:
    case WINED3DQUERYTYPE_PIPELINETIMINGS:
    case WINED3DQUERYTYPE_INTERFACETIMINGS:
    case WINED3DQUERYTYPE_VERTEXTIMINGS:
    case WINED3DQUERYTYPE_PIXELTIMINGS:
    case WINED3DQUERYTYPE_BANDWIDTHTIMINGS:
    case WINED3DQUERYTYPE_CACHEUTILIZATION:
    default:
        object->extendedData = 0;
        FIXME("(%p) Unhandled query type %d\n",This , Type);
    }
    TRACE("(%p) : Created Query %p\n", This, object);
    return WINED3D_OK;
}

/*****************************************************************************
 * IWineD3DDeviceImpl_SetupFullscreenWindow
 *
 * Helper function that modifies a HWND's Style and ExStyle for proper
 * fullscreen use.
 *
 * Params:
 *  iface: Pointer to the IWineD3DDevice interface
 *  window: Window to setup
 *
 *****************************************************************************/
static void WINAPI IWineD3DDeviceImpl_SetupFullscreenWindow(IWineD3DDevice *iface, HWND window) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    LONG style, exStyle;
    /* Don't do anything if an original style is stored.
     * That shouldn't happen
     */
    TRACE("(%p): Setting up window %p for exclusive mode\n", This, window);
    if (This->style || This->exStyle) {
        ERR("(%p): Want to change the window parameters of HWND %p, but "
            "another style is stored for restoration afterwards\n", This, window);
    }

    /* Get the parameters and save them */
    style = GetWindowLongW(window, GWL_STYLE);
    exStyle = GetWindowLongW(window, GWL_EXSTYLE);
    This->style = style;
    This->exStyle = exStyle;

    /* Filter out window decorations */
    style &= ~WS_CAPTION;
    style &= ~WS_THICKFRAME;
    exStyle &= ~WS_EX_WINDOWEDGE;
    exStyle &= ~WS_EX_CLIENTEDGE;

    /* Make sure the window is managed, otherwise we won't get keyboard input */
    style |= WS_POPUP | WS_SYSMENU;

    TRACE("Old style was %08x,%08x, setting to %08x,%08x\n",
          This->style, This->exStyle, style, exStyle);

    SetWindowLongW(window, GWL_STYLE, style);
    SetWindowLongW(window, GWL_EXSTYLE, exStyle);

    /* Inform the window about the update. */
    SetWindowPos(window, HWND_TOP, 0, 0,
            This->ddraw_width, This->ddraw_height, SWP_FRAMECHANGED);
    ShowWindow(window, SW_NORMAL);
}

/*****************************************************************************
 * IWineD3DDeviceImpl_RestoreWindow
 *
 * Helper function that restores a windows' properties when taking it out
 * of fullscreen mode
 *
 * Params:
 *  iface: Pointer to the IWineD3DDevice interface
 *  window: Window to setup
 *
 *****************************************************************************/
static void WINAPI IWineD3DDeviceImpl_RestoreWindow(IWineD3DDevice *iface, HWND window) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* This could be a DDSCL_NORMAL -> DDSCL_NORMAL
     * switch, do nothing
     */
    if (!This->style && !This->exStyle) return;

    TRACE("(%p): Restoring window settings of window %p to %08x, %08x\n",
          This, window, This->style, This->exStyle);

    SetWindowLongW(window, GWL_STYLE, This->style);
    SetWindowLongW(window, GWL_EXSTYLE, This->exStyle);

    /* Delete the old values */
    This->style = 0;
    This->exStyle = 0;

    /* Inform the window about the update */
    SetWindowPos(window, 0 /* InsertAfter, ignored */,
                 0, 0, 0, 0, /* Pos, Size, ignored */
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

/* example at http://www.fairyengine.com/articles/dxmultiviews.htm */
static HRESULT WINAPI IWineD3DDeviceImpl_CreateAdditionalSwapChain(IWineD3DDevice* iface, WINED3DPRESENT_PARAMETERS*  pPresentationParameters,                                                                   IWineD3DSwapChain** ppSwapChain,
                                                            IUnknown* parent,
                                                            D3DCB_CREATERENDERTARGETFN D3DCB_CreateRenderTarget,
                                                            D3DCB_CREATEDEPTHSTENCILSURFACEFN D3DCB_CreateDepthStencil) {
    IWineD3DDeviceImpl      *This = (IWineD3DDeviceImpl *)iface;

    HDC                     hDc;
    IWineD3DSwapChainImpl  *object; /** NOTE: impl ref allowed since this is a create function **/
    HRESULT                 hr = WINED3D_OK;
    IUnknown               *bufferParent;
    BOOL                    displaymode_set = FALSE;

    TRACE("(%p) : Created Aditional Swap Chain\n", This);

   /** FIXME: Test under windows to find out what the life cycle of a swap chain is,
   * does a device hold a reference to a swap chain giving them a lifetime of the device
   * or does the swap chain notify the device of its destruction.
    *******************************/

    /* Check the params */
    if(pPresentationParameters->BackBufferCount > WINED3DPRESENT_BACK_BUFFER_MAX) {
        ERR("App requested %d back buffers, this is not supported for now\n", pPresentationParameters->BackBufferCount);
        return WINED3DERR_INVALIDCALL;
    } else if (pPresentationParameters->BackBufferCount > 1) {
        FIXME("The app requests more than one back buffer, this can't be supported properly. Please configure the application to use double buffering(=1 back buffer) if possible\n");
    }

    D3DCREATEOBJECTINSTANCE(object, SwapChain)

    /*********************
    * Lookup the window Handle and the relating X window handle
    ********************/

    /* Setup hwnd we are using, plus which display this equates to */
    object->win_handle = pPresentationParameters->hDeviceWindow;
    if (!object->win_handle) {
        object->win_handle = This->createParms.hFocusWindow;
    }
    if(!This->ddraw_window) IWineD3DDevice_SetHWND(iface, object->win_handle);

    hDc                = GetDC(object->win_handle);
    TRACE("Using hDc %p\n", hDc);

    if (NULL == hDc) {
        WARN("Failed to get a HDc for Window %p\n", object->win_handle);
        return WINED3DERR_NOTAVAILABLE;
    }

    object->orig_width = GetSystemMetrics(SM_CXSCREEN);
    object->orig_height = GetSystemMetrics(SM_CYSCREEN);
    object->orig_fmt = pixelformat_for_depth(GetDeviceCaps(hDc, BITSPIXEL) * GetDeviceCaps(hDc, PLANES));
    ReleaseDC(object->win_handle, hDc);

    /** MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
     *  then the corresponding dimension of the client area of the hDeviceWindow
     *  (or the focus window, if hDeviceWindow is NULL) is taken.
      **********************/

    if (pPresentationParameters->Windowed &&
        ((pPresentationParameters->BackBufferWidth == 0) ||
         (pPresentationParameters->BackBufferHeight == 0) ||
         (pPresentationParameters->BackBufferFormat == WINED3DFMT_UNKNOWN))) {

        RECT Rect;
        GetClientRect(object->win_handle, &Rect);

        if (pPresentationParameters->BackBufferWidth == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
        if (pPresentationParameters->BackBufferFormat == WINED3DFMT_UNKNOWN) {
           pPresentationParameters->BackBufferFormat = object->orig_fmt;
           TRACE("Updating format to %s\n", debug_d3dformat(object->orig_fmt));
        }
    }

    /* Put the correct figures in the presentation parameters */
    TRACE("Copying across presentation parameters\n");
    object->presentParms = *pPresentationParameters;

    TRACE("calling rendertarget CB\n");
    hr = D3DCB_CreateRenderTarget((IUnknown *) This->parent,
                             parent,
                             object->presentParms.BackBufferWidth,
                             object->presentParms.BackBufferHeight,
                             object->presentParms.BackBufferFormat,
                             object->presentParms.MultiSampleType,
                             object->presentParms.MultiSampleQuality,
                             TRUE /* Lockable */,
                             &object->frontBuffer,
                             NULL /* pShared (always null)*/);
    if (object->frontBuffer != NULL) {
        IWineD3DSurface_ModifyLocation(object->frontBuffer, SFLAG_INDRAWABLE, TRUE);
        IWineD3DSurface_SetContainer(object->frontBuffer, (IWineD3DBase *)object);
    } else {
        ERR("Failed to create the front buffer\n");
        goto error;
    }

   /*********************
   * Windowed / Fullscreen
   *******************/

   /**
   * TODO: MSDN says that we are only allowed one fullscreen swapchain per device,
   * so we should really check to see if there is a fullscreen swapchain already
   * I think Windows and X have different ideas about fullscreen, does a single head count as full screen?
    **************************************/

   if (!pPresentationParameters->Windowed) {

        DEVMODEW devmode;
        HDC      hdc;
        int      bpp = 0;
        RECT     clip_rc;

        /* Get info on the current display setup */
        hdc = GetDC(0);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        /* Change the display settings */
        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize       = sizeof(devmode);
        devmode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp; /* Stupid XVidMode cannot change bpp */
        devmode.dmPelsWidth  = pPresentationParameters->BackBufferWidth;
        devmode.dmPelsHeight = pPresentationParameters->BackBufferHeight;
        ChangeDisplaySettingsExW(This->adapter->DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);
        displaymode_set = TRUE;

        /* For GetDisplayMode */
        This->ddraw_width = devmode.dmPelsWidth;
        This->ddraw_height = devmode.dmPelsHeight;
        This->ddraw_format = pPresentationParameters->BackBufferFormat;

        IWineD3DDevice_SetFullscreen(iface, TRUE);

        /* And finally clip mouse to our screen */
        SetRect(&clip_rc, 0, 0, devmode.dmPelsWidth, devmode.dmPelsHeight);
        ClipCursor(&clip_rc);
    }

        /**
     * Create an opengl context for the display visual
     *  NOTE: the visual is chosen as the window is created and the glcontext cannot
     *     use different properties after that point in time. FIXME: How to handle when requested format
     *     doesn't match actual visual? Cannot choose one here - code removed as it ONLY works if the one
     *     it chooses is identical to the one already being used!
         **********************************/
    /** FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat **/

    object->context = HeapAlloc(GetProcessHeap(), 0, sizeof(object->context));
    if(!object->context)
        return E_OUTOFMEMORY;
    object->num_contexts = 1;

    object->context[0] = CreateContext(This, (IWineD3DSurfaceImpl *) object->frontBuffer, object->win_handle, FALSE /* pbuffer */, pPresentationParameters);
    if (!object->context[0]) {
        ERR("Failed to create a new context\n");
        hr = WINED3DERR_NOTAVAILABLE;
        goto error;
    } else {
        TRACE("Context created (HWND=%p, glContext=%p)\n",
              object->win_handle, object->context[0]->glCtx);
    }

   /*********************
   * Create the back, front and stencil buffers
   *******************/
    if(object->presentParms.BackBufferCount > 0) {
        int i;

        object->backBuffer = HeapAlloc(GetProcessHeap(), 0, sizeof(IWineD3DSurface *) * object->presentParms.BackBufferCount);
        if(!object->backBuffer) {
            ERR("Out of memory\n");
            hr = E_OUTOFMEMORY;
            goto error;
        }

        for(i = 0; i < object->presentParms.BackBufferCount; i++) {
            TRACE("calling rendertarget CB\n");
            hr = D3DCB_CreateRenderTarget((IUnknown *) This->parent,
                                    parent,
                                    object->presentParms.BackBufferWidth,
                                    object->presentParms.BackBufferHeight,
                                    object->presentParms.BackBufferFormat,
                                    object->presentParms.MultiSampleType,
                                    object->presentParms.MultiSampleQuality,
                                    TRUE /* Lockable */,
                                    &object->backBuffer[i],
                                    NULL /* pShared (always null)*/);
            if(hr == WINED3D_OK && object->backBuffer[i]) {
                IWineD3DSurface_SetContainer(object->backBuffer[i], (IWineD3DBase *)object);
            } else {
                ERR("Cannot create new back buffer\n");
                goto error;
            }
            ENTER_GL();
            glDrawBuffer(GL_BACK);
            checkGLcall("glDrawBuffer(GL_BACK)");
            LEAVE_GL();
        }
    } else {
        object->backBuffer = NULL;

        /* Single buffering - draw to front buffer */
        ENTER_GL();
        glDrawBuffer(GL_FRONT);
        checkGLcall("glDrawBuffer(GL_FRONT)");
        LEAVE_GL();
    }

    /* Under directX swapchains share the depth stencil, so only create one depth-stencil */
    if (pPresentationParameters->EnableAutoDepthStencil && hr == WINED3D_OK) {
        TRACE("Creating depth stencil buffer\n");
        if (This->auto_depth_stencil_buffer == NULL ) {
            hr = D3DCB_CreateDepthStencil((IUnknown *) This->parent,
                                    parent,
                                    object->presentParms.BackBufferWidth,
                                    object->presentParms.BackBufferHeight,
                                    object->presentParms.AutoDepthStencilFormat,
                                    object->presentParms.MultiSampleType,
                                    object->presentParms.MultiSampleQuality,
                                    FALSE /* FIXME: Discard */,
                                    &This->auto_depth_stencil_buffer,
                                    NULL /* pShared (always null)*/  );
            if (This->auto_depth_stencil_buffer != NULL)
                IWineD3DSurface_SetContainer(This->auto_depth_stencil_buffer, 0);
        }

        /** TODO: A check on width, height and multisample types
        *(since the zbuffer must be at least as large as the render target and have the same multisample parameters)
         ****************************/
        object->wantsDepthStencilBuffer = TRUE;
    } else {
        object->wantsDepthStencilBuffer = FALSE;
    }

    TRACE("Created swapchain %p\n", object);
    TRACE("FrontBuf @ %p, BackBuf @ %p, DepthStencil %d\n",object->frontBuffer, object->backBuffer ? object->backBuffer[0] : NULL, object->wantsDepthStencilBuffer);
    return WINED3D_OK;

error:
    if (displaymode_set) {
        DEVMODEW devmode;
        HDC      hdc;
        int      bpp = 0;
        RECT     clip_rc;

        SetRect(&clip_rc, 0, 0, object->orig_width, object->orig_height);
        ClipCursor(NULL);

        /* Get info on the current display setup */
        hdc = GetDC(0);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        /* Change the display settings */
        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize       = sizeof(devmode);
        devmode.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp; /* Stupid XVidMode cannot change bpp */
        devmode.dmPelsWidth  = object->orig_width;
        devmode.dmPelsHeight = object->orig_height;
        ChangeDisplaySettingsExW(This->adapter->DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);
    }

    if (object->backBuffer) {
        int i;
        for(i = 0; i < object->presentParms.BackBufferCount; i++) {
            if(object->backBuffer[i]) {
                IWineD3DSurface_GetParent(object->backBuffer[i], &bufferParent);
                IUnknown_Release(bufferParent); /* once for the get parent */
                if (IUnknown_Release(bufferParent) > 0) {
                    FIXME("(%p) Something's still holding the back buffer\n",This);
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, object->backBuffer);
        object->backBuffer = NULL;
    }
    if(object->context[0])
        DestroyContext(This, object->context[0]);
    if(object->frontBuffer) {
        IWineD3DSurface_GetParent(object->frontBuffer, &bufferParent);
        IUnknown_Release(bufferParent); /* once for the get parent */
        if (IUnknown_Release(bufferParent) > 0) {
            FIXME("(%p) Something's still holding the front buffer\n",This);
        }
    }
    HeapFree(GetProcessHeap(), 0, object);
    return hr;
}

/** NOTE: These are ahead of the other getters and setters to save using a forward declaration **/
static UINT     WINAPI  IWineD3DDeviceImpl_GetNumberOfSwapChains(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    return This->NumberOfSwapChains;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetSwapChain(IWineD3DDevice *iface, UINT iSwapChain, IWineD3DSwapChain **pSwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : swapchain %d\n", This, iSwapChain);

    if(iSwapChain < This->NumberOfSwapChains) {
        *pSwapChain = This->swapchains[iSwapChain];
        IWineD3DSwapChain_AddRef(*pSwapChain);
        TRACE("(%p) returning %p\n", This, *pSwapChain);
        return WINED3D_OK;
    } else {
        TRACE("Swapchain out of range\n");
        *pSwapChain = NULL;
        return WINED3DERR_INVALIDCALL;
    }
}

/*****
 * Vertex Declaration
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppVertexDeclaration,
        IUnknown *parent, const WINED3DVERTEXELEMENT *elements, size_t element_count) {
    IWineD3DDeviceImpl            *This   = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexDeclarationImpl *object = NULL;
    HRESULT hr = WINED3D_OK;

    TRACE("(%p) : directXVersion %u, elements %p, element_count %d, ppDecl=%p\n",
            This, ((IWineD3DImpl *)This->wineD3D)->dxVersion, elements, element_count, ppVertexDeclaration);

    D3DCREATEOBJECTINSTANCE(object, VertexDeclaration)

    hr = IWineD3DVertexDeclaration_SetDeclaration((IWineD3DVertexDeclaration *)object, elements, element_count);

    return hr;
}

static size_t ConvertFvfToDeclaration(DWORD fvf, WINED3DVERTEXELEMENT** ppVertexElements) {

    unsigned int idx, idx2;
    unsigned int offset;
    BOOL has_pos = (fvf & WINED3DFVF_POSITION_MASK) != 0;
    BOOL has_blend = (fvf & WINED3DFVF_XYZB5) > WINED3DFVF_XYZRHW;
    BOOL has_blend_idx = has_blend &&
       (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB5) ||
        (fvf & WINED3DFVF_LASTBETA_D3DCOLOR) ||
        (fvf & WINED3DFVF_LASTBETA_UBYTE4));
    BOOL has_normal = (fvf & WINED3DFVF_NORMAL) != 0;
    BOOL has_psize = (fvf & WINED3DFVF_PSIZE) != 0;
    BOOL has_diffuse = (fvf & WINED3DFVF_DIFFUSE) != 0;
    BOOL has_specular = (fvf & WINED3DFVF_SPECULAR) !=0;

    DWORD num_textures = (fvf & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;
    DWORD texcoords = (fvf & 0x00FF0000) >> 16;

    WINED3DVERTEXELEMENT end_element = WINED3DDECL_END();
    WINED3DVERTEXELEMENT *elements = NULL;

    unsigned int size;
    DWORD num_blends = 1 + (((fvf & WINED3DFVF_XYZB5) - WINED3DFVF_XYZB1) >> 1);
    if (has_blend_idx) num_blends--;

    /* Compute declaration size */
    size = has_pos + (has_blend && num_blends > 0) + has_blend_idx + has_normal +
           has_psize + has_diffuse + has_specular + num_textures + 1;

    /* convert the declaration */
    elements = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WINED3DVERTEXELEMENT));
    if (!elements)
        return 0;

    memcpy(&elements[size-1], &end_element, sizeof(WINED3DVERTEXELEMENT));
    idx = 0;
    if (has_pos) {
        if (!has_blend && (fvf & WINED3DFVF_XYZRHW)) {
            elements[idx].Type = WINED3DDECLTYPE_FLOAT4;
            elements[idx].Usage = WINED3DDECLUSAGE_POSITIONT;
        }
        else {
            elements[idx].Type = WINED3DDECLTYPE_FLOAT3;
            elements[idx].Usage = WINED3DDECLUSAGE_POSITION;
        }
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_blend && (num_blends > 0)) {
        if (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2) && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR))
            elements[idx].Type = WINED3DDECLTYPE_D3DCOLOR;
        else
            elements[idx].Type = WINED3DDECLTYPE_FLOAT1 + num_blends - 1;
        elements[idx].Usage = WINED3DDECLUSAGE_BLENDWEIGHT;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_blend_idx) {
        if (fvf & WINED3DFVF_LASTBETA_UBYTE4 ||
            (((fvf & WINED3DFVF_XYZB5) == WINED3DFVF_XYZB2) && (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)))
            elements[idx].Type = WINED3DDECLTYPE_UBYTE4;
        else if (fvf & WINED3DFVF_LASTBETA_D3DCOLOR)
            elements[idx].Type = WINED3DDECLTYPE_D3DCOLOR;
        else
            elements[idx].Type = WINED3DDECLTYPE_FLOAT1;
        elements[idx].Usage = WINED3DDECLUSAGE_BLENDINDICES;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_normal) {
        elements[idx].Type = WINED3DDECLTYPE_FLOAT3;
        elements[idx].Usage = WINED3DDECLUSAGE_NORMAL;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_psize) {
        elements[idx].Type = WINED3DDECLTYPE_FLOAT1;
        elements[idx].Usage = WINED3DDECLUSAGE_PSIZE;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_diffuse) {
        elements[idx].Type = WINED3DDECLTYPE_D3DCOLOR;
        elements[idx].Usage = WINED3DDECLUSAGE_COLOR;
        elements[idx].UsageIndex = 0;
        idx++;
    }
    if (has_specular) {
        elements[idx].Type = WINED3DDECLTYPE_D3DCOLOR;
        elements[idx].Usage = WINED3DDECLUSAGE_COLOR;
        elements[idx].UsageIndex = 1;
        idx++;
    }
    for (idx2 = 0; idx2 < num_textures; idx2++) {
        unsigned int numcoords = (texcoords >> (idx2*2)) & 0x03;
        switch (numcoords) {
            case WINED3DFVF_TEXTUREFORMAT1:
                elements[idx].Type = WINED3DDECLTYPE_FLOAT1;
                break;
            case WINED3DFVF_TEXTUREFORMAT2:
                elements[idx].Type = WINED3DDECLTYPE_FLOAT2;
                break;
            case WINED3DFVF_TEXTUREFORMAT3:
                elements[idx].Type = WINED3DDECLTYPE_FLOAT3;
                break;
            case WINED3DFVF_TEXTUREFORMAT4:
                elements[idx].Type = WINED3DDECLTYPE_FLOAT4;
                break;
        }
        elements[idx].Usage = WINED3DDECLUSAGE_TEXCOORD;
        elements[idx].UsageIndex = idx2;
        idx++;
    }

    /* Now compute offsets, and initialize the rest of the fields */
    for (idx = 0, offset = 0; idx < size-1; idx++) {
        elements[idx].Stream = 0;
        elements[idx].Method = WINED3DDECLMETHOD_DEFAULT;
        elements[idx].Offset = offset;
        offset += WINED3D_ATR_SIZE(elements[idx].Type) * WINED3D_ATR_TYPESIZE(elements[idx].Type);
    }

    *ppVertexElements = elements;
    return size;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexDeclarationFromFVF(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppVertexDeclaration, IUnknown *Parent, DWORD Fvf) {
    WINED3DVERTEXELEMENT* elements = NULL;
    size_t size;
    DWORD hr;

    size = ConvertFvfToDeclaration(Fvf, &elements);
    if (size == 0) return WINED3DERR_OUTOFVIDEOMEMORY;

    hr = IWineD3DDevice_CreateVertexDeclaration(iface, ppVertexDeclaration, Parent, elements, size);
    HeapFree(GetProcessHeap(), 0, elements);
    if (hr != S_OK) return hr;

    return WINED3D_OK;
}

/* http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/programmingguide/programmable/vertexshaders/vscreate.asp */
static HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexShader(IWineD3DDevice *iface, IWineD3DVertexDeclaration *vertex_declaration, CONST DWORD *pFunction, IWineD3DVertexShader **ppVertexShader, IUnknown *parent) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShaderImpl *object;  /* NOTE: impl usage is ok, this is a create */
    HRESULT hr = WINED3D_OK;
    D3DCREATESHADEROBJECTINSTANCE(object, VertexShader)
    object->baseShader.shader_ins = IWineD3DVertexShaderImpl_shader_ins;

    TRACE("(%p) : Created Vertex shader %p\n", This, *ppVertexShader);

    if (vertex_declaration) {
        IWineD3DVertexShader_FakeSemantics(*ppVertexShader, vertex_declaration);
    }

    hr = IWineD3DVertexShader_SetFunction(*ppVertexShader, pFunction);

    if (WINED3D_OK != hr) {
        FIXME("(%p) : Failed to set the function, returning WINED3DERR_INVALIDCALL\n", iface);
        IWineD3DVertexShader_Release(*ppVertexShader);
        return WINED3DERR_INVALIDCALL;
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreatePixelShader(IWineD3DDevice *iface, CONST DWORD *pFunction, IWineD3DPixelShader **ppPixelShader, IUnknown *parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShaderImpl *object; /* NOTE: impl allowed, this is a create */
    HRESULT hr = WINED3D_OK;

    D3DCREATESHADEROBJECTINSTANCE(object, PixelShader)
    object->baseShader.shader_ins = IWineD3DPixelShaderImpl_shader_ins;
    hr = IWineD3DPixelShader_SetFunction(*ppPixelShader, pFunction);
    if (WINED3D_OK == hr) {
        TRACE("(%p) : Created Pixel shader %p\n", This, *ppPixelShader);
    } else {
        WARN("(%p) : Failed to create pixel shader\n", This);
    }

    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_CreatePalette(IWineD3DDevice *iface, DWORD Flags, PALETTEENTRY *PalEnt, IWineD3DPalette **Palette, IUnknown *Parent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DPaletteImpl *object;
    HRESULT hr;
    TRACE("(%p)->(%x, %p, %p, %p)\n", This, Flags, PalEnt, Palette, Parent);

    /* Create the new object */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DPaletteImpl));
    if(!object) {
        ERR("Out of memory when allocating memory for a IWineD3DPalette implementation\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &IWineD3DPalette_Vtbl;
    object->ref = 1;
    object->Flags = Flags;
    object->parent = Parent;
    object->wineD3DDevice = This;
    object->palNumEntries = IWineD3DPaletteImpl_Size(Flags);
	
    object->hpal = CreatePalette((const LOGPALETTE*)&(object->palVersion));

    if(!object->hpal) {
        HeapFree( GetProcessHeap(), 0, object);
        return E_OUTOFMEMORY;
    }

    hr = IWineD3DPalette_SetEntries((IWineD3DPalette *) object, 0, 0, IWineD3DPaletteImpl_Size(Flags), PalEnt);
    if(FAILED(hr)) {
        IWineD3DPalette_Release((IWineD3DPalette *) object);
        return hr;
    }

    *Palette = (IWineD3DPalette *) object;

    return WINED3D_OK;
}

static void IWineD3DDeviceImpl_LoadLogo(IWineD3DDeviceImpl *This, const char *filename) {
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;
    WINEDDCOLORKEY colorkey;

    hbm = (HBITMAP) LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if(hbm)
    {
        GetObjectA(hbm, sizeof(BITMAP), &bm);
        dcb = CreateCompatibleDC(NULL);
        if(!dcb) goto out;
        SelectObject(dcb, hbm);
    }
    else
    {
        /* Create a 32x32 white surface to indicate that wined3d is used, but the specified image
         * couldn't be loaded
         */
        memset(&bm, 0, sizeof(bm));
        bm.bmWidth = 32;
        bm.bmHeight = 32;
    }

    hr = IWineD3DDevice_CreateSurface((IWineD3DDevice *) This, bm.bmWidth, bm.bmHeight, WINED3DFMT_R5G6B5,
                                      TRUE, FALSE, 0, &This->logo_surface, WINED3DRTYPE_SURFACE, 0,
                                      WINED3DPOOL_DEFAULT, WINED3DMULTISAMPLE_NONE, 0, NULL, SURFACE_OPENGL, NULL);
    if(FAILED(hr)) {
        ERR("Wine logo requested, but failed to create surface\n");
        goto out;
    }

    if(dcb) {
        hr = IWineD3DSurface_GetDC(This->logo_surface, &dcs);
        if(FAILED(hr)) goto out;
        BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
        IWineD3DSurface_ReleaseDC(This->logo_surface, dcs);

        colorkey.dwColorSpaceLowValue = 0;
        colorkey.dwColorSpaceHighValue = 0;
        IWineD3DSurface_SetColorKey(This->logo_surface, WINEDDCKEY_SRCBLT, &colorkey);
    } else {
        /* Fill the surface with a white color to show that wined3d is there */
        IWineD3DDevice_ColorFill((IWineD3DDevice *) This, This->logo_surface, NULL, 0xffffffff);
    }

    out:
    if(dcb) {
        DeleteDC(dcb);
    }
    if(hbm) {
        DeleteObject(hbm);
    }
    return;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Init3D(IWineD3DDevice *iface, WINED3DPRESENT_PARAMETERS* pPresentationParameters, D3DCB_CREATEADDITIONALSWAPCHAIN D3DCB_CreateAdditionalSwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain;
    HRESULT hr;
    DWORD state;

    TRACE("(%p)->(%p,%p)\n", This, pPresentationParameters, D3DCB_CreateAdditionalSwapChain);
    if(This->d3d_initialized) return WINED3DERR_INVALIDCALL;

    /* TODO: Test if OpenGL is compiled in and loaded */

    TRACE("(%p) : Creating stateblock\n", This);
    /* Creating the startup stateBlock - Note Special Case: 0 => Don't fill in yet! */
    hr = IWineD3DDevice_CreateStateBlock(iface,
                                         WINED3DSBT_INIT,
                                         (IWineD3DStateBlock **)&This->stateBlock,
                                         NULL);
    if (WINED3D_OK != hr) {   /* Note: No parent needed for initial internal stateblock */
        WARN("Failed to create stateblock\n");
        goto err_out;
    }
    TRACE("(%p) : Created stateblock (%p)\n", This, This->stateBlock);
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock*)This->updateStateBlock);

    hr = allocate_shader_constants(This->updateStateBlock);
    if (WINED3D_OK != hr) {
        goto err_out;
    }

    This->render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DSurface *) * GL_LIMITS(buffers));
    This->fbo_color_attachments = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DSurface *) * GL_LIMITS(buffers));
    This->draw_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLenum) * GL_LIMITS(buffers));

    /* Initialize the texture unit mapping to a 1:1 mapping */
    for (state = 0; state < MAX_COMBINED_SAMPLERS; ++state) {
        if (state < GL_LIMITS(fragment_samplers)) {
            This->texUnitMap[state] = state;
            This->rev_tex_unit_map[state] = state;
        } else {
            This->texUnitMap[state] = -1;
            This->rev_tex_unit_map[state] = -1;
        }
    }

    /* Setup the implicit swapchain */
    TRACE("Creating implicit swapchain\n");
    hr=D3DCB_CreateAdditionalSwapChain((IUnknown *) This->parent, pPresentationParameters, (IWineD3DSwapChain **)&swapchain);
    if (FAILED(hr) || !swapchain) {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    This->NumberOfSwapChains = 1;
    This->swapchains = HeapAlloc(GetProcessHeap(), 0, This->NumberOfSwapChains * sizeof(IWineD3DSwapChain *));
    if(!This->swapchains) {
        ERR("Out of memory!\n");
        goto err_out;
    }
    This->swapchains[0] = (IWineD3DSwapChain *) swapchain;

    if(swapchain->backBuffer && swapchain->backBuffer[0]) {
        TRACE("Setting rendertarget to %p\n", swapchain->backBuffer);
        This->render_targets[0] = swapchain->backBuffer[0];
        This->lastActiveRenderTarget = swapchain->backBuffer[0];
    }
    else {
        TRACE("Setting rendertarget to %p\n", swapchain->frontBuffer);
        This->render_targets[0] = swapchain->frontBuffer;
        This->lastActiveRenderTarget = swapchain->frontBuffer;
    }
    IWineD3DSurface_AddRef(This->render_targets[0]);
    This->activeContext = swapchain->context[0];
    This->lastThread = GetCurrentThreadId();

    /* Depth Stencil support */
    This->stencilBufferTarget = This->auto_depth_stencil_buffer;
    if (NULL != This->stencilBufferTarget) {
        IWineD3DSurface_AddRef(This->stencilBufferTarget);
    }

    /* Set up some starting GL setup */
    ENTER_GL();

    /* Setup all the devices defaults */
    IWineD3DStateBlock_InitStartupStateBlock((IWineD3DStateBlock *)This->stateBlock);
#if 0
    IWineD3DImpl_CheckGraphicsMemory();
#endif

    { /* Set a default viewport */
        WINED3DVIEWPORT vp;
        vp.X      = 0;
        vp.Y      = 0;
        vp.Width  = pPresentationParameters->BackBufferWidth;
        vp.Height = pPresentationParameters->BackBufferHeight;
        vp.MinZ   = 0.0f;
        vp.MaxZ   = 1.0f;
        IWineD3DDevice_SetViewport((IWineD3DDevice *)This, &vp);
    }

    /* Initialize the current view state */
    This->view_ident = 1;
    This->contexts[0]->last_was_rhw = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &This->maxConcurrentLights);
    checkGLcall("glGetIntegerv(GL_MAX_LIGHTS, &This->maxConcurrentLights)");

    switch(wined3d_settings.offscreen_rendering_mode) {
        case ORM_FBO:
        case ORM_PBUFFER:
            This->offscreenBuffer = GL_BACK;
            break;

        case ORM_BACKBUFFER:
        {
            if(GL_LIMITS(aux_buffers) > 0) {
                TRACE("Using auxilliary buffer for offscreen rendering\n");
                This->offscreenBuffer = GL_AUX0;
            } else {
                TRACE("Using back buffer for offscreen rendering\n");
                This->offscreenBuffer = GL_BACK;
            }
        }
    }

    TRACE("(%p) All defaults now set up, leaving Init3D with %p\n", This, This);
    LEAVE_GL();

    /* Clear the screen */
    IWineD3DDevice_Clear((IWineD3DDevice *) This, 0, NULL,
                          WINED3DCLEAR_TARGET | pPresentationParameters->EnableAutoDepthStencil ? WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL : 0,
                          0x00, 1.0, 0);

    This->d3d_initialized = TRUE;

    if(wined3d_settings.logo) {
        IWineD3DDeviceImpl_LoadLogo(This, wined3d_settings.logo);
    }
    return WINED3D_OK;

    err_out:
    HeapFree(GetProcessHeap(), 0, This->render_targets);
    HeapFree(GetProcessHeap(), 0, This->fbo_color_attachments);
    HeapFree(GetProcessHeap(), 0, This->draw_buffers);
    HeapFree(GetProcessHeap(), 0, This->swapchains);
    This->NumberOfSwapChains = 0;
    if(swapchain) {
        IWineD3DSwapChain_Release( (IWineD3DSwapChain *) swapchain);
    }
    This->draw_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLenum) * GL_LIMITS(buffers));
    if(This->stateBlock) {
        IWineD3DStateBlock_Release((IWineD3DStateBlock *) This->stateBlock);
        This->stateBlock = NULL;
    }
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Uninit3D(IWineD3DDevice *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroyDepthStencilSurface, D3DCB_DESTROYSWAPCHAINFN D3DCB_DestroySwapChain) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    int sampler;
    UINT i;
    TRACE("(%p)\n", This);

    if(!This->d3d_initialized) return WINED3DERR_INVALIDCALL;

    /* I don't think that the interface guarants that the device is destroyed from the same thread
     * it was created. Thus make sure a context is active for the glDelete* calls
     */
    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

    if(This->logo_surface) IWineD3DSurface_Release(This->logo_surface);

    TRACE("Deleting high order patches\n");
    for(i = 0; i < PATCHMAP_SIZE; i++) {
        struct list *e1, *e2;
        struct WineD3DRectPatch *patch;
        LIST_FOR_EACH_SAFE(e1, e2, &This->patches[i]) {
            patch = LIST_ENTRY(e1, struct WineD3DRectPatch, entry);
            IWineD3DDevice_DeletePatch(iface, patch->Handle);
        }
    }

    /* Delete the palette conversion shader if it is around */
    if(This->paletteConversionShader) {
        GL_EXTCALL(glDeleteProgramsARB(1, &This->paletteConversionShader));
        This->paletteConversionShader = 0;
    }

    /* Delete the pbuffer context if there is any */
    if(This->pbufferContext) DestroyContext(This, This->pbufferContext);

    /* Delete the mouse cursor texture */
    if(This->cursorTexture) {
        ENTER_GL();
        glDeleteTextures(1, &This->cursorTexture);
        LEAVE_GL();
        This->cursorTexture = 0;
    }

    for (sampler = 0; sampler < MAX_FRAGMENT_SAMPLERS; ++sampler) {
        IWineD3DDevice_SetTexture(iface, sampler, NULL);
    }
    for (sampler = 0; sampler < MAX_VERTEX_SAMPLERS; ++sampler) {
        IWineD3DDevice_SetTexture(iface, WINED3DVERTEXTEXTURESAMPLER0 + sampler, NULL);
    }

    /* Release the update stateblock */
    if(IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->updateStateBlock) > 0){
        if(This->updateStateBlock != This->stateBlock)
            FIXME("(%p) Something's still holding the Update stateblock\n",This);
    }
    This->updateStateBlock = NULL;

    { /* because were not doing proper internal refcounts releasing the primary state block
        causes recursion with the extra checks in ResourceReleased, to avoid this we have
        to set this->stateBlock = NULL; first */
        IWineD3DStateBlock *stateBlock = (IWineD3DStateBlock *)This->stateBlock;
        This->stateBlock = NULL;

        /* Release the stateblock */
        if(IWineD3DStateBlock_Release(stateBlock) > 0){
            FIXME("(%p) Something's still holding the Update stateblock\n",This);
        }
    }

    /* Release the buffers (with sanity checks)*/
    TRACE("Releasing the depth stencil buffer at %p\n", This->stencilBufferTarget);
    if(This->stencilBufferTarget != NULL && (IWineD3DSurface_Release(This->stencilBufferTarget) >0)){
        if(This->auto_depth_stencil_buffer != This->stencilBufferTarget)
            FIXME("(%p) Something's still holding the stencilBufferTarget\n",This);
    }
    This->stencilBufferTarget = NULL;

    TRACE("Releasing the render target at %p\n", This->render_targets[0]);
    if(IWineD3DSurface_Release(This->render_targets[0]) >0){
          /* This check is a bit silly, itshould be in swapchain_release FIXME("(%p) Something's still holding the renderTarget\n",This); */
    }
    TRACE("Setting rendertarget to NULL\n");
    This->render_targets[0] = NULL;

    if (This->auto_depth_stencil_buffer) {
        if(D3DCB_DestroyDepthStencilSurface(This->auto_depth_stencil_buffer) > 0) {
            FIXME("(%p) Something's still holding the auto depth stencil buffer\n", This);
        }
        This->auto_depth_stencil_buffer = NULL;
    }

    for(i=0; i < This->NumberOfSwapChains; i++) {
        TRACE("Releasing the implicit swapchain %d\n", i);
        if (D3DCB_DestroySwapChain(This->swapchains[i])  > 0) {
            FIXME("(%p) Something's still holding the implicit swapchain\n", This);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->swapchains);
    This->swapchains = NULL;
    This->NumberOfSwapChains = 0;

    HeapFree(GetProcessHeap(), 0, This->render_targets);
    HeapFree(GetProcessHeap(), 0, This->fbo_color_attachments);
    HeapFree(GetProcessHeap(), 0, This->draw_buffers);
    This->render_targets = NULL;
    This->fbo_color_attachments = NULL;
    This->draw_buffers = NULL;


    This->d3d_initialized = FALSE;
    return WINED3D_OK;
}

static void WINAPI IWineD3DDeviceImpl_SetFullscreen(IWineD3DDevice *iface, BOOL fullscreen) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) Setting DDraw fullscreen mode to %s\n", This, fullscreen ? "true" : "false");

    /* Setup the window for fullscreen mode */
    if(fullscreen && !This->ddraw_fullscreen) {
        IWineD3DDeviceImpl_SetupFullscreenWindow(iface, This->ddraw_window);
    } else if(!fullscreen && This->ddraw_fullscreen) {
        IWineD3DDeviceImpl_RestoreWindow(iface, This->ddraw_window);
    }

    /* DirectDraw apps can change between fullscreen and windowed mode after device creation with
     * IDirectDraw7::SetCooperativeLevel. The GDI surface implementation needs to know this.
     * DDraw doesn't necessarily have a swapchain, so we have to store the fullscreen flag
     * separately.
     */
    This->ddraw_fullscreen = fullscreen;
}

/* Enables thread safety in the wined3d device and its resources. Called by DirectDraw
 * from SetCooperativeLevel if DDSCL_MULTITHREADED is specified, and by d3d8/9 from
 * CreateDevice if D3DCREATE_MULTITHREADED is passed.
 *
 * There is no way to deactivate thread safety once it is enabled.
 */
static void WINAPI IWineD3DDeviceImpl_SetMultithreaded(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    /*For now just store the flag(needed in case of ddraw) */
    This->createParms.BehaviorFlags |= WINED3DCREATE_MULTITHREADED;

    return;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDisplayMode(IWineD3DDevice *iface, UINT iSwapChain, WINED3DDISPLAYMODE* pMode) {
    DEVMODEW devmode;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    LONG ret;
    const StaticPixelFormatDesc *formatDesc  = getFormatDescEntry(pMode->Format, NULL, NULL);
    RECT clip_rc;

    TRACE("(%p)->(%d,%p) Mode=%dx%dx@%d, %s\n", This, iSwapChain, pMode, pMode->Width, pMode->Height, pMode->RefreshRate, debug_d3dformat(pMode->Format));

    /* Resize the screen even without a window:
     * The app could have unset it with SetCooperativeLevel, but not called
     * RestoreDisplayMode first. Then the release will call RestoreDisplayMode,
     * but we don't have any hwnd
     */

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmBitsPerPel = formatDesc->bpp * 8;
    if(devmode.dmBitsPerPel == 24) devmode.dmBitsPerPel = 32;
    devmode.dmPelsWidth  = pMode->Width;
    devmode.dmPelsHeight = pMode->Height;

    devmode.dmDisplayFrequency = pMode->RefreshRate;
    if (pMode->RefreshRate != 0)  {
        devmode.dmFields |= DM_DISPLAYFREQUENCY;
    }

    /* Only change the mode if necessary */
    if( (This->ddraw_width == pMode->Width) &&
        (This->ddraw_height == pMode->Height) &&
        (This->ddraw_format == pMode->Format) &&
        (pMode->RefreshRate == 0) ) {
        return WINED3D_OK;
    }

    ret = ChangeDisplaySettingsExW(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL);
    if (ret != DISP_CHANGE_SUCCESSFUL) {
        if(devmode.dmDisplayFrequency != 0) {
            WARN("ChangeDisplaySettingsExW failed, trying without the refresh rate\n");
            devmode.dmFields &= ~DM_DISPLAYFREQUENCY;
            devmode.dmDisplayFrequency = 0;
            ret = ChangeDisplaySettingsExW(NULL, &devmode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL;
        }
        if(ret != DISP_CHANGE_SUCCESSFUL) {
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    /* Store the new values */
    This->ddraw_width = pMode->Width;
    This->ddraw_height = pMode->Height;
    This->ddraw_format = pMode->Format;

    /* Only do this with a window of course */
    if(This->ddraw_window)
      MoveWindow(This->ddraw_window, 0, 0, pMode->Width, pMode->Height, TRUE);

    /* And finally clip mouse to our screen */
    SetRect(&clip_rc, 0, 0, pMode->Width, pMode->Height);
    ClipCursor(&clip_rc);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDirect3D(IWineD3DDevice *iface, IWineD3D **ppD3D) {
   IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
   *ppD3D= This->wineD3D;
   TRACE("(%p) : wineD3D returning %p\n", This,  *ppD3D);
   IWineD3D_AddRef(*ppD3D);
   return WINED3D_OK;
}

static UINT WINAPI IWineD3DDeviceImpl_GetAvailableTextureMem(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : simulating %dMB, returning %dMB left\n",  This,
         (This->adapter->TextureRam/(1024*1024)),
         ((This->adapter->TextureRam - This->adapter->UsedTextureRam) / (1024*1024)));
    /* return simulated texture memory left */
    return (This->adapter->TextureRam - This->adapter->UsedTextureRam);
}



/*****
 * Get / Set FVF
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetFVF(IWineD3DDevice *iface, DWORD fvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Update the current state block */
    This->updateStateBlock->changed.fvf      = TRUE;

    if(This->updateStateBlock->fvf == fvf) {
        TRACE("Application is setting the old fvf over, nothing to do\n");
        return WINED3D_OK;
    }

    This->updateStateBlock->fvf              = fvf;
    TRACE("(%p) : FVF Shader FVF set to %x\n", This, fvf);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    return WINED3D_OK;
}


static HRESULT WINAPI IWineD3DDeviceImpl_GetFVF(IWineD3DDevice *iface, DWORD *pfvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : GetFVF returning %x\n", This, This->stateBlock->fvf);
    *pfvf = This->stateBlock->fvf;
    return WINED3D_OK;
}

/*****
 * Get / Set Stream Source
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride) {
        IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBuffer     *oldSrc;

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return WINED3DERR_INVALIDCALL;
    }

    oldSrc = This->updateStateBlock->streamSource[StreamNumber];
    TRACE("(%p) : StreamNo: %u, OldStream (%p), NewStream (%p), OffsetInBytes %u, NewStride %u\n", This, StreamNumber, oldSrc, pStreamData, OffsetInBytes, Stride);

    This->updateStateBlock->changed.streamSource[StreamNumber] = TRUE;

    if(oldSrc == pStreamData &&
       This->updateStateBlock->streamStride[StreamNumber] == Stride &&
       This->updateStateBlock->streamOffset[StreamNumber] == OffsetInBytes) {
       TRACE("Application is setting the old values over, nothing to do\n");
       return WINED3D_OK;
    }

    This->updateStateBlock->streamSource[StreamNumber]         = pStreamData;
    if (pStreamData) {
        This->updateStateBlock->streamStride[StreamNumber]     = Stride;
        This->updateStateBlock->streamOffset[StreamNumber]     = OffsetInBytes;
    }

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if(pStreamData) IWineD3DVertexBuffer_AddRef(pStreamData);
        if(oldSrc) IWineD3DVertexBuffer_Release(oldSrc);
        return WINED3D_OK;
    }

    /* Need to do a getParent and pass the reffs up */
    /* MSDN says ..... When an application no longer holds a references to this interface, the interface will automatically be freed.
    which suggests that we shouldn't be ref counting? and do need a _release on the stream source to reset the stream source
    so for now, just count internally   */
    if (pStreamData != NULL) {
        IWineD3DVertexBufferImpl *vbImpl = (IWineD3DVertexBufferImpl *) pStreamData;
        InterlockedIncrement(&vbImpl->bindCount);
        IWineD3DVertexBuffer_AddRef(pStreamData);
    }
    if (oldSrc != NULL) {
        InterlockedDecrement(&((IWineD3DVertexBufferImpl *) oldSrc)->bindCount);
        IWineD3DVertexBuffer_Release(oldSrc);
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer** pStream, UINT *pOffset, UINT* pStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : StreamNo: %u, Stream (%p), Offset %u, Stride %u\n", This, StreamNumber,
           This->stateBlock->streamSource[StreamNumber],
           This->stateBlock->streamOffset[StreamNumber],
           This->stateBlock->streamStride[StreamNumber]);

    if (StreamNumber >= MAX_STREAMS) {
        WARN("Stream out of range %d\n", StreamNumber);
        return WINED3DERR_INVALIDCALL;
    }
    *pStream = This->stateBlock->streamSource[StreamNumber];
    *pStride = This->stateBlock->streamStride[StreamNumber];
    if (pOffset) {
        *pOffset = This->stateBlock->streamOffset[StreamNumber];
    }

    if (*pStream != NULL) {
        IWineD3DVertexBuffer_AddRef(*pStream); /* We have created a new reference to the VB */
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT oldFlags = This->updateStateBlock->streamFlags[StreamNumber];
    UINT oldFreq = This->updateStateBlock->streamFreq[StreamNumber];

    TRACE("(%p) StreamNumber(%d), Divider(%d)\n", This, StreamNumber, Divider);
    This->updateStateBlock->streamFlags[StreamNumber] = Divider & (WINED3DSTREAMSOURCE_INSTANCEDATA  | WINED3DSTREAMSOURCE_INDEXEDDATA );

    This->updateStateBlock->changed.streamFreq[StreamNumber]  = TRUE;
    This->updateStateBlock->streamFreq[StreamNumber]          = Divider & 0x7FFFFF;

    if(This->updateStateBlock->streamFreq[StreamNumber] != oldFreq ||
       This->updateStateBlock->streamFlags[StreamNumber] != oldFlags) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSourceFreq(IWineD3DDevice *iface,  UINT StreamNumber, UINT* Divider) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) StreamNumber(%d), Divider(%p)\n", This, StreamNumber, Divider);
    *Divider = This->updateStateBlock->streamFreq[StreamNumber] | This->updateStateBlock->streamFlags[StreamNumber];

    TRACE("(%p) : returning %d\n", This, *Divider);

    return WINED3D_OK;
}

/*****
 * Get / Set & Multiply Transform
 *****/
static HRESULT  WINAPI  IWineD3DDeviceImpl_SetTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE d3dts, CONST WINED3DMATRIX* lpmatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : Transform State=%s\n", This, debug_d3dtstype(d3dts));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        This->updateStateBlock->changed.transform[d3dts] = TRUE;
        memcpy(&This->updateStateBlock->transforms[d3dts], lpmatrix, sizeof(WINED3DMATRIX));
        return WINED3D_OK;
    }

    /*
     * If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters.
     */
    if (!memcmp(&This->stateBlock->transforms[d3dts].u.m[0][0], lpmatrix, sizeof(WINED3DMATRIX))) {
        TRACE("The app is setting the same matrix over again\n");
        return WINED3D_OK;
    } else {
        conv_mat(lpmatrix, &This->stateBlock->transforms[d3dts].u.m[0][0]);
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord
       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix.
     */

    /* Capture the times we can just ignore the change for now */
    if (d3dts == WINED3DTS_VIEW) { /* handle the VIEW matrice */
        This->view_ident = !memcmp(lpmatrix, identity, 16 * sizeof(float));
        /* Handled by the state manager */
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TRANSFORM(d3dts));
    return WINED3D_OK;

}
static HRESULT WINAPI IWineD3DDeviceImpl_GetTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE State, WINED3DMATRIX* pMatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for Transform State %s\n", This, debug_d3dtstype(State));
    memcpy(pMatrix, &This->stateBlock->transforms[State], sizeof(WINED3DMATRIX));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_MultiplyTransform(IWineD3DDevice *iface, WINED3DTRANSFORMSTATETYPE State, CONST WINED3DMATRIX* pMatrix) {
    WINED3DMATRIX *mat = NULL;
    WINED3DMATRIX temp;

    /* Note: Using 'updateStateBlock' rather than 'stateblock' in the code
     * below means it will be recorded in a state block change, but it
     * works regardless where it is recorded.
     * If this is found to be wrong, change to StateBlock.
     */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : For state %s\n", This, debug_d3dtstype(State));

    if (State < HIGHEST_TRANSFORMSTATE)
    {
        mat = &This->updateStateBlock->transforms[State];
    } else {
        FIXME("Unhandled transform state!!\n");
    }

    multiply_matrix(&temp, mat, (const WINED3DMATRIX *) pMatrix);

    /* Apply change via set transform - will reapply to eg. lights this way */
    return IWineD3DDeviceImpl_SetTransform(iface, State, &temp);
}

/*****
 * Get / Set Light
 *****/
/* Note lights are real special cases. Although the device caps state only eg. 8 are supported,
   you can reference any indexes you want as long as that number max are enabled at any
   one point in time! Therefore since the indexes can be anything, we need a hashmap of them.
   However, this causes stateblock problems. When capturing the state block, I duplicate the hashmap,
   but when recording, just build a chain pretty much of commands to be replayed.                  */

static HRESULT WINAPI IWineD3DDeviceImpl_SetLight(IWineD3DDevice *iface, DWORD Index, CONST WINED3DLIGHT* pLight) {
    float rho;
    PLIGHTINFOEL *object = NULL;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : Idx(%d), pLight(%p). Hash index is %d\n", This, Index, pLight, Hi);

    /* Check the parameter range. Need for speed most wanted sets junk lights which confuse
     * the gl driver.
     */
    if(!pLight) {
        WARN("Light pointer = NULL, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    switch(pLight->Type) {
        case WINED3DLIGHT_POINT:
        case WINED3DLIGHT_SPOT:
        case WINED3DLIGHT_PARALLELPOINT:
        case WINED3DLIGHT_GLSPOT:
            /* Incorrect attenuation values can cause the gl driver to crash. Happens with Need for speed
             * most wanted
             */
            if(pLight->Attenuation0 < 0.0 || pLight->Attenuation1 < 0.0 || pLight->Attenuation2 < 0.0) {
                WARN("Attenuation is negative, returning WINED3DERR_INVALIDCALL\n");
                return WINED3DERR_INVALIDCALL;
            }
            break;

        case WINED3DLIGHT_DIRECTIONAL:
            /* Ignores attenuation */
            break;

        default:
        WARN("Light type out of range, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
        object = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(object->OriginalIndex == Index) break;
        object = NULL;
    }

    if(!object) {
        TRACE("Adding new light\n");
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if(!object) {
            ERR("Out of memory error when allocating a light\n");
            return E_OUTOFMEMORY;
        }
        list_add_head(&This->updateStateBlock->lightMap[Hi], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = Index;
        object->changed = TRUE;
    }

    /* Initialize the object */
    TRACE("Light %d setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n", Index, pLight->Type,
          pLight->Diffuse.r, pLight->Diffuse.g, pLight->Diffuse.b, pLight->Diffuse.a,
          pLight->Specular.r, pLight->Specular.g, pLight->Specular.b, pLight->Specular.a,
          pLight->Ambient.r, pLight->Ambient.g, pLight->Ambient.b, pLight->Ambient.a);
    TRACE("... Pos(%f,%f,%f), Dirn(%f,%f,%f)\n", pLight->Position.x, pLight->Position.y, pLight->Position.z,
          pLight->Direction.x, pLight->Direction.y, pLight->Direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n", pLight->Range, pLight->Falloff, pLight->Theta, pLight->Phi);

    /* Save away the information */
    memcpy(&object->OriginalParms, pLight, sizeof(WINED3DLIGHT));

    switch (pLight->Type) {
    case WINED3DLIGHT_POINT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0f;
        object->cutoff = 180.0f;
        /* FIXME: Range */
        break;

    case WINED3DLIGHT_DIRECTIONAL:
        /* Direction */
        object->lightPosn[0] = -pLight->Direction.x;
        object->lightPosn[1] = -pLight->Direction.y;
        object->lightPosn[2] = -pLight->Direction.z;
        object->lightPosn[3] = 0.0;
        object->exponent     = 0.0f;
        object->cutoff       = 180.0f;
        break;

    case WINED3DLIGHT_SPOT:
        /* Position */
        object->lightPosn[0] = pLight->Position.x;
        object->lightPosn[1] = pLight->Position.y;
        object->lightPosn[2] = pLight->Position.z;
        object->lightPosn[3] = 1.0;

        /* Direction */
        object->lightDirn[0] = pLight->Direction.x;
        object->lightDirn[1] = pLight->Direction.y;
        object->lightDirn[2] = pLight->Direction.z;
        object->lightDirn[3] = 1.0;

        /*
         * opengl-ish and d3d-ish spot lights use too different models for the
         * light "intensity" as a function of the angle towards the main light direction,
         * so we only can approximate very roughly.
         * however spot lights are rather rarely used in games (if ever used at all).
         * furthermore if still used, probably nobody pays attention to such details.
         */
        if (pLight->Falloff == 0) {
            /* Falloff = 0 is easy, because d3d's and opengl's spot light equations have the
             * falloff resp. exponent parameter as an exponent, so the spot light lighting
             * will always be 1.0 for both of them, and we don't have to care for the
             * rest of the rather complex calculation
             */
            object->exponent = 0;
        } else {
            rho = pLight->Theta + (pLight->Phi - pLight->Theta)/(2*pLight->Falloff);
            if (rho < 0.0001) rho = 0.0001f;
            object->exponent = -0.3/log(cos(rho/2));
        }
	if (object->exponent > 128.0) {
		object->exponent = 128.0;
	}
        object->cutoff = pLight->Phi*90/M_PI;

        /* FIXME: Range */
        break;

    default:
        FIXME("Unrecognized light type %d\n", pLight->Type);
    }

    /* Update the live definitions if the light is currently assigned a glIndex */
    if (object->glIndex != -1 && !This->isRecordingState) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(object->glIndex));
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetLight(IWineD3DDevice *iface, DWORD Index, WINED3DLIGHT* pLight) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;
    TRACE("(%p) : Idx(%d), pLight(%p)\n", This, Index, pLight);

    LIST_FOR_EACH(e, &This->stateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }

    if (lightInfo == NULL) {
        TRACE("Light information requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }

    memcpy(pLight, &lightInfo->OriginalParms, sizeof(WINED3DLIGHT));
    return WINED3D_OK;
}

/*****
 * Get / Set Light Enable
 *   (Note for consistency, renamed d3dx function by adding the 'set' prefix)
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetLightEnable(IWineD3DDevice *iface, DWORD Index, BOOL Enable) {
    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    struct list *e;
    TRACE("(%p) : Idx(%d), enable? %d\n", This, Index, Enable);

    /* Tests show true = 128...not clear why */
    Enable = Enable? 128: 0;

    LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }
    TRACE("Found light: %p\n", lightInfo);

    /* Special case - enabling an undefined light creates one with a strict set of parms! */
    if (lightInfo == NULL) {

        TRACE("Light enabled requested but light not defined, so defining one!\n");
        IWineD3DDeviceImpl_SetLight(iface, Index, &WINED3D_default_light);

        /* Search for it again! Should be fairly quick as near head of list */
        LIST_FOR_EACH(e, &This->updateStateBlock->lightMap[Hi]) {
            lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
            if(lightInfo->OriginalIndex == Index) break;
            lightInfo = NULL;
        }
        if (lightInfo == NULL) {
            FIXME("Adding default lights has failed dismally\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    lightInfo->enabledChanged = TRUE;
    if(!Enable) {
        if(lightInfo->glIndex != -1) {
            if(!This->isRecordingState) {
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(lightInfo->glIndex));
            }

            This->stateBlock->activeLights[lightInfo->glIndex] = NULL;
            lightInfo->glIndex = -1;
        } else {
            TRACE("Light already disabled, nothing to do\n");
        }
    } else {
        if (lightInfo->glIndex != -1) {
            /* nop */
            TRACE("Nothing to do as light was enabled\n");
        } else {
            int i;
            /* Find a free gl light */
            for(i = 0; i < This->maxConcurrentLights; i++) {
                if(This->stateBlock->activeLights[i] == NULL) {
                    This->stateBlock->activeLights[i] = lightInfo;
                    lightInfo->glIndex = i;
                    break;
                }
            }
            if(lightInfo->glIndex == -1) {
                ERR("Too many concurrently active lights\n");
                return WINED3DERR_INVALIDCALL;
            }

            /* i == lightInfo->glIndex */
            if(!This->isRecordingState) {
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_ACTIVELIGHT(i));
            }
        }
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetLightEnable(IWineD3DDevice *iface, DWORD Index,BOOL* pEnable) {

    PLIGHTINFOEL *lightInfo = NULL;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct list *e;
    UINT Hi = LIGHTMAP_HASHFUNC(Index);
    TRACE("(%p) : for idx(%d)\n", This, Index);

    LIST_FOR_EACH(e, &This->stateBlock->lightMap[Hi]) {
        lightInfo = LIST_ENTRY(e, PLIGHTINFOEL, entry);
        if(lightInfo->OriginalIndex == Index) break;
        lightInfo = NULL;
    }

    if (lightInfo == NULL) {
        TRACE("Light enabled state requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }
    /* true is 128 according to SetLightEnable */
    *pEnable = lightInfo->glIndex != -1 ? 128 : 0;
    return WINED3D_OK;
}

/*****
 * Get / Set Clip Planes
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetClipPlane(IWineD3DDevice *iface, DWORD Index, CONST float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %d, %p\n", This, Index, pPlane);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return WINED3DERR_INVALIDCALL;
    }

    This->updateStateBlock->changed.clipplane[Index] = TRUE;

    if(This->updateStateBlock->clipplane[Index][0] == pPlane[0] &&
       This->updateStateBlock->clipplane[Index][1] == pPlane[1] &&
       This->updateStateBlock->clipplane[Index][2] == pPlane[2] &&
       This->updateStateBlock->clipplane[Index][3] == pPlane[3]) {
        TRACE("Application is setting old values over, nothing to do\n");
        return WINED3D_OK;
    }

    This->updateStateBlock->clipplane[Index][0] = pPlane[0];
    This->updateStateBlock->clipplane[Index][1] = pPlane[1];
    This->updateStateBlock->clipplane[Index][2] = pPlane[2];
    This->updateStateBlock->clipplane[Index][3] = pPlane[3];

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_CLIPPLANE(Index));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetClipPlane(IWineD3DDevice *iface, DWORD Index, float *pPlane) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for idx %d\n", This, Index);

    /* Validate Index */
    if (Index >= GL_LIMITS(clipplanes)) {
        TRACE("Application has requested clipplane this device doesn't support\n");
        return WINED3DERR_INVALIDCALL;
    }

    pPlane[0] = This->stateBlock->clipplane[Index][0];
    pPlane[1] = This->stateBlock->clipplane[Index][1];
    pPlane[2] = This->stateBlock->clipplane[Index][2];
    pPlane[3] = This->stateBlock->clipplane[Index][3];
    return WINED3D_OK;
}

/*****
 * Get / Set Clip Plane Status
 *   WARNING: This code relies on the fact that D3DCLIPSTATUS8 == D3DCLIPSTATUS9
 *****/
static HRESULT  WINAPI  IWineD3DDeviceImpl_SetClipStatus(IWineD3DDevice *iface, CONST WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return WINED3DERR_INVALIDCALL;
    }
    This->updateStateBlock->clip_status.ClipUnion = pClipStatus->ClipUnion;
    This->updateStateBlock->clip_status.ClipIntersection = pClipStatus->ClipIntersection;
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetClipStatus(IWineD3DDevice *iface, WINED3DCLIPSTATUS* pClipStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    if (NULL == pClipStatus) {
      return WINED3DERR_INVALIDCALL;
    }
    pClipStatus->ClipUnion = This->updateStateBlock->clip_status.ClipUnion;
    pClipStatus->ClipIntersection = This->updateStateBlock->clip_status.ClipIntersection;
    return WINED3D_OK;
}

/*****
 * Get / Set Material
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetMaterial(IWineD3DDevice *iface, CONST WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    This->updateStateBlock->changed.material = TRUE;
    memcpy(&This->updateStateBlock->material, pMaterial, sizeof(WINED3DMATERIAL));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_MATERIAL);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetMaterial(IWineD3DDevice *iface, WINED3DMATERIAL* pMaterial) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    memcpy(pMaterial, &This->updateStateBlock->material, sizeof (WINED3DMATERIAL));
    TRACE("(%p) : Diffuse (%f,%f,%f,%f)\n", This, pMaterial->Diffuse.r, pMaterial->Diffuse.g,
        pMaterial->Diffuse.b, pMaterial->Diffuse.a);
    TRACE("(%p) : Ambient (%f,%f,%f,%f)\n", This, pMaterial->Ambient.r, pMaterial->Ambient.g,
        pMaterial->Ambient.b, pMaterial->Ambient.a);
    TRACE("(%p) : Specular (%f,%f,%f,%f)\n", This, pMaterial->Specular.r, pMaterial->Specular.g,
        pMaterial->Specular.b, pMaterial->Specular.a);
    TRACE("(%p) : Emissive (%f,%f,%f,%f)\n", This, pMaterial->Emissive.r, pMaterial->Emissive.g,
        pMaterial->Emissive.b, pMaterial->Emissive.a);
    TRACE("(%p) : Power (%f)\n", This, pMaterial->Power);

    return WINED3D_OK;
}

/*****
 * Get / Set Indices
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer* pIndexData) {
    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DIndexBuffer *oldIdxs;

    TRACE("(%p) : Setting to %p\n", This, pIndexData);
    oldIdxs = This->updateStateBlock->pIndexData;

    This->updateStateBlock->changed.indices = TRUE;
    This->updateStateBlock->pIndexData = pIndexData;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if(pIndexData) IWineD3DIndexBuffer_AddRef(pIndexData);
        if(oldIdxs) IWineD3DIndexBuffer_Release(oldIdxs);
        return WINED3D_OK;
    }

    if(oldIdxs != pIndexData) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        if(pIndexData) IWineD3DIndexBuffer_AddRef(pIndexData);
        if(oldIdxs) IWineD3DIndexBuffer_Release(oldIdxs);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetIndices(IWineD3DDevice *iface, IWineD3DIndexBuffer** ppIndexData) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    *ppIndexData = This->stateBlock->pIndexData;

    /* up ref count on ppindexdata */
    if (*ppIndexData) {
        IWineD3DIndexBuffer_AddRef(*ppIndexData);
        TRACE("(%p) index data set to %p\n", This, ppIndexData);
    }else{
        TRACE("(%p) No index data set\n", This);
    }
    TRACE("Returning %p\n", *ppIndexData);

    return WINED3D_OK;
}

/* Method to offer d3d9 a simple way to set the base vertex index without messing with the index buffer */
static HRESULT WINAPI IWineD3DDeviceImpl_SetBaseVertexIndex(IWineD3DDevice *iface, INT BaseIndex) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)->(%d)\n", This, BaseIndex);

    if(This->updateStateBlock->baseVertexIndex == BaseIndex) {
        TRACE("Application is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    This->updateStateBlock->baseVertexIndex = BaseIndex;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }
    /* The base vertex index affects the stream sources */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetBaseVertexIndex(IWineD3DDevice *iface, INT* base_index) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : base_index %p\n", This, base_index);

    *base_index = This->stateBlock->baseVertexIndex;

    TRACE("Returning %u\n", *base_index);

    return WINED3D_OK;
}

/*****
 * Get / Set Viewports
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetViewport(IWineD3DDevice *iface, CONST WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p)\n", This);
    This->updateStateBlock->changed.viewport = TRUE;
    memcpy(&This->updateStateBlock->viewport, pViewport, sizeof(WINED3DVIEWPORT));

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    TRACE("(%p) : x=%d, y=%d, wid=%d, hei=%d, minz=%f, maxz=%f\n", This,
          pViewport->X, pViewport->Y, pViewport->Width, pViewport->Height, pViewport->MinZ, pViewport->MaxZ);

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VIEWPORT);
    return WINED3D_OK;

}

static HRESULT WINAPI IWineD3DDeviceImpl_GetViewport(IWineD3DDevice *iface, WINED3DVIEWPORT* pViewport) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);
    memcpy(pViewport, &This->stateBlock->viewport, sizeof(WINED3DVIEWPORT));
    return WINED3D_OK;
}

/*****
 * Get / Set Render States
 * TODO: Verify against dx9 definitions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetRenderState(IWineD3DDevice *iface, WINED3DRENDERSTATETYPE State, DWORD Value) {

    IWineD3DDeviceImpl  *This     = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue = This->stateBlock->renderState[State];

    TRACE("(%p)->state = %s(%d), value = %d\n", This, debug_d3drenderstate(State), State, Value);

    This->updateStateBlock->changed.renderState[State] = TRUE;
    This->updateStateBlock->renderState[State] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    /* Compared here and not before the assignment to allow proper stateblock recording */
    if(Value == oldValue) {
        TRACE("Application is setting the old value over, nothing to do\n");
    } else {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(State));
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetRenderState(IWineD3DDevice *iface, WINED3DRENDERSTATETYPE State, DWORD *pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) for State %d = %d\n", This, State, This->stateBlock->renderState[State]);
    *pValue = This->stateBlock->renderState[State];
    return WINED3D_OK;
}

/*****
 * Get / Set Sampler States
 * TODO: Verify against dx9 definitions
 *****/

static HRESULT WINAPI IWineD3DDeviceImpl_SetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue;

    TRACE("(%p) : Sampler %#x, Type %s (%#x), Value %#x\n",
            This, Sampler, debug_d3dsamplerstate(Type), Type, Value);

    if (Sampler >= WINED3DVERTEXTEXTURESAMPLER0 && Sampler <= WINED3DVERTEXTEXTURESAMPLER3) {
        Sampler -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    /**
    * SetSampler is designed to allow for more than the standard up to 8 textures
    *  and Geforce has stopped supporting more than 6 standard textures in openGL.
    * So I have to use ARB for Gforce. (maybe if the sampler > 4 then use ARB?)
    *
    * http://developer.nvidia.com/object/General_FAQ.html#t6
    *
    * There are two new settings for GForce
    * the sampler one:
    * GL_MAX_TEXTURE_IMAGE_UNITS_ARB
    * and the texture one:
    * GL_MAX_TEXTURE_COORDS_ARB.
    * Ok GForce say it's ok to use glTexParameter/glGetTexParameter(...).
     ******************/

    oldValue = This->stateBlock->samplerState[Sampler][Type];
    This->updateStateBlock->samplerState[Sampler][Type]         = Value;
    This->updateStateBlock->changed.samplerState[Sampler][Type] = Value;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    if(oldValue == Value) {
        TRACE("Application is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(Sampler));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetSamplerState(IWineD3DDevice *iface, DWORD Sampler, WINED3DSAMPLERSTATETYPE Type, DWORD* Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Sampler %#x, Type %s (%#x)\n",
            This, Sampler, debug_d3dsamplerstate(Type), Type);

    if (Sampler >= WINED3DVERTEXTEXTURESAMPLER0 && Sampler <= WINED3DVERTEXTEXTURESAMPLER3) {
        Sampler -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    *Value = This->stateBlock->samplerState[Sampler][Type];
    TRACE("(%p) : Returning %#x\n", This, *Value);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetScissorRect(IWineD3DDevice *iface, CONST RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    This->updateStateBlock->changed.scissorRect = TRUE;
    if(EqualRect(&This->updateStateBlock->scissorRect, pRect)) {
        TRACE("App is setting the old scissor rectangle over, nothing to do\n");
        return WINED3D_OK;
    }
    CopyRect(&This->updateStateBlock->scissorRect, pRect);

    if(This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SCISSORRECT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetScissorRect(IWineD3DDevice *iface, RECT* pRect) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    memcpy(pRect, &This->updateStateBlock->scissorRect, sizeof(pRect));
    TRACE("(%p)Returning a Scissor Rect of %d:%d-%d:%d\n", This, pRect->left, pRect->top, pRect->right, pRect->bottom);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration* pDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DVertexDeclaration *oldDecl = This->updateStateBlock->vertexDecl;

    TRACE("(%p) : pDecl=%p\n", This, pDecl);

    This->updateStateBlock->vertexDecl = pDecl;
    This->updateStateBlock->changed.vertexDecl = TRUE;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    } else if(pDecl == oldDecl) {
        /* Checked after the assignment to allow proper stateblock recording */
        TRACE("Application is setting the old declaration over, nothing to do\n");
        return WINED3D_OK;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexDeclaration(IWineD3DDevice* iface, IWineD3DVertexDeclaration** ppDecl) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : ppDecl=%p\n", This, ppDecl);

    *ppDecl = This->stateBlock->vertexDecl;
    if (NULL != *ppDecl) IWineD3DVertexDeclaration_AddRef(*ppDecl);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader* pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexShader* oldShader = This->updateStateBlock->vertexShader;

    This->updateStateBlock->vertexShader         = pShader;
    This->updateStateBlock->changed.vertexShader = TRUE;

    if (This->isRecordingState) {
        if(pShader) IWineD3DVertexShader_AddRef(pShader);
        if(oldShader) IWineD3DVertexShader_Release(oldShader);
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    } else if(oldShader == pShader) {
        /* Checked here to allow proper stateblock recording */
        TRACE("App is setting the old shader over, nothing to do\n");
        return WINED3D_OK;
    }

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    if(pShader) IWineD3DVertexShader_AddRef(pShader);
    if(oldShader) IWineD3DVertexShader_Release(oldShader);

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VSHADER);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShader(IWineD3DDevice *iface, IWineD3DVertexShader** ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        return WINED3DERR_INVALIDCALL;
    }
    *ppShader = This->stateBlock->vertexShader;
    if( NULL != *ppShader)
        IWineD3DVertexShader_AddRef(*ppShader);

    TRACE("(%p) : returning %p\n", This, *ppShader);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    CONST BOOL *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i, cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (srcData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantB[start], srcData, cnt * sizeof(BOOL));
    for (i = 0; i < cnt; i++)
        TRACE("Set BOOL constant %u to %s\n", start + i, srcData[i]? "true":"false");

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.vertexShaderConstantsB[i] = TRUE;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    BOOL *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantB[start], cnt * sizeof(BOOL));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    CONST int *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i, cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (srcData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantI[start * 4], srcData, cnt * sizeof(int) * 4);
    for (i = 0; i < cnt; i++)
        TRACE("Set INT constant %u to { %d, %d, %d, %d }\n", start + i,
           srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.vertexShaderConstantsI[i] = TRUE;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    int *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || ((signed int) MAX_CONST_I - (signed int) start) <= (signed int) 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantI[start * 4], cnt * sizeof(int) * 4);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetVertexShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    CONST float *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i;

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    /* Specifically test start > limit to catch MAX_UINT overflows when adding start + count */
    if (srcData == NULL || start + count > GL_LIMITS(vshader_constantsF) || start > GL_LIMITS(vshader_constantsF))
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->vertexShaderConstantF[start * 4], srcData, count * sizeof(float) * 4);
    if(TRACE_ON(d3d)) {
        for (i = 0; i < count; i++)
            TRACE("Set FLOAT constant %u to { %f, %f, %f, %f }\n", start + i,
                srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);
    }

    for (i = start; i < count + start; ++i) {
        if (!This->updateStateBlock->changed.vertexShaderConstantsF[i]) {
            constants_entry *ptr = LIST_ENTRY(list_head(&This->updateStateBlock->set_vconstantsF), constants_entry, entry);
            if (!ptr || ptr->count >= sizeof(ptr->idx) / sizeof(*ptr->idx)) {
                ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(constants_entry));
                list_add_head(&This->updateStateBlock->set_vconstantsF, &ptr->entry);
            }
            ptr->idx[ptr->count++] = i;
            This->updateStateBlock->changed.vertexShaderConstantsF[i] = TRUE;
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetVertexShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    float *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, GL_LIMITS(vshader_constantsF) - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->vertexShaderConstantF[start * 4], cnt * sizeof(float) * 4);
    return WINED3D_OK;
}

static inline void markTextureStagesDirty(IWineD3DDeviceImpl *This, DWORD stage) {
    DWORD i;
    for(i = 0; i < WINED3D_HIGHEST_TEXTURE_STATE; i++) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(stage, i));
    }
}

static void device_map_stage(IWineD3DDeviceImpl *This, int stage, int unit) {
    int i = This->rev_tex_unit_map[unit];
    int j = This->texUnitMap[stage];

    This->texUnitMap[stage] = unit;
    if (i != -1 && i != stage) {
        This->texUnitMap[i] = -1;
    }

    This->rev_tex_unit_map[unit] = stage;
    if (j != -1 && j != unit) {
        This->rev_tex_unit_map[j] = -1;
    }
}

static void device_update_fixed_function_usage_map(IWineD3DDeviceImpl *This) {
    int i;

    for (i = 0; i < MAX_TEXTURES; ++i) {
        WINED3DTEXTUREOP color_op = This->stateBlock->textureState[i][WINED3DTSS_COLOROP];
        WINED3DTEXTUREOP alpha_op = This->stateBlock->textureState[i][WINED3DTSS_ALPHAOP];
        DWORD color_arg1 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG1] & WINED3DTA_SELECTMASK;
        DWORD color_arg2 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG2] & WINED3DTA_SELECTMASK;
        DWORD color_arg3 = This->stateBlock->textureState[i][WINED3DTSS_COLORARG0] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg1 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG1] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg2 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG2] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg3 = This->stateBlock->textureState[i][WINED3DTSS_ALPHAARG0] & WINED3DTA_SELECTMASK;

        if (color_op == WINED3DTOP_DISABLE) {
            /* Not used, and disable higher stages */
            while (i < MAX_TEXTURES) {
                This->fixed_function_usage_map[i] = FALSE;
                ++i;
            }
            break;
        }

        if (((color_arg1 == WINED3DTA_TEXTURE) && color_op != WINED3DTOP_SELECTARG2)
                || ((color_arg2 == WINED3DTA_TEXTURE) && color_op != WINED3DTOP_SELECTARG1)
                || ((color_arg3 == WINED3DTA_TEXTURE) && (color_op == WINED3DTOP_MULTIPLYADD || color_op == WINED3DTOP_LERP))
                || ((alpha_arg1 == WINED3DTA_TEXTURE) && alpha_op != WINED3DTOP_SELECTARG2)
                || ((alpha_arg2 == WINED3DTA_TEXTURE) && alpha_op != WINED3DTOP_SELECTARG1)
                || ((alpha_arg3 == WINED3DTA_TEXTURE) && (alpha_op == WINED3DTOP_MULTIPLYADD || alpha_op == WINED3DTOP_LERP))) {
            This->fixed_function_usage_map[i] = TRUE;
        } else {
            This->fixed_function_usage_map[i] = FALSE;
        }

        if ((color_op == WINED3DTOP_BUMPENVMAP || color_op == WINED3DTOP_BUMPENVMAPLUMINANCE) && i < MAX_TEXTURES - 1) {
            This->fixed_function_usage_map[i+1] = TRUE;
        }
    }
}

static void device_map_fixed_function_samplers(IWineD3DDeviceImpl *This) {
    int i, tex;

    device_update_fixed_function_usage_map(This);

    if (!GL_SUPPORT(NV_REGISTER_COMBINERS) || This->stateBlock->lowest_disabled_stage <= GL_LIMITS(textures)) {
        for (i = 0; i < This->stateBlock->lowest_disabled_stage; ++i) {
            if (!This->fixed_function_usage_map[i]) continue;

            if (This->texUnitMap[i] != i) {
                device_map_stage(This, i, i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
                markTextureStagesDirty(This, i);
            }
        }
        return;
    }

    /* Now work out the mapping */
    tex = 0;
    for (i = 0; i < This->stateBlock->lowest_disabled_stage; ++i) {
        if (!This->fixed_function_usage_map[i]) continue;

        if (This->texUnitMap[i] != tex) {
            device_map_stage(This, i, tex);
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
            markTextureStagesDirty(This, i);
        }

        ++tex;
    }
}

static void device_map_psamplers(IWineD3DDeviceImpl *This) {
    DWORD *sampler_tokens = ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.reg_maps.samplers;
    int i;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i) {
        if (sampler_tokens[i] && This->texUnitMap[i] != i) {
            device_map_stage(This, i, i);
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(i));
            if (i < MAX_TEXTURES) {
                markTextureStagesDirty(This, i);
            }
        }
    }
}

static BOOL device_unit_free_for_vs(IWineD3DDeviceImpl *This, DWORD *pshader_sampler_tokens, DWORD *vshader_sampler_tokens, int unit) {
    int current_mapping = This->rev_tex_unit_map[unit];

    if (current_mapping == -1) {
        /* Not currently used */
        return TRUE;
    }

    if (current_mapping < MAX_FRAGMENT_SAMPLERS) {
        /* Used by a fragment sampler */

        if (!pshader_sampler_tokens) {
            /* No pixel shader, check fixed function */
            return current_mapping >= MAX_TEXTURES || !This->fixed_function_usage_map[current_mapping];
        }

        /* Pixel shader, check the shader's sampler map */
        return !pshader_sampler_tokens[current_mapping];
    }

    /* Used by a vertex sampler */
    return !vshader_sampler_tokens[current_mapping];
}

static void device_map_vsamplers(IWineD3DDeviceImpl *This, BOOL ps) {
    DWORD *vshader_sampler_tokens = ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.reg_maps.samplers;
    DWORD *pshader_sampler_tokens = NULL;
    int start = GL_LIMITS(combined_samplers) - 1;
    int i;

    if (ps) {
        IWineD3DPixelShaderImpl *pshader = (IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader;

        /* Make sure the shader's reg_maps are up to date. This is only relevant for 1.x pixelshaders. */
        IWineD3DPixelShader_CompileShader((IWineD3DPixelShader *)pshader);
        pshader_sampler_tokens = pshader->baseShader.reg_maps.samplers;
    }

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        int vsampler_idx = i + MAX_FRAGMENT_SAMPLERS;
        if (vshader_sampler_tokens[i]) {
            if (This->texUnitMap[vsampler_idx] != -1) {
                /* Already mapped somewhere */
                continue;
            }

            while (start >= 0) {
                if (device_unit_free_for_vs(This, pshader_sampler_tokens, vshader_sampler_tokens, start)) {
                    device_map_stage(This, vsampler_idx, start);
                    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(vsampler_idx));

                    --start;
                    break;
                }

                --start;
            }
        }
    }
}

void IWineD3DDeviceImpl_FindTexUnitMap(IWineD3DDeviceImpl *This) {
    BOOL vs = use_vs(This);
    BOOL ps = use_ps(This);
    /*
     * Rules are:
     * -> Pixel shaders need a 1:1 map. In theory the shader input could be mapped too, but
     * that would be really messy and require shader recompilation
     * -> When the mapping of a stage is changed, sampler and ALL texture stage states have
     * to be reset. Because of that try to work with a 1:1 mapping as much as possible
     */
    if (ps) {
        device_map_psamplers(This);
    } else {
        device_map_fixed_function_samplers(This);
    }

    if (vs) {
        device_map_vsamplers(This, ps);
    }
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader *pShader) {
    IWineD3DDeviceImpl *This        = (IWineD3DDeviceImpl *)iface;
    IWineD3DPixelShader *oldShader  = This->updateStateBlock->pixelShader;
    This->updateStateBlock->pixelShader         = pShader;
    This->updateStateBlock->changed.pixelShader = TRUE;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
    }

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        if(pShader) IWineD3DPixelShader_AddRef(pShader);
        if(oldShader) IWineD3DPixelShader_Release(oldShader);
        return WINED3D_OK;
    }

    if(pShader == oldShader) {
        TRACE("App is setting the old pixel shader over, nothing to do\n");
        return WINED3D_OK;
    }

    if(pShader) IWineD3DPixelShader_AddRef(pShader);
    if(oldShader) IWineD3DPixelShader_Release(oldShader);

    TRACE("(%p) : setting pShader(%p)\n", This, pShader);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADER);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShader(IWineD3DDevice *iface, IWineD3DPixelShader **ppShader) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (NULL == ppShader) {
        WARN("(%p) : PShader is NULL, returning INVALIDCALL\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    *ppShader =  This->stateBlock->pixelShader;
    if (NULL != *ppShader) {
        IWineD3DPixelShader_AddRef(*ppShader);
    }
    TRACE("(%p) : returning %p\n", This, *ppShader);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    CONST BOOL *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i, cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (srcData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantB[start], srcData, cnt * sizeof(BOOL));
    for (i = 0; i < cnt; i++)
        TRACE("Set BOOL constant %u to %s\n", start + i, srcData[i]? "true":"false");

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.pixelShaderConstantsB[i] = TRUE;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantB(
    IWineD3DDevice *iface,
    UINT start,
    BOOL *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_B - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantB[start], cnt * sizeof(BOOL));
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    CONST int *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i, cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    if (srcData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantI[start * 4], srcData, cnt * sizeof(int) * 4);
    for (i = 0; i < cnt; i++)
        TRACE("Set INT constant %u to { %d, %d, %d, %d }\n", start + i,
           srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);

    for (i = start; i < cnt + start; ++i) {
        This->updateStateBlock->changed.pixelShaderConstantsI[i] = TRUE;
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantI(
    IWineD3DDevice *iface,
    UINT start,
    int *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, MAX_CONST_I - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantI[start * 4], cnt * sizeof(int) * 4);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetPixelShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    CONST float *srcData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i;

    TRACE("(iface %p, srcData %p, start %d, count %d)\n",
            iface, srcData, start, count);

    /* Specifically test start > limit to catch MAX_UINT overflows when adding start + count */
    if (srcData == NULL || start + count > GL_LIMITS(pshader_constantsF) || start > GL_LIMITS(pshader_constantsF))
        return WINED3DERR_INVALIDCALL;

    memcpy(&This->updateStateBlock->pixelShaderConstantF[start * 4], srcData, count * sizeof(float) * 4);
    if(TRACE_ON(d3d)) {
        for (i = 0; i < count; i++)
            TRACE("Set FLOAT constant %u to { %f, %f, %f, %f }\n", start + i,
                srcData[i*4], srcData[i*4+1], srcData[i*4+2], srcData[i*4+3]);
    }

    for (i = start; i < count + start; ++i) {
        if (!This->updateStateBlock->changed.pixelShaderConstantsF[i]) {
            constants_entry *ptr = LIST_ENTRY(list_head(&This->updateStateBlock->set_pconstantsF), constants_entry, entry);
            if (!ptr || ptr->count >= sizeof(ptr->idx) / sizeof(*ptr->idx)) {
                ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(constants_entry));
                list_add_head(&This->updateStateBlock->set_pconstantsF, &ptr->entry);
            }
            ptr->idx[ptr->count++] = i;
            This->updateStateBlock->changed.pixelShaderConstantsF[i] = TRUE;
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetPixelShaderConstantF(
    IWineD3DDevice *iface,
    UINT start,
    float *dstData,
    UINT count) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int cnt = min(count, GL_LIMITS(pshader_constantsF) - start);

    TRACE("(iface %p, dstData %p, start %d, count %d)\n",
            iface, dstData, start, count);

    if (dstData == NULL || cnt < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(dstData, &This->stateBlock->pixelShaderConstantF[start * 4], cnt * sizeof(float) * 4);
    return WINED3D_OK;
}

#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT
process_vertices_strided(IWineD3DDeviceImpl *This, DWORD dwDestIndex, DWORD dwCount, WineDirect3DVertexStridedData *lpStrideData, IWineD3DVertexBufferImpl *dest, DWORD dwFlags) {
    char *dest_ptr, *dest_conv = NULL, *dest_conv_addr = NULL;
    unsigned int i;
    DWORD DestFVF = dest->fvf;
    WINED3DVIEWPORT vp;
    WINED3DMATRIX mat, proj_mat, view_mat, world_mat;
    BOOL doClip;
    int numTextures;

    if (lpStrideData->u.s.normal.lpData) {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (lpStrideData->u.s.position.lpData == NULL) {
        ERR("Source has no position mask\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* We might access VBOs from this code, so hold the lock */
    ENTER_GL();

    if (dest->resource.allocatedMemory == NULL) {
        /* This may happen if we do direct locking into a vbo. Unlikely,
         * but theoretically possible(ddraw processvertices test)
         */
        dest->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), 0, dest->resource.size);
        if(!dest->resource.allocatedMemory) {
            LEAVE_GL();
            ERR("Out of memory\n");
            return E_OUTOFMEMORY;
        }
        if(dest->vbo) {
            void *src;
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, dest->vbo));
            checkGLcall("glBindBufferARB");
            src = GL_EXTCALL(glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB));
            if(src) {
                memcpy(dest->resource.allocatedMemory, src, dest->resource.size);
            }
            GL_EXTCALL(glUnmapBufferARB(GL_ARRAY_BUFFER_ARB));
            checkGLcall("glUnmapBufferARB");
        }
    }

    /* Get a pointer into the destination vbo(create one if none exists) and
     * write correct opengl data into it. It's cheap and allows us to run drawStridedFast
     */
    if(!dest->vbo && GL_SUPPORT(ARB_VERTEX_BUFFER_OBJECT)) {
        CreateVBO(dest);
    }

    if(dest->vbo) {
        unsigned char extrabytes = 0;
        /* If the destination vertex buffer has D3DFVF_XYZ position(non-rhw), native d3d writes RHW position, where the RHW
         * gets written into the 4 bytes after the Z position. In the case of a dest buffer that only has D3DFVF_XYZ data,
         * this may write 4 extra bytes beyond the area that should be written
         */
        if(DestFVF == WINED3DFVF_XYZ) extrabytes = 4;
        dest_conv_addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCount * get_flexible_vertex_size(DestFVF) + extrabytes);
        if(!dest_conv_addr) {
            ERR("Out of memory\n");
            /* Continue without storing converted vertices */
        }
        dest_conv = dest_conv_addr;
    }

    /* Should I clip?
     * a) WINED3DRS_CLIPPING is enabled
     * b) WINED3DVOP_CLIP is passed
     */
    if(This->stateBlock->renderState[WINED3DRS_CLIPPING]) {
        static BOOL warned = FALSE;
        /*
         * The clipping code is not quite correct. Some things need
         * to be checked against IDirect3DDevice3 (!), d3d8 and d3d9,
         * so disable clipping for now.
         * (The graphics in Half-Life are broken, and my processvertices
         *  test crashes with IDirect3DDevice3)
        doClip = TRUE;
         */
        doClip = FALSE;
        if(!warned) {
           warned = TRUE;
           FIXME("Clipping is broken and disabled for now\n");
        }
    } else doClip = FALSE;
    dest_ptr = ((char *) dest->resource.allocatedMemory) + dwDestIndex * get_flexible_vertex_size(DestFVF);

    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_VIEW,
                                 &view_mat);
    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_PROJECTION,
                                 &proj_mat);
    IWineD3DDevice_GetTransform( (IWineD3DDevice *) This,
                                 WINED3DTS_WORLDMATRIX(0),
                                 &world_mat);

    TRACE("View mat:\n");
    TRACE("%f %f %f %f\n", view_mat.u.s._11, view_mat.u.s._12, view_mat.u.s._13, view_mat.u.s._14);
    TRACE("%f %f %f %f\n", view_mat.u.s._21, view_mat.u.s._22, view_mat.u.s._23, view_mat.u.s._24);
    TRACE("%f %f %f %f\n", view_mat.u.s._31, view_mat.u.s._32, view_mat.u.s._33, view_mat.u.s._34);
    TRACE("%f %f %f %f\n", view_mat.u.s._41, view_mat.u.s._42, view_mat.u.s._43, view_mat.u.s._44);

    TRACE("Proj mat:\n");
    TRACE("%f %f %f %f\n", proj_mat.u.s._11, proj_mat.u.s._12, proj_mat.u.s._13, proj_mat.u.s._14);
    TRACE("%f %f %f %f\n", proj_mat.u.s._21, proj_mat.u.s._22, proj_mat.u.s._23, proj_mat.u.s._24);
    TRACE("%f %f %f %f\n", proj_mat.u.s._31, proj_mat.u.s._32, proj_mat.u.s._33, proj_mat.u.s._34);
    TRACE("%f %f %f %f\n", proj_mat.u.s._41, proj_mat.u.s._42, proj_mat.u.s._43, proj_mat.u.s._44);

    TRACE("World mat:\n");
    TRACE("%f %f %f %f\n", world_mat.u.s._11, world_mat.u.s._12, world_mat.u.s._13, world_mat.u.s._14);
    TRACE("%f %f %f %f\n", world_mat.u.s._21, world_mat.u.s._22, world_mat.u.s._23, world_mat.u.s._24);
    TRACE("%f %f %f %f\n", world_mat.u.s._31, world_mat.u.s._32, world_mat.u.s._33, world_mat.u.s._34);
    TRACE("%f %f %f %f\n", world_mat.u.s._41, world_mat.u.s._42, world_mat.u.s._43, world_mat.u.s._44);

    /* Get the viewport */
    IWineD3DDevice_GetViewport( (IWineD3DDevice *) This, &vp);
    TRACE("Viewport: X=%d, Y=%d, Width=%d, Height=%d, MinZ=%f, MaxZ=%f\n",
          vp.X, vp.Y, vp.Width, vp.Height, vp.MinZ, vp.MaxZ);

    multiply_matrix(&mat,&view_mat,&world_mat);
    multiply_matrix(&mat,&proj_mat,&mat);

    numTextures = (DestFVF & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    for (i = 0; i < dwCount; i+= 1) {
        unsigned int tex_index;

        if ( ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZ ) ||
             ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) ) {
            /* The position first */
            float *p =
              (float *) (((char *) lpStrideData->u.s.position.lpData) + i * lpStrideData->u.s.position.dwStride);
            float x, y, z, rhw;
            TRACE("In: ( %06.2f %06.2f %06.2f )\n", p[0], p[1], p[2]);

            /* Multiplication with world, view and projection matrix */
            x =   (p[0] * mat.u.s._11) + (p[1] * mat.u.s._21) + (p[2] * mat.u.s._31) + (1.0 * mat.u.s._41);
            y =   (p[0] * mat.u.s._12) + (p[1] * mat.u.s._22) + (p[2] * mat.u.s._32) + (1.0 * mat.u.s._42);
            z =   (p[0] * mat.u.s._13) + (p[1] * mat.u.s._23) + (p[2] * mat.u.s._33) + (1.0 * mat.u.s._43);
            rhw = (p[0] * mat.u.s._14) + (p[1] * mat.u.s._24) + (p[2] * mat.u.s._34) + (1.0 * mat.u.s._44);

            TRACE("x=%f y=%f z=%f rhw=%f\n", x, y, z, rhw);

            /* WARNING: The following things are taken from d3d7 and were not yet checked
             * against d3d8 or d3d9!
             */

            /* Clipping conditions: From
             * http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/directx9_c/directx/graphics/programmingguide/fixedfunction/viewportsclipping/clippingvolumes.asp
             *
             * A vertex is clipped if it does not match the following requirements
             * -rhw < x <= rhw
             * -rhw < y <= rhw
             *    0 < z <= rhw
             *    0 < rhw ( Not in d3d7, but tested in d3d7)
             *
             * If clipping is on is determined by the D3DVOP_CLIP flag in D3D7, and
             * by the D3DRS_CLIPPING in D3D9(according to the msdn, not checked)
             *
             */

            if( !doClip ||
                ( (-rhw -eps < x) && (-rhw -eps < y) && ( -eps < z) &&
                  (x <= rhw + eps) && (y <= rhw + eps ) && (z <= rhw + eps) && 
                  ( rhw > eps ) ) ) {

                /* "Normal" viewport transformation (not clipped)
                 * 1) The values are divided by rhw
                 * 2) The y axis is negative, so multiply it with -1
                 * 3) Screen coordinates go from -(Width/2) to +(Width/2) and
                 *    -(Height/2) to +(Height/2). The z range is MinZ to MaxZ
                 * 4) Multiply x with Width/2 and add Width/2
                 * 5) The same for the height
                 * 6) Add the viewpoint X and Y to the 2D coordinates and
                 *    The minimum Z value to z
                 * 7) rhw = 1 / rhw Reciprocal of Homogeneous W....
                 *
                 * Well, basically it's simply a linear transformation into viewport
                 * coordinates
                 */

                x /= rhw;
                y /= rhw;
                z /= rhw;

                y *= -1;

                x *= vp.Width / 2;
                y *= vp.Height / 2;
                z *= vp.MaxZ - vp.MinZ;

                x += vp.Width / 2 + vp.X;
                y += vp.Height / 2 + vp.Y;
                z += vp.MinZ;

                rhw = 1 / rhw;
            } else {
                /* That vertex got clipped
                 * Contrary to OpenGL it is not dropped completely, it just
                 * undergoes a different calculation.
                 */
                TRACE("Vertex got clipped\n");
                x += rhw;
                y += rhw;

                x  /= 2;
                y  /= 2;

                /* Msdn mentions that Direct3D9 keeps a list of clipped vertices
                 * outside of the main vertex buffer memory. That needs some more
                 * investigation...
                 */
            }

            TRACE("Writing (%f %f %f) %f\n", x, y, z, rhw);


            ( (float *) dest_ptr)[0] = x;
            ( (float *) dest_ptr)[1] = y;
            ( (float *) dest_ptr)[2] = z;
            ( (float *) dest_ptr)[3] = rhw; /* SIC, see ddraw test! */

            dest_ptr += 3 * sizeof(float);

            if((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW) {
                dest_ptr += sizeof(float);
            }

            if(dest_conv) {
                float w = 1 / rhw;
                ( (float *) dest_conv)[0] = x * w;
                ( (float *) dest_conv)[1] = y * w;
                ( (float *) dest_conv)[2] = z * w;
                ( (float *) dest_conv)[3] = w;

                dest_conv += 3 * sizeof(float);

                if((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW) {
                    dest_conv += sizeof(float);
                }
            }
        }
        if (DestFVF & WINED3DFVF_PSIZE) {
            dest_ptr += sizeof(DWORD);
            if(dest_conv) dest_conv += sizeof(DWORD);
        }
        if (DestFVF & WINED3DFVF_NORMAL) {
            float *normal =
              (float *) (((float *) lpStrideData->u.s.normal.lpData) + i * lpStrideData->u.s.normal.dwStride);
            /* AFAIK this should go into the lighting information */
            FIXME("Didn't expect the destination to have a normal\n");
            copy_and_next(dest_ptr, normal, 3 * sizeof(float));
            if(dest_conv) {
                copy_and_next(dest_conv, normal, 3 * sizeof(float));
            }
        }

        if (DestFVF & WINED3DFVF_DIFFUSE) {
            DWORD *color_d = 
              (DWORD *) (((char *) lpStrideData->u.s.diffuse.lpData) + i * lpStrideData->u.s.diffuse.dwStride);
            if(!color_d) {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No diffuse color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xffffffff;
                dest_ptr += sizeof(DWORD);

                if(dest_conv) {
                    *( (DWORD *) dest_conv) = 0xffffffff;
                    dest_conv += sizeof(DWORD);
                }
            }
            else {
                copy_and_next(dest_ptr, color_d, sizeof(DWORD));
                if(dest_conv) {
                    *( (DWORD *) dest_conv)  = (*color_d & 0xff00ff00)      ; /* Alpha + green */
                    *( (DWORD *) dest_conv) |= (*color_d & 0x00ff0000) >> 16; /* Red */
                    *( (DWORD *) dest_conv) |= (*color_d & 0xff0000ff) << 16; /* Blue */
                    dest_conv += sizeof(DWORD);
                }
            }
        }

        if (DestFVF & WINED3DFVF_SPECULAR) { 
            /* What's the color value in the feedback buffer? */
            DWORD *color_s = 
              (DWORD *) (((char *) lpStrideData->u.s.specular.lpData) + i * lpStrideData->u.s.specular.dwStride);
            if(!color_s) {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No specular color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xFF000000;
                dest_ptr += sizeof(DWORD);

                if(dest_conv) {
                    *( (DWORD *) dest_conv) = 0xFF000000;
                    dest_conv += sizeof(DWORD);
                }
            }
            else {
                copy_and_next(dest_ptr, color_s, sizeof(DWORD));
                if(dest_conv) {
                    *( (DWORD *) dest_conv)  = (*color_s & 0xff00ff00)      ; /* Alpha + green */
                    *( (DWORD *) dest_conv) |= (*color_s & 0x00ff0000) >> 16; /* Red */
                    *( (DWORD *) dest_conv) |= (*color_s & 0xff0000ff) << 16; /* Blue */
                    dest_conv += sizeof(DWORD);
                }
            }
        }

        for (tex_index = 0; tex_index < numTextures; tex_index++) {
            float *tex_coord =
              (float *) (((char *) lpStrideData->u.s.texCoords[tex_index].lpData) + 
                            i * lpStrideData->u.s.texCoords[tex_index].dwStride);
            if(!tex_coord) {
                ERR("No source texture, but destination requests one\n");
                dest_ptr+=GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
                if(dest_conv) dest_conv += GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
            }
            else {
                copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
                if(dest_conv) {
                    copy_and_next(dest_conv, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
                }
            }
        }
    }

    if(dest_conv) {
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, dest->vbo));
        checkGLcall("glBindBufferARB(GL_ARRAY_BUFFER_ARB)");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, dwDestIndex * get_flexible_vertex_size(DestFVF),
                                      dwCount * get_flexible_vertex_size(DestFVF),
                                      dest_conv_addr));
        checkGLcall("glBufferSubDataARB(GL_ARRAY_BUFFER_ARB)");
        HeapFree(GetProcessHeap(), 0, dest_conv_addr);
    }

    LEAVE_GL();

    return WINED3D_OK;
}
#undef copy_and_next

static HRESULT WINAPI IWineD3DDeviceImpl_ProcessVertices(IWineD3DDevice *iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IWineD3DVertexBuffer* pDestBuffer, IWineD3DVertexDeclaration* pVertexDecl, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineDirect3DVertexStridedData strided;
    BOOL vbo = FALSE, streamWasUP = This->stateBlock->streamIsUP;
    TRACE("(%p)->(%d,%d,%d,%p,%p,%d\n", This, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);

    if(pVertexDecl) {
        ERR("Output vertex declaration not implemented yet\n");
    }

    /* Need any context to write to the vbo. In a non-multithreaded environment a context is there anyway,
     * and this call is quite performance critical, so don't call needlessly
     */
    if(This->createParms.BehaviorFlags & WINED3DCREATE_MULTITHREADED) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }

    /* ProcessVertices reads from vertex buffers, which have to be assigned. DrawPrimitive and DrawPrimitiveUP
     * control the streamIsUP flag, thus restore it afterwards.
     */
    This->stateBlock->streamIsUP = FALSE;
    memset(&strided, 0, sizeof(strided));
    primitiveDeclarationConvertToStridedData(iface, FALSE, &strided, &vbo);
    This->stateBlock->streamIsUP = streamWasUP;

    if(vbo || SrcStartIndex) {
        unsigned int i;
        /* ProcessVertices can't convert FROM a vbo, and vertex buffers used to source into ProcesVerticse are
         * unlikely to ever be used for drawing. Release vbos in those buffers and fix up the strided structure
         *
         * Also get the start index in, but only loop over all elements if there's something to add at all.
         */
#define FIXSRC(type) \
        if(strided.u.s.type.VBO) { \
            IWineD3DVertexBufferImpl *vb = (IWineD3DVertexBufferImpl *) This->stateBlock->streamSource[strided.u.s.type.streamNo]; \
            strided.u.s.type.VBO = 0; \
            strided.u.s.type.lpData = (BYTE *) ((unsigned long) strided.u.s.type.lpData + (unsigned long) vb->resource.allocatedMemory); \
            ENTER_GL(); \
            GL_EXTCALL(glDeleteBuffersARB(1, &vb->vbo)); \
            vb->vbo = 0; \
            LEAVE_GL(); \
        } \
        if(strided.u.s.type.lpData) { \
            strided.u.s.type.lpData += strided.u.s.type.dwStride * SrcStartIndex; \
        }
        FIXSRC(position);
        FIXSRC(blendWeights);
        FIXSRC(blendMatrixIndices);
        FIXSRC(normal);
        FIXSRC(pSize);
        FIXSRC(diffuse);
        FIXSRC(specular);
        for(i = 0; i < WINED3DDP_MAXTEXCOORD; i++) {
            FIXSRC(texCoords[i]);
        }
        FIXSRC(position2);
        FIXSRC(normal2);
        FIXSRC(tangent);
        FIXSRC(binormal);
        FIXSRC(tessFactor);
        FIXSRC(fog);
        FIXSRC(depth);
        FIXSRC(sample);
#undef FIXSRC
    }

    return process_vertices_strided(This, DestIndex, VertexCount, &strided, (IWineD3DVertexBufferImpl *) pDestBuffer, Flags);
}

/*****
 * Get / Set Texture Stage States
 * TODO: Verify against dx9 definitions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    DWORD oldValue = This->updateStateBlock->textureState[Stage][Type];

    TRACE("(%p) : Stage=%d, Type=%s(%d), Value=%d\n", This, Stage, debug_d3dtexturestate(Type), Type, Value);

    if (Stage >= MAX_TEXTURES) {
        WARN("Attempting to set stage %u which is higher than the max stage %u, ignoring\n", Stage, MAX_TEXTURES - 1);
        return WINED3D_OK;
    }

    This->updateStateBlock->changed.textureState[Stage][Type] = TRUE;
    This->updateStateBlock->textureState[Stage][Type]         = Value;

    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    /* Checked after the assignments to allow proper stateblock recording */
    if(oldValue == Value) {
        TRACE("App is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    if(Stage > This->stateBlock->lowest_disabled_stage &&
       StateTable[STATE_TEXTURESTAGE(0, Type)].representative == STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP)) {
        /* Colorop change above lowest disabled stage? That won't change anything in the gl setup
         * Changes in other states are important on disabled stages too
         */
        return WINED3D_OK;
    }

    if(Type == WINED3DTSS_COLOROP) {
        int i;

        if(Value == WINED3DTOP_DISABLE && oldValue != WINED3DTOP_DISABLE) {
            /* Previously enabled stage disabled now. Make sure to dirtify all enabled stages above Stage,
             * they have to be disabled
             *
             * The current stage is dirtified below.
             */
            for(i = Stage + 1; i < This->stateBlock->lowest_disabled_stage; i++) {
                TRACE("Additionally dirtifying stage %d\n", i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            This->stateBlock->lowest_disabled_stage = Stage;
            TRACE("New lowest disabled: %d\n", Stage);
        } else if(Value != WINED3DTOP_DISABLE && oldValue == WINED3DTOP_DISABLE) {
            /* Previously disabled stage enabled. Stages above it may need enabling
             * stage must be lowest_disabled_stage here, if it's bigger success is returned above,
             * and stages below the lowest disabled stage can't be enabled(because they are enabled already).
             *
             * Again stage Stage doesn't need to be dirtified here, it is handled below.
             */

            for(i = Stage + 1; i < GL_LIMITS(texture_stages); i++) {
                if(This->updateStateBlock->textureState[i][WINED3DTSS_COLOROP] == WINED3DTOP_DISABLE) {
                    break;
                }
                TRACE("Additionally dirtifying stage %d due to enable\n", i);
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            }
            This->stateBlock->lowest_disabled_stage = i;
            TRACE("New lowest disabled: %d\n", i);
        }
        if(GL_SUPPORT(NV_REGISTER_COMBINERS) && !This->stateBlock->pixelShader) {
            /* TODO: Built a stage -> texture unit mapping for register combiners */
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, Type));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetTextureStageState(IWineD3DDevice *iface, DWORD Stage, WINED3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : requesting Stage %d, Type %d getting %d\n", This, Stage, Type, This->updateStateBlock->textureState[Stage][Type]);
    *pValue = This->updateStateBlock->textureState[Stage][Type];
    return WINED3D_OK;
}

/*****
 * Get / Set Texture
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_SetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture* pTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DBaseTexture *oldTexture;

    TRACE("(%p) : Stage %#x, Texture %p\n", This, Stage, pTexture);

    if (Stage >= WINED3DVERTEXTEXTURESAMPLER0 && Stage <= WINED3DVERTEXTEXTURESAMPLER3) {
        Stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    oldTexture = This->updateStateBlock->textures[Stage];

    if(pTexture != NULL) {
        /* SetTexture isn't allowed on textures in WINED3DPOOL_SCRATCH; 
         */
        if(((IWineD3DTextureImpl*)pTexture)->resource.pool == WINED3DPOOL_SCRATCH) {
            WARN("(%p) Attempt to set scratch texture rejected\n", pTexture);
            return WINED3DERR_INVALIDCALL;
        }
        This->stateBlock->textureDimensions[Stage] = IWineD3DBaseTexture_GetTextureDimensions(pTexture);
    }

    TRACE("GL_LIMITS %d\n",GL_LIMITS(sampler_stages));
    TRACE("(%p) : oldtexture(%p)\n", This,oldTexture);

    This->updateStateBlock->changed.textures[Stage] = TRUE;
    TRACE("(%p) : setting new texture to %p\n", This, pTexture);
    This->updateStateBlock->textures[Stage]         = pTexture;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    if(oldTexture == pTexture) {
        TRACE("App is setting the same texture again, nothing to do\n");
        return WINED3D_OK;
    }

    /** NOTE: MSDN says that setTexture increases the reference count,
    * and that the application must set the texture back to null (or have a leaky application),
    * This means we should pass the refcount up to the parent
     *******************************/
    if (NULL != This->updateStateBlock->textures[Stage]) {
        IWineD3DBaseTextureImpl *new = (IWineD3DBaseTextureImpl *) This->updateStateBlock->textures[Stage];
        ULONG bindCount = InterlockedIncrement(&new->baseTexture.bindCount);

        IWineD3DBaseTexture_AddRef(This->updateStateBlock->textures[Stage]);
        if(oldTexture == NULL && Stage < MAX_TEXTURES) {
            /* The source arguments for color and alpha ops have different meanings when a NULL texture is bound,
             * so the COLOROP and ALPHAOP have to be dirtified.
             */
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_COLOROP));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_ALPHAOP));
        }
        if(bindCount == 1) {
            new->baseTexture.sampler = Stage;
        }
        /* More than one assignment? Doesn't matter, we only need one gl texture unit to use for uploading */

    }

    if (NULL != oldTexture) {
        IWineD3DBaseTextureImpl *old = (IWineD3DBaseTextureImpl *) oldTexture;
        LONG bindCount = InterlockedDecrement(&old->baseTexture.bindCount);

        IWineD3DBaseTexture_Release(oldTexture);
        if(pTexture == NULL && Stage < MAX_TEXTURES) {
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_COLOROP));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_TEXTURESTAGE(Stage, WINED3DTSS_ALPHAOP));
        }

        if(bindCount && old->baseTexture.sampler == Stage) {
            int i;
            /* Have to do a search for the other sampler(s) where the texture is bound to
             * Shouldn't happen as long as apps bind a texture only to one stage
             */
            TRACE("Searcing for other sampler / stage id where the texture is bound to\n");
            for(i = 0; i < MAX_COMBINED_SAMPLERS; i++) {
                if(This->updateStateBlock->textures[i] == oldTexture) {
                    old->baseTexture.sampler = i;
                    break;
                }
            }
        }
    }

    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(Stage));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetTexture(IWineD3DDevice *iface, DWORD Stage, IWineD3DBaseTexture** ppTexture) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Stage %#x, ppTexture %p\n", This, Stage, ppTexture);

    if (Stage >= WINED3DVERTEXTEXTURESAMPLER0 && Stage <= WINED3DVERTEXTEXTURESAMPLER3) {
        Stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);
    }

    *ppTexture=This->stateBlock->textures[Stage];
    if (*ppTexture)
        IWineD3DBaseTexture_AddRef(*ppTexture);

    TRACE("(%p) : Returning %p\n", This, *ppTexture);

    return WINED3D_OK;
}

/*****
 * Get Back Buffer
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_GetBackBuffer(IWineD3DDevice *iface, UINT iSwapChain, UINT BackBuffer, WINED3DBACKBUFFER_TYPE Type,
                                                IWineD3DSurface **ppBackBuffer) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) : BackBuf %d Type %d SwapChain %d returning %p\n", This, BackBuffer, Type, iSwapChain, *ppBackBuffer);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, &swapChain);
    if (hr == WINED3D_OK) {
        hr = IWineD3DSwapChain_GetBackBuffer(swapChain, BackBuffer, Type, ppBackBuffer);
            IWineD3DSwapChain_Release(swapChain);
    } else {
        *ppBackBuffer = NULL;
    }
    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDeviceCaps(IWineD3DDevice *iface, WINED3DCAPS* pCaps) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WARN("(%p) : stub, calling idirect3d for now\n", This);
    return IWineD3D_GetDeviceCaps(This->wineD3D, This->adapterNo, This->devType, pCaps);
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetDisplayMode(IWineD3DDevice *iface, UINT iSwapChain, WINED3DDISPLAYMODE* pMode) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    if(iSwapChain > 0) {
        hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
        if (hr == WINED3D_OK) {
            hr = IWineD3DSwapChain_GetDisplayMode(swapChain, pMode);
            IWineD3DSwapChain_Release(swapChain);
        } else {
            FIXME("(%p) Error getting display mode\n", This);
        }
    } else {
        /* Don't read the real display mode,
           but return the stored mode instead. X11 can't change the color
           depth, and some apps are pretty angry if they SetDisplayMode from
           24 to 16 bpp and find out that GetDisplayMode still returns 24 bpp

           Also don't relay to the swapchain because with ddraw it's possible
           that there isn't a swapchain at all */
        pMode->Width = This->ddraw_width;
        pMode->Height = This->ddraw_height;
        pMode->Format = This->ddraw_format;
        pMode->RefreshRate = 0;
        hr = WINED3D_OK;
    }

    return hr;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetHWND(IWineD3DDevice *iface, HWND hWnd) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)->(%p)\n", This, hWnd);

    if(This->ddraw_fullscreen) {
        if(This->ddraw_window && This->ddraw_window != hWnd) {
            IWineD3DDeviceImpl_RestoreWindow(iface, This->ddraw_window);
        }
        if(hWnd && This->ddraw_window != hWnd) {
            IWineD3DDeviceImpl_SetupFullscreenWindow(iface, hWnd);
        }
    }

    This->ddraw_window = hWnd;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_GetHWND(IWineD3DDevice *iface, HWND *hWnd) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)->(%p)\n", This, hWnd);

    *hWnd = This->ddraw_window;
    return WINED3D_OK;
}

/*****
 * Stateblock related functions
 *****/

static HRESULT WINAPI IWineD3DDeviceImpl_BeginStateBlock(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
    HRESULT temp_result;
    int i;

    TRACE("(%p)\n", This);
    
    if (This->isRecordingState) {
        return WINED3DERR_INVALIDCALL;
    }
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DStateBlockImpl));
    if (NULL == object ) {
        FIXME("(%p)Error allocating memory for stateblock\n", This);
        return E_OUTOFMEMORY;
    }
    TRACE("(%p) created object %p\n", This, object);
    object->wineD3DDevice= This;
    /** FIXME: object->parent       = parent; **/
    object->parent       = NULL;
    object->blockType    = WINED3DSBT_RECORDED;
    object->ref          = 1;
    object->lpVtbl       = &IWineD3DStateBlock_Vtbl;

    for(i = 0; i < LIGHTMAP_SIZE; i++) {
        list_init(&object->lightMap[i]);
    }

    temp_result = allocate_shader_constants(object);
    if (WINED3D_OK != temp_result)
        return temp_result;

    IWineD3DStateBlock_Release((IWineD3DStateBlock*)This->updateStateBlock);
    This->updateStateBlock = object;
    This->isRecordingState = TRUE;

    TRACE("(%p) recording stateblock %p\n",This , object);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_EndStateBlock(IWineD3DDevice *iface, IWineD3DStateBlock** ppStateBlock) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i, j;
    IWineD3DStateBlockImpl *object = This->updateStateBlock;

    if (!This->isRecordingState) {
        FIXME("(%p) not recording! returning error\n", This);
        *ppStateBlock = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    for(i = 1; i <= WINEHIGHEST_RENDER_STATE; i++) {
        if(object->changed.renderState[i]) {
            object->contained_render_states[object->num_contained_render_states] = i;
            object->num_contained_render_states++;
        }
    }
    for(i = 1; i <= HIGHEST_TRANSFORMSTATE; i++) {
        if(object->changed.transform[i]) {
            object->contained_transform_states[object->num_contained_transform_states] = i;
            object->num_contained_transform_states++;
        }
    }
    for(i = 0; i < GL_LIMITS(vshader_constantsF); i++) {
        if(object->changed.vertexShaderConstantsF[i]) {
            object->contained_vs_consts_f[object->num_contained_vs_consts_f] = i;
            object->num_contained_vs_consts_f++;
        }
    }
    for(i = 0; i < MAX_CONST_I; i++) {
        if(object->changed.vertexShaderConstantsI[i]) {
            object->contained_vs_consts_i[object->num_contained_vs_consts_i] = i;
            object->num_contained_vs_consts_i++;
        }
    }
    for(i = 0; i < MAX_CONST_B; i++) {
        if(object->changed.vertexShaderConstantsB[i]) {
            object->contained_vs_consts_b[object->num_contained_vs_consts_b] = i;
            object->num_contained_vs_consts_b++;
        }
    }
    for(i = 0; i < MAX_CONST_I; i++) {
        if(object->changed.pixelShaderConstantsI[i]) {
            object->contained_ps_consts_i[object->num_contained_ps_consts_i] = i;
            object->num_contained_ps_consts_i++;
        }
    }
    for(i = 0; i < MAX_CONST_B; i++) {
        if(object->changed.pixelShaderConstantsB[i]) {
            object->contained_ps_consts_b[object->num_contained_ps_consts_b] = i;
            object->num_contained_ps_consts_b++;
        }
    }
    for(i = 0; i < MAX_TEXTURES; i++) {
        for(j = 1; j <= WINED3D_HIGHEST_TEXTURE_STATE; j++) {
            if(object->changed.textureState[i][j]) {
                object->contained_tss_states[object->num_contained_tss_states].stage = i;
                object->contained_tss_states[object->num_contained_tss_states].state = j;
                object->num_contained_tss_states++;
            }
        }
    }
    for(i = 0; i < MAX_COMBINED_SAMPLERS; i++){
        for (j = 1; j < WINED3D_HIGHEST_SAMPLER_STATE; j++) {
            if(object->changed.samplerState[i][j]) {
                object->contained_sampler_states[object->num_contained_sampler_states].stage = i;
                object->contained_sampler_states[object->num_contained_sampler_states].state = j;
                object->num_contained_sampler_states++;
            }
        }
    }

    *ppStateBlock = (IWineD3DStateBlock*) object;
    This->isRecordingState = FALSE;
    This->updateStateBlock = This->stateBlock;
    IWineD3DStateBlock_AddRef((IWineD3DStateBlock*)This->updateStateBlock);
    /* IWineD3DStateBlock_AddRef(*ppStateBlock); don't need to do this, since we should really just release UpdateStateBlock first */
    TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, *ppStateBlock);
    return WINED3D_OK;
}

/*****
 * Scene related functions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_BeginScene(IWineD3DDevice *iface) {
    /* At the moment we have no need for any functionality at the beginning
       of a scene                                                          */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    if(This->inScene) {
        TRACE("Already in Scene, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    This->inScene = TRUE;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_EndScene(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p)\n", This);

    if(!This->inScene) {
        TRACE("Not in scene, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(This->createParms.BehaviorFlags & WINED3DCREATE_MULTITHREADED) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }
    /* We only have to do this if we need to read the, swapbuffers performs a flush for us */
    ENTER_GL();
    glFlush();
    checkGLcall("glFlush");
    LEAVE_GL();

    This->inScene = FALSE;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Present(IWineD3DDevice *iface,
                                          CONST RECT* pSourceRect, CONST RECT* pDestRect,
                                          HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain = NULL;
    int i;
    int swapchains = IWineD3DDeviceImpl_GetNumberOfSwapChains(iface);

    TRACE("(%p) Presenting the frame\n", This);

    for(i = 0 ; i < swapchains ; i ++) {

        IWineD3DDeviceImpl_GetSwapChain(iface, i , (IWineD3DSwapChain **)&swapChain);
        TRACE("presentinng chain %d, %p\n", i, swapChain);
        IWineD3DSwapChain_Present(swapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
        IWineD3DSwapChain_Release(swapChain);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Clear(IWineD3DDevice *iface, DWORD Count, CONST WINED3DRECT* pRects,
                                        DWORD Flags, WINED3DCOLOR Color, float Z, DWORD Stencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *target = (IWineD3DSurfaceImpl *)This->render_targets[0];

    GLbitfield     glMask = 0;
    unsigned int   i;
    CONST WINED3DRECT* curRect;

    TRACE("(%p) Count (%d), pRects (%p), Flags (%x), Color (0x%08x), Z (%f), Stencil (%d)\n", This,
          Count, pRects, Flags, Color, Z, Stencil);

    if(Flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL) && This->stencilBufferTarget == NULL) {
        WARN("Clearing depth and/or stencil without a depth stencil buffer attached, returning WINED3DERR_INVALIDCALL\n");
        /* TODO: What about depth stencil buffers without stencil bits? */
        return WINED3DERR_INVALIDCALL;
    }

    /* This is for offscreen rendering as well as for multithreading, thus activate the set render target
     * and not the last active one.
     */
    ActivateContext(This, This->render_targets[0], CTXUSAGE_CLEAR);
    ENTER_GL();

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        apply_fbo_state(iface);
    }

    if (Count > 0 && pRects) {
        curRect = pRects;
    } else {
        curRect = NULL;
    }

    /* Only set the values up once, as they are not changing */
    if (Flags & WINED3DCLEAR_STENCIL) {
        glClearStencil(Stencil);
        checkGLcall("glClearStencil");
        glMask = glMask | GL_STENCIL_BUFFER_BIT;
        glStencilMask(0xFFFFFFFF);
    }

    if (Flags & WINED3DCLEAR_ZBUFFER) {
        glDepthMask(GL_TRUE);
        glClearDepth(Z);
        checkGLcall("glClearDepth");
        glMask = glMask | GL_DEPTH_BUFFER_BIT;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ZWRITEENABLE));
    }

    if (Flags & WINED3DCLEAR_TARGET) {
        TRACE("Clearing screen with glClear to color %x\n", Color);
        glClearColor(D3DCOLOR_R(Color),
                     D3DCOLOR_G(Color),
                     D3DCOLOR_B(Color),
                     D3DCOLOR_A(Color));
        checkGLcall("glClearColor");

        /* Clear ALL colors! */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glMask = glMask | GL_COLOR_BUFFER_BIT;
    }

    if (!curRect) {
        /* In drawable flag is set below */

        if (This->render_offscreen) {
            glScissor(This->stateBlock->viewport.X,
                       This->stateBlock->viewport.Y,
                       This->stateBlock->viewport.Width,
                       This->stateBlock->viewport.Height);
        } else {
            glScissor(This->stateBlock->viewport.X,
                      (((IWineD3DSurfaceImpl *)This->render_targets[0])->currentDesc.Height -
                      (This->stateBlock->viewport.Y + This->stateBlock->viewport.Height)),
                       This->stateBlock->viewport.Width,
                       This->stateBlock->viewport.Height);
        }
        checkGLcall("glScissor");
        glClear(glMask);
        checkGLcall("glClear");
    } else {
        if(!(target->Flags & SFLAG_INDRAWABLE) &&
           !(wined3d_settings.offscreen_rendering_mode == ORM_FBO && This->render_offscreen && target->Flags & SFLAG_INTEXTURE)) {

            if(curRect[0].x1 > 0 || curRect[0].y1 > 0 ||
               curRect[0].x2 < target->currentDesc.Width ||
               curRect[0].y2 < target->currentDesc.Height) {
                TRACE("Partial clear, and surface not in drawable. Blitting texture to drawable\n");
                IWineD3DSurface_LoadLocation((IWineD3DSurface *) target, SFLAG_INDRAWABLE, NULL);
            }
        }

        /* Now process each rect in turn */
        for (i = 0; i < Count; i++) {
            /* Note gl uses lower left, width/height */
            TRACE("(%p) %p Rect=(%d,%d)->(%d,%d) glRect=(%d,%d), len=%d, hei=%d\n", This, curRect,
                  curRect[i].x1, curRect[i].y1, curRect[i].x2, curRect[i].y2,
                  curRect[i].x1, (target->currentDesc.Height - curRect[i].y2),
                  curRect[i].x2 - curRect[i].x1, curRect[i].y2 - curRect[i].y1);

            /* Tests show that rectangles where x1 > x2 or y1 > y2 are ignored silently.
             * The rectangle is not cleared, no error is returned, but further rectanlges are
             * still cleared if they are valid
             */
            if(curRect[i].x1 > curRect[i].x2 || curRect[i].y1 > curRect[i].y2) {
                TRACE("Rectangle with negative dimensions, ignoring\n");
                continue;
            }

            if(This->render_offscreen) {
                glScissor(curRect[i].x1, curRect[i].y1,
                          curRect[i].x2 - curRect[i].x1, curRect[i].y2 - curRect[i].y1);
            } else {
                glScissor(curRect[i].x1, target->currentDesc.Height - curRect[i].y2,
                          curRect[i].x2 - curRect[i].x1, curRect[i].y2 - curRect[i].y1);
            }
            checkGLcall("glScissor");

            glClear(glMask);
            checkGLcall("glClear");
        }
    }

    /* Restore the old values (why..?) */
    if (Flags & WINED3DCLEAR_STENCIL) {
        glStencilMask(This->stateBlock->renderState[WINED3DRS_STENCILWRITEMASK]);
    }
    if (Flags & WINED3DCLEAR_TARGET) {
        DWORD mask = This->stateBlock->renderState[WINED3DRS_COLORWRITEENABLE];
        glColorMask(mask & WINED3DCOLORWRITEENABLE_RED ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_GREEN ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_BLUE  ? GL_TRUE : GL_FALSE,
                    mask & WINED3DCOLORWRITEENABLE_ALPHA ? GL_TRUE : GL_FALSE);
    }

    LEAVE_GL();

    /* Dirtify the target surface for now. If the surface is locked regularly, and an up to date sysmem copy exists,
     * it is most likely more efficient to perform a clear on the sysmem copy too instead of downloading it
     */
    IWineD3DSurface_ModifyLocation(This->lastActiveRenderTarget, SFLAG_INDRAWABLE, TRUE);
    /* TODO: Move the fbo logic into ModifyLocation() */
    if(This->render_offscreen && wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        target->Flags |= SFLAG_INTEXTURE;
    }
    return WINED3D_OK;
}

/*****
 * Drawing functions
 *****/
static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitive(IWineD3DDevice *iface, WINED3DPRIMITIVETYPE PrimitiveType, UINT StartVertex,
                                                UINT PrimitiveCount) {

    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Type=(%d,%s), Start=%d, Count=%d\n", This, PrimitiveType,
                               debug_d3dprimitivetype(PrimitiveType),
                               StartVertex, PrimitiveCount);

    /* The index buffer is not needed here, but restore it, otherwise it is hell to keep track of */
    if(This->stateBlock->streamIsUP) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        This->stateBlock->streamIsUP = FALSE;
    }

    if(This->stateBlock->loadBaseVertexIndex != 0) {
        This->stateBlock->loadBaseVertexIndex = 0;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }
    /* Account for the loading offset due to index buffers. Instead of reloading all sources correct it with the startvertex parameter */
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, StartVertex, 0/* NumVertices */, -1 /* indxStart */,
                  0 /* indxSize */, NULL /* indxData */, 0 /* minIndex */);
    return WINED3D_OK;
}

/* TODO: baseVIndex needs to be provided from This->stateBlock->baseVertexIndex when called from d3d8 */
static HRESULT  WINAPI  IWineD3DDeviceImpl_DrawIndexedPrimitive(IWineD3DDevice *iface,
                                                           WINED3DPRIMITIVETYPE PrimitiveType,
                                                           UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) {

    IWineD3DDeviceImpl  *This = (IWineD3DDeviceImpl *)iface;
    UINT                 idxStride = 2;
    IWineD3DIndexBuffer *pIB;
    WINED3DINDEXBUFFER_DESC  IdxBufDsc;
    GLuint vbo;

    pIB = This->stateBlock->pIndexData;
    if (!pIB) {
        /* D3D9 returns D3DERR_INVALIDCALL when DrawIndexedPrimitive is called
         * without an index buffer set. (The first time at least...)
         * D3D8 simply dies, but I doubt it can do much harm to return
         * D3DERR_INVALIDCALL there as well. */
        ERR("(%p) : Called without a valid index buffer set, returning WINED3DERR_INVALIDCALL\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if(This->stateBlock->streamIsUP) {
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
        This->stateBlock->streamIsUP = FALSE;
    }
    vbo = ((IWineD3DIndexBufferImpl *) pIB)->vbo;

    TRACE("(%p) : Type=(%d,%s), min=%d, CountV=%d, startIdx=%d, countP=%d\n", This,
          PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
          minIndex, NumVertices, startIndex, primCount);

    IWineD3DIndexBuffer_GetDesc(pIB, &IdxBufDsc);
    if (IdxBufDsc.Format == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    if(This->stateBlock->loadBaseVertexIndex != This->stateBlock->baseVertexIndex) {
        This->stateBlock->loadBaseVertexIndex = This->stateBlock->baseVertexIndex;
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);
    }

    drawPrimitive(iface, PrimitiveType, primCount, 0, NumVertices, startIndex,
                   idxStride, vbo ? NULL : ((IWineD3DIndexBufferImpl *) pIB)->resource.allocatedMemory, minIndex);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveUP(IWineD3DDevice *iface, WINED3DPRIMITIVETYPE PrimitiveType,
                                                    UINT PrimitiveCount, CONST void* pVertexStreamZeroData,
                                                    UINT VertexStreamZeroStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBuffer *vb;

    TRACE("(%p) : Type=(%d,%s), pCount=%d, pVtxData=%p, Stride=%d\n", This, PrimitiveType,
             debug_d3dprimitivetype(PrimitiveType),
             PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    vb = This->stateBlock->streamSource[0];
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    if(vb) IWineD3DVertexBuffer_Release(vb);
    This->stateBlock->streamOffset[0] = 0;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->loadBaseVertexIndex = 0;

    /* TODO: Only mark dirty if drawing from a different UP address */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_STREAMSRC);

    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0 /* start vertex */, 0  /* NumVertices */,
                  0 /* indxStart*/, 0 /* indxSize*/, NULL /* indxData */, 0 /* indxMin */);

    /* MSDN specifies stream zero settings must be set to NULL */
    This->stateBlock->streamStride[0] = 0;
    This->stateBlock->streamSource[0] = NULL;

    /* stream zero settings set to null at end, as per the msdn. No need to mark dirty here, the app has to set
     * the new stream sources or use UP drawing again
     */
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveUP(IWineD3DDevice *iface, WINED3DPRIMITIVETYPE PrimitiveType,
                                                             UINT MinVertexIndex, UINT NumVertices,
                                                             UINT PrimitiveCount, CONST void* pIndexData,
                                                             WINED3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    int                 idxStride;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBuffer *vb;
    IWineD3DIndexBuffer *ib;

    TRACE("(%p) : Type=(%d,%s), MinVtxIdx=%d, NumVIdx=%d, PCount=%d, pidxdata=%p, IdxFmt=%d, pVtxdata=%p, stride=%d\n",
             This, PrimitiveType, debug_d3dprimitivetype(PrimitiveType),
             MinVertexIndex, NumVertices, PrimitiveCount, pIndexData,
             IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    if (IndexDataFormat == WINED3DFMT_INDEX16) {
        idxStride = 2;
    } else {
        idxStride = 4;
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    vb = This->stateBlock->streamSource[0];
    This->stateBlock->streamSource[0] = (IWineD3DVertexBuffer *)pVertexStreamZeroData;
    if(vb) IWineD3DVertexBuffer_Release(vb);
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->streamOffset[0] = 0;
    This->stateBlock->streamStride[0] = VertexStreamZeroStride;

    /* Set to 0 as per msdn. Do it now due to the stream source loading during drawPrimitive */
    This->stateBlock->baseVertexIndex = 0;
    This->stateBlock->loadBaseVertexIndex = 0;
    /* Mark the state dirty until we have nicer tracking of the stream source pointers */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);

    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0 /* vertexStart */, NumVertices, 0 /* indxStart */, idxStride, pIndexData, MinVertexIndex);

    /* MSDN specifies stream zero settings and index buffer must be set to NULL */
    This->stateBlock->streamSource[0] = NULL;
    This->stateBlock->streamStride[0] = 0;
    ib = This->stateBlock->pIndexData;
    if(ib) {
        IWineD3DIndexBuffer_Release(ib);
        This->stateBlock->pIndexData = NULL;
    }
    /* No need to mark the stream source state dirty here. Either the app calls UP drawing again, or it has to call
     * SetStreamSource to specify a vertex buffer
     */

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawPrimitiveStrided (IWineD3DDevice *iface, WINED3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, WineDirect3DVertexStridedData *DrawPrimStrideData) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;

    /* Mark the state dirty until we have nicer tracking
     * its fine to change baseVertexIndex because that call is only called by ddraw which does not need
     * that value.
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
    This->stateBlock->baseVertexIndex = 0;
    This->up_strided = DrawPrimStrideData;
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0, 0, 0, 0, NULL, 0);
    This->up_strided = NULL;
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawIndexedPrimitiveStrided(IWineD3DDevice *iface, WINED3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, WineDirect3DVertexStridedData *DrawPrimStrideData, UINT NumVertices, CONST void *pIndexData, WINED3DFORMAT IndexDataFormat) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    DWORD idxSize = (IndexDataFormat == WINED3DFMT_INDEX32 ? 4 : 2);

    /* Mark the state dirty until we have nicer tracking
     * its fine to change baseVertexIndex because that call is only called by ddraw which does not need
     * that value.
     */
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VDECL);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_INDEXBUFFER);
    This->stateBlock->streamIsUP = TRUE;
    This->stateBlock->baseVertexIndex = 0;
    This->up_strided = DrawPrimStrideData;
    drawPrimitive(iface, PrimitiveType, PrimitiveCount, 0 /* startvertexidx */, 0 /* numindices */, 0 /* startidx */, idxSize, pIndexData, 0 /* minindex */);
    This->up_strided = NULL;
    return WINED3D_OK;
}

static HRESULT IWineD3DDeviceImpl_UpdateVolume(IWineD3DDevice *iface, IWineD3DVolume *pSourceVolume, IWineD3DVolume *pDestinationVolume) {
    /* This is a helper function for UpdateTexture, there is no public UpdateVolume method in d3d. Since it's
     * not callable by the app directly no parameter validation checks are needed here.
     */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    WINED3DLOCKED_BOX src;
    WINED3DLOCKED_BOX dst;
    HRESULT hr;
    TRACE("(%p)->(%p, %p)\n", This, pSourceVolume, pDestinationVolume);

    /* TODO: Implement direct loading into the gl volume instead of using memcpy and
     * dirtification to improve loading performance.
     */
    hr = IWineD3DVolume_LockBox(pSourceVolume, &src, NULL, WINED3DLOCK_READONLY);
    if(FAILED(hr)) return hr;
    hr = IWineD3DVolume_LockBox(pDestinationVolume, &dst, NULL, WINED3DLOCK_DISCARD);
    if(FAILED(hr)) {
    IWineD3DVolume_UnlockBox(pSourceVolume);
            return hr;
    }

    memcpy(dst.pBits, src.pBits, ((IWineD3DVolumeImpl *) pDestinationVolume)->resource.size);

    hr = IWineD3DVolume_UnlockBox(pDestinationVolume);
    if(FAILED(hr)) {
        IWineD3DVolume_UnlockBox(pSourceVolume);
    } else {
        hr = IWineD3DVolume_UnlockBox(pSourceVolume);
    }
    return hr;
}

/* Yet another way to update a texture, some apps use this to load default textures instead of using surface/texture lock/unlock */
static HRESULT WINAPI IWineD3DDeviceImpl_UpdateTexture (IWineD3DDevice *iface, IWineD3DBaseTexture *pSourceTexture,  IWineD3DBaseTexture *pDestinationTexture){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT hr = WINED3D_OK;
    WINED3DRESOURCETYPE sourceType;
    WINED3DRESOURCETYPE destinationType;
    int i ,levels;

    /* TODO: think about moving the code into IWineD3DBaseTexture  */

    TRACE("(%p) Source %p Destination %p\n", This, pSourceTexture, pDestinationTexture);

    /* verify that the source and destination textures aren't NULL */
    if (NULL == pSourceTexture || NULL == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must not be NULL, returning WINED3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }

    if (pSourceTexture == pDestinationTexture) {
        WARN("(%p) : source (%p) and destination (%p) textures must be different, returning WINED3DERR_INVALIDCALL\n",
             This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }
    /* Verify that the source and destination textures are the same type */
    sourceType      = IWineD3DBaseTexture_GetType(pSourceTexture);
    destinationType = IWineD3DBaseTexture_GetType(pDestinationTexture);

    if (sourceType != destinationType) {
        WARN("(%p) Sorce and destination types must match, returning WINED3DERR_INVALIDCALL\n",
             This);
        hr = WINED3DERR_INVALIDCALL;
    }

    /* check that both textures have the identical numbers of levels  */
    if (IWineD3DBaseTexture_GetLevelCount(pDestinationTexture)  != IWineD3DBaseTexture_GetLevelCount(pSourceTexture)) {
        WARN("(%p) : source (%p) and destination (%p) textures must have identicle numbers of levels, returning WINED3DERR_INVALIDCALL\n", This, pSourceTexture, pDestinationTexture);
        hr = WINED3DERR_INVALIDCALL;
    }

    if (WINED3D_OK == hr) {

        /* Make sure that the destination texture is loaded */
        IWineD3DBaseTexture_PreLoad(pDestinationTexture);

        /* Update every surface level of the texture */
        levels = IWineD3DBaseTexture_GetLevelCount(pDestinationTexture);

        switch (sourceType) {
        case WINED3DRTYPE_TEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pSourceTexture,      i, &srcSurface);
                    IWineD3DTexture_GetSurfaceLevel((IWineD3DTexture *)pDestinationTexture, i, &destSurface);
                    hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                    IWineD3DSurface_Release(srcSurface);
                    IWineD3DSurface_Release(destSurface);
                    if (WINED3D_OK != hr) {
                        WARN("(%p) : Call to update surface failed\n", This);
                        return hr;
                    }
                }
            }
            break;
        case WINED3DRTYPE_CUBETEXTURE:
            {
                IWineD3DSurface *srcSurface;
                IWineD3DSurface *destSurface;
                WINED3DCUBEMAP_FACES faceType;

                for (i = 0 ; i < levels ; ++i) {
                    /* Update each cube face */
                    for (faceType = WINED3DCUBEMAP_FACE_POSITIVE_X; faceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z; ++faceType){
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pSourceTexture,      faceType, i, &srcSurface);
                        if (WINED3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got srcSurface %p\n", srcSurface);
                        }
                        hr = IWineD3DCubeTexture_GetCubeMapSurface((IWineD3DCubeTexture *)pDestinationTexture, faceType, i, &destSurface);
                        if (WINED3D_OK != hr) {
                            FIXME("(%p) : Failed to get src cube surface facetype %d, level %d\n", This, faceType, i);
                        } else {
                            TRACE("Got desrSurface %p\n", destSurface);
                        }
                        hr = IWineD3DDevice_UpdateSurface(iface, srcSurface, NULL, destSurface, NULL);
                        IWineD3DSurface_Release(srcSurface);
                        IWineD3DSurface_Release(destSurface);
                        if (WINED3D_OK != hr) {
                            WARN("(%p) : Call to update surface failed\n", This);
                            return hr;
                        }
                    }
                }
            }
            break;

        case WINED3DRTYPE_VOLUMETEXTURE:
            {
                IWineD3DVolume  *srcVolume  = NULL;
                IWineD3DVolume  *destVolume = NULL;

                for (i = 0 ; i < levels ; ++i) {
                    IWineD3DVolumeTexture_GetVolumeLevel((IWineD3DVolumeTexture *)pSourceTexture,      i, &srcVolume);
                    IWineD3DVolumeTexture_GetVolumeLevel((IWineD3DVolumeTexture *)pDestinationTexture, i, &destVolume);
                    hr =  IWineD3DDeviceImpl_UpdateVolume(iface, srcVolume, destVolume);
                    IWineD3DVolume_Release(srcVolume);
                    IWineD3DVolume_Release(destVolume);
                    if (WINED3D_OK != hr) {
                        WARN("(%p) : Call to update volume failed\n", This);
                        return hr;
                    }
                }
            }
            break;

        default:
            FIXME("(%p) : Unsupported source and destination type\n", This);
            hr = WINED3DERR_INVALIDCALL;
        }
    }

    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetFrontBufferData(IWineD3DDevice *iface,UINT iSwapChain, IWineD3DSurface *pDestSurface) {
    IWineD3DSwapChain *swapChain;
    HRESULT hr;
    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
    if(hr == WINED3D_OK) {
        hr = IWineD3DSwapChain_GetFrontBufferData(swapChain, pDestSurface);
                IWineD3DSwapChain_Release(swapChain);
    }
    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_ValidateDevice(IWineD3DDevice *iface, DWORD* pNumPasses) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /* return a sensible default */
    *pNumPasses = 1;
    /* TODO: If the window is minimized then validate device should return something other than WINED3D_OK */
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return WINED3DERR_INVALIDCALL;
    }
    for (j = 0; j < 256; ++j) {
        This->palettes[PaletteNumber][j].peRed   = pEntries[j].peRed;
        This->palettes[PaletteNumber][j].peGreen = pEntries[j].peGreen;
        This->palettes[PaletteNumber][j].peBlue  = pEntries[j].peBlue;
        This->palettes[PaletteNumber][j].peFlags = pEntries[j].peFlags;
    }
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetPaletteEntries(IWineD3DDevice *iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int j;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return WINED3DERR_INVALIDCALL;
    }
    for (j = 0; j < 256; ++j) {
        pEntries[j].peRed   = This->palettes[PaletteNumber][j].peRed;
        pEntries[j].peGreen = This->palettes[PaletteNumber][j].peGreen;
        pEntries[j].peBlue  = This->palettes[PaletteNumber][j].peBlue;
        pEntries[j].peFlags = This->palettes[PaletteNumber][j].peFlags;
    }
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetCurrentTexturePalette(IWineD3DDevice *iface, UINT PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : PaletteNumber %u\n", This, PaletteNumber);
    if ( PaletteNumber < 0 || PaletteNumber >= MAX_PALETTES) {
        WARN("(%p) : (%u) Out of range 0-%u, returning Invalid Call\n", This, PaletteNumber, MAX_PALETTES);
        return WINED3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    This->currentPalette = PaletteNumber;
    TRACE("(%p) : returning\n", This);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetCurrentTexturePalette(IWineD3DDevice *iface, UINT* PaletteNumber) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    if (PaletteNumber == NULL) {
        WARN("(%p) : returning Invalid Call\n", This);
        return WINED3DERR_INVALIDCALL;
    }
    /*TODO: stateblocks */
    *PaletteNumber = This->currentPalette;
    TRACE("(%p) : returning  %u\n", This, *PaletteNumber);
    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetSoftwareVertexProcessing(IWineD3DDevice *iface, BOOL bSoftware) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showFixmes = TRUE;
    if (showFixmes) {
        FIXME("(%p) : stub\n", This);
        showFixmes = FALSE;
    }

    This->softwareVertexProcessing = bSoftware;
    return WINED3D_OK;
}


static BOOL     WINAPI  IWineD3DDeviceImpl_GetSoftwareVertexProcessing(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showFixmes = TRUE;
    if (showFixmes) {
        FIXME("(%p) : stub\n", This);
        showFixmes = FALSE;
    }
    return This->softwareVertexProcessing;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_GetRasterStatus(IWineD3DDevice *iface, UINT iSwapChain, WINED3DRASTER_STATUS* pRasterStatus) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSwapChain *swapChain;
    HRESULT hr;

    TRACE("(%p) :  SwapChain %d returning %p\n", This, iSwapChain, pRasterStatus);

    hr = IWineD3DDeviceImpl_GetSwapChain(iface,  iSwapChain, (IWineD3DSwapChain **)&swapChain);
    if(hr == WINED3D_OK){
        hr = IWineD3DSwapChain_GetRasterStatus(swapChain, pRasterStatus);
        IWineD3DSwapChain_Release(swapChain);
    }else{
        FIXME("(%p) IWineD3DSwapChain_GetRasterStatus returned in error\n", This);
    }
    return hr;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_SetNPatchMode(IWineD3DDevice *iface, float nSegments) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showfixmes = TRUE;
    if(nSegments != 0.0f) {
        if( showfixmes) {
            FIXME("(%p) : stub nSegments(%f)\n", This, nSegments);
            showfixmes = FALSE;
        }
    }
    return WINED3D_OK;
}

static float    WINAPI  IWineD3DDeviceImpl_GetNPatchMode(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    static BOOL showfixmes = TRUE;
    if( showfixmes) {
        FIXME("(%p) : stub returning(%f)\n", This, 0.0f);
        showfixmes = FALSE;
    }
    return 0.0f;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_UpdateSurface(IWineD3DDevice *iface, IWineD3DSurface *pSourceSurface, CONST RECT* pSourceRect, IWineD3DSurface *pDestinationSurface, CONST POINT* pDestPoint) {
    IWineD3DDeviceImpl  *This         = (IWineD3DDeviceImpl *) iface;
    /** TODO: remove casts to IWineD3DSurfaceImpl
     *       NOTE: move code to surface to accomplish this
      ****************************************/
    IWineD3DSurfaceImpl *pSrcSurface  = (IWineD3DSurfaceImpl *)pSourceSurface;
    int srcWidth, srcHeight;
    unsigned int  srcSurfaceWidth, srcSurfaceHeight, destSurfaceWidth, destSurfaceHeight;
    WINED3DFORMAT destFormat, srcFormat;
    UINT          destSize;
    int srcLeft, destLeft, destTop;
    WINED3DPOOL       srcPool, destPool;
    int offset    = 0;
    int rowoffset = 0; /* how many bytes to add onto the end of a row to wraparound to the beginning of the next */
    glDescriptor *glDescription = NULL;
    GLenum dummy;
    int bpp;
    CONVERT_TYPES convert = NO_CONVERSION;

    WINED3DSURFACE_DESC  winedesc;

    TRACE("(%p) : Source (%p)  Rect (%p) Destination (%p) Point(%p)\n", This, pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
    memset(&winedesc, 0, sizeof(winedesc));
    winedesc.Width  = &srcSurfaceWidth;
    winedesc.Height = &srcSurfaceHeight;
    winedesc.Pool   = &srcPool;
    winedesc.Format = &srcFormat;

    IWineD3DSurface_GetDesc(pSourceSurface, &winedesc);

    winedesc.Width  = &destSurfaceWidth;
    winedesc.Height = &destSurfaceHeight;
    winedesc.Pool   = &destPool;
    winedesc.Format = &destFormat;
    winedesc.Size   = &destSize;

    IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);

    if(srcPool != WINED3DPOOL_SYSTEMMEM  || destPool != WINED3DPOOL_DEFAULT){
        WARN("source %p must be SYSTEMMEM and dest %p must be DEFAULT, returning WINED3DERR_INVALIDCALL\n", pSourceSurface, pDestinationSurface);
        return WINED3DERR_INVALIDCALL;
    }

    /* This call loads the opengl surface directly, instead of copying the surface to the
     * destination's sysmem copy. If surface conversion is needed, use BltFast instead to
     * copy in sysmem and use regular surface loading.
     */
    d3dfmt_get_conv((IWineD3DSurfaceImpl *) pDestinationSurface, FALSE, TRUE,
                    &dummy, &dummy, &dummy, &convert, &bpp, FALSE);
    if(convert != NO_CONVERSION) {
        return IWineD3DSurface_BltFast(pDestinationSurface,
                                        pDestPoint  ? pDestPoint->x : 0,
                                        pDestPoint  ? pDestPoint->y : 0,
                                        pSourceSurface, (RECT *) pSourceRect, 0);
    }

    if (destFormat == WINED3DFMT_UNKNOWN) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", This);
        IWineD3DSurface_SetFormat(pDestinationSurface, srcFormat);

        /* Get the update surface description */
        IWineD3DSurface_GetDesc(pDestinationSurface, &winedesc);
    }

    ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

    ENTER_GL();

    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
        checkGLcall("glActiveTextureARB");
    }

    /* Make sure the surface is loaded and up to date */
    IWineD3DSurface_PreLoad(pDestinationSurface);

    IWineD3DSurface_GetGlDesc(pDestinationSurface, &glDescription);

    /* this needs to be done in lines if the sourceRect != the sourceWidth */
    srcWidth   = pSourceRect ? pSourceRect->right - pSourceRect->left   : srcSurfaceWidth;
    srcHeight  = pSourceRect ? pSourceRect->bottom - pSourceRect->top   : srcSurfaceHeight;
    srcLeft    = pSourceRect ? pSourceRect->left : 0;
    destLeft   = pDestPoint  ? pDestPoint->x : 0;
    destTop    = pDestPoint  ? pDestPoint->y : 0;


    /* This function doesn't support compressed textures
    the pitch is just bytesPerPixel * width */
    if(srcWidth != srcSurfaceWidth  || srcLeft ){
        rowoffset = srcSurfaceWidth * pSrcSurface->bytesPerPixel;
        offset   += srcLeft * pSrcSurface->bytesPerPixel;
        /* TODO: do we ever get 3bpp?, would a shift and an add be quicker than a mul (well maybe a cycle or two) */
    }
    /* TODO DXT formats */

    if(pSourceRect != NULL && pSourceRect->top != 0){
       offset +=  pSourceRect->top * srcSurfaceWidth * pSrcSurface->bytesPerPixel;
    }
    TRACE("(%p) glTexSubImage2D, Level %d, left %d, top %d, width %d, height %d , ftm %d, type %d, memory %p\n"
    ,This
    ,glDescription->level
    ,destLeft
    ,destTop
    ,srcWidth
    ,srcHeight
    ,glDescription->glFormat
    ,glDescription->glType
    ,IWineD3DSurface_GetData(pSourceSurface)
    );

    /* Sanity check */
    if (IWineD3DSurface_GetData(pSourceSurface) == NULL) {

        /* need to lock the surface to get the data */
        FIXME("Surfaces has no allocated memory, but should be an in memory only surface\n");
    }

    /* TODO: Cube and volume support */
    if(rowoffset != 0){
        /* not a whole row so we have to do it a line at a time */
        int j;

        /* hopefully using pointer addtion will be quicker than using a point + j * rowoffset */
        const unsigned char* data =((const unsigned char *)IWineD3DSurface_GetData(pSourceSurface)) + offset;

        for(j = destTop ; j < (srcHeight + destTop) ; j++){

                glTexSubImage2D(glDescription->target
                    ,glDescription->level
                    ,destLeft
                    ,j
                    ,srcWidth
                    ,1
                    ,glDescription->glFormat
                    ,glDescription->glType
                    ,data /* could be quicker using */
                );
            data += rowoffset;
        }

    } else { /* Full width, so just write out the whole texture */

        if (WINED3DFMT_DXT1 == destFormat ||
            WINED3DFMT_DXT2 == destFormat ||
            WINED3DFMT_DXT3 == destFormat ||
            WINED3DFMT_DXT4 == destFormat ||
            WINED3DFMT_DXT5 == destFormat) {
            if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
                if (destSurfaceHeight != srcHeight || destSurfaceWidth != srcWidth) {
                    /* FIXME: The easy way to do this is to lock the destination, and copy the bits across */
                    FIXME("Updating part of a compressed texture is not supported at the moment\n");
                } if (destFormat != srcFormat) {
                    FIXME("Updating mixed format compressed texture is not curretly support\n");
                } else {
                    GL_EXTCALL(glCompressedTexImage2DARB)(glDescription->target,
                                                        glDescription->level,
                                                        glDescription->glFormatInternal,
                                                        srcWidth,
                                                        srcHeight,
                                                        0,
                                                        destSize,
                                                        IWineD3DSurface_GetData(pSourceSurface));
                }
            } else {
                FIXME("Attempting to update a DXT compressed texture without hardware support\n");
            }


        } else {
            glTexSubImage2D(glDescription->target
                    ,glDescription->level
                    ,destLeft
                    ,destTop
                    ,srcWidth
                    ,srcHeight
                    ,glDescription->glFormat
                    ,glDescription->glType
                    ,IWineD3DSurface_GetData(pSourceSurface)
                );
        }
     }
    checkGLcall("glTexSubImage2D");

    LEAVE_GL();

    IWineD3DSurface_ModifyLocation(pDestinationSurface, SFLAG_INTEXTURE, TRUE);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(0));

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DrawRectPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST WINED3DRECTPATCH_INFO* pRectPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct WineD3DRectPatch *patch;
    unsigned int i;
    struct list *e;
    BOOL found;
    TRACE("(%p) Handle(%d) noSegs(%p) rectpatch(%p)\n", This, Handle, pNumSegs, pRectPatchInfo);

    if(!(Handle || pRectPatchInfo)) {
        /* TODO: Write a test for the return value, thus the FIXME */
        FIXME("Both Handle and pRectPatchInfo are NULL\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(Handle) {
        i = PATCHMAP_HASHFUNC(Handle);
        found = FALSE;
        LIST_FOR_EACH(e, &This->patches[i]) {
            patch = LIST_ENTRY(e, struct WineD3DRectPatch, entry);
            if(patch->Handle == Handle) {
                found = TRUE;
                break;
            }
        }

        if(!found) {
            TRACE("Patch does not exist. Creating a new one\n");
            patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
            patch->Handle = Handle;
            list_add_head(&This->patches[i], &patch->entry);
        } else {
            TRACE("Found existing patch %p\n", patch);
        }
    } else {
        /* Since opengl does not load tesselated vertex attributes into numbered vertex
         * attributes we have to tesselate, read back, and draw. This needs a patch
         * management structure instance. Create one.
         *
         * A possible improvement is to check if a vertex shader is used, and if not directly
         * draw the patch.
         */
        FIXME("Drawing an uncached patch. This is slow\n");
        patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
    }

    if(pNumSegs[0] != patch->numSegs[0] || pNumSegs[1] != patch->numSegs[1] ||
       pNumSegs[2] != patch->numSegs[2] || pNumSegs[3] != patch->numSegs[3] ||
       (pRectPatchInfo && memcmp(pRectPatchInfo, &patch->RectPatchInfo, sizeof(*pRectPatchInfo)) != 0) ) {
        HRESULT hr;
        TRACE("Tesselation density or patch info changed, retesselating\n");

        if(pRectPatchInfo) {
            memcpy(&patch->RectPatchInfo, pRectPatchInfo, sizeof(*pRectPatchInfo));
        }
        patch->numSegs[0] = pNumSegs[0];
        patch->numSegs[1] = pNumSegs[1];
        patch->numSegs[2] = pNumSegs[2];
        patch->numSegs[3] = pNumSegs[3];

        hr = tesselate_rectpatch(This, patch);
        if(FAILED(hr)) {
            WARN("Patch tesselation failed\n");

            /* Do not release the handle to store the params of the patch */
            if(!Handle) {
                HeapFree(GetProcessHeap(), 0, patch);
            }
            return hr;
        }
    }

    This->currentPatch = patch;
    IWineD3DDevice_DrawPrimitiveStrided(iface, WINED3DPT_TRIANGLELIST, patch->numSegs[0] * patch->numSegs[1] * 2, &patch->strided);
    This->currentPatch = NULL;

    /* Destroy uncached patches */
    if(!Handle) {
        HeapFree(GetProcessHeap(), 0, patch->mem);
        HeapFree(GetProcessHeap(), 0, patch);
    }
    return WINED3D_OK;
}

/* http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawTriPatch.asp */
static HRESULT WINAPI IWineD3DDeviceImpl_DrawTriPatch(IWineD3DDevice *iface, UINT Handle, CONST float* pNumSegs, CONST WINED3DTRIPATCH_INFO* pTriPatchInfo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) Handle(%d) noSegs(%p) tripatch(%p)\n", This, Handle, pNumSegs, pTriPatchInfo);
    FIXME("(%p) : Stub\n", This);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_DeletePatch(IWineD3DDevice *iface, UINT Handle) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    int i;
    struct WineD3DRectPatch *patch;
    struct list *e;
    TRACE("(%p) Handle(%d)\n", This, Handle);

    i = PATCHMAP_HASHFUNC(Handle);
    LIST_FOR_EACH(e, &This->patches[i]) {
        patch = LIST_ENTRY(e, struct WineD3DRectPatch, entry);
        if(patch->Handle == Handle) {
            TRACE("Deleting patch %p\n", patch);
            list_remove(&patch->entry);
            HeapFree(GetProcessHeap(), 0, patch->mem);
            HeapFree(GetProcessHeap(), 0, patch);
            return WINED3D_OK;
        }
    }

    /* TODO: Write a test for the return value */
    FIXME("Attempt to destroy nonexistant patch\n");
    return WINED3DERR_INVALIDCALL;
}

static IWineD3DSwapChain *get_swapchain(IWineD3DSurface *target) {
    HRESULT hr;
    IWineD3DSwapChain *swapchain;

    hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if (SUCCEEDED(hr)) {
        IWineD3DSwapChain_Release((IUnknown *)swapchain);
        return swapchain;
    }

    return NULL;
}

static void bind_fbo(IWineD3DDevice *iface, GLenum target, GLuint *fbo) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (!*fbo) {
        GL_EXTCALL(glGenFramebuffersEXT(1, fbo));
        checkGLcall("glGenFramebuffersEXT()");
    }
    GL_EXTCALL(glBindFramebufferEXT(target, *fbo));
    checkGLcall("glBindFramebuffer()");
}

static void attach_surface_fbo(IWineD3DDeviceImpl *This, GLenum fbo_target, DWORD idx, IWineD3DSurface *surface) {
    const IWineD3DSurfaceImpl *surface_impl = (IWineD3DSurfaceImpl *)surface;
    IWineD3DBaseTextureImpl *texture_impl;
    GLenum texttarget, target;
    GLint old_binding;

    texttarget = surface_impl->glDescription.target;
    target = texttarget == GL_TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB;
    glGetIntegerv(texttarget == GL_TEXTURE_2D ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_CUBE_MAP_ARB, &old_binding);

    IWineD3DSurface_PreLoad(surface);

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(target, old_binding);

    /* Update base texture states array */
    if (SUCCEEDED(IWineD3DSurface_GetContainer(surface, &IID_IWineD3DBaseTexture, (void **)&texture_impl))) {
        texture_impl->baseTexture.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
        texture_impl->baseTexture.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
        if (texture_impl->baseTexture.bindCount) {
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(texture_impl->baseTexture.sampler));
        }

        IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)texture_impl);
    }

    GL_EXTCALL(glFramebufferTexture2DEXT(fbo_target, GL_COLOR_ATTACHMENT0_EXT + idx, texttarget,
            surface_impl->glDescription.textureName, surface_impl->glDescription.level));

    checkGLcall("attach_surface_fbo");
}

static void color_fill_fbo(IWineD3DDevice *iface, IWineD3DSurface *surface, CONST WINED3DRECT *rect, WINED3DCOLOR color) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChain *swapchain;

    swapchain = get_swapchain(surface);
    if (swapchain) {
        GLenum buffer;

        TRACE("Surface %p is onscreen\n", surface);

        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        buffer = surface_get_gl_buffer(surface, swapchain);
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer()");
    } else {
        TRACE("Surface %p is offscreen\n", surface);
        bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->dst_fbo);
        attach_surface_fbo(This, GL_FRAMEBUFFER_EXT, 0, surface);
    }

    if (rect) {
        glEnable(GL_SCISSOR_TEST);
        if(!swapchain) {
            glScissor(rect->x1, rect->y1, rect->x2 - rect->x1, rect->y2 - rect->y1);
        } else {
            glScissor(rect->x1, ((IWineD3DSurfaceImpl *)surface)->currentDesc.Height - rect->y2,
                    rect->x2 - rect->x1, rect->y2 - rect->y1);
        }
        checkGLcall("glScissor");
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_COLORWRITEENABLE));

    glClearColor(D3DCOLOR_R(color), D3DCOLOR_G(color), D3DCOLOR_B(color), D3DCOLOR_A(color));
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLcall("glClear");

    if (This->render_offscreen) {
        bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->fbo);
    } else {
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        checkGLcall("glBindFramebuffer()");
    }

    if (swapchain && surface == ((IWineD3DSwapChainImpl *)swapchain)->frontBuffer
            && ((IWineD3DSwapChainImpl *)swapchain)->backBuffer) {
        glDrawBuffer(GL_BACK);
        checkGLcall("glDrawBuffer()");
    }
}

static inline DWORD argb_to_fmt(DWORD color, WINED3DFORMAT destfmt) {
    unsigned int r, g, b, a;
    DWORD ret;

    if(destfmt == WINED3DFMT_A8R8G8B8 || destfmt == WINED3DFMT_X8R8G8B8 ||
       destfmt == WINED3DFMT_R8G8B8)
        return color;

    TRACE("Converting color %08x to format %s\n", color, debug_d3dformat(destfmt));

    a = (color & 0xff000000) >> 24;
    r = (color & 0x00ff0000) >> 16;
    g = (color & 0x0000ff00) >>  8;
    b = (color & 0x000000ff) >>  0;

    switch(destfmt)
    {
        case WINED3DFMT_R5G6B5:
            if(r == 0xff && g == 0xff && b == 0xff) return 0xffff;
            r = (r * 32) / 256;
            g = (g * 64) / 256;
            b = (b * 32) / 256;
            ret  = r << 11;
            ret |= g << 5;
            ret |= b;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
            a = (a *  2) / 256;
            r = (r * 32) / 256;
            g = (g * 32) / 256;
            b = (b * 32) / 256;
            ret  = a << 15;
            ret |= r << 10;
            ret |= g <<  5;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_A8:
            TRACE("Returning %08x\n", a);
            return a;

        case WINED3DFMT_X4R4G4B4:
        case WINED3DFMT_A4R4G4B4:
            a = (a * 16) / 256;
            r = (r * 16) / 256;
            g = (g * 16) / 256;
            b = (b * 16) / 256;
            ret  = a << 12;
            ret |= r <<  8;
            ret |= g <<  4;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_R3G3B2:
            r = (r * 8) / 256;
            g = (g * 8) / 256;
            b = (b * 4) / 256;
            ret  = r <<  5;
            ret |= g <<  2;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_X8B8G8R8:
        case WINED3DFMT_A8B8G8R8:
            ret  = a << 24;
            ret |= b << 16;
            ret |= g <<  8;
            ret |= r <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_A2R10G10B10:
            a = (a *    4) / 256;
            r = (r * 1024) / 256;
            g = (g * 1024) / 256;
            b = (b * 1024) / 256;
            ret  = a << 30;
            ret |= r << 20;
            ret |= g << 10;
            ret |= b <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        case WINED3DFMT_A2B10G10R10:
            a = (a *    4) / 256;
            r = (r * 1024) / 256;
            g = (g * 1024) / 256;
            b = (b * 1024) / 256;
            ret  = a << 30;
            ret |= b << 20;
            ret |= g << 10;
            ret |= r <<  0;
            TRACE("Returning %08x\n", ret);
            return ret;

        default:
            FIXME("Add a COLORFILL conversion for format %s\n", debug_d3dformat(destfmt));
            return 0;
    }
}

static HRESULT WINAPI IWineD3DDeviceImpl_ColorFill(IWineD3DDevice *iface, IWineD3DSurface *pSurface, CONST WINED3DRECT* pRect, WINED3DCOLOR color) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *) pSurface;
    WINEDDBLTFX BltFx;
    TRACE("(%p) Colour fill Surface: %p rect: %p color: 0x%08x\n", This, pSurface, pRect, color);

    if (surface->resource.pool != WINED3DPOOL_DEFAULT && surface->resource.pool != WINED3DPOOL_SYSTEMMEM) {
        FIXME("call to colorfill with non WINED3DPOOL_DEFAULT or WINED3DPOOL_SYSTEMMEM surface\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        color_fill_fbo(iface, pSurface, pRect, color);
        return WINED3D_OK;
    } else {
        /* Just forward this to the DirectDraw blitting engine */
        memset(&BltFx, 0, sizeof(BltFx));
        BltFx.dwSize = sizeof(BltFx);
        BltFx.u5.dwFillColor = argb_to_fmt(color, surface->resource.format);
        return IWineD3DSurface_Blt(pSurface, (RECT *) pRect, NULL, NULL, WINEDDBLT_COLORFILL, &BltFx, WINED3DTEXF_NONE);
    }
}

/* rendertarget and deptth stencil functions */
static HRESULT  WINAPI  IWineD3DDeviceImpl_GetRenderTarget(IWineD3DDevice* iface,DWORD RenderTargetIndex, IWineD3DSurface **ppRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (RenderTargetIndex >= GL_LIMITS(buffers)) {
        ERR("(%p) : Only %d render targets are supported.\n", This, GL_LIMITS(buffers));
        return WINED3DERR_INVALIDCALL;
    }

    *ppRenderTarget = This->render_targets[RenderTargetIndex];
    TRACE("(%p) : RenderTarget %d Index returning %p\n", This, RenderTargetIndex, *ppRenderTarget);
    /* Note inc ref on returned surface */
    if(*ppRenderTarget != NULL)
        IWineD3DSurface_AddRef(*ppRenderTarget);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetFrontBackBuffers(IWineD3DDevice *iface, IWineD3DSurface *Front, IWineD3DSurface *Back) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *FrontImpl = (IWineD3DSurfaceImpl *) Front;
    IWineD3DSurfaceImpl *BackImpl = (IWineD3DSurfaceImpl *) Back;
    IWineD3DSwapChainImpl *Swapchain;
    HRESULT hr;

    TRACE("(%p)->(%p,%p)\n", This, FrontImpl, BackImpl);

    hr = IWineD3DDevice_GetSwapChain(iface, 0, (IWineD3DSwapChain **) &Swapchain);
    if(hr != WINED3D_OK) {
        ERR("Can't get the swapchain\n");
        return hr;
    }

    /* Make sure to release the swapchain */
    IWineD3DSwapChain_Release((IWineD3DSwapChain *) Swapchain);

    if(FrontImpl && !(FrontImpl->resource.usage & WINED3DUSAGE_RENDERTARGET) ) {
        ERR("Trying to set a front buffer which doesn't have WINED3DUSAGE_RENDERTARGET usage\n");
        return WINED3DERR_INVALIDCALL;
    }
    else if(BackImpl && !(BackImpl->resource.usage & WINED3DUSAGE_RENDERTARGET)) {
        ERR("Trying to set a back buffer which doesn't have WINED3DUSAGE_RENDERTARGET usage\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(Swapchain->frontBuffer != Front) {
        TRACE("Changing the front buffer from %p to %p\n", Swapchain->frontBuffer, Front);

        if(Swapchain->frontBuffer)
            IWineD3DSurface_SetContainer(Swapchain->frontBuffer, NULL);
        Swapchain->frontBuffer = Front;

        if(Swapchain->frontBuffer) {
            IWineD3DSurface_SetContainer(Swapchain->frontBuffer, (IWineD3DBase *) Swapchain);
        }
    }

    if(Back && !Swapchain->backBuffer) {
        /* We need memory for the back buffer array - only one back buffer this way */
        Swapchain->backBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DSurface *));
        if(!Swapchain->backBuffer) {
            ERR("Out of memory\n");
            return E_OUTOFMEMORY;
        }
    }

    if(Swapchain->backBuffer[0] != Back) {
        TRACE("Changing the back buffer from %p to %p\n", Swapchain->backBuffer, Back);

        /* What to do about the context here in the case of multithreading? Not sure.
         * This function is called by IDirect3D7::CreateDevice so in theory its initialization code
         */
        ENTER_GL();
        if(!Swapchain->backBuffer[0]) {
            /* GL was told to draw to the front buffer at creation,
             * undo that
             */
            glDrawBuffer(GL_BACK);
            checkGLcall("glDrawBuffer(GL_BACK)");
            /* Set the backbuffer count to 1 because other code uses it to fing the back buffers */
            Swapchain->presentParms.BackBufferCount = 1;
        } else if (!Back) {
            /* That makes problems - disable for now */
            /* glDrawBuffer(GL_FRONT); */
            checkGLcall("glDrawBuffer(GL_FRONT)");
            /* We have lost our back buffer, set this to 0 to avoid confusing other code */
            Swapchain->presentParms.BackBufferCount = 0;
        }
        LEAVE_GL();

        if(Swapchain->backBuffer[0])
            IWineD3DSurface_SetContainer(Swapchain->backBuffer[0], NULL);
        Swapchain->backBuffer[0] = Back;

        if(Swapchain->backBuffer[0]) {
            IWineD3DSurface_SetContainer(Swapchain->backBuffer[0], (IWineD3DBase *) Swapchain);
        } else {
            HeapFree(GetProcessHeap(), 0, Swapchain->backBuffer);
        }

    }

    return WINED3D_OK;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_GetDepthStencilSurface(IWineD3DDevice* iface, IWineD3DSurface **ppZStencilSurface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *ppZStencilSurface = This->stencilBufferTarget;
    TRACE("(%p) : zStencilSurface  returning %p\n", This,  *ppZStencilSurface);

    if(*ppZStencilSurface != NULL) {
        /* Note inc ref on returned surface */
        IWineD3DSurface_AddRef(*ppZStencilSurface);
        return WINED3D_OK;
    } else {
        return WINED3DERR_NOTFOUND;
    }
}

/* TODO: Handle stencil attachments */
static void set_depth_stencil_fbo(IWineD3DDevice *iface, IWineD3DSurface *depth_stencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *depth_stencil_impl = (IWineD3DSurfaceImpl *)depth_stencil;

    TRACE("Set depth stencil to %p\n", depth_stencil);

    if (depth_stencil_impl) {
        if (depth_stencil_impl->current_renderbuffer) {
            GL_EXTCALL(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_stencil_impl->current_renderbuffer->id));
            checkGLcall("glFramebufferRenderbufferEXT()");
        } else {
            IWineD3DBaseTextureImpl *texture_impl;
            GLenum texttarget, target;
            GLint old_binding = 0;

            texttarget = depth_stencil_impl->glDescription.target;
            target = texttarget == GL_TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB;
            glGetIntegerv(texttarget == GL_TEXTURE_2D ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_CUBE_MAP_ARB, &old_binding);

            IWineD3DSurface_PreLoad(depth_stencil);

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            glBindTexture(target, old_binding);

            /* Update base texture states array */
            if (SUCCEEDED(IWineD3DSurface_GetContainer(depth_stencil, &IID_IWineD3DBaseTexture, (void **)&texture_impl))) {
                texture_impl->baseTexture.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
                texture_impl->baseTexture.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
                if (texture_impl->baseTexture.bindCount) {
                    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(texture_impl->baseTexture.sampler));
                }

                IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)texture_impl);
            }

            GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, texttarget,
                    depth_stencil_impl->glDescription.textureName, depth_stencil_impl->glDescription.level));
            checkGLcall("glFramebufferTexture2DEXT()");
        }
    } else {
        GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0));
        checkGLcall("glFramebufferTexture2DEXT()");
    }
}

static void set_render_target_fbo(IWineD3DDevice *iface, DWORD idx, IWineD3DSurface *render_target) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *rtimpl = (IWineD3DSurfaceImpl *)render_target;

    TRACE("Set render target %u to %p\n", idx, render_target);

    if (rtimpl) {
        attach_surface_fbo(This, GL_FRAMEBUFFER_EXT, idx, render_target);
        This->draw_buffers[idx] = GL_COLOR_ATTACHMENT0_EXT + idx;
    } else {
        GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + idx, GL_TEXTURE_2D, 0, 0));
        checkGLcall("glFramebufferTexture2DEXT()");

        This->draw_buffers[idx] = GL_NONE;
    }
}

static void check_fbo_status(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLenum status;

    status = GL_EXTCALL(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
        TRACE("FBO complete\n");
    } else {
        FIXME("FBO status %s (%#x)\n", debug_fbostatus(status), status);

        /* Dump the FBO attachments */
        if (status == GL_FRAMEBUFFER_UNSUPPORTED_EXT) {
            IWineD3DSurfaceImpl *attachment;
            int i;

            for (i = 0; i < GL_LIMITS(buffers); ++i) {
                attachment = (IWineD3DSurfaceImpl *)This->fbo_color_attachments[i];
                if (attachment) {
                    FIXME("\tColor attachment %d: (%p) %s %ux%u\n", i, attachment, debug_d3dformat(attachment->resource.format),
                            attachment->pow2Width, attachment->pow2Height);
                }
            }
            attachment = (IWineD3DSurfaceImpl *)This->fbo_depth_attachment;
            if (attachment) {
                FIXME("\tDepth attachment: (%p) %s %ux%u\n", attachment, debug_d3dformat(attachment->resource.format),
                        attachment->pow2Width, attachment->pow2Height);
            }
        }
    }
}

static BOOL depth_mismatch_fbo(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DSurfaceImpl *rt_impl = (IWineD3DSurfaceImpl *)This->render_targets[0];
    IWineD3DSurfaceImpl *ds_impl = (IWineD3DSurfaceImpl *)This->stencilBufferTarget;

    if (!ds_impl) return FALSE;

    if (ds_impl->current_renderbuffer) {
        return (rt_impl->pow2Width != ds_impl->current_renderbuffer->width ||
                rt_impl->pow2Height != ds_impl->current_renderbuffer->height);
    }

    return (rt_impl->pow2Width != ds_impl->pow2Width ||
            rt_impl->pow2Height != ds_impl->pow2Height);
}

void apply_fbo_state(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i;

    if (This->render_offscreen) {
        bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->fbo);

        /* Apply render targets */
        for (i = 0; i < GL_LIMITS(buffers); ++i) {
            IWineD3DSurface *render_target = This->render_targets[i];
            if (This->fbo_color_attachments[i] != render_target) {
                set_render_target_fbo(iface, i, render_target);
                This->fbo_color_attachments[i] = render_target;
            }
        }

        /* Apply depth targets */
        if (This->fbo_depth_attachment != This->stencilBufferTarget || depth_mismatch_fbo(iface)) {
            unsigned int w = ((IWineD3DSurfaceImpl *)This->render_targets[0])->pow2Width;
            unsigned int h = ((IWineD3DSurfaceImpl *)This->render_targets[0])->pow2Height;

            if (This->stencilBufferTarget) {
                surface_set_compatible_renderbuffer(This->stencilBufferTarget, w, h);
            }
            set_depth_stencil_fbo(iface, This->stencilBufferTarget);
            This->fbo_depth_attachment = This->stencilBufferTarget;
        }

        if (GL_SUPPORT(ARB_DRAW_BUFFERS)) {
            GL_EXTCALL(glDrawBuffersARB(GL_LIMITS(buffers), This->draw_buffers));
            checkGLcall("glDrawBuffers()");
        } else {
            glDrawBuffer(This->draw_buffers[0]);
            checkGLcall("glDrawBuffer()");
        }
    } else {
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
    }

    check_fbo_status(iface);
}

void stretch_rect_fbo(IWineD3DDevice *iface, IWineD3DSurface *src_surface, WINED3DRECT *src_rect,
        IWineD3DSurface *dst_surface, WINED3DRECT *dst_rect, const WINED3DTEXTUREFILTERTYPE filter, BOOL flip) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLbitfield mask = GL_COLOR_BUFFER_BIT; /* TODO: Support blitting depth/stencil surfaces */
    IWineD3DSwapChain *src_swapchain, *dst_swapchain;
    GLenum gl_filter;

    TRACE("(%p) : src_surface %p, src_rect %p, dst_surface %p, dst_rect %p, filter %s (0x%08x), flip %u\n",
            This, src_surface, src_rect, dst_surface, dst_rect, debug_d3dtexturefiltertype(filter), filter, flip);
    TRACE("src_rect [%u, %u]->[%u, %u]\n", src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2);
    TRACE("dst_rect [%u, %u]->[%u, %u]\n", dst_rect->x1, dst_rect->y1, dst_rect->x2, dst_rect->y2);

    switch (filter) {
        case WINED3DTEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (0x%08x)\n", debug_d3dtexturefiltertype(filter), filter);
        case WINED3DTEXF_NONE:
        case WINED3DTEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    /* Attach src surface to src fbo */
    src_swapchain = get_swapchain(src_surface);
    if (src_swapchain) {
        GLenum buffer;

        TRACE("Source surface %p is onscreen\n", src_surface);
        ActivateContext(This, src_surface, CTXUSAGE_RESOURCELOAD);

        ENTER_GL();
        GL_EXTCALL(glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0));
        buffer = surface_get_gl_buffer(src_surface, src_swapchain);
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer()");

        src_rect->y1 = ((IWineD3DSurfaceImpl *)src_surface)->currentDesc.Height - src_rect->y1;
        src_rect->y2 = ((IWineD3DSurfaceImpl *)src_surface)->currentDesc.Height - src_rect->y2;
    } else {
        TRACE("Source surface %p is offscreen\n", src_surface);
        ENTER_GL();
        bind_fbo(iface, GL_READ_FRAMEBUFFER_EXT, &This->src_fbo);
        attach_surface_fbo(This, GL_READ_FRAMEBUFFER_EXT, 0, src_surface);
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        checkGLcall("glReadBuffer()");
    }
    LEAVE_GL();

    /* Attach dst surface to dst fbo */
    dst_swapchain = get_swapchain(dst_surface);
    if (dst_swapchain) {
        GLenum buffer;

        TRACE("Destination surface %p is onscreen\n", dst_surface);
        ActivateContext(This, dst_surface, CTXUSAGE_RESOURCELOAD);

        ENTER_GL();
        GL_EXTCALL(glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0));
        buffer = surface_get_gl_buffer(dst_surface, dst_swapchain);
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer()");

        dst_rect->y1 = ((IWineD3DSurfaceImpl *)dst_surface)->currentDesc.Height - dst_rect->y1;
        dst_rect->y2 = ((IWineD3DSurfaceImpl *)dst_surface)->currentDesc.Height - dst_rect->y2;
    } else {
        TRACE("Destination surface %p is offscreen\n", dst_surface);

        /* No src or dst swapchain? Make sure some context is active(multithreading) */
        if(!src_swapchain) {
            ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }

        ENTER_GL();
        bind_fbo(iface, GL_DRAW_FRAMEBUFFER_EXT, &This->dst_fbo);
        attach_surface_fbo(This, GL_DRAW_FRAMEBUFFER_EXT, 0, dst_surface);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        checkGLcall("glDrawBuffer()");
    }
    glDisable(GL_SCISSOR_TEST);
    IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    if (flip) {
        GL_EXTCALL(glBlitFramebufferEXT(src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2,
                dst_rect->x1, dst_rect->y2, dst_rect->x2, dst_rect->y1, mask, gl_filter));
        checkGLcall("glBlitFramebuffer()");
    } else {
        GL_EXTCALL(glBlitFramebufferEXT(src_rect->x1, src_rect->y1, src_rect->x2, src_rect->y2,
                dst_rect->x1, dst_rect->y1, dst_rect->x2, dst_rect->y2, mask, gl_filter));
        checkGLcall("glBlitFramebuffer()");
    }

    if (This->render_offscreen) {
        bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->fbo);
    } else {
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
        checkGLcall("glBindFramebuffer()");
    }

    /* If we switched from GL_BACK to GL_FRONT above, we need to switch back here */
    if (dst_swapchain && dst_surface == ((IWineD3DSwapChainImpl *)dst_swapchain)->frontBuffer
            && ((IWineD3DSwapChainImpl *)dst_swapchain)->backBuffer) {
        glDrawBuffer(GL_BACK);
        checkGLcall("glDrawBuffer()");
    }
    LEAVE_GL();
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetRenderTarget(IWineD3DDevice *iface, DWORD RenderTargetIndex, IWineD3DSurface *pRenderTarget) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WINED3DVIEWPORT viewport;

    TRACE("(%p) : Setting rendertarget %d to %p\n", This, RenderTargetIndex, pRenderTarget);

    if (RenderTargetIndex >= GL_LIMITS(buffers)) {
        ERR("(%p) : Only %d render targets are supported.\n", This, GL_LIMITS(buffers));
        return WINED3DERR_INVALIDCALL;
    }

    /* MSDN says that null disables the render target
    but a device must always be associated with a render target
    nope MSDN says that we return invalid call to a null rendertarget with an index of 0

    see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/AdvancedTopics/PixelPipe/MultipleRenderTarget.asp
    for more details
    */
    if (RenderTargetIndex == 0 && pRenderTarget == NULL) {
        FIXME("Trying to set render target 0 to NULL\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (pRenderTarget && !((IWineD3DSurfaceImpl *)pRenderTarget)->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        FIXME("(%p)Trying to set the render target to a surface(%p) that wasn't created with a usage of WINED3DUSAGE_RENDERTARGET\n",This ,pRenderTarget);
        return WINED3DERR_INVALIDCALL;
    }

    /* If we are trying to set what we already have, don't bother */
    if (pRenderTarget == This->render_targets[RenderTargetIndex]) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
        return WINED3D_OK;
    }
    if(pRenderTarget) IWineD3DSurface_AddRef(pRenderTarget);
    if(This->render_targets[RenderTargetIndex]) IWineD3DSurface_Release(This->render_targets[RenderTargetIndex]);
    This->render_targets[RenderTargetIndex] = pRenderTarget;

    /* Render target 0 is special */
    if(RenderTargetIndex == 0) {
        /* Finally, reset the viewport as the MSDN states. */
        viewport.Height = ((IWineD3DSurfaceImpl *)This->render_targets[0])->currentDesc.Height;
        viewport.Width  = ((IWineD3DSurfaceImpl *)This->render_targets[0])->currentDesc.Width;
        viewport.X      = 0;
        viewport.Y      = 0;
        viewport.MaxZ   = 1.0f;
        viewport.MinZ   = 0.0f;
        IWineD3DDeviceImpl_SetViewport(iface, &viewport);
        /* Make sure the viewport state is dirty, because the render_offscreen thing affects it.
         * SetViewport may catch NOP viewport changes, which would occur when switching between equally sized targets
         */
        IWineD3DDeviceImpl_MarkStateDirty(This, STATE_VIEWPORT);

        /* Activate the new render target for now. This shouldn't stay here, but is needed until all methods using gl activate the
         * ctx properly.
         * Use resourceload usage, this will just set the drawables and context but not apply any states. The stateblock may be
         * incomplete or incorrect when SetRenderTarget is called. DrawPrim() will apply the states when it is called.
         */
        ActivateContext(This, This->render_targets[0], CTXUSAGE_RESOURCELOAD);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDepthStencilSurface(IWineD3DDevice *iface, IWineD3DSurface *pNewZStencil) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    HRESULT  hr = WINED3D_OK;
    IWineD3DSurface *tmp;

    TRACE("(%p) Swapping z-buffer. Old = %p, new = %p\n",This, This->stencilBufferTarget, pNewZStencil);

    if (pNewZStencil == This->stencilBufferTarget) {
        TRACE("Trying to do a NOP SetRenderTarget operation\n");
    } else {
        /** OpenGL doesn't support 'sharing' of the stencilBuffer so we may incure an extra memory overhead
        * depending on the renter target implementation being used.
        * A shared context implementation will share all buffers between all rendertargets (including swapchains),
        * implementations that use separate pbuffers for different swapchains or rendertargets will have to duplicate the
        * stencil buffer and incure an extra memory overhead
         ******************************************************/

        tmp = This->stencilBufferTarget;
        This->stencilBufferTarget = pNewZStencil;
        This->depth_copy_state = WINED3D_DCS_NO_COPY;
        /* should we be calling the parent or the wined3d surface? */
        if (NULL != This->stencilBufferTarget) IWineD3DSurface_AddRef(This->stencilBufferTarget);
        if (NULL != tmp) IWineD3DSurface_Release(tmp);
        hr = WINED3D_OK;

        if((!tmp && pNewZStencil) || (!pNewZStencil && tmp)) {
            /* Swapping NULL / non NULL depth stencil affects the depth and tests */
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_ZENABLE));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_STENCILENABLE));
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_RENDER(WINED3DRS_STENCILWRITEMASK));
        }
    }

    return hr;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_SetCursorProperties(IWineD3DDevice* iface, UINT XHotSpot,
                                                        UINT YHotSpot, IWineD3DSurface *pCursorBitmap) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /* TODO: the use of Impl is deprecated. */
    IWineD3DSurfaceImpl * pSur = (IWineD3DSurfaceImpl *) pCursorBitmap;
    WINED3DLOCKED_RECT lockedRect;

    TRACE("(%p) : Spot Pos(%u,%u)\n", This, XHotSpot, YHotSpot);

    /* some basic validation checks */
    if(This->cursorTexture) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        glDeleteTextures(1, &This->cursorTexture);
        LEAVE_GL();
        This->cursorTexture = 0;
    }

    if ( (pSur->currentDesc.Width == 32) && (pSur->currentDesc.Height == 32) )
        This->haveHardwareCursor = TRUE;
    else
        This->haveHardwareCursor = FALSE;

    if(pCursorBitmap) {
        WINED3DLOCKED_RECT rect;

        /* MSDN: Cursor must be A8R8G8B8 */
        if (WINED3DFMT_A8R8G8B8 != pSur->resource.format) {
            ERR("(%p) : surface(%p) has an invalid format\n", This, pCursorBitmap);
            return WINED3DERR_INVALIDCALL;
        }

        /* MSDN: Cursor must be smaller than the display mode */
        if(pSur->currentDesc.Width > This->ddraw_width ||
           pSur->currentDesc.Height > This->ddraw_height) {
            ERR("(%p) : Surface(%p) is %dx%d pixels, but screen res is %dx%d\n", This, pSur, pSur->currentDesc.Width, pSur->currentDesc.Height, This->ddraw_width, This->ddraw_height);
            return WINED3DERR_INVALIDCALL;
        }

        if (!This->haveHardwareCursor) {
            /* TODO: MSDN: Cursor sizes must be a power of 2 */

            /* Do not store the surface's pointer because the application may
             * release it after setting the cursor image. Windows doesn't
             * addref the set surface, so we can't do this either without
             * creating circular refcount dependencies. Copy out the gl texture
             * instead.
             */
            This->cursorWidth = pSur->currentDesc.Width;
            This->cursorHeight = pSur->currentDesc.Height;
            if (SUCCEEDED(IWineD3DSurface_LockRect(pCursorBitmap, &rect, NULL, WINED3DLOCK_READONLY)))
            {
                const GlPixelFormatDesc *glDesc;
                const StaticPixelFormatDesc *tableEntry = getFormatDescEntry(WINED3DFMT_A8R8G8B8, &GLINFO_LOCATION, &glDesc);
                char *mem, *bits = (char *)rect.pBits;
                GLint intfmt = glDesc->glInternal;
                GLint format = glDesc->glFormat;
                GLint type = glDesc->glType;
                INT height = This->cursorHeight;
                INT width = This->cursorWidth;
                INT bpp = tableEntry->bpp;
                INT i;

                /* Reformat the texture memory (pitch and width can be
                 * different) */
                mem = HeapAlloc(GetProcessHeap(), 0, width * height * bpp);
                for(i = 0; i < height; i++)
                    memcpy(&mem[width * bpp * i], &bits[rect.Pitch * i], width * bpp);
                IWineD3DSurface_UnlockRect(pCursorBitmap);
                ENTER_GL();

                if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
                    checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
                }

                /* Make sure that a proper texture unit is selected */
                if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
                    checkGLcall("glActiveTextureARB");
                }
                IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(0));
                /* Create a new cursor texture */
                glGenTextures(1, &This->cursorTexture);
                checkGLcall("glGenTextures");
                glBindTexture(GL_TEXTURE_2D, This->cursorTexture);
                checkGLcall("glBindTexture");
                /* Copy the bitmap memory into the cursor texture */
                glTexImage2D(GL_TEXTURE_2D, 0, intfmt, width, height, 0, format, type, mem);
                HeapFree(GetProcessHeap(), 0, mem);
                checkGLcall("glTexImage2D");

                if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
                    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
                    checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
                }

                LEAVE_GL();
            }
            else
            {
                FIXME("A cursor texture was not returned.\n");
                This->cursorTexture = 0;
            }
        }
        else
        {
            /* Draw a hardware cursor */
            ICONINFO cursorInfo;
            HCURSOR cursor;
            /* Create and clear maskBits because it is not needed for
             * 32-bit cursors.  32x32 bits split into 32-bit chunks == 32
             * chunks. */
            DWORD *maskBits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                (pSur->currentDesc.Width * pSur->currentDesc.Height / 8));
            IWineD3DSurface_LockRect(pCursorBitmap, &lockedRect, NULL,
                                         WINED3DLOCK_NO_DIRTY_UPDATE |
                                         WINED3DLOCK_READONLY
            );
            TRACE("width: %i height: %i\n", pSur->currentDesc.Width,
                  pSur->currentDesc.Height);

            cursorInfo.fIcon = FALSE;
            cursorInfo.xHotspot = XHotSpot;
            cursorInfo.yHotspot = YHotSpot;
            cursorInfo.hbmMask = CreateBitmap(pSur->currentDesc.Width,
                                              pSur->currentDesc.Height, 1,
                                              1, &maskBits);
            cursorInfo.hbmColor = CreateBitmap(pSur->currentDesc.Width,
                                               pSur->currentDesc.Height, 1,
                                               32, lockedRect.pBits);
            IWineD3DSurface_UnlockRect(pCursorBitmap);
            /* Create our cursor and clean up. */
            cursor = CreateIconIndirect(&cursorInfo);
            SetCursor(cursor);
            if (cursorInfo.hbmMask) DeleteObject(cursorInfo.hbmMask);
            if (cursorInfo.hbmColor) DeleteObject(cursorInfo.hbmColor);
            if (This->hardwareCursor) DestroyCursor(This->hardwareCursor);
            This->hardwareCursor = cursor;
            HeapFree(GetProcessHeap(), 0, maskBits);
        }
    }

    This->xHotSpot = XHotSpot;
    This->yHotSpot = YHotSpot;
    return WINED3D_OK;
}

static void     WINAPI  IWineD3DDeviceImpl_SetCursorPosition(IWineD3DDevice* iface, int XScreenSpace, int YScreenSpace, DWORD Flags) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : SetPos to (%u,%u)\n", This, XScreenSpace, YScreenSpace);

    This->xScreenSpace = XScreenSpace;
    This->yScreenSpace = YScreenSpace;

    return;

}

static BOOL     WINAPI  IWineD3DDeviceImpl_ShowCursor(IWineD3DDevice* iface, BOOL bShow) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    BOOL oldVisible = This->bCursorVisible;
    POINT pt;

    TRACE("(%p) : visible(%d)\n", This, bShow);

    /*
     * When ShowCursor is first called it should make the cursor appear at the OS's last
     * known cursor position.  Because of this, some applications just repetitively call
     * ShowCursor in order to update the cursor's position.  This behavior is undocumented.
     */
    GetCursorPos(&pt);
    This->xScreenSpace = pt.x;
    This->yScreenSpace = pt.y;

    if (This->haveHardwareCursor) {
        This->bCursorVisible = bShow;
        if (bShow)
            SetCursor(This->hardwareCursor);
        else
            SetCursor(NULL);
    }
    else
    {
        if (This->cursorTexture)
            This->bCursorVisible = bShow;
    }

    return oldVisible;
}

static HRESULT  WINAPI  IWineD3DDeviceImpl_TestCooperativeLevel(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DResourceImpl *resource;
    TRACE("(%p) : state (%u)\n", This, This->state);

    /* TODO: Implement wrapping of the WndProc so that mimimize and maxamise can be monitored and the states adjusted. */
    switch (This->state) {
    case WINED3D_OK:
        return WINED3D_OK;
    case WINED3DERR_DEVICELOST:
        {
            LIST_FOR_EACH_ENTRY(resource, &This->resources, IWineD3DResourceImpl, resource.resource_list_entry) {
                if (resource->resource.pool == WINED3DPOOL_DEFAULT)
                    return WINED3DERR_DEVICENOTRESET;
            }
            return WINED3DERR_DEVICELOST;
        }
    case WINED3DERR_DRIVERINTERNALERROR:
        return WINED3DERR_DRIVERINTERNALERROR;
    }

    /* Unknown state */
    return WINED3DERR_DRIVERINTERNALERROR;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_EvictManagedResources(IWineD3DDevice* iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    /** FIXME: Resource tracking needs to be done,
    * The closes we can do to this is set the priorities of all managed textures low
    * and then reset them.
     ***********************************************************/
    FIXME("(%p) : stub\n", This);
    return WINED3D_OK;
}

static void updateSurfaceDesc(IWineD3DSurfaceImpl *surface, WINED3DPRESENT_PARAMETERS* pPresentationParameters) {
    IWineD3DDeviceImpl *This = surface->resource.wineD3DDevice; /* for GL_SUPPORT */

    /* Reallocate proper memory for the front and back buffer and adjust their sizes */
    if(surface->Flags & SFLAG_DIBSECTION) {
        /* Release the DC */
        SelectObject(surface->hDC, surface->dib.holdbitmap);
        DeleteDC(surface->hDC);
        /* Release the DIB section */
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
        surface->resource.allocatedMemory = NULL;
        surface->Flags &= ~SFLAG_DIBSECTION;
    }
    surface->currentDesc.Width = pPresentationParameters->BackBufferWidth;
    surface->currentDesc.Height = pPresentationParameters->BackBufferHeight;
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO)) {
        surface->pow2Width = pPresentationParameters->BackBufferWidth;
        surface->pow2Height = pPresentationParameters->BackBufferHeight;
    } else {
        surface->pow2Width = surface->pow2Height = 1;
        while (surface->pow2Width < pPresentationParameters->BackBufferWidth) surface->pow2Width <<= 1;
        while (surface->pow2Height < pPresentationParameters->BackBufferHeight) surface->pow2Height <<= 1;
    }
    if(surface->glDescription.textureName) {
        ActivateContext(This, This->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        glDeleteTextures(1, &surface->glDescription.textureName);
        LEAVE_GL();
        surface->glDescription.textureName = 0;
        surface->Flags &= ~SFLAG_CLIENT;
    }
    if(surface->pow2Width != pPresentationParameters->BackBufferWidth ||
       surface->pow2Height != pPresentationParameters->BackBufferHeight) {
        surface->Flags |= SFLAG_NONPOW2;
    } else  {
        surface->Flags &= ~SFLAG_NONPOW2;
    }
    HeapFree(GetProcessHeap(), 0, surface->resource.allocatedMemory);
    surface->resource.size = IWineD3DSurface_GetPitch((IWineD3DSurface *) surface) * surface->pow2Width;
}

static HRESULT WINAPI IWineD3DDeviceImpl_Reset(IWineD3DDevice* iface, WINED3DPRESENT_PARAMETERS* pPresentationParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    IWineD3DSwapChainImpl *swapchain;
    HRESULT hr;
    BOOL DisplayModeChanged = FALSE;
    WINED3DDISPLAYMODE mode;
    TRACE("(%p)\n", This);

    hr = IWineD3DDevice_GetSwapChain(iface, 0, (IWineD3DSwapChain **) &swapchain);
    if(FAILED(hr)) {
        ERR("Failed to get the first implicit swapchain\n");
        return hr;
    }

    /* Is it necessary to recreate the gl context? Actually every setting can be changed
     * on an existing gl context, so there's no real need for recreation.
     *
     * TODO: Figure out how Reset influences resources in D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEMORY and D3DPOOL_MANAGED
     *
     * TODO: Figure out what happens to explicit swapchains, or if we have more than one implicit swapchain
     */
    TRACE("New params:\n");
    TRACE("BackBufferWidth = %d\n", pPresentationParameters->BackBufferWidth);
    TRACE("BackBufferHeight = %d\n", pPresentationParameters->BackBufferHeight);
    TRACE("BackBufferFormat = %s\n", debug_d3dformat(pPresentationParameters->BackBufferFormat));
    TRACE("BackBufferCount = %d\n", pPresentationParameters->BackBufferCount);
    TRACE("MultiSampleType = %d\n", pPresentationParameters->MultiSampleType);
    TRACE("MultiSampleQuality = %d\n", pPresentationParameters->MultiSampleQuality);
    TRACE("SwapEffect = %d\n", pPresentationParameters->SwapEffect);
    TRACE("hDeviceWindow = %p\n", pPresentationParameters->hDeviceWindow);
    TRACE("Windowed = %s\n", pPresentationParameters->Windowed ? "true" : "false");
    TRACE("EnableAutoDepthStencil = %s\n", pPresentationParameters->EnableAutoDepthStencil ? "true" : "false");
    TRACE("Flags = %08x\n", pPresentationParameters->Flags);
    TRACE("FullScreen_RefreshRateInHz = %d\n", pPresentationParameters->FullScreen_RefreshRateInHz);
    TRACE("PresentationInterval = %d\n", pPresentationParameters->PresentationInterval);

    /* No special treatment of these parameters. Just store them */
    swapchain->presentParms.SwapEffect = pPresentationParameters->SwapEffect;
    swapchain->presentParms.Flags = pPresentationParameters->Flags;
    swapchain->presentParms.PresentationInterval = pPresentationParameters->PresentationInterval;
    swapchain->presentParms.FullScreen_RefreshRateInHz = pPresentationParameters->FullScreen_RefreshRateInHz;

    /* What to do about these? */
    if(pPresentationParameters->BackBufferCount != 0 &&
        pPresentationParameters->BackBufferCount != swapchain->presentParms.BackBufferCount) {
        ERR("Cannot change the back buffer count yet\n");
    }
    if(pPresentationParameters->BackBufferFormat != WINED3DFMT_UNKNOWN &&
        pPresentationParameters->BackBufferFormat != swapchain->presentParms.BackBufferFormat) {
        ERR("Cannot change the back buffer format yet\n");
    }
    if(pPresentationParameters->hDeviceWindow != NULL &&
        pPresentationParameters->hDeviceWindow != swapchain->presentParms.hDeviceWindow) {
        ERR("Cannot change the device window yet\n");
    }
    if(pPresentationParameters->EnableAutoDepthStencil != swapchain->presentParms.EnableAutoDepthStencil) {
        ERR("What do do about a changed auto depth stencil parameter?\n");
    }

    if(pPresentationParameters->Windowed) {
        mode.Width = swapchain->orig_width;
        mode.Height = swapchain->orig_height;
        mode.RefreshRate = 0;
        mode.Format = swapchain->presentParms.BackBufferFormat;
    } else {
        mode.Width = pPresentationParameters->BackBufferWidth;
        mode.Height = pPresentationParameters->BackBufferHeight;
        mode.RefreshRate = pPresentationParameters->FullScreen_RefreshRateInHz;
        mode.Format = swapchain->presentParms.BackBufferFormat;
    }

    /* Should Width == 800 && Height == 0 set 800x600? */
    if(pPresentationParameters->BackBufferWidth != 0 && pPresentationParameters->BackBufferHeight != 0 &&
       (pPresentationParameters->BackBufferWidth != swapchain->presentParms.BackBufferWidth ||
        pPresentationParameters->BackBufferHeight != swapchain->presentParms.BackBufferHeight))
    {
        WINED3DVIEWPORT vp;
        int i;

        vp.X = 0;
        vp.Y = 0;
        vp.Width = pPresentationParameters->BackBufferWidth;
        vp.Height = pPresentationParameters->BackBufferHeight;
        vp.MinZ = 0;
        vp.MaxZ = 1;

        if(!pPresentationParameters->Windowed) {
            DisplayModeChanged = TRUE;
        }
        swapchain->presentParms.BackBufferWidth = pPresentationParameters->BackBufferWidth;
        swapchain->presentParms.BackBufferHeight = pPresentationParameters->BackBufferHeight;

        updateSurfaceDesc((IWineD3DSurfaceImpl *)swapchain->frontBuffer, pPresentationParameters);
        for(i = 0; i < swapchain->presentParms.BackBufferCount; i++) {
            updateSurfaceDesc((IWineD3DSurfaceImpl *)swapchain->backBuffer[i], pPresentationParameters);
        }

        /* Now set the new viewport */
        IWineD3DDevice_SetViewport(iface, &vp);
    }

    if((pPresentationParameters->Windowed && !swapchain->presentParms.Windowed) ||
       (swapchain->presentParms.Windowed && !pPresentationParameters->Windowed) ||
        DisplayModeChanged) {

        /* Switching to fullscreen? Change to fullscreen mode, THEN change the screen res */
        if(!pPresentationParameters->Windowed) {
            IWineD3DDevice_SetFullscreen(iface, TRUE);
        }

        IWineD3DDevice_SetDisplayMode(iface, 0, &mode);

        /* Switching out of fullscreen mode? First set the original res, then change the window */
        if(pPresentationParameters->Windowed) {
            IWineD3DDevice_SetFullscreen(iface, FALSE);
        }
        swapchain->presentParms.Windowed = pPresentationParameters->Windowed;
    }

    IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DDeviceImpl_SetDialogBoxMode(IWineD3DDevice *iface, BOOL bEnableDialogs) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    /** FIXME: always true at the moment **/
    if(!bEnableDialogs) {
        FIXME("(%p) Dialogs cannot be disabled yet\n", This);
    }
    return WINED3D_OK;
}


static HRESULT  WINAPI  IWineD3DDeviceImpl_GetCreationParameters(IWineD3DDevice *iface, WINED3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    TRACE("(%p) : pParameters %p\n", This, pParameters);

    *pParameters = This->createParms;
    return WINED3D_OK;
}

static void WINAPI IWineD3DDeviceImpl_SetGammaRamp(IWineD3DDevice * iface, UINT iSwapChain, DWORD Flags, CONST WINED3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;
    HRESULT hrc = WINED3D_OK;

    TRACE("Relaying  to swapchain\n");

    if ((hrc = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain)) == WINED3D_OK) {
        IWineD3DSwapChain_SetGammaRamp(swapchain, Flags, (WINED3DGAMMARAMP *)pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}

static void WINAPI IWineD3DDeviceImpl_GetGammaRamp(IWineD3DDevice *iface, UINT iSwapChain, WINED3DGAMMARAMP* pRamp) {
    IWineD3DSwapChain *swapchain;
    HRESULT hrc = WINED3D_OK;

    TRACE("Relaying  to swapchain\n");

    if ((hrc = IWineD3DDeviceImpl_GetSwapChain(iface, iSwapChain, &swapchain)) == WINED3D_OK) {
        hrc =IWineD3DSwapChain_GetGammaRamp(swapchain, pRamp);
        IWineD3DSwapChain_Release(swapchain);
    }
    return;
}


/** ********************************************************
*   Notification functions
** ********************************************************/
/** This function must be called in the release of a resource when ref == 0,
* the contents of resource must still be correct,
* any handels to other resource held by the caller must be closed
* (e.g. a texture should release all held surfaces because telling the device that it's been released.)
 *****************************************************/
static void WINAPI IWineD3DDeviceImpl_AddResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Adding Resource %p\n", This, resource);
    list_add_head(&This->resources, &((IWineD3DResourceImpl *) resource)->resource.resource_list_entry);
}

static void WINAPI IWineD3DDeviceImpl_RemoveResource(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : Removing resource %p\n", This, resource);

    list_remove(&((IWineD3DResourceImpl *) resource)->resource.resource_list_entry);
}


static void WINAPI IWineD3DDeviceImpl_ResourceReleased(IWineD3DDevice *iface, IWineD3DResource *resource){
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    int counter;

    TRACE("(%p) : resource %p\n", This, resource);
    switch(IWineD3DResource_GetType(resource)){
        /* TODO: check front and back buffers, rendertargets etc..  possibly swapchains? */
        case WINED3DRTYPE_SURFACE: {
            unsigned int i;

            /* Cleanup any FBO attachments if d3d is enabled */
            if(This->d3d_initialized) {
                if((IWineD3DSurface *)resource == This->lastActiveRenderTarget) {
                    IWineD3DSwapChainImpl *swapchain = This->swapchains ? (IWineD3DSwapChainImpl *) This->swapchains[0] : NULL;

                    TRACE("Last active render target destroyed\n");
                    /* Find a replacement surface for the currently active back buffer. The context manager does not do NULL
                     * checks, so switch to a valid target as long as the currently set surface is still valid. Use the
                     * surface of the implicit swpchain. If that is the same as the destroyed surface the device is destroyed
                     * and the lastActiveRenderTarget member shouldn't matter
                     */
                    if(swapchain) {
                        if(swapchain->backBuffer && swapchain->backBuffer[0] != (IWineD3DSurface *)resource) {
                            TRACE("Activating primary back buffer\n");
                            ActivateContext(This, swapchain->backBuffer[0], CTXUSAGE_RESOURCELOAD);
                        } else if(!swapchain->backBuffer && swapchain->frontBuffer != (IWineD3DSurface *)resource) {
                            /* Single buffering environment */
                            TRACE("Activating primary front buffer\n");
                            ActivateContext(This, swapchain->frontBuffer, CTXUSAGE_RESOURCELOAD);
                        } else {
                            TRACE("Device is being destroyed, setting lastActiveRenderTarget = 0xdeadbabe\n");
                            /* Implicit render target destroyed, that means the device is being destroyed
                             * whatever we set here, it shouldn't matter
                             */
                            This->lastActiveRenderTarget = (IWineD3DSurface *) 0xdeadbabe;
                        }
                    } else {
                        /* May happen during ddraw uninitialization */
                        TRACE("Render target set, but swapchain does not exist!\n");
                        This->lastActiveRenderTarget = (IWineD3DSurface *) 0xdeadcafe;
                    }
                }

                for (i = 0; i < GL_LIMITS(buffers); ++i) {
                    if (This->fbo_color_attachments[i] == (IWineD3DSurface *)resource) {
                        bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->fbo);
                        set_render_target_fbo(iface, i, NULL);
                        This->fbo_color_attachments[i] = NULL;
                    }
                }
                if (This->fbo_depth_attachment == (IWineD3DSurface *)resource) {
                    bind_fbo(iface, GL_FRAMEBUFFER_EXT, &This->fbo);
                    set_depth_stencil_fbo(iface, NULL);
                    This->fbo_depth_attachment = NULL;
                }
            }

            break;
        }
        case WINED3DRTYPE_TEXTURE:
        case WINED3DRTYPE_CUBETEXTURE:
        case WINED3DRTYPE_VOLUMETEXTURE:
                for (counter = 0; counter < MAX_COMBINED_SAMPLERS; counter++) {
                    if (This->stateBlock != NULL && This->stateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                        WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                        This->stateBlock->textures[counter] = NULL;
                    }
                    if (This->updateStateBlock != This->stateBlock ){
                        if (This->updateStateBlock->textures[counter] == (IWineD3DBaseTexture *)resource) {
                            WARN("Texture being released is still by a stateblock, Stage = %u Texture = %p\n", counter, resource);
                            This->updateStateBlock->textures[counter] = NULL;
                        }
                    }
                }
        break;
        case WINED3DRTYPE_VOLUME:
        /* TODO: nothing really? */
        break;
        case WINED3DRTYPE_VERTEXBUFFER:
        /* MSDN: When an application no longer holds a references to this interface, the interface will automatically be freed. */
        {
            int streamNumber;
            TRACE("Cleaning up stream pointers\n");

            for(streamNumber = 0; streamNumber < MAX_STREAMS; streamNumber ++){
                /* FINDOUT: should a warn be generated if were recording and updateStateBlock->streamSource is lost?
                FINDOUT: should changes.streamSource[StreamNumber] be set ?
                */
                if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
                    if ((IWineD3DResource *)This->updateStateBlock->streamSource[streamNumber] == resource) {
                        FIXME("Vertex buffer released while bound to a state block, stream %d\n", streamNumber);
                        This->updateStateBlock->streamSource[streamNumber] = 0;
                        /* Set changed flag? */
                    }
                }
                if (This->stateBlock != NULL ) { /* only happens if there is an error in the application, or on reset/release (because we don't manage internal tracking properly) */
                    if ((IWineD3DResource *)This->stateBlock->streamSource[streamNumber] == resource) {
                        TRACE("Vertex buffer released while bound to a state block, stream %d\n", streamNumber);
                        This->stateBlock->streamSource[streamNumber] = 0;
                    }
                }
#if 0   /* TODO: Manage internal tracking properly so that 'this shouldn't happen' */
                 else { /* This shouldn't happen */
                    FIXME("Calling application has released the device before relasing all the resources bound to the device\n");
                }
#endif

            }
        }
        break;
        case WINED3DRTYPE_INDEXBUFFER:
        /* MSDN: When an application no longer holds a references to this interface, the interface will automatically be freed.*/
        if (This->updateStateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->updateStateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->updateStateBlock->pIndexData =  NULL;
            }
        }
        if (This->stateBlock != NULL ) { /* ==NULL when device is being destroyed */
            if (This->stateBlock->pIndexData == (IWineD3DIndexBuffer *)resource) {
                This->stateBlock->pIndexData =  NULL;
            }
        }

        break;
        default:
        FIXME("(%p) unknown resource type %p %u\n", This, resource, IWineD3DResource_GetType(resource));
        break;
    }


    /* Remove the resoruce from the resourceStore */
    IWineD3DDeviceImpl_RemoveResource(iface, resource);

    TRACE("Resource released\n");

}

/**********************************************************
 * IWineD3DDevice VTbl follows
 **********************************************************/

const IWineD3DDeviceVtbl IWineD3DDevice_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DDeviceImpl_QueryInterface,
    IWineD3DDeviceImpl_AddRef,
    IWineD3DDeviceImpl_Release,
    /*** IWineD3DDevice methods ***/
    IWineD3DDeviceImpl_GetParent,
    /*** Creation methods**/
    IWineD3DDeviceImpl_CreateVertexBuffer,
    IWineD3DDeviceImpl_CreateIndexBuffer,
    IWineD3DDeviceImpl_CreateStateBlock,
    IWineD3DDeviceImpl_CreateSurface,
    IWineD3DDeviceImpl_CreateTexture,
    IWineD3DDeviceImpl_CreateVolumeTexture,
    IWineD3DDeviceImpl_CreateVolume,
    IWineD3DDeviceImpl_CreateCubeTexture,
    IWineD3DDeviceImpl_CreateQuery,
    IWineD3DDeviceImpl_CreateAdditionalSwapChain,
    IWineD3DDeviceImpl_CreateVertexDeclaration,
    IWineD3DDeviceImpl_CreateVertexDeclarationFromFVF,
    IWineD3DDeviceImpl_CreateVertexShader,
    IWineD3DDeviceImpl_CreatePixelShader,
    IWineD3DDeviceImpl_CreatePalette,
    /*** Odd functions **/
    IWineD3DDeviceImpl_Init3D,
    IWineD3DDeviceImpl_Uninit3D,
    IWineD3DDeviceImpl_SetFullscreen,
    IWineD3DDeviceImpl_SetMultithreaded,
    IWineD3DDeviceImpl_EvictManagedResources,
    IWineD3DDeviceImpl_GetAvailableTextureMem,
    IWineD3DDeviceImpl_GetBackBuffer,
    IWineD3DDeviceImpl_GetCreationParameters,
    IWineD3DDeviceImpl_GetDeviceCaps,
    IWineD3DDeviceImpl_GetDirect3D,
    IWineD3DDeviceImpl_GetDisplayMode,
    IWineD3DDeviceImpl_SetDisplayMode,
    IWineD3DDeviceImpl_GetHWND,
    IWineD3DDeviceImpl_SetHWND,
    IWineD3DDeviceImpl_GetNumberOfSwapChains,
    IWineD3DDeviceImpl_GetRasterStatus,
    IWineD3DDeviceImpl_GetSwapChain,
    IWineD3DDeviceImpl_Reset,
    IWineD3DDeviceImpl_SetDialogBoxMode,
    IWineD3DDeviceImpl_SetCursorProperties,
    IWineD3DDeviceImpl_SetCursorPosition,
    IWineD3DDeviceImpl_ShowCursor,
    IWineD3DDeviceImpl_TestCooperativeLevel,
    /*** Getters and setters **/
    IWineD3DDeviceImpl_SetClipPlane,
    IWineD3DDeviceImpl_GetClipPlane,
    IWineD3DDeviceImpl_SetClipStatus,
    IWineD3DDeviceImpl_GetClipStatus,
    IWineD3DDeviceImpl_SetCurrentTexturePalette,
    IWineD3DDeviceImpl_GetCurrentTexturePalette,
    IWineD3DDeviceImpl_SetDepthStencilSurface,
    IWineD3DDeviceImpl_GetDepthStencilSurface,
    IWineD3DDeviceImpl_SetFVF,
    IWineD3DDeviceImpl_GetFVF,
    IWineD3DDeviceImpl_SetGammaRamp,
    IWineD3DDeviceImpl_GetGammaRamp,
    IWineD3DDeviceImpl_SetIndices,
    IWineD3DDeviceImpl_GetIndices,
    IWineD3DDeviceImpl_SetBaseVertexIndex,
    IWineD3DDeviceImpl_GetBaseVertexIndex,
    IWineD3DDeviceImpl_SetLight,
    IWineD3DDeviceImpl_GetLight,
    IWineD3DDeviceImpl_SetLightEnable,
    IWineD3DDeviceImpl_GetLightEnable,
    IWineD3DDeviceImpl_SetMaterial,
    IWineD3DDeviceImpl_GetMaterial,
    IWineD3DDeviceImpl_SetNPatchMode,
    IWineD3DDeviceImpl_GetNPatchMode,
    IWineD3DDeviceImpl_SetPaletteEntries,
    IWineD3DDeviceImpl_GetPaletteEntries,
    IWineD3DDeviceImpl_SetPixelShader,
    IWineD3DDeviceImpl_GetPixelShader,
    IWineD3DDeviceImpl_SetPixelShaderConstantB,
    IWineD3DDeviceImpl_GetPixelShaderConstantB,
    IWineD3DDeviceImpl_SetPixelShaderConstantI,
    IWineD3DDeviceImpl_GetPixelShaderConstantI,
    IWineD3DDeviceImpl_SetPixelShaderConstantF,
    IWineD3DDeviceImpl_GetPixelShaderConstantF,
    IWineD3DDeviceImpl_SetRenderState,
    IWineD3DDeviceImpl_GetRenderState,
    IWineD3DDeviceImpl_SetRenderTarget,
    IWineD3DDeviceImpl_GetRenderTarget,
    IWineD3DDeviceImpl_SetFrontBackBuffers,
    IWineD3DDeviceImpl_SetSamplerState,
    IWineD3DDeviceImpl_GetSamplerState,
    IWineD3DDeviceImpl_SetScissorRect,
    IWineD3DDeviceImpl_GetScissorRect,
    IWineD3DDeviceImpl_SetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_GetSoftwareVertexProcessing,
    IWineD3DDeviceImpl_SetStreamSource,
    IWineD3DDeviceImpl_GetStreamSource,
    IWineD3DDeviceImpl_SetStreamSourceFreq,
    IWineD3DDeviceImpl_GetStreamSourceFreq,
    IWineD3DDeviceImpl_SetTexture,
    IWineD3DDeviceImpl_GetTexture,
    IWineD3DDeviceImpl_SetTextureStageState,
    IWineD3DDeviceImpl_GetTextureStageState,
    IWineD3DDeviceImpl_SetTransform,
    IWineD3DDeviceImpl_GetTransform,
    IWineD3DDeviceImpl_SetVertexDeclaration,
    IWineD3DDeviceImpl_GetVertexDeclaration,
    IWineD3DDeviceImpl_SetVertexShader,
    IWineD3DDeviceImpl_GetVertexShader,
    IWineD3DDeviceImpl_SetVertexShaderConstantB,
    IWineD3DDeviceImpl_GetVertexShaderConstantB,
    IWineD3DDeviceImpl_SetVertexShaderConstantI,
    IWineD3DDeviceImpl_GetVertexShaderConstantI,
    IWineD3DDeviceImpl_SetVertexShaderConstantF,
    IWineD3DDeviceImpl_GetVertexShaderConstantF,
    IWineD3DDeviceImpl_SetViewport,
    IWineD3DDeviceImpl_GetViewport,
    IWineD3DDeviceImpl_MultiplyTransform,
    IWineD3DDeviceImpl_ValidateDevice,
    IWineD3DDeviceImpl_ProcessVertices,
    /*** State block ***/
    IWineD3DDeviceImpl_BeginStateBlock,
    IWineD3DDeviceImpl_EndStateBlock,
    /*** Scene management ***/
    IWineD3DDeviceImpl_BeginScene,
    IWineD3DDeviceImpl_EndScene,
    IWineD3DDeviceImpl_Present,
    IWineD3DDeviceImpl_Clear,
    /*** Drawing ***/
    IWineD3DDeviceImpl_DrawPrimitive,
    IWineD3DDeviceImpl_DrawIndexedPrimitive,
    IWineD3DDeviceImpl_DrawPrimitiveUP,
    IWineD3DDeviceImpl_DrawIndexedPrimitiveUP,
    IWineD3DDeviceImpl_DrawPrimitiveStrided,
    IWineD3DDeviceImpl_DrawIndexedPrimitiveStrided,
    IWineD3DDeviceImpl_DrawRectPatch,
    IWineD3DDeviceImpl_DrawTriPatch,
    IWineD3DDeviceImpl_DeletePatch,
    IWineD3DDeviceImpl_ColorFill,
    IWineD3DDeviceImpl_UpdateTexture,
    IWineD3DDeviceImpl_UpdateSurface,
    IWineD3DDeviceImpl_GetFrontBufferData,
    /*** object tracking ***/
    IWineD3DDeviceImpl_ResourceReleased
};


const DWORD SavedPixelStates_R[NUM_SAVEDPIXELSTATES_R] = {
    WINED3DRS_ALPHABLENDENABLE   ,
    WINED3DRS_ALPHAFUNC          ,
    WINED3DRS_ALPHAREF           ,
    WINED3DRS_ALPHATESTENABLE    ,
    WINED3DRS_BLENDOP            ,
    WINED3DRS_COLORWRITEENABLE   ,
    WINED3DRS_DESTBLEND          ,
    WINED3DRS_DITHERENABLE       ,
    WINED3DRS_FILLMODE           ,
    WINED3DRS_FOGDENSITY         ,
    WINED3DRS_FOGEND             ,
    WINED3DRS_FOGSTART           ,
    WINED3DRS_LASTPIXEL          ,
    WINED3DRS_SHADEMODE          ,
    WINED3DRS_SRCBLEND           ,
    WINED3DRS_STENCILENABLE      ,
    WINED3DRS_STENCILFAIL        ,
    WINED3DRS_STENCILFUNC        ,
    WINED3DRS_STENCILMASK        ,
    WINED3DRS_STENCILPASS        ,
    WINED3DRS_STENCILREF         ,
    WINED3DRS_STENCILWRITEMASK   ,
    WINED3DRS_STENCILZFAIL       ,
    WINED3DRS_TEXTUREFACTOR      ,
    WINED3DRS_WRAP0              ,
    WINED3DRS_WRAP1              ,
    WINED3DRS_WRAP2              ,
    WINED3DRS_WRAP3              ,
    WINED3DRS_WRAP4              ,
    WINED3DRS_WRAP5              ,
    WINED3DRS_WRAP6              ,
    WINED3DRS_WRAP7              ,
    WINED3DRS_ZENABLE            ,
    WINED3DRS_ZFUNC              ,
    WINED3DRS_ZWRITEENABLE
};

const DWORD SavedPixelStates_T[NUM_SAVEDPIXELSTATES_T] = {
    WINED3DTSS_ADDRESSW              ,
    WINED3DTSS_ALPHAARG0             ,
    WINED3DTSS_ALPHAARG1             ,
    WINED3DTSS_ALPHAARG2             ,
    WINED3DTSS_ALPHAOP               ,
    WINED3DTSS_BUMPENVLOFFSET        ,
    WINED3DTSS_BUMPENVLSCALE         ,
    WINED3DTSS_BUMPENVMAT00          ,
    WINED3DTSS_BUMPENVMAT01          ,
    WINED3DTSS_BUMPENVMAT10          ,
    WINED3DTSS_BUMPENVMAT11          ,
    WINED3DTSS_COLORARG0             ,
    WINED3DTSS_COLORARG1             ,
    WINED3DTSS_COLORARG2             ,
    WINED3DTSS_COLOROP               ,
    WINED3DTSS_RESULTARG             ,
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedPixelStates_S[NUM_SAVEDPIXELSTATES_S] = {
    WINED3DSAMP_ADDRESSU         ,
    WINED3DSAMP_ADDRESSV         ,
    WINED3DSAMP_ADDRESSW         ,
    WINED3DSAMP_BORDERCOLOR      ,
    WINED3DSAMP_MAGFILTER        ,
    WINED3DSAMP_MINFILTER        ,
    WINED3DSAMP_MIPFILTER        ,
    WINED3DSAMP_MIPMAPLODBIAS    ,
    WINED3DSAMP_MAXMIPLEVEL      ,
    WINED3DSAMP_MAXANISOTROPY    ,
    WINED3DSAMP_SRGBTEXTURE      ,
    WINED3DSAMP_ELEMENTINDEX
};

const DWORD SavedVertexStates_R[NUM_SAVEDVERTEXSTATES_R] = {
    WINED3DRS_AMBIENT                       ,
    WINED3DRS_AMBIENTMATERIALSOURCE         ,
    WINED3DRS_CLIPPING                      ,
    WINED3DRS_CLIPPLANEENABLE               ,
    WINED3DRS_COLORVERTEX                   ,
    WINED3DRS_DIFFUSEMATERIALSOURCE         ,
    WINED3DRS_EMISSIVEMATERIALSOURCE        ,
    WINED3DRS_FOGDENSITY                    ,
    WINED3DRS_FOGEND                        ,
    WINED3DRS_FOGSTART                      ,
    WINED3DRS_FOGTABLEMODE                  ,
    WINED3DRS_FOGVERTEXMODE                 ,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE      ,
    WINED3DRS_LIGHTING                      ,
    WINED3DRS_LOCALVIEWER                   ,
    WINED3DRS_MULTISAMPLEANTIALIAS          ,
    WINED3DRS_MULTISAMPLEMASK               ,
    WINED3DRS_NORMALIZENORMALS              ,
    WINED3DRS_PATCHEDGESTYLE                ,
    WINED3DRS_POINTSCALE_A                  ,
    WINED3DRS_POINTSCALE_B                  ,
    WINED3DRS_POINTSCALE_C                  ,
    WINED3DRS_POINTSCALEENABLE              ,
    WINED3DRS_POINTSIZE                     ,
    WINED3DRS_POINTSIZE_MAX                 ,
    WINED3DRS_POINTSIZE_MIN                 ,
    WINED3DRS_POINTSPRITEENABLE             ,
    WINED3DRS_RANGEFOGENABLE                ,
    WINED3DRS_SPECULARMATERIALSOURCE        ,
    WINED3DRS_TWEENFACTOR                   ,
    WINED3DRS_VERTEXBLEND                   ,
    WINED3DRS_CULLMODE                      ,
    WINED3DRS_FOGCOLOR
};

const DWORD SavedVertexStates_T[NUM_SAVEDVERTEXSTATES_T] = {
    WINED3DTSS_TEXCOORDINDEX         ,
    WINED3DTSS_TEXTURETRANSFORMFLAGS
};

const DWORD SavedVertexStates_S[NUM_SAVEDVERTEXSTATES_S] = {
    WINED3DSAMP_DMAPOFFSET
};

void IWineD3DDeviceImpl_MarkStateDirty(IWineD3DDeviceImpl *This, DWORD state) {
    DWORD rep = StateTable[state].representative;
    DWORD idx;
    BYTE shift;
    UINT i;
    WineD3DContext *context;

    if(!rep) return;
    for(i = 0; i < This->numContexts; i++) {
        context = This->contexts[i];
        if(isStateDirty(context, rep)) continue;

        context->dirtyArray[context->numDirtyEntries++] = rep;
        idx = rep >> 5;
        shift = rep & 0x1f;
        context->isStateDirty[idx] |= (1 << shift);
    }
}
