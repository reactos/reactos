/*
 * Copyright (c) 1998
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

#ifndef _STLP_BITSET_H
#define _STLP_BITSET_H

// A bitset of size N has N % (sizeof(unsigned long) * CHAR_BIT) unused
// bits.  (They are the high- order bits in the highest word.)  It is
// a class invariant of class bitset<> that those unused bits are
// always zero.

// Most of the actual code isn't contained in bitset<> itself, but in the
// base class _Base_bitset.  The base class works with whole words, not with
// individual bits.  This allows us to specialize _Base_bitset for the
// important special case where the bitset is only a single word.

// The C++ standard does not define the precise semantics of operator[].
// In this implementation the const version of operator[] is equivalent
// to test(), except that it does no range checking.  The non-const version
// returns a reference to a bit, again without doing any range checking.


#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_H
#  include <stl/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_UNINITIALIZED_H
#  include <stl/_uninitialized.h>
#endif

#ifndef _STLP_RANGE_ERRORS_H
#  include <stl/_range_errors.h>
#endif

#ifndef _STLP_INTERNAL_STRING_H
#  include <stl/_string.h>
#endif

#define __BITS_PER_WORD (CHAR_BIT*sizeof(unsigned long))
#define __BITSET_WORDS(__n) ((__n + __BITS_PER_WORD - 1)/__BITS_PER_WORD)

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

// structure to aid in counting bits
class _STLP_CLASS_DECLSPEC _Bs_G
{
  public:
    //returns the number of bit set within the buffer between __beg and __end.
    static size_t _S_count(const unsigned char *__beg, const unsigned char *__end)
#if defined (_STLP_USE_NO_IOSTREAMS)
    {
      size_t __result = 0;
      for (; __beg != __end; ++__beg) {
        for (size_t i = 0; i < (sizeof(unsigned char) * 8); ++i) {
          if ((*__beg & (1 << i)) != 0) { ++__result; }
        }
      }
      return __result;
    }
#else
      ;
#endif
    // Mapping from 8 bit unsigned integers to the index of the first one bit set:
    static unsigned char _S_first_one(unsigned char __x)
#if defined (_STLP_USE_NO_IOSTREAMS)
    {
      for (unsigned char i = 0; i < (sizeof(unsigned char) * 8); ++i) {
        if ((__x & (1 << i)) != 0) { return i; }
      }
      return 0;
    }
#else
      ;
#endif
};

//
// Base class: general case.
//

template<size_t _Nw>
struct _Base_bitset {
  typedef unsigned long _WordT;

  _WordT _M_w[_Nw];                // 0 is the least significant word.

  _Base_bitset() { _M_do_reset(); }

  _Base_bitset(unsigned long __val) {
    _M_do_reset();
    _M_w[0] = __val;
  }

  static size_t _STLP_CALL _S_whichword( size_t __pos ) {
    return __pos / __BITS_PER_WORD;
  }
  static size_t _STLP_CALL _S_whichbyte( size_t __pos ) {
    return (__pos % __BITS_PER_WORD) / CHAR_BIT;
  }
  static size_t _STLP_CALL _S_whichbit( size_t __pos ) {
    return __pos % __BITS_PER_WORD;
  }
  static _WordT _STLP_CALL _S_maskbit( size_t __pos ) {
    return __STATIC_CAST(_WordT,1) << _S_whichbit(__pos);
  }

  _WordT& _M_getword(size_t __pos)       { return _M_w[_S_whichword(__pos)]; }
  _WordT  _M_getword(size_t __pos) const { return _M_w[_S_whichword(__pos)]; }

  _WordT& _M_hiword()       { return _M_w[_Nw - 1]; }
  _WordT  _M_hiword() const { return _M_w[_Nw - 1]; }

  void _M_do_and(const _Base_bitset<_Nw>& __x) {
    for ( size_t __i = 0; __i < _Nw; __i++ ) {
      _M_w[__i] &= __x._M_w[__i];
    }
  }

  void _M_do_or(const _Base_bitset<_Nw>& __x) {
    for ( size_t __i = 0; __i < _Nw; __i++ ) {
      _M_w[__i] |= __x._M_w[__i];
    }
  }

  void _M_do_xor(const _Base_bitset<_Nw>& __x) {
    for ( size_t __i = 0; __i < _Nw; __i++ ) {
      _M_w[__i] ^= __x._M_w[__i];
    }
  }

  void _M_do_left_shift(size_t __shift);

  void _M_do_right_shift(size_t __shift);

  void _M_do_flip() {
    for ( size_t __i = 0; __i < _Nw; __i++ ) {
      _M_w[__i] = ~_M_w[__i];
    }
  }

  void _M_do_set() {
    for ( size_t __i = 0; __i < _Nw; __i++ ) {
      _M_w[__i] = ~__STATIC_CAST(_WordT,0);
    }
  }

  void _M_do_reset() { memset(_M_w, 0, _Nw * sizeof(_WordT)); }

  bool _M_is_equal(const _Base_bitset<_Nw>& __x) const {
    for (size_t __i = 0; __i < _Nw; ++__i) {
      if (_M_w[__i] != __x._M_w[__i])
        return false;
    }
    return true;
  }

  bool _M_is_any() const {
    for ( size_t __i = 0; __i < _Nw ; __i++ ) {
      if ( _M_w[__i] != __STATIC_CAST(_WordT,0) )
        return true;
    }
    return false;
  }

  size_t _M_do_count() const {
    const unsigned char* __byte_ptr = (const unsigned char*)_M_w;
    const unsigned char* __end_ptr = (const unsigned char*)(_M_w+_Nw);

    return _Bs_G::_S_count(__byte_ptr, __end_ptr);
  }

  unsigned long _M_do_to_ulong() const;

  // find first "on" bit
  size_t _M_do_find_first(size_t __not_found) const;

  // find the next "on" bit that follows "prev"
  size_t _M_do_find_next(size_t __prev, size_t __not_found) const;
};

//
// Base class: specialization for a single word.
//
_STLP_TEMPLATE_NULL
struct _Base_bitset<1UL> {
  typedef unsigned long _WordT;
  typedef _Base_bitset<1UL> _Self;

  _WordT _M_w;

  _Base_bitset( void ) : _M_w(0) {}
  _Base_bitset(unsigned long __val) : _M_w(__val) {}

  static size_t _STLP_CALL _S_whichword( size_t __pos ) {
    return __pos / __BITS_PER_WORD ;
  }
  static size_t _STLP_CALL _S_whichbyte( size_t __pos ) {
    return (__pos % __BITS_PER_WORD) / CHAR_BIT;
  }
  static size_t _STLP_CALL _S_whichbit( size_t __pos ) {
    return __pos % __BITS_PER_WORD;
  }
  static _WordT _STLP_CALL _S_maskbit( size_t __pos ) {
    return (__STATIC_CAST(_WordT,1)) << _S_whichbit(__pos);
  }

  _WordT& _M_getword(size_t)       { return _M_w; }
  _WordT  _M_getword(size_t) const { return _M_w; }

  _WordT& _M_hiword()       { return _M_w; }
  _WordT  _M_hiword() const { return _M_w; }

  void _M_do_and(const _Self& __x) { _M_w &= __x._M_w; }
  void _M_do_or(const _Self& __x)  { _M_w |= __x._M_w; }
  void _M_do_xor(const _Self& __x) { _M_w ^= __x._M_w; }
  void _M_do_left_shift(size_t __shift)     { _M_w <<= __shift; }
  void _M_do_right_shift(size_t __shift)    { _M_w >>= __shift; }
  void _M_do_flip()                       { _M_w = ~_M_w; }
  void _M_do_set()                        { _M_w = ~__STATIC_CAST(_WordT,0); }
  void _M_do_reset()                      { _M_w = 0; }

  bool _M_is_equal(const _Self& __x) const {
    return _M_w == __x._M_w;
  }
  bool _M_is_any() const {
    return _M_w != 0;
  }

  size_t _M_do_count() const {
    const unsigned char* __byte_ptr = (const unsigned char*)&_M_w;
    const unsigned char* __end_ptr = ((const unsigned char*)&_M_w)+sizeof(_M_w);
    return _Bs_G::_S_count(__byte_ptr, __end_ptr);
  }

  unsigned long _M_do_to_ulong() const { return _M_w; }

  inline size_t _M_do_find_first(size_t __not_found) const;

  // find the next "on" bit that follows "prev"
  inline size_t _M_do_find_next(size_t __prev, size_t __not_found) const;
};


// ------------------------------------------------------------
//
// Definitions of should-be-non-inline functions from the single-word version of
//  _Base_bitset.
//
inline size_t
_Base_bitset<1UL>::_M_do_find_first(size_t __not_found) const {
  //  typedef unsigned long _WordT;
  _WordT __thisword = _M_w;

  if ( __thisword != __STATIC_CAST(_WordT,0) ) {
    // find byte within word
    for ( size_t __j = 0; __j < sizeof(_WordT); __j++ ) {
      unsigned char __this_byte
        = __STATIC_CAST(unsigned char,(__thisword & (~(unsigned char)0)));
      if ( __this_byte )
        return __j*CHAR_BIT + _Bs_G::_S_first_one(__this_byte);

      __thisword >>= CHAR_BIT;
    }
  }
  // not found, so return a value that indicates failure.
  return __not_found;
}

inline size_t
_Base_bitset<1UL>::_M_do_find_next(size_t __prev,
                                   size_t __not_found ) const {
  // make bound inclusive
  ++__prev;

  // check out of bounds
  if ( __prev >= __BITS_PER_WORD )
    return __not_found;

    // search first (and only) word
  _WordT __thisword = _M_w;

  // mask off bits below bound
  __thisword &= (~__STATIC_CAST(_WordT,0)) << _S_whichbit(__prev);

  if ( __thisword != __STATIC_CAST(_WordT,0) ) {
    // find byte within word
    // get first byte into place
    __thisword >>= _S_whichbyte(__prev) * CHAR_BIT;
    for ( size_t __j = _S_whichbyte(__prev); __j < sizeof(_WordT); __j++ ) {
      unsigned char __this_byte
        = __STATIC_CAST(unsigned char,(__thisword & (~(unsigned char)0)));
      if ( __this_byte )
        return __j*CHAR_BIT + _Bs_G::_S_first_one(__this_byte);

      __thisword >>= CHAR_BIT;
    }
  }

  // not found, so return a value that indicates failure.
  return __not_found;
} // end _M_do_find_next


// ------------------------------------------------------------
// Helper class to zero out the unused high-order bits in the highest word.

template <size_t _Extrabits> struct _Sanitize {
  static void _STLP_CALL _M_do_sanitize(unsigned long& __val)
  { __val &= ~((~__STATIC_CAST(unsigned long,0)) << _Extrabits); }
};

_STLP_TEMPLATE_NULL struct _Sanitize<0UL> {
  static void _STLP_CALL _M_do_sanitize(unsigned long) {}
};

_STLP_MOVE_TO_STD_NAMESPACE

// ------------------------------------------------------------
// Class bitset.
//   _Nb may be any nonzero number of type size_t.
template<size_t _Nb>
class bitset : public _STLP_PRIV _Base_bitset<__BITSET_WORDS(_Nb) > {
public:
  enum { _Words = __BITSET_WORDS(_Nb) } ;

private:
  typedef _STLP_PRIV _Base_bitset< _Words > _Base;

  void _M_do_sanitize() {
    _STLP_PRIV _Sanitize<_Nb%__BITS_PER_WORD >::_M_do_sanitize(this->_M_hiword());
  }
public:
  typedef unsigned long _WordT;
  struct reference;
  friend struct reference;

  // bit reference:
  struct reference {
  typedef _STLP_PRIV _Base_bitset<_Words > _Bitset_base;
  typedef bitset<_Nb> _Bitset;
    //    friend _Bitset;
    _WordT *_M_wp;
    size_t _M_bpos;

    // should be left undefined
    reference() {}

    reference( _Bitset& __b, size_t __pos ) {
      _M_wp = &__b._M_getword(__pos);
      _M_bpos = _Bitset_base::_S_whichbit(__pos);
    }

  public:
    ~reference() {}

    // for b[i] = __x;
    reference& operator=(bool __x) {
      if ( __x )
        *_M_wp |= _Bitset_base::_S_maskbit(_M_bpos);
      else
        *_M_wp &= ~_Bitset_base::_S_maskbit(_M_bpos);

      return *this;
    }

    // for b[i] = b[__j];
    reference& operator=(const reference& __j) {
      if ( (*(__j._M_wp) & _Bitset_base::_S_maskbit(__j._M_bpos)) )
        *_M_wp |= _Bitset_base::_S_maskbit(_M_bpos);
      else
        *_M_wp &= ~_Bitset_base::_S_maskbit(_M_bpos);

      return *this;
    }

    // flips the bit
    bool operator~() const { return (*(_M_wp) & _Bitset_base::_S_maskbit(_M_bpos)) == 0; }

    // for __x = b[i];
    operator bool() const { return (*(_M_wp) & _Bitset_base::_S_maskbit(_M_bpos)) != 0; }

    // for b[i].flip();
    reference& flip() {
      *_M_wp ^= _Bitset_base::_S_maskbit(_M_bpos);
      return *this;
    }
  };

  // 23.3.5.1 constructors:
  bitset() {}

  bitset(unsigned long __val) : _STLP_PRIV _Base_bitset<_Words>(__val) { _M_do_sanitize(); }

#if defined (_STLP_MEMBER_TEMPLATES)
  template<class _CharT, class _Traits, class _Alloc>
  explicit bitset(const basic_string<_CharT,_Traits,_Alloc>& __s,
                  size_t __pos = 0)
    : _STLP_PRIV _Base_bitset<_Words >() {
    if (__pos > __s.size())
      __stl_throw_out_of_range("bitset");
    _M_copy_from_string(__s, __pos,
                        basic_string<_CharT, _Traits, _Alloc>::npos);
  }
  template<class _CharT, class _Traits, class _Alloc>
  bitset(const basic_string<_CharT, _Traits, _Alloc>& __s,
          size_t __pos,
          size_t __n)
  : _STLP_PRIV _Base_bitset<_Words >() {
    if (__pos > __s.size())
      __stl_throw_out_of_range("bitset");
    _M_copy_from_string(__s, __pos, __n);
  }
#else /* _STLP_MEMBER_TEMPLATES */
  explicit bitset(const string& __s,
                  size_t __pos = 0,
                  size_t __n = (size_t)-1)
    : _STLP_PRIV _Base_bitset<_Words >() {
    if (__pos > __s.size())
      __stl_throw_out_of_range("bitset");
    _M_copy_from_string(__s, __pos, __n);
  }
#endif /* _STLP_MEMBER_TEMPLATES */

  // 23.3.5.2 bitset operations:
  bitset<_Nb>& operator&=(const bitset<_Nb>& __rhs) {
    this->_M_do_and(__rhs);
    return *this;
  }

  bitset<_Nb>& operator|=(const bitset<_Nb>& __rhs) {
    this->_M_do_or(__rhs);
    return *this;
  }

  bitset<_Nb>& operator^=(const bitset<_Nb>& __rhs) {
    this->_M_do_xor(__rhs);
    return *this;
  }

  bitset<_Nb>& operator<<=(size_t __pos) {
    this->_M_do_left_shift(__pos);
    this->_M_do_sanitize();
    return *this;
  }

  bitset<_Nb>& operator>>=(size_t __pos) {
    this->_M_do_right_shift(__pos);
    this->_M_do_sanitize();
    return *this;
  }

  //
  // Extension:
  // Versions of single-bit set, reset, flip, test with no range checking.
  //

  bitset<_Nb>& _Unchecked_set(size_t __pos) {
    this->_M_getword(__pos) |= _STLP_PRIV _Base_bitset<_Words > ::_S_maskbit(__pos);
    return *this;
  }

  bitset<_Nb>& _Unchecked_set(size_t __pos, int __val) {
    if (__val)
      this->_M_getword(__pos) |= this->_S_maskbit(__pos);
    else
      this->_M_getword(__pos) &= ~ this->_S_maskbit(__pos);

    return *this;
  }

  bitset<_Nb>& _Unchecked_reset(size_t __pos) {
    this->_M_getword(__pos) &= ~ this->_S_maskbit(__pos);
    return *this;
  }

  bitset<_Nb>& _Unchecked_flip(size_t __pos) {
    this->_M_getword(__pos) ^= this->_S_maskbit(__pos);
    return *this;
  }

  bool _Unchecked_test(size_t __pos) const {
    return (this->_M_getword(__pos) & this->_S_maskbit(__pos)) != __STATIC_CAST(_WordT,0);
  }

  // Set, reset, and flip.

  bitset<_Nb>& set() {
    this->_M_do_set();
    this->_M_do_sanitize();
    return *this;
  }

  bitset<_Nb>& set(size_t __pos) {
    if (__pos >= _Nb)
      __stl_throw_out_of_range("bitset");
    return _Unchecked_set(__pos);
  }

  bitset<_Nb>& set(size_t __pos, int __val) {
    if (__pos >= _Nb)
      __stl_throw_out_of_range("bitset");
    return _Unchecked_set(__pos, __val);
  }

  bitset<_Nb>& reset() {
    this->_M_do_reset();
    return *this;
  }

  bitset<_Nb>& reset(size_t __pos) {
    if (__pos >= _Nb)
      __stl_throw_out_of_range("bitset");

    return _Unchecked_reset(__pos);
  }

  bitset<_Nb>& flip() {
    this->_M_do_flip();
    this->_M_do_sanitize();
    return *this;
  }

  bitset<_Nb>& flip(size_t __pos) {
    if (__pos >= _Nb)
      __stl_throw_out_of_range("bitset");

    return _Unchecked_flip(__pos);
  }

  bitset<_Nb> operator~() const {
    return bitset<_Nb>(*this).flip();
  }

  // element access:
  //for b[i];
  reference operator[](size_t __pos) { return reference(*this,__pos); }
  bool operator[](size_t __pos) const { return _Unchecked_test(__pos); }

  unsigned long to_ulong() const { return this->_M_do_to_ulong(); }

#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS)
  template <class _CharT, class _Traits, class _Alloc>
  basic_string<_CharT, _Traits, _Alloc> to_string() const {
    basic_string<_CharT, _Traits, _Alloc> __result;
    _M_copy_to_string(__result);
    return __result;
  }
#else
  string to_string() const {
    string __result;
    _M_copy_to_string(__result);
    return __result;
  }
#endif /* _STLP_EXPLICIT_FUNCTION_TMPL_ARGS */

  size_t count() const { return this->_M_do_count(); }

  size_t size() const { return _Nb; }

  bool operator==(const bitset<_Nb>& __rhs) const {
    return this->_M_is_equal(__rhs);
  }
  bool operator!=(const bitset<_Nb>& __rhs) const {
    return !this->_M_is_equal(__rhs);
  }

  bool test(size_t __pos) const {
    if (__pos >= _Nb)
      __stl_throw_out_of_range("bitset");

    return _Unchecked_test(__pos);
  }

  bool any() const { return this->_M_is_any(); }
  bool none() const { return !this->_M_is_any(); }

  bitset<_Nb> operator<<(size_t __pos) const {
    bitset<_Nb> __result(*this);
    __result <<= __pos ;  return __result;
  }
  bitset<_Nb> operator>>(size_t __pos) const {
    bitset<_Nb> __result(*this);
    __result >>= __pos ;  return __result;
  }

#if !defined (_STLP_NO_EXTENSIONS)
  //
  // EXTENSIONS: bit-find operations.  These operations are
  // experimental, and are subject to change or removal in future
  // versions.
  //

  // find the index of the first "on" bit
  size_t _Find_first() const
    { return this->_M_do_find_first(_Nb); }

  // find the index of the next "on" bit after prev
  size_t _Find_next( size_t __prev ) const
    { return this->_M_do_find_next(__prev, _Nb); }
#endif

//
// Definitions of should-be non-inline member functions.
//
#if defined (_STLP_MEMBER_TEMPLATES)
  template<class _CharT, class _Traits, class _Alloc>
  void _M_copy_from_string(const basic_string<_CharT,_Traits,_Alloc>& __s,
                           size_t __pos, size_t __n) {
#else
  void _M_copy_from_string(const string& __s,
                           size_t __pos, size_t __n) {
    typedef typename string::traits_type _Traits;
#endif
    reset();
    size_t __tmp = _Nb;
    const size_t __Nbits = (min) (__tmp, (min) (__n, __s.size() - __pos));
    for ( size_t __i= 0; __i < __Nbits; ++__i) {
      typename _Traits::int_type __k = _Traits::to_int_type(__s[__pos + __Nbits - __i - 1]);
      // boris : widen() ?
      if (__k == '1')
        set(__i);
      else if (__k != '0')
        __stl_throw_invalid_argument("bitset");
    }
  }

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _CharT, class _Traits, class _Alloc>
  void _M_copy_to_string(basic_string<_CharT, _Traits, _Alloc>& __s) const
#else
  void _M_copy_to_string(string& __s) const
#endif
  {
    __s.assign(_Nb, '0');

    for (size_t __i = 0; __i < _Nb; ++__i) {
      if (_Unchecked_test(__i))
        __s[_Nb - 1 - __i] = '1';
    }
  }

#if !defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_NO_WCHAR_T)
  void _M_copy_to_string(wstring& __s) const {
    __s.assign(_Nb, '0');

    for (size_t __i = 0; __i < _Nb; ++__i) {
      if (_Unchecked_test(__i))
        __s[_Nb - 1 - __i] = '1';
    }
  }
#endif

#if defined (_STLP_NON_TYPE_TMPL_PARAM_BUG)
  bitset<_Nb> operator&(const bitset<_Nb>& __y) const {
    bitset<_Nb> __result(*this);
    __result &= __y;
    return __result;
  }
  bitset<_Nb> operator|(const bitset<_Nb>& __y) const {
    bitset<_Nb> __result(*this);
    __result |= __y;
    return __result;
  }
  bitset<_Nb> operator^(const bitset<_Nb>& __y) const {
    bitset<_Nb> __result(*this);
    __result ^= __y;
    return __result;
  }
#endif
};

// ------------------------------------------------------------
//
// 23.3.5.3 bitset operations:
//
#if ! defined (_STLP_NON_TYPE_TMPL_PARAM_BUG)
template <size_t _Nb>
inline bitset<_Nb>  _STLP_CALL
operator&(const bitset<_Nb>& __x,
          const bitset<_Nb>& __y) {
  bitset<_Nb> __result(__x);
  __result &= __y;
  return __result;
}


template <size_t _Nb>
inline bitset<_Nb>  _STLP_CALL
operator|(const bitset<_Nb>& __x,
          const bitset<_Nb>& __y) {
  bitset<_Nb> __result(__x);
  __result |= __y;
  return __result;
}

template <size_t _Nb>
inline bitset<_Nb>  _STLP_CALL
operator^(const bitset<_Nb>& __x,
          const bitset<_Nb>& __y) {
  bitset<_Nb> __result(__x);
  __result ^= __y;
  return __result;
}

#if !defined (_STLP_USE_NO_IOSTREAMS)

_STLP_END_NAMESPACE

#  if !(defined (_STLP_MSVC) && (_STLP_MSVC < 1300)) && \
      !(defined(__SUNPRO_CC) && (__SUNPRO_CC < 0x500))

#ifndef _STLP_INTERNAL_IOSFWD
#  include <stl/_iosfwd.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _CharT, class _Traits, size_t _Nb>
basic_istream<_CharT, _Traits>&  _STLP_CALL
operator>>(basic_istream<_CharT, _Traits>& __is, bitset<_Nb>& __x);

template <class _CharT, class _Traits, size_t _Nb>
basic_ostream<_CharT, _Traits>& _STLP_CALL
operator<<(basic_ostream<_CharT, _Traits>& __os, const bitset<_Nb>& __x);

#  else

#ifndef _STLP_STRING_IO_H
#  include <stl/_string_io.h> //includes _istream.h and _ostream.h
#endif

_STLP_BEGIN_NAMESPACE

template <size_t _Nb>
istream&  _STLP_CALL
operator>>(istream& __is, bitset<_Nb>& __x) {
  typedef typename string::traits_type _Traits;
  string __tmp;
  __tmp.reserve(_Nb);

  // Skip whitespace
  typename istream::sentry __sentry(__is);
  if (__sentry) {
    streambuf* __buf = __is.rdbuf();
    for (size_t __i = 0; __i < _Nb; ++__i) {
      static typename _Traits::int_type __eof = _Traits::eof();

      typename _Traits::int_type __c1 = __buf->sbumpc();
      if (_Traits::eq_int_type(__c1, __eof)) {
        __is.setstate(ios_base::eofbit);
        break;
      }
      else {
        typename _Traits::char_type __c2 = _Traits::to_char_type(__c1);
        char __c  = __is.narrow(__c2, '*');

        if (__c == '0' || __c == '1')
          __tmp.push_back(__c);
        else if (_Traits::eq_int_type(__buf->sputbackc(__c2), __eof)) {
          __is.setstate(ios_base::failbit);
          break;
        }
      }
    }

    if (__tmp.empty())
      __is.setstate(ios_base::failbit);
    else
      __x._M_copy_from_string(__tmp, __STATIC_CAST(size_t,0), _Nb);
  }

  return __is;
}

template <size_t _Nb>
ostream& _STLP_CALL
operator<<(ostream& __os, const bitset<_Nb>& __x) {
  string __tmp;
  __x._M_copy_to_string(__tmp);
  return __os << __tmp;
}

#    if !defined (_STLP_NO_WCHAR_T)

template <size_t _Nb>
wistream&  _STLP_CALL
operator>>(wistream& __is, bitset<_Nb>& __x) {
  typedef typename wstring::traits_type _Traits;
  wstring __tmp;
  __tmp.reserve(_Nb);

  // Skip whitespace
  typename wistream::sentry __sentry(__is);
  if (__sentry) {
    wstreambuf* __buf = __is.rdbuf();
    for (size_t __i = 0; __i < _Nb; ++__i) {
      static typename _Traits::int_type __eof = _Traits::eof();

      typename _Traits::int_type __c1 = __buf->sbumpc();
      if (_Traits::eq_int_type(__c1, __eof)) {
        __is.setstate(ios_base::eofbit);
        break;
      }
      else {
        typename _Traits::char_type __c2 = _Traits::to_char_type(__c1);
        char __c  = __is.narrow(__c2, '*');

        if (__c == '0' || __c == '1')
          __tmp.push_back(__c);
        else if (_Traits::eq_int_type(__buf->sputbackc(__c2), __eof)) {
          __is.setstate(ios_base::failbit);
          break;
        }
      }
    }

    if (__tmp.empty())
      __is.setstate(ios_base::failbit);
    else
      __x._M_copy_from_string(__tmp, __STATIC_CAST(size_t,0), _Nb);
  }

  return __is;
}

template <size_t _Nb>
wostream& _STLP_CALL
operator<<(wostream& __os, const bitset<_Nb>& __x) {
  wstring __tmp;
  __x._M_copy_to_string(__tmp);
  return __os << __tmp;
}

#    endif /* _STLP_NO_WCHAR_T */
#  endif
#endif

#endif /* _STLP_NON_TYPE_TMPL_PARAM_BUG */

#undef  bitset

_STLP_END_NAMESPACE

#undef __BITS_PER_WORD
#undef __BITSET_WORDS

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_bitset.c>
#endif

#endif /* _STLP_BITSET_H */

// Local Variables:
// mode:C++
// End:
