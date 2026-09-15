//
// libmain.c
// General routines for the SymCrypt library
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#include "C_asm_shared.inc"

#include "buildInfo.h"

// The following global g_SymCryptFlags has to be at least 32
// bits because the iOS environment has interlocked function
// support for variables of size at least 32 bits.
// The relevant function is OSAtomicOr32Barrier.
UINT32 g_SymCryptFlags = 0;

SYMCRYPT_CPU_FEATURES g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~0;
SYMCRYPT_CPU_FEATURES g_SymCryptCpuFeaturesPresentCheck = 0;

#if SYMCRYPT_DEBUG

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptLibraryWasNotInitialized(void)
{
    SymCryptFatal( 'init' );    // Function name helps figure out what the problem is.
}

#endif

const CHAR * const SymCryptBuildString =
        "v" SYMCRYPT_BUILD_INFO_VERSION
        "_" SYMCRYPT_BUILD_INFO_BRANCH
        "_" SYMCRYPT_BUILD_INFO_COMMIT
        "_" SYMCRYPT_BUILD_INFO_TIMESTAMP;

VOID
SYMCRYPT_CALL
SymCryptInitEnvCommon( UINT32 version )
// Returns TRUE if the initialization steps have to be performed.
{
    UINT32 tmp;

    const CHAR * p;

    // Assertion that verifies that the calling application was compiled with
    // the same version header files as the library.
    if( version != SYMCRYPT_API_VERSION )
    {
        SymCryptFatal( 'apiv' );
    }

    //
    // Use an interlocked to set the flag in case we add other flags
    // that are modified by different threads.
    //
    SYMCRYPT_ATOMIC_OR32_PRE_RELAXED( &g_SymCryptFlags, SYMCRYPT_FLAG_LIB_INITIALIZED );

    //
    // Do a forced write of our code version. This ensures that the code
    // version is part of the binary, so we can look at a binary and figure
    // out which version of SymCrypt it was linked with.
    //
    SYMCRYPT_FORCE_WRITE32( &tmp, SYMCRYPT_API_VERSION );

    //
    // Force the build string to be in memory, because otherwise the
    // compiler might get smart and remove it.
    // This ensures we can always track back to the SymCrypt source code from
    // any binary that links this library
    //
    for( p = SymCryptBuildString; *p!=0; p++ )
    {
        SYMCRYPT_FORCE_WRITE8( (PBYTE) &tmp, *p );
    }

    //
    // Make an inverted copy of the CPU detection results.
    // This helps us diagnose corruption of our flags
    // Force-write otherwise the compiler optimizes it away
    //
    SYMCRYPT_FORCE_WRITE32( &g_SymCryptCpuFeaturesPresentCheck, ~g_SymCryptCpuFeaturesNotPresent );

    //
    // Test that the C and assembler code agree on the various structure member offsets.
    // This gets optimized away in FRE builds as all the values are compile-time computable.
    //
#define SYMCRYPT_CHECK_ASM_OFFSET( a, b ) if( (a) != (b) ) {SymCryptFatal( b );}
    SYMCRYPT_CHECK_ASM_OFFSETS;
#undef SYMCRYPT_CHECK_ASM_OFFSET
}

_Analysis_noreturn_
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptFatalHang( UINT32 fatalCode )
//
// This function is used by the environment-specific fatal code
// as a last resort when none of the other fatal methods work.
//
{
    UINT32   fcode;

    //
    // Put the fatal code in a location we can find
    //
    SYMCRYPT_FORCE_WRITE32( &fcode, fatalCode );

fatalInfiniteLoop:
    goto fatalInfiniteLoop;
}

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64

VOID
SYMCRYPT_CALL
SymCryptWipeAsm( _Out_writes_bytes_( cbData ) PVOID pbData, SIZE_T cbData );

VOID
SYMCRYPT_CALL
SymCryptWipe( _Out_writes_bytes_( cbData ) PVOID pbData, SIZE_T cbData )
{
    SymCryptWipeAsm( pbData, cbData );
}

#else
//
// Generic but slow wipe routine.
//
VOID
SYMCRYPT_CALL
SymCryptWipe( _Out_writes_bytes_( cbData ) PVOID pbData, SIZE_T cbData )
{
    volatile BYTE * p = (volatile BYTE *) pbData;
    SIZE_T i;

    for( i=0; i<cbData; i++ ){
        p[i] = 0;
    }

}
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM
VOID
SYMCRYPT_CALL
SymCryptXorBytes(
    _In_reads_( cbBytes )   PCBYTE  pbSrc1,
    _In_reads_( cbBytes )   PCBYTE  pbSrc2,
    _Out_writes_( cbBytes ) PBYTE   pbResult,
                            SIZE_T  cbBytes )
{
    SIZE_T i;

    if( cbBytes == 16 )
    {
        PCUINT32 s1 = (PCUINT32) pbSrc1;
        PCUINT32 s2 = (PCUINT32) pbSrc2;
        PUINT32 d = (PUINT32) pbResult;

        d[0] = s1[0] ^ s2[0];
        d[1] = s1[1] ^ s2[1];
        d[2] = s1[2] ^ s2[2];
        d[3] = s1[3] ^ s2[3];
    }
    else
    {
        i = 0;
        while( i + 3 < cbBytes )
        {
            *(UINT32 *)&pbResult[i] = *(UINT32 *)&pbSrc1[i] ^ *(UINT32 *)&pbSrc2[i];
            i += 4;
        }

        while( i < cbBytes )
        {
            pbResult[i] = pbSrc1[i] ^ pbSrc2[i];
            i++;
        }
    }
}

#elif SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64

VOID
SYMCRYPT_CALL
SymCryptXorBytes(
    _In_reads_( cbBytes )   PCBYTE  pbSrc1,
    _In_reads_( cbBytes )   PCBYTE  pbSrc2,
    _Out_writes_( cbBytes ) PBYTE   pbResult,
                            SIZE_T  cbBytes )
{
    if( cbBytes == 16 )
    {
        PCUINT64 s1 = (PCUINT64) pbSrc1;
        PCUINT64 s2 = (PCUINT64) pbSrc2;
        PUINT64  d = (PUINT64) pbResult;

        d[0] = s1[0] ^ s2[0];
        d[1] = s1[1] ^ s2[1];
    }
    else
    {
        while( cbBytes >= 8 )
        {
            *(UINT64 *)pbResult = *(UINT64 *)pbSrc1 ^ *(UINT64 *)pbSrc2;
            pbSrc1 += 8;
            pbSrc2 += 8;
            pbResult += 8;
            cbBytes -= 8;
        }

        while( cbBytes > 0 )
        {
            *pbResult = *pbSrc1 ^ *pbSrc2;
            pbResult++;
            pbSrc1++;
            pbSrc2++;
            cbBytes--;
        }
    }
}


#else
//
// Generic code
//
VOID
SYMCRYPT_CALL
SymCryptXorBytes(
    _In_reads_( cbBytes )   PCBYTE  pbSrc1,
    _In_reads_( cbBytes )   PCBYTE  pbSrc2,
    _Out_writes_( cbBytes ) PBYTE   pbResult,
                            SIZE_T  cbBytes )
{
    SIZE_T i;

    for( i=0; i<cbBytes; i++ )
    {
        pbResult[i] = pbSrc1[i] ^ pbSrc2[i];
    }
}
#endif


//
// Generic LSB/MSBfirst load/store code for variable-sized buffers.
// These implementations are inefficient and not side-channel safe.
// This is sufficient for the current usage (typically to allow
// callers to read/write RSA public exponents from/to variable-sized
// buffers).
// Consider upgrading them in future.
//

UINT32
SymCryptUint32Bitsize( UINT32 value )
//
// Some CPUs/compilers have intrinsics for this,
// but this is portable and works everywhere.
//
{
    UINT32 res;

    res = 0;
    while( value != 0 )
    {
        res += 1;
        value >>= 1;
    }

    return res;
}

UINT32
SymCryptUint64Bitsize( UINT64 value )
{
    UINT32 res;
    UINT32 upper;

    upper = (UINT32)(value >> 32);

    if( upper == 0 )
    {
        res = SymCryptUint32Bitsize( (UINT32) value );
    } else {
        res = 32 + SymCryptUint32Bitsize( upper );
    }

    return res;
}

UINT32
SymCryptUint32Bytesize( UINT32 value )
{
    if( value == 0 )
    {
        return 0;
    }
    if( value < 0x100 )
    {
        return 1;
    }
    if( value < 0x10000 )
    {
        return 2;
    }
    if( value < 0x1000000 )
    {
        return 3;
    }
    return 4;
}

UINT32
SymCryptUint64Bytesize( UINT64 value )
{
    UINT32 res;
    UINT32 upper;

    upper = (UINT32)(value >> 32);

    if( upper == 0 )
    {
        res = SymCryptUint32Bytesize( (UINT32) value );
    } else {
        res = 4 + SymCryptUint32Bytesize( upper );
    }

    return res;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadLsbFirstUint32(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT32 pDst )
{
    UINT64 v64;
    UINT32 v32;
    SYMCRYPT_ERROR scError;

    scError = SymCryptLoadLsbFirstUint64( pbSrc, cbSrc, &v64 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    v32 = (UINT32) v64;
    if( v32 != v64 )
    {
        scError = SYMCRYPT_VALUE_TOO_LARGE;
        goto cleanup;
    }

    *pDst = v32;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadLsbFirstUint64(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT64 pDst )
{
    UINT64 v;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    v = 0;
    pbSrc += cbSrc;
    while( cbSrc > 8 )
    {
        if( *--pbSrc != 0 )
        {
            scError = SYMCRYPT_VALUE_TOO_LARGE;
            goto cleanup;
        }
        cbSrc--;
    }

    while( cbSrc > 0 )
    {
        v = (v << 8) | *--pbSrc;
        cbSrc--;
    }

    *pDst = v;

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadMsbFirstUint32(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT32 pDst )
{
    UINT64 v64;
    UINT32 v32;
    SYMCRYPT_ERROR scError;

    scError = SymCryptLoadMsbFirstUint64( pbSrc, cbSrc, &v64 );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    v32 = (UINT32) v64;
    if( v32 != v64 )
    {
        scError = SYMCRYPT_VALUE_TOO_LARGE;
        goto cleanup;
    }

    *pDst = v32;

cleanup:
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadMsbFirstUint64(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT64 pDst )
{
    UINT64 v;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    v = 0;
    while( cbSrc > 8 )
    {
        if( *pbSrc++ != 0 )
        {
            scError = SYMCRYPT_VALUE_TOO_LARGE;
            goto cleanup;
        }
        cbSrc--;
    }

    while( cbSrc > 0 )
    {
        v = (v << 8) | *pbSrc++;
        cbSrc--;
    }

    *pDst = v;

cleanup:
    return scError;
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreLsbFirstUint32(
                            UINT32  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst )
{
    return SymCryptStoreLsbFirstUint64( src, pbDst, cbDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreLsbFirstUint64(
                            UINT64  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    while( cbDst > 0 )
    {
        *pbDst++ = (BYTE) src;
        src >>= 8;
        cbDst--;
    }

    if( src != 0 )
    {
        scError = SYMCRYPT_VALUE_TOO_LARGE;
        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreMsbFirstUint32(
                            UINT32  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst )
{
    return SymCryptStoreMsbFirstUint64( src, pbDst, cbDst );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreMsbFirstUint64(
                            UINT64  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    pbDst += cbDst;
    while( cbDst > 0 )
    {
        *--pbDst = (BYTE) src;
        src >>= 8;
        cbDst--;
    }

    if( src != 0 )
    {
        scError = SYMCRYPT_VALUE_TOO_LARGE;
        goto cleanup;
    }

cleanup:
    return scError;
}
