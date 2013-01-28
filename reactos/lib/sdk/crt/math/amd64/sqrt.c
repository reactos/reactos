
#include <intrin.h>

double
sqrt (
    double x)
{
    register union
    {
        __m128d x128d;
        __m128i x128i;
    } u ;
    register union
    {
        unsigned long long ullx;
        double dbl;
    } u2;

    /* Set the lower double-precision value of u to x.
       All that we want, is that the compiler understands that we have the
       function parameter in a register that we can address as an __m128.
       Sadly there is no obvious way to do that. If we use the union, VS will
       generate code to store xmm0 in memory and the read it into a GPR.
       We avoid memory access by using a direct move. But even here we won't
       get a simple MOVSD. We can either do:
       a) _mm_set_sd: move x into the lower part of an xmm register and zero
          out the upper part (XORPD+MOVSD)
       b) _mm_set1_pd: move x into the lower and higher part of an xmm register
         (MOVSD+UNPCKLPD)
       c) _mm_set_pd, which either generates a memory access, when we try to
          tell it to keep the upper 64 bits, or generate 2 MOVAPS + UNPCKLPD
       We choose a, which is probably the fastest.
    */
    u.x128d = _mm_set_sd(x);

    /* Move the contents of the lower 64 bit into a 64 bit GPR using MOVD */
    u2.ullx = _mm_cvtsi128_si64(u.x128i);

    /* Check for negative values */
    if (u2.ullx & 0x8000000000000000ULL)
    {
        /* Check if this is *really* negative and not just -0.0 */
        if (u2.ullx != 0x8000000000000000ULL)
        {
            /* Return -1.#IND00 */
            u2.ullx = 0xfff8000000000000ULL;
        }

        /* Return what we have */
        return u2.dbl;
    }

    /* Check if this is a NaN (bits 52-62 are 1, bit 0-61 are not all 0) or
       negative (bit 63 is 1) */
    if (u2.ullx > 0x7FF0000000000000ULL)
    {
        /* Set this bit. That's what MS function does. */
        u2.ullx |= 0x8000000000000ULL;
        return u2.dbl;
    }

    /* Calculate the square root. */
#ifdef _MSC_VER
    /* Another YAY for the MS compiler. There are 2 instructions we could use:
       SQRTPD (computes sqrt for 2 double values) or SQRTSD (computes sqrt for
       only the lower 64 bit double value). Obviously we only need 1. And on
       Some architectures SQRTPD is twice as slow as SQRTSD. On the other hand
       the MS compiler is stupid and always generates an additional MOVAPS
       instruction when SQRTSD is used. We choose to use SQRTPD here since on
       modern hardware it's as fast as SQRTSD. */
    u.x128d = _mm_sqrt_pd(u.x128d); // SQRTPD
#else
    u.x128d = _mm_sqrt_sd(u.x128d, u.x128d); // SQRTSD
#endif

    return u.x128d.m128d_f64[0];
}
