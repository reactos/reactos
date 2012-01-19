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

#ifndef LOCALE_IMPL_H
#define LOCALE_IMPL_H

#include <clocale>             // C locale header file.
#include <vector>
#include <string>
#include <locale>
#include "c_locale.h"

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_TEMPLATE_EXPORT)
//Export of _Locale_impl facets container:
#  if !defined (_STLP_USE_PTR_SPECIALIZATIONS)
//If we are using pointer specialization, vector<locale::facet*> will use
//the already exported vector<void*> implementation.
_STLP_EXPORT_TEMPLATE_CLASS allocator<locale::facet*>;

_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<locale::facet**, locale::facet*, allocator<locale::facet*> >;
_STLP_EXPORT_TEMPLATE_CLASS _Vector_base<locale::facet*, allocator<locale::facet*> >;

_STLP_MOVE_TO_STD_NAMESPACE
#  endif
#  if defined (_STLP_DEBUG)
_STLP_MOVE_TO_PRIV_NAMESPACE
#    define _STLP_NON_DBG_VECTOR _STLP_NON_DBG_NAME(vector)
_STLP_EXPORT_TEMPLATE_CLASS __construct_checker<_STLP_NON_DBG_VECTOR<locale::facet*, allocator<locale::facet*> > >;
_STLP_EXPORT_TEMPLATE_CLASS _STLP_NON_DBG_VECTOR<locale::facet*, allocator<locale::facet*> >;
#    undef _STLP_NON_DBG_VECTOR
_STLP_MOVE_TO_STD_NAMESPACE
#  endif

_STLP_EXPORT_TEMPLATE_CLASS vector<locale::facet*, allocator<locale::facet*> >;
#endif

//----------------------------------------------------------------------
// Class _Locale_impl
// This is the base class which implements access only and is supposed to
// be used for classic locale only
class _STLP_CLASS_DECLSPEC _Locale_impl : public _Refcount_Base {
  public:
    _Locale_impl(const char* s);
    _Locale_impl(const _Locale_impl&);
    _Locale_impl(size_t n, const char* s);

  private:
    ~_Locale_impl();

  public:
    size_t size() const { return facets_vec.size(); }

    basic_string<char, char_traits<char>, allocator<char> > name;

    static void _STLP_FUNCTION_THROWS _STLP_CALL _M_throw_bad_cast();

  private:
    void operator=(const _Locale_impl&);

  public:
    class _STLP_CLASS_DECLSPEC Init {
      public:
        Init();
        ~Init();
      private:
        _Refcount_Base& _M_count() const;
    };

    static void _STLP_CALL _S_initialize();
    static void _STLP_CALL _S_uninitialize();

    static void make_classic_locale();
    static void free_classic_locale();

    friend class Init;

  public:
    // void remove(size_t index);
    locale::facet* insert(locale::facet*, const locale::id& n);
    void insert(_Locale_impl* from, const locale::id& n);

    // Helper functions for byname construction of locales.
    _Locale_name_hint* insert_ctype_facets(const char* &name, char *buf, _Locale_name_hint* hint);
    _Locale_name_hint* insert_numeric_facets(const char* &name, char *buf, _Locale_name_hint* hint);
    _Locale_name_hint* insert_time_facets(const char* &name, char *buf, _Locale_name_hint* hint);
    _Locale_name_hint* insert_collate_facets(const char* &name, char *buf, _Locale_name_hint* hint);
    _Locale_name_hint* insert_monetary_facets(const char* &name, char *buf, _Locale_name_hint* hint);
    _Locale_name_hint* insert_messages_facets(const char* &name, char *buf, _Locale_name_hint* hint);

    bool operator != (const locale& __loc) const { return __loc._M_impl != this; }

  private:
    vector<locale::facet*> facets_vec;

  private:
    friend _Locale_impl * _STLP_CALL _copy_Nameless_Locale_impl( _Locale_impl * );
    friend void _STLP_CALL _release_Locale_impl( _Locale_impl *& loc );
#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND) || \
    defined (_STLP_SIGNAL_RUNTIME_COMPATIBILITY) || defined (_STLP_CHECK_RUNTIME_COMPATIBILITY)
    friend class _STLP_NO_MEM_T_NAME(loc);
#else
    friend class locale;
#endif
};

void _STLP_CALL _release_Locale_impl( _Locale_impl *& loc );

_STLP_END_NAMESPACE

#endif

// Local Variables:
// mode:C++
// End:
