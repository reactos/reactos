#ifndef _EVNTRACE_
#define _EVNTRACE_

#ifndef WMIAPI
#ifndef MIDL_PASS
#ifdef _WMI_SOURCE_
#define WMIAPI __stdcall
#else
#define WMIAPI DECLSPEC_IMPORT __stdcall
#endif
#endif
#endif

typedef struct _EVENT_TRACE_HEADER
{
  USHORT  Size;
  union {
    USHORT  FieldTypeFlags;
    struct {
      UCHAR  HeaderType;
      UCHAR  MarkerFlags;
    };
  };
  union {
    ULONG  Version;
    struct {
      UCHAR  Type;
      UCHAR  Level;
      USHORT  Version;
    } Class;
  };
  ULONG  ThreadId;
  ULONG  ProcessId;
  LARGE_INTEGER  TimeStamp;
  union {
    GUID  Guid;
    ULONGLONG  GuidPtr;
  };
 union {
    struct {
      ULONG  ClientContext;
      ULONG  Flags;
    };
    struct {
      ULONG  KernelTime;
      ULONG  UserTime;
    };
    ULONG64  ProcessorTime;
  };
} EVENT_TRACE_HEADER;

#ifndef PEVENT_TRACE_HEADER_DEFINED
#define PEVENT_TRACE_HEADER_DEFINED
typedef struct _EVENT_TRACE_HEADER *PEVENT_TRACE_HEADER;
#endif


typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;


ULONG WMIAPI TraceEvent(TRACEHANDLE,PEVENT_TRACE_HEADER);
TRACEHANDLE WMIAPI GetTraceLoggerHandle(PVOID);
ULONG WMIAPI GetTraceEnableFlags(TRACEHANDLE);


#endif /* ENVTRACE_H */

