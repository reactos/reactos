

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/print.h>

// #define NDEBUG
#include <internal/debug.h>

INT  W32kAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT  W32kEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT  W32kEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT  W32kEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData)
{
  UNIMPLEMENTED;
}

INT  W32kExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData)
{
  UNIMPLEMENTED;
}

INT  W32kSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc)
{
  UNIMPLEMENTED;
}

INT  W32kStartDoc(HDC  hDC,
                  CONST PDOCINFO  di)
{
  UNIMPLEMENTED;
}

INT  W32kStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

