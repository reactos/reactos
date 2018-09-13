//-----------------------------------------------------------------------------
//  Package Title  ratpak
//  File           transh.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-96 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains hyperbolic sin, cos, and tan for rationals.
//
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

//-----------------------------------------------------------------------------
//
//  FUNCTION: sinhrat, _sinhrat
//
//  ARGUMENTS:  x PRAT representation of number to take the sine hyperbolic
//    of
//  RETURN: sinh of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___    2j+1
//   \  ]  X
//    \   ---------
//    /    (2j+1)!
//   /__]
//   j=0
//          or,
//    n
//   ___                                                 2
//   \  ]                                               X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j)*(2j+1)
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//   if x is bigger than 1.0 (e^x-e^-x)/2 is used.
//
//-----------------------------------------------------------------------------


void _sinhrat( PRAT *px )

{
    CREATETAYLOR();

    DUPRAT(pret,*px); 
    DUPRAT(thisterm,pret);

    DUPNUM(n2,num_one);

    do    {
        NEXTTERM(xx,INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
}

void sinhrat( PRAT *px )

{
    PRAT pret=NULL;
    PRAT tmpx=NULL;

    if ( rat_ge( *px, rat_one ) )
        {
        DUPRAT(tmpx,*px);
        exprat(px);
        tmpx->pp->sign *= -1;
        exprat(&tmpx);
        subrat( px, tmpx );
        divrat( px, rat_two );
        destroyrat( tmpx );
        }
    else
        {
        _sinhrat( px );
        }
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: coshrat
//
//  ARGUMENTS:  x PRAT representation of number to take the cosine
//              hyperbolic of
//
//  RETURN: cosh  of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___    2j
//   \  ]  X
//    \   ---------
//    /    (2j)!
//   /__]
//   j=0
//          or,
//    n
//   ___                                                 2
//   \  ]                                               X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j)*(2j+1)
//   /__]
//   j=0
//
//   thisterm  = 1 ;  and stop when thisterm < precision used.
//           0                              n
//
//   if x is bigger than 1.0 (e^x+e^-x)/2 is used.
//
//-----------------------------------------------------------------------------


void _coshrat( PRAT *px )

{
    CREATETAYLOR();

    pret->pp=longtonum( 1L, nRadix );
    pret->pq=longtonum( 1L, nRadix );

    DUPRAT(thisterm,pret)

    n2=longtonum(0L, nRadix);

    do    {
        NEXTTERM(xx,INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
        } while ( !SMALL_ENOUGH_RAT( thisterm ) );

    DESTROYTAYLOR();
}

void coshrat( PRAT *px )

{
    PRAT tmpx=NULL;

    (*px)->pp->sign = 1;
    (*px)->pq->sign = 1;
    if ( rat_ge( *px, rat_one ) )
        {
        DUPRAT(tmpx,*px);
        exprat(px);
        tmpx->pp->sign *= -1;
        exprat(&tmpx);
        addrat( px, tmpx );
        divrat( px, rat_two );
        destroyrat( tmpx );
        }
    else
        {
        _coshrat( px );
        }
    // Since *px might be epsilon below 1 due to TRIMIT 
    // we need this trick here.
    if ( rat_lt(*px,rat_one) )
        {
        DUPRAT(*px,rat_one);
        }
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: tanhrat
//
//  ARGUMENTS:  x PRAT representation of number to take the tangent
//              hyperbolic of
//
//  RETURN: tanh    of x in PRAT form.
//
//  EXPLANATION: This uses sinhrat and coshrat
//
//-----------------------------------------------------------------------------

void tanhrat( PRAT *px )

{
    PRAT ptmp=NULL;

    DUPRAT(ptmp,*px);
    sinhrat(px);
    coshrat(&ptmp);
    mulnumx(&((*px)->pp),ptmp->pq);
    mulnumx(&((*px)->pq),ptmp->pp);

    destroyrat(ptmp);

}
