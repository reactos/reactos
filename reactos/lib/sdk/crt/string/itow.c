/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/string/itow.c
 * PURPOSE:     converts a integer to wchar_t
 * PROGRAMER:
 * UPDATE HISTORY:
 */

#include <precomp.h>

/*
 * @implemented
 */
wchar_t *
CDECL
_i64tow(__int64 value, wchar_t *string, int radix)
{
    ULONGLONG val;
    int negative;
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

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
	digit = (WCHAR)(val % radix);
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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}

/*
 * @implemented
 */
int
CDECL
_i64tow_s(__int64 value, wchar_t *str, size_t size, int radix)
{
    unsigned __int64 val;
    unsigned int digit;
    int is_negative;
    wchar_t buffer[65], *pos;
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
        wchar_t *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 63, i = 0; i < size; i++)
            *p++ = *pos--;

        MSVCRT_INVALID_PMT("str[size] is too small", ERANGE);
        str[0] = '\0';
        return ERANGE;
    }

    memcpy(str, pos, len * sizeof(wchar_t));
    return 0;
}

/*
 * @implemented
 */
wchar_t *
CDECL
_ui64tow(unsigned __int64 value, wchar_t *string, int radix)
{
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = (WCHAR)(value % radix);
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    if (string != NULL) {
	memcpy(string, pos, (&buffer[64] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}

/*
 * @implemented
 */
int
CDECL
_ui64tow_s( unsigned __int64 value, wchar_t *str,
                             size_t size, int radix )
{
    wchar_t buffer[65], *pos;
    int digit;

    if (!MSVCRT_CHECK_PMT(str != NULL) || !MSVCRT_CHECK_PMT(size > 0) ||
        !MSVCRT_CHECK_PMT(radix>=2) || !MSVCRT_CHECK_PMT(radix<=36)) {
#ifndef _LIBCNT_
        *_errno() = EINVAL;
#endif
        return EINVAL;
    }

    pos = &buffer[64];
    *pos = '\0';

    do {
	digit = value % radix;
	value = value / radix;
	if (digit < 10)
	    *--pos = '0' + digit;
	else
	    *--pos = 'a' + digit - 10;
    } while (value != 0);

    if((size_t)(buffer-pos+65) > size) {
        MSVCRT_INVALID_PMT("str[size] is too small", EINVAL);
        return EINVAL;
    }

    memcpy(str, pos, buffer-pos+65);
    return 0;
}

/*
 * @implemented
 */
wchar_t *
CDECL
_itow(int value, wchar_t *string, int radix)
{
  return _ltow(value, string, radix);
}

/*
 * @implemented
 */
int
CDECL
_itow_s(int value, wchar_t *str, size_t size, int radix)
{
    return _ltow_s(value, str, size, radix);
}

/*
 * @implemented
 */
wchar_t *
CDECL
_ltow(long value, wchar_t *string, int radix)
{
    unsigned long val;
    int negative;
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

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
	digit = (WCHAR)(val % radix);
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

    if (string != NULL) {
	memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}

/*
 * @implemented
 */
int
CDECL
_ltow_s(long value, wchar_t *str, size_t size, int radix)
{
    unsigned long val;
    unsigned int digit;
    int is_negative;
    wchar_t buffer[33], *pos;
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
        wchar_t *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 31, i = 0; i < size; i++)
            *p++ = *pos--;

        MSVCRT_INVALID_PMT("str[size] is too small", ERANGE);
        str[0] = '\0';
        return ERANGE;
    }

    memcpy(str, pos, len * sizeof(wchar_t));
    return 0;
}

/*
 * @implemented
 */
wchar_t *
CDECL
_ultow(unsigned long value, wchar_t *string, int radix)
{
    WCHAR buffer[33];
    PWCHAR pos;
    WCHAR digit;

    pos = &buffer[32];
    *pos = '\0';

    do {
	digit = (WCHAR)(value % radix);
	value = value / radix;
	if (digit < 10) {
	    *--pos = '0' + digit;
	} else {
	    *--pos = 'a' + digit - 10;
	} /* if */
    } while (value != 0L);

    if (string != NULL) {
	memcpy(string, pos, (&buffer[32] - pos + 1) * sizeof(WCHAR));
    } /* if */
    return string;
}
