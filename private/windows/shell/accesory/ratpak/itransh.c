//-----------------------------------------------------------------------------
//  Package Title  ratpak
//  File           itransh.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-97 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//    Contains inverse hyperbolic sin, cos, and tan functions.
//
//  Special Information
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
//  FUNCTION: asinhrat
//
//  ARGUMENTS:  x PRAT representation of number to take the inverse
//    hyperbolic sine of
//  RETURN: asinh of x in PRAT form.
//
//  EXPLANATION: This uses Taylor series
//
//    n
//   ___                                                   2 2
//   \  ]                                           -(2j+1) X
//    \   thisterm  ; where thisterm   = thisterm  * ---------
//    /           j                 j+1          j   (2j+2)*(2j+3)
//   /__]
//   j=0
//
//   thisterm  = X ;  and stop when thisterm < precision used.
//           0                              n
//
//   For abs(x) < .85, and
//
//   asinh(x) = log(x+sqrt(x^2+1))
//
//   For abs(x) >= .85
//
//-----------------------------------------------------------------------------

void asinhrat( PRAT *px )

{
    PRAT neg_pt_eight_five = NULL;

    DUPRAT(neg_pt_eight_five,pt_eight_five);
    neg_pt_eight_five->pp->sign *= -1;
    if ( rat_gt( *px, pt_eight_five) || rat_lt( *px, neg_pt_eight_five) )
        {
        PRAT ptmp = NULL;
        DUPRAT(ptmp,(*px)); 
        mulrat(&ptmp,*px);
        addrat(&ptmp,rat_one);
        rootrat(&ptmp,rat_two);
        addrat(px,ptmp);
        lograt(px);
        destroyrat(ptmp);
        }
    else
        {
        CREATETAYLOR();
        xx->pp->sign *= -1;

        DUPRAT(pret,(*px)); 
        DUPRAT(thisterm,(*px));

        DUPNUM(n2,num_one);

        do
            {
            NEXTTERM(xx,MULNUM(n2) MULNUM(n2) 
                INC(n2) DIVNUM(n2) INC(n2) DIVNUM(n2));
            }
        while ( !SMALL_ENOUGH_RAT( thisterm ) );

        DESTROYTAYLOR();
        }
    destroyrat(neg_pt_eight_five);
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: acoshrat
//
//  ARGUMENTS:  x PRAT representation of number to take the inverse
//    hyperbolic cose of
//  RETURN: acosh of x in PRAT form.
//
//  EXPLANATION: This uses 
//
//   acosh(x)=ln(x+sqrt(x^2-1))
//
//   For x >= 1
//
//-----------------------------------------------------------------------------

void acoshrat( PRAT *px )

{
    if ( rat_lt( *px, rat_one ) )
        {
        throw CALC_E_DOMAIN;
        }
    else
        {
        PRAT ptmp = NULL;
        DUPRAT(ptmp,(*px)); 
        mulrat(&ptmp,*px);
        subrat(&ptmp,rat_one);
        rootrat(&ptmp,rat_two);
        addrat(px,ptmp);
        lograt(px);
        destroyrat(ptmp);
        }
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: atanhrat
//
//  ARGUMENTS:  x PRAT representation of number to take the inverse
//              hyperbolic tangent of
//
//  RETURN: atanh of x in PRAT form.
//
//  EXPLANATION: This uses
//
//             1     x+1
//  atanh(x) = -*ln(----)
//             2     x-1
//
//-----------------------------------------------------------------------------

void atanhrat( PRAT *px )

{
    PRAT ptmp = NULL;
    DUPRAT(ptmp,(*px)); 
    subrat(&ptmp,rat_one);
    addrat(px,rat_one);
    divrat(px,ptmp);
    (*px)->pp->sign *= -1;
    lograt(px);
    divrat(px,rat_two);
    destroyrat(ptmp);
}

