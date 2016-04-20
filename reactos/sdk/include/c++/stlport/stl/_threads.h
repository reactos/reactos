/*
 * Copyright (c) 1997-1999
 * Silicon Graphics Computer Systems, Inc.
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

// WARNING: This is an internal header file, included by other C++
// standard library headers.  You should not attempt to use this header
// file directly.


#ifndef _STLP_INTERNAL_THREADS_H
#define _STLP_INTERNAL_THREADS_H

// Supported threading models are native SGI, pthreads, uithreads
// (similar to pthreads, but based on an earlier draft of the Posix
// threads standard), and Win32 threads.  Uithread support by Jochen
// Schlick, 1999, and Solaris threads generalized to them.

#ifndef _STLP_INTERNAL_CSTDDEF
#  include <stl/_cstddef.h>
#endif

#ifndef _STLP_INTERNAL_CSTDLIB
#  include <stl/_cstdlib.h>
#endif

// On SUN and Mac OS X gcc, zero-initialization works just fine...
#if defined (__sun) || (defined (__GNUC__) && defined(__APPLE__))
#  define _STLP_MUTEX_INITIALIZER
#endif

/* This header defines the following atomic operation that platform should
 * try to support as much as possible. Atomic operation are exposed as macro
 * in order to easily test for their existance. They are:
 * __stl_atomic_t _STLP_ATOMIC_INCREMENT(volatile __stl_atomic_t* __ptr) :
 * increment *__ptr by 1 and returns the new value
 * __stl_atomic_t _STLP_ATOMIC_DECREMENT(volatile __stl_atomic_t* __ptr) :
 * decrement  *__ptr by 1 and returns the new value
 * __stl_atomic_t _STLP_ATOMIC_EXCHANGE(volatile __stl_atomic_t* __target, __stl_atomic_t __val) :
 * assign __val to *__target and returns former *__target value
 * void* _STLP_ATOMIC_EXCHANGE_PTR(void* volatile* __target, void* __ptr) :
 * assign __ptr to *__target and returns former *__target value
 */

#if defined (_STLP_THREADS)

#  if defined (_STLP_SGI_THREADS)

#    include <mutex.h>
// Hack for SGI o32 compilers.
#    if !defined(__add_and_fetch) && \
        (__mips < 3 || !(defined (_ABIN32) || defined(_ABI64)))
#      define __add_and_fetch(__l,__v) add_then_test((unsigned long*)__l,__v)
#      define __test_and_set(__l,__v)  test_and_set(__l,__v)
#    endif /* o32 */

#    if __mips < 3 || !(defined (_ABIN32) || defined(_ABI64))
#      define _STLP_ATOMIC_EXCHANGE(__p, __q) test_and_set(__p, __q)
#    else
#      define _STLP_ATOMIC_EXCHANGE(__p, __q) __test_and_set((unsigned long*)__p, (unsigned long)__q)
#    endif

#    define _STLP_ATOMIC_INCREMENT(__x) __add_and_fetch(__x, 1)
#    define _STLP_ATOMIC_DECREMENT(__x) __add_and_fetch(__x, (size_t) -1)
typedef long __stl_atomic_t;

#  elif defined (_STLP_PTHREADS)

#    include <pthread.h>
#    if !defined (_STLP_USE_PTHREAD_SPINLOCK)
#      if defined (PTHREAD_MUTEX_INITIALIZER) && !defined (_STLP_MUTEX_INITIALIZER) && defined (_REENTRANT)
#        define _STLP_MUTEX_INITIALIZER = { PTHREAD_MUTEX_INITIALIZER }
#      endif
//HPUX variants have (on some platforms optional) non-standard "DCE" pthreads impl
#      if defined (_DECTHREADS_) && (defined (_PTHREAD_USE_D4) || defined (__hpux)) && !defined (_CMA_SUPPRESS_EXTERNALS_)
#        define _STLP_PTHREAD_ATTR_DEFAULT pthread_mutexattr_default
#      else
#        define _STLP_PTHREAD_ATTR_DEFAULT 0
#      endif
#    else
#      if defined (__OpenBSD__)
#        include <spinlock.h>
#      endif
#    endif

#    if defined (__GNUC__) && defined (__i386__)
#      if !defined (_STLP_ATOMIC_INCREMENT)
inline long _STLP_atomic_increment_gcc_x86(long volatile* p) {
  long result;
  __asm__ __volatile__
    ("lock; xaddl  %1, %0;"
    :"=m" (*p), "=r" (result)
    :"m" (*p),  "1"  (1)
    :"cc");
  return result + 1;
}
#        define _STLP_ATOMIC_INCREMENT(__x) (_STLP_atomic_increment_gcc_x86((long volatile*)__x))
#      endif

#      if !defined (_STLP_ATOMIC_DECREMENT)
inline long _STLP_atomic_decrement_gcc_x86(long volatile* p) {
  long result;
  __asm__ __volatile__
    ("lock; xaddl  %1, %0;"
    :"=m" (*p), "=r" (result)
    :"m" (*p),  "1"  (-1)
    :"cc");
  return result - 1;
}
#        define _STLP_ATOMIC_DECREMENT(__x) (_STLP_atomic_decrement_gcc_x86((long volatile*)__x))
#      endif
typedef long __stl_atomic_t;
#    else
typedef size_t __stl_atomic_t;
#    endif /* if defined(__GNUC__) && defined(__i386__) */

#  elif defined (_STLP_WIN32THREADS)

#    if !defined (_STLP_ATOMIC_INCREMENT)
#      if !defined (_STLP_NEW_PLATFORM_SDK)
#        define _STLP_ATOMIC_INCREMENT(__x)           InterlockedIncrement(__CONST_CAST(long*, __x))
#        define _STLP_ATOMIC_DECREMENT(__x)           InterlockedDecrement(__CONST_CAST(long*, __x))
#        define _STLP_ATOMIC_EXCHANGE(__x, __y)       InterlockedExchange(__CONST_CAST(long*, __x), __y)
#      else
#        define _STLP_ATOMIC_INCREMENT(__x)           InterlockedIncrement(__x)
#        define _STLP_ATOMIC_DECREMENT(__x)           InterlockedDecrement(__x)
#        define _STLP_ATOMIC_EXCHANGE(__x, __y)       InterlockedExchange(__x, __y)
#      endif
#      define _STLP_ATOMIC_EXCHANGE_PTR(__x, __y)     STLPInterlockedExchangePointer(__x, __y)
#    endif
typedef long __stl_atomic_t;

#  elif defined (__DECC) || defined (__DECCXX)

#    include <machine/builtins.h>
#    define _STLP_ATOMIC_EXCHANGE __ATOMIC_EXCH_LONG
#    define _STLP_ATOMIC_INCREMENT(__x) __ATOMIC_ADD_LONG(__x, 1)
#    define _STLP_ATOMIC_DECREMENT(__x) __ATOMIC_ADD_LONG(__x, -1)
typedef long __stl_atomic_t;

#  elif defined (_STLP_SPARC_SOLARIS_THREADS)

typedef long __stl_atomic_t;
#    include <stl/_sparc_atomic.h>

#  elif defined (_STLP_UITHREADS)

// this inclusion is potential hazard to bring up all sorts
// of old-style headers. Let's assume vendor already know how
// to deal with that.
#    ifndef _STLP_INTERNAL_CTIME
#      include <stl/_ctime.h>
#    endif
#    if defined (_STLP_USE_NAMESPACES) && ! defined (_STLP_VENDOR_GLOBAL_CSTD)
using _STLP_VENDOR_CSTD::time_t;
#    endif
#    include <synch.h>
#    ifndef _STLP_INTERNAL_CSTDIO
#      include <stl/_cstdio.h>
#    endif
#    ifndef _STLP_INTERNAL_CWCHAR
#      include <stl/_cwchar.h>
#    endif
typedef size_t __stl_atomic_t;

#  elif defined (_STLP_BETHREADS)

#    include <OS.h>
#    include <cassert>
#    include <stdio.h>
#    define _STLP_MUTEX_INITIALIZER = { 0 }
typedef size_t __stl_atomic_t;

#  elif defined (_STLP_NWTHREADS)

#    include <nwthread.h>
#    include <nwsemaph.h>
typedef size_t __stl_atomic_t;

#  elif defined(_STLP_OS2THREADS)

#    if defined (__GNUC__)
#      define INCL_DOSSEMAPHORES
#      include <os2.h>
#    else
// This section serves to replace os2.h for VisualAge C++
  typedef unsigned long ULONG;
#      if !defined (__HEV__)  /* INCL_SEMAPHORE may also define HEV */
#        define __HEV__
  typedef ULONG HEV;
  typedef HEV*  PHEV;
#      endif
  typedef ULONG APIRET;
  typedef ULONG HMTX;
  typedef HMTX*  PHMTX;
  typedef const char*  PCSZ;
  typedef ULONG BOOL32;
  APIRET _System DosCreateMutexSem(PCSZ pszName, PHEV phev, ULONG flAttr, BOOL32 fState);
  APIRET _System DosRequestMutexSem(HMTX hmtx, ULONG ulTimeout);
  APIRET _System DosReleaseMutexSem(HMTX hmtx);
  APIRET _System DosCloseMutexSem(HMTX hmtx);
#      define _STLP_MUTEX_INITIALIZER = { 0 }
#    endif /* GNUC */
typedef size_t __stl_atomic_t;

#  else

typedef size_t __stl_atomic_t;

#  endif

#else
/* no threads */
#  define _STLP_ATOMIC_INCREMENT(__x) ++(*__x)
#  define _STLP_ATOMIC_DECREMENT(__x) --(*__x)
/* We do not grant other atomic operations as they are useless if STLport do not have
 * to be thread safe
 */
typedef size_t __stl_atomic_t;
#endif

#if !defined (_STLP_MUTEX_INITIALIZER)
#  if defined(_STLP_ATOMIC_EXCHANGE)
#    define _STLP_MUTEX_INITIALIZER = { 0 }
#  elif defined(_STLP_UITHREADS)
#    define _STLP_MUTEX_INITIALIZER = { DEFAULTMUTEX }
#  else
#    define _STLP_MUTEX_INITIALIZER
#  endif
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_THREADS) && !defined (_STLP_USE_PTHREAD_SPINLOCK)
// Helper struct.  This is a workaround for various compilers that don't
// handle static variables in inline functions properly.
template <int __inst>
struct _STLP_mutex_spin {
  enum { __low_max = 30, __high_max = 1000 };
  // Low if we suspect uniprocessor, high for multiprocessor.
  static unsigned __max;
  static unsigned __last;
  static void _STLP_CALL _M_do_lock(volatile __stl_atomic_t* __lock);
  static void _STLP_CALL _S_nsec_sleep(int __log_nsec, unsigned int& __iteration);
};
#endif // !_STLP_USE_PTHREAD_SPINLOCK

// Locking class.  Note that this class *does not have a constructor*.
// It must be initialized either statically, with _STLP_MUTEX_INITIALIZER,
// or dynamically, by explicitly calling the _M_initialize member function.
// (This is similar to the ways that a pthreads mutex can be initialized.)
// There are explicit member functions for acquiring and releasing the lock.

// There is no constructor because static initialization is essential for
// some uses, and only a class aggregate (see section 8.5.1 of the C++
// standard) can be initialized that way.  That means we must have no
// constructors, no base classes, no virtual functions, and no private or
// protected members.

// For non-static cases, clients should use  _STLP_mutex.

struct _STLP_CLASS_DECLSPEC _STLP_mutex_base {
#if defined (_STLP_ATOMIC_EXCHANGE) || defined (_STLP_SGI_THREADS)
  // It should be relatively easy to get this to work on any modern Unix.
  volatile __stl_atomic_t _M_lock;
#endif

#if defined (_STLP_THREADS)
#  if defined (_STLP_ATOMIC_EXCHANGE)
  inline void _M_initialize() { _M_lock = 0; }
  inline void _M_destroy() {}

  void _M_acquire_lock() {
    _STLP_mutex_spin<0>::_M_do_lock(&_M_lock);
  }

  inline void _M_release_lock() {
    volatile __stl_atomic_t* __lock = &_M_lock;
#    if defined(_STLP_SGI_THREADS) && defined(__GNUC__) && __mips >= 3
    asm("sync");
    *__lock = 0;
#    elif defined(_STLP_SGI_THREADS) && __mips >= 3 && \
         (defined (_ABIN32) || defined(_ABI64))
    __lock_release(__lock);
#    elif defined (_STLP_SPARC_SOLARIS_THREADS)
#      if defined (__WORD64) || defined (__arch64__) || defined (__sparcv9) || defined (__sparcv8plus)
    asm("membar #StoreStore ; membar #LoadStore");
#      else
    asm(" stbar ");
#      endif
    *__lock = 0;
#    else
    *__lock = 0;
    // This is not sufficient on many multiprocessors, since
    // writes to protected variables and the lock may be reordered.
#    endif
  }
#  elif defined (_STLP_PTHREADS)
#    if defined (_STLP_USE_PTHREAD_SPINLOCK)
#      if !defined (__OpenBSD__)
  pthread_spinlock_t _M_lock;
  inline void _M_initialize() { pthread_spin_init( &_M_lock, 0 ); }
  inline void _M_destroy() { pthread_spin_destroy( &_M_lock ); }

  // sorry, but no static initializer for pthread_spinlock_t;
  // this will not work for compilers that has problems with call
  // constructor of static object...

  // _STLP_mutex_base()
  //   { pthread_spin_init( &_M_lock, 0 ); }

  // ~_STLP_mutex_base()
  //   { pthread_spin_destroy( &_M_lock ); }

  inline void _M_acquire_lock() { pthread_spin_lock( &_M_lock ); }
  inline void _M_release_lock() { pthread_spin_unlock( &_M_lock ); }
#      else // __OpenBSD__
  spinlock_t _M_lock;
  inline void _M_initialize() { _SPINLOCK_INIT( &_M_lock ); }
  inline void _M_destroy() { }
  inline void _M_acquire_lock() { _SPINLOCK( &_M_lock ); }
  inline void _M_release_lock() { _SPINUNLOCK( &_M_lock ); }
#      endif // __OpenBSD__
#    else // !_STLP_USE_PTHREAD_SPINLOCK
  pthread_mutex_t _M_lock;
  inline void _M_initialize()
  { pthread_mutex_init(&_M_lock,_STLP_PTHREAD_ATTR_DEFAULT); }
  inline void _M_destroy()
  { pthread_mutex_destroy(&_M_lock); }
  inline void _M_acquire_lock() {
#      if defined ( __hpux ) && ! defined (PTHREAD_MUTEX_INITIALIZER)
    if (!_M_lock.field1)  _M_initialize();
#      endif
    pthread_mutex_lock(&_M_lock);
  }
  inline void _M_release_lock() { pthread_mutex_unlock(&_M_lock); }
#    endif // !_STLP_USE_PTHREAD_SPINLOCK

#  elif defined (_STLP_UITHREADS)
  mutex_t _M_lock;
  inline void _M_initialize()
  { mutex_init(&_M_lock, 0, NULL); }
  inline void _M_destroy()
  { mutex_destroy(&_M_lock); }
  inline void _M_acquire_lock() { mutex_lock(&_M_lock); }
  inline void _M_release_lock() { mutex_unlock(&_M_lock); }

#  elif defined (_STLP_OS2THREADS)
  HMTX _M_lock;
  inline void _M_initialize() { DosCreateMutexSem(NULL, &_M_lock, 0, false); }
  inline void _M_destroy() { DosCloseMutexSem(_M_lock); }
  inline void _M_acquire_lock() {
    if (!_M_lock) _M_initialize();
    DosRequestMutexSem(_M_lock, SEM_INDEFINITE_WAIT);
  }
  inline void _M_release_lock() { DosReleaseMutexSem(_M_lock); }
#  elif defined (_STLP_BETHREADS)
  sem_id sem;
  inline void _M_initialize() {
    sem = create_sem(1, "STLPort");
    assert(sem > 0);
  }
  inline void _M_destroy() {
    int t = delete_sem(sem);
    assert(t == B_NO_ERROR);
  }
  inline void _M_acquire_lock();
  inline void _M_release_lock() {
    status_t t = release_sem(sem);
    assert(t == B_NO_ERROR);
  }
#  elif defined (_STLP_NWTHREADS)
  LONG _M_lock;
  inline void _M_initialize()
  { _M_lock = OpenLocalSemaphore(1); }
  inline void _M_destroy()
  { CloseLocalSemaphore(_M_lock); }
  inline void _M_acquire_lock()
  { WaitOnLocalSemaphore(_M_lock); }
  inline void _M_release_lock() { SignalLocalSemaphore(_M_lock); }
#  else      //*ty 11/24/2001 - added configuration check
#    error "Unknown thread facility configuration"
#  endif
#else /* No threads */
  inline void _M_initialize() {}
  inline void _M_destroy() {}
  inline void _M_acquire_lock() {}
  inline void _M_release_lock() {}
#endif // _STLP_PTHREADS
};

// Locking class.  The constructor initializes the lock, the destructor destroys it.
// Well - behaving class, does not need static initializer

class _STLP_CLASS_DECLSPEC _STLP_mutex : public _STLP_mutex_base {
  public:
    inline _STLP_mutex () { _M_initialize(); }
    inline ~_STLP_mutex () { _M_destroy(); }
  private:
    _STLP_mutex(const _STLP_mutex&);
    void operator=(const _STLP_mutex&);
};

// A locking class that uses _STLP_STATIC_MUTEX.  The constructor takes
// a reference to an _STLP_STATIC_MUTEX, and acquires a lock.  The destructor
// releases the lock.
// It's not clear that this is exactly the right functionality.
// It will probably change in the future.

struct _STLP_CLASS_DECLSPEC _STLP_auto_lock {
  _STLP_auto_lock(_STLP_STATIC_MUTEX& __lock) : _M_lock(__lock)
  { _M_lock._M_acquire_lock(); }
  ~_STLP_auto_lock()
  { _M_lock._M_release_lock(); }

private:
  _STLP_STATIC_MUTEX& _M_lock;
  void operator=(const _STLP_auto_lock&);
  _STLP_auto_lock(const _STLP_auto_lock&);
};

/*
 * Class _Refcount_Base provides a type, __stl_atomic_t, a data member,
 * _M_ref_count, and member functions _M_incr and _M_decr, which perform
 * atomic preincrement/predecrement.  The constructor initializes
 * _M_ref_count.
 */
class _STLP_CLASS_DECLSPEC _Refcount_Base {
  // The data member _M_ref_count
#if defined (__DMC__)
public:
#endif
  _STLP_VOLATILE __stl_atomic_t _M_ref_count;

#if defined (_STLP_THREADS) && \
   (!defined (_STLP_ATOMIC_INCREMENT) || !defined (_STLP_ATOMIC_DECREMENT) || \
    defined (_STLP_WIN95_LIKE))
#  define _STLP_USE_MUTEX
  _STLP_mutex _M_mutex;
#endif

  public:
  // Constructor
  _Refcount_Base(__stl_atomic_t __n) : _M_ref_count(__n) {}
#if defined (__BORLANDC__)
  ~_Refcount_Base(){};
#endif

  // _M_incr and _M_decr
#if defined (_STLP_THREADS)
#  if !defined (_STLP_USE_MUTEX)
   __stl_atomic_t _M_incr() { return _STLP_ATOMIC_INCREMENT(&_M_ref_count); }
   __stl_atomic_t _M_decr() { return _STLP_ATOMIC_DECREMENT(&_M_ref_count); }
#  else
#    undef _STLP_USE_MUTEX
  __stl_atomic_t _M_incr() {
    _STLP_auto_lock l(_M_mutex);
    return ++_M_ref_count;
  }
  __stl_atomic_t _M_decr() {
    _STLP_auto_lock l(_M_mutex);
    return --_M_ref_count;
  }
#  endif
#else  /* No threads */
  __stl_atomic_t _M_incr() { return ++_M_ref_count; }
  __stl_atomic_t _M_decr() { return --_M_ref_count; }
#endif
};

/* Atomic swap on __stl_atomic_t
 * This is guaranteed to behave as though it were atomic only if all
 * possibly concurrent updates use _Atomic_swap.
 * In some cases the operation is emulated with a lock.
 * Idem for _Atomic_swap_ptr
 */
/* Helper struct to handle following cases:
 * - on platforms where sizeof(__stl_atomic_t) == sizeof(void*) atomic
 *   exchange can be done on pointers
 * - on platform without atomic operation swap is done in a critical section,
 *   portable but inefficient.
 */
template <int __use_ptr_atomic_swap>
class _Atomic_swap_struct {
public:
#if defined (_STLP_THREADS) && \
    !defined (_STLP_ATOMIC_EXCHANGE) && \
    (defined (_STLP_PTHREADS) || defined (_STLP_UITHREADS) || defined (_STLP_OS2THREADS) || \
     defined (_STLP_USE_PTHREAD_SPINLOCK) || defined (_STLP_NWTHREADS))
#  define _STLP_USE_ATOMIC_SWAP_MUTEX
  static _STLP_STATIC_MUTEX _S_swap_lock;
#endif

  static __stl_atomic_t _S_swap(_STLP_VOLATILE __stl_atomic_t* __p, __stl_atomic_t __q) {
#if defined (_STLP_THREADS)
#  if defined (_STLP_ATOMIC_EXCHANGE)
  return _STLP_ATOMIC_EXCHANGE(__p, __q);
#  elif defined (_STLP_USE_ATOMIC_SWAP_MUTEX)
  _S_swap_lock._M_acquire_lock();
  __stl_atomic_t __result = *__p;
  *__p = __q;
  _S_swap_lock._M_release_lock();
  return __result;
#  else
#    error Missing atomic swap implementation
#  endif
#else
  /* no threads */
  __stl_atomic_t __result = *__p;
  *__p = __q;
  return __result;
#endif // _STLP_THREADS
  }

  static void* _S_swap_ptr(void* _STLP_VOLATILE* __p, void* __q) {
#if defined (_STLP_THREADS)
#  if defined (_STLP_ATOMIC_EXCHANGE_PTR)
  return _STLP_ATOMIC_EXCHANGE_PTR(__p, __q);
#  elif defined (_STLP_ATOMIC_EXCHANGE)
  _STLP_STATIC_ASSERT(sizeof(__stl_atomic_t) == sizeof(void*))
  return __REINTERPRET_CAST(void*, _STLP_ATOMIC_EXCHANGE(__REINTERPRET_CAST(volatile __stl_atomic_t*, __p),
                                                         __REINTERPRET_CAST(__stl_atomic_t, __q))
                            );
#  elif defined (_STLP_USE_ATOMIC_SWAP_MUTEX)
  _S_swap_lock._M_acquire_lock();
  void *__result = *__p;
  *__p = __q;
  _S_swap_lock._M_release_lock();
  return __result;
#  else
#    error Missing pointer atomic swap implementation
#  endif
#else
  /* no thread */
  void *__result = *__p;
  *__p = __q;
  return __result;
#endif
  }
};

_STLP_TEMPLATE_NULL
class _Atomic_swap_struct<0> {
public:
#if defined (_STLP_THREADS) && \
    (!defined (_STLP_ATOMIC_EXCHANGE) || !defined (_STLP_ATOMIC_EXCHANGE_PTR)) && \
    (defined (_STLP_PTHREADS) || defined (_STLP_UITHREADS) || defined (_STLP_OS2THREADS) || \
     defined (_STLP_USE_PTHREAD_SPINLOCK) || defined (_STLP_NWTHREADS))
#  define _STLP_USE_ATOMIC_SWAP_MUTEX
  static _STLP_STATIC_MUTEX _S_swap_lock;
#endif

  static __stl_atomic_t _S_swap(_STLP_VOLATILE __stl_atomic_t* __p, __stl_atomic_t __q) {
#if defined (_STLP_THREADS)
#  if defined (_STLP_ATOMIC_EXCHANGE)
  return _STLP_ATOMIC_EXCHANGE(__p, __q);
#  elif defined (_STLP_USE_ATOMIC_SWAP_MUTEX)
  /* This should be portable, but performance is expected
   * to be quite awful.  This really needs platform specific
   * code.
   */
  _S_swap_lock._M_acquire_lock();
  __stl_atomic_t __result = *__p;
  *__p = __q;
  _S_swap_lock._M_release_lock();
  return __result;
#  else
#    error Missing atomic swap implementation
#  endif
#else
  /* no threads */
  __stl_atomic_t __result = *__p;
  *__p = __q;
  return __result;
#endif // _STLP_THREADS
  }

  static void* _S_swap_ptr(void* _STLP_VOLATILE* __p, void* __q) {
#if defined (_STLP_THREADS)
#  if defined (_STLP_ATOMIC_EXCHANGE_PTR)
  return _STLP_ATOMIC_EXCHANGE_PTR(__p, __q);
#  elif defined (_STLP_ATOMIC_EXCHANGE)
  _STLP_STATIC_ASSERT(sizeof(__stl_atomic_t) == sizeof(void*))
  return __REINTERPRET_CAST(void*, _STLP_ATOMIC_EXCHANGE(__REINTERPRET_CAST(volatile __stl_atomic_t*, __p),
                                                         __REINTERPRET_CAST(__stl_atomic_t, __q))
                            );
#  elif defined (_STLP_USE_ATOMIC_SWAP_MUTEX)
  _S_swap_lock._M_acquire_lock();
  void *__result = *__p;
  *__p = __q;
  _S_swap_lock._M_release_lock();
  return __result;
#  else
#    error Missing pointer atomic swap implementation
#  endif
#else
  /* no thread */
  void *__result = *__p;
  *__p = __q;
  return __result;
#endif
  }
};

#if defined (_STLP_MSVC) && (_STLP_MSVC == 1300)
#  pragma warning (push)
#  pragma warning (disable : 4189) //__use_ptr_atomic_swap initialized but not used
#endif

inline __stl_atomic_t _STLP_CALL _Atomic_swap(_STLP_VOLATILE __stl_atomic_t * __p, __stl_atomic_t __q) {
  const int __use_ptr_atomic_swap = sizeof(__stl_atomic_t) == sizeof(void*);
  return _Atomic_swap_struct<__use_ptr_atomic_swap>::_S_swap(__p, __q);
}

inline void* _STLP_CALL _Atomic_swap_ptr(void* _STLP_VOLATILE* __p, void* __q) {
  const int __use_ptr_atomic_swap = sizeof(__stl_atomic_t) == sizeof(void*);
  return _Atomic_swap_struct<__use_ptr_atomic_swap>::_S_swap_ptr(__p, __q);
}

#if defined (_STLP_MSVC) && (_STLP_MSVC == 1300)
#  pragma warning (pop)
#endif

#if defined (_STLP_BETHREADS)
template <int __inst>
struct _STLP_beos_static_lock_data {
  static bool is_init;
  struct mutex_t : public _STLP_mutex {
    mutex_t()
    { _STLP_beos_static_lock_data<0>::is_init = true; }
    ~mutex_t()
    { _STLP_beos_static_lock_data<0>::is_init = false; }
  };
  static mutex_t mut;
};

template <int __inst>
bool _STLP_beos_static_lock_data<__inst>::is_init = false;
template <int __inst>
typename _STLP_beos_static_lock_data<__inst>::mutex_t _STLP_beos_static_lock_data<__inst>::mut;

inline void _STLP_mutex_base::_M_acquire_lock() {
  if (sem == 0) {
    // we need to initialise on demand here
    // to prevent race conditions use our global
    // mutex if it's available:
    if (_STLP_beos_static_lock_data<0>::is_init) {
      _STLP_auto_lock al(_STLP_beos_static_lock_data<0>::mut);
      if (sem == 0) _M_initialize();
    }
    else {
      // no lock available, we must still be
      // in startup code, THERE MUST BE ONE THREAD
      // ONLY active at this point.
      _M_initialize();
    }
  }
  status_t t;
  t = acquire_sem(sem);
  assert(t == B_NO_ERROR);
}
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_threads.c>
#endif

#endif /* _STLP_INTERNAL_THREADS_H */

// Local Variables:
// mode:C++
// End:
