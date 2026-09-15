//
// sc_lib.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Internal definitions for the symcrypt library.
// This include file is used only for the files inside the library, not by
// the code that calls the library.
//


#if SYMCRYPT_MS_VC
#define SYMCRYPT_DISABLE_CFG    __declspec(guard(nocf))
#else
#define SYMCRYPT_DISABLE_CFG
#endif

//
// Global flags
//

#define SYMCRYPT_FLAG_LIB_INITIALIZED   0x00000001

extern UINT32 g_SymCryptFlags;

//==============================================================================================
//  Common environment functions
//==============================================================================================

VOID
SYMCRYPT_CALL
SymCryptInitEnvCommon( UINT32 version );

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalHang( UINT32 fatalcode );

#include <symcrypt_low_level.h>

// Types

typedef int                 BOOL;

#if !defined(TRUE)
#define TRUE  (1)
#endif

#if !defined(FALSE)
#define FALSE (0)
#endif

#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(x)   ((void)x)
#endif

#if !defined(FAST_FAIL_CRYPTO_LIBRARY)
#define FAST_FAIL_CRYPTO_LIBRARY    22
#endif

//
// We want to write some of our code to use the native register size provided by the platform we are using to enable
// generic code to compile into reasonable performant versions on 32b and 64b platforms. Below definitions give us
// this flexibility without relying on compiler specifics.
//
// WARNING: Some use of NATIVE_UINT also relies on the little-endianness of the 64b platform; our generic code normally
// uses UINT32, and at the time of writing mixing UINT32 and NATIVE_UINT will not work on a big-endian 64b platform!
//
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64
typedef INT64               NATIVE_INT;
typedef UINT64              NATIVE_UINT;
#define NATIVE_BITS         (64)
#define NATIVE_BYTES        (8)
#define NATIVE_BYTES_LOG2   (3)
#else
typedef INT32               NATIVE_INT;
typedef UINT32              NATIVE_UINT;
#define NATIVE_BITS         (32)
#define NATIVE_BYTES        (4)
#define NATIVE_BYTES_LOG2   (2)
#endif


//
// Our Wipe code uses FORCE_WRITE* which are implemented using
// WriteNoFence* functions. Unfortunately, they declare their parameter
// to be interlocked, and the compiler complains when we also access the variable
// using non-interlocked code.
// This warning is nonsensical in our situation, so we disable it.
// The second warning is about accessing a local variable via an interlocked ptr.
//
#pragma prefast( disable:28112 )
#pragma prefast( disable:28113 )
#pragma warning( disable: 4702 )        // unreachable code. The compilers are not equally smart, and some complain
                                        // about 'function must return a value' and some about 'unreachable code'
#pragma warning( disable: 4296 )        // expression is always false - this warning is forced to be an error by a
                                        // pragma in the SDK warning.h, but we don't consider it useful


//
// These macros allow a bunch of generic code to be written.
// For example, the Hash append function is written once generically
// using these macros.
//

#define CONCAT_I2( a, b )       a##b
#define CONCAT_I3( a, b, c )    a##b##c


#define CONCAT2( a, b )         CONCAT_I2( a, b )
#define CONCAT3( a, b, c )      CONCAT_I3( a, b, c )
//#define CONCAT4( a, b, c, d)    a##b##c##d



#define SYMCRYPT_XXX_STATE              CONCAT3( SYMCRYPT_, ALG, _STATE )
#define PSYMCRYPT_XXX_STATE             CONCAT3( PSYMCRYPT_, ALG, _STATE )
#define PCSYMCRYPT_XXX_STATE            CONCAT3( PCSYMCRYPT_, ALG, _STATE )

#define SYMCRYPT_Xxx                    CONCAT2( SymCrypt, Alg )

#define SYMCRYPT_XxxStateCopy           CONCAT3( SymCrypt, Alg, StateCopy )
#define SYMCRYPT_XxxInit                CONCAT3( SymCrypt, Alg, Init )
#define SYMCRYPT_XxxAppend              CONCAT3( SymCrypt, Alg, Append )
#define SYMCRYPT_XxxResult              CONCAT3( SymCrypt, Alg, Result )
#define SYMCRYPT_XxxAppendBlocks        CONCAT3( SymCrypt, Alg, AppendBlocks )
#define SYMCRYPT_XxxStateImport         CONCAT3( SymCrypt, Alg, StateImport)
#define SYMCRYPT_XxxStateExport         CONCAT3( SymCrypt, Alg, StateExport)

// for XOFs and KMAC
#define SYMCRYPT_XXX_EXPANDED_KEY       CONCAT3( SYMCRYPT_, ALG, _EXPANDED_KEY )
#define PSYMCRYPT_XXX_EXPANDED_KEY      CONCAT3( PSYMCRYPT_, ALG, _EXPANDED_KEY )
#define PCSYMCRYPT_XXX_EXPANDED_KEY     CONCAT3( PCSYMCRYPT_, ALG, _EXPANDED_KEY )
#define SYMCRYPT_XxxEx                  CONCAT3( SymCrypt, Alg, Ex)
#define SYMCRYPT_XxxDefault             CONCAT3( SymCrypt, Alg, Default )
#define SYMCRYPT_XxxExpandKey           CONCAT3( SymCrypt, Alg, ExpandKey )
#define SYMCRYPT_XxxExpandKeyEx         CONCAT3( SymCrypt, Alg, ExpandKeyEx )
#define SYMCRYPT_XxxExtract             CONCAT3( SymCrypt, Alg, Extract )
#define SYMCRYPT_XxxResultEx            CONCAT3( SymCrypt, Alg, ResultEx )
#define SYMCRYPT_XxxKeyCopy             CONCAT3( SymCrypt, Alg, KeyCopy )

#define SYMCRYPT_HmacXxx                CONCAT2( SymCryptHmac, Alg )
#define SYMCRYPT_HmacXxxStateCopy       CONCAT3( SymCryptHmac, Alg, StateCopy )
#define SYMCRYPT_HmacXxxKeyCopy         CONCAT3( SymCryptHmac, Alg, KeyCopy )
#define SYMCRYPT_HmacXxxExpandKey       CONCAT3( SymCryptHmac, Alg, ExpandKey )
#define SYMCRYPT_HmacXxxInit            CONCAT3( SymCryptHmac, Alg, Init )
#define SYMCRYPT_HmacXxxAppend          CONCAT3( SymCryptHmac, Alg, Append )
#define SYMCRYPT_HmacXxxResult          CONCAT3( SymCryptHmac, Alg, Result )


#define SYMCRYPT_XXX_INPUT_BLOCK_SIZE   CONCAT3( SYMCRYPT_, ALG, _INPUT_BLOCK_SIZE )
#define SYMCRYPT_XXX_RESULT_SIZE        CONCAT3( SYMCRYPT_, ALG, _RESULT_SIZE )

#define SYMCRYPT_HMAC_XXX_INPUT_BLOCK_SIZE  SYMCRYPT_XXX_INPUT_BLOCK_SIZE
#define SYMCRYPT_HMAC_XXX_RESULT_SIZE       SYMCRYPT_XXX_RESULT_SIZE

#define PSYMCRYPT_HMAC_XXX_EXPANDED_KEY     CONCAT3( PSYMCRYPT_HMAC_, ALG, _EXPANDED_KEY )
#define PCSYMCRYPT_HMAC_XXX_EXPANDED_KEY    CONCAT3( PCSYMCRYPT_HMAC_, ALG, _EXPANDED_KEY )
#define SYMCRYPT_HMAC_XXX_STATE             CONCAT3( SYMCRYPT_HMAC_, ALG, _STATE )
#define PSYMCRYPT_HMAC_XXX_STATE            CONCAT3( PSYMCRYPT_HMAC_, ALG, _STATE )
#define PCSYMCRYPT_HMAC_XXX_STATE            CONCAT3( PCSYMCRYPT_HMAC_, ALG, _STATE )


//==============================================================================================
//  PLATFORM SPECIFICS
//==============================================================================================

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

//
// The XMM save/restore functions need to be passed a buffer in which they can store their data.
// We have two different places where we use this, in kernel mode and in user mode (while testing)
// We can't declare a union of the two structs as we can't include the kernel-mode headers in this file
// when compiled for a user-mode app.
// Instead we define a structure with reserved space, and have each environment check the size and
// cast the pointer.
//
// We always use the KeSaveExtendedProcessorState call, and not the KeSaveFloatingPointState as it
// allows us to save only the XMM registers and not touch the X87/MMX registers which should
// save time.
//
#if SYMCRYPT_CPU_X86

//
// The XSTATE_SAVE structure consists of a union between
//  struct:
//      - INT64             8
//      - INT32             4
//      - Pointer           4
//      - Pointer           4
//      - Pointer           4
//      - Pointer           4
//      - BYTE              1 + 3 padding
//                          32 total
// - XSTATE_CONTEXT
//      - UINT64            8
//      - UINT32            4
//      - UINT32            4
//      - Pointer + UINT32  8
//      - Pointer + UINT32  8
//                          32 total
//
// Experimentally: need 4 more bytes, don't know why yet.
// Should have a look with the debugger when I have time.
//

#define SYMCRYPT_XSTATE_SAVE_SIZE    (32)

#elif SYMCRYPT_CPU_AMD64

//
// The XSTATE_SAVE structure consists of
// - pointer            8
// - pointer            8
// - BYTE               1 + 7 padding
// - XSTATE_CONTEXT
//      - UINT64        8
//      - UINT32        4
//      - UINT32        4
//      - Pointer       8
//      - Pointer       8
//
#define SYMCRYPT_XSTATE_SAVE_SIZE    (56)

#endif

typedef
SYMCRYPT_ALIGN
struct _SYMCRYPT_EXTENDED_SAVE_DATA {
    SYMCRYPT_ALIGN  BYTE    data[SYMCRYPT_XSTATE_SAVE_SIZE];
                    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_EXTENDED_SAVE_DATA, *PSYMCRYPT_EXTENDED_SAVE_DATA;


//
// Two functions to save/restore the XMM registers.
// These must ALWAYS be called in pairs, even if the SaveXmm function returned an error.
// XMM registers cannot be used if the save function returned an error.
// If the SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL feature is present, then the
// SymCryptSaveXmm function will never return an error.
//

//
// Functions to save/restore the XMM or YMM registers.
// If the Save*mm function is called and succeeds, then the corresponding
// Restore*mm function MUST be called later on the same thread.
// The extended registers cannot be called if the Save function returns an error.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmm( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData );

VOID
SYMCRYPT_CALL
SymCryptRestoreXmm( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData );


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmm( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData );

VOID
SYMCRYPT_CALL
SymCryptRestoreYmm( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData );
#endif


//==============================================================================================
//  Library declarations
//==============================================================================================

//
// Function to check that the library has been initialized
//
#if SYMCRYPT_DEBUG

VOID
SYMCRYPT_CALL
SymCryptLibraryWasNotInitialized(void);

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptCheckLibraryInitialized(void)
{
    if( !(g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED)  )
    {
        SymCryptLibraryWasNotInitialized();
    }
}
#else
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptCheckLibraryInitialized(void)
{
}
#endif

#define HMAC_IPAD_BYTE   0x36
#define HMAC_OPAD_BYTE   0x5c

// SYMCRYPT_CPU_FEATURES
#define SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE  (SYMCRYPT_CPU_FEATURE_PCLMULQDQ | SYMCRYPT_CPU_FEATURE_SSSE3 | SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL )

#define SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE (SYMCRYPT_CPU_FEATURE_SSSE3 | SYMCRYPT_CPU_FEATURE_AESNI)
#define SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE (SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE | SYMCRYPT_CPU_FEATURES_FOR_PCLMULQDQ_CODE)
#define SYMCRYPT_CPU_FEATURES_FOR_VAES_256_CODE (SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE | SYMCRYPT_CPU_FEATURE_AVX2 | SYMCRYPT_CPU_FEATURE_VAES)
#define SYMCRYPT_CPU_FEATURES_FOR_VAES_512_CODE (SYMCRYPT_CPU_FEATURES_FOR_AESNI_CODE | SYMCRYPT_CPU_FEATURE_AVX512 | SYMCRYPT_CPU_FEATURE_VAES)

#define SYMCRYPT_CPU_FEATURES_FOR_SHANI_CODE (SYMCRYPT_CPU_FEATURE_SSSE3 | SYMCRYPT_CPU_FEATURE_SHANI)

#define SYMCRYPT_CPU_FEATURES_FOR_MULX (SYMCRYPT_CPU_FEATURE_BMI2 | SYMCRYPT_CPU_FEATURE_ADX | SYMCRYPT_CPU_FEATURE_SSE2 )

//
// ROTATE OPERATIONS
//
//
// If this lib is ever ported to a platform that doesn't have the _rotx functions
// the macros can be replaced by portable definitions just like the ROL16/ROR16
//

#define ROL16( x, n ) ((UINT16)( ( ((x) << (n)) | ((x) >> (16-(n))) ) ))
#define ROR16( x, n ) ((UINT16)( ( ((x) >> (n)) | ((x) << (16-(n))) ) ))

#if SYMCRYPT_MS_VC
    #define ROL32( x, n ) _rotl( (x), (n) )
    #define ROR32( x, n ) _rotr( (x), (n) )
    #define ROL64( x, n ) _rotl64( (x), (n) )
    #define ROR64( x, n ) _rotr64( (x), (n) )
#elif SYMCRYPT_GNUC
    #define ROL32( x, n ) ((UINT32)( ( ((x) << (n)) | ((x) >> (32-(n))) ) ))
    #define ROR32( x, n ) ((UINT32)( ( ((x) >> (n)) | ((x) << (32-(n))) ) ))
    #define ROL64( x, n ) ((UINT64)( ( ((x) << (n)) | ((x) >> (64-(n))) ) ))
    #define ROR64( x, n ) ((UINT64)( ( ((x) >> (n)) | ((x) << (64-(n))) ) ))
#else
    #error Unknown compiler
#endif


#define SYMCRYPT_ARRAY_SIZE(_x)     (sizeof(_x)/sizeof(_x[0]))

enum{
    STATE_NEXT = 0,         // starting state = 0, set by structure wipe.
    STATE_DATA_START,
    STATE_DATA_END,
    STATE_RESULT2,          // 2nd phase of result computation (1st phase is at STATE_NEXT when the result operation is found)
    STATE_RESULT_DONE,      // 3rd phase of result computation
};



//==========================================================================
// Inline implementations ...
//==========================================================================

//
// These are a bunch of functions to convert between an array of
// 32 or 64-bit integers to an array of bytes in LSBfirst or MSBfirst convention.
// Not all variations have been implemented yet. We add them as they are
// needed.
//

//
// These implementations are optimized for inlining, especially when the
// size of the data to be converted is a compile-time constant.
//

//
// SymCryptUint32ToMsbFirst & SymCryptMsbFirstToUint32.
// This is used by the SHA family
//
#if SYMCRYPT_CPU_AMD64

//
// On AMD64 we can do 2 UINT32s at once by doing a ROL(x,32) and a BSWAP.
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint32ToMsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )
{
    while( cuData >= 2 )
    {
        SYMCRYPT_STORE_MSBFIRST64( pbResult, ROL64( *(UINT64*)puData, 32 ));
        pbResult += 8;
        puData += 2;
        cuData -= 2;
    }

    if( cuData != 0 )
    {
        SYMCRYPT_STORE_MSBFIRST32( pbResult, *puData );
    }
}

#else // not _AMD64_

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint32ToMsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )
{
    while( cuData != 0 )
    {
        SYMCRYPT_STORE_MSBFIRST32( pbResult, *puData );
        puData++;
        pbResult += 4;
        cuData--;
    }
}
#endif // platform switch for SymCryptUint32ToMsbFirst

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMsbFirstToUint32( _In_reads_(4*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT32 puResult,
                                                 SIZE_T  cuResult )
{
    while( cuResult != 0 )
    {
        *puResult = SYMCRYPT_LOAD_MSBFIRST32( pbData );
        puResult++;
        pbData += 4;
        cuResult--;
    }
}


//
// SymCryptUint32ToLsbFirst & SymCryptLsbFirstToUint32
// These are used by the MD4 and MD5 hash functions
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64

//
// On AMD64, X86, and ARM this is just a memcpy
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint32ToLsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )

{
    memcpy( pbResult, puData, 4*cuData );
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptLsbFirstToUint32( _In_reads_(4*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT32 puResult,
                                                 SIZE_T  cuResult )
{
    memcpy( puResult, pbData, 4*cuResult );
}

#else // not (AMD64_ or X86_ or ARM or ARM64)

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint32ToLsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )
{
    while( cuData != 0 )
    {
        SYMCRYPT_STORE_LSBFIRST32( pbResult, *puData );
        puData++;
        pbResult += 4;
        cuData--;
    }
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptLsbFirstToUint32( _In_reads_(4*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT32 puResult,
                                                 SIZE_T  cuResult )
{
    while( cuResult != 0 )
    {
        *puResult = SYMCRYPT_LOAD_LSBFIRST32( pbData );
        pbData += 4;
        puResult++;
        cuResult--;
    }
}

#endif // Platform switch for SymCryptUint32ToLsbFirst


//
// SymCryptUint64ToLsbFirst & SymCryptLsbFirstToUint64
// These are used by Keccak.
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64

//
// On AMD64, X86, and ARM this is just a memcpy
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint64ToLsbFirst( _In_reads_(cuData)     PCUINT64 puData,
                          _Out_writes_(8*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )

{
    memcpy( pbResult, puData, 8*cuData );
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptLsbFirstToUint64( _In_reads_(8*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT64 puResult,
                                                 SIZE_T  cuResult )
{
    memcpy( puResult, pbData, 8*cuResult );
}

#else // not (AMD64_ or X86_ or ARM or ARM64)

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint64ToLsbFirst( _In_reads_(cuData)     PCUINT64 puData,
                          _Out_writes_(8*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData )
{
    while( cuData != 0 )
    {
        SYMCRYPT_STORE_LSBFIRST64( pbResult, *puData );
        puData++;
        pbResult += 8;
        cuData--;
    }
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptLsbFirstToUint64( _In_reads_(8*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT64 puResult,
                                                 SIZE_T  cuResult )
{
    while( cuResult != 0 )
    {
        *puResult = SYMCRYPT_LOAD_LSBFIRST64( pbData );
        pbData += 8;
        puResult++;
        cuResult--;
    }
}

#endif // Platform switch for SymCryptUint64ToLsbFirst & SymCryptLsbFirstToUint64


//
// SymCryptUint64ToMsbFirst & SymCryptMsbFirstToUint64
//
FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptUint64ToMsbFirst( _In_reads_(cuData)     PCUINT64    puData,
                          _Out_writes_(8*cuData) PBYTE       pbResult,
                                                 SIZE_T      cuData )
{
    while( cuData != 0 )
    {
        SYMCRYPT_STORE_MSBFIRST64( pbResult, *puData );
        pbResult += 8;
        puData ++;
        cuData --;
    }
}

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMsbFirstToUint64( _In_reads_(8*cuResult) PCBYTE      pbData,
                          _Out_writes_(cuResult) PUINT64  puResult,
                                                 SIZE_T      cuResult )
{
    while( cuResult != 0 )
    {
        *puResult = SYMCRYPT_LOAD_MSBFIRST64( pbData );
        puResult++;
        pbData += 8;
        cuResult--;
    }
}

////////////////////////////////////////////////////////////////////////////////////
//  Internal function prototypes
//

//
// SymCryptSha1AppendBlocks
//
// Updates the chaining state of the hash function with one or more blocks of data.
// Each block is 64 bytes long, the natural size of a SHA256 input block.
//
// cbData must be a multiple of 64.
//
VOID
SYMCRYPT_CALL
SymCryptSha1AppendBlocks(
    _Inout_                 SYMCRYPT_SHA1_CHAINING_STATE  * pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining );

//
// SymCryptSha256AppendBlocks
//
// Updates the chaining state of the hash function with one or more blocks of data.
// Each block is 64 bytes long, the natural size of a SHA256 input block.
//
// cbData must be a multiple of 64.
//
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Intrinsics implementation processing 4 message blocks in parallel using XMM registers
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_xmm_4blocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Assembly implementation processing 4 message blocks in parallel using XMM registers
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_xmm_ssse3_asm(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Intrinsics implementation processing 8 message blocks in parallel using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_ymm_8blocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Assembly implementation processing 8 message blocks in parallel using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_ymm_avx2_asm(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );


//
// SymCryptSha512AppendBlocks
//
// Updates the chaining state of the hash function with one or more blocks of data.
// Each block is 128 bytes long, the natural size of a SHA512 input block.
//
// cbData must be a multiple of 128.
//
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE    * pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );


VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_xmm(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Intrinsics implementation using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_1block(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Intrinsics implementation processing 2 message blocks in parallel using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_2blocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Intrinsics implementation processing 4 message blocks in parallel using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_4blocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Assembly implementation processing 4 message blocks in parallel using YMM registers
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_avx2_asm(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );

// Assembly implementation processing 4 message blocks in parallel using YMM registers with AVX512 instruction set
VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_avx512vl_asm(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE  *   pChain,
    _In_reads_(cbData)      PCBYTE                              pbData,
                            SIZE_T                              cbData,
    _Out_                   SIZE_T                            * pcbRemaining );




//
// SymCryptMd5AppendBlocks
//
// Updates the chaining state of the hash function with one or more blocks of data.
// Each block is 64 bytes long, the natural size of a MD5 input block.
//
// cbData must be a multiple of 64.
//
VOID
SYMCRYPT_CALL
SymCryptMd5AppendBlocks(
    _Inout_                 SYMCRYPT_MD5_CHAINING_STATE   * pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining );


//
// SymCryptMd4AppendBlocks
//
// Updates the chaining state of the hash function with one or more blocks of data.
// Each block is 64 bytes long, the natural size of a MD5 input block.
//
// cbData must be a multiple of 64.
//
VOID
SYMCRYPT_CALL
SymCryptMd4AppendBlocks(
    _Inout_                 SYMCRYPT_MD4_CHAINING_STATE   * pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining );


//
// SymCryptMd2AppendBlock
//
// Update the C and X state based on the message block in the buffer.
//
VOID
SYMCRYPT_CALL
SymCryptMd2AppendBlocks(
    _Inout_                 SYMCRYPT_MD2_CHAINING_STATE   * pChain,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T                        * pcbRemaining );


//
// SymCryptUint32ToMsbFirst
//
// Convert an array of UINT32s to 4-byte values stored MSB first (big-endian) conversion.
// Note that the count is the number of UINT32s to convert, not the number
// of bytes. This is somewhat unusual, but it avoids any confusion about
// converting an odd number of bytes.
//
VOID
SYMCRYPT_CALL
SymCryptUint32ToMsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData );

//
// SymCryptUint32ToLsbFirst
//
// Convert an array of UINT32s to 4-byte values stored LSB first (little-endian) conversion.
// Note that the count is the number of UINT32s to convert, not the number
// of bytes. This is somewhat unusual, but it avoids any confusion about
// converting an odd number of bytes.
//
VOID
SYMCRYPT_CALL
SymCryptUint32ToLsbFirst( _In_reads_(cuData)     PCUINT32 puData,
                          _Out_writes_(4*cuData) PBYTE    pbResult,
                                                 SIZE_T   cuData );

//
// SymCryptMsbFirstToUint32
//
// Convert an array of 4-byte values stored MSB first to an array of UINT32s
// (big-endian) conversion.
// Note that the count is the number of UINT32s to convert, not the number
// of bytes. This is somewhat unusual, but it avoids any confusion about
// converting an odd number of bytes.
//
VOID
SYMCRYPT_CALL
SymCryptMsbFirstToUint32( _In_reads_(4*cuResult) PCBYTE   pbData,
                          _Out_writes_(cuResult) PUINT32  puResult,
                                                 SIZE_T   cuResult );

//
// SymCryptLsbFirstToUint32
//
// Convert an array of 4-byte values stored LSB first to an array of UINT32s
// (little-endian) conversion.
// Note that the count is the number of UINT32s to convert, not the number
// of bytes. This is somewhat unusual, but it avoids any confusion about
// converting an odd number of bytes.
//
VOID
SYMCRYPT_CALL
SymCryptLsbFirstToUint32( _In_reads_(4*cuResult) PCBYTE  pbData,
                          _Out_writes_(cuResult) PUINT32 puResult,
                                                 SIZE_T  cuResult );

//
// SymCryptUint64ToMsbFirst
//
// Convert an array of UINT64s to an array of bytes using the MSB first
// (big-endian) conversion.
//
VOID
SYMCRYPT_CALL
SymCryptUint64ToMsbFirst( _In_reads_(cuData)     PCUINT64    puData,
                          _Out_writes_(8*cuData) PBYTE       pbResult,
                                                 SIZE_T      cuData );

//
// SymCryptMsbFirstToUint64
//
// Convert an array of 4-byte values stored MSB first to an array of UINT64s
// (big-endian) conversion.
// Note that the count is the number of UINT64s to convert, not the number
// of bytes. This is somewhat unusual, but it avoids any confusion about
// converting an odd number of bytes.
//
VOID
SYMCRYPT_CALL
SymCryptMsbFirstToUint64( _In_reads_(8*cuResult) PCBYTE      pbData,
                          _Out_writes_(cuResult) PUINT64      puResult,
                                                 SIZE_T      cuResult );



//============================================================================
// HMAC macros and inline functions.
//
#define REPEAT_BYTE_TO_UINT32( x ) (((UINT32)x << 24) | ((UINT32)x << 16) | ((UINT32)x << 8) | x)
#define REPEAT_BYTE_TO_UINT64( x ) ( ((UINT64)REPEAT_BYTE_TO_UINT32(x) << 32) | REPEAT_BYTE_TO_UINT32(x) )

//
// The XorByteIntoBuffer function is a platform-optimized function to xor a byte
// repeatedly into a buffer.
// Note that the buffer length must be a multiple of 8.
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
FORCEINLINE
VOID
SYMCRYPT_CALL
XorByteIntoBuffer( _Inout_updates_( 8*cqBuf ) PBYTE pbBuf, SIZE_T cqBuf, BYTE v )
{
    SIZE_T i;
    const UINT64 v64 = REPEAT_BYTE_TO_UINT64( v );

    for( i=0; i<cqBuf; i++ )
    {
        ((UINT64 *)pbBuf)[i] ^= v64;
    }
}
#else
FORCEINLINE
VOID
SYMCRYPT_CALL
XorByteIntoBuffer( _Inout_updates_( 8*cqBuf ) PBYTE pbBuf, SIZE_T cqBuf, BYTE v )
{
    SIZE_T i;

    for( i=0; i<8*cqBuf; i++ )
    {
        pbBuf[i] ^= v;
    }
}
#endif

//
// GHASH
//

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKey(
    _Out_                                       PSYMCRYPT_GHASH_EXPANDED_KEY    expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )     PCBYTE                          pH );

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyC(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )     PCBYTE                  pH );

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyX86(
    _Out_                                   PSYMCRYPT_GHASH_EXPANDED_KEY    expandedKey,
   _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )  PCBYTE                          pH );

VOID
SYMCRYPT_CALL
SymCryptGHashExpandKeyAmd64(
    _Out_writes_( SYMCRYPT_GF128_FIELD_SIZE )   PSYMCRYPT_GF128_ELEMENT expandedKey,
    _In_reads_( SYMCRYPT_GF128_BLOCK_SIZE )     PCBYTE                  pH );

//
// For all GHashAppendData functions, data will be appended in multiples of SYMCRYPT_GF128_BLOCK_SIZE.
// If the data is not a multiple of SYMCRYPT_GF128_BLOCK_SIZE, any remaining data will be ignored.
//

VOID
SYMCRYPT_CALL
SymCryptGHashAppendData(
    _In_                    PCSYMCRYPT_GHASH_EXPANDED_KEY   expandedKey,
    _Inout_                 PSYMCRYPT_GF128_ELEMENT         pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataC(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE )     PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                     PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataXmm(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbData,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataNeon(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE )     PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                     PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptGHashAppendDataPclmulqdq(
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbData,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptGHashResult(
    _In_                                        PCSYMCRYPT_GF128_ELEMENT    pState,
    _Out_writes_( SYMCRYPT_GF128_BLOCK_SIZE )   PBYTE                       pbResult );


VOID
SYMCRYPT_CALL
SymCryptMarvin32AppendBlocks(
    _Inout_                 PSYMCRYPT_MARVIN32_CHAINING_STATE   pChain,
    _In_reads_( cbData )    PCBYTE                              pbData,
                            SIZE_T                              cbData );




extern const BYTE SymCryptTestMsg3[3];
extern const BYTE SymCryptTestMsg16[16];
extern const BYTE SymCryptTestKey32[32];

VOID
SYMCRYPT_CALL
SymCryptInjectError( PBYTE pbData, SIZE_T cbData );


#define SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM  1      // enable checking of OSXSAVE bit & XGETBV logic

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesByCpuid( UINT32 flags );

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromRegisters(void);

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromRegistersNoTry(void);

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent(void);

VOID
SYMCRYPT_CALL
SymCryptCpuidExFunc( int cpuInfo[4], int function_id, int subfunction_id );

////////////////////////////////////////////////////////////////////////////
// Export blob formats
////////////////////////////////////////////////////////////////////////

//==========================================================
// BLOBS
//
// SYMCRYPT_BLOB_HEADER
// Generic header for all exported blobs from SymCrypt
//

typedef enum _SYMCRYPT_BLOB_TYPE {
    SymCryptBlobTypeUnknown     = 0,
    SymCryptBlobTypeHashState   = 0x100,
    SymCryptBlobTypeMd2State    = SymCryptBlobTypeHashState + 1,       // explicit constants as these have to remain the same forever.
    SymCryptBlobTypeMd4State    = SymCryptBlobTypeHashState + 2,
    SymCryptBlobTypeMd5State    = SymCryptBlobTypeHashState + 3,
    SymCryptBlobTypeSha1State   = SymCryptBlobTypeHashState + 4,
    SymCryptBlobTypeSha256State = SymCryptBlobTypeHashState + 5,
    SymCryptBlobTypeSha384State = SymCryptBlobTypeHashState + 6,
    SymCryptBlobTypeSha512State = SymCryptBlobTypeHashState + 7,
    SymCryptBlobTypeSha3_256State = SymCryptBlobTypeHashState + 8,
    SymCryptBlobTypeSha3_384State = SymCryptBlobTypeHashState + 9,
    SymCryptBlobTypeSha3_512State = SymCryptBlobTypeHashState + 10,
    SymCryptBlobTypeSha224State = SymCryptBlobTypeHashState + 11,
    SymCryptBlobTypeSha512_224State = SymCryptBlobTypeHashState + 12,
    SymCryptBlobTypeSha512_256State = SymCryptBlobTypeHashState + 13,
    SymCryptBlobTypeSha3_224State = SymCryptBlobTypeHashState + 14,
} SYMCRYPT_BLOB_TYPE;

#define SYMCRYPT_BLOB_MAGIC ('cmys')

//
// We define all export structures with pack=1 so that there are no padding bytes.
//
#pragma pack(push, 1)

typedef struct _SYMCRYPT_BLOB_HEADER {
    UINT32              magic;              // 'cmys'
    UINT32              size;               // total size of blob
    UINT32              type;               // SYMCRYPT_BLOB_TYPE: type of blob
} SYMCRYPT_BLOB_HEADER, *PSYMCRYPT_BLOB_HEADER;

typedef struct _SYMCRYPT_BLOB_TRAILER {
    BYTE                checksum[8];        // contains the Marvin32 checksum of the rest of the blob
} SYMCRYPT_BLOB_TRAILER, *PSYMCRYPT_BLOB_TRAILER;

typedef struct _SYMCRYPT_MD2_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    C[16];
    BYTE                    X[16];
    UINT32                  bytesInBuffer;
    BYTE                    buffer[16];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_MD2_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_MD2_STATE_EXPORT_BLOB ) == SYMCRYPT_MD2_STATE_EXPORT_SIZE );


typedef struct _SYMCRYPT_MD4_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    chain[16];          // In the same format used for the final hash value of MD4
    UINT64                  dataLength;
    BYTE                    buffer[64];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_MD4_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_MD4_STATE_EXPORT_BLOB ) == SYMCRYPT_MD4_STATE_EXPORT_SIZE );


typedef struct _SYMCRYPT_MD5_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    chain[16];          // In the same format used for the final hash value of MD5
    UINT64                  dataLength;
    BYTE                    buffer[64];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_MD5_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_MD5_STATE_EXPORT_BLOB ) == SYMCRYPT_MD5_STATE_EXPORT_SIZE );


typedef struct _SYMCRYPT_SHA1_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    chain[20];          // in the same format used for the final hash value of SHA-1
    UINT64                  dataLength;
    BYTE                    buffer[64];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_SHA1_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_SHA1_STATE_EXPORT_BLOB ) == SYMCRYPT_SHA1_STATE_EXPORT_SIZE );


typedef struct _SYMCRYPT_SHA256_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    chain[32];          // in the same format used for the final hash value of SHA-256
    UINT64                  dataLength;
    BYTE                    buffer[64];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_SHA256_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_SHA256_STATE_EXPORT_BLOB ) == SYMCRYPT_SHA256_STATE_EXPORT_SIZE );


typedef struct _SYMCRYPT_SHA512_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    chain[64];          // in the same format used for the final hash value of SHA-512
    UINT64                  dataLengthL;        // low 64 bits of data length
    UINT64                  dataLengthH;        // high 64 bits of data length
    BYTE                    buffer[128];
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_SHA512_STATE_EXPORT_BLOB;

C_ASSERT( sizeof( SYMCRYPT_SHA512_STATE_EXPORT_BLOB ) == SYMCRYPT_SHA512_STATE_EXPORT_SIZE );

// Refer to SYMCRYPT_KECCAK_STATE documentation for the explanation of each struct member
typedef struct _SYMCRYPT_KECCAK_STATE_EXPORT_BLOB {
    SYMCRYPT_BLOB_HEADER    header;
    BYTE                    state[200];
    UINT32                  stateIndex;
    UINT8                   paddingValue;
    BOOLEAN                 squeezeMode;
    BYTE                    rfu[8];             // rfu = Reserved for Future Use.
    SYMCRYPT_BLOB_TRAILER   trailer;
} SYMCRYPT_KECCAK_STATE_EXPORT_BLOB;

typedef SYMCRYPT_KECCAK_STATE_EXPORT_BLOB SYMCRYPT_SHA3_224_STATE_EXPORT_BLOB;
typedef SYMCRYPT_KECCAK_STATE_EXPORT_BLOB SYMCRYPT_SHA3_256_STATE_EXPORT_BLOB;
typedef SYMCRYPT_KECCAK_STATE_EXPORT_BLOB SYMCRYPT_SHA3_384_STATE_EXPORT_BLOB;
typedef SYMCRYPT_KECCAK_STATE_EXPORT_BLOB SYMCRYPT_SHA3_512_STATE_EXPORT_BLOB;

C_ASSERT(sizeof(SYMCRYPT_SHA3_224_STATE_EXPORT_BLOB) == SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE);
C_ASSERT(sizeof(SYMCRYPT_SHA3_256_STATE_EXPORT_BLOB) == SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE);
C_ASSERT(sizeof(SYMCRYPT_SHA3_384_STATE_EXPORT_BLOB) == SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE);
C_ASSERT(sizeof(SYMCRYPT_SHA3_512_STATE_EXPORT_BLOB) == SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE);

#pragma pack(pop)

/////////////////////////////////////////////
// AES internal functions

extern const SYMCRYPT_BLOCKCIPHER SymCryptAesBlockCipherNoOpt;

VOID
SYMCRYPT_CALL
SymCryptAes4Sbox(
    _In_reads_(4)   PCBYTE  pIn,
    _Out_writes_(4) PBYTE   pOut,
                    BOOL    UseSimd );

VOID
SYMCRYPT_CALL
SymCryptAes4SboxC(
    _In_reads_(4)   PCBYTE  pIn,
    _Out_writes_(4) PBYTE   pOut );

VOID
SYMCRYPT_CALL
SymCryptAes4SboxXmm(
    _In_reads_(4)   PCBYTE  pIn,
    _Out_writes_(4) PBYTE   pOut );

VOID
SYMCRYPT_CALL
SymCryptAes4SboxNeon(
    _In_reads_(4)   PCBYTE  pIn,
    _Out_writes_(4) PBYTE   pOut );

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKey(
    _In_reads_(16)      PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey,
                        BOOL    UseSimd );

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyC(
    _In_reads_(16)     PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey );

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyXmm(
    _In_reads_(16)     PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey );

VOID
SYMCRYPT_CALL
SymCryptAesCreateDecryptionRoundKeyNeon(
    _In_reads_(16)     PCBYTE  pEncryptionRoundKey,
    _Out_writes_(16)    PBYTE   pDecryptionRoundKey );

VOID
SYMCRYPT_CALL
SymCryptAesEncryptC(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesEncryptAsm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesEncryptXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesEncryptNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesDecryptC(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesDecryptAsm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesDecryptXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesDecryptNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );
VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );
VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesEcbDecryptC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcEncryptAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );
VOID
SYMCRYPT_CALL
SymCryptAesCbcEncryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcEncryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcDecryptAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcDecryptXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcDecryptNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcMacXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcMacNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb64Asm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb64Xmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb64Neon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb32Xmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb32Neon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitC(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitAsm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

// pbScratch must currently be 16B aligned
VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )       PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

// pbScratch must currently be 16B aligned
VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitXmm(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )       PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitZmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitZmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitYmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitYmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitNeon(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsEncryptDataUnit(
    _In_                                        PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_                                        PCVOID                      pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptXtsDecryptDataUnit(
    _In_                                        PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_                                        PCVOID                      pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )  PBYTE                       pbTweakBlock,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedXmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

#define GCM_YMM_MINBLOCKS 16

// Caller must check cbData >= GCM_YMM_MINBLOCKS * SYMCRYPT_GCM_BLOCK_SIZE
VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedYmm_2048(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

// Caller must check cbData >= GCM_YMM_MINBLOCKS * SYMCRYPT_GCM_BLOCK_SIZE
VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedYmm_2048(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedNeon(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptGcmEncryptPartTwoPass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptGcmDecryptPartTwoPass(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptCtrMsb32(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// SymCryptCtrMsb32 implements the CTR cipher mode with a 32-bit increment function.
// It is not intended to be used as-is, rather it is a building block for modes like GCM.
// See the description of SymCryptCtrMsb64 in symcrypt.h for more details.
//
// For now, this function is only intended for use with GCM, which specifies the use a
// 32-bit increment function. It's only used in cases where we can't use one of the optimized
// implementations (i.e. on ARM32 or x86[-64] without AESNI). Therefore, unlike the 64-bit version,
// there are no optimized implementations of the CTR function to call. If we ever need this
// functionality for other block cipher modes, this function will need to be updated and we'll
// need to add an additional pointer to SYMCRYPT_BLOCKCIPHER for the optimized CTR function.

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb32(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

// SymCryptAesCtrMsb32 is a dispatch function for the optimized AES CTR implementations that use
//a 32-bit counter function (currently only relevant to GCM).

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelHashProcess_serial(
    _In_                                                            PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_updates_bytes_( nStates * pParHash->pHash->stateSize )   PVOID                               pStates,
                                                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )                                  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )                                       PBYTE                               pbScratch,
                                                                    SIZE_T                              cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelHashProcess(
    _In_                                                            PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_updates_bytes_( nStates * pParHash->pHash->stateSize )   PVOID                               pStates,
                                                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )                                  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )                                       PBYTE                               pbScratch,
                                                                    SIZE_T                              cbScratch,
                                                                    UINT32                              maxParallel );

VOID
SYMCRYPT_CALL
SymCryptHashAppendInternal(
    _In_                        PCSYMCRYPT_HASH             pHash,
    _Inout_                     PSYMCRYPT_COMMON_HASH_STATE pState,
    _In_reads_bytes_( cbData )  PCBYTE                      pbData,
                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHashCommonPaddingMd4Style(
    _In_                        PCSYMCRYPT_HASH             pHash,
    _Inout_                     PSYMCRYPT_COMMON_HASH_STATE pState );


extern const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha256Algorithm;
extern const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha384Algorithm;
extern const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha512Algorithm;

#define PAR_SCRATCH_ELEMENTS_256    (4+8+64)    // # scratch elements our parallel SHA256 implementations need
#define PAR_SCRATCH_ELEMENTS_512    (4+8+80)    // # scratch elements our parallel SHA512 implementations need

// pScratch must be 32B aligned, as it is used as an array of __m256i
VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBlocks_ymm(
    _Inout_updates_( 8 )                                PSYMCRYPT_SHA256_CHAINING_STATE   * pChain,
    _Inout_updates_( 8 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_256 * 32 )       PBYTE                               pScratch );

// pScratch must be 32B aligned, as it is used as an array of __m256i
VOID
SYMCRYPT_CALL
SymCryptParallelSha512AppendBlocks_ymm(
    _Inout_updates_( 4 )                                PSYMCRYPT_SHA512_CHAINING_STATE   * pChain,
    _Inout_updates_( 4 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_512 * 32 )       PBYTE                               pScratch );

extern const SYMCRYPT_HASH SymCryptMd2Algorithm_default;
extern const SYMCRYPT_HASH SymCryptMd4Algorithm_default;
extern const SYMCRYPT_HASH SymCryptMd5Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha1Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha224Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha256Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha384Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha512Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha512_224Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha512_256Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha3_224Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha3_256Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha3_384Algorithm_default;
extern const SYMCRYPT_HASH SymCryptSha3_512Algorithm_default;
extern const SYMCRYPT_HASH SymCryptShake128HashAlgorithm_default;
extern const SYMCRYPT_HASH SymCryptShake256HashAlgorithm_default;



// Paddings used by various SHA-3 derived algorithms
#define SYMCRYPT_SHA3_PADDING_VALUE     0x06    // 01 10* padding
#define SYMCRYPT_SHAKE_PADDING_VALUE    0x1f    // 11 11 10* padding
#define SYMCRYPT_CSHAKE_PADDING_VALUE   0x04    // 00 10* padding (used when N or S are non-empty strings)

//
// Functions operating on the Keccak state
//

VOID
SYMCRYPT_CALL
SymCryptKeccakPermute(_Inout_updates_(25) UINT64* pState);
// Keccak-f[1600] permutation

VOID
SYMCRYPT_CALL
SymCryptKeccakInit(_Out_ PSYMCRYPT_KECCAK_STATE pState, UINT32 inputBlockSize, UINT8 padding);

VOID
SYMCRYPT_CALL
SymCryptKeccakReset(_Out_ PSYMCRYPT_KECCAK_STATE pState);

VOID
SYMCRYPT_CALL
SymCryptKeccakZeroAppendBlock(_Inout_ PSYMCRYPT_KECCAK_STATE pState);
// Zero pads the current block by invoking the permutation and setting
// pState->stateIndex to 0.

VOID
SYMCRYPT_CALL
SymCryptKeccakAppend(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_(cbData)      PCBYTE                  pbData,
                            SIZE_T                  cbData);
// Generic append function.

VOID
SYMCRYPT_CALL
SymCryptKeccakExtract(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _Out_writes_(cbResult)  PBYTE                   pbResult,
                            SIZE_T                  cbResult,
                            BOOLEAN                 bWipe);
// Generic extract function, no restriction on cbResult.
// bWipe denotes whether to wipe the Keccak state and initialize it
// for a new computation.

VOID
SYMCRYPT_CALL
SymCryptKeccakStateExport(
                                                            SYMCRYPT_BLOB_TYPE      type,
    _In_                                                    PCSYMCRYPT_KECCAK_STATE pState,
    _Out_writes_bytes_(SYMCRYPT_KECCAK_STATE_EXPORT_SIZE)   PBYTE                   pbBlob);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKeccakStateImport(
                                                        SYMCRYPT_BLOB_TYPE      type,
    _Out_                                               PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_bytes_(SYMCRYPT_KECCAK_STATE_EXPORT_SIZE) PCBYTE                  pbBlob);

VOID
SYMCRYPT_CALL
SymCryptKeccakAppendEncodeTimes8(
    _Inout_ SYMCRYPT_KECCAK_STATE *pState,
            UINT64  uValue,
            BOOLEAN bLeftEncode);
// Appends the left-encoding of uValue * 8 to the state

VOID
SYMCRYPT_CALL
SymCryptKeccakAppendEncodedString(
    _Inout_                 PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_(cbString)    PCBYTE                  pbString,
                            SIZE_T                  cbString);
// Appends 'left_encode(cbString * 8) || pbString' to the state

VOID
SYMCRYPT_CALL
SymCryptCShakeEncodeInputStrings(
    _Inout_                             PSYMCRYPT_KECCAK_STATE  pState,
    _In_reads_( cbFunctionNameString )  PCBYTE                  pbFunctionNameString,
                                        SIZE_T                  cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE                  pbCustomizationString,
                                        SIZE_T                  cbCustomizationString);
// Process CShake input strings
// Appends byte_pad( encode_string( pbFunctionNameString ) || encode_string( pbCustomizationString ), pState->inputBlockSize )



VOID
SYMCRYPT_CALL
SymCryptFatalIntercept( UINT32 fatalCode );

extern const BYTE SymCryptSha256KATAnswer[32];
extern const BYTE SymCryptSha384KATAnswer[48];
extern const BYTE SymCryptSha512KATAnswer[64];

//
// Arithmetic
//

#define SYMCRYPT_ASSERT_ASYM_ALIGNED( _p )           SYMCRYPT_ASSERT( ((SIZE_T)(_p) & (SYMCRYPT_ASYM_ALIGN_VALUE - 1)) == 0 );


#define SYMCRYPT_FDEF_DIGIT_NUINT32             ((UINT32)(SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT32 ) ))

#define SYMCRYPT_OBJ_NDIGITS( _p )              ((_p)->nDigits)
#define SYMCRYPT_OBJ_NBYTES( _p )               ((_p)->nDigits * SYMCRYPT_FDEF_DIGIT_SIZE)
#define SYMCRYPT_OBJ_NUINT32( _p )              ((_p)->nDigits * SYMCRYPT_FDEF_DIGIT_SIZE / sizeof( UINT32 ))

#if SYMCRYPT_MS_VC
#define SYMCRYPT_MUL32x32TO64( _a, _b )         UInt32x32To64( (_a), (_b) )
#elif SYMCRYPT_GNUC
#define SYMCRYPT_MUL32x32TO64( _a, _b )         ( (UINT64)(_a)*(UINT64)(_b) )
#else
    #error Unknown compiler
#endif
typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MOD_BINARY_OP_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MOD_UNARY_OP_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

typedef SYMCRYPT_ERROR (SYMCRYPT_CALL * SYMCRYPT_MOD_UNARY_OP_FLAG_STATUS_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MOD_SET_POST_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

typedef PCUINT32 (SYMCRYPT_CALL * SYMCRYPT_MOD_PRE_GET_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MOD_COPY_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MODULUS_COPYFIXUP_FN)(
    _In_                            PCSYMCRYPT_MODULUS      pmSrc,
    _Out_                           PSYMCRYPT_MODULUS       pmDst );

typedef VOID (SYMCRYPT_CALL * SYMCRYPT_MODULUS_INIT_FN)(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

//
// In the future we might want to implement a 'prepare divisor' for people who want to do one or more modular divisions.
// In EC projective coordinates you have a value stored as (X,Z) with X/Z being the actual value that needs to be exported.
// In Montgomery format, this is stored as (RX, RZ), and just doing RX * (1/RZ) gets you the value to be exported.
// There seem to be many tricks here to get some more speed; maybe we just need to define export functions for each
// point format and allow the Modulus to contain special optimizations.
//
// The SetPost function is the post-processing function of any SetValue operation. The SetValue operation will store the
// modElement in the normal integer format into the ModElement. The SetPost function post-processes it into the proper
// representation for that modulus.
//
// The PreGet function is the pre-processing function to any GetValue operation. It returns a pointer to the proper value
// stored in standard integer format. This pointer can either be into the ModElement itself, or into the scratch space.
//

typedef struct _SYMCRYPT_MODULAR_FUNCTIONS {
    SYMCRYPT_MOD_BINARY_OP_FN               modAdd;
    SYMCRYPT_MOD_BINARY_OP_FN               modSub;
    SYMCRYPT_MOD_UNARY_OP_FN                modNeg;
    SYMCRYPT_MOD_BINARY_OP_FN               modMul;
    SYMCRYPT_MOD_UNARY_OP_FN                modSquare;
    SYMCRYPT_MOD_UNARY_OP_FLAG_STATUS_FN    modInv;
    SYMCRYPT_MOD_SET_POST_FN                modSetPost;
    SYMCRYPT_MOD_PRE_GET_FN                 modPreGet;
    SYMCRYPT_MODULUS_COPYFIXUP_FN           modulusCopyFixup;   // non-generic fixup after memcpy
    SYMCRYPT_MODULUS_INIT_FN                modulusInit;
    PVOID                                   slack[6];
} SYMCRYPT_MODULAR_FUNCTIONS;

#define SYMCRYPT_MODULAR_FUNCTIONS_SIZE    (sizeof( SYMCRYPT_MODULAR_FUNCTIONS ) )

extern const SYMCRYPT_MODULAR_FUNCTIONS g_SymCryptModFns[];
extern const UINT32 g_SymCryptModFnsMask;

//
// Table entry that contains the information about an implementation.
// Allows generic code to make the decision.
// First entry in the table that is allowed is chosen, last entry always matches everything
//

#define SYMCRYPT_MODULUS_FEATURE_MONTGOMERY         1       // Modulus is suitable for Montgomery processing
// #define SYMCRYPT_MODULUS_FEATURE_PSEUDO_MERSENNE    2       // Modulus is suitable for Pseudo-Mersenne processing
// #define SYMCRYPT_MODULUS_FEATURE_NISTP256           4       // Modulus is the NIST P256 curve prime
#define SYMCRYPT_MODULUS_FEATURE_NISTP384           8       // Modulus is the NIST P384 curve prime

typedef struct _SYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY
{
    UINT32                  type;               // Type value of this solution
    SYMCRYPT_CPU_FEATURES   cpuFeatures;        // Required CPU features
    UINT32                  maxBits;            // Max # bits that the actual value of the modulus is, 0 = no limit
    UINT32                  modulusFeatures;    // Required features of the modulus
} SYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY, *PSYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY;
typedef const SYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY* PCSYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY;

extern const SYMCRYPT_MODULUS_TYPE_SELECTION_ENTRY SymCryptModulusTypeSelections[];       // Array can be any size...


// Check that the size is a power of 2
C_ASSERT( (SYMCRYPT_MODULAR_FUNCTIONS_SIZE & (SYMCRYPT_MODULAR_FUNCTIONS_SIZE-1)) == 0 );

// The macro that we use to call modular functions
#define SYMCRYPT_MOD_CALL(v) ((SYMCRYPT_MODULAR_FUNCTIONS *)(( SYMCRYPT_FORCE_READ32( &(v)->type) & g_SymCryptModFnsMask) + (PBYTE)(&g_SymCryptModFns) ))->

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_GENERIC {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulGeneric,\
    &SymCryptFdefModSquareGeneric,\
    &SymCryptFdefModInvGeneric,\
    &SymCryptFdefModSetPostGeneric,\
    &SymCryptFdefModPreGetGeneric,\
    &SymCryptFdefModulusCopyFixupGeneric,\
    &SymCryptFdefModulusInitGeneric,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulMontgomery,\
    &SymCryptFdefModSquareMontgomery,\
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_ARM64256 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAdd256Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub256Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomery256Asm, \
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomery256Asm, \
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_ARM64P384 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAdd384Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub384Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomeryP384Asm, \
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomeryP384Asm, \
    &SymCryptFdef369ModInvMontgomery,\
    &SymCryptFdef369ModSetPostMontgomery,\
    &SymCryptFdef369ModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdef369ModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX256 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAddMulx256Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub256Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomeryMulx256Asm,\
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomeryMulx256Asm,\
    &SymCryptFdefModInvMontgomery256,\
    &SymCryptFdefModSetPostMontgomeryMulx256,\
    &SymCryptFdefModPreGetMontgomery256,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery256,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULXP256 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAddMulx256Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub256Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomeryMulxP256Asm,\
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomeryMulxP256Asm,\
    &SymCryptFdefModInvMontgomery256,\
    &SymCryptFdefModSetPostMontgomeryMulx256,\
    &SymCryptFdefModPreGetMontgomery256,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery256,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX384 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAddMulx384Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub384Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomeryMulx384Asm,\
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomeryMulx384Asm,\
    &SymCryptFdef369ModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomeryMulx384,\
    &SymCryptFdef369ModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdef369ModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULXP384 {\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModAddMulx384Asm,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModSub384Asm,\
    &SymCryptFdefModNegGeneric,\
    (SYMCRYPT_MOD_BINARY_OP_FN) &SymCryptFdefModMulMontgomeryMulxP384Asm,\
    (SYMCRYPT_MOD_UNARY_OP_FN) &SymCryptFdefModSquareMontgomeryMulxP384Asm,\
    &SymCryptFdef369ModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomeryMulxP384,\
    &SymCryptFdef369ModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdef369ModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF369_MONTGOMERY {\
    &SymCryptFdef369ModAddGeneric,\
    &SymCryptFdef369ModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdef369ModMulMontgomery,\
    &SymCryptFdef369ModSquareMontgomery,\
    &SymCryptFdef369ModInvMontgomery,\
    &SymCryptFdef369ModSetPostMontgomery,\
    &SymCryptFdef369ModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdef369ModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulMontgomeryMulx,\
    &SymCryptFdefModSquareMontgomeryMulx,\
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY512 {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulMontgomery512,\
    &SymCryptFdefModSquareMontgomery512,\
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY1024 {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulMontgomery1024,\
    &SymCryptFdefModSquareMontgomery1024,\
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

#define SYMCRYPT_MOD_FUNCTIONS_FDEF_MONTGOMERY_MULX1024 {\
    &SymCryptFdefModAddGeneric,\
    &SymCryptFdefModSubGeneric,\
    &SymCryptFdefModNegGeneric,\
    &SymCryptFdefModMulMontgomeryMulx1024,\
    &SymCryptFdefModSquareMontgomeryMulx1024,\
    &SymCryptFdefModInvMontgomery,\
    &SymCryptFdefModSetPostMontgomery,\
    &SymCryptFdefModPreGetMontgomery,\
    &SymCryptFdefModulusCopyFixupMontgomery,\
    &SymCryptFdefModulusInitMontgomery,\
}

VOID
SYMCRYPT_CALL
SymCryptFdefMaskedCopy(
    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
                                                                UINT32      nDigits,
                                                                UINT32      mask );
//
// Copies Src to Dst under mask.
// Requirements:
//  - mask == 0 or mask == 0xffffffff
//  - cbData must be a multiple of the size of a digit, or a multiple of the size of a ModElement.
//  - pbSrc and pbDst must be SYMCRYPT_ALIGNed
// if mask == 0 this function does nothing.
// if mask == 0xffffffff this function is a memcpy from Src to Dst.
// This function is side-channel safe; the value of mask is not revealed
// through the memory access patterns.
//

VOID
SYMCRYPT_CALL
SymCryptFdefConditionalSwap(
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc1,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbSrc2,
                                                                UINT32      nDigits,
                                                                UINT32      cond );

//
// Swaps the bytes of Src1 with the bytes of Src2 under a condition.
// Requirements:
//  - cond = 0 or cond = 1 .
//  - cbData must be a multiple of the size of a digit, or a multiple of the size of a ModElement.
//  - pbSrc1 and pbSrc2 must be SYMCRYPT_ALIGNed
// if cond == 0 this function does nothing.
// if cond == 1 this function swaps the bytes of Src1 with the bytes of Src2.
// This function is side-channel safe; the value of cond is not revealed
// through the memory access patterns.
//

VOID
SYMCRYPT_CALL
SymCryptFdefClaimScratch( PBYTE pbScratch, SIZE_T cbScratch, SIZE_T cbMin );

UINT32
SymCryptFdefDigitsFromBits( UINT32 nBits );

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntAllocate( UINT32 nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofIntFromDigits( UINT32 nDigits );

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

VOID
SymCryptFdefIntCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SymCryptFdefIntMaskedCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          mask );

VOID
SYMCRYPT_CALL
SymCryptFdefIntConditionalCopy(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Inout_ PSYMCRYPT_INT   piDst,
            UINT32          cond );

VOID
SYMCRYPT_CALL
SymCryptFdefIntConditionalSwap(
    _Inout_ PSYMCRYPT_INT   piSrc1,
    _Inout_ PSYMCRYPT_INT   piSrc2,
            UINT32          cond );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntBitsizeOfObject( _In_ PCSYMCRYPT_INT  piSrc );

UINT32
SYMCRYPT_CALL
SymCryptFdefNumberofDigitsFromInt( _In_ PCSYMCRYPT_INT piSrc );

SYMCRYPT_ERROR
SymCryptFdefIntCopyMixedSize(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntBitsizeOfValue( _In_ PCSYMCRYPT_INT piSrc );

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetValueUint32(
            UINT32          u32Src,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetValueUint64(
            UINT64          u64Src,
    _Out_   PSYMCRYPT_INT   piDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefIntSetValue(
    _In_reads_bytes_(cbSrc)     PCBYTE                  pbSrc,
                                SIZE_T                  cbSrc,
                                SYMCRYPT_NUMBER_FORMAT  format,
    _Out_                       PSYMCRYPT_INT           piDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefIntGetValue(
    _In_                        PCSYMCRYPT_INT          piSrc,
    _Out_writes_bytes_(cbDst)   PBYTE                   pbDst,
                                SIZE_T                  cbDst,
                                SYMCRYPT_NUMBER_FORMAT  format );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetValueLsbits32( _In_  PCSYMCRYPT_INT piSrc );

UINT64
SYMCRYPT_CALL
SymCryptFdefIntGetValueLsbits64( _In_  PCSYMCRYPT_INT piSrc );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntAddMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
            UINT32          u32Src2,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubSameSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntSubMixedSize(
    _In_    PCSYMCRYPT_INT piSrc1,
    _In_    PCSYMCRYPT_INT piSrc2,
    _Out_   PSYMCRYPT_INT  piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntNeg(
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );


VOID
SYMCRYPT_CALL
SymCryptFdefIntMulPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          Exp,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntDivPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntShr1(
            UINT32          highestBit,
    _In_    PCSYMCRYPT_INT  piSrc,
    _Out_   PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntModPow2(
    _In_    PCSYMCRYPT_INT  piSrc,
            SIZE_T          exp,
    _Out_   PSYMCRYPT_INT   piDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetBit(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntGetBits(
    _In_    PCSYMCRYPT_INT  piSrc,
            UINT32          iBit,
            UINT32          nBits );

VOID
SYMCRYPT_CALL
SymCryptFdefIntSetBits(
    _In_    PSYMCRYPT_INT   piDst,
            UINT32          value,
            UINT32          iBit,
            UINT32          nBits );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsEqualUint32(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    UINT32          u32Src2 );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsEqual(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntIsLessThan(
    _In_    PCSYMCRYPT_INT  piSrc1,
    _In_    PCSYMCRYPT_INT  piSrc2 );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntMulUint32(
    _In_                            PCSYMCRYPT_INT  piSrc1,
                                    UINT32          Src2,
    _Out_                           PSYMCRYPT_INT   piDst );

VOID
SYMCRYPT_CALL
SymCryptFdefIntMulSameSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
VOID
SYMCRYPT_CALL
SymCryptFdefIntSquare(
    _In_                            PCSYMCRYPT_INT  piSrc,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );
VOID
SYMCRYPT_CALL
SymCryptFdefIntMulMixedSize(
    _In_                            PCSYMCRYPT_INT  piSrc1,
    _In_                            PCSYMCRYPT_INT  piSrc2,
    _Out_                           PSYMCRYPT_INT   piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE           pbScratch,
                                    SIZE_T          cbScratch );

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorAllocate( UINT32 nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofDivisorFromDigits( UINT32 nDigits );

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorRetrieveHandle( _In_ PBYTE pbBuffer );

VOID
SymCryptFdefDivisorCopy(
    _In_    PCSYMCRYPT_DIVISOR  pdSrc,
    _Out_   PSYMCRYPT_DIVISOR   pdDst );

VOID
SymCryptFdefDivisorCopyFixup(
    _In_    PCSYMCRYPT_DIVISOR  pSrc,
    _Out_   PSYMCRYPT_DIVISOR   pDst );

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntFromDivisor( _In_ PSYMCRYPT_DIVISOR pdSrc );

VOID
SYMCRYPT_CALL
SymCryptFdefIntToDivisor(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_DIVISOR   pdDst,
                                    UINT32              totalOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefIntDivMod(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _In_                            PCSYMCRYPT_DIVISOR  pdDivisor,
    _Out_opt_                       PSYMCRYPT_INT       piQuotient,
    _Out_opt_                       PSYMCRYPT_INT       piRemainder,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefRawDivMod(
    _In_reads_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)           PCUINT32            pNum,
                                                                UINT32              nDigits,
    _In_                                                        PCSYMCRYPT_DIVISOR  pdDivisor,
    _Out_writes_opt_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32             pQuotient,
    _Out_writes_opt_(SYMCRYPT_OBJ_NUINT32(pdDivisor))           PUINT32             pRemainder,
    _Out_writes_bytes_( cbScratch )                             PBYTE               pbScratch,
                                                                SIZE_T              cbScratch );


PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptFdefModulusAllocate( UINT32 nDigits );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusFree( _Out_ PSYMCRYPT_MODULUS pmObj );

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofModulusFromDigits( UINT32 nDigits );

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptFdefModulusCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer,
                                    UINT32  nDigits );

PSYMCRYPT_MODULUS
SYMCRYPT_CALL
SymCryptFdefModulusRetrieveHandle( _In_ PBYTE pbBuffer );


VOID
SymCryptFdefModulusCopy(
    _In_    PCSYMCRYPT_MODULUS  pmSrc,
    _Out_   PSYMCRYPT_MODULUS   pmDst );

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptFdefModElementAllocate( _In_ PCSYMCRYPT_MODULUS pmMod );

VOID
SYMCRYPT_CALL
SymCryptFdefModElementFree(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peObj );

UINT32
SYMCRYPT_CALL
SymCryptFdefSizeofModElementFromModulus( PCSYMCRYPT_MODULUS pmMod );

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptFdefModElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
                                    PCSYMCRYPT_MODULUS   pmMod );

PSYMCRYPT_MODELEMENT
SYMCRYPT_CALL
SymCryptFdefModElementRetrieveHandle( _In_ PBYTE pbBuffer );

VOID
SYMCRYPT_CALL
SymCryptFdefModElementWipe(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _Out_   PSYMCRYPT_MODELEMENT    peDst );

VOID
SymCryptFdefModElementCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst );

VOID
SymCryptFdefModElementMaskedCopy(
    _In_    PCSYMCRYPT_MODULUS      pmMod,
    _In_    PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_   PSYMCRYPT_MODELEMENT    peDst,
            UINT32                  mask );

PSYMCRYPT_DIVISOR
SYMCRYPT_CALL
SymCryptFdefDivisorFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc );

VOID
SymCryptFdefModElementConditionalSwap(
    _In_       PCSYMCRYPT_MODULUS    pmMod,
    _Inout_    PSYMCRYPT_MODELEMENT  peData1,
    _Inout_    PSYMCRYPT_MODELEMENT  peData2,
    _In_       UINT32                cond );

PSYMCRYPT_INT
SYMCRYPT_CALL
SymCryptFdefIntFromModulus( _In_ PSYMCRYPT_MODULUS pmSrc );

VOID
SYMCRYPT_CALL
SymCryptFdefIntToModulus(
    _In_                            PCSYMCRYPT_INT      piSrc,
    _Out_                           PSYMCRYPT_MODULUS   pmDst,
                                    UINT32              averageOperations,
                                    UINT32              flags,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefIntToModElement(
    _In_                            PCSYMCRYPT_INT          piSrc,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModElementToIntGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_bytes_( pmMod->nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )
                                    PCUINT32                pSrc,
    _Out_                           PSYMCRYPT_INT           piDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefRawSetValue(
    _In_reads_bytes_(cbSrc)                             PCBYTE                  pbSrc,
                                                        SIZE_T                  cbSrc,
                                                        SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32                 pDst,
                                                        UINT32                  nDigits );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModElementSetValueGeneric(
    _In_reads_bytes_( cbSrc )       PCBYTE                  pbSrc,
                                    SIZE_T                  cbSrc,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModElementSetValueUint32Generic(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModElementSetValueNegUint32(
                                    UINT32                  value,
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefRawGetValue(
    _In_reads_(nDigits * SYMCRYPT_FDEF_DIGIT_NUINT32)   PCUINT32                pSrc,
                                                        UINT32                  nDigits,
    _Out_writes_bytes_(cbDst)                           PBYTE                   pbDst,
                                                        SIZE_T                  cbDst,
                                                        SYMCRYPT_NUMBER_FORMAT  format );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModElementGetValue(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_writes_bytes_( cbDst )     PBYTE                   pbDst,
                                    SIZE_T                  cbDst,
                                    SYMCRYPT_NUMBER_FORMAT  format,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptFdefModElementIsEqual(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc1,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc2 );

UINT32
SYMCRYPT_CALL
SymCryptFdefModElementIsZero(
    _In_    PCSYMCRYPT_MODULUS     pmMod,
    _In_    PCSYMCRYPT_MODELEMENT  peSrc );

VOID
SYMCRYPT_CALL
SymCryptFdefModAddGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModAddMulx256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModAddMulx384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModAdd256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModAdd384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModAddGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSubGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModSubGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSub256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModSub384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModNegGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomeryMulx256(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSetPostMontgomeryMulxP384(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModSetPostMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PSYMCRYPT_MODELEMENT    peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

PCUINT32
SYMCRYPT_CALL
SymCryptFdefModPreGetMontgomery256(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

PCUINT32
SYMCRYPT_CALL
SymCryptFdef369ModPreGetMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusCopyFixupGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmSrc,
    _Out_                           PSYMCRYPT_MODULUS       pmDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusCopyFixupMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pmSrc,
    _Out_                           PSYMCRYPT_MODULUS       pmDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitGeneric(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomeryInternal(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
                                    UINT32                  nUint32Used,            // R = 2^{32 * this parameter}
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomery(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModulusInitMontgomery256(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModulusInitMontgomery(
    _Inout_                         PSYMCRYPT_MODULUS       pmObj,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );
UINT32
SYMCRYPT_CALL
SymCryptFdefRawAdd(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawSub(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits );
UINT32
SYMCRYPT_CALL
SymCryptFdefRawSubUint32(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulx256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulxP384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomery256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryP384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModMulMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulx(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModMulMontgomeryMulx1024(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );


VOID
SYMCRYPT_CALL
SymCryptFdefModSquareGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulx256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulxP384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomery256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryP384Asm(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369ModSquareMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulx(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSquareMontgomeryMulx1024(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );


VOID
SYMCRYPT_CALL
SymCryptFdefRawMul(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawMulMulx(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawMulMulx1024(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc1,
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc2,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquare(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)         PCUINT32    pSrc,
                                                            UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquareMulx(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)         PCUINT32    pSrc,
                                                            UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquareMulx1024(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)         PCUINT32    pSrc,
                                                            UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369RawMul(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsEqualUint32(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PCUINT32        pSrc1,
                                                    UINT32          nDigits,
    _In_                                            UINT32          u32Src2 );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawNeg(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
                                                            UINT32      carryIn,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMaskedAdd(
    _Inout_updates_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )  PUINT32     pAcc,
    _In_reads_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )       PCUINT32    pSrc,
                                                            UINT32      mask,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawMaskedSub(
    _Inout_updates_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )  PUINT32     pAcc,
    _In_reads_( nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32 )       PCUINT32    pSrc,
                                                            UINT32      mask,
                                                            UINT32      nDigits );

VOID
SYMCRYPT_CALL
SymCryptFdefModDivPow2(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
                                    UINT32                  exp,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModDivSmallPow2(
    _In_                        PCSYMCRYPT_MODULUS      pmMod,
    _In_                        PCSYMCRYPT_MODELEMENT   peSrc,
    _In_range_(1, NATIVE_BITS)  UINT32                  exp,
    _Out_                       PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModDivSmallPow2Asm(
    _In_                        PCSYMCRYPT_MODULUS      pmMod,
    _In_                        PCSYMCRYPT_MODELEMENT   peSrc,
    _In_range_(1, NATIVE_BITS)  UINT32                  exp,
    _Out_                       PSYMCRYPT_MODELEMENT    peDst );

VOID
SYMCRYPT_CALL
SymCryptFdefModDivSmallPow2Mulx(
    _In_                        PCSYMCRYPT_MODULUS      pmMod,
    _In_                        PCSYMCRYPT_MODELEMENT   peSrc,
    _In_range_(1, NATIVE_BITS)  UINT32                  exp,
    _Out_                       PSYMCRYPT_MODELEMENT    peDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdefModInvMontgomery256(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptFdef369ModInvMontgomery(
    _In_                            PCSYMCRYPT_MODULUS      pMod,
    _In_                            PCSYMCRYPT_MODELEMENT   pSrc,
    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptModExpGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_                            PCSYMCRYPT_MODELEMENT   peBase,
    _In_                            PCSYMCRYPT_INT          piExp,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptModMultiExpGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _In_reads_( nBases )            PCSYMCRYPT_MODELEMENT * peBaseArray,
    _In_reads_( nBases )            PCSYMCRYPT_INT *        piExpArray,
                                    UINT32                  nBases,
                                    UINT32                  nBitsExp,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefModSetRandomGeneric(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawAddUint32(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
                                                            UINT32      Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawAddAsm(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdef369RawAddAsm(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawSubAsm(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdef369RawSubAsm(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
                                                            UINT32      nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefRawIsLessThan(
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
                                                            UINT32      nDigits );

VOID
SYMCRYPT_CALL
SymCryptFdefMaskedCopyAsm(
    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
                                                                UINT32      nDigits,
                                                                UINT32      mask );

VOID
SYMCRYPT_CALL
SymCryptFdef369MaskedCopyAsm(
    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
                                                                UINT32      nDigits,
                                                                UINT32      mask );

VOID
SYMCRYPT_CALL
SymCryptFdefRawMulAsm(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquareAsm(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369RawMulAsm(
    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
                                                                    UINT32      nDigits1,
    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
                                                                    UINT32      nDigits2,
    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawMul512Asm(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc1,
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc2,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquare512Asm(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawMul1024Asm(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc1,
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc2,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefRawSquare1024Asm(
    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
                                                        UINT32      nDigits,
    _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32) PUINT32     pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduceAsm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduce256Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduce512Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduce1024Asm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369MontgomeryReduce(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdef369MontgomeryReduceAsm(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduceMulx(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );

VOID
SYMCRYPT_CALL
SymCryptFdefMontgomeryReduceMulx1024(
    _In_                            PCSYMCRYPT_MODULUS      pmMod,
    _Inout_                         PUINT32                 pSrc,
    _Out_                           PUINT32                 pDst );


//=====================================================
// Current state of FIPS tests for asymmetric keys
//=====================================================

// --------------------------------------------------------------------
// Key type |       |
//     &    | Alg   | Description
// Operation|       |
// --------------------------------------------------------------------
// Dlkey    | DH    | Requires use of named safe-prime group (otherwise we cannot perform private
// Generate |       | key range check, or public key order validation).
//          |       |
//          |       | From SP800-56Ar3:
//          |       | Check private key is in the range [1, min(2^nBitsPriv, q)-1]
//          |       |   nBitsPriv is specified either using a default value or using
//          |       |   SymCryptDlkeySetPrivateKeyLength, such that 2s <= nBitsPriv <= nBitsOfQ.
//          |       |   (s is the maximum security strength for a named safe-prime group as
//          |       |   specified in SP800 - 56arev3)
//          |       | Check public key is in the range [2, p-2]
//          |       | Check that (Public key)^q == 1 mod p
//          |       |
//          |       | FIPS 140-3 does not require a further PCT before first use of the key.
//          |-----------------------------------------------------------
//          | DSA   | Requires use of a Dlgroup which has q, but is not a named safe-prime group.
//          |       |
//          |       | FIPS 186-4 and SP800-89 do not require DSA keypair owners to perform
//          |       | validation of keypairs they generate.
//          |       |
//          |       | FIPS 140-3 requires that a module generating a Dlkey keypair for use in DSA
//          |       | must perform a PCT on the keypair before first operational use in DSA.
//          |       | As the Dlgroups supported by FIPS are distinct for DH and DSA, we can perform
//          |       | this PCT on key generation without fear of adverse performance.
// --------------------------------------------------------------------
// Dlkey    | DH    | Requires use of named safe-prime group (otherwise we cannot perform private
// SetValue |       | key range check, or public key order validation).
//          |       |
//          |       | From SP800-56Ar3:
//          |       | If importing a private key:
//          |       |   Check private key is in the range [1, min(2^nBitsPriv, q)-1]
//          |       |     nBitsPriv is specified either using a default value or using
//          |       |     SymCryptDlkeySetPrivateKeyLength, such that 2s <= nBitsPriv <= nBitsOfQ.
//          |       |     (s is the maximum security strength for a named safe-prime group as
//          |       |     specified in SP800-56Arev3)
//          |       |
//          |       | If importing a public key:
//          |       |   Check public key is in the range [2, p-2]
//          |       |   Check that (Public key)^q == 1 mod p
//          |       |
//          |       | If importing both a private and public key, as above and also:
//          |       |   Use the imported Private key to generate a Public key, and check the
//          |       |   generated Public key is equal to the imported Public key.
//          |-----------------------------------------------------------
//          | DSA   | Requires use of a Dlgroup which is not a named safe-prime group.
//          |       |
//          |       | FIPS 184-4 refers to SP800-89:
//          |       | If importing a public key:
//          |       |   Check public key is in the range [2, p-2]
//          |       |   Check that (Public key)^q == 1 mod p
//          |       | If importing a private and public key:
//          |       |   Use the imported Private key to generate a Public key, and check the
//          |       |   generated Public key is equal to the imported Public key.
// --------------------------------------------------------------------
// Eckey    | ECDH  | Requires use of a NIST prime Elliptic Curve (P224, P256, P384, or P521)
// SetRandom|       |
//          |       | From SP800-56Ar3:
//          |       | Check private key is in range [1, GOrd-1]
//          |       | Check public key is nonzero, has coordinates in the underlying field, and is a
//          |       | point on the curve
//          |       | Check that GOrd*(Public key) == O
//          |       |
//          |       | FIPS 140-3 does not require a further PCT before first use of the key
//          |----------------------------------------------------------
//          | ECDSA | Requires use of a NIST prime Elliptic Curve (P224, P256, P384, or P521)
//          |       |
//          |       | FIPS 186-4 and SP800-89 do not require ECDSA keypair owners to perform
//          |       | validation of keypairs they generate.
//          |       |
//          |       | FIPS 140-3 requires that a module generating an Eckey keypair for use in ECDSA
//          |       | must perform a PCT on the keypair before first operational use in ECDSA.
//          |       | As the Elliptic curves used in ECDH and ECDSA are the same, an Eckey may be
//          |       | used for both ECDH and ECDSA. We defer the ECDSA PCT from the EckeySetRandom
//          |       | call to the first use of EcDsaSign, or the first export of the keypair.
// --------------------------------------------------------------------
// Eckey    | ECDH  | Requires use of a NIST prime Elliptic Curve (P224, P256, P384, or P521)
// SetValue |       |
//          |       | From SP800-56Ar3:
//          |       | If importing a private key:
//          |       |   Check private key is in range [1, GOrd-1]
//          |       |
//          |       | If importing a public key:
//          |       |   Check public key is nonzero, has coordinates in the underlying field, and is
//          |       |   a point on the curve
//          |       |   Check that GOrd*(Public key) == O
//          |       |
//          |       | If importing a private and public key:
//          |       |   Use the imported Private key to generate a Public key, and check the
//          |       |   generated Public key is equal to the imported Public key.
//          |----------------------------------------------------------
//          | ECDSA | Requires use of a NIST prime Elliptic Curve (P224, P256, P384, or P521)
//          |       |
//          |       | FIPS 184-4 refers to SP800-89:
//          |       | If importing a public key:
//          |       |   SP800-89 refers to ANS X9.62. Assume same tests required as SP800-56Ar3:
//          |       |   Check public key is nonzero, has coordinates in the underlying field, and is
//          |       |   a point on the curve
//          |       |   Check that GOrd*(Public key) == O
//          |       |
//          |       | If importing a private and public key:
//          |       |   Use the imported Private key to generate a Public key, and check the
//          |       |   generated Public key is equal to the imported Public key.
// --------------------------------------------------------------------
// Rsakey   | RSA   | From FIPS 186-4 (SIGN) and SP800-56Br2 (ENCRYPT for key transport):
// Generate |ENCRYPT| Ensure p and q are in open range (2 ^ ((nBits - 1) / 2), 2 ^ (nBits / 2))
//          | and   | Ensure |p-q| > 2^((nBits/2)-100)
//          | RSA   | Ensure e is coprime with (p-1) and (q-1)
//          | SIGN  | Ensure d is in range [2 ^ (nBits/2) + 1, LCM(p-1,q-1) - 1]
//          |       | Ensure that d*e == 1 mod LCM(p-1,q-1)
//          |       |
//          |       | FIPS 140-3 requires that a module generating an Rsakey keypair for use in an
//          |       | RSA algorithm must perform a PCT on the keypair before first operational use.
//          |       |
//          |       | For ENCRYPT, SP800-56Br2 specifies the PCT to perform as part of key
//          |       | generation is:
//          |       |   Check (m^e)^d == m mod n for some m in range [2, n-2]
//          |       |
//          |       | For SIGN, FIPS 186-4 refers to SP800-89, which does not clearly specify a
//          |       | PCT, but does specify that for an owner to have assurance of Private Key
//          |       | Possession they can sign a message with the private key and validate it with
//          |       | the public key to check they correspond to each other. Notably, this
//          |       | internally will verify (m^d)^e == m mod n for some m (along with testing
//          |       | additional padding logic)
//          |       |
//          |       | FIPS 140-2 explicitly says that only one PCT is required if a keypair may be
//          |       | used in either algorithm, with the module able to choose the PCT.
//          |       | FIPS 140-3 does not say anything specific about only requiring one PCT, but
//          |       | given that mathematically (m^e)^d == (m^ed) == (m^d)^e mod n, our
//          |       | current understanding is that the SIGN PCT works in lieu of the ENCRYPT PCT
//          |       |
//          |       | NOTE: FIPS 140-3 explicitly says that an RSA PCT cannot be used in lieu of an
//          |       | RSA algorithm selftest (CAST)
// --------------------------------------------------------------------
// Rsakey   | RSA   | If importing a keypair (primes and modulus):
// SetValue |ENCRYPT| SP800-56Br2 specifies:
//          |       | Check (m^e)^d mod n == m for some m in range [2, n-2]
//          |       | Check n == p*q
//          |       | Check p and q are in open range (2 ^ ((nBits - 1) / 2), 2 ^ (nBits / 2))
//          |       | Check |p-q| > 2^((nBits/2)-100)
//          |       | Check e is coprime with (p-1) and (q-1)
//          |       | Check p and q are probably prime
//          |       | Check d is in range [2 ^ (nBits/2) + 1, LCM(p-1,q-1) - 1]
//          |       | Check that d*e == 1 mod LCM(p-1,q-1)
//          |       |
//          |       | If importing a public key (only modulus):
//          |       | SP800-56Br2, refers to SP800-89 which details the following Partial Public Key
//          |       | Validation:
//          |       | Check n is odd
//          |       | Check n is not a prime or a power of a prime
//          |       | Check n has no factors smaller than 752
//          |----------------------------------------------------------
//          | RSA   | FIPS 186-4 refers only to SP800-89 which has weaker tests for a keypair than
//          | SIGN  | SP800-56Br2 (i.e. success at SP800-56Br2 tests implies success in SP800-89)
//          |       | The current strategy will be to always perform the stronger tests.
// --------------------------------------------------------------------

// Macro for executing a Cryptographic Algorithm Self-Test (CAST) and setting the corresponding
// flag. These selftests must be run once per algorithm before the algorithm is used. For algorithms
// like hashing and symmetric encryption which have a low performance cost, we run the CASTs when
// the module is loaded. For asymmetric algorithms, we defer the CASTs until the first use of the
// algorithm; hence we need flags to keep track of which CASTs have been run.
#define SYMCRYPT_RUN_SELFTEST_ONCE(AlgorithmSelftestFunction, AlgorithmSelftestFlag) \
if( ( g_SymCryptFipsSelftestsPerformed & AlgorithmSelftestFlag ) == 0 ) \
{ \
    AlgorithmSelftestFunction( ); \
    SYMCRYPT_ATOMIC_OR32_PRE_RELAXED( &g_SymCryptFipsSelftestsPerformed, AlgorithmSelftestFlag ); \
}

// Macros for executing a pairwise consistency test on a key and setting the per-key selftest flag.
// Typically PCTs must be run for each key before the key is first used or exported, but the
// specific requirements vary between algorithms.
//
// Note that a PCT is not considered a CAST and thus does not satisfy the aforementioned requirement
// for algorithm selftests.
#define SYMCRYPT_RUN_KEY_GEN_PCT(KeySelftestFunction, Key, KeySelftestFlag) \
if( ( Key->fAlgorithmInfo & (KeySelftestFlag | SYMCRYPT_FLAG_KEY_NO_FIPS) ) == 0 ) \
{ \
    /* PCT should never fail on key generation - FIPS assert that it does not */ \
    SYMCRYPT_FIPS_ASSERT( KeySelftestFunction( Key ) == SYMCRYPT_NO_ERROR ); \
    SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(&Key->fAlgorithmInfo, KeySelftestFlag); \
}

// Macro to check flag used in fAlgorithmInfo is non-zero and a power of 2
#define CHECK_ALGORITHM_INFO_FLAG_POW2( flag ) \
    C_ASSERT( (flag != 0) && ((flag & (flag-1)) == 0) );

// Macro to check flags used together in fAlgorithmInfo are distinct
#define CHECK_ALGORITHM_INFO_FLAGS_DISTINCT( flag0, flag1, flag2, flag3, flag4 ) \
    C_ASSERT( (flag0 < flag1) && (flag1 < flag2) && (flag2 < flag3) && (flag3 < flag4) );

CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_PCT_DSA);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_PCT_ECDSA);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_PCT_RSA_SIGN);

CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_KEY_NO_FIPS);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION);

CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_DLKEY_DSA);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_DLKEY_DH);

CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_ECKEY_ECDSA);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_ECKEY_ECDH);

CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_RSAKEY_SIGN);
CHECK_ALGORITHM_INFO_FLAG_POW2(SYMCRYPT_FLAG_RSAKEY_ENCRYPT);

CHECK_ALGORITHM_INFO_FLAGS_DISTINCT(SYMCRYPT_PCT_DSA, SYMCRYPT_FLAG_KEY_NO_FIPS, SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION, SYMCRYPT_FLAG_DLKEY_DSA, SYMCRYPT_FLAG_DLKEY_DH);
CHECK_ALGORITHM_INFO_FLAGS_DISTINCT(SYMCRYPT_PCT_ECDSA, SYMCRYPT_FLAG_KEY_NO_FIPS, SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION, SYMCRYPT_FLAG_ECKEY_ECDSA, SYMCRYPT_FLAG_ECKEY_ECDH);
CHECK_ALGORITHM_INFO_FLAGS_DISTINCT(SYMCRYPT_PCT_RSA_SIGN, SYMCRYPT_FLAG_KEY_NO_FIPS, SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION, SYMCRYPT_FLAG_RSAKEY_SIGN, SYMCRYPT_FLAG_RSAKEY_ENCRYPT);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaSignVerifyPct( PCSYMCRYPT_RSAKEY pkRsakey );
//
// FIPS pairwise consistency test for RSA sign/verify.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaPct( PCSYMCRYPT_DLKEY pkDlkey );
//
// FIPS pairwise consistency test for DSA sign/verify.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaPct( PCSYMCRYPT_ECKEY pkEckey );
//
// FIPS pairwise consistency test for ECDSA sign/verify.
//

typedef struct _SYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS {
    SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE eDhSafePrimeType;

    PCBYTE  pcbPrimeP;

    UINT32  nBitsOfP;           // nBitsOfQ == nBitsOfP-1
    UINT32  nMinBitsPriv;       // nMinBitsPriv == 2s
                                // s is the maximum security strength supported by the group based on SP800-56Arev3
    UINT32  nDefaultBitsPriv;   // nBitsOfQ >= nDefaultBitsPriv >= nMinBitsPriv
                                // nDefaultBitsPriv will be the default value of nBitsPriv for a Dlkey in this Dlgroup
                                // nBitsPriv is the maximum length of the private key
} SYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS;
typedef const SYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS * PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS;
//
// SYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS is used to specify all the parameters needed for creation
// of a Dlgroup based on a safe-prime group (i.e. p = 2q+1, and g = 2).
// Currently this is used exclusively internally, and the interface for explicitly specifying use of
// safe-prime group in SymCrypt is to use

// Internally supported Safe Prime groups
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsModp2048;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsModp3072;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsModp4096;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsModp6144;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsModp8192;

extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsffdhe2048;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsffdhe3072;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsffdhe4096;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsffdhe6144;
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptDlgroupDhSafePrimeParamsffdhe8192;

#define SYMCRYPT_DH_SAFEPRIME_GROUP_COUNT (10)

// Note, we rely on the ordering of the parameters from smallest to largest within each named set of
// safe-prime groups as we iterate through them assuming this order in SymCryptDlgroupSetValueSafePrime
extern const PCSYMCRYPT_DLGROUP_DH_SAFEPRIME_PARAMS SymCryptNamedSafePrimeGroups[SYMCRYPT_DH_SAFEPRIME_GROUP_COUNT];

//
// Definitions for ECurve dispatch functions
//
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_SET_ZERO_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_SET_DISTINGUISHED_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_SET_RANDOM_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_INT       piScalar,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef UINT32 (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_ISEQUAL_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

typedef UINT32 (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_ONCURVE_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef UINT32 (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_ISZERO_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_ADD_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_ADD_DIFF_NONZERO_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_DOUBLE_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_NEGATE_FUNC) (
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef SYMCRYPT_ERROR (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_SCALAR_MUL_FUNC) (
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT          piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT      poSrc,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

typedef SYMCRYPT_ERROR (SYMCRYPT_CALL * PSYMCRYPT_ECPOINT_MULTI_SCALAR_MUL_FUNC) (
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_reads_( nPoints )           PCSYMCRYPT_INT *        piSrcScalarArray,
    _In_reads_( nPoints )           PCSYMCRYPT_ECPOINT *    poSrcEcpointArray,
                                    UINT32                  nPoints,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch  );

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_ECURVE_FILL_SCRATCH_SPACES_FUNC) (
    _Inout_    PSYMCRYPT_ECURVE pCurve );


typedef struct _SYMCRYPT_ECURVE_FUNCTIONS
{
    PSYMCRYPT_ECPOINT_SET_ZERO_FUNC             setZeroFunc;
    PSYMCRYPT_ECPOINT_SET_DISTINGUISHED_FUNC    setDistinguishedFunc;
    PSYMCRYPT_ECPOINT_SET_RANDOM_FUNC           setRandomFunc;
    PSYMCRYPT_ECPOINT_ISEQUAL_FUNC              isEqualFunc;
    PSYMCRYPT_ECPOINT_ISZERO_FUNC               isZeroFunc;
    PSYMCRYPT_ECPOINT_ONCURVE_FUNC              onCurveFunc;
    PSYMCRYPT_ECPOINT_ADD_FUNC                  addFunc;
    PSYMCRYPT_ECPOINT_ADD_DIFF_NONZERO_FUNC     addDiffFunc;
    PSYMCRYPT_ECPOINT_DOUBLE_FUNC               doubleFunc;
    PSYMCRYPT_ECPOINT_NEGATE_FUNC               negateFunc;
    PSYMCRYPT_ECPOINT_SCALAR_MUL_FUNC           scalarMulFunc;
    PSYMCRYPT_ECPOINT_MULTI_SCALAR_MUL_FUNC     multiScalarMulFunc;
    PSYMCRYPT_ECURVE_FILL_SCRATCH_SPACES_FUNC   fillScratchSpacesFunc;
    PVOID                                       slack[3];
} SYMCRYPT_ECURVE_FUNCTIONS, *PSYMCRYPT_ECURVE_FUNCTIONS;
typedef const SYMCRYPT_ECURVE_FUNCTIONS  *PCSYMCRYPT_ECURVE_FUNCTIONS;

#define SYMCRYPT_ECURVE_FUNCTIONS_SIZE    (sizeof( SYMCRYPT_ECURVE_FUNCTIONS ) )

// Check that the size is a power of 2
C_ASSERT( (SYMCRYPT_ECURVE_FUNCTIONS_SIZE & (SYMCRYPT_ECURVE_FUNCTIONS_SIZE-1)) == 0 );

//
// Functions for the each type of curve
//

//--------------------------------------------------------
//--------- Short Weierstrass ----------------------------
//--------------------------------------------------------

extern const PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION SymCryptEcurveParamsV2ExtensionShortWeierstrass;

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassFillScratchSpaces( _In_ PSYMCRYPT_ECURVE pCurve );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassSetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptShortWeierstrassOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassAddDiffNonZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptShortWeierstrassDoubleSpecializedAm3(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

//--------------------------------------------------------
//--------- Twisted Edwards ------------------------------
//--------------------------------------------------------

extern const PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION SymCryptEcurveParamsV2ExtensionTwistedEdwards;

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsFillScratchSpaces( _In_ PSYMCRYPT_ECURVE pCurve );

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsSetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsAdd(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsAddDiffNonZero(
     _In_   PCSYMCRYPT_ECURVE   pCurve,
     _In_   PCSYMCRYPT_ECPOINT  poSrc1,
     _In_   PCSYMCRYPT_ECPOINT  poSrc2,
     _Out_  PSYMCRYPT_ECPOINT   poDst,
     _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsDouble(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsOnCurve(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

UINT32
SYMCRYPT_CALL
SymCryptTwistedEdwardsIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsSetZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

VOID
SYMCRYPT_CALL
SymCryptTwistedEdwardsNegate(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Inout_ PSYMCRYPT_ECPOINT   poSrc,
            UINT32              mask,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

//--------------------------------------------------------
//--------- Montgomery -----------------------------------
//--------------------------------------------------------

extern const PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION SymCryptEcurveParamsV2ExtensionMontgomery;

VOID
SYMCRYPT_CALL
SymCryptMontgomeryFillScratchSpaces( _In_ PSYMCRYPT_ECURVE pCurve );

VOID
SYMCRYPT_CALL
SymCryptMontgomerySetDistinguished(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _Out_   PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

UINT32
SYMCRYPT_CALL
SymCryptMontgomeryIsEqual(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc1,
    _In_    PCSYMCRYPT_ECPOINT  poSrc2,
            UINT32              flags,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch);

UINT32
SYMCRYPT_CALL
SymCryptMontgomeryIsZero(
    _In_    PCSYMCRYPT_ECURVE   pCurve,
    _In_    PCSYMCRYPT_ECPOINT  poSrc,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMontgomeryPointScalarMul(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT          piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT      poSrc,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

//--------------------------------------------------------
//--------- Generic multiplication-related functions -----
//--------------------------------------------------------

VOID
SYMCRYPT_CALL
SymCryptOfflinePrecomputation(
    _In_ PSYMCRYPT_ECURVE pCurve,
    _Out_writes_bytes_( cbScratch )
            PBYTE         pbScratch,
            SIZE_T        cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointScalarMulFixedWindow(
    _In_    PCSYMCRYPT_ECURVE       pCurve,
    _In_    PCSYMCRYPT_INT          piScalar,
    _In_opt_
            PCSYMCRYPT_ECPOINT      poSrc,
            UINT32                  flags,
    _Out_   PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch )
            PBYTE               pbScratch,
            SIZE_T              cbScratch );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcpointMultiScalarMulWnafWithInterleaving(
    _In_                            PCSYMCRYPT_ECURVE       pCurve,
    _In_reads_( nPoints )           PCSYMCRYPT_INT *        piSrcScalarArray,
    _In_reads_( nPoints )           PCSYMCRYPT_ECPOINT *    poSrcEcpointArray,
                                    UINT32                  nPoints,
                                    UINT32                  flags,
    _Out_                           PSYMCRYPT_ECPOINT       poDst,
    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
                                    SIZE_T                  cbScratch );

VOID
SYMCRYPT_CALL
SymCryptEcpointGenericSetRandom(
    _In_                            PCSYMCRYPT_ECURVE   pCurve,
    _Out_                           PSYMCRYPT_INT       piScalar,
    _Out_                           PSYMCRYPT_ECPOINT   poDst,
    _Out_writes_bytes_( cbScratch ) PBYTE               pbScratch,
                                    SIZE_T              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptEcurveFillScratchSpaces(
    _Inout_    PSYMCRYPT_ECURVE   pCurve);
//--------------------------------------------------------
//--------------------------------------------------------

// Table with the number of field elements for each point format (in ecpoint.c)
extern const UINT32 SymCryptEcpointFormatNumberofElements[4];

UINT32
SYMCRYPT_CALL
SymCryptSizeofEcpointEx(
    UINT32 cbModElement,
    UINT32 numOfCoordinates );


PCSYMCRYPT_TRIALDIVISION_CONTEXT
SYMCRYPT_CALL
SymCryptFdefCreateTrialDivisionContext( UINT32 nDigits );

UINT32
SYMCRYPT_CALL
SymCryptFdefIntFindSmallDivisor(
    _In_                            PCSYMCRYPT_TRIALDIVISION_CONTEXT    pContext,
    _In_                            PCSYMCRYPT_INT                      piSrc,
    _Out_writes_bytes_( cbScratch ) PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch );

VOID
SYMCRYPT_CALL
SymCryptFdefFreeTrialDivisionContext( PCSYMCRYPT_TRIALDIVISION_CONTEXT pContext );

UINT64
SymCryptInverseMod2e64( UINT64 m );


//--------------------------------------------------------
//--------------------------------------------------------

// Helper function for wiping the Ec key's private state (e.g. for use in layers such as composites)
VOID
SYMCRYPT_CALL
SymCryptEckeyWipePrivateState(
    _Inout_ PSYMCRYPT_ECKEY pkEckey );

// Recoding algorithms
VOID
SYMCRYPT_CALL
SymCryptFixedWindowRecoding(
            UINT32          W,
    _Inout_ PSYMCRYPT_INT   piK,
    _Inout_ PSYMCRYPT_INT   piTmp,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
    _Out_writes_( nRecodedDigits )
            PUINT32         sigofKIs,
            UINT32          nRecodedDigits );

VOID
SYMCRYPT_CALL
SymCryptWidthNafRecoding(
            UINT32          W,
    _Inout_ PSYMCRYPT_INT   piK,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
    _Out_writes_( nRecodedDigits )
            PUINT32         sigofKIs,
            UINT32          nRecodedDigits );

VOID
SYMCRYPT_CALL
SymCryptPositiveWidthNafRecoding(
            UINT32          W,
    _In_    PCSYMCRYPT_INT  piK,
            UINT32          nBitsExp,
    _Out_writes_( nRecodedDigits )
            PUINT32         absofKIs,
            UINT32          nRecodedDigits );

// M-LWE: Module Learning-With-Errors (ML-KEM, ML-DSA)
//
// ML-KEM (also known as Kyber) and ML-DSA (also known as Dilithium) are Post-Quantum algorithms
// based on the Learning-With-Errors problem over Module Lattices (or the hardness of the M-LWE
// problem).
//
// A Module is a Vector Space over a Ring. That is, elements of the vector spaces are elements in
// the underlying ring.
// We refer to Module as MLWE in the below types to avoid naming confusion with Module as in
// "FIPS module". Though technically components acting on MLWE types could be used outside of the
// MLWE problem, these types are SymCrypt-internal, and are only currently intended for use in
// these MLWE-based algorithms.
//
// In ML-KEM and ML-DSA, Polynomial Rings are used. That is, a ring defined over polynomials.
// For both schemes, the polynomial ring is defined modulo the polynomial (X^256 + 1). This means
// there is a representative of each polynomial ring element with 256 coefficients
// (c_255*X^255 + c_254*X^254 + ... + c_0). The coefficients themselves are modulo a small prime
// in both schemes. For ML-KEM the small prime is 3329 (12-bits), and for ML-DSA the small prime
// is 8380417 (23-bits).
// Additionally, for both schemes there is a Number Theoretic Transform (NTT) which maps polynomial
// ring elements to a corresponding ring for efficient multiplication.
// The in-memory representation of a polynomial ring element uses the same struct regardless of
// whether it is in standard form, or the NTT form. For brevity we tend to refer to polynomial
// ring elements as PolyElements.
//
#define SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS (256)

// MLWE internal function definitions are in their own headers
#include "sc_lib_mlkem.h"
#include "sc_lib_mldsa.h"

//
// Common Composite Definitions
//

typedef enum {
    SYMCRYPT_CACHED_ECURVE_ID_NIST_P256 = 0,
    SYMCRYPT_CACHED_ECURVE_ID_NIST_P384,
    SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519,
    SYMCRYPT_CACHED_ECURVE_ID_COUNT
} SYMCRYPT_CACHED_ECURVE_ID, *PSYMCRYPT_CACHED_ECURVE_ID;

PCSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptGetCachedEcurve(
    SYMCRYPT_CACHED_ECURVE_ID   curveId );

#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P256        (65)
#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384        (97)
#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_CURVE_25519 (32)

#define SYMCRYPT_COMPOSITE_SIZEOF_MAX_ENCODED_EC_PUBLIC_KEY SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384

#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P256           (51)
#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P384           (64)
#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_CURVE_25519    (32)

UINT32
SYMCRYPT_CALL
SymCryptCompositeGetSizeOfEncodedEcSk(
    SYMCRYPT_CACHED_ECURVE_ID   curveId );

UINT32
SYMCRYPT_CALL
SymCryptCompositeGetSizeOfEncodedEcPk(
    SYMCRYPT_CACHED_ECURVE_ID   curveId );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyGetValueCompositeEncodingSk(
    _In_                        PCSYMCRYPT_ECKEY            pEckey,
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyGetValueCompositeEncodingPk(
    _In_                        PCSYMCRYPT_ECKEY            pEckey,
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetValueCompositeEncodingPk(
    _In_                        SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_ECKEY             pEckey );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetValueCompositeEncodingSk(
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_ECKEY             pEckey );

//
// Composite ML-KEM definitions
//

typedef struct _SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS {
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS     params;
    SYMCRYPT_CACHED_ECURVE_ID           ecurveId;
    SYMCRYPT_MLKEM_PARAMS               mlKemParams;
    PCBYTE                              pbLabel;
    SIZE_T                              cbLabel;
    SIZE_T                              cbCiphertext;
    SYMCRYPT_NUMBER_FORMAT              numFormat;
    SYMCRYPT_ECPOINT_FORMAT             ecPointFormat;
    SIZE_T                              cbExpandedSeed;
    SIZE_T                              cbEncodedPrivateKey;
    SIZE_T                              cbEncodedPublicKey;
} SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS, *PSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS;
typedef const SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS *PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS;

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_COMPOSITE_MLKEMKEY {
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS  pParams;    // pointer to internal params for Composite ML-KEM being used
    PSYMCRYPT_MLKEMKEY                          pkMlKemkey;
    PSYMCRYPT_ECKEY                             pkEcKey;    // all composite keys with the same elliptic curve type
                                                            // share the same lazily allocated curve object. This
                                                            // avoids the overhead of setting up a new curve object per key.

    BOOLEAN                                     hasPrivateSeed;
    BYTE                                        privateSeed[SYMCRYPT_COMPOSITE_MLKEM_IRTF_PRIVATE_SEED_SIZE];

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_COMPOSITE_MLKEMKEY, *PSYMCRYPT_COMPOSITE_MLKEMKEY;
typedef const SYMCRYPT_COMPOSITE_MLKEMKEY *PCSYMCRYPT_COMPOSITE_MLKEMKEY;

// Rejection sampling for generating an EC scalar from an IRTF Composite ML-KEM seed
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemGetRandomScalarForEcKeyEx(
                                    SYMCRYPT_CACHED_ECURVE_ID   ecurveId,
                                    SYMCRYPT_NUMBER_FORMAT      numFormat,
    _In_reads_bytes_( cbSeed )      PCBYTE                      pbSeed,
                                    SIZE_T                      cbSeed,
    _Out_writes_bytes_( cbScalar )  PBYTE                       pbScalar,
                                    SIZE_T                      cbScalar );

//
// XMSS
//

//
// ADRS structure definitions as specified in RFC 8391
//
typedef enum _XMSS_ADRS_TYPE
{
    XMSS_ADRS_TYPE_OTS          = 0,
    XMSS_ADRS_TYPE_LTREE        = 1,
    XMSS_ADRS_TYPE_HASH_TREE    = 2,
} XMSS_ADRS_TYPE;

typedef struct _XMSS_OTS_ADDRESS
{
    BYTE  en32Leaf[4];
    BYTE  en32Chain[4];
    BYTE  en32Hash[4];
} XMSS_OTS_ADDRESS, *PXMSS_OTS_ADDRESS;

typedef struct _XMSS_LTREE_ADDRESS
{
    BYTE  en32Leaf[4];
    BYTE  en32Height[4];
    BYTE  en32Index[4];
} XMSS_LTREE_ADDRESS, * PXMSS_LTREE_ADDRESS;

typedef struct _XMSS_HASHTREE_ADDRESS
{
    BYTE  padding[4];
    BYTE  en32Height[4];
    BYTE  en32Index[4];
} XMSS_HASHTREE_ADDRESS, * PXMSS_HASHTREE_ADDRESS;

typedef struct _XMSS_ADRS
{
    BYTE  en32Layer[4];
    BYTE  en64Tree[8];
    BYTE  en32Type[4];

    union {
        XMSS_OTS_ADDRESS        ots;
        XMSS_LTREE_ADDRESS      ltree;
        XMSS_HASHTREE_ADDRESS   hashtree;
    } u;

    BYTE  en32KeyAndMask[4];

} XMSS_ADRS, *PXMSS_ADRS;


typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_XMSS_KEY
{
    UINT32  version;

    SYMCRYPT_XMSS_PARAMS params;

    SYMCRYPT_XMSSKEY_TYPE keyType;

    // Public key
    BYTE    Root[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE    Seed[SYMCRYPT_HASH_MAX_RESULT_SIZE];

    SYMCRYPT_MAGIC_FIELD

    // Private key
    SYMCRYPT_ALIGN_AT(16) UINT64  Idx;  // Aligning on 16-bytes to suppress clang warning
                                        // when atomic increment is performed on it.
    BYTE    SkXmss[SYMCRYPT_HASH_MAX_RESULT_SIZE];
    BYTE    SkPrf[SYMCRYPT_HASH_MAX_RESULT_SIZE];

} SYMCRYPT_XMSS_KEY;

typedef SYMCRYPT_XMSS_KEY* PSYMCRYPT_XMSS_KEY;


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssComputePublicRoot(
    _In_                            PCSYMCRYPT_XMSS_PARAMS  pParams,
    _In_reads_bytes_( cbSeed )      PCBYTE                  pbSeed,
                                    SIZE_T                  cbSeed,
    _In_reads_bytes_( cbSkXmss )    PCBYTE                  pbSkXmss,
                                    SIZE_T                  cbSkXmss,
    _Out_writes_bytes_( cbRoot )    PBYTE                   pbRoot,
                                    SIZE_T                  cbRoot );
//
//  Compute public root value from SEED and SK_XMSS
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyVerifyRoot(
    _In_    PCSYMCRYPT_XMSS_KEY pKey );
//
// Verifies that the public root matches the private key by recomputing it
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssVerifyInternal(
    _Inout_                         PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )   PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_( cbSignature ) PCBYTE              pbSignature,
                                    SIZE_T              cbSignature );
//
// The function that actually does the signature verification. This one doesn't
// run the self-tests so that it can be called from the self-test function.
//


VOID
SYMCRYPT_CALL
SymCryptHbsGetWinternitzLengths(
            UINT32  n,      // data size in bytes
            UINT32  w,      // digit length in bits (Winternitz coefficient)
    _Out_   PUINT32 puLen1, // number of w-bit digits in n
    _Out_   PUINT32 puLen2  // number of w-bit digits to store the checksum len1 * (2^w - 1)
    );

typedef struct _SYMCRYPT_TREEHASH_NODE
{
    UINT32  index;
    UINT32  height;
    BYTE    value[SYMCRYPT_ANYSIZE_ARRAY];
} SYMCRYPT_TREEHASH_NODE, * PSYMCRYPT_TREEHASH_NODE;

#define SYMCRYPT_SIZEOF_TREEHASH_NODE(cbValue) (sizeof(SYMCRYPT_TREEHASH_NODE) - 1 + (cbValue))

#define SYMCRYPT_TREEHASH_NODE_GET(aNodes, cbValue, i) ((PSYMCRYPT_TREEHASH_NODE)((PBYTE)(aNodes) + (i) * SYMCRYPT_SIZEOF_TREEHASH_NODE(cbValue)))


typedef struct _SYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT
{
    PCSYMCRYPT_XMSS_PARAMS  pParams;
    PCBYTE                  pbSeed;
    XMSS_ADRS               adrs;

} SYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT, * PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT;


typedef
VOID
(SYMCRYPT_CALL *PSYMCRYPT_INCREMENTAL_TREEHASH_FUNC)(
    _In_    PSYMCRYPT_TREEHASH_NODE pNodeLeft,
    _In_    PSYMCRYPT_TREEHASH_NODE pNodeRight,
    _Out_   PSYMCRYPT_TREEHASH_NODE pNodeOut,
    _Inout_ PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pContext );


typedef struct _SYMCRYPT_INCREMENTAL_TREEHASH
{
    UINT32 cbNode;      // node size; height + hash result
    UINT32 nSize;       // current size of the stack
    UINT32 nCapacity;   // maximum items
    UINT32 nLastLeafIndex;
    PSYMCRYPT_INCREMENTAL_TREEHASH_FUNC funcCompressNodes;
    PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pContext;

    SYMCRYPT_TREEHASH_NODE arrNodes[SYMCRYPT_ANYSIZE_ARRAY];

} SYMCRYPT_INCREMENTAL_TREEHASH, *PSYMCRYPT_INCREMENTAL_TREEHASH;


PSYMCRYPT_INCREMENTAL_TREEHASH
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashInit(
    UINT32  nLeaves,
    PBYTE   pbBuffer,
    SIZE_T  cbBuffer,
    UINT32  cbHashResult,
    PSYMCRYPT_INCREMENTAL_TREEHASH_FUNC funcCompressNodes,
    PSYMCRYPT_XMSS_INCREMENTAL_TREEHASH_CONTEXT pContext);

PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashGetNode(
    _In_ PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash,
         SIZE_T                         index );

PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashAllocNode(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
            UINT32                          nLeafIndex );

VOID
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashGetTopNodes(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
    _Out_   PSYMCRYPT_TREEHASH_NODE         *ppNodeLeft,
    _Out_   PSYMCRYPT_TREEHASH_NODE         *ppNodeRight );

PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashProcessCommon(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash,
            BOOLEAN                         fFinal );

PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashProcess(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH pIncHash);

PSYMCRYPT_TREEHASH_NODE
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashFinalize(
    _Inout_ PSYMCRYPT_INCREMENTAL_TREEHASH  pIncHash);

UINT32
SYMCRYPT_CALL
SymCryptHbsIncrementalTreehashStackDepth(
    UINT32 nLeaves);

SIZE_T
SYMCRYPT_CALL
SymCryptHbsSizeofScratchBytesForIncrementalTreehash(
    UINT32  cbNode,
    UINT32  nLeaves);

UINT32
SYMCRYPT_CALL
SymCryptHbsGetDigit(
            UINT32 width,
    _In_    PCBYTE pbBuffer,
            SIZE_T cbBuffer,
            UINT32 index);

//
// LMS
//
#define SYMCRYPT_IS_VALID_WINTERNITZ_WIDTH(w) ( ((w) == 1) || ((w) == 2) || ((w) == 4) || ((w) == 8) )
#define SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE   16
#define SYMCRYPT_LMS_MAX_N                      32
#define SYMCRYPT_LMS_MAX_P                      265
#define SYMCRYPT_LMS_MAX_H                      25
#define SYMCRYPT_LMS_MAX_CUSTOM_TREE_HEIGHT     31
#define SYMCRYPT_LMS_CHECKSUM_SIZE              16

// LmsAlgId || LmsOtsAlgId ||  I  || RootNode
#define SYMCRYPT_LMS_PUB_KEY_SIZE(cbHashOutput) (8 + SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE + cbHashOutput)

// LmsAlgId || LmsOtsAlgId ||  I  || RootNode || NextUnusedLeaf || Seed
#define SYMCRYPT_LMS_PRIV_KEY_SIZE(cbHashOutput) (SYMCRYPT_LMS_PUB_KEY_SIZE(cbHashOutput) + sizeof(UINT32) + cbHashOutput)

//==========================================================================
//   LMS internal structures
//==========================================================================
typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_LMS_KEY{
    SIZE_T cbSize;
    SYMCRYPT_LMS_PARAMS     params;

    // Leaf number of the next LM-OTS private key that has not yet been used
    UINT64                  nNextUnusedLeaf;

    // The key type, can be: SYMCRYPT_LMSKEY_TYPE_PUBLIC, or SYMCRYPT_LMSKEY_TYPE_PRIVATE
    UINT32                  keyType;

    // Key identifier
    BYTE                    abId[SYMCRYPT_LMS_KEY_PAIR_IDENTIFIER_SIZE];

    // Public key root
    BYTE                    abPublicRoot[SYMCRYPT_LMS_MAX_N];

    // Private key seed
    BYTE                    abSeed[SYMCRYPT_LMS_MAX_N];

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_LMS_KEY;
typedef         SYMCRYPT_LMS_KEY* PSYMCRYPT_LMS_KEY;
typedef const   SYMCRYPT_LMS_KEY* PCSYMCRYPT_LMS_KEY;

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsVerifyInternal(
    _In_                            PCSYMCRYPT_LMS_KEY  pKey,
    _In_reads_bytes_(cbMessage)     PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_(cbSignature)   PCBYTE              pbSignature,
                                    SIZE_T              cbSignature);
//
// This function carries out the actual LMS verification process. It's essential to prevent an infinite
// recursive call in SymCryptLmsVerifySelftest.
//


// Atomics.
//
// We define all our SymCrypt atomics below. Different compilers/environments have different
// intrinsics to handle atomics in different environments.
//
// The SymCrypt atomics take the form SYMCRYPT_ATOMIC_<Operation><Bitsize>_<Return>_<Ordering>
//
// <Operation> is the atomic operation (i.e. LOAD, OR, XOR, AND, ADD, INC, etc.)
// <Bitsize> indicates the bitsize of the values that the atomic operation operates on. Pointers to
// values which atomics operate on must be aligned to the size of the value.
// <Return> takes the value PRE or POST, indicating whether the return value of the atomic is the
// value of the destination before (PRE) or after (POST) the operation was performed. Not used when
// operation is LOAD!
// <Ordering> specifies the memory ordering of the atomic operation in relation to other loads/stores
// and can take one of the following values:
//   RELAXED corresponds to relaxed memory ordering in C++11
//   SEQ_CST corresponds to sequentially consistent memory ordering in C++11
//   ACQUIRE corresponds to acquire memory ordering in C++11
//   RELEASE corresponds to release memory ordering in C++11
//

#if SYMCRYPT_PLATFORM_WINDOWS
#include <intrin.h>

#if SYMCRYPT_CPU_ARM64
// 64b loads are naturally atomic on Arm64
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           SYMCRYPT_FORCE_READ64(_dest)
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) _InterlockedAdd64_nf( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) _InterlockedAdd( (volatile LONG *)(_dest), (LONG)(_val) )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)__ldar64( (volatile UINT64 *)(_dest) ))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   __stlr64( (volatile UINT64 *)(_dest), (UINT64)(_val) )

// For ARM/ARM64, MSVC does not have a dedicated acquire-release CAS intrinsic.
#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_ARM
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedOr64_nf( (volatile LONG64 *)(_dest), 0 )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) _InterlockedAdd64_nf( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) _InterlockedAdd( (volatile LONG *)(_dest), (LONG)(_val) )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr_acq( (volatile LONG *)(_dest), 0 ))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   _InterlockedExchangePointer_rel( (volatile PVOID *)(_dest), (PVOID)(_val) )

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_AMD64
// For MSVC on AMD64, there are no _nf atomic intrinsics
// 64b loads are naturally atomic on AMD64
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           SYMCRYPT_FORCE_READ64(_dest)
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) (_InterlockedExchangeAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) ) + (LONG64)(_val))

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

// Volatile load / store are sufficient for acquire-release semantics on AMD64
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)SYMCRYPT_FORCE_READ64(_dest))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   SYMCRYPT_FORCE_WRITE64(_dest, ((UINT64)(_val)))

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_X86
// For MSVC on x86, there is no 64b atomic load intrinsic - use expected to fail CAS, attempting to set from 0 to 0
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedCompareExchange64( (volatile LONG64 *)(_dest), 0, 0 )
// For MSVC on x86, there are no _nf atomic intrinsics
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )
// For MSVC on x86, there is no 64b atomic add intrinsic
// We could use InterlockedAdd64 function from windows.h if we are using MSVC for Windows, but
// to remove dependency we just define our own inline function using _InterlockedCompareExchange64
FORCEINLINE
LONG64
SymCryptInlineInterlockedAdd64( volatile LONG64* destination, LONG64 value )
{
    LONG64 preValue;
    do {
        preValue = *destination;
    } while (_InterlockedCompareExchange64(destination, preValue + value, preValue) != preValue);

    return preValue + value;
}
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) SymCryptInlineInterlockedAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

// Volatile load / store are sufficient for acquire-release semantics on x86
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)SYMCRYPT_FORCE_READ32(_dest))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   SYMCRYPT_FORCE_WRITE32(_dest, ((UINT32)(_val)))

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#else

// Fallback intended to generically work across all supported platforms for cases where
// we do not make decisions based on CPU architecture, such as no ASM builds. For the most
// part the same as x86 except in cases where the underlying definition relies on pointer size.

#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedCompareExchange64( (volatile LONG64 *)(_dest), 0, 0 )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )

FORCEINLINE
LONG64
SymCryptInlineInterlockedAdd64( volatile LONG64* destination, LONG64 value )
{
    LONG64 preValue;
    do {
        preValue = *destination;
    } while (_InterlockedCompareExchange64(destination, preValue + value, preValue) != preValue);

    return preValue + value;
}
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) SymCryptInlineInterlockedAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

#if defined(_WIN64)
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr64( (volatile LONG64 *)(_dest), 0 ))
#else
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr( (volatile LONG *)(_dest), 0 ))
#endif

#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   _InterlockedExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_val) )

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#endif

#elif SYMCRYPT_GNUC
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           __atomic_load_n( (volatile uint64_t *)(_dest), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   __atomic_fetch_or( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  __atomic_fetch_add( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) __atomic_add_fetch( (volatile uint64_t *)(_dest), (uint64_t)(_val), __ATOMIC_RELAXED )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) __atomic_add_fetch( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_ACQ_REL )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          __atomic_load_n( (volatile void* *)(_dest), __ATOMIC_ACQUIRE )
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   __atomic_store_n( (volatile void* *)(_dest), (void*)(_val), __ATOMIC_RELEASE )

FORCEINLINE
void*
SymCryptAtomicCasPtrAcqRel(
    void** dest,
    void* desired,
    void* expected)
{
    __atomic_compare_exchange_n(
        dest,               // ptr
        &expected,
        desired,
        FALSE,              // weak (set to FALSE => strong)
        __ATOMIC_RELEASE,   // success_memorder
        __ATOMIC_ACQUIRE ); // failure_memorder
    return expected;
}

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    SymCryptAtomicCasPtrAcqRel( (volatile void **)(_dest), (void *)(_exchange), (void *)(_comp) )

#endif

// Inline CAS-128 functions

// BOOLEAN
// SymCryptAtomicCas128Relaxed(
//     _Inout_updates_(2)  PUINT64     destination,
//     _Inout_updates_(2)  PUINT64     expectedValue,
//     _In_reads_(2)       PCUINT64    desiredValue);
// Performs Compare-and-Swap on a 128b memory location.
// Atomically reads destination, compares with expectedValue, and:
//   if they are equal, writes desiredValue to destination, and return TRUE
//   if they are not equal, writes the value read from destination to expectedValue, and returns FALSE
//
// Remarks:
// On success, the value of expectedValue is not guaranteed.
// Only destination is guaranteed to be read and written atomically, expectedValue should be a buffer
// which is only owned by the calling thread.
// destination must be aligned to 16 bytes
//

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64

#if SYMCRYPT_PLATFORM_WINDOWS

#if SYMCRYPT_CPU_ARM64
#define SYMCRYPT_MSVC_CAS128_NF _InterlockedCompareExchange128_nf
#elif SYMCRYPT_CPU_AMD64
#define SYMCRYPT_MSVC_CAS128_NF _InterlockedCompareExchange128
#endif

FORCEINLINE
BOOLEAN
SymCryptAtomicCas128Relaxed(
    _Inout_updates_(2)  PUINT64     destination,
    _Inout_updates_(2)  PUINT64     expectedValue,
    _In_reads_(2)       PCUINT64    desiredValue)
{
    return SYMCRYPT_MSVC_CAS128_NF(
        (volatile LONG64 *)destination,
        (LONG64)desiredValue[1],
        (LONG64)desiredValue[0],
        (LONG64 *) expectedValue );
}

#elif SYMCRYPT_GNUC

FORCEINLINE
BOOLEAN
SymCryptAtomicCas128Relaxed(
    _Inout_updates_(2)  PUINT64     destination,
    _Inout_updates_(2)  PUINT64     expectedValue,
    _In_reads_(2)       PCUINT64    desiredValue)
{
#if SYMCRYPT_CPU_AMD64
    // To avoid dynamically linking libatomic in OpenEnclave, use inline assembly for cmpxchg16b
    // on AMD64. We always need to perform CPU feature detection before we hit this function.
    BOOLEAN result;
    __asm__ __volatile__
    (
        "lock cmpxchg16b %1\n\t"
        "sete %0"
        : "=r" (result)
        , "+m" (*destination)
        , "+d" (expectedValue[1])
        , "+a" (expectedValue[0])
        : "c"  (desiredValue[1])
        , "b"  (desiredValue[0])
        : "cc"
    );
    return result;
#elif SYMCRYPT_CPU_ARM64
    // clang inlines this but GCC dynamically links to libatomic
    // For now, just let the compiler decide, and for ARM64 modules, always allow linking to libatomic
    // We may want to break out into inline asm for LDXP/STXP implementation (v8.0) vs. CASP
    // implementation (v8.1) in future
    return __atomic_compare_exchange(
        (__int128 *)destination,    // ptr
        (__int128 *)expectedValue,  // expected
        (__int128 *)desiredValue,   // desired
        FALSE,                      // weak (set to FALSE => strong)
        __ATOMIC_RELAXED,           // success_memorder
        __ATOMIC_RELAXED);          // failure_memorder
#endif
}

#endif

#endif

FORCEINLINE
UINT32
SymCryptCountTrailingZeros32( UINT32 value )
{
    unsigned long index = 0;
    if( value == 0 )
    {
        return 32;
    }

#if SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM)
    _BitScanForward(&index, value);
#elif SYMCRYPT_GNUC
    index = __builtin_ctz(value);
#else
    while( (value & 1) == 0 )
    {
        index++;
        value >>= 1;
    }
#endif

    return (UINT32)index;
}

FORCEINLINE
UINT32
SymCryptCountTrailingZeros64( UINT64 value )
{
    unsigned long index = 0;
    if( value == 0 )
    {
        return 64;
    }

#if SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64)
    _BitScanForward64(&index, value);
#elif SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM)
    if( ((UINT32)value) == 0 )
    {
        _BitScanForward(&index, (UINT32)(value>>32));
        index += 32;
    } else {
        _BitScanForward(&index, (UINT32)value);
    }

#elif SYMCRYPT_GNUC
    index = __builtin_ctzll(value);
#else
    while( (value & 1) == 0 )
    {
        index++;
        value >>= 1;
    }
#endif

    return (UINT32)index;
}

FORCEINLINE
UINT32
SymCryptCountLeadingZeros32( UINT32 value )
{
    unsigned long zeros = 0;

    if(value == 0)
    {
        return 32;
    }

#if SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64 | SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM)
    _BitScanReverse(&zeros, value);
    zeros = 31 - zeros;
#elif SYMCRYPT_GNUC
    zeros = __builtin_clz(value);
#else
    while( (value & 0x80000000) == 0 )
    {
        zeros++;
        value <<= 1;
    }
#endif

    return (UINT32)zeros;
}

FORCEINLINE
UINT32
SymCryptCountLeadingZeros64( UINT64 value )
{
    unsigned long zeros = 0;

    if(value == 0)
    {
        return 64;
    }

#if SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64)
    _BitScanReverse64(&zeros, value);
    zeros = 63 - zeros;
#elif SYMCRYPT_PLATFORM_WINDOWS && (SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM)
    if( (value >> 32) == 0 )
    {
        _BitScanReverse(&zeros, (UINT32)value);
        zeros = 63 - zeros;
    } else {
        _BitScanReverse(&zeros, (UINT32)(value >> 32));
        zeros = 31 - zeros;
    }
#elif SYMCRYPT_GNUC
    zeros = __builtin_clzll(value);
#else
    while( (value & 0x8000000000000000) == 0 )
    {
        zeros++;
        value <<= 1;
    }
#endif

    return (UINT32)zeros;
}
