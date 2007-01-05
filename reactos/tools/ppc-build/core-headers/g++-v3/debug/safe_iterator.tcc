// Debugging iterator implementation (out of line) -*- C++ -*-

// Copyright (C) 2003, 2004
// Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.

/** @file safe_iterator.tcc
 *  This is an internal header file, included by other library headers.
 *  You should not attempt to use it directly.
 */

#ifndef _GLIBCXX_DEBUG_SAFE_ITERATOR_TCC
#define _GLIBCXX_DEBUG_SAFE_ITERATOR_TCC 1

namespace __gnu_debug
{
  template<typename _Iterator, typename _Sequence>
    bool
    _Safe_iterator<_Iterator, _Sequence>::
    _M_can_advance(const difference_type& __n) const
    {
      typedef typename _Sequence::const_iterator const_iterator;

      if (this->_M_singular())
	return false;
      if (__n == 0)
	return true;
      if (__n < 0)
	{
	  const_iterator __begin =
	    static_cast<const _Sequence*>(_M_sequence)->begin();
	  pair<difference_type, _Distance_precision> __dist =
	    this->_M_get_distance(__begin, *this);
	  bool __ok =  (__dist.second == __dp_exact && __dist.first >= -__n
			|| __dist.second != __dp_exact && __dist.first > 0);
	  return __ok;
	}
      else
	{
	  const_iterator __end =
	    static_cast<const _Sequence*>(_M_sequence)->end();
	  pair<difference_type, _Distance_precision> __dist =
	    this->_M_get_distance(*this, __end);
	  bool __ok = (__dist.second == __dp_exact && __dist.first >= __n
		       || __dist.second != __dp_exact && __dist.first > 0);
	  return __ok;
	}
    }

  template<typename _Iterator, typename _Sequence>
    template<typename _Other>
      bool
      _Safe_iterator<_Iterator, _Sequence>::
      _M_valid_range(const _Safe_iterator<_Other, _Sequence>& __rhs) const
      {
	if (!_M_can_compare(__rhs))
	  return false;

	/* Determine if we can order the iterators without the help of
	   the container */
	pair<difference_type, _Distance_precision> __dist =
	  this->_M_get_distance(*this, __rhs);
	switch (__dist.second) {
	case __dp_equality:
	  if (__dist.first == 0)
	    return true;
	  break;

	case __dp_sign:
	case __dp_exact:
	  return __dist.first >= 0;
	}

	/* We can only test for equality, but check if one of the
	   iterators is at an extreme. */
	if (_M_is_begin() || __rhs._M_is_end())
	  return true;
	else if (_M_is_end() || __rhs._M_is_begin())
	  return false;

	// Assume that this is a valid range; we can't check anything else
	return true;
      }

  template<typename _Iterator, typename _Sequence>
    void
    _Safe_iterator<_Iterator, _Sequence>::
    _M_invalidate()
    {
      typedef typename _Sequence::iterator iterator;
      typedef typename _Sequence::const_iterator const_iterator;

      if (!this->_M_singular())
	{
	  for (_Safe_iterator_base* iter = _M_sequence->_M_iterators; iter; )
	    {
	      iterator* __victim = static_cast<iterator*>(iter);
	      iter = iter->_M_next;
	      if (this->base() == __victim->base())
		__victim->_M_version = 0;
	    }
	  for (_Safe_iterator_base* iter2 = _M_sequence->_M_const_iterators;
	       iter2; /* increment in loop */)
	    {
	      const_iterator* __victim = static_cast<const_iterator*>(iter2);
	      iter2 = iter2->_M_next;
	      if (this->base() == __victim->base())
		__victim->_M_version = 0;
	    }
	  _M_version = 0;
	}
    }
} // namespace __gnu_debug

#endif

