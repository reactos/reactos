/* $Id: stubsa.c,v 1.20 2003/07/29 16:44:48 royce Exp $
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for ANSI functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */
#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/text.h>
#include <win32k/metafile.h>
#include <win32k/dc.h>
#include <rosrtl/devmode.h>
#include <rosrtl/logfont.h>

NTSTATUS
STATIC
HEAP_strdupA2W ( HANDLE hHeap, LPWSTR* ppszW, LPCSTR lpszA )
{
  ULONG len;
  *ppszW = NULL;
  if ( !lpszA )
    return STATUS_SUCCESS;
  len = lstrlenA(lpszA);
  *ppszW = RtlAllocateHeap ( hHeap, 0, (len+1) * sizeof(WCHAR) );
  if ( !*ppszW )
    return STATUS_NO_MEMORY;
  return RtlMultiByteToUnicodeN ( *ppszW, len, NULL, (PCHAR)lpszA, len );
}


VOID
STATIC HEAP_free ( HANDLE hHeap, LPVOID memory )
{
  RtlFreeHeap ( hHeap, 0, memory );
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceExA ( LPCSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  PWSTR FilenameW;
  int rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &FilenameW, lpszFilename );
  if ( !NT_SUCCESS (Status) )
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = AddFontResourceExW ( FilenameW, fl, pvReserved );

      HEAP_free ( hHeap, &FilenameW );
    }
  return rc;
}

/*
 * @implemented
 */
int
STDCALL
AddFontResourceA ( LPCSTR lpszFilename )
{
  return AddFontResourceExA ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
CopyMetaFileA(
	HMETAFILE	Src,
	LPCSTR		lpszFile
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  PWSTR lpszFileW;
  HMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = W32kCopyMetaFile ( Src, lpszFileW );

    HEAP_free ( hHeap, lpszFileW );
  }

  return rc;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICA(
	LPCSTR			lpszDriver,
	LPCSTR			lpszDevice,
	LPCSTR			lpszOutput,
	CONST DEVMODEA *	lpdvmInit
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszDriverW, lpszDeviceW, lpszOutputW;
  DEVMODEW dvmInitW;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszDriverW, lpszDriver );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    Status = HEAP_strdupA2W ( hHeap, &lpszDeviceW, lpszDevice );
    if (!NT_SUCCESS (Status))
      SetLastError (RtlNtStatusToDosError(Status));
    else
      {
	Status = HEAP_strdupA2W ( hHeap, &lpszOutputW, lpszOutput );
	if (!NT_SUCCESS (Status))
	  SetLastError (RtlNtStatusToDosError(Status));
	else
	  {
	    if ( lpdvmInit )
	      RosRtlDevModeA2W ( &dvmInitW, (const LPDEVMODEA)lpdvmInit );

	    rc = W32kCreateIC ( lpszDriverW,
				lpszDeviceW,
				lpszOutputW,
				lpdvmInit ? &dvmInitW : NULL );

	    HEAP_free ( hHeap, lpszOutputW );
	  }
	HEAP_free ( hHeap, lpszDeviceW );
      }
    HEAP_free ( hHeap, lpszDriverW );
  }
  return rc;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateMetaFileA(
	LPCSTR		lpszFile
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  PWSTR lpszFileW;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kCreateMetaFile ( lpszFileW );

      HEAP_free ( hHeap, lpszFileW );
    }
  return rc;
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateScalableFontResourceA(
	DWORD		fdwHidden,
	LPCSTR		lpszFontRes,
	LPCSTR		lpszFontFile,
	LPCSTR		lpszCurrentPath
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszFontResW, lpszFontFileW, lpszCurrentPathW;
  BOOL rc = FALSE;

  Status = HEAP_strdupA2W ( hHeap, &lpszFontResW, lpszFontRes );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      Status = HEAP_strdupA2W ( hHeap, &lpszFontFileW, lpszFontFile );
      if (!NT_SUCCESS (Status))
	SetLastError (RtlNtStatusToDosError(Status));
      else
	{
	  Status = HEAP_strdupA2W ( hHeap, &lpszCurrentPathW, lpszCurrentPath );
	  if (!NT_SUCCESS (Status))
	    SetLastError (RtlNtStatusToDosError(Status));
	  else
	    {
	      rc = W32kCreateScalableFontResource ( fdwHidden,
						    lpszFontResW,
						    lpszFontFileW,
						    lpszCurrentPathW );

	      HEAP_free ( hHeap, lpszCurrentPathW );
	    }

	  HEAP_free ( hHeap, lpszFontFileW );
	}

      HEAP_free ( hHeap, lpszFontResW );
    }
  return rc;
}


/*
 * @unimplemented
 */
int
STDCALL
DeviceCapabilitiesExA(
	LPCSTR		pDevice,
	LPCSTR		pPort,
	WORD		fwCapability,
	LPSTR		pOutput,
	CONST DEVMODEA	*pDevMode
	)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
int
STDCALL
EnumFontFamiliesExA(
	HDC		hdc,
	LPLOGFONTA	lpLogFont,
	FONTENUMEXPROC	lpEnumFontFamProc,
	LPARAM		lParam,
	DWORD		dwFlags
	)
{
  LOGFONTW LogFontW;

  RosRtlLogFontA2W ( &LogFontW, lpLogFont );

  /* no need to convert LogFontW back to lpLogFont b/c it's an [in] parameter only */
  return W32kEnumFontFamiliesEx ( hdc, &LogFontW, lpEnumFontFamProc, lParam, dwFlags );
}


/*
 * @implemented
 */
int
STDCALL
EnumFontFamiliesA(
	HDC		hdc,
	LPCSTR		lpszFamily,
	FONTENUMPROC	lpEnumFontFamProc,
	LPARAM		lParam
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszFamilyW;
  int rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszFamilyW, lpszFamily );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kEnumFontFamilies ( hdc, lpszFamilyW, lpEnumFontFamProc, lParam );

      HEAP_free ( hHeap, lpszFamilyW );
    }

  return rc;
}


/*
 * @implemented
 */
int
STDCALL
EnumFontsA (
	HDC  hDC,
	LPCSTR lpFaceName,
	FONTENUMPROC  FontFunc,
	LPARAM  lParam
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpFaceNameW;
  int rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpFaceNameW, lpFaceName );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kEnumFonts ( hDC, lpFaceNameW, FontFunc, lParam );

      HEAP_free ( hHeap, lpFaceNameW );
    }
  return rc;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetCharWidthA (
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return W32kGetCharWidth ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetCharWidth32A(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return W32kGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharWidthFloatA(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	PFLOAT	pxBuffer
	)
{
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return W32kGetCharWidthFloat ( hdc, iFirstChar, iLastChar, pxBuffer );
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharABCWidthsA(
	HDC	hdc,
	UINT	uFirstChar,
	UINT	uLastChar,
	LPABC	lpabc
	)
{
  /* FIXME what to do with uFirstChar and uLastChar ??? */
  return W32kGetCharABCWidths ( hdc, uFirstChar, uLastChar, lpabc );
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatA(
	HDC		hdc,
	UINT		iFirstChar,
	UINT		iLastChar,
	LPABCFLOAT	lpABCF
	)
{
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return W32kGetCharABCWidthsFloat ( hdc, iFirstChar, iLastChar, lpABCF );
}


/*
 * @implemented
 */
DWORD
STDCALL
GetGlyphOutlineA(
	HDC		hdc,
	UINT		uChar,
	UINT		uFormat,
	LPGLYPHMETRICS	lpgm,
	DWORD		cbBuffer,
	LPVOID		lpvBuffer,
	CONST MAT2	*lpmat2
	)
{
  return W32kGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2 );
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
GetMetaFileA(
	LPCSTR	lpszMetaFile
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszMetaFileW;
  HMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszMetaFileW, lpszMetaFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kGetMetaFile ( lpszMetaFileW );

      HEAP_free ( hHeap, lpszMetaFileW );
    }

  return rc;
}


/*
 * @unimplemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
	HDC			hdc,
	UINT			cbData,
	LPOUTLINETEXTMETRICA	lpOTM
	)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointA(
	HDC		hdc,
	LPCSTR		lpszStr,
	int		cchString,
	int		nMaxExtent,
	LPINT		lpnFit,
	LPINT		alpDx,
	LPSIZE		lpSize
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszStrW;
  BOOL rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszStrW, lpszStr );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = W32kGetTextExtentExPoint (
      hdc, lpszStrW, cchString, nMaxExtent, lpnFit, alpDx, lpSize );

    HEAP_free ( hHeap, lpszStrW );
  }

  return rc;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetCharacterPlacementA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPGCP_RESULTS	a4,
	DWORD		a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @implemented
 */
HDC
STDCALL
ResetDCA(
	HDC		hdc,
	CONST DEVMODEA	*lpInitData
	)
{
  DEVMODEW InitDataW;

  RosRtlDevModeA2W ( &InitDataW, (CONST LPDEVMODEA)lpInitData );

  return W32kResetDC ( hdc, &InitDataW );
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceA(
	LPCSTR	lpFileName
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpFileNameW;
  BOOL rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpFileNameW, lpFileName );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kRemoveFontResource ( lpFileNameW );

      HEAP_free ( hHeap, lpFileNameW );
    }

  return rc;
}


/*
 * @implemented
 */
HENHMETAFILE
STDCALL
CopyEnhMetaFileA(
	HENHMETAFILE	hemfSrc,
	LPCSTR		lpszFile
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszFileW;
  HENHMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = W32kCopyEnhMetaFile ( hemfSrc, lpszFileW );

      HEAP_free ( hHeap, lpszFileW );
    }
  return rc;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateEnhMetaFileA(
	HDC		hdc,
	LPCSTR		lpFileName,
	CONST RECT	*lpRect,
	LPCSTR		lpDescription
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpFileNameW, lpDescriptionW;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpFileNameW, lpFileName );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      Status = HEAP_strdupA2W ( hHeap, &lpDescriptionW, lpDescription );
      if (!NT_SUCCESS (Status))
	SetLastError (RtlNtStatusToDosError(Status));
      else
      {
	rc = W32kCreateEnhMetaFile (
	  hdc, lpFileNameW, (CONST LPRECT)lpRect, lpDescriptionW );

	HEAP_free ( hHeap, lpDescriptionW );
      }
      HEAP_free ( hHeap, lpFileNameW );
    }

  return rc;
}


/*
 * @implemented
 */
HENHMETAFILE
STDCALL
GetEnhMetaFileA(
	LPCSTR	lpszMetaFile
	)
{
  NTSTATUS Status;
  HANDLE hHeap = RtlGetProcessHeap();
  LPWSTR lpszMetaFileW;
  HENHMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( hHeap, &lpszMetaFileW, lpszMetaFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = W32kGetEnhMetaFile ( lpszMetaFileW );

    HEAP_free ( hHeap, lpszMetaFileW );
  }

  return rc;
}


/*
 * @implemented
 */
UINT  
STDCALL 
GetEnhMetaFileDescriptionA(
	HENHMETAFILE	hemf,
	UINT		cchBuffer,
	LPSTR		lpszDescription
	)
{
  HANDLE hHeap;
  NTSTATUS Status;
  LPWSTR lpszDescriptionW;
  UINT rc;

  if ( lpszDescription && cchBuffer )
    {
      hHeap = RtlGetProcessHeap();
      lpszDescriptionW = (LPWSTR)RtlAllocateHeap ( hHeap, 0, cchBuffer*sizeof(WCHAR) );
      if ( !lpszDescriptionW )
	{
	  SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
	  return 0;
	}
    }
  else
    lpszDescriptionW = NULL;

  rc = W32kGetEnhMetaFileDescription ( hemf, cchBuffer, lpszDescriptionW );

  if ( lpszDescription && cchBuffer )
    {
      Status = RtlUnicodeToMultiByteN ( lpszDescription,
	                                cchBuffer,
	                                NULL,
	                                lpszDescriptionW,
	                                cchBuffer );
      RtlFreeHeap ( hHeap, 0, lpszDescriptionW );
      if ( !NT_SUCCESS(Status) )
	{
	  SetLastError (RtlNtStatusToDosError(Status));
	  return 0;
	}
    }

  return rc;
}


/*
 * @unimplemented
 */
int
STDCALL
StartDocA(
	HDC		hdc,
	CONST DOCINFO	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectA(
	HGDIOBJ		a0, 
	int		a1, 
	LPVOID		a2
	)
{
	return W32kGetObject ( a0, a1, a2 );
}


/*
 * @unimplemented
 */
BOOL  
STDCALL 
PolyTextOutA(
	HDC			hdc, 
	CONST POLYTEXT		*a1, 
	int			a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
GetTextFaceA(
	HDC	a0,
	int	a1,
	LPSTR	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetKerningPairsA(
	HDC		a0,
	DWORD		a1,
	LPKERNINGPAIR	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetLogColorSpaceA(
	HCOLORSPACE		a0,
	LPLOGCOLORSPACE	a1,
	DWORD			a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
CreateColorSpaceA(
	LPLOGCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetICMProfileA(
	HDC		a0,
	DWORD		a1,	/* MS says LPDWORD! */
	LPSTR		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetICMProfileA(
	HDC	a0,
	LPSTR	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
EnumICMProfilesA(
	HDC		a0,
	ICMENUMPROC	a1,
	LPARAM		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglUseFontBitmapsA(
	HDC		a0,
	DWORD		a1,
	DWORD		a2,
	DWORD		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglUseFontOutlinesA(
	HDC			a0,
	DWORD			a1,
	DWORD			a2,
	DWORD			a3,
	FLOAT			a4,
	FLOAT			a5,
	int			a6,
	LPGLYPHMETRICSFLOAT	a7
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UpdateICMRegKeyA(
	DWORD	a0,
	DWORD	a1,
	LPSTR	a2,
	UINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
