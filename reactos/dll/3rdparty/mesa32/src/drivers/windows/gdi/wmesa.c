/*
 * Windows (Win32) device driver for Mesa
 *
 */

#include "wmesadef.h"
#include "colors.h"
#include <GL/wmesa.h>
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "drivers/common/driverfuncs.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#define FLIP(Y)  (Current->height-(Y)-1)

/* Static Data */

static PWMC Current = NULL;

/*
 * Every driver should implement a GetString function in order to
 * return a meaningful GL_RENDERER string.
 */
static const GLubyte *wmesa_get_string(GLcontext *ctx, GLenum name)
{
    return (name == GL_RENDERER) ? 
	(GLubyte *) "Mesa Windows GDI Driver" : NULL;
}

/*
 * Determine the pixel format based on the pixel size.
 */
static void wmSetPixelFormat(PWMC pwc, HDC hDC)
{
    pwc->cColorBits = GetDeviceCaps(hDC, BITSPIXEL);

    // TEMP - only 16 and 32 bit targets are supported now
    assert(pwc->cColorBits == 16 || 
	   pwc->cColorBits == 32);

    switch(pwc->cColorBits){
    case 8:
	pwc->pixelformat = PF_INDEX8;
	break;
    case 16:
	pwc->pixelformat = PF_5R6G5B;
	break;
    case 32:
	pwc->pixelformat = PF_8R8G8B;
	break;
    default:
	pwc->pixelformat = PF_BADFORMAT;
    }
}

/*
 * Create DIB for back buffer.
 * We write into this memory with the span routines and then blit it
 * to the window on a buffer swap.
 */

BOOL wmCreateBackingStore(PWMC pwc, long lxSize, long lySize)
{
    HDC          hdc = pwc->hDC;
    LPBITMAPINFO pbmi = &(pwc->bmi);
    HDC          hic;

    assert(pwc->db_flag == GL_TRUE);

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = lxSize;
    pbmi->bmiHeader.biHeight= -lySize;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = GetDeviceCaps(pwc->hDC, BITSPIXEL);
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;
    
    pwc->cColorBits = pbmi->bmiHeader.biBitCount;
    pwc->ScanWidth = (lxSize * (pwc->cColorBits / 8) + 3) & ~3;
    
    hic = CreateIC("display", NULL, NULL, NULL);
    pwc->dib.hDC = CreateCompatibleDC(hic);
    
    pwc->hbmDIB = CreateDIBSection(hic,
				   &pwc->bmi,
				   DIB_RGB_COLORS,
				   (void **)&(pwc->pbPixels),
				   0,
				   0);
    pwc->hOldBitmap = SelectObject(pwc->dib.hDC, pwc->hbmDIB);
    
    DeleteDC(hic);

    wmSetPixelFormat(pwc, pwc->hDC);
    return TRUE;
}

static void wmDeleteBackingStore(PWMC pwc)
{
    if (pwc->hbmDIB) {
	SelectObject(pwc->dib.hDC, pwc->hOldBitmap);
	DeleteDC(pwc->dib.hDC);
	DeleteObject(pwc->hbmDIB);
    }
}

static void wmesa_get_buffer_size(GLframebuffer *buffer, 
				  GLuint *width, 
				  GLuint *height )
{
    *width = Current->width;
    *height = Current->height;
}


static void wmesa_flush(GLcontext* ctx)
{
    if (Current->db_flag) {
	BitBlt(Current->hDC, 0, 0, Current->width, Current->height,
	       Current->dib.hDC, 0, 0, SRCCOPY);
    }
    else {
	/* Do nothing for single buffer */
    }
}


/**********************************************************************/
/*****                   CLEAR Functions                          *****/
/**********************************************************************/

/* If we do not implement these, Mesa clears the buffers via the pixel
 * span writing interface, which is very slow for a clear operation.
 */

/*
 * Set the color index used to clear the color buffer.
 */
static void clear_index(GLcontext* ctx, GLuint index)
{
    /* Note that indexed mode is not supported yet */
    Current->clearColorRef = RGB(0,0,0);
}

/*
 * Set the color used to clear the color buffer.
 */
static void clear_color(GLcontext* ctx, const GLfloat color[4])
{
    GLubyte col[3];
    UINT    bytesPerPixel = Current->cColorBits / 8; 

    CLAMPED_FLOAT_TO_UBYTE(col[0], color[0]);
    CLAMPED_FLOAT_TO_UBYTE(col[1], color[1]);
    CLAMPED_FLOAT_TO_UBYTE(col[2], color[2]);
    Current->clearColorRef = RGB(col[0], col[1], col[2]);
    DeleteObject(Current->clearPen);
    DeleteObject(Current->clearBrush);
    Current->clearPen = CreatePen(PS_SOLID, 1, Current->clearColorRef); 
    Current->clearBrush = CreateSolidBrush(Current->clearColorRef); 
}


/* 
 * Clear the specified region of the color buffer using the clear color 
 * or index as specified by one of the two functions above. 
 * 
 * This procedure clears either the front and/or the back COLOR buffers. 
 * Only the "left" buffer is cleared since we are not stereo. 
 * Clearing of the other non-color buffers is left to the swrast. 
 */ 

static void clear(GLcontext* ctx, 
		  GLbitfield mask, 
		  GLboolean all, 
		  GLint x, GLint y, 
		  GLint width, GLint height) 
{
    int done = 0;

    /* Let swrast do all the work if the masks are not set to
     * clear all channels. */
    if (ctx->Color.ColorMask[0] != 0xff ||
	ctx->Color.ColorMask[1] != 0xff ||
	ctx->Color.ColorMask[2] != 0xff ||
	ctx->Color.ColorMask[3] != 0xff) {
	_swrast_Clear(ctx, mask, all, x, y, width, height); 
	return;
    }

    /* 'all' means clear the entire window */
    if (all) { 
	x = y = 0; 
	width = Current->width; 
	height = Current->height; 
    } 

    /* Back buffer */
    if (mask & BUFFER_BIT_BACK_LEFT) { 
	
	int     i, rowSize; 
	UINT    bytesPerPixel = Current->cColorBits / 8; 
	LPBYTE  lpb, clearRow;
	LPWORD  lpw;
	BYTE    bColor; 
	WORD    wColor; 
	BYTE    r, g, b; 
	DWORD   dwColor; 
	LPDWORD lpdw; 
	
	/* Try for a fast clear - clearing entire buffer with a single
	 * byte value. */
	if (all) { /* entire buffer */
	    /* Now check for an easy clear value */
	    switch (bytesPerPixel) {
	    case 1:
		bColor = BGR8(GetRValue(Current->clearColorRef), 
			      GetGValue(Current->clearColorRef), 
			      GetBValue(Current->clearColorRef));
		memset(Current->pbPixels, bColor, 
		       Current->ScanWidth * height);
		done = 1;
		break;
	    case 2:
		wColor = BGR16(GetRValue(Current->clearColorRef), 
			       GetGValue(Current->clearColorRef), 
			       GetBValue(Current->clearColorRef)); 
		if (((wColor >> 8) & 0xff) == (wColor & 0xff)) {
		    memset(Current->pbPixels, wColor & 0xff, 
			   Current->ScanWidth * height);
		    done = 1;
		}
		break;
	    case 3:
		/* fall through */
	    case 4:
		if (GetRValue(Current->clearColorRef) == 
		    GetGValue(Current->clearColorRef) &&
		    GetRValue(Current->clearColorRef) == 
		    GetBValue(Current->clearColorRef)) {
		    memset(Current->pbPixels, 
			   GetRValue(Current->clearColorRef), 
			   Current->ScanWidth * height);
		    done = 1;
		}
		break;
	    default:
		break;
	    }
	} /* all */

	if (!done) {
	    /* Need to clear a row at a time.  Begin by setting the first
	     * row in the area to be cleared to the clear color. */
	    
	    clearRow = Current->pbPixels + 
		Current->ScanWidth * FLIP(y) +
		bytesPerPixel * x; 
	    switch (bytesPerPixel) {
	    case 1:
		lpb = clearRow;
		bColor = BGR8(GetRValue(Current->clearColorRef), 
			      GetGValue(Current->clearColorRef), 
			      GetBValue(Current->clearColorRef));
		memset(lpb, bColor, width);
		break;
	    case 2:
		lpw = (LPWORD)clearRow;
		wColor = BGR16(GetRValue(Current->clearColorRef), 
			       GetGValue(Current->clearColorRef), 
			       GetBValue(Current->clearColorRef)); 
		for (i=0; i<width; i++)
		    *lpw++ = wColor;
		break;
	    case 3: 
		lpb = clearRow;
		r = GetRValue(Current->clearColorRef); 
		g = GetGValue(Current->clearColorRef); 
		b = GetBValue(Current->clearColorRef); 
		for (i=0; i<width; i++) {
		    *lpb++ = b; 
		    *lpb++ = g; 
		    *lpb++ = r; 
		} 
		break;
	    case 4: 
		lpdw = (LPDWORD)clearRow; 
		dwColor = BGR32(GetRValue(Current->clearColorRef), 
				GetGValue(Current->clearColorRef), 
				GetBValue(Current->clearColorRef)); 
		for (i=0; i<width; i++)
		    *lpdw++ = dwColor;
		break;
	    default:
		break;
	    } /* switch */
	    
	    /* copy cleared row to other rows in buffer */
	    lpb = clearRow - Current->ScanWidth;
	    rowSize = width * bytesPerPixel;
	    for (i=1; i<height; i++) { 
		memcpy(lpb, clearRow, rowSize); 
		lpb -= Current->ScanWidth; 
	    } 
	} /* not done */
	mask &= ~BUFFER_BIT_BACK_LEFT;
    } /* back buffer */ 

    /* front buffer */
    if (mask & BUFFER_BIT_FRONT_LEFT) { 
	HDC DC = Current->hDC; 
	HPEN Old_Pen = SelectObject(DC, Current->clearPen); 
	HBRUSH Old_Brush = SelectObject(DC, Current->clearBrush);
	Rectangle(DC,
		  x,
		  FLIP(y),
		  x + width + 1,
		  FLIP(y) - height + 1);
	SelectObject(DC, Old_Pen); 
	SelectObject(DC, Old_Brush); 
	mask &= ~BUFFER_BIT_FRONT_LEFT;
    } /* front buffer */ 
    
    /* Call swrast if there is anything left to clear (like DEPTH) */ 
    if (mask) 
	_swrast_Clear(ctx, mask, all, x, y, width, height); 
  
} 


/**********************************************************************/
/*****                   PIXEL Functions                          *****/
/**********************************************************************/

/* SINGLE BUFFER */

/* These are slow, but work with all non-indexed visual types */

/* Write a horizontal span of RGBA color pixels with a boolean mask. */
static void write_rgba_span_single(const GLcontext* ctx, 
				   struct gl_renderbuffer *rb, 
				   GLuint n, GLint x, GLint y,
				   const GLubyte rgba[][4], 
				   const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    
    (void) ctx;
    y=FLIP(y);
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
		SetPixel(pwc->hDC, x+i, y, RGB(rgba[i][RCOMP], rgba[i][GCOMP], 
					       rgba[i][BCOMP]));
    }
    else {
	for (i=0; i<n; i++)
	    SetPixel(pwc->hDC, x+i, y, RGB(rgba[i][RCOMP], rgba[i][GCOMP], 
					   rgba[i][BCOMP]));
    }
    
}

/* Write a horizontal span of RGB color pixels with a boolean mask. */
static void write_rgb_span_single(const GLcontext* ctx, 
				  struct gl_renderbuffer *rb, 
				  GLuint n, GLint x, GLint y,
				  const GLubyte rgb[][3], 
				  const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    
    (void) ctx;
    y=FLIP(y);
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
		SetPixel(pwc->hDC, x+i, y, RGB(rgb[i][RCOMP], rgb[i][GCOMP], 
					       rgb[i][BCOMP]));
    }
    else {
	for (i=0; i<n; i++)
	    SetPixel(pwc->hDC, x+i, y, RGB(rgb[i][RCOMP], rgb[i][GCOMP], 
					   rgb[i][BCOMP]));
    }
    
}

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_span_single(const GLcontext* ctx, 
					struct gl_renderbuffer *rb,
					GLuint n, GLint x, GLint y,
					const GLchan color[4], 
					const GLubyte mask[])
{
    GLuint i;
    PWMC pwc = Current;
    COLORREF colorref;

    (void) ctx;
    colorref = RGB(color[RCOMP], color[GCOMP], color[BCOMP]);
    y=FLIP(y);
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
		SetPixel(pwc->hDC, x+i, y, colorref);
    }
    else
	for (i=0; i<n; i++)
	    SetPixel(pwc->hDC, x+i, y, colorref);

}

/* Write an array of RGBA pixels with a boolean mask. */
static void write_rgba_pixels_single(const GLcontext* ctx, 
				     struct gl_renderbuffer *rb,
				     GLuint n, 
				     const GLint x[], const GLint y[],
				     const GLubyte rgba[][4], 
				     const GLubyte mask[] )
{
    GLuint i;
    PWMC    pwc = Current;
    (void) ctx;
    for (i=0; i<n; i++)
	if (mask[i])
	    SetPixel(pwc->hDC, x[i], FLIP(y[i]), 
		     RGB(rgba[i][RCOMP], rgba[i][GCOMP], 
			 rgba[i][BCOMP]));
}



/*
 * Write an array of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_pixels_single(const GLcontext* ctx, 
					  struct gl_renderbuffer *rb,
					  GLuint n,
					  const GLint x[], const GLint y[],
					  const GLchan color[4],
					  const GLubyte mask[] )
{
    GLuint i;
    PWMC    pwc = Current;
    COLORREF colorref;
    (void) ctx;
    colorref = RGB(color[RCOMP], color[GCOMP], color[BCOMP]);
    for (i=0; i<n; i++)
	if (mask[i])
	    SetPixel(pwc->hDC, x[i], FLIP(y[i]), colorref);
}

/* Read a horizontal span of color pixels. */
static void read_rgba_span_single(const GLcontext* ctx, 
				  struct gl_renderbuffer *rb,
				  GLuint n, GLint x, GLint y,
				  GLubyte rgba[][4] )
{
    GLuint i;
    COLORREF Color;
    y = FLIP(y);
    for (i=0; i<n; i++) {
	Color = GetPixel(Current->hDC, x+i, y);
	rgba[i][RCOMP] = GetRValue(Color);
	rgba[i][GCOMP] = GetGValue(Color);
	rgba[i][BCOMP] = GetBValue(Color);
	rgba[i][ACOMP] = 255;
    }
}


/* Read an array of color pixels. */
static void read_rgba_pixels_single(const GLcontext* ctx, 
				    struct gl_renderbuffer *rb,
				    GLuint n, const GLint x[], const GLint y[],
				    GLubyte rgba[][4])
{
  GLuint i;
  COLORREF Color;
  for (i=0; i<n; i++) {
      GLint y2 = FLIP(y[i]);
      Color = GetPixel(Current->hDC, x[i], y2);
      rgba[i][RCOMP] = GetRValue(Color);
      rgba[i][GCOMP] = GetGValue(Color);
      rgba[i][BCOMP] = GetBValue(Color);
      rgba[i][ACOMP] = 255;
  }
}

/*********************************************************************/

/* DOUBLE BUFFER 32-bit */

#define WMSETPIXEL32(pwc, y, x, r, g, b) { \
LPDWORD lpdw = ((LPDWORD)((pwc)->pbPixels + (pwc)->ScanWidth * (y)) + (x)); \
*lpdw = BGR32((r),(g),(b)); }



/* Write a horizontal span of RGBA color pixels with a boolean mask. */
static void write_rgba_span_32(const GLcontext* ctx, 
			       struct gl_renderbuffer *rb, 
			       GLuint n, GLint x, GLint y,
			       const GLubyte rgba[][4], 
			       const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    LPDWORD lpdw;

    (void) ctx;
    
    y=FLIP(y);
    lpdw = ((LPDWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpdw[i] = BGR32(rgba[i][RCOMP], rgba[i][GCOMP], 
				rgba[i][BCOMP]);
    }
    else {
	for (i=0; i<n; i++)
                *lpdw++ = BGR32(rgba[i][RCOMP], rgba[i][GCOMP], 
				rgba[i][BCOMP]);
    }
}


/* Write a horizontal span of RGB color pixels with a boolean mask. */
static void write_rgb_span_32(const GLcontext* ctx, 
			      struct gl_renderbuffer *rb, 
			      GLuint n, GLint x, GLint y,
			      const GLubyte rgb[][4], 
			      const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    LPDWORD lpdw;

    (void) ctx;
    
    y=FLIP(y);
    lpdw = ((LPDWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpdw[i] = BGR32(rgb[i][RCOMP], rgb[i][GCOMP], 
				rgb[i][BCOMP]);
    }
    else {
	for (i=0; i<n; i++)
                *lpdw++ = BGR32(rgb[i][RCOMP], rgb[i][GCOMP], 
				rgb[i][BCOMP]);
    }
}

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_span_32(const GLcontext* ctx, 
				    struct gl_renderbuffer *rb,
				    GLuint n, GLint x, GLint y,
				    const GLchan color[4], 
				    const GLubyte mask[])
{
    LPDWORD lpdw;
    DWORD pixel;
    GLuint i;
    PWMC pwc = Current;
    lpdw = ((LPDWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    y=FLIP(y);
    pixel = BGR32(color[RCOMP], color[GCOMP], color[BCOMP]);
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpdw[i] = pixel;
    }
    else
	for (i=0; i<n; i++)
                *lpdw++ = pixel;

}

/* Write an array of RGBA pixels with a boolean mask. */
static void write_rgba_pixels_32(const GLcontext* ctx, 
				 struct gl_renderbuffer *rb,
				 GLuint n, const GLint x[], const GLint y[],
				 const GLubyte rgba[][4], 
				 const GLubyte mask[])
{
    GLuint i;
    PWMC    pwc = Current;
    for (i=0; i<n; i++)
	if (mask[i])
	    WMSETPIXEL32(pwc, FLIP(y[i]), x[i],
			 rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
}

/*
 * Write an array of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_pixels_32(const GLcontext* ctx, 
				      struct gl_renderbuffer *rb,
				      GLuint n,
				      const GLint x[], const GLint y[],
				      const GLchan color[4],
				      const GLubyte mask[])
{
    GLuint i;
    PWMC pwc = Current;
    for (i=0; i<n; i++)
	if (mask[i])
	    WMSETPIXEL32(pwc, FLIP(y[i]),x[i],color[RCOMP],
			 color[GCOMP], color[BCOMP]);
}

/* Read a horizontal span of color pixels. */
static void read_rgba_span_32(const GLcontext* ctx, 
			      struct gl_renderbuffer *rb,
			      GLuint n, GLint x, GLint y,
			      GLubyte rgba[][4] )
{
    GLuint i;
    DWORD pixel;
    LPDWORD lpdw;
    PWMC pwc = Current;
    
    y = FLIP(y);
    lpdw = ((LPDWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    for (i=0; i<n; i++) {
	pixel = lpdw[i];
	rgba[i][RCOMP] = (pixel & 0x00ff0000) >> 16;
	rgba[i][GCOMP] = (pixel & 0x0000ff00) >> 8;
	rgba[i][BCOMP] = (pixel & 0x000000ff);
	rgba[i][ACOMP] = 255;
    }
}


/* Read an array of color pixels. */
static void read_rgba_pixels_32(const GLcontext* ctx, 
				struct gl_renderbuffer *rb,
				GLuint n, const GLint x[], const GLint y[],
				GLubyte rgba[][4])
{
    GLuint i;
    DWORD pixel;
    LPDWORD lpdw;
    PWMC pwc = Current;

    for (i=0; i<n; i++) {
	GLint y2 = FLIP(y[i]);
	lpdw = ((LPDWORD)(pwc->pbPixels + pwc->ScanWidth * y2)) + x[i];
	pixel = lpdw[i];
	rgba[i][RCOMP] = (pixel & 0x00ff0000) >> 16;
	rgba[i][GCOMP] = (pixel & 0x0000ff00) >> 8;
	rgba[i][BCOMP] = (pixel & 0x000000ff);
	rgba[i][ACOMP] = 255;
  }
}


/*********************************************************************/

/* DOUBLE BUFFER 16-bit */

#define WMSETPIXEL16(pwc, y, x, r, g, b) { \
LPWORD lpw = ((LPWORD)((pwc)->pbPixels + (pwc)->ScanWidth * (y)) + (x)); \
*lpw = BGR16((r),(g),(b)); }



/* Write a horizontal span of RGBA color pixels with a boolean mask. */
static void write_rgba_span_16(const GLcontext* ctx, 
			       struct gl_renderbuffer *rb, 
			       GLuint n, GLint x, GLint y,
			       const GLubyte rgba[][4], 
			       const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    LPWORD lpw;

    (void) ctx;
    
    y=FLIP(y);
    lpw = ((LPWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpw[i] = BGR16(rgba[i][RCOMP], rgba[i][GCOMP], 
			       rgba[i][BCOMP]);
    }
    else {
	for (i=0; i<n; i++)
                *lpw++ = BGR16(rgba[i][RCOMP], rgba[i][GCOMP], 
			       rgba[i][BCOMP]);
    }
}


/* Write a horizontal span of RGB color pixels with a boolean mask. */
static void write_rgb_span_16(const GLcontext* ctx, 
			      struct gl_renderbuffer *rb, 
			      GLuint n, GLint x, GLint y,
			      const GLubyte rgb[][4], 
			      const GLubyte mask[] )
{
    PWMC    pwc = Current;
    GLuint i;
    LPWORD lpw;

    (void) ctx;
    
    y=FLIP(y);
    lpw = ((LPWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpw[i] = BGR16(rgb[i][RCOMP], rgb[i][GCOMP], 
			       rgb[i][BCOMP]);
    }
    else {
	for (i=0; i<n; i++)
                *lpw++ = BGR16(rgb[i][RCOMP], rgb[i][GCOMP], 
			       rgb[i][BCOMP]);
    }
}

/*
 * Write a horizontal span of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_span_16(const GLcontext* ctx, 
				    struct gl_renderbuffer *rb,
				    GLuint n, GLint x, GLint y,
				    const GLchan color[4], 
				    const GLubyte mask[])
{
    LPWORD lpw;
    WORD pixel;
    GLuint i;
    PWMC pwc = Current;
    (void) ctx;
    lpw = ((LPWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    y=FLIP(y);
    pixel = BGR16(color[RCOMP], color[GCOMP], color[BCOMP]);
    if (mask) {
	for (i=0; i<n; i++)
	    if (mask[i])
                lpw[i] = pixel;
    }
    else
	for (i=0; i<n; i++)
                *lpw++ = pixel;

}

/* Write an array of RGBA pixels with a boolean mask. */
static void write_rgba_pixels_16(const GLcontext* ctx, 
				 struct gl_renderbuffer *rb,
				 GLuint n, const GLint x[], const GLint y[],
				 const GLubyte rgba[][4], 
				 const GLubyte mask[])
{
    GLuint i;
    PWMC    pwc = Current;
    (void) ctx;
    for (i=0; i<n; i++)
	if (mask[i])
	    WMSETPIXEL16(pwc, FLIP(y[i]), x[i],
			 rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP]);
}

/*
 * Write an array of pixels with a boolean mask.  The current color
 * is used for all pixels.
 */
static void write_mono_rgba_pixels_16(const GLcontext* ctx, 
				      struct gl_renderbuffer *rb,
				      GLuint n,
				      const GLint x[], const GLint y[],
				      const GLchan color[4],
				      const GLubyte mask[])
{
    GLuint i;
    PWMC    pwc = Current;
    (void) ctx;
    for (i=0; i<n; i++)
	if (mask[i])
	    WMSETPIXEL16(pwc, FLIP(y[i]),x[i],color[RCOMP],
			 color[GCOMP], color[BCOMP]);
}

/* Read a horizontal span of color pixels. */
static void read_rgba_span_16(const GLcontext* ctx, 
			      struct gl_renderbuffer *rb,
			      GLuint n, GLint x, GLint y,
			      GLubyte rgba[][4] )
{
    GLuint i, pixel;
    LPWORD lpw;
    PWMC pwc = Current;
    
    y = FLIP(y);
    lpw = ((LPWORD)(pwc->pbPixels + pwc->ScanWidth * y)) + x;
    for (i=0; i<n; i++) {
	pixel = lpw[i];
	/* Windows uses 5,5,5 for 16-bit */
	rgba[i][RCOMP] = (pixel & 0x7c00) >> 7;
	rgba[i][GCOMP] = (pixel & 0x03e0) >> 2;
	rgba[i][BCOMP] = (pixel & 0x001f) << 3;
	rgba[i][ACOMP] = 255;
    }
}


/* Read an array of color pixels. */
static void read_rgba_pixels_16(const GLcontext* ctx, 
				struct gl_renderbuffer *rb,
				GLuint n, const GLint x[], const GLint y[],
				GLubyte rgba[][4])
{
    GLuint i, pixel;
    LPWORD lpw;
    PWMC pwc = Current;

    for (i=0; i<n; i++) {
	GLint y2 = FLIP(y[i]);
	lpw = ((LPWORD)(pwc->pbPixels + pwc->ScanWidth * y2)) + x[i];
	pixel = lpw[i];
	/* Windows uses 5,5,5 for 16-bit */
	rgba[i][RCOMP] = (pixel & 0x7c00) >> 7;
	rgba[i][GCOMP] = (pixel & 0x03e0) >> 2;
	rgba[i][BCOMP] = (pixel & 0x001f) << 3;
	rgba[i][ACOMP] = 255;
  }
}




/**********************************************************************/
/*****                   BUFFER Functions                         *****/
/**********************************************************************/




static void
wmesa_delete_renderbuffer(struct gl_renderbuffer *rb)
{
    _mesa_free(rb);
}

static GLboolean
wmesa_renderbuffer_storage(GLcontext *ctx, 
			   struct gl_renderbuffer *rb,
			   GLenum internalFormat, 
			   GLuint width, 
			   GLuint height)
{
    return GL_TRUE;
}

/**
 * Called by ctx->Driver.ResizeBuffers()
 * Resize the front/back colorbuffers to match the latest window size.
 */
static void
wmesa_resize_buffers(GLcontext *ctx, GLframebuffer *buffer,
                     GLuint width, GLuint height)
{
    RECT CR;
    if (Current->width != width || Current->height != height) {
	Current->width = width;
	Current->height = height;
	/* Realloc back buffer */
	if (Current->db_flag) {
	    wmDeleteBackingStore(Current);
	    wmCreateBackingStore(Current, width, height);
	}
	GetClientRect(Current->Window, &CR);
	Current->width = CR.right;
	Current->height = CR.bottom;
    }
    _mesa_resize_framebuffer(ctx, buffer, width, height);
}


/**
 * Called by glViewport.
 * This is a good time for us to poll the current window size and adjust
 * our renderbuffers to match the current window size.
 * Remember, we have no opportunity to respond to conventional
 * resize events since the driver has no event loop.
 * Thus, we poll.
 * Note that this trick isn't fool-proof.  If the application never calls
 * glViewport, our notion of the current window size may be incorrect.
 */
static void wmesa_viewport(GLcontext *ctx, 
			   GLint x, GLint y, 
			   GLsizei width, GLsizei height)
{
   if (Current->width != width || Current->height != height) {
       wmesa_resize_buffers(ctx, ctx->WinSysDrawBuffer, width, height);
       ctx->NewState |= _NEW_BUFFERS;  /* to update scissor / window bounds */
   }
}




/**
 * Called when the driver should update it's state, based on the new_state
 * flags.
 */
static void wmesa_update_state(GLcontext *ctx, GLuint new_state)
{
    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _ac_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);

    /* TODO - need code to update the span functions in case the
     * renderer changes the target buffer (like a DB app writing to
     * the front buffer). */
}





/**********************************************************************/
/*****                   WMESA Functions                          *****/
/**********************************************************************/

WMesaContext WMesaCreateContext(HWND hWnd, 
				HPALETTE* Pal,
				GLboolean rgb_flag,
				GLboolean db_flag,
				GLboolean alpha_flag)
{
    RECT CR;
    WMesaContext c;
    struct dd_function_table functions;
    struct gl_renderbuffer *rb;

    (void) Pal;
    
    /* Indexed mode not supported */
    if (!rgb_flag)
	return NULL;

    c = CALLOC_STRUCT(wmesa_context);
    if (!c)
	return NULL;
    
    c->Window = hWnd;
    c->hDC = GetDC(hWnd);
    GetClientRect(c->Window, &CR);
    c->width = CR.right;
    c->height = CR.bottom;

    c->clearPen = CreatePen(PS_SOLID, 1, 0); 
    c->clearBrush = CreateSolidBrush(0); 

    /* Create back buffer if double buffered */
    if (db_flag) {
	c->db_flag = 1; 	
	wmCreateBackingStore(c, c->width, c->height);
		
    }
    
    c->gl_visual = _mesa_create_visual(rgb_flag,
				       db_flag,    /* db_flag */
				       GL_FALSE,   /* stereo */
				       8,8,8,      /* color RGB */
				       alpha_flag ? 8 : 0, /* color A */
				       0,          /* index bits */
				       16,         /* depth_bits */
				       8,          /* stencil_bits */
				       16,16,16,   /* accum RGB */
				       alpha_flag ? 16 : 0, /* accum A */
				       1);
    
    if (!c->gl_visual) {
	_mesa_free(c);
	return NULL;
    }

    /* Set up driver functions */
    _mesa_init_driver_functions(&functions);
    /* Fill in required functions */
    functions.GetString = wmesa_get_string;
    functions.UpdateState = wmesa_update_state;
    functions.GetBufferSize = wmesa_get_buffer_size;
    functions.Flush = wmesa_flush;

    functions.Clear = clear;
    functions.ClearIndex = clear_index;
    functions.ClearColor = clear_color;

    functions.ResizeBuffers = wmesa_resize_buffers;
    functions.Viewport = wmesa_viewport;

    /* allocate a new Mesa context */
    c->gl_ctx = _mesa_create_context(c->gl_visual, NULL,
				     &functions, (void *)c);
    if (!c->gl_ctx) {
	_mesa_destroy_visual( c->gl_visual );
	_mesa_free(c);
	return NULL;
    }
    
    _mesa_enable_sw_extensions(c->gl_ctx);
    _mesa_enable_1_3_extensions(c->gl_ctx);
    _mesa_enable_1_4_extensions(c->gl_ctx);
    _mesa_enable_1_5_extensions(c->gl_ctx);
    _mesa_enable_2_0_extensions(c->gl_ctx);
    
    c->gl_buffer = _mesa_create_framebuffer(c->gl_visual);
    if (!c->gl_buffer) {
	_mesa_destroy_visual(c->gl_visual);
	_mesa_free_context_data(c->gl_ctx);
	_mesa_free(c);
	return NULL;
    }
    
    rb = CALLOC_STRUCT(gl_renderbuffer);

    if (!rb) {
	_mesa_destroy_visual(c->gl_visual);
	_mesa_destroy_framebuffer(c->gl_buffer);
	_mesa_free_context_data(c->gl_ctx);
	_mesa_free(c);
	return NULL;
    }

    _mesa_init_renderbuffer(rb, (GLuint)0);
    
    rb->_BaseFormat = GL_RGBA;
    rb->InternalFormat = GL_RGBA;
    rb->DataType = CHAN_TYPE;
    rb->Delete = wmesa_delete_renderbuffer;
    rb->AllocStorage = wmesa_renderbuffer_storage;

    if (db_flag) {
	switch(c->cColorBits) {
	case 16:
	    rb->PutRow = write_rgba_span_16;
	    rb->PutRowRGB = write_rgb_span_16;
	    rb->PutMonoRow = write_mono_rgba_span_16;
	    rb->PutValues = write_rgba_pixels_16;
	    rb->PutMonoValues = write_mono_rgba_pixels_16;
	    rb->GetRow = read_rgba_span_16;
	    rb->GetValues = read_rgba_pixels_16;
	    break;
	case 32:
	    rb->PutRow = write_rgba_span_32;
	    rb->PutRowRGB = write_rgb_span_32;
	    rb->PutMonoRow = write_mono_rgba_span_32;
	    rb->PutValues = write_rgba_pixels_32;
	    rb->PutMonoValues = write_mono_rgba_pixels_32;
	    rb->GetRow = read_rgba_span_32;
	    rb->GetValues = read_rgba_pixels_32;
	    break;
	default:
	    break;
	}
	_mesa_add_renderbuffer(c->gl_buffer, BUFFER_BACK_LEFT, rb);
    }
    else { /* single buffer */
	rb->PutRow = write_rgba_span_single;
	rb->PutRowRGB = write_rgb_span_single;
	rb->PutMonoRow = write_mono_rgba_span_single;
	rb->PutValues = write_rgba_pixels_single;
	rb->PutMonoValues = write_mono_rgba_pixels_single;
	rb->GetRow = read_rgba_span_single;
	rb->GetValues = read_rgba_pixels_single;
    }
    _mesa_add_renderbuffer(c->gl_buffer, BUFFER_FRONT_LEFT, rb);

    _mesa_add_soft_renderbuffers(c->gl_buffer,
				 GL_FALSE, /* color */
				 c->gl_visual->depthBits > 0,
				 c->gl_visual->stencilBits > 0,
				 c->gl_visual->accumRedBits > 0,
				 alpha_flag, 
				 GL_FALSE);

    /* Initialize the software rasterizer and helper modules. */
    if (!_swrast_CreateContext(c->gl_ctx) ||
        !_ac_CreateContext(c->gl_ctx) ||
        !_tnl_CreateContext(c->gl_ctx) ||
	!_swsetup_CreateContext(c->gl_ctx)) {
	_mesa_destroy_visual(c->gl_visual);
	_mesa_destroy_framebuffer(c->gl_buffer);
	_mesa_free_context_data(c->gl_ctx);
	_mesa_free(c);
	return NULL;
    }
    
    _swsetup_Wakeup(c->gl_ctx);
    
    TNL_CONTEXT(c->gl_ctx)->Driver.RunPipeline = _tnl_run_pipeline;

    return c;
}

void WMesaDestroyContext( void )
{
    WMesaContext c = Current;

    WMesaMakeCurrent(NULL);

    ReleaseDC(c->Window, c->hDC);
    DeleteObject(c->clearPen); 
    DeleteObject(c->clearBrush); 
    
    if (c->db_flag)
	wmDeleteBackingStore(c);

    _swsetup_DestroyContext(c->gl_ctx);
    _tnl_DestroyContext(c->gl_ctx);
    _ac_DestroyContext(c->gl_ctx);
    _swrast_DestroyContext(c->gl_ctx);
    
    _mesa_destroy_visual(c->gl_visual);
    _mesa_destroy_framebuffer(c->gl_buffer);
    _mesa_free_context_data(c->gl_ctx);
    _mesa_free(c->gl_ctx);
    _mesa_free(c);
}


void WMesaMakeCurrent(WMesaContext c)
{
    /* return if already current */
    if (Current == c)
	return;
    
    if ((Current = c) != 0)
	_mesa_make_current(c->gl_ctx, c->gl_buffer, c->gl_buffer);
}


void WMesaSwapBuffers( void )
{
    GET_CURRENT_CONTEXT(ctx);
    
    /* If we're swapping the buffer associated with the current context
     * we have to flush any pending rendering commands first.
     */
    if (Current && Current->gl_ctx == ctx)
	_mesa_notifySwapBuffers(ctx);
    
    if (Current->db_flag)
	wmesa_flush(ctx);
}

/**********************************************************************/
/*****                        END                                 *****/
/**********************************************************************/
