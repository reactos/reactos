#include "calc.h"

void prepare_rpn_result_2(calc_number_t *rpn, TCHAR *buffer, int size, int base)
{
    TCHAR *ptr, *dst;
    calc_number_t tmp;
    int    width;

    switch (base) {
    case IDC_RADIO_HEX:
        _stprintf(buffer, TEXT("%I64X"), rpn->i);
        break;
    case IDC_RADIO_DEC:
/*
 * Modifed from 17 to 16 for fixing this bug:
 * 14+14+6.3+6.3= 40.5999999 instead of 40.6
 * So, it's probably better to leave the least
 * significant digit out of the display.
 */
#define MAX_LD_WIDTH    16
        /* calculate the width of integer number */
        width = (rpn->f==0) ? 1 : (int)log10(fabs(rpn->f))+1;
        if (calc.sci_out == TRUE || width > MAX_LD_WIDTH || width < -MAX_LD_WIDTH)
            ptr = buffer + _stprintf(buffer, TEXT("%#le"), rpn->f);
        else {
            ptr = buffer + _stprintf(buffer, TEXT("%#*.*lf"), width, ((MAX_LD_WIDTH-width-1)>=0) ? MAX_LD_WIDTH-width-1 : 0, rpn->f);
            dst = _tcschr(buffer, TEXT('.'));
            while (--ptr > dst)
                if (*ptr != TEXT('0'))
                    break;

            /* put the string terminator for removing the final '0' (if any) */
            ptr[1] = TEXT('\0');
            /* check if the number finishes with '.' */
            if (ptr == dst)
                /* remove the dot (it will be re-added later) */
                ptr[0] = TEXT('\0');
        }
#undef MAX_LD_WIDTH
        break;
    case IDC_RADIO_OCT:
        _stprintf(buffer, TEXT("%I64o"), rpn->i);
        break;
    case IDC_RADIO_BIN:
        if (rpn->i == 0) {
            buffer[0] = TEXT('0');
            buffer[1] = TEXT('\0');
            break;
        }
        tmp = *rpn;
        buffer[0] = TEXT('\0');
        while (tmp.u) {
            memmove(buffer+1, buffer, (size-1)*sizeof(TCHAR));
            if (tmp.u & 1)
                calc.buffer[0] = TEXT('1');
            else
                calc.buffer[0] = TEXT('0');
            tmp.u >>= 1;
        }
        break;
    }
}

void convert_text2number_2(calc_number_t *a)
{
    TCHAR *ptr;

    switch (calc.base) {
    case IDC_RADIO_HEX:
        _stscanf(calc.buffer, TEXT("%I64X"), &(a->i));
        break;
    case IDC_RADIO_DEC:
        _stscanf(calc.buffer, TEXT("%lf"), &(a->f));
        break;
    case IDC_RADIO_OCT:
        _stscanf(calc.buffer, TEXT("%I64o"), &(a->i));
        break;
    case IDC_RADIO_BIN:
        ptr = calc.buffer;
        a->i = 0;
        while (*ptr != TEXT('\0')) {
            a->i <<= 1;
            if (*ptr++ == TEXT('1'))
                a->i |= 1;
        }
        break;
    }
}

void convert_real_integer(unsigned int base)
{
    switch (base) {
    case IDC_RADIO_DEC:
        calc.code.f = (double)calc.code.i;
        break;
    case IDC_RADIO_OCT:
    case IDC_RADIO_BIN:
    case IDC_RADIO_HEX:
        if (calc.base == IDC_RADIO_DEC) {
            calc.code.i = (__int64)calc.code.f;
            apply_int_mask(&calc.code);
        }
        break;
    }
}
