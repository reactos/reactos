/******************************************************************************
 *                          WMI Library Support Functions                     *
 ******************************************************************************/

#ifdef RUN_WPP

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
DDKCDECLAPI
WmiTraceMessage(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPGUID MessageGuid,
  IN USHORT MessageNumber,
  IN ...);
#endif

#endif

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
DDKCDECLAPI
WmiTraceMessageVa(
  IN TRACEHANDLE LoggerHandle,
  IN ULONG MessageFlags,
  IN LPGUID MessageGuid,
  IN USHORT MessageNumber,
  IN va_list MessageArgList);
#endif

#endif

