
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/wingl.h>

// #define NDEBUG
#include <internal/debug.h>

INT  W32kChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}


INT  W32kDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             LPPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

UINT  W32kGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

INT  W32kGetPixelFormat(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

BOOL  W32kSwapBuffers(HDC  hDC)
{
  UNIMPLEMENTED;
}


