#include "precomp.h"
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
