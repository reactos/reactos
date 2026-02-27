/*
 * PROJECT:     ReactOS win32 kernel mode subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Font
 * COPYRIGHT:   Copyright (C) James Tabor <james.tabor@reactos.org>
 *              Copyright (C) Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright (C) Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/** Includes ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define MAX_TEXT_BUFFER 0x2710000

HFONT APIENTRY
HfontCreate(
    _In_ const ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG fl,
    _In_opt_ PVOID pvCliData);

/** Internal ******************************************************************/

HFONT FASTCALL
GreCreateFontIndirectW(_In_ const LOGFONTW *lplf)
{
    if (lplf)
    {
        ENUMLOGFONTEXDVW Logfont;

        RtlCopyMemory( &Logfont.elfEnumLogfontEx.elfLogFont, lplf, sizeof(LOGFONTW));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfFullName,
                       sizeof(Logfont.elfEnumLogfontEx.elfFullName));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfStyle,
                       sizeof(Logfont.elfEnumLogfontEx.elfStyle));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfScript,
                       sizeof(Logfont.elfEnumLogfontEx.elfScript));

        Logfont.elfDesignVector.dvNumAxes = 0;

        RtlZeroMemory( &Logfont.elfDesignVector, sizeof(DESIGNVECTOR));

        return HfontCreate(&Logfont, 0, 0, 0, NULL);
    }
    else return NULL;
}

DWORD
FASTCALL
GreGetKerningPairs(
    _In_ HDC hDC,
    _In_ ULONG NumPairs,
    _Out_writes_(NumPairs) LPKERNINGPAIR krnpair)
{
  PDC dc;
  PDC_ATTR pdcattr;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  DWORD Count;
  KERNINGPAIR *pKP;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  pdcattr = dc->pdcattr;
  TextObj = RealizeFontInit(pdcattr->hlfntNew);
  DC_UnlockDc(dc);

  if (!TextObj)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  FontGDI = ObjToGDI(TextObj->Font, FONT);
  TEXTOBJ_UnlockText(TextObj);

  Count = ftGdiGetKerningPairs(FontGDI,0,NULL);

  if ( Count && krnpair )
  {
     if (Count > NumPairs)
     {
        EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
     }
     pKP = ExAllocatePoolWithTag(PagedPool, Count * sizeof(KERNINGPAIR), GDITAG_TEXT);
     if (!pKP)
     {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
     }
     ftGdiGetKerningPairs(FontGDI,Count,pKP);

     RtlCopyMemory(krnpair, pKP, Count * sizeof(KERNINGPAIR));

     ExFreePoolWithTag(pKP,GDITAG_TEXT);
  }
  return Count;
}

/*
  It is recommended that an application use the GetFontLanguageInfo function
  to determine whether the GCP_DIACRITIC, GCP_DBCS, GCP_USEKERNING, GCP_LIGATE,
  GCP_REORDER, GCP_GLYPHSHAPE, and GCP_KASHIDA values are valid for the
  currently selected font. If not valid, GetCharacterPlacement ignores the
  value.

  MS must use a preset "compiled in" support for each language based releases.
  ReactOS uses FreeType, this will need to be supported. ATM this is hard coded
  for GCPCLASS_LATIN!
 */
#if 0
DWORD
FASTCALL
GreGetCharacterPlacementW(
    _In_ HDC hdc,
    _In_reads_(nCount) PCWCH pwsz,
    _In_ INT nCount,
    _In_ INT nMaxExtent,
    _Inout_opt_ LPGCP_RESULTSW pgcpw,
    _In_ DWORD dwFlags)
{
  GCP_RESULTSW gcpwSave;
  UINT i, nSet, cSet;
  INT *tmpDxCaretPos;
  LONG Cx;
  SIZE Size = {0,0};

  DPRINT1("GreGCPW Start\n");

   if (!pgcpw)
   {
      if (GreGetTextExtentW( hdc, pwsz, nCount, &Size, 1))
         return MAKELONG(Size.cx, Size.cy);
      return 0;
   }

  DPRINT1("GreGCPW 1\n");

  RtlCopyMemory(&gcpwSave, pgcpw, sizeof(GCP_RESULTSW));

  cSet = nSet = nCount;

  if ( nCount > gcpwSave.nGlyphs ) cSet = gcpwSave.nGlyphs;

  /* GCP_JUSTIFY may only be used in conjunction with GCP_MAXEXTENT. */
  if ( dwFlags & GCP_JUSTIFY) dwFlags |= GCP_MAXEXTENT;

  if ( !gcpwSave.lpDx && gcpwSave.lpCaretPos )
     tmpDxCaretPos = gcpwSave.lpCaretPos;
  else
     tmpDxCaretPos = gcpwSave.lpDx;

  if ( !GreGetTextExtentExW( hdc,
                             pwsz,
                             cSet,
                             nMaxExtent,
                            ((dwFlags & GCP_MAXEXTENT) ? (PULONG) &cSet : NULL),
                            (PULONG) tmpDxCaretPos,
                             &Size,
                             0) )
  {
     return 0;
  }

  DPRINT1("GreGCPW 2\n");

  nSet = cSet;

  if ( tmpDxCaretPos && nSet > 0)
  {
      for (i = (nSet - 1); i > 0; i--)
      {
          tmpDxCaretPos[i] -= tmpDxCaretPos[i - 1];
      }
  }

  if ( !(dwFlags & GCP_MAXEXTENT) || nSet )
  {
     if ( (dwFlags & GCP_USEKERNING) &&
           ( gcpwSave.lpDx ||
             gcpwSave.lpCaretPos ) &&
           nSet >= 2 )
     {
        DWORD Count;
        LPKERNINGPAIR pKP;

        Count = GreGetKerningPairs( hdc, 0, NULL);
        if (Count)
        {
           pKP = ExAllocatePoolWithTag(PagedPool, Count * sizeof(KERNINGPAIR), GDITAG_TEXT);
           if (pKP)
           {
              if ( GreGetKerningPairs( hdc, Count, pKP) != Count)
              {
                 ExFreePoolWithTag( pKP, GDITAG_TEXT);
                 return 0;
              }

              if ( (ULONG_PTR)(pKP) < ((ULONG_PTR)(pKP) + (ULONG_PTR)(Count * sizeof(KERNINGPAIR))) )
              {
                 DPRINT1("We Need to Do Something HERE!\n");
              }

              ExFreePoolWithTag( pKP, GDITAG_TEXT);

              if ( dwFlags & GCP_MAXEXTENT )
              {
                 if ( Size.cx > nMaxExtent )
                 {
                    for (Cx = Size.cx; nSet > 0; nSet--)
                    {
                        Cx -= tmpDxCaretPos[nSet - 1];
                        Size.cx = Cx;
                        if ( Cx <= nMaxExtent ) break;
                    }
                 }
                 if ( !nSet )
                 {
                    pgcpw->nGlyphs = 0;
                    pgcpw->nMaxFit = 0;
                    return 0;
                 }
              }
           }
        }
     }

     if ( (dwFlags & GCP_JUSTIFY) &&
           ( gcpwSave.lpDx ||
             gcpwSave.lpCaretPos ) &&
           nSet )
     {
         DPRINT1("We Need to Do Something HERE 2!\n");
     }

     if ( gcpwSave.lpDx && gcpwSave.lpCaretPos )
        RtlCopyMemory( gcpwSave.lpCaretPos, gcpwSave.lpDx, nSet * sizeof(LONG));

     if ( gcpwSave.lpCaretPos )
     {
        int pos = 0;
        i = 0;
        if ( nSet > 0 )
        {
           do
           {
              Cx = gcpwSave.lpCaretPos[i];
              gcpwSave.lpCaretPos[i] = pos;
              pos += Cx;
              ++i;
           }
           while ( i < nSet );
        }
     }

     if ( gcpwSave.lpOutString )
        RtlCopyMemory(gcpwSave.lpOutString, pwsz,  nSet * sizeof(WCHAR));

     if ( gcpwSave.lpClass )
        RtlFillMemory(gcpwSave.lpClass, nSet, GCPCLASS_LATIN);

     if ( gcpwSave.lpOrder )
     {
        for (i = 0; i < nSet; i++)
           gcpwSave.lpOrder[i] = i;
     }

     if ( gcpwSave.lpGlyphs )
     {
        if ( GreGetGlyphIndicesW( hdc, pwsz, nSet, gcpwSave.lpGlyphs, 0, 0) == GDI_ERROR )
        {
           nSet = 0;
           Size.cx = 0;
           Size.cy = 0;
        }
     }
     pgcpw->nGlyphs = nSet;
     pgcpw->nMaxFit = nSet;
  }
  DPRINT1("GreGCPW Exit\n");
  return MAKELONG(Size.cx, Size.cy);
}
#endif

ULONG
FASTCALL
FontGetObject(
    _Inout_ PTEXTOBJ plfont,
    _In_ ULONG cjBuffer,
    _Out_ PVOID pvBuffer)
{
    ULONG cjMaxSize;
    ENUMLOGFONTEXDVW *plf;

    ASSERT(plfont);
    plf = &plfont->logfont;

    if (!(plfont->fl & TEXTOBJECT_INIT))
    {
        NTSTATUS Status;
        DPRINT("FontGetObject font not initialized!\n");

        Status = TextIntRealizeFont(plfont->BaseObject.hHmgr, plfont);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("FontGetObject(TextIntRealizeFont) Status = 0x%lx\n", Status);
        }
    }

    /* If buffer is NULL, only the size is requested */
    if (pvBuffer == NULL) return sizeof(LOGFONTW);

    /* Calculate the maximum size according to number of axes */
    cjMaxSize = FIELD_OFFSET(ENUMLOGFONTEXDVW,
                    elfDesignVector.dvValues[plf->elfDesignVector.dvNumAxes]);

    if (cjBuffer > cjMaxSize) cjBuffer = cjMaxSize;

    RtlCopyMemory(pvBuffer, plf, cjBuffer);

    return cjBuffer;
}

DWORD
FASTCALL
IntGetCharDimensions(
    _In_ HDC hdc,
    _Out_opt_ PTEXTMETRICW ptm,
    _Out_opt_ PDWORD height)
{
  PDC pdc;
  PDC_ATTR pdcattr;
  PTEXTOBJ TextObj;
  SIZE sz;
  TMW_INTERNAL tmwi;
  BOOL Good;

  static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

  if(!ftGdiGetTextMetricsW(hdc, &tmwi)) return 0;

  pdc = DC_LockDc(hdc);

  if (!pdc) return 0;

  pdcattr = pdc->pdcattr;

  TextObj = RealizeFontInit(pdcattr->hlfntNew);
  if ( !TextObj )
  {
     DC_UnlockDc(pdc);
     return 0;
  }
  Good = TextIntGetTextExtentPoint(pdc, TextObj, alphabet, 52, 0, NULL, NULL, &sz, 0);
  TEXTOBJ_UnlockText(TextObj);
  DC_UnlockDc(pdc);

  if (!Good) return 0;
  if (ptm) *ptm = tmwi.TextMetric;
  if (height) *height = tmwi.TextMetric.tmHeight;

  return (sz.cx / 26 + 1) / 2;
}


DWORD
FASTCALL
IntGetFontLanguageInfo(_In_ PDC Dc)
{
  PDC_ATTR pdcattr;
  FONTSIGNATURE fontsig;
  static const DWORD GCP_DBCS_MASK=0x003F0000,
		GCP_DIACRITIC_MASK=0x00000000,
		FLI_GLYPHS_MASK=0x00000000,
		GCP_GLYPHSHAPE_MASK=0x00000040,
		GCP_KASHIDA_MASK=0x00000000,
		GCP_LIGATE_MASK=0x00000000,
		GCP_USEKERNING_MASK=0x00000000,
		GCP_REORDER_MASK=0x00000060;

  DWORD result=0;

  ftGdiGetTextCharsetInfo( Dc, &fontsig, 0 );

 /* We detect each flag we return using a bitmask on the Codepage Bitfields */
  if( (fontsig.fsCsb[0]&GCP_DBCS_MASK)!=0 )
		result|=GCP_DBCS;

  if( (fontsig.fsCsb[0]&GCP_DIACRITIC_MASK)!=0 )
		result|=GCP_DIACRITIC;

  if( (fontsig.fsCsb[0]&FLI_GLYPHS_MASK)!=0 )
		result|=FLI_GLYPHS;

  if( (fontsig.fsCsb[0]&GCP_GLYPHSHAPE_MASK)!=0 )
		result|=GCP_GLYPHSHAPE;

  if( (fontsig.fsCsb[0]&GCP_KASHIDA_MASK)!=0 )
		result|=GCP_KASHIDA;

  if( (fontsig.fsCsb[0]&GCP_LIGATE_MASK)!=0 )
		result|=GCP_LIGATE;

  if( (fontsig.fsCsb[0]&GCP_USEKERNING_MASK)!=0 )
		result|=GCP_USEKERNING;

  pdcattr = Dc->pdcattr;

  /* This might need a test for a HEBREW- or ARABIC_CHARSET as well */
  if ( pdcattr->flTextAlign & TA_RTLREADING )
     if( (fontsig.fsCsb[0]&GCP_REORDER_MASK)!=0 )
                    result|=GCP_REORDER;

  return result;
}

PTEXTOBJ
FASTCALL
RealizeFontInit(_In_ HFONT hFont)
{
  NTSTATUS Status = STATUS_SUCCESS;
  PTEXTOBJ pTextObj;

  pTextObj = TEXTOBJ_LockText(hFont);

  if ( pTextObj && !(pTextObj->fl & TEXTOBJECT_INIT))
  {
     Status = TextIntRealizeFont(hFont, pTextObj);
     if (!NT_SUCCESS(Status))
     {
        TEXTOBJ_UnlockText(pTextObj);
        return NULL;
     }
  }
  return pTextObj;
}

static BOOL
IntCheckFontPathNames(
    _In_reads_(cwc) PCWCH pwcFiles,
    _In_ ULONG cFiles,
    _In_ ULONG cwc)
{
    ULONG ich, cRealFiles;

    if (pwcFiles[cwc - 1] != UNICODE_NULL)
        return FALSE;

    for (ich = cRealFiles = 0; ich < cwc; ++ich)
    {
        if (!pwcFiles[ich])
            ++cRealFiles;
    }

    return cRealFiles >= cFiles;
}

/** Functions ******************************************************************/

INT
APIENTRY
NtGdiAddFontResourceW(
    _In_reads_(cwc) PCWCH pwcFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ FLONG fl,
    _In_ DWORD dwPidTid,
    _In_opt_ const DESIGNVECTOR *pdv)
{
    UNICODE_STRING SafeFileName;
    INT Ret;

    DBG_UNREFERENCED_PARAMETER(dwPidTid);
    DBG_UNREFERENCED_PARAMETER(pdv);

    DPRINT("NtGdiAddFontResourceW\n");

    /* cwc = Length + trailing zero. */
    if ((cwc <= 1) || (cwc > UNICODE_STRING_MAX_CHARS))
        return 0;

    SafeFileName.Length = (USHORT)((cwc - 1) * sizeof(WCHAR));
    SafeFileName.MaximumLength = SafeFileName.Length + sizeof(UNICODE_NULL);
    SafeFileName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                SafeFileName.MaximumLength,
                                                TAG_STRING);
    if (!SafeFileName.Buffer)
        return 0;

    _SEH2_TRY
    {
        ProbeForRead(pwcFiles, cwc * sizeof(WCHAR), sizeof(WCHAR));

        if (!IntCheckFontPathNames(pwcFiles, cFiles, cwc))
            return 0;
 
        RtlCopyMemory(SafeFileName.Buffer, pwcFiles, SafeFileName.Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(SafeFileName.Buffer, TAG_STRING);
        _SEH2_YIELD(return 0);
    }
    _SEH2_END;

    SafeFileName.Buffer[SafeFileName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    Ret = IntGdiAddFontResourceEx(&SafeFileName, cFiles, fl, 0);

    ExFreePoolWithTag(SafeFileName.Buffer, TAG_STRING);
    return Ret;
}

BOOL
APIENTRY
NtGdiRemoveFontResourceW(
    _In_reads_(cwc) PCWCH pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ ULONG fl,
    _In_ DWORD dwPidTid,
    _In_opt_ const DESIGNVECTOR *pdv)
{
    UNICODE_STRING SafeFileName;
    BOOL Ret;

    DBG_UNREFERENCED_PARAMETER(dwPidTid);
    DBG_UNREFERENCED_PARAMETER(pdv);

    DPRINT("NtGdiRemoveFontResourceW\n");

    /* cwc = Length + trailing zero. */
    if ((cwc <= 1) || (cwc > UNICODE_STRING_MAX_CHARS))
        return FALSE;

    SafeFileName.Length = (USHORT)((cwc - 1) * sizeof(WCHAR));
    SafeFileName.MaximumLength = SafeFileName.Length + sizeof(UNICODE_NULL);
    SafeFileName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                SafeFileName.MaximumLength,
                                                TAG_STRING);
    if (!SafeFileName.Buffer)
        return FALSE;

    _SEH2_TRY
    {
        ProbeForRead(pwszFiles, cwc * sizeof(WCHAR), sizeof(WCHAR));

        if (!IntCheckFontPathNames(pwszFiles, cFiles, cwc))
            return FALSE;

        RtlCopyMemory(SafeFileName.Buffer, pwszFiles, SafeFileName.Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(SafeFileName.Buffer, TAG_STRING);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    SafeFileName.Buffer[SafeFileName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    Ret = IntGdiRemoveFontResource(&SafeFileName, cFiles, fl);

    ExFreePoolWithTag(SafeFileName.Buffer, TAG_STRING);
    return Ret;
}

HANDLE
APIENTRY
NtGdiAddFontMemResourceEx(
    _In_reads_bytes_(cjBuffer) const VOID *pvBuffer,
    _In_ DWORD cjBuffer,
    _In_reads_bytes_opt_(cjDV) const DESIGNVECTOR *pdv,
    _In_ ULONG cjDV,
    _Out_ PDWORD pNumFonts)
{
    _SEH2_VOLATILE PVOID Buffer = NULL;
    HANDLE Ret;
    DWORD NumFonts = 0;

    DPRINT("NtGdiAddFontMemResourceEx\n");
    DBG_UNREFERENCED_PARAMETER(pdv);
    DBG_UNREFERENCED_PARAMETER(cjDV);

    if (!pvBuffer || !cjBuffer)
        return NULL;

    _SEH2_TRY
    {
        ProbeForRead(pvBuffer, cjBuffer, sizeof(BYTE));
        Buffer = ExAllocatePoolWithQuotaTag(PagedPool, cjBuffer, TAG_FONT);
        RtlCopyMemory(Buffer, pvBuffer, cjBuffer);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if (Buffer != NULL)
        {
            ExFreePoolWithTag(Buffer, TAG_FONT);
        }
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;

    Ret = IntGdiAddFontMemResource(Buffer, cjBuffer, &NumFonts);
    ExFreePoolWithTag(Buffer, TAG_FONT);

    _SEH2_TRY
    {
        ProbeForWrite(pNumFonts, sizeof(NumFonts), 1);
        *pNumFonts = NumFonts;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Leak it? */
        _SEH2_YIELD(return NULL);
    }
    _SEH2_END;


    return Ret;
}


BOOL
APIENTRY
NtGdiRemoveFontMemResourceEx(
    _In_ HANDLE hMMFont)
{
    return IntGdiRemoveFontMemResource(hMMFont);
}


 /*
 * @unimplemented
 */
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    _In_ HDC hdc,
    _In_reads_(nCount) PCWCH pwsz,
    _In_ INT nCount,
    _In_ INT nMaxExtent,
    _Inout_opt_ LPGCP_RESULTSW pgcpw,
    _In_ DWORD dwFlags)
{
    UNIMPLEMENTED;
    return 0;
#if 0
    return GreGetCharacterPlacementW( hdc,
                                      pwsz,
                                      nCount,
                                      nMaxExtent,
                                      pgcpw,
                                      dwFlags);
#endif
}

DWORD
APIENTRY
NtGdiGetFontData(
    _In_ HDC hDC,
    _In_ DWORD Table,
    _In_ DWORD Offset,
    _Out_writes_bytes_(Size) PVOID Buffer,
    _In_ DWORD Size)
{
  PDC Dc;
  PDC_ATTR pdcattr;
  HFONT hFont;
  PTEXTOBJ TextObj;
  PFONTGDI FontGdi;
  DWORD Result = GDI_ERROR;
  NTSTATUS Status = STATUS_SUCCESS;

  if (Buffer && Size)
  {
     _SEH2_TRY
     {
         ProbeForRead(Buffer, Size, 1);
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
         Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END
  }

  if (!NT_SUCCESS(Status)) return Result;

  Dc = DC_LockDc(hDC);
  if (Dc == NULL)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }
  pdcattr = Dc->pdcattr;

  hFont = pdcattr->hlfntNew;
  TextObj = RealizeFontInit(hFont);
  DC_UnlockDc(Dc);

  if (TextObj == NULL)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }

  FontGdi = ObjToGDI(TextObj->Font, FONT);

  Result = ftGdiGetFontData(FontGdi, Table, Offset, Buffer, Size);

  TEXTOBJ_UnlockText(TextObj);

  return Result;
}

/* @implemented */
DWORD
APIENTRY
NtGdiGetFontUnicodeRanges(
    _In_ HDC hdc,
    _Out_opt_ LPGLYPHSET pgs)
{
  PDC pDc;
  PDC_ATTR pdcattr;
  HFONT hFont;
  PTEXTOBJ TextObj;
  PFONTGDI FontGdi;
  DWORD Size = 0;
  PGLYPHSET pgsSafe;
  NTSTATUS Status = STATUS_SUCCESS;

  pDc = DC_LockDc(hdc);
  if (!pDc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  pdcattr = pDc->pdcattr;

  hFont = pdcattr->hlfntNew;
  TextObj = RealizeFontInit(hFont);

  if ( TextObj == NULL)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     goto Exit;
  }
  FontGdi = ObjToGDI(TextObj->Font, FONT);

  Size = ftGetFontUnicodeRanges( FontGdi, NULL);

  if (Size && pgs)
  {
     pgsSafe = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
     if (!pgsSafe)
     {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Size = 0;
        goto Exit;
     }

     Size = ftGetFontUnicodeRanges( FontGdi, pgsSafe);

     if (Size)
     {
        _SEH2_TRY
        {
            ProbeForWrite(pgs, Size, 1);
            RtlCopyMemory(pgs, pgsSafe, Size);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
           Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if (!NT_SUCCESS(Status)) Size = 0;
     }
     ExFreePoolWithTag(pgsSafe, GDITAG_TEXT);
  }
Exit:
  TEXTOBJ_UnlockText(TextObj);
  DC_UnlockDc(pDc);
  return Size;
}

ULONG
APIENTRY
NtGdiGetGlyphOutline(
    _In_ HDC hdc,
    _In_ WCHAR wch,
    _In_ UINT iFormat,
    _Out_ LPGLYPHMETRICS pgm,
    _In_ ULONG cjBuf,
    _Out_writes_bytes_opt_(cjBuf) PVOID UnsafeBuf,
    _In_opt_ const MAT2 *pmat2,
    _In_ BOOL bIgnoreRotation)
{
  ULONG Ret = GDI_ERROR;
  PDC dc;
  PVOID pvBuf = NULL;
  GLYPHMETRICS gm;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc(hdc);
  if (!dc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return GDI_ERROR;
  }

  if (UnsafeBuf && cjBuf)
  {
     pvBuf = ExAllocatePoolZero(PagedPool, cjBuf, GDITAG_TEXT);
     if (!pvBuf)
     {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
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
     _SEH2_TRY
     {
         ProbeForWrite(UnsafeBuf, cjBuf, 1);
         RtlCopyMemory(UnsafeBuf, pvBuf, cjBuf);
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
         Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END

     ExFreePoolWithTag(pvBuf, GDITAG_TEXT);
  }

  if (pgm)
  {
     _SEH2_TRY
     {
         ProbeForWrite(pgm, sizeof(GLYPHMETRICS), 1);
         RtlCopyMemory(pgm, &gm, sizeof(GLYPHMETRICS));
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
         Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END
  }

  if (! NT_SUCCESS(Status))
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     Ret = GDI_ERROR;
  }

Exit:
  DC_UnlockDc(dc);
  return Ret;
}

DWORD
APIENTRY
NtGdiGetKerningPairs(
    _In_ HDC  hDC,
    _In_ ULONG NumPairs,
    _Out_writes_(NumPairs) LPKERNINGPAIR krnpair)
{
  PDC dc;
  PDC_ATTR pdcattr;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  DWORD Count;
  KERNINGPAIR *pKP;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  pdcattr = dc->pdcattr;
  TextObj = RealizeFontInit(pdcattr->hlfntNew);
  DC_UnlockDc(dc);

  if (!TextObj)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }

  FontGDI = ObjToGDI(TextObj->Font, FONT);
  TEXTOBJ_UnlockText(TextObj);

  Count = ftGdiGetKerningPairs(FontGDI,0,NULL);

  if ( Count && krnpair )
  {
     if (Count > NumPairs)
     {
        EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
     }
     pKP = ExAllocatePoolWithTag(PagedPool, Count * sizeof(KERNINGPAIR), GDITAG_TEXT);
     if (!pKP)
     {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
     }
     ftGdiGetKerningPairs(FontGDI,Count,pKP);
     _SEH2_TRY
     {
        ProbeForWrite(krnpair, Count * sizeof(KERNINGPAIR), 1);
        RtlCopyMemory(krnpair, pKP, Count * sizeof(KERNINGPAIR));
     }
     _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
     {
        Status = _SEH2_GetExceptionCode();
     }
     _SEH2_END
     if (!NT_SUCCESS(Status))
     {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        Count = 0;
     }
     ExFreePoolWithTag(pKP,GDITAG_TEXT);
  }
  return Count;
}

/*
 From "Undocumented Windows 2000 Secrets" Appendix B, Table B-2, page
 472, this is NtGdiGetOutlineTextMetricsInternalW.
 */
ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    _In_ HDC hDC,
    _In_ ULONG Data,
    _Out_opt_ POUTLINETEXTMETRICW otm,
    _In_ PTMDIFF Tmd)
{
  PDC dc;
  PDC_ATTR pdcattr;
  PTEXTOBJ TextObj;
  PFONTGDI FontGDI;
  HFONT hFont = 0;
  ULONG Size;
  OUTLINETEXTMETRICW *potm;
  NTSTATUS Status = STATUS_SUCCESS;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }
  pdcattr = dc->pdcattr;
  hFont = pdcattr->hlfntNew;
  TextObj = RealizeFontInit(hFont);
  DC_UnlockDc(dc);
  if (!TextObj)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }
  FontGDI = ObjToGDI(TextObj->Font, FONT);
  if (!(FontGDI->flType & FO_TYPE_TRUETYPE))
  {
     TEXTOBJ_UnlockText(TextObj);
     return 0;
  }
  TextIntUpdateSize(dc, TextObj, FontGDI, TRUE);
  TEXTOBJ_UnlockText(TextObj);
  Size = IntGetOutlineTextMetrics(FontGDI, 0, NULL, FALSE);
  if (!otm) return Size;
  if (Size > Data)
  {
      EngSetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
  }
  potm = ExAllocatePoolWithTag(PagedPool, Size, GDITAG_TEXT);
  if (!potm)
  {
      EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
  }
  RtlZeroMemory(potm, Size);
  IntGetOutlineTextMetrics(FontGDI, Size, potm, FALSE);

  _SEH2_TRY
  {
      ProbeForWrite(otm, Size, 1);
      RtlCopyMemory(otm, potm, Size);
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
      Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END

  if (!NT_SUCCESS(Status))
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     Size = 0;
  }

  ExFreePoolWithTag(potm,GDITAG_TEXT);
  return Size;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetFontResourceInfoInternalW(
    _In_reads_(cwc) PCWCH pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ UINT cjIn,
    _Inout_ PDWORD pdwBytes,
    _Out_writes_bytes_(*pdwBytes) PVOID pvBuf,
    _In_ DWORD dwType)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwBytes, dwBytesRequested;
    UNICODE_STRING SafeFileNames;
    BOOL bRet = FALSE;
    ULONG cbStringSize;
    LPVOID Buffer;

    /* FIXME: Handle cFiles > 0 */

    /* Check for valid dwType values */
    if (dwType > 5)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Allocate a safe unicode string buffer */
    cbStringSize = cwc * sizeof(WCHAR);
    SafeFileNames.MaximumLength = SafeFileNames.Length = (USHORT)cbStringSize - sizeof(WCHAR);
    SafeFileNames.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                 cbStringSize,
                                                 TAG_USTR);
    if (!SafeFileNames.Buffer)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    RtlZeroMemory(SafeFileNames.Buffer, SafeFileNames.MaximumLength);

    /* Check buffers and copy pwszFiles to safe unicode string */
    _SEH2_TRY
    {
        ProbeForRead(pwszFiles, cbStringSize, 1);
        ProbeForWrite(pdwBytes, sizeof(DWORD), 1);
        if (pvBuf)
            ProbeForWrite(pvBuf, cjIn, 1);

        dwBytes = *pdwBytes;
        dwBytesRequested = dwBytes;

        RtlCopyMemory(SafeFileNames.Buffer, pwszFiles, cbStringSize);
        if (dwBytes > 0)
        {
            Buffer = ExAllocatePoolWithTag(PagedPool, dwBytes, TAG_FINF);
        }
        else
        {
            Buffer = ExAllocatePoolWithTag(PagedPool, sizeof(DWORD), TAG_FINF);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        /* Free the string buffer for the safe filename */
        ExFreePoolWithTag(SafeFileNames.Buffer, TAG_USTR);
        return FALSE;
    }

    /* Do the actual call */
    bRet = IntGdiGetFontResourceInfo(&SafeFileNames,
                                     (pvBuf ? Buffer : NULL),
                                     &dwBytes, dwType);

    /* Check if succeeded */
    if (bRet)
    {
        /* Copy the data back to caller */
        _SEH2_TRY
        {
            /* Buffers are already probed */
            if (pvBuf && dwBytesRequested > 0)
                RtlCopyMemory(pvBuf, Buffer, min(dwBytesRequested, dwBytes));
            *pdwBytes = dwBytes;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            bRet = FALSE;
        }
    }

    ExFreePoolWithTag(Buffer, TAG_FINF);
    /* Free the string for the safe filenames */
    ExFreePoolWithTag(SafeFileNames.Buffer, TAG_USTR);

    return bRet;
}

/* @unimplemented */
BOOL
APIENTRY
NtGdiGetRealizationInfo(
    _In_ HDC hdc,
    _Out_ PREALIZATION_INFO pri,
    _In_ HFONT hf)
{
  PDC pDc;
  PTEXTOBJ pTextObj;
  PFONTGDI pFontGdi;
  PDC_ATTR pdcattr;
  BOOL Ret = FALSE;
  INT i = 0;
  REALIZATION_INFO ri;

  pDc = DC_LockDc(hdc);
  if (!pDc)
  {
     EngSetLastError(ERROR_INVALID_HANDLE);
     return 0;
  }
  pdcattr = pDc->pdcattr;
  pTextObj = RealizeFontInit(pdcattr->hlfntNew);
  ASSERT(pTextObj != NULL);
  pFontGdi = ObjToGDI(pTextObj->Font, FONT);
  TEXTOBJ_UnlockText(pTextObj);
  DC_UnlockDc(pDc);

  Ret = ftGdiRealizationInfo(pFontGdi, &ri);
  if (Ret)
  {
     if (pri)
     {
        NTSTATUS Status = STATUS_SUCCESS;
        _SEH2_TRY
        {
            ProbeForWrite(pri, sizeof(REALIZATION_INFO), 1);
            RtlCopyMemory(pri, &ri, sizeof(REALIZATION_INFO));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if(!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return FALSE;
        }
     }
     do
     {
        if (GdiHandleTable->cfPublic[i].hf == hf)
        {
           GdiHandleTable->cfPublic[i].iTechnology = ri.iTechnology;
           GdiHandleTable->cfPublic[i].iUniq = ri.iUniq;
           GdiHandleTable->cfPublic[i].dwUnknown = ri.dwUnknown;
           GdiHandleTable->cfPublic[i].dwCFCount = GdiHandleTable->dwCFCount;
           GdiHandleTable->cfPublic[i].fl |= CFONT_REALIZATION;
        }
        i++;
     }
     while ( i < GDI_CFONT_MAX );
  }
  return Ret;
}

HFONT
APIENTRY
HfontCreate(
    const ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG  fl,
    _In_opt_ PVOID pvCliData)
{
  HFONT hNewFont;
  PLFONT plfont;

  if (!pelfw)
  {
      return NULL;
  }

  plfont = LFONT_AllocFontWithHandle();
  if (!plfont)
  {
      return NULL;
  }
  hNewFont = plfont->BaseObject.hHmgr;

  plfont->lft = lft;
  plfont->fl  = fl;
  RtlCopyMemory (&plfont->logfont, pelfw, sizeof(ENUMLOGFONTEXDVW));
  ExInitializePushLock(&plfont->lock);

  if (pelfw->elfEnumLogfontEx.elfLogFont.lfEscapement !=
      pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation)
  {
    /* This should really depend on whether GM_ADVANCED is set */
    plfont->logfont.elfEnumLogfontEx.elfLogFont.lfOrientation =
    plfont->logfont.elfEnumLogfontEx.elfLogFont.lfEscapement;
  }
  LFONT_UnlockFont(plfont);

  if (pvCliData && hNewFont)
  {
    // FIXME: Use GDIOBJ_InsertUserData
    KeEnterCriticalRegion();
    {
       INT Index = GDI_HANDLE_GET_INDEX((HGDIOBJ)hNewFont);
       PGDI_TABLE_ENTRY Entry = &GdiHandleTable->Entries[Index];
       Entry->UserData = pvCliData;
    }
    KeLeaveCriticalRegion();
  }

  return hNewFont;
}


HFONT
APIENTRY
NtGdiHfontCreate(
    _In_reads_bytes_(cjElfw) const ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG  fl,
    _In_opt_ PVOID pvCliData)
{
  ENUMLOGFONTEXDVW SafeLogfont;
  NTSTATUS Status = STATUS_SUCCESS;

  /* Silence GCC warnings */
  SafeLogfont.elfEnumLogfontEx.elfLogFont.lfEscapement = 0;
  SafeLogfont.elfEnumLogfontEx.elfLogFont.lfOrientation = 0;

  if (!pelfw)
  {
      return NULL;
  }

  _SEH2_TRY
  {
      ProbeForRead(pelfw, sizeof(ENUMLOGFONTEXDVW), 1);
      RtlCopyMemory(&SafeLogfont, pelfw, sizeof(ENUMLOGFONTEXDVW));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
      Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END

  if (!NT_SUCCESS(Status))
  {
      return NULL;
  }

  return HfontCreate(&SafeLogfont, cjElfw, lft, fl, pvCliData);
}


/* This function is called from GetCharWidthA/W/I, GetCharWidth32A/W, and GetCharWidthFloatA/W. */
BOOL NTAPI
NtGdiGetCharWidthW(
    _In_ HDC hDC,
    _In_ UINT FirstChar,
    _In_ UINT Count,
    _In_reads_opt_(Count) PCWCH UnSafepwc,
    _In_ FLONG fl,
    _Out_writes_bytes_(Count * sizeof(INT)) PVOID Buffer)
{
    BOOL ret = FALSE;
    PVOID pTmpBuffer = NULL;
    PWCHAR pSafePwc = NULL;
    NTSTATUS Status;
    WCHAR StackPwc[40];
    INT StackBuffer[40];

    if (!Count || Count > MAX_TEXT_BUFFER / sizeof(INT))
        return FALSE;

    if (UnSafepwc)
    {
        if (Count <= _countof(StackPwc))
            pSafePwc = StackPwc;
        else
            pSafePwc = ExAllocatePoolWithTag(PagedPool, Count * sizeof(WCHAR), GDITAG_TEXT);

        if (!pSafePwc)
            return FALSE;

        Status = MmCopyFromCaller(pSafePwc, UnSafepwc, Count * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
            goto Cleanup;
    }

    if (Count <= _countof(StackBuffer))
        pTmpBuffer = StackBuffer;
    else
        pTmpBuffer = ExAllocatePoolWithTag(PagedPool, Count * sizeof(INT), GDITAG_TEXT);

    if (!pTmpBuffer)
        goto Cleanup;

    ret = GreGetCharWidthW(hDC, FirstChar, Count, pSafePwc, fl, pTmpBuffer);
    if (ret)
    {
        Status = MmCopyToCaller(Buffer, pTmpBuffer, Count * sizeof(INT));
        ret = NT_SUCCESS(Status);
    }

Cleanup:
    if (pTmpBuffer && pTmpBuffer != StackBuffer)
        ExFreePoolWithTag(pTmpBuffer, GDITAG_TEXT);
    if (pSafePwc && pSafePwc != StackPwc)
        ExFreePoolWithTag(pSafePwc, GDITAG_TEXT);
    return ret;
}
