#include "precomp.h"

/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
CloseEnhMetaFile(
	HDC	hdc)
{
	return NtGdiCloseEnhMetaFile(hdc);
}


#if 0 // Remove once new EnhMetaFile support is implemented.

HDC WINAPI CreateEnhMetaFileW(
    HDC           hDC,        /* [in] optional reference DC */
    LPCWSTR       filename,   /* [in] optional filename for disk metafiles */
    const RECT*   rect,       /* [in] optional bounding rectangle */
    LPCWSTR       description /* [in] optional description */
    )
{
    HDC mDC;
    PDC_ATTR Dc_Attr;
    PLDC pLDC;
    HANDLE hFile;
    PENHMETAFILE EmfDC;
    DWORD size = 0, length = 0;

    mDC = NtGdiCreateMetafileDC( hDC ); // Basically changes the handle from 1xxxx to 46xxxx.
    // If hDC == NULL, works just like createdc in win32k.

    if ( !GdiGetHandleUserData((HGDIOBJ) mDC, (PVOID) &Dc_Attr))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return NULL; // need to delete the handle?
    }

    pLDC = LocalAlloc(LMEM_ZEROINIT, sizeof(LDC));

    Dc_Attr->pvLDC = pLDC;
    pLDC->hDC = mDC;
    pLDC->iType = LDC_EMFDC


    if (description)
    { /* App name\0Title\0\0 */
        length = lstrlenW(description);
        length += lstrlenW(description + length + 1);
        length += 3;
        length *= 2;
    }

    size = sizeof(ENHMETAFILE) + (length + 3) / 4 * 4;

    //Allocate ENHMETAFILE structure
    EmfDC = LocalAlloc(LMEM_ZEROINIT, sizeof(ENHMETAFILE));
    pLDC->pvEmfDC = EmfDC;

    EmfDC->handles_size = HANDLE_LIST_INC;
    EmfDC->cur_handles = 1;

    EmfDC->horzres = GetDeviceCaps(mDC, HORZRES);
    EmfDC->vertres = GetDeviceCaps(mDC, VERTRES);
    EmfDC->logpixelsx = GetDeviceCaps(mDC, LOGPIXELSX);
    EmfDC->logpixelsy = GetDeviceCaps(mDC, LOGPIXELSY);
    EmfDC->horzsize = GetDeviceCaps(mDC, HORZSIZE);
    EmfDC->vertsize = GetDeviceCaps(mDC, VERTSIZE);
    EmfDC->bitspixel = GetDeviceCaps(mDC, BITSPIXEL);
    EmfDC->textcaps = GetDeviceCaps(mDC, TEXTCAPS);
    EmfDC->rastercaps = GetDeviceCaps(mDC, RASTERCAPS);
    EmfDC->technology = GetDeviceCaps(mDC, TECHNOLOGY);
    EmfDC->planes = GetDeviceCaps(mDC, PLANES);

    EmfDC->emf = LocalAlloc(LMEM_ZEROINIT, size);

    EmfDC->emf->iType = EMR_HEADER;
    EmfDC->emf->nSize = size;

    EmfDC->emf->rclBounds.left = EmfDC->emf->rclBounds.top = 0;
    EmfDC->emf->rclBounds.right = EmfDC->emf->rclBounds.bottom = -1;

    if(rect)
    {
        EmfDC->emf->rclFrame.left   = rect->left;
        EmfDC->emf->rclFrame.top    = rect->top;
        EmfDC->emf->rclFrame.right  = rect->right;
        EmfDC->emf->rclFrame.bottom = rect->bottom;
    }
    else
    {  /* Set this to {0,0 - -1,-1} and update it at the end */
        EmfDC->emf->rclFrame.left = EmfDC->emf->rclFrame.top = 0;
        EmfDC->emf->rclFrame.right = EmfDC->emf->rclFrame.bottom = -1;
    }

    EmfDC->emf->dSignature = ENHMETA_SIGNATURE;
    EmfDC->emf->nVersion = 0x10000;
    EmfDC->emf->nBytes = pLDC->pvEmfDC->nSize;
    EmfDC->emf->nRecords = 1;
    EmfDC->emf->nHandles = 1;

    EmfDC->emf->sReserved = 0; /* According to docs, this is reserved and must be 0 */
    EmfDC->emf->nDescription = length / 2;

    EmfDC->emf->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

    EmfDC->emf->nPalEntries = 0; /* I guess this should start at 0 */

    /* Size in pixels */
    EmfDC->emf->szlDevice.cx = EmfDC->horzres;
    EmfDC->emf->szlDevice.cy = EmfDC->vertres;

    /* Size in millimeters */
    EmfDC->emf->szlMillimeters.cx = EmfDC->horzsize;
    EmfDC->emf->szlMillimeters.cy = EmfDC->vertsize;

    /* Size in micrometers */
    EmfDC->emf->szlMicrometers.cx = EmfDC->horzsize * 1000;
    EmfDC->emf->szlMicrometers.cy = EmfDC->vertsize * 1000;

    RtlCopyMemory((char *)EmfDC->emf + sizeof(ENHMETAHEADER), description, length);

    if (filename)  /* disk based metafile */
    {
        if ((hFile = CreateFileW(filename, GENERIC_WRITE | GENERIC_READ, 0,
				 NULL, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
        {
            EMFDRV_DeleteDC( EmfDC );
            return NULL;
        }
        if (!WriteFile( hFile, (LPSTR)EmfDC->emf, size, NULL, NULL ))
        {
            EMFDRV_DeleteDC( EmfDC );
            return NULL;
	}
	EmfDC.hFile = hFile;
	EmfDC.iType = METAFILE_DISK;
    }
    else
        EmfDC.iType = METAFILE_MEMORY;

    return mDC;
}
#endif

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

  lpFileNameW = NULL;
  if (lpFileName != NULL)
  {
	 Status = HEAP_strdupA2W ( &lpFileNameW, lpFileName );
     if (!NT_SUCCESS (Status))
         SetLastError (RtlNtStatusToDosError(Status));

	 return rc;
  }

  lpDescriptionW = NULL;
  if (lpDescription != NULL)
  {
     Status = HEAP_strdupA2W ( &lpDescriptionW, lpDescription );
      if (!NT_SUCCESS (Status))
	      SetLastError (RtlNtStatusToDosError(Status));

	  return rc;
  }

  rc = NtGdiCreateEnhMetaFile (hdcRef, lpFileNameW, (CONST LPRECT)lpRect, lpDescriptionW );

  if (lpDescriptionW != NULL)
      HEAP_free ( lpDescriptionW );

  if (lpFileNameW != NULL)
       HEAP_free ( lpFileNameW );

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

