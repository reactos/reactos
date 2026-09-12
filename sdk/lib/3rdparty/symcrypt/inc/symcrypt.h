//
// SymCrypt.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include "symcrypt_internal_shared.inc"

#define SYMCRYPT_API_VERSION ((SYMCRYPT_CODE_VERSION_API << 16) | SYMCRYPT_CODE_VERSION_MINOR)

//
// This is the header file for the SymCrypt library which contains
// implementations of cryptographic algorithms.
//
// All API information is in this file. Information in the
// other include files (symcrypt_internal.h) is subject
// to change at any time. Please use only the information in this file.
// The header file symcrypt_low_level contains low-level API functions that
// are sometimes needed. That API surface is not stable across releases.
//

;   // <-- non-functional semicolon that makes the editor's indent work properly.

//
// General information about SymCrypt:
//
//
// CPU
// This library is built and tested for: X86, AMD64, ARM, and ARM64.
//
// ENVIRONMENT
// SymCrypt can run in different environments, such as kernel mode, user mode,
// etc.
// In earlier versions of the library, the caller specified the environment by passing a
// pointer to the SymCryptInit function.
// It turns out that that model no longer scales with the use of new extended register sets
// or it introduces too much overhead.
// The current library uses a different model. The user of the library invokes one of the
// environment macros inside a C file in the calling process.
// SymCrypt defines macros for each environment.
// The same mechanism will also be used to select between different implementations of a single
// algorithm. For example, a caller might use
//      SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE
//      SYMCRYPT_SELECT_SHA256_COMPACT
// to indicate that the environment is kernel mode and the compact SHA-256 implementation is to
// be used.
// There are optimized environments for various Windows use cases.
//
//
// CHECKED BUILDS
// For each CPU, SymCrypt is available in both a checked build and a fre build. The
// checked build includes additional error checking which catches the most common
// errors. Please make sure you build a checked version of your binary and test with
// that regularly.
//
//
// MEMORY STRUCTURES
// Most SymCrypt functions do not allocate any memory; all memory is provided by the caller.
// However, callers may not copy, move, or otherwise manipulate the SymCrypt
// data structures. In particular, a memcpy of a SymCrypt data structure is not allowed.
// When necessary SymCrypt provides functions to perform the necessary manipulations.
// If you are missing one, please ask us.
//
//
// MULTI_THREADING
// The routines in this library are multi-thread safe, taking into account the usual
// rules of multiple threads accessing the same data structures.
// Any function that accepts a pointer-to-const argument must be assumed to read the
// corresponding data. If the function accepts a pointer-to-non-const it must be
// assumed to both read and write the data.
// It is safe for two threads to use the same data element as long as both of them
// are only reading from it. For example, an expanded key is typically passed as
// a pointer-to-const to the encryption and decryption routines. Thus, multiple
// threads can perform multiple encryptions/decryptions in parallel using the
// same expanded key.
//
// The normal memory re-order issues apply as well. If one thread initializes a
// data structure and the initialization function returns, it is NOT safe for
// another thread to read the data structure without a suitable memory barrier or
// synchronization primitive.
//
//
// SIDE CHANNELS
// Side channels are ways in which an attacker can receive information about what
// a target process is doing using other aspects than just the input/output behaviour
// of the target. For example, the memory subsystem, CPU load modulation, disk usage,
// and many other aspects can provide side-channels to an attacker.
//
// Wherever possible the implementations in SymCrypt have been hardened against side channels.
// The most important rules are that the instruction sequence and the memory addresses
// accessed do not depend on any of the data being processed.
// As a general rule, the actual data being processed is protected, but the
// length of the data (i.e. the number of bytes) is not protected in this way and
// is treated as public information.
//
// The implementation of the following algorithms are NOT side-channel safe:
//  - non-AES-NI based AES
//      used on CPUs that don't have AES-NI, or in kernel mode on x86 Win8 and below.
//  - DES, 3DES, DESX
//  - RC4
// Making these algorithms side-channel safe would incur an overhead that is too large.
//
//
// FATAL ERRORS
// This is a high-performance library with a minimum of error checking.
// Many functions do not return an error code; this avoids the cost of
// having any error checking on the caller's side for error situations that
// can never occur. However, this does assume that the caller is calling
// SymCrypt using a valid calling sequence with proper parameters.
// In some situations this library will detect improper parameters or
// calling sequences. In those situations the library will generate a fatal
// error, which leads to an abrupt termination of the process (bugcheck in
// kernel mode). Exceptional circumstances may also induce fatal errors within
// the library (i.e. a caller provided buffer causes an access violation when
// it is read, or the library is called without sufficient stack space for the
// requested operation).
// If a fatal error is generated within the library, the internal state of the
// library may be inconsistent (i.e. there may be outstanding memory allocations
// that will never be freed, or a lock may have been taken which will never be
// released). Callers should not catch fatal errors and continue executing, as
// there is no guarantee of stability.
// The checked version of the library has additional error checking which detects
// the most common errors. We strongly recommend that callers build and test a
// checked version of their binary to catch these common errors.
//
//
// ALGORITHM SELF TEST
// SymCrypt includes functions that perform simple self-tests on the algorithm
// implementations. These functions are designed to be used for FIPS certification
// of crypto binaries. They should never fail, and they generate a fatal error
// if they do fail.
// If you are not FIPS-certifying your binaries, you can ignore the self test functions.
//
//
// CHANGES FROM RSA32.LIB
// This library replaces the venerable rsa32(k).lib. The major changes are:
//
// - SymCrypt requires the caller to call a library initialization function
//   before calling the various algorithm implementations.
// - SymCrypt requires the caller to specify the environment in which the library
//   is running.
// - SymCrypt has a CHKed and FRE version for use in CHKed and FRE builds.
// - The API has been updated. The API is more consistent and has better support
//   for 64-bit platforms (use of SIZE_T rather than UINT32 for lengths).
// - All algorithm implementations have been updated to reflect the
//   latest cryptographic coding guidelines. Several security weaknesses
//   in the RSA32.lib code have been fixed.
// - Code has been optimized for the newer CPUs.
//   This includes support for AES-NI, PCLMULQDQ, AVX2, etc.
//   Most algorithms are faster, especially the recommended algorithms.
//   Some legacy algorithms are somewhat slower due to removal of assembler support.
//   Note: performance on older CPUs, like the Pentium 4, is reduced in some places.
// - Code and data now go into their default segments.
//   RSA32 has a kernel-mode version where the code and data go into
//   special segments. This allows the crypto code to be made pageable or
//   nonpageable separate from the rest of the executable. This feature is
//   error-prone, and not widely used. Furthermore, it switches on a per-lib
//   basis, rather than a per-functionality basis, which is the wrong granularity.
// - Added native support for HMAC-SHA256 and HMAC-SHA512.
// - Support for parallel hashing, improves throughput up to 500%.
// - SymCrypt does not support binary copying of internal state information, because
//   it imposes restrictions on what the library can do.
//   Thus, you may NOT do a memcpy or remote copy on any SymCrypt data structure.
//   SymCrypt provides copy functions where necessary, if you need others please ask.
//

//
// Error codes
//
// This is a high-performance library with a minimum of error checking. Most
// routines do not perform any error checking at all.
// Some routines perform internal consistency checks and will cause a fatal
// error if the library is used incorrectly.
//
// In a few cases routines return an error code when they are called incorrectly.
// Mostly this is for key expansion routines which return an error code when the key
// size is wrong. This allows a higher-level library to be agnostic as to the proper
// key sizes for an algorithm and use the SymCrypt library to detect key size errors.
//
// For performance reasons this library avoids per-message error codes wherever possible.
//
// As this library can be used in many different contexts---kernel mode, user mode,
// WinCE, Xbox, etc.---we don't use one of the standard error types but use our own.
// Callers should not depend on the integer value of any of these enums.
//
// Error codes will signal the cause of the error, but callers should not rely on the
// exact symbolic error code returned. Especially in situations where multiple errors
// occur at once (e.g. multiple invalid parameters) the exact error symbol returned
// could change between versions of the library.
//

#ifndef _Return_type_success_
#define _Return_type_success_(expr)
#endif

typedef _Return_type_success_( return == SYMCRYPT_NO_ERROR ) enum {
    SYMCRYPT_NO_ERROR = 0,
    SYMCRYPT_UNUSED = 0x8000, // Start our error codes here so they're easier to distinguish
    SYMCRYPT_WRONG_KEY_SIZE,
    SYMCRYPT_WRONG_BLOCK_SIZE,
    SYMCRYPT_WRONG_DATA_SIZE,
    SYMCRYPT_WRONG_NONCE_SIZE,
    SYMCRYPT_WRONG_TAG_SIZE,
    SYMCRYPT_WRONG_ITERATION_COUNT,
    SYMCRYPT_AUTHENTICATION_FAILURE,
    SYMCRYPT_EXTERNAL_FAILURE,
    SYMCRYPT_FIPS_FAILURE,
    SYMCRYPT_HARDWARE_FAILURE,
    SYMCRYPT_NOT_IMPLEMENTED,
    SYMCRYPT_INVALID_BLOB,
    SYMCRYPT_BUFFER_TOO_SMALL,
    SYMCRYPT_INVALID_ARGUMENT,
    SYMCRYPT_MEMORY_ALLOCATION_FAILURE,
    SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE,
    SYMCRYPT_INCOMPATIBLE_FORMAT,
    SYMCRYPT_VALUE_TOO_LARGE,
    SYMCRYPT_SESSION_REPLAY_FAILURE,
    SYMCRYPT_HBS_NO_OTS_KEYS_LEFT,
    SYMCRYPT_HBS_PUBLIC_ROOT_MISMATCH,
} SYMCRYPT_ERROR;

// SYMCRYPT_ECURVE_TYPE needs to be completely defined before including
// symcrypt_internal.h because it's a member of another type in there.
typedef enum _SYMCRYPT_ECURVE_TYPE {
    SYMCRYPT_ECURVE_TYPE_NULL               = 0,
    SYMCRYPT_ECURVE_TYPE_SHORT_WEIERSTRASS  = 1,
    SYMCRYPT_ECURVE_TYPE_TWISTED_EDWARDS    = 2,
    SYMCRYPT_ECURVE_TYPE_MONTGOMERY         = 3,
} SYMCRYPT_ECURVE_TYPE;
//
// SYMCRYPT_ECURVE_TYPE is used to specify the type of the curve.
//

// SYMCRYPT_DLGROUP_FIPS needs to be completely defined before including
// symcrypt_internal.h because it's a member of another type in there.

//=====================================================
// DL group operations

typedef enum _SYMCRYPT_DLGROUP_FIPS {
    SYMCRYPT_DLGROUP_FIPS_NONE  = 0,
    SYMCRYPT_DLGROUP_FIPS_186_2 = 1,
    SYMCRYPT_DLGROUP_FIPS_186_3 = 2,
} SYMCRYPT_DLGROUP_FIPS;
//
// Dlgroup enums for the generation and verification of the group parameters.
// These are used in:
//  - SymCryptDlgroupGenerate function to specify the appropriate standard to
//    be used.
//  - SymCryptDlgroupSetValue function to verify that the input parameters were
//    properly generated.
//

typedef enum _SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE {
    SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_NONE = 0,
    SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_IKE_3526 = 1,
    SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_TLS_7919 = 2,
} SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE;
#define SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_DEFAULT   SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_TLS_7919
//
// Dlgroup enums for the specification and verification of the named safe prime group parameters.
// These are used in:
//  - SymCryptDlgroupGenerateSafePrime function to specify the appropriate group to
//    be used.
//

//
// The symcrypt_internal.h file contains information only relevant to the internals
// of the library, but they have to be exposed to the compiler of the caller.
// We put those in a separate file to make this file easier to read
// for users of the library.
// The details in the symcrypt_internal.h file can change at any time;
// users should only rely on the information in this header file.
//
#include "symcrypt_internal.h"

//
// Useful macros
//
// A variety of useful macros.
//
// The load/store macros convert from integer types to an array of bytes and vice versa.
// LOAD<n>_* (p) loads a value of <n> bits from the byte pointer p.
// STORE<n>_* (p,v) stores the n-bit value v to byte pointer p.
// The macros can either do Most Significant Byte first (big-endian) or
// Least Significant Byte first.
// The actual definitions are in the symcrypt_internal.h file because they contain
// items that are not part of the stable public API of SymCrypt.
//

#define SYMCRYPT_LOAD_LSBFIRST16( p )   SYMCRYPT_INTERNAL_LOAD_LSBFIRST16( p )
#define SYMCRYPT_LOAD_LSBFIRST32( p )   SYMCRYPT_INTERNAL_LOAD_LSBFIRST32( p )
#define SYMCRYPT_LOAD_LSBFIRST64( p )   SYMCRYPT_INTERNAL_LOAD_LSBFIRST64( p )

#define SYMCRYPT_LOAD_MSBFIRST16( p )   SYMCRYPT_INTERNAL_LOAD_MSBFIRST16( p )
#define SYMCRYPT_LOAD_MSBFIRST32( p )   SYMCRYPT_INTERNAL_LOAD_MSBFIRST32( p )
#define SYMCRYPT_LOAD_MSBFIRST64( p )   SYMCRYPT_INTERNAL_LOAD_MSBFIRST64( p )

#define SYMCRYPT_STORE_LSBFIRST16( p, v )   SYMCRYPT_INTERNAL_STORE_LSBFIRST16( p, v )
#define SYMCRYPT_STORE_LSBFIRST32( p, v )   SYMCRYPT_INTERNAL_STORE_LSBFIRST32( p, v )
#define SYMCRYPT_STORE_LSBFIRST64( p, v )   SYMCRYPT_INTERNAL_STORE_LSBFIRST64( p, v )

#define SYMCRYPT_STORE_MSBFIRST16( p, v )   SYMCRYPT_INTERNAL_STORE_MSBFIRST16( p, v )
#define SYMCRYPT_STORE_MSBFIRST32( p, v )   SYMCRYPT_INTERNAL_STORE_MSBFIRST32( p, v )
#define SYMCRYPT_STORE_MSBFIRST64( p, v )   SYMCRYPT_INTERNAL_STORE_MSBFIRST64( p, v )

//
// Convert between UINT32/UINT64 and variable-sized byte buffers
//
// The load functions take any size input array, and will return an error if the value
// encoded in the array exceeds the range of the target type (UINT32 or UINT64).
// The store functions will return an error if the destination buffer is too small
// to encode the actual value passed.
// An empty buffer (length = 0) encodes the value 0, and the value 0 can be encoded
// in the empty buffer.
// These functions are not side-channel safe.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadLsbFirstUint32(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT32 pDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadLsbFirstUint64(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT64 pDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadMsbFirstUint32(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT32 pDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLoadMsbFirstUint64(
    _In_reads_( cbSrc ) PCBYTE  pbSrc,
                        SIZE_T  cbSrc,
    _Out_               PUINT64 pDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreLsbFirstUint32(
                            UINT32  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreLsbFirstUint64(
                            UINT64  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreMsbFirstUint32(
                            UINT32  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptStoreMsbFirstUint64(
                            UINT64  src,
    _Out_writes_( cbDst )   PBYTE   pbDst,
                            SIZE_T  cbDst );

//
// Functions to retrieve the bitsize/bytesize of UINT32/UINT64 values
// Note: the bitsize/bytesize of the value 0 is defined as 0.
// Some data formats don't allow empty encodings, so the caller
// should ensure they handle the 0-case properly.
// These functions are NOT side-channel safe.
//
UINT32
SymCryptUint32Bitsize( UINT32 value );

UINT32
SymCryptUint64Bitsize( UINT64 value );

UINT32
SymCryptUint32Bytesize( UINT32 value );

UINT32
SymCryptUint64Bytesize( UINT64 value );


//
// FORCED MEMORY ACCESS
//
// These macros force a memory access. That is, they require that the memory
// read or write takes place, and do not allow the compiler to optimize the access
// away. This is useful for wiping memory even if the compiler knows the memory will not be used in future.
//
// The READ<n> macros read an n-bit value from a PBYTE and return a BYTE if n=8 and an UINT<n> otherwise.
// The WRITE<n> macros write a value to a PBYTE using the same types as the corresponding READ<n>
//
// These macros provide no other memory ordering requirements, so there are no acquire/release
// semantics, memory barriers, etc.
//

#define SYMCRYPT_FORCE_READ8(  _p )     SYMCRYPT_INTERNAL_FORCE_READ8(  _p )
#define SYMCRYPT_FORCE_READ16( _p )     SYMCRYPT_INTERNAL_FORCE_READ16( _p )
#define SYMCRYPT_FORCE_READ32( _p )     SYMCRYPT_INTERNAL_FORCE_READ32( _p )
#define SYMCRYPT_FORCE_READ64( _p )     SYMCRYPT_INTERNAL_FORCE_READ64( _p )

#define SYMCRYPT_FORCE_WRITE8(  _p, _v )    SYMCRYPT_INTERNAL_FORCE_WRITE8(  _p, _v )
#define SYMCRYPT_FORCE_WRITE16( _p, _v )    SYMCRYPT_INTERNAL_FORCE_WRITE16( _p, _v )
#define SYMCRYPT_FORCE_WRITE32( _p, _v )    SYMCRYPT_INTERNAL_FORCE_WRITE32( _p, _v )
#define SYMCRYPT_FORCE_WRITE64( _p, _v )    SYMCRYPT_INTERNAL_FORCE_WRITE64( _p, _v )

//==========================================================================
//   TYPE MODIFIERS
//==========================================================================
//
// The SymCrypt library uses the following type modifiers
//
// SYMCRYPT_CALL
//
//      The calling-convention used by SymCrypt functions.
//      Some platforms have multiple calling conventions which differ in the
//      way arguments are passed and the stack is handled
//      The SYMCRYPT_CALL type modifier selects the correct calling convention.
//      The current implementation uses __fastcall on the x86 platform, which
//      passes arguments in registers and is generally faster than the __stdcall
//      calling convention.
//
//
// SYMCRYPT_ALIGN
//
//      On platforms that support alignment declaration this macro expands to
//      __declspec(align(<n>)) where <n> is platform-dependent.
//      Many data types that SymCrypt defines are SYMCRYPT_ALIGNed.
//      When allocating memory for any SymCrypt data type the caller
//      has to ensure that the memory is aligned to the natural alignment for
//      that platform. (e.g. 4 for x86, 16 for x64)
//      Memory allocation functions typically return properly aligned memory blocks.
//      The macro SYMCRYPT_ALIGN_VALUE contains the actual value of <n>.
//

//==========================================================================
// LIBRARY MANAGEMENT
//==========================================================================
//
// SymCrypt runs in many different environments. Boot library, kernel, user mode,
// (for each of x86, amd64, arm), and possibly WinCE, Mobile, Zune, Xbox, etc.
// These different environments can have different requirements.
//
// Creating different libraries for each environment has huge testing and maintenance
// costs. Instead, the user of the library invokes a pre-defined macro in their own code
// that contains the necessary adoptions to that environment.
// Using a macro makes the selection static, which allows the compiler to optimize
// away a lot of the overhead.
// (e.g. if XMM register saving is not needed, the stub function declared by the macro
// will always succeed, and the compiler will inline it and optimize it away.)
//
// Warning: due to recent changes in the Visual Studio C runtime, we cannot test saving
// of the YMM registers in Windows user mode. Because we do not have a kernel mode test
// for saving/restoring the YMM registers, this functionality is currently not tested.
// Before using SymCrypt in Windows 7 kernel mode, additional kernel mode tests should be
// added to verify this functionality.
//

//
// The following environment macros are available. Callers should invoke one of these
// in their own code.
//
// SYMCRYPT_ENVIRONMENT_WINDOWS_BOOTLIBRARY                 // only for the current OS release
//
// SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_LEGACY           // Use for any version of Windows.
// SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN7_N_LATER     // Only for Win7 and later
// SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN8_1_N_LATER   // Only for WinBlue and later
// SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_LATEST           // use for latest OS
//
// SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_LEGACY             // use for any version of Windows
// SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN7_N_LATER       // Only for Win7 and later (cannot use AVX2 instructions)
// SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN8_1_N_LATER     // Only for Win8.1 and later
// SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_LATEST             // use for latest OS
//
// SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELDEBUGGER
//
// SYMCRYPT_ENVIRONMENT_LINUX_USERMODE                      // use for Linux
//
// SYMCRYPT_ENVIRONMENT_OPTEE_TA                            // use for OPTEE
//
// SYMCRYPT_ENVIRONMENT_GENERIC                             // use for all other situations
//

VOID
SYMCRYPT_CALL
SymCryptInit(void);
//
// Initialize the static library.
// This function MUST be called before any other function in the library.
// It is not necessary to call this function when using the shared object library.
//
// This function does not perform the self tests in the library.
// Doing so would force the linking of all the algorithm in the library,
// which is obviously not desirable for applications that want to link in
// only one or two algorithms.
// If self test are required (e.g. for FIPS certification) they have to be
// called separately for each algorithm.
//
// It is safe to call this function multiple times.
// The library initialization is done in the first call; subsequent calls are no-ops.
//
// If you get an 'undefined symbol' error on this function name, then you forgot
// to invoke one of the environment macros documented above.
//

VOID
SYMCRYPT_CALL
SymCryptModuleInit(
    _In_ UINT32 api,
    _In_ UINT32 minor);

#define SYMCRYPT_MODULE_INIT() SymCryptModuleInit( SYMCRYPT_CODE_VERSION_API, SYMCRYPT_CODE_VERSION_MINOR );
//
// Initialize the SymCrypt shared object module/dynamic-link library. This function verifies
// that the module version supports the version requested by the application. If the version
// is unsupported, a fatal error will occur. Rather than explicitly calling SymCryptModuleInit,
// the macro SYMCRYPT_MODULE_INIT should be used to call it with the correct arguments.
//

//==========================================================================
//   DATA MANIPULATION
//==========================================================================
//
// This library provides some data manipulation functions that commonly occur
// in cryptographic code.
//

VOID
SYMCRYPT_CALL
SymCryptWipe(
    _Out_writes_bytes_( cbData )    PVOID   pbData,
                                    SIZE_T  cbData );

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptWipeKnownSize(
    _Out_writes_bytes_( cbData )    PVOID   pbData,
                                    SIZE_T  cbData );

//
// The SymCryptWipe and SymCryptWipeKnownSize functions wipe memory.
// They work for any size and any alignment.
// Wiping is faster on x86 and x64 if the data buffer is 16-aligned,
// and the size is a multiple of 16.
//
// The SymCryptWipe function is optimized for the case where the size of the buffer
// is not known at compile time.
//
// The SymCryptWipeKnownSize function is optimized for the case where the
// cbData parameter is a compile-time known value.
//
// The two functions are functionally equivalent, but there can be a significant performance
// differences:
//  - calling SymCryptWipeKnownSize when the size is not known at compile time incurs a
//      code size penalty.
//  - calling SymCryptWipeKnownSize when the size is not known at compile time and is sometimes <= 64
//      incurs a performance penalty.
//      (The code assumes that the compiler can optimize all the conditional jumps away.
//      Conditional jumps can be very expensive if they are not predicted correctly.)
//  - calling SymCryptWipe when the buffer is small and has a compile-time known size incurs
//      a performance penalty.
// When in doubt, use SymCryptWipe.
//

VOID
SYMCRYPT_CALL
SymCryptXorBytes(
    _In_reads_( cbBytes )   PCBYTE  pbSrc1,
    _In_reads_( cbBytes )   PCBYTE  pbSrc2,
    _Out_writes_( cbBytes ) PBYTE   pbResult,
                            SIZE_T  cbBytes );
//
// Xor two strings of bytes together.
//
// The result buffer can be the same as Src1 or Src2, or can be non-overlapping
// with the inputs. However, the result buffer may not partially overlap with
// one of the inputs.
//

BOOLEAN
SYMCRYPT_CALL
SymCryptEqual(
    _In_reads_( cbBytes )   PCBYTE pbSrc1,
    _In_reads_( cbBytes )   PCBYTE pbSrc2,
                            SIZE_T cbBytes );
//
// Compare two regions of memory and return TRUE if they are equal, FALSE otherwise.
//
// This function compares all the bytes without an early-out mechanism.
// An early-out implementation, such as memcmp, reveals through side channels
// the position of the first byte where the inputs differ, which leaks information.
//


//==========================================================================
//   HASH FUNCTIONS
//==========================================================================
//
// All hash functions have a similar interface. For consistency we describe
// the generic parts of the interface once.
// Algorithm-specific comments are given with the API functions of each algorithm separately.
//
// For an algorithm called XXX the following functions, types, and constants are defined:
//
//
// SYMCRYPT_XXX_RESULT_SIZE
//
//      A constant giving the size, in bytes, of the result of the hash function.
//
//
// SYMCRYPT_XXX_INPUT_BLOCK_SIZE
//
//      A constant giving the natural input block size for the hash function.
//      Most callers don't need to know this, but some uses, like the HMAC construction
//      adapt to this size to improve efficiency.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxx( _In_reads_( cbData )                        PCBYTE pbData,
//                                                          SIZE_T cbData,
//              _Out_writes_( SYMCRYPT_XXX_RESULT_SIZE )    PBYTE pbResult );
//
//      Computes the hash value of the data buffer.
//      If you have all the data to be hashed in a single buffer this is the simplest function to use.
//
//
// SYMCRYPT_XXX_STATE
//
//      Type to store the intermediate state of a hash computation.
//      This is an opaque type whose structure can change at will.
//      It should only be used for transient computations in a single executable
//      and not be stored  or transferred to a different process.
//      The pointer version is also defined (PSYMCRYPT_XXX_STATE)
//
//      The SYMCRYPT_XXX_STATE structure contains the entire state of an ongoing
//      hash computation. If you want to compute the hash on several strings that
//      have the same prefix, the caller may hash the prefix first, then create
//      multiple copies using the supplied state copy function,
//      and continue hashing the different states with different postfix strings.
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxInit( _Out_ PSYMCRYPT_XXX_STATE pState );
//
//      Initialize a SYMCRYPT_XXX_STATE for subsequent use.
//
//      The state encodes an ongoing hash computation and allows incremental
//      computation of a hash function.
//      At any point in time the state object encodes a state that is equivalent to
//      the hash computation of a data string.
//      This function can be called at any time and resets the state to correspond
//      to the empty data string.
//      The SymCryptXxxAppend function appends data to the data string
//      encoded by the state.
//      The SymCryptXxxResult function finalizes the computation and
//      returns the actual hash result.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxAppend( _Inout_               PSYMCRYPT_XXX_STATE pState,
//                    _In_reads_( cbData )  PCBYTE              pbData,
//                                          SIZE_T              cbData );
//
//      Provide more data to the ongoing hash computation specified by the state.
//      The state must have been initialized by SymCryptXxxInit.
//      This function can be called multiple times on the same state
//      to append more data to the encoded data string.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxResult(
//      _Inout_                                PSYMCRYPT_XXX_STATE pState,
//     _Out_writes_( SYMCRYPT_XXX_RESULT_SIZE )PBYTE               pbResult );
//
//      Returns the hash of the data string encoded by the state.
//      If the state was newly initialized this returns the hash of the empty string.
//      If one or more SymCryptXxxAppend function calls were made on this state
//      it returns the hash of the concatenation of all the data strings
//      passed to SymCryptXxxAppend.
//
//      The state is re-initialized and ready for re-use; you do not have to call
//      SymCryptXxxInit on the state to start another fresh hash computation.
//      The state is also wiped of any traces of old data to prevent accidental data leakage.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxStateCopy( _In_ PCSYMCRYPT_XXX_STATE pSrc, _Out_ PSYMCRYPT_XXX_STATE pDst );
//
//      Create a new copy of the state object.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxStateExport(
//      _In_                                                    PCSYMCRYPT_XXX_STATE    pState,
//      _Out_writes_bytes_( SYMCRYPT_XXX_STATE_EXPORT_SIZE )    PBYTE                   pbBlob );
//
//      Converts a hash state to an exported format that can be persisted and re-imported.
//      The exported blob is compatible across CPU architectures, and across different
//      versions of SymCrypt.
//
//      pState must point to a valid initialized hash state.
//
//
// SYMCRYPT_ERROR
// SYMCRYPT_CALL
// SymCryptXxxStateImport(
//      _Out_                                               PSYMCRYPT_XXX_STATE pState,
//      _In_reads_bytes_( SYMCRYPT_XXX_STATE_EXPORT_SIZE)   PCBYTE              pbBlob );
//
//      Imports a hash state that was previously exported with SymCryptXxxStateExport.
//      After this call, the effective state of *pState is identical to the effective
//      state of *pState that was passed to the SymCryptXxxStateExport function which
//      created this blob.
//
//      This function returns an error if the blob is incorrectly formatted.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxSelftest(void);
//
//      Perform a minimal self-test on the XXX algorithm.
//      This function is designed to be used for achieving FIPS 140-2 compliance or
//      to provide a simple self-test when an application starts.
//
//      If an error is detected, a platform-specific fatal error action is taken.
//      Callers do not need to handle any error conditions.
//
//
//
//
// There are also generic Hash functions that use a virtual table and work
// for any hash algorithm.
// Virtual table addresses that callers can use are supplied through a const-ptr-const definition.
// This supports an application switching the underlying implementation of one algorithm
// without the need to re-compile all the intermediate libraries in between.
// For example, you could use the same signature verification library with the fast hash implementation in one binary,
// and with a compact hash implementation in a second binary, without needing a different
// signature verification library.
//

typedef enum _SYMCRYPT_HASH_ID
{
    SYMCRYPT_HASH_ID_NULL                = 0,
    SYMCRYPT_HASH_ID_MD2                 = 1,
    SYMCRYPT_HASH_ID_MD4                 = 2,
    SYMCRYPT_HASH_ID_MD5                 = 3,
    SYMCRYPT_HASH_ID_SHA1                = 4,
    SYMCRYPT_HASH_ID_SHA224              = 5,
    SYMCRYPT_HASH_ID_SHA256              = 6,
    SYMCRYPT_HASH_ID_SHA384              = 7,
    SYMCRYPT_HASH_ID_SHA512              = 8,
    SYMCRYPT_HASH_ID_SHA512_224          = 9,
    SYMCRYPT_HASH_ID_SHA512_256          = 10,
    SYMCRYPT_HASH_ID_SHA3_224            = 11,
    SYMCRYPT_HASH_ID_SHA3_256            = 12,
    SYMCRYPT_HASH_ID_SHA3_384            = 13,
    SYMCRYPT_HASH_ID_SHA3_512            = 14,
    SYMCRYPT_HASH_ID_SHAKE128            = 15,
    SYMCRYPT_HASH_ID_SHAKE256            = 16
} SYMCRYPT_HASH_ID;

PCSYMCRYPT_HASH
SYMCRYPT_CALL
SymCryptGetHashAlgorithm( SYMCRYPT_HASH_ID hashId );
//
// Returns a pointer to the hash algorithm structure for the specified hash ID.
// Returns NULL if the hash ID is invalid.
//

SIZE_T
SYMCRYPT_CALL
SymCryptHashResultSize( _In_ PCSYMCRYPT_HASH pHash );

SIZE_T
SYMCRYPT_CALL
SymCryptHashInputBlockSize( _In_ PCSYMCRYPT_HASH pHash );

SIZE_T
SYMCRYPT_CALL
SymCryptHashStateSize( _In_ PCSYMCRYPT_HASH pHash );
//
// SymCryptHashStateSize
//
// Returns the size, in bytes, of the hash state for this hash algorithm.
// Note that the state must be SYMCRYPT_ALIGNed.
// Alternatively, the SYMCRYPT_HASH_STATE structure is large enough to contain
// any Symcrypt-implemented hash state, so sizeof( SYMCRYPT_HASH_STATE ) is always
// large enough to contain a hash state.
//

VOID
SYMCRYPT_CALL
SymCryptHash(
    _In_                                                         PCSYMCRYPT_HASH pHash,
    _In_reads_( cbData )                                         PCBYTE          pbData,
                                                                 SIZE_T          cbData,
    _Out_writes_( SYMCRYPT_MIN( cbResult, pHash->resultSize ) )  PBYTE           pbResult,
                                                                 SIZE_T          cbResult );
//
// SymCryptHash
//
// Compute a hash value using any hash function.
// The number of bytes written to the pbResult buffer is
//      min( cbResult, SymCryptHashResultSize( pHash ) )
//

VOID
SYMCRYPT_CALL
SymCryptHashInit(
    _In_                                        PCSYMCRYPT_HASH pHash,
    _Out_writes_bytes_( pHash->stateSize )      PVOID           pState );

VOID
SYMCRYPT_CALL
SymCryptHashAppend(
    _In_                                        PCSYMCRYPT_HASH pHash,
    _Inout_updates_bytes_( pHash->stateSize )   PVOID           pState,
    _In_reads_( cbData )                        PCBYTE          pbData,
                                                SIZE_T          cbData );

VOID
SYMCRYPT_CALL
SymCryptHashResult(
    _In_                                                         PCSYMCRYPT_HASH pHash,
    _Inout_updates_bytes_( pHash->stateSize )                    PVOID           pState,
    _Out_writes_( SYMCRYPT_MIN( cbResult, pHash->resultSize ) )  PBYTE           pbResult,
                                                                 SIZE_T          cbResult );
//
// SymCryptHashResult
//
// Finalizes the hash computation by calling the resultFunc member
// of pHash.
// The hash result is produced to an internal buffer and
// the number of bytes written to the pbResult buffer is
//      min( cbResult, SymCryptHashResultSize( pHash ) )

VOID
SYMCRYPT_CALL
SymCryptHashStateCopy(
    _In_                            PCSYMCRYPT_HASH pHash,
    _In_reads_(pHash->stateSize)    PCVOID          pSrc,
    _Out_writes_(pHash->stateSize)  PVOID           pDst);
//
// SymCryptHashStateCopy
//
// Copies the hash state from pSrc to pDst.

////////////////////////////////////////////////////////////////////////////
//   MD2
//
// Tha MD2 hash algorithm per RFC1319.
//
// The MD2 hash function has not received widespread analysis and is very slow
// compared to contemporary algorithms.
//
// The SymCrypt implementation of MD2 uses table lookups which leads to a side-channel
// vulnerability.
//
// Per the Crypto SDL, any use of this algorithm in Microsoft code requires
// a Crypto board exemption. Whenever possible, please use SHA-256 or SHA-512.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_MD2_RESULT_SIZE         (16)
#define SYMCRYPT_MD2_INPUT_BLOCK_SIZE    (16)

VOID
SYMCRYPT_CALL
SymCryptMd2(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_MD2_RESULT_SIZE )    PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd2Init( _Out_ PSYMCRYPT_MD2_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptMd2Append(
    _Inout_                 PSYMCRYPT_MD2_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbData,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptMd2Result(
    _Inout_                                  PSYMCRYPT_MD2_STATE pState,
    _Out_writes_( SYMCRYPT_MD2_RESULT_SIZE ) PBYTE               pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd2StateCopy( _In_ PCSYMCRYPT_MD2_STATE pSrc, _Out_ PSYMCRYPT_MD2_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptMd2StateExport(
    _In_                                                    PCSYMCRYPT_MD2_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD2_STATE_EXPORT_SIZE )    PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMd2StateImport(
    _Out_                                               PSYMCRYPT_MD2_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD2_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptMd2Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptMd2Algorithm;

////////////////////////////////////////////////////////////////////////////
//   MD4
//
// Tha MD4 hash algorithm per RFC1320.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The MD4 hash function has been badly broken and is not considered secure.
// Per the Crypto SDL, any use of this algorithm in Microsoft code requires
// a Crypto board exemption. Whenever possible, please use SHA-256 or SHA-512.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_MD4_RESULT_SIZE         (16)
#define SYMCRYPT_MD4_INPUT_BLOCK_SIZE    (64)

VOID
SYMCRYPT_CALL
SymCryptMd4(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_MD4_RESULT_SIZE )    PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd4Init( _Out_ PSYMCRYPT_MD4_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptMd4Append(
    _Inout_                 PSYMCRYPT_MD4_STATE  pState,
    _In_reads_( cbData )    PCBYTE               pbData,
                            SIZE_T               cbData );

VOID
SYMCRYPT_CALL
SymCryptMd4Result(
    _Inout_                                  PSYMCRYPT_MD4_STATE  pState,
    _Out_writes_( SYMCRYPT_MD4_RESULT_SIZE ) PBYTE                pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd4StateCopy( _In_ PCSYMCRYPT_MD4_STATE pSrc, _Out_ PSYMCRYPT_MD4_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptMd4StateExport(
    _In_                                                    PCSYMCRYPT_MD4_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD4_STATE_EXPORT_SIZE )    PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMd4StateImport(
    _Out_                                               PSYMCRYPT_MD4_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD4_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptMd4Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptMd4Algorithm;

////////////////////////////////////////////////////////////////////////////
//   MD5
//
// Tha MD5 hash algorithm per RFC1321.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The MD5 hash function has been badly broken and is not considered secure.
// Per the Crypto SDL, any use of this algorithm in Microsoft code requires
// a Crypto board exemption. Whenever possible, please use SHA-256 or SHA-512.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_MD5_RESULT_SIZE        (16)
#define SYMCRYPT_MD5_INPUT_BLOCK_SIZE   (64)

VOID
SYMCRYPT_CALL
SymCryptMd5(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_MD5_RESULT_SIZE )    PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd5Init( _Out_ PSYMCRYPT_MD5_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptMd5Append(
    _Inout_                 PSYMCRYPT_MD5_STATE   pState,
    _In_reads_( cbData )    PCBYTE                pbData,
                            SIZE_T                cbData );

VOID
SYMCRYPT_CALL
SymCryptMd5Result(
    _Inout_                                  PSYMCRYPT_MD5_STATE  pState,
    _Out_writes_( SYMCRYPT_MD5_RESULT_SIZE ) PBYTE                pbResult );

VOID
SYMCRYPT_CALL
SymCryptMd5StateCopy( _In_ PCSYMCRYPT_MD5_STATE pSrc, _Out_ PSYMCRYPT_MD5_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptMd5StateExport(
    _In_                                                    PCSYMCRYPT_MD5_STATE    pState,
    _Out_writes_bytes_( SYMCRYPT_MD5_STATE_EXPORT_SIZE )    PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMd5StateImport(
    _Out_                                               PSYMCRYPT_MD5_STATE     pState,
    _In_reads_bytes_( SYMCRYPT_MD5_STATE_EXPORT_SIZE)   PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptMd5Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptMd5Algorithm;


///////////////////////////////////////////////////////////////////////////////
//      SHA-1
//
// The SHA-1 hash algorithm per FIPS 180-4.
//
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-1 standard limits data inputs to a maximum of 2^61-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// The SHA-1 hash algorithm has been broken in a technical sense, and future
// attacks can only get better.
// This algorithm is not recommended for new applications and should only be used
// for backward compatibility.
// Per the Crypto SDL, new uses of this algorithm in Microsoft code require
// a Crypto board exemption. Whenever possible, please use SHA-256 or SHA-512.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA1_RESULT_SIZE       (20)
#define SYMCRYPT_SHA1_INPUT_BLOCK_SIZE  (64)

VOID
SYMCRYPT_CALL
SymCryptSha1(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA1_RESULT_SIZE )   PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha1Init( _Out_ PSYMCRYPT_SHA1_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha1Append(
    _Inout_                 PSYMCRYPT_SHA1_STATE    pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptSha1Result(
    _Inout_                                  PSYMCRYPT_SHA1_STATE pState,
    _Out_writes_( SYMCRYPT_SHA1_RESULT_SIZE )PBYTE                pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha1StateCopy( _In_ PCSYMCRYPT_SHA1_STATE pSrc, _Out_ PSYMCRYPT_SHA1_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha1StateExport(
    _In_                                                    PCSYMCRYPT_SHA1_STATE   pState,
    _Out_writes_bytes_( SYMCRYPT_SHA1_STATE_EXPORT_SIZE )   PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha1StateImport(
    _Out_                                               PSYMCRYPT_SHA1_STATE    pState,
    _In_reads_bytes_( SYMCRYPT_SHA1_STATE_EXPORT_SIZE)  PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha1Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha1Algorithm;

////////////////////////////////////////////////////////////////////////////
//   SHA-224
//
//
// The SHA-224 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-224 standard limits data inputs to a maximum of 2^61-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// This implementation is meant for interoperability and is not recommended for use.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA224_RESULT_SIZE         (28)
#define SYMCRYPT_SHA224_INPUT_BLOCK_SIZE    (64)

VOID
SYMCRYPT_CALL
SymCryptSha224(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA224_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha224Init( _Out_ PSYMCRYPT_SHA224_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha224Append(
    _Inout_                 PSYMCRYPT_SHA224_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptSha224Result(
    _Inout_                                       PSYMCRYPT_SHA224_STATE pState,
    _Out_writes_( SYMCRYPT_SHA224_RESULT_SIZE )   PBYTE                  pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha224StateCopy( _In_ PCSYMCRYPT_SHA224_STATE pSrc, _Out_ PSYMCRYPT_SHA224_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha224StateExport(
    _In_                                                    PCSYMCRYPT_SHA224_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA224_STATE_EXPORT_SIZE ) PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha224StateImport(
    _Out_                                                   PSYMCRYPT_SHA224_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA224_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha224Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha224Algorithm;

////////////////////////////////////////////////////////////////////////////
//   SHA-256
//
//
// The SHA-256 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-256 standard limits data inputs to a maximum of 2^61-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA256_RESULT_SIZE         (32)
#define SYMCRYPT_SHA256_INPUT_BLOCK_SIZE    (64)

VOID
SYMCRYPT_CALL
SymCryptSha256(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA256_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha256Init( _Out_ PSYMCRYPT_SHA256_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha256Append(
    _Inout_                 PSYMCRYPT_SHA256_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptSha256Result(
    _Inout_                                       PSYMCRYPT_SHA256_STATE pState,
    _Out_writes_( SYMCRYPT_SHA256_RESULT_SIZE )   PBYTE                  pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha256StateCopy( _In_ PCSYMCRYPT_SHA256_STATE pSrc, _Out_ PSYMCRYPT_SHA256_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha256StateExport(
    _In_                                                    PCSYMCRYPT_SHA256_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE ) PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha256StateImport(
    _Out_                                                   PSYMCRYPT_SHA256_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA256_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha256Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha256Algorithm;

////////////////////////////////////////////////////////////////////////////
//   SHA-384
//
//
// The SHA-384 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-384 standard limits data inputs to a maximum of 2^125-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA384_RESULT_SIZE         (48)
#define SYMCRYPT_SHA384_INPUT_BLOCK_SIZE    (128)

VOID
SYMCRYPT_CALL
SymCryptSha384(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA384_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha384Init( _Out_ PSYMCRYPT_SHA384_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha384Append(
    _Inout_                 PSYMCRYPT_SHA384_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptSha384Result(
    _Inout_                                     PSYMCRYPT_SHA384_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA384_RESULT_SIZE ) PBYTE                  pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha384StateCopy( _In_ PCSYMCRYPT_SHA384_STATE pSrc, _Out_ PSYMCRYPT_SHA384_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha384StateExport(
    _In_                                                    PCSYMCRYPT_SHA384_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA384_STATE_EXPORT_SIZE ) PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha384StateImport(
    _Out_                                                   PSYMCRYPT_SHA384_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA384_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha384Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha384Algorithm;

////////////////////////////////////////////////////////////////////////////
//   SHA-512
//
//
// The SHA-512 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-512 standard limits data inputs to a maximum of 2^125-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA512_RESULT_SIZE         (64)
#define SYMCRYPT_SHA512_INPUT_BLOCK_SIZE    (128)

VOID
SYMCRYPT_CALL
SymCryptSha512(
    _In_reads_( cbData )                        PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512Init( _Out_ PSYMCRYPT_SHA512_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha512Append(
    _Inout_                 PSYMCRYPT_SHA512_STATE  pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptSha512Result(
    _Inout_                                     PSYMCRYPT_SHA512_STATE pState,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE                  pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512StateCopy( _In_ PCSYMCRYPT_SHA512_STATE pSrc, _Out_ PSYMCRYPT_SHA512_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha512StateExport(
    _In_                                                    PCSYMCRYPT_SHA512_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE ) PBYTE                   pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512StateImport(
    _Out_                                                   PSYMCRYPT_SHA512_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_STATE_EXPORT_SIZE)    PCBYTE                  pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha512Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha512Algorithm;


////////////////////////////////////////////////////////////////////////////
//   SHA-512/224
//
//
// The SHA-512/224 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-512/224 standard limits data inputs to a maximum of 2^125-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// This implementation is meant for interoperability and is not recommended for use.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA512_224_RESULT_SIZE         (28)
#define SYMCRYPT_SHA512_224_INPUT_BLOCK_SIZE    (128)

VOID
SYMCRYPT_CALL
SymCryptSha512_224(
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA512_224_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512_224Init( _Out_ PSYMCRYPT_SHA512_224_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha512_224Append(
    _Inout_                 PSYMCRYPT_SHA512_224_STATE  pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptSha512_224Result(
    _Inout_                                         PSYMCRYPT_SHA512_224_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA512_224_RESULT_SIZE ) PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512_224StateCopy( _In_ PCSYMCRYPT_SHA512_224_STATE pSrc, _Out_ PSYMCRYPT_SHA512_224_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha512_224StateExport(
    _In_                                                        PCSYMCRYPT_SHA512_224_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_224_STATE_EXPORT_SIZE ) PBYTE                       pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512_224StateImport(
    _Out_                                                       PSYMCRYPT_SHA512_224_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_224_STATE_EXPORT_SIZE)    PCBYTE                      pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha512_224Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha512_224Algorithm;


////////////////////////////////////////////////////////////////////////////
//   SHA-512/256
//
//
// The SHA-512/256 hash algorithm per FIPS 180-4.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// The SHA-512/256 standard limits data inputs to a maximum of 2^125-1 bytes.
// This implementation supports larger inputs, and simply wraps the internal message
// length counter. Note that the security properties are unknown for
// such long messages, and their use is not recommended.
//
// This implementation is meant for interoperability and is not recommended for use.
//
// For details on this API see the description above about the generic hash function API.
//

#define SYMCRYPT_SHA512_256_RESULT_SIZE         (32)
#define SYMCRYPT_SHA512_256_INPUT_BLOCK_SIZE    (128)

VOID
SYMCRYPT_CALL
SymCryptSha512_256(
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA512_256_RESULT_SIZE ) PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512_256Init( _Out_ PSYMCRYPT_SHA512_256_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha512_256Append(
    _Inout_                 PSYMCRYPT_SHA512_256_STATE  pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptSha512_256Result(
    _Inout_                                         PSYMCRYPT_SHA512_256_STATE  pState,
    _Out_writes_( SYMCRYPT_SHA512_256_RESULT_SIZE ) PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha512_256StateCopy( _In_ PCSYMCRYPT_SHA512_256_STATE pSrc, _Out_ PSYMCRYPT_SHA512_256_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha512_256StateExport(
    _In_                                                        PCSYMCRYPT_SHA512_256_STATE pState,
    _Out_writes_bytes_( SYMCRYPT_SHA512_256_STATE_EXPORT_SIZE ) PBYTE                       pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha512_256StateImport(
    _Out_                                                       PSYMCRYPT_SHA512_256_STATE  pState,
    _In_reads_bytes_( SYMCRYPT_SHA512_256_STATE_EXPORT_SIZE)    PCBYTE                      pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha512_256Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha512_256Algorithm;


////////////////////////////////////////////////////////////////////////////
//   SHA-3
//
// The SHA-3 family of hash algorithms per FIPS 202.
// This implementation is limited to data strings that are in whole bytes.
// Odd bit length are not supported.
//
// SHA3-224 is meant for interoperability and is not recommended for use.
//
// SHA3-224(M) = KECCAK[448](M || 01, 224)
// SHA3-256(M) = KECCAK[512](M || 01, 256)
// SHA3-384(M) = KECCAK[768](M || 01, 384)
// SHA3-512(M) = KECCAK[1024](M || 01, 512)
//
// For details on this API see the description above about the generic hash function API.
//


//
// SHA-3-224
//

#define SYMCRYPT_SHA3_224_RESULT_SIZE         (28)
#define SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE    (144)

VOID
SYMCRYPT_CALL
SymCryptSha3_224(
    _In_reads_(cbData)                          PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_(SYMCRYPT_SHA3_224_RESULT_SIZE) PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_224Init(_Out_ PSYMCRYPT_SHA3_224_STATE pState);

VOID
SYMCRYPT_CALL
SymCryptSha3_224Append(
    _Inout_                 PSYMCRYPT_SHA3_224_STATE    pState,
    _In_reads_(cbData)      PCBYTE                      pbData,
                            SIZE_T                      cbData);

VOID
SYMCRYPT_CALL
SymCryptSha3_224Result(
    _Inout_                                     PSYMCRYPT_SHA3_224_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_224_RESULT_SIZE) PBYTE                       pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_224StateCopy(_In_ PCSYMCRYPT_SHA3_224_STATE pSrc, _Out_ PSYMCRYPT_SHA3_224_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptSha3_224StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_224_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE) PBYTE                       pbBlob);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_224StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_224_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_224_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob);

VOID
SYMCRYPT_CALL
SymCryptSha3_224Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha3_224Algorithm;


//
// SHA-3-256
//

#define SYMCRYPT_SHA3_256_RESULT_SIZE         (32)
#define SYMCRYPT_SHA3_256_INPUT_BLOCK_SIZE    (136)

VOID
SYMCRYPT_CALL
SymCryptSha3_256(
    _In_reads_(cbData)                          PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_(SYMCRYPT_SHA3_256_RESULT_SIZE) PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_256Init(_Out_ PSYMCRYPT_SHA3_256_STATE pState);

VOID
SYMCRYPT_CALL
SymCryptSha3_256Append(
    _Inout_                 PSYMCRYPT_SHA3_256_STATE    pState,
    _In_reads_(cbData)      PCBYTE                      pbData,
                            SIZE_T                      cbData);

VOID
SYMCRYPT_CALL
SymCryptSha3_256Result(
    _Inout_                                     PSYMCRYPT_SHA3_256_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_256_RESULT_SIZE) PBYTE                       pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_256StateCopy(_In_ PCSYMCRYPT_SHA3_256_STATE pSrc, _Out_ PSYMCRYPT_SHA3_256_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptSha3_256StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_256_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE) PBYTE                       pbBlob);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_256StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_256_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_256_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob);

VOID
SYMCRYPT_CALL
SymCryptSha3_256Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha3_256Algorithm;


//
// SHA-3-384
//

#define SYMCRYPT_SHA3_384_RESULT_SIZE         (48)
#define SYMCRYPT_SHA3_384_INPUT_BLOCK_SIZE    (104)

VOID
SYMCRYPT_CALL
SymCryptSha3_384(
    _In_reads_(cbData)                          PCBYTE  pbData,
                                                SIZE_T  cbData,
    _Out_writes_(SYMCRYPT_SHA3_384_RESULT_SIZE) PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_384Init(_Out_ PSYMCRYPT_SHA3_384_STATE pState);

VOID
SYMCRYPT_CALL
SymCryptSha3_384Append(
    _Inout_                 PSYMCRYPT_SHA3_384_STATE    pState,
    _In_reads_(cbData)      PCBYTE                      pbData,
                            SIZE_T                      cbData);

VOID
SYMCRYPT_CALL
SymCryptSha3_384Result(
    _Inout_                                     PSYMCRYPT_SHA3_384_STATE    pState,
    _Out_writes_(SYMCRYPT_SHA3_384_RESULT_SIZE) PBYTE                       pbResult);

VOID
SYMCRYPT_CALL
SymCryptSha3_384StateCopy(_In_ PCSYMCRYPT_SHA3_384_STATE pSrc, _Out_ PSYMCRYPT_SHA3_384_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptSha3_384StateExport(
    _In_                                                    PCSYMCRYPT_SHA3_384_STATE   pState,
    _Out_writes_bytes_(SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE) PBYTE                       pbBlob);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_384StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_384_STATE    pState,
    _In_reads_bytes_(SYMCRYPT_SHA3_384_STATE_EXPORT_SIZE)   PCBYTE                      pbBlob);

VOID
SYMCRYPT_CALL
SymCryptSha3_384Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha3_384Algorithm;


//
// SHA-3-512
//

#define SYMCRYPT_SHA3_512_RESULT_SIZE         (64)
#define SYMCRYPT_SHA3_512_INPUT_BLOCK_SIZE    (72)

VOID
SYMCRYPT_CALL
SymCryptSha3_512(
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHA3_512_RESULT_SIZE )   PBYTE   pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha3_512Init( _Out_ PSYMCRYPT_SHA3_512_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptSha3_512Append(
    _Inout_                 PSYMCRYPT_SHA3_512_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptSha3_512Result(
    _Inout_                                         PSYMCRYPT_SHA3_512_STATE    pState,
    _Out_writes_( SYMCRYPT_SHA3_512_RESULT_SIZE )   PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptSha3_512StateCopy( _In_ PCSYMCRYPT_SHA3_512_STATE pSrc, _Out_ PSYMCRYPT_SHA3_512_STATE pDst );

VOID
SYMCRYPT_CALL
SymCryptSha3_512StateExport(
    _In_                                                        PCSYMCRYPT_SHA3_512_STATE   pState,
    _Out_writes_bytes_( SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE )   PBYTE                       pbBlob );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSha3_512StateImport(
    _Out_                                                   PSYMCRYPT_SHA3_512_STATE    pState,
    _In_reads_bytes_( SYMCRYPT_SHA3_512_STATE_EXPORT_SIZE)  PCBYTE                      pbBlob );

VOID
SYMCRYPT_CALL
SymCryptSha3_512Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptSha3_512Algorithm;


//==========================================================================
//   Extendable-Output Functions (XOFs)
//==========================================================================
//
//  XOFs are similar to hash functions except that the output can be arbitrary length.
//  SHAKE128 and SHAKE256 are XOFs specified in FIPS 202.
//
//  SHAKE128(M, d) = KECCAK[256] (M || 1111, d)
//  SHAKE256(M, d) = KECCAK[512] (M || 1111, d)
//
//  SHAKEs share the same Keccak state as the other Keccak based algorithms under
//  the name SYMCRYPT_SHAKEXxx_STATE.
//
//  Both SHAKE128 and SHAKE256 have default result sizes (32- and 64-bytes resp.)
//  that allows them to be used as substitutes for hash functions with the Init-Append-Result
//  pattern.
//
//  Extract is a new type of function that does not exist in hash functions, which can
//  be called multiple times to successively generate output from the state. Extract
//  function also provides the caller with a flag to wipe the state when no further Extract
//  calls will be made. If the caller does not know in advance whether an Extract call is
//  the final one, wiping can be performed later with an Init call or an Extract call with
//  zero bytes output.
//
//  If Append is called after an Extract call which did not wipe the state (i.e., the state
//  is still in 'extract' mode), Append will notice this and switch from 'extract' mode to
//  'append' mode by wiping and initializing the state. This Append call effectively appends
//  data for a fresh computation, saving an additional call to wipe/initialize the state.
//
//
//  SYMCRYPT_SHAKEXXX_RESULT_SIZE
//
//      Default output size, used by the SymCryptShakeXxxResult function.
//
//  SYMCRYPT_SHAKEXXX_INPUT_BLOCK_SIZE
//
//      Rate for the Keccak permutation.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxDefault(
//      _In_reads_( cbData )                            PCBYTE  pbData,
//                                                      SIZE_T  cbData,
//      _Out_writes_( SYMCRYPT_SHAKEXXX_RESULT_SIZE )   PBYTE   pbResult);
//
//      SHAKE single-call function that produces default output size defined by
//      SYMCRYPT_SHAKEXXX_RESULT_SIZE.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxx(
//      _In_reads_( cbData )        PCBYTE  pbData,
//                                  SIZE_T  cbData,
//      _Out_writes_( cbResult )    PBYTE   pbResult,
//                                  SIZE_T  cbResult);
//
//      SHAKE single-call function that produces variable-length output specified
//      by the cbResult parameter.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxInit( _Out_ PSYMCRYPT_XXX_STATE pState );
//
//      Initializes the SHAKE state.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxAppend(
//      _Inout_                 PSYMCRYPT_XXX_STATE pState,
//      _In_reads_( cbData )    PCBYTE              pbData,
//                              SIZE_T              cbData );
//
//      Appends data to the SHAKE state.
//
//      Append cannot be the first call to an uninitialized SHAKE state. All
//      other uses independent of whether the state is in 'append' mode or 'extract'
//      mode are well defined. If the state was previously in 'extract' mode, (i.e., after
//      an Extract call with bWipe=FALSE) it wipes/resets the state and the data is
//      appended to a fresh state.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxExtract(
//      _Inout_                 PSYMCRYPT_XXX_STATE pState,
//      _Out_writes_(cbResult)  PBYTE               pbResult,
//                              SIZE_T              cbResult,
//                              BOOLEAN             bWipe);
//
//      Generates output from the SHAKE state.
//
//      Extract cannot be the first call to an uninitialized SHAKE state. All
//      other uses independent of whether the state is in 'append' mode or 'extract' mode
//      are well defined.
//
//      If the state was in 'append' mode before the Extract call, Extract switches
//      the state to 'extract' mode and generates the requested number of bytes from
//      the state. Extract wipes/resets the state and transitions the state to 'append'
//      mode if bWipe=TRUE, otherwise leaving the state in 'extract' mode, available for
//      further extractions.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxResult(
//      _Inout_                                     PSYMCRYPT_XXX_STATE pState,
//      _Out_writes_(SYMCRYPT_SHAKEXXX_RESULT_SIZE) PBYTE               pbResult );
//
//      Extracts SYMCRYPT_SHAKEXXX_RESULT_SIZE bytes from the state and wipes/resets
//      it for a new computation.
//
//      Result cannot be called with an uninitialized state. All other uses are well
//      defined. If it is called after an Extract call with bWipe=FALSE, it does the
//      final extraction from the state for SYMCRYPT_SHAKEXXX_RESULT_SIZE bytes,
//      effectively calling Extract with cbResult=SYMCRYPT_SHAKEXXX_RESULT_SIZE and
//      bWipe=TRUE.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptShakeXxxStateCopy(_In_ PCSYMCRYPT_SHAKEXXX_STATE pSrc, _Out_ PSYMCRYPT_SHAKEXXX_STATE pDst);
//
//      Create a new copy of the state object.
//
// VOID
// SYMCRYPT_CALL
// SymCryptShakeXxxSelftest(void);
//
//      Perform a minimal self-test on the ShakeXxx algorithm.
//      This function is designed to be used for achieving FIPS 140-2 compliance or
//      to provide a simple self-test when an application starts.
//
//      If an error is detected, a platform-specific fatal error action is taken.
//      Callers do not need to handle any error conditions.


//
// SHAKE128
//
#define SYMCRYPT_SHAKE128_RESULT_SIZE           (32)
#define SYMCRYPT_SHAKE128_INPUT_BLOCK_SIZE      (168)

VOID
SYMCRYPT_CALL
SymCryptShake128Default(
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHAKE128_RESULT_SIZE )   PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptShake128(
    _In_reads_( cbData )        PCBYTE  pbData,
                                SIZE_T  cbData,
    _Out_writes_( cbResult )    PBYTE   pbResult,
                                SIZE_T  cbResult);

VOID
SYMCRYPT_CALL
SymCryptShake128Init( _Out_ PSYMCRYPT_SHAKE128_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptShake128Append(
    _Inout_                 PSYMCRYPT_SHAKE128_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptShake128Extract(
    _Inout_                 PSYMCRYPT_SHAKE128_STATE    pState,
    _Out_writes_(cbResult)  PBYTE                       pbResult,
                            SIZE_T                      cbResult,
                            BOOLEAN                     bWipe);

VOID
SYMCRYPT_CALL
SymCryptShake128Result(
    _Inout_                                     PSYMCRYPT_SHAKE128_STATE    pState,
    _Out_writes_(SYMCRYPT_SHAKE128_RESULT_SIZE) PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptShake128StateCopy(_In_ PCSYMCRYPT_SHAKE128_STATE pSrc, _Out_ PSYMCRYPT_SHAKE128_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptShake128Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptShake128HashAlgorithm;

//
// SHAKE256
//
#define SYMCRYPT_SHAKE256_RESULT_SIZE           (64)
#define SYMCRYPT_SHAKE256_INPUT_BLOCK_SIZE      (136)

VOID
SYMCRYPT_CALL
SymCryptShake256Default(
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_SHAKE256_RESULT_SIZE )   PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptShake256(
    _In_reads_( cbData )        PCBYTE  pbData,
                                SIZE_T  cbData,
    _Out_writes_( cbResult )    PBYTE   pbResult,
                                SIZE_T  cbResult);

VOID
SYMCRYPT_CALL
SymCryptShake256Init( _Out_ PSYMCRYPT_SHAKE256_STATE pState );

VOID
SYMCRYPT_CALL
SymCryptShake256Append(
    _Inout_                 PSYMCRYPT_SHAKE256_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptShake256Extract(
    _Inout_                 PSYMCRYPT_SHAKE256_STATE    pState,
    _Out_writes_(cbResult)  PBYTE                       pbResult,
                            SIZE_T                      cbResult,
                            BOOLEAN                     bWipe);

VOID
SYMCRYPT_CALL
SymCryptShake256Result(
    _Inout_                                     PSYMCRYPT_SHAKE256_STATE    pState,
    _Out_writes_(SYMCRYPT_SHAKE256_RESULT_SIZE) PBYTE                       pbResult );


VOID
SYMCRYPT_CALL
SymCryptShake256StateCopy(_In_ PCSYMCRYPT_SHAKE256_STATE pSrc, _Out_ PSYMCRYPT_SHAKE256_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptShake256Selftest(void);

extern const PCSYMCRYPT_HASH SymCryptShake256HashAlgorithm;

//==========================================================================
//   Customizable Extendable-Output Functions (XOFs)
//==========================================================================
//
//  cSHAKE128 and cSHAKE256 are customizable SHAKE functions specified in NIST SP 800-185.
//
//  When cSHAKE input strings N (function name string) and S (customization string) are
//  both empty, cSHAKE is equivalent to SHAKE:
//
//  cSHAKE128(X, L, "", "") = SHAKE128(X, L)
//  cSHAKE256(X, L, "", "") = SHAKE256(X, L)
//
//  If at least one of N and S is non-empty, cSHAKE is defined as follows:
//
//  cSHAKE128(X, L, N, S) = KECCAK[256](bytepad(encode_string(N) || encode_string(S), 168) || X || 00, L)
//  cSHAKE256(X, L, N, S) = KECCAK[512](bytepad(encode_string(N) || encode_string(S), 136) || X || 00, L)
//
//  The following functions are equivalent to their SHAKE counterparts.
//  SymCryptCShakeXxxExtract with bWipe=TRUE and SymCryptCShakeXxxResult functions reset
//  the cSHAKE state to an empty SHAKE state after generating output. This behavior is
//  equivalent to calling SymCryptCShakeXxxInit with empty input strings.
//
//      SymCryptCShakeXxxAppend
//      SymCryptCShakeXxxExtract
//      SymCryptCShakeXxxResult
//
//  Calling SymCryptCShakeXxxAppend when cSHAKE state is in 'extract' mode results
//  in the same behavior described above: the state is wiped and initialized with
//  empty input strings, after which the data is appended to the empty state. This
//  converts the state to a SHAKE state since cSHAKE with empty input strings is
//  equivalent to SHAKE. This is a consequence of not being able to store the input
//  strings to cSHAKE and re-initialize it with them. Thus, if multiple cSHAKE
//  computations with the same input strings are to be carried out, cSHAKE state must
//  be initialized with the input strings each time.
//
//  The following functions differ from the SHAKE by the introduction of customization
//  strings:
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptCShakeXxx(
//      _In_reads_( cbFunctionNameString )  PCBYTE  pbFunctionNameString,
//                                          SIZE_T  cbFunctionNameString,
//      _In_reads_( cbCustomizationString ) PCBYTE  pbCustomizationString,
//                                          SIZE_T  cbCustomizationString,
//      _In_reads_( cbData )                PCBYTE  pbData,
//                                          SIZE_T  cbData,
//      _Out_writes_( cbResult )            PBYTE   pbResult,
//                                          SIZE_T  cbResult);
//
//      Single-call cSHAKE computation.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptCShakeXxxInit(
//      _Out_                               PSYMCRYPT_CSHAKEXXX_STATE   pState,
//      _In_reads_( cbFunctionNameString )  PCBYTE                      pbFunctionNameString,
//                                          SIZE_T                      cbFunctionNameString,
//      _In_reads_( cbCustomizationString ) PCBYTE                      pbCustomizationString,
//                                          SIZE_T                      cbCustomizationString);
//
//      Initializes the cSHAKE state with the provided input strings. If both of
//      the input strings are empty, the call is equivalent to SymCryptShakeXxxInit,
//      otherwise the input strings will be encoded and appended to the state.


//
// cSHAKE128
//
#define SYMCRYPT_CSHAKE128_RESULT_SIZE           SYMCRYPT_SHAKE128_RESULT_SIZE
#define SYMCRYPT_CSHAKE128_INPUT_BLOCK_SIZE      SYMCRYPT_SHAKE128_INPUT_BLOCK_SIZE

VOID
SYMCRYPT_CALL
SymCryptCShake128(
    _In_reads_( cbFunctionNameString )  PCBYTE  pbFunctionNameString,
                                        SIZE_T  cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE  pbCustomizationString,
                                        SIZE_T  cbCustomizationString,
    _In_reads_( cbData )                PCBYTE  pbData,
                                        SIZE_T  cbData,
    _Out_writes_( cbResult )            PBYTE   pbResult,
                                        SIZE_T  cbResult);

VOID
SYMCRYPT_CALL
SymCryptCShake128Init(
    _Out_                               PSYMCRYPT_CSHAKE128_STATE   pState,
    _In_reads_( cbFunctionNameString )  PCBYTE                      pbFunctionNameString,
                                        SIZE_T                      cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE                      pbCustomizationString,
                                        SIZE_T                      cbCustomizationString);

VOID
SYMCRYPT_CALL
SymCryptCShake128Append(
    _Inout_                 PSYMCRYPT_CSHAKE128_STATE   pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptCShake128Extract(
    _Inout_                 PSYMCRYPT_CSHAKE128_STATE   pState,
    _Out_writes_(cbResult)  PBYTE                       pbResult,
                            SIZE_T                      cbResult,
                            BOOLEAN                     bWipe);

VOID
SYMCRYPT_CALL
SymCryptCShake128Result(
    _Inout_                                         PSYMCRYPT_CSHAKE128_STATE   pState,
    _Out_writes_( SYMCRYPT_CSHAKE128_RESULT_SIZE )  PBYTE                       pbResult);

VOID
SYMCRYPT_CALL
SymCryptCShake128StateCopy(_In_ PCSYMCRYPT_CSHAKE128_STATE pSrc, _Out_ PSYMCRYPT_CSHAKE128_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptCShake128Selftest(void);


//
// cSHAKE256
//
#define SYMCRYPT_CSHAKE256_RESULT_SIZE           SYMCRYPT_SHAKE256_RESULT_SIZE
#define SYMCRYPT_CSHAKE256_INPUT_BLOCK_SIZE      SYMCRYPT_SHAKE256_INPUT_BLOCK_SIZE

VOID
SYMCRYPT_CALL
SymCryptCShake256(
    _In_reads_( cbFunctionNameString )  PCBYTE  pbFunctionNameString,
                                        SIZE_T  cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE  pbCustomizationString,
                                        SIZE_T  cbCustomizationString,
    _In_reads_( cbData )                PCBYTE  pbData,
                                        SIZE_T  cbData,
    _Out_writes_( cbResult )            PBYTE   pbResult,
                                        SIZE_T  cbResult);

VOID
SYMCRYPT_CALL
SymCryptCShake256Init(
    _Out_                               PSYMCRYPT_CSHAKE256_STATE   pState,
    _In_reads_( cbFunctionNameString )  PCBYTE                      pbFunctionNameString,
                                        SIZE_T                      cbFunctionNameString,
    _In_reads_( cbCustomizationString ) PCBYTE                      pbCustomizationString,
                                        SIZE_T                      cbCustomizationString);

VOID
SYMCRYPT_CALL
SymCryptCShake256Append(
    _Inout_                 PSYMCRYPT_CSHAKE256_STATE   pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptCShake256Extract(
    _Inout_                 PSYMCRYPT_CSHAKE256_STATE   pState,
    _Out_writes_(cbResult)  PBYTE                       pbResult,
                            SIZE_T                      cbResult,
                            BOOLEAN                     bWipe);

VOID
SYMCRYPT_CALL
SymCryptCShake256Result(
    _Inout_                                         PSYMCRYPT_CSHAKE256_STATE   pState,
    _Out_writes_( SYMCRYPT_CSHAKE256_RESULT_SIZE )  PBYTE                      pbResult);

VOID
SYMCRYPT_CALL
SymCryptCShake256StateCopy(_In_ PCSYMCRYPT_CSHAKE256_STATE pSrc, _Out_ PSYMCRYPT_CSHAKE256_STATE pDst);

VOID
SYMCRYPT_CALL
SymCryptCShake256Selftest(void);



//==========================================================================
//   PARALLELISED HASH FUNCTIONS
//==========================================================================
//
// On some platforms it is possible to parallelize the hash function
// computation to achieve a higher throughput.
// The parallel hash APIs support this.
// The parallel implementation tries to perform the computations as efficiently
// as possible. Applications that have many hashes to compute can always call these
// functions; the library will optimize the computation to the current situation.
// For example, if only a single hash is computed using these APIs, the
// single-hash version is used to achieve full single-hash speed.
// On platforms that do not support parallel hash implementations, these functions
// are still available, and will implement the parallel hashing by computing the
// hashes one at a time.
//
//
// SYMCRYPT_PARALLEL_XXX_MIN_PARALLELISM
//
//      Compile-time constant, but can vary per platform.
//      Minimum number of parallel computations at which
//      the parallel implementation is faster on at least some CPU versions.
//      Applications can safely ask for parallel computations with fewer hashes,
//      but there will be no speed gain.
//
// SYMCRYPT_PARALLEL_XXX_MAX_PARALLELISM
//
//      Maximum internal parallelism that the library uses internally on at least one
//      CPU version of this architecture.
//      If all hash computations are the same length, then there is no significant
//      benefit to providing more than this number of hash requests in parallel.
//      However, if the hash computations are of different lengths then the library
//      overlaps various hash computations and still gains efficiency when the
//      number of parallel hash computations increases past this bound.
//      Note that the internal parallelism that can be used might depend
//      on the CPU features available, so this value is only an upper bound.
//      We recommend that callers provide as much parallelism as practical,
//      and let the library perform the optimal sequence of computations.
//
// SYMCRYPT_HASH_OPERATION_TYPE
//
//      An enum that specifies which operation is to be performed in a command
//      structure passed to a parallel hash operations function.
//      Defined values:
//          SYMCRYPT_HASH_OPERATION_APPEND;
//          SYMCRYPT_HASH_OPERATION_RESULT;
//
// SYMCRYPT_PARALLEL_HASH_OPERATION
//
//      Structure that contains a command to be performed on a single item in a
//      parallel hash state array. Visible fields are:
//
//      SIZE_T                          iHash;          // index of hash object into the state array
//      SYMCRYPT_HASH_OPERATION_TYPE    hashOperation;  // operation to be performed
//      PBYTE                           pbBuffer;       // data to be hashed, or result buffer
//      SIZE_T                          cbBuffer;
//
//      There might be other fields in this structure that the caller should not use or assume anything about.
//
// SymCryptParallelXxxInit(
//          _Out_writes_( nStates ) PSYMCRYPT_XXX_STATE pStates,
//                                  SIZE_T              nStates );
//      Initialize an array of hash states.
//      The elements of the array are normal hash states, and they can be
//      manipulated individually using the standard functions for the hash
//      algorithm.
//
//      Functionally equivalent to:
//          for( i=0; i<nStates; i++ ) {
//              SymCryptXxxInit( &pStates[i] );
//          }
//
//      It is not necessary to use this function to initialize a state array;
//      the normal initialization function can also be used, but this function might
//      be faster.
//
// SymCryptParallelXxxProcess(
//          _Inout_updates_( nStates )      PSYMCRYPT_XXX_STATE                 pStates,
//                                          SIZE_T                              nStates,
//          _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperation,
//                                          SIZE_T                              nOperations,
//          _Out_writes_( cbScratch )       PBYTE                               pbScratch,
//                                          SIZE_T                              cbScratch );
//
//      Perform optionally parallel processing of hashes.
//      This is functionally equivalent to iterating over the pOperations array in order,
//      and executing the command in each PARALLEL_HASH_OPERATION one at a time.
//      For each command:
//          iHash           Which hash state this operation applies to; must be < nStates.
//          hashOperation   Specifies whether this is an append or result operation.
//          pbBuffer        The buffer that contains the data to be hashed, or that will receive the result.
//          cbBuffer        The size of pbBuffer. (Must be equal to the hash algorithm result size for RESULT operations.)
//      As the SAL annotations document, the pOperations array is updated by this function, and therefore
//      it cannot be in read-only memory.
//      The updates modify only to the internal scratch space that is reserved
//      in the SYMCRYPT_PARALLEL_HASH_OPERATION structure; none of the documented fields
//      (iHash, hashOperation, pbBuffer, cbBuffer) are modified.
//      The scratch fields are used purely within one call to this function, their value does not have to be
//      maintained between function calls. The scratch fields do not have to be initialized by the caller
//      of this function,
//      THREAD SAFETY: as the pOperations array is updated, it CANNOT be shared between different threads.
//      Obviously, the same is true of pStates and pbScratch.
//
//      The pbScratch pointer provides a scratch buffer for the parallel processing function.
//      This is used to organize the request and perform the functions in an optimal order for
//      maximum parallelism, and for storing intermediate results that are too large
//      to fit on the stack. The scratch buffer must be at least
//      SYMCRYPT_PARALLEL_XXX_FIXED_SCRATCH + nStates * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH
//      bytes in size.
//
//      For incremental hashing, we recommend that callers process data sizes that are
//      a multiple of the SYMCRYPT_XXX_INPUT_BLOCK_LEN.
//


VOID
SYMCRYPT_CALL
SymCryptParallelSha256Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA256_STATE pStates,
                            SIZE_T                 nStates );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha256Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA256_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch );


VOID
SYMCRYPT_CALL
SymCryptParallelSha384Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA384_STATE pStates,
                            SIZE_T                 nStates );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha384Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA384_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch );


VOID
SYMCRYPT_CALL
SymCryptParallelSha512Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA512_STATE pStates,
                            SIZE_T                 nStates );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha512Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA512_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch );


VOID
SYMCRYPT_CALL
SymCryptParallelSha256Selftest(void);

VOID
SYMCRYPT_CALL
SymCryptParallelSha384Selftest(void);

VOID
SYMCRYPT_CALL
SymCryptParallelSha512Selftest(void);



//==========================================================================
//   MESSAGE AUTHENTICATION CODE (MAC)
//==========================================================================
//
// All MAC functions have a similar interface. For consistency we describe
// the generic parts of the interface once.
// Algorithm-specific comments are given with the API functions of each algorithm separately.
//
// For a MAC algorithm called XXX the following functions, types, and constants are defined:
//
//
// SYMCRYPT_XXX_RESULT_SIZE
//
//      A constant giving is the size, in bytes, of the result of the MAC function.
//      Some applications use truncated MAC functions. These are not directly supported
//      by this library. Applications will have to perform the truncation themselves.
//
//
// SYMCRYPT_XXX_INPUT_BLOCK_SIZE
//
//      A constant giving the natural input block size for the MAC function.
//      Most callers don't need to know this, but in some cases it can be useful
//      for optimizations.
//
//
// SYMCRYPT_XXX_EXPANDED_KEY
//
//      Type which contains a key with all the pre-computations performed.
//      This is an opaque type whose structure can change at will.
//      It should only be used for transient computations in a single executable
//      and not be stored or transferred to a different environment.
//      The pointer and const-pointer versions are also declared
//      (PSYMCRYPOT_XXX_EXPANDED_KEY and PCSYMCRYPT_XXX_EXPANDED_KEY).
//
//      The EXPANDED_KEY structure contains keying material and should be wiped
//      once it is no longer used. (See SymCryptWipe & SymCryptWipeKnownSize)
//
//      Once a key has been expanded, multiple threads can simultaneously use the same expanded key
//      object for different MAC computations that use the same key as the expanded key
//      object does not change value.
//
//
// SYMCRYPT_ERROR
// SYMCRYPT_CALL
// SymCryptXxxExpandKey(   _Out_                PSYMCRYPT_XXX_EXPANDED_KEY  pExpandedKey,
//                         _In_reads_(cbKey)    PCBYTE                      pbKey,
//                                              SIZE_T                      cbKey );
//
//      Prepare a key for future use by the Xxx algorithm.
//      This function performs pre-computations on the key
//      to speed up the actual MAC computations later, and stores the result as an expanded key.
//      The expanded key must be kept unchanged until all MAC computations that use the key are finished.
//      When the key is no longer needed the expanded key structure should be wiped.
//
//      Different algorithms pose different requirements on the length of the key.
//      If the key that is provided is of an unsupported length the SYMCRYPT_WRONG_KEY_SIZE error is returned.
//      In this case the expanded key structure will not contain any keying material and does not have to be wiped.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxKeyCopy( _In_ PCSYMCRYPT_XXX_EXPANDED_KEY pSrc,
//                     _Out_ PSYMCRYPT_XXX_EXPANDED_KEY pDst );
//
//      Create a copy of an expanded key.
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxx( _In_                                        PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//              _In_reads_( cbData )                        PCBYTE                      pbData,
//                                                          SIZE_T                      cbData,
//              _Out_writes_( SYMCRYPT_XXX_RESULT_SIZE )    PBYTE                       pbResult );
//
//      Computes the MAC value of the data buffer with a given key.
//      If you have all the data to be MACed in a single buffer this is the simplest function to use.
//
//
// SYMCRYPT_XXX_STATE
//
//      The state encodes an ongoing MAC computation and allows incremental
//      computation of a MAC function.
//      At any point in time the state encodes a state that is equivalent to
//      the MAC computation of a data string X with the key specified during initialization of the state.
//      The SymCryptXxxInit() function initializes a state.
//      The SymCryptXxxAppend() function appends data to the data string X.
//      The SymCryptXxxResult() function returns the final MAC result.
//
//      The state is an opaque type whose structure can change at will.
//      It should only be used for transient computations in a single executable
//      and not be stored or transferred to a different environment.
//
//      Once initialized using SymCryptXxxInit, the state contains sensitive keying information.
//      The SymCryptXxxResult function wipes the sensitive information from the state.
//      Callers can also wipe the structure themselves if it is no longer needed.
//
//      The state can be duplicated using the SymCryptXxxStateCopy function. This supports
//      applications that compute the MAC over a prefix and then duplicate the state to
//      compute the MAC using multiple different continuations.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxStateCopy(
//      _In_        PCSYMCRYPT_XXX_STATE        pSrc,
//      _In_opt_    PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//      _Out_       PSYMCRYPT_XXX_STATE         pDst );
//
//      Create a copy of the pSrc state in pDst. If pExpandedKey is NULL, the pDst state
//      uses the same expanded key as the pSrc state did. If pExpandedKey is not NULL,
//      it must point to an expanded key that contains the same key material as the key
//      used by pSrc. (For example, a copy of the expanded key that pSrc uses.)
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxInit( _Out_   PSYMCRYPT_XXX_STATE         pState,
//                  _In_    PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey);
//
//      Initialize a SYMCRYPT_XXX_STATE for subsequent use with the provided key.
//
//      This function can be called at any time and resets the state to correspond
//      to the empty data string with the newly specified key.
//      The SymCryptXxxAppend function appends data to the data string
//      encoded by the state.
//      The SymCryptXxxResult function finalizes the computation and
//      returns the actual MAC result.
//
//      This function typically stores a pointer to the expanded key in the state.
//      The expanded key must remain unchanged in
//      memory until the SYMCRYPT_XXX_STATE structure is no longer used.
//
//      After initialization the state contains sensitive keying materials, and should
//      be wiped when the state is no longer used. The SymCryptXxxResult() function
//      also wipes the state, so this is only a concern for aborted MAC computations.
//      Note that SymCryptXxxResult() does not wipe the expanded key; callers are always
//      responsible for wiping the expanded key.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxAppend( _Inout_               PSYMCRYPT_XXX_STATE   pState,
//                    _In_reads_( cbData )  PCBYTE                pbData,
//                                          SIZE_T                cbData );
//
//      Provide more data to the ongoing MAC computation specified by the state.
//      The state must have been initialized by SymCryptXxxInit.
//      This function can be called multiple times on the same state
//      to append more data to the encoded data string.
//
//      The SYMCRYPT_XXX_STATE structure contains the entire state of an ongoing
//      MAC computation. If you want to MAC some data and then continue with
//      multiple other strings you may create one or more copies of the state.
//      (The expanded key must remain unchanged in memory until all copies of the state
//      are no longer used.)
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxResult(
//      _Inout_                                     PSYMCRYPT_XXX_STATE  pState,
//      _Out_writes_( SYMCRYPT_XXX_RESULT_SIZE )    PBYTE                pbResult );
//
//      Returns the MAC result of the state.
//      If the state was newly initialized this returns the MAC of the empty string
//      using the key specified in the SymCryptXxxInit call.
//      If one or more SymCryptXxxAppend function calls were made on this state
//      it returns the MAC of the concatenation of all the data strings
//      passed to SymCryptXxxAppend using the specified key.
//
//      The state is wiped to remove any traces of sensitive data.
//      To use the same state for another MAC computation you must call
//      SymCryptXxxInit again to re-initialize the state.
//      This behaviour is different from hash function states that are re-initialized for
//      use by the Result routine. This difference is by design; re-initializing a hash
//      state is a safe operation. Re-initializing a MAC state puts keying information
//      in the state, and callers would have to wipe the MAC state explicitly.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxSelftest(void);
//
//      Perform a minimal self-test on the XXX algorithm.
//      This function is designed to be used for achieving FIPS 140-2 compliance or
//      to provide a simple self-test when an application starts.
//
//      If an error is detected, a platform-specific fatal error action is taken.
//      Callers do not need to handle any error conditions.
//
//
// We also have the Generic HMAC API where the hash function to be used in the HMAC
// computation can be selected at runtime.
//

typedef enum _SYMCRYPT_MAC_ID
{
    SYMCRYPT_MAC_ID_NULL                = 0,
    SYMCRYPT_MAC_ID_HMAC_MD5            = 1,
    SYMCRYPT_MAC_ID_HMAC_SHA1           = 2,
    SYMCRYPT_MAC_ID_HMAC_SHA224         = 3,
    SYMCRYPT_MAC_ID_HMAC_SHA256         = 4,
    SYMCRYPT_MAC_ID_HMAC_SHA384         = 5,
    SYMCRYPT_MAC_ID_HMAC_SHA512         = 6,
    SYMCRYPT_MAC_ID_HMAC_SHA512_224     = 7,
    SYMCRYPT_MAC_ID_HMAC_SHA512_256     = 8,
    SYMCRYPT_MAC_ID_HMAC_SHA3_224       = 9,
    SYMCRYPT_MAC_ID_HMAC_SHA3_256       = 10,
    SYMCRYPT_MAC_ID_HMAC_SHA3_384       = 11,
    SYMCRYPT_MAC_ID_HMAC_SHA3_512       = 12,
    SYMCRYPT_MAC_ID_AES_CMAC            = 13,
    SYMCRYPT_MAC_ID_KMAC_128            = 14,
    SYMCRYPT_MAC_ID_KMAC_256            = 15
} SYMCRYPT_MAC_ID;

PCSYMCRYPT_MAC
SYMCRYPT_CALL
SymCryptGetMacAlgorithm( SYMCRYPT_MAC_ID macId );
//
// Returns a pointer to the MAC algorithm structure for the specified MAC ID.
// Returns NULL if the MAC ID is invalid.
//

//
// Generic HMAC API with parametrized hash function
//
VOID
SYMCRYPT_CALL
SymCryptHmacStateCopy(
    _In_        PCSYMCRYPT_HMAC_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_STATE            pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacKeyCopy(
    _In_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pSrc,
    _Out_   PSYMCRYPT_HMAC_EXPANDED_KEY     pDst );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacExpandKey(
    _In_                    PCSYMCRYPT_HASH             pHash,
    _Out_                   PSYMCRYPT_HMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                      pbKey,
                            SIZE_T                      cbKey );

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmacInit(
    _Out_   PSYMCRYPT_HMAC_STATE            pState,
    _In_    PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey );

VOID
SYMCRYPT_CALL
SymCryptHmacAppend(
    _Inout_                 PSYMCRYPT_HMAC_STATE    pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmacResult(
    _Inout_                                         PSYMCRYPT_HMAC_STATE    pState,
    _Out_writes_( pState->pKey->pHash->resultSize ) PBYTE                   pbResult );

SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptHmac(
    _In_                                            PCSYMCRYPT_HMAC_EXPANDED_KEY    pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                          pbData,
                                                    SIZE_T                          cbData,
    _Out_writes_( pExpandedKey->pHash->resultSize ) PBYTE                           pbResult );


////////////////////////////////////////////////////////////////////////////
//   HMAC-MD5
//
//

#define SYMCRYPT_HMAC_MD5_RESULT_SIZE       SYMCRYPT_MD5_RESULT_SIZE
#define SYMCRYPT_HMAC_MD5_INPUT_BLOCK_SIZE  SYMCRYPT_MD5_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacMd5ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_MD5_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                          pbKey,
                            SIZE_T                          cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacMd5KeyCopy(
    _In_    PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_MD5_EXPANDED_KEY  pDst );


VOID
SYMCRYPT_CALL
SymCryptHmacMd5(
    _In_                                            PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                           pbData,
                                                    SIZE_T                           cbData,
    _Out_writes_( SYMCRYPT_HMAC_MD5_RESULT_SIZE )   PBYTE                            pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacMd5StateCopy(
    _In_        PCSYMCRYPT_HMAC_MD5_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY    pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_MD5_STATE            pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacMd5Init(
    _Out_   PSYMCRYPT_HMAC_MD5_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_MD5_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacMd5Append(
    _Inout_                 PSYMCRYPT_HMAC_MD5_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacMd5Result(
    _Inout_                                      PSYMCRYPT_HMAC_MD5_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_MD5_RESULT_SIZE )PBYTE                      pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacMd5Selftest(void);

extern const PCSYMCRYPT_MAC SymCryptHmacMd5Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-1
//
//

#define SYMCRYPT_HMAC_SHA1_RESULT_SIZE       SYMCRYPT_SHA1_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA1_INPUT_BLOCK_SIZE  SYMCRYPT_SHA1_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha1ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA1_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                              pbKey,
                            SIZE_T                              cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha1KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY  pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA1_EXPANDED_KEY   pDst );


VOID
SYMCRYPT_CALL
SymCryptHmacSha1(
    _In_                                            PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA1_RESULT_SIZE )  PBYTE                               pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha1StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA1_STATE          pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY   pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA1_STATE           pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha1Init(
    _Out_   PSYMCRYPT_HMAC_SHA1_STATE           pState,
    _In_    PCSYMCRYPT_HMAC_SHA1_EXPANDED_KEY   pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha1Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA1_STATE   pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha1Result(
    _Inout_                                         PSYMCRYPT_HMAC_SHA1_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA1_RESULT_SIZE )  PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha1Selftest(void);

extern const PCSYMCRYPT_MAC SymCryptHmacSha1Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-224
//
// This implementation is meant for interoperability and is not recommended for use.
//
//

#define SYMCRYPT_HMAC_SHA224_RESULT_SIZE       SYMCRYPT_SHA224_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA224_INPUT_BLOCK_SIZE  SYMCRYPT_SHA224_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha224ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA224_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                              pbKey,
                            SIZE_T                              cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha224KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA224_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha224(
    _In_                                            PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA224_RESULT_SIZE )PBYTE                               pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha224StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA224_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA224_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha224Init(
    _Out_   PSYMCRYPT_HMAC_SHA224_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA224_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha224Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA224_STATE pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha224Result(
    _Inout_                                         PSYMCRYPT_HMAC_SHA224_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA224_RESULT_SIZE )PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha224Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha224Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-256
//
//

#define SYMCRYPT_HMAC_SHA256_RESULT_SIZE       SYMCRYPT_SHA256_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA256_INPUT_BLOCK_SIZE  SYMCRYPT_SHA256_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha256ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA256_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                              pbKey,
                            SIZE_T                              cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha256KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA256_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha256(
    _In_                                            PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA256_RESULT_SIZE )PBYTE                               pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha256StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA256_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA256_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha256Init(
    _Out_   PSYMCRYPT_HMAC_SHA256_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA256_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha256Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA256_STATE pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha256Result(
    _Inout_                                         PSYMCRYPT_HMAC_SHA256_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA256_RESULT_SIZE )PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha256Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha256Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-384
//
//

#define SYMCRYPT_HMAC_SHA384_RESULT_SIZE       SYMCRYPT_SHA384_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA384_INPUT_BLOCK_SIZE  SYMCRYPT_SHA384_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha384ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA384_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                              pbKey,
                            SIZE_T                              cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha384KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA384_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha384(
    _In_                                            PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA384_RESULT_SIZE )PBYTE                               pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha384StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA384_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA384_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha384Init(
    _Out_   PSYMCRYPT_HMAC_SHA384_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA384_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha384Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA384_STATE pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha384Result(
    _Inout_                                         PSYMCRYPT_HMAC_SHA384_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA384_RESULT_SIZE )PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha384Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha384Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-512
//
//

#define SYMCRYPT_HMAC_SHA512_RESULT_SIZE       SYMCRYPT_SHA512_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA512_INPUT_BLOCK_SIZE  SYMCRYPT_SHA512_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha512ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA512_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                              pbKey,
                            SIZE_T                              cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha512KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA512_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512(
    _In_                                            PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_RESULT_SIZE )PBYTE                               pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA512_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA512_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512Init(
    _Out_   PSYMCRYPT_HMAC_SHA512_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA512_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha512Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA512_STATE pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512Result(
    _Inout_                                         PSYMCRYPT_HMAC_SHA512_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_RESULT_SIZE )PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha512Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-512_224
//
// This implementation is meant for interoperability and is not recommended for use.
//
//

#define SYMCRYPT_HMAC_SHA512_224_RESULT_SIZE       SYMCRYPT_SHA512_224_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA512_224_INPUT_BLOCK_SIZE  SYMCRYPT_SHA512_224_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha512_224ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224(
    _In_                                                    PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                                    PCBYTE                                  pbData,
                                                            SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_224_RESULT_SIZE )    PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA512_224_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA512_224_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224Init(
    _Out_   PSYMCRYPT_HMAC_SHA512_224_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA512_224_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA512_224_STATE pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224Result(
    _Inout_                                                 PSYMCRYPT_HMAC_SHA512_224_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_224_RESULT_SIZE )    PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_224Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha512_224Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA-512_256
//
// This implementation is meant for interoperability and is not recommended for use.
//
//

#define SYMCRYPT_HMAC_SHA512_256_RESULT_SIZE       SYMCRYPT_SHA512_256_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA512_256_INPUT_BLOCK_SIZE  SYMCRYPT_SHA512_256_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha512_256ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256(
    _In_                                                    PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                                    PCBYTE                                  pbData,
                                                            SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_256_RESULT_SIZE )    PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA512_256_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA512_256_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256Init(
    _Out_   PSYMCRYPT_HMAC_SHA512_256_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA512_256_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA512_256_STATE pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256Result(
    _Inout_                                                 PSYMCRYPT_HMAC_SHA512_256_STATE pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA512_256_RESULT_SIZE )    PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha512_256Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha512_256Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA3-224
//
// This implementation is meant for interoperability and is not recommended for use.
//
//

#define SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE       SYMCRYPT_SHA3_224_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA3_224_INPUT_BLOCK_SIZE  SYMCRYPT_SHA3_224_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_224ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE )  PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_224_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_224_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_224_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_224_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_224_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_224_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_224_RESULT_SIZE )  PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_224Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha3_224Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA3-256
//
//

#define SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE       SYMCRYPT_SHA3_256_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA3_256_INPUT_BLOCK_SIZE  SYMCRYPT_SHA3_256_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_256ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE )  PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_256_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_256_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_256_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_256_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_256_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_256_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_256_RESULT_SIZE )  PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_256Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha3_256Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA3-384
//
//

#define SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE       SYMCRYPT_SHA3_384_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA3_384_INPUT_BLOCK_SIZE  SYMCRYPT_SHA3_384_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_384ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE )  PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_384_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_384_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_384_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_384_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_384_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_384_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_384_RESULT_SIZE )  PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_384Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha3_384Algorithm;

////////////////////////////////////////////////////////////////////////////
//   HMAC-SHA3-512
//
//

#define SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE       SYMCRYPT_SHA3_512_RESULT_SIZE
#define SYMCRYPT_HMAC_SHA3_512_INPUT_BLOCK_SIZE  SYMCRYPT_SHA3_512_INPUT_BLOCK_SIZE

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHmacSha3_512ExpandKey(
    _Out_                   PSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbKey)   PCBYTE                                  pbKey,
                            SIZE_T                                  cbKey );
//
// Supports all key lengths; never returns an error.
//

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512KeyCopy(
    _In_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512(
    _In_                                                PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY   pExpandedKey,
    _In_reads_( cbData )                                PCBYTE                                  pbData,
                                                        SIZE_T                                  cbData,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE )  PBYTE                                   pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512StateCopy(
    _In_        PCSYMCRYPT_HMAC_SHA3_512_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_HMAC_SHA3_512_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Init(
    _Out_   PSYMCRYPT_HMAC_SHA3_512_STATE         pState,
    _In_    PCSYMCRYPT_HMAC_SHA3_512_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Append(
    _Inout_                 PSYMCRYPT_HMAC_SHA3_512_STATE   pState,
    _In_reads_( cbData )    PCBYTE                          pbData,
                            SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Result(
    _Inout_                                             PSYMCRYPT_HMAC_SHA3_512_STATE   pState,
    _Out_writes_( SYMCRYPT_HMAC_SHA3_512_RESULT_SIZE )  PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptHmacSha3_512Selftest(void);

extern const PCSYMCRYPT_MAC  SymCryptHmacSha3_512Algorithm;


////////////////////////////////////////////////////////////////////////////
//   AES-CMAC
//
// This is the AES-CMAC algorithm per SP 800-38B & RFC 4493.
// It is also known as AES-OMAC1.
//

#define SYMCRYPT_AES_CMAC_RESULT_SIZE        (16)
#define SYMCRYPT_AES_CMAC_INPUT_BLOCK_SIZE   (16)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesCmacExpandKey(
    _Out_               PSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey );
//
// Key size must be a valid AES key (16, 24, or 32 bytes)
//

VOID
SYMCRYPT_CALL
SymCryptAesCmacKeyCopy(
    _In_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_AES_CMAC_EXPANDED_KEY  pDst );

VOID
SYMCRYPT_CALL
SymCryptAesCmac(
    _In_                                            PSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                            PCBYTE                          pbData,
                                                    SIZE_T                          cbData,
    _Out_writes_( SYMCRYPT_AES_CMAC_RESULT_SIZE )   PBYTE                           pbResult );

VOID
SYMCRYPT_CALL
SymCryptAesCmacStateCopy(
    _In_        PCSYMCRYPT_AES_CMAC_STATE        pSrc,
    _In_opt_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey,
    _Out_       PSYMCRYPT_AES_CMAC_STATE         pDst );

VOID
SYMCRYPT_CALL
SymCryptAesCmacInit(
    _Out_   PSYMCRYPT_AES_CMAC_STATE        pState,
    _In_    PCSYMCRYPT_AES_CMAC_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptAesCmacAppend(
    _Inout_                 PSYMCRYPT_AES_CMAC_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCmacResult(
    _Inout_                                         PSYMCRYPT_AES_CMAC_STATE    pState,
    _Out_writes_( SYMCRYPT_AES_CMAC_RESULT_SIZE )   PBYTE                       pbResult );

VOID
SYMCRYPT_CALL
SymCryptAesCmacSelftest(void);

extern const PCSYMCRYPT_MAC SymCryptAesCmacAlgorithm;

////////////////////////////////////////////////////////////////////////////
// KMAC
//
// Keccak Message Authentication Code (KMAC) is specified in NIST SP 800-185
// and has two variants; KMAC128 and KMAC256, using cSHAKE128 and cSHAKE256
// as the underlying functions, respectively.
//
// KMAC128(K, X, L, S) = cSHAKE128(bytepad(encode_string(K), 168) || X || right_encode(L), L, "KMAC", S)
// KMAC256(K, X, L, S) = cSHAKE256(bytepad(encode_string(K), 136) || X || right_encode(L), L, "KMAC", S)
//
// KMAC accepts a variable-size key. There's no restriction on the size of the key.
//
// KMAC differs from other MAC algorithms in SymCrypt by having two additional input
// parameters; a customization string and the length of the output. Output generated
// by KMAC also depends on the specified output length, i.e., outputs generated from
// two KMAC calls with the same key, message, customization string, but different output
// lengths will be unrelated/uncorrelated. This differs from SHAKE and cSHAKE where an
// output of size N bytes from the algorithm is a prefix of the output of size M bytes
// where N < M, when the inputs are the same.
//
// KMAC works in two modes; fixed-length mode and XOF mode. XOF variants are named KMACXOF128
// and  KMACXOF256. SymCrypt does not provide a separate KMACXOF API but supports them via
// the KMAC interface.
//
// KMACXOF128(K, X, L, S) = cSHAKE128(bytepad(encode_string(K), 168) || X || right_encode(0), L, "KMAC", S)
// KMACXOF256(K, X, L, S) = cSHAKE256(bytepad(encode_string(K), 136) || X || right_encode(0), L, "KMAC", S)
//
// KMAC output generation mode is determined by the output length parameter
// L in SP 800-185; if it is non-zero then KMAC works in fixed-length mode, otherwise (i.e., L=0)
// it works in XOF mode.
//  - Fixed-length mode generates result with SymCryptKmacXxxResult or SymCryptKmacXxxResultEx.
//    These functions wipe the state after generating output, thus can only be used
//    once per initialized state. The result size is SYMCRYPT_KMAC_XXX_RESULT
//    for SymCryptKmacXxxResult and specified by the caller for SymCryptKmacXxxResultEx.
//  - XOF mode can produce arbitrary length output. SymCryptKmacXxxExtract function puts KMAC
//    state into XOF mode and all the successive calls that generate output from the KMAC state will be
//    from the XOF mode. SymCryptKmacXxxResult and SymCryptKmacXxxResultEx functions
//    will also generate output in XOF mode IF they are called after a SymCryptKmacXxxExtract
//    function with bWipe=FALSE (so that the state remains in XOF mode). Note that
//    SymCryptKmacXxxResult and SymCryptKmacXxxResultEx functions wipe the state afterwards,
//    thus KMAC state can only be used to generate output in XOF mode once with these two functions.
//
//  SYMCRYPT_KMACXXX_RESULT_SIZE
//
//      Default result size when KMAC is used with the existing MAC interface.
//      Equals to twice the SYMCRYPT_KMACXXX_KEY_SIZE.
//
//  SYMCRYPT_ERROR
//  SYMCRYPT_CALL
//  SymCryptKmacXxxExpandKey(
//          _Out_                                       PSYMCRYPT_KMACXXX_EXPANDED_KEY pExpandedKey,
//          _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
//                                                      SIZE_T  cbKey);
//
//      Performs key expansion with empty customization string.
//      There's no restriction on the size of the key.
//
//  SYMCRYPT_ERROR
//  SYMCRYPT_CALL
//  SymCryptKmacXxxExpandKeyEx(
//          _Out_                                       PSYMCRYPT_KMAXXX_EXPANDED_KEY pExpandedKey,
//          _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
//                                                      SIZE_T  cbKey,
//          _In_reads_bytes_( cbCustomizationString )   PCBYTE  pbCustomizationString,
//                                                      SIZE_T  cbCustomizationString);
//
//      Performs key expansion for the provided key and customization string.
//      There's no restriction on the size of the key.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxx(
//          _In_                                                PCSYMCRYPT_KMACXXX_EXPANDED_KEY pExpandedKey,
//          _In_reads_bytes_( cbInput )                         PCBYTE  pbInput,
//                                                              SIZE_T  cbInput,
//          _Out_writes_bytes_( SYMCRYPT_KMACXXX_RESULT_SIZE )  PBYTE   pbResult);
//
//      Single-call KMAC computation for the given input producing default result
//      size SYMCRYPT_KMACXXX_RESULT_SIZE.
//
//      pExpandedKey must be initialized before the call. This function is equivalent
//      to SymCryptKmacXxxEx with output size set to SYMCRYPT_KMACXXX_RESULT_SIZE.
//      If a result size different than the default value is desired, SymCryptKmacXxxEx
//      must be called.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxEx(
//          _In_                            PCSYMCRYPT_KMACXXX_EXPANDED_KEY pExpandedKey,
//          _In_reads_bytes_( cbInput )     PCBYTE  pbInput,
//                                          SIZE_T  cbInput,
//          _Out_writes_bytes_( cbResult )  PBYTE   pbResult,
//                                          SIZE_T  cbResult);
//
//      Single-call KMAC computation for the given input producing cbResult bytes result.
//      pExpandedKey must be initialized before the call.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxInit(
//      _Out_   PSYMCRYPT_KMACXXX_STATE         pState,
//      _In_    PCSYMCRYPT_KMACXXX_EXPANDED_KEY pExpandedKey);
//
//      Initializes KMAC state for appending data for the provided key. Expanded
//      key must be generated prior to this call.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxAppend(
//      _Inout_                 PSYMCRYPT_KMACXXX_STATE pState,
//      _In_reads_( cbData )    PCBYTE                  pbData,
//                              SIZE_T                  cbData );
//
//      Appends data to the KMAC state.
//
//      This function must only be called after SymCryptKmacXxxInit or SymCryptKmacXxxAppend.
//      Calling SymCryptKmacXxxAppend after SymCryptKmacXxxExtract with bWipe=FALSE
//      is not well-defined. KMAC state must be initialized with SymCryptKmacXxxInit before
//      the first call to SymCryptKmacXxxAppend.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxExtract(
//      _Inout_                     PSYMCRYPT_KMACXXX_STATE pState,
//      _Out_writes_( cbOutput )    PBYTE                   pbOutput,
//                                  SIZE_T                  cbOutput,
//                                  BOOLEAN                 bWipe);
//
//      Generates KMAC output in XOF mode.
//
//      Extract can only be called after an Init, Append or Extract call.
//      The state is cleared if bWipe=TRUE, otherwise further Extract calls
//      can be made to generate more output.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxResult(
//      _Inout_                                         PSYMCRYPT_KMACXXX_STATE pState,
//      _Out_writes_( SYMCRYPT_KMACXXX_RESULT_SIZE )    PBYTE   pbResult);
//
//      Produces SYMCRYPT_KMACXXX_RESULT_SIZE bytes of output from the KMAC state.
//      The state is wiped on return.
//
//      This function internally calls SymCryptKmacXxxResultEx with result size
//      SYMCRYPT_KMACXXX_RESULT_SIZE.
//      If Result is called in XOF mode (i.e., after an Extract with bWipe=FALSE), it
//      performs a final extraction of SYMCRYPT_KMACXXX_RESULT_SIZE bytes in XOF mode
//      and clears the state afterwards.
//      Result function does not re-initialize the state for a new computation like
//      the Result for hash functions do. Computing a new MAC with the same key
//      requires calling the SymCryptKmacXxxInit function first.
//
//  VOID
//  SYMCRYPT_CALL
//  SymCryptKmacXxxResultEx(
//      _Inout_                     PSYMCRYPT_KMACXXX_STATE pState,
//      _Out_writes_( cbResult )    PBYTE   pbResult,
//                                  SIZE_T  cbResult);
//
//      Produces cbResult bytes of output from the KMAC state. The state is
//      wiped on return.
//
//      If ResultEx is called in XOF mode (i.e., after an Extract with bWipe=FALSE), it
//      performs a final extraction of cbResult bytes in XOF mode and clears the state
//      afterwards.
//      ResultEx function does not re-initialize the state for a new computation like
//      the Result for hash functions do. Computing a new MAC with the same key
//      requires calling the SymCryptKmacXxxInit function first.
//


//
//  KMAC128
//
#define SYMCRYPT_KMAC128_RESULT_SIZE        SYMCRYPT_CSHAKE128_RESULT_SIZE
#define SYMCRYPT_KMAC128_INPUT_BLOCK_SIZE   SYMCRYPT_CSHAKE128_INPUT_BLOCK_SIZE

VOID
SYMCRYPT_CALL
SymCryptKmac128(
        _In_                                                PCSYMCRYPT_KMAC128_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )                         PCBYTE  pbInput,
                                                            SIZE_T  cbInput,
        _Out_writes_bytes_( SYMCRYPT_KMAC128_RESULT_SIZE )  PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac128Ex(
        _In_                            PCSYMCRYPT_KMAC128_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )     PCBYTE  pbInput,
                                        SIZE_T  cbInput,
        _Out_writes_bytes_( cbResult )  PBYTE   pbResult,
                                        SIZE_T  cbResult);


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKmac128ExpandKey(
        _Out_                                       PSYMCRYPT_KMAC128_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
                                                    SIZE_T  cbKey);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKmac128ExpandKeyEx(
        _Out_                                       PSYMCRYPT_KMAC128_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
                                                    SIZE_T  cbKey,
        _In_reads_bytes_( cbCustomizationString )   PCBYTE  pbCustomizationString,
                                                    SIZE_T  cbCustomizationString);

VOID
SYMCRYPT_CALL
SymCryptKmac128Init(
    _Out_   PSYMCRYPT_KMAC128_STATE         pState,
    _In_    PCSYMCRYPT_KMAC128_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptKmac128Append(
    _Inout_                 PSYMCRYPT_KMAC128_STATE pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptKmac128Extract(
    _Inout_                     PSYMCRYPT_KMAC128_STATE pState,
    _Out_writes_( cbOutput )    PBYTE                   pbOutput,
                                SIZE_T                  cbOutput,
                                BOOLEAN                 bWipe);

VOID
SYMCRYPT_CALL
SymCryptKmac128Result(
    _Inout_                                         PSYMCRYPT_KMAC128_STATE pState,
    _Out_writes_( SYMCRYPT_KMAC128_RESULT_SIZE )    PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac128ResultEx(
    _Inout_                     PSYMCRYPT_KMAC128_STATE pState,
    _Out_writes_( cbResult )    PBYTE   pbResult,
                                SIZE_T  cbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac128KeyCopy(_In_ PCSYMCRYPT_KMAC128_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_KMAC128_EXPANDED_KEY pDst);

VOID
SYMCRYPT_CALL
SymCryptKmac128StateCopy(_In_ const SYMCRYPT_KMAC128_STATE* pSrc, _Out_ SYMCRYPT_KMAC128_STATE* pDst);

VOID
SYMCRYPT_CALL
SymCryptKmac128Selftest(void);

extern const PCSYMCRYPT_MAC SymCryptKmac128Algorithm;

//
//  KMAC256
//
#define SYMCRYPT_KMAC256_RESULT_SIZE        SYMCRYPT_CSHAKE256_RESULT_SIZE
#define SYMCRYPT_KMAC256_INPUT_BLOCK_SIZE   SYMCRYPT_CSHAKE256_INPUT_BLOCK_SIZE

VOID
SYMCRYPT_CALL
SymCryptKmac256(
        _In_                                                PCSYMCRYPT_KMAC256_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )                         PCBYTE  pbInput,
                                                            SIZE_T  cbInput,
        _Out_writes_bytes_( SYMCRYPT_KMAC256_RESULT_SIZE )  PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac256Ex(
        _In_                            PCSYMCRYPT_KMAC256_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbInput )     PCBYTE  pbInput,
                                        SIZE_T  cbInput,
        _Out_writes_bytes_( cbResult )  PBYTE   pbResult,
                                        SIZE_T  cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKmac256ExpandKey(
        _Out_                                       PSYMCRYPT_KMAC256_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
                                                    SIZE_T  cbKey);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptKmac256ExpandKeyEx(
        _Out_                                       PSYMCRYPT_KMAC256_EXPANDED_KEY pExpandedKey,
        _In_reads_bytes_( cbKey )                   PCBYTE  pbKey,
                                                    SIZE_T  cbKey,
        _In_reads_bytes_( cbCustomizationString )   PCBYTE  pbCustomizationString,
                                                    SIZE_T  cbCustomizationString);

VOID
SYMCRYPT_CALL
SymCryptKmac256Init(
    _Out_   PSYMCRYPT_KMAC256_STATE         pState,
    _In_    PCSYMCRYPT_KMAC256_EXPANDED_KEY pExpandedKey);

VOID
SYMCRYPT_CALL
SymCryptKmac256Append(
    _Inout_                 PSYMCRYPT_KMAC256_STATE pState,
    _In_reads_( cbData )    PCBYTE                  pbData,
                            SIZE_T                  cbData );

VOID
SYMCRYPT_CALL
SymCryptKmac256Extract(
    _Inout_                     PSYMCRYPT_KMAC256_STATE pState,
    _Out_writes_( cbOutput )    PBYTE                   pbOutput,
                                SIZE_T                  cbOutput,
                                BOOLEAN                 bWipe);

VOID
SYMCRYPT_CALL
SymCryptKmac256Result(
    _Inout_                                         PSYMCRYPT_KMAC256_STATE pState,
    _Out_writes_( SYMCRYPT_KMAC256_RESULT_SIZE )    PBYTE   pbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac256ResultEx(
    _Inout_                     PSYMCRYPT_KMAC256_STATE pState,
    _Out_writes_( cbResult )    PBYTE                   pbResult,
                                SIZE_T                  cbResult);

VOID
SYMCRYPT_CALL
SymCryptKmac256KeyCopy(_In_ PCSYMCRYPT_KMAC256_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_KMAC256_EXPANDED_KEY pDst);

VOID
SYMCRYPT_CALL
SymCryptKmac256StateCopy(_In_ const SYMCRYPT_KMAC256_STATE* pSrc, _Out_ SYMCRYPT_KMAC256_STATE* pDst);

VOID
SYMCRYPT_CALL
SymCryptKmac256Selftest(void);

extern const PCSYMCRYPT_MAC SymCryptKmac256Algorithm;


////////////////////////////////////////////////////////////////////////////
// POLY1305
//
// Poly1305 is different from other MAC functions because a key can only
// be used safely for a single message.
// We do not follow the default API pattern for MAC functions as that invites
// callers to compute multiple MACs per key.
//

#define SYMCRYPT_POLY1305_RESULT_SIZE   (16)
#define SYMCRYPT_POLY1305_BLOCK_SIZE    (16)
#define SYMCRYPT_POLY1305_KEY_SIZE      (32)

VOID
SYMCRYPT_CALL
SymCryptPoly1305(
    _In_reads_( SYMCRYPT_POLY1305_KEY_SIZE )        PCBYTE  pbKey,
    _In_reads_( cbData )                            PCBYTE  pbData,
                                                    SIZE_T  cbData,
    _Out_writes_( SYMCRYPT_POLY1305_RESULT_SIZE )   PBYTE   pbResult );
// Compute a Poly1305 authentication with the provided key on the data buffer.
// Note: A Poly1305 key may only be used for a single message.

VOID
SYMCRYPT_CALL
SymCryptPoly1305Init(
    _Out_                                       PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( SYMCRYPT_POLY1305_KEY_SIZE )    PCBYTE                      pbKey );
// Starts an incremental Poly1305 computation.
// Note: A Poly1305 key may only be used for a single message.

VOID
SYMCRYPT_CALL
SymCryptPoly1305Append(
    _Inout_                 PSYMCRYPT_POLY1305_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptPoly1305Result(
    _Inout_                                         PSYMCRYPT_POLY1305_STATE    pState,
    _Out_writes_( SYMCRYPT_POLY1305_RESULT_SIZE )   PBYTE                       pbResult );
// The state is wiped and not suitable for re-use.

VOID
SYMCRYPT_CALL
SymCryptPoly1305Selftest(void);

//
// We do NOT define a SYMCRYPT_MAC structure SymCryptPoly1305Algorithm
// for Poly1305 as it is a 1-time MAC function and cannot safely be used
// by any KDF we have
//
// NOT DEFINED: extern const PCSYMCRYPT_MAC SymCryptPoly1305Algorithm;
//

////////////////////////////////////////////////////////////////////////////
// CHACHA20_POLY1305
//
// This algorithm combines the CHACHA20 symmetric key stream cipher with
// the POLY1305 MAC function as per RFC 8439.
// The POLY1305 authenticator key is generated from the first 32 bytes
// of the CHACHA20 keystream and is only valid for a single message.
// For this reason each key and nonce combination passed to
// SymCryptChaCha20Poly1305Encrypt MUST only be used once.
//
// The Src and Dst buffers can be identical or non-overlapping; partial overlaps
// are not supported.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Encrypt(
    _In_reads_( cbKey )             PCBYTE    pbKey,
                                    SIZE_T    cbKey,      // Required. Key size MUST be 32 bytes.
    _In_reads_( cbNonce )           PCBYTE    pbNonce,
                                    SIZE_T    cbNonce,    // Required. Nonce size MUST be 12 bytes.
    _In_reads_opt_( cbAuthData )    PCBYTE    pbAuthData,
                                    SIZE_T    cbAuthData, // Optional. Can be any size.
    _In_reads_( cbData )            PCBYTE    pbSrc,
    _Out_writes_( cbData )          PBYTE     pbDst,
                                    SIZE_T    cbData,     // Required. Max size is 274,877,906,880 bytes.
    _Out_writes_( cbTag )           PBYTE     pbTag,
                                    SIZE_T    cbTag );    // Required. Tag size MUST be 16 bytes.

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Decrypt(
    _In_reads_( cbKey )             PCBYTE    pbKey,
                                    SIZE_T    cbKey,      // Required. Key size MUST be 32 bytes.
    _In_reads_( cbNonce )           PCBYTE    pbNonce,
                                    SIZE_T    cbNonce,    // Required. Nonce size MUST be 12 bytes.
    _In_reads_opt_( cbAuthData )    PCBYTE    pbAuthData,
                                    SIZE_T    cbAuthData, // Optional. Can be any size.
    _In_reads_( cbData )            PCBYTE    pbSrc,
    _Out_writes_( cbData )          PBYTE     pbDst,
                                    SIZE_T    cbData,     // Required. Max size is 274,877,906,880 bytes.
    _In_reads_( cbTag )             PCBYTE    pbTag,
                                    SIZE_T    cbTag );    // Required. Tag size MUST be 16 bytes.

VOID
SYMCRYPT_CALL
SymCryptChaCha20Poly1305Selftest(void);

////////////////////////////////////////////////////////////////////////////
//   MARVIN32
//
// Marvin is a checksum function optimized for speed on small inputs.
// IT IS NOT A CRYPTOGRAPHIC HASH FUNCTION.
// Marvin lacks the security properties of a cryptographic hash function.
// DO NOT USE FOR ANY SECURITY USE.
//
// A randomizable checksum function has essentially the same API as a MAC
// function. We use the SymCrypt MAC API here, with the difference
// that we use the word 'seed' rather than 'key'.
//
// See the description above of the generic MAC API for details on how
// these functions are used. Wherever the MAC API talks about keys, this
// applies to the seed for Marvin32.
//
// The randomization is useful for hash tables.
// There are DOS attacks where an attacker generates many inputs that
// hash to the same location in the hash table. Some hash table implementations
// then use O(n^2) CPU time, allowing a DOS attack.
// The randomization provided by the seed avoids this attack if:
// - The seed is unpredictable and unknown to the attacker.
// - The attacker cannot learn information about the output of the checksum function.
// In particular, if an attacker can measure how long it takes to add each
// element in a hash table, they might be able to determine enough information about
// the output of the checksum function to recover the seed. Of course,
// once that is done the DOS attack is once again possible.
//
// SymCrypt provides a default seed for applications that don't need a seed.
//
// FUTURE IMPROVEMENTS:
// At the moment it is relatively expensive to change the seed.
// If needed, we can add a facility to modify the seed faster than
// re-running the ExpandSeed function.
//

#define SYMCRYPT_MARVIN32_RESULT_SIZE       (8)
#define SYMCRYPT_MARVIN32_SEED_SIZE         (8)
#define SYMCRYPT_MARVIN32_INPUT_BLOCK_SIZE  (4)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMarvin32ExpandSeed(
    _Out_               PSYMCRYPT_MARVIN32_EXPANDED_SEED    pExpandedSeed,
    _In_reads_(cbSeed)  PCBYTE                              pbSeed,
                        SIZE_T                              cbSeed );
//
// The seed must be 8 bytes (= SYMCRYPT_MARVIN32_SEED_SIZE).
// Use of the all-zero seed is not recommended as it has some undesirable properties.
// Note that a pre-expanded default seed is provided for applications that do not wish to control
// their seed. Such applications do not need to call SymCryptMarvin32ExpandSeed
//

extern PCSYMCRYPT_MARVIN32_EXPANDED_SEED const SymCryptMarvin32DefaultSeed;

PCSYMCRYPT_MARVIN32_EXPANDED_SEED
SYMCRYPT_CALL
SymCryptGetMarvin32DefaultSeed( void );
//
// Returns a pointer to the default Marvin32 seed.
//

VOID
SYMCRYPT_CALL
SymCryptMarvin32SeedCopy(   _In_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pSrc,
                            _Out_   PSYMCRYPT_MARVIN32_EXPANDED_SEED    pDst );

VOID
SYMCRYPT_CALL
SymCryptMarvin32(
    _In_                                            PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed,
    _In_reads_( cbData )                            PCBYTE                              pbData,
                                                    SIZE_T                              cbData,
    _Out_writes_( SYMCRYPT_MARVIN32_RESULT_SIZE )   PBYTE                               pbResult );
//
// If the application does not wish to use a seed, a default expanded seed is provided.
// Callers can pass SymCryptMarvin32DefaultSeed as the first argument.
//

VOID
SYMCRYPT_CALL
SymCryptMarvin32StateCopy(
    _In_        PCSYMCRYPT_MARVIN32_STATE           pSrc,
    _In_opt_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed,
    _Out_       PSYMCRYPT_MARVIN32_STATE            pDst );


VOID
SYMCRYPT_CALL
SymCryptMarvin32Init(   _Out_   PSYMCRYPT_MARVIN32_STATE            pState,
                        _In_    PCSYMCRYPT_MARVIN32_EXPANDED_SEED   pExpandedSeed);

VOID
SYMCRYPT_CALL
SymCryptMarvin32Append(     _Inout_                 PSYMCRYPT_MARVIN32_STATE    pState,
                            _In_reads_( cbData )    PCBYTE                      pbData,
                                                    SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptMarvin32Result(
     _Inout_                                        PSYMCRYPT_MARVIN32_STATE    pState,
     _Out_writes_( SYMCRYPT_MARVIN32_RESULT_SIZE )  PBYTE                       pbResult );


VOID
SYMCRYPT_CALL
SymCryptMarvin32Selftest(void);


//==========================================================================
//   BLOCK CIPHERS
//==========================================================================
//
// For a block cipher XXX the following minimal functions, types, and constants are defined:
//
// SYMCRYPT_XXX_BLOCK_SIZE
//
//      A constant giving is the block size, in bytes, of the algorithm.
//
//
// SYMCRYPT_XXX_EXPANDED_KEY
//      Type which contains a key with all the pre-computations performed.
//      This is an opaque type whose structure can change at will.
//      It should only be used for transient computations in a single executable
//      and not be stored or transferred to a different environment.
//      The pointer and const-pointer versions are also declared
//      (PSYMCRYPOT_XXX_EXPANDED_KEY and PCSYMCRYPT_XXX_EXPANDED_KEY).
//
//      The EXPANDED_KEY structure contains keying material and should be wiped
//      once it is no longer used. (See SymCryptWipe & SymCryptWipeKnownSize)
//
//      Once initialized, multiple threads can use the same expanded key object simultaneously
//      for different block cipher computations as the expanded key is not modified once initialized.
//
// SymCryptXxxBlockCipher
//      A SYMCRYPT_BLOCKCIPHER structure that provides a description
//      of the block cipher and its primary functions. This is used by cipher modes to pass
//      all the block-cipher specific information in a single structure.
//
//
// SYMCRYPT_ERROR
// SYMCRYPT_CALL
// SymCryptXxxExpandKey(    _Out_               PSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//                          _In_reads_(cbKey)   PCBYTE                     pbKey,
//                                              SIZE_T                     cbKey );
//
//      Prepare a key for future use by the Xxx algorithm.
//      This function performs pre-computations on the key
//      to speed up the actual block cipher computations later, and stores the result as an expanded key.
//      The expanded key must be kept unchanged until all computations that use the key are finished.
//      When the key is no longer needed the expanded key structure should be wiped.
//
//      Different algorithms pose different requirements on the length of the key.
//      If the key that is provided is of an unsupported length the SYMCRYPT_WRONG_KEY_SIZE error is returned.
//      In this case the expanded key structure will not contain any keying material and does not have to be wiped.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxEncrypt(  _In_                                    PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//                      _In_reads_( SYMCRYPT_XXX_BLOCK_SIZE )   PCBYTE                      pbSrc,
//                      _Out_writes_( SYMCRYPT_XXX_BLOCK_SIZE ) PBYTE                       pbDst );
//
//      Encrypt a single block.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxDecrypt(  _In_                                    PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//                      _In_reads_( SYMCRYPT_XXX_BLOCK_SIZE )   PCBYTE                      pbSrc,
//                      _Out_writes_( SYMCRYPT_XXX_BLOCK_SIZE ) PBYTE                       pbDst );
//
//      Decrypt a single block.
//
//
// --------------------------------------------------------------------------------------------------------------
//      In addition to these elementary encrypt block/decrypt block functions a block cipher may also implement
//      optimized versions of CBC encryption, CBC decryption, CBC-MAC, and CTR encryption. Not all block ciphers
//      do implement these.
//      All block cipher modes are always available through the generic block cipher mode functions.
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxCbcEncrypt(
//      _In_                                        PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//      _Inout_updates_( SYMCRYPT_XXX_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//      _In_reads_( cbData )                        PCBYTE                      pbSrc,
//      _Out_writes_( cbData )                      PBYTE                       pbDst,
//                                                  SIZE_T                      cbData );
//
//      Encrypt data using the CBC chaining mode.
//      On entry the pbChainingValue is the IV which is xorred into the first plaintext block of the CBC encryption.
//      On exit the pbChainingValue is updated to the last ciphertext block of the result.
//      This allows a longer CBC encryption to be done incrementally.
//
//      cbData must be a multiple of the block size. For efficiency reasons this routine does not return an error
//      if cbData is not a proper multiple; instead the result is undefined. The routine might hang,
//      round cbData down to a multiple of the block size, or return random data that cannot be decrypted.
//
//      The pbSrc and pbDst buffers may be the same, or they may be non-overlapping. However, they may
//      not be partially overlapping.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxCbcDecrypt(
//      _In_                                        PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//      _Inout_updates_( SYMCRYPT_XXX_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//      _In_reads_( cbData )                        PCBYTE                      pbSrc,
//      _Out_writes_( cbData )                      PBYTE                       pbDst,
//                                                  SIZE_T                      cbData );
//
//      Decrypt data using the CBC chaining mode.
//      On entry the pbChainingValue is the IV to be xorred into the first plaintext block of the CBC decryption.
//      On exit the pbChainingValue is updated to the last ciphertext block of the input.
//      This allows a longer CBC decryption to be done incrementally.
//
//      cbData must be a multiple of the block size. For efficiency reasons this routine does not return an error
//      if cbData is not a proper multiple; instead the result is undefined. The routine might hang,
//      round cbData down to a multiple of the block size, or return random data.
//
//      The pbSrc and pbDst buffers may be the same, or they may be non-overlapping. However, they may
//      not be partially overlapping.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxCbcMac(
//      _In_                                        PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//      _Inout_updates_( SYMCRYPT_XXX_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//      _In_reads_( cbData )                        PCBYTE                      pbData,
//                                                  SIZE_T                      cbData );
//
//      Compute a CBC-MAC on the input data.
//      On entry the pbChainingValue is the current chaining state of the CBC-MAC computation; this routine
//      updates the state to reflect the chaining state after MACing the data.
//      cbData must be a multiple of the block size.
//      This function is NOT intended for general use; rather it is a high-performance primitive to support
//      implementations of other cipher modes like CCM and CMAC.
//      Note: If a key is used for CBC-MAC computations it should NOT be used for any encryptions.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxCtrMsb64(
//      _In_                                        PCSYMCRYPT_XXX_EXPANDED_KEY pExpandedKey,
//      _Inout_updates_( SYMCRYPT_XXX_BLOCK_SIZE )  PBYTE                       pbChainingValue,
//      _In_reads_( cbData )                        PCBYTE                      pbSrc,
//      _Out_writes_( cbData )                      PBYTE                       pbDst,
//                                                  SIZE_T                      cbData );
//
//      Perform a CTR encryption on the data. (Note: CTR encryption and decryption are the same operation.)
//      On entry pbChainingValue contains the first counter value to be used. On exit it contains
//      the next counter value to be used.
//      The increment function treats the last 8 bytes of the pbChainingValue string as an integer
//      in most-significant-byte-first format, and increments this integer.
//      Thus, the last byte is incremented the fastest.
//      The pbSrc and pbDst buffers may be identical or non-overlapping, but they may not partially overlap.
//      cbData must be a multiple of the block size.
//
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxSelftest(void);
//
//      Perform a minimal self-test on the XXX algorithm.
//      This function is designed to be used for achieving FIPS 140-2 compliance or
//      to provide a simple self-test when an application starts.
//
//      If an error is detected the fatal callback routine is called.
//
//      We do not provide self-tests for the various cipher modes. There are too many
//      (block cipher, key size, cipher mode) combinations and CNG performs the self tests
//      on the outside APIs, not on the internal APIs.
//      We retain a self test on the basic algorithm to help internal library testing.



////////////////////////////////////////////////////////////////////////////
//   AES
//
// The AES block cipher per FIPS 197
//
// WARNING:
// Unless this code is running on a CPU with AES-NI instructions,
// the AES implementation makes extensive use of table lookups to implement the S-boxes of the algorithm.
// This violates our current crypto implementation guidelines and opens up a possible side-channel attack
// through information leakage via the memory caching system of the CPU.
//
// Unfortunately there is no known software fix for this that does not lead to an order of magnitude performance loss.
// An implementation that is 10x slower will not be used by anybody and is useless, so we implement a fast
// version that uses table lookups. (Just like all other systems we know of.)
//
// The risk of this type of side-channel attack is limited as it requires malicious code to run on the same
// machine as the code being attacked.
//
// At the time of writing (Apr 2007) there are no approved alternative encryption algorithms that do not
// use table lookups. NIST and NSA are aware of this problem, but so far we have not seen any indication
// that they consider this important enough to create an alternative encryption algorithm that does not
// rely on table lookups as much.
//

#define SYMCRYPT_AES_BLOCK_SIZE  (16)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesExpandKey(
    _Out_               PSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );

//
// The SymCryptAesExpandKeyEncryptOnly creates an AES-expanded key that can ONLY be used
// for AES encryption operations. There are no safeguards when you use it for decryption; you get the wrong
// result if you try.
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesExpandKeyEncryptOnly(
    _Out_               PSYMCRYPT_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );

VOID
SYMCRYPT_CALL
SymCryptAesKeyCopy( _In_ PCSYMCRYPT_AES_EXPANDED_KEY pSrc,
                    _Out_ PSYMCRYPT_AES_EXPANDED_KEY pDst );

VOID
SYMCRYPT_CALL
SymCryptAesEncrypt(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesDecrypt(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptAesEcbEncrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesEcbDecrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcEncrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcDecrypt(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCbcMac(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbData,
                                                SIZE_T                      cbData );

VOID
SYMCRYPT_CALL
SymCryptAesCtrMsb64(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData );

//
// There are many optimized implementations for various AES modes.
// To test them all would pull in all the code for these modes.
// We solve this by letting the caller specify a bitmask of modes to be tested.
// Under the following circumstances this will avoid pulling in unnecessary code:
// - The argument is a compile-time constant.
// - The compiler implements the usual constant propagation optimizations.
//
// Note: GCM, CCM, and XTS are NOT tested by this function.

#define SYMCRYPT_AES_SELFTEST_BASE      0x01        // tests AesEncrypt & AesDecrypt
#define SYMCRYPT_AES_SELFTEST_ECB       0x02        // ECB mode
#define SYMCRYPT_AES_SELFTEST_CBC       0x04        // CBC mode
#define SYMCRYPT_AES_SELFTEST_CBCMAC    0x08        // CBC-mac
#define SYMCRYPT_AES_SELFTEST_CTR       0x10        // all CTR modes

#define SYMCRYPT_AES_SELFTEST_ALL       0x1f

VOID
SYMCRYPT_CALL
SymCryptAesSelftest( UINT32 maskTestsToRun );

extern const PCSYMCRYPT_BLOCKCIPHER SymCryptAesBlockCipher;


////////////////////////////////////////////////////////////////////////////
//   DES
//
// The DES block cipher per FIPS-46-3
//
// WARNING:
// DES is no longer considered secure and should not be used.
// Per the Crypto SDL, any use of DES in Microsoft code requires a Crypto board exemption
//
// The DES implementation makes extensive use of table lookups to implement the S-boxes of the algorithm.
// This violates our current crypto implementation guidelines and opens up a possible side-channel attack
// through information leakage via the memory caching system of the CPU.
//

#define SYMCRYPT_DES_BLOCK_SIZE  (8)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDesExpandKey(
    _Out_               PSYMCRYPT_DES_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );
//
// The key must be 8 bytes long. The parity bits in the key are ignored and can be any value.
//

VOID
SYMCRYPT_CALL
SymCryptDesEncrypt(
    _In_                                    PCSYMCRYPT_DES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_DES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_DES_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptDesDecrypt(
    _In_                                    PCSYMCRYPT_DES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_DES_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_DES_BLOCK_SIZE ) PBYTE                       pbDst );


VOID
SYMCRYPT_CALL
SymCryptDesSetOddParity(
    _Inout_updates_( cbData ) PBYTE   pbData,
    _In_                            SIZE_T  cbData );
//
// Set each byte to have odd parity by possibly flipping bit 0.
// This is the parity used by DES, and is needed for compatibility.
// The parity bit is ignored by the DES key expansion.
//

VOID
SYMCRYPT_CALL
SymCryptDesSelftest(void);

extern const PCSYMCRYPT_BLOCKCIPHER SymCryptDesBlockCipher;

////////////////////////////////////////////////////////////////////////////
//   3DES
//
// The triple-DES block cipher
//
// WARNING:
// The DES implementation makes extensive use of table lookups to implement the S-boxes of the algorithm.
// This violates our current crypto implementation guidelines and opens up a possible side-channel attack
// through information leakage via the memory caching system of the CPU.
//

#define SYMCRYPT_3DES_BLOCK_SIZE  (8)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCrypt3DesExpandKey(
    _Out_               PSYMCRYPT_3DES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );
//
// If the provided key is 24 bytes long this expands a 3-key 3DES key. If 16 bytes are provided it
// expands a 2-key 3DES. If 8 bytes are provided it creates the 3-key equivalent of the single
// key des encryption. The parity bits in the key are ignored.
//

VOID
SYMCRYPT_CALL
SymCrypt3DesEncrypt(
    _In_                                    PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_3DES_BLOCK_SIZE )  PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_3DES_BLOCK_SIZE )PBYTE                           pbDst );

VOID
SYMCRYPT_CALL
SymCrypt3DesDecrypt(
    _In_                                    PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_3DES_BLOCK_SIZE )  PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_3DES_BLOCK_SIZE )PBYTE                           pbDst );

VOID
SYMCRYPT_CALL
SymCrypt3DesCbcEncrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _Inout_updates_( SYMCRYPT_3DES_BLOCK_SIZE ) PBYTE                           pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                          pbSrc,
    _Out_writes_( cbData )                      PBYTE                           pbDst,
                                                SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCrypt3DesCbcDecrypt(
    _In_                                        PCSYMCRYPT_3DES_EXPANDED_KEY    pExpandedKey,
    _Inout_updates_( SYMCRYPT_3DES_BLOCK_SIZE ) PBYTE                           pbChainingValue,
    _In_reads_( cbData )                        PCBYTE                          pbSrc,
    _Out_writes_( cbData )                      PBYTE                           pbDst,
                                                SIZE_T                          cbData );

VOID
SYMCRYPT_CALL
SymCrypt3DesSelftest(void);

extern const PCSYMCRYPT_BLOCKCIPHER SymCrypt3DesBlockCipher;

////////////////////////////////////////////////////////////////////////////
//   DESX
//
// The DESX block cipher.
//
// Use of DESX is not recommended.
//

#define SYMCRYPT_DESX_BLOCK_SIZE  (8)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDesxExpandKey(
    _Out_               PSYMCRYPT_DESX_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );

VOID
SYMCRYPT_CALL
SymCryptDesxEncrypt(
    _In_                                    PCSYMCRYPT_DESX_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_DESX_BLOCK_SIZE )  PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_DESX_BLOCK_SIZE )PBYTE                           pbDst );

VOID
SYMCRYPT_CALL
SymCryptDesxDecrypt(
    _In_                                    PCSYMCRYPT_DESX_EXPANDED_KEY    pExpandedKey,
    _In_reads_( SYMCRYPT_DESX_BLOCK_SIZE )  PCBYTE                          pbSrc,
    _Out_writes_( SYMCRYPT_DESX_BLOCK_SIZE )PBYTE                           pbDst );


VOID
SYMCRYPT_CALL
SymCryptDesxSelftest(void);

extern const PCSYMCRYPT_BLOCKCIPHER SymCryptDesxBlockCipher;

////////////////////////////////////////////////////////////////////////////
//   RC2
//
// The RC2 block cipher
//
// WARNING:
// Use of RC2 is not recommended for many reasons.
//
// The RC2 implementation makes extensive use of table lookups to implement the S-boxes of the algorithm.
// This violates our current crypto implementation guidelines and opens up a possible side-channel attack
// through information leakage via the memory caching system of the CPU.
//

#define SYMCRYPT_RC2_BLOCK_SIZE  (8)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc2ExpandKey(
    _Out_               PSYMCRYPT_RC2_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey );
//
// The default effective key size is 8*cbKey. Note that this is NOT the default used in
// the old RSA32 library which used a default effective key size of 40 bits.
// That is too dangerous a default to implement. We chose 8*cbKey rather than 1024 as
// our choice provides slightly better mixing of the key bytes into the expanded key.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc2ExpandKeyEx(
    _Out_               PSYMCRYPT_RC2_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                      pbKey,
                        SIZE_T                      cbKey,
                        UINT32                      effectiveKeySizeInBits );
//
// Rc2 has an option to limit the effective key size, which means the key expansion function has an extra
// parameter.
//
// The effective key size in bits may be any value from 9..1024. If it is larger than 8*cbKey it does
// not significantly affect the key strength. However, the expanded key will always depend on the
// effective key size; expanding the same string of key bytes with differ effective key sizes leads
// to different expanded keys and different encryption functions.
//
// The original default was an effective key size of 40 bits.
//
// Do not allow your attacker to choose the effective key size. RC2 seems vulnerable to
// related-effective-key-size attacks.
//

VOID
SYMCRYPT_CALL
SymCryptRc2Encrypt(
    _In_                                    PCSYMCRYPT_RC2_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_RC2_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_RC2_BLOCK_SIZE ) PBYTE                       pbDst );

VOID
SYMCRYPT_CALL
SymCryptRc2Decrypt(
    _In_                                    PCSYMCRYPT_RC2_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_RC2_BLOCK_SIZE )   PCBYTE                      pbSrc,
    _Out_writes_( SYMCRYPT_RC2_BLOCK_SIZE ) PBYTE                       pbDst );


VOID
SYMCRYPT_CALL
SymCryptRc2Selftest(void);

extern const PCSYMCRYPT_BLOCKCIPHER SymCryptRc2BlockCipher;


//==========================================================================
//   BLOCK CIPHER MODES
//==========================================================================
//
// Block cipher modes use the block cipher description tables to implement
// the various modes in a block-cipher independent way.
//
// Some block ciphers implement optimized versions of the block cipher modes.
// These functions call that optimized version, but calling the block-cipher specific
// function has less overhead.
//
// Note that these functions will only work with SymCrypt-provided block ciphers.
// They are not designed to be used with externally provided block ciphers.
// (The SYMCRYPT_BLOCKCIPHER structure is a private one not available to callers.)
//

typedef enum _SYMCRYPT_BLOCKCIPHER_ID
{
    SYMCRYPT_BLOCKCIPHER_ID_NULL        = 0,
    SYMCRYPT_BLOCKCIPHER_ID_AES         = 1,
    SYMCRYPT_BLOCKCIPHER_ID_DES         = 2,
    SYMCRYPT_BLOCKCIPHER_ID_3DES        = 3,
    SYMCRYPT_BLOCKCIPHER_ID_DESX        = 4,
    SYMCRYPT_BLOCKCIPHER_ID_RC2         = 5
} SYMCRYPT_BLOCKCIPHER_ID;

PCSYMCRYPT_BLOCKCIPHER
SYMCRYPT_CALL
SymCryptGetBlockCipher( SYMCRYPT_BLOCKCIPHER_ID blockCipherId );
//
// Returns a pointer to the block cipher structure for the specified block cipher ID.
// Returns NULL if the block cipher ID is invalid.
//

VOID
SYMCRYPT_CALL
SymCryptEcbEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// Generic ECB encryption routine for block ciphers.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbSrc is the plaintext input buffer. The plaintext and ciphertext buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbData. Number of bytes to encrypt. This must be a multiple of the block size.
// - pbDst is the result buffer. It may be identical to pbPlaintext or non-overlapping,
//      but it may not partially overlap with the pbPlaintext buffer.
//

VOID
SYMCRYPT_CALL
SymCryptEcbDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// Generic ECB decryption routine for block ciphers.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbSrc is the plaintext input buffer. The plaintext and ciphertext buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbData. Number of bytes to encrypt. This must be a multiple of the block size.
// - pbDst is the result buffer. It may be identical to pbPlaintext or non-overlapping,
//      but it may not partially overlap with the pbPlaintext buffer.
//


VOID
SYMCRYPT_CALL
SymCryptCbcEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );

//
// Generic CBC encryption routine for block ciphers.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbChainingValue points to the chaining value. On entry it is the IV value for the CBC
//      encryption, on return it is the last ciphertext block. A long message can be encrypted
//      piecewise in multiple calls; at the end of one call the pbChainingValue buffer will contain
//      the correct chaining value for encrypting the next piece of the message.
//      Once the encryption is finished the value in the chaining buffer is no longer needed.
// - pbSrc is the plaintext input buffer. The plaintext and ciphertext buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbData. Number of bytes to encrypt. This must be a multiple of the block size.
// - pbDst is the result buffer. It may be identical to pbPlaintext or non-overlapping,
//      but it may not partially overlap with the pbPlaintext buffer.
//


VOID
SYMCRYPT_CALL
SymCryptCbcDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );

//
// This is the decryption version of SymCryptCbcEncrypt.
// All parameters have the same explanation and restrictions.:
//


VOID
SYMCRYPT_CALL
SymCryptCbcMac(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
                                SIZE_T                  cbData );
//
// This function implements the same function as SymCryptCbcEncrypt except that
// it does not produce a ciphertext output.
// All other restrictions apply.
// The pbChainingValue is the only output provided.
//
// This is the primitive operation used by other modes of operation,
// and some platforms have special optimizations for this primitive.
// As we expose special APIs for some algorithms, we provide the generic function so that it
// can be used for all algorithms.
//


VOID
SYMCRYPT_CALL
SymCryptCtrMsb64(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// This function implements the CTR cipher mode.
// It is not intended to be used as-is, rather it is a building block for modes like CCM.
// On some platforms we have optimized code for AES-CTR, on other platforms
// we use this generic construction to achieve the same effect.
//
// Note that in CTR mode encryption and decryption are the same operation.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbChainingValue points to the chaining value. On entry it is the first counter value to be
//      used. On exit is the next counter value to be used.
//      The pbChainingValue is incremented by cbData/blockSize.
//      The increment function treats the last 8 bytes of pbChaining a MSBfirst integer
//      and increments the integer representation by one for each block.
// - pbSrc is the input data buffer that will be encrypted/decrypted.
// - cbData. Number of bytes to encrypt/decrypt. This must be a multiple of the block size.
// - pbDst is the output buffer that receives the encrypted/decrypted data. The input and output
//      buffers may be the same or non-overlapping, but may not partially overlap.
//

VOID
SYMCRYPT_CALL
SymCryptCfbEncrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
                                SIZE_T                  cbShift,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// Encrypt a buffer using the CFB cipher mode.
//
// This implements the CFB mode, with selected shift amount (in bytes).
// In general, one block cipher encryption is used for each cbShift bytes
// of plaintext, which can be slow.
// Use of this cipher mode is not recommended.
//
// - pBlockCipher is a pointer to the block cipher description table.
//      Suitable description tables for all ciphers in this library have been pre-defined.
// - cbShift is the shift value (in bytes) of the CFB mode.
//      The only supported values are 1 and the block size.
// - pExpandedKey points to the expanded key to use. This generic function uses PVOID so there
//      is no type safety to ensure that the expanded key and the encryption function match.
// - pbChainingValue points to the chaining value. On entry and exit it
//      contains the last blockSize ciphertext bytes.
// - pbSrc is the input data buffer that will be encrypted/decrypted.
// - cbData. Number of bytes to encrypt/decrypt.
//      Must be a multiple of cbShift, or a multiple of the block size if cbShift = 0.
// - pbDst is the output buffer that receives the encrypted/decrypted data. The input and output
//      buffers may be the same or non-overlapping, but may not partially overlap.
//

VOID
SYMCRYPT_CALL
SymCryptCfbDecrypt(
    _In_                        PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
                                SIZE_T                  cbShift,
    _In_                        PCVOID                  pExpandedKey,
    _Inout_updates_( pBlockCipher->blockSize )
                                PBYTE                   pbChainingValue,
    _In_reads_( cbData )        PCBYTE                  pbSrc,
    _Out_writes_( cbData )      PBYTE                   pbDst,
                                SIZE_T                  cbData );
//
// The corresponding decryption routine.
//

VOID
SYMCRYPT_CALL
SymCryptPaddingPkcs7Add(
                                            SIZE_T  cbBlockSize,
    _In_reads_(cbSrc)                       PCBYTE  pbSrc,
                                            SIZE_T  cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)      PBYTE   pbDst,
                                            SIZE_T  cbDst,
                                            SIZE_T* pcbResult);
//
// Prerequisites:
//  cbBlockSize is a power of 2 and < 256
//  cbDst >= cbSrc - cbSrc % cbBlockSize + cbBlockSize
//
// Add PKCS7 block padding to a message
// The input data (pbSrc,cbSrc) is padded with between 1 and cbBlockSize bytes so that
// the length of the result is a multiple of cbBlockSize.
// The padded message is written to the pbDst buffer.
// The length of the padded message is returned in *pcbResult.
//
// If pbSrc == pbDst this function avoids copying all the data.
// Note that cbSrc == cbDst is not valid as it violates the prerequisites.
// Padding a message with cbSrc == 0 is valid.
//
// Note:
// Any whole blocks in Src are merely copied to Dst.
// Callers can either process the whole message in this call,
// or handle the whole blocks themselves and only pass the last few bytes of the message to this function.
//
// Note: the prerequisites are not checked by this function; if they are not satisfied
// the behaviour of the function is undefined.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPaddingPkcs7Remove(
                                            SIZE_T  cbBlockSize,
    _In_reads_(cbSrc)                       PCBYTE  pbSrc,
                                            SIZE_T  cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)      PBYTE   pbDst,
                                            SIZE_T  cbDst,
                                            SIZE_T* pcbResult);
//
// Prerequisites:
//  - cbBlockSize is a power of 2 and < 256
//  - cbSrc is a multiple of cbBlockSize
//  - cbSrc is greater than zero (at least equals to cbBlockSize)
//
// Remove PKCS7 block padding from a message in a side-channel safe way.
//  *** see below for important rules the caller should follow w.r.t. side-channel safety ***
// The input data (pbSrc, cbSrc) is a valid PKCS7 padded message for the given blocksize.
// This function removes the padding, copies the result to the (pbDst, cbDst) buffer,
// and returns the size of the result in *pcbResult.
//
// This function only supports padding with a size up to the block size.
//
// If pbSrc == pbDst this function avoids copying data.
//
// The following errors are returned:
//  - SYMCRYPT_INVALID_ARGUMENT if cbSrc or the padding is invalid
//  - SYMCRYPT_BUFFER_TOO_SMALL if cbDst < size of the unpadded message
// If cbDst >= cbSrc the SYMCRYPT_BUFFER_TOO_SMALL error will not be returned.
// Even if an error is returned, the pbDst buffer may or may not contain data from the message.
// Callers should wipe the buffer even if an error is returned.
//
// Note: Removal of PKCS7 padding is extremely sensitive to side channels.
// For example, if a message is encrypted with AES-CBC and the attacker can modify
// the ciphertext and then determine whether a padding error occurs during decryption,
// then the attacker can use the presence or absence of the error to decrypt the message itself.
// This function takes great care not to reveal whether an error occurred, and hides
// the size of the unpadded message. This is even true when writing to pbDst. If cbDst is large
// enough, the code will write cbSrc-1 bytes to pbDst, using masking to only update the bytes of the
// message and leaving the other bytes in pbDst unchanged.
// Callers should take great care not to reveal the returned error or success,
// or the size of the returned message, until they have authenticated
// the source of the data.
//
// In particular, any mapping of the error code should be done in a side-channel safe way.
// See the SymCryptMapUint32() function for a side-channel safe way to map error codes.
//
// The error caused by an invalid cbSrc value is not hidden from side channels as this does not reveal any
// secret information.
//
// Note: callers can either process the whole message in this call,
// or process the whole blocks themselves and only pass the last block to this function.

////////////////////////////
// CCM
////////////////////////////

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmValidateParameters(
    _In_    PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_    SIZE_T                  cbNonce,
    _In_    SIZE_T                  cbAssociatedData,
    _In_    UINT64                  cbData,
    _In_    SIZE_T                  cbTag
   );
//
// To achieve maximum performance, CCM functions do not check for valid parameters.
// Passing invalid parameters can lead to buffer overflows.
// Callers who want to validate their CCM parameters can call this function.
// Note: In Checked builds some CCM functions might fatal out when invalid parameters are
// passed.
//


VOID
SYMCRYPT_CALL
SymCryptCcmEncrypt(
     _In_                           PCSYMCRYPT_BLOCKCIPHER     pBlockCipher,
     _In_                           PCVOID                     pExpandedKey,
     _In_reads_( cbNonce )          PCBYTE                     pbNonce,
                                    SIZE_T                     cbNonce,
     _In_reads_opt_( cbAuthData )   PCBYTE                     pbAuthData,
                                    SIZE_T                     cbAuthData,
     _In_reads_( cbData )           PCBYTE                     pbSrc,
     _Out_writes_( cbData )         PBYTE                      pbDst,
                                    SIZE_T                     cbData,
     _Out_writes_( cbTag )          PBYTE                      pbTag,
                                    SIZE_T                     cbTag );

//
//  Encrypt a buffer using the block cipher in CCM mode.
//      - pBlockCipher points to the block cipher description table.
//      - pExpandedKey points to the expanded key for the block cipher.
//      - pbNonce: Pointer to the nonce for this encryption. For a single key, each nonce
//          value may be used at most once to encrypt data. Re-using nonce values leads
//          to catastrophic loss of security.
//      - cbNonce: number of bytes in the nonce: 7 <= cbNonce <= 13.
//      - pbAuthData: pointer to the associated authentication data. This data is not encrypted
//          but it is included in the authentication. Use NULL if not used.
//      - cbAuthData: # bytes of associated authentication data. (0 if not used)
//      - pbSrc: plaintext input
//      - pbDst: ciphertext output. The ciphertext buffer may be identical to the plaintext
//          buffer, or non-overlapping. The ciphertext is also cbData bytes long.
//      - cbData: # bytes of plaintext input. The maximum length is 2^{8(15-cbNonce)} - 1 bytes.
//      - pbTag: buffer that will receive the authentication tag.
//      - cbTag: size of tag. cbTag must be one of {4, 6, 8, 10, 12, 14, 16}.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmDecrypt(
     _In_                           PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
     _In_                           PCVOID                  pExpandedKey,
     _In_reads_( cbNonce )          PCBYTE                  pbNonce,
                                    SIZE_T                  cbNonce,
     _In_reads_opt_( cbAuthData )   PCBYTE                  pbAuthData,
                                    SIZE_T                  cbAuthData,
     _In_reads_( cbData )           PCBYTE                  pbSrc,
     _Out_writes_( cbData )         PBYTE                   pbDst,
                                    SIZE_T                  cbData,
     _In_reads_( cbTag )            PCBYTE                  pbTag,
                                    SIZE_T                  cbTag );
//
// Decrypt a buffer using the block cipher in CCM mode.
// See SymCryptCcmEncrypt for a description of the parameters. This function decrypts rather than
// encrypts, and as a result the pbTag parameter is read rather than filled.
//
// If the tag value is not correct the SYMCRYPT_AUTHENTICATION_FAILURE error is returned and the pbDst buffer
// is wiped of any plaintext.
// Note: While checking the authentication the purported plaintext is stored in pbDst. It is not safe to reveal
// purported plaintext when the authentication has not been checked. (Doing so would reveal key stream information
// that can be used to decrypt any message encrypted with the same nonce value.) Thus, users should be careful
// to not reveal the pbDst buffer until this function returns (e.g. through other threads or sharing memory).
//

//
// We also provide functions for incremental computation of CCM encryption and decryption. See the functions
// above for a description of the parameters and restrictions.
// In particular, note that the restriction on revealing the plaintext for unauthenticated decryptions holds
// for all the decrypted data, even when the decryption is done incrementally.
//
// SYMCRYPT_CCM_STATE
//      Ongoing state of an incremental CCM encryption or decryption operation.
//

VOID
SYMCRYPT_CALL
SymCryptCcmInit(
    _Out_                           PSYMCRYPT_CCM_STATE     pState,
    _In_                            PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_                            PCVOID                  pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                  pbNonce,
                                    SIZE_T                  cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                  pbAuthData,
                                    SIZE_T                  cbAuthData,
                                    UINT64                  cbData,
                                    SIZE_T                  cbTag );
//
// Initialize a CCM computation. Note that the ultimate data length has to be provided.
// The pBlockCipher and pExpandedKey structures must remain unchanged until the CCM computation is finished.
//

VOID
SYMCRYPT_CALL
SymCryptCcmEncryptPart(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptCcmEncryptFinal(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _Out_writes_( cbTag )   PBYTE               pbTag,
                            SIZE_T              cbTag );
//
// Note: passing cbTag is redundant but necessary for SAL purposes.
//

VOID
SYMCRYPT_CALL
SymCryptCcmDecryptPart(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCcmDecryptFinal(
    _Inout_                 PSYMCRYPT_CCM_STATE pState,
    _In_reads_( cbTag )     PCBYTE              pbTag,
                            SIZE_T              cbTag );
//
// WARNING: When the authentication fails the data already decrypted may not be revealed.
// This function cannot wipe the plaintext buffers; the caller is responsible for ensuring
// the plaintext is not revealed.
//

VOID
SYMCRYPT_CALL
SymCryptCcmSelftest(void);
//
// Self test for CCM cipher mode
//

///////////////////////////////////////
// GCM
///////////////////////////////////////
//
// The GCM algorithm per SP 800-38D.
// GMAC is just GCM with an empty data string; all the data is put in the pbAuthData buffer.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmValidateParameters(
    _In_    PCSYMCRYPT_BLOCKCIPHER  pBlockCipher,
    _In_    SIZE_T                  cbNonce,
    _In_    UINT64                  cbAssociatedData,
    _In_    UINT64                  cbData,
    _In_    SIZE_T                  cbTag
   );
//
// To achieve maximum performance, GCM functions do not check for valid parameters.
// Passing invalid parameters can lead to buffer overflows.
// Callers who want to validate their GCM parameters can call this function.
// Note: In Checked builds some GCM functions might fatal out when invalid parameters are
// passed.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmExpandKey(
    _Out_                   PSYMCRYPT_GCM_EXPANDED_KEY  pExpandedKey,
    _In_                    PCSYMCRYPT_BLOCKCIPHER      pBlockCipher,
    _In_reads_( cbKey )     PCBYTE                      pbKey,
                            SIZE_T                      cbKey );
//
// Create an expanded key suitable for GCM
//

VOID
SYMCRYPT_CALL
SymCryptGcmKeyCopy( _In_ PCSYMCRYPT_GCM_EXPANDED_KEY pSrc, _Out_ PSYMCRYPT_GCM_EXPANDED_KEY pDst );

//
// Create a copy of an expanded key
//

VOID
SYMCRYPT_CALL
SymCryptGcmEncrypt(
     _In_                           PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
     _In_reads_( cbNonce )          PCBYTE                      pbNonce,
                                    SIZE_T                      cbNonce,
     _In_reads_opt_( cbAuthData )   PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
     _In_reads_( cbData )           PCBYTE                      pbSrc,
     _Out_writes_( cbData )         PBYTE                       pbDst,
                                    SIZE_T                      cbData,
     _Out_writes_( cbTag )          PBYTE                       pbTag,
                                    SIZE_T                      cbTag );

//
//  Encrypt a buffer using the block cipher in GCM mode.
//      - pExpandedKey points to the expanded key for GCM.
//      - pbNonce: Pointer to the nonce for this encryption. For a single key, each nonce
//          value may be used at most once to encrypt data. Re-using nonce values leads
//          to catastrophic loss of security. Only 12-byte nonces are supported,
//          per the SP800-38D section 5.2.1.1 recommendation.
//      - cbNonce: number of bytes in the nonce, must be 12.
//      - pbAuthData: pointer to the associated authentication data. This data is not encrypted
//          but it is included in the authentication. Use NULL if not used.
//      - cbAuthData: # bytes of associated authentication data. (0 if not used)
//      - pbSrc: plaintext input
//      - pbDst: ciphertext output. The ciphertext buffer may be identical to the plaintext
//          buffer, or non-overlapping. The ciphertext is also cbData bytes long.
//      - cbData: # bytes of plaintext input. The maximum length is 2^{36} - 32 bytes.
//      - pbTag: buffer that will receive the authentication tag.
//      - cbTag: size of tag. cbTag must be one of {12, 13, 14, 15, 16} per SP800-38D
//          section 5.2.1.2. The optional shorter tag sizes (4 and 8) are not supported.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmDecrypt(
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbNonce )           PCBYTE                      pbNonce,
                                    SIZE_T                      cbNonce,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _In_reads_( cbTag )             PCBYTE                      pbTag,
                                    SIZE_T                      cbTag );
//
// Decrypt a buffer using the block cipher in GCM mode.
// See SymCryptGcmEncrypt for a description of the parameters. This function decrypts rather than
// encrypts, and as a result the pbTag parameter is read rather than filled.
// If the tag value is not correct the SYMCRYPT_AUTHENTICATION_FAILURE error is returned and the pbDst buffer
// is wiped of any plaintext.
// Note: While checking the authentication the purported plaintext is stored in pbDst. It is not safe to reveal
// purported plaintext when the authentication has not been checked. (Doing so would reveal key stream information
// that can be used to decrypt any message encrypted with the same nonce value.) Thus, users should be careful
// to not reveal the pbDst buffer until this function returns (e.g. through other threads or sharing memory).
//

//
// We also provide functions for incremental computation of GCM encryption and decryption. See the functions
// above for a description of the parameters and restrictions.
// In particular, note that the restriction on revealing the plaintext for unauthenticated decryptions holds
// for all the decrypted data, even when the decryption is done incrementally.
//
//
// SYMCRYPT_GCM_STATE
//      Ongoing state of an incremental GCM encryption or decryption operation.
//

VOID
SYMCRYPT_CALL
SymCryptGcmInit(
    _Out_                       PSYMCRYPT_GCM_STATE         pState,
    _In_                        PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_( cbNonce )       PCBYTE                      pbNonce,
                                SIZE_T                      cbNonce );
//
// Initialize a GCM computation.
// The pBlockCipher and pExpandedKey structures must remain unchanged until the GCM computation is finished.
//

VOID
SYMCRYPT_CALL
SymCryptGcmStateCopy(
    _In_        PCSYMCRYPT_GCM_STATE            pSrc,
    _In_opt_    PCSYMCRYPT_GCM_EXPANDED_KEY     pExpandedKeyCopy,
    _Out_       PSYMCRYPT_GCM_STATE             pDst );
//
// Copy a GCM state.
// If pExpandedKeyCopy is NULL, then the new pDst state uses the same expanded key as pSrc.
// If pExpandedKeyCopy is not NULL, it must point to a copy of the expanded key of the pSrc state.
// This new expanded key will be used as the expanded key for pDst.
//

VOID
SYMCRYPT_CALL
SymCryptGcmAuthPart(
    _Inout_                     PSYMCRYPT_GCM_STATE pState,
    _In_reads_opt_( cbData )    PCBYTE              pbAuthData,
                                SIZE_T              cbData );
//
// Incrementally process the authentication data. This function can be called multiple times
// after the SymCryptGcmInit function. It may not be called after any encrypt or decrypt
// function has been called on the GCM state.
//

VOID
SYMCRYPT_CALL
SymCryptGcmEncryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

VOID
SYMCRYPT_CALL
SymCryptGcmEncryptFinal(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _Out_writes_( cbTag )   PBYTE               pbTag,
                            SIZE_T              cbTag );

VOID
SYMCRYPT_CALL
SymCryptGcmDecryptPart(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
                            SIZE_T              cbData );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptGcmDecryptFinal(
    _Inout_                 PSYMCRYPT_GCM_STATE pState,
    _In_reads_( cbTag )     PCBYTE              pbTag,
                            SIZE_T              cbTag );
//
// Returns SYMCRYPT_AUTHENTICATION_FAILURE if the tag value does not match.
//


VOID
SYMCRYPT_CALL
SymCryptGcmSelftest(void);
//
// Self test for GCM cipher mode
//


//==========================================================================
//   SESSION BASED APIs
//==========================================================================


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionSenderInit(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT32              senderId,
            UINT32              flags );
//
//  Initialize an encryption session object. The default nonce size of 12B is used - 8B are provided
//  by message number, 4B by senderId.
//      - pSession: Pointer to an uninitialized session object.
//      - senderId: The id of the sender (must be unique for each user of a given key).
//        Callers should either choose a senderId which is specific to the sender, or
//        at least to the software and role in a system in which a key is being used.
//        Two encryption sessions using the same key and senderId leads to catastrophic loss of security.
//      - No flags are specified for this function
//
//  Remarks:
//  On some platforms use of a session object requires use of a mutex. On those platforms this
//  function will call SymCryptCallbackAllocateMutexFastInproc and may indicate failure by returning
//  SYMCRYPT_MEMORY_ALLOCATION_FAILURE if a mutex object cannot be created.
//  Callers must call SymCryptSessionDestroy to ensure any associated allocated mutex object is freed
//  either before calling another Init function on the SYMCRYPT_SESSION object, and instead of directly
//  calling SymCryptWipeKnownSize on the object.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionReceiverInit(
    _Inout_ PSYMCRYPT_SESSION   pSession,
            UINT32              senderId,
            UINT32              flags );
//
//  Initialize an decryption session object. The default nonce size of 12B is used - 8B are provided
//  by message number, 4B by senderId.
//      - pSession: Pointer to an uninitialized session object.
//      - senderId: The id of the sender (must be unique for each user of a given key).
//        Callers should either choose a senderId which is specific to the sender, or
//        at least to the software and role in a system in which a key is being used.
//        The id used in a decryption session must be the same as the id used in the corresponding
//        encryption session (i.e. sender and receiver must agree upon a senderId for their
//        communication session)
//      - No flags are specified for this function
//
//  Remarks:
//  On some platforms use of a session object requires use of a mutex. On those platforms this
//  function will call SymCryptCallbackAllocateMutexFastInproc and may indicate failure by returning
//  SYMCRYPT_MEMORY_ALLOCATION_FAILURE if a mutex object cannot be created.
//  Callers must call SymCryptSessionDestroy to ensure any associated allocated mutex object is freed
//  either before calling another Init function on the SYMCRYPT_SESSION object, and instead of directly
//  calling SymCryptWipeKnownSize on the object.
//

VOID
SYMCRYPT_CALL
SymCryptSessionDestroy(
    _Inout_ PSYMCRYPT_SESSION   pSession );
//
//  Clear session object and free any data associated with the object (i.e. allocated locks)
//  After this call the memory used for pSession is uninitialized and can be used for other purposes.
//  Note that it is not safe to just wipe the memory of the session object as the session
//  object contains pointers to other allocations.
//  The only way to safely destroy a session is to use this function.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionGcmEncrypt(
    _Inout_                         PSYMCRYPT_SESSION           pSession,
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _Out_writes_( cbTag )           PBYTE                       pbTag,
                                    SIZE_T                      cbTag,
    _Out_opt_                       PUINT64                     pu64MessageNumber );
//
//  Encrypt a buffer, in a series, using the block cipher in GCM mode.
//      - pSession points to the session object for this series of GCM encryptions. It handles
//        ensuring Nonce uniqueness across several encryption calls using the same key. The message
//        number in the pSession object is atomically incremented by this call.
//        If too many messages (2^64 - 2^32) have been encrypted with the same session object,
//        SYMCRYPT_INVALID_ARGUMENT is returned and no encryption takes place. This should never
//        occur in real use!
//      - pExpandedKey points to the expanded key for GCM.
//      - pbAuthData: pointer to the associated authentication data. This data is not encrypted
//          but it is included in the authentication. Use NULL if not used.
//      - cbAuthData: # bytes of associated authentication data. (0 if not used)
//      - pbSrc: plaintext input
//      - pbDst: ciphertext output. The ciphertext buffer may be identical to the plaintext
//          buffer, or non-overlapping. The ciphertext is also cbData bytes long.
//      - cbData: # bytes of plaintext input. The maximum length is 2^{36} - 32 bytes.
//      - pbTag: buffer that will receive the authentication tag.
//      - cbTag: size of tag. cbTag must be one of {12, 13, 14, 15, 16} per SP800-38D
//          section 5.2.1.2. The optional shorter tag sizes (4 and 8) are not supported.
//      - pu64MessageNumber: Optional message number output for this encryption. A unique message
//          number is extracted from the pSession object, this output is set to the value used in
//          the encryption. The first message number generated in a session will have the value 1,
//          and subsequent message numbers will be taken by atomically incrementing the counter.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSessionGcmDecrypt(
    _Inout_                         PSYMCRYPT_SESSION           pSession,
                                    UINT64                      messageNumber,
    _In_                            PCSYMCRYPT_GCM_EXPANDED_KEY pExpandedKey,
    _In_reads_opt_( cbAuthData )    PCBYTE                      pbAuthData,
                                    SIZE_T                      cbAuthData,
    _In_reads_( cbData )            PCBYTE                      pbSrc,
    _Out_writes_( cbData )          PBYTE                       pbDst,
                                    SIZE_T                      cbData,
    _In_reads_( cbTag )             PCBYTE                      pbTag,
                                    SIZE_T                      cbTag );
//
// Decrypt a buffer, in a series, using the block cipher in GCM mode.
//      - pSession points to the session object for this series of GCM decryptions. It handles
//          ensuring Nonce uniqueness across several decryption calls using the same key, particularly
//          ensuring there are no replays.
//      - messageNumber: The message number to be used for this decryption, forming part of the Nonce.
//          When performing decryption in a session, it is guaranteed that no 2 decryptions using the
//          same session and same message number can succeed. This is to provide protection against
//          replay attacks.
//          In order to provide this guarantee, pSession tracks a window of used message numbers
//          preceding the largest messageNumber successfully used so far in the decryption session.
//          A SYMCRYPT_SESSION_REPLAY_FAILURE error will be returned if either:
//          a) messageNumber is less than the smallest message number that can be tracked for replays
//          b) messageNumber is within the window that can be tracked for replays, and the message
//             number is marked as already having been used in a successful decryption in this session
//          In either case, the destination buffer is wiped.
// See SymCryptSessionGcmEncrypt for a description of the other parameters. This function decrypts
// rather than encrypts, and as a result the pbTag parameter is read rather than filled.
// If the tag value is not correct the SYMCRYPT_AUTHENTICATION_FAILURE error is returned and the
// pbDst buffer is wiped of any plaintext.
// Note: While checking the authentication the purported plaintext is stored in pbDst. It is not safe to reveal
// purported plaintext when the authentication has not been checked. (Doing so would reveal key stream information
// that can be used to decrypt any message encrypted with the same nonce value.) Thus, users should be careful
// to not reveal the pbDst buffer until this function returns (e.g. through other threads or sharing memory).
//


//==========================================================================
//   STREAM CIPHERS
//==========================================================================

////////////////////////////////////////////////////////////////////////////
//   RC4
//
// The RC4 stream cipher
//
// Use of RC4 is not recommended.
//
// The RC4 implementation makes extensive use of table lookups to implement the S-boxes of the algorithm.
// This violates our current crypto implementation guidelines and opens up a possible side-channel attack
// through information leakage via the memory caching system of the CPU.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRc4Init(
    _Out_                   PSYMCRYPT_RC4_STATE pState,
    _In_reads_( cbKey )     PCBYTE              pbKey,
    _In_                    SIZE_T              cbKey );
//
// Initialize an RC4 encryption/decryption state.
// WARNING: the most common error in using RC4 is to use the same key to encrypt two different pieces of data.
// This is insecure and should never be done; you need a unique key for each data element that is encrypted.
//

VOID
SYMCRYPT_CALL
SymCryptRc4Crypt(
    _Inout_                 PSYMCRYPT_RC4_STATE pState,
    _In_reads_( cbData )    PCBYTE              pbSrc,
    _Out_writes_( cbData )  PBYTE               pbDst,
    _In_                    SIZE_T              cbData );
//
// Encrypt or Decrypt data using the RC4 state. Note that the RC4 state is updated and therefore this
// function cannot be used by two threads simultaneously using the same state object.
//

VOID
SYMCRYPT_CALL
SymCryptRc4Selftest(void);


//
// ChaCha20
//
// The ChaCha20 stream cipher is specified in RFC 7539 and referenced by RFC 7905
// which specifies the ChaCha20-Poly1305 TLS cipher suite.
//
// ChaCha is a random-access stream cipher. It is possible to jump to any part of
// the key stream and start en/decrypting there.
// We support this by allowing the caller to select the position in the key stream
// to use.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptChaCha20Init(
    _Out_                   PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbKey )     PCBYTE                      pbKey,
    _In_                    SIZE_T                      cbKey,
    _In_reads_( cbNonce )   PCBYTE                      pbNonce,
                            SIZE_T                      cbNonce,
                            UINT64                      offset );
//
// Initialize a ChaCha20 en/decryption state.
// Key must be 32 bytes
// Nonce must be 12 bytes
// offset is the position into the key stream that the next encrypt/decrypt
// operation will use. Requirement: 0 <= offset < 2^38
// The ChaCha documentation is formulated in terms of a 'counter' or 'initial counter'.
// Callers can set offset = 64 * <counter> to achieve the same results.
//
// An error is returned only for invalid key or nonce sizes.
//
// A single (key,nonce) pair defines a key stream of 256 GB.
// Any part of that key stream can be used to encrypt a message, or part of a
// message.
// Note that it is critical that each key stream byte is used only once; thus
// callers have to ensure that for any key, each nonce is used at most once for
// a message, and messages cannot use any part of the 256 GB key stream more than
// once.
//

VOID
SYMCRYPT_CALL
SymCryptChaCha20SetOffset(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
                            UINT64                      offset );
//
// Specify the offset into the key stream where the next encrypt/decrypt operation
// will start.
// Requirement: 0 <= offset < 2^38
//

VOID
SYMCRYPT_CALL
SymCryptChaCha20Crypt(
    _Inout_                 PSYMCRYPT_CHACHA20_STATE    pState,
    _In_reads_( cbData )    PCBYTE                      pbSrc,
    _Out_writes_( cbData )  PBYTE                       pbDst,
                            SIZE_T                      cbData );
//
// Encrypt or Decrypt data using the CHACHA20 state.
// The Src data is xorred with the key stream generated from the state, and the result stored
// in the Dst buffer. The Src and Dst buffer can be identical or non-overlapping; partial overlaps
// are not supported.
// As the state is updated two threads cannot en/decrypt with the same state at the same time.
// The key stream used is the one generated from the key and nonce, starting at the specified
// offset into the key stream. This function updates the offset of the state by adding cbData to
// it so that the next call will use the next part of the key stream.
// Any attempt to use the key stream at offset >= 2^38 will result in catastrophic loss of security.
//

VOID
SYMCRYPT_CALL
SymCryptChaCha20Selftest(void);




//==========================================================================
//   KEY DERIVATION ALGORITHMS
//==========================================================================

////////////////////////////////////////////////////////////////////////////
// PBKDF2
//
// Generic KDF parameter handling:
//  - Generic parameter is passed in the Salt input;
//  - iterationCnt is set to 1.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2ExpandKey(
    _Out_               PSYMCRYPT_PBKDF2_EXPANDED_KEY   pExpandedKey,
    _In_                PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2Derive(
    _In_                    PCSYMCRYPT_PBKDF2_EXPANDED_KEY  pExpandedKey,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt,
                            UINT64                          iterationCnt,
    _Out_writes_(cbResult)  PBYTE                           pbResult,
                            SIZE_T                          cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptPbkdf2(
                            PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbKey)       PCBYTE          pbKey,
                            SIZE_T          cbKey,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
                            UINT64          iterationCnt,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult);

//
// Because the self-test pulls in the associated MAC function,
// we have several self-tests; each of which tests the PBKDF2 implementation
// using the specified MAC function.
// This allows a FIPS module to run the self-test with the MAC function it already
// uses internally.
//
// More can be added when needed.
//

VOID
SYMCRYPT_CALL
SymCryptPbkdf2_HmacSha1SelfTest(void);

VOID
SYMCRYPT_CALL
SymCryptPbkdf2_HmacSha256SelfTest(void);

////////////////////////////////////////////////////////////////////////////
// SP800-108 Counter mode
//
// Generic KDF parameter handling:
// Generic parameter contains the concatenation of the Label, a zero byte, and the Context.
// To pass a generic parameter do the following:
//  - pbLabel = NULL
//  - cbLabel = (SIZE_T) -1;
//  - pbContext/cbContext = generic parameter
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108ExpandKey(
    _Out_               PSYMCRYPT_SP800_108_EXPANDED_KEY    pExpandedKey,
    _In_                PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108Derive(
    _In_                        PCSYMCRYPT_SP800_108_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel)     PCBYTE                              pbLabel,
                                SIZE_T                              cbLabel,
    _In_reads_opt_(cbContext)   PCBYTE                              pbContext,
                                SIZE_T                              cbContext,
    _Out_writes_(cbResult)      PBYTE                               pbResult,
                                SIZE_T                              cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSp800_108(
                                PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbKey)           PCBYTE          pbKey,
                                SIZE_T          cbKey,
    _In_reads_opt_(cbLabel)     PCBYTE          pbLabel,
                                SIZE_T          cbLabel,
    _In_reads_opt_(cbContext)   PCBYTE          pbContext,
                                SIZE_T          cbContext,
    _Out_writes_(cbResult)      PBYTE           pbResult,
                                SIZE_T          cbResult);

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha1SelfTest(void);

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha256SelfTest(void);

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha384SelfTest(void);

VOID
SYMCRYPT_CALL
SymCryptSp800_108_HmacSha512SelfTest(void);

////////////////////////////////////////////////////////////////////////////
// TLS Key Derivation PRFs
//
// PRFs used in the key derivation functions of the TLS protocol, versions
// 1.0, 1.1, and 1.2. These are defined in RFC 2246, 4346, and 5246,
// respectively.
// Note: The PRFs for versions 1.0 and 1.1 are identical.
//

// Maximum sizes (in bytes) for the label and the seed inputs. See the
// above RFCs 2246, 4346, and 5246 for more details.
#define SYMCRYPT_TLS_MAX_LABEL_SIZE 256
#define SYMCRYPT_TLS_MAX_SEED_SIZE  256

//
//  Version 1.0/1.1
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1ExpandKey(
    _Out_               PSYMCRYPT_TLSPRF1_1_EXPANDED_KEY    pExpandedKey,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1Derive(
    _In_                    PCSYMCRYPT_TLSPRF1_1_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel) PCBYTE                              pbLabel,
    _In_                    SIZE_T                              cbLabel,        // Up to SYMCRYPT_TLS_MAX_LABEL_SIZE
    _In_reads_(cbSeed)      PCBYTE                              pbSeed,
    _In_                    SIZE_T                              cbSeed,         // Up to SYMCRYPT_TLS_MAX_SEED_SIZE
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_1(
    _In_reads_(cbKey)       PCBYTE   pbKey,
    _In_                    SIZE_T   cbKey,
    _In_reads_opt_(cbLabel) PCBYTE   pbLabel,
    _In_                    SIZE_T   cbLabel,
    _In_reads_(cbSeed)      PCBYTE   pbSeed,
    _In_                    SIZE_T   cbSeed,
    _Out_writes_(cbResult)  PBYTE    pbResult,
                            SIZE_T   cbResult);

VOID
SYMCRYPT_CALL
SymCryptTlsPrf1_1SelfTest(void);

//
//  Version 1.2
//
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2ExpandKey(
    _Out_               PSYMCRYPT_TLSPRF1_2_EXPANDED_KEY    pExpandedKey,
    _In_                PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_(cbKey)   PCBYTE                              pbKey,
                        SIZE_T                              cbKey);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2Derive(
    _In_                    PCSYMCRYPT_TLSPRF1_2_EXPANDED_KEY   pExpandedKey,
    _In_reads_opt_(cbLabel) PCBYTE                              pbLabel,
    _In_                    SIZE_T                              cbLabel,    // Up to SYMCRYPT_TLS_MAX_LABEL_SIZE
    _In_reads_(cbSeed)      PCBYTE                              pbSeed,
    _In_                    SIZE_T                              cbSeed,     // Up to SYMCRYPT_TLS_MAX_SEED_SIZE
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsPrf1_2(
    _In_                    PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbKey)       PCBYTE          pbKey,
    _In_                    SIZE_T          cbKey,
    _In_reads_opt_(cbLabel) PCBYTE          pbLabel,
    _In_                    SIZE_T          cbLabel,
    _In_reads_(cbSeed)      PCBYTE          pbSeed,
    _In_                    SIZE_T          cbSeed,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult);

VOID
SYMCRYPT_CALL
SymCryptTlsPrf1_2SelfTest(void);


////////////////////////////////////////////////////////////////////////////
// SSH-KDF as specified in RFC 4253 Section 7.2.
//


// Labels defined in RFC 4253
#define SYMCRYPT_SSHKDF_IV_CLIENT_TO_SERVER                0x41        // 'A'
#define SYMCRYPT_SSHKDF_IV_SERVER_TO_CLIENT                0x42        // 'B'
#define SYMCRYPT_SSHKDF_ENCRYPTION_KEY_CLIENT_TO_SERVER    0x43        // 'C'
#define SYMCRYPT_SSHKDF_ENCRYPTION_KEY_SERVER_TO_CLIENT    0x44        // 'D'
#define SYMCRYPT_SSHKDF_INTEGRITY_KEY_CLIENT_TO_SERVER     0x45        // 'E'
#define SYMCRYPT_SSHKDF_INTEGRITY_KEY_SERVER_TO_CLIENT     0x46        // 'F'


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdfExpandKey(
    _Out_               PSYMCRYPT_SSHKDF_EXPANDED_KEY   pExpandedKey,
    _In_                PCSYMCRYPT_HASH                 pHashFunc,
    _In_reads_(cbKey)   PCBYTE                          pbKey,
                        SIZE_T                          cbKey);
//
// Process the key using the specified hash function and store the result in
// SYMCRYPT_SSHKDF_EXPANDED_KEY structure. Once the key is expanded,
// SymCryptSshKdfDerive can be called multiple times to generate keys for
// different uses/labels.
//
// After all the keys are derived from a particular "shared secret" key,
// SYMCRYPT_SSHKDF_EXPANDED_KEY structure must be wiped.
//
// Parameters:
//      - pExpandedKey  :   Pointer to a SYMCRYPT_SSHKDF_EXPANDED_KEY structure that
//                          will contain the expanded key after the function returns.
//      - pHashFunc     :   Hash function that will be used in the key derivation.
//                          This function is saved in SYMCRYPT_SSHKDF_EXPANDED_KEY
//                          so that it is also used by the SymCryptSshKdfDerive function.
//      - pbKey, cbKey  :   Buffer containing the secret key for the KDF.
//
// Returns SYMCRYPT_NO_ERROR
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdfDerive(
    _In_                        PCSYMCRYPT_SSHKDF_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbHashValue)     PCBYTE                          pbHashValue,
                                SIZE_T                          cbHashValue,
                                BYTE                            label,
    _In_reads_(cbSessionId)     PCBYTE                          pbSessionId,
                                SIZE_T                          cbSessionId,
    _Inout_updates_(cbOutput)   PBYTE                           pbOutput,
                                SIZE_T                          cbOutput);
//
// Derive keys using the expanded key that was initialized with SymCryptSshKdfExpandKey
// along with other inputs. This function can be called consecutively with varying label
// values to generate keys for different purposes as defined in the RFC.
//
// Parameters:
//      - pExpandedKey              :   Pointer to a SYMCRYPT_SSHKDF_EXPANDED_KEY structure that is
//                                      initialized by a prior call to SymCryptSshKdfExpandKey.
//                                      Must be wiped when SymCryptSshKdfDerive is not going to be called
//                                      again with the same expanded key.
//      - pbHashValue, cbHashValue  :   Buffer pointing to "exchange hash" value. cbHashValue must be equal
//                                      to the output size of the hash function passed to SymCryptSshKdfExpandKey.
//      - label                     :   Label value used to indicate the type of the derived key.
//      - pbSessionId, cbSessionId  :   Buffer pointing to the session identifier. cbSessionId must be equal
//                                      to the output size of the hash function passed to SymCryptSshKdfExpandKey.
//      - pbOutput, cbOutput        :   Buffer to store the derived key. Exactly cbOutput bytes of output will be generated.
//
// Returns SYMCRYPT_NO_ERROR
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSshKdf(
    _In_                    PCSYMCRYPT_HASH     pHashFunc,
    _In_reads_(cbKey)       PCBYTE              pbKey,
                            SIZE_T              cbKey,
    _In_reads_(cbHashValue) PCBYTE              pbHashValue,
                            SIZE_T              cbHashValue,
                            BYTE                label,
    _In_reads_(cbSessionId) PCBYTE              pbSessionId,
                            SIZE_T              cbSessionId,
    _Out_writes_(cbOutput)  PBYTE               pbOutput,
                            SIZE_T              cbOutput);
//
// This function is a wrapper for using SymCryptSshKdfExpandKey followed by SymCryptSshKdfDerive
// in order to produce SSH-KDF output.
//
// All of the function arguments are forwarded to SymCryptSshKdfExpandKey and SymCryptSshKdfDerive
// functions, hence the documentation on those functions apply here as well.
//


VOID
SYMCRYPT_CALL
SymCryptSshKdfSha256SelfTest(void);

VOID
SYMCRYPT_CALL
SymCryptSshKdfSha512SelfTest(void);


////////////////////////////////////////////////////////////////////////////
// SRTP-KDF as specified in RFC 3711 Section 4.3.1.
//


// Labels defined in RFC 3711
#define SYMCRYPT_SRTP_ENCRYPTION_KEY        0x00
#define SYMCRYPT_SRTP_AUTHENTICATION_KEY    0x01
#define SYMCRYPT_SRTP_SALTING_KEY           0x02
#define SYMCRYPT_SRTCP_ENCRYPTION_KEY       0x03
#define SYMCRYPT_SRTCP_AUTHENTICATION_KEY   0x04
#define SYMCRYPT_SRTCP_SALTING_KEY          0x05


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdfExpandKey(
    _Out_                PSYMCRYPT_SRTPKDF_EXPANDED_KEY  pExpandedKey,
    _In_reads_(cbKey)    PCBYTE                          pbKey,
                         SIZE_T                          cbKey);
//
// Process the key and store the result in SYMCRYPT_SRTPKDF_EXPANDED_KEY structure.
// Once the key is expanded, SymCryptSrtpKdfDerive can be called multiple times to
// generate keys for different uses/labels.
//
// After all the keys are derived from a particular "shared secret" key,
// SYMCRYPT_SRTPKDF_EXPANDED_KEY structure must be wiped.
//
// Parameters:
//      - pExpandedKey  :   Pointer to a SYMCRYPT_SRTPKDF_EXPANDED_KEY structure that
//                          will contain the expanded key after the function returns.
//      - pbKey, cbKey  :   Buffer containing the secret key for the KDF. cbKey must be
//                          a valid AES key size (16-, 24-, or 32-bytes).
//
// Returns:
//      SYMCRYPT_WRONG_KEY_SIZE : If cbKey is not a valid AES key size
//      SYMCRYPT_NO_ERROR       : On success
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdfDerive(
    _In_                    PCSYMCRYPT_SRTPKDF_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSalt)      PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt,
                            UINT32                          uKeyDerivationRate,
                            UINT64                          uIndex,
                            UINT32                          uIndexWidth,
                            BYTE                            label,
    _Out_writes_(cbOutput)  PBYTE                           pbOutput,
                            SIZE_T                          cbOutput);
//
// Derive keys using the expanded key that was initialized with SymCryptSrtpKdfExpandKey
// along with other inputs. This function can be called consecutively with varying label
// values to generate keys for different purposes as defined in the RFC.
//
// Parameters:
//      - pExpandedKey              :   Pointer to a SYMCRYPT_SRTPKDF_EXPANDED_KEY structure that is
//                                      initialized by a prior call to SymCryptSrtpKdfExpandKey.
//                                      Must be wiped when SymCryptSrtpKdfDerive is not going to be called
//                                      again with the same expanded key.
//      - pbSalt, cbSalt            :   Buffer pointing to the salt value. cbSalt must always be 14 (112-bits).
//      - uKeyDerivationRate        :   Key derivation rate; must be zero or 2^i for 0 <= i <= 24.
//      - uIndex                    :   Denotes an SRTP index value when label is 0x00, 0x01, or 0x02, otherwise
//                                      denotes an SRTCP index value.
//      - uIndexWidth               :   Denotes how wide uIndex value is. Must be one of 0, 32, or 48. By default,
//                                      (when uIndexWidth = 0) uIndex is treated as 48-bits.
//                                      RFC 3711 initially defined SRTCP indices to be 32-bit values. It was updated
//                                      to be 48-bits by Errata ID 3712. SRTP index values are defined to be 48-bits.
//      - label                     :   Label value used to indicate the type of the derived key.
//      - pbOutput, cbOutput        :   Buffer to store the derived key. Exactly cbOutput bytes of output will be generated.
//
// Returns:
//      SYMCRYPT_INVALID_ARGUMENT   :   If cbSalt is not 14-bytes, or uKeyDerivationRate in invalid.
//      SYMCRYPT_NO_ERROR           :   On success.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSrtpKdf(
    _In_reads_(cbKey)           PCBYTE  pbKey,
                                SIZE_T  cbKey,
    _In_reads_(cbSalt)          PCBYTE  pbSalt,
                                SIZE_T  cbSalt,
                                UINT32  uKeyDerivationRate,
                                UINT64  uIndex,
                                UINT32  uIndexWidth,
                                BYTE    label,
    _Out_writes_(cbOutput)      PBYTE   pbOutput,
                                SIZE_T  cbOutput);
//
// This function is a wrapper for using SymCryptSrtpKdfExpandKey followed by SymCryptSrtpKdfDerive
// in order to produce SRTP-KDF output.
//
// All of the function arguments are forwarded to SymCryptSrtpKdfExpandKey and SymCryptSrtpKdfDerive
// functions, hence the documentation on those functions apply here as well.
//


VOID
SYMCRYPT_CALL
SymCryptSrtpKdfSelfTest(void);


////////////////////////////////////////////////////////////////////////////
// HKDF
//
// PRF used in the key derivation functions of the TLS protocol, version
// 1.3. It is defined in RFC 5869.
//
// The SymCrypt ExtractPrk function corresponds to the "HKDF-Extract" function
// of the RFC 5869, while the SymCrypt PrkExpandKey and Derive functions
// correspond to the "HKDF-Expand" function of the RFC.
//
// SymCryptHkdfExtractPrk takes as inputs the MAC algorithm, the IKM (input
// keying material), and the optional salt. It executes the full "HKDF-Extract"
// function to produce the PRK (pseudorandom key).
//
// SymCryptHkdfPrkExpandKey takes as inputs just the MAC algorithm and the PRK.
// It produces the final (MAC) key to be used by the "HKDF-Expand" function.
//
// SymCryptHkdfExpandKey performs SymCryptHkdfExtractPrk followed by
// SymCryptHkdfPrkExpandKey to produce the final (MAC) key to be used by the
// "HKDF-Expand" function, without exposing the PRK to the caller.
//
// SymCryptHkdfDerive takes as input the final MAC key and the optional info. It
// performs the rest of the "HKDF-Expand" function to produce the HKDF result.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfExpandKey(
    _Out_                   PSYMCRYPT_HKDF_EXPANDED_KEY     pExpandedKey,
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE                          pbIkm,
                            SIZE_T                          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfExtractPrk(
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE                          pbIkm,
                            SIZE_T                          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE                          pbSalt,
                            SIZE_T                          cbSalt,
    _Out_writes_(cbPrk)     PBYTE                           pbPrk,
                            SIZE_T                          cbPrk );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfPrkExpandKey(
    _Out_                   PSYMCRYPT_HKDF_EXPANDED_KEY     pExpandedKey,
    _In_                    PCSYMCRYPT_MAC                  macAlgorithm,
    _In_reads_(cbPrk)       PCBYTE                          pbPrk,
                            SIZE_T                          cbPrk );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdfDerive(
    _In_                    PCSYMCRYPT_HKDF_EXPANDED_KEY    pExpandedKey,
    _In_reads_opt_(cbInfo)  PCBYTE                          pbInfo,
                            SIZE_T                          cbInfo,
    _Out_writes_(cbResult)  PBYTE                           pbResult,
                            SIZE_T                          cbResult);

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHkdf(
                            PCSYMCRYPT_MAC  macAlgorithm,
    _In_reads_(cbIkm)       PCBYTE          pbIkm,
                            SIZE_T          cbIkm,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult);

VOID
SYMCRYPT_CALL
SymCryptHkdfSelfTest(void);

////////////////////////////////////////////////////////////////////////////
// SSKDF
//
// Single-Step KDF as specified in SP800-56C section 4.
//
// SSKDF requires an auxiliary function H. This can be approved hash function,
// HMAC with an approved hash function, or KMAC. The approved hash functions
// are listed in SP800-56C section 7.
//
// A salt value may be optionally provided if either HMAC or KMAC is used for H.
// When no salt is provided, an all-zero default salt is used instead. For HMAC,
// the default salt is the length of an input block of the HMAC's hash function.
// For KMAC128, the default salt is 164 bytes. For KMAC256, the default salt is 132 bytes.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMacExpandSalt(
    _Out_                   PSYMCRYPT_SSKDF_MAC_EXPANDED_SALT   pExpandedSalt,
    _In_                    PCSYMCRYPT_MAC                      macAlgorithm,
    _In_reads_opt_(cbSalt)  PCBYTE                              pbSalt,
                            SIZE_T                              cbSalt);
//
// Initializes *pExpandedSalt with the macAlgorithm, and optionally the salt. Used
// for SSKDF when H is a MAC function. After calling SymCryptSskdfMacExpandSalt,
// SymCryptSskdfMacDerive can be called multiple times to generate keys for different
// uses, fixed infos, and shared secrets. For multiple KDFs using the same MAC and salt,
// calling SymCryptSskdfMacExpandSalt once and SymCryptSskdfMacDerive multiple times
// is more efficient than calling SymCryptSskdfMac multiple times.
//
// The expanded salt contains no secrets and does not need to be wiped.
//
// Parameters:
//    - pExpandedSalt   :   Pointer to a SYMCRYPT_SSKDF_MAC_EXPANDED_SALT structure that
//                          will contain the expanded salt after the function returns.
//    - macAlgorithm    :   MAC algorithm that will be used in the key derivation.
//                          This function is saved in SYMCRYPT_SSKDF_MAC_EXPANDED_SALT.
//    - pbSalt, cbSalt  :   Buffer containing the salt for the KDF. cbSalt must be a valid
//                          key size for the MAC algorithm. If pbSalt is NULL, the default
//                          all zero-byte salt is used.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMacDerive(
    _In_                    PCSYMCRYPT_SSKDF_MAC_EXPANDED_SALT  pExpandedSalt,
                            SIZE_T                              cbMacOutputSize,
    _In_reads_(cbSecret)    PCBYTE                              pbSecret,
                            SIZE_T                              cbSecret,
    _In_reads_opt_(cbInfo)  PCBYTE                              pbInfo,
                            SIZE_T                              cbInfo,
    _Out_writes_(cbResult)  PBYTE                               pbResult,
                            SIZE_T                              cbResult);
//
// Derive keys using the expanded salt that was initialized with SymCryptSskdfMacExpandSalt
// along with other inputs. This function can be called consecutively with varying fixed infos
// and shared secrets to generate keys for different purposes as defined in the SP800-56C.
// The same pbExpandedKey can be used simultaneously by multiple threads.
//
// Parameters:
//    - pExpandedSalt       :   Pointer to a SYMCRYPT_SSKDF_MAC_EXPANDED_SALT structure that is
//                              initialized by a prior call to SymCryptSskdfMacExpandSalt.
//    - cbMacOutputSize     :   Output size used by the MAC algorithm for intermediate computations. Must not be
//                              greater than 64 bytes. Set to 0 for MACs that don't support variable output sizes,
//                              or to use the default output size. The default output size when KMAC is used is cbResult.
//    - pbSecret, cbSecret  :   Buffer containing the shared secret.
//    - pbInfo, cbInfo      :   Buffer containing the fixed info.
//    - pbResult, cbResult  :   Buffer to store the derived key. Exactly cbResult bytes of output will be generated.
//                              Must not exceed 2^{32} - 1 times the result size of the MAC algorithm.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfMac(
    _In_                    PCSYMCRYPT_MAC  macAlgorithm,
                            SIZE_T          cbMacOutputSize,
    _In_reads_(cbSecret)    PCBYTE          pbSecret,
                            SIZE_T          cbSecret,
    _In_reads_opt_(cbSalt)  PCBYTE          pbSalt,
                            SIZE_T          cbSalt,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult);
//
// This function is a wrapper for using SymCryptSskdfMacExpandSalt followed by SymCryptSskdfMacDerive
// in order to produce SSKDF output.
//
// All of the function arguments are forwarded to SymCryptSskdfMacExpandSalt and SymCryptSskdfMacDerive
// functions, hence the documentation on those functions apply here as well.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSskdfHash(
    _In_                    PCSYMCRYPT_HASH hashAlgorithm,
                            SIZE_T          cbHashOutputSize,
    _In_reads_(cbSecret)    PCBYTE          pbSecret,
                            SIZE_T          cbSecret,
    _In_reads_opt_(cbInfo)  PCBYTE          pbInfo,
                            SIZE_T          cbInfo,
    _Out_writes_(cbResult)  PBYTE           pbResult,
                            SIZE_T          cbResult);
//
// Derive keys using the specified hash algorithm as H.
//
// Parameters:
//    - hashAlgorithm       :   Hash algorithm that will be used in the key derivation.
//    - cbHashOutputSize    :   Output size used by the hash algorithm for intermediate computations.
//                              Set to 0 for hashes that don't support variable output sizes, or to use
//                              the default output size. Currently, no allowed hash algorithms support
//                              variable output sizes, so this should always be set to 0.
//    - pbSecret, cbSecret  :   Buffer containing the shared secret.
//    - pbInfo, cbInfo      :   Buffer containing the fixed info.
//    - pbResult, cbResult  :   Buffer to store the derived key. Exactly cbResult bytes of output will be generated.
//                              Must not exceed 2^{32} - 1 times the result size of hashAlgorithm.
//

VOID
SYMCRYPT_CALL
SymCryptSskdfSelfTest(void);

//==========================================================================
//   RNG ALGORITHMS
//==========================================================================

////////////////////////////////////////////////////////////////////////////
// AES-CTR-DRBG
//
// This is an implementation of AES-CTR_DRBG as specified in SP 800-90.
// It always uses a 256-bit security strength.
//
// Note: This RNG is NOT compliant with FIPS 140-2 as it lacks the continuous
//   self test required by FIPS 140-2. See the AES-FIPS RNG algorithm below.
//
// SYMCRYPT_RNG_AES_STATE
//      State of an AES-CTR_DRBG instance.
//

#define SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE   (32 + 16)
#define SYMCRYPT_RNG_AES_MIN_RESEED_SIZE   (32)
#define SYMCRYPT_RNG_AES_MAX_SEED_SIZE   (256)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesInstantiate(
    _Out_                       PSYMCRYPT_RNG_AES_STATE pRngState,
    _In_reads_(cbSeedMaterial)  PCBYTE                  pcbSeedMaterial,

    _In_range_(SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE, SYMCRYPT_RNG_AES_MAX_SEED_SIZE)
                                SIZE_T                  cbSeedMaterial );
//
// Initialize a new SYMCRYPT_RNG_AES_STATE, and seed it with the seed material.
//
// 'Instantiate' is the SP800-90 terminology.
// The seed material must be at least SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE bytes,
// and at most SYMCRYPT_RNG_AES_MAX_SEED_SIZE bytes.
//
// This implementation always uses 256-bit security strength, and
// does not support 'prediction resistance' as defined in SP 800-90.
//
// SP 800-90 specifies three inputs to the instantiation:
// - entropy
// - nonce
// - personalization string
// This function takes only a single input, which is the concatenation of these three:
//   seed material := entropy | nonce | personalization string
//
// The following are the requirements on the three inputs:
//  Entropy: must have at least 256 bits of entropy
//  Nonce: must either be a random value with 128-bits of entropy, or a value that does not
//      repeat with a probability of more than 2^{-128}.
// Together these requirements imply that cbSeedMaterial should be at least
//  SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE
//
// This function only returns an error if the cbSeedMaterial value is out of range.
//

VOID
SYMCRYPT_CALL
SymCryptRngAesGenerate(
    _Inout_                 PSYMCRYPT_RNG_AES_STATE pRngState,
    _Out_writes_(cbRandom)  PBYTE                   pbRandom,
                            SIZE_T                  cbRandom );
//
// Generate random output from the state.
//
// Callers do not need to limit themselves to requests of 64 kB or less;
// large requests are split internally to follow the request size limitations of SP 800-90.
//
// SP 800-90 also requires a limit on the # generate calls that can be done between reseeds.
// For AES-CTR_DRBG this limit is 2^48, which means it is all but impossible to hit this limit.
// If the caller were to succeed, the 2^48'th call will result in a fatal error.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesReseed(
    _Inout_                     PSYMCRYPT_RNG_AES_STATE pRngState,
    _In_reads_(cbSeedMaterial)  PCBYTE                  pcbSeedMaterial,

    _In_range_(SYMCRYPT_RNG_AES_MIN_RESEED_SIZE, SYMCRYPT_RNG_AES_MAX_SEED_SIZE)
                                SIZE_T                  cbSeedMaterial );
//
// Reseed the PRNG state.
//
// The seed material consists of the concatenation of the following SP800-90 fields:
// - entropy
// - additional input
//
// The entropy input should have at least 256 bits of entropy.
// This function only returns an error if the cbSeedMaterial value is out of range.
//

VOID
SYMCRYPT_CALL
SymCryptRngAesUninstantiate(
    _Inout_                 PSYMCRYPT_RNG_AES_STATE pRngState );
//
// Uninstantiate (clean up) the PRNG state
//

VOID
SYMCRYPT_CALL
SymCryptRngAesInstantiateSelftest(void);
//
// For FIPS-certified modules, this function should be called before every instantiation.
// If multiple DRBGs are instantiated 'in quick succession', a single self-test is sufficient
//  (see SP 800-90 11.3.2).
//


VOID
SYMCRYPT_CALL
SymCryptRngAesReseedSelftest(void);
//
// FIPS-certified modules should call this function before every call to the reseed function.
//

VOID
SYMCRYPT_CALL
SymCryptRngAesGenerateSelftest(void);
//
// FIPS-certified modules should call this function at least once on startup, and whenever
// they want to re-test the generate function.
//

////////////////////////////////////////////////////////////////////////////
// AES-CTR-DRBG with FIPS 140-2 continuous self-test
//
// This is a straightforward wrapper around the AES-CTR-DRBG implementation
// that adds the FIPS 140-2 continuous self-test.
// At the moment, it looks like this test will not be present in FIPS 140-3 so
// this RNG will be dropped when FIPS 140-3 comes out.
// The self-test requirements are met by calling the selftest functions of the
// AES-CTR_DRBG implementation directly.
//
// These functions are functionally equivalent to the ones for AES-CTR_DRBG.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesFips140_2Instantiate(
    _Out_                       PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState,
    _In_reads_(cbSeedMaterial)  PCBYTE                              pcbSeedMaterial,

    _In_range_(SYMCRYPT_RNG_AES_MIN_INSTANTIATE_SIZE, SYMCRYPT_RNG_AES_MAX_SEED_SIZE)
                                SIZE_T                              cbSeedMaterial );

VOID
SYMCRYPT_CALL
SymCryptRngAesFips140_2Generate(
    _Inout_                 PSYMCRYPT_RNG_AES_FIPS140_2_STATE       pRngState,
    _Out_writes_(cbRandom)  PBYTE                                   pbRandom,
                            SIZE_T                                  cbRandom );

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRngAesFips140_2Reseed(
    _Inout_                     PSYMCRYPT_RNG_AES_FIPS140_2_STATE   pRngState,
    _In_reads_(cbSeedMaterial)  PCBYTE                              pcbSeedMaterial,

    _In_range_(SYMCRYPT_RNG_AES_MIN_RESEED_SIZE, SYMCRYPT_RNG_AES_MAX_SEED_SIZE)
                                SIZE_T                              cbSeedMaterial );

VOID
SYMCRYPT_CALL
SymCryptRngAesFips140_2Uninstantiate(
    _Inout_                 PSYMCRYPT_RNG_AES_FIPS140_2_STATE pRngState );

////////////////////////////////////////////////////////////////////////////////////////////
//
// Internal RNG functions
//
// To satisfy FIPS 140-3 and SP 800-90B, certain modules of SymCrypt may set up internal
// RNG state(s) to keep random bit generation behind the module's FIPS boundary.
// These functions allow the caller to get random bits and provide entropy, respectively,
// to SymCrypt's internal RNG state(s).
// Implementation is module dependent, and these functions may not be defined
// for certain modules. Check before using.
//

VOID
SYMCRYPT_CALL
SymCryptRandom(
    _Out_writes_(cbRandom)  PBYTE   pbRandom,
                            SIZE_T  cbRandom );
// Fills pbRandom with cbRandom random bytes

VOID
SYMCRYPT_CALL
SymCryptProvideEntropy(
    _In_reads_(cbEntropy)   PCBYTE  pbEntropy,
                            SIZE_T  cbEntropy );
// Mixes pbEntropy into the internal RNG state. There may be module-specific limits on
// cbEntropy - check module before use


////////////////////////////////////////////////////////////////////////////////////////////
//
// RdRand support
// These functions provide access to the RdRand random number generator in
// the latest Intel CPUs.
// The DRBG that underlies the RdRand instruction is limited to 128-bit security.
// The seed for each consecutive 8 kB of data can be recovered in 2^128 work.
// Therefore, we allow for multiple blocks of 8 kB to be gathered in an attempt to
// extract 256-bit security from the hardware.
// In general, to achieve N*128 bits of security, you should use a buffer of
// (N+1)*SYMCRYPT_RDRAND_RESEED_SIZE bytes.
//

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

// The RdRand instruction reseeds its internal DRBG every 8 kB (or faster)
#define SYMCRYPT_RDRAND_RESEED_SIZE (1<<13)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdrandStatus(void);
//
// Returns SYMCRYPT_NO_ERROR if RdRand is available.
// returns SYMCRYPT_NOT_IMPLEMENTED if RdRand is not available.
// Note: the library must be initialized before you call this function.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdrandGetBytes(
    _Out_writes_( cbBuffer )                    PBYTE   pbBuffer,
                                                SIZE_T  cbBuffer,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE   pbResult );
//
// Gets cbBuffer bytes from the RdRand instruction and hashes them to the pbResult buffer.
// pbBuffer points to a scratch buffer that is used internally, but wiped upon exit.
// cbBuffer must be a multiple of 16.
// Fatal error if SymCryptRdrandStatus indicates that Rdrand is not available.
// Returns an error if the RdRand instruction failed consistently.
// Note: SymCrypt only checks whether RdRand self-reports as failing. SymCrypt does NOT attempt
// to validate that the values returned in successful RdRand calls are in fact random.
// See SymCryptRdrandGet for a version that does not return an error but fatals instead.
//

VOID
SYMCRYPT_CALL
SymCryptRdrandGet(
    _Out_writes_( cbBuffer )                    PBYTE   pbBuffer,
                                                SIZE_T  cbBuffer,
    _Out_writes_( SYMCRYPT_SHA512_RESULT_SIZE ) PBYTE   pbResult );
//
// Gets cbBuffer bytes from the RdRand instruction and hashes them to the pbResult buffer.
// pbBuffer points to a scratch buffer that is used internally, but wiped upon exit.
// cbBuffer must be a multiple of 16.
// Fatal error if the RdRand instruction fails.
// Note: SymCrypt only checks whether RdRand self-reports as failing. SymCrypt does NOT attempt
// to validate that the values returned in successful RdRand calls are in fact random.
//

#endif


////////////////////////////////////////////////////////////////////////////////////////////
//
// RdSeed support
// These functions provide access to the RdSeed random number generator in
// recent Intel CPUs.
//

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdseedStatus(void);
//
// Returns SYMCRYPT_NO_ERROR if RdSeed is available.
// returns SYMCRYPT_NOT_IMPLEMENTED if RdSeed is not available.
// Note: the library must be initialized before you call this function.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRdseedGetBytes(
    _Out_writes_( cbResult )                    PBYTE   pbResult,
                                                SIZE_T  cbResult );
//
// Queries cbResult bytes from the Rdseed instruction and puts them in the buffer.
// The number of bytes (cbResult) must be a multiple of 16.
// Fatal error if the Rdseed instruction is not present.
// Returns an error if the Rdseed instruction fails consistently.
// Note: SymCrypt only checks whether Rdseed self-reports as failing. SymCrypt does NOT attempt
// to validate that the values returned in successful Rdseed calls are in fact random.
// See SymCryptRdseedGet for a version that does not return an error but fatals instead.
//

VOID
SYMCRYPT_CALL
SymCryptRdseedGet(
    _Out_writes_( cbResult )                    PBYTE   pbResult,
                                                SIZE_T  cbResult );
//
// Queries cbResult bytes from the Rdseed instruction and puts them in the buffer.
// The number of bytes (cbResult) must be a multiple of 16.
// Fatal error if the Rdseed instruction is not present, or the instruction fails consistently.
// Note: SymCrypt only checks whether Rdseed self-reports as failing. SymCrypt does NOT attempt
// to validate that the values returned in successful Rdseed calls are in fact random.
//

#endif

////////////////////////////////////////////////////////////////////////////////////////////
//
// AES-XTS
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXtsAesExpandKey(
    _Out_               PSYMCRYPT_XTS_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_( cbKey ) PCBYTE                          pbKey,
                        SIZE_T                          cbKey );
// Note that this key expansion function does not perform FIPS checks for backwards compatibility.
// Use SymCryptXtsAesExpandKeyEx for FIPS-approved XTS key expansion.

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXtsAesExpandKeyEx(
    _Out_               PSYMCRYPT_XTS_AES_EXPANDED_KEY  pExpandedKey,
    _In_reads_( cbKey ) PCBYTE                          pbKey,
                        SIZE_T                          cbKey,
                        UINT32                          flags );
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS.
//   Currently this is just checking that 2 AES keys used in XTS are non-equal.

VOID
SYMCRYPT_CALL
SymCryptXtsAesKeyCopy(
    _In_    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pSrc,
    _Out_   PSYMCRYPT_XTS_AES_EXPANDED_KEY  pDst );
//
// Create a copy of an expanded key
//

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncrypt(
    _In_                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                            SIZE_T                          cbDataUnit,
                            UINT64                          tweak,
    _In_reads_( cbData )    PCBYTE                          pbSrc,
    _Out_writes_( cbData )  PBYTE                           pbDst,
                            SIZE_T                          cbData );
//
// Encrypt a buffer using XTS-AES and 64 bit tweak.
//      - pExpandedKey points to the expanded key for XTS.
//      - cbDataUnit: size of each data unit, must be at least 16 and cannot exceed 2^{24} bytes. Typically 512.
//      - tweak: 64 bit tweak value used for the first data unit in the buffer, incremented for subsequent data units.
//      - pbSrc: plaintext input
//      - pbDst: ciphertext output. The ciphertext buffer may be identical to the plaintext
//          buffer, or non-overlapping. The ciphertext is also cbData bytes long.
//      - cbData: # bytes of plaintext input. Must be a multiple of cbDataUnit.
//
// XTS-AES works on equal-sized data units, with each data unit being uniquely encrypted using a combination of
// an integer "tweak" value and the XTS key (a pair of AES keys). A data unit typically corresponds to a sector
// size on a disk.
//
// This API encrypts a buffer consisting of several consecutive data units, which use consecutive tweak values.
// As the tweak is 64 bits, if there is an overflow of 64 bits, the value of the tweak will wrap to 0.
//
// i.e. encryption with tweak 0xffffffffffffffff for a buffer consisting of 2 data units will correspond to:
// encryption using tweak 0xffffffffffffffff for the first data unit,
// encryption using tweak 0x0000000000000000 for the second data unit
//
// Note, using cbDataUnit which is a power of 2 >= 256, will likely be more performant.
//

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecrypt(
    _In_                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                            SIZE_T                          cbDataUnit,
                            UINT64                          tweak,
    _In_reads_( cbData )    PCBYTE                          pbSrc,
    _Out_writes_( cbData )  PBYTE                           pbDst,
                            SIZE_T                          cbData );
//
// Decrypt a buffer using XTS-AES and 64 bit tweak.
// See SymCryptXtsAesEncrypt for a more in depth description, everything is the same, only this decrypts rather than encrypts.
//

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptWith128bTweak(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData );
//
// Encrypt a buffer using XTS-AES and 128 bit tweak.
//      - pExpandedKey points to the expanded key for XTS.
//      - cbDataUnit: size of each data unit, must be at least 16 and cannot exceed 2^{24} bytes. Typically 512.
//      - pbTweak: 128 bit tweak value used for the first data unit in the buffer, incremented for subsequent data units.
//      - pbSrc: plaintext input
//      - pbDst: ciphertext output. The ciphertext buffer may be identical to the plaintext
//          buffer, or non-overlapping. The ciphertext is also cbData bytes long.
//      - cbData: # bytes of plaintext input. Must be a multiple of cbDataUnit.
//
// XTS-AES works on equal-sized data units, with each data unit being uniquely encrypted using a combination of
// an integer "tweak" value and the XTS key (a pair of AES keys). A data unit typically corresponds to a sector
// size on a disk.
//
// This API encrypts a buffer consisting of several consecutive data units, which use consecutive tweak values.
// As the tweak is 128 bits, if there is an overflow of 128 bits, the value of the tweak will wrap to 0.
//
// i.e. encryption with tweak 0x0000000000000000ffffffffffffffff for a buffer consisting of 2 data units will correspond to:
// encryption using tweak 0x0000000000000000ffffffffffffffff for the first data unit,
// encryption using tweak 0x00000000000000010000000000000000 for the second data unit
//  but encryption with tweak 0xffffffffffffffffffffffffffffffff for a buffer consisting of 2 data units will correspond to:
// encryption using tweak 0xffffffffffffffffffffffffffffffff for the first data unit,
// encryption using tweak 0x00000000000000000000000000000000 for the second data unit
//
// Note, using cbDataUnit which is a power of 2 >= 256, will likely be more performant.
//

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptWith128bTweak(
    _In_                                    PCSYMCRYPT_XTS_AES_EXPANDED_KEY pExpandedKey,
                                            SIZE_T                          cbDataUnit,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PCBYTE                          pbTweak,
    _In_reads_( cbData )                    PCBYTE                          pbSrc,
    _Out_writes_( cbData )                  PBYTE                           pbDst,
                                            SIZE_T                          cbData );
//
// Decrypt a buffer using XTS-AES and 128 bit tweak.
// See SymCryptXtsAesEncryptWith128bTweak for a more in depth description, everything is the same, only this decrypts rather than encrypts.
//

VOID
SYMCRYPT_CALL
SymCryptXtsAesSelftest(void);

////////////////////////////////////////////////////////////////////////////////////////////
//
// AES-KW and AES-KWP
//
// These are the AES-KW and AES-KWP algorithms per SP 800-38F.
//
// These are very slow compared to most AES modes, requiring a long serial chain of AES
// block encryption/decryptions, with a best case cost comparable to ~12x AES-CBC encryption
// for a given buffer size. In practice the cost is often higher.
// These cipher modes are not recommended.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwEncrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult );
//
// Encrypt a buffer using AES-KW mode.
//
// - pExpandedKey points to the expanded key to use.
// - pbSrc is the plaintext source buffer. The source and destination buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbSrc. # bytes of plaintext. This must be a multiple of 8, >=16, and <2^31.
// - pbDst is the ciphertext destination buffer. The source and destination buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbDst. # bytes in the destination buffer. This must be >= cbSrc+8.
// - pcbResult pointer to a variable which receives the length of the ciphertext written to pbDst.
//
// Returns:
//      SYMCRYPT_INVALID_ARGUMENT           :   If cbSrc is an invalid size
//      SYMCRYPT_BUFFER_TOO_SMALL           :   If cbDst is not large enough
//                                              (this can always be avoided if cbDst >= cbSrc+8)
//      SYMCRYPT_MEMORY_ALLOCATION_FAILURE  :   If there is insufficient memory for the operation
//      SYMCRYPT_NO_ERROR                   :   On success
//
// Remarks:
//  The standard allows larger plaintexts but there is no requirement to support them, we only support
//  plaintext up to 2^31 bytes because it avoids complexity in handling overflow of 32b buffer sizes, and
//  is larger than practically necessary.
//  The output parameters (pbDst and pcbResult) are only set on success.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwDecrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult );
//
// Decrypt a buffer using AES-KW mode.
//
// - pExpandedKey points to the expanded key to use.
// - pbSrc is the ciphertext source buffer. The source and destination buffers may be
//      identical (in-place decryption) or non-overlapping, but they may not partially overlap.
// - cbSrc. # bytes of ciphertext. This must be a multiple of 8, >=24, and <=2^31.
// - pbDst is the plaintext destination buffer. The source and destination buffers may be
//      identical (in-place decryption) or non-overlapping, but they may not partially overlap.
// - cbDst. # bytes in the destination buffer. This must be >= cbSrc-8.
// - pcbResult pointer to a variable which receives the length of the plaintext written to pbDst.
//
// Returns:
//      SYMCRYPT_INVALID_ARGUMENT           :   If cbSrc is an invalid size
//      SYMCRYPT_BUFFER_TOO_SMALL           :   If cbDst is not large enough
//                                              (this can always be avoided if cbDst >= cbSrc-8)
//      SYMCRYPT_AUTHENTICATION_FAILURE     :   If pbSrc does not decrypt successfully
//      SYMCRYPT_MEMORY_ALLOCATION_FAILURE  :   If there is insufficient memory for the operation
//      SYMCRYPT_NO_ERROR                   :   On success
//
// Remarks:
//  The standard allows larger plaintexts but there is no requirement to support them, we only support
//  plaintext up to 2^31 bytes because it avoids complexity in handling overflow of 32b buffer sizes, and
//  is larger than practically necessary.
//  The output parameters (pbDst and pcbResult) are only set on success.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwpEncrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult );
//
// Encrypt a buffer using AES-KWP mode.
//
// - pExpandedKey points to the expanded key to use.
// - pbSrc is the plaintext source buffer. The source and destination buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbSrc. # bytes of plaintext. This must be >0 and <=2^31-8.
// - pbDst is the ciphertext destination buffer. The source and destination buffers may be
//      identical (in-place encryption) or non-overlapping, but they may not partially overlap.
// - cbDst. # bytes in the destination buffer. This must be >= cbSrc + 16 - (cbSrc%8) - ((cbSrc%8)==0 ? 8 : 0)
// - pcbResult pointer to a variable which receives the length of the ciphertext written to pbDst.
//
// Returns:
//      SYMCRYPT_INVALID_ARGUMENT           :   If cbSrc is an invalid size
//      SYMCRYPT_BUFFER_TOO_SMALL           :   If cbDst is not large enough
//                                              (this can always be avoided if cbDst >= cbSrc+15)
//      SYMCRYPT_MEMORY_ALLOCATION_FAILURE  :   If there is insufficient memory for the operation
//      SYMCRYPT_NO_ERROR                   :   On success
//
// Remarks:
//  The standard allows larger plaintexts but there is no requirement to support them, we only support
//  plaintext up to 2^31 bytes because it avoids complexity in handling overflow of 32b buffer sizes, and
//  is larger than practically necessary.
//  The output parameters (pbDst and pcbResult) are only set on success.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptAesKwpDecrypt(
    _In_                                PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_(cbSrc)                   PCBYTE                      pbSrc,
                                        SIZE_T                      cbSrc,
    _Out_writes_to_(cbDst, *pcbResult)  PBYTE                       pbDst,
                                        SIZE_T                      cbDst,
    _Out_                               SIZE_T*                     pcbResult );
//
// Decrypt a buffer using AES-KWP mode.
//
// - pExpandedKey points to the expanded key to use.
// - pbSrc is the ciphertext source buffer. The source and destination buffers may be
//      identical (in-place decryption) or non-overlapping, but they may not partially overlap.
// - cbSrc. # bytes of ciphertext. This must be a multiple of 8, >=16, and <=2^31.
// - pbDst is the plaintext destination buffer. The source and destination buffers may be
//      identical (in-place decryption) or non-overlapping, but they may not partially overlap.
// - cbDst. # bytes in the destination buffer. This must be large enough to fit the plaintext,
//      a valid plaintext length is in the range [cbSrc-15, cbSrc-8]. If cbDst >= cbSrc-8 then the
//      destination buffer is guaranteed to be large enough.
// - pcbResult pointer to a variable which receives the length of the plaintext written to pbDst.
//
// Returns:
//      SYMCRYPT_INVALID_ARGUMENT           :   If cbSrc is an invalid size
//      SYMCRYPT_BUFFER_TOO_SMALL           :   If cbDst is not large enough
//                                              (this can always be avoided if cbDst >= cbSrc-8)
//      SYMCRYPT_AUTHENTICATION_FAILURE     :   If pbSrc does not decrypt successfully
//      SYMCRYPT_MEMORY_ALLOCATION_FAILURE  :   If there is insufficient memory for the operation
//      SYMCRYPT_NO_ERROR                   :   On success
//
// Remarks:
//  The standard allows larger plaintexts but there is no requirement to support them, we only support
//  plaintext up to 2^31 bytes because it avoids complexity in handling overflow of 32b buffer sizes, and
//  is larger than practically necessary.
//  The output parameters (pbDst and pcbResult) are only set on success.
//
//  If we fail to decrypt due to bad data, we return SYMCRYPT_AUTHENTICATION_FAILURE in constant time with
//  respect to how the decrypted data is corrupted. While there is no known attack on AES-KWP abusing
//  differential timing of different failure cases, being constant time for this is cheap, so is a reasonable
//  hardening measure.
//
//  On success we do not attempt to hide the plaintext length from sidechannels, as this could make it hard
//  for callers with known plaintext length to use precisely sized buffers to decrypt into (i.e. caller
//  knows the valid plaintext is 15 bytes but the API would require caller to provide a 16 byte pbDst). It
//  is expected that in any real use case the length of the plaintext would immediately be used to import the
//  unwrapped key into some other piece of code - so attempting to obscure the plaintext length would not be
//  of any benefit.
//


////////////////////////////////////////////////////////////////////////////////////////////
//
// TLS CBC cipher suites HMAC verification
//
// The TLS cipher suites for block cipher modes (typically CBC) are designed in an unfortunate way.
// The format is:
//      Plaintext | MAC | <padding> | <padding_length>
// Which is then encrypted by the block cipher.
// Plaintext is the data being transferred. MAC is the HMAC value over some header data and the plaintext.
// The padding_length is a byte (range 0-255) that specifies the length of the padding.
// The padding consists of padding_length bytes (up to 255) Each byte is equal to padding_length.
// The padding_length is chosen so that length of the whole structure is a multiple of the block cipher block
// size, so that it can be encrypted with CBC.
//
// The problem is that when decrypting this, the natural code will take actions that depend on the padding_length
// byte before it has been authenticated, and those actions might reveal information about padding_byte. This
// in turn can be used in an attack that lets the attacker decrypt data.
// We are particularly concerned with software side channels, where another thread infers information about what the
// active thread is doing through cache state and other shared CPU state.
//
// To address this issue once and for all, we created an implementation of the HMAC verification with the following
// properties:
// - It verifies the HMAC in the data structure above.
// - This is done in a side-channel safe manner, not revealing anything except whether the structure is valid or not.
// This means that the HMAC computation over the plaintext is constant-time and constant-memory-access pattern
// irrespective of the padding_length; thus this is a fixed-time implementation for variable-sized inputs.
// Similarly, the MAC value has to be extracted from a variable location in the input using a fixed memory access
// pattern.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptTlsCbcHmacVerify(
    _In_                PCSYMCRYPT_MAC  pMacAlgorithm,
    _In_                PVOID           pExpandedKey,
    _Inout_             PVOID           pState,
    _In_reads_(cbData)  PCBYTE          pbData,
                        SIZE_T          cbData);
// Verify a TLS CBC cipher suite MAC value
//  - macAlgorithm: one of SymCryptHmacSha1Algorithm, SymCryptHmacSha256Algorithm, or SymCryptHmacSha384Algorithm.
//      Other MAC algorithms are not supported.
//  - pState points to an SYMCRYPT_HMAC_SHAXXX_STATE. It is allowed to process data into the state before this call,
//      but the total # bytes processed must be < 2^16.
//  - pbData points to a buffer containing the concatenation of plaintext, MAC, padding, and padding_length.
//  - cbData is the size of the buffer.
// Note: callers should pass the entire (plaintext | MAC | padding | padding_length) in a single call to get
// the full side-channel protection.
// This function returns success if the HMAC verification is successful.
// It returns an error if the padding or HMAC verification fails.
// After the call pState is wiped of any sensitive data, just like the SymCryptHmacXxxResult function.
// Callers have to check the padding_length byte pbData[cbData-1] to determine the size of the plaintext.
//



/*

Yes, despite its name, SymCrypt supports asymmetric cryptographic algorithms.
The asymmetric implementations have the following primary design goals:
    - Implement asymmetric cryptographic algorithms like RSA, DSA, DH, ECDSA, ECDH, etc.
    - Protect against all software-based side-channel attacks
    - Protect against those hardware-based side-channel attacks that can be practically protected against in software.
    - High performance, dynamically using CPU features that are available on the current CPU stepping.
    - Support small code and small memory environments.
    - Support environments that need to control memory allocations.

The primary use-case is for SymCrypt to be the crypto library for MS products. This includes high-performance
scenarios such as TLS server termination, and low-footprint uses such as Bootmgr.
SymCrypt supports applications such as firmware updates for embedded CPUs where code and memory
footprint are of overriding importance.

Side channel attacks:
Defence against side channel attacks play an important part in the design and implementation of
SymCrypt. Side channel attacks are a class of attacks on cryptographic systems where the attacker
gets some information about a cryptographic computation in addition to the inputs and outputs.
For example, any of the following information could be retrieved by the attacker:
- The time it takes to perform a computation (either exactly or approximately)
- The power usage over time of the CPU.
- The noise made by the computer's power supply (a function of the CPU power consumption)
- Which cache lines are evicted from the attacker's thread A by a computation in thread B.
These may sound like esoteric attacks, but all of them have been used in practical demonstrations
to attack cryptographic systems.

SymCrypt uses the following API rules to protect against side-channel attacks:
- Information is divided into two classes: public information and private information.
- Public information is allowed to leak through side channels, and the library makes no attempt to hide
    public information.
- Private information is protected against side-channel attacks to the best ability of the library.
Unless otherwise documented, all information is treated as private.
Functions may document that a particular value is "published". This means that the function may use
the value in a way that is not side-channel safe, so any security analysis that considers
side-channel attacks must assume that the published value is public and known by the attacker.

The following information is always assumed to be public, and thus known to any side-channel attacker:
- Which SymCrypt function is being called.
- The location of any of the buffers passed as arguments.
- The size parameter of any buffer passed as an argument.
- Any details that cause a function to return an error.
Thus, it is important that callers who wish to be side-channel safe ensure that their buffer locations and sizes
do not reveal any information, and that they do not make any calls that result in an error, unless there is no
need for secrecy when an error occurs.

Because pointer values are all public (the memory address cannot be hidden on modern CPUs if the buffer is accessed)
side-channel safe code ends up using masked operations, such as masked-copy where the copy is done or not done
depending on a mask parameter to the function.
SymCrypt exposes a set of masked functions that applications can use for their own side-channel safe operations.

The following coding rules are used to protect private information:
- The sequence of instructions executed is independent of private information.
- The sequence of memory operations (read/write) and memory addresses accessed is independent of private information.
- Private information is not used in instructions whose timing may depend on the data being processed.
As far as we know these rules stop all software-based side-channel attacks, and many hardware-based ones.

One remaining line of attack is to feed the algorithm with values that are special. For example, an RSA
decryption may receive a value that contains many zeroes modulo one prime. If the power consumption of the
multiply instruction reveals whether one of the multiplicands is zero, then the attacker might learn
useful information. Note that this is a pure hardware attack, it is not applicable to software attackers.
Protecting against this style of attack is an area that still needs more research. Where applicable we
document the additional protections that SymCrypt provides.


Running with CHKed code:
All binaries that use SymCrypt must build CHKed versions of the binary (linking the CHKed version of SymCrypt)
and perform full test runs on the CHKed version.
Due to the performance and operational requirements, the production-optimized SymCrypt library API cannot
check all buffer sizes or even be fully SAL-annotated.
The necessary size information is simply not available at every call point, and passing
the size information around would add too much overhead.
The CHKed version of the library adds additional code & per-object storage to be able to implement check that
are broadly equivalent to what SAL would normally check.
SAL checks are part of the SDL requirements and need to be done on all Microsoft products.
Though this requirement cannot strictly speaking be satisfied with the SymCrypt library, running the CHKed
version through full validation is the best equivalent, and therefore should be considered mandatory.

Please ensure that the validation runs exercise all the border-cases of largest and smallest sizes, as well as
intermediate sizes for the parameters.

*/


//
// Caller-provided functions
//
// Some of the large-integer and asymmetric algorithm functions use callbacks.
// The callback functions do not have to be functional for binaries that only use the symmetric algorithm
// implementations.
// Use of callbacks is documented in each function that uses them.
//

PVOID
SYMCRYPT_CALL
SymCryptCallbackAlloc( SIZE_T nBytes );
//
// Allocate a buffer of nBytes; returns NULL on failure.
// Returned pointer must be aligned to a multiple of SYMCRYPT_ASYM_ALIGN_VALUE.
//

VOID
SYMCRYPT_CALL
SymCryptCallbackFree( PVOID pMem );
//
// Called by SymCrypt to free a buffer previously allocated by SymCryptCallbackAlloc().
// Note that callers should never call these functions directly. Buffers that were returned
// from the SymCrypt API are freed with SymCryptFree* functions, not this function.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SYMCRYPT_WEAK_SYMBOL
SymCryptCallbackRandom(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer );
//
// Fill the buffer with uniformly distributed random bytes from a cryptographically strong RNG source.
//

PVOID
SYMCRYPT_CALL
SymCryptCallbackAllocateMutexFastInproc(void);
//
// Allocate and initialize a mutex object; returns NULL on failure.
//
// Fast indicates that users of the mutex will only hold it for a short period of time, so it
// is not expected that threads should need to sleep before acquiring the mutex. (i.e. can be
// implemented by a spinlock in kernel mode).
// Inproc indicates the mutex is only used for synchronization between threads in a single process.
//
// Users of the library in contexts where mutexes are not available can set this callback to always
// return NULL, and attempts to use APIs requiring it will fail at runtime.
//

VOID
SYMCRYPT_CALL
SymCryptCallbackFreeMutexFastInproc( _Inout_ PVOID pMutex );
//
// Free a mutex object previously created by SymCryptCallbackAllocateMutexFastInproc
//

VOID
SYMCRYPT_CALL
SymCryptCallbackAcquireMutexFastInproc( _Inout_ PVOID pMutex );
//
// Take exclusive ownership of a mutex object allocated by SymCryptCallbackAllocateMutexFastInproc.
//
// This call must also ensure memory ordering such that stores before the previous call to
// SymCryptCallbackReleaseMutexFastInproc with this mutex are observable by loads after this call.
//

VOID
SYMCRYPT_CALL
SymCryptCallbackReleaseMutexFastInproc( _Inout_ PVOID pMutex );
//
// Relinquish ownership of a mutex object allocated by SymCryptCallbackAllocateMutexFastInproc and
// acquired by SymCryptCallbackAcquireMutexFastInproc.
//

//==============================================================================================
// Object types for high-level API
//
// SYMCRYPT_RSAKEY       A key that stores the information for the RSA algorithms (encryption and signing).
//                       It always contains the RSA parameters / public key, and may or may not contain
//                       the associated private key.
// SYMCRYPT_DLGROUP      A discrete log group to be used for the DSA and DH algorithms. It contains the
//                       group parameters (P,[Q],G) (The prime Q is optional).
// SYMCRYPT_DLKEY        A "discrete log" key that stores the information for the DSA and DH algorithms. It
//                       always contains a public key, and may or may not contain the associated private key.
// SYMCRYPT_ECURVE       An elliptic curve over a prime field. Contains field prime, curve parameters,
//                       and distinguished point (generator).
// SYMCRYPT_ECKEY        An elliptic curve key for the ECDH and ECDSA algorithms. It always contains a
//                       public key, and may or may not contain the associated private key.
//
// See symcrypt_internal.h for structure definitions.
//

//==============================================================================================
// Supported formats and parameters
//

typedef enum _SYMCRYPT_NUMBER_FORMAT {
    SYMCRYPT_NUMBER_FORMAT_LSB_FIRST = 1,
    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST = 2,
} SYMCRYPT_NUMBER_FORMAT;
//
// SYMCRYPT_NUMBER_FORMAT is used to specify the number format for import and export
// of BYTE arrays. We support the following two number formats:
// Let p[0], ..., p[n-1] be an array containing n bytes:
// LSB_FIRST:
//      Value   = \sum_{i=0}^{n-1} p[i] * 2^{8*i}
//              = p[0] + 2^8 * p[1] + 2^{16} * p[2] + ...
//
// MSB_FIRST:
//      Value   = \sum_{i=0}^{n-1} p[n-1-i] * 2^{8*i}
//              = p[n-1] + 2^8 * p[n-2] + 2^{16} * p[n-3] + ...
//

typedef struct _SYMCRYPT_RSA_PARAMS {
    UINT32              version;            // Version of the parameters structure
    UINT32              nBitsOfModulus;     // Number of bits in the modulus
    UINT32              nPrimes;            // Number of primes, 0 if object is only for public key
    UINT32              nPubExp;            // Number of public exponents (typically 1)
} SYMCRYPT_RSA_PARAMS, *PSYMCRYPT_RSA_PARAMS;
typedef const SYMCRYPT_RSA_PARAMS * PCSYMCRYPT_RSA_PARAMS;
//
// SYMCRYPT_RSA_PARAMS is used to specify all the parameters needed for creation of an
// RSA key object. The above is version 1 of the parameters.
// Currently, we only support nPubExp = 1 and nPrimes = 0 or 2.
// Note: nPrimes > 2 and nPubExp > 1 allow faster and more flexible
// RSA functionality. Though currently not supported, these parameters make it easy to add
// support in the future.
//

// Notation for elliptic curve parameters and functions
// ====================================================

//  E       The elliptic curve group. This is typically represented as the set of 2D points (with
//          coordinates from a finite field) that satisfy a specific curve equation.
//          An example equation is y^2 = x^3 + Ax + B for A,B. The set E also
//          contains a special "zero" point denoted by O.
//  |E|     The total number of points on the elliptic curve group E.
//  G       A special point in E which generates a (prime) order subgroup.
//  GOrd    The (prime) order of the generator point G. Therefore, GOrd * G = O.
//  h       The cofactor of the curve. It is defined as h = |E| / GOrd. Typical
//          cofactors are 4 (NUMS curves), and 8 (curve 25519).

// Definitions
// ===========

// A "proper public key" (PPK) on the curve E is defined to be an arbitrary nonzero point of the
// subgroup generated by the point G.

// A "proper secret key" (PSK) is the logarithm of a "proper public key" with
// respect to G. Therefore, if Q is the PPK, then the corresponding PSK is the unique
// integer s with 0 < s < GOrd such that s*G = Q.

// If the cofactor of the curve is equal to 1, then the entire group E is generated by
// the point G and all nonzero points in E are "proper public keys".

// Otherwise, an arbitrary point on the curve might or might not belong to the subgroup
// generated by G. Furthermore, in this case, an arbitrary point P may have order equal
// to the cofactor (or smaller), i.e. h*P=O, or an order larger than GOrd.

// To securely handle the cases where "non-proper" public keys are imported from possibly malicious
// sources, the creators of curve parameters impose several restrictions on the secret keys
// and the algorithms used. For example, the scalar multiplication algorithm for NUMS curves
// always pre-multiplies a point by the cofactor; in order to zero-out any possible
// components of lower order ("low-order clearing"). Curve 25519 imposes this by asserting
// that all secret keys have the 3 lowest bits set to 0, which is equivalent to multiplying
// by h=8.

typedef enum _SYMCRYPT_ECURVE_GEN_ALG_ID {
    SYMCRYPT_ECURVE_GEN_ALG_ID_NULL = 0,
} SYMCRYPT_ECURVE_GEN_ALG_ID;
//
// SYMCRYPT_ECURVE_GEN_ALG_ID is used to specify (if available) the algorithm that
// generates the curve parameters from the provided seed.
//


typedef struct _SYMCRYPT_ECURVE_PARAMS_V2_EXTENSION {
    UINT32  PrivateKeyDefaultFormat;
    UINT32  HighBitRestrictionNumOfBits;
    UINT32  HighBitRestrictionPosition;
    UINT32  HighBitRestrictionValue;
} SYMCRYPT_ECURVE_PARAMS_V2_EXTENSION, *PSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION;
typedef const SYMCRYPT_ECURVE_PARAMS_V2_EXTENSION * PCSYMCRYPT_ECURVE_PARAMS_V2_EXTENSION;
//
// SYMCRYPT_ECURVE_PARAMS_V2_EXTENSION is used to specify restrictions and default formats
// for known curves. The possible formats and restriction are explained below.
//

// Secret key formats
// ==================
// The possible secret key formats in SymCrypt are shown below. For all formats, s denotes
// a "proper secret key" defined as above. I.e. 0 < s < GOrd.
//
//  1. "Canonical":         s
//  2. "DivH":              s/h mod GOrd
//  3. "DivHTimesH":        h*(s/h mod GOrd)
//  4. "TimesH":            h*s                 <-- This format is currently unsupported
//
// Remarks:
//  -   The above formats apply **only to external formats**: When somebody is
//      importing a secret key (from test vectors, for example) or exporting a key.
//      The internal format of the secret keys might be one of them or something totally
//      different; the internal format is not visible to the caller.
//  -   Formats 3 and 4 have bigger storage requirements compared to 1 and 2, as
//      the key can be up to |E|.
//  -   When h=1 all formats are identical. This is the case for NIST curves.
//  -   The NUMS curves use the "DivH" secret key format in the test vectors and the
//      multiplication algorithm implicitly multiplies by h.
//  -   Curve 25519 uses the "DivHTimesH" secret key format in the test vectors.
typedef enum _SYMCRYPT_ECKEY_PRIVATE_FORMAT {
    SYMCRYPT_ECKEY_PRIVATE_FORMAT_NULL           = 0,
    SYMCRYPT_ECKEY_PRIVATE_FORMAT_CANONICAL      = 1,
    SYMCRYPT_ECKEY_PRIVATE_FORMAT_DIVH           = 2,
    SYMCRYPT_ECKEY_PRIVATE_FORMAT_DIVH_TIMESH    = 3,
} SYMCRYPT_ECKEY_PRIVATE_FORMAT;

// High bit restrictions
// =====================
// A high bit restriction is a requirement for some of the high bits of the secret keys
// (usually the most significant bits of the curve).
// Currently only curve 25519 imposes such a restriction: That the bits 255 and 254 of the
// secret key in the "DivHTimesH" format are 0 and 1, respectively.
//
// The high bit restrictions specification takes the following form:
//     - Number of bits that are specified
//     - Bit position of the lowest bit to be specified (starting from 0 for the LSB)
//     - The bit values
// The bits that are specified refer to the relevant secret key format.
// For Canonical and DivH formats the total number of bits is the # bits of GOrd-1.
// For DivHTimesH and TimesH formats the total number of bits is the # bits of |E|-1.
//
// Note: as GOrd must be prime, #bits(Gord) == #bits(Gord-1). The same is true
// for |E|=h*GOrd as it cannot be a power of 2.
//
// The HighBitRestrictionNumOfBits field is a value between 0 and 32 (inclusive)
// and specifies how many bits of the HighBitRestrictionValue are used (starting
// from the least significant bit of the value). The bits that are restricted are
// the bits [HighBitRestrictionPosition+HighBitRestrictionNumOfBits-1, ..., HighBitRestrictionPosition]
//
//      For example, let's assume it is required that the bits [104, 103, ..., 100]
//      of all private keys of a curve are always 11011.
//      Then the parameters should be set to
//              HighBitRestrictionNumOfBits = 5
//              HighBitRestrictionPosition = 100
//              HighBitRestrictionValue = 0x1B
//


typedef struct _SYMCRYPT_ECURVE_PARAMS {
    UINT32                      version;            // Version of the parameters structure (see comment below)
    SYMCRYPT_ECURVE_TYPE        type;               // Type of the curve
    SYMCRYPT_ECURVE_GEN_ALG_ID  algId;              // Algorithm ID for generation of parameters from seed
    UINT32                      cbFieldLength;      // Length of the field elements in bytes
    UINT32                      cbSubgroupOrder;    // Length of the subgroup in bytes
    UINT32                      cbCofactor;         // Length of the cofactor in bytes
    UINT32                      cbSeed;             // Length of the seed
    // This struct is followed in memory by:
    //P[cbFieldLength]      Prime of the base field
    //A[cbFieldLength]      Coefficient A of all three types of curves
    //B[cbFieldLength]      Coefficient B of Weierstrass and Montgomery curves and D for Twisted Edwards curves
    //Gx[cbFieldLength]     X-coordinate of the distinguished point (assuming SYMCRYPT_ECPOINT_FORMAT_XY)
    //Gy[cbFieldLength]     Y-coordinate of the distinguished point (assuming SYMCRYPT_ECPOINT_FORMAT_XY)
    //n[cbSubGroupOrder]    Order of the subgroup generated by the distinguished point
    //h[cbCofactor]         Cofactor of the distinguished point
    //S[cbSeed]             Seed of the curve

    //ParamsV2Extension[sizeof(SYMCRYPT_ECURVE_PARAMS_V2_EXTENSION)];  // Only on version 2 of the parameters
} SYMCRYPT_ECURVE_PARAMS, *PSYMCRYPT_ECURVE_PARAMS;
typedef const SYMCRYPT_ECURVE_PARAMS * PCSYMCRYPT_ECURVE_PARAMS;
//
// SYMCRYPT_ECURVE_PARAMS is used to specify all the parameters needed for the curve generation. The above
// are versions 1 and 2 of the curve parameters.
//

typedef enum _SYMCRYPT_ECPOINT_FORMAT {
    SYMCRYPT_ECPOINT_FORMAT_X   = 1,   // One value, encoding the X coordinate only of a point
    SYMCRYPT_ECPOINT_FORMAT_XY  = 2,   // Two equally-sized values, the first one encoding X and the second one encoding Y
} SYMCRYPT_ECPOINT_FORMAT;
//
// SYMCRYPT_ECPOINT_FORMAT is used to support different elliptic curve point formats, including possible point compression.
//

//========================================================================
//========================================================================
// Main schema for object creation, deletion, and management.
//
// Object management is the same for most object types. For an object type XXX we have
// the following functions:
//
// PSYMCRYPT_XXX
// SYMCRYPT_CALL
// SymCryptXxxAllocate( <size parameters> )
//  Allocates an object of type XXX according to the specified size parameters.
//  If the allocation fails, NULL is returned.
//  If the allocation succeeds, an XXX pointer is returned, and the caller is responsible
//  for freeing the result using SymCryptXxxFree().
//  The value of the new object is undefined.
//  All the parameters to this function are published. (Object sizes cannot be private information.)
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxFree( _Inout_ PSYMCRYPT_XXX p )
//  Free an XXX object allocated with SymCryptAllocateXxx().
//  Any storage location in the object that might have contained private information is wiped.
//
// UINT32
// SYMCRYPT_CALL
// SymCryptSizeofXxxFromYyy( <size parameters> );
//  Memory size that is sufficient to store an XXX object with size defined by the <size parameters>.
//  The Yyy specifies the form of the size parameters, for example Ecurve.
//  This is a runtime function as the size of an object is a run-time decision dependent on the CPU stepping.
//  The result is always a multiple of the alignment requirements of this object type, so arrays can be built
//  using this element size.
//
// SYMCRYPT_SIZEOF_XXX_FROM_YYY( <size parameters> )
//  This is a compile-time macro that computes a value not less than the SymCryptSizeofXxxFromYyy function, and
//  is suitable to statically compute the size of a memory buffer for an object.
//  (Not defined for all types.)
//
// PSYMCRYPT_XXX
// SYMCRYPT_CALL
// SymCryptXxxCreate(
//      _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
//                                      SIZE_T  cbBuffer,
//                                      <size parameters> );
//  Create an XXX object from the provided (pbBuffer, cbBuffer) space.
//  This function performs the necessary initializations of the object, but does not assign or set a value.
//  The object will be able to store values up to size determined by the <size parameters>.
//  Requirement:
//      - pbBuffer is aligned to SYMCRYPT_ASYM_ALIGN_VALUE. Note that this can be a stricter requirement than
//          SYMCRYPT_ALIGNED, and memory allocation functions might not return pointers that are suitably
//          aligned. For some object types and some CPUs, the alignment requirements might be less strict.
//          The main purpose of this relaxation is to always allow objects that are spaced
//          SymCryptSizeofXxxFromYyy apart. The common usage is to create an array of objects. The array
//          starts at a SYMCRYPT_ASYM_ALIGNed location, with each element SymCryptSizeofXxxFromYyy(..) bytes long.
//      - cbBuffer >= SymCryptSizeofXxxFromYyy( <size parameters> )
//      - (pbBuffer,cbBuffer) memory must be exclusively used by this object.
//  The last requirement ensures that all objects are non-overlapping (except for API functions
//  that explicitly create overlapping objects).
//  All parameters are published.
//  It is always safe to choose
//      cbBuffer = SymCryptSizeofXxxFromYyy( <size parameters> )
//  The returned object pointer is simply a cast of the pbBuffer pointer.
//  Callers that manage arrays of objects can reconstruct the PSYMCRYPT_XXX by casting the buffer pointer
//  to the right type.
//  An object that is created with this function should be wiped, even if it doesn't contain private data.
//  The SymCryptXxxWipe() function also frees any associated data that the library may maintain.
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxWipe( _Out_ PSYMCRYPT_XXX  Dst )
//  All private information in the Dst object is wiped, and any associated data is freed.
//  Unless otherwise specified, the Dst object is left in an undefined state.
//  An SymCryptXxxAllocate-d object does not have to be wiped before it is freed
//  because the SymCryptXxxFree function will perform the wipe.
//  However, SymCryptXxxCreate-d objects should always be wiped even if they don't contain
//  secret data, as the wipe also frees any associated data the library may maintain.
//
// VOID
// SYMCRYPT_CALL
// SymCryptXxxCopy(
//      _In_ PCSYMCRYPT_XXX    pxSrc,
//      _Out_PSYMCRYPT_XXX     pxDst );
// Dst = Src.
//  Requirement: The <size parameters> of both objects should the same.
//  Src must be in a defined state, it is not valid to copy an undefined object.
//  Src and Dst may be the same object (though that is a no-op).
//

//========================================================================
// RSAKEY objects' API
//

#define SYMCRYPT_SIZEOF_RSAKEY_FROM_PARAMS( modBits, nPrimes, nPubExps ) \
    SYMCRYPT_INTERNAL_SIZEOF_RSAKEY_FROM_PARAMS( modBits, nPrimes, nPubExps )
// Return a buffer size large enough to create an RSA key in which the specified
// modulus size, # primes, # public exponents, and upper bound for the bitsize of each public exponent.
// If the object will only contain a public key, nPrimes can be set to 0

PSYMCRYPT_RSAKEY
SYMCRYPT_CALL
SymCryptRsakeyAllocate(
    _In_    PCSYMCRYPT_RSA_PARAMS   pParams,
    _In_    UINT32                  flags );
//
// Allocate and create a new RSAKEY object sized according to the parameters.
// If the SYMCRYPT_RSAKEY object will only be used for a public key, the
// SYMCRYPT_RSA_PARAMS structure may set nPrimes = 0. Use of
// SymCryptRsakeySetValueFromPrivateExponent requires nPrimes = 2.
//
// This call does not initialize the key. It should be
// followed by a call to SymCryptRsakeyGenerate or
// SymCryptRsakeySetValue*.
//
// No flags are specified for this function.
//

VOID
SYMCRYPT_CALL
SymCryptRsakeyFree( _Out_ PSYMCRYPT_RSAKEY pkObj );

UINT32
SYMCRYPT_CALL
SymCryptSizeofRsakeyFromParams( _In_ PCSYMCRYPT_RSA_PARAMS pParams );
// If the to-be-allocated SYMCRYPT_RSAKEY object will only be used for a public key, the
// SYMCRYPT_RSA_PARAMS structure may set nPrimes = 0.

PSYMCRYPT_RSAKEY
SYMCRYPT_CALL
SymCryptRsakeyCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE                   pbBuffer,
                                    SIZE_T                  cbBuffer,
    _In_                            PCSYMCRYPT_RSA_PARAMS   pParams );
//
// Create an RSAKEY object from a buffer, but does not initialize it.
// If the SYMCRYPT_RSAKEY object will only be used for a public key, the
// SYMCRYPT_RSA_PARAMS structure may set nPrimes = 0. Use of
// SymCryptRsakeySetValueFromPrivateExponent requires nPrimes = 2.
//
// This call does not initialize the key. It should be
// followed by a call to SymCryptRsakeyGenerate or
// SymCryptRsakeySetValue*.
//

VOID
SYMCRYPT_CALL
SymCryptRsakeyWipe( _Out_ PSYMCRYPT_RSAKEY pkDst );

//
//VOID
//SYMCRYPT_CALL
//SymCryptRsakeyCopy(
//    _In_    PCSYMCRYPT_RSAKEY  pkSrc,
//    _Out_   PSYMCRYPT_RSAKEY   pkDst );
//
// This function is currently not available.
//

//========================================================================
// DLGROUP objects' API
//

PSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlgroupAllocate( UINT32  nBitsOfP, UINT32  nBitsOfQ );
//
// Allocate a Discrete Logarithm group object suitable for the given sizes.
//
// nBitsOfP: Maximum number of bits of the field prime P. Specifying a value larger
//  than the actual size is allowed, but inefficient.
// nBitsOfQ: Maximum number of bits of the group order Q. Specify the size of Q,
//  or 0 if the size of Q is not (yet) known.
//
// This call does not initialize the DLGROUP. It should be followed
// by a call to SymCryptDlgroupGenerate or SymCryptDlgroupSetValue.
//
// nBitsOfQ is allowed to be equal to 0 and signifies that the size of Q
// is unknown or Q does not exist. This may be used when creating a DLGROUP
// for the DH algorithm which does not use a prime Q.
//
// Setting nBitsOfQ to something bigger than 0 signifies that the size of
// the prime Q is known and if a future caller tries to import a bigger Q then
// the SymCryptDlgroupSetValue call will fail.
//
// Technically nBitsOfQ should always be strictly less than nBitsOfP, as Q divides
// P-1. For simplicity, it is allowed that callers specify nBitsOfQ equal to nBitsOfP
// in this call, but SymCrypt will treat this as setting nBitsOfQ to (nBitsOfP-1).
//
// Setting nBitsOfQ to 0 might result in a bigger size of the DLGROUP object
// compared to setting it to a specific size (see SymCryptSizeofDlgroupFromBitsizes).
//
// Requirements:
//  - nBitsOfP >= nBitsOfQ
//

VOID
SYMCRYPT_CALL
SymCryptDlgroupFree( _Out_ PSYMCRYPT_DLGROUP pgObj );

UINT32
SYMCRYPT_CALL
SymCryptSizeofDlgroupFromBitsizes( UINT32 nBitsOfP, UINT32 nBitsOfQ );
//
// This call returns the memory size that is sufficient to store a
// DLGROUP object with primes P,Q of size nBitsOfP and nBitsOfQ,
// respectively (L,N parameters in FIPS 186-3 specs).
//
// Requirements:
//  - nBitsOfP >= nBitsOfQ
//
// Remarks:
//  - The value in nBitsOfQ is allowed to be equal to 0
//  (see SymCryptDlgroupAllocate).
//
//  - When nBitsOfQ!=0 this is a monotonic function w.r.t. a partial order on N^2.
//    I.e. for all fixed (nBitsOfP_0,nBitsOfQ_0) and (nBitsOfP_1,nBitsOfQ_1) with
//       nBitsOfQ_0>0 and nBitsOfQ_1>0,
//
//      (nBitsOfP_0<=nBitsOfP_1 AND nBitsOfQ_0<=nBitsOfQ_1) implies that
//          F(nBitsOfP_0,nBitsOfQ_0) <= F(nBitsOfP_1,nBitsOfQ_1)
//      where F is the function SymCryptSizeofDlgroupFromBitsizes.
//
//  - F(nBitsOfP, 0)=F(nBitsOfP, nBitsOfP-1). Thus when nBitsOfQ==0 the
//    function takes the maximum value for a fixed nBitsOfP.
//

PSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlgroupCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
                                    UINT32              nBitsOfP,
                                    UINT32              nBitsOfQ );
//
// Creates a DL group object, but does not initialize it. It must be followed
// by a call to SymCryptDlgroupGenerate or SymCryptDlgroupSetValue.
//
// - pbBuffer,cbBuffer: memory buffer to create the object out of. The required size
//  can be computed with SymCryptSizeofDlgroupFromBitsizes().
// - nBitsOfP: number of bits of the field prime P.
// - nBitsOfQ: number of bits of the group order Q, or 0 if the size of Q is not (yet) known.
//

VOID
SYMCRYPT_CALL
SymCryptDlgroupWipe( _Out_ PSYMCRYPT_DLGROUP pgDst );

VOID
SYMCRYPT_CALL
SymCryptDlgroupCopy(
    _In_    PCSYMCRYPT_DLGROUP   pgSrc,
    _Out_   PSYMCRYPT_DLGROUP    pgDst );

//========================================================================
// DLKEY objects' API
//

PSYMCRYPT_DLKEY
SYMCRYPT_CALL
SymCryptDlkeyAllocate( _In_ PCSYMCRYPT_DLGROUP pDlgroup );
//
// This call does not initialize the key. It should be
// followed by a call to SymCryptDlkeyGenerate or
// SymCryptDlkeySetValue.
//

VOID
SYMCRYPT_CALL
SymCryptDlkeyFree( _Out_ PSYMCRYPT_DLKEY pkObj );

UINT32
SYMCRYPT_CALL
SymCryptSizeofDlkeyFromDlgroup( _In_ PCSYMCRYPT_DLGROUP pDlgroup );

PSYMCRYPT_DLKEY
SYMCRYPT_CALL
SymCryptDlkeyCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
    _In_                            PCSYMCRYPT_DLGROUP  pDlgroup );

VOID
SYMCRYPT_CALL
SymCryptDlkeyWipe( _Out_ PSYMCRYPT_DLKEY pkDst );

VOID
SYMCRYPT_CALL
SymCryptDlkeyCopy(
    _In_    PCSYMCRYPT_DLKEY   pkSrc,
    _Out_   PSYMCRYPT_DLKEY    pkDst );

//========================================================================
// ECURVE objects' API is slightly different than the above API schema because of the close
// relation to multiple parameters, the fact that they contain public information,
// and that they are persisted by the callers.
// Thus, the Allocate function takes in all the curve parameters and there are no Create,
// Wipe, or Copy functions.
//

PSYMCRYPT_ECURVE
SYMCRYPT_CALL
SymCryptEcurveAllocate(
    _In_    PCSYMCRYPT_ECURVE_PARAMS    pParams,
    _In_    UINT32                      flags );
//
// Allocate memory and create an ECURVE object which is defined
// by the parameters in pParams.
//
// - pParams: parameters that define the curve
// - flags: Not used, must be zero.
//
// Future versions might use the flags to enable different features/tradeoffs.
// There are a number of interesting memory/speed/pre-computation cost trades that can be made.
// For example, pre-computing multiples of the distinguished point, or (parallel?) pre-computation
// of (r, rG) pairs for random r values.
//
// This function applies limited validation of the pParams. The validation is intended to eliminate
// the threat of denial-of-service when hostile parameters are presented. It does not ensure that
// the parameters make sense, define a proper curve, or that any elliptic-curve operations made on
// the curve built from these parameters will fail, succeed or provide any security.
// The only guarantee provided for invalid parameters is that all operations on this curve will
// not crash and will return in some reasonable amount of time.
//
// Returns NULL if out of memory or the parameters are deemed invalid.
// If the return value is not NULL, the object must later be freed with SymCryptEcurveFree().
//

VOID
SYMCRYPT_CALL
SymCryptEcurveFree( _Out_ PSYMCRYPT_ECURVE pCurve );

//========================================================================
// ECKEY objects' API is slightly different than the above API schema in the sense that they
// take as input an ECURVE object pointer instead of the number of digits.
//

PSYMCRYPT_ECKEY
SYMCRYPT_CALL
SymCryptEckeyAllocate( _In_  PCSYMCRYPT_ECURVE pCurve );

VOID
SYMCRYPT_CALL
SymCryptEckeyFree( _Out_ PSYMCRYPT_ECKEY pkObj );

UINT32
SYMCRYPT_CALL
SymCryptSizeofEckeyFromCurve( _In_ PCSYMCRYPT_ECURVE pCurve );

PSYMCRYPT_ECKEY
SYMCRYPT_CALL
SymCryptEckeyCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE               pbBuffer,
                                    SIZE_T              cbBuffer,
                                    PCSYMCRYPT_ECURVE   pCurve );

VOID
SYMCRYPT_CALL
SymCryptEckeyWipe( _Out_ PSYMCRYPT_ECKEY pkDst );

VOID
SymCryptEckeyCopy(
    _In_    PCSYMCRYPT_ECKEY  pkSrc,
    _Out_   PSYMCRYPT_ECKEY   pkDst );


//=====================================================
// Flags for asymmetric key generation and import

// These flags are introduced primarily for FIPS purposes. For FIPS 140-3 rather than expose to the
// caller the specifics of what tests will be run with various algorithms, we are sanitizing flags
// provided on asymmetric key generation and import to enable the caller to indicate their intent,
// and for SymCrypt to perform the required testing.
// Below we define the flags that can be passed and when a caller should set them.
// The specifics of what tests will be run are likely to change over time, as FIPS requirements and
// our understanding of how best to implement them, change over time. Callers should not rely on
// specific behavior.


// Validation required by FIPS is enabled by default. This flag enables a caller to opt out of this
// validation.
#define SYMCRYPT_FLAG_KEY_NO_FIPS               (0x100)

// When opting out of FIPS, SymCrypt may still perform some sanity checks on key import
// In very performance sensitive situations where a caller strongly trusts the values it is passing
// to SymCrypt and does not care about FIPS (or can statically prove properties about the imported
// keys), a caller may specify SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION in addition to
// SYMCRYPT_FLAG_KEY_NO_FIPS to skip costly checks
#define SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION    (0x200)

// Callers must specify what algorithm(s) a given asymmetric key will be used for.
// This information will be tracked by SymCrypt, and attempting to use the key in an algorithm it
// was not generated or imported for will result in failure.
// If no algorithm is specified then the key generation or import function will fail.
#define SYMCRYPT_FLAG_DLKEY_DSA         (0x1000)
#define SYMCRYPT_FLAG_DLKEY_DH          (0x2000)

#define SYMCRYPT_FLAG_ECKEY_ECDSA       (0x1000)
#define SYMCRYPT_FLAG_ECKEY_ECDH        (0x2000)

#define SYMCRYPT_FLAG_RSAKEY_SIGN       (0x1000)
#define SYMCRYPT_FLAG_RSAKEY_ENCRYPT    (0x2000)

//=====================================================
// RSA key operations

BOOLEAN
SYMCRYPT_CALL
SymCryptRsakeyHasPrivateKey( _In_ PCSYMCRYPT_RSAKEY pkRsakey );
//
// Returns TRUE if the pkRsakey object has private key information.
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofModulus( _In_ PCSYMCRYPT_RSAKEY pkRsakey );
//
//  Returns the (tight) size in bytes of a byte array big enough to store
//  the modulus of the key.
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeyModulusBits( _In_ PCSYMCRYPT_RSAKEY pkRsakey );
//
// Return the number of bits in the RSA modulus
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofPublicExponent(
    _In_    PCSYMCRYPT_RSAKEY pRsakey,
            UINT32            index );
//
// Returns the (tight) size in bytes of a byte array big enough to store
// the public exponent. The index specifies the index
// of the public exponent, starting with 0.
//
// Remarks:
// - Currently, only one public exponent is supported, i.e. the only
//  valid index is 0.
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeySizeofPrime(
    _In_    PCSYMCRYPT_RSAKEY pkRsakey,
            UINT32            index );
//
//  Returns the (tight) size in bytes of a byte array big enough to store
//  the selected prime of the key. The index specifies the index of the
//  prime, starting at 0.
//
//  Remarks:
//  - Currently, only two prime RSA is supported, i.e. the only
//  valid indexes are 0 and 1.
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeyGetNumberOfPublicExponents( _In_ PCSYMCRYPT_RSAKEY pkRsakey );
//
//  Returns the number of public exponents stored in the key.
//

UINT32
SYMCRYPT_CALL
SymCryptRsakeyGetNumberOfPrimes( _In_ PCSYMCRYPT_RSAKEY pkRsakey );
//
//  Returns the number of primes stored in the key.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGenerate(
    _Inout_                     PSYMCRYPT_RSAKEY    pkRsakey,
    _In_reads_opt_( nPubExp )   PCUINT64            pu64PubExp,
                                UINT32              nPubExp,
    _In_                        UINT32              flags );
//
// Generate a new random RSA key using the information from the
// parameters passed to SymCryptRsaKeyAllocate/SymCryptRsaKeyCreate.
// PubExp is the array of nPubExp public exponent values, specifying
// the public exponents for the key.
// nPubExp must match the # public exponents in the parameters.
// If pu64PubExp == NULL, nPubExp == 0, and the key requires only one
// public exponent, then the default exponent 2^16 + 1 is used.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - At least one of the flags indicating what the Rsakey is to be used for must be specified:
//      SYMCRYPT_FLAG_RSAKEY_SIGN
//      SYMCRYPT_FLAG_RSAKEY_ENCRYPT

// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeySetValue(
    _In_reads_bytes_( cbModulus )   PCBYTE                  pbModulus,
                                    SIZE_T                  cbModulus,
    _In_reads_( nPubExp )           PCUINT64                pu64PubExp,
                                    UINT32                  nPubExp,
    _In_reads_opt_( nPrimes )       PCBYTE *                ppPrimes,
    _In_reads_opt_( nPrimes )       SIZE_T *                pcbPrimes,
                                    UINT32                  nPrimes,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
                                    UINT32                  flags,
    _Inout_                         PSYMCRYPT_RSAKEY        pkRsakey );
//
// Import key material to an RSAKEY object. The arguments are the following:
//  - pbModulus is a pointer to a byte buffer of cbModulus bytes. It cannot be NULL.
//  - pu64PubExp is a pointer to an array of nPubExp UINT64 exponent values.
//    nPubExp must match the RSA parameters used to create the key object.
//  - ppPrimes is an array of nPrimes pointers that point to byte buffers storing
//    the primes. pcbPrimes is an array of nPrimes sizes such that
//    the size of ppPrimes[i] is equal to pcbPrimes[i] for each i in [0, nPrimes-1].
//  - numFormat specifies the number format for all inputs
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// - At least one of the flags indicating what the Rsakey is to be used for must be specified:
//      SYMCRYPT_FLAG_RSAKEY_SIGN
//      SYMCRYPT_FLAG_RSAKEY_ENCRYPT
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//
//  Remarks:
//  - Modulus and all primes are stored in the same format specified by numFormat.
//  - ppPrimes, pcbPrimes, and nPrimes can be NULL, NULL, and 0 respectively, when
//    importing a public key.
//  - Currently, the only acceptable value of nPubExps is 1.
//  - Currently, the only acceptable value of nPrimes is 2 or 0.
//  - Elements of ppPrimes must represent prime numbers.
// We allow separate sizes for each prime. This seems redundant because all primes
// are approximately the same size. However, some storage/encoding formats, such as ASN.1,
// strip leading zeroes, or add an additional leading zero depending on the situation.
// Allowing separate sizes avoids the need for the caller to make a copy of the data
// into a possibly slightly larger buffer.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeySetValueFromPrivateExponent(
    _In_reads_bytes_( cbModulus )           PCBYTE                  pbModulus,
                                            SIZE_T                  cbModulus,
                                            UINT64                  u64PubExp,
    _In_reads_bytes_( cbPrivateExponent )   PCBYTE                  pbPrivateExponent,
                                            SIZE_T                  cbPrivateExponent,
                                            SYMCRYPT_NUMBER_FORMAT  numFormat,
                                            UINT32                  flags,
    _Inout_                                 PSYMCRYPT_RSAKEY        pkRsakey );
//
// Import private key to an RSAKEY object using a private exponent. This is not generally
// recommended - where possible it is more efficient to import a private key using primes
// with SymCryptRsakeySetValue.
//
// The arguments are the following:
//  - pbModulus is a pointer to a byte buffer of cbModulus bytes. It cannot be NULL.
//  - u64PubExp is a UINT64 public exponent value.
//  - pbPrivateExponent is a pointer to a byte buffer of cbPrivateExponent bytes. It
//    cannot be NULL.
//  - numFormat specifies the number format for all inputs
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// - At least one of the flags indicating what the Rsakey is to be used for must be specified:
//      SYMCRYPT_FLAG_RSAKEY_SIGN
//      SYMCRYPT_FLAG_RSAKEY_ENCRYPT
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//
// Remarks:
//
// Modulus and Private exponent are stored in the same format specified by numFormat.
//
// Internally this attempts to recover a pair of primes (p1, p2) that factorize Modulus.
// This procedure has following assumptions:
//      Modulus (n) is the product of two prime factors, p1 and p2
//      e*d == 1 modulo LCM(p1-1, p2-1)
//      e*d != 1 modulo 2^64
// If any of these assumptions are not met, then the method may fail.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGetValue(
    _In_                            PCSYMCRYPT_RSAKEY       pkRsakey,
    _Out_writes_bytes_( cbModulus ) PBYTE                   pbModulus,
                                    SIZE_T                  cbModulus,
    _Out_writes_opt_( nPubExp )     PUINT64                 pu64PubExp,
                                    UINT32                  nPubExp,
    _Out_writes_opt_( nPrimes )     PBYTE *                 ppPrimes,
    _In_reads_opt_( nPrimes )       SIZE_T *                pcbPrimes,
                                    UINT32                  nPrimes,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
                                    UINT32                  flags );
//
// Export key material from an RSAKEY object. The arguments are the following:
//  - pbModulus is a pointer to a byte buffer of cbModulus bytes.
//  - pu64PubExp is an pointer to an array of nPubExp elements that receives the public exponent values.
//    nPubExp must match the # public exponents in pkRsaKey.
//  - ppPrimes is an array of nPrimes pointers that point to byte buffers storing
//    the primes. pcbPrimes is an array of nPrimes sizes such that
//    the size of ppPrimes[i] is equal to pcbPrimes[i] for each i in [0, nPrimes-1].
//  Remarks:
//  - All parameters are stored in the same format specified by numFormat.
//  - ppPrimes, pcbPrimes, and nPrimes can be NULL, NULL, and 0 respectively, when
//    exporting a public key.
//  - Currently, the only acceptable value of nPubExp is 1 or 0.
//  - Currently, the only acceptable value of nPrimes is 2 or 0.
// We use separate sizes for each prime. This supports the tight encoding
// used by CNG export blobs, and uses the same format as RsakeySetValue
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyGetCrtValue(
    _In_                                        PCSYMCRYPT_RSAKEY       pkRsakey,
    _Out_writes_opt_(nCrtExponents)             PBYTE *                 ppCrtExponents,
    _In_reads_(nCrtExponents)                   SIZE_T *                pcbCrtExponents,
                                                UINT32                  nCrtExponents,
    _Out_writes_bytes_opt_(cbCrtCoefficient)    PBYTE                   pbCrtCoefficient,
                                                SIZE_T                  cbCrtCoefficient,
    _Out_writes_bytes_opt_(cbPrivateExponent)   PBYTE                   pbPrivateExponent,
                                                SIZE_T                  cbPrivateExponent,
                                                SYMCRYPT_NUMBER_FORMAT  numFormat,
                                                UINT32                  flags);
//
// Export Crt key material from an RSAKEY object. The arguments are the following:
//    ppCrtExponents is an array of nCrtExponent pointers that point to byte buffers
//    storing the Crt exponents. That is,  d mod p-1, d mod q-1.
//    pcbCrtExponents is an array of nCrtExponent sizes such that
//    the size of ppCrtExponents[i] is equal to pcbCrtExponents[i] for each i in [0, nCrtExponent-1]
//    pbCrtCoefficient is a pointer to a byte buffer of cbCrtCoefficient bytes, that is q^{-1} mod p
//    pbPrivateExponent is a pointer to a byte buffer of cbPrivateExponent bytes, that is, d.

//  Remarks:
//  - All parameters are stored in the same format specified by numFormat.
//  - ppCrtExponents, pcbCrtExponents, and nCrtExponent can be NULL, NULL, and 0 respectively
//  - Currently, the only acceptable value of nCrtExponent is 2 or 0.
//    pbCrtCoefficient, pbPrivateExponent can be NULL;

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsakeyExtendKeyUsage(
    _Inout_ PSYMCRYPT_RSAKEY    pkRsakey,
            UINT32              flags );
//
// Enable an existing key which has been generated or imported to be used in specified algorithms.
// Some callers may not know at key generation or import time what algorithms a key will be used for
// and this API allows the key to be extended for use in additional algorithms. Use of this API may
// not be compliant with FIPS 140-3
//
// - flags must be some bitwise OR of the following flags:
//      SYMCRYPT_FLAG_RSAKEY_SIGN
//      SYMCRYPT_FLAG_RSAKEY_ENCRYPT

#define SYMCRYPT_DLGROUP_FIPS_LATEST    (SYMCRYPT_DLGROUP_FIPS_186_3)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGenerate(
    _In_    PCSYMCRYPT_HASH         hashAlgorithm,
    _In_    SYMCRYPT_DLGROUP_FIPS   fipsStandard,
    _Inout_ PSYMCRYPT_DLGROUP       pDlgroup );
//
// Generate a Discrete Logarithm Group for use in Diffie-Hellman and DSA.
//
// - hashAlgorithm: Hash algorithm to be used for generating the group (if required by the algorithm)
// - fipsStandard: Which FIPS standard algorithm to use for generating the group.
// - pDlgroup: group object that will be initialized with a newly generated group.
//
// pDlGroup must have been created with SymCryptDlgroupAllocate() or SymCryptDlgroupCreate().
//
// If nBitsOfQ was equal to 0 when the DLGROUP was Allocate-d/Create-d
// (and only in this case), then this function picks a default size
// for the prime Q according to the following table:
//      - If        nBitsOfP <=  160 then the function fails with SYMCRYPT_FIPS_FAILURE
//      - If  160 < nBitsOfP <= 1024 then nBitsOfQ = 160
//      - If 1024 < nBitsOfP <= 2048 then nBitsOfQ = 256
//      - If 2048 < nBitsOfP         then nBitsOfQ = 256
//
// If fipsStandard == SYMCRYPT_DLGROUP_FIPS_NONE then no FIPS compliance is requested.
// The code defaults to SYMCRYPT_DLGROUP_FIPS_LATEST.
//
// The requirements below address the parameter values after the defaults have been substituted
// for nBitsOfQ and fipsStandard.
//
// Requirements:
//  - pDlgroup!=NULL. Otherwise it returns SYMCRYPT_INVALID_ARGUMENT.
//
//  - If fipsStandard == SYMCRYPT_DLGROUP_FIPS_186_2, hashAlgorithm MUST be equal to
//    NULL, and nBitsOfQ <= 160 or nBitsOfQ = 0 && nBitsOfP <= 1024.
//
//  - If fipsStandard == SYMCRYPT_DLGROUP_FIPS_186_3, then hashAlgorithm MUST NOT be equal
//    to NULL.
//
//  - If nBitsOfHash is the number of bits of the output block of hashAlgorithm,
//    it is required that:
//              nBitsOfQ <= nBitsOfHash <= nBitsOfP
//    (where nBitsOfQ>0 was either provided by the caller of Allocate/Create
//    or it was picked from the above table).
//
//  - For FIPS 186-2, we have that nBitsOfHash == 160 (SHA1 output size). Therefore
//    this flag can only work with nBitsOfQ up to 160 bits. Anything else will
//    return SYMCRYPT_INVALID_ARGUMENT.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupSetValueSafePrime(
            SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE   dhSafePrimeType,
    _Inout_ PSYMCRYPT_DLGROUP                   pDlgroup );
//
// Sets a Discrete Logarithm Group for use in Diffie-Hellman using a named safe-prime group.
//
// - dhSafePrimeType: The type of named safe-prime group to use
//
// pDlGroup must have been created with SymCryptDlgroupAllocate() or SymCryptDlgroupCreate().
//
// Selects the largest named safe-prime group that will fit in the allocated Dlgroup (based on the
// values of nBitsOfP and nBitsOfQ used in allocation). It is recommended that callers set nBitsOfQ
// to 0 in allocation (equivalent to nBitsOfQ = (nBitsOfP-1)) when creating a safe-prime group.
//
// Requirements:
//  - pDlgroup was allocated with sufficient bits for the selected P (and Q) to fit. If there is no
//    named safe-prime group with bit size <= the allocated size, it returns SYMCRYPT_INVALID_ARGUMENT.
//    The minimum currently supported bitsize of named safe-prime groups is nBitsOfP = 2048.
//
//  - dhSafePrimeType!=SYMCRYPT_DLGROUP_DH_SAFEPRIMETYPE_NONE. Otherwise it returns SYMCRYPT_INVALID_ARGUMENT.
//

BOOLEAN
SYMCRYPT_CALL
SymCryptDlgroupIsSame(
    _In_    PCSYMCRYPT_DLGROUP  pDlgroup1,
    _In_    PCSYMCRYPT_DLGROUP  pDlgroup2 );
//
// Returns true if pDlgroup1 and pDlgroup2 have same set of P and G, false otherwise.
//

VOID
SYMCRYPT_CALL
SymCryptDlgroupGetSizes(
    _In_    PCSYMCRYPT_DLGROUP  pDlgroup,
    _Out_   SIZE_T*             pcbPrimeP,
    _Out_   SIZE_T*             pcbPrimeQ,
    _Out_   SIZE_T*             pcbGenG,
    _Out_   SIZE_T*             pcbSeed );
//
// It returns the tight byte-sizes of each parameter of the group: prime P,
// prime Q, generator G, and the FIPS domain_parameter_seed.
//
// If one of the pointers is NULL then the corresponding size is ignored.
//
// Remarks:
//  - If the group has no prime Q, then the returned sizes in *pcbPrimeQ and
//    *pcbSeed will be 0.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupSetValue(
    _In_reads_bytes_( cbPrimeP )    PCBYTE                  pbPrimeP,
                                    SIZE_T                  cbPrimeP,
    _In_reads_bytes_( cbPrimeQ )    PCBYTE                  pbPrimeQ,
                                    SIZE_T                  cbPrimeQ,
    _In_reads_bytes_( cbGenG )      PCBYTE                  pbGenG,
                                    SIZE_T                  cbGenG,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
    _In_opt_                        PCSYMCRYPT_HASH         pHashAlgorithm,
    _In_reads_bytes_( cbSeed )      PCBYTE                  pbSeed,
                                    SIZE_T                  cbSeed,
                                    UINT32                  genCounter,
                                    SYMCRYPT_DLGROUP_FIPS   fipsStandard,
    _Inout_                         PSYMCRYPT_DLGROUP       pDlgroup );
//
// Import key material to a DLGROUP object.
//  - Prime P is NOT optional and should always be imported.
//  - Prime Q is an optional parameter that may or may not be imported. If not
//    the group will not have a prime Q.
//  - Generator G is an optional parameter. However, if not present, the
//    algorithm will generate a random G of order Q. If both Q and G are missing
//    the calls fails with SYMCRYPT_INVALID_ARGUMENT.
//  - The parameters pHashAlgorithm, pbSeed, cbSeed and genCounter are the generation
//    parameters of the FIPS standards. If fipsStandard is not equal to
//    SYMCRYPT_DLGROUP_FIPS_NONE, the algorithm verifies that the input P,Q,G parameters are properly
//    generated by the corresponding standard.
//    If there is any discrepancy the function returns SYMCRYPT_AUTHENTICATION_FAILURE.
//    Notice that these parameters are imported even if they aren't verified.
//
// Requirements:
//  - The number stored in pbPrimeP and pbGenG must have at most nBitsOfP significant bits.
//    Otherwise the function returns SYMCRYPT_INVALID_ARGUMENT.
//  - The number stored in pbPrimeQ must have at most nBitsOfQ where nBitsOfQ is either
//    the **non-zero** value input in the call of Allocate/Create or equal to nBitsOfP if
//    0 was input.
//    Otherwise the function returns SYMCRYPT_INVALID_ARGUMENT.
//  - The size of the seed cbSeed must be **exactly** equal to the byte-size of the imported
//    modulus Q. Otherwise the function returns SYMCRYPT_INVALID_ARGUMENT.
//
// Remarks:
//  - The buffers pbPrimeP, pbPrimeQ, pbGenG must all have the same number
//    format defined by numFormat.
//  - Primes P and (when provided) Q must represent prime numbers.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlgroupGetValue(
    _In_                            PCSYMCRYPT_DLGROUP      pDlgroup,
    _Out_writes_bytes_( cbPrimeP )  PBYTE                   pbPrimeP,
                                    SIZE_T                  cbPrimeP,
    _Out_writes_bytes_( cbPrimeQ )  PBYTE                   pbPrimeQ,
                                    SIZE_T                  cbPrimeQ,
    _Out_writes_bytes_( cbGenG )    PBYTE                   pbGenG,
                                    SIZE_T                  cbGenG,
                                    SYMCRYPT_NUMBER_FORMAT  numFormat,
    _Out_                           PCSYMCRYPT_HASH *       ppHashAlgorithm,
    _Out_writes_bytes_( cbSeed )    PBYTE                   pbSeed,
                                    SIZE_T                  cbSeed,
    _Out_                           PUINT32                 pGenCounter );

//
// Retrieve the group parameters from a DLGROUP. The buffers should be
// allocated by the caller. If a pbXXX parameter is NULL (and the cbXXX==0)
// then this parameter is not returned.
//
// Requirements:
//  - All the buffers must have size at least equal to the corresponding
//    size returned by SymCryptDlgroupGetSizes. For the pbSeed buffer the
//    size must be **exactly** equal to the size returned from SymCryptDlgroupGetSizes.
//
// Remarks:
//  - If the caller requests a Q but the group does not have one, this function
//    will fail with SYMCRYPT_INVALID_BLOB.
//  - The return value of *ppHashAlgorithm can be NULL if the group was generated
//    by FIPS 186-2.
//

//=====================================================
// DL flags
//
// Also see Generic key validation flags above

// SYMCRYPT_FLAG_DLKEY_GEN_MODP:
// When set on SymCryptDlkeyGenerate call, generate a private key between 1 and P-2.
// When Q is known, this overrides the default behavior of generating a private key between 1 and Q-1,
// or 1 and min(2^nBitsPriv-1, Q-1) for named safe-prime groups
// When Q is not known, this does not affect the behavior
#define SYMCRYPT_FLAG_DLKEY_GEN_MODP  (0x01)

//=====================================================
// DL key operations

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeySetPrivateKeyLength( _Inout_ PSYMCRYPT_DLKEY pkDlkey, UINT32 nBitsPriv, UINT32 flags );
//
// Sets the number of bits that this dlkey can have in its private key
// The set value is only used for when the dlkey is a named safe-prime dlgroup, otherwise the value
// is ignored.
//
// Requirements:
//  - pkDlkey->pDlgroup->nBitsOfQ >= nBitsPriv >= pkDlkey->pDlgroup->nMinBitsPriv
//    Otherwise SYMCRYPT_INVALID_ARGUMENT is returned
//
// Allowed flags:
//  - None.

PCSYMCRYPT_DLGROUP
SYMCRYPT_CALL
SymCryptDlkeyGetGroup( _In_ PCSYMCRYPT_DLKEY pkDlkey );
//
// Returns a pointer to the dlgroup object associated with the key.
//

UINT32
SYMCRYPT_CALL
SymCryptDlkeySizeofPublicKey( _In_ PCSYMCRYPT_DLKEY pkDlkey );
//
// Returns the size in bytes of a blob big enough to retrieve the public key.
//

UINT32
SYMCRYPT_CALL
SymCryptDlkeySizeofPrivateKey( _In_ PCSYMCRYPT_DLKEY pkDlkey );
//
// Returns the size in bytes of a blob big enough to retrieve the private key.
//

BOOLEAN
SYMCRYPT_CALL
SymCryptDlkeyHasPrivateKey( _In_ PCSYMCRYPT_DLKEY pkDlkey );
//
// Returns TRUE if the pkDlkey object has a private key set.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyGenerate(
    _In_    UINT32                      flags,
    _Inout_ PSYMCRYPT_DLKEY             pkDlkey );
//
// Allowed flags:
//  - SYMCRYPT_FLAG_DLKEY_GEN_MODP
//  When set, generate a private key between 1 and P-2.
//  When Q is known, this overrides the default behavior of generating a private key between 1 and Q-1,
//  or 1 and min(2^nBitsPriv-1, Q-1) for named safe-prime groups
//  When Q is not known, this does not affect the behavior
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - At least one of the flags indicating what the Dlkey is to be used for must be specified:
//      SYMCRYPT_FLAG_DLKEY_DSA
//      SYMCRYPT_FLAG_DLKEY_DH
//
// Note:
// If SYMCRYPT_FLAG_DLKEY_GEN_MODP is specified then SYMCRYPT_FLAG_KEY_NO_FIPS must also be
// specified to avoid SYMCRYPT_INVALID_ARGUMENT, as FIPS requires the default generation behavior
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeySetValue(
    _In_reads_bytes_( cbPrivateKey )    PCBYTE                  pbPrivateKey,
                                        SIZE_T                  cbPrivateKey,
    _In_reads_bytes_( cbPublicKey )     PCBYTE                  pbPublicKey,
                                        SIZE_T                  cbPublicKey,
                                        SYMCRYPT_NUMBER_FORMAT  numFormat,
                                        UINT32                  flags,
    _Inout_                             PSYMCRYPT_DLKEY         pkDlkey );
//
// Import key material to a DLKEY object.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// - At least one of the flags indicating what the Dlkey is to be used for must be specified:
//      SYMCRYPT_FLAG_DLKEY_DSA
//      SYMCRYPT_FLAG_DLKEY_DH
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyGetValue(
    _In_    PCSYMCRYPT_DLKEY        pkDlkey,
    _Out_writes_bytes_( cbPrivateKey )
            PBYTE                   pbPrivateKey,
            SIZE_T                  cbPrivateKey,
    _Out_writes_bytes_( cbPublicKey )
            PBYTE                   pbPublicKey,
            SIZE_T                  cbPublicKey,
            SYMCRYPT_NUMBER_FORMAT  numFormat,
            UINT32                  flags );
//
// Retrieve the public or the private key (or both) from a DLKEY. The buffers should be
// allocated by the caller.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDlkeyExtendKeyUsage(
    _Inout_ PSYMCRYPT_DLKEY pkDlkey,
            UINT32          flags );
//
// Enable an existing key which has been generated or imported to be used in specified algorithms.
// Some callers may not know at key generation or import time what algorithms a key will be used for
// and this API allows the key to be extended for use in additional algorithms. Use of this API may
// not be compliant with FIPS 140-3.
//
// - flags must be some bitwise OR of the following flags:
//      SYMCRYPT_FLAG_DLKEY_DSA
//      SYMCRYPT_FLAG_DLKEY_DH

//=====================================================
// Elliptic curve operations and supported curves
//

UINT32
SYMCRYPT_CALL
SymCryptEcurvePrivateKeyDefaultFormat( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the private key default format of the input curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionNumOfBits( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the number of bits specified by the high bit restriction
// value of the input curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionPosition( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the position of the high bit restriction
// value of the input curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveHighBitRestrictionValue( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the high bit restriction value of the input curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveBitsizeofFieldModulus( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the number of bits of a field element on which
// the curve is defined.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveBitsizeofGroupOrder( _In_ PCSYMCRYPT_ECURVE pCurve );
//
// This function returns the number of bits of the order of the subgroup generated by
// the distinguished point of the curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveSizeofFieldElement( _In_    PCSYMCRYPT_ECURVE   pCurve );
//
// This function returns the number of bytes of a field element. It is used to
// construct buffers for setting and getting the value of elliptic curve points (most
// notably the public key of an ECKEY object).
//
// The result is equal to the cbFieldLength field of the parameters that created the curve.
//

UINT32
SYMCRYPT_CALL
SymCryptEcurveSizeofScalarMultiplier( _In_    PCSYMCRYPT_ECURVE   pCurve );
//
// This function returns the number of bytes of a scalar integer that is big enough to
// store a private key (or a multiplier of an elliptic curve point). It is used to
// construct buffers for setting and getting the value of a scalar multiplier (most
// notably the private key of an ECKEY object - see SymCryptEckeySetValue and
// SymCryptEckeyGetValue).
//
// The result is equal to sizeof( subgroupOrder * co-factor ).
//

BOOLEAN
SYMCRYPT_CALL
SymCryptEcurveIsSame(
    _In_    PCSYMCRYPT_ECURVE  pCurve1,
    _In_    PCSYMCRYPT_ECURVE  pCurve2);
//
// Returns true if pCurve1 and pCurve2 have same type, P, A, and B - false otherwise.
//
// Note: This does not check that the curves have the same G set, callers may additionally
// consider calling SymCryptEcpointIsEqual to compare the curves' distinguished points.
//

// Internally supported curves
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNistP192;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNistP224;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNistP256;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNistP384;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNistP521;

extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNumsP256t1;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNumsP384t1;
extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsNumsP512t1;

extern const PCSYMCRYPT_ECURVE_PARAMS    SymCryptEcurveParamsCurve25519;

typedef enum _SYMCRYPT_ECURVE_ID
{
    SYMCRYPT_ECURVE_ID_NULL             = 0,
    SYMCRYPT_ECURVE_ID_NIST_P192        = 1,
    SYMCRYPT_ECURVE_ID_NIST_P224        = 2,
    SYMCRYPT_ECURVE_ID_NIST_P256        = 3,
    SYMCRYPT_ECURVE_ID_NIST_P384        = 4,
    SYMCRYPT_ECURVE_ID_NIST_P521        = 5,
    SYMCRYPT_ECURVE_ID_NUMS_P256T1      = 6,
    SYMCRYPT_ECURVE_ID_NUMS_P384T1      = 7,
    SYMCRYPT_ECURVE_ID_NUMS_P512T1      = 8,
    SYMCRYPT_ECURVE_ID_CURVE25519       = 9
} SYMCRYPT_ECURVE_ID;

PCSYMCRYPT_ECURVE_PARAMS
SYMCRYPT_CALL
SymCryptGetEcurveParams( SYMCRYPT_ECURVE_ID ecurveId );
//
// Returns a pointer to the elliptic curve parameters structure for the specified curve ID.
// Returns NULL if the curve ID is invalid.
//

//=====================================================
// ECC flags
//
// Also see Generic key validation flags above

// SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION: This flag applies only to the ECDSA algorithm. When set, the sign
//      and verify algorithms will not do hash truncation. The caller can use their own truncation method in such case.
//      (default: according to the ECDSA standard)
#define SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION        (0x08)

//=====================================================
// EC key operations

UINT32
SYMCRYPT_CALL
SymCryptEckeySizeofPublicKey(
    _In_ PCSYMCRYPT_ECKEY           pkEckey,
    _In_ SYMCRYPT_ECPOINT_FORMAT    ecPointFormat );
//
// Returns the size in bytes of a blob big enough to retrieve the public key in
// the specified ECPOINT format.
//

UINT32
SYMCRYPT_CALL
SymCryptEckeySizeofPrivateKey( _In_ PCSYMCRYPT_ECKEY pkEckey );
//
// Returns the size in bytes of a blob big enough to retrieve the private key.
// It is equal to SymCryptEcurveSizeofScalarMultiplier( pCurve ) where pCurve is the
// curve that created the key.
//

BOOLEAN
SYMCRYPT_CALL
SymCryptEckeyHasPrivateKey( _In_ PCSYMCRYPT_ECKEY pkEckey );
//
// Returns TRUE if the pkEckey object has a private key set.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetValue(
    _In_reads_bytes_( cbPrivateKey )    PCBYTE                  pbPrivateKey,
                                        SIZE_T                  cbPrivateKey,
    _In_reads_bytes_( cbPublicKey )     PCBYTE                  pbPublicKey,
                                        SIZE_T                  cbPublicKey,
                                        SYMCRYPT_NUMBER_FORMAT  numFormat,
                                        SYMCRYPT_ECPOINT_FORMAT ecPointFormat,
                                        UINT32                  flags,
    _Inout_                             PSYMCRYPT_ECKEY         pEckey );
//
// Import key material to an ECKEY object.
//
// Requirements:
//      (pbPrivateKey, cbPrivateKey): a buffer that contains the private key, encoded
//      in the format specified by the numFormat parameter.
//      Note that the integer encoded in (pbPrivateKey, cbPrivateKey) is taken modulo the order of the
//      subgroup generated by the curve generator. Callers that want a uniform private key value
//      should ensure that the input is uniform in the range [1..GOrd-1].
//
//      Requirements: cbPrivateKey == SymCryptEckeySizeofPrivateKey( pEckey )
//
//      If pbPrivateKey == NULL && cbPrivateKey == 0, then no private key is imported, and the
//      resulting ECKEY object will not have a private key.
//
//      (pbPublicKey, cbPublicKey): buffer that contains the public key, encoded in the format
//      specified by the format parameter, the buffer length, and the curve properties.
//
//      Requirements: cbPublicKey == SymCryptEckeySizeofPublicKey( pEckey, ecPointFormat )
//
//      If no public key is presented (pbPublicKey == NULL && cbPublicKey == 0) then the public
//      key is computed from the provided private key.
//
//      At least one of the public and private keys must be provided.
//
//      If both are provided, then they must match.
//
//      The algorithm always sets the corresponding public key
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// - At least one of the flags indicating what the Eckey is to be used for must be specified:
//      SYMCRYPT_FLAG_ECKEY_ECDSA
//      SYMCRYPT_FLAG_ECKEY_ECDH
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetRandom(
    _In_    UINT32                  flags,
    _Inout_ PSYMCRYPT_ECKEY         pEckey );
//
// Generates a new Eckey public/private key pair using the specified curve. The public key
// is a uniformly random non-zero point of the subgroup generated by the distinguished point
// of the curve. This complies with the FIPS 186-4 standard.
//
// Remarks:
//  - In the case that the highbit restrictions on the curve are unsatisfiable, i.e.
//    there is no private key smaller than the group order it returns
//    SYMCRYPT_INVALID_ARGUMENT.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - At least one of the flags indicating what the Eckey is to be used for must be specified:
//      SYMCRYPT_FLAG_ECKEY_ECDSA
//      SYMCRYPT_FLAG_ECKEY_ECDH

// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyGetValue(
    _In_    PCSYMCRYPT_ECKEY        pEckey,
    _Out_writes_bytes_( cbPrivateKey )
            PBYTE                   pbPrivateKey,
            SIZE_T                  cbPrivateKey,
    _Out_writes_bytes_( cbPublicKey )
            PBYTE                   pbPublicKey,
            SIZE_T                  cbPublicKey,
            SYMCRYPT_NUMBER_FORMAT  numFormat,
            SYMCRYPT_ECPOINT_FORMAT ecPointFormat,
            UINT32                  flags );
//
// Retrieve the public or the private key (or both) from an ECKEY. The buffers should be
// allocated by the caller.
//
// If (pbPrivateKey != NULL), then the function will return the private key in pbPrivateKey
// in the format specified by the numFormat parameter **as long as** the following three
// requirements are satisfied:
//      1. cbPrivateKey >= SymCryptEckeySizeofPrivateKey( pEckey )
//      2. pEckey contains a private key part (If this fails the function returns SYMCRYPT_INVALID_BLOB)
// If (pbPrivateKey == NULL) and (cbPrivateKey == 0), then these parameters are ignored
// and no private key is returned.
//
// If (pbPublicKey != NULL), then the function will return the public key in pbPublicKey
// in the format specified by the numFormat and the ecPointFormat parameters
// **as long as** the following requirement is satisfied:
//      1. cbPublicKey >= SymCryptEckeySizeofPublicKey( pEckey, ecPointFormat )
// If (pbPublicKey == NULL) and (cbPublicKey == 0), then these parameters are ignored
// and no public key is returned.
//
// Allowed flags:
//      - None.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyExtendKeyUsage(
    _Inout_ PSYMCRYPT_ECKEY pEckey,
            UINT32          flags );
//
// Enable an existing key which has been generated or imported to be used in specified algorithms.
// Some callers may not know at key generation or import time what algorithms a key will be used for
// and this API allows the key to be extended for use in additional algorithms. Use of this API may
// not be compliant with FIPS 140-3
//
// - flags must be some bitwise OR of the following flags:
//      SYMCRYPT_FLAG_ECKEY_ECDSA
//      SYMCRYPT_FLAG_ECKEY_ECDH

/************************
 * Crypto algorithm API *
 ************************/

//
// The Crypto algorithm API implements various cryptographic algorithms that use large-integer arithmetic.
//

//
// RSA Encryption Algorithms
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaRawEncrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst );
//
// This function encrypts the buffer pbSrc (of size cbSrc bytes) under the pkRsakey key using textbook RSA.
// The output is stored in the pbDst buffer (of size cbDst bytes).
// For in place encryption pbSrc = pbDst.
//
// Both input and output buffers store a number in the number format numFormat.
//
// Requirements:
//  - If cbDst is too small for the result then SYMCRYPT_BUFFER_TOO_SMALL is returned.
//    Safe size is cbDst = SymCryptRsakeySizeofModulus(pkRsakey).
//  - The number stored in the pbSrc buffer must be strictly smaller than the value
//    of the public modulus in pkRsakey.
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaRawDecrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      numFormat,
                                UINT32                      flags,
    _Out_writes_( cbDst )       PBYTE                       pbDst,
                                SIZE_T                      cbDst );
//
// This function decrypts the buffer pbSrc (of size cbSrc bytes) with the pkRsakey key using textbook RSA.
// The output is stored in the pbDst buffer (of size cbDst bytes).
// For in place decryption pbSrc = pbDst.
//
// Both input and output buffers store a number in the number format numFormat.
//
// Requirements:
//  - If cbDst is too small for the result then SYMCRYPT_BUFFER_TOO_SMALL is returned.
//    Safe size is cbDst = SymCryptRsakeySizeofModulus(pkRsakey).
//  - The number stored in the pbSrc buffer must be strictly smaller than the value
//    of the public modulus in pkRsakey.
//  - The RSAKEY pkRsakey must have a private key part. Otherwise SYMCRYPT_INVALID_ARGUMENT is returned.
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Encrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
                                SYMCRYPT_NUMBER_FORMAT      nfDst,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst );
//
// This function encrypts the buffer pbSrc under the pkRsakey key using RSA PKCS1 v1.5.
// The output is stored in the pbDst buffer and the number of bytes written in *pcbDst.
//
// If pbDst == NULL then only the *pcbDst is output.
//
// nfDst is the number format of the ciphertext (i.e. the pbDst buffer).
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Decrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      nfSrc,
                                UINT32                      flags,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst );
//
// Perform an RSA-PKCS1 decryption.
//  - pbSrc/cbSrc: source buffer
//  - nfSrc: format of source buffer
//  - flags: must be 0
//  - pbDst/cbDst: destination buffer
//  - pcbDst: receives the size of the decrypted data.
//
// If the data in improperly formatted, an error is returned.
// If pbDst == NULL, then *pcbDst is set to the decrypted data length, and the functions succeeds.
//      This is not recommended as retrieving the actual data requires a second RSA decryption,
//      which is expensive. We recommend that callers provide a large enough buffer the first time.
// If pbDst != NULL and cbDst is too small, then *pcbDst is set to the required size of pbDst
//      and SYMCRYPT_BUFFER_TOO_SMALL is returned.
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepEncrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
                                SYMCRYPT_NUMBER_FORMAT      nfDst,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst );
//
// This function encrypts the buffer pbSrc under the pkRsakey key using RSA OAEP.
// The output is stored in the pbDst buffer and the number of bytes written in *pcbDst.
//
// If pbDst == NULL then only the *pcbDst is output.
//
// nfDst is the number format of the ciphertext (i.e. the pbDst buffer).
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaOaepDecrypt(
    _In_                        PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_NUMBER_FORMAT      nfSrc,
    _In_                        PCSYMCRYPT_HASH             hashAlgorithm,
    _In_reads_bytes_( cbLabel ) PCBYTE                      pbLabel,
                                SIZE_T                      cbLabel,
                                UINT32                      flags,
    _Out_writes_opt_( cbDst )   PBYTE                       pbDst,
                                SIZE_T                      cbDst,
    _Out_                       SIZE_T                      *pcbDst );
//
// This function decrypts the buffer pbSrc with the pkRsakey key using RSA OAEP.
// The output is stored in the pbDst buffer and the number of bytes written in *pcbDst.
//
// If pbDst == NULL then only the *pcbDst is output.
//
// nfSrc is the number format of the ciphertext (i.e. the pbSrc buffer).
//
// Requirement:
//  - cbSrc <= SymCryptRsakeySizeofModulus( pkRsakey ). Otherwise the function
//    returns SYMCRYPT_INVALID_ARGUMENT.
//
// Allowed flags:
//      None
//

//
// RSA Signing Algorithms
//

#define SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1                 (0x01)
#define SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID       (0x02)

#define SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT  (0x04)

//
// SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1: For RSA PKCS1 to not use the OID on signing or verifying.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Sign(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_                                PCSYMCRYPT_OID              pHashOIDs,
    _In_                                SIZE_T                      nOIDCount,
                                        UINT32                      flags,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _Out_writes_opt_( cbSignature )     PBYTE                       pbSignature,
                                        SIZE_T                      cbSignature,
    _Out_                               SIZE_T                      *pcbSignature );
//
// This function signs a message (its hash value is stored in pbHashValue) with
// the pkRsakey key using RSA PKCS1 v1.5. The signature is stored in the pbSignature
// buffer and the number of bytes written in *pcbSignature.
//
// pHashOIDs points to an array of SYMCRYPT_OID and the array size is nOIDCount
//
// If pbSignature == NULL then only the *pcbSignature is output.
//
// nfSignature is the number format of the signature (i.e. the pbSignature buffer). Currently
// only SYMCRYPT_NUMBER_FORMAT_MSB_FIRST is supported.
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PKCS1_NO_ASN1
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPkcs1Verify(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                      pbSignature,
                                        SIZE_T                      cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _In_reads_opt_( nOIDCount )         PCSYMCRYPT_OID              pHashOID,
    _In_                                SIZE_T                      nOIDCount,
                                        UINT32                      flags );
//
// This function verifies the signature of a message (its hash value is input in
// pbHashValue) with the pkRsakey key using RSA PKCS1 v1.5. The signature is input
// in the pbSignature buffer.
//
// pHashOIDs points to an array of SYMCRYPT_OID and the array size is nOIDCount
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE
// if it failed.
//
// nfSignature is the number format of the signature (i.e. the pbSignature buffer). Currently
// only SYMCRYPT_NUMBER_FORMAT_MSB_FIRST is supported.
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PKCS1_OPTIONAL_HASH_OID
//
//      When the flag is set, this function will do signature verification by not using hash OID when needed
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssSign(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_                                PCSYMCRYPT_HASH             hashAlgorithm,
                                        SIZE_T                      cbSalt,
                                        UINT32                      flags,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _Out_writes_opt_( cbSignature )     PBYTE                       pbSignature,
                                        SIZE_T                      cbSignature,
    _Out_                               SIZE_T                      *pcbSignature );
//
// Sign a message using RSA-PSS
// - pkRsaKey: Key to sign with; must contain a private key
// - pbHashValue/cbHashValue: Value to sign
// - hashAlgorithm: Hash algorithm to use in the MGF of PSS
// - cbSalt: # bytes of salt to use (typically equal to size of hash value)
// - flags: must be 0
// - nfSignature: Number format of signature. Typically SYMCRYPT_NUMBER_FORMAT_MSB_FIRST
// - pbSignature/cbSignature: buffer that receives the signature.
//              If pbSignature == NULL< only *pcbSignature is returned.
//              Note: pbSignature receives an integer, so if the buffer is larger than the modulus size
//              it will be padded with zeroes. For MSB-first format the zeroes are at the start of the buffer.
//              Typically this buffer is the same size as the RSA modulus.
// - pcbSignature: receives the size of the signature.
//
// Return value:
//  If cbHashValue + cbSalt is too large (above modulus size minus 2 or 3 depending on details) then
//  signature generation fails.
//
// Allowed flags:
//      None
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptRsaPssVerify(
    _In_                                PCSYMCRYPT_RSAKEY           pkRsakey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                      pbHashValue,
                                        SIZE_T                      cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                      pbSignature,
                                        SIZE_T                      cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT      nfSignature,
    _In_                                PCSYMCRYPT_HASH             hashAlgorithm,
                                        SIZE_T                      cbSalt,
                                        UINT32                      flags );
//
// This function verifies the signature of a message (its hash value is input in
// pbHashValue) with the pkRsakey key using RSA PSS. The signature is input
// in the pbSignature buffer.
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE
// if it failed.
//
// nfSignature is the number format of the signature (i.e. the pbSignature buffer). Currently
// only SYMCRYPT_NUMBER_FORMAT_MSB_FIRST is supported.
//
// Requirements:
//  - cbHashValue <= SymCryptRsakeySizeofModulus( pkRsakey )
//  - cbSalt <= SymCryptRsakeySizeofModulus( pkRsakey )
//  - cbSignature <= SymCryptRsakeySizeofModulus( pkRsakey )
//
// Allowed flags:
//      SYMCRYPT_FLAG_RSA_PSS_VERIFY_WITH_MINIMUM_SALT
//
//      When the flag is set, this function will do signature verification using the cbSalt parameter as
//      a minimum value for the salt length, rather than using it as an exact value. Specifying this and
//      setting cbSalt = 0 allows callers to verify a signature which has a valid encoding with any salt
//      length using a single call.
//

VOID
SYMCRYPT_CALL
SymCryptRsaSelftest(void);
//
// FIPS self-test for RSA sign/verify. This function uses a hardcoded key to perform the self-test
// without having to generate a key. If the self-test fails, SymCryptFatal will be called to
// fastfail.
// The self-test will automatically be performed before first operational use of RSA if using a key
// with FIPS validation, so most callers should never use this function.
//


//
// DSA
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaSign(
    _In_                                PCSYMCRYPT_DLKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature );
//
// Sign a message using the DSA signature algorithm.
// (pbHashValue,cbHashValue) is the output of the hash function that hashed the message to be signed.
// (pbSignature,cbSignature) is the output buffer that receives the signature.
// The signature is encoded as two integers (R,S) mod Q in the format specified by the 'format' parameter.
//
// Allowed flags:
//      None
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDsaVerify(
    _In_                                PCSYMCRYPT_DLKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                  pbSignature,
                                        SIZE_T                  cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags );
//
// Verifies a DSA signature using the public part of Key.
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE
// if it failed.
//
// Allowed flags:
//      None
//

VOID
SYMCRYPT_CALL
SymCryptDsaSelftest(void);
//
// FIPS self-test for DSA sign/verify. This function uses a hardcoded key to perform the self-test
// without having to generate a key. If the self-test fails, SymCryptFatal will be called to
// fastfail.
// The self-test will automatically be performed before first operational use of DSA if using a key
// with FIPS validation, so most callers should never use this function.
//

//
// DH
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptDhSecretAgreement(
    _In_                            PCSYMCRYPT_DLKEY        pkPrivate,
    _In_                            PCSYMCRYPT_DLKEY        pkPublic,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    UINT32                  flags,
    _Out_writes_( cbAgreedSecret )  PBYTE                   pbAgreedSecret,
                                    SIZE_T                  cbAgreedSecret );
//
// Calculates the agreed secret of a DH key exchange and stores it
// in the pbAgreedSecret buffer under the specified number format.
//
// format is the number format of the agreed secret (pbAgreedSecret buffer).
//
// Allowed flags:
//      - None
//

VOID
SYMCRYPT_CALL
SymCryptDhSecretAgreementSelftest(void);
//
// FIPS self-test for DH secret agreement. This function uses two hardcoded keys and a precalculated
// known answer to perform the self-test without having to generate a key. If the self-test fails,
// SymCryptFatal will be called to fastfail.
// The self-test will automatically be performed before first operational use of DH if using keys
// with FIPS validation, so most callers should never use this function.
//

//
// For both ECDSA and ECDH algorithms the key generation and management is the same. The main algorithms are:
//      - SymCryptEckeyAllocate or SymCryptEckeyCreate for creation of the ECKEY object.
//      - SymCryptEckeySetValue or SymCryptEckeySetRandom for filling the key with the preferred key material.
//      - SymCryptEckeyFree or SymCryptEckeyWipe for freeing or wiping the key.
//

//
// ECDSA
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaSign(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )   PBYTE                   pbSignature,
                                        SIZE_T                  cbSignature );
//
// Sign a message using the ECDSA signature algorithm.
// (pbHashValue,cbHashValue) is the output of the hash function that hashed the message to be signed.
// (pbSignature,cbSignature) is the output buffer that receives the signature.
// The signature is encoded as two integers in the format specified by the 'format' parameter.
//
// Allowed flags:
//      SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION: If set then the hash value will
//      not be truncated.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDsaVerify(
    _In_                                PCSYMCRYPT_ECKEY        pKey,
    _In_reads_bytes_( cbHashValue )     PCBYTE                  pbHashValue,
                                        SIZE_T                  cbHashValue,
    _In_reads_bytes_( cbSignature )     PCBYTE                  pbSignature,
                                        SIZE_T                  cbSignature,
                                        SYMCRYPT_NUMBER_FORMAT  format,
                                        UINT32                  flags );

//
// Verifies an ECDSA signature using the public part of Key.
//
// It returns SYMCRYPT_NO_ERROR if the verification succeeded or SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE
// if it failed.
//
// Allowed flags:
//      SYMCRYPT_FLAG_ECDSA_NO_TRUNCATION: If set then the hash value will
//      not be truncated.

VOID
SYMCRYPT_CALL
SymCryptEcDsaSelftest(void);
//
// FIPS self-test for ECDSA sign/verify. This function uses a hardcoded key to perform the self-test
// without having to generate a key. If the self-test fails, SymCryptFatal will be called to
// fastfail.
// The self-test will automatically be performed before first operational use of ECDSA if using a
// key with FIPS validation, so most callers should never use this function.
//

//
// ECDH
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEcDhSecretAgreement(
    _In_                            PCSYMCRYPT_ECKEY        pkPrivate,
    _In_                            PCSYMCRYPT_ECKEY        pkPublic,
                                    SYMCRYPT_NUMBER_FORMAT  format,
                                    UINT32                  flags,
    _Out_writes_( cbAgreedSecret )  PBYTE                   pbAgreedSecret,
                                    SIZE_T                  cbAgreedSecret );

//
// Calculates the agreed secret of a DH key exchange and stores it
// in the pbAgreedSecret buffer under the specified number format.
//
// Allowed flags:
//      - None
//

VOID
SYMCRYPT_CALL
SymCryptEcDhSecretAgreementSelftest(void);
//
// FIPS self-test for ECDH secret agreement. This function uses two hardcoded keys and a
// precalculated known answer to perform the self-test without having to generate a key. If the
// self-test fails, SymCryptFatal will be called to fastfail.
// The self-test will automatically be performed before first operational use of ECDH if using keys
// with FIPS validation, so most callers should never use this function.
//

//========================================================================
//
// Stateful Hash-based Signatures
//
// Hash-based signature schemes are digital signature schemes built out of hash
// functions. Stateful hash-based signatures are many-time signature schemes
// composed of a one-time-signature (OTS) scheme and a Merkle-tree representing
// multiple OTS with a public root value. At each signing operation, one of the
// (unused) OTS keys is used to sign the message, and the private key is updated
// so that the same OTS is not used again. Because there is a limited number of
// OTS keys determined at key generation time, signing cannot be performed after
// all OTSs are used. This is an important distinction from other digital signature
// schemes such as RSA or ECDSA.
//
// It is crucial for the security of the *stateful* hash-based signatures that the
// same private key state NOT be used more than once to sign messages, otherwise all
// security is lost.
//


//========================================================================
// XMSS API
//
//  XMSS is a stateful hash-based signature scheme specified in RFC 8391. The
//  multi-tree variant is named XMSS^MT.
//
//  XMSS uses WOTS+ as the one-time-signature (OTS) scheme. Public key consists
//  of two parts; Merkle-tree hash of OTS public keys called the Root, and a Seed value
//  used in in hash computations. The private key consists of SK_XMSS which is
//  used to deterministically create OTS keys, SK_PRF which is used to generate
//  the randomizer for hashing, and an integer Idx is used to select the next OTS key
//  for signing.
//

typedef enum _SYMCRYPT_XMSS_ALGID
{
    //                                              Hash Fn.    RFC-8391    SP800-208
    SYMCRYPT_XMSS_SHA2_10_256       = 0x00000001,   // SHA-256      X           X
    SYMCRYPT_XMSS_SHA2_16_256       = 0x00000002,   // SHA-256      X           X
    SYMCRYPT_XMSS_SHA2_20_256       = 0x00000003,   // SHA-256      X           X
    SYMCRYPT_XMSS_SHA2_10_512       = 0x00000004,   // SHA-512      X
    SYMCRYPT_XMSS_SHA2_16_512       = 0x00000005,   // SHA-512      X
    SYMCRYPT_XMSS_SHA2_20_512       = 0x00000006,   // SHA-512      X
    SYMCRYPT_XMSS_SHAKE_10_256      = 0x00000007,   // SHAKE128     X
    SYMCRYPT_XMSS_SHAKE_16_256      = 0x00000008,   // SHAKE128     X
    SYMCRYPT_XMSS_SHAKE_20_256      = 0x00000009,   // SHAKE128     X
    SYMCRYPT_XMSS_SHAKE_10_512      = 0x0000000A,   // SHAKE256     X
    SYMCRYPT_XMSS_SHAKE_16_512      = 0x0000000B,   // SHAKE256     X
    SYMCRYPT_XMSS_SHAKE_20_512      = 0x0000000C,   // SHAKE256     X
    SYMCRYPT_XMSS_SHA2_10_192       = 0x0000000D,   // SHA-256                  X
    SYMCRYPT_XMSS_SHA2_16_192       = 0x0000000E,   // SHA-256                  X
    SYMCRYPT_XMSS_SHA2_20_192       = 0x0000000F,   // SHA-256                  X
    SYMCRYPT_XMSS_SHAKE256_10_256   = 0x00000010,   // SHAKE256                 X
    SYMCRYPT_XMSS_SHAKE256_16_256   = 0x00000011,   // SHAKE256                 X
    SYMCRYPT_XMSS_SHAKE256_20_256   = 0x00000012,   // SHAKE256                 X
    SYMCRYPT_XMSS_SHAKE256_10_192   = 0x00000013,   // SHAKE256                 X
    SYMCRYPT_XMSS_SHAKE256_16_192   = 0x00000014,   // SHAKE256                 X
    SYMCRYPT_XMSS_SHAKE256_20_192   = 0x00000015,   // SHAKE256                 X

} SYMCRYPT_XMSS_ALGID;

typedef enum _SYMCRYPT_XMSSMT_ALGID
{
    //                                                  Hash Fn.    RFC-8391    SP800-208
    //                                                  SHA-256         X           X
    SYMCRYPT_XMSSMT_SHA2_20_2_256       = 0x00000001,
    SYMCRYPT_XMSSMT_SHA2_20_4_256       = 0x00000002,
    SYMCRYPT_XMSSMT_SHA2_40_2_256       = 0x00000003,
    SYMCRYPT_XMSSMT_SHA2_40_4_256       = 0x00000004,
    SYMCRYPT_XMSSMT_SHA2_40_8_256       = 0x00000005,
    SYMCRYPT_XMSSMT_SHA2_60_3_256       = 0x00000006,
    SYMCRYPT_XMSSMT_SHA2_60_6_256       = 0x00000007,
    SYMCRYPT_XMSSMT_SHA2_60_12_256      = 0x00000008,

    //                                                  SHA-512         X
    SYMCRYPT_XMSSMT_SHA2_20_2_512       = 0x00000009,
    SYMCRYPT_XMSSMT_SHA2_20_4_512       = 0x0000000A,
    SYMCRYPT_XMSSMT_SHA2_40_2_512       = 0x0000000B,
    SYMCRYPT_XMSSMT_SHA2_40_4_512       = 0x0000000C,
    SYMCRYPT_XMSSMT_SHA2_40_8_512       = 0x0000000D,
    SYMCRYPT_XMSSMT_SHA2_60_3_512       = 0x0000000E,
    SYMCRYPT_XMSSMT_SHA2_60_6_512       = 0x0000000F,
    SYMCRYPT_XMSSMT_SHA2_60_12_512      = 0x00000010,

    //                                                  SHAKE128        X
    SYMCRYPT_XMSSMT_SHAKE_20_2_256      = 0x00000011,
    SYMCRYPT_XMSSMT_SHAKE_20_4_256      = 0x00000012,
    SYMCRYPT_XMSSMT_SHAKE_40_2_256      = 0x00000013,
    SYMCRYPT_XMSSMT_SHAKE_40_4_256      = 0x00000014,
    SYMCRYPT_XMSSMT_SHAKE_40_8_256      = 0x00000015,
    SYMCRYPT_XMSSMT_SHAKE_60_3_256      = 0x00000016,
    SYMCRYPT_XMSSMT_SHAKE_60_6_256      = 0x00000017,
    SYMCRYPT_XMSSMT_SHAKE_60_12_256     = 0x00000018,

    //                                                  SHAKE256        X
    SYMCRYPT_XMSSMT_SHAKE_20_2_512      = 0x00000019,
    SYMCRYPT_XMSSMT_SHAKE_20_4_512      = 0x0000001A,
    SYMCRYPT_XMSSMT_SHAKE_40_2_512      = 0x0000001B,
    SYMCRYPT_XMSSMT_SHAKE_40_4_512      = 0x0000001C,
    SYMCRYPT_XMSSMT_SHAKE_40_8_512      = 0x0000001D,
    SYMCRYPT_XMSSMT_SHAKE_60_3_512      = 0x0000001E,
    SYMCRYPT_XMSSMT_SHAKE_60_6_512      = 0x0000001F,
    SYMCRYPT_XMSSMT_SHAKE_60_12_512     = 0x00000020,

    //                                                  SHA-256                     X
    SYMCRYPT_XMSSMT_SHA2_20_2_192       = 0x00000021,
    SYMCRYPT_XMSSMT_SHA2_20_4_192       = 0x00000022,
    SYMCRYPT_XMSSMT_SHA2_40_2_192       = 0x00000023,
    SYMCRYPT_XMSSMT_SHA2_40_4_192       = 0x00000024,
    SYMCRYPT_XMSSMT_SHA2_40_8_192       = 0x00000025,
    SYMCRYPT_XMSSMT_SHA2_60_3_192       = 0x00000026,
    SYMCRYPT_XMSSMT_SHA2_60_6_192       = 0x00000027,
    SYMCRYPT_XMSSMT_SHA2_60_12_192      = 0x00000028,

    //                                                  SHAKE256                    X
    SYMCRYPT_XMSSMT_SHAKE256_20_2_256   = 0x00000029,
    SYMCRYPT_XMSSMT_SHAKE256_20_4_256   = 0x0000002A,
    SYMCRYPT_XMSSMT_SHAKE256_40_2_256   = 0x0000002B,
    SYMCRYPT_XMSSMT_SHAKE256_40_4_256   = 0x0000002C,
    SYMCRYPT_XMSSMT_SHAKE256_40_8_256   = 0x0000002D,
    SYMCRYPT_XMSSMT_SHAKE256_60_3_256   = 0x0000002E,
    SYMCRYPT_XMSSMT_SHAKE256_60_6_256   = 0x0000002F,
    SYMCRYPT_XMSSMT_SHAKE256_60_12_256  = 0x00000030,

    //                                                  SHAKE256                    X
    SYMCRYPT_XMSSMT_SHAKE256_20_2_192   = 0x00000031,
    SYMCRYPT_XMSSMT_SHAKE256_20_4_192   = 0x00000032,
    SYMCRYPT_XMSSMT_SHAKE256_40_2_192   = 0x00000033,
    SYMCRYPT_XMSSMT_SHAKE256_40_4_192   = 0x00000034,
    SYMCRYPT_XMSSMT_SHAKE256_40_8_192   = 0x00000035,
    SYMCRYPT_XMSSMT_SHAKE256_60_3_192   = 0x00000036,
    SYMCRYPT_XMSSMT_SHAKE256_60_6_192   = 0x00000037,
    SYMCRYPT_XMSSMT_SHAKE256_60_12_192  = 0x00000038,

} SYMCRYPT_XMSSMT_ALGID;


typedef enum _SYMCRYPT_XMSSKEY_TYPE
{
    SYMCRYPT_XMSSKEY_TYPE_NONE      = 0,
    SYMCRYPT_XMSSKEY_TYPE_PUBLIC    = 1,    // Key object contains only public key
    SYMCRYPT_XMSSKEY_TYPE_PRIVATE   = 2,    // Key object contains both public key and private key
} SYMCRYPT_XMSSKEY_TYPE;


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssParamsFromAlgId(
            SYMCRYPT_XMSS_ALGID     id,
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams);
//
// Populate SYMCRYPT_XMSS_PARAMS structure for the specified XMSS algorithm identifier
// using the predefined parameter sets from RFC 8391 and NIST SP800-208
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssMtParamsFromAlgId(
            SYMCRYPT_XMSSMT_ALGID   id,
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams);
//
// Populate SYMCRYPT_XMSS_PARAMS structure for the specified XMSS^MT algorithm identifier
// using the predefined parameter sets from RFC 8391 and NIST SP800-208
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSetParams(
    _Out_   PSYMCRYPT_XMSS_PARAMS   pParams,
            UINT32                  id,                 // algorithm identifier
    _In_    PCSYMCRYPT_HASH         pHash,              // hash algorithm
            UINT32                  cbHashOutput,       // hash output size
            UINT32                  nWinternitzWidth,   // Winternitz parameter (width of digits)
            UINT32                  nTotalTreeHeight,   // total tree height
            UINT32                  nLayers,            // number of levels
            UINT32                  cbPrefix            // domain separator prefix length
    );
//
// Populates SYMCRYPT_XMSS_PARAMS structure by user defined parameters
//
//
// Parameters:
//
//		pParams. Pointer to the structure that will be populated with the
//		supplied parameters.
//
//		id. Algorithm identifier, will be embedded in key and signature objects.
//
//		pHash. Pointer to a hash object that implements a hash function which
//		will be used in XMSS/XMSS^MT operations.
//
//		cbHashOutput. Output size of the hash function in bytes. Leading cbHashOutput
//      bytes are taken as hash output if the hash algorithm's actual output size is larger.
//
//		nWinternitzWidth. Winternitz parameter, width of digits in byte sequences.
//      See remark below for more explanation.
//
//		nTotalTreeHeight. Height of the XMSS/XMSS^MT tree. In a multi-tree setting,
//      it is the sum of the tree heights of each layer.
//
//		nLayers. Number of layers. For XMSS nLayers=1, otherwise nLayers > 1. When nLayers > 1,
//      it must divide nTotalTreeHeight without remainder, so that each layer has height
//      nTotalTreeHeight/nLayers.
//
//		cbPrefix. Number of bytes in the prefix to the hash inputs used to domain separate
//		PRF functions.
//
// Requirements:
//
//      cbHashOutput must be nonzero, must be less than or equal to pHash->resultSize,
//      and must be less than or equal to SYMCRYPT_HASH_MAX_RESULT_SIZE
//
//      nWinternitzWidth must be one of 1, 2, 4, or 8
//
//      nTotalTreeHeight must be non-zero, it must be less than or equal to 32 for
//      single-tree (nLayers = 1), and must be less than 64 for multi-tree (nLayers > 1)
//
//      nLayers must be non-zero and must divide nTotalTreeHeight without remainder
//
//      cbPrefix must be non-zero
//
// Remarks:
//
//      RFC 8391 specifies w as the length of the Winternitz chains. Here,
//      it is used as the width of the digits in an octet string, i.e.,
//		base2 logarithm of the chain length, which is similar to its use
//		in LMS/HSS in RFC 8554.
//


#define SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT           (0x00000001)
// Verifies the public root value when importing a private key

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSizeofKeyBlobFromParams(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
            SYMCRYPT_XMSSKEY_TYPE   keyType,
    _Out_   SIZE_T*                 pcbKey );
//
// Return the size of an XMSS/XMSS^MT key blob associated with the provided XMSS parameters
//
//
// Parameters:
//
//		pParams. Pointer to an XMSS parameters structure that has been properly
//      initialized before this call.
//
//      keyType. SYMCRYPT_XMSSKEY_TYPE_PUBLIC (resp. SYMCRYPT_XMSSKEY_TYPE_PRIVATE) to
//      retrieve the size of the public key (resp. private key) blob.
//
//      pcbKey. Pointer to the variable to store the size of a public/private
//      key blob associated with the XMSS parameters.
//

PSYMCRYPT_XMSS_KEY
SYMCRYPT_CALL
SymCryptXmsskeyAllocate(
    _In_    PCSYMCRYPT_XMSS_PARAMS  pParams,
            UINT32                  flags );
//
// Allocate an XMSS/XMSS^MT key object and initialize it
//
// After this call, the key object does not contain a key yet. It must be
// followed by a call to SymCryptXmsskeyGenerate or SymCryptXmsskeySetValue.
//
// Allowed flags:
//
//      No flags defined for this function
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyGenerate(
    _Inout_ PSYMCRYPT_XMSS_KEY pKey,
            UINT32             flags );
//
// Generate a public/private XMSS/XMSS^MT key-pair
//
// Parameters:
//
//      pKey. Key object to store the public/private key-pair
//
//      flags. No flags defined for this function
//
// Return values:
//
//      - SYMCRYPT_NO_ERROR
//      On successful key generation
//
//      - SYMCRYPT_MEMORY_ALLOCATION_FAILURE
//      If there is not enough memory to perform key generation
//
// Remarks:
//
//      - Generates a random private key (SK_XMSS, SK_PRF) and a random
//      public seed SEED, and computes the public value Root from it.
//      - If the function fails, the key object will be in an invalid state.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeySetValue(
    _In_reads_bytes_( cbInput ) PCBYTE                  pbInput,
                                SIZE_T                  cbInput,
                                SYMCRYPT_XMSSKEY_TYPE   keyType,
                                UINT32                  flags,
    _Inout_                     PSYMCRYPT_XMSS_KEY      pKey );
//
//  Set an XMSS/XMSS^MT public/private key from key blob
//
//  Key formats:
//
//      PubKey: algId | Root | Seed
//      PrvKey: algId | Root | Seed | Idx | SK_XMSS | SK_PRF
//
//  algId and Idx are 32-bit and 64-bit integers respectively, stored in big-endian format.
//  Other values are n-bytes where n is the output size (in bytes) of the hash
//  algorithm (or the truncated size if the hash output is truncated).
//
//  Public-key format is specified in RFC 8391, whereas private-key format is not.
//  We define the private-key as an extension of the public-key with the private key
//  material.
//
//  Parameters:
//
//      (pbInput, cbInput). Input key blob to import the key from
//
//      keyType. Indicates whether (pbInput, cbInput) contains a public or a private key.
//      Must be one of SYMCRYPT_XMSSKEY_TYPE_PUBLIC, or SYMCRYPT_XMSSKEY_TYPE_PRIVATE.
//
//      flags. See below
//
//      pKey. Pointer to the XMSS key object to be initialized from the key blob
//
//  Allowed flags:
//
//      - SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT
//      Can only be specified when importing a private key. Recomputes the
//      public root value and compares it to the one that is imported from the
//      key blob.
//
//  Return values:
//
//      - SYMCRYPT_NO_ERROR
//       On successfully updating the key object from the provided key blob
//
//      - SYMCRYPT_INVALID_ARGUMENT
//       If cbInput does not match a public/private key size indicated by keyType parameter
//       If an invalid flag is specified, or SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT is
//      specified when setting a public key
//
//      - SYMCRYPT_INVALID_BLOB
//       If the XMSS algorithm ID in the key blob does not match the algorithm ID
//      used in creating the key object pointed to by pKey
//
//      - SYMCRYPT_MEMORY_ALLOCATION_FAILURE
//       If there is not sufficient memory for public root verification (only if
//      SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT is set in flags)
//
//      - SYMCRYPT_HBS_PUBLIC_ROOT_MISMATCH
//       If public root value in the key blob does not match the recomputed root value
//      (only if key blob is for a private key and SYMCRYPT_FLAG_XMSSKEY_VERIFY_ROOT is
//      specified)
//
//  Remarks:
//
//      - The key blob size pbInput must match the size returned by SymCryptXmssSizeofKeyBlobFromParams
//      for the same keyType and XMSS parameters the key object is created with.
//      - If the function fails, the key object will be in an invalid state.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmsskeyGetValue(
    _In_                            PCSYMCRYPT_XMSS_KEY     pKey,
                                    SYMCRYPT_XMSSKEY_TYPE   keyType,
                                    UINT32                  flags,
    _Out_writes_bytes_( cbOutput )  PBYTE                   pbOutput,
                                    SIZE_T                  cbOutput );
//
//  Get public/private key value from an XMSS/XMSS^MT key object
//
//  Key formats:
//
//      PubKey: algId | Root | Seed
//      PrvKey: algId | Root | Seed | Idx | SK_XMSS | SK_PRF
//
//  algId and Idx are 32-bit and 64-bit integers respectively, stored in big-endian format.
//  Other values are n-bytes where n is the output size (in bytes) of the hash
//  algorithm (or the truncated size if the hash output is truncated).
//
//  Public-key format is specified in RFC 8391, whereas private-key format is not.
//  We define the private-key as an extension of the public-key with the private key
//  material.
//
// Parameters:
//
//      pKey. The key object to export the key material from
//
//      keyType. Type of the key (public or private) to get. If the key object
//      contains a public key, keyType must be SYMCRYPT_XMSSKEY_TYPE_PUBLIC. If
//      the key object contains a private key, keyType can be one of
//      SYMCRYPT_XMSSKEY_TYPE_PUBLIC or SYMCRYPT_XMSSKEY_TYPE_PRIVATE
//
//      flags. No flags defined for this function
//
//      (pbOutput, cbOutput). Buffer to store the exported key blob. cbOutput must match
//      the size of the key to be exported, which can be queried by calling
//      SymCryptXmssSizeofKeyBlobFromParams.
//
// Return values:
//
//      - SYMCRYPT_NO_ERROR
//       On successful exporting of the key
//
//      - SYMCRYPT_INVALID_ARGUMENT
//       If cbOutput does not match the exact size of the key blob for the specified
//      keyType
//       If the key object does not contain private key material when keyType
//      equals SYMCRYPT_XMSSKEY_TYPE_PRIVATE
//       If unsupported flags are specified in flags parameter
//

VOID
SYMCRYPT_CALL
SymCryptXmsskeyFree(
    _Inout_ PSYMCRYPT_XMSS_KEY pKey);
//
// Free an allocated XMSS/XMSS^MT key object
//

SIZE_T
SYMCRYPT_CALL
SymCryptXmssSizeofSignatureFromParams(
    _In_ PCSYMCRYPT_XMSS_PARAMS pParams );
//
// Return the size of the signature for given XMSS parameters
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssSign(
    _Inout_                             PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )       PCBYTE              pbMessage,
                                        SIZE_T              cbMessage,
                                        UINT32              flags,
    _Out_writes_bytes_( cbSignature )   PBYTE               pbSignature,
                                        SIZE_T              cbSignature );
//
// Sign a message using XMSS/XMSS^MT
//
//  Parameters:
//
//      pKey. Private XMSS/XMSS^MT key used in signing
//
//      (pbMessage, cbMessage). Message to be signed
//
//      flags. No flags defined for this function
//
//      (pbSignature, cbSignature). Buffer to store the generated signature
//
//  Requirements:
//
//      pKey must contain the private key
//
//      cbSignature must be equal to the generated signature size
//
//  Return values:
//
//      - SYMCRYPT_NO_ERROR on successful signature generation
//
//      - SYMCRYPT_INVALID_ARGUMENT
//      If flags parameter is invalid,
//      or if the key object does not contain private key,
//      or cbSignature is not of correct size
//
//      - SYMCRYPT_HBS_NO_OTS_KEYS_LEFT
//      If the key doesn't have any one-time-signatures left for signing
//
//  Remarks:
//
//      The input pbMessage can be of arbitrary length and its randomized hash will be the actual
//      value that is going to be signed with a WOTSP signature. Applications wanting to pass the hash
//      value of a message to be signed as opposed to the message itself must make sure to have
//      domain separation between the space of messages and the hashes of the messages.
//
//      The signature size can be queried with SymCryptSizeofXmssSignatureFromParams function.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptXmssVerify(
    _Inout_                         PSYMCRYPT_XMSS_KEY  pKey,
    _In_reads_bytes_( cbMessage )   PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_( cbSignature ) PCBYTE              pbSignature,
                                    SIZE_T              cbSignature );
//
//  Verify an XMSS/XMSS^MT signature on a message
//
//  Parameters:
//
//      pKey. XMSS key used to verify the signature
//
//      (pbMessage, cbMessage) Message for which the signature was created
//
//      flags. No flags defined for this function
//
//      (pbSignature, cbSignature) XMSS or XMSS^MT signature
//
//  Return values:
//
//      - SYMCRYPT_NO_ERROR
//      If signature verification succeeds
//
//		- SYMCRYPT_INVALID_ARGUMENT
//		If flags is invalid or cbSignature is of incorrect size
//
//      - SYMCRYPT_SIGNATURE_VERIFICATION_ERROR
//      If the signature is not valid
//
//  Requirements:
//
//      cbSignature must be equal to the exact signature size associated with
//      the XMSS parameters.
//
//  Remarks:
//
//      In XMSS, the message can be arbitrarily long and a randomized hash of the message
//      will be computed first to be signed by the WOTSP internally.
//

VOID
SYMCRYPT_CALL
SymCryptXmssSelftest(void);
//
//  FIPS self-test for signature verification
//

//========================================================================
// Leighton-Micali Hash-Based Signatures (LMS) - LMS external struct definitions - implementing
// RFC8554/NIST Special Publication 800-208
//
typedef enum _SYMCRYPT_LMS_ALGID
{
    //
    // Algorithm IDs for Leighton-Micali Hash-Based Signatures (LMS)
    // M equals the output length of the hash function, where H is the tree height.
    // The M parameter primarily affects the security and size of the signatures, while the H parameter
    // impacts the number of possible signatures and the computational cost for signing and verification.
    // Larger M increases security and signature size, but increases the computational cost
    // Higher H means more signatures but higher computational cost for signing and verification
    //
    SYMCRYPT_LMS_SHA256_M32_H5      = 0x00000005,
    SYMCRYPT_LMS_SHA256_M32_H10     = 0x00000006,
    SYMCRYPT_LMS_SHA256_M32_H15     = 0x00000007,
    SYMCRYPT_LMS_SHA256_M32_H20     = 0x00000008,
    SYMCRYPT_LMS_SHA256_M32_H25     = 0x00000009,
    SYMCRYPT_LMS_SHA256_M24_H5      = 0x0000000A,
    SYMCRYPT_LMS_SHA256_M24_H10     = 0x0000000B,
    SYMCRYPT_LMS_SHA256_M24_H15     = 0x0000000C,
    SYMCRYPT_LMS_SHA256_M24_H20     = 0x0000000D,
    SYMCRYPT_LMS_SHA256_M24_H25     = 0x0000000E,
    SYMCRYPT_LMS_SHAKE_M32_H5       = 0x0000000F,
    SYMCRYPT_LMS_SHAKE_M32_H10      = 0x00000010,
    SYMCRYPT_LMS_SHAKE_M32_H15      = 0x00000011,
    SYMCRYPT_LMS_SHAKE_M32_H20      = 0x00000012,
    SYMCRYPT_LMS_SHAKE_M32_H25      = 0x00000013,
    SYMCRYPT_LMS_SHAKE_M24_H5       = 0x00000014,
    SYMCRYPT_LMS_SHAKE_M24_H10      = 0x00000015,
    SYMCRYPT_LMS_SHAKE_M24_H15      = 0x00000016,
    SYMCRYPT_LMS_SHAKE_M24_H20      = 0x00000017,
    SYMCRYPT_LMS_SHAKE_M24_H25      = 0x00000018,
} SYMCRYPT_LMS_ALGID;

typedef enum _SYMCRYPT_LMS_OTS_ALGID
{
    // Algorithm IDs for Leighton-Micali Hash-Based Signatures (LMS) One-Time-Signature (OTS)
    // N parameter represents the number of bytes in the hash function output. It determines the size of the hash values used in
    // the LMS OTS scheme.
    // W parameter represents the width of the Winternitz parameter used in LMS OTS. A larger value of w results in shorter
    // signatures but requires more computation during key generation, signature generation, and signature verification.
    //
    SYMCRYPT_LMS_OTS_SHA256_N32_W1      = 0x00000001,
    SYMCRYPT_LMS_OTS_SHA256_N32_W2      = 0x00000002,
    SYMCRYPT_LMS_OTS_SHA256_N32_W4      = 0x00000003,
    SYMCRYPT_LMS_OTS_SHA256_N32_W8      = 0x00000004,
    SYMCRYPT_LMS_OTS_SHA256_N24_W1      = 0x00000005,
    SYMCRYPT_LMS_OTS_SHA256_N24_W2      = 0x00000006,
    SYMCRYPT_LMS_OTS_SHA256_N24_W4      = 0x00000007,
    SYMCRYPT_LMS_OTS_SHA256_N24_W8      = 0x00000008,
    SYMCRYPT_LMS_OTS_SHAKE_N32_W1       = 0x00000009,
    SYMCRYPT_LMS_OTS_SHAKE_N32_W2       = 0x0000000A,
    SYMCRYPT_LMS_OTS_SHAKE_N32_W4       = 0x0000000B,
    SYMCRYPT_LMS_OTS_SHAKE_N32_W8       = 0x0000000C,
    SYMCRYPT_LMS_OTS_SHAKE_N24_W1       = 0x0000000D,
    SYMCRYPT_LMS_OTS_SHAKE_N24_W2       = 0x0000000E,
    SYMCRYPT_LMS_OTS_SHAKE_N24_W4       = 0x0000000F,
    SYMCRYPT_LMS_OTS_SHAKE_N24_W8       = 0x00000010,
} SYMCRYPT_LMS_OTS_ALGID;

// Verifies the public key root value when importing a private key
#define SYMCRYPT_FLAG_LMSKEY_VERIFY_ROOT           (0x00000001)

typedef enum _SYMCRYPT_LMSKEY_TYPE
{
    SYMCRYPT_LMSKEY_TYPE_NONE       = 0,   // Key object does not contain any key material
    SYMCRYPT_LMSKEY_TYPE_PUBLIC     = 1,   // Key object contains only public key
    SYMCRYPT_LMSKEY_TYPE_PRIVATE    = 2,   // Key object contains both public key and private key
} SYMCRYPT_LMSKEY_TYPE;
//      The format of the private key blob is as follows:
//                                        [         Public key parts                   ||      Private key parts       ]
//                                        [    4     ||      4      ||  16 ||    m     ||      4         ||      m     ]
//                                        [ LmsAlgId || LmsOtsAlgId ||  I  || RootNode || NextUnusedLeaf || Seed ]
//
//      The format of the public key blob is as follows:
//                                        [     4    ||      4      ||  16 ||    m     ]
//                                        [ LmsAlgId || LmsOtsAlgId ||  I  || RootNode ]

//=====================================================
// LMS operations

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsParamsFromAlgId(
            SYMCRYPT_LMS_ALGID      lmsAlgID,
            SYMCRYPT_LMS_OTS_ALGID  lmsOtsAlgID,
    _Out_   PSYMCRYPT_LMS_PARAMS    pParams);
//
// This function populates a SYMCRYPT_LMS_PARAMS structure with the predefined parameter sets for a given LMS
// algorithm identifier and LMS OTS algorithm identifier. The resulting structure can be used to create LMS key objects.
// The values defined by SYMCRYPT_LMS_OTS_ALGID and SYMCRYPT_LMS_ALGID are all of the NIST SP 800-208 parameter
// sets supported by SymCrypt.
//
// Parameters:
//      lmsAlgID: The LMS algorithm identifier to use
//
//      lmsOtsAlgID: The LMS OTS algorithm identifier to use
//
//      pParams: A pointer to a SYMCRYPT_LMS_PARAMS structure that will be populated with the predefined parameter sets
//
// Return value:
//      If the function succeeds, it returns SYMCRYPT_NO_ERROR
//      If the function fails, it returns SYMCRYPT_INVALID_ARGUMENT
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSetParams(
    _Out_   PSYMCRYPT_LMS_PARAMS    pParams,
            UINT32                  lmsAlgID,
            UINT32                  lmsOtsAlgID,
    _In_    PCSYMCRYPT_HASH         pLmsHashFunction,
            UINT32                  cbHashOutput,
            UINT32                  nTreeHeight,
            UINT32                  nWinternitzChainWidth);
//
// This function allows for the customization of non-standard parameter sets, which cannot be set using LmsParamsFromAlgId.
//
// Parameters:
//      pParams: A pointer to a SYMCRYPT_LMS_PARAMS structure to be initialized.
//
//      lmsAlgID: LMS algorithm identifier, will be embedded in key and signature objects.
//
//      lmsOtsAlgID: LMS OTS algorithm identifier, will be embedded in key and signature objects.
//
//      pLmsHashFunction: A pointer to the hash function to be used for the LMS system.
//
//      cbHashOutput: The number of bytes for each tree node, equal to the output length of the hash function.
//      Must be less than or equal to 32.
//
//      nTreeHeight: The height of the LMS tree. Must be < 32, there are (2^nTreeHeight) leaves in the tree.
//
//      nWinternitzChainWidth: An integer that specifies the base2 logarithm of Winternitz chain lengths.
//      Must be one of 1, 2, 4, or 8
//
// Return value:
//      If the function succeeds, it fills PSYMCRYPT_LMS_PARAMS structure by user defined values and return SYMCRYPT_NO_ERROR.
//      Otherwise, it sets the values of PSYMCRYPT_LMS_PARAMS to 0 and returns SYMCRYPT_INVALID_ARGUMENT.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSizeofKeyBlobFromParams(
    _In_    PCSYMCRYPT_LMS_PARAMS   pParams,
            SYMCRYPT_LMSKEY_TYPE    keyType,
    _Out_   SIZE_T*                 pcbKey);
//
// Returns the size of an LMS key blob based on the provided LMS parameters and keyType.
//
// Parameters:
//      pParams: A pointer to a SYMCRYPT_LMS_PARAMS structure that specifies the parameters of the LMS key.
//
//      keyType: Specifies the type of blob for which to retrieve the size.
//      Must be one of SYMCRYPT_LMSKEY_TYPE_PUBLIC or SYMCRYPT_LMSKEY_TYPE_PRIVATE.
//
//      pcbKey: Pointer to the variable to store the size of a public/private
//      key blob associated with the LMS parameters.
//
// Return value:
//      If the function succeeds, it returns SYMCRYPT_NO_ERROR. In case keyType is not recognized
//      it returns SYMCRYPT_INVALID_ARGUMENT.
//

PSYMCRYPT_LMS_KEY
SYMCRYPT_CALL
SymCryptLmskeyAllocate(
    _In_    PCSYMCRYPT_LMS_PARAMS   pParams,
            UINT32                  flags);
//
// This function allocates a new SYMCRYPT_LMS_KEY object, which represents a key for the Leighton-Micali Signature (LMS)
// scheme, based on the given PCSYMCRYPT_LMS_PARAMS. The function allocates memory for the key object, and returns a pointer to it.
// The caller is responsible for freeing the memory when the key is no longer needed, using the SymCryptLmskeyFree function.
//
// Parameters:
//      pParams: A pointer to a constant SYMCRYPT_LMS_PARAMS structure that describes
//          the LMS parameters to be used for the key.
//          The structure must be non-null, and must be initialized by one of the initialization functions:
//          SymCryptLmsParamsFromAlgId or SymCryptLmsSetParams.
//
//      flags: Currently not used. Must be set to 0.
//
// Return value:
//      If the function succeeds, it returns a pointer to the newly created SYMCRYPT_LMS_KEY object.
//      Otherwise, it returns NULL, indicating an error that should be handled by the caller.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeyGenerate(
    _Inout_ PSYMCRYPT_LMS_KEY   pKey,
            UINT32              flags);
//
// This function generates an LMS public/private key pair in the pKey object.
//
// Parameters:
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that represents the LMS key object to be initialized. The structure
//      must be valid and non-null, and must have been previously created using the SymCryptLmskeyAllocate
//      function. If the key object already contains key values, they will be overwritten by the generated values.
//
//      flags: Currently not used. Must be set to 0.
//
// Return value:
//      If the function succeeds, it returns SYMCRYPT_NO_ERROR. Otherwise, it returns an error code that describes the nature of the
//      failure, such as SYMCRYPT_INVALID_ARGUMENT.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeySetValue(
    _In_reads_bytes_(cbInput)   PCBYTE                  pbInput,
                                SIZE_T                  cbInput,
                                SYMCRYPT_LMSKEY_TYPE    keyType,
                                UINT32                  flags,
    _Inout_                     PSYMCRYPT_LMS_KEY       pKey);
//
// This function imports an LMS key from a buffer, setting the key object with the provided data.
//
// Parameters:
//      pbInput: A pointer to a byte buffer containing the key data to be imported into the LMS key object
//      The format of the input buffer is specified by the SYMCRYPT_LMSKEY_TYPE enumeration.
//
//      cbInput: The size, in bytes, of the key data buffer pointed to by pbInput
//
//      keyType: Indicates whether (pbInput, cbInput) contains a public or a private key.
//      Must be one of SYMCRYPT_LMSKEY_TYPE_PUBLIC, or SYMCRYPT_LMSKEY_TYPE_PRIVATE.
//
//      flags: See allowed flags below.
//
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that will receive the imported key from the buffer pbInput
//
// Allowed flags:
//      SYMCRYPT_FLAG_LMSKEY_VERIFY_ROOT: Can only be specified when importing a private key. Recomputes the
//      public root value and compares it to the one that is imported from the key blob.
//
// Return value:
//      If the function succeeds, it returns SYMCRYPT_NO_ERROR and the SYMCRYPT_LMS_KEY structure is set with the imported key data.
//      If the function fails, it returns an error code that describes the nature of the failure, such as SYMCRYPT_INVALID_ARGUMENT.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmskeyGetValue(
    _In_                                PCSYMCRYPT_LMS_KEY      pKey,
                                        SYMCRYPT_LMSKEY_TYPE    keyType,
                                        UINT32                  flags,
    _Out_writes_bytes_(cbOutput)        PBYTE                   pbOutput,
                                        SIZE_T                  cbOutput);
//
// This function retrieves the public or private key value from an LMS key object, depending on the keyType parameter
//
// Parameters:
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that represents the LMS key object to retrieve the key value from.
//      The structure must be valid and non-null, and must contain the key values to retrieve
//
//      keyType: Type of the key (public or private) to get. If the key object only
//      contains a public key, keyType must be SYMCRYPT_LMSKEY_TYPE_PUBLIC. If
//      the key object contains a private key, keyType can be one of
//      SYMCRYPT_LMSKEY_TYPE_PUBLIC or SYMCRYPT_LMSKEY_TYPE_PRIVATE
//
//      flags: Currently not used. Must be set to 0.
//
//      pbOutput: A buffer to hold the key value. The buffer must be large enough to hold the key value.
//      The format of the output buffer is specified by the SYMCRYPT_LMSKEY_TYPE enumeration.
//
//      cbOutput: The size of the pbOutput buffer in bytes
//
// Return value:
//      If the function succeeds, it returns SYMCRYPT_NO_ERROR. Otherwise, it returns SYMCRYPT_INVALID_ARGUMENT.
//

VOID
SYMCRYPT_CALL
SymCryptLmskeyFree(
    _Inout_ PSYMCRYPT_LMS_KEY   pKey);
//
// This function frees the memory that was allocated for the given LMS key object, which was previously created using the
// SymCryptLmskeyAllocate function. The function wipes and deallocates the memory.
//
// Parameters:
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that represents the LMS key object to be freed. The structure
//      must be valid and non-null, and must have been previously created using the SymCryptLmskeyAllocate function.
//
// Return value:
//      The function does not return a value.
//

SIZE_T
SYMCRYPT_CALL
SymCryptLmsSizeofSignatureFromParams(
    _In_ PCSYMCRYPT_LMS_PARAMS pParams);
//
// This function returns the size, in bytes, of the signature that will be generated by the LMS signature scheme, based on the
// specified LMS parameters.
//
// Parameters:
//      pParams: A pointer to a SYMCRYPT_LMS_PARAMS structure that represents the parameters associated with the LMS key to
//      use for computing the signature size. The structure must be valid and non-null.
//
// Return value:
//      Signature size in bytes.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsSign(
    _Inout_                             PSYMCRYPT_LMS_KEY   pKey,
    _In_reads_bytes_(cbMessage)         PCBYTE              pbMessage,
                                        SIZE_T              cbMessage,
                                        UINT32              flags,
    _Out_writes_bytes_(cbSignature)     PBYTE               pbSignature,
                                        SIZE_T              cbSignature);
//
// This function generates an LMS signature for the given message, using the private key in the given LMS key object.
// The function fills the buffer pointed to by pbSignature with the LMS signature. It uses the LMS parameters
// and key values that were specified when the key object was created to generate the signature.
// Stateful hash-based signatures are not approved by FIPS for key generation and signature generation in software
// modules. Special care must be taken to ensure that the same private key state is not used more than once to
// sign messages. This can be done, for instance, by releasing a signature only after verifying that the private
// key has been updated and serialized to a physical storage.
//
// Parameters:
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that represents the LMS key object to be used for signing the message.
//      The structure must be valid and non-null, and must contain the private key values for the LMS scheme. The private key
//      must have been initialized previously using the SymCryptLmsKeyGenerate or SymCryptLmskeySetValue function.
//
//      pbMessage: A pointer to a buffer that contains the message to be signed.
//
//      cbMessage: The length in bytes of the message to be signed.
//
//      flags: Currently not used. Must be set to 0.
//
//      pbSignature: A pointer to the buffer that receives the computed signature. It must be large enough to hold the
//      generated signature. The required size can be retrieved using: SymCryptLmsSizeofSignatureFromParams.
//
//      cbSignature: The size of the signature buffer pbSignature. If the passed size is different than the
//      required signature size an error will be returned.
//
// Return value:
//      SYMCRYPT_NO_ERROR - If the function succeeds
//
//      SYMCRYPT_HBS_NO_OTS_KEYS_LEFT - If the key has run out of available OTS keys
//
//      SYMCRYPT_INVALID_ARGUMENT - If one of the input parameters is invalid
//
// Remarks:
//      The LMS signing process inherits its signature from the LMS OTS, which means that it will always compute a digest of the
//      given message before signing, even if a hash value is provided as the message.
//      Developers should always be consistent with the input to the LMS sign and verify functions and ensure that the input message
//      is in the correct format before passing it to these functions
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptLmsVerify(
    _In_                            PCSYMCRYPT_LMS_KEY  pKey,
    _In_reads_bytes_(cbMessage)     PCBYTE              pbMessage,
                                    SIZE_T              cbMessage,
                                    UINT32              flags,
    _In_reads_bytes_(cbSignature)   PCBYTE              pbSignature,
                                    SIZE_T              cbSignature);
//
// This function verifies the given LMS signature (pbSignature) for the given message (pbMessage), using the public key
// in the given LMS key object. The function returns SYMCRYPT_NO_ERROR if the signature is valid, and an error code otherwise.
//
// Parameters:
//      pKey: A pointer to a SYMCRYPT_LMS_KEY structure that represents the LMS key object to be used for verifying
//      the signature. The structure must be valid and non-null, and must contain the public or private key values for the LMS scheme.
//      The public key must have been generated previously using the SymCryptLmsKeyGenerate or SymCryptLmskeySetValue functions, and must match the
//      private key that was used to generate the signature.
//
//      pbMessage: A pointer to a buffer that contains the message that was signed.The buffer must be valid and non-null, and
//      must contain at least cbMessage bytes of data.
//
//      cbMessage: The length in bytes of the message that was signed. The length should be set to the actual size of the message.
//      If the message is larger than the maximum size allowed by the LMS parameters, the function will return an error.
//
//      flags: Currently not used. Must be set to 0.
//
//      pbSignature: A pointer to a buffer that contains the signature that was generated for the message. The buffer must
//      be valid and non-null, and must contain at least cbSignature bytes.
//
//      cbSignature: The length in bytes of the signature buffer that contains the signature. The length must be
//      equal to the exact signature size associated with the given LMS parameters and key values.
//
// Return value:
//      SYMCRYPT_NO_ERROR - If the function succeeds
//
//      SYMCRYPT_INVALID_ARGUMENT - If the signature structure is not correct or if there is a mismatch between the
//          input parameters.
//
//      SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE - If the signature verification fails
//

VOID
SYMCRYPT_CALL
SymCryptLmsSelftest(void);


// MLKEMKEY objects' API
//

// MLKEM key formats
// ==================
// The below formats apply **only to external formats**: When somebody is importing or exporting
// a key. The internal format of the keys is not visible to the caller.
typedef enum _SYMCRYPT_MLKEMKEY_FORMAT {
    SYMCRYPT_MLKEMKEY_FORMAT_NULL               = 0,
    SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED       = 1,
        // 64-byte concatenation of d || z from FIPS 203. Smallest representation of a full
        // ML-KEM key.
        // On its own it is ambiguous which ML-KEM parameter set this represents; callers wanting to
        // store this format must track the parameter set alongside the key.
    SYMCRYPT_MLKEMKEY_FORMAT_DECAPSULATION_KEY  = 2,
        // Standard byte encoding of an ML-KEM Decapsulation key, per FIPS 203.
        // Size is 1632, 2400, or 3168 bytes for ML-KEM 512, 768, and 1024 respectively.
    SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY  = 3,
        // Standard byte encoding of an ML-KEM Encapsulation key, per FIPS 203.
        // Size is 800, 1184, or 1568 bytes for ML-KEM 512, 768, and 1024 respectively.
} SYMCRYPT_MLKEMKEY_FORMAT;


typedef enum _SYMCRYPT_MLKEM_PARAMS {
    SYMCRYPT_MLKEM_PARAMS_NULL          = 0,
    SYMCRYPT_MLKEM_PARAMS_MLKEM512      = 1,
    SYMCRYPT_MLKEM_PARAMS_MLKEM768      = 2,
    SYMCRYPT_MLKEM_PARAMS_MLKEM1024     = 3,
} SYMCRYPT_MLKEM_PARAMS;
//
// Currently supported ML-KEM parameter sets are represented externally only by the enum
//

PSYMCRYPT_MLKEMKEY
SYMCRYPT_CALL
SymCryptMlKemkeyAllocate(
    SYMCRYPT_MLKEM_PARAMS   params );
//
// Allocate and create a new MLKEMKEY object sized according to the specified parameters.
//
// This call does not initialize the key. It should be
// followed by a call to SymCryptMlKemkeyGenerate or
// SymCryptMlKemkeySetValue.
//

VOID
SYMCRYPT_CALL
SymCryptMlKemkeyFree(
    _Inout_ PSYMCRYPT_MLKEMKEY  pkMlKemkey );


// d and z are each 32 bytes
#define SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE    (2*32)

#define SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM512   (800)
#define SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM768   (1184)
#define SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM1024  (1568)

#define SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM512   (1632)
#define SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM768   (2400)
#define SYMCRYPT_MLKEM_DECAPSULATION_KEY_SIZE_MLKEM1024  (3168)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemSizeofKeyFormatFromParams(
            SYMCRYPT_MLKEM_PARAMS       params,
            SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyformat,
    _Out_   SIZE_T*                     pcbKeyFormat );
//
// Gives the size in bytes of the blob of the given format for the given ML-KEM
// parameters via pcbKeyFormat output.
// Returns SYMCRYPT_INCOMPATIBLE_FORMAT if mlKemkeyFormat is an unsupported value,
// or SYMCRYPT_INVALID_ARGUMENT if other parameters are invalid.
//

#define SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM512    (768)
#define SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM768    (1088)
#define SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_MLKEM1024   (1568)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemSizeofCiphertextFromParams(
            SYMCRYPT_MLKEM_PARAMS       params,
    _Out_   SIZE_T*                     pcbCiphertext );
//
// Gives the size in bytes of the ciphertext for the given ML-KEM parameters.
// Returns SYMCRYPT_INVALID_ARGUMENT if parameters are invalid.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeyGenerate(
    _Inout_                     PSYMCRYPT_MLKEMKEY  pkMlKemkey,
                                UINT32              flags );
//
// Generate a new random ML-KEM key using the information from the
// parameters passed to SymCryptMlKemkeyAllocate.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeySetValue(
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyFormat,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_MLKEMKEY          pkMlKemkey );
//
// Import key material to an ML-KEM key object. The arguments are the following:
//  - (pbSrc, cbSrc): a buffer containing a representation of an ML-KEM key,
//    in format specified by mlKemkeyFormat.
//  - mlKemkeyFormat format of the input
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// Remarks:
// - cbSrc must be equal to the cbKeyFormat returned from
//   SymCryptMlKemSizeofKeyFormatFromParams(params, mlKemkeyFormat, &cbKeyFormat), though typically this
//   value can be known statically (see definition of SYMCRYPT_MLKEMKEY_FORMAT)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemkeyGetValue(
    _In_                        PCSYMCRYPT_MLKEMKEY         pkMlKemkey,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst,
                                SYMCRYPT_MLKEMKEY_FORMAT    mlKemkeyFormat,
                                UINT32                      flags );
//
// Export key material from an ML-KEM key object. The arguments are the following:
//  - (pbDst, cbDst): a buffer into which a representation of an ML-KEM key is
//    written, in the format specified by mlKemkeyFormat.
//  - mlKemkeyFormat format of the output
//
// Allowed flags:
//      - None.
//
//  Remarks:
//  - If the key object does not have the information required to export to the format
//    specified by mlKemkeyFormat this function will return SYMCRYPT_INCOMPATIBLE_FORMAT.
//  - cbDst must be equal to the cbKeyFormat returned from
//    SymCryptMlKemSizeofKeyFormatFromParams(params, mlKemkeyFormat, &cbKeyFormat), though typically this
//    value can be known statically (see definition of SYMCRYPT_MLKEMKEY_FORMAT)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulate(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE               pbCiphertext,
                                            SIZE_T              cbCiphertext );
//
// Performs the Encapsulate operation of ML-KEM.
// This uses the public information of an ML-KEM keypair to generate an agreed secret
// and a ciphertext. Only a peer with the private information of an ML-KEM keypair can
// decapsulate the ciphertext to compute the agreed secret.
//
// The arguments are the following:
// - pkMlKemkey: a key which contains public information required for encapsulation.
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of ML-KEM.
// - (pbCiphertext, cbCiphertext): a buffer into which the encapsulated secret is written.
//   cbCiphertext must equal cbCiphertext given by SymCryptMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_*).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemDecapsulate(
    _In_                                    PCSYMCRYPT_MLKEMKEY pkMlKemkey,
    _In_reads_bytes_( cbCiphertext )        PCBYTE              pbCiphertext,
                                            SIZE_T              cbCiphertext,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE               pbAgreedSecret,
                                            SIZE_T              cbAgreedSecret );
//
// Performs the Decapsulate operation of ML-KEM.
// This uses the private information of an ML-KEM keypair to generate an agreed
// secret from a ciphertext.
//
// The arguments are the following:
// - pkMlKemkey: a key which contains private information required for decapsulation.
// - (pbCiphertext, cbCiphertext): a buffer containing an encapsulated secret.
//   cbCiphertext must equal cbCiphertext given by SymCryptMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_MLKEM_CIPHERTEXT_SIZE_*).
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of ML-KEM.
//
// Note: Given an invalid, but correctly-sized, ciphertext, the ML-KEM Decapsulation operation
// will "implicitly reject" the ciphertext, by returning success in equal time to a valid
// decapsulation operation, with pseudo-random agreed secret output. This forces higher
// level protocols to fail later when symmetric keys of peers do not match.
// So decapsulate will only ever return an error if there are programming errors (e.g. incorrect size),
// or something fundamentally goes wrong with the environment (e.g. internal memory allocation fails).
//

VOID
SYMCRYPT_CALL
SymCryptMlKemSelftest(void);
//
// FIPS self-test for ML-KEM. If the self-test fails, SymCryptFatal will be called to fastfail.
// The self-test will automatically be performed before first operational use of ML-KEM if using
// keys with FIPS validation, so most callers should never use this function.
//

//
// COMPOSITE MLKEMKEY objects' API
//
// The below formats apply **only to external formats**: When somebody is importing or exporting
// a key. The internal format of the keys is not visible to the caller.
typedef enum _SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT {
    SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_NULL              = 0,
    SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_IRTF_PRIVATE_SEED = 1,
        // 32-byte seed for deriving Composite ML-KEM key, per irtf-cfrg-hybrid-kems CG framework
    SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_LAMPS_PRIVATE_KEY = 2,
        // Standard byte encoding of a Composite ML-KEM private key, per LAMPS composite ML-KEM draft 12.
        // Concatenation of ML-KEM private seed and private key of the traditional component:
        //   mlkemSeed || tradSK
        // Size in bytes are MLKEM768_P256: 115, MLKEM768_X25519: 96, MLKEM1024_P384: 128
    SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_PUBLIC_KEY        = 3,
        // Standard byte encoding of a Composite ML-KEM public key, per irtf-cfrg-hybrid-kems CG framework
        // and LAMPS composite ML-KEM draft 12.
        // Concatenation of ML-KEM encapsulation key and public key of the traditional component:
        //   mlkemPK || tradPK
        // Size in bytes are MLKEM768_P256: 1249, MLKEM768_X25519: 1216, MLKEM1024_P384: 1665
} SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT;


typedef enum _SYMCRYPT_COMPOSITE_MLKEM_PARAMS {
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS_NULL            = 0,
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_P256   = 1,
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_X25519 = 2,
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM1024_P384  = 3,
} SYMCRYPT_COMPOSITE_MLKEM_PARAMS;
//
// Currently supported Composite ML-KEM parameter sets are represented externally only by the enum
//

PSYMCRYPT_COMPOSITE_MLKEMKEY
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyAllocate(
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS   params );
//
// Allocate and create a new COMPOSITE_MLKEMKEY object sized according to the specified parameters.
//
// This call does not initialize the key. It should be
// followed by a call to SymCryptCompositeMlKemkeyGenerate or
// SymCryptCompositeMlKemkeySetValue.
//

VOID
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyFree(
    _Inout_ PSYMCRYPT_COMPOSITE_MLKEMKEY    pkCompositeMlKemkey );


#define SYMCRYPT_COMPOSITE_MLKEM_IRTF_PRIVATE_SEED_SIZE (32)

#define SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_P256   (115)
#define SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_X25519 (96)
#define SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM1024_P384  (128)

#define SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_P256      (1249)
#define SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_X25519    (1216)
#define SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM1024_P384     (1665)


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemSizeofKeyFormatFromParams(
            SYMCRYPT_COMPOSITE_MLKEM_PARAMS     params,
            SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemkeyformat,
    _Out_   SIZE_T*                             pcbKeyFormat );
//
// Gives the size in bytes of the blob of the given format for the given Composite ML-KEM
// parameters via pcbKeyFormat output.
// Returns SYMCRYPT_INCOMPATIBLE_FORMAT if compositeMlKemkeyformat is an unsupported value,
// or SYMCRYPT_INVALID_ARGUMENT if other parameters are invalid.
//

#define SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM768_P256    (1153)
#define SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM768_X25519  (1120)
#define SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM1024_P384   (1665)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemSizeofCiphertextFromParams(
            SYMCRYPT_COMPOSITE_MLKEM_PARAMS params,
    _Out_   SIZE_T*                         pcbCiphertext );
//
// Gives the size in bytes of the ciphertext for the given Composite ML-KEM parameters.
// Returns SYMCRYPT_INVALID_ARGUMENT if parameters are invalid.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyGenerate(
    _Inout_ PSYMCRYPT_COMPOSITE_MLKEMKEY    pkCompositeMlKemkey,
            UINT32                          flags );
//
// Generate a new random Composite ML-KEM key using the information from the
// parameters passed to SymCryptCompositeMlKemkeyAllocate.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// Described in more detail in the "Flags for asymmetric key generation and import" section above
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeySetValue(
    _In_reads_bytes_( cbSrc )   PCBYTE                              pbSrc,
                                SIZE_T                              cbSrc,
                                SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemkeyFormat,
                                UINT32                              flags,
    _Inout_                     PSYMCRYPT_COMPOSITE_MLKEMKEY        pkCompositeMlKemkey );
//
// Import key material to a Composite ML-KEM key object. The arguments are the following:
//  - (pbSrc, cbSrc): a buffer containing a representation of a Composite ML-KEM key,
//    in format specified by compositeMlKemkeyFormat.
//  - compositeMlKemkeyFormat format of the input
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// - SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION
//   Opt-out of performing almost all validation - must be specified with SYMCRYPT_FLAG_KEY_NO_FIPS
//
// Remarks:
// - cbSrc must be equal to the cbKeyFormat returned from
//   SymCryptCompositeMlKemSizeofKeyFormatFromParams(params, compositeMlKemkeyFormat, &cbKeyFormat), though
//   typically this value can be known statically (see definition of SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyGetValue(
    _In_                        PCSYMCRYPT_COMPOSITE_MLKEMKEY       pkCompositeMlKemkey,
    _Out_writes_bytes_( cbDst ) PBYTE                               pbDst,
                                SIZE_T                              cbDst,
                                SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemkeyFormat,
                                UINT32                              flags );
//
// Export key material from a Composite ML-KEM key object. The arguments are the following:
//  - (pbDst, cbDst): a buffer into which a representation of a Composite ML-KEM key is
//    written, in the format specified by compositeMlKemkeyFormat.
//  - compositeMlKemkeyFormat format of the output
//
// Allowed flags:
//      - None.
//
//  Remarks:
//  - If the key object does not have the information required to export to the format
//    specified by compositeMlKemkeyFormat this function will return SYMCRYPT_INCOMPATIBLE_FORMAT.
//  - cbDst must be equal to the cbKeyFormat returned from
//    SymCryptCompositeMlKemSizeofKeyFormatFromParams(params, compositeMlKemkeyFormat, &cbKeyFormat), though typically this
//    value can be known statically (see definition of SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemEncapsulate(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE                           pbCiphertext,
                                            SIZE_T                          cbCiphertext );
//
// Performs the Encapsulate operation of Composite ML-KEM.
// This uses the public information of a Composite ML-KEM keypair to generate an agreed secret
// and a ciphertext. Only a peer with the private information of a Composite ML-KEM keypair can
// decapsulate the ciphertext to compute the agreed secret.
//
// The arguments are the following:
// - pkCompositeMlKemkey: a key which contains public information required for encapsulation.
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of Composite ML-KEM.
// - (pbCiphertext, cbCiphertext): a buffer into which the encapsulated secret is written.
//   cbCiphertext must equal cbCiphertext given by SymCryptCompositeMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_*).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemDecapsulate(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _In_reads_bytes_( cbCiphertext )        PCBYTE                          pbCiphertext,
                                            SIZE_T                          cbCiphertext,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret );
//
// Performs the Decapsulate operation of Composite ML-KEM.
// This uses the private information of a Composite ML-KEM keypair to generate an agreed
// secret from a ciphertext.
//
// The arguments are the following:
// - pkCompositeMlKemkey: a key which contains private information required for decapsulation.
// - (pbCiphertext, cbCiphertext): a buffer containing an encapsulated secret.
//   cbCiphertext must equal cbCiphertext given by SymCryptCompositeMlKemSizeofCiphertextFromParams,
//   though typically this value can be known statically (see definition of
//   SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_*).
// - (pbAgreedSecret, cbAgreedSecret): a buffer into which the generated secret is written.
//   Currently cbAgreedSecret must be 32 for all parameterizations of Composite ML-KEM.
//
// Note: Given an invalid, but correctly-sized, ciphertext, the Composite ML-KEM Decapsulation operation
// will "implicitly reject" the ciphertext, by returning success in equal time to a valid
// decapsulation operation, with pseudo-random agreed secret output. This forces higher
// level protocols to fail later when symmetric keys of peers do not match.
// So decapsulate will only ever return an error if there are programming errors (e.g. incorrect size),
// or something fundamentally goes wrong with the environment (e.g. internal memory allocation fails).
//

////////////////////////////////////////////////////////////
// Module-Lattice-Based Digital Signature Algorithm (ML-DSA)
////////////////////////////////////////////////////////////

// Maximum length of the context string used in signing and verification
#define SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH   (255)

// ML-DSA key formats
// ==================
// The below formats apply **only to external formats**: When somebody is importing or exporting
// a key. The internal format of the keys is not visible to the caller.
typedef enum _SYMCRYPT_MLDSAKEY_FORMAT {
    SYMCRYPT_MLDSAKEY_FORMAT_NULL               = 0,
    SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED       = 1,
        // 32-byte private root seed xi from which all other parameters can be derived.
        // On its own it is ambiguous which ML-DSA parameter set this represents; callers wanting to
        // store this format must track the parameter set alongside the key.
    SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_KEY        = 2,
        // Standard byte encoding of an ML-DSA private key, per FIPS 204.
        // Size is 2560, 4032, or 4896 bytes for ML-DSA 44, 65, and 87 respectively.
    SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY         = 3,
        // Standard byte encoding of an ML-DSA public key, per FIPS 204.
        // Size is 1312, 1952, or 2592 bytes for ML-DSA 44, 65, and 87 respectively.
} SYMCRYPT_MLDSAKEY_FORMAT;

typedef enum _SYMCRYPT_MLDSA_PARAMS {
    SYMCRYPT_MLDSA_PARAMS_NULL                  = 0,
    SYMCRYPT_MLDSA_PARAMS_MLDSA44               = 1,
    SYMCRYPT_MLDSA_PARAMS_MLDSA65               = 2,
    SYMCRYPT_MLDSA_PARAMS_MLDSA87               = 3,
} SYMCRYPT_MLDSA_PARAMS;
// Currently supported ML-DSA parameter sets are represented externally only by the enum

typedef enum _SYMCRYPT_PQDSA_HASH_ID {
    SYMCRYPT_PQDSA_HASH_ID_NULL                = 0,
    SYMCRYPT_PQDSA_HASH_ID_SHA256              = 1,
    SYMCRYPT_PQDSA_HASH_ID_SHA384              = 2,
    SYMCRYPT_PQDSA_HASH_ID_SHA512              = 3,
    SYMCRYPT_PQDSA_HASH_ID_SHA512_256          = 4,
    SYMCRYPT_PQDSA_HASH_ID_SHA3_256            = 5,
    SYMCRYPT_PQDSA_HASH_ID_SHA3_384            = 6,
    SYMCRYPT_PQDSA_HASH_ID_SHA3_512            = 7,
    SYMCRYPT_PQDSA_HASH_ID_SHAKE128            = 8,
    SYMCRYPT_PQDSA_HASH_ID_SHAKE256            = 9,
} SYMCRYPT_PQDSA_HASH_ID;
// Supported hash algorithms for use with Hash-ML-DSA

//========================================================================
// MLDSAKEY objects' API
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSizeofKeyFormatFromParams(
            SYMCRYPT_MLDSA_PARAMS       params,
            SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
    _Out_   SIZE_T*                     pcbKeyFormat );
//
// Gives the size in bytes of the blob of the given format for the given ML-DSA
// parameters and the specified format via pcbKeyFormat output.
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INCOMPATIBLE_FORMAT if mlDsakeyFormat is an unsupported value.
// - SYMCRYPT_INVALID_ARGUMENT if other parameters are invalid.
//

#define SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA44    (2420)
#define SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA65    (3309)
#define SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA87    (4627)

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSizeofSignatureFromParams(
            SYMCRYPT_MLDSA_PARAMS       params,
    _Out_   SIZE_T*                     pcbSignature );
//
// Gives the size in bytes of the signature for the given ML-DSA parameters.
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INVALID_ARGUMENT if parameters are invalid.
//

_Success_( return != NULL )
PSYMCRYPT_MLDSAKEY
SYMCRYPT_CALL
SymCryptMlDsakeyAllocate(
            SYMCRYPT_MLDSA_PARAMS       params );
//
// Allocate a new ML-DSA key object sized according to the parameters.
//
// This call does not generate key material. It should be followed by a call to
// SymCryptMlDsakeyGenerate or SymCryptMlDsakeySetValue.
//
// May return NULL if memory allocation fails.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsakeyFree(
    _Post_invalid_  PSYMCRYPT_MLDSAKEY  pkMlDsakey );
//
// Free an ML-DSA key object that was allocated with SymCryptMlDsakeyAllocate.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeyGenerate(
    _Inout_     PSYMCRYPT_MLDSAKEY      pkMlDsakey,
                UINT32                  flags );
//
// Generate a new random ML-DSA key using the information from the
// parameters passed to SymCryptMlDsakeyAllocate.
//
// Parameters:
// - pkMlDsakey: a pointer to an ML-DSA key object allocated with SymCryptMlDsakeyAllocate
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_MEMORY_ALLOCATION_FAILURE if memory allocation fails.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeySetValue(
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_MLDSAKEY          pkMlDsakey );
//
// Import key material to an ML-DSA key object from a byte blob.
//
// Parameters:
// - (pbSrc, cbSrc): a buffer containing a representation of an ML-DSA key, in the format specified
//   by the format parameter.
// - mlDsakeyFormat: format of the input
// - pkMlDsakey: a pointer to an ML-DSA key object allocated with SymCryptMlDsakeyAllocate.
//
// Allowed flags:
//
// - SYMCRYPT_FLAG_KEY_NO_FIPS
//   Opt-out of performing validation required for FIPS
//
// Remarks:
// - cbSrc must be equal to the cbKeyFormat returned from
//   SymCryptMlDsaSizeofKeyFormatFromParams(params, format, &cbKeyFormat), though typically this
//   value can be known statically (see definition of SYMCRYPT_MLDSAKEY_FORMAT)
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INCOMPATIBLE_FORMAT if the key format is invalid.
// - SYMCRYPT_INVALID_ARGUMENT if other arguments are invalid.
// - SYMCRYPT_WRONG_KEY_SIZE if cbSrc does not match the expected size for the key format.
// - SYMCRYPT_INVALID_BLOB if the encoded key is invalid.
// - SYMCRYPT_MEMORY_ALLOCATION_FAILURE if memory allocation fails.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsakeyGetValue(
    _In_                        PCSYMCRYPT_MLDSAKEY         pkMlDsakey,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst,
                                SYMCRYPT_MLDSAKEY_FORMAT    mlDsakeyFormat,
                                UINT32                      flags );
//
// Export key material from an ML-DSA key object to a byte blob.
//
// Parameters:
// - pkMlDsakey: pointer to a valid ML-DSA key object.
// - (pbDst, cbDst): buffer for the exported ML-DSA key, in the format specified by the format
//   parameter.
// - mlDsakeyFormat: format of the output
// - flags: no flags are currently defined; must be set to 0
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INCOMPATIBLE_FORMAT if the key object does not have the information required to export
//   the format specified by mlDsakeyFormat.
// - SYMCRYPT_INVALID_ARGUMENT if the output buffer size or other arguments are incorrect.
//
// Remarks:
// - cbDst must be equal to the cbKeyFormat returned from
//   SymCryptMlDsaSizeofKeyFormatFromParams(params, format, &cbKeyFormat), though typically this
//   value can be known statically (see definition of SYMCRYPT_MLDSAKEY_FORMAT)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSign(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_bytes_( cbMessage )                       PCBYTE              pbMessage,
                                                        SIZE_T              cbMessage,
    _In_reads_bytes_opt_( cbContext )                   PCBYTE              pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T              cbContext,
                                                        UINT32              flags,
    _Out_writes_bytes_( cbSignature )                   PBYTE               pbSignature,
                                                        SIZE_T              cbSignature );
//
// Sign a message using "pure" ML-DSA. The message can be of arbitrary length.
//
// Parameters:
// - pkMlDsakey: an ML-DSA key object. Must contain the private key material.
// - (pbMessage, cbMessage): the message to sign. May be of arbitrary length.
// - (pbContext, cbContext): an optional context string which will be included in the message
//   representative to be signed. Length must be <= SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH.
// - flags: no flags are currently defined; must be set to 0
// - (pbSignature, cbSignature): the buffer into which the signature is written.
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INVALID_ARGUMENT if the key object does not contain a private key, or if other
//   parameters are invalid.
// - SYMCRYPT_MEMORY_ALLOCATION_FAILURE if memory allocation fails.
//
// Remarks:
//   cbSignature must be equal to the cbKeyFormat returned from
//   SymCryptMlDsaSizeofSignatureFromParams( params, &cbSignature ), though typically this
//   value can be known statically (see definition of SYMCRYPT_MLDSA_SIGNATURE_SIZE_*).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptExternalMuMlDsaSign(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_bytes_( cbMu )                            PCBYTE              pbMu,
                                                        SIZE_T              cbMu,
                                                        UINT32              flags,
    _Out_writes_bytes_( cbSignature )                   PBYTE               pbSignature,
                                                        SIZE_T              cbSignature );
//
// Sign a precomputed message representative Mu.
//
// Parameters:
// - (pbMu, cbMu): the message representative to sign,
//   which must be of size 64 (SYMCRYPT_SHAKE256_RESULT_SIZE).
// - All other parameters are the same as for SymCryptMlDsaSign.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaSign(
    _In_                                                PCSYMCRYPT_MLDSAKEY     pkMlDsakey,
                                                        SYMCRYPT_PQDSA_HASH_ID  hashAlg,
    _In_reads_bytes_( cbHash )                          PCBYTE                  pbHash,
                                                        SIZE_T                  cbHash,
    _In_reads_bytes_opt_( cbContext )                   PCBYTE                  pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T                  cbContext,
                                                        UINT32                  flags,
    _Out_writes_bytes_( cbSignature )                   PBYTE                   pbSignature,
                                                        SIZE_T                  cbSignature );
//
// Sign a message using "pre-hash" ML-DSA. The caller precomputes the hash of the message.
//
// Parameters:
// - hashAlg: the ID of the hash algorithm used to compute pbHash.
// - (pbHash, cbHash): the hash of the message to sign.
// - All other parameters are the same as for SymCryptMlDsaSign.
//
// Return values:
// - SYMCRYPT_NO_ERROR on success.
// - SYMCRYPT_INVALID_ARGUMENT if the key object does not contain a private key, or if other
//   parameters are invalid.
// - SYMCRYPT_MEMORY_ALLOCATION_FAILURE if memory allocation fails.
//
// Remarks:
//   The hash algorithm provided must meet the minimum required collision strength defined for the
//   chosen ML-DSA parameter set. This is the lambda parameter in FIPS 204. This means that the
//   following hash algorithms are supported:
//
//   ML-DSA-44 (lambda = 128): SHA-256, SHA-384, SHA-512, SHA-512/256, SHA3-256, SHA3-384, SHA3-512, SHAKE128, SHAKE256
//   ML-DSA-65 (lambda = 192): SHA-384, SHA-512, SHA3-384, SHA3-512, SHAKE256
//   ML-DSA-87 (lambda = 256): SHA-512, SHA3-512, SHAKE256
//
//   Additionally, cbHash must match the output length of the hash algorithm.
//   For XOFs, the any output length >= the minimum collision strength is acceptable. If this
//   requirement is not met, the function returns SYMCRYPT_INVALID_ARGUMENT.
//
//   As with SymCryptMlDsaSign, cbSignature must be equal to the cbKeyFormat returned from
//   SymCryptMlDsaSizeofSignatureFromParams( params, &cbSignature ), though typically this
//   value can be known statically (see definition of SYMCRYPT_MLDSA_SIGNATURE_SIZE_*).
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVerify(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_bytes_( cbMessage )                       PCBYTE              pbMessage,
                                                        SIZE_T              cbMessage,
    _In_reads_bytes_opt_( cbContext )                   PCBYTE              pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T              cbContext,
    _In_reads_bytes_( cbSignature )                     PCBYTE              pbSignature,
                                                        SIZE_T              cbSignature,
                                                        UINT32              flags );
//
// Verify a signature using "pure" ML-DSA. The message can be of arbitrary length.
//
// Parameters:
// - pkMlDsakey: the ML-DSA key object used to verify the signature.
// - (pbMessage, cbMessage): the message that the signature was generated from.
// - (pbContext, cbContext): an optional context string which will be included in the message
//   representative to be signed. Length must be <= SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH.
// - (pbSignature, cbSignature): the signature to verify.
// - flags: no flags are currently defined; must be set to 0
//
// Return values:
// - SYMCRYPT_NO_ERROR if the signature was verified successfully.
// - SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE if the signature is invalid.
// - SYMCRYPT_INVALID_ARGUMENT if the parameters are invalid.

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptExternalMuMlDsaVerify(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_bytes_( cbMu )                            PCBYTE              pbMu,
                                                        SIZE_T              cbMu,
    _In_reads_bytes_( cbSignature )                     PCBYTE              pbSignature,
                                                        SIZE_T              cbSignature,
                                                        UINT32              flags );
//
// Verify a signature of a precomputed message representative Mu.
//
// Parameters:
// - (pbMu, cbMu): the message representative that was signed,
//   which must be of size 64 (SYMCRYPT_SHAKE256_RESULT_SIZE).
// - All other parameters are the same as for SymCryptMlDsaVerify.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaVerify(
    _In_                                                PCSYMCRYPT_MLDSAKEY     pkMlDsakey,
                                                        SYMCRYPT_PQDSA_HASH_ID  hashAlg,
    _In_reads_bytes_( cbHash )                          PCBYTE                  pbHash,
                                                        SIZE_T                  cbHash,
    _In_reads_bytes_opt_( cbContext )                   PCBYTE                  pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T                  cbContext,
    _In_reads_bytes_( cbSignature )                     PCBYTE                  pbSignature,
                                                        SIZE_T                  cbSignature,
                                                        UINT32                  flags );
//
// Verify a signature using "pre-hash" ML-DSA. The caller precomputes the hash of the message.
//
// Parameters:
// - hashAlg: the ID of the hash algorithm used to compute pbHash.
// - (pbHash, cbHash): the hash of the message that the signature was generated from.
// - All other parameters are the same as for SymCryptMlDsaVerify.
//
// Return values:
// - SYMCRYPT_NO_ERROR if the signature was validated successfully.
// - SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE if the signature is invalid.
// - SYMCRYPT_INVALID_ARGUMENT if the parameters are invalid.
//
// Remarks:
//   See the remarks for SymCryptHashMlDsaSign regarding the required security strength of the hash
//   algorithm. For unsupported hash algorithms, the function will return SYMCRYPT_INVALID_ARGUMENT.

VOID
SYMCRYPT_CALL
SymCryptMlDsaSelftest( void );
//
// FIPS selftest for ML-DSA
//

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatal( UINT32 fatalCode );
//
// Call the Fatal routine passed to the library upon initialization
// We use the SYMCRYPT_ASSERT macro to catch problems in Debug builds
//


typedef struct _SYMCRYPT_UINT32_MAP {
    UINT32              from;       // map this value...
    UINT32              to;         // ...into this value
} SYMCRYPT_UINT32_MAP, *PSYMCRYPT_UINT32_MAP;
typedef const SYMCRYPT_UINT32_MAP * PCSYMCRYPT_UINT32_MAP;


UINT32
SYMCRYPT_CALL
SymCryptMapUint32(
                        UINT32                  u32Input,
                        UINT32                  u32Default,
    _In_reads_( nMap )  PCSYMCRYPT_UINT32_MAP   pcMap,
                        SIZE_T                  nMap );
//
// Map values in a side-channel safe way, typically used for mapping error codes.
//
// (pcMap, nMap) point to an array of nMap entries of type SYMCRYPT_UINT32_MAP;
// each entry specifies a single mapping. If u32Input matches the
// 'from' field, the return value will be the 'to' field value.
// If u32Input is not equal to any 'from' field values, the return value is u32Default.
// Both u32Input and the return value are treated as secrets w.r.t. side channels.
//
// If multiple map entries have the same 'from' field value, then the return value
// is one of the several 'to' field values; which one is not defined.
//
// This function is particularly useful when mapping error codes in situations where
// the actual error cannot be revealed through side channels.

#if SYMCRYPT_DEBUG
#define SYMCRYPT_ASSERT( _x ) \
    {\
        if( !(_x) ){ SymCryptFatal( 'asrt' ); }\
    }\
    _Analysis_assume_( _x )
#else
#define SYMCRYPT_ASSERT( _x ) \
    _Analysis_assume_( _x )
#endif


#ifdef __cplusplus
}
#endif
