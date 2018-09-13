/****************************** Module Header ******************************\
*
* Module Name: rtlres.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Resource Loading Routines
*
* History:
* 05-Apr-1991 ScottLu   Fixed up, resource code is now shared between client
*                       and server, added a few new resource loading routines.
* 24-Sep-1990 MikeKe    From win30
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

HICON IconFromBestImage(
    ICONFILEHEADER *pifh,
    LPNEWHEADER     lpnhSrc,
    int             cxDesired,
    int             cyDesired,
    UINT            LR_flags);

/***************************************************************************\
* LoadStringOrError
*
* NOTE: Passing a NULL value for lpch returns the string length. (WRONG!)
*
* Warning: The return count does not include the terminating NULL WCHAR;
*
* History:
* 05-Apr-1991 ScottLu   Fixed - code is now shared between client and server
* 24-Sep-1990 MikeKe    From Win30
\***************************************************************************/

int LoadStringOrError(
    HANDLE    hModule,
    UINT      wID,
    LPWSTR    lpBuffer,            // Unicode buffer
    int       cchBufferMax,        // cch in Unicode buffer
    WORD      wLangId)
{
    HANDLE hResInfo;
    HANDLE hStringSeg;
    LPTSTR lpsz;
    int    cch;

    /*
     * Make sure the parms are valid.
     */
    if (lpBuffer == NULL) {
        RIPMSG0(RIP_WARNING, "LoadStringOrError: lpBuffer == NULL");
        return 0;
    }


    cch = 0;

    /*
     * String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    if (hResInfo = FINDRESOURCEEXW(hModule, (LPTSTR)ULongToPtr( ((LONG)(((USHORT)wID >> 4) + 1)) ), RT_STRING, wLangId)) {

        /*
         * Load that segment.
         */
        hStringSeg = LOADRESOURCE(hModule, hResInfo);

        /*
         * Lock the resource.
         */
        if (lpsz = (LPTSTR)LOCKRESOURCE(hStringSeg, hModule)) {

            /*
             * Move past the other strings in this segment.
             * (16 strings in a segment -> & 0x0F)
             */
            wID &= 0x0F;
            while (TRUE) {
                cch = *((UTCHAR *)lpsz++);      // PASCAL like string count
                                                // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                    // Step to start if next string
            }

            /*
             * chhBufferMax == 0 means return a pointer to the read-only resource buffer.
             */
            if (cchBufferMax == 0) {
                *(LPTSTR *)lpBuffer = lpsz;
            } else {

                /*
                 * Account for the NULL
                 */
                cchBufferMax--;

                /*
                 * Don't copy more than the max allowed.
                 */
                if (cch > cchBufferMax)
                    cch = cchBufferMax;

                /*
                 * Copy the string into the buffer.
                 */
                RtlCopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));
            }

            /*
             * Unlock resource, but don't free it - better performance this
             * way.
             */
            UNLOCKRESOURCE(hStringSeg, hModule);
        }
    }

    /*
     * Append a NULL.
     */
    if (cchBufferMax != 0) {
        lpBuffer[cch] = 0;
    }

    return cch;
}


/***************************************************************************\
* RtlLoadObjectFromDIBFile
*
* Loads a resource object from file.
*
* 05-Sep-1995 ChrisWil      Created.
\***************************************************************************/

#define BITMAPFILEHEADER_SIZE 14
#define MINHEADERS_SIZE       (BITMAPFILEHEADER_SIZE + sizeof(BITMAPCOREHEADER))

HANDLE RtlLoadObjectFromDIBFile(
    LPCWSTR lpszName,
    LPWSTR  type,
    DWORD   cxDesired,
    DWORD   cyDesired,
    UINT    LR_flags)
{
    FILEINFO fi = { NULL, NULL, NULL };
    HANDLE   hFile;
    HANDLE   hFileMap = NULL;
    HANDLE   hObj     = NULL;
    TCHAR    szFile[MAX_PATH];
    TCHAR    szFile2[MAX_PATH];
    LPWSTR   pszFileDummy;

    if (LR_flags & LR_ENVSUBST) {

        /*
         * Do any %% string substitutions.  We need this feature to handle
         * loading custom cursors and icons from the registry which uses
         * %SystemRoot% in the paths.  It also makes the shell's job
         * easier.
         */
        ExpandEnvironmentStrings(lpszName, szFile2, MAX_PATH);

    } else {

        lstrcpy(szFile2, lpszName);
    }

    if (SearchPath(NULL,         // use default search locations
                   szFile2,      // file name to search for
                   NULL,         // already have file name extension
                   MAX_PATH,     // how big is that buffer, anyway?
                   szFile,       // stick fully qualified path name here
                   &pszFileDummy) == 0) {
        RIPERR0(ERROR_FILE_NOT_FOUND, RIP_VERBOSE, "");
        return NULL;
    }

    /*
     * Open File for reading.
     */
    hFile = CreateFileW(szFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        goto Done;

    /*
     * Create file-mapping for the file in question.
     */
    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFileMap == NULL)
        goto CloseDone;

    /*
     * Map the file into view.
     */
    fi.pFileMap = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

    if (fi.pFileMap == NULL)
        goto CloseDone;

    fi.pFileEnd = fi.pFileMap + GetFileSize(hFile, NULL);
    fi.pFilePtr = fi.pFileMap;
    fi.pszName  = szFile;

    try {
        switch(PTR_TO_ID(type)) {
        case PTR_TO_ID(RT_BITMAP): {

            LPBITMAPFILEHEADER pBFH;
            UPBITMAPINFOHEADER upBIH;
            LPBYTE             lpBits;
            DWORD              cx;
            DWORD              cy;
            WORD               planes;
            WORD               bpp;
            DWORD              cbSizeImage = 0;
            DWORD              cbSizeFile;
            DWORD              cbSizeBits;

            /*
             * Set the BitmapFileHeader and BitmapInfoHeader pointers.
             */
            pBFH  = (LPBITMAPFILEHEADER)fi.pFileMap;
            upBIH = (UPBITMAPINFOHEADER)(fi.pFileMap + BITMAPFILEHEADER_SIZE);

            /*
             * Are we dealing with a bitmap file.
             */
            if (pBFH->bfType != BFT_BITMAP)
                break;

            /*
             * We need to check the filesize against the potential size of
             * the image.  Bad-Bitmaps would otherwise be able to slam us
             * if they lied about the size (and/or) the file is truncated.
             */
            if (upBIH->biSize == sizeof(BITMAPCOREHEADER)) {

                cx     = ((UPBITMAPCOREHEADER)upBIH)->bcWidth;
                cy     = ((UPBITMAPCOREHEADER)upBIH)->bcHeight;
                bpp    = ((UPBITMAPCOREHEADER)upBIH)->bcBitCount;
                planes = ((UPBITMAPCOREHEADER)upBIH)->bcPlanes;

            } else {

                cx     = upBIH->biWidth;
                cy     = upBIH->biHeight;
                bpp    = upBIH->biBitCount;
                planes = upBIH->biPlanes;

                if(upBIH->biSizeImage >= sizeof(BITMAPINFOHEADER))
                cbSizeImage = upBIH->biSizeImage;
            }

            cbSizeFile = (DWORD)(fi.pFileEnd - fi.pFileMap);
            cbSizeBits = BitmapSize(cx, cy, planes, bpp);

            if ((!cbSizeImage && ((cbSizeFile - MINHEADERS_SIZE) < cbSizeBits)) ||
            (cbSizeImage && ((cbSizeFile - MINHEADERS_SIZE) < cbSizeImage))) {

                break;
            }

            /*
             * Get the bits-offset in the file.
             */
            if ((pBFH->bfOffBits >= (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER))) &&
                (pBFH->bfOffBits <= (cbSizeFile - cbSizeImage))) {

                lpBits = ((LPBYTE)upBIH) + pBFH->bfOffBits - sizeof(BITMAPFILEHEADER);

            } else {

                lpBits = NULL;
            }

            /*
             * Convert the dib-on-file to a bitmap-handle.  This can
             * convert both CORE and INFO formats.
             */
            hObj = ConvertDIBBitmap(upBIH,
                                    cxDesired,
                                    cyDesired,
                                    LR_flags,
                                    NULL,
                                    &lpBits);  // use these bits!
        }
        break;

        case PTR_TO_ID(RT_CURSOR):
        case PTR_TO_ID(RT_ICON):
        {
            RTAG           *prtag;
            ICONFILEHEADER *pifh;

            /*
             * Is this a RIFF file?
             */
            prtag = (RTAG *)fi.pFileMap;

            if (prtag->ckID != FOURCC_RIFF) {

                NEWHEADER nh;

                pifh = (ICONFILEHEADER *)fi.pFileMap;

                /*
                 * BUG?: looks like we can load icons as cursors and cursors
                 * as icons.  Does this work?  Is this desired? (SAS)
                 */
                if ((pifh->iReserved != 0) ||
                    ((pifh->iResourceType != IMAGE_ICON) &&
                        (pifh->iResourceType != IMAGE_CURSOR)) ||
                    (pifh->cresIcons < 1))

                    break;

                nh.ResType  = ((type == RT_ICON) ? IMAGE_ICON : IMAGE_CURSOR);
                nh.ResCount = pifh->cresIcons;
                nh.Reserved = 0;

                /*
                 * Get the size of the sucker and meanwhile seek the file pointer
                 * to point at the DIB we want.  Files that have more than one
                 * icon/cursor are treated like a group.  In other words,
                 * each image is treated like an individual element in the res
                 * dir.  So we need to pick the best fit one...
                 */
                hObj = IconFromBestImage(pifh,
                                     &nh,
                                     cxDesired,
                                     cyDesired,
                                     LR_flags);
            } else {

                BOOL fAni;

                hObj = LoadCursorIconFromFileMap(&fi,
                                                 &type,
                                                 cxDesired,
                                                 cyDesired,
                                                 LR_flags,
                                                 &fAni);
                }
            }
        break;

        default:
            UserAssert(FALSE);
            break;
        } // switch
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        hObj = NULL;
    }
CloseDone:

    if (fi.pFileMap != NULL)
        UnmapViewOfFile(fi.pFileMap);

    if (hFileMap)
        CloseHandle(hFileMap);

    if (hFile && (hFile != INVALID_HANDLE_VALUE))
        CloseHandle(hFile);

Done:
#if DBG
    if (hObj == NULL) {
        RIPMSG1(RIP_WARNING,
                "RtlLoadObjectFromDIBFile: Couldn't read resource from %ws",
                lpszName);
    }
#endif

    return hObj;
}

/***************************************************************************\
* IconFromBestImage
*
* Creates HICON from best fitting image in the given file.
*
\***************************************************************************/

HICON IconFromBestImage(
    ICONFILEHEADER  *pifh,
    LPNEWHEADER      lpnhSrc,
    int              cxDesired,
    int              cyDesired,
    UINT             LR_flags)
{
    UINT             iImage;
    UINT             iImageBest;
    LPNEWHEADER      lpnhDst;
    LPRESDIR         lprd;
    LPBYTE           lpRes;
    DWORD            cbDIB;
    HICON            hIcon = NULL;
    IMAGEFILEHEADER *pimh;

    if (lpnhSrc->ResCount > 1) {

        /*
         * First, alloc dummy group resource.
         */
        lpnhDst = (LPNEWHEADER)UserLocalAlloc(0,
                sizeof(NEWHEADER) + (lpnhSrc->ResCount * sizeof(RESDIR)));

        if (lpnhDst == NULL)
            goto Done;

        *lpnhDst = *lpnhSrc;
        lprd = (LPRESDIR)(lpnhDst + 1);

        /*
         * Build up an image directory from the file's image header info.
         */

        for (pimh = pifh->imh, iImage=0;
             iImage < lpnhDst->ResCount;
             iImage++, lprd++, pimh++) {

            /*
             * Fill in RESDIR
             */
            lprd->Icon.Width  = pimh->cx;
            lprd->Icon.Height = pimh->cy;

            if (lpnhDst->ResType == IMAGE_ICON)
                lprd->Icon.ColorCount = pimh->nColors;

            /*
             * NOTE:  These aren't used in the "GetBestImage" process.  So
             * stick in random stuff.
             */
            lprd->Planes     = gpsi->Planes;
            lprd->BitCount   = gpsi->BitCount;
            lprd->BytesInRes = pimh->cbDIB;

            /*
             * Make fake ID:  the index of the image.
             */
            lprd->idIcon = (WORD)iImage;
        }

        /*
         * Find the best image in the group
         */
        iImageBest = LookupIconIdFromDirectoryEx((PBYTE)lpnhDst,
                                                 (lpnhDst->ResType == IMAGE_ICON),
                                                 cxDesired,
                                                 cyDesired,
                                                 LR_flags);
        /*
         * Get rid of fake group resource
         */
        UserLocalFree(lpnhDst);

    } else {
        iImageBest = 0;
    }

    /*
     * Point to selected image.
     */
    pimh  = &pifh->imh[iImageBest];
    cbDIB = pimh->cbDIB;

    /*
     * If we're creating a cursor, we have to whack in HOTSPOT in front
     * Regardless of which type we are making, we need to make sure
     * the resource is aligned.  Thus we always copy.
     */
    if (lpnhSrc->ResType == IMAGE_CURSOR)
        cbDIB += sizeof(POINTS);

    lpRes = (LPBYTE)UserLocalAlloc(0, cbDIB);
    if (lpRes == NULL)
        goto Done;

    if (lpnhSrc->ResType == IMAGE_CURSOR)
        lpRes += sizeof(POINTS);

    RtlCopyMemory(lpRes,
                  ((LPBYTE)pifh) + pimh->offsetDIB,
                  pimh->cbDIB);

    if (lpnhSrc->ResType == IMAGE_CURSOR) {

        lpRes -= sizeof(POINTS);
        ((LPPOINTS)lpRes)->x = pimh->xHotSpot;
        ((LPPOINTS)lpRes)->y = pimh->yHotSpot;
    }

    hIcon = CreateIconFromResourceEx(lpRes,
                                     cbDIB,
                                     (lpnhSrc->ResType == IMAGE_ICON),
                                     0x00030000, // was WIN32VER40
                                     cxDesired,
                                     cyDesired,
                                     LR_flags);

    UserLocalFree(lpRes);

Done:

    return hIcon;
}
