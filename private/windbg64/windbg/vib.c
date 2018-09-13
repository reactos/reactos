/*++



Copyright 1992  Microsoft Corporation

Module Name:

    vib.c

Abstract:

    This module contains the VIB (Variable Information Block) support
    routines used by the watch and local windows.

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop


typedef struct {
    USHORT      cb;
    LPVOID      lpv;
} MEMLINK, * PTRMEMLINK;

#define NAMES_BUFFER 2048
#define BUFFERMAX 1024

//
// Global Storage (PROGRAM)
//

extern  CXF CxfIp;

//
// Global Storage (FILE)
//


static char        szBuffer[BUFFERMAX+1];

//
// Prototypes (LOCAL)
//

PTRCIF PASCAL FTGetCif( VOID );
VOID   PASCAL FTFreeVib( PTRVIB );
VOID   PASCAL FTFreeVibChildren( PTRVIB );
VOID   PASCAL FTFreeAllCif( PTRCIF );
LTS    PASCAL FTExpandVib( PTRVIB pvib );
VOID   PASCAL FTInvalidateVib( PTRVIB pvib);
VOID   PASCAL FTResetVib( PTRVIB pvib);
BOOL   PASCAL FTSetupWoj( PTRVIB pvib, PSTR pName, UINT cbSize);


/*** FTMakeWatchEntry
*
* Purpose: To make a watchentry
*
* Input:
*       ppvib   - A pointer to a vib pointer of where to place the new vib
*       pvit    - The vit tree that the watchentry is to be added to.
*       szExpStr- A string containing the expression
*
* Output:
*  Returns FALSE is ok, TRUE if errors
*
* Exceptions:
*
* Notes:
*
*/
int
FTMakeWatchEntry (
    void *ppvibVoid,
    void *pvit,
    char *szExpStr
    )
{
    EESTATUS    Err;
    UINT        cbszExpStr;
    char *      pch;
    ULONG        iLast;

    PTRVIB *    ppvib = (PTRVIB *) ppvibVoid;

    // get a new vib

    if( !(*ppvib = PvibAlloc ( NULL, (PTRVIB) pvit )) ) {
        FTError ( OUTOFMEMORY );
        return TRUE;
    }

    // take out leading and trailing spaces

    while( *szExpStr == ' ' ) {
        szExpStr++;
    }
    if ( !*szExpStr ) {
        return TRUE;
    }
    pch = szExpStr + strlen(szExpStr);
    while (pch > szExpStr) {
        if (*(pch = CharPrev(szExpStr, pch)) != ' ') {
            break;
        }
    }
    pch += (IsDBCSLeadByte(*pch) ? 2 : 1);
    strcpy(szBuffer,szExpStr);

    // do a parse
    if( (Err = EEParse(szBuffer, radix, fCaseSensitive, &(*ppvib)->hTMBd, &iLast)) ) {
//      (*ppvib) = (PTRVIB)NULL;
//      return(TRUE);
    }


    // Setup the Woj

    cbszExpStr = (UINT) (pch - szExpStr);
    if ( !FTSetupWoj( *ppvib, szExpStr, cbszExpStr) ) {
        *ppvib = NULL;
        FTError ( OUTOFMEMORY );
        return TRUE;
    }

    (*ppvib)->vibPtr            =       vibWatch;

    return FALSE;
}                                       /* WTMakeWatchEntry() */

/*** FTSetupWoj
*
* Purpose: Add a Woj (Watch Object?) to a vib
*
* Input:
*       pvib   - Pointer to the vib
*       pName  - Pointer to the Name
*       cbSize - Size of Name
*
* Output:
*       TRUE/FALSE if memory alloc failed
*
* Exceptions:
*
* Notes:
*
*/

BOOL
FTSetupWoj(
    PTRVIB pvib,
    PSTR pName,
    UINT cbSize
    )
{
    // If we already have one, lose it

    if ( pvib->pwoj ) {
        free( pvib->pwoj );
    }

    // Allocate the Woj

    pvib->pwoj = (PTRWOJ) malloc( sizeof(WOJ) + cbSize + 1 );
    if ( pvib->pwoj == NULL ) {
        return(FALSE);
    }

    // copy over the string

    strncpy ( pvib->pwoj->szExpStr, pName, cbSize);
    pvib->pwoj->szExpStr[cbSize] = '\0';

    // Fill out the other info

    pvib->pwoj->iFormSpec =       (WORD)cbSize;
    pvib->pwoj->ErrNbr    =       0;
    pvib->pwoj->cbLen     =       (unsigned char)cbSize;

    return(TRUE);
}

/***    FTVerify
 *
 *      Purpose: To verify the watch tree
 *
 *      Input:
 *      pcxf    - A pointer to the current CXF
 *      pvib    - A pointer to the vib to verify
 *
 *      Output:
 *      Returns:
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/


BOOL
FTVerify (
    PCXF pcxf,
    PTRVIB pvib
    )
{
    EEPDTYP     ExpTyp;
    BOOL        fBind = FALSE;
    EEHSTR      hTypeOld = NULL;
    EEHSTR      hTypeNew = NULL;
    LPSTR       lszTypeOld;
    LPSTR       lszTypeNew;
    HTI         hti;
    RTMI        rti = {0};
    PTI         pti;
    BOOL        fStatus = FALSE;

    /*
     * If we're in a function evaluation get out fast
     */

    if ( (LptdCur != NULL) && (LptdCur->fInFuncEval)) {
        return(fStatus);
    }

    /*
     *  only while the Variable Info Block is valid
     */

    for (; pvib; pvib = pvib->pvibSib ) {
        /*
         * Don't rebind this items
         */

        if (pvib->vibPtr == vibGeneric) {
            continue;
        }

        /*
         *  rebind, the error will be displayed during evaluation
         */

        if (EEGetTypeFromTM(&pvib->hTMBd, NULL, &hTypeOld, TRUE ) != EENOERROR) {
            hTypeOld = NULL;
        }

        fBind = ((pvib->hprocCache != ( SHpCXTFrompCXF ( pcxf ) )->hProc) ||
                 (pvib->hblkCache  != ( SHpCXTFrompCXF ( pcxf ) )->hBlk));

        if ( fBind ) {
              pvib->hprocCache = ( SHpCXTFrompCXF ( pcxf ) )->hProc;
              pvib->hblkCache  = ( SHpCXTFrompCXF ( pcxf ) )->hBlk;
              fStatus = TRUE;
        }

        /*
         *  Make sure we can still Bind
         */

        if ( pvib->hTMBd &&
             EEBindTM(&pvib->hTMBd, SHpCXTFrompCXF(pcxf),fBind,FALSE) == EENOERROR ) {

            /*
             * Reset the NoBind Flag because it back in context
             */

            FTResetVib(pvib);

            /*
             * Make sure the Function Evaluation gets updated
             */

            if (EEInfoFromTM ( &pvib->hTMBd, &rti, &hti ) == EENOERROR) {
                DAssert(hti != (HTI)NULL);
                pti = (PTI) MMLpvLockMb (hti);
                if (pti->fFunction) {
                    pvib->flags.FuncEval = 1;
                }
                if ( pti != NULL ) {
                    MMbUnlockMb ( hti );
                }
            }

            if (hti != (HTI) NULL) {
                EEFreeTI(&hti);
            }

            /*
             * Has the Type changed for a Watch or a Type
             */

            if ((pvib->vibPtr == vibWatch) || (pvib->vibPtr == vibType)) {
                lszTypeOld = NULL;
                lszTypeNew = NULL;
                if (hTypeOld) {
                    if (EEGetTypeFromTM(&pvib->hTMBd, NULL, &hTypeNew, TRUE ) != EENOERROR) {
                        hTypeNew = NULL;
                    } else {
                        lszTypeOld = (PSTR) MMLpvLockMb( hTypeOld );
                        lszTypeNew = (PSTR) MMLpvLockMb( hTypeNew );
                    }
                }
                if (!lszTypeNew || strcmp(lszTypeOld, lszTypeNew)) {
                    /*
                     * remove the rest of the tree
                     */

                    FTFreeAllCif(pvib->pcif);
                    pvib->pcif = NULL;
                    FTclnUpdateParent ( pvib, (1-pvib->cln) );
                    fStatus = TRUE;

                    /*
                     *  Make sure VibPtr is reset
                     */

                    if ( pvib->vibPtr == vibWatch || pvib->vibPtr == vibType ) {
                        ExpTyp = EEIsExpandable(&pvib->hTMBd);
                        if ( ExpTyp == EETYPE || ExpTyp == EETYPENOTEXP ) {
                            pvib->vibPtr = vibType;
                        } else {
                            pvib->vibPtr = vibWatch;
                        }
                    }
                }

                if (lszTypeNew) {
                    MMbUnlockMb( hTypeOld );
                    MMbUnlockMb( hTypeNew );
                }

            }

            /*
             * Update any children
             */

            if (pvib->pcif) {
                if ( pvib->pcif->hTMBd ) {
                    EEBindTM( &pvib->pcif->hTMBd, SHpCXTFrompCXF(pcxf),
                             fBind, FALSE);
                }

                /*
                 * If a child had a signicant change, so did we
                 */

                if ( FTVerify ( pcxf, pvib->pcif->pvibChild ) ) {
                    fStatus = TRUE;
                }
            }
        } else {

            /*
             *  Couldn't Bind, Invalidate any children
             */

            if ( pvib->pcif ) {
                FTInvalidateVib(pvib->pcif->pvibChild);
            }
        }
    }

    if (hTypeOld) {
        EEFreeStr(hTypeOld);
    }
    if (hTypeNew) {
        EEFreeStr(hTypeNew);
    }

    return(fStatus);
}                               /* FTVerify() */

/***    FTVerifyNew
 *
 *      Purpose: Determine if a vib's result have changed since
 *               the last time a user saw it
 *
 *      Input:
 *      pvit    - A pointer to the vit
 *      oln     - Item number of interest
 *
 *      Output:
 *      Returns:
 *          TRUE/FALSE the item has changed
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


BOOL
FTVerifyNew(
    PTRVIT pVit,
    ULONG oln
    )
{
    PTRVIB pVib = FTvibGetAtLine( pVit, oln);
    VPI    i;

    // No Vib, No change
    if ( pVib == 0) {
        return(FALSE);
    }
    i = pVib->vibIndex;

    // Give EE a chance to fill out the result
    FTEnsureTextExists( pVib );
    FTGetPanelString( pVit, pVib, ID_PANE_RIGHT);

    // Do we have a string at all?
    if ( pVib->pvtext[i].pszValueP || pVib->pvtext[i].pszValueC) {

        // Do we have both strings?
        if ( pVib->pvtext[i].pszValueP && pVib->pvtext[i].pszValueC   ) {
            if ((!_strcmpi(pVib->pvtext[i].pszValueC,pVib->pvtext[i].pszValueP)) &&
                (pVib->pvtext[i].fChanged == FALSE))
            {
                 return(FALSE);
            } else
            if ((!_strcmpi(pVib->pvtext[i].pszValueC,pVib->pvtext[i].pszValueP)) &&
                (pVib->pvtext[i].fChanged == TRUE))
            {
                pVib->pvtext[i].fChanged = FALSE;
                return(TRUE);
            }
        }

        // Nope, It changed
        pVib->pvtext[i].fChanged = TRUE;
        return(TRUE);
    }

    // Nope, No change
    return(FALSE);

}  /* FTVerifyNew() */


/***    FTvibGetAtLine Get the vib starting at the line specified.
 *
 *      Purpose: Given a vit, and a line, return the vib packet discribing that line.
 *
 *      Input:
 *        pvit  - A valid Variable Info block Top describing a Variable Info
 *                 Block list
 *        oln   - The line of interest
 *
 *      Output:
 *       Returns
 *        pvib    - A pointer to a vib that describes the requested line
 *
 *      Exceptions:
 *
 *      Notes: Make sure the Variable Info block Top list is valid!
 *
*/


PTRVIB
FTvibGetAtLine(
    PTRVIT pvit,
    ULONG oln
    )
{
    ULONG       olnCur = 0;
    VPI         vpiT   = 0;
    PTRVIB      pvib;
    ULONG       strIndex;
    HTI         hti;
    RTMI        rti = {0};
    PTI         pti;

    if ( pvit ) {
        pvib = pvit->pvibChild;
    } else {
        return(NULL);
    }

    /*
     * Loop through the vib tree looking for the correct vib.  Do this
     *  by adding the count of lines in a pvib structure until we get
     *  to the correct vib.
     */

    while( pvib  &&  olnCur != oln ) {

        /*
         * Is the line number we are looking for in the current vib?
         *
         *    YES -- move to the child of this vib (expanded item)
         *    NO  -- skip to the next vib in this list
         */

        if( oln < olnCur + (WORD)pvib->cln ) {
            olnCur++;
            vpiT = 0;
            pvib = pvib->pcif->pvibChild; // go into the expanded item.
        }

        else { // skip this vib, go on to the next
            olnCur += pvib->cln;
            pvib = pvib->pvibSib;
        }

        if ( pvib == NULL ) {
            break;
        }


        /*
         *  Move to the i-th element of the current vib, where i is the
         *      distance between the first line in the vib and the
         *      actual line desired.
         *
         *  If the current vib is a vibGeneric then we need to get the
         *      i-th element in this vib.
         *  If the current vib is a vibChild -????
         *  ELSE -- no action needs to be taken -- use the current vib
         *      since it is a one line item.
         */

        if ( pvib->vibPtr == vibGeneric || pvib->vibPtr == vibChild ) {
            HTM     hTM;

            if ((!pvib->pvibSib) ||
                (oln < olnCur + pvib->vibIndex - vpiT) ) {

                EESTATUS Err;

                /*
                 * search for the generic vib
                 */

                while( pvib->pvibSib ) {
                    pvib = pvib->pvibSib;
                }

                /*
                 * OK -- now if we have some data for this item then we
                 *      can get information about it.
                 */

                DAssert(pvib->hTMBd == 0);

                if ( !pvib->flags.NoData ) {

                    // get the index and TM
                    pvib->vibIndex = (unsigned short) (vpiT +  oln - olnCur);

                    /*
                     * Make sure the structure we are about to index into
                     *  already exitsts.
                     */

                    FTEnsureTextExists( pvib );

                    /*
                     * Now see if we have already created a TM for this, if
                     *  so then we can just use it.
                     */

                    if (pvib->pvtext[pvib->vibIndex].htm) {
                        ;
                    } else {
                        /*
                         * get the TM to expand.
                         */

                        if( pvib->pvibParent->pcif->hTMBd ) {
                            hTM = pvib->pvibParent->pcif->hTMBd;
                        } else {
                            hTM = pvib->pvibParent->hTMBd;
                        }

                        Err = EEGetChildTM ( &hTM, pvib->vibIndex,
                                            &pvib->pvtext[pvib->vibIndex].htm,
                                            &strIndex, fCaseSensitive, radix );

                        if (Err) {
                            CVExprErr ( Err, MSGGERRSTR, &hTM, NULL);
                            pvib->flags.NoBind;
                        } else {
                            if (!EEInfoFromTM(&pvib->pvtext[pvib->vibIndex].htm,
                                              &rti, &hti)) {
                                DAssert(hti != (HTI) NULL);
                                pti = (PTI) MMLpvLockMb( hti );
                                if (pti != NULL) {
                                    if (pti->fFunction) {
                                        pvib->flags.FuncEval = 1;
                                    }
                                    MMbUnlockMb( hti );
                                }
                            }
                            if (hti != (HTI) NULL) {
                                EEFreeTI(&hti);
                            }
                        }

                    }
                    olnCur = oln;
                }
            }
            else {
                // check out this Variable Info Block, it cannot be
                // the generic Variable Info Block!
                // we must advance by the preceeding blank space

                olnCur += pvib->vibIndex - vpiT;
                vpiT = pvib->vibIndex + 1;
            }
        }
    }

    return( pvib );
}                   /* FTvibGetAtLine() */


/***    PvibAlloc
 *
 *      Purpose:  To get a new Variable Info Block. If the list has a Variable
 *            Info Block after it, choose that as the next Variable Info Block,
 *            but don't alter the current Variable Info Block. If a new Variable
 *            Info Block must be allocated, all fields are initialized. This
 *            does not update anything in the parent!
 *
 *      Input:
 *        pvib       - A pointer to the potential next vib. This may be NULL;
 *        pvibParent - A pointer to the parent vib, This may not be NULL;
 *
 *      Output:
 *
 *       Returns
 *            A pointer to the next vib. The new vib will be unchanged if it
 *            already existed, otherwise it will be initialized.
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/


PTRVIB
PvibAlloc (
    PTRVIB pvib,
    PTRVIB pvibParent
    )
{
    if ( !pvib ) {
        pvib = (PTRVIB) malloc( sizeof(VIB) );

        if ( !( pvib ) ) {
            return ( NULL );
        }

        memset ( pvib, 0, sizeof( VIB ) );
        pvib->cln        = 1;
        pvib->pvibParent = pvibParent;
        pvib->vibPtr     = vibUndefined;

        // if the parent has a parent, then it must not be a Variable
        //   Info block Top, get one more than the parents level

        if ( pvibParent->pvibParent ) {
            pvib->level = (unsigned char) (pvibParent->level + 1);
        }
        else {
            pvib->level = 0;
        }
    }

    return ( pvib );
}                   /* PvibAlloc() */


/***    FTvibGet
 *
 *      Purpose:  To get a new Variable Info Block. If the list has a Variable
 *            Info Block after it, choose that as the next Variable Info Block,
 *            but don't alter the current Variable Info Block. If a new Variable
 *            Info Block must be allocated, all fields are initialized. This
 *            does not update anything in the parent!
 *
 *      Input:
 *        pvib       - A pointer to the potential next vib. This may be NULL;
 *        pvibParent - A pointer to the parent vib, This may not be NULL;
 *
 *      Output:
 *
 *       Returns
 *            A pointer to the next vib. The new vib will be unchanged if it
 *            already existed, otherwise it will be initialized.
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


PTRVIB
FTvibGet (
    PTRVIB pvib,
    PTRVIB pvibParent
    )
{
    if ( !pvib ) {
        pvib = (PTRVIB) malloc( sizeof(VIB) );
        if ( !( pvib ) ) {
            return ( NULL );
        }

        memset ( pvib, 0, sizeof( VIB ) );
        pvib->cln        = 1;
        pvib->pvibParent = pvibParent;
        pvib->vibPtr     = vibUndefined;

        // if the parent has a parent, then it must not be a Variable
        //   Info block Top, get one more than the parents level

        if ( pvibParent->pvibParent ) {
            pvib->level = (unsigned char) (pvibParent->level + 1);
        }
        else {
            pvib->level = 0;
        }
    }

    return ( pvib );
}




/***    FTvibInit
 *
 *      Purpose: To clear out an existing Variable Info Block, freeing all of
 *               its children and sibilings.
 *
 *      Input:
 *        pvib          - A pointer to the vib to be initialized
 *        pvibParent    - A pointer to the parent, this may not be NULL
 *
 *      Output:
 *       Returns pvib
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/


PTRVIB
FTvibInit(
    PTRVIB pvib,
    PTRVIB pvibParent
    )
{

    // Free all of the children. Don't use FTFreeVibChildren() because it may
    // merge this vibif it is an array

    FTFreeAllCif ( pvib->pcif );

    // free all remaining sibiling
    FTFreeAllSib ( pvib->pvibSib );
    pvib->pvibSib = (PTRVIB) NULL;

    // reinitialize the packet
    if ( pvib->vibPtr != vibGeneric && pvib->vibPtr != vibUndefined && pvib->hTMBd ) {
        EEFreeTM (&pvib->hTMBd);
    }

    if ( pvib->pwoj ) {
        free( pvib->pwoj );
        pvib->pwoj = (PTRWOJ)NULL;
    }

    memset ( pvib, 0, sizeof( VIB ) );
    pvib->cln           = 1;
    pvib->pvibParent    = pvibParent;
    pvib->vibPtr        = vibUndefined;

    // if the parent has a parent, then it must not be a Variable Info
    //   block Top, get one more than the parent's level

    if( pvibParent->pvibParent ) {
        pvib->level = (unsigned char) (pvibParent->level + 1);
    } else {
        pvib->level = 0;
    }

    return(pvib);

}   /* FTvibInit() */



/***    FTGetCif
 *
 *      Purpose: To allocate a full Child InFo packet.
 *
 *      Input: NONE
 *
 *      Output:
 *       Returns  A pointer to a new Child InFo packet.
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/

PTRCIF
FTGetCif(
    void
    )
{
    PTRCIF  pcif;

    if ( (pcif = (PTRCIF) malloc( sizeof(CIF) )) ) {
        memset(pcif, 0, sizeof(CIF));
    }

    return ( pcif );

} /* FTGetCif() */


/***    FTFreeVib
 *
 *      Purpose: To free a Variable Info Block and all of its associated memory
 *
 *      Input:
 *        pvib    - A pointer to the Variable Info Block that is to be freed
 *
 *      Output:
 *       Returns .....
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/

VOID
FTFreeVib (
    PTRVIB pvib
    )
{
    ULONG i;
    if ( pvib ) {

        // free the TM if it is appropriate
        //
        //  For child TMS -- this is a copy of the item in the vibGeneric
        //

        if ( pvib->vibPtr == vibChild) {
            pvib->hTMBd = 0;
        }
        if ( pvib->vibPtr != vibUndefined && pvib->hTMBd ) {
            EEFreeTM ( &pvib->hTMBd );
        }

        //  Free the watch information
        if ( pvib->pwoj     )
            free(pvib->pwoj );

        // free any text strings
        if ( pvib->cText ) {
            for (i=0; i<pvib->cText ; i++ ) {

                if ( pvib->pvtext[i].pszValueC)
                     free(pvib->pvtext[i].pszValueC);

                if ( pvib->pvtext[i].pszValueP)
                     free(pvib->pvtext[i].pszValueP);

                if ( pvib->pvtext[i].pszFormat)
                     free(pvib->pvtext[i].pszFormat);

                if ( pvib->pvtext[i].htm) {
                    EEFreeTM(&pvib->pvtext[i].htm);
                }
            }
            free(pvib->pvtext);
        }


        // now free the Variable Info Block itself

        free(pvib);
    }
    return;
}                   /* FTFreeVib() */


/***    FTFreeVibChildren
 *
 *      Purpose: Given a Variable Info Block, clear out all child structures.
 *               The pcif (Pointer to Child Info) field will be NULL on return.
 *
 *      Input:
 *        pvib    - A pointer to the Variable Info Block that is to be cleared
 *
 *      Output:
 *       Returns .....
 *
 *      Exceptions:
 *
 *      Notes: If the vib is of type vibArray, the vib itself may be merged in
 *         with the genaric array vib.
 *
*/


VOID
FTFreeVibChildren (
    PTRVIB pvib
    )
{
    PTRVIB  pvibCur;
    VIB     vibT;

    // free all of its Child InFo packets

    FTFreeAllCif ( pvib->pcif );
    pvib->pcif = NULL;

    // if the Variable Info Block is expanded, merge it back into the
    // the generic element by eliminating it from the child list
    if( pvib->vibPtr != vibChild ) return;

    // search the list
    pvibCur = &vibT;

    // This should never be at level 0 of the chain,
    // the parent should never be a Variable Info block Top

    pvibCur->pvibSib = pvib->pvibParent->pcif->pvibChild;
    while( pvibCur->pvibSib != pvib ) pvibCur = pvibCur->pvibSib;

    pvibCur->pvibSib = pvib->pvibSib;
    pvib->pvibParent->pcif->pvibChild = vibT.pvibSib;

    // Don't free the hTM since it is a copy of the one being saved in
    // the generic structure rather than a duplicate handle.

    pvib->hTMBd = 0;
//    if ( pvib->hTMBd ) {
//        EEFreeTM ( &pvib->hTMBd );
//    }

    FTFreeVib ( pvib );
    return;
}                   /* FTFreeVibChildren() */

/***    FTFreeAllSib
 *
 *      Purpose: Starting at the point specifed in the tree, all following children
 *            and sibilings are freed. The Variable Info Block specified is also freed.
 *
 *      Input:
 *        pvib    - The first Variable Info Block in the list to be freed.
 *
 *      Output:
 *       Returns .....
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/


void
FTFreeAllSib(
    PTRVIB pvib
    )
{
    while ( pvib ) {
        if( pvib->pcif ) {
            FTFreeAllCif( pvib->pcif );
        }


        if( pvib->pvibSib ) {
            pvib->pvibSib->pvibParent = pvib;
            pvib = pvib->pvibSib;
            FTFreeVib ( pvib->pvibParent );
        } else {
            FTFreeVib( pvib );  // free the last vib and get out
            break;
        }
    }
    return;
}                   /* FTFreeAllSib() */

/***    FTFreeAllCif
 *
 *      Purpose: To free all children blocks and the Child InFo.
 *
 *      Input:
 *        pcif    A pointer to the Child InFo.
 *
 *      Output:
 *       Returns .....
 *
 *      Exceptions:
 *
 *      Notes:
 *
*/

VOID
FTFreeAllCif(
    PTRCIF pcif
    )
{
    if( pcif ) {
        FTFreeAllSib ( pcif->pvibChild );
        pcif->pvibChild = (PTRVIB) NULL;

        if ( pcif->hTMBd ) {
            EEFreeTM ( &pcif->hTMBd );
        }

        free ( pcif );
    }
    return;
}                   /* FTFreeAllCif() */


/***    FTExpandOne(PTRVIB pvib)
 *
 *      Purpose:
 *        Expand all vibs at this level in the tree (All siblings)
 *
 *
 *      Input:
 *        pVib   - Pointer to a VIB
 *
 *      Output:
 *        None
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


VOID
FTExpandOne(
    PTRVIB pvib
    )
{
    while ( pvib) {

        // Don't Collapse if it's already open!
        if ( !pvib->pcif && pvib->flags.ExpandMe ) {
            FTExpandVib(pvib);
        }

        // Now take care of the siblings
        pvib = pvib->pvibSib;
    }
}


/***    FTExpand
 *
 *      Purpose: To expand the structure at the line pointed to by oln. oln is
 *               the offset from the start of the locals, not the start of the
 *               window.
 *
 *      Input:
 *          pvit - A Pointer to the Variable Information Top
 *          oln  - The offset from the first local
 *
 *      Output:
 *         Returns a LTS
 *
 *      Exceptions:
 *
 *
*/


LTS
FTExpand (
    PTRVIT pvit,
    ULONG oln
    )
{
    PTRVIB    pvib;

    if ( pvit  &&  (pvib = FTvibGetAtLine( pvit, oln)) ) {
        return( FTExpandVib(pvib) );
    }

    return(UNABLETOEXPAND);
}


/***    FTExpand
 *
 *      Purpose: To expand the structure at a particular vib.
 *
 *      Input:
 *          PTR pvib    - Pointer to a VIB
 *
 *      Output:
 *       Returns a LTS
 *
 *      Exceptions:
 *
 *
*/

LTS
FTExpandVib(
    PTRVIB pvib
    )
{
    EESTATUS  Err;
    HTM       hTM;
    long      cChild;
    EEPDTYP   ExpTyp;
    SHFLAG    shflag;
    ULONG     strIndex;
    HTI       hti;
    RTMI      rti = {0};
    HTM       hTMParent;

    //
    // If its already expand, collasse it
    //

    if ( pvib->pcif ) {
        FTclnUpdateParent ( pvib, (1-pvib->cln) );
        FTFreeVibChildren ( pvib );
        return OK;
    }

    //
    // Can we Expand?
    //

    if ( pvib->flags.NoData || pvib->flags.NoBind ) {
        return(UNABLETOEXPAND);
    }

    if (pvib->vibPtr == vibGeneric) {
        hTMParent = pvib->pvtext[pvib->vibIndex].htm;
    } else {
        hTMParent = pvib->hTMBd;
    }

    if ( EEInfoFromTM ( &hTMParent, &rti, &hti ) != EENOERROR) {
        return(UNABLETOEXPAND);
    }

    if (hti != (HTI) 0) {
        EEFreeTI(&hti);
    }
    ExpTyp = EEIsExpandable ( &hTMParent );
    if ( ExpTyp == EENOTEXP || ExpTyp == EETYPENOTEXP ) {
        return ( UNABLETOEXPAND );
    }

    // here is the nasty, if we are currently an expandable, we don't
    // want to update the generic vib, but rather make a new one

    if( pvib->vibPtr == vibGeneric  ) {
        PTRVIB *ppvib;
        PTRVIB  pvibNew;

        // get a new vib
        if ( !(pvibNew = PvibAlloc ( NULL, pvib->pvibParent )) ) return ( OUTOFMEMORY );

        // copy the generic vib
        *pvibNew = *pvib;
        pvibNew->cln = 1;       // put in the count

        // Copy over the correct hTM

        pvibNew->hTMBd = pvib->pvtext[pvib->vibIndex].htm;

        // put this vib in the list, this should alway be at least two down
        // from the root vit. A sym must be in between

        ppvib = &pvib->pvibParent->pcif->pvibChild;
        pvib = *ppvib;
        while ( pvib->pvibSib  &&  pvib->vibIndex < pvibNew->vibIndex ) {
            ppvib = &pvib->pvibSib;
            pvib = pvib->pvibSib;
        }

        // fixup the link list
        *ppvib = pvibNew;
        pvibNew->pvibSib = pvib;

        // now select this vib
        pvib = pvibNew;
        pvib->vibPtr = vibChild;
        pvib->pvtext = NULL;        // Make sure that Text Info isn't dup'd
        pvib->cText  = 0;           // because of the copy above.
    }

    // now expand it
    if( !(pvib->pcif = FTGetCif()) || !(pvib->pcif->pvibChild = PvibAlloc ( NULL, pvib )) ) {
        FTFreeVibChildren( pvib );
        return ( UNABLETOEXPAND );
    }

    // if it is a pointer and I can't dereference, get out

    hTM = hTMParent;
    if( ExpTyp == EEPOINTER  &&
        EEDereferenceTM ( &hTMParent, &hTM, &strIndex, fCaseSensitive ) != EENOERROR ) {

        FTFreeVibChildren( pvib );
        return ( UNABLETOEXPAND );
    }

    // get the count
    Err = EEcChildrenTM ( &hTM, &cChild, &shflag );

    // set up the new Variable Info Block after expansion
    switch(ExpTyp) {

      case EEAGGREGATE:
          if ( !cChild ) {
              pvib->pcif->pvibChild->flags.NoData;
              cChild = 1;
          }
          // FALL THROUGH

      case EETYPE:
          pvib->pcif->pvibChild->vibPtr = vibGeneric;
          break;

      case EEPOINTER:
          if( cChild ) {
              pvib->pcif->pvibChild->vibPtr = vibGeneric;
              pvib->pcif->hTMBd = hTM;
          } else {
               pvib->pcif->pvibChild->vibPtr = vibPointer;
               pvib->pcif->pvibChild->hTMBd  = hTM;
               cChild = 1;
          }
          break;
    }

    // say we are expanded, and put the expand count in the tree.
    FTclnUpdateParent( pvib, (short) cChild );
    pvib->flags.ExpandMe = FALSE;
    return(OK);
}                   /* FTExpand() */


/***    FTclnUpdateParent
 *
 *      Purpose: To Go up the tree and update all of the parent's cln fields
 *
 *      Input:
 *        pvibParent  - The first parent to update
 *        dcln        - The difference to update them by
 *
 *      Output:
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */
void
FTclnUpdateParent(
    PTRVIB pvibParent,
    int dcln
    )
{
    while ( pvibParent ) {   // Walk up the tree
        pvibParent->cln += (short) dcln;
        pvibParent = pvibParent->pvibParent;
    }
}                   /* FTclnUpdateParent() */


/***    FTGetVibTypeString
 *
 *      Purpose:
 *          Get a pointer to a vib return a string that contains the
 *          text for its expansion button.
 *
 *      Input:
 *          pVib - Pointer to the Vib
 *
 *      Output:
 *          Returns a pointer to the String o rNULL
 *
 *      Exceptions:
 *
 *
*/

PSTR
FTGetVibTypeString(
    PTRVIB pVib
    )
{
    EEPDTYP  ExpTyp;

    //  If we don't have any data, return a space

    if ( pVib->flags.NoData || pVib->flags.NoBind ) {
        return( " " );
    }

    /*
     * If EE says we can't expand, return a space
     */

    if (pVib->vibPtr == vibGeneric) {
        ExpTyp = EEIsExpandable( &pVib->pvtext[pVib->vibIndex].htm );
    } else {
        ExpTyp = EEIsExpandable ( &pVib->hTMBd );
    }

    if ( (ExpTyp == EENOTEXP) || (ExpTyp == EETYPENOTEXP)) {
         return( " " );

    //  If we have a child info block, we're already expanded

    } else if ( pVib->pcif ) {
        return( "-" );
    }

    //  Else we are expandable
    return( "+" );
}                               /* FTGetVibTypeString() */



/***    FTGetVibResultString
 *
 *      Purpose:
 *          Get a pointer to a vib return a string that contains the
 *          text for the evaluated result.
 *
 *      Input:
 *          pVit - Pointer to the VIT (Variable Information Top)
 *          pVib - Pointer to the VIB (Variable Information Block)
 *
 *      Output:
 *          Returns a pointer to the String o rNULL
 *
 *      Exceptions:
 *
 *
*/

PSTR
FTGetVibResultString(
    PTRVIT pVit,
    PTRVIB pVib
    )
{
    EEHSTR      hValue = 0;
    PCXF        pCxf;
    PSTR        szValue;
    EESTATUS    Err;
    PSTR        pszFormat = NULL;
    PVTEXT      pvtext;
    HTM         hTM;


    pCxf = (pVib->vibPtr == vibWatch) ? &CxfIp : &pVit->cxf;

    if ( pVib->flags.NoBind) {
        szBuffer[0] = 0;
        return(szBuffer);
    }

    if (pVib->vibPtr == vibGeneric) {
        hTM = pVib->pvtext[pVib->vibIndex].htm;
    } else {
        hTM = pVib->hTMBd;
    }

    if ( pVib->flags.NoData ) {
        LoadString(g_hInst, ERR_Expclass_No_Members, szBuffer, BUFFERMAX);
        return(szBuffer);
    }

    if ( pVib->flags.FuncEval) {
        LoadString(g_hInst, ERR_No_Funcs_In_Watch, szBuffer, BUFFERMAX);
        return(szBuffer);
    }

    // If we have a format override, make sure it passed
    if ( pVib->pvtext) {
        pvtext = &pVib->pvtext[pVib->vibIndex];
        if ( pvtext->pszFormat ) {
            pszFormat = pvtext->pszFormat;
        }
    }

    if (pszFormat && *pszFormat == ',') {
        pszFormat++;
    }
    if ( (Err = EEvaluateTM ( &hTM, SHhFrameFrompCXF(pCxf),EEVERTICAL )) ||
         (Err = EEGetValueFromTM ( &hTM, radix, (PUCHAR) pszFormat, &hValue)) ) {
        CVExprErr ( Err, MSGSTRING, &hTM, szBuffer );
        return(szBuffer);
    }

    szValue = (PSTR) MMLpvLockMb( (HDEP)hValue );
    strncpy(szBuffer,szValue,BUFFERMAX);
    MMbUnlockMb ( (HDEP)hValue );
    EEFreeStr(hValue);

    // If we have a new line in the string then terminate there
    //pszBuffer = strchr(szBuffer,'\n');
    //if (pszBuffer) *pszBuffer = 0;


    return(szBuffer);
}


/***    FTGetVibNameString
 *
 *      Purpose:
 *          Get a pointer to a vib return a string that contains the
 *          text for the name portion.
 *
 *      Input:
 *          pVib - Pointer to the VIB (Variable Information Block)
 *
 *      Output:
 *          Returns a pointer to the String o rNULL
 *
 *      Exceptions:
 *
 *
*/

PSTR
FTGetVibNameString(
    PTRVIB pVib
    )
{
    PSTR    pszBuffer = szBuffer;
    PSTR    szValue;
    HDEP    hType = 0;
    EEHSTR  hName = 0;
    int     i;
    int     len = 0;
    EESTATUS    Err;

    /*
     *  put in indent
     */


    if ((int)pVib->level < 128) {
        for(i=0; i<(int)pVib->level; i++) {
            *pszBuffer++ = '|';
            *pszBuffer++ = ' ';
            len += 2;
        }
    }

    /*
     *  Get the Name
     */

    switch(pVib->vibPtr) {
        /*
         *  Case 1:
         *
         *  This implies that the item is a real variable and we can
         *      know its name from what was typed in to the system. So
         *      get it from the typed in expression.
         */

    case vibWatch:
        strncpy(pszBuffer,pVib->pwoj->szExpStr,BUFFERMAX-len);
        break;

        /*
         *  Get the Name from the Type info
         */

    case vibType:
        if ((pVib->pwoj != NULL) && (pVib->pwoj->szExpStr != NULL)) {
            strncpy(pszBuffer, pVib->pwoj->szExpStr, BUFFERMAX-len);
        } else if ( pVib->flags.NoBind ) {
            /*
             *  Out of Context, no info
             */

            *pszBuffer = 0;
        } else {
            /*
             *  Get the Type String
             */

            if (((Err = EEGetNameFromTM(&pVib->hTMBd, &hName)) != EENOERROR) ||
                ((Err = EEGetTypeFromTM(&pVib->hTMBd, hName,
                                        (PEEHSTR)&hType, TRUE) != EENOERROR))) {
                CVExprErr ( Err, MSGSTRING, &pVib->hTMBd, szBuffer );
                if (hName) {
                    EEFreeStr(hName);
                }
                if (hType) {
                    EEFreeStr(hType);
                }
                return(szBuffer);
            }

            /*
             *  Lock it down and copy to the woj
             */

            szValue = (PSTR) MMLpvLockMb ( hType );
            strncpy(pszBuffer,szValue + sizeof(HDR_TYPE),BUFFERMAX-len);
            MMbUnlockMb ( hType );
            EEFreeStr ( (EEHSTR)hType );
        }
        break;

        /*
         *  Case 3:
         *      We are doing deverivations from a base item.  In this
         *      case we are hopless about getting the string correct
         *      on our own so let the EE get the string for use.  It
         *      will still be wrong sometimes but mostly correct.
         */

    case vibGeneric:
        if (pVib->flags.NoBind) {
            /*
             * Expression is out of context so return NULL string
             */
            *pszBuffer = 0;
        } else {
            /*
             * Get the name from the TM
             */

            Err = EEGetNameFromTM( &pVib->pvtext[pVib->vibIndex].htm,
                                  &hName );

            if ( Err != EENOERROR ) {
                CVExprErr( Err, MSGSTRING,
                          &pVib->pvtext[pVib->vibIndex].htm, szBuffer);
                if (hName) {
                    EEFreeStr( hName );
                }
                return szBuffer;
            }

            szValue = (PSTR) MMLpvLockMb( (HDEP) hName );
            strncpy( pszBuffer, szValue, BUFFERMAX-len );
            pszBuffer[BUFFERMAX-len] = 0;
            MMbUnlockMb( (HDEP) hName );
            EEFreeStr( hName );
        }
        break;

        /*
         *  Case 4:
         *
         *       Get the Name from the Evaluation tree
         */
    default:
        if ( pVib->flags.NoBind ) {
            /*
             *  Out of Context, no info
             */

            *pszBuffer = 0;
        } else {
            /*
             *  Get the Name from the tm
             */

            if ( Err = EEGetNameFromTM ( &pVib->hTMBd, &hName) ) {
                CVExprErr ( Err, MSGSTRING, &pVib->hTMBd, szBuffer );
                if ( hName ) {
                    EEFreeStr(hName);
                }
                return(szBuffer);
            }

            szValue  = (PSTR) MMLpvLockMb( (HDEP)hName );
            strncpy(pszBuffer,szValue,BUFFERMAX-len);
            pszBuffer[BUFFERMAX-len] = 0;
            MMbUnlockMb ( (HDEP)hName );
            EEFreeStr(hName);
        }
        break;
    }

    return(szBuffer);
}

/***    FTGetWatchList
 *
 *      Purpose:
 *        Return a buffer that has a list of all the Toplevel watchs
 *        for the given vit.
 *
 *      Input:
 *        pVit       - Pointer to the Variable Information Top
 *
 *      Output:
 *        Returns a pointer to the formatted buffer or NULL if there
 *        was an error.
 *
 *      Exceptions:
 *
 *      Notes:
 *        The Buffer was malloced and needs to be free'd by the caller.
 *        The first character of each line is the expansion type.
 *
 */


PSTR
FTGetWatchList(
    PTRVIT pVit
    )
{
    PTRVIB  pVib;
    PSTR    pBuffer;                    // Pointer to start of buffer
    PSTR    pCh;                        // Pointer to End of buffer
    PSTR    pName;                      // Pointer to next string
    PSTR    pType;                      // Pointer to the expansion string

    ULONG    Size = NAMES_BUFFER;       // Size of Buffer
    ULONG    Cnt = 0;                   // Count of Lines
    ULONG    Len = 1;                   // Lenth of filled buffer


    //
    // If we can't get a buffer, give up
    //

    if ( !pVit || (pBuffer = (PSTR) malloc(NAMES_BUFFER)) == NULL) {
        return(NULL);
    }
    pCh  = pBuffer;
    *pCh = 0;

    //
    // Loop through the VIT
    //

    pVib = pVit->pvibChild;
    while ( pVib ) {

        //  Get the values

        pName = FTGetVibNameString(pVib);
        pType = FTGetVibTypeString(pVib);

        //  Make sure we fit in the buffer

        if ( Len + strlen(pName) + 3 > Size ) {
            Size += NAMES_BUFFER;
            pBuffer = (PSTR) realloc(pBuffer, Size);
            pCh = pBuffer + Len - 1 ;   // Reposition to the same offset
        }

        //  Copy the string and point to the end

        *pCh++ = *pType;
        strcpy(pCh, pName);
        pCh += strlen(pName);

        //  Copy in the carriage return/linefeed

        *pCh++ = '\r';
        *pCh++ = '\n';

        //  Increment the length and get next vib

        Len += strlen(pName) + 3;
        pVib = pVib->pvibSib;
    }

    *pCh = 0;
    return(pBuffer);
}

/***    FTSetWatchList
 *
 *      Purpose:
 *        Restore a watch list that was generated by FTGetWatchList()
 *        for a given vit.
 *
 *      Input:
 *        pVit   - Pointer to the Variable Information Top
 *        List   - Save area generated by FTGetWatchList()
 *
 *      Output:
 *        None
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */

VOID
FTSetWatchList(
    PTRVIT pVit,
    PSTR list
    )
{
    PSTR   pWatch;
    PTRVIB pVib;
    char   cType;

    // While we have line with \r\n

    pWatch = strtok(list, "\r\n");
    while ( pWatch ) {

        if (IsDBCSLeadByte(*pWatch)) {
            pWatch = strtok( NULL, "\r\n"); // Get the Next Watch
            continue;
        }
        cType = *pWatch++;                  // Get the Expansion Type
        pVib  = AddCVWatch(pVit, pWatch);   // Add the Watch


#if V-WILLHE
        //
        // NOTENOTE - V-Willhe  If we do this we get free() areas.  The
        //                    vib is ok....check for corruption in
        //                    FTExpandVib.
        //

        if ( pVib && cType == '-') {        // If it was expanded,
            FTExpandVib( pVib);             //  Make it so...
        }
#endif
        pWatch = strtok( NULL, "\r\n");     // Get the Next Watch
    }
    return;
}

/***    FTInvalidateVib
 *
 *      Purpose:
 *        To invalidate the entire tree starting at a given node.
 *
 *      Input:
 *        pVib   - Pointer to a VIB
 *
 *      Output:
 *        None
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


VOID
FTInvalidateVib(
    PTRVIB pvib
    )
{
    while ( pvib) {

        // Make sure this one is invalidated
        pvib->flags.NoBind = TRUE;

        // If we have any children, invalidate them
        if ( pvib->pcif ) {
            FTInvalidateVib( pvib->pcif->pvibChild);
        }

        // Now take care of the siblings
        pvib = pvib->pvibSib;
    }
}

/***    FTResetVib
 *
 *      Purpose:
 *        Reset any NoBind flags from this vib down
 *
 *      Input:
 *        pVib   - Pointer to a VIB
 *
 *      Output:
 *        None
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


VOID
FTResetVib(
    PTRVIB pvib
    )
{
    while ( pvib) {

        // Make sure this one is turned off
        pvib->flags.NoBind = FALSE;

        // If we have any children, reset them
        if ( pvib->pcif ) {
            FTResetVib( pvib->pcif->pvibChild);
        }

        // Now take care of the siblings
        pvib = pvib->pvibSib;
    }
}


/***    FTAgeVibValues
 *
 *      Purpose:
 *        Age the Vib's value strings by moving it to the previous value.
 *        If we have a previous value free it.  Null the current value.
 *
 *      Input:
 *        pVib   - Pointer to a VIB
 *
 *      Output:
 *        None
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


VOID
FTAgeVibValues(
    PTRVIB pvib
    )
{
    ULONG i;

    while ( pvib) {

        FTEnsureTextExists( pvib );
        if ( pvib->cText ) {

            // For each text segment

            for ( i=0; i < pvib->cText ; i++) {

                // Free the Prev. If it exists
                if ( pvib->pvtext[i].pszValueP ) {
                    free(pvib->pvtext[i].pszValueP);
                }

                // Move Current to Prev. and Null current
                pvib->pvtext[i].pszValueP = pvib->pvtext[i].pszValueC;
                pvib->pvtext[i].pszValueC = NULL;
            }
        }

        // If we have any children,  Age them

        if ( pvib->pcif ) {
            FTAgeVibValues( pvib->pcif->pvibChild);
        }

        // Now take care of the siblings
        pvib = pvib->pvibSib;
    }
}


/***    FTGetPanelString
 *
 *      Purpose:
 *        Get the String associated with a vib index.
 *
 *      Input:
 *        UINT PanelNumber - Panel whos string we need (BUTTON, LEFT, RIGHT)
 *        UINT VibINdex    - Index of the Vib we need
 *
 *      Output:
 *        PSTR Pointer to the buffer containing the string.
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


PSTR
FTGetPanelString(
    PTRVIT pVit,
    PTRVIB pVib,
    UINT PanelNumber
    )
{
    PVTEXT  pvtext;

    FTEnsureTextExists( pVib );

    //  Focus in on the one we want
    switch (PanelNumber) {

        case ID_PANE_BUTTON:
            return(FTGetVibTypeString(pVib));

        case ID_PANE_LEFT:
            return(FTGetVibNameString(pVib));

        case ID_PANE_RIGHT:
            pvtext = &pVib->pvtext[pVib->vibIndex];
            if ( pvtext->pszValueC == NULL) {
                pvtext->pszValueC = _strdup(FTGetVibResultString(pVit,pVib));
            }
            return(pvtext->pszValueC);

        default:
            return(NULL);
    }

}

/***    FTGetPanelStatus
 *
 *      Purpose:
 *        Has the text for a panel id changed?
 *
 *      Input:
 *        UINT PanelNumber - Panel whos string we need (BUTTON, LEFT, RIGHT)
 *        UINT VibINdex    - Index of the Vib we need
 *
 *      Output:
 *        TRUE/FALSE
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */


BOOL
FTGetPanelStatus(
    PTRVIB pVib,
    UINT PanelNumber
    )
{
    PVTEXT  pvtext;

    if ( PanelNumber == ID_PANE_RIGHT) {
        FTEnsureTextExists( pVib );
        pvtext = &pVib->pvtext[pVib->vibIndex];
        if ( pvtext->pszValueC && pvtext->pszValueP) {
            return ( strcmp( pvtext->pszValueC, pvtext->pszValueP) );
        } else {
            return(FALSE);
        }
    }

    else {
        return(FALSE);
    }
}


/***    FTEnsureTextExists
 *
 *      Purpose:
 *        Ensure that the Text segment for a vib exists
 *
 *      Input:
 *        PRTVIB pVib  - Pointer to the vib to check
 *
 *      Output:
 *
 *      Exceptions:
 *
 *      Notes:
 *
 */

VOID
FTEnsureTextExists(
    PTRVIB pVib
    )
{
    int     count;
    int     slack;

    //  Make sure we have a text segment
    if ( pVib->pvtext == NULL) {
        pVib->pvtext = (PVTEXT) malloc( sizeof(VTEXT) );
        RAssert(pVib->pvtext);
        memset(pVib->pvtext,0, sizeof(VTEXT));
        pVib->cText = 1;
    }

    //  Make sure the Indicated index has text associated
    //  with it, if not expand in 10 entry increments

    if ( pVib->vibIndex + 1 > pVib->cText) {
        count = pVib->vibIndex + 11;
        slack = count - pVib->cText;
        pVib->pvtext = (PVTEXT) realloc(pVib->pvtext, count * sizeof(VTEXT) );
        RAssert(pVib->pvtext);
        memset(&pVib->pvtext[pVib->cText], 0, slack * sizeof(VTEXT) );
        pVib->cText = count;
    }

}


BOOL
FTAddWatchVariable(
                   PTRVIT * ppVit,
                   PTRVIB * ppVib,
                   LPSTR lpszWatchVar
                   )
/*++
Routine Desciption:
    Adds an expression to the pvitWatch tree
  
Arguments:
    ppVit           - * If NULL, then this argument is ignored.
                      * If not NULL, then the current VIT will be returned.
    ppVib           - * If NULL, then this argument is ignored.
                      * If not NULL, then the new VIB will be returned.
    lpszWatchVar    - Can be NULL, or point to a zero length string, However 
                      nothing  will be added.

Return Value:
    FALSE           - Failure. If the expression could not be added.
    TRUE            - Success. The expression was added.
--*/
{
    PTRVIT pVit = NULL;
    PTRVIB pVib = NULL;

    if (!lpszWatchVar) {
        return FALSE;
    }

    while (whitespace(*lpszWatchVar)) {
        lpszWatchVar++;
    }
    
    if (strlen(lpszWatchVar) == 0) {
        return FALSE;
    }
    
    pVit = InitWatchVit();
    pVib = AddCVWatch( pVit, lpszWatchVar);

    if (ppVit) {
        *ppVit = pVit;
    }
    if (ppVib) {
        *ppVib = pVib;
    }

    return TRUE;
}