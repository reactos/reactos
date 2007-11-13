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

#include "precomp.h"

#define NDEBUG
#include <debug.h>



typedef int  STDCALL (*CHOOSEPIXELFMT) (HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL STDCALL (*SETPIXELFMT) (HDC, int, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL STDCALL (*SWAPBUFFERS) (HDC hdc);
typedef int  STDCALL (*DESCRIBEPIXELFMT) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
typedef int  STDCALL (*GETPIXELFMT) (HDC);


static CHOOSEPIXELFMT    glChoosePixelFormat   = NULL;
static SETPIXELFMT       glSetPixelFormat      = NULL;
static SWAPBUFFERS       glSwapBuffers         = NULL;
static DESCRIBEPIXELFMT  glDescribePixelFormat = NULL;
static GETPIXELFMT       glGetPixelFormat      = NULL;

/*
		OpenGL Handle.
*/
HINSTANCE                hOpenGL               = NULL;

static BOOL OpenGLInitFunction(PCSTR name,
                               FARPROC *funcptr)
{
    PVOID func;

    func = (PVOID)GetProcAddress(hOpenGL, name);
    if (func != NULL)
    {
        (void)InterlockedCompareExchangePointer((PVOID*)funcptr,
                                                func,
                                                NULL);
        return TRUE;
    }

    return FALSE;
}

static BOOL OpenGLEnable(void)
{
    HMODULE hModOpengl32;
    BOOL Ret = TRUE;

    hModOpengl32 = LoadLibraryW(L"OPENGL32.DLL");
    if (hModOpengl32 == NULL)
        return FALSE;

    if (InterlockedCompareExchangePointer((PVOID*)&hOpenGL,
                                          (PVOID)hModOpengl32,
                                          NULL) != NULL)
    {
        FreeLibrary(hModOpengl32);

        /* NOTE: Even though another thread was faster loading the
                 library we can't just bail out here. We really need
                 to *try* to locate every function. This is slow but
                 thread-safe */
    }


    if (!OpenGLInitFunction("wglChoosePixelFormat", &glChoosePixelFormat))
        Ret = FALSE;

    if (!OpenGLInitFunction("wglSetPixelFormat", &glSetPixelFormat))
        Ret = FALSE;

    if (!OpenGLInitFunction("wglSwapBuffers", &glSwapBuffers))
        Ret = FALSE;

    if (!OpenGLInitFunction("wglDescribePixelFormat", &glDescribePixelFormat))
        Ret = FALSE;

    if (!OpenGLInitFunction("wglGetPixelFormat", &glGetPixelFormat))
        Ret = FALSE;

    return Ret;
}



/*
 * @implemented
 */
INT
STDCALL
ChoosePixelFormat(HDC  hdc,
                  CONST PIXELFORMATDESCRIPTOR * ppfd)
{
  if (glChoosePixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glChoosePixelFormat(hdc, ppfd));
}



/*
 * @implemented
 */
INT
STDCALL
DescribePixelFormat(HDC  hdc,
                    INT  iPixelFormat,
                    UINT  nBytes,
                    LPPIXELFORMATDESCRIPTOR  ppfd)
{
  if (glDescribePixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glDescribePixelFormat(hdc, iPixelFormat, nBytes, ppfd));
}



/*
 * @implemented
 */
INT
STDCALL
GetPixelFormat(HDC  hdc)
{
  if (glGetPixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glGetPixelFormat(hdc));
}



/*
 * @implemented
 */
BOOL
STDCALL
SetPixelFormat(HDC  hdc,
               INT  iPixelFormat,
               CONST PIXELFORMATDESCRIPTOR * ppfd)
{
  if (glSetPixelFormat == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);

  return(glSetPixelFormat(hdc, iPixelFormat, ppfd));
}



/*
 * @implemented
 */
BOOL
STDCALL
SwapBuffers(HDC  hdc)
{
  if (glSwapBuffers == NULL)
    if (OpenGLEnable() == FALSE)
      return(0);


  return(glSwapBuffers(hdc));
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
	HENHMETAFILE			hemf,
	UINT				cbBuffer,
	PIXELFORMATDESCRIPTOR	*ppfd
	)
{
	ENHMETAHEADER pemh;

	if(GetEnhMetaFileHeader(hemf, sizeof(ENHMETAHEADER), &pemh))
	{
	if(pemh.bOpenGL)
	{
		if(pemh.cbPixelFormat)
		{
		memcpy((void*)ppfd, UlongToPtr(pemh.offPixelFormat), cbBuffer );
		return(pemh.cbPixelFormat);
		}
	}
	}
	return(0);
}

/* EOF */
