#include <math.h>

#ifndef RADIX_BITS              /* If not previously #included */

#define MP_LONGEST_BITS  2048
                        /*
                           Multiple precision moduli can have up to
                           MP_LONGEST_BITS bits, which is
                           MP_LONGEST words.  Some routines allow
                           longer operands.
                        */


                        /*
                           Error messages are not printed in the
                           production version of the code.
                           In the test version, compiled
                           by MSCV with ENABLE_ERROR_MESSAGES
                           listed under PREPROCESSOR DEFINITIONS
                           in the project workspace, they are printed,
                        */

#ifndef PRINT_ERROR_MESSAGES
    #ifdef ENABLE_ERROR_MESSAGES
        #define PRINT_ERROR_MESSAGES 1
    #else
        #define PRINT_ERROR_MESSAGES 0
    #endif
#endif

#if PRINT_ERROR_MESSAGES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#define COMPILER_GCC 1
#define COMPILER_VC  2

#ifndef COMPILER
    #ifdef __GNUC__
        #define COMPILER COMPILER_GCC
                        /* GNU compiler */
    #endif
    #ifdef _MSC_VER
        #define COMPILER COMPILER_VC
    #endif              /* Microsoft Visual C compiler */
#endif

#if !defined(COMPILER) || COMPILER <= 0
    #error -- "Unknown COMPILER"
#endif

#define COMPILER_NAME  ( \
        COMPILER == COMPILER_GCC ? "GCC compiler" \
      : COMPILER == COMPILER_VC  ? "Microsoft Visual C++ compiler" \
      : "Unknown compiler")
/*
        List of architectures on which code has been run.
        The SPARC code was used only during development,
        and is not a deliverable.
*/

#define TARGET_ALPHA 1
#define TARGET_IX86 2
#define TARGET_MIPS 3
#define TARGET_PPC 4
#define TARGET_SPARC 5
#define TARGET_IA64 6

#ifndef TARGET
    #ifdef _M_ALPHA
        #define TARGET TARGET_ALPHA
    #endif
    #ifdef _M_IX86
        #define TARGET TARGET_IX86
    #endif
    #ifdef _M_MRX000
        #define TARGET TARGET_MIPS
    #endif
    #ifdef _M_PPC
        #define TARGET TARGET_PPC
    #endif
    #ifdef __sparc__
        #define TARGET TARGET_SPARC
    #endif
    #ifdef _M_IA64
        #define TARGET TARGET_IA64
    #endif
#endif

#if !defined(TARGET) || TARGET <= 0
    #error -- "Unknown TARGET"
#endif

#define TARGET_NAME ( \
          TARGET == TARGET_ALPHA   ? "DEC Alpha" \
        : TARGET == TARGET_IX86    ? "Intel x86 (x >= 3) and Pentium" \
        : TARGET == TARGET_MIPS    ? "MIPS R2000/R3000" \
        : TARGET == TARGET_PPC     ? "Macintosh PowerPC" \
        : TARGET == TARGET_SPARC   ? "Sun SPARC" \
        : TARGET == TARGET_IA64    ? "Intel IA64" \
        : "Unknown target architecture")

/*
        USEASM_ALPHA, ... specify whether to use assembly language,
        if it has been written for a platform.
*/
#ifndef USEASM
    #if TARGET == TARGET_IX86
        #define USEASM 1
    #elif TARGET == TARGET_ALPHA
        #define USEASM 1
    #else
        #define USEASM 0
    #endif
#endif

#define USEASM_ALPHA    (USEASM && TARGET == TARGET_ALPHA)
#define USEASM_IX86     (USEASM && TARGET == TARGET_IX86)
#define USEASM_MIPS     (USEASM && TARGET == TARGET_MIPS)
#define USEASM_PPC      (USEASM && TARGET == TARGET_PPC)
#define USEASM_SPARC    (USEASM && TARGET == TARGET_SPARC)
#define USEASM_IA64     (USEASM && TARGET == TARGET_IA64)



#if COMPILER == COMPILER_VC
        /*
           Visual C recognizes _inline but not inline.
        */
    #define inline _inline

    #pragma intrinsic(abs, labs, memcpy, memset)
    #pragma warning(disable: 4146 4514)
         /* 4146 -- unary minus operator applied
            to unsigned type, result still unsigned.
            4514 -- unreferenced inline function
          */
#endif
/*
        x86 assembly routines are declared naked,
        so they do their own stack management and
        register saving.

        When using a DLL on Intel platforms, all functions use
        the __stdcall convention, so the assembly routines use it too.
        To ensure they are called with the __stdcall
        conventions always (i.e., even when compiled under Microsoft
        Developer Studio), we put __stdcall explicitly in the prototypes.
*/

#if USEASM_IX86
    #define Naked86 __declspec(naked)
    #define Stdcall86 __stdcall
#else
    #define Naked86
    #define Stdcall86
#endif


#if (TARGET == TARGET_ALPHA) || (TARGET == TARGET_IA64)
    #define RADIX_BITS 64
    #define RADIX_BYTES 8
    typedef unsigned __int64 digit_t;
#else
    #define RADIX_BITS 32
    #define RADIX_BYTES 4
    typedef unsigned __int32 digit_t;
#endif

#define MP_LONGEST (MP_LONGEST_BITS/RADIX_BITS)

#if MP_LONGEST_BITS == RADIX_BITS
    #define LG2_MP_LONGEST 0
#elif MP_LONGEST_BITS == 2*RADIX_BITS
    #define LG2_MP_LONGEST 1
#elif MP_LONGEST_BITS == 4*RADIX_BITS
    #define LG2_MP_LONGEST 2
#elif MP_LONGEST_BITS == 8*RADIX_BITS
    #define LG2_MP_LONGEST 3
#elif MP_LONGEST_BITS == 16*RADIX_BITS
    #define LG2_MP_LONGEST 4
#elif MP_LONGEST_BITS == 32*RADIX_BITS
    #define LG2_MP_LONGEST 5
#elif MP_LONGEST_BITS == 64*RADIX_BITS
    #define LG2_MP_LONGEST 6
#elif MP_LONGEST_BITS == 128*RADIX_BITS
    #define LG2_MP_LONGEST 7
#elif MP_LONGEST_BITS == 256*RADIX_BITS
    #define LG2_MP_LONGEST 8
#else
    #define LG2_MP_LONGEST 0
#endif

#if MP_LONGEST_BITS != RADIX_BITS << LG2_MP_LONGEST
    #error "Unrecognized value of MP_LONGEST_BITS"
#endif


/*
        The letter 'c' following a type name identifies
        a const entity of that type.
*/
typedef const char charc;
typedef const  digit_t  digit_tc;
typedef const int intc;


typedef int BOOL;       /* Same as windef.h */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


#define DIGIT_ZERO ((digit_t)0)
#define DIGIT_ONE  ((digit_t)1)
#define RADIX_HALF (DIGIT_ONE << (RADIX_BITS - 1))
#define RADIXM1 (-DIGIT_ONE)
#define F_RADIX ((double)RADIXM1 + 1.0)

#define HALF_RADIX_BITS (RADIX_BITS/2)
#if (RADIX_BITS != 2*HALF_RADIX_BITS)
    #error -- "RADIX_BITS must be even"
#endif
#define RADIX_HALFMASK_BOTTOM (RADIXM1 >> HALF_RADIX_BITS)

#include "dblint.h"


//          Multiple-precision data is normally represented
//      in radix 2^RADIX_BITS, with RADIX_BITS bits per word.
//      Here ``word'' means type digit_t.  RADIX_BITS
//      is 32 on some architectures (Intel, MIPS, PowerPC)
//      and 64 bits on other architectures (Alpha).

//          Within Windows NT, the data type DWORD predominates.
//      DWORD is a 32-bit unsigned datatype on all platforms
//      (Intel, Alpha, MIPS, PowerPC).  DWORD data can safely be
//      written to disk on one architecture and read back on another,
//      unlike digit_t.


typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef const DWORD DWORDC;


#define DWORD_BITS 32
#define DWORD_LEFT_BIT 0x80000000UL

#if RADIX_BITS % DWORD_BITS != 0
    #error "RADIX_BITS not a multiple of 32"
#endif

#define DWORDS_PER_DIGIT (RADIX_BITS/DWORD_BITS)

//      DWORDS_TO_DIGITS(lng_dwords) computes the number of digit_t
//      elements required to store an array with -lng_dwords- DWORDs.
//      DIGITS_TO_DWORDS converts in the opposite direction.


#define DWORDS_TO_DIGITS(lng_dwords) \
                ( ((lng_dwords) + DWORDS_PER_DIGIT - 1)/DWORDS_PER_DIGIT)

#define DIGITS_TO_DWORDS(lng_digits) ((lng_digits) * DWORDS_PER_DIGIT)



/*
        DOUBLE_SHIFT_LEFT(n1, n0, amt) returns
        n1 shifted left by amt bits,
        with new bits coming in from the top of n0.

        DOUBLE_SHIFT_RIGHT(n1, n0, amt) returns n0 shifted right
        by amt bits, with new bits coming from the bottom of n1.

        The shift counts must satisfy 0 <= amt <= RADIX_BITS - 1.
        The shift by    RADIX_BITS - amt   is done in two stages
        (first by 1, then by RADIX_BITS - 1 - amt),
        to avoid an illegal shift count of RADIX_BITS if amt = 0.

*/

#define DOUBLE_SHIFT_LEFT(n1, n0, amt)  \
        (((n1) << (amt)) | (((n0) >> 1) >> (RADIX_BITS - 1 - (amt))))


#define DOUBLE_SHIFT_RIGHT(n1, n0, amt)  \
        (((n0) >> (amt)) | (((n1) << 1) << (RADIX_BITS - 1 - (amt))))

#define digit_getbit(iword, ibit) (((iword) >> (ibit)) & 1)
#define dword_getbit(iword, ibit) digit_getbit(iword, ibit)
                        /* Extract bit from a word.
                        // 0 <= ibit <= RADIX_BITS - 1.
                        // Rightmost (i.e., least significant) bit is bit 0.
                        */

/*
        Test whether a number is odd or even.
*/
#define IS_EVEN(n) (~(n) & 1)
#define IS_ODD(n) ((n) & 1)

/*
        Maximum and minimum of two arguments
        (no side effects in arguments)
*/

#if 0
    #define MAX _max
    #define MIN _min
#else
    #define MAX(x, y) ((x) > (y) ? (x) : (y))
    #define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

#if 0
/*
        If we are building a DLL, use __declspec before certain variable
        declarations (and out procedure names in a .def file).
        _PM_DLL should be #defined when compiling bignum but not the application.

        If we are building a static library, use normal C declarations.
*/
    #ifdef _PM_DLL
        #define exportable_var __declspec( dllexport )
        #define exportable_var_declaration __declspec (dllexport)
    #else
        #define exportable_var __declspec( dllimport )
    #endif
#else
    #define exportable_var extern
    #define exportable_var_declaration
#endif
#



/*
        Macro to return 3^i (exponentiation), for 0 <= i <= 15.
        Intended for use with constant argument, such as
        in array dimensions.  The POWER3 array should
        be used if the subscript is variable.
*/

#define POWER3CON(i) (   ((i) & 1 ?  3 : 1)  *  ((i) & 2 ?    9 : 1) \
                       * ((i) & 4 ? 81 : 1)  *  ((i) & 8 ? 6561 : 1) )

exportable_var DWORDC POWER3[16];   /* See mpglobals.c */
/*
        kara.c repeatedly replaces an operand by three
        half-length operands and a sign.  The sign has
        type kara_sign_t.  The operands are partitioned
        in half until their size at most VMUL_MAX_LNG_SINGLE,
        and sometimes further (see padinfo_initialization in kara.c)
        This may require up to KARA_MAX_HALVINGS halvings,
        giving 3^KARA_MAX_HALVINGS outputs each with size
        as large as VMUL_MAX_SINGLE words.  The signs
        array has length (3^KARA_MAX_HALVINGS - 1)/2.
*/
#if TARGET == TARGET_ALPHA
    typedef int kara_sign_t;
                        /* Try to avoid char data on Alpha */
#else
    typedef unsigned char kara_sign_t;
                        /* Values SIGN_PLUS, SIGN_MINUS.  See kara.c. */
#endif

typedef const kara_sign_t kara_sign_tc;
#define VMUL_MAX_LNG_SINGLE 12
#define KARA_MAX_HALVINGS (LG2_MP_LONGEST - 2)
#if KARA_MAX_HALVINGS > 15
    #error -- "Extend POWER3CON macro"
#endif
#define KARA_MAX_LNG_DIFS ((MP_LONGEST >> KARA_MAX_HALVINGS) * POWER3CON(KARA_MAX_HALVINGS))
#define KARA_MAX_LNG_SIGNS ((POWER3CON(KARA_MAX_HALVINGS) - 1)/2)
#define MEMORY_BANK_ALLOWANCE 1

typedef struct  {
                  digit_t     difs[KARA_MAX_LNG_DIFS + MEMORY_BANK_ALLOWANCE];
                  kara_sign_t signs[KARA_MAX_LNG_SIGNS];
                } kara_longest_t;   /* For MP_LONGEST or less */
                       /* On the Pentium P5 and P6,
                          the two arguments to vmulnn
                          should lie in different memory banks
                          (i.e., different addresses mod 32 bytes).
                          We make the .difs arrays one digit_t entry
                          larger than essential, in an attempt to reduce
                          data cache conflicts.  Look for the
                          MEMORY_BANK_ALLOWANCE symbol in the source code.
                        */


typedef struct  {
                  digit_t     difs[KARA_MAX_LNG_DIFS/3 + MEMORY_BANK_ALLOWANCE];
                  kara_sign_t signs[KARA_MAX_LNG_SIGNS/3];
                } kara_half_longest_t;  /* For MP_LONGEST/2 or less */

typedef const kara_half_longest_t kara_half_longest_tc;
typedef const kara_longest_t      kara_longest_tc;

typedef struct {      /* Constants relating to padding lengths. */
                  DWORD  length;
                                /* length = length3[0] * 2^nhalving */
                  DWORD  nhalving;
                  DWORD  length3[KARA_MAX_HALVINGS+1];
                                /* length3[0] is 1, 2, 3, or 4 */
                                /* length3[i] is length3[0] * 3^i */
               } padinfo_t;

typedef const padinfo_t padinfo_tc;
#define padinfo_NULL ((padinfo_t*)0)

/*
        The reciprocal_1_t type is used when div21
        or divide or divide_immediate would otherwise
        divide by the same number repeatedly.  See file divide.c.
*/

typedef struct {
                digit_t multiplier;
                int     shiftamt;
               } reciprocal_1_t;

typedef const reciprocal_1_t reciprocal_1_tc;

/*
        mp_modulus_t struct has modulus-dependent constants
        used for fast reduction (typically for a fixed modulus,
        which will be used several times, as in modular exponentiation).
        These constants are initialized by function create_modulus:

        modulus -- Modulus used for computations.  Must be nonzero.

        length  -- Length of the modulus, without leading zeros.
                   Operands to mod_add, mod_mul, mod_sub, ...
                   are assumed to have this length.

        padinfo -- Pointer to a padinfo_t struct.  For fast arithmetic,
                   operands are padded to a length
                   length_padded >= length (see find_padinfo in kara.c).
                   The value of length_padded is stored in padinfo->length.
                   The present implementation requires length_padded be either
                   a power of 2, or 3 times a power of 2.
                   For example, if length = 19, then length_padded = 24,
                   and the operands are treated as 24-word
                   operands for Karatsuba.

        half_padinfo -- Pointer to a padinfo_t struct for length
                   CEIL(length/2).  Used in modular_reduce to
                   use Karatsuba multiplication on half-length operands.
                   We denote half_length_padded = half_padinfo->length.

        reddir  -- Equal to FROM_LEFT if reductions of
                   products are done from the left (traditional
                   division), and to FROM_RIGHT if reductions of
                   products are done from the right (Montgomery reduction).

                   When using FROM_RIGHT, the modulus must be odd.
                   Arguments to mod_mul should be pre-scaled by
                   RADIX^scaling_power (mod modulus).
                   The product will be similarly scaled.

        scaling_power --  Equal to 2*half_length_padded when
                   reddir = FROM_RIGHT.  Undefined
                   if reddir = FROM_LEFT.

        one --     Constant 1 (length length), scaled if reddir = FROM_RIGHT.
                   When reddir = FROM_RIGHT, this is
                   RADIX^scaling_power (mod modulus).

        left_multiplier_first -- The first multiplier when reducing from the
                   left.  Length length.

-RADIX^(length + half_length_padded)/2^(left_reciprocal_1.shiftamt) mod modulus

        left_reciprocal_1 -- Reciprocal of the divisor starting at the
                   leftmost digit (i.e., modulus[length-1]);

        right_reciprocal_1 -- If modulus is odd, this holds
                   1/modulus (mod RADIX), for use in mod_shift.
                   Otherwise the field is zero.

        right_multiplier_second -- If reddir = FROM_RIGHT,
                   then this has 1/modulus mod RADIX^(half_length_padded).

        right_multiplier_first --   -1/RADIX^half_length_padded mod modulus.
                   Equal to

        left_multiplier_second -- Contains the half_length_padded*RADIX_BITS

            (modulus * right_multiplier_second - 1)/RADIX^half_length_padded.
                   most significant bits of (high power of 2)/modulus
                   (excluding the leading -1-).  More precisely, this has

        RADIX^(length + half_length_padded) - 1
 FLOOR( --------------------------------------- ) - RADIX^(half_length_padded)
        modulus * 2^(left_reciprocal_1.shiftamt)


                   See file divide.c for an explanation
                   about how this constant is used to get accurate
                   quotients when dividing from the left.

        left_multiplier_second_over2 -- Left_multiplier_second/2.
*/


typedef enum {FROM_LEFT, FROM_RIGHT} reddir_t;
typedef const reddir_t reddir_tc;

typedef struct {
                  digit_t   modulus[MP_LONGEST];
                  DWORD     length;      /* Length passed to create_modulus */
                  DWORD     scaling_power; /* 2*half_padinfo->length */
                  padinfo_tc *padinfo;   /* Pointer to struct containing
                                            padded length and related info */
                  padinfo_tc *half_padinfo;
                                         /* Padinfo info for CEIL(length/2) */
                  reddir_t  reddir;      /* FROM_LEFT or FROM_RIGHT */
                  reciprocal_1_t  left_reciprocal_1;
                  digit_t  right_reciprocal_1;
                                        /* 1/modulus[0] mod RADIX,
                                           if modulus is odd */

                  kara_half_longest_t modulus_kara2[2];
                                        /*
                                            Copy of modulus.

                                            Lower half_length_padded
                                            and upper
                                            length - half_length_padded
                                            words separately passed
                                            to to_kara.
                                         */
                  kara_half_longest_t left_multiplier_first_kara2[2];
                                /* Remainder when dividing
                                     -RADIX^(length + half_length_padded)
                                   / 2^(left_reciprocal_1.shiftamt)
                                   by modulus.

                                   Lower and upper halvves separately
                                   passed to to_kara.
                                */

                  kara_half_longest_t left_multiplier_second_kara;
                                /* half_length_padded*RADIX_BITS
                                   most significant bits of (left)
                                   reciprocal of modulus,
                                   excluding the leading -1-. */

                  digit_t  left_multiplier_second_over2[MP_LONGEST/2];
                                /* left_multiplier_second/2 */
                  kara_half_longest_t right_multiplier_first_kara2[2];
                                        /*  -1/RADIX^half_length_padded
                                            mod modulus.
                                        */
                  digit_t             right_multiplier_second[MP_LONGEST/2];
                  kara_half_longest_t right_multiplier_second_kara;
                              /* 1/modulus mod RADIX^(half_length_padded) */
                  digit_t  cofactor[MP_LONGEST];
                  DWORD    lng_cofactor;
                             /*
                                In factorization programs, this
                                holds the cofactor after dividing
                                modulus by any factors found.
                                Used by gcdex_jacobi.
                             */
                  digit_t  one[MP_LONGEST];
                } mp_modulus_t;


typedef const mp_modulus_t mp_modulus_tc;
/*
       The modular multiplication code and its
       relatives (e.g., modular_reduce, to_kara)
       need large amounts of temporary space
       during processing.  All big temporaries
       are gathered into a modmultemp_t struct.
       Users of these routines can allocate the
       storage themselves, and pass a pointer
       to the temporary storage (fastest), or can pass
       a null pointer (modmultemp_NULL).

*/
typedef struct {
                   // mmul fields are for mod_mul,
                   // mod_mul_kara, mod_mul_kara1

        digit_t             mmul_adifs[KARA_MAX_LNG_DIFS];
        kara_sign_t         mmul_asigns[KARA_MAX_LNG_SIGNS];
        digit_t             mmul_bdifs[KARA_MAX_LNG_DIFS
                                       + MEMORY_BANK_ALLOWANCE];
        kara_sign_t         mmul_bsigns[KARA_MAX_LNG_SIGNS];

                   // mr_ fields are for modular_reduce.
                   // The input to modular_reduce can be stored
                   // in mr_dividend -- this will save a mp_copy call.

        digit_t             mr_dividend[MAX(2*MP_LONGEST,
                                            2*KARA_MAX_LNG_DIFS+1)];

        digit_t             mr_prd1[2*MP_LONGEST];
        digit_t             mr_prd2[2*MP_LONGEST];
        digit_t             mr_mptemp[2*MP_LONGEST];

                   // htk_ fields are for half_times_kara
                   // and half_times_kara2

        digit_t             htk_abprd[2][2*KARA_MAX_LNG_DIFS/3];
        kara_half_longest_t htk_ak;
        } modmultemp_t;

typedef struct {
        void             *pInfo;
        void             *pFuncRNG;
        } RNGINFO;

/*
        When an error is detected, variable mp_errno is set
        to the error number and execution continues.
        If the library was compiled with #define PRINT_ERROR_MESSAGES,
        then a message is written to file mp_errfil.

        The application program should occasionally check mp_errno.

        Except for MP_ERRNO_NO_ERROR, the error numbers are
        in alphabetical order by name.  The routine issuing
        each error number is part of the name.
*/

typedef enum {
        MP_ERRNO_NO_ERROR = 0,
        MP_ERRNO_CREATE_MODULUS_LEADING_ZERO,
        MP_ERRNO_CREATE_MODULUS_MONTGOMERY_EVEN,
        MP_ERRNO_CREATE_MODULUS_TOO_LONG,
        MP_ERRNO_DIGIT_JACOBI_EVEN_DENOMINATOR,
        MP_ERRNO_DIGIT_MOD_DIVIDE_ODD_EVEN_MODULUS,
        MP_ERRNO_DIGIT_MOD_DIVIDE_ODD_NONTRIVIAL_GCD,
        MP_ERRNO_DIGIT_MOD_DIVIDE_ODD_ZERO_DENOMINATOR,
        MP_ERRNO_DIGIT_NEXT_PRIME_TOO_HIGH,
        MP_ERRNO_DIV21_INVALID_ARGUMENT,
        MP_ERRNO_DIVIDE_ESTIMATION_ERROR,
        MP_ERRNO_DIVIDE_INVALID_LENGTHS,
        MP_ERRNO_DIVIDE_LEADING_ZERO,
        MP_ERRNO_DSA_KEY_GENERATION_INVALID_SIZES,
        MP_ERRNO_DSA_PRECOMPUTE_BAD_G,
        MP_ERRNO_DSA_PRECOMPUTE_INVALID_KEY,
        MP_ERRNO_DSA_PRECOMPUTE_PQ_NONPRIME,
        MP_ERRNO_DSA_PRECOMPUTE_WRONG_SC,
        MP_ERRNO_DSA_SIGNATURE_VERIFICATION_NONTRIVIAL_GCD,
        MP_ERRNO_FIND_BIG_PRIME_BAD_CONGRUENCE_CLASS,
        MP_ERRNO_FIND_BIG_PRIME_CONG_MOD_TOO_LARGE,
        MP_ERRNO_FIND_BIG_PRIME_CONG_TO_TOO_LARGE,
        MP_ERRNO_GCDEX_JACOBI_EVEN_MODULUS,
        MP_ERRNO_KP_TOO_SHORT,
        MP_ERRNO_KPDIV_ZERO_DENOMINATOR,
        MP_ERRNO_MOD_ADD_CARRY_NONZERO,
        MP_ERRNO_MOD_SHIFT_LEFT_CARRY_NONZERO,
        MP_ERRNO_MOD_SHIFT_RIGHT_CARRY_NONZERO,
        MP_ERRNO_MOD_SHIFT_RIGHT_EVEN,
        MP_ERRNO_MOD_SUB_BORROW_NONZERO,
        MP_ERRNO_MODULAR_REDUCE_BOTTOM_BITS_DIFFERENT,
        MP_ERRNO_MODULAR_REDUCE_TOO_LONG,
        MP_ERRNO_MODULAR_REDUCE_UNEXPECTED_CARRY,
        MP_ERRNO_MP_DECIMAL_INPUT_NONDIGIT,
        MP_ERRNO_MP_DECIMAL_INPUT_OVERFLOW,
        MP_ERRNO_MP_GCD_INTERMEDIATE_EVEN,
        MP_ERRNO_MP_GCD_TOO_LONG,
        MP_ERRNO_MP_GCDEX_INTERNAL_ERROR,
        MP_ERRNO_MP_GCDEX_NONZERO_REMAINDER,
        MP_ERRNO_MP_GCDEX_ZERO_OPERAND,
        MP_ERRNO_MP_SHIFT_INVALID_SHIFT_COUNT,
        MP_ERRNO_MP_TRAILING_ZERO_COUNT_ZERO_ARG,
        MP_ERRNO_MULTIPLY_LOW_INVALID_LENGTH,
        MP_ERRNO_NO_MEMORY,      // From mp_alloc_temp
        MP_ERRNO_PADINFO_INITIALIZATION_BAD_CUTOFF,
        MP_ERRNO_RANDOM_DIGIT_INTERVAL_INVALID_PARAMETERS,
        MP_ERRNO_RANDOM_MOD_INVALID_PARAMETERS,
        MP_ERRNO_RANDOM_MOD_INVERSE_NOT_PRIME,
        MP_ERRNO_RANDOM_MOD_NONZERO_INVALID_PARAMETERS,
        MP_ERRNO_SELECT_A0B0_BAD_COFACTOR,
        MP_ERRNO_SELECT_A0B0_BAD_MU,
        MP_ERRNO_SELECT_A0B0_NON_CONSTANT_QUOTIENT,
        MP_ERRNO_SELECT_A0B0_NONZERO_REMAINDER,
        MP_ERRNO_SELECT_CURVE_BAD_FIELD_TYPE,
        MP_ERRNO_SELECT_D_UNSUCCESSFUL,
        MP_ERRNO_TO_KARA_INVALID_LENGTH,
        MP_ERRNO_TO_KARA2_INVALID_LENGTH,
        MP_ERRNO_COUNT      // Number of entries above
    } mp_errno_t;

exportable_var mp_errno_t mp_errno;
#define inadequate_memory (mp_errno == MP_ERRNO_NO_MEMORY)
extern const char* mp_errno_name(const mp_errno_t);
       // Update table in mperrnam.c when adding new error message


/*
        Some routine allow an argument of digit_NULL or
        reciprocal_1_NULL when the corresponding argument
        is not otherwise used.  For example, the division
        routine allows but does not require a
        reciprocal structure as argument,
        and allows the quotient to be suppressed.
*/

#define digit_NULL ((digit_t*)0)
#define reciprocal_1_NULL ((reciprocal_1_t*)0)
#define modmultemp_NULL ((modmultemp_t*)0)

/*
        The next several #defines are used in function prototypes.
*/

#define MP_INPUT      digit_tc[]
#define MP_OUTPUT     digit_t[]
#define MP_MODIFIED   digit_t[]
#define DIFS_INPUT    MP_INPUT
#define DIFS_OUTPUT   MP_OUTPUT
#define DIFS_MODIFIED MP_MODIFIED
#define SIGNS_INPUT   kara_sign_tc[]
#define SIGNS_MODIFIED kara_sign_t[]
#define SIGNS_OUTPUT  kara_sign_t[]

extern digit_t accumulate(MP_INPUT, digit_tc, MP_MODIFIED, DWORDC);

extern digit_t Stdcall86 add_diff(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern DWORD add_full(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern digit_t Stdcall86 add_same(MP_INPUT, MP_INPUT, MP_OUTPUT, DWORDC);

extern DWORD add_signed(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern int compare_diff(MP_INPUT, DWORDC, MP_INPUT, DWORDC);

extern int compare_sum_diff(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_INPUT, DWORDC);

BOOL create_modulus(MP_INPUT, DWORDC, reddir_tc, mp_modulus_t*);

extern dblint_t dblint_gcd(dblint_tc, dblint_tc);

extern dblint_t dblint_ogcd(dblint_tc, dblint_tc);

extern digit_t dblint_sqrt(dblint_tc);

extern digit_t decumulate(MP_INPUT, digit_tc, MP_MODIFIED, DWORDC);

extern DWORD digit_factor(digit_tc, digit_t[], DWORD[]);

extern digit_t digit_gcd(digit_tc, digit_tc);

extern int digit_jacobi(digit_tc, digit_tc);

extern digit_t digit_least_prime_divisor(digit_tc);

extern digit_t digit_mod_divide_odd(digit_tc, digit_tc, digit_tc);

extern digit_t digit_ogcd(digit_tc, digit_tc);

extern char* digit_out(digit_tc);

extern digit_t digit_sqrt(digit_tc);

extern void div21(dblint_tc, digit_tc, digit_t*, digit_t*);

extern void div21_fast(dblint_tc, digit_tc,
                       reciprocal_1_tc*, digit_t*, digit_t*);

extern DWORD divide(MP_INPUT, DWORDC, MP_INPUT, DWORDC,
                       reciprocal_1_tc*, MP_OUTPUT, MP_OUTPUT);

extern DWORD divide_rounded(MP_INPUT, DWORDC, MP_INPUT, DWORDC,
                       reciprocal_1_tc*, MP_OUTPUT, MP_OUTPUT);

extern void divide_precondition_1(MP_INPUT, DWORDC, reciprocal_1_t*);

extern digit_t divide_immediate(MP_INPUT, digit_tc,
                         reciprocal_1_tc*, MP_OUTPUT, DWORDC);

extern digit_t estimated_quotient_1(digit_tc, digit_tc,
                                    digit_tc, reciprocal_1_tc*);

extern BOOL find_big_prime(DWORDC, MP_INPUT, DWORDC,
                           MP_INPUT, DWORDC, MP_OUTPUT);

extern padinfo_tc *find_padinfo(DWORDC);

DWORD from_modular(MP_INPUT, MP_OUTPUT, mp_modulus_tc*);

extern int gcdex_jacobi(MP_INPUT, mp_modulus_tc*, MP_OUTPUT, MP_OUTPUT);

extern void mod_add(MP_INPUT, MP_INPUT, MP_OUTPUT, mp_modulus_tc*);

DWORD mod_exp(MP_INPUT, MP_INPUT, DWORDC, MP_OUTPUT,
                        mp_modulus_tc*);

extern DWORD mod_exp_immediate(MP_INPUT, digit_tc, MP_OUTPUT,
                                  mp_modulus_tc*);

extern int mod_jacobi_immediate(const signed long, mp_modulus_tc*);

extern void mod_Lucas(MP_INPUT, MP_INPUT, DWORDC, MP_OUTPUT,
                      mp_modulus_tc*);

extern void mod_LucasUV(MP_INPUT, MP_INPUT, MP_INPUT, DWORDC,
                        MP_OUTPUT, MP_OUTPUT, mp_modulus_tc*);

extern void mod_mul(MP_INPUT, MP_INPUT, MP_OUTPUT,
                    mp_modulus_tc*, modmultemp_t*);

extern void mod_mul_immediate(MP_INPUT, digit_tc,
                              MP_OUTPUT, mp_modulus_tc*);

extern void mod_mul_kara1(MP_INPUT, DIFS_INPUT, SIGNS_INPUT,
                          MP_OUTPUT, mp_modulus_tc*, modmultemp_t*);

extern void mod_mul_kara(DIFS_INPUT, SIGNS_INPUT,
                         DIFS_INPUT, SIGNS_INPUT,
                         MP_OUTPUT, mp_modulus_tc*, modmultemp_t*);

extern void mod_negate(MP_INPUT, MP_OUTPUT, mp_modulus_tc*);

extern void mod_shift(MP_INPUT, intc, MP_OUTPUT, mp_modulus_tc*);

extern BOOL mod_sqrt(MP_INPUT, MP_OUTPUT, mp_modulus_tc*);

extern void mod_sub(MP_INPUT, MP_INPUT, MP_OUTPUT, mp_modulus_tc*);

extern BOOL modular_reduce(MP_INPUT, DWORDC, reddir_tc,
                           MP_OUTPUT, mp_modulus_tc*, modmultemp_t*);

extern void* mp_alloc_temp(DWORDC);
#define Allocate_Temporaries(typename, ptr) \
               ptr = (typename*)mp_alloc_temp(sizeof(typename))

#define Allocate_Temporaries_Multiple(nelmt, typename, ptr) \
               ptr = (typename*)mp_alloc_temp((nelmt)*sizeof(typename))


#if USEASM_ALPHA
extern void mp_copy(MP_INPUT, MP_OUTPUT, DWORDC);
#else
#define mp_copy(src, dest, lng) \
            memcpy((void *)(dest), (const void *)(src), (lng)*sizeof(digit_t))
#endif

extern char* mp_decimal(MP_INPUT, DWORDC);

extern long mp_decimal_input(charc*, MP_OUTPUT, DWORDC, charc**);

extern char* mp_dword_decimal(DWORDC*, DWORDC);

extern int mp_format(MP_MODIFIED, DWORDC,
                     digit_tc, charc*, char*, DWORDC);

extern void mp_free_temp(void*);
#define Free_Temporaries(ptr)    mp_free_temp((void*)ptr)

extern DWORD mp_gcd(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern DWORD mp_gcdex(MP_INPUT, DWORDC, MP_INPUT, DWORDC,
                      MP_OUTPUT, MP_OUTPUT, MP_OUTPUT, MP_OUTPUT);

extern void mp_initialization(void);

extern void mp_longshift(MP_INPUT, intc, MP_OUTPUT, DWORDC);

extern digit_t mp_shift(MP_INPUT, intc, MP_OUTPUT, DWORDC);

extern DWORD mp_significant_bit_count(MP_INPUT, DWORDC);

extern BOOL mp_sqrt(MP_INPUT, MP_OUTPUT, DWORDC);

extern DWORD mp_trailing_zero_count(MP_INPUT, DWORDC);

extern void mul_kara(DIFS_INPUT, SIGNS_INPUT,
                     DIFS_INPUT, SIGNS_INPUT,
                     MP_OUTPUT,  padinfo_tc*);

extern void mul_kara_know_low(DIFS_INPUT, SIGNS_INPUT,
                              DIFS_INPUT, SIGNS_INPUT,
                              MP_INPUT, MP_OUTPUT,
                              padinfo_tc*);

extern void mul_kara_squaring(MP_INPUT, DWORDC,
                              DIFS_MODIFIED, SIGNS_MODIFIED,
                              MP_OUTPUT, padinfo_tc*,
                              modmultemp_t*);

extern void multiply(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern digit_t multiply_immediate(MP_INPUT, digit_tc, MP_OUTPUT, DWORDC);

extern void Stdcall86 multiply_low(MP_INPUT, MP_INPUT, MP_OUTPUT, DWORDC);

extern DWORD multiply_signed(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern DWORD multiply_signed_immediate(MP_INPUT, DWORDC,
                                       signed long, MP_OUTPUT);

#define PRIME_SIEVE_LENGTH 3000
#if (PRIME_SIEVE_LENGTH % 3 != 0)
    #error "PRIME_SIEVE_LENGTH must be a multiple of 3"
#endif

extern digit_t next_prime(
    digit_tc pstart,
    digit_t *lpsievbeg,
    digit_t sieve[PRIME_SIEVE_LENGTH],
    digit_t *lpmax_sieved_squared
    );

extern void padinfo_initialization(DWORDC);

extern BOOL probable_prime(MP_INPUT, DWORDC, MP_INPUT, DWORDC, DWORDC);

extern BOOL remove_small_primes(MP_INPUT, DWORDC, digit_tc,
                                digit_t[], DWORD[], DWORD*,
                                MP_OUTPUT, DWORD*);

#if USEASM_IX86
    #pragma warning(disable : 4035)      /* No return value */
    static inline int significant_bit_count(digit_tc pattern)
    {
    _asm {
            mov  eax,pattern        ; Nonzero pattern
            bsr  eax,eax            ; eax = index of leftmost nonzero bit
                                         ; BSR is slow on Pentium
                                         ; but fast on Pentium Pro
            add  eax,1              ; Add one to get significant bit count
         }
    }
    #pragma warning(default : 4035)
#elif USEASM_ALPHA
    extern const BYTE half_byte_significant_bit_count[128];  /* See mpmisc.c */
            /*
                The Alpha code uses the CMPBGE instruction to
                identify which bytes are nonzero.  The most significant
                bit must occur within the leftmost nonzero byte.
                We use the CMPBGE output to identify which byte that is.
                After we extract that byte, we identify its most significant bit.
            */
    static inline int significant_bit_count(digit_tc pattern)
    {
        intc zero_byte_pattern = __asm("cmpbge  zero, %0, v0", pattern);

        intc byte_offset_plus_1
                = 8*half_byte_significant_bit_count[127 - (zero_byte_pattern >> 1)] + 1;

        return byte_offset_plus_1
                + half_byte_significant_bit_count[pattern >> byte_offset_plus_1];
    }
#else
extern int significant_bit_count(digit_tc);
#endif


extern digit_t Stdcall86 sub_diff(MP_INPUT, DWORDC, MP_INPUT, DWORDC, MP_OUTPUT);

extern digit_t Stdcall86 sub_same(MP_INPUT, MP_INPUT, MP_OUTPUT, DWORDC);

#define sub_signed(a, lnga, b, lngb, c) add_signed(a, lnga, b, -(lngb), c)

BOOL test_primality(MP_INPUT, DWORDC);

BOOL test_primality_check_low(MP_INPUT, DWORDC);

BOOL get_prime(MP_OUTPUT, DWORDC);

BOOL get_generator(DWORD*, DWORD*, DWORDC);

extern void to_kara(MP_INPUT, DWORDC, DIFS_OUTPUT, SIGNS_OUTPUT,
                    padinfo_tc*);

BOOL to_modular(MP_INPUT, DWORDC, MP_OUTPUT, mp_modulus_tc*);

static inline int trailing_zero_count(digit_tc);  /* this file */

typedef void Stdcall86 vmul_t(DIFS_INPUT, DIFS_INPUT, DIFS_OUTPUT, DWORDC);

exportable_var vmul_t *vmulnn[VMUL_MAX_LNG_SINGLE];
                /* Addresses for 1 x 1 to 12 x 12 products */
                /* Defined at end of vmul.c */

#if PRINT_ERROR_MESSAGES
    extern void mp_display(FILE*, charc*, MP_INPUT, DWORDC);
    exportable_var FILE* mp_errfil;       /* Set to stdout in mp_globals */
#endif /* PRINT_ERROR_MESSAGES */


/****************************************************************************/
static inline digit_t add_immediate(digit_tc  a[],
                                    digit_tc iadd,
                                    digit_t   b[],
                                    DWORDC    lng)
/*
        Compute b = a + iadd, where iadd has length 1.
        Both a and b have length lng.
        Function value is carry out of leftmost digit in b.
*/
{
    if (lng == 0) {
        return iadd;
    } else if (a == b && b[0] <= RADIXM1 - iadd) {
        b[0] += iadd;
        return 0;
    } else {
        return add_diff(a, lng, &iadd, 1, b);
    }
}
/***************************************************************************/
static inline int compare_immediate(digit_tc  a[],
                                    digit_tc  ivalue,
                                    DWORDC    lng)
/*
        Compare a multiple-precision number to a scalar.
*/
{
    return compare_diff(a, lng, &ivalue, 1);
}
/****************************************************************************/
static inline int compare_same(digit_tc  a[],
                               digit_tc  b[],
                               DWORDC    lng)
/*
        Compare two multiple precision numbers a and b each of length lng.
        Function value is the sign of a - b, namely

                          +1 if a > b
                           0 if a = b
                          -1 if a < b
*/
#if USEASM_IX86
    #pragma warning(disable : 4035)      /* No return value */
{
                    /*
                            We could use REPE CMPSD,
                            but REPE is slow (4 cycles)
                            on the Pentium.  Plus we
                            would need std and cld
                            to adjust the direction flag.
                            We anticipate that most loops
                            will have either 1 or 2 iterations,
                            and use RISC instructions.
                    */

    _asm {
        mov  eax,lng
        mov  esi,a
        mov  edi,b
     label1:
        test eax,eax
        jz   label2              ; If nothing left, exit with eax = 0

        mov  ecx,[esi+4*eax-4]   ;
        mov  edx,[edi+4*eax-4]

        dec  eax                 ; Decrement remaining loop count
        cmp  ecx,edx             ; Test a[i] - b[i]

        je   label1

        sbb  eax,eax             ; eax = 0 if a > b,   -1 if a < b
        or   eax,1               ; eax = 1 if a > b,   -1 if a < b
     label2:
    }
}
    #pragma warning(default : 4035)
#else
{
    DWORD i;
    for (i = lng-1; i != -1; i--) {
        if (a[i] != b[i]) return (a[i] > b[i] ? +1 : -1);
    }
    return 0;
}  /* compare_same */
#endif
/****************************************************************************/
#if USEASM_ALPHA
    extern void mp_clear(MP_OUTPUT, DWORDC);
#elif 0
static inline void mp_clear(digit_t a[],
                            DWORDC  lnga)
/*
        Zero a multiple-precision number.
*/
{
    DWORD i;
    for (i = 0; i != lnga; i++) a[i] = 0;
}
#else
#define mp_clear(dest, lng) (void)memset((void *)(dest), 0, (lng)*sizeof(digit_t))
#endif
/****************************************************************************/
#if USEASM_ALPHA
    extern void mp_extend(MP_INPUT, DWORDC, MP_OUTPUT, DWORDC);
        // See alpha.s
#else
static inline void mp_extend(digit_tc  a[],
                             DWORDC    lnga,
                             digit_t   b[],
                             DWORDC    lngb)
/*
        Copy a to b, while changing its length from
        lnga to lngb (zero fill).  Require lngb >= lnga.
*/
{
    mp_copy(a, b, lnga);
    mp_clear(b + lnga, lngb - lnga);
}
#endif
/****************************************************************************/
static inline digit_t mp_getbit(digit_tc a[],
                                DWORDC ibit)
                /* Extract bit of multiple precision number */
{
    return digit_getbit(a[ibit/RADIX_BITS],  ibit % RADIX_BITS);
}

/******************************************************************************/
static inline int mp_jacobi_wrt_immediate(digit_tc  numer[],
                                          DWORD     lnumer,
                                          digit_tc  denom)
// Return jacobi(numer, denom), where denom is single precision
{
   digit_tc rem = divide_immediate(numer, denom,
                                   reciprocal_1_NULL,
                                   digit_NULL, lnumer);
   return digit_jacobi(rem, denom);
} /* mp_jacobi_wrt_immediate */
/***************************************************************************/
static inline DWORD mp_remove2(digit_t a[],
                               DWORDC lng)
/*
        mp_remove2 divides -a- by a power of 2 as large as possible
        (i.e., keep dividing by 2 until a is odd).
*/
{
    int nzero;
    digit_tc a0 = a[0];
    if (a0 == 0) {
        nzero = mp_trailing_zero_count(a, lng);
        mp_longshift(a, -nzero, a, lng);
    } else {
        nzero = trailing_zero_count(a0);
        if (nzero != 0) {
            DWORD i;
            digit_t carried_bits = DIGIT_ZERO;
            for (i = lng-1; i != -1; i--) {
                digit_tc anew = (a[i] >> nzero) | carried_bits;
                carried_bits = a[i] << (RADIX_BITS - nzero);
                a[i] = anew;
            }
        }
    }
    return (DWORD)nzero;
}  /* mp_remove2 */
/****************************************************************************/
static inline void mp_setbit(digit_t   a[],
                             DWORDC    ibit,
                             digit_tc  new_value)
/*
        Set a bit to 0 or 1,
        when the number is viewed as a bit array.
*/

{
    DWORDC j       = ibit / RADIX_BITS;
    DWORDC ishift  = ibit % RADIX_BITS;

    digit_tc mask1 = (DIGIT_ONE &  new_value) << ishift;
    digit_tc mask2 = (DIGIT_ONE & ~new_value) << ishift;

    a[j] = (a[j] & ~mask2) | mask1;
}
/****************************************************************************/
#if MEMORY_BANK_ALLOWANCE == 0
#define Preferred_Memory_Bank(new_array, old_array) new_array
#else
static inline digit_t* Preferred_Memory_Bank(digit_t  *new_array,
                                             digit_tc *old_array)
/*
        To avoid memory bank conflicts, it is desirable
        that (input) arguments to vmulxx assembly routines start
        on distinct memory banks, when not doing a squaring.
        If MEMORY_BANK_ALLOWANCE > 0,
        then new_array should have MEMORY_BANK_ALLOWANCE
        extra entries at the end.  We return either
        new_array or new_array + 1, whichever ensures the
        addresses are distinct.

        CAUTION -- This routine does non-portable pointer manipulations.
*/
{
    return new_array + (1 & ~(old_array - new_array));
}
#endif
/****************************************************************************/
static inline void set_immediate(digit_t  a[],
                                 digit_tc ivalue,
                                 DWORDC   lnga)
{
   a[0] = ivalue;
   mp_clear(a + 1, lnga - 1);
}
/****************************************************************************/
static inline DWORD set_immediate_signed(digit_t     a[],
                                         signed long ivalue)
{
    a[0] = labs(ivalue);
    return (ivalue > 0) - (ivalue < 0);     /* Sign of result  -- -1, 0, +1  */
}
/****************************************************************************/
static inline DWORD significant_digit_count(digit_tc  a[],
                                            DWORDC    lng)
/*
        Return the number of significant digits in a.
        Function value is zero precisely when a == 0.
*/
#if USEASM_IX86
    #pragma warning(disable : 4035)      /* No return value */
{
                /*
                   We could use REPE SCASD,
                   but the REPE overhead is
                   four cycles/compare on the Pentium.
                   We would also need sld and cld.
                   It is shorter to use RISC instructions.
                   We anticipate that the leading term a[lng-1]
                   will usually be nonzero.
                */

    _asm {
        mov  eax,lng
        mov  edx,a
     label1:
        test eax,eax
        jz   label2             ; If nothing left in number, return 0

        mov  ecx,[edx+4*eax-4]
        dec  eax

        test ecx,ecx            ; Test leading digit
        jz   label1

        inc  eax                ; Nonzero element found; return old eax
     label2:
    }
}
    #pragma warning(default : 4035)
#else
{
    DWORD i = lng;

    while (i != 0 && a[i-1] == 0) i--;
    return i;
}  /* significant_digit_count */
#endif
#define all_zero(a, lng) (significant_digit_count(a, lng) == 0)
/****************************************************************************/
static inline digit_t sub_immediate(digit_tc  a[],
                                    digit_tc  isub,
                                    digit_t   b[],
                                    DWORDC    lng)
/*
        Compute b = a - isub, where isub has length 1.
        Both a and b have length lng.
        Function value is borrow out of leftmost digit in b.
*/
{
    return (lng == 0 ? isub : sub_diff(a, lng, &isub, 1, b));
}
/****************************************************************************/
static inline int trailing_zero_count(digit_tc d)
/*
        Given a nonzero integer d, this routine computes
        the largest integer n such that 2^n divides d.

        If d = 2^n * (2k + 1), then

                        d =     k *2^(n+1) + 2^n
                       -d = (-1-k)*2^(n+1) + 2^n

        The integers k and -1 - k are one's complements of
        each other, so d & (-d) = 2^n.  Once we determine
        2^n from d, we can get n via significant_bit_count.
*/
#if USEASM_IX86
    #pragma warning(disable : 4035)      /* No return value */
{
    _asm {
            mov  eax,d
            bsf  eax,eax            ; eax = index of rightmost nonzero bit
                                    ; BSF is slow on Pentium,
                                    ; but fast on Pentium Pro.
         }

}
    #pragma warning(default : 4035)
#else
{
    return significant_bit_count(d & (-d)) - 1;
}  /* trailing_zero_count */
#endif
/****************************************************************************/
static inline void digits_to_dwords(digit_tc  pdigit[],
                                    DWORD     pdword[],
                                    DWORDC    lng_dwords)
{
#if DWORDS_PER_DIGIT == 1
    mp_copy(pdigit, (digit_t*)pdword, lng_dwords);
#elif DWORDS_PER_DIGIT == 2
    DWORDC lng_half = lng_dwords >> 1;
    DWORD i;

    if (IS_ODD(lng_dwords)) {
        pdword[lng_dwords-1] = (DWORD)pdigit[lng_half];
    }
    for (i = 0; i != lng_half; i++) {
        digit_tc dig = pdigit[i];
        pdword[2*i    ] = (DWORD)dig;
        pdword[2*i + 1] = (DWORD)(dig >> DWORD_BITS);
    }
#else
    #error "Unexpected DWORDS_PER_DIGIT"
#endif
}  /* digits_to_dwords */
/****************************************************************************/
static inline void dwords_to_digits(DWORDC  pdword[],
                                    digit_t pdigit[],
                                    DWORDC  lng_dwords)
{
#if DWORDS_PER_DIGIT == 1
    mp_copy((digit_t*)pdword, pdigit, lng_dwords);
#elif DWORDS_PER_DIGIT == 2
    DWORDC lng_half = lng_dwords >> 1;
    DWORD i;

    if (IS_ODD(lng_dwords)) {
        pdigit[lng_half] = (digit_t)pdword[lng_dwords - 1];  // Zero fill
    }
    for (i = 0; i != lng_half; i++) {
        pdigit[i] =    ((digit_t)pdword[2*i+1] << DWORD_BITS)
                     |  (digit_t)pdword[2*i];
    }
#else
    #error "Unexpected DWORDS_PER_DIGIT"
#endif
}  /* dwords_to_digits */

#endif // RADIX_BITS
