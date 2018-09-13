//---------------------------------------------------------------------------
//  Package Title  ratpak
//  File           num.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-99 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Contains routines for and, or, xor, not and other support
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <ratpak.h>

void lshrat( PRAT *pa, PRAT b )

{
    PRAT pwr=NULL;
    long intb;

    intrat(pa);
    if ( !zernum( (*pa)->pp ) )
        {
        // If input is zero we're done.
        if ( rat_gt( b, rat_max_exp ) )
            {
            // Don't attempt lsh of anything big
            throw( CALC_E_DOMAIN );
            }
        intb = rattolong(b);
        DUPRAT(pwr,rat_two);
        ratpowlong(&pwr,intb);
        mulrat(pa,pwr);
        destroyrat(pwr);
        }
}

void rshrat( PRAT *pa, PRAT b )

{
    PRAT pwr=NULL;
    long intb;

    intrat(pa);
    if ( !zernum( (*pa)->pp ) )
        { 
        // If input is zero we're done.
        if ( rat_lt( b, rat_min_exp ) )
            {
            // Don't attempt rsh of anything big and negative.
            throw( CALC_E_DOMAIN );
            }
        intb = rattolong(b);
        DUPRAT(pwr,rat_two);
        ratpowlong(&pwr,intb);
        divrat(pa,pwr);
        destroyrat(pwr);
       }
}

void boolrat( PRAT *pa, PRAT b, int func );
void boolnum( PNUMBER *pa, PNUMBER b, int func );


enum {
    FUNC_AND,
    FUNC_OR,
    FUNC_XOR
} BOOL_FUNCS;

void andrat( PRAT *pa, PRAT b )

{
    boolrat( pa, b, FUNC_AND );
}

void orrat( PRAT *pa, PRAT b )

{
    boolrat( pa, b, FUNC_OR );
}

void xorrat( PRAT *pa, PRAT b )

{
    boolrat( pa, b, FUNC_XOR );
}

//---------------------------------------------------------------------------
//
//    FUNCTION: boolrat
//
//    ARGUMENTS: pointer to a rational a second rational.
//
//    RETURN: None, changes pointer.
//
//    DESCRIPTION: Does the rational equivalent of *pa op= b;
//
//---------------------------------------------------------------------------

void boolrat( PRAT *pa, PRAT b, int func )

{
    PRAT tmp=NULL;
    intrat( pa );
    DUPRAT(tmp,b);
    intrat( &tmp );

    boolnum( &((*pa)->pp), tmp->pp, func );
    destroyrat(tmp);
}

//---------------------------------------------------------------------------
//
//    FUNCTION: boolnum
//
//    ARGUMENTS: pointer to a number a second number
//
//    RETURN: None, changes first pointer.
//
//    DESCRIPTION: Does the number equivalent of *pa &= b.
//    nRadix doesn't matter for logicals.
//    WARNING: Assumes numbers are unsigned.
//
//---------------------------------------------------------------------------

void boolnum( PNUMBER *pa, PNUMBER b, int func )

{
    PNUMBER c=NULL;
    PNUMBER a=NULL;
    MANTTYPE *pcha;
    MANTTYPE *pchb;
    MANTTYPE *pchc;
    long cdigits;
    long mexp;
    MANTTYPE da;
    MANTTYPE db;

    a=*pa;
    cdigits = max( a->cdigit+a->exp, b->cdigit+b->exp ) -
            min( a->exp, b->exp );
    createnum( c, cdigits );
    c->exp = min( a->exp, b->exp );
    mexp = c->exp;
    c->cdigit = cdigits;
    pcha = MANT(a);
    pchb = MANT(b);
    pchc = MANT(c);
    for ( ;cdigits > 0; cdigits--, mexp++ )
        {
        da = ( ( ( mexp >= a->exp ) && ( cdigits + a->exp - c->exp > 
                    (c->cdigit - a->cdigit) ) ) ? 
                    *pcha++ : 0 );
        db = ( ( ( mexp >= b->exp ) && ( cdigits + b->exp - c->exp > 
                    (c->cdigit - b->cdigit) ) ) ? 
                    *pchb++ : 0 );
        switch ( func )
            {
        case FUNC_AND:
            *pchc++ = da & db;
            break;
        case FUNC_OR:
            *pchc++ = da | db;
            break;
        case FUNC_XOR:
            *pchc++ = da ^ db;
            break;
            }
        }
    c->sign = a->sign;
    while ( c->cdigit > 1 && *(--pchc) == 0 )
        {
        c->cdigit--;
        }
    destroynum( *pa );
    *pa=c;
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: modrat
//
//    ARGUMENTS: pointer to a rational a second rational.
//
//    RETURN: None, changes pointer.
//
//    DESCRIPTION: Does the rational equivalent of frac(*pa);
//
//-----------------------------------------------------------------------------

void modrat( PRAT *pa, PRAT b )

{
    PRAT tmp = NULL;

    if ( zerrat( b ) )
		{
		throw CALC_E_INDEFINITE;
		}
    DUPRAT(tmp,b);

    mulnumx( &((*pa)->pp), tmp->pq );
    mulnumx( &(tmp->pp), (*pa)->pq );
    remnum( &((*pa)->pp), tmp->pp, BASEX );
    mulnumx( &((*pa)->pq), tmp->pq );
    
    //Get *pa back in the integer over integer form.
    RENORMALIZE(*pa);

    destroyrat( tmp );
}

