/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "tstr_util.h"
#include "prefs_util.h"
#include "netstr.h"

/* length of PT_*_PREFIX string in characters. All should have the same length */
#define TYPE_PREFIX_CCH_LEN 2

/* when we serialize names of variables, we prepend name with the following
   type indentifiers */
#define PT_INT_PREFIX       _T("i ")
#define PT_STRING_PREFIX    _T("s ")

static int pref_type_valid(pref_type type)
{
    if (PT_INT == type)
        return TRUE;
    if (PT_STRING == type)
        return TRUE;
    return FALSE;
}

/* Given a string value 'txt' and it's type 'type', return a string that
   encodes type within a string */
TCHAR *pref_tstr_with_type(const TCHAR *txt, pref_type type)
{
    if (PT_INT == type)
        return tstr_cat(PT_INT_PREFIX, txt);
    else if (PT_STRING == type)
        return tstr_cat(PT_STRING_PREFIX, txt);
    else
        assert(0);
    return NULL;
}

/* Serialize 'pref' to a buffer 'buf_ptr' of size 'buf_len_cb_ptr'.
   Return TRUE if ok, FALSE if failed (e.g. buffer is not large enough).
   If 'buf_ptr' is NULL, returns desired size in 'buf_len_cb_ptr'.
   */
static int prefs_serialize_pref(prefs_data *pref, TCHAR **buf_ptr, size_t *buf_len_cb_ptr)
{
    size_t  len_cb;
    TCHAR * name_with_type;
    int     f_ok;

    assert(pref);
    assert(pref->name);
    assert(pref_type_valid(pref->type));
    assert(buf_len_cb_ptr);

    if (!buf_ptr) {
        len_cb  = netstr_tstrn_serialized_len_cb(TYPE_PREFIX_CCH_LEN + tstr_len(pref->name));
        if (PT_INT == pref->type)
            len_cb += netstr_int_serialized_len_cb(*pref->data.data_int);
        else if (PT_STRING == pref->type)
            len_cb += netstr_tstr_serialized_len_cb(*pref->data.data_str);
        else
            assert(0);
        *buf_len_cb_ptr = len_cb;
        return TRUE;
    }

    name_with_type = pref_tstr_with_type(pref->name, pref->type);
    if (!name_with_type) return FALSE;
    f_ok = netstr_tstr_serialize(name_with_type, buf_ptr, buf_len_cb_ptr);
    free((void*)name_with_type);
    if (!f_ok)
        return FALSE;

    if (PT_INT == pref->type)
        f_ok = netstr_int_serialize(*pref->data.data_int, buf_ptr, buf_len_cb_ptr);
    else if (PT_STRING == pref->type)
        f_ok = netstr_tstr_serialize(*pref->data.data_str, buf_ptr, buf_len_cb_ptr);
    else
        assert(0);

    if (!f_ok)
        return FALSE;

    return TRUE;
}

/* Return the size of memory required to serialize 'pref' data */
static size_t prefs_serialized_pref_cb_len(prefs_data *pref)
{
    int     f_ok;
    size_t  len;

    f_ok = prefs_serialize_pref(pref, NULL, &len);
    if (!f_ok)
        return 0;
    return len;
}

/* Serialize 'prefs' as string. Returns newly allocated string and
   length, in bytes, of string in '*tstr_len_cb_ptr' (not including
   terminating zero). 'tstr_len_cb_ptr' can be NULL.
   Returns NULL on error.
   Caller needs to free() the result */
TCHAR *prefs_to_tstr(prefs_data *prefs, size_t *tstr_len_cb_ptr)
{
    int         i = 0;
    size_t      total_serialized_len_cb = 0;
    size_t      len_cb;
    int         f_ok;
    TCHAR *     serialized = NULL;
    TCHAR *     tmp;
    size_t      tmp_len_cb;

    /* calculate the size of buffer required to serialize 'prefs' */
    while (prefs[i].name) {
        len_cb = prefs_serialized_pref_cb_len(&(prefs[i]));
        assert(len_cb > 0);
        total_serialized_len_cb += len_cb;
        ++i;
    }

    if (0 == total_serialized_len_cb)
        return NULL;

    /* allocate the buffer and serialize to it */
    serialized = (TCHAR*)malloc(total_serialized_len_cb+sizeof(TCHAR));
    if (!serialized) return NULL;
    tmp = serialized;
    tmp_len_cb = total_serialized_len_cb;
    i = 0;
    while (prefs[i].name) {
        f_ok = prefs_serialize_pref(&(prefs[i]), &tmp, &tmp_len_cb);
        assert(f_ok);
        assert(tmp_len_cb >= 0);
        ++i;
    }
    assert(0 == tmp_len_cb);
    *tmp = 0;
    if (tstr_len_cb_ptr)
        *tstr_len_cb_ptr = total_serialized_len_cb;
    return serialized;
}

/* Find a variable with a given 'name' and 'type' in 'prefs' array */
prefs_data *prefs_find_by_name_type(prefs_data *prefs, const TCHAR *name, pref_type type)
{
    int     i = 0;
    while (prefs[i].name) {
        if ((prefs[i].type == type) && (tstr_ieq(name, prefs[i].name))) {
            return &(prefs[i]);
        }
        ++i;
    }
    return NULL;
}

/* Incrementally parse one serialized variable in a string '*str_ptr' of
   remaining size '*str_len_cb_ptr'.
   It reads name, type and value of the variable from the string and
   updates 'prefs' slot with this name/type with this value. If slot
   with a given name/type doesn't exist, nothing happens.
   It updates the '*str_ptr' and '*str_len_cb_ptr' to reflect consuming
   the part that contained one variable. The idea is to call it in a
   loop until '*str_len_cb_ptr' reaches 0.
   Returns FALSE on error. */
static int prefs_parse_item(prefs_data *prefs, const TCHAR **str_ptr, size_t *str_len_cb_ptr)
{
    const TCHAR *   name_with_type = NULL;
    const TCHAR *   name;
    size_t          str_len_cch;
    const TCHAR *   value_str = NULL;
    int             value_int;
    int             f_ok;
    pref_type       type;
    prefs_data *    pref;

    assert(str_ptr);
    if (!str_ptr) return FALSE;
    assert(str_len_cb_ptr);
    if (!str_len_cb_ptr) return FALSE;

    f_ok = netstr_parse_str(str_ptr, str_len_cb_ptr, &name_with_type, &str_len_cch);
    if (!f_ok)
        goto Error;

    if (tstr_startswithi(name_with_type, PT_INT_PREFIX))
        type = PT_INT;
    else if (tstr_startswithi(name_with_type, PT_STRING_PREFIX))
        type = PT_STRING;
    else {
        assert(0);
        goto Error;
    }
    /* skip the type prefix */
    name = name_with_type + TYPE_PREFIX_CCH_LEN;

    pref = prefs_find_by_name_type(prefs, name, type);

    if (PT_STRING == type)
        f_ok = netstr_parse_str(str_ptr, str_len_cb_ptr, &value_str, &str_len_cch);
    else if (PT_INT == type)
        f_ok = netstr_parse_int(str_ptr, str_len_cb_ptr, &value_int);
    else {
        assert(0);
        goto Error;
    }
    if (!f_ok)
        goto Error;

    if (!pref) {
        /* it's ok to not have a given preference e.g. when changing version some of the
           preferences might go away. But we still want to be notified about that during
           developement, since it's unlikely thing to happen */
        assert(0);
        goto Exit;
    }

    if (PT_INT == type)
        *pref->data.data_int = value_int;
    else if (PT_STRING == type) {
        /* taking memory ownership */
        *pref->data.data_str = (TCHAR*)value_str;
        value_str = NULL;
    } else {
        assert(0);
        goto Error;
    }

Exit:
    free((void*)name_with_type);
    free((void*)value_str);
    return TRUE;
Error:
    free((void*)name_with_type);
    free((void*)value_str);
    return FALSE;
}

int prefs_from_tstr(prefs_data *prefs, const TCHAR *str, size_t str_len_cch)
{
    int     f_ok;
    size_t  str_len_cb;

    assert(str);
    if (!str) return FALSE;

    if (-1 == str_len_cch)
        str_len_cch = tstr_len(str);

    str_len_cb = str_len_cch * sizeof(TCHAR);
    while (0 != str_len_cb) {
        f_ok = prefs_parse_item(prefs, &str, &str_len_cb);
        if (!f_ok)
            return FALSE;
    }
    assert(0 == str_len_cb);
    return TRUE;
}
