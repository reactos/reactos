/* taken from wine ntdll and msvcrt string.c */

#include <precomp.h>

/*
 * @implemented
 */
char *
CDECL
_i64toa(__int64 value, char *string, int radix)
{
    ULONGLONG  val;
    int negative;
    char buffer[65];
    char *pos;
    int digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    memcpy(string, pos, &buffer[64] - pos + 1);
    return string;
}

/*
 * @implemented
 */
int CDECL _i64toa_s(__int64 value, char *str, size_t size, int radix)
{
    unsigned __int64 val;
    unsigned int digit;
    int is_negative;
    char buffer[65], *pos;
    size_t len;

    if (!MSVCRT_CHECK_PMT(str != NULL) || !MSVCRT_CHECK_PMT(size > 0) ||
        !MSVCRT_CHECK_PMT(radix >= 2) || !MSVCRT_CHECK_PMT(radix <= 36))
    {
        if (str && size)
            str[0] = '\0';
#ifndef _LIBCNT_
        *_errno() = EINVAL;
#endif
        return EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = 1;
        val = -value;
    }
    else
    {
        is_negative = 0;
        val = value;
    }

    pos = buffer + 64;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 65 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 63, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", ERANGE);
        return ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

/*
 * @implemented
 */
char *
CDECL
_ui64toa(unsigned __int64 value, char *string, int radix)
{
    char buffer[65];
    char *pos;
    int digit;

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[64] - pos + 1);
    return string;
}

/*
 * @implemented
 */
int CDECL _ui64toa_s(unsigned __int64 value, char *str,
        size_t size, int radix)
{
    char buffer[65], *pos;
    int digit;

    if (!MSVCRT_CHECK_PMT(str != NULL) || !MSVCRT_CHECK_PMT(size > 0) ||
        !MSVCRT_CHECK_PMT(radix>=2) || !MSVCRT_CHECK_PMT(radix<=36)) {
#ifndef _LIBCNT_
        *_errno() = EINVAL;
#endif
        return EINVAL;
    }

    pos = buffer+64;
    *pos = '\0';

    do {
        digit = value%radix;
        value /= radix;

        if(digit < 10)
            *--pos = '0'+digit;
        else
            *--pos = 'a'+digit-10;
    }while(value != 0);

    if((unsigned)(buffer-pos+65) > size) {
        MSVCRT_INVALID_PMT("str[size] is too small", EINVAL);
        return EINVAL;
    }

    memcpy(str, pos, buffer-pos+65);
    return 0;
}

/*
 * @implemented
 */
int CDECL _itoa_s(int value, char *str, size_t size, int radix)
{
    return _ltoa_s(value, str, size, radix);
}

/*
 * @implemented
 */
char *
CDECL
_itoa(int value, char *string, int radix)
{
  return _ltoa(value, string, radix);
}

/*
 * @implemented
 */
char *
CDECL
_ltoa(long value, char *string, int radix)
{
    unsigned long val;
    int negative;
    char buffer[33];
    char *pos;
    int digit;

    if (value < 0 && radix == 10) {
	negative = 1;
        val = -value;
    } else {
	negative = 0;
        val = value;
    } /* if */

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = val % radix;
	val = val / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (val != 0L);

    if (negative) {
	*--pos = '-';
    } /* if */

    memcpy(string, pos, &buffer[32] - pos + 1);
    return string;
}

/*
 * @implemented
 */
int CDECL _ltoa_s(long value, char *str, size_t size, int radix)
{
    unsigned long val;
    unsigned int digit;
    int is_negative;
    char buffer[33], *pos;
    size_t len;

    if (!MSVCRT_CHECK_PMT(str != NULL) || !MSVCRT_CHECK_PMT(size > 0) ||
        !MSVCRT_CHECK_PMT(radix >= 2) || !MSVCRT_CHECK_PMT(radix <= 36))
    {
        if (str && size)
            str[0] = '\0';

#ifndef _LIBCNT_
        *_errno() = EINVAL;
#endif
        return EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = 1;
        val = -value;
    }
    else
    {
        is_negative = 0;
        val = value;
    }

    pos = buffer + 32;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    }
    while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 33 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        MSVCRT_INVALID_PMT("str[size] is too small", ERANGE);
        return ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

/*
 * @implemented
 */
char *
CDECL
_ultoa(unsigned long value, char *string, int radix)
{
    char buffer[33];
    char *pos;
    int digit;

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    memcpy(string, pos, &buffer[32] - pos + 1);

    return string;
}
