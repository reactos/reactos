/* $Id: stubsa.c,v 1.19 2003/07/28 19:38:58 royce Exp $
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

/*
 * @implemented
 */
int
STDCALL
AddFontResourceExA ( LPCSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
  NTSTATUS Status;
  UNICODE_STRING FilenameU;
  int rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FilenameU,
					      (PCSZ)lpszFilename );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = AddFontResourceExW ( FilenameU.Buffer, fl, pvReserved );

  RtlFreeUnicodeString ( &FilenameU );

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
  UNICODE_STRING FileU;
  HMETAFILE rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FileU,
					      (PCSZ)lpszFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kCopyMetaFile ( Src, FileU.Buffer );

  RtlFreeUnicodeString ( &FileU );

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
  UNICODE_STRING Driver, Device, Output;
  DEVMODEW dvmInitW;
  HDC rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &Driver,
					      (PCSZ)lpszDriver );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &Device,
					      (PCSZ)lpszDevice );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &Output,
					      (PCSZ)lpszOutput );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  if ( lpdvmInit )
    RosRtlDevModeA2W ( &dvmInitW, (const LPDEVMODEA)lpdvmInit );

  rc = W32kCreateIC ( Driver.Buffer,
		      Device.Buffer,
		      Output.Buffer,
		      lpdvmInit ? &dvmInitW : NULL );

  RtlFreeUnicodeString ( &Output );
  RtlFreeUnicodeString ( &Device );
  RtlFreeUnicodeString ( &Driver );

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
  HDC rc;
  NTSTATUS Status;
  UNICODE_STRING File;

  Status = RtlCreateUnicodeStringFromAsciiz ( &File,
					      (PCSZ)lpszFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kCreateMetaFile ( File.Buffer );

  RtlFreeUnicodeString ( &File );

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
  UNICODE_STRING FontRes, FontFile, CurrentPath;
  BOOL rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FontRes,
					      (PCSZ)lpszFontRes );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &FontFile,
					      (PCSZ)lpszFontFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &CurrentPath,
					      (PCSZ)lpszCurrentPath );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  return W32kCreateScalableFontResource ( fdwHidden,
					  FontRes.Buffer,
					  FontFile.Buffer,
					  CurrentPath.Buffer );

  RtlFreeUnicodeString ( &FontRes );
  RtlFreeUnicodeString ( &FontFile );
  RtlFreeUnicodeString ( &CurrentPath );

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
#if 0
  NTSTATUS Status;
  UNICODE_STRING Device, Port, Output;
  DEVMODEW DevModeW;
  int rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &Device,
					      (PCSZ)pDevice );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &Port,
					      (PCSZ)pPort );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &Output,
					      (PCSZ)pOutput );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  if ( pDevMode )
    RosRtlDevModeA2W ( &DevModeW, (const LPDEVMODEA)pDevMode );

  /* FIXME no W32kDeviceCapabilities???? */
  rc = W32kDeviceCapabilities ( Device.Buffer,
		      Port.Buffer,
		      fwCapability
		      Output.Buffer,
		      pDevMode ? &DevModeW : NULL );

  RtlFreeUnicodeString ( &Device );
  RtlFreeUnicodeString ( &Port );
  RtlFreeUnicodeString ( &Output );

  return rc;
#else
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
#endif
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
  UNICODE_STRING Family;
  int rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &Family,
					      (PCSZ)lpszFamily );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kEnumFontFamilies ( hdc, Family.Buffer, lpEnumFontFamProc, lParam );

  RtlFreeUnicodeString ( &Family );

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
  UNICODE_STRING FaceName;
  int rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FaceName,
					      (PCSZ)lpFaceName );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kEnumFonts ( hDC, FaceName.Buffer, FontFunc, lParam );

  RtlFreeUnicodeString ( &FaceName );

  return rc;
}


/*
 * @implemented
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
  return W32kGetCharWidth ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @implemented
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
  return W32kGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @implemented
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
  return W32kGetCharWidthFloat ( hdc, iFirstChar, iLastChar, pxBuffer );
}


/*
 * @implemented
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
  return W32kGetCharABCWidths ( hdc, uFirstChar, uLastChar, lpabc );
}


/*
 * @implemented
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
  UNICODE_STRING MetaFile;
  HMETAFILE rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &MetaFile,
					      (PCSZ)lpszMetaFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kGetMetaFile ( MetaFile.Buffer );

  RtlFreeUnicodeString ( &MetaFile );

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
 * @unimplemented
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
  UNICODE_STRING Str;
  BOOL rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &Str,
					      (PCSZ)lpszStr );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kGetTextExtentExPoint (
    hdc, Str.Buffer, cchString, nMaxExtent, lpnFit, alpDx, lpSize );

  RtlFreeUnicodeString ( &Str );

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
  UNICODE_STRING FileName;
  BOOL rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FileName,
					      (PCSZ)lpFileName );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kRemoveFontResource ( FileName.Buffer );

  RtlFreeUnicodeString ( &FileName );

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
  UNICODE_STRING File;
  HENHMETAFILE rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &File,
					      (PCSZ)lpszFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kCopyEnhMetaFile ( hemfSrc, File.Buffer );

  RtlFreeUnicodeString ( &File );

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
  UNICODE_STRING FileName, Description;
  HDC rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &FileName,
					      (PCSZ)lpFileName );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  Status = RtlCreateUnicodeStringFromAsciiz ( &Description,
					      (PCSZ)lpDescription );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kCreateEnhMetaFile ( hdc, FileName.Buffer, (CONST LPRECT)lpRect, Description.Buffer );

  RtlFreeUnicodeString ( &FileName );
  RtlFreeUnicodeString ( &Description );

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
  UNICODE_STRING MetaFile;
  HENHMETAFILE rc;

  Status = RtlCreateUnicodeStringFromAsciiz ( &MetaFile,
					      (PCSZ)lpszMetaFile );
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }

  rc = W32kGetEnhMetaFile ( MetaFile.Buffer );

  RtlFreeUnicodeString ( &MetaFile );

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
  LPWSTR lpszDescriptionW = NULL;
  UINT rc;

  if ( lpszDescription && cchBuffer )
    {
      hHeap = RtlGetProcessHeap();
      lpszDescriptionW = (LPWSTR)RtlAllocateHeap ( hHeap, 0, cchBuffer*sizeof(WCHAR) );
    }

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
