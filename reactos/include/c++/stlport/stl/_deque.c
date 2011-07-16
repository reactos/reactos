/*
 *
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996,1997
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
#ifndef _STLP_DEQUE_C
#define _STLP_DEQUE_C

#ifndef _STLP_INTERNAL_DEQUE_H
#  include <stl/_deque.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

// Non-inline member functions from _Deque_base.

template <class _Tp, class _Alloc >
_Deque_base<_Tp,_Alloc >::~_Deque_base() {
  if (_M_map._M_data) {
    _M_destroy_nodes(_M_start._M_node, this->_M_finish._M_node + 1);
    _M_map.deallocate(_M_map._M_data, _M_map_size._M_data);
  }
}

template <class _Tp, class _Alloc >
void _Deque_base<_Tp,_Alloc>::_M_initialize_map(size_t __num_elements) {
  size_t __num_nodes = __num_elements / this->buffer_size() + 1 ;

  _M_map_size._M_data = (max)((size_t) _S_initial_map_size, __num_nodes + 2);
  _M_map._M_data = _M_map.allocate(_M_map_size._M_data);

  _Tp** __nstart = _M_map._M_data + (_M_map_size._M_data - __num_nodes) / 2;
  _Tp** __nfinish = __nstart + __num_nodes;

  _STLP_TRY {
    _M_create_nodes(__nstart, __nfinish);
  }
  _STLP_UNWIND((_M_map.deallocate(_M_map._M_data, _M_map_size._M_data),
                _M_map._M_data = 0, _M_map_size._M_data = 0))
  _M_start._M_set_node(__nstart);
  this->_M_finish._M_set_node(__nfinish - 1);
  _M_start._M_cur = _M_start._M_first;
  this->_M_finish._M_cur = this->_M_finish._M_first + __num_elements % this->buffer_size();
}

template <class _Tp, class _Alloc >
void _Deque_base<_Tp,_Alloc>::_M_create_nodes(_Tp** __nstart,
                                              _Tp** __nfinish) {
  _Tp** __cur = __nstart;
  _STLP_TRY {
    for (; __cur < __nfinish; ++__cur)
      *__cur = _M_map_size.allocate(this->buffer_size());
  }
  _STLP_UNWIND(_M_destroy_nodes(__nstart, __cur))
}

template <class _Tp, class _Alloc >
void _Deque_base<_Tp,_Alloc>::_M_destroy_nodes(_Tp** __nstart,
                                               _Tp** __nfinish) {
  for (_Tp** __n = __nstart; __n < __nfinish; ++__n)
    _M_map_size.deallocate(*__n, this->buffer_size());
}

#if defined (_STLP_USE_PTR_SPECIALIZATIONS)
#  define deque _STLP_PTR_IMPL_NAME(deque)
#elif defined (_STLP_DEBUG)
#  define deque _STLP_NON_DBG_NAME(deque)
#else
_STLP_MOVE_TO_STD_NAMESPACE
#endif

#if defined (_STLP_NESTED_TYPE_PARAM_BUG)
// qualified references
#  define __iterator__   _Deque_iterator<_Tp, _Nonconst_traits<_Tp> >
#  define const_iterator _Deque_iterator<_Tp, _Const_traits<_Tp>  >
#  define iterator       __iterator__
#  define size_type      size_t
#  define value_type     _Tp
#else
#  define __iterator__   _STLP_TYPENAME_ON_RETURN_TYPE deque<_Tp, _Alloc>::iterator
#endif

template <class _Tp, class _Alloc >
deque<_Tp, _Alloc >&
deque<_Tp, _Alloc >::operator= (const deque<_Tp, _Alloc >& __x) {
  const size_type __len = size();
  if (&__x != this) {
    if (__len >= __x.size())
      erase(_STLP_STD::copy(__x.begin(), __x.end(), this->_M_start), this->_M_finish);
    else {
      const_iterator __mid = __x.begin() + difference_type(__len);
      _STLP_STD::copy(__x.begin(), __mid, this->_M_start);
      insert(this->_M_finish, __mid, __x.end());
    }
  }
  return *this;
}

template <class _Tp, class _Alloc >
void deque<_Tp, _Alloc >::_M_fill_insert(iterator __pos,
                                         size_type __n, const value_type& __x) {
#if !defined (_STLP_NO_MOVE_SEMANTIC)
  typedef typename __move_traits<_Tp>::implemented _Movable;
#endif
  if (__pos._M_cur == this->_M_start._M_cur) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    _STLP_TRY {
      uninitialized_fill(__new_start, this->_M_start, __x);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
    this->_M_start = __new_start;
  }
  else if (__pos._M_cur == this->_M_finish._M_cur) {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    _STLP_TRY {
      uninitialized_fill(this->_M_finish, __new_finish, __x);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node+1, __new_finish._M_node+1))
    this->_M_finish = __new_finish;
  }
  else
    _M_fill_insert_aux(__pos, __n, __x, _Movable());
}

#if !defined (_STLP_MEMBER_TEMPLATES)

template <class _Tp, class _Alloc >
void deque<_Tp, _Alloc>::insert(iterator __pos,
                                const value_type* __first, const value_type* __last) {
#if !defined (_STLP_NO_MOVE_SEMANTIC)
  typedef typename __move_traits<_Tp>::implemented _Movable;
#endif
  size_type __n = __last - __first;
  if (__pos._M_cur == this->_M_start._M_cur) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    _STLP_TRY {
      _STLP_PRIV __ucopy(__first, __last, __new_start);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
    this->_M_start = __new_start;
  }
  else if (__pos._M_cur == this->_M_finish._M_cur) {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    _STLP_TRY {
      _STLP_PRIV __ucopy(__first, __last, this->_M_finish);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1,
                                        __new_finish._M_node + 1))
    this->_M_finish = __new_finish;
  }
  else
    _M_insert_range_aux(__pos, __first, __last, __n, _Movable());
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::insert(iterator __pos,
                               const_iterator __first, const_iterator __last) {
#if !defined (_STLP_NO_MOVE_SEMANTIC)
  typedef typename __move_traits<_Tp>::implemented _Movable;
#endif
  size_type __n = __last - __first;
  if (__pos._M_cur == this->_M_start._M_cur) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    _STLP_TRY {
      _STLP_PRIV __ucopy(__first, __last, __new_start);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
    this->_M_start = __new_start;
  }
  else if (__pos._M_cur == this->_M_finish._M_cur) {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    _STLP_TRY {
      _STLP_PRIV __ucopy(__first, __last, this->_M_finish);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1,
                                        __new_finish._M_node + 1))
    this->_M_finish = __new_finish;
  }
  else
    _M_insert_range_aux(__pos, __first, __last, __n, _Movable());
}

#endif /* _STLP_MEMBER_TEMPLATES */

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_erase(iterator __pos,
                                         const __true_type& /*_Movable*/) {
  difference_type __index = __pos - this->_M_start;
  if (size_type(__index) < this->size() >> 1) {
    //We move the start of the deque one position to the right
    //starting from the rightmost element to move.
    iterator __src = __pos, __dst = __pos;
    _STLP_STD::_Destroy(&(*__dst));
    if (__src != this->_M_start) {
      for (--__src; __dst != this->_M_start; --__src, --__dst) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
    }
    _M_pop_front_aux();
  }
  else {
    iterator __src = __pos, __dst = __pos;
    _STLP_STD::_Destroy(&(*__dst));
    for (++__src; __src != this->_M_finish; ++__src, ++__dst) {
      _STLP_STD::_Move_Construct(&(*__dst), *__src);
      _STLP_STD::_Destroy_Moved(&(*__src));
    }
    //Duplication of the pop_back code without the destroy which has already been done:
    if (this->_M_finish._M_cur != this->_M_finish._M_first) {
      --this->_M_finish._M_cur;
    }
    else {
      _M_pop_back_aux();
    }
  }
  return this->_M_start + __index;
}

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_erase(iterator __pos,
                                         const __false_type& /*_Movable*/) {
  iterator __next = __pos;
  ++__next;
  difference_type __index = __pos - this->_M_start;
  if (size_type(__index) < this->size() >> 1) {
    copy_backward(this->_M_start, __pos, __next);
    pop_front();
  }
  else {
    _STLP_STD::copy(__next, this->_M_finish, __pos);
    pop_back();
  }
  return this->_M_start + __index;
}

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_erase(iterator __first, iterator __last,
                                         const __true_type& /*_Movable*/) {
  difference_type __n = __last - __first;
  difference_type __elems_before = __first - this->_M_start;
  if (__elems_before <= difference_type(this->size() - __n) / 2) {
    iterator __src = __first, __dst = __last;
    if (__src != this->_M_start) {
      for (--__src, --__dst; (__src >= this->_M_start) && (__dst >= __first); --__src, --__dst) {
        _STLP_STD::_Destroy(&(*__dst));
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
      }
      if (__dst >= __first) {
        //There are more elements to erase than elements to move
        _STLP_STD::_Destroy_Range(__first, ++__dst);
        _STLP_STD::_Destroy_Moved_Range(this->_M_start, __first);
      }
      else {
        //There are more elements to move than elements to erase
        for (; __src >= this->_M_start; --__src, --__dst) {
          _STLP_STD::_Destroy_Moved(&(*__dst));
          _STLP_STD::_Move_Construct(&(*__dst), *__src);
        }
        _STLP_STD::_Destroy_Moved_Range(this->_M_start, ++__dst);
      }
    }
    else {
      _STLP_STD::_Destroy_Range(this->_M_start, __last);
    }
    iterator __new_start = this->_M_start + __n;
    this->_M_destroy_nodes(this->_M_start._M_node, __new_start._M_node);
    this->_M_start = __new_start;
  }
  else {
    if (__last != this->_M_finish) {
      iterator __src = __last, __dst = __first;
      for (; (__src != this->_M_finish) && (__dst != __last); ++__src, ++__dst) {
        _STLP_STD::_Destroy(&(*__dst));
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
      }
      if (__dst != __last) {
        //There are more elements to erase than elements to move
        _STLP_STD::_Destroy_Range(__dst, __last);
        _STLP_STD::_Destroy_Moved_Range(__last, this->_M_finish);
      }
      else {
        //There are more elements to move than elements to erase
        for (; __src != this->_M_finish; ++__src, ++__dst) {
          _STLP_STD::_Destroy_Moved(&(*__dst));
          _STLP_STD::_Move_Construct(&(*__dst), *__src);
        }
        _STLP_STD::_Destroy_Moved_Range(__dst, this->_M_finish);
      }
    }
    else {
      _STLP_STD::_Destroy_Range(__first, this->_M_finish);
    }
    iterator __new_finish = this->_M_finish - __n;
    this->_M_destroy_nodes(__new_finish._M_node + 1, this->_M_finish._M_node + 1);
    this->_M_finish = __new_finish;
  }
  return this->_M_start + __elems_before;
}

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_erase(iterator __first, iterator __last,
                                         const __false_type& /*_Movable*/) {
  difference_type __n = __last - __first;
  difference_type __elems_before = __first - this->_M_start;
  if (__elems_before <= difference_type(this->size() - __n) / 2) {
    copy_backward(this->_M_start, __first, __last);
    iterator __new_start = this->_M_start + __n;
    _STLP_STD::_Destroy_Range(this->_M_start, __new_start);
    this->_M_destroy_nodes(this->_M_start._M_node, __new_start._M_node);
    this->_M_start = __new_start;
  }
  else {
    _STLP_STD::copy(__last, this->_M_finish, __first);
    iterator __new_finish = this->_M_finish - __n;
    _STLP_STD::_Destroy_Range(__new_finish, this->_M_finish);
    this->_M_destroy_nodes(__new_finish._M_node + 1, this->_M_finish._M_node + 1);
    this->_M_finish = __new_finish;
  }
  return this->_M_start + __elems_before;
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::clear() {
  for (_Map_pointer __node = this->_M_start._M_node + 1;
       __node < this->_M_finish._M_node;
       ++__node) {
    _STLP_STD::_Destroy_Range(*__node, *__node + this->buffer_size());
    this->_M_map_size.deallocate(*__node, this->buffer_size());
  }

  if (this->_M_start._M_node != this->_M_finish._M_node) {
    _STLP_STD::_Destroy_Range(this->_M_start._M_cur, this->_M_start._M_last);
    _STLP_STD::_Destroy_Range(this->_M_finish._M_first, this->_M_finish._M_cur);
    this->_M_map_size.deallocate(this->_M_finish._M_first, this->buffer_size());
  }
  else
    _STLP_STD::_Destroy_Range(this->_M_start._M_cur, this->_M_finish._M_cur);

  this->_M_finish = this->_M_start;
}

// Precondition: this->_M_start and this->_M_finish have already been initialized,
// but none of the deque's elements have yet been constructed.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_fill_initialize(const value_type& __val,
                                           const __false_type& /*_TrivialInit*/) {
  _Map_pointer __cur = this->_M_start._M_node;
  _STLP_TRY {
    for (; __cur < this->_M_finish._M_node; ++__cur)
      uninitialized_fill(*__cur, *__cur + this->buffer_size(), __val);
    uninitialized_fill(this->_M_finish._M_first, this->_M_finish._M_cur, __val);
  }
  _STLP_UNWIND(_STLP_STD::_Destroy_Range(this->_M_start, iterator(*__cur, __cur)))
}


// Called only if this->_M_finish._M_cur == this->_M_finish._M_last - 1.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_push_back_aux_v(const value_type& __t) {
  _M_reserve_map_at_back();
  *(this->_M_finish._M_node + 1) = this->_M_map_size.allocate(this->buffer_size());
  _STLP_TRY {
    _Copy_Construct(this->_M_finish._M_cur, __t);
    this->_M_finish._M_set_node(this->_M_finish._M_node + 1);
    this->_M_finish._M_cur = this->_M_finish._M_first;
  }
  _STLP_UNWIND(this->_M_map_size.deallocate(*(this->_M_finish._M_node + 1),
                                            this->buffer_size()))
}

#if defined(_STLP_DONT_SUP_DFLT_PARAM) && !defined(_STLP_NO_ANACHRONISMS)
// Called only if this->_M_finish._M_cur == this->_M_finish._M_last - 1.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_push_back_aux() {
  _M_reserve_map_at_back();
  *(this->_M_finish._M_node + 1) = this->_M_map_size.allocate(this->buffer_size());
  _STLP_TRY {
    _STLP_STD::_Construct(this->_M_finish._M_cur);
    this->_M_finish._M_set_node(this->_M_finish._M_node + 1);
    this->_M_finish._M_cur = this->_M_finish._M_first;
  }
  _STLP_UNWIND(this->_M_map_size.deallocate(*(this->_M_finish._M_node + 1),
                                            this->buffer_size()))
}
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

// Called only if this->_M_start._M_cur == this->_M_start._M_first.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_push_front_aux_v(const value_type& __t) {
  _M_reserve_map_at_front();
  *(this->_M_start._M_node - 1) = this->_M_map_size.allocate(this->buffer_size());
  _STLP_TRY {
    this->_M_start._M_set_node(this->_M_start._M_node - 1);
    this->_M_start._M_cur = this->_M_start._M_last - 1;
    _Copy_Construct(this->_M_start._M_cur, __t);
  }
  _STLP_UNWIND((++this->_M_start,
                this->_M_map_size.deallocate(*(this->_M_start._M_node - 1), this->buffer_size())))
}


#if defined (_STLP_DONT_SUP_DFLT_PARAM) && !defined (_STLP_NO_ANACHRONISMS)
// Called only if this->_M_start._M_cur == this->_M_start._M_first.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_push_front_aux() {
  _M_reserve_map_at_front();
  *(this->_M_start._M_node - 1) = this->_M_map_size.allocate(this->buffer_size());
  _STLP_TRY {
    this->_M_start._M_set_node(this->_M_start._M_node - 1);
    this->_M_start._M_cur = this->_M_start._M_last - 1;
    _STLP_STD::_Construct(this->_M_start._M_cur);
  }
  _STLP_UNWIND((++this->_M_start, this->_M_map_size.deallocate(*(this->_M_start._M_node - 1),
                                                               this->buffer_size())))
}
#endif /*_STLP_DONT_SUP_DFLT_PARAM && !_STLP_NO_ANACHRONISMS*/

// Called only if this->_M_finish._M_cur == this->_M_finish._M_first.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_pop_back_aux() {
  this->_M_map_size.deallocate(this->_M_finish._M_first, this->buffer_size());
  this->_M_finish._M_set_node(this->_M_finish._M_node - 1);
  this->_M_finish._M_cur = this->_M_finish._M_last - 1;
}

// Note that if the deque has at least one element (a precondition for this member
// function), and if this->_M_start._M_cur == this->_M_start._M_last, then the deque
// must have at least two nodes.
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_pop_front_aux() {
  if (this->_M_start._M_cur != this->_M_start._M_last - 1)
    ++this->_M_start._M_cur;
  else {
    this->_M_map_size.deallocate(this->_M_start._M_first, this->buffer_size());
    this->_M_start._M_set_node(this->_M_start._M_node + 1);
    this->_M_start._M_cur = this->_M_start._M_first;
  }
}

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_fill_insert_aux(iterator __pos, size_type __n,
                                                   const value_type& __x,
                                                   const __true_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = this->size();
  value_type __x_copy = __x;
  if (__elems_before <= difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      iterator __dst = __new_start;
      iterator __src = this->_M_start;
      for (; __src != __pos; ++__dst, ++__src) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_start = __new_start;
      uninitialized_fill(__dst, __src, __x_copy);
      __pos = __dst;
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    const difference_type __elems_after = difference_type(__length) - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {
      iterator __dst = __new_finish;
      iterator __src = this->_M_finish;
      for (--__src, --__dst; __src >= __pos; --__src, --__dst) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_finish = __new_finish;
      uninitialized_fill(__pos, __pos + __n, __x_copy);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
  return __pos;
}

template <class _Tp, class _Alloc >
__iterator__ deque<_Tp,_Alloc>::_M_fill_insert_aux(iterator __pos, size_type __n,
                                                   const value_type& __x,
                                                   const __false_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = this->size();
  value_type __x_copy = __x;
  if (__elems_before <= difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    iterator __old_start = this->_M_start;
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      if (__elems_before >= difference_type(__n)) {
        iterator __start_n = this->_M_start + difference_type(__n);
        _STLP_PRIV __ucopy(this->_M_start, __start_n, __new_start);
        this->_M_start = __new_start;
        _STLP_STD::copy(__start_n, __pos, __old_start);
        _STLP_STD::fill(__pos - difference_type(__n), __pos, __x_copy);
        __pos -= difference_type(__n);
      }
      else {
        _STLP_PRIV __uninitialized_copy_fill(this->_M_start, __pos, __new_start,
                                             this->_M_start, __x_copy);
        this->_M_start = __new_start;
        fill(__old_start, __pos, __x_copy);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    iterator __old_finish = this->_M_finish;
    const difference_type __elems_after =
      difference_type(__length) - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {
      if (__elems_after > difference_type(__n)) {
        iterator __finish_n = this->_M_finish - difference_type(__n);
        _STLP_PRIV __ucopy(__finish_n, this->_M_finish, this->_M_finish);
        this->_M_finish = __new_finish;
        copy_backward(__pos, __finish_n, __old_finish);
        fill(__pos, __pos + difference_type(__n), __x_copy);
      }
      else {
        _STLP_PRIV __uninitialized_fill_copy(this->_M_finish, __pos + difference_type(__n),
                                             __x_copy, __pos, this->_M_finish);
        this->_M_finish = __new_finish;
        fill(__pos, __old_finish, __x_copy);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
  return __pos;
}

#if !defined (_STLP_MEMBER_TEMPLATES)
template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_insert_range_aux(iterator __pos,
                                            const value_type* __first, const value_type* __last,
                                            size_type __n, const __true_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = size();
  if (__elems_before <= difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      iterator __dst = __new_start;
      iterator __src = this->_M_start;
      for (; __src != __pos; ++__dst, ++__src) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_start = __new_start;
      _STLP_PRIV __ucopy(__first, __last, __dst);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    const difference_type __elems_after = difference_type(__length) - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {
      iterator __dst = __new_finish;
      iterator __src = this->_M_finish;
      for (--__src, --__dst; __src >= __pos; --__src, --__dst) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_finish = __new_finish;
      _STLP_PRIV __ucopy(__first, __last, __pos);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_insert_range_aux(iterator __pos,
                                            const value_type* __first, const value_type* __last,
                                            size_type __n, const __false_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = size();
  if (__elems_before <= difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    iterator __old_start = this->_M_start;
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      if (__elems_before >= difference_type(__n)) {
        iterator __start_n = this->_M_start + difference_type(__n);
        _STLP_PRIV __ucopy(this->_M_start, __start_n, __new_start);
        this->_M_start = __new_start;
        _STLP_STD::copy(__start_n, __pos, __old_start);
        _STLP_STD::copy(__first, __last, __pos - difference_type(__n));
      }
      else {
        const value_type* __mid = __first + (difference_type(__n) - __elems_before);
        _STLP_PRIV __uninitialized_copy_copy(this->_M_start, __pos, __first, __mid, __new_start);
        this->_M_start = __new_start;
        _STLP_STD::copy(__mid, __last, __old_start);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    iterator __old_finish = this->_M_finish;
    const difference_type __elems_after =
      difference_type(__length) - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {

      if (__elems_after > difference_type(__n)) {
        iterator __finish_n = this->_M_finish - difference_type(__n);
        _STLP_PRIV __ucopy(__finish_n, this->_M_finish, this->_M_finish);
        this->_M_finish = __new_finish;
        _STLP_STD::copy_backward(__pos, __finish_n, __old_finish);
        _STLP_STD::copy(__first, __last, __pos);
      }
      else {
        const value_type* __mid = __first + __elems_after;
        _STLP_PRIV __uninitialized_copy_copy(__mid, __last, __pos, this->_M_finish, this->_M_finish);
        this->_M_finish = __new_finish;
        _STLP_STD::copy(__first, __mid, __pos);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_insert_range_aux(iterator __pos,
                                            const_iterator __first, const_iterator __last,
                                            size_type __n, const __true_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = size();
  if (__elems_before <= difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      iterator __dst = __new_start;
      iterator __src = this->_M_start;
      for (; __src != __pos; ++__dst, ++__src) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_start = __new_start;
      _STLP_PRIV __ucopy(__first, __last, __dst);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    const difference_type __elems_after = difference_type(__length) - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {
      iterator __dst = __new_finish;
      iterator __src = this->_M_finish;
      for (--__src, --__dst; __src >= __pos; --__src, --__dst) {
        _STLP_STD::_Move_Construct(&(*__dst), *__src);
        _STLP_STD::_Destroy_Moved(&(*__src));
      }
      this->_M_finish = __new_finish;
      _STLP_PRIV __ucopy(__first, __last, __pos);
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_insert_range_aux(iterator __pos,
                                            const_iterator __first, const_iterator __last,
                                            size_type __n, const __false_type& /*_Movable*/) {
  const difference_type __elems_before = __pos - this->_M_start;
  size_type __length = size();
  if (__elems_before < difference_type(__length / 2)) {
    iterator __new_start = _M_reserve_elements_at_front(__n);
    iterator __old_start = this->_M_start;
    __pos = this->_M_start + __elems_before;
    _STLP_TRY {
      if (__elems_before >= difference_type(__n)) {
        iterator __start_n = this->_M_start + __n;
        _STLP_PRIV __ucopy(this->_M_start, __start_n, __new_start);
        this->_M_start = __new_start;
        _STLP_STD::copy(__start_n, __pos, __old_start);
        _STLP_STD::copy(__first, __last, __pos - difference_type(__n));
      }
      else {
        const_iterator __mid = __first + (__n - __elems_before);
        _STLP_PRIV __uninitialized_copy_copy(this->_M_start, __pos, __first, __mid, __new_start);
        this->_M_start = __new_start;
        _STLP_STD::copy(__mid, __last, __old_start);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(__new_start._M_node, this->_M_start._M_node))
  }
  else {
    iterator __new_finish = _M_reserve_elements_at_back(__n);
    iterator __old_finish = this->_M_finish;
    const difference_type __elems_after = __length - __elems_before;
    __pos = this->_M_finish - __elems_after;
    _STLP_TRY {
      if (__elems_after > difference_type(__n)) {
        iterator __finish_n = this->_M_finish - difference_type(__n);
        _STLP_PRIV __ucopy(__finish_n, this->_M_finish, this->_M_finish);
        this->_M_finish = __new_finish;
        _STLP_STD::copy_backward(__pos, __finish_n, __old_finish);
        _STLP_STD::copy(__first, __last, __pos);
      }
      else {
        const_iterator __mid = __first + __elems_after;
        _STLP_PRIV __uninitialized_copy_copy(__mid, __last, __pos, this->_M_finish, this->_M_finish);
        this->_M_finish = __new_finish;
        _STLP_STD::copy(__first, __mid, __pos);
      }
    }
    _STLP_UNWIND(this->_M_destroy_nodes(this->_M_finish._M_node + 1, __new_finish._M_node + 1))
  }
}
#endif /* _STLP_MEMBER_TEMPLATES */

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_new_elements_at_front(size_type __new_elems) {
  size_type __new_nodes
      = (__new_elems + this->buffer_size() - 1) / this->buffer_size();
  _M_reserve_map_at_front(__new_nodes);
  size_type __i = 1;
  _STLP_TRY {
    for (; __i <= __new_nodes; ++__i)
      *(this->_M_start._M_node - __i) = this->_M_map_size.allocate(this->buffer_size());
  }
  _STLP_UNWIND(for (size_type __j = 1; __j < __i; ++__j)
                 this->_M_map_size.deallocate(*(this->_M_start._M_node - __j), this->buffer_size()))
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_new_elements_at_back(size_type __new_elems) {
  size_type __new_nodes
      = (__new_elems + this->buffer_size() - 1) / this->buffer_size();
  _M_reserve_map_at_back(__new_nodes);
  size_type __i = 1;
  _STLP_TRY {
    for (; __i <= __new_nodes; ++__i)
      *(this->_M_finish._M_node + __i) = this->_M_map_size.allocate(this->buffer_size());
  }
  _STLP_UNWIND(for (size_type __j = 1; __j < __i; ++__j)
                 this->_M_map_size.deallocate(*(this->_M_finish._M_node + __j), this->buffer_size()))
}

template <class _Tp, class _Alloc >
void deque<_Tp,_Alloc>::_M_reallocate_map(size_type __nodes_to_add,
                                          bool __add_at_front) {
  size_type __old_num_nodes = this->_M_finish._M_node - this->_M_start._M_node + 1;
  size_type __new_num_nodes = __old_num_nodes + __nodes_to_add;

  _Map_pointer __new_nstart;
  if (this->_M_map_size._M_data > 2 * __new_num_nodes) {
    __new_nstart = this->_M_map._M_data + (this->_M_map_size._M_data - __new_num_nodes) / 2
                     + (__add_at_front ? __nodes_to_add : 0);
    if (__new_nstart < this->_M_start._M_node)
      _STLP_STD::copy(this->_M_start._M_node, this->_M_finish._M_node + 1, __new_nstart);
    else
      _STLP_STD::copy_backward(this->_M_start._M_node, this->_M_finish._M_node + 1,
                               __new_nstart + __old_num_nodes);
  }
  else {
    size_type __new_map_size =
      this->_M_map_size._M_data + (max)((size_t)this->_M_map_size._M_data, __nodes_to_add) + 2;

    _Map_pointer __new_map = this->_M_map.allocate(__new_map_size);
    __new_nstart = __new_map + (__new_map_size - __new_num_nodes) / 2
                             + (__add_at_front ? __nodes_to_add : 0);
    _STLP_STD::copy(this->_M_start._M_node, this->_M_finish._M_node + 1, __new_nstart);
    this->_M_map.deallocate(this->_M_map._M_data, this->_M_map_size._M_data);

    this->_M_map._M_data = __new_map;
    this->_M_map_size._M_data = __new_map_size;
  }

  this->_M_start._M_set_node(__new_nstart);
  this->_M_finish._M_set_node(__new_nstart + __old_num_nodes - 1);
}

#if defined (deque)
#  undef deque
_STLP_MOVE_TO_STD_NAMESPACE
#endif

_STLP_END_NAMESPACE

#undef __iterator__
#undef iterator
#undef const_iterator
#undef size_type
#undef value_type

#endif /*  _STLP_DEQUE_C */

// Local Variables:
// mode:C++
// End:
