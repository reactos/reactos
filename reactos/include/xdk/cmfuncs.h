/******************************************************************************
 *                         Configuration Manager Functions                    *
 ******************************************************************************/

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
CmRegisterCallback(
  _In_ PEX_CALLBACK_FUNCTION Function,
  _In_opt_ PVOID Context,
  _Out_ PLARGE_INTEGER Cookie);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
CmUnRegisterCallback(
  _In_ LARGE_INTEGER Cookie);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
CmRegisterCallbackEx(
  _In_ PEX_CALLBACK_FUNCTION Function,
  _In_ PCUNICODE_STRING Altitude,
  _In_ PVOID Driver,
  _In_opt_ PVOID Context,
  _Out_ PLARGE_INTEGER Cookie,
  _Reserved_ PVOID Reserved);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
CmGetCallbackVersion(
  _Out_opt_ PULONG Major,
  _Out_opt_ PULONG Minor);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
CmSetCallbackObjectContext(
  _Inout_ PVOID Object,
  _In_ PLARGE_INTEGER Cookie,
  _In_ PVOID NewContext,
  _Out_opt_ PVOID *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
CmCallbackGetKeyObjectID(
  _In_ PLARGE_INTEGER Cookie,
  _In_ PVOID Object,
  _Out_opt_ PULONG_PTR ObjectID,
  _Outptr_opt_ PCUNICODE_STRING *ObjectName);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
CmGetBoundTransaction(
  _In_ PLARGE_INTEGER Cookie,
  _In_ PVOID Object);

#endif // NTDDI_VERSION >= NTDDI_VISTA

$endif (_WDMDDK_)

