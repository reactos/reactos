/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/font.c
 * PURPOSE:         Font
 * PROGRAMMER:
 */
      
/** Includes ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

ULONG
FASTCALL
ftGdiGetGlyphOutline(
    IN PDC pdc,
    IN WCHAR wch,
    IN UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID UnsafeBuf,
    IN LPMAT2 pmat2,
    IN BOOL bIgnoreRotation);

/** Functions ******************************************************************/

ULONG
APIENTRY
NtGdiGetGlyphOutline(
    IN HDC hdc,
    IN WCHAR wch,
    IN UINT iFormat,
    OUT LPGLYPHMETRICS pgm,
    IN ULONG cjBuf,
    OUT OPTIONAL PVOID UnsafeBuf,
    IN LPMAT2 pmat2,
    IN BOOL bIgnoreRotation)
{
  ULONG Ret;
  PDC dc;
  dc = DC_LockDc(hdc);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }
  Ret = ftGdiGetGlyphOutline( dc,
                             wch,
                         iFormat,
                             pgm,
                           cjBuf,
                       UnsafeBuf,
                           pmat2,
                 bIgnoreRotation);
  DC_UnlockDc(dc);
  return Ret;
}

/* EOF */



