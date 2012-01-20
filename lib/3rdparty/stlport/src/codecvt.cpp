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

_STLP_BEGIN_NAMESPACE

//----------------------------------------------------------------------
// codecvt<char, char, mbstate_t>

codecvt<char, char, mbstate_t>::~codecvt() {}

int codecvt<char, char, mbstate_t>::do_length(state_type&,
                                              const  char* from,
                                              const  char* end,
                                              size_t mx) const
{ return (int)(min) ( __STATIC_CAST(size_t, (end - from)), mx); }

int codecvt<char, char, mbstate_t>::do_max_length() const _STLP_NOTHROW
{ return 1; }

bool
codecvt<char, char, mbstate_t>::do_always_noconv() const _STLP_NOTHROW
{ return true; }

int
codecvt<char, char, mbstate_t>::do_encoding() const _STLP_NOTHROW
{ return 1; }

codecvt_base::result
codecvt<char, char, mbstate_t>::do_unshift(state_type& /* __state */,
                                           char*       __to,
                                           char*       /* __to_limit */,
                                           char*&      __to_next) const
{ __to_next = __to; return noconv; }

codecvt_base::result
codecvt<char, char, mbstate_t>::do_in (state_type&  /* __state */ ,
                                       const char*  __from,
                                       const char*  /* __from_end */,
                                       const char*& __from_next,
                                       char*        __to,
                                       char*        /* __to_end */,
                                       char*&       __to_next) const
{ __from_next = __from; __to_next   = __to; return noconv; }

codecvt_base::result
codecvt<char, char, mbstate_t>::do_out(state_type&  /* __state */,
                                       const char*  __from,
                                       const char*  /* __from_end */,
                                       const char*& __from_next,
                                       char*        __to,
                                       char*        /* __to_limit */,
                                       char*&       __to_next) const
{ __from_next = __from; __to_next   = __to; return noconv; }


#if !defined (_STLP_NO_WCHAR_T)
//----------------------------------------------------------------------
// codecvt<wchar_t, char, mbstate_t>

codecvt<wchar_t, char, mbstate_t>::~codecvt() {}


codecvt<wchar_t, char, mbstate_t>::result
codecvt<wchar_t, char, mbstate_t>::do_out(state_type&         /* state */,
                                          const intern_type*  from,
                                          const intern_type*  from_end,
                                          const intern_type*& from_next,
                                          extern_type*        to,
                                          extern_type*        to_limit,
                                          extern_type*&       to_next) const {
  ptrdiff_t len = (min) (from_end - from, to_limit - to);
  copy(from, from + len, to);
  from_next = from + len;
  to_next   = to   + len;
  return ok;
}

codecvt<wchar_t, char, mbstate_t>::result
codecvt<wchar_t, char, mbstate_t>::do_in (state_type&       /* state */,
                                          const extern_type*  from,
                                          const extern_type*  from_end,
                                          const extern_type*& from_next,
                                          intern_type*        to,
                                          intern_type*        to_limit,
                                          intern_type*&       to_next) const {
  ptrdiff_t len = (min) (from_end - from, to_limit - to);
  copy(__REINTERPRET_CAST(const unsigned char*, from),
       __REINTERPRET_CAST(const unsigned char*, from) + len, to);
  from_next = from + len;
  to_next   = to   + len;
  return ok;
}

codecvt<wchar_t, char, mbstate_t>::result
codecvt<wchar_t, char, mbstate_t>::do_unshift(state_type&   /* state */,
                                              extern_type*  to,
                                              extern_type*  ,
                                              extern_type*& to_next) const {
  to_next = to;
  return noconv;
}

int codecvt<wchar_t, char, mbstate_t>::do_encoding() const _STLP_NOTHROW
{ return 1; }

bool codecvt<wchar_t, char, mbstate_t>::do_always_noconv() const _STLP_NOTHROW
{ return true; }

int codecvt<wchar_t, char, mbstate_t>::do_length(state_type&,
                                                 const  extern_type* from,
                                                 const  extern_type* end,
                                                 size_t mx) const
{ return (int)(min) ((size_t) (end - from), mx); }

int codecvt<wchar_t, char, mbstate_t>::do_max_length() const _STLP_NOTHROW
{ return 1; }
#endif /* wchar_t */

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:

