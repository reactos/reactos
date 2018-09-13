#ifdef __cplusplus
extern "C" {
#endif

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <ntdbg.h>

//
// Convert between signed and unsigned
//

// LARGE_INTEGER
// RtlConvertULargeIntegerToSigned(ULARGE_INTEGER uli);
#define RtlConvertULargeIntegerToSigned(uli) (*(PLARGE_INTEGER)(&(uli)))


// ULARGE_INTEGER
// RtlConvertLargeIntegerToUnsigned(LARGE_INTEGER li);
#define RtlConvertLargeIntegerToUnsigned(li) (*((PULARGE_INTEGER)(&(li))))


//
// math routines for ULARGE_INTEGERs
//

ULARGE_INTEGER
RtlULargeIntegerNegate(ULARGE_INTEGER);


/*
 *  #define RtlULargeIntegerNegate(uli) RtlConvertLargeIntegerToUnsigned( \
 *                                      RtlLargeIntegerNegate(         \
 *                                        RtlConvertULargeIntegerToSigned(uli)))
**/

BOOLEAN
RtlULargeIntegerEqualToZero(ULARGE_INTEGER);
/* #define RtlULargeIntegerEqualToZero(uli) RtlConvertLargeIntegerToUnsigned( \
 *                                            RtlLargeIntegerEqualToZero(    \
 *                                             RtlConvertULargeIntegerToSigned(uli)))
**/


//
// from strtoli.c
//

LARGE_INTEGER
strtoli ( const char *, char **, int );

ULARGE_INTEGER
strtouli ( const char *, char **, int );



//
// Large integer and - 64-bite & 64-bits -> 64-bits.
//

#define RtlLargeIntegerOr(Result, Source, Mask)   \
        {                                           \
            Result.HighPart = Source.HighPart | Mask.HighPart; \
            Result.LowPart = Source.LowPart | Mask.LowPart; \
        }




//
// Arithmetic right shift (the one in ntrtl.h is logical)
//

LARGE_INTEGER
RtlLargeIntegerArithmeticShiftRight(LARGE_INTEGER, CCHAR);


//
// bit-wise negation
//
#define RtlLargeIntegerBitwiseNot(li)   \
         li.LowPart  = ~li.LowPart;     \
         li.HighPart = ~li.HighPart;

#ifdef __cplusplus
} // extern "C" {
#endif

