/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996-1998
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#if !defined (_STLP_INTERNAL_STREAM_ITERATOR_H) && !defined (_STLP_USE_NO_IOSTREAMS)
#define _STLP_INTERNAL_STREAM_ITERATOR_H

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#  include <stl/_iterator_base.h>
#endif

// streambuf_iterators predeclarations must appear first
#ifndef _STLP_INTERNAL_IOSFWD
#  include <stl/_iosfwd.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_INTERNAL_OSTREAMBUF_ITERATOR_H
#  include <stl/_ostreambuf_iterator.h>
#endif

#ifndef _STLP_INTERNAL_ISTREAMBUF_ITERATOR_H
#  include <stl/_istreambuf_iterator.h>
#endif

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h>
#endif

// istream_iterator and ostream_iterator look very different if we're
// using new, templatized iostreams than if we're using the old cfront
// version.

_STLP_BEGIN_NAMESPACE

#if !defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
#  define __ISI_TMPL_HEADER_ARGUMENTS class _Tp, class _CharT, class _Traits, class _Dist
#  define __ISI_TMPL_ARGUMENTS _Tp, _CharT, _Traits, _Dist
template <class _Tp,
          class _CharT = char, class _Traits = char_traits<_CharT>,
          class _Dist = ptrdiff_t>
class istream_iterator : public iterator<input_iterator_tag, _Tp , _Dist,
                                         const _Tp*, const _Tp& > {
#else
#  if defined (_STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS) && !defined (_STLP_DEFAULT_TYPE_PARAM)
#    define __ISI_TMPL_HEADER_ARGUMENTS class _Tp
#    define __ISI_TMPL_ARGUMENTS        _Tp
template <class _Tp>
class istream_iterator : public iterator<input_iterator_tag, _Tp , ptrdiff_t,
                                         const _Tp*, const _Tp& > {
#  else
#    define __ISI_TMPL_HEADER_ARGUMENTS class _Tp, class _Dist
#    define __ISI_TMPL_ARGUMENTS        _Tp, _Dist
template <class _Tp, _STLP_DFL_TYPE_PARAM(_Dist, ptrdiff_t)>
class istream_iterator : public iterator<input_iterator_tag, _Tp, _Dist ,
                                         const _Tp*, const _Tp& > {
#  endif /* _STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS */
#endif /* _STLP_LIMITED_DEFAULT_TEMPLATES */

#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
  typedef char _CharT;
  typedef char_traits<char> _Traits;
#  if defined (_STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS) && !defined (_STLP_DEFAULT_TYPE_PARAM)
  typedef ptrdiff_t _Dist;
#  endif
#endif

  typedef istream_iterator< __ISI_TMPL_ARGUMENTS > _Self;
public:
  typedef _CharT                         char_type;
  typedef _Traits                        traits_type;
  typedef basic_istream<_CharT, _Traits> istream_type;

  typedef input_iterator_tag             iterator_category;
  typedef _Tp                            value_type;
  typedef _Dist                          difference_type;
  typedef const _Tp*                     pointer;
  typedef const _Tp&                     reference;

  istream_iterator() : _M_stream(0), _M_ok(false), _M_read_done(true) {}
  istream_iterator(istream_type& __s) : _M_stream(&__s), _M_ok(false), _M_read_done(false) {}

  reference operator*() const {
    if (!_M_read_done) {
      _M_read();
    }
    return _M_value;
  }

  _STLP_DEFINE_ARROW_OPERATOR

  _Self& operator++() {
    _M_read();
    return *this;
  }
  _Self operator++(int)  {
    _Self __tmp = *this;
    _M_read();
    return __tmp;
  }

  bool _M_equal(const _Self& __x) const {
    if (!_M_read_done) {
      _M_read();
    }
    if (!__x._M_read_done) {
      __x._M_read();
    }
    return (_M_ok == __x._M_ok) && (!_M_ok || _M_stream == __x._M_stream);
  }

private:
  istream_type* _M_stream;
  mutable _Tp _M_value;
  mutable bool _M_ok;
  mutable bool _M_read_done;

  void _M_read() const {
    _STLP_MUTABLE(_Self, _M_ok) = ((_M_stream != 0) && !_M_stream->fail());
    if (_M_ok) {
      *_M_stream >> _STLP_MUTABLE(_Self, _M_value);
      _STLP_MUTABLE(_Self, _M_ok) = !_M_stream->fail();
    }
    _STLP_MUTABLE(_Self, _M_read_done) = true;
  }
};

#if !defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _TpP,
          class _CharT = char, class _Traits = char_traits<_CharT> >
#else
template <class _TpP>
#endif
class ostream_iterator: public iterator<output_iterator_tag, void, void, void, void> {
#if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
  typedef char _CharT;
  typedef char_traits<char> _Traits;
  typedef ostream_iterator<_TpP> _Self;
#else
  typedef ostream_iterator<_TpP, _CharT, _Traits> _Self;
#endif
public:
  typedef _CharT                         char_type;
  typedef _Traits                        traits_type;
  typedef basic_ostream<_CharT, _Traits> ostream_type;

  typedef output_iterator_tag            iterator_category;

  ostream_iterator(ostream_type& __s) : _M_stream(&__s), _M_string(0) {}
  ostream_iterator(ostream_type& __s, const _CharT* __c)
    : _M_stream(&__s), _M_string(__c)  {}
  _Self& operator=(const _TpP& __val) {
    *_M_stream << __val;
    if (_M_string) *_M_stream << _M_string;
    return *this;
  }
  _Self& operator*() { return *this; }
  _Self& operator++() { return *this; }
  _Self& operator++(int) { return *this; }
private:
  ostream_type* _M_stream;
  const _CharT* _M_string;
};

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
#  if defined (_STLP_LIMITED_DEFAULT_TEMPLATES)
template <class _TpP>
inline output_iterator_tag _STLP_CALL
iterator_category(const ostream_iterator<_TpP>&) { return output_iterator_tag(); }
#  else
template <class _TpP, class _CharT, class _Traits>
inline output_iterator_tag _STLP_CALL
iterator_category(const ostream_iterator<_TpP, _CharT, _Traits>&) { return output_iterator_tag(); }
#  endif
#endif

_STLP_END_NAMESPACE

// form-independent definiotion of stream iterators
_STLP_BEGIN_NAMESPACE

template < __ISI_TMPL_HEADER_ARGUMENTS >
inline bool _STLP_CALL
operator==(const istream_iterator< __ISI_TMPL_ARGUMENTS >& __x,
           const istream_iterator< __ISI_TMPL_ARGUMENTS >& __y)
{ return __x._M_equal(__y); }

#if defined (_STLP_USE_SEPARATE_RELOPS_NAMESPACE)
template < __ISI_TMPL_HEADER_ARGUMENTS >
inline bool _STLP_CALL
operator!=(const istream_iterator< __ISI_TMPL_ARGUMENTS >& __x,
           const istream_iterator< __ISI_TMPL_ARGUMENTS >& __y)
{ return !__x._M_equal(__y); }
#endif

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
template < __ISI_TMPL_HEADER_ARGUMENTS >
inline input_iterator_tag _STLP_CALL
iterator_category(const istream_iterator< __ISI_TMPL_ARGUMENTS >&)
{ return input_iterator_tag(); }
template < __ISI_TMPL_HEADER_ARGUMENTS >
inline _Tp* _STLP_CALL
value_type(const istream_iterator< __ISI_TMPL_ARGUMENTS >&) { return (_Tp*) 0; }

#  if defined (_STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS) && !defined (_STLP_DEFAULT_TYPE_PARAM)
template < __ISI_TMPL_HEADER_ARGUMENTS >
inline ptrdiff_t* _STLP_CALL
distance_type(const istream_iterator< __ISI_TMPL_ARGUMENTS >&) { return (ptrdiff_t*)0; }
#  else
template < __ISI_TMPL_HEADER_ARGUMENTS >
inline _Dist* _STLP_CALL
distance_type(const istream_iterator< __ISI_TMPL_ARGUMENTS >&) { return (_Dist*)0; }
#  endif /* _STLP_MINIMUM_DEFAULT_TEMPLATE_PARAMS */
#endif

_STLP_END_NAMESPACE

#undef __ISI_TMPL_HEADER_ARGUMENTS
#undef __ISI_TMPL_ARGUMENTS

#endif /* _STLP_INTERNAL_STREAM_ITERATOR_H */

// Local Variables:
// mode:C++
// End:
