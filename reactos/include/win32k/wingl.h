
#ifndef __WIN32K_WINGL_H
#define __WIN32K_WINGL_H

INT  W32kChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd);

INT  W32kDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             PPIXELFORMATDESCRIPTOR  pfd);

UINT  W32kGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd);

INT  W32kGetPixelFormat(HDC  hDC);

BOOL  W32kSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd);

BOOL  W32kSwapBuffers(HDC  hDC);

#endif

