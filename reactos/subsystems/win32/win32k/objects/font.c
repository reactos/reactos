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

/** Functions ******************************************************************/

INT
APIENTRY
NtGdiAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv)
{
  UNICODE_STRING SafeFileName;
  PWSTR src;
  NTSTATUS Status;
  int Ret;

  /* FIXME - Protect with SEH? */
  RtlInitUnicodeString(&SafeFileName, pwszFiles);

  /* Reserve for prepending '\??\' */
  SafeFileName.Length += 4 * sizeof(WCHAR);
  SafeFileName.MaximumLength += 4 * sizeof(WCHAR);

  src = SafeFileName.Buffer;
  SafeFileName.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, SafeFileName.MaximumLength, TAG_STRING);
  if(!SafeFileName.Buffer)
  {
    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  /* Prepend '\??\' */
  RtlCopyMemory(SafeFileName.Buffer, L"\\??\\", 4 * sizeof(WCHAR));

  Status = MmCopyFromCaller(SafeFileName.Buffer + 4, src, SafeFileName.MaximumLength - (4 * sizeof(WCHAR)));
  if(!NT_SUCCESS(Status))
  {
    ExFreePool(SafeFileName.Buffer);
    SetLastNtError(Status);
    return 0;
  }

  Ret = IntGdiAddFontResource(&SafeFileName, (DWORD)fl);

  ExFreePool(SafeFileName.Buffer);
  return Ret;
}

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

HFONT
STDCALL
NtGdiHfontCreate(
  IN PENUMLOGFONTEXDVW pelfw,
  IN ULONG cjElfw,
  IN LFTYPE lft,
  IN FLONG  fl,
  IN PVOID pvCliData )
{
  ENUMLOGFONTEXDVW SafeLogfont;
  HFONT hNewFont;
  PTEXTOBJ TextObj;
  NTSTATUS Status = STATUS_SUCCESS;

  if (!pelfw)
  {
      return NULL;
  }

  _SEH_TRY
  {
      ProbeForRead(pelfw, sizeof(ENUMLOGFONTEXDVW), 1);
      RtlCopyMemory(&SafeLogfont, pelfw, sizeof(ENUMLOGFONTEXDVW));
  }
  _SEH_HANDLE
  {
      Status = _SEH_GetExceptionCode();
  }
  _SEH_END

  if (!NT_SUCCESS(Status))
  {
      return NULL;
  }

  TextObj = TEXTOBJ_AllocTextWithHandle();
  if (!TextObj)
  {
      return NULL;
  }
  hNewFont = TextObj->BaseObject.hHmgr;

  RtlCopyMemory (&TextObj->logfont, &SafeLogfont, sizeof(ENUMLOGFONTEXDVW));

  if (SafeLogfont.elfEnumLogfontEx.elfLogFont.lfEscapement !=
      SafeLogfont.elfEnumLogfontEx.elfLogFont.lfOrientation)
  {
    /* this should really depend on whether GM_ADVANCED is set */
    TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation =
    TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfEscapement;
  }
  TEXTOBJ_UnlockText(TextObj);

  return hNewFont;
}


/* EOF */
