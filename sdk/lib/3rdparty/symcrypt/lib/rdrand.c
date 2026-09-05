//
// rdrand.c  Support for RdRand instruction
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#if (SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64)  // only available on x86 and amd64 architectures

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("rdrnd"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("rdrnd")
#endif

#if SYMCRYPT_MS_VC && _MSC_VER < 1610
#error MSVC version lacks support for RDRAND intrinsics. Compile for the generic environment instead.
#endif

//
// TODO: the _rdrand_u*() versions of the intrinsics can be removed once the new compiler
// with the _rdrand*_step() intrinsics is used in all branches

#if SYMCRYPT_MS_VC && _MSC_VER < 1700				// 1700 = Dev11,

//
// This is the code that uses the old intrinsics in the compiler version 16.1
//

unsigned int _rdrand_u32(void);
unsigned __int64 _rdrand_u64(void);


#if SYMCRYPT_CPU_X86
#define SymCryptRdrandSizet(p) ( *(p)=(SIZE_T)_rdrand_u32(), SYMCRYPT_NO_ERROR )
#else
#define SymCryptRdrandSizet(p) ( *(p)=(SIZE_T)_rdrand_u64(), SYMCRYPT_NO_ERROR )
#endif

#else	// _MSC_VER

//
// Code for the new Dev11 intrinsics
//

#if SYMCRYPT_CPU_X86
#define _rdrandxx_step(_p) _rdrand32_step( (unsigned int *) (_p) )
#else
#define _rdrandxx_step(_p) _rdrand64_step( (UINT64  *) (_p) )
#endif

FORCEINLINE
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdrandSizet( SIZE_T * p )
{
	int i;

    //
    // In Win8/WinBlue we iterated 1000 times.
    // But we got a crash bucket where we fail because of
    // not getting any random data.
    // I contacted the Intel people; according to them they cannot make the
    // RDRAND instruction fail more than a dozen times in a row under any tested
    // circumstance. They have no idea how it could fail 1000 times in a row.
    // As a failure of this code leads to a bugcheck (it fails a security promise, and
    // is therefore treated as a critical security bug) I have increased the
    // iteration count to 1000000.
    // This will not affect any machine that didn't bugcheck before, but it hopefully
    // will remove some of the current bugchecks.
    //
    // Niels Ferguson (niels) 2014-04-09.
    //
	for( i=0; i<1000000; i++ )
	{
		if( _rdrandxx_step( p ) != 0 )
		{
			return SYMCRYPT_NO_ERROR;
		}
	}
	return SYMCRYPT_HARDWARE_FAILURE;
}

#endif


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdrandStatus(void)
{
    //
    // Check that the library is initialized; otherwise the CPUID info
    // is all zeroes. (This check only happens in CHKed builds.)
    //
    SymCryptCheckLibraryInitialized();

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_RDRAND ) )
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
SymCryptRdrandGetBytes(
    _Out_writes_( cbBuffer )                    PBYTE   pbBuffer,
                                                SIZE_T  cbBuffer,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE   pbResult )
{
    SIZE_T * pBuf;
    SIZE_T nBuf;
    SIZE_T i;
	SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    //
    // Take care of the obvious errors that can happen
    //
    if( SymCryptRdrandStatus() != SYMCRYPT_NO_ERROR ||
        (cbBuffer & 0xf) != 0
      )
    {
        SymCryptFatal( 'rdrn' );
    }

    pBuf = (SIZE_T *) pbBuffer;
    nBuf = cbBuffer / sizeof( SIZE_T );

    for( i=0; i<nBuf; i++ )
    {
		scError = SymCryptRdrandSizet( &pBuf[i] );
		if( scError != SYMCRYPT_NO_ERROR )
		{
			goto cleanup;
		}
    }

    SymCryptSha512( pbBuffer, cbBuffer, pbResult );

cleanup:
    SymCryptWipe( pbBuffer, cbBuffer );

	return scError;
}


VOID
SYMCRYPT_CALL
SymCryptRdrandGet(
    _Out_writes_( cbBuffer )                    PBYTE   pbBuffer,
                                                SIZE_T  cbBuffer,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE   pbResult )
{
	if( SymCryptRdrandGetBytes( pbBuffer, cbBuffer, pbResult ) != SYMCRYPT_NO_ERROR )
	{
		SymCryptFatal( 'rdrx' );
	}
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif // SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
