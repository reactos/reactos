/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/wgl.c
 * PURPOSE:              OpenGL32 DLL, opengl software implementation
 */

#include "opengl32.h"
#include <GL/osmesa.h>

#include <assert.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(opengl32);

#define WIDTH_BYTES_ALIGN32(cx, bpp) ((((cx) * (bpp) + 31) & ~31) >> 3)

/* OSMesa stuff */
static HMODULE hMesaDll = NULL;
static OSMesaContext (GLAPIENTRY *pOSMesaCreateContextExt)(GLenum format, GLint depthBits, GLint stencilBits,
                                                 GLint accumBits, OSMesaContext sharelist);
static void (GLAPIENTRY *pOSMesaDestroyContext)(OSMesaContext ctx);
static GLboolean (GLAPIENTRY *pOSMesaMakeCurrent)( OSMesaContext ctx, void *buffer, GLenum type,
                                        GLsizei width, GLsizei height );
static void (GLAPIENTRY *pOSMesaPixelStore)( GLint pname, GLint value );
static OSMESAproc (GLAPIENTRY *pOSMesaGetProcAddress)(const char *funcName);

struct sw_context
{
    OSMesaContext mesa_ctx;
    HHOOK hook;
    struct sw_framebuffer* framebuffer;
    GLenum readmode;
};

#define SW_FB_DOUBLEBUFFERED    0x1
#define SW_FB_DIBSECTION        0x2
#define SW_FB_FREE_BITS         0x4
#define SW_FB_DIRTY_BITS        0x8
#define SW_FB_DIRTY_SIZE        0x10
#define SW_FB_DIRTY             (SW_FB_DIRTY_BITS | SW_FB_DIRTY_SIZE)

struct sw_framebuffer
{
    INT sw_format;
    UINT format_index;
    union
    {
        void* backbuffer;
        void* bits;
    };
    void* frontbuffer;
    DWORD flags;
    BITMAPINFO bmi;
};

/* 
 * Functions that we shadow to compensate with the impossibility 
 * to use double buffered format in osmesa 
 */
static void (GLAPIENTRY * pFinish)(void);
static void (GLAPIENTRY * pReadBuffer)(GLenum);
static void (GLAPIENTRY * pGetBooleanv)(GLenum pname, GLboolean* params);
static void (GLAPIENTRY * pGetFloatv)(GLenum pname, GLfloat* params);
static void (GLAPIENTRY * pGetDoublev)(GLenum pname, GLdouble* params);
static void (GLAPIENTRY * pGetIntegerv)(GLenum pname, GLint* params);
static void (GLAPIENTRY * pReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);

/* API table for single buffered mode */
static GLCLTPROCTABLE sw_table_sb;
/* API table for double buffered mode */
static GLCLTPROCTABLE sw_table_db;

static const struct
{
    UINT mesa;
    BYTE color_bits;
    BYTE red_bits, red_shift;
    BYTE green_bits, green_shift;
    BYTE blue_bits, blue_shift;
    BYTE alpha_bits, alpha_shift;
    BYTE accum_bits;
    BYTE depth_bits;
    BYTE stencil_bits;
} pixel_formats[] =
{
    { OSMESA_BGRA,     32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 32, 8 },
    { OSMESA_BGRA,     32,  8, 16, 8, 8,  8, 0,  8, 24,  16, 16, 8 },
    { OSMESA_RGBA,     32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 32, 8 },
    { OSMESA_RGBA,     32,  8, 0,  8, 8,  8, 16, 8, 24,  16, 16, 8 },
    { OSMESA_ARGB,     32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 32, 8 },
    { OSMESA_ARGB,     32,  8, 8,  8, 16, 8, 24, 8, 0,   16, 16, 8 },
    { OSMESA_RGB,      24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 32, 8 },
    { OSMESA_RGB,      24,  8, 0,  8, 8,  8, 16, 0, 0,   16, 16, 8 },
    { OSMESA_BGR,      24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 32, 8 },
    { OSMESA_BGR,      24,  8, 16, 8, 8,  8, 0,  0, 0,   16, 16, 8 },
    { OSMESA_RGB_565,  16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 32, 8 },
    { OSMESA_RGB_565,  16,  5, 0,  6, 5,  5, 11, 0, 0,   16, 16, 8 },
};

/* Single buffered format specifics */
static void GLAPIENTRY sw_sb_Finish(void)
{
    struct wgl_dc_data* dc_data = IntGetCurrentDcData();
    struct sw_framebuffer* fb;
    HDC hdc = IntGetCurrentDC();
    
    /* Call osmesa */
    pFinish();
    
    assert(dc_data != NULL);
    fb = dc_data->sw_data;
    assert(fb != NULL);
    
    /* osmesa directly updated the bits of the DIB section */
    if(fb->flags & SW_FB_DIBSECTION)
        return;
    
    /* Upload the data to the device */
    SetDIBitsToDevice(hdc,
        0,
        0,
        fb->bmi.bmiHeader.biWidth,
        fb->bmi.bmiHeader.biHeight,
        0,
        0,
        0,
        fb->bmi.bmiHeader.biWidth,
        fb->bits,
        &fb->bmi,
        DIB_RGB_COLORS);
}

/* Double buffered format specifics */
static void sw_db_update_frontbuffer(struct sw_framebuffer* fb, HDC hdc)
{
    unsigned int widthbytes = WIDTH_BYTES_ALIGN32(fb->bmi.bmiHeader.biWidth,
        pixel_formats[fb->format_index].color_bits);
    size_t buffer_size = widthbytes*fb->bmi.bmiHeader.biHeight;
    if(fb->flags & SW_FB_DIRTY_SIZE)
    {
        if(fb->frontbuffer)
            fb->frontbuffer = HeapReAlloc(GetProcessHeap(), 0, fb->frontbuffer, buffer_size);
        else
            fb->frontbuffer = HeapAlloc(GetProcessHeap(), 0, buffer_size);
        fb->flags ^= SW_FB_DIRTY_SIZE;
    }
    
    if(fb->flags & SW_FB_DIRTY_BITS)
    {
        /* On windows, there is SetDIBitsToDevice, but not FROM device... */
        HBITMAP hbmp;
        HDC hmemDC = CreateCompatibleDC(hdc);
        void* DIBits;
        hbmp = CreateDIBSection(hdc,
            &fb->bmi,
            DIB_RGB_COLORS,
            &DIBits,
            NULL, 0);
        hbmp = SelectObject(hmemDC, hbmp);
        BitBlt(hdc,
            0,
            0,
            fb->bmi.bmiHeader.biWidth,
            fb->bmi.bmiHeader.biHeight,
            hmemDC,
            0,
            0,
            SRCCOPY);
        /* Copy the bits */
        CopyMemory(fb->frontbuffer, DIBits, buffer_size);
        /* Clean up */
        hbmp = SelectObject(hmemDC, hbmp);
        DeleteDC(hmemDC);
        DeleteObject(hbmp);
        /* We're clean */
        fb->flags ^= SW_FB_DIRTY_BITS;
    }
}

static void GLAPIENTRY sw_db_ReadBuffer(GLenum mode)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    
    /* All good */
    if(ctx->readmode == mode)
        return;
    
    /* We don't support stereoscopic formats */
    if(mode == GL_FRONT)
        mode = GL_FRONT_LEFT;
    
    if(mode == GL_BACK)
        mode = GL_BACK_LEFT;
    
    /* Validate the asked mode */
    if((mode != GL_FRONT_LEFT) || (mode != GL_BACK_LEFT))
    {
        ERR("Incompatble mode passed: %lx.\n", mode);
        return;
    }

    /* Save it */
    ctx->readmode = mode;
}

static void GLAPIENTRY sw_db_GetBooleanv(GLenum pname, GLboolean* params)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    switch(pname)
    {
        case GL_READ_BUFFER:
            /* Well, it's never 0, but eh, the spec you know */
            *params = (ctx->readmode != 0);
            return;
        default:
            pGetBooleanv(pname, params);
            return;
    }
}

static void GLAPIENTRY sw_db_GetDoublev(GLenum pname, GLdouble* params)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    switch(pname)
    {
        case GL_READ_BUFFER:
            *params = (GLdouble)ctx->readmode;
            return;
        default:
            pGetDoublev(pname, params);
            return;
    }
}

static void GLAPIENTRY sw_db_GetFloatv(GLenum pname, GLfloat* params)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    switch(pname)
    {
        case GL_READ_BUFFER:
            *params = (GLfloat)ctx->readmode;
            return;
        default:
            pGetFloatv(pname, params);
            return;
    }
}

static void GLAPIENTRY sw_db_GetIntegerv(GLenum pname, GLint* params)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    switch(pname)
    {
        case GL_READ_BUFFER:
            *params = ctx->readmode;
            return;
        default:
            pGetIntegerv(pname, params);
            return;
    }
}

static void GLAPIENTRY sw_db_ReadPixels(
  GLint x,
  GLint y,
  GLsizei width,
  GLsizei height,
  GLenum format,
  GLenum type,
  GLvoid *pixels
)
{
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    struct wgl_dc_data* dc_data = IntGetCurrentDcData();
    struct sw_framebuffer* fb = dc_data->sw_data;
    
    /* Quick path */
    if(ctx->readmode == GL_BACK_LEFT)
    {
        pReadPixels(x, y, width, height, format, type, pixels);
        return;
    }
    
    assert(ctx->readmode == GL_FRONT_LEFT);
    
    /* Update frontbuffer */
    if(fb->flags & SW_FB_DIRTY)
        sw_db_update_frontbuffer(fb, IntGetCurrentDC());
    
    /* Finish wahtever is going on on backbuffer */
    pFinish();
    /* Tell osmesa we changed the buffer */
    pOSMesaMakeCurrent(ctx->mesa_ctx, fb->frontbuffer, GL_UNSIGNED_BYTE, width, height);
    
    /* Go ahead */
    pReadPixels(x, y, width, height, format, type, pixels);
    
    /* Go back to backbuffer operation */
    pOSMesaMakeCurrent(ctx->mesa_ctx, fb->backbuffer, GL_UNSIGNED_BYTE, width, height);
}

/* WGL <-> OSMesa functions */
static UINT index_from_format(struct wgl_dc_data* dc_data, INT format, BOOL* doubleBuffered)
{
    UINT index, nb_win_compat = 0, start_win_compat = 0;
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
    UINT index, nb_win_compat = 0, start_win_compat = 0;
    INT ret = sizeof(pixel_formats)/sizeof(pixel_formats[0]);
    
    if(GetObjectType(hdc) == OBJ_DC)
    {
        /* Find the window compatible formats */
        INT bpp = GetDeviceCaps(hdc, BITSPIXEL);
        for(index = 0; index<sizeof(pixel_formats)/sizeof(pixel_formats[0]); index++)
        {
            if(pixel_formats[index].color_bits == bpp)
            {
                if(!start_win_compat)
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

BOOL sw_SetPixelFormat(struct wgl_dc_data* dc_data, INT format)
{
    struct sw_framebuffer* fb;
    BOOL doubleBuffered;

    if(hMesaDll != NULL)
        goto osmesa_loaded;

    /* So, someone is crazy enough to ask for sw implementation. Load it. */
    TRACE("OpenGL software implementation START!\n");
    
    hMesaDll = LoadLibrary("osmesa.dll");
    if(!hMesaDll)
    {
        ERR("Failed loading osmesa.dll.\n");
        return FALSE;
    }
    
#define LOAD_PROC(x) do                                     \
{                                                           \
    p## x = (void*)GetProcAddress(hMesaDll, #x);                   \
    if(!p##x)                                               \
    {                                                       \
        ERR("Failed loading " #x " from osmesa.dll.\n");    \
        FreeLibrary(hMesaDll);                              \
        hMesaDll = NULL;                                    \
        return FALSE;                                       \
    }                                                       \
} while(0)
    LOAD_PROC(OSMesaCreateContextExt);
    LOAD_PROC(OSMesaDestroyContext);
    LOAD_PROC(OSMesaMakeCurrent);
    LOAD_PROC(OSMesaPixelStore);
    LOAD_PROC(OSMesaGetProcAddress);
#undef LOAD_PROC
    
    /* Load the GL api entries */
#define USE_GL_FUNC(x) do                                                       \
{                                                                               \
    sw_table_db.glDispatchTable.x = (void*)GetProcAddress(hMesaDll, "gl" #x);   \
    if(!sw_table_db.glDispatchTable.x)                                          \
    {                                                                           \
        ERR("Failed loading gl" #x " from osmesa.dll.\n");                      \
        FreeLibrary(hMesaDll);                                                  \
        hMesaDll = NULL;                                                        \
        return FALSE;                                                           \
    }                                                                           \
    sw_table_sb.glDispatchTable.x = sw_table_db.glDispatchTable.x;              \
} while(0);
    #include "glfuncs.h"
#undef USE_GL_FUNC
    /* For completeness */
    sw_table_db.cEntries = sw_table_sb.cEntries = OPENGL_VERSION_110_ENTRIES;
    
    /* We are not really single/double buffered. */
#define SWAP_SB_FUNC(x)  do                     \
{                                               \
    p##x = sw_table_sb.glDispatchTable.x;       \
    sw_table_sb.glDispatchTable.x = sw_sb_##x;  \
} while(0)
    SWAP_SB_FUNC(Finish);
#undef SWAP_SB_FUNC
#define SWAP_DB_FUNC(x)  do                     \
{                                               \
    p##x = sw_table_db.glDispatchTable.x;       \
    sw_table_db.glDispatchTable.x = sw_db_##x;  \
} while(0)
    SWAP_DB_FUNC(ReadBuffer);
    SWAP_DB_FUNC(ReadPixels);
    SWAP_DB_FUNC(GetBooleanv);
    SWAP_DB_FUNC(GetIntegerv);
    SWAP_DB_FUNC(GetFloatv);
    SWAP_DB_FUNC(GetDoublev);
#undef SWAP_DB_FUNC

    /* OpenGL spec: flush == all pending commands are sent to the server, 
     * and the client will receive the data in finished time.
     * We will call this finish in our case */
    sw_table_sb.glDispatchTable.Flush = sw_sb_Finish;

osmesa_loaded:
    /* Now allocate our structure */
    fb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*fb));
    if(!fb)
        return FALSE;
    /* Get the format index */
    fb->format_index = index_from_format(dc_data, format, &doubleBuffered);
    fb->flags = doubleBuffered ? SW_FB_DOUBLEBUFFERED : 0;
    /* Everything went fine */
    dc_data->sw_data = fb;
    return TRUE;
}

DHGLRC sw_CreateContext(struct wgl_dc_data* dc_data)
{
    struct sw_context *context;
    struct sw_framebuffer* fb = dc_data->sw_data;
    
    context = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context));
    if(!context)
        return NULL;

    context->mesa_ctx = pOSMesaCreateContextExt(pixel_formats[fb->format_index].mesa,
                                                pixel_formats[fb->format_index].depth_bits,
                                                pixel_formats[fb->format_index].stencil_bits,
                                                pixel_formats[fb->format_index].accum_bits, 0 );
    
    if(!context->mesa_ctx)
    {
        HeapFree( GetProcessHeap(), 0, context );
        return NULL;
    }
    /* Set defaults */
    context->hook = NULL;
    context->readmode = GL_BACK_LEFT;
    return (DHGLRC)context;
}

BOOL sw_DeleteContext(DHGLRC dhglrc)
{
    struct sw_context* context = (struct sw_context*)dhglrc;
    
    pOSMesaDestroyContext( context->mesa_ctx );
    HeapFree( GetProcessHeap(), 0, context );
    
    return TRUE;
}

void sw_ReleaseContext(DHGLRC dhglrc)
{
    struct sw_context* context = (struct sw_context*)dhglrc;
    
    /* Pass to osmesa the fact that there is no active context anymore */
    pOSMesaMakeCurrent( NULL, NULL, GL_UNSIGNED_BYTE, 0, 0 );
    
    /* Un-own */
    context->framebuffer = NULL;
    
    /* Unhook */
    if(context->hook)
    {
        UnhookWindowsHookEx(context->hook);
        context->hook = NULL;
    }
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
            UINT width, height, widthBytes;
            GetClientRect(pParams->hwnd, &client_rect);
            width = client_rect.right - client_rect.left;
            height = client_rect.bottom - client_rect.top;
            /* Do not reallocate for minimized windows */
            if(width <= 0 || height <= 0)
                goto end;
            /* Do not bother with unchanged size */
            if(width == fb->bmi.bmiHeader.biWidth && height == fb->bmi.bmiHeader.biHeight)
                goto end;
            /* Resize the buffer accordingly */
            widthBytes = WIDTH_BYTES_ALIGN32(width, pixel_formats[fb->format_index].color_bits);
            fb->bits = HeapReAlloc(GetProcessHeap(), 0, fb->bits, widthBytes * height);
            TRACE("New res: %lux%lu.\n", width, height);
            /* Update this */
            fb->bmi.bmiHeader.biWidth = width;
            fb->bmi.bmiHeader.biHeight = height;
            /* Re-enable osmesa */
            pOSMesaMakeCurrent(ctx->mesa_ctx, fb->bits, GL_UNSIGNED_BYTE, width, height);
            pOSMesaPixelStore(OSMESA_ROW_LENGTH, widthBytes * 8 / pixel_formats[fb->format_index].color_bits);
            /* Mark dirty bit acordingly */
            fb->flags |= SW_FB_DIRTY_SIZE;
        }
    }

end:
    return CallNextHookEx(ctx->hook, nCode, wParam, lParam);
}

const GLCLTPROCTABLE* sw_SetContext(struct wgl_dc_data* dc_data, DHGLRC dhglrc)
{
    struct sw_context* context = (struct sw_context*)dhglrc;
    struct sw_framebuffer* fb = dc_data->sw_data;
    UINT width, height, widthBytes;
    void* bits = NULL;

    if(dc_data->flags & WGL_DC_OBJ_DC)
    {
        HWND hwnd = dc_data->owner.hwnd;
        RECT client_rect;
        if(!hwnd)
        {
            ERR("Physical DC without a window!\n");
            return NULL;
        }
        if(!GetClientRect(hwnd, &client_rect))
        {
            ERR("GetClientRect failed!\n");
            return NULL;
        }
        /* This is a physical DC. Setup the hook */
        context->hook = SetWindowsHookEx(WH_CALLWNDPROC,
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
            return NULL;
        }
        if(GetObject(hbmp, sizeof(bm), &bm) == 0)
        {
            ERR("GetObject failed!\n");
            return NULL;
        }
        
        widthBytes = bm.bmWidthBytes;
        width = bm.bmWidth;
        height = bm.bmHeight;
        bits = bm.bmBits;
    }
    
    if(!width) width = 1;
    if(!height) height = 1;
    
    TRACE("Res: %lux%lu.\n", width, height);
    
    if(bits)
    {
        assert(!(fb->flags & SW_FB_DOUBLEBUFFERED));
        if(fb->flags & SW_FB_FREE_BITS)
        {
            fb->flags ^= SW_FB_FREE_BITS;
            HeapFree(GetProcessHeap(), 0, fb->bits);
        }
        fb->flags |= SW_FB_DIBSECTION;
        fb->bits = bits;
    }
    else if((width != fb->bmi.bmiHeader.biWidth) || (height != fb->bmi.bmiHeader.biHeight))
    {
        widthBytes = WIDTH_BYTES_ALIGN32(width, pixel_formats[fb->format_index].color_bits);
        if(fb->flags & SW_FB_FREE_BITS)
            fb->bits = HeapReAlloc(GetProcessHeap(), 0, fb->bits, widthBytes * height);
        else
            fb->bits = HeapAlloc(GetProcessHeap(), 0, widthBytes * height);
        fb->flags |= (SW_FB_FREE_BITS | SW_FB_DIRTY_SIZE);
        fb->flags &= ~SW_FB_DIBSECTION;
    }
    
    /* Set details up */
    fb->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    fb->bmi.bmiHeader.biWidth = width;
    fb->bmi.bmiHeader.biHeight = height;
    fb->bmi.bmiHeader.biPlanes = 1;
    fb->bmi.bmiHeader.biBitCount = pixel_formats[fb->format_index].color_bits;
    fb->bmi.bmiHeader.biCompression = BI_RGB;
    fb->bmi.bmiHeader.biSizeImage = 0;
    fb->bmi.bmiHeader.biXPelsPerMeter = 0;
    fb->bmi.bmiHeader.biYPelsPerMeter = 0;
    fb->bmi.bmiHeader.biClrUsed = 0;
    fb->bmi.bmiHeader.biClrImportant = 0;
    
    if(!pOSMesaMakeCurrent(context->mesa_ctx, fb->bits, GL_UNSIGNED_BYTE, width, height))
    {
        ERR("OSMesaMakeCurrent failed!\n");
        /* Damn! Free everything */
        if(fb->flags & SW_FB_FREE_BITS)
        {
            HeapFree(GetProcessHeap(), 0, fb->bits);
            fb->flags ^= ~SW_FB_FREE_BITS;
        }
        fb->bits = NULL;
    
        /* Unhook */
        if(context->hook)
        {
            UnhookWindowsHookEx(context->hook);
            context->hook = NULL;
        }
        return NULL;
    }
    
    /* Don't forget to tell mesa how our image is organized */
    pOSMesaPixelStore(OSMESA_ROW_LENGTH, widthBytes * 8 / pixel_formats[fb->format_index].color_bits);
    
    /* Own the context */
    context->framebuffer = fb;

    return (fb->flags & SW_FB_DOUBLEBUFFERED) ? &sw_table_db : &sw_table_sb;
}

PROC sw_GetProcAddress(LPCSTR name)
{
    return (PROC)pOSMesaGetProcAddress(name);
}

BOOL sw_CopyContext(DHGLRC dhglrcSrc, DHGLRC dhglrcDst, UINT mask)
{
    FIXME("Software wglCopyContext is UNIMPLEMENTED, mask %lx.\n", mask);
    return FALSE;
}

BOOL sw_ShareLists(DHGLRC dhglrcSrc, DHGLRC dhglrcDst)
{
    FIXME("Software wglShareLists is UNIMPLEMENTED.\n");
    return FALSE;
}

BOOL sw_SwapBuffers(HDC hdc, struct wgl_dc_data* dc_data)
{
    struct sw_framebuffer* fb = dc_data->sw_data;
    
    if(!(fb->flags & SW_FB_DOUBLEBUFFERED))
        return TRUE;
    
    if(!(fb->bits))
        return TRUE;
    
    /* Finish before swapping */
    pFinish();
    
    /* We are now dirty */
    fb->flags |= SW_FB_DIRTY_BITS;
    
    return (SetDIBitsToDevice(hdc,
        0,
        0,
        fb->bmi.bmiHeader.biWidth,
        fb->bmi.bmiHeader.biHeight,
        0,
        0,
        0,
        fb->bmi.bmiHeader.biWidth,
        fb->bits,
        &fb->bmi,
        DIB_RGB_COLORS) != 0);
}