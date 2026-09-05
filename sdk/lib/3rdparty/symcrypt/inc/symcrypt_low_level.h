//
// SymCrypt_low_level.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

//=======================================================================================
// WARNING: The low-level APIs are not stable, and can change from release to release.
// The low-level APIs are only provided for certain exceptional use cases.
// All aspects of the low-level API can change in any release.
// Users are strongly advised to only rely on the API surface defined in symcrypt.h
//=======================================================================================


//
// Low level asymmetric algorithm API. This is not to be used by external callers.
//

/**************************************************************************************************
    Low-level Integer API
 **************************************************************************************************
The low-level API allows manipulation of arbitrarily large integers.

The internal representation of large integers is not fixed. It depends on CPU architecture and
on the CPU features available on the exact CPU stepping the current software is running on.
In other words, it can change between different executions of the same binary.
Therefore it is critical that callers refrain from making assumptions about the internal
data format used. SymCrypt numbers should only be manipulated through the SymCrypt
API.

The low-level API allows the caller to allocate the necessary memory for all objects.
This is typically necessary for high-IRQL level callers, callers running in low-memory
environments, and high-performance scenarios where memory has to be pre-allocated.
SymCrypt also provides routines for allocating objects, which makes the API easier
to use. The caller has to provide the allocation functions that SymCrypt uses.

Internal data representation, and consequently the size of objects, can depend on the
exact CPU stepping the code is running on.
For robustness, the allocation size requirements are compile-time properties;
they vary per CPU architecture but do not depend on the exact available CPU features.

General rules:
The functions in the low-level API can impose requirements on their inputs. It is imperative that
these requirements are satisfied for every call; failing to satisfy the requirements leads to undefined
behaviour, including bugchecks, access violations, wrong results, or sometimes even the right result.
CHKed versions of the library add more low-level consistency checks; all binaries should be tested
with a CHKed version of the library to detect any errors that might go unnoticed on FRE versions.

Scratch space:
Many functions in the API require temporary storage for intermediate results.
Some function simply allocate the necessary memory using the caller-provided allocation routines.
Other low-level functions are so fast that the overhead of the allocations would significantly slow
down the computations. These functions require the caller to allocate the memory; this memory
is called the scratch space.

For each function that requires scratch space, there is a macro that determines how much scratch space
must be provided. This macro is a compile-time function of its arguments; if the parameters to the macro
are compile-time constants, then the result is also a compile-time constant. Therefore that
the macro can be used for statically sizing arrays.
The scratch space macros are all non-decreasing in each argument.
Callers that perform multiple operations can use a single scratch space, sized for the largest
argument(s) used. Note that the SYMCRYPT_C_MAX macro implements a compile-time MAX function suitable
for combining different scratch space sizes at compile time.

The scratch space is always passed as a pair or arguments: (pbScratch, cbScratch).
cbScratch needs to be at least as large as the macro definition requires, but may be larger.

Functions that take scratch parameters do not require memory allocation, and will not fail due to
low-memory conditions.
All functions that use memory allocation will return an error indication if a necessary memory
allocation fails. These functions return an error code, or an object pointer which will be NULL if
the allocation fails.
Functions that do not return an error code or object pointer do not use memory allocation.

SymCrypt uses several implementation techniques to minimize the cost of the scratch space parameters.
This is necessary because the cost of the parameter passing by itself is significant in scenarios
such as elliptic-curve operations.
In a FRE build, some functions will ignore the cbScratch parameter and simply assume they get enough space;
in this case the SymCrypt may provide an inline-able function that allow the compiler to optimize the cbScratch parameter
away, completely removing it from the actual code.
In environments where some functions don't need any scratch space, similar optimizations are possible for the
pbScratch parameter.

Scratch and object buffers must all be aligned to SYMCRYPT_ALIGN.

*/

//
// General flags
//
// SYMCRYPT_FLAG_DATA_PUBLIC is used to signal that the data being processed is public, and does not have
// to be protected from side-channel attacks.
#define SYMCRYPT_FLAG_DATA_PUBLIC   (0x01)

/*
INTEGERS

Integers are internally represented as a sequence of Digits. An INT object with n digits can store
numbers up to (but not including) R^n where R is the _radix_ of the representation.

The radix R, as well as the size and format of a Digit, are internal to the library,
and can depend on CPU architecture, CPU stepping and other run-time decisions. Therefore, callers
need to be especially careful not to make any assumptions about the size of a digit, or the number
of digits needed for any particular computation.

At the same time, most INT operations are defined in terms of Digit sizes, so the caller has to
be aware of digits. This becomes important in the following example. Suppose the radix R =2^256,
and a caller wants to multiply two 384-bit numbers. It takes 2 digits to store a 384-bit number.
The caller knows that the product is 768 bits, which can fit in 3 digits. So the caller might try
to multiply two 2-digit numbers into a 3-digit result, which will not work as the result is 4 digits.

For an INT object of n digits we call the value R^n the capacity of the object. It is the upper bound of
the values that can be stored in the object.

Additionally, there is a maximum number of bits for any integer value that the library supports (2^20 bits
in the current version). This bound is used to ensure that no object sizes and scratch space computations
have a value of magnitude more than 32 bits. Note that the computed upper bounds are very loose and the
actual values are much smaller.

Attempts to create objects larger than this bound will result in NULL being returned. Callers either have
to ensure they do not exceed the bounds, or check that create objects are not NULL before using them. The
rationale behind this approach is to avoid any potential route for malicious inputs to trigger DoS by
taking excessive CPU time which would be indistinguishable from an application hang.


Digit size and radix can vary widely; on some CPU steppings the library might use a digit that contains
128 bits are requires 16 bytes of memory, on another CPU stepping it might use a digit that contains
416 bits and uses 64 bytes of memory.

SAL annotations:
Because the different run-time selected implementations underneath this API might use
different size memory buffers for any one operation, fully accurate SAL annotations are not possible as SAL only
performs static analysis.
Furthermore, adding size parameters to every function would add too much overhead, and sizes are often passed
implicitly. Together with the fact that the same API can be implemented by different implementations, this means
that it isn't possible to write the actual size used in a form that SAL can understand.
Instead we use the following conventions:
-   Pointers to SYMCRYPT_* objects can only be created with functions that provide the right memory buffer size.
-   We annotate each object-pointer with _In_ our _Out_. The SAL engine treats this as just a read/write to
    a single object at the pointer location
-   The CHKed version of SymCrypt adds run-time checking that the various size parameters are correct.
This allows us to have both high performance and good checking of our memory management.

API rationale:
One important choice in this API is whether to pass a (ptr,len) for each INT or just a pointer.
We investigated this issue. The ptr-based API means that there are fewer parameters to pass around,
and generally makes the API simpler. The downside of a ptr-based API is that each INT object has some overhead
and this makes arrays of large integers less efficient, especially since the overhead can be a whole alignment
block.
The problem with the (ptr,len) format is that it isn't clear what length measure to use.
Using the bitsize is inefficient; the internal format might store 29 bits of the number in each 32-bit word,
and that means that the code would have to divide the bitsize by 29 just to find the size of the number.
Division is slow, and therefore this is not a good choice.
Another idea is to have the len parameter be the length of the INT object, in bytes.
But some APIs get really messy. For example, we need an API function to do a multiplication of two same-sized
numbers into a double-sized number. This is such a common operation that we want a separate function for it.
But the storage size of the result might not be twice the storage size of the inputs; if each number has some
fixed overhead then the output object might be smaller than the two times the size of the input objects.
This makes it impossible to write suitable SAL annotations.
For this reason, we use a ptr-based API for integers.

Most crypto algorithms that wish to store arrays of values actually want to store arrays of elements in a
ring modulo an modulus. And for modular operations the caller is already passing the modulus separately, so
there isn't any need to store per-object size information. The API is designed to allow the ModElement for bitsize
B to be smaller than an INT for bitsize B so that implementations can choose to not store any length information in
a ModElement object.
*/

//========================================================================
//========================================================================
// Main schema for object creation, deletion, and management (low - level calls).
//
// The following are descriptions of some of the generic functions specifically
// modified for the INT, DIVISOR, MODULUS, and MODELEMENT objects.

//
// PSYMCRYPT_XXX
// SYMCRYPT_CALL
// SymCryptXxxCreate(
//      _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
//                                      SIZE_T  cbBuffer,
//                                      UINT32  nDigits );
//  Create an XXX object from the provided (pbBuffer, cbBuffer) space.
//  The object will be able to store values up to R^nDigits where R is the digit radix.
//  Requirement:
//      - 1 <= nDigits <= SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
//          If the value is outside these bounds it will return NULL
//      - cbBuffer >= SymCryptSizeofXxxFromDigits( nDigits )
//      - (pbBuffer,cbBuffer) memory must be exclusively used by this object.
//  The last requirement ensures that all objects are non-overlapping (except for API functions
//  that explicitly create overlapping objects).
//  All parameters are published.
//  It is always safe to choose
//      cbBuffer = SYMCRYPT_SIZEOF_XXX_FROM_BITS( nBits )
//      nDigits  = SymCryptDigitsFromBits( nBits )
//  if the caller wants to be able to store numbers up to 2^nBits. However, it is frequently more
//  efficient to use cbBuffer = SymCryptSizeofXxxFromDigits( nDigits ) as that gives the exact size for the
//  current CPU stepping rather than the compile-time largest size that might be needed on any stepping.
//
// PSYMCRYPT_XXX
// SYMCRYPT_CALL
// SymCryptXxxRetrieveHandle( _In_  PBYTE   pbBuffer );
//  Retrieve the object's handle from the pointer to the memory space in which the object was created via
//  a call to SymCryptXxxCreate. This function allows callers to tightly store arrays of objects without having
//  to keep track of each object handle.
//  Requirement:
//      - A call to SymCryptXxxRetrieveHandle( pbBuffer1 ) must be preceded by at least one call to
//        SymCryptXxxCreate( pbBuffer2, cbBuffer2, nDigits ) with ( pbBuffer1 == pbBuffer2 )
//  If the requirement is not satisfied the result is undefined.
//
// #define SYMCRYPT_SIZEOF_XXX_FROM_BITS( nBits ) ...
//  Returns a memory size that is always sufficient to create an XXX object that can handle
//  values of size nBits bits, irrespective of the run-time decision of digit size.
//  This is a non-decreasing compile-time function of its inputs, suitable for computing static memory allocations.
//  It is always true that
//    SYMCRYPT_SIZEOF_XXX_FROM_BITS( n ) >= SymCryptSizeofXxxFromDigits( SymCryptDigitsFromBits( n ) )
//  which guarantees that the n-bit XXX can be stored in a memory area of SYMCRYPT_SIZEOF_XXX_FROM_BITS(n) bytes.
//  Warning: It is possible that
//    SYMCRYPT_SIZEOF_XXX_FROM_BITS( n+m ) < SymCryptSizeofXxxFromDigits( SymCryptDigitsFromBits( n ) + SymCryptDigitsFromBits( m ) )
//  for some inputs n and m. This is easy to see if you choose n = m = 1; each represents a 1-digit value, but an n+m bit (i.e. a 2-bit ) value is
//  also 1 digit.
//  In particular, you cannot use SYMCRYPT_SIZEOF_XXX_FROM_BITS( n + m ) to compute the size
//  necessary to store the product of two numbers with bitsize n and m respectively.
//  It is guaranteed that
//    SymCryptSizeofXxxFromDigits( SymCryptDigitsFromBits( n ) + SymCryptDigitsFromBits( m ) ) <=
//      SYMCRYPT_SIZEOF_XXX_FROM_BITS( n ) + SYMCRYPT_SIZEOF_XXX_FROM_BITS( m )
//  This is the proper way to statically compute the size needed to store the product of an n- and m-bit value.
//
// UINT32
// SYMCRYPT_CALL
// SymCryptSizeofXxxFromDigits( UINT32 nDigits );
//  Memory size that is sufficient to store an XXX object with nDigits digits.
//  This is a runtime function as the # digits and size of a digit are run-time decision that depend on the CPU stepping.
//  Requirement:
//      - 1 <= nDigits <= SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
//          If the value is outside these bounds the returned value will be 0 indicating failure.
//  This function is has the following property:
//    SymCryptSizeofXxxFromDigits( a + b ) <= SymCryptSizeofXxxFromDigits( a ) + SymCryptSizeofXxxFromDigits( b )
//  for all a and b.
//
// UINT32
// SYMCRYPT_CALL
// SymCryptXxxBitsizeOfObject( PCSYMCRYPT_XXX pObj )
//  Return the number of bits of the object.
//
// UINT32
// SYMCRYPT_CALL
// SymCryptXxxDigitsizeOfObject( PCSYMCRYPT_XXX pObj )
//  Return the number of digits of the object.
//

//==============================================================================================
// Object types for low-level API
//
// SYMCRYPT_INT          integer in range 0..N for some N
// SYMCRYPT_DIVISOR      an integer > 0 that can be used to divide with.
// SYMCRYPT_MODULUS      a value M > 1 to use in modulo-M computations
// SYMCRYPT_MODELEMENT   An element in a modulo-M ring.
// SYMCRYPT_ECPOINT      A point on an elliptic curve.
//
// See symcrypt_internal.h for definitions.
//

//========================================================================
//========================================================================
// General functions for integers
//

UINT32
SymCryptDigitsFromBits( UINT32 nBits );
//
// Returns the # digits needed to store values (INT, DIVISOR, MODULUS, MODELEMENT)
// in the range 0..(2^nBits - 1).
//
// Remarks:
//  If nBits==0 the returned number is 1.
//
//  If nBits exceeds SYMCRYPT_INT_MAX_BITS the function will return 0 to indicate an object with
//  this many bits is not supported.
//
//  This is a run-time decision; the return value can depend on the exact CPU stepping
//  the program is running on, or run-time configurations.
//  For a and b in the range 0..SYMCRYPT_INT_MAX_BITS, it is always true that
//  SymCryptDigitsFromBits( a + b ) <= SymCryptDigitsFromBits( a ) + SymCryptDigitsFromBits( b )
//

//========================================================================
// INT objects
//

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntAllocate( UINT32 nDigits );

VOID
SYMCRYPT_CALL
SymCryptIntFree( _Out_ PSYMCRYPT_INT piObj );

#define SYMCRYPT_SIZEOF_INT_FROM_BITS( _bitsize )    SYMCRYPT_INTERNAL_SIZEOF_INT_FROM_BITS( _bitsize )

UINT32
SYMCRYPT_CALL
SymCryptSizeofIntFromDigits( UINT32 nDigits );

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

VOID
SYMCRYPT_CALL
SymCryptIntWipe( _Out_ PSYMCRYPT_INT piObj );

VOID
SYMCRYPT_CALL
SymCryptIntCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );        // **** Documentation lacking: requires same size

VOID
SYMCRYPT_CALL
SymCryptIntMaskedCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          mask );

VOID
SYMCRYPT_CALL
SymCryptIntConditionalCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          cond );

VOID
SYMCRYPT_CALL
SymCryptIntConditionalSwap(
    _Inout_ PSYMCRYPT_INT   piSrc1,
    _Inout_ PSYMCRYPT_INT   piSrc2,
            UINT32          cond );

UINT32
SYMCRYPT_CALL
SymCryptIntBitsizeOfObject( _In_ PCSYMCRYPT_INT piSrc );

UINT32
SYMCRYPT_CALL
SymCryptIntDigitsizeOfObject( _In_ PCSYMCRYPT_INT piSrc );

//========================================================================
// DIVISOR objects
//

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorAllocate( UINT32 nDigits );

VOID
SYMCRYPT_CALL
SymCryptDivisorFree( _Out_ PSYMCRYPT_DIVISOR pdObj );

#define SYMCRYPT_SIZEOF_DIVISOR_FROM_BITS( _bitsize )    SYMCRYPT_INTERNAL_SIZEOF_DIVISOR_FROM_BITS( _bitsize )

UINT32
SYMCRYPT_CALL
SymCryptSizeofDivisorFromDigits( UINT32 nDigits );

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

VOID
SYMCRYPT_CALL
SymCryptDivisorWipe( _Out_ PSYMCRYPT_DIVISOR pdObj );

VOID
SymCryptDivisorCopy(
    _In_    PCSYMCRYPT_DIVISOR  pdSrc,
    _Out_   PSYMCRYPT_DIVISOR   pdDst );

UINT32
SYMCRYPT_CALL
SymCryptDivisorDigitsizeOfObject( _In_ PCSYMCRYPT_DIVISOR pdSrc );

//========================================================================
// MODULUS objects
//

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptModulusAllocate( UINT32 nDigits );

VOID
SYMCRYPT_CALL
SymCryptModulusFree( _Out_ PSYMCRYPT_MODULUS pmObj );

#define SYMCRYPT_SIZEOF_MODULUS_FROM_BITS( _bitsize )    SYMCRYPT_INTERNAL_SIZEOF_MODULUS_FROM_BITS( _bitsize )

UINT32
SYMCRYPT_CALL
SymCryptSizeofModulusFromDigits( UINT32 nDigits );

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptModulusCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

VOID
SYMCRYPT_CALL
SymCryptModulusWipe( _Out_ PSYMCRYPT_MODULUS pmObj );

VOID
SymCryptModulusCopy(
    _In_    PCSYMCRYPT_MODULUS  pmSrc,
    _Out_   PSYMCRYPT_MODULUS   pmDst );

UINT32
SYMCRYPT_CALL
SymCryptModulusDigitsizeOfObject( _In_ PCSYMCRYPT_MODULUS pmSrc );

//========================================================================
// MODELEMENT objects are treated slightly differently because it does not store its own size.
// This allows a MODELEMENT to be more compact which makes large arrays of ModElements more efficient
// and avoids checking that ModElements have the same size.
// All operations require a modulus to be passed.
//

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptModElementAllocate( _In_ PCSYMCRYPT_MODULUS pmMod );

VOID
SYMCRYPT_CALL
SymCryptModElementFree(
    _In_    PCSYMCRYPT_MODULUS      pmMod,      // only used to determine the digit size of peObj.
    _Out_   PSYMCRYPT_MODELEMENT    peObj );

#define SYMCRYPT_SIZEOF_MODELEMENT_FROM_BITS( _bitsize )    SYMCRYPT_INTERNAL_SIZEOF_MODELEMENT_FROM_BITS( _bitsize )

UINT32
SYMCRYPT_CALL
SymCryptSizeofModElementFromModulus( PCSYMCRYPT_MODULUS pmMod );

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptModElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
    _In_                            PCSYMCRYPT_MODULUS  pmMod );

VOID
SYMCRYPT_CALL
SymCryptModElementWipe(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peDst );

VOID
SymCryptModElementCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst );

VOID
SymCryptModElementMaskedCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst,
            UINT32                  mask );

VOID
SymCryptModElementConditionalSwap(
     _In_       PCSYMCRYPT_MODULUS    pmMod,
     _Inout_    PSYMCRYPT_MODELEMENT  peData1,
     _Inout_    PSYMCRYPT_MODELEMENT  peData2,
     _In_       UINT32                cond );

//========================================================================
// ECURVE objects

BOOLEAN
SYMCRYPT_CALL
SymCryptEcurveBufferSizesFromParams(
    _In_    PCSYMCRYPT_ECURVE_PARAMS    pParams,
    _Out_   SIZE_T *                    pcbCurve,
    _Out_   SIZE_T *                    pcbScratch );
//
// This call computes the memory size necessary to create the ECURVE object described by pParams,
// including the amount of scratch space needed for the operation.
//
// Returns FALSE if the given parameters are deemed invalid.
//

PSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptEcurveCreate(
    _In_                                PSYMCRYPT_ECURVE_PARAMS pParams,
    _In_                                UINT32                  flags,
    _Out_writes_bytes_( cbCurve )       PBYTE                   pbCurve,
                                        SIZE_T                  cbCurve,
    _Out_writes_bytes_( cbScratch )     PBYTE                   pbScratch,
                                        SIZE_T                  cbScratch );
//
// Use caller-allocated memory to create an ECURVE object which
// is defined by the parameters in pParams.
//
// - pParams: parameters that define the curve
// - flags: Not used, must be zero.
// - pbCurve: caller-allocated memory region to hold the curve object
// - cbCurve: size of memory region to hold the curve object
// - pbScratch: caller-allocated memory region used as scratch space to create the curve
// - cbScratch: size of scratch space memory region
//
// Caller should use SymCryptSizeofEcurveBuffersFromParams to determine the necessary sizes for
// pbCurve and pbScratch. These buffers must be SYMCRYPT_ALIGNed.
//
// Future versions might use the flags to enable different features/tradeoffs.
// There are a number of interesting memory/speed/pre-computation cost trades that can be made.
// For example, pre-computing multiples of the distinguished point, or (parallel?) pre-computation
// of (r, rG) pairs for random r values.
//
// This function applies limited validation of the pParams. The validation is intended to eliminate
// the threat of denial-of-service when hostile parameters are presented. It does not ensure that
// the parameters make sense, define a proper curve, or that any elliptic-curve operations made on
// the curve built from these parameters will fail, succeed or provide any security.
// The only guarantee provided for invalid parameters is that all operations on this curve will
// not crash and will return in some reasonable amount of time.
//
// Returns NULL if the given memory regions are not large enough or the
// parameters are deemed invalid. If the return value is not NULL, then
// pbCurve buffer must later be wiped with SymCryptWipe(). And as with all
// pbScratch buffers, it is the caller's responsibility to wipe after
// completing all operations that require scratch space.
//

//========================================================================
// ECPOINT objects' API is slightly different than the above API schema in the sense that they
// take as input an ECURVE object pointer instead of the number of digits.
//

PSYMCRYPT_ECPOINT
SYMCRYPT_CALL
SymCryptEcpointAllocate( _In_ PCSYMCRYPT_ECURVE pCurve );

VOID
SYMCRYPT_CALL
SymCryptEcpointFree(
     _In_   PCSYMCRYPT_ECURVE    pCurve,
     _Out_  PSYMCRYPT_ECPOINT    poDst );

UINT32
SYMCRYPT_CALL
SymCryptSizeofEcpointFromCurve( PCSYMCRYPT_ECURVE pCurve );

PSYMCRYPT_ECPOINT
SYMCRYPT_CALL
SymCryptEcpointCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
    _In_                            PCSYMCRYPT_ECURVE   pCurve );
// The above can take as input a pointer to a curve that has only the FMod, cbModElement, and the
// eformat fields set

PSYMCRYPT_ECPOINT
SYMCRYPT_CALL
SymCryptEcpointRetrieveHandle( _In_  PBYTE   pbBuffer );

VOID
SYMCRYPT_CALL
SymCryptEcpointWipe(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst );

VOID
SymCryptEcpointCopy(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst );

VOID
SymCryptEcpointMaskedCopy(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              mask );


//========================================
// Integer operations
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntCopyMixedSize(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = Src, but allows Dst and Src to have different # digits.
//
// Copy the value from piSrc to piDst.
// Returns success if  Src < R^Dst.nDigits
// If Src >= R^Dst.nDigits then the value in Src is published and an error is returned.
// Warning: it is not side-channel safe to use this function with a Src value that can't fit in Dst.
// Src and Dst may be the same object.
//

UINT32
SYMCRYPT_CALL
SymCryptIntBitsizeOfValue( _In_ PCSYMCRYPT_INT piSrc );
//
// Returns the number of bits necessary to store the value of Src.
//
// Let V be the value of Src.
// Then this function returns
//  0 if Src == 0
//  1 + floor( log(Src)/log(2) )  if V > 0
// Note that there is no defined relationship between the result of this function and the bitsize used to allocate Src.
// Digits can be large, so the value Src might be able to store values much larger than 2^b where b is the bitsize
// used when creating Src.
// This function is side-channel safe, and as a result might be slower than expected.
//


VOID
SYMCRYPT_CALL
SymCryptIntSetValueUint32(
            UINT32          u32Src,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = Src
// This always succeeds as R >= 2^32 on all implementations.
//

VOID
SYMCRYPT_CALL
SymCryptIntSetValueUint64(
            UINT64          u64Src,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = Src
// This always succeeds as R >= 2^64 on all implementations.
//


//========================================================================================
// Read/write INTegers in defined formats
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntSetValue(
    _In_reads_bytes_(cbSrc)     PCBYTE                  pbSrc,
                                SIZE_T                  cbSrc,
                                SYMCRYPT_NUMBER_FORMAT  format,
    _Out_                       PSYMCRYPT_INT           piDst );
//
// Set the value of an INT object from an array of bytes
//
// (pbSrc,cbSrc): buffer that contains the bytes that encode the value in the specified format.
// format: specifies the format of the pbBytes/cbBytes buffer.
// Dst : INT object that receives the value; must previously have been created/allocated.
//
// Return value:
//   If the value encoded in the (pbSrc,cbSrc) buffer fits in Dst, then the
//   function succeeds. If the value does not fit, then the function
//   returns an error. Note that the error condition is only dependent on the value in the input,
//   and not on how many bytes are in the input. Importing a very large (pbSrc,cbSrc) buffer
//   into a small piDst is fine as long as the value fits in the number (i.e. enough of the most significant
//   bytes in the buffer are zero).
//
// Warning:
//  Error return values are always published, so if this function fails it is visible to the attacker.
//
// Rationale:
//   Because the size of a digit can be any size (even odd) there are always scenarios in which the
//   caller can provide an input that is too large for the INT to store. (Restricting only the size of
//   the input buffer is not sufficient.) And if we have to handle this
//   in one case, we might as well handle it in all cases.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntGetValue(
    _In_                        PCSYMCRYPT_INT          piSrc,
    _Out_writes_bytes_( cbDst)  PBYTE                   pbDst,
                                SIZE_T                  cbDst,
                                SYMCRYPT_NUMBER_FORMAT  format );
//
// Convert a value from the internal number representation to a byte array.
//
// Src is the number whose value is to be stored in a byte array
// (pbDst, cbDst) the destination buffer
// format: the destination format.
// Return value: if the value of Src when encoded in the format fits in the output buffer then the function succeeds.
// If the encoded value does not fit, the function returns an error. (Note: All errors are published.)
//

UINT32
SYMCRYPT_CALL
SymCryptIntGetValueLsbits32( _In_  PCSYMCRYPT_INT piSrc );
//
// Returns Src mod 2^32
//
// Usecase: there are many number-theoretic algorithms where the algorithm
// depends on (n mod 8) or similar values.
//

UINT64
SYMCRYPT_CALL
SymCryptIntGetValueLsbits64( _In_  PCSYMCRYPT_INT piSrc );
//
// Returns Src mod 2^64
//
// Usecase: RSA public exponents can be 64 bits, and validating that
// a candidate prime is suitable uses this function
//

UINT32
SYMCRYPT_CALL
SymCryptIntIsEqualUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    UINT32          u32Src2 );
//
// Returns a mask value which is 0xffffffff if Src1 = Src2 and 0 otherwise.
//

UINT32
SYMCRYPT_CALL
SymCryptIntIsEqual(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 );
//
// Returns a mask value which is 0xffffffff if Src1 = Src2 and 0 otherwise.
//
// Note that Src1 and Src2 can be of different sizes.
//

UINT32
SYMCRYPT_CALL
SymCryptIntIsLessThan(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 );
//
// Returns a mask value which is 0xffffffff if Src1 < Src2 and 0 otherwise.
//
// Note that a <= b is equivalent to NOT( b < a ) so all possible comparisons
// can be made using the < and = comparison primitive.
//


//=============================================================
// Addition & subtraction
// For all addition and subtraction operations, the destination may be
// the same object as one of the inputs if the other requirements of the function
// allow that.
//

UINT32
SYMCRYPT_CALL
SymCryptIntAddUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = Src1 + Src2.
// Requirement: Dst.nDigits == Src1.nDigits
// If the result is larger than the capacity of Dst, then
// Dst is set to the result minus the capacity and the value 1 is returned.
// Otherwise the Dst is set to the sum and the value 0 is returned.
// The return value is thus a carry output of the addition.
//

UINT32
SYMCRYPT_CALL
SymCryptIntAddSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );
//
// Dst = Src1 + Src2.
// Requirement: Src1.nDigits == Src2.nDigits == Dst.nDigits
// In more detail:
//  if Src1 + Src2 < Dst.capacity:
//      Dst = Src1 + Src2
//      return 0
//  else
//      Dst = Src1 + Src2 - Dst.capacity
//      return 1
// The return value is a carry output of the addition.
//
// Dst may be the same object as Src1, Src2, or both.
//

UINT32
SYMCRYPT_CALL
SymCryptIntAddMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );
//
// Dst = Src1 + Src2.
// Requirement: Dst.nDigits >= max( Src1.nDigits, Src2.nDigits )
// In more detail:
//  if Src1 + Src2 < Dst.capacity:
//      Dst = Src1 + Src2
//      return 0
//  else
//      Dst = Src1 + Src2 - Dst.capacity
//      return 1
// The return value is a carry output of the addition.
//
// Dst may be the same object as Src1, Src2, or both.
//

//
// Subtraction
// Subtraction functions are the equivalent of addition functions.
// The return value is 1 if an underflow occurred (borrow), and 0 if no underflow/borrow occurred.
// On underflow, the value of the result is the result of the subtraction plus Dst.capacity.
//
// Rationale: For an underflow we could also return (UINT32)-1 or return -1 on a INT32.
// -1 in an unsigned type is actually 2^32 -1 which makes no sense.
// Returning a signed type is somewhat neater, but all other values are unsigned, and mixing
// signed and unsigned types is always error-prone. Furthermore, converting from a signed integer
// to a mask is also error-prone (at least within the behaviour guaranteed by the C standard.)
// Returning an unsigned 1 is therefore preferred.
//

UINT32
SYMCRYPT_CALL
SymCryptIntSubUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          Src2,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptIntSubSameSize(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptIntSubMixedSize(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptIntNeg(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );

//
// Dst = (- Src) mod Dst.Capacity;
// Requirement:
//  - Dst.nDigits == Src.nDigits;
// This is a negate modulo the capacity.
// Useful when you want the absolute value of a difference.
// Compute the difference, and if the subtraction yields a carry, negate the result.
//

//===================================================================
// Shifts
// Note that the shift amount is always published.
// If the need arises, we can define variants that are side-channel safe
// w.r.t. the shift size, but that incurs a significant performance cost.
//

VOID
SYMCRYPT_CALL
SymCryptIntMulPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = (Src * 2^Exp ) mod R^n  where n = Dst.nDigits.
// Requirement: Dst.nDigits == Src.nDigits, Dst == Src is allowed
// Exp is published.
//
// A variant that keeps Exp private is currently not available, but can be added to the API if needed.
// (A side-channel safe variant might require scratch space.)
//
// Dst may be the same object as Src1.
//

VOID
SYMCRYPT_CALL
SymCryptIntDivPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = (Src div 2^Exp )
// Requirement: Dst.nDigits == Src.nDigits, Dst == Src is allowed
// Exp is published
//
// A variant that keeps Exp private is currently not available, but can be added to the API if needed.
// (A side-channel safe variant might require scratch space.)
//
// Dst may be the same object as Src1.
//

VOID
SYMCRYPT_CALL
SymCryptIntShr1(
            UINT32          highestBit,
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = (Src + highestBit * Src.Capacity) div 2
//
// Requirements:
//  Src.nDigits == Dst.nDigits
//  highestBit <= 1
//
// This is the Int equivalent of the 'shift right 1' instruction.
// Shifting by one can be implemented faster than variable sized shifts.
//

VOID
SYMCRYPT_CALL
SymCryptIntModPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst );
//
// Dst = Src mod 2^Exp
// Requirement: Dst.nDigits == Src.nDigits, Dst == Src is allowed
// Exp is published
//
// Dst may be the same object as Src1.
//

UINT32
SYMCRYPT_CALL
SymCryptIntGetBit(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit );
//
// Returns the i-th bit (starting from 0 for the LSB) of piSrc.
// Therefore the only possible return values are 0 and 1.
//
// Requirements:
//  - iBit < SymCryptIntBitsizeOfObject( piSrc )
//

UINT32
SYMCRYPT_CALL
SymCryptIntGetBits(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit,
            UINT32          nBits );
//
// Returns the bits from position iBit up to (iBit + nBits - 1)
// (starting from 0 for the LSB). Total of nBits. The 0-th bit of
// the return value corresponds to the iBit-th bit of the source.
//
// Requirements:
//  -   1 <= nBits <= 32
//  -   iBit + nBits <= SymCryptIntBitsizeOfObject( piSrc )
//
// Remarks:
//      - The values iBit and nBits are not protected by side-channel attacks,
//        therefore they should be treated as published.
//      - The bits of the return value after the (nBits)-th bit are zero.
//

VOID
SYMCRYPT_CALL
SymCryptIntSetBits(
    _In_    PSYMCRYPT_INT   piDst,
            UINT32          value,
            UINT32          iBit,
            UINT32          nBits );
//
// Sets the bits from position iBit up to (iBit + nBits - 1)
// (starting from 0 for the LSB). Total of nBits. The 0-th bit of
// the input value corresponds to the iBit-th bit of the destination.
//
// Requirements:
//  -   1 <= nBits <= 32
//  -   iBit + nBits <= SymCryptIntBitsizeOfObject( piSrc )
//
// Remarks:
//      - The values iBit and nBits are not protected by side-channel attacks,
//        therefore they should be treated as published.
//      - The bits of the value after the (nBits)-th bit are ignored.
//

//===========================================================
// Mul & div
//

UINT32
SYMCRYPT_CALL
SymCryptIntMulUint32(
    _In_                            PCSYMCRYPT_INT  piSrc1,
                                    UINT32          Src2,
    _Out_                           PSYMCRYPT_INT   piDst );
//
// Dst = Src1 * Src2 mod Dst.capacity; return value = Src1 * Src2 div Dst.capacity
// Requirement: piDst.nDigits == piSrc1.nDigits, Dst == Src is allowed
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( _nResultDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_MUL( _nResultDigits )

VOID
SYMCRYPT_CALL
SymCryptIntMulSameSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
//
// Dst = Src1 * Src2.
// Requirement:
//  - Src1.nDigits == Src2.nDigits; Dst.nDigits == Src1.nDigits + Src2.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( Dst.nDigits )
//
// Note that Dst cannot be the same object as Src1 or Src2 because of the size restrictions.
//

VOID
SYMCRYPT_CALL
SymCryptIntSquare(
    _In_                            PCSYMCRYPT_INT  piSrc,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
//
// Dst = Src^2
// Requirement:
//  - Dst.nDigits == 2 * Src.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( Dst.nDigits )
//
// Note that Dst cannot be the same object as Src1 or Src2 because of the size restrictions.
//

VOID
SYMCRYPT_CALL
SymCryptIntMulMixedSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
//
// Dst = Src1 * Src2.
// Requirement:
//  - Dst.nDigits >= Src1.nDigits + Src2.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_MUL( Dst.nDigits )
//
// Note that Dst cannot be the same object as Src1 or Src2 because of the size restrictions.
//


//
// Division
// For all division and modulo operations, there are pre-computations that have to be done
// on the divisor. The pre-computed divisor information is stored in a DIVISOR object.
// Note that the bitsize of the value of the divisor is published.
// Therefore, a generic division is not side-channel safe.
// Rationale: Hiding the bitsize of the value of the divisor is quite expensive,
// and we have no cryptographic algorithms that require it.
//


PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntFromDivisor( _In_ PSYMCRYPT_DIVISOR pdSrc );
//
// Returns the INT object inside the DIVISOR object.
// Digit size of the INT object is equal to the digit size of the DIVISOR object.
// This object has two uses:
// - On an uninitialized DIVISOR object it is a suitable place to put a value before calling
//   SymCryptIntToDivisor.
// - On an initialized DIVISOR object the function returns a pointer to the INT that contains
//   the divisor value. Modifying the INT value from an initialized DIVISOR value corrupts
//   the divisor value.
//
// This is typically a very fast function, with a run-time cost that is zero or only one instruction.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits )

VOID
SYMCRYPT_CALL
SymCryptIntToDivisor(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_DIVISOR   pdDst,
                                    UINT32              totalOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );
//
// Create a DIVISOR object from an INT.
// Requirement:
//  - Dst.nDigits == Src.nDigits
//  - Src != 0
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( Src.nDigits )
// SymCryptIntBitsizeOfValue( Src ) is published.
// Src may be equal to SymCryptIntFromDivisor( Dst ).
// totalOperations is an estimate of how many divide/modulo operations will be performed with this divisor.
//   An implementation may use this to decide how much pre-computations to do.
// flags: any combination of the following flag values:
//  - SYMCRYPT_FLAG_DATA_PUBLIC
//      Signals that the Src value is public.
//      Implementations can use this to use more efficient divisor algorithms depending on the actual value of Src.
//      For example, if Src is very close to a power of 2, division can be implemented more efficiently.
//
// Once a divisor object has been created, it is immutable.
// Multiple threads can use the same divisor object for different division operations in parallel.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( _nSrcDigits, _nDivisorDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_DIVMOD( _nSrcDigits, _nDivisorDigits )

VOID
SYMCRYPT_CALL
SymCryptIntDivMod(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _In_                            PCSYMCRYPT_DIVISOR  pdDivisor,
    _Out_opt_                       PSYMCRYPT_INT       piQuotient,
    _Out_opt_                       PSYMCRYPT_INT       piRemainder,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );
//
// Quotient = Src div Divisor
// Remainder = Src mod Divisor
// Quotient & Remainder may be NULL in which case that result is not returned.
// Requirements:
//  - Quotient.nDigits >= Src.nDigits
//  - Remainder.nDigits >= Divisor.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_DIVMOD( Src.nDigits, Divisor.nDigits )
// Quotient and Remainder must be different objects.
// Src may be the same object as either Quotient or Remainder.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits  )  SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits )

VOID
SYMCRYPT_CALL
SymCryptIntExtendedGcd(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
                                    UINT32          flags,
    _Out_opt_                       PSYMCRYPT_INT   piGcd,
    _Out_opt_                       PSYMCRYPT_INT   piLcm,
    _Out_opt_                       PSYMCRYPT_INT   piInvSrc1ModSrc2,
    _Out_opt_                       PSYMCRYPT_INT   piInvSrc2ModSrc1,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
//
// Compute up to four results from Src1 and Src2.
//   GCD is the greatest common divisor of Src1 and Src2.
//   LCM is the Least Common Multiple of Src1 and Src2.
//   InvSrc1ModSrc2 is the smallest value such that (InvSrc1ModSrc2 * Src1) mod Src2= GCD( Src1, Src2 )
//      UNLESS Src1 is a multiple of Src2, i.e. when Src1 = 0 mod Src2. In this case the result is
//      undefined.
//   InvSrc2ModSrc1 is the smallest value such that (InvSrc2ModSrc1 * Src2) mod Src1= GCD( Src1, Src2 )
//      UNLESS Src2 is a multiple of Src1, i.e. when Src2 = 0 mod Src1. In this case the result is
//      undefined.
//
// The last two modular inverse values are not true modular inverses unless GCD( Src1, Src2 ) = 1.
//
// Any of the output pointers can be NULL and then that result is not returned.
// Requirements:
//  - Src1 > 0
//  - Src2 > 0 and  Src2 odd
//  - Gcd.nDigits >= min( Src1.nDigits, Src2.nDigits )
//  - Lcm.nDigits >= Src1.nDigits + Src2.nDigits
//  - InvSrc1ModSrc2.nDigits >= max(Src1.nDigits, Src2.nDigits)     // Future work: Make these bounds Src2 and Src1 respectively.
//  - InvSrc2ModSrc1.nDigits >= max(Src1.nDigits, Src2.nDigits)
//  - if piInvSrc2ModSrc1 is not NULL, max( Src1.nDigits, Src2.nDigits ) * 2 <= SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_EXTENDED_GCD( max( Src1.nDigits, Src2.nDigits ) )
//
// If only one inverse value is needed, it is most efficient to use only InvSrc1ModSrc2.
//
// The restriction that Src2 must be odd can be removed in a future version.
// The SYMCRYPT_FLAG_DATA_PUBLIC flag signals that the inputs are public information and do not have
// to be side-channel protected.
// The SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN signals that at least one input is odd. This speeds up the
// side-channel safe implementation; this flag is not needed if the inputs are signaled as public as the code can then
// afford to check that condition and change use a optimized algorithm.
// The SYMCRYPT_FLAG_GCD_PUBLIC signals that the GCD value is public. This can make some computations
// (of the inverses) more efficient when GCD = 1.
//

#define SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN      (0x02)
#define SYMCRYPT_FLAG_GCD_PUBLIC                    (0x04)


UINT64
SYMCRYPT_CALL
SymCryptUint64Gcd( UINT64 a, UINT64 b, UINT32 flags );
//
// Return GCD of two 64-bit integers.
//  a, b : inputs to the GCD
//  flags:
//      - SYMCRYPT_FLAG_DATA_PUBLIC signals that a and b are public values (w.r.t. side-channel safety)
//          This may improve performance.
//      - SYMCRYPT_FLAG_GCD_INPUTS_NOT_BOTH_EVEN: signals that at least one of (a,b) is odd. This
//          simplifies & speeds up the GCD computation.
//
// Note:
// The current implementation requires that the INPUTS_NOT_BOTH_EVEN flag is set (and at least one input be odd).
// Also note that GCD(x, 0) == GCD(0, x) == x
//


#define SYMCRYPT_SCRATCH_BYTES_FOR_CRT_GENERATION( _nDigits )  SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_CRT_GENERATION( _nDigits )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCrtGenerateInverses(
                                    UINT32                  nCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODULUS *    ppmCoprimes,
                                    UINT32                  flags,
    _Out_writes_( nCoprimes )       PSYMCRYPT_MODELEMENT *  ppeCrtInverses,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Compute the Chinese Remainder Theorem (CRT) constants for a set of nCoprimes
// pairwise coprime moduli. Pointers to the input numbers are stored in the array of
// pointers ppmCoprimes, while the outputs are stored to the locations pointed by
// ppeCrtInverses.
//
// For input numbers Src1, Src2, ..., SrcK where K = nCoprimes, let N = Src1*Src2*...*SrcK.
// Then this function outputs the constants:
//      (( Src1 / N ) mod Src1), (( Src2 / N ) mod Src2), ..., (( SrcK / N ) mod SrcK)
//
// The most common case is for the RSA algorithm where the inputs are 2 prime numbers P and Q
// and only Q^{-1} mod P is needed (i.e. only the first term of the output array).
//
// Any of the output pointers in the ppeCrtInverses can be NULL and then that result
// is not returned (resulting in a faster total running time).
//
// The number of inputs nCoprimes and which outputs are returned is public.
//
// Requirements:
//  - nCoprimes >= 2
//  - Both ppmCoprimes and ppeCrtInverses must be arrays of pointers of exactly nCoprimes pointers.
//  - ppmCoprimes[i] != NULL for all i in [0, nCoprimes-1].
//  - The input moduli must be pairwise coprime.
//  - The number of digits of all input moduli must match the number of digits of the corresponding
//    output modelements.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_CRT_GENERATION( nDigits ) where nDigits is the maximum number
//    of digits of the inputs and outputs.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_CRT_SOLUTION( _nDigits )  SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_CRT_SOLUTION( _nDigits )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCrtSolve(
                                    UINT32                  nCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODULUS *    ppmCoprimes,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODELEMENT * ppeCrtInverses,
    _In_reads_( nCoprimes )         PCSYMCRYPT_MODELEMENT * ppeCrtRemainders,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_INT           piSolution,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Solve for x the system of nCoprimes congruences of the form
//      x = ppeCrtRemainders[0] (mod ppmCoprimes[0])
//      x = ppeCrtRemainders[1] (mod ppmCoprimes[1])
//      ...
//      x = ppeCrtRemainders[nCoprimes-1] (mod ppmCoprimes[nCoprimes-1])
//
// The input array ppeCrtInverses must have been pre-computed by a call to SymCryptCrtGenerateInverses.
//
// The number of inputs nCoprimes is public.
//
// Requirements:
//  - nCoprimes == 2
//  - ppmCoprimes, ppeCrtInverses, and ppeCrtRemainders must be arrays of pointers of exactly nCoprimes elements. All
//    of them non-NULL.
//  - piSolution must be large enough to hold the result modulo the product of all the coprimes.
//  - max( ppmCoprimes[0].nDigits, ppmCoprimes[1].nDigits ) * 2 <= SymCryptDigitsFromBits(SYMCRYPT_INT_MAX_BITS)
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_CRT_SOLUTION( nDigits ) where nDigits is the maximum number
//    of digits of the input moduli.
//


typedef const struct _SYMCRYPT_TRIALDIVISION_CONTEXT *PCSYMCRYPT_TRIALDIVISION_CONTEXT;

PCSYMCRYPT_TRIALDIVISION_CONTEXT
SYMCRYPT_CALL
SymCryptCreateTrialDivisionContext( UINT32 nDigits );
//
// Create a trial division context that can be used for integers up to and including nDigits digits.
// The Trial division context can be used in multiple threads in parallel.
// The context should be freed with SymCryptFreeTrialDivisionContext after use.
// A context can be fairly large (100 kB) so freeing it is important.
// Returns NULL if out of memory or an invalid digit count is provided.
//

VOID
SYMCRYPT_CALL
SymCryptFreeTrialDivisionContext( PCSYMCRYPT_TRIALDIVISION_CONTEXT pContext );
//
// Free the trial division context after use.
//

UINT32
SYMCRYPT_CALL
SymCryptIntFindSmallDivisor(
    _In_                            PCSYMCRYPT_TRIALDIVISION_CONTEXT    pContext,
    _In_                            PCSYMCRYPT_INT                      piSrc,
    _Out_writes_bytes_( cbScratch ) PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch );
//
// Returns a divisor of piSrc, or zero.
// Requirements:
// Requirement:
//  - pContext is a valid trial division context, and Context.nDigits >= Src.nDigits
//  - Src >= 2
// Note:
//  - Src is published if this function returns a divisor.
//
// There is no guarantee that this function finds small divisors;
// it is valid for the implementation to always return 0.
// Any nonzero return value is always >= 2 and an actual divisor of Src.
// Note: this function might not find 2 as a small divisor.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_IS_PRIME( _nDigits )

UINT32
SYMCRYPT_CALL
SymCryptIntMillerRabinPrimalityTest(
    _In_                            PCSYMCRYPT_INT      piSrc,
                                    UINT32              nBitsSrc,
                                    UINT32              nIterations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );
//
// Applies the Miller-Rabin prime testing algorithm using nIterations on the integer
// piSrc.
//
// The maximum bitsize of the value of piSrc is equal to nBitsSrc and it is public.
// The number of iterations nIterations is also public.
//
// If the return value is 0, then Src is guaranteed to be a composite value.
//      In this case, the value of Src is treated as public.
//
// If the return value is 0xffffffff, then Src might be prime.
//      In this case, the value of Src is treated as private except when the
//      SYMCRYPT_FLAG_DATA_PUBLIC flag is specified.
//
//      If the flag SYMCRYPT_FLAG_DATA_PUBLIC is specified the
//      algorithm leaks the number of trailing zeros of Src-1. The reason for
//      not having a fully side-channel safe implementation for arbitrary
//      numbers is that such a function would be prohibitively slow.
//
// Requirements:
//  - SymCryptIntBitsizeOfValue( piSrc ) <= nBitsSrc <= SymCryptIntBitsizeOfObject( piSrc )
//  - Src is odd and greater than 3.
//  - If flags == 0 then Src must be 3 modulo 4. (See the comment above for
//    the SYMCRYPT_FLAG_DATA_PUBLIC flag)
//  - cbScratch >=  SYMCRYPT_SCRATCH_BYTES_FOR_INT_IS_PRIME( Src.nDigits )
//

//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_PRIME_GEN( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_PRIME_GEN( _nDigits )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntGenerateRandomPrime(
    _In_                            PCSYMCRYPT_INT      piLow,
    _In_                            PCSYMCRYPT_INT      piHigh,
    _In_reads_opt_( nPubExp )       PCUINT64            pu64PubExp,
                                    UINT32              nPubExp,
                                    UINT32              nTries,
                                    UINT32              flags,
    _Inout_                         PSYMCRYPT_INT       piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );
//
// This function generates a random prime Dst such that
//          Dst == 3 mod 4      and
//          Low <= Dst < High
//          for e in PubExp[]: GCD( Dst - 1, e ) == 1
//
// (pu64PubExp, nPubExp) can be (NULL, 0) if no pubexp restriction is needed.
// The nTries parameter specifies the maximum number of candidate numbers
// until a prime number is found satisfying the above restrictions.
// If the function cannot find one after nTries, it returns SYMCRYPT_INVALID_ARGUMENT
// (For example, if the caller passes in a Low bound bigger than the High bound,
// or if there are no primes between Low and High).
//
// The values of the pubexps, piLow and piHigh are public.
//
// flags: None
//
// Requirements:
//  - SymCryptIntBitsizeOfValue( piHigh ) <= SymCryptIntBitsizeOfObject(piDst)
//  - piLow > 3
//  - Each public exponent must be greater than 0
//  - 0 <= nPubExp <= SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS
//  - cbScratch >=  SYMCRYPT_SCRATCH_BYTES_FOR_INT_PRIME_GEN( Dst.nDigits )
//

//=====================================================
// Modular arithmetic
//
// To perform modular arithmetic the modulus has to be prepared into a Modulus object.
// Arithmetic in the ring modulo the modulus can then be done using ModElement objects.
//

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc );
//
// Returns the DIVISOR object inside the MODULUS object.
//
// Digit size of the DIVISOR object is equal to the digit size of the MODULUS object.
// This object has one use:
// - On an initialized MODULUS object the function returns a pointer to the DIVISOR that contains
//   the modulus value. Modifying the DIVISOR value from an initialized MODULUS value corrupts
//   the modulus.
//
// This is typically a very fast function, with a run-time cost that is zero or one instruction.
//

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc );
//
// Returns the INT object inside the MODULUS object.
//
// Digit size of the INT object is equal to the digit size of the MODULUS object.
// This object has two uses:
// - On an uninitialized MODULUS object it is a suitable place to put a value before calling
//   SymCryptIntToModulus.
// - On an initialized MODULUS object the function returns a pointer to the INT that contains
//   the modulus value. Modifying the INT value from an initialized MODULUS value corrupts
//   the modulus.
//
// This is typically a very fast function, with a run-time cost that is zero or one instruction.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _nDigits )

VOID
SYMCRYPT_CALL
SymCryptIntToModulus(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_MODULUS   pmDst,
                                    UINT32              averageOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );
//
// Create a modulus from an INT.
// Requirements:
//  - Src != 0
//  - Dst.nDigits == Src.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_INT_TO_MODULUS( Src.nDigits )
// SymCryptIntBitsizeOfValue( Src ) is published.
// averageOperations is the average number of multiplications that are performed on a ModElement created with this modulus between the time that the value is
//      created as a ModElement and the time it is exported out of modElement form.
//      There are multiple ways of doing modular computations; some of them are faster but have an overhead for converting into and out of modular form.
//      For example, for RSA verification the # operations is small and conversion overhead should be avoided.
//      For RSA signatures, the # operations is large and the fastest per-operation form should be used.
//      This parameter allows the library to select the right kind of modular arithmetic for this modulus.
// The following flags are supported:
//  SYMCRYPT_FLAG_DATA_PUBLIC
//      Signals the code that the Src value is public. This may improve performance because it allows further optimizations that
//      depend on the value. (For example, if Src is close to a power of 2, the modulo reduction can be made significantly faster.)
//  SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC
//      Signals that the parity of Src (whether it is even or odd) may be treated as a public value.
//      There are some algorithms that can speed up operations on odd moduli, but their use publishes the fact that the modulus is odd.
//  SYMCRYPT_FLAG_MODULUS_ADDITIVE_ONLY
//      The modulus will only be used for addition and subtraction, not for multiplication or division.
//      This can significantly reduce the cost of this function as there is no need to pre-compute the divisor information.
//  SYMCRYPT_FLAG_MODULUS_PRIME
//      Signals that the modulus is a prime. Some algorithms can be more efficient for prime moduli. Note that setting this flag
//      for a non-prime modulus can result in incorrect answers.
// The flags and averageOperations parameters are published.
//

#define SYMCRYPT_FLAG_MODULUS_PARITY_PUBLIC     (0x02)
#define SYMCRYPT_FLAG_MODULUS_ADDITIVE_ONLY     (0x04)
#define SYMCRYPT_FLAG_MODULUS_PRIME             (0x08)

#define SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nDigits )


VOID
SYMCRYPT_CALL
SymCryptIntToModElement(
    _In_                            PCSYMCRYPT_INT          piSrc,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src mod Mod
// Requirements:
//  - Dst.nDigits == Mod.nDigits
//  - piSrc.nDigits <= 2 * Mod.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// Note: the input is limited in size to be no more than twice the modulus size (in digits).
// This should be a rare case, and it simplifies the scratch space handling significantly.
//

VOID
SYMCRYPT_CALL
SymCryptModElementToInt(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_INT           piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src
//
// Requirement:
//  - Dst.nDigits >= Mod.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// Convert a ModElement to an Int.
// The internal format in which a ModElement is stored might be different
// from the format of an Int; this function converts the value to the INT format.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModElementSetValue(
    _In_reads_bytes_( cbSrc )       PCBYTE                  pbSrc,
                                    SIZE_T                  cbSrc,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = decode( pbSrc, cbSrc, format ) mod Mod
// Requirement:
//  - SymCryptDigitsFromBits( 8 * cbSrc ) <= Mod.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// This is a separate function as it is frequently used, and does not require the allocation of an INT object.
//

VOID
SYMCRYPT_CALL
SymCryptModElementSetValueUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = value mod Mod
// value is published.
// Requirement:
//  - value < Mod
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
// Note: this function does NOT hide the value.
// Rationale: typically the value parameter is known, either 0 or 1.
//

VOID
SYMCRYPT_CALL
SymCryptModElementSetValueNegUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = -value mod Mod
// value is published.
// Requirement:
//  - 0 < value < Mod
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
// Note: this function does NOT hide the value.
// Rationale: typically the value parameter is known, either 0 or 1.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModElementGetValue(
                                    PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_writes_bytes_( cbDst )     PBYTE                   pbDst,
                                    SIZE_T                  cbDst,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// (pbDst, cbDst) = encode( format, cbDst, Src )
// Requirement:
//  - SymCryptDigitsFromBits( 8 * cbDst ) <= Mod.nDigits
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// Retrieve the value of a ModElement as an array of bytes
//

UINT32
SYMCRYPT_CALL
SymCryptModElementIsEqual(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc1,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc2 );
//
// Returns a mask value which is 0xffffffff if Src1 = Src2 and 0 otherwise.
//
// Both SYMCRYPT_MODELEMENTs should have been created using the modulus pmMod. Otherwise
// the result is undefined.
//

UINT32
SYMCRYPT_CALL
SymCryptModElementIsZero(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc );
//
// Returns a mask value which is 0xffffffff if Src = 0 and 0 otherwise.
//
// Useful for quickly checking if a ModElement is 0.
//


//===============================
// Modular arithmetic.
//

VOID
SYMCRYPT_CALL
SymCryptModSetRandom(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = random value modulus Mod.
// Requirement:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// Random value is chosen uniformly from the set of allowed values.
// By default this function does not return the values 0, 1, or -1 (see below NOTE for small moduli exception)
// Flags parameter can signal that these special values are allowed.
// flags parameter is published.
//
// Rationale: these values cause problems in many situations, and for all commonly used cryptographic modulo sizes
// the absence of these values is statistically undetectable even if they are allowed.
// For completeness of the API, the flags parameter can be used to allow these three values.
// flags is a bitmask containing a combination of the following bit values:
//  SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO
//  SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE
//  SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE
// Specifying ALLOW_ZERO implies ALLOW_ONE, there is no way to allow 0 and disallow 1.
//
// NOTE:
// For very small moduli (1, 2, and 3), not allowing 0, 1, or -1 by default does not make sense because this would
// exclude all possible values! Instead the default behavior is to allow -1 for these moduli.
//  Modulo 1 => return 0 by default
//  Modulo 2 => return 1 by default
//      may also return 0 if SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO is specified
//  Modulo 3 => return 2 by default
//      may also return 1 if SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE is specified, and
//      may also return 0 or 1 if SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO is specified,
//
// Callers relying on not having 0, 1, or -1 are required to pass a larger modulus.

#define SYMCRYPT_FLAG_MODRANDOM_ALLOW_ZERO      (0x01)
#define SYMCRYPT_FLAG_MODRANDOM_ALLOW_ONE       (0x02)
#define SYMCRYPT_FLAG_MODRANDOM_ALLOW_MINUSONE  (0x04)


VOID
SYMCRYPT_CALL
SymCryptModNeg(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = -Src mod Mod
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//

VOID
SYMCRYPT_CALL
SymCryptModAdd(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src1 + Src2 mod Mod
// Requirement:
//  - Src1.modulus == Src2.modulus == Mod.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
// Dst == Src1, Dst == Src2, and Src1 == Src2 are all allowed.
// Rationale:
//  scratch space can make the mod-add faster for side-channel safe implementations.
//  It allows:
//      Dst = Src1 + Src2;
//      Tmp = Dst - Mod;
//      Dst = choose( Dst, Tmp, carry_bits )
//  And the choose() operation is fast because it does not require carry propagation.
//


VOID
SYMCRYPT_CALL
SymCryptModSub(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src1 - Src2 mod Mod
// Requirement:
//  Same as SymCryptModAdd
//


VOID
SYMCRYPT_CALL
SymCryptModMul(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src1 * Src2 mod Mod
// Requirement:
//      - Src1.modulus == Src2.modulus == Mod.
//      - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//

VOID
SYMCRYPT_CALL
SymCryptModSquare(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src1^2 mod Mod
// Requirement:
//      - Src.modulus == Mod.
//      - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//

VOID
SYMCRYPT_CALL
SymCryptModDivPow2(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
                                    UINT32                  exp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Src / 2^exp mod Mod
// Requirements:
//      - Mod is odd.
//      - Src.modulus == Dst.modulus == Mod.
//      - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( Mod.nDigits )
//
// Remarks:
//  - The value exp is *** public ***; hence it should be treated as known to the attacker.
//  - This function may write intermediate values to peDst and read them back, violating the
//    read-once/write-once rule, so the caller must ensure that the peDst buffer is trusted.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODINV( _nDigits )

// SYMCRYPT_FLAG_DATA_PUBLIC signals that the Src element is public and does not have to be protected
// against side-channel attacks. The public-ness of the Modulus is part of the Modulus object, specified when the
// modulus value was set.
// Marking the source value as public has very little effect on performance, but it removes the random blinding used.
// The main goal of this flag is to allow ECDSA verification without a source of random numbers.

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModInv(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = 1/Src mod Mod.
//
// - pmMod: Modulus, must have the SYMCRYPT_FLAG_MODULUS_PRIME and SYMCRYPT_FLAG_DATA_PUBLIC flag set.
//      Non-prime or non-public moduli are currently not supported.
// - peSrc: Source value, modulo pmMod
// - peDst: Destination value, mod element modulo pmMod
// - flags: SYMCRYPT_FLAG_DATA_PUBLIC signals that peSrc is a public value.
// - pbScratch/cbScratch: scratch space >= SYMCRYPT_SCRATCH_BYTES_FOR_MODINV( nDigits( pmMod ) )
//
// Returns an error if
//  - GCD( Src, Mod ) != 1
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( _nDigits ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODEXP( _nDigits )

VOID
SYMCRYPT_CALL
SymCryptModExp(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peBase,
    _In_                            PCSYMCRYPT_INT          piExp,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = Base ^ Exp mod Mod
// where only the least significant (nBitsExp) bits of the exponent are used.
//
// Requirements:
//  - nBitsExp != 0
//  - Mod > 1
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_MODEXP( Mod.nDigits )
//
// Allowed flags:
//      SYMCRYPT_FLAG_DATA_PUBLIC: If set then the algorithm
//      is not side-channel safe (For use in RSA encryption - exponentiation
//      with a public exponent). The default behaviour is side channel safety.
//
// Remarks:
//  - The undefined operation 0^0 will return 1.
//  - The value nBitsExp is *** public ***; hence it should be treated as known to the attacker.
//      Examples:
//          -   nBitsExp = SymCryptIntBitsizeOfObject( piExp ) => This processes all the
//              bits of the exponent object.
//          -   nBitsExp = number of bits of modulus ==> This processes the same
//              number of bits (even leading zeros) as the modulus. In this case
//              the exponent should have a value with bitsize less or equal to the
//              bitsize of the modulus.
//          -   nBitsExp = max(1, SymCryptIntBitsizeOfValue( piExp )) => This processes
//              the bits of the exponent ignoring the leading zeros. Therefore, this
//              option leaks the bitsize of the value of the exponent.
//

// SYMCRYPT_MODMULTIEXP_MAX_NBASES, _NBITSEXP: The maximum number of bases
// and exponent bits allowed for the multi-exponentiation operation.
#define SYMCRYPT_MODMULTIEXP_MAX_NBASES         (8)
#define SYMCRYPT_MODMULTIEXP_MAX_NBITSEXP       (SYMCRYPT_INT_MAX_BITS)

#define SYMCRYPT_SCRATCH_BYTES_FOR_MODMULTIEXP( _nDigits, _nBases, _nBitsExp ) SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODMULTIEXP( _nDigits, _nBases, _nBitsExp )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModMultiExp(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_( nBases )            PCSYMCRYPT_MODELEMENT * peBaseArray,
    _In_reads_( nBases )            PCSYMCRYPT_INT *        piExpArray,
                                    UINT32                  nBases,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Dst = ( peBaseArray[0]^piExpArray[0] * peBaseArray[1]^piExpArray[1] * ... *
//         peBaseArray[nBases-1]^piExpArray[nBases-1] ) mod Mod
// where only the least significant (nBitsExp) bits of the exponents are used.
//
// Requirements:
//  - 1<= nBitsExp <= SYMCRYPT_MODMULTIEXP_MAX_NBITSEXP
//  - Mod > 1
//  - 1<= nBases <= SYMCRYPT_MODMULTIEXP_MAX_NBASES
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_MODMULTIEXP( Mod.nDigits, nBases, nBitsExp )
//
// Allowed flags:
//      SYMCRYPT_FLAG_DATA_PUBLIC: If set then the algorithm
//      is not side-channel safe (For use in DSA verification).
//      The default behaviour is side channel safety.
//

// =========================================
// Tools for side-channel safety
// ========================================

//
//Side-channel safe lookup table
//

typedef struct _SYMCRYPT_SCSTABLE {
    UINT32  groupSize;
    UINT32  interleaveSize;
    UINT32  nElements;          // must be multiple of groupSize
    UINT32  elementSize;        // # bytes in each element, note: limited to UINT32 for efficiency
    PBYTE   pbTableData;
    UINT32  cbTableData;
} SYMCRYPT_SCSTABLE, *PSYMCRYPT_SCSTABLE;

UINT32
SYMCRYPT_CALL
SymCryptScsTableInit(
    _Out_   PSYMCRYPT_SCSTABLE  pScsTable,
            UINT32              nElements,
            UINT32              elementSize );
// Initializes an ScsTable for nElements elements each of elementSize bytes.
// nElements and elementSize are limited to less than 2^16.
// Return value is the size of the buffer that the caller needs to provide.
//
// Requirements:
//  -   nElements must be a multiple of groupSize and elementSize must be a
//      multiple of interleaveSize. Currently all implementations have as
//      defaults
//              groupSize = 4
//              interleaveSize = 8
//

VOID
SYMCRYPT_CALL
SymCryptScsTableSetBuffer(
    _Inout_                             PSYMCRYPT_SCSTABLE  pScsTable,
    _Inout_updates_bytes_( cbBuffer )   PBYTE               pbBuffer,
                                        UINT32              cbBuffer );
// Sets the caller-provided buffer on the ScsTable.
// cbBuffer should be >= the size returned by the SymCryptScsTableInit function

VOID
SYMCRYPT_CALL
SymCryptScsTableStore(
    _Inout_                     PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _In_reads_bytes_( cbData )  PCBYTE              pbData,
                                UINT32              cbData );
// Not side-channel safe; publishes iIndex.
// cbData must match the elementSize i.e. the size of a single element.

VOID
SYMCRYPT_CALL
SymCryptScsTableLoad(
    _In_                        PSYMCRYPT_SCSTABLE  pScsTable,
                                UINT32              iIndex,
    _Out_writes_bytes_(cbData)  PBYTE               pbData,
                                UINT32              cbData );
// Side-channel safe fetching of data; iIndex is kept secret.
// cbData must match the elementSize i.e. the size of a single element.

VOID
SYMCRYPT_CALL
SymCryptScsTableWipe(
    _Inout_ PSYMCRYPT_SCSTABLE pScsTable );
// Wipes the part of the buffer that the table used

// Other Side-channel safety tools

VOID
SYMCRYPT_CALL
SymCryptScsRotateBuffer(
    _Inout_updates_( cbBuffer ) PBYTE   pbBuffer,
                                SIZE_T  cbBuffer,
                                SIZE_T  lshift );
// Rotates buffer left by lshift without revealing lshift
// through side channels.
// - pbBuffer/cbBuffer: buffer to rotate
//   pbBuffer must be aligned to the native integer of the platform (4 or 8 bytes)
//   cbBuffer must be a power of two >= 32
// - lshift: # bytes to left rotate the buffer
//      pbBuffer[0] will get the value pbBuffer[ lshift % cbBuffer ]

VOID
SYMCRYPT_CALL
SymCryptScsCopy(
    _In_reads_( cbDst )     PCBYTE  pbSrc,
                            SIZE_T  cbSrc,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst );
// Copy cbSrc bytes of pbSrc into pbDst without revealing cbSrc
// through side channels.
//
// WARNING: pbSrc buffer must be at least cbDst bytes long; not cbSrc!
//
// - pbSrc  pointer to buffer to copy data from
//          This buffer must be at least cbDst bytes long
// - cbSrc  number of bytes to be copied, must be <= 2^31
// - pbDst  destination buffer
// - pbDst  size of the destination buffer, must be <= 2^31
// Equivalent to:
//      n = min( cbSrc, cbDst )
//      pbDst[ 0.. n-1 ] = pbSrc[ 0 .. n - 1 ]
// cbSrc is protected from side-channels; cbDst is public.


//
// Mask generation functions.
// All these functions are side-channel safe in all parameters.
// Naming convention:
// SymCrypt <MaskType> <Op> <ParameterType>
// <MaskType> is the type of the function result:
//      Mask32      UINT32 mask that is 0 or -1
//      Mask64      UINT64 mask that is 0 or -1
// <Op> is the boolean operation performed on the parameters
//      IsZero      v == 0
//      IsNonzero   v != 0
//      Eq          a == b
//      Neq         a != b
// <ParameterType> is an indication of the parameter type supported.
//      U31     UINT32 which is limited to values < 2^31
//              This allows more efficient masking functions.
//      U32     UINT32
// Other mask types, operations, and parameter types may be defined in future.
//

UINT32
SYMCRYPT_CALL
SymCryptMask32IsZeroU31( UINT32 v );

UINT32
SYMCRYPT_CALL
SymCryptMask32IsNonzeroU31( UINT32 v );


UINT32
SYMCRYPT_CALL
SymCryptMask32EqU32( UINT32 a, UINT32 b );

UINT32
SYMCRYPT_CALL
SymCryptMask32NeqU31( UINT32 a, UINT32 b );

UINT32
SYMCRYPT_CALL
SymCryptMask32LtU31( UINT32 a, UINT32 b );


//
// Other helper functions
//
SIZE_T
SYMCRYPT_CALL
SymCryptRoundUpPow2Sizet( SIZE_T v );
// Round up to the next power of 2
//
// Requirements:
//  v <= (SIZE_T_MAX / 2) + 1
//    i.e. rounding v up to the next power of 2 fits within SIZE_T, so v is
//    less than or equal to the maximum power of 2 representable in SIZE_T


//=====================================================
//=====================================================
// RSA padding operations
//=====================================================
//=====================================================

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1ApplyEncryptionPadding(
    _In_reads_bytes_( cbPlaintext )     PCBYTE      pbPlaintext,
                                        SIZE_T      cbPlaintext,
    _Out_writes_bytes_( cbPkcs1Format ) PBYTE       pbPkcs1Format,
                                        SIZE_T      cbPkcs1Format );
//
// Applies the RSA PKCS1 v1.5 encryption padding to the plaintext buffer.
// - Plaintext      buffer containing plaintext to be encoded
// - Pkcs1Format    Output buffer, typically the size of the RSA modulus
// Requirement: cbPkcs1Format >= cbPlaintext + 11 due to the PKCS1 overhead.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1RemoveEncryptionPadding(
    _Inout_updates_bytes_( cbPkcs1Buffer )  PBYTE       pbPkcs1Format,
                                            SIZE_T      cbPkcs1Format,
                                            SIZE_T      cbPkcs1Buffer,
    _Out_writes_bytes_opt_( cbPlaintext )   PBYTE       pbPlaintext,
                                            SIZE_T      cbPlaintext,
    _Out_                                   SIZE_T     *pcbPlaintext );
//
// Remove the PKCS1 encryption padding and extract the message plaintext.
// This function is side-channel safe w.r.t. the data in the Pkcs1Format buffer.
// - pbPkcs1Format points to a buffer containing the raw RSA decrypted data.
//      This buffer will be modified by this function.
// - cbPkcs1Format is the # bytes of the buffer that were decrypted with raw RSA
// - cbPkcs1Buffer is the size of the buffer that pbPkcs1Format points to
//      cbPkcs1Buffer must be a power of 2 and >= cbPkcs1Format and >= 32
//      cbPkcs1Buffer must be <= 2^30
// - pbPlaintext/cbPlaintext is the output buffer that will receive the data.
//      if pbPlaintext == NULL no message is output, but *pcbPlaintext is still set.
// - pcbPlaintext receives the # bytes in the actual decrypted message.
//      set to 0 if an error occurred.
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( _hashAlgorithm, _nBytesOAEP )      SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_OAEP( _hashAlgorithm, _nBytesOAEP )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepApplyEncryptionPadding(
    _In_reads_bytes_( cbPlaintext )     PCBYTE          pbPlaintext,
                                        SIZE_T          cbPlaintext,
    _In_                                PCSYMCRYPT_HASH hashAlgorithm,
    _In_reads_bytes_( cbLabel )         PCBYTE          pbLabel,
                                        SIZE_T          cbLabel,
    _In_reads_bytes_opt_( cbSeed )      PCBYTE          pbSeed,
                                        SIZE_T          cbSeed,
    _Out_writes_bytes_( cbOaepFormat )  PBYTE           pbOaepFormat,
                                        SIZE_T          cbOaepFormat,
    _Out_writes_bytes_( cbScratch )     PBYTE           pbScratch,
                                        SIZE_T          cbScratch );
//
// Apply the RSA OAEP encryption padding to the plaintext buffer.
// - Plaintext      Plaintext to be encoded
// - hashAlgorithm  Hash algorithm to use during padding
// - Label          Label input for OAEP
// - Seed           Specified seed value. 0 <= cbSeed < hash size
// - OaepFormat     Output buffer, typically the size of the RSA modulus
//
// Remarks:
//      - If pbSeed == NULL and cbSeed != 0, then the function picks
//        a uniformly random seed of size cbSeed bytes.
//
// Requirements:
//      cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( hashAlgorithm, cbOAEPFormat )
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepRemoveEncryptionPadding(
    _In_reads_bytes_( cbOAEPFormat )
                                PCBYTE                      pbOAEPFormat,
                                SIZE_T                      cbOAEPFormat,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPlaintext )
                                PBYTE                       pbPlaintext,
                                SIZE_T                      cbPlaintext,
    _Out_                       SIZE_T                      *pcbPlaintext,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch );
//
// Removes the RSA OAEP encryption padding from the OAEP formatted buffer
// after it checks the validity of the format.
//
// *pcbPlaintext is the number of bytes output. If pbPlaintext == NULL then this
// is the only output value.
//
// Allowed flags:
//      None
//
// Requirements:
//      cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_RSA_OAEP( hashAlgorithm, cbOAEPFormat )
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PKCS1( _nBytesPKCS1 )    SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_PKCS1( _nBytesPKCS1 )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1ApplySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_reads_bytes_( cbHashOid )
                                PCBYTE                      pbHashOid,
                                SIZE_T                      cbHashOid,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPKCS1Format )
                                PBYTE                       pbPKCS1Format,
                                SIZE_T                      cbPKCS1Format );
//
// Applies the RSA PKCS1 v1.5 signature padding to the source buffer, which typically contains the
// hash of the message.
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1VerifySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_reads_( nOIDCount )     PCSYMCRYPT_OID              pHashOIDs,
    _In_                        SIZE_T                      nOIDCount,
    _In_reads_bytes_( cbPKCS1Format )
                                PCBYTE                      pbPKCS1Format,
                                SIZE_T                      cbPKCS1Format,
                                UINT32                      flags,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch );
//
// Verifies that the RSA PKCS1 v1.5 signature padding is valid.
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_VERIFICATION_FAIL
// if it failed.
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID
//
// Requirements:
//      cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PKCS1( cbPKCS1Format )
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( _hashAlgorithm, _nBytesMessage, _nBytesPSS )    SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_PSS( _hashAlgorithm, _nBytesMessage, _nBytesPSS )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssApplySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_opt_( cbSalt )
                                PCBYTE                      pbSalt,
    _In_range_(0, cbPSSFormat)  SIZE_T                      cbSalt,
                                UINT32                      nBitsOfModulus,
                                UINT32                      flags,
    _Out_writes_bytes_( cbPSSFormat )
                                PBYTE                       pbPSSFormat,
                                SIZE_T                      cbPSSFormat,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch );
//
// Applies the RSA PSS signature padding to the source buffer, which typically contains the
// hash of the message.
//
// Remarks:
//      - If pbSalt == NULL and cbSalt != 0, then the function picks
//        a uniformly random salt of size cbSalt bytes.
//
// Allowed flags:
//      None
//
// Requirements:
//      cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( hashAlgorithm, cbHash, cbPSSFormat )
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssVerifySignaturePadding(
    _In_reads_bytes_( cbHash )  PCBYTE                      pbHash,
                                SIZE_T                      cbHash,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_range_(0, cbPSSFormat)  SIZE_T                      cbSalt,
    _In_reads_bytes_( cbPSSFormat )
                                PCBYTE                      pbPSSFormat,
                                SIZE_T                      cbPSSFormat,
                                UINT32                      nBitsOfModulus,
                                UINT32                      flags,
    _Out_writes_bytes_( cbScratch )
                                PBYTE                       pbScratch,
                                SIZE_T                      cbScratch );
//
// Verifies that the RSA PSS signature padding is valid.
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_VERIFICATION_FAIL
// if it failed.
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT
//
//      When the flag is set, this function will do signature verification using the cbSalt parameter as
//      a minimum value for the salt length, rather than using it as an exact value. Specifying this and
//      setting cbSalt = 0 allows callers to verify a signature which has a valid encoding with any salt
//      length using a single call.
//
//
// Requirements:
//      cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_RSA_PSS( hashAlgorithm, cbHash, cbPSSFormat )
//

//=====================================================
//=====================================================
// EC point operations
//=====================================================
//=====================================================

PCSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptEcurveGroupOrder( _In_    PCSYMCRYPT_ECURVE   pCurve );
//
// This function returns a pointer to the group order of the curve's subgroup.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveDigitsofScalarMultiplier( _In_    PCSYMCRYPT_ECURVE   pCurve );
//
// This function returns the number of digits of a scalar that is big enough to
// store a multiplier of an elliptic curve point.
// See also, SymCryptEcurveSizeofScalarMultiplier.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveDigitsofFieldElement( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the number of digits for one coordinate of the public key.
//

//=====================================================
//  GETSET_VALUE_ECURVE_OPERATIONS
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( _pCurve )    SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( _pCurve )

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointSetValue(
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_reads_bytes_(cbSrc)         PCBYTE                  pbSrc,
                                    SIZE_T                  cbSrc,
                                    SYMCRYPT_NUMBER_FORMAT  nformat,
                                    SYMCRYPT_ECPOINT_FORMAT eformat,
    _Out_                           PSYMCRYPT_ECPOINT       poDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Set the value of an ECPOINT object from a source buffer pbSrc of size cbSrc. The buffer
// will contain the necessary coordinates of the ECPOINT in the format specified by nformat
// and eformat. The nformat determines the format of the integers in the buffer while the
// eformat determines the layout (and the number) of the coordinates.
//
// Requirements:
//  - cbSrc = X * SymCryptEcurveSizeofFieldElement( pCurve ) where X depends on the
//    eformat specified and denotes the number of coordinates. For example, for
//    SYMCRYPT_ECPOINT_FORMAT_XY it is equal to 2.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve )
//
// Flag values:
//      SYMCRYPT_FLAG_DATA_PUBLIC   data is public (no side-channel protection needed)
//
// Rationale:
//    Scratch space provides room for conversion of point representations.
//
// Example:
//    Set an ECPOINT to (X,Y) point in affine coordinates where the size of each coordinate
//    is t = SymCryptEcurveSizeofFieldElement( pCurve ) bytes. The coordinates are
//    X=(X_(t-1), ... , X_1, X_0) and Y=(Y_(t-1), ... , Y_1, Y_0) with t-1 the
//    most significant byte. Then the function can be called with
//      pbSrc = { X_(t-1), ... , X_1, X_0, Y_(t-1), ... , Y_1, Y_0 }
//      cbSrc = 2 * t
//      nformat = SYMCRYPT_NUMBER_FORMAT_MSB_FIRST
//      eformat = SYMCRYPT_ECPOINT_FORMAT_XY
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointGetValue(
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_                            PCSYMCRYPT_ECPOINT      poSrc,
                                    SYMCRYPT_NUMBER_FORMAT  nformat,
                                    SYMCRYPT_ECPOINT_FORMAT eformat,
    _Out_writes_bytes_(cbDst)       PBYTE                   pbDst,
                                    SIZE_T                  cbDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
//
// Retrieve the value of an ECPOINT object into a destination buffer pbDst of size cbDst. The buffer
// will contain the necessary coordinates of the ECPOINT in the format specified by nformat
// and eformat. The nformat determines the format of the integers in the buffer while the
// eformat determines the layout (and the number) of the coordinates.
//
// Flag values:
//      SYMCRYPT_FLAG_DATA_PUBLIC   data is public (no side-channel protection needed)
//
// Remarks:
//  - If the source point is the "zero" point and it cannot be exported into the
//    required ECPOINT_FORMAT (XY or X), the function fails with SYMCRYPT_INCOMPATIBLE_FORMAT.
//
// Requirements:
//  - cbDst = X * SymCryptEcurveSizeofFieldElement( pCurve ) where X depends on the
//    eformat specified and denotes the number of coordinates. For example for SYMCRYPT_ECPOINT_FORMAT_XY it is equal to 2.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( pCurve )
//
// Rationale:
// Scratch space provides room for conversion of point representations.
//

//
// Low-level flags for ECC operations
//
// SYMCRYPT_FLAG_DATA_PUBLIC: When set, the operation will not be side-channel safe.
//      It is used to speed up operation on public data. (default: side-channel safe)
//
// SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL: When set, the underlying operation will multiply
//      by the cofactor of the curve. (default: no multiplication by the cofactor)
//      Remark: **Notice that the default behaviour is the opposite of the higher-level
//      functions in symcrypt.h.**
#define SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL          (0x20)

//=====================================================
//  COMMON_ECURVE_OPERATIONS
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( _pCurve )      SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( _pCurve )

VOID
SYMCRYPT_CALL
SymCryptEcpointSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Set the destination point poDst to the zero
// element of the additive group defined by the
// elliptic curve addition rule.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

VOID
SYMCRYPT_CALL
SymCryptEcpointSetDistinguishedPoint(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Set the destination point poDst to the
// distinguished point of the curve.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

#define SYMCRYPT_FLAG_ECPOINT_EQUAL             (0x01)
#define SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL         (0x02)

UINT32
SYMCRYPT_CALL
SymCryptEcpointIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// If the flags argument is equal to 0 (default) or SYMCRYPT_FLAG_ECPOINT_EQUAL, it returns a mask value which is
// 0xffffffff if poSrc1 = poSrc2 and 0 otherwise.
// If the flags argument is equal to SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL, it returns a mask value which is
// 0xffffffff if poSrc1 = -poSrc2 and 0 otherwise.
// If the flags argument is equal to SYMCRYPT_FLAG_ECPOINT_EQUAL | SYMCRYPT_FLAG_ECPOINT_NEG_EQUAL,
// it returns a mask value which is 0xffffffff if (poSrc1 = poSrc2) or (poSrc1 = -poSrc2) and 0 otherwise.
//
// The points should have been created with the same curve pCurve. Otherwise the result is undefined.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

UINT32
SYMCRYPT_CALL
SymCryptEcpointIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Returns a mask value which is 0xffffffff if the point is the zero point of the group.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

UINT32
SYMCRYPT_CALL
SymCryptEcpointOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Returns a mask value which is 0xffffffff if the point is on curve.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

VOID
SYMCRYPT_CALL
SymCryptEcpointAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Point addition over the curve pCurve.
//      poDst = poSrc1 + poSrc2
//
// Allowed flags:
//      SYMCRYPT_FLAG_DATA_PUBLIC: If set then the algorithm
//      is not side-channel safe (and faster). The default behaviour
//      is side-channel safety.
//
// Remarks:
//      - Complete (i.e. works for all points)
//      - Writes intermediate results to poDst breaking the read-once/write-once rule
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

VOID
SYMCRYPT_CALL
SymCryptEcpointAddDiffNonZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Point addition when *peSrc1 != +- *peSrc2
// and none of them is equal to the zero point.
//
// Remarks:
//      - Side-channel safe
//      - Complete (i.e. works for all points)
//      - Writes intermediate results to poDst breaking the read-once/write-once rule
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

VOID
SYMCRYPT_CALL
SymCryptEcpointDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Point doubling.
//
// Allowed flags:
//      SYMCRYPT_FLAG_DATA_PUBLIC: If set then the algorithm
//      is not side-channel safe (and faster). The default behaviour
//      is side-channel safety.
//
// Remarks:
//      - Side-channel safe
//      - Writes intermediate results to poDst breaking the read-once/write-once rule
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

VOID
SYMCRYPT_CALL
SymCryptEcpointNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Negates (in place) the source point poSrc if mask == 0xffffffff.
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( pCurve ).
//

//=====================================================
//  SCALAR_ECURVE_OPERATIONS
//

#define SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( _pCurve )                  SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( (_pCurve), 1 )
#define SYMCRYPT_SCRATCH_BYTES_FOR_MULTI_SCALAR_ECURVE_OPERATIONS( _pCurve, _nPoints )  SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( (_pCurve), (_nPoints) )

VOID
SYMCRYPT_CALL
SymCryptEcpointSetRandom(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _Out_   PSYMCRYPT_INT           piScalar,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch );
//
// Set the destination point poDst to a random non-zero point
// of the subgroup generated by the distinguished point.
// The function outputs the integer k and the point kG
// where k is picked uniformly at random from the set
// [1, SubgroupOrder-1] ( 0 is excluded).
//
// Requirements:
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve ).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointScalarMul(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_INT      piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT  poSrc,
            UINT32              flags,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );
//
// Multiplication of point by scalar.
//  poDst = piScalar x poSrc
//
// If poSrc == NULL the algorithm uses the distinguished point of the curve as source
// point and it might be faster (depending on the curve optimizations).
//
// Allowed flags:
//      SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL: If set then
//      the scalar is multiplied by the cofactor of
//      the curve. The default behaviour is to not multiply
//      by the cofactor.
//
// Remarks:
//      - Complete
//      - Side-channel safe
//
// Requirements:
//  - The piScalar must have SymCryptEcurveDigitsofScalarMultiplier( pCurve ) digits.
//  - For Non-Montgomery curves, the piScalar must be in the range [0, SubgroupOrder].
//      - This is the caller's responsibility, it is not checked.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( pCurve ).
//

// SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS: The maximum number of points allowed for the
// multi-scalar multiplication operation.
#define SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS        (2)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointMultiScalarMul(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT *        piSrcScalarArray,
    _In_    PCSYMCRYPT_ECPOINT *    poSrcEcpointArray,
            UINT32                  nPoints,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_opt_( cbScratch )
            PBYTE                   pbScratch,
            SIZE_T                  cbScratch );
//
// It executes the multi scalar - add operation for nPoints
// pairs of (exponents, points) in (piSrcScalarArray, poSrcEcpointArray).
//
// If poSrcEcpointArray[0] == NULL the algorithm uses the distinguished point of the curve as
// the first source point and it might be faster (depending on the curve optimizations).
// Only the first source point can be NULL.
//
// Allowed flags:
//      SYMCRYPT_FLAG_ECC_LL_COFACTOR_MUL: If set then
//      the scalar is multiplied by the cofactor of
//      the curve. The default behaviour is to not multiply
//      by the cofactor.
//      SYMCRYPT_FLAG_DATA_PUBLIC: If set then the algorithm
//      is not side-channel safe (For use in the ECDSA
//      verification with public information). The default behaviour
//      is side channel safe.
//
// Requirements:
//  - 1<= nPoints <= SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS
//  - Each piScalar must have SymCryptEcurveDigitsofScalarMultiplier( pCurve ) digits.
//  - cbScratch >= SYMCRYPT_SCRATCH_BYTES_FOR_MULTI_SCALAR_ECURVE_OPERATIONS( pCurve, nPoints ).
//


////////////////////////////////////////////////////////////////////////////
// AES-CTR-DRBG
//

#define SYMCRYPT_RNG_AES_INTERNAL_SEED_SIZE         (32 + 16)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesGenerateSmall(
    _Inout_                             PSYMCRYPT_RNG_AES_STATE pRngState,
    _Out_writes_( cbRandom )            PBYTE                   pbRandom,
                                        SIZE_T                  cbRandom,
    _In_reads_opt_( cbAdditionalInput ) PCBYTE                  pbAdditionalInput,
                                        SIZE_T                  cbAdditionalInput );
//
// Generate random output from the state per SP 800-90.
// Callers should almost always use SymCryptRngAesGenerate from symcrypt.h instead.
//
// This is the core generation function that produces up to 64 kB at a time
// This function returns an error code so that we can test the
// error handling of having done more than 2^48 requests between reseeds,
// as required by SP 800-90.
// This is also the Generate function of our SP800-90 compliant implementation.
// If pRngState->fips140-2Check is true, this function runs the continuous self test
// required by FIPS 140-2 (but not by FIPS 140-3 as far as we know).
// pbAdditionalInput is optional.
//

//=====================================================
// ECDSA-EX
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaSignEx(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_opt_                            PCSYMCRYPT_INT          piK,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature );
//
// This algorithm is the same as SymCryptEcDsaSign except that the caller can specify
// a value of k in piK. It is used in verifying test vectors of ECDSA.
//
// Requirements:
//  - If piK is not NULL it must have SymCryptEcurveDigitsofScalarMultiplier( pCurve ) digits, and
//    must be in range [1, SubgroupOrder-1].
//  - If piK is not NULL and the generated signature would be 0, SYMCRYPT_INVALID_ARGUMENT is
//    returned.
//
// Allowed flags:
//      SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION: If set then the hash value will
//      not be truncated.
//
//      SYMCRYPT_FLAG_DATA_PUBLIC: If specified, all inputs, including the private key, are
//      considered as public information and are not protected against side channel attacks.
//      This should only be used when signing with a publicly known private key (i.e. in the ECDSA self-test)
//

//=====================================================
// ML-KEM-EX
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulateEx(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _In_reads_bytes_( cbRandom )            PCBYTE              pbRandom,
                                            SIZE_T              cbRandom,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE               pbCiphertext,
                                            SIZE_T              cbCiphertext );
//
// Performs the Encapsulate operation of ML-KEM using caller-provided random input.
// It is used in verifying test vectors of ML-KEM.
//
// This uses the public information of an ML-KEM keypair to generate an agreed secret
// and a ciphertext. Only a peer with the private information of an ML-KEM keypair can
// decapsulate the ciphertext to compute the agreed secret.
//
// The arguments are the following:
// - pkMlKemkey: a key which contains public information required for encapsulation.
// - (pbRandom, cbRandom): a buffer containing the input random.
//   Currently cbRandom must be 32 for all parameterizations of ML-KEM.
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of ML-KEM.
// - (pbCiphertext, cbCiphertext): a buffer into which the encapsulated secret is written.
//   cbCiphertext must equal cbCiphertext given by SymCryptMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_*).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemEncapsulateEx(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _In_reads_bytes_opt_( cbMlKemRandom )   PCBYTE                          pbMlKemRandom,
                                            SIZE_T                          cbMlKemRandom,
    _In_reads_bytes_opt_( cbTradRandom )    PCBYTE                          pbTradRandom,
                                            SIZE_T                          cbTradRandom,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE                           pbCiphertext,
                                            SIZE_T                          cbCiphertext );

//
// Performs the Encapsulate operation of Composite ML-KEM using caller-provided random input.
// It is used in verifying test vectors of Composite ML-KEM.
//
// This uses the public information of a Composite ML-KEM keypair to generate an agreed secret
// and a ciphertext. Only a peer with the private information of a Composite ML-KEM keypair can
// decapsulate the ciphertext to compute the agreed secret.
//
// The arguments are the following:
// - pkCompositeMlKemkey: a key which contains public information required for encapsulation.
// - (pbMlKemRandom, cbMlKemRandom): a buffer containing the input random for the ML-KEM component.
//   When pbMlKemRandom is NULL, cbMlKemRandom should be 0, and the function will generate the necessary random input internally.
//   Currently when pbMlKemRandom is not NULL, cbMlKemRandom must be 32 for all parameterizations of Composite ML-KEM.
// - (pbTradRandom, cbTradRandom): a buffer containing the input random for the traditional component.
//   When the traditional portion is an EC key, cbTradRandom must be equal to the private key size of the EC key.
//   If pbTradRandom is NULL, cbTradRandom should be 0, and the function will generate the necessary random input internally.
//   Currently, only EC keys are supported for the traditional component.
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of Composite ML-KEM.
// - (pbCiphertext, cbCiphertext): a buffer into which the encapsulated secret is written.
//   cbCiphertext must equal cbCiphertext given by SymCryptCompositeMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_*).
//

//===================================================================
// 802.11 SAE protocol
//===================================================================
//
// WARNING: These functions are NOT part of the stable SymCrypt API. They are a private
// interface for the Windows WiFi driver. These functions can change or disappear
// at any time as we update our WiFi solutions.
//
// These functions implement the non-standard or 'custom' parts of the SAE protocol for
// 802.11 SAE as specified in IEEE 801.11-2016 12.4
//
// Parts of the protocol that are easy to implement with conventional crypto functions are
// not included in this custom part.
//
// Limitation: The Hunting-and-Pecking method supports only NIST P256 curve. The Hash-to-Element
//             method supports curves NIST P256 and NIST P384.
//

//
// IANA Group numbers identify the elliptic curve and associated parameters to be used in the SAE method.
//
typedef enum _SYMCRYPT_802_11_SAE_GROUP {
    SYMCRYPT_SAE_GROUP_19 = 19,        // NIST P256
    SYMCRYPT_SAE_GROUP_20,             // NIST P384
} SYMCRYPT_802_11_SAE_GROUP;

// The sizes of scalars, elliptic curve points, and HMAC outputs will vary depending on which group is selected.
// The following macros define the largest possible sizes supported.
#define SYMCRYPT_SAE_MAX_MOD_SIZE_BITS          384
#define SYMCRYPT_SAE_MAX_MOD_SIZE_BYTES         SYMCRYPT_BYTES_FROM_BITS( SYMCRYPT_SAE_MAX_MOD_SIZE_BITS )
#define SYMCRYPT_SAE_MAX_EC_POINT_SIZE_BYTES    ( 2 * SYMCRYPT_SAE_MAX_MOD_SIZE_BYTES )
#define SYMCRYPT_SAE_MAX_HMAC_OUTPUT_SIZE_BYTES SYMCRYPT_BYTES_FROM_BITS( 384 )


typedef struct _SYMCRYPT_802_11_SAE_CUSTOM_STATE SYMCRYPT_802_11_SAE_CUSTOM_STATE, *PSYMCRYPT_802_11_SAE_CUSTOM_STATE;
typedef const SYMCRYPT_802_11_SAE_CUSTOM_STATE *PCSYMCRYPT_802_11_SAE_CUSTOM_STATE;
//
// The struct itself is opaque and is defined elsewhere.
// Caller may not rely on the internal fields of the structure as they can
// change at any time.
//

VOID SymCrypt802_11SaeGetGroupSizes(
                                SYMCRYPT_802_11_SAE_GROUP       group,
                    _Out_opt_   SIZE_T*                         pcbScalar,
                    _Out_opt_   SIZE_T*                         pcbPoint );
//
// Helper function that returns the sizes of the field elements and elliptic curve points in bytes
// for a given IANA group number. Both output parameters are optional.
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInit(
    _Out_                       PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
    _In_reads_( 6 )             PCBYTE                              pbMacA,
    _In_reads_( 6 )             PCBYTE                              pbMacB,
    _In_reads_( cbPassword )    PCBYTE                              pbPassword,
                                SIZE_T                              cbPassword,
    _Out_opt_                   PBYTE                               pbCounter,
    _Inout_updates_opt_( 32 )   PBYTE                               pbRand,
    _Inout_updates_opt_( 32 )   PBYTE                               pbMask );
//
// Initialize the state object with the MAC addresses and password.
// All choices for the protocol (i.e. rand and mask) are made at this time.
//
// - State                      Protocol state to initialize
// - pbMacA, pbMacB             Two 6-byte MAC addresses with MacA >= MacB.
// - pbPassword, cbPassword     The password buffer
// - pbCounter                  If not NULL, receives the counter value of the
//                              successful PWE generation per section 12.4.4.2.2
// - pbRand                     Optional pointer to Rand buffer (see below)
// - pbMask                     Optional pointer to Mask buffer (see below)
//
// The Rand and Mask buffers are optional. If a pointer is not provided then the caller
// has no access to the corresponding value.
// For either of these pointers there are three cases:
//  - If a NULL pointer is provided, the function generates an appropriate value internally,
//      but does not return it to the caller.
//  - If a buffer is provided and the buffer is all-zero, the function generates an appropriate
//      value internally and returns it in the buffer.
//  - If a buffer is provided and the buffer is nonzero, the value in the buffer is used for
//      the corresponding protocol parameter without further validation.
// This last option is useful for testing as it lets the caller specify all the random choices.
// Rand and Mask buffers are MSByte first.
//
// Note: currently this method only supports the NIST P256 curve. If we ever want to support other curves
// we'll update this function to accept a curve parameter and update the SAL annotations
// of the other functions.
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCreatePT(
    _In_reads_( cbSsid )                                    PCBYTE                              pbSsid,
                                                            SIZE_T                              cbSsid,
    _In_reads_( cbPassword )                                PCBYTE                              pbPassword,
                                                            SIZE_T                              cbPassword,
    _In_reads_opt_( cbPasswordIdentifier )                  PCBYTE                              pbPasswordIdentifier,
                                                            SIZE_T                              cbPasswordIdentifier,
    _Out_writes_( 64 )                                      PBYTE                               pbPT );
//
// Generate the PT secret element for use with the SAE Hash-to-Element algorithm, as described in
// section 12.4.4.2.3 of the 802.11 spec ("Hash-to-curve generation of the password element with
// ECC groups"). The PT value can be "stored until needed to generate a session specific PWE."
//
// - pbSsid, cbSsid                              SSID for the connection as a string of bytes
// - pbPassword, cbPassword                      Password buffer
// - pbPasswordIdentifier, cbPasswordIdentifier  Optional password identifier, as a string of bytes
// - pbPT                                        Out pointer to PT (as a byte buffer)
//
// This function uses the NIST P256 curve.
//


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCreatePTGeneric(
                                                            SYMCRYPT_802_11_SAE_GROUP           group,
    _In_reads_( cbSsid )                                    PCBYTE                              pbSsid,
                                                            SIZE_T                              cbSsid,
    _In_reads_( cbPassword )                                PCBYTE                              pbPassword,
                                                            SIZE_T                              cbPassword,
    _In_reads_opt_( cbPasswordIdentifier )                  PCBYTE                              pbPasswordIdentifier,
                                                            SIZE_T                              cbPasswordIdentifier,
    _Out_writes_( cbPT )                                    PBYTE                               pbPT,
                                                            SIZE_T                              cbPT );
//
// Generic version of the SymCrypt802_11SaeCustomCreatePT() function that allows elliptic curve
// group selection.
// Generate the PT secret element for use with the SAE Hash-to-Element algorithm, as described in
// section 12.4.4.2.3 of the 802.11 spec ("Hash-to-curve generation of the password element with
// ECC groups"). The PT value can be "stored until needed to generate a session specific PWE."
//
// - group                                       Group number for the elliptic curve selection
// - pbSsid, cbSsid                              SSID for the connection as a string of bytes
// - pbPassword, cbPassword                      Password buffer
// - pbPasswordIdentifier, cbPasswordIdentifier  Optional password identifier, as a string of bytes
// - pbPT, cbPt                                  PT (as a byte buffer)
//


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInitH2E(
    _Out_                                                   PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
    _In_reads_( 64 )                                        PCBYTE                              pbPT,
    _In_reads_( 6 )                                         PCBYTE                              pbMacA,
    _In_reads_( 6 )                                         PCBYTE                              pbMacB,
    _Inout_updates_opt_( 32 )                               PBYTE                               pbRand,
    _Inout_updates_opt_( 32 )                               PBYTE                               pbMask );
//
// Initialize the state object  using the Hash-to-Element algorithm, using the PT value calculated
// by SymCrypt802_11SaeCustomCreatePT.
//
// - pState                     Protocol state
// - pbPT                       PT value calculated using SymCrypt802_11SaeCustomCreatePT()
// - pbMacA, pbMacB             Two 6-byte MAC addresses
// - pbRand                     Optional pointer to Rand buffer. See SymCrypt802_11SaeCustomInit() documentation for the use of this parameter.
// - pbMask                     Optional pointer to Mask buffer. See SymCrypt802_11SaeCustomInit() documentation for the use of this parameter.
//
// See the comment on SymCrypt802_11SaeCustomInit() for more details about the pbRand and pbMask
// parameters.
//


SYMCRYPT_ERROR
SymCrypt802_11SaeCustomInitH2EGeneric(
    _Out_                           PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState,
                                    SYMCRYPT_802_11_SAE_GROUP           group,
    _In_reads_( cbPT )              PCBYTE                              pbPT,
                                    SIZE_T                              cbPT,
    _In_reads_( 6 )                 PCBYTE                              pbMacA,
    _In_reads_( 6 )                 PCBYTE                              pbMacB,
    _Inout_updates_opt_( cbRand )   PBYTE                               pbRand,
                                    SIZE_T                              cbRand,
    _Inout_updates_opt_( cbMask )   PBYTE                               pbMask,
                                    SIZE_T                              cbMask );
//
// Generic version of the SymCrypt802_11SaeCustomInitH2E() function that allows elliptic curve
// group selection.
// Initialize the state object  using the Hash-to-Element algorithm, using the PT value calculated
// by SymCrypt802_11SaeCustomCreatePT.
//
// - pState                     Protocol state
// - group                      Group number for the elliptic curve selection
// - pbPT, cbPT                 PT value (as a byte array) calculated using SymCrypt802_11SaeCustomCreatePTGeneric().
//                              PT must be generated on the same elliptic curve as the one supplied in the group parameter.
// - pbMacA, pbMacB             Two 6-byte MAC addresses
// - pbRand, cbRand             Optional Rand buffer. See SymCrypt802_11SaeCustomInit() documentation for the use of this parameter.
// - pbMask, cbMask             Optional Mask buffer. See SymCrypt802_11SaeCustomInit() documentation for the use of this parameter.
//
// See the comment on SymCrypt802_11SaeCustomInit() for more details about the pbRand and pbMask
// parameters.
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitCreate(
    _In_                    PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _Out_writes_( 32 )      PBYTE                               pbCommitScalar,
    _Out_writes_( 64 )      PBYTE                               pbCommitElement );
//
// Compute the commit-scalar and commit-element values for the Commit message.
// This function does not update the pState and is multi-thread safe w.r.t. the pState object.
//
//  - pState            Protocol state that was initialized with SymCrypt802_11SaeCustomInit().
//  - pCommitScalar     Buffer that receives the commit-scalar value, MSByte first.
//  - pCommitElement    Buffer that receives the commit-element value encoded as two values
//                      (x,y) in order, each value in MSByte first.
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitCreateGeneric(
    _In_                                PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _Out_writes_( cbCommitScalar )      PBYTE                               pbCommitScalar,
                                        SIZE_T                              cbCommitScalar,
    _Out_writes_( cbCommitElement )     PBYTE                               pbCommitElement,
                                        SIZE_T                              cbCommitElement);
//
// Generic version of the SymCrypt802_11SaeCustomCommitCreate() function that uses the
// state object to determine which elliptic curve group is selected.
// Compute the commit-scalar and commit-element values for the Commit message.
// This function does not update the pState and is multi-thread safe w.r.t. the pState object.
//
//  - pState                            Protocol state that was initialized with SymCrypt802_11SaeCustomInit().
//  - pbCommitScalar, cbCommitScalar    Buffer that receives the commit-scalar value, MSByte first.
//  - pbCommitElement, cbCommitElement  Buffer that receives the commit-element value encoded as two values
//                                      (x,y) in order, each value in MSByte first.
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitProcess(
    _In_                    PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _In_reads_( 32 )        PCBYTE                              pbPeerCommitScalar,
    _In_reads_( 64 )        PCBYTE                              pbPeerCommitElement,
    _Out_writes_( 32 )      PBYTE                               pbSharedSecret,
    _Out_writes_( 32 )      PBYTE                               pbScalarSum );
//
// Process the commit message received from the peer.
// This function does not update pState and is multi-thread safe w.r.t. the pState object.
//
// - pState                 pointer to the protocol state.
// - pbPeerCommitScalar     pointer to the peer's commit scalar value, MSByte first.
// - pbPeerCommitElement    pointer to the peer's commit element, see CommitCreate for format.
// - pbSharedSecret         buffer that receives the 'k' value that is the shared secret, MSByte first
// - pbScalarSum            buffer that receives the sum of the two commit scalars, MSByte first
//

SYMCRYPT_ERROR
SymCrypt802_11SaeCustomCommitProcessGeneric(
    _In_                                    PCSYMCRYPT_802_11_SAE_CUSTOM_STATE  pState,
    _In_reads_( cbPeerCommitScalar )        PCBYTE                              pbPeerCommitScalar,
                                            SIZE_T                              cbPeerCommitScalar,
    _In_reads_( cbPeerCommitElement )       PCBYTE                              pbPeerCommitElement,
                                            SIZE_T                              cbPeerCommitElement,
    _Out_writes_( cbSharedSecret )          PBYTE                               pbSharedSecret,
                                            SIZE_T                              cbSharedSecret,
    _Out_writes_( cbScalarSum )             PBYTE                               pbScalarSum,
                                            SIZE_T                              cbScalarSum );
//
// Generic version of the SymCrypt802_11SaeCustomCommitProcess() function that uses the
// state object to determine which elliptic curve group is selected.
// Process the commit message received from the peer.
// This function does not update pState and is multi-thread safe w.r.t. the pState object.
//
// - pState                                     pointer to the protocol state.
// - pbPeerCommitScalar, cbPeerCommitScalar     pointer to the peer's commit scalar value, MSByte first.
// - pbPeerCommitElement, cbPeerCommitElement   pointer to the peer's commit element, see CommitCreate for format.
// - pbSharedSecret, cbSharedSecret             buffer that receives the 'k' value that is the shared secret, MSByte first
// - pbScalarSum, pbSharedSecret                buffer that receives the sum of the two commit scalars, MSByte first
//


VOID
SymCrypt802_11SaeCustomDestroy(
    _Inout_   PSYMCRYPT_802_11_SAE_CUSTOM_STATE   pState );
//
// Wipe a state object.
// After this call the memory used for pState is uninitialized and can be used for other purposes.
// Note that it is not safe to just wipe the memory of the state object as the state
// object contains pointers to other allocations, which can contain secret information.
// The only way to safely destroy a state is to use this function.
//

//===================================================================



#ifdef __cplusplus
}
#endif
