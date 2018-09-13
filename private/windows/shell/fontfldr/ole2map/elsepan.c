/***************************************************************************
 * ELSEPAN.C - ElseWare PANOSE(tm) 1.0 Font Mapper routines.
 *
 * $keywords: elsepan.c 1.5 19-Jul-93 11:15:09 AM$
 *
 * Copyright (C) 1991-93 ElseWare Corporation.  All rights reserved.
 ***************************************************************************/
#define ELSE_MAPPER_CORE
#include <windows.h>
#include "elsepan.h"

/* Sanity check: this is the poor man's way to make sure the mapstate
 * we get is valid. We init this value during startup and check it upon
 * entry to every API routine.
 *
 * Note it is a good idea to modify SANITY_VALUE every time you make a
 * significant change to the software just in case the mapper ends up
 * in an environment where there may be multiple copies of it running,
 * or if client software tries to save the mapstate (which is a no-no: it
 * should use the API to get and set the exposed mapstate variables).
 */

#ifndef NOELSEWEIGHTS

#define SANITY_VALUE           0xD0CACA12L

#else

#define SANITY_VALUE           0xD0CACA13L

#endif

#define M_SANE(lpMapState) \
   (((lpMapState) != NULL) && ((lpMapState)->ulSanity == SANITY_VALUE))

#define M_lpjOFFS(lpDB, lOffs) (((EW_LPBYTE)(lpDB)) + (lOffs))

#ifndef M_ELSEMEMCPY

#define ELSELOCALMEMCPY

#define M_ELSEMEMCPY(dst, src, len) s_lpPANMemCpy((dst), (src), (len))
LOCAL EW_LPBYTE EW_NEAR EW_PASCAL s_lpPANMemCpy
   ELSEARGS (( EW_LPBYTE lpDst, EW_LPBYTE lpSrc, EW_USHORT unLen ));

#endif

LOCAL EW_LPPIND_MEM EW_NEAR EW_PASCAL s_lpPANGetIndRec
   ELSEARGS (( EW_LPPDICT_MEM lpPDB, EW_LPBYTE EW_FAR *lplpPanWant,
      EW_LPBYTE EW_FAR *lplpPanThis ));

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANGetPenaltyC0
   ELSEARGS (( EW_LPPIND_MEM lpPanIndRec, EW_LPPTBL_C0_MEM lpPC0,
      EW_LPUSHORT lpunMatch, EW_USHORT unTblSize, EW_USHORT unAttrA,
      EW_USHORT unAttrB ));

LOCAL EW_USHORT EW_NEAR EW_PASCAL s_unPANGetPenaltyC1
   ELSEARGS (( EW_USHORT unAttrA, EW_USHORT unAttrB ));

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANGetPenaltyC2
   ELSEARGS (( EW_LPPIND_MEM lpPanIndRec, EW_LPBYTE lpPTbl,
      EW_LPUSHORT lpunMatch, EW_USHORT unTblSize,
      EW_USHORT unAttrA, EW_USHORT unAttrB ));

LOCAL EW_USHORT EW_NEAR EW_PASCAL s_unPANGetPenaltyC4
   ELSEARGS (( EW_LPPTBL_C4_MEM lpPC4, EW_USHORT unAttrA,
      EW_USHORT unAttrB ));

LOCAL EW_LPBYTE EW_NEAR EW_PASCAL s_lpPANGetWeights
   ELSEARGS (( EW_LPMAPSTATE lpMapState, EW_LPPDICT_MEM lpPDB,
      EW_LPPIND_MEM lpPanIndRec ));

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANMatchDigits
   ELSEARGS (( EW_LPPDICT_MEM lpPDB, EW_LPUSHORT lpunMatchTotal,
      EW_LPPIND_MEM lpPanIndRec, EW_LPPTBL_MEM lpPTblRec, EW_USHORT unWt,
      EW_USHORT unAttrA, EW_USHORT unAttrB ));



/***************************************************************************
 * FUNCTION: nPANMapInit
 *
 * PURPOSE:  Initialize the font mapper.  Fill in the default settings.
 *
 * RETURNS:  Return the size of the map-state struct if successful, or
 *           the negative size if the passed in struct was too small.
 *           The function returns zero if it failed to initialize.
 ***************************************************************************/
EW_SHORT EW_FAR EW_PASCAL nPANMapInit( EW_LPMAPSTATE lpMapState,
                                       EW_USHORT unSizeMapState)
{
    EW_USHORT i;
    EW_LPPDICT_MEM lpPDB;
    EW_LPBYTE lpPanDef;
    EW_LPBYTE lpjWtA;
    EW_LPBYTE lpjWtB;
    
    //
    //  Simple version check: make sure we got the right size struct.
    //

    if( unSizeMapState < sizeof( EW_MAPSTATE ) )
    {
        if( unSizeMapState >= sizeof( EW_ULONG ) )
        {
            lpMapState->ulSanity = 0L;
        }
        return( -(EW_SHORT) sizeof( EW_MAPSTATE ) );
    }

    lpMapState->ulSanity = 0L;
    
    //
    //  Attempt to allocate the penalty database. We keep the handle
    //  until the mapper is disabled.
    //

    if( !( lpMapState->ulhPan1Data = M_lAllocPAN1DATA( ) ) )
    {
        goto errout0;
    }

    //
    //  Make sure the penalty database is the right version and
    //  in the right byte ordering.
    //

    if( !( lpPDB = M_lLockPAN1DATA( lpMapState->ulhPan1Data ) ) )
    {
        goto errout1;
    }

    if( ( lpPDB->unVersion != PANOSE_PENALTY_VERS ) ||
         ( lpPDB->unByteOrder != PTBL_BYTE_ORDER ) )
    {
        goto errout2;
    }

    M_bUnlockPAN1DATA( lpMapState->ulhPan1Data );
    
    //
    //  Fill in defaults.
    //

    lpMapState->unThreshold = ELSEDEFTHRESHOLD;
    lpMapState->unRelaxThresholdCount = 0;
    lpMapState->bUseDef = TRUE;
    
    //
    //  Initial default font is the PANOSE number for Courier.
    //

    lpPanDef = lpMapState->ajPanDef;

    lpPanDef[PAN_IND_FAMILY]     = FAMILY_LATTEXT;
    lpPanDef[PAN_IND_SERIF]      = SERIF_THIN;
    lpPanDef[PAN_IND_WEIGHT]     = WEIGHT_THIN;
    lpPanDef[PAN_IND_PROPORTION] = PROPORTION_MONOSPACE;
    lpPanDef[PAN_IND_CONTRAST]   = CONTRAST_NONE;
    lpPanDef[PAN_IND_STROKE]     = STROKE_GRADVERT;
    lpPanDef[PAN_IND_ARMSTYLE]   = ARM_STRAIGHTSGLSERIF;
    lpPanDef[PAN_IND_LTRFORM]    = LTRFORM_NORMCONTACT;
    lpPanDef[PAN_IND_MIDLINE]    = MIDLINE_STDSERIFED;
    lpPanDef[PAN_IND_XHEIGHT]    = XHEIGHT_CONSTLARGE;


#ifndef NOELSEWEIGHTS
    //
    //  Initialize the custom weights array.
    //

    for( i = 0, lpjWtA = lpMapState->ajWtRefA,
          lpjWtB = lpMapState->ajWtRefB;
          i < MAX_CUSTOM_WEIGHTS;
          ++i, *lpjWtA++ = PANOSE_ANY, *lpjWtB++ = PANOSE_ANY)
       ;

#endif
    
    //
    //  This value is checked by all other functions, in an attempt
    //  to safeguard against a mapstate that we didn't initialize.
    //

    lpMapState->ulSanity = SANITY_VALUE;
    
    //
    //  Normal return.
    //

    return( sizeof( EW_MAPSTATE ) );
    
errout2:
    M_bUnlockPAN1DATA(lpMapState->ulhPan1Data);

errout1:
    M_bFreePAN1DATA(lpMapState->ulhPan1Data);

errout0:
    return( 0 );
}


/***************************************************************************
 * FUNCTION: bPANMapClose
 *
 * PURPOSE:  Free the penalty database and close the font mapper. Also
 *           clear the sanity value so we will not service any more calls
 *           on this mapstate.
 *
 * RETURNS:  The function returns TRUE if the penalty database is
 *           successfully freed.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANMapClose( EW_LPMAPSTATE lpMapState )
{
    if( M_SANE( lpMapState ) )
    {
       lpMapState->ulSanity = 0L;

       return( M_bFreePAN1DATA( lpMapState->ulhPan1Data ) );
    }

    return( FALSE );
}


#ifndef NOELSEPICKFONTS

/***************************************************************************
 * FUNCTION: nPANGetMapDefault
 *
 * PURPOSE:  Fill in the passed-in PANOSE number structure with the
 *           default font.
 *
 * RETURNS:  Return 0 if the default number was not copied (passed-in
 *           structure too small), or NUM_PAN_DIGITS if it was.
 ***************************************************************************/

EW_SHORT EW_FAR EW_PASCAL nPANGetMapDefault( EW_LPMAPSTATE lpMapState,
                                             EW_LPBYTE lpPanDef,
                                             EW_USHORT unSizePanDef)
{
    //
    //  Sanity checks.
    //
    
    if( !M_SANE( lpMapState ) || ( unSizePanDef < SIZE_PAN1_NUM ) )
    {
        return( 0 );
    }

    //
    //  Copy the number.
    //

    M_ELSEMEMCPY( lpPanDef, lpMapState->ajPanDef, SIZE_PAN1_NUM );
    
    return( NUM_PAN_DIGITS );
}


/***************************************************************************
 * FUNCTION: nPANSetMapDefault
 *
 * PURPOSE:  Make the passed-in PANOSE number the new default font.  There
 *           is no sanity checking on the number.
 *
 * RETURNS:  Return 0 if the default number was not copied (passed-in
 *           structure too small), or NUM_PAN_DIGITS if it was.
 ***************************************************************************/

EW_SHORT EW_FAR EW_PASCAL nPANSetMapDefault( EW_LPMAPSTATE lpMapState,
                                             EW_LPBYTE lpPanDef,
                                             EW_USHORT unSizePanDef)
{
    //
    //  Sanity checks.
    //
    
    if( !M_SANE( lpMapState ) || ( unSizePanDef < SIZE_PAN1_NUM ) )
    {
       return( 0 );
    }
    
    //
    //  Copy the number.
    //
    
    M_ELSEMEMCPY( lpMapState->ajPanDef, lpPanDef, SIZE_PAN1_NUM );
    
    return( NUM_PAN_DIGITS );
}


/***************************************************************************
 * FUNCTION: bPANEnableMapDefault
 *
 * PURPOSE:  Enable/disable usage of the default font.
 *
 * RETURNS:  Return the previous usage state, or FALSE in the event of
 *           an error.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANEnableMapDefault( EW_LPMAPSTATE lpMapState,
                                               EW_BOOL bEnable)
{
    if( M_SANE( lpMapState ) )
    {
        EW_BOOL bPrev = lpMapState->bUseDef;

        lpMapState->bUseDef = bEnable;

        return( bPrev );
    }
    else
    {
        return( FALSE );
    }
}


/***************************************************************************
 * FUNCTION: bPANIsDefaultEnabled
 *
 * PURPOSE:  This function gets the state of using the default font.
 *
 * RETURNS:  Return TRUE if usage of the default font is enabled, and
 *           FALSE if it is not or an error occurred.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANIsDefaultEnabled( EW_LPMAPSTATE lpMapState )
{
    return( M_SANE( lpMapState ) && lpMapState->bUseDef );
}

#endif /* ifndef NOELSEPICKFONTS */


#ifndef NOELSETHRESHOLD

/***************************************************************************
 * FUNCTION: unPANGetMapThreshold
 *
 * PURPOSE:  This function gets the state of using threshold checking
 *           in the mapper.
 *
 * RETURNS:  Return the match threshold, or zero if an error occurred.
 ***************************************************************************/

EW_USHORT EW_FAR EW_PASCAL unPANGetMapThreshold( EW_LPMAPSTATE lpMapState )
{
    return( M_SANE( lpMapState ) ? lpMapState->unThreshold : 0 );
}


/***************************************************************************
 * FUNCTION: bPANSetMapThreshold
 *
 * PURPOSE:  Change the match threshold.
 *
 * RETURNS:  Return TRUE if the threshold is changed, FALSE if it is
 *           equal to the match error value and therefore rejected, or
 *           an error occurred.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANSetMapThreshold( EW_LPMAPSTATE lpMapState,
                                              EW_USHORT unThreshold)
{
    //
    //  Cannot set a threshold equal to the error value.
    //
    
    if( !M_SANE( lpMapState ) || ( unThreshold == PAN_MATCH_ERROR ) )
    {
       return( FALSE );
    }
    
    //
    //  Set new threshold.
    //
    
    lpMapState->unThreshold = unThreshold;
    
    return( TRUE );
}


/***************************************************************************
 * FUNCTION: bPANIsThresholdRelaxed
 *
 * PURPOSE:  This function gets the state of using the threshold in
 *           mapping.
 *
 * RETURNS:  Return TRUE if the match threshold is relaxed, or FALSE if
 *           it is not or an error occurred.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANIsThresholdRelaxed( EW_LPMAPSTATE lpMapState )
{
    return( M_SANE( lpMapState ) &&( lpMapState->unRelaxThresholdCount > 0 ) );
}


/***************************************************************************
 * FUNCTION: vPANRelaxThreshold
 *
 * PURPOSE:  Temporarily relax the threshold variable so every font
 *           except the erroneous ones will return a match value.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

EW_VOID EW_FAR EW_PASCAL vPANRelaxThreshold( EW_LPMAPSTATE lpMapState )
{
    if( M_SANE( lpMapState ) )
    {
       ++lpMapState->unRelaxThresholdCount;
    }
}


/***************************************************************************
 * FUNCTION: bPANRestoreThreshold
 *
 * PURPOSE:  Restore mapping within a threshold.
 *
 * RETURNS:  Return TRUE if the threshold is back in effect or an error
 *           occurred, FALSE if someone else has relaxed it too so it
 *           still is relaxed. We return TRUE on error in the event someone
 *           rights a 'for' loop restoring until TRUE is returned.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANRestoreThreshold( EW_LPMAPSTATE lpMapState )
{
    if( M_SANE( lpMapState ) &&( lpMapState->unRelaxThresholdCount > 0 ) )
    {
       return( --lpMapState->unRelaxThresholdCount == 0 );
    }
    else
    {
       return( TRUE );
    }
}

#endif /* ifndef NOELSETHRESHOLD */


#ifndef NOELSEWEIGHTS

/***************************************************************************
 * FUNCTION: bPANGetMapWeights
 *
 * PURPOSE:  Retrieve the mapper weight values for the passed-in family
 *           digits pair. The variable *lpbIsCustom is set if custom
 *           mapper weights have been set by the caller.
 *
 *           The weights array is an array of 10 bytes corresponding to
 *           the 10 PANOSE digits. The first weight is ignored (and usually
 *           set to zero) because we never actually assess a weighted
 *           penalty on the family digit. We include it so the index
 *           constants may be used to access the values in the weights
 *           array.
 *
 * RETURNS:  Return TRUE if mapper weights were retrieved/available (it is
 *           legal for the caller to pass in NULL for lpjWts), or FALSE
 *           if none exist.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANGetMapWeights( EW_LPMAPSTATE lpMapState,
                                            EW_BYTE jFamilyA,
                                            EW_BYTE jFamilyB,
                                            EW_LPBYTE lpjWts,
                                            EW_LPBOOL lpbIsCustom)
{
    EW_USHORT      i;
    EW_BOOL        bFound = FALSE;
    EW_LPPDICT_MEM lpPDB;
    EW_LPPIND_MEM  lpPanIndRec;
    EW_LPBYTE      lpjWtA;
    EW_LPBYTE      lpjWtB;
    

    //
    //  Sanity test on the family digits.
    //

    if( !M_SANE( lpMapState ) ||
         ( jFamilyA <= PANOSE_NOFIT ) ||( jFamilyA > MAX_PAN1_FAMILY ) ||
         ( jFamilyB <= PANOSE_NOFIT ) ||( jFamilyB > MAX_PAN1_FAMILY ) )
    {
        return( FALSE );
    }

    //
    //  Search for custom weights.
    //

    for( i = 0, lpjWtA = lpMapState->ajWtRefA, lpjWtB = lpMapState->ajWtRefB;
          !bFound && ( i < MAX_CUSTOM_WEIGHTS ) && *lpjWtA;
          ++i, ++lpjWtA, ++lpjWtB)
    {
        //
        //  If custom weights are found then set *lpbIsCustom to
        //  TRUE, copy the weights, and return success.
        //
        
        if( ( (*lpjWtA == jFamilyA ) &&( *lpjWtB == jFamilyB ) ) ||
             ( (*lpjWtA == jFamilyB ) &&( *lpjWtB == jFamilyA ) ) )
        {
            if( lpjWts )
            {
                M_ELSEMEMCPY( lpjWts,
                              &lpMapState->ajCustomWt[SIZE_PAN1_NUM * i],
                              SIZE_PAN1_NUM );
            }

            if( lpbIsCustom )
            {
                *lpbIsCustom = TRUE;
            }

            bFound = TRUE;
        }
    }

    //
    //  No custom weights available. Search the penalty database
    //  for default weights.
    //

    if( !bFound && ( lpPDB = M_lLockPAN1DATA( lpMapState->ulhPan1Data ) ) )
    {
        for( i = 0, lpPanIndRec = lpPDB->pind;
             !bFound && ( i < lpPDB->unNumDicts );
             ++i, ++lpPanIndRec )
        {
            if( ( (lpPanIndRec->jFamilyA == jFamilyA ) &&
                (  lpPanIndRec->jFamilyB == jFamilyB ) ) ||
                ( (lpPanIndRec->jFamilyA == jFamilyB ) &&
                (  lpPanIndRec->jFamilyB == jFamilyA ) ) )
            {
                if( lpPanIndRec->unOffsWts )
                {
                    if( lpjWts )
                    {
                        M_ELSEMEMCPY( lpjWts,
                                      M_lpjOFFS( lpPDB, lpPanIndRec->unOffsWts ),
                                      SIZE_PAN1_NUM );
                    }

                    if( lpbIsCustom )
                    {
                        *lpbIsCustom = FALSE;
                    }

                    bFound = TRUE;
                }
            }
        }

        M_bUnlockPAN1DATA( lpMapState->ulhPan1Data );
    }

    //
    //  Return the result of the search.
    //

    return( bFound );
}


/***************************************************************************
 * FUNCTION: bPANSetMapWeights
 *
 * PURPOSE:  Set the mapper weight values for the passed-in family
 *           digits pair.
 *
 *           The weights array is an array of 10 bytes corresponding to
 *           the 10 PANOSE digits. The first weight is ignored (and usually
 *           set to zero) because we never actually assess a weighted
 *           penalty on the family digit. We include it so the index
 *           constants may be used to access the values in the weights
 *           array.
 *
 * RETURNS:  Return TRUE if mapper weights were set, or FALSE if this
 *           family pair is not supported by the mapper or there is no
 *           more room for custom mapper weights.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANSetMapWeights( EW_LPMAPSTATE lpMapState,
                                            EW_BYTE jFamilyA,
                                            EW_BYTE jFamilyB,
                                            EW_LPBYTE lpjWts )
{
    EW_USHORT      i;
    EW_BOOL        bFound;
    EW_LPPDICT_MEM lpPDB;
    EW_LPPIND_MEM  lpPanIndRec;
    EW_LPBYTE      lpjWtA;
    EW_LPBYTE      lpjWtB;
    EW_LPBYTE      lpjWtFam;
    

    //
    //  Sanity test on the family digits.
    //

    if( !M_SANE( lpMapState ) || !lpjWts ||
         ( jFamilyA <= PANOSE_NOFIT ) ||( jFamilyA > MAX_PAN1_FAMILY ) ||
         ( jFamilyB <= PANOSE_NOFIT ) ||( jFamilyB > MAX_PAN1_FAMILY ) )
    {
        return( FALSE );
    }

    //
    //  First make sure this family pair exists in the penalty
    //  database (it does not make sense to store penalties for
    //  a family pair we'll never map against).
    //

    if( lpPDB = M_lLockPAN1DATA( lpMapState->ulhPan1Data ) )
    {
        for( i = 0, bFound = FALSE, lpPanIndRec = lpPDB->pind;
             i < lpPDB->unNumDicts; ++i, ++lpPanIndRec)
        {
            if( ( (lpPanIndRec->jFamilyA == jFamilyA ) &&
                (  lpPanIndRec->jFamilyB == jFamilyB ) ) ||
                ( (lpPanIndRec->jFamilyA == jFamilyB ) &&
                (  lpPanIndRec->jFamilyB == jFamilyA ) ) )
            {
                bFound = TRUE;
                break;
            }
        }

        M_bUnlockPAN1DATA( lpMapState->ulhPan1Data );

        if( !bFound )
        {
            return( FALSE );
        }
    }
    else
    {
        return( FALSE );
    }

    //
    //  Search for an existing entry.
    //

    for( i = 0, lpjWtA = lpMapState->ajWtRefA, lpjWtB = lpMapState->ajWtRefB;
         ( i < MAX_CUSTOM_WEIGHTS ) && *lpjWtA;
          ++i, ++lpjWtA, ++lpjWtB)
    {
        if( ( (*lpjWtA == jFamilyA ) &&( *lpjWtB == jFamilyB ) ) ||
            ( (*lpjWtA == jFamilyB ) &&( *lpjWtB == jFamilyA ) ) )
        {
            break;
        }
    }

    //
    //  Abort if the weights were not found and there are no free slots.
    //

    if( i >= MAX_CUSTOM_WEIGHTS )
    {
        return( FALSE );
    }

    //
    //  We either found the previous weights or have a free slot,
    //  in both cases copy the passed-in weights. For aesthetics,
    //  preserve zero for the family weight( it is not used ).
    //

    *lpjWtA = jFamilyA;
    *lpjWtB = jFamilyB;

    M_ELSEMEMCPY( lpjWtFam = &lpMapState->ajCustomWt[SIZE_PAN1_NUM * i],
                  lpjWts, SIZE_PAN1_NUM);

    *lpjWtFam = 0;
    
    //
    //  Return success.
    //

    return( TRUE );
}


/***************************************************************************
 * FUNCTION: bPANClearMapWeights
 *
 * PURPOSE:  Locate the custom mapper weights for the passed-in family
 *           digit pair and clear them, thus causing the mapper to revert
 *           back to using the default weights.
 *
 * RETURNS:  Return TRUE if custom mapper weights were located and cleared,
 *           FALSE if there are no custom weights for the passed-in family
 *           digit pair.
 ***************************************************************************/

EW_BOOL EW_FAR EW_PASCAL bPANClearMapWeights( EW_LPMAPSTATE lpMapState,
                                              EW_BYTE jFamilyA,
                                              EW_BYTE jFamilyB )
{
    EW_USHORT i;
    EW_USHORT j;
    EW_LPBYTE lpjWtA;
    EW_LPBYTE lpjWtB;
    

    //
    //  Sanity test on the family digits.
    //

    if( !M_SANE( lpMapState ) ||
         ( jFamilyA <= PANOSE_NOFIT ) ||( jFamilyA > MAX_PAN1_FAMILY ) ||
         ( jFamilyB <= PANOSE_NOFIT ) ||( jFamilyB > MAX_PAN1_FAMILY ) )
    {
        return( FALSE );
    }

    //
    //  Search for custom weights.
    //

    for( i = 0, lpjWtA = lpMapState->ajWtRefA, lpjWtB = lpMapState->ajWtRefB;
         ( i < MAX_CUSTOM_WEIGHTS ) && *lpjWtA;
          ++i, ++lpjWtA, ++lpjWtB)
    {
        //
        //  If custom weights are found then overwrite them by
        //  shifting other weights forward in the array.
        //

        if( ( (*lpjWtA == jFamilyA ) &&( *lpjWtB == jFamilyB ) ) ||
            ( (*lpjWtA == jFamilyB ) &&( *lpjWtB == jFamilyA ) ) )
        {
            for( j = i + 1, ++lpjWtA, ++lpjWtB;
                 ( j < MAX_CUSTOM_WEIGHTS ) && *lpjWtA;
                  ++j, ++lpjWtA, ++lpjWtB)
            {
                lpjWtA[-1] = *lpjWtA;
                lpjWtB[-1] = *lpjWtB;
            }

            lpjWtA[-1] = PANOSE_ANY;
            lpjWtB[-1] = PANOSE_ANY;

            if( i < ( j - 1 ) )
            {
                M_ELSEMEMCPY( &lpMapState->ajCustomWt[SIZE_PAN1_NUM * i],
                              &lpMapState->ajCustomWt[SIZE_PAN1_NUM * (i + 1)],
                              ( SIZE_PAN1_NUM * (j - i - 1 ) ) );
            }

            return( TRUE );
        }
    }

    //
    //  Custom weights matching this family digit pair were not
    //  found, return failure.
    //

    return( FALSE );
}


#endif /* ifndef NOELSEWEIGHTS */


/***************************************************************************
 * FUNCTION: unPANMatchFonts
 *
 * PURPOSE:  Match two PANOSE numbers.
 *
 * RETURNS:  Return a match value if the fonts are successfully compared
 *           and are within range of the threshold, otherwise return
 *           PAN_MATCH_ERROR if there is an error or the fonts are out
 *           of range.
 ***************************************************************************/

EW_USHORT EW_FAR EW_PASCAL unPANMatchFonts( EW_LPMAPSTATE lpMapState,
                                            EW_LPBYTE lpPanWant,
                                            EW_ULONG ulSizeWant,
                                            EW_LPBYTE lpPanThis,
                                            EW_ULONG ulSizeThis,
                                            EW_BYTE jMapToFamily )
{
    EW_USHORT unMatch = PAN_MATCH_ERROR;
    EW_USHORT unThreshold;
    EW_USHORT i;
    EW_USHORT j;
    EW_LPPDICT_MEM lpPDB;
    EW_LPPIND_MEM lpPanIndRec;
    EW_LPPTBL_MEM lpPTblRec;
    EW_LPBYTE lpjWts;
    EW_LPATOB_MEM lpAtoBHead;
    EW_LPATOB_ITEM_MEM lpAtoB;


    //
    //  Sanity check on the PANOSE numbers. Both numbers must be
    //  valid PANOSE 1.0 numbers, and the 'this'( compared-to )
    //  number must match the map-to family.
    //

    if( !M_SANE( lpMapState ) ||
         ( ulSizeWant != SIZE_PAN1_NUM ) || ( ulSizeThis != SIZE_PAN1_NUM ) ||
         ( lpPanWant[PAN_IND_FAMILY] <= PANOSE_NOFIT )   ||
         ( lpPanWant[PAN_IND_FAMILY] > MAX_PAN1_FAMILY ) ||
         ( lpPanThis[PAN_IND_FAMILY] <= PANOSE_NOFIT )   ||
         ( lpPanThis[PAN_IND_FAMILY] > MAX_PAN1_FAMILY ) ||
         ( lpPanThis[PAN_IND_FAMILY] != jMapToFamily ) )
    {
        goto backout0;
    }

    //
    //  Lock the penalty database.
    //

    if( !(lpPDB = M_lLockPAN1DATA( lpMapState->ulhPan1Data ) ) )
    {
        goto backout0;
    }

    //
    //  Locate the index entry that points to the dictionary containing
    //  the penalty tables for this PANOSE number.
    //  This routine may flip what lpPanWant and lpPanThis point to so
    //  we can guarantee the 'FamilyA' from the penalty tables is always
    //  associated with lpPanWant and 'FamilyB' is associated with
    //  lpPanThis.
    // 
    //  Optimization for unsupported families: If we do not support this
    //  family, but the numbers are identical, then return an 'exact match'
    //  value of zero. Otherwise return the usual match error value.
    //

    if( !(lpPanIndRec = s_lpPANGetIndRec(lpPDB, &lpPanWant, &lpPanThis ) ) )
    {
        for( i = 0; ( i < NUM_PAN_DIGITS ) && ( *lpPanWant == *lpPanThis ) &&
             ( *lpPanWant != PANOSE_NOFIT );
              ++i, ++lpPanWant, ++lpPanThis)
           ;
        
        if( i >= NUM_PAN_DIGITS )
        {
            unMatch = 0;
        }

        goto backout1;
    }

    //
    //  Get the array of mapper weights -- this could be a custom array
    //  supplied by the user, or the default array from the penalty
    //  database.
    //

    if( !( lpjWts = s_lpPANGetWeights( lpMapState, lpPDB, lpPanIndRec ) ) )
    {
        goto backout1;
    }

    //
    //  If we are NOT supposed to do threshold testing then just set
    //  it to the maximum integer.
    //

    if( lpMapState->unRelaxThresholdCount > 0 )
    {
        unThreshold = ELSEMAXSHORT;
    }
    else
    {
        unThreshold = lpMapState->unThreshold;
    }

    //
    //  Index the penalty table array.
    //

    lpPTblRec = (EW_LPPTBL_MEM) M_lpjOFFS( lpPDB, lpPanIndRec->unOffsPTbl );
    
    //
    //  There are two flavors of walking the digits:
    // 
    //  1. For cross-family matching, we walk an array of indices mapping
    //     digits from one family to the digits of another.
    //  2. For normal( same family ) matching, we directly walk the digits.
    // 
    //  Test for an a-to-b array( cross-family matching ).
    //

    if( lpPanIndRec->unOffsAtoB )
    {
        //
        //  This is a cross-family mapping, get the a-to-b array head.
        //
        
        lpAtoBHead = (EW_LPATOB_MEM) M_lpjOFFS( lpPDB, lpPanIndRec->unOffsAtoB );
        
        //
        //  Walk the a-to-b array.
        //
        
        for( i = unMatch = 0, j = lpAtoBHead->unNumAtoB,
              lpAtoB = lpAtoBHead->AtoBItem;
              i < j;
              ++i, ++lpPTblRec, ++lpjWts, ++lpAtoB)
        {
            //
            //  Compare the two digits. Abort if the test fails or the
            //  accumulated match value is greater than the threshold.
            //

            if( !s_bPANMatchDigits( lpPDB, &unMatch, lpPanIndRec,
                  lpPTblRec, *lpjWts, lpPanWant[lpAtoB->jAttrA],
                  lpPanThis[lpAtoB->jAttrB]) ||
                 ( unMatch > unThreshold ) )
            {
                unMatch = PAN_MATCH_ERROR;
                goto backout1;
            }
        }
    }
    else
    {
        //
        //  Normal match: comparing PANOSE numbers from the same
        //  families. Walk the digits accumulating the match result.
        //
        
        for( i = unMatch = 0, ++lpPanWant, ++lpPanThis;
              i <( NUM_PAN_DIGITS - 1 );
              ++i, ++lpPTblRec, ++lpjWts, ++lpPanWant, ++lpPanThis )
        {
            //
            //  Compare the two digits. Abort if the test fails or the
            //  accumulated match value is greater than the threshold.
            //

            if( !s_bPANMatchDigits( lpPDB, &unMatch, lpPanIndRec,
                  lpPTblRec, *lpjWts, *lpPanWant, *lpPanThis) ||
                 ( unMatch > unThreshold ) )
            {
                unMatch = PAN_MATCH_ERROR;
                goto backout1;
            }
        }
    }

    //
    //  Return the match value. If it was out of range or an error
    //  occurred, then it will equal PAN_MATCH_ERROR.
    //

backout1:
    M_bUnlockPAN1DATA( lpMapState->ulhPan1Data );

backout0:
    return( unMatch );
}


#ifndef NOELSEPICKFONTS


/***************************************************************************
 * FUNCTION: unPANPickFonts
 *
 * PURPOSE:  Walk an array of fonts ordering them by the closest to the
 *           requested font.  If no font is within range of the threshold
 *           then look for the closest to the default font.  If still
 *           no font is found then just pick the first font in the list.
 *
 *           Implementation note: This proc assumes PANOSE 1.0 numbers.
 *           A future version of this proc will accept intermixed PANOSE
 *           1.0 and 2.0 numbers, and will call a callback routine to
 *           supply each record, instead of presuming it can walk an
 *           array of fixed-length records.
 *
 * RETURNS:  Return the number of fonts found to match the requested
 *           font, or zero if unNumInds == 0 or an error ocurred.
 *
 *           If no close match was found but the default font is enabled,
 *           then one is returned and *lpMatchValues == PAN_MATCH_ERROR.
 *
 *           If no suitable match was found and the default font is
 *           disabled, then zero is returned.
 ***************************************************************************/
EW_USHORT EW_FAR EW_PASCAL unPANPickFonts( EW_LPMAPSTATE lpMapState,
                                           EW_LPUSHORT lpIndsBest,
                                           EW_LPUSHORT lpMatchValues,
                                           EW_LPBYTE lpPanWant,
                                           EW_USHORT unNumInds,
                                           EW_LPBYTE lpPanFirst,
                                           EW_USHORT unNumAvail,
                                           EW_SHORT nRecSize,
                                           EW_BYTE jMapToFamily )
{
    EW_USHORT i;
    EW_USHORT j;
    EW_USHORT k;
    EW_USHORT unNumFound = 0;
    EW_USHORT unMatchValue;
    EW_USHORT unSavedThreshold;
    EW_LPUSHORT lpMatches;
    EW_LPUSHORT lpInds;
    EW_LPBYTE lpPanThis;
    
    //
    //  Sanity check.
    //

    if( !M_SANE( lpMapState ) || ( unNumInds == 0 ) || ( unNumAvail == 0 ) ||
         ( (nRecSize < 0 ) &&( nRecSize > -(EW_SHORT )SIZE_PAN1_NUM) ) ||
         ( (nRecSize > 0 ) &&( nRecSize < (EW_SHORT )SIZE_PAN1_NUM) ) )
    {
        return( 0 );
    }

    //
    //  This routine implements a 'quit early' algorithm by modifying
    //  the threshold to the worst acceptable value in the list (once
    //  the list is full). This has the effect of causing matchfonts
    //  to abort & return PAN_MATCH_ERROR whenever a penalty exceeds
    //  the threshold.
    //

    unSavedThreshold = lpMapState->unThreshold;
    
    //
    //  Walk the PANOSE numbers ordering them from best to worst
    //  match.  Walk the array with a byte pointer, advancing by
    //  the passed-in record size.
    //

    for( i = 0, lpPanThis = lpPanFirst; i < unNumAvail;
         ++i, lpPanThis += nRecSize)
    {
        //
        //  Get the match value.
        //

        if( ( unMatchValue = unPANMatchFonts( lpMapState,
              lpPanWant, SIZE_PAN1_NUM, lpPanThis, SIZE_PAN1_NUM,
              jMapToFamily ) ) != PAN_MATCH_ERROR )
        {
            //
            //  Find the slot in the array where this match value
            //  should reside.
            // 

            for( j = 0, lpMatches = lpMatchValues;
                ( j < unNumFound ) &&( *lpMatches < unMatchValue );
                ++j, ++lpMatches)
               ;
            
            //
            //  If this match value is better than one of the matches
            //  already in the array, then insert it.  Notice that
            //  until the array is full everything goes in it.  After
            //  that, we shuffle less close matches off the end.
            //

            if( j < unNumInds )
            {
                if( unNumFound < unNumInds )
                {
                   ++unNumFound;
                }

                for( lpInds = &lpIndsBest[k = unNumFound - 1],
                     lpMatches = &lpMatchValues[k];
                     k > j;
                     lpInds[0] = lpInds[-1], lpMatches[0] = lpMatches[-1],
                     --k, --lpInds, --lpMatches)
                    ;

                *lpInds = i;
                *lpMatches = unMatchValue;
                
                //
                //  If the list is full, then set the threshold equal
                //  to the last match value in the list. The matchfonts
                //  routine will abort & return PAN_MATCH_ERROR on any
                //  match greater than this value.
                // 
                //  Also, if the last value in the list is zero (exact
                //  match), then exit the loop because the list will
                //  not change.
                //

                if( unNumFound == unNumInds )
                {
                    if( (k = lpMatchValues[unNumFound - 1] ) == 0)
                    {
                        break;
                    }

                    lpMapState->unThreshold = k;
                }
            }
        }
    }

    //
    //  If no acceptable match was found, then attempt to find a match
    //  for the default font.  We temporarily step off the threshold
    //  so we will definitely find something.  At this point, we do
    //  not care if the default is not within the threshold, we just
    //  want to find it.
    //

    if( !unNumFound && lpMapState->bUseDef )
    {
        lpMapState->unThreshold = ELSEMAXSHORT;
    
        for( i = 0, lpPanThis = lpPanFirst; i < unNumAvail;
             ++i, lpPanThis += nRecSize)
        {
            if( ( unMatchValue = unPANMatchFonts( lpMapState,
                lpMapState->ajPanDef, SIZE_PAN1_NUM, lpPanThis, SIZE_PAN1_NUM,
                lpMapState->ajPanDef[PAN_IND_FAMILY] ) ) != PAN_MATCH_ERROR )
            {
                if( unNumFound == 0 )
                {
                    *lpIndsBest = i;
                    lpMapState->unThreshold = *lpMatchValues = unMatchValue;
                    ++unNumFound;
                }
                else if( unMatchValue < *lpMatchValues )
                {
                    *lpIndsBest = i;
                    lpMapState->unThreshold = *lpMatchValues = unMatchValue;
                }
            }
        }

        //
        //  We flag this match with the error so the caller can
        // determine that the default font was substituted.
        // 

        if( unNumFound > 0 )
        {
            *lpMatchValues = PAN_MATCH_ERROR;
        }
    }

    //
    //  Restore the threshold.
    // 

    lpMapState->unThreshold = unSavedThreshold;
    
    //
    //  If still no match is found then just pick the first font.
    //

    if( !unNumFound )
    {
        *lpIndsBest = 0;
        *lpMatchValues = PAN_MATCH_ERROR;
        ++unNumFound;
    }

    //
    //  Return the number of fonts found.  It will be zero if we
    //  encountered an error or couldn't find a suitable match.
    //

    return( unNumFound );
}


#endif /* ifndef NOELSEPICKFONTS */


/***************************************************************************
 * FUNCTION: vPANMakeDummy
 *
 * PURPOSE:  Build a dummy PANOSE number with all attributes set to
 *           PANOSE_NOFIT.
 *
 * RETURNS:  Nothing.
 ***************************************************************************/

EW_VOID EW_FAR EW_PASCAL vPANMakeDummy( EW_LPBYTE lpPanThis,
                                        EW_USHORT unSize )
{
    EW_USHORT i;
    EW_USHORT j;
    
    unSize /= sizeof( EW_BYTE );
    
    for( i = j = 0; (i < NUM_PAN_DIGITS ) &&( j < unSize );
       ++i, j += sizeof( EW_BYTE ), *lpPanThis++ = PANOSE_NOFIT)
       ;
}


/***************************************************************************/
/************************** LOCAL SERVICE ROUTINES *************************/
/***************************************************************************/


/***************************************************************************
 * FUNCTION: s_lpPANGetIndRec
 *
 * PURPOSE:  Search the header of the database looking for a dictionary
 *           of penalty tables designed for this family pair.
 *
 *           There is a similar search for the index rec in the routine
 *           bPANGetMapWeights. If you make a change here, also check in
 *           that routine.
 *
 * RETURNS:  Return the pointer to the index record if a match is found,
 *           or NULL if one is not.
 ***************************************************************************/

LOCAL EW_LPPIND_MEM EW_NEAR EW_PASCAL s_lpPANGetIndRec(
                                               EW_LPPDICT_MEM lpPDB,
                                               EW_LPBYTE EW_FAR *lplpPanWant,
                                               EW_LPBYTE EW_FAR *lplpPanThis )
{
    EW_USHORT i;
    EW_BYTE jFamilyA =( *lplpPanWant )[PAN_IND_FAMILY];
    EW_BYTE jFamilyB =( *lplpPanThis )[PAN_IND_FAMILY];
    EW_LPBYTE lpPanSwitch;
    EW_LPPIND_MEM lpPanIndRec;


    //
    //  Walk the index array in the penalty database looking for
    //  a matching family pair.
    //

    for( i = 0, lpPanIndRec = lpPDB->pind; i < lpPDB->unNumDicts;
          ++i, ++lpPanIndRec)
    {
        if( ( lpPanIndRec->jFamilyA == jFamilyA ) &&
            ( lpPanIndRec->jFamilyB == jFamilyB ) )
        {
            //
            //  Straight match. Return the index.
            //

            return( lpPanIndRec );
        
        }
        else if( ( lpPanIndRec->jFamilyA == jFamilyB ) &&
                 ( lpPanIndRec->jFamilyB == jFamilyA ) )
        {
            //
            //  There is a match but the families are swapped. Swap
            //  the PANOSE numbers to match the order in the penalty
            //  database in the event it contains tables that are
            //  order-dependent (this can happen with cross-family
            //  mapping, C0-style/uncompressed/non-symmetric tables).
            //

            lpPanSwitch = *lplpPanWant;

            *lplpPanWant = *lplpPanThis;
            *lplpPanThis = lpPanSwitch;

            return( lpPanIndRec );
        }
    }

    //
    //  No match found, return an error.
    // 

    return( NULL );
}


/***************************************************************************
 * FUNCTION: s_bPANGetPenaltyC0
 *
 * PURPOSE:  Compute the penalty between two PANOSE digits using 'C0'
 *           compression, where the entire table is provided (except
 *           the any and no-fit rows and columns).
 *
 * RETURNS:  Return TRUE if the computed index is within range, and
 *           *lpunMatch is filled in with the penalty value, FALSE if
 *           it is out of range.
 ***************************************************************************/

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANGetPenaltyC0( EW_LPPIND_MEM lpPanIndRec,
                                                    EW_LPPTBL_C0_MEM lpPC0,
                                                    EW_LPUSHORT lpunMatch,
                                                    EW_USHORT unTblSize,
                                                    EW_USHORT unAttrA,
                                                    EW_USHORT unAttrB )
{
    EW_USHORT unInd;


    //
    //  Make sure each value is within range.  Notice this may
    //  be a non-square table.
    //

    if( ( unAttrA > lpPC0->jARangeLast ) ||( unAttrB > lpPC0->jBRangeLast ) )
    {
        *lpunMatch = lpPanIndRec->jDefNoFitPenalty;

        return( FALSE );
    }

    //
    //  Compute the table index.
    //

    if( ( unInd = ( (unAttrA - 2 ) *(lpPC0->jBRangeLast - 1 ) )
                        + unAttrB - 2) >= unTblSize )
    {
        *lpunMatch = lpPanIndRec->jDefNoFitPenalty;

        return( FALSE );
    }

    //
    //  Get the penalty.
    //

    *lpunMatch = lpPC0->jPenalties[unInd];

    return( TRUE );
}


/***************************************************************************
 * FUNCTION: s_unPANGetPenaltyC1
 *
 * PURPOSE:  Compute the penalty between two PANOSE digits using 'C1'
 *           compression, which is a perfectly symmetrical table around
 *           the diagonal.  Two digits on the diagonal are an exact match.
 *           A difference of 1 yields a penalty of 1, a difference of 2
 *           yields a penalty of 2, and so on.
 *
 *           It is assumed the caller handled any, no-fit, and exact
 *           matches.
 *
 * RETURNS:  Return the penalty from the table, the function cannot fail.
 ***************************************************************************/
LOCAL EW_USHORT EW_NEAR EW_PASCAL s_unPANGetPenaltyC1( EW_USHORT unAttrA,
                                                       EW_USHORT unAttrB )
{
    EW_SHORT nDiff;
    

    //
    //  Compute the penalty, which is simply the absolute value
    //  of the difference between the two numbers.
    //

    if( ( nDiff = (EW_SHORT) unAttrA - (EW_SHORT) unAttrB ) < 0 )
    {
        nDiff = -nDiff;
    }

    return( nDiff );
}


/***************************************************************************
 * FUNCTION: s_bPANGetPenaltyC2
 *
 * PURPOSE:  Compute the penalty between two PANOSE digits using 'C2'
 *           compression, which is a table symmetrical about the
 *           diagonal, but not a smooth range from low to high, so the
 *           lower left corner of the table is provided.  The unAttrA
 *           digit references the row and unAttrB references the column.
 *
 *           It is assumed the caller handled any, no-fit, and exact
 *           matches.
 *
 * RETURNS:  Return TRUE if the computed index is within range, and
 *           *lpunMatch is filled in with the penalty value, FALSE if
 *           it is out of range.
 ***************************************************************************/

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANGetPenaltyC2( EW_LPPIND_MEM lpPanIndRec,
                                                    EW_LPBYTE lpPTbl,
                                                    EW_LPUSHORT lpunMatch,
                                                    EW_USHORT unTblSize,
                                                    EW_USHORT unAttrA,
                                                    EW_USHORT unAttrB )
{
    EW_USHORT unSwap;
    EW_SHORT nInd;


    //
    //  The formula we use assumes the lower left half of the
    //  penalty table, which means row > column.  The table is
    //  symmetric about the diagonal, so if row < column we can
    //  just switch their values.
    // 

    if( unAttrA < unAttrB )
    {
       unSwap = unAttrA;
       unAttrA = unAttrB;
       unAttrB = unSwap;
    }

    //
    //  The table is missing the any, no-fit, and exact match
    //  penalties as those are handled separately.  Since the
    //  table is triangular shaped, we use the additive series
    //  to compute the row:
    // 
    //    n + ... + 3 + 2 + 1 == 1/2 * n *( n + 1 )
    // 
    //  Substituting n for row - 3, the first possible row, and
    //  adding the column offset, we get the following formula:
    // 
    //   ( 1/2 * (row - 3 ) *( row - 2 ) ) +( col - 2 )
    // 
    //  We know that row >= 3 and col >= 2 as we catch the other
    //  cases above.
    // 

    if( ( nInd = M_ELSEMULDIV( unAttrA - 3, unAttrA - 2, 2 ) +
         (EW_SHORT) unAttrB - 2) >= (EW_SHORT) unTblSize )
    {
        *lpunMatch = lpPanIndRec->jDefNoFitPenalty;

        return( FALSE );
    }

    *lpunMatch = lpPTbl[nInd];

    return( TRUE );
}


/***************************************************************************
 * FUNCTION: s_unPANGetPenaltyC4
 *
 * PURPOSE:  Compute the penalty between two PANOSE digits using 'C4'
 *           compression, which is almost identical to  'C1' compression
 *           except a start and increment value are supplied.
 *
 *           It is assumed the caller handled any, no-fit, and exact
 *           matches.
 *
 * RETURNS:  Return the penalty from the table, the function cannot fail.
 ***************************************************************************/

LOCAL EW_USHORT EW_NEAR EW_PASCAL s_unPANGetPenaltyC4( EW_LPPTBL_C4_MEM lpPC4,
                                                       EW_USHORT unAttrA,
                                                       EW_USHORT unAttrB )
{
    EW_SHORT nDiff;
    

    //
    //  First compute the absolute value of the difference
    //  between the two numbers.
    // 

    if( (nDiff = (EW_SHORT )unAttrA -( EW_SHORT )unAttrB) < 0)
    {
       nDiff = -nDiff;
    }

    //
    //  Then scale by the increment and start values.
    // 

    if( nDiff > 0 )
    {
       nDiff = ( ( nDiff - 1 ) *(EW_SHORT) lpPC4->jIncrement ) +
                (EW_SHORT) lpPC4->jStart;
    }

    return( nDiff );
}


/***************************************************************************
 * FUNCTION: s_lpPANGetWeights
 *
 * PURPOSE:  Check the mapstate record for a set of user-supplied custom
 *           weights. If none are present, then use the default weights
 *           from the mapping table.
 *
 * RETURNS:  Return the pointer to the array of weight values.
 ***************************************************************************/

LOCAL EW_LPBYTE EW_NEAR EW_PASCAL s_lpPANGetWeights( EW_LPMAPSTATE lpMapState,
                                                     EW_LPPDICT_MEM lpPDB,
                                                     EW_LPPIND_MEM lpPanIndRec )
{
    EW_USHORT i;
    EW_LPBYTE lpjWtA;
    EW_LPBYTE lpjWtB;
    EW_BYTE jFamilyA = lpPanIndRec->jFamilyA;
    EW_BYTE jFamilyB = lpPanIndRec->jFamilyB;
    
#ifndef NOELSEWEIGHTS
    //
    //  Search for custom weights.
    // 

    for( i = 0, lpjWtA = lpMapState->ajWtRefA, lpjWtB = lpMapState->ajWtRefB;
         ( i < MAX_CUSTOM_WEIGHTS ) && *lpjWtA;
          ++i, ++lpjWtA, ++lpjWtB )
    {
        //
        //  If custom weights are found then return a pointer into
        //  the mapstate struct. We store a weight value for the family
        //  digit but do not use it. The pointer points to the first
        //  digit after the family digit.
        // 

        if( ( (*lpjWtA == jFamilyA ) &&( *lpjWtB == jFamilyB ) ) ||
            ( (*lpjWtA == jFamilyB ) &&( *lpjWtB == jFamilyA ) ) )
        {
            return( &lpMapState->ajCustomWt[ ( SIZE_PAN1_NUM * i ) + 1] );
        }
    }
#endif

    //
    //  If no custom weights were found then return the default
    //  weight from the penalty database.
    // 

    if( lpPanIndRec->unOffsWts )
    {
       return( M_lpjOFFS(lpPDB, lpPanIndRec->unOffsWts + 1 ) );
    }
    else
    {
       return( NULL );
    }
}


/***************************************************************************
 * FUNCTION: s_bPANMatchDigits
 *
 * PURPOSE:  Compute the match value between two PANOSE digits and add
 *           it to the passed in match total.
 *
 * RETURNS:  Return TRUE if the match value is computed and added to
 *           *lpunMatchTotal.  If an error occurs, return FALSE and
 *           set *lpunMatchTotal to the value PAN_MATCH_ERROR.
 ***************************************************************************/

LOCAL EW_BOOL EW_NEAR EW_PASCAL s_bPANMatchDigits( EW_LPPDICT_MEM lpPDB,
                                                   EW_LPUSHORT lpunMatchTotal,
                                                   EW_LPPIND_MEM lpPanIndRec,
                                                   EW_LPPTBL_MEM lpPTblRec,
                                                   EW_USHORT unWt,
                                                   EW_USHORT unAttrA,
                                                   EW_USHORT unAttrB )
{
    EW_USHORT unLast = lpPTblRec->jRangeLast;
    EW_USHORT unMatch;
    

    //
    //  First make sure the digit values are not out of range.
    // 

    if( (unAttrA > unLast ) ||( unAttrB > unLast ) )
    {
       goto errout;
    }

    //
    //  Special case no-fit, any, or exact matches.
    // 

    if( ( unAttrA == PANOSE_NOFIT ) || ( unAttrB == PANOSE_NOFIT ) )
    {
        if( lpPTblRec->jCompress != PAN_COMPRESS_C3 )
        {
           *lpunMatchTotal += lpPanIndRec->jDefNoFitPenalty * unWt;
           return( TRUE );
        }
    }
    else if( ( unAttrA == PANOSE_ANY ) || ( unAttrB == PANOSE_ANY ) )
    {
        *lpunMatchTotal += lpPanIndRec->jDefAnyPenalty * unWt;
        return( TRUE );
    }
    else if( (unAttrA == unAttrB ) &&
            ( lpPTblRec->jCompress != PAN_COMPRESS_C0 ) )
    {
        *lpunMatchTotal += lpPanIndRec->jDefMatchPenalty * unWt;
        return( TRUE );
    }

    //
    //  Compute the penalty depending on the kind of compression
    //  used for the table.
    // 

    switch( lpPTblRec->jCompress )
    {
    
        case PAN_COMPRESS_C0:
            if( !lpPTblRec->unOffsTbl || !lpPTblRec->unTblSize
                || !s_bPANGetPenaltyC0( lpPanIndRec,
                  (EW_LPPTBL_C0_MEM) M_lpjOFFS( lpPDB, lpPTblRec->unOffsTbl ),
                  &unMatch, lpPTblRec->unTblSize, unAttrA, unAttrB ) )
            {
                goto errout;
            }
            
            *lpunMatchTotal += unMatch * unWt;
            break;
        
        case PAN_COMPRESS_C1:
            *lpunMatchTotal += s_unPANGetPenaltyC1( unAttrA, unAttrB ) * unWt;
            break;
        
        case PAN_COMPRESS_C2:
            if( !lpPTblRec->unOffsTbl || !lpPTblRec->unTblSize ||
                !s_bPANGetPenaltyC2( lpPanIndRec,
                  M_lpjOFFS( lpPDB, lpPTblRec->unOffsTbl ), &unMatch,
                  lpPTblRec->unTblSize, unAttrA, unAttrB ) )
            {
                goto errout;
            }
            
            *lpunMatchTotal += unMatch * unWt;
            break;
        
        case PAN_COMPRESS_C3:
            if( !lpPTblRec->unOffsTbl || !lpPTblRec->unTblSize )
            {
                goto errout;
            }
            
            if( ( unAttrA == PANOSE_NOFIT ) || ( unAttrB == PANOSE_NOFIT ) )
            {
                unMatch = *M_lpjOFFS( lpPDB, lpPTblRec->unOffsTbl );
            }
            else if( !s_bPANGetPenaltyC2( lpPanIndRec,
                     M_lpjOFFS( lpPDB, lpPTblRec->unOffsTbl + 1 ), &unMatch,
                                (EW_USHORT) ( lpPTblRec->unTblSize - 1 ),
                                unAttrA, unAttrB ) )
            {
                goto errout;
            }
            
            *lpunMatchTotal += unMatch * unWt;
            break;
        
        case PAN_COMPRESS_C4:
            if( !lpPTblRec->unOffsTbl || !lpPTblRec->unTblSize )
            {
                goto errout;
            }
            
            *lpunMatchTotal += s_unPANGetPenaltyC4(
                (EW_LPPTBL_C4_MEM) M_lpjOFFS( lpPDB, lpPTblRec->unOffsTbl ),
                unAttrA, unAttrB) * unWt;
            break;
    }

    //
    //  Match computed, successful return.
    // 

    return( TRUE );
    
    //
    //  An error occurred, return FALSE.
    // 

errout:

    *lpunMatchTotal = PAN_MATCH_ERROR;

    return( FALSE );
}


/***************************************************************************
 * FUNCTION: s_lpPANMemCpy
 *
 * PURPOSE:  Perform a memcpy operation.
 *
 * RETURNS:  The function returns lpDst.
 ***************************************************************************/

#ifdef ELSELOCALMEMCPY
LOCAL EW_LPBYTE EW_NEAR EW_PASCAL s_lpPANMemCpy( EW_LPBYTE lpDst,
                                                 EW_LPBYTE lpSrc,
                                                 EW_USHORT unLen)
{
    EW_LPBYTE lpRet = lpDst;
    EW_USHORT i;

    for( i = 0; i < unLen; ++i, *lpDst++ = *lpSrc++ )
        ;

    return( lpRet );
}
#endif

/***************************************************************************
 * Revision log:
 *
 * 31-Jan-93  msd PANOSE 1.0 mapper: 10-digit PANOSE.
 *  2-Feb-93  msd Removed huge pointer stuff.
 *  3-Feb-93  msd Removed ctrl-z at EOF.  Added 'unused' pragmas.
 *  3-Feb-93  msd Fixed bug caused by vcs check-in.
 * 14-Feb-93  msd Removed extra restore-threshold call in pickfonts.
 * 15-Feb-93  msd For extra security, bumped the sanity value from
 *                word to a long.
 ***************************************************************************/
/*
 * $lgb$
 * 1.0    17-Feb-93    msd New module created because of vcs problems.
 * 1.1    17-Feb-93    msd Small doc change.
 * 1.2    18-Feb-93    msd Added penalty table byte-ordering check, and C4 ptbl compression( new version of ptbl ). Modified internal routines so 'unused' pragmas are not necessary. Use EW_FAR.
 * 1.3    23-Feb-93    msd On close session, kill the sanity value so subsequent mapper calls will fail.
 * 1.4    25-Feb-93    msd Modified the default font search logic in pickfonts -- search by the default font's family, not by the requested family. Also use M_ELSEMEMCPY() in a few more places.
 * 1.5    19-Jul-93    msd Added compilation flags to selectively disable mapper routines.
 * $lge$
 */
