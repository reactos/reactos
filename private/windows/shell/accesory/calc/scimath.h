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
#include "..\ratpak\ratpak.h"

#define HNUMOBJ   PRAT
typedef HNUMOBJ * PHNUMOBJ;


//
// Memory Alloc functions
//
#define NumObjAllocMem( cb )         zmalloc( cb )
#define NumObjFreeMem( h )           zfree( h ),(h=NULL)

//
// Unary functions
//

void NumObjInvert( PHNUMOBJ phno );

#define NumObjNegate( phno )                ( ((PRAT)*phno)->pp->sign= -(((PRAT)*phno)->pp->sign) )
#define NumObjAbs( phno )                   ( ((PRAT)*phno)->pp->sign=1, ((PRAT)*phno)->pq->sign=1 )

extern void NumObjSin( PHNUMOBJ phno );
extern void NumObjCos( PHNUMOBJ phno );
extern void NumObjTan( PHNUMOBJ phno );
extern void NumObjAntiLog10( PHNUMOBJ phno );

extern void NumObjNot( PHNUMOBJ phno );

//
// Comparison functions
//
#define NumObjIsZero( hno )                 zerrat( hno )
#define NumObjIsLess( hno1, hno2 )          rat_lt( hno1, hno2 )
#define NumObjIsLessEq( hno1, hno2 )        rat_le( hno1, hno2 )
#define NumObjIsGreaterEq( hno1, hno2 )     rat_ge( hno1, hno2 )
#define NumObjIsEq( hno1, hno2 )            rat_equ(hno1, hno2 )

//
// Assignment operator.  ('=' in C language)
//
#define NumObjAssign( phnol, hnor )         if (1) { DUPRAT( (*phnol), hnor ); } else 


//
// Data type conversion functions
//
void NumObjSetIntValue( PHNUMOBJ phnol, LONG i );


//
//  NumObjMakeNumber
//
//      HNUMOBJ NumObjMakeNumber( LPTSTR psz );
//
//  Converts psz to a number and returns it.  Call NumObjDestroy()
//  when you are done using the returned NumObj.
//
#define     NumObjMakeNumber( fMantNeg, pszMant, fExpNeg, pszExp )      inrat( fMantNeg, pszMant, fExpNeg, pszExp )

//
//  NumObjGetSzValue
//
//      void NumObjGetSzValue( LPTSTR *ppszNum, HNUMOBJ hnoNum, INT nRadix, NUMOBJ_FMT gafmt );
//
//  Converts hnoNum to a string and places the pointer into *ppszNum.  If *ppszNum already points
//  to a string, then that string is freed.
//
//  NOTES:  *ppszNum must either be NULL or point to a string previously returned by this function!
//          If you wish to free the string without replacing it.  You MUST use the NumObjFreeMem() function!
//
void NumObjGetSzValue( LPTSTR *ppszNum, HNUMOBJ hnoNum, INT nRadix, NUMOBJ_FMT gafmt );

//
//  GetObjGetExp
//
//  returns an int that equals the exponent of the NumObj
//
#define NumObjGetExp( hno )         LOGRATRADIX(hno)

//
//  NumObjCvtEpsilonToZero
//
//  if the input is < 1*10^(-nPrecision), then it gets set to zero
//  useful for special cases in ln, log, and sin, and cos
//
#define NumObjCvtEpsilonToZero( phno )
//#define NumObjCvtEpsilonToZero( phno )  if ( NumObjGetExp( *phno ) <= -nPrecision ) { NumObjAssign( phno, HNO_ZERO );} else

//
//  NumObjAbortOperation( fAbort )
//
//  If called with fAbort==TRUE, it will cause RATPAK to abort the current calculation and to return
//  immeadiatly.
//
//  It MUST be called again with fAbort=FALSE after ratpak has aborted to reset ratpak.
//
#define NumObjAbortOperation( fAbort )  (fhalt=fAbort)
#define NumObjWasAborted()              (fhalt)

//
//  NumObjOK( hno )
//
//      returns TRUE if the HNUMOBJ is valid (ie created and initialized)
//
//  Used to check the HNUMOBJ returned from NumObjMakeNumber and NumObjCreate
//
#   define NumObjOK( hno )              ((hno) == NULL ? FALSE : TRUE)

//
//  NumObjDestroy( hno )
//
//      call this when you nolonger need the NumObj.  Failure to do so
//  will result in memory leaks.
//
#   define NumObjDestroy( phno )            destroyrat( (*(phno)) )

//
// DECLARE_HNUMOBJ( hno )
//
//  Use this macro when ever you want to declare a local variable.
//
#   define DECLARE_HNUMOBJ( hno )       HNUMOBJ hno = NULL

//
// Useful Constants.  These have to be recomputed after a base or precision change.
//
void BaseOrPrecisionChanged( void );

#define HNO_ZERO                rat_zero
#define HNO_ONE_OVER_TWO        rat_half
#define HNO_ONE                 rat_one
#define HNO_TWO                 rat_two
#define HNO_180_OVER_PI         rad_to_deg
#define HNO_200_OVER_PI         rad_to_grad
#define HNO_2PI                 two_pi
#define HNO_PI                  pi
#define HNO_PI_OVER_TWO         pi_over_two
#define HNO_THREE_PI_OVER_TWO   one_pt_five_pi
