#include "precomp.h"

#define NDEBUG
#include <debug.h>


HCOLORSPACE
FASTCALL
IntCreateColorSpaceW(
        LPLOGCOLORSPACEW lplcpw,
        BOOL Ascii
        )
{
  LOGCOLORSPACEEXW lcpeexw;  

  if ((lplcpw->lcsSignature != LCS_SIGNATURE) ||
                (lplcpw->lcsVersion != 0x400) ||
      (lplcpw->lcsSize != sizeof(LOGCOLORSPACEW)))
  {
      SetLastError(ERROR_INVALID_COLORSPACE); 
      return NULL;
  }
  RtlCopyMemory(&lcpeexw.lcsColorSpace, lplcpw, sizeof(LOGCOLORSPACEW));
 
  return NtGdiCreateColorSpace(&lcpeexw);
}

/*
 * @implemented
 */
HCOLORSPACE
STDCALL
CreateColorSpaceW(
	LPLOGCOLORSPACEW lplcpw
	)
{
  return IntCreateColorSpaceW(lplcpw, FALSE);
}


/*
 * @implemented
 */
HCOLORSPACE
STDCALL
CreateColorSpaceA(
	LPLOGCOLORSPACEA lplcpa
	)
{
  LOGCOLORSPACEW lcpw;

  if ((lplcpa->lcsSignature != LCS_SIGNATURE) ||
                (lplcpa->lcsVersion != 0x400) ||
      (lplcpa->lcsSize != sizeof(LOGCOLORSPACEA)))
  {
      SetLastError(ERROR_INVALID_COLORSPACE); 
      return NULL;
  }

  lcpw.lcsSignature  = lplcpa->lcsSignature;
  lcpw.lcsVersion    = lplcpa->lcsVersion;
  lcpw.lcsSize       = sizeof(LOGCOLORSPACEW);
  lcpw.lcsCSType     = lplcpa->lcsCSType;
  lcpw.lcsIntent     = lplcpa->lcsIntent;
  lcpw.lcsEndpoints  = lplcpa->lcsEndpoints;
  lcpw.lcsGammaRed   = lplcpa->lcsGammaRed;
  lcpw.lcsGammaGreen = lplcpa->lcsGammaGreen;
  lcpw.lcsGammaBlue  = lplcpa->lcsGammaBlue;

  RtlMultiByteToUnicodeN( lcpw.lcsFilename,
                                  MAX_PATH,
                                      NULL,
                       lplcpa->lcsFilename,
           strlen(lplcpa->lcsFilename) + 1);

  return IntCreateColorSpaceW(&lcpw, FALSE);
}

/*
 * @implemented
 */
HCOLORSPACE
STDCALL
GetColorSpace(HDC hDC)
{
  PDC_ATTR pDc_Attr;

  if (!GdiGetHandleUserData(hDC, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
  {
     SetLastError(ERROR_INVALID_HANDLE);
     return NULL;
  }
  return pDc_Attr->hColorSpace;
}


/*
 * @implemented
 */
HCOLORSPACE
STDCALL
SetColorSpace(
	HDC hDC,
	HCOLORSPACE hCS
	)
{
  HCOLORSPACE rhCS = GetColorSpace(hDC);

  if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
  {
     if (NtGdiSetColorSpace(hDC, hCS)) return rhCS;
  }
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC)
  {
     PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return NULL;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return NULL;
      }
  }
#endif
  return NULL;
}

