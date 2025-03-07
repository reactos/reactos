//
// atoldbl.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _atoldbl and _atoldbl_l functions, which convert a string representation
// of a floating point number into a 10-byte _LDOUBLE object.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal.h>
#include <corecrt_internal_fltintrn.h>
#include <corecrt_internal_strtox.h>
#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>



#define PTR_12(x) ((uint8_t*)(&(x)->ld12))

#define MSB_USHORT  ((uint16_t)     0x8000)
#define MSB_ULONG   ((uint32_t) 0x80000000)

#define TMAX10  5200       /* maximum temporary decimal exponent */
#define TMIN10 -5200       /* minimum temporary decimal exponent */
#define LD_MAX_EXP_LEN   4 /* maximum number of decimal exponent digits */
#define LD_MAX_MAN_LEN  24 /* maximum length of mantissa (decimal)*/
#define LD_MAX_MAN_LEN1 25 /* MAX_MAN_LEN+1 */

#define LD_BIAS   0x3fff  /* exponent bias for long double */
#define LD_BIASM1 0x3ffe  /* LD_BIAS - 1 */
#define LD_MAXEXP 0x7fff  /* maximum biased exponent */

#define D_BIAS   0x3ff  /* exponent bias for double */
#define D_BIASM1 0x3fe  /* D_BIAS - 1 */
#define D_MAXEXP 0x7ff  /* maximum biased exponent */

// Macros for manipulation of a 12-byte long double number (an ordinary 10-byte
// long double plus two extra bytes of mantissa).
// byte layout:
//
//              +-----+--------+--------+-------+
//              |XT(2)|MANLO(4)|MANHI(4)|EXP(2) |
//              +-----+--------+--------+-------+
//              |<-UL_LO->|<-UL_MED->|<-UL_HI ->|
//                  (4)       (4)        (4)
#define ALIGN(x) ((unsigned long _UNALIGNED*)(x))

#define U_EXP_12(p)    ((uint16_t           *)(PTR_12(p) + 10))
#define UL_MANHI_12(p) ((uint32_t _UNALIGNED*)(PTR_12(p) +  6))
#define UL_MANLO_12(p) ((uint32_t _UNALIGNED*)(PTR_12(p) +  2))
#define U_XT_12(p)     ((uint16_t           *)(PTR_12(p)     ))

// Pointers to the four low, mid, and high order bytes of the extended mantissa
#define UL_LO_12(p)  ((uint32_t*)(PTR_12(p)    ))
#define UL_MED_12(p) ((uint32_t*)(PTR_12(p) + 4))
#define UL_HI_12(p)  ((uint32_t*)(PTR_12(p) + 8))

// Pointers to the uint8_t, uint16_t, and uint32_t of order i (LSB = 0; MSB = 9)
#define UCHAR_12(p, i)   ((uint8_t *)(          PTR_12(p) + (i)))
#define USHORT_12(p, i)  ((uint16_t*)((uint8_t*)PTR_12(p) + (i)))
#define ULONG_12(p, i)   ((uint32_t*)((uint8_t*)PTR_12(p) + (i)))
#define TEN_BYTE_PART(p) ((uint8_t *)(          PTR_12(p) +  2 ))

// Manipulation of a 10-byte long double number
#define U_EXP_LD(p)    ((uint16_t*)(_PTR_LD(p) + 8))
#define UL_MANHI_LD(p) ((uint32_t*)(_PTR_LD(p) + 4))
#define UL_MANLO_LD(p) ((uint32_t*)(_PTR_LD(p)    ))

// Manipulation of a 64bit IEEE double
#define U_SHORT4_D(p) ((uint16_t*)(p) + 3)
#define UL_HI_D(p)    ((uint32_t*)(p) + 1)
#define UL_LO_D(p)    ((uint32_t*)(p)    )

#define PUT_INF_12(p, sign)                           \
    *UL_HI_12 (p) = (sign) ? 0xffff8000 : 0x7fff8000; \
    *UL_MED_12(p) = 0;                                \
    *UL_LO_12 (p) = 0;

#define PUT_ZERO_12(p) \
    *UL_HI_12 (p) = 0; \
    *UL_MED_12(p) = 0; \
    *UL_LO_12 (p) = 0;

#define ISZERO_12(p) \
    ((*UL_HI_12 (p) & 0x7fffffff) == 0 && \
      *UL_MED_12(p)               == 0 && \
      *UL_LO_12 (p)               == 0)

#define PUT_INF_LD(p, sign)                     \
    *U_EXP_LD   (p) = (sign) ? 0xffff : 0x7fff; \
    *UL_MANHI_LD(p) = 0x8000;                   \
    *UL_MANLO_LD(p) = 0;

#define PUT_ZERO_LD(p)   \
    *U_EXP_LD   (p) = 0; \
    *UL_MANHI_LD(p) = 0; \
    *UL_MANLO_LD(p) = 0;

#define ISZERO_LD(p)                    \
    ((*U_EXP_LD   (p) & 0x7fff) == 0 && \
      *UL_MANHI_LD(p)           == 0 && \
      *UL_MANLO_LD(p)           == 0)



static _LDBL12 const ld12_pow10_positive[] =
{
    /*P0001*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA0,0x02,0x40}},
    /*P0002*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC8,0x05,0x40}},
    /*P0003*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFA,0x08,0x40}},
    /*P0004*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x9C,0x0C,0x40}},
    /*P0005*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xC3,0x0F,0x40}},
    /*P0006*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x24,0xF4,0x12,0x40}},
    /*P0007*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x80,0x96,0x98,0x16,0x40}},
    /*P0008*/ {{0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x20,0xBC,0xBE,0x19,0x40}},
    /*P0016*/ {{0x00,0x00, 0x00,0x00,0x00,0x04,0xBF,0xC9,0x1B,0x8E,0x34,0x40}},
    /*P0024*/ {{0x00,0x00, 0x00,0xA1,0xED,0xCC,0xCE,0x1B,0xC2,0xD3,0x4E,0x40}},
    /*P0032*/ {{0x20,0xF0, 0x9E,0xB5,0x70,0x2B,0xA8,0xAD,0xC5,0x9D,0x69,0x40}},
    /*P0040*/ {{0xD0,0x5D, 0xFD,0x25,0xE5,0x1A,0x8E,0x4F,0x19,0xEB,0x83,0x40}},
    /*P0048*/ {{0x71,0x96, 0xD7,0x95,0x43,0x0E,0x05,0x8D,0x29,0xAF,0x9E,0x40}},
    /*P0056*/ {{0xF9,0xBF, 0xA0,0x44,0xED,0x81,0x12,0x8F,0x81,0x82,0xB9,0x40}},
    /*P0064*/ {{0xBF,0x3C, 0xD5,0xA6,0xCF,0xFF,0x49,0x1F,0x78,0xC2,0xD3,0x40}},
    /*P0128*/ {{0x6F,0xC6, 0xE0,0x8C,0xE9,0x80,0xC9,0x47,0xBA,0x93,0xA8,0x41}},
    /*P0192*/ {{0xBC,0x85, 0x6B,0x55,0x27,0x39,0x8D,0xF7,0x70,0xE0,0x7C,0x42}},
    /*P0256*/ {{0xBC,0xDD, 0x8E,0xDE,0xF9,0x9D,0xFB,0xEB,0x7E,0xAA,0x51,0x43}},
    /*P0320*/ {{0xA1,0xE6, 0x76,0xE3,0xCC,0xF2,0x29,0x2F,0x84,0x81,0x26,0x44}},
    /*P0384*/ {{0x28,0x10, 0x17,0xAA,0xF8,0xAE,0x10,0xE3,0xC5,0xC4,0xFA,0x44}},
    /*P0448*/ {{0xEB,0xA7, 0xD4,0xF3,0xF7,0xEB,0xE1,0x4A,0x7A,0x95,0xCF,0x45}},
    /*P0512*/ {{0x65,0xCC, 0xC7,0x91,0x0E,0xA6,0xAE,0xA0,0x19,0xE3,0xA3,0x46}},
    /*P1024*/ {{0x0D,0x65, 0x17,0x0C,0x75,0x81,0x86,0x75,0x76,0xC9,0x48,0x4D}},
    /*P1536*/ {{0x58,0x42, 0xE4,0xA7,0x93,0x39,0x3B,0x35,0xB8,0xB2,0xED,0x53}},
    /*P2048*/ {{0x4D,0xA7, 0xE5,0x5D,0x3D,0xC5,0x5D,0x3B,0x8B,0x9E,0x92,0x5A}},
    /*P2560*/ {{0xFF,0x5D, 0xA6,0xF0,0xA1,0x20,0xC0,0x54,0xA5,0x8C,0x37,0x61}},
    /*P3072*/ {{0xD1,0xFD, 0x8B,0x5A,0x8B,0xD8,0x25,0x5D,0x89,0xF9,0xDB,0x67}},
    /*P3584*/ {{0xAA,0x95, 0xF8,0xF3,0x27,0xBF,0xA2,0xC8,0x5D,0xDD,0x80,0x6E}},
    /*P4096*/ {{0x4C,0xC9, 0x9B,0x97,0x20,0x8A,0x02,0x52,0x60,0xC4,0x25,0x75}}
};

static _LDBL12 const ld12_pow10_negative[] =
{
    /*N0001*/ {{0xCD,0xCC, 0xCD,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFB,0x3F}},
    /*N0002*/ {{0x71,0x3D, 0x0A,0xD7,0xA3,0x70,0x3D,0x0A,0xD7,0xA3,0xF8,0x3F}},
    /*N0003*/ {{0x5A,0x64, 0x3B,0xDF,0x4F,0x8D,0x97,0x6E,0x12,0x83,0xF5,0x3F}},
    /*N0004*/ {{0xC3,0xD3, 0x2C,0x65,0x19,0xE2,0x58,0x17,0xB7,0xD1,0xF1,0x3F}},
    /*N0005*/ {{0xD0,0x0F, 0x23,0x84,0x47,0x1B,0x47,0xAC,0xC5,0xA7,0xEE,0x3F}},
    /*N0006*/ {{0x40,0xA6, 0xB6,0x69,0x6C,0xAF,0x05,0xBD,0x37,0x86,0xEB,0x3F}},
    /*N0007*/ {{0x33,0x3D, 0xBC,0x42,0x7A,0xE5,0xD5,0x94,0xBF,0xD6,0xE7,0x3F}},
    /*N0008*/ {{0xC2,0xFD, 0xFD,0xCE,0x61,0x84,0x11,0x77,0xCC,0xAB,0xE4,0x3F}},
    /*N0016*/ {{0x2F,0x4C, 0x5B,0xE1,0x4D,0xC4,0xBE,0x94,0x95,0xE6,0xC9,0x3F}},
    /*N0024*/ {{0x92,0xC4, 0x53,0x3B,0x75,0x44,0xCD,0x14,0xBE,0x9A,0xAF,0x3F}},
    /*N0032*/ {{0xDE,0x67, 0xBA,0x94,0x39,0x45,0xAD,0x1E,0xB1,0xCF,0x94,0x3F}},
    /*N0040*/ {{0x24,0x23, 0xC6,0xE2,0xBC,0xBA,0x3B,0x31,0x61,0x8B,0x7A,0x3F}},
    /*N0048*/ {{0x61,0x55, 0x59,0xC1,0x7E,0xB1,0x53,0x7C,0x12,0xBB,0x5F,0x3F}},
    /*N0056*/ {{0xD7,0xEE, 0x2F,0x8D,0x06,0xBE,0x92,0x85,0x15,0xFB,0x44,0x3F}},
    /*N0064*/ {{0x24,0x3F, 0xA5,0xE9,0x39,0xA5,0x27,0xEA,0x7F,0xA8,0x2A,0x3F}},
    /*N0128*/ {{0x7D,0xAC, 0xA1,0xE4,0xBC,0x64,0x7C,0x46,0xD0,0xDD,0x55,0x3E}},
    /*N0192*/ {{0x63,0x7B, 0x06,0xCC,0x23,0x54,0x77,0x83,0xFF,0x91,0x81,0x3D}},
    /*N0256*/ {{0x91,0xFA, 0x3A,0x19,0x7A,0x63,0x25,0x43,0x31,0xC0,0xAC,0x3C}},
    /*N0320*/ {{0x21,0x89, 0xD1,0x38,0x82,0x47,0x97,0xB8,0x00,0xFD,0xD7,0x3B}},
    /*N0384*/ {{0xDC,0x88, 0x58,0x08,0x1B,0xB1,0xE8,0xE3,0x86,0xA6,0x03,0x3B}},
    /*N0448*/ {{0xC6,0x84, 0x45,0x42,0x07,0xB6,0x99,0x75,0x37,0xDB,0x2E,0x3A}},
    /*N0512*/ {{0x33,0x71, 0x1C,0xD2,0x23,0xDB,0x32,0xEE,0x49,0x90,0x5A,0x39}},
    /*N1024*/ {{0xA6,0x87, 0xBE,0xC0,0x57,0xDA,0xA5,0x82,0xA6,0xA2,0xB5,0x32}},
    /*N1536*/ {{0xE2,0x68, 0xB2,0x11,0xA7,0x52,0x9F,0x44,0x59,0xB7,0x10,0x2C}},
    /*N2048*/ {{0x25,0x49, 0xE4,0x2D,0x36,0x34,0x4F,0x53,0xAE,0xCE,0x6B,0x25}},
    /*N2560*/ {{0x8F,0x59, 0x04,0xA4,0xC0,0xDE,0xC2,0x7D,0xFB,0xE8,0xC6,0x1E}},
    /*N3072*/ {{0x9E,0xE7, 0x88,0x5A,0x57,0x91,0x3C,0xBF,0x50,0x83,0x22,0x18}},
    /*N3584*/ {{0x4E,0x4B, 0x65,0x62,0xFD,0x83,0x8F,0xAF,0x06,0x94,0x7D,0x11}},
    /*N4096*/ {{0xE4,0x2D, 0xDE,0x9F,0xCE,0xD2,0xC8,0x04,0xDD,0xA6,0xD8,0x0A}}
};



// Adds x and y, storing the result in *sum.  Returns true if overflow occurred;
// false otherwise.
static __forceinline bool __cdecl add_uint32_carry(uint32_t const x, uint32_t const y, uint32_t* const sum) throw()
{
    uint32_t const r = x + y;

    *sum = r;

    return r < x || r < y; // carry
}

// Adds *x and *y as 12-byte integers, storing the result in *x.  Overflow is ignored.
static __forceinline void __cdecl add_ld12(_LDBL12* const x, _LDBL12 const* const y) throw()
{
    if (add_uint32_carry(*UL_LO_12(x), *UL_LO_12(y), UL_LO_12(x)))
    {
        if (add_uint32_carry(*UL_MED_12(x), 1, UL_MED_12(x)))
        {
            ++*UL_HI_12(x);
        }
    }

    if (add_uint32_carry(*UL_MED_12(x), *UL_MED_12(y), UL_MED_12(x)))
    {
        ++*UL_HI_12(x);
    }

    // Ignore next carry -- assume no overflow will occur
    add_uint32_carry(*UL_HI_12(x), *UL_HI_12(y), UL_HI_12(x));
}

// Shifts *p N bits to the left.  The number is shifted as a 12-byte integer.
template <uint32_t N>
static __forceinline void __cdecl shl_ld12(_LDBL12* const p) throw()
{
    uint32_t const total_bits{sizeof(uint32_t) * CHAR_BIT};
    uint32_t const msb_bits{N};
    uint32_t const lsb_bits{total_bits - N};

    static_assert(msb_bits <= total_bits, "shift too large");

    uint32_t const lsb_mask{(1 << (lsb_bits - 1)) - 1};
    uint32_t const msb_mask{static_cast<uint32_t>(-1) ^ lsb_mask};

    uint32_t const lo_carry {(*UL_LO_12 (p) & msb_mask) >> lsb_bits};
    uint32_t const med_carry{(*UL_MED_12(p) & msb_mask) >> lsb_bits};

    *UL_LO_12 (p) = (*UL_LO_12 (p) << msb_bits);
    *UL_MED_12(p) = (*UL_MED_12(p) << msb_bits) | lo_carry;
    *UL_HI_12 (p) = (*UL_HI_12 (p) << msb_bits) | med_carry;
}

// Shifts *p one bit to the right.  The number is shifted as a 12-byte integer.
static __forceinline void __cdecl shr_ld12(_LDBL12* const p) throw()
{
    uint32_t const c2 = *UL_HI_12 (p) & 0x1 ? MSB_ULONG : 0;
    uint32_t const c1 = *UL_MED_12(p) & 0x1 ? MSB_ULONG : 0;

    *UL_HI_12 (p) >>= 1;
    *UL_MED_12(p) = *UL_MED_12(p) >> 1 | c2;
    *UL_LO_12 (p) = *UL_LO_12 (p) >> 1 | c1;
}

// Multiplies *px and *py, storing the result in *px.
static __forceinline void __cdecl multiply_ld12(_LDBL12* const px, _LDBL12 const* const py) throw()
{
    _LDBL12 tempman; // This is actually a 12-byte mantissa, not a 12-byte long double
    *UL_LO_12 (&tempman) = 0;
    *UL_MED_12(&tempman) = 0;
    *UL_HI_12 (&tempman) = 0;

    uint16_t expx = *U_EXP_12(px);
    uint16_t expy = *U_EXP_12(py);

    uint16_t const sign = (expx ^ expy) & static_cast<uint16_t>(0x8000);
    expx &= 0x7fff;
    expy &= 0x7fff;
    uint16_t expsum = expx + expy;

    if (expx   >= LD_MAXEXP ||
        expy   >= LD_MAXEXP ||
        expsum >  LD_MAXEXP + LD_BIASM1)
    {
        // Overflow to infinity
        PUT_INF_12(px, sign);
        return;
    }

    if (expsum <= LD_BIASM1 - 63)
    {
        // Underflow to zero
        PUT_ZERO_12(px);
        return;
    }

    if (expx == 0)
    {
        // If this is a denormal temp real then the mantissa was shifted right
        // once to set bit 63 to zero.
        ++expsum; // Correct for this

        if (ISZERO_12(px))
        {
            // Put positive sign:
            *U_EXP_12(px) = 0;
            return;
        }
    }

    if (expy == 0)
    {
        ++expsum; // Because arg2 is denormal
        if (ISZERO_12(py))
        {
            PUT_ZERO_12(px);
            return;
        }
    }

    int roffs = 0;
    for (int i = 0; i < 5; ++i)
    {
        int poffs = i << 1;
        int qoffs = 8;
        for (int j = 5 - i; j > 0; --j)
        {
            bool carry = false;

            uint16_t* const p = USHORT_12(px, poffs);
            uint16_t* const q = USHORT_12(py, qoffs);
            uint32_t* const r = ULONG_12(&tempman, roffs);
            uint32_t  const prod = static_cast<uint32_t>(*p) * static_cast<uint32_t>(*q);

            #if defined _M_X64 || defined _M_ARM
            // handle misalignment problems
            if (i & 0x1) // i is odd
            {
                uint32_t sum = 0;
                carry = add_uint32_carry(*ALIGN(r), prod, &sum);
                *ALIGN(r) = sum;
            }
            else // i is even
            {
                carry = add_uint32_carry(*r, prod, r);
            }
            #else
            carry = add_uint32_carry(*r, prod, r);
            #endif

            if (carry)
            {
                // roffs should be less than 8 in this case
                ++*USHORT_12(&tempman, roffs + 4);
            }

            poffs += 2;
            qoffs -= 2;
        }

        roffs += 2;
    }

    expsum -= LD_BIASM1;

    // Normalize
    while (static_cast<int16_t>(expsum) > 0 && (*UL_HI_12(&tempman) & MSB_ULONG) == 0)
    {
         shl_ld12<1>(&tempman);
         expsum--;
    }

    if (static_cast<int16_t>(expsum) <= 0)
    {
        bool sticky = false;

        expsum--;
        while (static_cast<int16_t>(expsum) < 0)
        {
            if (*U_XT_12(&tempman) & 0x1)
                sticky = true;

            shr_ld12(&tempman);
            expsum++;
        }

        if (sticky)
        {
            *U_XT_12(&tempman) |= 0x1;
        }
    }

    if (*U_XT_12(&tempman) > 0x8000 || (*UL_LO_12(&tempman) & 0x1ffff) == 0x18000)
    {
        // Round up:
        if (*UL_MANLO_12(&tempman) == UINT32_MAX)
        {
            *UL_MANLO_12(&tempman) = 0;

            if (*UL_MANHI_12(&tempman) == UINT32_MAX)
            {
                *UL_MANHI_12(&tempman) = 0;

                if (*U_EXP_12(&tempman) == UINT16_MAX)
                {
                    // 12-byte mantissa overflow:
                    *U_EXP_12(&tempman) = MSB_USHORT;
                    ++expsum;
                }
                else
                {
                    ++*U_EXP_12(&tempman);
                }
            }
            else
            {
                ++*UL_MANHI_12(&tempman);
            }
        }
        else
        {
            ++*UL_MANLO_12(&tempman);
        }
    }


    // Check for exponent overflow:
    if (expsum >= 0x7fff)
    {
        PUT_INF_12(px, sign);
        return;
    }

    // Put result in px:
    *U_XT_12    (px) = *USHORT_12(&tempman, 2);
    *UL_MANLO_12(px) = *UL_MED_12(&tempman);
    *UL_MANHI_12(px) = *UL_HI_12 (&tempman);
    *U_EXP_12   (px) = expsum | sign;
}

// Multiplies *pld12 by 10^pow.
static __forceinline void __cdecl multiply_ten_pow_ld12(_LDBL12* const pld12, int pow) throw()
{
    if (pow == 0)
        return;

    _LDBL12 const* pow_10p = ld12_pow10_positive - 8;
    if (pow < 0)
    {
        pow     = -pow;
        pow_10p = ld12_pow10_negative-8;
    }

    while (pow != 0)
    {
        pow_10p += 7;
        int const last3 = pow & 0x7; // The three least significant bits of pow
        pow >>= 3;

        if (last3 == 0)
            continue;

        _LDBL12 const* py = pow_10p + last3;

        _LDBL12 unround;

        // Do an exact 12byte multiplication:
        if (*U_XT_12(py) >= 0x8000)
        {
            // Copy number:
            unround = *py;

            // Unround adjacent byte:
            --*UL_MANLO_12(&unround);

            // Point to new operand:
            py = &unround;
        }

        multiply_ld12(pld12, py);
    }
}

// Multiplies *ld12 by 2^power.
static __forceinline void __cdecl multiply_two_pow_ld12(_LDBL12* const ld12, int const power) throw()
{
    _LDBL12 multiplicand{};
    *U_XT_12    (&multiplicand) = 0;
    *UL_MANLO_12(&multiplicand) = 0;
    *UL_MANHI_12(&multiplicand) = (1u << (sizeof(uint32_t) * CHAR_BIT - 1));
    *U_EXP_12   (&multiplicand) = static_cast<uint16_t>(power + LD_BIAS);

    multiply_ld12(ld12, &multiplicand);
}


// These multiply a 12-byte integer stored in an _LDBL12 by N.  N must be 10 or 16.
template <uint32_t N>
static __forceinline void __cdecl multiply_ld12_by(_LDBL12*) throw();

template <>
__forceinline void __cdecl multiply_ld12_by<10>(_LDBL12* const ld12) throw()
{
    _LDBL12 const original_ld12 = *ld12;
    shl_ld12<2>(ld12);
    add_ld12(ld12, &original_ld12);
    shl_ld12<1>(ld12);
}

template <>
__forceinline void __cdecl multiply_ld12_by<16>(_LDBL12* const ld12) throw()
{
    shl_ld12<4>(ld12);
}

// Converts a mantissa into an _LDBL12.  The mantissa to be converted must be
// represented as an array of BCD digits, one per byte, read from the byte range
// [mantissa, mantissa + mantissa_count).
template <uint32_t Base>
static __forceinline void __cdecl convert_mantissa_to_ld12(
    uint8_t const* const mantissa,
    size_t         const mantissa_count,
    _LDBL12*       const ld12
    ) throw()
{
    *UL_LO_12 (ld12) = 0;
    *UL_MED_12(ld12) = 0;
    *UL_HI_12 (ld12) = 0;

    uint8_t const* const mantissa_last = mantissa + mantissa_count;
    for (uint8_t const* it = mantissa; it != mantissa_last; ++it)
    {
        multiply_ld12_by<Base>(ld12);

        // Add the new digit into the mantissa:
        _LDBL12 digit_ld12{};
        *UL_LO_12 (&digit_ld12) = *it;
        *UL_MED_12(&digit_ld12) = 0;
        *UL_HI_12 (&digit_ld12) = 0;
        add_ld12(ld12, &digit_ld12);
    }

    uint16_t expn = LD_BIASM1 + 80;

    // Normalize mantissa.  First shift word-by-word:
    while (*UL_HI_12(ld12) == 0)
    {
        *UL_HI_12 (ld12) = *UL_MED_12(ld12) >> 16;
        *UL_MED_12(ld12) = *UL_MED_12(ld12) << 16 | *UL_LO_12(ld12) >> 16;
        *UL_LO_12 (ld12) <<= 16;
        expn -= 16;
    }

    while ((*UL_HI_12(ld12) & MSB_USHORT) == 0)
    {
        shl_ld12<1>(ld12);
        --expn;
    }

    *U_EXP_12(ld12) = expn;
}

namespace __crt_strtox {

void __cdecl assemble_floating_point_zero(bool const is_negative, _LDBL12& result) throw()
{
    uint16_t const sign_bit{static_cast<uint16_t>(is_negative ? MSB_USHORT : 0x0000)};

    // Zero is all zero bits with an optional sign bit:
    *U_XT_12    (&result) = 0;
    *UL_MANLO_12(&result) = 0;
    *UL_MANHI_12(&result) = 0;
    *U_EXP_12   (&result) = sign_bit;
}

void __cdecl assemble_floating_point_infinity(bool const is_negative, _LDBL12& result) throw()
{
    uint16_t const sign_bit{static_cast<uint16_t>(is_negative ? MSB_USHORT : 0x0000)};

    // Infinity has an all-zero mantissa and an all-one exponent
    *U_XT_12    (&result) = 0;
    *UL_MANLO_12(&result) = 0;
    *UL_MANHI_12(&result) = 0;
    *U_EXP_12   (&result) = static_cast<uint16_t>(LD_MAXEXP) | sign_bit;
}

void __cdecl assemble_floating_point_qnan(bool const is_negative, _LDBL12& result) throw()
{
    uint16_t const sign_bit{static_cast<uint16_t>(is_negative ? MSB_USHORT : 0x0000)};

    *U_XT_12    (&result) = 0xffff;
    *UL_MANLO_12(&result) = 0xffffffff;
    *UL_MANHI_12(&result) = 0xffffffff;
    *U_EXP_12   (&result) = static_cast<uint16_t>(LD_MAXEXP) | sign_bit;
}

void __cdecl assemble_floating_point_snan(bool const is_negative, _LDBL12& result) throw()
{
    uint16_t const sign_bit{static_cast<uint16_t>(is_negative ? MSB_USHORT : 0x0000)};

    *U_XT_12    (&result) = 0xffff;
    *UL_MANLO_12(&result) = 0xffffffff;
    *UL_MANHI_12(&result) = 0xbfffffff;
    *U_EXP_12   (&result) = static_cast<uint16_t>(LD_MAXEXP) | sign_bit;
}

void __cdecl assemble_floating_point_ind(_LDBL12& result) throw()
{
    uint16_t const sign_bit{static_cast<uint16_t>(MSB_USHORT)};

    *U_XT_12    (&result) = 0x0000;
    *UL_MANLO_12(&result) = 0x00000000;
    *UL_MANHI_12(&result) = 0xc0000000;
    *U_EXP_12   (&result) = static_cast<uint16_t>(LD_MAXEXP) | sign_bit;
}

static SLD_STATUS __cdecl common_convert_to_ldbl12(
    floating_point_string const& immutable_data,
    bool                  const  is_hexadecimal,
    _LDBL12                    & result
    ) throw()
{
    floating_point_string data = immutable_data;

    // Cap the number of digits to LD_MAX_MAN_LEN, and round the last digit:
    if (data._mantissa_count > LD_MAX_MAN_LEN)
    {
        if (data._mantissa[LD_MAX_MAN_LEN] >= (is_hexadecimal ? 8 : 5))
        {
            ++data._mantissa[LD_MAX_MAN_LEN - 1];
        }

        data._mantissa_count = LD_MAX_MAN_LEN;
    }

    // The input exponent is an adjustment from the left (so 12.3456 is represented
    // as a mantiss a of 123456 with an exponent of 2), but the legacy functions
    // used here expect an adjustment from the right (so 12.3456 is represented
    // with an exponent of -4).
    int const exponent_adjustment_multiplier = is_hexadecimal ? 4 : 1;
    data._exponent -= data._mantissa_count * exponent_adjustment_multiplier;

    if (is_hexadecimal)
    {
        convert_mantissa_to_ld12<16>(data._mantissa, data._mantissa_count, &result);
        multiply_two_pow_ld12(&result, data._exponent);
    }
    else
    {
        convert_mantissa_to_ld12<10>(data._mantissa, data._mantissa_count, &result);
        multiply_ten_pow_ld12(&result, data._exponent);
    }

    if (data._is_negative)
    {
        *U_EXP_12(&result) |= 0x8000;
    }

    // If the combination of the mantissa and the exponent produced an infinity,
    // we've overflowed the range of the _LDBL12.
    if ((*U_EXP_12(&result) & LD_MAXEXP) == LD_MAXEXP)
    {
        return SLD_OVERFLOW;
    }

    return SLD_OK;
}

SLD_STATUS __cdecl convert_decimal_string_to_floating_type(
    floating_point_string const& data,
    _LDBL12                    & result
    ) throw()
{
    return common_convert_to_ldbl12(data, false, result);
}

SLD_STATUS __cdecl convert_hexadecimal_string_to_floating_type(
    floating_point_string const& data,
    _LDBL12                    & result
    ) throw()
{
    return common_convert_to_ldbl12(data, true, result);
}

} // namespace __crt_strtox

using namespace __crt_strtox;



static int __cdecl transform_into_return_value(SLD_STATUS const status) throw()
{
    switch (status)
    {
    case SLD_OVERFLOW:  return _OVERFLOW;
    case SLD_UNDERFLOW: return _UNDERFLOW;
    default:            return 0;
    }
}

// The internal mantissa length in ints
#define INTRNMAN_LEN 3

// Internal mantissaa representation for string conversion routines
typedef uint32_t* mantissa_t;


// Tests whether a mantissa ends in nbit zeroes.  Returns true if all mantissa
// bits after (and including) nbit are zero; returns false otherwise.
static __forceinline bool __cdecl mantissa_has_zero_tail(mantissa_t const mantissa, int const nbit) throw()
{
    int nl = nbit / 32;
    int const nb = 31 - nbit % 32;

    //
    //             |<---- tail to be checked --->
    //
    //    --  ------------------------           ----
    //    |...    |          |  ...      |
    //    --  ------------------------           ----
    //    ^    ^    ^
    //    |    |    |<----nb----->
    //    man    nl   nbit
    //

    uint32_t const bitmask = ~(UINT32_MAX << nb);

    if (mantissa[nl] & bitmask)
        return false;

    ++nl;

    for (; nl < INTRNMAN_LEN; ++nl)
    {
        if (mantissa[nl])
            return false;
    }

    return true;
}



// Increments a mantissa.  The nbit argument specifies the end of the part to
// be incremented.  Returns true if overflow occurs; false otherwise.
static __forceinline bool __cdecl increment_mantissa(mantissa_t const mantissa, int const nbit) throw()
{
    int nl = nbit / 32;
    int const nb = 31 - nbit % 32;

    //
    //    |<--- part to be incremented -->|
    //
    //    ---------------------------------
    //    |...          |            |   ...      |
    //    ---------------------------------
    //    ^          ^        ^
    //    |          |        |<--nb-->
    //    man          nl        nbit
    //

    uint32_t const one = static_cast<uint32_t>(1) << nb;

    bool carry = add_uint32_carry(mantissa[nl], one, &mantissa[nl]);

    --nl;

    for (; nl >= 0 && carry; --nl)
    {
        carry = add_uint32_carry(mantissa[nl], 1, &mantissa[nl]);
    }

    return carry;
}

// Rounds a mantissa to the given precision.  Returns true if overflow occurs;
// returns false otherwise.
static __forceinline bool __cdecl round_mantissa(mantissa_t const mantissa, int const precision) throw()
{
    // The order of the n'th bit is n-1, since the first bit is bit 0
    // therefore decrement precision to get the order of the last bit
    // to be kept

    int const nbit = precision - 1;

    int const rndbit = nbit + 1;

    int const nl = rndbit / 32;
    int const nb = 31 - rndbit % 32;

    // Get value of round bit
    uint32_t const rndmask = static_cast<uint32_t>(1) << nb;

    bool retval = false;
    if ((mantissa[nl] & rndmask) && !mantissa_has_zero_tail(mantissa, rndbit))
    {
        // round up
        retval = increment_mantissa(mantissa, nbit);
    }

    // Fill rest of mantissa with zeroes
    mantissa[nl] &= UINT32_MAX << nb;
    for (int i = nl + 1; i < INTRNMAN_LEN; ++i)
    {
        mantissa[i] = 0;
    }

    return retval;
}

static void __cdecl convert_ld12_to_ldouble(
    _LDBL12 const* const pld12,
    _LDOUBLE*      const result
    ) throw()
{
    // This implementation is based on the fact that the _LDBL12 format is
    // identical to the long double and has 2 extra bytes of mantissa
    uint16_t       exponent = *U_EXP_12(pld12) & static_cast<uint16_t>(0x7fff);
    uint16_t const sign     = *U_EXP_12(pld12) & static_cast<uint16_t>(0x8000);

    uint32_t mantissa[] =
    {
        *UL_MANHI_12(pld12),
        *UL_MANLO_12(pld12),
        uint32_t ((*U_XT_12(pld12)) << 16)
    };

    if (round_mantissa(mantissa, 64))
    {
        // The MSB of the mantissa is explicit and should be 1
        // since we had a carry, the mantissa is now 0.
        mantissa[0] = MSB_ULONG;
        ++exponent;
    }

    *UL_MANHI_LD(result) = mantissa[0];
    *UL_MANLO_LD(result) = mantissa[1];
    *U_EXP_LD   (result) = sign | exponent;
}

extern "C" int __cdecl _atoldbl_l(_LDOUBLE* const result, char* const string, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    _LDBL12 intermediate_result{};
    SLD_STATUS const conversion_status = parse_floating_point(
        locale_update.GetLocaleT(),
        make_c_string_character_source(string, nullptr),
        &intermediate_result);

    convert_ld12_to_ldouble(&intermediate_result, result);
    return transform_into_return_value(conversion_status);
}

extern "C" int __cdecl _atoldbl(_LDOUBLE* const result, char* const string)
{
    return _atoldbl_l(result, string, nullptr);
}
