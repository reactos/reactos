/*	File name	:	wmesadef.h
 *  Version		:	2.3
 *
 *  Header file for display driver for Mesa 2.3  under
 *	Windows95, WindowsNT and Win32
 *
 *	Copyright (C) 1996-  Li Wei
 *  Address		:		Institute of Artificial Intelligence
 *				:			& Robotics
 *				:		Xi'an Jiaotong University
 *  Email		:		liwei@aiar.xjtu.edu.cn
 *  Web page	:		http://sun.aiar.xjtu.edu.cn
 *
 *  This file and its associations are partially based on the
 *  Windows NT driver for Mesa, written by Mark Leaming
 *  (mark@rsinc.com).
 */

/*
 * $Log: wmesadef.h,v 
 * Revision 1.1.1.1  1999/08/19 00:55:42  jt
 * Imported source
 
 * Revision 1.3  1999/01/03 03:08:57  brian
 * Ted Jump's change
 *
 * Initial version 1997/6/14 CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 */

/*
 * $Log: wmesadef.h,v 
 * Revision 1.1.1.1  1999/08/19 00:55:42  jt
 * Imported source
 
 * Revision 1.3  1999/01/03 03:08:57  brian
 * Ted Jump's change
 *
 * Revision 2.1  1996/11/15 10:54:00  CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * a new element added to wmesa_context :
 * dither_flag
 */

/*
 * $Log: wmesadef.h,v 
 * Revision 1.1.1.1  1999/08/19 00:55:42  jt
 * Imported source
 
 * Revision 1.3  1999/01/03 03:08:57  brian
 * Ted Jump's change
 *
 * Revision 2.0  1996/11/15 10:54:00  CST by Li Wei(liwei@aiar.xjtu.edu.cn)
 * Initial revision
 */



#ifndef DDMESADEF_H
#define DDMESADEF_H

// uncomment this to use DirectDraw driver
//#define DDRAW 1
// uncomment this to use a pointer to a function for setting the pixels
// in the buffer
#define COMPILE_SETPIXEL 1
// uncomment this to enable the fast win32 rasterizers ( commented out for MesaGL 4.0 )
// #define FAST_RASTERIZERS 1
// uncomment this to enable setting function pointers once inside of
// WMesaCreateContext instead of on every call to wmesa_update_state()
#define SET_FPOINTERS_ONCE 1


#include <windows.h>
#include <GL/gl.h>
#include "context.h"
#ifdef DDRAW
#define DIRECTDRAW_VERSION 0x0100
	#include <ddraw.h>
#endif
//#include "profile.h"

#define REDBITS		0x03
#define REDSHIFT	0x00
#define GREENBITS	0x03
#define GREENSHIFT	0x03
#define BLUEBITS	0x02
#define BLUESHIFT	0x06

typedef struct _dibSection{
	HDC		hDC;
	HANDLE	hFileMap;
	BOOL	fFlushed;
	LPVOID	base;
}WMDIBSECTION, *PWMDIBSECTION;

#ifdef COMPILE_SETPIXEL
typedef void (*SETPIXELTYPE)(struct wmesa_context *pwc, int iScanLine, int iPixel, BYTE r, BYTE g, BYTE b);
#endif

typedef struct wmesa_context{
    GLcontext *gl_ctx;		/* The core GL/Mesa context */
	GLvisual *gl_visual;		/* Describes the buffers */
    GLframebuffer *gl_buffer;	/* Depth, stencil, accum, etc buffers */


	HWND				Window;
    HDC                 hDC;
    HPALETTE            hPalette;
    HPALETTE            hOldPalette;
    HPEN                hPen;
    HPEN                hOldPen;
    HCURSOR             hOldCursor;
    COLORREF            crColor;
    // 3D projection stuff
    RECT                drawRect;
    UINT                uiDIBoffset;
    // OpenGL stuff
    HPALETTE            hGLPalette;
	GLuint				width;
	GLuint				height;
	GLuint				ScanWidth;
	GLboolean			db_flag;	//* double buffered?
	GLboolean			rgb_flag;	//* RGB mode?
	GLboolean			dither_flag;	//* use dither when 256 color mode for RGB?
	GLuint				depth;		//* bits per pixel (1, 8, 24, etc)
	ULONG				pixel;	// current color index or RGBA pixel value
	ULONG				clearpixel; //* pixel for clearing the color buffers
	PBYTE				ScreenMem; // WinG memory
	BITMAPINFO			*IndexFormat;
	HPALETTE			hPal; // Current Palette
	HPALETTE			hPalHalfTone;


	WMDIBSECTION		dib;
    BITMAPINFO          bmi;
    HBITMAP             hbmDIB;
    HBITMAP             hOldBitmap;
	HBITMAP				Old_Compat_BM;
	HBITMAP				Compat_BM;            // Bitmap for double buffering
    PBYTE               pbPixels;
    int                 nColors;
	BYTE				cColorBits;
	int					pixelformat;

#ifdef DDRAW
	LPDIRECTDRAW            lpDD;           // DirectDraw object
//	LPDIRECTDRAW2            lpDD2;           // DirectDraw object
	LPDIRECTDRAWSURFACE     lpDDSPrimary;   // DirectDraw primary surface
	LPDIRECTDRAWSURFACE     lpDDSOffScreen;	// DirectDraw off screen surface
	LPDIRECTDRAWPALETTE     lpDDPal;        // DirectDraw palette
	BOOL                    bActive;        // is application active?
	DDSURFACEDESC	        ddsd;			// surface description
	int 					fullScreen;		// fullscreen ?
	int			            gMode ;			// fullscreen mode
	LONG					oldWndProc;		// old Window proc. we need to hook WM_MOVE message to update the drawing rectangle
#endif
	RECT					rectOffScreen;
	RECT					rectSurface;
	HWND					hwnd;
	DWORD					pitch;
	PBYTE					addrOffScreen;
#ifdef COMPILE_SETPIXEL
      SETPIXELTYPE                    wmSetPixel;
#endif // COMPILE_SETPIXEL
//#ifdef PROFILE
//	MESAPROF	profile;
//#endif
}  *PWMC;


#define PAGE_FILE		0xffffffff



#endif
