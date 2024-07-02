/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/swimpl.c
 * PURPOSE:              OpenGL32 DLL, opengl software implementation
 */

#include "opengl32.h"

/* MESA includes */
#include <main/context.h>
#include <main/framebuffer.h>
#include <main/renderbuffer.h>
#include <main/shared.h>
#include <main/viewport.h>
#include <swrast/s_context.h>
#include <swrast/s_renderbuffer.h>
#include <swrast_setup/swrast_setup.h>
#include <tnl/t_pipeline.h>
#include <tnl/tnl.h>
#include <drivers/common/driverfuncs.h>
#include <drivers/common/meta.h>

WINE_DEFAULT_DEBUG_CHANNEL(opengl32);

#define WIDTH_BYTES_ALIGN32(cx, bpp) ((((cx) * (bpp) + 31) & ~31) >> 3)

static const struct
{
    gl_format mesa;
    BYTE color_bits;
    BYTE red_bits, red_shift; DWORD red_mask;
    BYTE green_bits, green_shift; DWORD green_mask;
    BYTE blue_bits, blue_shift; DWORD blue_mask;
    BYTE alpha_bits, alpha_shift; DWORD alpha_mask;
    BYTE accum_bits;
    BYTE depth_bits;
    BYTE stencil_bits;
    DWORD bmp_compression;
} pixel_formats[] =
{
    { MESA_FORMAT_ARGB8888,     32,  8, 8,  0x00FF0000, 8, 16, 0x0000FF00, 8, 24, 0x000000FF, 8, 0,  0xFF000000, 16, 32, 8, BI_BITFIELDS },
    { MESA_FORMAT_ARGB8888,     32,  8, 8,  0x00FF0000, 8, 16, 0x0000FF00, 8, 24, 0x000000FF, 8, 0,  0xFF000000, 16, 16, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGBA8888_REV, 32,  8, 8,  0x000000FF, 8, 16, 0x0000FF00, 8, 24, 0x00FF0000, 8, 0,  0xFF000000, 16, 32, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGBA8888_REV, 32,  8, 8,  0x000000FF, 8, 16, 0x0000FF00, 8, 24, 0x00FF0000, 8, 0,  0xFF000000, 16, 16, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGBA8888,     32,  8, 0,  0xFF000000, 8, 8,  0x00FF0000, 8, 16, 0x0000FF00, 8, 24, 0x000000FF, 16, 32, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGBA8888,     32,  8, 0,  0xFF000000, 8, 8,  0x00FF0000, 8, 16, 0x0000FF00, 8, 24, 0x000000FF, 16, 16, 8, BI_BITFIELDS },
    { MESA_FORMAT_ARGB8888_REV, 32,  8, 16, 0x0000FF00, 8, 8,  0x00FF0000, 8, 0,  0xFF000000, 8, 24, 0x000000FF, 16, 32, 8, BI_BITFIELDS },
    { MESA_FORMAT_ARGB8888_REV, 32,  8, 16, 0x0000FF00, 8, 8,  0x00FF0000, 8, 0,  0xFF000000, 8, 24, 0x000000FF, 16, 16, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGB888,       24,  8, 0,  0x00000000, 8, 8,  0x00000000, 8, 16, 0x00000000, 0, 0,  0x00000000, 16, 32, 8, BI_RGB },
    { MESA_FORMAT_RGB888,       24,  8, 0,  0x00000000, 8, 8,  0x00000000, 8, 16, 0x00000000, 0, 0,  0x00000000, 16, 16, 8, BI_RGB },
    // { MESA_FORMAT_BGR888,       24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 32, 8 },
    // { MESA_FORMAT_BGR888,       24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 16, 8 },
    { MESA_FORMAT_RGB565,       16,  5, 0,  0x0000F800, 6, 5,  0x000007E0, 5, 11, 0x0000001F, 0, 0,  0x00000000, 16, 32, 8, BI_BITFIELDS },
    { MESA_FORMAT_RGB565,       16,  5, 0,  0x0000F800, 6, 5,  0x000007E0, 5, 11, 0x0000001F, 0, 0,  0x00000000, 16, 16, 8, BI_BITFIELDS },
};

#define SW_BACK_RENDERBUFFER_CLASS  0x8911
#define SW_FRONT_RENDERBUFFER_CLASS 0x8910
struct sw_front_renderbuffer
{
    struct swrast_renderbuffer swrast;
    
    HDC hdcmem;
    GLuint x, y, w, h;
    HBITMAP hbmp;
    BOOL write;
};

#define SW_FB_DOUBLEBUFFERED    0x1
#define SW_FB_DIRTY_SIZE        0x2
struct sw_framebuffer
{
    HDC hdc;
    INT sw_format;
    UINT format_index;
    DWORD flags;
    /* The mesa objects */
    struct gl_config *gl_visual;		/* Describes the buffers */
    struct gl_framebuffer *gl_buffer;	/* The mesa representation of frame buffer */
    struct sw_front_renderbuffer frontbuffer;
    struct swrast_renderbuffer backbuffer;
    /* The bitmapi info we will use for rendering to display */
    BITMAPINFO bmi;
};

struct sw_context
{
    struct gl_context mesa;		/* Base class - this must be first */
    /* This is to keep track of the size of the front buffer */
    HHOOK hook;
    /* Framebuffer currently owning the context */
    struct sw_framebuffer framebuffer;
};

/* mesa opengl32 "driver" specific */
static const GLubyte *
sw_get_string( struct gl_context *ctx, GLenum name )
{
    (void) ctx;
    if(name == GL_RENDERER)
    {
        static const GLubyte renderer[] = { 'R','e','a','c','t','O','S',' ',
            'S','o','f','t','w','a','r','e',' ',
            'I','m','p','l','e','m','e','n','t','a','t','i','o','n',0 };
        return renderer;
    }
    /* Don't claim to support the fancy extensions that mesa supports, they will be slow anyway */
    if(name == GL_EXTENSIONS)
    {
        static const GLubyte extensions[] = { 0 };
        return extensions;
    }
    return NULL;
}

static void
sw_update_state( struct gl_context *ctx, GLuint new_state )
{
   /* easy - just propagate */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
}

/* Renderbuffer routines */
static GLboolean
sw_bb_renderbuffer_storage(struct gl_context* ctx, struct gl_renderbuffer *rb,
                          GLenum internalFormat,
                          GLuint width, GLuint height)
{
    struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
    struct sw_framebuffer* fb = CONTAINING_RECORD(srb, struct sw_framebuffer, backbuffer);
    UINT widthBytes = WIDTH_BYTES_ALIGN32(width, pixel_formats[fb->format_index].color_bits); 
    srb->Base.Format = pixel_formats[fb->format_index].mesa;

    if(srb->Buffer)
        srb->Buffer = HeapReAlloc(GetProcessHeap(), 0, srb->Buffer, widthBytes*height);
    else
        srb->Buffer = HeapAlloc(GetProcessHeap(), 0, widthBytes*height);
    if(!srb->Buffer)
    {
        srb->Base.Format = MESA_FORMAT_NONE;
        return GL_FALSE;
    }
    srb->Base.Width = width;
    srb->Base.Height = height;
    srb->RowStride = widthBytes;
    return GL_TRUE;
}

static void
sw_bb_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);

    if (srb->Buffer)
    {
        HeapFree(GetProcessHeap(), 0, srb->Buffer);
        srb->Buffer = NULL;
    }
}

static void
sw_fb_renderbuffer_delete(struct gl_renderbuffer *rb)
{
    struct sw_front_renderbuffer* srb = (struct sw_front_renderbuffer*)rb;

    if (srb->hbmp)
    {
        srb->hbmp = SelectObject(srb->hdcmem, srb->hbmp);
        DeleteDC(srb->hdcmem);
        DeleteObject(srb->hbmp);
        srb->hdcmem = NULL;
        srb->hbmp = NULL;
        srb->swrast.Buffer = NULL;
    }
}

static GLboolean
sw_fb_renderbuffer_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat,
                           GLuint width, GLuint height)
{
    struct sw_front_renderbuffer* srb = (struct sw_front_renderbuffer*)rb;
    struct sw_framebuffer* fb = CONTAINING_RECORD(srb, struct sw_framebuffer, frontbuffer);
    HDC hdc = IntGetCurrentDC();
    
    /* Don't bother if the size doesn't change */
    if(rb->Width == width && rb->Height == height)
        return GL_TRUE;

    /* Delete every objects which could have been used before */
    sw_fb_renderbuffer_delete(&srb->swrast.Base);
    
    /* So the app wants to use the frontbuffer, allocate a DIB for it */
    srb->hbmp = CreateDIBSection(
        hdc,
        &fb->bmi,
        DIB_RGB_COLORS,
        (void**)&srb->swrast.Buffer,
        NULL, 0);
    if(!srb->hbmp)
    {
        ERR("Failed to create the DIB section for the front buffer, %lu.\n", GetLastError());
        return GL_FALSE;
    }
    /* Create the DC and attach the DIB section to it */
    srb->hdcmem = CreateCompatibleDC(hdc);
    assert(srb->hdcmem != NULL);
    srb->hbmp = SelectObject(srb->hdcmem, srb->hbmp);
    assert(srb->hbmp != NULL);
    /* Set formats, width and height */
    srb->swrast.Base.Format = pixel_formats[fb->format_index].mesa;
    srb->swrast.Base.Width = width;
    srb->swrast.Base.Height = height;
    return GL_TRUE;
}

static
void
sw_init_renderbuffers(struct sw_framebuffer *fb)
{
    _mesa_init_renderbuffer(&fb->frontbuffer.swrast.Base, 0);
    fb->frontbuffer.swrast.Base.ClassID = SW_FRONT_RENDERBUFFER_CLASS;
    fb->frontbuffer.swrast.Base.AllocStorage = sw_fb_renderbuffer_storage;
    fb->frontbuffer.swrast.Base.Delete = sw_fb_renderbuffer_delete;
    fb->frontbuffer.swrast.Base.InternalFormat = GL_RGBA;
    fb->frontbuffer.swrast.Base._BaseFormat = GL_RGBA;
    _mesa_remove_renderbuffer(fb->gl_buffer, BUFFER_FRONT_LEFT);
    _mesa_add_renderbuffer(fb->gl_buffer, BUFFER_FRONT_LEFT, &fb->frontbuffer.swrast.Base);
    
    if(fb->flags & SW_FB_DOUBLEBUFFERED)
    {
        _mesa_init_renderbuffer(&fb->backbuffer.Base, 0);
        fb->backbuffer.Base.ClassID = SW_BACK_RENDERBUFFER_CLASS;
        fb->backbuffer.Base.AllocStorage = sw_bb_renderbuffer_storage;
        fb->backbuffer.Base.Delete = sw_bb_renderbuffer_delete;
        fb->backbuffer.Base.InternalFormat = GL_RGBA;
        fb->backbuffer.Base._BaseFormat = GL_RGBA;
        _mesa_remove_renderbuffer(fb->gl_buffer, BUFFER_BACK_LEFT);
        _mesa_add_renderbuffer(fb->gl_buffer, BUFFER_BACK_LEFT, &fb->backbuffer.Base);
    }
    
    
}

static void
sw_MapRenderbuffer(struct gl_context *ctx,
                   struct gl_renderbuffer *rb,
                   GLuint x, GLuint y, GLuint w, GLuint h,
                   GLbitfield mode,
                   GLubyte **mapOut, GLint *rowStrideOut)
{
    if(rb->ClassID == SW_FRONT_RENDERBUFFER_CLASS)
    {
        /* This is our front buffer */
        struct sw_front_renderbuffer* srb = (struct sw_front_renderbuffer*)rb;
        struct sw_framebuffer* fb = CONTAINING_RECORD(srb, struct sw_framebuffer, frontbuffer);
        /* Set the stride */
        *rowStrideOut = WIDTH_BYTES_ALIGN32(rb->Width, pixel_formats[fb->format_index].color_bits);
        /* Remember where we "mapped" */
        srb->x = x; srb->y = y; srb->w = w; srb->h = h;
        /* Remember if we should write it later */
        srb->write = !!(mode & GL_MAP_WRITE_BIT);
        /* Get the bits, if needed */
        if(mode & GL_MAP_READ_BIT)
        {
            BitBlt(srb->hdcmem, srb->x, srb->y, srb->w, srb->h,
                IntGetCurrentDC(), srb->x, srb->y, SRCCOPY);
        }
        /* And return it */
        *mapOut = (BYTE*)srb->swrast.Buffer + *rowStrideOut*y + x*pixel_formats[fb->format_index].color_bits/8;
        return;
    }
    
    if(rb->ClassID == SW_BACK_RENDERBUFFER_CLASS)
    {
        /* This is our front buffer */
        struct swrast_renderbuffer* srb = (struct swrast_renderbuffer*)rb;
        const GLuint bpp = _mesa_get_format_bytes(rb->Format);
        /* Set the stride */
        *rowStrideOut = srb->RowStride;
        *mapOut = (BYTE*)srb->Buffer + srb->RowStride*y + x*bpp;
        return;
    }
    
    /* Let mesa rasterizer take care of this */
    _swrast_map_soft_renderbuffer(ctx, rb, x, y, w, h, mode,
                                  mapOut, rowStrideOut);
}

static void
sw_UnmapRenderbuffer(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   if (rb->ClassID== SW_FRONT_RENDERBUFFER_CLASS)
    {
        /* This is our front buffer */
        struct sw_front_renderbuffer* srb = (struct sw_front_renderbuffer*)rb;
        if(srb->write)
        {
            /* Copy the bits to our display */
            BitBlt(IntGetCurrentDC(),
                srb->x, srb->y, srb->w, srb->h,
                srb->hdcmem, srb->x, srb->y, SRCCOPY);
            srb->write = FALSE;
        }
        return;
    }
    
    if(rb->ClassID == SW_BACK_RENDERBUFFER_CLASS)
        return; /* nothing to do */
    
    /* Let mesa rasterizer take care of this */
    _swrast_unmap_soft_renderbuffer(ctx, rb);
}

/* WGL <-> mesa glue */
static UINT index_from_format(struct wgl_dc_data* dc_data, INT format, BOOL* doubleBuffered)
{
    UINT index;
    INT nb_win_compat = 0, start_win_compat = 0;
    HDC hdc;
    INT bpp;
    
    *doubleBuffered = FALSE;
    
    if(!(dc_data->flags & WGL_DC_OBJ_DC))
        return format - 1; /* OBJ_MEMDC, not double buffered */
    
    hdc = GetDC(dc_data->owner.hwnd);

    /* Find the window compatible formats */
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    for(index = 0; index<sizeof(pixel_formats)/sizeof(pixel_formats[0]); index++)
    {
        if(pixel_formats[index].color_bits == bpp)
        {
            if(!start_win_compat)
                start_win_compat = index+1;
            nb_win_compat++;
        }
    }
    ReleaseDC(dc_data->owner.hwnd, hdc);
    
    /* Double buffered format */
    if(format < (start_win_compat + nb_win_compat))
    {
        if(format >= start_win_compat)
            *doubleBuffered = TRUE;
        return format-1;
    }
    /* Shift */
    return format - nb_win_compat - 1;
}

INT sw_DescribePixelFormat(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR* descr)
{
    UINT index;
    INT nb_win_compat = 0, start_win_compat = 0;
    INT ret = sizeof(pixel_formats)/sizeof(pixel_formats[0]);
    
    if (GetObjectType(hdc) == OBJ_DC)
    {
        /* Find the window compatible formats */
        INT bpp = GetDeviceCaps(hdc, BITSPIXEL);
        for (index = 0; index < sizeof(pixel_formats)/sizeof(pixel_formats[0]); index++)
        {
            if (pixel_formats[index].color_bits == bpp)
            {
                if (!start_win_compat)
                    start_win_compat = index+1;
                nb_win_compat++;
            }
        }
        /* Add the double buffered formats */
        ret += nb_win_compat;
    }

    index = (UINT)format - 1;
    if(!descr)
        return ret;
    if((format > ret) || (size != sizeof(*descr)))
        return 0;

    /* Set flags */
    descr->dwFlags = PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_BITMAP | PFD_GENERIC_FORMAT;
    /* See if this is a format compatible with the window */
    if(format >= start_win_compat && format < (start_win_compat + nb_win_compat*2) )
    {
        /* It is */
        descr->dwFlags |= PFD_DRAW_TO_WINDOW;
        /* See if this should be double buffered */
        if(format < (start_win_compat + nb_win_compat))
        {
            /* No GDI, no bitmap */
            descr->dwFlags &= ~(PFD_SUPPORT_GDI | PFD_DRAW_TO_BITMAP);
            descr->dwFlags |= PFD_DOUBLEBUFFER;
        }
    }
    /* Normalize the index */
    if(format >= start_win_compat + nb_win_compat)
        index -= nb_win_compat;
    
    /* Fill the rest of the structure */
    descr->nSize            = sizeof(*descr);
    descr->nVersion         = 1;
    descr->iPixelType       = PFD_TYPE_RGBA;
    descr->cColorBits       = pixel_formats[index].color_bits;
    descr->cRedBits         = pixel_formats[index].red_bits;
    descr->cRedShift        = pixel_formats[index].red_shift;
    descr->cGreenBits       = pixel_formats[index].green_bits;
    descr->cGreenShift      = pixel_formats[index].green_shift;
    descr->cBlueBits        = pixel_formats[index].blue_bits;
    descr->cBlueShift       = pixel_formats[index].blue_shift;
    descr->cAlphaBits       = pixel_formats[index].alpha_bits;
    descr->cAlphaShift      = pixel_formats[index].alpha_shift;
    descr->cAccumBits       = pixel_formats[index].accum_bits;
    descr->cAccumRedBits    = pixel_formats[index].accum_bits / 4;
    descr->cAccumGreenBits  = pixel_formats[index].accum_bits / 4;
    descr->cAccumBlueBits   = pixel_formats[index].accum_bits / 4;
    descr->cAccumAlphaBits  = pixel_formats[index].accum_bits / 4;
    descr->cDepthBits       = pixel_formats[index].depth_bits;
    descr->cStencilBits     = pixel_formats[index].stencil_bits;
    descr->cAuxBuffers      = 0;
    descr->iLayerType       = PFD_MAIN_PLANE;
    return ret;
}

BOOL sw_SetPixelFormat(HDC hdc, struct wgl_dc_data* dc_data, INT format)
{
    struct sw_framebuffer* fb;
    BOOL doubleBuffered;
    
    assert(dc_data->sw_data == NULL);

    /* So, someone is crazy enough to ask for sw implementation. Announce it. */
    TRACE("OpenGL software implementation START!\n");
    
    /* allocate our structure */
    fb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET(struct sw_framebuffer, bmi.bmiColors[3]));
    if(!fb)
    {
        ERR("HeapAlloc FAILED!\n");
        return FALSE;
    }
    /* Get the format index */
    fb->format_index = index_from_format(dc_data, format, &doubleBuffered);
    TRACE("Using format %u - double buffered: %x.\n", fb->format_index, doubleBuffered);
    fb->flags = doubleBuffered ? SW_FB_DOUBLEBUFFERED : 0;
    /* Set the bitmap info describing the framebuffer */
    fb->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    fb->bmi.bmiHeader.biPlanes = 1;
    fb->bmi.bmiHeader.biBitCount = pixel_formats[fb->format_index].color_bits;
    fb->bmi.bmiHeader.biCompression = pixel_formats[fb->format_index].bmp_compression;
    fb->bmi.bmiHeader.biSizeImage = 0;
    fb->bmi.bmiHeader.biXPelsPerMeter = 0;
    fb->bmi.bmiHeader.biYPelsPerMeter = 0;
    fb->bmi.bmiHeader.biClrUsed = 0;
    fb->bmi.bmiHeader.biClrImportant = 0;
    *((DWORD*)&fb->bmi.bmiColors[0]) = pixel_formats[fb->format_index].red_mask;
    *((DWORD*)&fb->bmi.bmiColors[1]) = pixel_formats[fb->format_index].green_mask;
    *((DWORD*)&fb->bmi.bmiColors[2]) = pixel_formats[fb->format_index].blue_mask;
    /* Save the HDC */
    fb->hdc = hdc;
    
    /* Allocate the visual structure describing the format */
    fb->gl_visual = _mesa_create_visual(
        !!(fb->flags & SW_FB_DOUBLEBUFFERED),
        GL_FALSE, /* No stereoscopic support */
        pixel_formats[fb->format_index].red_bits,
        pixel_formats[fb->format_index].green_bits,
        pixel_formats[fb->format_index].blue_bits,
        pixel_formats[fb->format_index].alpha_bits,
        pixel_formats[fb->format_index].depth_bits,
        pixel_formats[fb->format_index].stencil_bits,
        pixel_formats[fb->format_index].accum_bits,
        pixel_formats[fb->format_index].accum_bits,
        pixel_formats[fb->format_index].accum_bits,
        pixel_formats[fb->format_index].alpha_bits ? 
            pixel_formats[fb->format_index].accum_bits : 0);
    
    if(!fb->gl_visual)
    {
        ERR("Failed to allocate a GL visual.\n");
        HeapFree(GetProcessHeap(), 0, fb);
        return FALSE;
    }

    /* Allocate the framebuffer structure */
    fb->gl_buffer = _mesa_create_framebuffer(fb->gl_visual);
    if (!fb->gl_buffer) {
        ERR("Failed to allocate the mesa framebuffer structure.\n");
        _mesa_destroy_visual( fb->gl_visual );
        HeapFree(GetProcessHeap(), 0, fb);
        return FALSE;
    }
    
    /* Add the depth/stencil/accum buffers */
    _swrast_add_soft_renderbuffers(fb->gl_buffer,
                             GL_FALSE, /* color */
                             fb->gl_visual->haveDepthBuffer,
                             fb->gl_visual->haveStencilBuffer,
                             fb->gl_visual->haveAccumBuffer,
                             GL_FALSE, /* alpha */
                             GL_FALSE /* aux */ );
    
    /* Initialize our render buffers */
    sw_init_renderbuffers(fb);

    /* Everything went fine */
    dc_data->sw_data = fb;
    return TRUE;
}

DHGLRC sw_CreateContext(struct wgl_dc_data* dc_data)
{
    struct sw_context* sw_ctx;
    struct sw_framebuffer* fb = dc_data->sw_data;
    struct dd_function_table mesa_drv_functions;
    TNLcontext *tnl;

    /* We use the mesa memory routines for this function */
    sw_ctx = CALLOC_STRUCT(sw_context);
    if(!sw_ctx)
        return NULL;
    
    /* Set mesa default functions */
    _mesa_init_driver_functions(&mesa_drv_functions);
    /* Override */
    mesa_drv_functions.GetString = sw_get_string;
    mesa_drv_functions.UpdateState = sw_update_state;
    mesa_drv_functions.GetBufferSize = NULL;
    
    /* Initialize the context */
    if(!_mesa_initialize_context(&sw_ctx->mesa,
                                 fb->gl_visual,
                                 NULL,
                                 &mesa_drv_functions,
                                 (void *) sw_ctx))
    {
        ERR("Failed to initialize the mesa context.\n");
        free(sw_ctx);
        return NULL;
    }
    
    /* Initialize the "meta driver" */
    _mesa_meta_init(&sw_ctx->mesa);
    
    /* Initialize helpers */
    if(!_swrast_CreateContext(&sw_ctx->mesa) ||
       !_vbo_CreateContext(&sw_ctx->mesa) ||
       !_tnl_CreateContext(&sw_ctx->mesa) ||
       !_swsetup_CreateContext(&sw_ctx->mesa))
    {
        ERR("Failed initializing helpers.\n");
        _mesa_free_context_data(&sw_ctx->mesa);
        free(sw_ctx);
        return NULL;
    }
    
    /* Wake up! */
    _swsetup_Wakeup(&sw_ctx->mesa);
    
    /* Use TnL defaults */
    tnl = TNL_CONTEXT(&sw_ctx->mesa);
    tnl->Driver.RunPipeline = _tnl_run_pipeline;
    
    /* To map the display into user memory */
    sw_ctx->mesa.Driver.MapRenderbuffer = sw_MapRenderbuffer;
    sw_ctx->mesa.Driver.UnmapRenderbuffer = sw_UnmapRenderbuffer;
    
    return (DHGLRC)sw_ctx;
}

BOOL sw_DeleteContext(DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;
    /* Those get clobbered by _mesa_free_context_data via _glapi_set{context,dispath_table} */
    void* icd_save = IntGetCurrentICDPrivate();
    const GLDISPATCHTABLE* table_save = IntGetCurrentDispatchTable();

    /* Destroy everything */
    _mesa_meta_free( &sw_ctx->mesa );

    _swsetup_DestroyContext( &sw_ctx->mesa );
    _tnl_DestroyContext( &sw_ctx->mesa );
    _vbo_DestroyContext( &sw_ctx->mesa );
    _swrast_DestroyContext( &sw_ctx->mesa );

    _mesa_free_context_data( &sw_ctx->mesa );
    free( sw_ctx );
    
    /* Restore this */
    IntSetCurrentDispatchTable(table_save);
    IntSetCurrentICDPrivate(icd_save);

    return TRUE;
}

PROC sw_GetProcAddress(LPCSTR name)
{
    /* We don't support any extensions */
    WARN("Asking for proc address %s, returning NULL.\n", name);
    return NULL;
}

BOOL sw_CopyContext(DHGLRC dhglrcSrc, DHGLRC dhglrcDst, UINT mask)
{
    FIXME("Software wglCopyContext is UNIMPLEMENTED, mask %lx.\n", mask);
    return FALSE;
}

BOOL sw_ShareLists(DHGLRC dhglrcSrc, DHGLRC dhglrcDst)
{
    struct sw_context* sw_ctx_src = (struct sw_context*)dhglrcSrc;
    struct sw_context* sw_ctx_dst = (struct sw_context*)dhglrcDst;

    /* See if it was already shared */
    if(sw_ctx_dst->mesa.Shared->RefCount > 1)
        return FALSE;
    
    /* Unreference the old, share the new */
    _mesa_reference_shared_state(&sw_ctx_dst->mesa,
        &sw_ctx_dst->mesa.Shared,
        sw_ctx_src->mesa.Shared);
    
    return TRUE;
}

static
LRESULT CALLBACK
sw_call_window_proc(
   int nCode,
   WPARAM wParam,
   LPARAM lParam )
{
    struct wgl_dc_data* dc_data = IntGetCurrentDcData();
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    struct sw_framebuffer* fb;
    PCWPSTRUCT pParams = (PCWPSTRUCT)lParam;

    if((!dc_data) || (!ctx))
        return 0;

    if(!(dc_data->flags & WGL_DC_OBJ_DC))
        return 0;

    if((nCode < 0) || (dc_data->owner.hwnd != pParams->hwnd) || (dc_data->sw_data == NULL))
        return CallNextHookEx(ctx->hook, nCode, wParam, lParam);
    
    fb = dc_data->sw_data;

    if (pParams->message == WM_WINDOWPOSCHANGED)
    {
        /* We handle WM_WINDOWPOSCHANGED instead of WM_SIZE because according to
         * http://blogs.msdn.com/oldnewthing/archive/2008/01/15/7113860.aspx
         * WM_SIZE is generated from WM_WINDOWPOSCHANGED by DefWindowProc so it
         * can be masked out by the application. */
        LPWINDOWPOS lpWindowPos = (LPWINDOWPOS)pParams->lParam;
        if((lpWindowPos->flags & SWP_SHOWWINDOW) ||
            !(lpWindowPos->flags & SWP_NOMOVE) ||
            !(lpWindowPos->flags & SWP_NOSIZE))
        {
            /* Size in WINDOWPOS includes the window frame, so get the size
             * of the client area via GetClientRect.  */
            RECT client_rect;
            UINT width, height;

            TRACE("Got WM_WINDOWPOSCHANGED\n");

            GetClientRect(pParams->hwnd, &client_rect);
            width = client_rect.right - client_rect.left;
            height = client_rect.bottom - client_rect.top;
            /* Do not reallocate for minimized windows */
            if(width <= 0 || height <= 0)
                goto end;
            /* Update framebuffer size */
            fb->bmi.bmiHeader.biWidth = width;
            fb->bmi.bmiHeader.biHeight = height;
            /* Propagate to mesa */
            _mesa_resize_framebuffer(&ctx->mesa, fb->gl_buffer, width, height);
        }
    }

end:
    return CallNextHookEx(ctx->hook, nCode, wParam, lParam);
}

BOOL sw_SetContext(struct wgl_dc_data* dc_data, DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;
    struct sw_framebuffer* fb = dc_data->sw_data;
    UINT width, height;
    
    /* Update state */
    sw_update_state(&sw_ctx->mesa, 0);
    
    /* Get framebuffer size */
    if(dc_data->flags & WGL_DC_OBJ_DC)
    {
        HWND hwnd = dc_data->owner.hwnd;
        RECT client_rect;
        if(!hwnd)
        {
            ERR("Physical DC without a window!\n");
            return FALSE;
        }
        if(!GetClientRect(hwnd, &client_rect))
        {
            ERR("GetClientRect failed!\n");
            return FALSE;
        }
        /* This is a physical DC. Setup the hook */
        sw_ctx->hook = SetWindowsHookEx(WH_CALLWNDPROC,
                            sw_call_window_proc,
                            NULL,
                            GetCurrentThreadId());
        /* Calculate width & height */
        width  = client_rect.right  - client_rect.left;
        height = client_rect.bottom - client_rect.top;
    }
    else /* OBJ_MEMDC */
    {
        BITMAP bm;
        HBITMAP hbmp;
        HDC hdc = dc_data->owner.hdc;
        
        if(fb->flags & SW_FB_DOUBLEBUFFERED)
        {
            ERR("Memory DC called with a double buffered format.\n");
            return FALSE;
        }

        hbmp = GetCurrentObject( hdc, OBJ_BITMAP );
        if(!hbmp)
        {
            ERR("No Bitmap!\n");
            return FALSE;
        }
        if(GetObject(hbmp, sizeof(bm), &bm) == 0)
        {
            ERR("GetObject failed!\n");
            return FALSE;
        }
        width = bm.bmWidth;
        height = bm.bmHeight;
    }

    if(!width) width = 1;
    if(!height) height = 1;
    
    fb->bmi.bmiHeader.biWidth = width;
    fb->bmi.bmiHeader.biHeight = height;
    
    /* Also make the mesa context current to mesa */
    if(!_mesa_make_current(&sw_ctx->mesa, fb->gl_buffer, fb->gl_buffer))
    {
        ERR("_mesa_make_current filaed!\n");
        return FALSE;
    }
    
    /* Set the viewport if this is the first time we initialize this context */
    if(sw_ctx->mesa.Viewport.X == 0 && 
       sw_ctx->mesa.Viewport.Y == 0 &&
       sw_ctx->mesa.Viewport.Width == 0 &&
       sw_ctx->mesa.Viewport.Height == 0)
    {
        _mesa_set_viewport(&sw_ctx->mesa, 0, 0, width, height);
    }

    /* update the framebuffer size */
    _mesa_resize_framebuffer(&sw_ctx->mesa, fb->gl_buffer, width, height);
   
   /* We're good */
   return TRUE;
}

void sw_ReleaseContext(DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;

    /* Forward to mesa */
    _mesa_make_current(NULL, NULL, NULL);
    
    /* Unhook */
    if(sw_ctx->hook)
    {
        UnhookWindowsHookEx(sw_ctx->hook);
        sw_ctx->hook = NULL;
    }
}

BOOL sw_SwapBuffers(HDC hdc, struct wgl_dc_data* dc_data)
{
    struct sw_framebuffer* fb = dc_data->sw_data;
    struct sw_context* sw_ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    
    /* Notify mesa */
    if(sw_ctx)
        _mesa_notifySwapBuffers(&sw_ctx->mesa);
    
    if(!(fb->flags & SW_FB_DOUBLEBUFFERED))
        return TRUE;

    /* Upload to the display */
    return (SetDIBitsToDevice(hdc,
        0,
        0,
        fb->bmi.bmiHeader.biWidth,
        fb->bmi.bmiHeader.biHeight,
        0,
        0,
        0,
        fb->bmi.bmiHeader.biHeight,
        fb->backbuffer.Buffer,
        &fb->bmi,
        DIB_RGB_COLORS) != 0);
}
