#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#ifdef __cplusplus
extern "C" {
#endif

// global function for allocating a task scheduler.
// any object that uses it must make sure that its tasks are removed from the queue
// before it exits.
// BUGBUG: use CoCreateInstance - thread pool removes need for global scheduler
HRESULT SHGetSystemScheduler( LPSHELLTASKSCHEDULER * ppScheduler );


#ifdef DEBUG
VOID SHValidateEmptySystemScheduler(void);
#else
#define SHValidateEmptySystemScheduler()
#endif

#ifdef __cplusplus
};
#endif
#endif

