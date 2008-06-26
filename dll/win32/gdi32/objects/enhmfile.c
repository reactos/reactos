#include "precomp.h"

#define NDEBUG
#include <debug.h>



/*
 * @unimplemented 
 */
HENHMETAFILE
WINAPI
CloseEnhMetaFile(
	HDC	hdc)
{
	UNIMPLEMENTED;
	return 0;
}


#if 0 // Remove once new EnhMetaFile support is implemented.
HDC
WINAPI
CreateEnhMetaFileW(
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

    if ( !GdiGetHandleUserData((HGDIOBJ) mDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr))
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
 * @unimplemented
 */
HENHMETAFILE
WINAPI
CopyEnhMetaFileA(
	HENHMETAFILE	hemfSrc,
	LPCSTR		lpszFile)
{
  NTSTATUS Status;
  LPWSTR lpszFileW;
  HENHMETAFILE rc = 0;

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
 * @unimplemented
 */
HDC
WINAPI
CreateEnhMetaFileA(
	HDC		hdcRef,
	LPCSTR		lpFileName,
	CONST RECT	*lpRect,
	LPCSTR		lpDescription)
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

  rc = NULL;

  if (lpDescriptionW != NULL)
      HEAP_free ( lpDescriptionW );

  if (lpFileNameW != NULL)
       HEAP_free ( lpFileNameW );

  return rc;
}

#if 0
/* Previous implementation in win32k */
HDC
STDCALL
NtGdiCreateEnhMetaFile(HDC  hDCRef,
                           LPCWSTR  File,
                           CONST LPRECT  Rect,
                           LPCWSTR  Description)
{
   PDC Dc;
   HDC ret = NULL;
   DWORD length = 0;
   HDC tempHDC;
   DWORD MemSize;
   DWORD dwDesiredAccess;

   tempHDC = hDCRef;
   if (hDCRef == NULL)
   {
       /* FIXME ??
        * Shall we create hdc NtGdiHdcCompatible hdc ??
        */
       UNICODE_STRING DriverName;
       RtlInitUnicodeString(&DriverName, L"DISPLAY");
       //IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
           tempHDC = NtGdiOpenDCW( &DriverName,
                                          NULL,
                                          NULL,
                                             0,  // DCW 0 and ICW 1.
                                          NULL,
                                  (PVOID) NULL,
                                  (PVOID) NULL );
   }

   GDIOBJ_SetOwnership(GdiHandleTable, tempHDC, PsGetCurrentProcess());
   DC_SetOwnership(tempHDC, PsGetCurrentProcess());

   Dc = DC_LockDc(tempHDC);
   if (Dc == NULL)
   {
	  if (hDCRef == NULL)
	  {
          NtGdiDeleteObjectApp(tempHDC);
	  }
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
   }

   if(Description)
   {
      length = wcslen(Description);
      length += wcslen(Description + length + 1);
      length += 3;
      length *= 2;
   }

   MemSize = sizeof(ENHMETAHEADER) + (length + 3) / 4 * 4;

   if (!(Dc->emh = EngAllocMem(FL_ZERO_MEMORY, MemSize, 0)))
   {
       DC_UnlockDc(Dc);
       if (hDCRef == NULL)
       {
           NtGdiDeleteObjectApp(tempHDC);
       }
       SetLastWin32Error(ERROR_INVALID_HANDLE);
       return NULL;
   }

   Dc->emh->iType = EMR_HEADER;
   Dc->emh->nSize = MemSize;

   Dc->emh->rclBounds.left = Dc->emh->rclBounds.top = 0;
   Dc->emh->rclBounds.right = Dc->emh->rclBounds.bottom = -1;

   if(Rect)
   {
      Dc->emh->rclFrame.left   = Rect->left;
      Dc->emh->rclFrame.top    = Rect->top;
      Dc->emh->rclFrame.right  = Rect->right;
      Dc->emh->rclFrame.bottom = Rect->bottom;
   }
   else
   {
      /* Set this to {0,0 - -1,-1} and update it at the end */
      Dc->emh->rclFrame.left = Dc->emh->rclFrame.top = 0;
      Dc->emh->rclFrame.right = Dc->emh->rclFrame.bottom = -1;
   }

   Dc->emh->dSignature = ENHMETA_SIGNATURE;
   Dc->emh->nVersion = 0x10000;
   Dc->emh->nBytes = Dc->emh->nSize;
   Dc->emh->nRecords = 1;
   Dc->emh->nHandles = 1;

   Dc->emh->sReserved = 0; /* According to docs, this is reserved and must be 0 */
   Dc->emh->nDescription = length / 2;

   Dc->emh->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

   Dc->emh->nPalEntries = 0; /* I guess this should start at 0 */

   /* Size in pixels */
   Dc->emh->szlDevice.cx = NtGdiGetDeviceCaps(tempHDC, HORZRES);
   Dc->emh->szlDevice.cy = NtGdiGetDeviceCaps(tempHDC, VERTRES);

   /* Size in millimeters */
   Dc->emh->szlMillimeters.cx = NtGdiGetDeviceCaps(tempHDC, HORZSIZE);
   Dc->emh->szlMillimeters.cy = NtGdiGetDeviceCaps(tempHDC, VERTSIZE);

   /* Size in micrometers */
   Dc->emh->szlMicrometers.cx = Dc->emh->szlMillimeters.cx * 1000;
   Dc->emh->szlMicrometers.cy = Dc->emh->szlMillimeters.cy * 1000;

   if(Description)
   {
      memcpy((char *)Dc->emh + sizeof(ENHMETAHEADER), Description, length);
   }

   ret = tempHDC;
   if (File)
   {
      OBJECT_ATTRIBUTES ObjectAttributes;
      IO_STATUS_BLOCK IoStatusBlock;
      IO_STATUS_BLOCK Iosb;
      UNICODE_STRING NtPathU;
      NTSTATUS Status;
      ULONG FileAttributes = (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY);

      DPRINT1("Trying Create EnhMetaFile\n");

      /* disk based metafile */
      dwDesiredAccess = GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES;

      if (!RtlDosPathNameToNtPathName_U (File, &NtPathU, NULL, NULL))
      {
         DC_UnlockDc(Dc);
         if (hDCRef == NULL)
         {
             NtGdiDeleteObjectApp(tempHDC);
         }
         DPRINT1("Can not Create EnhMetaFile\n");
         SetLastWin32Error(ERROR_PATH_NOT_FOUND);
         return NULL;
      }

      InitializeObjectAttributes(&ObjectAttributes, &NtPathU, 0, NULL, NULL);

      Status = NtCreateFile (&Dc->hFile, dwDesiredAccess, &ObjectAttributes, &IoStatusBlock,
                             NULL, FileAttributes, 0, FILE_OVERWRITE_IF, FILE_NON_DIRECTORY_FILE,
                             NULL, 0);

      RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathU.Buffer);

      if (!NT_SUCCESS(Status))
      {
         Dc->hFile = NULL;
         DC_UnlockDc(Dc);
         if (hDCRef == NULL)
         {
             NtGdiDeleteObjectApp(tempHDC);
         }
         DPRINT1("Create EnhMetaFile fail\n");
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return NULL;
      }

      SetLastWin32Error(IoStatusBlock.Information == FILE_OVERWRITTEN ? ERROR_ALREADY_EXISTS : 0);

      Status = NtWriteFile(Dc->hFile, NULL, NULL, NULL, &Iosb, (PVOID)&Dc->emh, Dc->emh->nSize, NULL, NULL);
      if (Status == STATUS_PENDING)
      {
          Status = NtWaitForSingleObject(Dc->hFile,FALSE,NULL);
          if (NT_SUCCESS(Status))
          {
              Status = Iosb.Status;
          }
      }

      if (NT_SUCCESS(Status))
      {
          ret = tempHDC;
          DC_UnlockDc(Dc);
      }
      else
      {
          Dc->hFile = NULL;
          DPRINT1("Write to EnhMetaFile fail\n");
          SetLastWin32Error(ERROR_CAN_NOT_COMPLETE);
          ret = NULL;
          DC_UnlockDc(Dc);
          if (hDCRef == NULL)
          {
             NtGdiDeleteObjectApp(tempHDC);
          }
      }
    }
    else
    {
      DC_UnlockDc(Dc);
    }

    return ret;
}
#endif



/*
 * @unimplemented
 */
HENHMETAFILE
WINAPI
GetEnhMetaFileA(
	LPCSTR	lpszMetaFile)
{
  NTSTATUS Status;
  LPWSTR lpszMetaFileW;
  HENHMETAFILE rc = 0;

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


/*
 * @unimplemented
 */
UINT
WINAPI
GetEnhMetaFileDescriptionA(
	HENHMETAFILE	hemf,
	UINT		cchBuffer,
	LPSTR		lpszDescription)
{
  NTSTATUS Status;
  LPWSTR lpszDescriptionW;

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

  return 0;
}



/* Unimplemented functions  */

HENHMETAFILE
WINAPI
CopyEnhMetaFileW(
	HENHMETAFILE	hemfSrc,
	LPCWSTR		lpszFile)
{
  UNIMPLEMENTED;
  return 0;
}


HENHMETAFILE
WINAPI
GetEnhMetaFileW(
	LPCWSTR	lpszMetaFile)
{
  UNIMPLEMENTED;
  return 0;
}


UINT
WINAPI
GetEnhMetaFileDescriptionW(
	HENHMETAFILE	hemf,
	UINT		cchBuffer,
	LPWSTR		lpszDescription)
{
  UNIMPLEMENTED;
  return 0;
}


HDC
WINAPI
CreateEnhMetaFileW(
	HDC  hdcRef,
	LPCWSTR  lpFileName,
	LPCRECT  lpRect,
	LPCWSTR  lpDescription)
{
  UNIMPLEMENTED;
  return 0;
}
