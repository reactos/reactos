//-----------------------------------------------------------------------------
//  Package Title  ratpak
//  File           itrans.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-96 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains inverse sin, cos, tan functions for rationals
//
//  Special Information
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( DOS )
#include <dosstub.h>
#else
#include <windows.h>
#endif
#include <ratpak.h>

void ascalerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    switch ( angletype )
        {
    case ANGLE_RAD:
        break;
    case ANGLE_DEG:
        divrat( pa, two_pi );
        mulrat( pa, rat_360 );
        break;
    case ANGLE_GRAD:
        divrat( pa, two_pi );
        mulrat( pa, rat_400 );
        break;
        }
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: asinrat, _asinrat
//
//  ARGUMENTS: x PRAT representation of number to take the inverse
//    sine of
//  RETURN: asin  of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___                                                   2 2
//   \  ]                                            (2j+1) X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j+2)*(2j+3)
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//   If abs(x) > 0.85 then an alternate form is used
//      pi/2-sgn(x)*asin(sqrt(1-x^2)
//
//
//-----------------------------------------------------------------------------

void _asinrat( PRAT *px )

{
    CREATETAYLOR();
    DUPRAT(pret,*px); 
    DUPRAT(thisterm,*px);
    DUPNUM(n2,num_one);

    do
        {
        NEXTTERM(xx,MULNUM(n2) MULNUM(n2) 
            INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        }
    while ( !SMALL_ENOUGH_RAT( thisterm ) );
    DESTROYTAYLOR();
}

void asinanglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    asinrat( pa );
    ascalerat( pa, angletype );
}

void asinrat( PRAT *px )

{
    long sgn;
    PRAT pret=NULL;
    PRAT phack=NULL;

    sgn = (*px)->pp->sign* (*px)->pq->sign;

    (*px)->pp->sign = 1;
    (*px)->pq->sign = 1;
    
    // Nasty hack to avoid the really bad part of the asin curve near +/-1.
    DUPRAT(phack,*px);
    subrat(&phack,rat_one);
    // Since *px might be epsilon near zero we must set it to zero.
    if ( rat_le(phack,rat_smallest) && rat_ge(phack,rat_negsmallest) )
        {
        destroyrat(phack);
        DUPRAT( *px, pi_over_two );
        }
    else
        {
        destroyrat(phack);
        if ( rat_gt( *px, pt_eight_five ) )
            {
            if ( rat_gt( *px, rat_one ) )
                {
                subrat( px, rat_one );
                if ( rat_gt( *px, rat_smallest ) )
                    {
                	throw( CALC_E_DOMAIN );
                    }
                else
                    {
                	DUPRAT(*px,rat_one);
                    }
                }
            DUPRAT(pret,*px);
            mulrat( px, pret );
            (*px)->pp->sign *= -1;
            addrat( px, rat_one );
            rootrat( px, rat_two );
            _asinrat( px );
            (*px)->pp->sign *= -1;
            addrat( px, pi_over_two );
            destroyrat(pret);
            }
        else
            {
            _asinrat( px );
            }
        }
    (*px)->pp->sign = sgn;
    (*px)->pq->sign = 1;
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: acosrat, _acosrat
//
//  ARGUMENTS: x PRAT representation of number to take the inverse
//    cosine of
//  RETURN: acos  of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___                                                   2 2
//   \  ]                                            (2j+1) X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j+2)*(2j+3)
//   /__]
//   j=0
//
//   thisterm  = 1 ;  and stop when thisterm < precision used.
//           0                              n
//
//   In this case pi/2-asin(x) is used.  At least for now _acosrat isn't
//      called.
//
//-----------------------------------------------------------------------------

void acosanglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    acosrat( pa );
    ascalerat( pa, angletype );
}

void _acosrat( PRAT *px )

{
    CREATETAYLOR();

    createrat(thisterm); 
    thisterm->pp=longtonum( 1L, BASEX );
    thisterm->pq=longtonum( 1L, BASEX ); 

    DUPNUM(n2,num_one);

    do
        {
        NEXTTERM(xx,MULNUM(n2) MULNUM(n2) 
            INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        }
    while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
}

void acosrat( PRAT *px )

{
    long sgn;

    sgn = (*px)->pp->sign*(*px)->pq->sign;

    (*px)->pp->sign = 1;
    (*px)->pq->sign = 1;
    
    if ( rat_equ( *px, rat_one ) )
        {
        if ( sgn == -1 )
            {
            DUPRAT(*px,pi);
            }
        else
            {
            DUPRAT( *px, rat_zero );
            }
        }
    else
        {
        (*px)->pp->sign = sgn;
        asinrat( px );
        (*px)->pp->sign *= -1;
        addrat(px,pi_over_two);
        }
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: atanrat, _atanrat
//
//  ARGUMENTS: x PRAT representation of number to take the inverse
//              hyperbolic tangent of
//
//  RETURN: atanh of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___                                                   2
//   \  ]                                            (2j)*X (-1^j)
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j+2)
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//   If abs(x) > 0.85 then an alternate form is used
//      asin(x/sqrt(q+x^2))
//
//   And if abs(x) > 2.0 then this form is used.
//
//   pi/2 - atan(1/x)
//
//-----------------------------------------------------------------------------

void atananglerat( IN OUT PRAT *pa, IN ANGLE_TYPE angletype )

{
    atanrat( pa );
    ascalerat( pa, angletype );
}

void _atanrat( PRAT *px )

{
    CREATETAYLOR();

    DUPRAT(pret,*px); 
    DUPRAT(thisterm,*px);

    DUPNUM(n2,num_one);

    xx->pp->sign *= -1;

    do    {
        NEXTTERM(xx,MULNUM(n2) INC(n2) INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
}

void atan2rat( PRAT *py, PRAT x )

{
    if ( rat_gt( x, rat_zero ) )
        {
        if ( !zerrat( (*py) ) )
            {
            divrat( py, x);
            atanrat( py );
            }
        }
    else if ( rat_lt( x, rat_zero ) )
        {
        if ( rat_gt( (*py), rat_zero ) )
            {
            divrat( py, x);
            atanrat( py );
            addrat( py, pi );
            }
        else if ( rat_lt( (*py), rat_zero ) )
            {
            divrat( py, x);
            atanrat( py );
            subrat( py, pi );
            }
        else // (*py) == 0
            {
            DUPRAT( *py, pi );
            }
        }
    else // x == 0
        {
        if ( !zerrat( (*py) ) )
            {
            int sign;
            sign=(*py)->pp->sign*(*py)->pq->sign;
            DUPRAT( *py, pi_over_two );
            (*py)->pp->sign = sign;
            }
        else // (*py) == 0
            {
            DUPRAT( *py, rat_zero );
            }
        }
}

void atanrat( PRAT *px )

{
    long sgn;
    PRAT tmpx=NULL;

    sgn = (*px)->pp->sign * (*px)->pq->sign;

    (*px)->pp->sign = 1;
    (*px)->pq->sign = 1;
    
    if ( rat_gt( (*px), pt_eight_five ) )
        {
        if ( rat_gt( (*px), rat_two ) )
            {
            (*px)->pp->sign = sgn;
            (*px)->pq->sign = 1;
            DUPRAT(tmpx,rat_one);
            divrat(&tmpx,(*px));
            _atanrat(&tmpx);
            tmpx->pp->sign = sgn;
            tmpx->pq->sign = 1;
            DUPRAT(*px,pi_over_two);
            subrat(px,tmpx);
            destroyrat( tmpx );
            }
        else 
            {
            (*px)->pp->sign = sgn;
            DUPRAT(tmpx,*px);
            mulrat( &tmpx, *px );
            addrat( &tmpx, rat_one );
            rootrat( &tmpx, rat_two );
            divrat( px, tmpx );
            destroyrat( tmpx );
            asinrat( px );
            (*px)->pp->sign = sgn;
            (*px)->pq->sign = 1;
            }
        }
    else
        {
        (*px)->pp->sign = sgn;
        (*px)->pq->sign = 1;
        _atanrat( px );
        }
    if ( rat_gt( *px, pi_over_two ) )
        {
        subrat( px, pi );
        }
}

