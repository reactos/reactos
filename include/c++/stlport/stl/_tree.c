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
 * Modified CRP 7/10/00 for improved conformance / efficiency on insert_unique /
 * insert_equal with valid hint -- efficiency is improved all around, and it is
 * should now be standard conforming for complexity on insert point immediately
 * after hint (amortized constant time).
 *
 */
#ifndef _STLP_TREE_C
#define _STLP_TREE_C

#ifndef _STLP_INTERNAL_TREE_H
#  include <stl/_tree.h>
#endif

#if defined (_STLP_DEBUG)
#  define _Rb_tree _STLP_NON_DBG_NAME(Rb_tree)
#endif

// fbp: these defines are for outline methods definitions.
// needed for definitions to be portable. Should not be used in method bodies.
#if defined (_STLP_NESTED_TYPE_PARAM_BUG)
#  define __iterator__  _Rb_tree_iterator<_Value, _STLP_HEADER_TYPENAME _Traits::_NonConstTraits>
#  define __size_type__ size_t
#  define iterator __iterator__
#else
#  define __iterator__  _STLP_TYPENAME_ON_RETURN_TYPE _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc>::iterator
#  define __size_type__  _STLP_TYPENAME_ON_RETURN_TYPE _Rb_tree<_Key, _Compare, _Value, _KeyOfValue, _Traits, _Alloc>::size_type
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

#if defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rotate_left(_Rb_tree_node_base* __x,
                                 _Rb_tree_node_base*& __root) {
  _Rb_tree_node_base* __y = __x->_M_right;
  __x->_M_right = __y->_M_left;
  if (__y->_M_left != 0)
    __y->_M_left->_M_parent = __x;
  __y->_M_parent = __x->_M_parent;

  if (__x == __root)
    __root = __y;
  else if (__x == __x->_M_parent->_M_left)
    __x->_M_parent->_M_left = __y;
  else
    __x->_M_parent->_M_right = __y;
  __y->_M_left = __x;
  __x->_M_parent = __y;
}

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rotate_right(_Rb_tree_node_base* __x,
                                  _Rb_tree_node_base*& __root) {
  _Rb_tree_node_base* __y = __x->_M_left;
  __x->_M_left = __y->_M_right;
  if (__y->_M_right != 0)
    __y->_M_right->_M_parent = __x;
  __y->_M_parent = __x->_M_parent;

  if (__x == __root)
    __root = __y;
  else if (__x == __x->_M_parent->_M_right)
    __x->_M_parent->_M_right = __y;
  else
    __x->_M_parent->_M_left = __y;
  __y->_M_right = __x;
  __x->_M_parent = __y;
}

template <class _Dummy> void _STLP_CALL
_Rb_global<_Dummy>::_Rebalance(_Rb_tree_node_base* __x,
                               _Rb_tree_node_base*& __root) {
  __x->_M_color = _S_rb_tree_red;
  while (__x != __root && __x->_M_parent->_M_color == _S_rb_tree_red) {
    if (__x->_M_parent == __x->_M_parent->_M_parent->_M_left) {
      _Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_right;
      if (__y && __y->_M_color == _S_rb_tree_red) {
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __y->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        __x = __x->_M_parent->_M_parent;
      }
      else {
        if (__x == __x->_M_parent->_M_right) {
          __x = __x->_M_parent;
          _Rotate_left(__x, __root);
        }
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        _Rotate_right(__x->_M_parent->_M_parent, __root);
      }
    }
    else {
      _Rb_tree_node_base* __y = __x->_M_parent->_M_parent->_M_left;
      if (__y && __y->_M_color == _S_rb_tree_red) {
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __y->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        __x = __x->_M_parent->_M_parent;
      }
      else {
        if (__x == __x->_M_parent->_M_left) {
          __x = __x->_M_parent;
          _Rotate_right(__x, __root);
        }
        __x->_M_parent->_M_color = _S_rb_tree_black;
        __x->_M_parent->_M_parent->_M_color = _S_rb_tree_red;
        _Rotate_left(__x->_M_parent->_M_parent, __root);
      }
    }
  }
  __root->_M_color = _S_rb_tree_black;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_Rebalance_for_erase(_Rb_tree_node_base* __z,
                                         _Rb_tree_node_base*& __root,
                                         _Rb_tree_node_base*& __leftmost,
                                         _Rb_tree_node_base*& __rightmost) {
  _Rb_tree_node_base* __y = __z;
  _Rb_tree_node_base* __x;
  _Rb_tree_node_base* __x_parent;

  if (__y->_M_left == 0)     // __z has at most one non-null child. y == z.
    __x = __y->_M_right;     // __x might be null.
  else {
    if (__y->_M_right == 0)  // __z has exactly one non-null child. y == z.
      __x = __y->_M_left;    // __x is not null.
    else {                   // __z has two non-null children.  Set __y to
      __y = _Rb_tree_node_base::_S_minimum(__y->_M_right);   //   __z's successor.  __x might be null.
      __x = __y->_M_right;
    }
  }

  if (__y != __z) {          // relink y in place of z.  y is z's successor
    __z->_M_left->_M_parent = __y;
    __y->_M_left = __z->_M_left;
    if (__y != __z->_M_right) {
      __x_parent = __y->_M_parent;
      if (__x) __x->_M_parent = __y->_M_parent;
      __y->_M_parent->_M_left = __x;      // __y must be a child of _M_left
      __y->_M_right = __z->_M_right;
      __z->_M_right->_M_parent = __y;
    }
    else
      __x_parent = __y;
    if (__root == __z)
      __root = __y;
    else if (__z->_M_parent->_M_left == __z)
      __z->_M_parent->_M_left = __y;
    else
      __z->_M_parent->_M_right = __y;
    __y->_M_parent = __z->_M_parent;
    _STLP_STD::swap(__y->_M_color, __z->_M_color);
    __y = __z;
    // __y now points to node to be actually deleted
  }
  else {                        // __y == __z
    __x_parent = __y->_M_parent;
    if (__x) __x->_M_parent = __y->_M_parent;
    if (__root == __z)
      __root = __x;
    else {
      if (__z->_M_parent->_M_left == __z)
        __z->_M_parent->_M_left = __x;
      else
        __z->_M_parent->_M_right = __x;
    }

    if (__leftmost == __z) {
      if (__z->_M_right == 0)        // __z->_M_left must be null also
        __leftmost = __z->_M_parent;
    // makes __leftmost == _M_header if __z == __root
      else
        __leftmost = _Rb_tree_node_base::_S_minimum(__x);
    }
    if (__rightmost == __z) {
      if (__z->_M_left == 0)         // __z->_M_right must be null also
        __rightmost = __z->_M_parent;
    // makes __rightmost == _M_header if __z == __root
      else                      // __x == __z->_M_left
        __rightmost = _Rb_tree_node_base::_S_maximum(__x);
    }
  }

  if (__y->_M_color != _S_rb_tree_red) {
    while (__x != __root && (__x == 0 || __x->_M_color == _S_rb_tree_black))
      if (__x == __x_parent->_M_left) {
        _Rb_tree_node_base* __w = __x_parent->_M_right;
        if (__w->_M_color == _S_rb_tree_red) {
          __w->_M_color = _S_rb_tree_black;
          __x_parent->_M_color = _S_rb_tree_red;
          _Rotate_left(__x_parent, __root);
          __w = __x_parent->_M_right;
        }
        if ((__w->_M_left == 0 ||
             __w->_M_left->_M_color == _S_rb_tree_black) && (__w->_M_right == 0 ||
             __w->_M_right->_M_color == _S_rb_tree_black)) {
          __w->_M_color = _S_rb_tree_red;
          __x = __x_parent;
          __x_parent = __x_parent->_M_parent;
        } else {
          if (__w->_M_right == 0 ||
              __w->_M_right->_M_color == _S_rb_tree_black) {
            if (__w->_M_left) __w->_M_left->_M_color = _S_rb_tree_black;
            __w->_M_color = _S_rb_tree_red;
            _Rotate_right(__w, __root);
            __w = __x_parent->_M_right;
          }
          __w->_M_color = __x_parent->_M_color;
          __x_parent->_M_color = _S_rb_tree_black;
          if (__w->_M_right) __w->_M_right->_M_color = _S_rb_tree_black;
          _Rotate_left(__x_parent, __root);
          break;
        }
      } else {                  // same as above, with _M_right <-> _M_left.
        _Rb_tree_node_base* __w = __x_parent->_M_left;
        if (__w->_M_color == _S_rb_tree_red) {
          __w->_M_color = _S_rb_tree_black;
          __x_parent->_M_color = _S_rb_tree_red;
          _Rotate_right(__x_parent, __root);
          __w = __x_parent->_M_left;
        }
        if ((__w->_M_right == 0 ||
             __w->_M_right->_M_color == _S_rb_tree_black) && (__w->_M_left == 0 ||
             __w->_M_left->_M_color == _S_rb_tree_black)) {
          __w->_M_color = _S_rb_tree_red;
          __x = __x_parent;
          __x_parent = __x_parent->_M_parent;
        } else {
          if (__w->_M_left == 0 ||
              __w->_M_left->_M_color == _S_rb_tree_black) {
            if (__w->_M_right) __w->_M_right->_M_color = _S_rb_tree_black;
            __w->_M_color = _S_rb_tree_red;
            _Rotate_left(__w, __root);
            __w = __x_parent->_M_left;
          }
          __w->_M_color = __x_parent->_M_color;
          __x_parent->_M_color = _S_rb_tree_black;
          if (__w->_M_left) __w->_M_left->_M_color = _S_rb_tree_black;
          _Rotate_right(__x_parent, __root);
          break;
        }
      }
    if (__x) __x->_M_color = _S_rb_tree_black;
  }
  return __y;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_M_decrement(_Rb_tree_node_base* _M_node) {
  if (_M_node->_M_color == _S_rb_tree_red && _M_node->_M_parent->_M_parent == _M_node)
    _M_node = _M_node->_M_right;
  else if (_M_node->_M_left != 0) {
    _M_node = _Rb_tree_node_base::_S_maximum(_M_node->_M_left);
  }
  else {
    _Base_ptr __y = _M_node->_M_parent;
    while (_M_node == __y->_M_left) {
      _M_node = __y;
      __y = __y->_M_parent;
    }
    _M_node = __y;
  }
  return _M_node;
}

template <class _Dummy> _Rb_tree_node_base* _STLP_CALL
_Rb_global<_Dummy>::_M_increment(_Rb_tree_node_base* _M_node) {
  if (_M_node->_M_right != 0) {
    _M_node = _Rb_tree_node_base::_S_minimum(_M_node->_M_right);
  }
  else {
    _Base_ptr __y = _M_node->_M_parent;
    while (_M_node == __y->_M_right) {
      _M_node = __y;
      __y = __y->_M_parent;
    }
    // check special case: This is necessary if _M_node is the
    // _M_head and the tree contains only a single node __y. In
    // that case parent, left and right all point to __y!
    if (_M_node->_M_right != __y)
      _M_node = __y;
  }
  return _M_node;
}

#endif /* _STLP_EXPOSE_GLOBALS_IMPLEMENTATION */


template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>&
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::operator=(
  const _Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>& __x) {
  if (this != &__x) {
    // Note that _Key may be a constant type.
    clear();
    _M_node_count = 0;
    _M_key_compare = __x._M_key_compare;
    if (__x._M_root() == 0) {
      _M_root() = 0;
      _M_leftmost() = &this->_M_header._M_data;
      _M_rightmost() = &this->_M_header._M_data;
    }
    else {
      _M_root() = _M_copy(__x._M_root(), &this->_M_header._M_data);
      _M_leftmost() = _S_minimum(_M_root());
      _M_rightmost() = _S_maximum(_M_root());
      _M_node_count = __x._M_node_count;
    }
  }
  return *this;
}

// CRP 7/10/00 inserted argument __on_right, which is another hint (meant to
// act like __on_left and ignore a portion of the if conditions -- specify
// __on_right != 0 to bypass comparison as false or __on_left != 0 to bypass
// comparison as true)
template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::_M_insert(_Rb_tree_node_base * __parent,
                                                                      const _Value& __val,
                                                                      _Rb_tree_node_base * __on_left,
                                                                      _Rb_tree_node_base * __on_right) {
  // We do not create the node here as, depending on tests, we might call
  // _M_key_compare that can throw an exception.
  _Base_ptr __new_node;

  if ( __parent == &this->_M_header._M_data ) {
    __new_node = _M_create_node(__val);
    _S_left(__parent) = __new_node;   // also makes _M_leftmost() = __new_node
    _M_root() = __new_node;
    _M_rightmost() = __new_node;
  }
  else if ( __on_right == 0 &&     // If __on_right != 0, the remainder fails to false
           ( __on_left != 0 ||     // If __on_left != 0, the remainder succeeds to true
             _M_key_compare( _KeyOfValue()(__val), _S_key(__parent) ) ) ) {
    __new_node = _M_create_node(__val);
    _S_left(__parent) = __new_node;
    if (__parent == _M_leftmost())
      _M_leftmost() = __new_node;   // maintain _M_leftmost() pointing to min node
  }
  else {
    __new_node = _M_create_node(__val);
    _S_right(__parent) = __new_node;
    if (__parent == _M_rightmost())
      _M_rightmost() = __new_node;  // maintain _M_rightmost() pointing to max node
  }
  _S_parent(__new_node) = __parent;
  _Rb_global_inst::_Rebalance(__new_node, this->_M_header._M_data._M_parent);
  ++_M_node_count;
  return iterator(__new_node);
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_equal(const _Value& __val) {
  _Base_ptr __y = &this->_M_header._M_data;
  _Base_ptr __x = _M_root();
  while (__x != 0) {
    __y = __x;
    if (_M_key_compare(_KeyOfValue()(__val), _S_key(__x))) {
      __x = _S_left(__x);
    }
    else
      __x = _S_right(__x);
  }
  return _M_insert(__y, __val, __x);
}


template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
pair<__iterator__, bool>
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_unique(const _Value& __val) {
  _Base_ptr __y = &this->_M_header._M_data;
  _Base_ptr __x = _M_root();
  bool __comp = true;
  while (__x != 0) {
    __y = __x;
    __comp = _M_key_compare(_KeyOfValue()(__val), _S_key(__x));
    __x = __comp ? _S_left(__x) : _S_right(__x);
  }
  iterator __j = iterator(__y);
  if (__comp) {
    if (__j == begin())
      return pair<iterator,bool>(_M_insert(__y, __val, /* __x*/ __y), true);
    else
      --__j;
  }
  if (_M_key_compare(_S_key(__j._M_node), _KeyOfValue()(__val))) {
    return pair<iterator,bool>(_M_insert(__y, __val, __x), true);
  }
  return pair<iterator,bool>(__j, false);
}

// Modifications CRP 7/10/00 as noted to improve conformance and
// efficiency.
template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_unique(iterator __position,
                                                                          const _Value& __val) {
  if (__position._M_node == this->_M_header._M_data._M_left) { // begin()

    // if the container is empty, fall back on insert_unique.
    if (empty())
      return insert_unique(__val).first;

    if (_M_key_compare(_KeyOfValue()(__val), _S_key(__position._M_node))) {
      return _M_insert(__position._M_node, __val, __position._M_node);
    }
    // first argument just needs to be non-null
    else {
      bool __comp_pos_v = _M_key_compare( _S_key(__position._M_node), _KeyOfValue()(__val) );

      if (__comp_pos_v == false)  // compare > and compare < both false so compare equal
        return __position;
      //Below __comp_pos_v == true

      // Standard-conformance - does the insertion point fall immediately AFTER
      // the hint?
      iterator __after = __position;
      ++__after;

      // Check for only one member -- in that case, __position points to itself,
      // and attempting to increment will cause an infinite loop.
      if (__after._M_node == &this->_M_header._M_data)
        // Check guarantees exactly one member, so comparison was already
        // performed and we know the result; skip repeating it in _M_insert
        // by specifying a non-zero fourth argument.
        return _M_insert(__position._M_node, __val, 0, __position._M_node);

      // All other cases:

      // Optimization to catch insert-equivalent -- save comparison results,
      // and we get this for free.
      if (_M_key_compare( _KeyOfValue()(__val), _S_key(__after._M_node) )) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else {
        return insert_unique(__val).first;
      }
    }
  }
  else if (__position._M_node == &this->_M_header._M_data) { // end()
    if (_M_key_compare(_S_key(_M_rightmost()), _KeyOfValue()(__val))) {
        // pass along to _M_insert that it can skip comparing
        // v, Key ; since compare Key, v was true, compare v, Key must be false.
        return _M_insert(_M_rightmost(), __val, 0, __position._M_node); // Last argument only needs to be non-null
    }
    else
      return insert_unique(__val).first;
  }
  else {
    iterator __before = __position;
    --__before;

    bool __comp_v_pos = _M_key_compare(_KeyOfValue()(__val), _S_key(__position._M_node));

    if (__comp_v_pos
        && _M_key_compare( _S_key(__before._M_node), _KeyOfValue()(__val) )) {

      if (_S_right(__before._M_node) == 0)
        return _M_insert(__before._M_node, __val, 0, __before._M_node); // Last argument only needs to be non-null
      else
        return _M_insert(__position._M_node, __val, __position._M_node);
      // first argument just needs to be non-null
    }
    else {
      // Does the insertion point fall immediately AFTER the hint?
      iterator __after = __position;
      ++__after;
      // Optimization to catch equivalent cases and avoid unnecessary comparisons
      bool __comp_pos_v = !__comp_v_pos;  // Stored this result earlier
      // If the earlier comparison was true, this comparison doesn't need to be
      // performed because it must be false.  However, if the earlier comparison
      // was false, we need to perform this one because in the equal case, both will
      // be false.
      if (!__comp_v_pos) {
        __comp_pos_v = _M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val));
      }

      if ( (!__comp_v_pos) // comp_v_pos true implies comp_v_pos false
          && __comp_pos_v
          && (__after._M_node == &this->_M_header._M_data ||
              _M_key_compare( _KeyOfValue()(__val), _S_key(__after._M_node) ))) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      } else {
        // Test for equivalent case
        if (__comp_v_pos == __comp_pos_v)
          return __position;
        else
          return insert_unique(__val).first;
      }
    }
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
__iterator__
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::insert_equal(iterator __position,
                                                                         const _Value& __val) {
  if (__position._M_node == this->_M_header._M_data._M_left) { // begin()

    // Check for zero members
    if (size() <= 0)
        return insert_equal(__val);

    if (!_M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val)))
      return _M_insert(__position._M_node, __val, __position._M_node);
    else {
      // Check for only one member
      if (__position._M_node->_M_left == __position._M_node)
        // Unlike insert_unique, can't avoid doing a comparison here.
        return _M_insert(__position._M_node, __val);

      // All other cases:
      // Standard-conformance - does the insertion point fall immediately AFTER
      // the hint?
      iterator __after = __position;
      ++__after;

      // Already know that compare(pos, v) must be true!
      // Therefore, we want to know if compare(after, v) is false.
      // (i.e., we now pos < v, now we want to know if v <= after)
      // If not, invalid hint.
      if ( __after._M_node == &this->_M_header._M_data ||
           !_M_key_compare( _S_key(__after._M_node), _KeyOfValue()(__val) ) ) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else { // Invalid hint
        return insert_equal(__val);
      }
    }
  }
  else if (__position._M_node == &this->_M_header._M_data) { // end()
    if (!_M_key_compare(_KeyOfValue()(__val), _S_key(_M_rightmost())))
      return _M_insert(_M_rightmost(), __val, 0, __position._M_node); // Last argument only needs to be non-null
    else {
      return insert_equal(__val);
    }
  }
  else {
    iterator __before = __position;
    --__before;
    // store the result of the comparison between pos and v so
    // that we don't have to do it again later.  Note that this reverses the shortcut
    // on the if, possibly harming efficiency in comparisons; I think the harm will
    // be negligible, and to do what I want to do (save the result of a comparison so
    // that it can be re-used) there is no alternative.  Test here is for before <= v <= pos.
    bool __comp_pos_v = _M_key_compare(_S_key(__position._M_node), _KeyOfValue()(__val));
    if (!__comp_pos_v &&
        !_M_key_compare(_KeyOfValue()(__val), _S_key(__before._M_node))) {
      if (_S_right(__before._M_node) == 0)
        return _M_insert(__before._M_node, __val, 0, __before._M_node); // Last argument only needs to be non-null
      else
        return _M_insert(__position._M_node, __val, __position._M_node);
    }
    else {
      // Does the insertion point fall immediately AFTER the hint?
      // Test for pos < v <= after
      iterator __after = __position;
      ++__after;

      if (__comp_pos_v &&
          ( __after._M_node == &this->_M_header._M_data ||
            !_M_key_compare( _S_key(__after._M_node), _KeyOfValue()(__val) ) ) ) {
        if (_S_right(__position._M_node) == 0)
          return _M_insert(__position._M_node, __val, 0, __position._M_node);
        else
          return _M_insert(__after._M_node, __val, __after._M_node);
      }
      else { // Invalid hint
        return insert_equal(__val);
      }
    }
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
_Rb_tree_node_base*
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc> ::_M_copy(_Rb_tree_node_base* __x,
                                                                    _Rb_tree_node_base* __p) {
  // structural copy.  __x and __p must be non-null.
  _Base_ptr __top = _M_clone_node(__x);
  _S_parent(__top) = __p;

  _STLP_TRY {
    if (_S_right(__x))
      _S_right(__top) = _M_copy(_S_right(__x), __top);
    __p = __top;
    __x = _S_left(__x);

    while (__x != 0) {
      _Base_ptr __y = _M_clone_node(__x);
      _S_left(__p) = __y;
      _S_parent(__y) = __p;
      if (_S_right(__x))
        _S_right(__y) = _M_copy(_S_right(__x), __y);
      __p = __y;
      __x = _S_left(__x);
    }
  }
  _STLP_UNWIND(_M_erase(__top))

  return __top;
}

// this has to stay out-of-line : it's recursive
template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
void
_Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>::_M_erase(_Rb_tree_node_base *__x) {
  // erase without rebalancing
  while (__x != 0) {
    _M_erase(_S_right(__x));
    _Base_ptr __y = _S_left(__x);
    _STLP_STD::_Destroy(&_S_value(__x));
    this->_M_header.deallocate(__STATIC_CAST(_Link_type, __x),1);
    __x = __y;
  }
}

#if defined (_STLP_DEBUG)
inline int
__black_count(_Rb_tree_node_base* __node, _Rb_tree_node_base* __root) {
  if (__node == 0)
    return 0;
  else {
    int __bc = __node->_M_color == _S_rb_tree_black ? 1 : 0;
    if (__node == __root)
      return __bc;
    else
      return __bc + __black_count(__node->_M_parent, __root);
  }
}

template <class _Key, class _Compare,
          class _Value, class _KeyOfValue, class _Traits, class _Alloc>
bool _Rb_tree<_Key,_Compare,_Value,_KeyOfValue,_Traits,_Alloc>::__rb_verify() const {
  if (_M_node_count == 0 || begin() == end())
    return ((_M_node_count == 0) &&
            (begin() == end()) &&
            (this->_M_header._M_data._M_left == &this->_M_header._M_data) &&
            (this->_M_header._M_data._M_right == &this->_M_header._M_data));

  int __len = __black_count(_M_leftmost(), _M_root());
  for (const_iterator __it = begin(); __it != end(); ++__it) {
    _Base_ptr __x = __it._M_node;
    _Base_ptr __L = _S_left(__x);
    _Base_ptr __R = _S_right(__x);

    if (__x->_M_color == _S_rb_tree_red)
      if ((__L && __L->_M_color == _S_rb_tree_red) ||
          (__R && __R->_M_color == _S_rb_tree_red))
        return false;

    if (__L && _M_key_compare(_S_key(__x), _S_key(__L)))
      return false;
    if (__R && _M_key_compare(_S_key(__R), _S_key(__x)))
      return false;

    if (!__L && !__R && __black_count(__x, _M_root()) != __len)
      return false;
  }

  if (_M_leftmost() != _Rb_tree_node_base::_S_minimum(_M_root()))
    return false;
  if (_M_rightmost() != _Rb_tree_node_base::_S_maximum(_M_root()))
    return false;

  return true;
}
#endif /* _STLP_DEBUG */

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#undef _Rb_tree
#undef __iterator__
#undef iterator
#undef __size_type__

#endif /*  _STLP_TREE_C */

// Local Variables:
// mode:C++
// End:
