//
// rdseed.c  Support for RdSeed instruction
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#if (SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64) // only available on x86 and amd64 architectures.

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("rdseed"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("rdseed")
#endif

#if SYMCRYPT_MS_VC && _MSC_VER < 1610
#error MSVC version lacks support for RDSEED intrinsics. Compile for the generic environment instead.
#endif

//
// Create a definition that works on SIZE_Ts.
//

#if SYMCRYPT_CPU_X86
#define _rdseedxx_step(_p) _rdseed32_step( (unsigned int *) (_p) )
#else
#define _rdseedxx_step(_p) _rdseed64_step( (UINT64  *) (_p) )
#endif

FORCEINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdseedSizet( SIZE_T * p )
{
	int i;

    //
    // There is no way to report errors, and customers rely on the RNG to work properly.
    // Therefore, higher layers will fatal if this function fails.
    // This is why we have a very high retry count; the alternative is to fatal.
    //
    //
	for( i=0; i<10000000; i++ )
	{
		if( _rdseedxx_step( p ) != 0 )
		{
			return SYMCRYPT_NO_ERROR;
		}
	}
	return SYMCRYPT_HARDWARE_FAILURE;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdseedStatus(void)
{
    //
    // Check that the library is initialized; otherwise the CPUID info
    // is all zeroes. (This check only happens in CHKed builds.)
    //
    SymCryptCheckLibraryInitialized();

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_RDSEED ) )
    {
        return SYMCRYPT_NO_ERROR;
    }
    else
    {
        return SYMCRYPT_NOT_IMPLEMENTED;
    }
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdseedGetBytes(
    _Out_writes_( cbResult )                    PBYTE   pbResult,
                                                SIZE_T  cbResult )
{
    SIZE_T * pBuf;
    SIZE_T nBuf;
    SIZE_T i;
	SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    //
    // Take care of the obvious errors that can happen
    //
    if( SymCryptRdseedStatus() != SYMCRYPT_NO_ERROR ||
        (cbResult & 0xf) != 0
      )
    {
        SymCryptFatal( 'rdsd' );
    }

    pBuf = (SIZE_T *) pbResult;
    nBuf = cbResult / sizeof( SIZE_T );

    for( i=0; i<nBuf; i++ )
    {
		scError = SymCryptRdseedSizet( &pBuf[i] );
		if( scError != SYMCRYPT_NO_ERROR )
		{
			goto cleanup;
		}
    }

cleanup:

	return scError;
}


VOID
SYMCRYPT_CALL
SymCryptRdseedGet(
    _Out_writes_( cbResult )                    PBYTE   pbResult,
                                                SIZE_T  cbResult )
{
	if( SymCryptRdseedGetBytes( pbResult, cbResult ) != SYMCRYPT_NO_ERROR )
	{
		SymCryptFatal( 'rdsx' );
	}
}


#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif // SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
