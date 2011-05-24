/*
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

#ifndef _STLP_DEBUG_C
#define _STLP_DEBUG_C

#if defined (_STLP_DEBUG)
#if defined (_STLP_THREADS)
#  if !defined (_STLP_NEED_MUTABLE)
#    define _STLP_ACQUIRE_LOCK(_Lock) _Lock._M_acquire_lock();
#    define _STLP_RELEASE_LOCK(_Lock) _Lock._M_release_lock();
#  else
#    define _STLP_ACQUIRE_LOCK(_Lock) ((_STLP_mutex&)_Lock)._M_acquire_lock();
#    define _STLP_RELEASE_LOCK(_Lock) ((_STLP_mutex&)_Lock)._M_release_lock();
#  endif /* _STLP_NEED_MUTABLE */
#else
#  define _STLP_ACQUIRE_LOCK(_Lock)
#  define _STLP_RELEASE_LOCK(_Lock)
#endif /* _STLP_THREADS */

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

//==========================================================
//  global non-inline functions
//==========================================================
// [ i1, i2)
#if !defined (__DMC__)
template <class _Iterator>
inline bool  _STLP_CALL
stlp_in_range_aux(const _Iterator& __it, const _Iterator& __first,
                  const _Iterator& __last, const random_access_iterator_tag &) {
    return ( __it >= __first &&
             __it < __last);
}
#endif

template <class _Iterator1, class _Iterator>
#if defined (_STLP_MSVC)
inline bool _STLP_CALL  stlp_in_range_aux(_Iterator1 __it, const _Iterator& __first,
#else
inline bool _STLP_CALL  stlp_in_range_aux(const _Iterator1& __it, const _Iterator& __first,
#endif
                                          const _Iterator& __last, const forward_iterator_tag &) {
  _Iterator1 __i(__first);
  for (;  __i != __last && __i != __it; ++__i);
  return (__i != __last);
}

#if defined (_STLP_NONTEMPL_BASE_MATCH_BUG)
template <class _Iterator1, class _Iterator>
inline bool  _STLP_CALL
stlp_in_range_aux(const _Iterator1& __it, const _Iterator& __first,
                  const _Iterator& __last, const bidirectional_iterator_tag &) {
  _Iterator1 __i(__first);
  for (;  __i != __last && __i != __it; ++__i);
  return (__i != __last);
}
#endif

template <class _Iterator>
bool _STLP_CALL __check_range_aux(const _Iterator& __first, const _Iterator& __last,
                                  const __false_type& /*_IsIntegral*/) {
  _STLP_VERBOSE_RETURN(__valid_range(__first,__last), _StlMsg_INVALID_RANGE )
  return true;
}

template <class _Integral>
bool _STLP_CALL __check_range_aux(_Integral /*__first*/, _Integral /*__last*/,
                                  const __true_type& /*_IsIntegral*/)
{ return true; }

template <class _Iterator>
bool _STLP_CALL  __check_range(const _Iterator& __first, const _Iterator& __last) {
  typedef typename _IsIntegral<_Iterator>::_Ret _Integral;
  return __check_range_aux(__first, __last, _Integral());
}

template <class _Iterator>
bool _STLP_CALL  __check_range(const _Iterator& __it,
                               const _Iterator& __start, const _Iterator& __finish) {
  _STLP_VERBOSE_RETURN(stlp_in_range(__it, __start, __finish),
                       _StlMsg_NOT_IN_RANGE_1)
  return true;
}

template <class _Iterator>
bool _STLP_CALL  __check_range(const _Iterator& __first, const _Iterator& __last,
                               const _Iterator& __start, const _Iterator& __finish) {
  _STLP_VERBOSE_RETURN(stlp_in_range(__first, __last, __start, __finish),
                       _StlMsg_NOT_IN_RANGE_2)
  return true;
}

template <class _Tp>
bool _STLP_CALL __check_ptr_range(const _Tp* __first, const _Tp* __last) {
  _STLP_VERBOSE_RETURN((__first != 0 || __last == 0), _StlMsg_INVALID_ARGUMENT)
  _STLP_VERBOSE_RETURN(__valid_range(__first, __last, random_access_iterator_tag()),
                       _StlMsg_INVALID_RANGE)
  return true;
}

//===============================================================
template <class _Iterator>
void _STLP_CALL __invalidate_range(const __owned_list* __base,
                                   const _Iterator& __first,
                                   const _Iterator& __last) {
  typedef __owned_link _L_type;
  _STLP_ACQUIRE_LOCK(__base->_M_lock)
  _L_type* __prev = __CONST_CAST(_L_type*, &__base->_M_node);
  _L_type* __pos = __prev->_M_next;

  while (__pos != 0) {
    if (!(&__first == __STATIC_CAST(_Iterator*, __pos) || &__last == __STATIC_CAST(_Iterator*, __pos)) &&
        stlp_in_range_aux(__STATIC_CAST(_Iterator*, __pos)->_M_iterator,
                          __first._M_iterator, __last._M_iterator,
                          _STLP_ITERATOR_CATEGORY(__first, _Iterator))) {
      __pos->_M_owner = 0;
      __prev->_M_next = __pos->_M_next;
    }
    else {
      __prev = __pos;
    }
    __pos = __prev->_M_next;
  }
  _STLP_RELEASE_LOCK(__base->_M_lock)
}

template <class _Iterator>
void _STLP_CALL __invalidate_iterator(const __owned_list* __base,
                                      const _Iterator& __it) {
  typedef __owned_link   _L_type;
  _STLP_ACQUIRE_LOCK(__base->_M_lock)
  _L_type* __prev = __CONST_CAST(_L_type*, &__base->_M_node);
  _L_type* __pos = __prev->_M_next;
  while (__pos != 0) {
    // this requires safe iterators to be derived from __owned_link
    if ((__pos != __STATIC_CAST(const _L_type*, &__it)) &&
        (__STATIC_CAST(_Iterator*, __pos)->_M_iterator == __it._M_iterator)) {
      __pos->_M_owner = 0;
      __prev->_M_next = __pos->_M_next;
    }
    else {
      __prev = __pos;
    }
    __pos = __prev->_M_next;
  }
  _STLP_RELEASE_LOCK(__base->_M_lock)
}

template <class _Iterator>
void _STLP_CALL  __change_range_owner(const _Iterator& __first,
                                      const _Iterator& __last,
                                      const __owned_list* __dst) {
  if (__first._Owner() == __dst)
    return;

  typedef __owned_link _L_type;
  // Check __stl_debug_engine<_Dummy>::_Swap_owners comments to see why there is no lock here
  //_STLP_ACQUIRE_LOCK(__base->_M_lock)
  __owned_list *__base = __CONST_CAST(__owned_list*, __first._Owner());
  _L_type* __src_prev = &__base->_M_node;
  _L_type* __pos = __src_prev->_M_next;
  _L_type* __dst_prev = __CONST_CAST(_L_type*, &__dst->_M_node);

  while (__pos != 0) {
    if (!(&__first == __STATIC_CAST(_Iterator*, __pos) || &__last == __STATIC_CAST(_Iterator*, __pos)) &&
        stlp_in_range_aux(__STATIC_CAST(_Iterator*, __pos)->_M_iterator,
                          __first._M_iterator, __last._M_iterator,
                          _STLP_ITERATOR_CATEGORY(__first, _Iterator))) {
      __pos->_M_owner = __CONST_CAST(__owned_list*, __dst);
      //remove __pos from __base:
      __src_prev->_M_next = __pos->_M_next;
      //add __pos to __dst:
      __pos->_M_next = __dst_prev->_M_next;
      __dst_prev->_M_next = __pos;
    }
    else {
      __src_prev = __pos;
    }
    __pos = __src_prev->_M_next;
  }

#if defined(_STLP_WCE) && defined(_ARM_)
  // Note: This part is needed for compiling under Windows CE under ARM and correctly using
  // _STLP_DEBUG mode. This comes from a bug in the ARM compiler where checked iterators that
  // are passed by value are not copied over correctly. When __change_range_owner is called,
  // e.g. in std::list::splice() the wrong _M_owner field gets modified and the __first
  // iterator has the old _M_owner field, but was moved to the new __owned_list. Setting
  // the first iterator's _M_owner field fixes this. Ugly but works.
  __pos = __CONST_CAST(_Iterator*, &__first);
  __pos->_M_owner = __CONST_CAST(__owned_list*, __dst);
#endif
  //_STLP_RELEASE_LOCK(__base->_M_lock)
}

template <class _Iterator>
void _STLP_CALL __change_ite_owner(const _Iterator& __it,
                                   const __owned_list* __dst) {
  if (__it._Owner() == __dst)
    return;

  typedef __owned_link _L_type;
  // Check __stl_debug_engine<_Dummy>::_Swap_owners comments to see why there is no lock here
  //_STLP_ACQUIRE_LOCK(__base->_M_lock)
  __owned_list *__base = __CONST_CAST(__owned_list*, __it._Owner());
  _L_type* __prev = &__base->_M_node;
  _L_type* __pos = __prev->_M_next;
  _L_type* __dst_prev = __CONST_CAST(_L_type*, &__dst->_M_node);

  while (__pos != 0) {
    // this requires safe iterators to be derived from __owned_link
    if ((__pos != __STATIC_CAST(const _L_type*, &__it)) &&
        (__STATIC_CAST(_Iterator*, __pos)->_M_iterator == __it._M_iterator)) {
      __pos->_M_owner = __CONST_CAST(__owned_list*, __dst);
      //remove __pos from __base:
      __prev->_M_next = __pos->_M_next;
      //add __pos to __dst:
      __pos->_M_next = __dst_prev->_M_next;
      __dst_prev->_M_next = __pos;
    }
    else {
      __prev = __pos;
    }
    __pos = __prev->_M_next;
  }
  //_STLP_RELEASE_LOCK(__base->_M_lock)
}

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#endif /* _STLP_DEBUG */

#if defined (_STLP_EXPOSE_GLOBALS_IMPLEMENTATION)

#  ifndef _STLP_INTERNAL_CSTDLIB
#    include <stl/_cstdlib.h>
#  endif

//==========================================================
// .c section
//  owned_list non-inline methods and global functions
//==========================================================

#  if defined (_STLP_ASSERTIONS)

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

#    if !defined (_STLP_STRING_LITERAL)
#      define _STLP_STRING_LITERAL(__x) __x
#    endif

#    if defined (_STLP_USE_WIDE_INTERFACE)
// note: WinCE needs this to format single-byte strings in __stl_debug_engine::_Message
#      define _STLP_PERCENT_S "%hs"
#    else
#      define _STLP_PERCENT_S "%s"
#    endif /* _STLP_USE_WIDE_INTERFACE */

#    define _STLP_MESSAGE_TABLE_BODY = { \
_STLP_STRING_LITERAL("\n" _STLP_PERCENT_S "(%d): STL error: " _STLP_PERCENT_S "\n"), \
_STLP_STRING_LITERAL(_STLP_PERCENT_S "(%d): STL assertion failure : " _STLP_PERCENT_S "\n" _STLP_ASSERT_MSG_TRAILER), \
_STLP_STRING_LITERAL("\n" _STLP_PERCENT_S "(%d): STL error : " _STLP_PERCENT_S "\n" _STLP_PERCENT_S "(%d): STL assertion failure:     " _STLP_PERCENT_S " \n" _STLP_ASSERT_MSG_TRAILER), \
_STLP_STRING_LITERAL("Invalid argument to operation (see operation documentation)"),                  \
_STLP_STRING_LITERAL("Taking an iterator out of destroyed (or otherwise corrupted) container"),       \
_STLP_STRING_LITERAL("Trying to extract an object out from empty container"),\
_STLP_STRING_LITERAL("Past-the-end iterator could not be erased"),  \
_STLP_STRING_LITERAL("Index out of bounds"),  \
_STLP_STRING_LITERAL("Container doesn't own the iterator"),  \
_STLP_STRING_LITERAL("Container is owner of the iterator, but should not"),  \
_STLP_STRING_LITERAL("Uninitialized or invalidated (by mutating operation) iterator used"),  \
_STLP_STRING_LITERAL("Uninitialized or invalidated (by mutating operation) lefthand iterator in expression"),  \
_STLP_STRING_LITERAL("Uninitialized or invalidated (by mutating operation) righthand iterator in expression"),  \
_STLP_STRING_LITERAL("Iterators used in expression are from different owners"),  \
_STLP_STRING_LITERAL("Iterator could not be dereferenced (past-the-end ?)"),  \
_STLP_STRING_LITERAL("Range [first,last) is invalid"),  \
_STLP_STRING_LITERAL("Iterator is not in range [first,last)"),  \
_STLP_STRING_LITERAL("Range [first,last) is not in range [start,finish)"),  \
_STLP_STRING_LITERAL("The advance would produce invalid iterator"),  \
_STLP_STRING_LITERAL("Iterator is singular (advanced beyond the bounds ?)"),  \
_STLP_STRING_LITERAL("Invalid strict weak ordering predicate, if pred(a, b) then we should have !pred(b, a)"), \
_STLP_STRING_LITERAL("Invalid equivalent predicate, if pred(a, b) then we should have pred(b, a)"), \
_STLP_STRING_LITERAL("Memory block deallocated twice"),  \
_STLP_STRING_LITERAL("Deallocating a block that was never allocated"),  \
_STLP_STRING_LITERAL("Deallocating a memory block allocated for another type"),  \
_STLP_STRING_LITERAL("Size of block passed to deallocate() doesn't match block size"),  \
_STLP_STRING_LITERAL("Pointer underrun - safety margin at front of memory block overwritten"),  \
_STLP_STRING_LITERAL("Pointer overrrun - safety margin at back of memory block overwritten"),   \
_STLP_STRING_LITERAL("Attempt to dereference null pointer returned by auto_ptr::get()"),   \
_STLP_STRING_LITERAL("Memory allocation function returned a wrongly align memory block"),   \
_STLP_STRING_LITERAL("Unknown problem") \
  }

template <class _Dummy>
const char* __stl_debug_engine<_Dummy>::_Message_table[_StlMsg_MAX] _STLP_MESSAGE_TABLE_BODY;

#    undef _STLP_STRING_LITERAL
#    undef _STLP_PERCENT_S

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#  if !defined (_STLP_DEBUG_MESSAGE)
#    ifndef _STLP_INTERNAL_CSTDARG
#      include <stl/_cstdarg.h>
#    endif
#    ifndef _STLP_INTERNAL_CSTDIO
#      include <stl/_cstdio.h>
#    endif
#    if defined (_STLP_DEBUG_MODE_THROWS) && !defined (_STLP_RANGE_ERRORS_H)
#      include <stl/_range_errors.h>
#    endif

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Message(const char * __format_str, ...) {
  STLPORT_CSTD::va_list __args;
  va_start( __args, __format_str );

#      if !defined (_STLP_DEBUG_MODE_THROWS)
#        if defined (_STLP_USE_WIDE_INTERFACE)
  TCHAR __buffer[512];
  int _convert = strlen(__format_str) + 1;
  LPWSTR _lpw = (LPWSTR)alloca(_convert * sizeof(wchar_t));
  _lpw[0] = '\0';
  MultiByteToWideChar(GetACP(), 0, __format_str, -1, _lpw, _convert);
  wvsprintf(__buffer, _lpw, __args);
  _STLP_WINCE_TRACE(__buffer);
#        elif defined (_STLP_WIN32) && (defined(_STLP_MSVC) || defined (__ICL))
  char __buffer [4096];

#          if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
  vsnprintf(__buffer, _STLP_ARRAY_SIZE(__buffer), __format_str, __args);
#          else
  vsnprintf_s(__buffer, _STLP_ARRAY_SIZE(__buffer), _TRUNCATE, __format_str, __args);
#          endif

  OutputDebugStringA(__buffer);

#        elif defined (__amigaos__)
  STLPORT_CSTD::vfprintf(stderr, __format_str, (char *)__args);
#        else
  STLPORT_CSTD::vfprintf(stderr, __format_str, __args);
#        endif
#      else
  char __buffer[4096];

#        if defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
  vsnprintf_s(__buffer, _STLP_ARRAY_SIZE(__buffer), _TRUNCATE, __format_str, __args);
#        elif defined (_STLP_WIN32) && (defined(_STLP_MSVC) || defined (__ICL))
  vsnprintf(__buffer, _STLP_ARRAY_SIZE(__buffer), __format_str, __args);
#        else
  vsprintf(__buffer, __format_str, __args);
#        endif
#      endif

#      ifdef _STLP_DEBUG_MESSAGE_POST
  _STLP_DEBUG_MESSAGE_POST
#      endif

  va_end(__args);

#      if defined (_STLP_DEBUG_MODE_THROWS)
  __stl_throw_runtime_error(__buffer);
#      endif
}

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#    else
_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE
template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Message(const char * __format_str, ...)
{}
_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE
#    endif /* _STLP_DEBUG_MESSAGE */

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_IndexedError(int __error_ind, const char* __f, int __l) {
  __stl_debug_message(_Message_table[_StlFormat_ERROR_RETURN],
                      __f, __l, _Message_table[__error_ind]);
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_VerboseAssert(const char* __expr, int __error_ind, const char* __f, int __l) {
  __stl_debug_message(_Message_table[_StlFormat_VERBOSE_ASSERTION_FAILURE],
                      __f, __l, _Message_table[__error_ind], __f, __l, __expr);
  __stl_debug_terminate();
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Assert(const char* __expr, const char* __f, int __l) {
  __stl_debug_message(_Message_table[_StlFormat_ASSERTION_FAILURE],__f, __l, __expr);
  __stl_debug_terminate();
}

// if exceptions are present, sends unique exception
// if not, calls abort() to terminate
template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Terminate()
{ _STLP_ABORT(); }

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#  endif /* _STLP_ASSERTIONS */

#  if defined (_STLP_DEBUG)

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE

//==========================================================
//  owned_list non-inline methods
//==========================================================

template <class _Dummy>
void  _STLP_CALL
__stl_debug_engine<_Dummy>::_Invalidate_all(__owned_list* __l) {
  _STLP_ACQUIRE_LOCK(__l->_M_lock);
  _Stamp_all(__l, 0);
  __l->_M_node._M_next =0;
  _STLP_RELEASE_LOCK(__l->_M_lock);
}

// boris : this is unasafe routine; should be used from within critical section only !
template <class _Dummy>
void  _STLP_CALL
__stl_debug_engine<_Dummy>::_Stamp_all(__owned_list* __l, __owned_list* __o) {
  // crucial
  if (__l->_M_node._M_owner) {
    for (__owned_link*  __pos = (__owned_link*)__l->_M_node._M_next;
      __pos != 0; __pos = (__owned_link*)__pos->_M_next) {
      _STLP_ASSERT(__pos->_Owner()== __l)
      __pos->_M_owner=__o;
    }
  }
}

template <class _Dummy>
void  _STLP_CALL
__stl_debug_engine<_Dummy>::_Verify(const __owned_list* __l) {
  _STLP_ACQUIRE_LOCK(__l->_M_lock);
  if (__l) {
    _STLP_ASSERT(__l->_M_node._Owner() != 0)
    for (__owned_link* __pos = (__owned_link*)__l->_M_node._M_next;
         __pos != 0; __pos = (__owned_link*)__pos->_M_next) {
      _STLP_ASSERT(__pos->_Owner()== __l)
    }
  }
  _STLP_RELEASE_LOCK(__l->_M_lock);
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Swap_owners(__owned_list& __x, __owned_list& __y) {
  /*
   *  according to the standard : --no swap() function invalidates any references,
   *  pointers,  or  iterators referring to the elements of the containers being swapped.
   */

  __owned_link* __tmp;

  /*
   * boris : there is a deadlock potential situation here if we lock two containers sequentially.
   * As user is supposed to provide its own synchronization around swap() ( it is unsafe to do any container/iterator access
   * in parallel with swap()), we just do not use any locking at all -- that behaviour is closer to non-debug version
   */

  __tmp = __x._M_node._M_next;

  _Stamp_all(&__x, &__y);
  _Stamp_all(&__y, &__x);

  __x._M_node._M_next = __y._M_node._M_next;
  __y._M_node._M_next = __tmp;
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_Set_owner(__owned_list& __src, __owned_list& __dst) {
  if (&__src == &__dst)
    return;

  // Check __stl_debug_engine<_Dummy>::_Swap_owners comments to see why there is no lock here
  typedef __owned_link _L_type;
  _L_type* __prev = &__src._M_node;
  _L_type* __pos = __prev->_M_next;

  while (__pos != 0) {
    __pos->_M_owner = &__dst;
    __prev = __pos;
    __pos = __prev->_M_next;
  }
  __prev->_M_next = __dst._M_node._M_next;
  __dst._M_node._M_next = __src._M_node._M_next;
  __src._M_node._M_next = 0;
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_M_detach(__owned_list* __l, __owned_link* __c_node) {
  if (__l  != 0) {

    _STLP_VERBOSE_ASSERT(__l->_Owner()!=0, _StlMsg_INVALID_CONTAINER)

    _STLP_ACQUIRE_LOCK(__l->_M_lock)
      // boris : re-test the condition in case someone else already deleted us
      if(__c_node->_M_owner != 0) {
        __owned_link* __prev, *__next;

        for (__prev = &__l->_M_node; (__next = __prev->_M_next) != __c_node;
             __prev = __next) {
          _STLP_ASSERT(__next && __next->_Owner() == __l)
            }

        __prev->_M_next = __c_node->_M_next;
        __c_node->_M_owner=0;
      }
    _STLP_RELEASE_LOCK(__l->_M_lock)
  }
}

template <class _Dummy>
void _STLP_CALL
__stl_debug_engine<_Dummy>::_M_attach(__owned_list* __l, __owned_link* __c_node) {
  if (__l ==0) {
    (__c_node)->_M_owner = 0;
  } else {
    _STLP_VERBOSE_ASSERT(__l->_Owner()!=0, _StlMsg_INVALID_CONTAINER)
    _STLP_ACQUIRE_LOCK(__l->_M_lock)
    __c_node->_M_owner = __l;
    __c_node->_M_next = __l->_M_node._M_next;
    __l->_M_node._M_next = __c_node;
    _STLP_RELEASE_LOCK(__l->_M_lock)
  }
}

template <class _Dummy>
void* _STLP_CALL
__stl_debug_engine<_Dummy>::_Get_container_ptr(const __owned_link* __l) {
  const __owned_list* __owner    = __l->_Owner();
  _STLP_VERBOSE_RETURN_0(__owner != 0, _StlMsg_INVALID_ITERATOR)
  void* __ret = __CONST_CAST(void*,__owner->_Owner());
  _STLP_VERBOSE_RETURN_0(__ret !=0, _StlMsg_INVALID_CONTAINER)
  return __ret;
}

template <class _Dummy>
bool _STLP_CALL
__stl_debug_engine<_Dummy>::_Check_same_owner(const __owned_link& __i1,
                                              const __owned_link& __i2) {
  _STLP_VERBOSE_RETURN(__i1._Valid(), _StlMsg_INVALID_LEFTHAND_ITERATOR)
  _STLP_VERBOSE_RETURN(__i2._Valid(), _StlMsg_INVALID_RIGHTHAND_ITERATOR)
  _STLP_VERBOSE_RETURN((__i1._Owner() == __i2._Owner()), _StlMsg_DIFFERENT_OWNERS)
  return true;
}

template <class _Dummy>
bool _STLP_CALL
__stl_debug_engine<_Dummy>::_Check_same_or_null_owner(const __owned_link& __i1,
                                                      const __owned_link& __i2) {
  _STLP_VERBOSE_RETURN(__i1._Owner() == __i2._Owner(), _StlMsg_DIFFERENT_OWNERS)
  return true;
}

template <class _Dummy>
bool _STLP_CALL
__stl_debug_engine<_Dummy>::_Check_if_owner( const __owned_list * __l, const __owned_link& __it) {
  const __owned_list* __owner_ptr = __it._Owner();
  _STLP_VERBOSE_RETURN(__owner_ptr != 0, _StlMsg_INVALID_ITERATOR)
  _STLP_VERBOSE_RETURN(__l == __owner_ptr, _StlMsg_NOT_OWNER)
  return true;
}

template <class _Dummy>
bool _STLP_CALL
__stl_debug_engine<_Dummy>::_Check_if_not_owner( const __owned_list * __l, const __owned_link& __it) {
  const __owned_list* __owner_ptr = __it._Owner();
  _STLP_VERBOSE_RETURN(__owner_ptr != 0, _StlMsg_INVALID_ITERATOR)
  _STLP_VERBOSE_RETURN(__l != __owner_ptr, _StlMsg_SHOULD_NOT_OWNER)
  return true;
}

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#  endif /* _STLP_DEBUG */

#endif /* if defined (EXPOSE_GLOBALS_IMPLEMENTATION) */

#endif /* header guard */

// Local Variables:
// mode:C++
// End:
