/*
 *  Digital Signature Algorithm (DSA)
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#ifndef MBEDTLS_DSA_H
#define MBEDTLS_DSA_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#include "bignum.h"
#include "md.h"

/*
 * DSA-specific error codes
 */
#define MBEDTLS_ERR_DSA_BAD_INPUT_DATA                    -0x3180  /**< Bad input parameters to function. */
#define MBEDTLS_ERR_DSA_INVALID_KEY                       -0x3200  /**< The private key is not valid. */
#define MBEDTLS_ERR_DSA_VERIFY_FAILED                     -0x3280  /**< The signature is not valid. */
#define MBEDTLS_ERR_DSA_ALLOC_FAILED                      -0x3300  /**< Memory allocation failed. */
#define MBEDTLS_ERR_DSA_RANDOM_FAILED                     -0x3380  /**< Generation of random value, such as k, failed. */
#define MBEDTLS_ERR_DSA_FILE_IO_ERROR                     -0x3400  /**< Read/write of file failed. */
#define MBEDTLS_ERR_DSA_SIG_LEN_MISMATCH                  -0x3480  /**< The signature is valid but its length is not matches to the given length. */

#define MBEDTLS_DSA_ASN_SIGNATURE_MAX_LEN 64

/* Max DSA group size in bytes */
#define MBEDTLS_DSA_MAX_GROUP 64

/* Max DSA modulus size in bytes (the actual DSA size, max 8192 bits) */
#define MBEDTLS_DSA_MAX_MODULUS 1024

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief     DSA context structure
 */
typedef struct mbedtls_dsa_context
{
    size_t len;
    mbedtls_mpi P;           /*!<  prime modulus P              */
    mbedtls_mpi Q;           /*!<  prime sub-modulus Q          */
    mbedtls_mpi G;           /*!<  generator G                  */
    mbedtls_mpi Y;           /*!<  public key Y = G^X mod P     */
    mbedtls_mpi X;           /*!<  private key X                */
}
mbedtls_dsa_context;

/**
 * \brief           This function initializes a DSA context.
 *
 * \param ctx       The DSA context to initialize.
 */
void mbedtls_dsa_init( mbedtls_dsa_context *ctx );

/**
 * \brief           This function frees a DSA context.
 *
 * \param ctx       The DSA context to free.
 */
void mbedtls_dsa_free( mbedtls_dsa_context *ctx );

/**
 * \brief           This function sets the DSA group parameters.
 *
 * \param ctx       The DSA context to store the parameters in.
 * \param P         The prime modulus P.
 * \param Q         The prime sub-modulus Q.
 * \param G         The generator G.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_set_group( mbedtls_dsa_context *ctx, const mbedtls_mpi *P, const mbedtls_mpi *Q, const mbedtls_mpi *G );

/**
 * \brief            This function generates a DSA keypair.
 *
 * \param ctx          The DSA context to store the keypair in.
 * \param group_size   The group size for the keypair.
 * \param modulus_size The modulus size for the keypair.
 * \param f_rng        The RNG function.
 * \param p_rng        The RNG context.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_genkey( mbedtls_dsa_context *ctx, size_t group_size, size_t modulus_size,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );

/**
 * \brief           This function checks if the DSA group parameters are valid.
 *
 * \param P         The prime modulus P.
 * \param Q         The prime sub-modulus Q.
 * \param G         The generator G.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_check_pqg( const mbedtls_mpi *P, const mbedtls_mpi *Q, const mbedtls_mpi *G );

/**
 * \brief           This function checks if the public key is valid.
 *
 * \param ctx       The DSA context.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_check_pubkey( const mbedtls_dsa_context *ctx );

/**
 * \brief           This function checks if the private key is valid.
 *
 * \param ctx       The DSA context.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_check_privkey( const mbedtls_dsa_context *ctx );

/**
 * \brief           This function derived public key from private key.
 *
 * \param ctx       The DSA context.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_pubkey_from_privkey( mbedtls_dsa_context *ctx );

/**
 * \brief           This function computes the DSA signature of a message hash.
 *
 * \param ctx       The DSA context.
 * \param hash      The message hash.
 * \param hlen      The length of the hash.
 * \param sig       The buffer to write the signature to.
 * \param slen      The length of the signature written.
 * \param f_rng     The RNG function.
 * \param p_rng     The RNG context.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_sign( mbedtls_dsa_context *ctx,
                      const unsigned char *hash, size_t hlen,
                      unsigned char *sig, size_t *slen,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng );

/**
 * \brief           This function reads and verifies a DSA signature.
 *
 * \param ctx       The DSA context.
 * \param hash      The message hash.
 * \param hlen      The length of the hash.
 * \param sig       The signature to read and verify.
 * \param slen      The length of the signature.
 *
 * \return          0 if successful, or a specific error code.
 */
int mbedtls_dsa_verify( mbedtls_dsa_context *ctx,
                        const unsigned char *hash, size_t hlen,
                        const unsigned char *sig, size_t slen );

#if defined(MBEDTLS_SELF_TEST)
/**
 * \brief          The DSA checkup routine.
 *
 * \param verbose  Verbosity level.
 *
 * \return         0 if successful, or 1 if the test failed.
 */
int mbedtls_dsa_self_test( int verbose );
#endif

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_DSA_H */