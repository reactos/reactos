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

#include <typeinfo>

#include "message_facets.h"
#include "acquire_release.h"

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

void _Catalog_locale_map::insert(nl_catd_type key, const locale& L) {
  _STLP_TRY {
#if !defined (_STLP_NO_TYPEINFO) && !defined (_STLP_NO_RTTI)
    // Don't bother to do anything unless we're using a non-default ctype facet
#  ifdef _STLP_NO_WCHAR_T
    typedef char _Char;
#  else
    typedef wchar_t _Char;
#  endif

    typedef ctype<_Char> wctype;
    wctype const& wct = use_facet<wctype>(L);
    if (typeid(wct) != typeid(wctype)) {
#endif
      if (!M)
        M = new map_type;

      M->insert(map_type::value_type(key, L));
#if !defined (_STLP_NO_TYPEINFO) && !defined (_STLP_NO_RTTI)
    }
#endif
  }
  _STLP_CATCH_ALL {}
}

void _Catalog_locale_map::erase(nl_catd_type key) {
  if (M)
    M->erase(key);
}

locale _Catalog_locale_map::lookup(nl_catd_type key) const {
  if (M) {
    map_type::const_iterator i = M->find(key);
    return i != M->end() ? (*i).second : locale::classic();
  }
  else
    return locale::classic();
}


#if defined (_STLP_USE_NL_CATD_MAPPING)
_STLP_VOLATILE __stl_atomic_t _Catalog_nl_catd_map::_count = 0;

messages_base::catalog _Catalog_nl_catd_map::insert(nl_catd_type cat) {
  messages_base::catalog &res = Mr[cat];
  if ( res == 0 ) {
#if defined (_STLP_ATOMIC_INCREMENT)
    res = __STATIC_CAST(int, _STLP_ATOMIC_INCREMENT(&_count));
#else
    static _STLP_STATIC_MUTEX _Count_lock _STLP_MUTEX_INITIALIZER;
    {
      _STLP_auto_lock sentry(_Count_lock);
      res = __STATIC_CAST(int, ++_count);
    }
#endif
    M[res] = cat;
  }
  return res;
}

void _Catalog_nl_catd_map::erase(messages_base::catalog cat) {
  map_type::iterator mit(M.find(cat));
  if (mit != M.end()) {
    Mr.erase((*mit).second);
    M.erase(mit);
  }
}
#endif

//----------------------------------------------------------------------
//
_Messages::_Messages(bool is_wide, const char *name) :
  _M_message_obj(0), _M_map(0) {
  if (!name)
    locale::_M_throw_on_null_name();

  int __err_code;
  char buf[_Locale_MAX_SIMPLE_NAME];
  _M_message_obj = _STLP_PRIV __acquire_messages(name, buf, 0, &__err_code);
  if (!_M_message_obj)
    locale::_M_throw_on_creation_failure(__err_code, name, "messages");

  if (is_wide)
    _M_map = new _Catalog_locale_map;
}

_Messages::_Messages(bool is_wide, _Locale_messages* msg) :
  _M_message_obj(msg), _M_map(is_wide ? new _Catalog_locale_map() : 0)
{}

_Messages::~_Messages() {
  __release_messages(_M_message_obj);
  delete _M_map;
}

_Messages::catalog _Messages::do_open(const string& filename, const locale& L) const {
  nl_catd_type result = _M_message_obj ? _Locale_catopen(_M_message_obj, filename.c_str())
    : (nl_catd_type)(-1);

  if ( result != (nl_catd_type)(-1) ) {
    if ( _M_map != 0 ) {
      _M_map->insert(result, L);
    }
    return _STLP_MUTABLE(_Messages_impl, _M_cat).insert( result );
  }

  return -1;
}

string _Messages::do_get(catalog cat,
                         int set, int p_id, const string& dfault) const {
  return _M_message_obj != 0 && cat >= 0
    ? string(_Locale_catgets(_M_message_obj, _STLP_MUTABLE(_Messages_impl, _M_cat)[cat],
                             set, p_id, dfault.c_str()))
    : dfault;
}

#if !defined (_STLP_NO_WCHAR_T)

wstring
_Messages::do_get(catalog thecat,
                  int set, int p_id, const wstring& dfault) const {
  typedef ctype<wchar_t> wctype;
  const wctype& ct = use_facet<wctype>(_M_map->lookup(_STLP_MUTABLE(_Messages_impl, _M_cat)[thecat]));

  const char* str = _Locale_catgets(_M_message_obj, _STLP_MUTABLE(_Messages_impl, _M_cat)[thecat], set, p_id, "");

  // Verify that the lookup failed; an empty string might represent success.
  if (!str)
    return dfault;
  else if (str[0] == '\0') {
    const char* str2 = _Locale_catgets(_M_message_obj, _STLP_MUTABLE(_Messages_impl, _M_cat)[thecat], set, p_id, "*");
    if (!str2 || ((str2[0] == '*') && (str2[1] == '\0')))
      return dfault;
  }

  // str is correct.  Now we must widen it to get a wstring.
  size_t n = strlen(str);

  // NOT PORTABLE.  What we're doing relies on internal details of the
  // string implementation.  (Contiguity of string elements.)
  wstring result(n, wchar_t(0));
  ct.widen(str, str + n, &*result.begin());
  return result;
}

#endif

void _Messages::do_close(catalog thecat) const {
  if (_M_message_obj)
    _Locale_catclose(_M_message_obj, _STLP_MUTABLE(_Messages_impl, _M_cat)[thecat]);
  if (_M_map) _M_map->erase(_STLP_MUTABLE(_Messages_impl, _M_cat)[thecat]);
  _STLP_MUTABLE(_Messages_impl, _M_cat).erase( thecat );
}

_STLP_MOVE_TO_STD_NAMESPACE

//----------------------------------------------------------------------
// messages<char>
messages<char>::messages(size_t refs)
  : locale::facet(refs) {}

messages_byname<char>::messages_byname(const char *name, size_t refs)
  : messages<char>(refs), _M_impl(new _STLP_PRIV _Messages(false, name)) {}

messages_byname<char>::messages_byname(_Locale_messages* msg)
  : messages<char>(0), _M_impl(new _STLP_PRIV _Messages(false, msg)) {}

messages_byname<char>::~messages_byname()
{ delete _M_impl; }

messages_byname<char>::catalog
messages_byname<char>::do_open(const string& filename, const locale& l) const
{ return _M_impl->do_open(filename, l); }

string
messages_byname<char>::do_get(catalog cat, int set, int p_id,
                              const string& dfault) const
{ return _M_impl->do_get(cat, set, p_id, dfault); }

void messages_byname<char>::do_close(catalog cat) const
{ _M_impl->do_close(cat); }

#if !defined (_STLP_NO_WCHAR_T)

//----------------------------------------------------------------------
// messages<wchar_t>

messages<wchar_t>::messages(size_t refs)
  : locale::facet(refs) {}

messages_byname<wchar_t>::messages_byname(const char *name, size_t refs)
  : messages<wchar_t>(refs), _M_impl(new _STLP_PRIV _Messages(true, name)) {}

messages_byname<wchar_t>::messages_byname(_Locale_messages* msg)
  : messages<wchar_t>(0), _M_impl(new _STLP_PRIV _Messages(true, msg)) {}

messages_byname<wchar_t>::~messages_byname()
{ delete _M_impl; }

messages_byname<wchar_t>::catalog
messages_byname<wchar_t>::do_open(const string& filename, const locale& L) const
{ return _M_impl->do_open(filename, L); }

wstring
messages_byname<wchar_t>::do_get(catalog thecat,
                                 int set, int p_id, const wstring& dfault) const
{ return _M_impl->do_get(thecat, set, p_id, dfault); }

void messages_byname<wchar_t>::do_close(catalog cat) const
{ _M_impl->do_close(cat); }

#endif

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
