#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>


/*
 * @implemented
 */
HPEN
STDCALL
CreatePen(INT fnPenStyle, INT nWidth, COLORREF crColor)
{
  return NtGdiCreatePen(fnPenStyle, nWidth, crColor);
}


/*
 * @implemented
 */
HPEN
STDCALL
CreatePenIndirect(CONST LOGPEN *lplgpn)
{
  return NtGdiCreatePenIndirect((CONST PLOGPEN)lplgpn);
}
