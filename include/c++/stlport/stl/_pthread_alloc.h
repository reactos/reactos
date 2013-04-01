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

#ifndef _STLP_PTHREAD_ALLOC_H
#define _STLP_PTHREAD_ALLOC_H

/*
 * Pthread-specific node allocator.
 * This is similar to the default allocator, except that free-list
 * information is kept separately for each thread, avoiding locking.
 * This should be reasonably fast even in the presence of threads.
 * The down side is that storage may not be well-utilized.
 * It is not an error to allocate memory in thread A and deallocate
 * it in thread B.  But this effectively transfers ownership of the memory,
 * so that it can only be reallocated by thread B.  Thus this can effectively
 * result in a storage leak if it's done on a regular basis.
 * It can also result in frequent sharing of
 * cache lines among processors, with potentially serious performance
 * consequences.
 */

#if !defined (_STLP_PTHREADS)
#  error POSIX specific allocator implementation. Your system do not seems to \
have this interface so please comment the _STLP_USE_PERTHREAD_ALLOC macro \
or report to the STLport forum.
#endif

#if defined (_STLP_USE_NO_IOSTREAMS)
#  error You cannot use per thread allocator implementation without building \
STLport libraries.
#endif

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

struct _Pthread_alloc_per_thread_state;

// Pthread-specific allocator.
class _STLP_CLASS_DECLSPEC _Pthread_alloc {
public: // but only for internal use:
  typedef _Pthread_alloc_per_thread_state __state_type;
  typedef char value_type;

public:
  // Return a recycled or new per thread state.
  static __state_type * _STLP_CALL _S_get_per_thread_state();

  /* n must be > 0      */
  static void * _STLP_CALL allocate(size_t& __n);

  /* p may not be 0 */
  static void _STLP_CALL deallocate(void *__p, size_t __n);

  // boris : versions for per_thread_allocator
  /* n must be > 0      */
  static void * _STLP_CALL allocate(size_t& __n, __state_type* __a);

  /* p may not be 0 */
  static void _STLP_CALL deallocate(void *__p, size_t __n, __state_type* __a);

  static void * _STLP_CALL reallocate(void *__p, size_t __old_sz, size_t& __new_sz);
};

_STLP_MOVE_TO_STD_NAMESPACE

typedef _STLP_PRIV _Pthread_alloc __pthread_alloc;
typedef __pthread_alloc pthread_alloc;

template <class _Tp>
class pthread_allocator : public __stlport_class<pthread_allocator<_Tp> > {
  typedef pthread_alloc _S_Alloc;          // The underlying allocator.
public:
  typedef size_t     size_type;
  typedef ptrdiff_t  difference_type;
  typedef _Tp*       pointer;
  typedef const _Tp* const_pointer;
  typedef _Tp&       reference;
  typedef const _Tp& const_reference;
  typedef _Tp        value_type;

#ifdef _STLP_MEMBER_TEMPLATE_CLASSES
  template <class _NewType> struct rebind {
    typedef pthread_allocator<_NewType> other;
  };
#endif

  pthread_allocator() _STLP_NOTHROW {}
  pthread_allocator(const pthread_allocator<_Tp>& a) _STLP_NOTHROW {}

#if defined (_STLP_MEMBER_TEMPLATES) /* && defined (_STLP_FUNCTION_PARTIAL_ORDER) */
  template <class _OtherType> pthread_allocator(const pthread_allocator<_OtherType>&)
    _STLP_NOTHROW {}
#endif

  ~pthread_allocator() _STLP_NOTHROW {}

  pointer address(reference __x) const { return &__x; }
  const_pointer address(const_reference __x) const { return &__x; }

  // __n is permitted to be 0.  The C++ standard says nothing about what
  // the return value is when __n == 0.
  _Tp* allocate(size_type __n, const void* = 0) {
    if (__n > max_size()) {
      _STLP_THROW_BAD_ALLOC;
    }
    if (__n != 0) {
      size_type __buf_size = __n * sizeof(value_type);
      _Tp* __ret = __REINTERPRET_CAST(value_type*, _S_Alloc::allocate(__buf_size));
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__ret, _STLP_SHRED_BYTE, __buf_size);
#endif
      return __ret;
    }
    else
      return 0;
  }

  void deallocate(pointer __p, size_type __n) {
    _STLP_ASSERT( (__p == 0) == (__n == 0) )
    if (__p != 0) {
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__p, _STLP_SHRED_BYTE, __n * sizeof(value_type));
#endif
      _S_Alloc::deallocate(__p, __n * sizeof(value_type));
    }
  }

  size_type max_size() const _STLP_NOTHROW
  { return size_t(-1) / sizeof(_Tp); }

  void construct(pointer __p, const _Tp& __val) { new(__p) _Tp(__val); }
  void destroy(pointer _p) { _p->~_Tp(); }

#if defined (_STLP_NO_EXTENSIONS)
  /* STLport extension giving rounded size of an allocated memory buffer
   * This method do not have to be part of a user defined allocator implementation
   * and won't even be called if such a function was granted.
   */
protected:
#endif
  _Tp* allocate(size_type __n, size_type& __allocated_n) {
    if (__n > max_size()) {
      _STLP_THROW_BAD_ALLOC;
    }
    if (__n != 0) {
      size_type __buf_size = __n * sizeof(value_type);
      _Tp* __ret = __REINTERPRET_CAST(value_type*, _S_Alloc::allocate(__buf_size));
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__ret, _STLP_SHRED_BYTE, __buf_size);
#endif
      __allocated_n = __buf_size / sizeof(value_type);
      return __ret;
    }
    else
      return 0;
  }
#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(pthread_allocator<_Tp>& __x) {}
#endif
};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC pthread_allocator<void> {
public:
  typedef size_t      size_type;
  typedef ptrdiff_t   difference_type;
  typedef void*       pointer;
  typedef const void* const_pointer;
  typedef void        value_type;
#ifdef _STLP_MEMBER_TEMPLATE_CLASSES
  template <class _NewType> struct rebind {
    typedef pthread_allocator<_NewType> other;
  };
#endif
};

template <class _T1, class _T2>
inline bool operator==(const pthread_allocator<_T1>&,
                       const pthread_allocator<_T2>& a2)
{ return true; }

#ifdef _STLP_FUNCTION_TMPL_PARTIAL_ORDER
template <class _T1, class _T2>
inline bool operator!=(const pthread_allocator<_T1>&,
                       const pthread_allocator<_T2>&)
{ return false; }
#endif


#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)

template <class _Tp, class _Atype>
struct _Alloc_traits<_Tp, pthread_allocator<_Atype> >
{ typedef pthread_allocator<_Tp> allocator_type; };

#endif

#if defined (_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE)

template <class _Tp1, class _Tp2>
inline pthread_allocator<_Tp2>&
__stl_alloc_rebind(pthread_allocator<_Tp1>& __x, const _Tp2*)
{ return (pthread_allocator<_Tp2>&)__x; }

template <class _Tp1, class _Tp2>
inline pthread_allocator<_Tp2>
__stl_alloc_create(pthread_allocator<_Tp1>&, const _Tp2*)
{ return pthread_allocator<_Tp2>(); }

#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp>
struct __pthread_alloc_type_traits {
  typedef typename _IsSTLportClass<pthread_allocator<_Tp> >::_Ret _STLportAlloc;
  //The default allocator implementation which is recognize thanks to the
  //__stlport_class inheritance is a stateless object so:
  typedef _STLportAlloc has_trivial_default_constructor;
  typedef _STLportAlloc has_trivial_copy_constructor;
  typedef _STLportAlloc has_trivial_assignment_operator;
  typedef _STLportAlloc has_trivial_destructor;
  typedef _STLportAlloc is_POD_type;
};

_STLP_MOVE_TO_STD_NAMESPACE

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp>
struct __type_traits<pthread_allocator<_Tp> > : _STLP_PRIV __pthread_alloc_type_traits<_Tp> {};
#else
_STLP_TEMPLATE_NULL
struct __type_traits<pthread_allocator<char> > : _STLP_PRIV __pthread_alloc_type_traits<char> {};
#  if defined (_STLP_HAS_WCHAR_T)
_STLP_TEMPLATE_NULL
struct __type_traits<pthread_allocator<wchar_t> > : _STLP_PRIV __pthread_alloc_type_traits<wchar_t> {};
#  endif
#  if defined (_STLP_USE_PTR_SPECIALIZATIONS)
_STLP_TEMPLATE_NULL
struct __type_traits<pthread_allocator<void*> > : _STLP_PRIV __pthread_alloc_type_traits<void*> {};
#  endif
#endif

//
// per_thread_allocator<> : this allocator always return memory to the same thread
// it was allocated from.
//

template <class _Tp>
class per_thread_allocator {
  typedef pthread_alloc _S_Alloc;          // The underlying allocator.
  typedef pthread_alloc::__state_type __state_type;
public:
  typedef size_t     size_type;
  typedef ptrdiff_t  difference_type;
  typedef _Tp*       pointer;
  typedef const _Tp* const_pointer;
  typedef _Tp&       reference;
  typedef const _Tp& const_reference;
  typedef _Tp        value_type;

#ifdef _STLP_MEMBER_TEMPLATE_CLASSES
  template <class _NewType> struct rebind {
    typedef per_thread_allocator<_NewType> other;
  };
#endif

  per_thread_allocator() _STLP_NOTHROW {
    _M_state = _S_Alloc::_S_get_per_thread_state();
  }
  per_thread_allocator(const per_thread_allocator<_Tp>& __a) _STLP_NOTHROW : _M_state(__a._M_state){}

#if defined (_STLP_MEMBER_TEMPLATES) /* && defined (_STLP_FUNCTION_PARTIAL_ORDER) */
  template <class _OtherType> per_thread_allocator(const per_thread_allocator<_OtherType>& __a)
    _STLP_NOTHROW : _M_state(__a._M_state) {}
#endif

  ~per_thread_allocator() _STLP_NOTHROW {}

  pointer address(reference __x) const { return &__x; }
  const_pointer address(const_reference __x) const { return &__x; }

  // __n is permitted to be 0.  The C++ standard says nothing about what
  // the return value is when __n == 0.
  _Tp* allocate(size_type __n, const void* = 0) {
    if (__n > max_size()) {
      _STLP_THROW_BAD_ALLOC;
    }
    if (__n != 0) {
      size_type __buf_size = __n * sizeof(value_type);
      _Tp* __ret = __REINTERPRET_CAST(_Tp*, _S_Alloc::allocate(__buf_size, _M_state));
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__ret, _STLP_SHRED_BYTE, __buf_size);
#endif
      return __ret;
    }
    else
      return 0;
  }

  void deallocate(pointer __p, size_type __n) {
    _STLP_ASSERT( (__p == 0) == (__n == 0) )
    if (__p != 0) {
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__p, _STLP_SHRED_BYTE, __n * sizeof(value_type));
#endif
      _S_Alloc::deallocate(__p, __n * sizeof(value_type), _M_state);
    }
  }

  size_type max_size() const _STLP_NOTHROW
  { return size_t(-1) / sizeof(_Tp); }

  void construct(pointer __p, const _Tp& __val) { new(__p) _Tp(__val); }
  void destroy(pointer _p) { _p->~_Tp(); }

  // state is being kept here
  __state_type* _M_state;

#if defined (_STLP_NO_EXTENSIONS)
  /* STLport extension giving rounded size of an allocated memory buffer
   * This method do not have to be part of a user defined allocator implementation
   * and won't even be called if such a function was granted.
   */
protected:
#endif
  _Tp* allocate(size_type __n, size_type& __allocated_n) {
    if (__n > max_size()) {
      _STLP_THROW_BAD_ALLOC;
    }
    if (__n != 0) {
      size_type __buf_size = __n * sizeof(value_type);
      _Tp* __ret = __REINTERPRET_CAST(value_type*, _S_Alloc::allocate(__buf_size, _M_state));
#if defined (_STLP_DEBUG_UNINITIALIZED) && !defined (_STLP_DEBUG_ALLOC)
      memset((char*)__ret, _STLP_SHRED_BYTE, __buf_size);
#endif
      __allocated_n = __buf_size / sizeof(value_type);
      return __ret;
    }
    else
      return 0;
  }
};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC per_thread_allocator<void> {
public:
  typedef size_t      size_type;
  typedef ptrdiff_t   difference_type;
  typedef void*       pointer;
  typedef const void* const_pointer;
  typedef void        value_type;
#ifdef _STLP_MEMBER_TEMPLATE_CLASSES
  template <class _NewType> struct rebind {
    typedef per_thread_allocator<_NewType> other;
  };
#endif
};

template <class _T1, class _T2>
inline bool operator==(const per_thread_allocator<_T1>& __a1,
                       const per_thread_allocator<_T2>& __a2)
{ return __a1._M_state == __a2._M_state; }

#ifdef _STLP_FUNCTION_TMPL_PARTIAL_ORDER
template <class _T1, class _T2>
inline bool operator!=(const per_thread_allocator<_T1>& __a1,
                       const per_thread_allocator<_T2>& __a2)
{ return __a1._M_state != __a2._M_state; }
#endif


#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)

template <class _Tp, class _Atype>
struct _Alloc_traits<_Tp, per_thread_allocator<_Atype> >
{ typedef per_thread_allocator<_Tp> allocator_type; };

#endif

#if defined (_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE)

template <class _Tp1, class _Tp2>
inline per_thread_allocator<_Tp2>&
__stl_alloc_rebind(per_thread_allocator<_Tp1>& __x, const _Tp2*)
{ return (per_thread_allocator<_Tp2>&)__x; }

template <class _Tp1, class _Tp2>
inline per_thread_allocator<_Tp2>
__stl_alloc_create(per_thread_allocator<_Tp1>&, const _Tp2*)
{ return per_thread_allocator<_Tp2>(); }

#endif /* _STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE */

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp>
struct __perthread_alloc_type_traits {
  typedef typename _IsSTLportClass<per_thread_allocator<_Tp> >::_Ret _STLportAlloc;
  //The default allocator implementation which is recognize thanks to the
  //__stlport_class inheritance is a stateless object so:
  typedef __false_type has_trivial_default_constructor;
  typedef _STLportAlloc has_trivial_copy_constructor;
  typedef _STLportAlloc has_trivial_assignment_operator;
  typedef _STLportAlloc has_trivial_destructor;
  typedef __false_type is_POD_type;
};

_STLP_MOVE_TO_STD_NAMESPACE

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp>
struct __type_traits<per_thread_allocator<_Tp> > : _STLP_PRIV __perthread_alloc_type_traits<_Tp> {};
#else
_STLP_TEMPLATE_NULL
struct __type_traits<per_thread_allocator<char> > : _STLP_PRIV __perthread_alloc_type_traits<char> {};
#  if defined (_STLP_HAS_WCHAR_T)
_STLP_TEMPLATE_NULL
struct __type_traits<per_thread_allocator<wchar_t> > : _STLP_PRIV __perthread_alloc_type_traits<wchar_t> {};
#  endif
#  if defined (_STLP_USE_PTR_SPECIALIZATIONS)
_STLP_TEMPLATE_NULL
struct __type_traits<per_thread_allocator<void*> > : _STLP_PRIV __perthread_alloc_type_traits<void*> {};
#  endif
#endif


_STLP_END_NAMESPACE

#endif /* _STLP_PTHREAD_ALLOC */

// Local Variables:
// mode:C++
// End:
