/* $Id$ */

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

struct __pixelformat__
{
    PIXELFORMATDESCRIPTOR	pfd;
    GLboolean doubleBuffered;
};

struct __pixelformat__	pix[] =
{
    /* Double Buffer, alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_TRUE
    },
    /* Single Buffer, alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	8,	24,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_FALSE
    },
    /* Double Buffer, no alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT|PFD_DOUBLEBUFFER|PFD_SWAP_COPY,
        PFD_TYPE_RGBA,
        24,	8,	0,	8,	8,	8,	16,	0,	0,
        0,	0,	0,	0,	0,	16,	8,	0,	0,	0,	0,	0,	0 },
        GL_TRUE
    },
    /* Single Buffer, no alpha */
    {	{	sizeof(PIXELFORMATDESCRIPTOR),	1,
        PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT,
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

WGLAPI BOOL GLAPIENTRY wglCopyContext(HGLRC hglrcSrc,HGLRC hglrcDst,UINT mask)
{
    (void) hglrcSrc; (void) hglrcDst; (void) mask;
    return(FALSE);
}

WGLAPI HGLRC GLAPIENTRY wglCreateContext(HDC hdc)
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

WGLAPI BOOL GLAPIENTRY wglDeleteContext(HGLRC hglrc)
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

WGLAPI HGLRC GLAPIENTRY wglCreateLayerContext(HDC hdc,int iLayerPlane)
{
    (void) hdc; (void) iLayerPlane;
    SetLastError(0);
    return(NULL);
}

WGLAPI HGLRC GLAPIENTRY wglGetCurrentContext(VOID)
{
   if (ctx_current < 0)
      return 0;
   else
      return (HGLRC) wgl_ctx[ctx_current].ctx;
}

WGLAPI HDC GLAPIENTRY wglGetCurrentDC(VOID)
{
   if (ctx_current < 0)
      return 0;
   else
      return wgl_ctx[ctx_current].hdc;
}

WGLAPI BOOL GLAPIENTRY wglMakeCurrent(HDC hdc,HGLRC hglrc)
{
    int i;

    /* new code suggested by Andy Sy */
    if (!hdc || !hglrc) {
       WMesaMakeCurrent(NULL);
       ctx_current = -1;
       return TRUE;
    }

    for ( i = 0; i < MESAWGL_CTX_MAX_COUNT; i++ )
    {
        if ( wgl_ctx[i].ctx == (PWMC) hglrc )
        {
            wgl_ctx[i].hdc = hdc;
            WMesaMakeCurrent( (WMesaContext) hglrc );
            ctx_current = i;
            return TRUE;
        }
    }
    return FALSE;
}

WGLAPI BOOL GLAPIENTRY wglShareLists(HGLRC hglrc1,HGLRC hglrc2)
{
    (void) hglrc1; (void) hglrc2;
    return(TRUE);
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
  // HDC bitDevice = CreateDC("DISPLAY", NULL, NULL, NULL);
  // VERIFY(bitDevice);

  // Swap fore and back colors so the bitmap has the right polarity
  tempColor = GetBkColor(bitDevice);
  SetBkColor(bitDevice, GetTextColor(bitDevice));
  SetTextColor(bitDevice, tempColor);

  // Place chars based on base line
  VERIFY(SetTextAlign(bitDevice, TA_BASELINE) != GDI_ERROR ? 1 : 0);

  for(i = 0; i < (int)numChars; i++) {
    SIZE size;
    char curChar;
    int charWidth,charHeight,bmapWidth,bmapHeight,numBytes,res;
    HBITMAP bitObject;
    HGDIOBJ origBmap;
    unsigned char *bmap;

    curChar = i + firstChar;

    // Find how high/wide this character is
    VERIFY(GetTextExtentPoint32(bitDevice, &curChar, 1, &size));

    // Create the output bitmap
    charWidth = size.cx;
    charHeight = size.cy;
    bmapWidth = ((charWidth + 31) / 32) * 32;   // Round up to the next multiple of 32 bits
    bmapHeight = charHeight;
    bitObject = CreateCompatibleBitmap(bitDevice,
                                       bmapWidth,
                                       bmapHeight);
    //VERIFY(bitObject);

    // Assign the output bitmap to the device
    origBmap = SelectObject(bitDevice, bitObject);
    (void) VERIFY(origBmap);

    VERIFY( PatBlt( bitDevice, 0, 0, bmapWidth, bmapHeight,BLACKNESS ) );

    // Use our source font on the device
    VERIFY(SelectObject(bitDevice, GetCurrentObject(fontDevice,OBJ_FONT)));

    // Draw the character
    VERIFY(TextOut(bitDevice, 0, metric.tmAscent, &curChar, 1));

    // Unselect our bmap object
    VERIFY(SelectObject(bitDevice, origBmap));

    // Convert the display dependant representation to a 1 bit deep DIB
    numBytes = (bmapWidth * bmapHeight) / 8;
    bmap = malloc(numBytes);
    dibInfo->bmiHeader.biWidth = bmapWidth;
    dibInfo->bmiHeader.biHeight = bmapHeight;
    res = GetDIBits(bitDevice, bitObject, 0, bmapHeight, bmap,
                    dibInfo,
                    DIB_RGB_COLORS);
    //VERIFY(res);

    // Create the GL object
    glNewList(i + listBase, GL_COMPILE);
    glBitmap(bmapWidth, bmapHeight, 0.0, metric.tmDescent,
             charWidth, 0.0,
             bmap);
    glEndList();
    // CheckGL();

    // Destroy the bmap object
    DeleteObject(bitObject);

    // Deallocate the bitmap data
    free(bmap);
  }

  // Destroy the DC
  VERIFY(DeleteDC(bitDevice));

  free(dibInfo);

  return TRUE;
#undef VERIFY
}

WGLAPI BOOL GLAPIENTRY wglUseFontBitmapsA(HDC hdc, DWORD first,
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
       == GDI_ERROR )
   {
       return wglUseFontBitmaps_FX( hdc, first, count, listBase );
   }

   /*
   ** Otherwise process all desired characters.
   */
   for (i = 0; i < (int)count; i++)
   {
       DWORD err;
       
      glNewList( font_list+i, GL_COMPILE );

      /* allocate space for the bitmap/outline */
      size = GetGlyphOutline(hdc, first + i, GGO_BITMAP, &gm, 0, NULL, &mat);
      if (size == GDI_ERROR)
      {
         glEndList( );
         err = GetLastError();
         success = FALSE;
         continue;
      }

      hBits  = GlobalAlloc(GHND, size+1);
      lpBits = GlobalLock(hBits);

      err = 
      GetGlyphOutline(hdc,    /* handle to device context */
                      first + i,          /* character to query */
                      GGO_BITMAP,         /* format of data to return */
                      &gm,                /* pointer to structure for metrics*/
                      size,               /* size of buffer for data */
                      lpBits,             /* pointer to buffer for data */
                      &mat                /* pointer to transformation */
                                          /* matrix structure */
                  );

      if (err == GDI_ERROR)
      {
         GlobalUnlock(hBits);
         GlobalFree(hBits);
         
         glEndList( );
         err = GetLastError();
         success = FALSE;
         continue;
      }

      glBitmap(gm.gmBlackBoxX,gm.gmBlackBoxY,
               -gm.gmptGlyphOrigin.x,
               gm.gmptGlyphOrigin.y,
               gm.gmCellIncX,gm.gmCellIncY,
               (const GLubyte * )lpBits);

      GlobalUnlock(hBits);
      GlobalFree(hBits);

      glEndList( );
   }

   return success;
}


WGLAPI BOOL GLAPIENTRY wglUseFontBitmapsW(HDC hdc,DWORD first,DWORD count,DWORD listBase)
{
    (void) hdc; (void) first; (void) count; (void) listBase;
    return FALSE;
}

WGLAPI BOOL GLAPIENTRY wglUseFontOutlinesA(HDC hdc,DWORD first,DWORD count,
                                  DWORD listBase,FLOAT deviation,
                                  FLOAT extrusion,int format,
                                  LPGLYPHMETRICSFLOAT lpgmf)
{
    (void) hdc; (void) first; (void) count;
    (void) listBase; (void) deviation; (void) extrusion; (void) format;
    (void) lpgmf;
    SetLastError(0);
    return(FALSE);
}

WGLAPI BOOL GLAPIENTRY wglUseFontOutlinesW(HDC hdc,DWORD first,DWORD count,
                                  DWORD listBase,FLOAT deviation,
                                  FLOAT extrusion,int format,
                                  LPGLYPHMETRICSFLOAT lpgmf)
{
    (void) hdc; (void) first; (void) count;
    (void) listBase; (void) deviation; (void) extrusion; (void) format;
    (void) lpgmf;
    SetLastError(0);
    return(FALSE);
}

WGLAPI BOOL GLAPIENTRY wglDescribeLayerPlane(HDC hdc,int iPixelFormat,
                                    int iLayerPlane,UINT nBytes,
                                    LPLAYERPLANEDESCRIPTOR plpd)
{
    (void) hdc; (void) iPixelFormat; (void) iLayerPlane; (void) nBytes; (void) plpd;
    SetLastError(0);
    return(FALSE);
}

WGLAPI int GLAPIENTRY wglSetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       CONST COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WGLAPI int GLAPIENTRY wglGetLayerPaletteEntries(HDC hdc,int iLayerPlane,
                                       int iStart,int cEntries,
                                       COLORREF *pcr)
{
    (void) hdc; (void) iLayerPlane; (void) iStart; (void) cEntries; (void) pcr;
    SetLastError(0);
    return(0);
}

WGLAPI BOOL GLAPIENTRY wglRealizeLayerPalette(HDC hdc,int iLayerPlane,BOOL bRealize)
{
    (void) hdc; (void) iLayerPlane; (void) bRealize;
    SetLastError(0);
    return(FALSE);
}

WGLAPI BOOL GLAPIENTRY wglSwapLayerBuffers(HDC hdc,UINT fuPlanes)
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

WGLAPI int GLAPIENTRY wglChoosePixelFormat(HDC hdc,
                                  CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    int		i,best = -1,bestdelta = 0x7FFFFFFF,delta,qt_valid_pix;
    (void) hdc;

    qt_valid_pix = qt_pix;
    if(ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR) || ppfd->nVersion != 1)
    {
        SetLastError(0);
        return(0);
    }
    for(i = 0;i < qt_valid_pix;i++)
    {
        delta = 0;
        if(
            (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) &&
            !(pix[i].pfd.dwFlags & PFD_DRAW_TO_WINDOW))
            continue;
        if(
            (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) &&
            !(pix[i].pfd.dwFlags & PFD_DRAW_TO_BITMAP))
            continue;
        if(
            (ppfd->dwFlags & PFD_SUPPORT_GDI) &&
            !(pix[i].pfd.dwFlags & PFD_SUPPORT_GDI))
            continue;
        if(
            (ppfd->dwFlags & PFD_SUPPORT_OPENGL) &&
            !(pix[i].pfd.dwFlags & PFD_SUPPORT_OPENGL))
            continue;
        if(
            !(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE) &&
            ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != (pix[i].pfd.dwFlags & PFD_DOUBLEBUFFER)))
            continue;
        if(
            !(ppfd->dwFlags & PFD_STEREO_DONTCARE) &&
            ((ppfd->dwFlags & PFD_STEREO) != (pix[i].pfd.dwFlags & PFD_STEREO)))
            continue;
        if(ppfd->iPixelType != pix[i].pfd.iPixelType)
            delta++;
        if(ppfd->cAlphaBits != pix[i].pfd.cAlphaBits)
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

WGLAPI int GLAPIENTRY wglDescribePixelFormat(HDC hdc,int iPixelFormat,UINT nBytes,
                                    LPPIXELFORMATDESCRIPTOR ppfd)
{
    int		qt_valid_pix;
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
WGLAPI PROC GLAPIENTRY wglGetProcAddress(LPCSTR lpszProc)
{
   PROC p = (PROC) _glapi_get_proc_address((const char *) lpszProc);
   if (p)
      return p;

   SetLastError(0);
   return(NULL);
}

WGLAPI int GLAPIENTRY wglGetPixelFormat(HDC hdc)
{
    (void) hdc;
    if(curPFD == 0)
    {
        SetLastError(0);
        return(0);
    }
    return(curPFD);
}

WGLAPI BOOL GLAPIENTRY wglSetPixelFormat(HDC hdc,int iPixelFormat,
                                PIXELFORMATDESCRIPTOR *ppfd)
{
    int		qt_valid_pix;
    (void) hdc;

    qt_valid_pix = qt_pix;
    if(iPixelFormat < 1 || iPixelFormat > qt_valid_pix || ppfd->nSize != sizeof(PIXELFORMATDESCRIPTOR))
    {
        SetLastError(0);
        return(FALSE);
    }
    curPFD = iPixelFormat;
    return(TRUE);
}

WGLAPI BOOL GLAPIENTRY wglSwapBuffers(HDC hdc)
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

#endif /* WIN32 */
