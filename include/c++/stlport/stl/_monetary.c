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
#ifndef _STLP_MONETARY_C
#define _STLP_MONETARY_C

# ifndef _STLP_INTERNAL_MONETARY_H
#  include <stl/_monetary.h>
# endif

#ifndef _STLP_INTERNAL_IOS_H
# include <stl/_ios.h>
#endif

#ifndef _STLP_INTERNAL_NUM_PUT_H
# include <stl/_num_put.h>
#endif

#ifndef _STLP_INTERNAL_NUM_GET_H
# include <stl/_num_get.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _CharT, class _InputIterator>
locale::id money_get<_CharT, _InputIterator>::id;

template <class _CharT, class _OutputIterator>
locale::id money_put<_CharT, _OutputIterator>::id;

// money_get facets

_STLP_MOVE_TO_PRIV_NAMESPACE

// helper functions for do_get
template <class _InIt1, class _InIt2>
pair<_InIt1, bool> __get_string( _InIt1 __first, _InIt1 __last,
                                 _InIt2 __str_first, _InIt2 __str_last) {
  while ( __first != __last && __str_first != __str_last && *__first == *__str_first ) {
    ++__first;
    ++__str_first;
  }
  return make_pair(__first, __str_first == __str_last);
}

template <class _InIt, class _OuIt, class _CharT>
bool
__get_monetary_value(_InIt& __first, _InIt __last, _OuIt __out_ite,
                     const ctype<_CharT>& _c_type,
                     _CharT __point, int __frac_digits, _CharT __sep,
                     const string& __grouping, bool &__syntax_ok) {
  if (__first == __last || !_c_type.is(ctype_base::digit, *__first))
    return false;

  char __group_sizes[128];
  char* __group_sizes_end = __grouping.empty()? 0 : __group_sizes;
  char   __current_group_size = 0;

  while (__first != __last) {
    if (_c_type.is(ctype_base::digit, *__first)) {
      ++__current_group_size;
      *__out_ite++ = *__first++;
    }
    else if (__group_sizes_end) {
      if (*__first == __sep) {
        *__group_sizes_end++ = __current_group_size;
        __current_group_size = 0;
        ++__first;
      }
      else break;
    }
    else
      break;
  }

  if (__grouping.empty())
    __syntax_ok = true;
  else {
    if (__group_sizes_end != __group_sizes)
      *__group_sizes_end++ = __current_group_size;

    __syntax_ok = __valid_grouping(__group_sizes, __group_sizes_end,
                                   __grouping.data(), __grouping.data()+ __grouping.size());

    if (__first == __last || *__first != __point) {
      for (int __digits = 0; __digits != __frac_digits; ++__digits)
        *__out_ite++ = _CharT('0');
      return true; // OK not to have decimal point
    }
  }

  ++__first;

  int __digits = 0;

  while (__first != __last && _c_type.is(ctype_base::digit, *__first)) {
      *__out_ite++ = *__first++;
     ++__digits;
  }

  __syntax_ok = __syntax_ok && (__digits == __frac_digits);

  return true;
}


template <class _CharT, class _InputIter, class _StrType>
_InputIter __money_do_get(_InputIter __s, _InputIter __end, bool  __intl,
                     ios_base&  __str, ios_base::iostate&  __err,
                     _StrType& __digits, bool &__is_positive, _CharT* /*__dummy*/) {
  if (__s == __end) {
    __err |= ios_base::eofbit;
    return __s;
  }

  typedef _CharT char_type;
  typedef _StrType string_type;
  typedef _InputIter iter_type;
  typedef moneypunct<char_type, false> _Punct;
  typedef moneypunct<char_type, true>  _Punct_intl;
  typedef ctype<char_type>             _Ctype;

  locale __loc = __str.getloc();
  const _Punct&      __punct      = use_facet<_Punct>(__loc) ;
  const _Punct_intl& __punct_intl = use_facet<_Punct_intl>(__loc) ;
  const _Ctype&      __c_type     = use_facet<_Ctype>(__loc) ;

  money_base::pattern __format = __intl ? __punct_intl.neg_format()
                                        : __punct.neg_format();
  string_type __ns = __intl ? __punct_intl.negative_sign()
                            : __punct.negative_sign();
  string_type __ps = __intl ? __punct_intl.positive_sign()
                            : __punct.positive_sign();
  int __i;
  bool __symbol_required = (__str.flags() & ios_base::showbase) != 0;
  string_type __buf;
  back_insert_iterator<string_type> __out_ite(__buf);

  for (__i = 0; __i < 4; ++__i) {
    switch (__format.field[__i]) {
    case money_base::space:
      if (!__c_type.is(ctype_base::space, *__s)) {
        __err = ios_base::failbit;
        return __s;
      }
      ++__s;
    case money_base::none:
      while (__s != __end && __c_type.is(ctype_base::space, *__s))
        ++__s;
      break;
    case money_base::symbol: {
      string_type __curs = __intl ? __punct_intl.curr_symbol()
                                  : __punct.curr_symbol();
      pair<iter_type, bool>
      __result  = __get_string(__s, __end, __curs.begin(), __curs.end());
      if (!__result.second && __symbol_required)
        __err = ios_base::failbit;
      __s = __result.first;
      break;
    }
    case money_base::sign: {
      if (__s == __end) {
        if (__ps.empty())
          break;
        if (__ns.empty()) {
          __is_positive = false;
          break;
        }
        __err = ios_base::failbit;
        return __s;
      }
      else {
        if (__ps.empty()) {
          if (__ns.empty())
            break;
          if (*__s == __ns[0]) {
            ++__s;
            __is_positive = false;
          }
          break;
        }
        else {
          if (*__s == __ps[0]) {
            ++__s;
            break;
          }
          if (__ns.empty())
            break;
          if (*__s == __ns[0]) {
            ++__s;
            __is_positive = false;
            break;
          }
          __err = ios_base::failbit;
        }
      }
      return __s;
    }
    case money_base::value: {
      char_type __point = __intl ? __punct_intl.decimal_point()
                                 : __punct.decimal_point();
      int __frac_digits = __intl ? __punct_intl.frac_digits()
                                 : __punct.frac_digits();
      string __grouping = __intl ? __punct_intl.grouping()
                                 : __punct.grouping();
      bool __syntax_ok = true;

      bool __result;

      char_type __sep = __grouping.empty() ? char_type() :
      __intl ? __punct_intl.thousands_sep() : __punct.thousands_sep();

      __result = __get_monetary_value(__s, __end, __out_ite, __c_type,
                                      __point, __frac_digits,
                                      __sep,
                                      __grouping, __syntax_ok);

      if (!__syntax_ok)
        __err |= ios_base::failbit;
      if (!__result) {
        __err = ios_base::failbit;
        return __s;
      }
      break;

    }                           // Close money_base::value case
    }                           // Close switch statement
  }                             // Close for loop

  if (__is_positive) {
    if (__ps.size() > 1) {
      pair<_InputIter, bool>
        __result = __get_string(__s, __end, __ps.begin() + 1, __ps.end());
      __s = __result.first;
      if (!__result.second)
        __err |= ios::failbit;
    }
    if (!(__err & ios_base::failbit))
      __digits = __buf;
  }
  else {
    if (__ns.size() > 1) {
      pair<_InputIter, bool>
        __result = __get_string(__s, __end, __ns.begin() + 1, __ns.end());
      __s = __result.first;
      if (!__result.second)
        __err |= ios::failbit;
    }
    if (!(__err & ios::failbit)) {
      __digits = __c_type.widen('-');
      __digits += __buf;
    }
  }
  if (__s == __end)
    __err |= ios::eofbit;

  return __s;
}

_STLP_MOVE_TO_STD_NAMESPACE

//===== methods ======
template <class _CharT, class _InputIter>
_InputIter
money_get<_CharT, _InputIter>::do_get(_InputIter __s, _InputIter  __end, bool  __intl,
                                      ios_base&  __str, ios_base::iostate& __err,
                                      _STLP_LONGEST_FLOAT_TYPE& __units) const {
  string_type __buf;
  bool __is_positive = true;
  __s = _STLP_PRIV __money_do_get(__s, __end, __intl, __str, __err, __buf, __is_positive, (_CharT*)0);

  if (__err == ios_base::goodbit || __err == ios_base::eofbit) {
    typename string_type::iterator __b = __buf.begin(), __e = __buf.end();

    if (!__is_positive) ++__b;
    // Can't use atold, since it might be wchar_t. Don't get confused by name below :
    // it's perfectly capable of reading long double.
    _STLP_PRIV __get_decimal_integer(__b, __e, __units, (_CharT*)0);

    if (!__is_positive) {
      __units = -__units;
    }
  }

  return __s;
}

template <class _CharT, class _InputIter>
_InputIter
money_get<_CharT, _InputIter>::do_get(iter_type __s, iter_type  __end, bool  __intl,
                                      ios_base&  __str, ios_base::iostate&  __err,
                                      string_type& __digits) const {
  bool __is_positive = true;
  return _STLP_PRIV __money_do_get(__s, __end, __intl, __str, __err, __digits, __is_positive, (_CharT*)0);
}

// money_put facets

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _CharT, class _OutputIter, class _Str_Type, class _Str>
_OutputIter __money_do_put(_OutputIter __s, bool  __intl, ios_base&  __str,
                           _CharT __fill, const _Str& __digits, bool __check_digits,
                           _Str_Type * /*__dummy*/) {
  typedef _CharT char_type;
  typedef _Str_Type string_type;
  typedef ctype<char_type>             _Ctype;
  typedef moneypunct<char_type, false> _Punct;
  typedef moneypunct<char_type, true>  _Punct_intl;

  locale __loc = __str.getloc();
  const _Ctype&      __c_type     = use_facet<_Ctype>(__loc) ;
  const _Punct&      __punct      = use_facet<_Punct>(__loc) ;
  const _Punct_intl& __punct_intl = use_facet<_Punct_intl>(__loc) ;

  // some special characters
  char_type __minus = __c_type.widen('-');
  char_type __plus  = __c_type.widen('+');
  char_type __space = __c_type.widen(' ');
  char_type __zero  = __c_type.widen('0');
  char_type __point = __intl ? __punct_intl.decimal_point()
                             : __punct.decimal_point();

  char_type __sep = __intl ? __punct_intl.thousands_sep()
                           : __punct.thousands_sep();

  string __grouping = __intl ? __punct_intl.grouping()
                             : __punct.grouping();

  int __frac_digits      = __intl ? __punct_intl.frac_digits()
                                  : __punct.frac_digits();

  string_type __curr_sym = __intl ? __punct_intl.curr_symbol()
                                  : __punct.curr_symbol();

  // if there are no digits we are going to return __s.  If there
  // are digits, but not enough to fill the frac_digits, we are
  // going to add zeros.  I don't know whether this is right or
  // not.
  if (__digits.empty())
    return __s;

  typename string_type::const_iterator __digits_first = __digits.begin();
  typename string_type::const_iterator __digits_last  = __digits.end();

  bool __is_negative = *__digits_first == __minus;
  if (__is_negative)
    ++__digits_first;

#if !defined (__BORLANDC__)
  string_type __sign = __intl ? __is_negative ? __punct_intl.negative_sign()
                                              : __punct_intl.positive_sign()
                              : __is_negative ? __punct.negative_sign()
                                              : __punct.positive_sign();
#else
  string_type __sign;
  if (__intl) {
    if (__is_negative)
      __sign = __punct_intl.negative_sign();
    else
      __sign = __punct_intl.positive_sign();
  }
  else {
    if (__is_negative)
      __sign = __punct.negative_sign();
    else
      __sign = __punct.positive_sign();
  }
#endif

  if (__check_digits) {
    typename string_type::const_iterator __cp = __digits_first;
    while (__cp != __digits_last && __c_type.is(ctype_base::digit, *__cp))
      ++__cp;
    if (__cp == __digits_first)
      return __s;
    __digits_last = __cp;
  }

  // If grouping is required, we make a copy of __digits and
  // insert the grouping.
  _STLP_BASIC_IOSTRING(char_type) __new_digits;
  if (!__grouping.empty()) {
    __new_digits.assign(__digits_first, __digits_last);
    __insert_grouping(__new_digits,
                      __new_digits.size() - __frac_digits,
                      __grouping,
                      __sep, __plus, __minus, 0);
    __digits_first = __new_digits.begin(); // <<--
    __digits_last  = __new_digits.end();   // <<--
  }

  // Determine the amount of padding required, if any.
  streamsize __width = __str.width();

#if defined (_STLP_DEBUG) && (defined(__HP_aCC) && (__HP_aCC <= 1))
  size_t __value_length = operator -(__digits_last, __digits_first);
#else
  size_t __value_length = __digits_last - __digits_first;
#endif

  size_t __length = __value_length + __sign.size();

  if (__frac_digits != 0)
    ++__length;

  bool __generate_curr = (__str.flags() & ios_base::showbase) !=0;
  if (__generate_curr)
    __length += __curr_sym.size();
  money_base::pattern __format = __intl ? (__is_negative ? __punct_intl.neg_format()
                                                         : __punct_intl.pos_format())
                                        : (__is_negative ? __punct.neg_format()
                                                         : __punct.pos_format());
  {
    //For the moment the following is commented for decoding reason.
    //No reason to add a space last if the money symbol do not have to be display
    //if (__format.field[3] == (char) money_base::symbol && !__generate_curr) {
    //  if (__format.field[2] == (char) money_base::space) {
    //    __format.field[2] = (char) money_base::none;
    //  }
    //}
    //space can only be second or third and only once (22.2.6.3-1):
    if ((__format.field[1] == (char) money_base::space) ||
        (__format.field[2] == (char) money_base::space))
      ++__length;
  }

  const bool __need_fill = (((sizeof(streamsize) > sizeof(size_t)) && (__STATIC_CAST(streamsize, __length) < __width)) ||
                            ((sizeof(streamsize) <= sizeof(size_t)) && (__length < __STATIC_CAST(size_t, __width))));
  streamsize __fill_amt = __need_fill ? __width - __length : 0;

  ios_base::fmtflags __fill_pos = __str.flags() & ios_base::adjustfield;

  if (__fill_amt != 0 &&
      !(__fill_pos & (ios_base::left | ios_base::internal)))
    __s = _STLP_PRIV __fill_n(__s, __fill_amt, __fill);

  for (int __i = 0; __i < 4; ++__i) {
    char __ffield = __format.field[__i];
    switch (__ffield) {
      case money_base::space:
        *__s++ = __space;
      case money_base::none:
        if (__fill_amt != 0 && __fill_pos == ios_base::internal)
          __s = _STLP_PRIV __fill_n(__s, __fill_amt, __fill);
        break;
      case money_base::symbol:
        if (__generate_curr)
          __s = _STLP_STD::copy(__curr_sym.begin(), __curr_sym.end(), __s);
        break;
      case money_base::sign:
        if (!__sign.empty())
          *__s++ = __sign[0];
        break;
      case money_base::value:
        if (__frac_digits == 0) {
          __s = _STLP_STD::copy(__digits_first, __digits_last, __s);
        } else {
          if ((int)__value_length <= __frac_digits) {
            // if we see '9' here, we should out 0.09
            *__s++ = __zero;  // integer part is zero
            *__s++ = __point; // decimal point
            __s =  _STLP_PRIV __fill_n(__s, __frac_digits - __value_length, __zero); // zeros
            __s = _STLP_STD::copy(__digits_first, __digits_last, __s); // digits
          } else {
            __s = _STLP_STD::copy(__digits_first, __digits_last - __frac_digits, __s);
            if (__frac_digits != 0) {
              *__s++ = __point;
              __s = _STLP_STD::copy(__digits_last - __frac_digits, __digits_last, __s);
            }
          }
        }
        break;
    } //Close for switch
  } // Close for loop

  // Ouput rest of sign if necessary.
  if (__sign.size() > 1)
    __s = _STLP_STD::copy(__sign.begin() + 1, __sign.end(), __s);
  if (__fill_amt != 0 &&
      !(__fill_pos & (ios_base::right | ios_base::internal)))
    __s = _STLP_PRIV __fill_n(__s, __fill_amt, __fill);

  return __s;
}

_STLP_MOVE_TO_STD_NAMESPACE

template <class _CharT, class _OutputIter>
_OutputIter
money_put<_CharT, _OutputIter>
 ::do_put(_OutputIter __s, bool __intl, ios_base& __str,
          char_type __fill, _STLP_LONGEST_FLOAT_TYPE __units) const {
  _STLP_BASIC_IOSTRING(char_type) __digits;
  _STLP_PRIV __get_money_digits(__digits, __str, __units);
  return _STLP_PRIV __money_do_put(__s, __intl, __str, __fill, __digits, false, __STATIC_CAST(string_type*, 0));
}

template <class _CharT, class _OutputIter>
_OutputIter
money_put<_CharT, _OutputIter>
 ::do_put(_OutputIter __s, bool __intl, ios_base& __str,
          char_type __fill, const string_type& __digits) const {
  return _STLP_PRIV __money_do_put(__s, __intl, __str, __fill, __digits, true, __STATIC_CAST(string_type*, 0));
}

_STLP_END_NAMESPACE

#endif /* _STLP_MONETARY_C */

// Local Variables:
// mode:C++
// End:
