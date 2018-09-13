//-----------------------------------------------------------------------------
//  Package Title  ratpak
//  File           fact.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-96 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains fact(orial) and supporting _gamma functions.
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

#define ABSRAT(x) (((x)->pp->sign=1),((x)->pq->sign=1))
#define NEGATE(x) ((x)->pp->sign *= -1)

//-----------------------------------------------------------------------------
//
//  FUNCTION: factrat, _gamma, gamma
//
//  ARGUMENTS:  x PRAT representation of number to take the sine of
//
//  RETURN: factorial of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//      n
//     ___    2j
//   n \  ]  A       1          A
//  A   \   -----[ ---- - ---------------]
//      /   (2j)!  n+2j   (n+2j+1)(2j+1)
//     /__]
//     j=0
//
//                        / oo
//                        |    n-1 -x     __
//  This was derived from |   x   e  dx = |
//                        |               | (n) { = (n-1)! for +integers}
//                        / 0
//
//  GregSte showed the above series to be within precision if A was chosen
//  big enough.
//                          A    n  precision
//  Based on the relation ne  = A 10            A was chosen as
//
//             precision
//  A = ln(Base         /n)+1
//  A += n*ln(A)  This is close enough for precision > base and n < 1.5
//
//
//-----------------------------------------------------------------------------


void _gamma( PRAT *pn )

{
    PRAT factorial=NULL;
    PNUMBER count=NULL;
    PRAT tmp=NULL;
    PRAT one_pt_five=NULL;
    PRAT a=NULL;
    PRAT a2=NULL;
    PRAT term=NULL;
    PRAT sum=NULL;
    PRAT err=NULL;
    PRAT mpy=NULL;
    PRAT ratprec = NULL;
    PRAT ratRadix = NULL;
    long oldprec;
    
    // Set up constants and initial conditions
    oldprec = maxout;
    ratprec = longtorat( oldprec );
    
    // Find the best 'A' for convergence to the required precision.
    a=longtorat( nRadix );
    lograt(&a);
    mulrat(&a,ratprec);

    // Really is -ln(n)+1, but -ln(n) will be < 1 
    // if we scale n between 0.5 and 1.5
    addrat(&a,rat_two);
    DUPRAT(tmp,a);
    lograt(&tmp);
    mulrat(&tmp,*pn);
    addrat(&a,tmp);
    addrat(&a,rat_one);
    
    // Calculate the necessary bump in precision and up the precision.
    // The following code is equivalent to 
    // maxout += ln(exp(a)*pow(a,n+1.5))-ln(nRadix));
    DUPRAT(tmp,*pn);
    one_pt_five=longtorat( 3L );
    divrat( &one_pt_five, rat_two );
    addrat( &tmp, one_pt_five );
    DUPRAT(term,a);
    powrat( &term, tmp );
    DUPRAT( tmp, a );
    exprat( &tmp );
    mulrat( &term, tmp );
    lograt( &term );
    ratRadix = longtorat( nRadix );
    DUPRAT(tmp,ratRadix);
    lograt( &tmp );
    subrat( &term, tmp );
    maxout += rattolong( term );
    
    // Set up initial terms for series, refer to series in above comment block.
    DUPRAT(factorial,rat_one); // Start factorial out with one
    count = longtonum( 0L, BASEX );

    DUPRAT(mpy,a);
    powrat(&mpy,*pn);
    // a2=a^2
    DUPRAT(a2,a);
    mulrat(&a2,a);
    
    // sum=(1/n)-(a/(n+1))
    DUPRAT(sum,rat_one);
    divrat(&sum,*pn);
    DUPRAT(tmp,*pn);
    addrat(&tmp,rat_one);
    DUPRAT(term,a);
    divrat(&term,tmp);
    subrat(&sum,term);

    DUPRAT(err,ratRadix);
    NEGATE(ratprec);
    powrat(&err,ratprec);
    divrat(&err,ratRadix);

    // Just get something not tiny in term
    DUPRAT(term, rat_two );    

    // Loop until precision is reached, or asked to halt.
    while ( !zerrat( term ) && rat_gt( term, err) && !fhalt )
        {
        addrat(pn,rat_two);
        
        // WARNING: mixing numbers and  rationals here.  
        // for speed and efficiency.
        INC(count);
        mulnumx(&(factorial->pp),count);
        INC(count)
        mulnumx(&(factorial->pp),count);

        divrat(&factorial,a2);

        DUPRAT(tmp,*pn);
        addrat( &tmp, rat_one );
        destroyrat(term);
        createrat(term);
        DUPNUM(term->pp,count);
        DUPNUM(term->pq,num_one);
        addrat( &term, rat_one );
        mulrat( &term, tmp );
        DUPRAT(tmp,a);
        divrat( &tmp, term );

        DUPRAT(term,rat_one);
        divrat( &term, *pn);
        subrat( &term, tmp);
        
        divrat (&term, factorial);
        addrat( &sum, term);
        ABSRAT(term);
        }
    
    // Multiply by factor.
    mulrat( &sum, mpy );
    
    // And cleanup
    maxout = oldprec;
    destroyrat(ratprec);
    destroyrat(err);
    destroyrat(term);
    destroyrat(a);
    destroyrat(a2);
    destroyrat(tmp);
    destroyrat(one_pt_five);

    destroynum(count);

    destroyrat(factorial);
    destroyrat(*pn);
    DUPRAT(*pn,sum);
    destroyrat(sum);
}

void factrat( PRAT *px )

{
    PRAT fact = NULL;
    PRAT frac = NULL;
    PRAT neg_rat_one = NULL;
    DUPRAT(fact,rat_one);

    DUPRAT(neg_rat_one,rat_one);
    neg_rat_one->pp->sign *= -1;

    DUPRAT( frac, *px );
    fracrat( &frac );

    // Check for negative integers and throw an error.
    if ( ( zerrat(frac) || ( LOGRATRADIX(frac) <= -maxout ) ) && 
		( (*px)->pp->sign * (*px)->pq->sign == -1 ) )
		{
        throw CALC_E_DOMAIN;
		}
    while ( rat_gt(  *px, rat_zero ) && !fhalt && 
        ( LOGRATRADIX(*px) > -maxout ) )
        {
        mulrat( &fact, *px );
        subrat( px, rat_one );
        }
    
    // Added to make numbers 'close enough' to integers use integer factorial.
    if ( LOGRATRADIX(*px) <= -maxout )
        {
        DUPRAT((*px),rat_zero);
        intrat(&fact);
        }

    while ( rat_lt(  *px, neg_rat_one ) && !fhalt )
        {
        addrat( px, rat_one );
        divrat( &fact, *px );
        }

    if ( rat_neq( *px, rat_zero ) )
        {
        addrat( px, rat_one );
        _gamma( px );
        mulrat( px, fact );
        }
    else
        {
        DUPRAT(*px,fact);
        }
}

