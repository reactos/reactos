//
// cpuid.c   code for CPU feature detection based on CPUID
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#include "precomp.h"

#if (SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64) & SYMCRYPT_MS_VC
#include <excpt.h>
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("xsave"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("xsave")
#endif

//
// RDRAND availability is signaled by CPUID.1.ecx[30]
// PCLMULQDQ availability is signaled by CPUID.1.ecx[1]
// AES_NI availability is signaled by CPUID.1.ecx[25]
// SSSE3 availability is signaled by CPUID.1.ecx[9]
// SSE3 availability is signaled by CPUID.1.ecx[0]
// SSE2 availability is signaled by CPUID.1.edx[26]
//

#define CPUID_1_ECX_RDRAND_BIT      30
#define CPUID_1_ECX_PCLMULQDQ_BIT   1
#define CPUID_1_ECX_AESNI_BIT       25
#define CPUID_1_ECX_SSSE3_BIT       9
#define CPUID_1_ECX_SSE3_BIT        0
#define CPUID_1_EDX_SSE2_BIT        26
#define CPUID_1_EDX_SSE_BIT         25
#define CPUID_1_ECX_AVX_BIT         28
#define CPUID_1_ECX_CMPXCHG16B_BIT  13
#define CPUID_70_EBX_AVX2_BIT       5
#define CPUID_70_EBX_RDSEED_BIT     18
#define CPUID_70_EBX_SHANI_BIT      29
#define CPUID_70_EBX_ADX_BIT        19
#define CPUID_70_EBX_BMI2_BIT       8
#define CPUID_70_EBX_AVX512F_BIT    16
#define CPUID_70_EBX_AVX512BW_BIT   30
#define CPUID_70_EBX_AVX512DQ_BIT   17
#define CPUID_70_EBX_AVX512VL_BIT   31
#define CPUID_70_ECX_VAES_BIT       9
#define CPUID_70_ECX_VPCLMULQDQ_BIT 10


#define CPUID_1_ECX_OSXSAVE_BIT     27

typedef struct _CPUID_BIT_INFO {
    BYTE                    leaf;
    BYTE                    word;
    BYTE                    bitno;
    SYMCRYPT_CPU_FEATURES   requiredBy;
} CPUID_BIT_INFO;

#define WORD_EAX    0
#define WORD_EBX    1
#define WORD_ECX    2
#define WORD_EDX    3

int g_SymCryptCpuid1[4];        // We cache the results of CPUID(1) to help diagnose CPU detection errors

const
CPUID_BIT_INFO  cpuidBitInfo[] = {
    {1, WORD_ECX, CPUID_1_ECX_RDRAND_BIT,       SYMCRYPT_CPU_FEATURE_RDRAND },
    {1, WORD_ECX, CPUID_1_ECX_PCLMULQDQ_BIT,    SYMCRYPT_CPU_FEATURE_PCLMULQDQ },
    {1, WORD_ECX, CPUID_1_ECX_AESNI_BIT,        SYMCRYPT_CPU_FEATURE_AESNI },
    {1, WORD_EDX, CPUID_1_EDX_SSE_BIT,          SYMCRYPT_CPU_FEATURE_SSE2 | SYMCRYPT_CPU_FEATURE_SSSE3 },
    {1, WORD_EDX, CPUID_1_EDX_SSE2_BIT,         SYMCRYPT_CPU_FEATURE_SSE2 | SYMCRYPT_CPU_FEATURE_SSSE3 },
    {1, WORD_ECX, CPUID_1_ECX_SSE3_BIT,         SYMCRYPT_CPU_FEATURE_SSSE3 },
    {1, WORD_ECX, CPUID_1_ECX_SSSE3_BIT,        SYMCRYPT_CPU_FEATURE_SSSE3 },
    {1, WORD_ECX, CPUID_1_ECX_AVX_BIT,          SYMCRYPT_CPU_FEATURE_AVX2 },
    {1, WORD_ECX, CPUID_1_ECX_CMPXCHG16B_BIT,   SYMCRYPT_CPU_FEATURE_CMPXCHG16B },
    {7, WORD_EBX, CPUID_70_EBX_AVX2_BIT,        SYMCRYPT_CPU_FEATURE_AVX2 },
    {7, WORD_EBX, CPUID_70_EBX_RDSEED_BIT,      SYMCRYPT_CPU_FEATURE_RDSEED },
    {7, WORD_EBX, CPUID_70_EBX_SHANI_BIT,       SYMCRYPT_CPU_FEATURE_SHANI },
    {7, WORD_EBX, CPUID_70_EBX_ADX_BIT,         SYMCRYPT_CPU_FEATURE_ADX },
    {7, WORD_EBX, CPUID_70_EBX_BMI2_BIT,        SYMCRYPT_CPU_FEATURE_BMI2 },
    {7, WORD_EBX, CPUID_70_EBX_AVX512F_BIT,     SYMCRYPT_CPU_FEATURE_AVX512 },
    {7, WORD_EBX, CPUID_70_EBX_AVX512VL_BIT,    SYMCRYPT_CPU_FEATURE_AVX512 },
    {7, WORD_EBX, CPUID_70_EBX_AVX512BW_BIT,    SYMCRYPT_CPU_FEATURE_AVX512 },
    {7, WORD_EBX, CPUID_70_EBX_AVX512DQ_BIT,    SYMCRYPT_CPU_FEATURE_AVX512 },
    {7, WORD_ECX, CPUID_70_ECX_VAES_BIT,        SYMCRYPT_CPU_FEATURE_VAES },
    {7, WORD_ECX, CPUID_70_ECX_VPCLMULQDQ_BIT,  SYMCRYPT_CPU_FEATURE_VAES },
};

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesByCpuid( UINT32 flags )
{
    UINT32   result;
    int     CPUInfo[4];
    int     InfoType;
    int     maxInfoType;
    int     i;
    BOOLEAN allowYmm, allowZmm;
    INT64 xGetBvResult;

    //
    // Mark all features as present (the result bits indicate not-present, so set the features we know to 0).
    //
    result = ~ (UINT32)(
        SYMCRYPT_CPU_FEATURE_SSE2       |
        SYMCRYPT_CPU_FEATURE_SSSE3      |
        SYMCRYPT_CPU_FEATURE_AESNI      |
        SYMCRYPT_CPU_FEATURE_PCLMULQDQ  |
        SYMCRYPT_CPU_FEATURE_AVX2       |
        SYMCRYPT_CPU_FEATURE_SHANI      |
        SYMCRYPT_CPU_FEATURE_BMI2       |
        SYMCRYPT_CPU_FEATURE_ADX        |
        SYMCRYPT_CPU_FEATURE_RDRAND     |
        SYMCRYPT_CPU_FEATURE_RDSEED     |
        SYMCRYPT_CPU_FEATURE_AVX512     |
        SYMCRYPT_CPU_FEATURE_VAES       |
        SYMCRYPT_CPU_FEATURE_CMPXCHG16B
        );

    // InfoType holds the function id of previous cpuid
    // so we don't have to repeatedly invoke cpuid.
    InfoType = 0;
    SymCryptCpuidExFunc( CPUInfo, InfoType, 0 );
    maxInfoType = CPUInfo[WORD_EAX];

    for( i=0; i< sizeof( cpuidBitInfo ) / sizeof( *cpuidBitInfo ); i++ )
    {
        if( cpuidBitInfo[i].leaf != InfoType )
        {
            InfoType = cpuidBitInfo[i].leaf;
            SymCryptCpuidExFunc( CPUInfo, InfoType, 0 );
        }
        if( cpuidBitInfo[i].leaf > maxInfoType || (CPUInfo[ cpuidBitInfo[i].word ] & (1UL << cpuidBitInfo[i].bitno) ) == 0 )
        {
            result |= cpuidBitInfo[i].requiredBy;
        }
    }

    if( (flags & SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM) != 0 )
    {
        //
        // Check for OS support of the YMM registers.
        // This detection is optional in any environment because some environments are single-threaded, and
        // OS support is not required. (E.g. Boot library.)
        //
        // We use the following logic:
        // Check that the OSXSAVE bit is 1, which means we can use XGETBV
        // Use XGETBV and check that XCR0[2:1] = '11b' signaling that both XMM and YMM are enabled by OS
        // Note that we only disable the AVX2 usage; AESNI & XMM registers are used independent of OS support, because
        // all our (known) OSes have it.
        //
        allowYmm = FALSE;
        allowZmm = FALSE;
        SymCryptCpuidExFunc( CPUInfo, 1, 0 );

        if( (CPUInfo[WORD_ECX] & (1 << CPUID_1_ECX_OSXSAVE_BIT)) != 0 )
        {
            // OSXSAVE bit is set, we can use XGETBV
            xGetBvResult = _xgetbv( _XCR_XFEATURE_ENABLED_MASK );

            // Check that bits 1 and 2 are set, corresponding to the XMM and YMM register state
            if( (xGetBvResult & 0x6) == 0x6)
            {
                allowYmm = TRUE;

                //
                // For AVX-512, also check that bits 5, 6, and 7 are set, corresponding to the
                // opmask, ZMM (0-15), and ZMM (16-31) register states
                // This follows the recommendation in the Intel 64 and IA-32 Architectures Software
                // Developer's Manual, Volume 1, 15.3 / 15.4.
                //
                // It seems plausible that on some system the OS would not support save/restore of
                // AVX-512 state, but use of AVX-512VL instructions on Ymm or Xmm registers would be
                // OK, however Intel explicitly suggests that we should only use AVX512-VL if the
                // support is indicated by xgetbv, so we use the same logic as for AVX2 (our
                // SymCrypt feature indicates both CPU support, and OS support for saving/restoring
                // the extended state)
                //
                if( (xGetBvResult & 0xe0) == 0xe0)
                {
                    allowZmm = TRUE;
                }
            }
        }

        if( !allowYmm )
        {
            // Disallow the AVX2-dependent code because we don't have OS YMM support.
            result |= SYMCRYPT_CPU_FEATURE_AVX2;
        }

        if( !allowZmm )
        {
            // Disallow any AVX512-dependent code because we don't have OS ZMM support.
            // Note that not all AVX-512 dependent code will need to save/restore ZMM state, but we
            // do not support AVX-512 instructions (even acting on YMM or XMM registers), unless the
            // OS indicates support via XCR0
            result |= SYMCRYPT_CPU_FEATURE_AVX512;
        }
    }


    if( (result & SYMCRYPT_CPU_FEATURE_AESNI) == 0 )    // thus, if AES-NI is present according to CPUID
    {
        //
        // In Win7 Beta we had an interesting crash bucket.
        // It only occurred on the AsusTek A6K line of laptops which sometimes
        // set the cpuid AES-NI bit (but not always). This leads to a crash as
        // we start using AES instructions that don't exist on those machines.
        //
        // I found on-line reviews for the A6K line from december 2005 so it was launched around
        // that time.
        //
        // These laptops all have AMD CPUs, so we fix it by locking out the particular AMD CPUs
        // families that don't have AES-NI anyway.
        //
        // We really shouldn't need this logic, and it only slows things down.
        // We should be able to remove it at some point in the future.
        //
        // At AMD's recommendation, we use the logic below.
        // The AMD engineers reviewed this code to ensure we don't lock out future CPUs
        // that will have AES-NI.
        //
        SymCryptCpuidExFunc( CPUInfo, 0, 0 );
        if(    CPUInfo[WORD_EBX] == 'htuA'
            && CPUInfo[WORD_ECX] == 'DMAc'
            && CPUInfo[WORD_EDX] == 'itne' )
        {
            //
            // We have an AMD cpu, check the family.
            //
            UINT32 baseFamily;
            UINT32 extFamily;
            UINT32 family;

            //
            // Extract the base family and extended family values, and combine them to the full
            // family value.
            //
            SymCryptCpuidExFunc( CPUInfo, 1, 0 );

            baseFamily = (CPUInfo[WORD_EAX] >> 8) & 0xf;

            extFamily = (CPUInfo[WORD_EAX] >> 20) & 0xff;

            if( baseFamily < 0xf )
            {
                family = baseFamily;
            } else {
                family = baseFamily + extFamily;
            }

            //
            // AMD will not implement the AES instruction set until family 0x15
            //
            if( family < 0x15 )
            {
                result |= SYMCRYPT_CPU_FEATURE_AESNI;
            }
        }
    }

    SymCryptCpuidExFunc( g_SymCryptCpuid1, 1, 0 );             // Keep cache of CPUID results for diagnosis

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) result;
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#elif SYMCRYPT_CPU_ARM

#define CP15_ISAR5             15, 0,  0,  2, 5         // Instruction Set Attribute Register 5

#define READ_ARM_FEATURE(_FeatureRegister, _Index) \
        (((ULONG)_MoveFromCoprocessor(_FeatureRegister) >> ((_Index) * 4)) & 0xF)

#define ISAR5_AES              1
#define ISAR5_AES_AESE         1
#define ISAR5_AES_PMULL        2

#define ISAR5_SHA2             3
#define ISAR5_SHA2_SHA256H     1

#define ISAR5_CRC32            4
#define ISAR5_CRC32_IMP        1

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromRegisters(void)
{
    UINT32 result;

#if 0   // We currently do not use any neon crypto features on ARM code, so no detection needed.

    //
    // We start with a result that allows everything.
    // This makes the code simpler when you have one CPU feature flag that disables multiple feature bits.
    //
    result = ~ (UINT32)(
        SYMCRYPT_CPU_FEATURE_NEON           |
        SYMCRYPT_CPU_FEATURE_NEON_AES       |
        SYMCRYPT_CPU_FEATURE_NEON_PMULL     |
        SYMCRYPT_CPU_FEATURE_NEON_SHA256
        );

    //
    // Reading the status registers might fail, so we use a try block.
    //
    try {

        if( READ_ARM_FEATURE(CP15_ISAR5, ISAR5_AES) < ISAR5_AES_AESE )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_AES;
        }

        if( READ_ARM_FEATURE(CP15_ISAR5, ISAR5_AES) < ISAR5_AES_PMULL )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_PMULL;
        }

        if( READ_ARM_FEATURE(CP15_ISAR5, ISAR5_SHA2) < ISAR5_SHA2_SHA256H )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_SHA256;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // Something went wrong reading the registers; disable all the crypto extensions leaving only the standard NEON registers available.
        //
        result |= SYMCRYPT_CPU_FEATURE_NEON_AES | SYMCRYPT_CPU_FEATURE_NEON_PMULL | SYMCRYPT_CPU_FEATURE_NEON_SHA256;
    }
#endif
    //
    // For now we ignore the new instructions in ARM until we can get clarity on how to detect Arm32-on-Arm64.
    //
    result = ~(UINT32)SYMCRYPT_CPU_FEATURE_NEON;

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) result;
}


#elif SYMCRYPT_CPU_ARM64 && !SYMCRYPT_PLATFORM_WINE


#define ARM64_SYSREG(op0, op1, crn, crm, op2) \
        ( ((op0 & 1) << 14) | \
          ((op1 & 7) << 11) | \
          ((crn & 15) << 7) | \
          ((crm & 15) << 3) | \
          ((op2 & 7) << 0) )

#define ARM64_ID_AA64ISAR0_EL1  ARM64_SYSREG(3,0, 0, 6,0)   // ISA Feature Register 0

#define ISAR0_AES                   1
#define ISAR0_AES_NI                0
#define ISAR0_AES_INSTRUCTIONS      1
#define ISAR0_AES_PLUS_PMULL64      2

#define ISAR0_SHA2                  3
#define ISAR0_SHA2_NI               0
#define ISAR0_SHA2_INSTRUCTIONS     1

#define ISAR0_CRC32                 4
#define ISAR0_CRC32_NI              0
#define ISAR0_CRC32_INSTRUCTIONS    1

#define READ_ARM64_FEATURE(_FeatureRegister, _Index) \
        (((ULONG64)_ReadStatusReg(_FeatureRegister) >> ((_Index) * 4)) & 0xF)

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromRegisters(void)
{
    UINT32 result;

    result = ~ (UINT32)(
        SYMCRYPT_CPU_FEATURE_NEON           |
        SYMCRYPT_CPU_FEATURE_NEON_AES       |
        SYMCRYPT_CPU_FEATURE_NEON_PMULL     |
        SYMCRYPT_CPU_FEATURE_NEON_SHA256
        );

#if SYMCRYPT_MS_VC
    __try {

        if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_AES) < ISAR0_AES_INSTRUCTIONS )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_AES;
        }

        if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_AES) < ISAR0_AES_PLUS_PMULL64 )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_PMULL;
        }

        if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_SHA2) < ISAR0_SHA2_INSTRUCTIONS )
        {
            result |= SYMCRYPT_CPU_FEATURE_NEON_SHA256;
        }

        g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) result;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ; //NOTHING;
    }
#endif

}

#endif // CPU arch selection
