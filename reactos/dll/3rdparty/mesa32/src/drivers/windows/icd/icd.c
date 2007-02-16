/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/*
 * File name: icd.c
 * Author:    Gregor Anich
 *
 * ICD (Installable Client Driver) interface.
 * Based on the windows GDI/WGL driver.
 */

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

#include <stdio.h>
#include <tchar.h>
#include "GL/wmesa.h"
#include "mtypes.h"
#include "glapi.h"

#define MAX_MESA_ATTRS	20

typedef struct wmesa_context *PWMC;

typedef struct _icdTable {
    DWORD size;
    PROC  table[336];
} ICDTABLE, *PICDTABLE;

#ifdef USE_MGL_NAMESPACE
# define GL_FUNC(func) mgl##func
#else
# define GL_FUNC(func) gl##func
#endif

static ICDTABLE icdTable = { 336, {
#define ICD_ENTRY(func) (PROC)GL_FUNC(func),
#include "icdlist.h"
#undef ICD_ENTRY
} };

struct __pixelformat__
{
    PIXELFORMATDESCRIPTOR	pfd;
    GLboolean doubleBuffered;
};

struct __pixelformat__	pix[] =
{
    /* Double Buffer, alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_TRUE
    },
    /* Single Buffer, alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL, /* | PFD_SUPPORT_GDI ? */
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_FALSE
    },
    /* Double Buffer, no alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	0,	0,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_TRUE
    },
    /* Single Buffer, no alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL, /* | PFD_SUPPORT_GDI ? */
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	0,	0,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_FALSE
    },
};

int qt_pix = sizeof(pix) / sizeof(pix[0]);

typedef struct {
    WMesaContext ctx;
    HDC hdc;
} MesaWglCtx;

#define MESAWGL_CTX_MAX_COUNT 20

static MesaWglCtx wgl_ctx[MESAWGL_CTX_MAX_COUNT];

static unsigned ctx_count = 0;
static int ctx_current = -1;
static unsigned curPFD = 0;

WGLAPI BOOL GLAPIENTRY DrvCopyContext(HGLRC hglrcSrc,HGLRC hglrcDst,UINT mask)
{
    (void) hglrcSrc; (void) hglrcDst; (void) mask;
    return(FALSE);
}

WGLAPI HGLRC GLAPIENTRY DrvCreateContext(HDC hdc)
{
    HWND		hWnd;
    int i = 0;

    if(!(hWnd = WindowFromDC(hdc)))
    {
        SetLastError(0);
        return(NULL);
    }
    if (!ctx_count)
    {
    	for(i=0;i<MESAWGL_CTX_MAX_COUNT;i++)
    	{
    		wgl_ctx[i].ctx = NULL;
    		wgl_ctx[i].hdc = NULL;
    	}
    }
    for( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
        if ( wgl_ctx[i].ctx == NULL )
        {
            wgl_ctx[i].ctx = WMesaCreateContext( hWnd, NULL, GL_TRUE,
                pix[curPFD-1].doubleBuffered, 
                pix[curPFD-1].pfd.cAlphaBits ? GL_TRUE : GL_FALSE);
            if (wgl_ctx[i].ctx == NULL)
                break;
            wgl_ctx[i].hdc = hdc;
            ctx_count++;
            return ((HGLRC)wgl_ctx[i].ctx);
        }
    }
    SetLastError(0);
    return(NULL);
}

WGLAPI BOOL GLAPIENTRY DrvDeleteContext(HGLRC hglrc)
{
    int i;
    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
    	if ( wgl_ctx[i].ctx == (PWMC) hglrc )
    	{
            WMesaMakeCurrent((PWMC) hglrc);
            WMesaDestroyContext();
            wgl_ctx[i].ctx = NULL;
            wgl_ctx[i].hdc = NULL;
            ctx_count--;
            return(TRUE);
    	}
    }
    SetLastError(0);
    return(FALSE);
}

WGLAPI HGLRC GLAPIENTRY DrvCreateLayerContext(HDC hdc,int iLayerPlane)
{
    if (iLayerPlane == 0)
      return DrvCreateContext(hdc);
    SetLastError(0);
    return(NULL);
}

WGLAPI PICDTABLE GLAPIENTRY DrvSetContext(HDC hdc,HGLRC hglrc,void *callback)
{
    int i;
    (void) callback;

    /* new code suggested by Andy Sy */
    if (!hdc || !hglrc) {
       WMesaMakeCurrent(NULL);
       ctx_current = -1;
       return NULL;
    }

    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
        if ( wgl_ctx[i].ctx == (PWMC) hglrc )
        {
            wgl_ctx[i].hdc = hdc;
            WMesaMakeCurrent( (PWMC) hglrc );
            ctx_current = i;
            return &icdTable;
        }
    }
    return NULL;
}

WGLAPI void GLAPIENTRY DrvReleaseContext(HGLRC hglrc)
{
    (void) hglrc;
    WMesaMakeCurrent(NULL);
    ctx_current = -1;
}

WGLAPI BOOL GLAPIENTRY DrvShareLists(HGLRC hglrc1,HGLRC hglrc2)
{
    (void) hglrc1; (void) hglrc2;
    return(TRUE);
}

WGLAPI BOOL GLAPIENTRY DrvDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane,UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    (void) hdc; (void) iPixelFormat; (void) iLayerPlane; (void) nBytes; (void) plpd;
    SetLastError(0);
    return(FALSE);
}

WGLAPI int GLAPIENTRY DrvSetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       CONST COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WGLAPI int GLAPIENTRY DrvGetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WGLAPI BOOL GLAPIENTRY DrvRealizeLayerPalette(HDC hdc,int iLayerPlane,BOOL bRealize)
{
    (void) hdc; (void) iLayerPlane; (void) bRealize;
    SetLastError(0);
    return(FALSE);
}

WGLAPI BOOL GLAPIENTRY DrvSwapLayerBuffers(HDC hdc,UINT fuPlanes)
{
    (void) fuPlanes;
    if( !hdc )
    {
        WMesaSwapBuffers();
        return(TRUE);
    }
    SetLastError(0);
    return(FALSE);
}

WGLAPI int GLAPIENTRY DrvDescribePixelFormat(HDC hdc,int iPixelFormat,UINT nBytes,
                                    LPPIXELFORMATDESCRIPTOR ppfd)
{
    int	qt_valid_pix;
    (void) hdc;

    qt_valid_pix = qt_pix;
    if(ppfd == NULL)
	return(qt_valid_pix);
    if(iPixelFormat < 1 || iPixelFormat > qt_valid_pix || nBytes != sizeof(PIXELFORMATDESCRIPTOR))
    {
        SetLastError(0);
        return(0);
    }
    *ppfd = pix[iPixelFormat - 1].pfd;
    return(qt_valid_pix);
}

/*
* GetProcAddress - return the address of an appropriate extension
*/
WGLAPI PROC GLAPIENTRY DrvGetProcAddress(LPCSTR lpszProc)
{
   PROC p = (PROC) (int) _glapi_get_proc_address((const char *) lpszProc);
   if (p)
      return p;

   SetLastError(0);
   return(NULL);
}

WGLAPI BOOL GLAPIENTRY DrvSetPixelFormat(HDC hdc,int iPixelFormat)
{
    int qt_valid_pix;
    (void) hdc;

    qt_valid_pix = qt_pix;
    if(iPixelFormat < 1 || iPixelFormat > qt_valid_pix)
    {
        SetLastError(0);
        return(FALSE);
    }
    curPFD = iPixelFormat;
    return(TRUE);
}

WGLAPI BOOL GLAPIENTRY DrvSwapBuffers(HDC hdc)
{
    (void) hdc;
    if (ctx_current < 0)
        return FALSE;

    if(wgl_ctx[ctx_current].ctx == NULL) {
        SetLastError(0);
        return(FALSE);
    }
    WMesaSwapBuffers();
    return(TRUE);
}

WGLAPI BOOL GLAPIENTRY DrvValidateVersion(DWORD version)
{
    (void) version;
    return TRUE;
}
