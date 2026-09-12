//
// SymCrypt_internal.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This file contains information that is internal to the symcrypt library,
// but which still needs to be known to the compiler to be able to use the library.
// This includes structure declarations and all support for inline implementations
// of some of the library functions.
// Information in this file is not part of the API and can change at any time.
//

#pragma GCC diagnostic ignored "-Wunknown-pragmas"

//
// We use Prefast pragmas, but they are not recognized by the compiler.
// We disable the 'unknown pragma' warning if we are not in prefast mode.
//
#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif

//==============================================================================================
//  PLATFORM/COMPILER DETECTION
//==============================================================================================

#define SYMCRYPT_PLATFORM_WINDOWS 0
#define SYMCRYPT_PLATFORM_APPLE   0 // macOS and other Apple platforms
#define SYMCRYPT_PLATFORM_UNIX    0 // Linux and other Unix-likes, besides macOS. Must support POSIX.

#if defined(_WIN32)
    #undef  SYMCRYPT_PLATFORM_WINDOWS
    #define SYMCRYPT_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #undef  SYMCRYPT_PLATFORM_APPLE
    #define SYMCRYPT_PLATFORM_APPLE 1
#elif (defined(linux) || defined(__unix__))
    #undef  SYMCRYPT_PLATFORM_UNIX
    #define SYMCRYPT_PLATFORM_UNIX 1
#endif

#define SYMCRYPT_MS_VC       0 // Microsoft compiler (cl.exe - Visual Studio/MSBuild)
#define SYMCRYPT_GNUC        0 // GCC and compatible compilers (including Clang)

#if defined(_MSC_VER)
    #undef  SYMCRYPT_MS_VC
    #define SYMCRYPT_MS_VC  1
#elif defined(__GNUC__)
    #undef  SYMCRYPT_GNUC
    #define SYMCRYPT_GNUC 1
#else
    #error Unsupported compiler
#endif

#if SYMCRYPT_MS_VC

// This should go somewhere else. Same in the other #if branches.
#define SYMCRYPT_ANYSIZE_ARRAY               1
#define SYMCRYPT_NOINLINE __declspec(noinline)
#define SYMCRYPT_CDECL __cdecl
#define SYMCRYPT_FASTCALL __fastcall

#define SYMCRYPT_UNALIGNED

#elif SYMCRYPT_GNUC

// Ignore the multi-character character constant warnings
#pragma GCC diagnostic ignored "-Wmultichar"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#define SYMCRYPT_ANYSIZE_ARRAY               1
#define SYMCRYPT_NOINLINE           __attribute__ ((noinline))
#define SYMCRYPT_UNALIGNED
#define SYMCRYPT_CDECL
#define SYMCRYPT_FASTCALL           __attribute__((fastcall))

#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmultichar"
#pragma clang diagnostic ignored "-Wincompatible-function-pointer-types"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

//==============================================================================================
//  PLATFORM SPECIFICS
//==============================================================================================

//
// SYMCRYPT_CALL & SYMCRYPT_ALIGN
//
// SYMCRYPT_CALL is a macro that selects the calling convention used by the library.
// Crypto functions often have to perform very many small operations, and a fast calling convention is
// preferable. We use __fastcall on platforms that support it.
//
// SYMCRYPT_ALIGN is the default alignment for the platform.
// On platforms that have alignment restrictions the default alignment should be large enough that
// an aligned BYTE * can be cast to a pointer to a UINT32 and be used.
//
//
// The SYMCRYPT_IGNORE_PLATFORM macro can be defined to switch off any platform-specific
// optimizations and run just the C implementations.
// The rest of the library uses SYMCRYPT_CPU_* macros to make platform decisions.
//
//
// WARNING: both the library and the calling application must be compiled with the same
// set of flags, as the flags affect things like the structure layout and size and
// the calling convention, both of which need to be in sync between the lib and the caller.
//

//#define SYMCRYPT_IGNORE_PLATFORM        // #defining this flag disables all platform optimizations.

#ifndef __WINE_PE_BUILD
#define SYMCRYPT_IGNORE_PLATFORM
#elif defined __clang_major__ && __clang_major__ < 19
/* clang versions < 19 don't implement target attributes correctly */
#define SYMCRYPT_IGNORE_PLATFORM
#endif

#define SYMCRYPT_CPU_X86            0
#define SYMCRYPT_CPU_AMD64          0
#define SYMCRYPT_CPU_ARM            0
#define SYMCRYPT_CPU_ARM64          0
#define SYMCRYPT_CPU_UNKNOWN        0

#if (defined( _X86_ ) || defined( _M_IX86 ) || defined( __i386__ )) && !defined ( SYMCRYPT_IGNORE_PLATFORM )

#undef  SYMCRYPT_CPU_X86
#define SYMCRYPT_CPU_X86        1

#define SYMCRYPT_CALL           SYMCRYPT_FASTCALL
#define SYMCRYPT_ALIGN_VALUE    4

#ifndef _PREFAST_
#pragma warning(push)
#pragma warning(disable:4359)   // *** Alignment specifier is less than actual alignment
#endif

#elif (defined( _ARM64_ ) || defined( _ARM64EC_ ) || defined( _M_ARM64 ) || defined( __aarch64__ ) || defined(__arm64ec__)) && !defined( SYMCRYPT_IGNORE_PLATFORM )

#undef  SYMCRYPT_CPU_ARM64
#define SYMCRYPT_CPU_ARM64      1
#define SYMCRYPT_CALL
#define SYMCRYPT_ALIGN_VALUE    16

#elif (defined( _AMD64_ ) || defined( _M_AMD64 ) || defined( __amd64__ )) && !defined ( SYMCRYPT_IGNORE_PLATFORM )

#undef  SYMCRYPT_CPU_AMD64
#define SYMCRYPT_CPU_AMD64      1

#define SYMCRYPT_CALL
#define SYMCRYPT_ALIGN_VALUE    16

#elif (defined( _ARM_ ) || defined( _M_ARM ) || defined( __arm__ )) && !defined( SYMCRYPT_IGNORE_PLATFORM )

#undef  SYMCRYPT_CPU_ARM
#define SYMCRYPT_CPU_ARM        1
#define SYMCRYPT_CALL
#define SYMCRYPT_ALIGN_VALUE    8

#elif defined( SYMCRYPT_IGNORE_PLATFORM )

#undef  SYMCRYPT_CPU_UNKNOWN
#define SYMCRYPT_CPU_UNKNOWN    1
#define SYMCRYPT_CALL
#define SYMCRYPT_ALIGN_VALUE    4

#ifndef _PREFAST_
#pragma warning(push)
#pragma warning(disable:4359)   // *** Alignment specifier is less than actual alignment
#endif

#else

#error Unknown CPU platform

#endif   // SYMCRYPT_CALL platforms switch


//
// Datatypes used by the SymCrypt library. This ensures compatibility
// with multiple environments, such as Windows, iOS, and Android.
//

#if SYMCRYPT_PLATFORM_WINDOWS

    //
    // Types included in intsafe.h:
    //      BYTE,
    //      INT16, UINT16,
    //      INT32, UINT32,
    //      INT64, UINT64,
    //      UINT_PTR
    // and macro:
    //      UINT32_MAX
    //
#include <intsafe.h>

#else

#include <stdint.h>

typedef uint8_t         BYTE;

#ifndef UINT32_MAX
#define UINT32_MAX      (0xffffffff)
#endif

#ifndef TRUE
#define TRUE            0x01
#endif

#ifndef FALSE
#define FALSE           0x00
#endif

// Size_t
typedef size_t          SIZE_T;

#ifndef SIZE_T_MAX
#define SIZE_T_MAX      SIZE_MAX
#endif

typedef int                 BOOL;

typedef int8_t              INT8, *PINT8;
typedef int16_t             INT16, *PINT16;
typedef int32_t             INT32, *PINT32;
typedef int64_t             INT64, *PINT64;
typedef uint8_t             UINT8, *PUINT8;
typedef uint16_t            UINT16, *PUINT16;
typedef uint32_t            UINT32, *PUINT32;
typedef uint64_t            UINT64, *PUINT64;

// minwindef.h
typedef char CHAR;

#endif //WIN32

#include <stddef.h>

//
// Pointer types
//
typedef BYTE *          PBYTE;
typedef const BYTE *    PCBYTE;

typedef UINT16 *        PUINT16;
typedef const UINT16 *  PCUINT16;

typedef UINT32 *        PUINT32;
typedef const UINT32 *  PCUINT32;

typedef UINT64 *        PUINT64;
typedef const UINT64 *  PCUINT64;

// Void

#ifndef VOID
#define VOID void
#endif

typedef void *          PVOID;
typedef const void *    PCVOID;

// winnt.h
typedef BYTE  BOOLEAN;

// Useful macros for structs
#define SYMCRYPT_FIELD_OFFSET(type, field)      (offsetof(type, field))
#define SYMCRYPT_FIELD_SIZE(type, field)        (sizeof( ((type *)0)->field ))

#if SYMCRYPT_MS_VC

#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#endif

#else

#undef FORCEINLINE
#define FORCEINLINE static inline

#endif

C_ASSERT( (SYMCRYPT_ALIGN_VALUE & (SYMCRYPT_ALIGN_VALUE - 1 )) == 0 );
#define SYMCRYPT_ALIGN_UP( _p ) ((PBYTE) ( ((SIZE_T) (_p) + SYMCRYPT_ALIGN_VALUE - 1) & ~(SYMCRYPT_ALIGN_VALUE - 1 ) ) )

#if SYMCRYPT_MS_VC
    #define SYMCRYPT_ALIGN_AT(alignment)                 __declspec(align(alignment))
    #define SYMCRYPT_WEAK_SYMBOL
#elif SYMCRYPT_GNUC
    #define SYMCRYPT_ALIGN_AT(alignment)                 __attribute__((aligned(alignment)))
    #define SYMCRYPT_WEAK_SYMBOL                         __attribute__((weak))
#else
    #define SYMCRYPT_ALIGN_AT(alignment)
    #define SYMCRYPT_WEAK_SYMBOL
#endif
#define SYMCRYPT_ALIGN_TYPE_AT(typename, alignment)  typename SYMCRYPT_ALIGN_AT(alignment)
#define SYMCRYPT_ALIGN          SYMCRYPT_ALIGN_AT(SYMCRYPT_ALIGN_VALUE)
#define SYMCRYPT_ALIGN_STRUCT   SYMCRYPT_ALIGN_TYPE_AT(struct, SYMCRYPT_ALIGN_VALUE)
#define SYMCRYPT_ALIGN_UNION    SYMCRYPT_ALIGN_TYPE_AT(union, SYMCRYPT_ALIGN_VALUE)


#define SYMCRYPT_MAX( _a, _b )  ((_a)>(_b)?(_a):(_b))
#define SYMCRYPT_MIN( _a, _b )  ((_a)<(_b)?(_a):(_b))

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
//
// XMM related declarations, used in data structures.
//
#pragma prefast(push)
#pragma prefast(disable: 28251, "Windows headers define _mm_clflush with SAL annotation, Intel header doesn't have SAL annotation leading to inconsistent annotation errors")
#include <emmintrin.h>
#pragma prefast(pop)
#endif


//
// To provide quick error detection we have magic values in all
// our data structures, but only in CHKed builds.
// Our magic value depends on the address of the structure.
// This has the advantage that we detect blind memcpy's of our data structures.
// Memcpy is not supported as it limits what the library is allowed to do.
// Where needed the library provides for copy functions of its internal data structures.
//
#if SYMCRYPT_DEBUG
    #define SYMCRYPT_MAGIC_ENABLED
#endif

#if defined(SYMCRYPT_MAGIC_ENABLED )

#define SYMCRYPT_MAGIC_FIELD        SIZE_T   magic;
#define SYMCRYPT_MAGIC_VALUE( p )   ((SIZE_T) p + 'S1mv' + SYMCRYPT_API_VERSION)


#define SYMCRYPT_SET_MAGIC( p )     {(p)->magic = SYMCRYPT_MAGIC_VALUE( p );}
#define SYMCRYPT_CHECK_MAGIC( p )   {if((p)->magic!=SYMCRYPT_MAGIC_VALUE(p)) SymCryptFatal('magc');}
#define SYMCRYPT_WIPE_MAGIC( p )    {(p)->magic = 0;}

#else

//
// We define the magic field even for FRE builds, because we get too many
// hard-to-debug problems with people who accidentally mix FRE headers with CHKed libraries,
// or the other way around.
// E.g. BitLocker only publishes the FRE version of their library, and building a CHKed binary with
// that FRE lib crashes
//

#define SYMCRYPT_MAGIC_FIELD        SIZE_T   magic;
#define SYMCRYPT_SET_MAGIC( p )
#define SYMCRYPT_CHECK_MAGIC( p )
#define SYMCRYPT_WIPE_MAGIC( p )

#endif

//
// CPU feature detection infrastructure
//

#if !SYMCRYPT_PLATFORM_WINDOWS
    // Forward declarations for CPUID intrinsic replacements
    void __cpuidex(int CPUInfo[4], int InfoType, int ECXValue);
#endif

#if SYMCRYPT_CPU_ARM || SYMCRYPT_CPU_ARM64

#define SYMCRYPT_CPU_FEATURE_NEON           0x01
#define SYMCRYPT_CPU_FEATURE_NEON_AES       0x02
#define SYMCRYPT_CPU_FEATURE_NEON_PMULL     0x04
#define SYMCRYPT_CPU_FEATURE_NEON_SHA256    0x08

#elif SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

//
// We keep the most commonly tested bits in the least significant byte, to make it easier for the compiler to optimize
// There is a many to one relationship between CPUID feature flags and SYMCRYPT_CPU_FEATURE_XXX bits
// since a SYMCRYPT_CPU_FEATURE_XXX could require multiple CPUID features.

#define SYMCRYPT_CPU_FEATURE_SSE2               0x0001          // includes SSE, SSE2
#define SYMCRYPT_CPU_FEATURE_SSSE3              0x0002          // includes SSE, SSE2, SSE3, SSSE3
#define SYMCRYPT_CPU_FEATURE_AESNI              0x0004
#define SYMCRYPT_CPU_FEATURE_PCLMULQDQ          0x0008
#define SYMCRYPT_CPU_FEATURE_AVX2               0x0010          // includes AVX, AVX2 - also indicates support for saving/restoring Ymm registers
#define SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL     0x0020          // if SymCryptSaveXmm() will never fail
#define SYMCRYPT_CPU_FEATURE_SHANI              0x0040
#define SYMCRYPT_CPU_FEATURE_BMI2               0x0080          // MULX, RORX, SARX, SHLX, SHRX

#define SYMCRYPT_CPU_FEATURE_ADX                0x0100          // ADCX, ADOX
#define SYMCRYPT_CPU_FEATURE_RDRAND             0x0200
#define SYMCRYPT_CPU_FEATURE_RDSEED             0x0400
#define SYMCRYPT_CPU_FEATURE_VAES               0x0800          // support for VAES and VPCLMULQDQ (may only be supported on Ymm registers (i.e. Zen3))
#define SYMCRYPT_CPU_FEATURE_AVX512             0x1000          // includes F, VL, DQ, BW (VL allows AVX-512 instructions to be used on Xmm and Ymm registers)
                                                                // also indicates support for saving/restoring additional AVX-512 state

#define SYMCRYPT_CPU_FEATURE_CMPXCHG16B         0x2000          // Compare and Swap 128b value

#endif

typedef UINT32 SYMCRYPT_CPU_FEATURES;

//
// We have two feature fields.
// g_SymCryptCpuFeaturesNotPresent reports with features are not present on the current CPU
// SymCryptCpuFeaturesNeverPresent() is a function that returns a static (compiler-predictable) value,
//  and allows the environment to lock out features in a way that the compiler can optimize away all the code that uses these features.
// Using a function allows the environment macro to forward it to an environment-specific function.
//

extern SYMCRYPT_CPU_FEATURES g_SymCryptCpuFeaturesNotPresent;

SYMCRYPT_CPU_FEATURES
SYMCRYPT_CALL
SymCryptCpuFeaturesNeverPresent(void);

#define SYMCRYPT_CPU_FEATURES_PRESENT( x )   ( ((x) & SymCryptCpuFeaturesNeverPresent()) == 0 && ( (x) & g_SymCryptCpuFeaturesNotPresent ) == 0 )

//
// VOLATILE MEMORY ACCESS
//
// These macros are used to explicitly handle volatile memory access independent of compiler settings.
// If volatile memory is accessed directly without using the appropriate macro, MSVC may emit warning
// C4746, because the volatile semantics depend on the value of the /volatile flag, which can result in
// undesired hardware memory barriers that impact performance.
//
// More info:
// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-c4746?view=msvc-170
// https://docs.microsoft.com/en-us/cpp/build/reference/volatile-volatile-keyword-interpretation?view=msvc-170
//

#if SYMCRYPT_MS_VC  // Microsoft VC++ Compiler

    #if SYMCRYPT_CPU_ARM || SYMCRYPT_CPU_ARM64
        #define SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )    ( __iso_volatile_load8( (const volatile char*)(_p) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )   ( __iso_volatile_load16( (const volatile short*)(_p) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )   ( __iso_volatile_load32( (const volatile int*)(_p) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ64( _p )   ( __iso_volatile_load64( (const volatile __int64*)(_p) ) )

        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )  ( __iso_volatile_store8( (volatile char*)(_p), (_v) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v ) ( __iso_volatile_store16( (volatile short*)(_p), (_v) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v ) ( __iso_volatile_store32( (volatile int*)(_p), (_v) ) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE64( _p, _v ) ( __iso_volatile_store64( (volatile __int64*)(_p), (_v) ) )
    #elif SYMCRYPT_CPU_X86 || SYMCRYPT_CPU_AMD64
        #define SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )    ( *((const volatile BYTE*)  (_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )   ( *((const volatile UINT16*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )   ( *((const volatile UINT32*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ64( _p )   ( *((const volatile UINT64*)(_p)) )

        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )  ( *((volatile BYTE*)  (_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v ) ( *((volatile UINT16*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v ) ( *((volatile UINT32*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE64( _p, _v ) ( *((volatile UINT64*)(_p)) = (_v) )
    #else // Temporary workaround for CMake compilation issues on Windows. Assume X86/ADM64.
        #define SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )    ( *((const volatile BYTE*)  (_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )   ( *((const volatile UINT16*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )   ( *((const volatile UINT32*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ64( _p )   ( *((const volatile UINT64*)(_p)) )

        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )  ( *((volatile BYTE*)  (_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v ) ( *((volatile UINT16*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v ) ( *((volatile UINT32*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE64( _p, _v ) ( *((volatile UINT64*)(_p)) = (_v) )
    #endif

#elif SYMCRYPT_GNUC

    #if !SYMCRYPT_CPU_ARM
        #define SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )    ( *((const volatile BYTE*)  (_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )   ( *((const volatile UINT16*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )   ( *((const volatile UINT32*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ64( _p )   ( *((const volatile UINT64*)(_p)) )

        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )  ( *((volatile BYTE*)  (_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v ) ( *((volatile UINT16*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v ) ( *((volatile UINT32*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE64( _p, _v ) ( *((volatile UINT64*)(_p)) = (_v) )
    #else // SYMCRYPT_CPU_ARM
        #define SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )    ( *((const volatile BYTE*)  (_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )   ( *((const volatile UINT16*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )   ( *((const volatile UINT32*)(_p)) )
        #define SYMCRYPT_INTERNAL_VOLATILE_READ64( p ) ( (UINT64)SYMCRYPT_INTERNAL_VOLATILE_READ32(&((PBYTE)p)[4]) << 32 |  SYMCRYPT_INTERNAL_VOLATILE_READ32(&((PBYTE)p)[0]) )

        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )  ( *((volatile BYTE*)  (_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v ) ( *((volatile UINT16*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v ) ( *((volatile UINT32*)(_p)) = (_v) )
        #define SYMCRYPT_INTERNAL_VOLATILE_WRITE64( p, x ) { \
            SYMCRYPT_INTERNAL_VOLATILE_WRITE32( &((PBYTE)p)[0], (UINT32)((x)    ) );\
            SYMCRYPT_INTERNAL_VOLATILE_WRITE32( &((PBYTE)p)[4], (UINT32)(((UINT64)(x))>>32) );\
            }
    #endif

#else

    #error Unknown compiler

#endif

//
// FORCED MEMORY ACCESS
//
// These macros force a memory access. That is, they require that the memory
// read or write takes place, and do not allow the compiler to optimize the access
// away.
// They provide no other memory ordering requirements, so there are no acquire/release
// semantics, memory barriers, etc.
//
// The generic versions are implemented with a volatile access, but that is inefficient on some platforms
// because it might introduce memory ordering requirements.
//

#define SYMCRYPT_INTERNAL_FORCE_READ8( _p )    SYMCRYPT_INTERNAL_VOLATILE_READ8( _p )
#define SYMCRYPT_INTERNAL_FORCE_READ16( _p )   SYMCRYPT_INTERNAL_VOLATILE_READ16( _p )
#define SYMCRYPT_INTERNAL_FORCE_READ32( _p )   SYMCRYPT_INTERNAL_VOLATILE_READ32( _p )
#define SYMCRYPT_INTERNAL_FORCE_READ64( _p )   SYMCRYPT_INTERNAL_VOLATILE_READ64( _p )

#define SYMCRYPT_INTERNAL_FORCE_WRITE8( _p, _v )  SYMCRYPT_INTERNAL_VOLATILE_WRITE8( _p, _v )
#define SYMCRYPT_INTERNAL_FORCE_WRITE16( _p, _v ) SYMCRYPT_INTERNAL_VOLATILE_WRITE16( _p, _v )
#define SYMCRYPT_INTERNAL_FORCE_WRITE32( _p, _v ) SYMCRYPT_INTERNAL_VOLATILE_WRITE32( _p, _v )
#define SYMCRYPT_INTERNAL_FORCE_WRITE64( _p, _v ) SYMCRYPT_INTERNAL_VOLATILE_WRITE64( _p, _v )

//
// FIXED ENDIANNESS ACCESS
//
// Fixed endianness load and store
// We do this by platform because it affected by both endianness and alignment requirements
// The p pointer is always a pointer to BYTE
//
#if SYMCRYPT_MS_VC  // Microsoft VC++ Compiler
    #define SYMCRYPT_BSWAP16( x ) _byteswap_ushort(x)
    #define SYMCRYPT_BSWAP32( x ) _byteswap_ulong(x)
    #define SYMCRYPT_BSWAP64( x ) _byteswap_uint64(x)
#elif SYMCRYPT_GNUC
    #define SYMCRYPT_BSWAP16( x ) __builtin_bswap16(x)
    #define SYMCRYPT_BSWAP32( x ) __builtin_bswap32(x)
    #define SYMCRYPT_BSWAP64( x ) __builtin_bswap64(x)
#endif

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64


//
// X86, AMD64, ARM, and ARM64 have no alignment restrictions, and are little-endian.
// We do straight store/loads with BSWAPs where required.
// This technically relies upon on undefined behavior, as we assume the compiler will translate
// operations on unaligned pointers to 2, 4, and 8 bytes types to appropriately unaligned store/load
// instructions on these platforms (not just in these macros). This works for all compilers we
// currently use.
//
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST16( p ) SYMCRYPT_BSWAP16( *((UINT16 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST16( p )                 ( *((UINT16 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST32( p ) SYMCRYPT_BSWAP32( *((UINT32 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST32( p )                 ( *((UINT32 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST64( p ) SYMCRYPT_BSWAP64( *((UINT64 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST64( p )                 ( *((UINT64 *)(p)) )

#define SYMCRYPT_INTERNAL_STORE_MSBFIRST16( p, x ) ( *(UINT16 *)(p) = SYMCRYPT_BSWAP16(x) )
#define SYMCRYPT_INTERNAL_STORE_LSBFIRST16( p, x ) ( *(UINT16 *)(p) =                 (x) )
#define SYMCRYPT_INTERNAL_STORE_MSBFIRST32( p, x ) ( *(UINT32 *)(p) = SYMCRYPT_BSWAP32(x) )
#define SYMCRYPT_INTERNAL_STORE_LSBFIRST32( p, x ) ( *(UINT32 *)(p) =                 (x) )
#define SYMCRYPT_INTERNAL_STORE_MSBFIRST64( p, x ) ( *(UINT64 *)(p) = SYMCRYPT_BSWAP64(x) )
#define SYMCRYPT_INTERNAL_STORE_LSBFIRST64( p, x ) ( *(UINT64 *)(p) =                 (x) )

#elif SYMCRYPT_CPU_ARM

//
// Only 64 bit accesses need to be aligned.
//
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST16( p ) SYMCRYPT_BSWAP16( *((UINT16 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST16( p )                 ( *((UINT16 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST32( p ) SYMCRYPT_BSWAP32( *((UINT32 *)(p)) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST32( p )                 ( *((UINT32 *)(p)) )

#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST64( p ) ( (UINT64)SYMCRYPT_INTERNAL_LOAD_MSBFIRST32(&((PBYTE)p)[0]) << 32 | SYMCRYPT_INTERNAL_LOAD_MSBFIRST32(&((PBYTE)p)[4]) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST64( p ) ( (UINT64)SYMCRYPT_INTERNAL_LOAD_LSBFIRST32(&((PBYTE)p)[4]) << 32 | SYMCRYPT_INTERNAL_LOAD_LSBFIRST32(&((PBYTE)p)[0]) )



#define SYMCRYPT_INTERNAL_STORE_MSBFIRST16( p, x ) ( *(UINT16 *)(p) = SYMCRYPT_BSWAP16(x) )
#define SYMCRYPT_INTERNAL_STORE_LSBFIRST16( p, x ) ( *(UINT16 *)(p) =                 (x) )
#define SYMCRYPT_INTERNAL_STORE_MSBFIRST32( p, x ) ( *(UINT32 *)(p) = SYMCRYPT_BSWAP32(x) )
#define SYMCRYPT_INTERNAL_STORE_LSBFIRST32( p, x ) ( *(UINT32 *)(p) =                 (x) )
#define SYMCRYPT_INTERNAL_STORE_MSBFIRST64( p, x ) { \
    SYMCRYPT_INTERNAL_STORE_MSBFIRST32( &((PBYTE)p)[0],(UINT32)(((UINT64)(x))>>32) );\
    SYMCRYPT_INTERNAL_STORE_MSBFIRST32( &((PBYTE)p)[4],(UINT32)(x));\
    }

#define SYMCRYPT_INTERNAL_STORE_LSBFIRST64( p, x ) { \
    SYMCRYPT_INTERNAL_STORE_LSBFIRST32( &((PBYTE)p)[0], (UINT32)((x)    ) );\
    SYMCRYPT_INTERNAL_STORE_LSBFIRST32( &((PBYTE)p)[4], (UINT32)(((UINT64)(x))>>32) );\
    }
#else // unknown platform

//
// These functions have to handle arbitrary alignments too, so we do them byte-by-byte in the
// generic case.
// So far these macros have not been fully tested
//
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST16( p ) ( ((UINT16)((PBYTE)p)[0]) << 8 | ((PBYTE)p)[1] )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST16( p ) ( ((UINT16)((PBYTE)p)[1]) << 8 | ((PBYTE)p)[0] )
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST32( p ) ( (UINT32)SYMCRYPT_INTERNAL_LOAD_MSBFIRST16(&((PBYTE)p)[0]) << 16 | SYMCRYPT_INTERNAL_LOAD_MSBFIRST16(&((PBYTE)p)[2]) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST32( p ) ( (UINT32)SYMCRYPT_INTERNAL_LOAD_LSBFIRST16(&((PBYTE)p)[2]) << 16 | SYMCRYPT_INTERNAL_LOAD_LSBFIRST16(&((PBYTE)p)[0]) )
#define SYMCRYPT_INTERNAL_LOAD_MSBFIRST64( p ) ( (UINT64)SYMCRYPT_INTERNAL_LOAD_MSBFIRST32(&((PBYTE)p)[0]) << 32 | SYMCRYPT_INTERNAL_LOAD_MSBFIRST32(&((PBYTE)p)[4]) )
#define SYMCRYPT_INTERNAL_LOAD_LSBFIRST64( p ) ( (UINT64)SYMCRYPT_INTERNAL_LOAD_LSBFIRST32(&((PBYTE)p)[4]) << 32 | SYMCRYPT_INTERNAL_LOAD_LSBFIRST32(&((PBYTE)p)[0]) )

#define SYMCRYPT_INTERNAL_STORE_MSBFIRST16( p, x ) { \
    ((PBYTE)p)[0] = (BYTE)((x)>> 8);\
    ((PBYTE)p)[1] = (BYTE)((x)    );\
    }

#define SYMCRYPT_INTERNAL_STORE_LSBFIRST16( p, x ) { \
    ((PBYTE)p)[0] = (BYTE)((x)    );\
    ((PBYTE)p)[1] = (BYTE)((x)>> 8);\
    }

#define SYMCRYPT_INTERNAL_STORE_MSBFIRST32( p, x ) { \
    ((PBYTE)p)[0] = (BYTE)((x)>>24);\
    ((PBYTE)p)[1] = (BYTE)((x)>>16);\
    ((PBYTE)p)[2] = (BYTE)((x)>> 8);\
    ((PBYTE)p)[3] = (BYTE)((x)    );\
    }

#define SYMCRYPT_INTERNAL_STORE_LSBFIRST32( p, x ) { \
    ((PBYTE)p)[0] = (BYTE)((x)    );\
    ((PBYTE)p)[1] = (BYTE)((x)>> 8);\
    ((PBYTE)p)[2] = (BYTE)((x)>>16);\
    ((PBYTE)p)[3] = (BYTE)((x)>>24);\
    }

#define SYMCRYPT_INTERNAL_STORE_MSBFIRST64( p, x ) { \
    SYMCRYPT_INTERNAL_STORE_MSBFIRST32( &((PBYTE)p)[0],(UINT32)(((UINT64)(x))>>32) );\
    SYMCRYPT_INTERNAL_STORE_MSBFIRST32( &((PBYTE)p)[4],(UINT32)(x));\
    }

#define SYMCRYPT_INTERNAL_STORE_LSBFIRST64( p, x ) { \
    SYMCRYPT_INTERNAL_STORE_LSBFIRST32( &((PBYTE)p)[0], (UINT32)((x)    ) );\
    SYMCRYPT_INTERNAL_STORE_LSBFIRST32( &((PBYTE)p)[4], (UINT32)(((UINT64)(x))>>32) );\
    }

#endif // platform switch for load/store macros


//==============================================================================================
//  INTERNAL DATA STRUCTURES
//==============================================================================================
//
// Note: we do not use the symbolic names like SYMCRYPT_SHA1_INPUT_BLOCK_SIZE as this
// file is included before that name is defined. Fixing that would make the public API header
// file harder to read by moving the constant away from the associated functions, or forcing
// the header file to use the struct name rather than the typedef. The current solution
// works quite well.
//

//-----------------------------------------------------------------
//     Block cipher description table
// Below are the typedefs for the block cipher description table type
// Callers can use this to define their own block cipher and use the block cipher
// modes.
//

typedef struct _SYMCRYPT_BLOCKCIPHER    SYMCRYPT_BLOCKCIPHER, *PSYMCRYPT_BLOCKCIPHER;
typedef const SYMCRYPT_BLOCKCIPHER  * PCSYMCRYPT_BLOCKCIPHER;

//
// Note that blockSize must be <= 32 and must be a power of two. This is true for all the block ciphers
// implemented in SymCrypt.
//

//
// HASH STATES
//
// All hash states have the same basic structure. This allows all hash implementations to share
// the same buffer management code. Some algorithms might still have optimized buffer management code
// specific for their algorithm, but most algs use the generic code.
// This is especially important for parallel hashing, where the buffer management & parallel organizational
// code are tightly coupled.
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_COMMON_HASH_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[SYMCRYPT_ANYSIZE_ARRAY];  // Size depends on algorithm
    // ...
    // Chaining state                                       // type/location depends on algorithm
    //
} SYMCRYPT_COMMON_HASH_STATE, *PSYMCRYPT_COMMON_HASH_STATE;


//
// SYMCRYPT_MD2_STATE
//
// Data structure that stores the state of an ongoing MD2 computation.
//
// The field names are from RFC 1319.
// It would be more efficient to store only the first 16 bytes of the X array,
// but that would complicate the code and MD2 isn't important enough to add
// extra complications.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD2_CHAINING_STATE
{
    SYMCRYPT_ALIGN  BYTE    C[16];      // State for internal checksum computation
                    BYTE    X[48];      // State for actual hash chaining
} SYMCRYPT_MD2_CHAINING_STATE, *PSYMCRYPT_MD2_CHAINING_STATE;

//
// MD2 hash computation state.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD2_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[16];             // buffer to keep one input block in
                    SYMCRYPT_MD2_CHAINING_STATE     chain;
} SYMCRYPT_MD2_STATE, *PSYMCRYPT_MD2_STATE;
typedef const SYMCRYPT_MD2_STATE *PCSYMCRYPT_MD2_STATE;

//
// SYMCRYPT_MD4_STATE
//
// Data structure that stores the state of an ongoing MD4 computation.
// The buffer contains dataLength % 64 bytes of data.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD4_CHAINING_STATE
{
    UINT32   H[4];
} SYMCRYPT_MD4_CHAINING_STATE, *PSYMCRYPT_MD4_CHAINING_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD4_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[64];             // buffer to keep one input block in
                    SYMCRYPT_MD4_CHAINING_STATE     chain;      // chaining state
} SYMCRYPT_MD4_STATE, *PSYMCRYPT_MD4_STATE;
typedef const SYMCRYPT_MD4_STATE *PCSYMCRYPT_MD4_STATE;


//
// SYMCRYPT_MD5_STATE
//
// Data structure that stores the state of an ongoing MD5 computation.
// The buffer contains dataLength % 64 bytes of data.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD5_CHAINING_STATE
{
    UINT32   H[4];
} SYMCRYPT_MD5_CHAINING_STATE, *PSYMCRYPT_MD5_CHAINING_STATE;


typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MD5_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[64];             // buffer to keep one input block in
                    SYMCRYPT_MD5_CHAINING_STATE     chain;      // chaining state
} SYMCRYPT_MD5_STATE, *PSYMCRYPT_MD5_STATE;
typedef const SYMCRYPT_MD5_STATE *PCSYMCRYPT_MD5_STATE;


//
// SYMCRYPT_SHA1_STATE
//
// Data structure that stores the state of an ongoing SHA1 computation.
// The buffer contains dataLength % 64 bytes of data.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA1_CHAINING_STATE
{
    UINT32   H[5];
} SYMCRYPT_SHA1_CHAINING_STATE, *PSYMCRYPT_SHA1_CHAINING_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA1_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[64];             // buffer to keep one input block in
                    SYMCRYPT_SHA1_CHAINING_STATE    chain;      // chaining state
} SYMCRYPT_SHA1_STATE, *PSYMCRYPT_SHA1_STATE;
typedef const SYMCRYPT_SHA1_STATE *PCSYMCRYPT_SHA1_STATE;


//
// SYMCRYPT_SHA256_STATE
//
// Data structure that stores the state of an ongoing SHA256 computation.
// The buffer contains dataLength % 64 bytes of data.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA256_CHAINING_STATE
{
    SYMCRYPT_ALIGN  UINT32   H[8];
} SYMCRYPT_SHA256_CHAINING_STATE, * PSYMCRYPT_SHA256_CHAINING_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA256_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[64];             // buffer to keep one input block in
                    SYMCRYPT_SHA256_CHAINING_STATE  chain;      // chaining state
} SYMCRYPT_SHA256_STATE, *PSYMCRYPT_SHA256_STATE;
typedef const SYMCRYPT_SHA256_STATE *PCSYMCRYPT_SHA256_STATE;


//
// SYMCRYPT_SHA224_STATE
//
// This is identical to the SHA256 state.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA224_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[64];             // buffer to keep one input block in
                    SYMCRYPT_SHA256_CHAINING_STATE  chain;      // chaining state
} SYMCRYPT_SHA224_STATE, *PSYMCRYPT_SHA224_STATE;
typedef const SYMCRYPT_SHA224_STATE *PCSYMCRYPT_SHA224_STATE;


//
// SYMCRYPT_SHA512_STATE
//
// Data structure that stores the state of an ongoing SHA512 computation.
// The buffer contains dataLength % 128 bytes of data.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA512_CHAINING_STATE
{
    UINT64   H[8];
} SYMCRYPT_SHA512_CHAINING_STATE, *PSYMCRYPT_SHA512_CHAINING_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA512_STATE
{
                    UINT32                          bytesInBuffer;
                    SYMCRYPT_MAGIC_FIELD
                    UINT64                          dataLengthL;            // lower part of msg length
                    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE                            buffer[128];            // buffer to keep one input block in
                    SYMCRYPT_SHA512_CHAINING_STATE  chain;          // chaining state
} SYMCRYPT_SHA512_STATE, *PSYMCRYPT_SHA512_STATE;
typedef const SYMCRYPT_SHA512_STATE *PCSYMCRYPT_SHA512_STATE;


//
// SYMCRYPT_SHA384_STATE
//
// This is identical to the SHA512.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA384_STATE
{
    UINT32                          bytesInBuffer;
    SYMCRYPT_MAGIC_FIELD
    UINT64                          dataLengthL;            // lower part of msg length
    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE            buffer[128];            // buffer to keep one input block in
    SYMCRYPT_SHA512_CHAINING_STATE  chain;          // chaining state
} SYMCRYPT_SHA384_STATE, *PSYMCRYPT_SHA384_STATE;
typedef const SYMCRYPT_SHA384_STATE *PCSYMCRYPT_SHA384_STATE;


//
// SYMCRYPT_SHA512_224_STATE
//
// This is identical to the SHA512.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA512_224_STATE
{
    UINT32                          bytesInBuffer;
    SYMCRYPT_MAGIC_FIELD
    UINT64                          dataLengthL;            // lower part of msg length
    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE            buffer[128];            // buffer to keep one input block in
    SYMCRYPT_SHA512_CHAINING_STATE  chain;          // chaining state
} SYMCRYPT_SHA512_224_STATE, *PSYMCRYPT_SHA512_224_STATE;
typedef const SYMCRYPT_SHA512_224_STATE *PCSYMCRYPT_SHA512_224_STATE;


//
// SYMCRYPT_SHA512_256_STATE
//
// This is identical to the SHA512.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA512_256_STATE
{
    UINT32                          bytesInBuffer;
    SYMCRYPT_MAGIC_FIELD
    UINT64                          dataLengthL;            // lower part of msg length
    UINT64                          dataLengthH;            // upper part of msg length
    SYMCRYPT_ALIGN  BYTE            buffer[128];            // buffer to keep one input block in
    SYMCRYPT_SHA512_CHAINING_STATE  chain;          // chaining state
} SYMCRYPT_SHA512_256_STATE, *PSYMCRYPT_SHA512_256_STATE;
typedef const SYMCRYPT_SHA512_256_STATE *PCSYMCRYPT_SHA512_256_STATE;


//
// SYMCRYPT_KECCAK_STATE
//
// Data structure that stores the state of an ongoing SHA-3 derived algorithm computation.
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_KECCAK_STATE
{
    SYMCRYPT_ALIGN  UINT64  state[25];      // state for Keccak-f[1600] permutation
    UINT32                  inputBlockSize; // rate
    UINT32                  stateIndex;     // position in the state for next merge/extract operation
    UINT8                   paddingValue;   // Keccak padding value
    BOOLEAN                 squeezeMode;    // denotes whether the state is in squeeze mode
} SYMCRYPT_KECCAK_STATE, *PSYMCRYPT_KECCAK_STATE;
typedef const SYMCRYPT_KECCAK_STATE *PCSYMCRYPT_KECCAK_STATE;

//
// SYMCRYPT_SHA3_224_STATE
//
// Data structure that stores the state of an ongoing SHA3-224 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA3_224_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHA3_224_STATE, * PSYMCRYPT_SHA3_224_STATE;
typedef const SYMCRYPT_SHA3_224_STATE* PCSYMCRYPT_SHA3_224_STATE;

//
// SYMCRYPT_SHA3_256_STATE
//
// Data structure that stores the state of an ongoing SHA3-256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA3_256_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHA3_256_STATE, * PSYMCRYPT_SHA3_256_STATE;
typedef const SYMCRYPT_SHA3_256_STATE* PCSYMCRYPT_SHA3_256_STATE;

//
// SYMCRYPT_SHA3_384_STATE
//
// Data structure that stores the state of an ongoing SHA3-384 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA3_384_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHA3_384_STATE, * PSYMCRYPT_SHA3_384_STATE;
typedef const SYMCRYPT_SHA3_384_STATE* PCSYMCRYPT_SHA3_384_STATE;

//
// SYMCRYPT_SHA3_512_STATE
//
// Data structure that stores the state of an ongoing SHA3-512 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHA3_512_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHA3_512_STATE, * PSYMCRYPT_SHA3_512_STATE;
typedef const SYMCRYPT_SHA3_512_STATE* PCSYMCRYPT_SHA3_512_STATE;

//
// SYMCRYPT_SHAKE128_STATE
//
// Data structure that stores the state of an ongoing SHAKE128 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHAKE128_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHAKE128_STATE, * PSYMCRYPT_SHAKE128_STATE;
typedef const SYMCRYPT_SHAKE128_STATE* PCSYMCRYPT_SHAKE128_STATE;

//
// SYMCRYPT_SHAKE256_STATE
//
// Data structure that stores the state of an ongoing SHAKE256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_SHAKE256_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_SHAKE256_STATE, * PSYMCRYPT_SHAKE256_STATE;
typedef const SYMCRYPT_SHAKE256_STATE* PCSYMCRYPT_SHAKE256_STATE;

//
// SYMCRYPT_CSHAKE128_STATE
//
// Data structure that stores the state of an ongoing CSHAKE128 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_CSHAKE128_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_CSHAKE128_STATE, * PSYMCRYPT_CSHAKE128_STATE;
typedef const SYMCRYPT_CSHAKE128_STATE* PCSYMCRYPT_CSHAKE128_STATE;

//
// SYMCRYPT_CSHAKE256_STATE
//
// Data structure that stores the state of an ongoing CSHAKE256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_CSHAKE256_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_CSHAKE256_STATE, * PSYMCRYPT_CSHAKE256_STATE;
typedef const SYMCRYPT_CSHAKE256_STATE* PCSYMCRYPT_CSHAKE256_STATE;

//
// SYMCRYPT_KMAC128_EXPANDED_KEY
//
// Data structure that stores the expanded key for KMAC128.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_KMAC128_EXPANDED_KEY
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_KMAC128_EXPANDED_KEY, * PSYMCRYPT_KMAC128_EXPANDED_KEY;
typedef const SYMCRYPT_KMAC128_EXPANDED_KEY* PCSYMCRYPT_KMAC128_EXPANDED_KEY;

//
// SYMCRYPT_KMAC128_STATE
//
// Data structure that stores the state of an ongoing KMAC128 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_KMAC128_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_KMAC128_STATE, * PSYMCRYPT_KMAC128_STATE;
typedef const SYMCRYPT_KMAC128_STATE* PCSYMCRYPT_KMAC128_STATE;

//
// SYMCRYPT_KMAC256_EXPANDED_KEY
//
// Data structure that stores the expanded key for KMAC256.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_KMAC256_EXPANDED_KEY
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_KMAC256_EXPANDED_KEY, * PSYMCRYPT_KMAC256_EXPANDED_KEY;
typedef const SYMCRYPT_KMAC256_EXPANDED_KEY* PCSYMCRYPT_KMAC256_EXPANDED_KEY;

//
// SYMCRYPT_KMAC256_STATE
//
// Data structure that stores the state of an ongoing KMAC256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_KMAC256_STATE
{
    SYMCRYPT_KECCAK_STATE   ks;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_KMAC256_STATE, * PSYMCRYPT_KMAC256_STATE;
typedef const SYMCRYPT_KMAC256_STATE* PCSYMCRYPT_KMAC256_STATE;


//
// Generic hashing
//

typedef struct _SYMCRYPT_OID {
    UINT32  cbOID;
    _Field_size_( cbOID ) PCBYTE  pbOID;
} SYMCRYPT_OID, *PSYMCRYPT_OID;
typedef const SYMCRYPT_OID *PCSYMCRYPT_OID;

//
// OID lists for the most commonly used hash functions
//

#define SYMCRYPT_MD5_OID_COUNT         (2)
extern const SYMCRYPT_OID SymCryptMd5OidList[SYMCRYPT_MD5_OID_COUNT];

#define SYMCRYPT_SHA1_OID_COUNT        (2)
extern const SYMCRYPT_OID SymCryptSha1OidList[SYMCRYPT_SHA1_OID_COUNT];

#define SYMCRYPT_SHA224_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha224OidList[SYMCRYPT_SHA224_OID_COUNT];

#define SYMCRYPT_SHA256_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha256OidList[SYMCRYPT_SHA256_OID_COUNT];

#define SYMCRYPT_SHA384_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha384OidList[SYMCRYPT_SHA384_OID_COUNT];

#define SYMCRYPT_SHA512_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha512OidList[SYMCRYPT_SHA512_OID_COUNT];

#define SYMCRYPT_SHA512_224_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha512_224OidList[SYMCRYPT_SHA512_224_OID_COUNT];

#define SYMCRYPT_SHA512_256_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha512_256OidList[SYMCRYPT_SHA512_256_OID_COUNT];

#define SYMCRYPT_SHA3_224_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha3_224OidList[SYMCRYPT_SHA3_224_OID_COUNT];

#define SYMCRYPT_SHA3_256_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha3_256OidList[SYMCRYPT_SHA3_256_OID_COUNT];

#define SYMCRYPT_SHA3_384_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha3_384OidList[SYMCRYPT_SHA3_384_OID_COUNT];

#define SYMCRYPT_SHA3_512_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptSha3_512OidList[SYMCRYPT_SHA3_512_OID_COUNT];

#define SYMCRYPT_SHAKE128_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptShake128OidList[SYMCRYPT_SHAKE128_OID_COUNT];

#define SYMCRYPT_SHAKE256_OID_COUNT      (2)
extern const SYMCRYPT_OID SymCryptShake256OidList[SYMCRYPT_SHAKE256_OID_COUNT];

typedef enum _SYMCRYPT_OID_LIST_ID
{
    SYMCRYPT_OID_LIST_ID_NULL           = 0,
    SYMCRYPT_OID_LIST_ID_MD5            = 1,
    SYMCRYPT_OID_LIST_ID_SHA1           = 2,
    SYMCRYPT_OID_LIST_ID_SHA224         = 3,
    SYMCRYPT_OID_LIST_ID_SHA256         = 4,
    SYMCRYPT_OID_LIST_ID_SHA384         = 5,
    SYMCRYPT_OID_LIST_ID_SHA512         = 6,
    SYMCRYPT_OID_LIST_ID_SHA512_224     = 7,
    SYMCRYPT_OID_LIST_ID_SHA512_256     = 8,
    SYMCRYPT_OID_LIST_ID_SHA3_224       = 9,
    SYMCRYPT_OID_LIST_ID_SHA3_256       = 10,
    SYMCRYPT_OID_LIST_ID_SHA3_384       = 11,
    SYMCRYPT_OID_LIST_ID_SHA3_512       = 12,
    SYMCRYPT_OID_LIST_ID_SHAKE128       = 13,
    SYMCRYPT_OID_LIST_ID_SHAKE256       = 14
} SYMCRYPT_OID_LIST_ID;

PCSYMCRYPT_OID
SYMCRYPT_CALL
SymCryptGetOidList( SYMCRYPT_OID_LIST_ID oidId, _Out_opt_ SIZE_T* pCount );
//
// Returns a pointer to the OID list for the specified OID list ID. If pCount is non-NULL, the
// pointed-to value will be set to the number of elements in the OID list.
// Returns NULL if the OID list ID is invalid.
//

typedef union _SYMCRYPT_HASH_STATE
{
    SYMCRYPT_MD2_STATE          md2State;
    SYMCRYPT_MD4_STATE          md4State;
    SYMCRYPT_MD5_STATE          md5State;
    SYMCRYPT_SHA1_STATE         sha1State;
    SYMCRYPT_SHA224_STATE       sha224State;
    SYMCRYPT_SHA256_STATE       sha256State;
    SYMCRYPT_SHA384_STATE       sha384State;
    SYMCRYPT_SHA512_STATE       sha512State;
    SYMCRYPT_SHA512_224_STATE   sha512_224State;
    SYMCRYPT_SHA512_256_STATE   sha512_256State;
    SYMCRYPT_SHA3_224_STATE     sha3_224State;
    SYMCRYPT_SHA3_256_STATE     sha3_256State;
    SYMCRYPT_SHA3_384_STATE     sha3_384State;
    SYMCRYPT_SHA3_512_STATE     sha3_512State;
} SYMCRYPT_HASH_STATE, *PSYMCRYPT_HASH_STATE;
typedef const SYMCRYPT_HASH_STATE *PCSYMCRYPT_HASH_STATE;

#define SYMCRYPT_HASH_MAX_RESULT_SIZE    SYMCRYPT_SHA512_RESULT_SIZE

SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HASH;
SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_PARALLEL_HASH;

typedef struct _SYMCRYPT_HASH SYMCRYPT_HASH, *PSYMCRYPT_HASH;
typedef const SYMCRYPT_HASH  *PCSYMCRYPT_HASH;
typedef struct _SYMCRYPT_PARALLEL_HASH SYMCRYPT_PARALLEL_HASH, *PSYMCRYPT_PARALLEL_HASH;
typedef const SYMCRYPT_PARALLEL_HASH  *PCSYMCRYPT_PARALLEL_HASH;

typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_HASH_INIT_FUNC)             ( PVOID pState );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_HASH_APPEND_FUNC)           ( PVOID pState, PCBYTE pbData, SIZE_T cbData );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_HASH_RESULT_FUNC)           ( PVOID pState, PVOID pbResult );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_HASH_APPEND_BLOCKS_FUNC)    ( PVOID pChain, PCBYTE pbData, SIZE_T cbData, SIZE_T * pcbRemaining );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_HASH_STATE_COPY_FUNC)       ( PCVOID pStateSrc, PVOID pStateDst );

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HASH
{
    PSYMCRYPT_HASH_INIT_FUNC            initFunc;
    PSYMCRYPT_HASH_APPEND_FUNC          appendFunc;
    PSYMCRYPT_HASH_RESULT_FUNC          resultFunc;
    PSYMCRYPT_HASH_APPEND_BLOCKS_FUNC   appendBlockFunc;
    PSYMCRYPT_HASH_STATE_COPY_FUNC      stateCopyFunc;
    UINT32                              stateSize;          // sizeof( hash state )
    UINT32                              resultSize;         // size of hash result
    UINT32                              inputBlockSize;
    UINT32                              chainOffset;        // offset into state structure of the chaining state
    UINT32                              chainSize;          // size of chaining state
} SYMCRYPT_HASH, *PSYMCRYPT_HASH;


//
// Parallel hashing
//

#if SYMCRYPT_CPU_ARM
#define SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM    (3)
#define SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM    (4)
#else
#define SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM    (2)
#define SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM    (8)
#endif

typedef enum _SYMCRYPT_HASH_OPERATION_TYPE {
    SYMCRYPT_HASH_OPERATION_APPEND = 1,
    SYMCRYPT_HASH_OPERATION_RESULT = 2,
} SYMCRYPT_HASH_OPERATION_TYPE;

typedef struct _SYMCRYPT_PARALLEL_HASH_OPERATION    SYMCRYPT_PARALLEL_HASH_OPERATION, *PSYMCRYPT_PARALLEL_HASH_OPERATION;
typedef const SYMCRYPT_PARALLEL_HASH_OPERATION *PCSYMRYPT_PARALLEL_HASH_OPERATION;

struct _SYMCRYPT_PARALLEL_HASH_OPERATION {
                                SIZE_T                              iHash;          // index of hash object into the state array
                                SYMCRYPT_HASH_OPERATION_TYPE        hashOperation;  // operation to be performed
    _Field_size_( cbBuffer )    PBYTE                               pbBuffer;       // data to be hashed, or result buffer
                                SIZE_T                              cbBuffer;       // size of pbData buffer.
                                PSYMCRYPT_PARALLEL_HASH_OPERATION   next;           // internal scratch space; do not use.
};


SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_PARALLEL_HASH_SCRATCH_OPERATION; // as yet unspecified struct
typedef struct _SYMCRYPT_PARALLEL_HASH_SCRATCH_OPERATION
        SYMCRYPT_PARALLEL_HASH_SCRATCH_OPERATION, *PSYMCRYPT_PARALLEL_HASH_SCRATCH_OPERATION;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_PARALLEL_HASH_SCRATCH_STATE {
    PVOID                               hashState;          // the actual hash state
    BYTE                                processingState;
    BYTE                                bytesAlreadyProcessed;  // of the next Append operation
    UINT64                              bytes;              // # bytes left to process on this state
    PSYMCRYPT_PARALLEL_HASH_OPERATION   next;               // next operation to be performed.
    PCBYTE                              pbData;             // data/size of ongoing append operation; this op has already been removed from the next linked list
    SIZE_T                              cbData;
}SYMCRYPT_PARALLEL_HASH_SCRATCH_STATE, *PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE;


//
// The scratch space used by parallel SHA-256 consists of three regions:
// - an array of SYMCRYPT_PARALLEL_HASH_SCRATCH_STATE structures, aligned to SYMCRYPT_ALIGN_VALUE.
// - the work array, an array of pointers to SYMCRYPT_PARALLEL_HASH_SCRATCH_STATEs.
// - an array of 4 + 8 + 64 SIMD vector elements, aligned to the size of those elements.
//
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
#define SYMCRYPT_SIMD_ELEMENT_SIZE  32
#elif SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
#define SYMCRYPT_SIMD_ELEMENT_SIZE  16
#elif SYMCRYPT_CPU_UNKNOWN
#define SYMCRYPT_SIMD_ELEMENT_SIZE  0
#else
#error Unknown CPU
#endif

#define SYMCRYPT_PARALLEL_SHA256_FIXED_SCRATCH  ( (4 + 8 + 64) * SYMCRYPT_SIMD_ELEMENT_SIZE + SYMCRYPT_SIMD_ELEMENT_SIZE - 1  + SYMCRYPT_ALIGN_VALUE - 1 )
#define SYMCRYPT_PARALLEL_SHA384_FIXED_SCRATCH  ( (4 + 8 + 80) * SYMCRYPT_SIMD_ELEMENT_SIZE + SYMCRYPT_SIMD_ELEMENT_SIZE - 1  + SYMCRYPT_ALIGN_VALUE - 1 )
#define SYMCRYPT_PARALLEL_SHA512_FIXED_SCRATCH  ( (4 + 8 + 80) * SYMCRYPT_SIMD_ELEMENT_SIZE + SYMCRYPT_SIMD_ELEMENT_SIZE - 1  + SYMCRYPT_ALIGN_VALUE - 1 )
#define SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH  (sizeof( SYMCRYPT_PARALLEL_HASH_SCRATCH_STATE ) + sizeof( PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE ) )

SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_PARALLEL_HASH;
typedef struct _SYMCRYPT_PARALLEL_HASH SYMCRYPT_PARALLEL_HASH, *PSYMCRYPT_PARALLEL_HASH;
typedef const SYMCRYPT_PARALLEL_HASH  *PCSYMCRYPT_PARALLEL_HASH;

typedef BOOLEAN (SYMCRYPT_CALL * PSYMCRYPT_PARALLEL_HASH_RESULT_FUNC) (PCSYMCRYPT_PARALLEL_HASH pParHash, PSYMCRYPT_COMMON_HASH_STATE pState, PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE pScratch, BOOLEAN *pRes );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_PARALLEL_HASH_RESULT_DONE_FUNC ) (PCSYMCRYPT_PARALLEL_HASH pParHash, PSYMCRYPT_COMMON_HASH_STATE pState, PCSYMRYPT_PARALLEL_HASH_OPERATION pOp);
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_PARALLEL_APPEND_FUNC) (
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
                                            SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes,
    _Out_writes_( cbSimdScratch )           PBYTE                                   pbSimdScratch,
                                            SIZE_T                                  cbSimdScratch );

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_PARALLEL_HASH
{
    PCSYMCRYPT_HASH                             pHash;
    UINT32                                      parScratchFixed;    // fixed scratch size for parallel hash
    PSYMCRYPT_PARALLEL_HASH_RESULT_FUNC         parResult1Func;
    PSYMCRYPT_PARALLEL_HASH_RESULT_FUNC         parResult2Func;
    PSYMCRYPT_PARALLEL_HASH_RESULT_DONE_FUNC    parResultDoneFunc;

    PSYMCRYPT_PARALLEL_APPEND_FUNC              parAppendFunc;
} SYMCRYPT_PARALLEL_HASH, *PSYMCRYPT_PARALLEL_HASH;


//======================================================================================================
// MAC
//


//
// SYMCRYPT_HMAC_MD5_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-MD5.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_MD5_EXPANDED_KEY
{
    SYMCRYPT_MD5_CHAINING_STATE     innerState;
    SYMCRYPT_MD5_CHAINING_STATE     outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_MD5_EXPANDED_KEY, *PSYMCRYPT_HMAC_MD5_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_MD5_EXPANDED_KEY * PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_MD5_STATE
//
// Data structure that encodes an ongoing HMAC-MD5 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_MD5_STATE
{
    SYMCRYPT_MD5_STATE                 hash;
    PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_MD5_STATE, *PSYMCRYPT_HMAC_MD5_STATE;
typedef const SYMCRYPT_HMAC_MD5_STATE *PCSYMCRYPT_HMAC_MD5_STATE;


//
// SYMCRYPT_HMAC_SHA1_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA1.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA1_EXPANDED_KEY
{
    SYMCRYPT_SHA1_CHAINING_STATE    innerState;
    SYMCRYPT_SHA1_CHAINING_STATE    outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA1_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA1_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA1_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA1_STATE
//
// Data structure that encodes an ongoing HMAC-SHA1 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA1_STATE
{
    SYMCRYPT_SHA1_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA1_STATE, *PSYMCRYPT_HMAC_SHA1_STATE;
typedef const SYMCRYPT_HMAC_SHA1_STATE *PCSYMCRYPT_HMAC_SHA1_STATE;


//
// SYMCRYPT_HMAC_SHA224_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA224.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA224_EXPANDED_KEY
{
    SYMCRYPT_SHA256_CHAINING_STATE  innerState;
    SYMCRYPT_SHA256_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA224_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA224_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA224_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA224_STATE
//
// Data structure that encodes an ongoing HMAC-SHA224 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA224_STATE
{
    SYMCRYPT_SHA224_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA224_STATE, *PSYMCRYPT_HMAC_SHA224_STATE;
typedef const SYMCRYPT_HMAC_SHA224_STATE *PCSYMCRYPT_HMAC_SHA224_STATE;


//
// SYMCRYPT_HMAC_SHA256_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA256.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA256_EXPANDED_KEY
{
    SYMCRYPT_SHA256_CHAINING_STATE  innerState;
    SYMCRYPT_SHA256_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA256_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA256_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA256_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA256_STATE
//
// Data structure that encodes an ongoing HMAC-SHA256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA256_STATE
{
    SYMCRYPT_SHA256_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA256_STATE, *PSYMCRYPT_HMAC_SHA256_STATE;
typedef const SYMCRYPT_HMAC_SHA256_STATE *PCSYMCRYPT_HMAC_SHA256_STATE;


//
// SYMCRYPT_HMAC_SHA384_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA384.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA384_EXPANDED_KEY
{
    SYMCRYPT_SHA512_CHAINING_STATE  innerState;
    SYMCRYPT_SHA512_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA384_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA384_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA384_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA384_STATE
//
// Data structure that encodes an ongoing HMAC-SHA384 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA384_STATE
{
    SYMCRYPT_SHA384_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA384_STATE, *PSYMCRYPT_HMAC_SHA384_STATE;
typedef const SYMCRYPT_HMAC_SHA384_STATE *PCSYMCRYPT_HMAC_SHA384_STATE;

//
// SYMCRYPT_HMAC_SHA512_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA512.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_EXPANDED_KEY
{
    SYMCRYPT_SHA512_CHAINING_STATE  innerState;
    SYMCRYPT_SHA512_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA512_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA512_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA512_STATE
//
// Data structure that encodes an ongoing HMAC-SHA512 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_STATE
{
    SYMCRYPT_SHA512_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_STATE, *PSYMCRYPT_HMAC_SHA512_STATE;
typedef const SYMCRYPT_HMAC_SHA512_STATE *PCSYMCRYPT_HMAC_SHA512_STATE;

//
// SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA512_224.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY
{
    SYMCRYPT_SHA512_CHAINING_STATE  innerState;
    SYMCRYPT_SHA512_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA512_224_STATE
//
// Data structure that encodes an ongoing HMAC-SHA512_224 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_224_STATE
{
    SYMCRYPT_SHA512_224_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_224_STATE, *PSYMCRYPT_HMAC_SHA512_224_STATE;
typedef const SYMCRYPT_HMAC_SHA512_224_STATE *PCSYMCRYPT_HMAC_SHA512_224_STATE;

//
// SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA512_256.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY
{
    SYMCRYPT_SHA512_CHAINING_STATE  innerState;
    SYMCRYPT_SHA512_CHAINING_STATE  outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA512_256_STATE
//
// Data structure that encodes an ongoing HMAC-SHA512_256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA512_256_STATE
{
    SYMCRYPT_SHA512_256_STATE                 hash;
    PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY   pKey;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_SHA512_256_STATE, *PSYMCRYPT_HMAC_SHA512_256_STATE;
typedef const SYMCRYPT_HMAC_SHA512_256_STATE *PCSYMCRYPT_HMAC_SHA512_256_STATE;

//
// SYMCRYPT_HMAC_EXPANDED_KEY
//
// Generic HMAC Expanded Key data structure
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_EXPANDED_KEY
{
    PCSYMCRYPT_HASH     pHash;
    SYMCRYPT_HASH_STATE innerState;
    SYMCRYPT_HASH_STATE outerState;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_EXPANDED_KEY, * PSYMCRYPT_HMAC_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_EXPANDED_KEY* PCSYMCRYPT_HMAC_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_STATE
//
// Generic HMAC data structure
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_STATE
{
    PCSYMCRYPT_HMAC_EXPANDED_KEY    pKey;
    SYMCRYPT_HASH_STATE             hash;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_HMAC_STATE, * PSYMCRYPT_HMAC_STATE;
typedef const SYMCRYPT_HMAC_STATE* PCSYMCRYPT_HMAC_STATE;

//
// SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA3-224
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY
{
    SYMCRYPT_HMAC_EXPANDED_KEY  generic;

} SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA3_224_STATE
//
// Data structure that encodes an ongoing HMAC-SHA3-224 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_224_STATE
{
    SYMCRYPT_HMAC_STATE generic;

} SYMCRYPT_HMAC_SHA3_224_STATE, *PSYMCRYPT_HMAC_SHA3_224_STATE;
typedef const SYMCRYPT_HMAC_SHA3_224_STATE *PCSYMCRYPT_HMAC_SHA3_224_STATE;

//
// SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA3-256
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY
{
    SYMCRYPT_HMAC_EXPANDED_KEY  generic;

} SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA3_256_STATE
//
// Data structure that encodes an ongoing HMAC-SHA3-256 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_256_STATE
{
    SYMCRYPT_HMAC_STATE generic;

} SYMCRYPT_HMAC_SHA3_256_STATE, *PSYMCRYPT_HMAC_SHA3_256_STATE;
typedef const SYMCRYPT_HMAC_SHA3_256_STATE *PCSYMCRYPT_HMAC_SHA3_256_STATE;

//
// SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA3-384
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY
{
    SYMCRYPT_HMAC_EXPANDED_KEY  generic;

} SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA3_384_STATE
//
// Data structure that encodes an ongoing HMAC-SHA3-384 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_384_STATE
{
    SYMCRYPT_HMAC_STATE generic;

} SYMCRYPT_HMAC_SHA3_384_STATE, *PSYMCRYPT_HMAC_SHA3_384_STATE;
typedef const SYMCRYPT_HMAC_SHA3_384_STATE *PCSYMCRYPT_HMAC_SHA3_384_STATE;

//
// SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY
//
// Data structure to store an expanded key for HMAC-SHA3-512
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY
{
    SYMCRYPT_HMAC_EXPANDED_KEY  generic;

} SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY, *PSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY;
typedef const SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY * PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY;

//
// SYMCRYPT_HMAC_SHA3_512_STATE
//
// Data structure that encodes an ongoing HMAC-SHA3-512 computation.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_HMAC_SHA3_512_STATE
{
    SYMCRYPT_HMAC_STATE generic;

} SYMCRYPT_HMAC_SHA3_512_STATE, *PSYMCRYPT_HMAC_SHA3_512_STATE;
typedef const SYMCRYPT_HMAC_SHA3_512_STATE *PCSYMCRYPT_HMAC_SHA3_512_STATE;

//
// SYMCRYPT_AES_EXPANDED_KEY
//
// Expanded key for AES operations.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_AES_EXPANDED_KEY {
    SYMCRYPT_ALIGN BYTE RoundKey[29][4][4];
        // Round keys, first the encryption round keys in encryption order,
        // followed by the decryption round keys in decryption order.
        // The first decryption round key is the last encryption round key.
        // AES-256 has 14 rounds and thus 15 round keys for encryption and 15
        // for decryption. As they share one round key, we need room for 29.
    BYTE   (*lastEncRoundKey)[4][4];    // Pointer to last encryption round key
                                        // also the first round key for decryption
    BYTE   (*lastDecRoundKey)[4][4];    // Pointer to last decryption round key.

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_AES_EXPANDED_KEY, *PSYMCRYPT_AES_EXPANDED_KEY;
typedef const SYMCRYPT_AES_EXPANDED_KEY * PCSYMCRYPT_AES_EXPANDED_KEY;

//
// AES-CMAC
//
// Note: SYMCRYPT_AES_BLOCK_SIZE is not yet defined, so we use
// literal constants instead.
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_AES_CMAC_EXPANDED_KEY
{
    SYMCRYPT_AES_EXPANDED_KEY   aesKey;
    BYTE                        K1[16];
    BYTE                        K2[16];
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_AES_CMAC_EXPANDED_KEY, *PSYMCRYPT_AES_CMAC_EXPANDED_KEY;
typedef const SYMCRYPT_AES_CMAC_EXPANDED_KEY * PCSYMCRYPT_AES_CMAC_EXPANDED_KEY;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_AES_CMAC_STATE
{
    BYTE                                chain[16];
    BYTE                                buf[16];
    SIZE_T                              bytesInBuf;
    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY    pKey;

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_AES_CMAC_STATE, *PSYMCRYPT_AES_CMAC_STATE;
typedef const SYMCRYPT_AES_CMAC_STATE * PCSYMCRYPT_AES_CMAC_STATE;

//
// POLY1305
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_POLY1305_STATE
{
    UINT32  r[4];       // R := \sum 2^{32*i} r[i]. R is already clamped.
    UINT32  s[4];       // S := \sum 2^{32*i} s[i]
    UINT32  a[5];       // Accumulator := sum 2^{32*i} a[i], a[4] <= approx 8
    SIZE_T  bytesInBuffer;
    BYTE    buf[16];    // Partial block buffer

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_POLY1305_STATE, *PSYMCRYPT_POLY1305_STATE;

//
// XTS-AES
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_XTS_AES_EXPANDED_KEY
{
    SYMCRYPT_AES_EXPANDED_KEY   key1;
    SYMCRYPT_AES_EXPANDED_KEY   key2;
} SYMCRYPT_XTS_AES_EXPANDED_KEY, *PSYMCRYPT_XTS_AES_EXPANDED_KEY;
typedef const SYMCRYPT_XTS_AES_EXPANDED_KEY * PCSYMCRYPT_XTS_AES_EXPANDED_KEY;


//-----------------------------------------------------------------
//     Mac description table
// Below are the typedefs for the Mac description table type
// Callers can use this to define Mac algorithm they want to use
//

#define SYMCRYPT_MAC_MAX_RESULT_SIZE    SYMCRYPT_HMAC_SHA512_RESULT_SIZE

typedef union _SYMCRYPT_MAC_STATE
{
    SYMCRYPT_HMAC_MD5_STATE         md5State;
    SYMCRYPT_HMAC_SHA1_STATE        sha1State;
    SYMCRYPT_HMAC_SHA224_STATE      sha224State;
    SYMCRYPT_HMAC_SHA256_STATE      sha256State;
    SYMCRYPT_HMAC_SHA384_STATE      sha384State;
    SYMCRYPT_HMAC_SHA512_STATE      sha512State;
    SYMCRYPT_HMAC_SHA512_224_STATE  sha512_224State;
    SYMCRYPT_HMAC_SHA512_256_STATE  sha512_256State;
    SYMCRYPT_HMAC_SHA3_224_STATE    sha3_224State;
    SYMCRYPT_HMAC_SHA3_256_STATE    sha3_256State;
    SYMCRYPT_HMAC_SHA3_384_STATE    sha3_384State;
    SYMCRYPT_HMAC_SHA3_512_STATE    sha3_512State;
    SYMCRYPT_AES_CMAC_STATE         aescmacState;
    SYMCRYPT_KMAC128_STATE          kmac128State;
    SYMCRYPT_KMAC256_STATE          kmac256State;
} SYMCRYPT_MAC_STATE, *PSYMCRYPT_MAC_STATE;
typedef const SYMCRYPT_MAC_STATE *PCSYMCRYPT_MAC_STATE;

typedef union _SYMCRYPT_MAC_EXPANDED_KEY
{
    SYMCRYPT_HMAC_MD5_EXPANDED_KEY          md5Key;
    SYMCRYPT_HMAC_SHA1_EXPANDED_KEY         sha1Key;
    SYMCRYPT_HMAC_SHA224_EXPANDED_KEY       sha224Key;
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY       sha256Key;
    SYMCRYPT_HMAC_SHA384_EXPANDED_KEY       sha384Key;
    SYMCRYPT_HMAC_SHA512_EXPANDED_KEY       sha512Key;
    SYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY   sha512_224Key;
    SYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY   sha512_256Key;
    SYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY     sha3_224Key;
    SYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY     sha3_256Key;
    SYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY     sha3_384Key;
    SYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY     sha3_512Key;
    SYMCRYPT_AES_CMAC_EXPANDED_KEY          aescmacKey;
    SYMCRYPT_KMAC128_EXPANDED_KEY           kmac128Key;
    SYMCRYPT_KMAC256_EXPANDED_KEY           kmac256Key;
} SYMCRYPT_MAC_EXPANDED_KEY, *PSYMCRYPT_MAC_EXPANDED_KEY;
typedef const SYMCRYPT_MAC_EXPANDED_KEY *PCSYMCRYPT_MAC_EXPANDED_KEY;

typedef SYMCRYPT_ERROR (SYMCRYPT_CALL * PSYMCRYPT_MAC_EXPAND_KEY)
                                        ( PVOID pExpandedKey, PCBYTE pbKey, SIZE_T cbKey );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_MAC_INIT)     ( PVOID pState,  PCVOID pExpandedKey );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_MAC_APPEND)( PVOID pState, PCBYTE pbData, SIZE_T cbData );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_MAC_RESULT)  ( PVOID pState, PVOID pbResult );
typedef VOID (SYMCRYPT_CALL * PSYMCRYPT_MAC_RESULT_EX)  ( PVOID pState, PVOID pbResult, SIZE_T cbResult );

typedef struct _SYMCRYPT_MAC
{
    PSYMCRYPT_MAC_EXPAND_KEY    expandKeyFunc;
    PSYMCRYPT_MAC_INIT          initFunc;
    PSYMCRYPT_MAC_APPEND        appendFunc;
    PSYMCRYPT_MAC_RESULT        resultFunc;
    SIZE_T                      expandedKeySize;
    SIZE_T                      stateSize;
    SIZE_T                      resultSize;
    const PCSYMCRYPT_HASH     * ppHashAlgorithm;            // NULL for MACs not based on hashes
    UINT32                      outerChainingStateOffset;   // Offset into expanded key of outer chaining state; 0 for non-HMAC algorithms
} SYMCRYPT_MAC, *PSYMCRYPT_MAC;
typedef const SYMCRYPT_MAC  *PCSYMCRYPT_MAC;



//
// 3DES
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_3DES_EXPANDED_KEY {
    UINT32  roundKey[3][16][2];     // 3 keys, 16 rounds, 2 UINT32s/round
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_3DES_EXPANDED_KEY, *PSYMCRYPT_3DES_EXPANDED_KEY;
typedef const SYMCRYPT_3DES_EXPANDED_KEY * PCSYMCRYPT_3DES_EXPANDED_KEY;

//
// DES
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_DES_EXPANDED_KEY {
    SYMCRYPT_3DES_EXPANDED_KEY threeDes;
} SYMCRYPT_DES_EXPANDED_KEY, *PSYMCRYPT_DES_EXPANDED_KEY;
typedef const SYMCRYPT_DES_EXPANDED_KEY * PCSYMCRYPT_DES_EXPANDED_KEY;

//
// DESX
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_DESX_EXPANDED_KEY {
    SYMCRYPT_DES_EXPANDED_KEY   desKey;
    BYTE                        inputWhitening[8];
    BYTE                        outputWhitening[8];
} SYMCRYPT_DESX_EXPANDED_KEY, *PSYMCRYPT_DESX_EXPANDED_KEY;
typedef const SYMCRYPT_DESX_EXPANDED_KEY * PCSYMCRYPT_DESX_EXPANDED_KEY;

//
// RC2
//
typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_RC2_EXPANDED_KEY {
    UINT16  K[64];
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_RC2_EXPANDED_KEY, *PSYMCRYPT_RC2_EXPANDED_KEY;
typedef const SYMCRYPT_RC2_EXPANDED_KEY * PCSYMCRYPT_RC2_EXPANDED_KEY;


//
// CCM states for incremental computations
//
#define SYMCRYPT_CCM_BLOCK_SIZE (16)

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_CCM_STATE {
                                                    PCSYMCRYPT_BLOCKCIPHER  pBlockCipher;
                                                    PCVOID                  pExpandedKey;
                                                    UINT64                  cbData;                                     // exact length of data
                                                    SIZE_T                  cbTag;
                                                    SIZE_T                  cbNonce;
                                                    SIZE_T                  cbCounter;                                  // # bytes in counter field
                                                    UINT64                  bytesProcessed;                             // data bytes processed so far
    _Field_range_( 0, SYMCRYPT_CCM_BLOCK_SIZE-1 )   SIZE_T                  bytesInMacBlock;
                                                    SYMCRYPT_ALIGN BYTE     counterBlock[SYMCRYPT_CCM_BLOCK_SIZE];      // Current counter block value
                                                    SYMCRYPT_ALIGN BYTE     macBlock[SYMCRYPT_CCM_BLOCK_SIZE];          // Current state of the CBC-MAC part of CCM
                                                    SYMCRYPT_ALIGN BYTE     keystreamBlock[SYMCRYPT_CCM_BLOCK_SIZE];    // Remaining key stream if partial block has been processed
                                                    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_CCM_STATE, *PSYMCRYPT_CCM_STATE;


//
// GHash & GCM
//

typedef union _SYMCRYPT_GCM_SUPPORTED_BLOCKCIPHER_KEYS
{
    SYMCRYPT_AES_EXPANDED_KEY aes;
} SYMCRYPT_GCM_SUPPORTED_BLOCKCIPHER_KEYS;

#define SYMCRYPT_GCM_BLOCKCIPHER_KEY_SIZE sizeof( union _SYMCRYPT_GCM_SUPPORTED_BLOCKCIPHER_KEYS )

#define SYMCRYPT_GF128_FIELD_SIZE   (128)
#define SYMCRYPT_GF128_BLOCK_SIZE   (16)        // # bytes in a field element/block
#define SYMCRYPT_GCM_BLOCK_SIZE     (16)
#define SYMCRYPT_GCM_MAX_KEY_SIZE   (32)


#define SYMCRYPT_GCM_MAX_DATA_SIZE           (((UINT64)1 << 36) - 32)

#define SYMCRYPT_GCM_BLOCK_MOD_MASK      (SYMCRYPT_GCM_BLOCK_SIZE - 1)
#define SYMCRYPT_GCM_BLOCK_ROUND_MASK    (~SYMCRYPT_GCM_BLOCK_MOD_MASK)

#if SYMCRYPT_CPU_X86
    //
    // x86 needs extra alignment of the GHASH expanded key to support
    // aligned (fast) XMM access. AMD64 has enough natural alignment to
    // achieve this.
    //
    #define SYMCRYPT_GHASH_EXTRA_KEY_ALIGNMENT
#endif

#define SYMCRYPT_GHASH_ALLOW_XMM    (SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64)
#define SYMCRYPT_GHASH_ALLOW_NEON   (SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64)


#if SYMCRYPT_CPU_ARM
#include <arm_neon.h>
#if SYMCRYPT_GNUC || defined(__clang__)
    #define __n128 uint32x4_t
    #define __n64 uint64x1_t
#endif

#elif SYMCRYPT_CPU_ARM64

    #if SYMCRYPT_MS_VC && !defined(__clang__)
        #include <arm64_neon.h>

        // See section 6.7.8 of the C standard for details on this initializer usage.
        #define SYMCRYPT_SET_N128_U64(d0, d1) \
            ((__n128) {.n128_u64 = {d0, d1}})
        #define SYMCRYPT_SET_N64_U64(d0) \
            ((__n64) {.n64_u64 = {d0}})
        #define SYMCRYPT_SET_N128_U8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
            ((__n128) {.n128_u8 = {b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}})
    #else
        #include <arm_neon.h>

        #define __n128 uint8x16_t
        #define __n64 uint8x8_t

        #define SYMCRYPT_SET_N128_U64(d0, d1) \
            ((__n128) ((uint64x2_t) {d0, d1}))
        #define SYMCRYPT_SET_N64_U64(d0) \
            ((__n64) ((uint64x1_t) {d0}))
        #define SYMCRYPT_SET_N128_U8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15) \
            ((__n128) ((uint8x16_t) {b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15}))

        #define vmullq_p64( a, b )      ((__n128) vmull_p64(vgetq_lane_p64((poly64x2_t)a, 0), vgetq_lane_p64((poly64x2_t)b, 0)))
        #define vmull_p64( a, b )       ((__n128) vmull_p64( (poly64_t)a, (poly64_t)b ))
        #define vmull_high_p64( a, b )  ((__n128) vmull_high_p64( (poly64x2_t)a, (poly64x2_t)b ))
    #endif

#endif

//
// All platforms use the same in-memory representation:
// elements of GF(2^128) stored as two 64-bit integers which are best
// interpreted as a single 128-bit integer, least significant half first.
// Note: the actual GF(2^128) bit order is reversed in the standard
// for some reason; the
// polynomial \sum b_i x^i is represented by integer \sum b_i 2^{127-i})
// On x86/amd64 the same in-memory byte structure is also accessed as an
// __m128i, which works as both the UINT64s, UINT32s, and the __m128i use
// LSBfirst convention.
//
typedef SYMCRYPT_ALIGN_UNION _SYMCRYPT_GF128_ELEMENT {
    UINT64 ull[2];
#if SYMCRYPT_GHASH_ALLOW_XMM
    //
    // The XMM code accesses this both as UINT32[] and __m128i
    // This is safe as XMM code only runs on little endian machines so the
    // ordering is known.
    //
    __m128i     m128i;
    UINT32      ul[4];
#endif
#if SYMCRYPT_GHASH_ALLOW_NEON
    __n128      n128;
    UINT32      ul[4];
#endif
} SYMCRYPT_GF128_ELEMENT, *PSYMCRYPT_GF128_ELEMENT;
typedef const SYMCRYPT_GF128_ELEMENT * PCSYMCRYPT_GF128_ELEMENT;



typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_GHASH_EXPANDED_KEY {
#if defined( SYMCRYPT_GHASH_EXTRA_KEY_ALIGNMENT )
    UINT32  tableOffset;
    BYTE    tableSpace[ (SYMCRYPT_GF128_FIELD_SIZE + 1) * sizeof( SYMCRYPT_GF128_ELEMENT ) ];
#else
    SYMCRYPT_GF128_ELEMENT  table[ SYMCRYPT_GF128_FIELD_SIZE ];
#endif
} SYMCRYPT_GHASH_EXPANDED_KEY, *PSYMCRYPT_GHASH_EXPANDED_KEY;
typedef const SYMCRYPT_GHASH_EXPANDED_KEY * PCSYMCRYPT_GHASH_EXPANDED_KEY;


typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_GCM_EXPANDED_KEY {
    SYMCRYPT_GHASH_EXPANDED_KEY             ghashKey;
    PCSYMCRYPT_BLOCKCIPHER                  pBlockCipher;
    SYMCRYPT_GCM_SUPPORTED_BLOCKCIPHER_KEYS blockcipherKey;
    SIZE_T                                  cbKey;
    BYTE                                    abKey[SYMCRYPT_GCM_MAX_KEY_SIZE];
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_GCM_EXPANDED_KEY, * PSYMCRYPT_GCM_EXPANDED_KEY;
typedef const SYMCRYPT_GCM_EXPANDED_KEY * PCSYMCRYPT_GCM_EXPANDED_KEY;


typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_GCM_STATE {
                                                    PCSYMCRYPT_GCM_EXPANDED_KEY pKey;
                                                    UINT64                      cbData;         // Number of data bytes
                                                    UINT64                      cbAuthData;     // Number of AAD bytes
    _Field_range_( 0, SYMCRYPT_GCM_BLOCK_SIZE-1 )   SIZE_T                      bytesInMacBlock;
                                                    SYMCRYPT_GF128_ELEMENT      ghashState;
                                                    SYMCRYPT_ALIGN BYTE         counterBlock[SYMCRYPT_GCM_BLOCK_SIZE];
                                                    SYMCRYPT_ALIGN BYTE         macBlock[SYMCRYPT_GCM_BLOCK_SIZE];
                                                    SYMCRYPT_ALIGN BYTE         keystreamBlock[SYMCRYPT_GCM_BLOCK_SIZE];
                                                    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_GCM_STATE, * PSYMCRYPT_GCM_STATE;
typedef const SYMCRYPT_GCM_STATE * PCSYMCRYPT_GCM_STATE;


//
// Block ciphers
//
#define SYMCRYPT_MAX_BLOCK_SIZE  (32)        // max block length of a block cipher.

typedef SYMCRYPT_ERROR( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY )
(PVOID pExpandedKey, PCBYTE pbKey, SIZE_T cbKey);
typedef VOID( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_CRYPT )         (PCVOID pExpandedKey, PCBYTE pbSrc, PBYTE pbDst);
typedef VOID( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB )     (PCVOID pExpandedKey, PCBYTE pbSrc, PBYTE pbDst, SIZE_T cbData);
typedef VOID( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE )    (PCVOID pExpandedKey, PBYTE pbChainingValue, PCBYTE pbSrc, PBYTE pbDst, SIZE_T cbData);
typedef VOID( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_MAC_MODE )      (PCVOID pExpandedKey, PBYTE pbChainingValue, PCBYTE pbSrc, SIZE_T cbData);
typedef VOID( SYMCRYPT_CALL * PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE ) (PVOID pState, PCBYTE pbSrc, PBYTE pbDst, SIZE_T cbData);

struct _SYMCRYPT_BLOCKCIPHER {
                                                PSYMCRYPT_BLOCKCIPHER_EXPAND_KEY    expandKeyFunc;      // mandatory
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT         encryptFunc;        // mandatory
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT         decryptFunc;        // mandatory
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbEncryptFunc;     // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT_ECB     ecbDecryptFunc;     // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcEncryptFunc;     // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    cbcDecryptFunc;     // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_MAC_MODE      cbcMacFunc;         // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_CRYPT_MODE    ctrMsb64Func;       // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmEncryptPartFunc; // NULL if no optimized version available
                                                PSYMCRYPT_BLOCKCIPHER_AEADPART_MODE gcmDecryptPartFunc; // NULL if no optimized version available
    _Field_range_( 1, SYMCRYPT_MAX_BLOCK_SIZE ) SIZE_T                              blockSize;          // = SYMCRYPT_XXX_BLOCK_SIZE, power of 2, 1 <= value <= 32.
                                                SIZE_T                              expandedKeySize;    // = sizeof( SYMCRYPT_XXX_EXPANDED_KEY )
};



//
// Session structs
//

#define SYMCRYPT_FLAG_SESSION_ENCRYPT       (0x1)

//
// SYMCRYPT_SESSION tracks the Nonces being used in a session. It is used differently depending on
// whether the session is an Encryption session or a Decryption session.
//
// In Encryption sessions, SYMCRYPT_SESSION tracks the Nonce which was used in the most recent
// attempted encryption in the session.
// messageNumber is atomically incremented by each encryption call, and the encryption method uses
// the messageNumber value that is the _result_ of the increment.
//
// In Decryption sessions, SYMCRYPT_SESSION tracks the most recently received Nonces in a series of
// successful decryptions. Nonces used in unsuccessful decryption calls do not update SYMCRYPT_SESSION.
// Information is tracked such that the decryption function can detect repeated Nonce values and
// fail decryption in this case. In order for this to work the message numbers that are provided
// to decrypt calls must be somewhat ordered. Provided message numbers may be arbitrarily far ahead
// of previously successfully decrypted message numbers, but may only be up to 63 behind the highest
// message number successfully decrypted so far.
// messageNumber normally represents the highest message number used in a successful decryption in
// this session. (The exception is at initialization, where messageNumber is initialized to 64
// without the corresponding 0th bit in the replayMask being set - this initial state represents
// there have been no successful decryptions yet, and that the earliest messageNumber that can be
// successfully received is 1)
// replayMask represents whether a window of 64 message numbers up to messageNumber have already been
// successfully used;
// bit n of replayMask (from n=0 to n=63) represents message number = (messageNumber-n), 0 means not
// yet used, and 1 means already used in a successful decryption call
//

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64
#define SYMCRYPT_USE_CAS128 (1)

// For CompareAndSwap128 method, SYMCRYPT_SESSION must be aligned to 16B
#define SYMCRYPT_ALIGN_SESSION SYMCRYPT_ALIGN_TYPE_AT(struct, 16)
#else
#define SYMCRYPT_USE_CAS128 (0)

// For method with only 64-bit atomics, SYMCRYPT_SESSION must be aligned to 8B
#define SYMCRYPT_ALIGN_SESSION SYMCRYPT_ALIGN_TYPE_AT(struct, 8)
#endif

// Nested struct used within SYMCRYPT_SESSION
typedef SYMCRYPT_ALIGN_SESSION _SYMCRYPT_SESSION_REPLAY_STATE {
    UINT64  replayMask;
    // 64 bit mask representing message numbers previously successfully decrypted up to 63
    // before the most recent message number.

    UINT64  messageNumber;
    // the last 8 bytes of the Nonce (MSB-first)
} SYMCRYPT_SESSION_REPLAY_STATE, * PSYMCRYPT_SESSION_REPLAY_STATE;
typedef const SYMCRYPT_SESSION_REPLAY_STATE * PCSYMCRYPT_SESSION_REPLAY_STATE;

typedef SYMCRYPT_ALIGN_SESSION _SYMCRYPT_SESSION {
    SYMCRYPT_SESSION_REPLAY_STATE replayState;
    // nested replayState struct is to improve code clarity in SymCryptSessionDecryptUpdate*

    UINT32  senderId;
    // the first 4 bytes of the Nonce (MSB-first)
    // (set by the caller and constant for the lifetime of a session)

    UINT32  flags;
    // SYMCRYPT_FLAG_SESSION_ENCRYPT indicates the struct is to be used for an encryption session,
    // otherwise the struct is to be used for a decryption session

    PVOID   pMutex;
    // Pointer to a fast single-process mutex object used to enable atomic update of replayMask and
    // messageNumber in the absence of support for a 128b CAS operation
} SYMCRYPT_SESSION, * PSYMCRYPT_SESSION;

#define SYMCRYPT_SESSION_MAX_MESSAGE_NUMBER (0xffffffff00000000ull)
// We do not allow messageNumber to go above some maximum value (currently 2^64 - 2^32)
// This gives us a large window to prevent many concurrent encryption threads from updating the
// session such that the messageNumber overflows and the same IV is used in many encryptions
// (i.e. we would only potentially get a spurious success using a repeated IV when there are
// >2^32 concurrent threads!)

#if SYMCRYPT_USE_CAS128
C_ASSERT(SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SESSION, replayState.replayMask) == 0);
C_ASSERT(SYMCRYPT_FIELD_OFFSET(SYMCRYPT_SESSION, replayState.messageNumber) == 8);
// For CompareAndSwap128 method, replayMask and messageNumber must be tightly packed
#endif

//
// RC4
//

//
// Some CPUs like the S array type to be larger than BYTE. We abstract the data type
// of the S array to accommodate such CPUs in future.
//

typedef BYTE    SYMCRYPT_RC4_S_TYPE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_RC4_STATE {
    SYMCRYPT_RC4_S_TYPE  S[256];
    BYTE i;
    BYTE j;
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_RC4_STATE, *PSYMCRYPT_RC4_STATE;

//
// ChaCha20
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_CHACHA20_STATE {
    UINT32      key[8];
    UINT32      nonce[3];
    UINT64      offset;                 // offset to use for next operation
    BOOLEAN     keystreamBufferValid;   // keystream buffer matches offset value
    BYTE        keystream[64];
} SYMCRYPT_CHACHA20_STATE, *PSYMCRYPT_CHACHA20_STATE;


//
// AES_CTR_DRBG
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_RNG_AES_STATE {
    //
    // Key and V value are in one array, to allow fast generation of both of them
    // in a single call.
    //
    BYTE        keyAndV[32 + 16];
    BYTE        previousBlock[16];
    UINT64      requestCounter;         // called reseed_counter in SP 800-90
    BOOLEAN     fips140_2Check;         // set if the FIPS 140-2 continuous self-test is required
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_RNG_AES_STATE, * PSYMCRYPT_RNG_AES_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_RNG_AES_FIPS140_2_STATE {
    SYMCRYPT_RNG_AES_STATE  rng;
} SYMCRYPT_RNG_AES_FIPS140_2_STATE, *PSYMCRYPT_RNG_AES_FIPS140_2_STATE;


//
// MARVIN32
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MARVIN32_EXPANDED_SEED
{
    UINT32   s[2];
    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_MARVIN32_EXPANDED_SEED, *PSYMCRYPT_MARVIN32_EXPANDED_SEED;
typedef const SYMCRYPT_MARVIN32_EXPANDED_SEED * PCSYMCRYPT_MARVIN32_EXPANDED_SEED;


typedef SYMCRYPT_MARVIN32_EXPANDED_SEED SYMCRYPT_MARVIN32_CHAINING_STATE, * PSYMCRYPT_MARVIN32_CHAINING_STATE;

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MARVIN32_STATE
{
    SYMCRYPT_ALIGN  BYTE                                buffer[8];  // 4 bytes of data, 4 more bytes for final padding
                    SYMCRYPT_MARVIN32_CHAINING_STATE    chain;      // chaining state
                    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pSeed;      //
                    UINT32                              dataLength; // length of the data processed so far, mod 2^32
                    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_MARVIN32_STATE, *PSYMCRYPT_MARVIN32_STATE;
typedef const SYMCRYPT_MARVIN32_STATE *PCSYMCRYPT_MARVIN32_STATE;


//
// Export blob sizes
//

#define SYMCRYPT_MD2_STATE_EXPORT_SIZE          (80)
#define SYMCRYPT_MD4_STATE_EXPORT_SIZE          (116)
#define SYMCRYPT_MD5_STATE_EXPORT_SIZE          (116)
#define SYMCRYPT_SHA1_STATE_EXPORT_SIZE         (120)
#define SYMCRYPT_SHA224_STATE_EXPORT_SIZE       (132)
#define SYMCRYPT_SHA256_STATE_EXPORT_SIZE       (132)
#define SYMCRYPT_SHA384_STATE_EXPORT_SIZE       (236)
#define SYMCRYPT_SHA512_STATE_EXPORT_SIZE       (236)
#define SYMCRYPT_SHA512_224_STATE_EXPORT_SIZE   (236)
#define SYMCRYPT_SHA512_256_STATE_EXPORT_SIZE   (236)

#define SYMCRYPT_KECCAK_STATE_EXPORT_SIZE   (234)
#define SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE SYMCRYPT_KECCAK_STATE_EXPORT_SIZE
#define SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE SYMCRYPT_KECCAK_STATE_EXPORT_SIZE
#define SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE SYMCRYPT_KECCAK_STATE_EXPORT_SIZE
#define SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE SYMCRYPT_KECCAK_STATE_EXPORT_SIZE


//
// KDF algorithms
//

//
// PBKDF2
//

typedef struct _SYMCRYPT_PBKDF2_EXPANDED_KEY {
    SYMCRYPT_MAC_EXPANDED_KEY   macKey;
    PCSYMCRYPT_MAC              macAlg;
} SYMCRYPT_PBKDF2_EXPANDED_KEY, *PSYMCRYPT_PBKDF2_EXPANDED_KEY;
typedef const SYMCRYPT_PBKDF2_EXPANDED_KEY *PCSYMCRYPT_PBKDF2_EXPANDED_KEY;

//
// SP 800-108
//

typedef struct _SYMCRYPT_SP800_108_EXPANDED_KEY {
    SYMCRYPT_MAC_EXPANDED_KEY   macKey;
    PCSYMCRYPT_MAC              macAlg;
} SYMCRYPT_SP800_108_EXPANDED_KEY, *PSYMCRYPT_SP800_108_EXPANDED_KEY;
typedef const SYMCRYPT_SP800_108_EXPANDED_KEY *PCSYMCRYPT_SP800_108_EXPANDED_KEY;

//
// TLS PRF 1.1
//

typedef struct _SYMCRYPT_TLSPRF1_1_EXPANDED_KEY {
    SYMCRYPT_HMAC_MD5_EXPANDED_KEY   macMd5Key;
    SYMCRYPT_HMAC_SHA1_EXPANDED_KEY   macSha1Key;
} SYMCRYPT_TLSPRF1_1_EXPANDED_KEY, *PSYMCRYPT_TLSPRF1_1_EXPANDED_KEY;
typedef const SYMCRYPT_TLSPRF1_1_EXPANDED_KEY *PCSYMCRYPT_TLSPRF1_1_EXPANDED_KEY;

//
// TLS PRF 1.2
//

typedef struct _SYMCRYPT_TLSPRF1_2_EXPANDED_KEY {
    SYMCRYPT_MAC_EXPANDED_KEY   macKey;
    PCSYMCRYPT_MAC              macAlg;
} SYMCRYPT_TLSPRF1_2_EXPANDED_KEY, *PSYMCRYPT_TLSPRF1_2_EXPANDED_KEY;
typedef const SYMCRYPT_TLSPRF1_2_EXPANDED_KEY *PCSYMCRYPT_TLSPRF1_2_EXPANDED_KEY;

//
// SSH-KDF
//
typedef struct _SYMCRYPT_SSHKDF_EXPANDED_KEY {
    PCSYMCRYPT_HASH     pHashFunc;
    SYMCRYPT_HASH_STATE hashState;
} SYMCRYPT_SSHKDF_EXPANDED_KEY, *PSYMCRYPT_SSHKDF_EXPANDED_KEY;
typedef const SYMCRYPT_SSHKDF_EXPANDED_KEY *PCSYMCRYPT_SSHKDF_EXPANDED_KEY;

//
// SRTP-KDF
//
typedef struct _SYMCRYPT_SRTPKDF_EXPANDED_KEY {
    SYMCRYPT_AES_EXPANDED_KEY     aesExpandedKey;
} SYMCRYPT_SRTPKDF_EXPANDED_KEY, *PSYMCRYPT_SRTPKDF_EXPANDED_KEY;
typedef const SYMCRYPT_SRTPKDF_EXPANDED_KEY *PCSYMCRYPT_SRTPKDF_EXPANDED_KEY;

//
// HKDF
//

typedef struct _SYMCRYPT_HKDF_EXPANDED_KEY {
    SYMCRYPT_MAC_EXPANDED_KEY   macKey;
    PCSYMCRYPT_MAC              macAlg;
} SYMCRYPT_HKDF_EXPANDED_KEY, *PSYMCRYPT_HKDF_EXPANDED_KEY;
typedef const SYMCRYPT_HKDF_EXPANDED_KEY *PCSYMCRYPT_HKDF_EXPANDED_KEY;

//
// SSKDF
//
typedef struct _SYMCRYPT_SSKDF_MAC_EXPANDED_SALT {
    SYMCRYPT_MAC_EXPANDED_KEY   macKey;
    PCSYMCRYPT_MAC              macAlg;
} SYMCRYPT_SSKDF_MAC_EXPANDED_SALT, *PSYMCRYPT_SSKDF_MAC_EXPANDED_SALT;
typedef const SYMCRYPT_SSKDF_MAC_EXPANDED_SALT *PCSYMCRYPT_SSKDF_MAC_EXPANDED_SALT;

//
// Digit & alignment sizes.
//
// WARNING: do not change these without updating all the optimized code,
// including assembler code.
// The FDEF_DIGIT_SIZE is the digit size used by the FDEF format.
//
#if SYMCRYPT_CPU_AMD64

#define SYMCRYPT_FDEF_DIGIT_SIZE    64
#define SYMCRYPT_ASYM_ALIGN_VALUE   32

#elif SYMCRYPT_CPU_ARM64

#define SYMCRYPT_FDEF_DIGIT_SIZE    32
#define SYMCRYPT_ASYM_ALIGN_VALUE   32

#else

#define SYMCRYPT_FDEF_DIGIT_SIZE    16
#define SYMCRYPT_ASYM_ALIGN_VALUE   16              // We have some bugs when ASYM_ALIGN_VALUE > DIGIT_SIZE; need to fix them if we implement AVX2-based x86 code.

#endif

#define SYMCRYPT_ASYM_ALIGN_UP( _p ) ((PBYTE) ( ((SIZE_T) (_p) + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1 ) ) )


//==============================================================================================
// Object types for low-level API
//
// INT          integer in range 0..N for some N
// DIVISOR      an integer > 0 that can be used to divide with.
// MODULUS      a value M > 1 to use in modulo-M computations
// MODELEMENT   An element in a modulo-M ring.
// ECPOINT      A point on an elliptic curve.
//
// These objects are all aligned to SYMCRYPT_ASYM_ALIGN
//
#define SYMCRYPT_ASYM_ALIGN SYMCRYPT_ALIGN_AT(SYMCRYPT_ASYM_ALIGN_VALUE)
#if SYMCRYPT_MS_VC
#define SYMCRYPT_ASYM_ALIGN_STRUCT SYMCRYPT_ASYM_ALIGN struct
#elif SYMCRYPT_GNUC
#define SYMCRYPT_ASYM_ALIGN_STRUCT struct SYMCRYPT_ASYM_ALIGN
#else
#error Unknown compiler
#endif

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_INT;
typedef struct _SYMCRYPT_INT   SYMCRYPT_INT;
typedef       SYMCRYPT_INT * PSYMCRYPT_INT;
typedef const SYMCRYPT_INT * PCSYMCRYPT_INT;

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_DIVISOR;
typedef struct _SYMCRYPT_DIVISOR   SYMCRYPT_DIVISOR;
typedef       SYMCRYPT_DIVISOR * PSYMCRYPT_DIVISOR;
typedef const SYMCRYPT_DIVISOR * PCSYMCRYPT_DIVISOR;

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MODULUS;
typedef struct _SYMCRYPT_MODULUS   SYMCRYPT_MODULUS;
typedef       SYMCRYPT_MODULUS * PSYMCRYPT_MODULUS;
typedef const SYMCRYPT_MODULUS * PCSYMCRYPT_MODULUS;

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MODELEMENT;
typedef  struct _SYMCRYPT_MODELEMENT   SYMCRYPT_MODELEMENT;
typedef       SYMCRYPT_MODELEMENT * PSYMCRYPT_MODELEMENT;
typedef const SYMCRYPT_MODELEMENT * PCSYMCRYPT_MODELEMENT;

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_ECPOINT;
typedef struct _SYMCRYPT_ECPOINT   SYMCRYPT_ECPOINT;
typedef       SYMCRYPT_ECPOINT * PSYMCRYPT_ECPOINT;
typedef const SYMCRYPT_ECPOINT * PCSYMCRYPT_ECPOINT;


//
// Arithmetic formats
//

#define SYMCRYPT_ANYSIZE    1       // used to mark arrays of arbitrary size

#define SYMCRYPT_FDEF_DIGIT_BITS    (8*SYMCRYPT_FDEF_DIGIT_SIZE)
#define SYMCRYPT_FDEF_DIGITS_FROM_BITS( _bits )   ( \
                                                   ((_bits)/ SYMCRYPT_FDEF_DIGIT_BITS) + \
                                                   (( ((_bits) & (SYMCRYPT_FDEF_DIGIT_BITS-1)) + (SYMCRYPT_FDEF_DIGIT_BITS - 1) )/SYMCRYPT_FDEF_DIGIT_BITS) \
                                                  )

#define SYMCRYPT_BYTES_FROM_BITS(bits)          ( ( (bits) + 7 ) / 8 )

// The maximum number of bits in any integer value that the library supports. If the
// caller's input exceed this bound then the integer object will not be created.
// The caller either must ensure the bound is not exceeded, or check for NULL before
// using created SymCrypt objects.
// The primary purpose of this limit is to avoid integer overflows in size computations.
// Having a reasonable upper bound avoids all size overflows, even on 32-bit CPUs
#define SYMCRYPT_INT_MAX_BITS       ((UINT32)(1 << 20))

//
// Upper bound for the number of digits: this MUST be enforced on runtime
// on all Allocate, SizeOf, and Create calls which take as input a digit number.
//
// Using this upper bound and the SYMCRYPT_INT_MAX_BITS upper bound we can argue
// that no integer overflow on 32-bit sizes can happen. Note that the computed upper
// bounds are very loose and the actual values are much smaller.
//
#define SYMCRYPT_FDEF_UPB_DIGITS    (SYMCRYPT_FDEF_DIGITS_FROM_BITS(SYMCRYPT_INT_MAX_BITS))




//
// All of the following SYMCRYPT_FDEF_SIZEOF_XXX_FROM_YYY computations for the four
// main SymCrypt objects (INT, DIVISOR, MODULUS, MODELEMENT) return a value not
// larger than 2^19 if the inputs _nDigits and _bits are not larger than
// SYMCRYPT_FDEF_UPB_DIGITS and SYMCRYPT_INT_MAX_BITS respectively (For MODELEMENT this bound
// is 2^17). The latter bounds must be enforced on runtime for all calculations taking as inputs
// number of digits or bits.
//
// The 2^19 upper bound is derived from:
//      - the maximum (byte) size of an "integer": 2^20 bits / 8 = 2^17 bytes
//      - "sizeof" computations add up to less than 2^18 bytes ~ 262 Kb
//      - the modulus object contains two "integers"
//

//
// Type fields contain the following:
// lower 16 bits: offset into virtual table (if any)
// upper 16 bits: bits 16-23: 1-character object type. Bits 24-31: 1 char implementation type
// The upper bits allow objects to be recognized in memory, making debugging easier.
//

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_INT {
                                                    UINT32  type;
    _Field_range_( 1, SYMCRYPT_FDEF_UPB_DIGITS )    UINT32  nDigits;    // digit size depends on run-time decisions...
                                                    UINT32  cbSize;

    SYMCRYPT_MAGIC_FIELD
    SYMCRYPT_ASYM_ALIGN union {
        struct {
            UINT32          uint32[SYMCRYPT_ANYSIZE];   // FDEF: array UINT32[nDigits * # uint32 per digit]
        } fdef;
    } ti;                   // we must have a name here. 'ti' stands for 'Type-Int', it helps catch type errors when type-casting macros are used.
};

#define SYMCRYPT_FDEF_INT_PUINT32( p )  (&(p)->ti.fdef.uint32[0])


#define SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nDigits )    ((_nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE + sizeof( SYMCRYPT_INT ) )
#define SYMCRYPT_FDEF_SIZEOF_INT_FROM_BITS( _bits )         SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( SYMCRYPT_FDEF_DIGITS_FROM_BITS( _bits ))

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_DIVISOR {
                                                    UINT32  type;
    _Field_range_( 1, SYMCRYPT_FDEF_UPB_DIGITS )    UINT32  nDigits;    // digit size depends on run-time decisions...
                                                    UINT32  cbSize;

                                                    UINT32  nBits;      // # bits in divisor

    SYMCRYPT_MAGIC_FIELD
    union{
        struct {
            UINT64                  W;              // approximate inverse of the divisor. Some implementations will use 64 bits, others 32 bits.
        } fdef;
    } td;
    SYMCRYPT_INT            Int;                    // Having a full Int here uses more space, but allows any Divisor to still be used as an Int.
    // This structure is directly followed by the Int extension
};

#define SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_DIGITS( _nDigits ) ((_nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE + sizeof( SYMCRYPT_DIVISOR ) )
#define SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_BITS( _bits ) SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_DIGITS( SYMCRYPT_FDEF_DIGITS_FROM_BITS( _bits ))

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MODULUS {
                                                    UINT32  type;
    _Field_range_( 1, SYMCRYPT_FDEF_UPB_DIGITS )    UINT32  nDigits;        // digit size depends on run-time decisions...
                                                    UINT32  cbSize;         // Size of modulus object

                                                    UINT32  flags;          // The flags the modulus was created with
                                                    UINT32  cbModElement;   // Size of one modElement
                                                    UINT64  inv64;          // -1/modulus mod 2^64 (always set but only to a useful value when the modulus is odd)

    SYMCRYPT_MAGIC_FIELD
    union{
        struct {
            //UINT32          nUint32Used;    // # 32-bit words used in representing numbers. modulus < 2^{32*nUint32Used}.
                                            // only values used are nDigits * uint32-per-digit or specific smaller values for optimized implementations
            PCUINT32        Rsqr;           // R^2 mod modulus, in uint32 form, nUint32Used words. Stored after Divisor. R = 2^{32*nUint32Used}
        } montgomery;
        struct {
            UINT32          k;              // modulus = 2^<bitsize of modelement> - k
        } pseudoMersenne;
    } tm;                                   // type specific data. Every Modulus can be used as a generic modulus, so no type-specific data for generic.

    SYMCRYPT_DIVISOR        Divisor;
    // This structure is directly followed by:
    //  The extensions of the Divisor object
    // and after that:
    // FDEF: Rsqr as an array of UINT32, size = nDigits * digitsize
    // FDEF: negDivisor as an array of UINT32, size = nDigits * digitsize
};

#define SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_DIGITS( _nDigits )    (sizeof( SYMCRYPT_MODULUS ) + SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_DIGITS( _nDigits ) + (2 * _nDigits * SYMCRYPT_FDEF_DIGIT_SIZE) )
#define SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_BITS( _bits )         SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_DIGITS(SYMCRYPT_FDEF_DIGITS_FROM_BITS( _bits ))

SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MODELEMENT {
    // ModElements just store the information without any header. This union makes this well-defined, and allows easy access.
    union{
        UINT32    uint32[SYMCRYPT_ANYSIZE];
    } d;
};

#define SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nDigits )    ((_nDigits) * SYMCRYPT_FDEF_DIGIT_SIZE)
#define SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_BITS( _bits )          SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( SYMCRYPT_FDEF_DIGITS_FROM_BITS( _bits ) )

//
// Upper bound for scratch size computations for FDEF objects depending only on digits
//
// The following 14 scratch size computation macros are all of the form:
//      Some SIZEOF macros + max( some other scratch macros )
// and all depend on some number of digits. (Slight exceptions are
// INT_TO_MODULUS and INT_PRIME_GEN but they can fit into the below
// rationale.)
//
// One can see that the deepest recursion in these macros and the biggest
// return value is for
//      INT_PRIME_GEN -> INT_MILLER_RABIN -> MODEXP ->
//      COMMON_MOD_OPERATIONS -> SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD
//
// Using the 2^19 (2^17) bound on the sizeof computations the biggest contribution on the above chain is for MODEXP:
//      ((1 << SYMCRYPT_FDEF_MAX_WINDOW_MODEXP) + 2) * SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nModDigits )
// which is bounded above by
//      (2^6 + 2) * 2^17 < 2^24
//
// By doubling on each subsequent recursive call we get the conservative
// upper bound for all scratch size computation macros of 2^26.
//

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits )  (16 * (_nDigits))   // unused currently, but this catches errors

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( _nDigits )         (16 * (_nDigits))   // unused currently, but nonzero size catches errors

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( _nSrcDigits, _nDivisorDigits )  ( (_nSrcDigits + 1) * SYMCRYPT_FDEF_DIGIT_SIZE )

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits ) ( \
            4 * SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nDigits ) + \
            SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( 2 * _nDigits ) + \
            2 * SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_DIGITS( _nDigits ) + \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( 2 * _nDigits, _nDigits ), \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( 2 * _nDigits ), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits ) )) )

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits ) \
            ( (2*(_nModDigits) * SYMCRYPT_FDEF_DIGIT_SIZE) + \
            SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( 2*(_nModDigits), _nModDigits )) // for mult: tmp product + divmod scratch

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_CRT_GENERATION( _nDigits ) ( \
            2*SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nDigits ) + \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits ), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nDigits ) ))

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_CRT_SOLUTION( _nDigits ) ( \
            SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nDigits ) + \
            SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nDigits ) + \
            SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( 2*_nDigits ) + \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nDigits ), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( 2*_nDigits ) ))

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _nDigits )  ( \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits ),\
                (2*_nDigits+1) * SYMCRYPT_FDEF_DIGIT_SIZE + SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( 2*_nDigits + 1, nDigits )) )

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODINV( _nModDigits ) ( \
            4 * SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nModDigits ) + \
            3 * SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nModDigits ) + \
            SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits ) )

#define SYMCRYPT_FDEF_MAX_WINDOW_MODEXP         (6)

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODEXP( _nModDigits ) ( \
            ((1 << SYMCRYPT_FDEF_MAX_WINDOW_MODEXP) + 2) * SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nModDigits ) + \
            SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits ) )

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_IS_POTENTIAL_PRIME( _nDigits )  (0)

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MILLER_RABIN( _nDigits ) ( \
            SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_DIGITS(_nDigits) + \
            3*SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS(_nDigits) + \
            SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS(_nDigits) + \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_MODULUS(_nDigits), \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS(_nDigits), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODEXP( _nDigits ) )) )

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_IS_PRIME( _nDigits ) ( \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_IS_POTENTIAL_PRIME( _nDigits ), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MILLER_RABIN( _nDigits ) ))

#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_PRIME_GEN( _nDigits ) ( \
            SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS * SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_DIGITS( 1 ) + \
            SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( 1 ) + \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( 1 ), \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( _nDigits, 1 ), \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SIZEOF_INT_FROM_DIGITS( _nDigits ), \
            SYMCRYPT_MAX( SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_IS_POTENTIAL_PRIME( _nDigits ), \
                 SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MILLER_RABIN( _nDigits ) )))))

//
// Upper bound for SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODMULTIEXP
//
// _nBase and _nBitsExp are bounded by SYMCRYPT_MODMULTIEXP_MAX_NBASES = 8 and
// SYMCRYPT_MODMULTIEXP_MAX_NBITSEXP = 2^20. Therefore the upper bound on this computation
// is
//      2^21 + 2^3*(2^6+4)*2^17 + 2^3*2^20*4 < 2^27
//
#define SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODMULTIEXP( _nModDigits, _nBases, _nBitsExp ) ( \
    SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits ) + \
    ((_nBases)*(1<<SYMCRYPT_FDEF_MAX_WINDOW_MODEXP) + 4)*SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS( _nModDigits ) + \
    (((_nBases)*(_nBitsExp)*sizeof(UINT32) + SYMCRYPT_ASYM_ALIGN_VALUE - 1) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1)) )
// Note: We need +4 multiplied with SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_DIGITS so that SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODMULTIEXP
// is always at least 2 modelements bigger than SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODEXP (see modexp.c)

//
// Support for masked operations

#define SYMCRYPT_MASK32_SET             ((UINT32)-1)
#define SYMCRYPT_MASK32_NONZERO( _v )   ((UINT32)(((UINT64)0 - (_v)) >> 32))
#define SYMCRYPT_MASK32_ZERO( _v )      (~SYMCRYPT_MASK32_NONZERO( _v ))
#define SYMCRYPT_MASK32_EQ( _a, _b )    (~SYMCRYPT_MASK32_NONZERO( (_a) ^ (_b) ))
#define SYMCRYPT_MASK32_LT( _a, _b )    ((UINT32)( ((UINT64)(_a) - (_b)) >> 32 ))


//
// Dispatch definitions
// When multiple formats are supported, this is where the information of the multiple formats is combined.
//
//  See the comments in SYMCRYPT_FDEF_SCRATCH_XXX regarding 32 bit overflow protection. All results
//  are bounded above by 2^27.
//

#define SYMCRYPT_INTERNAL_SIZEOF_INT_FROM_BITS( _bitsize )          SYMCRYPT_FDEF_SIZEOF_INT_FROM_BITS( _bitsize )
#define SYMCRYPT_INTERNAL_SIZEOF_DIVISOR_FROM_BITS( _bitsize )      SYMCRYPT_FDEF_SIZEOF_DIVISOR_FROM_BITS( _bitsize )
#define SYMCRYPT_INTERNAL_SIZEOF_MODULUS_FROM_BITS( _bitsize )      SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_BITS( _bitsize )
#define SYMCRYPT_INTERNAL_SIZEOF_MODELEMENT_FROM_BITS( _bitsize )   SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_BITS( _bitsize )

#define SYMCRYPT_INTERNAL_SIZEOF_RSAKEY_FROM_PARAMS( modBits, nPrimes, nPubExps ) SYMCRYPT_FDEF_SIZEOF_RSAKEY_FROM_PARAMS( modBits, nPrimes, nPubExps )
// For now we don't need the pubExpBits so we drop them, but we might use them later.

#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits )                      SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_DIVISOR( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_MUL( _nDigits )                             SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_MUL( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_DIVMOD( _nSrcDigits, _nDivisorDigits )      SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_DIVMOD( _nSrcDigits, _nDivisorDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits )                        SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_EXTENDED_GCD( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits )            SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_COMMON_MOD_OPERATIONS( _nModDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_CRT_GENERATION( _nDigits )                      SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_CRT_GENERATION( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_CRT_SOLUTION( _nDigits )                        SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_CRT_SOLUTION( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _nDigits )                      SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_TO_MODULUS( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODINV( _nModDigits )                           SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODINV( _nModDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODEXP( _nModDigits )                           SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODEXP( _nModDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_IS_PRIME( _nDigits )                        SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_IS_PRIME( _nDigits )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_INT_PRIME_GEN( _nDigits )                       SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_INT_PRIME_GEN( _nDigits )

#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_MODMULTIEXP( _nModDigits, _nBases, _nBitsExp )  SYMCRYPT_FDEF_SCRATCH_BYTES_FOR_MODMULTIEXP( _nModDigits, _nBases, _nBitsExp )

//
// Forward declarations for MlKemkey types
//
SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEMKEY;
typedef struct _SYMCRYPT_MLKEMKEY SYMCRYPT_MLKEMKEY;
typedef       SYMCRYPT_MLKEMKEY * PSYMCRYPT_MLKEMKEY;
typedef const SYMCRYPT_MLKEMKEY * PCSYMCRYPT_MLKEMKEY;

//
// Forward declarations for MlDsakey types
//
struct _SYMCRYPT_MLDSAKEY;
typedef struct _SYMCRYPT_MLDSAKEY SYMCRYPT_MLDSAKEY;
typedef       SYMCRYPT_MLDSAKEY * PSYMCRYPT_MLDSAKEY;
typedef const SYMCRYPT_MLDSAKEY * PCSYMCRYPT_MLDSAKEY;

//
// Forward declarations for CompositeMlKemkey types
//
SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_COMPOSITE_MLKEMKEY;
typedef struct _SYMCRYPT_COMPOSITE_MLKEMKEY SYMCRYPT_COMPOSITE_MLKEMKEY;
typedef       SYMCRYPT_COMPOSITE_MLKEMKEY * PSYMCRYPT_COMPOSITE_MLKEMKEY;
typedef const SYMCRYPT_COMPOSITE_MLKEMKEY * PCSYMCRYPT_COMPOSITE_MLKEMKEY;

//
// RSA padding scratch definitions
//
// The maximum sizes of the state and the result for all hash algorithms are
// sizeof(SYMCRYPT_HASH_STATE) and SYMCRYPT_HASH_MAX_RESULT_SIZE, both not bigger
// 2^20. All the nBytes inputs are bounded by 2^17 (the maximum byte-size
// of the RSA modulus).
//
// Thus a total upper bound on these results is 2^20.
//
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_OAEP( _hashAlgorithm, _nBytesOAEP ) ( SymCryptHashStateSize( _hashAlgorithm ) + \
                                                                                      SymCryptHashResultSize( _hashAlgorithm ) + \
                                                                                      2*(_nBytesOAEP - 1) )

#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_PKCS1( _nBytesPKCS1 ) ( _nBytesPKCS1 )

#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_RSA_PSS( _hashAlgorithm, _nBytesMessage, _nBytesPSS ) ( SymCryptHashStateSize( _hashAlgorithm ) + \
                                                                                                    _nBytesMessage + \
                                                                                                    3*(_nBytesPSS) + 5 )

//
// RSAKEY Type
//

#define SYMCRYPT_FDEF_SIZEOF_RSAKEY_FROM_PARAMS( modBits, nPrimes, nPubExps ) \
    sizeof( SYMCRYPT_RSAKEY ) + \
    (nPrimes + 1) * SYMCRYPT_FDEF_SIZEOF_MODULUS_FROM_BITS( modBits ) + \
    nPrimes * SYMCRYPT_FDEF_SIZEOF_MODELEMENT_FROM_BITS( modBits ) + \
    (nPrimes + 1) * nPubExps * SYMCRYPT_FDEF_SIZEOF_INT_FROM_BITS( modBits )
// 1 modulus object per prime + 1 for the RSA modulus
// 1 modelement for every crtInverse
// 1 int per pubexp for each privexp +  1 int per prime*pubexp for each crtprivexp

#define SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES            (2)
#define SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS           (1)

#define SYMCRYPT_RSAKEY_MIN_BITSIZE_MODULUS         (256)               // Some of our SCS code requires at least 32 bytes...
#define SYMCRYPT_RSAKEY_MAX_BITSIZE_MODULUS         (1 << 16)           // Avoid any integer overflows in size calculations

// RSA FIPS self-tests require at least 496 bits to avoid fatal
// Require caller to specify NO_FIPS for up to 1024 bits as running FIPS tests on too-small keys
// does not make it FIPS certifiable and gives the wrong impression to callers
#define SYMCRYPT_RSAKEY_FIPS_MIN_BITSIZE_MODULUS    (1024)

#define SYMCRYPT_RSAKEY_MIN_BITSIZE_PRIME           (128)
#define SYMCRYPT_RSAKEY_MAX_BITSIZE_PRIME           (SYMCRYPT_RSAKEY_MAX_BITSIZE_MODULUS / 2)

// Minimum allowable bit sizes for generated and imported parameters for
// the RSA modulus and each prime.

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_RSAKEY {
                    UINT32              fAlgorithmInfo;     // Tracks which algorithms the key can be used in
                                                            // Also tracks which per-key selftests have been performed on this key
                                                            // A bitwise OR of SYMCRYPT_FLAG_KEY_*, SYMCRYPT_FLAG_RSAKEY_*, and
                                                            // SYMCRYPT_PCT_* values

                    UINT32              cbTotalSize;        // Total size of the rsa key
                    BOOLEAN             hasPrivateKey;      // Set to true if there is private key information set

                    UINT32              nSetBitsOfModulus;  // Bits of modulus specified during creation

                    UINT32              nBitsOfModulus;     // Number of bits of the value of the modulus (not the object's size)
                    UINT32              nDigitsOfModulus;   // Number of digits of the modulus object (always equal to SymCryptDigitsFromBits(nSetBitsOfModulus))

                    UINT32              nPubExp;            // Number of public exponents

                    UINT32              nPrimes;            // Number of primes, can be 0 if the object only supports public keys
                    UINT32              nBitsOfPrimes[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                                                            // Number of bits of the value of each prime (not the object's size)
                    UINT32              nDigitsOfPrimes[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                                                            // Number of digits of each prime object
                    UINT32              nMaxDigitsOfPrimes; // Maximum number of digits in nDigitsOfPrimes

                    UINT64              au64PubExp[SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS];
                    // SYMCRYPT_ASYM_ALIGN'ed buffers that point to memory allocated for each object
                    PBYTE               pbPrimes[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                    PBYTE               pbCrtInverses[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                    PBYTE               pbPrivExps[SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS];
                    PBYTE               pbCrtPrivExps[SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS * SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];

                    // SymCryptObjects
                    PSYMCRYPT_MODULUS   pmModulus;          // The modulus N=p*q
                    PSYMCRYPT_MODULUS   pmPrimes[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                                                            // Pointers to the secret primes
                    PSYMCRYPT_MODELEMENT peCrtInverses[SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                                                            // Pointers to the CRT inverses of the primes
                    PSYMCRYPT_INT       piPrivExps[SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS];
                                                            // Pointers to the corresponding private exponents
                    PSYMCRYPT_INT       piCrtPrivExps[SYMCRYPT_RSAKEY_MAX_NUMOF_PUBEXPS * SYMCRYPT_RSAKEY_MAX_NUMOF_PRIMES];
                                                            // Pointers to the private exponents modulo each prime minus 1 (for CRT)

                    SYMCRYPT_MAGIC_FIELD
                    // Followed by:
                    // Modulus
                    // Primes
                    // CrtInverses
                    // PrivExps
                    // CrtPrivExps
} SYMCRYPT_RSAKEY;
typedef       SYMCRYPT_RSAKEY * PSYMCRYPT_RSAKEY;
typedef const SYMCRYPT_RSAKEY * PCSYMCRYPT_RSAKEY;

//
// The following definitions relating to trial division are not needed by normal callers
// but are used by the test program to measure performance of components.
//

typedef struct _SYMCRYPT_TRIALDIVISION_PRIME {
    UINT64  invMod2e64;         // Inverse of prime modulo 2^64
    UINT64  compareLimit;       // floor( (2^{64}-1)/ prime )
} SYMCRYPT_TRIALDIVISION_PRIME, *PSYMCRYPT_TRIALDIVISION_PRIME;
typedef const SYMCRYPT_TRIALDIVISION_PRIME * PCSYMCRYPT_TRIALDIVISION_PRIME;
//
// This structure is used to test whether a UINT64 is a multiple of a (small) prime.
// Let V be the input value, P the small prime, and W the inverse of P modulo 2^64.
// If V = k*P then V * M mod 2^64 = V/P mod 2^64 = k.
// This holds for k = 0, 1, ..., floor( (2^{64}-1)/p ).
// If V is not a multiple of P then the result of the multiplication must be larger than that.
//

typedef struct _SYMCRYPT_TRIALDIVISION_GROUP {
    UINT32  nPrimes;       // # primes are in this group (use the next ones)
    UINT32  factor[9];     // factors[i] = 2^{32*(i+1)} mod Prod where Prod = product of the primes
                           // It is guaranteed that Prod <= (2^{32}-1)/9
} SYMCRYPT_TRIALDIVISION_GROUP, *PSYMCRYPT_TRIALDIVISION_GROUP;
typedef const SYMCRYPT_TRIALDIVISION_GROUP * PCSYMCRYPT_TRIALDIVISION_GROUP;


typedef struct _SYMCRYPT_TRIALDIVISION_CONTEXT {
    SIZE_T                          nBytesAlloc;
    UINT32                          maxTrialPrime;
    PSYMCRYPT_TRIALDIVISION_GROUP   pGroupList; // terminated with 0 record
    PSYMCRYPT_TRIALDIVISION_PRIME   pPrimeList; // terminated with 0 record
    PUINT32                         pPrimes;    // terminated with a 0.
    SYMCRYPT_TRIALDIVISION_PRIME    Primes3_5_17[3];    // Structures for 3, 5 and 17 in that order
} SYMCRYPT_TRIALDIVISION_CONTEXT, *PSYMCRYPT_TRIALDIVISION_CONTEXT;
typedef const SYMCRYPT_TRIALDIVISION_CONTEXT * PCSYMCRYPT_TRIALDIVISION_CONTEXT;

UINT32
SymCryptTestTrialdivisionMaxSmallPrime( PCSYMCRYPT_TRIALDIVISION_CONTEXT pContext );   // Expose small prime limit to help test code

//
// DLGROUP type
//

#define SYMCRYPT_DLGROUP_MIN_BITSIZE_P          (32)
#define SYMCRYPT_DLGROUP_MIN_BITSIZE_Q          (31)  // Q must always be at least 1 bit shorter than P
// Minimum allowable bit sizes for generated and imported parameters for both P and
// Q primes.

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_DLGROUP {
    UINT32                  cbTotalSize;    // Total size of the dl group object
    BOOLEAN                 fHasPrimeQ;     // Flag that specifies whether the object has a Q parameter

    UINT32                  nBitsOfP;       // Number of bits of the value of P (not the object's size)
    UINT32                  cbPrimeP;       // Number of bytes of the value of P (not the object's size), equal to ceil(nBitsOfP/8)
    UINT32                  nDigitsOfP;     // Number of digits of the object of prime P
    UINT32                  nMaxBitsOfP;    // Maximum number of bits of the value of P

    UINT32                  nBitsOfQ;       // Number of bits of the value of Q (not the object's bits)
    UINT32                  cbPrimeQ;       // Number of bytes of the value of Q (not the object's size), equal to ceil(nBitsOfQ/8)
    UINT32                  nDigitsOfQ;     // Number of digits of the object of prime Q
    UINT32                  nMaxBitsOfQ;    // Maximum number of bits of the value of Q

    BOOLEAN                 isSafePrimeGroup;   // Boolean indicating if this is a Safe Prime group
    UINT32                  nMinBitsPriv;   // Minimum number of bits to be used in private keys for this group
                                            // This only applies to named Safe Prime groups where this is related to the security strength
                                            // i.e. this corresponds to 2s in SP800-56arev3 5.6.1.1.1 / 5.6.2.1.2
    UINT32                  nDefaultBitsPriv;   // Default number of bits used in private keys for this group
                                                // Normally equals nBitsOfQ, but may be further restricted (i.e. for named Safe Prime groups)
                                                // i.e. this corresponds to a default value of N in SP800-56arev3 5.6.1.1.1 / 5.6.2.1.2

    UINT32                  nBitsOfSeed;    // Number of bits of the seed used for generation (seedlen in FIPS 186-3)
    UINT32                  cbSeed;         // Number of bytes of the seed, equal to ceil(nBitsOfSeed/8)

    SYMCRYPT_DLGROUP_FIPS   eFipsStandard;  // Code specifying the FIPS standard used to create the keys. If 0 the group is unverified.

    PCSYMCRYPT_HASH         pHashAlgorithm; // Hash algorithm used for the generation of parameters
    UINT32                  dwGenCounter;   // Number of iterations used for the generation of parameters
    BYTE                    bIndexGenG;     // Index for the generation of generator G (FIPS 186-3) (Always 1 for now)

    PBYTE                   pbQ;            // SYMCRYPT_ASYM_ALIGN'ed buffer that points to the memory allocated for modulus Q

    PSYMCRYPT_MODULUS       pmP;            // Pointer to the prime P
    PSYMCRYPT_MODULUS       pmQ;            // Pointer to the prime Q

    PSYMCRYPT_MODELEMENT    peG;            // Pointer to the generator G

    PBYTE                   pbSeed;         // Buffer that will hold the seed (this is padded at the end so that the entire structure
                                            // has size a multiple of SYMCRYPT_ASYM_ALIGN_VALUE)

    SYMCRYPT_MAGIC_FIELD

    // P
    // Q
    // G
    // Seed
} SYMCRYPT_DLGROUP;
typedef       SYMCRYPT_DLGROUP * PSYMCRYPT_DLGROUP;
typedef const SYMCRYPT_DLGROUP * PCSYMCRYPT_DLGROUP;

//
// DLKEY type
//
typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_DLKEY {
                    UINT32                  fAlgorithmInfo; // Tracks which algorithms the key can be used in
                                                            // Also tracks which per-key selftests have been performed on this key
                                                            // A bitwise OR of SYMCRYPT_FLAG_KEY_*, SYMCRYPT_FLAG_DLKEY_*, and
                                                            // SYMCRYPT_PCT_* values

                    BOOLEAN                 fHasPrivateKey; // Set to true if there is a private key set
                    BOOLEAN                 fPrivateModQ;   // Set to true if the private key is at most Q-1, otherwise it is at most P-2
                    UINT32                  nBitsPriv;      // Number of bits used in private keys

                    PCSYMCRYPT_DLGROUP      pDlgroup;       // Handle to the group which created the key

                    PBYTE                   pbPrivate;      // SYMCRYPT_ASYM_ALIGN'ed buffer that points to the memory allocated for the private key

                    PSYMCRYPT_MODELEMENT    pePublicKey;    // Public key (modelement modulo P)
                    PSYMCRYPT_INT           piPrivateKey;   // Private key (integer up to 2^nBitsPriv-1, Q-1 or P-2)

                    SYMCRYPT_MAGIC_FIELD

                    // PublicKey
                    // PrivateKey                           // The size of this must always be the same as the size of P
} SYMCRYPT_DLKEY;
typedef       SYMCRYPT_DLKEY * PSYMCRYPT_DLKEY;
typedef const SYMCRYPT_DLKEY * PCSYMCRYPT_DLKEY;

//
// Elliptic Curve Function Types
//

#define SYMCRYPT_ECPOINT_FORMAT_MAX_LENGTH                      4   // Number of MODELEMENTs for the largest ECPOINT format

// Coordinate representations for ECPOINTs
// NOTE: The value masked with 0xf gives you the number of coordinates
typedef enum _SYMCRYPT_ECPOINT_COORDINATES {
    SYMCRYPT_ECPOINT_COORDINATES_INVALID             = 0x00,   // Invalid point representation
    SYMCRYPT_ECPOINT_COORDINATES_SINGLE              = 0x11,   // Representation with only X
    SYMCRYPT_ECPOINT_COORDINATES_AFFINE              = 0x22,   // Affine representation (X,Y)
    SYMCRYPT_ECPOINT_COORDINATES_PROJECTIVE          = 0x33,   // Three equally-sized values where the triple (X,Y,Z) represents the affine point (X/Z, Y/Z)
    SYMCRYPT_ECPOINT_COORDINATES_JACOBIAN            = 0x43,   // Three equally-sized values where the triple (X,Y,Z) represents the affine point (X/Z^2, Y/Z^3)
    SYMCRYPT_ECPOINT_COORDINATES_EXTENDED_PROJECTIVE = 0x54,   // Four equally-sized values where (X,Y,Z,T) represents the affine point (X/Z, Y/Z) with T=X*Y*Z
    SYMCRYPT_ECPOINT_COORDINATES_SINGLE_PROJECTIVE   = 0x62,   // Two equally-sized values where (X,Z) represents the point (X/Z)
} SYMCRYPT_ECPOINT_COORDINATES;

#define SYMCRYPT_INTERNAL_NUMOF_COORDINATES( _eCoordinates )              ((_eCoordinates) & 0xf)


//
// Curve-type-dependent information
//

// Short-Weierstrass

#define SYMCRYPT_ECURVE_SW_DEF_WINDOW               (6)         // Default window size for the windowed methods

#define SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS      (64)        // Maximum number of precomputed points

typedef struct _SYMCRYPT_ECURVE_INFO_PRECOMP {
                    UINT32              window;         // Window size
                    UINT32              nPrecompPoints; // Number of precomputed points
                    UINT32              nRecodedDigits; // Number of recoded digits
                    PSYMCRYPT_ECPOINT   poPrecompPoints[SYMCRYPT_ECURVE_SW_MAX_NPRECOMP_POINTS];
                                                        // Table of pointers to precomputed powers of the distinguished point
} SYMCRYPT_ECURVE_INFO_PRECOMP;

//
//  ECURVE object
//

#define SYMCRYPT_ECURVE_MIN_BITSIZE_FMOD        (32)
#define SYMCRYPT_ECURVE_MIN_BITSIZE_GORD        (32)
#define SYMCRYPT_ECURVE_MAX_COFACTOR_POWER      (8)
// Minimum (maximum for cofactor) allowable bit sizes for imported
// parameters for field modulus, group order of curve (and cofactor).

#define SYMCRYPT_INTERNAL_ECURVE_VERSION_LATEST                         1

typedef enum _SYMCRYPT_INTERNAL_ECURVE_TYPE {
    SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS     = 1,
    SYMCRYPT_INTERNAL_ECURVE_TYPE_TWISTED_EDWARDS       = 2,
    SYMCRYPT_INTERNAL_ECURVE_TYPE_MONTGOMERY            = 3,
    SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS_AM3 = 4,// This type is a specialization of Short-Weierstrass when A == -3
                                                            // This condition is detected and used for all NIST prime curves
} SYMCRYPT_INTERNAL_ECURVE_TYPE;

C_ASSERT((UINT32)SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS   == (UINT32)SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS );
C_ASSERT((UINT32)SYMCRYPT_INTERNAL_ECURVE_TYPE_TWISTED_EDWARDS     == (UINT32)SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS );
C_ASSERT((UINT32)SYMCRYPT_INTERNAL_ECURVE_TYPE_MONTGOMERY          == (UINT32)SYMCRYPT_ECURVE_TYPE_MONTGOMERY );

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_ECURVE {
                    UINT32                  version;        // Version #
                    SYMCRYPT_INTERNAL_ECURVE_TYPE
                                            type;           // Internal type of the curve
                    SYMCRYPT_ECPOINT_COORDINATES
                                            eCoordinates;   // Default representation of the EC points

                    UINT32                  FModBitsize;    // Bitsize of the field modulus
                    UINT32                  FModDigits;     // Number of digits of the field modulus
                    UINT32                  FModBytesize;   // Bytesize of the field modulus (specified in the curve parameters as cbFieldLength)

                    UINT32                  GOrdBitsize;    // Bitsize of the (sub)group order
                    UINT32                  GOrdDigits;     // Number of digits of the (sub)group order
                    UINT32                  GOrdBytesize;   // Bytesize of the (sub)group order (specified in the curve parameters as cbSubgroupOrder)

                    UINT32                  cbModElement;   // (Internal) bytesize of one mod element

                    UINT32                  cbAlloc;        // Bytesize of the total curve blob

                    UINT32                  cbScratchCommon;        // Size of scratch space for common ecurve operations
                    UINT32                  cbScratchScalar;        // Size of constant scratch space for scalar ecurve operations (without the nPoints dependence)
                    UINT32                  cbScratchScalarMulti;   // Dependence of scratch space for scalar ecurve operations from nPoints
                    UINT32                  cbScratchGetSetValue;   // Size of scratch space for get set value ecpoint operations
                    UINT32                  cbScratchEckey;         // Size of scratch space for eckey operations

                    UINT32                  coFactorPower;  // The cofactor of the curve will be equal to 2^coFactorPower

                    // Parameters V2 Extensions
                    UINT32                  PrivateKeyDefaultFormat;
                    UINT32                  HighBitRestrictionNumOfBits;
                    UINT32                  HighBitRestrictionPosition;
                    UINT32                  HighBitRestrictionValue;

                    union {

                        SYMCRYPT_ECURVE_INFO_PRECOMP sw;    // Info for short Weierstrass curves (only the precomputation parameters are needed now)

                    } info;                                 // Precomputed information related to each curve

                    PSYMCRYPT_MODULUS       FMod;           // Field modulus
                    PSYMCRYPT_MODULUS       GOrd;           // Order of the subgroup

                    PSYMCRYPT_MODELEMENT    A;              // Parameter A
                    PSYMCRYPT_MODELEMENT    B;              // Parameter B
                    PSYMCRYPT_ECPOINT       G;              // Distinguished point (generator of the subgroup)
                    PSYMCRYPT_INT           H;              // Cofactor of the curve

                    SYMCRYPT_MAGIC_FIELD

                    // FMod
                    // A
                    // B
                    // GOrd
                    // H
                    // G
} SYMCRYPT_ECURVE;
typedef       SYMCRYPT_ECURVE * PSYMCRYPT_ECURVE;
typedef const SYMCRYPT_ECURVE * PCSYMCRYPT_ECURVE;

#define SYMCRYPT_INTERNAL_ECPOINT_COORDINATE_OFFSET( _pCurve, _ord )        ( sizeof(SYMCRYPT_ECPOINT) + (_ord) * (_pCurve)->cbModElement )
#define SYMCRYPT_INTERNAL_ECPOINT_COORDINATE( _ord, _pCurve, _pEcpoint )    (PSYMCRYPT_MODELEMENT)( (PBYTE)(_pEcpoint) + SYMCRYPT_INTERNAL_ECPOINT_COORDINATE_OFFSET( (_pCurve), _ord ) )

// Convenience macros to make adding internal specializations easier
#define SYMCRYPT_CURVE_IS_SHORT_WEIERSTRASS_TYPE( _pCurve ) \
    ( _pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS || \
      _pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_SHORT_WEIERSTRASS_AM3 )

#define SYMCRYPT_CURVE_IS_TWISTED_EDWARDS_TYPE( _pCurve ) \
    ( _pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_TWISTED_EDWARDS )

#define SYMCRYPT_CURVE_IS_MONTGOMERY_TYPE( _pCurve ) \
    ( _pCurve->type == SYMCRYPT_INTERNAL_ECURVE_TYPE_MONTGOMERY )

//
// Scratch space sizes for ECURVE operations
//
//  Overflow protection is enforced when creating the ECURVE objects on
//  the cbScratchCommon, cbScratchScalar, cbScratchScalarMulti, and cbScratchEckey fields.
//
//  All of them are upper bounded by 2^26 (see SymCrypt<CurveType>FillScratchSpaces functions)
//  and since _nPoints is bounded by SYMCRYPT_ECURVE_MULTI_SCALAR_MUL_MAX_NPOINTS = 2, all
//  the macros are bounded by 2^27.
//

#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_COMMON_ECURVE_OPERATIONS( _pCurve )                 ( (_pCurve)->cbScratchCommon)
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_SCALAR_ECURVE_OPERATIONS( _pCurve, _nPoints )       ( (_pCurve)->cbScratchScalar + \
                                                                                                (_nPoints) * (_pCurve)->cbScratchScalarMulti )
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_GETSET_VALUE_ECURVE_OPERATIONS( _pCurve )           ( (_pCurve)->cbScratchGetSetValue)
#define SYMCRYPT_INTERNAL_SCRATCH_BYTES_FOR_ECKEY_ECURVE_OPERATIONS( _pCurve )                  ( (_pCurve)->cbScratchEckey)

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_ECPOINT {
                    BOOLEAN normalized;     // A flag specifying whether the point is normalized or not. This flag
                                            // makes sense only for PROJECTIVE, JACOBIAN, EXTENDED_PROJECTIVE, and
                                            // SINGLE_PROJECTIVE coordinates. If set to TRUE (non-zero), it means
                                            // that the Z coordinate of the point is equal to 1.
                    PCSYMCRYPT_ECURVE   pCurve; // Handle to the curve which the point is on. Only used in CHKed builds for ASSERTs
                    SYMCRYPT_MAGIC_FIELD
                    // An array of MODELEMENTs. The total size will depend on the MODELEMENT size and the number of MODELEMENTs.
} SYMCRYPT_ECPOINT, *PSYMCRYPT_ECPOINT;
typedef const SYMCRYPT_ECPOINT * PCSYMCRYPT_ECPOINT;

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_ECKEY {
                    UINT32              fAlgorithmInfo; // Tracks which algorithms the key can be used in
                                                        // Also tracks which per-key selftests have been performed on this key
                                                        // A bitwise OR of SYMCRYPT_FLAG_KEY_*, SYMCRYPT_FLAG_ECKEY_*, and
                                                        // SYMCRYPT_PCT_* values
                    BOOLEAN             hasPrivateKey;  // Set to true if there is a private key set
                    PCSYMCRYPT_ECURVE   pCurve;         // Handle to the curve which created the key

                    PSYMCRYPT_ECPOINT   poPublicKey;    // Public key (ECPOINT)
                    PSYMCRYPT_INT       piPrivateKey;   // Private key

                    SYMCRYPT_MAGIC_FIELD

                    // PublicKey
                    // PrivateKey
} SYMCRYPT_ECKEY;
typedef       SYMCRYPT_ECKEY * PSYMCRYPT_ECKEY;
typedef const SYMCRYPT_ECKEY * PCSYMCRYPT_ECKEY;

SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_802_11_SAE_CUSTOM_STATE {
    PSYMCRYPT_ECURVE        pCurve;
    PCSYMCRYPT_MAC          macAlgorithm;
    PSYMCRYPT_MODELEMENT    peRand;
    PSYMCRYPT_MODELEMENT    peMask;
    PSYMCRYPT_ECPOINT       poPWE;
    BYTE                    counter;
};

//
// XMSS
//

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_XMSS_PARAMS
{
    PCSYMCRYPT_HASH hash;           // hash function
    UINT32          id;             // algorithm identifier
    UINT32          cbHashOutput;   // hash function output size, must be less than or equal to hash->resultSize
    UINT32          nWinternitzWidth;// Winternitz coefficient, width of digits in bits (chain length = 2^nWinternitzWidth)
    UINT32          nTotalTreeHeight;// number of layers times the tree height of one layer (each layer has the same height)
    UINT32          nLayers;        // hyper-tree layers, 1 for single tree
    UINT32          cbPrefix;       // length of the domain separator prefix in PRFs

    //
    // The following are derived from the above
    //
    UINT32          len1;           // number of w-bit digits in the hash output to be signed ( len1 = ceil(8n / w) )
    UINT32          len2;           // number of w-bit digits in the checksum
    UINT32          len;            // len1 + len2
    UINT32          nLayerHeight;   // tree height of a single layer (h / d)
    UINT32          cbIdx;          // size of leaf counter in bytes (for single trees cbIdx = 4)
    UINT32          nLeftShift32;   // left shift count to align the checksum digits to MSB of a 32-bit word

    BYTE            Reserved[16];   // Reserved for future use
} SYMCRYPT_XMSS_PARAMS;

typedef SYMCRYPT_XMSS_PARAMS* PSYMCRYPT_XMSS_PARAMS;
typedef const SYMCRYPT_XMSS_PARAMS* PCSYMCRYPT_XMSS_PARAMS;

struct _SYMCRYPT_XMSS_KEY;
typedef struct _SYMCRYPT_XMSS_KEY SYMCRYPT_XMSS_KEY;
typedef       SYMCRYPT_XMSS_KEY* PSYMCRYPT_XMSS_KEY;
typedef const SYMCRYPT_XMSS_KEY* PCSYMCRYPT_XMSS_KEY;


//==========================================================================
//   LMS internal structures
//==========================================================================

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_LMS_PARAMS
{
    // algorithm ID of the LMS signature scheme
    UINT32                  lmsAlgID;

    // algorithm ID of the LM-OTS signature scheme
    UINT32                  lmsOtsAlgID;

    // hash function pointer to be used as part of the LMS operations
    PCSYMCRYPT_HASH         pLmsHashFunction;

    // the height of the LMS tree. There are 2^h leaves in the tree - h
    UINT32                  nTreeHeight;

    // the number of bytes for each tree node, equals to the output length of the hash function - m, n
    UINT32                  cbHashOutput;

    // Winternitz coefficient, width of digits in bits (chain length = 2^w) - w
    UINT32                  nWinternitzChainWidth;

    // the number of n-byte string elements that make up the LM-OTS signature - p
    UINT32                  nByteStringCount;

    // the number of left-shift bits used in the checksum function Cksm - ls
    UINT32                  nChecksumLShiftBits;
} SYMCRYPT_LMS_PARAMS;
typedef       SYMCRYPT_LMS_PARAMS* PSYMCRYPT_LMS_PARAMS;
typedef const SYMCRYPT_LMS_PARAMS* PCSYMCRYPT_LMS_PARAMS;

struct _SYMCRYPT_LMS_KEY;
typedef struct _SYMCRYPT_LMS_KEY SYMCRYPT_LMS_KEY;
typedef        SYMCRYPT_LMS_KEY* PSYMCRYPT_LMS_KEY;
typedef const  SYMCRYPT_LMS_KEY* PCSYMCRYPT_LMS_KEY;

#ifndef _PREFAST_
#if SYMCRYPT_CPU_X86
#pragma warning(pop)
#endif
#endif



//////////////////////////////////////////////////////////
//
// Environment macros
//

#ifdef __cplusplus
#define SYMCRYPT_EXTERN_C extern "C" {
#define SYMCRYPT_EXTERN_C_END }
#else
#define SYMCRYPT_EXTERN_C
#define SYMCRYPT_EXTERN_C_END
#endif

//
// Callers of SymCrypt should NOT depend on the function names in these macros.
// The definition of these macros can change in future releases of the library.
//

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
typedef struct _SYMCRYPT_EXTENDED_SAVE_DATA      SYMCRYPT_EXTENDED_SAVE_DATA, *PSYMCRYPT_EXTENDED_SAVE_DATA;

#define SYMCRYPT_ENVIRONMENT_DEFS_SAVEYMM( envName ) \
    SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptSaveYmmEnv##envName( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ); \
    SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptSaveYmm( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) \
        { return SymCryptSaveYmmEnv##envName( pSaveArea ); } \
    \
    VOID SYMCRYPT_CALL SymCryptRestoreYmmEnv##envName( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ); \
    VOID SYMCRYPT_CALL SymCryptRestoreYmm( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) \
        { SymCryptRestoreYmmEnv##envName( pSaveArea ); } \

#define SYMCRYPT_ENVIRONMENT_DEFS_SAVEXMM( envName ) \
    SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptSaveXmmEnv##envName( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ); \
    SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptSaveXmm( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) \
        { return SymCryptSaveXmmEnv##envName( pSaveArea ); } \
    \
    VOID SYMCRYPT_CALL SymCryptRestoreXmmEnv##envName( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ); \
    VOID SYMCRYPT_CALL SymCryptRestoreXmm( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) \
        { SymCryptRestoreXmmEnv##envName( pSaveArea ); } \


#else

#define SYMCRYPT_ENVIRONMENT_DEFS_SAVEYMM( envName )
#define SYMCRYPT_ENVIRONMENT_DEFS_SAVEXMM( envName )

#endif

// Environment forwarding functions.
// CPUIDEX is only forwarded on CPUs that have it.
#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86
#define SYMCRYPT_ENVIRONMENT_FORWARD_CPUIDEX( envName ) \
    VOID SYMCRYPT_CALL SymCryptCpuidExFuncEnv##envName( int cpuInfo[4], int function_id, int subfunction_id ); \
    VOID SYMCRYPT_CALL SymCryptCpuidExFunc( int cpuInfo[4], int function_id, int subfunction_id ) \
        { SymCryptCpuidExFuncEnv##envName( cpuInfo, function_id, subfunction_id ); }
#else
#define SYMCRYPT_ENVIRONMENT_FORWARD_CPUIDEX( envName )
#endif

#define SYMCRYPT_ENVIRONMENT_DEFS( envName ) \
SYMCRYPT_EXTERN_C \
    VOID SYMCRYPT_CALL SymCryptInitEnv##envName( UINT32 version ); \
    VOID SYMCRYPT_CALL SymCryptInit(void) \
        { SymCryptInitEnv##envName( SYMCRYPT_API_VERSION ); } \
    \
    _Analysis_noreturn_ VOID SYMCRYPT_CALL SymCryptFatalEnv##envName( UINT32 fatalCode ); \
    _Analysis_noreturn_ VOID SYMCRYPT_CALL SymCryptFatal( UINT32 fatalCode ) \
        { SymCryptFatalEnv##envName( fatalCode ); } \
    SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnv##envName(void); \
    SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresent(void) \
        { return SymCryptCpuFeaturesNeverPresentEnv##envName(); } \
    \
    SYMCRYPT_ENVIRONMENT_DEFS_SAVEXMM( envName ) \
    SYMCRYPT_ENVIRONMENT_DEFS_SAVEYMM( envName ) \
    \
    VOID SYMCRYPT_CALL SymCryptTestInjectErrorEnv##envName( PBYTE pbBuf, SIZE_T cbBuf ); \
    VOID SYMCRYPT_CALL SymCryptInjectError( PBYTE pbBuf, SIZE_T cbBuf ) \
        { SymCryptTestInjectErrorEnv##envName( pbBuf, cbBuf ); } \
    SYMCRYPT_ENVIRONMENT_FORWARD_CPUIDEX( envName ) \
SYMCRYPT_EXTERN_C_END

//
// To avoid hard-do-diagnose mistakes, we skip defining environment macros in those cases where we
// know they cannot or should not be used.
//

#define SYMCRYPT_ENVIRONMENT_GENERIC                            SYMCRYPT_ENVIRONMENT_DEFS( Generic )

#if defined(EFI) | defined(PCAT) | defined(DIRECT)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_BOOTLIBRARY                SYMCRYPT_ENVIRONMENT_DEFS( WindowsBootlibrary )
#endif

//
// There are no defined symbols that we can use to detect that we are in debugger code
// But this is unlikely to be misused.
//
#define SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELDEBUGGER             SYMCRYPT_ENVIRONMENT_DEFS( WindowsKernelDebugger )



#define SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_LEGACY          SYMCRYPT_ENVIRONMENT_GENERIC

#ifdef NTDDI_VERSION
#if (NTDDI_VERSION >= NTDDI_WIN7)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN7_N_LATER    SYMCRYPT_ENVIRONMENT_DEFS( WindowsKernelmodeWin7nLater )
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN8_1_N_LATER  SYMCRYPT_ENVIRONMENT_DEFS( WindowsKernelmodeWin8_1nLater )
#endif

#define SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_LATEST          SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN8_1_N_LATER



#define SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_LEGACY            SYMCRYPT_ENVIRONMENT_GENERIC

#if (NTDDI_VERSION >= NTDDI_WIN7)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN7_N_LATER      SYMCRYPT_ENVIRONMENT_DEFS( WindowsUsermodeWin7nLater )
#endif

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN8_1_N_LATER    SYMCRYPT_ENVIRONMENT_DEFS( WindowsUsermodeWin8_1nLater )
#endif

#if (NTDDI_VERSION >= NTDDI_WIN10)
#define SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN10_SGX         SYMCRYPT_ENVIRONMENT_DEFS( Win10Sgx )
#endif
#endif // NTDDI_VERSION

#define SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_LATEST            SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN8_1_N_LATER


#define SYMCRYPT_ENVIRONMENT_POSIX_USERMODE                     SYMCRYPT_ENVIRONMENT_DEFS( PosixUsermode )

// For backwards compatibility with previous macro name
#define SYMCRYPT_ENVIRONMENT_LINUX_USERMODE                     SYMCRYPT_ENVIRONMENT_POSIX_USERMODE


#define SYMCRYPT_ENVIRONMENT_OPTEE_TA                           SYMCRYPT_ENVIRONMENT_DEFS( OpteeTa )

//////////////////////////////////////////////////////////
//
// SymCryptWipe & SymCryptWipeKnownSize
//

VOID
SYMCRYPT_CALL
SymCryptWipe(
    _Out_writes_bytes_(cbData)    PVOID   pbData,
    SIZE_T  cbData);

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64

//
// If the known size is large we call the generic wipe function anyway.
// For small known sizes we perform the wipe inline.
// This is a tradeoff between speed and code size and there are diminishing returns to supporting
// increasingly large sizes.
// We currently put the limit at ~8 native writes, which varies by platform.
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_ARM
#define SYMCRYPT_WIPE_FUNCTION_LIMIT (32)   // If this is increased beyond 127 the code below must be updated.
#elif SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_ARM64
#define SYMCRYPT_WIPE_FUNCTION_LIMIT (64)   // If this is increased beyond 127 the code below must be updated.
#else
#error ??
#endif

//
// The buffer analysis code doesn't understand our optimized in-line wiping code
// well enough to conclude it is safe.
//
#pragma prefast(push)
#pragma prefast( disable: 26001 )

FORCEINLINE
VOID
SYMCRYPT_CALL
#pragma prefast( suppress: 6101, "Logic why this properly initializes the pbData buffer is too complicated for prefast" )
SymCryptWipeKnownSize(_Out_writes_bytes_(cbData) PVOID pbData, SIZE_T cbData)
{
    volatile BYTE * pb = (volatile BYTE *)pbData;

    if (cbData > SYMCRYPT_WIPE_FUNCTION_LIMIT)
    {
        SymCryptWipe(pbData, cbData);
    }
    else
    {
        //
        // We assume that pb is aligned, so we wipe from the end to the front to keep alignment.
        //
        if (cbData & 1)
        {
            cbData--;
            SYMCRYPT_INTERNAL_FORCE_WRITE8((volatile BYTE *)&pb[cbData], 0);
        }
        if (cbData & 2)
        {
            cbData -= 2;
            SYMCRYPT_INTERNAL_FORCE_WRITE16((volatile UINT16 *)&pb[cbData], 0);
        }
        if (cbData & 4)
        {
            cbData -= 4;
            SYMCRYPT_INTERNAL_FORCE_WRITE32((volatile UINT32 *)&pb[cbData], 0);
        }
        if (cbData & 8)
        {
            cbData -= 8;
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData], 0);
        }
        if (cbData & 16)
        {
            cbData -= 16;
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 8], 0);
        }
        if (cbData & 32)
        {
            cbData -= 32;
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 8], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 16], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 24], 0);
        }
#if SYMCRYPT_WIPE_FUNCTION_LIMIT >= 64
        if (cbData & 64)
        {
            cbData -= 64;
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 8], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 16], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 24], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 32], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 40], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 48], 0);
            SYMCRYPT_INTERNAL_FORCE_WRITE64((volatile UINT64 *)&pb[cbData + 56], 0);
        }
#endif
    }
}

#pragma prefast(pop)

#else // Platform switch for SymCryptWipeKnownSize

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptWipeKnownSize(_Out_writes_bytes_(cbData) PVOID pbData, SIZE_T cbData)
{
    SymCryptWipe(pbData, cbData);
}

#endif  // Platform switch for SymCryptWipeKnownSize

#define SYMCRYPT_FIPS_ASSERT(x) { if(!(x)){ SymCryptFatal('FIPS'); } }

// Flags for FIPS on-demand selftests. When an on-demand selftest succeeds, the corresponding flag
// will be set in g_SymCryptFipsSelftestsPerformed. Other selftests are performed automatically
// when the module is loaded, so they don't have a corresponding flag.
typedef enum _SYMCRYPT_SELFTEST_ALGORITHM {
    SYMCRYPT_SELFTEST_ALGORITHM_NONE    =   0x0,
    SYMCRYPT_SELFTEST_ALGORITHM_STARTUP =   0x1,
    SYMCRYPT_SELFTEST_ALGORITHM_DSA     =   0x2,
    SYMCRYPT_SELFTEST_ALGORITHM_ECDSA   =   0x4,
    SYMCRYPT_SELFTEST_ALGORITHM_RSA     =   0x8,
    SYMCRYPT_SELFTEST_ALGORITHM_DH      =  0x10,
    SYMCRYPT_SELFTEST_ALGORITHM_ECDH    =  0x20,
    SYMCRYPT_SELFTEST_ALGORITHM_MLKEM   =  0x40,
    SYMCRYPT_SELFTEST_ALGORITHM_XMSS    =  0x80,
    SYMCRYPT_SELFTEST_ALGORITHM_LMS     = 0x100,
    SYMCRYPT_SELFTEST_ALGORITHM_MLDSA   = 0x200,
} SYMCRYPT_SELFTEST_ALGORITHM;

// Takes values which are some bitwise OR combination of SYMCRYPT_SELFTEST_ALGORITHM values
// Specified as UINT32 as we will update with 32 bit atomics, and compilers may choose to make enum
// types smaller than 32 bits.
extern UINT32 g_SymCryptFipsSelftestsPerformed;

UINT32
SYMCRYPT_CALL
SymCryptFipsGetSelftestsPerformed(void);
// Returns current value of g_SymCryptFipsSelftestsPerformed so callers may inspect which FIPS
// algorithm selftests have run

// Flags for per-key selftests.
// When an asymmetric key is generated or imported, and SYMCRYPT_FLAG_KEY_NO_FIPS is not specified,
// some selftests must be performed on the key, before its operational use in an algorithm, to
// comply with FIPS.
// The algorithms the key may be used in will be tracked in the key's fAlgorithmInfo field, as a
// bitwise OR of SYMCRYPT_FLAG_<keytype>_<algorithm> (e.g. SYMCRYPT_FLAG_DLKEY_DH).
// This field will also track which per-key selftests have been run on the key using the below flags
// We want to track which selftests have been run independently of which algorithms the key may be
// used in as in some scenarios at key generation / import time we may not know what algorithm the
// key will actually be used in. Tracking the run per-key selftests in fAlgorithmInfo allows us to
// defer running expensive tests until we know they are required (e.g. if we generate an Eckey which
// may be used in ECDH or ECDSA, and only use it for ECDH, the ECDSA PCT is deferred until we first
// attempt to use the key in ECDSA, or export the private key).
//
// For clarity, SYMCRYPT_PCT_* should be used instead of SYMCRYPT_SELFTEST_KEY_* going forward.
// The latter is retained for compatibility with existing code, but may be removed in a future
// breaking change.

// Dlkey selftest flags
// DSA Pairwise Consistency Test to be run on generated keys
#define SYMCRYPT_SELFTEST_KEY_DSA       (0x1)
#define SYMCRYPT_PCT_DSA                SYMCRYPT_SELFTEST_KEY_DSA

// Eckey selftest flags
// ECDSA Pairwise Consistency Test to be run on generated keys
#define SYMCRYPT_SELFTEST_KEY_ECDSA     (0x1)
#define SYMCRYPT_PCT_ECDSA              SYMCRYPT_SELFTEST_KEY_ECDSA

// Rsakey selftest flags
// RSA Pairwise Consistency Test to be run on generated keys
#define SYMCRYPT_SELFTEST_KEY_RSA_SIGN  (0x1)
#define SYMCRYPT_PCT_RSA_SIGN           SYMCRYPT_SELFTEST_KEY_RSA_SIGN

UINT32
SYMCRYPT_CALL
SymCryptDeprecatedStatusIndicator(PBYTE pbOutput, UINT32 cbOutput);
//
// Returns the FIPS Approved Services Status Indicator as an ASCII string.
// This API is required to satisfy FIPS 140-3 requirements, but is *not* recommended
// to be used in production code. It should be considered unstable,
// and may be removed at any time.
//
// The output string will be copied to pbOutput if the size of the buffer
// cbOutput is large enough. The function returns the required buffer size
// when pbOutput is passed as NULL. If pbOutput is not NULL, the function
// returns the number of bytes copied to pbOutput.
//



typedef enum _SYMCRYPT_SI_TYPE {

    // Algorithm types (specific algorithms are represented as a bitmask of a type)
    SYMCRYPT_SI_TYPE_CIPHER             = 0x01,
    SYMCRYPT_SI_TYPE_HASH               = 0x02,
    SYMCRYPT_SI_TYPE_MAC                = 0x03,
    SYMCRYPT_SI_TYPE_KDF                = 0x04,
    SYMCRYPT_SI_TYPE_DRBG               = 0x05,
    SYMCRYPT_SI_TYPE_ASYM_ALG           = 0x06,
    SYMCRYPT_SI_TYPE_KAS                = 0x07,
    SYMCRYPT_SI_TYPE_KEM                = 0x08,

    // Other types where elements are a bitmask
    SYMCRYPT_SI_TYPE_ECURVE             = 0x40,
    SYMCRYPT_SI_TYPE_KAS_SCHEME         = 0x41,
    SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP   = 0x42,

    // Non-bitmask types
    SYMCRYPT_SI_TYPE_INTRANGE           = 0x80,
    SYMCRYPT_SI_TYPE_INTPAIR            = 0x81,
    SYMCRYPT_SI_TYPE_SIZERANGE          = 0x82,

    SYMCRYPT_SI_TYPE_MAX                = 0xFF
} SYMCRYPT_SI_TYPE;

#define SYMCRYPT_SI_CREATE_ID(type, index)  (((UINT64)(type) << 56) + (1ULL << (index)))

#define SYMCRYPT_SI_INTBITS                 ((64 - 8) / 2)  // 8-bits for type, remaining bits shared by two integers
#define SYMCRYPT_SI_INTMASK                 ((1ULL << SYMCRYPT_SI_INTBITS) - 1) // typically should be 0x0FFFFFFF with 28 1s
#define SYMCRYPT_SI_INTPACK(High, Low)      (((((UINT64)High) & SYMCRYPT_SI_INTMASK) << SYMCRYPT_SI_INTBITS) | (((UINT64)Low) & SYMCRYPT_SI_INTMASK))
#define SYMCRYPT_SI_INTUNPACKLO(X)          ((X) & SYMCRYPT_SI_INTMASK)
#define SYMCRYPT_SI_INTUNPACKHI(X)          (((X) >> SYMCRYPT_SI_INTBITS) & SYMCRYPT_SI_INTMASK)

#define SYMCRYPT_SI_INTRANGE(Low, High)     (((UINT64)SYMCRYPT_SI_TYPE_INTRANGE << 56) | SYMCRYPT_SI_INTPACK(High, Low))
#define SYMCRYPT_SI_INTPAIR(X, Y)           (((UINT64)SYMCRYPT_SI_TYPE_INTPAIR << 56) | SYMCRYPT_SI_INTPACK(Y, X))
#define SYMCRYPT_SI_SIZERANGE(Low, High)    (((UINT64)SYMCRYPT_SI_TYPE_SIZERANGE << 56) | SYMCRYPT_SI_INTPACK(High, Low))

#define SYMCRYPT_SI_CHECK_INT(L)            C_ASSERT(L <= SYMCRYPT_SI_INTMASK)

#define SYMCRYPT_SI_KEYBITS(L)              SYMCRYPT_SI_SIZERANGE(L, L)
#define SYMCRYPT_SI_MODULUS(L)              SYMCRYPT_SI_SIZERANGE(L, L)
#define SYMCRYPT_SI_DSAPARAMS(N, L)         SYMCRYPT_SI_INTPAIR(N, L)


// Services
#define SYMCRYPT_SI_SVC_ENCRYPTION                      0x00000001
#define SYMCRYPT_SI_SVC_DECRYPTION                      0x00000002
#define SYMCRYPT_SI_SVC_HASHING                         0x00000004
#define SYMCRYPT_SI_SVC_MESSAGE_AUTHENTICATION          0x00000008
#define SYMCRYPT_SI_SVC_KEY_DERIVATION                  0x00000010
#define SYMCRYPT_SI_SVC_ASYMMETRIC_KEY_GENERATION       0x00000020
#define SYMCRYPT_SI_SVC_ASYMMETRIC_KEY_VERIFICATION     0x00000080
#define SYMCRYPT_SI_SVC_RANDOM_NUMBER_GENERATION        0x00000400
#define SYMCRYPT_SI_SVC_SECRET_AGREEMENT                0x00000800
#define SYMCRYPT_SI_SVC_SIGNATURE_GENERATION            0x00001000
#define SYMCRYPT_SI_SVC_SIGNATURE_VERIFICATION          0x00002000
#define SYMCRYPT_SI_SVC_KEY_ENCAPSULATION               0x00004000
#define SYMCRYPT_SI_SVC_KEY_DECAPSULATION               0x00008000

// Ciphers
#define SYMCRYPT_SI_AES_CBC                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 0)
#define SYMCRYPT_SI_AES_CCM                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 1)
#define SYMCRYPT_SI_AES_CFB128              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 2)
#define SYMCRYPT_SI_AES_CFB8                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 3)
#define SYMCRYPT_SI_AES_CTR                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 4)
#define SYMCRYPT_SI_AES_ECB                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 5)
#define SYMCRYPT_SI_AES_GCM                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 6)
#define SYMCRYPT_SI_AES_XTS                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 7)
#define SYMCRYPT_SI_RC2                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 8)
#define SYMCRYPT_SI_RC4                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 9)
#define SYMCRYPT_SI_CHACHA                  SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 10)
#define SYMCRYPT_SI_DES                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 11)
#define SYMCRYPT_SI_TRIPLEDES               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 12)
#define SYMCRYPT_SI_CHACHA20                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 13)
#define SYMCRYPT_SI_CHACHA20_POLY1305       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 14)
#define SYMCRYPT_SI_AES_KW                  SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 15)
#define SYMCRYPT_SI_AES_KWP                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_CIPHER, 16)

// Hash Functions
#define SYMCRYPT_SI_MD2                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 0)
#define SYMCRYPT_SI_MD4                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 1)
#define SYMCRYPT_SI_MD5                     SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 2)
#define SYMCRYPT_SI_SHA1                    SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 3)
#define SYMCRYPT_SI_SHA2_224                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 4)
#define SYMCRYPT_SI_SHA2_256                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 5)
#define SYMCRYPT_SI_SHA2_384                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 6)
#define SYMCRYPT_SI_SHA2_512                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 7)
#define SYMCRYPT_SI_SHA2_512_224            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 8)
#define SYMCRYPT_SI_SHA2_512_256            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 9)
#define SYMCRYPT_SI_SHA3_224                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 10)
#define SYMCRYPT_SI_SHA3_256                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 11)
#define SYMCRYPT_SI_SHA3_384                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 12)
#define SYMCRYPT_SI_SHA3_512                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 13)
#define SYMCRYPT_SI_SHAKE128                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 14)
#define SYMCRYPT_SI_SHAKE256                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 15)
#define SYMCRYPT_SI_CSHAKE128               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 16)
#define SYMCRYPT_SI_CSHAKE256               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 17)
#define SYMCRYPT_SI_MARVIN32                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_HASH, 18)

// MAC
#define SYMCRYPT_SI_HMAC_MD2                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 0)
#define SYMCRYPT_SI_HMAC_MD4                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 1)
#define SYMCRYPT_SI_HMAC_MD5                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 2)
#define SYMCRYPT_SI_HMAC_SHA1               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 3)
#define SYMCRYPT_SI_HMAC_SHA2_224           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 4)
#define SYMCRYPT_SI_HMAC_SHA2_256           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 5)
#define SYMCRYPT_SI_HMAC_SHA2_384           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 6)
#define SYMCRYPT_SI_HMAC_SHA2_512           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 7)
#define SYMCRYPT_SI_HMAC_SHA2_512_224       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 8)
#define SYMCRYPT_SI_HMAC_SHA2_512_256       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 9)
#define SYMCRYPT_SI_HMAC_SHA3_224           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 10)
#define SYMCRYPT_SI_HMAC_SHA3_256           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 11)
#define SYMCRYPT_SI_HMAC_SHA3_384           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 12)
#define SYMCRYPT_SI_HMAC_SHA3_512           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 13)
#define SYMCRYPT_SI_KMAC128                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 14)
#define SYMCRYPT_SI_KMAC256                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 15)
#define SYMCRYPT_SI_AES_GMAC                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 16)
#define SYMCRYPT_SI_AES_CMAC                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 17)
#define SYMCRYPT_SI_AES_CBCMAC              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 18)
#define SYMCRYPT_SI_POLY1305                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_MAC, 19)

// KDF
#define SYMCRYPT_SI_HKDF                    SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 0)
#define SYMCRYPT_SI_PBKDF                   SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 1)
#define SYMCRYPT_SI_KDA_ONESTEP             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 2)
#define SYMCRYPT_SI_KDF_IKEV1               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 3)
#define SYMCRYPT_SI_KDF_IKEV2               SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 4)
#define SYMCRYPT_SI_KDF_SP800_108_CTR       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 5)
#define SYMCRYPT_SI_KDF_SRTP                SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 6)
#define SYMCRYPT_SI_KDF_SSH                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 7)
#define SYMCRYPT_SI_KDF_TLS                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 8)
#define SYMCRYPT_SI_KDF_TLS_V12             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KDF, 9)

// DRBG
#define SYMCRYPT_SI_CTR_DRBG_AES256         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_DRBG, 0)

// Asymmetric Algorithms
#define SYMCRYPT_SI_SAFE_PRIME_KEYGEN       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 0)
#define SYMCRYPT_SI_DSA_KEYGEN              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 1)
#define SYMCRYPT_SI_DSA_PQGGEN              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 2)
#define SYMCRYPT_SI_DSA_PQGVER              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 3)
#define SYMCRYPT_SI_DSA_SIGVER              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 4)

#define SYMCRYPT_SI_ECDSA_KEYGEN            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 5)
#define SYMCRYPT_SI_ECDSA_KEYVER            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 6)
#define SYMCRYPT_SI_ECDSA_SIGGEN            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 7)
#define SYMCRYPT_SI_ECDSA_SIGVER            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 8)
#define SYMCRYPT_SI_ECDSA_SIGGEN_COMP       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 9)

#define SYMCRYPT_SI_RSA_KEYGEN              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 10)
#define SYMCRYPT_SI_RSA_DEC_PRIM            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 12)
#define SYMCRYPT_SI_RSA_SIG_PRIM            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 13)
#define SYMCRYPT_SI_RSA_SIGGEN_PKCS15       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 14)
#define SYMCRYPT_SI_RSA_SIGGEN_PKCSPSS      SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 15)
#define SYMCRYPT_SI_RSA_SIGVER_PKCS15       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 16)
#define SYMCRYPT_SI_RSA_SIGVER_PKCSPSS      SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 17)

#define SYMCRYPT_SI_KAS_ECC                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 18)
#define SYMCRYPT_SI_KAS_ECC_SSC             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 19)
#define SYMCRYPT_SI_KAS_FFC                 SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 20)
#define SYMCRYPT_SI_KAS_FFC_SSC             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 21)

// PQ Algorithms

// Asym Alg IDs for PQC algorithms in range 22-26 are replaced with more granular
// algorithms as below.
// Keeping this range reserved until there's a need to use it in the future.

#define SYMCRYPT_SI_MLDSA_KEYGEN            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 27)
#define SYMCRYPT_SI_MLDSA_SIGGEN            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 28)
#define SYMCRYPT_SI_MLDSA_SIGVER            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 29)
#define SYMCRYPT_SI_LMS_KEYGEN              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 30)
#define SYMCRYPT_SI_LMS_SIGGEN              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 31)
#define SYMCRYPT_SI_LMS_SIGVER              SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 32)
#define SYMCRYPT_SI_XMSS_KEYGEN             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 33)
#define SYMCRYPT_SI_XMSS_SIGGEN             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 34)
#define SYMCRYPT_SI_XMSS_SIGVER             SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 35)
#define SYMCRYPT_SI_XMSS_MT_KEYGEN          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 36)
#define SYMCRYPT_SI_XMSS_MT_SIGGEN          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 37)
#define SYMCRYPT_SI_XMSS_MT_SIGVER          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ASYM_ALG, 38)

#define SYMCRYPT_SI_MLKEM_KEYGEN            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KEM, 0)
#define SYMCRYPT_SI_MLKEM_ENCAPS            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KEM, 1)
#define SYMCRYPT_SI_MLKEM_DECAPS            SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KEM, 2)


// Elliptic Curves
#define SYMCRYPT_SI_ECURVE_NISTP192         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 0)
#define SYMCRYPT_SI_ECURVE_NISTP224         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 1)
#define SYMCRYPT_SI_ECURVE_NISTP256         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 2)
#define SYMCRYPT_SI_ECURVE_NISTP384         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 3)
#define SYMCRYPT_SI_ECURVE_NISTP521         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 4)
#define SYMCRYPT_SI_ECURVE_NUMSP256T1       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 5)
#define SYMCRYPT_SI_ECURVE_NUMSP384T1       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 6)
#define SYMCRYPT_SI_ECURVE_NUMSP512T1       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 7)
#define SYMCRYPT_SI_ECURVE_CURVE25519       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_ECURVE, 8)

// Safe Prime Groups
#define SYMCRYPT_SI_SPG_FFDHE_2048          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 0)
#define SYMCRYPT_SI_SPG_FFDHE_3072          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 1)
#define SYMCRYPT_SI_SPG_FFDHE_4096          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 2)
#define SYMCRYPT_SI_SPG_FFDHE_6144          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 3)
#define SYMCRYPT_SI_SPG_FFDHE_8192          SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 4)
#define SYMCRYPT_SI_SPG_MODP_2048           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 5)
#define SYMCRYPT_SI_SPG_MODP_3072           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 6)
#define SYMCRYPT_SI_SPG_MODP_4096           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 7)
#define SYMCRYPT_SI_SPG_MODP_6144           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 8)
#define SYMCRYPT_SI_SPG_MODP_8192           SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_SAFE_PRIME_GROUP, 9)

// KAS Schemes
#define SYMCRYPT_SI_SCHEME_EPHEM_UNIFIED    SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KAS_SCHEME, 0)
#define SYMCRYPT_SI_SCHEME_DH_EPHEM         SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KAS_SCHEME, 1)
#define SYMCRYPT_SI_SCHEME_DH_ONEFLOW       SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KAS_SCHEME, 2)
#define SYMCRYPT_SI_SCHEME_DH_STATIC        SYMCRYPT_SI_CREATE_ID(SYMCRYPT_SI_TYPE_KAS_SCHEME, 3)


UINT32
SYMCRYPT_CALL
SymCryptDeprecatedServiceIndicator(
    UINT32 Service,
    UINT64 Alg,
    UINT64 Param1,
    UINT64 Param2,
    UINT64 Param3);
//
// Returns FIPS 140 Approved Services Indicator for an algorithm.
//
// Parameters:
//  - Service. Service identifier, one of SYMCRYPT_SI_SVC_XXX.
//  - Alg. Identifier of the algorithm for which the status is being queried. This must be
//  exactly one of the algorithm identifiers defined above.
//  - Param1, Param2, Param3. Depending on the Alg parameter, these parameters provide
//  additional information about the capabilities and parameters associated with an
//  algorithm. For each algorithm, the number and type of the parameters must be provided
//  as specified below. Any unused parameters must be passed as 0. The algorithms that require
//  parameters to be specified are listed below, the remaining algorithms do not have any parameters.
//
//      Alg Id                          Param1                              Param2
//      -----------------------------   --------------------------------    ---------------
//      SYMCRYPT_SI_AES_XTS             SYMCRYPT_SI_KEYBITS(int)                -
//      SYMCRYPT_SI_DSA_PQGVER          SYMCRYPT_SI_DSAPARAMS(int, int)         -
//      SYMCRYPT_SI_DSA_SIGVER          SYMCRYPT_SI_DSAPARAMS(int, int)         -
//      SYMCRYPT_SI_ECDSA_KEYGEN        SYMCRYPT_SI_ECURVE_XXX                  -
//      SYMCRYPT_SI_ECDSA_KEYVER        SYMCRYPT_SI_ECURVE_XXX                  -
//      SYMCRYPT_SI_ECDSA_SIGGEN        SYMCRYPT_SI_ECURVE_XXX                  Hash Alg Id
//      SYMCRYPT_SI_ECDSA_SIGGEN_COMP   SYMCRYPT_SI_ECURVE_XXX                  Hash Alg Id
//      SYMCRYPT_SI_ECDSA_SIGVER        SYMCRYPT_SI_ECURVE_XXX                  Hash Alg Id
//      SYMCRYPT_SI_RSA_DEC_PRIM        SYMCRYPT_SI_MODULUS(int)                -
//      SYMCRYPT_SI_RSA_KEYGEN          SYMCRYPT_SI_MODULUS(int)                -
//      SYMCRYPT_SI_RSA_SIGGEN_PKCS15   SYMCRYPT_SI_MODULUS(int)                Hash Alg Id
//      SYMCRYPT_SI_RSA_SIGVER_PKCS15   SYMCRYPT_SI_MODULUS(int)                Hash Alg Id
//      SYMCRYPT_SI_RSA_SIGGEN_PKCSPSS  SYMCRYPT_SI_MODULUS(int)                Hash Alg Id
//      SYMCRYPT_SI_RSA_SIGVER_PKCSPSS  SYMCRYPT_SI_MODULUS(int)                Hash Alg Id
//      SYMCRYPT_SI_SAFE_PRIME_KEYGEN   SYMCRYPT_SI_SPG_XXX                     Hash Alg Id
//      SYMCRYPT_SI_HMAC_XXX            SYMCRYPT_SI_KEYBITS(int)                -
//      SYMCRYPT_SI_KDA_ONESTEP         Hash Alg Id or MAC alg Id               -
//      SYMCRYPT_SI_PBKDF               MAC Alg Id                              -
//      SYMCRYPT_SI_KDF_SP800_108_CTR   MAC Alg Id                              -
//      SYMCRYPT_SI_KDF_SSH             Hash Alg Id                             -
//      SYMCRYPT_SI_TLS_V12_KDF         Hash Alg Id                             -
//      SYMCRYPT_SI_KAS_ECC             SYMCRYPT_SI_ECURVE_XXX                  Hash Alg Id
//      SYMCRYPT_SI_KAS_ECC_SSC         SYMCRYPT_SI_ECURVE_XXX                  SYMCRYPT_SI_SCHEME_XXX
//      SYMCRYPT_SI_KAS_FFC             SYMCRYPT_SI_SPG_XXX                     Hash Alg Id
//      SYMCRYPT_SI_KAS_FFC_SSC         SYMCRYPT_SI_SPG_XXX                     SYMCRYPT_SI_SCHEME_XXX
//      SYMCRYPT_SI_LMS_SIGVER          SYMCRYPT_LMS_XXX                        -
//      SYMCRYPT_SI_XMSS_SIGVER         SYMCRYPT_XMSS_XXX                       -
//      SYMCRYPT_SI_XMSS_MT_SIGVER      SYMCRYPT_XMSSMT_XXX                     -
//
//
// Return value:
//  For the specified service and algorithm (and parameters if any), the function
//  returns 0 if SymCrypt implements the algorithm in an approved manner. A non-zero
//  value indicates either the algorithm is non-approved or the parameters were invalid.
//
// Remarks:
// - For parameters that contain integer values, the callers must ensure that the values
// are within the acceptable limits by using the SYMCRYPT_SI_CHECK_INT(L) macro.
