extern "C" {
#pragma warning( disable : 4200 )
//----------------------------------------------------------------------------- 
//  Package Title  ratpak
//  File           ratpak.h
//  Author         Timothy David Corrie Jr. (timc@microsoft.com)
//  Copyright      (C) 1995-99 Microsoft
//  Date           01-16-95
//
//
//  Description
//
//     Infinite precision math package header file, if you use ratpak.lib you
//  need to include this header.
//
//----------------------------------------------------------------------------- 

#include "CalcErr.h"

#define BASEXPWR 31L    // Internal log2(BASEX)
#define BASEX 0x80000000 // Internal nRadix used in calculations, hope to raise
                        // this to 2^32 after solving scaling problems with
                        // overflow detection esp. in mul

typedef unsigned long MANTTYPE;
typedef unsigned __int64 TWO_MANTTYPE;

enum eNUMOBJ_FMT {
    FMT_FLOAT,        // returns floating point, or exponential if number is too big
    FMT_SCIENTIFIC,    // always returns scientific notation
    FMT_ENGINEERING    // always returns engineering notation such that exponent is a multiple of 3

};

enum eANGLE_TYPE {
    ANGLE_DEG,    // Calculate trig using 360 degrees per revolution
    ANGLE_RAD,    // Calculate trig using 2 pi  radians per revolution
    ANGLE_GRAD    // Calculate trig using 400 gradients per revolution

};

typedef enum eNUMOBJ_FMT NUMOBJ_FMT;
typedef enum eANGLE_TYPE ANGLE_TYPE;

typedef int BOOL;

//----------------------------------------------------------------------------- 
//
//  NUMBER type is a representation of a generic sized generic nRadix number
//
//----------------------------------------------------------------------------- 

typedef struct _number
    {
    long sign;        // The sign of the mantissa, +1, or -1
    long cdigit;    // The number of digits, or what passes for digits in the
                    // nRadix being used.
    long exp;       // The offset of digits from the nRadix point 
                    // (decimal point in nRadix 10)
    MANTTYPE mant[0];  
                    // This is actually allocated as a continuation of the 
                    // NUMBER structure.
    } NUMBER, *PNUMBER, **PPNUMBER;


//----------------------------------------------------------------------------- 
//
//  RAT type is a representation nRadix  on 2 NUMBER types.
//  pp/pq, where pp and pq are pointers to integral NUMBER types.
//
//----------------------------------------------------------------------------- 

typedef struct _rat
    {
    PNUMBER pp;
    PNUMBER pq;
    } RAT, *PRAT;

//----------------------------------------------------------------------------- 
//
//  LINKEDLIST is an aid for division, it contains foreward and reverse links
//  to a list of NUMBERS.
//
//----------------------------------------------------------------------------- 

typedef struct _linkedlist
    {
    PNUMBER pnum;
    struct _linkedlist *llnext;
    struct _linkedlist *llprev;
    } LINKEDLIST, *PLINKEDLIST;




#if !defined( TRUE )
#define TRUE 1
#endif

#if !defined( FALSE )
#define FALSE 0
#endif

#define MAX_LONG_SIZE 33    // Base 2 requires 32 'digits'

//-----------------------------------------------------------------------------
//
// List of useful constants for evaluation, note this list needs to be
// initialized.
//
//-----------------------------------------------------------------------------

extern PNUMBER num_one;
extern PNUMBER num_two;
extern PNUMBER num_five;
extern PNUMBER num_six;
extern PNUMBER num_nRadix;
extern PNUMBER num_ten;

extern PRAT ln_ten;
extern PRAT ln_two;
extern PRAT rat_zero;
extern PRAT rat_neg_one;
extern PRAT rat_one;
extern PRAT rat_two;
extern PRAT rat_six;
extern PRAT rat_half;
extern PRAT rat_ten;
extern PRAT pt_eight_five;
extern PRAT pi;
extern PRAT pi_over_two;
extern PRAT two_pi;
extern PRAT one_pt_five_pi;
extern PRAT e_to_one_half;
extern PRAT rat_exp;
extern PRAT rad_to_deg;
extern PRAT rad_to_grad;
extern PRAT rat_qword;
extern PRAT rat_dword;
extern PRAT rat_word;
extern PRAT rat_byte;
extern PRAT rat_360;
extern PRAT rat_400;
extern PRAT rat_180;
extern PRAT rat_200;
extern PRAT rat_nRadix;
extern PRAT rat_smallest;
extern PRAT rat_negsmallest;
extern PRAT rat_max_exp;
extern PRAT rat_min_exp;
extern PRAT rat_min_long;


// MANT returns a long pointer to the mantissa of number 'a'
#define MANT(a) ((a)->mant)

// DUPNUM Duplicates a number taking care of allocation and internals
#define DUPNUM(a,b) destroynum(a);createnum( a, b->cdigit ); \
    memcpy( a, b, (int)( sizeof( NUMBER ) + ( b->cdigit )*(sizeof(MANTTYPE)) ) );

// DUPRAT Duplicates a rational taking care of allocation and internals
#define DUPRAT(a,b) destroyrat(a);createrat(a);DUPNUM((a)->pp,(b)->pp);DUPNUM((a)->pq,(b)->pq);

// LOG*RADIX calculates the integral portion of the log of a number in
// the base currently being used, only accurate to within ratio

#define LOGNUMRADIX(pnum) (((pnum)->cdigit+(pnum)->exp)*ratio)
#define LOGRATRADIX(prat) (LOGNUMRADIX((prat)->pp)-LOGNUMRADIX((prat)->pq))

// LOG*2 calculates the integral portion of the log of a number in
// the internal base being used, only accurate to within ratio

#define LOGNUM2(pnum) ((pnum)->cdigit+(pnum)->exp)
#define LOGRAT2(prat) (LOGNUM2((prat)->pp)-LOGNUM2((prat)->pq))

#if defined( DEBUG )
//----------------------------------------------------------------------------- 
//
//   Debug versions of rational number creation and destruction routines.
//   used for debugging allocation errors.
//
//----------------------------------------------------------------------------- 

#define createrat(y) y=_createrat();fprintf( stderr, "createrat %lx %s file= %s, line= %d\n", y, # y, __FILE__, __LINE__ )
#define destroyrat(x) fprintf( stderr, "destroyrat %lx file= %s, line= %d\n", x, __FILE__, __LINE__ ),_destroyrat(x),x=NULL
#define createnum(y,x) y=_createnum(x);fprintf( stderr, "createnum %lx %s file= %s, line= %d\n", y, # y, __FILE__, __LINE__ );
#define destroynum(x) fprintf( stderr, "destroynum %lx file= %s, line= %d\n", x, __FILE__, __LINE__ ),_destroynum(x),x=NULL
#else
#define createrat(y) y=_createrat()
#define destroyrat(x) _destroyrat(x),x=NULL
#define createnum(y,x) y=_createnum(x)
#define destroynum(x) _destroynum(x),x=NULL
#endif

//----------------------------------------------------------------------------- 
//
//   Defines for checking when to stop taylor series expansions due to
//   precision satisfaction.
//
//----------------------------------------------------------------------------- 

// RENORMALIZE, gets the exponents non-negative.
#define RENORMALIZE(x) if ( (x)->pp->exp < 0 ) { \
	(x)->pq->exp -= (x)->pp->exp; \
	(x)->pp->exp = 0; \
	} \
	if ( (x)->pq->exp < 0 ) { \
	(x)->pp->exp -= (x)->pq->exp; \
	(x)->pq->exp = 0; \
	}

// TRIMNUM ASSUMES the number is in nRadix form NOT INTERNAL BASEX!!!
#define TRIMNUM(x) if ( !ftrueinfinite ) { \
		long trim = (x)->cdigit - maxout-ratio;\
            if ( trim > 1 ) \
                { \
memmove( MANT(x), &(MANT(x)[trim]), sizeof(MANTTYPE)*((x)->cdigit-trim) ); \
                (x)->cdigit -= trim; \
                (x)->exp += trim; \
                } \
            }
// TRIMTOP ASSUMES the number is in INTERNAL BASEX!!!
#define TRIMTOP(x) if ( !ftrueinfinite ) { \
		long trim = (x)->pp->cdigit - (maxout/ratio) - 2;\
            if ( trim > 1 ) \
                { \
memmove( MANT((x)->pp), &(MANT((x)->pp)[trim]), sizeof(MANTTYPE)*((x)->pp->cdigit-trim) ); \
                (x)->pp->cdigit -= trim; \
                (x)->pp->exp += trim; \
                } \
            trim = min((x)->pp->exp,(x)->pq->exp);\
            (x)->pp->exp -= trim;\
            (x)->pq->exp -= trim;\
            }

#define CLOSE_ENOUGH_RAT(a,b) ( ( ( ( ( a->pp->cdigit + a->pp->exp ) - \
( a->pq->cdigit + a->pq->exp ) ) - ( ( b->pp->cdigit + b->pp->exp ) - \
( b->pq->cdigit + b->pq->exp ) ) ) * ratio > maxout ) || fhalt )

#define SMALL_ENOUGH_RAT(a) (zernum(a->pp) || ( ( ( a->pq->cdigit + a->pq->exp ) - ( a->pp->cdigit + a->pp->exp ) - 1 ) * ratio > maxout ) || fhalt )

//----------------------------------------------------------------------------- 
//
//   Defines for setting up taylor series expansions for infinite precision
//   functions.
//
//----------------------------------------------------------------------------- 

#define CREATETAYLOR() PRAT xx=NULL;\
    PNUMBER n2=NULL; \
    PRAT pret=NULL; \
    PRAT thisterm=NULL; \
    DUPRAT(xx,*px); \
    mulrat(&xx,*px); \
    createrat(pret); \
    pret->pp=longtonum( 0L, BASEX ); \
    pret->pq=longtonum( 0L, BASEX ); 

#define DESTROYTAYLOR() destroynum( n2 ); \
    destroyrat( xx );\
    destroyrat( thisterm );\
    destroyrat( *px );\
    trimit(&pret);\
    *px=pret;

// SUM(a,b) is the rational equivalent of a += b
#define SUM(a,b) addnum( &a, b, BASEX);

// INC(a) is the rational equivalent of a++ 
// Check to see if we can avoid doing this the hard way.
#define INC(a) if ( a->mant[0] < BASEX - 1 ) \
    { \
    a->mant[0]++; \
    } \
    else \
    { \
    addnum( &a, num_one, BASEX); \
    } 

#define MSD(x) ((x)->mant[(x)->cdigit-1])
// MULNUM(b) is the rational equivalent of thisterm *= b where thisterm is
// a rational and b is a number, NOTE this is a mixed type operation for
// efficiency reasons.
#define MULNUM(b) mulnumx( &(thisterm->pp), b);

// DIVNUM(b) is the rational equivalent of thisterm /= b where thisterm is
// a rational and b is a number, NOTE this is a mixed type operation for
// efficiency reasons.
#define DIVNUM(b) mulnumx( &(thisterm->pq), b);

// NEXTTERM(p,d) is the rational equivalent of
// thisterm *= p
// d    <d is usually an expansion of operations to get thisterm updated.>
// pret += thisterm
#define NEXTTERM(p,d) mulrat(&thisterm,p);d addrat( &pret, thisterm )

// ONEOVER(x) is the rational equivalent of x=1/x
#define ONEOVER(x) {PNUMBER __tmpnum;__tmpnum=x->pp;x->pp=x->pq;x->pq=__tmpnum;}

#ifndef DOS
#   if defined(ALTERNATE_ALLOCATION)
//----------------------------------------------------------------------------- 
//
//   WARNING if you change the allocation package you need to rebuild
//   ratpak.lib
//
//----------------------------------------------------------------------------- 

extern void *zmalloc( IN unsigned long sze );
extern void zfree( IN double *pd );
#   define zstrdup( x ) strcpy( zmalloc( strlen(x)+1 ), x )

#   else

#       ifdef USE_HEAPALLOC
//
// NT Heap macros.  Calling process must create a heap with HeapCreate()
//
#           define zmalloc(a)   HeapAlloc( hheap, 0, a )
#           define zfree(a)     HeapFree( hheap, 0, a )
#       elif DBG
//
// Debug heap workers
//
HLOCAL MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes);
HLOCAL MemFreeWorker(LPSTR szFile, int iLine, HLOCAL hMem);
#           define zmalloc(a)   MemAllocWorker( __FILE__, __LINE__, LPTR, a )
#           define zfree(a)     MemFreeWorker( __FILE__, __LINE__, a )

#       else
//
// Windows heap macros
//
#           define zmalloc(a)   LocalAlloc( LPTR, a )
#           define zfree(a)     LocalFree( a )

#       endif
#       define zstrdup( x ) strcpy( zmalloc( strlen(x)+1 ), x )

#   endif
#endif

//----------------------------------------------------------------------------- 
//
//   External variables used in the math package.
//
//----------------------------------------------------------------------------- 

extern BOOL fhalt;	// contains the command to halt execution if true.
extern BOOL fparserror;	// set to true if last innum ended in error, else false.
extern NUMOBJ_FMT fmt;	// contains the format to use
extern TCHAR szDec[5];     // extern decimal point representation
extern long nRadix;     // extern nRadix used for input and output routines
extern unsigned char ftrueinfinite; // set to true to allow infinite precision
                             // don't use unless you know what you are doing
                      // used to help decide when to stop calculating.
extern long maxout;   // Maximum digits nRadix <nRadix> to use for precision.
                      // used to help decide when to stop calculating.

extern long ratio;    // Internally calculated ratio of internal nRadix
                      // v.s. nRadix used for input output number routines

extern LPTSTR oom;     // Out of memory error message

typedef void ERRFUNC( LPTSTR szErr );
typedef ERRFUNC *LPERRFUNC;
extern LPERRFUNC glpErrFunc;    // This function will get called if an error
                        // occurs inside of ratpak.



#ifndef DOS
extern HANDLE hheap;  // hheap is a pointer used in allocation, ratpak.lib
                      // users responsibility to make sure this is set up
                      // for use with Heap{Alloc,Free} routines.
#endif


//----------------------------------------------------------------------------- 
//
//   External functions defined in the math package.
//
//----------------------------------------------------------------------------- 

// Call whenever radix changes and at start of program. (Obsolete)
extern void changeRadix( IN long nRadix );         
// Call whenever precision changes and at start of program. (Obsolete)
extern void changePrecision( IN long nPrecision );  

// Call whenever either nRadix or nPrecision changes, is smarter about
// recalculating constants. 
// (Prefered replacement for the ChangeRadix and ChangePrecision calls.)
extern void ChangeConstants( IN long nRadix, IN long nPrecision );  

extern BOOL equnum( IN PNUMBER a, IN PNUMBER b );     // returns true of a == b
extern BOOL lessnum( IN PNUMBER a, IN PNUMBER b );    // returns true of a < b
extern BOOL zernum( IN PNUMBER a );                // returns true of a == 0
extern BOOL zerrat( IN PRAT a );                   // returns true if a == 0/q
extern TCHAR *putnum( IN OUT PNUMBER *ppnum, IN int fmt );

// returns a text representation of a (*pa)
extern TCHAR *putrat( IN OUT PRAT *pa, IN unsigned long nRadix, IN int fmt );

extern long longpow( IN unsigned long nRadix, IN long power );
extern long numtolong( IN PNUMBER pnum, IN unsigned long nRadix );
extern long rattolong( IN PRAT prat );
extern PNUMBER _createnum( IN long size );         // returns an empty number structure with size digits
extern PNUMBER nRadixxtonum( IN PNUMBER a, IN unsigned long nRadix );
extern PNUMBER binomial( IN long lroot, IN PNUMBER digitnum, IN PNUMBER c, IN PLINKEDLIST pll, IN unsigned long nRadix );
extern PNUMBER gcd( IN PNUMBER a, IN PNUMBER b );
extern PNUMBER innum( IN LPTSTR buffer );           // takes a text representation of a number and returns a number.

// takes a text representation of a number as a mantissa with sign and an exponent with sign.
extern PRAT inrat( IN BOOL fMantIsNeg, IN LPTSTR pszMant, IN BOOL fExpIsNeg, IN LPTSTR pszExp );

extern PNUMBER longfactnum( IN long inlong, IN unsigned long nRadix );
extern PNUMBER longprodnum( IN long start, IN long stop, IN unsigned long nRadix );
extern PNUMBER longtonum( IN long inlong, IN unsigned long nRadix );
extern PNUMBER numtonRadixx( IN PNUMBER a, IN unsigned long nRadix, IN long ratio );

// creates a empty/undefined rational representation (p/q)
extern PRAT _createrat( void );            

// returns a new rat structure with the acos of x->p/x->q taking into account
// angle type
extern void acosanglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

// returns a new rat structure with the acosh of x->p/x->q
extern void acoshrat( IN OUT PRAT *px );

// returns a new rat structure with the acos of x->p/x->q
extern void acosrat( IN OUT PRAT *px );                  

// returns a new rat structure with the asin of x->p/x->q taking into account
// angle type
extern void asinanglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

extern void asinhrat( IN OUT PRAT *px ); 
// returns a new rat structure with the asinh of x->p/x->q

// returns a new rat structure with the asin of x->p/x->q
extern void asinrat( IN OUT PRAT *px );

// returns a new rat structure with the atan of x->p/x->q taking into account
// angle type
extern void atananglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

// returns a new rat structure with the atanh of x->p/x->q
extern void atanhrat( IN OUT PRAT *px );

// returns a new rat structure with the atan of x->p/x->q
extern void atanrat( IN OUT PRAT *px );

// returns a new rat structure with the atan2 of x->p/x->q, y->p/y->q
extern void atan2rat( IN OUT PRAT *py, IN PRAT y );

// returns a new rat structure with the cosh of x->p/x->q
extern void coshrat( IN OUT PRAT *px );

// returns a new rat structure with the cos of x->p/x->q
extern void cosrat( IN OUT PRAT *px );

// returns a new rat structure with the cos of x->p/x->q taking into account
// angle type
extern void cosanglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

// returns a new rat structure with the exp of x->p/x->q this should not be called explicitly.
extern void _exprat( IN OUT PRAT *px );

// returns a new rat structure with the exp of x->p/x->q
extern void exprat( IN OUT PRAT *px );

// returns a new rat structure with the log base 10 of x->p/x->q
extern void log10rat( IN OUT PRAT *px );

// returns a new rat structure with the natural log of x->p/x->q
extern void lograt( IN OUT PRAT *px );

extern PRAT longtorat( IN long inlong );
extern PRAT numtorat( IN PNUMBER pin, IN unsigned long nRadix );
extern PRAT realtorat( IN double real );

extern void sinhrat( IN OUT PRAT *px );
extern void sinrat( IN OUT PRAT *px );

// returns a new rat structure with the sin of x->p/x->q taking into account
// angle type
extern void sinanglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

extern void tanhrat( IN OUT PRAT *px );
extern void tanrat( IN OUT PRAT *px );

// returns a new rat structure with the tan of x->p/x->q taking into account
// angle type
extern void tananglerat( IN OUT PRAT *px, IN ANGLE_TYPE angletype );

extern void _destroynum( IN PNUMBER pnum );
extern void _destroyrat( IN PRAT prat );
extern void addnum( IN OUT PNUMBER *pa, IN PNUMBER b, unsigned long nRadix );
extern void addrat( IN OUT PRAT *pa, IN PRAT b );
extern void andrat( IN OUT PRAT *pa, IN PRAT b );
extern void const_init( void );
extern void divnum( IN OUT PNUMBER *pa, IN PNUMBER b, IN unsigned long nRadix );
extern void divnumx( IN OUT PNUMBER *pa, IN PNUMBER b );
extern void divrat( IN OUT PRAT *pa, IN PRAT b );
extern void fracrat( IN OUT PRAT *pa );
extern void factrat( IN OUT PRAT *pa );
extern void modrat( IN OUT PRAT *pa, IN PRAT b );
extern void gcdrat( IN OUT PRAT *pa );
extern void intrat( IN OUT PRAT *px);
extern void mulnum( IN OUT PNUMBER *pa, IN PNUMBER b, IN unsigned long nRadix );
extern void mulnumx( IN OUT PNUMBER *pa, IN PNUMBER b );
extern void mulrat( IN OUT PRAT *pa, IN PRAT b );
extern void numpowlong( IN OUT PNUMBER *proot, IN long power, IN unsigned long nRadix );
extern void numpowlongx( IN OUT PNUMBER *proot, IN long power );
extern void orrat( IN OUT PRAT *pa, IN PRAT b );
extern void powrat( IN OUT PRAT *pa, IN PRAT b );
extern void ratpowlong( IN OUT PRAT *proot, IN long power );
extern void remnum( IN OUT PNUMBER *pa, IN PNUMBER b, IN long nRadix );
extern void rootnum( IN OUT PNUMBER *pa, IN PNUMBER b, IN unsigned long nRadix );
extern void rootrat( IN OUT PRAT *pa, IN PRAT b );
extern void scale2pi( IN OUT PRAT *px );
extern void scale( IN OUT PRAT *px, IN PRAT scalefact );
extern void subrat( IN OUT PRAT *pa, IN PRAT b );
extern void xorrat( IN OUT PRAT *pa, IN PRAT b );
extern void lshrat( IN OUT PRAT *pa, IN PRAT b );
extern void rshrat( IN OUT PRAT *pa, IN PRAT b );
extern BOOL rat_equ( IN PRAT a, IN PRAT b );
extern BOOL rat_neq( IN PRAT a, IN PRAT b );
extern BOOL rat_gt( IN PRAT a, IN PRAT b );
extern BOOL rat_ge( IN PRAT a, IN PRAT b );
extern BOOL rat_lt( IN PRAT a, IN PRAT b );
extern BOOL rat_le( IN PRAT a, IN PRAT b );
extern void inbetween( IN PRAT *px, IN PRAT range );
extern DWORDLONG __inline Mul32x32( IN DWORD a, IN DWORD b );
//extern DWORDLONG __inline __fastcall Shr32xbase( IN DWORDLONG a );
extern void factnum( IN OUT PLINKEDLIST *ppllfact, PNUMBER pnum );
extern void trimit( IN OUT PRAT *px );
}
