/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->wineD3DDevice->adapter->gl_info

/***************************************
 * Stateblock helper functions follow
 **************************************/

/** Allocates the correct amount of space for pixel and vertex shader constants, 
 * along with their set/changed flags on the given stateblock object
 */
HRESULT allocate_shader_constants(IWineD3DStateBlockImpl* object) {
    
    IWineD3DStateBlockImpl *This = object;

#define WINED3D_MEMCHECK(_object) if (NULL == _object) { FIXME("Out of memory!\n"); return E_OUTOFMEMORY; }

    /* Allocate space for floating point constants */
    object->pixelShaderConstantF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(float) * GL_LIMITS(pshader_constantsF) * 4);
    WINED3D_MEMCHECK(object->pixelShaderConstantF);
    object->changed.pixelShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BOOL) * GL_LIMITS(pshader_constantsF));
    WINED3D_MEMCHECK(object->changed.pixelShaderConstantsF);
    object->vertexShaderConstantF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(float) * GL_LIMITS(vshader_constantsF) * 4);
    WINED3D_MEMCHECK(object->vertexShaderConstantF);
    object->changed.vertexShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BOOL) * GL_LIMITS(vshader_constantsF));
    WINED3D_MEMCHECK(object->changed.vertexShaderConstantsF);
    object->contained_vs_consts_f = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * GL_LIMITS(vshader_constantsF));
    WINED3D_MEMCHECK(object->contained_vs_consts_f);
    object->contained_ps_consts_f = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * GL_LIMITS(pshader_constantsF));
    WINED3D_MEMCHECK(object->contained_ps_consts_f);

    list_init(&object->set_vconstantsF);
    list_init(&object->set_pconstantsF);

#undef WINED3D_MEMCHECK

    return WINED3D_OK;
}

/** Copy all members of one stateblock to another */
void stateblock_savedstates_copy(
    IWineD3DStateBlock* iface,
    SAVEDSTATES* dest,
    SAVEDSTATES* source) {
    
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    unsigned bsize = sizeof(BOOL);

    /* Single values */
    dest->indices = source->indices;
    dest->material = source->material;
    dest->fvf = source->fvf;
    dest->viewport = source->viewport;
    dest->vertexDecl = source->vertexDecl;
    dest->pixelShader = source->pixelShader;
    dest->vertexShader = source->vertexShader;
    dest->scissorRect = dest->scissorRect;

    /* Fixed size arrays */
    memcpy(dest->streamSource, source->streamSource, bsize * MAX_STREAMS);
    memcpy(dest->streamFreq, source->streamFreq, bsize * MAX_STREAMS);
    memcpy(dest->textures, source->textures, bsize * MAX_COMBINED_SAMPLERS);
    memcpy(dest->transform, source->transform, bsize * (HIGHEST_TRANSFORMSTATE + 1));
    memcpy(dest->renderState, source->renderState, bsize * (WINEHIGHEST_RENDER_STATE + 1));
    memcpy(dest->textureState, source->textureState, bsize * MAX_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE + 1));
    memcpy(dest->samplerState, source->samplerState, bsize * MAX_COMBINED_SAMPLERS * (WINED3D_HIGHEST_SAMPLER_STATE + 1));
    memcpy(dest->clipplane, source->clipplane, bsize * MAX_CLIPPLANES);
    memcpy(dest->pixelShaderConstantsB, source->pixelShaderConstantsB, bsize * MAX_CONST_B);
    memcpy(dest->pixelShaderConstantsI, source->pixelShaderConstantsI, bsize * MAX_CONST_I);
    memcpy(dest->vertexShaderConstantsB, source->vertexShaderConstantsB, bsize * MAX_CONST_B);
    memcpy(dest->vertexShaderConstantsI, source->vertexShaderConstantsI, bsize * MAX_CONST_I);

    /* Dynamically sized arrays */
    memcpy(dest->pixelShaderConstantsF, source->pixelShaderConstantsF, bsize * GL_LIMITS(pshader_constantsF));
    memcpy(dest->vertexShaderConstantsF, source->vertexShaderConstantsF, bsize * GL_LIMITS(vshader_constantsF));
}

/** Set all members of a stateblock savedstate to the given value */
void stateblock_savedstates_set(
    IWineD3DStateBlock* iface,
    SAVEDSTATES* states,
    BOOL value) {
    
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    unsigned bsize = sizeof(BOOL);

    /* Single values */
    states->indices = value;
    states->material = value;
    states->fvf = value;
    states->viewport = value;
    states->vertexDecl = value;
    states->pixelShader = value;
    states->vertexShader = value;
    states->scissorRect = value;

    /* Fixed size arrays */
    memset(states->streamSource, value, bsize * MAX_STREAMS);
    memset(states->streamFreq, value, bsize * MAX_STREAMS);
    memset(states->textures, value, bsize * MAX_COMBINED_SAMPLERS);
    memset(states->transform, value, bsize * (HIGHEST_TRANSFORMSTATE + 1));
    memset(states->renderState, value, bsize * (WINEHIGHEST_RENDER_STATE + 1));
    memset(states->textureState, value, bsize * MAX_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE + 1));
    memset(states->samplerState, value, bsize * MAX_COMBINED_SAMPLERS * (WINED3D_HIGHEST_SAMPLER_STATE + 1));
    memset(states->clipplane, value, bsize * MAX_CLIPPLANES);
    memset(states->pixelShaderConstantsB, value, bsize * MAX_CONST_B);
    memset(states->pixelShaderConstantsI, value, bsize * MAX_CONST_I);
    memset(states->vertexShaderConstantsB, value, bsize * MAX_CONST_B);
    memset(states->vertexShaderConstantsI, value, bsize * MAX_CONST_I);

    /* Dynamically sized arrays */
    memset(states->pixelShaderConstantsF, value, bsize * GL_LIMITS(pshader_constantsF));
    memset(states->vertexShaderConstantsF, value, bsize * GL_LIMITS(vshader_constantsF));
}

void stateblock_copy(
    IWineD3DStateBlock* destination,
    IWineD3DStateBlock* source) {
    int l;

    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)source;
    IWineD3DStateBlockImpl *Dest = (IWineD3DStateBlockImpl *)destination;

    /* IUnknown fields */
    Dest->lpVtbl                = This->lpVtbl;
    Dest->ref                   = This->ref;

    /* IWineD3DStateBlock information */
    Dest->parent                = This->parent;
    Dest->wineD3DDevice         = This->wineD3DDevice;
    Dest->blockType             = This->blockType;

    /* Saved states */
    stateblock_savedstates_copy(source, &Dest->changed, &This->changed);

    /* Single items */
    Dest->fvf = This->fvf;
    Dest->vertexDecl = This->vertexDecl;
    Dest->vertexShader = This->vertexShader;
    Dest->streamIsUP = This->streamIsUP;
    Dest->pIndexData = This->pIndexData;
    Dest->baseVertexIndex = This->baseVertexIndex;
    /* Dest->lights = This->lights; */
    Dest->clip_status = This->clip_status;
    Dest->viewport = This->viewport;
    Dest->material = This->material;
    Dest->pixelShader = This->pixelShader;
    Dest->scissorRect = This->scissorRect;

    /* Lights */
    memset(This->activeLights, 0, sizeof(This->activeLights));
    for(l = 0; l < LIGHTMAP_SIZE; l++) {
        struct list *e1, *e2;
        LIST_FOR_EACH_SAFE(e1, e2, &Dest->lightMap[l]) {
            PLIGHTINFOEL *light = LIST_ENTRY(e1, PLIGHTINFOEL, entry);
            list_remove(&light->entry);
            HeapFree(GetProcessHeap(), 0, light);
        }

        LIST_FOR_EACH(e1, &This->lightMap[l]) {
            PLIGHTINFOEL *light = LIST_ENTRY(e1, PLIGHTINFOEL, entry), *light2;
            light2 = HeapAlloc(GetProcessHeap(), 0, sizeof(*light));
            *light2 = *light;
            list_add_tail(&Dest->lightMap[l], &light2->entry);
            if(light2->glIndex != -1) Dest->activeLights[light2->glIndex] = light2;
        }
    }

    /* Fixed size arrays */
    memcpy(Dest->vertexShaderConstantB, This->vertexShaderConstantB, sizeof(BOOL) * MAX_CONST_B);
    memcpy(Dest->vertexShaderConstantI, This->vertexShaderConstantI, sizeof(INT) * MAX_CONST_I * 4);
    memcpy(Dest->pixelShaderConstantB, This->pixelShaderConstantB, sizeof(BOOL) * MAX_CONST_B);
    memcpy(Dest->pixelShaderConstantI, This->pixelShaderConstantI, sizeof(INT) * MAX_CONST_I * 4);
    
    memcpy(Dest->streamStride, This->streamStride, sizeof(UINT) * MAX_STREAMS);
    memcpy(Dest->streamOffset, This->streamOffset, sizeof(UINT) * MAX_STREAMS);
    memcpy(Dest->streamSource, This->streamSource, sizeof(IWineD3DVertexBuffer*) * MAX_STREAMS);
    memcpy(Dest->streamFreq,   This->streamFreq,   sizeof(UINT) * MAX_STREAMS);
    memcpy(Dest->streamFlags,  This->streamFlags,  sizeof(UINT) * MAX_STREAMS);
    memcpy(Dest->transforms,   This->transforms,   sizeof(WINED3DMATRIX) * (HIGHEST_TRANSFORMSTATE + 1));
    memcpy(Dest->clipplane,    This->clipplane,    sizeof(double) * MAX_CLIPPLANES * 4);
    memcpy(Dest->renderState,  This->renderState,  sizeof(DWORD) * (WINEHIGHEST_RENDER_STATE + 1));
    memcpy(Dest->textures,     This->textures,     sizeof(IWineD3DBaseTexture*) * MAX_COMBINED_SAMPLERS);
    memcpy(Dest->textureDimensions, This->textureDimensions, sizeof(int) * MAX_COMBINED_SAMPLERS);
    memcpy(Dest->textureState, This->textureState, sizeof(DWORD) * MAX_TEXTURES * (WINED3D_HIGHEST_TEXTURE_STATE + 1));
    memcpy(Dest->samplerState, This->samplerState, sizeof(DWORD) * MAX_COMBINED_SAMPLERS * (WINED3D_HIGHEST_SAMPLER_STATE + 1));

    /* Dynamically sized arrays */
    memcpy(Dest->vertexShaderConstantF, This->vertexShaderConstantF, sizeof(float) * GL_LIMITS(vshader_constantsF) * 4);
    memcpy(Dest->pixelShaderConstantF,  This->pixelShaderConstantF,  sizeof(float) * GL_LIMITS(pshader_constantsF) * 4);
}

/**********************************************************
 * IWineD3DStateBlockImpl IUnknown parts follows
 **********************************************************/
static HRESULT  WINAPI IWineD3DStateBlockImpl_QueryInterface(IWineD3DStateBlock *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DStateBlock)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DStateBlockImpl_AddRef(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG  WINAPI IWineD3DStateBlockImpl_Release(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %d\n", This, refCount + 1);

    if (!refCount) {
        constants_entry *constant, *constant2;
        int counter;

        /* type 0 represents the primary stateblock, so free all the resources */
        if (This->blockType == WINED3DSBT_INIT) {
            /* NOTE: according to MSDN: The application is responsible for making sure the texture references are cleared down */
            for (counter = 0; counter < MAX_COMBINED_SAMPLERS; counter++) {
                if (This->textures[counter]) {
                    /* release our 'internal' hold on the texture */
                    if(0 != IWineD3DBaseTexture_Release(This->textures[counter])) {
                        TRACE("Texture still referenced by stateblock, applications has leaked Stage = %u Texture = %p\n", counter, This->textures[counter]);
                    }
                }
            }
        }

        for (counter = 0; counter < MAX_STREAMS; counter++) {
            if(This->streamSource[counter]) {
                if(0 != IWineD3DVertexBuffer_Release(This->streamSource[counter])) {
                    TRACE("Vertex buffer still referenced by stateblock, applications has leaked Stream %u, buffer %p\n", counter, This->streamSource[counter]);
                }
            }
        }
        if(This->pIndexData) IWineD3DIndexBuffer_Release(This->pIndexData);
        if(This->vertexShader) IWineD3DVertexShader_Release(This->vertexShader);
        if(This->pixelShader) IWineD3DPixelShader_Release(This->pixelShader);

        for(counter = 0; counter < LIGHTMAP_SIZE; counter++) {
            struct list *e1, *e2;
            LIST_FOR_EACH_SAFE(e1, e2, &This->lightMap[counter]) {
                PLIGHTINFOEL *light = LIST_ENTRY(e1, PLIGHTINFOEL, entry);
                list_remove(&light->entry);
                HeapFree(GetProcessHeap(), 0, light);
            }
        }

        HeapFree(GetProcessHeap(), 0, This->vertexShaderConstantF);
        HeapFree(GetProcessHeap(), 0, This->changed.vertexShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, This->pixelShaderConstantF);
        HeapFree(GetProcessHeap(), 0, This->changed.pixelShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, This->contained_vs_consts_f);
        HeapFree(GetProcessHeap(), 0, This->contained_ps_consts_f);

        LIST_FOR_EACH_ENTRY_SAFE(constant, constant2, &This->set_vconstantsF, constants_entry, entry) {
            HeapFree(GetProcessHeap(), 0, constant);
        }

        LIST_FOR_EACH_ENTRY_SAFE(constant, constant2, &This->set_pconstantsF, constants_entry, entry) {
            HeapFree(GetProcessHeap(), 0, constant);
        }

        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/**********************************************************
 * IWineD3DStateBlockImpl parts follows
 **********************************************************/
static HRESULT  WINAPI IWineD3DStateBlockImpl_GetParent(IWineD3DStateBlock *iface, IUnknown **pParent) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IUnknown_AddRef(This->parent);
    *pParent = This->parent;
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DStateBlockImpl_GetDevice(IWineD3DStateBlock *iface, IWineD3DDevice** ppDevice){

    IWineD3DStateBlockImpl *This   = (IWineD3DStateBlockImpl *)iface;

    *ppDevice = (IWineD3DDevice*)This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);
    return WINED3D_OK;

}

static inline void record_lights(IWineD3DStateBlockImpl *This, IWineD3DStateBlockImpl *targetStateBlock) {
    UINT i;

    /* Lights... For a recorded state block, we just had a chain of actions to perform,
     * so we need to walk that chain and update any actions which differ
     */
    for(i = 0; i < LIGHTMAP_SIZE; i++) {
        struct list *e, *f;
        LIST_FOR_EACH(e, &This->lightMap[i]) {
            BOOL updated = FALSE;
            PLIGHTINFOEL *src = LIST_ENTRY(e, PLIGHTINFOEL, entry), *realLight;
            if(!src->changed || !src->enabledChanged) continue;

            /* Look up the light in the destination */
            LIST_FOR_EACH(f, &targetStateBlock->lightMap[i]) {
                realLight = LIST_ENTRY(f, PLIGHTINFOEL, entry);
                if(realLight->OriginalIndex == src->OriginalIndex) {
                    if(src->changed) {
                        src->OriginalParms = realLight->OriginalParms;
                    }
                    if(src->enabledChanged) {
                            /* Need to double check because enabledChanged does not catch enabled -> disabled -> enabled
                        * or disabled -> enabled -> disabled changes
                            */
                        if(realLight->glIndex == -1 && src->glIndex != -1) {
                            /* Light disabled */
                            This->activeLights[src->glIndex] = NULL;
                        } else if(realLight->glIndex != -1 && src->glIndex == -1){
                            /* Light enabled */
                            This->activeLights[realLight->glIndex] = src;
                        }
                        src->glIndex = realLight->glIndex;
                    }
                    updated = TRUE;
                    break;
                }
            }

            if(updated) {
                /* Found a light, all done, proceed with next hash entry */
                continue;
            } else if(src->changed) {
                /* Otherwise assign defaul params */
                src->OriginalParms = WINED3D_default_light;
            } else {
                /* Not enabled by default */
                src->glIndex = -1;
            }
        }
    }
}

static HRESULT  WINAPI IWineD3DStateBlockImpl_Capture(IWineD3DStateBlock *iface){

    IWineD3DStateBlockImpl *This             = (IWineD3DStateBlockImpl *)iface;
    IWineD3DStateBlockImpl *targetStateBlock = This->wineD3DDevice->stateBlock;
    unsigned int i, j;

    TRACE("(%p) : Updating state block %p ------------------v\n", targetStateBlock, This);

    /* If not recorded, then update can just recapture */
    if (This->blockType == WINED3DSBT_RECORDED) {

        /* Recorded => Only update 'changed' values */
        if (This->changed.vertexShader && This->vertexShader != targetStateBlock->vertexShader) {
            TRACE("Updating vertex shader from %p to %p\n", This->vertexShader, targetStateBlock->vertexShader);

            if(targetStateBlock->vertexShader) IWineD3DVertexShader_AddRef(targetStateBlock->vertexShader);
            if(This->vertexShader) IWineD3DVertexShader_Release(This->vertexShader);
            This->vertexShader = targetStateBlock->vertexShader;
        }

        /* Vertex Shader Float Constants */
        for (j = 0; j < This->num_contained_vs_consts_f; ++j) {
            i = This->contained_vs_consts_f[j];
            TRACE("Setting %p from %p %d to { %f, %f, %f, %f }\n", This, targetStateBlock, i,
                targetStateBlock->vertexShaderConstantF[i * 4],
                targetStateBlock->vertexShaderConstantF[i * 4 + 1],
                targetStateBlock->vertexShaderConstantF[i * 4 + 2],
                targetStateBlock->vertexShaderConstantF[i * 4 + 3]);

            This->vertexShaderConstantF[i * 4]      = targetStateBlock->vertexShaderConstantF[i * 4];
            This->vertexShaderConstantF[i * 4 + 1]  = targetStateBlock->vertexShaderConstantF[i * 4 + 1];
            This->vertexShaderConstantF[i * 4 + 2]  = targetStateBlock->vertexShaderConstantF[i * 4 + 2];
            This->vertexShaderConstantF[i * 4 + 3]  = targetStateBlock->vertexShaderConstantF[i * 4 + 3];
        }

        /* Vertex Shader Integer Constants */
        for (j = 0; j < This->num_contained_vs_consts_i; ++j) {
            i = This->contained_vs_consts_i[j];
            TRACE("Setting %p from %p %d to { %d, %d, %d, %d }\n", This, targetStateBlock, i,
                targetStateBlock->vertexShaderConstantI[i * 4],
                targetStateBlock->vertexShaderConstantI[i * 4 + 1],
                targetStateBlock->vertexShaderConstantI[i * 4 + 2],
                targetStateBlock->vertexShaderConstantI[i * 4 + 3]);

            This->vertexShaderConstantI[i * 4]      = targetStateBlock->vertexShaderConstantI[i * 4];
            This->vertexShaderConstantI[i * 4 + 1]  = targetStateBlock->vertexShaderConstantI[i * 4 + 1];
            This->vertexShaderConstantI[i * 4 + 2]  = targetStateBlock->vertexShaderConstantI[i * 4 + 2];
            This->vertexShaderConstantI[i * 4 + 3]  = targetStateBlock->vertexShaderConstantI[i * 4 + 3];
        }

        /* Vertex Shader Boolean Constants */
        for (j = 0; j < This->num_contained_vs_consts_b; ++j) {
            i = This->contained_vs_consts_b[j];
            TRACE("Setting %p from %p %d to %s\n", This, targetStateBlock, i,
                targetStateBlock->vertexShaderConstantB[i]? "TRUE":"FALSE");

            This->vertexShaderConstantB[i] =  targetStateBlock->vertexShaderConstantB[i];
        }

        /* Pixel Shader Float Constants */
        for (j = 0; j < This->num_contained_ps_consts_f; ++j) {
            i = This->contained_ps_consts_f[j];
            TRACE("Setting %p from %p %d to { %f, %f, %f, %f }\n", This, targetStateBlock, i,
                targetStateBlock->pixelShaderConstantF[i * 4],
                targetStateBlock->pixelShaderConstantF[i * 4 + 1],
                targetStateBlock->pixelShaderConstantF[i * 4 + 2],
                targetStateBlock->pixelShaderConstantF[i * 4 + 3]);

            This->pixelShaderConstantF[i * 4]      = targetStateBlock->pixelShaderConstantF[i * 4];
            This->pixelShaderConstantF[i * 4 + 1]  = targetStateBlock->pixelShaderConstantF[i * 4 + 1];
            This->pixelShaderConstantF[i * 4 + 2]  = targetStateBlock->pixelShaderConstantF[i * 4 + 2];
            This->pixelShaderConstantF[i * 4 + 3]  = targetStateBlock->pixelShaderConstantF[i * 4 + 3];
        }

        /* Pixel Shader Integer Constants */
        for (j = 0; j < This->num_contained_ps_consts_i; ++j) {
            i = This->contained_ps_consts_i[j];
            TRACE("Setting %p from %p %d to { %d, %d, %d, %d }\n", This, targetStateBlock, i,
                targetStateBlock->pixelShaderConstantI[i * 4],
                targetStateBlock->pixelShaderConstantI[i * 4 + 1],
                targetStateBlock->pixelShaderConstantI[i * 4 + 2],
                targetStateBlock->pixelShaderConstantI[i * 4 + 3]);

            This->pixelShaderConstantI[i * 4]      = targetStateBlock->pixelShaderConstantI[i * 4];
            This->pixelShaderConstantI[i * 4 + 1]  = targetStateBlock->pixelShaderConstantI[i * 4 + 1];
            This->pixelShaderConstantI[i * 4 + 2]  = targetStateBlock->pixelShaderConstantI[i * 4 + 2];
            This->pixelShaderConstantI[i * 4 + 3]  = targetStateBlock->pixelShaderConstantI[i * 4 + 3];
        }

        /* Pixel Shader Boolean Constants */
        for (j = 0; j < This->num_contained_ps_consts_b; ++j) {
            i = This->contained_ps_consts_b[j];
            TRACE("Setting %p from %p %d to %s\n", This, targetStateBlock, i,
                targetStateBlock->pixelShaderConstantB[i]? "TRUE":"FALSE");

            This->pixelShaderConstantB[i] =  targetStateBlock->pixelShaderConstantB[i];
        }

        /* Others + Render & Texture */
        for (i = 0; i < This->num_contained_transform_states; i++) {
            TRACE("Updating transform %d\n", i);
            This->transforms[This->contained_transform_states[i]] =
                targetStateBlock->transforms[This->contained_transform_states[i]];
        }

        if (This->changed.indices && ((This->pIndexData != targetStateBlock->pIndexData)
                        || (This->baseVertexIndex != targetStateBlock->baseVertexIndex))) {
            TRACE("Updating pindexData to %p, baseVertexIndex to %d\n",
                  targetStateBlock->pIndexData, targetStateBlock->baseVertexIndex);
            if(targetStateBlock->pIndexData) IWineD3DIndexBuffer_AddRef(targetStateBlock->pIndexData);
            if(This->pIndexData) IWineD3DIndexBuffer_Release(This->pIndexData);
            This->pIndexData = targetStateBlock->pIndexData;
            This->baseVertexIndex = targetStateBlock->baseVertexIndex;
        }

        if(This->changed.vertexDecl && This->vertexDecl != targetStateBlock->vertexDecl){
            TRACE("Updating vertex declaration from %p to %p\n", This->vertexDecl, targetStateBlock->vertexDecl);

            This->vertexDecl = targetStateBlock->vertexDecl;
        }

        if(This->changed.fvf && This->fvf != targetStateBlock->fvf){
            This->fvf = targetStateBlock->fvf;
        }

        if (This->changed.material && memcmp(&targetStateBlock->material,
                                                    &This->material,
                                                    sizeof(WINED3DMATERIAL)) != 0) {
            TRACE("Updating material\n");
            This->material = targetStateBlock->material;
        }

        if (This->changed.viewport && memcmp(&targetStateBlock->viewport,
                                                    &This->viewport,
                                                    sizeof(WINED3DVIEWPORT)) != 0) {
            TRACE("Updating viewport\n");
            This->viewport = targetStateBlock->viewport;
        }

        if(This->changed.scissorRect && memcmp(&targetStateBlock->scissorRect,
                                           &This->scissorRect,
                                           sizeof(targetStateBlock->scissorRect)))
        {
            TRACE("Updating scissor rect\n");
            targetStateBlock->scissorRect = This->scissorRect;
        }

        for (i = 0; i < MAX_STREAMS; i++) {
            if (This->changed.streamSource[i] &&
                            ((This->streamStride[i] != targetStateBlock->streamStride[i]) ||
                            (This->streamSource[i] != targetStateBlock->streamSource[i]))) {
                TRACE("Updating stream source %d to %p, stride to %d\n", i, targetStateBlock->streamSource[i],
                                                                            targetStateBlock->streamStride[i]);
                This->streamStride[i] = targetStateBlock->streamStride[i];
                if(targetStateBlock->streamSource[i]) IWineD3DVertexBuffer_AddRef(targetStateBlock->streamSource[i]);
                if(This->streamSource[i]) IWineD3DVertexBuffer_Release(This->streamSource[i]);
                This->streamSource[i] = targetStateBlock->streamSource[i];
            }

            if (This->changed.streamFreq[i] &&
            (This->streamFreq[i] != targetStateBlock->streamFreq[i]
            || This->streamFlags[i] != targetStateBlock->streamFlags[i])){
                    TRACE("Updating stream frequency %d to %d flags to %d\n", i ,  targetStateBlock->streamFreq[i] ,
                                                                                   targetStateBlock->streamFlags[i]);
                    This->streamFreq[i]  =  targetStateBlock->streamFreq[i];
                    This->streamFlags[i] =  targetStateBlock->streamFlags[i];
            }
        }

        for (i = 0; i < GL_LIMITS(clipplanes); i++) {
            if (This->changed.clipplane[i] && memcmp(&targetStateBlock->clipplane[i],
                                                        &This->clipplane[i],
                                                        sizeof(This->clipplane)) != 0) {

                TRACE("Updating clipplane %d\n", i);
                memcpy(&This->clipplane[i], &targetStateBlock->clipplane[i],
                                        sizeof(This->clipplane));
            }
        }

        /* Render */
        for (i = 0; i < This->num_contained_render_states; i++) {
            TRACE("Updating renderState %d to %d\n",
                  This->contained_render_states[i], targetStateBlock->renderState[This->contained_render_states[i]]);
            This->renderState[This->contained_render_states[i]] = targetStateBlock->renderState[This->contained_render_states[i]];
        }

        /* Texture states */
        for (j = 0; j < This->num_contained_tss_states; j++) {
            DWORD stage = This->contained_tss_states[j].stage;
            DWORD state = This->contained_tss_states[j].state;

            TRACE("Updating texturestagestate %d,%d to %d (was %d)\n", stage,state,
                  targetStateBlock->textureState[stage][state], This->textureState[stage][state]);
                    This->textureState[stage][state] =  targetStateBlock->textureState[stage][state];
        }

        /* Samplers */
        /* TODO: move over to using memcpy */
        for (j = 0; j < MAX_COMBINED_SAMPLERS; j++) {
            if (This->changed.textures[j]) {
                TRACE("Updating texture %d to %p (was %p)\n", j, targetStateBlock->textures[j],  This->textures[j]);
                This->textures[j] = targetStateBlock->textures[j];
            }
        }

        for (j = 0; j < This->num_contained_sampler_states; j++) {
            DWORD stage = This->contained_sampler_states[j].stage;
            DWORD state = This->contained_sampler_states[j].state;
            TRACE("Updating sampler state %d,%d to %d (was %d)\n",
                stage, state, targetStateBlock->samplerState[stage][state],
                This->samplerState[stage][state]);
            This->samplerState[stage][state] = targetStateBlock->samplerState[stage][state];
        }
        if(This->changed.pixelShader && This->pixelShader != targetStateBlock->pixelShader) {
            if(targetStateBlock->pixelShader) IWineD3DPixelShader_AddRef(targetStateBlock->pixelShader);
            if(This->pixelShader) IWineD3DPixelShader_Release(This->pixelShader);
            This->pixelShader = targetStateBlock->pixelShader;
        }

        record_lights(This, targetStateBlock);
    } else if(This->blockType == WINED3DSBT_ALL) {
        This->vertexDecl = targetStateBlock->vertexDecl;
        memcpy(This->vertexShaderConstantB, targetStateBlock->vertexShaderConstantB, sizeof(This->vertexShaderConstantI));
        memcpy(This->vertexShaderConstantI, targetStateBlock->vertexShaderConstantI, sizeof(This->vertexShaderConstantF));
        memcpy(This->vertexShaderConstantF, targetStateBlock->vertexShaderConstantF, sizeof(float) * GL_LIMITS(vshader_constantsF) * 4);
        memcpy(This->streamStride, targetStateBlock->streamStride, sizeof(This->streamStride));
        memcpy(This->streamOffset, targetStateBlock->streamOffset, sizeof(This->streamOffset));
        memcpy(This->streamFreq, targetStateBlock->streamFreq, sizeof(This->streamFreq));
        memcpy(This->streamFlags, targetStateBlock->streamFlags, sizeof(This->streamFlags));
        This->baseVertexIndex = targetStateBlock->baseVertexIndex;
        memcpy(This->transforms, targetStateBlock->transforms, sizeof(This->transforms));
        record_lights(This, targetStateBlock);
        memcpy(This->clipplane, targetStateBlock->clipplane, sizeof(This->clipplane));
        This->clip_status = targetStateBlock->clip_status;
        This->viewport = targetStateBlock->viewport;
        This->material = targetStateBlock->material;
        memcpy(This->pixelShaderConstantB, targetStateBlock->pixelShaderConstantB, sizeof(This->pixelShaderConstantI));
        memcpy(This->pixelShaderConstantI, targetStateBlock->pixelShaderConstantI, sizeof(This->pixelShaderConstantF));
        memcpy(This->pixelShaderConstantF, targetStateBlock->pixelShaderConstantF, sizeof(float) * GL_LIMITS(pshader_constantsF) * 4);
        memcpy(This->renderState, targetStateBlock->renderState, sizeof(This->renderState));
        memcpy(This->textures, targetStateBlock->textures, sizeof(This->textures));
        memcpy(This->textureDimensions, targetStateBlock->textureDimensions, sizeof(This->textureDimensions));
        memcpy(This->textureState, targetStateBlock->textureState, sizeof(This->textureState));
        memcpy(This->samplerState, targetStateBlock->samplerState, sizeof(This->samplerState));
        This->scissorRect = targetStateBlock->scissorRect;

        if(targetStateBlock->pIndexData != This->pIndexData) {
            if(targetStateBlock->pIndexData) IWineD3DIndexBuffer_AddRef(targetStateBlock->pIndexData);
            if(This->pIndexData) IWineD3DIndexBuffer_Release(This->pIndexData);
            This->pIndexData = targetStateBlock->pIndexData;
        }
        for(i = 0; i < MAX_STREAMS; i++) {
            if(targetStateBlock->streamSource[i] != This->streamSource[i]) {
                if(targetStateBlock->streamSource[i]) IWineD3DVertexBuffer_AddRef(targetStateBlock->streamSource[i]);
                if(This->streamSource[i]) IWineD3DVertexBuffer_Release(This->streamSource[i]);
                This->streamSource[i] = targetStateBlock->streamSource[i];
            }
        }
        if(This->vertexShader != targetStateBlock->vertexShader) {
            if(targetStateBlock->vertexShader) IWineD3DVertexShader_AddRef(targetStateBlock->vertexShader);
            if(This->vertexShader) IWineD3DVertexShader_Release(This->vertexShader);
            This->vertexShader = targetStateBlock->vertexShader;
        }
        if(This->pixelShader != targetStateBlock->pixelShader) {
            if(targetStateBlock->pixelShader) IWineD3DPixelShader_AddRef(targetStateBlock->pixelShader);
            if(This->pixelShader) IWineD3DPixelShader_Release(This->pixelShader);
            This->pixelShader = targetStateBlock->pixelShader;
        }
    } else if(This->blockType == WINED3DSBT_VERTEXSTATE) {
        memcpy(This->vertexShaderConstantB, targetStateBlock->vertexShaderConstantB, sizeof(This->vertexShaderConstantI));
        memcpy(This->vertexShaderConstantI, targetStateBlock->vertexShaderConstantI, sizeof(This->vertexShaderConstantF));
        memcpy(This->vertexShaderConstantF, targetStateBlock->vertexShaderConstantF, sizeof(float) * GL_LIMITS(vshader_constantsF) * 4);
        record_lights(This, targetStateBlock);
        for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            This->renderState[SavedVertexStates_R[i]] = targetStateBlock->renderState[SavedVertexStates_R[i]];
        }
        for (j = 0; j < MAX_COMBINED_SAMPLERS; j++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_S; i++) {
                This->samplerState[j][SavedVertexStates_S[i]] = targetStateBlock->samplerState[j][SavedVertexStates_S[i]];
            }
        }
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
                This->textureState[j][SavedVertexStates_R[i]] = targetStateBlock->textureState[j][SavedVertexStates_R[i]];
            }
        }
        for(i = 0; i < MAX_STREAMS; i++) {
            if(targetStateBlock->streamSource[i] != This->streamSource[i]) {
                if(targetStateBlock->streamSource[i]) IWineD3DVertexBuffer_AddRef(targetStateBlock->streamSource[i]);
                if(This->streamSource[i]) IWineD3DVertexBuffer_Release(This->streamSource[i]);
                This->streamSource[i] = targetStateBlock->streamSource[i];
            }
        }
        if(This->vertexShader != targetStateBlock->vertexShader) {
            if(targetStateBlock->vertexShader) IWineD3DVertexShader_AddRef(targetStateBlock->vertexShader);
            if(This->vertexShader) IWineD3DVertexShader_Release(This->vertexShader);
            This->vertexShader = targetStateBlock->vertexShader;
        }
    } else if(This->blockType == WINED3DSBT_PIXELSTATE) {
        memcpy(This->pixelShaderConstantB, targetStateBlock->pixelShaderConstantB, sizeof(This->pixelShaderConstantI));
        memcpy(This->pixelShaderConstantI, targetStateBlock->pixelShaderConstantI, sizeof(This->pixelShaderConstantF));
        memcpy(This->pixelShaderConstantF, targetStateBlock->pixelShaderConstantF, sizeof(float) * GL_LIMITS(pshader_constantsF) * 4);
        for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            This->renderState[SavedPixelStates_R[i]] = targetStateBlock->renderState[SavedPixelStates_R[i]];
        }
        for (j = 0; j < MAX_COMBINED_SAMPLERS; j++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_S; i++) {
                This->samplerState[j][SavedPixelStates_S[i]] = targetStateBlock->samplerState[j][SavedPixelStates_S[i]];
            }
        }
        for (j = 0; j < MAX_TEXTURES; j++) {
            for (i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
                This->textureState[j][SavedPixelStates_R[i]] = targetStateBlock->textureState[j][SavedPixelStates_R[i]];
            }
        }
        if(This->pixelShader != targetStateBlock->pixelShader) {
            if(targetStateBlock->pixelShader) IWineD3DPixelShader_AddRef(targetStateBlock->pixelShader);
            if(This->pixelShader) IWineD3DPixelShader_Release(This->pixelShader);
            This->pixelShader = targetStateBlock->pixelShader;
        }
    }

    TRACE("(%p) : Updated state block %p ------------------^\n", targetStateBlock, This);

    return WINED3D_OK;
}

static inline void apply_lights(IWineD3DDevice *pDevice, IWineD3DStateBlockImpl *This) {
    UINT i;
    for(i = 0; i < LIGHTMAP_SIZE; i++) {
        struct list *e;

        LIST_FOR_EACH(e, &This->lightMap[i]) {
            PLIGHTINFOEL *light = LIST_ENTRY(e, PLIGHTINFOEL, entry);

            if(light->changed) {
                IWineD3DDevice_SetLight(pDevice, light->OriginalIndex, &light->OriginalParms);
            }
            if(light->enabledChanged) {
                IWineD3DDevice_SetLightEnable(pDevice, light->OriginalIndex, light->glIndex != -1);
            }
        }
    }
}

static HRESULT  WINAPI IWineD3DStateBlockImpl_Apply(IWineD3DStateBlock *iface){
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IWineD3DDevice*        pDevice     = (IWineD3DDevice*)This->wineD3DDevice;

/*Copy thing over to updateBlock is isRecording otherwise StateBlock,
should really perform a delta so that only the changes get updated*/

    UINT i;
    UINT j;

    TRACE("(%p) : Applying state block %p ------------------v\n", This, pDevice);

    TRACE("Blocktype: %d\n", This->blockType);

    if(This->blockType == WINED3DSBT_RECORDED) {
        if (This->changed.vertexShader) {
            IWineD3DDevice_SetVertexShader(pDevice, This->vertexShader);
        }
        /* Vertex Shader Constants */
        for (i = 0; i < This->num_contained_vs_consts_f; i++) {
            IWineD3DDevice_SetVertexShaderConstantF(pDevice, This->contained_vs_consts_f[i],
                    This->vertexShaderConstantF + This->contained_vs_consts_f[i] * 4, 1);
        }
        for (i = 0; i < This->num_contained_vs_consts_i; i++) {
            IWineD3DDevice_SetVertexShaderConstantI(pDevice, This->contained_vs_consts_i[i],
                    This->vertexShaderConstantI + This->contained_vs_consts_i[i] * 4, 1);
        }
        for (i = 0; i < This->num_contained_vs_consts_b; i++) {
            IWineD3DDevice_SetVertexShaderConstantB(pDevice, This->contained_vs_consts_b[i],
                    This->vertexShaderConstantB + This->contained_vs_consts_b[i], 1);
        }

        apply_lights(pDevice, This);

        if (This->changed.pixelShader) {
            IWineD3DDevice_SetPixelShader(pDevice, This->pixelShader);
        }
        /* Pixel Shader Constants */
        for (i = 0; i < This->num_contained_ps_consts_f; i++) {
            IWineD3DDevice_SetPixelShaderConstantF(pDevice, This->contained_ps_consts_f[i],
                    This->pixelShaderConstantF + This->contained_ps_consts_f[i] * 4, 1);
        }
        for (i = 0; i < This->num_contained_ps_consts_i; i++) {
            IWineD3DDevice_SetPixelShaderConstantI(pDevice, This->contained_ps_consts_i[i],
                    This->pixelShaderConstantI + This->contained_ps_consts_i[i] * 4, 1);
        }
        for (i = 0; i < This->num_contained_ps_consts_b; i++) {
            IWineD3DDevice_SetPixelShaderConstantB(pDevice, This->contained_ps_consts_b[i],
                    This->pixelShaderConstantB + This->contained_ps_consts_b[i], 1);
        }

        /* Render */
        for (i = 0; i <= This->num_contained_render_states; i++) {
            IWineD3DDevice_SetRenderState(pDevice, This->contained_render_states[i],
                                          This->renderState[This->contained_render_states[i]]);
        }
        /* Texture states */
        for (i = 0; i < This->num_contained_tss_states; i++) {
            DWORD stage = This->contained_tss_states[i].stage;
            DWORD state = This->contained_tss_states[i].state;
            ((IWineD3DDeviceImpl *)pDevice)->stateBlock->textureState[stage][state]         = This->textureState[stage][state];
            ((IWineD3DDeviceImpl *)pDevice)->stateBlock->changed.textureState[stage][state] = TRUE;
            /* TODO: Record a display list to apply all gl states. For now apply by brute force */
            IWineD3DDeviceImpl_MarkStateDirty((IWineD3DDeviceImpl *)pDevice, STATE_TEXTURESTAGE(stage, state));
        }
        /* Sampler states */
        for (i = 0; i < This->num_contained_sampler_states; i++) {
            DWORD stage = This->contained_sampler_states[i].stage;
            DWORD state = This->contained_sampler_states[i].state;
            ((IWineD3DDeviceImpl *)pDevice)->stateBlock->samplerState[stage][state]         = This->samplerState[stage][state];
            ((IWineD3DDeviceImpl *)pDevice)->stateBlock->changed.samplerState[stage][state] = TRUE;
            IWineD3DDeviceImpl_MarkStateDirty((IWineD3DDeviceImpl *)pDevice, STATE_SAMPLER(stage));
        }

        for (i = 0; i < This->num_contained_transform_states; i++) {
            IWineD3DDevice_SetTransform(pDevice, This->contained_transform_states[i],
                                        &This->transforms[This->contained_transform_states[i]]);
        }

        if (This->changed.indices) {
            IWineD3DDevice_SetIndices(pDevice, This->pIndexData);
            IWineD3DDevice_SetBaseVertexIndex(pDevice, This->baseVertexIndex);
        }

        if (This->changed.fvf) {
            IWineD3DDevice_SetFVF(pDevice, This->fvf);
        }

        if (This->changed.vertexDecl) {
            IWineD3DDevice_SetVertexDeclaration(pDevice, This->vertexDecl);
        }

        if (This->changed.material ) {
            IWineD3DDevice_SetMaterial(pDevice, &This->material);
        }

        if (This->changed.viewport) {
            IWineD3DDevice_SetViewport(pDevice, &This->viewport);
        }

        if (This->changed.scissorRect) {
            IWineD3DDevice_SetScissorRect(pDevice, &This->scissorRect);
        }

        /* TODO: Proper implementation using SetStreamSource offset (set to 0 for the moment)\n") */
        for (i=0; i<MAX_STREAMS; i++) {
            if (This->changed.streamSource[i])
                IWineD3DDevice_SetStreamSource(pDevice, i, This->streamSource[i], 0, This->streamStride[i]);

            if (This->changed.streamFreq[i])
                IWineD3DDevice_SetStreamSourceFreq(pDevice, i, This->streamFreq[i] | This->streamFlags[i]);
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++){
            if (This->changed.textures[j]) {
                if (j < MAX_FRAGMENT_SAMPLERS) {
                    IWineD3DDevice_SetTexture(pDevice, j, This->textures[j]);
                } else {
                    IWineD3DDevice_SetTexture(pDevice, WINED3DVERTEXTEXTURESAMPLER0 + j - MAX_FRAGMENT_SAMPLERS, This->textures[j]);
                }
            }
        }

        for (i = 0; i < GL_LIMITS(clipplanes); i++) {
            if (This->changed.clipplane[i]) {
                float clip[4];

                clip[0] = This->clipplane[i][0];
                clip[1] = This->clipplane[i][1];
                clip[2] = This->clipplane[i][2];
                clip[3] = This->clipplane[i][3];
                IWineD3DDevice_SetClipPlane(pDevice, i, clip);
            }
        }

    } else if(This->blockType == WINED3DSBT_VERTEXSTATE) {
        IWineD3DDevice_SetVertexShader(pDevice, This->vertexShader);
        for (i = 0; i < GL_LIMITS(vshader_constantsF); i++) {
            IWineD3DDevice_SetVertexShaderConstantF(pDevice, i,
                    This->vertexShaderConstantF + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_I; i++) {
            IWineD3DDevice_SetVertexShaderConstantI(pDevice, i,
                    This->vertexShaderConstantI + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_B; i++) {
            IWineD3DDevice_SetVertexShaderConstantB(pDevice, i,
                    This->vertexShaderConstantB + i, 1);
        }

        apply_lights(pDevice, This);

        for(i = 0; i < NUM_SAVEDVERTEXSTATES_R; i++) {
            IWineD3DDevice_SetRenderState(pDevice, SavedVertexStates_R[i], This->renderState[SavedVertexStates_R[i]]);
        }
        for(j = 0; j < MAX_TEXTURES; j++) {
            for(i = 0; i < NUM_SAVEDVERTEXSTATES_T; i++) {
                IWineD3DDevice_SetTextureStageState(pDevice, j, SavedVertexStates_T[i],
                        This->textureState[j][SavedVertexStates_T[i]]);
            }
        }

        for(j = 0; j < MAX_FRAGMENT_SAMPLERS; j++) {
            for(i = 0; i < NUM_SAVEDVERTEXSTATES_S; i++) {
                IWineD3DDevice_SetSamplerState(pDevice, j, SavedVertexStates_S[i],
                                               This->samplerState[j][SavedVertexStates_S[i]]);
            }
        }
        for(j = MAX_FRAGMENT_SAMPLERS; j < MAX_COMBINED_SAMPLERS; j++) {
            for(i = 0; i < NUM_SAVEDVERTEXSTATES_S; i++) {
                IWineD3DDevice_SetSamplerState(pDevice,
                                               WINED3DVERTEXTEXTURESAMPLER0 + j - MAX_FRAGMENT_SAMPLERS,
                                               SavedVertexStates_S[i],
                                               This->samplerState[j][SavedVertexStates_S[i]]);
            }
        }
    } else if(This->blockType == WINED3DSBT_PIXELSTATE) {
        IWineD3DDevice_SetPixelShader(pDevice, This->pixelShader);
        for (i = 0; i < GL_LIMITS(pshader_constantsF); i++) {
            IWineD3DDevice_SetPixelShaderConstantF(pDevice, i,
                    This->pixelShaderConstantF + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_I; i++) {
            IWineD3DDevice_SetPixelShaderConstantI(pDevice, i,
                    This->pixelShaderConstantI + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_B; i++) {
            IWineD3DDevice_SetPixelShaderConstantB(pDevice, i,
                    This->pixelShaderConstantB + i, 1);
        }

        for(i = 0; i < NUM_SAVEDPIXELSTATES_R; i++) {
            IWineD3DDevice_SetRenderState(pDevice, SavedPixelStates_R[i], This->renderState[SavedPixelStates_R[i]]);
        }
        for(j = 0; j < MAX_TEXTURES; j++) {
            for(i = 0; i < NUM_SAVEDPIXELSTATES_T; i++) {
                IWineD3DDevice_SetTextureStageState(pDevice, j, SavedPixelStates_T[i],
                        This->textureState[j][SavedPixelStates_T[i]]);
            }
        }

        for(j = 0; j < MAX_FRAGMENT_SAMPLERS; j++) {
            for(i = 0; i < NUM_SAVEDPIXELSTATES_S; i++) {
                IWineD3DDevice_SetSamplerState(pDevice, j, SavedPixelStates_S[i],
                                               This->samplerState[j][SavedPixelStates_S[i]]);
            }
        }
        for(j = MAX_FRAGMENT_SAMPLERS; j < MAX_COMBINED_SAMPLERS; j++) {
            for(i = 0; i < NUM_SAVEDPIXELSTATES_S; i++) {
                IWineD3DDevice_SetSamplerState(pDevice,
                                               WINED3DVERTEXTEXTURESAMPLER0 + j - MAX_FRAGMENT_SAMPLERS,
                                               SavedPixelStates_S[i],
                                               This->samplerState[j][SavedPixelStates_S[i]]);
            }
        }
    } else if(This->blockType == WINED3DSBT_ALL) {
        IWineD3DDevice_SetVertexShader(pDevice, This->vertexShader);
        for (i = 0; i < GL_LIMITS(vshader_constantsF); i++) {
            IWineD3DDevice_SetVertexShaderConstantF(pDevice, i,
                    This->vertexShaderConstantF + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_I; i++) {
            IWineD3DDevice_SetVertexShaderConstantI(pDevice, i,
                    This->vertexShaderConstantI + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_B; i++) {
            IWineD3DDevice_SetVertexShaderConstantB(pDevice, i,
                    This->vertexShaderConstantB + i, 1);
        }

        IWineD3DDevice_SetPixelShader(pDevice, This->pixelShader);
        for (i = 0; i < GL_LIMITS(pshader_constantsF); i++) {
            IWineD3DDevice_SetPixelShaderConstantF(pDevice, i,
                    This->pixelShaderConstantF + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_I; i++) {
            IWineD3DDevice_SetPixelShaderConstantI(pDevice, i,
                    This->pixelShaderConstantI + i * 4, 1);
        }
        for (i = 0; i < MAX_CONST_B; i++) {
            IWineD3DDevice_SetPixelShaderConstantB(pDevice, i,
                    This->pixelShaderConstantB + i, 1);
        }

        apply_lights(pDevice, This);

        for(i = 1; i <= WINEHIGHEST_RENDER_STATE; i++) {
            IWineD3DDevice_SetRenderState(pDevice, i, This->renderState[i]);
        }
        for(j = 0; j < MAX_TEXTURES; j++) {
            for(i = 1; i <= WINED3D_HIGHEST_TEXTURE_STATE; i++) {
                IWineD3DDevice_SetTextureStageState(pDevice, j, i, This->textureState[j][i]);
            }
        }

        /* Skip unused values between TEXTURE8 and WORLD0 ? */
        for(i = 1; i <= HIGHEST_TRANSFORMSTATE; i++) {
            IWineD3DDevice_SetTransform(pDevice, i, &This->transforms[i]);
        }
        IWineD3DDevice_SetIndices(pDevice, This->pIndexData);
        IWineD3DDevice_SetBaseVertexIndex(pDevice, This->baseVertexIndex);
        IWineD3DDevice_SetFVF(pDevice, This->fvf);
        IWineD3DDevice_SetVertexDeclaration(pDevice, This->vertexDecl);
        IWineD3DDevice_SetMaterial(pDevice, &This->material);
        IWineD3DDevice_SetViewport(pDevice, &This->viewport);
        IWineD3DDevice_SetScissorRect(pDevice, &This->scissorRect);

        /* TODO: Proper implementation using SetStreamSource offset (set to 0 for the moment)\n") */
        for (i=0; i<MAX_STREAMS; i++) {
            IWineD3DDevice_SetStreamSource(pDevice, i, This->streamSource[i], 0, This->streamStride[i]);
            IWineD3DDevice_SetStreamSourceFreq(pDevice, i, This->streamFreq[i] | This->streamFlags[i]);
        }
        for (j = 0 ; j < MAX_COMBINED_SAMPLERS; j++){
            UINT sampler = j < MAX_FRAGMENT_SAMPLERS ? j : WINED3DVERTEXTEXTURESAMPLER0 + j - MAX_FRAGMENT_SAMPLERS;

            IWineD3DDevice_SetTexture(pDevice, sampler, This->textures[j]);
            for(i = 1; i < WINED3D_HIGHEST_SAMPLER_STATE; i++) {
                IWineD3DDevice_SetSamplerState(pDevice, sampler, i, This->samplerState[j][i]);
            }
        }
        for (i = 0; i < GL_LIMITS(clipplanes); i++) {
            float clip[4];

            clip[0] = This->clipplane[i][0];
            clip[1] = This->clipplane[i][1];
            clip[2] = This->clipplane[i][2];
            clip[3] = This->clipplane[i][3];
            IWineD3DDevice_SetClipPlane(pDevice, i, clip);
        }
    }

    ((IWineD3DDeviceImpl *)pDevice)->stateBlock->lowest_disabled_stage = MAX_TEXTURES - 1;
    for(j = 0; j < MAX_TEXTURES - 1; j++) {
        if(((IWineD3DDeviceImpl *)pDevice)->stateBlock->textureState[j][WINED3DTSS_COLOROP] == WINED3DTOP_DISABLE) {
            ((IWineD3DDeviceImpl *)pDevice)->stateBlock->lowest_disabled_stage = j;
            break;
        }
    }
    TRACE("(%p) : Applied state block %p ------------------^\n", This, pDevice);

    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DStateBlockImpl_InitStartupStateBlock(IWineD3DStateBlock* iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IWineD3DDevice         *device = (IWineD3DDevice *)This->wineD3DDevice;
    IWineD3DDeviceImpl     *ThisDevice = (IWineD3DDeviceImpl *)device;
    union {
        WINED3DLINEPATTERN lp;
        DWORD d;
    } lp;
    union {
        float f;
        DWORD d;
    } tmpfloat;
    unsigned int i;
    IWineD3DSwapChain *swapchain;
    IWineD3DSurface *backbuffer;
    WINED3DSURFACE_DESC desc = {0};
    UINT width, height;
    RECT scissorrect;
    HRESULT hr;

    /* Note this may have a large overhead but it should only be executed
       once, in order to initialize the complete state of the device and
       all opengl equivalents                                            */
    TRACE("(%p) -----------------------> Setting up device defaults... %p\n", This, This->wineD3DDevice);
    /* TODO: make a special stateblock type for the primary stateblock (it never gets applied so it doesn't need a real type) */
    This->blockType = WINED3DSBT_INIT;

    /* Set some of the defaults for lights, transforms etc */
    memcpy(&This->transforms[WINED3DTS_PROJECTION], identity, sizeof(identity));
    memcpy(&This->transforms[WINED3DTS_VIEW], identity, sizeof(identity));
    for (i = 0; i < 256; ++i) {
      memcpy(&This->transforms[WINED3DTS_WORLDMATRIX(i)], identity, sizeof(identity));
    }

    TRACE("Render states\n");
    /* Render states: */
    if (ThisDevice->auto_depth_stencil_buffer != NULL) {
       IWineD3DDevice_SetRenderState(device, WINED3DRS_ZENABLE,       WINED3DZB_TRUE);
    } else {
       IWineD3DDevice_SetRenderState(device, WINED3DRS_ZENABLE,       WINED3DZB_FALSE);
    }
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FILLMODE,         WINED3DFILL_SOLID);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SHADEMODE,        WINED3DSHADE_GOURAUD);
    lp.lp.wRepeatFactor = 0;
    lp.lp.wLinePattern  = 0;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_LINEPATTERN,      lp.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ZWRITEENABLE,     TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ALPHATESTENABLE,  FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_LASTPIXEL,        TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SRCBLEND,         WINED3DBLEND_ONE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DESTBLEND,        WINED3DBLEND_ZERO);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CULLMODE,         WINED3DCULL_CCW);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ZFUNC,            WINED3DCMP_LESSEQUAL);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ALPHAFUNC,        WINED3DCMP_ALWAYS);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ALPHAREF,         0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DITHERENABLE,     FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ALPHABLENDENABLE, FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGENABLE,        FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SPECULARENABLE,   FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ZVISIBLE,         0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGCOLOR,         0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGTABLEMODE,     WINED3DFOG_NONE);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGSTART,         tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGEND,           tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGDENSITY,       tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_EDGEANTIALIAS,    FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ZBIAS,            0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_RANGEFOGENABLE,   FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILENABLE,    FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILFAIL,      WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILZFAIL,     WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILPASS,      WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILREF,       0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILMASK,      0xFFFFFFFF);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILFUNC,      WINED3DCMP_ALWAYS);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_TEXTUREFACTOR,    0xFFFFFFFF);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP0, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP1, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP2, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP3, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP4, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP5, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP6, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP7, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CLIPPING,                 TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_LIGHTING,                 TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_AMBIENT,                  0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_FOGVERTEXMODE,            WINED3DFOG_NONE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_COLORVERTEX,              TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_LOCALVIEWER,              TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_NORMALIZENORMALS,         FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DIFFUSEMATERIALSOURCE,    WINED3DMCS_COLOR1);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SPECULARMATERIALSOURCE,   WINED3DMCS_COLOR2);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_AMBIENTMATERIALSOURCE,    WINED3DMCS_MATERIAL);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_EMISSIVEMATERIALSOURCE,   WINED3DMCS_MATERIAL);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_VERTEXBLEND,              WINED3DVBF_DISABLE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CLIPPLANEENABLE,          0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SOFTWAREVERTEXPROCESSING, FALSE);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSIZE,                tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSIZE_MIN,            tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSPRITEENABLE,        FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSCALEENABLE,         FALSE);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSCALE_A,             tmpfloat.d);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSCALE_B,             tmpfloat.d);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSCALE_C,             tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_MULTISAMPLEANTIALIAS,     TRUE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_MULTISAMPLEMASK,          0xFFFFFFFF);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_PATCHEDGESTYLE,           WINED3DPATCHEDGE_DISCRETE);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_PATCHSEGMENTS,            tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DEBUGMONITORTOKEN,        0xbaadcafe);
    tmpfloat.f = 64.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POINTSIZE_MAX,            tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_COLORWRITEENABLE,         0x0000000F);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_TWEENFACTOR,              tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_BLENDOP,                  WINED3DBLENDOP_ADD);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_POSITIONDEGREE,           WINED3DDEGREE_CUBIC);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_NORMALDEGREE,             WINED3DDEGREE_LINEAR);
    /* states new in d3d9 */
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SCISSORTESTENABLE,        FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SLOPESCALEDEPTHBIAS,      0);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_MINTESSELLATIONLEVEL,     tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_MAXTESSELLATIONLEVEL,     tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ANTIALIASEDLINEENABLE,    FALSE);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ADAPTIVETESS_X,           tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ADAPTIVETESS_Y,           tmpfloat.d);
    tmpfloat.f = 1.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ADAPTIVETESS_Z,           tmpfloat.d);
    tmpfloat.f = 0.0f;
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ADAPTIVETESS_W,           tmpfloat.d);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_ENABLEADAPTIVETESSELLATION, FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_TWOSIDEDSTENCILMODE,      FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CCW_STENCILFAIL,          WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CCW_STENCILZFAIL,         WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CCW_STENCILPASS,          WINED3DSTENCILOP_KEEP);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_CCW_STENCILFUNC,          WINED3DCMP_ALWAYS);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_COLORWRITEENABLE1,        0x0000000F);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_COLORWRITEENABLE2,        0x0000000F);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_COLORWRITEENABLE3,        0x0000000F);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_BLENDFACTOR,              0xFFFFFFFF);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SRGBWRITEENABLE,          0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DEPTHBIAS,                0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP8,  0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP9,  0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP10, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP11, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP12, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP13, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP14, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_WRAP15, 0);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_SRCBLENDALPHA,            WINED3DBLEND_ONE);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_DESTBLENDALPHA,           WINED3DBLEND_ZERO);
    IWineD3DDevice_SetRenderState(device, WINED3DRS_BLENDOPALPHA,             WINED3DBLENDOP_ADD);

    /* clipping status */
    This->clip_status.ClipUnion = 0;
    This->clip_status.ClipIntersection = 0xFFFFFFFF;

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < MAX_TEXTURES; i++) {
        TRACE("Setting up default texture states for texture Stage %d\n", i);
        memcpy(&This->transforms[WINED3DTS_TEXTURE0 + i], identity, sizeof(identity));
        This->textureState[i][WINED3DTSS_COLOROP               ] = (i==0)? WINED3DTOP_MODULATE :  WINED3DTOP_DISABLE;
        This->textureState[i][WINED3DTSS_COLORARG1             ] = WINED3DTA_TEXTURE;
        This->textureState[i][WINED3DTSS_COLORARG2             ] = WINED3DTA_CURRENT;
        This->textureState[i][WINED3DTSS_ALPHAOP               ] = (i==0)? WINED3DTOP_SELECTARG1 :  WINED3DTOP_DISABLE;
        This->textureState[i][WINED3DTSS_ALPHAARG1             ] = WINED3DTA_TEXTURE;
        This->textureState[i][WINED3DTSS_ALPHAARG2             ] = WINED3DTA_CURRENT;
        This->textureState[i][WINED3DTSS_BUMPENVMAT00          ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_BUMPENVMAT01          ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_BUMPENVMAT10          ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_BUMPENVMAT11          ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_TEXCOORDINDEX         ] = i;
        This->textureState[i][WINED3DTSS_BUMPENVLSCALE         ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_BUMPENVLOFFSET        ] = (DWORD) 0.0;
        This->textureState[i][WINED3DTSS_TEXTURETRANSFORMFLAGS ] = WINED3DTTFF_DISABLE;
        This->textureState[i][WINED3DTSS_ADDRESSW              ] = WINED3DTADDRESS_WRAP;
        This->textureState[i][WINED3DTSS_COLORARG0             ] = WINED3DTA_CURRENT;
        This->textureState[i][WINED3DTSS_ALPHAARG0             ] = WINED3DTA_CURRENT;
        This->textureState[i][WINED3DTSS_RESULTARG             ] = WINED3DTA_CURRENT;
    }
    This->lowest_disabled_stage = 1;

        /* Sampler states*/
    for (i = 0 ; i <  MAX_COMBINED_SAMPLERS; i++) {
        TRACE("Setting up default samplers states for sampler %d\n", i);
        This->samplerState[i][WINED3DSAMP_ADDRESSU         ] = WINED3DTADDRESS_WRAP;
        This->samplerState[i][WINED3DSAMP_ADDRESSV         ] = WINED3DTADDRESS_WRAP;
        This->samplerState[i][WINED3DSAMP_ADDRESSW         ] = WINED3DTADDRESS_WRAP;
        This->samplerState[i][WINED3DSAMP_BORDERCOLOR      ] = 0x00;
        This->samplerState[i][WINED3DSAMP_MAGFILTER        ] = WINED3DTEXF_POINT;
        This->samplerState[i][WINED3DSAMP_MINFILTER        ] = WINED3DTEXF_POINT;
        This->samplerState[i][WINED3DSAMP_MIPFILTER        ] = WINED3DTEXF_NONE;
        This->samplerState[i][WINED3DSAMP_MIPMAPLODBIAS    ] = 0;
        This->samplerState[i][WINED3DSAMP_MAXMIPLEVEL      ] = 0;
        This->samplerState[i][WINED3DSAMP_MAXANISOTROPY    ] = 1;
        This->samplerState[i][WINED3DSAMP_SRGBTEXTURE      ] = 0;
        This->samplerState[i][WINED3DSAMP_ELEMENTINDEX     ] = 0; /* TODO: Indicates which element of a  multielement texture to use */
        This->samplerState[i][WINED3DSAMP_DMAPOFFSET       ] = 0; /* TODO: Vertex offset in the presampled displacement map */
    }

    for(i = 0; i < GL_LIMITS(textures); i++) {
        /* Note: This avoids calling SetTexture, so pretend it has been called */
        This->changed.textures[i] = TRUE;
        This->textures[i]         = NULL;
    }

    /* Set the default scissor rect values */
    desc.Width = &width;
    desc.Height = &height;

    /* check the return values, because the GetBackBuffer call isn't valid for ddraw */
    hr = IWineD3DDevice_GetSwapChain(device, 0, &swapchain);
    if( hr == WINED3D_OK && swapchain != NULL) {
        hr = IWineD3DSwapChain_GetBackBuffer(swapchain, 0, WINED3DBACKBUFFER_TYPE_MONO, &backbuffer);
        if( hr == WINED3D_OK && backbuffer != NULL) {
            IWineD3DSurface_GetDesc(backbuffer, &desc);
            IWineD3DSurface_Release(backbuffer);

            scissorrect.left = 0;
            scissorrect.right = width;
            scissorrect.top = 0;
            scissorrect.bottom = height;
            hr = IWineD3DDevice_SetScissorRect(device, &scissorrect);
            if( hr != WINED3D_OK ) {
                ERR("This should never happen, expect rendering issues!\n");
            }
        }
        IWineD3DSwapChain_Release(swapchain);
    }

    TRACE("-----------------------> Device defaults now set up...\n");
    return WINED3D_OK;
}

/**********************************************************
 * IWineD3DStateBlock VTbl follows
 **********************************************************/

const IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl =
{
    /* IUnknown */
    IWineD3DStateBlockImpl_QueryInterface,
    IWineD3DStateBlockImpl_AddRef,
    IWineD3DStateBlockImpl_Release,
    /* IWineD3DStateBlock */
    IWineD3DStateBlockImpl_GetParent,
    IWineD3DStateBlockImpl_GetDevice,
    IWineD3DStateBlockImpl_Capture,
    IWineD3DStateBlockImpl_Apply,
    IWineD3DStateBlockImpl_InitStartupStateBlock
};
