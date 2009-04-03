
#include <w32k.h>

#define NDEBUG
#include <debug.h>


BOOL
APIENTRY
STROBJ_bEnum(
	IN STROBJ  *pstro,
	OUT ULONG  *pc,
	OUT PGLYPHPOS  *ppgpos
	)
{
  // www.osr.com/ddk/graphics/gdifncs_65uv.htm
  UNIMPLEMENTED;
  return FALSE;
}

DWORD
APIENTRY
STROBJ_dwGetCodePage ( IN STROBJ *pstro )
{
  // www.osr.com/ddk/graphics/gdifncs_9jmv.htm
  PSTRGDI pStrGdi = (PSTRGDI) pstro;
  return pStrGdi->dwCodePage;
}

VOID
APIENTRY
STROBJ_vEnumStart ( IN STROBJ *pstro )
{
  // www.osr.com/ddk/graphics/gdifncs_32uf.htm
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
STROBJ_bEnumPositionsOnly(
   IN STROBJ *StringObj,
   OUT ULONG *Count,
   OUT PGLYPHPOS *Pos)
{
   UNIMPLEMENTED;
   return (BOOL) DDI_ERROR;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
STROBJ_bGetAdvanceWidths(
   IN STROBJ *StringObj,
   IN ULONG First,
   IN ULONG Count,
   OUT POINTQF *Widths)
{
   UNIMPLEMENTED;
   return FALSE;
}

/*
 * @implemented
 */
FIX APIENTRY
STROBJ_fxBreakExtra(
   IN STROBJ *StringObj)
{
  PSTRGDI pStrGdi = (PSTRGDI) StringObj;
  if (pStrGdi->StrObj.flAccel & SO_BREAK_EXTRA) return pStrGdi->fxBreakExtra;
  return (FIX) 0;
}

/*
 * @implemented
 */
FIX APIENTRY
STROBJ_fxCharacterExtra(
   IN STROBJ *StringObj)
{
  PSTRGDI pStrGdi = (PSTRGDI) StringObj;
  if (pStrGdi->StrObj.flAccel & SO_CHARACTER_EXTRA) return pStrGdi->fxExtra;
  return (FIX) 0;
}

/* EOF */
