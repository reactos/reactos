/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/*
 * It is impossible to write the C++ locale library in terms of locales
 * as defined in the C standard.  Instead, we write the C++ locale and I/O
 * library in terms of a low level C-like interface.  This file defines
 * that interface.
 *
 * The low-level locale interface can't be written portably; there
 * must be a version of it for each platform that the C++ library
 * is ported to.  On many systems this interface may be a thin wrapper
 * for existing functionality.
 */

#ifndef _STLP_C_LOCALE_IMPL_H
#define _STLP_C_LOCALE_IMPL_H

#include "stlport_prefix.h"

#include <wchar.h> /* for mbstate_t */
#include <stl/c_locale.h>

struct _Locale_name_hint;

#if defined (_GNU_SOURCE) && defined (__GLIBC__) && \
    ((__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2))
#  define _STLP_USE_GLIBC2_LOCALIZATION
#  include <nl_types.h>
typedef nl_catd nl_catd_type;
#else
typedef int nl_catd_type;
#endif

/*
 * A number: the maximum length of a simple locale name.
 * (i.e. a name like like en_US, as opposed to a name like
 * en_US/de_AT/de_AT/es_MX/en_US/en_US) */
#define _Locale_MAX_SIMPLE_NAME 256

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Typedefs:
 */
typedef unsigned short int _Locale_mask_t;

/* Function called during STLport library load phase. Might contain any
 * code necessary to the platform localization layer.
 */
void _Locale_init(void);

/* Function called during STLport library unload. Might contain any
 * code necessary to the platform localization layer.
 */
void _Locale_final(void);

/* Create a category of the locale with the given name.
 *
 * The char* argument is a simple (not a composite) locale name, which may
 * neither be an empty string nor a null pointer.
 *
 * These functions return NULL to indicate failure. Failure reason should be reported
 * using the __err_code pointer.
 */
struct _Locale_ctype* _Locale_ctype_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_codecvt* _Locale_codecvt_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_numeric* _Locale_numeric_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_time* _Locale_time_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_collate* _Locale_collate_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_monetary* _Locale_monetary_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);
struct _Locale_messages* _Locale_messages_create(const char *, struct _Locale_name_hint*, int * /* __err_code */);

/* Give error reason on failure of one of the _Locale_*_create functions. Available
 * reasons are:
 * 0: No specific error reason has been reported.
 * 1: No platform support for the given facet.
 * 2: Unknown locale name
 * 3: No platform API for localization support.
 * 4: No more memory
 */
#define _STLP_LOC_UNDEFINED 0
#define _STLP_LOC_UNSUPPORTED_FACET_CATEGORY 1
#define _STLP_LOC_UNKNOWN_NAME 2
#define _STLP_LOC_NO_PLATFORM_SUPPORT 3
#define _STLP_LOC_NO_MEMORY 4

/* Release a category of a locale
 *
 * These functions are used to release a category acquired with the
 * according _Locale_*_create() functions.
 */
void _Locale_ctype_destroy(struct _Locale_ctype *);
void _Locale_codecvt_destroy(struct _Locale_codecvt *);
void _Locale_numeric_destroy(struct _Locale_numeric *);
void _Locale_time_destroy(struct _Locale_time *);
void _Locale_collate_destroy(struct _Locale_collate *);
void _Locale_monetary_destroy(struct _Locale_monetary *);
void _Locale_messages_destroy(struct _Locale_messages *);

/*
 * Returns the name of the user's default locale in each
 * category, as a null-terminated string.  A NULL value
 * means the default "C" locale.
 */
const char * _Locale_ctype_default(char * __buf);
const char * _Locale_numeric_default(char * __buf);
const char * _Locale_time_default(char * __buf);
const char * _Locale_collate_default(char * __buf);
const char * _Locale_monetary_default(char * __buf);
const char * _Locale_messages_default(char * __buf);

/* Retrieve the name of the given category
 *
 * __buf points to a buffer that can hold at least _Locale_MAX_SIMPLE_NAME
 * characters.  These functions store the name, as a null-terminated
 * string, in __buf. This function can't fail, at worst name is truncated.
 */
char const* _Locale_ctype_name(const struct _Locale_ctype *, char* __buf);
char const* _Locale_codecvt_name(const struct _Locale_codecvt *, char* __buf);
char const* _Locale_numeric_name(const struct _Locale_numeric *, char* __buf);
char const* _Locale_time_name(const struct _Locale_time *, char* __buf);
char const* _Locale_collate_name(const struct _Locale_collate *, char*  __buf);
char const* _Locale_monetary_name(const struct _Locale_monetary *, char* __buf);
char const* _Locale_messages_name(const struct _Locale_messages *, char* __buf);

/*
 * cname is a (possibly composite) locale name---i.e. a name that can
 * be passed to setlocale. __buf points to an array large enough to
 * store at least _Locale_MAX_SIMPLE_NAME characters, and each of these
 * functions extracts the name of a single category, stores it in buf
 * as a null-terminated string, and returns buf.
 */
char const* _Locale_extract_ctype_name(const char *cname, char *__buf,
                                       struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_numeric_name(const char *cname, char *__buf,
                                         struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_time_name(const char *cname, char *__buf,
                                      struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_collate_name(const char *cname, char *__buf,
                                         struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_monetary_name(const char *cname, char *__buf,
                                          struct _Locale_name_hint* __hint, int *__err_code);
char const* _Locale_extract_messages_name(const char *cname, char *__buf,
                                          struct _Locale_name_hint* __hint, int *__err_code);

/* Functions to improve locale creation process. For some locale API (Win32)
 * you need to find a locale identification from the name which can be a
 * rather expensive operation especially if you do so for all facets of a
 * locale. Those functions can be used to extract from a API dependent facet
 * struct the information necessary to skip this lookup process for other
 * facets creation. If not supported those function should return NULL.
 */
struct _Locale_name_hint* _Locale_get_ctype_hint(struct _Locale_ctype*);
struct _Locale_name_hint* _Locale_get_numeric_hint(struct _Locale_numeric*);
struct _Locale_name_hint* _Locale_get_time_hint(struct _Locale_time*);
struct _Locale_name_hint* _Locale_get_collate_hint(struct _Locale_collate*);
struct _Locale_name_hint* _Locale_get_monetary_hint(struct _Locale_monetary*);
struct _Locale_name_hint* _Locale_get_messages_hint(struct _Locale_messages*);

/*
 * FUNCTIONS THAT USE CTYPE
 */

/*
 * Narrow character functions:
 */

/*
 * Returns a pointer to the beginning of the ctype table.  The table is
 * at least 257 bytes long; if p is the pointer returned by this
 * function, then p[c] is valid if c is EOF or if p is any value of
 * type unsigned char.
 */
const _Locale_mask_t * _Locale_ctype_table(struct _Locale_ctype *);

/*
 * c is either EOF, or an unsigned char value.
 */
int _Locale_toupper(struct _Locale_ctype *, int /* c */);
int _Locale_tolower(struct _Locale_ctype *, int /* c */);


#ifndef _STLP_NO_WCHAR_T
/*
 * Wide character functions:
 */
_Locale_mask_t _WLocale_ctype(struct _Locale_ctype *, wint_t, _Locale_mask_t);
wint_t _WLocale_tolower(struct _Locale_ctype *, wint_t);
wint_t _WLocale_toupper(struct _Locale_ctype *, wint_t);

/*
 * Multibyte functions:
 */

/*
 * Returns the number of bytes of the longest allowed multibyte
 * character in the current encoding.
 */
int _WLocale_mb_cur_max(struct _Locale_codecvt *);

/*
 * Returns the number of bytes of the shortest allowed multibyte
 * character in the current encoding.
 */
int _WLocale_mb_cur_min(struct _Locale_codecvt *);

/*
 * Returns 1 if the current multibyte encoding is stateless
 * and does not require the use of an mbstate_t value.
 */
int _WLocale_is_stateless(struct _Locale_codecvt *);

/*
 * Almost identical to mbrtowc, from 4.6.5.3.2 of NA1.  The only
 * important difference is that mbrtowc treats null wide characters
 * as special, and we don't.  Specifically: examines the characters
 * in [from, from + n), extracts a single wide character, and stores
 * it in *to.  Modifies shift_state if appropriate.  The return value,
 * which is always positive, is the number of characters extracted from
 * the input sequence.  Return value is (size_t) -1 if there was an
 * encoding error in the input sequence, and (size_t) -2 if
 * [from, from + n) is correct but not complete.  None of the pointer
 * arguments may be null pointers.
 */
size_t _WLocale_mbtowc(struct _Locale_codecvt *,
                       wchar_t * /* to */,
                       const char * /* from */, size_t /* n */,
                       mbstate_t *);

/*
 * Again, very similar to wcrtomb.  The differences are that (1) it
 * doesn't treat null characters as special; and (2) it stores at most
 * n characters.  Converts c to a multibyte sequence, stores that
 * sequence in the array 'to', and returns the length of the sequence.
 * Modifies shift_state if appropriate.  The return value is (size_t) -1
 * if c is not a valid wide character, and (size_t) -2 if the length of
 * the multibyte character sequence is greater than n.
 */
size_t _WLocale_wctomb(struct _Locale_codecvt *,
                       char *, size_t,
                       const wchar_t,
                       mbstate_t *);

/*
 * Inserts whatever characters are necessary to restore st to an
 * initial shift state.  Sets *next to buf + m, where m is the number
 * of characters inserted.  (0 <= m <= n.)  Returns m to indicate
 * success, (size_t) -1 to indicate error, (size_t) -2 to indicate
 * partial success (more than n characters needed).  For success or partial
 * success, sets *next to buf + m.
 */
size_t _WLocale_unshift(struct _Locale_codecvt *,
                        mbstate_t *,
                        char *, size_t, char **);
#endif

/*
 * FUNCTIONS THAT USE COLLATE
 */

/*
 * Compares the two sequences [s1, s1 + n1) and [s2, s2 + n2).  Neither
 * sequence is assumed to be null-terminated, and null characters
 * aren't special.  If the two sequences are the same up through
 * min(n1, n2), then the sequence that compares less is whichever one
 * is shorter.
 */
int _Locale_strcmp(struct _Locale_collate *,
                   const char * /* s1 */, size_t /* n1 */,
                   const char * /* s2 */, size_t /* n2 */);
#ifndef _STLP_NO_WCHAR_T
int _WLocale_strcmp(struct _Locale_collate *,
                    const wchar_t * /* s1 */, size_t /* n1 */,
                    const wchar_t * /* s2 */, size_t /* n2 */);
#endif

/*
 * Creates a transformed version of the string [s2, s2 + n2).  The
 * string may contain embedded null characters; nulls aren't special.
 * The transformed string begins at s1, and contains at most n1
 * characters.  The return value is the length of the transformed
 * string.  If the return value is greater than n1 then this is an
 * error condition: it indicates that there wasn't enough space.  In
 * that case, the contents of [s1, s1 + n1) is unspecified.
*/
size_t _Locale_strxfrm(struct _Locale_collate *,
                       char * /* s1 */, size_t /* n1 */,
                       const char * /* s2 */, size_t /* n2 */);

#ifndef _STLP_NO_WCHAR_T
size_t _WLocale_strxfrm(struct _Locale_collate *,
                        wchar_t * /* s1 */, size_t /* n1 */,
                        const wchar_t * /* s2 */, size_t /* n2 */);
#endif


/*
 * FUNCTIONS THAT USE NUMERIC
 */

/*
 * Equivalent to the first three fields in struct lconv.  (C standard,
 * section 7.4.)
 */
char _Locale_decimal_point(struct _Locale_numeric *);
char _Locale_thousands_sep(struct _Locale_numeric *);
const char * _Locale_grouping(struct _Locale_numeric *);

#ifndef _STLP_NO_WCHAR_T
wchar_t _WLocale_decimal_point(struct _Locale_numeric *);
wchar_t _WLocale_thousands_sep(struct _Locale_numeric *);
#endif

/*
 * Return "true" and "false" in English locales, and something
 * appropriate in non-English locales.
 */
const char * _Locale_true(struct _Locale_numeric *);
const char * _Locale_false(struct _Locale_numeric *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_true(struct _Locale_numeric *, wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_false(struct _Locale_numeric *, wchar_t* /* buf */, size_t /* bufSize */);
#endif

/*
 * FUNCTIONS THAT USE MONETARY
 */

/*
 * Return the obvious fields of struct lconv.
 */
const char * _Locale_int_curr_symbol(struct _Locale_monetary *);
const char * _Locale_currency_symbol(struct _Locale_monetary *);
char         _Locale_mon_decimal_point(struct _Locale_monetary *);
char         _Locale_mon_thousands_sep(struct _Locale_monetary *);
const char * _Locale_mon_grouping(struct _Locale_monetary *);
const char * _Locale_positive_sign(struct _Locale_monetary *);
const char * _Locale_negative_sign(struct _Locale_monetary *);
char         _Locale_int_frac_digits(struct _Locale_monetary *);
char         _Locale_frac_digits(struct _Locale_monetary *);
int          _Locale_p_cs_precedes(struct _Locale_monetary *);
int          _Locale_p_sep_by_space(struct _Locale_monetary *);
int          _Locale_p_sign_posn(struct _Locale_monetary *);
int          _Locale_n_cs_precedes(struct _Locale_monetary *);
int          _Locale_n_sep_by_space(struct _Locale_monetary *);
int          _Locale_n_sign_posn(struct _Locale_monetary *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_int_curr_symbol(struct _Locale_monetary *, wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_currency_symbol(struct _Locale_monetary *, wchar_t* /* buf */, size_t /* bufSize */);
wchar_t         _WLocale_mon_decimal_point(struct _Locale_monetary *);
wchar_t         _WLocale_mon_thousands_sep(struct _Locale_monetary *);
const wchar_t * _WLocale_positive_sign(struct _Locale_monetary *, wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_negative_sign(struct _Locale_monetary *, wchar_t* /* buf */, size_t /* bufSize */);
#endif

/*
 * FUNCTIONS THAT USE TIME
 */

/*
 * month is in the range [0, 12).
 */
const char * _Locale_full_monthname(struct _Locale_time *, int /* month */);
const char * _Locale_abbrev_monthname(struct _Locale_time *, int /* month */);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_full_monthname(struct _Locale_time *, int /* month */,
                                        wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_abbrev_monthname(struct _Locale_time *, int /* month */,
                                          wchar_t* /* buf */, size_t /* bufSize */);
#endif

/*
 * day is in the range [0, 7).  Sunday is 0.
 */
const char * _Locale_full_dayofweek(struct _Locale_time *, int /* day */);
const char * _Locale_abbrev_dayofweek(struct _Locale_time *, int /* day */);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_full_dayofweek(struct _Locale_time *, int /* day */,
                                        wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_abbrev_dayofweek(struct _Locale_time *, int /* day */,
                                          wchar_t* /* buf */, size_t /* bufSize */);
#endif

const char * _Locale_d_t_fmt(struct _Locale_time *);
const char * _Locale_d_fmt(struct _Locale_time *);
const char * _Locale_t_fmt(struct _Locale_time *);
const char * _Locale_long_d_t_fmt(struct _Locale_time*);
const char * _Locale_long_d_fmt(struct _Locale_time*);

const char * _Locale_am_str(struct _Locale_time *);
const char * _Locale_pm_str(struct _Locale_time *);

#ifndef _STLP_NO_WCHAR_T
const wchar_t * _WLocale_am_str(struct _Locale_time *,
                                wchar_t* /* buf */, size_t /* bufSize */);
const wchar_t * _WLocale_pm_str(struct _Locale_time *,
                                wchar_t* /* buf */, size_t /* bufSize */);
#endif

/*
 * FUNCTIONS THAT USE MESSAGES
 */

/*
 * Very similar to catopen, except that it uses the given message
 * category to determine which catalog to open.
 */
nl_catd_type _Locale_catopen(struct _Locale_messages*, const char*);

/* Complementary to _Locale_catopen.
 * The catalog must be a value that was returned by a previous call
 * to _Locale_catopen.
 */
void _Locale_catclose(struct _Locale_messages*, nl_catd_type);

/*
 * Returns a string, identified by a set index and a message index,
 * from an opened message catalog.  Returns the supplied default if
 * no such string exists.
 */
const char * _Locale_catgets(struct _Locale_messages *, nl_catd_type,
                             int, int,const char *);

#ifdef __cplusplus
}
#endif

#endif /* _STLP_C_LOCALE_IMPL_H */
