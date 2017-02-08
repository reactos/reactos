#include <precomp.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsPlayMetafileDC(HDC hDC)
{
#if 0
    PLDC pLDC = GdiGetLDC(hDC);
    if ( pLDC )
    {
        if ( pLDC->Flags & LDC_PLAY_MFDC ) return TRUE;
    }
    return FALSE;
#else
    UNIMPLEMENTED;
    return FALSE;
#endif
}

/*
 * @implemented
 */
BOOL
WINAPI
GdiIsMetaFileDC(HDC hdc)
{
    ULONG ulObjType;

    ulObjType = GDI_HANDLE_GET_TYPE(hdc);
    if (ulObjType == GDILoObjType_LO_METADC16_TYPE)
    {
        return TRUE;
    }

    if (ulObjType == GDILoObjType_LO_ALTDC_TYPE)
    {
#if 0
        PLDC pLDC = GdiGetLDC(hdc);
        if ( !pLDC )
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
        if ( pLDC->iType == LDC_EMFLDC) return TRUE;
        return FALSE;
#endif
        return TRUE;
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
#if 0
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
#else
    UNIMPLEMENTED;
    return FALSE;
#endif
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
