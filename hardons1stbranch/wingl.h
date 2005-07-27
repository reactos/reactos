
#ifndef __WIN32K_WINGL_H
#define __WIN32K_WINGL_H

INT
STDCALL
NtGdiChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd);

INT
STDCALL
NtGdiDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             PPIXELFORMATDESCRIPTOR  pfd);

UINT
STDCALL
NtGdiGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd);

INT
STDCALL
NtGdiGetPixelFormat(HDC  hDC);

BOOL
STDCALL
NtGdiSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd);

BOOL
STDCALL
NtGdiSwapBuffers(HDC  hDC);

#endif

