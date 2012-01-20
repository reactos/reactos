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
#ifndef _STLP_SLIST_BASE_C
#define _STLP_SLIST_BASE_C

#ifndef _STLP_INTERNAL_SLIST_BASE_H
#  include <stl/_slist_base.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Dummy>
_Slist_node_base*  _STLP_CALL
_Sl_global<_Dummy>::__previous(_Slist_node_base* __head,
                               const _Slist_node_base* __node) {
  while (__head && __head->_M_next != __node)
    __head = __head->_M_next;
  return __head;
}

template <class _Dummy>
void _STLP_CALL
_Sl_global<_Dummy>::__splice_after(_Slist_node_base* __pos, _Slist_node_base* __head) {
  _Slist_node_base* __before_last = __previous(__head, 0);
  if (__before_last != __head) {
    _Slist_node_base* __after = __pos->_M_next;
    __pos->_M_next = __head->_M_next;
    __head->_M_next = 0;
    __before_last->_M_next = __after;
  }
}

template <class _Dummy>
void _STLP_CALL
_Sl_global<_Dummy>::__splice_after(_Slist_node_base* __pos,
                                   _Slist_node_base* __before_first,
                                   _Slist_node_base* __before_last) {
  if (__pos != __before_first && __pos != __before_last) {
    _Slist_node_base* __first = __before_first->_M_next;
    _Slist_node_base* __after = __pos->_M_next;
    __before_first->_M_next = __before_last->_M_next;
    __pos->_M_next = __first;
    __before_last->_M_next = __after;
  }
}

template <class _Dummy>
_Slist_node_base* _STLP_CALL
_Sl_global<_Dummy>::__reverse(_Slist_node_base* __node) {
  _Slist_node_base* __result = __node;
  __node = __node->_M_next;
  __result->_M_next = 0;
  while(__node) {
    _Slist_node_base* __next = __node->_M_next;
    __node->_M_next = __result;
    __result = __node;
    __node = __next;
  }
  return __result;
}

template <class _Dummy>
size_t _STLP_CALL
_Sl_global<_Dummy>::size(_Slist_node_base* __node) {
  size_t __result = 0;
  for ( ; __node != 0; __node = __node->_M_next)
    ++__result;
  return __result;
}

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif /*  _STLP_SLIST_BASE_C */

// Local Variables:
// mode:C++
// End:
