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

/** Internal ******************************************************************/

INT
FASTCALL
FontGetObject(PTEXTOBJ TFont, INT Count, PVOID Buffer)
{
  if( Buffer == NULL ) return sizeof(LOGFONTW);

  switch (Count)
  {

     case sizeof(ENUMLOGFONTEXDVW):
        RtlCopyMemory( (LPENUMLOGFONTEXDVW) Buffer,
                                            &TFont->logfont,
                                            sizeof(ENUMLOGFONTEXDVW));
        break;
     case sizeof(ENUMLOGFONTEXW):
        RtlCopyMemory( (LPENUMLOGFONTEXW) Buffer,
                                          &TFont->logfont.elfEnumLogfontEx,
                                          sizeof(ENUMLOGFONTEXW));
        break;

     case sizeof(EXTLOGFONTW):
     case sizeof(ENUMLOGFONTW):
        RtlCopyMemory((LPENUMLOGFONTW) Buffer,
                                    &TFont->logfont.elfEnumLogfontEx.elfLogFont,
                                       sizeof(ENUMLOGFONTW));
        break;

     case sizeof(LOGFONTW):
        RtlCopyMemory((LPLOGFONTW) Buffer,
                                   &TFont->logfont.elfEnumLogfontEx.elfLogFont,
                                   sizeof(LOGFONTW));
        break;

     default:
        SetLastWin32Error(ERROR_BUFFER_OVERFLOW);
        return 0;
  }
  return Count;
}

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

DWORD
STDCALL
NtGdiGetFontData(
   HDC hDC,
   DWORD Table,
   DWORD Offset,
   LPVOID Buffer,
   DWORD Size)
{
  PDC Dc;
  PDC_ATTR Dc_Attr;
  HFONT hFont;
  PTEXTOBJ TextObj;
  PFONTGDI FontGdi;
  DWORD Result = GDI_ERROR;
  NTSTATUS Status = STATUS_SUCCESS;

  if (Buffer && Size)
  {
     _SEH_TRY
     {
         ProbeForRead(Buffer, Size, 1);
     }
     _SEH_HANDLE
     {
         Status = _SEH_GetExceptionCode();
     }
     _SEH_END
  }

  if (!NT_SUCCESS(Status)) return Result;

  Dc = DC_LockDc(hDC);
  if (Dc == NULL)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }
  Dc_Attr = Dc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &Dc->Dc_Attr;

  hFont = Dc_Attr->hlfntNew;
  TextObj = TEXTOBJ_LockText(hFont);
  DC_UnlockDc(Dc);

  if (TextObj == NULL)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }

  FontGdi = ObjToGDI(TextObj->Font, FONT);

  Result = ftGdiGetFontData(FontGdi, Table, Offset, Buffer, Size);

  TEXTOBJ_UnlockText(TextObj);

  return Result;
}

 /*
 * @implemented
 */
DWORD
APIENTRY
NtGdiGetFontUnicodeRanges(
    IN HDC hdc,
    OUT OPTIONAL LPGLYPHSET pgs)
{
  PDC pDc;
  PDC_ATTR Dc_Attr;
  HFONT hFont;
  PTEXTOBJ TextObj;
  PFONTGDI FontGdi;
  DWORD Size = 0;
  PGLYPHSET pgsSafe;
  NTSTATUS Status = STATUS_SUCCESS;

  pDc = DC_LockDc(hdc);
  if (!pDc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return 0;
  }

  Dc_Attr = pDc->pDc_Attr;
  if(!Dc_Attr) Dc_Attr = &pDc->Dc_Attr;

  hFont = Dc_Attr->hlfntNew;
  TextObj = TEXTOBJ_LockText(hFont);
        
  if ( TextObj == NULL)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     goto Exit;
  }
  FontGdi = ObjToGDI(TextObj->Font, FONT);


  Size = ftGetFontUnicodeRanges( FontGdi, NULL);
  if (Size && pgs)
  {
     pgsSafe = ExAllocatePoolWithTag(PagedPool, Size, TAG_GDITEXT);
     if (!pgsSafe)
     {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        Size = 0;
        goto Exit;
     }

     Size = ftGetFontUnicodeRanges( FontGdi, pgsSafe);

     if (Size)
     {     
        _SEH_TRY
        {
            ProbeForWrite(pgsSafe, Size, 1);
            RtlCopyMemory(pgs, pgsSafe, Size);
        }
        _SEH_HANDLE
        {
           Status = _SEH_GetExceptionCode();
        }
        _SEH_END

        if (!NT_SUCCESS(Status)) Size = 0;
     }
     ExFreePoolWithTag(pgsSafe, TAG_GDITEXT);
  }
Exit:
  TEXTOBJ_UnlockText(TextObj);
  DC_UnlockDc(pDc);
  return Size;
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
  ULONG Ret = GDI_ERROR;
  PDC dc;
  PVOID pvBuf = NULL;
  GLYPHMETRICS gm;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc(hdc);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }

  if (UnsafeBuf && cjBuf)
  {
     pvBuf = ExAllocatePoolWithTag(PagedPool, cjBuf, TAG_GDITEXT);
     if (!pvBuf)
     {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
     }
  }

  Ret = ftGdiGetGlyphOutline( dc,
                             wch,
                         iFormat,
                             pgm ? &gm : NULL,
                           cjBuf,
                           pvBuf,
                           pmat2,
                 bIgnoreRotation);

  if (pvBuf)
  {
     _SEH_TRY
     {
         ProbeForWrite(UnsafeBuf, cjBuf, 1);
         RtlCopyMemory(UnsafeBuf, pvBuf, cjBuf);
     }
     _SEH_HANDLE
     {
         Status = _SEH_GetExceptionCode();
     }
     _SEH_END

     ExFreePoolWithTag(pvBuf, TAG_GDITEXT);
  }

  if (pgm)
  {
     _SEH_TRY
     {
         ProbeForWrite(pgm, sizeof(GLYPHMETRICS), 1);
         RtlCopyMemory(pgm, &gm, sizeof(GLYPHMETRICS));
     }
     _SEH_HANDLE
     {
         Status = _SEH_GetExceptionCode();
     }
     _SEH_END
  }

  if (! NT_SUCCESS(Status))
  {
     SetLastWin32Error(ERROR_INVALID_PARAMETER);
     Ret = GDI_ERROR;
  }

Exit:
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

W32KAPI
BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    IN LPWSTR   pwszFiles,
    IN ULONG    cwc,
    IN ULONG    cFiles,
    IN UINT     cjIn,
    OUT LPDWORD pdwBytes,
    OUT LPVOID  pvBuf,
    IN DWORD    dwType)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwBytes;
    UNICODE_STRING SafeFileNames;
    BOOL bRet = FALSE;
    ULONG cbStringSize;

    union
    {
        LOGFONTW logfontw;
        WCHAR FullName[LF_FULLFACESIZE];
    } Buffer;

    /* FIXME: handle cFiles > 0 */

    /* Check for valid dwType values
       dwType == 4 seems to be handled by gdi32 only */
    if (dwType == 4 || dwType > 5)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Allocate a safe unicode string buffer */
    cbStringSize = cwc * sizeof(WCHAR);
    SafeFileNames.MaximumLength = SafeFileNames.Length = cbStringSize - sizeof(WCHAR);
    SafeFileNames.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                 cbStringSize,
                                                 TAG('R','T','S','U'));
    if (!SafeFileNames.Buffer)
    {
        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Check buffers and copy pwszFiles to safe unicode string */
    _SEH_TRY
    {
        ProbeForRead(pwszFiles, cbStringSize, 1);
        ProbeForWrite(pdwBytes, sizeof(DWORD), 1);
        ProbeForWrite(pvBuf, cjIn, 1);

        RtlCopyMemory(SafeFileNames.Buffer, pwszFiles, cbStringSize);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        /* Free the string buffer for the safe filename */
        ExFreePool(SafeFileNames.Buffer);
        return FALSE;
    }

    /* Do the actual call */
    bRet = IntGdiGetFontResourceInfo(&SafeFileNames, &Buffer, &dwBytes, dwType);

    /* Check if succeeded and the buffer is big enough */
    if (bRet && cjIn >= dwBytes)
    {
        /* Copy the data back to caller */
        _SEH_TRY
        {
            /* Buffers are already probed */
            RtlCopyMemory(pvBuf, &Buffer, dwBytes);
            *pdwBytes = dwBytes;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END

        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            bRet = FALSE;
        }
    }

    /* Free the string for the safe filenames */
    ExFreePool(SafeFileNames.Buffer);

    return bRet;
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

  TextObj->lft = cjElfw;
  TextObj->fl  = fl;
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
