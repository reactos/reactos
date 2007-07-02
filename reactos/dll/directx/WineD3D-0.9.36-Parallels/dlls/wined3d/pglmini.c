#include "config.h"

#include <windows.h>

#include <GL/gl.h>

#include "pglmini.h"

#undef wglDescribePixelFormat
#undef wglSetPixelFormat
#undef wglMakeCurrent
#undef wglGetCurrentContext
#undef wglGetCurrentDC
#undef wglCreateContext
#undef wglDeleteContext
#undef wglGetProcAddress


BOOL WINE_WINAPI(*pwglMakeCurrent)(HDC hdc, HGLRC hgl);
HGLRC WINE_WINAPI(*pwglGetCurrentContext)(VOID);
HDC WINE_WINAPI(*pwglGetCurrentDC)(VOID);

int WINE_WINAPI(*pDrvDescribePixelFormat)(
    HDC hdc, int index, UINT size, LPPIXELFORMATDESCRIPTOR format);
BOOL WINE_WINAPI(*pDrvSetPixelFormat)(HDC hdc, int index);
PVOID WINE_APIENTRY(*pDrvSetContext)(HDC hdc, HGLRC hgl, PVOID);
VOID WINE_APIENTRY(*pDrvReleaseContext)(HGLRC hgl);
BOOL WINE_APIENTRY(*pDrvSwapBuffers)(HDC hdc);

HGLRC WINE_APIENTRY(*pDrvCreateContext)(HDC hdc);
BOOL WINE_APIENTRY(*pDrvDeleteContext)(HGLRC hgl);
PROC WINE_APIENTRY(*pDrvGetProcAddress)(LPCSTR name);

struct pgl_iat_s pgl_iat;

static const char *pgl_names[] = {
	"glAlphaFunc",
	"glBegin",
	"glBindTexture",
	"glBlendFunc",
	"glClear",
	"glClearColor",
	"glClearDepth",
	"glClearIndex",
	"glClearStencil",
	"glClipPlane",
	"glColor3d",
	"glColor3f",
	"glColor4f",
	"glColor4ub",
	"glColorMask",
	"glColorMaterial",
	"glColorPointer",
	"glCopyTexImage2D",
	"glCopyTexSubImage2D",
	"glCullFace",
	"glDeleteTextures",
	"glDepthFunc",
	"glDepthMask",
	"glDepthRange",
	"glDisable",
	"glDisableClientState",
	"glDrawArrays",
	"glDrawBuffer",
	"glDrawElements",
	"glDrawPixels",
	"glEnable",
	"glEnableClientState",
	"glEnd",
	"glFlush",
	"glFogf",
	"glFogfv",
	"glFogi",
	"glFrontFace",
	"glGenTextures",
	"glGetError",
	"glGetFloatv",
	"glGetIntegerv",
	"glGetString",
	"glGetTexImage",
	"glHint",
	"glLightModelfv",
	"glLightModeli",
	"glLightf",
	"glLightfv",
	"glLineStipple",
	"glLoadIdentity",
	"glLoadMatrixf",
	"glMaterialf",
	"glMaterialfv",
	"glMatrixMode",
	"glMultMatrixf",
	"glNormal3f",
	"glNormalPointer",
	"glOrtho",
	"glPixelStorei",
	"glPixelZoom",
	"glPointSize",
	"glPolygonMode",
	"glPolygonOffset",
	"glPopAttrib",
	"glPopMatrix",
	"glPrioritizeTextures",
	"glPushAttrib",
	"glPushMatrix",
	"glRasterPos3i",
	"glRasterPos3iv",
	"glReadBuffer",
	"glReadPixels",
	"glScissor",
	"glShadeModel",
	"glStencilFunc",
	"glStencilMask",
	"glStencilOp",
	"glTexCoord1f",
	"glTexCoord2f",
	"glTexCoord3f",
	"glTexCoord3iv",
	"glTexCoord4f",
	"glTexCoordPointer",
	"glTexEnvf",
	"glTexEnvfv",
	"glTexEnvi",
	"glTexGenfv",
	"glTexGeni",
	"glTexImage2D",
	"glTexParameterfv",
	"glTexParameteri",
	"glTexSubImage2D",
	"glTranslatef",
	"glVertex2f",
	"glVertex2i",
	"glVertex3d",
	"glVertex3f",
	"glVertex4f",
	"glVertexPointer",
	"glViewport",
};
struct pgl_assert_s {
    char assert_size[sizeof(pgl_iat) == sizeof(pgl_names) ? 1 : -1];
};


HMODULE hPrlicd32;
HMODULE hOpengl32;

BOOL
pglInit()
{
    int i;

    hPrlicd32 = LoadLibrary("prlicd32.dll");
    if (hPrlicd32) {
#define getproc(x) *(FARPROC *)&p##x = GetProcAddress(hPrlicd32, #x)
        getproc(DrvDescribePixelFormat);
        getproc(DrvSetPixelFormat);
        getproc(DrvSetContext);
        getproc(DrvReleaseContext);
        getproc(DrvSwapBuffers);
        getproc(DrvCreateContext);
        getproc(DrvDeleteContext);
        getproc(DrvGetProcAddress);
#undef getproc
        for (i = sizeof(pgl_names)/sizeof(pgl_names[1]); --i >= 0; ) {
            ((FARPROC *)&pgl_iat)[i] = GetProcAddress(hPrlicd32, pgl_names[i]);
        }
        return TRUE;
    }
    hOpengl32 = LoadLibrary("opengl32.dll");
    if (hOpengl32) {
#define getproc(x) *(FARPROC *)&p##x = GetProcAddress(hOpengl32, #x)
        getproc(wglMakeCurrent);
        getproc(wglGetCurrentContext);
        getproc(wglGetCurrentDC);
#undef getproc
#define getproc(x, y) *(FARPROC *)&p##x = GetProcAddress(hOpengl32, #y)
        getproc(DrvCreateContext, wglCreateContext);
        getproc(DrvDeleteContext, wglDeleteContext);
        getproc(DrvGetProcAddress, wglGetProcAddress);
#undef getproc
        for (i = sizeof(pgl_names)/sizeof(pgl_names[1]); --i >= 0; ) {
            ((FARPROC *)&pgl_iat)[i] = GetProcAddress(hOpengl32, pgl_names[i]);
        }
        return TRUE;
    }
    return FALSE;
}

VOID
pglFini()
{
    if (hPrlicd32)
        FreeLibrary(hPrlicd32);
    hPrlicd32 = NULL;
    if (hOpengl32)
        FreeLibrary(hOpengl32);
    hOpengl32 = NULL;
}


static HDC g_hdc;
static HGLRC g_hgl;

int WINAPI
pglDescribePixelFormat(
    HDC hdc, int index, UINT size, LPPIXELFORMATDESCRIPTOR format)
{
    if (pDrvDescribePixelFormat)
        return pDrvDescribePixelFormat(hdc, index, size, format);
    return DescribePixelFormat(hdc, index, size, format);
}

BOOL WINAPI
pglSetPixelFormat(HDC hdc, int index, const LPPIXELFORMATDESCRIPTOR format)
{
    if (pDrvSetPixelFormat)
        return pDrvSetPixelFormat(hdc, index);
    return SetPixelFormat(hdc, index, format);
}

BOOL WINAPI
pglMakeCurrent(HDC hdc, HGLRC hgl)
{
    if (pwglMakeCurrent)
        return pwglMakeCurrent(hdc, hgl);
    if (!hgl) {
        if (g_hgl)
            pDrvReleaseContext(g_hgl);
        g_hdc = 0;
        g_hgl = 0;
        return TRUE;
    }
    if (pDrvSetContext(hdc, hgl, 0)) {
        g_hdc = hdc;
        g_hgl = hgl;
        return TRUE;
    }
    return FALSE;
}

HGLRC WINAPI
pglGetCurrentContext(VOID)
{
    if (pwglGetCurrentContext)
        return pwglGetCurrentContext();
    return g_hgl;
}

HDC WINAPI
pglGetCurrentDC(VOID)
{
    if (pwglGetCurrentDC)
        return pwglGetCurrentDC();
    return g_hdc;
}

BOOL WINAPI
pglSwapBuffers(HDC hdc)
{
    if (pDrvSwapBuffers)
        return pDrvSwapBuffers(hdc);
    return SwapBuffers(hdc);
}

HGLRC WINAPI
pglCreateContext(HDC hdc)
{
    return pDrvCreateContext(hdc);
}

BOOL WINAPI
pglDeleteContext(HGLRC hgl)
{
    return pDrvDeleteContext(hgl);
}

PROC WINAPI
pglGetProcAddress(LPCSTR name)
{
    return pDrvGetProcAddress(name);
}
