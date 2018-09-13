#include <windows.h>
#include <stdlib.h>
#include "scicalc.h"
#include "unifunc.h"
#include "..\ratpak\debug.h"

/**************************************************************************\
*                                                                          *
*                                                                          *
*                                                                          *
*    #          #                           #####                          *
*    #         #              #             #    #                         *
*    #         #  #        #  #             #    #                         *
*    #        ###            ###            #    #                         *
*    # # ###   #  # # ###  #  #   ###       #####  # ###  ###   ###        *
*    # ##   #  #  # ##   # #  #  #   #      #      ##    #   # #           *
*    # #    #  #  # #    # #  #  #####      #      #     ##### #           *
*    # #    #  #  # #    # #  #  #          #      #     #     #    ##     *
*    # #    #  #  # #    # #   #  ###       #      #      ###   ### ##     *
*                                                                          *
*                                                                          *
*              Infinte Precision Production Version                        *
*                                                                          *
\**************************************************************************/
//
// RETAIL version of NUMOBJ math that uses Infinite Precision
//
// History
//
//  16-Nov-1996 JonPa   Wrote it
//  whenever-97 ToddB   Rewrote it using improved ratpak model
//

/*****************************************************************\
*
* Generic Math Package support routines and variables
*
* History:
*   01-Dec-1996 JonPa   Wrote them
*   whenever-97 ToddB   Rewrote them
*
\*****************************************************************/

//
// Worker for NumObjRecalcConstants
//
//  Returns the nearest power of two
//
int QuickLog2( int iNum )
{
    int iRes = 0;

    // while first digit is a zero
    while ( !(iNum & 1) )
    {
        iRes++;
        iNum >>= 1;
    }

    // if our number isn't a perfect square
    if ( iNum = iNum >> 1 )
    {
        // find the largest digit
        while ( iNum = iNum >> 1 )
           ++iRes;

        // and then add two
        iRes += 2;
    }

    return iRes;
}

////////////////////////////////////////////////////////////////////////
//
//  UpdateMaxIntDigits
//
// determine the maximum number of digits needed for the current precision,
// word size, and base.  This number is conservative towards the small side
// such that there may be some extra bits left over.  The number of extra
// bits is returned.  For example, base 8 requires 3 bits per digit.  A word
// size of 32 bits allows for 10 digits with a remainder of two bits.  Bases
// that require variable numnber of bits (non-power-of-two bases) are approximated
// by the next highest power-of-two base (again, to be conservative and gaurentee
// there will be no over flow verse the current word size for numbers entered).
// Base 10 is a special case and always uses the base 10 precision (nPrecision).
void UpdateMaxIntDigits()
{
    extern int gcIntDigits;
    int iRemainderBits;

    if ( nRadix == 10 )
    {
        gcIntDigits = nPrecision;
        iRemainderBits = 0;
    }
    else
    {
        int log2;

        log2 = QuickLog2( nRadix );

        ASSERT( 0 != log2 );     // same as ASSERT( nRadix != 1 )

        gcIntDigits = dwWordBitWidth / log2;
        iRemainderBits = dwWordBitWidth % log2;
    }
}

void BaseOrPrecisionChanged( void ) 
{
    extern LONG dwWordBitWidth;
    extern int  gcIntDigits;

    UpdateMaxIntDigits();
    if ( 10 == nRadix )
    {
        // to prevent unwanted rounded digits from showing up in the
        // gcIntDigits + 1 spot during non-integer mode we don't want
        // to add the extra 1 that we ortherwise add
        ChangeConstants( nRadix, gcIntDigits );
    }
    else
    {
        ChangeConstants( nRadix, gcIntDigits+1 );
    }
}

/*****************************************************************\
*
* Unary functions
*
* History:
*   01-Dec-1996 JonPa   Wrote them
*   whenever-97 ToddB   Rewrote them
*
\*****************************************************************/

void NumObjInvert( PHNUMOBJ phno ) {
    DECLARE_HNUMOBJ( hno );

    NumObjAssign( &hno, HNO_ONE );
    divrat( &hno, *phno );
    NumObjAssign( phno, hno );
    NumObjDestroy( &hno );
}

void NumObjAntiLog10( PHNUMOBJ phno ) {
    DECLARE_HNUMOBJ( hno );

    NumObjSetIntValue( &hno, 10 );
    powrat( &hno, *phno );
    NumObjAssign( phno, hno );
    NumObjDestroy( &hno );
}

void NumObjNot( PHNUMOBJ phno )
{
    if ( nRadix == 10 )
    {
        intrat( phno );
        addrat( phno, HNO_ONE );
        NumObjNegate( phno );
    }
    else
    {
        ASSERT( (nHexMode >= 0) && (nHexMode <= 3) );
        ASSERT( phno );
        ASSERT( *phno );
        ASSERT( g_ahnoChopNumbers[ nHexMode ] );

        xorrat( phno, g_ahnoChopNumbers[ nHexMode ] );
    }
}

void NumObjSin( PHNUMOBJ phno )
{
    ASSERT(( nDecMode == ANGLE_DEG ) || ( nDecMode == ANGLE_RAD ) || ( nDecMode == ANGLE_GRAD ));

    sinanglerat( (PRAT *)phno, nDecMode );
    NumObjCvtEpsilonToZero( phno );
}

void NumObjCos( PHNUMOBJ phno )
{
    ASSERT(( nDecMode == ANGLE_DEG ) || ( nDecMode == ANGLE_RAD ) || ( nDecMode == ANGLE_GRAD ));

    cosanglerat( (PRAT *)phno, nDecMode );
    NumObjCvtEpsilonToZero( phno );
}

void NumObjTan( PHNUMOBJ phno )
{
    ASSERT(( nDecMode == ANGLE_DEG ) || ( nDecMode == ANGLE_RAD ) || ( nDecMode == ANGLE_GRAD ));

    tananglerat( (PRAT *)phno, nDecMode );
    NumObjCvtEpsilonToZero( phno );
}

/******************************************************************\
*
* Number format conversion routines
*
* History:
*   06-Dec-1996 JonPa   wrote them
\******************************************************************/
void NumObjSetIntValue( PHNUMOBJ phnol, LONG i ) {
    PRAT pr = NULL;

    pr = longtorat( i );
    NumObjAssign( phnol, (HNUMOBJ)pr );
    destroyrat(pr);
}

void NumObjGetSzValue( LPTSTR *ppszNum, HNUMOBJ hnoNum, INT nRadix, NUMOBJ_FMT fmt ) {
    LPTSTR psz;

    psz = putrat( &hnoNum, nRadix, fmt );

    if (psz != NULL) {
        if (*ppszNum != NULL) {
            NumObjFreeMem( *ppszNum );
        }
        *ppszNum = psz;
    }
}
