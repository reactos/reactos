//----------------------------------------------------------------------------
//  File           trans.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-96 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains sin, cos and tan for rationals
//
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( DOS )
#include <dosstub.h>
#else
#include <windows.h>
#endif
#include <ratpak.h>


void scalerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    switch ( angletype )
        {
    case ANGLE_RAD:
        scale2pi( pa );
        break;
    case ANGLE_DEG:
        scale( pa, rat_360 );
        break;
    case ANGLE_GRAD:
        scale( pa, rat_400 );
        break;
        }
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: sinrat, _sinrat
//
//  ARGUMENTS:  x PRAT representation of number to take the sine of
//
//  RETURN: sin of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___    2j+1   j
//   \  ]  X     -1
//    \   ---------
//    /    (2j+1)!
//   /__]
//   j=0
//          or,
//    n
//   ___                                                 2
//   \  ]                                              -X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j)*(2j+1)
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//-----------------------------------------------------------------------------


void _sinrat( PRAT *px )

{
    CREATETAYLOR();

    DUPRAT(pret,*px); 
    DUPRAT(thisterm,*px);

    DUPNUM(n2,num_one);
    xx->pp->sign *= -1;

    do    {
        NEXTTERM(xx,INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
    
    // Since *px might be epsilon above 1 or below -1, due to TRIMIT we need 
    // this trick here.
    inbetween(px,rat_one);
    
    // Since *px might be epsilon near zero we must set it to zero.
    if ( rat_le(*px,rat_smallest) && rat_ge(*px,rat_negsmallest) )
        {
        DUPRAT(*px,rat_zero);
        }
}

void sinrat( PRAT *px )
{
    scale2pi(px); 
    _sinrat(px); 
}

void sinanglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    scalerat( pa, angletype );
    switch ( angletype )
        {
    case ANGLE_DEG:
        if ( rat_gt( *pa, rat_180 ) )
            {
            subrat(pa,rat_360);
            }
        divrat( pa, rat_180 );
        mulrat( pa, pi );
        break;
    case ANGLE_GRAD:
        if ( rat_gt( *pa, rat_200 ) )
            {
            subrat(pa,rat_400);
            }
        divrat( pa, rat_200 );
        mulrat( pa, pi );
        break;
        }
    _sinrat( pa );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: cosrat, _cosrat
//
//  ARGUMENTS:  x PRAT representation of number to take the cosine of
//
//  RETURN: cosin of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___    2j   j
//   \  ]  X   -1
//    \   ---------
//    /    (2j)!
//   /__]
//   j=0
//          or,
//    n
//   ___                                                 2
//   \  ]                                              -X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j)*(2j+1)
//   /__]
//   j=0
//
//   thisterm  = 1 ;  and stop when thisterm < precision used.
//           0                              n
//
//-----------------------------------------------------------------------------


void _cosrat( PRAT *px )

{
    CREATETAYLOR();

    pret->pp=longtonum( 1L, nRadix );
    pret->pq=longtonum( 1L, nRadix );

    DUPRAT(thisterm,pret)

    n2=longtonum(0L, nRadix);
    xx->pp->sign *= -1;

    do    {
        NEXTTERM(xx,INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
    // Since *px might be epsilon above 1 or below -1, due to TRIMIT we need 
    // this trick here.
    inbetween(px,rat_one);
    // Since *px might be epsilon near zero we must set it to zero.
    if ( rat_le(*px,rat_smallest) && rat_ge(*px,rat_negsmallest) )
        {
        DUPRAT(*px,rat_zero);
        }
}

void cosrat( PRAT *px )
{
    scale2pi(px); 
    _cosrat(px); 
}

void cosanglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    scalerat( pa, angletype );
    switch ( angletype )
        {
    case ANGLE_DEG:
        if ( rat_gt( *pa, rat_180 ) )
            {
            PRAT ptmp=NULL;
            DUPRAT(ptmp,rat_360);
            subrat(&ptmp,*pa);
            destroyrat(*pa);
            *pa=ptmp;
            }
        divrat( pa, rat_180 );
        mulrat( pa, pi );
        break;
    case ANGLE_GRAD:
        if ( rat_gt( *pa, rat_200 ) )
            {
            PRAT ptmp=NULL;
            DUPRAT(ptmp,rat_400);
            subrat(&ptmp,*pa);
            destroyrat(*pa);
            *pa=ptmp;
            }
        divrat( pa, rat_200 );
        mulrat( pa, pi );
        break;
        }
    _cosrat( pa );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: tanrat, _tanrat
//
//  ARGUMENTS:  x PRAT representation of number to take the tangent of
//
//  RETURN: tan     of x in PRAT form.
//
//  EXPLANATION: This uses sinrat and cosrat
//
//-----------------------------------------------------------------------------


void _tanrat( PRAT *px )

{
    PRAT ptmp=NULL;

    DUPRAT(ptmp,*px);
    _sinrat(px);
    _cosrat(&ptmp);
    if ( zerrat( ptmp ) )
        {
    	destroyrat(ptmp);
        throw( CALC_E_DOMAIN );
        }
    divrat(px,ptmp);

    destroyrat(ptmp);

}

void tanrat( PRAT *px )
{
    scale2pi(px); 
    _tanrat(px); 
}

void tananglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    scalerat( pa, angletype );
    switch ( angletype )
        {
    case ANGLE_DEG:
        if ( rat_gt( *pa, rat_180 ) )
            {
            subrat(pa,rat_180);
            }
        divrat( pa, rat_180 );
        mulrat( pa, pi );
        break;
    case ANGLE_GRAD:
        if ( rat_gt( *pa, rat_200 ) )
            {
            subrat(pa,rat_200);
            }
        divrat( pa, rat_200 );
        mulrat( pa, pi );
        break;
        }
    _tanrat( pa );
}

