/*
 * IWineD3DSurface Implementation
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2007-2008 Henri Verbeet
 * Copyright 2006-2008 Roderick Colenbrander
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
WINE_DECLARE_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

static void d3dfmt_p8_init_palette(IWineD3DSurfaceImpl *This, BYTE table[256][4], BOOL colorkey);
static void d3dfmt_p8_upload_palette(IWineD3DSurface *iface, CONVERT_TYPES convert);
static void surface_remove_pbo(IWineD3DSurfaceImpl *This);

void surface_force_reload(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    This->Flags &= ~SFLAG_ALLOCATED;
}

void surface_set_texture_name(IWineD3DSurface *iface, GLuint name)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : setting texture name %u\n", This, name);

    if (!This->glDescription.textureName && name)
    {
        /* FIXME: We shouldn't need to remove SFLAG_INTEXTURE if the
         * surface has no texture name yet. See if we can get rid of this. */
        if (This->Flags & SFLAG_INTEXTURE)
            ERR("Surface has SFLAG_INTEXTURE set, but no texture name\n");
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INTEXTURE, FALSE);
    }

    This->glDescription.textureName = name;
    surface_force_reload(iface);
}

void surface_set_texture_target(IWineD3DSurface *iface, GLenum target)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : setting target %#x\n", This, target);

    if (This->glDescription.target != target)
    {
        if (target == GL_TEXTURE_RECTANGLE_ARB)
        {
            This->Flags &= ~SFLAG_NORMCOORD;
        }
        else if (This->glDescription.target == GL_TEXTURE_RECTANGLE_ARB)
        {
            This->Flags |= SFLAG_NORMCOORD;
        }
    }
    This->glDescription.target = target;
    surface_force_reload(iface);
}

static void surface_bind_and_dirtify(IWineD3DSurfaceImpl *This) {
    int active_sampler;

    /* We don't need a specific texture unit, but after binding the texture the current unit is dirty.
     * Read the unit back instead of switching to 0, this avoids messing around with the state manager's
     * gl states. The current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be called
     * from sampler() in state.c. This means we can't touch anything other than
     * whatever happens to be the currently active texture, or we would risk
     * marking already applied sampler states dirty again.
     *
     * TODO: Track the current active texture per GL context instead of using glGet
     */
    GLint active_texture;
    ENTER_GL();
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    LEAVE_GL();
    active_sampler = This->resource.wineD3DDevice->rev_tex_unit_map[active_texture - GL_TEXTURE0_ARB];

    if (active_sampler != -1) {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.wineD3DDevice, STATE_SAMPLER(active_sampler));
    }
    IWineD3DSurface_BindTexture((IWineD3DSurface *)This);
}

/* This function checks if the primary render target uses the 8bit paletted format. */
static BOOL primary_render_target_is_p8(IWineD3DDeviceImpl *device)
{
    if (device->render_targets && device->render_targets[0]) {
        IWineD3DSurfaceImpl* render_target = (IWineD3DSurfaceImpl*)device->render_targets[0];
        if((render_target->resource.usage & WINED3DUSAGE_RENDERTARGET) && (render_target->resource.format == WINED3DFMT_P8))
            return TRUE;
    }
    return FALSE;
}

/* This call just downloads data, the caller is responsible for activating the
 * right context and binding the correct texture. */
static void surface_download_data(IWineD3DSurfaceImpl *This) {
    if (0 == This->glDescription.textureName) {
        ERR("Surface does not have a texture, but SFLAG_INTEXTURE is set\n");
        return;
    }

    /* Only support read back of converted P8 surfaces */
    if(This->Flags & SFLAG_CONVERTED && (This->resource.format != WINED3DFMT_P8)) {
        FIXME("Read back converted textures unsupported, format=%s\n", debug_d3dformat(This->resource.format));
        return;
    }

    ENTER_GL();

    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5 ||
            This->resource.format == WINED3DFMT_ATI2N) {
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
        GLenum format = This->glDescription.glFormat;
        GLenum type = This->glDescription.glType;
        int src_pitch = 0;
        int dst_pitch = 0;

        /* In case of P8 the index is stored in the alpha component if the primary render target uses P8 */
        if(This->resource.format == WINED3DFMT_P8 && primary_render_target_is_p8(This->resource.wineD3DDevice)) {
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
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
                format, type, mem);

        if(This->Flags & SFLAG_PBO) {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, This->pbo));
            checkGLcall("glBindBufferARB");

            glGetTexImage(This->glDescription.target, This->glDescription.level, format,
                          type, NULL);
            checkGLcall("glGetTexImage()");

            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
            checkGLcall("glBindBufferARB");
        } else {
            glGetTexImage(This->glDescription.target, This->glDescription.level, format,
                          type, mem);
            checkGLcall("glGetTexImage()");
        }
        LEAVE_GL();

        if (This->Flags & SFLAG_NONPOW2) {
            const BYTE *src_data;
            BYTE *dst_data;
            UINT y;
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

/* This call just uploads data, the caller is responsible for activating the
 * right context and binding the correct texture. */
static void surface_upload_data(IWineD3DSurfaceImpl *This, GLenum internal, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data) {

    if(This->heightscale != 1.0 && This->heightscale != 0.0) height *= This->heightscale;

    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5 ||
            This->resource.format == WINED3DFMT_ATI2N) {
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

/* This call just allocates the texture, the caller is responsible for
 * activating the right context and binding the correct texture. */
static void surface_allocate_surface(IWineD3DSurfaceImpl *This, GLenum internal, GLsizei width, GLsizei height, GLenum format, GLenum type) {
    BOOL enable_client_storage = FALSE;
    const BYTE *mem = NULL;

    if(This->heightscale != 1.0 && This->heightscale != 0.0) height *= This->heightscale;

    TRACE("(%p) : Creating surface (target %#x)  level %d, d3d format %s, internal format %#x, width %d, height %d, gl format %#x, gl type=%#x\n", This,
            This->glDescription.target, This->glDescription.level, debug_d3dformat(This->resource.format), internal, width, height, format, type);

    if (This->resource.format == WINED3DFMT_DXT1 ||
            This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
            This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5 ||
            This->resource.format == WINED3DFMT_ATI2N) {
        /* glCompressedTexImage2D does not accept NULL pointers, so we cannot allocate a compressed texture without uploading data */
        TRACE("Not allocating compressed surfaces, surface_upload_data will specify them\n");

        /* We have to point GL to the client storage memory here, because upload_data might use a PBO. This means a double upload
         * once, unfortunately
         */
        if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
            /* Neither NONPOW2, DIBSECTION nor OVERSIZE flags can be set on compressed textures */
            This->Flags |= SFLAG_CLIENT;
            mem = (BYTE *)(((ULONG_PTR) This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
            ENTER_GL();
            GL_EXTCALL(glCompressedTexImage2DARB(This->glDescription.target, This->glDescription.level, internal,
                       width, height, 0 /* border */, This->resource.size, mem));
            LEAVE_GL();
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
        const struct GlPixelFormatDesc *glDesc;
        getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);

        GL_EXTCALL(glGenRenderbuffersEXT(1, &renderbuffer));
        GL_EXTCALL(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer));
        GL_EXTCALL(glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, glDesc->glInternal, width, height));

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

/* Slightly inefficient way to handle multiple dirty rects but it works :) */
void surface_add_dirty_rect(IWineD3DSurface *iface, const RECT *dirty_rect)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;

    if (!(This->Flags & SFLAG_INSYSMEM) && (This->Flags & SFLAG_INTEXTURE))
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL /* no partial locking for textures yet */);

    IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
    if (dirty_rect)
    {
        This->dirtyRect.left = min(This->dirtyRect.left, dirty_rect->left);
        This->dirtyRect.top = min(This->dirtyRect.top, dirty_rect->top);
        This->dirtyRect.right = max(This->dirtyRect.right, dirty_rect->right);
        This->dirtyRect.bottom = max(This->dirtyRect.bottom, dirty_rect->bottom);
    }
    else
    {
        This->dirtyRect.left = 0;
        This->dirtyRect.top = 0;
        This->dirtyRect.right = This->currentDesc.Width;
        This->dirtyRect.bottom = This->currentDesc.Height;
    }

    TRACE("(%p) : Dirty: yes, Rect:(%d, %d, %d, %d)\n", This, This->dirtyRect.left,
            This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);

    /* if the container is a basetexture then mark it dirty. */
    if (SUCCEEDED(IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture)))
    {
        TRACE("Passing to container\n");
        IWineD3DBaseTexture_SetDirty(baseTexture, TRUE);
        IWineD3DBaseTexture_Release(baseTexture);
    }
}

static ULONG WINAPI IWineD3DSurfaceImpl_Release(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
        renderbuffer_entry_t *entry, *entry2;
        TRACE("(%p) : cleaning up\n", This);

        /* Need a context to destroy the texture. Use the currently active render target, but only if
         * the primary render target exists. Otherwise lastActiveRenderTarget is garbage, see above.
         * When destroying the primary rt, Uninit3D will activate a context before doing anything
         */
        if(device->render_targets && device->render_targets[0]) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }

        ENTER_GL();
        if (This->glDescription.textureName != 0) { /* release the openGL texture.. */
            TRACE("Deleting texture %d\n", This->glDescription.textureName);
            glDeleteTextures(1, &This->glDescription.textureName);
        }

        if(This->Flags & SFLAG_PBO) {
            /* Delete the PBO */
            GL_EXTCALL(glDeleteBuffersARB(1, &This->pbo));
        }

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &This->renderbuffers, renderbuffer_entry_t, entry) {
            GL_EXTCALL(glDeleteRenderbuffersEXT(1, &entry->id));
            HeapFree(GetProcessHeap(), 0, entry);
        }
        LEAVE_GL();

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

        resource_cleanup((IWineD3DResource *)iface);

        if(This->overlay_dest) {
            list_remove(&This->overlay_entry);
        }

        TRACE("(%p) Released\n", This);
        HeapFree(GetProcessHeap(), 0, This);

    }
    return ref;
}

/* ****************************************************
   IWineD3DSurface IWineD3DResource parts follow
   **************************************************** */

static void WINAPI IWineD3DSurfaceImpl_PreLoad(IWineD3DSurface *iface)
{
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

        if (This->resource.format == WINED3DFMT_P8 || This->resource.format == WINED3DFMT_A8P8) {
            if(palette9_changed(This)) {
                TRACE("Reloading surface because the d3d8/9 palette was changed\n");
                /* TODO: This is not necessarily needed with hw palettized texture support */
                IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change, this kills performance though :( */
                IWineD3DSurface_ModifyLocation(iface, SFLAG_INTEXTURE, FALSE);
            }
        }

        IWineD3DSurface_LoadTexture(iface, FALSE);

        if (This->resource.pool == WINED3DPOOL_DEFAULT) {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            ENTER_GL();
            glPrioritizeTextures(1, &This->glDescription.textureName, &tmp);
            LEAVE_GL();
        }
    }
    return;
}

static void surface_remove_pbo(IWineD3DSurfaceImpl *This) {
    This->resource.heapMemory = HeapAlloc(GetProcessHeap() ,0 , This->resource.size + RESOURCE_ALIGNMENT);
    This->resource.allocatedMemory =
            (BYTE *)(((ULONG_PTR) This->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));

    ENTER_GL();
    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
    checkGLcall("glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->pbo)");
    GL_EXTCALL(glGetBufferSubDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0, This->resource.size, This->resource.allocatedMemory));
    checkGLcall("glGetBufferSubData");
    GL_EXTCALL(glDeleteBuffersARB(1, &This->pbo));
    checkGLcall("glDeleteBuffers");
    LEAVE_GL();

    This->pbo = 0;
    This->Flags &= ~SFLAG_PBO;
}

static void WINAPI IWineD3DSurfaceImpl_UnLoad(IWineD3DSurface *iface) {
    IWineD3DBaseTexture *texture = NULL;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    renderbuffer_entry_t *entry, *entry2;
    TRACE("(%p)\n", iface);

    if(This->resource.pool == WINED3DPOOL_DEFAULT) {
        /* Default pool resources are supposed to be destroyed before Reset is called.
         * Implicit resources stay however. So this means we have an implicit render target
         * or depth stencil. The content may be destroyed, but we still have to tear down
         * opengl resources, so we cannot leave early.
         *
         * Put the most up to date surface location into the drawable. D3D-wise this content
         * is undefined, so it would be nowhere, but that would make the location management
         * more complicated. The drawable is a sane location, because if we mark sysmem or
         * texture up to date, drawPrim will copy the uninitialized texture or sysmem to the
         * uninitialized drawable. That's pointless and we'd have to allocate the texture /
         * sysmem copy here.
         */
        if (This->resource.usage & WINED3DUSAGE_DEPTHSTENCIL) {
            IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
        } else {
            IWineD3DSurface_ModifyLocation(iface, SFLAG_INDRAWABLE, TRUE);
        }
    } else {
        /* Load the surface into system memory */
        IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL);
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INDRAWABLE, FALSE);
    }
    IWineD3DSurface_ModifyLocation(iface, SFLAG_INTEXTURE, FALSE);
    This->Flags &= ~SFLAG_ALLOCATED;

    /* Destroy PBOs, but load them into real sysmem before */
    if(This->Flags & SFLAG_PBO) {
        surface_remove_pbo(This);
    }

    /* Destroy fbo render buffers. This is needed for implicit render targets, for
     * all application-created targets the application has to release the surface
     * before calling _Reset
     */
    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &This->renderbuffers, renderbuffer_entry_t, entry) {
        ENTER_GL();
        GL_EXTCALL(glDeleteRenderbuffersEXT(1, &entry->id));
        LEAVE_GL();
        list_remove(&entry->entry);
        HeapFree(GetProcessHeap(), 0, entry);
    }
    list_init(&This->renderbuffers);
    This->current_renderbuffer = NULL;

    /* If we're in a texture, the texture name belongs to the texture. Otherwise,
     * destroy it
     */
    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **) &texture);
    if(!texture) {
        ENTER_GL();
        glDeleteTextures(1, &This->glDescription.textureName);
        This->glDescription.textureName = 0;
        LEAVE_GL();
    } else {
        IWineD3DBaseTexture_Release(texture);
    }
    return;
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

static void WINAPI IWineD3DSurfaceImpl_GetGlDesc(IWineD3DSurface *iface, glDescriptor **glDescription)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, &This->glDescription);
    *glDescription = &This->glDescription;
}

/* Read the framebuffer back into the surface */
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
    GLint rowLen = 0;
    GLint skipPix = 0;
    GLint skipRow = 0;

    if(wined3d_settings.rendertargetlock_mode == RTL_DISABLE) {
        static BOOL warned = FALSE;
        if(!warned) {
            ERR("The application tries to lock the render target, but render target locking is disabled\n");
            warned = TRUE;
        }
        return;
    }

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
    if (SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface *) This, &IID_IWineD3DSwapChain, (void **)&swapchain)))
    {
        GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *) This, (IWineD3DSwapChain *)swapchain);
        TRACE("Locking %#x buffer\n", buffer);
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
        srcIsUpsideDown = FALSE;
    } else {
        /* Locking the primary render target which is not on a swapchain(=offscreen render target).
         * Read from the back buffer
         */
        TRACE("Locking offscreen render target\n");
        glReadBuffer(myDevice->offscreenBuffer);
        srcIsUpsideDown = TRUE;
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
            if(primary_render_target_is_p8(myDevice)) {
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
        if(mem != NULL) {
            ERR("mem not null for pbo -- unexpected\n");
            mem = NULL;
        }
    }

    /* Save old pixel store pack state */
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLen);
    checkGLcall("glIntegerv");
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPix);
    checkGLcall("glIntegerv");
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRow);
    checkGLcall("glIntegerv");

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    glPixelStorei(GL_PACK_ROW_LENGTH, This->currentDesc.Width);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_PIXELS, local_rect.left);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_ROWS, local_rect.top);
    checkGLcall("glPixelStorei");

    glReadPixels(local_rect.left, (!srcIsUpsideDown) ? (This->currentDesc.Height - local_rect.bottom) : local_rect.top ,
                 local_rect.right - local_rect.left,
                 local_rect.bottom - local_rect.top,
                 fmt, type, mem);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    glPixelStorei(GL_PACK_ROW_LENGTH, rowLen);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_PIXELS, skipPix);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_ROWS, skipRow);
    checkGLcall("glPixelStorei");

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
        bottom = mem + pitch * (local_rect.bottom - 1);
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

    LEAVE_GL();

    /* For P8 textures we need to perform an inverse palette lookup. This is done by searching for a palette
     * index which matches the RGB value. Note this isn't guaranteed to work when there are multiple entries for
     * the same color but we have no choice.
     * In case of P8 render targets, the index is stored in the alpha component so no conversion is needed.
     */
    if((This->resource.format == WINED3DFMT_P8) && !primary_render_target_is_p8(myDevice)) {
        const PALETTEENTRY *pal = NULL;
        DWORD width = pitch / 3;
        int x, y, c;

        if(This->palette) {
            pal = This->palette->palents;
        } else {
            ERR("Palette is missing, cannot perform inverse palette lookup\n");
            HeapFree(GetProcessHeap(), 0, mem);
            return ;
        }

        for(y = local_rect.top; y < local_rect.bottom; y++) {
            for(x = local_rect.left; x < local_rect.right; x++) {
                /*                      start              lines            pixels      */
                const BYTE *blue = mem + y * pitch + x * (sizeof(BYTE) * 3);
                const BYTE *green = blue  + 1;
                const BYTE *red = green + 1;

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
}

/* Read the framebuffer contents into a texture */
static void read_from_framebuffer_texture(IWineD3DSurfaceImpl *This)
{
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain;
    int bpp;
    GLenum format, internal, type;
    CONVERT_TYPES convert;
    GLint prevRead;

    d3dfmt_get_conv(This, TRUE /* We need color keying */, TRUE /* We will use textures */, &format, &internal, &type, &convert, &bpp, This->srgb);

    /* Activate the surface to read from. In some situations it isn't the currently active target(e.g. backbuffer
     * locking during offscreen rendering). RESOURCELOAD is ok because glCopyTexSubImage2D isn't affected by any
     * states in the stateblock, and no driver was found yet that had bugs in that regard.
     */
    ActivateContext(device, (IWineD3DSurface *) This, CTXUSAGE_RESOURCELOAD);
    surface_bind_and_dirtify(This);

    ENTER_GL();
    glGetIntegerv(GL_READ_BUFFER, &prevRead);
    LEAVE_GL();

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it, every part of the code
     * that reads sets the read buffer as desired.
     */
    if (SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface *)This, &IID_IWineD3DSwapChain, (void **)&swapchain)))
    {
        GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *) This, (IWineD3DSwapChain *)swapchain);
        TRACE("Locking %#x buffer\n", buffer);

        ENTER_GL();
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer");
        LEAVE_GL();

        IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    } else {
        /* Locking the primary render target which is not on a swapchain(=offscreen render target).
         * Read from the back buffer
         */
        TRACE("Locking offscreen render target\n");
        ENTER_GL();
        glReadBuffer(device->offscreenBuffer);
        checkGLcall("glReadBuffer");
        LEAVE_GL();
    }

    if(!(This->Flags & SFLAG_ALLOCATED)) {
        surface_allocate_surface(This, internal, This->pow2Width,
                                 This->pow2Height, format, type);
    }

    ENTER_GL();
    /* If !SrcIsUpsideDown we should flip the surface.
     * This can be done using glCopyTexSubImage2D but this
     * is VERY slow, so don't do that. We should prevent
     * this code from getting called in such cases or perhaps
     * we can use FBOs */

    glCopyTexSubImage2D(This->glDescription.target,
                        This->glDescription.level,
                        0, 0, 0, 0,
                        This->currentDesc.Width,
                        This->currentDesc.Height);
    checkGLcall("glCopyTexSubImage2D");

    glReadBuffer(prevRead);
    checkGLcall("glReadBuffer");

    LEAVE_GL();
    TRACE("Updated target %d\n", This->glDescription.target);
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
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
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
        surface_prepare_system_memory(This); /* Makes sure memory is allocated */
        This->Flags |= SFLAG_INSYSMEM;
        goto lock_end;
    }

    if (This->Flags & SFLAG_INSYSMEM) {
        TRACE("Local copy is up to date, not downloading data\n");
        surface_prepare_system_memory(This); /* Makes sure memory is allocated */
        goto lock_end;
    }

    /* Now download the surface content from opengl
     * Use the render target readback if the surface is on a swapchain(=onscreen render target) or the current primary target
     * Offscreen targets which are not active at the moment or are higher targets(FBOs) can be locked with the texture path
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
                IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, pass_rect);
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
        surface_add_dirty_rect(iface, pRect);

        /** Dirtify Container if needed */
        if (SUCCEEDED(IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&pBaseTexture))) {
            TRACE("Making container dirty\n");
            IWineD3DBaseTexture_SetDirty(pBaseTexture, TRUE);
            IWineD3DBaseTexture_Release(pBaseTexture);
        } else {
            TRACE("Surface is standalone, no need to dirty the container\n");
        }
    }

    return IWineD3DBaseSurfaceImpl_LockRect(iface, pLockedRect, pRect, Flags);
}

static void flush_to_framebuffer_drawpixels(IWineD3DSurfaceImpl *This, GLenum fmt, GLenum type, UINT bpp, const BYTE *mem) {
    GLint  prev_store;
    GLint  prev_rasterpos[4];
    GLint skipBytes = 0;
    UINT pitch = IWineD3DSurface_GetPitch((IWineD3DSurface *) This);    /* target is argb, 4 byte */
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain;

    /* Activate the correct context for the render target */
    ActivateContext(myDevice, (IWineD3DSurface *) This, CTXUSAGE_BLIT);
    ENTER_GL();

    if (SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface *)This, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
        GLenum buffer = surface_get_gl_buffer((IWineD3DSurface *) This, (IWineD3DSwapChain *)swapchain);
        TRACE("Unlocking %#x buffer\n", buffer);
        glDrawBuffer(buffer);
        checkGLcall("glDrawBuffer");

        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
    } else {
        /* Primary offscreen render target */
        TRACE("Offscreen render target\n");
        glDrawBuffer(myDevice->offscreenBuffer);
        checkGLcall("glDrawBuffer(myDevice->offscreenBuffer)");
    }

    glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
    checkGLcall("glIntegerv");
    glGetIntegerv(GL_CURRENT_RASTER_POSITION, &prev_rasterpos[0]);
    checkGLcall("glIntegerv");
    glPixelZoom(1.0, -1.0);
    checkGLcall("glPixelZoom");

    /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &skipBytes);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, This->currentDesc.Width);

    glRasterPos3i(This->lockedRect.left, This->lockedRect.top, 1);
    checkGLcall("glRasterPos2f");

    /* Some drivers(radeon dri, others?) don't like exceptions during
     * glDrawPixels. If the surface is a DIB section, it might be in GDIMode
     * after ReleaseDC. Reading it will cause an exception, which x11drv will
     * catch to put the dib section in InSync mode, which leads to a crash
     * and a blocked x server on my radeon card.
     *
     * The following lines read the dib section so it is put in InSync mode
     * before glDrawPixels is called and the crash is prevented. There won't
     * be any interfering gdi accesses, because UnlockRect is called from
     * ReleaseDC, and the app won't use the dc any more afterwards.
     */
    if((This->Flags & SFLAG_DIBSECTION) && !(This->Flags & SFLAG_PBO)) {
        volatile BYTE read;
        read = This->resource.allocatedMemory[0];
    }

    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, This->pbo));
        checkGLcall("glBindBufferARB");
    }

    /* When the surface is locked we only have to refresh the locked part else we need to update the whole image */
    if(This->Flags & SFLAG_LOCKED) {
        glDrawPixels(This->lockedRect.right - This->lockedRect.left,
                     (This->lockedRect.bottom - This->lockedRect.top)-1,
                     fmt, type,
                     mem + bpp * This->lockedRect.left + pitch * This->lockedRect.top);
        checkGLcall("glDrawPixels");
    } else {
        glDrawPixels(This->currentDesc.Width,
                     This->currentDesc.Height,
                     fmt, type, mem);
        checkGLcall("glDrawPixels");
    }

    if(This->Flags & SFLAG_PBO) {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");
    }

    glPixelZoom(1.0,1.0);
    checkGLcall("glPixelZoom");

    glRasterPos3iv(&prev_rasterpos[0]);
    checkGLcall("glRasterPos3iv");

    /* Reset to previous pack row length */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, skipBytes);
    checkGLcall("glPixelStorei GL_UNPACK_ROW_LENGTH");

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
        return WINEDDERR_NOTLOCKED;
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
        if(swapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);

        if(wined3d_settings.rendertargetlock_mode == RTL_DISABLE) {
            static BOOL warned = FALSE;
            if(!warned) {
                ERR("The application tries to write to the render target, but render target locking is disabled\n");
                warned = TRUE;
            }
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

    /* Overlays have to be redrawn manually after changes with the GL implementation */
    if(This->overlay_dest) {
        IWineD3DSurface_DrawOverlay(iface);
    }
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_GetDC(IWineD3DSurface *iface, HDC *pHDC)
{
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

    /* According to Direct3D9 docs, only these formats are supported */
    if (((IWineD3DImpl *)This->resource.wineD3DDevice->wineD3D)->dxVersion > 7) {
        if (This->resource.format != WINED3DFMT_R5G6B5 &&
            This->resource.format != WINED3DFMT_X1R5G5B5 &&
            This->resource.format != WINED3DFMT_R8G8B8 &&
            This->resource.format != WINED3DFMT_X8R8G8B8) return WINED3DERR_INVALIDCALL;
    }

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
        /* GetDC on palettized formats is unsupported in D3D9, and the method is missing in
            D3D8, so this should only be used for DX <=7 surfaces (with non-device palettes) */
        unsigned int n;
        const PALETTEENTRY *pal = NULL;

        if(This->palette) {
            pal = This->palette->palents;
        } else {
            IWineD3DSurfaceImpl *dds_primary;
            IWineD3DSwapChainImpl *swapchain;
            swapchain = (IWineD3DSwapChainImpl *)This->resource.wineD3DDevice->swapchains[0];
            dds_primary = (IWineD3DSurfaceImpl *)swapchain->frontBuffer;
            if (dds_primary && dds_primary->palette)
                pal = dds_primary->palette->palents;
        }

        if (pal) {
            for (n=0; n<256; n++) {
                col[n].rgbRed   = pal[n].peRed;
                col[n].rgbGreen = pal[n].peGreen;
                col[n].rgbBlue  = pal[n].peBlue;
                col[n].rgbReserved = 0;
            }
            SetDIBColorTable(This->hDC, 0, 256, col);
        }
    }

    *pHDC = This->hDC;
    TRACE("returning %p\n",*pHDC);
    This->Flags |= SFLAG_DCINUSE;

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_ReleaseDC(IWineD3DSurface *iface, HDC hDC)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,hDC);

    if (!(This->Flags & SFLAG_DCINUSE))
        return WINEDDERR_NODC;

    if (This->hDC !=hDC) {
        WARN("Application tries to release an invalid DC(%p), surface dc is %p\n", hDC, This->hDC);
        return WINEDDERR_NODC;
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
    const struct GlPixelFormatDesc *glDesc;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);

    /* Default values: From the surface */
    *format = glDesc->glFormat;
    *type = glDesc->glType;
    *convert = NO_CONVERSION;
    *target_bpp = This->bytesPerPixel;

    if(srgb_mode) {
        *internal = glDesc->glGammaInternal;
    } else if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        *internal = glDesc->rtInternal;
    } else {
        *internal = glDesc->glInternal;
    }

    /* Ok, now look if we have to do any conversion */
    switch(This->resource.format) {
        case WINED3DFMT_P8:
            /* ****************
                Paletted Texture
                **************** */

             /* Use conversion when the paletted texture extension OR fragment shaders are available. When either
             * of the two is available make sure texturing is requested as neither of the two works in
             * conjunction with calls like glDraw-/glReadPixels. Further also use conversion in case of color keying.
             * Paletted textures can be emulated using shaders but only do that for 2D purposes e.g. situations
             * in which the main render target uses p8. Some games like GTA Vice City use P8 for texturing which
             * conflicts with this.
             */
            if( !(GL_SUPPORT(EXT_PALETTED_TEXTURE) ||
                  (GL_SUPPORT(ARB_FRAGMENT_PROGRAM) &&
                   device->render_targets &&
                   This == (IWineD3DSurfaceImpl*)device->render_targets[0])) ||
                colorkey_active || !use_texturing ) {
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
            else if(!GL_SUPPORT(EXT_PALETTED_TEXTURE) && GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
                *format = GL_ALPHA;
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
            break;

        case WINED3DFMT_V16U16:
            if(GL_SUPPORT(NV_TEXTURE_SHADER3)) break;
            *convert = CONVERT_V16U16;
            *format = GL_BGR;
            *internal = GL_RGB16_EXT;
            *type = GL_UNSIGNED_SHORT;
            *target_bpp = 6;
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

        case WINED3DFMT_G16R16:
            *convert = CONVERT_G16R16;
            *format = GL_RGB;
            *internal = GL_RGB16_EXT;
            *type = GL_UNSIGNED_SHORT;
            *target_bpp = 6;
            break;

        default:
            break;
    }

    return WINED3D_OK;
}

static HRESULT d3dfmt_convert_surface(const BYTE *src, BYTE *dst, UINT pitch, UINT width,
        UINT height, UINT outpitch, CONVERT_TYPES convert, IWineD3DSurfaceImpl *This)
{
    const BYTE *source;
    BYTE *dest;
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
            const WORD *Source;
            WORD *Dest;

            TRACE("Color keyed 565\n");

            for (y = 0; y < height; y++) {
                Source = (const WORD *)(src + y * pitch);
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
            const WORD *Source;
            WORD *Dest;
            TRACE("Color keyed 5551\n");
            for (y = 0; y < height; y++) {
                Source = (const WORD *)(src + y * pitch);
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

        case CONVERT_CK_RGB24:
        {
            /* Converting R8G8B8 format to R8G8B8A8 with color-keying. */
            unsigned int x, y;
            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                for (x = 0; x < width; x++) {
                    DWORD color = ((DWORD)source[0] << 16) + ((DWORD)source[1] << 8) + (DWORD)source[2] ;
                    DWORD dstcolor = color << 8;
                    if ((color < This->SrcBltCKey.dwColorSpaceLowValue) ||
                        (color > This->SrcBltCKey.dwColorSpaceHighValue)) {
                        dstcolor |= 0xff;
                    }
                    *(DWORD*)dest = dstcolor;
                    source += 3;
                    dest += 4;
                }
            }
        }
        break;

        case CONVERT_RGB32_888:
        {
            /* Converting X8R8G8B8 format to R8G8B8A8 with color-keying. */
            unsigned int x, y;
            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                for (x = 0; x < width; x++) {
                    DWORD color = 0xffffff & *(const DWORD*)source;
                    DWORD dstcolor = color << 8;
                    if ((color < This->SrcBltCKey.dwColorSpaceLowValue) ||
                        (color > This->SrcBltCKey.dwColorSpaceHighValue)) {
                        dstcolor |= 0xff;
                    }
                    *(DWORD*)dest = dstcolor;
                    source += 4;
                    dest += 4;
                }
            }
        }
        break;

        case CONVERT_V8U8:
        {
            unsigned int x, y;
            const short *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = (const short *)(src + y * pitch);
                Dest = dst + y * outpitch;
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
            const DWORD *Source;
            unsigned short *Dest;
            for(y = 0; y < height; y++) {
                Source = (const DWORD *)(src + y * pitch);
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
            const DWORD *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = (const DWORD *)(src + y * pitch);
                Dest = dst + y * outpitch;
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
            const WORD *Source;
            unsigned char *Dest;

            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* This makes the gl surface bigger(24 bit instead of 16), but it works with
                 * fixed function and shaders without further conversion once the surface is
                 * loaded
                 */
                for(y = 0; y < height; y++) {
                    Source = (const WORD *)(src + y * pitch);
                    Dest = dst + y * outpitch;
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
                    Source = (const WORD *)(src + y * pitch);
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
            const DWORD *Source;
            unsigned char *Dest;

            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* This implementation works with the fixed function pipeline and shaders
                 * without further modification after converting the surface.
                 */
                for(y = 0; y < height; y++) {
                    Source = (const DWORD *)(src + y * pitch);
                    Dest = dst + y * outpitch;
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
                    Source = (const DWORD *)(src + y * pitch);
                    Dest = dst + y * outpitch;
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
            const unsigned char *Source;
            unsigned char *Dest;
            for(y = 0; y < height; y++) {
                Source = src + y * pitch;
                Dest = dst + y * outpitch;
                for (x = 0; x < width; x++ ) {
                    unsigned char color = (*Source++);
                    /* A */ Dest[1] = (color & 0xf0) << 0;
                    /* L */ Dest[0] = (color & 0x0f) << 4;
                    Dest += 2;
                }
            }
            break;
        }

        case CONVERT_G16R16:
        {
            unsigned int x, y;
            const WORD *Source;
            WORD *Dest;

            for(y = 0; y < height; y++) {
                Source = (const WORD *)(src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD green = (*Source++);
                    WORD red = (*Source++);
                    Dest[0] = green;
                    Dest[1] = red;
                    Dest[2] = 0xffff;
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
    int dxVersion = ( (IWineD3DImpl *) device->wineD3D)->dxVersion;
    unsigned int i;

    /* Old games like StarCraft, C&C, Red Alert and others use P8 render targets.
    * Reading back the RGB output each lockrect (each frame as they lock the whole screen)
    * is slow. Further RGB->P8 conversion is not possible because palettes can have
    * duplicate entries. Store the color key in the unused alpha component to speed the
    * download up and to make conversion unneeded. */
    index_in_alpha = primary_render_target_is_p8(device);

    if (pal == NULL) {
        /* In DirectDraw the palette is a property of the surface, there are no such things as device palettes. */
        if(dxVersion <= 7) {
            ERR("This code should never get entered for DirectDraw!, expect problems\n");
            if(index_in_alpha) {
                /* Guarantees that memory representation remains correct after sysmem<->texture transfers even if
                   there's no palette at this time. */
                for (i = 0; i < 256; i++) table[i][3] = i;
            }
        } else {
            /*  Direct3D >= 8 palette usage style: P8 textures use device palettes, palette entry format is A8R8G8B8,
                alpha is stored in peFlags and may be used by the app if D3DPTEXTURECAPS_ALPHAPALETTE device
                capability flag is present (wine does advertise this capability) */
            for (i = 0; i < 256; i++) {
                table[i][0] = device->palettes[device->currentPalette][i].peRed;
                table[i][1] = device->palettes[device->currentPalette][i].peGreen;
                table[i][2] = device->palettes[device->currentPalette][i].peBlue;
                table[i][3] = device->palettes[device->currentPalette][i].peFlags;
            }
        }
    } else {
        TRACE("Using surface palette %p\n", pal);
        /* Get the surface's palette */
        for (i = 0; i < 256; i++) {
            table[i][0] = pal->palents[i].peRed;
            table[i][1] = pal->palents[i].peGreen;
            table[i][2] = pal->palents[i].peBlue;

            /* When index_in_alpha is the palette index is stored in the alpha component. In case of a readback
               we can then read GL_ALPHA. Color keying is handled in BltOverride using a GL_ALPHA_TEST using GL_NOT_EQUAL.
               In case of index_in_alpha the color key itself is passed to glAlphaFunc in other cases the alpha component
               of pixels that should be masked away is set to 0. */
            if(index_in_alpha) {
                table[i][3] = i;
            } else if(colorkey && (i >= This->SrcBltCKey.dwColorSpaceLowValue) &&  (i <= This->SrcBltCKey.dwColorSpaceHighValue)) {
                table[i][3] = 0x00;
            } else if(pal->Flags & WINEDDPCAPS_ALPHA) {
                table[i][3] = pal->palents[i].peFlags;
            } else {
                table[i][3] = 0xFF;
            }
        }
    }
}

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
        GL_EXTCALL(glColorTableEXT(This->glDescription.target,GL_RGBA,256,GL_RGBA,GL_UNSIGNED_BYTE, table));
    }
    else
    {
        /* Let a fragment shader do the color conversion by uploading the palette to a 1D texture.
         * The 8bit pixel data will be used as an index in this palette texture to retrieve the final color. */
        TRACE("Using fragment shaders for emulating 8-bit paletted texture support\n");

        /* Create the fragment program if we don't have it */
        if(!device->paletteConversionShader)
        {
            const char *fragment_palette_conversion =
                "!!ARBfp1.0\n"
                "TEMP index;\n"
                /* { 255/256, 0.5/255*255/256, 0, 0 } */
                "PARAM constants = { 0.996, 0.00195, 0, 0 };\n"
                /* The alpha-component contains the palette index */
                "TEX index, fragment.texcoord[0], texture[0], 2D;\n"
                /* Scale the index by 255/256 and add a bias of '0.5' in order to sample in the middle */
                "MAD index.a, index.a, constants.x, constants.y;\n"
                /* Use the alpha-component as an index in the palette to get the final color */
                "TEX result.color, index.a, texture[1], 1D;\n"
                "END";

            glEnable(GL_FRAGMENT_PROGRAM_ARB);
            GL_EXTCALL(glGenProgramsARB(1, &device->paletteConversionShader));
            GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, device->paletteConversionShader));
            GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(fragment_palette_conversion), fragment_palette_conversion));
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
        }

        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, device->paletteConversionShader));

        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE1));
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

BOOL palette9_changed(IWineD3DSurfaceImpl *This) {
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
        /* Make sure the texture is reloaded because of the color key change, this kills performance though :( */
        /* TODO: This is not necessarily needed with hw palettized texture support */
        IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
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

static void WINAPI IWineD3DSurfaceImpl_BindTexture(IWineD3DSurface *iface) {
    /* TODO: check for locks */
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("(%p)Checking to see if the container is a base texture\n", This);
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == WINED3D_OK) {
        TRACE("Passing to container\n");
        IWineD3DBaseTexture_BindTexture(baseTexture);
        IWineD3DBaseTexture_Release(baseTexture);
    } else {
        TRACE("(%p) : Binding surface\n", This);

        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }

        ENTER_GL();

        if (!This->glDescription.level) {
            if (!This->glDescription.textureName) {
                glGenTextures(1, &This->glDescription.textureName);
                checkGLcall("glGenTextures");
                TRACE("Surface %p given name %d\n", This, This->glDescription.textureName);

                glBindTexture(This->glDescription.target, This->glDescription.textureName);
                checkGLcall("glBindTexture");
                glTexParameteri(This->glDescription.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                checkGLcall("glTexParameteri(dimension, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)");
                glTexParameteri(This->glDescription.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                checkGLcall("glTexParameteri(dimension, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)");
                glTexParameteri(This->glDescription.target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                checkGLcall("glTexParameteri(dimension, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)");
                glTexParameteri(This->glDescription.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                checkGLcall("glTexParameteri(dimension, GL_TEXTURE_MIN_FILTER, GL_NEAREST)");
                glTexParameteri(This->glDescription.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                checkGLcall("glTexParameteri(dimension, GL_TEXTURE_MAG_FILTER, GL_NEAREST)");
            }
            /* This is where we should be reducing the amount of GLMemoryUsed */
        } else if (This->glDescription.textureName) {
            /* Mipmap surfaces should have a base texture container */
            ERR("Mipmap surface has a glTexture bound to it!\n");
        }

        glBindTexture(This->glDescription.target, This->glDescription.textureName);
        checkGLcall("glBindTexture");

        LEAVE_GL();
    }
    return;
}

#include <errno.h>
#include <stdio.h>
static HRESULT WINAPI IWineD3DSurfaceImpl_SaveSnapshot(IWineD3DSurface *iface, const char* filename)
{
    FILE* f = NULL;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    char *allocatedMemory;
    const char *textureRow;
    IWineD3DSwapChain *swapChain = NULL;
    int width, height, i, y;
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
        checkGLcall("glGetIntegerv");
        glReadBuffer(swapChain ? GL_BACK : This->resource.wineD3DDevice->offscreenBuffer);
        checkGLcall("glReadBuffer");
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
        surface_bind_and_dirtify(This);
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
            color = *((const DWORD*)textureRow);
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

static HRESULT WINAPI IWineD3DSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    HRESULT hr;
    const struct GlPixelFormatDesc *glDesc;
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

static HRESULT WINAPI IWineD3DSurfaceImpl_SetMem(IWineD3DSurface *iface, void *Mem) {
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
        /* LockRect and GetDC will re-create the dib section and allocated memory */
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

void flip_surface(IWineD3DSurfaceImpl *front, IWineD3DSurfaceImpl *back) {

    /* Flip the surface contents */
    /* Flip the DC */
    {
        HDC tmp;
        tmp = front->hDC;
        front->hDC = back->hDC;
        back->hDC = tmp;
    }

    /* Flip the DIBsection */
    {
        HBITMAP tmp;
        BOOL hasDib = front->Flags & SFLAG_DIBSECTION;
        tmp = front->dib.DIBsection;
        front->dib.DIBsection = back->dib.DIBsection;
        back->dib.DIBsection = tmp;

        if(back->Flags & SFLAG_DIBSECTION) front->Flags |= SFLAG_DIBSECTION;
        else front->Flags &= ~SFLAG_DIBSECTION;
        if(hasDib) back->Flags |= SFLAG_DIBSECTION;
        else back->Flags &= ~SFLAG_DIBSECTION;
    }

    /* Flip the surface data */
    {
        void* tmp;

        tmp = front->dib.bitmap_data;
        front->dib.bitmap_data = back->dib.bitmap_data;
        back->dib.bitmap_data = tmp;

        tmp = front->resource.allocatedMemory;
        front->resource.allocatedMemory = back->resource.allocatedMemory;
        back->resource.allocatedMemory = tmp;

        tmp = front->resource.heapMemory;
        front->resource.heapMemory = back->resource.heapMemory;
        back->resource.heapMemory = tmp;
    }

    /* Flip the PBO */
    {
        GLuint tmp_pbo = front->pbo;
        front->pbo = back->pbo;
        back->pbo = tmp_pbo;
    }

    /* client_memory should not be different, but just in case */
    {
        BOOL tmp;
        tmp = front->dib.client_memory;
        front->dib.client_memory = back->dib.client_memory;
        back->dib.client_memory = tmp;
    }

    /* Flip the opengl texture */
    {
        glDescriptor tmp_desc = back->glDescription;
        back->glDescription = front->glDescription;
        front->glDescription = tmp_desc;
    }

    {
        DWORD tmp_flags = back->Flags;
        back->Flags = front->Flags;
        front->Flags = tmp_flags;
    }
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Flip(IWineD3DSurface *iface, IWineD3DSurface *override, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSwapChainImpl *swapchain = NULL;
    HRESULT hr;
    TRACE("(%p)->(%p,%x)\n", This, override, Flags);

    /* Flipping is only supported on RenderTargets and overlays*/
    if( !(This->resource.usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_OVERLAY)) ) {
        WARN("Tried to flip a non-render target, non-overlay surface\n");
        return WINEDDERR_NOTFLIPPABLE;
    }

    if(This->resource.usage & WINED3DUSAGE_OVERLAY) {
        flip_surface(This, (IWineD3DSurfaceImpl *) override);

        /* Update the overlay if it is visible */
        if(This->overlay_dest) {
            return IWineD3DSurface_DrawOverlay((IWineD3DSurface *) This);
        } else {
            return WINED3D_OK;
        }
    }

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
static inline void fb_copy_to_texture_direct(IWineD3DSurfaceImpl *This, IWineD3DSurface *SrcSurface,
        IWineD3DSwapChainImpl *swapchain, const WINED3DRECT *srect, const WINED3DRECT *drect,
        BOOL upsidedown, WINED3DTEXTUREFILTERTYPE Filter)
{
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    float xrel, yrel;
    UINT row;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;


    ActivateContext(myDevice, SrcSurface, CTXUSAGE_BLIT);
    IWineD3DSurface_PreLoad((IWineD3DSurface *) This);
    ENTER_GL();

    /* Bind the target texture */
    glBindTexture(This->glDescription.target, This->glDescription.textureName);
    checkGLcall("glBindTexture");
    if(!swapchain) {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
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
    checkGLcall("glCopyTexSubImage2D");

    LEAVE_GL();
}

/* Uses the hardware to stretch and flip the image */
static inline void fb_copy_to_texture_hwstretch(IWineD3DSurfaceImpl *This, IWineD3DSurface *SrcSurface,
        IWineD3DSwapChainImpl *swapchain, const WINED3DRECT *srect, const WINED3DRECT *drect,
        BOOL upsidedown, WINED3DTEXTUREFILTERTYPE Filter)
{
    GLuint src, backup = 0;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    float left, right, top, bottom; /* Texture coordinates */
    UINT fbwidth = Src->currentDesc.Width;
    UINT fbheight = Src->currentDesc.Height;
    GLenum drawBuffer = GL_BACK;
    GLenum texture_target;
    BOOL noBackBufferBackup;

    TRACE("Using hwstretch blit\n");
    /* Activate the Proper context for reading from the source surface, set it up for blitting */
    ActivateContext(myDevice, SrcSurface, CTXUSAGE_BLIT);
    IWineD3DSurface_PreLoad((IWineD3DSurface *) This);

    noBackBufferBackup = !swapchain && wined3d_settings.offscreen_rendering_mode == ORM_FBO;
    if(!noBackBufferBackup && Src->glDescription.textureName == 0) {
        /* Get it a description */
        IWineD3DSurface_PreLoad(SrcSurface);
    }
    ENTER_GL();

    /* Try to use an aux buffer for drawing the rectangle. This way it doesn't need restoring.
     * This way we don't have to wait for the 2nd readback to finish to leave this function.
     */
    if(myDevice->activeContext->aux_buffers >= 2) {
        /* Got more than one aux buffer? Use the 2nd aux buffer */
        drawBuffer = GL_AUX1;
    } else if((swapchain || myDevice->offscreenBuffer == GL_BACK) && myDevice->activeContext->aux_buffers >= 1) {
        /* Only one aux buffer, but it isn't used (Onscreen rendering, or non-aux orm)? Use it! */
        drawBuffer = GL_AUX0;
    }

    if(noBackBufferBackup) {
        glGenTextures(1, &backup);
        checkGLcall("glGenTextures\n");
        glBindTexture(GL_TEXTURE_2D, backup);
        checkGLcall("glBindTexture(Src->glDescription.target, Src->glDescription.textureName)");
        texture_target = GL_TEXTURE_2D;
    } else {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        texture_target = Src->glDescription.target;
        glBindTexture(texture_target, Src->glDescription.textureName);
        checkGLcall("glBindTexture(texture_target, Src->glDescription.textureName)");
        glEnable(texture_target);
        checkGLcall("glEnable(texture_target)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        Src->Flags &= ~SFLAG_INTEXTURE;
    }

    if(swapchain) {
        glReadBuffer(surface_get_gl_buffer(SrcSurface, (IWineD3DSwapChain *)swapchain));
    } else {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        glReadBuffer(myDevice->offscreenBuffer);
    }

    /* TODO: Only back up the part that will be overwritten */
    glCopyTexSubImage2D(texture_target, 0,
                        0, 0 /* read offsets */,
                        0, 0,
                        fbwidth,
                        fbheight);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER,
                    magLookup[Filter - WINED3DTEXF_NONE]);
    checkGLcall("glTexParameteri");
    glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER,
                    minMipLookup[Filter].mip[WINED3DTEXF_NONE]);
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

        if(texture_target != GL_TEXTURE_2D) {
            glDisable(texture_target);
            glEnable(GL_TEXTURE_2D);
            texture_target = GL_TEXTURE_2D;
        }
    }
    checkGLcall("glEnd and previous");

    left = srect->x1;
    right = srect->x2;

    if(upsidedown) {
        top = Src->currentDesc.Height - srect->y1;
        bottom = Src->currentDesc.Height - srect->y2;
    } else {
        top = Src->currentDesc.Height - srect->y2;
        bottom = Src->currentDesc.Height - srect->y1;
    }

    if(Src->Flags & SFLAG_NORMCOORD) {
        left /= Src->pow2Width;
        right /= Src->pow2Width;
        top /= Src->pow2Height;
        bottom /= Src->pow2Height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP);

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

    if(texture_target != This->glDescription.target) {
        glDisable(texture_target);
        glEnable(This->glDescription.target);
        texture_target = This->glDescription.target;
    }

    /* Now read the stretched and upside down image into the destination texture */
    glBindTexture(texture_target, This->glDescription.textureName);
    checkGLcall("glBindTexture");
    glCopyTexSubImage2D(texture_target,
                        0,
                        drect->x1, drect->y1, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        drect->x2 - drect->x1, drect->y2 - drect->y1);
    checkGLcall("glCopyTexSubImage2D");

    if(drawBuffer == GL_BACK) {
        /* Write the back buffer backup back */
        if(backup) {
            if(texture_target != GL_TEXTURE_2D) {
                glDisable(texture_target);
                glEnable(GL_TEXTURE_2D);
                texture_target = GL_TEXTURE_2D;
            }
            glBindTexture(GL_TEXTURE_2D, backup);
            checkGLcall("glBindTexture(GL_TEXTURE_2D, backup)");
        } else {
            if(texture_target != Src->glDescription.target) {
                glDisable(texture_target);
                glEnable(Src->glDescription.target);
                texture_target = Src->glDescription.target;
            }
            glBindTexture(Src->glDescription.target, Src->glDescription.textureName);
            checkGLcall("glBindTexture(Src->glDescription.target, Src->glDescription.textureName)");
        }

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
    glDisable(texture_target);
    checkGLcall("glDisable(texture_target)");

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
static HRESULT IWineD3DSurfaceImpl_BltOverride(IWineD3DSurfaceImpl *This, const RECT *DestRect,
        IWineD3DSurface *SrcSurface, const RECT *SrcRect, DWORD Flags, const WINEDDBLTFX *DDBltFx,
        WINED3DTEXTUREFILTERTYPE Filter)
{
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
    } else if(dstSwapchain && dstSwapchain == srcSwapchain) {
        FIXME("Implement hardware blit between two surfaces on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    } else if(dstSwapchain && srcSwapchain) {
        FIXME("Implement hardware blit between two different swapchains\n");
        return WINED3DERR_INVALIDCALL;
    } else if(dstSwapchain) {
        if(SrcSurface == myDevice->render_targets[0]) {
            TRACE("Blit from active render target to a swapchain\n");
            /* Handled with regular texture -> swapchain blit */
        }
    } else if(srcSwapchain && This == (IWineD3DSurfaceImpl *) myDevice->render_targets[0]) {
        FIXME("Implement blit from a swapchain to the active render target\n");
        return WINED3DERR_INVALIDCALL;
    }

    if((srcSwapchain || SrcSurface == myDevice->render_targets[0]) && !dstSwapchain) {
        /* Blit from render target to texture */
        WINED3DRECT srect;
        BOOL upsideDown, stretchx;
        BOOL paletteOverride = FALSE;

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

        if(rect.x2 - rect.x1 != srect.x2 - srect.x1) {
            stretchx = TRUE;
        } else {
            stretchx = FALSE;
        }

        /* When blitting from a render target a texture, the texture isn't required to have a palette.
         * In this case grab the palette from the render target. */
        if((This->resource.format == WINED3DFMT_P8) && (This->palette == NULL)) {
            paletteOverride = TRUE;
            TRACE("Source surface (%p) lacks palette, overriding palette with palette %p of destination surface (%p)\n", Src, This->palette, This);
            This->palette = Src->palette;
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

        /* Clear the palette as the surface didn't have a palette attached, it would confuse GetPalette and other calls */
        if(paletteOverride)
            This->palette = NULL;

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
        WINEDDCOLORKEY oldBltCKey = Src->SrcBltCKey;
        RECT SourceRectangle;
        BOOL paletteOverride = FALSE;

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

        /* When blitting from an offscreen surface to a rendertarget, the source
         * surface is not required to have a palette. Our rendering / conversion
         * code further down the road retrieves the palette from the surface, so
         * it must have a palette set. */
        if((Src->resource.format == WINED3DFMT_P8) && (Src->palette == NULL)) {
            paletteOverride = TRUE;
            TRACE("Source surface (%p) lacks palette, overriding palette with palette %p of destination surface (%p)\n", Src, This->palette, This);
            Src->palette = This->palette;
        }

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && GL_SUPPORT(EXT_FRAMEBUFFER_BLIT) &&
            (Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE)) == 0) {
            TRACE("Using stretch_rect_fbo\n");
            /* The source is always a texture, but never the currently active render target, and the texture
             * contents are never upside down
             */
            stretch_rect_fbo((IWineD3DDevice *)myDevice, SrcSurface, (WINED3DRECT *) &SourceRectangle,
                              (IWineD3DSurface *)This, &rect, Filter, FALSE);

            /* Clear the palette as the surface didn't have a palette attached, it would confuse GetPalette and other calls */
            if(paletteOverride)
                Src->palette = NULL;
            return WINED3D_OK;
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
            Src->SrcBltCKey = DDBltFx->ddckSrcColorkey;
        } else {
            /* Do not use color key */
            Src->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
        }

        /* Now load the surface */
        IWineD3DSurface_PreLoad((IWineD3DSurface *) Src);

        /* Activate the destination context, set it up for blitting */
        ActivateContext(myDevice, (IWineD3DSurface *) This, CTXUSAGE_BLIT);

        /* The coordinates of the ddraw front buffer are always fullscreen ('screen coordinates',
         * while OpenGL coordinates are window relative.
         * Also beware of the origin difference(top left vs bottom left).
         * Also beware that the front buffer's surface size is screen width x screen height,
         * whereas the real gl drawable size is the size of the window.
         */
        if (dstSwapchain && (IWineD3DSurface *)This == dstSwapchain->frontBuffer) {
            RECT windowsize;
            POINT offset = {0,0};
            UINT h;
            ClientToScreen(dstSwapchain->win_handle, &offset);
            GetClientRect(dstSwapchain->win_handle, &windowsize);
            h = windowsize.bottom - windowsize.top;
            rect.x1 -= offset.x; rect.x2 -=offset.x;
            rect.y1 -= offset.y; rect.y2 -=offset.y;
            rect.y1 += This->currentDesc.Height - h; rect.y2 += This->currentDesc.Height - h;
        }

        myDevice->blitter->set_shader((IWineD3DDevice *) myDevice, Src->resource.format,
                                       Src->glDescription.target, Src->pow2Width, Src->pow2Height);

        ENTER_GL();

        /* Bind the texture */
        glBindTexture(Src->glDescription.target, Src->glDescription.textureName);
        checkGLcall("glBindTexture");

        /* Filtering for StretchRect */
        glTexParameteri(Src->glDescription.target, GL_TEXTURE_MAG_FILTER,
                        magLookup[Filter - WINED3DTEXF_NONE]);
        checkGLcall("glTexParameteri");
        glTexParameteri(Src->glDescription.target, GL_TEXTURE_MIN_FILTER,
                        minMipLookup[Filter].mip[WINED3DTEXF_NONE]);
        checkGLcall("glTexParameteri");
        glTexParameteri(Src->glDescription.target, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(Src->glDescription.target, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        checkGLcall("glTexEnvi");

        /* This is for color keying */
        if(Flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE)) {
            glEnable(GL_ALPHA_TEST);
            checkGLcall("glEnable GL_ALPHA_TEST");

            /* When the primary render target uses P8, the alpha component contains the palette index.
             * Which means that the colorkey is one of the palette entries. In other cases pixels that
             * should be masked away have alpha set to 0. */
            if(primary_render_target_is_p8(myDevice))
                glAlphaFunc(GL_NOTEQUAL, (float)Src->SrcBltCKey.dwColorSpaceLowValue / 256.0);
            else
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

        glBindTexture(Src->glDescription.target, 0);
        checkGLcall("glBindTexture(Src->glDescription.target, 0)");

        /* Restore the color key parameters */
        Src->CKeyFlags = oldCKeyFlags;
        Src->SrcBltCKey = oldBltCKey;

        /* Clear the palette as the surface didn't have a palette attached, it would confuse GetPalette and other calls */
        if(paletteOverride)
            Src->palette = NULL;

        LEAVE_GL();

        /* Leave the opengl state valid for blitting */
        myDevice->blitter->unset_shader((IWineD3DDevice *) myDevice);

        /* Flush in case the drawable is used by multiple GL contexts */
        if(dstSwapchain && (This == (IWineD3DSurfaceImpl *) dstSwapchain->frontBuffer || dstSwapchain->num_contexts >= 2))
            glFlush();

        /* TODO: If the surface is locked often, perform the Blt in software on the memory instead */
        /* The surface is now in the drawable. On onscreen surfaces or without fbos the texture
         * is outdated now
         */
        IWineD3DSurface_ModifyLocation((IWineD3DSurface *) This, SFLAG_INDRAWABLE, TRUE);

        return WINED3D_OK;
    } else {
        /* Source-Less Blit to render target */
        if (Flags & WINEDDBLT_COLORFILL) {
            /* This is easy to handle for the D3D Device... */
            DWORD color;

            TRACE("Colorfill\n");

            /* This == (IWineD3DSurfaceImpl *) myDevice->render_targets[0] || dstSwapchain
                must be true if we are here */
            if (This != (IWineD3DSurfaceImpl *) myDevice->render_targets[0] &&
                    !(This == (IWineD3DSurfaceImpl*) dstSwapchain->frontBuffer ||
                      (dstSwapchain->backBuffer && This == (IWineD3DSurfaceImpl*) dstSwapchain->backBuffer[0]))) {
                TRACE("Surface is higher back buffer, falling back to software\n");
                return WINED3DERR_INVALIDCALL;
            }

            /* The color as given in the Blt function is in the format of the frame-buffer...
             * 'clear' expect it in ARGB format => we need to do some conversion :-)
             */
            if (This->resource.format == WINED3DFMT_P8) {
                DWORD alpha;

                if (primary_render_target_is_p8(myDevice)) alpha = DDBltFx->u5.dwFillColor << 24;
                else alpha = 0xFF000000;

                if (This->palette) {
                    color = (alpha |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peRed << 16) |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peGreen << 8) |
                            (This->palette->palents[DDBltFx->u5.dwFillColor].peBlue));
                } else {
                    color = alpha;
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

            TRACE("(%p) executing Render Target override, color = %x\n", This, color);
            IWineD3DDeviceImpl_ClearSurface(myDevice, This,
                                            1, /* Number of rectangles */
                                            &rect, WINED3DCLEAR_TARGET, color,
                                            0.0 /* Z */,
                                            0 /* Stencil */);
            return WINED3D_OK;
        }
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

static HRESULT IWineD3DSurfaceImpl_BltZ(IWineD3DSurfaceImpl *This, const RECT *DestRect,
        IWineD3DSurface *SrcSurface, const RECT *SrcRect, DWORD Flags, const WINEDDBLTFX *DDBltFx)
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
                                    (const WINED3DRECT *)DestRect,
                                    WINED3DCLEAR_ZBUFFER,
                                    0x00000000,
                                    depth,
                                    0x00000000);
    }

    FIXME("(%p): Unsupp depthstencil blit\n", This);
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Blt(IWineD3DSurface *iface, const RECT *DestRect, IWineD3DSurface *SrcSurface,
        const RECT *SrcRect, DWORD Flags, const WINEDDBLTFX *DDBltFx, WINED3DTEXTUREFILTERTYPE Filter) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    TRACE("(%p)->(%p,%p,%p,%x,%p)\n", This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);
    TRACE("(%p): Usage is %s\n", This, debug_d3dusage(This->resource.usage));

    if ( (This->Flags & SFLAG_LOCKED) || ((Src != NULL) && (Src->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

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

static HRESULT WINAPI IWineD3DSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty,
        IWineD3DSurface *Source, const RECT *rsrc, DWORD trans)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *srcImpl = (IWineD3DSurfaceImpl *) Source;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    TRACE("(%p)->(%d, %d, %p, %p, %08x\n", iface, dstx, dsty, Source, rsrc, trans);

    if ( (This->Flags & SFLAG_LOCKED) || ((srcImpl != NULL) && (srcImpl->Flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

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

static HRESULT WINAPI IWineD3DSurfaceImpl_RealizePalette(IWineD3DSurface *iface)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    RGBQUAD col[256];
    IWineD3DPaletteImpl *pal = This->palette;
    unsigned int n;
    TRACE("(%p)\n", This);

    if (!pal) return WINED3D_OK;

    if(This->resource.format == WINED3DFMT_P8 ||
       This->resource.format == WINED3DFMT_A8P8)
    {
        int bpp;
        GLenum format, internal, type;
        CONVERT_TYPES convert;

        /* Check if we are using a RTL mode which uses texturing for uploads */
        BOOL use_texture = (wined3d_settings.rendertargetlock_mode == RTL_READTEX || wined3d_settings.rendertargetlock_mode == RTL_TEXTEX);

        /* Check if we have hardware palette conversion if we have convert is set to NO_CONVERSION */
        d3dfmt_get_conv(This, TRUE, use_texture, &format, &internal, &type, &convert, &bpp, This->srgb);

        if((This->resource.usage & WINED3DUSAGE_RENDERTARGET) && (convert == NO_CONVERSION))
        {
            /* Make sure the texture is up to date. This call doesn't do anything if the texture is already up to date. */
            IWineD3DSurface_LoadLocation(iface, SFLAG_INTEXTURE, NULL);

            /* We want to force a palette refresh, so mark the drawable as not being up to date */
            IWineD3DSurface_ModifyLocation(iface, SFLAG_INDRAWABLE, FALSE);

            /* Re-upload the palette */
            d3dfmt_p8_upload_palette(iface, convert);
        } else {
            if(!(This->Flags & SFLAG_INSYSMEM)) {
                TRACE("Palette changed with surface that does not have an up to date system memory copy\n");
                IWineD3DSurface_LoadLocation(iface, SFLAG_INSYSMEM, NULL);
            }
            TRACE("Dirtifying surface\n");
            IWineD3DSurface_ModifyLocation(iface, SFLAG_INSYSMEM, TRUE);
        }
    }

    if(This->Flags & SFLAG_DIBSECTION) {
        TRACE("(%p): Updating the hdc's palette\n", This);
        for (n=0; n<256; n++) {
            col[n].rgbRed   = pal->palents[n].peRed;
            col[n].rgbGreen = pal->palents[n].peGreen;
            col[n].rgbBlue  = pal->palents[n].peBlue;
            col[n].rgbReserved = 0;
        }
        SetDIBColorTable(This->hDC, 0, 256, col);
    }

    /* Propagate the changes to the drawable when we have a palette. */
    if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
        IWineD3DSurface_LoadLocation(iface, SFLAG_INDRAWABLE, NULL);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_PrivateSetup(IWineD3DSurface *iface) {
    /** Check against the maximum texture sizes supported by the video card **/
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    unsigned int pow2Width, pow2Height;
    const struct GlPixelFormatDesc *glDesc;

    getFormatDescEntry(This->resource.format, &GLINFO_LOCATION, &glDesc);
    /* Setup some glformat defaults */
    This->glDescription.glFormat         = glDesc->glFormat;
    This->glDescription.glFormatInternal = glDesc->glInternal;
    This->glDescription.glType           = glDesc->glType;

    This->glDescription.textureName      = 0;
    This->glDescription.target           = GL_TEXTURE_2D;

    /* Non-power2 support */
    if (GL_SUPPORT(ARB_TEXTURE_NON_POWER_OF_TWO) || GL_SUPPORT(WINE_NORMALIZED_TEXRECT)) {
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
            || Format == WINED3DFMT_DXT4 || Format == WINED3DFMT_DXT5
            || This->resource.format == WINED3DFMT_ATI2N) {
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
        WARN("(%p) Creating an oversized surface: %ux%u (texture is %ux%u)\n",
             This, This->pow2Width, This->pow2Height, This->currentDesc.Width, This->currentDesc.Height);
        This->Flags |= SFLAG_OVERSIZE;

        /* This will be initialized on the first blt */
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = 0;
        This->glRect.bottom = 0;
    } else {
        /* Check this after the oversize check - do not make an oversized surface a texture_rectangle one.
           Second also don't use ARB_TEXTURE_RECTANGLE in case the surface format is P8 and EXT_PALETTED_TEXTURE
           is used in combination with texture uploads (RTL_READTEX/RTL_TEXTEX). The reason is that EXT_PALETTED_TEXTURE
           doesn't work in combination with ARB_TEXTURE_RECTANGLE.
        */
        if(This->Flags & SFLAG_NONPOW2 && GL_SUPPORT(ARB_TEXTURE_RECTANGLE) &&
           !((This->resource.format == WINED3DFMT_P8) && GL_SUPPORT(EXT_PALETTED_TEXTURE) && (wined3d_settings.rendertargetlock_mode == RTL_READTEX || wined3d_settings.rendertargetlock_mode == RTL_TEXTEX)))
        {
            This->glDescription.target = GL_TEXTURE_RECTANGLE_ARB;
            This->pow2Width  = This->currentDesc.Width;
            This->pow2Height = This->currentDesc.Height;
            This->Flags &= ~(SFLAG_NONPOW2 | SFLAG_NORMCOORD);
        }

        /* No oversize, gl rect is the full texture size */
        This->Flags &= ~SFLAG_OVERSIZE;
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = This->pow2Width;
        This->glRect.bottom = This->pow2Height;
    }

    if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        switch(wined3d_settings.offscreen_rendering_mode) {
            case ORM_FBO:        This->get_drawable_size = get_drawable_size_fbo;        break;
            case ORM_PBUFFER:    This->get_drawable_size = get_drawable_size_pbuffer;    break;
            case ORM_BACKBUFFER: This->get_drawable_size = get_drawable_size_backbuffer; break;
        }
    }

    This->Flags |= SFLAG_INSYSMEM;

    return WINED3D_OK;
}

struct depth_blt_info
{
    GLenum binding;
    GLenum bind_target;
    enum tex_types tex_type;
    GLfloat coords[4][3];
};

static void surface_get_depth_blt_info(GLenum target, GLsizei w, GLsizei h, struct depth_blt_info *info)
{
    GLfloat (*coords)[3] = info->coords;

    switch (target)
    {
        default:
            FIXME("Unsupported texture target %#x\n", target);
            /* Fall back to GL_TEXTURE_2D */
        case GL_TEXTURE_2D:
            info->binding = GL_TEXTURE_BINDING_2D;
            info->bind_target = GL_TEXTURE_2D;
            info->tex_type = tex_2d;
            coords[0][0] = 0.0f;    coords[0][1] = 1.0f;    coords[0][2] = 0.0f;
            coords[1][0] = 1.0f;    coords[1][1] = 1.0f;    coords[1][2] = 0.0f;
            coords[2][0] = 0.0f;    coords[2][1] = 0.0f;    coords[2][2] = 0.0f;
            coords[3][0] = 1.0f;    coords[3][1] = 0.0f;    coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            info->binding = GL_TEXTURE_BINDING_RECTANGLE_ARB;
            info->bind_target = GL_TEXTURE_RECTANGLE_ARB;
            info->tex_type = tex_rect;
            coords[0][0] = 0.0f;    coords[0][1] = h;       coords[0][2] = 0.0f;
            coords[1][0] = w;       coords[1][1] = h;       coords[1][2] = 0.0f;
            coords[2][0] = 0.0f;    coords[2][1] = 0.0f;    coords[2][2] = 0.0f;
            coords[3][0] = w;       coords[3][1] = 0.0f;    coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] =  1.0f;   coords[0][1] = -1.0f;   coords[0][2] =  1.0f;
            coords[1][0] =  1.0f;   coords[1][1] = -1.0f;   coords[1][2] = -1.0f;
            coords[2][0] =  1.0f;   coords[2][1] =  1.0f;   coords[2][2] =  1.0f;
            coords[3][0] =  1.0f;   coords[3][1] =  1.0f;   coords[3][2] = -1.0f;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] = -1.0f;   coords[0][1] = -1.0f;   coords[0][2] = -1.0f;
            coords[1][0] = -1.0f;   coords[1][1] = -1.0f;   coords[1][2] =  1.0f;
            coords[2][0] = -1.0f;   coords[2][1] =  1.0f;   coords[2][2] = -1.0f;
            coords[3][0] = -1.0f;   coords[3][1] =  1.0f;   coords[3][2] =  1.0f;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] = -1.0f;   coords[0][1] =  1.0f;   coords[0][2] =  1.0f;
            coords[1][0] =  1.0f;   coords[1][1] =  1.0f;   coords[1][2] =  1.0f;
            coords[2][0] = -1.0f;   coords[2][1] =  1.0f;   coords[2][2] = -1.0f;
            coords[3][0] =  1.0f;   coords[3][1] =  1.0f;   coords[3][2] = -1.0f;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] = -1.0f;   coords[0][1] = -1.0f;   coords[0][2] = -1.0f;
            coords[1][0] =  1.0f;   coords[1][1] = -1.0f;   coords[1][2] = -1.0f;
            coords[2][0] = -1.0f;   coords[2][1] = -1.0f;   coords[2][2] =  1.0f;
            coords[3][0] =  1.0f;   coords[3][1] = -1.0f;   coords[3][2] =  1.0f;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] = -1.0f;   coords[0][1] = -1.0f;   coords[0][2] =  1.0f;
            coords[1][0] =  1.0f;   coords[1][1] = -1.0f;   coords[1][2] =  1.0f;
            coords[2][0] = -1.0f;   coords[2][1] =  1.0f;   coords[2][2] =  1.0f;
            coords[3][0] =  1.0f;   coords[3][1] =  1.0f;   coords[3][2] =  1.0f;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            coords[0][0] =  1.0f;   coords[0][1] = -1.0f;   coords[0][2] = -1.0f;
            coords[1][0] = -1.0f;   coords[1][1] = -1.0f;   coords[1][2] = -1.0f;
            coords[2][0] =  1.0f;   coords[2][1] =  1.0f;   coords[2][2] = -1.0f;
            coords[3][0] = -1.0f;   coords[3][1] =  1.0f;   coords[3][2] = -1.0f;
    }
}

static void surface_depth_blt(IWineD3DSurfaceImpl *This, GLuint texture, GLsizei w, GLsizei h, GLenum target)
{
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    struct depth_blt_info info;
    GLint old_binding = 0;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ZERO, GL_ONE);
    glViewport(0, 0, w, h);

    surface_get_depth_blt_info(target, w, h, &info);
    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
    glGetIntegerv(info.binding, &old_binding);
    glBindTexture(info.bind_target, texture);

    device->shader_backend->shader_select_depth_blt((IWineD3DDevice *)device, info.tex_type);

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord3fv(info.coords[0]);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord3fv(info.coords[1]);
    glVertex2f(1.0f, -1.0f);
    glTexCoord3fv(info.coords[2]);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord3fv(info.coords[3]);
    glVertex2f(1.0f, 1.0f);
    glEnd();

    glBindTexture(info.bind_target, old_binding);

    glPopAttrib();

    device->shader_backend->shader_deselect_depth_blt((IWineD3DDevice *)device);
}

void surface_modify_ds_location(IWineD3DSurface *iface, DWORD location) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) New location %#x\n", This, location);

    if (location & ~SFLAG_DS_LOCATIONS) {
        FIXME("(%p) Invalid location (%#x) specified\n", This, location);
    }

    This->Flags &= ~SFLAG_DS_LOCATIONS;
    This->Flags |= location;
}

void surface_load_ds_location(IWineD3DSurface *iface, DWORD location) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("(%p) New location %#x\n", This, location);

    /* TODO: Make this work for modes other than FBO */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return;

    if (This->Flags & location) {
        TRACE("(%p) Location (%#x) is already up to date\n", This, location);
        return;
    }

    if (This->current_renderbuffer) {
        FIXME("(%p) Not supported with fixed up depth stencil\n", This);
        return;
    }

    if (location == SFLAG_DS_OFFSCREEN) {
        if (This->Flags & SFLAG_DS_ONSCREEN) {
            GLint old_binding = 0;
            GLenum bind_target;

            TRACE("(%p) Copying onscreen depth buffer to depth texture\n", This);

            ENTER_GL();

            if (!device->depth_blt_texture) {
                glGenTextures(1, &device->depth_blt_texture);
            }

            /* Note that we use depth_blt here as well, rather than glCopyTexImage2D
             * directly on the FBO texture. That's because we need to flip. */
            GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
            if (This->glDescription.target == GL_TEXTURE_RECTANGLE_ARB) {
                glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &old_binding);
                bind_target = GL_TEXTURE_RECTANGLE_ARB;
            } else {
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
                bind_target = GL_TEXTURE_2D;
            }
            glBindTexture(bind_target, device->depth_blt_texture);
            glCopyTexImage2D(bind_target,
                    This->glDescription.level,
                    This->glDescription.glFormatInternal,
                    0,
                    0,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    0);
            glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(bind_target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            glBindTexture(bind_target, old_binding);

            /* Setup the destination */
            if (!device->depth_blt_rb) {
                GL_EXTCALL(glGenRenderbuffersEXT(1, &device->depth_blt_rb));
                checkGLcall("glGenRenderbuffersEXT");
            }
            if (device->depth_blt_rb_w != This->currentDesc.Width
                    || device->depth_blt_rb_h != This->currentDesc.Height) {
                GL_EXTCALL(glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, device->depth_blt_rb));
                checkGLcall("glBindRenderbufferEXT");
                GL_EXTCALL(glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, This->currentDesc.Width, This->currentDesc.Height));
                checkGLcall("glRenderbufferStorageEXT");
                device->depth_blt_rb_w = This->currentDesc.Width;
                device->depth_blt_rb_h = This->currentDesc.Height;
            }

            context_bind_fbo((IWineD3DDevice *)device, GL_FRAMEBUFFER_EXT, &device->activeContext->dst_fbo);
            GL_EXTCALL(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, device->depth_blt_rb));
            checkGLcall("glFramebufferRenderbufferEXT");
            context_attach_depth_stencil_fbo(device, GL_FRAMEBUFFER_EXT, iface, FALSE);

            /* Do the actual blit */
            surface_depth_blt(This, device->depth_blt_texture, This->currentDesc.Width, This->currentDesc.Height, bind_target);
            checkGLcall("depth_blt");

            if (device->activeContext->current_fbo) {
                context_bind_fbo((IWineD3DDevice *)device, GL_FRAMEBUFFER_EXT, &device->activeContext->current_fbo->id);
            } else {
                GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
                checkGLcall("glBindFramebuffer()");
            }

            LEAVE_GL();
        } else {
            FIXME("No up to date depth stencil location\n");
        }
    } else if (location == SFLAG_DS_ONSCREEN) {
        if (This->Flags & SFLAG_DS_OFFSCREEN) {
            TRACE("(%p) Copying depth texture to onscreen depth buffer\n", This);

            ENTER_GL();

            GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
            checkGLcall("glBindFramebuffer()");
            surface_depth_blt(This, This->glDescription.textureName, This->currentDesc.Width, This->currentDesc.Height, This->glDescription.target);
            checkGLcall("depth_blt");

            if (device->activeContext->current_fbo) {
                GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, device->activeContext->current_fbo->id));
                checkGLcall("glBindFramebuffer()");
            }

            LEAVE_GL();
        } else {
            FIXME("No up to date depth stencil location\n");
        }
    } else {
        ERR("(%p) Invalid location (%#x) specified\n", This, location);
    }

    This->Flags |= location;
}

static void WINAPI IWineD3DSurfaceImpl_ModifyLocation(IWineD3DSurface *iface, DWORD flag, BOOL persistent) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DBaseTexture *texture;
    IWineD3DSurfaceImpl *overlay;

    TRACE("(%p)->(%s, %s)\n", iface,
          flag == SFLAG_INSYSMEM ? "SFLAG_INSYSMEM" : flag == SFLAG_INDRAWABLE ? "SFLAG_INDRAWABLE" : "SFLAG_INTEXTURE",
          persistent ? "TRUE" : "FALSE");

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        IWineD3DSwapChain *swapchain = NULL;

        if (SUCCEEDED(IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
            TRACE("Surface %p is an onscreen surface\n", iface);

            IWineD3DSwapChain_Release(swapchain);
        } else {
            /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets. */
            if (flag & (SFLAG_INTEXTURE | SFLAG_INDRAWABLE)) flag |= (SFLAG_INTEXTURE | SFLAG_INDRAWABLE);
        }
    }

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

        /* Redraw emulated overlays, if any */
        if(flag & SFLAG_INDRAWABLE && !list_empty(&This->overlays)) {
            LIST_FOR_EACH_ENTRY(overlay, &This->overlays, IWineD3DSurfaceImpl, overlay_entry) {
                IWineD3DSurface_DrawOverlay((IWineD3DSurface *) overlay);
            }
        }
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
    GLfloat x, y, z;
};

struct float_rect
{
    float l;
    float t;
    float r;
    float b;
};

static inline void cube_coords_float(const RECT *r, UINT w, UINT h, struct float_rect *f)
{
    f->l = ((r->left * 2.0f) / w) - 1.0f;
    f->t = ((r->top * 2.0f) / h) - 1.0f;
    f->r = ((r->right * 2.0f) / w) - 1.0f;
    f->b = ((r->bottom * 2.0f) / h) - 1.0f;
}

static inline void surface_blt_to_drawable(IWineD3DSurfaceImpl *This, const RECT *rect_in) {
    struct coords coords[4];
    RECT rect;
    IWineD3DSwapChain *swapchain;
    IWineD3DBaseTexture *texture;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    GLenum bind_target;
    struct float_rect f;

    if(rect_in) {
        rect = *rect_in;
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.right = This->currentDesc.Width;
        rect.bottom = This->currentDesc.Height;
    }

    switch(This->glDescription.target)
    {
        case GL_TEXTURE_2D:
            bind_target = GL_TEXTURE_2D;

            coords[0].x = (float)rect.left / This->pow2Width;
            coords[0].y = (float)rect.top / This->pow2Height;
            coords[0].z = 0;

            coords[1].x = (float)rect.left / This->pow2Width;
            coords[1].y = (float)rect.bottom / This->pow2Height;
            coords[1].z = 0;

            coords[2].x = (float)rect.right / This->pow2Width;
            coords[2].y = (float)rect.bottom / This->pow2Height;
            coords[2].z = 0;

            coords[3].x = (float)rect.right / This->pow2Width;
            coords[3].y = (float)rect.top / This->pow2Height;
            coords[3].z = 0;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            bind_target = GL_TEXTURE_RECTANGLE_ARB;
            coords[0].x = rect.left;    coords[0].y = rect.top;     coords[0].z = 0;
            coords[1].x = rect.left;    coords[1].y = rect.bottom;  coords[1].z = 0;
            coords[2].x = rect.right;   coords[2].y = rect.bottom;  coords[2].z = 0;
            coords[3].x = rect.right;   coords[3].y = rect.top;     coords[3].z = 0;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x =    1; coords[0].y = -f.t; coords[0].z = -f.l;
            coords[1].x =    1; coords[1].y = -f.b; coords[1].z = -f.l;
            coords[2].x =    1; coords[2].y = -f.b; coords[2].z = -f.r;
            coords[3].x =    1; coords[3].y = -f.t; coords[3].z = -f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x =   -1; coords[0].y = -f.t; coords[0].z =  f.l;
            coords[1].x =   -1; coords[1].y = -f.b; coords[1].z =  f.l;
            coords[2].x =   -1; coords[2].y = -f.b; coords[2].z =  f.r;
            coords[3].x =   -1; coords[3].y = -f.t; coords[3].z =  f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x =  f.l; coords[0].y =    1; coords[0].z =  f.t;
            coords[1].x =  f.l; coords[1].y =    1; coords[1].z =  f.b;
            coords[2].x =  f.r; coords[2].y =    1; coords[2].z =  f.b;
            coords[3].x =  f.r; coords[3].y =    1; coords[3].z =  f.t;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x =  f.l; coords[0].y =   -1; coords[0].z = -f.t;
            coords[1].x =  f.l; coords[1].y =   -1; coords[1].z = -f.b;
            coords[2].x =  f.r; coords[2].y =   -1; coords[2].z = -f.b;
            coords[3].x =  f.r; coords[3].y =   -1; coords[3].z = -f.t;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x =  f.l; coords[0].y = -f.t; coords[0].z =    1;
            coords[1].x =  f.l; coords[1].y = -f.b; coords[1].z =    1;
            coords[2].x =  f.r; coords[2].y = -f.b; coords[2].z =    1;
            coords[3].x =  f.r; coords[3].y = -f.t; coords[3].z =    1;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(&rect, This->pow2Width, This->pow2Height, &f);
            coords[0].x = -f.l; coords[0].y = -f.t; coords[0].z =   -1;
            coords[1].x = -f.l; coords[1].y = -f.b; coords[1].z =   -1;
            coords[2].x = -f.r; coords[2].y = -f.b; coords[2].z =   -1;
            coords[3].x = -f.r; coords[3].y = -f.t; coords[3].z =   -1;
            break;

        default:
            ERR("Unexpected texture target %#x\n", This->glDescription.target);
            return;
    }

    ActivateContext(device, (IWineD3DSurface*)This, CTXUSAGE_BLIT);
    ENTER_GL();

    glEnable(bind_target);
    checkGLcall("glEnable(bind_target)");
    glBindTexture(bind_target, This->glDescription.textureName);
    checkGLcall("bind_target, This->glDescription.textureName)");
    glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    checkGLcall("glTexParameteri");
    glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    checkGLcall("glTexParameteri");

    if (device->render_offscreen)
    {
        LONG tmp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = tmp;
    }

    glBegin(GL_QUADS);
    glTexCoord3fv(&coords[0].x);
    glVertex2i(rect.left, rect.top);

    glTexCoord3fv(&coords[1].x);
    glVertex2i(rect.left, rect.bottom);

    glTexCoord3fv(&coords[2].x);
    glVertex2i(rect.right, rect.bottom);

    glTexCoord3fv(&coords[3].x);
    glVertex2i(rect.right, rect.top);
    glEnd();
    checkGLcall("glEnd");

    glDisable(bind_target);
    checkGLcall("glDisable(bind_target)");

    LEAVE_GL();

    if(SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface*)This, &IID_IWineD3DSwapChain, (void **) &swapchain)))
    {
        /* Make sure to flush the buffers. This is needed in apps like Red Alert II and Tiberian SUN that use multiple WGL contexts. */
        if(((IWineD3DSwapChainImpl*)swapchain)->frontBuffer == (IWineD3DSurface*)This ||
           ((IWineD3DSwapChainImpl*)swapchain)->num_contexts >= 2)
            glFlush();

        IWineD3DSwapChain_Release(swapchain);
    } else {
        /* We changed the filtering settings on the texture. Inform the container about this to get the filters
         * reset properly next draw
         */
        if(SUCCEEDED(IWineD3DSurface_GetContainer((IWineD3DSurface*)This, &IID_IWineD3DBaseTexture, (void **) &texture)))
        {
            ((IWineD3DBaseTextureImpl *) texture)->baseTexture.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
            ((IWineD3DBaseTextureImpl *) texture)->baseTexture.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
            ((IWineD3DBaseTextureImpl *) texture)->baseTexture.states[WINED3DTEXSTA_MIPFILTER] = WINED3DTEXF_NONE;
            IWineD3DBaseTexture_Release(texture);
        }
    }
}

/*****************************************************************************
 * IWineD3DSurface::LoadLocation
 *
 * Copies the current surface data from wherever it is to the requested
 * location. The location is one of the surface flags, SFLAG_INSYSMEM,
 * SFLAG_INTEXTURE and SFLAG_INDRAWABLE. When the surface is current in
 * multiple locations, the gl texture is preferred over the drawable, which is
 * preferred over system memory. The PBO counts as system memory. If rect is
 * not NULL, only the specified rectangle is copied (only supported for
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
    IWineD3DSwapChain *swapchain = NULL;
    GLenum format, internal, type;
    CONVERT_TYPES convert;
    int bpp;
    int width, pitch, outpitch;
    BYTE *mem;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
        if (SUCCEEDED(IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
            TRACE("Surface %p is an onscreen surface\n", iface);

            IWineD3DSwapChain_Release(swapchain);
        } else {
            /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets.
             * Prefer SFLAG_INTEXTURE. */
            if (flag == SFLAG_INDRAWABLE) flag = SFLAG_INTEXTURE;
        }
    }

    TRACE("(%p)->(%s, %p)\n", iface,
          flag == SFLAG_INSYSMEM ? "SFLAG_INSYSMEM" : flag == SFLAG_INDRAWABLE ? "SFLAG_INDRAWABLE" : "SFLAG_INTEXTURE",
          rect);
    if(rect) {
        TRACE("Rectangle: (%d,%d)-(%d,%d)\n", rect->left, rect->top, rect->right, rect->bottom);
    }

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
            if(!device->isInDraw) ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            surface_bind_and_dirtify(This);

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
            d3dfmt_get_conv(This, TRUE /* We need color keying */, FALSE /* We won't use textures */, &format, &internal, &type, &convert, &bpp, This->srgb);

            /* The width is in 'length' not in bytes */
            width = This->currentDesc.Width;
            pitch = IWineD3DSurface_GetPitch(iface);

            /* Don't use PBOs for converted surfaces. During PBO conversion we look at SFLAG_CONVERTED
             * but it isn't set (yet) in all cases it is getting called. */
            if((convert != NO_CONVERSION) && (This->Flags & SFLAG_PBO)) {
                TRACE("Removing the pbo attached to surface %p\n", This);
                surface_remove_pbo(This);
            }

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
            } else {
                This->Flags &= ~SFLAG_CONVERTED;
                mem = This->resource.allocatedMemory;
            }

            flush_to_framebuffer_drawpixels(This, format, type, bpp, mem);

            /* Don't delete PBO memory */
            if((mem != This->resource.allocatedMemory) && !(This->Flags & SFLAG_PBO))
                HeapFree(GetProcessHeap(), 0, mem);
        }
    } else /* if(flag == SFLAG_INTEXTURE) */ {
        if (This->Flags & SFLAG_INDRAWABLE) {
            read_from_framebuffer_texture(This);
        } else { /* Upload from system memory */
            d3dfmt_get_conv(This, TRUE /* We need color keying */, TRUE /* We will use textures */, &format, &internal, &type, &convert, &bpp, This->srgb);

            if(!device->isInDraw) ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            surface_bind_and_dirtify(This);

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

            /* Don't use PBOs for converted surfaces. During PBO conversion we look at SFLAG_CONVERTED
             * but it isn't set (yet) in all cases it is getting called. */
            if((convert != NO_CONVERSION) && (This->Flags & SFLAG_PBO)) {
                TRACE("Removing the pbo attached to surface %p\n", This);
                surface_remove_pbo(This);
            }

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
            ENTER_GL();
            glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
            LEAVE_GL();

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
            ENTER_GL();
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            LEAVE_GL();

            /* Don't delete PBO memory */
            if((mem != This->resource.allocatedMemory) && !(This->Flags & SFLAG_PBO))
                HeapFree(GetProcessHeap(), 0, mem);
        }
    }

    if(rect == NULL) {
        This->Flags |= flag;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && !swapchain
            && (This->Flags & (SFLAG_INTEXTURE | SFLAG_INDRAWABLE))) {
        /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets. */
        This->Flags |= (SFLAG_INTEXTURE | SFLAG_INDRAWABLE);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSwapChain *swapchain = NULL;

    /* Update the drawable size method */
    if(container) {
        IWineD3DBase_QueryInterface(container, &IID_IWineD3DSwapChain, (void **) &swapchain);
    }
    if(swapchain) {
        This->get_drawable_size = get_drawable_size_swapchain;
        IWineD3DSwapChain_Release(swapchain);
    } else if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        switch(wined3d_settings.offscreen_rendering_mode) {
            case ORM_FBO:        This->get_drawable_size = get_drawable_size_fbo;        break;
            case ORM_PBUFFER:    This->get_drawable_size = get_drawable_size_pbuffer;    break;
            case ORM_BACKBUFFER: This->get_drawable_size = get_drawable_size_backbuffer; break;
        }
    }

    return IWineD3DBaseSurfaceImpl_SetContainer(iface, container);
}

static WINED3DSURFTYPE WINAPI IWineD3DSurfaceImpl_GetImplType(IWineD3DSurface *iface) {
    return SURFACE_OPENGL;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_DrawOverlay(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    HRESULT hr;

    /* If there's no destination surface there is nothing to do */
    if(!This->overlay_dest) return WINED3D_OK;

    /* Blt calls ModifyLocation on the dest surface, which in turn calls DrawOverlay to
     * update the overlay. Prevent an endless recursion
     */
    if(This->overlay_dest->Flags & SFLAG_INOVERLAYDRAW) {
        return WINED3D_OK;
    }
    This->overlay_dest->Flags |= SFLAG_INOVERLAYDRAW;
    hr = IWineD3DSurfaceImpl_Blt((IWineD3DSurface *) This->overlay_dest, &This->overlay_destrect,
                                 iface, &This->overlay_srcrect, WINEDDBLT_WAIT,
                                 NULL, WINED3DTEXF_LINEAR);
    This->overlay_dest->Flags &= ~SFLAG_INOVERLAYDRAW;

    return hr;
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
    IWineD3DSurfaceImpl_UnLoad,
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
    IWineD3DSurfaceImpl_RealizePalette,
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
    IWineD3DSurfaceImpl_LoadTexture,
    IWineD3DSurfaceImpl_BindTexture,
    IWineD3DSurfaceImpl_SaveSnapshot,
    IWineD3DSurfaceImpl_SetContainer,
    IWineD3DSurfaceImpl_GetGlDesc,
    IWineD3DBaseSurfaceImpl_GetData,
    IWineD3DSurfaceImpl_SetFormat,
    IWineD3DSurfaceImpl_PrivateSetup,
    IWineD3DSurfaceImpl_ModifyLocation,
    IWineD3DSurfaceImpl_LoadLocation,
    IWineD3DSurfaceImpl_GetImplType,
    IWineD3DSurfaceImpl_DrawOverlay
};
#undef GLINFO_LOCATION

#define GLINFO_LOCATION device->adapter->gl_info
static HRESULT ffp_blit_alloc(IWineD3DDevice *iface) { return WINED3D_OK; }
static void ffp_blit_free(IWineD3DDevice *iface) { }

static HRESULT ffp_blit_set(IWineD3DDevice *iface, WINED3DFORMAT fmt, GLenum textype, UINT width, UINT height) {
    glEnable(textype);
    checkGLcall("glEnable(textype)");
    return WINED3D_OK;
}

static void ffp_blit_unset(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
}

static BOOL ffp_blit_color_fixup_supported(struct color_fixup_desc fixup)
{
    if (TRACE_ON(d3d_surface) && TRACE_ON(d3d))
    {
        TRACE("Checking support for fixup:\n");
        dump_color_fixup_desc(fixup);
    }

    /* We only support identity conversions. */
    if (is_identity_fixup(fixup))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    TRACE("[FAILED]\n");
    return FALSE;
}

const struct blit_shader ffp_blit =  {
    ffp_blit_alloc,
    ffp_blit_free,
    ffp_blit_set,
    ffp_blit_unset,
    ffp_blit_color_fixup_supported
};
