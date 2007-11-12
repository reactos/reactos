/*
 * IWineD3DSurface Implementation
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2007 Stefan Dösinger for CodeWeavers
 * Copyright 2007 Henri Verbeet
 * Copyright 2006-2007 Roderick Colenbrander
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
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

HRESULT d3dfmt_convert_surface(BYTE *src, BYTE *dst, UINT pitch, UINT width, UINT height, UINT outpitch, CONVERT_TYPES convert, IWineD3DSurfaceImpl *surf);
static void d3dfmt_p8_init_palette(IWineD3DSurfaceImpl *This, BYTE table[256][4], BOOL colorkey);

static void surface_download_data(IWineD3DSurfaceImpl *This) {
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;

    if (0 == This->glDescription.textureName) {
        ERR("Surface does not have a texture, but SFLAG_INTEXTURE is set\n");
        return;
    }

    if(myDevice->createParms.BehaviorFlags & WINED3DCREATE_MULTITHREADED) {
        ActivateContext(myDevice, myDevice->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }

    ENTER_GL();
            /* Make sure that a proper texture unit is selected, bind the texture
    * and dirtify the sampler to restore the texture on the next draw
            */
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
        checkGLcall("glActiveTextureARB");
    }
    IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_SAMPLER(0));
    IWineD3DSurface_PreLoad((IWineD3DSurface *) This);

    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) {
        if (!GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) { /* We can assume this as the texture would not have been created otherwise */
            FIXME("(%p) : Attempting to lock a compressed texture when texture compression isn't supported by opengl\n", This);
        } else {
            TRACE("(%p) : Calling glGetCompressedTexImageARB level %d, format %#x, type %#x, data %p\n", This, This->glDescription.level,
                This->glDescription.glFormat, This->glDescription.glType, This->resource.allocatedMemory);

            if(This->Flags & SFLAG_PBO) {
                GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, This->pbo));
                checkGLcall("glBindBufferARB");
                GL_EXTCALL(glGetCompressedTexImageARB(This->glDescription.target, This->glDescription.level, NULL));
                checkGLcall("glGetCompressedTexImageARB()");
                GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
                checkGLcall("glBindBufferARB");
            } else {
                GL_EXTCALL(glGetCompressedTexImageARB(This->glDescription.target, This->glDescription.level, This->resource.allocatedMemory));
                checkGLcall("glGetCompressedTexImageARB()");
            }
        }
        LEAVE_GL();
    } else {
        void *mem;
        int src_pitch = 0;
        int dst_pitch = 0;

         if(This->Flags & SFLAG_CONVERTED) {
             FIXME("Read back converted textures unsupported\n");
             LEAVE_GL();
             return;
         }

        if (This->Flags & SFLAG_NONPOW2) {
            unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
            src_pitch = This->bytesPerPixel * This->pow2Width;
            dst_pitch = IWineD3DSurface_GetPitch((IWineD3DSurface *) This);
            src_pitch = (src_pitch + alignment - 1) & ~(alignment - 1);
            mem = HeapAlloc(GetProcessHeap(), 0, src_pitch * This->pow2Height);
        } else {
            mem = This->resource.allocatedMemory;
        }

        TRACE("(%p) : Calling glGetTexImage level %d, format %#x, type %#x, data %p\n", This, This->glDescription.level,
                This->glDescription.glFormat, This->glDescription.glType, mem);

        if(This->Flags & SFLAG_PBO) {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, This->pbo));
            checkGLcall("glBindBufferARB");

            glGetTexImage(This->glDescription.target, This->glDescription.level, This->glDescription.glFormat,
                          This->glDescription.glType, NULL);
            checkGLcall("glGetTexImage()");

            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
            checkGLcall("glBindBufferARB");
        } else {
            glGetTexImage(This->glDescription.target, This->glDescription.level, This->glDescription.glFormat,
                          This->glDescription.glType, mem);
            checkGLcall("glGetTexImage()");
        }
        LEAVE_GL();

        if (This->Flags & SFLAG_NONPOW2) {
            LPBYTE src_data, dst_data;
            int y;
            /*
             * Some games (e.g. warhammer 40k) don't work properly with the odd pitches, preventing
             * the surface pitch from being used to box non-power2 textures. Instead we have to use a hack to
             * repack the texture so that the bpp * width pitch can be used instead of bpp * pow2width.
             *
             * We're doing this...
             *
             * instead of boxing the texture :
             * |<-texture width ->|  -->pow2width|   /\
             * |111111111111111111|              |   |
             * |222 Texture 222222| boxed empty  | texture height
             * |3333 Data 33333333|              |   |
             * |444444444444444444|              |   \/
             * -----------------------------------   |
             * |     boxed  empty | boxed empty  | pow2height
             * |                  |              |   \/
             * -----------------------------------
             *
             *
             * we're repacking the data to the expected texture width
             *
             * |<-texture width ->|  -->pow2width|   /\
             * |111111111111111111222222222222222|   |
             * |222333333333333333333444444444444| texture height
             * |444444                           |   |
             * |                                 |   \/
             * |                                 |   |
             * |            empty                | pow2height
             * |                                 |   \/
             * -----------------------------------
             *
             * == is the same as
             *
             * |<-texture width ->|    /\
             * |111111111111111111|
             * |222222222222222222|texture height
             * |333333333333333333|
             * |444444444444444444|    \/
             * --------------------
             *
             * this also means that any references to allocatedMemory should work with the data as if were a
             * standard texture with a non-power2 width instead of texture boxed up to be a power2 texture.
             *
             * internally the texture is still stored in a boxed format so any references to textureName will
             * get a boxed texture with width pow2width and not a texture of width currentDesc.Width.
             *
             * Performance should not be an issue, because applications normally do not lock the surfaces when
             * rendering. If an app does, the SFLAG_DYNLOCK flag will kick in and the memory copy won't be released,
             * and doesn't have to be re-read.
             */
            src_data = mem;
            dst_data = This->resource.allocatedMemory;
            TRACE("(%p) : Repacking the surface data from pitch %d to pitch %d\n", This, src_pitch, dst_pitch);
            for (y = 1 ; y < This->currentDesc.Height; y++) {
                /* skip the first row */
                src_data += src_pitch;
                dst_data += dst_pitch;
                memcpy(dst_data, src_data, dst_pitch);
            }

            HeapFree(GetProcessHeap(), 0, mem);
        }
    }

    /* Surface has now been downloaded */
    This->Flags |= SFLAG_INSYSMEM;
}

static void surface_upload_data(IWineD3DSurfaceImpl *This, GLenum internal, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data) {
    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) {
        if (!GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
            FIXME("Using DXT1/3/5 without advertized support\n");
        } else {
            /* glCompressedTexSubImage2D for uploading and glTexImage2D for allocating does not work well on some drivers(r200 dri, MacOS ATI driver)
             * glCompressedTexImage2D does not accept NULL pointers. So for compressed textures surface_allocate_surface does nothing, and this
             * function uses glCompressedTexImage2D instead of the SubImage call
             */
            TRACE("(%p) : Calling glCompressedTexSubImage2D w %d, h %d, data %p\n", This, width, height, data);
            ENTER_GL();

            if(This->Flags & SFLAG_PBO) {
                GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
                checkGLcall("glBindBufferARB");
                TRACE("(%p) pbo: %#x, data: %p\n", This, This->pbo, data);

                GL_EXTCALL(glCompressedTexImage2DARB(This->glDescription.target, This->glDescription.level, internal,
                        width, height, 0 /* border */, This->resource.size, NULL));
                checkGLcall("glCompressedTexSubImage2D");

                GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
                checkGLcall("glBindBufferARB");
            } else {
                GL_EXTCALL(glCompressedTexImage2DARB(This->glDescription.target, This->glDescription.level, internal,
                        width, height, 0 /* border */, This->resource.size, data));
                checkGLcall("glCompressedTexSubImage2D");
            }
            LEAVE_GL();
        }
    } else {
        TRACE("(%p) : Calling glTexSubImage2D w %d,  h %d, data, %p\n", This, width, height, data);
        ENTER_GL();

        if(This->Flags & SFLAG_PBO) {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
            checkGLcall("glBindBufferARB");
            TRACE("(%p) pbo: %#x, data: %p\n", This, This->pbo, data);

            glTexSubImage2D(This->glDescription.target, This->glDescription.level, 0, 0, width, height, format, type, NULL);
            checkGLcall("glTexSubImage2D");

            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
            checkGLcall("glBindBufferARB");
        }
        else {
            glTexSubImage2D(This->glDescription.target, This->glDescription.level, 0, 0, width, height, format, type, data);
            checkGLcall("glTexSubImage2D");
        }

        LEAVE_GL();
    }
}

static void surface_allocate_surface(IWineD3DSurfaceImpl *This, GLenum internal, GLsizei width, GLsizei height, GLenum format, GLenum type) {
    BOOL enable_client_storage = FALSE;
    BYTE *mem = NULL;

    TRACE("(%p) : Creating surface (target %#x)  level %d, d3d format %s, internal format %#x, width %d, height %d, gl format %#x, gl type=%#x\n", This,
            This->glDescription.target, This->glDescription.level, debug_d3dformat(This->resource.format), internal, width, height, format, type);

    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) {
        /* glCompressedTexImage2D does not accept NULL pointers, so we cannot allocate a compressed texture without uploading data */
        TRACE("Not allocating compressed surfaces, surface_upload_data will specify them\n");

        /* We have to point GL to the client storage memory here, because upload_data might use a PBO. This means a double upload
         * once, unfortunately
         */
        if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
            /* Neither NONPOW2, DIBSECTION nor OVERSIZE flags can be set on compressed textures */
            This->Flags |= SFLAG_CLIENT;
            mem = (BYTE *)(((ULONG_PTR) This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
            GL_EXTCALL(glCompressedTexImage2DARB(This->glDescription.target, This->glDescription.level, internal,
                       width, height, 0 /* border */, This->resource.size, mem));
        }

        return;
    }

    ENTER_GL();

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        if(This->Flags & (SFLAG_NONPOW2 | SFLAG_DIBSECTION | SFLAG_OVERSIZE | SFLAG_CONVERTED) || This->resource.allocatedMemory == NULL) {
            /* In some cases we want to disable client storage.
             * SFLAG_NONPOW2 has a bigger opengl texture than the client memory, and different pitches
             * SFLAG_DIBSECTION: Dibsections may have read / write protections on the memory. Avoid issues...
             * SFLAG_OVERSIZE: The gl texture is smaller than the allocated memory
             * SFLAG_CONVERTED: The conversion destination memory is freed after loading the surface
             * allocatedMemory == NULL: Not defined in the extension. Seems to disable client storage effectively
             */
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
            checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
            This->Flags &= ~SFLAG_CLIENT;
            enable_client_storage = TRUE;
        } else {
            This->Flags |= SFLAG_CLIENT;

            /* Point opengl to our allocated texture memory. Do not use resource.allocatedMemory here because
             * it might point into a pbo. Instead use heapMemory, but get the alignment right.
             */
            mem = (BYTE *)(((ULONG_PTR) This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
        }
    }
    glTexImage2D(This->glDescription.target, This->glDescription.level, internal, width, height, 0, format, type, mem);
    checkGLcall("glTexImage2D");

    if(enable_client_storage) {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }
    LEAVE_GL();

    This->Flags |= SFLAG_ALLOCATED;
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
void surface_set_compatible_renderbuffer(IWineD3DSurface *iface, unsigned int width, unsigned int height) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    renderbuffer_entry_t *entry;
    GLuint renderbuffer = 0;
    unsigned int src_width, src_height;

    src_width = This->pow2Width;
    src_height = This->pow2Height;

    /* A depth stencil smaller than the render target is not valid */
    if (width > src_width || height > src_height) return;

    /* Remove any renderbuffer set if the sizes match */
    if (width == src_width && height == src_height) {
        This->current_renderbuffer = NULL;
        return;
    }

    /* Look if we've already got a renderbuffer of the correct dimensions */
    LIST_FOR_EACH_ENTRY(entry, &This->renderbuffers, renderbuffer_entry_t, entry) {
        if (entry->width == width && entry->height == height) {
            renderbuffer = entry->id;
            This->current_renderbuffer = entry;
            break;
        }
    }

    if (!renderbuffer) {
        const GlPixelFormatDesc *glDesc;
        getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);

        GL_EXTCALL(glGenRenderbuffersEXT(1, &renderbuffer));
        GL_EXTCALL(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer));
        GL_EXTCALL(glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, glDesc->glFormat, width, height));

        entry = HeapAlloc(GetProcessHeap(), 0, sizeof(renderbuffer_entry_t));
        entry->width = width;
        entry->height = height;
        entry->id = renderbuffer;
        list_add_head(&This->renderbuffers, &entry->entry);

        This->current_renderbuffer = entry;
    }

    checkGLcall("set_compatible_renderbuffer");
}

GLenum surface_get_gl_buffer(IWineD3DSurface *iface, IWineD3DSwapChain *swapchain) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSwapChainImpl *swapchain_impl = (IWineD3DSwapChainImpl *)swapchain;

    TRACE("(%p) : swapchain %p\n", This, swapchain);

    if (swapchain_impl->backBuffer && swapchain_impl->backBuffer[0] == iface) {
        TRACE("Returning GL_BACK\n");
        return GL_BACK;
    } else if (swapchain_impl->frontBuffer == iface) {
        TRACE("Returning GL_FRONT\n");
        return GL_FRONT;
    }

    FIXME("Higher back buffer, returning GL_BACK\n");
    return GL_BACK;
}

ULONG WINAPI IWineD3DSurfaceImpl_Release(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->resource.wineD3DDevice;
        renderbuffer_entry_t *entry, *entry2;
        TRACE("(%p) : cleaning up\n", This);

        if (This->glDescription.textureName != 0) { /* release the openGL texture.. */

            /* Need a context to destroy the texture. Use the currently active render target, but only if
             * the primary render target exists. Otherwise lastActiveRenderTarget is garbage, see above.
             * When destroying the primary rt, Uninit3D will activate a context before doing anything
             */
            if(device->render_targets && device->render_targets[0]) {
                ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            }

            TRACE("Deleting texture %d\n", This->glDescription.textureName);
            ENTER_GL();
            glDeleteTextures(1, &This->glDescription.textureName);
            LEAVE_GL();
        }

        if(This->Flags & SFLAG_PBO) {
            /* Delete the PBO */
            GL_EXTCALL(glDeleteBuffersARB(1, &This->pbo));
        }

        if(This->Flags & SFLAG_DIBSECTION) {
            /* Release the DC */
            SelectObject(This->hDC, This->dib.holdbitmap);
            DeleteDC(This->hDC);
            /* Release the DIB section */
            DeleteObject(This->dib.DIBsection);
            This->dib.bitmap_data = NULL;
            This->resource.allocatedMemory = NULL;
        }
        if(This->Flags & SFLAG_USERPTR) IWineD3DSurface_SetMem(iface, NULL);

        HeapFree(GetProcessHeap(), 0, This->palette9);

        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        if(iface == device->ddraw_primary)
            device->ddraw_primary = NULL;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &This->renderbuffers, renderbuffer_entry_t, entry) {
            GL_EXTCALL(glDeleteRenderbuffersEXT(1, &entry->id));
            HeapFree(GetProcessHeap(), 0, entry);
        }

        TRACE("(%p) Released\n", This);
        HeapFree(GetProcessHeap(), 0, This);

    }
    return ref;
}

/* ****************************************************
   IWineD3DSurface IWineD3DResource parts follow
   **************************************************** */

void WINAPI IWineD3DSurfaceImpl_PreLoad(IWineD3DSurface *iface) {
    /* TODO: check for locks */
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("(%p)Checking to see if the container is a base texture\n", This);
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == WINED3D_OK) {
        TRACE("Passing to container\n");
        IWineD3DBaseTexture_PreLoad(baseTexture);
        IWineD3DBaseTexture_Release(baseTexture);
    } else {
    TRACE("(%p) : About to load surface\n", This);

    if(!device->isInDraw) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }

    ENTER_GL();
    glEnable(This->glDescription.target);/* make sure texture support is enabled in this context */
    if (!This->glDescription.level) {
        if (!This->glDescription.textureName) {
            glGenTextures(1, &This->glDescription.textureName);
            checkGLcall("glGenTextures");
            TRACE("Surface %p given name %d\n", This, This->glDescription.textureName);
        }
        glBindTexture(This->glDescription.target, This->glDescription.textureName);
        checkGLcall("glBindTexture");
        IWineD3DSurface_LoadTexture(iface, FALSE);
        /* This is where we should be reducing the amount of GLMemoryUsed */
    } else if (This->glDescription.textureName) { /* NOTE: the level 0 surface of a mpmapped texture must be loaded first! */
        /* assume this is a coding error not a real error for now */
        FIXME("Mipmap surface has a glTexture bound to it!\n");
    }
    if (This->resource.pool == WINED3DPOOL_DEFAULT) {
       /* Tell opengl to try and keep this texture in video ram (well mostly) */
       GLclampf tmp;
       tmp = 0.9f;
        glPrioritizeTextures(1, &This->glDescription.textureName, &tmp);
    }
    LEAVE_GL();
    }
    return;
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

void WINAPI IWineD3DSurfaceImpl_SetGlTextureDesc(IWineD3DSurface *iface, UINT textureName, int target) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : setting textureName %u, target %i\n", This, textureName, target);
    if (This->glDescription.textureName == 0 && textureName != 0) {
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INTEXTURE, FALSE);
        IWineD3DSurface_AddDirtyRect(iface, NULL);
    }
    This->glDescription.textureName = textureName;
    This->glDescription.target      = target;
    This->Flags &= ~SFLAG_ALLOCATED;
}

void WINAPI IWineD3DSurfaceImpl_GetGlDesc(IWineD3DSurface *iface, glDescriptor **glDescription) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, &This->glDescription);
    *glDescription = &This->glDescription;
}

/* TODO: think about moving this down to resource? */
const void *WINAPI IWineD3DSurfaceImpl_GetData(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    /* This should only be called for sysmem textures, it may be a good idea to extend this to all pools at some point in the future  */
    if (This->resource.pool != WINED3DPOOL_SYSTEMMEM) {
        FIXME(" (%p)Attempting to get system memory for a non-system memory texture\n", iface);
    }
    return (CONST void*)(This->resource.allocatedMemory);
}

static void read_from_framebuffer(IWineD3DSurfaceImpl *This, CONST RECT *rect, void *dest, UINT pitch) {
    IWineD3DSwapChainImpl *swapchain;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    BYTE *mem;
    GLint fmt;
    GLint type;
    BYTE *row, *top, *bottom;
    int i;
    BOOL bpp;
    RECT local_rect;
    BOOL srcIsUpsideDown;

    if(wined3d_settings.rendertargetlock_mode == RTL_DISABLE) {
        static BOOL warned = FALSE;
        if(!warned) {
            ERR("The application tries to lock the render target, but render target locking is disabled\n");
            warned = TRUE;
        }
        return;
    }

    IWineD3DSurface_GetContainer((IWineD3DSurface *) This, &IID_IWineD3DSwapChain, (void **)&swapchain);
    /* Activate the surface. Set it up for blitting now, although not necessarily needed for LockRect.
     * Certain graphics drivers seem to dislike some enabled states when reading from opengl, the blitting usage
     * should help here. Furthermore unlockrect will need the context set up for blitting. The context manager will find
     * context->last_was_blit set on the unlock.
     */
    ActivateContext(myDevice, (IWineD3DSurface *) This, CTXUSAGE_BLIT);
    ENTER_GL();

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it, every part of the code
     * that reads sets the read buffer as desired.
     */
    if(!swapchain) {
        /* Locking the primary render target which is not on a swapchain(=offscreen render target).
         * Read from the back buffer
         */
        TRACE("Locking offscreen render target\n");
        glReadBuffer(myDevice->offscreenBuffer);
        srcIsUpsideDown = TRUE;
    } else {
        GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *) This, (IWineD3DSwapChain *)swapchain);
        TRACE("Locking %#x buffer\n", buffer);
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
        srcIsUpsideDown = FALSE;
    }

    /* TODO: Get rid of the extra rectangle comparison and construction of a full surface rectangle */
    if(!rect) {
        local_rect.left = 0;
        local_rect.top = 0;
        local_rect.right = This->currentDesc.Width;
        local_rect.bottom = This->currentDesc.Height;
    } else {
        local_rect = *rect;
    }
    /* TODO: Get rid of the extra GetPitch call, LockRect does that too. Cache the pitch */

    switch(This->resource.format)
    {
        case WINED3DFMT_P8:
        {
            if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
                /* In case of P8 render targets the index is stored in the alpha component */
                fmt = GL_ALPHA;
                type = GL_UNSIGNED_BYTE;
                mem = dest;
                bpp = This->bytesPerPixel;
            } else {
                /* GL can't return palettized data, so read ARGB pixels into a
                 * separate block of memory and convert them into palettized format
                 * in software. Slow, but if the app means to use palettized render
                 * targets and locks it...
                 *
                 * Use GL_RGB, GL_UNSIGNED_BYTE to read the surface for performance reasons
                 * Don't use GL_BGR as in the WINED3DFMT_R8G8B8 case, instead watch out
                 * for the color channels when palettizing the colors.
                 */
                fmt = GL_RGB;
                type = GL_UNSIGNED_BYTE;
                pitch *= 3;
                mem = HeapAlloc(GetProcessHeap(), 0, This->resource.size * 3);
                if(!mem) {
                    ERR("Out of memory\n");
                    LEAVE_GL();
                    return;
                }
                bpp = This->bytesPerPixel * 3;
            }
        }
        break;

        default:
            mem = dest;
            fmt = This->glDescription.glFormat;
            type = This->glDescription.glType;
            bpp = This->bytesPerPixel;
    }

    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, This->pbo));
        checkGLcall("glBindBufferARB");
    }

    glReadPixels(local_rect.left, local_rect.top,
                 local_rect.right - local_rect.left,
                 local_rect.bottom - local_rect.top,
                 fmt, type, mem);
    vcheckGLcall("glReadPixels");

    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        /* Check if we need to flip the image. If we need to flip use glMapBufferARB
         * to get a pointer to it and perform the flipping in software. This is a lot
         * faster than calling glReadPixels for each line. In case we want more speed
         * we should rerender it flipped in a FBO and read the data back from the FBO. */
        if(!srcIsUpsideDown) {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
            checkGLcall("glBindBufferARB");

            mem = GL_EXTCALL(glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_READ_WRITE_ARB));
            checkGLcall("glMapBufferARB");
        }
    }

    /* TODO: Merge this with the palettization loop below for P8 targets */
    if(!srcIsUpsideDown) {
        UINT len, off;
        /* glReadPixels returns the image upside down, and there is no way to prevent this.
            Flip the lines in software */
        len = (local_rect.right - local_rect.left) * bpp;
        off = local_rect.left * bpp;

        row = HeapAlloc(GetProcessHeap(), 0, len);
        if(!row) {
            ERR("Out of memory\n");
            if(This->resource.format == WINED3DFMT_P8) HeapFree(GetProcessHeap(), 0, mem);
            LEAVE_GL();
            return;
        }

        top = mem + pitch * local_rect.top;
        bottom = ((BYTE *) mem) + pitch * ( local_rect.bottom - local_rect.top - 1);
        for(i = 0; i < (local_rect.bottom - local_rect.top) / 2; i++) {
            memcpy(row, top + off, len);
            memcpy(top + off, bottom + off, len);
            memcpy(bottom + off, row, len);
            top += pitch;
            bottom -= pitch;
        }
        HeapFree(GetProcessHeap(), 0, row);

        /* Unmap the temp PBO buffer */
        if(This->Flags & SFLAG_PBO) {
            GL_EXTCALL(glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB));
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        }
    }

    /* For P8 textures we need to perform an inverse palette lookup. This is done by searching for a palette
     * index which matches the RGB value. Note this isn't guaranteed to work when there are multiple entries for
     * the same color but we have no choice.
     * In case of render targets, the index is stored in the alpha component so no conversion is needed.
     */
    if((This->resource.format == WINED3DFMT_P8) && !(This->resource.usage & WINED3DUSAGE_RENDERTARGET)) {
        PALETTEENTRY *pal;
        DWORD width = pitch / 3;
        int x, y, c;
        if(This->palette) {
            pal = This->palette->palents;
        } else {
            pal = This->resource.wineD3DDevice->palettes[This->resource.wineD3DDevice->currentPalette];
        }

        for(y = local_rect.top; y < local_rect.bottom; y++) {
            for(x = local_rect.left; x < local_rect.right; x++) {
                /*                      start              lines            pixels      */
                BYTE *blue =  (BYTE *) ((BYTE *) mem) + y * pitch + x * (sizeof(BYTE) * 3);
                BYTE *green = blue  + 1;
                BYTE *red =   green + 1;

                for(c = 0; c < 256; c++) {
                    if(*red   == pal[c].peRed   &&
                       *green == pal[c].peGreen &&
                       *blue  == pal[c].peBlue)
                    {
                        *((BYTE *) dest + y * width + x) = c;
                        break;
                    }
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, mem);
    }
    LEAVE_GL();
}

static void surface_prepare_system_memory(IWineD3DSurfaceImpl *This) {
    /* Performance optimization: Count how often a surface is locked, if it is locked regularly do not throw away the system memory copy.
     * This avoids the need to download the surface from opengl all the time. The surface is still downloaded if the opengl texture is
     * changed
     */
    if(!(This->Flags & SFLAG_DYNLOCK)) {
        This->lockCount++;
        /* MAXLOCKCOUNT is defined in wined3d_private.h */
        if(This->lockCount > MAXLOCKCOUNT) {
            TRACE("Surface is locked regularly, not freeing the system memory copy any more\n");
            This->Flags |= SFLAG_DYNLOCK;
        }
    }

    /* Create a PBO for dynamically locked surfaces but don't do it for converted or non-pow2 surfaces.
     * Also don't create a PBO for systemmem surfaces.
     */
    if(GL_SUPPORT(ARB_PIXEL_BUFFER_OBJECT) && (This->Flags & SFLAG_DYNLOCK) && !(This->Flags & (SFLAG_PBO | SFLAG_CONVERTED | SFLAG_NONPOW2)) && (This->resource.pool != WINED3DPOOL_SYSTEMMEM)) {
        GLenum error;
        ENTER_GL();

        GL_EXTCALL(glGenBuffersARB(1, &This->pbo));
        error = glGetError();
        if(This->pbo == 0 || error != GL_NO_ERROR) {
            ERR("Failed to bind the PBO with error %s (%#x)\n", debug_glerror(error), error);
        }

        TRACE("Attaching pbo=%#x to (%p)\n", This->pbo, This);

        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
        checkGLcall("glBindBufferARB");

        GL_EXTCALL(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->resource.size + 4, This->resource.allocatedMemory, GL_STREAM_DRAW_ARB));
        checkGLcall("glBufferDataARB");

        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        /* We don't need the system memory anymore and we can't even use it for PBOs */
        if(!(This->Flags & SFLAG_CLIENT)) {
            HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
            This->resource.heapMemory = NULL;
        }
        This->resource.allocatedMemory = NULL;
        This->Flags |= SFLAG_PBO;
        LEAVE_GL();
    } else if(!(This->resource.allocatedMemory || This->Flags & SFLAG_PBO)) {
        /* Whatever surface we have, make sure that there is memory allocated for the downloaded copy,
         * or a pbo to map
         */
        if(!This->resource.heapMemory) {
            This->resource.heapMemory = HeapAlloc(GetProcessHeap() ,0 , This->resource.size + RESOURCE_ALIGNMENT);
        }
        This->resource.allocatedMemory =
                (BYTE *)(((ULONG_PTR) This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
        if(This->Flags & SFLAG_INSYSMEM) {
            ERR("Surface without memory or pbo has SFLAG_INSYSMEM set!\n");
        }
    }
}

static HRESULT WINAPI IWineD3DSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChain *swapchain = NULL;

    TRACE("(%p) : rect@%p flags(%08x), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);

    /* This is also done in the base class, but we have to verify this before loading any data from
     * gl into the sysmem copy. The PBO may be mapped, a different rectangle locked, the discard flag
     * may interfere, and all other bad things may happen
     */
    if (This->Flags & SFLAG_LOCKED) {
        WARN("Surface is already locked, returning D3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    This->Flags |= SFLAG_LOCKED;

    if (!(This->Flags & SFLAG_LOCKABLE))
    {
        TRACE("Warning: trying to lock unlockable surf@%p\n", This);
    }

    if (Flags & WINED3DLOCK_DISCARD) {
        /* Set SFLAG_INSYSMEM, so we'll never try to download the data from the texture. */
        TRACE("WINED3DLOCK_DISCARD flag passed, marking local copy as up to date\n");
        This->Flags |= SFLAG_INSYSMEM;
    }

    if (This->Flags & SFLAG_INSYSMEM) {
        TRACE("Local copy is up to date, not downloading data\n");
        surface_prepare_system_memory(This); /* Makes sure memory is allocated */
        goto lock_end;
    }

    /* Now download the surface content from opengl
     * Use the render target readback if the surface is on a swapchain(=onscreen render target) or the current primary target
     * Offscreen targets which are not active at the moment or are higher targets(fbos) can be locked with the texture path
     */
    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if(swapchain || iface == myDevice->render_targets[0]) {
        const RECT *pass_rect = pRect;

        /* IWineD3DSurface_LoadLocation does not check if the rectangle specifies the full surfaces
         * because most caller functions do not need that. So do that here
         */
        if(pRect &&
           pRect->top    == 0 &&
           pRect->left   == 0 &&
           pRect->right  == This->currentDesc.Width &&
           pRect->bottom == This->currentDesc.Height) {
            pass_rect = NULL;
        }

        switch(wined3d_settings.rendertargetlock_mode) {
            case RTL_TEXDRAW:
            case RTL_TEXTEX:
                FIXME("Reading from render target with a texture isn't implemented yet, falling back to framebuffer reading\n");
#if 0
                /* Disabled for now. LoadLocation prefers the texture over the drawable as the source. So if we copy to the
                 * texture first, then to sysmem, we'll avoid glReadPixels and use glCopyTexImage and glGetTexImage2D instead.
                 * This may be faster on some cards
                 */
                IWineD3DSurface_LoadLocation(iface, SFLAG_INTEXTURE, NULL /* No partial texture copy yet */);
#endif
                /* drop through */

            case RTL_AUTO:
            case RTL_READDRAW:
            case RTL_READTEX:
                IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, pRect);
                break;

            case RTL_DISABLE:
                break;
        }
        if(swapchain) IWineD3DSwapChain_Release(swapchain);

    } else if(iface == myDevice->stencilBufferTarget) {
        /** the depth stencil in openGL has a format of GL_FLOAT
         * which should be good for WINED3DFMT_D16_LOCKABLE
         * and WINED3DFMT_D16
         * it is unclear what format the stencil buffer is in except.
         * 'Each index is converted to fixed point...
         * If GL_MAP_STENCIL is GL_TRUE, indices are replaced by their
         * mappings in the table GL_PIXEL_MAP_S_TO_S.
         * glReadPixels(This->lockedRect.left,
         *             This->lockedRect.bottom - j - 1,
         *             This->lockedRect.right - This->lockedRect.left,
         *             1,
         *             GL_DEPTH_COMPONENT,
         *             type,
         *             (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-This->lockedRect.top)));
         *
         * Depth Stencil surfaces which are not the current depth stencil target should have their data in a
         * gl texture(next path), or in local memory(early return because of set SFLAG_INSYSMEM above). If
         * none of that is the case the problem is not in this function :-)
         ********************************************/
        FIXME("Depth stencil locking not supported yet\n");
    } else {
        /* This path is for normal surfaces, offscreen render targets and everything else that is in a gl texture */
        TRACE("locking an ordinary surface\n");
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL /* no partial locking for textures yet */);
    }

lock_end:
    if(This->Flags & SFLAG_PBO) {
        ActivateContext(myDevice, myDevice->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
        checkGLcall("glBindBufferARB");

        /* This shouldn't happen but could occur if some other function didn't handle the PBO properly */
        if(This->resource.allocatedMemory) {
            ERR("The surface already has PBO memory allocated!\n");
        }

        This->resource.allocatedMemory = GL_EXTCALL(glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_READ_WRITE_ARB));
        checkGLcall("glMapBufferARB");

        /* Make sure the pbo isn't set anymore in order not to break non-pbo calls */
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        LEAVE_GL();
    }

    if (Flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)) {
        /* Don't dirtify */
    } else {
        IWineD3DBaseTexture *pBaseTexture;
        /**
         * Dirtify on lock
         * as seen in msdn docs
         */
        IWineD3DSurface_AddDirtyRect(iface, pRect);

        /** Dirtify Container if needed */
        if (WINED3D_OK == IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&pBaseTexture) && pBaseTexture != NULL) {
            TRACE("Making container dirty\n");
            IWineD3DBaseTexture_SetDirty(pBaseTexture, TRUE);
            IWineD3DBaseTexture_Release(pBaseTexture);
        } else {
            TRACE("Surface is standalone, no need to dirty the container\n");
        }
    }

    return IWineD3DBaseSurfaceImpl_LockRect(iface, pLockedRect, pRect, Flags);
}

static void flush_to_framebuffer_drawpixels(IWineD3DSurfaceImpl *This) {
    GLint  prev_store;
    GLint  prev_rasterpos[4];
    GLint skipBytes = 0;
    BOOL storechanged = FALSE, memory_allocated = FALSE;
    GLint fmt, type;
    BYTE *mem;
    UINT bpp;
    UINT pitch = IWineD3DSurface_GetPitch((IWineD3DSurface *) This);    /* target is argb, 4 byte */
    IWineD3DDeviceImpl *myDevice = (IWineD3DDeviceImpl *) This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain;

    /* Activate the correct context for the render target */
    ActivateContext(myDevice, (IWineD3DSurface *) This, CTXUSAGE_BLIT);
    ENTER_GL();

    IWineD3DSurface_GetContainer((IWineD3DSurface *) This, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if(!swapchain) {
        /* Primary offscreen render target */
        TRACE("Offscreen render target\n");
        glDrawBuffer(myDevice->offscreenBuffer);
        checkGLcall("glDrawBuffer(myDevice->offscreenBuffer)");
    } else {
        GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *) This, (IWineD3DSwapChain *)swapchain);
        TRACE("Unlocking %#x buffer\n", buffer);
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
    }

    glDisable(GL_TEXTURE_2D);
    vcheckGLcall("glDisable(GL_TEXTURE_2D)");

    glFlush();
    vcheckGLcall("glFlush");
    glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
    vcheckGLcall("glIntegerv");
    glGetIntegerv(GL_CURRENT_RASTER_POSITION, &prev_rasterpos[0]);
    vcheckGLcall("glIntegerv");
    glPixelZoom(1.0, -1.0);
    vcheckGLcall("glPixelZoom");

    /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &skipBytes);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, This->currentDesc.Width);

    glRasterPos3i(This->lockedRect.left, This->lockedRect.top, 1);
    vcheckGLcall("glRasterPos2f");

    /* Some drivers(radeon dri, others?) don't like exceptions during
     * glDrawPixels. If the surface is a DIB section, it might be in GDIMode
     * after ReleaseDC. Reading it will cause an exception, which x11drv will
     * catch to put the dib section in InSync mode, which leads to a crash
     * and a blocked x server on my radeon card.
     *
     * The following lines read the dib section so it is put in inSync mode
     * before glDrawPixels is called and the crash is prevented. There won't
     * be any interfering gdi accesses, because UnlockRect is called from
     * ReleaseDC, and the app won't use the dc any more afterwards.
     */
    if((This->Flags & SFLAG_DIBSECTION) && !(This->Flags & SFLAG_PBO)) {
        volatile BYTE read;
        read = This->resource.allocatedMemory[0];
    }

    switch (This->resource.format) {
        /* No special care needed */
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_R8G8B8:
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
            break;

        case WINED3DFMT_X4R4G4B4:
        {
            int size;
            unsigned short *data;
            data = (unsigned short *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0xF000;
                data++;
                size--;
            }
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
        }
        break;

        case WINED3DFMT_X1R5G5B5:
        {
            int size;
            unsigned short *data;
            data = (unsigned short *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0x8000;
                data++;
                size--;
            }
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
        }
        break;

        case WINED3DFMT_X8R8G8B8:
        {
            /* make sure the X byte is set to alpha on, since it 
               could be any random value. This fixes the intro movie in Pirates! */
            int size;
            unsigned int *data;
            data = (unsigned int *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0xFF000000;
                data++;
                size--;
            }
        }
        /* Fall through */

        case WINED3DFMT_A8R8G8B8:
        {
            glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
            vcheckGLcall("glPixelStorei");
            storechanged = TRUE;
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
        }
        break;

        case WINED3DFMT_A2R10G10B10:
        {
            glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
            vcheckGLcall("glPixelStorei");
            storechanged = TRUE;
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
        }
        break;

        case WINED3DFMT_P8:
        {
            int height = This->glRect.bottom - This->glRect.top;
            type = GL_UNSIGNED_BYTE;
            fmt = GL_RGBA;

            mem = HeapAlloc(GetProcessHeap(), 0, This->resource.size * sizeof(DWORD));
            if(!mem) {
                ERR("Out of memory\n");
                goto cleanup;
            }
            memory_allocated = TRUE;
            d3dfmt_convert_surface(This->resource.allocatedMemory,
                                   mem,
                                   pitch,
                                   pitch,
                                   height,
                                   pitch * 4,
                                   CONVERT_PALETTED,
                                   This);
            bpp = This->bytesPerPixel * 4;
            pitch *= 4;
        }
        break;

        default:
            FIXME("Unsupported Format %u in locking func\n", This->resource.format);

            /* Give it a try */
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            bpp = This->bytesPerPixel;
    }

    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
        checkGLcall("glBindBufferARB");
    }

    glDrawPixels(This->lockedRect.right - This->lockedRect.left,
                 (This->lockedRect.bottom - This->lockedRect.top)-1,
                 fmt, type,
                 mem + bpp * This->lockedRect.left + pitch * This->lockedRect.top);
    checkGLcall("glDrawPixels");

cleanup:
    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");
    }

    glPixelZoom(1.0,1.0);
    vcheckGLcall("glPixelZoom");

    glRasterPos3iv(&prev_rasterpos[0]);
    vcheckGLcall("glRasterPos3iv");

    /* Reset to previous pack row length */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, skipBytes);
    vcheckGLcall("glPixelStorei GL_UNPACK_ROW_LENGTH");
    if(storechanged) {
        glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
        vcheckGLcall("glPixelStorei GL_PACK_SWAP_BYTES");
    }

    /* Blitting environment requires that 2D texturing is enabled. It was turned off before,
     * turn it on again
     */
    glEnable(GL_TEXTURE_2D);
    checkGLcall("glEnable(GL_TEXTURE_2D)");

    if(memory_allocated) HeapFree(GetProcessHeap(), 0, mem);

    if(!swapchain) {
        glDrawBuffer(myDevice->offscreenBuffer);
        checkGLcall("glDrawBuffer(myDevice->offscreenBuffer)");
    } else if(swapchain->backBuffer) {
        glDrawBuffer(GL_BACK);
        checkGLcall("glDrawBuffer(GL_BACK)");
    } else {
        glDrawBuffer(GL_FRONT);
        checkGLcall("glDrawBuffer(GL_FRONT)");
    }
    LEAVE_GL();

    return;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_UnlockRect(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain = NULL;
    BOOL fullsurface;

    if (!(This->Flags & SFLAG_LOCKED)) {
        WARN("trying to Unlock an unlocked surf@%p\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if (This->Flags & SFLAG_PBO) {
        TRACE("Freeing PBO memory\n");
        ActivateContext(myDevice, myDevice->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
        GL_EXTCALL(glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB));
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glUnmapBufferARB");
        LEAVE_GL();
        This->resource.allocatedMemory = NULL;
    }

    TRACE("(%p) : dirtyfied(%d)\n", This, This->Flags & (SFLAG_INDRAWABLE | SFLAG_INTEXTURE) ? 0 : 1);

    if (This->Flags & (SFLAG_INDRAWABLE | SFLAG_INTEXTURE)) {
        TRACE("(%p) : Not Dirtified so nothing to do, return now\n", This);
        goto unlock_end;
    }

    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if(swapchain || (myDevice->render_targets && iface == myDevice->render_targets[0])) {
        if(wined3d_settings.rendertargetlock_mode == RTL_DISABLE) {
            static BOOL warned = FALSE;
            if(!warned) {
                ERR("The application tries to write to the render target, but render target locking is disabled\n");
                warned = TRUE;
            }
            if(swapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
            goto unlock_end;
        }

        if(This->dirtyRect.left   == 0 &&
           This->dirtyRect.top    == 0 &&
           This->dirtyRect.right  == This->currentDesc.Width &&
           This->dirtyRect.bottom == This->currentDesc.Height) {
            fullsurface = TRUE;
        } else {
            /* TODO: Proper partial rectangle tracking */
            fullsurface = FALSE;
            This->Flags |= SFLAG_INSYSMEM;
        }

        switch(wined3d_settings.rendertargetlock_mode) {
            case RTL_READTEX:
            case RTL_TEXTEX:
                ActivateContext(myDevice, iface, CTXUSAGE_BLIT);
                ENTER_GL();
                if (This->glDescription.textureName == 0) {
                    glGenTextures(1, &This->glDescription.textureName);
                    checkGLcall("glGenTextures");
                }
                glBindTexture(GL_TEXTURE_2D, This->glDescription.textureName);
                checkGLcall("glBindTexture(GL_TEXTURE_2D, This->glDescription.textureName);");
                LEAVE_GL();
                IWineD3DSurface_LoadLocation(iface, SFLAG_INTEXTURE, NULL /* partial texture loading not supported yet */);
                /* drop through */

            case RTL_AUTO:
            case RTL_READDRAW:
            case RTL_TEXDRAW:
                IWineD3DSurface_LoadLocation(iface, SFLAG_INDRAWABLE, fullsurface ? NULL : &This->dirtyRect);
                break;
        }

        if(!fullsurface) {
            /* Partial rectangle tracking is not commonly implemented, it is only done for render targets. Overwrite
             * the flags to bring them back into a sane state. INSYSMEM was set before to tell LoadLocation where
             * to read the rectangle from. Indrawable is set because all modifications from the partial sysmem copy
             * are written back to the drawable, thus the surface is merged again in the drawable. The sysmem copy is
             * not fully up to date because only a subrectangle was read in LockRect.
             */
            This->Flags &= ~SFLAG_INSYSMEM;
            This->Flags |= SFLAG_INDRAWABLE;
        }

        This->dirtyRect.left   = This->currentDesc.Width;
        This->dirtyRect.top    = This->currentDesc.Height;
        This->dirtyRect.right  = 0;
        This->dirtyRect.bottom = 0;
    } else if(iface == myDevice->stencilBufferTarget) {
        FIXME("Depth Stencil buffer locking is not implemented\n");
    } else {
        /* The rest should be a normal texture */
        IWineD3DBaseTextureImpl *impl;
        /* Check if the texture is bound, if yes dirtify the sampler to force a re-upload of the texture
         * Can't load the texture here because PreLoad may destroy and recreate the gl texture, so sampler
         * states need resetting
         */
        if(IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&impl) == WINED3D_OK) {
            if(impl->baseTexture.bindCount) {
                IWineD3DDeviceImpl_MarkStateDirty(myDevice, STATE_SAMPLER(impl->baseTexture.sampler));
            }
            IWineD3DBaseTexture_Release((IWineD3DBaseTexture *) impl);
        }
    }

    unlock_end:
    This->Flags &= ~SFLAG_LOCKED;
    memset(&This->lockedRect, 0, sizeof(RECT));
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetDC(IWineD3DSurface *iface, HDC *pHDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    WINED3DLOCKED_RECT lock;
    HRESULT hr;
    RGBQUAD col[256];

    TRACE("(%p)->(%p)\n",This,pHDC);

    if(This->Flags & SFLAG_USERPTR) {
        ERR("Not supported on surfaces with an application-provided surfaces\n");
        return WINEDDERR_NODC;
    }

    /* Give more detailed info for ddraw */
    if (This->Flags & SFLAG_DCINUSE)
        return WINEDDERR_DCALREADYCREATED;

    /* Can't GetDC if the surface is locked */
    if (This->Flags & SFLAG_LOCKED)
        return WINED3DERR_INVALIDCALL;

    memset(&lock, 0, sizeof(lock)); /* To be sure */

    /* Create a DIB section if there isn't a hdc yet */
    if(!This->hDC) {
        IWineD3DBaseSurfaceImpl_CreateDIBSection(iface);
        if(This->Flags & SFLAG_CLIENT) {
            IWineD3DSurface_PreLoad(iface);
        }

        /* Use the dib section from now on if we are not using a PBO */
        if(!(This->Flags & SFLAG_PBO))
            This->resource.allocatedMemory = This->dib.bitmap_data;
    }

    /* Lock the surface */
    hr = IWineD3DSurface_LockRect(iface,
                                  &lock,
                                  NULL,
                                  0);

    if(This->Flags & SFLAG_PBO) {
        /* Sync the DIB with the PBO. This can't be done earlier because LockRect activates the allocatedMemory */
        memcpy(This->dib.bitmap_data, This->resource.allocatedMemory, This->dib.bitmap_size);
    }

    if(FAILED(hr)) {
        ERR("IWineD3DSurface_LockRect failed with hr = %08x\n", hr);
        /* keep the dib section */
        return hr;
    }

    if(This->resource.format == WINED3DFMT_P8 ||
        This->resource.format == WINED3DFMT_A8P8) {
        unsigned int n;
        if(This->palette) {
            PALETTEENTRY ent[256];

            GetPaletteEntries(This->palette->hpal, 0, 256, ent);
            for (n=0; n<256; n++) {
                col[n].rgbRed   = ent[n].peRed;
                col[n].rgbGreen = ent[n].peGreen;
                col[n].rgbBlue  = ent[n].peBlue;
                col[n].rgbReserved = 0;
            }
        } else {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            for (n=0; n<256; n++) {
                col[n].rgbRed   = device->palettes[device->currentPalette][n].peRed;
                col[n].rgbGreen = device->palettes[device->currentPalette][n].peGreen;
                col[n].rgbBlue  = device->palettes[device->currentPalette][n].peBlue;
                col[n].rgbReserved = 0;
            }

        }
        SetDIBColorTable(This->hDC, 0, 256, col);
    }

    *pHDC = This->hDC;
    TRACE("returning %p\n",*pHDC);
    This->Flags |= SFLAG_DCINUSE;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_ReleaseDC(IWineD3DSurface *iface, HDC hDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,hDC);

    if (!(This->Flags & SFLAG_DCINUSE))
        return WINED3DERR_INVALIDCALL;

    if (This->hDC !=hDC) {
        WARN("Application tries to release an invalid DC(%p), surface dc is %p\n", hDC, This->hDC);
        return WINED3DERR_INVALIDCALL;
    }

    if((This->Flags & SFLAG_PBO) && This->resource.allocatedMemory) {
        /* Copy the contents of the DIB over to the PBO */
        memcpy(This->resource.allocatedMemory, This->dib.bitmap_data, This->dib.bitmap_size);
    }

    /* we locked first, so unlock now */
    IWineD3DSurface_UnlockRect(iface);

    This->Flags &= ~SFLAG_DCINUSE;

    return WINED3D_OK;
}

/* ******************************************************
   IWineD3DSurface Internal (No mapping to directx api) parts follow
   ****************************************************** */

HRESULT d3dfmt_get_conv(IWineD3DSurfaceImpl *This, BOOL need_alpha_ck, BOOL use_texturing, GLenum *format, GLenum *internal, GLenum *type, CONVERT_TYPES *convert, int *target_bpp, BOOL srgb_mode) {
    BOOL colorkey_active = need_alpha_ck && (This->CKeyFlags & WINEDDSD_CKSRCBLT);
    const GlPixelFormatDesc *glDesc;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL p8_render_target = FALSE;
    getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);

    /* Default values: From the surface */
    *format = glDesc->glFormat;
    *internal = srgb_mode?glDesc->glGammaInternal:glDesc->glInternal;
    *type = glDesc->glType;
    *convert = NO_CONVERSION;
    *target_bpp = This->bytesPerPixel;

    /* Ok, now look if we have to do any conversion */
    switch(This->resource.format) {
        case WINED3DFMT_P8:
            /* ****************
                Paletted Texture
                **************** */

            if (device->render_targets && device->render_targets[0]) {
                IWineD3DSurfaceImpl* render_target = (IWineD3DSurfaceImpl*)device->render_targets[0];
                if((render_target->resource.usage & WINED3DUSAGE_RENDERTARGET) && (render_target->resource.format == WINED3DFMT_P8))
                    p8_render_target = TRUE;
            }

            /* Use conversion when the paletted texture extension is not available, or when it is available make sure it is used
             * for texturing as it won't work for calls like glDraw-/glReadPixels and further also use conversion in case of color keying.
             * Paletted textures can be emulated using shaders but only do that for 2D purposes e.g. situations in which the main render target uses p8. Some games like GTA Vice City use P8 for texturing which conflicts with this.
             */
            if( !(GL_SUPPORT(EXT_PALETTED_TEXTURE) || (GL_SUPPORT(ARB_FRAGMENT_PROGRAM) && p8_render_target))  || colorkey_active || (!use_texturing && GL_SUPPORT(EXT_PALETTED_TEXTURE)) ) {
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_BYTE;
                *target_bpp = 4;
                if(colorkey_active) {
                    *convert = CONVERT_PALETTED_CK;
                } else {
                    *convert = CONVERT_PALETTED;
                }
            }
            else if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
                *format = GL_RED;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_BYTE;
                *target_bpp = 1;
            }

            break;

        case WINED3DFMT_R3G3B2:
            /* **********************
                GL_UNSIGNED_BYTE_3_3_2
                ********************** */
            if (colorkey_active) {
                /* This texture format will never be used.. So do not care about color keying
                    up until the point in time it will be needed :-) */
                FIXME(" ColorKeying not supported in the RGB 332 format !\n");
            }
            break;

        case WINED3DFMT_R5G6B5:
            if (colorkey_active) {
                *convert = CONVERT_CK_565;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_SHORT_5_5_5_1;
            }
            break;

        case WINED3DFMT_X1R5G5B5:
            if (colorkey_active) {
                *convert = CONVERT_CK_5551;
                *format = GL_BGRA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            }
            break;

        case WINED3DFMT_R8G8B8:
            if (colorkey_active) {
                *convert = CONVERT_CK_RGB24;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_INT_8_8_8_8;
                *target_bpp = 4;
            }
            break;

        case WINED3DFMT_X8R8G8B8:
            if (colorkey_active) {
                *convert = CONVERT_RGB32_888;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_INT_8_8_8_8;
            }
            break;

        case WINED3DFMT_V8U8:
            if(GL_SUPPORT(NV_TEXTURE_SHADER3)) break;
            else if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
                *format = GL_DUDV_ATI;
                *internal = GL_DU8DV8_ATI;
                *type = GL_BYTE;
                /* No conversion - Just change the gl type */
                break;
            }
            *convert = CONVERT_V8U8;
            *format = GL_BGR;
            *internal = GL_RGB8;
            *type = GL_UNSIGNED_BYTE;
            *target_bpp = 3;
            break;

        case WINED3DFMT_L6V5U5:
            *convert = CONVERT_L6V5U5;
            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                *target_bpp = 3;
                /* Use format and types from table */
            } else {
                /* Load it into unsigned R5G6B5, swap L and V channels, and revert that in the shader */
                *target_bpp = 2;
                *format = GL_RGB;
                *internal = GL_RGB5;
                *type = GL_UNSIGNED_SHORT_5_6_5;
            }
            break;

        case WINED3DFMT_X8L8V8U8:
            *convert = CONVERT_X8L8V8U8;
            *target_bpp = 4;
            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Use formats from gl table. It is a bit unfortunate, but the conversion
                 * is needed to set the X format to 255 to get 1.0 for alpha when sampling
                 * the texture. OpenGL can't use GL_DSDT8_MAG8_NV as internal format with
                 * the needed type and format parameter, so the internal format contains a
                 * 4th component, which is returned as alpha
                 */
            } else {
                /* Not supported by GL_ATI_envmap_bumpmap */
                *format = GL_BGRA;
                *internal = GL_RGB8;
                *type = GL_UNSIGNED_INT_8_8_8_8_REV;
            }
            break;

        case WINED3DFMT_Q8W8V8U8:
            if(GL_SUPPORT(NV_TEXTURE_SHADER3)) break;
            *convert = CONVERT_Q8W8V8U8;
            *format = GL_BGRA;
            *internal = GL_RGBA8;
            *type = GL_UNSIGNED_BYTE;
            *target_bpp = 4;
            /* Not supported by GL_ATI_envmap_bumpmap */
            break;

        case WINED3DFMT_V16U16:
            if(GL_SUPPORT(NV_TEXTURE_SHADER3)) break;
            *convert = CONVERT_V16U16;
            *format = GL_BGR;
            *internal = GL_RGB16_EXT;
            *type = GL_UNSIGNED_SHORT;
            *target_bpp = 6;
            /* What should I do here about GL_ATI_envmap_bumpmap?
             * Convert it or allow data loss by loading it into a 8 bit / channel texture?
             */
            break;

        case WINED3DFMT_A4L4:
            /* A4L4 exists as an internal gl format, but for some reason there is not
             * format+type combination to load it. Thus convert it to A8L8, then load it
             * with A4L4 internal, but A8L8 format+type
             */
            *convert = CONVERT_A4L4;
            *format = GL_LUMINANCE_ALPHA;
            *internal = GL_LUMINANCE4_ALPHA4;
            *type = GL_UNSIGNED_BYTE;
            *target_bpp = 2;
            break;

        case WINED3DFMT_R32F:
            /* Can be loaded in theory with fmt=GL_RED, type=GL_FLOAT, but this fails. The reason
             * is that D3D expects the undefined green, blue and alpha channels to return 1.0
             * when sampling, but OpenGL sets green and blue to 0.0 instead. Thus we have to inject
             * 1.0 instead.
             *
             * The alpha channel defaults to 1.0 in opengl, so nothing has to be done about it.
             */
            *convert = CONVERT_R32F;
            *format = GL_RGB;
            *internal = GL_RGB32F_ARB;
            *type = GL_FLOAT;
            *target_bpp = 12;
            break;

        case WINED3DFMT_R16F:
            /* Similar to R32F */
            *convert = CONVERT_R16F;
            *format = GL_RGB;
            *internal = GL_RGB16F_ARB;
            *type = GL_HALF_FLOAT_ARB;
            *target_bpp = 6;
            break;

        default:
            break;
    }

    return WINED3D_OK;
}

HRESULT d3dfmt_convert_surface(BYTE *src, BYTE *dst, UINT pitch, UINT width, UINT height, UINT outpitch, CONVERT_TYPES convert, IWineD3DSurfaceImpl *This) {
    BYTE *source, *dest;
    TRACE("(%p)->(%p),(%d,%d,%d,%d,%p)\n", src, dst, pitch, height, outpitch, convert,This);

    switch (convert) {
        case NO_CONVERSION:
        {
            memcpy(dst, src, pitch * height);
            break;
        }
        case CONVERT_PALETTED:
        case CONVERT_PALETTED_CK:
        {
            IWineD3DPaletteImpl* pal = This->palette;
            BYTE table[256][4];
            unsigned int x, y;

            if( pal == NULL) {
                /* TODO: If we are a sublevel, try to get the palette from level 0 */
            }

            d3dfmt_p8_init_palette(This, table, (convert == CONVERT_PALETTED_CK));

            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                /* This is an 1 bpp format, using the width here is fine */
                for (x = 0; x < width; x++) {
                    BYTE color = *source++;
                    *dest++ = table[color][0];
                    *dest++ = table[color][1];
                    *dest++ = table[color][2];
                    *dest++ = table[color][3];
                }
            }
        }
        break;

        case CONVERT_CK_565:
        {
            /* Converting the 565 format in 5551 packed to emulate color-keying.

              Note : in all these conversion, it would be best to average the averaging
                      pixels to get the color of the pixel that will be color-keyed to
                      prevent 'color bleeding'. This will be done later on if ever it is
                      too visible.

              Note2: Nvidia documents say that their driver does not support alpha + color keying
                     on the same surface and disables color keying in such a case
            */
            unsigned int x, y;
            WORD *Source;
            WORD *Dest;

            TRACE("Color keyed 565\n");

            for (y = 0; y < height; y++) {
                Source = (WORD *) (src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD color = *Source++;
                    *Dest = ((color & 0xFFC0) | ((color & 0x1F) << 1));
                    if ((color < This->SrcBltCKey.dwColorSpaceLowValue) ||
                        (color > This->SrcBltCKey.dwColorSpaceHighValue)) {
                        *Dest |= 0x0001;
                    }
                    Dest++;
                }
            }
        }
        break;

        case CONVERT_CK_5551:
        {
            /* Converting X1R5G5B5 format to R5G5B5A1 to emulate color-keying. */
            unsigned int x, y;
            WORD *Source;
            WORD *Dest;
            TRACE("Color keyed 5551\n");
            for (y = 0; y < height; y++) {
                Source = (WORD *) (src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD color = *Source++;
		    *Dest = color;
                    if ((color < This->SrcBltCKey.dwColorSpaceLowValue) ||
                        (color > This->SrcBltCKey.dwColorSpaceHighValue)) {
                        *Dest |= (1 << 15);
                    }
                    else {
                        *Dest &= ~(1 << 15);
                    }
                    Dest++;
                }
            }
        }
        break;

        case CONVERT_V8U8:
        {
            unsigned int x, y;
            short *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = (short *) (src + y * pitch);
                Dest = (unsigned char *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    long color = (*Source++);
                    /* B */ Dest[0] = 0xff;
                    /* G */ Dest[1] = (color >> 8) + 128; /* V */
                    /* R */ Dest[2] = (color) + 128;      /* U */
                    Dest += 3;
                }
            }
            break;
        }

        case CONVERT_V16U16:
        {
            unsigned int x, y;
            DWORD *Source;
            unsigned short *Dest;
            for(y = 0; y < height; y++) {
                Source = (DWORD *) (src + y * pitch);
                Dest = (unsigned short *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    DWORD color = (*Source++);
                    /* B */ Dest[0] = 0xffff;
                    /* G */ Dest[1] = (color >> 16) + 32768; /* V */
                    /* R */ Dest[2] = (color      ) + 32768; /* U */
                    Dest += 3;
                }
            }
            break;
        }

        case CONVERT_Q8W8V8U8:
        {
            unsigned int x, y;
            DWORD *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = (DWORD *) (src + y * pitch);
                Dest = (unsigned char *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    long color = (*Source++);
                    /* B */ Dest[0] = ((color >> 16) & 0xff) + 128; /* W */
                    /* G */ Dest[1] = ((color >> 8 ) & 0xff) + 128; /* V */
                    /* R */ Dest[2] = (color         & 0xff) + 128; /* U */
                    /* A */ Dest[3] = ((color >> 24) & 0xff) + 128; /* Q */
                    Dest += 4;
                }
            }
            break;
        }

        case CONVERT_L6V5U5:
        {
            unsigned int x, y;
            WORD *Source;
            unsigned char *Dest;

            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* This makes the gl surface bigger(24 bit instead of 16), but it works with
                 * fixed function and shaders without further conversion once the surface is
                 * loaded
                 */
                for(y = 0; y < height; y++) {
                    Source = (WORD *) (src + y * pitch);
                    Dest = (unsigned char *) (dst + y * outpitch);
                    for (x = 0; x < width; x++ ) {
                        short color = (*Source++);
                        unsigned char l = ((color >> 10) & 0xfc);
                                  char v = ((color >>  5) & 0x3e);
                                  char u = ((color      ) & 0x1f);

                        /* 8 bits destination, 6 bits source, 8th bit is the sign. gl ignores the sign
                         * and doubles the positive range. Thus shift left only once, gl does the 2nd
                         * shift. GL reads a signed value and converts it into an unsigned value.
                         */
                        /* M */ Dest[2] = l << 1;

                        /* Those are read as signed, but kept signed. Just left-shift 3 times to scale
                         * from 5 bit values to 8 bit values.
                         */
                        /* V */ Dest[1] = v << 3;
                        /* U */ Dest[0] = u << 3;
                        Dest += 3;
                    }
                }
            } else {
                for(y = 0; y < height; y++) {
                    unsigned short *Dest_s = (unsigned short *) (dst + y * outpitch);
                    Source = (WORD *) (src + y * pitch);
                    for (x = 0; x < width; x++ ) {
                        short color = (*Source++);
                        unsigned char l = ((color >> 10) & 0xfc);
                                 short v = ((color >>  5) & 0x3e);
                                 short u = ((color      ) & 0x1f);
                        short v_conv = v + 16;
                        short u_conv = u + 16;

                        *Dest_s = ((v_conv << 11) & 0xf800) | ((l << 5) & 0x7e0) | (u_conv & 0x1f);
                        Dest_s += 1;
                    }
                }
            }
            break;
        }

        case CONVERT_X8L8V8U8:
        {
            unsigned int x, y;
            DWORD *Source;
            unsigned char *Dest;

            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* This implementation works with the fixed function pipeline and shaders
                 * without further modification after converting the surface.
                 */
                for(y = 0; y < height; y++) {
                    Source = (DWORD *) (src + y * pitch);
                    Dest = (unsigned char *) (dst + y * outpitch);
                    for (x = 0; x < width; x++ ) {
                        long color = (*Source++);
                        /* L */ Dest[2] = ((color >> 16) & 0xff);   /* L */
                        /* V */ Dest[1] = ((color >> 8 ) & 0xff);   /* V */
                        /* U */ Dest[0] = (color         & 0xff);   /* U */
                        /* I */ Dest[3] = 255;                      /* X */
                        Dest += 4;
                    }
                }
            } else {
                /* Doesn't work correctly with the fixed function pipeline, but can work in
                 * shaders if the shader is adjusted. (There's no use for this format in gl's
                 * standard fixed function pipeline anyway).
                 */
                for(y = 0; y < height; y++) {
                    Source = (DWORD *) (src + y * pitch);
                    Dest = (unsigned char *) (dst + y * outpitch);
                    for (x = 0; x < width; x++ ) {
                        long color = (*Source++);
                        /* B */ Dest[0] = ((color >> 16) & 0xff);       /* L */
                        /* G */ Dest[1] = ((color >> 8 ) & 0xff) + 128; /* V */
                        /* R */ Dest[2] = (color         & 0xff) + 128;  /* U */
                        Dest += 4;
                    }
                }
            }
            break;
        }

        case CONVERT_A4L4:
        {
            unsigned int x, y;
            unsigned char *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = (unsigned char *) (src + y * pitch);
                Dest = (unsigned char *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    unsigned char color = (*Source++);
                    /* A */ Dest[1] = (color & 0xf0) << 0;
                    /* L */ Dest[0] = (color & 0x0f) << 4;
                    Dest += 2;
                }
            }
            break;
        }

        case CONVERT_R32F:
        {
            unsigned int x, y;
            float *Source;
            float *Dest;
            for(y = 0; y < height; y++) {
                Source = (float *) (src + y * pitch);
                Dest = (float *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    float color = (*Source++);
                    Dest[0] = color;
                    Dest[1] = 1.0;
                    Dest[2] = 1.0;
                    Dest += 3;
                }
            }
            break;
        }

        case CONVERT_R16F:
        {
            unsigned int x, y;
            WORD *Source;
            WORD *Dest;
            WORD one = 0x3c00;
            for(y = 0; y < height; y++) {
                Source = (WORD *) (src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD color = (*Source++);
                    Dest[0] = color;
                    Dest[1] = one;
                    Dest[2] = one;
                    Dest += 3;
                }
            }
            break;
        }
        default:
            ERR("Unsupported conversation type %d\n", convert);
    }
    return WINED3D_OK;
}

static void d3dfmt_p8_init_palette(IWineD3DSurfaceImpl *This, BYTE table[256][4], BOOL colorkey) {
    IWineD3DPaletteImpl* pal = This->palette;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL index_in_alpha = FALSE;
    int i;

    /* Old games like StarCraft, C&C, Red Alert and others use P8 render targets.
    * Reading back the RGB output each lockrect (each frame as they lock the whole screen)
    * is slow. Further RGB->P8 conversion is not possible because palettes can have
    * duplicate entries. Store the color key in the unused alpha component to speed the
    * download up and to make conversion unneeded. */
    if (device->render_targets && device->render_targets[0]) {
        IWineD3DSurfaceImpl* render_target = (IWineD3DSurfaceImpl*)device->render_targets[0];

        if(render_target->resource.usage & WINED3DUSAGE_RENDERTARGET)
            index_in_alpha = TRUE;
    }

    if (pal == NULL) {
        /* Still no palette? Use the device's palette */
        /* Get the surface's palette */
        for (i = 0; i < 256; i++) {
            table[i][0] = device->palettes[device->currentPalette][i].peRed;
            table[i][1] = device->palettes[device->currentPalette][i].peGreen;
            table[i][2] = device->palettes[device->currentPalette][i].peBlue;

            if(index_in_alpha) {
                table[i][3] = i;
            } else if (colorkey &&
                (i >= This->SrcBltCKey.dwColorSpaceLowValue) &&
                (i <= This->SrcBltCKey.dwColorSpaceHighValue)) {
                /* We should maybe here put a more 'neutral' color than the standard bright purple
                   one often used by application to prevent the nice purple borders when bi-linear
                   filtering is on */
                table[i][3] = 0x00;
            } else {
                table[i][3] = 0xFF;
            }
        }
    } else {
        TRACE("Using surface palette %p\n", pal);
        /* Get the surface's palette */
        for (i = 0; i < 256; i++) {
            table[i][0] = pal->palents[i].peRed;
            table[i][1] = pal->palents[i].peGreen;
            table[i][2] = pal->palents[i].peBlue;

            if(index_in_alpha) {
                table[i][3] = i;
            }
            else if (colorkey &&
                (i >= This->SrcBltCKey.dwColorSpaceLowValue) &&
                (i <= This->SrcBltCKey.dwColorSpaceHighValue)) {
                /* We should maybe here put a more 'neutral' color than the standard bright purple
                   one often used by application to prevent the nice purple borders when bi-linear
                   filtering is on */
                table[i][3] = 0x00;
            } else if(pal->Flags & WINEDDPCAPS_ALPHA) {
                table[i][3] = pal->palents[i].peFlags;
            } else {
                table[i][3] = 0xFF;
            }
        }
    }
}

const char *fragment_palette_conversion =
    "!!ARBfp1.0\n"
    "TEMP index;\n"
    "PARAM constants = { 0.996, 0.00195, 0, 0 };\n" /* { 255/256, 0.5/255*255/256, 0, 0 } */
    "TEX index.x, fragment.texcoord[0], texture[0], 2D;\n" /* store the red-component of the current pixel */
    "MAD index.x, index.x, constants.x, constants.y;\n" /* Scale the index by 255/256 and add a bias of '0.5' in order to sample in the middle */
    "TEX result.color, index, texture[1], 1D;\n" /* use the red-component as a index in the palette to get the final color */
    "END";

/* This function is used in case of 8bit paletted textures to upload the palette.
   It supports GL_EXT_paletted_texture and GL_ARB_fragment_program, support for other
   extensions like ATI_fragment_shaders is possible.
*/
static void d3dfmt_p8_upload_palette(IWineD3DSurface *iface, CONVERT_TYPES convert) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    BYTE table[256][4];
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    d3dfmt_p8_init_palette(This, table, (convert == CONVERT_PALETTED_CK));

    /* Try to use the paletted texture extension */
    if(GL_SUPPORT(EXT_PALETTED_TEXTURE))
    {
        TRACE("Using GL_EXT_PALETTED_TEXTURE for 8-bit paletted texture support\n");
        GL_EXTCALL(glColorTableEXT(GL_TEXTURE_2D,GL_RGBA,256,GL_RGBA,GL_UNSIGNED_BYTE, table));
    }
    else
    {
        /* Let a fragment shader do the color conversion by uploading the palette to a 1D texture.
         * The 8bit pixel data will be used as an index in this palette texture to retrieve the final color. */
        TRACE("Using fragment shaders for emulating 8-bit paletted texture support\n");

        /* Create the fragment program if we don't have it */
        if(!device->paletteConversionShader)
        {
            glEnable(GL_FRAGMENT_PROGRAM_ARB);
            GL_EXTCALL(glGenProgramsARB(1, &device->paletteConversionShader));
            GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, device->paletteConversionShader));
            GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(fragment_palette_conversion), (const GLbyte *)fragment_palette_conversion));
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
        }

        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, device->paletteConversionShader));

        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE1));
        glEnable(GL_TEXTURE_1D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); /* Make sure we have discrete color levels. */
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, table); /* Upload the palette */

        /* Switch back to unit 0 in which the 2D texture will be stored. */
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0));

        /* Rebind the texture because it isn't bound anymore */
        glBindTexture(This->glDescription.target, This->glDescription.textureName);
    }
}

static BOOL palette9_changed(IWineD3DSurfaceImpl *This) {
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    if(This->palette || (This->resource.format != WINED3DFMT_P8 && This->resource.format != WINED3DFMT_A8P8)) {
        /* If a ddraw-style palette is attached assume no d3d9 palette change.
         * Also the palette isn't interesting if the surface format isn't P8 or A8P8
         */
        return FALSE;
    }

    if(This->palette9) {
        if(memcmp(This->palette9, &device->palettes[device->currentPalette], sizeof(PALETTEENTRY) * 256) == 0) {
            return FALSE;
        }
    } else {
        This->palette9 = HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * 256);
    }
    memcpy(This->palette9, &device->palettes[device->currentPalette], sizeof(PALETTEENTRY) * 256);
    return TRUE;
}

static inline void clear_unused_channels(IWineD3DSurfaceImpl *This) {
    GLboolean oldwrite[4];

    /* Some formats have only some color channels, and the others are 1.0.
     * since our rendering renders to all channels, and those pixel formats
     * are emulated by using a full texture with the other channels set to 1.0
     * manually, clear the unused channels.
     *
     * This could be done with hacking colorwriteenable to mask the colors,
     * but before drawing the buffer would have to be cleared too, so there's
     * no gain in that
     */
    switch(This->resource.format) {
        case WINED3DFMT_R16F:
        case WINED3DFMT_R32F:
            TRACE("R16F or R32F format, clearing green, blue and alpha to 1.0\n");
            /* Do not activate a context, the correct drawable is active already
             * though just the read buffer is set, make sure to have the correct draw
             * buffer too
             */
            glDrawBuffer(This->resource.wineD3DDevice->offscreenBuffer);
            glDisable(GL_SCISSOR_TEST);
            glGetBooleanv(GL_COLOR_WRITEMASK, oldwrite);
            glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
            glClearColor(0.0, 1.0, 1.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            glColorMask(oldwrite[0], oldwrite[1], oldwrite[2], oldwrite[3]);
            if(!This->resource.wineD3DDevice->render_offscreen) glDrawBuffer(GL_BACK);
            checkGLcall("Unused channel clear\n");
            break;

        default: break;
    }
}

static HRESULT WINAPI IWineD3DSurfaceImpl_LoadTexture(IWineD3DSurface *iface, BOOL srgb_mode) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if (!(This->Flags & SFLAG_INTEXTURE)) {
        TRACE("Reloading because surface is dirty\n");
    } else if(/* Reload: gl texture has ck, now no ckey is set OR */
              ((This->Flags & SFLAG_GLCKEY) && (!(This->CKeyFlags & WINEDDSD_CKSRCBLT))) ||
              /* Reload: vice versa  OR */
              ((!(This->Flags & SFLAG_GLCKEY)) && (This->CKeyFlags & WINEDDSD_CKSRCBLT)) ||
              /* Also reload: Color key is active AND the color key has changed */
              ((This->CKeyFlags & WINEDDSD_CKSRCBLT) && (
                (This->glCKey.dwColorSpaceLowValue != This->SrcBltCKey.dwColorSpaceLowValue) ||
                (This->glCKey.dwColorSpaceHighValue != This->SrcBltCKey.dwColorSpaceHighValue)))) {
        TRACE("Reloading because of color keying\n");
        /* To perform the color key conversion we need a sysmem copy of
         * the surface. Make sure we have it
         */
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL);
    } else if(palette9_changed(This)) {
        TRACE("Reloading surface because the d3d8/9 palette was changed\n");
        /* TODO: This is not necessarily needed with hw palettized texture support */
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL);
    } else {
        TRACE("surface is already in texture\n");
        return WINED3D_OK;
    }

    /* Resources are placed in system RAM and do not need to be recreated when a device is lost.
     *  These resources are not bound by device size or format restrictions. Because of this,
     *  these resources cannot be accessed by the Direct3D device nor set as textures or render targets.
     *  However, these resources can always be created, locked, and copied.
     */
    if (This->resource.pool == WINED3DPOOL_SCRATCH )
    {
        FIXME("(%p) Operation not supported for scratch textures\n",This);
        return WINED3DERR_INVALIDCALL;
    }

    This->srgb = srgb_mode;
    IWineD3DSurface_LoadLocation(iface, SFLAG_INTEXTURE, NULL /* no partial locking for textures yet */);

#if 0
    {
        static unsigned int gen = 0;
        char buffer[4096];
        ++gen;
        if ((gen % 10) == 0) {
            snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, This->glDescription.target, This->glDescription.level, gen);
            IWineD3DSurfaceImpl_SaveSnapshot(iface, buffer);
        }
        /*
         * debugging crash code
         if (gen == 250) {
         void** test = NULL;
         *test = 0;
         }
         */
    }
#endif

    if (!(This->Flags & SFLAG_DONOTFREE)) {
        HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
        This->resource.allocatedMemory = NULL;
        This->resource.heapMemory = NULL;
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, FALSE);
    }

    return WINED3D_OK;
}

#include <errno.h>
#include <stdio.h>
HRESULT WINAPI IWineD3DSurfaceImpl_SaveSnapshot(IWineD3DSurface *iface, const char* filename) {
    FILE* f = NULL;
    UINT i, y;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    char *allocatedMemory;
    char *textureRow;
    IWineD3DSwapChain *swapChain = NULL;
    int width, height;
    GLuint tmpTexture = 0;
    DWORD color;
    /*FIXME:
    Textures may not be stored in ->allocatedgMemory and a GlTexture
    so we should lock the surface before saving a snapshot, or at least check that
    */
    /* TODO: Compressed texture images can be obtained from the GL in uncompressed form
    by calling GetTexImage and in compressed form by calling
    GetCompressedTexImageARB.  Queried compressed images can be saved and
    later reused by calling CompressedTexImage[123]DARB.  Pre-compressed
    texture images do not need to be processed by the GL and should
    significantly improve texture loading performance relative to uncompressed
    images. */

/* Setup the width and height to be the internal texture width and height. */
    width  = This->pow2Width;
    height = This->pow2Height;
/* check to see if we're a 'virtual' texture, e.g. we're not a pbuffer of texture, we're a back buffer*/
    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapChain);

    if (This->Flags & SFLAG_INDRAWABLE && !(This->Flags & SFLAG_INTEXTURE)) {
        /* if were not a real texture then read the back buffer into a real texture */
        /* we don't want to interfere with the back buffer so read the data into a temporary
         * texture and then save the data out of the temporary texture
         */
        GLint prevRead;
        ENTER_GL();
        TRACE("(%p) Reading render target into texture\n", This);
        glEnable(GL_TEXTURE_2D);

        glGenTextures(1, &tmpTexture);
        glBindTexture(GL_TEXTURE_2D, tmpTexture);

        glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        width,
                        height,
                        0/*border*/,
                        GL_RGBA,
                        GL_UNSIGNED_INT_8_8_8_8_REV,
                        NULL);

        glGetIntegerv(GL_READ_BUFFER, &prevRead);
        vcheckGLcall("glGetIntegerv");
        glReadBuffer(swapChain ? GL_BACK : This->resource.wineD3DDevice->offscreenBuffer);
        vcheckGLcall("glReadBuffer");
        glCopyTexImage2D(GL_TEXTURE_2D,
                            0,
                            GL_RGBA,
                            0,
                            0,
                            width,
                            height,
                            0);

        checkGLcall("glCopyTexImage2D");
        glReadBuffer(prevRead);
        LEAVE_GL();

    } else { /* bind the real texture, and make sure it up to date */
        IWineD3DSurface_PreLoad(iface);
    }
    allocatedMemory = HeapAlloc(GetProcessHeap(), 0, width  * height * 4);
    ENTER_GL();
    FIXME("Saving texture level %d width %d height %d\n", This->glDescription.level, width, height);
    glGetTexImage(GL_TEXTURE_2D,
                This->glDescription.level,
                GL_RGBA,
                GL_UNSIGNED_INT_8_8_8_8_REV,
                allocatedMemory);
    checkGLcall("glTexImage2D");
    if (tmpTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &tmpTexture);
    }
    LEAVE_GL();

    f = fopen(filename, "w+");
    if (NULL == f) {
        ERR("opening of %s failed with: %s\n", filename, strerror(errno));
        return WINED3DERR_INVALIDCALL;
    }
/* Save the data out to a TGA file because 1: it's an easy raw format, 2: it supports an alpha channel */
    TRACE("(%p) opened %s with format %s\n", This, filename, debug_d3dformat(This->resource.format));
/* TGA header */
    fputc(0,f);
    fputc(0,f);
    fputc(2,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
/* short width*/
    fwrite(&width,2,1,f);
/* short height */
    fwrite(&height,2,1,f);
/* format rgba */
    fputc(0x20,f);
    fputc(0x28,f);
/* raw data */
    /* if the data is upside down if we've fetched it from a back buffer, so it needs flipping again to make it the correct way up */
    if(swapChain)
        textureRow = allocatedMemory + (width * (height - 1) *4);
    else
        textureRow = allocatedMemory;
    for (y = 0 ; y < height; y++) {
        for (i = 0; i < width;  i++) {
            color = *((DWORD*)textureRow);
            fputc((color >> 16) & 0xFF, f); /* B */
            fputc((color >>  8) & 0xFF, f); /* G */
            fputc((color >>  0) & 0xFF, f); /* R */
            fputc((color >> 24) & 0xFF, f); /* A */
            textureRow += 4;
        }
        /* take two rows of the pointer to the texture memory */
        if(swapChain)
            (textureRow-= width << 3);

    }
    TRACE("Closing file\n");
    fclose(f);

    if(swapChain) {
        IWineD3DSwapChain_Release(swapChain);
    }
    HeapFree(GetProcessHeap(), 0, allocatedMemory);
    return WINED3D_OK;
}

/**
 *   Slightly inefficient way to handle multiple dirty rects but it works :)
 */
extern HRESULT WINAPI IWineD3DSurfaceImpl_AddDirtyRect(IWineD3DSurface *iface, CONST RECT* pDirtyRect) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;

    if (!(This->Flags & SFLAG_INSYSMEM) && (This->Flags & SFLAG_INTEXTURE))
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL /* no partial locking for textures yet */);

    IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
    if (NULL != pDirtyRect) {
        This->dirtyRect.left   = min(This->dirtyRect.left,   pDirtyRect->left);
        This->dirtyRect.top    = min(This->dirtyRect.top,    pDirtyRect->top);
        This->dirtyRect.right  = max(This->dirtyRect.right,  pDirtyRect->right);
        This->dirtyRect.bottom = max(This->dirtyRect.bottom, pDirtyRect->bottom);
    } else {
        This->dirtyRect.left   = 0;
        This->dirtyRect.top    = 0;
        This->dirtyRect.right  = This->currentDesc.Width;
        This->dirtyRect.bottom = This->currentDesc.Height;
    }
    TRACE("(%p) : Dirty: yes, Rect:(%d,%d,%d,%d)\n", This, This->dirtyRect.left,
          This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);
    /* if the container is a basetexture then mark it dirty. */
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == WINED3D_OK) {
        TRACE("Passing to container\n");
        IWineD3DBaseTexture_SetDirty(baseTexture, TRUE);
        IWineD3DBaseTexture_Release(baseTexture);
    }
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    HRESULT hr;
    const GlPixelFormatDesc *glDesc;
    getFormatDescEntry(format, &GLINFO_LOCATION, &glDesc);

    TRACE("(%p) : Calling base function first\n", This);
    hr = IWineD3DBaseSurfaceImpl_SetFormat(iface, format);
    if(SUCCEEDED(hr)) {
        /* Setup some glformat defaults */
        This->glDescription.glFormat         = glDesc->glFormat;
        This->glDescription.glFormatInternal = glDesc->glInternal;
        This->glDescription.glType           = glDesc->glType;

        This->Flags &= ~SFLAG_ALLOCATED;
        TRACE("(%p) : glFormat %d, glFotmatInternal %d, glType %d\n", This,
              This->glDescription.glFormat, This->glDescription.glFormatInternal, This->glDescription.glType);
    }
    return hr;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetMem(IWineD3DSurface *iface, void *Mem) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    if(This->Flags & (SFLAG_LOCKED | SFLAG_DCINUSE)) {
        WARN("Surface is locked or the HDC is in use\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(Mem && Mem != This->resource.allocatedMemory) {
        void *release = NULL;

        /* Do I have to copy the old surface content? */
        if(This->Flags & SFLAG_DIBSECTION) {
                /* Release the DC. No need to hold the critical section for the update
                 * Thread because this thread runs only on front buffers, but this method
                 * fails for render targets in the check above.
                 */
                SelectObject(This->hDC, This->dib.holdbitmap);
                DeleteDC(This->hDC);
                /* Release the DIB section */
                DeleteObject(This->dib.DIBsection);
                This->dib.bitmap_data = NULL;
                This->resource.allocatedMemory = NULL;
                This->hDC = NULL;
                This->Flags &= ~SFLAG_DIBSECTION;
        } else if(!(This->Flags & SFLAG_USERPTR)) {
            release = This->resource.heapMemory;
            This->resource.heapMemory = NULL;
        }
        This->resource.allocatedMemory = Mem;
        This->Flags |= SFLAG_USERPTR | SFLAG_INSYSMEM;

        /* Now the surface memory is most up do date. Invalidate drawable and texture */
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);

        /* For client textures opengl has to be notified */
        if(This->Flags & SFLAG_CLIENT) {
            This->Flags &= ~SFLAG_ALLOCATED;
            IWineD3DSurface_PreLoad(iface);
            /* And hope that the app behaves correctly and did not free the old surface memory before setting a new pointer */
        }

        /* Now free the old memory if any */
        HeapFree(GetProcessHeap(), 0, release);
    } else if(This->Flags & SFLAG_USERPTR) {
        /* Lockrect and GetDC will re-create the dib section and allocated memory */
        This->resource.allocatedMemory = NULL;
        /* HeapMemory should be NULL already */
        if(This->resource.heapMemory != NULL) ERR("User pointer surface has heap memory allocated\n");
        This->Flags &= ~SFLAG_USERPTR;

        if(This->Flags & SFLAG_CLIENT) {
            This->Flags &= ~SFLAG_ALLOCATED;
            /* This respecifies an empty texture and opengl knows that the old memory is gone */
            IWineD3DSurface_PreLoad(iface);
        }
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Flip(IWineD3DSurface *iface, IWineD3DSurface *override, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSwapChainImpl *swapchain = NULL;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, override, Flags);

    /* Flipping is only supported on RenderTargets */
    if( !(This->resource.usage & WINED3DUSAGE_RENDERTARGET) ) return WINEDDERR_NOTFLIPPABLE;

    if(override) {
        /* DDraw sets this for the X11 surfaces, so don't confuse the user 
         * FIXME("(%p) Target override is not supported by now\n", This);
         * Additionally, it isn't really possible to support triple-buffering
         * properly on opengl at all
         */
    }

    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **) &swapchain);
    if(!swapchain) {
        ERR("Flipped surface is not on a swapchain\n");
        return WINEDDERR_NOTFLIPPABLE;
    }

    /* Just overwrite the swapchain presentation interval. This is ok because only ddraw apps can call Flip,
     * and only d3d8 and d3d9 apps specify the presentation interval
     */
    if((Flags & (WINEDDFLIP_NOVSYNC | WINEDDFLIP_INTERVAL2 | WINEDDFLIP_INTERVAL3 | WINEDDFLIP_INTERVAL4)) == 0) {
        /* Most common case first to avoid wasting time on all the other cases */
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_ONE;
    } else if(Flags & WINEDDFLIP_NOVSYNC) {
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_IMMEDIATE;
    } else if(Flags & WINEDDFLIP_INTERVAL2) {
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_TWO;
    } else if(Flags & WINEDDFLIP_INTERVAL3) {
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_THREE;
    } else {
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_FOUR;
    }

    /* Flipping a OpenGL surface -> Use WineD3DDevice::Present */
    hr = IWineD3DSwapChain_Present((IWineD3DSwapChain *) swapchain, NULL, NULL, 0, NULL, 0);
    IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    return hr;
}

/* Does a direct frame buffer -> texture copy. Stretching is done
 * with single pixel copy calls
 */
static inline void fb_copy_to_texture_direct(IWineD3DSurfaceImpl *This, IWineD3DSurface *SrcSurface, IWineD3DSwapChainImpl *swapchain, WINED3DRECT *srect, WINED3DRECT *drect, BOOL upsidedown, WINED3DTEXTUREFILTERTYPE Filter) {
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    float xrel, yrel;
    UINT row;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;


    ActivateContext(myDevice, SrcSurface, CTXUSAGE_BLIT);
    ENTER_GL();
    IWineD3DSurface_PreLoad((IWineD3DSurface *) This);

    /* Bind the target texture */
    glBindTexture(GL_TEXTURE_2D, This->glDescription.textureName);
    checkGLcall("glBindTexture");
    if(!swapchain) {
        glReadBuffer(myDevice->offscreenBuffer);
    } else {
        GLenum buffer = surface_get_gl_buffer(SrcSurface, (IWineD3DSwapChain *)swapchain);
        glReadBuffer(buffer);
    }
    checkGLcall("glReadBuffer");

    xrel = (float) (srect->x2 - srect->x1) / (float) (drect->x2 - drect->x1);
    yrel = (float) (srect->y2 - srect->y1) / (float) (drect->y2 - drect->y1);

    if( (xrel - 1.0 < -eps) || (xrel - 1.0 > eps)) {
        FIXME("Doing a pixel by pixel copy from the framebuffer to a texture, expect major performance issues\n");

        if(Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT) {
            ERR("Texture filtering not supported in direct blit\n");
        }
    } else if((Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT) && ((yrel - 1.0 < -eps) || (yrel - 1.0 > eps))) {
        ERR("Texture filtering not supported in direct blit\n");
    }

    if(upsidedown &&
       !((xrel - 1.0 < -eps) || (xrel - 1.0 > eps)) &&
       !((yrel - 1.0 < -eps) || (yrel - 1.0 > eps))) {
        /* Upside down copy without stretching is nice, one glCopyTexSubImage call will do */

        glCopyTexSubImage2D(This->glDescription.target,
                            This->glDescription.level,
                            drect->x1, drect->y1, /* xoffset, yoffset */
                            srect->x1, Src->currentDesc.Height - srect->y2,
                            drect->x2 - drect->x1, drect->y2 - drect->y1);
    } else {
        UINT yoffset = Src->currentDesc.Height - srect->y1 + drect->y1 - 1;
        /* I have to process this row by row to swap the image,
         * otherwise it would be upside down, so stretching in y direction
         * doesn't cost extra time
         *
         * However, stretching in x direction can be avoided if not necessary
         */
        for(row = drect->y1; row < drect->y2; row++) {
            if( (xrel - 1.0 < -eps) || (xrel - 1.0 > eps)) {
                /* Well, that stuff works, but it's very slow.
                 * find a better way instead
                 */
                UINT col;

                for(col = drect->x1; col < drect->x2; col++) {
                    glCopyTexSubImage2D(This->glDescription.target,
                                        This->glDescription.level,
                                        drect->x1 + col, row, /* xoffset, yoffset */
                                        srect->x1 + col * xrel, yoffset - (int) (row * yrel),
                                        1, 1);
                }
            } else {
                glCopyTexSubImage2D(This->glDescription.target,
                                    This->glDescription.level,
                                    drect->x1, row, /* xoffset, yoffset */
                                    srect->x1, yoffset - (int) (row * yrel),
                                    drect->x2-drect->x1, 1);
            }
        }
    }

    vcheckGLcall("glCopyTexSubImage2D");
    LEAVE_GL();
}

/* Uses the hardware to stretch and flip the image */
static inline void fb_copy_to_texture_hwstretch(IWineD3DSurfaceImpl *This, IWineD3DSurface *SrcSurface, IWineD3DSwapChainImpl *swapchain, WINED3DRECT *srect, WINED3DRECT *drect, BOOL upsidedown, WINED3DTEXTUREFILTERTYPE Filter) {
    GLuint src, backup = 0;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    float left, right, top, bottom; /* Texture coordinates */
    UINT fbwidth = Src->currentDesc.Width;
    UINT fbheight = Src->currentDesc.Height;
    GLenum drawBuffer = GL_BACK;

    TRACE("Using hwstretch blit\n");
    /* Activate the Proper context for reading from the source surface, set it up for blitting */
    ActivateContext(myDevice, SrcSurface, CTXUSAGE_BLIT);
    ENTER_GL();
    IWineD3DSurface_PreLoad((IWineD3DSurface *) This);

    /* Try to use an aux buffer for drawing the rectangle. This way it doesn't need restoring.
     * This way we don't have to wait for the 2nd readback to finish to leave this function.
     */
    if(GL_LIMITS(aux_buffers) >= 2) {
        /* Got more than one aux buffer? Use the 2nd aux buffer */
        drawBuffer = GL_AUX1;
    } else if((swapchain || myDevice->offscreenBuffer == GL_BACK) && GL_LIMITS(aux_buffers) >= 1) {
        /* Only one aux buffer, but it isn't used (Onscreen rendering, or non-aux orm)? Use it! */
        drawBuffer = GL_AUX0;
    }

    if(!swapchain && wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        glGenTextures(1, &backup);
        checkGLcall("glGenTextures\n");
        glBindTexture(GL_TEXTURE_2D, backup);
        checkGLcall("glBindTexture(Src->glDescription.target, Src->glDescription.textureName)");
    } else {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        if(Src->glDescription.textureName == 0) {
            /* Get it a description */
            IWineD3DSurface_PreLoad(SrcSurface);
        }
        glBindTexture(GL_TEXTURE_2D, Src->glDescription.textureName);
        checkGLcall("glBindTexture(Src->glDescription.target, Src->glDescription.textureName)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        Src->Flags &= ~SFLAG_INTEXTURE;
    }

    glReadBuffer(GL_BACK);
    checkGLcall("glReadBuffer(GL_BACK)");

    /* TODO: Only back up the part that will be overwritten */
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0 /* read offsets */,
                        0, 0,
                        fbwidth,
                        fbheight);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    stateLookup[WINELOOKUP_MAGFILTER][Filter - minLookup[WINELOOKUP_MAGFILTER]]);
    checkGLcall("glTexParameteri");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    minMipLookup[Filter][WINED3DTEXF_NONE]);
    checkGLcall("glTexParameteri");

    if(!swapchain || (IWineD3DSurface *) Src == swapchain->backBuffer[0]) {
        src = backup ? backup : Src->glDescription.textureName;
    } else {
        glReadBuffer(GL_FRONT);
        checkGLcall("glReadBuffer(GL_FRONT)");

        glGenTextures(1, &src);
        checkGLcall("glGenTextures(1, &src)");
        glBindTexture(GL_TEXTURE_2D, src);
        checkGLcall("glBindTexture(GL_TEXTURE_2D, src)");

        /* TODO: Only copy the part that will be read. Use srect->x1, srect->y2 as origin, but with the width watch
         * out for power of 2 sizes
         */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Src->pow2Width, Src->pow2Height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLcall("glTexImage2D");
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                            0, 0 /* read offsets */,
                            0, 0,
                            fbwidth,
                            fbheight);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        glReadBuffer(GL_BACK);
        checkGLcall("glReadBuffer(GL_BACK)");
    }
    checkGLcall("glEnd and previous");

    left = (float) srect->x1 / (float) Src->pow2Width;
    right = (float) srect->x2 / (float) Src->pow2Width;

    if(upsidedown) {
        top = (float) (Src->currentDesc.Height - srect->y1) / (float) Src->pow2Height;
        bottom = (float) (Src->currentDesc.Height - srect->y2) / (float) Src->pow2Height;
    } else {
        top = (float) (Src->currentDesc.Height - srect->y2) / (float) Src->pow2Height;
        bottom = (float) (Src->currentDesc.Height - srect->y1) / (float) Src->pow2Height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glDrawBuffer(drawBuffer);
    glReadBuffer(drawBuffer);

    glBegin(GL_QUADS);
        /* bottom left */
        glTexCoord2f(left, bottom);
        glVertex2i(0, fbheight);

        /* top left */
        glTexCoord2f(left, top);
        glVertex2i(0, fbheight - drect->y2 - drect->y1);

        /* top right */
        glTexCoord2f(right, top);
        glVertex2i(drect->x2 - drect->x1, fbheight - drect->y2 - drect->y1);

        /* bottom right */
        glTexCoord2f(right, bottom);
        glVertex2i(drect->x2 - drect->x1, fbheight);
    glEnd();
    checkGLcall("glEnd and previous");

    /* Now read the stretched and upside down image into the destination texture */
    glBindTexture(This->glDescription.target, This->glDescription.textureName);
    checkGLcall("glBindTexture");
    glCopyTexSubImage2D(This->glDescription.target,
                        0,
                        drect->x1, drect->y1, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        drect->x2 - drect->x1, drect->y2 - drect->y1);
    checkGLcall("glCopyTexSubImage2D");

    /* Write the back buffer backup back */
    glBindTexture(GL_TEXTURE_2D, backup ? backup : Src->glDescription.textureName);
    checkGLcall("glBindTexture(GL_TEXTURE_2D, Src->glDescription.textureName)");

    if(drawBuffer == GL_BACK) {
        glBegin(GL_QUADS);
            /* top left */
            glTexCoord2f(0.0, (float) fbheight / (float) Src->pow2Height);
            glVertex2i(0, 0);

            /* bottom left */
            glTexCoord2f(0.0, 0.0);
            glVertex2i(0, fbheight);

            /* bottom right */
            glTexCoord2f((float) fbwidth / (float) Src->pow2Width, 0.0);
            glVertex2i(fbwidth, Src->currentDesc.Height);

            /* top right */
            glTexCoord2f((float) fbwidth / (float) Src->pow2Width, (float) fbheight / (float) Src->pow2Height);
            glVertex2i(fbwidth, 0);
        glEnd();
    } else {
        /* Restore the old draw buffer */
        glDrawBuffer(GL_BACK);
    }

    /* Cleanup */
    if(src != Src->glDescription.textureName && src != backup) {
        glDeleteTextures(1, &src);
        checkGLcall("glDeleteTextures(1, &src)");
    }
    if(backup) {
        glDeleteTextures(1, &backup);
        checkGLcall("glDeleteTextures(1, &backup)");
    }
    LEAVE_GL();
}

/* Not called from the VTable */
static HRESULT IWineD3DSurfaceImpl_BltOverride(IWineD3DSurfaceImpl *This, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter) {
    WINED3DRECT rect;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *srcSwapchain = NULL, *dstSwapchain = NULL;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;

    TRACE("(%p)->(%p,%p,%p,%08x,%p)\n", This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);

    /* Get the swapchain. One of the surfaces has to be a primary surface */
    if(This->resource.pool == WINED3DPOOL_SYSTEMMEM) {
        WARN("Destination is in sysmem, rejecting gl blt\n");
        return WINED3DERR_INVALIDCALL;
    }
    IWineD3DSurface_GetContainer( (IWineD3DSurface *) This, &IID_IWineD3DSwapChain, (void **)&dstSwapchain);
    if(dstSwapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) dstSwapchain);
    if(Src) {
        if(Src->resource.pool == WINED3DPOOL_SYSTEMMEM) {
            WARN("Src is in sysmem, rejecting gl blt\n");
            return WINED3DERR_INVALIDCALL;
        }
        IWineD3DSurface_GetContainer( (IWineD3DSurface *) Src, &IID_IWineD3DSwapChain, (void **)&srcSwapchain);
        if(srcSwapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) srcSwapchain);
    }

    /* Early sort out of cases where no render target is used */
    if(!dstSwapchain && !srcSwapchain &&
        SrcSurface != myDevice->render_targets[0] && This != (IWineD3DSurfaceImpl *) myDevice->render_targets[0]) {
        TRACE("No surface is render target, not using hardware blit. Src = %p, dst = %p\n", Src, This);
        return WINED3DERR_INVALIDCALL;
    }

    /* No destination color keying supported */
    if(Flags & (WINEDDBLT_KEYDEST | WINEDDBLT_KEYDESTOVERRIDE)) {
        /* Can we support that with glBlendFunc if blitting to the frame buffer? */
        TRACE("Destination color key not supported in accelerated Blit, falling back to software\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (DestRect) {
        rect.x1 = DestRect->left;
        rect.y1 = DestRect->top;
        rect.x2 = DestRect->right;
        rect.y2 = DestRect->bottom;
    } else {
        rect.x1 = 0;
        rect.y1 = 0;
        rect.x2 = This->currentDesc.Width;
        rect.y2 = This->currentDesc.Height;
    }

    /* The only case where both surfaces on a swapchain are supported is a back buffer -> front buffer blit on the same swapchain */
    if(dstSwapchain && dstSwapchain == srcSwapchain && dstSwapchain->backBuffer &&
       ((IWineD3DSurface *) This == dstSwapchain->frontBuffer) && SrcSurface == dstSwapchain->backBuffer[0]) {
        /* Half-life does a Blt from the back buffer to the front buffer,
         * Full surface size, no flags... Use present instead
         *
         * This path will only be entered for d3d7 and ddraw apps, because d3d8/9 offer no way to blit TO the front buffer
         */

        /* Check rects - IWineD3DDevice_Present doesn't handle them */
        while(1)
        {
            RECT mySrcRect;
            TRACE("Looking if a Present can be done...\n");
            /* Source Rectangle must be full surface */
            if( SrcRect ) {
                if(SrcRect->left != 0 || SrcRect->top != 0 ||
                   SrcRect->right != Src->currentDesc.Width || SrcRect->bottom != Src->currentDesc.Height) {
                    TRACE("No, Source rectangle doesn't match\n");
                    break;
                }
            }
            mySrcRect.left = 0;
            mySrcRect.top = 0;
            mySrcRect.right = Src->currentDesc.Width;
            mySrcRect.bottom = Src->currentDesc.Height;

            /* No stretching may occur */
            if(mySrcRect.right != rect.x2 - rect.x1 ||
               mySrcRect.bottom != rect.y2 - rect.y1) {
                TRACE("No, stretching is done\n");
                break;
            }

            /* Destination must be full surface or match the clipping rectangle */
            if(This->clipper && ((IWineD3DClipperImpl *) This->clipper)->hWnd)
            {
                RECT cliprect;
                POINT pos[2];
                GetClientRect(((IWineD3DClipperImpl *) This->clipper)->hWnd, &cliprect);
                pos[0].x = rect.x1;
                pos[0].y = rect.y1;
                pos[1].x = rect.x2;
                pos[1].y = rect.y2;
                MapWindowPoints(GetDesktopWindow(), ((IWineD3DClipperImpl *) This->clipper)->hWnd,
                                pos, 2);

                if(pos[0].x != cliprect.left  || pos[0].y != cliprect.top   ||
                   pos[1].x != cliprect.right || pos[1].y != cliprect.bottom)
                {
                    TRACE("No, dest rectangle doesn't match(clipper)\n");
                    TRACE("Clip rect at (%d,%d)-(%d,%d)\n", cliprect.left, cliprect.top, cliprect.right, cliprect.bottom);
                    TRACE("Blt dest: (%d,%d)-(%d,%d)\n", rect.x1, rect.y1, rect.x2, rect.y2);
                    break;
                }
            }
            else
            {
                if(rect.x1 != 0 || rect.y1 != 0 ||
                   rect.x2 != This->currentDesc.Width || rect.y2 != This->currentDesc.Height) {
                    TRACE("No, dest rectangle doesn't match(surface size)\n");
                    break;
                }
            }

            TRACE("Yes\n");

            /* These flags are unimportant for the flag check, remove them */
            if((Flags & ~(WINEDDBLT_DONOTWAIT | WINEDDBLT_WAIT)) == 0) {
                WINED3DSWAPEFFECT orig_swap = dstSwapchain->presentParms.SwapEffect;

                /* The idea behind this is that a glReadPixels and a glDrawPixels call
                    * take very long, while a flip is fast.
                    * This applies to Half-Life, which does such Blts every time it finished
                    * a frame, and to Prince of Persia 3D, which uses this to draw at least the main
                    * menu. This is also used by all apps when they do windowed rendering
                    *
                    * The problem is that flipping is not really the same as copying. After a
                    * Blt the front buffer is a copy of the back buffer, and the back buffer is
                    * untouched. Therefore it's necessary to override the swap effect
                    * and to set it back after the flip.
                    *
                    * Windowed Direct3D < 7 apps do the same. The D3D7 sdk demos are nice
                    * testcases.
                    */

                dstSwapchain->presentParms.SwapEffect = WINED3DSWAPEFFECT_COPY;
                dstSwapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_IMMEDIATE;

                TRACE("Full screen back buffer -> front buffer blt, performing a flip instead\n");
                IWineD3DSwapChain_Present((IWineD3DSwapChain *) dstSwapchain, NULL, NULL, 0, NULL, 0);

                dstSwapchain->presentParms.SwapEffect = orig_swap;

                return WINED3D_OK;
            }
            break;
        }

        TRACE("Unsupported blit between buffers on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    } else if((dstSwapchain || This == (IWineD3DSurfaceImpl *) myDevice->render_targets[0]) &&
              (srcSwapchain || SrcSurface == myDevice->render_targets[0]) ) {
        ERR("Can't perform hardware blit between 2 different swapchains, falling back to software\n");
        return WINED3DERR_INVALIDCALL;
    }

    if(srcSwapchain || SrcSurface == myDevice->render_targets[0]) {
        /* Blit from render target to texture */
        WINED3DRECT srect;
        BOOL upsideDown, stretchx;

        if(Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE)) {
            TRACE("Color keying not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
            /* Destination color key is checked above */
        }

        /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
         * glCopyTexSubImage is a bit picky about the parameters we pass to it
         */
        if(SrcRect) {
            if(SrcRect->top < SrcRect->bottom) {
                srect.y1 = SrcRect->top;
                srect.y2 = SrcRect->bottom;
                upsideDown = FALSE;
            } else {
                srect.y1 = SrcRect->bottom;
                srect.y2 = SrcRect->top;
                upsideDown = TRUE;
            }
            srect.x1 = SrcRect->left;
            srect.x2 = SrcRect->right;
        } else {
            srect.x1 = 0;
            srect.y1 = 0;
            srect.x2 = Src->currentDesc.Width;
            srect.y2 = Src->currentDesc.Height;
            upsideDown = FALSE;
        }
        if(rect.x1 > rect.x2) {
            UINT tmp = rect.x2;
            rect.x2 = rect.x1;
            rect.x1 = tmp;
            upsideDown = !upsideDown;
        }
        if(!srcSwapchain) {
            TRACE("Reading from an offscreen target\n");
            upsideDown = !upsideDown;
        }

        if(rect.x2 - rect.x1 != srect.x2 - srect.x1) {
            stretchx = TRUE;
        } else {
            stretchx = FALSE;
        }

        /* Blt is a pretty powerful call, while glCopyTexSubImage2D is not. glCopyTexSubImage cannot
         * flip the image nor scale it.
         *
         * -> If the app asks for a unscaled, upside down copy, just perform one glCopyTexSubImage2D call
         * -> If the app wants a image width an unscaled width, copy it line per line
         * -> If the app wants a image that is scaled on the x axis, and the destination rectangle is smaller
         *    than the frame buffer, draw an upside down scaled image onto the fb, read it back and restore the
         *    back buffer. This is slower than reading line per line, thus not used for flipping
         * -> If the app wants a scaled image with a dest rect that is bigger than the fb, it has to be copied
         *    pixel by pixel
         *
         * If EXT_framebuffer_blit is supported that can be used instead. Note that EXT_framebuffer_blit implies
         * FBO support, so it doesn't really make sense to try and make it work with different offscreen rendering
         * backends.
         */
        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && GL_SUPPORT(EXT_FRAMEBUFFER_BLIT)) {
            stretch_rect_fbo((IWineD3DDevice *)myDevice, SrcSurface, &srect,
                    (IWineD3DSurface *)This, &rect, Filter, upsideDown);
        } else if((!stretchx) || rect.x2 - rect.x1 > Src->currentDesc.Width ||
                                    rect.y2 - rect.y1 > Src->currentDesc.Height) {
            TRACE("No stretching in x direction, using direct framebuffer -> texture copy\n");
            fb_copy_to_texture_direct(This, SrcSurface, srcSwapchain, &srect, &rect, upsideDown, Filter);
        } else {
            TRACE("Using hardware stretching to flip / stretch the texture\n");
            fb_copy_to_texture_hwstretch(This, SrcSurface, srcSwapchain, &srect, &rect, upsideDown, Filter);
        }

        if(!(This->Flags & SFLAG_DONOTFREE)) {
            HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
            This->resource.allocatedMemory = NULL;
            This->resource.heapMemory = NULL;
        } else {
            This->Flags &= ~SFLAG_INSYSMEM;
        }
        /* The texture is now most up to date - If the surface is a render target and has a drawable, this
         * path is never entered
         */
        IWineD3DSurface_ModifyLocation((IWineD3DSurface *) This, SFLAG_INTEXTURE, TRUE);

        return WINED3D_OK;
    } else if(Src) {
        /* Blit from offscreen surface to render target */
        float glTexCoord[4];
        DWORD oldCKeyFlags = Src->CKeyFlags;
        WINEDDCOLORKEY oldBltCKey = This->SrcBltCKey;
        RECT SourceRectangle;

        TRACE("Blt from surface %p to rendertarget %p\n", Src, This);

        if(SrcRect) {
            SourceRectangle.left = SrcRect->left;
            SourceRectangle.right = SrcRect->right;
            SourceRectangle.top = SrcRect->top;
            SourceRectangle.bottom = SrcRect->bottom;
        } else {
            SourceRectangle.left = 0;
            SourceRectangle.right = Src->currentDesc.Width;
            SourceRectangle.top = 0;
            SourceRectangle.bottom = Src->currentDesc.Height;
        }

        if(!CalculateTexRect(Src, &SourceRectangle, glTexCoord)) {
            /* Fall back to software */
            WARN("(%p) Source texture area (%d,%d)-(%d,%d) is too big\n", Src,
                    SourceRectangle.left, SourceRectangle.top,
                    SourceRectangle.right, SourceRectangle.bottom);
            return WINED3DERR_INVALIDCALL;
        }

        /* Color keying: Check if we have to do a color keyed blt,
         * and if not check if a color key is activated.
         *
         * Just modify the color keying parameters in the surface and restore them afterwards
         * The surface keeps track of the color key last used to load the opengl surface.
         * PreLoad will catch the change to the flags and color key and reload if necessary.
         */
        if(Flags & WINEDDBLT_KEYSRC) {
            /* Use color key from surface */
        } else if(Flags & WINEDDBLT_KEYSRCOVERRIDE) {
            /* Use color key from DDBltFx */
            Src->CKeyFlags |= WINEDDSD_CKSRCBLT;
            This->SrcBltCKey = DDBltFx->ddckSrcColorkey;
        } else {
            /* Do not use color key */
            Src->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
        }

        /* Now load the surface */
        IWineD3DSurface_PreLoad((IWineD3DSurface *) Src);


        /* Activate the destination context, set it up for blitting */
        ActivateContext(myDevice, (IWineD3DSurface *) This, CTXUSAGE_BLIT);
        ENTER_GL();

        if(!dstSwapchain) {
            TRACE("Drawing to offscreen buffer\n");
            glDrawBuffer(myDevice->offscreenBuffer);
            checkGLcall("glDrawBuffer");
        } else {
            GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *)This, (IWineD3DSwapChain *)dstSwapchain);
            TRACE("Drawing to %#x buffer\n", buffer);
            glDrawBuffer(buffer);
            checkGLcall("glDrawBuffer");
        }

        /* Bind the texture */
        glBindTexture(GL_TEXTURE_2D, Src->glDescription.textureName);
        checkGLcall("glBindTexture");

        /* Filtering for StretchRect */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        stateLookup[WINELOOKUP_MAGFILTER][Filter - minLookup[WINELOOKUP_MAGFILTER]]);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        minMipLookup[Filter][WINED3DTEXF_NONE]);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        checkGLcall("glTexEnvi");

        /* This is for color keying */
        if(Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE)) {
            glEnable(GL_ALPHA_TEST);
            checkGLcall("glEnable GL_ALPHA_TEST");
            glAlphaFunc(GL_NOTEQUAL, 0.0);
            checkGLcall("glAlphaFunc\n");
        } else {
            glDisable(GL_ALPHA_TEST);
            checkGLcall("glDisable GL_ALPHA_TEST");
        }

        /* Draw a textured quad
         */
        glBegin(GL_QUADS);

        glColor3d(1.0f, 1.0f, 1.0f);
        glTexCoord2f(glTexCoord[0], glTexCoord[2]);
        glVertex3f(rect.x1,
                    rect.y1,
                    0.0);

        glTexCoord2f(glTexCoord[0], glTexCoord[3]);
        glVertex3f(rect.x1, rect.y2, 0.0);

        glTexCoord2f(glTexCoord[1], glTexCoord[3]);
        glVertex3f(rect.x2,
                    rect.y2,
                    0.0);

        glTexCoord2f(glTexCoord[1], glTexCoord[2]);
        glVertex3f(rect.x2,
                    rect.y1,
                    0.0);
        glEnd();
        checkGLcall("glEnd");

        if(Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE)) {
            glDisable(GL_ALPHA_TEST);
            checkGLcall("glDisable(GL_ALPHA_TEST)");
        }

        /* Unbind the texture */
        glBindTexture(GL_TEXTURE_2D, 0);
        checkGLcall("glEnable glBindTexture");

        /* The draw buffer should only need to be restored if we were drawing to the front buffer, and there is a back buffer.
         * otherwise the context manager should choose between GL_BACK / offscreenDrawBuffer
         */
        if(dstSwapchain && This == (IWineD3DSurfaceImpl *) dstSwapchain->frontBuffer && dstSwapchain->backBuffer) {
            glDrawBuffer(GL_BACK);
            checkGLcall("glDrawBuffer");
        }
        /* Restore the color key parameters */
        Src->CKeyFlags = oldCKeyFlags;
        This->SrcBltCKey = oldBltCKey;

        LEAVE_GL();

        /* TODO: If the surface is locked often, perform the Blt in software on the memory instead */
        /* The surface is now in the drawable. On onscreen surfaces or without fbos the texture
         * is outdated now
         */
        IWineD3DSurface_ModifyLocation((IWineD3DSurface *) This, SFLAG_INDRAWABLE, TRUE);
        /* TODO: This should be moved to ModifyLocation() */
        if(!(dstSwapchain || wined3d_settings.offscreen_rendering_mode != ORM_FBO)) {
            This->Flags |= SFLAG_INTEXTURE;
        }

        return WINED3D_OK;
    } else {
        /* Source-Less Blit to render target */
        if (Flags & WINEDDBLT_COLORFILL) {
            /* This is easy to handle for the D3D Device... */
            DWORD color;

            TRACE("Colorfill\n");

            /* The color as given in the Blt function is in the format of the frame-buffer...
             * 'clear' expect it in ARGB format => we need to do some conversion :-)
             */
            if (This->resource.format == WINED3DFMT_P8) {
                if (This->palette) {
                    color = ((0xFF000000) |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peRed << 16) |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peGreen << 8) |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peBlue));
                } else {
                    color = 0xFF000000;
                }
            }
            else if (This->resource.format == WINED3DFMT_R5G6B5) {
                if (DDBltFx->u5.dwFillColor == 0xFFFF) {
                    color = 0xFFFFFFFF;
                } else {
                    color = ((0xFF000000) |
                            ((DDBltFx->u5.dwFillColor & 0xF800) << 8) |
                            ((DDBltFx->u5.dwFillColor & 0x07E0) << 5) |
                            ((DDBltFx->u5.dwFillColor & 0x001F) << 3));
                }
            }
            else if ((This->resource.format == WINED3DFMT_R8G8B8) ||
                    (This->resource.format == WINED3DFMT_X8R8G8B8) ) {
                color = 0xFF000000 | DDBltFx->u5.dwFillColor;
            }
            else if (This->resource.format == WINED3DFMT_A8R8G8B8) {
                color = DDBltFx->u5.dwFillColor;
            }
            else {
                ERR("Wrong surface type for BLT override(Format doesn't match) !\n");
                return WINED3DERR_INVALIDCALL;
            }

            TRACE("Calling GetSwapChain with mydevice = %p\n", myDevice);
            if(dstSwapchain && dstSwapchain->backBuffer && This == (IWineD3DSurfaceImpl*) dstSwapchain->backBuffer[0]) {
                glDrawBuffer(GL_BACK);
                checkGLcall("glDrawBuffer(GL_BACK)");
            } else if (dstSwapchain && This == (IWineD3DSurfaceImpl*) dstSwapchain->frontBuffer) {
                glDrawBuffer(GL_FRONT);
                checkGLcall("glDrawBuffer(GL_FRONT)");
            } else if(This == (IWineD3DSurfaceImpl *) myDevice->render_targets[0]) {
                glDrawBuffer(myDevice->offscreenBuffer);
                checkGLcall("glDrawBuffer(myDevice->offscreenBuffer3)");
            } else {
                TRACE("Surface is higher back buffer, falling back to software\n");
                return WINED3DERR_INVALIDCALL;
            }

            TRACE("(%p) executing Render Target override, color = %x\n", This, color);

            IWineD3DDevice_Clear( (IWineD3DDevice *) myDevice,
                                1 /* Number of rectangles */,
                                &rect,
                                WINED3DCLEAR_TARGET,
                                color,
                                0.0 /* Z */,
                                0 /* Stencil */);

            /* Restore the original draw buffer */
            if(!dstSwapchain) {
                glDrawBuffer(myDevice->offscreenBuffer);
            } else if(dstSwapchain->backBuffer && dstSwapchain->backBuffer[0]) {
                glDrawBuffer(GL_BACK);
            }
            vcheckGLcall("glDrawBuffer");

            return WINED3D_OK;
        }
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_BltZ(IWineD3DSurfaceImpl *This, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, WINEDDBLTFX *DDBltFx)
{
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    float depth;

    if (Flags & WINEDDBLT_DEPTHFILL) {
        switch(This->resource.format) {
            case WINED3DFMT_D16:
                depth = (float) DDBltFx->u5.dwFillDepth / (float) 0x0000ffff;
                break;
            case WINED3DFMT_D15S1:
                depth = (float) DDBltFx->u5.dwFillDepth / (float) 0x0000fffe;
                break;
            case WINED3DFMT_D24S8:
            case WINED3DFMT_D24X8:
                depth = (float) DDBltFx->u5.dwFillDepth / (float) 0x00ffffff;
                break;
            case WINED3DFMT_D32:
                depth = (float) DDBltFx->u5.dwFillDepth / (float) 0xffffffff;
                break;
            default:
                depth = 0.0;
                ERR("Unexpected format for depth fill: %s\n", debug_d3dformat(This->resource.format));
        }

        return IWineD3DDevice_Clear((IWineD3DDevice *) myDevice,
                                    DestRect == NULL ? 0 : 1,
                                    (WINED3DRECT *) DestRect,
                                    WINED3DCLEAR_ZBUFFER,
                                    0x00000000,
                                    depth,
                                    0x00000000);
    }

    FIXME("(%p): Unsupp depthstencil blit\n", This);
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Blt(IWineD3DSurface *iface, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    TRACE("(%p)->(%p,%p,%p,%x,%p)\n", This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);
    TRACE("(%p): Usage is %s\n", This, debug_d3dusage(This->resource.usage));

    /* Accessing the depth stencil is supposed to fail between a BeginScene and EndScene pair,
     * except depth blits, which seem to work
     */
    if(iface == myDevice->stencilBufferTarget || (SrcSurface && SrcSurface == myDevice->stencilBufferTarget)) {
        if(myDevice->inScene && !(Flags & WINEDDBLT_DEPTHFILL)) {
            TRACE("Attempt to access the depth stencil surface in a BeginScene / EndScene pair, returning WINED3DERR_INVALIDCALL\n");
            return WINED3DERR_INVALIDCALL;
        } else if(IWineD3DSurfaceImpl_BltZ(This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx) == WINED3D_OK) {
            TRACE("Z Blit override handled the blit\n");
            return WINED3D_OK;
        }
    }

    /* Special cases for RenderTargets */
    if( (This->resource.usage & WINED3DUSAGE_RENDERTARGET) ||
        ( Src && (Src->resource.usage & WINED3DUSAGE_RENDERTARGET) )) {
        if(IWineD3DSurfaceImpl_BltOverride(This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx, Filter) == WINED3D_OK) return WINED3D_OK;
    }

    /* For the rest call the X11 surface implementation.
     * For RenderTargets this should be implemented OpenGL accelerated in BltOverride,
     * other Blts are rather rare
     */
    return IWineD3DBaseSurfaceImpl_Blt(iface, DestRect, SrcSurface, SrcRect, Flags, DDBltFx, Filter);
}

HRESULT WINAPI IWineD3DSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty, IWineD3DSurface *Source, RECT *rsrc, DWORD trans) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *srcImpl = (IWineD3DSurfaceImpl *) Source;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    TRACE("(%p)->(%d, %d, %p, %p, %08x\n", iface, dstx, dsty, Source, rsrc, trans);

    if(myDevice->inScene &&
       (iface == myDevice->stencilBufferTarget ||
       (Source && Source == myDevice->stencilBufferTarget))) {
        TRACE("Attempt to access the depth stencil surface in a BeginScene / EndScene pair, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Special cases for RenderTargets */
    if( (This->resource.usage & WINED3DUSAGE_RENDERTARGET) ||
        ( srcImpl && (srcImpl->resource.usage & WINED3DUSAGE_RENDERTARGET) )) {

        RECT SrcRect, DstRect;
        DWORD Flags=0;

        if(rsrc) {
            SrcRect.left = rsrc->left;
            SrcRect.top= rsrc->top;
            SrcRect.bottom = rsrc->bottom;
            SrcRect.right = rsrc->right;
        } else {
            SrcRect.left = 0;
            SrcRect.top = 0;
            SrcRect.right = srcImpl->currentDesc.Width;
            SrcRect.bottom = srcImpl->currentDesc.Height;
        }

        DstRect.left = dstx;
        DstRect.top=dsty;
        DstRect.right = dstx + SrcRect.right - SrcRect.left;
        DstRect.bottom = dsty + SrcRect.bottom - SrcRect.top;

        /* Convert BltFast flags into Btl ones because it is called from SurfaceImpl_Blt as well */
        if(trans & WINEDDBLTFAST_SRCCOLORKEY)
            Flags |= WINEDDBLT_KEYSRC;
        if(trans & WINEDDBLTFAST_DESTCOLORKEY)
            Flags |= WINEDDBLT_KEYDEST;
        if(trans & WINEDDBLTFAST_WAIT)
            Flags |= WINEDDBLT_WAIT;
        if(trans & WINEDDBLTFAST_DONOTWAIT)
            Flags |= WINEDDBLT_DONOTWAIT;

        if(IWineD3DSurfaceImpl_BltOverride(This, &DstRect, Source, &SrcRect, Flags, NULL, WINED3DTEXF_POINT) == WINED3D_OK) return WINED3D_OK;
    }


    return IWineD3DBaseSurfaceImpl_BltFast(iface, dstx, dsty, Source, rsrc, trans);
}

static HRESULT WINAPI IWineD3DSurfaceImpl_PrivateSetup(IWineD3DSurface *iface) {
    /** Check against the maximum texture sizes supported by the video card **/
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    unsigned int pow2Width, pow2Height;
    const GlPixelFormatDesc *glDesc;

    getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);
    /* Setup some glformat defaults */
    This->glDescription.glFormat         = glDesc->glFormat;
    This->glDescription.glFormatInternal = glDesc->glInternal;
    This->glDescription.glType           = glDesc->glType;

    This->glDescription.textureName      = 0;
    This->glDescription.target           = GL_TEXTURE_2D;

    /* Non-power2 support */
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO)) {
        pow2Width = This->currentDesc.Width;
        pow2Height = This->currentDesc.Height;
    } else {
        /* Find the nearest pow2 match */
        pow2Width = pow2Height = 1;
        while (pow2Width < This->currentDesc.Width) pow2Width <<= 1;
        while (pow2Height < This->currentDesc.Height) pow2Height <<= 1;
    }
    This->pow2Width  = pow2Width;
    This->pow2Height = pow2Height;

    if (pow2Width > This->currentDesc.Width || pow2Height > This->currentDesc.Height) {
        WINED3DFORMAT Format = This->resource.format;
        /** TODO: add support for non power two compressed textures **/
        if (Format == WINED3DFMT_DXT1 || Format == WINED3DFMT_DXT2 || Format == WINED3DFMT_DXT3
            || Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5) {
            FIXME("(%p) Compressed non-power-two textures are not supported w(%d) h(%d)\n",
                  This, This->currentDesc.Width, This->currentDesc.Height);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if(pow2Width != This->currentDesc.Width ||
       pow2Height != This->currentDesc.Height) {
        This->Flags |= SFLAG_NONPOW2;
    }

    TRACE("%p\n", This);
    if ((This->pow2Width > GL_LIMITS(texture_size) || This->pow2Height > GL_LIMITS(texture_size)) && !(This->resource.usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL))) {
        /* one of three options
        1: Do the same as we do with nonpow 2 and scale the texture, (any texture ops would require the texture to be scaled which is potentially slow)
        2: Set the texture to the maximum size (bad idea)
        3:    WARN and return WINED3DERR_NOTAVAILABLE;
        4: Create the surface, but allow it to be used only for DirectDraw Blts. Some apps(e.g. Swat 3) create textures with a Height of 16 and a Width > 3000 and blt 16x16 letter areas from them to the render target.
        */
        WARN("(%p) Creating an oversized surface\n", This);
        This->Flags |= SFLAG_OVERSIZE;

        /* This will be initialized on the first blt */
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = 0;
        This->glRect.bottom = 0;
    } else {
        /* No oversize, gl rect is the full texture size */
        This->Flags &= ~SFLAG_OVERSIZE;
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = This->pow2Width;
        This->glRect.bottom = This->pow2Height;
    }

    This->Flags |= SFLAG_INSYSMEM;

    return WINED3D_OK;
}

static void WINAPI IWineD3DSurfaceImpl_ModifyLocation(IWineD3DSurface *iface, DWORD flag, BOOL persistent) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DBaseTexture *texture;

    TRACE("(%p)->(%s, %s)\n", iface,
          flag == SFLAG_INSYSMEM ? "SFLAG_INSYSMEM" : flag == SFLAG_INDRAWABLE ? "SFLAG_INDRAWABLE" : "SFLAG_INTEXTURE",
          persistent ? "TRUE" : "FALSE");

    /* TODO: For offscreen textures with fbo offscreen rendering the drawable is the same as the texture.*/
    if(persistent) {
        if((This->Flags & SFLAG_INTEXTURE) && !(flag & SFLAG_INTEXTURE)) {
            if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&texture) == WINED3D_OK) {
                TRACE("Passing to container\n");
                IWineD3DBaseTexture_SetDirty(texture, TRUE);
                IWineD3DBaseTexture_Release(texture);
            }
        }
        This->Flags &= ~SFLAG_LOCATIONS;
        This->Flags |= flag;
    } else {
        if((This->Flags & SFLAG_INTEXTURE) && (flag & SFLAG_INTEXTURE)) {
            if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&texture) == WINED3D_OK) {
                TRACE("Passing to container\n");
                IWineD3DBaseTexture_SetDirty(texture, TRUE);
                IWineD3DBaseTexture_Release(texture);
            }
        }
        This->Flags &= ~flag;
    }
}

struct coords {
    int x, y, z;
};

static inline void surface_blt_to_drawable(IWineD3DSurfaceImpl *This, const RECT *rect_in) {
    struct coords coords[4];
    RECT rect;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    if(rect_in) {
        rect = *rect_in;
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.right = This->currentDesc.Width;
        rect.bottom = This->currentDesc.Height;
    }

    ActivateContext(device, device->render_targets[0], CTXUSAGE_BLIT);
    ENTER_GL();

    if(This->glDescription.target == GL_TEXTURE_2D) {
        glBindTexture(GL_TEXTURE_2D, This->glDescription.textureName);
        checkGLcall("GL_TEXTURE_2D, This->glDescription.textureName)");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        coords[0].x = rect.left   / This->pow2Width;
        coords[0].z = 0;

        coords[1].x = rect.left   / This->pow2Width;
        coords[1].z = 0;

        coords[2].x = rect.right  / This->pow2Width;
        coords[2].z = 0;

        coords[3].x = rect.right  / This->pow2Width;
        coords[3].z = 0;

        coords[0].y = rect.top    / This->pow2Height;
        coords[1].y = rect.bottom / This->pow2Height;
        coords[2].y = rect.bottom / This->pow2Height;
        coords[3].y = rect.top    / This->pow2Height;
    } else {
        /* Must be a cube map */
        glDisable(GL_TEXTURE_2D);
        checkGLcall("glDisable(GL_TEXTURE_2D)");
        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glEnable(GL_TEXTURE_CUBE_MAP_ARB)");
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, This->glDescription.textureName);
        checkGLcall("GL_TEXTURE_CUBE_MAP_ARB, This->glDescription.textureName)");
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        switch(This->glDescription.target) {
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                coords[0].x =  1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x =  1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x = -1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x = -1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                coords[0].x = -1;   coords[0].y =  1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y =  1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y =  1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y =  1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;
                break;

            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z =  1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z =  1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z =  1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z =  1;
                break;

            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                coords[0].x = -1;   coords[0].y = -1;   coords[0].z = -1;
                coords[1].x =  1;   coords[1].y = -1;   coords[1].z = -1;
                coords[2].x =  1;   coords[2].y = -1;   coords[2].z = -1;
                coords[3].x = -1;   coords[3].y = -1;   coords[3].z = -1;

            default:
                ERR("Unexpected texture target\n");
                LEAVE_GL();
                return;
        }
    }

    glBegin(GL_QUADS);
    glTexCoord3iv((GLint *) &coords[0]);
    glVertex2i(rect.left, device->render_offscreen ? rect.bottom : rect.top);

    glTexCoord3iv((GLint *) &coords[1]);
    glVertex2i(0, device->render_offscreen ? rect.top : rect.bottom);

    glTexCoord3iv((GLint *) &coords[2]);
    glVertex2i(rect.right, device->render_offscreen ? rect.top : rect.bottom);

    glTexCoord3iv((GLint *) &coords[3]);
    glVertex2i(rect.right, device->render_offscreen ? rect.bottom : rect.top);
    glEnd();
    checkGLcall("glEnd");

    if(This->glDescription.target != GL_TEXTURE_2D) {
        glEnable(GL_TEXTURE_2D);
        checkGLcall("glEnable(GL_TEXTURE_2D)");
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    LEAVE_GL();
}

/*****************************************************************************
 * IWineD3DSurface::LoadLocation
 *
 * Copies the current surface data from wherever it is to the requested
 * location. The location is one of the surface flags, SFLAG_INSYSMEM,
 * SFLAG_INTEXTURE and SFLAG_INDRAWABLE. When the surface is current in
 * multiple locations, the gl texture is prefered over the drawable, which is
 * prefered over system memory. The PBO counts as system memory. If rect is
 * not NULL, only the specified rectangle is copied(only supported for
 * sysmem<->drawable copies at the moment). If rect is NULL, the destination
 * location is marked up to date after the copy.
 *
 * Parameters:
 *  flag: Surface location flag to be updated
 *  rect: rectangle to be copied
 *
 * Returns:
 *  WINED3D_OK on success
 *  WINED3DERR_DEVICELOST on an internal error
 *
 *****************************************************************************/
static HRESULT WINAPI IWineD3DSurfaceImpl_LoadLocation(IWineD3DSurface *iface, DWORD flag, const RECT *rect) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    GLenum format, internal, type;
    CONVERT_TYPES convert;
    int bpp;
    int width, pitch, outpitch;
    BYTE *mem;

    TRACE("(%p)->(%s, %p)\n", iface,
          flag == SFLAG_INSYSMEM ? "SFLAG_INSYSMEM" : flag == SFLAG_INDRAWABLE ? "SFLAG_INDRAWABLE" : "SFLAG_INTEXTURE",
          rect);
    if(rect) {
        TRACE("Rectangle: (%d,%d)-(%d,%d)\n", rect->left, rect->top, rect->right, rect->bottom);
    }

    /* TODO: For fbo targets, texture == drawable */
    if(This->Flags & flag) {
        TRACE("Location already up to date\n");
        return WINED3D_OK;
    }

    if(!(This->Flags & SFLAG_LOCATIONS)) {
        ERR("Surface does not have any up to date location\n");
        This->Flags |= SFLAG_LOST;
        return WINED3DERR_DEVICELOST;
    }

    if(flag == SFLAG_INSYSMEM) {
        surface_prepare_system_memory(This);

        /* Download the surface to system memory */
        if(This->Flags & SFLAG_INTEXTURE) {
            surface_download_data(This);
        } else {
            read_from_framebuffer(This, rect,
                                  This->resource.allocatedMemory,
                                  IWineD3DSurface_GetPitch(iface));
        }
    } else if(flag == SFLAG_INDRAWABLE) {
        if(This->Flags & SFLAG_INTEXTURE) {
            surface_blt_to_drawable(This, rect);
        } else {
            flush_to_framebuffer_drawpixels(This);
        }
    } else /* if(flag == SFLAG_INTEXTURE) */ {
        d3dfmt_get_conv(This, TRUE /* We need color keying */, TRUE /* We will use textures */, &format, &internal, &type, &convert, &bpp, This->srgb);

        if (This->Flags & SFLAG_INDRAWABLE) {
            GLint prevRead;

            ENTER_GL();
            glGetIntegerv(GL_READ_BUFFER, &prevRead);
            vcheckGLcall("glGetIntegerv");
            glReadBuffer(This->resource.wineD3DDevice->offscreenBuffer);
            vcheckGLcall("glReadBuffer");

            if(!(This->Flags & SFLAG_ALLOCATED)) {
                surface_allocate_surface(This, internal, This->pow2Width,
                                            This->pow2Height, format, type);
            }

            clear_unused_channels(This);

            glCopyTexSubImage2D(This->glDescription.target,
                                This->glDescription.level,
                                0, 0, 0, 0,
                                This->currentDesc.Width,
                                This->currentDesc.Height);
            checkGLcall("glCopyTexSubImage2D");

            glReadBuffer(prevRead);
            vcheckGLcall("glReadBuffer");

            LEAVE_GL();

            TRACE("Updated target %d\n", This->glDescription.target);
        } else {
            /* The only place where LoadTexture() might get called when isInDraw=1
             * is ActivateContext where lastActiveRenderTarget is preloaded.
             */
            if(iface == device->lastActiveRenderTarget && device->isInDraw)
                ERR("Reading back render target but SFLAG_INDRAWABLE not set\n");

            /* Otherwise: System memory copy must be most up to date */

            if(This->CKeyFlags & WINEDDSD_CKSRCBLT) {
                This->Flags |= SFLAG_GLCKEY;
                This->glCKey = This->SrcBltCKey;
            }
            else This->Flags &= ~SFLAG_GLCKEY;

            /* The width is in 'length' not in bytes */
            width = This->currentDesc.Width;
            pitch = IWineD3DSurface_GetPitch(iface);

            if((convert != NO_CONVERSION) && This->resource.allocatedMemory) {
                int height = This->currentDesc.Height;

                /* Stick to the alignment for the converted surface too, makes it easier to load the surface */
                outpitch = width * bpp;
                outpitch = (outpitch + device->surface_alignment - 1) & ~(device->surface_alignment - 1);

                mem = HeapAlloc(GetProcessHeap(), 0, outpitch * height);
                if(!mem) {
                    ERR("Out of memory %d, %d!\n", outpitch, height);
                    return WINED3DERR_OUTOFVIDEOMEMORY;
                }
                d3dfmt_convert_surface(This->resource.allocatedMemory, mem, pitch, width, height, outpitch, convert, This);

                This->Flags |= SFLAG_CONVERTED;
            } else if( (This->resource.format == WINED3DFMT_P8) && (GL_SUPPORT(EXT_PALETTED_TEXTURE) || GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) ) {
                d3dfmt_p8_upload_palette(iface, convert);
                This->Flags &= ~SFLAG_CONVERTED;
                mem = This->resource.allocatedMemory;
            } else {
                This->Flags &= ~SFLAG_CONVERTED;
                mem = This->resource.allocatedMemory;
            }

            /* Make sure the correct pitch is used */
            glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

            if ((This->Flags & SFLAG_NONPOW2) && !(This->Flags & SFLAG_OVERSIZE)) {
                TRACE("non power of two support\n");
                if(!(This->Flags & SFLAG_ALLOCATED)) {
                    surface_allocate_surface(This, internal, This->pow2Width, This->pow2Height, format, type);
                }
                if (mem || (This->Flags & SFLAG_PBO)) {
                    surface_upload_data(This, internal, This->currentDesc.Width, This->currentDesc.Height, format, type, mem);
                }
            } else {
                /* When making the realloc conditional, keep in mind that GL_APPLE_client_storage may be in use, and This->resource.allocatedMemory
                 * changed. So also keep track of memory changes. In this case the texture has to be reallocated
                 */
                if(!(This->Flags & SFLAG_ALLOCATED)) {
                    surface_allocate_surface(This, internal, This->glRect.right - This->glRect.left, This->glRect.bottom - This->glRect.top, format, type);
                }
                if (mem || (This->Flags & SFLAG_PBO)) {
                    surface_upload_data(This, internal, This->glRect.right - This->glRect.left, This->glRect.bottom - This->glRect.top, format, type, mem);
                }
            }

            /* Restore the default pitch */
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            /* Don't delete PBO memory */
            if((mem != This->resource.allocatedMemory) && !(This->Flags & SFLAG_PBO))
                HeapFree(GetProcessHeap(), 0, mem);
        }
    }

    if(rect == NULL) {
        This->Flags |= flag;
    }

    return WINED3D_OK;
}

const IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl =
{
    /* IUnknown */
    IWineD3DBaseSurfaceImpl_QueryInterface,
    IWineD3DBaseSurfaceImpl_AddRef,
    IWineD3DSurfaceImpl_Release,
    /* IWineD3DResource */
    IWineD3DBaseSurfaceImpl_GetParent,
    IWineD3DBaseSurfaceImpl_GetDevice,
    IWineD3DBaseSurfaceImpl_SetPrivateData,
    IWineD3DBaseSurfaceImpl_GetPrivateData,
    IWineD3DBaseSurfaceImpl_FreePrivateData,
    IWineD3DBaseSurfaceImpl_SetPriority,
    IWineD3DBaseSurfaceImpl_GetPriority,
    IWineD3DSurfaceImpl_PreLoad,
    IWineD3DBaseSurfaceImpl_GetType,
    /* IWineD3DSurface */
    IWineD3DBaseSurfaceImpl_GetContainer,
    IWineD3DBaseSurfaceImpl_GetDesc,
    IWineD3DSurfaceImpl_LockRect,
    IWineD3DSurfaceImpl_UnlockRect,
    IWineD3DSurfaceImpl_GetDC,
    IWineD3DSurfaceImpl_ReleaseDC,
    IWineD3DSurfaceImpl_Flip,
    IWineD3DSurfaceImpl_Blt,
    IWineD3DBaseSurfaceImpl_GetBltStatus,
    IWineD3DBaseSurfaceImpl_GetFlipStatus,
    IWineD3DBaseSurfaceImpl_IsLost,
    IWineD3DBaseSurfaceImpl_Restore,
    IWineD3DSurfaceImpl_BltFast,
    IWineD3DBaseSurfaceImpl_GetPalette,
    IWineD3DBaseSurfaceImpl_SetPalette,
    IWineD3DBaseSurfaceImpl_RealizePalette,
    IWineD3DBaseSurfaceImpl_SetColorKey,
    IWineD3DBaseSurfaceImpl_GetPitch,
    IWineD3DSurfaceImpl_SetMem,
    IWineD3DBaseSurfaceImpl_SetOverlayPosition,
    IWineD3DBaseSurfaceImpl_GetOverlayPosition,
    IWineD3DBaseSurfaceImpl_UpdateOverlayZOrder,
    IWineD3DBaseSurfaceImpl_UpdateOverlay,
    IWineD3DBaseSurfaceImpl_SetClipper,
    IWineD3DBaseSurfaceImpl_GetClipper,
    /* Internal use: */
    IWineD3DSurfaceImpl_AddDirtyRect,
    IWineD3DSurfaceImpl_LoadTexture,
    IWineD3DSurfaceImpl_SaveSnapshot,
    IWineD3DBaseSurfaceImpl_SetContainer,
    IWineD3DSurfaceImpl_SetGlTextureDesc,
    IWineD3DSurfaceImpl_GetGlDesc,
    IWineD3DSurfaceImpl_GetData,
    IWineD3DSurfaceImpl_SetFormat,
    IWineD3DSurfaceImpl_PrivateSetup,
    IWineD3DSurfaceImpl_ModifyLocation,
    IWineD3DSurfaceImpl_LoadLocation
};
