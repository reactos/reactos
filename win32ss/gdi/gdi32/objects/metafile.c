/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            win32ss/gdi/gdi32/objects/metafile.c
 * PURPOSE:         metafile and enhanced metafile support
 * PROGRAMMERS:     Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <precomp.h>

#define NDEBUG
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
GdiCreateLocalMetaFilePict(HANDLE hmo)
{
    HGLOBAL         hMetaFilePict;
    METAFILEPICT *  pInfo;
    HMETAFILE       hMF = NULL;
    BYTE *          Buffer = NULL;
    BYTE *          BufNew = NULL;
    HDC             hDC = NULL;
    UINT            nSize, cSize;
    DWORD           iType;

    // NOTE: On Win32, there is no difference between the local heap and
    //       the global heap. GlobalAlloc and LocalAlloc have same effect.

    // allocate for METAFILEPICT
    hMetaFilePict = GlobalAlloc(GHND | GMEM_SHARE, sizeof(METAFILEPICT));
    pInfo = (METAFILEPICT *)GlobalLock(hMetaFilePict);
    if (pInfo == NULL)
        goto Exit;

    nSize = NtGdiGetServerMetaFileBits( hmo, 0, NULL, NULL, NULL, NULL, NULL );

    // allocate buffer
    Buffer = (BYTE *)LocalAlloc(LPTR, nSize);
    if (Buffer == NULL)
        goto Exit;

    // store to buffer
    nSize = NtGdiGetServerMetaFileBits( hmo, nSize, Buffer, &iType, (PDWORD)&pInfo->mm, (PDWORD)&pInfo->xExt, (PDWORD)&pInfo->yExt );
    if (nSize == 0)
        goto Exit;

    if ( iType == GDITAG_TYPE_EMF ) // handle conversion to MFP
    {
        static const WCHAR szDisplayW[] = { 'D','I','S','P','L','A','Y','\0' };
        HENHMETAFILE hEMF;
        ENHMETAHEADER emh;

        hEMF = SetEnhMetaFileBits(nSize, Buffer);
        if (hEMF == NULL)
            goto Exit;

        if (!GetEnhMetaFileHeader( hEMF, sizeof(emh), &emh ))
        {
            DeleteEnhMetaFile(hEMF);
            goto Exit;
        }

        pInfo->mm   = MM_ANISOTROPIC; // wine uses MM_ISOTROPIC.
        pInfo->xExt = emh.rclFrame.right   - emh.rclFrame.left; // Width
        pInfo->yExt = emh.rclFrame.bottom  - emh.rclFrame.top;  // Height

        hDC = CreateDCW(szDisplayW, NULL, NULL, NULL);
        if (hDC)
        {
            cSize = GetWinMetaFileBits( hEMF, 0, NULL, MM_ANISOTROPIC, hDC );
            if (cSize)
            {
                BufNew = (BYTE *)LocalAlloc(LPTR, cSize);
                if (BufNew)
                {
                    nSize = GetWinMetaFileBits( hEMF, cSize, (LPBYTE)BufNew, MM_ANISOTROPIC, hDC );
                    if (nSize == cSize)
                    {
                        if (Buffer) LocalFree(Buffer);
                        Buffer = BufNew;
                    }
                }
            }
            DeleteDC(hDC);
        }
        DeleteEnhMetaFile(hEMF);

        if (Buffer != BufNew)
            goto Exit;
    }

    // create metafile from buffer
    hMF = SetMetaFileBitsEx(nSize, Buffer);
    if (hMF == NULL)
        goto Exit;

    // set metafile handle
    pInfo->hMF = hMF;

Exit:
    // clean up
    if (Buffer)
        LocalFree(Buffer);
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
HANDLE
WINAPI
GdiConvertMetaFilePict(HGLOBAL hMetaFilePict)
{
    HMETAFILE       hMF;
    UINT            nSize;
    HANDLE          hmo    = NULL;
    BYTE *          Buffer = NULL;
    METAFILEPICT *  pInfo  = NULL;

    // get METAFILEPICT pointer
    pInfo = (METAFILEPICT *)GlobalLock(hMetaFilePict);
    if (pInfo == NULL)
        goto Exit;

    // get metafile handle
    hMF = pInfo->hMF;

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

    hmo = NtGdiCreateServerMetaFile( GDITAG_TYPE_MFP, nSize, Buffer, pInfo->mm, pInfo->xExt, pInfo->yExt);

Exit:
    // clean up
    if (pInfo)
        GlobalUnlock(hMetaFilePict);
    if (Buffer)
        LocalFree(Buffer);
    return hmo;    // success if non-NULL
}
