#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <string.h>
#include <windows.h>
#include <ddk/ntddk.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <internal/heap.h>
#include <win32k/kapi.h>
#include <win32k/metafile.h>


/*
 * @implemented
 */
HMETAFILE
STDCALL
CopyMetaFileW(
	HMETAFILE	hmfSrc,
	LPCWSTR		lpszFile
	)
{
  return NtGdiCopyMetaFile (hmfSrc, lpszFile);
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
CopyMetaFileA(
	HMETAFILE	hmfSrc,
	LPCSTR		lpszFile
	)
{
  NTSTATUS Status;
  PWSTR lpszFileW;
  HMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = NtGdiCopyMetaFile ( hmfSrc, lpszFileW );

    HEAP_free ( lpszFileW );
  }

  return rc;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateMetaFileW(
	LPCWSTR		lpszFile
	)
{
  return NtGdiCreateMetaFile ( lpszFile );
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
  PWSTR lpszFileW;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = NtGdiCreateMetaFile ( lpszFileW );

      HEAP_free ( lpszFileW );
    }
  return rc;
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
GetMetaFileW(
	LPCWSTR	lpszMetaFile
	)
{
  return NtGdiGetMetaFile ( lpszMetaFile );
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
  LPWSTR lpszMetaFileW;
  HMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( &lpszMetaFileW, lpszMetaFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = NtGdiGetMetaFile ( lpszMetaFileW );

      HEAP_free ( lpszMetaFileW );
    }

  return rc;
}


/*
 * @implemented
 */
HENHMETAFILE 
STDCALL 
CopyEnhMetaFileW(
	HENHMETAFILE	hemfSrc,
	LPCWSTR		lpszFile
	)
{
  return NtGdiCopyEnhMetaFile ( hemfSrc, lpszFile );
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
  LPWSTR lpszFileW;
  HENHMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( &lpszFileW, lpszFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = NtGdiCopyEnhMetaFile ( hemfSrc, lpszFileW );

      HEAP_free ( lpszFileW );
    }
  return rc;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateEnhMetaFileW(
	HDC		hdcRef,
	LPCWSTR		lpFileName,
	CONST RECT	*lpRect,
	LPCWSTR		lpDescription
	)
{
  return NtGdiCreateEnhMetaFile ( hdcRef, lpFileName, (CONST LPRECT)lpRect, lpDescription );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateEnhMetaFileA(
	HDC		hdcRef,
	LPCSTR		lpFileName,
	CONST RECT	*lpRect,
	LPCSTR		lpDescription
	)
{
  NTSTATUS Status;
  LPWSTR lpFileNameW, lpDescriptionW;
  HDC rc = 0;

  Status = HEAP_strdupA2W ( &lpFileNameW, lpFileName );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      Status = HEAP_strdupA2W ( &lpDescriptionW, lpDescription );
      if (!NT_SUCCESS (Status))
	SetLastError (RtlNtStatusToDosError(Status));
      else
      {
	rc = NtGdiCreateEnhMetaFile (
	  hdcRef, lpFileNameW, (CONST LPRECT)lpRect, lpDescriptionW );

	HEAP_free ( lpDescriptionW );
      }
      HEAP_free ( lpFileNameW );
    }

  return rc;
}

/*
 * @implemented
 */
HENHMETAFILE
STDCALL
GetEnhMetaFileW(
	LPCWSTR	lpszMetaFile
	)
{
  return NtGdiGetEnhMetaFile ( lpszMetaFile );
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
  LPWSTR lpszMetaFileW;
  HENHMETAFILE rc = 0;

  Status = HEAP_strdupA2W ( &lpszMetaFileW, lpszMetaFile );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = NtGdiGetEnhMetaFile ( lpszMetaFileW );

    HEAP_free ( lpszMetaFileW );
  }

  return rc;
}


/*
 * @implemented
 */
UINT
STDCALL
GetEnhMetaFileDescriptionW(
	HENHMETAFILE	hemf,
	UINT		cchBuffer,
	LPWSTR		lpszDescription
	)
{
  return NtGdiGetEnhMetaFileDescription ( hemf, cchBuffer, lpszDescription );
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
  NTSTATUS Status;
  LPWSTR lpszDescriptionW;
  UINT rc;

  if ( lpszDescription && cchBuffer )
    {
      lpszDescriptionW = (LPWSTR)HEAP_alloc ( cchBuffer*sizeof(WCHAR) );
      if ( !lpszDescriptionW )
	{
	  SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
	  return 0;
	}
    }
  else
    lpszDescriptionW = NULL;

  rc = NtGdiGetEnhMetaFileDescription ( hemf, cchBuffer, lpszDescriptionW );

  if ( lpszDescription && cchBuffer )
    {
      Status = RtlUnicodeToMultiByteN ( lpszDescription,
	                                cchBuffer,
	                                NULL,
	                                lpszDescriptionW,
	                                cchBuffer );
      HEAP_free ( lpszDescriptionW );
      if ( !NT_SUCCESS(Status) )
	{
	  SetLastError (RtlNtStatusToDosError(Status));
	  return 0;
	}
    }

  return rc;
}

