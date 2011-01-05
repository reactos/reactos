/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/ogldrv.c
 * PURPOSE:         OpenGL driver for ReactOS/Windows
 * PROGRAMMERS:     Kamil Hornicek
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosogldrv);

typedef INT  (WINAPI *CHOOSEPIXELFMT) (HDC, CONST PIXELFORMATDESCRIPTOR *);
typedef INT  (WINAPI *DESCRIBEPIXELFMT) (HDC, INT, UINT, PIXELFORMATDESCRIPTOR *);
typedef INT  (WINAPI *GETPIXELFMT) (HDC);
typedef BOOL (WINAPI *SETPIXELFMT) (HDC, INT, CONST PIXELFORMATDESCRIPTOR *);
typedef BOOL (WINAPI *SWAPBUFFERS) (HDC);

static CHOOSEPIXELFMT   glChoosePixelFormat   = NULL;
static DESCRIBEPIXELFMT glDescribePixelFormat = NULL;
static GETPIXELFMT      glGetPixelFormat      = NULL;
static SETPIXELFMT      glSetPixelFormat      = NULL;
static SWAPBUFFERS      glSwapBuffers         = NULL;

/* FUNCTIONS **************************************************************/

BOOL InitOGL(VOID)
{
    HMODULE hOGL;

    hOGL = LoadLibraryW(L"OPENGL32.DLL");

    if (!hOGL)
        return FALSE;

    glChoosePixelFormat = GetProcAddress(hOGL, "wglChoosePixelFormat");
    glDescribePixelFormat = GetProcAddress(hOGL, "wglDescribePixelFormat");
    glGetPixelFormat = GetProcAddress(hOGL, "wglGetPixelFormat");
    glSetPixelFormat = GetProcAddress(hOGL, "wglSetPixelFormat");
    glSwapBuffers = GetProcAddress(hOGL, "wglSwapBuffers");

    if (!glChoosePixelFormat || !glDescribePixelFormat || !glGetPixelFormat ||
        !glSetPixelFormat || !glSwapBuffers)
    {
        FreeLibrary(hOGL);
        ERR("Failed to load required wgl* functions from opengl32\n");
        return FALSE;
    }

    return TRUE;
}

INT CDECL RosDrv_ChoosePixelFormat(NTDRV_PDEVICE *physDev,
                                   CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    if (!glChoosePixelFormat)
        if (!InitOGL())
            return 0;

    return glChoosePixelFormat(physDev->hUserDC, ppfd);
}

INT CDECL RosDrv_GetPixelFormat(NTDRV_PDEVICE *physDev)
{
    if (!glGetPixelFormat)
        if (!InitOGL())
            return 0;

    return glGetPixelFormat(physDev->hUserDC);
}

INT CDECL RosDrv_DescribePixelFormat(NTDRV_PDEVICE *physDev,
                                     INT iPixelFormat,
                                     UINT nBytes,
                                     PIXELFORMATDESCRIPTOR *ppfd)
{
    if (!glDescribePixelFormat)
        if (!InitOGL())
            return 0;

    return glDescribePixelFormat(physDev->hUserDC, iPixelFormat, nBytes, ppfd);
}

BOOL CDECL RosDrv_SetPixelFormat(NTDRV_PDEVICE *physDev,
                                 INT iPixelFormat,
                                 CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    if (!glSetPixelFormat)
        if (!InitOGL())
            return 0;

    return glSetPixelFormat(physDev->hUserDC, iPixelFormat, ppfd);
}


BOOL CDECL RosDrv_SwapBuffers(NTDRV_PDEVICE *physDev)
{
    if (!glSwapBuffers)
        if (!InitOGL())
            return 0;

    return glSwapBuffers(physDev->hUserDC);
}

/* EOF */
