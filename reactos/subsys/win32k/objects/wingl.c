
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/wingl.h>

// #define NDEBUG
#include <internal/debug.h>

INT
STDCALL
W32kChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}


INT
STDCALL
W32kDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             LPPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kGetPixelFormat(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSwapBuffers(HDC  hDC)
{
  UNIMPLEMENTED;
}


