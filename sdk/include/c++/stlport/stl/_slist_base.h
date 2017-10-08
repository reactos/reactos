/*
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_SLIST_BASE_H
#define _STLP_INTERNAL_SLIST_BASE_H

#ifndef _STLP_INTERNAL_CSTDDEF
#  include <stl/_cstddef.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

struct _Slist_node_base {
  _Slist_node_base* _M_next;
};

inline _Slist_node_base*
__slist_make_link(_Slist_node_base* __prev_node,
                  _Slist_node_base* __new_node) {
  __new_node->_M_next = __prev_node->_M_next;
  __prev_node->_M_next = __new_node;
  return __new_node;
}


template <class _Dummy>
class _Sl_global {
public:
  // those used to be global functions
  // moved here to reduce code bloat without templatizing _Slist_iterator_base
  static size_t _STLP_CALL size(_Slist_node_base* __node);
  static _Slist_node_base* _STLP_CALL __reverse(_Slist_node_base* __node);
  static void _STLP_CALL __splice_after(_Slist_node_base* __pos,
                                        _Slist_node_base* __before_first,
                                        _Slist_node_base* __before_last);

  static void _STLP_CALL __splice_after(_Slist_node_base* __pos, _Slist_node_base* __head);

  static _Slist_node_base* _STLP_CALL __previous(_Slist_node_base* __head,
                                                 const _Slist_node_base* __node);
  static const _Slist_node_base* _STLP_CALL __previous(const _Slist_node_base* __head,
                                                       const _Slist_node_base* __node) {
    return _Sl_global<_Dummy>::__previous(__CONST_CAST(_Slist_node_base*, __head), __node);
  }
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS _Sl_global<bool>;
#endif

typedef _Sl_global<bool> _Sl_global_inst;

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION) && defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)
#  include <stl/_slist_base.c>
#endif

#endif /* _STLP_INTERNAL_SLIST_BASE_H */

// Local Variables:
// mode:C++
// End:
