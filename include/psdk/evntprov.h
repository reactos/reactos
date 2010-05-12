#pragma once

#ifndef EVNTAPI
#ifndef MIDL_PASS
#ifdef _EVNT_SOURCE_
#define EVNTAPI __stdcall
#else
#define EVNTAPI DECLSPEC_IMPORT __stdcall
#endif /* _EVNT_SOURCE_ */
#endif /* MIDL_PASS */
#endif /* EVNTAPI */

#ifdef __cplusplus
extern "C" {
#endif

#define EVENT_MIN_LEVEL                      (0)
#define EVENT_MAX_LEVEL                      (0xff)

#define EVENT_ACTIVITY_CTRL_GET_ID           (1)
#define EVENT_ACTIVITY_CTRL_SET_ID           (2)
#define EVENT_ACTIVITY_CTRL_CREATE_ID        (3)
#define EVENT_ACTIVITY_CTRL_GET_SET_ID       (4)
#define EVENT_ACTIVITY_CTRL_CREATE_SET_ID    (5)

typedef ULONGLONG REGHANDLE, *PREGHANDLE;

#define MAX_EVENT_DATA_DESCRIPTORS           (128)
#define MAX_EVENT_FILTER_DATA_SIZE           (1024)

#define EVENT_FILTER_TYPE_SCHEMATIZED        (0x80000000)

typedef struct _EVENT_DATA_DESCRIPTOR {
  ULONGLONG Ptr;
  ULONG Size;
  ULONG Reserved;
} EVENT_DATA_DESCRIPTOR, *PEVENT_DATA_DESCRIPTOR;

typedef struct _EVENT_DESCRIPTOR {
  USHORT Id;
  UCHAR Version;
  UCHAR Channel;
  UCHAR Level;
  UCHAR Opcode;
  USHORT Task;
  ULONGLONG Keyword;
} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;
typedef const EVENT_DESCRIPTOR *PCEVENT_DESCRIPTOR;

typedef struct _EVENT_FILTER_DESCRIPTOR {
  ULONGLONG Ptr;
  ULONG Size;
  ULONG Type;
} EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;

typedef struct _EVENT_FILTER_HEADER {
  USHORT Id;
  UCHAR Version;
  UCHAR Reserved[5];
  ULONGLONG InstanceId;
  ULONG Size;
  ULONG NextOffset;
} EVENT_FILTER_HEADER, *PEVENT_FILTER_HEADER;

#ifndef _ETW_KM_

typedef VOID
(NTAPI *PENABLECALLBACK)(
  IN LPCGUID SourceId,
  IN ULONG IsEnabled,
  IN UCHAR Level,
  IN ULONGLONG MatchAnyKeyword,
  IN ULONGLONG MatchAllKeyword,
  IN PEVENT_FILTER_DESCRIPTOR FilterData OPTIONAL,
  IN OUT PVOID CallbackContext OPTIONAL);

#if (WINVER >= _WIN32_WINNT_VISTA)
ULONG
EVNTAPI
EventRegister(
  IN LPCGUID ProviderId,
  IN PENABLECALLBACK EnableCallback OPTIONAL,
  IN PVOID CallbackContext OPTIONAL,
  OUT PREGHANDLE RegHandle);

ULONG
EVNTAPI
EventUnregister(
  IN REGHANDLE RegHandle);

BOOLEAN
EVNTAPI
EventEnabled(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor);

BOOLEAN
EVNTAPI
EventProviderEnabled(
  IN REGHANDLE RegHandle,
  IN UCHAR Level,
  IN ULONGLONG Keyword);

ULONG
EVNTAPI
EventWrite(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR UserData);

ULONG
EVNTAPI
EventWriteTransfer(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN LPCGUID ActivityId OPTIONAL,
  IN LPCGUID RelatedActivityId OPTIONAL,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR UserData OPTIONAL);

ULONG
EVNTAPI
EventWriteString(
  IN REGHANDLE RegHandle,
  IN UCHAR Level,
  IN ULONGLONG Keyword,
  IN PCWSTR String);

ULONG
EVNTAPI
EventActivityIdControl(
  IN ULONG ControlCode,
  IN OUT LPGUID ActivityId);

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

#if (WINVER >= _WIN32_WINNT_WIN7)
ULONG
EVNTAPI
EventWriteEx(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN ULONG64 Filter,
  IN ULONG Flags,
  IN LPCGUID ActivityId OPTIONAL,
  IN LPCGUID RelatedActivityId OPTIONAL,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR UserData OPTIONAL);
#endif

#endif // _ETW_KM_ 

FORCEINLINE
VOID
EventDataDescCreate(
  OUT PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
  IN const VOID* DataPtr,
  IN ULONG DataSize)
{
  EventDataDescriptor->Ptr = (ULONGLONG)(ULONG_PTR)DataPtr;
  EventDataDescriptor->Size = DataSize;
  EventDataDescriptor->Reserved = 0;
}

FORCEINLINE
VOID
EventDescCreate(
  OUT PEVENT_DESCRIPTOR EventDescriptor,
  IN USHORT Id,
  IN UCHAR Version,
  IN UCHAR Channel,
  IN UCHAR Level,
  IN USHORT Task,
  IN UCHAR Opcode,
  IN ULONGLONG Keyword)
{
  EventDescriptor->Id = Id;
  EventDescriptor->Version = Version;
  EventDescriptor->Channel = Channel;
  EventDescriptor->Level = Level;
  EventDescriptor->Task = Task;
  EventDescriptor->Opcode = Opcode;
  EventDescriptor->Keyword = Keyword;
}

FORCEINLINE
VOID
EventDescZero(
  OUT PEVENT_DESCRIPTOR EventDescriptor)
{
  memset(EventDescriptor, 0, sizeof(EVENT_DESCRIPTOR));
}

FORCEINLINE
USHORT
EventDescGetId(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Id);
}

FORCEINLINE
UCHAR
EventDescGetVersion(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Version);
}

FORCEINLINE
USHORT
EventDescGetTask(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Task);
}

FORCEINLINE
UCHAR
EventDescGetOpcode(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Opcode);
}

FORCEINLINE
UCHAR
EventDescGetChannel(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Channel);
}

FORCEINLINE
UCHAR
EventDescGetLevel(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Level);
}

FORCEINLINE
ULONGLONG
EventDescGetKeyword(
  IN PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Keyword);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetId(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN USHORT Id)
{
  EventDescriptor->Id = Id;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetVersion(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN UCHAR Version)
{
  EventDescriptor->Version = Version;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetTask(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN USHORT Task)
{
  EventDescriptor->Task = Task;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetOpcode(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN UCHAR Opcode)
{
  EventDescriptor->Opcode = Opcode;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetLevel(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN UCHAR  Level)
{
  EventDescriptor->Level = Level;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetChannel(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN UCHAR Channel)
{
  EventDescriptor->Channel = Channel;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetKeyword(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN ULONGLONG Keyword)
{
  EventDescriptor->Keyword = Keyword;
  return (EventDescriptor);
}


FORCEINLINE
PEVENT_DESCRIPTOR
EventDescOrKeyword(
  IN PEVENT_DESCRIPTOR EventDescriptor,
  IN ULONGLONG Keyword)
{
  EventDescriptor->Keyword |= Keyword;
  return (EventDescriptor);
}

#ifdef __cplusplus
}
#endif

