//---------------------------------------------------------------------------
//  Package Title  ratpak
//  File           conv.c
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-97 Microsoft
//  Date           01-16-95
//
// 
//  Description
//
//     Contains conversion, input and output routines for numbers rationals
//  and longs.
//
//
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include <tchar.h>      // TCHAR version of sprintf
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#if defined( DOS )
#include <dosstub.h>
#else
#include <windows.h>
#endif
#include <ratpak.h>

BOOL fparserror=FALSE;
BOOL gbinexact=FALSE;

// digits 0..64 used by bases 2 .. 64
TCHAR digits[65]=TEXT("0123456789")
TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
TEXT("abcdefghijklmnopqrstuvwxyz_@");

// ratio of internal 'digits' to output 'digits'
// Calculated elsewhere as part of initialization and when base is changed
long ratio;    // int(log(2L^BASEXPWR)/log(nRadix))

// Used to strip trailing zeroes, and prevent combinatorial explosions 
BOOL stripzeroesnum( PNUMBER pnum, long starting );

// returns int(lognRadix(x)) quickly.
long longlognRadix( long x );


//----------------------------------------------------------------------------
//
//    FUNCTION: fail
//
//    ARGUMENTS: pointer to an error message.
//
//    RETURN: None
//
//    DESCRIPTION: fail dumps the error message then throws an exception
//
//----------------------------------------------------------------------------

void fail( IN long errmsg )

{
#ifdef DEBUG
    fprintf( stderr, "%s\n", TEXT("Out of Memory") );
#endif
    throw( CALC_E_OUTOFMEMORY );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: _destroynum
//
//    ARGUMENTS: pointer to a number
//
//    RETURN: None
//
//    DESCRIPTION: Deletes the number and associated allocation
//
//-----------------------------------------------------------------------------

void _destroynum( IN PNUMBER pnum )

{
    if ( pnum != NULL )
        {
        zfree( pnum );
        }
}


//-----------------------------------------------------------------------------
//
//    FUNCTION: _destroyrat
//
//    ARGUMENTS: pointer to a rational
//
//    RETURN: None
//
//    DESCRIPTION: Deletes the rational and associated
//    allocations.
//
//-----------------------------------------------------------------------------

void _destroyrat( IN PRAT prat )

{
    if ( prat != NULL )
        {
        destroynum( prat->pp );
        destroynum( prat->pq );
        zfree( prat );
        }
}


//-----------------------------------------------------------------------------
//
//    FUNCTION: _createnum
//
//    ARGUMENTS: size of number in 'digits'
//
//    RETURN: pointer to a number
//
//    DESCRIPTION: allocates and zeroes out number type.
//
//-----------------------------------------------------------------------------

PNUMBER _createnum( IN long size )

{
    PNUMBER pnumret=NULL;
    
    // sizeof( MANTTYPE ) is the size of a 'digit'
    pnumret = (PNUMBER)zmalloc( (int)(size+1) * sizeof( MANTTYPE ) + 
        sizeof( NUMBER ) );
    if ( pnumret == NULL )
        {
        fail( CALC_E_OUTOFMEMORY );
        }
    return( pnumret );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: _createrat
//
//    ARGUMENTS: none
//
//    RETURN: pointer to a rational
//
//    DESCRIPTION: allocates a rational structure but does not
//    allocate the numbers that make up the rational p over q
//    form.  These number pointers are left pointing to null.
//
//-----------------------------------------------------------------------------


PRAT _createrat( void )

{
    PRAT prat=NULL;

    prat = (PRAT)zmalloc( sizeof( RAT ) );

    if ( prat == NULL )
        {
        fail( CALC_E_OUTOFMEMORY );
        }
    prat->pp = NULL;
    prat->pq = NULL;
    return( prat );
}



//-----------------------------------------------------------------------------
//
//    FUNCTION: numtorat
//
//    ARGUMENTS: pointer to a number, nRadix number is in.
//
//    RETURN: Rational representation of number.
//
//    DESCRIPTION: The rational representation of the number
//    is guaranteed to be in the form p (number with internal
//    base   representation) over q (number with internal base
//    representation)  Where p and q are integers.
//
//-----------------------------------------------------------------------------

PRAT numtorat( IN PNUMBER pin, IN unsigned long nRadix )

{
    PRAT pout=NULL;
    PNUMBER pnRadixn=NULL;
    PNUMBER qnRadixn=NULL;

    DUPNUM( pnRadixn, pin );

    qnRadixn=longtonum( 1, nRadix );
    
    // Ensure p and q start out as integers.
    if ( pnRadixn->exp < 0 )
        {
        qnRadixn->exp -= pnRadixn->exp;
        pnRadixn->exp = 0;
        }

    createrat(pout);

    // There is probably a better way to do this.
    pout->pp = numtonRadixx( pnRadixn, nRadix, ratio );
    pout->pq = numtonRadixx( qnRadixn, nRadix, ratio );


    destroynum( pnRadixn );
    destroynum( qnRadixn );

    return( pout );
}



//----------------------------------------------------------------------------
//
//    FUNCTION: nRadixxtonum
//
//    ARGUMENTS: pointer to a number, base requested.
//
//    RETURN: number representation in nRadix requested.
//
//    DESCRIPTION: Does a base conversion on a number from
//    internal to requested base. Assumes number being passed
//    in is really in internal base form.
//
//----------------------------------------------------------------------------

PNUMBER nRadixxtonum( IN PNUMBER a, IN unsigned long nRadix )

{
    PNUMBER sum=NULL;
    PNUMBER powofnRadix=NULL;
    unsigned long bitmask;
    unsigned long cdigits;
    MANTTYPE *ptr;

    sum = longtonum( 0, nRadix );
    powofnRadix = longtonum( BASEX, nRadix );

    // A large penalty is paid for conversion of digits no one will see anyway.
    // limit the digits to the minimum of the existing precision or the 
    // requested precision.
    cdigits = maxout + 1;
    if ( cdigits > (unsigned long)a->cdigit )
        {
        cdigits = (unsigned long)a->cdigit; 
        }

    // scale by the internal base to the internal exponent offset of the LSD
    numpowlong( &powofnRadix, a->exp + (a->cdigit - cdigits), nRadix );
    
    // Loop over all the relative digits from MSD to LSD
    for ( ptr = &(MANT(a)[a->cdigit-1]); cdigits > 0 && !fhalt; 
        ptr--, cdigits-- )
        {
        // Loop over all the bits from MSB to LSB
        for ( bitmask = BASEX/2; bitmask > 0; bitmask /= 2 )
            {
            addnum( &sum, sum, nRadix );
            if ( *ptr & bitmask )
                {
                sum->mant[0] |= 1;
                }
            }
        }

    // Scale answer by power of internal exponent.
    mulnum( &sum, powofnRadix, nRadix );

    destroynum( powofnRadix );
    sum->sign = a->sign;
    return( sum );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: numtonRadixx
//
//    ARGUMENTS: pointer to a number, nRadix of that number.
//       previously calculated ratio
//
//    RETURN: number representation in internal nRadix.
//
//    DESCRIPTION: Does a nRadix conversion on a number from
//    specified nRadix to requested nRadix.  Assumes the nRadix
//    specified is the nRadix of the number passed in.
//
//-----------------------------------------------------------------------------

PNUMBER numtonRadixx( IN PNUMBER a, IN unsigned long nRadix, IN long ratio )

{
    PNUMBER pnumret = NULL;        // pnumret is the number in internal form.
    PNUMBER thisdigit = NULL;      // thisdigit holds the current digit of a
                                   // being summed into result.
    PNUMBER powofnRadix = NULL;    // offset of external base exponent.
    MANTTYPE *ptrdigit;            // pointer to digit being worked on.
    long idigit;                   // idigit is the iterate of digits in a.


    pnumret = longtonum( 0, BASEX );

    ptrdigit = MANT(a);
    
    // Digits are in reverse order, back over them LSD first.
    ptrdigit += a->cdigit-1;
    

    for ( idigit = 0; idigit < a->cdigit; idigit++ )
        {
        mulnumx( &pnumret, num_nRadix );
        // BUGBUG:
        // This should just smack in each digit into a 'special' thisdigit.
        // and not do the overhead of recreating the number type each time.
        thisdigit = longtonum( *ptrdigit--, BASEX );
        addnum( &pnumret, thisdigit, BASEX );
        destroynum( thisdigit );
        }
    DUPNUM( powofnRadix, num_nRadix );

    // Calculate the exponent of the external base for scaling.
    numpowlongx( &powofnRadix, a->exp );

    // ... and scale the result.
    mulnumx( &pnumret, powofnRadix );
    
    destroynum( powofnRadix );
    
    // And propagate the sign.
    pnumret->sign = a->sign;

    return( pnumret );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: inrat
//
//  ARGUMENTS:
//              fMantIsNeg true if mantissa is less than zero
//              pszMant a string representation of a number
//              fExpIsNeg  true if exponent is less than zero
//              pszExp a string representation of a number
//
//  RETURN: prat    representation of string input.
//          Or NULL if no number scanned.
//
//  EXPLANATION: This is for calc.
//
//
//-----------------------------------------------------------------------------

PRAT inrat( IN BOOL fMantIsNeg, IN LPTSTR pszMant, IN BOOL fExpIsNeg, 
    IN LPTSTR pszExp )

{
    PNUMBER pnummant=NULL;              // holds mantissa in number form.
    PNUMBER pnumexp=NULL;               // holds exponent in number form.
    PRAT pratexp=NULL;                  // holds exponent in rational form.
    PRAT prat=NULL;                     // holds exponent in rational form.
    long expt;                          // holds exponent
    
    // Deal with Mantissa
    if ( ( pszMant == NULL ) || ( *pszMant == TEXT('\0') ) )
        {
        // Preset value if no mantissa
        if ( ( pszExp == NULL ) || ( *pszExp == TEXT('\0') ) )
            {
            // Exponent not specified, preset value to zero
            DUPRAT(prat,rat_zero);
            }
        else
            {
            // Exponent specified, preset value to one
            DUPRAT(prat,rat_one);
            }
        }
    else
        {
        // Mantissa specified, convert to number form.
        pnummant = innum( pszMant );
        if ( pnummant == NULL )
            {
            return( NULL );
            }
        prat = numtorat( pnummant, nRadix );
        // convert to rational form, and cleanup.
        destroynum(pnummant);
        }

    if ( ( pszExp == NULL ) || ( *pszExp == TEXT('\0') ) )
        {
        // Exponent not specified, preset value to zero
        expt=0;
        }
    else
        {
        // Exponent specified, convert to number form.
        // Don't use native stuff, as it is restricted in the bases it can 
        // handle.
        pnumexp = innum( pszExp );
        if ( pnumexp == NULL )
            {
            return( NULL );
            }
        
        // Convert exponent number form to native integral form,  and cleanup.
        expt = numtolong( pnumexp, nRadix );
        destroynum( pnumexp );
        }
    
    
    // Convert native integral exponent form to rational multiplier form.
    pnumexp=longtonum( nRadix, BASEX );
    numpowlongx(&(pnumexp),abs(expt));
    createrat(pratexp);
    DUPNUM( pratexp->pp, pnumexp );
    pratexp->pq = longtonum( 1, BASEX );
    destroynum(pnumexp);

    if ( fExpIsNeg )
        {
        // multiplier is less than 1, this means divide.
        divrat( &prat, pratexp );
        }
    else
        {
        if ( expt > 0 )
            {
            // multiplier is greater than 1, this means divide.
            mulrat(&prat, pratexp);
            }
        // multiplier can be 1, in which case it'd be a waste of time to 
        // multiply.
        }

    if ( fMantIsNeg )
        {
        // A negative number was used, adjust the sign.
        prat->pp->sign *= -1;
        }
    return( prat );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: innum
//
//  ARGUMENTS:
//              TCHAR *buffer
//
//  RETURN: pnumber representation of string input.
//          Or NULL if no number scanned.
//
//  EXPLANATION: This is a state machine,
//
//    State      Description            Example, ^shows just read position.
//                                                which caused the transition
//
//    START      Start state            ^1.0
//    MANTS      Mantissa sign          -^1.0
//    LZ         Leading Zero           0^1.0
//    LZDP       Post LZ dec. pt.       000.^1
//    LD         Leading digit          1^.0
//    DZ         Post LZDP Zero         000.0^1
//    DD         Post Decimal digit     .01^2
//    DDP        Leading Digit dec. pt. 1.^2
//    EXPB       Exponent Begins        1.0e^2
//    EXPS       Exponent sign          1.0e+^5
//    EXPD       Exponent digit         1.0e1^2 or  even 1.0e0^1
//    EXPBZ      Exponent begin post 0  0.000e^+1
//    EXPSZ      Exponent sign post 0   0.000e+^1
//    EXPDZ      Exponent digit post 0  0.000e+1^2
//    ERR        Error case             0.0.^
//
//    Terminal   Description
//
//    DP         '.'
//    ZR         '0'
//    NZ         '1'..'9' 'A'..'Z' 'a'..'z' '@' '_'
//    SG         '+' '-'
//    EX         'e' '^' e is used for nRadix 10, ^ for all other nRadixs.
//
//-----------------------------------------------------------------------------

#define DP 0
#define ZR 1
#define NZ 2
#define SG 3
#define EX 4

#define START    0
#define MANTS    1
#define LZ       2
#define LZDP     3
#define LD       4
#define DZ       5
#define DD       6
#define DDP      7
#define EXPB     8
#define EXPS     9
#define EXPD     10
#define EXPBZ    11
#define EXPSZ    12
#define EXPDZ    13
#define ERR      14

#if defined( DEBUG )
char *statestr[] = {
    "START",
    "MANTS",
    "LZ",
    "LZDP",
    "LD",
    "DZ",
    "DD",
    "DDP",
    "EXPB",
    "EXPS",
    "EXPD",
    "EXPBZ",
    "EXPSZ",
    "EXPDZ",
    "ERR",
};
#endif

// New state is machine[state][terminal]
char machine[ERR+1][EX+1]= {
    //        DP,     ZR,      NZ,      SG,     EX
    // START    
        {     LZDP,   LZ,      LD,      MANTS,  ERR },
    // MANTS    
        {     LZDP,   LZ,      LD,      ERR,    ERR },
    // LZ    
        {     LZDP,   LZ,      LD,      ERR,    EXPBZ },
    // LZDP    
        {     ERR,    DZ,      DD,      ERR,    EXPB },
    // LD    
        {     DDP,    LD,      LD,      ERR,    EXPB },
    // DZ        
        {     ERR,    DZ,      DD,      ERR,    EXPBZ },
    // DD    
        {     ERR,    DD,      DD,      ERR,    EXPB },
    // DDP    
        {     ERR,    DD,      DD,      ERR,    EXPB },
    // EXPB    
        {     ERR,    EXPD,    EXPD,    EXPS,   ERR },
    // EXPS    
        {     ERR,    EXPD,    EXPD,    ERR,    ERR },
    // EXPD    
        {     ERR,    EXPD,    EXPD,    ERR,    ERR },
    // EXPBZ    
        {     ERR,    EXPDZ,   EXPDZ,   EXPSZ,  ERR },
    // EXPSZ    
        {     ERR,    EXPDZ,   EXPDZ,   ERR,    ERR },
    // EXPDZ    
        {     ERR,    EXPDZ,   EXPDZ,   ERR,    ERR },
    // ERR    
        {     ERR,    ERR,     ERR,     ERR,    ERR }
};


PNUMBER innum( IN TCHAR *buffer )

{
    int c;                    // c is character being worked on currently.
    int state;                // state is the state of the input state machine.
    long exps = 1L;           // exps is exponent sign ( +/- 1 )
    long expt = 0L;           // expt is exponent mantissa, should be unsigned
    long length = 0L;         // length is the length of the input string.
    MANTTYPE *pmant;          //
    PNUMBER pnumret=NULL;     //

    length = _tcslen(buffer);
    createnum( pnumret, length );
    pnumret->sign = 1L;
    pnumret->cdigit = 0;
    pnumret->exp = 0;
    pmant = MANT(pnumret)+length-1;
    state = START;
    fparserror=FALSE;        // clear global flag for parse error initially.
    while ( ( c = *buffer ) && c != TEXT('\n') )
        {
        int dp;
        dp = 0;
        // Added code to deal with international decimal point.
        while ( szDec[dp] && ( szDec[dp] == *buffer ) )
            {
            dp++;
            buffer++;
            }
        if ( dp )
            {
            if ( szDec[dp] == TEXT('\0') )
                {
                // OK pretend that was a decimal point for the state machine
                c = TEXT('.');
                buffer--;
                }
            else
                {
                // Backup that was no decimal point
                buffer -= (dp-1);
                c = *buffer++;
                }
            }
        switch ( c )
            {
        case TEXT('-'):
        case TEXT('+'):
            state=machine[state][SG];
            break;
        case TEXT('.'):
            state=machine[state][DP];
            break;
        case TEXT('0'):
            state=machine[state][ZR];
            break;
        case TEXT('^'):
        case TEXT('e'):
            if ( ( c == TEXT('^') ) || ( nRadix == 10 ) )
                {
                state=machine[state][EX];
                break;
                }
        // WARNING tricky dropthrough in the TEXT('e') as a digit case!!!
        default:
            state=machine[state][NZ];
            break;
            }
        switch ( state )
            {
        case MANTS:
            pnumret->sign = ( ( c == TEXT('-') ) ? -1 : 1);
            break;
        case EXPSZ:
        case EXPS:
            exps = ( ( c == TEXT('-') ) ? -1 : 1);
            break;
        case EXPDZ:
        case EXPD:
            {
            TCHAR *ptr;               // offset into digit table.
            if ( ( nRadix <= 36 ) && ( nRadix > 10 ) )
                {
                c = toupper( c );
                }
            ptr = _tcschr( digits, c );
            if ( ptr != NULL )
                {
                expt *= nRadix;
                expt += (long)(ptr - digits);
                }
            else
                {
                state=ERR;
                }
            }
            break;
        case LD:
            pnumret->exp++;
        case DD:
            {
            TCHAR *ptr;               // offset into digit table.
            if ( ( nRadix <= 36 ) && ( nRadix > 10 ) )
                {
                // Allow upper and lower case letters as equivalent, base
                // is in the range where this is not ambiguous.
                c = toupper( c );
                }
            ptr = _tcschr( digits, c );
            if ( ptr != NULL && ( (ptr - digits) < nRadix ) )
                {
                *pmant-- = (MANTTYPE)(ptr - digits);
                pnumret->exp--;
                pnumret->cdigit++;
                }
            else
                {
                state=ERR;
                // set global flag for parse error just in case anyone cares.
                fparserror=TRUE;
                }
            }
            break;
        case DZ:
            pnumret->exp--;
            break;
        case LZ:
        case LZDP:
        case DDP:
            break;
            }
        buffer++;
        }
    if ( state == DZ || state == EXPDZ )
        {
        pnumret->cdigit = 1;
        pnumret->exp=0;
        pnumret->sign=1;
        }
    else
        {
        while ( pnumret->cdigit < length )
            {
            pnumret->cdigit++;
            pnumret->exp--;
            }
        pnumret->exp += exps*expt;
        }


    if ( pnumret->cdigit == 0 )
        {
        destroynum( pnumret );
        pnumret = NULL;
        }
    stripzeroesnum( pnumret, maxout );
    return( pnumret );
}



//-----------------------------------------------------------------------------
//
//    FUNCTION: longtorat
//
//    ARGUMENTS: long
//
//    RETURN: Rational representation of long input.
//
//    DESCRIPTION: Converts long input to rational (p over q)
//    form, where q is 1 and p is the long.
//
//-----------------------------------------------------------------------------

PRAT longtorat( IN long inlong )

{
    PRAT pratret=NULL;
    createrat( pratret );
    pratret->pp = longtonum(inlong, BASEX );
    pratret->pq = longtonum(1L, BASEX );
    return( pratret );
}


//-----------------------------------------------------------------------------
//
//    FUNCTION: realtorat
//
//    ARGUMENTS: double real value.
//
//    RETURN: Rational representation of the double
//
//    DESCRIPTION: returns the rational (p over q)
//    representation of the double.
//
//-----------------------------------------------------------------------------

PRAT realtorat( IN double real )

{
#if !defined( CLEVER )
    // get clever later, right now hack something to work
    TCHAR *ptr;
    PNUMBER pnum=NULL;
    PRAT prat=NULL;
    if ( ( ptr = (TCHAR*)zmalloc( 60 * sizeof(TCHAR) ) ) != NULL )
        {
        _stprintf( ptr, TEXT("%20.20le"), real );
        pnum=innum( ptr );
        prat = numtorat( pnum, nRadix );
        destroynum( pnum );
        zfree( ptr );
        return( prat );
        }
    else
        {
        return( NULL );
        }
#else
    int i;
    union {
        double real;
        BYTE split[8];
    } unpack;
    long expt;
    long ratio;
    MANTTYPE *pmant;
    PNUMBER pnumret = NULL;
    PRAT pratret = NULL;

    createrat( pratret );

    if ( real == 0.0 )
        {
        pnumret=longtonum( 0L, 2L );
        }
    else
        {
        unpack.real=real;

        expt=unpack.split[7]*0x100+(unpack.split[6]>>4)-1023;
        createnum( pnumret, 52 );
        pmant = MANT(pnumret);
        for ( i = 63; i > 10; i-- )
            {
            *pmant++ = (MANTTYPE)((unpack.split[i/8]&(1<<(i%8)))!=0);
            }
        pnumret->exp=expt-52;
        pnumret->cdigit=52;
        }

    ratio = 1;
    while ( ratio > BASEX )
        {
        ratio *= 2;
        }

    pratret->pp = numtonRadixx( pnumret, 2, ratio );
    destroynum( pnumret );
    
    pratret->pq=longtonum( 1L, BASEX );

    if ( pratret->pp->exp < 0 )
        {
        pratret->pq->exp -= pratret->pp->exp;
        pratret->pp->exp = 0;
        }

    return( pratret );
#endif
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: longtonum
//
//    ARGUMENTS: long input and nRadix requested.
//
//    RETURN: number
//
//    DESCRIPTION: Returns a number representation in the
//    base   requested of the long value passed in.
//
//-----------------------------------------------------------------------------

PNUMBER longtonum( IN long inlong, IN unsigned long nRadix )

{
    MANTTYPE *pmant;
    PNUMBER pnumret=NULL;

    createnum( pnumret, MAX_LONG_SIZE );
    pmant = MANT(pnumret);
    pnumret->cdigit = 0;
    pnumret->exp = 0;
    if ( inlong < 0 )
        {
        pnumret->sign = -1;
        inlong *= -1;
        }
    else
        {
        pnumret->sign = 1;
        }

    do    {
        *pmant++ = (MANTTYPE)(inlong % nRadix);
        inlong /= nRadix;
        pnumret->cdigit++;
        } while ( inlong );

    return( pnumret );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: rattolong
//
//    ARGUMENTS: rational number in internal base.
//
//    RETURN: long
//
//    DESCRIPTION: returns the long representation of the
//    number input.  Assumes that the number is in the internal
//    base.
//
//-----------------------------------------------------------------------------

long rattolong( IN PRAT prat )

{
    long lret;
    PRAT pint = NULL;

    if ( rat_gt( prat, rat_dword ) || rat_lt( prat, rat_min_long ) )
        {
        // Don't attempt rattolong of anything too big or small
        throw( CALC_E_DOMAIN );
        }

    DUPRAT(pint,prat);

    intrat( &pint );
    divnumx( &(pint->pp), pint->pq );
    DUPNUM( pint->pq, num_one );

    lret = numtolong( pint->pp, BASEX );

    destroyrat(pint);

    return( lret );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: numtolong
//
//    ARGUMENTS: number input and base   of that number.
//
//    RETURN: long
//
//    DESCRIPTION: returns the long representation of the
//    number input.  Assumes that the number is really in the
//    base   claimed.
//
//-----------------------------------------------------------------------------

long numtolong( IN PNUMBER pnum, IN unsigned long nRadix )

{
    long lret;
    long expt;
    long length;
    MANTTYPE *pmant;

    lret = 0;
    pmant = MANT( pnum );
    pmant += pnum->cdigit - 1;

    expt = pnum->exp;
    length = pnum->cdigit;
    while ( length > 0  && length + expt > 0 )
        {
        lret *= nRadix;
        lret += *(pmant--);
        length--;
        }
    while ( expt-- > 0 )
        {
        lret *= (long)nRadix;
        }
    lret *= pnum->sign;
    return( lret );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: BOOL stripzeroesnum
//
//    ARGUMENTS:            a number representation
//
//    RETURN: TRUE if stripping done, modifies number in place.
//
//    DESCRIPTION: Strips off trailing zeroes.
//
//-----------------------------------------------------------------------------

BOOL stripzeroesnum( IN OUT PNUMBER pnum, long starting )

{
    MANTTYPE *pmant;
    long cdigits;
    BOOL fstrip = FALSE;
    
    // point pmant to the LeastCalculatedDigit
    pmant=MANT(pnum);
    cdigits=pnum->cdigit; 
    // point pmant to the LSD
    if ( cdigits > starting )
        {
        pmant += cdigits - starting;
        cdigits = starting;
        }
    
    // Check we haven't gone too far, and we are still looking at zeroes.
    while ( ( cdigits > 0 ) && !(*pmant) )
        {
        // move to next significant digit and keep track of digits we can 
    // ignore later.
        pmant++;
        cdigits--;
        fstrip = TRUE;
        }
    
    // If there are zeroes to remove.
    if ( fstrip )
        {
        // Remove them.
        memcpy( MANT(pnum), pmant, (int)(cdigits*sizeof(MANTTYPE)) );
        // And adjust exponent and digit count accordingly.
        pnum->exp += ( pnum->cdigit - cdigits );
        pnum->cdigit = cdigits;
        }
    return( fstrip );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: putnum
//
//    ARGUMENTS: number representation
//          fmt, one of FMT_FLOAT FMT_SCIENTIFIC or
//          FMT_ENGINEERING
//
//    RETURN: String representation of number.
//
//    DESCRIPTION: Converts a number to it's string
//    representation.  Returns a string that should be
//    zfree'd after use.
//
//-----------------------------------------------------------------------------

TCHAR *putnum( IN PNUMBER *ppnum, IN int fmt )

{
    TCHAR *psz;
    TCHAR *pret;
    long expt;        // Actual number of digits to the left of decimal
    long eout;        // Displayed exponent.
    long cexp;        // the size of the exponent needed.
    long elen;
    long length;
    MANTTYPE *pmant;
    int fsciform=0;    // If true scientific form is called for.
    PNUMBER pnum;
    PNUMBER round=NULL;
    long oldfmt = fmt;


    pnum=*ppnum;
    stripzeroesnum( pnum, maxout+2 );
    length = pnum->cdigit;
    expt = pnum->exp+length;
    if ( ( expt > maxout ) && ( fmt == FMT_FLOAT ) )
        {
        // Force scientific mode to prevent user from assuming 33rd digit is
        // exact.
        fmt = FMT_SCIENTIFIC;
        }

    
    // Make length small enough to fit in pret.
    if ( length > maxout )
        {
        length = maxout;
        }

    eout=expt-1;
    cexp = longlognRadix( expt );

    // 2 for signs, 1 for 'e'(or leading zero), 1 for dp, 1 for null and
    // 10 for maximum exponent size.
    pret = (TCHAR*)zmalloc( (maxout + 16) * sizeof(TCHAR) );
    psz = pret;

    // If there is a chance a round has to occour, round.
    if ( 
        // if number is zero no rounding.
        !zernum( pnum ) && 
        // if number of digits is less than the maximum output no rounding.
        pnum->cdigit >= maxout 
        )
        {
        // Otherwise round.
        round=longtonum( nRadix, nRadix );
        divnum(&round, num_two, nRadix );

        // Make round number exponent one below the LSD for the number.
        round->exp = pnum->exp + pnum->cdigit - round->cdigit - maxout;
        round->sign = pnum->sign;
        }

    if ( fmt == FMT_FLOAT )
        {
        // cexp will now contain the size required by exponential.
        // Figure out if the exponent will fill more space than the nonexponent field.
        if ( ( length - expt > maxout + 2 ) || ( expt > maxout + 3 ) )
            {
            // Case where too many zeroes are to the right or left of the 
            // decimal pt. And we are forced to switch to scientific form.
            fmt = FMT_SCIENTIFIC;
            }
        else
            {
            // Minimum loss of precision occours with listing leading zeros
            // if we need to make room for zeroes sacrifice some digits.
            if ( length + abs(expt) < maxout )
                {
                if ( round )
                    {
                    round->exp -= expt;
                    }
                }
            }
        }
    if ( round != NULL )
    	{
        BOOL fstrip=FALSE;
        long offset;
    	addnum( ppnum, round, nRadix );
    	pnum=*ppnum;
        offset=(pnum->cdigit+pnum->exp) - (round->cdigit+round->exp);
        fstrip = stripzeroesnum( pnum, offset );
        destroynum( round );
        if ( fstrip )
            {
            // WARNING: nesting/recursion, too much has been changed, need to
            // refigure format.
            return( putnum( &pnum, oldfmt ) );
            }
    	}
    else
    	{
        stripzeroesnum( pnum, maxout );
    	}

    // Set up all the post rounding stuff.
    pmant = MANT(pnum)+pnum->cdigit-1;

    if ( 
        // Case where too many digits are to the left of the decimal or 
        // FMT_SCIENTIFIC or FMT_ENGINEERING was specified.
        ( fmt == FMT_SCIENTIFIC ) ||
        ( fmt == FMT_ENGINEERING ) )

        {
        fsciform=1;
        if ( eout != 0 )
            {

            if ( fmt == FMT_ENGINEERING )
                {
                expt = (eout % 3);
                eout -= expt;
                expt++;
                
                // Fix the case where 0.02e-3 should really be 2.e-6 etc.
                if ( expt < 0 )
                    {
                    expt += 3;
                    eout -= 3;
                    }

                }
            else
                {
                expt = 1;
                }
            }
        }
    else
        {
        fsciform=0;
        eout=0;
        }
    
    // Make sure negative zeroes aren't allowed.
    if ( ( pnum->sign == -1 ) && ( length > 0 ) )
        {
        *psz++ = TEXT('-');
        }

    if ( ( expt <= 0 ) && ( fsciform == 0 ) )
        {
        *psz++ = TEXT('0');
        *psz++ = szDec[0];
        // Used up a digit unaccounted for.
        }
    while ( expt < 0 )
        {
        *psz++ = TEXT('0');
        expt++;
        }

    while ( length > 0 )
        {
        expt--;
        *psz++ = digits[ *pmant-- ];
        length--;
        // Be more regular in using a decimal point.
        if ( expt == 0 )
            {
            *psz++ = szDec[0];
            }
        }

    while ( expt > 0 )
        {
        *psz++ = TEXT('0');
        expt--;
        // Be more regular in using a decimal point.
        if ( expt == 0 )
            {
            *psz++ = szDec[0];
            }
        }


    if ( fsciform )
        {
        if ( nRadix == 10 )
            {
            *psz++ = TEXT('e');
            }
        else
            {
            *psz++ = TEXT('^');
            }
        *psz++ = ( eout < 0 ? TEXT('-') : TEXT('+') );
        eout = abs( eout );
        elen=0;
        do
            {
            // should this be eout % nRadix?  or is that insane?
            *psz++ = digits[ eout % nRadix ];
            elen++;
            eout /= nRadix;
            } while ( eout > 0 );
        *psz = TEXT('\0');
        _tcsrev( &(psz[-elen]) );
        }
    *psz = TEXT('\0');
    return( pret );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: putrat
//
//  ARGUMENTS:
//              PRAT *representation of a number.
//              long representation of base  to  dump to screen.
//              fmt, one of FMT_FLOAT FMT_SCIENTIFIC or FMT_ENGINEERING
//
//  RETURN: string
//
//  DESCRIPTION: returns a string representation of rational number passed
//  in, at least to the maxout digits.  String returned should be zfree'd
//  after use.
//
//  NOTE: It may be that doing a GCD() could shorten the rational form
//       And it may eventually be worthwhile to keep the result.  That is
//       why a pointer to the rational is passed in.
//
//-----------------------------------------------------------------------------

TCHAR *putrat( IN OUT PRAT *pa, IN unsigned long nRadix, IN int fmt )

{
    TCHAR *psz;
    PNUMBER p=NULL;
    PNUMBER q=NULL;
    long scaleby=0;
    
    
    // Convert p and q of rational form from internal base to requested base.
    
    // Scale by largest power of BASEX possible.

    scaleby=min((*pa)->pp->exp,(*pa)->pq->exp);
    if ( scaleby < 0 )
        {
        scaleby = 0;
        }
    (*pa)->pp->exp -= scaleby;
    (*pa)->pq->exp -= scaleby;

    p = nRadixxtonum( (*pa)->pp, nRadix );

    q = nRadixxtonum( (*pa)->pq, nRadix );
    
    // finally take the time hit to actually divide.
    divnum( &p, q, nRadix );
    
    psz = putnum( &p, fmt );
    destroynum( p );
    destroynum( q );
    return( psz );
}


//-----------------------------------------------------------------------------
//
//  FUNCTION: gcd
//
//  ARGUMENTS:
//              PNUMBER representation of a number.
//              PNUMBER representation of a number.
//
//  RETURN: Greatest common divisor in internal BASEX PNUMBER form.
//
//  DESCRIPTION: gcd uses remainders to find the greatest common divisor.
//
//  ASSUMPTIONS: gcd assumes inputs are integers.
//
//  NOTE: Before GregSte and TimC proved the TRIM macro actually kept the
//        size down cheaper than GCD, this routine was used extensively.
//        now it is not used but might be later.
//
//-----------------------------------------------------------------------------

PNUMBER gcd( IN PNUMBER a, IN PNUMBER b )

{
    PNUMBER r=NULL;
    PNUMBER tmpa=NULL;
    PNUMBER tmpb=NULL;

    if ( lessnum( a, b ) )
        {
        DUPNUM(tmpa,b);
        if ( zernum(a) )
            {
            return(tmpa);
            }
        DUPNUM(tmpb,a);
        }
    else
        {
        DUPNUM(tmpa,a);
        if ( zernum(b) )
            {
            return(tmpa);
            }
        DUPNUM(tmpb,b);
        }
    
    remnum( &tmpa, tmpb, nRadix );
    while ( !zernum( tmpa ) )
        {
        // swap tmpa and tmpb
        r = tmpa;
        tmpa = tmpb;
        tmpb = r;
        remnum( &tmpa, tmpb, nRadix );
        }
    destroynum( tmpa );
    return( tmpb );

}

//-----------------------------------------------------------------------------
//
//  FUNCTION: longfactnum
//
//  ARGUMENTS:
//              long integer to factorialize.
//              long integer representing base   of answer.
//
//  RETURN: Factorial of input in nRadix PNUMBER form.
//
//  NOTE:  Not currently used.
//
//-----------------------------------------------------------------------------

PNUMBER longfactnum( IN long inlong, IN unsigned long nRadix )

{
    PNUMBER lret=NULL;
    PNUMBER tmp=NULL;
    PNUMBER tmp1=NULL;

    lret = longtonum( 1, nRadix );

    while ( inlong > 0 )
        {
        tmp = longtonum( inlong--, nRadix );
        mulnum( &lret, tmp, nRadix );
        destroynum( tmp );
        }
    return( lret );
}

//-----------------------------------------------------------------------------
//
//  FUNCTION: longprodnum
//
//  ARGUMENTS:
//              long integer to factorialize.
//              long integer representing base of answer.
//
//  RETURN: Factorial of input in base PNUMBER form.
//
//-----------------------------------------------------------------------------

PNUMBER longprodnum( IN long start, IN long stop, IN unsigned long nRadix )

{
    PNUMBER lret=NULL;
    PNUMBER tmp=NULL;

    lret = longtonum( 1, nRadix );

    while ( start <= stop )
        {
        if ( start )
            {
            tmp = longtonum( start, nRadix );
            mulnum( &lret, tmp, nRadix );
            destroynum( tmp );
            }
        start++;
        }
    return( lret );
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: numpowlong
//
//    ARGUMENTS: root as number power as long and nRadix of
//               number.
//
//    RETURN: None root is changed.
//
//    DESCRIPTION: changes numeric representation of root to
//    root ** power. Assumes nRadix is the nRadix of root.
//
//-----------------------------------------------------------------------------

void numpowlong( IN OUT PNUMBER *proot, IN long power, 
                IN unsigned long nRadix )

{
    PNUMBER lret=NULL;

    lret = longtonum( 1, nRadix );

    while ( power > 0 )
        {
        if ( power & 1 )
            {
            mulnum( &lret, *proot, nRadix );
            }
        mulnum( proot, *proot, nRadix );
        TRIMNUM(*proot);
        power >>= 1;
        }
    destroynum( *proot );
    *proot=lret;
    
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: ratpowlong
//
//    ARGUMENTS: root as rational, power as long.
//
//    RETURN: None root is changed.
//
//    DESCRIPTION: changes rational representation of root to
//    root ** power.
//
//-----------------------------------------------------------------------------

void ratpowlong( IN OUT PRAT *proot, IN long power )

{
    if ( power < 0 )
        {
        // Take the positive power and invert answer.
        PNUMBER pnumtemp = NULL;
        ratpowlong( proot, -power );
        pnumtemp = (*proot)->pp;
        (*proot)->pp  = (*proot)->pq;
        (*proot)->pq = pnumtemp;
        }
    else
        {
        PRAT lret=NULL;

        lret = longtorat( 1 );

        while ( power > 0 )
            {
            if ( power & 1 )
                {
                mulnumx( &(lret->pp), (*proot)->pp );
                mulnumx( &(lret->pq), (*proot)->pq );
                }
            mulrat( proot, *proot );
            trimit(&lret);
            trimit(proot);
            power >>= 1;
            }
        destroyrat( *proot );
        *proot=lret;
        }
}

//-----------------------------------------------------------------------------
//
//    FUNCTION: longlog10
//
//    ARGUMENTS: number as long.
//
//    RETURN: returns int(log10(abs(number)+1)), useful in formatting output
//
//-----------------------------------------------------------------------------

long longlognRadix( long x )

{
    long ret = 0;
    x--;
    if ( x < 0 ) 
        {
        x = -x;
        }
    while ( x )
        {
        ret++;
        x /= nRadix;
        }
    return( ret );
}
