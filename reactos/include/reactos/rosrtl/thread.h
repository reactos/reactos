/* $Id: thread.h,v 1.5 2004/10/24 20:37:26 weiden Exp $
 */

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS NTAPI RtlRosCreateUserThread
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
);

NTSTATUS CDECL RtlRosCreateUserThreadVa
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 ...
);

__declspec(noreturn) VOID NTAPI RtlRosExitUserThread
(
 IN NTSTATUS Status
);

NTSTATUS NTAPI RtlRosInitializeContext
(
 IN HANDLE ProcessHandle,
 OUT PCONTEXT ThreadContext,
 IN PVOID ThreadStartAddress,
 IN PINITIAL_TEB InitialTeb,
 IN ULONG ParameterCount,
 IN ULONG_PTR *Parameters
);

NTSTATUS NTAPI RtlRosCreateStack
(
 IN HANDLE ProcessHandle,
 OUT PINITIAL_TEB InitialTeb,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL
);

NTSTATUS NTAPI RtlRosDeleteStack
(
 IN HANDLE ProcessHandle,
 IN PINITIAL_TEB InitialTeb
);

NTSTATUS NTAPI RtlRosFreeUserThreadStack
(
 IN HANDLE ProcessHandle,
 IN HANDLE ThreadHandle
);

NTSTATUS NTAPI RtlRosSwitchStackForExit
(
 IN PVOID StackBase,
 IN SIZE_T StackSize,
 IN VOID (NTAPI * ExitRoutine)(ULONG_PTR Parameter),
 IN ULONG_PTR Parameter
);

/* Private functions - for ROSRTL internal use only */
NTSTATUS NTAPI RtlpRosGetStackLimits
(
 IN PINITIAL_TEB InitialTeb,
 OUT PVOID * StackBase,
 OUT PVOID * StackLimit
);

NTSTATUS NTAPI RtlpRosValidateLinearUserStack
(
 IN PVOID StackBase,
 IN PVOID StackLimit,
 IN BOOLEAN Direction
);

#define RtlpRosValidateTopDownUserStack(__B__, __L__) \
 (RtlpRosValidateLinearUserStack((__B__), (__L__), FALSE))

#define RtlpRosValidateDownTopUserStack(__B__, __L__) \
 (RtlpRosValidateLinearUserStack((__B__), (__L__), TRUE))

#ifdef __cplusplus
}
#endif

/* EOF */
