#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

BOOL
STDCALL
LineTo(HDC hDC, int XEnd, int YEnd)
{
   return W32kLineTo(hDC, XEnd, YEnd);
}

BOOL  
STDCALL 
MoveToEx(HDC hDC, int X, int Y, LPPOINT Point)
{
   return W32kMoveToEx(hDC, X, Y, Point);
}


