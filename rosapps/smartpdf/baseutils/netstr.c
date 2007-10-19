/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "tstr_util.h"
#include "netstr.h"

/* Implements djb idea of net strings */
/* Return the number of digits needed to represents a given number in base 10
   string representation.
*/
size_t digits_for_number(int num)
{
    size_t digits = 1;
    /* negative numbers need '-' in front of them */
    if (num < 0) {
        ++digits;
        num = -num;
    }

    while (num >= 10)
    {
        ++digits;
        num = num / 10;
    }
    return digits;
}

/* Netstring format is a safe, easy and mostly human readable format for
   serializing strings (well, any binary data). Netstring format is:
   - a byte length of the data as a string
   - ':' (single character)
   - data
   - ',' (single character)
   e.g. "foo" is encoded as "3:foo,"
   I learned about netstring format from djb (http://cr.yp.to/proto/netstrings.txt)
*/
size_t netstr_tstrn_serialized_len_cb(size_t str_len_cch)
{
    size_t total_len_cch;

    /* 2 is for ':" and ',' */
    total_len_cch = str_len_cch + digits_for_number((int)str_len_cch) + 2;
    return total_len_cch * sizeof(TCHAR);
}

/* Return number of bytes needed to serialize string 'str' in netstring format. */
size_t netstr_tstr_serialized_len_cb(const TCHAR *str)
{
    size_t str_len_cch;

    if (!str) return 0;
    str_len_cch = tstr_len(str);
    return netstr_tstrn_serialized_len_cb(str_len_cch);
}

/* Return number of bytes needed to serialize integer 'num' in netstring format. */
size_t netstr_int_serialized_len_cb(int num)
{
    size_t str_len_cch;
    size_t total_len_cch;

    str_len_cch = digits_for_number(num);
    total_len_cch = str_len_cch + digits_for_number((int)str_len_cch) + 2;
    return total_len_cch * sizeof(TCHAR);
}

int netstr_tstr_serialize(const TCHAR *str, TCHAR **buf_ptr, size_t *buf_len_cb_ptr)
{
    char *  buf;
    size_t  buf_len_cb;
    size_t  len_needed_cb;
    TCHAR * num_str;
    size_t  str_len_cch;
    size_t  len_cb;
    size_t  total_len_cb = 0;

    assert(buf_len_cb_ptr);
    if (!buf_len_cb_ptr)
        return FALSE;

    if (!buf_ptr)
    {
        *buf_len_cb_ptr += netstr_tstr_serialized_len_cb(str);
        return TRUE;
    }

    buf = (char*)*buf_ptr;
    assert(buf);
    if (!buf)
        return FALSE;

    buf_len_cb = *buf_len_cb_ptr;
    assert(buf_len_cb > 0);
    if (buf_len_cb <= 0)
        return FALSE;

    len_needed_cb = netstr_tstr_serialized_len_cb(str);
    if (len_needed_cb > buf_len_cb)
        return FALSE;

    str_len_cch = tstr_len(str);
    num_str = tstr_printf(_T("%d:"), str_len_cch);
    if (!num_str)
        return FALSE;

    len_cb = tstr_len(num_str)*sizeof(TCHAR);
    memcpy(buf, num_str, len_cb);
    buf += len_cb;
    total_len_cb += len_cb;
    assert(total_len_cb <= len_needed_cb);
    len_cb = tstr_len(str)*sizeof(TCHAR);
    memcpy(buf, str, len_cb);
    buf += len_cb;
    total_len_cb += len_cb;
    assert(total_len_cb <= len_needed_cb);
    len_cb = sizeof(TCHAR);
    memcpy(buf, _T(","), len_cb);
    buf += len_cb;
    total_len_cb += len_cb;
    assert(total_len_cb == len_needed_cb);

    *buf_len_cb_ptr -= total_len_cb;
    *buf_ptr = (TCHAR*)buf;
    free((void*)num_str);
    return TRUE;
}

int netstr_int_serialize(int num, TCHAR **buf_ptr, size_t *buf_len_cb_ptr)
{
    TCHAR * num_str;
    int     f_ok;

    assert(buf_len_cb_ptr);
    if (!buf_len_cb_ptr)
        return FALSE;

    if (!buf_ptr)
    {
        *buf_len_cb_ptr += netstr_int_serialized_len_cb(num);
        return TRUE;
    }

    num_str = tstr_printf(_T("%d"), num);
    if (!num_str)
        return FALSE;

    f_ok = netstr_tstr_serialize(num_str, buf_ptr, buf_len_cb_ptr);
    free((void*)num_str);
    return f_ok;
}

/* Parse a netstring number i.e. a list of digits until ':', skipping ':'.
   Returns FALSE if there's an error parsing (string doesn't follow the format) */
static int netstr_get_str_len(const TCHAR **str_ptr, size_t *str_len_cb_ptr, int *num_out)
{
    int             num = 0;
    const TCHAR *   tmp;
    size_t          str_len_cb;
    TCHAR           c;
    int             digit = 0;

    assert(str_ptr);
    if (!str_ptr)
        return FALSE;
    assert(str_len_cb_ptr);
    if (!str_len_cb_ptr)
        return FALSE;
    assert(num_out);
    if (!num_out)
        return FALSE;

    tmp = *str_ptr;
    assert(tmp);
    if (!tmp)
        return FALSE;

    str_len_cb = *str_len_cb_ptr;
    assert(str_len_cb > 0);
    if (str_len_cb <= 0)
        return FALSE;

    for (;;) {
        str_len_cb -= sizeof(TCHAR);
        if (str_len_cb < 0)
            return FALSE;
        c = *tmp++;
        if (_T(':') == c)
            break;
        if ( (c >= _T('0')) && (c <= _T('9')) )
            digit = (int)c - _T('0');
        else
            return FALSE;
        num = (num * 10) + digit;
    }
    if (str_len_cb == *str_len_cb_ptr)
        return FALSE;

    *str_ptr = tmp;
    *str_len_cb_ptr = str_len_cb;
    *num_out = num;
    return TRUE;
}

int netstr_valid_separator(TCHAR c)
{
    if (c == _T(','))
        return TRUE;
    return FALSE;
}

int netstr_parse_str(const TCHAR **str_ptr, size_t *str_len_cb_ptr, const TCHAR **str_out, size_t *str_len_cch_out)
{
    int             f_ok;
    size_t          str_len_cch;
    size_t          str_len_cb;
    const TCHAR *   str;
    const TCHAR *   str_copy;
    int             num;

    f_ok = netstr_get_str_len(str_ptr, str_len_cb_ptr, &num);
    if (!f_ok)
        return FALSE;
    assert(num >= 0);
    str_len_cch = (size_t)num;
    str_len_cb = (str_len_cch+1)*sizeof(TCHAR);
    if (str_len_cb > *str_len_cb_ptr)
        return FALSE;

    str = *str_ptr;
    if (!netstr_valid_separator(str[str_len_cch]))
        return FALSE;
    str_copy = (const TCHAR*)tstr_dupn(str, str_len_cch);
    if (!str_copy)
        return FALSE;
    *str_out = str_copy;
    *str_len_cch_out = str_len_cch;
    *str_ptr = str + str_len_cch + 1;
    *str_len_cb_ptr -= str_len_cb ;
    return TRUE;
}

int netstr_parse_int(const TCHAR **str_ptr, size_t *str_len_cb_ptr, int *int_out)
{
    const TCHAR *   str = NULL;
    const TCHAR *   tmp;
    TCHAR           c;
    size_t          str_len_cch;
    int             f_ok;
    int             num = 0;
    int             digit = 0;

    f_ok = netstr_parse_str(str_ptr, str_len_cb_ptr, &str, &str_len_cch);
    if (!f_ok)
        return FALSE;

    tmp = str;
    while (*tmp) {
        c = *tmp++;
        if ( (c >= _T('0')) && (c <= _T('9')) )
            digit = (int)c - _T('0');
        else
            goto Error;
        num = (num * 10) + digit;
    }
    *int_out = num;
    free((void*)str);
    return TRUE;
Error:
    free((void*)str);
    return FALSE;
}
