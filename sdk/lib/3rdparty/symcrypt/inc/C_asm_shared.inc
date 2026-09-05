/*
 C_asm_shared.inc  file to synchronize C and Asm information
 Copyright (c) Microsoft Corporation. Licensed under the MIT license.

 This is a file that is included in both C and ASM such that the values are the same on both sides.
 We use the C preprocessor to set ASM constants, as we already need to use the C preprocessor for
 symcryptasm processing (see scripts/symcryptasm_processor.py).
 We use this to define the structure offsets that the ASM code uses.
 By having equivalent C constants we can add checks to the C code to ensure they are correct.

*/

#include "symcrypt_internal_shared.inc"

#if defined(SYMCRYPT_MASM)
#define SET(_variable, _value) _variable EQU _value
#elif defined(SYMCRYPT_GAS)
#define SET(_variable, _value) .set _variable, _value
#else // assume C
#define SET(_variable, _value) const SIZE_T _variable = _value;
#endif

SET(SymCryptModulusNdigitsOffsetAmd64,          4);
SET(SymCryptModulusInv64OffsetAmd64,            24);
SET(SymCryptModulusValueOffsetAmd64,            128);
SET(SymCryptNegDivisorSingleDigitOffsetAmd64,   256);

SET(SymCryptModulusNdigitsOffsetX86,            4);
SET(SymCryptModulusInv64OffsetX86,              24);
SET(SymCryptModulusValueOffsetX86,              96);

SET(SymCryptModulusNdigitsOffsetArm64,          4);
SET(SymCryptModulusInv64OffsetArm64,            24);
SET(SymCryptModulusValueOffsetArm64,            128);

SET(SymCryptModulusNdigitsOffsetArm,            4);
SET(SymCryptModulusInv64OffsetArm,              24);
SET(SymCryptModulusValueOffsetArm,              96);

#if !defined(SYMCRYPT_MASM) && !defined(SYMCRYPT_GAS)
// Preserve the definition of SET for use in symcryptasm processing
#undef SET
#endif

#if SYMCRYPT_CPU_AMD64
#define SYMCRYPT_CHECK_ASM_OFFSETS \
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusNdigitsOffsetAmd64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, nDigits ) );\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusInv64OffsetAmd64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, inv64 ));\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusValueOffsetAmd64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor.Int.ti.fdef.uint32 ));\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptNegDivisorSingleDigitOffsetAmd64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor.Int.ti.fdef.uint32 ) + (2*SYMCRYPT_FDEF_DIGIT_SIZE) );\

#elif SYMCRYPT_CPU_X86
#define SYMCRYPT_CHECK_ASM_OFFSETS \
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusNdigitsOffsetX86, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, nDigits ) );\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusInv64OffsetX86, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, inv64 ));\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusValueOffsetX86, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor.Int.ti.fdef.uint32 ));\

#elif SYMCRYPT_CPU_ARM64
#define SYMCRYPT_CHECK_ASM_OFFSETS \
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusNdigitsOffsetArm64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, nDigits ) );\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusInv64OffsetArm64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, inv64 ));\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusValueOffsetArm64, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor.Int.ti.fdef.uint32 ));\

#elif SYMCRYPT_CPU_ARM
#define SYMCRYPT_CHECK_ASM_OFFSETS \
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusNdigitsOffsetArm, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, nDigits ) );\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusInv64OffsetArm, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, inv64 ));\
    SYMCRYPT_CHECK_ASM_OFFSET( SymCryptModulusValueOffsetArm, SYMCRYPT_FIELD_OFFSET( SYMCRYPT_MODULUS, Divisor.Int.ti.fdef.uint32 ));\

#endif // CPU_*

#if !defined(SYMCRYPT_CHECK_ASM_OFFSETS)
#define SYMCRYPT_CHECK_ASM_OFFSETS
#endif
