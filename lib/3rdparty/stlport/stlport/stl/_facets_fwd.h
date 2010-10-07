#ifndef _STLP_FACETS_FWD_H
#define _STLP_FACETS_FWD_H


#include <stl/_iosfwd.h>

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _CharT, class _InputIter>
#else
template <class _CharT, class _InputIter = istreambuf_iterator<_CharT, char_traits<_CharT> > >
#endif
class money_get;

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _CharT, class _OutputIter>
#else
template <class _CharT, class _OutputIter = ostreambuf_iterator<_CharT, char_traits<_CharT> > >
#endif
class money_put;

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _CharT, class _InputIter>
#else
template <class _CharT, class _InputIter = istreambuf_iterator<_CharT, char_traits<_CharT> > >
#endif
class num_get;

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _CharT, class _OutputIter>
#else
template <class _CharT, class _OutputIter = ostreambuf_iterator<_CharT, char_traits<_CharT> > >
#endif
class num_put;

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _Ch, class _InIt>
#else
template <class _Ch, class _InIt = istreambuf_iterator<_Ch, char_traits<_Ch> > >
#endif
class time_get;

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _Ch, class _OutIt>
#else
template <class _Ch, class _OutIt = ostreambuf_iterator<_Ch, char_traits<_Ch> > >
#endif
class time_put;

_STLP_END_NAMESPACE

#endif
