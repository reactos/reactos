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
#include <stdexcept>

#include "c_locale.h"
#include "locale_impl.h"

_STLP_BEGIN_NAMESPACE

static const string _Nameless("*");

static inline bool is_C_locale_name (const char* name)
{ return ((name[0] == 'C') && (name[1] == 0)); }

locale* _Stl_get_classic_locale();
locale* _Stl_get_global_locale();

#if defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND) || \
    defined (_STLP_SIGNAL_RUNTIME_COMPATIBILITY) || defined (_STLP_CHECK_RUNTIME_COMPATIBILITY)
#  define locale _STLP_NO_MEM_T_NAME(loc)
#endif

locale::facet::~facet() {}

#if !defined (_STLP_MEMBER_TEMPLATES) || defined (_STLP_INLINE_MEMBER_TEMPLATES)
// members that fail to be templates
bool locale::operator()(const string& __x,
                        const string& __y) const
{ return __locale_do_operator_call(*this, __x, __y); }

#  if !defined (_STLP_NO_WCHAR_T)
bool locale::operator()(const wstring& __x,
                        const wstring& __y) const
{ return __locale_do_operator_call(*this, __x, __y); }
#  endif
#endif

void _STLP_CALL locale::_M_throw_on_null_name()
{ _STLP_THROW(runtime_error("Invalid null locale name")); }

void _STLP_CALL locale::_M_throw_on_combine_error(const string& name) {
  string what = "Unable to find facet";
  what += " in ";
  what += name.empty() ? "system" : name.c_str();
  what += " locale";
  _STLP_THROW(runtime_error(what.c_str()));
}

void _STLP_CALL locale::_M_throw_on_creation_failure(int __err_code,
                                                     const char* name, const char* facet) {
  string what;
  switch (__err_code) {
    case _STLP_LOC_UNSUPPORTED_FACET_CATEGORY:
      what = "No platform localization support for ";
      what += facet;
      what += " facet category, unable to create facet for ";
      what += name[0] == 0 ? "system" : name;
      what += " locale";
      break;
    case _STLP_LOC_NO_PLATFORM_SUPPORT:
      what = "No platform localization support, unable to create ";
      what += name[0] == 0 ? "system" : name;
      what += " locale";
      break;
    default:
    case _STLP_LOC_UNKNOWN_NAME:
      what = "Unable to create facet ";
      what += facet;
      what += " from name '";
      what += name;
      what += "'";
      break;
    case _STLP_LOC_NO_MEMORY:
      _STLP_THROW_BAD_ALLOC;
      break;
  }

  _STLP_THROW(runtime_error(what.c_str()));
}

// Takes a reference to a locale::id, assign a numeric index if not already
// affected and returns it. The returned index is always positive.
static const locale::id& _Stl_loc_get_index(locale::id& id) {
  if (id._M_index == 0) {
#if defined (_STLP_ATOMIC_INCREMENT) && !defined (_STLP_WIN95_LIKE)
    static _STLP_VOLATILE __stl_atomic_t _S_index = __STATIC_CAST(__stl_atomic_t, locale::id::_S_max);
    id._M_index = _STLP_ATOMIC_INCREMENT(&_S_index);
#else
    static _STLP_STATIC_MUTEX _Index_lock _STLP_MUTEX_INITIALIZER;
    _STLP_auto_lock sentry(_Index_lock);
    size_t new_index = locale::id::_S_max++;
    id._M_index = new_index;
#endif
  }
  return id;
}

// Default constructor: create a copy of the global locale.
locale::locale() _STLP_NOTHROW
  : _M_impl(_get_Locale_impl(_Stl_get_global_locale()->_M_impl))
{}

// Copy constructor
locale::locale(const locale& L) _STLP_NOTHROW
  : _M_impl( _get_Locale_impl( L._M_impl ) )
{}

void locale::_M_insert(facet* f, locale::id& n) {
  if (f)
    _M_impl->insert(f, _Stl_loc_get_index(n));
}

locale::locale( _Locale_impl* impl ) :
  _M_impl( _get_Locale_impl( impl ) )
{}

// Create a locale from a name.
locale::locale(const char* name)
  : _M_impl(0) {
  if (!name)
    _M_throw_on_null_name();

  if (is_C_locale_name(name)) {
    _M_impl = _get_Locale_impl( locale::classic()._M_impl );
    return;
  }

  _Locale_impl* impl = 0;
  _STLP_TRY {
    impl = new _Locale_impl(locale::id::_S_max, name);

    // Insert categories one at a time.
    _Locale_name_hint *hint = 0;
    const char* ctype_name = name;
    char ctype_buf[_Locale_MAX_SIMPLE_NAME];
    const char* numeric_name = name;
    char numeric_buf[_Locale_MAX_SIMPLE_NAME];
    const char* time_name = name;
    char time_buf[_Locale_MAX_SIMPLE_NAME];
    const char* collate_name = name;
    char collate_buf[_Locale_MAX_SIMPLE_NAME];
    const char* monetary_name = name;
    char monetary_buf[_Locale_MAX_SIMPLE_NAME];
    const char* messages_name = name;
    char messages_buf[_Locale_MAX_SIMPLE_NAME];
    hint = impl->insert_ctype_facets(ctype_name, ctype_buf, hint);
    hint = impl->insert_numeric_facets(numeric_name, numeric_buf, hint);
    hint = impl->insert_time_facets(time_name, time_buf, hint);
    hint = impl->insert_collate_facets(collate_name, collate_buf, hint);
    hint = impl->insert_monetary_facets(monetary_name, monetary_buf, hint);
    impl->insert_messages_facets(messages_name, messages_buf, hint);

    // Try to use a normalize locale name in order to have the == operator
    // to behave correctly:
    if (strcmp(ctype_name, numeric_name) == 0 &&
        strcmp(ctype_name, time_name) == 0 &&
        strcmp(ctype_name, collate_name) == 0 &&
        strcmp(ctype_name, monetary_name) == 0 &&
        strcmp(ctype_name, messages_name) == 0) {
      impl->name = ctype_name;
    }
    // else we keep current name.

    // reassign impl
    _M_impl = _get_Locale_impl( impl );
  }
  _STLP_UNWIND(delete impl);
}

static void _Stl_loc_combine_names_aux(_Locale_impl* L,
                                       const char* name,
                                       const char* ctype_name, const char* time_name, const char* numeric_name,
                                       const char* collate_name, const char* monetary_name, const char* messages_name,
                                       locale::category c) {
  // This function is only called when names has been validated so using _Locale_extract_*_name
  // can't fail.
  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  L->name = string("LC_CTYPE=") + _Locale_extract_ctype_name((c & locale::ctype) ? ctype_name : name, buf, 0, &__err_code) + ";";
  L->name += string("LC_TIME=") + _Locale_extract_time_name((c & locale::time) ? time_name : name, buf, 0, &__err_code) + ";";
  L->name += string("LC_NUMERIC=") + _Locale_extract_numeric_name((c & locale::numeric) ? numeric_name : name, buf, 0, &__err_code) + ";";
  L->name += string("LC_COLLATE=") + _Locale_extract_collate_name((c & locale::collate) ? collate_name : name, buf, 0, &__err_code) + ";";
  L->name += string("LC_MONETARY=") + _Locale_extract_monetary_name((c & locale::monetary) ? monetary_name : name, buf, 0, &__err_code) + ";";
  L->name += string("LC_MESSAGES=") + _Locale_extract_messages_name((c & locale::messages) ? messages_name : name, buf, 0, &__err_code);
}

// Give L a name where all facets except those in category c
// are taken from name1, and those in category c are taken from name2.
static void _Stl_loc_combine_names(_Locale_impl* L,
                                   const char* name1, const char* name2,
                                   locale::category c) {
  if ((c & locale::all) == 0 || strcmp(name1, name1) == 0)
    L->name = name1;
  else if ((c & locale::all) == locale::all)
    L->name = name2;
  else {
    _Stl_loc_combine_names_aux(L, name1, name2, name2, name2, name2, name2, name2, c);
  }
}

static void _Stl_loc_combine_names(_Locale_impl* L,
                                   const char* name,
                                   const char* ctype_name, const char* time_name, const char* numeric_name,
                                   const char* collate_name, const char* monetary_name, const char* messages_name,
                                   locale::category c) {
  if ((c & locale::all) == 0 || (strcmp(name, ctype_name) == 0 &&
                                 strcmp(name, time_name) == 0 &&
                                 strcmp(name, numeric_name) == 0 &&
                                 strcmp(name, collate_name) == 0 &&
                                 strcmp(name, monetary_name) == 0 &&
                                 strcmp(name, messages_name) == 0))
    L->name = name;
  else if ((c & locale::all) == locale::all && strcmp(ctype_name, time_name) == 0 &&
                                               strcmp(ctype_name, numeric_name) == 0 &&
                                               strcmp(ctype_name, collate_name) == 0 &&
                                               strcmp(ctype_name, monetary_name) == 0 &&
                                               strcmp(ctype_name, messages_name) == 0)
    L->name = ctype_name;
  else {
    _Stl_loc_combine_names_aux(L, name, ctype_name, time_name, numeric_name, collate_name, monetary_name, messages_name, c);
  }
}


// Create a locale that's a copy of L, except that all of the facets
// in category c are instead constructed by name.
locale::locale(const locale& L, const char* name, locale::category c)
  : _M_impl(0) {
  if (!name)
    _M_throw_on_null_name();

  if (_Nameless == name)
    _STLP_THROW(runtime_error((string("Invalid locale name '") + _Nameless + "'").c_str()));

  _Locale_impl* impl = 0;

  _STLP_TRY {
    impl = new _Locale_impl(*L._M_impl);

    _Locale_name_hint *hint = 0;
    const char* ctype_name = name;
    char ctype_buf[_Locale_MAX_SIMPLE_NAME];
    const char* numeric_name = name;
    char numeric_buf[_Locale_MAX_SIMPLE_NAME];
    const char* time_name = name;
    char time_buf[_Locale_MAX_SIMPLE_NAME];
    const char* collate_name = name;
    char collate_buf[_Locale_MAX_SIMPLE_NAME];
    const char* monetary_name = name;
    char monetary_buf[_Locale_MAX_SIMPLE_NAME];
    const char* messages_name = name;
    char messages_buf[_Locale_MAX_SIMPLE_NAME];
    if (c & locale::ctype)
      hint = impl->insert_ctype_facets(ctype_name, ctype_buf, hint);
    if (c & locale::numeric)
      hint = impl->insert_numeric_facets(numeric_name, numeric_buf, hint);
    if (c & locale::time)
      hint = impl->insert_time_facets(time_name, time_buf, hint);
    if (c & locale::collate)
      hint = impl->insert_collate_facets(collate_name, collate_buf, hint);
    if (c & locale::monetary)
      hint = impl->insert_monetary_facets(monetary_name, monetary_buf,hint);
    if (c & locale::messages)
      impl->insert_messages_facets(messages_name, messages_buf, hint);

    _Stl_loc_combine_names(impl, L._M_impl->name.c_str(),
                           ctype_name, time_name, numeric_name,
                           collate_name, monetary_name, messages_name, c);
    _M_impl = _get_Locale_impl( impl );
  }
  _STLP_UNWIND(delete impl)
}

// Contruct a new locale where all facets that aren't in category c
// come from L1, and all those that are in category c come from L2.
locale::locale(const locale& L1, const locale& L2, category c)
  : _M_impl(0) {
  _Locale_impl* impl = new _Locale_impl(*L1._M_impl);

  _Locale_impl* i2 = L2._M_impl;

  if (L1.name() != _Nameless && L2.name() != _Nameless)
    _Stl_loc_combine_names(impl, L1._M_impl->name.c_str(), L2._M_impl->name.c_str(), c);
  else {
    impl->name = _Nameless;
  }

  if (c & collate) {
    impl->insert( i2, _STLP_STD::collate<char>::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::collate<wchar_t>::id);
# endif
  }
  if (c & ctype) {
    impl->insert( i2, _STLP_STD::ctype<char>::id);
    impl->insert( i2, _STLP_STD::codecvt<char, char, mbstate_t>::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::ctype<wchar_t>::id);
    impl->insert( i2, _STLP_STD::codecvt<wchar_t, char, mbstate_t>::id);
# endif
  }
  if (c & monetary) {
    impl->insert( i2, _STLP_STD::moneypunct<char, true>::id);
    impl->insert( i2, _STLP_STD::moneypunct<char, false>::id);
    impl->insert( i2, _STLP_STD::money_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    impl->insert( i2, _STLP_STD::money_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::moneypunct<wchar_t, true>::id);
    impl->insert( i2, _STLP_STD::moneypunct<wchar_t, false>::id);
    impl->insert( i2, _STLP_STD::money_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    impl->insert( i2, _STLP_STD::money_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
# endif
  }
  if (c & numeric) {
    impl->insert( i2, _STLP_STD::numpunct<char>::id);
    impl->insert( i2, _STLP_STD::num_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    impl->insert( i2, _STLP_STD::num_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::numpunct<wchar_t>::id);
    impl->insert( i2, _STLP_STD::num_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    impl->insert( i2, _STLP_STD::num_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
# endif
  }
  if (c & time) {
    impl->insert( i2, _STLP_STD::time_get<char, istreambuf_iterator<char, char_traits<char> > >::id);
    impl->insert( i2, _STLP_STD::time_put<char, ostreambuf_iterator<char, char_traits<char> > >::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::time_get<wchar_t, istreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
    impl->insert( i2, _STLP_STD::time_put<wchar_t, ostreambuf_iterator<wchar_t, char_traits<wchar_t> > >::id);
# endif
  }
  if (c & messages) {
    impl->insert( i2, _STLP_STD::messages<char>::id);
# ifndef _STLP_NO_WCHAR_T
    impl->insert( i2, _STLP_STD::messages<wchar_t>::id);
# endif
  }
  _M_impl = _get_Locale_impl( impl );
}

// Destructor.
locale::~locale() _STLP_NOTHROW {
  if (_M_impl)
    _release_Locale_impl(_M_impl);
}

// Assignment operator.  Much like the copy constructor: just a bit of
// pointer twiddling.
const locale& locale::operator=(const locale& L) _STLP_NOTHROW {
  if (this->_M_impl != L._M_impl) {
    if (this->_M_impl)
      _release_Locale_impl(this->_M_impl);
    this->_M_impl = _get_Locale_impl(L._M_impl);
  }
  return *this;
}

locale::facet* locale::_M_get_facet(const locale::id& n) const {
  return n._M_index < _M_impl->size() ? _M_impl->facets_vec[n._M_index] : 0;
}

locale::facet* locale::_M_use_facet(const locale::id& n) const {
  locale::facet* f = (n._M_index < _M_impl->size() ? _M_impl->facets_vec[n._M_index] : 0);
  if (!f)
    _M_impl->_M_throw_bad_cast();
  return f;
}

string locale::name() const {
  return _M_impl->name;
}

// Compare two locales for equality.
bool locale::operator==(const locale& L) const {
  return this->_M_impl == L._M_impl ||
         (this->name() == L.name() && this->name() != _Nameless);
}

bool locale::operator!=(const locale& L) const {
  return !(*this == L);
}

// static data members.

const locale& _STLP_CALL locale::classic() {
  return *_Stl_get_classic_locale();
}

#if !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
locale _STLP_CALL locale::global(const locale& L) {
#else
_Locale_impl* _STLP_CALL locale::global(const locale& L) {
#endif
  locale old(_Stl_get_global_locale()->_M_impl);
  if (_Stl_get_global_locale()->_M_impl != L._M_impl) {
    _release_Locale_impl(_Stl_get_global_locale()->_M_impl);
    // this assign should be atomic, should be fixed here:
    _Stl_get_global_locale()->_M_impl = _get_Locale_impl(L._M_impl);

    // Set the global C locale, if appropriate.
#if !defined(_STLP_NO_LOCALE_SUPPORT)
    if (L.name() != _Nameless)
      setlocale(LC_ALL, L.name().c_str());
#endif
  }

#if !defined (_STLP_USE_MSVC6_MEM_T_BUG_WORKAROUND)
  return old;
#else
  return old._M_impl;
#endif
}

#if !defined (_STLP_STATIC_CONST_INIT_BUG) && !defined (_STLP_NO_STATIC_CONST_DEFINITION)
const locale::category locale::none;
const locale::category locale::collate;
const locale::category locale::ctype;
const locale::category locale::monetary;
const locale::category locale::numeric;
const locale::category locale::time;
const locale::category locale::messages;
const locale::category locale::all;
#endif

_STLP_END_NAMESPACE

