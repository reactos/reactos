/****************************** Module Header ******************************\
* Module Name: acons.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains code for dealing with animated icons/cursors.
*
* History:
* 10-02-91 DarrinM      Created.
* 07-30-92 DarrinM      Unicodized.
* 11-28-94 JimA         Moved to client from server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Resource Directory format for IconEditor generated icon and cursor
 * (.ICO & .CUR) files.  All fields are shared except xHotspot and yHotspot
 * which are only valid for cursors.
 */
typedef struct _ICONFILERESDIR {    // ird
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} ICONFILERESDIR;

typedef struct _HOTSPOTREC {    // hs
    WORD xHotspot;
    WORD yHotspot;
} HOTSPOTREC;

PCURSORRESOURCE ReadIconGuts(
    IN  PFILEINFO   pfi,
    IN  LPNEWHEADER pnhBase,
    IN  int         offResBase,
    OUT LPWSTR     *prt,
    IN  int         cxDesired,
    IN  int         cyDesired,
    IN  DWORD       LR_flags);

BOOL ReadTag(
    IN  PFILEINFO pfi,
    OUT PRTAG     ptag);

BOOL ReadChunk(
    IN  PFILEINFO pfi,
    IN      PRTAG ptag,
    OUT     PVOID pv);

BOOL SkipChunk(
    IN PFILEINFO pfi,
    IN PRTAG     ptag);

HICON CreateAniIcon(
    LPCWSTR pszName,
    LPWSTR rt,
    int cicur,
    DWORD *aicur,
    int cpcur,
    HCURSOR *ahcur,
    JIF jifRate,
    PJIF ajifRate,
    BOOL fPublic);

HCURSOR ReadIconFromFileMap(
    IN PFILEINFO   pfi,
    IN int         cbSize,
    IN DWORD       cxDesired,
    IN DWORD       cyDesired,
    IN DWORD       LR_flags);

HICON LoadAniIcon(
    IN PFILEINFO pfi,
    IN LPWSTR    rt,
    IN DWORD     cxDesired,
    IN DWORD     cyDesired,
    IN DWORD     LR_flags);



/***************************************************************************\
* LoadCursorFromFile (API)
*
* Called by SetSystemCursor.
*
* History:
* 08-03-92 DarrinM      Created.
\***************************************************************************/

HCURSOR WINAPI LoadCursorFromFileW(
    LPCWSTR pszFilename)
{
    return(LoadImage(NULL,
                     pszFilename,
                     IMAGE_CURSOR,
                     0,
                     0,
                     LR_DEFAULTSIZE | LR_LOADFROMFILE));
}



/***********************************************************************\
* LoadCursorFromFileA
*
* Returns: hCursor
*
* 10/9/1995 Created SanfordS
\***********************************************************************/

HCURSOR WINAPI LoadCursorFromFileA(
    LPCSTR pszFilename)
{
    LPWSTR lpUniName;
    HCURSOR hcur;

    if (pszFilename == NULL ||
            !MBToWCS(pszFilename, -1, &lpUniName, -1, TRUE))
        return (HANDLE)NULL;

    hcur = LoadCursorFromFileW(lpUniName);

    UserLocalFree(lpUniName);

    return hcur;
}



/***********************************************************************\
* ReadFilePtr
*
* Works like ReadFile but with pointers to a mapped file buffer.
*
* Returns:
*
* 11/16/1995 Created SanfordS
\***********************************************************************/
BOOL ReadFilePtr(
    IN  PFILEINFO pfi,
    OUT LPVOID   *ppBuf,
    IN  DWORD     cb)
{
    *ppBuf = pfi->pFilePtr;
    pfi->pFilePtr += cb;
    return (pfi->pFilePtr <= pfi->pFileEnd);
}



/***********************************************************************\
* ReadFilePtrUnaligned
*
* Works like ReadFile but with pointers to a mapped file buffer.
*
* Returns:
*
* 11/16/1995 Created SanfordS
\***********************************************************************/
BOOL ReadFilePtrUnaligned(
    IN  PFILEINFO pfi,
    OUT VOID UNALIGNED **ppBuf,
    IN  DWORD     cb)
{
    *ppBuf = pfi->pFilePtr;
    pfi->pFilePtr += cb;
    return (pfi->pFilePtr <= pfi->pFileEnd);
}



/***********************************************************************\
* ReadFilePtrCopy
*
* Works even more like ReadFile in that is copies data to the given buffer.
*
* Returns:
*
* 11/16/1995 Created SanfordS
\***********************************************************************/
BOOL ReadFilePtrCopy(
    IN     PFILEINFO pfi,
    IN OUT LPVOID pBuf,
    IN     DWORD cb)
{
    if (pfi->pFilePtr + cb > pfi->pFileEnd) {
        return(FALSE);
    }
    RtlCopyMemory(pBuf, pfi->pFilePtr, cb);
    pfi->pFilePtr += cb;
    return TRUE;
}



/***************************************************************************\
* ReadTag, ReadChunk, SkipChunk
*
* Some handy functions for reading RIFF files.
*
* History:
* 10-02-91 DarrinM      Created.
* 03-25-93 Jonpa        Changed to use RIFF format instead of ASDF
\***************************************************************************/
BOOL ReadTag(
    IN  PFILEINFO pfi,
    OUT PRTAG     ptag)
{
    ptag->ckID = ptag->ckSize = 0L;  // in case we fail the read.

    return(ReadFilePtrCopy(pfi, ptag, sizeof(RTAG)));
}



BOOL ReadChunk(
    IN  PFILEINFO pfi,
    IN  PRTAG     ptag,
    OUT PVOID     pv)
{
    if (!ReadFilePtrCopy(pfi, pv, ptag->ckSize))
        return FALSE;

    /* WORD align file pointer */
    if( ptag->ckSize & 1 )
        pfi->pFilePtr++;


    if (pfi->pFilePtr <= pfi->pFileEnd) {
        return TRUE;
    } else {
        RIPMSG0(RIP_WARNING, "ReadChunk: Advanced pointer past end of file map");
        return FALSE;
    }
}



BOOL SkipChunk(
    IN PFILEINFO pfi,
    IN PRTAG     ptag)
{
    /*
     * Round ptag->ckSize up to nearest word boundary
     * to maintain alignment
     */
    pfi->pFilePtr += (ptag->ckSize + 1) & (~1);
    if (pfi->pFilePtr <= pfi->pFileEnd) {
        return TRUE;
    } else {
        RIPMSG0(RIP_WARNING, "SkipChunk: Advanced pointer past end of file map");
        return FALSE;
    }
}



/***************************************************************************\
* LoadCursorIconFromFileMap
*
* If pszName is one of the IDC_* values then we use WIN.INI to find a
* custom cursor/icon.  Otherwise, pszName points to a filename of a .ICO/.CUR
* file to be loaded.  If the file is an .ANI file containing a multiframe
* animation then LoadAniIcon is called to create an ACON.  Otherwise if
* the file is an .ANI file containing just a single frame then it is loaded
* and a normal CURSOR/ICON resource is created from it.
*
* 12-26-91 DarrinM      Wrote it.
* 03-17-93 JonPa        Changed to use RIFF format for ani-cursors
* 11/16/1995 SanfordS   Added LR_flags support
\***************************************************************************/

HANDLE LoadCursorIconFromFileMap(
    IN PFILEINFO   pfi,
    IN OUT LPWSTR *prt,
    IN DWORD       cxDesired,
    IN DWORD       cyDesired,
    IN DWORD       LR_flags,
    OUT LPBOOL     pfAni)
{
    LPNEWHEADER pnh;
    int offResBase;

    *pfAni = FALSE;
    offResBase = 0;

    /*
     * Determine if this is an .ICO/.CUR file or an .ANI file.
     */
    pnh = (LPNEWHEADER)pfi->pFileMap;
    if (*(LPDWORD)pnh == FOURCC_RIFF) {

        RTAG tag;

        /*
         * It's an ANICURSOR!
         * Seek back to beginning + 1 tag.
         */
        pfi->pFilePtr = pfi->pFileMap + sizeof(tag);

        /* check RIFF type for ACON */
        if (*(LPDWORD)pfi->pFilePtr != FOURCC_ACON) {
            return NULL;
        }
        pfi->pFilePtr += sizeof(DWORD);
        if (pfi->pFilePtr > pfi->pFileEnd) {
            return NULL;
        }

        /*
         * Ok, we have a ACON chunk.  Find the first ICON chunk and set
         * things up so it looks we've just loaded the header of a normal
         * .CUR file, then fall into the .CUR bits handling code below.
         */
        while (ReadTag(pfi, &tag)) {
            /*
             * Handle each chunk type.
             */
            if (tag.ckID == FOURCC_anih) {

                ANIHEADER anih;

                if (!ReadChunk(pfi, &tag, &anih)) {
                    return NULL;
                }

                if (!(anih.fl & AF_ICON) || (anih.cFrames == 0)) {
                    return NULL;
                }

                // If this ACON has more than one frame then go ahead
                // and create an ACON, otherwise just use the first
                // frame to create a normal ICON/CURSOR.

                if (anih.cFrames > 1) {

                    *pfAni = TRUE;
                    *prt = RT_CURSOR;
                    return(LoadAniIcon(pfi,
                                       RT_CURSOR,
                                       cxDesired,
                                       cyDesired,
                                       LR_flags));
                }

            } else if (tag.ckID == FOURCC_LIST) {
                LPDWORD pdwType = NULL;
                BOOL fOK = FALSE;
                /*
                 * If this is the fram list, then get the first icon out of it
                 */

                /* check LIST type for fram */

                if( tag.ckSize >= sizeof(DWORD) &&
                        (fOK = ReadFilePtr( pfi,
                                            &pdwType,
                                            sizeof(DWORD))) &&
                        *pdwType == FOURCC_fram) {

                    if (!ReadTag(pfi, &tag)) {
                        return NULL;
                    }

                    if (tag.ckID == FOURCC_icon) {
                        /*
                         * We've found what we're looking for.  Get current position
                         * in file to be used as the base from which the icon data
                         * offsets are offset from.
                         */
                        offResBase = (int)(pfi->pFilePtr - pfi->pFileMap);

                        /*
                         * Grab the header first, since the following code assumes
                         * it was read above.
                         */
                        ReadFilePtr(pfi, &pnh, sizeof(NEWHEADER));

                        /*
                         * Break out and let the icon loading/cursor creating code
                         * take it from here.
                         */
                        break;
                    } else {
                        SkipChunk(pfi, &tag);
                    }
                } else {
                    /*
                     * Something bad happened in the type read, if it was
                     * a file error then close and exit, otherwise just
                     * skip the rest of the chunk
                     */
                    if(!fOK) {
                        return NULL;
                    }
                    /*
                     * take the type we just read out of the tag size and
                     * skip the rest
                     */
                    tag.ckSize -= sizeof(DWORD);
                    SkipChunk(pfi, &tag);
                }
            } else {
                /*
                 * We're not interested in this chunk, skip it.
                 */
                SkipChunk(pfi, &tag);
            }
        }
    } else { // not a RIFF file.
        if ((pnh->ResType != FT_ICON) && (pnh->ResType != FT_CURSOR)) {
            return NULL;
        }
    }
    {
        PCURSORRESOURCE pcres;

        pcres = ReadIconGuts(pfi,
                             pnh,
                             offResBase,
                             prt,
                             cxDesired,
                             cyDesired,
                             LR_flags);

        return ConvertDIBIcon((LPBITMAPINFOHEADER)pcres,
                              NULL,
                              pfi->pszName,
                              *prt == RT_ICON,
                              cxDesired,
                              cyDesired,
                              LR_flags);
    }
}


/***********************************************************************\
* ReadIconGuts
*
* Returns: a pointer to a locally allocated buffer extraced from the
*          given file that looks like a icon/acon resource.
*          Also returns the type of the icon (RT_ICON or RT_CURSOR)
*
*
* 8/23/1995 SanfordS   Documented
* 11/16/1995 SanfordS  Added LR_flags support
\***********************************************************************/
PCURSORRESOURCE ReadIconGuts(
    IN  PFILEINFO  pfi,
    IN  NEWHEADER *pnhBase,
    IN  int        offResBase,
    OUT LPWSTR    *prt,
    IN  int        cxDesired,
    IN  int        cyDesired,
    IN  DWORD      LR_flags)
{
    NEWHEADER *pnh;
    int i, Id;
    ICONFILERESDIR UNALIGNED *pird;
    PCURSORRESOURCE pcres;
    RESDIR UNALIGNED *prd;
    DWORD cb;
    HOTSPOTREC UNALIGNED *phs;
    LPBITMAPINFOHEADER pbih;

    /*
     * Construct a fake array of RESDIR entries using the info at the head
     * of the file.  Store the data offset in the idIcon WORD so it can be
     * returned by RtlGetIdFromDirectory.
     */
    pnh = (NEWHEADER *)UserLocalAlloc(0, sizeof(NEWHEADER) +
            (pnhBase->ResCount * (sizeof(RESDIR) + sizeof(HOTSPOTREC))));
    if (pnh == NULL)
        return NULL;

    *pnh = *pnhBase;
    prd = (RESDIR UNALIGNED *)(pnh + 1);
    phs = (HOTSPOTREC UNALIGNED *)(prd + pnhBase->ResCount);

    for (i = 0; i < (int)pnh->ResCount; i++, prd++) {
        /*
         * Read the resource directory from the icon file.
         */
        ReadFilePtrUnaligned(pfi, &pird, sizeof(ICONFILERESDIR));

        /*
         * Convert from the icon editor's resource directory format
         * to the post-RC.EXE format LookupIconIdFromDirectory expects.
         */
        prd->Icon.Width = pird->bWidth;
        prd->Icon.Height = pird->bHeight;
        if (pnh->ResType == FT_ICON) {     // ICON
            prd->Icon.ColorCount = pird->bColorCount;
            prd->Icon.reserved = 0;
        }
        prd->Planes = 0;                // Hopefully nobody uses this
        prd->BitCount = 0;              //        "        "
        prd->BytesInRes = pird->dwDIBSize;
        prd->idIcon = (WORD)pird->dwDIBOffset;

        phs->xHotspot = pird->xHotspot;
        phs->yHotspot = pird->yHotspot;
        phs++;
    }

    *prt = pnhBase->ResType == FT_ICON ? RT_ICON : RT_CURSOR;
    Id = RtlGetIdFromDirectory((PBYTE)pnh,
                                *prt == RT_ICON,
                                cxDesired,
                                cyDesired,
                                LR_flags,
                                &cb);

    /*
     * Allocate for worst case (cursor).
     */
    pcres = (PCURSORRESOURCE)UserLocalAlloc(0,
            cb + FIELD_OFFSET(CURSORRESOURCE, bih));
    if (pcres == NULL) {
        goto CleanExit;
    }

    if (*prt == RT_CURSOR) {
        /*
         * Fill in hotspot info for cursors.
         */
        prd = (RESDIR UNALIGNED *)(pnh + 1);
        phs = (HOTSPOTREC UNALIGNED *)(prd + pnh->ResCount);

        for( i = 0; i < pnh->ResCount; i++ ) {
            if (prd[i].idIcon == (WORD)Id) {
                pcres->xHotspot = phs[i].xHotspot;
                pcres->yHotspot = phs[i].yHotspot;
                break;
            }
        }

        if (i == pnh->ResCount) {
            pcres->xHotspot = pird->xHotspot;
            pcres->yHotspot = pird->yHotspot;
        }
        pbih = &pcres->bih;
    } else {
        pbih = (LPBITMAPINFOHEADER)pcres;
    }

    /*
     * Read in the header information into pcres.
     */
    pfi->pFilePtr = pfi->pFileMap + offResBase + Id;
    if (!ReadFilePtrCopy(pfi, pbih, cb)) {
        UserLocalFree(pnh);
        UserLocalFree(pcres);
        return NULL;
    }


CleanExit:
    UserLocalFree(pnh);
    return pcres;
}


/***************************************************************************\
* CreateAniIcon
*
* For now, CreateAniIcon copies the jif rate table and the sequence table
* but not the CURSOR structs.  This is ok as long as this routine is
* internal only.
*
* History:
* 10-02-91 DarrinM      Created.
\***************************************************************************/

HCURSOR CreateAniIcon(
    LPCWSTR pszName,
    LPWSTR  rt,
    int     cicur,
    DWORD   *aicur,
    int     cpcur,
    HCURSOR *ahcur,
    JIF     jifRate,
    PJIF    ajifRate,
    BOOL    fPublic)
{
    HCURSOR hacon;
    CURSORDATA acon;
    DWORD cbacon;
    HCURSOR *ahcurT;             // Array of image frame pointers
    DWORD *aicurT;               // Array of frame indices (sequence table)
    PJIF ajifRateT;              // Array of time offsets
    int i;

    /*
     * Start by allocating space for the ACON structure and the ahcur and
     * ajifRate arrays.
     */
    hacon = (HCURSOR)NtUserCallOneParam(fPublic,
                                        SFI__CREATEEMPTYCURSOROBJECT);
    if (hacon == NULL)
        return NULL;

    /*
     * Save a couple LocalAlloc calls by allocating the memory needed for
     * the CURSOR, JIF, and SEQ arrays at once.
     */
    RtlZeroMemory(&acon, sizeof(acon));
    cbacon = (cpcur * sizeof(HCURSOR)) +
            (cicur * sizeof(JIF)) + (cicur * sizeof(DWORD));
    ahcurT = (HCURSOR *)UserLocalAlloc(HEAP_ZERO_MEMORY, cbacon);
    if (ahcurT == NULL) {
        NtUserDestroyCursor((HCURSOR)hacon, CURSOR_ALWAYSDESTROY);
        return NULL;
    }
    acon.aspcur = (PCURSOR *)ahcurT;

    /*
     * Set up work pointers
     */
    ajifRateT = (PJIF)((PBYTE)ahcurT + (cpcur * sizeof(HCURSOR)));
    aicurT = (DWORD *)((PBYTE)ajifRateT + (cicur * sizeof(JIF)));

    /*
     * Save offsets to arrays to make copying them to the server
     * easier.
     */
    acon.ajifRate = (PJIF)(cpcur * sizeof(HCURSOR));
    acon.aicur = (DWORD *)((PBYTE)acon.ajifRate + (cicur * sizeof(JIF)));

    acon.cpcur = cpcur;
    acon.cicur = cicur;

    acon.CURSORF_flags = CURSORF_ACON;

    /*
     * Store this information away so we can identify
     * repeated calls to LoadCursor/Icon for the same
     * resource type/id.
     */
    acon.rt = PTR_TO_ID(rt);
    acon.lpModName = szUSER32;
    acon.lpName = (LPWSTR)pszName;

    /*
     * Make a private copy of the cursor pointers and the animation rate table.
     */
    for (i = 0; i < cpcur; i++) {
        ahcurT[i] = ahcur[i];
//        ahcurT[i]->fPointer |= PTRI_ANIMATED;   // if GDI needs it

    }

    for (i = 0; i < cicur; i++) {

        /*
         * If constant rate, initialize the rate table to a single value.
         */
        if (ajifRate == NULL)
            ajifRateT[i] = jifRate;
        else
            ajifRateT[i] = ajifRate[i];

        /*
         * If no sequence table then build a unity map to the cursor table.
         */
        if (aicur == NULL)
            aicurT[i] = i;
        else
            aicurT[i] = aicur[i];
    }

    /*
     * Stuff acon data into the cursor
     */
    if (!_SetCursorIconData(hacon, &acon)) {
        NtUserDestroyCursor(hacon, CURSOR_ALWAYSDESTROY);
        hacon = NULL;
    }
    UserLocalFree(ahcurT);

    return hacon;
}


/***************************************************************************\
* ReadIconFromFileMap
*
* LATER: Error handling.
*
* History:
* 12-21-91 DarrinM      Created.
\***************************************************************************/

HCURSOR ReadIconFromFileMap(
    PFILEINFO   pfi,
    int         cbSize,   // used to seek past this chunk in case of error
    DWORD       cxDesired,
    DWORD       cyDesired,
    DWORD       LR_flags)
{
    PCURSORRESOURCE pcres;
    HCURSOR         hcur = NULL;
    LPNEWHEADER     pnh;
    int             offResBase;
    LPWSTR          rt;

    /*
     * Get current position in file to be used as the base from which
     * the icon data offsets are offset from.
     */
    offResBase = (int)(pfi->pFilePtr - pfi->pFileMap);

    /*
     * Read the .ICO/.CUR data's header.
     */
    ReadFilePtr(pfi, &pnh, sizeof(NEWHEADER));

    pcres = ReadIconGuts(pfi,
                         pnh,
                         offResBase,
                         &rt,
                         cxDesired,
                         cyDesired,
                         LR_flags);

    if (pcres != NULL) {
        hcur = (HCURSOR)ConvertDIBIcon((LPBITMAPINFOHEADER)pcres,
                                       NULL,
                                       NULL,
                                       (rt == RT_ICON),
                                       cxDesired,
                                       cyDesired,
                                       LR_ACONFRAME | LR_flags);

        UserLocalFree(pcres);
    }

    /*
     * Seek to the end of this chunk, regardless of our current position.
     */
    pfi->pFilePtr = pfi->pFileMap + ((offResBase + cbSize + 1) & (~1));

    return hcur;
}


/***************************************************************************\
* LoadAniIcon
*
*   Loads an animatied cursor from a RIFF file.  The RIFF file format for
*   animated cursors looks like this:
*
*   RIFF( 'ACON'
*       LIST( 'INFO'
*           INAM( <name> )
*           IART( <artist> )
*       )
*       anih( <anihdr> )
*       [rate( <rateinfo> )  ]
*       ['seq '( <seq_info> )]
*   LIST( 'fram' icon( <icon_file> ) ... )
*   )
*
*
* History:
* 10-02-91 DarrinM      Created.
* 03-17-93 JonPa        Rewrote to use RIFF format instead of RAD
* 04-22-93 JonPa        Finalized RIFF format (changed from ANI to ACON etc)
* 11/16/1995 SanfordS   Added LR_flags support.
\***************************************************************************/
HICON LoadAniIcon(
    IN PFILEINFO pfi,
    IN LPWSTR    rt,
    IN DWORD     cxDesired,
    IN DWORD     cyDesired,
    IN DWORD     LR_flags)
{
    int cpcur, ipcur = 0, i, cicur;
    ANIHEADER anih;
    ANIHEADER *panih = NULL;
    HICON hacon = NULL;
    HCURSOR *phcur;
    JIF jifRate, *pjifRate;
    RTAG tag;
    DWORD *picur;

    /*
     * Position to the beginning of the file.
     */
    pfi->pFilePtr = pfi->pFileMap + sizeof(tag);

#if DBG
    if ((ULONG_PTR)pfi->pFileEnd != ((ULONG_PTR)(pfi->pFileMap + sizeof (RTAG) + ((RTAG *)(pfi->pFileMap))->ckSize + 1) & ~1)) {
        RIPMSG2(RIP_WARNING, "LoadAniIcon: First RIFF chunk has invalid ckSize. Actual:%#lx Expected:%#lx",
                ((RTAG *)(pfi->pFileMap))->ckSize, (pfi->pFileEnd - pfi->pFileMap - sizeof(RTAG)) & ~1);
    }
#endif

    /* read the chunk type */
    if(!ReadFilePtrCopy(pfi,
                        &tag.ckID,
                        sizeof(tag.ckID))) {
        goto laiFileErr;
    }

    if (tag.ckID != FOURCC_ACON)
        goto laiFileErr;

    /* look for 'anih', 'rate', 'seq ', and 'icon' chunks */
    while( ReadTag(pfi, &tag)) {

        switch( tag.ckID ) {
        case FOURCC_anih:
            if (!ReadChunk(pfi, &tag, &anih))
                goto laiFileErr;

            if (!(anih.fl & AF_ICON) || (anih.cFrames == 0))
                goto laiFileErr;

            /*
             * Allocate space for the ANIHEADER, HCURSOR array and a
             * rate table (in case we run into one later).
             */
            cpcur = anih.cFrames;
            cicur = anih.cSteps;
            panih = (PANIHEADER)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(ANIHEADER) +
                    (cicur * sizeof(JIF)) + (cpcur * sizeof(HCURSOR)) +
                    (cicur * sizeof(DWORD)));

            if (panih == NULL)
                goto laiFileErr;


            phcur = (HCURSOR *)((PBYTE)panih + sizeof(ANIHEADER));
            pjifRate = NULL;
            picur = NULL;

            *panih = anih;
            jifRate = panih->jifRate;
            break;


        case FOURCC_rate:
            /*
             * If we find a rate chunk, read it into its preallocated
             * space.
             */
            pjifRate = (PJIF)((PBYTE)phcur + cpcur * sizeof(HCURSOR));
            if(!ReadChunk(pfi, &tag, (PBYTE)pjifRate))
                goto laiFileErr;
            break;


        case FOURCC_seq:
            /*
             * If we find a seq chunk, read it into its preallocated
             * space.
             */
            picur = (DWORD *)((PBYTE)phcur + cpcur * sizeof(HCURSOR) +
                    cicur * sizeof(JIF));
            if(!ReadChunk(pfi, &tag, (PBYTE)picur))
                goto laiFileErr;
            break;


        case FOURCC_LIST:
            {
                DWORD cbChunk = (tag.ckSize + 1) & ~1;

                /*
                 * See if this list is the 'fram' list of icon chunks
                 */
                if(!ReadFilePtrCopy(pfi, &tag.ckID, sizeof(tag.ckID))) {
                    goto laiFileErr;
                }

                cbChunk -= sizeof(tag.ckID);

                if (tag.ckID != FOURCC_fram) {
                    /*
                     * Not the fram list (probably the INFO list).  Skip
                     * the rest of this chunk.  (Don't forget that we have
                     * already skipped one dword!)
                     */
                    tag.ckSize = cbChunk;
                    SkipChunk(pfi, &tag);
                    break;
                }

                while(cbChunk >= sizeof(tag)) {
                    if (!ReadTag(pfi, &tag))
                        goto laiFileErr;

                    cbChunk -= sizeof(tag);

                    if(tag.ckID == FOURCC_icon) {

                        /*
                         * Ok, load the icon/cursor bits, create a cursor from
                         * them, and save a pointer to it away in the ACON
                         * cursor pointer array.
                         */
                        phcur[ipcur] = ReadIconFromFileMap(pfi,
                                                           tag.ckSize,
                                                           cxDesired,
                                                           cyDesired,
                                                           LR_flags);

                        if (phcur[ipcur] == NULL) {
                            for (i = 0; i < ipcur; i++)
                                NtUserDestroyCursor(phcur[i], 0);
                            goto laiFileErr;
                        }

                        ipcur++;
                    } else {
                        /*
                         * Unknown chunk in fram list, just ignore it
                         */
                        SkipChunk(pfi, &tag);
                    }

                    cbChunk -= (tag.ckSize + 1) & ~1;
                }
            }
            break;

        default:
            /*
             * We're not interested in this chunk, skip it.
             */
            if(!SkipChunk(pfi, &tag))
                goto laiFileErr;
            break;

        }

    }

    /*
     * Sanity check the count of frames so we won't fault trying
     * to select a nonexistant cursor
     */
    if (cpcur != ipcur) {
        RIPMSG2(RIP_WARNING, "LoadAniIcon: Invalid number of frames; Actual:%#lx Expected:%#lx",
                ipcur, cpcur);
        for (i = 0; i < ipcur; i++)
            NtUserDestroyCursor(phcur[i], CURSOR_ALWAYSDESTROY);
        goto laiFileErr;
    }



    if (cpcur != 0)
        hacon = CreateAniIcon(pfi->pszName,
                              rt,
                              cicur,
                              picur,
                              cpcur,
                              phcur,
                              jifRate,
                              pjifRate,
                              LR_flags & LR_GLOBAL);

laiFileErr:

#if DBG
    if (hacon == NULL) {
        RIPMSG0(RIP_WARNING, "LoadAniIcon: Invalid icon data format");
    }
#endif

    if (panih != NULL)
        UserLocalFree(panih);

    return hacon;
}
