/* $Id: mutex.c,v 1.4 2002/10/29 04:45:38 rex Exp $
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS POSIX+ Subsystem
 * FILE:        subsys/psx/lib/psxdll/pthread/mutex.c
 * PURPOSE:     Mutex functions
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              19/12/2001: Created
 */

#include <ntos.h>
#include <ddk/ntddk.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <psx/debug.h>
#include <psx/pthread.h>
#include <psx/errno.h>
#include <psx/safeobj.h>

int pthread_mutex_init(pthread_mutex_t *mutex,
    const pthread_mutexattr_t *attr)
{
 struct __mutex     *pmMutex;
 struct __mutexattr *pmaMutexAttrs;
 BOOL                 bShared;
 OBJECT_ATTRIBUTES    oaMutexAttrs;
 NTSTATUS             nErrCode;

 /* invalid return buffer */
 if(mutex == NULL)
  return (EINVAL);

 /* object still open */
 if(__safeobj_validate(*mutex, __PTHREAD_MUTEX_MAGIC))
  return (EBUSY);

 if(attr == NULL)
 {
  /* use default attributes */
  /* create new mutex object */
  pmMutex = (struct __mutex *)malloc(sizeof(struct __mutex));

  /* malloc() failure */
  if(!pmMutex)
   return (ENOMEM);

  /* set the attributes */
  bShared = FALSE;
  pmMutex->type = PTHREAD_MUTEX_RECURSIVE;
 }
 else if(__safeobj_validate(*attr, __PTHREAD_MUTEX_ATTR_MAGIC))
 {
  /* use provided attributes */
  /* create new mutex object */
  pmMutex = (struct __mutex *)malloc(sizeof(struct __mutex));

  /* malloc() failure */
  if(!pmMutex)
   return (ENOMEM);

  /* get the attributes object */
  pmaMutexAttrs = (struct __mutexattr *) *attr;

  /* set the attributes */
  bShared = (pmaMutexAttrs->pshared != PTHREAD_PROCESS_PRIVATE);
  pmMutex->type = pmaMutexAttrs->type;
 }
 else
  return (EINVAL);

 /* necessary for the mutex to be considered valid later */
 pmMutex->signature = __PTHREAD_MUTEX_MAGIC;

 /* creation of the native mutex object */
 pmMutex->handle = 0;

 /* initialize generic object attributes */
 oaMutexAttrs.Length = sizeof(OBJECT_ATTRIBUTES);
 oaMutexAttrs.RootDirectory = NULL;
 oaMutexAttrs.ObjectName = NULL;
 oaMutexAttrs.Attributes = 0;
 oaMutexAttrs.SecurityDescriptor = NULL;
 oaMutexAttrs.SecurityQualityOfService = NULL;

 /* process-exclusive mutex */
 if(bShared)
  oaMutexAttrs.Attributes |= OBJ_EXCLUSIVE;

 /* try to create the object */
 nErrCode = NtCreateMutant
 (
  &pmMutex->handle,
  MUTANT_ALL_ACCESS,
  &oaMutexAttrs,
  FALSE
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  /* free the internal mutex object */
  free(pmMutex);
  /* return errno */
  return (__status_to_errno(nErrCode));
 }

 /* return the pointer to the mutex */
 *mutex = (pthread_mutex_t)pmMutex;

 /* success */
 return (0);

}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
 struct __mutex         *pmMutex;
 NTSTATUS                 nErrCode;
 MUTANT_BASIC_INFORMATION mbiMutexInfo;

 /* invalid pointer or pointer to invalid object */
 if(mutex == NULL || !__safeobj_validate(*mutex, __PTHREAD_MUTEX_MAGIC))
 {
  return (EINVAL);
 }

 pmMutex = (struct __mutex *)*mutex;

 /* query the mutex's status */
 nErrCode = NtQueryMutant
 (
  pmMutex->handle,
  MutantBasicInformation,
  &mbiMutexInfo,
  sizeof(MUTANT_BASIC_INFORMATION),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* the thread is owned - cannot destroy it */
 if(mbiMutexInfo.Count <= 0)
 {
  return (EBUSY);
 }

 /* try to close the handle */
 nErrCode = NtClose(pmMutex->handle);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* free the object, nil the pointer */
 free(*mutex);
 *mutex = NULL;

 /* success */
 return (0);

}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
 struct __mutex * pmMutex;
 NTSTATUS nErrCode;

 /* invalid pointer or pointer to invalid object */
 if(mutex == NULL || !__safeobj_validate(*mutex, __PTHREAD_MUTEX_MAGIC))
  return (EINVAL);

 pmMutex = (struct __mutex *)*mutex;

 /* decide the behavior from the mutex type */
 switch(pmMutex->type)
 {
  case PTHREAD_MUTEX_NORMAL:
  {
   /* unconditionally try to lock the mutex */
   /* FIXME? should we "artificially" hang the thread if it's the mutex owner, since
      NT mutexes always behave recursively? */

#if 0
   if(0 /* mutex owner */ == pthread_self() */)
    NtDelayExecution(FALSE, NULL);
#endif

   nErrCode = NtWaitForSingleObject(pmMutex->handle, FALSE, NULL);
   break;
  }

  case PTHREAD_MUTEX_ERRORCHECK:
  {
   /* prevent a thread from recursively locking the same mutex */
   if(0 /* mutex owner */ == pthread_self()) /* FIXME: implement the correct logic */
    return (EDEADLK);
   else
    nErrCode = NtWaitForSingleObject(pmMutex->handle, FALSE, NULL);

   break;
  }

  case PTHREAD_MUTEX_RECURSIVE:
  {
   /* allow recursive locking */
   /* ASSERT: this is the default behavior for NT */
   nErrCode = NtWaitForSingleObject(pmMutex->handle, FALSE, NULL);
   break;
  }

  default:
   /* we should never reach this point */
   INFO("you should never read this");

 }

 if(nErrCode == STATUS_ABANDONED)
 {
  FIXME("mutex abandoned, not sure on what to do: should we try to lock the mutex again?");
 }
 else if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* success */
 return (0);

}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
 struct __mutex * pmMutex;
 NTSTATUS nErrCode;
 MUTANT_BASIC_INFORMATION mbiMutexInfo;

 /* invalid pointer or pointer to invalid object */
 if(mutex == NULL || !__safeobj_validate(*mutex, __PTHREAD_MUTEX_MAGIC))
  return (EINVAL);

 pmMutex = (struct __mutex *)*mutex;

 /* query the mutex's status */
 nErrCode = NtQueryMutant
 (
  pmMutex->handle,
  MutantBasicInformation,
  &mbiMutexInfo,
  sizeof(MUTANT_BASIC_INFORMATION),
  NULL
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* mutex already locked */
 if(mbiMutexInfo.Count <= 0)
  return (EBUSY);

 /* mutex not locked - mutex type attribute doesn't matter */
 nErrCode = NtWaitForSingleObject(pmMutex->handle, FALSE, NULL);

 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* success */
 return (0);

}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
 struct __mutex * pmMutex;
 NTSTATUS nErrCode;

 /* invalid pointer or pointer to invalid object */
 if(mutex == NULL || !__safeobj_validate(*mutex, __PTHREAD_MUTEX_MAGIC))
  return (EINVAL);

 pmMutex = (struct __mutex *)*mutex;

 /* try to release the mutex */
 nErrCode = NtReleaseMutant(pmMutex->handle, NULL);

 /* failure */
 if(!NT_SUCCESS(nErrCode))
 {
  return (__status_to_errno(nErrCode));
 }

 /* success */
 return (0);

}

/* mutex attributes routines */

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
 struct __mutexattr * pmaMutexAttrs;

 /* invalid return pointer */
 if(!attr)
  return (EINVAL);

 /* allocate internal structure for mutex attributes */
 pmaMutexAttrs = (struct __mutexattr *)malloc(sizeof(struct __mutexattr));

 /* failure */
 if(pmaMutexAttrs == 0)
  return (ENOMEM);

 /* attribute defaults */
 pmaMutexAttrs->pshared = PTHREAD_PROCESS_PRIVATE;
 pmaMutexAttrs->type = PTHREAD_MUTEX_DEFAULT;

 /* return the pointer to the attributes object */
 *attr = (pthread_mutexattr_t)pmaMutexAttrs;

 /* success */
 return (0);

}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
 /* invalid pointer or pointer to invalid object */
 if(attr == NULL || !__safeobj_validate(*attr, __PTHREAD_MUTEX_ATTR_MAGIC))
  return (EINVAL);

 /* deallocate internal structure */
 free(*attr);

 /* success */
 return (0);

}

#define PTHREAD_MUTEXATTR_GET(PATTR,PVAR,FIELD) \
 if( \
  (PATTR) == NULL || \
  (PVAR) == NULL || \
  !__safeobj_validate(*(PATTR), __PTHREAD_MUTEX_ATTR_MAGIC) \
 ) \
  return (EINVAL); \
 else \
 { \
  (*(PVAR)) = ((struct __mutexattr *)*(PATTR))->FIELD; \
  return (0); \
 }

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr,
    int *pshared)
{
 PTHREAD_MUTEXATTR_GET(attr, pshared, pshared)
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
 PTHREAD_MUTEXATTR_GET(attr, type, type)
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr,
    int pshared)
{
 /* invalid pointer or pointer to invalid object */
 if(attr == NULL || !__safeobj_validate(*attr, __PTHREAD_MUTEX_ATTR_MAGIC))
  return (EINVAL);

 /* validate value */
 switch(pshared)
 {
  case PTHREAD_PROCESS_SHARED: break;
  case PTHREAD_PROCESS_PRIVATE: break;
  default: return (EINVAL);
 }

 ((struct __mutexattr *)*attr)->pshared = pshared;

 return (0);

}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
 /* invalid pointer or pointer to invalid object */
 if(attr == NULL || !__safeobj_validate(*attr, __PTHREAD_MUTEX_ATTR_MAGIC))
  return (EINVAL);

 /* validate value */
 switch(type)
 {
  case PTHREAD_MUTEX_NORMAL: break;
  case PTHREAD_MUTEX_ERRORCHECK: break;
  case PTHREAD_MUTEX_RECURSIVE: break;
  default: return (EINVAL);
 }

 ((struct __mutexattr *)*attr)->type = type;

 return (0);

}

/* STUBS */

int pthread_mutex_setprioceiling(pthread_mutex_t *mutex,
    int prioceiling, int *old_ceiling)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

int pthread_mutex_getprioceiling(const pthread_mutex_t *mutex,
    int *prioceiling)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr,
    int *protocol)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr,
    int protocol)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr,
    int prioceiling)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr,
    int *prioceiling)
{
 TODO("realtime threads not currently implemented");
 return (ENOSYS);
}

/* EOF */

