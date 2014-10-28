#include <precomp.h>
#include <debug.h>

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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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

    pmfDC->hPen = GetStockObject(BLACK_PEN);
    pmfDC->hBrush = GetStockObject(WHITE_BRUSH);
    pmfDC->hFont = GetStockObject(DEVICE_DEFAULT_FONT);
    pmfDC->hBitmap = GetStockObject(DEFAULT_BITMAP);
    pmfDC->hPalette = GetStockObject(DEFAULT_PALETTE);

    if (lpszFile)  /* disk based metafile */
    {
        pmfDC->mh.mtType = METAFILE_DISK;

        if(!GetFullPathName(  lpszFile,
                              MAX_PATH,
                              (LPTSTR) &pmfDC->Filename,
                              (LPTSTR*) &lpszFile))
        {
            LocalFree(pmfDC);
            return NULL;
        }

        if ((hFile = CreateFileW(pmfDC->Filename, GENERIC_WRITE, 0, NULL,
                                 CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
        {
            LocalFree(pmfDC);
            return NULL;
        }

        if (!WriteFile( hFile, &pmfDC->mh, sizeof(pmfDC->mh), &pmfDC->dwWritten, NULL ))
        {
            LocalFree(pmfDC);
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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


/*
 * @unimplemented
 */
UINT
WINAPI
GetMetaFileBitsEx(
    HMETAFILE	a0,
    UINT		a1,
    LPVOID		a2
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HMETAFILE
WINAPI
SetMetaFileBitsEx(
    UINT		size,
    CONST BYTE	*lpData
)
{
    const METAHEADER *mh_in = (const METAHEADER *)lpData;

    if (size & 1) return 0;

    if (!size || mh_in->mtType != METAFILE_MEMORY || mh_in->mtVersion != 0x300 ||
            mh_in->mtHeaderSize != sizeof(METAHEADER) / 2)
    {
        DPRINT1("SetMetaFileBitsEx failed: %lu,%lu,0x&lx,%lu\n",
                size, mh_in->mtType, mh_in->mtVersion, mh_in->mtHeaderSize);
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }

    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsPlayMetafileDC(HDC hDC)
{
    PLDC pLDC = GdiGetLDC(hDC);
    if ( pLDC )
    {
        if ( pLDC->Flags & LDC_PLAY_MFDC ) return TRUE;
    }
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
PlayMetaFile(
    HDC		a0,
    HMETAFILE	a1
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
PlayMetaFileRecord(
    HDC		a0,
    LPHANDLETABLE	a1,
    LPMETARECORD	a2,
    UINT		a3
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
EnumMetaFile(
    HDC			a0,
    HMETAFILE		a1,
    MFENUMPROC		a2,
    LPARAM			a3
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetWinMetaFileBits(
    HENHMETAFILE	a0,
    UINT		a1,
    LPBYTE		a2,
    INT		a3,
    HDC		a4
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HENHMETAFILE
WINAPI
SetWinMetaFileBits(
    UINT			a0,
    CONST BYTE		*a1,
    HDC			a2,
    CONST METAFILEPICT	*a3)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsMetaFileDC(HDC hDC)
{
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return TRUE;
        else
        {
            PLDC pLDC = GdiGetLDC(hDC);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if ( pLDC->iType == LDC_EMFLDC) return TRUE;
        }
    }
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsMetaPrintDC(HDC hDC)
{

    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return FALSE;
        else
        {
            PLDC pLDC = GdiGetLDC(hDC);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if ( pLDC->Flags & LDC_META_PRINT) return TRUE;
        }
    }
    return FALSE;
}

/*
 * @unimplemented
 */
METAFILEPICT *
WINAPI
GdiCreateLocalMetaFilePict(HENHMETAFILE hmo)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
GdiConvertMetaFilePict(HGLOBAL hMem)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
