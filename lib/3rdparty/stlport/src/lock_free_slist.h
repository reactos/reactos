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

#ifndef _STLP_LOCK_FREE_SLIST_H
#define _STLP_LOCK_FREE_SLIST_H

#if defined(_STLP_PTHREADS)
#  include <pthread.h>

#  if defined (__GNUC__) && defined (__i386__)

#    define _STLP_HAS_ATOMIC_FREELIST
/**
 * Class that implements a non-blocking and thread-safe freelist.
 * It is used for the lock-free node allocation engine.
 *
 * @author felixw@inin.com
 */
class _STLP_atomic_freelist {
public:
  /**
   * Type representing items of the freelist
   */
  struct item {
    item* _M_next;
  };

  _STLP_atomic_freelist() {
    // Statically assert layout of member is as expected by assembly code
    _STLP_STATIC_ASSERT(sizeof(_M) == 8)
    _M._M_data._M_top       = 0;
    _M._M_data._M_sequence  = 0;
  }

  /**
   * Atomically pushes the specified item onto the freelist.
   *
   * @param __item [in] Item to add to the front of the list
   */
  void push(item* __item) {
    // NOTE: GCC uses ebx as the PIC register for globals in shared libraries.
    //       The GCC version I'm using (3.4.1) won't temporarily spill it if it's
    //       used as input, output, or clobber.  Instead, it complains with a
    //       "can't find a register in class `BREG' while reloading `asm'" error.
    //       This is probably a compiler bug, but as the cmpxchg8b instruction
    //       requires ebx, I work around this here by using ecx for the '__item'
    //       input and spilling ebx into edi.  This also precludes us from using
    //       a "m" operand for the cmpxchg8b argument (GCC might think it can make
    //       it relative to ebx).  Instead, we're using esi for the address of _M_data.
    //
    int __tmp1;     // These dummy variables are used to tell GCC that the eax, ecx,
    int __tmp2;     // and edx registers will not have the same value as their input.
    int __tmp3;     // The optimizer will remove them as their values are not used.
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "       movl       %%ecx, %%ebx\n\t"
       "L1_%=: movl       %%eax, (%%ebx)\n\t"     // __item._M_next = _M._M_data._M_top
       "       leal       1(%%edx),%%ecx\n\t"     // new sequence = _M._M_data._M_sequence + 1
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              // Failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
       "       movl       %%edi, %%ebx"
      :"=a" (__tmp1), "=d" (__tmp2), "=c" (__tmp3)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "c" (__item), "S" (&_M._M_data)
      :"edi", "memory", "cc");
  }

  /**
   * Atomically removes the topmost item from the freelist and returns a
   * pointer to it.  Returns NULL if the list is empty.
   *
   * @return Item that was removed from front of list; NULL if list empty
   */
  item* pop() {
    item*   __result;
    int     __tmp;
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "L1_%=: testl      %%eax, %%eax\n\t"       // _M_top == NULL?
       "       je         L2_%=\n\t"              // If yes, we're done
       "       movl       (%%eax), %%ebx\n\t"     // new top = _M._M_data._M_top->_M_next
       "       leal       1(%%edx),%%ecx\n\t"     // new sequence = _M._M_data._M_sequence + 1
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              // We failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
       "L2_%=: movl       %%edi, %%ebx"
      :"=a" (__result), "=d" (__tmp)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "S" (&_M._M_data)
      :"edi", "ecx", "memory", "cc");
    return __result;
  }

  /**
   * Atomically detaches all items from the list and returns a pointer to the
   * topmost item.  The items are still chained and may be traversed safely as
   * they're now "owned" by the calling thread.
   *
   * @return Pointer to topmost item in the list; NULL if list empty
   */
  item* clear() {
    item*   __result;
    int     __tmp;
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "L1_%=: testl      %%eax, %%eax\n\t"       // _M_top == NULL?
       "       je         L2_%=\n\t"              // If yes, we're done
       "       xorl       %%ebx, %%ebx\n\t"       // We're attempting to set _M_top to NULL
       "       leal       1(%%edx),%%ecx\n\t"     // new sequence = _M._M_data._M_sequence + 1
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              // Failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
       "L2_%=: movl       %%edi, %%ebx"
      :"=a" (__result), "=d" (__tmp)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "S" (&_M._M_data)
      :"edi", "ecx", "memory", "cc");
    return __result;
  }

private:
    union {
      long long   _M_align;
      struct {
        item*           _M_top;         // Topmost element in the freelist
        unsigned int    _M_sequence;    // Sequence counter to prevent "ABA problem"
      } _M_data;
    } _M;

  _STLP_atomic_freelist(const _STLP_atomic_freelist&);
  _STLP_atomic_freelist& operator=(const _STLP_atomic_freelist&);
};

#  endif /* if defined(__GNUC__) && defined(__i386__) */

#elif defined (_STLP_WIN32THREADS)

#  if !defined (_WIN64)
#    define _STLP_USE_ASM_IMPLEMENTATION
#  endif

// Here are the compiler/platform requirements for the thread safe and
// lock free singly linked list implementation:
#  if defined (_STLP_USE_ASM_IMPLEMENTATION)
// For the asm version:
#    if defined (_STLP_MSVC) && defined (_M_IX86) && (_M_IX86 >= 500)
#      define _STLP_HAS_ATOMIC_FREELIST
#    endif
#  else
// For the API based version:
#    if defined (_STLP_NEW_PLATFORM_SDK) && (!defined (WINVER) || (WINVER >= 0x0501)) && \
                                            (!defined (_WIN32_WINNT) || (_WIN32_WINNT >= 0x0501))
#      define _STLP_HAS_ATOMIC_FREELIST
#    endif
#  endif

#  if defined (_STLP_HAS_ATOMIC_FREELIST)
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
#      if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) || defined (__ICL)
#        pragma warning (push)
#        pragma warning (disable : 4035) //function has no return value
#      endif
#    endif
/**
 * Class that implements a non-blocking and thread-safe freelist.
 * It is used for the lock-free node allocation engine.
 *
 * @author felixw@inin.com
 */
class _STLP_atomic_freelist {
public:
  /**
   * Type representing items of the freelist
   */
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
  struct item {
      item*   _M_next;
  };
#    else
  typedef SLIST_ENTRY item;
#    endif

  _STLP_atomic_freelist() {
    // Statically assert layout of member is as expected by assembly code
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    _STLP_STATIC_ASSERT((sizeof(item) == sizeof(item*)) && (sizeof(_M) == 8))
    _M._M_data._M_top       = 0;
    _M._M_data._M_sequence  = 0;
#    else
    InitializeSListHead(&_M_head);
#    endif
  }

  /**
   * Atomically pushes the specified item onto the freelist.
   *
   * @param __item [in] Item to add to the front of the list
   */
  void push(item* __item) {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             ebx, __item
        mov             eax, [esi]              // _M._M_data._M_top
        mov             edx, [esi+4]            // _M._M_data._M_sequence
    L1: mov             [ebx], eax              // __item._M_next = _M._M_data._M_top
        lea             ecx, [edx+1]            // new sequence = _M._M_data._M_sequence + 1
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      // Failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
    }
#    else
    InterlockedPushEntrySList(&_M_head, __item);
#    endif
  }

  /**
   * Atomically removes the topmost item from the freelist and returns a
   * pointer to it.  Returns NULL if the list is empty.
   *
   * @return Item that was removed from front of list; NULL if list empty
   */
  item* pop() {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             eax, [esi]              // _M._M_data._M_top
        mov             edx, [esi+4]            // _M._M_data._M_sequence
    L1: test            eax, eax                // _M_top == NULL?
        je              L2                      // Yes, we're done
        mov             ebx, [eax]              // new top = _M._M_data._M_top->_M_next
        lea             ecx, [edx+1]            // new sequence = _M._M_data._M_sequence + 1
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      // Failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
    L2:
    }
#    else
    return InterlockedPopEntrySList(&_M_head);
#    endif
  }

  /**
   * Atomically detaches all items from the list and returns pointer to the
   * topmost.  The items are still chained and may be traversed safely as
   * they're now "owned" by the calling thread.
   *
   * @return Pointer to topmost item in the list; NULL if list empty
   */
  item* clear() {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             eax, [esi]              // _M._M_data._M_top
        mov             edx, [esi+4]            // _M._M_data._M_sequence
    L1: test            eax, eax                // _M_top == NULL?
        je              L2                      // Yes, we're done
        xor             ebx,ebx                 // We're attempting to set _M._M_data._M_top to NULL
        lea             ecx, [edx+1]            // new sequence = _M._M_data._M_sequence + 1
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      // Failed, retry! (edx:eax now contain most recent _M_sequence:_M_top)
    L2:
    }
#    else
    return InterlockedFlushSList(&_M_head);
#    endif
  }

private:
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
  union {
    __int64     _M_align;
    struct {
      item*           _M_top;         // Topmost element in the freelist
      unsigned int    _M_sequence;    // Sequence counter to prevent "ABA problem"
    } _M_data;
  } _M;
#    else
  SLIST_HEADER _M_head;
#    endif

  _STLP_atomic_freelist(const _STLP_atomic_freelist&);
  _STLP_atomic_freelist& operator = (const _STLP_atomic_freelist&);
};

#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
#      if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) || defined (__ICL)
#        pragma warning (pop)
#      endif
#    endif

#  endif /* _STLP_HAS_ATOMIC_FREELIST */

#endif

#endif /* _STLP_LOCK_FREE_SLIST_H */
