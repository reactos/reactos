/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 *         Simon Goldschmidt
 *
 */

#include <stdlib.h>
#include <stdio.h> /* sprintf() for task names */

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif
#include <windows.h>
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#include <time.h>

#include <lwip/opt.h>
#include <lwip/arch.h>
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>
#include <lwip/tcpip.h>

/** Set this to 1 to enable assertion checks that SYS_ARCH_PROTECT() is only
 * called once in a call stack (calling it nested might cause trouble in some
 * implementations, so let's avoid this in core code as long as we can).
 */
#ifndef LWIP_SYS_ARCH_CHECK_NESTED_PROTECT
#define LWIP_SYS_ARCH_CHECK_NESTED_PROTECT 1
#endif

/** Set this to 1 to enable assertion checks that SYS_ARCH_PROTECT() is *not*
 * called before functions potentiolly involving the OS scheduler.
 *
 * This scheme is currently broken only for non-core-locking when waking up
 * threads waiting on a socket via select/poll.
 */
#ifndef LWIP_SYS_ARCH_CHECK_SCHEDULING_UNPROTECTED
#define LWIP_SYS_ARCH_CHECK_SCHEDULING_UNPROTECTED LWIP_TCPIP_CORE_LOCKING
#endif

#define LWIP_WIN32_SYS_ARCH_ENABLE_PROTECT_COUNTER (LWIP_SYS_ARCH_CHECK_NESTED_PROTECT || LWIP_SYS_ARCH_CHECK_SCHEDULING_UNPROTECTED)

/* These functions are used from NO_SYS also, for precise timer triggering */
static LARGE_INTEGER freq, sys_start_time;
#define SYS_INITIALIZED() (freq.QuadPart != 0)

static DWORD netconn_sem_tls_index;

static HCRYPTPROV hcrypt;

static void
sys_win_rand_init(void)
{
  if (!CryptAcquireContext(&hcrypt, NULL, NULL, PROV_RSA_FULL, 0)) {
    DWORD err = GetLastError();
    LWIP_PLATFORM_DIAG(("CryptAcquireContext failed with error %d, trying to create NEWKEYSET", (int)err));
    if(!CryptAcquireContext(&hcrypt, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
      char errbuf[128];
      err = GetLastError();
      snprintf(errbuf, sizeof(errbuf), "CryptAcquireContext failed with error %d", (int)err);
      LWIP_UNUSED_ARG(err);
      LWIP_ASSERT(errbuf, 0);
    }
  }
}

unsigned int
lwip_port_rand(void)
{
  u32_t ret;
  if (CryptGenRandom(hcrypt, sizeof(ret), (BYTE*)&ret)) {
    return ret;
  }
  /* maybe CryptAcquireContext has not been called... */
  sys_win_rand_init();
  if (CryptGenRandom(hcrypt, sizeof(ret), (BYTE*)&ret)) {
    return ret;
  }
  LWIP_ASSERT("CryptGenRandom failed", 0);
  return 0;
}

static void
sys_init_timing(void)
{
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&sys_start_time);
}

static LONGLONG
sys_get_ms_longlong(void)
{
  LONGLONG ret;
  LARGE_INTEGER now;
#if NO_SYS
  if (!SYS_INITIALIZED()) {
    sys_init();
    LWIP_ASSERT("initialization failed", SYS_INITIALIZED());
  }
#endif /* NO_SYS */
  QueryPerformanceCounter(&now);
  ret = now.QuadPart-sys_start_time.QuadPart;
  return (u32_t)(((ret)*1000)/freq.QuadPart);
}

u32_t
sys_jiffies(void)
{
  return (u32_t)sys_get_ms_longlong();
}

u32_t
sys_now(void)
{
  u32_t now = (u32_t)sys_get_ms_longlong();
#ifdef LWIP_FUZZ_SYS_NOW
  now += sys_now_offset;
#endif
  return now;
}

CRITICAL_SECTION critSec;
#if LWIP_WIN32_SYS_ARCH_ENABLE_PROTECT_COUNTER
static int protection_depth;
#endif

static void
InitSysArchProtect(void)
{
  InitializeCriticalSection(&critSec);
}

sys_prot_t
sys_arch_protect(void)
{
#if NO_SYS
  if (!SYS_INITIALIZED()) {
    sys_init();
    LWIP_ASSERT("initialization failed", SYS_INITIALIZED());
  }
#endif
  EnterCriticalSection(&critSec);
#if LWIP_SYS_ARCH_CHECK_NESTED_PROTECT
  LWIP_ASSERT("nested SYS_ARCH_PROTECT", protection_depth == 0);
#endif
#if LWIP_WIN32_SYS_ARCH_ENABLE_PROTECT_COUNTER
  protection_depth++;
#endif
  return 0;
}

void
sys_arch_unprotect(sys_prot_t pval)
{
  LWIP_UNUSED_ARG(pval);
#if LWIP_SYS_ARCH_CHECK_NESTED_PROTECT
  LWIP_ASSERT("missing SYS_ARCH_PROTECT", protection_depth == 1);
#else
  LWIP_ASSERT("missing SYS_ARCH_PROTECT", protection_depth > 0);
#endif
#if LWIP_WIN32_SYS_ARCH_ENABLE_PROTECT_COUNTER
  protection_depth--;
#endif
  LeaveCriticalSection(&critSec);
}

#if LWIP_SYS_ARCH_CHECK_SCHEDULING_UNPROTECTED
/** This checks that SYS_ARCH_PROTECT() hasn't been called by protecting
 * and then checking the level
 */
static void
sys_arch_check_not_protected(void)
{
  sys_arch_protect();
  LWIP_ASSERT("SYS_ARCH_PROTECT before scheduling", protection_depth == 1);
  sys_arch_unprotect(0);
}
#else
#define sys_arch_check_not_protected()
#endif

static void
msvc_sys_init(void)
{
  sys_win_rand_init();
  sys_init_timing();
  InitSysArchProtect();
  netconn_sem_tls_index = TlsAlloc();
  LWIP_ASSERT("TlsAlloc failed", netconn_sem_tls_index != TLS_OUT_OF_INDEXES);
}

void
sys_init(void)
{
  msvc_sys_init();
}

#if !NO_SYS

struct threadlist {
  lwip_thread_fn function;
  void *arg;
  DWORD id;
  struct threadlist *next;
};

static struct threadlist *lwip_win32_threads = NULL;

err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
  HANDLE new_sem = NULL;

  LWIP_ASSERT("sem != NULL", sem != NULL);

  new_sem = CreateSemaphore(0, count, 100000, 0);
  LWIP_ASSERT("Error creating semaphore", new_sem != NULL);
  if(new_sem != NULL) {
    if (SYS_INITIALIZED()) {
      SYS_ARCH_LOCKED(SYS_STATS_INC_USED(sem));
    } else {
      SYS_STATS_INC_USED(sem);
    }
#if LWIP_STATS && SYS_STATS
    LWIP_ASSERT("sys_sem_new() counter overflow", lwip_stats.sys.sem.used != 0);
#endif /* LWIP_STATS && SYS_STATS*/
    sem->sem = new_sem;
    return ERR_OK;
  }

  /* failed to allocate memory... */
  if (SYS_INITIALIZED()) {
    SYS_ARCH_LOCKED(SYS_STATS_INC(sem.err));
  } else {
    SYS_STATS_INC(sem.err);
  }
  sem->sem = NULL;
  return ERR_MEM;
}

void
sys_sem_free(sys_sem_t *sem)
{
  /* parameter check */
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);
  LWIP_ASSERT("sem->sem != INVALID_HANDLE_VALUE", sem->sem != INVALID_HANDLE_VALUE);
  CloseHandle(sem->sem);

  SYS_ARCH_LOCKED(SYS_STATS_DEC(sem.used));
#if LWIP_STATS && SYS_STATS
  LWIP_ASSERT("sys_sem_free() closed more than created", lwip_stats.sys.sem.used != (u16_t)-1);
#endif /* LWIP_STATS && SYS_STATS */
  sem->sem = NULL;
}

u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  DWORD ret;
  LONGLONG starttime, endtime;
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);
  LWIP_ASSERT("sem->sem != INVALID_HANDLE_VALUE", sem->sem != INVALID_HANDLE_VALUE);
  if (!timeout) {
    /* wait infinite */
    starttime = sys_get_ms_longlong();
    ret = WaitForSingleObject(sem->sem, INFINITE);
    LWIP_ASSERT("Error waiting for semaphore", ret == WAIT_OBJECT_0);
    endtime = sys_get_ms_longlong();
    /* return the time we waited for the sem */
    return (u32_t)(endtime - starttime);
  } else {
    starttime = sys_get_ms_longlong();
    ret = WaitForSingleObject(sem->sem, timeout);
    LWIP_ASSERT("Error waiting for semaphore", (ret == WAIT_OBJECT_0) || (ret == WAIT_TIMEOUT));
    if (ret == WAIT_OBJECT_0) {
      endtime = sys_get_ms_longlong();
      /* return the time we waited for the sem */
      return (u32_t)(endtime - starttime);
    } else {
      /* timeout */
      return SYS_ARCH_TIMEOUT;
    }
  }
}

void
sys_sem_signal(sys_sem_t *sem)
{
  BOOL ret;
  sys_arch_check_not_protected();
  LWIP_ASSERT("sem != NULL", sem != NULL);
  LWIP_ASSERT("sem->sem != NULL", sem->sem != NULL);
  LWIP_ASSERT("sem->sem != INVALID_HANDLE_VALUE", sem->sem != INVALID_HANDLE_VALUE);
  ret = ReleaseSemaphore(sem->sem, 1, NULL);
  LWIP_ASSERT("Error releasing semaphore", ret != 0);
  LWIP_UNUSED_ARG(ret);
}

err_t
sys_mutex_new(sys_mutex_t *mutex)
{
  HANDLE new_mut = NULL;

  LWIP_ASSERT("mutex != NULL", mutex != NULL);

  new_mut = CreateMutex(NULL, FALSE, NULL);
  LWIP_ASSERT("Error creating mutex", new_mut != NULL);
  if (new_mut != NULL) {
    SYS_ARCH_LOCKED(SYS_STATS_INC_USED(mutex));
#if LWIP_STATS && SYS_STATS
    LWIP_ASSERT("sys_mutex_new() counter overflow", lwip_stats.sys.mutex.used != 0);
#endif /* LWIP_STATS && SYS_STATS*/
    mutex->mut = new_mut;
    return ERR_OK;
  }

  /* failed to allocate memory... */
  SYS_ARCH_LOCKED(SYS_STATS_INC(mutex.err));
  mutex->mut = NULL;
  return ERR_MEM;
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
  /* parameter check */
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);
  LWIP_ASSERT("mutex->mut != INVALID_HANDLE_VALUE", mutex->mut != INVALID_HANDLE_VALUE);
  CloseHandle(mutex->mut);

  SYS_ARCH_LOCKED(SYS_STATS_DEC(mutex.used));
#if LWIP_STATS && SYS_STATS
  LWIP_ASSERT("sys_mutex_free() closed more than created", lwip_stats.sys.mutex.used != (u16_t)-1);
#endif /* LWIP_STATS && SYS_STATS */
  mutex->mut = NULL;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
  DWORD ret;
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);
  LWIP_ASSERT("mutex->mut != INVALID_HANDLE_VALUE", mutex->mut != INVALID_HANDLE_VALUE);
  /* wait infinite */
  ret = WaitForSingleObject(mutex->mut, INFINITE);
  LWIP_ASSERT("Error waiting for mutex", ret == WAIT_OBJECT_0);
  LWIP_UNUSED_ARG(ret);
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
  sys_arch_check_not_protected();
  LWIP_ASSERT("mutex != NULL", mutex != NULL);
  LWIP_ASSERT("mutex->mut != NULL", mutex->mut != NULL);
  LWIP_ASSERT("mutex->mut != INVALID_HANDLE_VALUE", mutex->mut != INVALID_HANDLE_VALUE);
  /* wait infinite */
  if (!ReleaseMutex(mutex->mut)) {
    LWIP_ASSERT("Error releasing mutex", 0);
  }
}


#ifdef _MSC_VER
const DWORD MS_VC_EXCEPTION=0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType; /* Must be 0x1000. */
  LPCSTR szName; /* Pointer to name (in user addr space). */
  DWORD dwThreadID; /* Thread ID (-1=caller thread). */
  DWORD dwFlags; /* Reserved for future use, must be zero. */
} THREADNAME_INFO;
#pragma pack(pop)

static void
SetThreadName(DWORD dwThreadID, const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags = 0;

  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  }
  __except(EXCEPTION_EXECUTE_HANDLER) {
  }
}
#else /* _MSC_VER */
static void
SetThreadName(DWORD dwThreadID, const char* threadName)
{
  LWIP_UNUSED_ARG(dwThreadID);
  LWIP_UNUSED_ARG(threadName);
}
#endif /* _MSC_VER */

static DWORD WINAPI
sys_thread_function(void* arg)
{
  struct threadlist* t = (struct threadlist*)arg;
#if LWIP_NETCONN_SEM_PER_THREAD
  sys_arch_netconn_sem_alloc();
#endif
  t->function(t->arg);
#if LWIP_NETCONN_SEM_PER_THREAD
  sys_arch_netconn_sem_free();
#endif
  return 0;
}

sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
  struct threadlist *new_thread;
  HANDLE h;
  SYS_ARCH_DECL_PROTECT(lev);

  LWIP_UNUSED_ARG(name);
  LWIP_UNUSED_ARG(stacksize);
  LWIP_UNUSED_ARG(prio);

  new_thread = (struct threadlist*)malloc(sizeof(struct threadlist));
  LWIP_ASSERT("new_thread != NULL", new_thread != NULL);
  if (new_thread != NULL) {
    new_thread->function = function;
    new_thread->arg = arg;
    SYS_ARCH_PROTECT(lev);
    new_thread->next = lwip_win32_threads;
    lwip_win32_threads = new_thread;

    h = CreateThread(0, 0, sys_thread_function, new_thread, 0, &(new_thread->id));
    LWIP_ASSERT("h != 0", h != 0);
    LWIP_ASSERT("h != -1", h != INVALID_HANDLE_VALUE);
    LWIP_UNUSED_ARG(h);
    SetThreadName(new_thread->id, name);

    SYS_ARCH_UNPROTECT(lev);
    return new_thread->id;
  }
  return 0;
}

#if !NO_SYS
#if LWIP_TCPIP_CORE_LOCKING

static DWORD lwip_core_lock_holder_thread_id;

void
sys_lock_tcpip_core(void)
{
  sys_mutex_lock(&lock_tcpip_core);
  lwip_core_lock_holder_thread_id = GetCurrentThreadId();
}

void
sys_unlock_tcpip_core(void)
{
  lwip_core_lock_holder_thread_id = 0;
  sys_mutex_unlock(&lock_tcpip_core);
}
#endif /* LWIP_TCPIP_CORE_LOCKING */

static DWORD lwip_tcpip_thread_id;

void
sys_mark_tcpip_thread(void)
{
  lwip_tcpip_thread_id = GetCurrentThreadId();
}

void
sys_check_core_locking(void)
{
  /* Embedded systems should check we are NOT in an interrupt context here */

  if (lwip_tcpip_thread_id != 0) {
    DWORD current_thread_id = GetCurrentThreadId();

#if LWIP_TCPIP_CORE_LOCKING
    LWIP_ASSERT("Function called without core lock", current_thread_id == lwip_core_lock_holder_thread_id);
#else /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_ASSERT("Function called from wrong thread", current_thread_id == lwip_tcpip_thread_id);
#endif /* LWIP_TCPIP_CORE_LOCKING */
    LWIP_UNUSED_ARG(current_thread_id); /* for LWIP_NOASSERT */
  }
}
#endif /* !NO_SYS */

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_UNUSED_ARG(size);

  mbox->sem = CreateSemaphore(0, 0, MAX_QUEUE_ENTRIES, 0);
  LWIP_ASSERT("Error creating semaphore", mbox->sem != NULL);
  if (mbox->sem == NULL) {
    SYS_ARCH_LOCKED(SYS_STATS_INC(mbox.err));
    return ERR_MEM;
  }
  memset(&mbox->q_mem, 0, sizeof(u32_t)*MAX_QUEUE_ENTRIES);
  mbox->head = 0;
  mbox->tail = 0;
  SYS_ARCH_LOCKED(SYS_STATS_INC_USED(mbox));
#if LWIP_STATS && SYS_STATS
  LWIP_ASSERT("sys_mbox_new() counter overflow", lwip_stats.sys.mbox.used != 0);
#endif /* LWIP_STATS && SYS_STATS */
  return ERR_OK;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
  /* parameter check */
  LWIP_ASSERT("mbox != NULL", mbox != NULL);
  LWIP_ASSERT("mbox->sem != NULL", mbox->sem != NULL);
  LWIP_ASSERT("mbox->sem != INVALID_HANDLE_VALUE", mbox->sem != INVALID_HANDLE_VALUE);

  CloseHandle(mbox->sem);

  SYS_STATS_DEC(mbox.used);
#if LWIP_STATS && SYS_STATS
  LWIP_ASSERT( "sys_mbox_free() ", lwip_stats.sys.mbox.used != (u16_t)-1);
#endif /* LWIP_STATS && SYS_STATS */
  mbox->sem = NULL;
}

void
sys_mbox_post(sys_mbox_t *q, void *msg)
{
  BOOL ret;
  SYS_ARCH_DECL_PROTECT(lev);
  sys_arch_check_not_protected();

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);
  LWIP_ASSERT("q->sem != INVALID_HANDLE_VALUE", q->sem != INVALID_HANDLE_VALUE);

  SYS_ARCH_PROTECT(lev);
  q->q_mem[q->head] = msg;
  q->head++;
  if (q->head >= MAX_QUEUE_ENTRIES) {
    q->head = 0;
  }
  LWIP_ASSERT("mbox is full!", q->head != q->tail);
  ret = ReleaseSemaphore(q->sem, 1, 0);
  LWIP_ASSERT("Error releasing sem", ret != 0);
  LWIP_UNUSED_ARG(ret);

  SYS_ARCH_UNPROTECT(lev);
}

err_t
sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
  u32_t new_head;
  BOOL ret;
  SYS_ARCH_DECL_PROTECT(lev);
  sys_arch_check_not_protected();

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);
  LWIP_ASSERT("q->sem != INVALID_HANDLE_VALUE", q->sem != INVALID_HANDLE_VALUE);

  SYS_ARCH_PROTECT(lev);

  new_head = q->head + 1;
  if (new_head >= MAX_QUEUE_ENTRIES) {
    new_head = 0;
  }
  if (new_head == q->tail) {
    SYS_ARCH_UNPROTECT(lev);
    return ERR_MEM;
  }

  q->q_mem[q->head] = msg;
  q->head = new_head;
  LWIP_ASSERT("mbox is full!", q->head != q->tail);
  ret = ReleaseSemaphore(q->sem, 1, 0);
  LWIP_ASSERT("Error releasing sem", ret != 0);
  LWIP_UNUSED_ARG(ret);

  SYS_ARCH_UNPROTECT(lev);
  return ERR_OK;
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
  return sys_mbox_trypost(q, msg);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout)
{
  DWORD ret;
  LONGLONG starttime, endtime;
  SYS_ARCH_DECL_PROTECT(lev);

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);
  LWIP_ASSERT("q->sem != INVALID_HANDLE_VALUE", q->sem != INVALID_HANDLE_VALUE);

  if (timeout == 0) {
    timeout = INFINITE;
  }
  starttime = sys_get_ms_longlong();
  ret = WaitForSingleObject(q->sem, timeout);
  if (ret == WAIT_OBJECT_0) {
    SYS_ARCH_PROTECT(lev);
    if (msg != NULL) {
      *msg  = q->q_mem[q->tail];
    }

    q->tail++;
    if (q->tail >= MAX_QUEUE_ENTRIES) {
      q->tail = 0;
    }
    SYS_ARCH_UNPROTECT(lev);
    endtime = sys_get_ms_longlong();
    return (u32_t)(endtime - starttime);
  } else {
    LWIP_ASSERT("Error waiting for sem", ret == WAIT_TIMEOUT);
    if (msg != NULL) {
      *msg  = NULL;
    }

    return SYS_ARCH_TIMEOUT;
  }
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
  DWORD ret;
  SYS_ARCH_DECL_PROTECT(lev);

  /* parameter check */
  LWIP_ASSERT("q != SYS_MBOX_NULL", q != SYS_MBOX_NULL);
  LWIP_ASSERT("q->sem != NULL", q->sem != NULL);
  LWIP_ASSERT("q->sem != INVALID_HANDLE_VALUE", q->sem != INVALID_HANDLE_VALUE);

  ret = WaitForSingleObject(q->sem, 0);
  if (ret == WAIT_OBJECT_0) {
    SYS_ARCH_PROTECT(lev);
    if (msg != NULL) {
      *msg  = q->q_mem[q->tail];
    }

    q->tail++;
    if (q->tail >= MAX_QUEUE_ENTRIES) {
      q->tail = 0;
    }
    SYS_ARCH_UNPROTECT(lev);
    return 0;
  } else {
    LWIP_ASSERT("Error waiting for sem", ret == WAIT_TIMEOUT);
    if (msg != NULL) {
      *msg  = NULL;
    }

    return SYS_MBOX_EMPTY;
  }
}

#if LWIP_NETCONN_SEM_PER_THREAD
sys_sem_t*
sys_arch_netconn_sem_get(void)
{
  LPVOID tls_data = TlsGetValue(netconn_sem_tls_index);
  return (sys_sem_t*)tls_data;
}

void
sys_arch_netconn_sem_alloc(void)
{
  sys_sem_t *sem;
  err_t err;
  BOOL done;

  sem = (sys_sem_t*)malloc(sizeof(sys_sem_t));
  LWIP_ASSERT("failed to allocate memory for TLS semaphore", sem != NULL);
  err = sys_sem_new(sem, 0);
  LWIP_ASSERT("failed to initialise TLS semaphore", err == ERR_OK);
  done = TlsSetValue(netconn_sem_tls_index, sem);
  LWIP_UNUSED_ARG(done);
  LWIP_ASSERT("failed to initialise TLS semaphore storage", done == TRUE);
}

void
sys_arch_netconn_sem_free(void)
{
  LPVOID tls_data = TlsGetValue(netconn_sem_tls_index);
  if (tls_data != NULL) {
    BOOL done;
    free(tls_data);
    done = TlsSetValue(netconn_sem_tls_index, NULL);
    LWIP_UNUSED_ARG(done);
    LWIP_ASSERT("failed to de-init TLS semaphore storage", done == TRUE);
  }
}
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* !NO_SYS */

/* get keyboard state to terminate the debug app on any kbhit event using win32 API */
int
lwip_win32_keypressed(void)
{
  INPUT_RECORD rec;
  DWORD num = 0;
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  BOOL ret = PeekConsoleInput(h, &rec, 1, &num);
  if (ret && num) {
    ReadConsoleInput(h, &rec, 1, &num);
    if (rec.EventType == KEY_EVENT) {
      if (rec.Event.KeyEvent.bKeyDown) {
        /* not a special key? */
        if (rec.Event.KeyEvent.uChar.AsciiChar != 0) {
          return 1;
        }
      }
    }
  }
  return 0;
}

#include <stdarg.h>

/* This is an example implementation for LWIP_PLATFORM_DIAG:
 * format a string and pass it to your output function.
 */
void
lwip_win32_platform_diag(const char *format, ...)
{
  va_list ap;
  /* get the varargs */
  va_start(ap, format);
  /* print via varargs; to use another output function, you could use
     vsnprintf here */
  vprintf(format, ap);
  va_end(ap);
}
