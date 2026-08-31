//
// a_dispatch.c   Dispatch between different arithmetic format implementations.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// SymCrypt can have multiple implementations of the arithmetic operations, and these can
// have incompatible formats used to store the integers.
// This file contains logic to dispatch between these incompatible formats.
// Currently all implementations use the default format, or "Fdef".
//

#include "precomp.h"

//
// Define the FDEF dispatch table here.
// This should eventually be split out so that different users of the library can use different
// table sets & implementation choice functions.
//


const SYMCRYPT_MODULAR_FUNCTIONS g_SymCryptModFns[] = {
    SYMCRYPT_MOD_FUNCTIONS_FDEF_GENERIC,                // Handles any type of modulus
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY,             // Montgomery, only for odd parity-public moduli

#if SYMCRYPT_CPU_AMD64

    SYMCRYPT_MOD_FUNCTIONS_FDEF369_MONTGOMERY,          // optimized for 384 and 576-bit moduli
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX256,     // Special faster code for 256-bit Montgomery moduli, MULX-based code
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULXP384,    // Special faster code for P-384 field modulus, MULX-based code
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX,        // MULX-based code, for any size (digit size = 512 bits)
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX1024,    // Special faster code for 1024-bit Montgomery moduli, MULX-based code
    {NULL,},

    // SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULXP256,    // Special faster code for P-256 field modulus, MULX-based code
    // SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX384,     // Special faster code for 384-bit Montgomery moduli, MULX-based code
    // SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY256,          // Special faster code for 256-bit Montgomery moduli
    // SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY512,          // Special faster code for 512-bit Montgomery moduli
    // SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY1024,         // Special faster code for 1024-bit Montgomery moduli

#elif SYMCRYPT_CPU_ARM64

    SYMCRYPT_MOD_FUNCTIONS_FDEF369_MONTGOMERY,
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_ARM64256,
    SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_ARM64P384,
    {NULL,},
    {NULL,},
    {NULL,},

#endif
};

#define SymCryptModLabel(_label)                (_label << 16)
#define SymCryptModFntableGeneric               (SymCryptModLabel('gM') + (0 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomery            (SymCryptModLabel('mM') + (1 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntable369Montgomery         (SymCryptModLabel('9m') + (2 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomeryMulx256     (SymCryptModLabel('2x') + (3 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomeryMulxP384    (SymCryptModLabel('3n') + (4 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomeryMulx        (SymCryptModLabel('xM') + (5 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomeryMulx1024    (SymCryptModLabel('1x') + (6 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))

#define SymCryptModFntableMontgomeryArm64256    (SymCryptModLabel('2m') + (3 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
#define SymCryptModFntableMontgomeryArm64P384   (SymCryptModLabel('3n') + (4 * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))

// #define SymCryptModFntableMontgomeryMulxP256    (SymCryptModLabel('2n') + (xx * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
// #define SymCryptModFntableMontgomeryMulx384     (SymCryptModLabel('3x') + (xx * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
// #define SymCryptModFntableMontgomery256         (SymCryptModLabel('2m') + (xx * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
// #define SymCryptModFntableMontgomery512         (SymCryptModLabel('5m') + (xx * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))
// #define SymCryptModFntableMontgomery1024        (SymCryptModLabel('1m') + (xx * SYMCRYPT_MODULAR_FUNCTIONS_SIZE))

C_ASSERT( (sizeof( g_SymCryptModFns ) & (sizeof( g_SymCryptModFns) - 1 )) == 0 ); // size of the table must be a power of 2 to be CFG-safe.

const UINT32 g_SymCryptModFnsMask = sizeof( g_SymCryptModFns ) - sizeof( g_SymCryptModFns[0] );

//
// Tweaking the selection & function tables allows different tradeoffs of performance vs codesize
//
const SYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY SymCryptModulusTypeSelections[] =
{
#if SYMCRYPT_CPU_AMD64
    // Mulx used for 0-512 and 577-... bits
    {SymCryptModFntableMontgomeryMulxP384,  SYMCRYPT_CPU_FEATURES_FOR_MULX,  384,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY | SYMCRYPT_MODULUS_FEATURE_NISTP384 },
    {SymCryptModFntableMontgomeryMulx256,   SYMCRYPT_CPU_FEATURES_FOR_MULX,  256,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableMontgomeryMulx,      SYMCRYPT_CPU_FEATURES_FOR_MULX,  512,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntable369Montgomery,       0,                               384,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableMontgomery,          0,                               512,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntable369Montgomery,       0,                               576,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableMontgomeryMulx1024,  SYMCRYPT_CPU_FEATURES_FOR_MULX, 1024,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableMontgomeryMulx,      SYMCRYPT_CPU_FEATURES_FOR_MULX,    0,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },

#elif SYMCRYPT_CPU_ARM64

    {SymCryptModFntableMontgomeryArm64P384, 0,                               384,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY | SYMCRYPT_MODULUS_FEATURE_NISTP384 },
    {SymCryptModFntableMontgomeryArm64256,  0,                               256,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntable369Montgomery,       0,                               384,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableMontgomery,          0,                               512,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntable369Montgomery,       0,                               576,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },

#endif

    {SymCryptModFntableMontgomery,          0,                                 0,   SYMCRYPT_MODULUS_FEATURE_MONTGOMERY },
    {SymCryptModFntableGeneric,             0,                                 0,   0 },
    // This last entry always matches, so the code never falls off the end of this table.
};


//
// At the moment there is only the default number format.
//

UINT32
SymCryptDigitsFromBits( UINT32 nBits )
{
    return SymCryptFdefDigitsFromBits( nBits );
}


PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntAllocate( UINT32 nDigits )
{
    return SymCryptFdefIntAllocate( nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptIntFree( _Out_ PSYMCRYPT_INT piObj )
{
    SymCryptIntWipe( piObj );
    SymCryptCallbackFree( piObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofIntFromDigits( UINT32 nDigits )
{
    return SymCryptFdefSizeofIntFromDigits( nDigits );
}

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    return SymCryptFdefIntCreate( pbBuffer, cbBuffer, nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptIntWipe( _Out_ PSYMCRYPT_INT piDst )
{
    SYMCRYPT_CHECK_MAGIC( piDst );

    // Wipe the whole structure in one go;
    SymCryptWipe( piDst, piDst->cbSize );
}

VOID
SYMCRYPT_CALL
SymCryptIntCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntCopy( piSrc, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntMaskedCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          mask )
{
    SymCryptFdefIntMaskedCopy( piSrc, piDst, mask );
}

VOID
SYMCRYPT_CALL
SymCryptIntConditionalCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          cond )
{
    SymCryptFdefIntConditionalCopy( piSrc, piDst, cond );
}

VOID
SYMCRYPT_CALL
SymCryptIntConditionalSwap(
    _Inout_ PSYMCRYPT_INT   piSrc1,
    _Inout_ PSYMCRYPT_INT   piSrc2,
            UINT32          cond )
{
    SymCryptFdefIntConditionalSwap( piSrc1, piSrc2, cond );
}

UINT32
SYMCRYPT_CALL
SymCryptIntBitsizeOfObject( _In_ PCSYMCRYPT_INT  piSrc )
{
    return SymCryptFdefIntBitsizeOfObject( piSrc );
}

UINT32
SYMCRYPT_CALL
SymCryptIntDigitsizeOfObject( _In_ PCSYMCRYPT_INT piSrc )
{
    return piSrc->nDigits;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntCopyMixedSize(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    return SymCryptFdefIntCopyMixedSize( piSrc, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntBitsizeOfValue( _In_ PCSYMCRYPT_INT piSrc )
{
    return SymCryptFdefIntBitsizeOfValue( piSrc );
}

VOID
SYMCRYPT_CALL
SymCryptIntSetValueUint32(
            UINT32          u32Src,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntSetValueUint32( u32Src, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntSetValueUint64(
            UINT64          u64Src,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntSetValueUint64( u64Src, piDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntSetValue(
    _In_reads_bytes_(cbSrc)     PCBYTE                  pbSrc,
                                SIZE_T                  cbSrc,
                                SYMCRYPT_NUMBER_FORMAT  format,
    _Out_                       PSYMCRYPT_INT           piDst )
{
    return SymCryptFdefIntSetValue( pbSrc, cbSrc, format, piDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptIntGetValue(
    _In_                        PCSYMCRYPT_INT          piSrc,
    _Out_writes_bytes_( cbDst ) PBYTE                   pbDst,
                                SIZE_T                  cbDst,
                                SYMCRYPT_NUMBER_FORMAT  format )
{
    return SymCryptFdefIntGetValue( piSrc, pbDst, cbDst, format );
}

UINT32
SYMCRYPT_CALL
SymCryptIntGetValueLsbits32( _In_  PCSYMCRYPT_INT piSrc )
{
    return SymCryptFdefIntGetValueLsbits32( piSrc );
}

UINT64
SYMCRYPT_CALL
SymCryptIntGetValueLsbits64( _In_  PCSYMCRYPT_INT piSrc )
{
    return SymCryptFdefIntGetValueLsbits64( piSrc );
}

UINT32
SYMCRYPT_CALL
SymCryptIntAddUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst )
{
    return SymCryptFdefIntAddUint32( piSrc1, u32Src2, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntAddSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    return SymCryptFdefIntAddSameSize( piSrc1, piSrc2, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntAddMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    return SymCryptFdefIntAddMixedSize( piSrc1, piSrc2, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntSubUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst )
{
    return SymCryptFdefIntSubUint32( piSrc1, u32Src2, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntSubSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    return SymCryptFdefIntSubSameSize( piSrc1, piSrc2, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntSubMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst )
{
    return SymCryptFdefIntSubMixedSize( piSrc1, piSrc2, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntNeg(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntNeg( piSrc, piDst );
}


VOID
SYMCRYPT_CALL
SymCryptIntMulPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntMulPow2( piSrc, exp, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntDivPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntDivPow2( piSrc, exp, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntShr1(
            UINT32          highestBit,
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntShr1( highestBit, piSrc, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntModPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst )
{
    SymCryptFdefIntModPow2( piSrc, exp, piDst );
}

UINT32
SYMCRYPT_CALL
SymCryptIntGetBit(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit )
{
    return SymCryptFdefIntGetBit( piSrc, iBit );
}

UINT32
SYMCRYPT_CALL
SymCryptIntGetBits(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit,
            UINT32          nBits )
{
    return SymCryptFdefIntGetBits( piSrc, iBit, nBits );
}

VOID
SYMCRYPT_CALL
SymCryptIntSetBits(
    _In_    PSYMCRYPT_INT   piDst,
            UINT32          value,
            UINT32          iBit,
            UINT32          nBits )
{
    SymCryptFdefIntSetBits( piDst, value, iBit, nBits );
}

UINT32
SYMCRYPT_CALL
SymCryptIntIsEqualUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    UINT32          u32Src2 )
{
    return SymCryptFdefIntIsEqualUint32( piSrc1, u32Src2 );
}

UINT32
SYMCRYPT_CALL
SymCryptIntIsEqual(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 )
{
    return SymCryptFdefIntIsEqual( piSrc1, piSrc2 );
}

UINT32
SYMCRYPT_CALL
SymCryptIntIsLessThan(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 )
{
    return SymCryptFdefIntIsLessThan( piSrc1, piSrc2 );
}

UINT32
SYMCRYPT_CALL
SymCryptIntMulUint32(
    _In_                            PCSYMCRYPT_INT  piSrc1,
                                    UINT32          Src2,
    _Out_                           PSYMCRYPT_INT   piDst )
{
    return SymCryptFdefIntMulUint32( piSrc1, Src2, piDst );
}

VOID
SYMCRYPT_CALL
SymCryptIntMulSameSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    SymCryptFdefIntMulSameSize( piSrc1, piSrc2, piDst, pbScratch, cbScratch );
}


VOID
SYMCRYPT_CALL
SymCryptIntSquare(
    _In_                            PCSYMCRYPT_INT  piSrc,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    SymCryptFdefIntSquare( piSrc, piDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptIntMulMixedSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch )
{
    SymCryptFdefIntMulMixedSize( piSrc1, piSrc2, piDst, pbScratch, cbScratch );
}

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorAllocate( UINT32 nDigits )
{
    return SymCryptFdefDivisorAllocate( nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptDivisorFree( _Out_ PSYMCRYPT_DIVISOR pdObj )
{
    SymCryptDivisorWipe( pdObj );
    SymCryptCallbackFree( pdObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofDivisorFromDigits( UINT32 nDigits )
{
    return SymCryptFdefSizeofDivisorFromDigits( nDigits );
}

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    return SymCryptFdefDivisorCreate( pbBuffer, cbBuffer, nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptDivisorWipe( _Out_ PSYMCRYPT_DIVISOR pdObj )
{
    SYMCRYPT_CHECK_MAGIC( pdObj );

    SymCryptWipe( pdObj, pdObj->cbSize );
}

VOID
SymCryptDivisorCopy(
    _In_    PCSYMCRYPT_DIVISOR  pdSrc,
    _Out_   PSYMCRYPT_DIVISOR   pdDst )
{
    SymCryptFdefDivisorCopy( pdSrc, pdDst );
}

UINT32
SYMCRYPT_CALL
SymCryptDivisorDigitsizeOfObject( _In_ PCSYMCRYPT_DIVISOR pdSrc )
{
    return pdSrc->nDigits;
}

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntFromDivisor( _In_ PSYMCRYPT_DIVISOR pdSrc )
{
    return SymCryptFdefIntFromDivisor( pdSrc );
}

VOID
SYMCRYPT_CALL
SymCryptIntToDivisor(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_DIVISOR   pdDst,
                                    UINT32              totalOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    SymCryptFdefIntToDivisor( piSrc, pdDst, totalOperations, flags, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptIntDivMod(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _In_                            PCSYMCRYPT_DIVISOR  pdDivisor,
    _Out_opt_                       PSYMCRYPT_INT       piQuotient,
    _Out_opt_                       PSYMCRYPT_INT       piRemainder,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    SymCryptFdefIntDivMod( piSrc, pdDivisor, piQuotient, piRemainder, pbScratch, cbScratch );
}


PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptModulusAllocate( UINT32 nDigits )
{
    return SymCryptFdefModulusAllocate( nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptModulusFree( _Out_ PSYMCRYPT_MODULUS pmObj )
{
    SymCryptFdefModulusFree( pmObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofModulusFromDigits( UINT32 nDigits )
{
    return SymCryptFdefSizeofModulusFromDigits( nDigits );
}

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptModulusCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits )
{
    return SymCryptFdefModulusCreate( pbBuffer, cbBuffer, nDigits );
}

VOID
SYMCRYPT_CALL
SymCryptModulusWipe( _Out_ PSYMCRYPT_MODULUS pmObj )
{
    SYMCRYPT_CHECK_MAGIC( pmObj );

    SymCryptWipe( pmObj, pmObj->cbSize );
}

VOID
SymCryptModulusCopy(
    _In_    PCSYMCRYPT_MODULUS  pmSrc,
    _Out_   PSYMCRYPT_MODULUS   pmDst )
{
    SymCryptFdefModulusCopy( pmSrc, pmDst );
}

UINT32
SYMCRYPT_CALL
SymCryptModulusDigitsizeOfObject( _In_ PCSYMCRYPT_MODULUS pmSrc )
{
    return pmSrc->nDigits;
}

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptModElementAllocate( _In_ PCSYMCRYPT_MODULUS pmMod )
{
    return SymCryptFdefModElementAllocate( pmMod );
}

VOID
SYMCRYPT_CALL
SymCryptModElementFree(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peObj )
{
    SymCryptFdefModElementFree( pmMod, peObj );
}

UINT32
SYMCRYPT_CALL
SymCryptSizeofModElementFromModulus( PCSYMCRYPT_MODULUS pmMod )
{
    return SymCryptFdefSizeofModElementFromModulus( pmMod );
}

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptModElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
    _In_                            PCSYMCRYPT_MODULUS  pmMod )
{
    return SymCryptFdefModElementCreate( pbBuffer, cbBuffer, pmMod );
}

VOID
SYMCRYPT_CALL
SymCryptModElementWipe(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peDst )
{
    SymCryptFdefModElementWipe( pmMod, peDst );
}

VOID
SymCryptModElementCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst )
{
    SymCryptFdefModElementCopy( pmMod, peSrc, peDst );
}

VOID
SymCryptModElementMaskedCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst,
            UINT32                  mask )
{
    SymCryptFdefModElementMaskedCopy( pmMod, peSrc, peDst, mask );
}

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptDivisorFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc )
{
    return SymCryptFdefDivisorFromModulus( pmSrc );
}

VOID
SymCryptModElementConditionalSwap(
    _In_     PCSYMCRYPT_MODULUS    pmMod,
    _Inout_  PSYMCRYPT_MODELEMENT  peData1,
    _Inout_  PSYMCRYPT_MODELEMENT  peData2,
    _In_     UINT32                cond )
{
     SymCryptFdefModElementConditionalSwap( pmMod, peData1, peData2, cond );
}

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptIntFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc )
{
    return SymCryptFdefIntFromModulus( pmSrc );
}

VOID
SYMCRYPT_CALL
SymCryptIntToModulus(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_MODULUS   pmDst,
                                    UINT32              averageOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch )
{
    PSYMCRYPT_INT piSrcTweak = (PSYMCRYPT_INT) piSrc;

    // In CHKed build, we'll verify that the modulus is not prime, or that it is 2 or odd
    // (Some inversion algorithms fail hard when one input isn't 2 or odd.)
    // We are constant-time w.r.t. piSrc being odd or =2. We don't hide the size of any input,
    // but inputs 2 and 3 are handled with the same code path.
    SYMCRYPT_ASSERT( ((flags & SYMCRYPT_FLAG_MODULUS_PRIME) == 0) ||
        (((SymCryptIntGetValueLsbits32( piSrc ) & 1) | SymCryptIntIsEqualUint32( piSrc, 2 )) != 0) );

    SymCryptFdefIntToModulus( piSrcTweak, pmDst, averageOperations, flags, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptIntToModElement(
    _In_                            PCSYMCRYPT_INT          piSrc,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefIntToModElement( piSrc, pmMod, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModElementToInt(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_INT           piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    PCUINT32 pData;

    SYMCRYPT_ASSERT( piDst->nDigits >= pmMod->nDigits );

    pData = SYMCRYPT_MOD_CALL( pmMod ) modPreGet( pmMod, peSrc, pbScratch, cbScratch );

    SymCryptFdefModElementToIntGeneric( pmMod, pData, piDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModElementSetValue(
    _In_reads_bytes_( cbSrc )       PCBYTE                  pbSrc,
                                    SIZE_T                  cbSrc,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR  scError;

    scError = SymCryptFdefModElementSetValueGeneric( pbSrc, cbSrc, format, pmMod, peDst, pbScratch, cbScratch );

    if( scError == SYMCRYPT_NO_ERROR )
    {
        SYMCRYPT_MOD_CALL( pmMod ) modSetPost( pmMod, peDst, pbScratch, cbScratch );
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModElementGetValue(
                                    PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_writes_bytes_( cbDst )     PBYTE                   pbDst,
                                    SIZE_T                  cbDst,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    return SymCryptFdefModElementGetValue( pmMod, peSrc, pbDst, cbDst, format, pbScratch, cbScratch );
}

UINT32
SYMCRYPT_CALL
SymCryptModElementIsEqual(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc1,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc2 )
{
    return SymCryptFdefModElementIsEqual( pmMod, peSrc1, peSrc2 );
}

UINT32
SYMCRYPT_CALL
SymCryptModElementIsZero(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc )
{
    return SymCryptFdefModElementIsZero( pmMod, peSrc );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModAdd(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_MOD_CALL( pmMod ) modAdd( pmMod, peSrc1, peSrc2, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModSub(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_MOD_CALL( pmMod ) modSub( pmMod, peSrc1, peSrc2, peDst, pbScratch, cbScratch );
}


SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModMul(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_MOD_CALL( pmMod ) modMul( pmMod, peSrc1, peSrc2, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModSquare(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_MOD_CALL( pmMod ) modSquare( pmMod, peSrc, peDst, pbScratch, cbScratch );
}


SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModNeg(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_MOD_CALL( pmMod ) modNeg( pmMod, peSrc, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModElementSetValueUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModElementSetValueUint32Generic( value, pmMod, peDst, pbScratch, cbScratch );

    SYMCRYPT_MOD_CALL( pmMod ) modSetPost( pmMod, peDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptModElementSetValueNegUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModElementSetValueNegUint32( value, pmMod, peDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptModDivPow2(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
                                    UINT32                  exp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModDivPow2( pmMod, peSrc, exp, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModInv(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    return SYMCRYPT_MOD_CALL( pmMod ) modInv( pmMod, peSrc, peDst, flags, pbScratch, cbScratch );
}

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
                                    SIZE_T                  cbScratch )
{
    SymCryptModExpGeneric( pmMod, peBase, piExp, nBitsExp, flags, peDst, pbScratch, cbScratch );
}

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
                                    SIZE_T                  cbScratch )
{
    return SymCryptModMultiExpGeneric( pmMod, peBaseArray, piExpArray, nBases, nBitsExp, flags, peDst, pbScratch, cbScratch );
}

SYMCRYPT_DISABLE_CFG
VOID
SYMCRYPT_CALL
SymCryptModSetRandom(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModSetRandomGeneric( pmMod, peDst, flags, pbScratch, cbScratch );

    SYMCRYPT_MOD_CALL( pmMod ) modSetPost( pmMod, peDst, pbScratch, cbScratch );
}

PCSYMCRYPT_TRIALDIVISION_CONTEXT
SYMCRYPT_CALL
SymCryptCreateTrialDivisionContext( UINT32 nDigits )
{
    return SymCryptFdefCreateTrialDivisionContext( nDigits );
}

UINT32
SYMCRYPT_CALL
SymCryptIntFindSmallDivisor(
    _In_                            PCSYMCRYPT_TRIALDIVISION_CONTEXT    pContext,
    _In_                            PCSYMCRYPT_INT                      piSrc,
    _Out_writes_bytes_( cbScratch ) PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptFdefIntFindSmallDivisor( pContext, piSrc, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFreeTrialDivisionContext( PCSYMCRYPT_TRIALDIVISION_CONTEXT pContext )
{
    SymCryptFdefFreeTrialDivisionContext( pContext );
}
