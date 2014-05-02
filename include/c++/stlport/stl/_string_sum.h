/*
 * Copyright (c) 2003
 * Francois Dumont
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

#ifndef _STLP_STRING_SUM_H
#define _STLP_STRING_SUM_H

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

/*char wrapper to simulate basic_string*/
template <class _CharT>
struct __char_wrapper {
  typedef const _CharT& const_reference;

  __char_wrapper(_CharT __val) : _Val(__val) {}

  _CharT getValue() const { return _Val; }
  size_t size() const { return 1; }

  const_reference operator[] (size_t __n) const {
    //To avoid a check on __n we use this strange implementation
    return (&_Val)[__n];
  }

private:
  _CharT _Val;
};

/*C string wrapper to simulate basic_string*/
template <class _CharT>
struct __cstr_wrapper {
  typedef const _CharT& const_reference;

  __cstr_wrapper(const _CharT *__cstr, size_t __size) :
    _CStr(__cstr), _Size(__size) {}

  const _CharT* c_str() const { return _CStr; }

  size_t size() const { return _Size; }

  const_reference operator[] (size_t __n) const { return _CStr[__n]; }

private:
  const _CharT *_CStr;
  size_t _Size;
};

/*basic_string wrapper to ensure that we only store a reference to the original string and not copy it*/
template <class _CharT, class _Traits, class _Alloc>
struct __bstr_wrapper {
  typedef const _CharT& const_reference;
  typedef basic_string<_CharT, _Traits, _Alloc> _BString;

  __bstr_wrapper (_BString const& __s) :
    _BStr(__s) {}

  size_t size() const { return _BStr.size(); }

  const_reference operator[] (size_t __n) const { return _BStr[__n]; }

  _BString const& b_str() const { return _BStr; }

private:
  _BString const& _BStr;
};

struct __on_left {};
struct __on_right {};

template <class _CharT, class _Traits, class _Alloc,
          class _Left, class _Right,
          class _StorageDirection>
class __bstr_sum {
public:
  typedef basic_string<_CharT, _Traits, _Alloc> _BString;
  typedef typename _BString::const_reference const_reference;
  typedef typename _BString::const_iterator const_iterator;
  typedef typename _BString::const_reverse_iterator const_reverse_iterator;
  typedef typename _BString::size_type size_type;
  typedef typename _BString::allocator_type allocator_type;
  typedef __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDirection> _Self;

  __bstr_sum (_Left const& lhs, _Right const& rhs) :
    _lhs(lhs), _rhs(rhs) {}

  _Left const& getLhs() const { return _lhs; }
  _Right const& getRhs() const { return _rhs; }

  allocator_type get_allocator() const { return _M_get_storage(false).get_allocator(); }

  const_iterator begin() const { return _M_get_storage().begin(); }
  const_iterator end()   const { return _M_get_storage().end(); }
  const_reverse_iterator rbegin() const { return _M_get_storage().rbegin(); }
  const_reverse_iterator rend()   const { return _M_get_storage().rend(); }

  size_type size() const { return _lhs.size() + _rhs.size(); }
  size_type length() const { return size(); }

  size_t max_size() const { return _M_get_storage().max_size(); }
  size_type capacity() const { return size(); }
  bool empty() const { return size() == 0; }

  const_reference operator[](size_t __n) const
  { return (__n < _lhs.size())?_lhs[__n]:_rhs[__n - _lhs.size()]; }

  const_reference at(size_type __n) const
  { return _M_get_storage().at(__n); }

  //operator +=
  typedef __bstr_sum<_CharT, _Traits, _Alloc, _Self, __bstr_wrapper<_CharT, _Traits, _Alloc>, __on_left> _BStrOnLeft;
  _BStrOnLeft operator += (const _BString& __s) { return append(__s); }

  typedef __bstr_sum<_CharT, _Traits, _Alloc, _Self, __cstr_wrapper<_CharT>, __on_left> _CStrOnLeft;
  _CStrOnLeft operator += (const _CharT* __s) { return append(__s); }

  typedef __bstr_sum<_CharT, _Traits, _Alloc, _Self, __char_wrapper<_CharT>, __on_left> _CharOnLeft;
  _CharOnLeft operator += (_CharT __c) { return _CharOnLeft(*this, __c); }

  //append
  _BStrOnLeft append (const _BString& __s)
  { return _BStrOnLeft(*this, __s); }
  _BString& append(const _BString& __s, size_type __pos, size_type __n)
  { return _M_get_storage().append(__s, __pos, __n); }
  _CStrOnLeft append(const _CharT* __s) {
    const size_type __n = _Traits::length(__s);
    return _CStrOnLeft(*this, __cstr_wrapper<_CharT>(__s, __n));
  }
  _CStrOnLeft append(const _CharT* __s, size_type __n)
  { return _CStrOnLeft(*this, __cstr_wrapper<_CharT>(__s, __n)); }
  _BString& append(size_type __n, _CharT __c)
  {return _M_get_storage().append(__n, __c);}
  template <class _InputIter>
  _BString& append(_InputIter __first, _InputIter __last)
  {return _M_get_storage().append(__first, __last);}

  //assign
  _BString& assign(const _BString& __s) {return _M_get_storage().assign(__s);}
  _BString& assign(const _BString& __s, size_type __pos, size_type __n) {return _M_get_storage().assign(__s, __pos, __n);}
  _BString& assign(const _CharT* __s, size_type __n) {return _M_get_storage().assign(__s, __n);}
  _BString& assign(const _CharT* __s) {return _M_get_storage().assign(__s); }
  _BString& assign(size_type __n, _CharT __c) {return _M_get_storage().assign(__n, __c);}

  //insert
  _BString& insert(size_type __pos, const _BString& __s) {return _M_get_storage().insert(__pos, __s);}
  _BString& insert(size_type __pos, const _BString& __s, size_type __beg, size_type __n)
  {return _M_get_storage().insert(__pos, __s, __beg, __n);}
  _BString& insert(size_type __pos, const _CharT* __s, size_type __n) {return _M_get_storage().insert(__pos, __s, __n);}
  _BString& insert(size_type __pos, const _CharT* __s) {return _M_get_storage().insert(__pos, __s);}
  _BString& insert(size_type __pos, size_type __n, _CharT __c) {return _M_get_storage().insert(__pos, __n, __c);}

  //erase
  _BString& erase(size_type __pos = 0, size_type __n =_BString::npos) {return _M_get_storage().erase(__pos, __n);}

  //replace
  _BString& replace(size_type __pos, size_type __n, const _BString& __s)
  {return _M_get_storage().replace(__pos, __n, __s);}
  _BString& replace(size_type __pos1, size_type __n1, const _BString& __s, size_type __pos2, size_type __n2)
  {return _M_get_storage().replace(__pos1, __n1, __s, __pos2, __n2);}
  _BString& replace(size_type __pos, size_type __n1, const _CharT* __s, size_type __n2)
  {return _M_get_storage().replace(__pos, __n1, __s, __n2);}
  _BString& replace(size_type __pos, size_type __n1, const _CharT* __s)
  {return _M_get_storage().replace(__pos, __n1, __s);}
  _BString& replace(size_type __pos, size_type __n1, size_type __n2, _CharT __c)
  {return _M_get_storage().replace(__pos, __n1, __n2, __c);}

  size_type copy(_CharT* __s, size_type __n, size_type __pos = 0) const
  {return _M_get_storage().copy(__s, __n, __pos);}

  void swap(_BString& __s)
  {_M_get_storage().swap(__s);}

  const _CharT* c_str() const { return _M_get_storage().c_str(); }
  const _CharT* data()  const { return _M_get_storage().data(); }

  //find family
  size_type find(const _BString& __s, size_type __pos = 0) const { return _M_get_storage().find(__s, __pos); }
  size_type find(const _CharT* __s, size_type __pos = 0) const { return _M_get_storage().find(__s, __pos); }
  size_type find(const _CharT* __s, size_type __pos, size_type __n) const { return _M_get_storage().find(__s, __pos, __n); }
  size_type find(_CharT __c, size_type __pos = 0) const { return _M_get_storage().find(__c, __pos); }

  size_type rfind(const _BString& __s, size_type __pos = _BString::npos) const { return _M_get_storage().rfind(__s, __pos); }
  size_type rfind(const _CharT* __s, size_type __pos = _BString::npos) const { return _M_get_storage().rfind(__s, __pos); }
  size_type rfind(const _CharT* __s, size_type __pos, size_type __n) const { return _M_get_storage().rfind(__s, __pos, __n); }
  size_type rfind(_CharT __c, size_type __pos = _BString::npos) const { return _M_get_storage().rfind(__c, __pos); }

  size_type find_first_of(const _BString& __s, size_type __pos = 0) const
  { return _M_get_storage().find_first_of(__s, __pos); }
  size_type find_first_of(const _CharT* __s, size_type __pos = 0) const
  { return _M_get_storage().find_first_of(__s, __pos); }
  size_type find_first_of(const _CharT* __s, size_type __pos, size_type __n) const
  { return _M_get_storage().find_first_of(__s, __pos, __n); }
  size_type find_first_of(_CharT __c, size_type __pos = 0) const
  { return _M_get_storage().find(__c, __pos); }

  size_type find_last_of(const _BString& __s, size_type __pos = _BString::npos) const
  { return _M_get_storage().find_last_of(__s, __pos); }
  size_type find_last_of(const _CharT* __s, size_type __pos = _BString::npos) const
  { return _M_get_storage().find_last_of(__s, __pos); }
  size_type find_last_of(const _CharT* __s, size_type __pos, size_type __n) const
  { return _M_get_storage().find_last_of(__s, __pos, __n); }
  size_type find_last_of(_CharT __c, size_type __pos = _BString::npos) const
  { return _M_get_storage().rfind(__c, __pos); }

  size_type find_first_not_of(const _BString& __s, size_type __pos = 0) const
  { return _M_get_storage().find_first_not_of(__s, __pos); }
  size_type find_first_not_of(const _CharT* __s, size_type __pos = 0) const
  { return _M_get_storage().find_first_not_of(__s, __pos); }
  size_type find_first_not_of(const _CharT* __s, size_type __pos, size_type __n) const
  { return _M_get_storage().find_first_not_of(__s, __pos, __n); }
  size_type find_first_not_of(_CharT __c, size_type __pos = 0) const
  { return _M_get_storage().find_first_not_of(__c, __pos); }

  size_type find_last_not_of(const _BString& __s, size_type __pos = _BString::npos) const
  { return _M_get_storage().find_last_not_of(__s, __pos); }
  size_type find_last_not_of(const _CharT* __s, size_type __pos =_BString:: npos) const
  { return _M_get_storage().find_last_not_of(__s, __pos); }
  size_type find_last_not_of(const _CharT* __s, size_type __pos, size_type __n) const
  { return _M_get_storage().find_last_not_of(__s, __pos, __n); }
  size_type find_last_not_of(_CharT __c, size_type __pos = _BString::npos) const
  { return _M_get_storage().find_last_not_of(__c, __pos); }

  _BString substr(size_type __pos = 0, size_type __n = _BString::npos) const
  { return _M_get_storage().substr(__pos, __n); }

  //compare
  int compare(const _BString& __s) const
  { return _M_get_storage().compare(__s); }
  int compare(size_type __pos1, size_type __n1, const _Self& __s) const
  { return _M_get_storage().compare(__pos1, __n1, __s); }
  int compare(size_type __pos1, size_type __n1, const _Self& __s, size_type __pos2, size_type __n2) const
  { return _M_get_storage().compare(__pos1, __n1, __s, __pos2, __n2); }
  int compare(const _CharT* __s) const
  { return _M_get_storage().compare(__s); }
  int compare(size_type __pos1, size_type __n1, const _CharT* __s) const
  { return _M_get_storage().compare(__pos1, __n1, __s); }
  int compare(size_type __pos1, size_type __n1, const _CharT* __s, size_type __n2) const
  { return _M_get_storage().compare(__pos1, __n1, __s, __n2); }

  //Returns the underlying basic_string representation of the template expression
  //The non const method will always initialise it.
  _BString& _M_get_storage()
  { return _rhs._M_get_storage(*this, _StorageDirection()); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref,
                           __on_left const& /*StorageDir*/)
  { return _lhs._M_get_storage(__ref); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref,
                           __on_right const& /*StorageDir*/)
  { return _rhs._M_get_storage(__ref); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref)
  { return _M_get_storage(__ref, _StorageDirection()); }

  //The const method can be invoked without initialising the basic_string so avoiding dynamic allocation.
  _BString const& _M_get_storage(bool __do_init = true) const
  { return _M_get_storage(*this, __do_init, _StorageDirection()); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString const& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref,
                                 bool __do_init, __on_left const& /*StorageDir*/) const
  { return _lhs._M_get_storage(__ref, __do_init); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString const& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref,
                                 bool __do_init, __on_right const& /*StorageDir*/) const
  { return _rhs._M_get_storage(__ref, __do_init); }

  template <class _Lhs, class _Rhs, class _StorageDir>
  _BString const& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Lhs, _Rhs, _StorageDir>  const& __ref,
                                 bool __do_init) const
  { return _M_get_storage(__ref, __do_init, _StorageDirection()); }

private:
  _Left  _lhs;
  _Right _rhs;
};

/*
 * For this operator we choose to use the right part as the storage part
 */
template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline __bstr_sum<_CharT, _Traits, _Alloc,
                  __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1>,
                  __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2>,
                  __on_right> _STLP_CALL
operator + (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
            const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs) {
  return __bstr_sum<_CharT, _Traits, _Alloc,
                    __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1>,
                    __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2>,
                    __on_right>(__lhs, __rhs);
}

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator == (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
             const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return (__lhs.size() == __rhs.size()) && (__lhs._M_get_storage() == __rhs._M_get_storage()); }

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator < (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
            const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return __lhs._M_get_storage() < __rhs._M_get_storage(); }

#ifdef _STLP_USE_SEPARATE_RELOPS_NAMESPACE

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator != (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
             const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return !(__lhs == __rhs); }

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator > (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
            const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return __rhs < __lhs; }

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator <= (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
             const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return !(__rhs < __lhs); }

template <class _CharT, class _Traits, class _Alloc,
          class _Lh1, class _Rh1, class _StoreDir1,
          class _Lh2, class _Rh2, class _StoreDir2>
inline bool _STLP_CALL
operator >= (const __bstr_sum<_CharT, _Traits, _Alloc, _Lh1, _Rh1, _StoreDir1> &__lhs,
             const __bstr_sum<_CharT, _Traits, _Alloc, _Lh2, _Rh2, _StoreDir2> &__rhs)
{ return !(__lhs < __rhs); }

#endif /* _STLP_USE_SEPARATE_RELOPS_NAMESPACE */


/*
 * This class will be used to simulate a temporary string that is required for
 * a call to the c_str method on the __bstr_sum class.
 */

template <class _CharT, class _Traits, class _Alloc>
struct __sum_storage_elem {
  typedef __sum_storage_elem<_CharT, _Traits, _Alloc> _Self;
  typedef basic_string<_CharT, _Traits, _Alloc> _BString;

  __sum_storage_elem(_Alloc __alloc) : _M_init(false), _M_storage(__alloc)
  {}

  template <class _Left, class _Right, class _StorageDir>
  void _M_Init(__bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir>  const& __ref) const {
    if (!_M_init) {
      _STLP_MUTABLE(_Self, _M_storage) = __ref;
      _STLP_MUTABLE(_Self, _M_init) = true;
    }
  }

  template <class _Left, class _Right, class _StorageDir>
  _BString const& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir>  const& __ref,
                                 bool __do_init) const {
    if (__do_init) {
      _M_Init(__ref);
    }
    return _M_storage;
  }
  template <class _Left, class _Right, class _StorageDir>
  _BString& _M_get_storage(__bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir>  const& __ref) {
    _M_Init(__ref);
    return _M_storage;
  }

  size_t size() const { return 0; }
  _CharT const& operator[](size_t __n) const
  { return __STATIC_CAST(_CharT*, 0)[__n]; }

private:
  mutable bool _M_init;
  mutable basic_string<_CharT, _Traits, _Alloc> _M_storage;
};

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /*_STLP_STRING_SUM_H*/
