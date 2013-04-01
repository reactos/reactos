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
#include "stlport_prefix.h"

#include <hash_map>
#include <vector>

#include <locale>
#include <istream>

#include <algorithm>
#include <functional>

#include "c_locale.h"
#include "locale_impl.h"
#include "acquire_release.h"

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// ctype_byname<char>

#if defined (__DMC__)
_STLP_DECLSPEC
#endif
ctype_byname<char>::ctype_byname(const char* name, size_t refs)
    : ctype<char>( 0, false, refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_ctype = _STLP_PRIV __acquire_ctype(name, buf, 0, &__err_code);
  if (!_M_ctype)
    locale::_M_throw_on_creation_failure(__err_code, name, "ctype");

  _M_init();
}

void ctype_byname<char>::_M_init() {
  _M_ctype_table = _M_byname_table;

  // We have to do this, instead of just pointer twiddling, because
  // ctype_base::mask isn't the same type as _Locale_mask_t.
  const _Locale_mask_t* p = _Locale_ctype_table(_M_ctype);
  for (size_t i = 0; i != table_size; ++i) {
    _M_byname_table[i] = ctype_base::mask(p[i]);
  }
}

ctype_byname<char>::~ctype_byname()
{ _STLP_PRIV __release_ctype(_M_ctype); }

char ctype_byname<char>::do_toupper(char c) const
{ return (char)_Locale_toupper(_M_ctype, c); }

char ctype_byname<char>::do_tolower(char c) const
{ return (char)_Locale_tolower(_M_ctype, c); }

const char*
ctype_byname<char>::do_toupper(char* first, const char* last) const {
  for ( ; first != last ; ++first)
    *first = (char)_Locale_toupper(_M_ctype, *first);
  return last;
}

const char*
ctype_byname<char>::do_tolower(char* first, const char* last) const {
  for ( ; first != last ; ++first)
    *first = (char)_Locale_tolower(_M_ctype, *first);
  return last;
}


// Some helper functions used in ctype<>::scan_is and scan_is_not.
#if !defined (_STLP_NO_WCHAR_T)

_STLP_MOVE_TO_PRIV_NAMESPACE

// ctype_byname<wchar_t>

struct _Ctype_byname_w_is_mask : public unary_function<wchar_t, bool> {
  _Locale_mask_t M;
  _Locale_ctype* M_ctp;

  _Ctype_byname_w_is_mask(_Locale_mask_t m, _Locale_ctype* c)
    : M(m), M_ctp(c) {}
  bool operator()(wchar_t c) const
  { return _WLocale_ctype(M_ctp, c, M) != 0; }
};

_STLP_MOVE_TO_STD_NAMESPACE

#if defined (__DMC__)
_STLP_DECLSPEC
#endif
ctype_byname<wchar_t>::ctype_byname(const char* name, size_t refs)
  : ctype<wchar_t>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_ctype = _STLP_PRIV __acquire_ctype(name, buf, 0, &__err_code);
  if (!_M_ctype)
    locale::_M_throw_on_creation_failure(__err_code, name, "ctype");
}

ctype_byname<wchar_t>::~ctype_byname()
{ _STLP_PRIV __release_ctype(_M_ctype); }

bool ctype_byname<wchar_t>::do_is(ctype_base::mask  m, wchar_t c) const
{ return _WLocale_ctype(_M_ctype, c, (_Locale_mask_t)m) != 0; }

const wchar_t*
ctype_byname<wchar_t>::do_is(const wchar_t* low, const wchar_t* high,
                             ctype_base::mask * m) const {
  _Locale_mask_t all_bits = _Locale_mask_t(ctype_base::space |
                                           ctype_base::print |
                                           ctype_base::cntrl |
                                           ctype_base::upper |
                                           ctype_base::lower |
                                           ctype_base::alpha |
                                           ctype_base::digit |
                                           ctype_base::punct |
                                           ctype_base::xdigit);

  for ( ; low < high; ++low, ++m)
    *m = ctype_base::mask (_WLocale_ctype(_M_ctype, *low, all_bits));
  return high;
}

const wchar_t*
ctype_byname<wchar_t>
  ::do_scan_is(ctype_base::mask  m, const wchar_t* low, const wchar_t* high) const
{ return find_if(low, high, _STLP_PRIV _Ctype_byname_w_is_mask(m, _M_ctype)); }

const wchar_t*
ctype_byname<wchar_t>
  ::do_scan_not(ctype_base::mask  m, const wchar_t* low, const wchar_t* high) const
{ return find_if(low, high, not1(_STLP_PRIV _Ctype_byname_w_is_mask(m, _M_ctype))); }

wchar_t ctype_byname<wchar_t>::do_toupper(wchar_t c) const
{ return _WLocale_toupper(_M_ctype, c); }

const wchar_t*
ctype_byname<wchar_t>::do_toupper(wchar_t* low, const wchar_t* high) const {
  for ( ; low < high; ++low)
    *low = _WLocale_toupper(_M_ctype, *low);
  return high;
}

wchar_t ctype_byname<wchar_t>::do_tolower(wchar_t c) const
{ return _WLocale_tolower(_M_ctype, c); }

const wchar_t*
ctype_byname<wchar_t>::do_tolower(wchar_t* low, const wchar_t* high) const {
  for ( ; low < high; ++low)
    *low = _WLocale_tolower(_M_ctype, *low);
  return high;
}

#endif /* WCHAR_T */

// collate_byname<char>
#if defined (__DMC__)
_STLP_DECLSPEC
#endif
collate_byname<char>::collate_byname(const char* name, size_t refs)
  : collate<char>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_collate = _STLP_PRIV __acquire_collate(name, buf, 0, &__err_code);
  if (!_M_collate)
    locale::_M_throw_on_creation_failure(__err_code, name, "collate");
}

collate_byname<char>::~collate_byname()
{ _STLP_PRIV __release_collate(_M_collate); }

int collate_byname<char>::do_compare(const char* __low1,
                                     const char* __high1,
                                     const char* __low2,
                                     const char* __high2) const {
  return _Locale_strcmp(_M_collate,
                        __low1, __high1 - __low1,
                        __low2, __high2 - __low2);
}

collate_byname<char>::string_type
collate_byname<char>::do_transform(const char* low, const char* high) const {
  if (low == high)
    return string_type();

  size_t n = _Locale_strxfrm(_M_collate, NULL, 0, low, high - low);

  // NOT PORTABLE.  What we're doing relies on internal details of the
  // string implementation.  (Contiguity of string elements and presence
  // of trailing zero.)
  string_type buf(n, 0);
  _Locale_strxfrm(_M_collate, &(*buf.begin()), n + 1, low, high - low);
  return buf;
}


#if !defined (_STLP_NO_WCHAR_T)

// collate_byname<wchar_t>

#if defined (__DMC__)
_STLP_DECLSPEC
#endif
collate_byname<wchar_t>::collate_byname(const char* name, size_t refs)
  : collate<wchar_t>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_collate = _STLP_PRIV __acquire_collate(name, buf, 0, &__err_code);
  if (!_M_collate)
    locale::_M_throw_on_creation_failure(__err_code, name, "collate");
}

collate_byname<wchar_t>::~collate_byname()
{ _STLP_PRIV __release_collate(_M_collate); }

int collate_byname<wchar_t>::do_compare(const wchar_t* low1,
                                        const wchar_t* high1,
                                        const wchar_t* low2,
                                        const wchar_t* high2) const {
  return _WLocale_strcmp(_M_collate,
                         low1, high1 - low1,
                         low2, high2 - low2);
}

collate_byname<wchar_t>::string_type
collate_byname<wchar_t>::do_transform(const wchar_t* low,
                                      const wchar_t* high) const {
  if (low == high)
    return string_type();

  size_t n = _WLocale_strxfrm(_M_collate, NULL, 0, low, high - low);

  // NOT PORTABLE.  What we're doing relies on internal details of the
  // string implementation.  (Contiguity of string elements and presence
  // of trailing zero.)
  string_type buf(n, 0);
  _WLocale_strxfrm(_M_collate, &(*buf.begin()), n + 1, low, high - low);
  return buf;
}

#endif /*  _STLP_NO_WCHAR_T */

//----------------------------------------------------------------------
// codecvt_byname<char>

codecvt_byname<char, char, mbstate_t>
  ::codecvt_byname(const char* name, size_t refs)
    : codecvt<char, char, mbstate_t>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();
}

codecvt_byname<char, char, mbstate_t>::~codecvt_byname() {}


#if !defined (_STLP_NO_WCHAR_T)

//----------------------------------------------------------------------
// codecvt_byname<wchar_t>
codecvt_byname<wchar_t, char, mbstate_t>::codecvt_byname(const char* name, size_t refs)
  : codecvt<wchar_t, char, mbstate_t>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_codecvt = _STLP_PRIV __acquire_codecvt(name, buf, 0, &__err_code);
  if (!_M_codecvt)
    locale::_M_throw_on_creation_failure(__err_code, name, "ctype");
}

codecvt_byname<wchar_t, char, mbstate_t>::~codecvt_byname()
{ _STLP_PRIV __release_codecvt(_M_codecvt); }

codecvt<wchar_t, char, mbstate_t>::result
codecvt_byname<wchar_t, char, mbstate_t>::do_out(state_type&         state,
                                                 const intern_type*  from,
                                                 const intern_type*  from_end,
                                                 const intern_type*& from_next,
                                                 extern_type*        to,
                                                 extern_type*        to_limit,
                                                 extern_type*&       to_next) const {
  while (from != from_end && to != to_limit) {
    size_t chars_stored = _WLocale_wctomb(_M_codecvt,
                                          to, to_limit - to, *from,
                                          &state);
    if (chars_stored == (size_t) -1) {
      from_next = from;
      to_next   = to;
      return error;
    }
    else if (chars_stored == (size_t) -2) {
      from_next = from;
      to_next   = to;
      return partial;
    }

    ++from;
    to += chars_stored;
  }

  from_next = from;
  to_next   = to;
  return ok;
}

codecvt<wchar_t, char, mbstate_t>::result
codecvt_byname<wchar_t, char, mbstate_t>::do_in(state_type&         state,
                                                const extern_type*  from,
                                                const extern_type*  from_end,
                                                const extern_type*& from_next,
                                                intern_type*        to,
                                                intern_type*        to_end,
                                                intern_type*&       to_next) const {
  while (from != from_end && to != to_end) {
    size_t chars_read = _WLocale_mbtowc(_M_codecvt,
                                        to, from, from_end - from,
                                        &state);
    if (chars_read == (size_t) -1) {
      from_next = from;
      to_next   = to;
      return error;
    }

    if (chars_read == (size_t) -2) {
      from_next = from;
      to_next   = to;
      return partial;
    }

    from += chars_read;
    to++;
  }

  from_next = from;
  to_next   = to;
  return ok;
}

codecvt<wchar_t, char, mbstate_t>::result
codecvt_byname<wchar_t, char, mbstate_t>::do_unshift(state_type&   state,
                                                     extern_type*  to,
                                                     extern_type*  to_limit,
                                                     extern_type*& to_next) const {
  to_next = to;
  size_t result = _WLocale_unshift(_M_codecvt, &state,
                                   to, to_limit - to, &to_next);
  if (result == (size_t) -1)
    return error;
  else if (result == (size_t) -2)
    return partial;
  else
#    if defined (__ISCPP__)
    return /*to_next == to ? noconv :*/ ok;
#    else
    return to_next == to ? noconv : ok;
#    endif
}

int
codecvt_byname<wchar_t, char, mbstate_t>::do_encoding() const _STLP_NOTHROW {
  if (_WLocale_is_stateless(_M_codecvt)) {
    int max_width = _WLocale_mb_cur_max(_M_codecvt);
    int min_width = _WLocale_mb_cur_min(_M_codecvt);
    return min_width == max_width ? min_width : 0;
  }
  else
    return -1;
}

bool
codecvt_byname<wchar_t, char, mbstate_t>::do_always_noconv() const _STLP_NOTHROW
{ return false; }

int
codecvt_byname<wchar_t, char, mbstate_t>::do_length(state_type&         state,
                                                    const  extern_type* from,
                                                    const  extern_type* end,
                                                    size_t              mx) const {
  size_t __count = 0;
  while (from != end && mx--) {
    intern_type __dummy;
    size_t chars_read = _WLocale_mbtowc(_M_codecvt,
                                        &__dummy, from, end - from,
                                        &state);
    if ((chars_read == (size_t) -1) || (chars_read == (size_t) -2)) // error or partial
      break;
    __count += chars_read;
    from += chars_read;
  }
  return int(__count); 
}

int
codecvt_byname<wchar_t, char, mbstate_t>::do_max_length() const _STLP_NOTHROW
{ return _WLocale_mb_cur_max(_M_codecvt); }
#endif

// numpunct_byname<char>
numpunct_byname<char>::numpunct_byname(const char* name, size_t refs)
: numpunct<char>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_numeric = _STLP_PRIV __acquire_numeric(name, buf, 0, &__err_code);
  if (!_M_numeric)
    locale::_M_throw_on_creation_failure(__err_code, name, "numpunct");
}

numpunct_byname<char>::~numpunct_byname()
{ _STLP_PRIV __release_numeric(_M_numeric); }

char numpunct_byname<char>::do_decimal_point() const
{ return _Locale_decimal_point(_M_numeric); }

char numpunct_byname<char>::do_thousands_sep() const
{ return _Locale_thousands_sep(_M_numeric); }

string numpunct_byname<char>::do_grouping() const {
  const char * __grouping = _Locale_grouping(_M_numeric);
  if (__grouping != NULL && __grouping[0] == CHAR_MAX)
    __grouping = "";
  return __grouping;
}

string numpunct_byname<char>::do_truename() const
{ return _Locale_true(_M_numeric); }

string numpunct_byname<char>::do_falsename() const
{ return _Locale_false(_M_numeric); }

//----------------------------------------------------------------------
// numpunct<wchar_t>

#if !defined (_STLP_NO_WCHAR_T)

// numpunct_byname<wchar_t>

numpunct_byname<wchar_t>::numpunct_byname(const char* name, size_t refs)
: numpunct<wchar_t>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_numeric = _STLP_PRIV __acquire_numeric(name, buf, 0, &__err_code);
  if (!_M_numeric)
    locale::_M_throw_on_creation_failure(__err_code, name, "numpunct");
}

numpunct_byname<wchar_t>::~numpunct_byname()
{ _STLP_PRIV __release_numeric(_M_numeric); }

wchar_t numpunct_byname<wchar_t>::do_decimal_point() const
{ return _WLocale_decimal_point(_M_numeric); }

wchar_t numpunct_byname<wchar_t>::do_thousands_sep() const
{ return _WLocale_thousands_sep(_M_numeric); }

string numpunct_byname<wchar_t>::do_grouping() const {
  const char * __grouping = _Locale_grouping(_M_numeric);
  if (__grouping != NULL && __grouping[0] == CHAR_MAX)
    __grouping = "";
  return __grouping;
}

wstring numpunct_byname<wchar_t>::do_truename() const {
  wchar_t buf[16];
  return _WLocale_true(_M_numeric, _STLP_ARRAY_AND_SIZE(buf));
}

wstring numpunct_byname<wchar_t>::do_falsename() const {
  wchar_t buf[16];
  return _WLocale_false(_M_numeric, _STLP_ARRAY_AND_SIZE(buf));
}

#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

static void _Init_monetary_formats(money_base::pattern& pos_format,
                                   money_base::pattern& neg_format,
                                   _Locale_monetary * monetary) {
  switch (_Locale_p_sign_posn(monetary)) {
    case 0: // Parentheses surround the quantity and currency symbol
    case 1: // The sign string precedes the quantity and currency symbol
      pos_format.field[0] = (char) money_base::sign;
      if (_Locale_p_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a positive value
        pos_format.field[1] = (char) money_base::symbol;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::space;
          pos_format.field[3] = (char) money_base::value;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::value;
          pos_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a positive value
        pos_format.field[1] = (char) money_base::value;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::space;
          pos_format.field[3] = (char) money_base::symbol;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::symbol;
          pos_format.field[3] = (char) money_base::none;
        }
      }
      break;
    case 2: // The sign string succeeds the quantity and currency symbol.
      if (_Locale_p_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a positive value
        pos_format.field[0] = (char) money_base::symbol;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::space;
          pos_format.field[2] = (char) money_base::value;
          pos_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::value;
          pos_format.field[2] = (char) money_base::sign;
          pos_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a positive value
        pos_format.field[0] = (char) money_base::value;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::space;
          pos_format.field[2] = (char) money_base::symbol;
          pos_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::symbol;
          pos_format.field[2] = (char) money_base::sign;
          pos_format.field[3] = (char) money_base::none;
        }
      }
      break;
    case 3: // The sign string immediately precedes the currency symbol.
      if (_Locale_p_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a positive value
        pos_format.field[0] = (char) money_base::sign;
        pos_format.field[1] = (char) money_base::symbol;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::space;
          pos_format.field[3] = (char) money_base::value;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[2] = (char) money_base::value;
          pos_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a positive value
        pos_format.field[0] = (char) money_base::value;
        pos_format.field[1] = (char) money_base::sign;
        pos_format.field[2] = (char) money_base::symbol;
        pos_format.field[3] = (char) money_base::none;
      }
      break;
    case 4: // The sign string immediately succeeds the currency symbol.
      if (_Locale_p_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a positive value
        pos_format.field[0] = (char) money_base::symbol;
        pos_format.field[1] = (char) money_base::sign;
        pos_format.field[2] = (char) money_base::value;
        pos_format.field[3] = (char) money_base::none;
      } else {
        // 0 if currency symbol succeeds a positive value
        pos_format.field[0] = (char) money_base::value;
        if (_Locale_p_sep_by_space(monetary)) {
          // a space separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::space;
          pos_format.field[2] = (char) money_base::symbol;
          pos_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a positive value.
          pos_format.field[1] = (char) money_base::symbol;
          pos_format.field[2] = (char) money_base::sign;
          pos_format.field[3] = (char) money_base::none;
        }
      }
      break;
    default: // Default C++ Standard format
      pos_format.field[0] = (char) money_base::symbol;
      pos_format.field[1] = (char) money_base::sign;
      pos_format.field[2] = (char) money_base::none;
      pos_format.field[3] = (char) money_base::value;
      break;
  }

  switch (_Locale_n_sign_posn(monetary)) {
    case 0: // Parentheses surround the quantity and currency symbol
    case 1: // The sign string precedes the quantity and currency symbol
      neg_format.field[0] = (char) money_base::sign;
      if (_Locale_n_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a negative value
        neg_format.field[1] = (char) money_base::symbol;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::space;
          neg_format.field[3] = (char) money_base::value;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::value;
          neg_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a negative value
        neg_format.field[1] = (char) money_base::value;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::space;
          neg_format.field[3] = (char) money_base::symbol;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::symbol;
          neg_format.field[3] = (char) money_base::none;
        }
      }
      break;
    case 2: // The sign string succeeds the quantity and currency symbol.
      if (_Locale_n_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a negative value
        neg_format.field[0] = (char) money_base::symbol;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::space;
          neg_format.field[2] = (char) money_base::value;
          neg_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::value;
          neg_format.field[2] = (char) money_base::sign;
          neg_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a negative value
        neg_format.field[0] = (char) money_base::value;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::space;
          neg_format.field[2] = (char) money_base::symbol;
          neg_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::symbol;
          neg_format.field[2] = (char) money_base::sign;
          neg_format.field[3] = (char) money_base::none;
        }
      }
      break;
    case 3: // The sign string immediately precedes the currency symbol.
      if (_Locale_n_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a negative value
        neg_format.field[0] = (char) money_base::sign;
        neg_format.field[1] = (char) money_base::symbol;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::space;
          neg_format.field[3] = (char) money_base::value;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[2] = (char) money_base::value;
          neg_format.field[3] = (char) money_base::none;
        }
      } else {
        // 0 if currency symbol succeeds a negative value
        neg_format.field[0] = (char) money_base::value;
        neg_format.field[1] = (char) money_base::sign;
        neg_format.field[2] = (char) money_base::symbol;
        neg_format.field[3] = (char) money_base::none;
      }
      break;
    case 4: // The sign string immediately succeeds the currency symbol.
      if (_Locale_n_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a negative value
        neg_format.field[0] = (char) money_base::symbol;
        neg_format.field[1] = (char) money_base::sign;
        neg_format.field[2] = (char) money_base::none;
        neg_format.field[3] = (char) money_base::value;
      } else {
        // 0 if currency symbol succeeds a negative value
        neg_format.field[0] = (char) money_base::value;
        if (_Locale_n_sep_by_space(monetary)) {
          // a space separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::space;
          neg_format.field[2] = (char) money_base::symbol;
          neg_format.field[3] = (char) money_base::sign;
        } else {
          // a space not separates currency symbol from a negative value.
          neg_format.field[1] = (char) money_base::symbol;
          neg_format.field[2] = (char) money_base::sign;
          neg_format.field[3] = (char) money_base::none;
        }
      }
      break;
    default: // Default C++ Standard format
      neg_format.field[0] = (char) money_base::symbol;
      neg_format.field[1] = (char) money_base::sign;
      neg_format.field[2] = (char) money_base::none;
      neg_format.field[3] = (char) money_base::value;
      break;
  }
}

// international variant of monetary

/*
 * int_curr_symbol
 *
 *   The international currency symbol. The operand is a four-character
 *   string, with the first three characters containing the alphabetic
 *   international currency symbol in accordance with those specified
 *   in the ISO 4217 specification. The fourth character is the character used
 *   to separate the international currency symbol from the monetary quantity.
 *
 * (http://www.opengroup.org/onlinepubs/7990989775/xbd/locale.html)
 */

/*
 * Standards are unclear in the usage of international currency
 * and monetary formats.
 * But I am expect that international currency symbol should be the first
 * (not depends upon where currency symbol situated in the national
 * format).
 *
 * If this isn't so, let's see:
 *       1 234.56 RUR
 *       GBP 1,234.56
 *       USD 1,234.56
 * The situation really is worse than you see above:
 * RUR typed wrong here---it prints '1 234.56 RUR ' (see space after RUR).
 * This is due to intl_fmp.curr_symbol() == "RUR ". (see reference in comments
 * above).
 *
 */

static void _Init_monetary_formats_int(money_base::pattern& pos_format,
                                       money_base::pattern& neg_format,
                                       _Locale_monetary * monetary)
{

  switch (_Locale_p_sign_posn(monetary)) {
    case 0: // Parentheses surround the quantity and currency symbol
    case 1: // The sign string precedes the quantity and currency symbol
      pos_format.field[0] = (char) money_base::symbol;
      pos_format.field[1] = (char) money_base::sign;
      pos_format.field[2] = (char) money_base::value;
      pos_format.field[3] = (char) money_base::none;
      break;
    case 2: // The sign string succeeds the quantity and currency symbol.
      pos_format.field[0] = (char) money_base::symbol;
      pos_format.field[1] = (char) money_base::value;
      pos_format.field[2] = (char) money_base::sign;
      pos_format.field[3] = (char) money_base::none;
      break;
    case 3: // The sign string immediately precedes the currency symbol.
    case 4: // The sign string immediately succeeds the currency symbol.
      pos_format.field[0] = (char) money_base::symbol;
      if (_Locale_p_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a positive value
        pos_format.field[1] = (char) money_base::sign;
        pos_format.field[2] = (char) money_base::value;
      } else {
        // 0 if currency symbol succeeds a positive value
        pos_format.field[1] = (char) money_base::value;
        pos_format.field[2] = (char) money_base::sign;
      }
      pos_format.field[3] = (char) money_base::none;
      break;
    default: // Default C++ Standard format
      pos_format.field[0] = (char) money_base::symbol;
      pos_format.field[1] = (char) money_base::sign;
      pos_format.field[2] = (char) money_base::none;
      pos_format.field[3] = (char) money_base::value;
      break;
  }


  switch (_Locale_n_sign_posn(monetary)) {
    case 0: // Parentheses surround the quantity and currency symbol
    case 1: // The sign string precedes the quantity and currency symbol
      neg_format.field[0] = (char) money_base::symbol;
      neg_format.field[1] = (char) money_base::sign;
      neg_format.field[2] = (char) money_base::value;
      neg_format.field[3] = (char) money_base::none;
      break;
    case 2: // The sign string succeeds the quantity and currency symbol.
      neg_format.field[0] = (char) money_base::symbol;
      neg_format.field[1] = (char) money_base::value;
      neg_format.field[2] = (char) money_base::sign;
      neg_format.field[3] = (char) money_base::none;
      break;
    case 3: // The sign string immediately precedes the currency symbol.
    case 4: // The sign string immediately succeeds the currency symbol.
      neg_format.field[0] = (char) money_base::symbol;
      if (_Locale_n_cs_precedes(monetary)) {
        // 1 if currency symbol precedes a negative value
        neg_format.field[1] = (char) money_base::sign;
        neg_format.field[2] = (char) money_base::value;
      } else {
        // 0 if currency symbol succeeds a negative value
        neg_format.field[1] = (char) money_base::value;
        neg_format.field[2] = (char) money_base::sign;
      }
      neg_format.field[3] = (char) money_base::none;
      break;
    default: // Default C++ Standard format
      neg_format.field[0] = (char) money_base::symbol;
      neg_format.field[1] = (char) money_base::sign;
      neg_format.field[2] = (char) money_base::none;
      neg_format.field[3] = (char) money_base::value;
      break;
  }
}

_STLP_MOVE_TO_STD_NAMESPACE

//
// moneypunct_byname<>
//
moneypunct_byname<char, true>::moneypunct_byname(const char * name,
                                                 size_t refs)
    : moneypunct<char, true>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_monetary = _STLP_PRIV __acquire_monetary(name, buf, 0, &__err_code);
  if (!_M_monetary)
    locale::_M_throw_on_creation_failure(__err_code, name, "moneypunct");

  _STLP_PRIV _Init_monetary_formats_int(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<char, true>::moneypunct_byname(_Locale_monetary *__mon)
  : _M_monetary(__mon) {
  _STLP_PRIV _Init_monetary_formats_int(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<char, true>::~moneypunct_byname()
{ _STLP_PRIV __release_monetary(_M_monetary); }

char moneypunct_byname<char, true>::do_decimal_point() const
{ return _Locale_mon_decimal_point(_M_monetary); }

char moneypunct_byname<char, true>::do_thousands_sep() const
{ return _Locale_mon_thousands_sep(_M_monetary); }

string moneypunct_byname<char, true>::do_grouping() const
{ return _Locale_mon_grouping(_M_monetary); }

string moneypunct_byname<char, true>::do_curr_symbol() const
{ return _Locale_int_curr_symbol(_M_monetary); }

string moneypunct_byname<char, true>::do_positive_sign() const
{ return _Locale_positive_sign(_M_monetary); }

string moneypunct_byname<char, true>::do_negative_sign() const
{ return _Locale_negative_sign(_M_monetary); }

int moneypunct_byname<char, true>::do_frac_digits() const
{ return _Locale_int_frac_digits(_M_monetary); }

moneypunct_byname<char, false>::moneypunct_byname(const char * name,
                                                  size_t refs)
    : moneypunct<char, false>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_monetary = _STLP_PRIV __acquire_monetary(name, buf, 0, &__err_code);
  if (!_M_monetary)
    locale::_M_throw_on_creation_failure(__err_code, name, "moneypunct");

  _STLP_PRIV _Init_monetary_formats(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<char, false>::moneypunct_byname(_Locale_monetary *__mon)
  : _M_monetary(__mon) {
  _STLP_PRIV _Init_monetary_formats(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<char, false>::~moneypunct_byname()
{ _STLP_PRIV __release_monetary(_M_monetary); }

char moneypunct_byname<char, false>::do_decimal_point() const
{ return _Locale_mon_decimal_point(_M_monetary); }

char moneypunct_byname<char, false>::do_thousands_sep() const
{ return _Locale_mon_thousands_sep(_M_monetary); }

string moneypunct_byname<char, false>::do_grouping() const
{ return _Locale_mon_grouping(_M_monetary); }

string moneypunct_byname<char, false>::do_curr_symbol() const
{ return _Locale_currency_symbol(_M_monetary); }

string moneypunct_byname<char, false>::do_positive_sign() const
{ return _Locale_positive_sign(_M_monetary); }

string moneypunct_byname<char, false>::do_negative_sign() const
{ return _Locale_negative_sign(_M_monetary); }

int moneypunct_byname<char, false>::do_frac_digits() const
{ return _Locale_frac_digits(_M_monetary); }

//
// moneypunct_byname<wchar_t>
//
#if !defined (_STLP_NO_WCHAR_T)

moneypunct_byname<wchar_t, true>::moneypunct_byname(const char * name,
                                                    size_t refs)
    : moneypunct<wchar_t, true>(refs) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_monetary = _STLP_PRIV __acquire_monetary(name, buf, 0, &__err_code);
  if (!_M_monetary)
    locale::_M_throw_on_creation_failure(__err_code, name, "moneypunct");

  _STLP_PRIV _Init_monetary_formats_int(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<wchar_t, true>::moneypunct_byname(_Locale_monetary *__mon)
  : _M_monetary(__mon) {
  _STLP_PRIV _Init_monetary_formats_int(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<wchar_t, true>::~moneypunct_byname()
{ _STLP_PRIV __release_monetary(_M_monetary); }

wchar_t moneypunct_byname<wchar_t, true>::do_decimal_point() const
{ return _Locale_mon_decimal_point(_M_monetary); }

wchar_t moneypunct_byname<wchar_t, true>::do_thousands_sep() const
{ return _Locale_mon_thousands_sep(_M_monetary); }

string moneypunct_byname<wchar_t, true>::do_grouping() const
{ return _Locale_mon_grouping(_M_monetary); }

inline wstring __do_widen (string const& str) {
#if defined (_STLP_NO_MEMBER_TEMPLATES) || defined (_STLP_MSVC)
  wstring::_Reserve_t __Reserve;
  size_t __size = str.size();
  wstring result(__Reserve, __size);
  copy(str.begin(), str.end(), result.begin());
#else
  wstring result(str.begin(), str.end());
#endif
  return result;
}

wstring moneypunct_byname<wchar_t, true>::do_curr_symbol() const
{ wchar_t buf[16]; return _WLocale_int_curr_symbol(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

wstring moneypunct_byname<wchar_t, true>::do_positive_sign() const
{ wchar_t buf[16]; return _WLocale_positive_sign(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

wstring moneypunct_byname<wchar_t, true>::do_negative_sign() const
{ wchar_t buf[16]; return _WLocale_negative_sign(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

int moneypunct_byname<wchar_t, true>::do_frac_digits() const
{ return _Locale_int_frac_digits(_M_monetary); }

moneypunct_byname<wchar_t, false>::moneypunct_byname(const char * name,
                                                     size_t refs)
    : moneypunct<wchar_t, false>(refs) {
  if (!name)
    locale::_M_throw_on_null_name() ;

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_monetary = _STLP_PRIV __acquire_monetary(name, buf, 0, &__err_code);
  if (!_M_monetary)
    locale::_M_throw_on_creation_failure(__err_code, name, "moneypunct");

  _STLP_PRIV _Init_monetary_formats(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<wchar_t, false>::moneypunct_byname(_Locale_monetary *__mon)
  : _M_monetary(__mon) {
  _STLP_PRIV _Init_monetary_formats(_M_pos_format, _M_neg_format, _M_monetary);
}

moneypunct_byname<wchar_t, false>::~moneypunct_byname()
{ _STLP_PRIV __release_monetary(_M_monetary); }

wchar_t moneypunct_byname<wchar_t, false>::do_decimal_point() const
{ return _Locale_mon_decimal_point(_M_monetary); }

wchar_t moneypunct_byname<wchar_t, false>::do_thousands_sep() const
{ return _Locale_mon_thousands_sep(_M_monetary); }

string moneypunct_byname<wchar_t, false>::do_grouping() const
{ return _Locale_mon_grouping(_M_monetary); }

wstring moneypunct_byname<wchar_t, false>::do_curr_symbol() const
{ wchar_t buf[16]; return _WLocale_currency_symbol(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

wstring moneypunct_byname<wchar_t, false>::do_positive_sign() const
{ wchar_t buf[16]; return _WLocale_positive_sign(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

wstring moneypunct_byname<wchar_t, false>::do_negative_sign() const
{ wchar_t buf[16]; return _WLocale_negative_sign(_M_monetary, _STLP_ARRAY_AND_SIZE(buf)); }

int moneypunct_byname<wchar_t, false>::do_frac_digits() const
{ return _Locale_frac_digits(_M_monetary); }

#endif

_STLP_END_NAMESPACE

