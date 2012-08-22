#ifndef GLTHREAD_H
#define GLTHREAD_H

#include "mapi/u_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _glthread_DECLARE_STATIC_MUTEX(name) u_mutex_declare_static(name)
#define _glthread_INIT_MUTEX(name)           u_mutex_init(name)
#define _glthread_DESTROY_MUTEX(name)        u_mutex_destroy(name)
#define _glthread_LOCK_MUTEX(name)           u_mutex_lock(name)
#define _glthread_UNLOCK_MUTEX(name)         u_mutex_unlock(name)

#define _glthread_InitTSD(tsd)               u_tsd_init(tsd);
#define _glthread_DestroyTSD(tsd)            u_tsd_destroy(tsd);
#define _glthread_GetTSD(tsd)                u_tsd_get(tsd);
#define _glthread_SetTSD(tsd, ptr)           u_tsd_set(tsd, ptr);

typedef struct u_tsd _glthread_TSD;
typedef u_mutex _glthread_Mutex;

#ifdef __cplusplus
}
#endif

#endif /* GLTHREAD_H */
