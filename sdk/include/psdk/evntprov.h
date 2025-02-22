/*
 * evntprov.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Amine Khaldi.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#define _EVNTPROV_

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

#ifndef _EVNTRACE_
typedef struct _EVENT_FILTER_DESCRIPTOR {
  ULONGLONG Ptr;
  ULONG Size;
  ULONG Type;
} EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;
#endif

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
  _In_ LPCGUID SourceId,
  _In_ ULONG IsEnabled,
  _In_ UCHAR Level,
  _In_ ULONGLONG MatchAnyKeyword,
  _In_ ULONGLONG MatchAllKeyword,
  _In_opt_ PEVENT_FILTER_DESCRIPTOR FilterData,
  _Inout_opt_ PVOID CallbackContext);

#if (WINVER >= _WIN32_WINNT_VISTA)
ULONG
EVNTAPI
EventRegister(
  _In_ LPCGUID ProviderId,
  _In_opt_ PENABLECALLBACK EnableCallback,
  _In_opt_ PVOID CallbackContext,
  _Out_ PREGHANDLE RegHandle);

ULONG
EVNTAPI
EventUnregister(
  _In_ REGHANDLE RegHandle);

BOOLEAN
EVNTAPI
EventEnabled(
  _In_ REGHANDLE RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor);

BOOLEAN
EVNTAPI
EventProviderEnabled(
  _In_ REGHANDLE RegHandle,
  _In_ UCHAR Level,
  _In_ ULONGLONG Keyword);

ULONG
EVNTAPI
EventWrite(
  _In_ REGHANDLE RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor,
  _In_ ULONG UserDataCount,
  _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData);

ULONG
EVNTAPI
EventWriteTransfer(
  _In_ REGHANDLE RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor,
  _In_opt_ LPCGUID ActivityId,
  _In_opt_ LPCGUID RelatedActivityId,
  _In_ ULONG UserDataCount,
  _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData);

ULONG
EVNTAPI
EventWriteString(
  _In_ REGHANDLE RegHandle,
  _In_ UCHAR Level,
  _In_ ULONGLONG Keyword,
  _In_ PCWSTR String);

ULONG
EVNTAPI
EventActivityIdControl(
  _In_ ULONG ControlCode,
  _Inout_ LPGUID ActivityId);

#endif /* (WINVER >= _WIN32_WINNT_VISTA) */

#if (WINVER >= _WIN32_WINNT_WIN7)
ULONG
EVNTAPI
EventWriteEx(
  _In_ REGHANDLE RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor,
  _In_ ULONG64 Filter,
  _In_ ULONG Flags,
  _In_opt_ LPCGUID ActivityId,
  _In_opt_ LPCGUID RelatedActivityId,
  _In_ ULONG UserDataCount,
  _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData);
#endif

#endif // _ETW_KM_

FORCEINLINE
VOID
EventDataDescCreate(
  _Out_ PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
  _In_reads_bytes_(DataSize) const VOID* DataPtr,
  _In_ ULONG DataSize)
{
  EventDataDescriptor->Ptr = (ULONGLONG)(ULONG_PTR)DataPtr;
  EventDataDescriptor->Size = DataSize;
  EventDataDescriptor->Reserved = 0;
}

FORCEINLINE
VOID
EventDescCreate(
  _Out_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ USHORT Id,
  _In_ UCHAR Version,
  _In_ UCHAR Channel,
  _In_ UCHAR Level,
  _In_ USHORT Task,
  _In_ UCHAR Opcode,
  _In_ ULONGLONG Keyword)
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
  _Out_ PEVENT_DESCRIPTOR EventDescriptor)
{
  memset(EventDescriptor, 0, sizeof(EVENT_DESCRIPTOR));
}

FORCEINLINE
USHORT
EventDescGetId(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Id);
}

FORCEINLINE
UCHAR
EventDescGetVersion(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Version);
}

FORCEINLINE
USHORT
EventDescGetTask(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Task);
}

FORCEINLINE
UCHAR
EventDescGetOpcode(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Opcode);
}

FORCEINLINE
UCHAR
EventDescGetChannel(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Channel);
}

FORCEINLINE
UCHAR
EventDescGetLevel(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Level);
}

FORCEINLINE
ULONGLONG
EventDescGetKeyword(
  _In_ PCEVENT_DESCRIPTOR EventDescriptor)
{
  return (EventDescriptor->Keyword);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetId(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ USHORT Id)
{
  EventDescriptor->Id = Id;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetVersion(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ UCHAR Version)
{
  EventDescriptor->Version = Version;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetTask(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ USHORT Task)
{
  EventDescriptor->Task = Task;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetOpcode(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ UCHAR Opcode)
{
  EventDescriptor->Opcode = Opcode;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetLevel(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ UCHAR  Level)
{
  EventDescriptor->Level = Level;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetChannel(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ UCHAR Channel)
{
  EventDescriptor->Channel = Channel;
  return (EventDescriptor);
}

FORCEINLINE
PEVENT_DESCRIPTOR
EventDescSetKeyword(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ ULONGLONG Keyword)
{
  EventDescriptor->Keyword = Keyword;
  return (EventDescriptor);
}


FORCEINLINE
PEVENT_DESCRIPTOR
EventDescOrKeyword(
  _In_ PEVENT_DESCRIPTOR EventDescriptor,
  _In_ ULONGLONG Keyword)
{
  EventDescriptor->Keyword |= Keyword;
  return (EventDescriptor);
}

#ifdef __cplusplus
}
#endif
