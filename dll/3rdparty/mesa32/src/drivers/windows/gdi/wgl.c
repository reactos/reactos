
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
#include "config.h"
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
	    DEFAULT_SOFTWARE_DEPTH_BITS,	8,	
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
	    DEFAULT_SOFTWARE_DEPTH_BITS,	8,	
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
	    DEFAULT_SOFTWARE_DEPTH_BITS,	8,	
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
	    DEFAULT_SOFTWARE_DEPTH_BITS,	8,	
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

WINGDIAPI BOOL GLAPIENTRY wglShareLists(HGLRC hglrc1,
					HGLRC hglrc2)
{
    WMesaShareLists((WMesaContext)hglrc1, (WMesaContext)hglrc2);
    return(TRUE);
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
    SetLastError(0);
    if (iLayerPlane == 0)
        return wglCreateContext( hdc );
    return(NULL);
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

/* WGL_ARB_pixel_format */
#define WGL_NUMBER_PIXEL_FORMATS_ARB    0x2000
#define WGL_DRAW_TO_WINDOW_ARB          0x2001
#define WGL_DRAW_TO_BITMAP_ARB          0x2002
#define WGL_ACCELERATION_ARB            0x2003
#define WGL_NEED_PALETTE_ARB            0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB     0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB      0x2006
#define WGL_SWAP_METHOD_ARB             0x2007
#define WGL_NUMBER_UNDERLAYS_ARB        0x2009
#define WGL_TRANSPARENT_ARB             0x200A
#define WGL_SHARE_DEPTH_ARB             0x200C
#define WGL_SHARE_ACCUM_ARB             0x200E
#define WGL_SUPPORT_GDI_ARB             0x200F
#define WGL_SUPPORT_OPENGL_ARB          0x2010
#define WGL_DOUBLE_BUFFER_ARB           0x2011
#define WGL_STEREO_ARB                  0x2012
#define WGL_PIXEL_TYPE_ARB              0x2013
#define WGL_COLOR_BITS_ARB              0x2014
#define WGL_RED_BITS_ARB                0x2015
#define WGL_RED_SHIFT_ARB               0x2016
#define WGL_GREEN_BITS_ARB              0x2017
#define WGL_GREEN_SHIFT_ARB             0x2018
#define WGL_BLUE_BITS_ARB               0x2019
#define WGL_BLUE_SHIFT_ARB              0x201A
#define WGL_ALPHA_BITS_ARB              0x201B
#define WGL_ALPHA_SHIFT_ARB             0x201C
#define WGL_ACCUM_BITS_ARB              0x201D
#define WGL_ACCUM_RED_BITS_ARB          0x201E
#define WGL_ACCUM_GREEN_BITS_ARB        0x201F
#define WGL_ACCUM_BLUE_BITS_ARB         0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB        0x2021
#define WGL_DEPTH_BITS_ARB              0x2022
#define WGL_STENCIL_BITS_ARB            0x2023
#define WGL_AUX_BUFFERS_ARB             0x2024
#define WGL_NO_ACCELERATION_ARB         0x2025
#define WGL_GENERIC_ACCELERATION_ARB    0x2026
#define WGL_FULL_ACCELERATION_ARB       0x2027
#define WGL_DRAW_TO_PBUFFER_ARB         0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB      0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB       0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB      0x2030
#define WGL_SAMPLE_BUFFERS_ARB          0x2041
#define WGL_SAMPLES_ARB                 0x2042


GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribivARB (HDC hdc,
                              int iPixelFormat,
                              int iLayerPlane,
                              UINT nAttributes,
                              int *piAttributes,
                              int *piValues)
{
    BOOL retVal = FALSE;
    BOOL Count = 0;
    int i;

    for (i=0;i<nAttributes;i++)
    {
        switch (piAttributes[i])
        {

            case WGL_ACCELERATION_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ( ( pfd[iPixelFormat - 1].pfd.dwFlags & PFD_GENERIC_FORMAT ) == PFD_GENERIC_FORMAT)
                    {
                        piValues[i] = WGL_NO_ACCELERATION_ARB;  // or WGL_GENERIC_ACCELERATION_ARB ?
                    }

                    else if ( ( pfd[iPixelFormat - 1].pfd.dwFlags & PFD_GENERIC_FORMAT ) == PFD_GENERIC_ACCELERATED)
                    {
                        piValues[i] = WGL_GENERIC_ACCELERATION_ARB;  // or WGL_FULL_ACCELERATION_ARB ?
                    }
                    else
                    {
                        piValues[i] = WGL_FULL_ACCELERATION_ARB;  // or WGL_NO_ACCELERATION_ARB ?
                    }
                    Count++;
                }
                else
                {
                     SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                /* note from http://developer.3dlabs.com/documents/WGLmanpages/wglgetpixelformatattribarb.htm
                 *
                 * WGL_NO_ACCELERATION_ARB
                 * Only the software renderer supports this pixel format.
                 *
                 * WGL_GENERIC_ACCELERATION_ARB
                 * The pixel format is supported by an MCD driver.
                 *
                 * WGL_FULL_ACCELERATION_ARB
                 * The pixel format is supported by an ICD driver.  
                 */
                break;

            case WGL_ACCUM_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_ALPHA_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumAlphaBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;


            case WGL_ACCUM_BLUE_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumBlueBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_GREEN_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumGreenBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_RED_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumRedBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ALPHA_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAlphaBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ALPHA_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAlphaShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_AUX_BUFFERS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAuxBuffers;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_BLUE_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cBlueBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_BLUE_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cBlueShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_COLOR_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cColorBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_DEPTH_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cDepthBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_DRAW_TO_BITMAP_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.dwFlags & ~PFD_DRAW_TO_BITMAP;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_DRAW_TO_WINDOW_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.dwFlags & ~PFD_DRAW_TO_WINDOW;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_DRAW_TO_PBUFFER_ARB :
                piValues[i] = GL_TRUE;
                break;

            case WGL_DOUBLE_BUFFER_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_DOUBLEBUFFER) == PFD_DOUBLEBUFFER)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_GREEN_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cGreenBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_GREEN_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cGreenShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;


            case WGL_MAX_PBUFFER_PIXELS_ARB :
                // FIXME
                break;

            case WGL_MAX_PBUFFER_WIDTH_ARB :
                // FIXME
                break;

            case WGL_MAX_PBUFFER_HEIGHT_ARB :
                // FIXME
                break;

            case WGL_NEED_PALETTE_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_NEED_PALETTE) == PFD_NEED_PALETTE)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_NEED_SYSTEM_PALETTE_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_NEED_PALETTE) == PFD_NEED_SYSTEM_PALETTE)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_NUMBER_PIXEL_FORMATS_ARB :
                piValues[i] = (int)npfd;
                Count++;
                break;

            case WGL_NUMBER_UNDERLAYS_ARB :
                // FIXME
                break;
/*
            case WGL_OPTIMAL_PBUFFER_WIDTH_ARB
                // FIXME
                break;

            case WGL_OPTIMAL_PBUFFER_HEIGHT_ARB
                // FIXME
                break;
*/
            case WGL_PIXEL_TYPE_ARB :
                // FIXME
                break;

            case WGL_RED_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cRedBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_RED_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cRedShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_SAMPLES_ARB :
                // FIXME
                break;

            case WGL_SAMPLE_BUFFERS_ARB :
                // FIXME
                break;

            case WGL_SHARE_ACCUM_ARB :
                // FIXME - True if the layer plane shares the accumulation buffer with the main planes. If iLayerPlane is zero, this is always true.
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if (iLayerPlane == 0)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SHARE_DEPTH_ARB :
                // FIXME - True if the layer plane shares the depth buffer with the main planes. If iLayerPlane is zero, this is always true.
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if (iLayerPlane == 0)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;
                break;

            case WGL_STENCIL_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    piValues[i] = (int)pfd[iPixelFormat - 1].pfd.cStencilBits ;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_STEREO_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_STEREO) == PFD_STEREO)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SUPPORT_GDI_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_GDI) == PFD_SUPPORT_GDI)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SUPPORT_OPENGL_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_OPENGL) == PFD_SUPPORT_OPENGL)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SWAP_LAYER_BUFFERS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_OPENGL) == PFD_SWAP_LAYER_BUFFERS)
                    {
                        piValues[i] = GL_TRUE;
                    }
                    else
                    {
                        piValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SWAP_METHOD_ARB :
                // FIXME
                break;

            case WGL_TRANSPARENT_ARB :
                //FIXME after WGL_TRANSPARENT_VALUE been implement piValues[i] = GL_TRUE;
                piValues[i] = GL_FALSE;
                Count++;
                break;

            default :
                SetLastError(0);
                break;
        }
    }

    if(GetObjectType(hdc) != OBJ_DC)
    {
        SetLastError(ERROR_DC_NOT_FOUND);
    }
    else if (Count == nAttributes)
    {
       retVal = TRUE;
    }
    

    return retVal;
}

GLAPI BOOL GLAPIENTRY
wglGetPixelFormatAttribfvARB (HDC hdc,
                              int iPixelFormat,
                              int iLayerPlane,
                              UINT nAttributes,
                              int *piAttributes,
                              FLOAT *pfValues)
{
    BOOL retVal = FALSE;
    BOOL Count = 0;
    int i;

    for (i=0;i<nAttributes;i++)
    {
        switch (piAttributes[i])
        {

            case WGL_ACCELERATION_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ( ( pfd[iPixelFormat - 1].pfd.dwFlags & PFD_GENERIC_FORMAT ) == PFD_GENERIC_FORMAT)
                    {
                        pfValues[i] = WGL_NO_ACCELERATION_ARB;  // or WGL_GENERIC_ACCELERATION_ARB ?
                    }

                    else if ( ( pfd[iPixelFormat - 1].pfd.dwFlags & PFD_GENERIC_FORMAT ) == PFD_GENERIC_ACCELERATED)
                    {
                        pfValues[i] = WGL_GENERIC_ACCELERATION_ARB;  // or WGL_FULL_ACCELERATION_ARB ?
                    }
                    else
                    {
                        pfValues[i] = WGL_FULL_ACCELERATION_ARB;  // or WGL_NO_ACCELERATION_ARB ?
                    }
                    Count++;
                }
                else
                {
                     SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                /* note from http://developer.3dlabs.com/documents/WGLmanpages/wglgetpixelformatattribarb.htm
                 *
                 * WGL_NO_ACCELERATION_ARB
                 * Only the software renderer supports this pixel format.
                 *
                 * WGL_GENERIC_ACCELERATION_ARB
                 * The pixel format is supported by an MCD driver.
                 *
                 * WGL_FULL_ACCELERATION_ARB
                 * The pixel format is supported by an ICD driver.  
                 */
                break;

            case WGL_ACCUM_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_ALPHA_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumAlphaBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;


            case WGL_ACCUM_BLUE_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumBlueBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_GREEN_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumGreenBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ACCUM_RED_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAccumRedBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ALPHA_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAlphaBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_ALPHA_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAlphaShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_AUX_BUFFERS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cAuxBuffers;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_BLUE_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cBlueBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_BLUE_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cBlueShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_COLOR_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cColorBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_DEPTH_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cDepthBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_DRAW_TO_BITMAP_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.dwFlags & ~PFD_DRAW_TO_BITMAP;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_DRAW_TO_WINDOW_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.dwFlags & ~PFD_DRAW_TO_WINDOW;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_DRAW_TO_PBUFFER_ARB :
                pfValues[i] = GL_TRUE;
                break;

            case WGL_DOUBLE_BUFFER_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_DOUBLEBUFFER) == PFD_DOUBLEBUFFER)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_GREEN_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cGreenBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_GREEN_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cGreenShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;


            case WGL_MAX_PBUFFER_PIXELS_ARB :
                // FIXME
                break;

            case WGL_MAX_PBUFFER_WIDTH_ARB :
                // FIXME
                break;

            case WGL_MAX_PBUFFER_HEIGHT_ARB :
                // FIXME
                break;

            case WGL_NEED_PALETTE_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_NEED_PALETTE) == PFD_NEED_PALETTE)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_NEED_SYSTEM_PALETTE_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_NEED_PALETTE) == PFD_NEED_SYSTEM_PALETTE)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_NUMBER_PIXEL_FORMATS_ARB :
                pfValues[i] = (int)npfd;
                Count++;
                break;

            case WGL_NUMBER_UNDERLAYS_ARB :
                // FIXME
                break;
/*
            case WGL_OPTIMAL_PBUFFER_WIDTH_ARB
                // FIXME
                break;

            case WGL_OPTIMAL_PBUFFER_HEIGHT_ARB
                // FIXME
                break;
*/
            case WGL_PIXEL_TYPE_ARB :
                // FIXME
                break;

            case WGL_RED_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cRedBits;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_RED_SHIFT_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cRedShift;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }

                break;

            case WGL_SAMPLES_ARB :
                // FIXME
                break;

            case WGL_SAMPLE_BUFFERS_ARB :
                // FIXME
                break;

            case WGL_SHARE_ACCUM_ARB :
                // FIXME - True if the layer plane shares the accumulation buffer with the main planes. If iLayerPlane is zero, this is always true.
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if (iLayerPlane == 0)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SHARE_DEPTH_ARB :
                // FIXME - True if the layer plane shares the depth buffer with the main planes. If iLayerPlane is zero, this is always true.
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if (iLayerPlane == 0)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_STENCIL_BITS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    pfValues[i] = (int)pfd[iPixelFormat - 1].pfd.cStencilBits ;
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_STEREO_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_STEREO) == PFD_STEREO)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SUPPORT_GDI_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_GDI) == PFD_SUPPORT_GDI)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SUPPORT_OPENGL_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_OPENGL) == PFD_SUPPORT_OPENGL)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SWAP_LAYER_BUFFERS_ARB :
                if ((iPixelFormat > 0) &&  (iPixelFormat<=npfd))
                {
                    if ((pfd[iPixelFormat - 1].pfd.dwFlags & PFD_SUPPORT_OPENGL) == PFD_SWAP_LAYER_BUFFERS)
                    {
                        pfValues[i] = GL_TRUE;
                    }
                    else
                    {
                        pfValues[i] = GL_FALSE;
                    }
                    Count++;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PIXEL_FORMAT);
                }
                break;

            case WGL_SWAP_METHOD_ARB :
                // FIXME
                break;

            case WGL_TRANSPARENT_ARB :
                //FIXME after WGL_TRANSPARENT_VALUE been implement piValues[i] = GL_TRUE;
                pfValues[i] = GL_FALSE;
                Count++;
                break;

            default :
                SetLastError(0);
                break;
        }
    }

    if(GetObjectType(hdc) != OBJ_DC)
    {
        SetLastError(ERROR_DC_NOT_FOUND);
    }
    else if (Count == nAttributes)
    {
       retVal = TRUE;
    }

    return retVal;
}


GLAPI BOOL GLAPIENTRY
wglMakeContextCurrentARB(HDC hDrawDC,
                         HDC hReadDC,
                         HGLRC hglrc)
{
   SetLastError(0);
   return FALSE;
}

GLAPI HANDLE GLAPIENTRY
wglGetCurrentReadDCARB(void)
{
   SetLastError(0);
   return NULL;
}

typedef void *HPBUFFERARB;

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

GLAPI HANDLE GLAPIENTRY
wglCreateBufferRegionARB(HDC hDC,
                         int iLayerPlane,
                         UINT uType)
{
   SetLastError(0);
   return NULL;
}

GLAPI VOID GLAPIENTRY
wglDeleteBufferRegionARB(HANDLE hRegion)
{
   SetLastError(0);
   return;
}

GLAPI BOOL GLAPIENTRY
wglSaveBufferRegionARB(HANDLE hRegion,
                       int x,
                       int y,
                       int width,
                       int height)
{
   SetLastError(0);
   return FALSE;
}

GLAPI BOOL GLAPIENTRY
wglRestoreBufferRegionARB(HANDLE hRegion,
                          int x,
                          int y,
                          int width,
                          int height,
                          int xSrc,
                          int ySrc)
{
   SetLastError(0);
   return FALSE;
}

/* WGL_ARB_render_texture */
GLAPI BOOL GLAPIENTRY
wglSetPbufferAttribARB (HPBUFFERARB hPbuffer,
                        const int *piAttribList)
{
   SetLastError(0);
   return FALSE;
}

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




