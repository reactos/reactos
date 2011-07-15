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
# include "stlport_prefix.h"

#include <istream>

_STLP_BEGIN_NAMESPACE

#if !defined(_STLP_NO_FORCE_INSTANTIATE)

// instantiations
#  if defined (_STLP_USE_TEMPLATE_EXPORT)
template class _STLP_CLASS_DECLSPEC _Isentry<char, char_traits<char> >;
#  endif

template class _STLP_CLASS_DECLSPEC basic_iostream<char, char_traits<char> >;
template class _STLP_CLASS_DECLSPEC basic_istream<char, char_traits<char> >;

#  if !defined (_STLP_NO_WCHAR_T)
#    if defined (_STLP_USE_TEMPLATE_EXPORT)
template class _STLP_CLASS_DECLSPEC _Isentry<wchar_t, char_traits<wchar_t> >;
#    endif
template class _STLP_CLASS_DECLSPEC basic_istream<wchar_t, char_traits<wchar_t> >;
template class _STLP_CLASS_DECLSPEC basic_iostream<wchar_t, char_traits<wchar_t> >;
#  endif /* !_STLP_NO_WCHAR_T */

#endif /* _STLP_NO_FORCE_INSTANTIATE */

_STLP_END_NAMESPACE

// Local Variables:
// mode:C++
// End:
