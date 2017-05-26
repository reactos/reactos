#ifndef _RDBSSLOG_INCLUDED_
#define _RDBSSLOG_INCLUDED_

VOID
NTAPI
RxDebugControlCommand(
    _In_ PSTR ControlString);

NTSTATUS
NTAPI
RxInitializeLog(
    VOID);

#ifdef RDBSSLOG

#if DBG
#define RxLog(Args) _RxLog##Args
#define RxLogRetail(Args) _RxLog##Args
#else
#define RxLogRetail(Args) _RxLog##Args
#define RxLog(Args) { }
#endif

#define RxPauseLog() _RxPauseLog()
#define RxResumeLog() _RxResumeLog()

#else

#define RxLog(Args) { ;}
#define RxLogRetail(Args) { ;}
#define RxPauseLog() { ; }
#define RxResumeLog() { ; }

#endif

#endif
