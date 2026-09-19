//
// sc_lib_mlkem.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Internal ML-KEM definitions for the symcrypt library.
// Always intended to be included as part of sc_lib.h
//

//=====================================================
//  ML-KEM internal high level types
//

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEM_POLYELEMENT {
    // PolyElements just store the coefficients without any header.
    UINT16    coeffs[SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS];
} SYMCRYPT_MLKEM_POLYELEMENT;
typedef       SYMCRYPT_MLKEM_POLYELEMENT * PSYMCRYPT_MLKEM_POLYELEMENT;
typedef const SYMCRYPT_MLKEM_POLYELEMENT * PCSYMCRYPT_MLKEM_POLYELEMENT;

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR {
    // PolyElement Accumulators just store the coefficients without any header.
    UINT32    coeffs[SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS];
} SYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR;
typedef SYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR * PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR;

// Currently maximum size of MLKEM matrices is baked in, they are always square and up to 4x4.
#define SYMCRYPT_MLKEM_MATRIX_MAX_NROWS (4)

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEM_VECTOR {
    _Field_range_( 1, SYMCRYPT_MLKEM_MATRIX_MAX_NROWS )
        UINT32                      nRows;
        UINT32                      cbTotalSize;    // Total size of the Vector

    // Followed by:
    // nRows PolyElements
} SYMCRYPT_MLKEM_VECTOR, *PSYMCRYPT_MLKEM_VECTOR;
typedef const SYMCRYPT_MLKEM_VECTOR * PCSYMCRYPT_MLKEM_VECTOR;

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEM_MATRIX {
    _Field_range_( 1, SYMCRYPT_MLKEM_MATRIX_MAX_NROWS )
        UINT32                      nRows;
        UINT32                      cbTotalSize;    // Total size of the Matrix

    // Array of pointers to PolyElements in row-major order
        PSYMCRYPT_MLKEM_POLYELEMENT apPolyElements[SYMCRYPT_MLKEM_MATRIX_MAX_NROWS * SYMCRYPT_MLKEM_MATRIX_MAX_NROWS];
    // Note: the extra indirection is intentional to make transposing the matrix cheap,
    // given that in the MLKEM context the underlying PolyElements are relatively large
    // so we don't want to move them around

    // Followed by:
    // nRows*nRows PolyElements
} SYMCRYPT_MLKEM_MATRIX, *PSYMCRYPT_MLKEM_MATRIX;
typedef const SYMCRYPT_MLKEM_MATRIX * PCSYMCRYPT_MLKEM_MATRIX;

//
// MLKEMKEY type
//

#define SYMCRYPT_MLKEMKEY_MAX_SIZEOF_ENCODED_T (1536)

typedef SYMCRYPT_ALIGN_STRUCT _SYMCRYPT_MLKEM_INTERNAL_PARAMS {
    UINT32  params;         // parameter set of ML-KEM being used, takes a value from SYMCRYPT_MLKEM_PARAMS

    UINT32  cbPolyElement;  // size of one polynomial ring element
    UINT32  cbVector;       // size of one vector
    UINT32  cbMatrix;       // size of one matrix

    UINT8   nRows;          // corresponds to k from FIPS 203; the number of rows and columns in the matrix A,
                            // and the number of rows in column vectors s and t
    UINT8   nEta1;          // corresponds to eta_1 from FIPS 203; number of coinflips used in generating s and e
                            // in keypair generation, and r in encapsulation
    UINT8   nEta2;          // corresponds to eta_2 from FIPS 203; number of coinflips used in generating e_1 and
                            // e_2 in encapsulation
    UINT8   nBitsOfU;       // corresponds to d_u from FIPS 203; number of bits that the coefficients of the polynomial
                            // ring elements of u are compressed to in encapsulation for encoding into ciphertext
    UINT8   nBitsOfV;       // corresponds to d_v from FIPS 203; number of bits that the coefficients of the polynomial
                            // ring element v is compressed to in encapsulation for encoding into ciphertext
} SYMCRYPT_MLKEM_INTERNAL_PARAMS, *PSYMCRYPT_MLKEM_INTERNAL_PARAMS;
typedef const SYMCRYPT_MLKEM_INTERNAL_PARAMS * PCSYMCRYPT_MLKEM_INTERNAL_PARAMS;

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEMKEY {
    UINT32                  fAlgorithmInfo; // Tracks which algorithms the key can be used in
                                            // Also tracks which per-key selftests have been performed on this key
                                            // A bitwise OR of SYMCRYPT_FLAG_KEY_*, SYMCRYPT_FLAG_MLKEMKEY_*, and
                                            // SYMCRYPT_SELFTEST_KEY_* values

    SYMCRYPT_MLKEM_INTERNAL_PARAMS params;

    UINT32                  cbTotalSize;    // Total in-memory size of the ML-KEM key (this header and the following structs)

    BOOLEAN                 hasPrivateSeed; // Set to true if key has the private seed (d)
    BOOLEAN                 hasPrivateKey;  // Set to true if key has the private key (s and z)

    // seeds
    BYTE                    privateSeed[32];    // private seed (d) from which entire private PKE key can be derived
    BYTE                    privateRandom[32];  // private random (z) used in implicit rejection

    BYTE                    publicSeed[32];     // public seed (rho) from which A can be derived

    // A o s + e = t
    PSYMCRYPT_MLKEM_MATRIX  pmAtranspose;   // public matrix in NTT form (derived from publicSeed)
    PSYMCRYPT_MLKEM_VECTOR  pvt;            // public vector in NTT form

    PSYMCRYPT_MLKEM_VECTOR  pvs;            // private vector in NTT form

    // misc fields
    BYTE                    encodedT[SYMCRYPT_MLKEMKEY_MAX_SIZEOF_ENCODED_T]; // byte-encoding of public vector
                                                                              // may only use a prefix of this buffer
    BYTE                    encapsKeyHash[32];  // Precomputed value of hash of ML-KEM's byte-encoding of encapsulation key

    SYMCRYPT_MAGIC_FIELD
    // Followed by:
    // Atranspose
    // t
    // s
} SYMCRYPT_MLKEMKEY;

//=====================================================
//  ML-KEM primitives
//

#define SYMCRYPT_MLKEM_Q                        (3329)

#define SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT              ( sizeof(SYMCRYPT_MLKEM_POLYELEMENT) )
#define SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT_ACCUMULATOR  ( sizeof(SYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR) )
#define SYMCRYPT_INTERNAL_MLKEM_MAXIMUM_VECTOR_SIZE                 ( sizeof(SYMCRYPT_MLKEM_VECTOR) + (SYMCRYPT_MLKEM_MATRIX_MAX_NROWS * SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT) )
#define SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT_OFFSET( _row )       ( sizeof(SYMCRYPT_MLKEM_VECTOR) + ((_row) * SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT) )
#define SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT( _row, _pVector )    (PSYMCRYPT_MLKEM_POLYELEMENT)( (PBYTE)(_pVector) + SYMCRYPT_INTERNAL_MLKEM_VECTOR_ELEMENT_OFFSET(_row) )

#define SYMCRYPT_MLKEM_SIZEOF_MAX_CIPHERTEXT    (1568UL)
#define SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET     (32UL)
#define SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM     (32UL)

typedef SYMCRYPT_ASYM_ALIGN_STRUCT _SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES {
    BYTE abVectorBuffer0[SYMCRYPT_INTERNAL_MLKEM_MAXIMUM_VECTOR_SIZE];
    BYTE abVectorBuffer1[SYMCRYPT_INTERNAL_MLKEM_MAXIMUM_VECTOR_SIZE];
    BYTE abPolyElementBuffer0[SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT];
    BYTE abPolyElementBuffer1[SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT];
    BYTE abPolyElementAccumulatorBuffer[SYMCRYPT_INTERNAL_MLKEM_SIZEOF_POLYRINGELEMENT_ACCUMULATOR];
    union {
        SYMCRYPT_SHAKE128_STATE shake128State;
        SYMCRYPT_SHAKE256_STATE shake256State;
        SYMCRYPT_SHA3_256_STATE sha3_256State;
        SYMCRYPT_SHA3_512_STATE sha3_512State;
    } hashState0;
    union {
        SYMCRYPT_SHAKE128_STATE shake128State;
        SYMCRYPT_SHAKE256_STATE shake256State;
        SYMCRYPT_SHA3_256_STATE sha3_256State;
        SYMCRYPT_SHA3_512_STATE sha3_512State;
    } hashState1;
} SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES;
typedef SYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES * PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES;

// exposed here for KAT testing
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemEncapsulateInternal(
    _In_    PCSYMCRYPT_MLKEMKEY                                 pkMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )
            PBYTE                                               pbAgreedSecret,
            SIZE_T                                              cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )
            PBYTE                                               pbCiphertext,
            SIZE_T                                              cbCiphertext,
    _In_reads_bytes_( SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM )
            PCBYTE                                              pbRandom,
    _Inout_ PSYMCRYPT_MLKEM_INTERNAL_COMPUTATION_TEMPORARIES    pCompTemps );

PSYMCRYPT_MLKEM_POLYELEMENT
SYMCRYPT_CALL
SymCryptMlKemPolyElementCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer );

PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR
SYMCRYPT_CALL
SymCryptMlKemPolyElementAccumulatorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer );

PSYMCRYPT_MLKEM_VECTOR
SYMCRYPT_CALL
SymCryptMlKemVectorCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer,
                                    UINT32  nRows );

PSYMCRYPT_MLKEM_MATRIX
SYMCRYPT_CALL
SymCryptMlKemMatrixCreate(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    UINT32  cbBuffer,
                                    UINT32  nRows );

//
// ML-KEM operations acting on individual polynomial ring elements (PolyElements)
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementMulAndAccumulate(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT            peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT            peSrc2,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paDst );
//
// ML-KEM Polynomial Ring Element multiply and add:
//      paDst = paDst + (peSrc1 o peSrc2)
// where:
//  o is polynomial multiplication given sources in NTT form
//
// Requirements:
//  - peSrc1 and peSrc2 must be PolyElements in ML-KEM's NTT form
//  - paDst must be in NTT form
//

VOID
SYMCRYPT_CALL
SymCryptMlKemMontgomeryReduceAndAddPolyElementAccumulatorToPolyElement(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paSrc,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT             peDst );
//
// Montgomery reduce and add a Polynomial Ring Element Accumulator to a Polynomial Ring
// Element, and wipe the accumulator:
//      peDst = peDst + (paSrc ./ R)
//      paSrc = 0
// where:
//  ./ is coefficient-wise division and R is Montgomery multiplier
//
//  - One of the following conditions must be true:
//      - paSrc to be pre-multiplied coefficient-wise by R for addition with a canonical
//      representation of peDst
//      - peDst must be coefficient-wise multiplied by the same constant factor as the
//      resulting of (paSrc ./ R) for the addition to make sense
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementMulR(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT     peDst );
//
// ML-KEM Polynomial Ring Element multiply each coefficient by Montgomery multiplier R
//      peDst = peSrc .* R
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementAdd(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc2,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT     peDst );
//
// ML-KEM Polynomial Ring Element addition
//      peDst = peSrc1 + peSrc2
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSub(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc1,
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc2,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT     peDst );
//
// ML-KEM Polynomial Ring Element subtract:
//      peDst = peSrc1 - peSrc2
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementNTT(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT peSrc );
//
// ML-KEM Polynomial Ring Element NTT:
//      peSrc = NTT(peSrc) per FIPS 203
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementINTTAndMulR(
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT peSrc );
//
// ML-KEM Polynomial Ring Element INTT:
//      peSrc = NTTinverse(peSrc) .* R
// where .* is coefficient-wise multiplication and R is Montgomery multiplier
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementCompressAndEncode(
    _In_    PCSYMCRYPT_MLKEM_POLYELEMENT    peSrc,
            UINT32                          nBitsPerCoefficient,
    _Out_writes_bytes_(nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8))
            PBYTE                           pbDst );
//
// ML-KEM Polynomial Ring Element Compress and Encode.
//
// Each coefficient in the ring element is Compressed to nBitsPerCoefficient using
// rounding logic specified in FIPS 203, and the coefficients are encoded
// (packed together densely as 256 contiguous bitfields) into the pbDst buffer.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemPolyElementDecodeAndDecompress(
    _In_reads_bytes_(nBitsPerCoefficient*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8))
            PCBYTE                      pbSrc,
            UINT32                      nBitsPerCoefficient,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT peDst );
//
// ML-KEM Polynomial Ring Element Decode and Decompress.
//
// The pbSrc buffer is interpreted as an encoded ring element, with each coefficient
// being represented by nBitsPerCoefficient. The resulting ring element is written to
// peDst.
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSampleNTTFromShake128(
    _Inout_ PSYMCRYPT_SHAKE128_STATE    pState,
    _Out_   PSYMCRYPT_MLKEM_POLYELEMENT peDst );
//
// Generates an ML-KEM Polynomial Ring Element in NTT form by extracting bytes from
// pre-instantiated SHAKE128 state.
//
// NOTE: we pass the SHAKE state to this function because we do not know up front
// how many bytes need to be extracted.
//

VOID
SYMCRYPT_CALL
SymCryptMlKemPolyElementSampleCBDFromBytes(
    _In_reads_bytes_(eta*2*(SYMCRYPT_MLWE_POLYNOMIAL_COEFFICIENTS / 8) + 1)
                    PCBYTE                      pbSrc,
    _In_range_(2,3) UINT32                      eta,
    _Out_           PSYMCRYPT_MLKEM_POLYELEMENT peDst );
//
// Generates an ML-KEM Polynomial Ring Element in centered binomial distribution
// from input byte array.
// Each coefficient is generated using 2*eta bits.
//


//
// ML-KEM operations acting on Linear Algebra objects
//

VOID
SYMCRYPT_CALL
SymCryptMlKemMatrixTranspose(
    _Inout_ PSYMCRYPT_MLKEM_MATRIX  pmSrc );
//
// pmSrc = transpose(pmSrc)
//

VOID
SYMCRYPT_CALL
SymCryptMlKemMatrixVectorMontMulAndAdd(
    _In_    PCSYMCRYPT_MLKEM_MATRIX                 pmSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc2,
    _Inout_ PSYMCRYPT_MLKEM_VECTOR                  pvDst,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp );
//
// pvDst = ((pmSrc1 o pvSrc2) ./ R) + pvDst
//
// Remarks:
//  - paTmp is used internally for temporary storage, it is wiped before and after use
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorMontDotProduct(
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR                 pvSrc2,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT             peDst,
    _Inout_ PSYMCRYPT_MLKEM_POLYELEMENT_ACCUMULATOR paTmp );
//
// peDst = (pvSrc1 o pvSrc2) ./ R
//
// Remarks:
//  - paTmp is used internally for temporary storage, it is wiped before and after use
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorSetZero(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc );
//
// pvSrc = 0
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorMulR(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst );
//
// pvDst = pvSrc .* R
//


VOID
SYMCRYPT_CALL
SymCryptMlKemVectorAdd(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc2,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst );
//
// pvDst = pvSrc1 + pvSrc2
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorSub(
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc1,
    _In_    PCSYMCRYPT_MLKEM_VECTOR pvSrc2,
    _Out_   PSYMCRYPT_MLKEM_VECTOR  pvDst );
//
// pvDst = pvSrc1 - pvSrc2
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorNTT(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc );
//
// pvSrc = NTT(peSrc) per FIPS 203
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorINTTAndMulR(
    _Inout_ PSYMCRYPT_MLKEM_VECTOR  pvSrc );
//
// pvSrc = NTTinverse(pvSrc) .* R
//

VOID
SYMCRYPT_CALL
SymCryptMlKemVectorCompressAndEncode(
    _In_                        PCSYMCRYPT_MLKEM_VECTOR pvSrc,
                                UINT32                  nBitsPerCoefficient,
    _Out_writes_bytes_(cbDst)   PBYTE                   pbDst,
                                SIZE_T                  cbDst );
//
// See ML-KEM Polynomial Ring Element Compress and Encode
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptMlKemVectorDecodeAndDecompress(
    _In_reads_bytes_(cbSrc) PCBYTE                  pbSrc,
                            SIZE_T                  cbSrc,
                            UINT32                  nBitsPerCoefficient,
    _Out_                   PSYMCRYPT_MLKEM_VECTOR  pvDst );
//
// See ML-KEM Polynomial Ring Element Decode and Decompress
//

VOID
SYMCRYPT_CALL
SymCryptMlKemkeyWipePrivateState(
    _Inout_ PSYMCRYPT_MLKEMKEY  pkMlKemkey );
//
// Wipes the ML-KEM key's private state.
//
