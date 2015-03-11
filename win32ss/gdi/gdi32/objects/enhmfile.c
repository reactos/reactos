#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
DWORD
WINAPI
IsValidEnhMetaRecord(
    DWORD	a0,
    DWORD	a1
)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;

}

/*
 * @unimplemented
 */
DWORD
WINAPI
IsValidEnhMetaRecordOffExt(
    DWORD	a0,
    DWORD	a1,
    DWORD	a2,
    DWORD	a3
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
GdiConvertEnhMetaFile(HENHMETAFILE hmf)
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
GdiCreateLocalEnhMetaFile(HENHMETAFILE hmo)
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
GdiComment(
    _In_ HDC hdc,
    _In_ UINT cbSize,
    _In_ const BYTE *lpData)
{
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_ALTDC_TYPE)
        return TRUE;

    HANDLE_METADC(BOOL, GdiComment, FALSE, hdc, cbSize, lpData);

    return TRUE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetEnhMetaFilePixelFormat(
    HENHMETAFILE			hemf,
    UINT				cbBuffer,
    PIXELFORMATDESCRIPTOR	*ppfd
)
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
