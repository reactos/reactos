/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/streamout.c
 * PURPOSE:         Implementation of streamout
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <strings.h>
#include <math.h>
#include <float.h>

#ifdef _UNICODE
# define streamout wstreamout
# define format_float format_floatw
#endif

#define MB_CUR_MAX 10
#define BUFFER_SIZE (32 + 17)

int mbtowc(wchar_t *wchar, const char *mbchar, size_t count);
int wctomb(char *mbchar, wchar_t wchar);

typedef struct _STRING
{
  unsigned short Length;
  unsigned short MaximumLength;
  void *Buffer;
} STRING;

enum
{
    /* Formatting flags */
    FLAG_ALIGN_LEFT =    0x01,
    FLAG_FORCE_SIGN =    0x02,
    FLAG_FORCE_SIGNSP =  0x04,
    FLAG_PAD_ZERO =      0x08,
    FLAG_SPECIAL =       0x10,

    /* Data format flags */
    FLAG_SHORT =         0x100,
    FLAG_LONG =          0x200,
    FLAG_WIDECHAR =      FLAG_LONG,
    FLAG_INT64 =         0x400,
#ifdef _WIN64
    FLAG_INTPTR =        FLAG_INT64,
#else
    FLAG_INTPTR =        0,
#endif
    FLAG_LONGDOUBLE =    0x800,
};

#define va_arg_f(argptr, flags) \
    (flags & FLAG_INT64) ? va_arg(argptr, __int64) : \
    (flags & FLAG_SHORT) ? (short)va_arg(argptr, int) : \
    va_arg(argptr, int)

#define va_arg_fu(argptr, flags) \
    (flags & FLAG_INT64) ? va_arg(argptr, unsigned __int64) : \
    (flags & FLAG_SHORT) ? (unsigned short)va_arg(argptr, int) : \
    va_arg(argptr, unsigned int)

#define va_arg_ffp(argptr, flags) \
    (flags & FLAG_LONGDOUBLE) ? va_arg(argptr, long double) : \
    va_arg(argptr, double)

#define get_exp(f) (int)floor(f == 0 ? 0 : (f >= 0 ? log10(f) : log10(-f)))
#define round(x) floor((x) + 0.5)

#ifndef _USER32_WSPRINTF

void
#ifdef _LIBCNT_
/* Due to restrictions in kernel mode regarding the use of floating point,
   we prevent it from being inlined */
__declspec(noinline)
#endif
format_float(
    _TCHAR chr,
    unsigned int flags,
    int precision,
    _TCHAR **string,
    const _TCHAR **prefix,
    va_list *argptr)
{
    static const _TCHAR digits_l[] = _T("0123456789abcdef0x");
    static const _TCHAR digits_u[] = _T("0123456789ABCDEF0X");
    static const _TCHAR _nan[] = _T("#QNAN");
    static const _TCHAR _infinity[] = _T("#INF");
    const _TCHAR *digits = digits_l;
    int exponent = 0, sign;
    long double fpval, fpval2;
    int padding = 0, num_digits, val32, base = 10;

    /* Normalize the precision */
    if (precision < 0) precision = 6;
    else if (precision > 17)
    {
        padding = precision - 17;
        precision = 17;
    }

    /* Get the float value and calculate the exponent */
    fpval = va_arg_ffp(*argptr, flags);
    exponent = get_exp(fpval);
    sign = fpval < 0 ? -1 : 1;

    switch (chr)
    {
        case _T('G'):
            digits = digits_u;
        case _T('g'):
            if (precision > 0) precision--;
            if (exponent < -4 || exponent >= precision) goto case_e;

            /* Shift the decimal point and round */
            fpval2 = round(sign * fpval * pow(10., precision));

            /* Skip trailing zeroes */
            while (precision && (unsigned __int64)fpval2 % 10 == 0)
            {
                precision--;
                fpval2 /= 10;
            }
            break;

        case _T('E'):
            digits = digits_u;
        case _T('e'):
        case_e:
            /* Shift the decimal point and round */
            fpval2 = round(sign * fpval * pow(10., precision - exponent));

            /* Compensate for changed exponent through rounding */
            if (fpval2 >= (unsigned __int64)pow(10., precision + 1))
            {
                exponent++;
                fpval2 = round(sign * fpval * pow(10., precision - exponent));
            }

            val32 = exponent >= 0 ? exponent : -exponent;

            // FIXME: handle length of exponent field:
            // http://msdn.microsoft.com/de-de/library/0fatw238%28VS.80%29.aspx (DEAD_LINK)
            num_digits = 3;
            while (num_digits--)
            {
                *--(*string) = digits[val32 % 10];
                val32 /= 10;
            }

            /* Sign for the exponent */
            *--(*string) = exponent >= 0 ? _T('+') : _T('-');

            /* Add 'e' or 'E' separator */
            *--(*string) = digits[0xe];
            break;

        case _T('A'):
            digits = digits_u;
        case _T('a'):
//            base = 16;
            // FIXME: TODO

        case _T('f'):
        default:
            /* Shift the decimal point and round */
            fpval2 = round(sign * fpval * pow(10., precision));
            break;
    }

    /* Handle sign */
    if (fpval < 0)
    {
        *prefix = _T("-");
    }
    else if (flags & FLAG_FORCE_SIGN)
        *prefix = _T("+");
    else if (flags & FLAG_FORCE_SIGNSP)
        *prefix = _T(" ");

    /* Handle special cases first */
    if (_isnan(fpval))
    {
        (*string) -= sizeof(_nan) / sizeof(_TCHAR) - 1;
        _tcscpy((*string), _nan);
        fpval2 = 1;
    }
    else if (!_finite(fpval))
    {
        (*string) -= sizeof(_infinity) / sizeof(_TCHAR) - 1;
        _tcscpy((*string), _infinity);
        fpval2 = 1;
    }
    else
    {
        /* Zero padding */
        while (padding-- > 0) *--(*string) = _T('0');

        /* Digits after the decimal point */
        num_digits = precision;
        while (num_digits-- > 0)
        {
            *--(*string) = digits[(unsigned __int64)fpval2 % 10];
            fpval2 /= base;
        }
    }

    if (precision > 0 || flags & FLAG_SPECIAL)
        *--(*string) = _T('.');

    /* Digits before the decimal point */
    do
    {
        *--(*string) = digits[(unsigned __int64)fpval2 % base];
        fpval2 /= base;
    }
    while ((unsigned __int64)fpval2);

}
#endif

static
int
streamout_char(FILE *stream, int chr)
{
#if !defined(_USER32_WSPRINTF)
     if ((stream->_flag & _IOSTRG) && (stream->_base == NULL))
        return 1;
#endif
#if defined(_USER32_WSPRINTF) || defined(_LIBCNT_)
    /* Check if the buffer is full */
    if (stream->_cnt < sizeof(_TCHAR))
        return 0;

    *(_TCHAR*)stream->_ptr = chr;
    stream->_ptr += sizeof(_TCHAR);
    stream->_cnt -= sizeof(_TCHAR);

    return 1;
#else
    return _fputtc((_TCHAR)chr, stream) != _TEOF;
#endif
}

static
int
streamout_astring(FILE *stream, const char *string, size_t count)
{
    _TCHAR chr;
    int written = 0;

#if !defined(_USER32_WSPRINTF)
     if ((stream->_flag & _IOSTRG) && (stream->_base == NULL))
        return count;
#endif

    while (count--)
    {
#ifdef _UNICODE
        int len;
        if ((len = mbtowc(&chr, string, MB_CUR_MAX)) < 1) break;
        string += len;
#else
        chr = *string++;
#endif
        if (streamout_char(stream, chr) == 0) return -1;
        written++;
    }

    return written;
}

static
int
streamout_wstring(FILE *stream, const wchar_t *string, size_t count)
{
    wchar_t chr;
    int written = 0;

#if defined(_UNICODE) && !defined(_USER32_WSPRINTF)
     if ((stream->_flag & _IOSTRG) && (stream->_base == NULL))
        return count;
#endif

    while (count--)
    {
#ifndef _UNICODE
        char mbchar[MB_CUR_MAX], *ptr = mbchar;
        int mblen;

        mblen = wctomb(mbchar, *string++);
        if (mblen <= 0) return written;

        while (chr = *ptr++, mblen--)
#else
        chr = *string++;
#endif
        {
            if (streamout_char(stream, chr) == 0) return -1;
            written++;
        }
    }

    return written;
}

#ifdef _UNICODE
#define streamout_string streamout_wstring
#else
#define streamout_string streamout_astring
#endif

#ifdef _USER32_WSPRINTF
# define USE_MULTISIZE 0
#else
# define USE_MULTISIZE 1
#endif

int
__cdecl
streamout(FILE *stream, const _TCHAR *format, va_list argptr)
{
    static const _TCHAR digits_l[] = _T("0123456789abcdef0x");
    static const _TCHAR digits_u[] = _T("0123456789ABCDEF0X");
    static const char *_nullstring = "(null)";
    _TCHAR buffer[BUFFER_SIZE + 1];
    _TCHAR chr, *string;
    STRING *nt_string;
    const _TCHAR *digits, *prefix;
    int base, fieldwidth, precision, padding;
    size_t prefixlen, len;
    int written = 1, written_all = 0;
    unsigned int flags;
    unsigned __int64 val64;

    buffer[BUFFER_SIZE] = '\0';

    while (written >= 0)
    {
        chr = *format++;

        /* Check for end of format string */
        if (chr == _T('\0')) break;

        /* Check for 'normal' character or double % */
        if ((chr != _T('%')) ||
            (chr = *format++) == _T('%'))
        {
            /* Write the character to the stream */
            if ((written = streamout_char(stream, chr)) == 0) return -1;
            written_all += written;
            continue;
        }

        /* Handle flags */
        flags = 0;
        while (1)
        {
                 if (chr == _T('-')) flags |= FLAG_ALIGN_LEFT;
            else if (chr == _T('+')) flags |= FLAG_FORCE_SIGN;
            else if (chr == _T(' ')) flags |= FLAG_FORCE_SIGNSP;
            else if (chr == _T('0')) flags |= FLAG_PAD_ZERO;
            else if (chr == _T('#')) flags |= FLAG_SPECIAL;
            else break;
            chr = *format++;
        }

        /* Handle field width modifier */
        if (chr == _T('*'))
        {
#ifdef _USER32_WSPRINTF
            if ((written = streamout_char(stream, chr)) == 0) return -1;
            written_all += written;
            continue;
#else
            fieldwidth = va_arg(argptr, int);
            if (fieldwidth < 0)
            {
                flags |= FLAG_ALIGN_LEFT;
                fieldwidth = -fieldwidth;
            }
            chr = *format++;
#endif
        }
        else
        {
            fieldwidth = 0;
            while (chr >= _T('0') && chr <= _T('9'))
            {
                fieldwidth = fieldwidth * 10 + (chr - _T('0'));
                chr = *format++;
            }
        }

        /* Handle precision modifier */
        if (chr == '.')
        {
            chr = *format++;

            if (chr == _T('*'))
            {
#ifdef _USER32_WSPRINTF
                if ((written = streamout_char(stream, chr)) == 0) return -1;
                written_all += written;
                continue;
#else
                precision = va_arg(argptr, int);
                chr = *format++;
#endif
            }
            else
            {
                precision = 0;
                while (chr >= _T('0') && chr <= _T('9'))
                {
                    precision = precision * 10 + (chr - _T('0'));
                    chr = *format++;
                }
            }
        }
        else precision = -1;

        /* Handle argument size prefix */
        do
        {
                 if (chr == _T('h')) flags |= FLAG_SHORT;
            else if (chr == _T('w')) flags |= FLAG_WIDECHAR;
            else if (chr == _T('L')) flags |= 0; // FIXME: long double
            else if (chr == _T('F')) flags |= 0; // FIXME: what is that?
            else if (chr == _T('z') && *format && strchr("udxXion", *format))
            {
                flags |= FLAG_INTPTR;
            }
            else if (chr == _T('l'))
            {
                /* Check if this is the 2nd 'l' in a row */
                if (format[-2] == 'l') flags |= FLAG_INT64;
                else flags |= FLAG_LONG;
            }
            else if (chr == _T('I'))
            {
                if (format[0] == _T('3') && format[1] == _T('2'))
                {
                    format += 2;
                }
                else if (format[0] == _T('6') && format[1] == _T('4'))
                {
                    format += 2;
                    flags |= FLAG_INT64;
                }
                else if (format[0] == _T('x') || format[0] == _T('X') ||
                         format[0] == _T('d') || format[0] == _T('i') ||
                         format[0] == _T('u') || format[0] == _T('o'))
                {
                    flags |= FLAG_INTPTR;
                }
                else break;
            }
            else break;
            chr = *format++;
        }
        while (USE_MULTISIZE);

        /* Handle the format specifier */
        digits = digits_l;
        string = &buffer[BUFFER_SIZE];
        base = 10;
        prefix = 0;
        switch (chr)
        {
            case _T('n'):
                if (flags & FLAG_INT64)
                    *va_arg(argptr, __int64*) = written_all;
                else if (flags & FLAG_SHORT)
                    *va_arg(argptr, short*) = written_all;
                else
                    *va_arg(argptr, int*) = written_all;
                continue;

            case _T('C'):
#ifndef _UNICODE
                if (!(flags & FLAG_SHORT)) flags |= FLAG_WIDECHAR;
#endif
                goto case_char;

            case _T('c'):
#ifdef _UNICODE
                if (!(flags & FLAG_SHORT)) flags |= FLAG_WIDECHAR;
#endif
            case_char:
                string = buffer;
                len = 1;
                if (flags & FLAG_WIDECHAR)
                {
                    ((wchar_t*)string)[0] = va_arg(argptr, int);
                    ((wchar_t*)string)[1] = _T('\0');
                }
                else
                {
                    ((char*)string)[0] = va_arg(argptr, int);
                    ((char*)string)[1] = _T('\0');
                }
                break;

            case _T('Z'):
                nt_string = va_arg(argptr, void*);
                if (nt_string && (string = nt_string->Buffer))
                {
                    len = nt_string->Length;
                    if (flags & FLAG_WIDECHAR) len /= sizeof(wchar_t);
                    break;
                }
                string = 0;
                goto case_string;

            case _T('S'):
                string = va_arg(argptr, void*);
#ifndef _UNICODE
                if (!(flags & FLAG_SHORT)) flags |= FLAG_WIDECHAR;
#endif
                goto case_string;

            case _T('s'):
                string = va_arg(argptr, void*);
#ifdef _UNICODE
                if (!(flags & FLAG_SHORT)) flags |= FLAG_WIDECHAR;
#endif

            case_string:
                if (!string)
                {
                    string = (_TCHAR*)_nullstring;
                    flags &= ~FLAG_WIDECHAR;
                }

                if (flags & FLAG_WIDECHAR)
                    len = wcsnlen((wchar_t*)string, (unsigned)precision);
                else
                    len = strnlen((char*)string, (unsigned)precision);
                precision = 0;
                break;

#ifndef _USER32_WSPRINTF
            case _T('G'):
            case _T('E'):
            case _T('A'):
            case _T('g'):
            case _T('e'):
            case _T('a'):
            case _T('f'):
#ifdef _UNICODE
                flags |= FLAG_WIDECHAR;
#else
                flags &= ~FLAG_WIDECHAR;
#endif
                /* Use external function, one for kernel one for user mode */
                format_float(chr, flags, precision, &string, &prefix, &argptr);
                len = _tcslen(string);
                precision = 0;
                break;
#endif

            case _T('d'):
            case _T('i'):
                val64 = (__int64)va_arg_f(argptr, flags);

                if ((__int64)val64 < 0)
                {
                    val64 = -(__int64)val64;
                    prefix = _T("-");
                }
                else if (flags & FLAG_FORCE_SIGN)
                    prefix = _T("+");
                else if (flags & FLAG_FORCE_SIGNSP)
                    prefix = _T(" ");

                goto case_number;

            case _T('o'):
                base = 8;
                if (flags & FLAG_SPECIAL)
                {
                    prefix = _T("0");
                    if (precision > 0) precision--;
                }
                goto case_unsigned;

            case _T('p'):
                precision = 2 * sizeof(void*);
                flags &= ~FLAG_PAD_ZERO;
                flags |= FLAG_INTPTR;
                /* Fall through */

            case _T('X'):
                digits = digits_u;
                /* Fall through */

            case _T('x'):
                base = 16;
                if (flags & FLAG_SPECIAL)
                {
                    prefix = &digits[16];
#ifdef _USER32_WSPRINTF
                    fieldwidth += 2;
#endif
                }

            case _T('u'):
            case_unsigned:
                val64 = va_arg_fu(argptr, flags);

            case_number:
#ifdef _UNICODE
                flags |= FLAG_WIDECHAR;
#else
                flags &= ~FLAG_WIDECHAR;
#endif
                if (precision < 0) precision = 1;

                /* Gather digits in reverse order */
                while (val64)
                {
                    *--string = digits[val64 % base];
                    val64 /= base;
                    precision--;
                }

                len = _tcslen(string);
                break;

            default:
                /* Treat anything else as a new character */
                format--;
                continue;
        }

        /* Calculate padding */
        prefixlen = prefix ? _tcslen(prefix) : 0;
        if (precision < 0) precision = 0;
        padding = (int)(fieldwidth - len - prefixlen - precision);
        if (padding < 0) padding = 0;

        /* Optional left space padding */
        if ((flags & (FLAG_ALIGN_LEFT | FLAG_PAD_ZERO)) == 0)
        {
            for (; padding > 0; padding--)
            {
                if ((written = streamout_char(stream, _T(' '))) == 0) return -1;
                written_all += written;
            }
        }

        /* Optional prefix */
        if (prefix)
        {
            written = streamout_string(stream, prefix, prefixlen);
            if (written == -1) return -1;
            written_all += written;
        }

        /* Optional left '0' padding */
        if ((flags & FLAG_ALIGN_LEFT) == 0) precision += padding;
        while (precision-- > 0)
        {
            if ((written = streamout_char(stream, _T('0'))) == 0) return -1;
            written_all += written;
        }

        /* Output the string */
        if (flags & FLAG_WIDECHAR)
            written = streamout_wstring(stream, (wchar_t*)string, len);
        else
            written = streamout_astring(stream, (char*)string, len);
        if (written == -1) return -1;
        written_all += written;

#if 0 && SUPPORT_FLOAT
        /* Optional right '0' padding */
        while (precision-- > 0)
        {
            if ((written = streamout_char(stream, _T('0'))) == 0) return -1;
            written_all += written;
            len++;
        }
#endif

        /* Optional right padding */
        if (flags & FLAG_ALIGN_LEFT)
        {
            while (padding-- > 0)
            {
                if ((written = streamout_char(stream, _T(' '))) == 0) return -1;
                written_all += written;
            }
        }

    }

    if (written == -1) return -1;

    return written_all;
}

