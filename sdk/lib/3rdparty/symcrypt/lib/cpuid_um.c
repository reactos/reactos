//
// cpuid_um.c   code for CPU feature detection based on OS features available in user-mode
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// This file contains the feature detection code that is only compiled for user-mode.
// The IsProcessorFeaturePresent API is only in UM, so linking any code out of
// a source file that contains a call to it doesn't work for KM code.
// By splitting it into a separate file, the code is ignored by KM callers because
// they never reference anything in this file.
//



#include "precomp.h"

#if SYMCRYPT_CPU_ARM64 && SYMCRYPT_PLATFORM_WINDOWS
#undef UNREFERENCED_PARAMETER
#include <processthreadsapi.h>

// From winnt.h
#define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE 30

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent(void)
{
    if( IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) )
    {
        g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~(
            SYMCRYPT_CPU_FEATURE_NEON           |
            SYMCRYPT_CPU_FEATURE_NEON_AES       |
            SYMCRYPT_CPU_FEATURE_NEON_PMULL     |
            SYMCRYPT_CPU_FEATURE_NEON_SHA256
            );
    } else {
        g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~SYMCRYPT_CPU_FEATURE_NEON;
    }
}

#elif SYMCRYPT_CPU_ARM64 && SYMCRYPT_GNUC

#if SYMCRYPT_PLATFORM_APPLE
#include <sys/sysctl.h>

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent(void)
{
    // Arm64 code relies on presence of ASIMD everywhere (it is always present with Armv8); the
    // compiler is permitted to generate ASIMD instructions anywhere
    // The SYMCRYPT_CPU_FEATURE_NEON is currently always present and never checked
    SYMCRYPT_CPU_FEATURES result = ~SYMCRYPT_CPU_FEATURE_NEON;

    // On macOS ARM64, we use sysctl to query CPU features
    // All Apple Silicon Macs support AES, PMULL, and SHA2 instructions
    uint32_t has_feature = 0;
    size_t len = sizeof(has_feature);

    // Check for AES support
    if( sysctlbyname("hw.optional.arm.FEAT_AES", &has_feature, &len, NULL, 0) == 0 && has_feature )
    {
        result &= ~SYMCRYPT_CPU_FEATURE_NEON_AES;
    }

    // Check for PMULL support
    has_feature = 0;
    len = sizeof(has_feature);
    if( sysctlbyname("hw.optional.arm.FEAT_PMULL", &has_feature, &len, NULL, 0) == 0 && has_feature )
    {
        result &= ~SYMCRYPT_CPU_FEATURE_NEON_PMULL;
    }

    // Check for SHA2 support
    has_feature = 0;
    len = sizeof(has_feature);
    if( sysctlbyname("hw.optional.arm.FEAT_SHA256", &has_feature, &len, NULL, 0) == 0 && has_feature )
    {
        result &= ~SYMCRYPT_CPU_FEATURE_NEON_SHA256;
    }

    g_SymCryptCpuFeaturesNotPresent = result;
}

#else // Linux and other Unix platforms

#include <sys/auxv.h>

// #include <asm/hwcap.h>
#define HWCAP_AES   (1 << 3)
#define HWCAP_PMULL (1 << 4)
#define HWCAP_SHA2  (1 << 6)

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent(void)
{
    unsigned long hwcaps = getauxval( AT_HWCAP );

    SYMCRYPT_CPU_FEATURES result = ~(
        SYMCRYPT_CPU_FEATURE_NEON           |
        SYMCRYPT_CPU_FEATURE_NEON_AES       |
        SYMCRYPT_CPU_FEATURE_NEON_PMULL     |
        SYMCRYPT_CPU_FEATURE_NEON_SHA256
        );

    // Arm64 code relies on presence of ASIMD everywhere (it is always present with Armv8); the
    // compiler is permitted to generate ASIMD instructions anywhere
    // The SYMCRYPT_CPU_FEATURE_NEON is currently always present and never checked

    if( !(hwcaps & HWCAP_AES) )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_AES;
    }

    if( !(hwcaps & HWCAP_PMULL) )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_PMULL;
    }

    if( !(hwcaps & HWCAP_SHA2) )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_SHA256;
    }

    g_SymCryptCpuFeaturesNotPresent = result;
}

#endif // SYMCRYPT_PLATFORM_APPLE

#endif
