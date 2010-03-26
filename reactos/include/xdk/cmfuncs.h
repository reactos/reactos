/******************************************************************************
 *                         Configuration Manager Functions                    *
 ******************************************************************************/

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
NTSTATUS
NTAPI
CmRegisterCallback(
  IN PEX_CALLBACK_FUNCTION Function,
  IN PVOID Context OPTIONAL,
  OUT PLARGE_INTEGER Cookie);

NTKERNELAPI
NTSTATUS
NTAPI
CmUnRegisterCallback(
  IN LARGE_INTEGER Cookie);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
NTSTATUS
NTAPI
CmRegisterCallbackEx(
  PEX_CALLBACK_FUNCTION Function,
  PCUNICODE_STRING Altitude,
  PVOID Driver,
  PVOID Context,
  PLARGE_INTEGER Cookie,
  PVOID Reserved);

NTKERNELAPI
VOID
NTAPI
CmGetCallbackVersion(
  OUT PULONG Major OPTIONAL,
  OUT PULONG Minor OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
CmSetCallbackObjectContext(
  IN OUT PVOID Object,
  IN PLARGE_INTEGER Cookie,
  IN PVOID NewContext,
  OUT PVOID *OldContext OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
CmCallbackGetKeyObjectID(
  IN PLARGE_INTEGER Cookie,
  IN PVOID Object,
  OUT PULONG_PTR ObjectID OPTIONAL,
  OUT PCUNICODE_STRING *ObjectName OPTIONAL);

NTKERNELAPI
PVOID
NTAPI
CmGetBoundTransaction(
  IN PLARGE_INTEGER Cookie,
  IN PVOID Object);

#endif // NTDDI_VERSION >= NTDDI_VISTA

$endif

