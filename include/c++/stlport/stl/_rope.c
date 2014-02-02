/*
 * Copyright (c) 1996,1997
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

// Set buf_start, buf_end, and buf_ptr appropriately, filling tmp_buf
// if necessary.  Assumes path_end[leaf_index] and leaf_pos are correct.
// Results in a valid buf_ptr if the iterator can be legitimately
// dereferenced.
#ifndef _STLP_ROPEIMPL_H
#define _STLP_ROPEIMPL_H

#ifndef _STLP_INTERNAL_ROPE_H
#  include <stl/_rope.h>
#endif

#ifndef _STLP_INTERNAL_CSTDIO
#  include <stl/_cstdio.h>
#endif

#if !defined (_STLP_USE_NO_IOSTREAMS)
#  ifndef _STLP_INTERNAL_OSTREAM_H
#    include <stl/_ostream.h>
#  endif

#  ifndef _STLP_INTERNAL_ISTREAM
#    include <stl/_istream.h>
#  endif
#endif

#include <stl/_range_errors.h>

_STLP_BEGIN_NAMESPACE

#if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
#  define __allocator__ _Alloc
#else
#  define __allocator__ allocator_type
#endif

template<class _CharT, class _Alloc>
_Rope_iterator<_CharT, _Alloc>::_Rope_iterator(rope<_CharT,_Alloc>* __r, size_t __pos)
  : _Rope_iterator_base<_CharT,_Alloc>(__r->_M_tree_ptr._M_data, __pos),
  _M_root_rope(__r) { _RopeRep::_S_ref(this->_M_root); }

template<class _CharT, class _Alloc>
_Rope_iterator<_CharT, _Alloc>::_Rope_iterator(rope<_CharT,_Alloc>& __r, size_t __pos):
  _Rope_iterator_base<_CharT,_Alloc>(__r._M_tree_ptr._M_data, __pos),
  _M_root_rope(&__r) {
#if !defined (__DMC__)
  _RopeRep::_S_ref(this->_M_root); if (!(__r.empty()))_S_setcache(*this);
#else
  _Rope_iterator_base<_CharT, _Alloc>* __x = this;
  _RopeRep::_S_ref(this->_M_root); if (!(__r.empty()))_S_setcache(*__x);
#endif
}

template<class _CharT, class _Alloc>
void _Rope_RopeRep<_CharT, _Alloc>::_M_free_c_string() {
  _CharT* __cstr = _M_c_string;
  if (0 != __cstr) {
    size_t _p_size = _M_size._M_data + 1;
    _STLP_STD::_Destroy_Range(__cstr, __cstr + _p_size);
    _M_size.deallocate(__cstr, _p_size);
  }
}

// Set buf_start, buf_end, and buf_ptr appropriately, filling tmp_buf
// if necessary.  Assumes _M_path_end[leaf_index] and leaf_pos are correct.
// Results in a valid buf_ptr if the iterator can be legitimately
// dereferenced.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setbuf(
  _Rope_iterator_base<_CharT,_Alloc>& __x) {
  const _RopeRep* __leaf = __x._M_path_end._M_data[__x._M_leaf_index];
  size_t __leaf_pos = __x._M_leaf_pos;
  size_t __pos = __x._M_current_pos;

  switch(__leaf->_M_tag) {
  case _RopeRep::_S_leaf:
    typedef _Rope_RopeLeaf<_CharT, _Alloc> _RopeLeaf;
    __x._M_buf_start = __STATIC_CAST(const _RopeLeaf*, __leaf)->_M_data;
    __x._M_buf_ptr = __x._M_buf_start + (__pos - __leaf_pos);
    __x._M_buf_end = __x._M_buf_start + __leaf->_M_size._M_data;
    break;
  case _RopeRep::_S_function:
  case _RopeRep::_S_substringfn:
    {
      size_t __len = _S_iterator_buf_len;
      size_t __buf_start_pos = __leaf_pos;
      size_t __leaf_end = __leaf_pos + __leaf->_M_size._M_data;
      typedef _Rope_RopeFunction<_CharT, _Alloc> _RopeFunction;
      char_producer<_CharT>* __fn = __STATIC_CAST(const _RopeFunction*, __leaf)->_M_fn;

      if (__buf_start_pos + __len <= __pos) {
        __buf_start_pos = __pos - __len/4;
        if (__buf_start_pos + __len > __leaf_end) {
          __buf_start_pos = __leaf_end - __len;
        }
      }
      if (__buf_start_pos + __len > __leaf_end) {
        __len = __leaf_end - __buf_start_pos;
      }
      (*__fn)(__buf_start_pos - __leaf_pos, __len, __x._M_tmp_buf._M_data);
      __x._M_buf_ptr = __x._M_tmp_buf._M_data + (__pos - __buf_start_pos);
      __x._M_buf_start = __x._M_tmp_buf._M_data;
      __x._M_buf_end = __x._M_tmp_buf._M_data + __len;
    }
    break;
  default:
      _STLP_ASSERT(0)
      ;
  }
}

// Set path and buffer inside a rope iterator.  We assume that
// pos and root are already set.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setcache(
  _Rope_iterator_base<_CharT,_Alloc>& __x) {
  const _RopeRep* __path[_RopeRep::_S_max_rope_depth+1];
  const _RopeRep* __curr_rope;
  int __curr_depth = -1;  /* index into path    */
  size_t __curr_start_pos = 0;
  size_t __pos = __x._M_current_pos;
  unsigned char __dirns = 0; // Bit vector marking right turns in the path

  _STLP_ASSERT(__pos <= __x._M_root->_M_size._M_data)
  if (__pos >= __x._M_root->_M_size._M_data) {
    __x._M_buf_ptr = 0;
    return;
  }
  __curr_rope = __x._M_root;
  if (0 != __curr_rope->_M_c_string) {
    /* Treat the root as a leaf. */
    __x._M_buf_start = __curr_rope->_M_c_string;
    __x._M_buf_end = __curr_rope->_M_c_string + __curr_rope->_M_size._M_data;
    __x._M_buf_ptr = __curr_rope->_M_c_string + __pos;
    __x._M_path_end._M_data[0] = __curr_rope;
    __x._M_leaf_index = 0;
    __x._M_leaf_pos = 0;
    return;
  }
  for(;;) {
    ++__curr_depth;
    _STLP_ASSERT(__curr_depth <= _RopeRep::_S_max_rope_depth)
    __path[__curr_depth] = __curr_rope;
    switch(__curr_rope->_M_tag) {
    case _RopeRep::_S_leaf:
    case _RopeRep::_S_function:
    case _RopeRep::_S_substringfn:
      __x._M_leaf_pos = __curr_start_pos;
      goto done;
    case _RopeRep::_S_concat:
      {
        const _RopeConcat* __c = __STATIC_CAST(const _RopeConcat*, __curr_rope);
        _RopeRep* __left = __c->_M_left;
        size_t __left_len = __left->_M_size._M_data;

        __dirns <<= 1;
        if (__pos >= __curr_start_pos + __left_len) {
          __dirns |= 1;
          __curr_rope = __c->_M_right;
          __curr_start_pos += __left_len;
        } else {
          __curr_rope = __left;
        }
      }
      break;
    }
  }
done:
    // Copy last section of path into _M_path_end.
  {
    int __i = -1;
    int __j = __curr_depth + 1 - _S_path_cache_len;

    if (__j < 0) __j = 0;
    while (__j <= __curr_depth) {
      __x._M_path_end._M_data[++__i] = __path[__j++];
    }
    __x._M_leaf_index = __i;
  }
  __x._M_path_directions = __dirns;
  _S_setbuf(__x);
}

// Specialized version of the above.  Assumes that
// the path cache is valid for the previous position.
template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_S_setcache_for_incr(
_Rope_iterator_base<_CharT,_Alloc>& __x) {
  int __current_index = __x._M_leaf_index;
  const _RopeRep* __current_node = __x._M_path_end._M_data[__current_index];
  size_t __len = __current_node->_M_size._M_data;
  size_t __node_start_pos = __x._M_leaf_pos;
  unsigned char __dirns = __x._M_path_directions;
  const _RopeConcat* __c;

  _STLP_ASSERT(__x._M_current_pos <= __x._M_root->_M_size._M_data)
  if (__x._M_current_pos - __node_start_pos < __len) {
    /* More stuff in this leaf, we just didn't cache it. */
    _S_setbuf(__x);
    return;
  }
  _STLP_ASSERT(__node_start_pos + __len == __x._M_current_pos)
    //  node_start_pos is starting position of last_node.
  while (--__current_index >= 0) {
    if (!(__dirns & 1) /* Path turned left */)
      break;
    __current_node = __x._M_path_end._M_data[__current_index];
    __c = __STATIC_CAST(const _RopeConcat*, __current_node);
    // Otherwise we were in the right child.  Thus we should pop
    // the concatenation node.
    __node_start_pos -= __c->_M_left->_M_size._M_data;
    __dirns >>= 1;
  }
  if (__current_index < 0) {
    // We underflowed the cache. Punt.
    _S_setcache(__x);
    return;
  }
  __current_node = __x._M_path_end._M_data[__current_index];
  __c = __STATIC_CAST(const _RopeConcat*, __current_node);
  // current_node is a concatenation node.  We are positioned on the first
  // character in its right child.
  // node_start_pos is starting position of current_node.
  __node_start_pos += __c->_M_left->_M_size._M_data;
  __current_node = __c->_M_right;
  __x._M_path_end._M_data[++__current_index] = __current_node;
  __dirns |= 1;
  while (_RopeRep::_S_concat == __current_node->_M_tag) {
    ++__current_index;
    if (_S_path_cache_len == __current_index) {
      int __i;
      for (__i = 0; __i < _S_path_cache_len-1; ++__i) {
        __x._M_path_end._M_data[__i] = __x._M_path_end._M_data[__i+1];
      }
      --__current_index;
    }
    __current_node = __STATIC_CAST(const _RopeConcat*, __current_node)->_M_left;
    __x._M_path_end._M_data[__current_index] = __current_node;
    __dirns <<= 1;
    // node_start_pos is unchanged.
  }
  __x._M_leaf_index = __current_index;
  __x._M_leaf_pos = __node_start_pos;
  __x._M_path_directions = __dirns;
  _S_setbuf(__x);
}

template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_M_incr(size_t __n) {
  _M_current_pos += __n;
  if (0 != _M_buf_ptr) {
    size_t __chars_left = _M_buf_end - _M_buf_ptr;
    if (__chars_left > __n) {
      _M_buf_ptr += __n;
    } else if (__chars_left == __n) {
      _M_buf_ptr += __n;
      _S_setcache_for_incr(*this);
    } else {
      _M_buf_ptr = 0;
    }
  }
}

template <class _CharT, class _Alloc>
void _Rope_iterator_base<_CharT,_Alloc>::_M_decr(size_t __n) {
  if (0 != _M_buf_ptr) {
    size_t __chars_left = _M_buf_ptr - _M_buf_start;
    if (__chars_left >= __n) {
      _M_buf_ptr -= __n;
    } else {
      _M_buf_ptr = 0;
    }
  }
  _M_current_pos -= __n;
}

template <class _CharT, class _Alloc>
void _Rope_iterator<_CharT,_Alloc>::_M_check() {
  if (_M_root_rope->_M_tree_ptr._M_data != this->_M_root) {
    // _Rope was modified.  Get things fixed up.
    _RopeRep::_S_unref(this->_M_root);
    this->_M_root = _M_root_rope->_M_tree_ptr._M_data;
    _RopeRep::_S_ref(this->_M_root);
    this->_M_buf_ptr = 0;
  }
}

//  There are several reasons for not doing this with virtual destructors
//  and a class specific delete operator:
//  - A class specific delete operator can't easily get access to
//    allocator instances if we need them.
//  - Any virtual function would need a 4 or byte vtable pointer;
//    this only requires a one byte tag per object.
template <class _CharT, class _Alloc>
void _Rope_RopeRep<_CharT,_Alloc>::_M_free_tree() {
  switch (_M_tag) {
  case _S_leaf:
    {
      typedef _Rope_RopeLeaf<_CharT, _Alloc> _RopeLeaf;
      _RopeLeaf* __l = __STATIC_CAST(_RopeLeaf*, this);
      _STLP_STD::_Destroy(__l); // ->_Rope_RopeLeaf<_CharT,_Alloc>::~_Rope_RopeLeaf();
      _STLP_CREATE_ALLOCATOR(allocator_type,(const allocator_type&)_M_size,
                             _RopeLeaf).deallocate(__l, 1);
      break;
    }
  case _S_concat:
    {
      typedef _Rope_RopeConcatenation<_CharT, _Alloc> _RopeConcatenation;
      _RopeConcatenation* __c  = __STATIC_CAST(_RopeConcatenation*, this);
      _STLP_STD::_Destroy(__c);
      _STLP_CREATE_ALLOCATOR(allocator_type,(const allocator_type&)_M_size,
                             _RopeConcatenation).deallocate(__c, 1);
      break;
    }
  case _S_function:
    {
      typedef _Rope_RopeFunction<_CharT, _Alloc> _RopeFunction;
      _RopeFunction* __f = __STATIC_CAST(_RopeFunction*, this);
      _STLP_STD::_Destroy(__f);
      _STLP_CREATE_ALLOCATOR(allocator_type, (const allocator_type&)_M_size,
                             _RopeFunction).deallocate(__f, 1);
      break;
    }
  case _S_substringfn:
    {
      typedef _Rope_RopeSubstring<_CharT, _Alloc> _RopeSubstring;
      _RopeSubstring* __rss = __STATIC_CAST(_RopeSubstring*, this);
      _STLP_STD::_Destroy(__rss);
      _STLP_CREATE_ALLOCATOR(allocator_type, (const allocator_type&)_M_size,
                             _RopeSubstring).deallocate(__rss, 1);
      break;
    }
  }
}

# if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
#   define __RopeLeaf__ _Rope_RopeLeaf<_CharT,_Alloc>
#   define __RopeRep__ _Rope_RopeRep<_CharT,_Alloc>
#   define _RopeLeaf _Rope_RopeLeaf<_CharT,_Alloc>
#   define _RopeRep _Rope_RopeRep<_CharT,_Alloc>
#   define size_type size_t
# else
#   define __RopeLeaf__ _STLP_TYPENAME_ON_RETURN_TYPE rope<_CharT,_Alloc>::_RopeLeaf
#   define __RopeRep__ _STLP_TYPENAME_ON_RETURN_TYPE rope<_CharT,_Alloc>::_RopeRep
# endif

template <class _CharT, class _Alloc>
void rope<_CharT, _Alloc>::_M_throw_out_of_range() const {
  __stl_throw_out_of_range("rope");
}

// Concatenate a C string onto a leaf rope by copying the rope data.
// Used for short ropes.
template <class _CharT, class _Alloc>
__RopeLeaf__*
rope<_CharT,_Alloc>::_S_leaf_concat_char_iter (
  _RopeLeaf* __r, const _CharT* __iter, size_t __len) {
  size_t __old_len = __r->_M_size._M_data;
  _CharT* __new_data = __r->_M_size.allocate(_S_rounded_up_size(__old_len + __len));
  _RopeLeaf* __result;

  _STLP_PRIV __ucopy_n(__r->_M_data, __old_len, __new_data);
  _STLP_PRIV __ucopy_n(__iter, __len, __new_data + __old_len);
  _S_construct_null(__new_data + __old_len + __len);
  _STLP_TRY {
    __result = _S_new_RopeLeaf(__new_data, __old_len + __len, __r->get_allocator());
  }
  _STLP_UNWIND(_RopeRep::_S_free_string(__new_data, __old_len + __len,
                                        __r->get_allocator()))
  return __result;
}

template <class _CharT, class _Alloc>
void _Terminate_RopeLeaf(_Rope_RopeLeaf<_CharT,_Alloc> *__r,
                         size_t __size, const __true_type& /*basic char type*/) {
  _S_construct_null(__r->_M_data + __size);
  _STLP_ASSERT(__r->_M_c_string == __r->_M_data)
}

template <class _CharT, class _Alloc>
void _Terminate_RopeLeaf(_Rope_RopeLeaf<_CharT,_Alloc> *__r,
                         size_t, const __false_type& /*basic char type*/) {
  if (__r->_M_c_string != __r->_M_data && 0 != __r->_M_c_string) {
    __r->_M_free_c_string();
    __r->_M_c_string = 0;
  }
}

// As above, but it's OK to clobber original if refcount is 1
template <class _CharT, class _Alloc>
__RopeLeaf__*
rope<_CharT,_Alloc>::_S_destr_leaf_concat_char_iter (_RopeLeaf* __r, const _CharT* __iter, size_t __len) {
  //_STLP_ASSERT(__r->_M_ref_count >= 1)
  if ( /* __r->_M_ref_count > 1 */  __r->_M_incr() > 2 ) { // - ptr
    __r->_M_decr(); // - ptr
    return _S_leaf_concat_char_iter(__r, __iter, __len);
  }
  __r->_M_decr(); // - ptr, __r->_M_ref_count == 1 or 0
  size_t __old_len = __r->_M_size._M_data;
  if (_S_rounded_up_size(__old_len) == _S_rounded_up_size(__old_len + __len)) {
    // The space has been partially initialized for the standard
    // character types.  But that doesn't matter for those types.
    _STLP_PRIV __ucopy_n(__iter, __len, __r->_M_data + __old_len);
    _Terminate_RopeLeaf(__r, __old_len + __len, _IsBasicCharType());
    __r->_M_size._M_data = __old_len + __len;
    // _STLP_ASSERT(__r->_M_ref_count == 1)
    // __r->_M_ref_count = 2;
    __r->_M_incr(); // i.e.  __r->_M_ref_count = 2
    return __r;
  } else {
    _RopeLeaf* __result = _S_leaf_concat_char_iter(__r, __iter, __len);
    //_STLP_ASSERT(__result->_M_ref_count == 1)
    return __result;
  }
}

// Assumes left and right are not 0.
// Does not increment (nor decrement on exception) child reference counts.
// Result has ref count 1.
template <class _CharT, class _Alloc>
__RopeRep__*
rope<_CharT,_Alloc>::_S_tree_concat (_RopeRep* __left, _RopeRep* __right) {
  _RopeConcatenation* __result =
    _S_new_RopeConcatenation(__left, __right, __left->get_allocator());
  size_t __depth = __result->_M_depth;

  _STLP_ASSERT(__left->get_allocator() == __right->get_allocator())
  if (__depth > 20 && (__result->_M_size._M_data < 1000 ||
      __depth > _RopeRep::_S_max_rope_depth)) {
    _RopeRep* __balanced;

    _STLP_TRY {
      __balanced = _S_balance(__result);
     // _STLP_ASSERT(__result == __balanced ||
     //              1 == __result->_M_ref_count &&
     //              1 == __balanced->_M_ref_count)
      __result->_M_unref_nonnil();
    }
    _STLP_UNWIND((_STLP_CREATE_ALLOCATOR(allocator_type,(allocator_type&)__left->_M_size,
                                         _RopeConcatenation).deallocate(__result,1)))
    // In case of exception, we need to deallocate
    // otherwise dangling result node.  But caller
    // still owns its children.  Thus unref is
    // inappropriate.
    return __balanced;
  } else {
    return __result;
  }
}

template <class _CharT, class _Alloc>
__RopeRep__*
rope<_CharT,_Alloc>::_S_concat_char_iter (_RopeRep* __r,
                                          const _CharT*__s, size_t __slen) {
  _RopeRep* __result;
  if (0 == __slen) {
    _S_ref(__r);
    return __r;
  }
  if (0 == __r)
    return _S_RopeLeaf_from_unowned_char_ptr(__s, __slen, __r->get_allocator());
  if (_RopeRep::_S_leaf == __r->_M_tag &&
      __r->_M_size._M_data + __slen <= _S_copy_max) {
    __result = _S_leaf_concat_char_iter((_RopeLeaf*)__r, __s, __slen);
    // _STLP_ASSERT(1 == __result->_M_ref_count)
    return __result;
  }
  if (_RopeRep::_S_concat == __r->_M_tag &&
      _RopeRep::_S_leaf == ((_RopeConcatenation*)__r)->_M_right->_M_tag) {
    _RopeLeaf* __right = (_RopeLeaf* )(((_RopeConcatenation* )__r)->_M_right);
    if (__right->_M_size._M_data + __slen <= _S_copy_max) {
      _RopeRep* __left = ((_RopeConcatenation*)__r)->_M_left;
      _RopeRep* __nright = _S_leaf_concat_char_iter((_RopeLeaf*)__right, __s, __slen);
      __left->_M_ref_nonnil();
      _STLP_TRY {
        __result = _S_tree_concat(__left, __nright);
      }
      _STLP_UNWIND(_S_unref(__left); _S_unref(__nright))
      // _STLP_ASSERT(1 == __result->_M_ref_count)
      return __result;
    }
  }
  _RopeRep* __nright =
    _S_RopeLeaf_from_unowned_char_ptr(__s, __slen, __r->get_allocator());
  _STLP_TRY {
    __r->_M_ref_nonnil();
    __result = _S_tree_concat(__r, __nright);
  }
  _STLP_UNWIND(_S_unref(__r); _S_unref(__nright))
  // _STLP_ASSERT(1 == __result->_M_ref_count)
  return __result;
}

template <class _CharT, class _Alloc>
__RopeRep__*
rope<_CharT,_Alloc>::_S_destr_concat_char_iter(
  _RopeRep* __r, const _CharT* __s, size_t __slen) {
  _RopeRep* __result;
  if (0 == __r)
    return _S_RopeLeaf_from_unowned_char_ptr(__s, __slen,
                                             __r->get_allocator());
  // size_t __count = __r->_M_ref_count;
  size_t __orig_size = __r->_M_size._M_data;
  // _STLP_ASSERT(__count >= 1)
  if ( /* __count > 1 */ __r->_M_incr() > 2 ) {
    __r->_M_decr();
    return _S_concat_char_iter(__r, __s, __slen);
  }
  if (0 == __slen) {
    return __r;
  }
  __r->_M_decr();
  if (__orig_size + __slen <= _S_copy_max && _RopeRep::_S_leaf == __r->_M_tag) {
    return _S_destr_leaf_concat_char_iter((_RopeLeaf*)__r, __s, __slen);
  }
  if (_RopeRep::_S_concat == __r->_M_tag) {
    _RopeLeaf* __right = __STATIC_CAST(_RopeLeaf*, __STATIC_CAST(_RopeConcatenation*, __r)->_M_right);
    if (_RopeRep::_S_leaf == __right->_M_tag &&
        __right->_M_size._M_data + __slen <= _S_copy_max) {
      _RopeRep* __new_right = _S_destr_leaf_concat_char_iter(__right, __s, __slen);
      if (__right == __new_right) {
        // _STLP_ASSERT(__new_right->_M_ref_count == 2)
        // __new_right->_M_ref_count = 1;
        __new_right->_M_decr();
      } else {
        // _STLP_ASSERT(__new_right->_M_ref_count >= 1)
        __right->_M_unref_nonnil();
      }
      // _STLP_ASSERT(__r->_M_ref_count == 1)
      // __r->_M_ref_count = 2;    // One more than before.
      __r->_M_incr();
      __STATIC_CAST(_RopeConcatenation*, __r)->_M_right = __new_right;
      // E.Musser : moved below
      //    __r->_M_size._M_data = __orig_size + __slen;
      if (0 != __r->_M_c_string) {
        __r->_M_free_c_string();
        __r->_M_c_string = 0;
      }
      __r->_M_size._M_data = __orig_size + __slen;
      return __r;
    }
  }
  _RopeRep* __right =
    _S_RopeLeaf_from_unowned_char_ptr(__s, __slen, __r->get_allocator());
  __r->_M_ref_nonnil();
  _STLP_TRY {
    __result = _S_tree_concat(__r, __right);
  }
  _STLP_UNWIND(_S_unref(__r); _S_unref(__right))
    // _STLP_ASSERT(1 == __result->_M_ref_count)
  return __result;
}

template <class _CharT, class _Alloc>
__RopeRep__*
rope<_CharT,_Alloc>::_S_concat_rep(_RopeRep* __left, _RopeRep* __right) {
  if (0 == __left) {
    _S_ref(__right);
    return __right;
  }
  if (0 == __right) {
    __left->_M_ref_nonnil();
    return __left;
  }
  if (_RopeRep::_S_leaf == __right->_M_tag) {
    if (_RopeRep::_S_leaf == __left->_M_tag) {
      if (__right->_M_size._M_data + __left->_M_size._M_data <= _S_copy_max) {
        return _S_leaf_concat_char_iter(__STATIC_CAST(_RopeLeaf*, __left),
                                        __STATIC_CAST(_RopeLeaf*, __right)->_M_data,
                                        __right->_M_size._M_data);
      }
    } else if (_RopeRep::_S_concat == __left->_M_tag &&
               _RopeRep::_S_leaf == __STATIC_CAST(_RopeConcatenation*, __left)->_M_right->_M_tag) {
      _RopeLeaf* __leftright =
        __STATIC_CAST(_RopeLeaf*, __STATIC_CAST(_RopeConcatenation*, __left)->_M_right);
      if (__leftright->_M_size._M_data + __right->_M_size._M_data <= _S_copy_max) {
        _RopeRep* __leftleft = __STATIC_CAST(_RopeConcatenation*, __left)->_M_left;
        _RopeRep* __rest = _S_leaf_concat_char_iter(__leftright,
                                                    __STATIC_CAST(_RopeLeaf*, __right)->_M_data,
                                                    __right->_M_size._M_data);
        __leftleft->_M_ref_nonnil();
        _STLP_TRY {
          return _S_tree_concat(__leftleft, __rest);
        }
        _STLP_UNWIND(_S_unref(__leftleft); _S_unref(__rest))
      }
    }
  }
  __left->_M_ref_nonnil();
  __right->_M_ref_nonnil();
  _STLP_TRY {
    return _S_tree_concat(__left, __right);
  }
  _STLP_UNWIND(_S_unref(__left); _S_unref(__right))
  _STLP_RET_AFTER_THROW(0)
}

template <class _CharT, class _Alloc>
__RopeRep__*
rope<_CharT,_Alloc>::_S_substring(_RopeRep* __base,
                                  size_t __start, size_t __endp1) {
  if (0 == __base) return 0;
  size_t __len = __base->_M_size._M_data;
  size_t __adj_endp1;
  const size_t __lazy_threshold = 128;

  if (__endp1 >= __len) {
    if (0 == __start) {
      __base->_M_ref_nonnil();
      return __base;
    } else {
      __adj_endp1 = __len;
    }
  } else {
    __adj_endp1 = __endp1;
  }
  switch(__base->_M_tag) {
  case _RopeRep::_S_concat:
  {
    _RopeConcatenation* __c = __STATIC_CAST(_RopeConcatenation*, __base);
    _RopeRep* __left = __c->_M_left;
    _RopeRep* __right = __c->_M_right;
    size_t __left_len = __left->_M_size._M_data;
    _RopeRep* __result;

    if (__adj_endp1 <= __left_len) {
      return _S_substring(__left, __start, __endp1);
    } else if (__start >= __left_len) {
      return _S_substring(__right, __start - __left_len,
                          __adj_endp1 - __left_len);
    }
    _Self_destruct_ptr __left_result(_S_substring(__left, __start, __left_len));
    _Self_destruct_ptr __right_result(_S_substring(__right, 0, __endp1 - __left_len));
    _STLP_MPWFIX_TRY    //*TY 06/01/2000 - mpw forgets to call dtor on __left_result and __right_result without this try block
    __result = _S_concat_rep(__left_result, __right_result);
    // _STLP_ASSERT(1 == __result->_M_ref_count)
    return __result;
    _STLP_MPWFIX_CATCH    //*TY 06/01/2000 -
  }
  case _RopeRep::_S_leaf:
  {
    _RopeLeaf* __l = __STATIC_CAST(_RopeLeaf*, __base);
    _RopeLeaf* __result;
    size_t __result_len;
    if (__start >= __adj_endp1) return 0;
    __result_len = __adj_endp1 - __start;
    if (__result_len > __lazy_threshold) goto lazy;
    const _CharT* __section = __l->_M_data + __start;
    // We should sometimes create substring node instead.
    __result = _S_RopeLeaf_from_unowned_char_ptr(__section, __result_len,
                                                 __base->get_allocator());
    return __result;
  }
  case _RopeRep::_S_substringfn:
  // Avoid introducing multiple layers of substring nodes.
  {
    _RopeSubstring* __old = __STATIC_CAST(_RopeSubstring*, __base);
    size_t __result_len;
    if (__start >= __adj_endp1) return 0;
    __result_len = __adj_endp1 - __start;
    if (__result_len > __lazy_threshold) {
      _RopeSubstring* __result = _S_new_RopeSubstring(__old->_M_base,
                                                      __start + __old->_M_start,
                                                      __adj_endp1 - __start,
                                                      __base->get_allocator());
      return __result;
    } // *** else fall through: ***
  }
  case _RopeRep::_S_function:
  {
    _RopeFunction* __f = __STATIC_CAST(_RopeFunction*, __base);
    if (__start >= __adj_endp1) return 0;
    size_t __result_len = __adj_endp1 - __start;

    if (__result_len > __lazy_threshold) goto lazy;
    _CharT* __section = __base->_M_size.allocate(_S_rounded_up_size(__result_len));
    _STLP_TRY {
      (*(__f->_M_fn))(__start, __result_len, __section);
    }
    _STLP_UNWIND(_RopeRep::_S_free_string(__section,
                                          __result_len, __base->get_allocator()))
    _S_construct_null(__section + __result_len);
    return _S_new_RopeLeaf(__section, __result_len,
                           __base->get_allocator());
  }
  }
  /*NOTREACHED*/
  _STLP_ASSERT(false)
  lazy:
  {
    // Create substring node.
    return _S_new_RopeSubstring(__base, __start, __adj_endp1 - __start,
                                __base->get_allocator());
  }
}

template<class _CharT>
class _Rope_flatten_char_consumer : public _Rope_char_consumer<_CharT> {
private:
  _CharT* _M_buf_ptr;
public:
  _Rope_flatten_char_consumer(_CharT* __buffer) {
    _M_buf_ptr = __buffer;
  }
  ~_Rope_flatten_char_consumer() {}
  bool operator() (const _CharT* __leaf, size_t __n) {
    _STLP_PRIV __ucopy_n(__leaf, __n, _M_buf_ptr);
    _M_buf_ptr += __n;
    return true;
  }
};

template<class _CharT>
class _Rope_find_char_char_consumer : public _Rope_char_consumer<_CharT> {
private:
  _CharT _M_pattern;
public:
  size_t _M_count;  // Number of nonmatching characters
  _Rope_find_char_char_consumer(_CharT __p)
    : _M_pattern(__p), _M_count(0) {}
  ~_Rope_find_char_char_consumer() {}
  bool operator() (const _CharT* __leaf, size_t __n) {
    size_t __i;
    for (__i = 0; __i < __n; ++__i) {
      if (__leaf[__i] == _M_pattern) {
        _M_count += __i; return false;
      }
    }
    _M_count += __n; return true;
  }
};

#if !defined (_STLP_USE_NO_IOSTREAMS)
template<class _CharT, class _Traits>
// Here _CharT is both the stream and rope character type.
class _Rope_insert_char_consumer : public _Rope_char_consumer<_CharT> {
private:
  typedef basic_ostream<_CharT,_Traits> _Insert_ostream;
  typedef _Rope_insert_char_consumer<_CharT,_Traits> _Self;
  _Insert_ostream& _M_o;

  //explicitely defined as private to avoid warnings:
  _Self& operator = (_Self const&);
public:
  _Rope_insert_char_consumer(_Insert_ostream& __writer)
    : _M_o(__writer) {}
  ~_Rope_insert_char_consumer() {}
  // Caller is presumed to own the ostream
  bool operator() (const _CharT* __leaf, size_t __n);
  // Returns true to continue traversal.
};

template<class _CharT, class _Traits>
bool _Rope_insert_char_consumer<_CharT, _Traits>::operator()
  (const _CharT* __leaf, size_t __n) {
  size_t __i;
  //  We assume that formatting is set up correctly for each element.
  for (__i = 0; __i < __n; ++__i) _M_o.put(__leaf[__i]);
  return true;
}
#endif /* !_STLP_USE_NO_IOSTREAMS */

template <class _CharT, class _Alloc, class _CharConsumer>
bool _S_apply_to_pieces(_CharConsumer& __c,
                        _Rope_RopeRep<_CharT, _Alloc> * __r,
                        size_t __begin, size_t __end) {
  typedef _Rope_RopeRep<_CharT, _Alloc> _RopeRep;
  typedef _Rope_RopeConcatenation<_CharT,_Alloc> _RopeConcatenation;
  typedef _Rope_RopeLeaf<_CharT,_Alloc> _RopeLeaf;
  typedef _Rope_RopeFunction<_CharT,_Alloc> _RopeFunction;

  if (0 == __r) return true;
  switch(__r->_M_tag) {
  case _RopeRep::_S_concat:
  {
    _RopeConcatenation* __conc = __STATIC_CAST(_RopeConcatenation*, __r);
    _RopeRep* __left =  __conc->_M_left;
    size_t __left_len = __left->_M_size._M_data;
    if (__begin < __left_len) {
      size_t __left_end = (min) (__left_len, __end);
      if (!_S_apply_to_pieces(__c, __left, __begin, __left_end))
        return false;
    }
    if (__end > __left_len) {
      _RopeRep* __right =  __conc->_M_right;
      size_t __right_start = (max)(__left_len, __begin);
      if (!_S_apply_to_pieces(__c, __right,
                              __right_start - __left_len,
                              __end - __left_len)) {
        return false;
      }
    }
  }
  return true;
  case _RopeRep::_S_leaf:
  {
    _RopeLeaf* __l = __STATIC_CAST(_RopeLeaf*, __r);
    return __c(__l->_M_data + __begin, __end - __begin);
  }
  case _RopeRep::_S_function:
  case _RopeRep::_S_substringfn:
  {
    _RopeFunction* __f = __STATIC_CAST(_RopeFunction*, __r);
    size_t __len = __end - __begin;
    bool __result;
    _CharT* __buffer = __r->get_allocator().allocate(__len);
    _STLP_TRY {
      (*(__f->_M_fn))(__begin, __len, __buffer);
      __result = __c(__buffer, __len);
      __r->get_allocator().deallocate(__buffer, __len);
    }
    _STLP_UNWIND((__r->get_allocator().deallocate(__buffer, __len)))
    return __result;
  }
  default:
    _STLP_ASSERT(false)
    /*NOTREACHED*/
    return false;
  }
}

#if !defined (_STLP_USE_NO_IOSTREAMS)
template<class _CharT, class _Traits>
inline void _Rope_fill(basic_ostream<_CharT, _Traits>& __o, streamsize __n) {
  char __f = __o.fill();
  for (streamsize __i = 0; __i < __n; ++__i) __o.put(__f);
}

template<class _CharT, class _Traits, class _Alloc>
basic_ostream<_CharT, _Traits>& _S_io_get(basic_ostream<_CharT, _Traits>& __o,
                                          const rope<_CharT, _Alloc>& __r, const __true_type& /*_IsBasicCharType*/) {
  streamsize __w = __o.width();
  const bool __left = (__o.flags() & ios::left) != 0;
  size_t __rope_len = __r.size();
  _Rope_insert_char_consumer<_CharT, _Traits> __c(__o);

  const bool __need_pad = (((sizeof(streamsize) > sizeof(size_t)) && (__STATIC_CAST(streamsize, __rope_len) < __w)) ||
                           ((sizeof(streamsize) <= sizeof(size_t)) && (__rope_len < __STATIC_CAST(size_t, __w))));
  streamsize __pad_len = __need_pad ? __w - __rope_len : 0;

  if (!__left && __pad_len > 0) {
    _Rope_fill(__o, __pad_len);
  }
  __r.apply_to_pieces(0, __rope_len, __c);
  if (__left && __pad_len > 0) {
    _Rope_fill(__o, __pad_len);
  }
  return __o;
}

template<class _CharT, class _Traits, class _Alloc>
basic_ostream<_CharT, _Traits>& _S_io_get(basic_ostream<_CharT, _Traits>& __o,
                                         const rope<_CharT, _Alloc>& __r, const __false_type& /*_IsBasicCharType*/) {
  streamsize __w = __o.width();
  size_t __rope_len = __r.size();
  _Rope_insert_char_consumer<_CharT, _Traits> __c(__o);

  __o.width(__w /__rope_len);
  _STLP_TRY {
    __r.apply_to_pieces(0, __rope_len, __c);
    __o.width(__w);
  }
  _STLP_UNWIND(__o.width(__w))
  return __o;
}

template<class _CharT, class _Traits, class _Alloc>
basic_ostream<_CharT, _Traits>& operator<<(basic_ostream<_CharT, _Traits>& __o,
                                           const rope<_CharT, _Alloc>& __r) {
  typedef typename _IsIntegral<_CharT>::_Ret _Char_Is_Integral;
  return _S_io_get(__o, __r, _Char_Is_Integral());
}
#endif /* NO_IOSTREAMS */

template <class _CharT, class _Alloc>
_CharT* rope<_CharT,_Alloc>::_S_flatten(_RopeRep* __r,
                                        size_t __start, size_t __len,
                                        _CharT* __buffer) {
  _Rope_flatten_char_consumer<_CharT> __c(__buffer);
  _S_apply_to_pieces(__c, __r, __start, __start + __len);
  return(__buffer + __len);
}

template <class _CharT, class _Alloc>
size_t rope<_CharT,_Alloc>::find(_CharT __pattern, size_t __start) const {
  _Rope_find_char_char_consumer<_CharT> __c(__pattern);
  _S_apply_to_pieces(__c, _M_tree_ptr._M_data, __start, size());
  size_type __result_pos = __start + __c._M_count;
#ifndef _STLP_OLD_ROPE_SEMANTICS
  if (__result_pos == size()) __result_pos = npos;
#endif
  return __result_pos;
}

template <class _CharT, class _Alloc>
_CharT*
rope<_CharT,_Alloc>::_S_flatten(_Rope_RopeRep<_CharT, _Alloc>* __r, _CharT* __buffer) {
  if (0 == __r) return __buffer;
  switch(__r->_M_tag) {
  case _RopeRep::_S_concat:
  {
    _RopeConcatenation* __c = __STATIC_CAST(_RopeConcatenation*, __r);
    _RopeRep* __left = __c->_M_left;
    _RopeRep* __right = __c->_M_right;
    _CharT* __rest = _S_flatten(__left, __buffer);
    return _S_flatten(__right, __rest);
  }
  case _RopeRep::_S_leaf:
  {
    _RopeLeaf* __l = __STATIC_CAST(_RopeLeaf*, __r);
    return _STLP_PRIV __ucopy_n(__l->_M_data, __l->_M_size._M_data, __buffer).second;
  }
  case _RopeRep::_S_function:
  case _RopeRep::_S_substringfn:
  // We dont yet do anything with substring nodes.
  // This needs to be fixed before ropefiles will work well.
  {
    _RopeFunction* __f = __STATIC_CAST(_RopeFunction*, __r);
    (*(__f->_M_fn))(0, __f->_M_size._M_data, __buffer);
    return __buffer + __f->_M_size._M_data;
  }
  default:
    _STLP_ASSERT(false)
    /*NOTREACHED*/
    return 0;
  }
}

#ifdef _STLP_DEBUG
// This needs work for _CharT != char
template <class _CharT, class _Alloc>
void rope<_CharT,_Alloc>::_S_dump(_RopeRep* __r, int __indent) {
  for (int __i = 0; __i < __indent; ++__i) putchar(' ');
  if (0 == __r) {
    printf("NULL\n"); return;
  }
  if (_RopeRep::_S_concat == __r->_M_tag) {
    _RopeConcatenation* __c = __STATIC_CAST(_RopeConcatenation*, __r);
    _RopeRep* __left = __c->_M_left;
    _RopeRep* __right = __c->_M_right;
    printf("Concatenation %p (rc = %ld, depth = %d, len = %ld, %s balanced)\n",
      __r, __r->_M_ref_count, __r->_M_depth, __r->_M_size._M_data,
      __r->_M_is_balanced? "" : "not");
    _S_dump(__left, __indent + 2);
    _S_dump(__right, __indent + 2);
    return;
  }
  else {
    const char* __kind;

    switch (__r->_M_tag) {
      case _RopeRep::_S_leaf:
        __kind = "Leaf";
        break;
      case _RopeRep::_S_function:
        __kind = "Function";
        break;
      case _RopeRep::_S_substringfn:
        __kind = "Function representing substring";
        break;
      default:
        __kind = "(corrupted kind field!)";
    }
    printf("%s %p (rc = %ld, depth = %d, len = %ld) ",
     __kind, __r, __r->_M_ref_count, __r->_M_depth, __r->_M_size._M_data);
    if (sizeof(_CharT) == 1) {
      const int __max_len = 40;
      _Self_destruct_ptr __prefix(_S_substring(__r, 0, __max_len));
      _CharT __buffer[__max_len + 1];
      bool __too_big = __r->_M_size._M_data > __prefix->_M_size._M_data;

      _S_flatten(__prefix, __buffer);
      __buffer[__prefix->_M_size._M_data] = _STLP_DEFAULT_CONSTRUCTED(_CharT);
      printf("%s%s\n", (char*)__buffer, __too_big? "...\n" : "\n");
    } else {
      printf("\n");
    }
  }
}
#endif /* _STLP_DEBUG */

# define __ROPE_TABLE_BODY  = { \
/* 0 */1, /* 1 */2, /* 2 */3, /* 3 */5, /* 4 */8, /* 5 */13, /* 6 */21,         \
/* 7 */34, /* 8 */55, /* 9 */89, /* 10 */144, /* 11 */233, /* 12 */377,         \
/* 13 */610, /* 14 */987, /* 15 */1597, /* 16 */2584, /* 17 */4181,             \
/* 18 */6765ul, /* 19 */10946ul, /* 20 */17711ul, /* 21 */28657ul, /* 22 */46368ul,   \
/* 23 */75025ul, /* 24 */121393ul, /* 25 */196418ul, /* 26 */317811ul,                \
/* 27 */514229ul, /* 28 */832040ul, /* 29 */1346269ul, /* 30 */2178309ul,             \
/* 31 */3524578ul, /* 32 */5702887ul, /* 33 */9227465ul, /* 34 */14930352ul,          \
/* 35 */24157817ul, /* 36 */39088169ul, /* 37 */63245986ul, /* 38 */102334155ul,      \
/* 39 */165580141ul, /* 40 */267914296ul, /* 41 */433494437ul,                        \
/* 42 */701408733ul, /* 43 */1134903170ul, /* 44 */1836311903ul,                      \
/* 45 */2971215073ul }

template <class _CharT, class _Alloc>
const unsigned long
rope<_CharT,_Alloc>::_S_min_len[__ROPE_DEPTH_SIZE] __ROPE_TABLE_BODY;
# undef __ROPE_DEPTH_SIZE
# undef __ROPE_MAX_DEPTH
# undef __ROPE_TABLE_BODY

// These are Fibonacci numbers < 2**32.

template <class _CharT, class _Alloc>
__RopeRep__* rope<_CharT,_Alloc>::_S_balance(_RopeRep* __r) {
  _RopeRep* __forest[_RopeRep::_S_max_rope_depth + 1] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                         0,0,0,0,0,0};
  _RopeRep* __result = 0;
  int __i;
  // Invariant:
  // The concatenation of forest in descending order is equal to __r.
  // __forest[__i]._M_size._M_data >= _S_min_len[__i]
  // __forest[__i]._M_depth = __i
  // References from forest are included in refcount.

  _STLP_TRY {
    _S_add_to_forest(__r, __forest);
    for (__i = 0; __i <= _RopeRep::_S_max_rope_depth; ++__i)
      if (0 != __forest[__i]) {
        _Self_destruct_ptr __old(__result);
        __result = _S_concat_rep(__forest[__i], __result);
        __forest[__i]->_M_unref_nonnil();
# ifdef _STLP_USE_EXCEPTIONS
        __forest[__i] = 0;
# endif
      }
    }
    _STLP_UNWIND(for(__i = 0; __i <= _RopeRep::_S_max_rope_depth; ++__i)
    _S_unref(__forest[__i]))
    if (__result->_M_depth > _RopeRep::_S_max_rope_depth) {
      __stl_throw_range_error("rope too long");
    }
    return(__result);
}


template <class _CharT, class _Alloc>
void
rope<_CharT,_Alloc>::_S_add_to_forest(_RopeRep* __r, _RopeRep** __forest)
{
    if (__r -> _M_is_balanced) {
      _S_add_leaf_to_forest(__r, __forest);
      return;
    }
    _STLP_ASSERT(__r->_M_tag == _RopeRep::_S_concat)
    {
      _RopeConcatenation* __c = (_RopeConcatenation*)__r;

      _S_add_to_forest(__c->_M_left, __forest);
      _S_add_to_forest(__c->_M_right, __forest);
    }
}


template <class _CharT, class _Alloc>
void
rope<_CharT,_Alloc>::_S_add_leaf_to_forest(_RopeRep* __r, _RopeRep** __forest)
{
    _RopeRep* __insertee;       // included in refcount
    _RopeRep* __too_tiny = 0;      // included in refcount
    int __i;          // forest[0..__i-1] is empty
    size_t __s = __r->_M_size._M_data;

    for (__i = 0; __s >= _S_min_len[__i+1]/* not this bucket */; ++__i) {
  if (0 != __forest[__i]) {
        _Self_destruct_ptr __old(__too_tiny);
      __too_tiny = _S_concat_and_set_balanced(__forest[__i], __too_tiny);
      __forest[__i]->_M_unref_nonnil();
      __forest[__i] = 0;
  }
    }
    {
    _Self_destruct_ptr __old(__too_tiny);
  __insertee = _S_concat_and_set_balanced(__too_tiny, __r);
    }
    // Too_tiny dead, and no longer included in refcount.
    // Insertee is live and included.
    _STLP_ASSERT(_S_is_almost_balanced(__insertee))
    _STLP_ASSERT(__insertee->_M_depth <= __r->_M_depth + 1)
    for (;; ++__i) {
      if (0 != __forest[__i]) {
        _Self_destruct_ptr __old(__insertee);
      __insertee = _S_concat_and_set_balanced(__forest[__i], __insertee);
      __forest[__i]->_M_unref_nonnil();
      __forest[__i] = 0;
      _STLP_ASSERT(_S_is_almost_balanced(__insertee))
  }
  _STLP_ASSERT(_S_min_len[__i] <= __insertee->_M_size._M_data)
  _STLP_ASSERT(__forest[__i] == 0)
  if (__i == _RopeRep::_S_max_rope_depth ||
        __insertee->_M_size._M_data < _S_min_len[__i+1]) {
      __forest[__i] = __insertee;
      // refcount is OK since __insertee is now dead.
      return;
  }
    }
}

template <class _CharT, class _Alloc>
_CharT
rope<_CharT,_Alloc>::_S_fetch(_RopeRep* __r, size_type __i)
{
    _CharT* __cstr = __r->_M_c_string;

    _STLP_ASSERT(__i < __r->_M_size._M_data)
    if (0 != __cstr) return __cstr[__i];
    for(;;) {
      switch(__r->_M_tag) {
  case _RopeRep::_S_concat:
      {
    _RopeConcatenation* __c = (_RopeConcatenation*)__r;
    _RopeRep* __left = __c->_M_left;
    size_t __left_len = __left->_M_size._M_data;

    if (__i >= __left_len) {
        __i -= __left_len;
        __r = __c->_M_right;
    } else {
        __r = __left;
    }
      }
      break;
  case _RopeRep::_S_leaf:
      {
    _RopeLeaf* __l = (_RopeLeaf*)__r;
    return __l->_M_data[__i];
      }
  case _RopeRep::_S_function:
  case _RopeRep::_S_substringfn:
      {
    _RopeFunction* __f = (_RopeFunction*)__r;
    _CharT __result;

    (*(__f->_M_fn))(__i, 1, &__result);
    return __result;
      }
      }
    }
#if defined(_STLP_NEED_UNREACHABLE_RETURN)
    return 0;
#endif
}

// Return a uniquely referenced character slot for the given
// position, or 0 if that's not possible.
template <class _CharT, class _Alloc>
_CharT*
rope<_CharT,_Alloc>::_S_fetch_ptr(_RopeRep* __r, size_type __i)
{
    _RopeRep* __clrstack[_RopeRep::_S_max_rope_depth];
    size_t __csptr = 0;

    for(;;) {
      // if (__r->_M_ref_count > 1) return 0;
      if ( __r->_M_incr() > 2 ) {
        __r->_M_decr();
        return 0;
      }
      switch(__r->_M_tag) {
  case _RopeRep::_S_concat:
      {
    _RopeConcatenation* __c = (_RopeConcatenation*)__r;
    _RopeRep* __left = __c->_M_left;
    size_t __left_len = __left->_M_size._M_data;

    if (__c->_M_c_string != 0) __clrstack[__csptr++] = __c;
    if (__i >= __left_len) {
        __i -= __left_len;
        __r = __c->_M_right;
    } else {
        __r = __left;
    }
      }
      break;
  case _RopeRep::_S_leaf:
      {
    _RopeLeaf* __l = (_RopeLeaf*)__r;
    if (__l->_M_c_string != __l->_M_data && __l->_M_c_string != 0)
        __clrstack[__csptr++] = __l;
    while (__csptr > 0) {
        -- __csptr;
        _RopeRep* __d = __clrstack[__csptr];
        __d->_M_free_c_string();
        __d->_M_c_string = 0;
    }
    return __l->_M_data + __i;
      }
  case _RopeRep::_S_function:
  case _RopeRep::_S_substringfn:
      return 0;
      }
    }
#if defined(_STLP_NEED_UNREACHABLE_RETURN)
    return 0;
#endif

}

// The following could be implemented trivially using
// lexicographical_compare_3way.
// We do a little more work to avoid dealing with rope iterators for
// flat strings.
template <class _CharT, class _Alloc>
int
rope<_CharT,_Alloc>::_S_compare (const _RopeRep* __left,
                                 const _RopeRep* __right) {
  size_t __left_len;
  size_t __right_len;

  if (0 == __right) return 0 != __left;
  if (0 == __left) return -1;
  __left_len = __left->_M_size._M_data;
  __right_len = __right->_M_size._M_data;
  if (_RopeRep::_S_leaf == __left->_M_tag) {
    const _RopeLeaf* __l = __STATIC_CAST(const _RopeLeaf*, __left);
    if (_RopeRep::_S_leaf == __right->_M_tag) {
      const _RopeLeaf* __r = __STATIC_CAST(const _RopeLeaf*, __right);
      return _STLP_PRIV __lexicographical_compare_3way(__l->_M_data, __l->_M_data + __left_len,
                                                       __r->_M_data, __r->_M_data + __right_len);
    }
    else {
      const_iterator __rstart(__right, 0);
      const_iterator __rend(__right, __right_len);
      return _STLP_PRIV __lexicographical_compare_3way(__l->_M_data, __l->_M_data + __left_len,
                                                       __rstart, __rend);
    }
  }
  else {
    const_iterator __lstart(__left, 0);
    const_iterator __lend(__left, __left_len);
    if (_RopeRep::_S_leaf == __right->_M_tag) {
      const _RopeLeaf* __r = __STATIC_CAST(const _RopeLeaf*, __right);
      return _STLP_PRIV __lexicographical_compare_3way(__lstart, __lend,
                                                       __r->_M_data, __r->_M_data + __right_len);
    }
    else {
      const_iterator __rstart(__right, 0);
      const_iterator __rend(__right, __right_len);
      return _STLP_PRIV __lexicographical_compare_3way(__lstart, __lend, __rstart, __rend);
    }
  }
}

// Assignment to reference proxies.
template <class _CharT, class _Alloc>
_Rope_char_ref_proxy<_CharT, _Alloc>&
_Rope_char_ref_proxy<_CharT, _Alloc>::operator= (_CharT __c) {
    _RopeRep* __old = _M_root->_M_tree_ptr._M_data;
  // First check for the case in which everything is uniquely
  // referenced.  In that case we can do this destructively.
  _CharT* __ptr = _My_rope::_S_fetch_ptr(__old, _M_pos);
  if (0 != __ptr) {
      *__ptr = __c;
      return *this;
  }
    _Self_destruct_ptr __left(
      _My_rope::_S_substring(__old, 0, _M_pos));
    _Self_destruct_ptr __right(
      _My_rope::_S_substring(__old, _M_pos+1, __old->_M_size._M_data));
    _Self_destruct_ptr __result_left(
      _My_rope::_S_destr_concat_char_iter(__left, &__c, 1));

    // _STLP_ASSERT(__left == __result_left || 1 == __result_left->_M_ref_count)
    _RopeRep* __result =
      _My_rope::_S_concat_rep(__result_left, __right);
    // _STLP_ASSERT(1 <= __result->_M_ref_count)
    _RopeRep::_S_unref(__old);
    _M_root->_M_tree_ptr._M_data = __result;
    return *this;
}

template <class _CharT, class _Alloc>
_Rope_char_ptr_proxy<_CharT, _Alloc>
_Rope_char_ref_proxy<_CharT, _Alloc>::operator& () const {
    return _Rope_char_ptr_proxy<_CharT, _Alloc>(*this);
}

template<class _CharT, class _Alloc>
_CharT rope<_CharT,_Alloc>::_S_empty_c_str[1] = { _CharT() };
// # endif

#if !defined (_STLP_STATIC_CONST_INIT_BUG) && !defined (_STLP_NO_STATIC_CONST_DEFINITION)
template <class _CharT, class _Alloc>
const size_t rope<_CharT, _Alloc>::npos;
#endif

template<class _CharT, class _Alloc>
const _CharT* rope<_CharT,_Alloc>::c_str() const {
  if (0 == _M_tree_ptr._M_data) {
    // Possibly redundant, but probably fast.
    _S_empty_c_str[0] = _STLP_DEFAULT_CONSTRUCTED(_CharT);
    return _S_empty_c_str;
  }
  _CharT* __old_c_string = _M_tree_ptr._M_data->_M_c_string;
  if (0 != __old_c_string) return __old_c_string;
  size_t __s = size();
  _CharT* __result = _STLP_CREATE_ALLOCATOR(allocator_type,(const allocator_type&)_M_tree_ptr, _CharT).allocate(__s + 1);
  _S_flatten(_M_tree_ptr._M_data, __result);
  _S_construct_null(__result + __s);
  __old_c_string = __STATIC_CAST(_CharT*, _Atomic_swap_ptr(__REINTERPRET_CAST(void* _STLP_VOLATILE*, &(_M_tree_ptr._M_data->_M_c_string)),
                                                           __result));
  if (0 != __old_c_string) {
    // It must have been added in the interim.  Hence it had to have been
    // separately allocated.  Deallocate the old copy, since we just
    // replaced it.
    _STLP_STD::_Destroy_Range(__old_c_string, __old_c_string + __s + 1);
    _STLP_CREATE_ALLOCATOR(allocator_type,(const allocator_type&)_M_tree_ptr, _CharT).deallocate(__old_c_string, __s + 1);
  }
  return __result;
}

template<class _CharT, class _Alloc>
const _CharT* rope<_CharT,_Alloc>::replace_with_c_str() {
  if (0 == _M_tree_ptr._M_data) {
    _S_empty_c_str[0] = _STLP_DEFAULT_CONSTRUCTED(_CharT);
    return _S_empty_c_str;
  }
  _CharT* __old_c_string = _M_tree_ptr._M_data->_M_c_string;
  if (_RopeRep::_S_leaf == _M_tree_ptr._M_data->_M_tag && 0 != __old_c_string) {
    return __old_c_string;
  }
  size_t __s = size();
  _CharT* __result = _M_tree_ptr.allocate(_S_rounded_up_size(__s));
  _S_flatten(_M_tree_ptr._M_data, __result);
  _S_construct_null(__result + __s);
  _M_tree_ptr._M_data->_M_unref_nonnil();
  _M_tree_ptr._M_data = _S_new_RopeLeaf(__result, __s, _M_tree_ptr);
  return __result;
}

// Algorithm specializations.  More should be added.

#if (!defined (_STLP_MSVC) || (_STLP_MSVC >= 1310)) && !defined (__DMC__)
// I couldn't get this to work with VC++
template<class _CharT,class _Alloc>
void _Rope_rotate(_Rope_iterator<_CharT,_Alloc> __first,
                  _Rope_iterator<_CharT,_Alloc> __middle,
                  _Rope_iterator<_CharT,_Alloc> __last) {
  _STLP_ASSERT(__first.container() == __middle.container() &&
               __middle.container() == __last.container())
  rope<_CharT,_Alloc>& __r(__first.container());
  rope<_CharT,_Alloc> __prefix = __r.substr(0, __first.index());
  rope<_CharT,_Alloc> __suffix =
    __r.substr(__last.index(), __r.size() - __last.index());
  rope<_CharT,_Alloc> __part1 =
    __r.substr(__middle.index(), __last.index() - __middle.index());
  rope<_CharT,_Alloc> __part2 =
    __r.substr(__first.index(), __middle.index() - __first.index());
  __r = __prefix;
  __r += __part1;
  __r += __part2;
  __r += __suffix;
}


# if 0
// Probably not useful for several reasons:
// - for SGIs 7.1 compiler and probably some others,
//   this forces lots of rope<wchar_t, ...> instantiations, creating a
//   code bloat and compile time problem.  (Fixed in 7.2.)
// - wchar_t is 4 bytes wide on most UNIX platforms, making it unattractive
//   for unicode strings.  Unsigned short may be a better character
//   type.
inline void rotate(
    _Rope_iterator<wchar_t, allocator<char> > __first,
                _Rope_iterator<wchar_t, allocator<char> > __middle,
                _Rope_iterator<wchar_t, allocator<char> > __last) {
    _Rope_rotate(__first, __middle, __last);
}
# endif
#endif /* _STLP_MSVC */

#   undef __RopeLeaf__
#   undef __RopeRep__
#   undef __RopeLeaf
#   undef __RopeRep
#   undef size_type

_STLP_END_NAMESPACE

# endif /* ROPEIMPL_H */

// Local Variables:
// mode:C++
// End:
