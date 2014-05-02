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
// WARNING: This is an internal header file, included by other C++
// standard library headers.  You should not attempt to use this header
// file directly.


#ifndef _STLP_INTERNAL_LOCALE_H
#define _STLP_INTERNAL_LOCALE_H

#ifndef _STLP_INTERNAL_CSTDLIB
#  include <stl/_cstdlib.h>
#endif

#ifndef _STLP_INTERNAL_CWCHAR
#  include <stl/_cwchar.h>
#endif

#ifndef _STLP_INTERNAL_THREADS_H
#  include <stl/_threads.h>
#endif

#ifndef _STLP_STRING_FWD_H
#  include <stl/_string_fwd.h>
#endif

#include <stl/_facets_fwd.h>

_STLP_BEGIN_NAMESPACE

class _Locale_impl;        // Forward declaration of opaque type.
class locale;

template <class _CharT, class _Traits, class _Alloc>
bool __locale_do_operator_call(const locale& __loc,
                               const basic_string<_CharT, _Traits, _Alloc>& __x,
                               const basic_string<_CharT, _Traits, _Alloc>& __y);

_STLP_DECLSPEC _Locale_impl * _STLP_CALL _get_Locale_impl( _Locale_impl *locimpl );
_STLP_DECLSPEC _Locale_impl * _STLP_CALL _copy_Nameless_Locale_impl( _Locale_impl *locimpl );

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Facet>
bool _HasFacet(const locale& __loc, const _Facet* __facet) _STLP_NOTHROW;

template <class _Facet>
_Facet* _UseFacet(const locale& __loc, const _Facet* __facet);

template <class _Facet>
void _InsertFacet(locale& __loc, _Facet* __facet);

_STLP_MOVE_TO_STD_NAMESPACE

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND) || \
    defined (_STLP_SIGNAL_RUNTIME_COMPATIBILITY) || defined (_STLP_CHECK_RUNTIME_COMPATIBILITY)
#  define locale _STLP_NO_MEM_T_NAME(loc)
#endif

class _STLP_CLASS_DECLSPEC locale {
public:
  // types:
  class _STLP_CLASS_DECLSPEC facet : protected _Refcount_Base {
  protected:
    /* Here we filter __init_count user value to 0 or 1 because __init_count is a
     * size_t instance and _Refcount_Base use __stl_atomic_t instances that might
     * have lower sizeof and generate roll issues. 1 is enough to keep the facet
     * alive when required. */
    explicit facet(size_t __init_count = 0) : _Refcount_Base( __init_count == 0 ? 0 : 1 ) {}
    virtual ~facet();
    friend class locale;
    friend class _Locale_impl;
    friend facet * _STLP_CALL _get_facet( facet * );
    friend void _STLP_CALL _release_facet( facet *& );

  private:                        // Invalidate assignment and copying.
    facet(const facet& ) /* : _Refcount_Base(1) {} */;
    void operator=(const facet&);
  };

#if defined (__MVS__) || defined (__OS400__)
  struct
#else
  class
#endif
  _STLP_CLASS_DECLSPEC id {
  public:
    size_t _M_index;
    static size_t _S_max;
  };

  typedef int category;
  _STLP_STATIC_CONSTANT(category, none = 0x000);
  _STLP_STATIC_CONSTANT(category, collate = 0x010);
  _STLP_STATIC_CONSTANT(category, ctype = 0x020);
  _STLP_STATIC_CONSTANT(category, monetary = 0x040);
  _STLP_STATIC_CONSTANT(category, numeric = 0x100);
  _STLP_STATIC_CONSTANT(category, time = 0x200);
  _STLP_STATIC_CONSTANT(category, messages = 0x400);
  _STLP_STATIC_CONSTANT(category, all = collate | ctype | monetary | numeric | time | messages);

  // construct/copy/destroy:
  locale() _STLP_NOTHROW;
  locale(const locale&) _STLP_NOTHROW;
  explicit locale(const char *);
  locale(const locale&, const char*, category);

#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _Facet>
  locale(const locale& __loc, _Facet* __f) {
    if ( __f != 0 ) {
      this->_M_impl = _get_Locale_impl( _copy_Nameless_Locale_impl( __loc._M_impl ) );
      _STLP_PRIV _InsertFacet(*this, __f);
    } else {
      this->_M_impl = _get_Locale_impl( __loc._M_impl );
    }
  }
#endif

protected:
  // those are for internal use
  locale(_Locale_impl*);

public:
  locale(const locale&, const locale&, category);
  const locale& operator=(const locale&) _STLP_NOTHROW;

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
protected:
#endif
   ~locale() _STLP_NOTHROW;

public:
#if defined (_STLP_MEMBER_TEMPLATES) && !defined (_STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS) && \
   !defined(_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _Facet>
  locale combine(const locale& __loc) const {
    _Facet *__facet = 0;
    if (!_STLP_PRIV _HasFacet(__loc, __facet))
      _M_throw_on_combine_error(__loc.name());

    return locale(*this, _STLP_PRIV _UseFacet(__loc, __facet));
  }
#endif

  // locale operations:
  string name() const;

  bool operator==(const locale&) const;
  bool operator!=(const locale&) const;

#if !defined (_STLP_MEMBER_TEMPLATES) || defined (_STLP_INLINE_MEMBER_TEMPLATES) || (defined(__MWERKS__) && __MWERKS__ <= 0x2301)
  bool operator()(const string& __x, const string& __y) const;
#  ifndef _STLP_NO_WCHAR_T
  bool operator()(const wstring& __x, const wstring& __y) const;
#  endif
#elif !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  template <class _CharT, class _Traits, class _Alloc>
  bool operator()(const basic_string<_CharT, _Traits, _Alloc>& __x,
                  const basic_string<_CharT, _Traits, _Alloc>& __y) const
  { return __locale_do_operator_call(*this, __x, __y); }
#endif

  // global locale objects:
#if !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  static locale _STLP_CALL global(const locale&);
#else
  static _Locale_impl* _STLP_CALL global(const locale&);
#endif
  static const locale& _STLP_CALL classic();

//protected:                         // Helper functions for locale globals.
  facet* _M_get_facet(const id&) const;
  // same, but throws
  facet* _M_use_facet(const id&) const;
  static void _STLP_FUNCTION_THROWS _STLP_CALL _M_throw_on_combine_error(const string& name);
  static void _STLP_FUNCTION_THROWS _STLP_CALL _M_throw_on_null_name();
  static void _STLP_FUNCTION_THROWS _STLP_CALL _M_throw_on_creation_failure(int __err_code,
                                                                            const char* name, const char* facet);

//protected:                        // More helper functions.
  void _M_insert(facet* __f, id& __id);

  // friends:
  friend class _Locale_impl;

protected:                        // Data members
  _Locale_impl* _M_impl;
  _Locale_impl* _M_get_impl() const { return _M_impl; }
};

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND) || \
    defined (_STLP_SIGNAL_RUNTIME_COMPATIBILITY) || defined (_STLP_CHECK_RUNTIME_COMPATIBILITY)
#  undef locale
#  define _Locale _STLP_NO_MEM_T_NAME(loc)

class locale : public _Locale {
public:

  // construct/copy/destroy:
  locale() _STLP_NOTHROW {
#if defined (_STLP_CHECK_RUNTIME_COMPATIBILITY)
    _STLP_CHECK_RUNTIME_COMPATIBILITY();
#endif
  }
  locale(const locale& __loc) _STLP_NOTHROW : _Locale(__loc) {}
  explicit locale(const char *__str) : _Locale(__str) {}
  locale(const locale& __loc, const char* __str, category __cat)
    : _Locale(__loc, __str, __cat) {}

  template <class _Facet>
  locale(const locale& __loc, _Facet* __f) 
    : _Locale(__f != 0 ? _copy_Nameless_Locale_impl(__loc._M_impl) : __loc._M_impl) {
    if ( __f != 0 ) {
      _STLP_PRIV _InsertFacet(*this, __f);
    }
  }

private:
  // those are for internal use
  locale(_Locale_impl* __impl) : _Locale(__impl) {}
  locale(const _Locale& __loc) : _Locale(__loc) {}

public:

  locale(const locale& __loc1, const locale& __loc2, category __cat)
    : _Locale(__loc1, __loc2, __cat) {}

  const locale& operator=(const locale& __loc) _STLP_NOTHROW {
    _Locale::operator=(__loc);
    return *this;
  }

  template <class _Facet>
  locale combine(const locale& __loc) const {
    _Facet *__facet = 0;
    if (!_STLP_PRIV _HasFacet(__loc, __facet))
      _M_throw_on_combine_error(__loc.name());

    return locale(*this, _STLP_PRIV _UseFacet(__loc, __facet));
  }

  // locale operations:
  bool operator==(const locale& __loc) const { return _Locale::operator==(__loc); }
  bool operator!=(const locale& __loc) const { return _Locale::operator!=(__loc); }

  template <class _CharT, class _Traits, class _Alloc>
  bool operator()(const basic_string<_CharT, _Traits, _Alloc>& __x,
                  const basic_string<_CharT, _Traits, _Alloc>& __y) const
  { return __locale_do_operator_call(*this, __x, __y); }

  // global locale objects:
  static locale _STLP_CALL global(const locale& __loc) {
    return _Locale::global(__loc);
  }
  static const locale& _STLP_CALL classic() {
    return __STATIC_CAST(const locale&, _Locale::classic());
  }

  // friends:
  friend class _Locale_impl;
};

#  undef _Locale
#endif

//----------------------------------------------------------------------
// locale globals

template <class _Facet>
inline const _Facet&
#ifdef _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS
_Use_facet<_Facet>::operator *() const
#else
use_facet(const locale& __loc)
#endif
{
  _Facet *__facet = 0;
  return *(_STLP_PRIV _UseFacet(__loc, __facet));
}

template <class _Facet>
#ifdef _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS
struct has_facet {
  const locale& __loc;
  has_facet(const locale& __p_loc) : __loc(__p_loc) {}
  operator bool() const _STLP_NOTHROW
#else
inline bool has_facet(const locale& __loc) _STLP_NOTHROW
#endif
{
  _Facet *__facet = 0;
  return _STLP_PRIV _HasFacet(__loc, __facet);
}

#ifdef _STLP_NO_EXPLICIT_FUNCTION_TMPL_ARGS
}; // close class definition
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

/* _GetFacetId is a helper function that allow delaying access to
 * facet id static instance in the library source code to avoid
 * the other static instances that many compilers are generating
 * in all dynamic library or executable when instanciating facet
 * template class.
 */
template <class _Facet>
inline locale::id& _GetFacetId(const _Facet*)
{ return _Facet::id; }

_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_get<char, istreambuf_iterator<char, char_traits<char> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_put<char, ostreambuf_iterator<char, char_traits<char> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_get<char, istreambuf_iterator<char, char_traits<char> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_put<char, ostreambuf_iterator<char, char_traits<char> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_get<char, istreambuf_iterator<char, char_traits<char> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_put<char, ostreambuf_iterator<char, char_traits<char> > >*);

#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*);
#endif

template <class _Facet>
inline bool _HasFacet(const locale& __loc, const _Facet* __facet) _STLP_NOTHROW
{ return (__loc._M_get_facet(_GetFacetId(__facet)) != 0); }

template <class _Facet>
inline _Facet* _UseFacet(const locale& __loc, const _Facet* __facet)
{ return __STATIC_CAST(_Facet*, __loc._M_use_facet(_GetFacetId(__facet))); }

template <class _Facet>
inline void _InsertFacet(locale& __loc, _Facet* __facet)
{ __loc._M_insert(__facet, _GetFacetId(__facet)); }

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_LOCALE_H */

// Local Variables:
// mode:C++
// End:

