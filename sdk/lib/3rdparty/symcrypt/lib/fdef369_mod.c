//
// fdef_int.c   INT functions for default number format
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#if (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64)

#if !(SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64)
#error code in this file is specific to normal digits being 4 64-bit words, and here 3 64-bit words
#endif

//
// Some of the functions in this file need to know how large the values are in # UINT32s
// On ARM64, normal digits are 256 bits and 369 digits are 192 bits, so the answer is 6 * # digits
// On AMD64, normal digits are 512 bits, and this code is only used for 384 and 576 bits.
//  So if nDigits = 1 then nUint32 = 12, and if nDigits=2 then nUint32 = 18.
//
#if SYMCRYPT_CPU_AMD64
#define SYMCRYPT_FDEF369_DIGITS_TO_NUINT32( nD )        (((nD) + 1) * 6)
#elif SYMCRYPT_CPU_ARM64
#define SYMCRYPT_FDEF369_DIGITS_TO_NUINT32( nD )        ((nD) * 6)
#else
#error ??
#endif

VOID
SYMCRYPT_CALL
SymCryptFdef369ModAddGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 c;
    UINT32 d;
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= nDigits*SYMCRYPT_FDEF_DIGIT_SIZE );

    //
    // Doing add/cmp/sub might be faster or not.
    // Masked add is hard because the mask operations destroy the carry flag.
    //
	// dcl - cleanup?
//    c = SymCryptFdefRawAdd( &pSrc1->uint32[0], &pSrc2->uint32[0], &pDst->uint32[0], nDigits);
//    d = SymCryptFdefRawSub( &pDst->uint32[0], &pMod->Divisor.Int.uint32[0], &pDst->uint32[0], nDigits );
//    e = SymCryptFdefRawMaskedAdd( &pDst->uint32[0], &pMod->Divisor.Int.uint32[0], 0 - (c^d), nDigits );

    c = SymCryptFdef369RawAddAsm( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], &peDst->d.uint32[0], nDigits );
    d = SymCryptFdef369RawSubAsm( &peDst->d.uint32[0], SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), (PUINT32) pbScratch, nDigits );
    SymCryptFdef369MaskedCopyAsm( pbScratch, (PBYTE) &peDst->d.uint32[0], nDigits, (c^d) - 1 );

    // We can't have a carry in the first addition, and no carry in the subtraction.
    SYMCRYPT_ASSERT( !( c == 1 && d == 0 ) );
}


VOID
SYMCRYPT_CALL
SymCryptFdef369ModSubGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 c;
    UINT32 d;
    UINT32 nDigits = pmMod->nDigits;

    SymCryptFdefClaimScratch( pbScratch, cbScratch, SYMCRYPT_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( nDigits ) );
    SYMCRYPT_ASSERT( cbScratch >= nDigits*SYMCRYPT_FDEF_DIGIT_SIZE );

    c = SymCryptFdef369RawSubAsm( &peSrc1->d.uint32[0], &peSrc2->d.uint32[0], &peDst->d.uint32[0], nDigits );
    d = SymCryptFdef369RawAddAsm( &peDst->d.uint32[0], SYMCRYPT_FDEF_INT_PUINT32( &pmMod->Divisor.Int ), (PUINT32) pbScratch, nDigits );
    SymCryptFdef369MaskedCopyAsm( pbScratch, (PBYTE) &peDst->d.uint32[0], nDigits, 0 - c );

    SYMCRYPT_ASSERT( !(c == 1 && d == 0) );
}

VOID
SYMCRYPT_CALL
SymCryptFdef369ModulusInitMontgomery(
    _Inout_                         PSYMCRYPT_MODULUS       pmMod,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdefModulusInitMontgomeryInternal( pmMod, SYMCRYPT_FDEF369_DIGITS_TO_NUINT32( pmMod->nDigits ), pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFdef369ModMulMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    UINT32 nDigits = pmMod->nDigits;
    PUINT32 pTmp = (PUINT32) pbScratch;

    UNREFERENCED_PARAMETER( cbScratch );
    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );

    SymCryptFdef369RawMul( &peSrc1->d.uint32[0], nDigits, &peSrc2->d.uint32[0], nDigits, pTmp );
    SymCryptFdef369MontgomeryReduce( pmMod, pTmp, &peDst->d.uint32[0] );
}

VOID
SYMCRYPT_CALL
SymCryptFdef369ModSquareMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SymCryptFdef369ModMulMontgomery( pmMod, peSrc, peSrc, peDst, pbScratch, cbScratch );
}

VOID
SYMCRYPT_CALL
SymCryptFdef369ModSetPostMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    // Montgomery representation for X is R*X mod M where R = 2^<nDigits * bits-per-digit>
    // Montgomery reduction performs an implicit division by R
    // This function converts to the internal representation by multiplying by R^2 mod M and then performing a Montgomery reduction
    UINT32 nDigits = pmMod->nDigits;

	// dcl - release mode check?
    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    SymCryptFdef369RawMul( &peObj->d.uint32[0], nDigits, pmMod->tm.montgomery.Rsqr, nDigits, (PUINT32) pbScratch );
    SymCryptFdef369MontgomeryReduce( pmMod, (PUINT32) pbScratch, &peObj->d.uint32[0] );
}

PCUINT32
SYMCRYPT_CALL
SymCryptFdef369ModPreGetMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    PUINT32 pTmp = (PUINT32) pbScratch;
    UINT32 nDigits = pmMod->nDigits;
    UINT32 nUint32 = SYMCRYPT_FDEF369_DIGITS_TO_NUINT32( nDigits );

	// dcl - release mode check?
    SYMCRYPT_ASSERT( cbScratch >= nDigits * 2 * SYMCRYPT_FDEF_DIGIT_SIZE );
    UNREFERENCED_PARAMETER( cbScratch );

    memcpy( pTmp, &peObj->d.uint32[0], nUint32 * sizeof( UINT32 ) );
    SymCryptWipe( pTmp + nUint32, nUint32 * sizeof( UINT32 ) );
    SymCryptFdef369MontgomeryReduce( pmMod, pTmp, pTmp );

    // This gives the right result, but it isn't the size that is expected.
    // Wipe the extra bytes
    SymCryptWipe( pTmp + nUint32, nDigits * SYMCRYPT_FDEF_DIGIT_SIZE - nUint32 * sizeof( UINT32 ) );

    return pTmp;
}

VOID
SymCryptFdef369MontgomeryReduce(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst )
{
    SymCryptFdef369MontgomeryReduceAsm( pmMod, pSrc, pDst );
}

VOID
SYMCRYPT_CALL
SymCryptFdef369RawMul(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst )
{
    SymCryptFdef369RawMulAsm( pSrc1, nDigits1, pSrc2, nDigits2, pDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdef369ModInvMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 nDigits = pmMod->nDigits;
    UINT32 nBytes = SYMCRYPT_FDEF369_DIGITS_TO_NUINT32( nDigits ) * sizeof( UINT32 );
    PUINT32 pTmp = (PUINT32) pbScratch;

    SYMCRYPT_ASSERT_ASYM_ALIGNED( pTmp );
    SYMCRYPT_ASSERT( cbScratch >= 2 * nBytes );

    //
    // We have R*X; we first apply the montgomery reduction twice to get X/R, and then invert that
    // using the generic inversion to get R/X.
    //
    memcpy( pTmp, &peSrc->d.uint32[0], nBytes );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdef369MontgomeryReduce( pmMod, pTmp, pTmp );

    SymCryptWipe( (PBYTE)pTmp + nBytes, nBytes );
    SymCryptFdef369MontgomeryReduce( pmMod, pTmp, &peDst->d.uint32[0] );

    scError = SymCryptFdefModInvGeneric( pmMod, peDst, peDst, flags, pbScratch, cbScratch );

    return scError;
}


#endif // CPU_AMD64
