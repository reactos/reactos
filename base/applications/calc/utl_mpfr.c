#include "calc.h"

void prepare_rpn_result_2(calc_number_t *rpn, TCHAR *buffer, int size, int base)
{
    char   temp[1024];
    char  *ptr, *dst;
    int    width, max_ld_width;
    unsigned long int n, q;
    mpz_t   zz;
    mpf_t   ff;

    mpz_init(zz);
    mpf_init(ff);
    mpfr_get_z(zz, rpn->mf, MPFR_DEFAULT_RND);
    mpfr_get_f(ff, rpn->mf, MPFR_DEFAULT_RND);

    switch (base) {
    case IDC_RADIO_HEX:
        gmp_sprintf(temp, "%ZX", zz);
        break;
    case IDC_RADIO_DEC:
        /*
         * The output display is much shorter in standard mode,
         * so I'm forced to reduce the precision here :(
         */
        if (calc.layout == CALC_LAYOUT_STANDARD)
            max_ld_width = 16;
        else
            max_ld_width = 64;

        /* calculate the width of integer number */
        if (mpf_sgn(ff) == 0)
            width = 1;
        else {
            mpfr_t t;
            mpfr_init(t);
            mpfr_abs(t, rpn->mf, MPFR_DEFAULT_RND);
            mpfr_log10(t, t, MPFR_DEFAULT_RND);
            width = 1 + mpfr_get_si(t, MPFR_DEFAULT_RND);
            mpfr_clear(t);
        }
        if (calc.sci_out == TRUE || width > max_ld_width || width < -max_ld_width)
            ptr = temp + gmp_sprintf(temp, "%*.*#Fe", 1, max_ld_width, ff);
        else {
            ptr = temp + gmp_sprintf(temp, "%#*.*Ff", width, ((max_ld_width-width-1)>=0) ? max_ld_width-width-1 : 0, ff);
            dst = strchr(temp, '.');
            while (--ptr > dst)
                if (*ptr != '0')
                    break;

            /* put the string terminator for removing the final '0' (if any) */
            ptr[1] = '\0';
            /* check if the number finishes with '.' */
            if (ptr == dst)
                /* remove the dot (it will be re-added later) */
                ptr[0] = '\0';
        }
        break;
    case IDC_RADIO_OCT:
        gmp_sprintf(temp, "%Zo", zz);
        break;
    case IDC_RADIO_BIN:
        /* if the number is zero, just write 0 ;) */
        if (rpn_is_zero(rpn)) {
            temp[0] = TEXT('0');
            temp[1] = TEXT('\0');
            break;
        }
        /* repeat until a bit set to '1' is found */
        n = 0;
        do {
            q = mpz_scan1(zz, n);
            if (q == ULONG_MAX)
                break;
            while (n < q)
                temp[n++] = '0';
            temp[n++] = '1';
        } while (1);
        /* now revert the string into TCHAR buffer */
        for (q=0; q<n; q++)
            buffer[n-q-1] = (temp[q] == '1') ? TEXT('1') : TEXT('0');
        buffer[n] = TEXT('\0');

        mpz_clear(zz);
        mpf_clear(ff);
        return;
    }
    mpz_clear(zz);
    mpf_clear(ff);
    _sntprintf(buffer, SIZEOF(calc.buffer), TEXT("%s"), temp);
}

void convert_text2number_2(calc_number_t *a)
{
    int base;
#ifdef UNICODE
    int sz;
    char *temp;
#endif

    switch (calc.base) {
    case IDC_RADIO_HEX: base = 16; break;
    case IDC_RADIO_DEC: base = 10; break;
    case IDC_RADIO_OCT: base = 8; break;
    case IDC_RADIO_BIN: base = 2; break;
    default: return;
    }
#ifdef UNICODE
/*
 * libmpfr and libgmp accept only ascii chars.
 */
    sz = WideCharToMultiByte(CP_ACP, 0, calc.buffer, -1, NULL, 0, NULL, NULL);
    if (!sz)
        return;
    temp = (char *)_alloca(sz);
    sz = WideCharToMultiByte(CP_ACP, 0, calc.buffer, -1, temp, sz, NULL, NULL);
    mpfr_strtofr(a->mf, temp, NULL, base, MPFR_DEFAULT_RND);
#else
    mpfr_strtofr(a->mf, calc.buffer, NULL, base, MPFR_DEFAULT_RND);
#endif
}

void convert_real_integer(unsigned int base)
{
    switch (base) {
    case IDC_RADIO_DEC:
        break;
    case IDC_RADIO_OCT:
    case IDC_RADIO_BIN:
    case IDC_RADIO_HEX:
        if (calc.base == IDC_RADIO_DEC) {
            mpfr_trunc(calc.code.mf, calc.code.mf);
            apply_int_mask(&calc.code);
        }
        break;
    }
}

