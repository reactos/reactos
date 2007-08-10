#include "precomp.h"

/* DEFINES *******************************************************************/


/* PRIVATE DATA **************************************************************/

PMF_ENTRY hMF_List = NULL;
DWORD hMFCount = 0;

/* INTERNAL FUNCTIONS ********************************************************/

BOOL
MF_CreateMFDC ( HGDIOBJ hMDC, 
                PMETAFILEDC pmfDC )
{
  PMF_ENTRY pMFME;
  
  pMFME = LocalAlloc(LMEM_ZEROINIT, sizeof(MF_ENTRY));
  if (!pMFME)
  {
    return FALSE;
  }
  
  if (hMF_List == NULL)
  {
    hMF_List = pMFME;
    InitializeListHead(&hMF_List->List);
  }
  else
    InsertTailList(&hMF_List->List, &pMFME->List);

  pMFME->hmDC  = hMDC;
  pMFME->pmfDC = pmfDC;
    
  hMFCount++;
  return TRUE;
}


PMETAFILEDC
MF_GetMFDC ( HGDIOBJ hMDC )
{
  PMF_ENTRY pMFME = hMF_List;

  do
  {
    if ( pMFME->hmDC == hMDC ) return pMFME->pmfDC;
    pMFME = (PMF_ENTRY) pMFME->List.Flink;
  }
  while ( pMFME != hMF_List );

  return NULL;
}


BOOL
MF_DeleteMFDC ( HGDIOBJ hMDC )
{
  PMF_ENTRY pMFME = hMF_List;

  do
  {
    if ( pMFME->hmDC == hMDC)
    {
       RemoveEntryList(&pMFME->List);
       LocalFree ( pMFME );
       hMFCount--;
       if (!hMFCount) hMF_List = NULL;
       return TRUE;
    }
    pMFME = (PMF_ENTRY) pMFME->List.Flink;
  }
  while ( pMFME != hMF_List );

  return FALSE;
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
HMETAFILE
STDCALL
CloseMetaFile(
	HDC	a0
	)
{
	return 0;
}


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
    rc = CopyMetaFileW( hmfSrc, lpszFileW );
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
  HANDLE hFile;
  HDC hmDC;
  PMETAFILEDC pmfDC = LocalAlloc(LMEM_ZEROINIT, sizeof(METAFILEDC));
  if (!pmfDC) return NULL;
  
  pmfDC->mh.mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
  pmfDC->mh.mtVersion      = 0x0300;
  pmfDC->mh.mtSize         = pmfDC->mh.mtHeaderSize;

  if (lpszFile)  /* disk based metafile */
  {
    pmfDC->mh.mtType = METAFILE_DISK;

    if(!GetFullPathName(  lpszFile,
                          MAX_PATH,
         (LPTSTR) &pmfDC->Filename,
               (LPTSTR*) &lpszFile))
    {
//       MFDRV_DeleteDC( dc->physDev );
       return NULL;
    }

    if ((hFile = CreateFileW(pmfDC->Filename, GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
    {
//       MFDRV_DeleteDC( dc->physDev );
       return NULL;
    }

    if (!WriteFile( hFile, &pmfDC->mh, sizeof(pmfDC->mh), NULL, NULL ))
    {
//       MFDRV_DeleteDC( dc->physDev );
       return NULL;
    }
      pmfDC->hFile = hFile; 
  }
  else  /* memory based metafile */
    pmfDC->mh.mtType = METAFILE_MEMORY;

  hmDC = NtGdiCreateClientObj ( GDI_OBJECT_TYPE_METADC );

  MF_CreateMFDC ( hmDC, pmfDC );

  return hmDC;
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
      rc = CreateMetaFileW( lpszFileW );
      HEAP_free ( lpszFileW );
    }
  return rc;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteMetaFile(
	HMETAFILE	a0
	)
{
	return FALSE;
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
      rc = GetMetaFileW( lpszMetaFileW );
      HEAP_free ( lpszMetaFileW );
    }

  return rc;
}


