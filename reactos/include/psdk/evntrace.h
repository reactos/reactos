#ifndef ENVTRACE_H
#define ENVTRACE_H

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
} EVENT_TRACE_HEADER, *PEVENT_TRACE_HEADER;

#endif /* ENVTRACE_H */

