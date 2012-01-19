/*
 * Copyright (c) 2003
 * Francois Dumont
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

/*
 * All the necessary methods used for template expressions with basic_string
 * This file do not have to be macro guarded as it is only used in the _string.h
 * file and it is a part of the basic_string definition.
 */

  template <class _Left, class _Right, class _StorageDir>
  basic_string(_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s)
    : _M_non_dbg_impl(_Reserve_t(), __s.size(), __s.get_allocator()),
      _M_iter_list(&_M_non_dbg_impl)
  { _M_append_sum(__s, _M_non_dbg_impl); }

  template <class _Left, class _Right, class _StorageDir>
  basic_string(_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s,
               size_type __pos, size_type __n = npos,
               const allocator_type& __a = allocator_type())
    : _M_non_dbg_impl(_Reserve_t(), (__pos <= __s.size()) ? ((min) (__n, __s.size() - __pos)) : 0, __a),
      _M_iter_list(&_M_non_dbg_impl) {
    size_type __size = __s.size();
    if (__pos > __size)
      //This call will generate the necessary out of range exception:
      _M_non_dbg_impl.at(0);
    else
      _M_append_sum_pos(__s, __pos, (min) (__n, __size - __pos), _M_non_dbg_impl);
  }

private:
  _Base& _M_append_fast(_STLP_PRIV __char_wrapper<_CharT> __c, _Base &__str)
  { return __str += __c.getValue(); }
  _Base& _M_append_fast(_CharT const* __s, size_type __s_size, _Base &__str)
  { return __str.append(__s, __s_size); }
  _Base& _M_append_fast(_STLP_PRIV __cstr_wrapper<_CharT> const& __s, _Base &__str)
  { return _M_append_fast(__s.c_str(), __s.size(), __str); }
  _Base& _M_append_fast(_STLP_PRIV __bstr_wrapper<_CharT, _Traits, _Alloc> __s, _Base &__str)
  { return _M_append_fast(__s.b_str(), __str); }
  _Base& _M_append_fast(_Self const& __s, _Base &__str)
  { return _M_append_fast(__s.data(), __s.size(), __str); }
  _Base& _M_append_fast(_STLP_PRIV __sum_storage_elem<_CharT, _Traits, _Alloc> const&, _Base &__str)
  { return __str; }
  template <class _Left, class _Right, class _StorageDir>
  _Base& _M_append_fast(_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s, _Base &__str)
  { return _M_append_fast(__s.getRhs(), _M_append_fast(__s.getLhs(), __str)); }

  _Base& _M_append_fast_pos(_STLP_PRIV __char_wrapper<_CharT> __c, _Base &__str, size_type /*__pos*/, size_type __n) {
    if (__n == 0)
      return __str;
    return __str += __c.getValue();
  }
  _Base& _M_append_fast_pos(_CharT const* __s, size_type __s_size, _Base &__str,
                            size_type __pos, size_type __n)
  { return __str.append(__s + __pos, __s + __pos + (min)(__n, __s_size - __pos)); }
  _Base& _M_append_fast_pos(_STLP_PRIV __cstr_wrapper<_CharT> const& __s, _Base &__str,
                            size_type __pos, size_type __n)
  { return _M_append_fast_pos(__s.c_str(), __s.size(), __str, __pos, __n); }
  _Base& _M_append_fast_pos(_STLP_PRIV __bstr_wrapper<_CharT, _Traits, _Alloc> __s, _Base &__str,
                            size_type __pos, size_type __n)
  { return _M_append_fast_pos(__s.b_str(), __str, __pos, __n); }
  _Base& _M_append_fast_pos(_Self const& __s, _Base &__str, size_type __pos, size_type __n)
  { return _M_append_fast_pos(__s.data(), __s.size(), __str, __pos, __n); }
  _Base& _M_append_fast_pos(_STLP_PRIV __sum_storage_elem<_CharT, _Traits, _Alloc> const&, _Base &__str,
                            size_type /*__pos*/, size_type /*__n*/)
  { return __str; }

  template <class _Left, class _Right, class _StorageDir>
  _Base& _M_append_fast_pos(_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s,
                            _Base &__str, size_type __pos, size_type __n) {
    if (__n == 0) {
      return __str;
    }
    size_type __lhs_size = __s.getLhs().size();
    if (__pos < __lhs_size) {
      if (__n < (__lhs_size - __pos)) {
        return _M_append_fast_pos(__s.getLhs(), __str, __pos, __n);
      } else {
        return _M_append_fast_pos(__s.getRhs(), _M_append_fast_pos(__s.getLhs(), __str, __pos, __n),
                                  0, __n - (__lhs_size - __pos));
      }
    } else {
      return _M_append_fast_pos(__s.getRhs(), __str, __pos - __lhs_size, __n);
    }
  }

  template <class _Left, class _Right, class _StorageDir>
  _Self& _M_append_sum (_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s,
                        _Base &__impl) {
    _M_append_fast(__s, __impl);
    return *this;
  }

  template <class _Left, class _Right, class _StorageDir>
  _Self& _M_append_sum_pos (_STLP_PRIV __bstr_sum<_CharT, _Traits, _Alloc, _Left, _Right, _StorageDir> const& __s,
                            size_type __pos, size_type __n, _Base &__impl) {
    _M_non_dbg_impl.reserve(_M_non_dbg_impl.size() + (min) (__s.size() - __pos, __n));
    _M_append_fast_pos(__s, __impl, __pos, __n);
    return *this;
  }
