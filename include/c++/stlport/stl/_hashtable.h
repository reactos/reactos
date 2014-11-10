/*
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

/* NOTE: This is an internal header file, included by other STL headers.
 *   You should not attempt to use it directly.
 */

#ifndef _STLP_INTERNAL_HASHTABLE_H
#define _STLP_INTERNAL_HASHTABLE_H

#ifndef _STLP_INTERNAL_VECTOR_H
#  include <stl/_vector.h>
#endif

#ifndef _STLP_INTERNAL_SLIST_H
#  include <stl/_slist.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_H
#  include <stl/_iterator.h>
#endif

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#  include <stl/_function_base.h>
#endif

#ifndef _STLP_INTERNAL_ALGOBASE_H
#  include <stl/_algobase.h>
#endif

#ifndef _STLP_HASH_FUN_H
#  include <stl/_hash_fun.h>
#endif

/*
 * Hashtable class, used to implement the hashed associative containers
 * hash_set, hash_map, hash_multiset, hash_multimap,
 * unordered_set, unordered_map, unordered_multiset, unordered_multimap.
 */

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_TEMPLATE_EXPORT)
//Export of the classes used to represent buckets in the hashtable implementation.
#  if !defined (_STLP_USE_PTR_SPECIALIZATIONS)
//If pointer specialization is enabled vector<_Slist_node_base*> will use the void*
//storage type for which internal classes have already been exported.
_STLP_EXPORT_TEMPLATE_CLASS allocator<_STLP_PRIV _Slist_node_base*>;

_STLP_MOVE_TO_PRIV_NAMESPACE
_STLP_EXPORT_TEMPLATE_CLASS _STLP_alloc_proxy<_Slist_node_base**, _Slist_node_base*,
                                              allocator<_Slist_node_base*> >;
_STLP_EXPORT_TEMPLATE_CLASS _Vector_base<_Slist_node_base*,
                                         allocator<_Slist_node_base*> >;
_STLP_MOVE_TO_STD_NAMESPACE
#  endif

#  if defined (_STLP_DEBUG)
_STLP_MOVE_TO_PRIV_NAMESPACE
#    define _STLP_NON_DBG_VECTOR _STLP_NON_DBG_NAME(vector)
_STLP_EXPORT_TEMPLATE_CLASS __construct_checker<_STLP_NON_DBG_VECTOR<_Slist_node_base*, allocator<_Slist_node_base*> > >;
_STLP_EXPORT_TEMPLATE_CLASS _STLP_NON_DBG_VECTOR<_Slist_node_base*, allocator<_Slist_node_base*> >;
#    undef _STLP_NON_DBG_VECTOR
_STLP_MOVE_TO_STD_NAMESPACE
#  endif

_STLP_EXPORT_TEMPLATE_CLASS vector<_STLP_PRIV _Slist_node_base*,
                                   allocator<_STLP_PRIV _Slist_node_base*> >;
#endif

#if defined (_STLP_DEBUG)
#  define hashtable _STLP_NON_DBG_NAME(hashtable)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

// some compilers require the names of template parameters to be the same
template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
class hashtable;

#if !defined (_STLP_DEBUG)
_STLP_MOVE_TO_PRIV_NAMESPACE
#endif

template <class _BaseIte, class _Traits>
struct _Ht_iterator {
  typedef typename _Traits::_ConstTraits _ConstTraits;
  typedef typename _Traits::_NonConstTraits _NonConstTraits;

  typedef _Ht_iterator<_BaseIte,_Traits> _Self;

  typedef typename _Traits::value_type value_type;
  typedef typename _Traits::pointer pointer;
  typedef typename _Traits::reference reference;
  typedef forward_iterator_tag iterator_category;
  typedef ptrdiff_t difference_type;
  typedef size_t size_type;

  typedef _Ht_iterator<_BaseIte, _NonConstTraits> iterator;
  typedef _Ht_iterator<_BaseIte, _ConstTraits> const_iterator;

  _Ht_iterator() {}
  //copy constructor for iterator and constructor from iterator for const_iterator
  _Ht_iterator(const iterator& __it) : _M_ite(__it._M_ite) {}
  _Ht_iterator(_BaseIte __it) : _M_ite(__it) {}

  reference operator*() const {
    return *_M_ite;
  }
  _STLP_DEFINE_ARROW_OPERATOR

  _Self& operator++() {
    ++_M_ite;
    return *this;
  }
  _Self operator++(int) {
    _Self __tmp = *this;
    ++*this;
    return __tmp;
  }

  bool operator == (const_iterator __rhs) const {
    return _M_ite == __rhs._M_ite;
  }
  bool operator != (const_iterator __rhs) const {
    return _M_ite != __rhs._M_ite;
  }

  _BaseIte _M_ite;
};

_STLP_MOVE_TO_STD_NAMESPACE

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _BaseIte, class _Traits>
struct __type_traits<_STLP_PRIV _Ht_iterator<_BaseIte, _Traits> > {
  typedef __false_type   has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __false_type   is_POD_type;
};
#endif /* _STLP_CLASS_PARTIAL_SPECIALIZATION */

#if defined (_STLP_USE_OLD_HP_ITERATOR_QUERIES)
template <class _BaseIte, class _Traits>
inline
#  if defined (_STLP_NESTED_TYPE_PARAM_BUG)
_STLP_TYPENAME_ON_RETURN_TYPE _Traits::value_type *
#  else
_STLP_TYPENAME_ON_RETURN_TYPE _STLP_PRIV _Ht_iterator<_BaseIte,_Traits>::value_type *
#  endif
value_type(const _STLP_PRIV _Ht_iterator<_BaseIte,_Traits>&) {
  typedef _STLP_TYPENAME _STLP_PRIV _Ht_iterator<_BaseIte,_Traits>::value_type _Val;
  return (_Val*) 0;
}
template <class _BaseIte, class _Traits>
inline forward_iterator_tag iterator_category(const _STLP_PRIV _Ht_iterator<_BaseIte,_Traits>&)
{ return forward_iterator_tag(); }
template <class _BaseIte, class _Traits>
inline ptrdiff_t* distance_type(const _STLP_PRIV _Ht_iterator<_BaseIte,_Traits>&)
{ return (ptrdiff_t*) 0; }
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Dummy>
class _Stl_prime {
  // Returns begining of primes list and size by reference.
  static const size_t* _S_primes(size_t&);
public:
  //Returns the maximum number of buckets handled by the hashtable implementation
  static size_t _STLP_CALL _S_max_nb_buckets();

  //Returns the bucket size next to a required size
  static size_t _STLP_CALL _S_next_size(size_t);

  // Returns the bucket range containing sorted list of prime numbers <= __hint.
  static void _STLP_CALL _S_prev_sizes(size_t __hint, const size_t *&__begin, const size_t *&__end);
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS _Stl_prime<bool>;
#endif

typedef _Stl_prime<bool> _Stl_prime_type;

#if !defined (_STLP_DEBUG)
_STLP_MOVE_TO_STD_NAMESPACE
#endif

/*
 * Hashtables handle allocators a bit differently than other containers
 * do. If we're using standard-conforming allocators, then a hashtable
 * unconditionally has a member variable to hold its allocator, even if
 * it so happens that all instances of the allocator type are identical.
 * This is because, for hashtables, this extra storage is negligible.
 * Additionally, a base class wouldn't serve any other purposes; it
 * wouldn't, for example, simplify the exception-handling code.
 */
template <class _Val, class _Key, class _HF,
          class _Traits, class _ExK, class _EqK, class _All>
class hashtable {
  typedef hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All> _Self;
  typedef typename _Traits::_NonConstTraits _NonConstTraits;
  typedef typename _Traits::_ConstTraits _ConstTraits;
  typedef typename _Traits::_NonConstLocalTraits _NonConstLocalTraits;
  typedef typename _Traits::_ConstLocalTraits _ConstLocalTraits;

public:
  typedef _Key key_type;
  typedef _Val value_type;
  typedef _HF hasher;
  typedef _EqK key_equal;

  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef typename _NonConstTraits::pointer pointer;
  typedef const value_type* const_pointer;
  typedef typename _NonConstTraits::reference reference;
  typedef const value_type& const_reference;
  typedef forward_iterator_tag _Iterator_category;

  hasher hash_funct() const { return _M_hash; }
  key_equal key_eq() const { return _M_equals; }

private:
  _STLP_FORCE_ALLOCATORS(_Val, _All)
#if defined (_STLP_DEBUG)
  typedef _STLP_PRIV _STLP_NON_DBG_NAME(slist)<value_type, _All> _ElemsCont;
#else
  typedef slist<value_type, _All> _ElemsCont;
#endif
  typedef typename _ElemsCont::iterator _ElemsIte;
  typedef typename _ElemsCont::const_iterator _ElemsConstIte;
  typedef _STLP_PRIV _Slist_node_base _BucketType;
  typedef typename _Alloc_traits<_BucketType*, _All>::allocator_type _BucketAllocType;
  /*
   * We are going to use vector of _Slist_node_base pointers for 2 reasons:
   *  - limit code bloat, all hashtable instanciation use the same buckets representation.
   *  - avoid _STLP_DEBUG performance trouble: with a vector of iterator on slist the resize
   *    method would be too slow because the slist::splice_after method become linear on
   *    the number of iterators in the buckets rather than constant in time as the iterator
   *    has to be move from a slist to the other.
   */
#if defined (_STLP_DEBUG)
  typedef _STLP_PRIV _STLP_NON_DBG_NAME(vector)<_BucketType*, _BucketAllocType> _BucketVector;
#else
  typedef vector<_BucketType*, _BucketAllocType> _BucketVector;
#endif

  hasher                _M_hash;
  key_equal             _M_equals;
  _ElemsCont            _M_elems;
  _BucketVector         _M_buckets;
  size_type             _M_num_elements;
  float                 _M_max_load_factor;
  _STLP_KEY_TYPE_FOR_CONT_EXT(key_type)

  static const key_type& _M_get_key(const value_type& __val) {
    _ExK k;
    return k(__val);
  }
public:
  typedef _STLP_PRIV _Ht_iterator<_ElemsIte, _NonConstTraits> iterator;
  typedef _STLP_PRIV _Ht_iterator<_ElemsIte, _ConstTraits> const_iterator;
  //TODO: Avoids this debug check and make the local_iterator different from
  //iterator in debug mode too.
#if !defined (_STLP_DEBUG)
  typedef _STLP_PRIV _Ht_iterator<_ElemsIte, _NonConstLocalTraits> local_iterator;
  typedef _STLP_PRIV _Ht_iterator<_ElemsIte, _ConstLocalTraits> const_local_iterator;
#else
  typedef iterator       local_iterator;
  typedef const_iterator const_local_iterator;
#endif

  typedef _All allocator_type;
  allocator_type get_allocator() const { return _M_elems.get_allocator(); }

#if !defined (_STLP_DONT_SUP_DFLT_PARAM)
  hashtable(size_type __n,
            const _HF&    __hf,
            const _EqK&   __eql,
            const allocator_type& __a = allocator_type())
#else
  hashtable(size_type __n,
            const _HF&    __hf,
            const _EqK&   __eql)
    : _M_hash(__hf),
      _M_equals(__eql),
      _M_elems(allocator_type()),
      _M_buckets(_STLP_CONVERT_ALLOCATOR(__a, _BucketType*)),
      _M_num_elements(0),
      _M_max_load_factor(1.0f)
  { _M_initialize_buckets(__n); }

  hashtable(size_type __n,
            const _HF&    __hf,
            const _EqK&   __eql,
            const allocator_type& __a)
#endif
    : _M_hash(__hf),
      _M_equals(__eql),
      _M_elems(__a),
      _M_buckets(_STLP_CONVERT_ALLOCATOR(__a, _BucketType*)),
      _M_num_elements(0),
      _M_max_load_factor(1.0f)
  { _M_initialize_buckets(__n); }

  hashtable(const _Self& __ht)
    : _M_hash(__ht._M_hash),
      _M_equals(__ht._M_equals),
      _M_elems(__ht.get_allocator()),
      _M_buckets(_STLP_CONVERT_ALLOCATOR(__ht.get_allocator(), _BucketType*)),
      _M_num_elements(0),
      _M_max_load_factor(1.0f)
  { _M_copy_from(__ht); }

#if !defined (_STLP_NO_MOVE_SEMANTIC)
  hashtable(__move_source<_Self> src)
    : _M_hash(_STLP_PRIV _AsMoveSource(src.get()._M_hash)),
      _M_equals(_STLP_PRIV _AsMoveSource(src.get()._M_equals)),
      _M_elems(__move_source<_ElemsCont>(src.get()._M_elems)),
      _M_buckets(__move_source<_BucketVector>(src.get()._M_buckets)),
      _M_num_elements(src.get()._M_num_elements),
      _M_max_load_factor(src.get()._M_max_load_factor) {}
#endif

  _Self& operator= (const _Self& __ht) {
    if (&__ht != this) {
      clear();
      _M_hash = __ht._M_hash;
      _M_equals = __ht._M_equals;
      _M_copy_from(__ht);
    }
    return *this;
  }

  ~hashtable() { clear(); }

  size_type size() const { return _M_num_elements; }
  size_type max_size() const { return size_type(-1); }
  bool empty() const { return size() == 0; }

  void swap(_Self& __ht) {
    _STLP_STD::swap(_M_hash, __ht._M_hash);
    _STLP_STD::swap(_M_equals, __ht._M_equals);
    _M_elems.swap(__ht._M_elems);
    _M_buckets.swap(__ht._M_buckets);
    _STLP_STD::swap(_M_num_elements, __ht._M_num_elements);
    _STLP_STD::swap(_M_max_load_factor, __ht._M_max_load_factor);
  }

  iterator begin() { return _M_elems.begin(); }
  iterator end() { return _M_elems.end(); }
  local_iterator begin(size_type __n) { return _ElemsIte(_M_buckets[__n]); }
  local_iterator end(size_type __n) { return _ElemsIte(_M_buckets[__n + 1]); }

  const_iterator begin() const { return __CONST_CAST(_ElemsCont&, _M_elems).begin(); }
  const_iterator end() const { return __CONST_CAST(_ElemsCont&, _M_elems).end(); }
  const_local_iterator begin(size_type __n) const { return _ElemsIte(_M_buckets[__n]); }
  const_local_iterator end(size_type __n) const { return _ElemsIte(_M_buckets[__n + 1]); }

  //static bool _STLP_CALL _M_equal (const _Self&, const _Self&);

public:
  //The number of buckets is size() - 1 because the last bucket always contains
  //_M_elems.end() to make algo easier to implement.
  size_type bucket_count() const { return _M_buckets.size() - 1; }
  size_type max_bucket_count() const { return _STLP_PRIV _Stl_prime_type::_S_max_nb_buckets(); }
  size_type elems_in_bucket(size_type __bucket) const
  { return _STLP_STD::distance(_ElemsIte(_M_buckets[__bucket]), _ElemsIte(_M_buckets[__bucket + 1])); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type bucket(const _KT& __k) const { return _M_bkt_num_key(__k); }

  // hash policy
  float load_factor() const { return (float)size() / (float)bucket_count(); }
  float max_load_factor() const { return _M_max_load_factor; }
  void max_load_factor(float __z) {
    _M_max_load_factor = __z;
    _M_resize();
  }

  pair<iterator, bool> insert_unique(const value_type& __obj) {
    _M_enlarge(_M_num_elements + 1);
    return insert_unique_noresize(__obj);
  }

  iterator insert_equal(const value_type& __obj) {
    _M_enlarge(_M_num_elements + 1);
    return insert_equal_noresize(__obj);
  }

protected:
  iterator _M_insert_noresize(size_type __n, const value_type& __obj);
public:
  pair<iterator, bool> insert_unique_noresize(const value_type& __obj);
  iterator insert_equal_noresize(const value_type& __obj);

#if defined (_STLP_MEMBER_TEMPLATES)
  template <class _InputIterator>
  void insert_unique(_InputIterator __f, _InputIterator __l)
  { insert_unique(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIterator)); }

  template <class _InputIterator>
  void insert_equal(_InputIterator __f, _InputIterator __l)
  { insert_equal(__f, __l, _STLP_ITERATOR_CATEGORY(__f, _InputIterator)); }

  template <class _InputIterator>
  void insert_unique(_InputIterator __f, _InputIterator __l,
                     const input_iterator_tag &) {
    for ( ; __f != __l; ++__f)
      insert_unique(*__f);
  }

  template <class _InputIterator>
  void insert_equal(_InputIterator __f, _InputIterator __l,
                    const input_iterator_tag &) {
    for ( ; __f != __l; ++__f)
      insert_equal(*__f);
  }

  template <class _ForwardIterator>
  void insert_unique(_ForwardIterator __f, _ForwardIterator __l,
                     const forward_iterator_tag &) {
    size_type __n = _STLP_STD::distance(__f, __l);
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_unique_noresize(*__f);
  }

  template <class _ForwardIterator>
  void insert_equal(_ForwardIterator __f, _ForwardIterator __l,
                    const forward_iterator_tag &) {
    size_type __n = _STLP_STD::distance(__f, __l);
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_equal_noresize(*__f);
  }

#else /* _STLP_MEMBER_TEMPLATES */
  void insert_unique(const value_type* __f, const value_type* __l) {
    size_type __n = __l - __f;
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_unique_noresize(*__f);
  }

  void insert_equal(const value_type* __f, const value_type* __l) {
    size_type __n = __l - __f;
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_equal_noresize(*__f);
  }

  void insert_unique(const_iterator __f, const_iterator __l) {
    size_type __n = _STLP_STD::distance(__f, __l);
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_unique_noresize(*__f);
  }

  void insert_equal(const_iterator __f, const_iterator __l) {
    size_type __n = _STLP_STD::distance(__f, __l);
    _M_enlarge(_M_num_elements + __n);
    for ( ; __n > 0; --__n, ++__f)
      insert_equal_noresize(*__f);
  }
#endif /*_STLP_MEMBER_TEMPLATES */

  //reference find_or_insert(const value_type& __obj);

private:
  _STLP_TEMPLATE_FOR_CONT_EXT
  _ElemsIte _M_find(const _KT& __key) const {
    size_type __n = _M_bkt_num_key(__key);
    _ElemsIte __first(_M_buckets[__n]);
    _ElemsIte __last(_M_buckets[__n + 1]);
    for ( ; (__first != __last) && !_M_equals(_M_get_key(*__first), __key); ++__first);
    if (__first != __last)
      return __first;
    else
      return __CONST_CAST(_ElemsCont&, _M_elems).end();
  }

public:
  _STLP_TEMPLATE_FOR_CONT_EXT
  iterator find(const _KT& __key) { return _M_find(__key); }
  _STLP_TEMPLATE_FOR_CONT_EXT
  const_iterator find(const _KT& __key) const { return _M_find(__key); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type count(const _KT& __key) const {
    const size_type __n = _M_bkt_num_key(__key);

    _ElemsIte __cur(_M_buckets[__n]);
    _ElemsIte __last(_M_buckets[__n + 1]);
    for (; __cur != __last; ++__cur) {
      if (_M_equals(_M_get_key(*__cur), __key)) {
        size_type __result = 1;
        for (++__cur;
             __cur != __last && _M_equals(_M_get_key(*__cur), __key);
             ++__result, ++__cur);
        return __result;
      }
    }
    return 0;
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<iterator, iterator> equal_range(const _KT& __key) {
    typedef pair<iterator, iterator> _Pii;
    const size_type __n = _M_bkt_num_key(__key);

    for (_ElemsIte __first(_M_buckets[__n]), __last(_M_buckets[__n + 1]);
         __first != __last; ++__first) {
      if (_M_equals(_M_get_key(*__first), __key)) {
        _ElemsIte __cur(__first);
        for (++__cur; (__cur != __last) && _M_equals(_M_get_key(*__cur), __key); ++__cur);
        return _Pii(__first, __cur);
      }
    }
    return _Pii(end(), end());
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  pair<const_iterator, const_iterator> equal_range(const _KT& __key) const {
    typedef pair<const_iterator, const_iterator> _Pii;
    const size_type __n = _M_bkt_num_key(__key);

    for (_ElemsIte __first(_M_buckets[__n]), __last(_M_buckets[__n + 1]);
         __first != __last; ++__first) {
      if (_M_equals(_M_get_key(*__first), __key)) {
        _ElemsIte __cur(__first);
        for (++__cur; (__cur != __last) && _M_equals(_M_get_key(*__cur), __key); ++__cur);
        return _Pii(__first, __cur);
      }
    }
    return _Pii(end(), end());
  }

  size_type erase(const key_type& __key);
  void erase(const_iterator __it);
  void erase(const_iterator __first, const_iterator __last);

private:
  void _M_enlarge(size_type __n);
  void _M_reduce();
  void _M_resize();
  void _M_rehash(size_type __num_buckets);
#if defined (_STLP_DEBUG)
  void _M_check() const;
#endif

public:
  void rehash(size_type __num_buckets_hint);
  void resize(size_type __num_buckets_hint)
  { rehash(__num_buckets_hint); }
  void clear();

  // this is for hash_map::operator[]
  reference _M_insert(const value_type& __obj);

private:
  //__n is set to the first bucket that has to be modified if any
  //erase/insert operation is done after the returned iterator.
  iterator _M_before_begin(size_type &__n) const;

  static iterator _S_before_begin(const _ElemsCont& __elems, const _BucketVector& __buckets,
                                  size_type &__n);

  void _M_initialize_buckets(size_type __n) {
    const size_type __n_buckets = _STLP_PRIV _Stl_prime_type::_S_next_size(__n) + 1;
    _M_buckets.reserve(__n_buckets);
    _M_buckets.assign(__n_buckets, __STATIC_CAST(_BucketType*, 0));
  }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type _M_bkt_num_key(const _KT& __key) const
  { return _M_bkt_num_key(__key, bucket_count()); }

  size_type _M_bkt_num(const value_type& __obj) const
  { return _M_bkt_num_key(_M_get_key(__obj)); }

  _STLP_TEMPLATE_FOR_CONT_EXT
  size_type _M_bkt_num_key(const _KT& __key, size_type __n) const
  { return _M_hash(__key) % __n; }

  size_type _M_bkt_num(const value_type& __obj, size_t __n) const
  { return _M_bkt_num_key(_M_get_key(__obj), __n); }

  void _M_copy_from(const _Self& __ht);
};

#if defined (_STLP_DEBUG)
#  undef hashtable
_STLP_MOVE_TO_STD_NAMESPACE
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_hashtable.c>
#endif

#if defined (_STLP_DEBUG)
#  include <stl/debug/_hashtable.h>
#endif

_STLP_BEGIN_NAMESPACE

#define _STLP_TEMPLATE_HEADER template <class _Val, class _Key, class _HF, class _Traits, class _ExK, class _EqK, class _All>
#define _STLP_TEMPLATE_CONTAINER hashtable<_Val,_Key,_HF,_Traits,_ExK,_EqK,_All>
#include <stl/_relops_hash_cont.h>
#undef _STLP_TEMPLATE_CONTAINER
#undef _STLP_TEMPLATE_HEADER

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_NO_MOVE_SEMANTIC)
template <class _Val, class _Key, class _HF, class _Traits, class _ExK, class _EqK, class _All>
struct __move_traits<hashtable<_Val, _Key, _HF, _Traits, _ExK, _EqK, _All> > {
  //Hashtables are movable:
  typedef __true_type implemented;

  //Completeness depends on many template parameters, for the moment we consider it not complete:
  typedef __false_type complete;
};
#endif

_STLP_END_NAMESPACE

#endif /* _STLP_INTERNAL_HASHTABLE_H */

// Local Variables:
// mode:C++
// End:
