
#pragma once
#define _NTOSIFS_

#ifdef __cplusplus
extern "C" {
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA) || defined(__REACTOS__)

_Must_inspect_result_
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PVOID
NTAPI
KeAllocateCalloutStack(
    _In_ BOOLEAN LargeStack);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeFreeCalloutStack (
    _In_ PVOID Context);

#endif // (NTDDI_VERSION >= NTDDI_VISTA) || defined(__REACTOS__)

#if (NTDDI_VERSION >= NTDDI_WIN7) || defined(__REACTOS__)

typedef enum _KSTACK_TYPE
{
    ReserveStackNormal = 0,
    ReserveStackLarge,
    MaximumReserveStacks
} KSTACK_TYPE;

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
KeAllocateCalloutStackEx(
    _In_ _Strict_type_match_ KSTACK_TYPE StackType,
    _In_ UCHAR RecursionDepth,
    _Reserved_ SIZE_T Reserved,
    _Outptr_ PVOID *StackContext);

#endif // (NTDDI_VERSION >= NTDDI_WIN7) || defined(__REACTOS__)

#ifdef __cplusplus
} // extern "C"
#endif
