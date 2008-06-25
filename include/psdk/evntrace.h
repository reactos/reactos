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
typedef ULONG (WINAPI *WMIDPREQUEST)(
    WMIDPREQUESTCODE RequestCode,
    PVOID RequestContext,
    ULONG *BufferSize,
    PVOID Buffer
);

typedef struct _TRACE_GUID_REGISTRATION
{
    LPCGUID Guid;
    HANDLE RegHandle;
} TRACE_GUID_REGISTRATION, *PTRACE_GUID_REGISTRATION;


ULONG WMIAPI TraceEvent(TRACEHANDLE,PEVENT_TRACE_HEADER);
TRACEHANDLE WMIAPI GetTraceLoggerHandle(PVOID);
ULONG WMIAPI GetTraceEnableFlags(TRACEHANDLE);
UCHAR WMIAPI GetTraceEnableLevel(TRACEHANDLE);
ULONG WMIAPI RegisterTraceGuidsA(WMIDPREQUEST,PVOID,LPCGUID,ULONG,PTRACE_GUID_REGISTRATION,LPCSTR,LPCSTR,PTRACEHANDLE);
ULONG WMIAPI RegisterTraceGuidsW(WMIDPREQUEST,PVOID,LPCGUID,ULONG,PTRACE_GUID_REGISTRATION,LPCWSTR,LPCWSTR,PTRACEHANDLE);
ULONG WMIAPI UnregisterTraceGuids(TRACEHANDLE);

#if defined (UNICODE) || defined (_UNICODE)
#define RegisterTraceGuids RegisterTraceGuidsW
#else
#define RegisterTraceGuids RegisterTraceGuidsA
#endif

#endif /* ENVTRACE_H */

