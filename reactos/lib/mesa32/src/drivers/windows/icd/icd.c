/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * File name 	: wgl.c
 * WGL stuff. Added by Oleg Letsinsky, ajl@ultersys.ru
 * Some things originated from the 3Dfx WGL functions
 */

#ifdef WIN32

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
//#include <GL/glu.h>

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <tchar.h>
#include "wmesadef.h"
#include "GL/wmesa.h"
#include "mtypes.h"
#include "glapi.h"

#define MAX_MESA_ATTRS	20


typedef struct _icdTable {
    DWORD size;
    PROC  table[336];
} ICDTABLE, *PICDTABLE;

static ICDTABLE icdTable = { 336, {
#define ICD_ENTRY(func) mgl##func,
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
static unsigned ctx_current = -1;
static unsigned curPFD = 0;

WGLAPI BOOL GLAPIENTRY DrvCopyContext(HGLRC hglrcSrc,HGLRC hglrcDst,UINT mask)
{
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
    WMesaMakeCurrent(NULL);
    ctx_current = -1;
}

WGLAPI BOOL GLAPIENTRY DrvShareLists(HGLRC hglrc1,HGLRC hglrc2)
{
    return(TRUE);
}


static FIXED FixedFromDouble(double d)
{
   long l = (long) (d * 65536L);
   return *(FIXED *)&l;
}


WGLAPI BOOL GLAPIENTRY DrvDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane,UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    SetLastError(0);
    return(FALSE);
}

WGLAPI int GLAPIENTRY DrvSetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       CONST COLORREF *pcr)
{
    SetLastError(0);
    return(0);
}

WGLAPI int GLAPIENTRY DrvGetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       COLORREF *pcr)
{
    SetLastError(0);
    return(0);
}

WGLAPI BOOL GLAPIENTRY DrvRealizeLayerPalette(HDC hdc,int iLayerPlane,BOOL bRealize)
{
    SetLastError(0);
    return(FALSE);
}

WGLAPI BOOL GLAPIENTRY DrvSwapLayerBuffers(HDC hdc,UINT fuPlanes)
{
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
    int		qt_valid_pix;

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
   PROC p = (PROC) _glapi_get_proc_address((const char *) lpszProc);
   if (p)
      return p;

   SetLastError(0);
   return(NULL);
}

WGLAPI BOOL GLAPIENTRY DrvSetPixelFormat(HDC hdc,int iPixelFormat)
{
    int		qt_valid_pix;

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
    return TRUE;
}

#endif /* WIN32 */
