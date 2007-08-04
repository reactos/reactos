#include "precomp.h"


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
  return NULL;
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

    rc = NULL;
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
  return NULL;
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
      rc = NULL;

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
  return NULL;
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
      rc = NULL;

      HEAP_free ( lpszMetaFileW );
    }

  return rc;
}


