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

#include <locale>
#include <algorithm>
#include <typeinfo>

#include "c_locale.h"
#include "aligned_buffer.h"
#include "acquire_release.h"
#include "locale_impl.h"

_STLP_BEGIN_NAMESPACE

static const string _Nameless("*");

static inline bool is_C_locale_name (const char* name)
{ return ((name[0] == 'C') && (name[1] == 0)); }

locale::facet * _STLP_CALL _get_facet(locale::facet *f)
{
  if (f != 0)
    f->_M_incr();
  return f;
}

void _STLP_CALL _release_facet(locale::facet *&f)
{
  if ((f != 0) && (f->_M_decr() == 0)) {
    delete f;
    f = 0;
  }
}

size_t locale::id::_S_max = 27;

static void _Stl_loc_assign_ids();

static _Stl_aligned_buffer<_Locale_impl::Init> __Loc_init_buf;

_Locale_impl::Init::Init() {
  if (_M_count()._M_incr() == 1) {
    _Locale_impl::_S_initialize();
  }
}

_Locale_impl::Init::~Init() {
  if (_M_count()._M_decr() == 0) {
    _Locale_impl::_S_uninitialize();
  }
}

_Refcount_Base& _Locale_impl::Init::_M_count() const {
  static _Refcount_Base _S_count(0);
  return _S_count;
}

_Locale_impl::_Locale_impl(const char* s)
  : _Refcount_Base(0), name(s), facets_vec() {
  facets_vec.reserve( locale::id::_S_max );
  new (&__Loc_init_buf) Init();
}

_Locale_impl::_Locale_impl( _Locale_impl const& locimpl )
  : _Refcount_Base(0), name(locimpl.name), facets_vec() {
  for_each( locimpl.facets_vec.begin(), locimpl.facets_vec.end(), _get_facet);
  facets_vec = locimpl.facets_vec;
  new (&__Loc_init_buf) Init();
}

_Locale_impl::_Locale_impl( size_t n, const char* s)
  : _Refcount_Base(0), name(s), facets_vec(n, 0) {
  new (&__Loc_init_buf) Init();
}

_Locale_impl::~_Locale_impl() {
  (&__Loc_init_buf)->~Init();
  for_each( facets_vec.begin(), facets_vec.end(), _release_facet);
}

// Initialization of the locale system.  This must be called before
// any locales are constructed.  (Meaning that it must be called when
// the I/O library itself is initialized.)
void _STLP_CALL _Locale_impl::_S_initialize() {
  _Stl_loc_assign_ids();
  make_classic_locale();
}

// Release of the classic locale ressources. Has to be called after the last
// locale destruction and not only after the classic locale destruction as
// the facets can be shared between different facets.
void _STLP_CALL _Locale_impl::_S_uninitialize() {
  //Not necessary anymore as classic facets are now 'normal' dynamically allocated
  //facets with a reference counter telling to _release_facet when the facet can be
  //deleted.
  //free_classic_locale();
}

// _Locale_impl non-inline member functions.
void _STLP_CALL _Locale_impl::_M_throw_bad_cast() {
  _STLP_THROW(bad_cast());
}

void _Locale_impl::insert(_Locale_impl *from, const locale::id& n) {
  if (n._M_index > 0 && n._M_index < from->size()) {
    this->insert(from->facets_vec[n._M_index], n);
  }
}

locale::facet* _Locale_impl::insert(locale::facet *f, const locale::id& n) {
  if (f == 0 || n._M_index == 0)
    return 0;

  if (n._M_index >= facets_vec.size()) {
    facets_vec.resize(n._M_index + 1);
  }

  if (f != facets_vec[n._M_index])
  {
    _release_facet(facets_vec[n._M_index]);
    facets_vec[n._M_index] = _get_facet(f);
  }

  return f;
}

//
// <locale> content which is dependent on the name
//

/* Six functions, one for each category.  Each of them takes a
 * a name, constructs that appropriate category facets by name,
 * and inserts them into the locale. */
_Locale_name_hint* _Locale_impl::insert_ctype_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_ctype_default(buf);

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    _Locale_impl* i2 = locale::classic()._M_impl;
    this->insert(i2, ctype<char>::id);
    this->insert(i2, codecvt<char, char, mbstate_t>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, ctype<wchar_t>::id);
    this->insert(i2, codecvt<wchar_t, char, mbstate_t>::id);
#endif
  } else {
    locale::facet*    ct  = 0;
    locale::facet*    cvt = 0;
#ifndef _STLP_NO_WCHAR_T
    locale::facet* wct    = 0;
    locale::facet* wcvt   = 0;
#endif
    int __err_code;
    _Locale_ctype *__lct = _STLP_PRIV __acquire_ctype(name, buf, hint, &__err_code);
    if (!__lct) {
      locale::_M_throw_on_creation_failure(__err_code, name, "ctype");
      return hint;
    }

    if (hint == 0) hint = _Locale_get_ctype_hint(__lct);

    _STLP_TRY {
      ct   = new ctype_byname<char>(__lct);
    }
    _STLP_UNWIND(_STLP_PRIV __release_ctype(__lct));

    _STLP_TRY {
      cvt  = new codecvt_byname<char, char, mbstate_t>(name);
    }
    _STLP_UNWIND(delete ct);

#ifndef _STLP_NO_WCHAR_T
    _STLP_TRY {
      _Locale_ctype *__lwct = _STLP_PRIV __acquire_ctype(name, buf, hint, &__err_code);
      if (!__lwct) {
        locale::_M_throw_on_creation_failure(__err_code, name, "ctype");
        return hint;
      }

      _STLP_TRY {
        wct  = new ctype_byname<wchar_t>(__lwct);
      }
      _STLP_UNWIND(_STLP_PRIV __release_ctype(__lwct));
      
      _Locale_codecvt *__lwcvt = _STLP_PRIV __acquire_codecvt(name, buf, hint, &__err_code);
      if (__lwcvt) {
        _STLP_TRY {
          wcvt = new codecvt_byname<wchar_t, char, mbstate_t>(__lwcvt);
        }
        _STLP_UNWIND(_STLP_PRIV __release_codecvt(__lwcvt); delete wct);
      }
    }
    _STLP_UNWIND(delete cvt; delete ct);
#endif

    this->insert(ct, ctype<char>::id);
    this->insert(cvt, codecvt<char, char, mbstate_t>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(wct, ctype<wchar_t>::id);
    if (wcvt) this->insert(wcvt, codecvt<wchar_t, char, mbstate_t>::id);
#endif
  }
  return hint;
}

_Locale_name_hint* _Locale_impl::insert_numeric_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_numeric_default(buf);

  _Locale_impl* i2 = locale::classic()._M_impl;

  // We first insert name independant facets taken from the classic locale instance:
  this->insert(i2,
               num_put<char, ostreambuf_iterator<char, char_traits<char> >  >::id);
  this->insert(i2,
               num_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
  this->insert(i2,
               num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> >  >::id);
  this->insert(i2,
               num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    this->insert(i2, numpunct<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, numpunct<wchar_t>::id);
#endif
  }
  else {
    locale::facet* punct  = 0;
#ifndef _STLP_NO_WCHAR_T
    locale::facet* wpunct = 0;
#endif

    int __err_code;
    _Locale_numeric *__lpunct = _STLP_PRIV __acquire_numeric(name, buf, hint, &__err_code);
    if (!__lpunct) {
      locale::_M_throw_on_creation_failure(__err_code, name, "numpunct");
      return hint;
    }

    if (hint == 0) hint = _Locale_get_numeric_hint(__lpunct);
    _STLP_TRY {
      punct = new numpunct_byname<char>(__lpunct);
    }
    _STLP_UNWIND(_STLP_PRIV __release_numeric(__lpunct));

#ifndef _STLP_NO_WCHAR_T
    _Locale_numeric *__lwpunct = _STLP_PRIV __acquire_numeric(name, buf, hint, &__err_code);
    if (!__lwpunct) {
      delete punct;
      locale::_M_throw_on_creation_failure(__err_code, name, "numpunct");
      return hint;
    }
    if (__lwpunct) {
      _STLP_TRY {
        wpunct  = new numpunct_byname<wchar_t>(__lwpunct);
      }
      _STLP_UNWIND(_STLP_PRIV __release_numeric(__lwpunct); delete punct);
    }
#endif

    this->insert(punct, numpunct<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(wpunct, numpunct<wchar_t>::id);
#endif
  }
  return hint;
}

_Locale_name_hint* _Locale_impl::insert_time_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_time_default(buf);

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    _Locale_impl* i2 = locale::classic()._M_impl;
    this->insert(i2,
                 time_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    this->insert(i2,
                 time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2,
                 time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    this->insert(i2,
                 time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif
  } else {
    locale::facet *get = 0;
    locale::facet *put = 0;
#ifndef _STLP_NO_WCHAR_T
    locale::facet *wget = 0;
    locale::facet *wput = 0;
#endif

    int __err_code;
    _Locale_time *__time = _STLP_PRIV __acquire_time(name, buf, hint, &__err_code);
    if (!__time) {
      // time facets category is not mandatory for correct stream behavior so if platform
      // do not support it we do not generate a runtime_error exception.
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        _STLP_THROW_BAD_ALLOC;
      }
      return hint;
    }

    if (!hint) hint = _Locale_get_time_hint(__time);
    _STLP_TRY {
      get = new time_get_byname<char, istreambuf_iterator<char, char_traits<char> > >(__time);
      put = new time_put_byname<char, ostreambuf_iterator<char, char_traits<char> > >(__time);
#ifndef _STLP_NO_WCHAR_T
      wget = new time_get_byname<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(__time);
      wput = new time_put_byname<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(__time);
#endif
    }
#ifndef _STLP_NO_WCHAR_T
    _STLP_UNWIND(delete wget; delete put; delete get; _STLP_PRIV __release_time(__time));
#else
    _STLP_UNWIND(delete get; _STLP_PRIV __release_time(__time));
#endif

    _STLP_PRIV __release_time(__time);

    this->insert(get, time_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    this->insert(put, time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(wget, time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    this->insert(wput, time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif
  }
  return hint;
}

_Locale_name_hint* _Locale_impl::insert_collate_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_collate_default(buf);

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    _Locale_impl* i2 = locale::classic()._M_impl;
    this->insert(i2, collate<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, collate<wchar_t>::id);
#endif
  }
  else {
    locale::facet *col = 0;
#ifndef _STLP_NO_WCHAR_T
    locale::facet *wcol = 0;
#endif

    int __err_code;
    _Locale_collate *__coll = _STLP_PRIV __acquire_collate(name, buf, hint, &__err_code);
    if (!__coll) {
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        _STLP_THROW_BAD_ALLOC;
      }
      return hint;
    }

    if (hint == 0) hint = _Locale_get_collate_hint(__coll);
    _STLP_TRY {
      col = new collate_byname<char>(__coll);
    }
    _STLP_UNWIND(_STLP_PRIV __release_collate(__coll));

#ifndef _STLP_NO_WCHAR_T
    _Locale_collate *__wcoll = _STLP_PRIV __acquire_collate(name, buf, hint, &__err_code);
    if (!__wcoll) {
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        delete col;
        _STLP_THROW_BAD_ALLOC;
      }
    }
    if (__wcoll) {
      _STLP_TRY {
        wcol  = new collate_byname<wchar_t>(__wcoll);
      }
      _STLP_UNWIND(_STLP_PRIV __release_collate(__wcoll); delete col);
    }
#endif

    this->insert(col, collate<char>::id);
#ifndef _STLP_NO_WCHAR_T
    if (wcol) this->insert(wcol, collate<wchar_t>::id);
#endif
  }
  return hint;
}

_Locale_name_hint* _Locale_impl::insert_monetary_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_monetary_default(buf);

  _Locale_impl* i2 = locale::classic()._M_impl;

  // We first insert name independant facets taken from the classic locale instance:
  this->insert(i2, money_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
  this->insert(i2, money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
#ifndef _STLP_NO_WCHAR_T
  this->insert(i2, money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
  this->insert(i2, money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
#endif

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    this->insert(i2, moneypunct<char, false>::id);
    this->insert(i2, moneypunct<char, true>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, moneypunct<wchar_t, false>::id);
    this->insert(i2, moneypunct<wchar_t, true>::id);
#endif
  }
  else {
    locale::facet *punct   = 0;
    locale::facet *ipunct  = 0;

#ifndef _STLP_NO_WCHAR_T
    locale::facet* wpunct  = 0;
    locale::facet* wipunct = 0;
#endif

    int __err_code;
    _Locale_monetary *__mon = _STLP_PRIV __acquire_monetary(name, buf, hint, &__err_code);
    if (!__mon) {
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        _STLP_THROW_BAD_ALLOC;
      }
      return hint;
    }

    if (hint == 0) hint = _Locale_get_monetary_hint(__mon);

    _STLP_TRY {
      punct   = new moneypunct_byname<char, false>(__mon);
    }
    _STLP_UNWIND(_STLP_PRIV __release_monetary(__mon));

    _Locale_monetary *__imon = _STLP_PRIV __acquire_monetary(name, buf, hint, &__err_code);
    if (!__imon) {
      delete punct;
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        _STLP_THROW_BAD_ALLOC;
      }
      return hint;
    }

    _STLP_TRY {
      ipunct  = new moneypunct_byname<char, true>(__imon);
    }
    _STLP_UNWIND(_STLP_PRIV __release_monetary(__imon); delete punct);

#ifndef _STLP_NO_WCHAR_T
    _STLP_TRY {
      _Locale_monetary *__wmon = _STLP_PRIV __acquire_monetary(name, buf, hint, &__err_code);
      if (!__wmon) {
        if (__err_code == _STLP_LOC_NO_MEMORY) {
          _STLP_THROW_BAD_ALLOC;
        }
      }

      if (__wmon) {
        _STLP_TRY {
          wpunct  = new moneypunct_byname<wchar_t, false>(__wmon);
        }
        _STLP_UNWIND(_STLP_PRIV __release_monetary(__wmon));
      
        _Locale_monetary *__wimon = _STLP_PRIV __acquire_monetary(name, buf, hint, &__err_code);
        if (!__wimon) {
          delete wpunct;
          if (__err_code == _STLP_LOC_NO_MEMORY) {
            _STLP_THROW_BAD_ALLOC;
          }
          wpunct = 0;
        }
        else {
          _STLP_TRY {
            wipunct = new moneypunct_byname<wchar_t, true>(__wimon);
          }
          _STLP_UNWIND(_STLP_PRIV __release_monetary(__wimon); delete wpunct);
        }
      }
    }
    _STLP_UNWIND(delete ipunct; delete punct);
#endif

    this->insert(punct, moneypunct<char, false>::id);
    this->insert(ipunct, moneypunct<char, true>::id);
#ifndef _STLP_NO_WCHAR_T
    if (wpunct) this->insert(wpunct, moneypunct<wchar_t, false>::id);
    if (wipunct) this->insert(wipunct, moneypunct<wchar_t, true>::id);
#endif
  }
  return hint;
}

_Locale_name_hint* _Locale_impl::insert_messages_facets(const char* &name, char *buf, _Locale_name_hint* hint) {
  if (name[0] == 0)
    name = _Locale_messages_default(buf);

  if (name == 0 || name[0] == 0 || is_C_locale_name(name)) {
    _Locale_impl* i2 = locale::classic()._M_impl;
    this->insert(i2, messages<char>::id);
#ifndef _STLP_NO_WCHAR_T
    this->insert(i2, messages<wchar_t>::id);
#endif
  }
  else {
    locale::facet *msg  = 0;
#ifndef _STLP_NO_WCHAR_T
    locale::facet *wmsg = 0;
#endif

    int __err_code;
    _Locale_messages *__msg = _STLP_PRIV __acquire_messages(name, buf, hint, &__err_code);
    if (!__msg) {
      if (__err_code == _STLP_LOC_NO_MEMORY) {
        _STLP_THROW_BAD_ALLOC;
      }
      return hint;
    }

    _STLP_TRY {
      msg  = new messages_byname<char>(__msg);
    }
    _STLP_UNWIND(_STLP_PRIV __release_messages(__msg));

#ifndef _STLP_NO_WCHAR_T
    _STLP_TRY {
      _Locale_messages *__wmsg = _STLP_PRIV __acquire_messages(name, buf, hint, &__err_code);
      if (!__wmsg) {
        if (__err_code == _STLP_LOC_NO_MEMORY) {
          _STLP_THROW_BAD_ALLOC;
        }
      }

      if (__wmsg) {
        _STLP_TRY {
          wmsg = new messages_byname<wchar_t>(__wmsg);
        }
        _STLP_UNWIND(_STLP_PRIV __release_messages(__wmsg));
      }
    }
    _STLP_UNWIND(delete msg);
#endif

    this->insert(msg, messages<char>::id);
#ifndef _STLP_NO_WCHAR_T
    if (wmsg) this->insert(wmsg, messages<wchar_t>::id);
#endif
  }
  return hint;
}

static void _Stl_loc_assign_ids() {
  // This assigns ids to every facet that is a member of a category,
  // and also to money_get/put, num_get/put, and time_get/put
  // instantiated using ordinary pointers as the input/output
  // iterators.  (The default is [io]streambuf_iterator.)

  money_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index          = 8;
  money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index          = 9;
  num_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index            = 10;
  num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index            = 11;
  time_get<char, istreambuf_iterator<char, char_traits<char> > >::id._M_index           = 12;
  time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id._M_index           = 13;

#ifndef _STLP_NO_WCHAR_T
  money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index = 21;
  money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index = 22;
  num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index   = 23;
  num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > > ::id._M_index  = 24;
  time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index  = 25;
  time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id._M_index  = 26;
#endif
  //  locale::id::_S_max                               = 27;
}

// To access those static instance use the getter below, they guaranty
// a correct initialization.
static locale *_Stl_classic_locale = 0;
static locale *_Stl_global_locale = 0;

locale* _Stl_get_classic_locale() {
  static _Locale_impl::Init init;
  return _Stl_classic_locale;
}

locale* _Stl_get_global_locale() {
  static _Locale_impl::Init init;
  return _Stl_global_locale;
}

#if defined (_STLP_MSVC) || defined (__ICL) || defined (__ISCPP__) || defined (__DMC__)
/*
 * The following static variable needs to be initialized before STLport
 * users static variable in order for him to be able to use Standard
 * streams in its variable initialization.
 * This variable is here because MSVC do not allow to change the initialization
 * segment in a given translation unit, iostream.cpp already contains an
 * initialization segment specification.
 */
#  pragma warning (disable : 4073)
#  pragma init_seg(lib)
#endif

static ios_base::Init _IosInit;

void _Locale_impl::make_classic_locale() {
  // This funcion will be called once: during build classic _Locale_impl

  // The classic locale contains every facet that belongs to a category.
  static _Stl_aligned_buffer<_Locale_impl> _Locale_classic_impl_buf;
  _Locale_impl *classic = new(&_Locale_classic_impl_buf) _Locale_impl("C");

  locale::facet* classic_facets[] = {
    0,
    new collate<char>(1),
    new ctype<char>(0, false, 1),
    new codecvt<char, char, mbstate_t>(1),
    new moneypunct<char, true>(1),
    new moneypunct<char, false>(1),
    new numpunct<char>(1),
    new messages<char>(1),
    new money_get<char, istreambuf_iterator<char, char_traits<char> > >(1),
    new money_put<char, ostreambuf_iterator<char, char_traits<char> > >(1),
    new num_get<char, istreambuf_iterator<char, char_traits<char> > >(1),
    new num_put<char, ostreambuf_iterator<char, char_traits<char> > >(1),
    new time_get<char, istreambuf_iterator<char, char_traits<char> > >(1),
    new time_put<char, ostreambuf_iterator<char, char_traits<char> > >(1),
#ifndef _STLP_NO_WCHAR_T
    new collate<wchar_t>(1),
    new ctype<wchar_t>(1),
    new codecvt<wchar_t, char, mbstate_t>(1),
    new moneypunct<wchar_t, true>(1),
    new moneypunct<wchar_t, false>(1),
    new numpunct<wchar_t>(1),
    new messages<wchar_t>(1),
    new money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
    new money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
    new num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
    new num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
    new time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
    new time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >(1),
#endif
    0
  };

  const size_t nb_classic_facets = sizeof(classic_facets) / sizeof(locale::facet *);
  classic->facets_vec.reserve(nb_classic_facets);
  classic->facets_vec.assign(&classic_facets[0], &classic_facets[0] + nb_classic_facets);

  static locale _Locale_classic(classic);
  _Stl_classic_locale = &_Locale_classic;

  static locale _Locale_global(classic);
  _Stl_global_locale = &_Locale_global;
}

// Declarations of (non-template) facets' static data members
// size_t locale::id::_S_max = 27; // made before

locale::id collate<char>::id = { 1 };
locale::id ctype<char>::id = { 2 };
locale::id codecvt<char, char, mbstate_t>::id = { 3 };
locale::id moneypunct<char, true>::id = { 4 };
locale::id moneypunct<char, false>::id = { 5 };
locale::id numpunct<char>::id = { 6 } ;
locale::id messages<char>::id = { 7 };

#ifndef _STLP_NO_WCHAR_T
locale::id collate<wchar_t>::id = { 14 };
locale::id ctype<wchar_t>::id = { 15 };
locale::id codecvt<wchar_t, char, mbstate_t>::id = { 16 };
locale::id moneypunct<wchar_t, true>::id = { 17 } ;
locale::id moneypunct<wchar_t, false>::id = { 18 } ;
locale::id numpunct<wchar_t>::id = { 19 };
locale::id messages<wchar_t>::id = { 20 };
#endif

_STLP_DECLSPEC _Locale_impl* _STLP_CALL _get_Locale_impl(_Locale_impl *loc)
{
  _STLP_ASSERT( loc != 0 );
  loc->_M_incr();
  return loc;
}

void _STLP_CALL _release_Locale_impl(_Locale_impl *& loc)
{
  _STLP_ASSERT( loc != 0 );
  if (loc->_M_decr() == 0) {
    if (*loc != *_Stl_classic_locale)
      delete loc;
    else
      loc->~_Locale_impl();
    loc = 0;
  }
}

_STLP_DECLSPEC _Locale_impl* _STLP_CALL _copy_Nameless_Locale_impl(_Locale_impl *loc)
{
  _STLP_ASSERT( loc != 0 );
  _Locale_impl *loc_new = new _Locale_impl(*loc);
  loc_new->name = _Nameless;
  return loc_new;
}

/* _GetFacetId implementation have to be here in order to be in the same translation unit
 * as where id are initialize (in _Stl_loc_assign_ids) */
_STLP_MOVE_TO_PRIV_NAMESPACE

_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_get<char, istreambuf_iterator<char, char_traits<char> > >*)
{ return money_get<char, istreambuf_iterator<char, char_traits<char> > >::id; }
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_put<char, ostreambuf_iterator<char, char_traits<char> > >*)
{ return money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id; }
#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
#endif

_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_get<char, istreambuf_iterator<char, char_traits<char> > >*)
{ return num_get<char, istreambuf_iterator<char, char_traits<char> > >::id; }
#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
#endif

_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_put<char, ostreambuf_iterator<char, char_traits<char> > >*)
{ return num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id; }
#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
#endif

_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_get<char, istreambuf_iterator<char, char_traits<char> > >*)
{ return time_get<char, istreambuf_iterator<char, char_traits<char> > >::id; }
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_put<char, ostreambuf_iterator<char, char_traits<char> > >*)
{ return time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id; }
#ifndef _STLP_NO_WCHAR_T
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
_STLP_DECLSPEC locale::id& _STLP_CALL _GetFacetId(const time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >*)
{ return time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id; }
#endif

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

