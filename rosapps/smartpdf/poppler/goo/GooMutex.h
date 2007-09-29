//========================================================================
//
// GooMutex.h
//
// Portable mutex macros.
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GMUTEX_H
#define GMUTEX_H

// Usage:
//
// GooMutex m;
// gInitMutex(&m);
// ...
// gLockMutex(&m);
//   ... critical section ...
// gUnlockMutex(&m);
// ...
// gDestroyMutex(&m);

#ifdef WIN32

#include <windows.h>

typedef CRITICAL_SECTION GooMutex;

#define gInitMutex(m) InitializeCriticalSection(m)
#define gInitMutexReentrant(m) InitializeCriticalSection(m)
#define gDestroyMutex(m) DeleteCriticalSection(m)
#define gLockMutex(m) EnterCriticalSection(m)
#define gUnlockMutex(m) LeaveCriticalSection(m)

#else // assume pthreads

#include <pthread.h>

typedef pthread_mutex_t GooMutex;

#define gInitMutex(m) pthread_mutex_init(m, NULL)
#define gDestroyMutex(m) pthread_mutex_destroy(m)
#define gLockMutex(m) pthread_mutex_lock(m)
#define gUnlockMutex(m) pthread_mutex_unlock(m)

#define gInitMutexReentrant(m) \
  { \
  pthread_mutexattr_t attr;  \
  pthread_mutexattr_init(&attr); \
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP); \
  pthread_mutex_init(mutex, &attr); \
  }
#endif

class MutexAutoInitDestroy {
public:
    MutexAutoInitDestroy(GooMutex *mutex) {
        mMutex = mutex;
        gInitMutexReentrant(mMutex);
    }
    ~MutexAutoInitDestroy() {
        gDestroyMutex(mMutex);
    }
private:
    GooMutex *mMutex;
};

#endif
