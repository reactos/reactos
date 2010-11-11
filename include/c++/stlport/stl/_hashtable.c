/*
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
#ifndef _STLP_HASHTABLE_C
#define _STLP_HASHTABLE_C

#ifndef _STLP_INTERNAL_HASHTABLE_H
#  include <stl/_hashtable.h>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)

_STLP_MOVE_TO_PRIV_NAMESPACE

#  define __PRIME_LIST_BODY { \
  7ul,          23ul, \
  53ul,         97ul,         193ul,       389ul,       769ul,      \
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,    \
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,   \
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul, \
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,\
  1610612741ul, 3221225473ul, 4294967291ul  \
}

template <class _Dummy>
const size_t* _STLP_CALL
_Stl_prime<_Dummy>::_S_primes(size_t &__size) {
  static const size_t _list[] = __PRIME_LIST_BODY;
#  ifndef __MWERKS__
  __size =  sizeof(_list) / sizeof(_list[0]);
#  else
  __size =  30;
#  endif
  return _list;
}

template <class _Dummy>
size_t _STLP_CALL
_Stl_prime<_Dummy>::_S_max_nb_buckets() {
  size_t __size;
  const size_t* __first = _S_primes(__size);
  return *(__first + __size - 1);
}

template <class _Dummy>
size_t _STLP_CALL
_Stl_prime<_Dummy>::_S_next_size(size_t __n) {
  size_t __size;
  const size_t* __first = _S_primes(__size);
  const size_t* __last =  __first + __size;
  const size_t* pos = __lower_bound(__first, __last, __n, 
                                    __less((size_t*)0), __less((size_t*)0), (ptrdiff_t*)0);
  return (pos == __last ? *(__last - 1) : *pos);
}

template <class _Dummy>
void _STLP_CALL
_Stl_prime<_Dummy>::_S_prev_sizes(size_t __n, size_t const*&__begin, size_t const*&__pos) {
  size_t __size;
  __begin = _S_primes(__size);
  const size_t* __last =  __begin + __size;
  __pos = __lower_bound(__begin, __last, __n, 
                        __less((size_t*)0), __less((size_t*)0), (ptrdiff_t*)0);

  if (__pos== __last)
    --__pos;
  else if (*__pos == __n) {
    if (__pos != __begin)
      --__pos;
  }
}

#  undef __PRIME_LIST_BODY

_STLP_MOVE_TO_STD_NAMESPACE

#endif

#if defined (_STLP_DEBUG)
#  define hashtable _STLP_NON_DBG_NAME(hashtable)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

// fbp: these defines are for outline methods definitions.
// needed to definitions to be portable. Should not be used in method bodies.

#if defined ( _STLP_NESTED_TYPE_PARAM_BUG )
#  define __size_type__       size_t
#  define size_type           size_t
#  define value_type          _Val
#  define key_type            _Key
#  define __reference__       _Val&

#  define __iterator__        _Ht_iterator<_Val, _STLP_HEADER_TYPENAME _Traits::_NonConstTraits, \
                                           _Key, _HF, _ExK, _EqK, _All>
#  define __const_iterator__  _Ht_iterator<_Val, _STLP_HEADER_TYPENAME _Traits::_ConstTraits, \
                                           _Key, _HF, _ExK, _EqK, _All>
#else
#  define __size_type__       _STLP_TYPENAME_ON_RETURN_TYPE hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All>::size_type
#  define __reference__       _STLP_TYPENAME_ON_RETURN_TYPE  hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All>::reference
#  define __iterator__        _STLP_TYPENAME_ON_RETURN_TYPE hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All>::iterator
#  define __const_iterator__  _STLP_TYPENAME_ON_RETURN_TYPE hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All>::const_iterator
#endif

/*
 * This method is too difficult to implement for hashtable that do not
 * require a sorted operation on the stored type.
template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
bool hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>::_M_equal(
              const hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>& __ht1,
              const hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>& __ht2) {
  return __ht1._M_buckets == __ht2._M_buckets &&
         __ht1._M_elems == __ht2._M_elems;
}
*/

/* Returns the iterator before the first iterator of the bucket __n and set
 * __n to the first previous bucket having the same first iterator as bucket
 * __n.
 */
template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__iterator__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_before_begin(size_type &__n) const {
  return _S_before_begin(_M_elems, _M_buckets, __n);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__iterator__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_S_before_begin(const _ElemsCont& __elems, const _BucketVector& __buckets,
                    size_type &__n) {
  _ElemsCont &__mutable_elems = __CONST_CAST(_ElemsCont&, __elems);
  typename _BucketVector::const_iterator __bpos(__buckets.begin() + __n);

  _ElemsIte __pos(*__bpos);
  if (__pos == __mutable_elems.begin()) {
    __n = 0;
    return __mutable_elems.before_begin();
  }

  typename _BucketVector::const_iterator __bcur(__bpos);
  _BucketType *__pos_node = __pos._M_node;
  for (--__bcur; __pos_node == *__bcur; --__bcur);

  __n = __bcur - __buckets.begin() + 1;
  _ElemsIte __cur(*__bcur);
  _ElemsIte __prev = __cur++;
  for (; __cur != __pos; ++__prev, ++__cur);
  return __prev;
}


template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__iterator__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_insert_noresize(size_type __n, const value_type& __obj) {
  //We always insert this element as 1st in the bucket to not break
  //the elements order as equal elements must be kept next to each other.
  size_type __prev = __n;
  _ElemsIte __pos = _M_before_begin(__prev)._M_ite;

  fill(_M_buckets.begin() + __prev, _M_buckets.begin() + __n + 1,
       _M_elems.insert_after(__pos, __obj)._M_node);
  ++_M_num_elements;
  return iterator(_ElemsIte(_M_buckets[__n]));
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
pair<__iterator__, bool>
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::insert_unique_noresize(const value_type& __obj) {
  const size_type __n = _M_bkt_num(__obj);
  _ElemsIte __cur(_M_buckets[__n]);
  _ElemsIte __last(_M_buckets[__n + 1]);

  if (__cur != __last) {
    for (; __cur != __last; ++__cur) {
      if (_M_equals(_M_get_key(*__cur), _M_get_key(__obj))) {
        //We check that equivalent keys have equals hash code as otherwise, on resize,
        //equivalent value might not be in the same bucket
        _STLP_ASSERT(_M_hash(_M_get_key(*__cur)) == _M_hash(_M_get_key(__obj)))
        return pair<iterator, bool>(iterator(__cur), false);
      }
    }
    /* Here we do not rely on the _M_insert_noresize method as we know
     * that we cannot break element orders, elements are unique, and
     * insertion after the first bucket element is faster than what is
     * done in _M_insert_noresize.
     */
    __cur = _M_elems.insert_after(_ElemsIte(_M_buckets[__n]), __obj);
    ++_M_num_elements;
    return pair<iterator, bool>(iterator(__cur), true);
  }

  return pair<iterator, bool>(_M_insert_noresize(__n, __obj), true);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__iterator__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::insert_equal_noresize(const value_type& __obj) {
  const size_type __n = _M_bkt_num(__obj);
  {
    _ElemsIte __cur(_M_buckets[__n]);
    _ElemsIte __last(_M_buckets[__n + 1]);

    for (; __cur != __last; ++__cur) {
      if (_M_equals(_M_get_key(*__cur), _M_get_key(__obj))) {
        //We check that equivalent keys have equals hash code as otherwise, on resize,
        //equivalent value might not be in the same bucket
        _STLP_ASSERT(_M_hash(_M_get_key(*__cur)) == _M_hash(_M_get_key(__obj)))
        ++_M_num_elements;
        return _M_elems.insert_after(__cur, __obj);
      }
    }
  }

  return _M_insert_noresize(__n, __obj);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__reference__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_insert(const value_type& __obj) {
  _M_enlarge(_M_num_elements + 1);
  return *insert_unique_noresize(__obj).first;
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
__size_type__
hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::erase(const key_type& __key) {
  const size_type __n = _M_bkt_num_key(__key);

  _ElemsIte __cur(_M_buckets[__n]);
  _ElemsIte __last(_M_buckets[__n + 1]);
  if (__cur == __last)
    return 0;

  size_type __erased = 0;
  if (_M_equals(_M_get_key(*__cur), __key)) {
    //We look for the pos before __cur:
    size_type __prev_b = __n;
    _ElemsIte __prev = _M_before_begin(__prev_b)._M_ite;
    do {
      __cur = _M_elems.erase_after(__prev);
      ++__erased;
    } while ((__cur != __last) && _M_equals(_M_get_key(*__cur), __key));
    fill(_M_buckets.begin() + __prev_b, _M_buckets.begin() + __n + 1, __cur._M_node);
  }
  else {
    _ElemsIte __prev = __cur++;
    for (; __cur != __last; ++__prev, ++__cur) {
      if (_M_equals(_M_get_key(*__cur), __key)) {
        do {
          __cur = _M_elems.erase_after(__prev);
          ++__erased;
        } while ((__cur != __last) && _M_equals(_M_get_key(*__cur), __key));
        break;
      }
    }
  }

  _M_num_elements -= __erased;
  _M_reduce();
  return __erased;
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::erase(const_iterator __it) {
  const size_type __n = _M_bkt_num(*__it);
  _ElemsIte __cur(_M_buckets[__n]);

  size_type __erased = 0;
  if (__cur == __it._M_ite) {
    size_type __prev_b = __n;
    _ElemsIte __prev = _M_before_begin(__prev_b)._M_ite;
    fill(_M_buckets.begin() + __prev_b, _M_buckets.begin() + __n + 1,
         _M_elems.erase_after(__prev)._M_node);
    ++__erased;
  }
  else {
    _ElemsIte __prev = __cur++;
    _ElemsIte __last(_M_buckets[__n + 1]);
    for (; __cur != __last; ++__prev, ++__cur) {
      if (__cur == __it._M_ite) {
        _M_elems.erase_after(__prev);
        ++__erased;
        break;
      }
    }
  }

  _M_num_elements -= __erased;
  _M_reduce();
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::erase(const_iterator __first, const_iterator __last) {
  if (__first == __last)
    return;
  size_type __f_bucket = _M_bkt_num(*__first);
  size_type __l_bucket = __last != end() ? _M_bkt_num(*__last) : (_M_buckets.size() - 1);

  _ElemsIte __cur(_M_buckets[__f_bucket]);
  _ElemsIte __prev;
  if (__cur == __first._M_ite) {
    __prev = _M_before_begin(__f_bucket)._M_ite;
  }
  else {
    _ElemsIte __last(_M_buckets[++__f_bucket]);
    __prev = __cur++;
    for (; (__cur != __last) && (__cur != __first._M_ite); ++__prev, ++__cur);
  }
  size_type __erased = 0;
  //We do not use the slist::erase_after method taking a range to count the
  //number of erased elements:
  while (__cur != __last._M_ite) {
    __cur = _M_elems.erase_after(__prev);
    ++__erased;
  }
  fill(_M_buckets.begin() + __f_bucket, _M_buckets.begin() + __l_bucket + 1, __cur._M_node);
  _M_num_elements -= __erased;
  _M_reduce();
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::rehash(size_type __num_buckets_hint) {
  if (bucket_count() >= __num_buckets_hint) {
    // We are trying to reduce number of buckets, we have to validate it:
    size_type __limit_num_buckets = (size_type)((float)size() / max_load_factor());
    if (__num_buckets_hint < __limit_num_buckets) {
      // Targetted number of buckets __num_buckets_hint would break
      // load_factor() <= max_load_factor() rule.
      return;
    }
  }

  _M_rehash(__num_buckets_hint);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_enlarge(size_type __to_size) {
  size_type __num_buckets = bucket_count();
  size_type __num_buckets_hint = (size_type)((float)__to_size / max_load_factor());
  if (__num_buckets_hint <= __num_buckets) {
    return;
  }
  __num_buckets = _STLP_PRIV _Stl_prime_type::_S_next_size(__num_buckets_hint);

  _M_rehash(__num_buckets);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_reduce() {
  size_type __num_buckets = bucket_count();
  // We only try to reduce the hashtable if the theorical load factor
  // is lower than a fraction of the max load factor:
  // 4 factor is coming from the fact that prime number list is almost a
  // geometrical suite with reason 2, as we try to jump 2 levels is means
  // a 4 factor.
  if ((float)size() / (float)__num_buckets > max_load_factor() / 4.0f)
    return;

  const size_type *__first;
  const size_type *__prev;
  _STLP_PRIV _Stl_prime_type::_S_prev_sizes(__num_buckets, __first, __prev);

  /* We are only going to reduce number of buckets if moving to yet the previous number
   * of buckets in the prime numbers would respect the load rule. Otherwise algorithm
   * successively removing and adding an element would each time perform an expensive
   * rehash operation. */
  const size_type *__prev_prev = __prev;
  if (__prev_prev != __first) {
    --__prev_prev;
    if ((float)size() / (float)*__prev_prev > max_load_factor())
      return;
  }
  else {
    if (*__prev >= __num_buckets)
      return;
  }

  // Can we reduce further:
  while (__prev_prev != __first) {
    --__prev_prev;
    if ((float)size() / (float)*__prev_prev > max_load_factor())
      // We cannot reduce further.
      break;
    --__prev;
  }

  _M_rehash(*__prev);
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_resize() {
  if (load_factor() > max_load_factor()) {
    // We have to enlarge
    _M_enlarge(size());
  }
  else {
    // We can try to reduce size:
    _M_reduce();
  }
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_rehash(size_type __num_buckets) {
#if defined (_STLP_DEBUG)
  _M_check();
#endif
  _ElemsCont __tmp_elems(_M_elems.get_allocator());
  _BucketVector __tmp(__num_buckets + 1, __STATIC_CAST(_BucketType*, 0), _M_buckets.get_allocator());
  _ElemsIte __cur, __last(_M_elems.end());
  while (!_M_elems.empty()) {
    __cur = _M_elems.begin();
    size_type __new_bucket = _M_bkt_num(*__cur, __num_buckets);
    _ElemsIte __ite(__cur), __before_ite(__cur);
    for (++__ite;
         __ite != __last && _M_equals(_M_get_key(*__cur), _M_get_key(*__ite));
         ++__ite, ++__before_ite);
    size_type __prev_bucket = __new_bucket;
    _ElemsIte  __prev = _S_before_begin(__tmp_elems, __tmp, __prev_bucket)._M_ite;
    __tmp_elems.splice_after(__prev, _M_elems, _M_elems.before_begin(), __before_ite);
    fill(__tmp.begin() + __prev_bucket, __tmp.begin() + __new_bucket + 1, __cur._M_node);
  }
  _M_elems.swap(__tmp_elems);
  _M_buckets.swap(__tmp);
}

#if defined (_STLP_DEBUG)
template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>::_M_check() const {
  //We check that hash code of stored keys haven't change and also that equivalent
  //relation hasn't been modified
  size_t __num_buckets = bucket_count();
  for (size_t __b = 0; __b < __num_buckets; ++__b) {
    _ElemsIte __cur(_M_buckets[__b]), __last(_M_buckets[__b + 1]);
    _ElemsIte __fst(__cur), __snd(__cur);
    for (; __cur != __last; ++__cur) {
      _STLP_ASSERT( _M_bkt_num(*__cur, __num_buckets) == __b )
      _STLP_ASSERT( !_M_equals(_M_get_key(*__fst), _M_get_key(*__cur)) || _M_equals(_M_get_key(*__snd), _M_get_key(*__cur)) )
      if (__fst != __snd)
        ++__fst;
      if (__snd != __cur)
        ++__snd;
    }
  }
}
#endif

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>::clear() {
  _M_elems.clear();
  _M_buckets.assign(_M_buckets.size(), __STATIC_CAST(_BucketType*, 0));
  _M_num_elements = 0;
}

template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
void hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
  ::_M_copy_from(const hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>& __ht) {
  _M_elems.clear();
  _M_elems.insert(_M_elems.end(), __ht._M_elems.begin(), __ht._M_elems.end());
  _M_buckets.resize(__ht._M_buckets.size());
  _ElemsConstIte __src(__ht._M_elems.begin()), __src_end(__ht._M_elems.end());
  _ElemsIte __dst(_M_elems.begin());
  typename _BucketVector::const_iterator __src_b(__ht._M_buckets.begin()),
                                         __src_end_b(__ht._M_buckets.end());
  typename _BucketVector::iterator __dst_b(_M_buckets.begin()), __dst_end_b(_M_buckets.end());
  for (; __src != __src_end; ++__src, ++__dst) {
    for (; __src_b != __src_end_b; ++__src_b, ++__dst_b) {
      if (*__src_b == __src._M_node) {
        *__dst_b = __dst._M_node;
      }
      else
        break;
    }
  }
  fill(__dst_b, __dst_end_b, __STATIC_CAST(_BucketType*, 0));
  _M_num_elements = __ht._M_num_elements;
  _M_max_load_factor = __ht._M_max_load_factor;
}

#undef __iterator__
#undef const_iterator
#undef __size_type__
#undef __reference__
#undef size_type
#undef value_type
#undef key_type
#undef __stl_num_primes

#if defined (_STLP_DEBUG)
#  undef hashtable
_STLP_MOVE_TO_STD_NAMESPACE
#endif

_STLP_END_NAMESPACE

#endif /*  _STLP_HASHTABLE_C */

// Local Variables:
// mode:C++
// End:
