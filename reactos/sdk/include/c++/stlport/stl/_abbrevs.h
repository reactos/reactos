/*
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

#ifndef _STLP_INTERNAL_ABBREVS_H
# define _STLP_INTERNAL_ABBREVS_H

// ugliness is intentional - to reduce conflicts
#  define input_iterator_tag             _In__ItT
#  define output_iterator_tag            _Ou__ItT
#  define bidirectional_iterator_tag     _Bd__ItT
#  define random_access_iterator_tag     _Ra__ItT
#  define input_iterator                 _In__It
#  define output_iterator                _Ou__It
#  define bidirectional_iterator         _Bd__It
#  define random_access_iterator         _Ra__It
#  define reverse_bidirectional_iterator _rBd__It
#  define reverse_iterator               _r__It
#  define back_insert_iterator           _bI__It
#  define front_insert_iterator          _fI__It
#  define raw_storage_iterator           _rS__It
#  define _Const_traits                  _C_Tr
#  define _Const_Const_traits            _CC_Tr
#  define _Nonconst_traits               _N_Tr
#  define _Nonconst_Const_traits         _NC_Tr

// ugliness is intentional - to reduce conflicts probability
#  define __malloc_alloc    M__A
#  define __node_alloc      D__A
#  define __new_alloc       N__A
#  define __debug_alloc     G__A
#  define _STLP_alloc_proxy P__A

#  define _Deque_iterator_base     _Dq__ItB
#  define _Deque_iterator          _Dq__It

#  define _Select1st                  _S1st
#  define _Select2nd                  _S2nd
#  define __move_source               __m_s
#  define _Vector_nonconst_traits     _V_nct

#  define _Ht_iterator                _Ht_It

#  define _List_node_base          _L__NB
#  define _List_iterator_base      _L__ItB
#  define _List_iterator           _L__It

#  define _Slist_iterator_base     _SL__ItB
#  define _Slist_iterator          _SL__It

#  define _Rb_tree_node_base       _rbT__NB
#  define _Rb_tree_node            _rbT__N
#  define _Rb_tree_base_iterator   _rbT__It
#  define _Rb_tree_base            _rbT__B

#  if defined (__DMC__) && defined (_STLP_DEBUG)
#    define _NonDbg_hashtable      _Nd_Ht
#    define _DBG_iter              _d__It
#  endif
#endif

