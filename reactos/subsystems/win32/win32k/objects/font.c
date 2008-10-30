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

//
// FIXME PLEASE!!!!
// Why are these here? Well there is a problem with drivers/directx.
// 1st: It does not belong there.
// 2nd: Due to being placed outside Win32k build environment, it creates
//      compiling issues.
// Until svn mv drivers/directx subsystem/win32/win32k/drivers/directx,
// it will not get fixed.
//
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

INT
FASTCALL
IntGetOutlineTextMetrics(PFONTGDI FontGDI, UINT Size, OUTLINETEXTMETRICW *Otm);

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

DWORD
STDCALL
NtGdiGetKerningPairs(HDC  hDC,
                     ULONG  NumPairs,
                     LPKERNINGPAIR  krnpair)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 From "Undocumented Windows 2000 Secrets" Appendix B, Table B-2, page
 472, this is NtGdiGetOutlineTextMetricsInternalW.
 */
ULONG
STDCALL
NtGdiGetOutlineTextMetricsInternalW (HDC  hDC,
                                   ULONG  Data,
                      OUTLINETEXTMETRICW  *otm,
                                   TMDIFF *Tmd)
{
  PDC dc;
  PDC_ATTR Dc_Attr;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  HFONT hFont = 0;
  ULONG Size;
  OUTLINETEXTMETRICW *potm;
  NTSTATUS Status;

  dc = DC_LockDc(hDC);
  if (dc == NULL)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  Dc_Attr = dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  hFont = Dc_Attr->hlfntNew;
  TextObj = TEXTOBJ_LockText(hFont);
  DC_UnlockDc(dc);
  if (TextObj == NULL)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  FontGDI = ObjToGDI(TextObj->Font, FONT);
  TEXTOBJ_UnlockText(TextObj);
  Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL);
  if (!otm) return Size;
  if (Size > Data)
    {
      SetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }
  potm = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
  if (NULL == potm)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
    }
  IntGetOutlineTextMetrics(FontGDI, Size, potm);
  if (otm)
    {
      Status = MmCopyToCaller(otm, potm, Size);
      if (! NT_SUCCESS(Status))
        {
          SetLastWin32Error(ERROR_INVALID_PARAMETER);
          ExFreePool(potm);
          return 0;
        }
    }
  ExFreePool(potm);
  return Size;
}

/* EOF */
