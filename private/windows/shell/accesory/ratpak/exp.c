//-----------------------------------------------------------------------------
//  Package Title  ratpak
//  File           exp.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-96 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains exp, and log functions for rationals
//
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#if defined( DOS )
#include <dosstub.h>
#else
#include <windows.h>
#endif
#include <ratpak.h>

//-----------------------------------------------------------------------------
//
//  FUNCTION: exprat
//
//  ARGUMENTS: x PRAT representation of number to exponentiate
//
//  RETURN: exp  of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___
//   \  ]                                               X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j      j+1
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//-----------------------------------------------------------------------------

void _exprat( PRAT *px )

{
    CREATETAYLOR();

    addnum(&(pret->pp),num_one, BASEX); 
    addnum(&(pret->pq),num_one, BASEX); 
    DUPRAT(thisterm,pret);

    n2=longtonum(0L, BASEX);

    do    {
        NEXTTERM(*px, INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) && !fhalt );

    DESTROYTAYLOR();
}

void exprat( PRAT *px )

{
    PRAT pwr=NULL;
    PRAT pint=NULL;
    long intpwr;

    if ( rat_gt( *px, rat_max_exp ) || rat_lt( *px, rat_min_exp ) )
        {
        // Don't attempt exp of anything large.
        throw( CALC_E_DOMAIN );
        }

    DUPRAT(pwr,rat_exp);
    DUPRAT(pint,*px);

    intrat(&pint);

    intpwr = rattolong(pint);
    ratpowlong( &pwr, intpwr );

    subrat(px,pint);
    
    // It just so happens to be an integral power of e.
    if ( rat_gt( *px, rat_negsmallest ) && rat_lt( *px, rat_smallest ) )
        {
        DUPRAT(*px,pwr);
        }
    else
        {
        _exprat(px);
        mulrat(px,pwr);
        }

    destroyrat( pwr );
    destroyrat( pint );
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: lograt, _lograt
//
//  ARGUMENTS: x PRAT representation of number to logarithim
//
//  RETURN: log  of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___
//   \  ]                                             j*(1-X)
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j      j+1
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//   Number is scaled between one and e_to_one_half prior to taking the
//   log. This is to keep execution time from exploding.
//
//
//-----------------------------------------------------------------------------

void _lograt( PRAT *px )

{
    CREATETAYLOR();

    createrat(thisterm);
    
    // sub one from x
    (*px)->pq->sign *= -1;
    addnum(&((*px)->pp),(*px)->pq, BASEX);
    (*px)->pq->sign *= -1;

    DUPRAT(pret,*px);
    DUPRAT(thisterm,*px);

    n2=longtonum(1L, BASEX);
    (*px)->pp->sign *= -1;

    do    {
        NEXTTERM(*px, MULNUM(n2) INC(n2) DIVNUM(n2));
        TRIMTOP(*px);
        } while ( !SMALL_ENOUGH_RAT( thisterm ) && !fhalt );

    DESTROYTAYLOR();
}


void lograt( PRAT *px )

{
    BOOL fneglog;
    PRAT pwr=NULL;            // pwr is the large scaling factor.
    PRAT offset=NULL;        // offset is the incremental scaling factor.
    
    
    // Check for someone taking the log of zero or a negative number.
    if ( rat_le( *px, rat_zero ) )
        {
        throw( CALC_E_DOMAIN );
        }
    
    // Get number > 1, for scaling
    fneglog = rat_lt( *px, rat_one );
    if ( fneglog )
        {
        // WARNING: This is equivalent to doing *px = 1 / *px
        PNUMBER pnumtemp=NULL;
        pnumtemp = (*px)->pp;
        (*px)->pp = (*px)->pq;
        (*px)->pq = pnumtemp;
        }
    
    // Scale the number within BASEX factor of 1, for the large scale.
    // log(x*2^(BASEXPWR*k)) = BASEXPWR*k*log(2)+log(x)
    if ( LOGRAT2(*px) > 1 )
        {
        // Take advantage of px's base BASEX to scale quickly down to 
        // a reasonable range.
        long intpwr;
        intpwr=LOGRAT2(*px)-1;
        (*px)->pq->exp += intpwr;
        pwr=longtorat(intpwr*BASEXPWR);
        mulrat(&pwr,ln_two);
        // ln(x+e)-ln(x) looks close to e when x is close to one using some
        // expansions.  This means we can trim past precision digits+1.
        TRIMTOP(*px);
        }
    else
        {
        DUPRAT(pwr,rat_zero);
        }

    DUPRAT(offset,rat_zero);
    // Scale the number between 1 and e_to_one_half, for the small scale.
    while ( rat_gt( *px, e_to_one_half ) && !fhalt )
        {
        divrat( px, e_to_one_half );
        addrat( &offset, rat_one );
        }

    _lograt(px);
    
    // Add the large and small scaling factors, take into account
    // small scaling was done in e_to_one_half chunks.
    divrat(&offset,rat_two);
    addrat(&pwr,offset);
    
    // And add the resulting scaling factor to the answer.
    addrat(px,pwr);

    trimit(px);
    
    // If number started out < 1 rescale answer to negative.
    if ( fneglog )
        {
        (*px)->pp->sign *= -1;
        }

    destroyrat(pwr);
}
    
void log10rat( PRAT *px )

{
    lograt(px);
    divrat(px,ln_ten);
}


//---------------------------------------------------------------------------
//
//  FUNCTION: powrat
//
//  ARGUMENTS: PRAT *px, and PRAT y
//
//  RETURN: none, sets *px to *px to the y.
//
//  EXPLANATION: This uses x^y=e(y*ln(x)), or a more exact calculation where
//  y is an integer.
//  Assumes, all checking has been done on validity of numbers.
//
//
//---------------------------------------------------------------------------

void powrat( PRAT *px, PRAT y )

{
    PRAT podd=NULL;
    PRAT plnx=NULL;
    long sign=1;
    sign=( (*px)->pp->sign * (*px)->pq->sign );
    
    // Take the absolute value
    (*px)->pp->sign = 1;
    (*px)->pq->sign = 1;

    if ( zerrat( *px ) )
        {
        // *px is zero.
        if ( rat_lt( y, rat_zero ) )
            {
            throw( CALC_E_DOMAIN );
            }
        else if ( zerrat( y ) )
            {
            // *px and y are both zero, special case a 1 return.
            DUPRAT(*px,rat_one);
            // Ensure sign is positive.
            sign = 1;
            }
        }
    else 
        {
        PRAT pxint=NULL;
        DUPRAT(pxint,*px);
        subrat(&pxint,rat_one);
        if ( rat_gt( pxint, rat_negsmallest ) && 
             rat_lt( pxint, rat_smallest ) && ( sign == 1 ) )
            {
            // *px is one, special case a 1 return.
            DUPRAT(*px,rat_one);
            // Ensure sign is positive.
            sign = 1;
            }
        else
            {

            // Only do the exp if the number isn't zero or one
            DUPRAT(podd,y);
            fracrat(&podd);
            if ( rat_gt( podd, rat_negsmallest ) && rat_lt( podd, rat_smallest ) )
                {
                // If power is an integer let ratpowlong deal with it.
                PRAT iy = NULL;
                long inty;
                DUPRAT(iy,y);
                subrat(&iy,podd);
                inty = rattolong(iy);

                DUPRAT(plnx,*px);
                lograt(&plnx);
                mulrat(&plnx,iy);
                if ( rat_gt( plnx, rat_max_exp ) || rat_lt( plnx, rat_min_exp ) )
                    {
                    // Don't attempt exp of anything large or small.A
                    destroyrat(plnx);
                    destroyrat(iy);
                    throw( CALC_E_DOMAIN );
                    }
                destroyrat(plnx);
                ratpowlong(px,inty);
                if ( ( inty & 1 ) == 0 )
                    {
                    sign=1;
                    }
                destroyrat(iy);
                }
            else
                {
                // power is a fraction
                if ( sign == -1 )
                    {
                    // And assign the sign after computations, if appropriate.
                    if ( rat_gt( y, rat_neg_one ) && rat_lt( y, rat_zero ) )
                        {
                        // Check to see if reciprocal is odd.
                        DUPRAT(podd,rat_one);
                        divrat(&podd,y);
                        // Only interested in the absval for determining oddness.
                        podd->pp->sign = 1;
                        podd->pq->sign = 1;
                        divrat(&podd,rat_two);
                        fracrat(&podd);
                        addrat(&podd,podd);
                        subrat(&podd,rat_one);
                        if ( rat_lt( podd, rat_zero ) )
                            {
                            // Negative nonodd root of negative number.
                            destroyrat(podd);
                            throw( CALC_E_DOMAIN );
                            }
                        }
                    else
                        {
                        // Negative nonodd power of negative number.
                        destroyrat(podd);
                        throw( CALC_E_DOMAIN );
                        }

                     }
                 else
                     {
                     // If the exponent is not odd disregard the sign.
                     sign = 1;
                     }
    
                 lograt( px );
                 mulrat( px, y );
                 exprat( px );
                 }
             destroyrat(podd);
             }
        destroyrat(pxint);
        }
    (*px)->pp->sign *= sign;
}
