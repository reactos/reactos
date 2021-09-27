#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
BOOL
WINAPI
IsValidEnhMetaRecord(
    PVOID pv0,
    PVOID pv1)
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
IsValidEnhMetaRecordOffExt(
    PVOID pv0,
    PVOID pv1,
    DWORD dwOffset,
    DWORD dwExtends )
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
GdiConvertEnhMetaFile(HENHMETAFILE hemf)
{
    HANDLE hmo = NULL;
    BYTE * Buffer = NULL;
    UINT nSize;

    nSize = GetEnhMetaFileBits( hemf, 0, NULL );
    if (nSize == 0)
        goto Exit;

    // allocate buffer
    Buffer = (BYTE *)LocalAlloc(LPTR, nSize);
    if (Buffer == NULL)
        goto Exit;

    nSize = GetEnhMetaFileBits( hemf, nSize, Buffer );
    if (nSize == 0)
        goto Exit;

    hmo = NtGdiCreateServerMetaFile( GDITAG_TYPE_EMF, nSize, Buffer, 0, 0, 0 );

Exit:
    // clean up
    if (Buffer)
        LocalFree(Buffer);
    return hmo;
}

/*
 * @implemented
 */
HENHMETAFILE
WINAPI
GdiCreateLocalEnhMetaFile(HANDLE hmo)
{
    HENHMETAFILE hEMF;
    BYTE *       Buffer = NULL;
    UINT         nSize;
    DWORD        iType, mm, xExt, yExt;

    nSize = NtGdiGetServerMetaFileBits( hmo, 0, NULL, NULL, NULL, NULL, NULL);
    if (nSize == 0)
        goto Exit;

    // allocate buffer
    Buffer = (BYTE *)LocalAlloc(LPTR, nSize);
    if (Buffer == NULL)
        goto Exit;

    // store to buffer
    nSize = NtGdiGetServerMetaFileBits( hmo, nSize, Buffer, &iType, &mm, &xExt, &yExt);
    if (nSize == 0)
        goto Exit;

    if ( iType == GDITAG_TYPE_MFP ) // handle conversion to EMF
    {
        METAFILEPICT Info;

        Info.hMF  = NULL;
        Info.mm   = mm;
        Info.xExt = xExt;
        Info.yExt = yExt;

        hEMF = SetWinMetaFileBits( nSize, Buffer, NULL, &Info ); // Translate from old style to new style.
        if (hEMF == NULL)
            goto Exit;
    }
    else
    {
        hEMF = SetEnhMetaFileBits(nSize, Buffer);
        if (hEMF == NULL)
            goto Exit;
    }

Exit:
    // clean up
    if (Buffer)
        LocalFree(Buffer);
    return hEMF;
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiComment(
    _In_ HDC hdc,
    _In_ UINT cbSize,
    _In_ const BYTE *lpData)
{
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_ALTDC_TYPE)
        return TRUE;

    return EMFDC_GdiComment( hdc, cbSize, lpData );
}

/*
 * @implemented
 */
UINT
WINAPI
GetEnhMetaFilePixelFormat(
    HENHMETAFILE hemf,
    UINT cbBuffer,
    PIXELFORMATDESCRIPTOR *ppfd )
{
    ENHMETAHEADER pemh;

    if(GetEnhMetaFileHeader(hemf, sizeof(ENHMETAHEADER), &pemh))
    {
        if(pemh.bOpenGL)
        {
            if(pemh.cbPixelFormat)
            {
                memcpy((void*)ppfd, UlongToPtr(pemh.offPixelFormat), cbBuffer );
                return(pemh.cbPixelFormat);
            }
        }
    }
    return(0);
}
