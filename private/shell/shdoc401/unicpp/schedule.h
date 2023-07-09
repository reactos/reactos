#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#ifdef __cplusplus
extern "C" {
#endif

void CShellTaskScheduler_CreateThreadPool( void );
void CShellTaskScheduler_FreeThreadPool( void );

// global function for allocating a task scheduler.
// any object that uses it must make sure that its tasks are removed from the queue
// before it exits.
HRESULT SHGetSystemScheduler( LPSHELLTASKSCHEDULER * ppScheduler );
HRESULT SHFreeSystemScheduler( void );

#ifdef DEBUG
VOID SHValidateEmptySystemScheduler(void);
#else
#define SHValidateEmptySystemScheduler()
#endif

#ifdef __cplusplus
};
#endif
#endif

