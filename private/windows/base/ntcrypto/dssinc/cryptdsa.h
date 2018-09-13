#ifndef __CRYPTDSA_
#define __CRYPTDSA_

#ifdef __cplusplus
extern "C" {
#endif

#define SHA_BITS      160
                            // Number of bits output by SHA
#define SHA_DWORDS      5
                            // Number of DWORDS output by SHA
#define DSA_Q_MINDWORDS 5
                            // Minimum number of DWORDS in q
#define DSA_Q_MAXDWORDS 128
                            // Maximum number of DWORDS in q
#define DSA_P_MINDWORDS 16
                            // Minimum number of DWORDS in p
#define DSA_P_MAXDWORDS 128
                            // Maximum number of DWORDS in p

#define DSA_Q_MAXDIGITS DWORDS_TO_DIGITS(DSA_Q_MAXDWORDS)
#define DSA_P_MAXDIGITS DWORDS_TO_DIGITS(DSA_P_MAXDWORDS)


typedef struct {
                DWORD    nbitp;             // Number of significant bits in p.
                                            // (Multiple of 64,   512 <= nbitp <= 1024)
                DWORD    nbitq;             // Number of significant bits in q.
                                            // Must be exactly 160.
                DWORD    p[DSA_P_MAXDWORDS];// Public prime p, 512-1024 bits
                DWORD    q[DSA_Q_MAXDWORDS];// Public prime q (160 bits, divides p-1)
                DWORD    g[DSA_P_MAXDWORDS];// Public generator g of order q (mod p)
                DWORD    j[DSA_P_MAXDWORDS];// j = (p - 1) / q
                DWORD    y[DSA_P_MAXDWORDS];// Public value g^x (mod p), where x is private
                DWORD    S[SHA_DWORDS];     // 160-bit pattern used to construct q
                DWORD    C;                 // 12-bit value of C used to construct p
               } dsa_public_t;

typedef struct {
                digit_t        qdigit[DSA_Q_MAXDIGITS];
                DWORD          lngq_digits;           // Length of q in digits
                reciprocal_1_t qrecip;                // Information about 1/q
                digit_t        gmodular[DSA_P_MAXDIGITS];
                                                      // g as residue mod p
                digit_t        ymodular[DSA_P_MAXDIGITS];
                                                      // y as residue mod p
                mp_modulus_t   pmodulus;              // Constants mod p
               } dsa_precomputed_t;


typedef struct {
                dsa_public_t      pub;               // Public data
                DWORD             x[DSA_P_MAXDWORDS];// Private exponent x (mod q)
                dsa_precomputed_t precomputed;       // Precomputed public data
               } dsa_private_t;

typedef struct {
                DWORD r[SHA_DWORDS];            // (g^k mod p)       mod q
                DWORD s[SHA_DWORDS];            // (SHA(m) + x*r)/k  mod q
               } dsa_signature_t;

typedef struct {
                VOID *pOffload;            // pointer to expo offload info
                FARPROC pFuncExpoOffload;
                RNGINFO *pRNGInfo;            // pointer to RNG info
               } dsa_other_info;

typedef const dsa_precomputed_t dsa_precomputed_tc;
typedef const dsa_private_t     dsa_private_tc;
typedef const dsa_public_t      dsa_public_tc;
typedef const dsa_signature_t   dsa_signature_tc;

void DSA_gen_x(DWORDC cXDigits,                         // In
               DWORDC cXDwords,                         // In
               digit_t *pMod,                           // In
               dsa_other_info *pOtherInfo,              // In
               DWORD *pdwX,                             // Out
               digit_t *pXDigit);                       // Out

BOOL DSA_gen_x_and_y(BOOL fUseQ,                             // In
                     dsa_other_info *pOtherInfo,             // In
                     dsa_private_t *privkey);                // Out

BOOL DSA_check_g(DWORDC         lngp_digits,                   // In
                 digit_tc       *pGModular,                    // In
                 mp_modulus_t   *pPModulo,                     // In
                 DWORDC         lngq_digits,                   // In
                 digit_tc       *pQDigit);                     // In

BOOL DSA_key_generation(DWORDC         nbitp,                   // In
                        DWORDC         nbitq,                   // In
                        dsa_other_info *pOtherInfo,             // In
                        dsa_private_t  *privkey);               // Out

BOOL DSA_key_import_fillin(dsa_private_t *privkey);				// In, Out

BOOL DSA_precompute_pgy(dsa_public_tc     *pubkey,               // In
                        dsa_precomputed_t *precomputed);         // Out

BOOL DSA_precompute(dsa_public_tc     *pubkey,                    // In
                    dsa_precomputed_t *precomputed,               // Out
                    const BOOL         checkSC);                  // In

BOOL DSA_sign(DWORDC           message_hash[SHA_DWORDS],   /* TBD */
              dsa_private_tc   *privkey,
              dsa_signature_t  *signature,
              dsa_other_info   *pOtherInfo);

BOOL DSA_signature_verification(DWORDC              message_hash[SHA_DWORDS],
                                dsa_public_tc       *pubkey,
                                dsa_precomputed_tc  *precomputed_argument,
                                dsa_signature_tc    *signature,
                                dsa_other_info      *pOtherInfo);

BOOL DSA_parameter_verification(
                                dsa_public_tc       *pPubkey,
                                dsa_precomputed_tc  *pPrecomputed
                                );

BOOL DSA_verify_j(
                  dsa_public_tc       *pPubkey,
                  dsa_precomputed_tc  *pPrecomputed
                  );

#ifdef __cplusplus
}
#endif

#endif __CRYPTDSA_

