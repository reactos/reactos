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

$endif

