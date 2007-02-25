/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Keith Whitwell
 *    Hiroshi Morii
 *    Daniel Borca
 */

/* fxwgl.c - Microsoft wgl functions emulation for
 *           3Dfx VooDoo/Mesa interface
 */


#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glext.h"

#ifdef __cplusplus
}
#endif

#include "GL/fxmesa.h"
#include "glheader.h"
#include "glapi.h"
#include "imports.h"
#include "../../glide/fxdrv.h"

#define MAX_MESA_ATTRS  20

#if (_MSC_VER >= 1200)
#pragma warning( push )
#pragma warning( disable : 4273 )
#endif

struct __extensions__ {
   PROC proc;
   char *name;
};

struct __pixelformat__ {
   PIXELFORMATDESCRIPTOR pfd;
   GLint mesaAttr[MAX_MESA_ATTRS];
};

WINGDIAPI void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint *);
static GLushort gammaTable[3 * 256];

struct __pixelformat__ pix[] = {
   /* 16bit RGB565 single buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
     PFD_TYPE_RGBA,
     16,
     5, 0, 6, 5, 5, 11, 0, 0,
     0, 0, 0, 0, 0,
     16,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 16,
     FXMESA_ALPHA_SIZE, 0,
     FXMESA_DEPTH_SIZE, 16,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
   ,
   /* 16bit RGB565 double buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     16,
     5, 0, 6, 5, 5, 11, 0, 0,
     0, 0, 0, 0, 0,
     16,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 16,
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 0,
     FXMESA_DEPTH_SIZE, 16,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
   ,
   /* 16bit ARGB1555 single buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
     PFD_TYPE_RGBA,
     16,
     5, 0, 5, 5, 5, 10, 1, 15,
     0, 0, 0, 0, 0,
     16,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 15,
     FXMESA_ALPHA_SIZE, 1,
     FXMESA_DEPTH_SIZE, 16,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
   ,
   /* 16bit ARGB1555 double buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     16,
     5, 0, 5, 5, 5, 10, 1, 15,
     0, 0, 0, 0, 0,
     16,
     0,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 15,
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 1,
     FXMESA_DEPTH_SIZE, 16,
     FXMESA_STENCIL_SIZE, 0,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
   ,
   /* 32bit ARGB8888 single buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
     PFD_TYPE_RGBA,
     32,
     8, 0, 8, 8, 8, 16, 8, 24,
     0, 0, 0, 0, 0,
     24,
     8,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 32,
     FXMESA_ALPHA_SIZE, 8,
     FXMESA_DEPTH_SIZE, 24,
     FXMESA_STENCIL_SIZE, 8,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
   ,
   /* 32bit ARGB8888 double buffer with depth */
   {
    {sizeof(PIXELFORMATDESCRIPTOR), 1,
     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
     PFD_DOUBLEBUFFER | PFD_SWAP_COPY,
     PFD_TYPE_RGBA,
     32,
     8, 0, 8, 8, 8, 16, 8, 24,
     0, 0, 0, 0, 0,
     24,
     8,
     0,
     PFD_MAIN_PLANE,
     0, 0, 0, 0}
    ,
    {FXMESA_COLORDEPTH, 32,
     FXMESA_DOUBLEBUFFER,
     FXMESA_ALPHA_SIZE, 8,
     FXMESA_DEPTH_SIZE, 24,
     FXMESA_STENCIL_SIZE, 8,
     FXMESA_ACCUM_SIZE, 0,
     FXMESA_NONE}
   }
};

static fxMesaContext ctx = NULL;
static WNDPROC hWNDOldProc;
static int curPFD = 0;
static HDC hDC;
static HWND hWND;

static GLboolean haveDualHead;

/* For the in-window-rendering hack */

#ifndef GR_CONTROL_RESIZE
/* Apparently GR_CONTROL_RESIZE can be ignored. OK? */
#define GR_CONTROL_RESIZE -1
#endif

static GLboolean gdiWindowHack;
static void *dibSurfacePtr;
static BITMAPINFO *dibBMI;
static HBITMAP dibHBM;
static HWND dibWnd;

static int
env_check (const char *var, int val)
{
   const char *env = getenv(var);
   return (env && (env[0] == val));
}

static LRESULT APIENTRY
__wglMonitor (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
   long ret;                    /* Now gives the resized window at the end to hWNDOldProc */

   if (ctx && hwnd == hWND) {
      switch (message) {
         case WM_PAINT:
         case WM_MOVE:
            break;
         case WM_DISPLAYCHANGE:
         case WM_SIZE:
#if 0
            if (wParam != SIZE_MINIMIZED) {
               static int moving = 0;
               if (!moving) {
                  if (!FX_grSstControl(GR_CONTROL_RESIZE)) {
                     moving = 1;
                     SetWindowPos(hwnd, 0, 0, 0, 300, 300, SWP_NOMOVE | SWP_NOZORDER);
                     moving = 0;
                     if (!FX_grSstControl(GR_CONTROL_RESIZE)) {
                        /*MessageBox(0,_T("Error changing windowsize"),_T("fxMESA"),MB_OK);*/
                        PostMessage(hWND, WM_CLOSE, 0, 0);
                     }
                  }
                  /* Do the clipping in the glide library */
                  grClipWindow(0, 0, FX_grSstScreenWidth(), FX_grSstScreenHeight());
                  /* And let the new size set in the context */
                  fxMesaUpdateScreenSize(ctx);
               }
            }
#endif
            break;
         case WM_ACTIVATE:
            break;
         case WM_SHOWWINDOW:
            break;
         case WM_SYSKEYDOWN:
         case WM_SYSCHAR:
            break;
      }
   }

   /* Finally call the hWNDOldProc, which handles the resize with the
    * now changed window sizes */
   ret = CallWindowProc(hWNDOldProc, hwnd, message, wParam, lParam);

   return ret;
}

static void
wgl_error (long error)
{
#define WGL_INVALID_PIXELFORMAT ERROR_INVALID_PIXEL_FORMAT
   SetLastError(0xC0000000      /* error severity */
               |0x00070000      /* error facility (who we are) */
               |error);
}

GLAPI BOOL GLAPIENTRY
wglCopyContext (HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
   return FALSE;
}

GLAPI HGLRC GLAPIENTRY
wglCreateContext (HDC hdc)
{
   HWND hWnd;
   WNDPROC oldProc;
   int error;

   if (ctx) {
      SetLastError(0);
      return NULL;
   }

   if (!(hWnd = WindowFromDC(hdc))) {
      SetLastError(0);
      return NULL;
   }

   if (curPFD == 0) {
      wgl_error(WGL_INVALID_PIXELFORMAT);
      return NULL;
   }

   if ((oldProc = (WNDPROC)GetWindowLong(hWnd, GWL_WNDPROC)) != __wglMonitor) {
      hWNDOldProc = oldProc;
      SetWindowLong(hWnd, GWL_WNDPROC, (LONG)__wglMonitor);
   }

   /* always log when debugging, or if user demands */
   if (TDFX_DEBUG || env_check("MESA_FX_INFO", 'r')) {
      freopen("MESA.LOG", "w", stderr);
   }

   {
      RECT cliRect;
      ShowWindow(hWnd, SW_SHOWNORMAL);
      SetForegroundWindow(hWnd);
      Sleep(100);               /* a hack for win95 */
      if (env_check("MESA_GLX_FX", 'w') && !(GetWindowLong(hWnd, GWL_STYLE) & WS_POPUP)) {
         /* XXX todo - windowed modes */
         error = !(ctx = fxMesaCreateContext((GLuint) hWnd, GR_RESOLUTION_NONE, GR_REFRESH_NONE, pix[curPFD - 1].mesaAttr));
      } else {
         GetClientRect(hWnd, &cliRect);
         error = !(ctx = fxMesaCreateBestContext((GLuint) hWnd, cliRect.right, cliRect.bottom, pix[curPFD - 1].mesaAttr));
      }
   }

   /*if (getenv("SST_DUALHEAD"))
      haveDualHead =
         ((atoi(getenv("SST_DUALHEAD")) == 1) ? GL_TRUE : GL_FALSE);
   else
      haveDualHead = GL_FALSE;*/

   if (error) {
      SetLastError(0);
      return NULL;
   }

   hDC = hdc;
   hWND = hWnd;

   /* Required by the OpenGL Optimizer 1.1 (is it a Optimizer bug ?) */
   wglMakeCurrent(hdc, (HGLRC)1);

   return (HGLRC)1;
}

GLAPI HGLRC GLAPIENTRY
wglCreateLayerContext (HDC hdc, int iLayerPlane)
{
   SetLastError(0);
   return NULL;
}

GLAPI BOOL GLAPIENTRY
wglDeleteContext (HGLRC hglrc)
{
   if (ctx && hglrc == (HGLRC)1) {

      fxMesaDestroyContext(ctx);

      SetWindowLong(WindowFromDC(hDC), GWL_WNDPROC, (LONG) hWNDOldProc);

      ctx = NULL;
      hDC = 0;
      return TRUE;
   }

   SetLastError(0);

   return FALSE;
}

GLAPI HGLRC GLAPIENTRY
wglGetCurrentContext (VOID)
{
   if (ctx)
      return (HGLRC)1;

   SetLastError(0);
   return NULL;
}

GLAPI HDC GLAPIENTRY
wglGetCurrentDC (VOID)
{
   if (ctx)
      return hDC;

   SetLastError(0);
   return NULL;
}

GLAPI BOOL GLAPIENTRY
wglSwapIntervalEXT (int interval)
{
   if (ctx == NULL) {
      return FALSE;
   }
   if (interval < 0) {
      interval = 0;
   } else if (interval > 3) {
      interval = 3;
   }
   ctx->swapInterval = interval;
   return TRUE;
}

GLAPI int GLAPIENTRY
wglGetSwapIntervalEXT (void)
{
   return (ctx == NULL) ? -1 : ctx->swapInterval;
}

GLAPI BOOL GLAPIENTRY
wglGetDeviceGammaRamp3DFX (HDC hdc, LPVOID arrays)
{
   /* gammaTable should be per-context */
   memcpy(arrays, gammaTable, 3 * 256 * sizeof(GLushort));
   return TRUE;
}

GLAPI BOOL GLAPIENTRY
wglSetDeviceGammaRamp3DFX (HDC hdc, LPVOID arrays)
{
   GLint i, tableSize, inc, index;
   GLushort *red, *green, *blue;
   FxU32 gammaTableR[256], gammaTableG[256], gammaTableB[256];

   /* gammaTable should be per-context */
   memcpy(gammaTable, arrays, 3 * 256 * sizeof(GLushort));

   tableSize = FX_grGetInteger(GR_GAMMA_TABLE_ENTRIES);
   inc = 256 / tableSize;
   red = (GLushort *)arrays;
   green = (GLushort *)arrays + 256;
   blue = (GLushort *)arrays + 512;
   for (i = 0, index = 0; i < tableSize; i++, index += inc) {
      gammaTableR[i] = red[index] >> 8;
      gammaTableG[i] = green[index] >> 8;
      gammaTableB[i] = blue[index] >> 8;
   }

   grLoadGammaTable(tableSize, gammaTableR, gammaTableG, gammaTableB);

   return TRUE;
}

typedef void *HPBUFFERARB;

/* WGL_ARB_pixel_format */
GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribivARB (HDC hdc,
                              int iPixelFormat,
                              int iLayerPlane,
                              UINT nAttributes,
                              const int *piAttributes,
                              int *piValues)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribfvARB (HDC hdc,
                              int iPixelFormat,
                              int iLayerPlane,
                              UINT nAttributes,
                              const int *piAttributes,
                              FLOAT *pfValues)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglChoosePixelFormatARB (HDC hdc,
                         const int *piAttribIList,
                         const FLOAT *pfAttribFList,
                         UINT nMaxFormats,
                         int *piFormats,
                         UINT *nNumFormats)
{
   SetLastError(0);
   return FALSE;
}

/* WGL_ARB_render_texture */
GLAPI BOOL GLAPIENTRY
wglBindTexImageARB (HPBUFFERARB hPbuffer, int iBuffer)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglReleaseTexImageARB (HPBUFFERARB hPbuffer, int iBuffer)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglSetPbufferAttribARB (HPBUFFERARB hPbuffer,
                        const int *piAttribList)
{
   SetLastError(0);
   return FALSE;
}

/* WGL_ARB_pbuffer */
GLAPI HPBUFFERARB GLAPIENTRY
wglCreatePbufferARB (HDC hDC,
                     int iPixelFormat,
                     int iWidth,
                     int iHeight,
                     const int *piAttribList)
{
   SetLastError(0);
   return NULL;
}

GLAPI HDC GLAPIENTRY
wglGetPbufferDCARB (HPBUFFERARB hPbuffer)
{
   SetLastError(0);
   return NULL;
}

GLAPI int GLAPIENTRY
wglReleasePbufferDCARB (HPBUFFERARB hPbuffer, HDC hDC)
{
   SetLastError(0);
   return -1;
}

GLAPI BOOL GLAPIENTRY
wglDestroyPbufferARB (HPBUFFERARB hPbuffer)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglQueryPbufferARB (HPBUFFERARB hPbuffer,
                    int iAttribute,
                    int *piValue)
{
   SetLastError(0);
   return FALSE;
}

GLAPI const char * GLAPIENTRY
wglGetExtensionsStringEXT (void)
{
   return "WGL_3DFX_gamma_control "
          "WGL_EXT_swap_control "
          "WGL_EXT_extensions_string WGL_ARB_extensions_string"
         /*WGL_ARB_pixel_format WGL_ARB_render_texture WGL_ARB_pbuffer*/;
}

GLAPI const char * GLAPIENTRY
wglGetExtensionsStringARB (HDC hdc)
{
   return wglGetExtensionsStringEXT();
}

static struct {
   const char *name;
   PROC func;
} wgl_ext[] = {
       {"wglGetExtensionsStringARB",    (PROC)wglGetExtensionsStringARB},
       {"wglGetExtensionsStringEXT",    (PROC)wglGetExtensionsStringEXT},
       {"wglSwapIntervalEXT",           (PROC)wglSwapIntervalEXT},
       {"wglGetSwapIntervalEXT",        (PROC)wglGetSwapIntervalEXT},
       {"wglGetDeviceGammaRamp3DFX",    (PROC)wglGetDeviceGammaRamp3DFX},
       {"wglSetDeviceGammaRamp3DFX",    (PROC)wglSetDeviceGammaRamp3DFX},
       /* WGL_ARB_pixel_format */
       {"wglGetPixelFormatAttribivARB", (PROC)wglGetPixelFormatAttribivARB},
       {"wglGetPixelFormatAttribfvARB", (PROC)wglGetPixelFormatAttribfvARB},
       {"wglChoosePixelFormatARB",      (PROC)wglChoosePixelFormatARB},
       /* WGL_ARB_render_texture */
       {"wglBindTexImageARB",           (PROC)wglBindTexImageARB},
       {"wglReleaseTexImageARB",        (PROC)wglReleaseTexImageARB},
       {"wglSetPbufferAttribARB",       (PROC)wglSetPbufferAttribARB},
       /* WGL_ARB_pbuffer */
       {"wglCreatePbufferARB",          (PROC)wglCreatePbufferARB},
       {"wglGetPbufferDCARB",           (PROC)wglGetPbufferDCARB},
       {"wglReleasePbufferDCARB",       (PROC)wglReleasePbufferDCARB},
       {"wglDestroyPbufferARB",         (PROC)wglDestroyPbufferARB},
       {"wglQueryPbufferARB",           (PROC)wglQueryPbufferARB},
       {NULL, NULL}
};

GLAPI PROC GLAPIENTRY
wglGetProcAddress (LPCSTR lpszProc)
{
   int i;
   PROC p = (PROC)_glapi_get_proc_address((const char *)lpszProc);

   /* we can't BlendColor. work around buggy applications */
   if (p && strcmp(lpszProc, "glBlendColor")
         && strcmp(lpszProc, "glBlendColorEXT"))
      return p;

   for (i = 0; wgl_ext[i].name; i++) {
      if (!strcmp(lpszProc, wgl_ext[i].name)) {
         return wgl_ext[i].func;
      }
   }

   SetLastError(0);
   return NULL;
}

GLAPI PROC GLAPIENTRY
wglGetDefaultProcAddress (LPCSTR lpszProc)
{
   SetLastError(0);
   return NULL;
}

GLAPI BOOL GLAPIENTRY
wglMakeCurrent (HDC hdc, HGLRC hglrc)
{
   if ((hdc == NULL) && (hglrc == NULL))
      return TRUE;

   if (!ctx || hglrc != (HGLRC)1 || WindowFromDC(hdc) != hWND) {
      SetLastError(0);
      return FALSE;
   }

   hDC = hdc;

   fxMesaMakeCurrent(ctx);

   return TRUE;
}

GLAPI BOOL GLAPIENTRY
wglShareLists (HGLRC hglrc1, HGLRC hglrc2)
{
   if (!ctx || hglrc1 != (HGLRC)1 || hglrc1 != hglrc2) {
      SetLastError(0);
      return FALSE;
   }

   return TRUE;
}

static BOOL
wglUseFontBitmaps_FX (HDC fontDevice, DWORD firstChar, DWORD numChars,
                      DWORD listBase)
{
   TEXTMETRIC metric;
   BITMAPINFO *dibInfo;
   HDC bitDevice;
   COLORREF tempColor;
   int i;

   GetTextMetrics(fontDevice, &metric);

   dibInfo = (BITMAPINFO *)calloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD), 1);
   dibInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   dibInfo->bmiHeader.biPlanes = 1;
   dibInfo->bmiHeader.biBitCount = 1;
   dibInfo->bmiHeader.biCompression = BI_RGB;

   bitDevice = CreateCompatibleDC(fontDevice);

   /* Swap fore and back colors so the bitmap has the right polarity */
   tempColor = GetBkColor(bitDevice);
   SetBkColor(bitDevice, GetTextColor(bitDevice));
   SetTextColor(bitDevice, tempColor);

   /* Place chars based on base line */
   SetTextAlign(bitDevice, TA_BASELINE);

   for (i = 0; i < (int)numChars; i++) {
      SIZE size;
      char curChar;
      int charWidth, charHeight, bmapWidth, bmapHeight, numBytes, res;
      HBITMAP bitObject;
      HGDIOBJ origBmap;
      unsigned char *bmap;

      curChar = (char)(i + firstChar); /* [koolsmoky] explicit cast */

      /* Find how high/wide this character is */
      GetTextExtentPoint32(bitDevice, &curChar, 1, &size);

      /* Create the output bitmap */
      charWidth = size.cx;
      charHeight = size.cy;
      bmapWidth = ((charWidth + 31) / 32) * 32; /* Round up to the next multiple of 32 bits */
      bmapHeight = charHeight;
      bitObject = CreateCompatibleBitmap(bitDevice, bmapWidth, bmapHeight);
      /*VERIFY(bitObject);*/

      /* Assign the output bitmap to the device */
      origBmap = SelectObject(bitDevice, bitObject);

      PatBlt(bitDevice, 0, 0, bmapWidth, bmapHeight, BLACKNESS);

      /* Use our source font on the device */
      SelectObject(bitDevice, GetCurrentObject(fontDevice, OBJ_FONT));

      /* Draw the character */
      TextOut(bitDevice, 0, metric.tmAscent, &curChar, 1);

      /* Unselect our bmap object */
      SelectObject(bitDevice, origBmap);

      /* Convert the display dependant representation to a 1 bit deep DIB */
      numBytes = (bmapWidth * bmapHeight) / 8;
      bmap = MALLOC(numBytes);
      dibInfo->bmiHeader.biWidth = bmapWidth;
      dibInfo->bmiHeader.biHeight = bmapHeight;
      res = GetDIBits(bitDevice, bitObject, 0, bmapHeight, bmap,
                      dibInfo, DIB_RGB_COLORS);

      /* Create the GL object */
      glNewList(i + listBase, GL_COMPILE);
      glBitmap(bmapWidth, bmapHeight, 0.0, metric.tmDescent,
               charWidth, 0.0, bmap);
      glEndList();
      /* CheckGL(); */

      /* Destroy the bmap object */
      DeleteObject(bitObject);

      /* Deallocate the bitmap data */
      FREE(bmap);
   }

   /* Destroy the DC */
   DeleteDC(bitDevice);

   FREE(dibInfo);

   return TRUE;
}

GLAPI BOOL GLAPIENTRY
wglUseFontBitmapsW (HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglUseFontOutlinesA (HDC hdc, DWORD first, DWORD count,
                     DWORD listBase, FLOAT deviation,
                     FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglUseFontOutlinesW (HDC hdc, DWORD first, DWORD count,
                     DWORD listBase, FLOAT deviation,
                     FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
   SetLastError(0);
   return FALSE;
}


GLAPI BOOL GLAPIENTRY
wglSwapLayerBuffers (HDC hdc, UINT fuPlanes)
{
   if (ctx && WindowFromDC(hdc) == hWND) {
      fxMesaSwapBuffers();

      return TRUE;
   }

   SetLastError(0);
   return FALSE;
}

static int
pfd_tablen (void)
{
   /* we should take an envvar for `fxMesaSelectCurrentBoard' */
   return (fxMesaSelectCurrentBoard(0) < GR_SSTTYPE_Voodoo4)
         ? 2                      /* only 16bit entries */
         : sizeof(pix) / sizeof(pix[0]);  /* full table */
}

GLAPI int GLAPIENTRY
wglChoosePixelFormat (HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{
   int i, best = -1, qt_valid_pix;
   PIXELFORMATDESCRIPTOR pfd = *ppfd;

   qt_valid_pix = pfd_tablen();

#if 1 || QUAKE2 || GORE
   /* QUAKE2: 24+32 */
   /* GORE  : 24+16 */
   if ((pfd.cColorBits == 24) || (pfd.cColorBits == 32)) {
      /* the first 2 entries are 16bit */
      pfd.cColorBits = (qt_valid_pix > 2) ? 32 : 16;
   }
   if (pfd.cColorBits == 32) {
      pfd.cDepthBits = 24;
   } else if (pfd.cColorBits == 16) {
      pfd.cDepthBits = 16;
   }
#endif

   if (pfd.nSize != sizeof(PIXELFORMATDESCRIPTOR) || pfd.nVersion != 1) {
      SetLastError(0);
      return 0;
   }

   for (i = 0; i < qt_valid_pix; i++) {
      if (pfd.cColorBits > 0 && pix[i].pfd.cColorBits != pfd.cColorBits)
         continue;

      if ((pfd.dwFlags & PFD_DRAW_TO_WINDOW)
          && !(pix[i].pfd.dwFlags & PFD_DRAW_TO_WINDOW)) continue;
      if ((pfd.dwFlags & PFD_DRAW_TO_BITMAP)
          && !(pix[i].pfd.dwFlags & PFD_DRAW_TO_BITMAP)) continue;
      if ((pfd.dwFlags & PFD_SUPPORT_GDI)
          && !(pix[i].pfd.dwFlags & PFD_SUPPORT_GDI)) continue;
      if ((pfd.dwFlags & PFD_SUPPORT_OPENGL)
          && !(pix[i].pfd.dwFlags & PFD_SUPPORT_OPENGL)) continue;
      if (!(pfd.dwFlags & PFD_DOUBLEBUFFER_DONTCARE)
          && ((pfd.dwFlags & PFD_DOUBLEBUFFER) !=
              (pix[i].pfd.dwFlags & PFD_DOUBLEBUFFER))) continue;
#if 1 /* Doom3 fails here! */
      if (!(pfd.dwFlags & PFD_STEREO_DONTCARE)
          && ((pfd.dwFlags & PFD_STEREO) !=
              (pix[i].pfd.dwFlags & PFD_STEREO))) continue;
#endif

      if (pfd.cDepthBits > 0 && pix[i].pfd.cDepthBits == 0)
         continue;              /* need depth buffer */

      if (pfd.cAlphaBits > 0 && pix[i].pfd.cAlphaBits == 0)
         continue;              /* need alpha buffer */

#if 0                           /* regression bug? */
      if (pfd.cStencilBits > 0 && pix[i].pfd.cStencilBits == 0)
         continue;              /* need stencil buffer */
#endif

      if (pfd.iPixelType == pix[i].pfd.iPixelType) {
         best = i + 1;
         break;
      }
   }

   if (best == -1) {
      FILE *err = fopen("MESA.LOG", "w");
      if (err != NULL) {
         fprintf(err, "wglChoosePixelFormat failed\n");
         fprintf(err, "\tnSize           = %d\n", ppfd->nSize);
         fprintf(err, "\tnVersion        = %d\n", ppfd->nVersion);
         fprintf(err, "\tdwFlags         = %lu\n", ppfd->dwFlags);
         fprintf(err, "\tiPixelType      = %d\n", ppfd->iPixelType);
         fprintf(err, "\tcColorBits      = %d\n", ppfd->cColorBits);
         fprintf(err, "\tcRedBits        = %d\n", ppfd->cRedBits);
         fprintf(err, "\tcRedShift       = %d\n", ppfd->cRedShift);
         fprintf(err, "\tcGreenBits      = %d\n", ppfd->cGreenBits);
         fprintf(err, "\tcGreenShift     = %d\n", ppfd->cGreenShift);
         fprintf(err, "\tcBlueBits       = %d\n", ppfd->cBlueBits);
         fprintf(err, "\tcBlueShift      = %d\n", ppfd->cBlueShift);
         fprintf(err, "\tcAlphaBits      = %d\n", ppfd->cAlphaBits);
         fprintf(err, "\tcAlphaShift     = %d\n", ppfd->cAlphaShift);
         fprintf(err, "\tcAccumBits      = %d\n", ppfd->cAccumBits);
         fprintf(err, "\tcAccumRedBits   = %d\n", ppfd->cAccumRedBits);
         fprintf(err, "\tcAccumGreenBits = %d\n", ppfd->cAccumGreenBits);
         fprintf(err, "\tcAccumBlueBits  = %d\n", ppfd->cAccumBlueBits);
         fprintf(err, "\tcAccumAlphaBits = %d\n", ppfd->cAccumAlphaBits);
         fprintf(err, "\tcDepthBits      = %d\n", ppfd->cDepthBits);
         fprintf(err, "\tcStencilBits    = %d\n", ppfd->cStencilBits);
         fprintf(err, "\tcAuxBuffers     = %d\n", ppfd->cAuxBuffers);
         fprintf(err, "\tiLayerType      = %d\n", ppfd->iLayerType);
         fprintf(err, "\tbReserved       = %d\n", ppfd->bReserved);
         fprintf(err, "\tdwLayerMask     = %lu\n", ppfd->dwLayerMask);
         fprintf(err, "\tdwVisibleMask   = %lu\n", ppfd->dwVisibleMask);
         fprintf(err, "\tdwDamageMask    = %lu\n", ppfd->dwDamageMask);
         fclose(err);
      }

      SetLastError(0);
      return 0;
   }

   return best;
}

GLAPI int GLAPIENTRY
ChoosePixelFormat (HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{

   return wglChoosePixelFormat(hdc, ppfd);
}

GLAPI int GLAPIENTRY
wglDescribePixelFormat (HDC hdc, int iPixelFormat, UINT nBytes,
                        LPPIXELFORMATDESCRIPTOR ppfd)
{
   int qt_valid_pix;

   qt_valid_pix = pfd_tablen();

   if (iPixelFormat < 1 || iPixelFormat > qt_valid_pix ||
       ((nBytes != sizeof(PIXELFORMATDESCRIPTOR)) && (nBytes != 0))) {
      SetLastError(0);
      return qt_valid_pix;
   }

   if (nBytes != 0)
      *ppfd = pix[iPixelFormat - 1].pfd;

   return qt_valid_pix;
}

GLAPI int GLAPIENTRY
DescribePixelFormat (HDC hdc, int iPixelFormat, UINT nBytes,
                     LPPIXELFORMATDESCRIPTOR ppfd)
{
   return wglDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}

GLAPI int GLAPIENTRY
wglGetPixelFormat (HDC hdc)
{
   if (curPFD == 0) {
      SetLastError(0);
      return 0;
   }

   return curPFD;
}

GLAPI int GLAPIENTRY
GetPixelFormat (HDC hdc)
{
   return wglGetPixelFormat(hdc);
}

GLAPI BOOL GLAPIENTRY
wglSetPixelFormat (HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
   int qt_valid_pix;

   qt_valid_pix = pfd_tablen();

   if (iPixelFormat < 1 || iPixelFormat > qt_valid_pix) {
      if (ppfd == NULL) {
         PIXELFORMATDESCRIPTOR my_pfd;
         if (!wglDescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &my_pfd)) {
            SetLastError(0);
            return FALSE;
         }
      } else if (ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR)) {
         SetLastError(0);
         return FALSE;
      }
   }
   curPFD = iPixelFormat;

   return TRUE;
}

GLAPI BOOL GLAPIENTRY
wglSwapBuffers (HDC hdc)
{
   if (!ctx) {
      SetLastError(0);
      return FALSE;
   }

   fxMesaSwapBuffers();

   return TRUE;
}

GLAPI BOOL GLAPIENTRY
SetPixelFormat (HDC hdc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd)
{
   return wglSetPixelFormat(hdc, iPixelFormat, ppfd);
}

GLAPI BOOL GLAPIENTRY
SwapBuffers(HDC hdc)
{
   return wglSwapBuffers(hdc);
}

static FIXED
FixedFromDouble (double d)
{
   struct {
      FIXED f;
      long l;
   } pun;
   pun.l = (long)(d * 65536L);
   return pun.f;
}

/*
** This was yanked from windows/gdi/wgl.c
*/
GLAPI BOOL GLAPIENTRY
wglUseFontBitmapsA (HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
   int i;
   GLuint font_list;
   DWORD size;
   GLYPHMETRICS gm;
   HANDLE hBits;
   LPSTR lpBits;
   MAT2 mat;
   int success = TRUE;

   font_list = listBase;

   mat.eM11 = FixedFromDouble(1);
   mat.eM12 = FixedFromDouble(0);
   mat.eM21 = FixedFromDouble(0);
   mat.eM22 = FixedFromDouble(-1);

   memset(&gm, 0, sizeof(gm));

   /*
    ** If we can't get the glyph outline, it may be because this is a fixed
    ** font.  Try processing it that way.
    */
   if (GetGlyphOutline(hdc, first, GGO_BITMAP, &gm, 0, NULL, &mat) == GDI_ERROR) {
      return wglUseFontBitmaps_FX(hdc, first, count, listBase);
   }

   /*
    ** Otherwise process all desired characters.
    */
   for (i = 0; i < count; i++) {
      DWORD err;

      glNewList(font_list + i, GL_COMPILE);

      /* allocate space for the bitmap/outline */
      size = GetGlyphOutline(hdc, first + i, GGO_BITMAP, &gm, 0, NULL, &mat);
      if (size == GDI_ERROR) {
         glEndList();
         err = GetLastError();
         success = FALSE;
         continue;
      }

      hBits = GlobalAlloc(GHND, size + 1);
      lpBits = GlobalLock(hBits);

      err = GetGlyphOutline(hdc,        /* handle to device context */
                            first + i,  /* character to query */
                            GGO_BITMAP, /* format of data to return */
                            &gm,        /* pointer to structure for metrics */
                            size,       /* size of buffer for data */
                            lpBits,     /* pointer to buffer for data */
                            &mat        /* pointer to transformation */
                                        /* matrix structure */
          );

      if (err == GDI_ERROR) {
         GlobalUnlock(hBits);
         GlobalFree(hBits);

         glEndList();
         err = GetLastError();
         success = FALSE;
         continue;
      }

      glBitmap(gm.gmBlackBoxX, gm.gmBlackBoxY,
               -gm.gmptGlyphOrigin.x,
               gm.gmptGlyphOrigin.y,
               gm.gmCellIncX, gm.gmCellIncY,
               (const GLubyte *)lpBits);

      GlobalUnlock(hBits);
      GlobalFree(hBits);

      glEndList();
   }

   return success;
}

GLAPI BOOL GLAPIENTRY
wglDescribeLayerPlane (HDC hdc, int iPixelFormat, int iLayerPlane,
                       UINT nBytes, LPLAYERPLANEDESCRIPTOR ppfd)
{
   SetLastError(0);
   return FALSE;
}

GLAPI int GLAPIENTRY
wglGetLayerPaletteEntries (HDC hdc, int iLayerPlane, int iStart,
                           int cEntries, COLORREF *pcr)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglRealizeLayerPalette (HDC hdc, int iLayerPlane, BOOL bRealize)
{
   SetLastError(0);
   return FALSE;
}

GLAPI int GLAPIENTRY
wglSetLayerPaletteEntries (HDC hdc, int iLayerPlane, int iStart,
                           int cEntries, CONST COLORREF *pcr)
{
   SetLastError(0);
   return FALSE;
}


/***************************************************************************
 * [dBorca] simplistic ICD implementation, based on ICD code by Gregor Anich
 */

typedef struct _icdTable {
   DWORD size;
   PROC table[336];
} ICDTABLE, *PICDTABLE;

#ifdef USE_MGL_NAMESPACE
#define GL_FUNC(func) mgl##func
#else
#define GL_FUNC(func) gl##func
#endif

static ICDTABLE icdTable = { 336, {
#define ICD_ENTRY(func) (PROC)GL_FUNC(func),
#include "../icd/icdlist.h"
#undef ICD_ENTRY
} };


GLAPI BOOL GLAPIENTRY
DrvCopyContext (HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask)
{
   return wglCopyContext(hglrcSrc, hglrcDst, mask);
}


GLAPI HGLRC GLAPIENTRY
DrvCreateContext (HDC hdc)
{
   return wglCreateContext(hdc);
}


GLAPI BOOL GLAPIENTRY
DrvDeleteContext (HGLRC hglrc)
{
   return wglDeleteContext(hglrc);
}


GLAPI HGLRC GLAPIENTRY
DrvCreateLayerContext (HDC hdc, int iLayerPlane)
{
   return wglCreateContext(hdc);
}


GLAPI PICDTABLE GLAPIENTRY
DrvSetContext (HDC hdc, HGLRC hglrc, void *callback)
{
   return wglMakeCurrent(hdc, hglrc) ? &icdTable : NULL;
}


GLAPI BOOL GLAPIENTRY
DrvReleaseContext (HGLRC hglrc)
{
   return TRUE;
}


GLAPI BOOL GLAPIENTRY
DrvShareLists (HGLRC hglrc1, HGLRC hglrc2)
{
   return wglShareLists(hglrc1, hglrc2);
}


GLAPI BOOL GLAPIENTRY
DrvDescribeLayerPlane (HDC hdc, int iPixelFormat,
                       int iLayerPlane, UINT nBytes,
                       LPLAYERPLANEDESCRIPTOR plpd)
{
   return wglDescribeLayerPlane(hdc, iPixelFormat, iLayerPlane, nBytes, plpd);
}


GLAPI int GLAPIENTRY
DrvSetLayerPaletteEntries (HDC hdc, int iLayerPlane,
                           int iStart, int cEntries, CONST COLORREF *pcr)
{
   return wglSetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
}


GLAPI int GLAPIENTRY
DrvGetLayerPaletteEntries (HDC hdc, int iLayerPlane,
                           int iStart, int cEntries, COLORREF *pcr)
{
   return wglGetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
}


GLAPI BOOL GLAPIENTRY
DrvRealizeLayerPalette (HDC hdc, int iLayerPlane, BOOL bRealize)
{
   return wglRealizeLayerPalette(hdc, iLayerPlane, bRealize);
}


GLAPI BOOL GLAPIENTRY
DrvSwapLayerBuffers (HDC hdc, UINT fuPlanes)
{
   return wglSwapLayerBuffers(hdc, fuPlanes);
}

GLAPI int GLAPIENTRY
DrvDescribePixelFormat (HDC hdc, int iPixelFormat, UINT nBytes,
                        LPPIXELFORMATDESCRIPTOR ppfd)
{
   return wglDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd);
}


GLAPI PROC GLAPIENTRY
DrvGetProcAddress (LPCSTR lpszProc)
{
   return wglGetProcAddress(lpszProc);
}


GLAPI BOOL GLAPIENTRY
DrvSetPixelFormat (HDC hdc, int iPixelFormat)
{
   return wglSetPixelFormat(hdc, iPixelFormat, NULL);
}


GLAPI BOOL GLAPIENTRY
DrvSwapBuffers (HDC hdc)
{
   return wglSwapBuffers(hdc);
}


GLAPI BOOL GLAPIENTRY
DrvValidateVersion (DWORD version)
{
   (void)version;
   return TRUE;
}


#if (_MSC_VER >= 1200)
#pragma warning( pop )
#endif

#endif /* FX */
