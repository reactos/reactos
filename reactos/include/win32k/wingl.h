
#ifndef __WIN32K_WINGL_H
#define __WIN32K_WINGL_H

INT
STDCALL
W32kChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd);

INT
STDCALL
W32kDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             PPIXELFORMATDESCRIPTOR  pfd);

UINT
STDCALL
W32kGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd);

INT
STDCALL
W32kGetPixelFormat(HDC  hDC);

BOOL
STDCALL
W32kSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd);

BOOL
STDCALL
W32kSwapBuffers(HDC  hDC);

#endif

