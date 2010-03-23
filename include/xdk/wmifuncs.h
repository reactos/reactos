/******************************************************************************
 *                          WMI Library Support Functions                     *
 ******************************************************************************/

#ifdef RUN_WPP
#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
__cdecl
WmiTraceMessage(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPGUID MessageGuid,
  IN USHORT MessageNumber,
  IN ...);
#endif
#endif /* RUN_WPP */

 #if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
WmiQueryTraceInformation(
  IN TRACE_INFORMATION_CLASS TraceInformationClass,
  OUT PVOID TraceInformation,
  IN ULONG TraceInformationLength,
  OUT PULONG RequiredLength OPTIONAL,
  IN PVOID Buffer OPTIONAL);

#if 0
/* FIXME: Get va_list from where? */
NTKERNELAPI
NTSTATUS
__cdecl
WmiTraceMessageVa(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPGUID MessageGuid,
  IN USHORT MessageNumber,
  IN va_list MessageArgList);
#endif

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#ifndef TRACE_INFORMATION_CLASS_DEFINE

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
NTAPI
WmiQueryTraceInformation(
  IN TRACE_INFORMATION_CLASS TraceInformationClass,
  OUT PVOID TraceInformation,
  IN ULONG TraceInformationLength,
  OUT PULONG RequiredLength OPTIONAL,
  IN PVOID Buffer OPTIONAL);
#endif

#define TRACE_INFORMATION_CLASS_DEFINE

#endif /* TRACE_INFOPRMATION_CLASS_DEFINE */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSTATUS
NTKERNELAPI
NTAPI
EtwRegister(
  IN LPCGUID ProviderId,
  IN PETWENABLECALLBACK EnableCallback OPTIONAL,
  IN PVOID CallbackContext OPTIONAL,
  OUT PREGHANDLE RegHandle);

NTSTATUS
NTKERNELAPI
NTAPI
EtwUnregister(
  IN REGHANDLE RegHandle);

BOOLEAN
NTKERNELAPI
NTAPI
EtwEventEnabled(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor);

BOOLEAN
NTKERNELAPI
NTAPI
EtwProviderEnabled(
  IN REGHANDLE RegHandle,
  IN UCHAR Level,
  IN ULONGLONG Keyword);

NTSTATUS
NTKERNELAPI
NTAPI
EtwActivityIdControl(
  IN ULONG ControlCode,
  IN OUT LPGUID ActivityId);

NTSTATUS
NTKERNELAPI
NTAPI
EtwWrite(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN LPCGUID ActivityId OPTIONAL,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR  UserData OPTIONAL);

NTSTATUS
NTKERNELAPI
NTAPI
EtwWriteTransfer(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN LPCGUID ActivityId OPTIONAL,
  IN LPCGUID RelatedActivityId OPTIONAL,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR UserData OPTIONAL);

NTSTATUS
NTKERNELAPI
NTAPI
EtwWriteString(
  IN REGHANDLE RegHandle,
  IN UCHAR Level,
  IN ULONGLONG Keyword,
  IN LPCGUID ActivityId OPTIONAL,
  IN PCWSTR String);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTSTATUS
NTKERNELAPI
NTAPI
EtwWriteEx(
  IN REGHANDLE RegHandle,
  IN PCEVENT_DESCRIPTOR EventDescriptor,
  IN ULONG64 Filter,
  IN ULONG Flags,
  IN LPCGUID ActivityId OPTIONAL,
  IN LPCGUID RelatedActivityId OPTIONAL,
  IN ULONG UserDataCount,
  IN PEVENT_DATA_DESCRIPTOR UserData OPTIONAL);
#endif



