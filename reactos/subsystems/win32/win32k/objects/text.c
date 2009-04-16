/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/text.c
 * PURPOSE:         Text/Font
 * PROGRAMMER:      
 */
      
/** Includes ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/** Functions ******************************************************************/

DWORD
APIENTRY
NtGdiGetCharSet(HDC hDC)
{
  PDC Dc;
  PDC_ATTR pdcattr;
  DWORD cscp;
  // If here, update everything!
  Dc = DC_LockDc(hDC);
  if (!Dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return 0;
  }
  cscp = ftGdiGetTextCharsetInfo(Dc,NULL,0);
  pdcattr = Dc->pdcattr;
  pdcattr->iCS_CP = cscp;
  pdcattr->ulDirty_ &= ~DIRTY_CHARSET;
  DC_UnlockDc( Dc );
  return cscp;
}

BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    OUT LPRASTERIZER_STATUS praststat,
    IN ULONG cjBytes)
{
  NTSTATUS Status = STATUS_SUCCESS;
  RASTERIZER_STATUS rsSafe;

  if (praststat && cjBytes)
  {
     if ( cjBytes >= sizeof(RASTERIZER_STATUS) ) cjBytes = sizeof(RASTERIZER_STATUS);
     if ( ftGdiGetRasterizerCaps(&rsSafe))
     {
        _SEH2_TRY
        {
           ProbeForWrite( praststat,
                          sizeof(RASTERIZER_STATUS),
                          1);
           RtlCopyMemory(praststat, &rsSafe, cjBytes );
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
           Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
           SetLastNtError(Status);
           return FALSE;
        }
     }
  }
  return FALSE;
}

INT
APIENTRY
NtGdiGetTextCharsetInfo(
    IN HDC hdc,
    OUT OPTIONAL LPFONTSIGNATURE lpSig,
    IN DWORD dwFlags)
{
  PDC Dc;
  INT Ret;
  FONTSIGNATURE fsSafe;
  PFONTSIGNATURE pfsSafe = &fsSafe;
  NTSTATUS Status = STATUS_SUCCESS;

  Dc = DC_LockDc(hdc);
  if (!Dc)
  {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return DEFAULT_CHARSET;
  }

  if (!lpSig) pfsSafe = NULL;

  Ret = HIWORD(ftGdiGetTextCharsetInfo( Dc, pfsSafe, dwFlags));

  if (lpSig)
  {
     if (Ret == DEFAULT_CHARSET)
        RtlZeroMemory(pfsSafe, sizeof(FONTSIGNATURE));

     _SEH2_TRY
     {
         ProbeForWrite( lpSig,
                        sizeof(FONTSIGNATURE),
                        1);
         RtlCopyMemory(lpSig, pfsSafe, sizeof(FONTSIGNATURE));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         return DEFAULT_CHARSET;
      }
  }
  DC_UnlockDc(Dc);
  return Ret;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtentExW(
    IN HDC hDC,
    IN OPTIONAL LPWSTR UnsafeString,
    IN ULONG Count,
    IN ULONG MaxExtent,
    OUT OPTIONAL PULONG UnsafeFit,
    OUT OPTIONAL PULONG UnsafeDx,
    OUT LPSIZE UnsafeSize,
    IN FLONG fl
)
{
  PDC dc;
  PDC_ATTR pdcattr;
  LPWSTR String;
  SIZE Size;
  NTSTATUS Status;
  BOOLEAN Result;
  INT Fit;
  LPINT Dx;
  PTEXTOBJ TextObj;

  /* FIXME: Handle fl */

  if (0 == Count)
    {
      Size.cx = 0;
      Size.cy = 0;
      Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  return FALSE;
	}
      return TRUE;
    }

  String = ExAllocatePoolWithTag(PagedPool, Count * sizeof(WCHAR), TAG_GDITEXT);
  if (NULL == String)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (NULL != UnsafeDx)
    {
      Dx = ExAllocatePoolWithTag(PagedPool, Count * sizeof(INT), TAG_GDITEXT);
      if (NULL == Dx)
	{
	  ExFreePoolWithTag(String, TAG_GDITEXT);
	  SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
	  return FALSE;
	}
    }
  else
    {
      Dx = NULL;
    }

  Status = MmCopyFromCaller(String, UnsafeString, Count * sizeof(WCHAR));
  if (! NT_SUCCESS(Status))
    {
      if (NULL != Dx)
	{
	  ExFreePoolWithTag(Dx, TAG_GDITEXT);
	}
      ExFreePoolWithTag(String, TAG_GDITEXT);
      SetLastNtError(Status);
      return FALSE;
    }

  dc = DC_LockDc(hDC);
  if (NULL == dc)
    {
      if (NULL != Dx)
	{
	  ExFreePoolWithTag(Dx, TAG_GDITEXT);
	}
      ExFreePoolWithTag(String, TAG_GDITEXT);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  pdcattr = dc->pdcattr;
  TextObj = RealizeFontInit(pdcattr->hlfntNew);
  if ( TextObj )
  {
    Result = TextIntGetTextExtentPoint(dc, TextObj, String, Count, MaxExtent,
                                     NULL == UnsafeFit ? NULL : &Fit, Dx, &Size);
    TEXTOBJ_UnlockText(TextObj);
  }
  else
    Result = FALSE;
  DC_UnlockDc(dc);

  ExFreePoolWithTag(String, TAG_GDITEXT);
  if (! Result)
    {
      if (NULL != Dx)
	{
	  ExFreePoolWithTag(Dx, TAG_GDITEXT);
	}
      return FALSE;
    }

  if (NULL != UnsafeFit)
    {
      Status = MmCopyToCaller(UnsafeFit, &Fit, sizeof(INT));
      if (! NT_SUCCESS(Status))
	{
	  if (NULL != Dx)
	    {
	      ExFreePoolWithTag(Dx, TAG_GDITEXT);
	    }
	  SetLastNtError(Status);
	  return FALSE;
	}
    }

  if (NULL != UnsafeDx)
    {
      Status = MmCopyToCaller(UnsafeDx, Dx, Count * sizeof(INT));
      if (! NT_SUCCESS(Status))
	{
	  if (NULL != Dx)
	    {
	      ExFreePoolWithTag(Dx, TAG_GDITEXT);
	    }
	  SetLastNtError(Status);
	  return FALSE;
	}
    }
  if (NULL != Dx)
    {
      ExFreePoolWithTag(Dx,TAG_GDITEXT);
    }

  Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }

  return TRUE;
}

BOOL
APIENTRY
NtGdiGetTextExtent(HDC hdc,
                   LPWSTR lpwsz,
                   INT cwc,
                   LPSIZE psize,
                   UINT flOpts)
{
  return NtGdiGetTextExtentExW(hdc, lpwsz, cwc, 0, NULL, NULL, psize, 0);
}

BOOL
APIENTRY
NtGdiSetTextJustification(HDC  hDC,
                          int  BreakExtra,
                          int  BreakCount)
{
  PDC pDc;
  PDC_ATTR pdcattr;

  pDc = DC_LockDc(hDC);
  if (!pDc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  pdcattr = pDc->pdcattr;

  pdcattr->lBreakExtra = BreakExtra;
  pdcattr->cBreak = BreakCount;

  DC_UnlockDc(pDc);
  return TRUE;
}


W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    IN HDC hDC,
    IN INT Count,
    OUT OPTIONAL LPWSTR FaceName,
    IN BOOL bAliasName
)
{
   PDC Dc;
   PDC_ATTR pdcattr;
   HFONT hFont;
   PTEXTOBJ TextObj;
   NTSTATUS Status;
   INT fLen, ret;

   /* FIXME: Handle bAliasName */

   Dc = DC_LockDc(hDC);
   if (Dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   pdcattr = Dc->pdcattr;
   hFont = pdcattr->hlfntNew;
   DC_UnlockDc(Dc);

   TextObj = RealizeFontInit(hFont);
   ASSERT(TextObj != NULL);
   fLen = wcslen(TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfFaceName) + 1;

   if (FaceName != NULL)
   {
      Count = min(Count, fLen);
      Status = MmCopyToCaller(FaceName, TextObj->logfont.elfEnumLogfontEx.elfLogFont.lfFaceName, Count * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
      {
         TEXTOBJ_UnlockText(TextObj);
         SetLastNtError(Status);
         return 0;
      }
      /* Terminate if we copied only part of the font name */
      if (Count > 0 && Count < fLen)
      {
         FaceName[Count - 1] = '\0';
      }
      ret = Count;
   }
   else
   {
      ret = fLen; 
   }

   TEXTOBJ_UnlockText(TextObj);
   return ret;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    IN HDC hDC,
    OUT TMW_INTERNAL * pUnsafeTmwi,
    IN ULONG cj
)
{
  TMW_INTERNAL Tmwi;
  NTSTATUS Status = STATUS_SUCCESS;

  if ( cj <= sizeof(TMW_INTERNAL) )
  {
     if (ftGdiGetTextMetricsW(hDC,&Tmwi))
     {
        _SEH2_TRY
        {
           ProbeForWrite(pUnsafeTmwi, cj, 1);
           RtlCopyMemory(pUnsafeTmwi,&Tmwi,cj);
        }
       _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
           Status = _SEH2_GetExceptionCode();
        }
       _SEH2_END

        if (!NT_SUCCESS(Status))
        {
           SetLastNtError(Status);
           return FALSE;
        }
        return TRUE;
     }
  }
  return FALSE;
}

/* EOF */
