/* $Id: stubsa.c,v 1.14 2003/07/21 04:56:31 royce Exp $
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
 * @unimplemented
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
 * @unimplemented
 */
int
STDCALL
EnumFontFamiliesExA(
	HDC		hdc,
	LPLOGFONT	lpLogFont,
	FONTENUMEXPROC	lpEnumFontFamProc,
	LPARAM		lParam,
	DWORD		dwFlags
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
EnumFontFamiliesA(
	HDC		hdc,
	LPCSTR		lpszFamily,
	FONTENUMPROC	lpEnumFontFamProc,
	LPARAM		lParam
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
EnumFontsA(
	HDC		a0,
	LPCSTR		a1,
	ENUMFONTSPROC	a2,
	LPARAM		a3
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
GetCharWidthA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPINT	a3
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
GetCharWidth32A(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharWidthFloatA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	PFLOAT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharABCWidthsA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPABC	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatA(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPABCFLOAT	a3
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
GetGlyphOutlineA(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPGLYPHMETRICS	a3,
	DWORD		a4,
	LPVOID		a5,
	CONST MAT2	*a6
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HMETAFILE
STDCALL
GetMetaFileA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
	HDC			a0,
	UINT			a1,
	LPOUTLINETEXTMETRICA	a2
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
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPINT		a4,
	LPINT		a5,
	LPSIZE		a6
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
 * @unimplemented
 */
HDC
STDCALL
ResetDCA(
	HDC		a0,
	CONST DEVMODEA	*a1
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
RemoveFontResourceA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HENHMETAFILE 
STDCALL 
CopyEnhMetaFileA(
	HENHMETAFILE	a0,
	LPCSTR		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HDC   
STDCALL 
CreateEnhMetaFileA(
	HDC		a0,
	LPCSTR		a1,
	CONST RECT	*a2,
	LPCSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HENHMETAFILE  
STDCALL 
GetEnhMetaFileA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
UINT  
STDCALL 
GetEnhMetaFileDescriptionA(
	HENHMETAFILE	a0,
	UINT		a1,
	LPSTR		a2
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
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
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
