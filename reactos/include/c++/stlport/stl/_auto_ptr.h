/*
 * Copyright (c) 1997-1999
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

#ifndef _STLP_AUTO_PTR_H
#define _STLP_AUTO_PTR_H

_STLP_BEGIN_NAMESPACE
// implementation primitive
class __ptr_base {
public:
  void* _M_p;
  void  __set(const volatile void* p) { _M_p = __CONST_CAST(void*,p); }
  void  __set(void* p) { _M_p = p; }
};

template <class _Tp>
class auto_ptr_ref {
public:
  __ptr_base& _M_r;
  _Tp* const _M_p;

  auto_ptr_ref(__ptr_base& __r, _Tp* __p) : _M_r(__r), _M_p(__p) {  }

  _Tp* release() const { _M_r.__set(__STATIC_CAST(void*, 0)); return _M_p; }

private:
  //explicitely defined as private to avoid warnings:
  typedef auto_ptr_ref<_Tp> _Self;
  _Self& operator = (_Self const&);
};

template<class _Tp>
class auto_ptr :  public __ptr_base {
public:
  typedef _Tp element_type;
  typedef auto_ptr<_Tp> _Self;

  _Tp* release() _STLP_NOTHROW {
    _Tp* __px = this->get();
    this->_M_p = 0;
    return __px;
  }

  void reset(_Tp* __px = 0) _STLP_NOTHROW {
    _Tp* __pt = this->get();
    if (__px != __pt)
      delete __pt;
    this->__set(__px);
  }

  _Tp* get() const _STLP_NOTHROW
#if !defined (__GNUC__) || (__GNUC__ > 2)
  { return __STATIC_CAST(_Tp*, _M_p); }
#else
  { return __REINTERPRET_CAST(_Tp*, _M_p); }
#endif

#if !defined (_STLP_NO_ARROW_OPERATOR)
  _Tp* operator->() const _STLP_NOTHROW {
    _STLP_VERBOSE_ASSERT(get() != 0, _StlMsg_AUTO_PTR_NULL)
    return get();
  }
#endif
  _Tp& operator*() const _STLP_NOTHROW {
    _STLP_VERBOSE_ASSERT(get() != 0, _StlMsg_AUTO_PTR_NULL)
    return *get();
  }

  explicit auto_ptr(_Tp* __px = 0) _STLP_NOTHROW { this->__set(__px); }

#if defined (_STLP_MEMBER_TEMPLATES)
#  if !defined (_STLP_NO_TEMPLATE_CONVERSIONS)
  template<class _Tp1> auto_ptr(auto_ptr<_Tp1>& __r) _STLP_NOTHROW {
    _Tp* __conversionCheck = __r.release();
    this->__set(__conversionCheck);
  }
#  endif
  template<class _Tp1> auto_ptr<_Tp>& operator=(auto_ptr<_Tp1>& __r) _STLP_NOTHROW {
    _Tp* __conversionCheck = __r.release();
    reset(__conversionCheck);
    return *this;
  }
#endif

  auto_ptr(_Self& __r) _STLP_NOTHROW { this->__set(__r.release()); }

  _Self& operator=(_Self& __r) _STLP_NOTHROW {
    reset(__r.release());
    return *this;
  }

  ~auto_ptr() _STLP_NOTHROW { /* boris : reset(0) might be better */ delete this->get(); }

  auto_ptr(auto_ptr_ref<_Tp> __r) _STLP_NOTHROW
  { this->__set(__r.release()); }

  _Self& operator=(auto_ptr_ref<_Tp> __r) _STLP_NOTHROW {
    reset(__r.release());
    return *this;
  }

#if defined(_STLP_MEMBER_TEMPLATES) && !defined(_STLP_NO_TEMPLATE_CONVERSIONS)
  template<class _Tp1> operator auto_ptr_ref<_Tp1>() _STLP_NOTHROW
  { return auto_ptr_ref<_Tp1>(*this, this->get()); }
  template<class _Tp1> operator auto_ptr<_Tp1>() _STLP_NOTHROW
  { return auto_ptr<_Tp1>(release()); }
#else
  operator auto_ptr_ref<_Tp>() _STLP_NOTHROW
  { return auto_ptr_ref<_Tp>(*this, this->get()); }
#endif
};
_STLP_END_NAMESPACE

#endif /* _STLP_AUTO_PTR_H */

// Local Variables:
// mode:C++
// End:
