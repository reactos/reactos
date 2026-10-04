//
// sc_lib_mldsa.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Internal ML-DSA definitions for the symcrypt library.
// Always intended to be included as part of sc_lib.h
//

//
// Modulus for ML-DSA
//
#define SYMCRYPT_MLDSA_Q (8380417)

//
// Montgomery multiplier for ML-DSA, log 2 (i.e. R = 2^32)
//
#define SYMCRYPT_MLDSA_R_LOG2 (32)

//
// Size of the root seed xi used in key generation
//
#define SYMCRYPT_MLDSA_ROOT_SEED_SIZE (32)

//
// Size of the public seed rho
//
#define SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE (32)

//
// Size of public key hash (tr) = SHAKE256 result size = 64 bytes
//
#define SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE SYMCRYPT_SHAKE256_RESULT_SIZE

//
// Size of private signing seed K
//
#define SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE (32)

//
// Size of the private vector seed rho prime
//
#define SYMCRYPT_MLDSA_PRIVATE_VECTOR_SEED_SIZE (64)

//
// Size of random value used in signing (rnd in FIPS 204)
//
#define SYMCRYPT_MLDSA_SIGNING_RANDOM_SIZE (32)

//
// Length of hash algorithm OIDs in bytes. Currently all supported hash algorithms have 11-byte
// OIDs, but this is not guaranteed to be the case as more algorithms are added in the future.
// If the OID length becomes variable, functions which use this value will need to be changed.
//
#define SYMCRYPT_MLDSA_SUPPORTED_HASH_OID_SIZE (11)

//
// Flag for Sign and Verify with External Mu
//
#define SYMCRYPT_FLAG_MLDSA_EXTERNALMU (0x1)

typedef struct _SYMCRYPT_MLDSA_POLYELEMENT {
    // PolyElements just store the coefficients without any header.
    UINT32      coeffs[SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS];
} SYMCRYPT_MLDSA_POLYELEMENT, * PSYMCRYPT_MLDSA_POLYELEMENT;
typedef const SYMCRYPT_MLDSA_POLYELEMENT* PCSYMCRYPT_MLDSA_POLYELEMENT;

// Maximum number of rows and columns in A matrix for ML-DSA
#define SYMCRYPT_MLDSA_VECTOR_MAX_LENGTH (8)
#define SYMCRYPT_MLDSA_MATRIX_MAX_NROWS  (8)
#define SYMCRYPT_MLDSA_MATRIX_MAX_NCOLS  (7)

typedef _Struct_size_bytes_( cbTotalSize ) struct _SYMCRYPT_MLDSA_VECTOR {
    _Field_range_( 1, SYMCRYPT_MLDSA_VECTOR_MAX_LENGTH )
    UINT8       nElems;         // Number of PolyElements in the vector
    UINT32      cbTotalSize;    // Total size of the Vector

    // Followed by:
    // nElems PolyElements
} SYMCRYPT_MLDSA_VECTOR, * PSYMCRYPT_MLDSA_VECTOR;
typedef const SYMCRYPT_MLDSA_VECTOR* PCSYMCRYPT_MLDSA_VECTOR;

typedef _Struct_size_bytes_( cbTotalSize ) struct _SYMCRYPT_MLDSA_MATRIX {
    _Field_range_( 1, SYMCRYPT_MLDSA_MATRIX_MAX_NROWS )
    UINT8       nRows;          // k in FIPS-204
    _Field_range_( 1, SYMCRYPT_MLDSA_MATRIX_MAX_NCOLS )
    UINT8       nCols;          // l in FIPS-204
    UINT32      cbTotalSize;    // Total size of the Matrix

    // Followed by:
    // nRows*nCols PolyElements in row-major order
} SYMCRYPT_MLDSA_MATRIX, * PSYMCRYPT_MLDSA_MATRIX;
typedef const SYMCRYPT_MLDSA_MATRIX* PCSYMCRYPT_MLDSA_MATRIX;

typedef struct _SYMCRYPT_MLDSA_INTERNAL_PARAMS {
    UINT32 params;                      // parameter set of ML-DSA being used - takes a value from SYMCRYPT_MLDSA_PARAMS

    UINT32 cbPolyElement;               // size in bytes of one polynomial ring element
    UINT32 cbRowVector;                 // size in bytes of one row vector (k elements)
    UINT32 cbColVector;                 // size in bytes of one column vector (l elements)
    UINT32 cbMatrix;                    // size in bytes of one matrix

    UINT8 nRows;                        // Number of rows in the A matrix (k in FIPS-204)
    UINT8 nCols;                        // Number of columns in the A matrix (l in FIPS-204)

    UINT8 privateKeyRange;              // Coefficient range of s1, s2 private key vectors (eta in FIPS-204)
    UINT8 encodedCoefficientBitLength;  // Bit length of encoded private key coefficients

    UINT8 nChallengeNonZeroCoeffs;        // Number of non-zero coefficients in the challenge polynomial (tau in FIPS-204)
    UINT8 nHintNonZeroCoeffs;             // Max number of non-zero coefficients in the hint polynomial (omega in FIPS-204)
    UINT8 maskCoefficientRangeLog2;       // Coefficient range of mask polynomial y (log_2(gamma_1) in FIPS-204)
    UINT8 commitmentModulus;              // Modulus for commitment values in UseHint and MakeHint (q-1)/(2*gamma_2)
    UINT32 decomposeR1Factor;             // Multiplication factor for R1 in SymCryptMlDsaDecompose - see function comments
    UINT32 commitmentRoundingRange;       // Rounding range for commitment value (gamma_2 in FIPS-204)
    UINT32 w1EncodeCoefficientBitLength;  // Bit length of coefficients for w1 encoding (q - 1) / ((2 * gamma_2) - 1))

    UINT32 cbCommitmentHash;            // Size of the commitment hash (lambda / 4 in FIPS 204)
    UINT32 cbEncodedPrivateKey;         // Size of the encoded private key
    UINT32 cbEncodedPublicKey;          // Size of the encoded public key
    UINT32 cbEncodedSignature;          // Size of the encoded signature
} SYMCRYPT_MLDSA_INTERNAL_PARAMS, * PSYMCRYPT_MLDSA_INTERNAL_PARAMS;
typedef const SYMCRYPT_MLDSA_INTERNAL_PARAMS* PCSYMCRYPT_MLDSA_INTERNAL_PARAMS;

typedef _Struct_size_bytes_( cbTotalSize ) struct _SYMCRYPT_MLDSAKEY {
    UINT32                  fAlgorithmInfo; // Tracks which algorithms the key can be used in (not currently used)
                                            // Also tracks which per-key selftests have been performed on this key
                                            // A bitwise OR of SYMCRYPT_FLAG_KEY_*, SYMCRYPT_FLAG_MLDSAKEY_*, and
                                            // SYMCRYPT_SELFTEST_KEY_* values

    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pParams;

    UINT32 cbTotalSize;             // Total in-memory size of the ML-DSA key (this header and the following structs)

    BOOLEAN hasRootSeed;            // True if the key has the seed used in key generation (xi)
    BOOLEAN hasPrivateKey;          // True if the key has private vectors s1, s2, t0

    // Seeds
    _When_( hasRootSeed, _Field_size_bytes_(SYMCRYPT_MLDSA_ROOT_SEED_SIZE) )
    _When_( !hasRootSeed, _Field_size_bytes_part_(SYMCRYPT_MLDSA_ROOT_SEED_SIZE, 0) )
    BYTE rootSeed[SYMCRYPT_MLDSA_ROOT_SEED_SIZE];       // Root seed used in key generation (xi) - only available for keys generated by SymCrypt, or imported from a seed

    _When_( hasPrivateKey, _Field_size_bytes_(SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE) )
    _When_( !hasPrivateKey, _Field_size_bytes_part_(SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE, 0) )
    BYTE privateSigningSeed[SYMCRYPT_MLDSA_PRIVATE_SIGNING_SEED_SIZE];  // Private seed used in signing (K)

    BYTE publicSeed[SYMCRYPT_MLDSA_PUBLIC_SEED_SIZE];   // Public seed from which A can be derived (rho)
    BYTE publicKeyHash[SYMCRYPT_MLDSA_PUBLIC_KEY_HASH_SIZE];  // SHAKE-256 hash of the public key

    //
    // ML-DSA matrix/vector components: A * s1 + s2 = t
    //
    // t is separated into two components, t0 and t1, using Power2Round. t0 is private and is used
    // during signing; t1 is public and is used during verification. All components are stored in
    // NTT form so that we do not need to convert them during signing or verification.
    //

    // Public components - always valid
    PSYMCRYPT_MLDSA_MATRIX pmA;    // Public matrix A - size nRows x nCols
    PSYMCRYPT_MLDSA_VECTOR pvt1;   // Public component of t vector from Power2Round (row vector)

    // Private components - only valid when hasPrivateKey is TRUE
    PSYMCRYPT_MLDSA_VECTOR pvs1;   // Private vector s1 (column vector)
    PSYMCRYPT_MLDSA_VECTOR pvs2;   // Private vector s2 (row vector)
    PSYMCRYPT_MLDSA_VECTOR pvt0;   // Private component of t vector from Power2Round (row vector)

    SYMCRYPT_MAGIC_FIELD
    // Followed by:
    // A
    // t1
    // s1
    // s2
    // t0
} SYMCRYPT_MLDSAKEY, * PSYMCRYPT_MLDSAKEY;
typedef const SYMCRYPT_MLDSAKEY* PCSYMCRYPT_MLDSAKEY;

typedef _Struct_size_bytes_(cbTotalSize) struct _SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES
{
    UINT32                          cbTotalSize;     // Total in-memory size of this structure
    UINT32                          nRowVectors;     // Number of row vectors
    UINT32                          nColVectors;     // Number of column vectors
    UINT32                          nPolyElements;   // Number of PolyElements
    UINT32                          cbScratch;       // Size of scratch buffer


    SYMCRYPT_SHAKE256_STATE         shake256State;

    _Field_size_( nRowVectors )
    PSYMCRYPT_MLDSA_VECTOR*         pvRowVectors;   // Array of pointers to row vectors
    _Field_size_( nColVectors )
    PSYMCRYPT_MLDSA_VECTOR*         pvColVectors;   // Array of pointers to column vectors
    _Field_size_( nPolyElements)
    PSYMCRYPT_MLDSA_POLYELEMENT*    pePolyElements; // Array of pointers to PolyElements

    _Field_size_bytes_( cbScratch )
    PBYTE                           pbScratch;

    SYMCRYPT_MAGIC_FIELD
    // Followed by:
    // pvRowVectors[0..nRowVectors-1]
    // pvColVectors[0..nColVectors-1]
    // pePolyElements[0..nPolyElements-1]
    // nRowVectors * SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( nRows ) buffer for row vectors
    // nColVectors * SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( nCols ) buffer for column vectors
    // nPoly * SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT buffer for PolyElements
    // cbScratch bytes of scratch space
} SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES, * PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES;

#define SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT                ( sizeof( SYMCRYPT_MLDSA_POLYELEMENT ) )
#define SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( _nElems )          ( sizeof( SYMCRYPT_MLDSA_VECTOR ) + ( _nElems * sizeof( SYMCRYPT_MLDSA_POLYELEMENT ) ) )
#define SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX( _nRows, _nCols )   ( sizeof( SYMCRYPT_MLDSA_MATRIX ) + ( _nRows * _nCols * sizeof( SYMCRYPT_MLDSA_POLYELEMENT ) ) )
#define SYMCRYPT_INTERNAL_MLDSA_SIZEOF_KEY( _nRows, _nCols )      ( sizeof( SYMCRYPT_MLDSAKEY) + \
                                                                    SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX( _nRows, _nCols ) + \
                                                                    SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( _nCols ) + \
                                                                    (SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( _nRows ) * 3u) )

#define SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT_OFFSET( _row )     ( sizeof( SYMCRYPT_MLDSA_VECTOR ) + (_row * SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT) )
#define SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT( _row, _pVector )  ((PSYMCRYPT_MLDSA_POLYELEMENT) ( ((PBYTE) (_pVector)) + SYMCRYPT_INTERNAL_MLDSA_VECTOR_ELEMENT_OFFSET( _row ) ))
#define SYMCRYPT_INTERNAL_MLDSA_MATRIX_ELEMENT_OFFSET( _row, _col, _pMatrix )  ( sizeof( SYMCRYPT_MLDSA_MATRIX ) + ((_row * (_pMatrix)->nCols + _col) * SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT) )
#define SYMCRYPT_INTERNAL_MLDSA_MATRIX_ELEMENT( _row, _col, _pMatrix) ((PSYMCRYPT_MLDSA_POLYELEMENT) ( ((PBYTE) (_pMatrix)) + SYMCRYPT_INTERNAL_MLDSA_MATRIX_ELEMENT_OFFSET( _row, _col, _pMatrix ) ))

#define SYMCRYPT_INTERNAL_MLDSA_SIZEOF_ENCODED_VECTOR( _pVector, _nBitsPerCoeff ) ( ((_pVector)->nElems * SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS * (_nBitsPerCoeff) ) / 8u )

// For packing signed coefficients into the minimum possible number of bits for encoding, ML-DSA
// converts them to (signed upper bound - x) for each coefficient x. For example, when encoding
// s1 and s2 which have coefficients in the range [-eta, eta] with ML-DSA-65 (eta = 4), 1 is encoded
// as (4 - 1) = 3, 0 is encoded as (4 - 0) = 4, -1 is encoded as (4 - (-1)) = 5, etc. Conveniently,
// this also works in reverse to decode the coefficients.
#define SYMCRYPT_INTERNAL_MLDSA_SHORT_COEFFICIENT_ENCODE_DECODE( _val, _bound)   ( _bound - _val )

//////////////////////////////////////////////////////////////////////////
// Internal implementations of public APIs
//////////////////////////////////////////////////////////////////////////

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaKeyGenerateEx(
    _Inout_                     PSYMCRYPT_MLDSAKEY  pkMlDsakey,
    _In_reads_( cbRootSeed )    PCBYTE              pbRootSeed,
                                SIZE_T              cbRootSeed,
                                UINT32              flags );
//
// Implements SymCryptMlDsakeyGenerate. Takes a seed from the caller so that keys can be generated
// deterministically for testing.
//
// Parameters:
// - (pbRootSeed, cbRootSeed): The seed used to generate the key (xi in FIPS 204)
//
// See SymCryptMlDsakeyGenerate for additional documentation.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSignEx(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_( cbInput )                               PCBYTE              pbInput,
                                                        SIZE_T              cbInput,
    _In_reads_opt_( cbContext )                         PCBYTE              pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T              cbContext,
    _In_reads_opt_( cbHashOid )                         PCBYTE              pbHashOid,
                                                        SIZE_T              cbHashOid,
    _In_reads_( cbRandom )                              PCBYTE              pbRandom,
                                                        SIZE_T              cbRandom,
                                                        UINT32              flags,
    _Out_writes_( cbSignature )                         PBYTE               pbSignature,
                                                        SIZE_T              cbSignature );
//
// Implements SymCryptMlDsaSign, SymCryptExternalMuMlDsaSign, and SymCryptHashMlDsaSign.
// Takes the random value from the caller so that signing can be done deterministically for testing.
//
// Parameters:
// - (pbInput, cbInput): The message to be signed. For SymCryptMlDsaSign, this is the full message.
//   For SymCryptHashMlDsaSign, this is the hash of the message.
// - (pbContext, cbContext): An optional context string which will be prepended to the message.
// - (pbHashOid, cbHashOid): The DER-encoded OID of the hash algorithm used to hash the message,
//   when using SymCryptHashMlDsaSign. Must be NULL for SymCryptMlDsaSign.
// - (pbRandom, cbRandom): The random value used in the signing process (rnd in FIPS 204).
// - flags: 0 or SYMCRYPT_FLAG_MLDSA_EXTERNALMU.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVerifyEx(
    _In_                                                PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _In_reads_( cbInput )                               PCBYTE              pbInput,
                                                        SIZE_T              cbInput,
    _In_reads_opt_( cbContext )                         PCBYTE              pbContext,
    _In_range_( 0, SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH )  SIZE_T              cbContext,
    _In_reads_opt_( cbHashOid )                         PCBYTE              pbHashOid,
                                                        SIZE_T              cbHashOid,
    _In_reads_( cbSignature )                           PCBYTE              pbSignature,
                                                        SIZE_T              cbSignature,
                                                        UINT32              flags );
//
// Implements SymCryptMlDsaVerify, SymCryptExternalMuMlDsaVerify, and SymCryptHashMlDsaVerify.
//
// Parameters:
// - (pbInput, cbInput): The message to be verified. For SymCryptMlDsaVerify, this is the full
//   message. For SymCryptHashMlDsaVerify, this is the hash of the message.
// - (pbContext, cbContext): An optional context string which will be prepended to the message.
// - (pbHashOid, cbHashOid): The DER-encoded OID of the hash algorithm used to hash the message,
//   when using SymCryptHashMlDsaVerify. Must be NULL for SymCryptMlDsaVerify.
// - (pbSignature, cbSignature): The signature to be verified.
// - flags: 0 or SYMCRYPT_FLAG_MLDSA_EXTERNALMU.
//

_Success_( TRUE )
PSYMCRYPT_MLDSAKEY
SYMCRYPT_CALL
SymCryptMlDsakeyInitialize(
    _In_                        PCSYMCRYPT_MLDSA_INTERNAL_PARAMS pInternalParams,
    _Out_writes_bytes_(cbKey)   PBYTE                            pbKey,
                                UINT32                           cbKey );
//
// Initializes a SYMCRYPT_MLDSAKEY structure in the given buffer. The buffer size (cbKey) must
// be exactly equal to the size of the key structure, which can be calculated using
// SYMCRYPT_INTERNAL_MLDSA_SIZEOF_KEY.
//
// Parameters:
// - pInternalParams: Parameter set to use for the key.
// - (pbKey, cbKey): Buffer for the key structure.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsakeyComputeT(
    _In_    PCSYMCRYPT_MLDSA_MATRIX     pmA,
    _In_    PCSYMCRYPT_MLDSA_VECTOR     pvs1,
    _In_    PCSYMCRYPT_MLDSA_VECTOR     pvs2,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR      pvt0,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR      pvt1,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR      pvTmp,
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT peTmp );
//
// Helper function for computing the t vector in ML-DSA: A * s1 + s2 = t. Used by key generation
// and private key import. All inputs must be in NTT form. The outputs t0 and t1 are NOT returned
// in NTT form; it is the caller's responsibility to convert them when appropriate.
//
// Parameters:
// - pmA: Public matrix A
// - pvs1: Private vector s1.
// - pvs2: Private vector s2.
// - pvt0: Private component of t vector from Power2Round.
// - pvt1: Public component of t vector from Power2Round.
// - pvTmp: Temporary vector for intermediate computations.
// - peTmp: Temporary PolyElement for intermediate computations.
//

//////////////////////////////////////////////////////////////////////////
// Montgomery reduction and multiplication
//////////////////////////////////////////////////////////////////////////

UINT32
SYMCRYPT_CALL
SymCryptMlDsaMontReduce( UINT64 a );
//
// Montgomery reduction
// res = a * R^-1 mod Q.
//
// Note that this divides out a factor of R.
//

UINT32
SYMCRYPT_CALL
SymCryptMlDsaMontMul( UINT32 a, UINT32 b );
//
// Montgomery multiplication
// res = (a * b) / R mod Q
//
// Equivalent to SymCryptMlDsaMontReduce( (UINT64) a * b )
// As above, this divides out a factor of R, which can be compensated for in either input,
// or taken into account in the output.
//

//////////////////////////////////////////////////////////////////////////
// 32-bit modular arithmetic
//////////////////////////////////////////////////////////////////////////

UINT32
SYMCRYPT_CALL
SymCryptMlDsaModAdd( UINT32 a, UINT32 b );
//
// res := a + b mod Q
//
// Requirements: a < Q, b < Q
//

UINT32
SYMCRYPT_CALL
SymCryptMlDsaModSub( UINT32 a, UINT32 b );
//
// res := a - b mod Q
//
// Requirements: a < Q, b < Q
//

//////////////////////////////////////////////////////////////////////////
// Polynomial operations
//////////////////////////////////////////////////////////////////////////

_Success_( TRUE )
PSYMCRYPT_MLDSA_POLYELEMENT
SYMCRYPT_CALL
SymCryptMlDsaPolyElementCreate(
    _Inout_updates_( cbBuffer ) PBYTE   pbBuffer,
                                SIZE_T  cbBuffer );
//
// Initializes a SYMCRYPT_MLDSA_POLYELEMENT in the given buffer.
// cbBuffer must be equal to SYMCRYPT_INTERNAL_MLDSA_SIZEOF_POLYELEMENT.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementSetZero(
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peDst );
//
// Sets all coefficients to zero
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementNTT(
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peSrc );
//
// ML-DSA Polynomial Ring Element NTT:
//      peSrc = NTT(peSrc) per FIPS 204
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementINTT(
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peSrc );
//
// ML-DSA Polynomial Ring Element inverse NTT:
//      peSrc = InverseNTT(peSrc) per FIPS 204
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementMulR(
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peSrc );
//
// ML-DSA Polynomial multiplication by the Montgomery multiplier R:
//    peSrc = (peSrc * R) mod Q
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementMontMul(
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    _Out_   PSYMCRYPT_MLDSA_POLYELEMENT     peDst );
//
// ML-DSA Polynomial Montgomery multiplication:
//     peDst = (peSrc1 * peSrc2) ./ R
// where:
// * is polynomial multiplication given sources in NTT form
// ./ is coefficient-wise division and R is the Montgomery multiplier
//
// Requirements:
//  - peSrc1 and peSrc2 must be PolyElements in ML-DSA NTT form
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementAdd(
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    _Out_   PSYMCRYPT_MLDSA_POLYELEMENT     peDst );
//
// ML-DSA Polynomial Ring Element addition
//      peDst = peSrc1 + peSrc2
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementSub(
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc1,
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    _Out_   PSYMCRYPT_MLDSA_POLYELEMENT     peDst );
//
// ML-DSA Polynomial Ring Element subtraction
//      peDst = peSrc1 - peSrc2
//

//////////////////////////////////////////////////////////////////////////
// Vector operations
//////////////////////////////////////////////////////////////////////////

_Success_( TRUE )
PSYMCRYPT_MLDSA_VECTOR
SYMCRYPT_CALL
SymCryptMlDsaVectorCreate(
    _Out_writes_( cbBuffer )    PBYTE   pbBuffer,
                                UINT32  cbBuffer,
                                UINT8   nElems );
//
// Initializes a vector of nElems PolyElements in the given buffer.
// cbBuffer must be equal to SYMCRYPT_INTERNAL_MLDSA_SIZEOF_VECTOR( nElems ).
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorCopy(
    _In_      PCSYMCRYPT_MLDSA_VECTOR         pvSrc,
    _Inout_   PSYMCRYPT_MLDSA_VECTOR          pvDst );
//
// pvDst = pvSrc. Vectors must be the same size.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorSetZero(
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvDst );
//
// Sets all elements of the vector to zero.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorAdd(
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc1,
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc2,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvDst );
//
//  pvDst = pvSrc1 + pvSrc2
//
// Requirements: pvSrc1, pvSrc2, and pvDst must all have the same number of elements.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorSub(
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc1,
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc2,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvDst );
//
//  pvDst = pvSrc1 - pvSrc2
//
// Requirements: pvSrc1, pvSrc2, and pvDst must all have the same number of elements.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorPolyElementMontMul(
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc1,
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc2,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvDst );
//
// ML-DSA Vector-PolyElement Montgomery Multiplication:
//    pvDst[i] = (pvSrc1[i] * peSrc2) ./ R
//
// where:
// * is polynomial multiplication given sources in NTT form
// ./ is coefficient-wise division and R is Montgomery multiplier
//
// Requirements:
//  - peSrc2, and all elements of pvSrc1 must be in ML-DSA NTT form
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorNTT(
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvSrc );
//
// ML-DSA Vector NTT:
//     pvSrc[i] = NTT(pvSrc[i]) for each element in pvSrc
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorINTT(
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvSrc );
//
// ML-DSA Vector inverse NTT:
//     pvSrc[i] = INTT(pvSrc[i]) for each element in pvSrc
//


//////////////////////////////////////////////////////////////////////////
// Matrix operations
//////////////////////////////////////////////////////////////////////////

PSYMCRYPT_MLDSA_MATRIX
SYMCRYPT_CALL
SymCryptMlDsaMatrixCreate(
    _Out_writes_( cbBuffer )    PBYTE       pbBuffer,
                                UINT32      cbBuffer,
                                UINT8       nRows,
                                UINT8       nCols );
//
// Initializes a matrix of nRows * nCols PolyElements in the given buffer.
// cbBuffer must be equal to SYMCRYPT_INTERNAL_MLDSA_SIZEOF_MATRIX( nRows, nCols ).
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaMatrixVectorMontMul(
    _In_    PCSYMCRYPT_MLDSA_MATRIX         pmSrc1,
    _In_    PCSYMCRYPT_MLDSA_VECTOR         pvSrc2,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR          pvDst,
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peTmp );
//
// ML-DSA Matrix-Vector Montgomery Multiplication:
//     pvDst = (pmSrc1 * pvSrc2) ./ R
//
// where:
// * is matrix-vector multiplication of polynomials in NTT form
// ./ is coefficient-wise division and R is Montgomery multiplier
//


//////////////////////////////////////////////////////////////////////////
// Sampling and rejection
//////////////////////////////////////////////////////////////////////////

VOID
SYMCRYPT_CALL
SymCryptMlDsaRejNttPoly(
    _In_reads_( cbRejNttPolySeed )  PCBYTE                      pbRejNttPolySeed,
                                    SIZE_T                      cbRejNttPolySeed,
    _Inout_                         PSYMCRYPT_MLDSA_POLYELEMENT peDst );
//
// RejNTTPoly from FIPS 204
// Used by SymCryptMlDsaExpandA to generate a polynomials in the public matrix A from the expanded
// public seed. The output polynomial is in NTT form with coefficients modulo Q.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandA(
    _In_reads_( cbPublicSeed )  PCBYTE                      pbPublicSeed,
                                SIZE_T                      cbPublicSeed,
    _Inout_                     PSYMCRYPT_MLDSA_MATRIX      pmA );
//
// ExpandA from FIPS 204
// Expands the public seed into the public matrix A.
// \hat{A}[i, j] = RejNttPoly(seed || j || i) for each index (i, j) in A
//

FORCEINLINE
INT8
SYMCRYPT_CALL
SymCryptMlDsaCoeffFromHalfByte(
    _In_                 PCSYMCRYPT_MLDSA_INTERNAL_PARAMS   pParams,
    _In_range_( 0, 15 )  UINT8                              halfByte );
//
// CoeffFromHalfByte from FIPS 204
// Converts a nibble (range [0, 15]) to a coefficient in the range [-eta, eta]
// If the nibble is outside of the valid private key range ([0, 14] for eta = 2, [0, 8] for eta = 4),
// returns INT8_MIN.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaRejBoundedPoly(
    _In_                                PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_( cbRejBoundedPolySeed )  PCBYTE                              pbRejBoundedPolySeed,
                                        SIZE_T                              cbRejBoundedPolySeed,
    _Inout_                             PSYMCRYPT_MLDSA_POLYELEMENT         peDst );
//
// RejBoundedPoly from FIPS 204
// Used by SymCryptMlDsaExpandS to generate polynomials in the private vectors s1 and s2 from the
// expanded private vector seed. Coefficients in the output polynomial are modulo Q.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandS(
    _In_                                PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_( cbPrivateVectorSeed )   PCBYTE                              pbPrivateVectorSeed,
                                        SIZE_T                              cbPrivateVectorSeed,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvs1,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvs2 );
//
// ExpandS from FIPS 204
// s1 = RejBoundedPoly(seed || i) for each index i in s1 (column vector)
// s2 = RejBoundedPoly(seed || i) for each index i in s2 (row vector)
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaSampleInBall(
    _In_                            PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_( cbCommitmentHash )  PCBYTE                              pbCommitmentHash,
                                    SIZE_T                              cbCommitmentHash,
    _Inout_                         PSYMCRYPT_MLDSA_POLYELEMENT         peChallenge );
//
// SampleInBall from FIPS 204
// Samples a polynomial c in R_q with coefficients in {-1, 0, 1} and Hamming weight tau.
// As with all polynomials, coefficients are represented as unsigned integers modulo Q.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaExpandMask(
    _In_                            PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _Inout_                         PSYMCRYPT_SHAKE256_STATE            pShakeState,
    _In_reads_( cbPrivateRandom )   PCBYTE                              pbPrivateRandom,
                                    SIZE_T                              cbPrivateRandom,
    _In_                            UINT16                              counter,
    _Inout_                         PSYMCRYPT_MLDSA_VECTOR              pvMask );
//
// ExpandMask from FIPS 204
// Samples a polynomial vector y in R^l such that each polynomial y[r] has coefficients between
// (-gamma_1 + 1, gamma_1) modulo Q, where gamma_1 == 2^(maskCoefficientRangeLog2) . The output
// vector is returned in NTT form.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaMakeHint(
    _In_        PCSYMCRYPT_MLDSA_INTERNAL_PARAMS                pParams,
    _Inout_     PSYMCRYPT_MLDSA_VECTOR                          pvWMinusCs2,
    _Inout_     PSYMCRYPT_MLDSA_VECTOR                          pvWMinusCs2PlusCt0,
    _Inout_     PSYMCRYPT_MLDSA_VECTOR                          pvDst,
    _Out_       UINT32*                                         nBitsSet );
//
// MakeHint from FIPS 204
// Computes the hint vector. Each coefficient of the polynomials in the vector is a single bit
// indicating whether adding ct0 to (w - cs2) alters the high bits of the corresponding coefficient.
// We define our inputs differently from FIPS 204 to reduce computations:
//
// In FIPS 204, MakeHint is defined as:
// [[r1 != v1]] where r1 = HighBits(r), v1 = HighBits(r + z)
//
// ML-DSA.Sign_internal calls MakeHint with inputs:
// z = -ct0, r = w - cs2 + ct0
//
// We can simplify this to:
// r1 = HighBits(w - cs2 + ct0), v1 = HighBits(w - cs2)
//
// Note that this function modifies the inputs in place for efficiency.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaUseHint(
    _In_        PCSYMCRYPT_MLDSA_INTERNAL_PARAMS                pParams,
    _In_        PCSYMCRYPT_MLDSA_VECTOR                         pvHint,
    _Inout_     PSYMCRYPT_MLDSA_VECTOR                          pvCommitment );
//
// UseHint from FIPS 204
// Uses the hint vector to recalculate the original commitment vector from the approximated
// commitment vector by setting the high bits of the coefficients that were dropped in the
// approximation. On input, pvCommitment is the approximated commitment vector. On output, it is
// the recalculated original commitment vector.
//
// TODO osgvsowi/55435592 Consider decoding the hint just-in-time to avoid allocating an
// entire vector for it
//

//////////////////////////////////////////////////////////////////////////
// Encoding/decoding
//////////////////////////////////////////////////////////////////////////

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementEncode(
    _In_    PCSYMCRYPT_MLDSA_POLYELEMENT    peSrc,
            UINT32                          nBitsPerCoefficient,
            UINT32                          signedCoefficientBound,
    _Out_writes_( nBitsPerCoefficient * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) )
            PBYTE                           pbDst );
//
// Encode a polynomial with coefficients in the range [0, 2^nBitsPerCoefficient] into a tightly
// packed byte array.
//
// Signed coefficients are encoded as described in the comment for
// SYMCRYPT_INTERNAL_MLDSA_SHORT_COEFFICIENT_ENCODE_DECODE. For these coefficients, the
// signedCoefficientBound parameter indicates the upper bound of the coefficients when they are
// positive, and is used to convert them from their internal representation modulo Q to the
// encoded representation.
//
// For polynomials whose coefficients are always positive and do not need any special encoding
// (e.g. t1), signedCoefficientBound must be 0.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPolyElementDecode(
    _In_reads_bytes_( nBitsPerCoefficient * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) )
            PCBYTE                          pbSrc,
            UINT32                          nBitsPerCoefficient,
            UINT32                          signedCoefficientBound,
    _Inout_ PSYMCRYPT_MLDSA_POLYELEMENT     peDst );
//
// From a byte array that was previously encoded as described in SymCryptMlDsaPolyElementEncode,
// decode a polynomial with coefficients in the range [0, 2^nBitsPerCoefficient].
//
// See comments on SymCryptMlDsaPolyElementEncode for information about how coefficients are
// encoded and decoded.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorEncode(
    _In_    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
            UINT32                              nBitsPerCoefficient,
            UINT32                              signedCoefficientBound,
    _Out_writes_( pvSrc->nElems * nBitsPerCoefficient * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) )
            PBYTE                               pbDst );
//
// Encodes a vector of polynomials into a tightly packed byte array.
// pbDst := SymCryptMlDsaPolyElementEncode(i) for each polynomial i in pvSrc
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaVectorDecode(
    _In_reads_bytes_( pvDst->nElems * nBitsPerCoefficient * (SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) )
            PCBYTE                              pbSrc,
            UINT32                              nBitsPerCoefficient,
            UINT32                              signedCoefficientBound,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR              pvDst );
//
// Decodes a vector of encoded polynomials from a byte array.
// pvDst[i] := SymCryptMlDsaPolyElementDecode(i) for each encoded polynomial i in pbSrc
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPkEncode(
    _In_                    PCSYMCRYPT_MLDSAKEY pkMlDsakey,
    _Out_writes_( cbDst )   PBYTE               pbDst,
                            SIZE_T              cbDst );
//
// pkEncode(key) = rho || SimpleBitPack(t1)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaPkDecode(
    _In_reads_( cbSrc ) PCBYTE                  pbSrc,
                        SIZE_T                  cbSrc,
                        UINT32                  flags,
    _Inout_             PSYMCRYPT_MLDSAKEY      pkMlDsakey );
//
// Decodes a public key from a byte array. The encoded public key only contains rho and t1.
// We recalculate the A matrix from rho.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSkEncode(
    _In_                    PCSYMCRYPT_MLDSAKEY pKey,
    _Out_writes_( cbDst )   PBYTE               pbDst,
                            SIZE_T              cbDst );
//
// skEncode(key) = rho || K || H(pkEncode(key)) || BitPack(s1) || BitPack(s2) || BitPack(t0)
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSkDecode(
    _In_reads_( cbSrc ) PCBYTE                  pbSrc,
                        SIZE_T                  cbSrc,
                        UINT32                  flags,
    _Inout_             PSYMCRYPT_MLDSAKEY      pKey );
//
// Decodes a private key from a byte array. The encoded private key contains rho, K, s1, s2 and t0.
// We recalculate the A matrix from rho, t1 by recalculating A * s1 + s2 = t. This function also
// validates that the recalculated public key hash and t0 match the encoded values. If they do
// not, it returns SYMCRYPT_INVALID_BLOB.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaSigEncode(
    _In_                            PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_( cbCommitmentHash )  PBYTE                               pbCommitmentHash,
                                    SIZE_T                              cbCommitmentHash,
    _In_                            PCSYMCRYPT_MLDSA_VECTOR             pvResponse,
    _In_                            PCSYMCRYPT_MLDSA_VECTOR             pvHint,
    _Out_writes_( cbDst )           PBYTE                               pbDst,
                                    SIZE_T                              cbDst );
//
// SigEncode from FIPS 204
// Encodes a signature into a tightly packed byte array.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaSigDecode(
    _In_                            PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_( cbSig )             PCBYTE                              pbSig,
                                    SIZE_T                              cbSig,
    _Out_writes_( cbCommitmentHash) PBYTE                               pbCommitmentHash,
                                    SIZE_T                              cbCommitmentHash,
    _Inout_                         PSYMCRYPT_MLDSA_VECTOR              pvResponse,
    _Inout_                         PSYMCRYPT_MLDSA_VECTOR              pvHint );
//
// SigDecode from FIPS 204
// Decodes a signature from a tightly packed byte array, producing the commitment hash, response
// vector, and hint vector.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaHintBitPack(
    _In_    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_    PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    _Out_writes_bytes_( pParams->nHintNonZeroCoeffs + pvSrc->nElems )
            PBYTE                               pbDst );
//
// HintBitPack from FIPS 204
// Packs the hint vector into a byte array. The first nHintNonZeroCoeffs bytes are the indices
// of non-zero coefficients in the vector, and the last nElems bytes contain the number of
// non-zero coefficients in polynomials 0..i of the vector.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaHintBitUnpack(
    _In_    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_reads_bytes_( pParams->nHintNonZeroCoeffs + pvDst->nElems )
            PCBYTE                              pbSrc,
    _Inout_ PSYMCRYPT_MLDSA_VECTOR              pvDst );
//
// HintBitUnpack from FIPS 204
// Unpacks the hint vector from a byte array where each byte indicates the index of a non-zero
// coefficient in the corresponding polynomial. See comment on SymCryptMlDsaHintBitPack for more
// details about encoding.
//

//////////////////////////////////////////////////////////////////////////
// Auxiliary functions
//////////////////////////////////////////////////////////////////////////

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlDsaGetInternalParamsFromParams(
            SYMCRYPT_MLDSA_PARAMS               params,
    _Out_   PCSYMCRYPT_MLDSA_INTERNAL_PARAMS*   pInternalParams );
//
// Get the internal parameter structure corresponding to the given parameter set enum.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptHashMlDsaValidateHashAlgAndGetOid(
    _In_    PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
            SYMCRYPT_PQDSA_HASH_ID              hashAlg,
            SIZE_T                              cbHash,
    _Out_   PCSYMCRYPT_OID*                     ppOid );
//
// Validates that the given hash algorithm meets the required collision strength for the ML-DSA
// parameter set, as defined in FIPS 204. Also validates that cbHash matches the expected length
// for the hash algorithm, or for XOFs, is >= the required collision strength.
// See comments on the definition of SymCryptHashMlDsaSign
//

FORCEINLINE
INT32
SYMCRYPT_CALL
SymCryptMlDsaModPlusMinus( UINT32 r, UINT32 modulus );
//
// Helper function which implements the mod+- operation from FIPS 204.
// In FIPS 204, r0 := r mod+- 2^d where mod+- returns the unique element in  (-(2^d/2), 2^d/2]
// which is congruent to r modulo 2^d. Importantly, this means that r0 may be negative.
// To use consistent data structures throughout the our implementation and simplify modular
// arithmetic, we do not use negative numbers. Instead, we always represent negative values as
// UINT32s modulo Q.
//
// Requirements: r < modulus
//

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlDsaPolyElementInfinityNorm( _In_ PCSYMCRYPT_MLDSA_POLYELEMENT peSrc );
//
// Returns the infinity norm of the given polynomial element as defined in FIPS 204.
// The infinity norm is the maximum absolute value of w mod+- Q for each coefficient w in the
// polynomial.
//

UINT32
SYMCRYPT_CALL
SymCryptMlDsaVectorInfinityNorm( _In_ PCSYMCRYPT_MLDSA_VECTOR pvSrc );
//
// Returns the infinity norm of the given vector as defined in FIPS 204.
// = max(InfinityNorm(pvSrc[i])) for each polynomial in pvSrc
//

FORCEINLINE
VOID
SYMCRYPT_CALL
SymCryptMlDsaDecompose(
    _In_                                PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_range_(0, SYMCRYPT_MLDSA_Q - 1) UINT32                              r,
    _Out_opt_                           UINT32                              *puR1,
    _Out_opt_                           UINT32                              *puR0 );
//
// Decompose from FIPS 204
// Decomposes r into (r1, r0) such that r1*2*gamma_2 + r0 is congruent to r modulo q
// See note above in SymCryptMlDsaModPlusMinus for important information about the
// representation of r0.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorHighBits(
    _In_                                PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_                                PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvDst );
//
// HighBits from FIPS 204
// For each coefficent r of each polynomial in pvSrc, the corresponding coefficient in pvDst is
// set to *puR1 from Decompose(r).
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorLowBits(
    _In_                                PCSYMCRYPT_MLDSA_INTERNAL_PARAMS    pParams,
    _In_                                PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvDst );
//
// LowBits from FIPS 204
// For each coefficent r of each polynomial in pvSrc, the corresponding coefficient in pvDst is
// set to *puR0 from Decompose(r).
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPower2Round(
    _In_range_(0, SYMCRYPT_MLDSA_Q - 1) UINT32                              r,
    _Out_                               UINT32                              *puR1,
    _Out_                               UINT32                              *puR0 );
//
// Power2Round from FIPS 204
// Decomposes r into (r1, r0) such that r1*2^d + r0 is congruent to r modulo q
// See note above in SymCryptMlDsaModPlusMinus for important information about the
// representation of r0.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaPolyElementPower2Round(
    _In_                                PCSYMCRYPT_MLDSA_POLYELEMENT        peSrc,
    _Inout_                             PSYMCRYPT_MLDSA_POLYELEMENT         peDst1,
    _Inout_                             PSYMCRYPT_MLDSA_POLYELEMENT         peDst0 );
//
// (peDst1[i], peDst0[i]) = Power2Round(peSrc[i]) for each coefficient in peSrc
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaVectorPower2Round(
    _In_                                PCSYMCRYPT_MLDSA_VECTOR             pvSrc,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvDst1,
    _Inout_                             PSYMCRYPT_MLDSA_VECTOR              pvDst0 );
//
// (pvDst1[i], pvDst0[i]) = Power2Round(pvSrc[i]) for each polynomial in pvSrc
//

FORCEINLINE
UINT32
SYMCRYPT_CALL
SymCryptMlDsaSignedCoefficientModQ( INT32 coefficient );
//
// Maps a signed short coefficient to a residue modulo Q.
//

_Success_( return != NULL )
PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES
SYMCRYPT_CALL
SymCryptMlDsaTemporariesAllocateAndInitialize(
    _In_            PCSYMCRYPT_MLDSA_INTERNAL_PARAMS                pParams,
                    UINT32                                          nRowVectors,
                    UINT32                                          nColVectors,
                    UINT32                                          nPolyElements,
                    UINT32                                          cbScratch );
//
// Allocates and initializes a SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES structure and
// returns a pointer to the caller. Returns NULL if allocation fails.
//

VOID
SYMCRYPT_CALL
SymCryptMlDsaTemporariesFree(
    _In_ _Post_invalid_ PSYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES pTemporaries );
//
// Wipes and frees a SYMCRYPT_MLDSA_INTERNAL_COMPUTATION_TEMPORARIES structure previously allocated
// by SymCryptMlDsaTemporariesAllocateAndInitialize.
//
