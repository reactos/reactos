/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/wgl.c
 * PURPOSE:              OpenGL32 DLL, WGL functions
 */

#include "opengl32.h"

#include <pseh/pseh2.h>

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

static CRITICAL_SECTION dc_data_cs = {NULL, -1, 0, 0, 0, 0};
static struct wgl_dc_data* dc_data_list = NULL;

LIST_ENTRY ContextListHead;

/* FIXME: suboptimal */
static
struct wgl_dc_data*
get_dc_data_ex(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr)
{
    HWND hwnd = NULL;
    struct wgl_dc_data* data;
    DWORD objType = GetObjectType(hdc);
    ULONG flags = 0;
    union
    {
        HWND hwnd;
        HDC hdc;
        HANDLE u;
    } id;

    /* Look for the right data identifier */
    if(objType == OBJ_DC)
    {
        hwnd = WindowFromDC(hdc);
        if(!hwnd)
            return NULL;
        id.hwnd = hwnd;
        flags = WGL_DC_OBJ_DC;
    }
    else if(objType == OBJ_MEMDC)
    {
        id.hdc = hdc;
    }
    else
    {
        return NULL;
    }

    EnterCriticalSection(&dc_data_cs);
    data = dc_data_list;
    while(data)
    {
        if(data->owner.u == id.u)
        {
            LeaveCriticalSection(&dc_data_cs);
            return data;
        }
        data = data->next;
    }
    data= HeapAlloc(GetProcessHeap(), 0, sizeof(*data));
    if(!data)
    {
        LeaveCriticalSection(&dc_data_cs);
        return NULL;
    }
    /* initialize the structure */
    data->owner.u = id.u;
    data->flags = flags;
    data->pixelformat = 0;
    data->sw_data = NULL;
    /* Load the driver */
    data->icd_data = IntGetIcdData(hdc);
    /* Get the number of available formats for this DC once and for all */
    if(data->icd_data)
        data->nb_icd_formats = data->icd_data->DrvDescribePixelFormat(hdc, format, size, descr);
    else
        data->nb_icd_formats = 0;
    TRACE("ICD %S has %u formats for HDC %x.\n", data->icd_data ? data->icd_data->DriverName : NULL, data->nb_icd_formats, hdc);
    data->nb_sw_formats = sw_DescribePixelFormat(hdc, 0, 0, NULL);
    data->next = dc_data_list;
    dc_data_list = data;
    LeaveCriticalSection(&dc_data_cs);
    return data;
}

static
struct wgl_dc_data*
get_dc_data(HDC hdc)
{
    return get_dc_data_ex(hdc, 0, 0, NULL);
}

void release_dc_data(struct wgl_dc_data* dc_data)
{
    (void)dc_data;
}

struct wgl_context* get_context(HGLRC hglrc)
{
    struct wgl_context* context = (struct wgl_context*)hglrc;

    if(!hglrc)
        return NULL;

    _SEH2_TRY
    {
        if(context->magic != 'GLRC')
            context = NULL;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        context = NULL;
    }
    _SEH2_END;

    return context;
}

INT WINAPI wglDescribePixelFormat(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR *descr )
{
    struct wgl_dc_data* dc_data = get_dc_data_ex(hdc, format, size, descr);
    INT ret;

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    ret = dc_data->nb_icd_formats + dc_data->nb_sw_formats;

    if(!descr)
    {
        release_dc_data(dc_data);
        return ret;
    }
    if((format <= 0) || (format > ret) || (size < sizeof(*descr)))
    {
        release_dc_data(dc_data);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Query ICD if needed */
    if(format <= dc_data->nb_icd_formats)
    {
        struct ICD_Data* icd_data = dc_data->icd_data;
        /* SetPixelFormat may have NULLified this */
        if (!icd_data)
            icd_data = IntGetIcdData(hdc);
        if(!icd_data->DrvDescribePixelFormat(hdc, format, size, descr))
        {
            ret = 0;
        }
    }
    else
    {
        /* This is a software format */
        format -= dc_data->nb_icd_formats;
        if(!sw_DescribePixelFormat(hdc, format, size, descr))
        {
            ret = 0;
        }
    }

    release_dc_data(dc_data);
    return ret;
}

INT WINAPI wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd)
{
    PIXELFORMATDESCRIPTOR format, best;
    int i, count, best_format;
    int bestDBuffer = -1, bestStereo = -1;

    TRACE_(wgl)( "%p %p: size %u version %u flags %u type %u color %u %u,%u,%u,%u "
                 "accum %u depth %u stencil %u aux %u\n",
                 hdc, ppfd, ppfd->nSize, ppfd->nVersion, ppfd->dwFlags, ppfd->iPixelType,
                 ppfd->cColorBits, ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits, ppfd->cAlphaBits,
                 ppfd->cAccumBits, ppfd->cDepthBits, ppfd->cStencilBits, ppfd->cAuxBuffers );

    count = wglDescribePixelFormat( hdc, 0, 0, NULL );
    if (!count) return 0;

    best_format = 0;
    best.dwFlags = PFD_GENERIC_FORMAT;
    best.cAlphaBits = -1;
    best.cColorBits = -1;
    best.cDepthBits = -1;
    best.cStencilBits = -1;
    best.cAuxBuffers = -1;

    for (i = 1; i <= count; i++)
    {
        if (!wglDescribePixelFormat( hdc, i, sizeof(format), &format )) continue;

        if (ppfd->iPixelType != format.iPixelType)
        {
            TRACE( "pixel type mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* only use bitmap capable formats for bitmap rendering */
        if ((ppfd->dwFlags & PFD_DRAW_TO_BITMAP) && !(format.dwFlags & PFD_DRAW_TO_BITMAP))
        {
            TRACE( "PFD_DRAW_TO_BITMAP mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* only use window capable formats for window rendering */
        if ((ppfd->dwFlags & PFD_DRAW_TO_WINDOW) && !(format.dwFlags & PFD_DRAW_TO_WINDOW))
        {
            TRACE( "PFD_DRAW_TO_WINDOW mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* only use opengl capable formats for opengl rendering */
        if ((ppfd->dwFlags & PFD_SUPPORT_OPENGL) && !(format.dwFlags & PFD_SUPPORT_OPENGL))
        {
            TRACE( "PFD_SUPPORT_OPENGL mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* only use GDI capable formats for GDI rendering */
        if ((ppfd->dwFlags & PFD_SUPPORT_GDI) && !(format.dwFlags & PFD_SUPPORT_GDI))
        {
            TRACE( "PFD_SUPPORT_GDI mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        /* The behavior of PDF_STEREO/PFD_STEREO_DONTCARE and PFD_DOUBLEBUFFER / PFD_DOUBLEBUFFER_DONTCARE
         * is not very clear on MSDN. They specify that ChoosePixelFormat tries to match pixel formats
         * with the flag (PFD_STEREO / PFD_DOUBLEBUFFERING) set. Otherwise it says that it tries to match
         * formats without the given flag set.
         * A test on Windows using a Radeon 9500pro on WinXP (the driver doesn't support Stereo)
         * has indicated that a format without stereo is returned when stereo is unavailable.
         * So in case PFD_STEREO is set, formats that support it should have priority above formats
         * without. In case PFD_STEREO_DONTCARE is set, stereo is ignored.
         *
         * To summarize the following is most likely the correct behavior:
         * stereo not set -> prefer non-stereo formats, but also accept stereo formats
         * stereo set -> prefer stereo formats, but also accept non-stereo formats
         * stereo don't care -> it doesn't matter whether we get stereo or not
         *
         * In Wine we will treat non-stereo the same way as don't care because it makes
         * format selection even more complicated and second drivers with Stereo advertise
         * each format twice anyway.
         */

        /* Doublebuffer, see the comments above */
        if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) &&
                ((format.dwFlags & PFD_DOUBLEBUFFER) == (ppfd->dwFlags & PFD_DOUBLEBUFFER)))
                goto found;

            if (bestDBuffer != -1 && (format.dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) continue;
        }

        /* Stereo, see the comments above. */
        if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((format.dwFlags & PFD_STEREO) == (ppfd->dwFlags & PFD_STEREO)))
                goto found;

            if (bestStereo != -1 && (format.dwFlags & PFD_STEREO) != bestStereo) continue;
        }

        /* Below we will do a number of checks to select the 'best' pixelformat.
         * We assume the precedence cColorBits > cAlphaBits > cDepthBits > cStencilBits -> cAuxBuffers.
         * The code works by trying to match the most important options as close as possible.
         * When a reasonable format is found, we will try to match more options.
         * It appears (see the opengl32 test) that Windows opengl drivers ignore options
         * like cColorBits, cAlphaBits and friends if they are set to 0, so they are considered
         * as DONTCARE. At least Serious Sam TSE relies on this behavior. */

        if (ppfd->cColorBits)
        {
            if (((ppfd->cColorBits > best.cColorBits) && (format.cColorBits > best.cColorBits)) ||
                ((format.cColorBits >= ppfd->cColorBits) && (format.cColorBits < best.cColorBits)))
                goto found;

            if (best.cColorBits != format.cColorBits)  /* Do further checks if the format is compatible */
            {
                TRACE( "color mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAlphaBits)
        {
            if (((ppfd->cAlphaBits > best.cAlphaBits) && (format.cAlphaBits > best.cAlphaBits)) ||
                ((format.cAlphaBits >= ppfd->cAlphaBits) && (format.cAlphaBits < best.cAlphaBits)))
                goto found;

            if (best.cAlphaBits != format.cAlphaBits)
            {
                TRACE( "alpha mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cDepthBits)
        {
            if (((ppfd->cDepthBits > best.cDepthBits) && (format.cDepthBits > best.cDepthBits)) ||
                ((format.cDepthBits >= ppfd->cDepthBits) && (format.cDepthBits < best.cDepthBits)))
                goto found;

            if (best.cDepthBits != format.cDepthBits)
            {
                TRACE( "depth mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cStencilBits)
        {
            if (((ppfd->cStencilBits > best.cStencilBits) && (format.cStencilBits > best.cStencilBits)) ||
                ((format.cStencilBits >= ppfd->cStencilBits) && (format.cStencilBits < best.cStencilBits)))
                goto found;

            if (best.cStencilBits != format.cStencilBits)
            {
                TRACE( "stencil mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAuxBuffers)
        {
            if (((ppfd->cAuxBuffers > best.cAuxBuffers) && (format.cAuxBuffers > best.cAuxBuffers)) ||
                ((format.cAuxBuffers >= ppfd->cAuxBuffers) && (format.cAuxBuffers < best.cAuxBuffers)))
                goto found;

            if (best.cAuxBuffers != format.cAuxBuffers)
            {
                TRACE( "aux mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        continue;

    found:
        /* Prefer HW accelerated formats */
        if ((format.dwFlags & PFD_GENERIC_FORMAT) && !(best.dwFlags & PFD_GENERIC_FORMAT))
            continue;
        best_format = i;
        best = format;
        bestDBuffer = format.dwFlags & PFD_DOUBLEBUFFER;
        bestStereo = format.dwFlags & PFD_STEREO;
    }

    TRACE( "returning %u\n", best_format );
    return best_format;
}

BOOL WINAPI wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
    struct wgl_context* ctx_src = get_context(hglrcSrc);
    struct wgl_context* ctx_dst = get_context(hglrcDst);

    if(!ctx_src || !ctx_dst)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Check this is the same pixel format */
    if((ctx_dst->icd_data != ctx_src->icd_data) ||
        (ctx_dst->pixelformat != ctx_src->pixelformat))
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return FALSE;
    }

    if(ctx_src->icd_data)
        return ctx_src->icd_data->DrvCopyContext(ctx_src->dhglrc, ctx_dst->dhglrc, mask);

    return sw_CopyContext(ctx_src->dhglrc, ctx_dst->dhglrc, mask);
}

HGLRC WINAPI wglCreateContext(HDC hdc)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);
    struct wgl_context* context;
    DHGLRC dhglrc;

    TRACE("Creating context for %p.\n", hdc);

    if(!dc_data)
    {
        WARN("Not a DC handle!\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    if(!dc_data->pixelformat)
    {
        WARN("Pixel format not set!\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if(!dc_data->icd_data)
    {
        TRACE("Calling SW implementation.\n");
        dhglrc = sw_CreateContext(dc_data);
        TRACE("done\n");
    }
    else
    {
        TRACE("Calling ICD.\n");
        dhglrc = dc_data->icd_data->DrvCreateContext(hdc);
    }

    if(!dhglrc)
    {
        WARN("Failed!\n");
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    context = HeapAlloc(GetProcessHeap(), 0, sizeof(*context));
    if(!context)
    {
        WARN("Failed to allocate a context!\n");
        if(!dc_data->icd_data)
            sw_DeleteContext(dhglrc);
        else
            dc_data->icd_data->DrvDeleteContext(dhglrc);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    /* Copy info from the DC data */
    context->dhglrc = dhglrc;
    context->icd_data = dc_data->icd_data;
    context->pixelformat = dc_data->pixelformat;
    context->thread_id = 0;

    /* Insert into the list */
    InsertTailList(&ContextListHead, &context->ListEntry);

    context->magic = 'GLRC';
    TRACE("Success!\n");
    return (HGLRC)context;
}

HGLRC WINAPI wglCreateLayerContext(HDC hdc, int iLayerPlane)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);
    struct wgl_context* context;
    DHGLRC dhglrc;

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    if(!dc_data->pixelformat)
    {
        release_dc_data(dc_data);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    if(!dc_data->icd_data)
    {
        if(iLayerPlane != 0)
        {
            /* Not supported in SW implementation  */
            release_dc_data(dc_data);
            SetLastError(ERROR_INVALID_PIXEL_FORMAT);
            return NULL;
        }
        dhglrc = sw_CreateContext(dc_data);
    }
    else
    {
        dhglrc = dc_data->icd_data->DrvCreateLayerContext(hdc, iLayerPlane);
    }

    if(!dhglrc)
    {
        release_dc_data(dc_data);
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }

    context = HeapAlloc(GetProcessHeap(), 0, sizeof(*context));
    if(!context)
    {
        if(!dc_data->icd_data)
            sw_DeleteContext(dhglrc);
        else
            dc_data->icd_data->DrvDeleteContext(dhglrc);
        release_dc_data(dc_data);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    /* Copy info from the DC data */
    context->dhglrc = dhglrc;
    context->icd_data = dc_data->icd_data;
    context->pixelformat = dc_data->pixelformat;
    context->thread_id = 0;

    context->magic = 'GLRC';

    release_dc_data(dc_data);
    return (HGLRC)context;
}

BOOL WINAPI wglDeleteContext(HGLRC hglrc)
{
    struct wgl_context* context = get_context(hglrc);
    LONG thread_id = GetCurrentThreadId();

    if(!context)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Own this context before touching it */
    if(InterlockedCompareExchange(&context->thread_id, thread_id, 0) != 0)
    {
        /* We can't delete a context current to another thread */
        if(context->thread_id != thread_id)
        {
            SetLastError(ERROR_BUSY);
            return FALSE;
        }

        /* This is in our thread. Release and try again */
        if(!wglMakeCurrent(NULL, NULL))
            return FALSE;
        return wglDeleteContext(hglrc);
    }

    if(context->icd_data)
        context->icd_data->DrvDeleteContext(context->dhglrc);
    else
        sw_DeleteContext(context->dhglrc);

    context->magic = 0;
    RemoveEntryList(&context->ListEntry);
    HeapFree(GetProcessHeap(), 0, context);

    return TRUE;
}

BOOL WINAPI wglDescribeLayerPlane(HDC hdc,
                                  int iPixelFormat,
                                  int iLayerPlane,
                                  UINT nBytes,
                                  LPLAYERPLANEDESCRIPTOR plpd)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(iPixelFormat <= dc_data->nb_icd_formats)
        return dc_data->icd_data->DrvDescribeLayerPlane(hdc, iPixelFormat, iLayerPlane, nBytes, plpd);

    /* SW implementation doesn't support this */
    return FALSE;
}

HGLRC WINAPI wglGetCurrentContext(void)
{
    return IntGetCurrentRC();
}

HDC WINAPI wglGetCurrentDC(void)
{
    return IntGetCurrentDC();
}

PROC WINAPI wglGetDefaultProcAddress(LPCSTR lpszProc)
{
    /* undocumented... */
    return NULL;
}

int WINAPI wglGetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF* pcr )
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if(!dc_data->pixelformat)
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return 0;
    }

    if(dc_data->icd_data)
        return dc_data->icd_data->DrvGetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);

    /* SW implementation doesn't support this */
    return 0;
}

INT WINAPI wglGetPixelFormat(HDC hdc)
{
    INT ret;
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    ret = dc_data->pixelformat;
    release_dc_data(dc_data);
    return ret;
}

PROC WINAPI wglGetProcAddress(LPCSTR name)
{
    struct wgl_context* context = get_context(IntGetCurrentRC());
    if(!context)
        return NULL;

    /* This shall fail for opengl 1.1 functions */
#define USE_GL_FUNC(func, w, x, y, z) if(!strcmp(name, "gl" #func)) return NULL;
#include "glfuncs.h"

    /* Forward */
    if(context->icd_data)
        return context->icd_data->DrvGetProcAddress(name);
    return sw_GetProcAddress(name);
}

void APIENTRY set_api_table(const GLCLTPROCTABLE* table)
{
    IntSetCurrentDispatchTable(&table->glDispatchTable);
}

BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    struct wgl_context* ctx = get_context(hglrc);
    struct wgl_context* old_ctx = get_context(IntGetCurrentRC());
    const GLCLTPROCTABLE* apiTable;
    LONG thread_id = (LONG)GetCurrentThreadId();

    if(ctx)
    {
        struct wgl_dc_data* dc_data = get_dc_data(hdc);
        if(!dc_data)
        {
            ERR("wglMakeCurrent was passed an invalid DC handle.\n");
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        /* Check compatibility */
        if((ctx->icd_data != dc_data->icd_data) || (ctx->pixelformat != dc_data->pixelformat))
        {
            /* That's bad, man */
            ERR("HGLRC %p and HDC %p are not compatible.\n", hglrc, hdc);
            release_dc_data(dc_data);
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        /* Set the thread ID */
        if(InterlockedCompareExchange(&ctx->thread_id, thread_id, 0) != 0)
        {
            /* Already current for a thread. Maybe it's us ? */
            release_dc_data(dc_data);
            if(ctx->thread_id != thread_id)
                SetLastError(ERROR_BUSY);
            return (ctx->thread_id == thread_id);
        }

        if(old_ctx)
        {
            /* Unset it */
            if(old_ctx->icd_data)
                old_ctx->icd_data->DrvReleaseContext(old_ctx->dhglrc);
            else
                sw_ReleaseContext(old_ctx->dhglrc);
            InterlockedExchange(&old_ctx->thread_id, 0);
        }

        /* Call the ICD or SW implementation */
        if(ctx->icd_data)
        {
            apiTable = ctx->icd_data->DrvSetContext(hdc, ctx->dhglrc, set_api_table);
            if(!apiTable)
            {
                ERR("DrvSetContext failed!\n");
                /* revert */
                InterlockedExchange(&ctx->thread_id, 0);
                IntSetCurrentDispatchTable(NULL);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            set_api_table(apiTable);
            /* Make it current */
            IntMakeCurrent(hglrc, hdc, dc_data);
        }
        else
        {
            /* We must set current before, SW implementation relies on it */
            IntMakeCurrent(hglrc, hdc, dc_data);
            if(!sw_SetContext(dc_data, ctx->dhglrc))
            {
                ERR("sw_SetContext failed!\n");
                /* revert */
                IntMakeCurrent(NULL, NULL, NULL);
                InterlockedExchange(&ctx->thread_id, 0);
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }
    }
    else if(old_ctx)
    {
        if(old_ctx->icd_data)
            old_ctx->icd_data->DrvReleaseContext(old_ctx->dhglrc);
        else
            sw_ReleaseContext(old_ctx->dhglrc);
        InterlockedExchange(&old_ctx->thread_id, 0);
        /* Unset it */
        IntMakeCurrent(NULL, NULL, NULL);
        IntSetCurrentDispatchTable(NULL);
        /* Test conformance (extreme cases) */
        return hglrc == NULL;
    }
    else
    {
        /* Winetest conformance */
        DWORD objType = GetObjectType(hdc);
        if (objType != OBJ_DC && objType != OBJ_MEMDC)
        {
            if (hdc)
            {
                ERR("hdc (%p) is not a DC handle (ObjectType: %d)!\n", hdc, objType);
            }
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }
    }

    return TRUE;
}

BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
                                   int iLayerPlane,
                                   BOOL bRealize)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!dc_data->pixelformat)
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return FALSE;
    }

    if(dc_data->icd_data)
        return dc_data->icd_data->DrvRealizeLayerPalette(hdc, iLayerPlane, bRealize);

    /* SW implementation doesn't support this */
    return FALSE;
}

int WINAPI wglSetLayerPaletteEntries(HDC hdc,
                                     int iLayerPlane,
                                     int iStart,
                                     int cEntries,
                                     const COLORREF *pcr)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if(!dc_data->pixelformat)
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return 0;
    }

    if(dc_data->icd_data)
        return dc_data->icd_data->DrvSetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);

    /* SW implementation doesn't support this */
    return 0;
}

BOOL WINAPI wglSetPixelFormat(HDC hdc, INT format, const PIXELFORMATDESCRIPTOR *descr)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);
    INT sw_format;
    BOOL ret;

    TRACE("HDC %p, format %i.\n", hdc, format);

    if(!dc_data)
    {
        WARN("Not a valid DC!.\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!format)
    {
        WARN("format == 0!\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(dc_data->pixelformat)
    {
        TRACE("DC format already set, %i.\n", dc_data->pixelformat);
        return (format == dc_data->pixelformat);
    }

    if(format <= dc_data->nb_icd_formats)
    {
        TRACE("Calling ICD.\n");
        ret = dc_data->icd_data->DrvSetPixelFormat(hdc, format);
        if(ret)
        {
            TRACE("Success!\n");
            dc_data->pixelformat = format;
        }
        return ret;
    }

    sw_format = format - dc_data->nb_icd_formats;
    if(sw_format <= dc_data->nb_sw_formats)
    {
        TRACE("Calling SW implementation.\n");
        ret = sw_SetPixelFormat(hdc, dc_data, sw_format);
        if(ret)
        {
            TRACE("Success!\n");
            /* This is now officially a software-only HDC */
            dc_data->icd_data = NULL;
            dc_data->pixelformat = format;
        }
        return ret;
    }

    TRACE("Invalid pixel format!\n");
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL WINAPI wglShareLists(HGLRC hglrcSrc, HGLRC hglrcDst)
{
    struct wgl_context* ctx_src = get_context(hglrcSrc);
    struct wgl_context* ctx_dst = get_context(hglrcDst);

    if(!ctx_src || !ctx_dst)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Check this is the same pixel format */
    if((ctx_dst->icd_data != ctx_src->icd_data) ||
        (ctx_dst->pixelformat != ctx_src->pixelformat))
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return FALSE;
    }

    if(ctx_src->icd_data)
        return ctx_src->icd_data->DrvShareLists(ctx_src->dhglrc, ctx_dst->dhglrc);

    return sw_ShareLists(ctx_src->dhglrc, ctx_dst->dhglrc);
}

BOOL WINAPI DECLSPEC_HOTPATCH wglSwapBuffers(HDC hdc)
{
    struct wgl_dc_data* dc_data = get_dc_data(hdc);

    if(!dc_data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!dc_data->pixelformat)
    {
        SetLastError(ERROR_INVALID_PIXEL_FORMAT);
        return FALSE;
    }

    if(dc_data->icd_data)
        return dc_data->icd_data->DrvSwapBuffers(hdc);

    return sw_SwapBuffers(hdc, dc_data);
}

BOOL WINAPI wglSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
    return FALSE;
}

DWORD WINAPI wglSwapMultipleBuffers(UINT count, CONST WGLSWAP * toSwap)
{
    return 0;
}

/* Clean up on DLL unload */
void
IntDeleteAllContexts(void)
{
    struct wgl_context* context;
    LIST_ENTRY* Entry = ContextListHead.Flink;

    while (Entry != &ContextListHead)
    {
        context = CONTAINING_RECORD(Entry, struct wgl_context, ListEntry);
        wglDeleteContext((HGLRC)context);
        Entry = Entry->Flink;
    }
}
