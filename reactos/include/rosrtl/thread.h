/* $Id: thread.h,v 1.3 2003/07/22 20:10:04 hyperion Exp $
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

NTSTATUS NTAPI RtlRosInitializeContext
(
 IN HANDLE ProcessHandle,
 OUT PCONTEXT Context,
 IN PVOID StartAddress,
 IN PUSER_STACK UserStack,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
);

NTSTATUS NTAPI RtlRosCreateStack
(
 IN HANDLE ProcessHandle,
 OUT PUSER_STACK UserStack,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL
);

NTSTATUS NTAPI RtlRosDeleteStack
(
 IN HANDLE ProcessHandle,
 IN PUSER_STACK UserStack
);

/* Private functions - for ROSRTL internal use only */
NTSTATUS NTAPI RtlpRosGetStackLimits
(
 IN PUSER_STACK UserStack,
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
