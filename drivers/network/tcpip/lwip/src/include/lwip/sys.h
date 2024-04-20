/**
 * @file
 * OS abstraction layer
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 */

#ifndef LWIP_HDR_SYS_H
#define LWIP_HDR_SYS_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if NO_SYS

/* For a totally minimal and standalone system, we provide null
   definitions of the sys_ functions. */
typedef u8_t sys_sem_t;
typedef u8_t sys_mutex_t;
typedef u8_t sys_mbox_t;

#define sys_sem_new(s, c) ERR_OK
#define sys_sem_signal(s)
#define sys_sem_wait(s)
#define sys_arch_sem_wait(s,t)
#define sys_sem_free(s)
#define sys_sem_valid(s) 0
#define sys_sem_valid_val(s) 0
#define sys_sem_set_invalid(s)
#define sys_sem_set_invalid_val(s)
#define sys_mutex_new(mu) ERR_OK
#define sys_mutex_lock(mu)
#define sys_mutex_unlock(mu)
#define sys_mutex_free(mu)
#define sys_mutex_valid(mu) 0
#define sys_mutex_set_invalid(mu)
#define sys_mbox_new(m, s) ERR_OK
#define sys_mbox_fetch(m,d)
#define sys_mbox_tryfetch(m,d)
#define sys_mbox_post(m,d)
#define sys_mbox_trypost(m,d)
#define sys_mbox_free(m)
#define sys_mbox_valid(m)
#define sys_mbox_valid_val(m)
#define sys_mbox_set_invalid(m)
#define sys_mbox_set_invalid_val(m)

#define sys_thread_new(n,t,a,s,p)

#define sys_msleep(t)

#else /* NO_SYS */

/** Return code for timeouts from sys_arch_mbox_fetch and sys_arch_sem_wait */
#define SYS_ARCH_TIMEOUT 0xffffffffUL

/** sys_mbox_tryfetch() returns SYS_MBOX_EMPTY if appropriate.
 * For now we use the same magic value, but we allow this to change in future.
 */
#define SYS_MBOX_EMPTY SYS_ARCH_TIMEOUT

#include "lwip/err.h"
#include "arch/sys_arch.h"

/** Function prototype for thread functions */
typedef void (*lwip_thread_fn)(void *arg);

/* Function prototypes for functions to be implemented by platform ports
   (in sys_arch.c) */

/* Mutex functions: */

/** Define LWIP_COMPAT_MUTEX if the port has no mutexes and binary semaphores
    should be used instead */
#ifndef LWIP_COMPAT_MUTEX
#define LWIP_COMPAT_MUTEX 0
#endif

#if LWIP_COMPAT_MUTEX
/* for old ports that don't have mutexes: define them to binary semaphores */
#define sys_mutex_t                   sys_sem_t
#define sys_mutex_new(mutex)          sys_sem_new(mutex, 1)
#define sys_mutex_lock(mutex)         sys_sem_wait(mutex)
#define sys_mutex_unlock(mutex)       sys_sem_signal(mutex)
#define sys_mutex_free(mutex)         sys_sem_free(mutex)
#define sys_mutex_valid(mutex)        sys_sem_valid(mutex)
#define sys_mutex_set_invalid(mutex)  sys_sem_set_invalid(mutex)

#else /* LWIP_COMPAT_MUTEX */

/**
 * @ingroup sys_mutex
 * Create a new mutex.
 * Note that mutexes are expected to not be taken recursively by the lwIP code,
 * so both implementation types (recursive or non-recursive) should work.
 * The mutex is allocated to the memory that 'mutex'
 * points to (which can be both a pointer or the actual OS structure).
 * If the mutex has been created, ERR_OK should be returned. Returning any
 * other error will provide a hint what went wrong, but except for assertions,
 * no real error handling is implemented.
 *
 * @param mutex pointer to the mutex to create
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_mutex_new(sys_mutex_t *mutex);
/**
 * @ingroup sys_mutex
 * Blocks the thread until the mutex can be grabbed.
 * @param mutex the mutex to lock
 */
void sys_mutex_lock(sys_mutex_t *mutex);
/**
 * @ingroup sys_mutex
 * Releases the mutex previously locked through 'sys_mutex_lock()'.
 * @param mutex the mutex to unlock
 */
void sys_mutex_unlock(sys_mutex_t *mutex);
/**
 * @ingroup sys_mutex
 * Deallocates a mutex.
 * @param mutex the mutex to delete
 */
void sys_mutex_free(sys_mutex_t *mutex);
#ifndef sys_mutex_valid
/**
 * @ingroup sys_mutex
 * Returns 1 if the mutes is valid, 0 if it is not valid.
 * When using pointers, a simple way is to check the pointer for != NULL.
 * When directly using OS structures, implementing this may be more complex.
 * This may also be a define, in which case the function is not prototyped.
 */
int sys_mutex_valid(sys_mutex_t *mutex);
#endif
#ifndef sys_mutex_set_invalid
/**
 * @ingroup sys_mutex
 * Invalidate a mutex so that sys_mutex_valid() returns 0.
 * ATTENTION: This does NOT mean that the mutex shall be deallocated:
 * sys_mutex_free() is always called before calling this function!
 * This may also be a define, in which case the function is not prototyped.
 */
void sys_mutex_set_invalid(sys_mutex_t *mutex);
#endif
#endif /* LWIP_COMPAT_MUTEX */

/* Semaphore functions: */

/**
 * @ingroup sys_sem
 * Create a new semaphore
 * Creates a new semaphore. The semaphore is allocated to the memory that 'sem'
 * points to (which can be both a pointer or the actual OS structure).
 * The "count" argument specifies the initial state of the semaphore (which is
 * either 0 or 1).
 * If the semaphore has been created, ERR_OK should be returned. Returning any
 * other error will provide a hint what went wrong, but except for assertions,
 * no real error handling is implemented.
 *
 * @param sem pointer to the semaphore to create
 * @param count initial count of the semaphore
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count);
/**
 * @ingroup sys_sem
 * Signals a semaphore
 * @param sem the semaphore to signal
 */
void sys_sem_signal(sys_sem_t *sem);
/**
 * @ingroup sys_sem
 *  Blocks the thread while waiting for the semaphore to be signaled. If the
 * "timeout" argument is non-zero, the thread should only be blocked for the
 * specified time (measured in milliseconds). If the "timeout" argument is zero,
 * the thread should be blocked until the semaphore is signalled.
 *
 * The return value is SYS_ARCH_TIMEOUT if the semaphore wasn't signaled within
 * the specified time or any other value if it was signaled (with or without
 * waiting).
 * Notice that lwIP implements a function with a similar name,
 * sys_sem_wait(), that uses the sys_arch_sem_wait() function.
 *
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return SYS_ARCH_TIMEOUT on timeout, any other value on success
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout);
/**
 * @ingroup sys_sem
 * Deallocates a semaphore.
 * @param sem semaphore to delete
 */
void sys_sem_free(sys_sem_t *sem);
/** Wait for a semaphore - forever/no timeout */
#define sys_sem_wait(sem)                  sys_arch_sem_wait(sem, 0)
#ifndef sys_sem_valid
/**
 * @ingroup sys_sem
 * Returns 1 if the semaphore is valid, 0 if it is not valid.
 * When using pointers, a simple way is to check the pointer for != NULL.
 * When directly using OS structures, implementing this may be more complex.
 * This may also be a define, in which case the function is not prototyped.
 */
int sys_sem_valid(sys_sem_t *sem);
#endif
#ifndef sys_sem_set_invalid
/**
 * @ingroup sys_sem
 * Invalidate a semaphore so that sys_sem_valid() returns 0.
 * ATTENTION: This does NOT mean that the semaphore shall be deallocated:
 * sys_sem_free() is always called before calling this function!
 * This may also be a define, in which case the function is not prototyped.
 */
void sys_sem_set_invalid(sys_sem_t *sem);
#endif
#ifndef sys_sem_valid_val
/**
 * Same as sys_sem_valid() but taking a value, not a pointer
 */
#define sys_sem_valid_val(sem)       sys_sem_valid(&(sem))
#endif
#ifndef sys_sem_set_invalid_val
/**
 * Same as sys_sem_set_invalid() but taking a value, not a pointer
 */
#define sys_sem_set_invalid_val(sem) sys_sem_set_invalid(&(sem))
#endif

#ifndef sys_msleep
/**
 * @ingroup sys_misc
 * Sleep for specified number of ms
 */
void sys_msleep(u32_t ms); /* only has a (close to) 1 ms resolution. */
#endif

/* Mailbox functions. */

/**
 * @ingroup sys_mbox
 * Creates an empty mailbox for maximum "size" elements. Elements stored
 * in mailboxes are pointers. You have to define macros "_MBOX_SIZE"
 * in your lwipopts.h, or ignore this parameter in your implementation
 * and use a default size.
 * If the mailbox has been created, ERR_OK should be returned. Returning any
 * other error will provide a hint what went wrong, but except for assertions,
 * no real error handling is implemented.
 *
 * @param mbox pointer to the mbox to create
 * @param size (minimum) number of messages in this mbox
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size);
/**
 * @ingroup sys_mbox
 * Post a message to an mbox - may not fail
 * -> blocks if full, only to be used from tasks NOT from ISR!
 *
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg);
/**
 * @ingroup sys_mbox
 * Try to post a message to an mbox - may fail if full.
 * Can be used from ISR (if the sys arch layer allows this).
 * Returns ERR_MEM if it is full, else, ERR_OK if the "msg" is posted.
 *
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg);
/**
 * @ingroup sys_mbox
 * Try to post a message to an mbox - may fail if full.
 * To be be used from ISR.
 * Returns ERR_MEM if it is full, else, ERR_OK if the "msg" is posted.
 *
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg);
/**
 * @ingroup sys_mbox
 * Blocks the thread until a message arrives in the mailbox, but does
 * not block the thread longer than "timeout" milliseconds (similar to
 * the sys_arch_sem_wait() function). If "timeout" is 0, the thread should
 * be blocked until a message arrives. The "msg" argument is a result
 * parameter that is set by the function (i.e., by doing "*msg =
 * ptr"). The "msg" parameter maybe NULL to indicate that the message
 * should be dropped.
 * The return values are the same as for the sys_arch_sem_wait() function:
 * SYS_ARCH_TIMEOUT if there was a timeout, any other value if a messages
 * is received.
 *
 * Note that a function with a similar name, sys_mbox_fetch(), is
 * implemented by lwIP.
 *
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message (0 = wait forever)
 * @return SYS_ARCH_TIMEOUT on timeout, any other value if a message has been received
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout);
/* Allow port to override with a macro, e.g. special timeout for sys_arch_mbox_fetch() */
#ifndef sys_arch_mbox_tryfetch
/**
 * @ingroup sys_mbox
 * This is similar to sys_arch_mbox_fetch, however if a message is not
 * present in the mailbox, it immediately returns with the code
 * SYS_MBOX_EMPTY. On success 0 is returned.
 * To allow for efficient implementations, this can be defined as a
 * function-like macro in sys_arch.h instead of a normal function. For
 * example, a naive implementation could be:
 * \#define sys_arch_mbox_tryfetch(mbox,msg) sys_arch_mbox_fetch(mbox,msg,1)
 * although this would introduce unnecessary delays.
 *
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg);
#endif
/**
 * For now, we map straight to sys_arch implementation.
 */
#define sys_mbox_tryfetch(mbox, msg) sys_arch_mbox_tryfetch(mbox, msg)
/**
 * @ingroup sys_mbox
 * Deallocates a mailbox. If there are messages still present in the
 * mailbox when the mailbox is deallocated, it is an indication of a
 * programming error in lwIP and the developer should be notified.
 *
 * @param mbox mbox to delete
 */
void sys_mbox_free(sys_mbox_t *mbox);
#define sys_mbox_fetch(mbox, msg) sys_arch_mbox_fetch(mbox, msg, 0)
#ifndef sys_mbox_valid
/**
 * @ingroup sys_mbox
 * Returns 1 if the mailbox is valid, 0 if it is not valid.
 * When using pointers, a simple way is to check the pointer for != NULL.
 * When directly using OS structures, implementing this may be more complex.
 * This may also be a define, in which case the function is not prototyped.
 */
int sys_mbox_valid(sys_mbox_t *mbox);
#endif
#ifndef sys_mbox_set_invalid
/**
 * @ingroup sys_mbox
 * Invalidate a mailbox so that sys_mbox_valid() returns 0.
 * ATTENTION: This does NOT mean that the mailbox shall be deallocated:
 * sys_mbox_free() is always called before calling this function!
 * This may also be a define, in which case the function is not prototyped.
 */
void sys_mbox_set_invalid(sys_mbox_t *mbox);
#endif
#ifndef sys_mbox_valid_val
/**
 * Same as sys_mbox_valid() but taking a value, not a pointer
 */
#define sys_mbox_valid_val(mbox)       sys_mbox_valid(&(mbox))
#endif
#ifndef sys_mbox_set_invalid_val
/**
 * Same as sys_mbox_set_invalid() but taking a value, not a pointer
 */
#define sys_mbox_set_invalid_val(mbox) sys_mbox_set_invalid(&(mbox))
#endif


/**
 * @ingroup sys_misc
 * The only thread function:
 * Starts a new thread named "name" with priority "prio" that will begin its
 * execution in the function "thread()". The "arg" argument will be passed as an
 * argument to the thread() function. The stack size to used for this thread is
 * the "stacksize" parameter. The id of the new thread is returned. Both the id
 * and the priority are system dependent.
 * ATTENTION: although this function returns a value, it MUST NOT FAIL (ports have to assert this!)
 *
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio);

#endif /* NO_SYS */

/**
 * @ingroup lwip_opts_lock
 * Called as first thing in the lwIP TCPIP thread. Can be used in conjunction
 * with @ref LWIP_ASSERT_CORE_LOCKED to check core locking.
 * @see @ref multithreading
 */
#if !defined LWIP_MARK_TCPIP_THREAD || defined __DOXYGEN__
#define LWIP_MARK_TCPIP_THREAD()
#endif

/**
 * @ingroup sys_misc
 * sys_init() must be called before anything else.
 * Initialize the sys_arch layer.
 */
void sys_init(void);

#ifndef sys_jiffies
/**
 * Ticks/jiffies since power up.
 */
u32_t sys_jiffies(void);
#endif

#ifdef LWIP_FUZZ_SYS_NOW
/* This offset should be added to the time 'sys_now()' returns */
extern u32_t sys_now_offset;
#endif

/**
 * @ingroup sys_time
 * Returns the current time in milliseconds,
 * may be the same as sys_jiffies or at least based on it.
 * Don't care for wraparound, this is only used for time diffs.
 * Not implementing this function means you cannot use some modules (e.g. TCP
 * timestamps, internal timeouts for NO_SYS==1).
 */
u32_t sys_now(void);

/* Critical Region Protection */
/* These functions must be implemented in the sys_arch.c file.
   In some implementations they can provide a more light-weight protection
   mechanism than using semaphores. Otherwise semaphores can be used for
   implementation */
#ifndef SYS_ARCH_PROTECT
/** SYS_LIGHTWEIGHT_PROT
 * define SYS_LIGHTWEIGHT_PROT in lwipopts.h if you want inter-task protection
 * for certain critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#if SYS_LIGHTWEIGHT_PROT

/**
 * @ingroup sys_prot
 * SYS_ARCH_DECL_PROTECT
 * declare a protection variable. This macro will default to defining a variable of
 * type sys_prot_t. If a particular port needs a different implementation, then
 * this macro may be defined in sys_arch.h.
 */
#define SYS_ARCH_DECL_PROTECT(lev) sys_prot_t lev
/**
 * @ingroup sys_prot
 * SYS_ARCH_PROTECT
 * Perform a "fast" protect. This could be implemented by
 * disabling interrupts for an embedded system or by using a semaphore or
 * mutex. The implementation should allow calling SYS_ARCH_PROTECT when
 * already protected. The old protection level is returned in the variable
 * "lev". This macro will default to calling the sys_arch_protect() function
 * which should be implemented in sys_arch.c. If a particular port needs a
 * different implementation, then this macro may be defined in sys_arch.h
 */
#define SYS_ARCH_PROTECT(lev) lev = sys_arch_protect()
/**
 * @ingroup sys_prot
 * SYS_ARCH_UNPROTECT
 * Perform a "fast" set of the protection level to "lev". This could be
 * implemented by setting the interrupt level to "lev" within the MACRO or by
 * using a semaphore or mutex.  This macro will default to calling the
 * sys_arch_unprotect() function which should be implemented in
 * sys_arch.c. If a particular port needs a different implementation, then
 * this macro may be defined in sys_arch.h
 */
#define SYS_ARCH_UNPROTECT(lev) sys_arch_unprotect(lev)
sys_prot_t sys_arch_protect(void);
void sys_arch_unprotect(sys_prot_t pval);

#else

#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev)
#define SYS_ARCH_UNPROTECT(lev)

#endif /* SYS_LIGHTWEIGHT_PROT */

#endif /* SYS_ARCH_PROTECT */

/*
 * Macros to set/get and increase/decrease variables in a thread-safe way.
 * Use these for accessing variable that are used from more than one thread.
 */

#ifndef SYS_ARCH_INC
#define SYS_ARCH_INC(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var += val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_INC */

#ifndef SYS_ARCH_DEC
#define SYS_ARCH_DEC(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var -= val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_DEC */

#ifndef SYS_ARCH_GET
#define SYS_ARCH_GET(var, ret) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                ret = var; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_GET */

#ifndef SYS_ARCH_SET
#define SYS_ARCH_SET(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var = val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_SET */

#ifndef SYS_ARCH_LOCKED
#define SYS_ARCH_LOCKED(code) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                code; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_LOCKED */


#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_SYS_H */
