/* $Id: wgl.c,v 1.12 2006/03/30 07:58:24 kschultz Exp $ */

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

/* 
 * This file contains the implementation of the wgl* functions for
 * Mesa on Windows.  Since these functions are provided by Windows in
 * GDI/OpenGL, we must supply our versions that work with Mesa here.
 */


/* We're essentially building part of GDI here, so define this so that
 * we get the right export linkage. */
#ifdef __MINGW32__

#include <stdarg.h>
#include <windef.h>
#include <wincon.h>
#include <winbase.h>

#  if defined(BUILD_GL32)
#    define WINGDIAPI __declspec(dllexport)	
#  else
#    define __W32API_USE_DLLIMPORT__
#  endif

#include <wingdi.h>
#include "GL/mesa_wgl.h"
#include <stdlib.h>

#else

#define _GDI32_
#include <windows.h>

#endif

#include "glapi.h"
#include "GL/wmesa.h"   /* protos for wmesa* functions */

/*
 * Pixel Format Descriptors
 */

/* Extend the PFD to include DB flag */
struct __pixelformat__
{
    PIXELFORMATDESCRIPTOR pfd;
    GLboolean doubleBuffered;
};

/* These are the PFD's supported by this driver. */
struct __pixelformat__	pfd[] =
{
#if 0
    /* Double Buffer, alpha */
    {	
	{	
	    sizeof(PIXELFORMATDESCRIPTOR),	1,
	    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|
	    PFD_GENERIC_FORMAT|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
	    PFD_TYPE_RGBA,
	    24,	
	    8, 0,	
	    8, 8,	
	    8, 16,	
	    8, 24,
	    0, 0, 0, 0, 0,	
	    16,	8,	
	    0, 0, 0,	
	    0, 0, 0 
	},
        GL_TRUE
    },
    /* Single Buffer, alpha */
    {	
	{	
	    sizeof(PIXELFORMATDESCRIPTOR),	1,
	    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|
	    PFD_GENERIC_FORMAT,
	    PFD_TYPE_RGBA,
	    24,	
	    8, 0,	
	    8, 8,	
	    8, 16,	
	    8, 24,
	    0, 0, 0, 0,	0,	
	    16,	8,
	    0, 0, 0,	
	    0, 0, 0
	},
        GL_FALSE
    },
#endif
    /* Double Buffer, no alpha */
    {	
	{	
	    sizeof(PIXELFORMATDESCRIPTOR),	1,
	    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|
	    PFD_GENERIC_FORMAT|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
	    PFD_TYPE_RGBA,
	    24,	
	    8, 0,
	    8, 8,
	    8, 16,
	    0, 0,
	    0, 0, 0, 0,	0,
	    16,	8,
	    0, 0, 0, 
	    0, 0, 0 
	},
        GL_TRUE
    },
    /* Single Buffer, no alpha */
    {	
	{
	    sizeof(PIXELFORMATDESCRIPTOR),	1,
	    PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|
	    PFD_GENERIC_FORMAT,
	    PFD_TYPE_RGBA,
	    24,	
	    8, 0,
	    8, 8,
	    8, 16,
	    0, 0,
	    0, 0, 0, 0,	0,
	    16,	8,
	    0, 0, 0,
	    0, 0, 0 
	},
        GL_FALSE
    },
};

int npfd = sizeof(pfd) / sizeof(pfd[0]);


/*
 * Contexts
 */

typedef struct {
    WMesaContext ctx;
} MesaWglCtx;

#define MESAWGL_CTX_MAX_COUNT 20

static MesaWglCtx wgl_ctx[MESAWGL_CTX_MAX_COUNT];

static unsigned ctx_count = 0;
static int ctx_current = -1;
static unsigned curPFD = 0;

static HDC CurrentHDC = 0;


WINGDIAPI HGLRC GLAPIENTRY wglCreateContext(HDC hdc)
{
    int i = 0;
    if (!ctx_count) {
	for(i=0;i<MESAWGL_CTX_MAX_COUNT;i++) {
	    wgl_ctx[i].ctx = NULL;
	}
    }
    for( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ ) {
        if ( wgl_ctx[i].ctx == NULL ) {
            wgl_ctx[i].ctx = 
		WMesaCreateContext(hdc, NULL, (GLboolean)GL_TRUE,
				   (GLboolean) (pfd[curPFD-1].doubleBuffered ?
                                   GL_TRUE : GL_FALSE), 
				   (GLboolean)(pfd[curPFD-1].pfd.cAlphaBits ? 
				   GL_TRUE : GL_FALSE) );
            if (wgl_ctx[i].ctx == NULL)
                break;
            ctx_count++;
            return ((HGLRC)wgl_ctx[i].ctx);
        }
    }
    SetLastError(0);
    return(NULL);
}

WINGDIAPI BOOL GLAPIENTRY wglDeleteContext(HGLRC hglrc)
{
    int i;
    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ ) {
    	if ( wgl_ctx[i].ctx == (WMesaContext) hglrc ){
            WMesaMakeCurrent((WMesaContext) hglrc, NULL);
            WMesaDestroyContext(wgl_ctx[i].ctx);
            wgl_ctx[i].ctx = NULL;
            ctx_count--;
            return(TRUE);
    	}
    }
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI HGLRC GLAPIENTRY wglGetCurrentContext(VOID)
{
    if (ctx_current < 0)
	return 0;
    else
	return (HGLRC) wgl_ctx[ctx_current].ctx;
}

WINGDIAPI HDC GLAPIENTRY wglGetCurrentDC(VOID)
{
    return CurrentHDC;
}

WINGDIAPI BOOL GLAPIENTRY wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    int i;
    
    CurrentHDC = hdc;

    if (!hdc || !hglrc) {
	WMesaMakeCurrent(NULL, NULL);
	ctx_current = -1;
	return TRUE;
    }
    
    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ ) {
	if ( wgl_ctx[i].ctx == (WMesaContext) hglrc ) {
	    WMesaMakeCurrent( (WMesaContext) hglrc, hdc );
	    ctx_current = i;
	    return TRUE;
	}
    }
    return FALSE;
}


WINGDIAPI int GLAPIENTRY wglChoosePixelFormat(HDC hdc,
					      CONST 
					      PIXELFORMATDESCRIPTOR *ppfd)
{
    int		i,best = -1,bestdelta = 0x7FFFFFFF,delta;
    (void) hdc;
    
    if(ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR) || ppfd->nVersion != 1)
	{
	    SetLastError(0);
	    return(0);
	}
    for(i = 0; i < npfd;i++)
	{
	    delta = 0;
	    if(
		(ppfd->dwFlags & PFD_DRAW_TO_WINDOW) &&
		!(pfd[i].pfd.dwFlags & PFD_DRAW_TO_WINDOW))
		continue;
	    if(
		(ppfd->dwFlags & PFD_DRAW_TO_BITMAP) &&
		!(pfd[i].pfd.dwFlags & PFD_DRAW_TO_BITMAP))
		continue;
	    if(
		(ppfd->dwFlags & PFD_SUPPORT_GDI) &&
		!(pfd[i].pfd.dwFlags & PFD_SUPPORT_GDI))
		continue;
	    if(
		(ppfd->dwFlags & PFD_SUPPORT_OPENGL) &&
		!(pfd[i].pfd.dwFlags & PFD_SUPPORT_OPENGL))
		continue;
	    if(
		!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
		((ppfd->dwFlags & PFD_DOUBLEBUFFER) != 
		 (pfd[i].pfd.dwFlags & PFD_DOUBLEBUFFER)))
		continue;
	    if(
		!(ppfd->dwFlags & PFD_STEREO_DONTCARE) &&
		((ppfd->dwFlags & PFD_STEREO) != 
		 (pfd[i].pfd.dwFlags & PFD_STEREO)))
		continue;
	    if(ppfd->iPixelType != pfd[i].pfd.iPixelType)
		delta++;
	    if(ppfd->cAlphaBits != pfd[i].pfd.cAlphaBits)
		delta++;
	    if(delta < bestdelta)
		{
		    best = i + 1;
		    bestdelta = delta;
		    if(bestdelta == 0)
			break;
		}
	}
    if(best == -1)
	{
	    SetLastError(0);
	    return(0);
	}
    return(best);
}

WINGDIAPI int GLAPIENTRY wglDescribePixelFormat(HDC hdc,
					        int iPixelFormat,
					        UINT nBytes,
					        LPPIXELFORMATDESCRIPTOR ppfd)
{
    (void) hdc;
    
    if(ppfd == NULL)
	return(npfd);
    if(iPixelFormat < 1 || iPixelFormat > npfd || 
       nBytes != sizeof(PIXELFORMATDESCRIPTOR))
	{
	    SetLastError(0);
	    return(0);
	}
    *ppfd = pfd[iPixelFormat - 1].pfd;
    return(npfd);
}

WINGDIAPI PROC GLAPIENTRY wglGetProcAddress(LPCSTR lpszProc)
{
    PROC p = (PROC) _glapi_get_proc_address((const char *) lpszProc);
    if (p)
	return p;
    
    SetLastError(0);
    return(NULL);
}

WINGDIAPI int GLAPIENTRY wglGetPixelFormat(HDC hdc)
{
    (void) hdc;
    if(curPFD == 0) {
	SetLastError(0);
	return(0);
    }
    return(curPFD);
}

WINGDIAPI BOOL GLAPIENTRY wglSetPixelFormat(HDC hdc,int iPixelFormat,
					const PIXELFORMATDESCRIPTOR *ppfd)
{
    (void) hdc;
    
    if(iPixelFormat < 1 || iPixelFormat > npfd || 
       ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR)) {
	SetLastError(0);
	return(FALSE);
    }
    curPFD = iPixelFormat;
    return(TRUE);
}

WINGDIAPI BOOL GLAPIENTRY wglSwapBuffers(HDC hdc)
{
    WMesaSwapBuffers(hdc);
    return TRUE;
}

static FIXED FixedFromDouble(double d)
{
   long l = (long) (d * 65536L);
   return *(FIXED *) (void *) &l;
}


/*
** This is cribbed from FX/fxwgl.c, and seems to implement support
** for bitmap fonts where the wglUseFontBitmapsA() code implements
** support for outline fonts.  In combination they hopefully give
** fairly generic support for fonts.
*/
static BOOL wglUseFontBitmaps_FX(HDC fontDevice, DWORD firstChar,
                                 DWORD numChars, DWORD listBase)
{
#define VERIFY(a) a
    
    TEXTMETRIC metric;
    BITMAPINFO *dibInfo;
    HDC bitDevice;
    COLORREF tempColor;
    int i;
    
    VERIFY(GetTextMetrics(fontDevice, &metric));
    
    dibInfo = (BITMAPINFO *) calloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD), 1);
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
    VERIFY(SetTextAlign(bitDevice, TA_BASELINE) != GDI_ERROR ? 1 : 0);
    
    for(i = 0; i < (int)numChars; i++) {
	SIZE size;
	char curChar;
	int charWidth,charHeight,bmapWidth,bmapHeight,numBytes,res;
	HBITMAP bitObject;
	HGDIOBJ origBmap;
	unsigned char *bmap;
	
	curChar = (char)(i + firstChar);
	
	/* Find how high/wide this character is */
	VERIFY(GetTextExtentPoint32(bitDevice, &curChar, 1, &size));
	
	/* Create the output bitmap */
	charWidth = size.cx;
	charHeight = size.cy;
	/* Round up to the next multiple of 32 bits */
	bmapWidth = ((charWidth + 31) / 32) * 32;   
	bmapHeight = charHeight;
	bitObject = CreateCompatibleBitmap(bitDevice,
					   bmapWidth,
					   bmapHeight);
	/* VERIFY(bitObject); */
	
	/* Assign the output bitmap to the device */
	origBmap = SelectObject(bitDevice, bitObject);
	(void) VERIFY(origBmap);
	
	VERIFY( PatBlt( bitDevice, 0, 0, bmapWidth, bmapHeight,BLACKNESS ) );
	
	/* Use our source font on the device */
	VERIFY(SelectObject(bitDevice, GetCurrentObject(fontDevice,OBJ_FONT)));
	
	/* Draw the character */
	VERIFY(TextOut(bitDevice, 0, metric.tmAscent, &curChar, 1));
	
	/* Unselect our bmap object */
	VERIFY(SelectObject(bitDevice, origBmap));
	
	/* Convert the display dependant representation to a 1 bit deep DIB */
	numBytes = (bmapWidth * bmapHeight) / 8;
	bmap = malloc(numBytes);
	dibInfo->bmiHeader.biWidth = bmapWidth;
	dibInfo->bmiHeader.biHeight = bmapHeight;
	res = GetDIBits(bitDevice, bitObject, 0, bmapHeight, bmap,
			dibInfo,
			DIB_RGB_COLORS);
	/* VERIFY(res); */
	
	/* Create the GL object */
	glNewList(i + listBase, GL_COMPILE);
	glBitmap(bmapWidth, bmapHeight, 0.0, (GLfloat)metric.tmDescent,
		 (GLfloat)charWidth, 0.0,
		 bmap);
	glEndList();
	/* CheckGL(); */
	
	/* Destroy the bmap object */
	DeleteObject(bitObject);
	
	/* Deallocate the bitmap data */
	free(bmap);
    }
    
    /* Destroy the DC */
    VERIFY(DeleteDC(bitDevice));
    
    free(dibInfo);
    
    return TRUE;
#undef VERIFY
}

WINGDIAPI BOOL GLAPIENTRY wglUseFontBitmapsA(HDC hdc, DWORD first,
					     DWORD count, DWORD listBase)
{
    int i;
    GLuint font_list;
    DWORD size;
    GLYPHMETRICS gm;
    HANDLE hBits;
    LPSTR lpBits;
    MAT2 mat;
    int  success = TRUE;
    
    if (count == 0)
	return FALSE;
    
    font_list = listBase;
    
    mat.eM11 = FixedFromDouble(1);
    mat.eM12 = FixedFromDouble(0);
    mat.eM21 = FixedFromDouble(0);
    mat.eM22 = FixedFromDouble(-1);
    
    memset(&gm,0,sizeof(gm));
    
    /*
    ** If we can't get the glyph outline, it may be because this is a fixed
    ** font.  Try processing it that way.
    */
    if( GetGlyphOutline(hdc, first, GGO_BITMAP, &gm, 0, NULL, &mat)
	== GDI_ERROR ) {
	return wglUseFontBitmaps_FX( hdc, first, count, listBase );
    }
    
    /*
    ** Otherwise process all desired characters.
    */
    for (i = 0; i < (int)count; i++) {
	DWORD err;
	
	glNewList( font_list+i, GL_COMPILE );
	
	/* allocate space for the bitmap/outline */
	size = GetGlyphOutline(hdc, first + i, GGO_BITMAP, 
			       &gm, 0, NULL, &mat);
	if (size == GDI_ERROR) {
	    glEndList( );
	    err = GetLastError();
	    success = FALSE;
	    continue;
	}
	
	hBits  = GlobalAlloc(GHND, size+1);
	lpBits = GlobalLock(hBits);
	
	err = 
	    GetGlyphOutline(hdc,         /* handle to device context */
			    first + i,   /* character to query */
			    GGO_BITMAP,  /* format of data to return */
			    &gm,         /* ptr to structure for metrics*/
			    size,        /* size of buffer for data */
			    lpBits,      /* pointer to buffer for data */
			    &mat         /* pointer to transformation */
			    /* matrix structure */
		);
	
	if (err == GDI_ERROR) {
	    GlobalUnlock(hBits);
	    GlobalFree(hBits);
	    
	    glEndList( );
	    err = GetLastError();
	    success = FALSE;
	    continue;
	}
	
	glBitmap(gm.gmBlackBoxX,gm.gmBlackBoxY,
		 (GLfloat)-gm.gmptGlyphOrigin.x,
		 (GLfloat)gm.gmptGlyphOrigin.y,
		 (GLfloat)gm.gmCellIncX,
		 (GLfloat)gm.gmCellIncY,
		 (const GLubyte * )lpBits);
	
	GlobalUnlock(hBits);
	GlobalFree(hBits);
	
	glEndList( );
    }
    
    return success;
}



/* NOT IMPLEMENTED YET */
WINGDIAPI BOOL GLAPIENTRY wglCopyContext(HGLRC hglrcSrc,
					 HGLRC hglrcDst,
					 UINT mask)
{
    (void) hglrcSrc; (void) hglrcDst; (void) mask;
    return(FALSE);
}

WINGDIAPI HGLRC GLAPIENTRY wglCreateLayerContext(HDC hdc,
						 int iLayerPlane)
{
    (void) hdc; (void) iLayerPlane;
    SetLastError(0);
    return(NULL);
}

WINGDIAPI BOOL GLAPIENTRY wglShareLists(HGLRC hglrc1,
					HGLRC hglrc2)
{
    (void) hglrc1; (void) hglrc2;
    return(TRUE);
}


WINGDIAPI BOOL GLAPIENTRY wglUseFontBitmapsW(HDC hdc,
					     DWORD first,
					     DWORD count,
					     DWORD listBase)
{
    (void) hdc; (void) first; (void) count; (void) listBase;
    return FALSE;
}

WINGDIAPI BOOL GLAPIENTRY wglUseFontOutlinesA(HDC hdc,
					      DWORD first,
					      DWORD count,
					      DWORD listBase,
					      FLOAT deviation,
					      FLOAT extrusion,
					      int format,
					      LPGLYPHMETRICSFLOAT lpgmf)
{
    (void) hdc; (void) first; (void) count;
    (void) listBase; (void) deviation; (void) extrusion; (void) format;
    (void) lpgmf;
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI BOOL GLAPIENTRY wglUseFontOutlinesW(HDC hdc,
					      DWORD first,
					      DWORD count,
					      DWORD listBase,
					      FLOAT deviation,
					      FLOAT extrusion,
					      int format,
					      LPGLYPHMETRICSFLOAT lpgmf)
{
    (void) hdc; (void) first; (void) count;
    (void) listBase; (void) deviation; (void) extrusion; (void) format;
    (void) lpgmf;
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI BOOL GLAPIENTRY wglDescribeLayerPlane(HDC hdc,
						int iPixelFormat,
						int iLayerPlane,
						UINT nBytes,
						LPLAYERPLANEDESCRIPTOR plpd)
{
    (void) hdc; (void) iPixelFormat; (void) iLayerPlane; 
    (void) nBytes; (void) plpd;
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI int GLAPIENTRY wglSetLayerPaletteEntries(HDC hdc,
						   int iLayerPlane,
						   int iStart,
						   int cEntries,
						   CONST COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; 
    (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WINGDIAPI int GLAPIENTRY wglGetLayerPaletteEntries(HDC hdc,
						   int iLayerPlane,
						   int iStart,
						   int cEntries,
						   COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WINGDIAPI BOOL GLAPIENTRY wglRealizeLayerPalette(HDC hdc,
						 int iLayerPlane,
						 BOOL bRealize)
{
    (void) hdc; (void) iLayerPlane; (void) bRealize;
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI BOOL GLAPIENTRY wglSwapLayerBuffers(HDC hdc,
                                              UINT fuPlanes)
{
    (void) hdc; (void) fuPlanes;
    SetLastError(0);
    return(FALSE);
}

WINGDIAPI const char * GLAPIENTRY wglGetExtensionsStringARB(HDC hdc)
{
     /* WGL_ARB_render_texture */
    return "WGL_ARB_extensions_string WGL_"
           "ARB_pixel_format WGL_ARB_multi"
           "sample WGL_EXT_swap_control WG"
           "L_ARB_pbuffer WGL_ARB_render_t"
           "exture WGL_ARB_make_current_re"
           "ad WGL_EXT_extensions_string W"
           "GL_ARB_buffer_region ";
}

GLAPI const char * GLAPIENTRY
wglGetExtensionsStringEXT (void)
{
    /* WGL_ARB_render_texture */
    return "WGL_ARB_extensions_string WGL_"
           "ARB_pixel_format WGL_ARB_multi"
           "sample WGL_EXT_swap_control WG"
           "L_ARB_pbuffer WGL_ARB_render_t"
           "exture WGL_ARB_make_current_re"
           "ad WGL_EXT_extensions_string W"
           "GL_ARB_buffer_region ";
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

GLAPI BOOL GLAPIENTRY
wglSwapIntervalEXT (int interval)
{
   /*
   WMesaContext ctx = wglGetCurrentContext();
   if (ctx == NULL) {
      return FALSE;
   }
   if (interval < 0) {
      interval = 0;
   } else if (interval > 3) {
      interval = 3;
   }
   ctx->gl_ctx.swapInterval = interval;
   return TRUE;
   */
    return FALSE;
}

GLAPI int GLAPIENTRY
wglGetSwapIntervalEXT (void)
{
    /*

   WMesaContext ctx = wglGetCurrentContext();

   if (ctx == NULL) {
      return -1;
   }
   return (int)ctx->gl_ctx.swapInterval;
   */
    return -1;
}

