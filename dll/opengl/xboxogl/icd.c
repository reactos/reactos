

#include "xboxogl.h"
#include <stdlib.h>
#include <debug.h>

#ifndef PFD_DRAW_TO_WINDOW
#define PFD_DRAW_TO_WINDOW   0x00000004
#define PFD_SUPPORT_OPENGL   0x00000020
#define PFD_GENERIC_FORMAT   0x00000040
#define PFD_TYPE_RGBA        0
#define PFD_MAIN_PLANE       0
#endif

/* Apperently every GL app uses this */
#define XGL_NUM_FORMATS 2

static const PIXELFORMATDESCRIPTOR g_pfds[XGL_NUM_FORMATS] =
{
    {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,                     /* cColorBits */
        8, 16, 8, 8, 8, 0,      /* RGB */
        0, 0,                   /* alpha */
        0, 0, 0, 0, 0,          /* accum */
        24, 8, 0,               /* depth (Z24S8 zeta), stencil, aux */
        PFD_MAIN_PLANE, 0, 0, 0, 0
    },
    {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
        PFD_TYPE_RGBA,
        32,                     /* cColorBits */
        8, 16, 8, 8, 8, 0,      /* RGB */
        0, 0,                   /* alpha */
        0, 0, 0, 0, 0,          /* accum */
        24, 8, 0,               /* depth (Z24S8 zeta), stencil, aux */
        PFD_MAIN_PLANE, 0, 0, 0, 0
    }
};

/* ----- Pixel format ------------------------------------------------------ */

int WINAPI DrvDescribePixelFormat(HDC hdc, int ifmt, UINT cj,
                                  LPPIXELFORMATDESCRIPTOR ppfd)
{
    (void)hdc;
    if (ppfd && cj >= sizeof(PIXELFORMATDESCRIPTOR) &&
        ifmt >= 1 && ifmt <= XGL_NUM_FORMATS)
        *ppfd = g_pfds[ifmt - 1];
    return XGL_NUM_FORMATS;   /* number of supported formats */
}

BOOL WINAPI DrvSetPixelFormat(HDC hdc, int ifmt)
{
    (void)hdc;
    return (ifmt >= 1 && ifmt <= XGL_NUM_FORMATS) ? TRUE : FALSE;
}

/* ----- Context lifecycle ------------------------------------------------ */

DHGLRC WINAPI DrvCreateContext(HDC hdc)
{
    PXGL_CONTEXT c;
    RECT rc;
    HWND hwnd;
    int  i;

    c = (PXGL_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*c));
    if (!c) return NULL;

    c->hdc = hdc;

    /* GL default state (HEAP_ZERO_MEMORY already cleared everything else). */
    c->curColor[0]=c->curColor[1]=c->curColor[2]=c->curColor[3]=1.0f;
    c->curNormal[2]=1.0f;
    c->curTexCoord[3]=1.0f;
    c->clearDepth   = 1.0f;
    c->depthFunc    = GL_LESS;
    c->depthMask    = GL_TRUE;
    c->shadeModel   = GL_SMOOTH;
    c->cullFaceMode = GL_BACK;
    c->frontFace    = GL_CCW;
    c->blendSrc     = GL_ONE;
    c->blendDst     = GL_ZERO;
    c->depthNear    = 0.0f;
    c->depthFar     = 1.0f;
    c->matrixMode   = GL_MODELVIEW;
    c->lastError    = GL_NO_ERROR;
    c->nextTexture  = 1;   /* 0 is the default (unnamed) texture */

    /* GL lighting defaults: global ambient 0.2; material ambient 0.2 / diffuse 0.8;
     * LIGHT0 diffuse white (others black), default direction toward the viewer. */
    c->globalAmbient[0]=c->globalAmbient[1]=c->globalAmbient[2]=0.2f; c->globalAmbient[3]=1.0f;
    c->matAmbient[0]=c->matAmbient[1]=c->matAmbient[2]=0.2f; c->matAmbient[3]=1.0f;
    c->matDiffuse[0]=c->matDiffuse[1]=c->matDiffuse[2]=0.8f; c->matDiffuse[3]=1.0f;
    c->matEmission[3]=1.0f;
    c->matSpecular[3]=1.0f;   /* default specular black, shininess 0 */
    c->pointSize = 1.0f; c->lineWidth = 1.0f;
    {
        int li;
        for (li = 0; li < XGL_MAX_LIGHTS; li++)
        {
            c->lights[li].pos[2] = 1.0f;          /* default direction (0,0,1,0) */
            c->lights[li].ambient[3] = 1.0f;
            c->lights[li].diffuse[3] = 1.0f;
            c->lights[li].specular[3] = 1.0f;
        }
        c->lights[0].diffuse[0]=c->lights[0].diffuse[1]=c->lights[0].diffuse[2]=1.0f; /* LIGHT0 white */
        c->lights[0].specular[0]=c->lights[0].specular[1]=c->lights[0].specular[2]=1.0f;
        c->lights[0].pos[0]=0.408f; c->lights[0].pos[1]=0.408f; c->lights[0].pos[2]=0.816f;
    }

    /* Fog defaults (GL): EXP, density 1, range [0,1], black. */
    c->fogMode = GL_EXP;
    c->fogStart = 0.0f; c->fogEnd = 1.0f; c->fogDensity = 1.0f;

    /* Alpha test / polygon mode defaults (GL): ALWAYS, ref 0, FILL. */
    c->alphaFunc = GL_ALWAYS;
    c->alphaRef = 0.0f;
    c->polygonMode = GL_FILL;

    /* GL 1.2 defaults: blend equation FUNC_ADD, constant blend colour (0,0,0,0),
     * single-colour lighting (specular folded into the primary colour). */
    c->blendEquation = GL_FUNC_ADD;
    c->blendColor[0]=c->blendColor[1]=c->blendColor[2]=c->blendColor[3]=0.0f;
    c->colorControl = GL_SINGLE_COLOR;
    c->colorMaterialMode = GL_AMBIENT_AND_DIFFUSE;   /* GL default tracked parameter */

    /* Remaining GL 1.1 feature defaults. */
    c->colorMask[0]=c->colorMask[1]=c->colorMask[2]=c->colorMask[3]=GL_TRUE;
    c->pixelZoomX = c->pixelZoomY = 1.0f;
    c->unpackAlignment = c->packAlignment = 4;
    c->renderMode = GL_RENDER;
    c->logicOp = GL_COPY;
    c->drawBuffer = c->readBuffer = GL_BACK;
    c->indexWriteMask = 0xFFFFFFFF;
    c->stencilWriteMask = c->stencilValueMask = 0xFFFFFFFF;
    c->stencilFunc = GL_ALWAYS;
    c->stencilFail = c->stencilZFail = c->stencilZPass = GL_KEEP;
    {
        int tg;
        for (tg = 0; tg < 4; tg++)
        {
            c->texGenMode[tg] = GL_EYE_LINEAR;   /* GL default */
            /* default S plane (1,0,0,0); T plane (0,1,0,0); R,Q all zero */
            c->texGenObjPlane[tg][tg < 2 ? tg : 0] = (tg < 2) ? 1.0f : 0.0f;
            c->texGenEyePlane[tg][tg < 2 ? tg : 0] = (tg < 2) ? 1.0f : 0.0f;
        }
    }

    /* Identity matrices on every stack. */
    for (i = 0; i < XGL_MAT_MODE_MAX; i++)
    {
        c->matTop[i] = 0;
        XglMatIdentity(&c->matStack[i][0]);
    }
    c->matMode = XGL_MAT_MODELVIEW;

    hwnd = WindowFromDC(hdc);
    if (hwnd && GetClientRect(hwnd, &rc))
    {
        c->width  = rc.right - rc.left;
        c->height = rc.bottom - rc.top;
    }
    else
    {
        c->width = 640; c->height = 480;
    }
    c->vpX = 0; c->vpY = 0; c->vpW = c->width; c->vpH = c->height;
    c->scissorX = 0; c->scissorY = 0; c->scissorW = c->width; c->scissorH = c->height;

    return (DHGLRC)c;
}

DHGLRC WINAPI DrvCreateLayerContext(HDC hdc, int layerPlane)
{
    if (layerPlane != 0)
        return NULL;
    return DrvCreateContext(hdc);
}

BOOL WINAPI DrvDeleteContext(DHGLRC dhglrc)
{
    PXGL_CONTEXT c = (PXGL_CONTEXT)dhglrc;
    GLuint i;
    if (!c) return FALSE;
    for (i = 0; i < XGL_MAX_TEXTURES; i++)
        if (c->textures[i].texels)
            HeapFree(GetProcessHeap(), 0, c->textures[i].texels);
    XglFreeLists(c);
    XglFreeEvalMaps(c);
    if (c->accumBuf) HeapFree(GetProcessHeap(), 0, c->accumBuf);
    if (XglCurrent() == c)
        XglSetCurrent(NULL);
    HeapFree(GetProcessHeap(), 0, c);
    return TRUE;
}

const GLCLTPROCTABLE * WINAPI DrvSetContext(HDC hdc, DHGLRC dhglrc,
                                            PFN_SETPROCTABLE callback)
{
    PXGL_CONTEXT c = (PXGL_CONTEXT)dhglrc;
    if (!c) return NULL;

    c->hdc = hdc;
    XglSetCurrent(c);
    XglDispatchInstallReal();

    if (callback)
        callback(&g_xglDispatchTable);

    return &g_xglDispatchTable;
}

void WINAPI DrvReleaseContext(DHGLRC dhglrc)
{
    (void)dhglrc;
    XglSetCurrent(NULL);
}

BOOL WINAPI DrvCopyContext(DHGLRC src, DHGLRC dst, UINT mask)
{
    (void)src; (void)dst; (void)mask;
    return FALSE; /* honest: state copy not implemented */
}

BOOL WINAPI DrvShareLists(DHGLRC ctx1, DHGLRC ctx2)
{
    (void)ctx1; (void)ctx2;
    return TRUE; /* we don't keep display lists, so sharing is a no-op */
}

/* ----- Buffer presentation --------------------------------------------- */

BOOL WINAPI DrvSwapBuffers(HDC hdc)
{
    (void)hdc;
    /* Present the software back buffer to the window. */
    XglPresentCurrent();
    return TRUE;
}

BOOL WINAPI DrvSwapLayerBuffers(HDC hdc, UINT planes)
{
    (void)planes;
    return DrvSwapBuffers(hdc);
}

/* ----- Layer planes (not supported) ------------------------------------ */

BOOL WINAPI DrvDescribeLayerPlane(HDC hdc, int ifmt, int layerPlane,
                                  UINT cb, LPLAYERPLANEDESCRIPTOR plpd)
{
    (void)hdc; (void)ifmt; (void)layerPlane; (void)cb; (void)plpd;
    return FALSE;
}

int WINAPI DrvSetLayerPaletteEntries(HDC hdc, int layerPlane,
                                     int start, int entries,
                                     CONST COLORREF *cr)
{
    (void)hdc; (void)layerPlane; (void)start; (void)entries; (void)cr;
    return 0;
}

int WINAPI DrvGetLayerPaletteEntries(HDC hdc, int layerPlane,
                                     int start, int entries,
                                     COLORREF *cr)
{
    (void)hdc; (void)layerPlane; (void)start; (void)entries; (void)cr;
    return 0;
}

BOOL WINAPI DrvRealizeLayerPalette(HDC hdc, int layerPlane, BOOL realize)
{
    (void)hdc; (void)layerPlane; (void)realize;
    return FALSE;
}

/* ----- WGL extensions --------------------------------------------------- */

/* WGL_EXT_swap_control.  Quake III (and most GL games) probe for this via
 * wglGetProcAddress("wglSwapIntervalEXT") to control vsync.  We present by
 * blitting the offscreen surface on SwapBuffers with no real vblank sync, so
 * the interval is tracked but not enforced; returning TRUE lets the app believe
 * swap-control works (the alternative — returning NULL — makes some apps treat
 * the GL as broken). */
BOOL WINAPI xgl_wglSwapIntervalEXT(int interval)
{
    PXGL_CONTEXT c = XglCurrent();
    if (interval < 0)
        return FALSE;
    if (c)
        c->swapInterval = interval;
    return TRUE;
}

int WINAPI xgl_wglGetSwapIntervalEXT(void)
{
    PXGL_CONTEXT c = XglCurrent();
    return c ? c->swapInterval : 0;
}

/* ----- Misc ------------------------------------------------------------- */

PROC WINAPI DrvGetProcAddress(LPCSTR name)
{
    DPRINT1("DrvGetProcAddress: %s\n", name);
    /* GL 1.2 (and the EXT extensions it promoted) entry points are NOT part of
     * the GL 1.1 GLCLTPROCTABLE that opengl32 exports directly, so applications
     * reach them through wglGetProcAddress, which calls us here.  Return our real
     * implementations from real.c; the GL_*_EXT aliases share the same code. */
    static const struct { const char *name; PROC fn; } table[] =
    {
        { "glDrawRangeElements",    (PROC)xgl_real_DrawRangeElements },
        { "glDrawRangeElementsEXT", (PROC)xgl_real_DrawRangeElements },
        { "glTexImage3D",           (PROC)xgl_real_TexImage3D },
        { "glTexImage3DEXT",        (PROC)xgl_real_TexImage3D },
        { "glTexSubImage3D",        (PROC)xgl_real_TexSubImage3D },
        { "glTexSubImage3DEXT",     (PROC)xgl_real_TexSubImage3D },
        { "glCopyTexSubImage3D",    (PROC)xgl_real_CopyTexSubImage3D },
        { "glCopyTexSubImage3DEXT", (PROC)xgl_real_CopyTexSubImage3D },
        { "glBlendColor",           (PROC)xgl_real_BlendColor },
        { "glBlendColorEXT",        (PROC)xgl_real_BlendColor },
        { "glBlendEquation",        (PROC)xgl_real_BlendEquation },
        { "glBlendEquationEXT",     (PROC)xgl_real_BlendEquation },
        /* WGL extensions */
        { "wglSwapIntervalEXT",     (PROC)xgl_wglSwapIntervalEXT },
        { "wglGetSwapIntervalEXT",  (PROC)xgl_wglGetSwapIntervalEXT },
    };
    int i;
    if (!name) return NULL;
    for (i = 0; i < (int)(sizeof(table)/sizeof(table[0])); i++)
        if (lstrcmpA(name, table[i].name) == 0)
            return table[i].fn;
    return NULL;
}

BOOL WINAPI DrvValidateVersion(DWORD version)
{
    /* Opengl32 passes the version it found in the registry.  Accept anything
     * non-zero; the contract is just "are we compatible". */
    return (version != 0);
}

void WINAPI DrvSetCallbackProcs(int nProcs, PROC *procs)
{
    /* opengl32 hands us its internal callbacks (e.g. wglGetCurrentValue) so
     * the ICD can stash per-context state without round-tripping through it.
     * Our context is stored in our own TLS slot, so we ignore the procs.
     * Accepting them silently is the documented behaviour for ICDs that
     * don't need callback wiring. */
    (void)nProcs; (void)procs;
}
