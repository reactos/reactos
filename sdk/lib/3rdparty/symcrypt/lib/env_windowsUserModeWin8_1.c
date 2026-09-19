//
// env_windowsUserMode.c
// Platform-specific code for windows user mode.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//#include "precomp.h"

#pragma warning(push)
#pragma warning(disable: 5103) // Arm64's wdm.h included below currently generate a lot of 5103 warnings
#include <windows.h>
#pragma warning(pop)
#include "symcrypt.h"
#include "sc_lib.h"

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWindowsUsermodeWin8_1nLater()
{
    return 0;
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvWindowsUsermodeWin8_1nLater( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    //
    // First we detect what the CPU has
    //
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    //
    // We also need to be sure that the OS supports the extended registers.
    //
    {
        ULONGLONG FeatureMask = GetEnabledXStateFeatures();

        if( !(FeatureMask & XSTATE_MASK_AVX) )
        {
            g_SymCryptCpuFeaturesNotPresent |= SYMCRYPT_CPU_FEATURE_AVX2;
        }

        if( !(FeatureMask & XSTATE_MASK_AVX512) )
        {
            g_SymCryptCpuFeaturesNotPresent |= SYMCRYPT_CPU_FEATURE_AVX512;
        }
    }

    //
    // Our SaveXmm function never fails because it doesn't have to do anything in User mode.
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~SYMCRYPT_CPU_FEATURE_NEON;

#elif SYMCRYPT_CPU_ARM64

    SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent();

#endif

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalEnvWindowsUsermodeWin8_1nLater( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Our first preference is to fastfail,
    // the second to create an AV, which triggers a Watson report so that we get to
    // see what is going wrong.
    //
    __fastfail( FAST_FAIL_CRYPTO_LIBRARY );

    //
    // Next we write to the NULL pointer, this causes an AV
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    //
    // If that fails, we terminate the process. (This function call also ensures that this environment is actually
    // used in user mode and not some other environment.)
    // (During testing we had the TerminateProcess as the first option, but that makes debugging very hard as
    // it leaves no traces of what went wrong.)
    //
    TerminateProcess( GetCurrentProcess(), fatalCode );

    SymCryptFatalHang( fatalCode );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvWindowsUsermodeWin8_1nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvWindowsUsermodeWin8_1nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvWindowsUsermodeWin8_1nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvWindowsUsermodeWin8_1nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID
SYMCRYPT_CALL
SymCryptTestInjectErrorEnvWindowsUsermodeWin8_1nLater( PBYTE pbBuf, SIZE_T cbBuf )
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvWindowsUsermodeWin8_1nLater( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
