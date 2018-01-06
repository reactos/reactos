#pragma once

#define _EVNTRACE_

#ifndef WMIAPI
#ifndef MIDL_PASS
#ifdef _WMI_SOURCE_
#define WMIAPI __stdcall
#else
#define WMIAPI DECLSPEC_IMPORT __stdcall
#endif
#endif /* MIDL_PASS */
#endif /* WMIAPI */

#include <guiddef.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID (EventTraceGuid, 0x68fdd900, 0x4a3e, 0x11d1, 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3);
DEFINE_GUID (SystemTraceControlGuid, 0x9e814aad, 0x3204, 0x11d2, 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39);
DEFINE_GUID (EventTraceConfigGuid, 0x01853a65, 0x418f, 0x4f36, 0xae, 0xfc, 0xdc, 0x0f, 0x1d, 0x2f, 0xd2, 0x35);
DEFINE_GUID (DefaultTraceSecurityGuid, 0x0811c1af, 0x7a07, 0x4a06, 0x82, 0xed, 0x86, 0x94, 0x55, 0xcd, 0xf7, 0x13);

#define KERNEL_LOGGER_NAMEW                 L"NT Kernel Logger"
#define GLOBAL_LOGGER_NAMEW                 L"GlobalLogger"
#define EVENT_LOGGER_NAMEW                  L"EventLog"
#define DIAG_LOGGER_NAMEW                   L"DiagLog"

#define KERNEL_LOGGER_NAMEA                 "NT Kernel Logger"
#define GLOBAL_LOGGER_NAMEA                 "GlobalLogger"
#define EVENT_LOGGER_NAMEA                  "EventLog"
#define DIAG_LOGGER_NAMEA                   "DiagLog"

#define MAX_MOF_FIELDS                      16

#ifndef _TRACEHANDLE_DEFINED
#define _TRACEHANDLE_DEFINED
typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
#endif

#define SYSTEM_EVENT_TYPE                   1

#define EVENT_TRACE_TYPE_INFO               0x00
#define EVENT_TRACE_TYPE_START              0x01
#define EVENT_TRACE_TYPE_END                0x02
#define EVENT_TRACE_TYPE_STOP               0x02
#define EVENT_TRACE_TYPE_DC_START           0x03
#define EVENT_TRACE_TYPE_DC_END             0x04
#define EVENT_TRACE_TYPE_EXTENSION          0x05
#define EVENT_TRACE_TYPE_REPLY              0x06
#define EVENT_TRACE_TYPE_DEQUEUE            0x07
#define EVENT_TRACE_TYPE_RESUME             0x07
#define EVENT_TRACE_TYPE_CHECKPOINT         0x08
#define EVENT_TRACE_TYPE_SUSPEND            0x08
#define EVENT_TRACE_TYPE_WINEVT_SEND        0x09
#define EVENT_TRACE_TYPE_WINEVT_RECEIVE     0XF0

#define TRACE_LEVEL_NONE                    0
#define TRACE_LEVEL_CRITICAL                1
#define TRACE_LEVEL_FATAL                   1
#define TRACE_LEVEL_ERROR                   2
#define TRACE_LEVEL_WARNING                 3
#define TRACE_LEVEL_INFORMATION             4
#define TRACE_LEVEL_VERBOSE                 5
#define TRACE_LEVEL_RESERVED6               6
#define TRACE_LEVEL_RESERVED7               7
#define TRACE_LEVEL_RESERVED8               8
#define TRACE_LEVEL_RESERVED9               9

#define EVENT_TRACE_TYPE_LOAD               0x0A

#define EVENT_TRACE_TYPE_IO_READ            0x0A
#define EVENT_TRACE_TYPE_IO_WRITE           0x0B
#define EVENT_TRACE_TYPE_IO_READ_INIT       0x0C
#define EVENT_TRACE_TYPE_IO_WRITE_INIT      0x0D
#define EVENT_TRACE_TYPE_IO_FLUSH           0x0E
#define EVENT_TRACE_TYPE_IO_FLUSH_INIT      0x0F

#define EVENT_TRACE_TYPE_MM_TF              0x0A
#define EVENT_TRACE_TYPE_MM_DZF             0x0B
#define EVENT_TRACE_TYPE_MM_COW             0x0C
#define EVENT_TRACE_TYPE_MM_GPF             0x0D
#define EVENT_TRACE_TYPE_MM_HPF             0x0E
#define EVENT_TRACE_TYPE_MM_AV              0x0F

#define EVENT_TRACE_TYPE_SEND               0x0A
#define EVENT_TRACE_TYPE_RECEIVE            0x0B
#define EVENT_TRACE_TYPE_CONNECT            0x0C
#define EVENT_TRACE_TYPE_DISCONNECT         0x0D
#define EVENT_TRACE_TYPE_RETRANSMIT         0x0E
#define EVENT_TRACE_TYPE_ACCEPT             0x0F
#define EVENT_TRACE_TYPE_RECONNECT          0x10
#define EVENT_TRACE_TYPE_CONNFAIL           0x11
#define EVENT_TRACE_TYPE_COPY_TCP           0x12
#define EVENT_TRACE_TYPE_COPY_ARP           0x13
#define EVENT_TRACE_TYPE_ACKFULL            0x14
#define EVENT_TRACE_TYPE_ACKPART            0x15
#define EVENT_TRACE_TYPE_ACKDUP             0x16

#define EVENT_TRACE_TYPE_GUIDMAP            0x0A
#define EVENT_TRACE_TYPE_CONFIG             0x0B
#define EVENT_TRACE_TYPE_SIDINFO            0x0C
#define EVENT_TRACE_TYPE_SECURITY           0x0D

#define EVENT_TRACE_TYPE_REGCREATE          0x0A
#define EVENT_TRACE_TYPE_REGOPEN            0x0B
#define EVENT_TRACE_TYPE_REGDELETE          0x0C
#define EVENT_TRACE_TYPE_REGQUERY           0x0D
#define EVENT_TRACE_TYPE_REGSETVALUE        0x0E
#define EVENT_TRACE_TYPE_REGDELETEVALUE     0x0F
#define EVENT_TRACE_TYPE_REGQUERYVALUE      0x10
#define EVENT_TRACE_TYPE_REGENUMERATEKEY    0x11
#define EVENT_TRACE_TYPE_REGENUMERATEVALUEKEY 0x12
#define EVENT_TRACE_TYPE_REGQUERYMULTIPLEVALUE 0x13
#define EVENT_TRACE_TYPE_REGSETINFORMATION  0x14
#define EVENT_TRACE_TYPE_REGFLUSH           0x15
#define EVENT_TRACE_TYPE_REGKCBCREATE       0x16
#define EVENT_TRACE_TYPE_REGKCBDELETE       0x17
#define EVENT_TRACE_TYPE_REGKCBRUNDOWNBEGIN 0x18
#define EVENT_TRACE_TYPE_REGKCBRUNDOWNEND   0x19
#define EVENT_TRACE_TYPE_REGVIRTUALIZE      0x1A
#define EVENT_TRACE_TYPE_REGCLOSE           0x1B
#define EVENT_TRACE_TYPE_REGSETSECURITY     0x1C
#define EVENT_TRACE_TYPE_REGQUERYSECURITY   0x1D
#define EVENT_TRACE_TYPE_REGCOMMIT          0x1E
#define EVENT_TRACE_TYPE_REGPREPARE         0x1F
#define EVENT_TRACE_TYPE_REGROLLBACK        0x20
#define EVENT_TRACE_TYPE_REGMOUNTHIVE       0x21

#define EVENT_TRACE_TYPE_CONFIG_CPU         0x0A
#define EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK 0x0B
#define EVENT_TRACE_TYPE_CONFIG_LOGICALDISK 0x0C
#define EVENT_TRACE_TYPE_CONFIG_NIC         0x0D
#define EVENT_TRACE_TYPE_CONFIG_VIDEO       0x0E
#define EVENT_TRACE_TYPE_CONFIG_SERVICES    0x0F
#define EVENT_TRACE_TYPE_CONFIG_POWER       0x10
#define EVENT_TRACE_TYPE_CONFIG_NETINFO     0x11

#define EVENT_TRACE_TYPE_CONFIG_IRQ         0x15
#define EVENT_TRACE_TYPE_CONFIG_PNP         0x16
#define EVENT_TRACE_TYPE_CONFIG_IDECHANNEL  0x17
#define EVENT_TRACE_TYPE_CONFIG_PLATFORM    0x19

#define EVENT_TRACE_FLAG_PROCESS            0x00000001
#define EVENT_TRACE_FLAG_THREAD             0x00000002
#define EVENT_TRACE_FLAG_IMAGE_LOAD         0x00000004

#define EVENT_TRACE_FLAG_DISK_IO            0x00000100
#define EVENT_TRACE_FLAG_DISK_FILE_IO       0x00000200

#define EVENT_TRACE_FLAG_MEMORY_PAGE_FAULTS 0x00001000
#define EVENT_TRACE_FLAG_MEMORY_HARD_FAULTS 0x00002000

#define EVENT_TRACE_FLAG_NETWORK_TCPIP      0x00010000

#define EVENT_TRACE_FLAG_REGISTRY           0x00020000
#define EVENT_TRACE_FLAG_DBGPRINT           0x00040000

#define EVENT_TRACE_FLAG_PROCESS_COUNTERS   0x00000008
#define EVENT_TRACE_FLAG_CSWITCH            0x00000010
#define EVENT_TRACE_FLAG_DPC                0x00000020
#define EVENT_TRACE_FLAG_INTERRUPT          0x00000040
#define EVENT_TRACE_FLAG_SYSTEMCALL         0x00000080

#define EVENT_TRACE_FLAG_DISK_IO_INIT       0x00000400

#define EVENT_TRACE_FLAG_ALPC               0x00100000
#define EVENT_TRACE_FLAG_SPLIT_IO           0x00200000

#define EVENT_TRACE_FLAG_DRIVER             0x00800000
#define EVENT_TRACE_FLAG_PROFILE            0x01000000
#define EVENT_TRACE_FLAG_FILE_IO            0x02000000
#define EVENT_TRACE_FLAG_FILE_IO_INIT       0x04000000

#define EVENT_TRACE_FLAG_DISPATCHER         0x00000800
#define EVENT_TRACE_FLAG_VIRTUAL_ALLOC      0x00004000

#define EVENT_TRACE_FLAG_EXTENSION          0x80000000
#define EVENT_TRACE_FLAG_FORWARD_WMI        0x40000000
#define EVENT_TRACE_FLAG_ENABLE_RESERVE     0x20000000

#define EVENT_TRACE_FILE_MODE_NONE          0x00000000
#define EVENT_TRACE_FILE_MODE_SEQUENTIAL    0x00000001
#define EVENT_TRACE_FILE_MODE_CIRCULAR      0x00000002
#define EVENT_TRACE_FILE_MODE_APPEND        0x00000004

#define EVENT_TRACE_REAL_TIME_MODE          0x00000100
#define EVENT_TRACE_DELAY_OPEN_FILE_MODE    0x00000200
#define EVENT_TRACE_BUFFERING_MODE          0x00000400
#define EVENT_TRACE_PRIVATE_LOGGER_MODE     0x00000800
#define EVENT_TRACE_ADD_HEADER_MODE         0x00001000

#define EVENT_TRACE_USE_GLOBAL_SEQUENCE     0x00004000
#define EVENT_TRACE_USE_LOCAL_SEQUENCE      0x00008000

#define EVENT_TRACE_RELOG_MODE              0x00010000

#define EVENT_TRACE_USE_PAGED_MEMORY        0x01000000

#define EVENT_TRACE_FILE_MODE_NEWFILE       0x00000008
#define EVENT_TRACE_FILE_MODE_PREALLOCATE   0x00000020

#define EVENT_TRACE_NONSTOPPABLE_MODE       0x00000040
#define EVENT_TRACE_SECURE_MODE             0x00000080
#define EVENT_TRACE_USE_KBYTES_FOR_SIZE     0x00002000
#define EVENT_TRACE_PRIVATE_IN_PROC         0x00020000
#define EVENT_TRACE_MODE_RESERVED           0x00100000

#define EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING 0x10000000

#define EVENT_TRACE_CONTROL_QUERY           0
#define EVENT_TRACE_CONTROL_STOP            1
#define EVENT_TRACE_CONTROL_UPDATE          2

#define EVENT_TRACE_CONTROL_FLUSH           3

#define TRACE_MESSAGE_SEQUENCE              1
#define TRACE_MESSAGE_GUID                  2
#define TRACE_MESSAGE_COMPONENTID           4
#define TRACE_MESSAGE_TIMESTAMP             8
#define TRACE_MESSAGE_PERFORMANCE_TIMESTAMP 16
#define TRACE_MESSAGE_SYSTEMINFO            32

#define TRACE_MESSAGE_POINTER32             0x0040
#define TRACE_MESSAGE_POINTER64             0x0080

#define TRACE_MESSAGE_FLAG_MASK             0xFFFF

#define TRACE_MESSAGE_MAXIMUM_SIZE          8*1024

#define EVENT_TRACE_USE_PROCTIME            0x0001
#define EVENT_TRACE_USE_NOCPUTIME           0x0002

#define TRACE_HEADER_FLAG_USE_TIMESTAMP     0x00000200
#define TRACE_HEADER_FLAG_TRACED_GUID       0x00020000
#define TRACE_HEADER_FLAG_LOG_WNODE         0x00040000
#define TRACE_HEADER_FLAG_USE_GUID_PTR      0x00080000
#define TRACE_HEADER_FLAG_USE_MOF_PTR       0x00100000

#define ETW_NULL_TYPE_VALUE                 0
#define ETW_OBJECT_TYPE_VALUE               1
#define ETW_STRING_TYPE_VALUE               2
#define ETW_SBYTE_TYPE_VALUE                3
#define ETW_BYTE_TYPE_VALUE                 4
#define ETW_INT16_TYPE_VALUE                5
#define ETW_UINT16_TYPE_VALUE               6
#define ETW_INT32_TYPE_VALUE                7
#define ETW_UINT32_TYPE_VALUE               8
#define ETW_INT64_TYPE_VALUE                9
#define ETW_UINT64_TYPE_VALUE               10
#define ETW_CHAR_TYPE_VALUE                 11
#define ETW_SINGLE_TYPE_VALUE               12
#define ETW_DOUBLE_TYPE_VALUE               13
#define ETW_BOOLEAN_TYPE_VALUE              14
#define ETW_DECIMAL_TYPE_VALUE              15

#define ETW_GUID_TYPE_VALUE                 101
#define ETW_ASCIICHAR_TYPE_VALUE            102
#define ETW_ASCIISTRING_TYPE_VALUE          103
#define ETW_COUNTED_STRING_TYPE_VALUE       104
#define ETW_POINTER_TYPE_VALUE              105
#define ETW_SIZET_TYPE_VALUE                106
#define ETW_HIDDEN_TYPE_VALUE               107
#define ETW_BOOL_TYPE_VALUE                 108
#define ETW_COUNTED_ANSISTRING_TYPE_VALUE   109
#define ETW_REVERSED_COUNTED_STRING_TYPE_VALUE 110
#define ETW_REVERSED_COUNTED_ANSISTRING_TYPE_VALUE 111
#define ETW_NON_NULL_TERMINATED_STRING_TYPE_VALUE 112
#define ETW_REDUCED_ANSISTRING_TYPE_VALUE   113
#define ETW_REDUCED_STRING_TYPE_VALUE       114
#define ETW_SID_TYPE_VALUE                  115
#define ETW_VARIANT_TYPE_VALUE              116
#define ETW_PTVECTOR_TYPE_VALUE             117
#define ETW_WMITIME_TYPE_VALUE              118
#define ETW_DATETIME_TYPE_VALUE             119
#define ETW_REFRENCE_TYPE_VALUE             120

#define TRACE_PROVIDER_FLAG_LEGACY          0x00000001
#define TRACE_PROVIDER_FLAG_PRE_ENABLE      0x00000002

#define EVENT_CONTROL_CODE_DISABLE_PROVIDER 0
#define EVENT_CONTROL_CODE_ENABLE_PROVIDER  1
#define EVENT_CONTROL_CODE_CAPTURE_STATE    2

#define DEFINE_TRACE_MOF_FIELD(MOF, ptr, length, type) \
  (MOF)->DataPtr = (ULONG64)(ULONG_PTR) ptr; \
  (MOF)->Length = (ULONG) length; \
  (MOF)->DataType = (ULONG) type;

typedef struct _EVENT_TRACE_HEADER {
  USHORT Size;
  _ANONYMOUS_UNION union {
    USHORT FieldTypeFlags;
    _ANONYMOUS_STRUCT struct {
      UCHAR HeaderType;
      UCHAR MarkerFlags;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  _ANONYMOUS_UNION union {
    ULONG Version;
    struct {
      UCHAR Type;
      UCHAR Level;
      USHORT Version;
    } Class;
  } DUMMYUNIONNAME2;
  ULONG ThreadId;
  ULONG ProcessId;
  LARGE_INTEGER TimeStamp;
  _ANONYMOUS_UNION union {
    GUID Guid;
    ULONGLONG GuidPtr;
  } DUMMYUNIONNAME3;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      ULONG KernelTime;
      ULONG UserTime;
    } DUMMYSTRUCTNAME;
    ULONG64 ProcessorTime;
    _ANONYMOUS_STRUCT struct {
      ULONG ClientContext;
      ULONG Flags;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME4;
} EVENT_TRACE_HEADER;

#ifndef PEVENT_TRACE_HEADER_DEFINED
#define PEVENT_TRACE_HEADER_DEFINED
typedef struct _EVENT_TRACE_HEADER *PEVENT_TRACE_HEADER;
#endif

typedef struct _EVENT_INSTANCE_HEADER {
  USHORT Size;
  _ANONYMOUS_UNION union {
    USHORT FieldTypeFlags;
    _ANONYMOUS_STRUCT struct {
      UCHAR HeaderType;
      UCHAR MarkerFlags;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  _ANONYMOUS_UNION union {
    ULONG Version;
    struct {
      UCHAR Type;
      UCHAR Level;
      USHORT Version;
    } Class;
  } DUMMYUNIONNAME2;
  ULONG ThreadId;
  ULONG ProcessId;
  LARGE_INTEGER TimeStamp;
  ULONGLONG RegHandle;
  ULONG InstanceId;
  ULONG ParentInstanceId;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      ULONG KernelTime;
      ULONG UserTime;
    } DUMMYSTRUCTNAME;
    ULONG64 ProcessorTime;
    _ANONYMOUS_STRUCT struct {
      ULONG EventId;
      ULONG Flags;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME3;
  ULONGLONG ParentRegHandle;
} EVENT_INSTANCE_HEADER, *PEVENT_INSTANCE_HEADER;

typedef struct _MOF_FIELD {
  ULONG64 DataPtr;
  ULONG Length;
  ULONG DataType;
} MOF_FIELD, *PMOF_FIELD;

typedef struct _EVENT_INSTANCE_INFO {
  HANDLE RegHandle;
  ULONG InstanceId;
} EVENT_INSTANCE_INFO, *PEVENT_INSTANCE_INFO;

typedef struct _TRACE_GUID_PROPERTIES {
  GUID Guid;
  ULONG GuidType;
  ULONG LoggerId;
  ULONG EnableLevel;
  ULONG EnableFlags;
  BOOLEAN IsEnable;
} TRACE_GUID_PROPERTIES, *PTRACE_GUID_PROPERTIES;

typedef struct _ETW_BUFFER_CONTEXT {
  UCHAR ProcessorNumber;
  UCHAR Alignment;
  USHORT LoggerId;
} ETW_BUFFER_CONTEXT, *PETW_BUFFER_CONTEXT;

typedef struct _TRACE_ENABLE_INFO {
  ULONG IsEnabled;
  UCHAR Level;
  UCHAR Reserved1;
  USHORT LoggerId;
  ULONG EnableProperty;
  ULONG Reserved2;
  ULONGLONG MatchAnyKeyword;
  ULONGLONG MatchAllKeyword;
} TRACE_ENABLE_INFO, *PTRACE_ENABLE_INFO;

typedef struct _TRACE_PROVIDER_INSTANCE_INFO {
  ULONG NextOffset;
  ULONG EnableCount;
  ULONG Pid;
  ULONG Flags;
} TRACE_PROVIDER_INSTANCE_INFO, *PTRACE_PROVIDER_INSTANCE_INFO;

typedef struct _TRACE_GUID_INFO {
  ULONG InstanceCount;
  ULONG Reserved;
} TRACE_GUID_INFO, *PTRACE_GUID_INFO;

typedef struct _EVENT_TRACE {
  EVENT_TRACE_HEADER Header;
  ULONG InstanceId;
  ULONG ParentInstanceId;
  GUID ParentGuid;
  PVOID MofData;
  ULONG MofLength;
  _ANONYMOUS_UNION union {
    ULONG ClientContext;
    ETW_BUFFER_CONTEXT BufferContext;
  } DUMMYUNIONNAME;
} EVENT_TRACE, *PEVENT_TRACE;

#if !(defined(_NTDDK_) || defined(_NTIFS_)) || defined(_WMIKM_)

typedef struct _TRACE_LOGFILE_HEADER {
  ULONG BufferSize;
  _ANONYMOUS_UNION union {
    ULONG Version;
    struct {
      UCHAR MajorVersion;
      UCHAR MinorVersion;
      UCHAR SubVersion;
      UCHAR SubMinorVersion;
    } VersionDetail;
  } DUMMYUNIONNAME;
  ULONG ProviderVersion;
  ULONG NumberOfProcessors;
  LARGE_INTEGER EndTime;
  ULONG TimerResolution;
  ULONG MaximumFileSize;
  ULONG LogFileMode;
  ULONG BuffersWritten;
  _ANONYMOUS_UNION union {
    GUID LogInstanceGuid;
    _ANONYMOUS_STRUCT struct {
      ULONG StartBuffers;
      ULONG PointerSize;
      ULONG EventsLost;
      ULONG CpuSpeedInMHz;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME2;
#if defined(_WMIKM_)
  PWCHAR LoggerName;
  PWCHAR LogFileName;
  RTL_TIME_ZONE_INFORMATION TimeZone;
#else
  LPWSTR LoggerName;
  LPWSTR LogFileName;
  TIME_ZONE_INFORMATION TimeZone;
#endif
  LARGE_INTEGER BootTime;
  LARGE_INTEGER PerfFreq;
  LARGE_INTEGER StartTime;
  ULONG ReservedFlags;
  ULONG BuffersLost;
} TRACE_LOGFILE_HEADER, *PTRACE_LOGFILE_HEADER;

typedef struct _TRACE_LOGFILE_HEADER32 {
  ULONG BufferSize;
  _ANONYMOUS_UNION union {
    ULONG Version;
    struct {
      UCHAR MajorVersion;
      UCHAR MinorVersion;
      UCHAR SubVersion;
      UCHAR SubMinorVersion;
    } VersionDetail;
  } DUMMYUNIONNAME;
  ULONG ProviderVersion;
  ULONG NumberOfProcessors;
  LARGE_INTEGER EndTime;
  ULONG TimerResolution;
  ULONG MaximumFileSize;
  ULONG LogFileMode;
  ULONG BuffersWritten;
  _ANONYMOUS_UNION union {
    GUID LogInstanceGuid;
    _ANONYMOUS_STRUCT struct {
      ULONG StartBuffers;
      ULONG PointerSize;
      ULONG EventsLost;
      ULONG CpuSpeedInMHz;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME2;
#if defined(_WMIKM_)
  ULONG32 LoggerName;
  ULONG32 LogFileName;
  RTL_TIME_ZONE_INFORMATION TimeZone;
#else
  ULONG32 LoggerName;
  ULONG32 LogFileName;
  TIME_ZONE_INFORMATION TimeZone;
#endif
  LARGE_INTEGER BootTime;
  LARGE_INTEGER PerfFreq;
  LARGE_INTEGER StartTime;
  ULONG ReservedFlags;
  ULONG BuffersLost;
} TRACE_LOGFILE_HEADER32, *PTRACE_LOGFILE_HEADER32;

typedef struct _TRACE_LOGFILE_HEADER64 {
  ULONG BufferSize;
  _ANONYMOUS_UNION union {
    ULONG Version;
    struct {
      UCHAR MajorVersion;
      UCHAR MinorVersion;
      UCHAR SubVersion;
      UCHAR SubMinorVersion;
    } VersionDetail;
  } DUMMYUNIONNAME;
  ULONG ProviderVersion;
  ULONG NumberOfProcessors;
  LARGE_INTEGER EndTime;
  ULONG TimerResolution;
  ULONG MaximumFileSize;
  ULONG LogFileMode;
  ULONG BuffersWritten;
  _ANONYMOUS_UNION union {
    GUID LogInstanceGuid;
    _ANONYMOUS_STRUCT struct {
      ULONG StartBuffers;
      ULONG PointerSize;
      ULONG EventsLost;
      ULONG CpuSpeedInMHz;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME2;
#if defined(_WMIKM_)
  ULONG64 LoggerName;
  ULONG64 LogFileName;
  RTL_TIME_ZONE_INFORMATION TimeZone;
#else
  ULONG64 LoggerName;
  ULONG64 LogFileName;
  TIME_ZONE_INFORMATION TimeZone;
#endif
  LARGE_INTEGER BootTime;
  LARGE_INTEGER PerfFreq;
  LARGE_INTEGER StartTime;
  ULONG ReservedFlags;
  ULONG BuffersLost;
} TRACE_LOGFILE_HEADER64, *PTRACE_LOGFILE_HEADER64;

#endif /* !_NTDDK_ || _WMIKM_ */

#if !defined(_WMIKM_) && !defined(_NTDDK_) && !defined(_NTIFS_)

#define ENABLE_TRACE_PARAMETERS_VERSION     1

typedef struct _EVENT_TRACE_PROPERTIES {
  WNODE_HEADER Wnode;
  ULONG BufferSize;
  ULONG MinimumBuffers;
  ULONG MaximumBuffers;
  ULONG MaximumFileSize;
  ULONG LogFileMode;
  ULONG FlushTimer;
  ULONG EnableFlags;
  LONG  AgeLimit;
  ULONG NumberOfBuffers;
  ULONG FreeBuffers;
  ULONG EventsLost;
  ULONG BuffersWritten;
  ULONG LogBuffersLost;
  ULONG RealTimeBuffersLost;
  HANDLE LoggerThreadId;
  ULONG LogFileNameOffset;
  ULONG LoggerNameOffset;
} EVENT_TRACE_PROPERTIES, *PEVENT_TRACE_PROPERTIES;

typedef struct _TRACE_GUID_REGISTRATION {
  LPCGUID Guid;
  HANDLE RegHandle;
} TRACE_GUID_REGISTRATION, *PTRACE_GUID_REGISTRATION;

typedef struct _EVENT_RECORD EVENT_RECORD, *PEVENT_RECORD;
typedef struct _EVENT_TRACE_LOGFILEW EVENT_TRACE_LOGFILEW, *PEVENT_TRACE_LOGFILEW;
typedef struct _EVENT_TRACE_LOGFILEA EVENT_TRACE_LOGFILEA, *PEVENT_TRACE_LOGFILEA;
#ifndef _EVNTPROV_
typedef struct _EVENT_FILTER_DESCRIPTOR EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;
#endif

typedef ULONG
(WINAPI *PEVENT_TRACE_BUFFER_CALLBACKW)(
  PEVENT_TRACE_LOGFILEW Logfile);

typedef ULONG
(WINAPI *PEVENT_TRACE_BUFFER_CALLBACKA)(
  PEVENT_TRACE_LOGFILEA Logfile);

typedef VOID
(WINAPI *PEVENT_CALLBACK)(
  PEVENT_TRACE pEvent );

typedef VOID
(WINAPI *PEVENT_RECORD_CALLBACK)(
  PEVENT_RECORD EventRecord);

typedef ULONG
(WINAPI *WMIDPREQUEST)(
  IN WMIDPREQUESTCODE RequestCode,
  IN PVOID RequestContext,
  IN OUT ULONG *BufferSize,
  IN OUT PVOID Buffer);

struct _EVENT_TRACE_LOGFILEW {
  LPWSTR LogFileName;
  LPWSTR LoggerName;
  LONGLONG CurrentTime;
  ULONG BuffersRead;
  _ANONYMOUS_UNION union {
    ULONG LogFileMode;
    ULONG ProcessTraceMode;
  } DUMMYUNIONNAME;
  EVENT_TRACE CurrentEvent;
  TRACE_LOGFILE_HEADER LogfileHeader;
  PEVENT_TRACE_BUFFER_CALLBACKW BufferCallback;
  ULONG BufferSize;
  ULONG Filled;
  ULONG EventsLost;
  _ANONYMOUS_UNION union {
    PEVENT_CALLBACK EventCallback;
    PEVENT_RECORD_CALLBACK EventRecordCallback;
  } DUMMYUNIONNAME2;
  ULONG IsKernelTrace;
  PVOID Context;
};

struct _EVENT_TRACE_LOGFILEA {
  LPSTR LogFileName;
  LPSTR LoggerName;
  LONGLONG CurrentTime;
  ULONG BuffersRead;
  _ANONYMOUS_UNION union {
    ULONG LogFileMode;
    ULONG ProcessTraceMode;
  } DUMMYUNIONNAME;
  EVENT_TRACE CurrentEvent;
  TRACE_LOGFILE_HEADER LogfileHeader;
  PEVENT_TRACE_BUFFER_CALLBACKA BufferCallback;
  ULONG BufferSize;
  ULONG Filled;
  ULONG EventsLost;
  _ANONYMOUS_UNION union {
    PEVENT_CALLBACK EventCallback;
    PEVENT_RECORD_CALLBACK EventRecordCallback;
  } DUMMYUNIONNAME2;
  ULONG IsKernelTrace;
  PVOID Context;
};

#if defined(_UNICODE) || defined(UNICODE)

#define PEVENT_TRACE_BUFFER_CALLBACK PEVENT_TRACE_BUFFER_CALLBACKW
#define EVENT_TRACE_LOGFILE EVENT_TRACE_LOGFILEW
#define PEVENT_TRACE_LOGFILE PEVENT_TRACE_LOGFILEW
#define KERNEL_LOGGER_NAME KERNEL_LOGGER_NAMEW
#define GLOBAL_LOGGER_NAME GLOBAL_LOGGER_NAMEW
#define EVENT_LOGGER_NAME EVENT_LOGGER_NAMEW

#else

#define PEVENT_TRACE_BUFFER_CALLBACK PEVENT_TRACE_BUFFER_CALLBACKA
#define EVENT_TRACE_LOGFILE EVENT_TRACE_LOGFILEA
#define PEVENT_TRACE_LOGFILE PEVENT_TRACE_LOGFILEA
#define KERNEL_LOGGER_NAME KERNEL_LOGGER_NAMEA
#define GLOBAL_LOGGER_NAME GLOBAL_LOGGER_NAMEA
#define EVENT_LOGGER_NAME EVENT_LOGGER_NAMEA

#endif /* defined(_UNICODE) || defined(UNICODE) */

typedef enum _TRACE_QUERY_INFO_CLASS {
  TraceGuidQueryList,
  TraceGuidQueryInfo,
  TraceGuidQueryProcess,
  TraceStackTracingInfo,
  MaxTraceSetInfoClass
} TRACE_QUERY_INFO_CLASS, TRACE_INFO_CLASS;

typedef struct _CLASSIC_EVENT_ID {
  GUID EventGuid;
  UCHAR Type;
  UCHAR Reserved[7];
} CLASSIC_EVENT_ID, *PCLASSIC_EVENT_ID;

typedef struct _ENABLE_TRACE_PARAMETERS {
  ULONG Version;
  ULONG EnableProperty;
  ULONG ControlFlags;
  GUID SourceId;
  PEVENT_FILTER_DESCRIPTOR EnableFilterDesc;
} ENABLE_TRACE_PARAMETERS, *PENABLE_TRACE_PARAMETERS;

#define INVALID_PROCESSTRACE_HANDLE ((TRACEHANDLE)(ULONG_PTR)INVALID_HANDLE_VALUE)

#if defined(UNICODE) || defined(_UNICODE)

#define RegisterTraceGuids RegisterTraceGuidsW
#define StartTrace StartTraceW
#define ControlTrace ControlTraceW

#if defined(__TRACE_W2K_COMPATIBLE)

#define StopTrace(a,b,c) ControlTraceW((a),(b),(c), EVENT_TRACE_CONTROL_STOP)
#define QueryTrace(a,b,c) ControlTraceW((a),(b),(c), EVENT_TRACE_CONTROL_QUERY)
#define UpdateTrace(a,b,c) ControlTraceW((a),(b),(c), EVENT_TRACE_CONTROL_UPDATE)

#else

#define StopTrace StopTraceW
#define QueryTrace QueryTraceW
#define UpdateTrace UpdateTraceW

#endif /* defined(__TRACE_W2K_COMPATIBLE) */

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define FlushTrace FlushTraceW
#endif

#define QueryAllTraces QueryAllTracesW
#define OpenTrace OpenTraceW

#else /* defined(UNICODE) || defined(_UNICODE) */

#define RegisterTraceGuids RegisterTraceGuidsA
#define StartTrace StartTraceA
#define ControlTrace ControlTraceA

#if defined(__TRACE_W2K_COMPATIBLE)

#define StopTrace(a,b,c) ControlTraceA((a),(b),(c), EVENT_TRACE_CONTROL_STOP)
#define QueryTrace(a,b,c) ControlTraceA((a),(b),(c), EVENT_TRACE_CONTROL_QUERY)
#define UpdateTrace(a,b,c) ControlTraceA((a),(b),(c), EVENT_TRACE_CONTROL_UPDATE)

#else

#define StopTrace StopTraceA
#define QueryTrace QueryTraceA
#define UpdateTrace UpdateTraceA

#endif /* defined(__TRACE_W2K_COMPATIBLE) */

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define FlushTrace FlushTraceA
#endif

#define QueryAllTraces QueryAllTracesA
#define OpenTrace OpenTraceA

#endif /* defined(UNICODE) || defined(_UNICODE) */

EXTERN_C
ULONG
WMIAPI
StartTraceW(
  OUT PTRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
StartTraceA(
  OUT PTRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
StopTraceW(
  IN TRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
StopTraceA(
  IN TRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
QueryTraceW(
  IN TRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
QueryTraceA(
  IN TRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
UpdateTraceW(
  IN TRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
UpdateTraceA(
  IN TRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
ControlTraceW(
  IN TRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties,
  IN ULONG ControlCode);

EXTERN_C
ULONG
WMIAPI
ControlTraceA(
  IN TRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties,
  IN ULONG ControlCode);

EXTERN_C
ULONG
WMIAPI
QueryAllTracesW(
  OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
  IN ULONG PropertyArrayCount,
  OUT PULONG LoggerCount);

EXTERN_C
ULONG
WMIAPI
QueryAllTracesA(
  OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
  IN ULONG PropertyArrayCount,
  OUT PULONG LoggerCount);

EXTERN_C
ULONG
WMIAPI
EnableTrace(
  IN ULONG Enable,
  IN ULONG EnableFlag,
  IN ULONG EnableLevel,
  IN LPCGUID ControlGuid,
  IN TRACEHANDLE TraceHandle);

EXTERN_C
ULONG
WMIAPI
CreateTraceInstanceId(
  IN HANDLE RegHandle,
  IN OUT PEVENT_INSTANCE_INFO InstInfo);

EXTERN_C
ULONG
WMIAPI
TraceEvent(
  IN TRACEHANDLE TraceHandle,
  IN PEVENT_TRACE_HEADER EventTrace);

EXTERN_C
ULONG
WMIAPI
TraceEventInstance(
  IN TRACEHANDLE TraceHandle,
  IN PEVENT_INSTANCE_HEADER EventTrace,
  IN PEVENT_INSTANCE_INFO InstInfo,
  IN PEVENT_INSTANCE_INFO ParentInstInfo OPTIONAL);

EXTERN_C
ULONG
WMIAPI
RegisterTraceGuidsW(
  IN WMIDPREQUEST RequestAddress,
  IN PVOID RequestContext OPTIONAL,
  IN LPCGUID ControlGuid,
  IN ULONG GuidCount,
  IN PTRACE_GUID_REGISTRATION TraceGuidReg OPTIONAL,
  IN LPCWSTR MofImagePath OPTIONAL,
  IN LPCWSTR MofResourceName OPTIONAL,
  OUT PTRACEHANDLE RegistrationHandle);

EXTERN_C
ULONG
WMIAPI
RegisterTraceGuidsA(
  IN WMIDPREQUEST RequestAddress,
  IN PVOID RequestContext OPTIONAL,
  IN LPCGUID ControlGuid,
  IN ULONG GuidCount,
  IN PTRACE_GUID_REGISTRATION TraceGuidReg OPTIONAL,
  IN LPCSTR MofImagePath OPTIONAL,
  IN LPCSTR MofResourceName OPTIONAL,
  OUT PTRACEHANDLE RegistrationHandle);

EXTERN_C
ULONG
WMIAPI
UnregisterTraceGuids(
  IN TRACEHANDLE RegistrationHandle);

EXTERN_C
TRACEHANDLE
WMIAPI
GetTraceLoggerHandle(
  IN PVOID Buffer);

EXTERN_C
UCHAR
WMIAPI
GetTraceEnableLevel(
  IN TRACEHANDLE TraceHandle);

EXTERN_C
ULONG
WMIAPI
GetTraceEnableFlags(
  IN TRACEHANDLE TraceHandle);

EXTERN_C
TRACEHANDLE
WMIAPI
OpenTraceA(
  IN OUT PEVENT_TRACE_LOGFILEA Logfile);

EXTERN_C
TRACEHANDLE
WMIAPI
OpenTraceW(
  IN OUT PEVENT_TRACE_LOGFILEW Logfile);

EXTERN_C
ULONG
WMIAPI
ProcessTrace(
  IN PTRACEHANDLE HandleArray,
  IN ULONG HandleCount,
  IN LPFILETIME StartTime OPTIONAL,
  IN LPFILETIME EndTime OPTIONAL);

EXTERN_C
ULONG
WMIAPI
CloseTrace(
  IN TRACEHANDLE TraceHandle);

EXTERN_C
ULONG
WMIAPI
SetTraceCallback(
  IN LPCGUID pGuid,
  IN PEVENT_CALLBACK EventCallback);

EXTERN_C
ULONG
WMIAPI
RemoveTraceCallback(
  IN LPCGUID pGuid);

EXTERN_C
ULONG
__cdecl
TraceMessage(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPCGUID MessageGuid,
  IN USHORT MessageNumber,
  ...);

EXTERN_C
ULONG
WMIAPI
TraceMessageVa(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPCGUID MessageGuid,
  IN USHORT MessageNumber,
  IN va_list MessageArgList);

#if (WINVER >= _WIN32_WINNT_WINXP)

EXTERN_C
ULONG
WMIAPI
EnumerateTraceGuids(
  IN OUT PTRACE_GUID_PROPERTIES *GuidPropertiesArray,
  IN ULONG PropertyArrayCount,
  OUT PULONG GuidCount);

EXTERN_C
ULONG
WMIAPI
FlushTraceW(
  IN TRACEHANDLE TraceHandle,
  IN LPCWSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

EXTERN_C
ULONG
WMIAPI
FlushTraceA(
  IN TRACEHANDLE TraceHandle,
  IN LPCSTR InstanceName OPTIONAL,
  IN OUT PEVENT_TRACE_PROPERTIES Properties);

#endif /* (WINVER >= _WIN32_WINNT_WINXP) */

#if (WINVER >= _WIN32_WINNT_VISTA)

EXTERN_C
ULONG
WMIAPI
EnableTraceEx(
  IN LPCGUID ProviderId,
  IN LPCGUID SourceId OPTIONAL,
  IN TRACEHANDLE TraceHandle,
  IN ULONG IsEnabled,
  IN UCHAR Level,
  IN ULONGLONG MatchAnyKeyword,
  IN ULONGLONG MatchAllKeyword,
  IN ULONG EnableProperty,
  IN PEVENT_FILTER_DESCRIPTOR EnableFilterDesc OPTIONAL);

EXTERN_C
ULONG
WMIAPI
EnumerateTraceGuidsEx(
  IN TRACE_QUERY_INFO_CLASS TraceQueryInfoClass,
  IN PVOID InBuffer OPTIONAL,
  IN ULONG InBufferSize,
  OUT PVOID OutBuffer OPTIONAL,
  IN ULONG OutBufferSize,
  OUT PULONG ReturnLength);

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

#if (WINVER >= _WIN32_WINNT_WIN7)

EXTERN_C
ULONG
WMIAPI
EnableTraceEx2(
  IN TRACEHANDLE TraceHandle,
  IN LPCGUID ProviderId,
  IN ULONG ControlCode,
  IN UCHAR Level,
  IN ULONGLONG MatchAnyKeyword,
  IN ULONGLONG MatchAllKeyword,
  IN ULONG Timeout,
  IN PENABLE_TRACE_PARAMETERS EnableParameters OPTIONAL);

EXTERN_C
ULONG
WMIAPI
TraceSetInformation(
  IN TRACEHANDLE SessionHandle,
  IN TRACE_INFO_CLASS InformationClass,
  IN PVOID TraceInformation,
  IN ULONG InformationLength);

#endif /* (WINVER >= _WIN32_WINNT_WIN7) */

#endif /* !defined(_WMIKM_) && !defined(_NTDDK_) && !defined(_NTIFS_) */

#ifdef __cplusplus
} /* extern "C" */
#endif
