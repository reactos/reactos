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


#ifndef _STLP_INTERNAL_CODECVT_H
#define _STLP_INTERNAL_CODECVT_H

#ifndef _STLP_C_LOCALE_H
#  include <stl/c_locale.h>
#endif

#ifndef _STLP_INTERNAL_LOCALE_H
#  include <stl/_locale.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

_STLP_BEGIN_NAMESPACE

class _STLP_CLASS_DECLSPEC codecvt_base {
public:
  enum result {ok, partial, error, noconv};
};

template <class _InternT, class _ExternT, class _StateT>
class codecvt : public locale::facet, public codecvt_base {
public:
  typedef _InternT intern_type;
  typedef _ExternT extern_type;
  typedef _StateT state_type;

#if defined (_STLP_MSVC) && (_STLP_MSVC < 1300)
  /* For the moment VC6 do not support this facet default implementation
   * because of the static locale::id instance. When VC6 see this definition
   * it goes crasy with locale::id static instances and all the has_facet tests
   * unit tests are failing.
   */
};
#else
  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}

  result out(state_type&          __state,
             const intern_type*   __from,
             const intern_type*   __from_end,
             const intern_type*&  __from_next,
             extern_type*         __to,
             extern_type*         __to_limit,
             extern_type*&        __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_out(__state,
                  __from, __from_end, __from_next,
                  __to,   __to_limit, __to_next);
  }

  result unshift(state_type&    __state,
                 extern_type*   __to,
                 extern_type*   __to_limit,
                 extern_type*&  __to_next) const {
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_unshift(__state, __to, __to_limit, __to_next);
  }

  result in(state_type&         __state,
            const extern_type*  __from,
            const extern_type*  __from_end,
            const extern_type*& __from_next,
            intern_type*        __to,
            intern_type*        __to_limit,
            intern_type*&       __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_in(__state,
                 __from, __from_end, __from_next,
                 __to,  __to_limit, __to_next);
  }

  int encoding() const _STLP_NOTHROW { return do_encoding(); }

  bool always_noconv() const _STLP_NOTHROW { return do_always_noconv(); }

  int length(state_type&  __state,
             const extern_type* __from,
             const extern_type* __from_end,
             size_t             __max) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    return do_length(__state, __from, __from_end, __max);
  }

  int max_length() const _STLP_NOTHROW { return do_max_length(); }

  static locale::id id;

protected:
  ~codecvt() {}

  virtual result do_out(state_type&,
                        const intern_type*  __from,
                        const intern_type*,
                        const intern_type*& __from_next,
                        extern_type*        __to,
                        extern_type*,
                        extern_type*&       __to_next) const
  { __from_next = __from; __to_next   = __to; return noconv; }

  virtual result do_in (state_type&,
                        const extern_type*  __from,
                        const extern_type*,
                        const extern_type*& __from_next,
                        intern_type*        __to,
                        intern_type*,
                        intern_type*&       __to_next) const
  { __from_next = __from; __to_next = __to; return noconv; }

  virtual result do_unshift(state_type&,
                            extern_type* __to,
                            extern_type*,
                            extern_type*& __to_next) const
  { __to_next = __to; return noconv; }

  virtual int do_encoding() const _STLP_NOTHROW
  { return 1; }

  virtual bool do_always_noconv() const _STLP_NOTHROW
  { return true; }

  virtual int do_length(state_type&,
                        const extern_type* __from,
                        const extern_type* __end,
                        size_t __max) const
  { return (int)(min) ( __STATIC_CAST(size_t, (__end - __from)), __max); }

  virtual int do_max_length() const _STLP_NOTHROW
  { return 1; }

private:
  codecvt(const codecvt<intern_type, extern_type, state_type>&);
  codecvt<intern_type, extern_type, state_type>& operator = (const codecvt<intern_type, extern_type, state_type>&);
};

#  if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#    if !defined (__BORLANDC__) || (__BORLANDC__ >= 0x590)
template <class _InternT, class _ExternT, class _StateT>
locale::id codecvt<_InternT, _ExternT, _StateT>::id;
#    endif
#  endif
#endif

template <class _InternT, class _ExternT, class _StateT>
class codecvt_byname : public codecvt<_InternT, _ExternT, _StateT> {};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC codecvt<char, char, mbstate_t>
  : public locale::facet, public codecvt_base {
public:
  typedef char       intern_type;
  typedef char       extern_type;
  typedef mbstate_t  state_type;

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}

  result out(state_type&   __state,
             const char*  __from,
             const char*  __from_end,
             const char*& __from_next,
             char*        __to,
             char*        __to_limit,
             char*&       __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_out(__state,
                  __from, __from_end, __from_next,
                  __to,   __to_limit, __to_next);
  }

  result unshift(state_type& __state,
                 char* __to, char* __to_limit, char*& __to_next) const {
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_unshift(__state, __to, __to_limit, __to_next);
  }

  result in(state_type&   __state,
            const char*  __from,
            const char*  __from_end,
            const char*& __from_next,
            char*        __to,
            char*        __to_limit,
            char*&       __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_in(__state,
                 __from, __from_end, __from_next,
                 __to,   __to_limit, __to_next);
  }

  int encoding() const _STLP_NOTHROW { return do_encoding(); }

  bool always_noconv() const _STLP_NOTHROW { return do_always_noconv(); }

  int length(state_type& __state,
             const char* __from, const char* __from_end,
             size_t __max) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    return do_length(__state, __from, __from_end, __max);
  }

  int max_length() const _STLP_NOTHROW { return do_max_length(); }

  static _STLP_STATIC_DECLSPEC locale::id id;

protected:
  ~codecvt();

  virtual result do_out(state_type&   /* __state */,
                        const char*  __from,
                        const char*  /* __from_end */,
                        const char*& __from_next,
                        char*        __to,
                        char*        /* __to_limit */,
                        char*&       __to_next) const;

  virtual result do_in (state_type&   /* __state */ ,
                        const char*  __from,
                        const char*  /* __from_end */,
                        const char*& __from_next,
                        char*        __to,
                        char*        /* __to_end */,
                        char*&       __to_next) const;

  virtual result do_unshift(state_type& /* __state */,
                            char*      __to,
                            char*      /* __to_limit */,
                            char*&     __to_next) const;

  virtual int do_encoding() const _STLP_NOTHROW;
  virtual bool do_always_noconv() const _STLP_NOTHROW;
  virtual int do_length(state_type&  __state,
                        const  char* __from,
                        const  char* __end,
                        size_t __max) const;
  virtual int do_max_length() const _STLP_NOTHROW;
private:
  codecvt(const codecvt<char, char, mbstate_t>&);
  codecvt<char, char, mbstate_t>& operator =(const codecvt<char, char, mbstate_t>&);
};

# ifndef _STLP_NO_WCHAR_T

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC codecvt<wchar_t, char, mbstate_t>
  : public locale::facet, public codecvt_base {
public:
  typedef wchar_t    intern_type;
  typedef char       extern_type;
  typedef mbstate_t  state_type;

  explicit codecvt(size_t __refs = 0) : locale::facet(__refs) {}

  result out(state_type&      __state,
             const wchar_t*  __from,
             const wchar_t*  __from_end,
             const wchar_t*& __from_next,
             char*           __to,
             char*           __to_limit,
             char*&          __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_out(__state,
                  __from, __from_end, __from_next,
                  __to,   __to_limit, __to_next);
  }

  result unshift(state_type& __state,
                 char*  __to, char*  __to_limit, char*& __to_next) const {
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_unshift(__state, __to, __to_limit, __to_next);
  }

  result in(state_type&   __state,
            const char*  __from,
            const char*  __from_end,
            const char*& __from_next,
            wchar_t*     __to,
            wchar_t*     __to_limit,
            wchar_t*&    __to_next) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    _STLP_VERBOSE_ASSERT(__to <= __to_limit, _StlMsg_INVALID_ARGUMENT)
    return do_in(__state,
                 __from, __from_end, __from_next,
                 __to,   __to_limit, __to_next);
  }

  int encoding() const _STLP_NOTHROW { return do_encoding(); }

  bool always_noconv() const _STLP_NOTHROW { return do_always_noconv(); }

  int length(state_type& __state,
             const char* __from, const char* __from_end,
             size_t __max) const {
    _STLP_VERBOSE_ASSERT(__from <= __from_end, _StlMsg_INVALID_ARGUMENT)
    return do_length(__state, __from, __from_end, __max);
  }

  int max_length() const _STLP_NOTHROW { return do_max_length(); }

  static _STLP_STATIC_DECLSPEC locale::id id;

protected:
  ~codecvt();

  virtual result do_out(state_type&         __state,
                        const wchar_t*  __from,
                        const wchar_t*  __from_end,
                        const wchar_t*& __from_next,
                        char*        __to,
                        char*        __to_limit,
                        char*&       __to_next) const;

  virtual result do_in (state_type&         __state,
                        const char*  __from,
                        const char*  __from_end,
                        const char*& __from_next,
                        wchar_t*        __to,
                        wchar_t*        __to_limit,
                        wchar_t*&       __to_next) const;

  virtual result do_unshift(state_type&   __state,
                            char*  __to,
                            char*  __to_limit,
                            char*& __to_next) const;

  virtual int do_encoding() const _STLP_NOTHROW;

  virtual bool do_always_noconv() const _STLP_NOTHROW;

  virtual int do_length(state_type&  __state,
                        const  char* __from,
                        const  char* __end,
                        size_t __max) const;

  virtual int do_max_length() const _STLP_NOTHROW;

private:
  codecvt(const codecvt<wchar_t, char, mbstate_t>&);
  codecvt<wchar_t, char, mbstate_t>& operator = (const codecvt<wchar_t, char, mbstate_t>&);
};

# endif

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC codecvt_byname<char, char, mbstate_t>
  : public codecvt<char, char, mbstate_t> {
public:
  explicit codecvt_byname(const char* __name, size_t __refs = 0);
  ~codecvt_byname();
private:
  codecvt_byname(const codecvt_byname<char, char, mbstate_t>&);
  codecvt_byname<char, char, mbstate_t>& operator =(const codecvt_byname<char, char, mbstate_t>&);
};

# ifndef _STLP_NO_WCHAR_T
_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC codecvt_byname<wchar_t, char, mbstate_t>
  : public codecvt<wchar_t, char, mbstate_t> {
  friend class _Locale_impl;
public:
  explicit codecvt_byname(const char * __name, size_t __refs = 0);

protected:
  ~codecvt_byname();

  virtual result do_out(state_type&         __state,
                        const wchar_t*  __from,
                        const wchar_t*  __from_end,
                        const wchar_t*& __from_next,
                        char*        __to,
                        char*        __to_limit,
                        char*&       __to_next) const;

  virtual result do_in (state_type&         __state,
                        const char*  __from,
                        const char*  __from_end,
                        const char*& __from_next,
                        wchar_t*        __to,
                        wchar_t*        __to_limit,
                        wchar_t*&       __to_next) const;

  virtual result do_unshift(state_type&   __state,
                            char*  __to,
                            char*  __to_limit,
                            char*& __to_next) const;

  virtual int do_encoding() const _STLP_NOTHROW;

  virtual bool do_always_noconv() const _STLP_NOTHROW;

  virtual int do_length(state_type&  __state,
                        const  char* __from,
                        const  char* __end,
                        size_t __max) const;

  virtual int do_max_length() const _STLP_NOTHROW;

private:
  codecvt_byname(_Locale_codecvt* __cvt)
    : _M_codecvt(__cvt) {}

  codecvt_byname(const codecvt_byname<wchar_t, char, mbstate_t>&);
  codecvt_byname<wchar_t, char, mbstate_t>& operator =(const codecvt_byname<wchar_t, char, mbstate_t>&);
  _Locale_codecvt* _M_codecvt;
};

# endif

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_CODECVT_H */

// Local Variables:
// mode:C++
// End:

