/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Tests extensions exposed by the software implementation
 * PROGRAMMERS:     Jérôme Gardou
 */

#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>

#include "wine/test.h"

START_TEST(sw_extensions)
{
    BITMAPINFO biDst;
    HDC hdcDst = CreateCompatibleDC(0);
    HBITMAP bmpDst, bmpOld;
    INT nFormats, iPixelFormat, res, i;
    PIXELFORMATDESCRIPTOR pfd;
    const char* output;
    HGLRC Context;
    UINT *dstBuffer = NULL;

    memset(&biDst, 0, sizeof(BITMAPINFO));
    biDst.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    biDst.bmiHeader.biWidth = 4;
    biDst.bmiHeader.biHeight = -4;
    biDst.bmiHeader.biPlanes = 1;
    biDst.bmiHeader.biBitCount = 32;
    biDst.bmiHeader.biCompression = BI_RGB;

    bmpDst = CreateDIBSection(0, &biDst, DIB_RGB_COLORS, (void**)&dstBuffer, NULL, 0);

    bmpOld = SelectObject(hdcDst, bmpDst);

    /* Choose a pixel format */
    nFormats = DescribePixelFormat(hdcDst, 0, 0, NULL);
    for(i=1; i<=nFormats; i++)
    {
        memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
        DescribePixelFormat(hdcDst, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

        if((pfd.dwFlags & PFD_DRAW_TO_BITMAP) &&
           (pfd.dwFlags & PFD_SUPPORT_OPENGL) &&
           (pfd.cColorBits == 32) &&
           (pfd.cAlphaBits == 8) )
        {
            iPixelFormat = i;
            break;
        }
    }

    ok(pfd.dwFlags & PFD_GENERIC_FORMAT, "We found a pixel format for drawing to bitmap which is not generic !\n");
    ok (iPixelFormat >= 1 && iPixelFormat <= nFormats, "Could not find a suitable pixel format.\n");
    res = SetPixelFormat(hdcDst, iPixelFormat, &pfd);
    ok (res != 0, "SetPixelFormat failed.\n");
    Context = wglCreateContext(hdcDst);
    ok(Context != NULL, "We failed to create a GL context.\n");
    wglMakeCurrent(hdcDst, Context);

    /* Get the version */
    output = (const char*)glGetString(GL_VERSION);
    ok(strcmp(output, "1.1.0") == 0, "Expected version 1.1.0, got \"%s\".\n", output);

    /* Get the extensions list */
    output = (const char*)glGetString(GL_EXTENSIONS);
    trace("GL extensions are %s.\n", output);
    ok (strlen(output) == strlen("GL_WIN_swap_hint GL_EXT_bgra GL_EXT_paletted_texture"), "Wrong extension list : \"%s\".\n", output);
    ok(strstr(output, "GL_WIN_swap_hint") != NULL, "GL_WIN_swap_hint extension is not present.\n");
    ok(strstr(output, "GL_EXT_bgra") != NULL, "GL_EXT_bgra extension is not present.\n");
    ok(strstr(output, "GL_EXT_paletted_texture") != NULL, "GL_EXT_paletted_texture extension is not present.\n");

    /* cleanup */
    wglDeleteContext(Context);
    SelectObject(hdcDst, bmpOld);
    DeleteDC(hdcDst);
}
