/*
 *  ReactOS Gdi32
 *  Copyright (C) 2003 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * 
 *
 */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <string.h>

#define NDEBUG
#include <win32k/debug1.h>



typedef int  (*CHOOSEPIXELFMT) (HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL (*SETPIXELFMT) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL (*SWAPBUFFERS) (HDC hdc);
typedef int  (*DESCRIBEPIXELFMT) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
typedef int  (*GETPIXELFMT) (HDC);


static CHOOSEPIXELFMT    glChoosePixelFormat   = NULL;
static SETPIXELFMT       glSetPixelFormat      = NULL;
static SWAPBUFFERS       glSwapBuffers         = NULL;
static DESCRIBEPIXELFMT  glDescribePixelFormat = NULL;
static GETPIXELFMT       glGetPixelFormat      = NULL;

/*
		OpenGL Handle.
*/
HINSTANCE                hOpenGL               = NULL;

static BOOL OpenGLEnable(void)
{
  if(hOpenGL == NULL)
  {
     hOpenGL = LoadLibraryA("OPENGL32.DLL");
     if(hOpenGL == NULL)
       return(FALSE);
  }

  if(glChoosePixelFormat == NULL) {
        glChoosePixelFormat = (CHOOSEPIXELFMT)GetProcAddress(hOpenGL, "wglChoosePixelFormat");
        if(glChoosePixelFormat == NULL)
                return(0);
  }

  if(glSetPixelFormat == NULL) {
        glSetPixelFormat = (SETPIXELFMT)GetProcAddress(hOpenGL, "wglSetPixelFormat");
        if(glSetPixelFormat == NULL)
                return(FALSE);
  }

  if(glSwapBuffers == NULL) {
        glSwapBuffers = (SWAPBUFFERS)GetProcAddress(hOpenGL, "wglSwapBuffers");
        if(glSwapBuffers == NULL)
                return(FALSE);
  }

  if(glDescribePixelFormat == NULL) {
        glDescribePixelFormat = (DESCRIBEPIXELFMT)GetProcAddress(hOpenGL, "wglDescribePixelFormat");
        if(glDescribePixelFormat == NULL)
                return(FALSE);
  }

  if(glGetPixelFormat == NULL) {
        glGetPixelFormat = (GETPIXELFMT)GetProcAddress(hOpenGL, "wglGetPixelFormat");
        if(glGetPixelFormat == NULL)
                return(FALSE);
  }

  return(TRUE);	/* OpenGL is initialized and enabled*/
}



/*
 * @implemented
 */
INT
STDCALL
ChoosePixelFormat(HDC  hDC,
                  CONST PIXELFORMATDESCRIPTOR * pfd)
{
  if (glChoosePixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glChoosePixelFormat(hDC, pfd));
}



/*
 * @implemented
 */
INT
STDCALL
DescribePixelFormat(HDC  hDC,
                    INT  PixelFormat,
                    UINT  BufSize,
                    LPPIXELFORMATDESCRIPTOR  pfd)
{
  if (glDescribePixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glDescribePixelFormat(hDC, PixelFormat, BufSize, pfd));
}



/*
 * @implemented
 */
INT
STDCALL
GetPixelFormat(HDC  hDC)
{
  if (glGetPixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glGetPixelFormat(hDC));
}



/*
 * @implemented
 */
BOOL
STDCALL
SetPixelFormat(HDC  hDC,
               INT  PixelFormat,
               CONST PIXELFORMATDESCRIPTOR * pfd)
{
  if (glSetPixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glSetPixelFormat(hDC, PixelFormat, pfd));
}



/*
 * @implemented
 */
BOOL
STDCALL
SwapBuffers(HDC  hDC)
{
  if (glSwapBuffers == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);


  return(glSwapBuffers(hDC));
}


/*
	Do this here for now.
*/

/*
 * @implemented
 */
UINT
STDCALL
GetEnhMetaFilePixelFormat(
	HENHMETAFILE			hEmh,
	DWORD				cbBufz,
	CONST PIXELFORMATDESCRIPTOR	*ppfd
	)
{
	ENHMETAHEADER pemh;

	if(GetEnhMetaFileHeader(hEmh, sizeof(ENHMETAHEADER), &pemh))
	{
	if(pemh.bOpenGL)
	{
		if(pemh.cbPixelFormat)
		{
		memcpy((void*)ppfd, (const void *)pemh.offPixelFormat, cbBufz );
		return(pemh.cbPixelFormat);
		}
	}
	}
	return(0);
}

/* EOF */
