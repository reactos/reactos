/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            win32ss/gdi/gdi32/objects/metafile.c
 * PURPOSE:         metafile and enhanced metafile support
 * PROGRAMMERS:     Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
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

// NOTE: I wanna use GdiCreateLocalMetaFilePict and GdiConvertMetaFilePict
//       functions for clipboard data conversion. --- katahiromz

/*
 * @implemented
 */
HGLOBAL
WINAPI
GdiCreateLocalMetaFilePict(HENHMETAFILE hEMF)
{
    HGLOBAL         hMetaFilePict;
    METAFILEPICT *  pInfo;
    HMETAFILE       hMF = NULL;
    BYTE *          Buffer = NULL;
    HDC             hDC = NULL;
    UINT            nSize;

    // NOTE: On Win32, there is no difference between the local heap and
    //       the global heap. GlobalAlloc and LocalAlloc have same effect.

    // allocate for METAFILEPICT
    hMetaFilePict = GlobalAlloc(GHND | GMEM_SHARE, sizeof(METAFILEPICT));
    pInfo = (METAFILEPICT *)GlobalLock(hMetaFilePict);
    if (pInfo == NULL)
        goto Exit;

    // create DC
    hDC = CreateCompatibleDC(NULL);
    if (hDC == NULL)
        goto Exit;

    // get size of dest buffer
    nSize = GetWinMetaFileBits(hEMF, 0, NULL, MM_ANISOTROPIC, hDC);
    if (nSize == 0)
        goto Exit;

    // allocate buffer
    Buffer = (BYTE *)LocalAlloc(LPTR, nSize);
    if (Buffer == NULL)
        goto Exit;

    // store to buffer
    nSize = GetWinMetaFileBits(hEMF, nSize, Buffer, MM_ANISOTROPIC, hDC);
    if (nSize == 0)
        goto Exit;

    // create metafile from buffer
    hMF = SetMetaFileBitsEx(nSize, Buffer);
    if (hMF == NULL)
        goto Exit;

    // no suggested size is supplied
    pInfo->mm = MM_ANISOTROPIC;
    pInfo->xExt = 0;
    pInfo->yExt = 0;

    // set metafile handle
    pInfo->hMF = hMF;

Exit:
    // clean up
    if (Buffer)
        LocalFree(Buffer);
    if (hDC)
        DeleteDC(hDC);
    if (pInfo)
        GlobalUnlock(hMetaFilePict);
    if (hMF == NULL)
    {
        // failure
        GlobalFree(hMetaFilePict);
        hMetaFilePict = NULL;
    }

    return hMetaFilePict;   // success if non-NULL
}

/*
 * @implemented
 */
HENHMETAFILE
WINAPI
GdiConvertMetaFilePict(HGLOBAL hMetaFilePict)
{
    HMETAFILE       hMF;
    UINT            nSize;
    HENHMETAFILE    hEMF    = NULL;
    BYTE *          Buffer  = NULL;
    HDC             hDC     = NULL;
    METAFILEPICT *  pInfo   = NULL;

    // get METAFILEPICT pointer
    pInfo = (METAFILEPICT *)GlobalLock(hMetaFilePict);
    if (pInfo == NULL)
        goto Exit;

    // get metafile handle
    hMF = pInfo->hMF;

    // Missing test for GDILoObjType_LO_METADC16_TYPE (hMF)

    // get size of buffer
    nSize = GetMetaFileBitsEx(hMF, 0, NULL);
    if (nSize == 0)
        goto Exit;

    // allocate buffer
    Buffer = (BYTE *)LocalAlloc(LPTR, nSize);
    if (Buffer == NULL)
        goto Exit;

    // store to buffer
    nSize = GetMetaFileBitsEx(hMF, nSize, Buffer);
    if (nSize == 0)
        goto Exit;

    // create DC
    hDC = CreateCompatibleDC(NULL);
    if (hDC == NULL)
        goto Exit;

    // create enhanced metafile from buffer
    hEMF = SetWinMetaFileBits(nSize, Buffer, hDC, pInfo);

Exit:
    // clean up
    if (pInfo)
        GlobalUnlock(hMetaFilePict);
    if (hDC)
        DeleteDC(hDC);
    if (Buffer)
        LocalFree(Buffer);
    return hEMF;    // success if non-NULL
}
