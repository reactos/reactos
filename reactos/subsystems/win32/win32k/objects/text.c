/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/text.c
 * PURPOSE:         Text
 * PROGRAMMER:      
 */
      
/** Includes ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/** Functions ******************************************************************/

BOOL
STDCALL
NtGdiSetTextJustification(HDC  hDC,
                          int  BreakExtra,
                          int  BreakCount)
{
  PDC pDc;
  PDC_ATTR pDc_Attr;

  pDc = DC_LockDc(hDC);
  if (!pDc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  pDc_Attr = pDc->pDc_Attr;
  if(!pDc_Attr) pDc_Attr = &pDc->Dc_Attr;

  pDc_Attr->lBreakExtra = BreakExtra;
  pDc_Attr->cBreak = BreakCount;

  DC_UnlockDc(pDc);
  return TRUE;
}

/* EOF */
