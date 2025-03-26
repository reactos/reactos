#include <precomp.h>

// From Wine msvcrt.h
typedef struct {ULONG x80[3];} MSVCRT__LDOUBLE; /* Intel 80 bit FP format has sizeof() 12 */
enum fpmod {
    FP_ROUND_ZERO, /* only used when dropped part contains only zeros */
    FP_ROUND_DOWN,
    FP_ROUND_EVEN,
    FP_ROUND_UP,
    FP_VAL_INFINITY,
    FP_VAL_NAN
};
struct fpnum {
    int sign;
    int exp;
    ULONGLONG m;
    enum fpmod mod;
};

// From wine bnum.h
#define EXP_BITS 11
#define MANT_BITS 53

int fpnum_double(struct fpnum *fp, double *d)
{
    ULONGLONG bits = 0;

    if (fp->mod == FP_VAL_INFINITY)
    {
        *d = fp->sign * INFINITY;
        return 0;
    }

    if (fp->mod == FP_VAL_NAN)
    {
        bits = ~0;
        if (fp->sign == 1)
            bits &= ~((ULONGLONG)1 << (MANT_BITS + EXP_BITS - 1));
        *d = *(double*)&bits;
        return 0;
    }

    TRACE("%c %#I64x *2^%d (round %d)\n", fp->sign == -1 ? '-' : '+',
        fp->m, fp->exp, fp->mod);
    if (!fp->m)
    {
        *d = fp->sign * 0.0;
        return 0;
    }

    /* make sure that we don't overflow modifying exponent */
    if (fp->exp > 1<<EXP_BITS)
    {
        *d = fp->sign * INFINITY;
        return ERANGE;
    }
    if (fp->exp < -(1<<EXP_BITS))
    {
        *d = fp->sign * 0.0;
        return ERANGE;
    }
    fp->exp += MANT_BITS - 1;

    /* normalize mantissa */
    while(fp->m < (ULONGLONG)1 << (MANT_BITS-1))
    {
        fp->m <<= 1;
        fp->exp--;
    }
    while(fp->m >= (ULONGLONG)1 << MANT_BITS)
    {
        if (fp->m & 1 || fp->mod != FP_ROUND_ZERO)
        {
            if (!(fp->m & 1)) fp->mod = FP_ROUND_DOWN;
            else if(fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
            else fp->mod = FP_ROUND_UP;
        }
        fp->m >>= 1;
        fp->exp++;
    }
    fp->exp += (1 << (EXP_BITS-1)) - 1;

    /* handle subnormals */
    if (fp->exp <= 0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
    }
    while(fp->m && fp->exp<0)
    {
        if (fp->m & 1 && fp->mod == FP_ROUND_ZERO) fp->mod = FP_ROUND_EVEN;
        else if (fp->m & 1) fp->mod = FP_ROUND_UP;
        else if (fp->mod != FP_ROUND_ZERO) fp->mod = FP_ROUND_DOWN;
        fp->m >>= 1;
        fp->exp++;
    }

    /* round mantissa */
    if (fp->mod == FP_ROUND_UP || (fp->mod == FP_ROUND_EVEN && fp->m & 1))
    {
        fp->m++;

        /* handle subnormal that falls into regular range due to rounding */
        if (fp->m == (ULONGLONG)1 << (MANT_BITS - 1))
        {
            fp->exp++;
        }
        else if (fp->m >= (ULONGLONG)1 << MANT_BITS)
        {
            fp->exp++;
            fp->m >>= 1;
        }
    }

    if (fp->exp >= (1<<EXP_BITS)-1)
    {
        *d = fp->sign * INFINITY;
        return ERANGE;
    }
    if (!fp->m || fp->exp < 0)
    {
        *d = fp->sign * 0.0;
        return ERANGE;
    }

    if (fp->sign == -1)
        bits |= (ULONGLONG)1 << (MANT_BITS + EXP_BITS - 1);
    bits |= (ULONGLONG)fp->exp << (MANT_BITS - 1);
    bits |= fp->m & (((ULONGLONG)1 << (MANT_BITS - 1)) - 1);

    TRACE("returning %#I64x\n", bits);
    *d = *(double*)&bits;
    return 0;
}

#define I10_OUTPUT_MAX_PREC 21
/* Internal structure used by $I10_OUTPUT */
struct _I10_OUTPUT_DATA {
    short pos;
    char sign;
    BYTE len;
    char str[I10_OUTPUT_MAX_PREC+1]; /* add space for '\0' */
};

/*********************************************************************
 *              $I10_OUTPUT (MSVCRT.@)
 * ld80 - long double (Intel 80 bit FP in 12 bytes) to be printed to data
 * prec - precision of part, we're interested in
 * flag - 0 for first prec digits, 1 for fractional part
 * data - data to be populated
 *
 * return value
 *      0 if given double is NaN or INF
 *      1 otherwise
 *
 * FIXME
 *      Native sets last byte of data->str to '0' or '9', I don't know what
 *      it means. Current implementation sets it always to '0'.
 */
int CDECL I10_OUTPUT(MSVCRT__LDOUBLE ld80, int prec, int flag, struct _I10_OUTPUT_DATA *data)
{
    struct fpnum num;
    double d;
    char format[8];
    char buf[I10_OUTPUT_MAX_PREC+9]; /* 9 = strlen("0.e+0000") + '\0' */
    char *p;

    if ((ld80.x80[2] & 0x7fff) == 0x7fff)
    {
        if (ld80.x80[0] == 0 && ld80.x80[1] == 0x80000000)
            strcpy( data->str, "1#INF" );
        else
            strcpy( data->str, (ld80.x80[1] & 0x40000000) ? "1#QNAN" : "1#SNAN" );
        data->pos = 1;
        data->sign = (ld80.x80[2] & 0x8000) ? '-' : ' ';
        data->len = (BYTE)strlen(data->str);
        return 0;
    }

    num.sign = (ld80.x80[2] & 0x8000) ? -1 : 1;
    num.exp  = (ld80.x80[2] & 0x7fff) - 0x3fff - 63;
    num.m    = ld80.x80[0] | ((ULONGLONG)ld80.x80[1] << 32);
    num.mod  = FP_ROUND_EVEN;
    fpnum_double( &num, &d );
    TRACE("(%lf %d %x %p)\n", d, prec, flag, data);

    if(d<0) {
        data->sign = '-';
        d = -d;
    } else
        data->sign = ' ';

    if(flag&1) {
        int exp = 1 + floor(log10(d));

        prec += exp;
        if(exp < 0)
            prec--;
    }
    prec--;

    if(prec+1 > I10_OUTPUT_MAX_PREC)
        prec = I10_OUTPUT_MAX_PREC-1;
    else if(prec < 0) {
        d = 0.0;
        prec = 0;
    }

    sprintf_s(format, sizeof(format), "%%.%dle", prec);
    sprintf_s(buf, sizeof(buf), format, d);

    buf[1] = buf[0];
    data->pos = atoi(buf+prec+3);
    if(buf[1] != '0')
        data->pos++;

    for(p = buf+prec+1; p>buf+1 && *p=='0'; p--);
    data->len = p-buf;

    memcpy(data->str, buf+1, data->len);
    data->str[data->len] = '\0';

    if(buf[1]!='0' && prec-data->len+1>0)
        memcpy(data->str+data->len+1, buf+data->len+1, prec-data->len+1);

    return 1;
}
#undef I10_OUTPUT_MAX_PREC
