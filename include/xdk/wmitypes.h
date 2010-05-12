/******************************************************************************
 *                          WMI Library Support Types                         *
 ******************************************************************************/

#ifdef RUN_WPP
#include <evntrace.h>
#include <stdarg.h>
#endif

#ifndef _TRACEHANDLE_DEFINED
#define _TRACEHANDLE_DEFINED
typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
#endif

#ifndef TRACE_INFORMATION_CLASS_DEFINE

typedef struct _ETW_TRACE_SESSION_SETTINGS {
  ULONG Version;
  ULONG BufferSize;
  ULONG MinimumBuffers;
  ULONG MaximumBuffers;
  ULONG LoggerMode;
  ULONG FlushTimer;
  ULONG FlushThreshold;
  ULONG ClockType;
} ETW_TRACE_SESSION_SETTINGS, *PETW_TRACE_SESSION_SETTINGS;

typedef enum _TRACE_INFORMATION_CLASS {
  TraceIdClass,
  TraceHandleClass,
  TraceEnableFlagsClass,
  TraceEnableLevelClass,
  GlobalLoggerHandleClass,
  EventLoggerHandleClass,
  AllLoggerHandlesClass,
  TraceHandleByNameClass,
  LoggerEventsLostClass,
  TraceSessionSettingsClass,
  LoggerEventsLoggedClass,
  MaxTraceInformationClass
} TRACE_INFORMATION_CLASS;

#endif /* TRACE_INFORMATION_CLASS_DEFINE */

#ifndef _ETW_KM_
#define _ETW_KM_
#endif

#include <evntprov.h>

typedef VOID
(NTAPI *PETWENABLECALLBACK)(
  IN LPCGUID SourceId,
  IN ULONG ControlCode,
  IN UCHAR Level,
  IN ULONGLONG MatchAnyKeyword,
  IN ULONGLONG MatchAllKeyword,
  IN PEVENT_FILTER_DESCRIPTOR FilterData OPTIONAL,
  IN OUT PVOID CallbackContext OPTIONAL);

#define EVENT_WRITE_FLAG_NO_FAULTING             0x00000001

